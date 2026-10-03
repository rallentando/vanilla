#include "switch.hpp"
#include "const.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include "treebank.hpp"

#include <QNetworkCookie>
#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QPointer>
#include <QClipboard>
#include <QTimerEvent>

#include "page.hpp"
#include "webelement.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "dialog.hpp"

void EdgeWebView::PullCookiesIntoJar(){
    if(m_Impl->m_PrivateMode) return;
    if(!m_Impl->m_WebView || !page()) return;

    NetworkAccessManager *nam = page()->GetNetworkAccessManager();
    if(!nam || !nam->GetNetworkCookieJar()) return;

    ComPtr<ICoreWebView2_2> webview2;
    if(FAILED(m_Impl->m_WebView.As(&webview2)) || !webview2) return;

    ComPtr<ICoreWebView2CookieManager> cookies;
    if(FAILED(webview2->get_CookieManager(&cookies)) || !cookies) return;

    NetworkCookieJar *jar = nam->GetNetworkCookieJar();

    const QString uri = m_Impl->m_Url.toString();
    if(uri.isEmpty()) return;

    const QUrl scope = m_Impl->m_Url;
    const QString source = QStringLiteral("edge:") + ProfileKey();
    const quint64 turn = jar->BeginMirrorScope(source, scope);
    const int ticket = s_Profiles.Cookies().Ticket();
    const QString key = ProfileKey();

    cookies->GetCookies
        (reinterpret_cast<PCWSTR>(uri.utf16()),
         Callback<ICoreWebView2GetCookiesCompletedHandler>
         ([jar, scope, source, turn, ticket, key](HRESULT result, ICoreWebView2CookieList *list) -> HRESULT {
             if(!s_Profiles.Cookies().MayWrite(ticket, key)) return S_OK;
             if(FAILED(result) || !list) return S_OK;

             UINT count = 0;
             list->get_Count(&count);

             QList<QNetworkCookie> gathered;
             for(UINT i = 0; i < count; i++){
                 ComPtr<ICoreWebView2Cookie> cookie;
                 if(FAILED(list->GetValueAtIndex(i, &cookie)) || !cookie) continue;

                 LPWSTR name = nullptr;
                 LPWSTR value = nullptr;
                 LPWSTR domain = nullptr;
                 LPWSTR path = nullptr;
                 cookie->get_Name(&name);
                 cookie->get_Value(&value);
                 cookie->get_Domain(&domain);
                 cookie->get_Path(&path);

                 if(name && value && domain){
                     double expires = -1;
                     if(FAILED(cookie->get_Expires(&expires))) expires = -1;

                     BOOL httpOnly = FALSE;
                     BOOL secure = FALSE;
                     if(FAILED(cookie->get_IsHttpOnly(&httpOnly))) httpOnly = FALSE;
                     if(FAILED(cookie->get_IsSecure(&secure))) secure = FALSE;

                     gathered.append
                         (EdgeCookieFromParts
                          (QString::fromWCharArray(name),
                           QString::fromWCharArray(value),
                           QString::fromWCharArray(domain),
                           path ? QString::fromWCharArray(path) : QString(),
                           expires, httpOnly ? true : false, secure ? true : false));
                 }

                 if(name) CoTaskMemFree(name);
                 if(value) CoTaskMemFree(value);
                 if(domain) CoTaskMemFree(domain);
                 if(path) CoTaskMemFree(path);
             }

             jar->MirrorScope(source, scope, turn, gathered);
             return S_OK;
         }).Get());
}

