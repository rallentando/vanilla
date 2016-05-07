#include "switch.hpp"
#include "const.hpp"

//[[!WEV]]
#ifdef QTWEBKIT
//[[/!WEV]]

#include "webpagebase.hpp"
#include "page.hpp"

//[[WEV]]
#ifdef USE_WEBCHANNEL
#  include <QWebChannel>
#endif
//[[/WEV]]
//[[!WEV]]
#include <QWebHistoryBase>
#include <QWebElementBase>
//[[/!WEV]]

#include <QTimer>
#include <QNetworkReply>
#include <QAction>
#include <QPrinter>
#include <QPrintDialog>
#include <QClipboard>
#include <QTextCodec>
#include <QAuthenticator>
#include <QPushButton>
#include <QPair>
#include <QSet>
#include <QMenu>
#include <QCursor>
#include <QUrlQuery>
//[[!WEV]]
#include <QWebInspector>
//[[/!WEV]]
//[[WEV]]
#if QT_VERSION >= 0x050600
#  include <QWebEngineFullScreenRequest>
#endif
//[[/WEV]]

#include <functional>
#include <memory>

#include "view.hpp"
//[[!WEV]]
#include "webview.hpp"
#include "graphicswebview.hpp"
#include "quickwebview.hpp"
//[[/!WEV]]
//[[WEV]]
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
//[[/WEV]]
#include "application.hpp"
#include "mainwindow.hpp"
#include "networkcontroller.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

WebPageBase::WebPageBase(NetworkAccessManager *nam, QObject *parent)
//[[WEV]]
    : QWebPageBase(nam->GetProfile(), parent)
//[[/WEV]]
//[[!WEV]]
    : QWebPageBase(parent)
//[[/!WEV]]
{
    setNetworkAccessManager(nam);

    connect(this,   SIGNAL(ViewChanged()),
            parent, SIGNAL(ViewChanged()));
    connect(this,   SIGNAL(ScrollChanged(QPointF)),
            parent, SIGNAL(ScrollChanged(QPointF)));

    // these are self defined signals.
    connect(this,   SIGNAL(urlChanged(const QUrl&)),
            parent, SIGNAL(urlChanged(const QUrl&)));
    connect(this,   SIGNAL(titleChanged(const QString&)),
            parent, SIGNAL(titleChanged(const QString&)));
    connect(this,   SIGNAL(statusBarMessage2(const QString&, const QString&)),
            parent, SIGNAL(statusBarMessage2(const QString&, const QString&)));
    //[[WEV]]
#if QT_VERSION >= 0x050700
    connect(this,   SIGNAL(iconChanged(const QIcon&)),
            parent, SIGNAL(iconChanged(const QIcon&)));
#endif
    //[[/WEV]]

    // needless, because it's connected yet.
    //connect(this,   SIGNAL(loadStarted()),
    //        parent, SIGNAL(loadStarted()));
    //connect(this,   SIGNAL(loadProgress(int)),
    //        parent, SIGNAL(loadProgress(int)));
    //connect(this,   SIGNAL(loadFinished(bool)),
    //        parent, SIGNAL(loadFinished(bool)));
    //connect(this,   SIGNAL(statusBarMessage(const QString&)),
    //        parent, SIGNAL(statusBarMessage(const QString&)));
    //connect(this,   SIGNAL(linkClicked(QUrl)),
    //        parent, SIGNAL(linkClicked(QUrl)));
    //connect(this,   SIGNAL(selectionChanged()),
    //        parent, SIGNAL(selectionChanged()));

    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(CloseLater()));

    //[[!WEV]]
    connect(this, SIGNAL(downloadRequested(QNetworkRequest)),
            this, SLOT(Download(QNetworkRequest)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(HandleUnsupportedContent(QNetworkReply*)));
    connect(mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
            this, SLOT(AddJsObject()));
    //[[/!WEV]]

    //[[WEV]]
    connect(this, SIGNAL(linkHovered(const QString&)),
            this, SLOT(OnLinkHovered(const QString&)));
    connect(this, SIGNAL(featurePermissionRequested(const QUrl&, QWebEnginePage::Feature)),
            this, SLOT(HandleFeaturePermission(const QUrl&, QWebEnginePage::Feature)));
    connect(this, SIGNAL(authenticationRequired(const QUrl&, QAuthenticator*)),
            this, SLOT(HandleAuthentication(const QUrl&, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QUrl&, QAuthenticator*, const QString&)),
            this, SLOT(HandleProxyAuthentication(const QUrl&, QAuthenticator*, const QString&)));
#if QT_VERSION >= 0x050600
    connect(this, SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
            this, SLOT(HandleFullScreen(QWebEngineFullScreenRequest)));
    connect(this, SIGNAL(renderProcessTerminated(RenderProcessTerminationStatus, int)),
            this, SLOT(HandleProcessTermination(RenderProcessTerminationStatus, int)));
#endif
    //[[/WEV]]

    // instead of this.
    //m_View = (View *)parent;
    if(parent){
        //[[!WEV]]
        if(WebView *w = qobject_cast<WebView*>(parent))
            m_View = w;
        else if(GraphicsWebView *w = qobject_cast<GraphicsWebView*>(parent))
            m_View = w;
        else if(QuickWebView *w = qobject_cast<QuickWebView*>(parent))
            m_View = w;
        //[[/!WEV]]
        //[[WEV]]
        if(WebEngineView *w = qobject_cast<WebEngineView*>(parent))
            m_View = w;
        else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(parent))
            m_View = w;
        //[[/WEV]]
        else m_View = 0;
    } else m_View = 0;

    m_Page = new Page(this);
    m_Page->SetView(m_View);

    //[[WEV]]
