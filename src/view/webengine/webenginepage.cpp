#include "switch.hpp"
#include "const.hpp"

#ifdef WEBENGINEVIEW

#include "webenginepage.hpp"

#include "loadending.hpp"
#include "page.hpp"

#ifdef USE_WEBCHANNEL
#  include <QWebChannel>
#endif

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
#include "quickwebengineview.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "gadgets.hpp"
#include "networkcontroller.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "jsobject.hpp"
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

#ifdef USE_WEBCHANNEL
    AddJsObject();
#endif
}

WebEnginePage::~WebEnginePage(){
#ifdef USE_WEBCHANNEL
    if(webChannel()) delete webChannel();
#endif
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
#ifdef USE_WEBCHANNEL
#endif
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
        foreach(QString file, files){
            Application::AppendChosenFile(file);
        }
    }
    return files;
}

void WebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &msg,
                                             int lineNumber, const QString &sourceId){
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventToken()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        m_View->KeyPressEvent(&ke);
        return;
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventToken()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyRelease, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        m_View->KeyReleaseEvent(&ke);
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
    if     (clicked == Dialog::Yes) permission.grant();
    else if(clicked == Dialog::No)  permission.deny();
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
    Q_UNUSED(requestUrl)

    ModalDialog::Authentication(authenticator);
}

