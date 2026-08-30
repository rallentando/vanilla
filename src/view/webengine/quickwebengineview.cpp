#include "switch.hpp"
#include "const.hpp"
#include "devicescale.hpp"

#ifdef WEBENGINEVIEW

#include "quickwebengineview.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>
#include <QWebEngineProfile>
#include <QWebEngineHistory>
#include <QQuickWebEngineProfile>
#include <QDataStream>

#include "view.hpp"
#include "webenginepage.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "directorypage.hpp"

#include <memory>

QuickWebEngineView::QuickWebEngineView(TreeBank *parent, QString id, QStringList set)
    :
    QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebengineview6.qml")), parent)
    , View(parent, id, set)
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlWebEngineView = rootObject();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);

    m_QmlWebEngineView->setProperty(
        "profile",
        QVariant::fromValue<QObject*>(
            nam->GetProfile()->isOffTheRecord()
                ? NetworkController::QuickPrivateProfile(id)
                : NetworkController::QuickProfile(id)));

    m_Page = new WebEnginePage(nam, this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_Inspector = nullptr;
    m_ScrollSignalTimer = 0;
    m_PreventScrollRestoration = false;
    m_SuspendedSpecificSettings = false;
#ifdef MEDIATIME
    m_MediaTimeSaveTimer = 0;
    connect(this, &QuickWebEngineView::recentlyAudibleChanged,
            this, [this](bool audible){
                if(audible){
                    if(!m_MediaTimeSaveTimer)
                        m_MediaTimeSaveTimer = startTimer(5000);
                } else {
                    if(m_MediaTimeSaveTimer){
                        killTimer(m_MediaTimeSaveTimer);
                        m_MediaTimeSaveTimer = 0;
                    }
                    SaveMediaTime();
                }
            });
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
    connect(this, &QuickWebEngineView::printRequested,
            this, &QuickWebEngineView::Print);
    connect(this, SIGNAL(renderProcessTerminated(int, int)),
            this, SLOT(HandleRenderProcessTermination(int, int)));
    connect(this, SIGNAL(fullScreenRequested(bool)),
            this, SLOT(HandleFullScreen(bool)));
    connect(this, SIGNAL(contextMenuRequested(QObject*, bool)),
            this, SLOT(HandleContextMenu(QObject*, bool)));
    connect(this, SIGNAL(downloadRequested(QObject*)),
            this, SLOT(HandleDownload(QObject*)));
    connect(this, SIGNAL(contentsSizeChanged(const QSizeF&)),
            this, SLOT(HandleContentsSizeChange(const QSizeF&)));
    connect(this, SIGNAL(scrollPositionChanged(const QPointF&)),
            this, SLOT(HandleScrollPositionChange(const QPointF&)));

    connect(m_QmlWebEngineView, SIGNAL(callBackResult(int, QVariant)),
            this,               SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlWebEngineView, SIGNAL(viewChanged()),
            this,               SIGNAL(ViewChanged()));
    connect(m_QmlWebEngineView, SIGNAL(scrollChanged(QPointF)),
            this,               SIGNAL(ScrollChanged(QPointF)));

}

QuickWebEngineView::~QuickWebEngineView(){
    if(m_Inspector){
        if(m_TreeBank)
            if(MainWindow *win = m_TreeBank->GetMainWindow())
                if(win->DockedInspectorPane() == m_Inspector)
                    win->SetInspectorPane(nullptr);
        m_QmlWebEngineView->setProperty("devToolsView",
                                        QVariant::fromValue<QObject*>(nullptr));
        m_Inspector->deleteLater();
    }

}

