#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>
#include <QCursor>
#include <QSslCertificate>

#include "dialog.hpp"
#include "certificatepolicy.hpp"
#include "page.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "mainwindow.hpp"
#include "application.hpp"
#include "networkcontroller.hpp"
#include "downloadname.hpp"
#include "settingspage.hpp"

void EdgeWebView::RemoveWebViewHandlers(){
    m_Impl->m_WebViewEvents.RevokeAll();
}

void EdgeWebView::RemoveControllerHandlers(){
    m_Impl->m_ControllerEvents.RevokeAll();
}

void EdgeWebView::RemoveCompositionHandlers(){

    m_Impl->m_CompositionEvents.RevokeAll();
    m_Impl->m_DragOut.Reset();

    s_DragOut.Cancel(m_Impl->m_Token);
}

namespace {

    QString CertificateErrorDescription(COREWEBVIEW2_WEB_ERROR_STATUS status){
        switch(status){
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT:
            return EdgeWebView::tr("The certificate is for another host.");
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED:
            return EdgeWebView::tr("The certificate has expired.");
        case COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS:
            return EdgeWebView::tr("The client certificate contains errors.");
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED:
            return EdgeWebView::tr("The certificate has been revoked.");
        case COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID:
            return EdgeWebView::tr("The certificate is not valid.");
        default:
            return EdgeWebView::tr("Certificate error %1").arg(static_cast<int>(status));
        }
    }

    QList<QSslCertificate> CertificateChainOf(ICoreWebView2Certificate *certificate){
        QList<QSslCertificate> chain;
        if(!certificate) return chain;

        LPWSTR pem = nullptr;
        if(SUCCEEDED(certificate->ToPemEncoding(&pem)) && pem){
            chain << QSslCertificate
                (QString::fromWCharArray(pem).toLatin1(), QSsl::Pem);
            CoTaskMemFree(pem);
        }

        ComPtr<ICoreWebView2StringCollection> issuers;
        if(SUCCEEDED(certificate->get_PemEncodedIssuerCertificateChain(&issuers)) &&
           issuers){
            UINT32 count = 0;
            issuers->get_Count(&count);
            for(UINT32 i = 0; i < count; i++){
                LPWSTR value = nullptr;
                if(FAILED(issuers->GetValueAtIndex(i, &value)) || !value) continue;
                chain << QSslCertificate
                    (QString::fromWCharArray(value).toLatin1(), QSsl::Pem);
                CoTaskMemFree(value);
            }
        }
        return chain;
    }

    QString PermissionName(COREWEBVIEW2_PERMISSION_KIND kind){
        switch(kind){
        case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:
            return EdgeWebView::tr("MediaAudioCapture");
        case COREWEBVIEW2_PERMISSION_KIND_CAMERA:
            return EdgeWebView::tr("MediaVideoCapture");
        case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:
            return EdgeWebView::tr("Geolocation");
        case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:
            return EdgeWebView::tr("Notifications");
        case COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS:
            return EdgeWebView::tr("OtherSensors");
        case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:
            return EdgeWebView::tr("ClipboardReadWrite");
        case COREWEBVIEW2_PERMISSION_KIND_MULTIPLE_AUTOMATIC_DOWNLOADS:
            return EdgeWebView::tr("MultipleAutomaticDownloads");
        case COREWEBVIEW2_PERMISSION_KIND_FILE_READ_WRITE:
            return EdgeWebView::tr("FileReadWrite");
        case COREWEBVIEW2_PERMISSION_KIND_AUTOPLAY:
            return EdgeWebView::tr("Autoplay");
        case COREWEBVIEW2_PERMISSION_KIND_LOCAL_FONTS:
            return EdgeWebView::tr("LocalFontsAccess");
        case COREWEBVIEW2_PERMISSION_KIND_MIDI_SYSTEM_EXCLUSIVE_MESSAGES:
            return EdgeWebView::tr("MidiSystemExclusiveMessages");
        case COREWEBVIEW2_PERMISSION_KIND_WINDOW_MANAGEMENT:
            return EdgeWebView::tr("WindowManagement");
        case COREWEBVIEW2_PERMISSION_KIND_PERSISTENT_STORAGE:
            return EdgeWebView::tr("PersistentStorage");
        default:
            return QString();
        }
    }

    QUrl ReportedSourceOf(ICoreWebView2 *sender){
        if(!sender) return QUrl();
        LPWSTR source = nullptr;
        if(FAILED(sender->get_Source(&source)) || !source) return QUrl();
        const QUrl reported = QUrl(QString::fromWCharArray(source));
        CoTaskMemFree(source);
        return reported;
    }
}

void EdgeWebView::AdoptReportedSource(const QUrl &reported){
    if(reported.isEmpty()) return;

    if(!EdgeIsOwnViewSource(m_Impl->m_Url, reported)) m_Impl->m_Url = reported;
    m_Impl->m_History.Visit(m_Impl->m_Url);
    SaveHistory();
    emit urlChanged(m_Impl->m_Url);
}

