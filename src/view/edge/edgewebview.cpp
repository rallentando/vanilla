#include "switch.hpp"
#include "theme.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "mainwindow.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"
#include "extensionhost.hpp"
#include "edgehostwindow.hpp"

#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QTimer>
#include <QDebug>

#include "directorypage.hpp"
#include "page.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"

EdgeProfileCoordinator s_Profiles;

EdgeWebView::EdgeWebView(TreeBank *parent, QString id, QStringList set)
    : QWidget(parent)
    , View(parent, id, set)
    , m_Impl(new Private())
{
    m_Impl->m_ProfileName = NetworkController::ProfileStorageName(id);
    m_Impl->m_Space = id;

    static int counter = 0;
    m_Impl->m_Token = ++counter;
    m_Impl->m_HostNumber = ExtensionHost::NumberView(this, this);

    m_Impl->m_DropEchoClock.start();

    Initialize();

    EdgeHostWindow *host = new EdgeHostWindow();
    m_Impl->m_HostWindow = host;
    m_Impl->m_Container = QWidget::createWindowContainer(m_Impl->m_HostWindow, this);
    host->GiveQtFocusOnPress(m_Impl->m_Container);
    m_Impl->m_Container->setGeometry(rect());
    m_Impl->m_Container->setFocusPolicy(Qt::StrongFocus);
    m_Impl->m_HostWindow->installEventFilter(this);
    m_Impl->m_Container->installEventFilter(this);
    setFocusProxy(m_Impl->m_Container);

    m_Impl->m_Hwnd = reinterpret_cast<HWND>(m_Impl->m_HostWindow->winId());

    m_Impl->m_PrivateMode = DirectoryPage::SaysPrivate(set);

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new Page(this, nam);
    page()->SetView(this);
    ApplySpecificSettings(set);
    ApplyTheme();

    if(parent) setParent(parent);

    s_Profiles.Opened(this);

    if(m_Impl->m_State.Start() == EdgeControllerState::Effect::RequestEnvironment)
        EdgeEnvironment::Instance()->Request(m_Impl->m_Token, this);
}

EdgeWebView::~EdgeWebView(){
    Retire(true);
}

void EdgeWebView::DeleteLater(){
    if(m_Impl->m_Retiring){
        View::Orphan();
        m_Impl->m_ContextMenu.NoteDeleteLater();
        return;
    }
    QPointer<EdgeWebView> alive(this);
    Retire();
    if(!alive) return;
    if(m_Impl->m_ContextMenu.IsRetireWanted()){
        View::Orphan();
        m_Impl->m_ContextMenu.NoteDeleteLater();
        return;
    }
    View::DeleteLater();
}

bool EdgeWebView::IsGoing() const {
    return m_Impl->m_State.IsRetired() || m_Impl->m_ContextMenu.IsRetireWanted();
}

void EdgeWebView::Retire(bool force){

    if(m_Impl->m_State.IsRetired()) return;

    const EdgeContextMenuState::Leaving leaving = m_Impl->m_ContextMenu.Retire(force);
    if(leaving == EdgeContextMenuState::Leaving::AfterFinish) return;

    const EdgeControllerState::Effect effect = m_Impl->m_State.Retire();
    m_Impl->m_Retiring = true;
    emit ExtensionContextChanged();
    ExtensionHost::ForgetView(m_Impl->m_HostNumber);
    m_Impl->m_HostToken.reset();

    s_Profiles.Closed(this);

    RemoveCompositionHandlers();
    RemoveWebViewHandlers();
    RemoveControllerHandlers();

    AbandonMouse();

    RevokeDropTarget();

    PullCookiesIntoJar();

    m_Impl->m_History.Release();

    m_Impl->m_Aborts.Retire();

#ifdef MEDIATIME
    if(m_Impl->m_MediaTimeSaveTimer){
        killTimer(m_Impl->m_MediaTimeSaveTimer);
        m_Impl->m_MediaTimeSaveTimer = 0;
    }
#endif

    ReleaseInspector(InspectorRelease::Close);

    EdgeEnvironment::Instance()->Forget(m_Impl->m_Token);

    if(effect == EdgeControllerState::Effect::CloseOwned && m_Impl->m_Controller)
        m_Impl->m_Controller->Close();

    m_Impl->m_WebView.Reset();
    m_Impl->m_Controller.Reset();

    m_Impl->m_Composition.Reset();

    ReleaseCompositionTree();

    QPointer<EdgeWebView> alive(this);
    if(leaving == EdgeContextMenuState::Leaving::WithDeferral) FinishContextMenu(-1);
    if(!alive) return;
    m_Impl->m_Retiring = false;
    if(m_Impl->m_ContextMenu.TakeDeleteLater()) View::DeleteLater();
}

