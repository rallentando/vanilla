#include "switch.hpp"
#include "const.hpp"

#ifdef WEBENGINEVIEW

#include "webenginepage.hpp"

#include "loadending.hpp"
#include "page.hpp"

#ifdef VANILLA_PASSWORDS
#  include <QWebChannel>
#endif
#include <QPointer>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineView>

#include <QTimer>
#include <QNetworkReply>
#include <QAction>
#include <QPrinter>
#include <QPrintDialog>
#include <QClipboard>
#include <QAuthenticator>
#include <QPushButton>
#include <QPair>
#include <QSet>
#include <QMenu>
#include <QCursor>
#include <QUrlQuery>
#include <QAbstractListModel>

#include <functional>
#include <memory>

#include "view.hpp"
#include "webengineview.hpp"
#include "webenginepasswords.hpp"
#include "quickwebengineview.hpp"
#include "application.hpp"
#include "runninguploads.hpp"
#include "networkcontroller.hpp"
#include "sitepermissions.hpp"
#include "mainwindow.hpp"
#include "gadgets.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "dialog.hpp"
#include "certificatepolicy.hpp"

WebEnginePage::WebEnginePage(NetworkAccessManager *nam, bool offTheRecord, QObject *parent)
    : QWebEnginePage(nam->GetProfile(offTheRecord), parent)
    , m_Profile(nam->GetSharedProfile(offTheRecord))
{
    setNetworkAccessManager(nam);

    connect(this,   SIGNAL(ViewChanged()),
            parent, SIGNAL(ViewChanged()));
    connect(this,   SIGNAL(ScrollChanged(QPointF)),
            parent, SIGNAL(ScrollChanged(QPointF)));

    connect(this,   SIGNAL(urlChanged(const QUrl&)),
            parent, SIGNAL(urlChanged(const QUrl&)));
    connect(this,   SIGNAL(titleChanged(const QString&)),
            parent, SIGNAL(titleChanged(const QString&)));
    connect(this,   SIGNAL(statusBarMessage2(const QString&, const QString&)),
            parent, SIGNAL(statusBarMessage2(const QString&, const QString&)));
    connect(this,   SIGNAL(iconChanged(const QIcon&)),
            parent, SIGNAL(iconChanged(const QIcon&)));

    connect(this, SIGNAL(linkHovered(const QString&)),
            this, SLOT(OnLinkHovered(const QString&)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect(this, &QWebEnginePage::permissionRequested,
            this, &WebEnginePage::HandlePermission);
#else
    connect(this, SIGNAL(featurePermissionRequested(const QUrl&, QWebEnginePage::Feature)),
            this, SLOT(HandleFeaturePermission(const QUrl&, QWebEnginePage::Feature)));
#endif
    connect(this, SIGNAL(authenticationRequired(const QUrl&, QAuthenticator*)),
            this, SLOT(HandleAuthentication(const QUrl&, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QUrl&, QAuthenticator*, const QString&)),
            this, SLOT(HandleProxyAuthentication(const QUrl&, QAuthenticator*, const QString&)));
    connect(this, SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
            this, SLOT(HandleFullScreen(QWebEngineFullScreenRequest)));
    connect(this, SIGNAL(renderProcessTerminated(RenderProcessTerminationStatus, int)),
            this, SLOT(HandleProcessTermination(RenderProcessTerminationStatus, int)));
    connect(this, SIGNAL(contentsSizeChanged(const QSizeF&)),
            this, SLOT(HandleContentsSizeChange(const QSizeF&)));
    connect(this, SIGNAL(scrollPositionChanged(const QPointF&)),
            this, SLOT(HandleScrollPositionChange(const QPointF&)));

    connect(this, SIGNAL(quotaRequested(QWebEngineQuotaRequest)),
            this, SLOT(HandleQuota(QWebEngineQuotaRequest)));
    connect(this, SIGNAL(registerProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest)),
            this, SLOT(HandleRegisterProtocolHandler(QWebEngineRegisterProtocolHandlerRequest)));
    connect(this, SIGNAL(selectClientCertificate(QWebEngineClientCertificateSelection)),
            this, SLOT(HandleSelectClientCertificate(QWebEngineClientCertificateSelection)));

    connect(this, &QWebEnginePage::certificateError,
            this, &WebEnginePage::HandleCertificateError);
    connect(this, &QWebEnginePage::fileSystemAccessRequested,
            this, &WebEnginePage::HandleFileSystemAccess);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(this, &QWebEnginePage::desktopMediaRequested,
            this, &WebEnginePage::HandleDesktopMedia);
    connect(this, &QWebEnginePage::webAuthUxRequested,
            this, &WebEnginePage::HandleWebAuthUx);
#endif
    connect(this, &QWebEnginePage::printRequested,
            this, &WebEnginePage::HandlePrintRequest);
    connect(this, &QWebEnginePage::loadingChanged,
            this, &WebEnginePage::HandleLoading);
    connect(this, &QWebEnginePage::findTextFinished,
            this, &WebEnginePage::HandleFindTextFinished);

#ifdef VANILLA_PASSWORDS
    {
        QPointer<WebEnginePage> page(this);
        WebEnginePasswords::Host host;
        host.current = [page](){
            WebEnginePasswords::Snapshot now;
            if(!page) return now;
            now.alive = true;
            now.url = page->url();
            now.space = NetworkController::CredentialSpace(page->profile());
            now.profile = page->profile();
            now.isPrivate = page->profile()->isOffTheRecord();
            now.zoom = page->zoomFactor();
            return now;
        };
        host.runInApplicationWorld = [page](const QString &code,
                                            std::function<void(const QVariant&)> done){
            if(page) page->runJavaScript(code, QWebEngineScript::ApplicationWorld, done);
        };
        host.mainFrame = [page](){
            return page ? page->mainFrame() : QWebEngineFrame();
        };
        host.mapToGlobal = [page](const QPoint &pos){
            QWebEngineView *view = page ? QWebEngineView::forPage(page) : nullptr;
            return view ? view->mapToGlobal(pos) : QCursor::pos();
        };
        host.message = [page](const QString &text){
            if(page) emit page->statusBarMessage(text);
        };
        host.widget = [page]() -> QWidget* {
            return page ? QWebEngineView::forPage(page) : nullptr;
        };
        host.managePasswords = [page](){
            if(!page) return;
            if(TreeBank *tb = page->m_View->GetTreeBank()) tb->OpenPasswords();
        };
        WebEnginePasswords *passwords = new WebEnginePasswords(this, host);
        QWebChannel *channel = new QWebChannel(passwords);
        channel->registerObject(QStringLiteral("vanillaPasswords"), passwords->Bridge());
        setWebChannel(channel, QWebEngineScript::ApplicationWorld);
        connect(this, &QWebEnginePage::loadStarted, passwords, &WebEnginePasswords::Forget);
        connect(this, &QWebEnginePage::urlChanged, passwords, &WebEnginePasswords::Navigated);
    }
#endif

    if(parent){
        if(WebEngineView *w = qobject_cast<WebEngineView*>(parent))
            m_View = w;
        else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(parent))
            m_View = w;
        else m_View = nullptr;
    } else m_View = nullptr;

    m_Page = new Page(this);
    m_Page->SetView(m_View);

    connect(this, SIGNAL(windowCloseRequested()), m_Page, SLOT(Close()));
}