void EdgeWebView::RegisterNavigationHandlers(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()) return;

    ICoreWebView2 *webview = m_Impl->m_WebView.Get();
    EventRegistrationToken token = {};

    if(SUCCEEDED(webview->add_NavigationStarting
        (Callback<ICoreWebView2NavigationStartingEventHandler>
         ([this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             m_Impl->m_PageScroll = QPointF();
             m_Impl->m_PageContents = QSizeF();
             m_Impl->m_PageViewport = QSizeF();
             m_Impl->m_Document.NavigationStarted();
             emit loadStarted();
             emit loadProgress(0);
             emit statusBarMessage(tr("Started loading."));
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, NavigationStarting, token);
    VANILLA_STOP_IF_RETIRED();

    if(SUCCEEDED(webview->add_ContentLoading
        (Callback<ICoreWebView2ContentLoadingEventHandler>
         ([this](ICoreWebView2 *sender, ICoreWebView2ContentLoadingEventArgs *args) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             m_Impl->m_Document.DocumentArrived();
             m_Impl->m_HandledKeys.clear();

             const QUrl reported = ReportedSourceOf(sender);
             if(!reported.isEmpty() && reported != m_Impl->m_Url &&
                !EdgeIsOwnViewSource(m_Impl->m_Url, reported))
                 AdoptReportedSource(reported);
             emit loadProgress(50);
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, ContentLoading, token);
    VANILLA_STOP_IF_RETIRED();

    if(SUCCEEDED(webview->add_NavigationCompleted
        (Callback<ICoreWebView2NavigationCompletedEventHandler>
         ([this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             m_Impl->m_History.Release();

             BOOL success = FALSE;
             if(args) args->get_IsSuccess(&success);
             emit loadProgress(100);
             emit loadFinished(success ? true : false);
             emit ViewChanged();

             PullCookiesIntoJar();

             if(success) RestoreStateAfterLoad();
             emit statusBarMessage(success ? tr("Finished loading.") : tr("Failed to load."));

             if(visible() && m_TreeBank &&
                m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible())
                 UpdateThumbnail();
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, NavigationCompleted, token);
    VANILLA_STOP_IF_RETIRED();

    if(SUCCEEDED(webview->add_DocumentTitleChanged
        (Callback<ICoreWebView2DocumentTitleChangedEventHandler>
         ([this](ICoreWebView2 *sender, IUnknown*) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             LPWSTR title = nullptr;
             if(sender && SUCCEEDED(sender->get_DocumentTitle(&title)) && title){
                 m_Impl->m_Title = QString::fromWCharArray(title);
                 CoTaskMemFree(title);
                 emit titleChanged(m_Impl->m_Title);
             }
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, DocumentTitleChanged, token);
    VANILLA_STOP_IF_RETIRED();

    if(SUCCEEDED(webview->add_SourceChanged
        (Callback<ICoreWebView2SourceChangedEventHandler>
         ([this](ICoreWebView2 *sender, ICoreWebView2SourceChangedEventArgs*) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             AdoptReportedSource(ReportedSourceOf(sender));
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, SourceChanged, token);
}

void EdgeWebView::RegisterDocumentHandlers(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()) return;

    ICoreWebView2 *webview = m_Impl->m_WebView.Get();
    EventRegistrationToken token = {};

    if(SUCCEEDED(webview->add_NewWindowRequested
        (Callback<ICoreWebView2NewWindowRequestedEventHandler>
         ([this](ICoreWebView2*,
                 ICoreWebView2NewWindowRequestedEventArgs *args) -> HRESULT {
             if(!args) return S_OK;
             args->put_Handled(TRUE);

             if(m_Impl->m_State.IsRetired()) return S_OK;

             LPWSTR uri = nullptr;
             if(FAILED(args->get_Uri(&uri)) || !uri) return S_OK;
             const QUrl url = QUrl(QString::fromWCharArray(uri));
             CoTaskMemFree(uri);
             if(url.isEmpty()) return S_OK;

             if(m_Impl->m_DropEcho.Claim(url.adjusted(QUrl::StripTrailingSlash),
                                 m_Impl->m_DropEchoClock.elapsed()))
                 return S_OK;

             QPointer<EdgeWebView> alive(this);
             QTimer::singleShot(0, this, [alive, url](){
                 if(!alive || !alive->m_TreeBank) return;
                 alive->m_TreeBank->OpenInNewViewNode
                     (url, Page::Activate(), alive->GetViewNode());
             });
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, NewWindowRequested, token);
    VANILLA_STOP_IF_RETIRED();

    ComPtr<ICoreWebView2_4> webview4;
    if(FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview4)))) webview4.Reset();
    VANILLA_STOP_IF_RETIRED();

    if(webview4 &&
       SUCCEEDED(webview4->add_DownloadStarting
        (Callback<ICoreWebView2DownloadStartingEventHandler>
         ([this](ICoreWebView2*,
                 ICoreWebView2DownloadStartingEventArgs *args) -> HRESULT {
             if(!args) return S_OK;

             args->put_Handled(TRUE);

             if(m_Impl->m_State.IsRetired()) return S_OK;

             ComPtr<ICoreWebView2DownloadOperation> operation;
             if(FAILED(args->get_DownloadOperation(&operation)) || !operation)
                 return S_OK;

             LPWSTR path = nullptr;
             QString name;
             if(SUCCEEDED(args->get_ResultFilePath(&path)) && path){
                 name = QFileInfo(QString::fromWCharArray(path)).fileName();
                 CoTaskMemFree(path);
             }
             if(name.isEmpty()) return S_OK;

             const QString target = DownloadName::Unique
                 (QDir::cleanPath
                  (QDir(Application::GetDownloadDirectory())
                   .filePath(DownloadName::Sanitize(name))));

             if(!target.isEmpty()){
                 const QString native = QDir::toNativeSeparators(target);

                 if(SUCCEEDED(args->put_ResultFilePath
                              (reinterpret_cast<PCWSTR>(native.utf16()))))
                     name = QFileInfo(native).fileName();
             }

             emit statusBarMessage(tr("Downloading %1").arg(name));

             const bool downloadOnly = m_Impl->m_History.IsEmpty();
             if(downloadOnly) m_Impl->m_Downloads.Started();

             auto settled = std::make_shared<bool>(false);

             QPointer<EdgeWebView> alive(this);
             EventRegistrationToken stateToken = {};
             const HRESULT hr = operation->add_StateChanged
                 (Callback<ICoreWebView2StateChangedEventHandler>
                  ([alive, name, downloadOnly, settled]
                   (ICoreWebView2DownloadOperation *sender, IUnknown*) -> HRESULT {
                      if(!alive || !sender) return S_OK;
                      COREWEBVIEW2_DOWNLOAD_STATE state =
                          COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS;
                      sender->get_State(&state);
                      if(state == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED)
                          emit alive->statusBarMessage(tr("Downloaded %1").arg(name));
                      else if(state == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED)
                          emit alive->statusBarMessage(tr("Download of %1 was interrupted").arg(name));
                      else
                          return S_OK;

                      if(!downloadOnly || *settled) return S_OK;
                      *settled = true;
                      alive->m_Impl->m_Downloads.Settled();

                      if(!alive->m_Impl->m_Downloads.MayClose() ||
                         !alive->m_Impl->m_History.IsEmpty()) return S_OK;

                      QPointer<EdgeWebView> waiting(alive);
                      View::CloseLater(alive->GetThis(), [waiting](){
                          return waiting && !waiting->m_Impl->m_State.IsRetired() &&
                              waiting->m_Impl->m_Downloads.MayClose() &&
                              waiting->m_Impl->m_History.IsEmpty();
                      });
                      return S_OK;
                  }).Get(), &stateToken);

             if(FAILED(hr) && downloadOnly){
                 qWarning() << "edge: a download cannot be watched;"
                            << "this tab will not close itself";
                 alive->m_Impl->m_Downloads.Untracked();
             }

             return S_OK;
         }).Get(), &token))){
        VANILLA_KEEP_WEBVIEW_EVENT(webview4.Get(), DownloadStarting, token);
    }
    VANILLA_STOP_IF_RETIRED();

    ComPtr<ICoreWebView2_8> webview8;
    if(FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview8)))) webview8.Reset();
    VANILLA_STOP_IF_RETIRED();

    if(webview8 &&
       SUCCEEDED(webview8->add_IsDocumentPlayingAudioChanged
        (Callback<ICoreWebView2IsDocumentPlayingAudioChangedEventHandler>
         ([this](ICoreWebView2 *sender, IUnknown*) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             ComPtr<ICoreWebView2_8> playing8;
             BOOL playing = FALSE;
             if(!sender ||
                FAILED(sender->QueryInterface(IID_PPV_ARGS(&playing8))) || !playing8 ||
                FAILED(playing8->get_IsDocumentPlayingAudio(&playing)))
                 return S_OK;

             HandleAudioStateChanged(playing ? true : false);
             return S_OK;
         }).Get(), &token))){
        VANILLA_KEEP_WEBVIEW_EVENT
            (webview8.Get(), IsDocumentPlayingAudioChanged, token);
    }
    VANILLA_STOP_IF_RETIRED();


    if(SUCCEEDED(webview->add_ContainsFullScreenElementChanged
        (Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>
         ([this](ICoreWebView2 *sender, IUnknown*) -> HRESULT {
             if(m_Impl->m_State.IsRetired()) return S_OK;
             BOOL full = FALSE;
             if(!sender || FAILED(sender->get_ContainsFullScreenElement(&full)))
                 return S_OK;
             HandleFullScreen(full ? true : false);
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT
            (webview, ContainsFullScreenElementChanged, token);
    VANILLA_STOP_IF_RETIRED();

    ComPtr<ICoreWebView2_11> webview11;
    if(SUCCEEDED(webview->QueryInterface(IID_PPV_ARGS(&webview11))) && webview11){
        VANILLA_STOP_IF_RETIRED();
        if(SUCCEEDED(webview11->add_ContextMenuRequested
            (Callback<ICoreWebView2ContextMenuRequestedEventHandler>
             ([this](ICoreWebView2*,
                     ICoreWebView2ContextMenuRequestedEventArgs *args) -> HRESULT {
                 if(!args) return S_OK;
                 args->put_Handled(TRUE);

                 if(m_Impl->m_State.IsRetired()) return S_OK;

                 ContextTarget target;

                 target.m_Position = mapFromGlobal(QCursor::pos());

                 ComPtr<ICoreWebView2ContextMenuTarget> hit;
                 if(SUCCEEDED(args->get_ContextMenuTarget(&hit)) && hit){
                     BOOL has = FALSE;
                     LPWSTR text = nullptr;

                     if(SUCCEEDED(hit->get_HasLinkUri(&has)) && has &&
                        SUCCEEDED(hit->get_LinkUri(&text)) && text){
                         target.m_LinkUrl = QUrl(QString::fromWCharArray(text));
                         CoTaskMemFree(text);
                         text = nullptr;
                     }
                     if(SUCCEEDED(hit->get_HasSourceUri(&has)) && has &&
                        SUCCEEDED(hit->get_SourceUri(&text)) && text){
                         target.m_SourceUrl = QUrl(QString::fromWCharArray(text));
                         CoTaskMemFree(text);
                         text = nullptr;
                     }
                     if(SUCCEEDED(hit->get_HasSelection(&has)) && has &&
                        SUCCEEDED(hit->get_SelectionText(&text)) && text){
                         target.m_SelectedText = QString::fromWCharArray(text);
                         CoTaskMemFree(text);
                         text = nullptr;
                     }

                     BOOL editable = FALSE;
                     if(SUCCEEDED(hit->get_IsEditable(&editable)))
                         target.m_Editable = editable ? true : false;

                     COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND kind =
                         COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_PAGE;
                     if(SUCCEEDED(hit->get_Kind(&kind))){
                         if(kind == COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_IMAGE)
                             target.m_MediaType = Page::MediaTypeImage;
                         else if(kind == COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_AUDIO ||
                                 kind == COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_VIDEO)
                             target.m_MediaType = Page::MediaTypePlayable;
                     }
                 }

                 QPointer<EdgeWebView> alive(this);
                 QTimer::singleShot(0, this, [alive, target](){
                     if(alive) alive->DisplayContextMenuFor(target);
                 });
                 return S_OK;
             }).Get(), &token)))
            VANILLA_KEEP_WEBVIEW_EVENT
                (webview11.Get(), ContextMenuRequested, token);
    }
}


void EdgeWebView::RegisterAskingHandlers(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()) return;

    ICoreWebView2 *webview = m_Impl->m_WebView.Get();
    EventRegistrationToken token = {};

    ComPtr<ICoreWebView2_14> webview14;
    if(FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview14)))) webview14.Reset();
    VANILLA_STOP_IF_RETIRED();

    if(webview14 &&
       SUCCEEDED(webview14->add_ServerCertificateErrorDetected
        (Callback<ICoreWebView2ServerCertificateErrorDetectedEventHandler>
         ([this](ICoreWebView2*,
                 ICoreWebView2ServerCertificateErrorDetectedEventArgs *args) -> HRESULT {
             if(!args) return S_OK;
             if(m_Impl->m_State.IsRetired()){
                 args->put_Action(COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL);
                 return S_OK;
             }

             COREWEBVIEW2_WEB_ERROR_STATUS status =
                 COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID;
             args->get_ErrorStatus(&status);

             QUrl url;
             LPWSTR uri = nullptr;
             if(SUCCEEDED(args->get_RequestUri(&uri)) && uri){
                 url = QUrl(QString::fromWCharArray(uri));
                 CoTaskMemFree(uri);
             }

             ComPtr<ICoreWebView2Certificate> certificate;
             args->get_ServerCertificate(&certificate);
             const QString fingerprint = CertificatePolicy::Fingerprint
                 (CertificateChainOf(certificate.Get()));

             ComPtr<ICoreWebView2Deferral> deferral;
             if(FAILED(args->GetDeferral(&deferral)) || !deferral){
                 args->put_Action(COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL);
                 return S_OK;
             }

             const QString description = CertificateErrorDescription(status);
             QPointer<EdgeWebView> alive(this);
             ComPtr<ICoreWebView2ServerCertificateErrorDetectedEventArgs> held(args);

             QTimer::singleShot(0, EdgeEnvironment::Instance(),
                                [alive, held, deferral, url,
                                 description, fingerprint](){
                 if(alive && !alive->m_Impl->m_State.IsRetired())
                     held->put_Action
                         (static_cast<COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION>
                          (alive->DecideCertificateError(url, description, fingerprint)));
                 else
                     held->put_Action(COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL);
                 deferral->Complete();
             });
             return S_OK;
         }).Get(), &token))){
        VANILLA_KEEP_WEBVIEW_EVENT
            (webview14.Get(), ServerCertificateErrorDetected, token);
    }
    VANILLA_STOP_IF_RETIRED();

    if(SUCCEEDED(webview->add_PermissionRequested
        (Callback<ICoreWebView2PermissionRequestedEventHandler>
         ([this](ICoreWebView2*,
                 ICoreWebView2PermissionRequestedEventArgs *args) -> HRESULT {
             if(!args) return S_OK;

             COREWEBVIEW2_PERMISSION_KIND kind =
                 COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
             args->get_PermissionKind(&kind);

             QUrl origin;
             LPWSTR uri = nullptr;
             if(SUCCEEDED(args->get_Uri(&uri)) && uri){
                 origin = QUrl(QString::fromWCharArray(uri));
                 CoTaskMemFree(uri);
             }

             ComPtr<ICoreWebView2PermissionRequestedEventArgs2> args2;
             if(SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args2))) && args2)
                 args2->put_Handled(TRUE);

             if(m_Impl->m_State.IsRetired()){
                 args->put_State(COREWEBVIEW2_PERMISSION_STATE_DEFAULT);
                 return S_OK;
             }

             ComPtr<ICoreWebView2Deferral> deferral;
             if(FAILED(args->GetDeferral(&deferral)) || !deferral){
                 args->put_State(COREWEBVIEW2_PERMISSION_STATE_DEFAULT);
                 return S_OK;
             }

             QPointer<EdgeWebView> alive(this);
             ComPtr<ICoreWebView2PermissionRequestedEventArgs> held(args);

             QTimer::singleShot(0, EdgeEnvironment::Instance(),
                                [alive, held, deferral, origin, kind](){
                 if(alive && !alive->m_Impl->m_State.IsRetired()){
                     bool remember = false;
                     const int state = alive->DecidePermission
                         (origin, static_cast<int>(kind), &remember);
                     held->put_State
                         (static_cast<COREWEBVIEW2_PERMISSION_STATE>(state));

                     ComPtr<ICoreWebView2PermissionRequestedEventArgs3> args3;
                     if(SUCCEEDED(held->QueryInterface(IID_PPV_ARGS(&args3))) && args3)
                         args3->put_SavesInProfile(remember ? TRUE : FALSE);
                 } else {
                     held->put_State(COREWEBVIEW2_PERMISSION_STATE_DEFAULT);
                 }
                 deferral->Complete();
             });
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, PermissionRequested, token);
    VANILLA_STOP_IF_RETIRED();

    ComPtr<ICoreWebView2_24> webview24;
    if(FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview24)))) webview24.Reset();
    VANILLA_STOP_IF_RETIRED();

    if(webview24 &&
       SUCCEEDED(webview24->add_NotificationReceived
        (Callback<ICoreWebView2NotificationReceivedEventHandler>
         ([this](ICoreWebView2*,
                 ICoreWebView2NotificationReceivedEventArgs *args) -> HRESULT {
             if(!args) return S_OK;
             args->put_Handled(TRUE);

             ComPtr<ICoreWebView2Notification> notification;
             if(FAILED(args->get_Notification(&notification)) || !notification)
                 return S_OK;
             if(m_Impl->m_State.IsRetired()){
                 notification->ReportClosed();
                 return S_OK;
             }

             QUrl origin;
             LPWSTR text = nullptr;
             if(SUCCEEDED(args->get_SenderOrigin(&text)) && text){
                 origin = QUrl(QString::fromWCharArray(text));
                 CoTaskMemFree(text);
                 text = nullptr;
             }
             QString title;
             if(SUCCEEDED(notification->get_Title(&text)) && text){
                 title = QString::fromWCharArray(text);
                 CoTaskMemFree(text);
                 text = nullptr;
             }
             QString body;
             if(SUCCEEDED(notification->get_Body(&text)) && text){
                 body = QString::fromWCharArray(text);
                 CoTaskMemFree(text);
                 text = nullptr;
             }

             notification->ReportShown();

             ComPtr<ICoreWebView2Deferral> deferral;
             const bool held =
                 SUCCEEDED(args->GetDeferral(&deferral)) && deferral;
             if(!held) notification->ReportClosed();

             const auto conclude = [notification, deferral, held](bool opened){
                 if(held){
                     if(opened) notification->ReportClicked();
                     notification->ReportClosed();
                     deferral->Complete();
                 }
             };

             QPointer<EdgeWebView> alive(this);
             QTimer::singleShot(0, EdgeEnvironment::Instance(),
                                [alive, conclude, title, body, origin](){
                 if(!alive || alive->m_Impl->m_State.IsRetired()){
                     conclude(false);
                     return;
                 }
                 alive->ShowPageNotification(title, body, origin, conclude);
             });
             return S_OK;
         }).Get(), &token))){
        VANILLA_KEEP_WEBVIEW_EVENT
            (webview24.Get(), NotificationReceived, token);
    }
}

void EdgeWebView::RegisterInputHandlers(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()) return;

    ICoreWebView2 *webview = m_Impl->m_WebView.Get();
    ICoreWebView2Controller *controller = m_Impl->m_Controller.Get();
    EventRegistrationToken token = {};

    if(SUCCEEDED(webview->add_WebMessageReceived
        (Callback<ICoreWebView2WebMessageReceivedEventHandler>
         ([this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs *args) -> HRESULT {
             if(!args) return S_OK;
             if(m_Impl->m_State.IsRetired()) return S_OK;
             LPWSTR json = nullptr;
             LPWSTR source = nullptr;
             const bool gotJson = SUCCEEDED(args->get_WebMessageAsJson(&json)) && json;
             const bool gotSource = SUCCEEDED(args->get_Source(&source)) && source;
             if(gotJson && gotSource)
                 HandleWebMessage(QString::fromWCharArray(json),
                                  QString::fromWCharArray(source));
             if(json) CoTaskMemFree(json);
             if(source) CoTaskMemFree(source);
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT(webview, WebMessageReceived, token);
    VANILLA_STOP_IF_RETIRED();

    if(controller){
        if(SUCCEEDED(controller->add_AcceleratorKeyPressed
            (Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>
             ([this](ICoreWebView2Controller*,
                     ICoreWebView2AcceleratorKeyPressedEventArgs *args) -> HRESULT {
                 if(!args) return S_OK;

                 COREWEBVIEW2_KEY_EVENT_KIND kind = COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN;
                 UINT32 virtualKey = 0;
                 INT32 lParam = 0;
                 args->get_KeyEventKind(&kind);
                 args->get_VirtualKey(&virtualKey);
                 args->get_KeyEventLParam(&lParam);

                 const bool down =
                     kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN ||
                     kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN;
                 const bool repeat = (lParam & (1 << 30)) != 0;

                 if(HandleAcceleratorKey(static_cast<int>(virtualKey), down, repeat))
                     args->put_Handled(TRUE);
                 return S_OK;
             }).Get(), &token)))
            VANILLA_KEEP_CONTROLLER_EVENT
                (controller, AcceleratorKeyPressed, token);
        VANILLA_STOP_IF_RETIRED();

        if(SUCCEEDED(controller->add_GotFocus
            (Callback<ICoreWebView2FocusChangedEventHandler>
             ([this](ICoreWebView2Controller*, IUnknown*) -> HRESULT {
                 m_Impl->m_HasFocus = true;
                 return S_OK;
             }).Get(), &token)))
            VANILLA_KEEP_CONTROLLER_EVENT(controller, GotFocus, token);
        VANILLA_STOP_IF_RETIRED();

        if(SUCCEEDED(controller->add_LostFocus
            (Callback<ICoreWebView2FocusChangedEventHandler>
             ([this](ICoreWebView2Controller*, IUnknown*) -> HRESULT {
                 m_Impl->m_HasFocus = false;
                 m_Impl->m_HandledKeys.clear();
                 return S_OK;
             }).Get(), &token)))
            VANILLA_KEEP_CONTROLLER_EVENT(controller, LostFocus, token);
        VANILLA_STOP_IF_RETIRED();
    }

    if(m_Impl->m_Composition){
        if(SUCCEEDED(m_Impl->m_Composition->add_CursorChanged
            (Callback<ICoreWebView2CursorChangedEventHandler>
             ([this](ICoreWebView2CompositionController *sender, IUnknown*) -> HRESULT {
                 UINT32 id = 0;
                 if(!sender || FAILED(sender->get_SystemCursorId(&id))) return S_OK;
                 ApplyCursor(id);
                 return S_OK;
             }).Get(), &token)))
            VANILLA_KEEP_COMPOSITION_EVENT
                (m_Impl->m_Composition.Get(), CursorChanged, token);
    }
}

void EdgeWebView::RegisterHandlers(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()) return;
    RegisterNavigationHandlers();
    VANILLA_STOP_IF_RETIRED();
    RegisterDocumentHandlers();
    VANILLA_STOP_IF_RETIRED();
    RegisterAskingHandlers();
    VANILLA_STOP_IF_RETIRED();
    RegisterInputHandlers();
}


HRESULT EdgeWebView::AnswerVanillaPage(ICoreWebView2WebResourceRequestedEventArgs *args,
                                       ICoreWebView2WebResourceRequest *request,
                                       const QUrl &url, bool document){
    ICoreWebView2Environment *environment =
        EdgeEnvironment::Instance()->GetEnvironment();
    if(!environment) return S_OK;

    QByteArray method = QByteArrayLiteral("GET");
    LPWSTR text = nullptr;
    if(SUCCEEDED(request->get_Method(&text)) && text){
        method = QString::fromWCharArray(text).toLatin1();
        CoTaskMemFree(text);
    }

    QByteArray body;
    ComPtr<IStream> content;
    if(SUCCEEDED(request->get_Content(&content)) && content){
        char buffer[4096];
        ULONG read = 0;
        while(SUCCEEDED(content->Read(buffer, sizeof(buffer), &read)) && read > 0)
            body.append(buffer, static_cast<int>(read));
    }

    QString originHeader, refererHeader;
    ComPtr<ICoreWebView2HttpRequestHeaders> requestHeaders;
    if(SUCCEEDED(request->get_Headers(&requestHeaders)) && requestHeaders){
        LPWSTR value = nullptr;
        if(SUCCEEDED(requestHeaders->GetHeader(L"Origin", &value)) && value){
            originHeader = QString::fromWCharArray(value);
            CoTaskMemFree(value);
            value = nullptr;
        }
        if(SUCCEEDED(requestHeaders->GetHeader(L"Referer", &value)) && value){
            refererHeader = QString::fromWCharArray(value);
            CoTaskMemFree(value);
        }
    }
    const QUrl initiator = VanillaPage::InitiatorFromHeaders
        (originHeader, refererHeader, m_Impl->m_Url,
         document && method != QByteArrayLiteral("GET"));

    const VanillaPageResponse answer =
        VanillaPage::Answer(url, method, body, initiator);

    ComPtr<IStream> stream;
    if(!answer.m_Body.isEmpty())
        stream.Attach(SHCreateMemStream
                      (reinterpret_cast<const BYTE*>(answer.m_Body.constData()),
                       static_cast<UINT>(answer.m_Body.size())));

    const QString headers = QStringLiteral("Content-Type: ") +
        QString::fromLatin1(answer.m_ContentType);
    const QString reason = answer.m_Status == 200
        ? QStringLiteral("OK") : QStringLiteral("Error");

    ComPtr<ICoreWebView2WebResourceResponse> response;
    if(SUCCEEDED(environment->CreateWebResourceResponse
                 (stream.Get(), answer.m_Status,
                  reinterpret_cast<PCWSTR>(reason.utf16()),
                  reinterpret_cast<PCWSTR>(headers.utf16()), &response)) && response)
        args->put_Response(response.Get());
    return S_OK;
}

void EdgeWebView::RegisterResourceFilter(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()) return;

    if(FAILED(m_Impl->m_WebView->AddWebResourceRequestedFilter
              (L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL))){
        qWarning() << "edge: no web resource filter; blocking and DNT are off";
        return;
    }
    VANILLA_STOP_IF_RETIRED();

    EventRegistrationToken token = {};
    if(SUCCEEDED(m_Impl->m_WebView->add_WebResourceRequested
        (Callback<ICoreWebView2WebResourceRequestedEventHandler>
         ([this](ICoreWebView2*,
                 ICoreWebView2WebResourceRequestedEventArgs *args) -> HRESULT {
             if(!args) return S_OK;
             const bool retired = m_Impl->m_State.IsRetired();

             ComPtr<ICoreWebView2WebResourceRequest> request;
             if(FAILED(args->get_Request(&request)) || !request) return S_OK;

             COREWEBVIEW2_WEB_RESOURCE_CONTEXT context =
                 COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL;
             args->get_ResourceContext(&context);
             const bool document =
                 context == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT;

             const bool bypassCache = document && !retired && m_Impl->m_BypassCacheOnce;
             if(document && !retired) m_Impl->m_BypassCacheOnce = false;

             {
                 LPWSTR pageUri = nullptr;
                 QUrl pageUrl;
                 if(SUCCEEDED(request->get_Uri(&pageUri)) && pageUri){
                     pageUrl = QUrl(QString::fromWCharArray(pageUri));
                     CoTaskMemFree(pageUri);
                 }
                 if(VanillaPage::IsPageUrl(pageUrl)){
                     if(!retired)
                         return AnswerVanillaPage(args, request.Get(), pageUrl,
                                                  document);
                     ICoreWebView2Environment *environment =
                         EdgeEnvironment::Instance()->GetEnvironment();
                     ComPtr<ICoreWebView2WebResourceResponse> response;
                     if(environment &&
                        SUCCEEDED(environment->CreateWebResourceResponse
                                  (nullptr, 410, L"Gone", L"", &response)) && response)
                         args->put_Response(response.Get());
                     return S_OK;
                 }
             }

             ComPtr<ICoreWebView2HttpRequestHeaders> headers;
             request->get_Headers(&headers);

             if(headers){
                 if(UrlBlockRules::SendDoNotTrack()){
                     headers->SetHeader(L"DNT", L"1");
                     headers->SetHeader(L"Sec-GPC", L"1");
                 }
                 const QString languages = Application::GetAcceptLanguage();
                 if(!languages.isEmpty())
                     headers->SetHeader
                         (L"Accept-Language",
                          reinterpret_cast<PCWSTR>(languages.utf16()));

                 if(bypassCache){
                     headers->SetHeader(L"Cache-Control", L"no-cache");
                     headers->SetHeader(L"Pragma", L"no-cache");
                 }
             }

             if(document) return S_OK;

             LPWSTR uri = nullptr;
             if(FAILED(request->get_Uri(&uri)) || !uri) return S_OK;
             const QString url = QString::fromWCharArray(uri);
             CoTaskMemFree(uri);

             if(!UrlBlockRules::IsBlocked(url)) return S_OK;

             ICoreWebView2Environment *environment =
                 EdgeEnvironment::Instance()->GetEnvironment();
             if(!environment) return S_OK;

             ComPtr<ICoreWebView2WebResourceResponse> response;
             if(SUCCEEDED(environment->CreateWebResourceResponse
                          (nullptr, 403, L"Blocked", L"", &response)) && response)
                 args->put_Response(response.Get());
             return S_OK;
         }).Get(), &token)))
        VANILLA_KEEP_WEBVIEW_EVENT
            (m_Impl->m_WebView.Get(), WebResourceRequested, token);
}

int EdgeWebView::DecideCertificateError(const QUrl &url, const QString &description,
                                        const QString &fingerprint){
    Application::AskSslErrorPolicyIfNeed();

    const QString host = url.host();
    const QString detail = tr("Url: %1\nSsl error : %2").arg(url.toString(), description);

    switch(Application::GetSslErrorPolicy()){
    case Application::IgnoreSslErrors:
        return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW;

    case Application::AskForEachAccess: {
        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors."));
        dialog->SetCaption(tr("Ssl errors."));
        dialog->SetInformativeText(tr("Ignore errors in this access?"));
        dialog->SetDetailedText(detail);
        dialog->SetButtons(Dialog::Allow | Dialog::Block);
        if(dialog->Execute() && dialog->ClickedButton() == Dialog::Allow)
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW;
        return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;
    }

    case Application::AskForEachHost: {
        if(Application::GetBlockedHosts().contains(host))
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;
        if(Application::GetAllowedHosts().contains(host))
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW;

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors on host:%1").arg(host));
        dialog->SetCaption(tr("Ssl errors on host:%1").arg(host));
        dialog->SetInformativeText(tr("Allow or Block this host?"));
        dialog->SetDetailedText(detail);
        dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
        dialog->Execute();

        const Dialog::Button clicked = dialog->ClickedButton();
        if(clicked == Dialog::Allow){
            Application::AppendToAllowedHosts(host);
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW;
        }
        if(clicked == Dialog::Block) Application::AppendToBlockedHosts(host);
        return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;
    }

    case Application::AskForEachCertificate: {
        if(fingerprint.isEmpty())
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;

        const QString key = CertificatePolicy::KeyFromFingerprint(host, fingerprint);
        switch(CertificatePolicy::Find(key,
                                       Application::GetAllowedCertificates(),
                                       Application::GetBlockedCertificates())){
        case CertificatePolicy::Allow:
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW;
        case CertificatePolicy::Block:
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;
        case CertificatePolicy::Ask:
            break;
        }

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors on host:%1").arg(host));
        dialog->SetCaption(tr("Ssl errors on host:%1").arg(host));
        dialog->SetInformativeText(tr("Allow or Block this certificate?"));
        dialog->SetDetailedText(detail + QStringLiteral("\nSHA-256: ") + fingerprint);
        dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
        dialog->Execute();

        const Dialog::Button clicked = dialog->ClickedButton();
        if(clicked == Dialog::Allow){
            Application::RememberCertificate(key, true);
            return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW;
        }
        if(clicked == Dialog::Block) Application::RememberCertificate(key, false);
        return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;
    }

    default:
        return COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL;
    }
}

int EdgeWebView::DecidePermission(const QUrl &origin, int kind, bool *remember){
    if(remember) *remember = false;

    const QString name = PermissionName
        (static_cast<COREWEBVIEW2_PERMISSION_KIND>(kind));
    if(name.isEmpty()) return COREWEBVIEW2_PERMISSION_STATE_DEFAULT;

    ModalDialog *dialog = new ModalDialog();
    dialog->SetTitle(tr("Feature Permission Requested."));
    dialog->SetCaption(tr("Feature Permission Requested."));
    dialog->SetInformativeText
        (tr("Url: ") + origin.toString() + QStringLiteral("\n") +
         tr("Feature: ") + name + QStringLiteral("\n\n") +
         tr("Allow this feature?"));
    dialog->SetButtons(Dialog::Yes | Dialog::No | Dialog::Cancel);
    dialog->Execute();

    const Dialog::Button clicked = dialog->ClickedButton();
    if(clicked != Dialog::Yes && clicked != Dialog::No)
        return COREWEBVIEW2_PERMISSION_STATE_DEFAULT;

    if(remember)
        *remember = !m_Impl->m_PrivateMode &&
            Application::GlobalSettings()
                .value(QStringLiteral("network/@RememberPermissions"), true).value<bool>();

    return clicked == Dialog::Yes
        ? COREWEBVIEW2_PERMISSION_STATE_ALLOW
        : COREWEBVIEW2_PERMISSION_STATE_DENY;
}

void EdgeWebView::ShowPageNotification(const QString &title, const QString &body,
                                       const QUrl &origin,
                                       std::function<void(bool)> answer){
    ModelessDialog *dialog = new ModelessDialog();
    dialog->SetTitle(title.isEmpty() ? origin.host() : title);
    dialog->SetCaption(body);
    dialog->SetButtons(Dialog::Open | Dialog::Close);
    dialog->SetDefaultValue(false);

    auto once = std::make_shared<EdgeOnce>();
    auto conclude = [once, answer](bool opened){
        if(once->Take()) answer(opened);
    };
    dialog->SetCallBack([dialog, conclude](bool ok){
        conclude(ok && dialog->ClickedButton() == Dialog::Open);
    });
    QObject *sentinel = new QObject(dialog);
    QObject::connect(sentinel, &QObject::destroyed, EdgeEnvironment::Instance(),
                     [conclude](){ conclude(false);});

    QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
}

void EdgeWebView::HandleFullScreen(bool on){
    TreeBank *tb = GetTreeBank();
    if(!tb) return;

    tb->GetMainWindow()->SetFullScreen(on);
    SetDisplayObscured(on);
    emit fullScreenRequested(on);
    if(!on) return;

    ModelessDialog *dialog = new ModelessDialog();
    connect(this, &EdgeWebView::destroyed, dialog, &ModelessDialog::Returned);
    connect(this, &EdgeWebView::fullScreenRequested, dialog, &ModelessDialog::Returned);
    dialog->SetTitle(tr("This page becomes full screen mode."));
    dialog->SetCaption(tr("Press Esc to exit."));
    dialog->SetButtons(Dialog::Ok | Dialog::Cancel);
    dialog->SetDefaultValue(true);
    dialog->SetCallBack([this](bool ok){ if(!ok) ExitFullScreen();});
    QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
}

void EdgeWebView::ExitFullScreen(){
    RunScript(QStringLiteral("if(document.exitFullscreen) document.exitFullscreen();"));
}

#endif