QString EdgeWebView::GetTitle(){
    return m_Impl->m_Title;
}

QIcon EdgeWebView::GetIcon(){
    return m_Impl->m_Icon;
}

int EdgeWebView::GetToken() const {
    return m_Impl->m_Token;
}

QString EdgeWebView::GetProfileName() const {
    return m_Impl->m_ProfileName;
}

bool EdgeWebView::IsPrivateMode() const {
    return m_Impl->m_PrivateMode;
}

static QString EdgeFailureHint(long result){
    if(!EdgeFailureMayBeFolderClash(result)) return QString();
    return QLatin1Char('\n') +
        EdgeWebView::tr("Another vanilla sharing this data folder is running with different arguments.\n"
                        "1. If more than one vanilla is installed under Program Files: match their "
                        "Chromium switches and autoplay settings, or move one of them out of Program Files.\n"
                        "2. Otherwise: quit every vanilla and start again.");
}

void EdgeWebView::ReportCreationFailure(long result){
    ShowFailure(EdgeFailureShowsCode(result)
                ? tr("The Edge WebView2 view could not be created (0x%1).")
                  .arg(static_cast<uint>(result), 8, 16, QLatin1Char('0')) + EdgeFailureHint(result)
                : tr("The Edge WebView2 view could not be created."));
}

void EdgeWebView::ShowFailure(const QString &text){
    m_Impl->m_FailureText = text;
    emit statusBarMessage(text);
    PaintHostWindow();
}

void EdgeWebView::EnvironmentReady(){
    if(m_Impl->m_State.EnvironmentReady() == EdgeControllerState::Effect::CreateController)
        CreateController();
}

void EdgeWebView::EnvironmentFailed(long result){
    if(m_Impl->m_State.EnvironmentFailed() != EdgeControllerState::Effect::ReportFailure) return;

    ShowFailure(tr("The Edge WebView2 runtime could not be started (0x%1).")
                .arg(static_cast<uint>(result), 8, 16, QLatin1Char('0')) + EdgeFailureHint(result));
}

bool EdgeWebView::CreateCompositionTree(){
    if(!m_Impl->m_Hwnd) return false;
    if(m_Impl->m_Visual) return true;

    if(FAILED(DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&m_Impl->m_Device))) ||
       !m_Impl->m_Device){
        qWarning() << "edge: no composition device";
        return false;
    }
    if(FAILED(m_Impl->m_Device->CreateTargetForHwnd(m_Impl->m_Hwnd, TRUE, &m_Impl->m_Target)) ||
       !m_Impl->m_Target){
        qWarning() << "edge: no composition target";
        return false;
    }
    if(FAILED(m_Impl->m_Device->CreateVisual(&m_Impl->m_Visual)) || !m_Impl->m_Visual){
        qWarning() << "edge: no visual";
        return false;
    }
    if(FAILED(m_Impl->m_Target->SetRoot(m_Impl->m_Visual.Get()))){
        qWarning() << "edge: SetRoot failed";
        return false;
    }
    m_Impl->m_Device->Commit();
    qInfo() << "edge: composition tree ready";
    return true;
}