#ifdef USE_WEBCHANNEL
    AddJsObject();
#endif
    m_ObscureDisplay = false;
    //[[/WEV]]
    //[[!WEV]]
    setForwardUnsupportedContent(true);
    //[[/!WEV]]
}

WebPageBase::~WebPageBase(){
    setNetworkAccessManager(0);
}

View* WebPageBase::GetView(){
    return m_View;
}

WebPageBase* WebPageBase::createWindow(WebWindowType type){
    Q_UNUSED(type);
    View *view = m_Page->OpenInNew(QUrl(QStringLiteral("about:blank")));

    //[[!WEV]]
    if(WebView *w = qobject_cast<WebView*>(view->base()))
        return w->page();
    else if(GraphicsWebView *w = qobject_cast<GraphicsWebView*>(view->base()))
        return w->page();
    //[[/!WEV]]
    //[[WEV]]
    if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
        return w->page();
    //[[/WEV]]
    return 0;
}

void WebPageBase::triggerAction(WebAction action, bool checked){
    QWebPageBase::triggerAction(action, checked);
}

//[[!WEV]]
bool WebPageBase::acceptNavigationRequest(QWebFrameBase *frame,
                                          const QNetworkRequest &req,
                                          NavigationType type){
    // is there any good way?
    return QWebPageBase::acceptNavigationRequest(frame, req, type);

    // QWebPageBase::NavigationTypeBackOrForward
    // QWebPageBase::NavigationTypeReload
    // QWebPageBase::NavigationTypeFormResubmitted
    // QWebPageBase::NavigationTypeFormSubmitted
    // QWebPageBase::NavigationTypeOther
    // :
    // default navigation

    // QWebPageBase::NavigationTypeLinkClicked
    // :
    // call 'OpenInNewHistNode' or 'OpenInNewViewNode' if 'EnableLoadHackLocal' is true;

    if(type == QWebPageBase::NavigationTypeBackOrForward ||
       type == QWebPageBase::NavigationTypeReload ||
       // cannot make a distinction of search engine or authentication.
       type == QWebPageBase::NavigationTypeFormSubmitted ||
       type == QWebPageBase::NavigationTypeFormResubmitted ||
       // cannot make a distinction of javascript or redirect.
       type == QWebPageBase::NavigationTypeOther ||
       !frame || frame->toHtml() == EMPTY_FRAME_HTML ||
       mainFrame()->toHtml() == EMPTY_FRAME_HTML){

        return QWebPageBase::acceptNavigationRequest(frame, req, type);
    }

    // this is not useful yet...
    // if request is search engine or javascript, want to make new node,
    // but cannot identify that.
    if(type == QWebPageBase::NavigationTypeLinkClicked){

        TreeBank *tb = m_View->GetTreeBank();
        ViewNode *vn = m_View->GetViewNode();
        HistNode *hn = m_View->GetHistNode();

        if(Page::ShiftMod()){

            return !tb->OpenInNewViewNode(req, Page::Activate(), vn);

        } else if(m_View->EnableLoadHackLocal() && Page::Activate()){

            return !tb->OpenInNewHistNode(req, true, hn);
        }
    }
    return QWebPageBase::acceptNavigationRequest(frame, req, type);
}

QObject* WebPageBase::createPlugin(const QString &id, const QUrl &url,
                                   const QStringList &names, const QStringList &values){
    return QWebPageBase::createPlugin(id, url, names, values);
}

QString WebPageBase::userAgentForUrl(const QUrl &url) const{
    QString ua =
        static_cast<NetworkAccessManager*>(networkAccessManager())->GetUserAgent();
    if(ua.isEmpty()) return QWebPageBase::userAgentForUrl(url);
    return ua;
}

QString WebPageBase::chooseFile(QWebFrameBase *parentframe, const QString &suggested){
    Q_UNUSED(parentframe);

    QString file = ModalDialog::GetOpenFileName_
        (QString::null,
         suggested.isEmpty() ?
         Application::GetUploadDirectory() :
         suggested);

    if(file.isEmpty() || !file.contains(QStringLiteral("/"))) return file;
    QStringList path = file.split(QStringLiteral("/"));
    path.removeLast();
    Application::SetUploadDirectory(path.join(QStringLiteral("/")));
    Application::AppendChosenFile(file);
    return file;
}

