#include "switch.hpp"
#include "const.hpp"

#ifdef NATIVEWEBVIEW

#include "quicknativewebview.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>
#include <QNetworkCookie>
#include <QClipboard>
#include <QRegularExpression>

#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#  include <QWebEngineSettings>
#endif

#ifdef Q_OS_WIN
#  include <windows.h>
#  pragma comment(lib, "User32.lib")
#  pragma comment(lib, "Gdi32.lib")
#endif

#include "view.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "dialog.hpp"

#include <memory>

QuickNativeWebView::QuickNativeWebView(TreeBank *parent, QString id, QStringList set)
    : QWidget(parent)
    , View(parent, id, set)
{
    Initialize();

    m_QuickView = new QQuickView();
    m_QuickView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_QuickView->rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);
    m_QuickView->setSource(QUrl(QStringLiteral("qrc:/view/quicknativewebview.qml")));

    m_Container = QWidget::createWindowContainer(m_QuickView, this);
    m_Container->setGeometry(rect());
    m_Container->setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_Container);

    m_QmlNativeWebView = m_QuickView->rootObject();
    ConnectQmlSignals();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new Page(this, nam);
    page()->SetView(this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_PreventScrollRestoration = false;
    m_EverShown = false;
    m_LoadedWhileHidden = false;
    m_ScrollSignalTimer = 0;
#ifdef MEDIATIME
    m_MediaTimePollTimer = startTimer(5000);
#endif

    m_ActionTable = QMap<Page::CustomAction, QAction*>();
    m_RequestId = 0;

    m_Icon = QIcon();
    connect(this, SIGNAL(iconUrlChanged(const QUrl&)),
            this, SLOT(UpdateIcon(const QUrl&)));

    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(HandleWindowClose()));
    connect(this, SIGNAL(javascriptConsoleMessage(int, const QString&)),
            this, SLOT(HandleJavascriptConsoleMessage(int, const QString&)));
    connect(this, SIGNAL(featurePermissionRequested(const QUrl&, int)),
            this, SLOT(HandleFeaturePermission(const QUrl&, int)));
    connect(this, SIGNAL(renderProcessTerminated(int, int)),
            this, SLOT(HandleRenderProcessTermination(int, int)));
    connect(this, SIGNAL(fullScreenRequested(bool)),
            this, SLOT(HandleFullScreen(bool)));
    connect(this, SIGNAL(downloadRequested(QObject*)),
            this, SLOT(HandleDownload(QObject*)));

}

void QuickNativeWebView::ConnectQmlSignals(){
    connect(m_QmlNativeWebView, SIGNAL(callBackResult(int, QVariant)),
            this,               SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlNativeWebView, SIGNAL(viewChanged()),
            this,               SIGNAL(ViewChanged()));
    connect(m_QmlNativeWebView, SIGNAL(scrollChanged(QPointF)),
            this,               SIGNAL(ScrollChanged(QPointF)));
}

void QuickNativeWebView::RebuildQmlItem(){
    const QUrl current = url();

    m_History.Release();

    disconnect(m_QmlNativeWebView, nullptr, this, nullptr);

    m_QuickView->setSource(QUrl());
    m_QuickView->setSource(QUrl(QStringLiteral("qrc:/view/quicknativewebview.qml")));
    m_QmlNativeWebView = m_QuickView->rootObject();
    ConnectQmlSignals();
    ApplyUserAgent();
    ApplyPageSettings();

    if(!current.isEmpty() && current != BLANK_URL){
        m_QmlNativeWebView->setProperty("url", current);
    }
}

QuickNativeWebView::~QuickNativeWebView(){
}

void QuickNativeWebView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);
    m_SpecificSet = set;
    ApplyUserAgent();
    ApplyPageSettings();
}

void QuickNativeWebView::ApplyUserAgent(){
    if(!page() || !page()->GetNetworkAccessManager()) return;
    const QString ua = page()->GetNetworkAccessManager()->GetUserAgent();
    if(!ua.isEmpty())
        m_QmlNativeWebView->setProperty("httpUserAgent", ua);
}

