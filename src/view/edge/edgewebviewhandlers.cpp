#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"
#include "extensionhostwire.hpp"

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QCursor>
#include <QThread>
#include <QSslCertificate>
#include <QElapsedTimer>

#include "dialog.hpp"
#include "certificatepolicy.hpp"
#include "page.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "mainwindow.hpp"
#include "application.hpp"
#include "networkcontroller.hpp"
#include "notifier.hpp"
#include "downloadname.hpp"
#include "loadending.hpp"
#include "settingspage.hpp"

class EdgeDownloadAdapter : public QObject {
    Q_OBJECT

    Q_PROPERTY(QUrl url READ GetUrl CONSTANT)
    Q_PROPERTY(QString path READ GetPath CONSTANT)
    Q_PROPERTY(int state READ GetState NOTIFY stateChanged)
    Q_PROPERTY(int interruptReason READ GetInterruptReason NOTIFY stateChanged)
    Q_PROPERTY(qint64 receivedBytes READ GetReceivedBytes NOTIFY receivedBytesChanged)
    Q_PROPERTY(qint64 totalBytes READ GetTotalBytes NOTIFY receivedBytesChanged)

public:
    EdgeDownloadAdapter(QObject *parent,
                        Microsoft::WRL::ComPtr<ICoreWebView2DownloadOperation> operation,
                        std::shared_ptr<EdgeDownloadCell> cell,
                        const QUrl &url, const QString &path)
        : QObject(parent)
        , m_Operation(operation)
        , m_Cell(cell)
        , m_Url(url)
        , m_Path(path)
        , m_Releasing(false)
    {}

    QUrl GetUrl() const { return m_Url;}
    QString GetPath() const { return m_Path;}
    int GetState() const { return m_Cell ? m_Cell->ReportedState() : 0;}
    int GetInterruptReason() const { return m_Cell ? m_Cell->ReportedReason() : 0;}
    qint64 GetReceivedBytes() const { return m_Cell ? m_Cell->ReceivedBytes() : 0;}
    qint64 GetTotalBytes() const { return m_Cell ? m_Cell->TotalBytes() : -1;}

    void NotifyState(){ emit stateChanged();}
    void NotifyReceivedBytes(){ emit receivedBytesChanged();}

    void Replay(){
        NotifyReceivedBytes();
        NotifyState();
    }

    void ReleaseAfterUnwind(){
        if(m_Releasing) return;
        m_Releasing = true;
        EdgeDownloadCarriers::Instance()->WhenCallsAreDone(this, [this](){
            m_Operation.Reset();
            m_Cell.reset();
            deleteLater();
        });
    }

public slots:

    void cancel(){
        if(!m_Operation) return;
        HRESULT hr = E_FAIL;
        {
            EdgeBackendCall call(EdgeDownloadCarriers::Instance()->Calls());
            hr = m_Operation->Cancel();
        }
        if(FAILED(hr))
            qWarning() << "edge: a download refused to be cancelled;"
                       << "it continues, and its row has already gone";
    }

    void resume(){
        if(!m_Cell || m_Cell->ResumeAttempted()) return;
        m_Cell->MarkResumeAttempted();
        if(!m_Operation) return;

        HRESULT hr = E_FAIL;
        {
            EdgeBackendCall call(EdgeDownloadCarriers::Instance()->Calls());
            hr = m_Operation->Resume();
        }
        if(SUCCEEDED(hr)) return;

        qWarning() << "edge: a download refused to be resumed";
        m_Cell->FailResume();
        EdgeDownloadCarriers::Instance()->WhenCallsAreDone(this, [this](){
            NotifyState();
            ReleaseAfterUnwind();
        });
    }

signals:
    void stateChanged();
    void receivedBytesChanged();

private:
    Microsoft::WRL::ComPtr<ICoreWebView2DownloadOperation> m_Operation;
    std::shared_ptr<EdgeDownloadCell> m_Cell;
    QUrl m_Url;
    QString m_Path;
    bool m_Releasing;
};

namespace {

    qint64 EdgeMonotonicMsec(){
        static QElapsedTimer clock;
        if(!clock.isValid()) clock.start();
        return clock.elapsed();
    }

}

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
}