void QuickWebEngineView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    {
        const int state = DirectoryPage::StateIn
            (set, QStringLiteral("(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"));
        QQuickWebEngineProfile *current =
            qobject_cast<QQuickWebEngineProfile*>(
                m_QmlWebEngineView->property("profile").value<QObject*>());
        if(state != -1 && current){
            const QString id = NetworkController::ProfileKey(current)
                .section(QLatin1Char(':'), 1);
            QQuickWebEngineProfile *wanted = state == 1
                ? NetworkController::QuickPrivateProfile(id)
                : NetworkController::QuickProfile(id);
            if(wanted != current)
                m_QmlWebEngineView->setProperty(
                    "profile", QVariant::fromValue<QObject*>(wanted));
        }
    }

    if(!page()) return;

    SetPreference(QWebEngineSettings::AutoLoadImages,                    "AutoLoadImages");
    SetPreference(QWebEngineSettings::JavascriptCanAccessClipboard,      "JavascriptCanAccessClipboard");
    SetPreference(QWebEngineSettings::JavascriptCanOpenWindows,          "JavascriptCanOpenWindows");
    SetPreference(QWebEngineSettings::JavascriptEnabled,                 "JavascriptEnabled");
    SetPreference(QWebEngineSettings::LinksIncludedInFocusChain,         "LinksIncludedInFocusChain");
    SetPreference(QWebEngineSettings::LocalContentCanAccessFileUrls,     "LocalContentCanAccessFileUrls");
    SetPreference(QWebEngineSettings::LocalContentCanAccessRemoteUrls,   "LocalContentCanAccessRemoteUrls");
    SetPreference(QWebEngineSettings::LocalStorageEnabled,               "LocalStorageEnabled");
    SetPreference(QWebEngineSettings::PluginsEnabled,                    "PluginsEnabled");
    SetPreference(QWebEngineSettings::SpatialNavigationEnabled,          "SpatialNavigationEnabled");
    SetPreference(QWebEngineSettings::HyperlinkAuditingEnabled,          "HyperlinkAuditingEnabled");
    SetPreference(QWebEngineSettings::ScrollAnimatorEnabled,             "ScrollAnimatorEnabled");
    SetPreference(QWebEngineSettings::ScreenCaptureEnabled,              "ScreenCaptureEnabled");
    SetPreference(QWebEngineSettings::WebGLEnabled,                      "WebGLEnabled");
    SetPreference(QWebEngineSettings::Accelerated2dCanvasEnabled,        "Accelerated2dCanvasEnabled");
    SetPreference(QWebEngineSettings::AutoLoadIconsForPage,              "AutoLoadIconsForPage");
    SetPreference(QWebEngineSettings::TouchIconsEnabled,                 "TouchIconsEnabled");
    SetPreference(QWebEngineSettings::ErrorPageEnabled,                  "ErrorPageEnabled");
    SetPreference(QWebEngineSettings::FullScreenSupportEnabled,          "FullScreenSupportEnabled");
    SetPreference(QWebEngineSettings::FocusOnNavigationEnabled,          "FocusOnNavigationEnabled");
    SetPreference(QWebEngineSettings::PrintElementBackgrounds,           "PrintElementBackgrounds");
    SetPreference(QWebEngineSettings::AllowRunningInsecureContent,       "AllowRunningInsecureContent");
    SetPreference(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, "AllowGeolocationOnInsecureOrigins");
    SetPreference(QWebEngineSettings::AllowWindowActivationFromJavaScript, "AllowWindowActivationFromJavaScript");
    SetPreference(QWebEngineSettings::ShowScrollBars,                    "ShowScrollBars");
    SetPreference(QWebEngineSettings::PlaybackRequiresUserGesture,       "PlaybackRequiresUserGesture");
    SetPreference(QWebEngineSettings::WebRTCPublicInterfacesOnly,        "WebRTCPublicInterfacesOnly");
    SetPreference(QWebEngineSettings::JavascriptCanPaste,                "JavascriptCanPaste");
    SetPreference(QWebEngineSettings::DnsPrefetchEnabled,                "DnsPrefetchEnabled");
    SetPreference(QWebEngineSettings::PdfViewerEnabled,                  "PdfViewerEnabled");
    SetPreference(QWebEngineSettings::NavigateOnDropEnabled,             "NavigateOnDropEnabled");
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    SetPreference(QWebEngineSettings::ReadingFromCanvasEnabled,          "ReadingFromCanvasEnabled");
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    SetPreference(QWebEngineSettings::ForceDarkMode,                     "ForceDarkMode");
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    SetPreference(QWebEngineSettings::PrintHeaderAndFooter,              "PrintHeaderAndFooter");
    SetPreference(QWebEngineSettings::PreferCSSMarginsForPrinting,       "PreferCSSMarginsForPrinting");
    SetPreference(QWebEngineSettings::TouchEventsApiEnabled,             "TouchEventsApiEnabled");
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    SetPreference(QWebEngineSettings::BackForwardCacheEnabled,           "BackForwardCacheEnabled");
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    SetPreference(QWebEngineSettings::TrimAccessibilityIdentifiers,      "TrimAccessibilityIdentifiers");
#endif

    QMetaObject::invokeMethod(m_QmlWebEngineView, "setUserAgent",
                              Q_ARG(QVariant, QVariant::fromValue(page()->userAgentForUrl(QUrl()))));
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setAcceptLanguage",
                              Q_ARG(QVariant, QVariant::fromValue(page()->profile()->httpAcceptLanguage())));
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setDefaultTextEncoding",
                              Q_ARG(QVariant, QVariant::fromValue(page()->settings()->defaultTextEncoding())));

    QString policy;
    switch(page()->settings()->unknownUrlSchemePolicy()){
    case QWebEngineSettings::DisallowUnknownUrlSchemes:
        policy = "DisallowUnknownUrlSchemes"; break;
    case QWebEngineSettings::AllowUnknownUrlSchemesFromUserInteraction:
        policy = "AllowUnknownUrlSchemesFromUserInteraction"; break;
    case QWebEngineSettings::AllowAllUnknownUrlSchemes:
        policy = "AllowAllUnknownUrlSchemes"; break;
    }
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setUnknownUrlSchemePolicy",
                              Q_ARG(QVariant, QVariant::fromValue(policy)));
}