WebEnginePage::~WebEnginePage(){
    setNetworkAccessManager(nullptr);
}

View* WebEnginePage::GetView(){
    return m_View;
}

WebEnginePage* WebEnginePage::createWindow(WebWindowType type){

    if(View::TakeOwnDrop()) return nullptr;

    static const QUrl blank = BLANK_URL;

    View *view = type == QWebEnginePage::WebBrowserBackgroundTab
        ? OpenInNewBackground(blank)
        : OpenInNew(blank);

    if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base())){
        return w->page();
    }
    return this;
}

void WebEnginePage::triggerAction(WebAction action, bool checked){
    QWebEnginePage::triggerAction(action, checked);
}

bool WebEnginePage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame){
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

QStringList WebEnginePage::chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                                       const QStringList &acceptedMimeTypes){
    Q_UNUSED(acceptedMimeTypes)

    QStringList suggestedFiles = oldFiles;

    if(suggestedFiles.isEmpty() || suggestedFiles.first().isEmpty()){
        suggestedFiles = QStringList() << Application::GetUploadDirectory();
    }

    QStringList files;

    if(mode == QWebEnginePage::FileSelectOpenMultiple){

        files = ModalDialog::GetOpenFileNames(QString(), suggestedFiles.first());

    } else if(mode == QWebEnginePage::FileSelectOpen){

        files << ModalDialog::GetOpenFileName_(QString(), suggestedFiles.first());
    }

    if(!files.isEmpty() && !files.first().isEmpty()){
        QString file = files.first();
        if(!file.contains(QStringLiteral("/"))) return files;
        QStringList path = file.split(QStringLiteral("/"));
        path.removeLast();
        Application::SetUploadDirectory(path.join(QStringLiteral("/")));
    }
    return files;
}

void WebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &msg,
                                             int lineNumber, const QString &sourceId){
    Application::PageKey pageKey;
    if(Application::PageKeyFromMessage(msg, pageKey)){
        QKeyEvent ke = QKeyEvent(pageKey.type, pageKey.key, pageKey.modifiers);
        if(pageKey.type == QEvent::KeyPress){
            Application::InputScope key(true);
            m_View->KeyPressEvent(&ke);
        } else {
            m_View->KeyReleaseEvent(&ke);
        }
        return;
    }
    QWebEnginePage::javaScriptConsoleMessage(level, msg, lineNumber, sourceId);
}

QNetworkAccessManager *WebEnginePage::networkAccessManager() const {
    return m_NetworkAccessManager;
}

void WebEnginePage::setNetworkAccessManager(QNetworkAccessManager *nam){
    m_NetworkAccessManager = nam;
}

QString WebEnginePage::userAgentForUrl(const QUrl &url) const {
    Q_UNUSED(url)
    return static_cast<NetworkAccessManager*>(networkAccessManager())->GetUserAgent();
}

void WebEnginePage::DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                                       QPoint localPos, QPoint globalPos, Page::MediaType type){
    m_Page->DisplayContextMenu(parent, elem, localPos, globalPos, type);
}

void WebEnginePage::TriggerAction(Page::CustomAction action, QVariant data){
    Action(action, data)->trigger();
}

QAction *WebEnginePage::Action(Page::CustomAction a, QVariant data){
    return m_Page->Action(a, data);
}

void WebEnginePage::OnLinkHovered(const QString &url){
    emit linkHovered(url, QString(), QString());
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)

void WebEnginePage::HandlePermission(QWebEnginePermission permission){
    if(!permission.isValid()) return;

    const QStringList kinds = NetworkController::PermissionKindsOfType
        (static_cast<int>(permission.permissionType()));
    if(!kinds.isEmpty()){
        PermissionTable::State known = SitePermissions::Lookup(permission.origin(), kinds.first());
        if(kinds.length() == 2)
            known = PermissionTable::Both(known, SitePermissions::Lookup(permission.origin(), kinds.last()));
        if(known == PermissionTable::Allow){ permission.grant(); return;}
        if(known == PermissionTable::Block){ permission.deny();  return;}
    }

    QString typeString;
    switch(permission.permissionType()){
    case QWebEnginePermission::PermissionType::Notifications:
        typeString = tr("Notifications");             break;
    case QWebEnginePermission::PermissionType::Geolocation:
        typeString = tr("Geolocation");               break;
    case QWebEnginePermission::PermissionType::MediaAudioCapture:
        typeString = tr("MediaAudioCapture");         break;
    case QWebEnginePermission::PermissionType::MediaVideoCapture:
        typeString = tr("MediaVideoCapture");         break;
    case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
        typeString = tr("MediaAudioVideoCapture");    break;
    case QWebEnginePermission::PermissionType::MouseLock:
        typeString = tr("MouseLock");                 break;
    case QWebEnginePermission::PermissionType::DesktopVideoCapture:
        typeString = tr("DesktopVideoCapture");       break;
    case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
        typeString = tr("DesktopAudioVideoCapture");  break;
    case QWebEnginePermission::PermissionType::ClipboardReadWrite:
        typeString = tr("ClipboardReadWrite");        break;
    case QWebEnginePermission::PermissionType::LocalFontsAccess:
        typeString = tr("LocalFontsAccess");          break;
    case QWebEnginePermission::PermissionType::Unsupported:
        permission.deny();
        return;
    }

    ModalDialog *dialog = new ModalDialog();
    dialog->SetTitle(tr("Feature Permission Requested."));
    dialog->SetCaption(tr("Feature Permission Requested."));
    dialog->SetInformativeText
        (tr("Url: ") + permission.origin().toString() + QStringLiteral("\n") +
         tr("Feature: ") + typeString + QStringLiteral("\n\n") +
         tr("Allow this feature?"));
    dialog->SetButtons(Dialog::Yes | Dialog::No | Dialog::Cancel);
    dialog->Execute();

    const Dialog::Button clicked = dialog->ClickedButton();
    if(clicked != Dialog::Yes && clicked != Dialog::No) return;

    if(clicked == Dialog::Yes) permission.grant();
    else permission.deny();
    SitePermissions::Remember(permission.origin(), kinds,
                              clicked == Dialog::Yes ? PermissionTable::Allow : PermissionTable::Block,
                              profile()->isOffTheRecord());
}
#else
void WebEnginePage::HandleFeaturePermission(const QUrl &securityOrigin,
                                            QWebEnginePage::Feature feature){
    QString featureString;
    switch(feature){
    case QWebEnginePage::Notifications:
        featureString = QStringLiteral("Notifications");          break;
    case QWebEnginePage::Geolocation:
        featureString = QStringLiteral("Geolocation");            break;
    case QWebEnginePage::MediaAudioCapture:
        featureString = QStringLiteral("MediaAudioCapture");      break;
    case QWebEnginePage::MediaVideoCapture:
        featureString = QStringLiteral("MediaVideoCapture");      break;
    case QWebEnginePage::MediaAudioVideoCapture:
        featureString = QStringLiteral("MediaAudioVideoCapture"); break;
    case QWebEnginePage::MouseLock:
        featureString = QStringLiteral("MouseLock");              break;
    case QWebEnginePage::DesktopVideoCapture:
        featureString = QStringLiteral("DesktopVideoCapture");    break;
    case QWebEnginePage::DesktopAudioVideoCapture:
        featureString = QStringLiteral("DesktopAudioVideoCapture"); break;
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
        setFeaturePermission(securityOrigin, feature,
                             QWebEnginePage::PermissionGrantedByUser);
    } else if(clicked == Dialog::No){
        setFeaturePermission(securityOrigin, feature,
                             QWebEnginePage::PermissionDeniedByUser);
    } else if(clicked == Dialog::Cancel){
        emit featurePermissionRequestCanceled(securityOrigin, feature);
    }
}
#endif