bool WebPageBase::extension(Extension extension,
                            const ExtensionOption *option,
                            ExtensionReturn *output){

    if(extension == ChooseMultipleFilesExtension){

        QStringList suggestedFiles =
            static_cast<const ChooseMultipleFilesExtensionOption*>(option)->suggestedFileNames;

        if(suggestedFiles.isEmpty() || suggestedFiles.first().isEmpty()){
            suggestedFiles = QStringList() << Application::GetUploadDirectory();
        }

        QStringList files = ModalDialog::GetOpenFileNames(QString::null, suggestedFiles.first());

        static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = files;

        if(!files.isEmpty() && !files.first().isEmpty()){
            QString file = files.first();
            if(!file.contains(QStringLiteral("/"))) return true;
            QStringList path = file.split(QStringLiteral("/"));
            path.removeLast();
            Application::SetUploadDirectory(path.join(QStringLiteral("/")));
            foreach(QString file, files){
                Application::AppendChosenFile(file);
            }
        }
        return true;
    }
    if(extension == ErrorPageExtension){
        return QWebPageBase::extension(extension, option, output);
    }
    return QWebPageBase::extension(extension, option, output);
}

bool WebPageBase::supportsExtension(Extension extension) const{
    if(extension == ChooseMultipleFilesExtension)
        return true;
    if(extension == ErrorPageExtension)
        return false;
    return QWebPageBase::supportsExtension(extension);
}
//[[/!WEV]]
//[[WEV]]
bool WebPageBase::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame){
    return QWebPageBase::acceptNavigationRequest(url, type, isMainFrame);
}