void EdgeWebView::CallWithScriptResult(const QString &script, VariantCallBack callBack,
                                       VoidCallBack discarded){
    if(!m_Impl->m_WebView){
        if(discarded) discarded();
        return;
    }
    if(!m_Impl->m_Document.HasLiveDocument()){
        if(discarded) discarded();
        return;
    }

    const int generation = m_Impl->m_Document.Generation();
    QPointer<EdgeWebView> alive(this);

    const HRESULT started = m_Impl->m_WebView->ExecuteScript
        (reinterpret_cast<PCWSTR>(script.utf16()),
         Callback<ICoreWebView2ExecuteScriptCompletedHandler>
         ([alive, generation, callBack, discarded](HRESULT result, PCWSTR json) -> HRESULT {
             if(!alive || !alive->m_Impl->m_Document.StillCurrent(generation) ||
                FAILED(result) || !json){
                 if(discarded) discarded();
                 return S_OK;
             }

             callBack(EdgeScriptResultToVariant
                      (QString::fromWCharArray(json).toUtf8()));
             return S_OK;
         }).Get());
    if(FAILED(started) && discarded) discarded();
}

qreal EdgeWebView::PageScale() const {
    if(!m_Impl->m_Controller) return 1.0;

    double scale = 1.0;
    ComPtr<ICoreWebView2Controller3> controller3;
    if(SUCCEEDED(m_Impl->m_Controller->QueryInterface(IID_PPV_ARGS(&controller3))) &&
       controller3)
        controller3->get_RasterizationScale(&scale);
    if(scale <= 0.0) scale = 1.0;

    double zoom = 1.0;
    if(FAILED(m_Impl->m_Controller->get_ZoomFactor(&zoom)) || zoom <= 0.0) zoom = 1.0;

    return scale * zoom;
}

QPoint EdgeWebView::PagePointOf(const QPoint &widgetPoint) const {
    const qreal ratio = m_Impl->m_HostWindow ? m_Impl->m_HostWindow->devicePixelRatio() : 1.0;
    const qreal scale = PageScale();
    if(scale <= 0.0) return widgetPoint;
    return QPoint(qRound(widgetPoint.x() * ratio / scale),
                  qRound(widgetPoint.y() * ratio / scale));
}

QRect EdgeWebView::WidgetRectOf(const QRect &pageRect) const {
    const qreal ratio = m_Impl->m_HostWindow ? m_Impl->m_HostWindow->devicePixelRatio() : 1.0;
    if(ratio == 1.0 || pageRect.isNull()) return pageRect;
    return QRect(qRound(pageRect.x()      / ratio), qRound(pageRect.y()      / ratio),
                 qRound(pageRect.width()  / ratio), qRound(pageRect.height() / ratio));
}

void EdgeWebView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                    VariantCallBack callBack){
    CallWithScriptResult(code, callBack);
}

void EdgeWebView::CallWithGotBaseUrl(UrlCallBack callBack){
    CallWithScriptResult(GetBaseUrlJsCode(), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toUrl() : QUrl());
    });
}

void EdgeWebView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    CallWithScriptResult(GetCurrentBaseUrlJsCode(), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toUrl() : QUrl());
    });
}

void EdgeWebView::CallWithFoundElements(Page::FindElementsOption option,
                                        WebElementListCallBack callBack){
    QPointer<EdgeWebView> alive(this);
    CallWithScriptResult(FindElementsJsCode(option), [alive, callBack](const QVariant &var){
        if(!alive || !var.isValid()) return callBack(SharedWebElementList());

        const QVariantList list = var.toMap().values();
        SharedWebElementList result;
        const QRect viewport = QRect(QPoint(), alive->size());

        for(int i = 0; i < list.length(); i++){
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(alive.data(), list[i]);
            const QRect rect = alive->WidgetRectOf(e->Rectangle());
            e->SetRectangle(viewport.intersects(rect) ? rect : QRect());
            result << e;
        }
        callBack(result);
    });
}

void EdgeWebView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(pos.isNull()) return callBack(SharedWebElement());
    QPointer<EdgeWebView> alive(this);
    CallWithScriptResult(HitElementJsCode(PagePointOf(pos)),
                         [alive, callBack](const QVariant &var){
        if(!alive || !var.isValid()) return callBack(SharedWebElement());
        std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
        *e = JsWebElement(alive.data(), var);
        e->SetRectangle(alive->WidgetRectOf(e->Rectangle()));
        callBack(e);
    });
}

void EdgeWebView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithScriptResult(HitLinkUrlJsCode(PagePointOf(pos)), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toUrl() : QUrl());
    });
}