void QuickWebEngineView::suspendSpecificSettingsIfNeed(const QUrl &url){
    const bool own = url.scheme() == VANILLA_SCHEME;
    if(own == m_SuspendedSpecificSettings) return;

    m_SuspendedSpecificSettings = own;
    if(own){
        QMetaObject::invokeMethod
            (m_QmlWebEngineView, "setPreference",
             Q_ARG(QVariant, QVariant::fromValue(QStringLiteral("JavascriptEnabled"))),
             Q_ARG(QVariant, QVariant::fromValue(true)));
    } else {
        ApplySpecificSettings(SpecificSettings());
    }
}

QQuickWidget *QuickWebEngineView::base(){
    return static_cast<QQuickWidget*>(this);
}

WebEnginePage *QuickWebEngineView::page(){
    return static_cast<WebEnginePage*>(View::page());
}

QUrl QuickWebEngineView::url(){
    return m_QmlWebEngineView->property("url").toUrl();
}

QString QuickWebEngineView::html(){
    return WholeHtml();
}

TreeBank *QuickWebEngineView::parent(){
    return m_TreeBank;
}

void QuickWebEngineView::setUrl(const QUrl &url){
    m_QmlWebEngineView->setProperty("url", url);
    emit urlChanged(url);
}

void QuickWebEngineView::setHtml(const QString &html, const QUrl &url){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "loadHtml",
                              Q_ARG(QString, html),
                              Q_ARG(QUrl,    url));
    emit urlChanged(url);
}

void QuickWebEngineView::setParent(TreeBank* t){
    View::SetTreeBank(t);
    base()->setParent(t);
}