void EdgeWebView::AdoptReportedSource(const QUrl &reported){
    if(reported.isEmpty()) return;

    if(!IsOwnReportedSource(reported)){
        m_Impl->m_Url = reported;
        m_Impl->m_StringDocument.ViewWentElsewhere();
    }
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

             if(args && EdgeDownloadCarriers::Instance()->Contains(this)){
                 args->put_Cancel(TRUE);
                 return S_OK;
             }

             if(args){
                 UINT64 navigation = 0;
                 args->get_NavigationId(&navigation);
                 LPWSTR uri = nullptr;
                 QString address;
                 if(SUCCEEDED(args->get_Uri(&uri)) && uri){
                     address = QString::fromWCharArray(uri);
                     CoTaskMemFree(uri);
                 }
                 m_Impl->m_Aborts.Started(navigation, address);
             }

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
                !EdgeIsOwnViewSource(m_Impl->m_Url, reported)){
                 AdoptReportedSource(reported);
             } else if(m_Impl->m_History.CurrentUrl() != m_Impl->m_Url){
                 m_Impl->m_History.Visit(m_Impl->m_Url);
                 SaveHistory();
             }
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

             COREWEBVIEW2_WEB_ERROR_STATUS webError =
                 COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
             if(args) args->get_WebErrorStatus(&webError);
             UINT64 navigation = 0;
             if(args) args->get_NavigationId(&navigation);

             if(success){
                 m_Impl->m_Aborts.Completed(navigation, true, int(webError),
                                            EdgeMonotonicMsec(), nullptr);
                 emit statusBarMessage(tr("Finished loading."));
             } else {
                 int generation = 0;
                 const EdgeAbortLatch::Verdict verdict =
                     m_Impl->m_Aborts.Completed(navigation, false, int(webError),
                                                EdgeMonotonicMsec(), &generation);
                 if(verdict == EdgeAbortLatch::Verdict::Say)
                     emit statusBarMessage(tr("Failed to load."));
                 else if(verdict == EdgeAbortLatch::Verdict::Hold)
                     AskAgainAboutAbort(generation, EdgeAbortLatch::Wait);
             }

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

namespace {

    QList<EdgeMenuItem> MenuItemsOf(ICoreWebView2ContextMenuItemCollection *items, bool extensionsOnly);

    EdgeMenuItem MenuItemOf(ICoreWebView2ContextMenuItem *item){
        EdgeMenuItem made;
        LPWSTR label = nullptr;
        if(SUCCEEDED(item->get_Label(&label)) && label){
            made.label = QString::fromWCharArray(label);
            CoTaskMemFree(label);
        }
        INT32 id = -1;
        if(SUCCEEDED(item->get_CommandId(&id))) made.commandId = id;
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND kind = COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND;
        if(SUCCEEDED(item->get_Kind(&kind))){
            switch(kind){
            case COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_CHECK_BOX: made.kind = EdgeMenuItem::Kind::CheckBox; break;
            case COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_RADIO:     made.kind = EdgeMenuItem::Kind::Radio;    break;
            case COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SEPARATOR: made.kind = EdgeMenuItem::Kind::Separator; break;
            case COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SUBMENU:   made.kind = EdgeMenuItem::Kind::Submenu;  break;
            default:                                            made.kind = EdgeMenuItem::Kind::Command;  break;
            }
        }
        BOOL flag = FALSE;
        if(SUCCEEDED(item->get_IsEnabled(&flag))) made.enabled = flag ? true : false;
        flag = FALSE;
        if(SUCCEEDED(item->get_IsChecked(&flag))) made.checked = flag ? true : false;
        if(made.kind == EdgeMenuItem::Kind::Submenu){
            ComPtr<ICoreWebView2ContextMenuItemCollection> children;
            if(SUCCEEDED(item->get_Children(&children)) && children)
                made.children = MenuItemsOf(children.Get(), false);
        }
        return made;
    }