void EdgeWebView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithScriptResult(HitImageUrlJsCode(PagePointOf(pos)), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toUrl() : QUrl());
    });
}

void EdgeWebView::CallWithSelectedText(StringCallBack callBack){
    CallWithScriptResult(SelectedTextJsCode(), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toString() : QString());
    });
}

void EdgeWebView::CallWithSelectedHtml(StringCallBack callBack){
    CallWithScriptResult(SelectedHtmlJsCode(), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toString() : QString());
    });
}

void EdgeWebView::CallWithWholeText(StringCallBack callBack){
    CallWithScriptResult(WholeTextJsCode(), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toString() : QString());
    });
}

void EdgeWebView::CallWithWholeHtml(StringCallBack callBack){
    CallWithScriptResult(WholeHtmlJsCode(), [callBack](const QVariant &var){
        callBack(var.isValid() ? var.toString() : QString());
    });
}

void EdgeWebView::CallWithSelectionRegion(RegionCallBack callBack){
    QPointer<EdgeWebView> alive(this);
    CallWithScriptResult(SelectionRegionJsCode(), [alive, callBack](const QVariant &var){
        if(!alive || !var.isValid() || !var.canConvert<QVariantMap>())
            return callBack(QRegion());

        const QRect viewport = QRect(QPoint(), alive->size());
        QRegion region;
        const QVariantMap map = var.toMap();
        foreach(const QString &key, map.keys()){
            const QVariantMap m = map[key].toMap();
            const QRect rect(m[QStringLiteral("x")].toInt(),
                             m[QStringLiteral("y")].toInt(),
                             m[QStringLiteral("width")].toInt(),
                             m[QStringLiteral("height")].toInt());
            region |= alive->WidgetRectOf(rect).intersected(viewport);
        }
        callBack(region);
    });
}

void EdgeWebView::SetFocusToElement(QString xpath){
    RunScript(SetFocusToElementJsCode(xpath));
}

void EdgeWebView::FireClickEvent(QString xpath, QPoint pos){
    RunScript(FireClickEventJsCode(xpath, PagePointOf(pos)));
}

void EdgeWebView::SetTextValue(QString xpath, QString text){
    RunScript(SetTextValueJsCode(xpath, text));
}

void EdgeWebView::RunScript(const QString &script){
    CallWithScriptResult(script, [](const QVariant&){});
}

bool EdgeWebView::SeekText(const QString &str, View::FindFlags opt){
    if(str.isEmpty()){
        RunScript(QStringLiteral("getSelection().removeAllRanges();"));
        return true;
    }
    RunScript(QStringLiteral("window.find(\"%1\", %2, %3, true);")
              .arg(EscapeJsStringLiteral(str),
                   (opt & CaseSensitively) ? QStringLiteral("true") : QStringLiteral("false"),
                   (opt & FindBackward)    ? QStringLiteral("true") : QStringLiteral("false")));
    return true;
}

void EdgeWebView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

void EdgeWebView::Copy(){
    RunScript(ExecCommandJsCode(QStringLiteral("copy")));
}

void EdgeWebView::Cut(){
    RunScript(ExecCommandJsCode(QStringLiteral("cut")));
}

void EdgeWebView::Paste(){
    const QString text = Application::clipboard()->text();
    if(text.isEmpty()) return;
    RunScript(QStringLiteral("document.execCommand(\"insertText\", false, \"%1\");")
              .arg(EscapeJsStringLiteral(text)));
}

void EdgeWebView::PasteAndMatchStyle(){
    Paste();
}

#define VANILLA_EXEC_COMMAND_ACTION(name, command)                      \
    void EdgeWebView::name(){                                           \
        RunScript(ExecCommandJsCode(QStringLiteral(command)));          \
    }