void QuickWebEngineView::Connect(TreeBank *tb){
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

void QuickWebEngineView::Disconnect(TreeBank *tb){
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

void QuickWebEngineView::OnSetViewNode(ViewNode*){}


void QuickWebEngineView::OnSetThis(WeakView){}

void QuickWebEngineView::OnSetMaster(WeakView){}

void QuickWebEngineView::OnSetSlave(WeakView){}

void QuickWebEngineView::OnSetJsObject(_View*){}

void QuickWebEngineView::OnSetJsObject(_Vanilla*){}

void QuickWebEngineView::OnLoadStarted(){
    if(!GetViewNode()) return;

    View::OnLoadStarted();
    Application::ReassertColorSchemeForWeb();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;

#ifdef USE_WEBCHANNEL
#endif

    if(m_Icon.isNull() && url() != BLANK_URL)
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
}

void QuickWebEngineView::OnLoadProgress(int progress){
    if(!GetViewNode()) return;
    View::OnLoadProgress(progress);
    if(progress != 100)
        emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void QuickWebEngineView::OnLoadFinished(bool ok){
    if(!GetViewNode()) return;

    View::OnLoadFinished(ok);

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

void QuickWebEngineView::OnTitleChanged(const QString &title){
    if(!GetViewNode()) return;
    ChangeNodeTitle(title);
}

void QuickWebEngineView::OnUrlChanged(const QUrl &url){
    if(!GetViewNode()) return;
    SaveHistory();
#ifdef MEDIATIME
    if(url != GetViewNode()->GetUrl())
        GetViewNode()->SetMediaTime(0);
#endif
    ChangeNodeUrl(url);
}

void QuickWebEngineView::OnViewChanged(){
    if(!GetViewNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickWebEngineView::OnScrollChanged(){
    if(!GetViewNode()) return;
    SaveScroll();
}

void QuickWebEngineView::EmitScrollChanged(){
    if(!m_ScrollSignalTimer)
        m_ScrollSignalTimer = startTimer(200);
}

void QuickWebEngineView::CallWithScroll(PointFCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetScrollRatioPointJsCode(), [callBack](QVariant var){
            if(!var.isValid()) return callBack(QPointF(0.5f, 0.5f));
            QVariantList list = var.toList();
            callBack(QPointF(list[0].toFloat(), list[1].toFloat()));
        });
}

void QuickWebEngineView::SetScrollBarState(){
    CallWithEvaluatedJavaScriptResult
        (GetScrollBarStateJsCode(), [this](QVariant var){
            Q_UNUSED(this)
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

QPointF QuickWebEngineView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    return QPointF(0.5f, 0.5f);
}

void QuickWebEngineView::SetScroll(QPointF pos){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setScroll",
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

bool QuickWebEngineView::PageGeometry(QSizeF *contents, QRectF *viewport){
    if(!m_QmlWebEngineView) return false;
    const QSizeF whole =
        m_QmlWebEngineView->property("contentsSize").toSizeF();
    if(whole.isEmpty()) return false;
    *contents = whole;
    *viewport = QRectF(
        m_QmlWebEngineView->property("scrollPosition").toPointF(),
        QSizeF(size()));
    return true;
}

bool QuickWebEngineView::SaveScroll(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "saveScroll");
    return true;
}

bool QuickWebEngineView::RestoreScroll(){
    if(size().isEmpty()) return false;
    if(m_PreventScrollRestoration) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "restoreScroll");
    return true;
}

bool QuickWebEngineView::SaveZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "saveZoom");
    return true;
}

bool QuickWebEngineView::RestoreZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "restoreZoom");
    return true;
}

QWebEngineHistory *QuickWebEngineView::history(){
    return m_QmlWebEngineView->property("history").value<QWebEngineHistory*>();
}

bool QuickWebEngineView::SaveHistory(){
    if(!GetViewNode() || !history()) return false;
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << (*history());
    if(!ba.isEmpty()){
        GetViewNode()->SetHistoryData(ba);
        return true;
    }
    return false;
}

bool QuickWebEngineView::RestoreHistory(){
    return false;
}

#ifdef MEDIATIME
bool QuickWebEngineView::SaveMediaTime(){
    if(IsLoading()) return false;
    const QUrl source = url();
    CallWithEvaluatedJavaScriptResult
        (GetMediaTimeJsCode(), [this, source](QVariant var){
            if(!var.isValid() || !GetViewNode()) return;
            if(url() != source) return;
            GetViewNode()->SetMediaTime(var.toFloat());
        });
    return true;
}

bool QuickWebEngineView::RestoreMediaTime(){
    if(!GetViewNode()) return false;
    float time = GetViewNode()->GetMediaTime();
    if(time <= 1.0f) return false;
    CallWithEvaluatedJavaScriptResult(SetMediaTimeJsCode(time), [](QVariant){});
    return true;
}
#endif

void QuickWebEngineView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickWebEngineView::SeekText(const QString &str, View::FindFlags opt){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "seekText",
                              Q_ARG(QVariant, QVariant::fromValue(str)),
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<int>(opt))));
    return true;
}

