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
#include "webenginesitedatalink.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "directorypage.hpp"
#include "settingspage.hpp"
#include "extensioncontroller.hpp"
#include "webengineextensions.hpp"
#include "webenginepasswords.hpp"
#include "sitepermissions.hpp"

#include <QCursor>
#include <QWebEngineScript>
#ifdef VANILLA_QUICK_PASSWORDS
#  include <QQmlWebChannel>
#endif

#include <memory>

#ifdef VANILLA_QUICK_PASSWORD_FRAMES
#  include <QtWebEngineQuick/private/qquickwebengineframe_p.h>
#endif

QuickWebEngineView::QuickWebEngineView(TreeBank *parent, QString id, QStringList set)
    :
    QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebengineview6.qml")), parent)
    , View(parent, id, set)
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlWebEngineView = rootObject();

#ifdef VANILLA_QUICK_PASSWORDS
    InstallPasswords();
#endif
    QMetaObject::invokeMethod(m_QmlWebEngineView, "installScripts");
    ApplyTheme();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);

    const bool offTheRecord = DirectoryPage::SaysPrivate(set);
    m_QmlWebEngineView->setProperty(
        "profile",
        QVariant::fromValue<QObject*>(
            offTheRecord
                ? NetworkController::QuickPrivateProfile(id)
                : NetworkController::QuickProfile(id)));

    m_Page = new WebEnginePage(nam, offTheRecord, this);
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
    connect(this, &QuickWebEngineView::iconUrlChanged,
            this, &QuickWebEngineView::UpdateIcon);

    connect(this, &QuickWebEngineView::windowCloseRequested,
            this, &QuickWebEngineView::HandleWindowClose);
    connect(this, &QuickWebEngineView::javascriptConsoleMessage,
            this, &QuickWebEngineView::HandleJavascriptConsoleMessage);
    connect(this, &QuickWebEngineView::featurePermissionRequested,
            this, &QuickWebEngineView::HandleFeaturePermission);
    connect(this, &QuickWebEngineView::printRequested,
            this, &QuickWebEngineView::Print);
    connect(this, &QuickWebEngineView::renderProcessTerminated,
            this, &QuickWebEngineView::HandleRenderProcessTermination);
    connect(this, &QuickWebEngineView::fullScreenRequested,
            this, &QuickWebEngineView::HandleFullScreen);
    connect(this, &QuickWebEngineView::contextMenuRequested,
            this, &QuickWebEngineView::HandleContextMenu);
    connect(this, &QuickWebEngineView::javaScriptDialogRequested,
            this, &QuickWebEngineView::HandleJavaScriptDialog);
    connect(this, &QuickWebEngineView::downloadRequested,
            this, &QuickWebEngineView::HandleDownload);
    connect(this, &QuickWebEngineView::contentsSizeChanged,
            this, &QuickWebEngineView::HandleContentsSizeChange);
    connect(this, &QuickWebEngineView::scrollPositionChanged,
            this, &QuickWebEngineView::HandleScrollPositionChange);

    connect(m_QmlWebEngineView, SIGNAL(callBackResult(int, QVariant)),
            this,               SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlWebEngineView, SIGNAL(viewChanged()),
            this,               SIGNAL(ViewChanged()));

    connect(this, &QuickWebEngineView::recentlyAudibleChanged,
            this, [](bool){ TreeBank::TabsLookChanged();});
    connect(m_QmlWebEngineView, SIGNAL(findCount(int, int)),
            this, SIGNAL(FindCount(int, int)));
    connect(m_QmlWebEngineView, SIGNAL(scrollChanged(QPointF)),
            this,               SIGNAL(ScrollChanged(QPointF)));

}

#ifdef VANILLA_QUICK_PASSWORDS