QStringList WebPageBase::chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                                     const QStringList &acceptedMimeTypes){
    Q_UNUSED(acceptedMimeTypes);

    QStringList suggestedFiles = oldFiles;

    if(suggestedFiles.isEmpty() || suggestedFiles.first().isEmpty()){
        suggestedFiles = QStringList() << Application::GetUploadDirectory();
    }

    QStringList files;

    if(mode == QWebEnginePage::FileSelectOpenMultiple){

        files = ModalDialog::GetOpenFileNames(QString::null, suggestedFiles.first());

    } else if(mode == QWebEnginePage::FileSelectOpen){

        files << ModalDialog::GetOpenFileName_(QString::null, suggestedFiles.first());
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

bool WebPageBase::certificateError(const QWebEngineCertificateError& error){
    QString errorTypeString;
    switch(error.error()){
    case QWebEngineCertificateError::SslPinnedKeyNotInCertificateChain:
        errorTypeString = QStringLiteral("SslPinnedKeyNotInCertificateChain");  break;
    case QWebEngineCertificateError::CertificateCommonNameInvalid:
        errorTypeString = QStringLiteral("CertificateCommonNameInvalid");       break;
    case QWebEngineCertificateError::CertificateDateInvalid:
        errorTypeString = QStringLiteral("CertificateDateInvalid");             break;
    case QWebEngineCertificateError::CertificateAuthorityInvalid:
        errorTypeString = QStringLiteral("CertificateAuthorityInvalid");        break;
    case QWebEngineCertificateError::CertificateContainsErrors:
        errorTypeString = QStringLiteral("CertificateContainsErrors");          break;
    case QWebEngineCertificateError::CertificateNoRevocationMechanism:
        errorTypeString = QStringLiteral("CertificateNoRevocationMechanism");   break;
    case QWebEngineCertificateError::CertificateUnableToCheckRevocation:
        errorTypeString = QStringLiteral("CertificateUnableToCheckRevocation"); break;
    case QWebEngineCertificateError::CertificateRevoked:
        errorTypeString = QStringLiteral("CertificateRevoked");                 break;
    case QWebEngineCertificateError::CertificateInvalid:
        errorTypeString = QStringLiteral("CertificateInvalid");                 break;
    case QWebEngineCertificateError::CertificateWeakSignatureAlgorithm:
        errorTypeString = QStringLiteral("CertificateWeakSignatureAlgorithm");  break;
    case QWebEngineCertificateError::CertificateNonUniqueName:
        errorTypeString = QStringLiteral("CertificateNonUniqueName");           break;
    case QWebEngineCertificateError::CertificateWeakKey:
        errorTypeString = QStringLiteral("CertificateWeakKey");                 break;
    case QWebEngineCertificateError::CertificateNameConstraintViolation:
        errorTypeString = QStringLiteral("CertificateNameConstraintViolation"); break;
    default: return false;
    }

    Application::AskSslErrorPolicyIfNeed();

    // false : block, true : allow.

    switch(Application::GetSslErrorPolicy()){
    case Application::BlockAccess:
        return false;
    case Application::IgnoreSslErrors:
        return true;
    case Application::AskForEachAccess:{

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Certificate error."));
        dialog->SetCaption(tr("Certificate error."));
        dialog->SetInformativeText(tr("Ignore this error?"));
        dialog->SetDetailedText
            (tr("Url: ") + error.url().toString() + QStringLiteral("\n") +
             tr("Type: ") + errorTypeString + QStringLiteral("\n") +
             error.errorDescription());
        dialog->SetButtons(QStringList() << tr("Allow") << tr("Block"));
        if(dialog->Execute() && dialog->ClickedButton() == tr("Allow"))
            return true;
        break;
    }
    case Application::AskForEachHost:
    case Application::AskForEachCertificate:{

        QString host = error.url().host();

        if(Application::GetBlockedHosts().contains(host)){
            return false;
        } else if(Application::GetAllowedHosts().contains(host)){
            return true;
        } else {
            ModalDialog *dialog = new ModalDialog();
            dialog->SetTitle(tr("Certificate error on host:%1").arg(host));
            dialog->SetCaption(tr("Certificate error on host:%1").arg(host));
            dialog->SetInformativeText(tr("Allow or Block this host?"));
            dialog->SetDetailedText
                (tr("Url: ") + error.url().toString() + QStringLiteral("\n") +
                 tr("Type: ") + errorTypeString + QStringLiteral("\n") +
                 error.errorDescription());

            dialog->SetButtons(QStringList() << tr("Allow") << tr("Block") << tr("Cancel"));
            dialog->Execute();
            QString text = dialog->ClickedButton();
            if(text == tr("Allow")){
                Application::AppendToAllowedHosts(host);
                return true;
            } else if(text == tr("Block")){
                Application::AppendToBlockedHosts(host);
                return false;
            }
            break;
        }
    }
    }
    return QWebPageBase::certificateError(error);
}

QNetworkAccessManager *WebPageBase::networkAccessManager() const{
    return m_NetworkAccessManager;
}

void WebPageBase::setNetworkAccessManager(QNetworkAccessManager *nam){
    m_NetworkAccessManager = nam;
}

QString WebPageBase::userAgentForUrl(const QUrl &url) const{
    Q_UNUSED(url);
    return static_cast<NetworkAccessManager*>(networkAccessManager())->GetUserAgent();
}

bool WebEnginePage::ObscureDisplay(){
    return m_ObscureDisplay;
}
//[[/WEV]]

void WebPageBase::DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                                     QPoint localPos, QPoint globalPos){

    QMenu *menu = new QMenu(parent);
    menu->setToolTipsVisible(true);

    QUrl linkUrl  = elem ? elem->LinkUrl()  : QUrl();
    QUrl imageUrl = elem ? elem->ImageUrl() : QUrl();

    //[[!WEV]]
    updatePositionDependentActions(localPos);
    QMenu *standardMenu = createStandardContextMenu();
    if(standardMenu){
        foreach(QAction *action, standardMenu->actions()){
            if((action != Action(QWebPageBase::Copy)) &&
               (action != Action(QWebPageBase::OpenLink)) &&
               //(action != Action(QWebPageBase::OpenLinkInThisWindow)) &&
               //(action != Action(QWebPageBase::OpenLinkInNewTab)) &&
               (action != Action(QWebPageBase::OpenLinkInNewWindow)) &&
               (action != Action(QWebPageBase::DownloadLinkToDisk)) &&
               (action != Action(QWebPageBase::CopyLinkToClipboard)) &&
               (action != Action(QWebPageBase::OpenImageInNewWindow)) &&
               (action != Action(QWebPageBase::DownloadImageToDisk)) &&
               (action != Action(QWebPageBase::CopyImageToClipboard)) &&
               (action != Action(QWebPageBase::CopyImageUrlToClipboard)) &&
               (action != Action(QWebPageBase::InspectElement))
               ){
                menu->addAction(action);
            }
        }
        delete standardMenu;
    }
    //[[/!WEV]]
    //[[WEV]]
    // 'selectedText' only work for WebEngineView.
    if(linkUrl.isEmpty() && imageUrl.isEmpty() && selectedText().isEmpty()){
        static QIcon backIcon = QIcon(":/resources/menu/back.png");
        static QIcon forwardIcon = QIcon(":/resources/menu/forward.png");
        static QIcon rewindIcon = QIcon(":/resources/menu/rewind.png");
        static QIcon fastForwardIcon = QIcon(":/resources/menu/fastforward.png");
        static QIcon reloadIcon = QIcon(":/resources/menu/reload.png");
        static QIcon stopIcon = QIcon(":/resources/menu/stop.png");
        QAction *backAction    = Action(QWebPageBase::Back);
        QAction *forwardAction = Action(QWebPageBase::Forward);
        QAction *rewindAction  = Action(Page::_Rewind);
        QAction *fastForwardAction = Action(Page::_FastForward);
        QAction *reloadAction  = Action(QWebPageBase::Reload);
        QAction *stopAction    = Action(QWebPageBase::Stop);
        backAction->setText(tr("Back"));
        backAction->setIcon(backIcon);
        forwardAction->setText(tr("Forward"));
        forwardAction->setIcon(forwardIcon);
        rewindAction->setText(tr("Rewind"));
        rewindAction->setIcon(rewindIcon);
        fastForwardAction->setText(tr("FastForward"));
        fastForwardAction->setIcon(fastForwardIcon);
        reloadAction->setText(tr("Reload"));
        reloadAction->setIcon(reloadIcon);
        stopAction->setText(tr("Stop"));
        stopAction->setIcon(stopIcon);

        if(backAction->isEnabled())
            menu->addAction(backAction);
        if(forwardAction->isEnabled())
            menu->addAction(forwardAction);

        if(!View::EnableDestinationInferrer()){
            if(backAction->isEnabled())
                menu->addAction(rewindAction);
            menu->addAction(fastForwardAction);
        }

        if(stopAction->isEnabled())
            menu->addAction(stopAction);
        else
            menu->addAction(reloadAction);
    }
    //[[/WEV]]

    m_View->AddContextMenu(menu, elem);

    //[[!WEV]]
    if(settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled)){
        menu->addSeparator();
        menu->addAction(Action(Page::_InspectElement));
    }
    //[[/!WEV]]
    //[[WEV]]
#if QT_VERSION >= 0x050600
    menu->addSeparator();
    menu->addAction(Action(Page::_InspectElement));
#endif
    //[[/WEV]]

    if(elem && !elem->IsNull() && elem->IsQueryInputElement()){
        if(!menu->isEmpty()) menu->addSeparator();
        menu->addAction(Action(Page::_AddSearchEngine, localPos));
    }

    m_View->AddRegularMenu(menu, elem);

    menu->exec(globalPos);
    delete menu;
}