void QuickWebEngineView::SetFocusToElement(QString xpath){
    CallWithEvaluatedJavaScriptResult(SetFocusToElementJsCode(xpath), [](QVariant){});
}

void QuickWebEngineView::FireClickEvent(QString xpath, QPoint pos){
    CallWithEvaluatedJavaScriptResult(FireClickEventJsCode(xpath, pos/engineZoomFactor()), [](QVariant){});
}

void QuickWebEngineView::SetTextValue(QString xpath, QString text){
    CallWithEvaluatedJavaScriptResult(SetTextValueJsCode(xpath, text), [](QVariant){});
}

void QuickWebEngineView::UpdateIcon(const QUrl &iconUrl){
    m_Icon = QIcon();
    if(!page()) return;
    QString host = url().host();
    QNetworkRequest req(iconUrl);
    DownloadItem *item = NetworkController::Download
        (static_cast<NetworkAccessManager*>(page()->networkAccessManager()),
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

void QuickWebEngineView::HandleWindowClose(){
    TriggerAction(Page::_Close);
}

void QuickWebEngineView::HandleJavascriptConsoleMessage(int level, const QString &msg){
    if(level != 0) return;
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        if(!Application::IsOnlyModifier(&ke)) TriggerKeyEvent(&ke);
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
    }
    else if(Application::ExactMatch(QStringLiteral("preventScrollRestoration%1").arg(Application::EventKey()), msg)){
        m_PreventScrollRestoration = true;
    }
}

void QuickWebEngineView::HandleFeaturePermission(const QUrl &securityOrigin, int feature){
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
    case 6:
        featureString = QStringLiteral("Notifications");          break;
    case 7:
        featureString = QStringLiteral("ClipboardReadWrite");     break;
    case 8:
        featureString = QStringLiteral("LocalFontsAccess");       break;
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
        QMetaObject::invokeMethod(m_QmlWebEngineView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(true)));
    } else if(clicked == Dialog::No){
        QMetaObject::invokeMethod(m_QmlWebEngineView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    } else if(clicked == Dialog::Cancel){
    }
}

void QuickWebEngineView::HandleRenderProcessTermination(int status, int code){
    QVariant discarded;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "isDiscarded",
                              Q_RETURN_ARG(QVariant, discarded));
    if(discarded.toBool()) return;

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
    if(giveUp) return;
    QTimer::singleShot(0, m_QmlWebEngineView, SLOT(reload()));
}

void QuickWebEngineView::Suspend(){
    if(HiddenViewsStayActive()) return;

    QPointer<QuickWebEngineView> self = this;
    const QString state = SuspendHiddenViews();
    QTimer::singleShot(0, this, [self, state](){
        if(!self || !self->m_QmlWebEngineView) return;
        if(self->visible()) return;
        if(self->RecentlyAudible()) return;
        if(self->IsLoading()) return;

        QMetaObject::invokeMethod(self->m_QmlWebEngineView, "suspend",
                                  Q_ARG(QVariant, QVariant::fromValue(state)));
    });
}