void EdgeWebView::ReleaseCompositionTree(){
    if(m_Impl->m_Target) m_Impl->m_Target->SetRoot(nullptr);
    if(m_Impl->m_Device) m_Impl->m_Device->Commit();
    m_Impl->m_Visual.Reset();
    m_Impl->m_Target.Reset();
    m_Impl->m_Device.Reset();
}

void EdgeWebView::FailCreation(const char *why, long result){
    if(why) qWarning() << "edge:" << why << "; the view is refused";
    if(m_Impl->m_State.ControllerFailed() == EdgeControllerState::Effect::ReportFailure)
        ReportCreationFailure(result);
}

void EdgeWebView::UnwireComposition(){
    if(!m_Impl->m_Composition) return;
    m_Impl->m_Composition->put_RootVisualTarget(nullptr);
    m_Impl->m_Composition.Reset();
}

bool EdgeWebView::BuildControllerOptions(ICoreWebView2Environment *environment,
                                         ICoreWebView2Environment10 **environment10,
                                         ICoreWebView2ControllerOptions **options){
    ComPtr<ICoreWebView2Environment10> ten;
    ComPtr<ICoreWebView2ControllerOptions> made;
    if(FAILED(environment->QueryInterface(IID_PPV_ARGS(&ten))) || !ten) return false;
    ten.CopyTo(environment10);
    if(FAILED(ten->CreateCoreWebView2ControllerOptions(&made)) || !made) return false;

    const bool carried =
        SUCCEEDED(made->put_ProfileName
                  (reinterpret_cast<PCWSTR>(m_Impl->m_ProfileName.utf16()))) &&
        SUCCEEDED(made->put_IsInPrivateModeEnabled(m_Impl->m_PrivateMode ? TRUE : FALSE));
    made.CopyTo(options);
    return carried;
}

void EdgeWebView::TakeController(EdgeUnadoptedController &made){
    const HRESULT hidden = made.Get()->put_IsVisible(FALSE);
    if(FAILED(hidden)){
        UnwireComposition();
        FailCreation("the controller could not be hidden", hidden);
        return;
    }

    QString actualName;
    bool actualPrivate = false;
    const bool measured = MeasureProfile(made.Get(), &actualName, &actualPrivate);

    if(m_Impl->m_PrivateMode && (!measured || !actualPrivate)){
        UnwireComposition();
        FailCreation("the controller's profile is not private", S_OK);
        return;
    }

    if(measured){
        if(actualName != m_Impl->m_ProfileName)
            qWarning() << "edge: asked for profile" << m_Impl->m_ProfileName
                       << "and got" << actualName;
        m_Impl->m_ActualProfileName = actualName;
        m_Impl->m_ActualPrivate = actualPrivate;
        m_Impl->m_ProfileMeasured = true;
    }

    if(m_Impl->m_State.ControllerCreated() != EdgeControllerState::Effect::AdoptController) return;

    m_Impl->m_Controller = made.Take();
    m_Impl->m_Controller->get_CoreWebView2(&m_Impl->m_WebView);
    SetupExtensions();
    if(m_Impl->m_State.IsRetired()) return;
    ApplyThemeToBackend();

    if(m_Impl->m_ActualPrivate){
        const QString key = ProfileKey();
        switch(s_Profiles.EnterPrivateProfile(key, this)){
        case EdgePrivateWipeLedger::Effect::Wipe: {
            const bool placed = ClearBrowsingData
                (COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_PROFILE,
                 [key](bool ok){ EdgeWebView::SettlePrivateWipe(key, ok);});
            if(!placed) EdgeWebView::SettlePrivateWipe(key, false);
            break;
        }
        case EdgePrivateWipeLedger::Effect::Wait:
            break;
        case EdgePrivateWipeLedger::Effect::Proceed:
            break;
        case EdgePrivateWipeLedger::Effect::Fail:
            ReportPrivateWipeFailure();
            break;
        }
    }
    RegisterHandlers();
    RegisterDropTarget();
    RegisterInputBridge();
    RegisterResourceFilter();
}