void WebPageBase::TriggerAction(QWebPageBase::WebAction action){
    Action(action)->trigger();
}

void WebPageBase::TriggerAction(Page::CustomAction action, QVariant data){
    Action(action, data)->trigger();
}

QAction *WebPageBase::Action(QWebPageBase::WebAction a){
    return action(a);
}

QAction *WebPageBase::Action(Page::CustomAction a, QVariant data){
    QAction *result = 0;
    switch(a){
    // set text manually.
    case Page::_Copy:          result = action(Copy);                    result->setText(tr("Copy"));          return result;
    case Page::_Cut:           result = action(Cut);                     result->setText(tr("Cut"));           return result;
    case Page::_Paste:         result = action(Paste);                   result->setText(tr("Paste"));         return result;
    case Page::_Undo:          result = action(Undo);                    result->setText(tr("Undo"));          return result;
    case Page::_Redo:          result = action(Redo);                    result->setText(tr("Redo"));          return result;
    case Page::_SelectAll:     result = action(SelectAll);               result->setText(tr("SelectAll"));     return result;
    case Page::_Reload:        result = action(Reload);                  result->setText(tr("Reload"));        return result;
    case Page::_Stop:          result = action(Stop);                    result->setText(tr("Stop"));          return result;

    //[[!WEV]]

  //case Page::_LoadLink:      result = action(OpenLink);                result->setText(tr("LoadLink"));      return result;
    case Page::_OpenLink:      result = action(OpenLinkInNewWindow);     result->setText(tr("OpenLink"));      return result;
  //case Page::_DownloadLink:  result = action(DownloadLinkToDisk);      result->setText(tr("DownloadLink"));  return result;
    case Page::_CopyLinkUrl:   result = action(CopyLinkToClipboard);     result->setText(tr("CopyLinkUrl"));   return result;

    case Page::_OpenImage:     result = action(OpenImageInNewWindow);    result->setText(tr("OpenImage"));     return result;
  //case Page::_DownloadImage: result = action(DownloadImageToDisk);     result->setText(tr("DownloadImage")); return result;
    case Page::_CopyImage:     result = action(CopyImageToClipboard);    result->setText(tr("CopyImage"));     return result;
  //case Page::_CopyImageUrl:  result = action(CopyImageUrlToClipboard); result->setText(tr("CopyImageUrl"));  return result;

    //[[/!WEV]]
    }
    return m_Page->Action(a, data);
}

//[[WEV]]
void WebPageBase::OnLinkHovered(const QString &url){
    emit linkHovered(url, QString(), QString());
}

void WebPageBase::HandleFeaturePermission(const QUrl &securityOrigin,
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
    default: return;
    }

    ModalDialog *dialog = new ModalDialog();
    dialog->SetTitle(tr("Feature Permission Requested."));
    dialog->SetCaption(tr("Feature Permission Requested."));
    dialog->SetInformativeText
        (tr("Url: ") + securityOrigin.toString() + QStringLiteral("\n") +
         tr("Feature: ") + featureString + QStringLiteral("\n\n") +
         tr("Allow this feature?"));
    dialog->SetButtons(QStringList() << tr("Yes") << tr("No") << tr("Cancel"));
    dialog->Execute();

    QString text = dialog->ClickedButton();
    if(text == tr("Yes")){
        setFeaturePermission(securityOrigin, feature,
                             QWebEnginePage::PermissionGrantedByUser);
    } else if(text == tr("No")){
        setFeaturePermission(securityOrigin, feature,
                             QWebEnginePage::PermissionDeniedByUser);
    } else if(text == tr("Cancel")){
        emit featurePermissionRequestCanceled(securityOrigin, feature);
    }
}