void QuickNativeWebView::ApplyPageSettings(){
    QObject *settings = m_QmlNativeWebView->property("settings").value<QObject*>();
    if(!settings) return;

    bool js = true, storage = true;
#ifdef WEBENGINEVIEW
    QWebEngineSettings *g = QWebEngineProfile::defaultProfile()->settings();
    js      = g->testAttribute(QWebEngineSettings::JavascriptEnabled);
    storage = g->testAttribute(QWebEngineSettings::LocalStorageEnabled);
#endif

    static const QString token = QStringLiteral("[jJ](?:ava)?[sS](?:cript)?");
    if(m_SpecificSet.indexOf(QRegularExpression(QStringLiteral("\\A!%1\\Z").arg(token))) != -1)
        js = false;
    else if(m_SpecificSet.indexOf(QRegularExpression(QStringLiteral("\\A%1\\Z").arg(token))) != -1)
        js = true;

    settings->setProperty("javaScriptEnabled", js);
    settings->setProperty("localStorageEnabled", storage);
    settings->setProperty("allowFileAccess", true);
}

QWidget *QuickNativeWebView::base(){
    return static_cast<QWidget*>(this);
}

Page *QuickNativeWebView::page(){
    return static_cast<Page*>(View::page());
}

QImage QuickNativeWebView::GrabView(){
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(m_QuickView->winId());
    RECT rect = {};
    if(!hwnd || !GetWindowRect(hwnd, &rect))
        return m_QuickView->grabWindow();
    const int width  = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if(width <= 0 || height <= 0) return QImage();

    const HDC screen = GetDC(nullptr);
    const HDC hdc = CreateCompatibleDC(screen);
    const HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    const HGDIOBJ previous = SelectObject(hdc, bitmap);

    QImage image;
    if(PrintWindow(hwnd, hdc, 2)){
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
            image.setDevicePixelRatio(m_QuickView->devicePixelRatio());
    }

    SelectObject(hdc, previous);
    DeleteObject(bitmap);
    DeleteDC(hdc);
    ReleaseDC(nullptr, screen);

    return image.isNull() ? m_QuickView->grabWindow() : image;
#else
    return m_QuickView->grabWindow();
#endif
}

QUrl QuickNativeWebView::url(){
    return m_QmlNativeWebView->property("url").toUrl();
}

QString QuickNativeWebView::html(){
    return WholeHtml();
}

TreeBank *QuickNativeWebView::parent(){
    return m_TreeBank;
}

void QuickNativeWebView::setUrl(const QUrl &url){
    AskForLoadOf(url);
    m_QmlNativeWebView->setProperty("url", url);
    emit urlChanged(url);
}

void QuickNativeWebView::setHtml(const QString &html, const QUrl &url){
    m_History.Release();
    QMetaObject::invokeMethod(m_QmlNativeWebView, "loadHtml",
                              Q_ARG(QString, html),
                              Q_ARG(QUrl,    url));
    emit urlChanged(url);
}

void QuickNativeWebView::setParent(TreeBank* t){
    View::SetTreeBank(t);
    base()->setParent(t);
}