VANILLA_EXEC_COMMAND_ACTION(ToggleBold,          "bold")
VANILLA_EXEC_COMMAND_ACTION(ToggleItalic,        "italic")
VANILLA_EXEC_COMMAND_ACTION(ToggleUnderline,     "underline")
VANILLA_EXEC_COMMAND_ACTION(ToggleStrikethrough, "strikeThrough")
VANILLA_EXEC_COMMAND_ACTION(AlignLeft,           "justifyLeft")
VANILLA_EXEC_COMMAND_ACTION(AlignCenter,         "justifyCenter")
VANILLA_EXEC_COMMAND_ACTION(AlignRight,          "justifyRight")
VANILLA_EXEC_COMMAND_ACTION(AlignJustified,      "justifyFull")
VANILLA_EXEC_COMMAND_ACTION(Indent,              "indent")
VANILLA_EXEC_COMMAND_ACTION(Outdent,             "outdent")
VANILLA_EXEC_COMMAND_ACTION(InsertOrderedList,   "insertOrderedList")
VANILLA_EXEC_COMMAND_ACTION(InsertUnorderedList, "insertUnorderedList")
VANILLA_EXEC_COMMAND_ACTION(Undo,                "undo")
VANILLA_EXEC_COMMAND_ACTION(Redo,                "redo")
VANILLA_EXEC_COMMAND_ACTION(SelectAll,           "selectAll")
#undef VANILLA_EXEC_COMMAND_ACTION

void EdgeWebView::ChangeTextDirectionLTR(){
    RunScript(ChangeTextDirectionJsCode(QStringLiteral("ltr")));
}

void EdgeWebView::ChangeTextDirectionRTL(){
    RunScript(ChangeTextDirectionJsCode(QStringLiteral("rtl")));
}

void EdgeWebView::Unselect(){
    RunScript(QStringLiteral
              ("document.activeElement.blur(); getSelection().removeAllRanges();"));
}

void EdgeWebView::StopAndUnselect(){
    Stop();
    Unselect();
}

void EdgeWebView::Save(){
    if(!page()) return;
    QNetworkRequest req(url());
    req.setRawHeader("Referer", url().toEncoded());
    page()->Download(req);
}

void EdgeWebView::ToggleMediaControls(){
    RunScript(ToggleMediaControlsJsCode());
}

void EdgeWebView::ToggleMediaLoop(){
    RunScript(ToggleMediaLoopJsCode());
}

void EdgeWebView::ToggleMediaPlayPause(){
    RunScript(ToggleMediaPlayPauseJsCode());
}

void EdgeWebView::ToggleMediaMute(){
    SetAudioMuted(!m_Impl->m_Muted);
}

bool EdgeWebView::RecentlyAudible(){
    return m_Impl->m_Audible;
}

bool EdgeWebView::IsAudioMuted(){
    return m_Impl->m_Muted;
}

void EdgeWebView::SetAudioMuted(bool muted){
    if(!m_Impl->m_WebView) return;
    ComPtr<ICoreWebView2_8> webview8;
    if(FAILED(m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview8))) || !webview8)
        return;
    if(FAILED(webview8->put_IsMuted(muted ? TRUE : FALSE))) return;
    m_Impl->m_Muted = muted;
    emit ViewChanged();
}

void EdgeWebView::RestoreStateAfterLoad(){
    RestoreZoom();
    RestoreScroll();
#ifdef MEDIATIME
    RestoreMediaTime();
#endif
}

void EdgeWebView::HandleAudioStateChanged(bool playing){
    if(m_Impl->m_Audible == playing) return;
    m_Impl->m_Audible = playing;
    emit ViewChanged();

#ifdef MEDIATIME
    if(m_Impl->m_Audible){
        if(!m_Impl->m_MediaTimeSaveTimer) m_Impl->m_MediaTimeSaveTimer = startTimer(5000);
        return;
    }
    if(m_Impl->m_MediaTimeSaveTimer){
        killTimer(m_Impl->m_MediaTimeSaveTimer);
        m_Impl->m_MediaTimeSaveTimer = 0;
    }
    SaveMediaTime();
#endif
}

#ifdef MEDIATIME