void QuickWebEngineView::InstallPasswords(){
    m_ApplicationWorldSerial = 0;

    QPointer<QuickWebEngineView> view(this);
    WebEnginePasswords::Host host;
    host.current = [view](){
        WebEnginePasswords::Snapshot now;
        if(!view || !view->m_QmlWebEngineView) return now;
        QObject *profile = view->m_QmlWebEngineView->property("profile").value<QObject*>();
        now.alive = true;
        now.url = view->m_QmlWebEngineView->property("url").toUrl();
        now.space = NetworkController::CredentialSpace(profile);
        now.profile = profile;
        now.isPrivate = profile && profile->property("offTheRecord").toBool();
        now.zoom = view->m_QmlWebEngineView->property("zoomFactor").toReal();
        return now;
    };
    host.runInApplicationWorld = [view](const QString &code,
                                        std::function<void(const QVariant&)> done){
        if(!view) return;
        const int id = ++view->m_ApplicationWorldSerial;
        view->m_ApplicationWorldWaiting.insert(id, done);
        QMetaObject::invokeMethod(view->m_QmlWebEngineView, "runInApplicationWorld",
                                  Q_ARG(QVariant, QVariant(id)),
                                  Q_ARG(QVariant, QVariant(code)));
    };
    host.mapToGlobal = [view](const QPoint &pos){
        return view ? view->mapToGlobal(pos) : QCursor::pos();
    };
    host.message = [view](const QString &text){
        if(view) emit view->statusBarMessage(text);
    };
    host.widget = [view]() -> QWidget* { return view.data(); };
    host.managePasswords = [view](){
        if(!view) return;
        if(TreeBank *tb = view->GetTreeBank()) tb->OpenPasswords();
    };
#ifdef VANILLA_QUICK_PASSWORD_FRAMES
    host.mainFrame = [view](){
        if(!view || !view->m_QmlWebEngineView) return QWebEngineFrame();
        const QVariant frame = view->m_QmlWebEngineView->property("mainFrame");
        if(qstrcmp(frame.metaType().name(), "QQuickWebEngineFrame") != 0) return QWebEngineFrame();
        return QWebEngineFrame(*static_cast<const QQuickWebEngineFrame*>(frame.constData()));
    };
#endif

    m_Passwords = new WebEnginePasswords(this, host);
    QQmlWebChannel *channel = new QQmlWebChannel(this);
    channel->registerObject(QStringLiteral("vanillaPasswords"), m_Passwords->Bridge());
    m_QmlWebEngineView->setProperty("webChannelWorld",
                                    QVariant(uint(QWebEngineScript::ApplicationWorld)));
    m_QmlWebEngineView->setProperty("webChannel", QVariant::fromValue(channel));

    connect(this, &QuickWebEngineView::loadStarted,
            m_Passwords.data(), &WebEnginePasswords::Forget);
    connect(this, &QuickWebEngineView::urlChanged,
            m_Passwords.data(), &WebEnginePasswords::Navigated);
}
#endif

QString QuickWebEngineView::passwordScript(){
#ifdef VANILLA_QUICK_PASSWORDS
    if(m_Passwords) return WebEnginePasswords::ScriptSource();
#endif
    return QString();
}

void QuickWebEngineView::applicationWorldResult(int id, const QVariant &result){
#ifdef VANILLA_QUICK_PASSWORDS
    std::function<void(const QVariant&)> done = m_ApplicationWorldWaiting.take(id);
    if(done) done(result);
#else
    Q_UNUSED(id)
    Q_UNUSED(result)
#endif
}

QuickWebEngineView::~QuickWebEngineView(){
    if(m_QmlWebEngineView)
        m_QmlWebEngineView->setProperty("devToolsView", QVariant());
    if(m_Inspector){
        if(m_TreeBank)
            if(MainWindow *win = m_TreeBank->GetMainWindow())
                if(win->DockedInspectorPane() == m_Inspector)
                    win->SetInspectorPane(nullptr);
        m_Inspector->deleteLater();
    }

}

bool QuickWebEngineView::IsOffTheRecord(){
    if(m_QmlWebEngineView)
        if(QObject *profile = m_QmlWebEngineView->property("profile").value<QObject*>())
            return profile->property("offTheRecord").toBool();
    return View::IsOffTheRecord();
}

QObject *QuickWebEngineView::EngineProfile(){
    if(!m_QmlWebEngineView) return nullptr;
    return m_QmlWebEngineView->property("profile").value<QObject*>();
}