void QuickNativeWebView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(this, SIGNAL(titleChanged(const QString&)),
            tb->parent(), SLOT(SetWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        connect(this, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(this, SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
                notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        connect(this, SIGNAL(ScrollChanged(QPointF)),
                notifier, SLOT(SetScroll(QPointF)));
        connect(notifier, SIGNAL(ScrollRequest(QPointF)),
                this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        connect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                this, SLOT(Load(const QString&)));
        connect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                this, SLOT(SeekText(const QString&, View::FindFlags)));
        connect(receiver, SIGNAL(KeyEvent(QString)),
                this, SLOT(KeyEvent(QString)));

        connect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                page(), SLOT(DownloadSuggest(const QUrl&)));
        connect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void QuickNativeWebView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(this, SIGNAL(titleChanged(const QString&)),
               tb->parent(), SLOT(SetWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                   notifier, SLOT(SetStatus(const QString&, const QString&)));
        disconnect(this, SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
                   notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        disconnect(this, SIGNAL(ScrollChanged(QPointF)),
                   notifier, SLOT(SetScroll(QPointF)));
        disconnect(notifier, SIGNAL(ScrollRequest(QPointF)),
                   this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        disconnect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                   this, SLOT(Load(const QString&)));
        disconnect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                   this, SLOT(SeekText(const QString&, View::FindFlags)));
        disconnect(receiver, SIGNAL(KeyEvent(QString)),
                   this, SLOT(KeyEvent(QString)));

        disconnect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                   page(), SLOT(DownloadSuggest(const QUrl&)));
        disconnect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                   receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void QuickNativeWebView::OnSetViewNode(ViewNode*){}


void QuickNativeWebView::OnSetThis(WeakView){}

void QuickNativeWebView::OnSetMaster(WeakView){}

void QuickNativeWebView::OnSetSlave(WeakView){}

void QuickNativeWebView::OnSetJsObject(_View*){}

void QuickNativeWebView::OnSetJsObject(_Vanilla*){}

void QuickNativeWebView::OnLoadStarted(){
    if(!GetViewNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;

    if(m_Icon.isNull() && url() != BLANK_URL)
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
}

void QuickNativeWebView::OnLoadProgress(int progress){
    if(!GetViewNode()) return;
    View::OnLoadProgress(progress);
    if(progress != 100)
        emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void QuickNativeWebView::OnLoadFinished(bool ok){
    m_History.Release();

    if(!GetViewNode()) return;

    View::OnLoadFinished(ok);

    if(!visible() && !m_EverShown) m_LoadedWhileHidden = true;

    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
#ifdef MEDIATIME
    RestoreMediaTime();
#endif
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

    if(visible() && m_TreeBank &&
       m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
        UpdateThumbnail();
    }
}

void QuickNativeWebView::OnTitleChanged(const QString &title){
    if(!GetViewNode()) return;
    ChangeNodeTitle(title);
}

void QuickNativeWebView::OnUrlChanged(const QUrl &url){
    m_History.Visit(url);
    SaveHistory();

    if(!GetViewNode()) return;
#ifdef MEDIATIME
    if(url != GetViewNode()->GetUrl())
        GetViewNode()->SetMediaTime(0);
#endif
    ChangeNodeUrl(url);
}

void QuickNativeWebView::OnViewChanged(){
    if(!GetViewNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickNativeWebView::OnScrollChanged(){
    if(!GetViewNode()) return;
    SaveScroll();
}

void QuickNativeWebView::EmitScrollChanged(){
    if(!m_ScrollSignalTimer)
        m_ScrollSignalTimer = startTimer(200);
}

void QuickNativeWebView::CallWithScroll(PointFCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetScrollRatioPointJsCode(), [callBack](QVariant var){
            if(!var.isValid()) return callBack(QPointF(0.5f, 0.5f));
            QVariantList list = var.toList();
            callBack(QPointF(list[0].toFloat(), list[1].toFloat()));
        });
}

void QuickNativeWebView::SetScrollBarState(){
    CallWithEvaluatedJavaScriptResult
        (GetScrollBarStateJsCode(), [this](QVariant var){
            if(!var.isValid()) return;
            QVariantList list = var.toList();
            int hmax = list[0].toInt();
            int vmax = list[1].toInt();
            if(hmax < 0) hmax = 0;
            if(vmax < 0) vmax = 0;
            if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
            else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
            else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
            else             m_ScrollBarState = NoScrollBarEnabled;
        });
}

QPointF QuickNativeWebView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    return QPointF(0.5f, 0.5f);
}

void QuickNativeWebView::SetScroll(QPointF pos){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "setScroll",
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

bool QuickNativeWebView::SaveScroll(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "saveScroll");
    return true;
}

bool QuickNativeWebView::RestoreScroll(){
    if(size().isEmpty()) return false;
    if(m_PreventScrollRestoration) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "restoreScroll");
    return true;
}

bool QuickNativeWebView::SaveZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "saveZoom");
    return true;
}

bool QuickNativeWebView::RestoreZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "restoreZoom");
    return true;
}

bool QuickNativeWebView::SaveHistory(){
    if(!GetViewNode()) return false;

    const QByteArray data = m_History.Serialize();
    if(data.isEmpty()) return false;

    const QByteArray already = GetViewNode()->GetHistoryData();
    if(!already.isEmpty() && !NativeHistory::LooksLikeOurs(already)) return false;

    GetViewNode()->SetHistoryData(data);
    return true;
}

bool QuickNativeWebView::RestoreHistory(){
    if(!GetViewNode()) return false;

    NativeHistory restored;
    if(!NativeHistory::Deserialize(GetViewNode()->GetHistoryData(), &restored)) return false;

    m_History = restored;
    m_QmlNativeWebView->setProperty("url", m_History.CurrentUrl());
    return true;
}

void QuickNativeWebView::PerformHistoryMove(const NativeHistory::Request &request){
    switch(request.kind){
    case NativeHistory::LoadMove:
        m_QmlNativeWebView->setProperty("url", request.url);
        break;
    case NativeHistory::ImmediateMove:
        SaveHistory();
        emit ViewChanged();
        break;
    case NativeHistory::NoMove:
        break;
    }
}