void EdgeWebView::CreateController(){
    ICoreWebView2Environment *environment = EdgeEnvironment::Instance()->GetEnvironment();

    if(!environment || !m_Impl->m_Hwnd){
        FailCreation(nullptr, S_OK);
        return;
    }

    ComPtr<ICoreWebView2Environment10> environment10;
    ComPtr<ICoreWebView2ControllerOptions> options;
    const bool optionsCarried =
        BuildControllerOptions(environment, &environment10, &options);

    switch(EdgeAnswerForControllerOptions(m_Impl->m_PrivateMode, optionsCarried)){
    case EdgeControllerOptionsAnswer::RefusePrivate:
        FailCreation("private mode cannot be asked for", S_OK);
        return;
    case EdgeControllerOptionsAnswer::ShareDefaultProfile:
        qWarning() << "edge: the profile name could not be carried;"
                   << "this view shares the default profile";
        break;
    case EdgeControllerOptionsAnswer::Proceed:
        break;
    }

    QPointer<EdgeWebView> self(this);

    auto completed = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
        ([self](HRESULT result, ICoreWebView2Controller *controller) -> HRESULT {
            EdgeSettleControllerArrival
                (self, SUCCEEDED(result), controller,
                 [&](EdgeUnadoptedController &made){ self->TakeController(made);},
                 [&](){ self->FailCreation(nullptr, result);});
            return S_OK;
        });

    const bool canUseOptions = environment10 && optionsCarried;
    const bool hasCompositionTree = canUseOptions && CreateCompositionTree();

    HRESULT hr = E_FAIL;
    switch(EdgeChooseControllerCreation(canUseOptions, hasCompositionTree)){
    case EdgeControllerCreation::Composition: {
        auto compositionCompleted =
            Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>
            ([self](HRESULT result, ICoreWebView2CompositionController *composition) -> HRESULT {
                ComPtr<ICoreWebView2Controller> controller;
                const bool arrived = SUCCEEDED(result) && composition;
                const char *refusal = nullptr;
                bool usable = false;
                HRESULT reported = result;
                if(arrived){
                    const HRESULT qi =
                        composition->QueryInterface(IID_PPV_ARGS(&controller));
                    usable = SUCCEEDED(qi) && controller;
                    if(!usable){
                        refusal = "composition controller has no controller";
                        reported = qi;
                    }
                }

                EdgeSettleControllerArrival
                    (self, usable, controller.Get(),
                     [&](EdgeUnadoptedController &made){

                    const HRESULT hidden = made.Get()->put_IsVisible(FALSE);
                    if(FAILED(hidden)){
                        self->FailCreation("the controller could not be hidden", hidden);
                        return;
                    }
                    if(self->m_Impl->m_State.IsRetired()) return;

                    self->m_Impl->m_Composition = composition;

                    if(self->m_Impl->m_Visual){
                        if(FAILED(composition->put_RootVisualTarget(self->m_Impl->m_Visual.Get())))
                            qWarning() << "edge: put_RootVisualTarget failed";
                        else if(self->m_Impl->m_Device)
                            self->m_Impl->m_Device->Commit();
                    }

                    qInfo() << "edge: composition controller adopted";
                    self->TakeController(made);
                }, [&](){ self->FailCreation(refusal, reported);});
                return S_OK;
            });

        hr = environment10->CreateCoreWebView2CompositionControllerWithOptions
            (m_Impl->m_Hwnd, options.Get(), compositionCompleted.Get());
        break;
    }
    case EdgeControllerCreation::WindowedWithOptions:
        hr = environment10->CreateCoreWebView2ControllerWithOptions
            (m_Impl->m_Hwnd, options.Get(), completed.Get());
        break;
    case EdgeControllerCreation::WindowedPlain:
        hr = environment->CreateCoreWebView2Controller(m_Impl->m_Hwnd, completed.Get());
        break;
    }

    if(FAILED(hr)){
        QTimer::singleShot(0, this, [this, hr](){ FailCreation(nullptr, hr);});
    }
}