bool EdgeWebView::SaveMediaTime(VoidCallBack settled){
    if(IsLoading()){
        if(settled) settled();
        return false;
    }
    const QUrl source = url();
    if(source.isEmpty() || source == BLANK_URL || !m_Impl->m_WebView ||
       !m_Impl->m_Document.HasLiveDocument()){
        if(settled) settled();
        return false;
    }

    std::shared_ptr<VoidCallBack> settle =
        std::make_shared<VoidCallBack>([settled](){ if(settled) settled();});
    const VoidCallBack settleOnce = [settle](){
        VoidCallBack call = *settle;
        *settle = VoidCallBack();
        if(call) call();
    };
    QPointer<EdgeWebView> alive(this);
    CallWithScriptResult(GetMediaTimeJsCode(), [alive, source, settleOnce](const QVariant &var){
        if(alive && var.isValid() && alive->GetViewNode() && alive->url() == source)
            alive->GetViewNode()->SetMediaTime(var.toFloat());
        settleOnce();
    }, settleOnce);
    return true;
}

bool EdgeWebView::RestoreMediaTime(){
    if(!GetViewNode()) return false;
    const float time = GetViewNode()->GetMediaTime();
    if(time <= 1.0f) return false;
    RunScript(SetMediaTimeJsCode(time));
    return true;
}
#endif

void EdgeWebView::timerEvent(QTimerEvent *ev){
    QWidget::timerEvent(ev);
#ifdef MEDIATIME
    if(ev->timerId() == m_Impl->m_MediaTimeSaveTimer) SaveMediaTime();
#endif
}

void EdgeWebView::ZoomIn(){
    if(!GetViewNode() || !m_Impl->m_Controller) return;
    const float zoom = PrepareForZoomIn();
    m_Impl->m_Controller->put_ZoomFactor(zoom);
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom * 100.0));
}

void EdgeWebView::ZoomOut(){
    if(!GetViewNode() || !m_Impl->m_Controller) return;
    const float zoom = PrepareForZoomOut();
    m_Impl->m_Controller->put_ZoomFactor(zoom);
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom * 100.0));
}

QPointF EdgeWebView::GetScroll(){
    return m_Impl->m_Scroll;
}

void EdgeWebView::SetScroll(QPointF pos){
    m_Impl->m_Scroll = pos;
    RunScript(SetScrollRatioPointJsCode(pos));
}

bool EdgeWebView::PageGeometry(QSizeF *contents, QRectF *viewport){
    if(m_Impl->m_PageContents.isEmpty() || m_Impl->m_PageViewport.isEmpty()) return false;
    *contents = m_Impl->m_PageContents;
    *viewport = QRectF(m_Impl->m_PageScroll, m_Impl->m_PageViewport);
    return true;
}

bool EdgeWebView::SaveScroll(){
    if(!GetViewNode() || size().isEmpty()) return false;

    QPointer<EdgeWebView> alive(this);
    CallWithScriptResult(GetScrollValuePointJsCode(), [alive](const QVariant &var){
        if(!alive || !alive->GetViewNode()) return;
        const QVariantList list = var.toList();
        if(list.length() < 2) return;
        alive->GetViewNode()->SetScrollX(list[0].toInt());
        alive->GetViewNode()->SetScrollY(list[1].toInt());
    });
    return true;
}

bool EdgeWebView::RestoreScroll(){
    if(!GetViewNode() || size().isEmpty()) return false;
    const QPoint pos = QPoint(GetViewNode()->GetScrollX(),
                              GetViewNode()->GetScrollY());
    if(pos.isNull()) return false;
    RunScript(SetScrollValuePointJsCode(pos));
    return true;
}

bool EdgeWebView::SaveZoom(){
    if(!GetViewNode() || !m_Impl->m_Controller) return false;
    double zoom = 1.0;
    if(FAILED(m_Impl->m_Controller->get_ZoomFactor(&zoom))) return false;
    GetViewNode()->SetZoom(static_cast<float>(zoom));
    return true;
}

bool EdgeWebView::RestoreZoom(){
    if(!GetViewNode() || !m_Impl->m_Controller) return false;
    const float zoom = GetViewNode()->GetZoom();
    if(zoom <= 0) return false;
    m_Impl->m_Controller->put_ZoomFactor(zoom);
    return true;
}