    QList<EdgeMenuItem> MenuItemsOf(ICoreWebView2ContextMenuItemCollection *items, bool extensionsOnly){
        QList<EdgeMenuItem> made;
        UINT32 count = 0;
        if(FAILED(items->get_Count(&count))) return made;
        for(UINT32 i = 0; i < count; ++i){
            ComPtr<ICoreWebView2ContextMenuItem> item;
            if(FAILED(items->GetValueAtIndex(i, &item)) || !item) continue;
            if(extensionsOnly){
                LPWSTR name = nullptr;
                const bool ours = SUCCEEDED(item->get_Name(&name)) && name && wcscmp(name, L"extension") == 0;
                if(name) CoTaskMemFree(name);
                if(!ours) continue;
            }
            made.append(MenuItemOf(item.Get()));
        }
        return made;
    }
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
             QString file;
             if(SUCCEEDED(args->get_ResultFilePath(&path)) && path){
                 file = QString::fromWCharArray(path);
                 name = QFileInfo(file).fileName();
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
                              (reinterpret_cast<PCWSTR>(native.utf16())))){
                     file = native;
                     name = QFileInfo(native).fileName();
                 }
             }
             file = QDir::cleanPath(QDir::fromNativeSeparators(file));

             emit statusBarMessage(tr("Downloading %1").arg(name));

             const bool downloadOnly = m_Impl->m_History.IsEmpty();
             if(downloadOnly) m_Impl->m_Downloads.Started();

             auto settled = std::make_shared<bool>(false);

             auto cell = std::make_shared<EdgeDownloadCell>();
             auto wired = std::make_shared<QPointer<EdgeDownloadAdapter>>();

             QPointer<EdgeWebView> alive(this);
             EventRegistrationToken stateToken = {};
             const HRESULT hr = operation->add_StateChanged
                 (Callback<ICoreWebView2StateChangedEventHandler>
                  ([alive, name, downloadOnly, settled, cell, wired]
                   (ICoreWebView2DownloadOperation *sender, IUnknown*) -> HRESULT {
                      if(!sender) return S_OK;

                      COREWEBVIEW2_DOWNLOAD_STATE state =
                          COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS;
                      sender->get_State(&state);

                      BOOL resumable = FALSE;
                      if(state == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED &&
                         FAILED(sender->get_CanResume(&resumable)))
                          resumable = FALSE;

                      const EdgeDownloadReport report =
                          cell->Arrive(static_cast<EdgeDownloadCell::Backend>(state),
                                       resumable != FALSE);
                      if(EdgeDownloadAdapter *adapter = wired->data()){
                          adapter->NotifyState();
                          if(report.terminal) adapter->ReleaseAfterUnwind();
                      }

                      if(!alive) return S_OK;

                      if(state == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED)
                          emit alive->statusBarMessage(tr("Downloaded %1").arg(name));
                      else if(state == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED)
                          emit alive->statusBarMessage(tr("Download of %1 was interrupted").arg(name));
                      else
                          return S_OK;

                      if(!report.terminal) return S_OK;

                      if(!downloadOnly || *settled) return S_OK;
                      *settled = true;
                      alive->m_Impl->m_Downloads.Settled();

                      if(!alive->m_Impl->m_Downloads.MayClose()) return S_OK;

                      EdgeDownloadCarriers::Instance()->ReleaseLater(alive->GetThis());

                      if(!alive->m_Impl->m_History.IsEmpty()) return S_OK;

                      QPointer<EdgeWebView> waiting(alive);
                      View::CloseLater(alive->GetThis(), [waiting](){
                          return waiting && !waiting->m_Impl->m_State.IsRetired() &&
                              waiting->m_Impl->m_Downloads.MayClose() &&
                              waiting->m_Impl->m_History.IsEmpty();
                      });
                      return S_OK;
                  }).Get(), &stateToken);

             if(FAILED(hr)){
                 if(downloadOnly){
                     qWarning() << "edge: a download cannot be watched;"
                                << "this tab will not close itself";
                     m_Impl->m_Downloads.Untracked();
                 }
                 return S_OK;
             }

             EventRegistrationToken bytesToken = {};
             operation->add_BytesReceivedChanged
                 (Callback<ICoreWebView2BytesReceivedChangedEventHandler>
                  ([cell, wired]
                   (ICoreWebView2DownloadOperation *sender, IUnknown*) -> HRESULT {
                      if(!sender) return S_OK;
                      INT64 received = 0;
                      INT64 total = -1;
                      if(FAILED(sender->get_BytesReceived(&received))) received = 0;
                      if(FAILED(sender->get_TotalBytesToReceive(&total))) total = -1;
                      cell->SetProgress(received, total);
                      if(EdgeDownloadAdapter *adapter = wired->data())
                          adapter->NotifyReceivedBytes();
                      return S_OK;
                  }).Get(), &bytesToken);

             {
                 INT64 received = 0;
                 INT64 total = -1;
                 if(FAILED(operation->get_BytesReceived(&received))) received = 0;
                 if(FAILED(operation->get_TotalBytesToReceive(&total))) total = -1;
                 cell->SetProgress(received, total);
             }

             QUrl remote;
             {
                 LPWSTR uri = nullptr;
                 if(SUCCEEDED(operation->get_Uri(&uri)) && uri){
                     const QString address = QString::fromWCharArray(uri);
                     remote = QUrl(address);
                     CoTaskMemFree(uri);
                     m_Impl->m_Aborts.DownloadStarted(address);
                 }
             }

             EdgeDownloadAdapter *adapter =
                 new EdgeDownloadAdapter(this, operation, cell, remote, file);
             *wired = adapter;

             DownloadItem *item = new DownloadItem(adapter);
             MainWindow *win = Application::GetCurrentWindow();
             if(win && win->GetTreeBank() && win->GetTreeBank()->GetNotifier())
                 win->GetTreeBank()->GetNotifier()->RegisterDownload(item);

             const bool terminalBeforeReplay = cell->IsTerminal();

             adapter->Replay();
             if(terminalBeforeReplay) adapter->ReleaseAfterUnwind();

             if(!downloadOnly || terminalBeforeReplay) return S_OK;

             ViewNode *vn = GetViewNode();
             TreeBank *tb = GetTreeBank();
             if(!vn || !tb) return S_OK;

             SharedView held = tb->ExtractDownloadCarrier(vn);
             if(!held) return S_OK;

             EdgeDownloadCarriers::Instance()->Add(held);

             if(m_Impl->m_Downloads.MayClose())
                 EdgeDownloadCarriers::Instance()->ReleaseLater(held);

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
                 const int sequence = ++m_Impl->m_MenuSequence;
                 m_Impl->m_MenuHandlerDepth++;
                 struct HandlerDepth {
                     QPointer<EdgeWebView> view;
                     ~HandlerDepth(){ if(view && view->m_Impl->m_MenuHandlerDepth > 0) view->m_Impl->m_MenuHandlerDepth--;}
                 } depth{QPointer<EdgeWebView>(this)};
                 QPointer<EdgeWebView> here(this);
                 args->put_Handled(TRUE);
                 if(!here) return S_OK;

                 if(m_Impl->m_State.IsRetired()) return S_OK;

                 if(EdgeInspectorTraceOn()){
                     fprintf(stderr, "edge-menu: request outstanding=%d generation=%d\n",
                             m_Impl->m_ContextMenu.HasOutstanding() ? 1 : 0, m_Impl->m_ContextMenu.Generation());
                     fflush(stderr);
                 }

                 ContextTarget target;
                 target.m_Sequence = sequence;

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

                 if(!here) return S_OK;
                 if(m_Impl->m_State.IsRetired()) return S_OK;

                 {
                     QPointer<EdgeWebView> still(this);
                     if(m_Impl->m_ContextMenu.HasOutstanding())
                         CompleteContextMenu(m_Impl->m_ContextMenu.Generation(), -1);
                     if(!still) return S_OK;
                     if(m_Impl->m_State.IsRetired()) return S_OK;
                 }
                 if(m_Impl->m_ContextMenuWidget) m_Impl->m_ContextMenuWidget->close();
                 {
                     ComPtr<ICoreWebView2ContextMenuItemCollection> items;
                     if(SUCCEEDED(args->get_MenuItems(&items)) && items)
                         target.m_ExtensionItems = MenuItemsOf(items.Get(), true);
                 }
                 if(!here) return S_OK;
                 if(!target.m_ExtensionItems.isEmpty()){
                     ComPtr<ICoreWebView2Deferral> deferral;
                     const HRESULT deferred = args->GetDeferral(&deferral);
                     if(!here){
                         if(SUCCEEDED(deferred) && deferral) deferral->Complete();
                         return S_OK;
                     }
                     const bool current = !m_Impl->m_State.IsRetired() &&
                         sequence == m_Impl->m_MenuSequence;
                     const int generation =
                         SUCCEEDED(deferred) && deferral && current
                         ? m_Impl->m_ContextMenu.Take() : 0;
                     if(generation > 0){
                         target.m_Generation = generation;
                         m_Impl->m_ContextMenuArgs = args;
                         m_Impl->m_ContextMenuDeferral = deferral;
                     } else {
                         target.m_Superseded = deferral &&
                             (m_Impl->m_ContextMenu.HasOutstanding() || m_Impl->m_ContextMenu.IsRetired());
                         target.m_ExtensionItems.clear();
                         QPointer<EdgeWebView> still(this);
                         if(deferral) CountedComplete(nullptr, deferral.Get(), -1);
                         if(!still) return S_OK;
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
                 if(alive && !alive->IsGoing())
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
                 if(alive && !alive->IsGoing()){
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
                 if(!alive || alive->IsGoing()){
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
                 if(IsGoing() || !m_TreeBank){
                     args->put_Handled(TRUE);
                     return S_OK;
                 }

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
                 if(!m_Impl->m_State.IsRetired() && m_TreeBank && visible() &&
                    m_TreeBank->GetCurrentView().get() == this){
                     if(MainWindow *win = m_TreeBank->GetMainWindow()){
                         Application::SetCurrentWindow(win);
                         win->RaiseAllEdgeWidgets();
                         win->UpdateAllEdgeWidgets();
                     }
                 }
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

namespace {

    void PutVanillaPageResponse
        (ICoreWebView2WebResourceRequestedEventArgs *args,
         ICoreWebView2Environment *environment,
         const VanillaPageResponse &answer){
        Q_ASSERT(!answer.NeedsUserInteraction());

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
    }

    VanillaPageResponse VanillaPageFailure(int status){
        VanillaPageResponse response;
        response.m_Status = status;
        return response;
    }

    class VanillaPageDeferralCompletion {
    public:
        explicit VanillaPageDeferralCompletion
            (ComPtr<ICoreWebView2Deferral> deferral)
            : m_Deferral(deferral), m_Thread(QThread::currentThread())
            , m_Completed(false) {}

        ~VanillaPageDeferralCompletion(){ Complete();}

        void Complete(){
            if(m_Completed) return;
            m_Completed = true;
            Q_ASSERT(QThread::currentThread() == m_Thread);
            ComPtr<ICoreWebView2Deferral> deferral = m_Deferral;
            m_Deferral.Reset();
            if(deferral) deferral->Complete();
        }

    private:
        ComPtr<ICoreWebView2Deferral> m_Deferral;
        QThread *m_Thread;
        bool m_Completed;
    };

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

    if(!answer.NeedsUserInteraction()){
        PutVanillaPageResponse(args, environment, answer);
        return S_OK;
    }

    ComPtr<ICoreWebView2Deferral> deferral;
    if(FAILED(args->GetDeferral(&deferral)) || !deferral){
        PutVanillaPageResponse(args, environment, VanillaPageFailure(503));
        return S_OK;
    }

    QCoreApplication *application = QCoreApplication::instance();
    Q_ASSERT(!application || QThread::currentThread() == application->thread());

    QPointer<EdgeWebView> alive(this);
    ComPtr<ICoreWebView2WebResourceRequestedEventArgs> heldArgs(args);
    ComPtr<ICoreWebView2Environment> heldEnvironment(environment);
    auto completion =
        std::make_shared<VanillaPageDeferralCompletion>(deferral);

    QTimer::singleShot(0, EdgeEnvironment::Instance(),
                       [alive, heldArgs, heldEnvironment, answer, completion](){
        VanillaPageResponse completed;
        if(!alive || alive->IsGoing())
            completed = VanillaPageFailure(410);
        else
            completed = VanillaPage::CompleteUserInteraction(answer);

        PutVanillaPageResponse(heldArgs.Get(), heldEnvironment.Get(), completed);
        completion->Complete();
    });
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
                 if(pageUrl.scheme() == QLatin1String(ExtensionHostWire::SCHEME)){
                     if(retired){
                         ICoreWebView2Environment *environment =
                             EdgeEnvironment::Instance()->GetEnvironment();
                         ComPtr<ICoreWebView2WebResourceResponse> response;
                         if(environment &&
                            SUCCEEDED(environment->CreateWebResourceResponse
                                      (nullptr, 410, L"Gone", L"", &response)) && response)
                             args->put_Response(response.Get());
                         return S_OK;
                     }
                     return EdgeAnswerExtensionHost(m_Impl->m_Extensions.data(), m_Impl->m_HostNumber,
                                                    m_Impl->m_HostToken.data(), args, request.Get(), pageUrl);
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

    QPointer<EdgeWebView> alive(this);
    QTimer::singleShot(0, dialog, [dialog, alive](){
        if(!alive || alive->IsGoing()){ dialog->deleteLater(); return;}
        dialog->Execute();
    });
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

void EdgeWebView::AskAgainAboutAbort(int generation, qint64 after){
    QPointer<EdgeWebView> waiting(this);
    QTimer::singleShot(int(after), this, [waiting, generation](){
        if(!waiting || waiting->m_Impl->m_State.IsRetired()) return;

        const qint64 now = EdgeMonotonicMsec();
        if(waiting->m_Impl->m_Aborts.Elapsed(now, generation) ==
           EdgeAbortLatch::Verdict::Say){
            emit waiting->statusBarMessage(tr("Failed to load."));
            return;
        }

        const qint64 left = waiting->m_Impl->m_Aborts.Remaining(now, generation);
        if(left > 0) waiting->AskAgainAboutAbort(generation, left);
    });
}

namespace {

    void RetireCarrier(SharedView view){
        if(!view) return;
        if(EdgeWebView *edge = dynamic_cast<EdgeWebView*>(view.get()))
            edge->DeleteLater();
        else
            view->DeleteLater();
    }

}

EdgeDownloadCarriers::EdgeDownloadCarriers()
    : QObject(Application::GetInstance())
    , m_Carriers(SharedViewList())
{
}

EdgeDownloadCarriers *EdgeDownloadCarriers::Instance(){
    static EdgeDownloadCarriers *instance = nullptr;
    if(!instance) instance = new EdgeDownloadCarriers();
    return instance;
}

void EdgeDownloadCarriers::Add(SharedView view){
    if(!view || m_Carriers.contains(view)) return;
    m_Carriers << view;
}

bool EdgeDownloadCarriers::Contains(const View *view) const {
    if(!view) return false;
    foreach(SharedView held, m_Carriers)
        if(held.get() == view) return true;
    return false;
}

void EdgeDownloadCarriers::ReleaseLater(WeakView weak){

    WhenCallsAreDone(this, [this, weak](){
        SharedView view = weak.lock();
        if(!view) return;
        if(!Contains(view.get())) return;

        EdgeWebView *edge = dynamic_cast<EdgeWebView*>(view.get());
        if(edge && !edge->m_Impl->m_Downloads.MayClose()) return;

        m_Carriers.removeAll(view);
        RetireCarrier(view);
    });
}

void EdgeDownloadCarriers::ReleaseAll(){
    WhenCallsAreDone(this, [this](){ RetireEveryCarrier();});
}

void EdgeDownloadCarriers::RetireEveryCarrier(){
    SharedViewList carriers = m_Carriers;
    m_Carriers.clear();
    foreach(SharedView view, carriers) RetireCarrier(view);
}

void EdgeWebView::ReleaseDownloadCarriers(){
    EdgeDownloadCarriers::Instance()->ReleaseAll();
    EdgeDownloadCarriers::Instance()->DrainNow();
}

#include "edgewebviewhandlers.moc"

#endif