void QuickWebEngineView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    {
        QQuickWebEngineProfile *current =
            qobject_cast<QQuickWebEngineProfile*>(
                m_QmlWebEngineView->property("profile").value<QObject*>());
        if(current){
            const QString id = NetworkController::ProfileKey(current)
                .section(QLatin1Char(':'), 1);
            QQuickWebEngineProfile *wanted = DirectoryPage::SaysPrivate(set)
                ? NetworkController::QuickPrivateProfile(id)
                : NetworkController::QuickProfile(id);
            if(wanted != current){
#ifdef VANILLA_QUICK_PASSWORDS
                m_ApplicationWorldWaiting.clear();
                if(m_Passwords) m_Passwords->Forget();
#endif
                emit ExtensionContextChanged();
                ExtensionNavigation::Of(this)->Cancel();
                ExtensionController *controller = ExtensionController::Of(wanted);
                const bool starting = controller && !controller->IsReady();
                m_QmlWebEngineView->setProperty(
                    "profile", QVariant::fromValue<QObject*>(wanted));
                emit ExtensionContextChanged();
                if(starting)
                    ExtensionNavigation::Of(this)->Request(controller, [this](){ Reload(); });
            }
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
    ExtensionNavigation::Of(this)->Request(Extensions(), [this, url] {
        m_QmlWebEngineView->setProperty("url", url);
    });
    emit urlChanged(url);
}

void QuickWebEngineView::setHtml(const QString &html, const QUrl &url){
    ExtensionNavigation::Of(this)->Request(Extensions(), [this, html, url] {
        QMetaObject::invokeMethod(m_QmlWebEngineView, "loadHtml", Q_ARG(QString, html), Q_ARG(QUrl, url));
    });
    emit urlChanged(url);
}

ExtensionController *QuickWebEngineView::Extensions() const {
    return ExtensionController::Of(m_QmlWebEngineView->property("profile").value<QObject*>());
}

bool QuickWebEngineView::DeferExtensionNavigation(const QUrl &url) {
    auto *manager = Extensions();
    if (!manager || manager->IsReady()) return false;
    ExtensionNavigation::Of(this)->Request(manager, [this, url] { setUrl(url); });
    return true;
}

QString QuickWebEngineView::ExtensionStatus() const {
    QObject *profile = m_QmlWebEngineView->property("profile").value<QObject*>();
    const QString error = profile ? profile->property("extensionProfileError").toString() : QString();
    return error.isEmpty() ? View::ExtensionStatus() : error;
}

QWidget *QuickWebEngineView::CreateExtensionView(const QUrl &url, ExtensionPage kind, QWidget *parent, const std::function<void()> &closed) {
    Q_UNUSED(closed)
    if(kind == ExtensionSidePanelPage) return nullptr;
    auto *profile = qobject_cast<QQuickWebEngineProfile*>(m_QmlWebEngineView->property("profile").value<QObject*>());
    return WebEngineExtensions::CreatePopup(profile, url, kind == ExtensionActionPage, GetThis(), parent);
}

void QuickWebEngineView::setParent(TreeBank* t){
    View::SetTreeBank(t);
    base()->setParent(t);
}

void QuickWebEngineView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;
    WireWebView(this, this, tb, true);
}

void QuickWebEngineView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;
    WireWebView(this, this, tb, false);
}

void QuickWebEngineView::OnSetViewNode(ViewNode*){}

void QuickWebEngineView::OnSetThis(WeakView){}

void QuickWebEngineView::OnSetMaster(WeakView){}

void QuickWebEngineView::OnSetSlave(WeakView){}

void QuickWebEngineView::OnLoadStarted(){
    if(!GetViewNode()) return;

    View::OnLoadStarted();
    Application::ReassertColorSchemeForWeb();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;

    if(m_Icon.isNull() && url() != BLANK_URL)
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
}