void WebEnginePage::HandleProxyAuthentication(const QUrl &requestUrl,
                                              QAuthenticator *authenticator,
                                              const QString &proxyHost){
    Q_UNUSED(requestUrl)
    Q_UNUSED(proxyHost)

    ModalDialog::Authentication(authenticator);
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
    if(lifecycleState() == QWebEnginePage::LifecycleState::Discarded) return;

    const RenderProcessLedger::Verdict verdict =
        m_View->RenderProcessDeaths().Count();
    if(verdict == RenderProcessLedger::Verdict::Silent) return;
    const bool giveUp = verdict == RenderProcessLedger::Verdict::GiveUp;

    QString info = giveUp
        ? tr("A page is left as it is, because that's process is terminated "
             "%1 times in a row.\n").arg(RenderProcessLedger::Limit)
        : tr("A page is reloaded, because that's process is terminated.\n");
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

void WebEnginePage::HandleQuota(QWebEngineQuotaRequest request){
    QString info = tr("This page requests persistent storage.\n");
    info += tr("Url: %1\n").arg(request.origin().toString());
    info += tr("Size: %1 bytes.").arg(request.requestedSize());
    ModelessDialog *dialog = new ModelessDialog();
    connect(this, &WebEnginePage::destroyed, dialog, &ModelessDialog::Returned);
    dialog->SetTitle(tr("Quota requested."));
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

void WebEnginePage::HandleRegisterProtocolHandler(QWebEngineRegisterProtocolHandlerRequest request){
    QString info = tr("This page tries to register a custom protocol.\n");
    info += tr("Url: %1\n").arg(request.origin().toString());
    info += tr("Scheme: %1").arg(request.scheme());
    ModelessDialog *dialog = new ModelessDialog();
    connect(this, &WebEnginePage::destroyed, dialog, &ModelessDialog::Returned);
    dialog->SetTitle(tr("Register protocol handler requested."));
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

    switch(Application::GetSslErrorPolicy()){
    case Application::IgnoreSslErrors:
        error.acceptCertificate();
        return;
    case Application::AskForEachAccess:{
        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors."));
        dialog->SetCaption(tr("Ssl errors."));
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
        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors on host:%1").arg(host));
        dialog->SetCaption(tr("Ssl errors on host:%1").arg(host));
        dialog->SetInformativeText(tr("Allow or Block this host?"));
        dialog->SetDetailedText(tr("Url: %1\nSsl error : %2")
                                .arg(error.url().toString(), error.description()));
        dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
        dialog->Execute();

        const Dialog::Button clicked = dialog->ClickedButton();
        if(clicked == Dialog::Allow){
            Application::AppendToAllowedHosts(host);
            error.acceptCertificate();
        } else {
            if(clicked == Dialog::Block) Application::AppendToBlockedHosts(host);
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

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors on host:%1").arg(host));
        dialog->SetCaption(tr("Ssl errors on host:%1").arg(host));
        dialog->SetInformativeText(tr("Allow or Block this certificate?"));
        QString detail = tr("Url: %1\nSsl error : %2")
            .arg(error.url().toString(), error.description());
        if(!fingerprint.isEmpty())
            detail += QStringLiteral("\nSHA-256: ") + fingerprint;
        dialog->SetDetailedText(detail);
        dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
        dialog->Execute();

        const Dialog::Button clicked = dialog->ClickedButton();
        if(clicked == Dialog::Allow){
            Application::RememberCertificate(key, true);
            error.acceptCertificate();
        } else {
            if(clicked == Dialog::Block) Application::RememberCertificate(key, false);
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

    ModelessDialog *dialog = new ModelessDialog();
    connect(this, &WebEnginePage::destroyed, dialog, &ModelessDialog::Returned);
    dialog->SetTitle(tr("File system access requested."));
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

void WebEnginePage::HandleUnsupportedContent(QNetworkReply *reply){
    Q_UNUSED(reply)
}

void WebEnginePage::AddJsObject(){
#ifdef USE_WEBCHANNEL

    QWebChannel *channel = webChannel();

    setWebChannel(new QWebChannel(this));
    if(channel) delete channel;

    if(m_View && m_View->GetJsObject()){
        webChannel()->registerObject(QStringLiteral("_view"), m_View->GetJsObject());
        m_View->OnSetJsObject(m_View->GetJsObject());
    }
    if(m_View && m_View->GetTreeBank() && m_View->GetTreeBank()->GetJsObject()){
        webChannel()->registerObject(QStringLiteral("_vanilla"), m_View->GetTreeBank()->GetJsObject());
        m_View->OnSetJsObject(m_View->GetTreeBank()->GetJsObject());
    }
#endif
}

void WebEnginePage::InspectElement(){
    QWebEnginePage::triggerAction(QWebEnginePage::InspectElement);
}

void WebEnginePage::AddSearchEngine(QPoint pos){
    m_View->CallWithGotCurrentBaseUrl([this, pos](QUrl base){

    m_View->CallWithEvaluatedJavaScriptResult(QStringLiteral(
"(function(){\n"
"    var x = %1;\n"
"    var y = %2;\n"
"    var elem = document.elementFromPoint(x, y);\n"
"    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
"        try{\n"
"            var frameDocument = elem.contentDocument;\n"
"            var rect = elem.getBoundingClientRect();\n"
"            x -= rect.left;\n"
"            y -= rect.top;\n"
"            elem = frameDocument.elementFromPoint(x, y);\n"
"        }\n"
"        catch(e){ break;}\n"
"    }\n"
"    if(!elem) return {};\n"
"    var name = elem.name;\n"
"    var form = elem;\n"
"    while(form && form.tagName != \"FORM\"){\n"
"        form = form.parentNode;\n"
"    }\n"
"    if(!form) return {};\n"
"    var encode = form.getAttribute(\"accept-charset\") || form.ownerDocument.charset || \"UTF-8\";\n"
"    var method = form.method || \"get\";\n"
"    if(method.toLowerCase() != \"get\") return {};\n"
"    var result = {};\n"
"    var queries = {};\n"
"    var engines = {};\n"
"    var inputs = form.getElementsByTagName(\"input\");\n"
"    var buttons = form.getElementsByTagName(\"button\");\n"
"    var selects = form.getElementsByTagName(\"select\");\n"
"    for(var i = 0; i < inputs.length; i++){\n"
"        var field = inputs[i];\n"
"        var type = (field.type || \"text\").toLowerCase();\n"
"        var name = field.name;\n"
"        var val = field.value;\n"
"        if(type == \"submit\"){\n"
"            engines[name] = val; continue;\n"
"        } else if(type == \"text\" || type == \"search\"){\n"
"            if(field == elem) val = \"{query}\";\n"
"        } else if(type == \"checkbox\" || type == \"radio\"){\n"
"            if(!field.checked) continue;\n"
"        } else if(type != \"hidden\") continue;\n"
"        queries[name] = val;\n"
"    }\n"
"    for(var i = 0; i < buttons.length; i++){\n"
"        var button = buttons[i];\n"
"        engines[button.name] = button.getAttribute(\"aria-label\");\n"
"    }\n"
"    for(var i = 0; i < selects.length; i++){\n"
"        var select = selects[i];\n"
"        var index = select.selectedIndex;\n"
"        if(index != -1){\n"
"            var options = select.getElementsByTagName(\"option\");\n"
"            queries[select.name] = options[index].textContent;\n"
"        }\n"
"    }\n"
"    var labels = form.querySelectorAll(\"label[for=\\\"\"+name+\"\\\"]\");\n"
"    var tag = labels.length ? labels[0].innerText : \"\";\n"
"    result[0] = encode;\n"
"    result[1] = method;\n"
"    result[2] = form.action;\n"
"    result[3] = queries;\n"
"    result[4] = engines;\n"
"    result[5] = tag;\n"
"    return result;\n"
"})();").arg(pos.x()).arg(pos.y()),
[base](QVariant var){

    if(!var.isValid()) return;
    QVariantList list = var.toMap().values();
    if(list.isEmpty()) return;

    QString encode = list[0].toString();
    QString method = list[1].toString();
    QUrl result = Page::StringToUrl(list[2].toString(), base);
    QUrlQuery queries = QUrlQuery(result);
    QMap<QString, QString> engines;
    foreach(QString key, list[3].toMap().keys()){
        QString k = QString::fromLatin1(QUrl::toPercentEncoding(key));
        QString v = QString::fromLatin1(QUrl::toPercentEncoding(list[3].toMap()[key].toString()));
        queries.addQueryItem(k, v);
    }
    foreach(QString key, list[4].toMap().keys()){
        QString k = QString::fromLatin1(QUrl::toPercentEncoding(key));
        QString v = QString::fromLatin1(QUrl::toPercentEncoding(list[4].toMap()[key].toString()));
        engines[k] = v;
    }
    QString tag = list[5].toString();

    bool ok = true;
    if(engines.count() > 1){

        QString engine = ModalDialog::GetItem
            (tr("Search button"),
             tr("Select search button."),
             engines.keys(), false, &ok);

        if(!ok) return;
        if(!engines[engine].isEmpty()){
            queries.addQueryItem(engine, engines[engine]);
        }
    }

    tag = ModalDialog::GetText
        (tr("Search tag"),
         tr("Input search tag.(It will be used as command)"),
         tag, &ok);

    if(!ok || tag.isEmpty()) return;

    QStringList format;

    result.setQuery(queries);
    format
        << result.toString()
             .replace(QStringLiteral("{query}"),     QStringLiteral("%1"))
             .replace(QStringLiteral("%7Bquery%7D"), QStringLiteral("%1"))
        << encode << QStringLiteral("false");

    Page::RegisterSearchEngine(tag, format);
    });});
}

void WebEnginePage::AddBookmarklet(QPoint pos){
    m_View->CallWithHitElement(pos, [this](SharedWebElement elem){

    QUrl link  = elem ? elem->LinkUrl()  : QUrl();
    QUrl image = elem ? elem->ImageUrl() : QUrl();
    QString text = selectedText();

    QStringList places;
    if(!link.isEmpty())  places << tr("Link at Mouse Cursor");
    if(!image.isEmpty()) places << tr("Image at Mouse Cursor");
    if(!text.isEmpty())  places << tr("Selected Text");

    places << tr("Manual Input");

    bool ok;

    QString place = ModalDialog::GetItem
        (tr("Input type"),
         tr("Select input type of bookmarklet."),
         places, false, &ok);
    if(!ok) return;

    QString bookmark;

    if(place == tr("Link at Mouse Cursor")){
        bookmark = link.toString(QUrl::None);

        int count = 2;

        for(int i = count; i > 0; i--){
            bookmark.replace(QStringLiteral("%%%%%%%"), QStringLiteral("%25%25%25%25%25%25%25"));
            bookmark.replace(QStringLiteral("%%%%%"), QStringLiteral("%25%25%25%25%25"));
            bookmark.replace(QStringLiteral("%%%"), QStringLiteral("%25%25%25"));
            bookmark.replace(QStringLiteral("%%"), QStringLiteral("%25%25"));
            bookmark.replace(QRegularExpression(QStringLiteral("%([^0-9A-F][0-9A-F]|[0-9A-F][^0-9A-F]|[^0-9A-F][^0-9A-F])")),
                             QStringLiteral("%25\\1"));
            bookmark = QUrl::fromPercentEncoding(bookmark.toLatin1());
        }
    }

    if(place == tr("Image at Mouse Cursor")){
        bookmark = image.toDisplayString(QUrl::FullyDecoded);
    }

    if(place == tr("Selected Text")){
        bookmark = text;
    }

    if(place == tr("Manual Input")){
        bookmark = ModalDialog::GetText
            (tr("Bookmarklet body"),
             tr("Input bookmarklet body."),
             QString(), &ok);
    }

    if(bookmark.isEmpty()) return;

    QString tag = ModalDialog::GetText
        (tr("Bookmarklet Name"),
         tr("Input bookmarklet name.(It will be used as command)"),
         QString(), &ok);

    if(!ok || tag.isEmpty()) return;

    QStringList result;
    result << bookmark;

    Page::RegisterBookmarklet(tag, result);
    });
}

void WebEnginePage::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    DownloadItem *item =
        NetworkController::Download(static_cast<NetworkAccessManager*>(networkAccessManager()),
                                    req, NetworkController::ToVariable);

    if(!item) return;

    connect(item, &DownloadItem::DownloadResult, this, &WebEnginePage::SuggestResult);
}

#endif