void QuickNativeWebView::TriggerNativeGoBackAction(){
    PerformHistoryMove(m_History.RequestBack(NativeIsLoading()));
}

void QuickNativeWebView::TriggerNativeGoForwardAction(){
    PerformHistoryMove(m_History.RequestForward(NativeIsLoading()));
}

void QuickNativeWebView::TriggerNativeRewindAction(){
    PerformHistoryMove(m_History.RequestRewind(NativeIsLoading()));
}

void QuickNativeWebView::TriggerNativeFastForwardAction(){
    PerformHistoryMove(m_History.RequestFastForward(NativeIsLoading()));
}

#ifdef MEDIATIME
bool QuickNativeWebView::SaveMediaTime(){
    if(IsLoading()) return false;
    const QUrl source = url();
    if(source.isEmpty() || source == BLANK_URL) return false;
    CallWithEvaluatedJavaScriptResult
        (GetMediaTimeJsCode(), [this, source](QVariant var){
            if(!var.isValid() || !GetViewNode()) return;
            if(url() != source) return;
            GetViewNode()->SetMediaTime(var.toFloat());
        });
    return true;
}

bool QuickNativeWebView::RestoreMediaTime(){
    if(!GetViewNode()) return false;
    float time = GetViewNode()->GetMediaTime();
    if(time <= 1.0f) return false;
    CallWithEvaluatedJavaScriptResult(SetMediaTimeJsCode(time), [](QVariant){});
    return true;
}
#endif

void QuickNativeWebView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickNativeWebView::SeekText(const QString &str, View::FindFlags opt){
    if(str.isEmpty()){
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("getSelection().removeAllRanges();"), [](QVariant){});
        return true;
    }
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("window.find(\"%1\", %2, %3, true);")
         .arg(EscapeJsStringLiteral(str),
              (opt & CaseSensitively) ? QStringLiteral("true") : QStringLiteral("false"),
              (opt & FindBackward)    ? QStringLiteral("true") : QStringLiteral("false")),
         [](QVariant){});
    return true;
}

void QuickNativeWebView::SetFocusToElement(QString xpath){
    CallWithEvaluatedJavaScriptResult(SetFocusToElementJsCode(xpath), [](QVariant){});
}

void QuickNativeWebView::FireClickEvent(QString xpath, QPoint pos){

    Q_UNUSED(xpath) Q_UNUSED(pos)
}

void QuickNativeWebView::SetTextValue(QString xpath, QString text){
    CallWithEvaluatedJavaScriptResult(SetTextValueJsCode(xpath, text), [](QVariant){});
}

void QuickNativeWebView::UpdateIcon(const QUrl &iconUrl){
    m_Icon = QIcon();
    if(!page()) return;
    QString host = url().host();
    QNetworkRequest req(iconUrl);
    DownloadItem *item = NetworkController::Download
        (page()->GetNetworkAccessManager(),
         req, NetworkController::ToVariable);

    if(!item) return;

    item->setParent(base());

    connect(item, &DownloadItem::DownloadResult, [this, host](const QByteArray &result){
        QPixmap pixmap;
        if(pixmap.loadFromData(result)){
            QIcon icon = QIcon(pixmap);
            Application::RegisterIcon(host, icon);
            if(url().host() == host) m_Icon = icon;
        }
    });
}

void QuickNativeWebView::HandleWindowClose(){
    QTimer::singleShot(0, page(), &Page::Close);
}

void QuickNativeWebView::HandleJavascriptConsoleMessage(int level, const QString &msg){
    if(level != 0) return;
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        KeyPressEvent(&ke);
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyRelease, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        KeyReleaseEvent(&ke);
    }
    else if(Application::ExactMatch(QStringLiteral("preventScrollRestoration%1").arg(Application::EventKey()), msg)){
        m_PreventScrollRestoration = true;
    }
}