void WebEnginePage::HandleAuthentication(const QUrl &requestUrl,
                                         QAuthenticator *authenticator){
    ModalDialog::Authentication(authenticator, ModalDialog::AuthenticationCaption(requestUrl));
}

void WebEnginePage::HandleProxyAuthentication(const QUrl &requestUrl,
                                              QAuthenticator *authenticator,
                                              const QString &proxyHost){
    ModalDialog::Authentication(authenticator, ModalDialog::AuthenticationCaption(requestUrl, proxyHost));
}

void WebEnginePage::HandleFullScreen(QWebEngineFullScreenRequest request){
    if(TreeBank *tb = m_View->GetTreeBank()){
        bool on = request.toggleOn();
        tb->GetMainWindow()->SetFullScreen(on);
        m_View->SetDisplayObscured(on);
        request.accept();
        if(!on) return;
        ModelessDialog *dialog = new ModelessDialog();
        connect(this, &WebEnginePage::destroyed, dialog, &ModelessDialog::Returned);
        connect(this, &WebEnginePage::fullScreenRequested, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(tr("This page becomes full screen mode."));
        dialog->SetCaption(tr("Press Esc to exit."));
        dialog->SetButtons(Dialog::Ok | Dialog::Cancel);
        dialog->SetDefaultValue(true);
        dialog->SetCallBack([this](bool ok){ if(!ok) triggerAction(ExitFullScreen);});
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    } else {
        request.reject();
    }
}

void WebEnginePage::HandleProcessTermination(RenderProcessTerminationStatus status, int code){
    RunningUploads::Instance()->Ended(m_View);

    if(lifecycleState() == QWebEnginePage::LifecycleState::Discarded) return;

    const RenderProcessLedger::Verdict verdict =
        m_View->RenderProcessDeaths().Count();
    if(verdict == RenderProcessLedger::Verdict::Silent) return;
    const bool giveUp = verdict == RenderProcessLedger::Verdict::GiveUp;

    QString info = giveUp
        ? tr("A page is left as it is, because its process was terminated "
             "%1 times in a row.\n").arg(RenderProcessLedger::Limit)
        : tr("A page is reloaded, because its process was terminated.\n");
    switch(status){
    case QWebEnginePage::NormalTerminationStatus:
        info += tr("Normal termination. (code: %1)");   break;
    case QWebEnginePage::AbnormalTerminationStatus:
        info += tr("Abnormal termination. (code: %1)"); break;
    case QWebEnginePage::CrashedTerminationStatus:
        info += tr("Crashed termination. (code: %1)");  break;
    case QWebEnginePage::KilledTerminationStatus:
        info += tr("Killed termination. (code: %1)");   break;
    }
    ModelessDialog::Information(giveUp
                                ? tr("Render process terminated repeatedly.")
                                : tr("Render process terminated."),
                                info.arg(code), m_View->base());
    if(giveUp) return;
    QTimer::singleShot(0, m_Page, SLOT(Reload()));
}

void WebEnginePage::HandleContentsSizeChange(const QSizeF &size){
    Q_UNUSED(size)
    m_View->RestoreScroll();
}

void WebEnginePage::HandleScrollPositionChange(const QPointF &pos){
    Q_UNUSED(pos)
    m_View->EmitScrollChanged();
}

namespace {

    template <class Request>
    void AskAllowOrBlock(WebEnginePage *page, const QString &title, const QString &info, Request request){
        ModelessDialog *dialog = new ModelessDialog();
        QObject::connect(page, &WebEnginePage::destroyed, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(title);
        dialog->SetCaption(info);
        dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
        dialog->SetDefaultValue(false);
        dialog->SetCallBack([dialog, request](bool ok) mutable {
            if(!ok) return;
            if(dialog->ClickedButton() == Dialog::Allow) request.accept();
            if(dialog->ClickedButton() == Dialog::Block) request.reject();
        });
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    }
}

void WebEnginePage::HandleQuota(QWebEngineQuotaRequest request){
    QString info = tr("This page requests persistent storage.\n");
    info += tr("Url: %1\n").arg(request.origin().toString());
    info += tr("Size: %1 bytes.").arg(request.requestedSize());
    AskAllowOrBlock(this, tr("Quota requested."), info, request);
}

void WebEnginePage::HandleRegisterProtocolHandler(QWebEngineRegisterProtocolHandlerRequest request){
    QString info = tr("This page tries to register a custom protocol.\n");
    info += tr("Url: %1\n").arg(request.origin().toString());
    info += tr("Scheme: %1").arg(request.scheme());
    AskAllowOrBlock(this, tr("Register protocol handler requested."), info, request);
}

void WebEnginePage::HandleSelectClientCertificate(QWebEngineClientCertificateSelection selection){

    static QMap<QUrl, QString> map = QMap<QUrl, QString>();

    QStringList items = QStringList();
    foreach(QSslCertificate cert, selection.certificates()){
        items << QStringLiteral("%1 - %2")
            .arg(cert.subjectInfo(QSslCertificate::CommonName).join(", "))
            .arg(cert.issuerInfo(QSslCertificate::CommonName).join(", "));
    }

    if(map.contains(selection.host())){
        selection.select(selection.certificates().at(items.indexOf(map[selection.host()])));
        return;
    }

    bool ok = true;
    QString cert = ModalDialog::GetItem
        (tr("This page requests a certificate."),
         tr("Select client certificate."),
         items, false, &ok);

    if(!ok) return;

    map[selection.host()] = cert;

    selection.select(selection.certificates().at(items.indexOf(cert)));
}

void WebEnginePage::HandleCertificateError(QWebEngineCertificateError error){
    if(!error.isOverridable()){
        error.rejectCertificate();
        return;
    }

    error.defer();

    Application::AskSslErrorPolicyIfNeed();

    const QString host = error.url().host();
    const bool offTheRecord = Page::OffTheRecord(m_View);

    switch(Application::GetSslErrorPolicy()){
    case Application::IgnoreSslErrors:
        error.acceptCertificate();
        return;
    case Application::AskForEachAccess:{
        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("SSL errors."));
        dialog->SetCaption(tr("SSL errors."));
        dialog->SetInformativeText(tr("Ignore errors in this access?"));
        dialog->SetDetailedText(tr("Url: %1\nSsl error : %2")
                                .arg(error.url().toString(), error.description()));
        dialog->SetButtons(Dialog::Allow | Dialog::Block);
        if(dialog->Execute() && dialog->ClickedButton() == Dialog::Allow)
            error.acceptCertificate();
        else
            error.rejectCertificate();
        return;
    }
    case Application::AskForEachHost:{
        if(Application::GetBlockedHosts().contains(host)){
            error.rejectCertificate();
            return;
        }
        if(Application::GetAllowedHosts().contains(host)){
            error.acceptCertificate();
            return;
        }
        const Dialog::Button clicked = ModalDialog::AllowBlockOrCancel
            (tr("SSL errors on host: %1").arg(host),
             tr("Allow or Block this host?"),
             tr("Url: %1\nSsl error : %2").arg(error.url().toString(), error.description()));
        if(clicked == Dialog::Allow){
            Application::AppendToAllowedHosts(host, offTheRecord);
            error.acceptCertificate();
        } else {
            if(clicked == Dialog::Block) Application::AppendToBlockedHosts(host, offTheRecord);
            error.rejectCertificate();
        }
        return;
    }
    case Application::AskForEachCertificate:{
        const QList<QSslCertificate> chain = error.certificateChain();
        const QString fingerprint = CertificatePolicy::Fingerprint(chain);
        const QString key =
            CertificatePolicy::KeyFromFingerprint(host, fingerprint);
        switch(CertificatePolicy::Find(key,
                                       Application::GetAllowedCertificates(),
                                       Application::GetBlockedCertificates())){
        case CertificatePolicy::Allow:
            error.acceptCertificate();
            return;
        case CertificatePolicy::Block:
            error.rejectCertificate();
            return;
        case CertificatePolicy::Ask:
            break;
        }

        QString detail = tr("Url: %1\nSsl error : %2")
            .arg(error.url().toString(), error.description());
        if(!fingerprint.isEmpty())
            detail += QStringLiteral("\nSHA-256: ") + fingerprint;
        const Dialog::Button clicked = ModalDialog::AllowBlockOrCancel
            (tr("SSL errors on host: %1").arg(host),
             tr("Allow or Block this certificate?"), detail);
        if(clicked == Dialog::Allow){
            Application::RememberCertificate(key, true, offTheRecord);
            error.acceptCertificate();
        } else {
            if(clicked == Dialog::Block) Application::RememberCertificate(key, false, offTheRecord);
            error.rejectCertificate();
        }
        return;
    }
    default:
        error.rejectCertificate();
        return;
    }
}

void WebEnginePage::HandleFileSystemAccess(QWebEngineFileSystemAccessRequest request){
    const bool directory =
        request.handleType() == QWebEngineFileSystemAccessRequest::Directory;
    const bool write =
        request.accessFlags() & QWebEngineFileSystemAccessRequest::Write;

    QString info = tr("This page requests access to a file on this machine.\n");
    info += tr("Url: %1\n").arg(request.origin().toString());
    info += (directory ? tr("Directory: %1\n") : tr("File: %1\n"))
        .arg(request.filePath().toLocalFile());
    info += write ? tr("Access: read and write") : tr("Access: read");

    AskAllowOrBlock(this, tr("File system access requested."), info, request);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)

void WebEnginePage::HandleDesktopMedia(const QWebEngineDesktopMediaRequest &request){
    QAbstractListModel *screens = request.screensModel();
    QAbstractListModel *windows = request.windowsModel();

    QStringList items;
    const int screenCount = screens ? screens->rowCount() : 0;
    const int windowCount = windows ? windows->rowCount() : 0;

    for(int i = 0; i < screenCount; i++)
        items << tr("Screen: %1").arg(screens->index(i, 0).data().toString());
    for(int i = 0; i < windowCount; i++)
        items << tr("Window: %1").arg(windows->index(i, 0).data().toString());

    if(items.isEmpty()){
        request.cancel();
        return;
    }

    bool ok = true;
    const QString chosen = ModalDialog::GetItem
        (tr("This page requests to capture the screen."),
         tr("Select what to share."),
         items, false, &ok);

    const int index = ok ? items.indexOf(chosen) : -1;
    if(index < 0){
        request.cancel();
    } else if(index < screenCount){
        request.selectScreen(screens->index(index, 0));
    } else {
        request.selectWindow(windows->index(index - screenCount, 0));
    }
}

void WebEnginePage::HandleWebAuthUx(QWebEngineWebAuthUxRequest *request){
    if(!request) return;

    connect(request, &QWebEngineWebAuthUxRequest::stateChanged,
            this, [this, request](QWebEngineWebAuthUxRequest::WebAuthUxState){
                HandleWebAuthUx(request);
            }, Qt::UniqueConnection);

    switch(request->state()){
    case QWebEngineWebAuthUxRequest::WebAuthUxState::SelectAccount:{
        const QStringList names = request->userNames();
        if(names.isEmpty()){ request->cancel(); return;}
        bool ok = true;
        const QString name = ModalDialog::GetItem
            (tr("Sign in to %1").arg(request->relyingPartyId()),
             tr("Select an account."), names, false, &ok);
        if(ok && names.contains(name)) request->setSelectedAccount(name);
        else request->cancel();
        return;
    }
    case QWebEngineWebAuthUxRequest::WebAuthUxState::CollectPin:{
        const QWebEngineWebAuthPinRequest pin = request->pinRequest();

        QString caption;
        switch(pin.reason){
        case QWebEngineWebAuthUxRequest::PinEntryReason::Set:
            caption = tr("Set a PIN for this security key."); break;
        case QWebEngineWebAuthUxRequest::PinEntryReason::Change:
            caption = tr("Change the PIN of this security key."); break;
        case QWebEngineWebAuthUxRequest::PinEntryReason::Challenge:
            caption = tr("Enter the PIN of this security key."); break;
        }
        if(pin.error != QWebEngineWebAuthUxRequest::PinEntryError::NoError)
            caption += tr("\n%1 attempts left.").arg(pin.remainingAttempts);

        bool ok = true;
        const QString entered = ModalDialog::GetPass
            (tr("Sign in to %1").arg(request->relyingPartyId()),
             caption, QString(), &ok);
        if(ok && !entered.isEmpty()) request->setPin(entered);
        else request->cancel();
        return;
    }
    case QWebEngineWebAuthUxRequest::WebAuthUxState::RequestFailed:
        ModelessDialog::Information
            (tr("Sign in failed."),
             tr("The security key could not be used."),
             m_View ? m_View->base() : nullptr);
        request->cancel();
        return;
    default:
        return;
    }
}
#endif

void WebEnginePage::HandlePrintRequest(){
    if(m_Page) m_Page->Print();
}

void WebEnginePage::HandleLoading(const QWebEngineLoadingInfo &info){
    const bool own = info.url().scheme() == VANILLA_SCHEME;
    if(own != m_SuspendedSpecificSettings){
        m_SuspendedSpecificSettings = own;
        if(own)
            settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        else if(m_View)
            m_View->ApplySpecificSettings(m_View->SpecificSettings());
    }

    switch(LoadEnding::WebEngineEnding(int(info.status()), int(info.errorDomain()))){
    case LoadEnding::Verdict::Clear:
        emit statusBarMessage2(QString(), QString());
        return;
    case LoadEnding::Verdict::Report:
        emit statusBarMessage2(tr("Failed to load: %1")
                               .arg(LoadEnding::WebEngineErrorText
                                    (info.errorString(), int(info.errorDomain()),
                                     info.errorCode(), info.url())),
                               QString());
        return;
    case LoadEnding::Verdict::Nothing:
        return;
    }
}

void WebEnginePage::HandleFindTextFinished(const QWebEngineFindTextResult &result){
    if(result.numberOfMatches() == 0){
        emit statusBarMessage2(tr("Not found."), QString());
        return;
    }
    emit statusBarMessage2(tr("%1 of %2 matches.")
                           .arg(result.activeMatch())
                           .arg(result.numberOfMatches()),
                           QString());
}

void WebEnginePage::InspectElement(){
    QWebEnginePage::triggerAction(QWebEnginePage::InspectElement);
}

void WebEnginePage::AddSearchEngine(QPoint pos){
    m_Page->AddSearchEngineAt(pos);
}

void WebEnginePage::AddBookmarklet(QPoint pos){
    m_Page->AddBookmarkletAt(pos);
}

void WebEnginePage::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    NetworkAccessManager::SetRequestOffTheRecord(req, Page::OffTheRecord(m_View));
    DownloadItem *item =
        NetworkController::Download(static_cast<NetworkAccessManager*>(networkAccessManager()),
                                    req, NetworkController::ToVariable);

    if(!item) return;

    connect(item, &DownloadItem::DownloadResult, this, &WebEnginePage::SuggestResult);
}

#endif