void QuickWebEngineView::WakeUp(){
    if(!m_QmlWebEngineView) return;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "wakeUp");
}

void QuickWebEngineView::certificateError(QWebEngineCertificateError error){
    if(page()) page()->HandleCertificateError(error);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
void QuickWebEngineView::desktopMediaRequested(const QWebEngineDesktopMediaRequest &request){
    if(page()) page()->HandleDesktopMedia(request);
}

void QuickWebEngineView::webAuthUxRequested(QWebEngineWebAuthUxRequest *request){
    if(page()) page()->HandleWebAuthUx(request);
}
#endif

void QuickWebEngineView::HandleFullScreen(bool on){
    if(TreeBank *tb = GetTreeBank()){
        tb->GetMainWindow()->SetFullScreen(on);
        SetDisplayObscured(on);
        if(!on) return;
        ModelessDialog *dialog = new ModelessDialog();
        connect(this, &QuickWebEngineView::destroyed, dialog, &ModelessDialog::Returned);
        connect(this, &QuickWebEngineView::fullScreenRequested, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(tr("This page becomes full screen mode."));
        dialog->SetCaption(tr("Press Esc to exit."));
        dialog->SetButtons(Dialog::Ok | Dialog::Cancel);
        dialog->SetDefaultValue(true);
        dialog->SetCallBack([this](bool ok){ if(!ok) ExitFullScreen();});
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    }
}

void QuickWebEngineView::HandleContextMenu(QObject *object, bool isMedia){
    if(TakeRightButtonConsumed()) return;

    SharedWebElement elem = m_ClickedElement;
    Page::MediaType type = Page::MediaTypeNone;
    QPoint pos = QPoint(object->property("x").toInt(), object->property("y").toInt());
    if(!elem){
        QUrl linkUrl = object->property("linkUrl").toUrl();
        QUrl mediaUrl = object->property("mediaUrl").toUrl();
        bool isEditable = object->property("isContentEditable").toBool();
        std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
        *e = JsWebElement(this, pos, linkUrl, mediaUrl, isEditable);
        elem = e;
    }
    m_SelectedText = object->property("selectedText").toString();
    if(isMedia)
        type = Page::MediaTypePlayable;
    else if(elem && !elem->ImageUrl().isEmpty())
        type = Page::MediaTypeImage;
    page()->DisplayContextMenu(m_TreeBank, elem, pos, mapToGlobal(pos), type);
    GestureAborted();
}

void QuickWebEngineView::HandleDownload(QObject *object){
    static_cast<NetworkAccessManager*>(page()->networkAccessManager())->HandleDownload(object);
}

void QuickWebEngineView::HandleContentsSizeChange(const QSizeF &size){
    Q_UNUSED(size)
    RestoreScroll();
}

void QuickWebEngineView::HandleScrollPositionChange(const QPointF &pos){
    Q_UNUSED(pos)
    EmitScrollChanged();
}

void QuickWebEngineView::Copy(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "copy");
}

void QuickWebEngineView::Cut(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "cut");
}

void QuickWebEngineView::Paste(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "paste");
}

void QuickWebEngineView::Undo(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "undo");
}

void QuickWebEngineView::Redo(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "redo");
}

void QuickWebEngineView::SelectAll(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "selectAll");
}

void QuickWebEngineView::Unselect(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "unselect");
}

void QuickWebEngineView::Reload(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "reload");
}

void QuickWebEngineView::ReloadAndBypassCache(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "reloadAndBypassCache");
}

void QuickWebEngineView::Stop(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "stop");
}

void QuickWebEngineView::StopAndUnselect(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "stopAndUnselect");
}