void EdgeWebView::ApplyBounds(){
    if(m_Impl->m_Container) m_Impl->m_Container->setGeometry(rect());
    if(!m_Impl->m_Controller) return;
    m_Impl->m_Controller->put_Bounds(PhysicalBoundsOf(m_Impl->m_HostWindow));
}

void EdgeWebView::ApplyPendingState(){
    if(!m_Impl->m_Controller) return;

    ApplyPageSettings();
    ApplyUserAgent();

    ApplyBounds();
    if(!m_Impl->m_Controller) return;
    ApplyVisibility();

    QTimer::singleShot(0, this, [this](){ ApplyBounds();});

    TryFlushPendingNavigation();

    if(m_Impl->m_Controller && visible() && hasFocus())
        m_Impl->m_Controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}

void EdgeWebView::ApplyVisibility(){
    if(!m_Impl->m_Controller) return;
    m_Impl->m_Controller->put_IsVisible(visible() ? TRUE : FALSE);
}

void EdgeWebView::Navigate(const QUrl &url){
    if(url.isEmpty()) return;

    EdgePendingLoad load;
    load.url = url;
    StartLoad(load);
}

bool EdgeWebView::MayStartLoad() const {
    if(m_Impl->m_State.GetState() != EdgeControllerState::State::Ready) return false;
    if(m_Impl->m_Extensions && !m_Impl->m_Extensions->IsReady()) return false;
    if(!m_Impl->m_ActualPrivate) return true;
    return s_Profiles.IsPrivateProfileClean(ProfileKey());
}

void EdgeWebView::StartLoad(const EdgePendingLoad &load){
    if(load.url.isEmpty()) return;

    m_Impl->m_Url = load.url;
    m_Impl->m_StringDocument.LoadStarted(load);
    emit urlChanged(m_Impl->m_Url);

    if(!m_Impl->m_WebView || !MayStartLoad()){
        m_Impl->m_State.SetPendingLoad(load);
        return;
    }
    PerformLoad(load);
}

void EdgeWebView::TryFlushPendingNavigation(){
    if(!m_Impl->m_State.HasPendingLoad()) return;
    if(!m_Impl->m_WebView || !MayStartLoad()) return;
    PerformLoad(m_Impl->m_State.TakePendingLoad());
}

void EdgeWebView::PerformLoad(const EdgePendingLoad &load){
    if(!m_Impl->m_WebView || load.url.isEmpty()) return;

    const QString url = load.url.toString();

    if(load.IsHtml()){
        if(FAILED(m_Impl->m_WebView->NavigateToString
                  (reinterpret_cast<PCWSTR>(load.html.utf16())))){
            EdgePendingLoad plain;
            plain.url = load.url;
            StartLoad(plain);
        }
        return;
    }

    if(!load.IsRequest()){
        m_Impl->m_WebView->Navigate(reinterpret_cast<PCWSTR>(url.utf16()));
        return;
    }

    ICoreWebView2Environment *environment =
        EdgeEnvironment::Instance()->GetEnvironment();
    ComPtr<ICoreWebView2Environment2> environment2;
    ComPtr<ICoreWebView2_2> webview2;

    if(!environment ||
       FAILED(environment->QueryInterface(IID_PPV_ARGS(&environment2))) || !environment2 ||
       FAILED(m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview2))) || !webview2){
        m_Impl->m_WebView->Navigate(reinterpret_cast<PCWSTR>(url.utf16()));
        return;
    }

    ComPtr<IStream> content;
    if(!load.body.isEmpty())
        content.Attach(SHCreateMemStream
                       (reinterpret_cast<const BYTE*>(load.body.constData()),
                        static_cast<UINT>(load.body.size())));

    const QString headers = load.headers.join(QStringLiteral("\r\n"));

    ComPtr<ICoreWebView2WebResourceRequest> request;
    if(FAILED(environment2->CreateWebResourceRequest
              (reinterpret_cast<PCWSTR>(url.utf16()),
               reinterpret_cast<PCWSTR>(load.method.utf16()),
               content.Get(),
               reinterpret_cast<PCWSTR>(headers.utf16()),
               &request)) || !request){
        m_Impl->m_WebView->Navigate(reinterpret_cast<PCWSTR>(url.utf16()));
        return;
    }

    webview2->NavigateWithWebResourceRequest(request.Get());
}