void WebPageBase::HandleAuthentication(const QUrl &requestUrl,
                                       QAuthenticator *authenticator){
    Q_UNUSED(requestUrl);

    ModalDialog::Authentication(authenticator);
}

void WebPageBase::HandleProxyAuthentication(const QUrl &requestUrl,
                                            QAuthenticator *authenticator,
                                            const QString &proxyHost){
    Q_UNUSED(requestUrl);
    Q_UNUSED(proxyHost);

    ModalDialog::Authentication(authenticator);
}

#if QT_VERSION >= 0x050600
void WebPageBase::HandleFullScreen(QWebEngineFullScreenRequest request){
    if(TreeBank *tb = m_View->GetTreeBank()){
        bool fullScreen = request.toggleOn();
        tb->GetMainWindow()->SetFullScreen(fullScreen);
        m_ObscureDisplay = fullScreen;
        if(fullScreen){
            ModelessDialog *dialog = new ModelessDialog();
            connect(this, &WebPageBase::destroyed, dialog, &ModelessDialog::Aborted);
            connect(this, &WebPageBase::fullScreenRequested, dialog, &ModelessDialog::Aborted);
            dialog->SetTitle(tr("This page becomes full screen mode."));
            dialog->SetCaption(tr("Press Esc to exit."));
            dialog->SetButtons(QStringList() << tr("OK") << tr("Cancel"));
            dialog->SetDefaultValue(true);
            dialog->SetCallBack([this](bool ok){ if(!ok) triggerAction(ExitFullScreen);});
            QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
        }
        request.accept();
    } else {
        request.reject();
    }
}

void WebPageBase::HandleProcessTermination(RenderProcessTerminationStatus status, int code){
    QString info = tr("A page is reloaded, because that's process is terminated.\n");
    switch(status){
    case NormalTerminationStatus:
        info += tr("Normal termination. (code: %1)");   break;
    case AbnormalTerminationStatus:
        info += tr("Abnormal termination. (code: %1)"); break;
    case CrashedTerminationStatus:
        info += tr("Crashed termination. (code: %1)");  break;
    case KilledTerminationStatus:
        info += tr("Killed termination. (code: %1)");   break;
    }
    ModelessDialog::Information(tr("Render process terminated."),
                                info.arg(code), m_View->base());
    QTimer::singleShot(0, m_Page, SLOT(Reload()));
}
#endif //if QT_VERSION >= 0x050600
//[[/WEV]]

void WebPageBase::HandleUnsupportedContent(QNetworkReply *reply){
    //[[WEV]]
    Q_UNUSED(reply);
    //[[/WEV]]
    //[[!WEV]]

    if(!reply) return;
    QUrl url = reply->url();
    if(url.isEmpty()) return;
    if(url.scheme() == QStringLiteral("abp")) return;

    switch (reply->error()){
    case QNetworkReply::NoError:{
        if(reply->header(QNetworkRequest::ContentTypeHeader).isValid()){

            // Donload and jump to viewable page if need.

            if(DownloadItem *item = NetworkController::Download(reply)){
                if(m_View->GetTreeBank() && m_View->GetTreeBank()->GetNotifier())
                    m_View->GetTreeBank()->GetNotifier()->RegisterDownload(item);
            }

            if(mainFrame()->toHtml().isEmpty() ||
               mainFrame()->toHtml() == EMPTY_FRAME_HTML){

                // go back or remove needless 'ViewNode' or 'HistNode'.

                HistNode *hist = m_View->GetHistNode();

                if(history()->canGoBack())
                    QWebPageBase::triggerAction(QWebPageBase::Back);
                else if(hist->IsRoot())
                    CloseLater();
                else
                    m_View->GetTreeBank()->DeleteNode(hist);
            }
            return;
        }
        break;
    }
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::SslHandshakeFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentReSendError:
    case QNetworkReply::ProtocolUnknownError:
    case QNetworkReply::ProtocolInvalidOperationError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::UnknownProxyError:
    case QNetworkReply::UnknownContentError:
    case QNetworkReply::ProtocolFailure:
        return;
    }

    QWebFrameBase *frame = mainFrame();
    if(!frame) return;
    if(reply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()){
        QByteArray data = reply->readAll().toLower();
        if(// for html or xhtml ~version 4.
           data.contains("<!doctype") ||
           data.contains("<script")   ||
           data.contains("<html")     ||
           data.contains("<html")     ||
           data.contains("<head")     ||
           data.contains("<iframe")   ||
           data.contains("<h1")       ||
           data.contains("<div")      ||
           data.contains("<font")     ||
           data.contains("<table")    ||
           data.contains("<a")        ||
           data.contains("<style")    ||
           data.contains("<title")    ||
           data.contains("<b")        ||
           data.contains("<body")     ||
           data.contains("<br")       ||
           data.contains("<p")        ||
           // for xhtml version 5,
           //ignore tag if its length is less than 4.
           data.contains(":script")   ||
           data.contains(":html")     ||
           data.contains(":head")     ||
           data.contains(":iframe")   ||
           data.contains(":font")     ||
           data.contains(":table")    ||
           data.contains(":style")    ||
           data.contains(":title")    ||
           data.contains(":body")){

            frame->setHtml(QLatin1String(data), url);
            return;
        }
    }
    /*not found*/
    //[[/!WEV]]
}