void QuickNativeWebView::HandleFeaturePermission(const QUrl &securityOrigin, int feature){
    QString featureString;
    switch(feature){
    case 0:
        featureString = QStringLiteral("MediaAudioCapture");      break;
    case 1:
        featureString = QStringLiteral("MediaVideoCapture");      break;
    case 2:
        featureString = QStringLiteral("MediaAudioVideoCapture"); break;
    case 3:
        featureString = QStringLiteral("Geolocation");            break;
    case 4:
        featureString = QStringLiteral("DesktopVideoCapture");    break;
    case 5:
        featureString = QStringLiteral("DesktopAudioVideoCapture"); break;
    default: return;
    }

    ModalDialog *dialog = new ModalDialog();
    dialog->SetTitle(tr("Feature Permission Requested."));
    dialog->SetCaption(tr("Feature Permission Requested."));
    dialog->SetInformativeText
        (tr("Url: ") + securityOrigin.toString() + QStringLiteral("\n") +
         tr("Feature: ") + featureString + QStringLiteral("\n\n") +
         tr("Allow this feature?"));
    dialog->SetButtons(Dialog::Yes | Dialog::No | Dialog::Cancel);
    dialog->Execute();

    const Dialog::Button clicked = dialog->ClickedButton();
    if(clicked == Dialog::Yes){
        QMetaObject::invokeMethod(m_QmlNativeWebView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(true)));
    } else if(clicked == Dialog::No){
        QMetaObject::invokeMethod(m_QmlNativeWebView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    } else if(clicked == Dialog::Cancel){
    }
}

void QuickNativeWebView::HandleRenderProcessTermination(int status, int code){
    const RenderProcessLedger::Verdict verdict = RenderProcessDeaths().Count();
    if(verdict == RenderProcessLedger::Verdict::Silent) return;
    const bool giveUp = verdict == RenderProcessLedger::Verdict::GiveUp;

    QString info = giveUp
        ? tr("A page is left as it is, because that's process is terminated "
             "%1 times in a row.\n").arg(RenderProcessLedger::Limit)
        : tr("A page is reloaded, because that's process is terminated.\n");
    switch(status){
    case 0:
        info += tr("Normal termination. (code: %1)");   break;
    case 1:
        info += tr("Abnormal termination. (code: %1)"); break;
    case 2:
        info += tr("Crashed termination. (code: %1)");  break;
    case 3:
        info += tr("Killed termination. (code: %1)");   break;
    }
    ModelessDialog::Information(giveUp
                                ? tr("Render process terminated repeatedly.")
                                : tr("Render process terminated."),
                                info.arg(code), base());
    m_History.Release();
    if(giveUp) return;
    QTimer::singleShot(0, m_QmlNativeWebView, SLOT(reload()));
}

void QuickNativeWebView::HandleFullScreen(bool on){
    if(TreeBank *tb = GetTreeBank()){
        tb->GetMainWindow()->SetFullScreen(on);
        SetDisplayObscured(on);
        if(!on) return;
        ModelessDialog *dialog = new ModelessDialog();
        connect(this, &QuickNativeWebView::destroyed, dialog, &ModelessDialog::Returned);
        connect(this, &QuickNativeWebView::fullScreenRequested, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(tr("This page becomes full screen mode."));
        dialog->SetCaption(tr("Press Esc to exit."));
        dialog->SetButtons(Dialog::Ok | Dialog::Cancel);
        dialog->SetDefaultValue(true);
        dialog->SetCallBack([this](bool ok){ if(!ok) ExitFullScreen();});
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    }
}

void QuickNativeWebView::HandleDownload(QObject *object){
    page()->GetNetworkAccessManager()->HandleDownload(object);
}

void QuickNativeWebView::Copy(){
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.execCommand(\"copy\");"), [](QVariant){});
}

void QuickNativeWebView::Cut(){
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.execCommand(\"cut\");"), [](QVariant){});
}

void QuickNativeWebView::Paste(){
    const QString text = Application::clipboard()->text();
    if(text.isEmpty()) return;
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.execCommand(\"insertText\", false, \"%1\");")
         .arg(EscapeJsStringLiteral(text)), [](QVariant){});
}

void QuickNativeWebView::PasteAndMatchStyle(){
    Paste();
}

#define VANILLA_EXEC_COMMAND_ACTION(name, command)                        \
    void QuickNativeWebView::name(){                                      \
        CallWithEvaluatedJavaScriptResult                                 \
            (ExecCommandJsCode(QStringLiteral(command)), [](QVariant){}); \
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
#undef VANILLA_EXEC_COMMAND_ACTION

void QuickNativeWebView::ChangeTextDirectionLTR(){
    CallWithEvaluatedJavaScriptResult
        (ChangeTextDirectionJsCode(QStringLiteral("ltr")), [](QVariant){});
}

void QuickNativeWebView::ChangeTextDirectionRTL(){
    CallWithEvaluatedJavaScriptResult
        (ChangeTextDirectionJsCode(QStringLiteral("rtl")), [](QVariant){});
}

void QuickNativeWebView::Undo(){
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.execCommand(\"undo\");"), [](QVariant){});
}