void EdgeWebView::NavigateWithRequest(const QNetworkRequest &req,
                                      QNetworkAccessManager::Operation operation,
                                      const QByteArray &body){
    const bool post = operation == QNetworkAccessManager::PostOperation;
    if(!post && req.rawHeaderList().isEmpty()){
        Navigate(req.url());
        return;
    }

    EdgePendingLoad load;
    load.url = req.url();
    load.method = post ? QStringLiteral("POST") : QStringLiteral("GET");

    foreach(const QByteArray &name, req.rawHeaderList())
        load.headers << QString::fromLatin1(name) + QStringLiteral(": ") +
                        QString::fromLatin1(req.rawHeader(name));
    if(post && !req.hasRawHeader("Content-Type"))
        load.headers << QStringLiteral("Content-Type: application/x-www-form-urlencoded");
    if(post) load.body = body;

    StartLoad(load);
}

QWidget *EdgeWebView::base(){ return this;}
Page *EdgeWebView::page(){ return static_cast<Page*>(m_Page);}

QUrl EdgeWebView::url(){ return m_Impl->m_Url;}
QUrl EdgeWebView::ReportedSource() const { return ReportedSourceOf(m_Impl->m_WebView.Get());}
TreeBank *EdgeWebView::parent(){ return m_TreeBank;}

void EdgeWebView::setUrl(const QUrl &url){ Navigate(url);}

void EdgeWebView::setHtml(const QString &html, const QUrl &url){
    EdgePendingLoad load;
    load.url = url.isEmpty() ? BLANK_URL : url;
    load.html = html;
    load.isHtml = true;
    StartLoad(load);
}

bool EdgeWebView::IsOwnReportedSource(const QUrl &reported) const {
    return EdgeIsOwnViewSource(m_Impl->m_Url, reported) ||
        m_Impl->m_StringDocument.IsOwn(m_Impl->m_Url, reported);
}

void EdgeWebView::setParent(TreeBank *t){
    m_TreeBank = t;
    QWidget::setParent(t);
}

bool EdgeWebView::CanGoBack(){
    return m_Impl->m_History.CanGoBack() && !m_Impl->m_History.Busy(IsLoading());
}

bool EdgeWebView::CanGoForward(){
    return m_Impl->m_History.CanGoForward() && !m_Impl->m_History.Busy(IsLoading());
}

void EdgeWebView::GoBackToInferedUrl(){
    if(m_Impl->m_History.Busy(IsLoading())) return;
    View::GoBackToInferedUrl();
}

void EdgeWebView::GoForwardToInferedUrl(){
    if(m_Impl->m_History.Busy(IsLoading())) return;
    View::GoForwardToInferedUrl();
}

bool EdgeWebView::SaveHistory(){
    if(!GetViewNode()) return false;

    const QByteArray data = m_Impl->m_History.Serialize();
    if(data.isEmpty()) return false;

    const QByteArray already = GetViewNode()->GetHistoryData();
    if(!already.isEmpty() && !NativeHistory::LooksLikeOurs(already)) return false;

    GetViewNode()->SetHistoryData(data);
    return true;
}

bool EdgeWebView::RestoreHistory(){
    if(!GetViewNode()) return false;

    NativeHistory restored;
    if(!NativeHistory::Deserialize(GetViewNode()->GetHistoryData(), &restored)) return false;

    m_Impl->m_History = restored;
    Navigate(m_Impl->m_History.CurrentUrl());
    return true;
}