void WebPageBase::AddJsObject(){
    //[[WEV]]
#ifdef USE_WEBCHANNEL
    if(!webChannel())
        setWebChannel(new QWebChannel(this));

    if(m_View && m_View->GetJsObject()){
        webChannel()->registerObject(QStringLiteral("_view"), m_View->GetJsObject());
        m_View->OnSetJsObject(m_View->GetJsObject());
    }
    if(m_View && m_View->GetTreeBank() && m_View->GetTreeBank()->GetJsObject()){
        webChannel()->registerObject(QStringLiteral("_vanilla"), m_View->GetTreeBank()->GetJsObject());
        m_View->OnSetJsObject(m_View->GetTreeBank()->GetJsObject());
    }
#endif
    //[[/WEV]]
    //[[!WEV]]
    if(!mainFrame()) return;

    if(m_View && m_View->GetJsObject()){
        mainFrame()->addToJavaScriptWindowObject(QStringLiteral("_view"), m_View->GetJsObject());
        m_View->OnSetJsObject(m_View->GetJsObject());
    }
    if(m_View && m_View->GetTreeBank() && m_View->GetTreeBank()->GetJsObject()){
        mainFrame()->addToJavaScriptWindowObject(QStringLiteral("_vanilla"), m_View->GetTreeBank()->GetJsObject());
        m_View->OnSetJsObject(m_View->GetTreeBank()->GetJsObject());
    }
    //[[/!WEV]]
}

////////////////////////////////////////////////////////////////////////////////

void WebPageBase::Print(){
    //[[!WEV]]
    QPrinter printer;
    QPrintDialog dialog(&printer, Application::CurrentWidget());
    if(dialog.exec() == QDialog::Accepted){
        mainFrame()->print(&printer);
    }
    //[[/!WEV]]
}