void QuickNativeWebView::Redo(){
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.execCommand(\"redo\");"), [](QVariant){});
}

void QuickNativeWebView::SelectAll(){
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.execCommand(\"selectAll\");"), [](QVariant){});
}

void QuickNativeWebView::Unselect(){
    CallWithEvaluatedJavaScriptResult
        (QStringLiteral("document.activeElement.blur(); getSelection().removeAllRanges();"),
         [](QVariant){});
}

void QuickNativeWebView::Reload(){
    m_History.Release();
    QMetaObject::invokeMethod(m_QmlNativeWebView, "reload");
}

void QuickNativeWebView::ReloadAndBypassCache(){
    Reload();
}

void QuickNativeWebView::Stop(){
    m_History.Release();
    QMetaObject::invokeMethod(m_QmlNativeWebView, "stop");
}

void QuickNativeWebView::StopAndUnselect(){
    Stop();
    Unselect();
}

void QuickNativeWebView::Print(){

    QString filename = ModalDialog::GetSaveFileName_
        (QString(), QString(),
         QStringLiteral("Pdf document (*.pdf);;Images (*.jpg *.jpeg *.gif *.png *.bmp *.xpm)"));

    if(filename.isEmpty()) return;

    if(filename.toLower().endsWith(QStringLiteral(".pdf"))){

        QMetaObject::invokeMethod(m_QmlNativeWebView, "printToPdf",
                                  Q_ARG(QString, filename));
    } else {
        QSize origSize = size();
        QPointF origPos = m_QmlNativeWebView->property("scrollPosition").toPointF();
        QSizeF contentsSize = m_QmlNativeWebView->property("contentsSize").toSizeF();
        resize(contentsSize.toSize());

        QTimer::singleShot(700, this, [this, filename, origSize, origPos](){

        GrabView().save(filename);

        resize(origSize);
        CallWithEvaluatedJavaScriptResult
            (SetScrollValuePointJsCode(origPos.toPoint()), [](QVariant){});

        });
    }
}

void QuickNativeWebView::Save(){
    if(!page()) return;
    QNetworkRequest req(url());
    req.setRawHeader("Referer", url().toEncoded());
    page()->Download(req);
}

void QuickNativeWebView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    QMetaObject::invokeMethod(m_QmlNativeWebView, "setZoom",
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<qreal>(zoom))));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickNativeWebView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    QMetaObject::invokeMethod(m_QmlNativeWebView, "setZoom",
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<qreal>(zoom))));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickNativeWebView::ToggleMediaControls(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaControls");
}

void QuickNativeWebView::ToggleMediaLoop(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaLoop");
}

void QuickNativeWebView::ToggleMediaPlayPause(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaPlayPause");
}

void QuickNativeWebView::ToggleMediaMute(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaMute");
}

void QuickNativeWebView::ExitFullScreen(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "fullScreenCancelled");
}

void QuickNativeWebView::AddSearchEngine(QPoint pos){
    Q_UNUSED(pos)
}

void QuickNativeWebView::AddBookmarklet(QPoint pos){
    Q_UNUSED(pos)
}

void QuickNativeWebView::timerEvent(QTimerEvent *ev){
    QWidget::timerEvent(ev);
    if(ev->timerId() == m_ScrollSignalTimer){
        QMetaObject::invokeMethod(m_QmlNativeWebView, "emitScrollChanged");
        killTimer(m_ScrollSignalTimer);
        m_ScrollSignalTimer = 0;
    }
#ifdef MEDIATIME
    if(ev->timerId() == m_MediaTimePollTimer){
        SaveMediaTime();
    }
#endif
}

void QuickNativeWebView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QWidget::hideEvent(ev);
}

void QuickNativeWebView::showEvent(QShowEvent *ev){
    m_PreventScrollRestoration = false;

    if(!m_EverShown && m_LoadedWhileHidden){
        m_LoadedWhileHidden = false;
        RebuildQmlItem();
    }
    m_EverShown = true;

    QWidget::showEvent(ev);
    RestoreViewState();
}