void EdgeWebView::PerformHistoryMove(const NativeHistory::Request &request){
    switch(request.kind){
    case NativeHistory::LoadMove:
        Navigate(request.url);
        break;
    case NativeHistory::ImmediateMove:
        SaveHistory();
        emit ViewChanged();
        break;
    case NativeHistory::NoMove:
        break;
    }
}

void EdgeWebView::TriggerNativeGoBackAction(){
    PerformHistoryMove(m_Impl->m_History.RequestBack(IsLoading()));
}

void EdgeWebView::TriggerNativeGoForwardAction(){
    PerformHistoryMove(m_Impl->m_History.RequestForward(IsLoading()));
}

void EdgeWebView::TriggerNativeRewindAction(){
    PerformHistoryMove(m_Impl->m_History.RequestRewind(IsLoading()));
}

void EdgeWebView::TriggerNativeFastForwardAction(){
    PerformHistoryMove(m_Impl->m_History.RequestFastForward(IsLoading()));
}

void EdgeWebView::Reload(){
    if(m_Impl && m_Impl->m_WebView) m_Impl->m_WebView->Reload();
}

void EdgeWebView::ReloadAndBypassCache(){
    m_Impl->m_BypassCacheOnce = true;
    Reload();
}

void EdgeWebView::Stop(){
    if(m_Impl) m_Impl->m_Aborts.Stopped(IsLoading());
    if(m_Impl && m_Impl->m_WebView) m_Impl->m_WebView->Stop();
}

void EdgeWebView::resizeEvent(QResizeEvent *ev){
    QWidget::resizeEvent(ev);
    ApplyBounds();
}

void EdgeWebView::showEvent(QShowEvent *ev){
    QWidget::showEvent(ev);
    if(m_Impl && m_Impl->m_Controller){
        ApplyBounds();
        if(m_Impl->m_Controller) ApplyVisibility();
    }
    m_Impl->m_Drop.SetReady(m_Impl->m_Drag);
    m_Impl->m_GrabbedDisplayData = QImage();
    RestoreViewState();

    if(m_TreeBank){
        if(Notifier *notifier = m_TreeBank->GetNotifier())
            notifier->SetScroll(m_Impl->m_Scroll);
    }
}

void EdgeWebView::hideEvent(QHideEvent *ev){
    AbandonMouse();
    AbandonDrag();
    m_Impl->m_Drop.SetReady(false);
    SaveViewState();
    if(m_Impl && m_Impl->m_Controller)
        m_Impl->m_Controller->put_IsVisible(FALSE);
    QWidget::hideEvent(ev);
}

bool EdgeWebView::event(QEvent *ev){
    if(ev->type() == QEvent::ParentChange ||
       ev->type() == QEvent::Show ||
       ev->type() == QEvent::DevicePixelRatioChange){
        ApplyBounds();
    }

    return QWidget::event(ev);
}

void EdgeWebView::Connect(TreeBank *tb){
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

void EdgeWebView::Disconnect(TreeBank *tb){
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

void EdgeWebView::OnLoadStarted(){
    View::OnLoadStarted();
}

void EdgeWebView::OnLoadProgress(int progress){
    View::OnLoadProgress(progress);
}

void EdgeWebView::OnLoadFinished(bool ok){
    ApplyTheme();
    View::OnLoadFinished(ok);
}

void EdgeWebView::OnTitleChanged(const QString &title){
    if(!GetViewNode()) return;
    ChangeNodeTitle(title);
}

void EdgeWebView::OnUrlChanged(const QUrl &url){
    if(!GetViewNode()) return;
#ifdef MEDIATIME
    if(url != GetViewNode()->GetUrl()) GetViewNode()->SetMediaTime(0);
#endif
    ChangeNodeUrl(url);
}

void EdgeWebView::OnViewChanged(){
    if(!GetViewNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void EdgeWebView::OnScrollChanged(){}

#endif