void WebPageBase::AddSearchEngine(QPoint pos){
    //[[!WEV]]
    QWebHitTestResult r = mainFrame()->hitTestContent(pos);
    QWebElementBase elem = r.element();
    QString name = elem.attribute(QStringLiteral("name"));
    QWebElementBase form = elem;
    while(!form.isNull() && form.tagName().toLower() != QStringLiteral("form")){
        form = form.parent();
    }
    if(form.isNull()) return;

    QString encode = form.attribute(QStringLiteral("accept-charset"), QStringLiteral("UTF-8"));
    QString method = form.attribute(QStringLiteral("method"), QStringLiteral("get"));
    if(method.toLower() != QStringLiteral("get")) return;

    QUrl result = Page::StringToUrl(form.attribute(QStringLiteral("action")), currentFrame()->baseUrl());
    QUrlQuery queries = QUrlQuery(result);
    QMap<QString, QString> engines;

    QWebElementBaseCollection fields = form.findAll(QStringLiteral("input"));
    foreach(QWebElementBase field, fields){
        QString type = field.attribute(QStringLiteral("type"), QStringLiteral("text")).toLower();
        type = QString::fromLatin1(QUrl::toPercentEncoding(type));
        QString name = field.attribute(QStringLiteral("name"));
        name = QString::fromLatin1(QUrl::toPercentEncoding(name));
        QString val = field.evaluateJavaScript(QStringLiteral("this.value")).toString();
        val = QString::fromLatin1(QUrl::toPercentEncoding(val));
        if(type == QStringLiteral("submit")){
            engines[name] = val; continue;
        } else if(type == QStringLiteral("text") || type == QStringLiteral("search")){
            if(field == elem) val = QStringLiteral("{query}");
        } else if(type == QStringLiteral("checkbox") || type == QStringLiteral("radio")){
            if(!field.evaluateJavaScript(QStringLiteral("this.checked")).toBool()) continue;
        } else if(type != QStringLiteral("hidden")) continue;

        queries.addQueryItem(name, val);
    }

    QWebElementBaseCollection buttons = form.findAll(QStringLiteral("button"));
    foreach(QWebElementBase button, buttons){
        QString name = button.attribute(QStringLiteral("name"));
        name = QString::fromLatin1(QUrl::toPercentEncoding(name));
        QString val = button.attribute(QStringLiteral("aria-label"));
        val = QString::fromLatin1(QUrl::toPercentEncoding(val));
        engines[name] = val;
    }

    QWebElementBaseCollection selects = form.findAll(QStringLiteral("select"));
    foreach(QWebElementBase select, selects){
        int index = select.evaluateJavaScript(QStringLiteral("this.selectedIndex")).toInt();

        if(index != -1){
            QWebElementBaseCollection options = select.findAll(QStringLiteral("option"));
            QString name = select.attribute(QStringLiteral("name"));
            QString val = options[index].toPlainText();
            queries.addQueryItem(name, val);
        }
    }

    QString tag;
    QWebElementBaseCollection labels =
        form.findAll(QStringLiteral("label[for=\"%1\"]").arg(name));
    if(labels.count() > 0) tag = labels[0].toPlainText();
    //[[/!WEV]]
    //[[WEV]]
    m_View->CallWithGotCurrentBaseUrl([this, pos](QUrl base){

    runJavaScript(QStringLiteral(
  "(function(){\n"
VV"   var x = %1;\n"
VV"   var y = %2;\n"
VV"   var elem = document.elementFromPoint(x, y);\n"
VV"   var name = elem.name;\n"
VV"   var form = elem;\n"
VV"   while(form && form.tagName != \"FORM\"){\n"
VV"       form = form.parentNode;\n"
VV"   }\n"
VV"   if(!form) return {};\n"
VV"   var encode = form.getAttribute(\"accept-charset\") || \"UTF-8\";\n"
VV"   var method = form.method || \"get\";\n"
VV"   if(method.toLowerCase() != \"get\") return {};\n"
VV"   var result = {};\n"
VV"   var queries = {};\n"
VV"   var engines = {};\n"
VV"   var inputs = form.getElementsByTagName(\"input\");\n"
VV"   var buttons = form.getElementsByTagName(\"button\");\n"
VV"   var selects = form.getElementsByTagName(\"select\");\n"
VV"   for(var i = 0; i < inputs.length; i++){\n"
VV"       var field = inputs[i];\n"
VV"       var type = (field.type || \"text\").toLowerCase();\n"
VV"       var name = field.name;\n"
VV"       var val = field.value;\n"
VV"       if(type == \"submit\"){\n"
VV"           engines[name] = val; continue;\n"
VV"       } else if(type == \"text\" || type == \"search\"){\n"
VV"           if(field == elem) val = \"{query}\";\n"
VV"       } else if(type == \"checkbox\" || type == \"radio\"){\n"
VV"           if(!field.checked) continue;\n"
VV"       } else if(type != \"hidden\") continue;\n"
VV"       queries[name] = val;\n"
VV"   }\n"
VV"   for(var i = 0; i < buttons.length; i++){\n"
VV"       var button = buttons[i];\n"
VV"       engines[button.name] = button.getAttribute(\"aria-label\");\n"
VV"   }\n"
VV"   for(var i = 0; i < selects.length; i++){\n"
VV"       var select = selects[i];\n"
VV"       var index = select.selectedIndex;\n"
VV"       if(index != -1){\n"
VV"           var options = select.getElementsByTagName(\"option\");\n"
VV"           queries[select.name] = options[index].textContent;\n"
VV"       }\n"
VV"   }\n"
VV"   var labels = form.querySelectorAll(\"label[for=\\\"\"+name+\"\\\"]\");\n"
VV"   var tag = labels.length ? labels[0].innerText : \"\";\n"
VV"   result[0] = encode;\n"
VV"   result[1] = method;\n"
VV"   result[2] = form.action;\n"
VV"   result[3] = queries;\n"
VV"   result[4] = engines;\n"
VV"   result[5] = tag;\n"
VV"   return result;\n"
VV"})()").arg(pos.x()).arg(pos.y()),
[this, pos, base](QVariant var){

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
    //[[/WEV]]

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
    //[[WEV]]
    });});
    //[[/WEV]]
}

void WebPageBase::AddBookmarklet(QPoint pos){
    //[[!WEV]]
    SharedWebElement elem = m_View->HitElement(pos);
    //[[/!WEV]]
    //[[WEV]]
    m_View->CallWithHitElement(pos, [this, pos](SharedWebElement elem){
    //[[/WEV]]

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

        //int count = ModalDialog::GetInt
        //    (tr("Multiple Decoding."),
        //     tr("How many times to Decode?"),
        //     2, 1, 10, 1, &ok);
        int count = 2;

        // too dirty...
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
    //[[WEV]]
    });
    //[[/WEV]]
}

void WebPageBase::InspectElement(){
    // my inspector doesn't catch element.
    //QWebInspector *inspector = new QWebInspector;
    //inspector->setPage(this);
    //inspector->show();
    //[[!WEV]]
    QWebPageBase::triggerAction(QWebPageBase::InspectElement);
    //[[/!WEV]]
    //[[WEV]]
#if QT_VERSION >= 0x050600
    QWebPageBase::triggerAction(QWebPageBase::InspectElement);
#endif
    //[[/WEV]]
}

void WebPageBase::ReloadAndBypassCache(){
    // 'action(ReloadAndBypassCache)' returns 0...
    QWebPageBase::triggerAction(QWebPageBase::ReloadAndBypassCache);
}

void WebPageBase::CloseLater(){
    QTimer::singleShot(0, m_Page, &Page::Close);
}

void WebPageBase::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    DownloadItem *item =
        NetworkController::Download(static_cast<NetworkAccessManager*>(networkAccessManager()),
                                    req, NetworkController::ToVariable);
    connect(item, &DownloadItem::DownloadResult, this, &WebPageBase::SuggestResult);
}

//[[!WEV]]
#endif //ifdef QTWEBKIT
//[[/!WEV]]