void QuickWebEngineView::OnLoadProgress(int progress){
    if(!GetViewNode()) return;
    View::OnLoadProgress(progress);
    if(progress != 100)
        emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

bool QuickWebEngineView::EndLoad(bool ok){
    if(!GetViewNode()) return false;

    View::OnLoadFinished(ok);
    return true;
}

void QuickWebEngineView::loadStopped(){
    if(!EndLoad(false)) return;

    emit statusBarMessage(QString());
}

void QuickWebEngineView::OnLoadFinished(bool ok){
    ApplyTheme();
    if(!EndLoad(ok)) return;

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

    if(visible() && m_TreeBank){
        UpdateThumbnail();
    }
}

void QuickWebEngineView::ApplyTheme(){
    if(!m_QmlWebEngineView) return;
    const bool clear = Application::EnableTransparentWindow() &&
        VanillaPage::IsTranslucentPage(url());
    const QColor color = clear ? QColor(Qt::transparent) : QColor(Qt::white);
    if(color == m_BaseBackgroundColor) return;
    m_BaseBackgroundColor = color;
    m_QmlWebEngineView->setProperty("backgroundColor", color);
    setClearColor(color);
    setAttribute(Qt::WA_TranslucentBackground, clear);
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
        (GetScrollBarStateJsCode(), [](QVariant var){ SetScrollBarStateFrom(var);});
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
    if(VanillaPage::IsSettingsUrl(url())) return false;
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

QUrl QuickWebEngineView::CommittedUrl(){
    return history() ? history()->currentItem().url() : QUrl();
}

bool QuickWebEngineView::SaveHistory(){
    if(!GetViewNode() || !history()) return false;
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << (*history());
    if(!ba.isEmpty()){
        GetViewNode()->SetHistoryData(ba, KeepsOffDisk());
        return true;
    }
    return false;
}

bool QuickWebEngineView::RestoreHistory(){
    return false;
}

#ifdef MEDIATIME
bool QuickWebEngineView::SaveMediaTime(VoidCallBack settled){
    if(IsLoading()){
        if(settled) settled();
        return false;
    }
    const QUrl source = url();
    QPointer<QuickWebEngineView> alive(this);
    CallWithEvaluatedJavaScriptResult
        (GetMediaTimeJsCode(), [alive, source, settled](QVariant var){
            if(alive && var.isValid() && alive->GetViewNode() && alive->url() == source)
                alive->GetViewNode()->SetMediaTime(var.toFloat());
            if(settled) settled();
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
    TreeBank::TabsLookChanged();
    if(!page()) return;
    QString host = url().host();
    const bool offTheRecord = Page::OffTheRecord(this);
    QNetworkRequest req(iconUrl);
    NetworkAccessManager::SetRequestOffTheRecord(req, offTheRecord);
    DownloadItem *item = NetworkController::Download
        (static_cast<NetworkAccessManager*>(page()->networkAccessManager()),
         req, NetworkController::ToVariable);

    if(!item) return;

    item->setParent(base());

    connect(item, &DownloadItem::DownloadResult, [this, host, offTheRecord](const QByteArray &result){
        QPixmap pixmap;
        if(pixmap.loadFromData(result)){
            QIcon icon = QIcon(pixmap);
            Application::RegisterIcon(host, icon, offTheRecord || Page::OffTheRecord(this));
            if(url().host() == host){
                m_Icon = icon;
                TreeBank::TabsLookChanged();
            }
        }
    });
}

void QuickWebEngineView::HandleWindowClose(){
    TriggerAction(Page::_Close);
}

void QuickWebEngineView::HandleJavascriptConsoleMessage(int level, const QString &msg){
    if(level != 0) return;
    Application::PageKey pageKey;
    if(Application::PageKeyFromMessage(msg, pageKey)){
        if(pageKey.type == QEvent::KeyPress){
            QKeyEvent ke = QKeyEvent(pageKey.type, pageKey.key, pageKey.modifiers);
            Application::InputScope key(true);
            if(!Application::IsOnlyModifier(&ke)) TriggerKeyEvent(&ke);
        }
    } else if(Application::ExactMatch(QStringLiteral("preventScrollRestoration%1").arg(Application::EventToken()), msg)){
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

    const QStringList kinds = NetworkController::PermissionKindsOfFeature(feature);
    PermissionTable::State known = SitePermissions::Lookup(securityOrigin, kinds.first());
    if(kinds.length() == 2)
        known = PermissionTable::Both(known, SitePermissions::Lookup(securityOrigin, kinds.last()));
    if(known != PermissionTable::Ask){
        QMetaObject::invokeMethod(m_QmlWebEngineView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(known == PermissionTable::Allow)));
        return;
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
    if(clicked == Dialog::Yes || clicked == Dialog::No)
        SitePermissions::Remember(securityOrigin, kinds,
                                  clicked == Dialog::Yes ? PermissionTable::Allow : PermissionTable::Block,
                                  [this](){
                                      QQuickWebEngineProfile *profile =
                                          qobject_cast<QQuickWebEngineProfile*>(
                                              m_QmlWebEngineView->property("profile").value<QObject*>());
                                      return !profile || profile->isOffTheRecord();
                                  }());
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
        ? tr("A page is left as it is, because its process was terminated "
             "%1 times in a row.\n").arg(RenderProcessLedger::Limit)
        : tr("A page is reloaded, because its process was terminated.\n");
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

void QuickWebEngineView::HandleJavaScriptDialog(QObject *request){
    enum { Alert = 0, Confirm = 1, Prompt = 2, Unload = 3 };
    const int type = request->property("type").toInt();
    const QString message = request->property("message").toString();
    const QUrl origin = ModalDialog::SiteOrigin(request->property("securityOrigin").toUrl(), url());
    const QString title = ModalDialog::SiteTitle(origin);
    const QString icon = ModalDialog::SiteIcon();
    QPointer<QObject> guard(request);
    bool accept = true;
    QString text;
    switch(type){
    case Alert:
        ModalDialog::Information(title, message, icon);
        break;
    case Prompt: {
        bool ok = true;
        text = ModalDialog::GetText(title, message, request->property("defaultText").toString(), &ok, icon);
        accept = ok;
        break;
    }
    default:
        accept = ModalDialog::Question(title, message, icon);
        break;
    }
    if(!guard) return;
    if(accept) QMetaObject::invokeMethod(request, "dialogAccept", Q_ARG(QString, text));
    else       QMetaObject::invokeMethod(request, "dialogReject");
}

void QuickWebEngineView::HandleContextMenu(QObject *object, bool isMedia){
    if(TakeRightButtonConsumed()) return;

    SharedWebElement elem = m_ClickedElement;
    Page::MediaType type = Page::MediaTypeNone;
    QPoint pos = object->property("position").toPoint();
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

SiteDataLink *QuickWebEngineView::OpenSiteDataLink(){
    return m_QmlWebEngineView ? new QuickWebEngineSiteDataLink(m_QmlWebEngineView) : nullptr;
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

    if(VanillaPage::IsSettingsUrl(url())){
        if(Application::IsMoveKey(ev)) m_PreventScrollRestoration = true;
        QQuickWidget::keyPressEvent(ev);
        return;
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
    MoveHooks hooks;
    hooks.dragParent = this;
    hooks.forward = [this](QMouseEvent *move){ QQuickWidget::mouseMoveEvent(move);};
    hooks.networkAccessManager = [this](){ return static_cast<NetworkAccessManager*>(page()->networkAccessManager());};
    hooks.status = [this](){
        emit statusBarMessage(GestureStatusLine(m_RightGestureMap, tr("NoAction")));
    };
    hooks.acceptForwardedWhileDragging = false;
    MouseMoveWithGestures(ev, hooks);
}

View::ButtonHooks QuickWebEngineView::Buttons(){
    ButtonHooks hooks;
    hooks.pressToEngine = [this](QMouseEvent *ev){ QQuickWidget::mousePressEvent(ev);};
    hooks.releaseToEngine = [this](QMouseEvent *ev){ ForwardMouseReleaseToEngine(ev);};
    hooks.doubleClickToEngine = [this](QMouseEvent *ev){ QQuickWidget::mouseDoubleClickEvent(ev);};
    hooks.acceptForwarded = true;
    hooks.clearStatus = [this](){ emit statusBarMessage(QString());};
    return hooks;
}

void QuickWebEngineView::mousePressEvent(QMouseEvent *ev){
    MousePressWithMap(ev, Buttons());
}

void QuickWebEngineView::mouseReleaseEvent(QMouseEvent *ev){
    MouseReleaseWithGestures(ev, Buttons());
}

void QuickWebEngineView::ForwardMouseReleaseToEngine(QMouseEvent *ev){
    QQuickWidget::mouseReleaseEvent(ev);
}

void QuickWebEngineView::mouseDoubleClickEvent(QMouseEvent *ev){
    MouseDoubleClickWithMap(ev, Buttons());
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
        emit statusBarMessage(GestureStatusLine(m_DragGestureMap, tr("NoAction")));
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

    for(QUrl u : std::as_const(urls)){ if(u.isLocalFile()) isLocal = true;}

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

    WheelHooks hooks;
    hooks.toEngine = [this](QWheelEvent *wheel){
        m_PreventScrollRestoration = true;
        QWheelEvent scaled(wheel->position(), wheel->globalPosition(),
                           wheel->pixelDelta(), View::EngineWheelAngle(wheel->angleDelta()),
                           wheel->buttons(), wheel->modifiers(), wheel->phase(),
                           wheel->inverted(), wheel->source(), wheel->pointingDevice());
        scaled.setTimestamp(wheel->timestamp());
        QQuickWidget::wheelEvent(&scaled);
        wheel->setAccepted(true);
    };
    hooks.actionPos = [](QWheelEvent *wheel){ return wheel->position();};
    WheelWithMap(ev, hooks);
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
    Q_UNUSED(next)
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
            MainWindow *win = Application::GetCurrentWindow();
            QSize s =
                m_TreeBank ? m_TreeBank->size() :
                win ? win->GetTreeBank()->size() :
                !size().isEmpty() ? size() :
                DEFAULT_WINDOW_SIZE;
            QRect viewport = QRect(QPoint(), s);
            callBack(ElementsFrom(this, var, viewport));
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
            callBack(SelectionRegionFrom(var, viewport));
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