void QuickNativeWebView::keyPressEvent(QKeyEvent *ev){
    if(!visible()) return;

    if(GetDisplayObscured()){
        if(ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_F11){
            ExitFullScreen();
            ev->setAccepted(true);
            return;
        }
    }

    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){
        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }

    if(!m_PreventScrollRestoration &&
       Application::IsMoveKey(ev)){
        m_PreventScrollRestoration = true;
        return;
    }

    if(
       !Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void QuickNativeWebView::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev)

    if(!visible()) return;

}

void QuickNativeWebView::resizeEvent(QResizeEvent *ev){
    m_Container->setGeometry(rect());
    QWidget::resizeEvent(ev);
}

void QuickNativeWebView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickNativeWebView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
        return;
    }
    if(m_EnableMouseGesture &&
       ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_RightGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_RightGestureMap[gesture])
              ? Action(Page::StringToAction(m_RightGestureMap[gesture]))->text()
            : m_RightGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
        ev->setAccepted(false);
        return;
    }

    int scrollBarWidth = Application::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    bool horizontal = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == HorizontalScrollBarEnabled;
    bool vertical   = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == VerticalScrollBarEnabled;
    QRect touchableRect =
        QRect(QPoint(),
              size() - QSize(vertical   ? scrollBarWidth : 0,
                             horizontal ? scrollBarWidth : 0));

    if(ev->buttons() & Qt::LeftButton &&
       !m_GestureStartedPos.isNull() &&
       touchableRect.contains(m_GestureStartedPos) &&
       (m_ClickedElement &&
        !m_ClickedElement->IsNull() &&
        !m_ClickedElement->IsEditableElement())){

        if(QLineF(ev->pos(), m_GestureStartedPos).length() < 2){
            QWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }
        QDrag *drag = new QDrag(this);

        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam =
            page()->GetNetworkAccessManager();

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

            GestureAborted();
            QWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = ev->pos() - rect.topLeft();

        if(pixmap.size().width()  > MAX_DRAGGING_PIXMAP_WIDTH ||
           pixmap.size().height() > MAX_DRAGGING_PIXMAP_HEIGHT){

            pos /= qMax(static_cast<float>(pixmap.size().width()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_WIDTH),
                        static_cast<float>(pixmap.size().height()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_HEIGHT));
            pixmap = pixmap.scaled(MAX_DRAGGING_PIXMAP_WIDTH,
                                   MAX_DRAGGING_PIXMAP_HEIGHT,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }

        if(m_HadSelection){
            mime->setImageData(pixmap.toImage());
        } else {
            QPixmap element = m_ClickedElement->Pixmap();
            if(element.isNull())
                mime->setImageData(pixmap.toImage());
            else
                mime->setImageData(element.toImage());
        }
        if(m_EnableDragGestureLocal){
            GestureMoved(ev->pos());
        } else {
            GestureAborted();
        }
        m_DragStarted = true;
        drag->setMimeData(mime);
        drag->setPixmap(pixmap);
        drag->setHotSpot(pos);
        drag->exec(Qt::CopyAction | Qt::MoveAction);
        drag->deleteLater();
        ev->setAccepted(true);
    } else {
        GestureAborted();
        QWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void QuickNativeWebView::mousePressEvent(QMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, ev->pos())){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(ev->pos());
    QWidget::mousePressEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    if(m_DragStarted){
        m_DragStarted = false;
        ev->setAccepted(true);
        return;
    }

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MiddleButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(Application::HasShiftModifier(ev) ||
           Application::HasCtrlModifier(ev) ||
           ev->button() == Qt::MiddleButton){

            GestureAborted();
            m_TreeBank->OpenInNewViewNode(req, Page::Activate(), GetViewNode());
            ev->setAccepted(true);
            return;

        }
    }

    if(ev->button() == Qt::RightButton){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            if(!TakeRightButtonConsumed()){
                SharedWebElement elem = m_ClickedElement;
                page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPosition().toPoint());
            }
            GestureAborted();
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QWidget::mouseReleaseEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::mouseDoubleClickEvent(QMouseEvent *ev){
    QWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickNativeWebView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::dragMoveEvent(QDragMoveEvent *ev){
    if(m_EnableDragGestureLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(ev->position().toPoint());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_DragGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_DragGestureMap[gesture])
              ? Action(Page::StringToAction(m_DragGestureMap[gesture]))->text()
            : m_DragGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QWidget::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    bool isLocal = false;
    QPoint pos = ev->position().toPoint();
    QObject *source = ev->source();
    QString text = ev->mimeData()->text();
    QList<QUrl> urls = Page::MimeDataToUrls(ev->mimeData(), source);

    foreach(QUrl u, urls){ if(u.isLocalFile()) isLocal = true;}

    if(text.isEmpty() && !urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    const bool own = IsOwnDragSource(source);

    CallWithHitElement(pos, [this, pos, own, text, urls](SharedWebElement elem){

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        elem->SetText(text);
        return;
    }

    if(!m_Gesture.isEmpty() && own){
        GestureFinished(pos, Qt::LeftButton);
        return;
    }

    GestureAborted();

    if(!urls.isEmpty() && !own){
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }

    });

    const bool consumed = (!m_Gesture.isEmpty() && own) || (!urls.isEmpty() && !own);

    if(isLocal || consumed ||
       (DragToStartDownload() && !urls.isEmpty() && own))
        ;
    else {
        if(own && !urls.isEmpty()) MarkOwnDrop();
        QWidget::dropEvent(ev);
    }
    ev->setAccepted(true);
}

void QuickNativeWebView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QWidget::dragLeaveEvent(ev);
}

void QuickNativeWebView::wheelEvent(QWheelEvent *ev){
    if(!visible()) return;

    if(ev->source() != Qt::MouseEventSynthesizedBySystem){
        QString wheel;
        bool up = Application::WheelWentUp(ev);

        Application::AddModifiersToString(wheel, ev->modifiers());
        Application::AddMouseButtonsToString(wheel, ev->buttons());
        Application::AddWheelDirectionToString(wheel, up);

        if(m_MouseMap.contains(wheel)){

            QString str = m_MouseMap[wheel];
            if(!str.isEmpty()){
                if(ev->buttons() & Qt::RightButton)
                    View::ConsumeRightButton();
                View::TriggerAction(str, ev->position());
            }
            ev->setAccepted(true);
            return;
        }
    }
    m_PreventScrollRestoration = true;
    QWidget::wheelEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::focusInEvent(QFocusEvent *ev){
    QWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickNativeWebView::focusOutEvent(QFocusEvent *ev){
    QWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickNativeWebView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QWidget::focusNextPrevChild(next);
    return false;
}

void QuickNativeWebView::CallWithGotBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetCurrentBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithFoundElements(Page::FindElementsOption option,
                                               WebElementListCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (FindElementsJsCode(option), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElementList());
            QVariantList list = var.toMap().values();
            SharedWebElementList result;

            MainWindow *win = Application::GetCurrentWindow();
            QSize s =
                m_TreeBank ? m_TreeBank->size() :
                win ? win->GetTreeBank()->size() :
                !size().isEmpty() ? size() :
                DEFAULT_WINDOW_SIZE;
            QRect viewport = QRect(QPoint(), s);

            for(int i = 0; i < list.length(); i++){
                std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
                *e = JsWebElement(this, list[i]);
                if(!viewport.intersects(e->Rectangle()))
                    e->SetRectangle(QRect());
                result << e;
            }
            callBack(result);
        });
}

void QuickNativeWebView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(pos.isNull()) return callBack(SharedWebElement());
    CallWithEvaluatedJavaScriptResult
        (HitElementJsCode(pos / m_ViewNode->GetZoom()), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElement());
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void QuickNativeWebView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitLinkUrlJsCode(pos / m_ViewNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitImageUrlJsCode(pos / m_ViewNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithSelectedText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithSelectedHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithWholeText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithWholeHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithSelectionRegion(RegionCallBack callBack){
    QRect viewport = QRect(QPoint(), size());
    CallWithEvaluatedJavaScriptResult
        (SelectionRegionJsCode(), [viewport, callBack](QVariant var){
            if(!var.isValid() || !var.canConvert<QVariantMap>())
                return callBack(QRegion());
            QRegion region;
            QVariantMap map = var.toMap();
            foreach(QString key, map.keys()){
                QVariantMap m = map[key].toMap();
                region |= QRect(m["x"].toInt(),
                                m["y"].toInt(),
                                m["width"].toInt(),
                                m["height"].toInt()).intersected(viewport);
            }
            callBack(region);
        });
}

void QuickNativeWebView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                           VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickNativeWebView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlNativeWebView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

#endif