void QuickWebEngineView::Print(){

    QString filename = ModalDialog::GetSaveFileName_
        (QString(), QString(),
         QStringLiteral("Pdf document (*.pdf);;Images (*.jpg *.jpeg *.gif *.png *.bmp *.xpm)"));

    if(filename.isEmpty()) return;

    if(filename.toLower().endsWith(QStringLiteral(".pdf"))){

        QMetaObject::invokeMethod(m_QmlWebEngineView, "print_",
                                  Q_ARG(QVariant, QVariant::fromValue(filename)));
    } else {
        QSize origSize = size();
        QPointF origPos = m_QmlWebEngineView->property("scrollPosition").toPointF();
        QSizeF contentsSize = m_QmlWebEngineView->property("contentsSize").toSizeF();
        resize(contentsSize.toSize());

        QTimer::singleShot(700, this, [this, filename, origSize, origPos](){

        grabFramebuffer().save(filename);

        resize(origSize);
        CallWithEvaluatedJavaScriptResult
            (SetScrollValuePointJsCode(origPos.toPoint()), [](QVariant){});

        });
    }
}

void QuickWebEngineView::Save(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "save");
}

void QuickWebEngineView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    m_QmlWebEngineView->setProperty("zoomFactor", static_cast<qreal>(zoom) * DeviceZoomScale());
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebEngineView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    m_QmlWebEngineView->setProperty("zoomFactor", static_cast<qreal>(zoom) * DeviceZoomScale());
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebEngineView::ToggleMediaControls(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "toggleMediaControls");
}

void QuickWebEngineView::ToggleMediaLoop(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "toggleMediaLoop");
}

void QuickWebEngineView::ToggleMediaPlayPause(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "toggleMediaPlayPause");
}

void QuickWebEngineView::ToggleMediaMute(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "toggleMediaMute");
}

void QuickWebEngineView::ExitFullScreen(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "fullScreenCancelled");
}

void QuickWebEngineView::InspectElement(){
    if(!m_Inspector){
        m_Inspector = new QQuickWidget
            (QUrl(QStringLiteral("qrc:/view/quickwebengineinspector6.qml")), nullptr);
        m_Inspector->setAttribute(Qt::WA_DeleteOnClose, false);
        m_Inspector->setResizeMode(QQuickWidget::SizeRootObjectToView);
        m_Inspector->resize(DeviceScale::PrimarySize(QSize(1024, 768)));

        QQuickItem *inspector = m_Inspector->rootObject();

        if(inspector){
            inspector->setProperty(
                "profile",
                QVariant::fromValue<QObject*>(NetworkController::QuickInspectorProfile()));
            inspector->setProperty("zoomFactor", DeviceZoomScale());

            connect(inspector, SIGNAL(windowCloseRequested()),
                    this,      SLOT(CloseInspector()));

            m_QmlWebEngineView->setProperty(
                "devToolsView", QVariant::fromValue<QObject*>(inspector));
        }
    } else if(QQuickItem *inspector = m_Inspector->rootObject()){
        QMetaObject::invokeMethod(inspector, "reload");
    }

    MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr;

    if(win && InspectorPane()){
        win->SetInspectorPane(m_Inspector, true);
        return;
    }
    if(win && win->DockedInspectorPane() == m_Inspector)
        win->SetInspectorPane(nullptr);
    m_Inspector->show();
    m_Inspector->raise();
}

void QuickWebEngineView::CloseInspector(){
    if(!m_Inspector) return;
    MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr;

    if(win && win->DockedInspectorPane() == m_Inspector)
        win->CloseInspectorPane();
    else
        m_Inspector->hide();
}

void QuickWebEngineView::AddSearchEngine(QPoint pos){
    if(page()) page()->AddSearchEngine(pos);
}

void QuickWebEngineView::AddBookmarklet(QPoint pos){
    if(page()) page()->AddBookmarklet(pos);
}