void EdgeWebView::OnBeforeStartingDisplayGadgets(){
    if(visible()) m_Impl->m_GrabbedDisplayData = GrabView();
    hide();
}

bool EdgeWebView::IsRenderable(){
    return visible() || !m_Impl->m_GrabbedDisplayData.isNull();
}

void EdgeWebView::Render(QPainter *painter){
    if(visible()) m_Impl->m_GrabbedDisplayData = GrabView();
    painter->drawImage(QPoint(), m_Impl->m_GrabbedDisplayData);
}

void EdgeWebView::Render(QPainter *painter, const QRegion &clip){
    if(visible()) m_Impl->m_GrabbedDisplayData = GrabView();
    foreach(const QRect &rect, clip)
        painter->drawImage(rect, m_Impl->m_GrabbedDisplayData.copy(rect));
}

QSize EdgeWebView::GetViewportSize(){
    return visible() ? QWidget::size()
                     : m_Impl->m_GrabbedDisplayData.deviceIndependentSize().toSize();
}

QImage EdgeWebView::CaptureVisible(){
    if(!visible() || !window() || window()->isMinimized()) return QImage();
    return GrabView();
}

QImage EdgeWebView::GrabView(){
    if(!m_Impl->m_Hwnd) return QImage();

    const RECT bounds = PhysicalBoundsOf(m_Impl->m_HostWindow);
    const int width  = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    if(width <= 0 || height <= 0) return QImage();

    const HDC screen = GetDC(nullptr);
    const HDC hdc = CreateCompatibleDC(screen);
    const HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    const HGDIOBJ previous = SelectObject(hdc, bitmap);

    QImage image;
    if(PrintWindow(m_Impl->m_Hwnd, hdc, 2)){
        image = QImage(width, height, QImage::Format_RGB32);
        BITMAPINFO info = {};
        info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth       = width;
        info.bmiHeader.biHeight      = -height;
        info.bmiHeader.biPlanes      = 1;
        info.bmiHeader.biBitCount    = 32;
        info.bmiHeader.biCompression = BI_RGB;
        if(!GetDIBits(hdc, bitmap, 0, height, image.bits(), &info, DIB_RGB_COLORS))
            image = QImage();
        else
            image.setDevicePixelRatio(devicePixelRatioF());
    }

    SelectObject(hdc, previous);
    DeleteObject(bitmap);
    DeleteDC(hdc);
    ReleaseDC(nullptr, screen);

    return image;
}

void EdgeWebView::Print(){
    const QString filename = ModalDialog::GetSaveFileName_
        (QString(), QString(),
         QStringLiteral("Pdf document (*.pdf);;Images (*.jpg *.jpeg *.gif *.png *.bmp *.xpm)"));

    if(filename.isEmpty()) return;

    if(!filename.toLower().endsWith(QStringLiteral(".pdf"))){
        const QImage image = GrabView();
        if(image.isNull() || !image.save(filename)){
            emit statusBarMessage(tr("Failed to save %1").arg(filename));
            return;
        }
        emit statusBarMessage(tr("Saved %1").arg(filename));
        return;
    }

    ComPtr<ICoreWebView2_7> webview7;
    if(!m_Impl->m_WebView ||
       FAILED(m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview7))) || !webview7){
        emit statusBarMessage(tr("This view cannot print to pdf."));
        return;
    }

    QPointer<EdgeWebView> alive(this);
    webview7->PrintToPdf
        (reinterpret_cast<PCWSTR>(filename.utf16()), nullptr,
         Callback<ICoreWebView2PrintToPdfCompletedHandler>
         ([alive, filename](HRESULT result, BOOL ok) -> HRESULT {
             if(!alive) return S_OK;
             if(SUCCEEDED(result) && ok)
                 emit alive->statusBarMessage(tr("Saved %1").arg(filename));
             else
                 emit alive->statusBarMessage(tr("Failed to save %1").arg(filename));
             return S_OK;
         }).Get());
}

void EdgeWebView::HandlePagePrintRequest(){
    Print();
}

#endif