void QuickWebEngineView::timerEvent(QTimerEvent *ev){
    QQuickWidget::timerEvent(ev);
    if(ev->timerId() == m_ScrollSignalTimer){
        QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChanged");
        killTimer(m_ScrollSignalTimer);
        m_ScrollSignalTimer = 0;
    }
#ifdef MEDIATIME
    if(ev->timerId() == m_MediaTimeSaveTimer){
        SaveMediaTime();
    }
#endif
}

void QuickWebEngineView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QQuickWidget::hideEvent(ev);
}

void QuickWebEngineView::showEvent(QShowEvent *ev){
    m_PreventScrollRestoration = false;
    QQuickWidget::showEvent(ev);
    RestoreViewState();
}

void QuickWebEngineView::keyPressEvent(QKeyEvent *ev){
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

    QQuickWidget::keyPressEvent(ev);
}

void QuickWebEngineView::keyReleaseEvent(QKeyEvent *ev){
    if(!visible()) return;
    QQuickWidget::keyReleaseEvent(ev);
}

void QuickWebEngineView::resizeEvent(QResizeEvent *ev){
    QQuickWidget::resizeEvent(ev);
}

void QuickWebEngineView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QQuickWidget::mouseMoveEvent(ev);
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
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }
        QDrag *drag = new QDrag(this);

        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam =
            static_cast<NetworkAccessManager*>(page()->networkAccessManager());

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

            GestureAborted();
            QQuickWidget::mouseMoveEvent(ev);
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
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void QuickWebEngineView::mousePressEvent(QMouseEvent *ev){
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
    QQuickWidget::mousePressEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseReleaseEvent(QMouseEvent *ev){
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
#if defined(Q_OS_MAC)
            ev->setAccepted(false);
            return;
#else
            if(!TakeRightButtonConsumed()){
                SharedWebElement elem = m_ClickedElement;
                page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPosition().toPoint());
            }
            GestureAborted();
#endif
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QQuickWidget::mouseReleaseEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseDoubleClickEvent(QMouseEvent *ev){
    QQuickWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickWebEngineView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QQuickWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dragMoveEvent(QDragMoveEvent *ev){
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
    QQuickWidget::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dropEvent(QDropEvent *ev){
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
        QQuickWidget::dropEvent(ev);
    }
    ev->setAccepted(true);
}

void QuickWebEngineView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QQuickWidget::dragLeaveEvent(ev);
}

void QuickWebEngineView::wheelEvent(QWheelEvent *ev){
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
    QWheelEvent scaled(ev->position(), ev->globalPosition(),
                       ev->pixelDelta(), View::EngineWheelAngle(ev->angleDelta()),
                       ev->buttons(), ev->modifiers(), ev->phase(),
                       ev->inverted(), ev->source(), ev->pointingDevice());
    scaled.setTimestamp(ev->timestamp());
    QQuickWidget::wheelEvent(&scaled);
    ev->setAccepted(true);
}

void QuickWebEngineView::focusInEvent(QFocusEvent *ev){
    QQuickWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickWebEngineView::focusOutEvent(QFocusEvent *ev){
    QQuickWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickWebEngineView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QQuickWidget::focusNextPrevChild(next);
    return false;
}

void QuickWebEngineView::CallWithGotBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetCurrentBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithFoundElements(Page::FindElementsOption option,
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

void QuickWebEngineView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(pos.isNull()) return callBack(SharedWebElement());
    CallWithEvaluatedJavaScriptResult
        (HitElementJsCode(pos / engineZoomFactor()), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElement());
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void QuickWebEngineView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitLinkUrlJsCode(pos / engineZoomFactor()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitImageUrlJsCode(pos / engineZoomFactor()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithSelectedText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithSelectedHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithWholeText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithWholeHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithSelectionRegion(RegionCallBack callBack){
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

void QuickWebEngineView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                           VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    Q_UNUSED(this)
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

#endif
