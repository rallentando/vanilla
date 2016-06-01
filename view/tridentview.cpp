#include "switch.hpp"
#include "const.hpp"

#include "tridentview.hpp"

#include <QtWin>

#include <functional>

#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

#include <Exdisp.h>
#include <Mshtml.h>
#include <Mshtmhst.h>
#include <Comdef.h>
#include <Shlobj.h>
#include <Tlogstg.h>

namespace {

    inline QPoint ToPoint(QPoint p){
        return p;
    }

    inline QSize ToSize(QSize s){
        return s;
    }

    template <class T>
    QPoint LocalPos(T *t){
        return t->pos();
    }

    template <class T>
    QPoint GlobalPos(T *t){
        return t->globalPos();
    }

    class WinVariant : public VARIANT{
    public:
        WinVariant(){
            VariantInit(this);
            V_VT(this) = VT_EMPTY;
        }
        virtual ~WinVariant(){
            VariantClear(this);
        }
    };

    class WinStrVariant : public WinVariant{
    public:
        WinStrVariant()
            : WinVariant()
        {
            V_VT(this) = VT_BSTR;
            V_BSTR(this) = 0;
        }
        ~WinStrVariant(){
        }
    };

    class WinI4Variant : public WinVariant{
    public:
        WinI4Variant()
            : WinVariant()
        {
            V_VT(this) = VT_I4;
            V_I4(this) = 0;
        }
        ~WinI4Variant(){
        }
    };

    class WinR4Variant : public WinVariant{
    public:
        WinR4Variant()
            : WinVariant()
        {
            V_VT(this) = VT_R4;
            V_R4(this) = 0;
        }
        ~WinR4Variant(){
        }
    };

    class WinString{
    public:
        WinString(const OLECHAR *str = 0){
            data = SysAllocString(str);
        }
        ~WinString(){
            SysFreeString(data);
        }
        BSTR data;
    };

    QString toQt(BSTR bstr){
        return bstr ? QString((QChar*)bstr, SysStringLen(bstr)) : QString();
    }

    QString toQt(WinString &str){
        return toQt(str.data);
    }

    QString toQt(WinStrVariant &var){
        return V_VT(&var) == VT_BSTR ? toQt(var.bstrVal) : QString();
    }

    QRect toQt(IHTMLRect *rect){
        if(!rect) return QRect();
        long left;
        long right;
        long top;
        long bottom;
        rect->get_left(&left);
        rect->get_right(&right);
        rect->get_top(&top);
        rect->get_bottom(&bottom);
        return QRect(left, top, right-left, bottom-top);
    }
}

class Handler : public IDocHostUIHandler, public IOleCommandTarget{
public:
    Handler(TridentView *parent)
        : IDocHostUIHandler()
        , IOleCommandTarget()
    {
        m_Ref = 0;
        m_Parent = parent;
    }
    virtual ~Handler(){
    }

    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu
    (/* [annotation][in] */
     _In_  DWORD dwID,
     /* [annotation][in] */
     _In_  POINT *ppt,
     /* [annotation][in] */
     _In_  IUnknown *pcmdtReserved,
     /* [annotation][in] */
     _In_  IDispatch *pdispReserved) DECL_OVERRIDE {
        Q_UNUSED(dwID); Q_UNUSED(ppt); Q_UNUSED(pcmdtReserved); Q_UNUSED(pdispReserved);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetHostInfo
    (/* [annotation][out][in] */
     _Inout_  DOCHOSTUIINFO *pInfo) DECL_OVERRIDE {

        pInfo->cbSize        = sizeof(DOCHOSTUIINFO);
        pInfo->dwFlags       = DOCHOSTUIFLAG_NO3DBORDER;
        pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
        pInfo->pchHostCss    = 0;
        pInfo->pchHostNS     = 0;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ShowUI
    (/* [annotation][in] */
     _In_  DWORD dwID,
     /* [annotation][in] */
     _In_  IOleInPlaceActiveObject *pActiveObject,
     /* [annotation][in] */
     _In_  IOleCommandTarget *pCommandTarget,
     /* [annotation][in] */
     _In_  IOleInPlaceFrame *pFrame,
     /* [annotation][in] */
     _In_  IOleInPlaceUIWindow *pDoc) DECL_OVERRIDE {
        Q_UNUSED(dwID); Q_UNUSED(pActiveObject); Q_UNUSED(pCommandTarget);
        Q_UNUSED(pFrame); Q_UNUSED(pDoc);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE HideUI
    (void) DECL_OVERRIDE {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UpdateUI
    (void) DECL_OVERRIDE {
        m_Parent->EmitScrollChangedIfNeed();
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE EnableModeless
    (/* [in] */ BOOL fEnable) DECL_OVERRIDE {
        Q_UNUSED(fEnable);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate
    (/* [in] */ BOOL fActivate) DECL_OVERRIDE {
        Q_UNUSED(fActivate);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate
    (/* [in] */ BOOL fActivate) DECL_OVERRIDE {
        Q_UNUSED(fActivate);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE ResizeBorder
    (/* [annotation][in] */
     _In_  LPCRECT prcBorder,
     /* [annotation][in] */
     _In_  IOleInPlaceUIWindow *pUIWindow,
     /* [annotation][in] */
     _In_  BOOL fRameWindow) DECL_OVERRIDE {
        Q_UNUSED(prcBorder); Q_UNUSED(pUIWindow); Q_UNUSED(fRameWindow);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator
    (/* [in] */ LPMSG lpMsg,
     /* [in] */ const GUID *pguidCmdGroup,
     /* [in] */ DWORD nCmdID) DECL_OVERRIDE {
        Q_UNUSED(lpMsg); Q_UNUSED(pguidCmdGroup); Q_UNUSED(nCmdID);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath
    (/* [annotation][out] */
     _Out_  LPOLESTR *pchKey,
     /* [in] */ DWORD dw) DECL_OVERRIDE {
        Q_UNUSED(pchKey); Q_UNUSED(dw);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDropTarget
    (/* [annotation][in] */
     _In_  IDropTarget *pDropTarget,
     /* [annotation][out] */
     _Outptr_  IDropTarget **ppDropTarget) DECL_OVERRIDE {
        Q_UNUSED(pDropTarget); Q_UNUSED(ppDropTarget);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetExternal
    (/* [annotation][out] */
     _Outptr_result_maybenull_  IDispatch **ppDispatch) DECL_OVERRIDE {

        *ppDispatch = 0;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE TranslateUrl
    (/* [in] */ DWORD dwTranslate,
     /* [annotation][in] */
     _In_  LPWSTR pchURLIn,
     /* [annotation][out] */
     _Outptr_  LPWSTR *ppchURLOut) DECL_OVERRIDE {
        Q_UNUSED(dwTranslate); Q_UNUSED(pchURLIn); Q_UNUSED(ppchURLOut);
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE FilterDataObject
    (/* [annotation][in] */
     _In_  IDataObject *pDO,
     /* [annotation][out] */
     _Outptr_result_maybenull_  IDataObject **ppDORet) DECL_OVERRIDE {
        Q_UNUSED(pDO); Q_UNUSED(ppDORet);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface
    (/* [in] */ REFIID riid,
     /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) DECL_OVERRIDE {

        *ppvObject = 0;

        if(IsEqualIID(riid, IID_IOleCommandTarget)){
            *ppvObject = static_cast<IOleCommandTarget*>(this);
        } else if(IsEqualIID(riid, IID_IDocHostUIHandler)){
            *ppvObject = static_cast<IDocHostUIHandler*>(this);
        } else if(IsEqualIID(riid, IID_IDispatch)){
            *ppvObject = this;
        } else if(IsEqualIID(riid, IID_IUnknown)){
            *ppvObject = this;
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE QueryStatus
    (/* [unique][in] */ __RPC__in_opt const GUID *pguidCmdGroup,
     /* [in] */ ULONG cCmds,
     /* [out][in][size_is] */ __RPC__inout_ecount_full(cCmds) OLECMD prgCmds[],
     /* [unique][out][in] */ __RPC__inout_opt OLECMDTEXT *pCmdText) DECL_OVERRIDE {
        Q_UNUSED(pguidCmdGroup); Q_UNUSED(cCmds); Q_UNUSED(prgCmds); Q_UNUSED(pCmdText);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Exec
    (/* [unique][in] */ __RPC__in_opt const GUID *pguidCmdGroup,
     /* [in] */ DWORD nCmdID,
     /* [in] */ DWORD nCmdexecopt,
     /* [unique][in] */ __RPC__in_opt VARIANT *pvaIn,
     /* [unique][out][in] */ __RPC__inout_opt VARIANT *pvaOut) DECL_OVERRIDE {
        Q_UNUSED(pguidCmdGroup); Q_UNUSED(nCmdexecopt); Q_UNUSED(pvaIn);

        if(nCmdID == OLECMDID_SHOWSCRIPTERROR){
            V_VT(pvaOut) = VT_BOOL;
            V_BOOL(pvaOut) = VARIANT_TRUE;
            return S_OK;
        } else if(nCmdID == OLECMDID_SHOWMESSAGE){
            // which is alert or confirm? cannot identify...
        }
        return OLECMDERR_E_NOTSUPPORTED;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void) DECL_OVERRIDE {
        return InterlockedIncrement(&m_Ref);
    }

    virtual ULONG STDMETHODCALLTYPE Release(void) DECL_OVERRIDE {
        ULONG ref = InterlockedDecrement(&m_Ref);
        if(!ref) delete this;
        return ref;
    }

private:
    ULONG m_Ref;
    TridentView *m_Parent;
};

TridentView::TridentView(TreeBank *parent, QString id, QStringList set)
    : View(parent, id, set)
    , QAxWidget(TreeBank::PurgeView() ? 0 : static_cast<QWidget*>(parent))
{
    // background load blocks thread of displayed view.
    Application::StopAutoLoadTimer();

    Initialize();
    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new Page(this, nam);
    page()->SetView(this);
    ApplySpecificSettings(set);

    if(TreeBank::PurgeView()){
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    } else {
        if(parent) setParent(parent);
    }

    setMouseTracking(true);

    setAcceptDrops(true);

    setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");

    m_Handler = new Handler(this);
    m_Handler->AddRef();
    m_HtmlDocument = 0;
    m_CustomDocument = 0;
    m_Interface = 0;
    queryInterface(IID_IWebBrowser2, (void**)&m_Interface);

    m_Url = QUrl();
    m_Title = QString();
    m_Icon = QIcon();

    connect(this, SIGNAL(signal(const QString&, int, void*)),
            this, SLOT(OnSignal(const QString&, int, void*)));
    connect(this, SIGNAL(exception(int,QString,QString,QString)),
            this, SLOT(OnException(int,QString,QString,QString)));
    connect(this, SIGNAL(BeforeNavigate(QString,int,QString,QVariant&,QString,bool&)),
            this, SLOT(OnBeforeNavigate(QString,int,QString,QVariant&,QString,bool&)));
    connect(this, SIGNAL(BeforeNavigate2(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&)),
            this, SLOT(OnBeforeNavigate2(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&)));
    connect(this, SIGNAL(BeforeScriptExecute(IDispatch*)),
            this, SLOT(OnBeforeScriptExecute(IDispatch*)));
    connect(this, SIGNAL(CommandStateChange(int,bool)),
            this, SLOT(OnCommandStateChange(int,bool)));
    connect(this, SIGNAL(DocumentComplete(IDispatch*,QVariant&)),
            this, SLOT(OnDocumentComplete(IDispatch*,QVariant&)));
    connect(this, SIGNAL(DownloadBegin()),
            this, SLOT(OnDownloadBegin()));
    connect(this, SIGNAL(DownloadComplete()),
            this, SLOT(OnDownloadComplete()));
    connect(this, SIGNAL(FileDownload(bool,bool&)),
            this, SLOT(OnFileDownload(bool,bool&)));
    connect(this, SIGNAL(FrameBeforeNavigate(QString,int,QString,QVariant&,QString,bool&)),
            this, SLOT(OnFrameBeforeNavigate(QString,int,QString,QVariant&,QString,bool&)));
    connect(this, SIGNAL(FrameNavigateComplete(QString)),
            this, SLOT(OnFrameNavigateComplete(QString)));
    connect(this, SIGNAL(NavigateComplete(QString)),
            this, SLOT(OnNavigateComplete(QString)));
    connect(this, SIGNAL(NavigateComplete2(IDispatch*,QVariant&)),
            this, SLOT(OnNavigateComplete2(IDispatch*,QVariant&)));
    connect(this, SIGNAL(NavigateError(IDispatch*,QVariant&,QVariant&,QVariant&,bool&)),
            this, SLOT(OnNavigateError(IDispatch*,QVariant&,QVariant&,QVariant&,bool&)));
    connect(this, SIGNAL(NewWindow(QString,int,QString,QVariant&,QString,bool&)),
            this, SLOT(OnNewWindow(QString,int,QString,QVariant&,QString,bool&)));
    connect(this, SIGNAL(NewWindow2(IDispatch**,bool&)),
            this, SLOT(OnNewWindow2(IDispatch**,bool&)));
    connect(this, SIGNAL(NewWindow3(IDispatch**,bool&,uint,QString,QString)),
            this, SLOT(OnNewWindow3(IDispatch**,bool&,uint,QString,QString)));
    connect(this, SIGNAL(PrivacyImpactedStateChange(bool)),
            this, SLOT(OnPrivacyImpactedStateChange(bool)));
    connect(this, SIGNAL(ProgressChange(int,int)),
            this, SLOT(OnProgressChange(int,int)));
    connect(this, SIGNAL(PropertyChange(QString)),
            this, SLOT(OnPropertyChange(QString)));
    connect(this, SIGNAL(SetSecureLockIcon(int)),
            this, SLOT(OnSetSecureLockIcon(int)));
    connect(this, SIGNAL(StatusTextChange(QString)),
            this, SLOT(OnStatusTextChange(QString)));
    connect(this, SIGNAL(TitleChange(QString)),
            this, SLOT(OnTitleChange(QString)));

    if(m_Interface){
        VARIANT_BOOL on = TRUE;
        m_Interface->put_Silent(on);
    }
}

TridentView::~TridentView(){
}

void TridentView::Clear(){
    if(m_HtmlDocument){
        m_HtmlDocument->Release();
        m_HtmlDocument = 0;
    }
    if(m_CustomDocument){
        m_CustomDocument->SetUIHandler(0);
        m_CustomDocument->Release();
        m_CustomDocument = 0;
    }
    if(m_Interface){
        m_Interface->Release();
        m_Interface = 0;
    }
    if(m_Handler){
        m_Handler->Release();
        m_Handler = 0;
    }
    clear();
}

void TridentView::SetFeatureControl(){
    // register to windows registry.
    QSettings f("HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl",
                QSettings::NativeFormat);

    f.beginGroup("FEATURE_BROWSER_EMULATION");
    int val = f.value("vanilla.exe", 0).value<int>();
    f.endGroup();
    if(val) return; // already configured.

    std::function<void(QString, QVariant)> setValue = [&f](QString group, QVariant var){
        f.beginGroup(group);
        f.setValue("vanilla.exe", var);
        f.endGroup();
    };
    setValue("FEATURE_BROWSER_EMULATION",                       11000);
    setValue("FEATURE_ADDON_MANAGEMENT",                        0 );
    setValue("FEATURE_AJAX_CONNECTIONEVENTS",                   1 );
    setValue("FEATURE_ALIGNED_TIMERS",                          1 );
    setValue("FEATURE_ALLOW_HIGHFREQ_TIMERS",                   1 );
    setValue("FEATURE_BLOCK_CROSS_PROTOCOL_FILE_NAVIGATION",    1 );
    setValue("FEATURE_BLOCK_LMZ_IMG",                           0 );
    setValue("FEATURE_BLOCK_LMZ_OBJECT",                        0 );
    setValue("FEATURE_BLOCK_LMZ_SCRIPT",                        1 );
    setValue("FEATURE_DISABLE_LEGACY_COMPRESSION",              1 );
    setValue("FEATURE_DISABLE_NAVIGATION_SOUNDS",               1 );
    setValue("FEATURE_DOMSTORAGE",                              1 );
    setValue("FEATURE_ENABLE_CLIPCHILDREN_OPTIMIZATION",        1 );
    setValue("FEATURE_ENABLE_SCRIPT_PASTE_URLACTION_IF_PROMPT", 0 );
    setValue("FEATURE_GPU_RENDERING",                           1 );
    setValue("FEATURE_IVIEWOBJECTDRAW_DMLT9_WITH_GDI",          0 );
    setValue("FEATURE_LEGACY_DISPPARAMS",                       0 );
    setValue("FEATURE_LOCALMACHINE_LOCKDOWN",                   0 );
    setValue("FEATURE_MANAGE_SCRIPT_CIRCULAR_REFS",             1 );
    setValue("FEATURE_MAXCONNECTIONSPER1_0SERVER",              6 );
    setValue("FEATURE_MAXCONNECTIONSPERSERVER",                 6 );
    setValue("FEATURE_SCRIPTURL_MITIGATION",                    1 );
    setValue("FEATURE_SPELLCHECKING",                           0 );
    setValue("FEATURE_STATUS_BAR_THROTTLING",                   1 );
    setValue("FEATURE_TABBED_BROWSING",                         1 );
    setValue("FEATURE_USE_LEGACY_JSCRIPT",                      0 );
    setValue("FEATURE_VALIDATE_NAVIGATE_URL",                   1 );
    setValue("FEATURE_WEBOC_DOCUMENT_ZOOM",                     1 );
    setValue("FEATURE_WEBOC_MOVESIZECHILD",                     1 );
    setValue("FEATURE_WEBOC_POPUPMANAGEMENT",                   0 );
    setValue("FEATURE_WEBSOCKET",                               1 );
    setValue("FEATURE_WINDOW_RESTRICTIONS",                     0 );
    setValue("FEATURE_XMLHTTP",                                 1 );
}

void TridentView::ResetDocument(){
    if(!m_Interface) return;
    IDispatch *disp = 0;
    m_Interface->get_Document(&disp);
    if(m_HtmlDocument){
        m_HtmlDocument->Release();
    }
    if(m_CustomDocument){
        m_CustomDocument->SetUIHandler(0);
        m_CustomDocument->Release();
    }
    if(!disp) return;
    disp->QueryInterface(IID_IHTMLDocument2, (void**)&m_HtmlDocument);
    disp->QueryInterface(IID_ICustomDoc, (void**)&m_CustomDocument);
    if(m_CustomDocument) m_CustomDocument->SetUIHandler(m_Handler);
}

int TridentView::GetZoomFactor(){
    if(!m_Interface) return 100;
    WinI4Variant zoomVariant;
    m_Interface->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, 0, &zoomVariant);
    return V_I4(&zoomVariant);
}

void TridentView::SetZoomFactor(int zoom){
    if(!m_Interface) return;
    WinI4Variant zoomVariant;
    V_I4(&zoomVariant) = zoom;
    m_Interface->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &zoomVariant, 0);
}

QAxWidget *TridentView::base(){
    return static_cast<QAxWidget*>(this);
}

Page *TridentView::page(){
    return static_cast<Page*>(View::page());
}

QUrl TridentView::url(){
    return m_Url;
}

QString TridentView::html(){
    return WholeHtml();
}

TreeBank *TridentView::parent(){
    return m_TreeBank;
}

void TridentView::setUrl(const QUrl &url){
    TriggerNativeLoadAction(url);
    emit urlChanged(url);
}

void TridentView::setHtml(const QString &html, const QUrl &url){
    Q_UNUSED(url);
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            elem->put_outerHTML(WinString(html.toStdWString().c_str()).data);
            elem->Release();
        }
        doc->Release();
    }
}

void TridentView::setParent(TreeBank* tb){
    View::SetTreeBank(tb);
    if(!TreeBank::PurgeView()) base()->setParent(tb);
    if(tb) resize(size());
}

void TridentView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(this, SIGNAL(titleChanged(const QString&)),
            tb->parent(), SLOT(SetWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        connect(this, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(page(), SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
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

void TridentView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(this, SIGNAL(titleChanged(const QString&)),
               tb->parent(), SLOT(SetWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                   notifier, SLOT(SetStatus(const QString&, const QString&)));
        disconnect(page(), SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
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

QUrl TridentView::BaseUrl(){
    QString str;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        WinString wstr;
        doc->get_baseUrl(&wstr.data);
        str = toQt(wstr);
        doc->Release();
    }
    if(str.isEmpty()) return url();
    return QUrl(str);
}

bool TridentView::CanGoBack(){
    if(!m_Interface) return false;
    bool ok = false;
    IServiceProvider *provider = 0;
    m_Interface->QueryInterface(IID_IServiceProvider, (void**)&provider);
    if(provider){
        ITravelLogStg *log = 0;
        provider->QueryService(IID_ITravelLogStg, &log);
        if(log){
            ITravelLogEntry *entry = 0;
            log->GetRelativeEntry(-1, &entry);
            if(entry) ok = true;
            log->Release();
        }
        provider->Release();
    }
    return ok;
}

bool TridentView::CanGoForward(){
    if(!m_Interface) return false;
    bool ok = false;
    IServiceProvider *provider = 0;
    m_Interface->QueryInterface(IID_IServiceProvider, (void**)&provider);
    if(provider){
        ITravelLogStg *log = 0;
        provider->QueryService(IID_ITravelLogStg, &log);
        if(log){
            ITravelLogEntry *entry = 0;
            log->GetRelativeEntry(1, &entry);
            if(entry) ok = true;
            log->Release();
        }
        provider->Release();
    }
    return ok;
}

void TridentView::TriggerNativeRewindAction(){
    if(!m_Interface) return;
    IServiceProvider *provider = 0;
    m_Interface->QueryInterface(IID_IServiceProvider, (void**)&provider);
    if(provider){
        ITravelLogStg *log = 0;
        provider->QueryService(IID_ITravelLogStg, &log);
        if(log){
            ITravelLogEntry *entry = 0;
            ITravelLogEntry *result = 0;
            for(int i = -1;; i--){
                entry = 0;
                log->GetRelativeEntry(i, &entry);
                if(entry) result = entry;
                else break;
            }
            if(result) log->TravelTo(result);
            log->Release();
        }
        provider->Release();
    }
}

void TridentView::TriggerNativeFastForwardAction(){
    if(!m_Interface) return;
    IServiceProvider *provider = 0;
    m_Interface->QueryInterface(IID_IServiceProvider, (void**)&provider);
    if(provider){
        ITravelLogStg *log = 0;
        provider->QueryService(IID_ITravelLogStg, &log);
        if(log){
            ITravelLogEntry *entry = 0;
            ITravelLogEntry *result = 0;
            for(int i = 1;; i++){
                entry = 0;
                log->GetRelativeEntry(i, &entry);
                if(entry) result = entry;
                else break;
            }
            if(result) log->TravelTo(result);
            log->Release();
        }
        provider->Release();
    }
}

void TridentView::UpKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollTop;
                elem2->get_scrollTop(&scrollTop);
                elem2->put_scrollTop(scrollTop - 40);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::DownKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollTop;
                elem2->get_scrollTop(&scrollTop);
                elem2->put_scrollTop(scrollTop + 40);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::RightKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollLeft;
                elem2->get_scrollLeft(&scrollLeft);
                elem2->put_scrollLeft(scrollLeft + 40);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::LeftKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollLeft;
                elem2->get_scrollLeft(&scrollLeft);
                elem2->put_scrollLeft(scrollLeft - 40);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::PageDownKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollTop;
                long clientHeight;
                elem2->get_scrollTop(&scrollTop);
                elem2->get_clientHeight(&clientHeight);
                elem2->put_scrollTop(scrollTop + clientHeight*0.9);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::PageUpKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollTop;
                long clientHeight;
                elem2->get_scrollTop(&scrollTop);
                elem2->get_clientHeight(&clientHeight);
                elem2->put_scrollTop(scrollTop - clientHeight*0.9);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::HomeKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                elem2->put_scrollTop(0);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

void TridentView::EndKeyEvent(){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollHeight;
                long clientHeight;
                elem2->get_scrollHeight(&scrollHeight);
                elem2->get_clientHeight(&clientHeight);
                elem2->put_scrollTop(scrollHeight - clientHeight);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChanged();
}

namespace {
    class Element : public WebElement{
    public:
        Element(TridentView *parent = 0)
            : WebElement()
        {
            m_Parent = parent;
            m_Element = 0;
            m_IsEditable = false;
            m_LinkUrl = QUrl();
            m_ImageUrl = QUrl();
            m_Pixmap = QPixmap();
            m_CoordinateOverridden = false;
            m_OverriddenRectangle = QRect();
        }
        Element(TridentView *parent, IHTMLElement *elem)
            : WebElement()
        {
            m_Parent = parent;
            m_Element = elem;
            m_IsEditable = false;
            m_LinkUrl = QUrl();
            m_ImageUrl = QUrl();
            m_Pixmap = QPixmap();
            m_CoordinateOverridden = false;
            m_OverriddenRectangle = QRect();
            m_IsEditable = false;
            m_Element->QueryInterface(IID_IHTMLElement2, (void**)&m_Element2);

            IHTMLElement3 *elem3 = 0;
            m_Element->QueryInterface(IID_IHTMLElement3, (void**)&elem3);
            VARIANT_BOOL editable = VARIANT_FALSE;
            elem3->get_isContentEditable(&editable);
            if(editable == VARIANT_TRUE) m_IsEditable = true;
            elem3->Release();

            m_Element->AddRef();
        }
        ~Element(){
            m_Element = 0;
            m_Element2 = 0;
        }

        bool SetFocus() DECL_OVERRIDE {
            if(m_Element2){
                m_Element2->focus();
                return true;
            }
            return false;
        }
        bool ClickEvent() DECL_OVERRIDE {
            if(m_Element){
                m_Element->click();
                return true;
            }
            return false;
        }
        QString Attribute(WinString &name) const {
            return Attribute(m_Element, name);
        }
        QString Attribute(IHTMLElement *elem, WinString &name) const {
            if(!elem) return QString();
            WinStrVariant attr;
            elem->getAttribute(name.data, 0, &attr);
            QString result = toQt(attr);
            return result;
        }
        QString TagName() const DECL_OVERRIDE {
            if(!m_Element) return QString();
            WinString str;
            m_Element->get_tagName(&str.data);
            QString tag = toQt(str);
            return tag;
        }
        QString InnerText() const DECL_OVERRIDE {
            if(!m_Element) return QString();
            WinString str;
            m_Element->get_innerText(&str.data);
            QString text = toQt(str);
            return text;
        }
        QUrl BaseUrl() const DECL_OVERRIDE {
            return m_Parent->BaseUrl();
        }
        QUrl LinkUrl() const DECL_OVERRIDE {
            if(m_Element && m_LinkUrl.isEmpty()){
                IHTMLElement *elem = m_Element;
                elem->AddRef();
                WinString hrefStr(L"href");
                WinString srcStr(L"src");
                QString href;
                QString src;
                while(elem){
                    href = Attribute(elem, hrefStr);
                    src  = Attribute(elem, srcStr);
                    if(!href.isEmpty() && href != src){
                        elem->Release();
                        break;
                    }
                    IHTMLElement *temp = 0;
                    elem->get_parentElement(&temp);
                    elem->Release();
                    elem = temp;
                }
                return Page::StringToUrl(href, BaseUrl());
            }
            return m_LinkUrl;
        }
        QUrl ImageUrl() const DECL_OVERRIDE {
            if(m_Element && m_ImageUrl.isEmpty()){
                IHTMLElement *elem = m_Element;
                elem->AddRef();
                WinString srcStr(L"src");
                QString src;
                while(elem){
                    src = Attribute(elem, srcStr);
                    if(!src.isEmpty()){
                        elem->Release();
                        break;
                    }
                    IHTMLElement *temp = 0;
                    elem->get_parentElement(&temp);
                    elem->Release();
                    elem = temp;
                }
                return Page::StringToUrl(src, BaseUrl());
            }
            return m_ImageUrl;
        }
        QString LinkHtml() const DECL_OVERRIDE {
            if(!m_Element) return QString();
            IHTMLElement *elem = m_Element;
            elem->AddRef();
            WinString hrefStr(L"href");
            WinString srcStr(L"src");
            QString href;
            QString src;
            QString str;
            while(elem){
                href = Attribute(elem, hrefStr);
                src  = Attribute(elem, srcStr);
                if(!href.isEmpty() && href != src){
                    WinString wstr;
                    elem->get_outerHTML(&wstr.data);
                    str = toQt(wstr);
                    elem->Release();
                    break;
                }
                IHTMLElement *temp = 0;
                elem->get_parentElement(&temp);
                elem->Release();
                elem = temp;
            }
            return str;
        }
        QString ImageHtml() const DECL_OVERRIDE {
            if(!m_Element) return QString();
            IHTMLElement *elem = m_Element;
            elem->AddRef();
            WinString srcStr(L"src");
            QString src;
            QString str;
            while(elem){
                src = Attribute(elem, srcStr);
                if(!src.isEmpty()){
                    WinString wstr;
                    elem->get_outerHTML(&wstr.data);
                    str = toQt(wstr);
                    elem->Release();
                    break;
                }
                IHTMLElement *temp = 0;
                elem->get_parentElement(&temp);
                elem->Release();
                elem = temp;
            }
            return str;
        }
        QPoint Position() const DECL_OVERRIDE {
            if(m_CoordinateOverridden)
                return m_OverriddenRectangle.center();
            return Rectangle().center();
        }
        QRect Rectangle() const DECL_OVERRIDE {
            if(m_CoordinateOverridden)
                return m_OverriddenRectangle;
            IHTMLRect *rect;
            m_Element2->getBoundingClientRect(&rect);
            QRect r = toQt(rect);
            int zoom = m_Parent->GetZoomFactor();
            return QRect(r.topLeft()*zoom/100.0, r.size()*zoom/100.0);
        }
        void SetPosition(QPoint pos) DECL_OVERRIDE {
            int zoom = m_Parent->GetZoomFactor();
            m_CoordinateOverridden = true;
            m_OverriddenRectangle.moveCenter(pos*zoom/100.0);
        }
        void SetRectangle(QRect rect) DECL_OVERRIDE {
            int zoom = m_Parent->GetZoomFactor();
            m_CoordinateOverridden = true;
            m_OverriddenRectangle = QRect(rect.topLeft()*zoom/100.0, rect.size()*zoom/100.0);
        }
        QPixmap Pixmap() DECL_OVERRIDE {
            IHTMLElementRender *render = 0;
            m_Element->QueryInterface(IID_IHTMLElementRender, (void**)&render);
            if(render){
                HWND hwnd = (HWND)m_Parent->winId();
                QRect rect = Rectangle();
                HDC hdc = GetDC(hwnd);
                HBITMAP bitmap = CreateCompatibleBitmap(hdc, rect.width(), rect.height());
                HDC buffer = CreateCompatibleDC(hdc);
                SelectObject(buffer, bitmap);
                render->DrawToDC(buffer);
                m_Pixmap = QtWin::fromHBITMAP(bitmap, QtWin::HBitmapAlpha);
                ReleaseDC(hwnd, hdc);
                DeleteDC(buffer);
                DeleteObject(bitmap);
            }
            return m_Pixmap;
        }
        bool IsNull() const DECL_OVERRIDE {
            return !m_Element || Rectangle().isNull() || Position().isNull();
        }
        bool IsJsCommandElement() const DECL_OVERRIDE {
            if(!m_Element) return false;
            QString onclick = Attribute(WinString(L"onclick"));
            QString href = Attribute(WinString(L"href")).toLower();
            QString src  = Attribute(WinString(L"src")).toLower();
            QString role = Attribute(WinString(L"role")).toLower();
            if(href == src) href = QString();
            return !onclick.isEmpty() ||
                href.startsWith(QStringLiteral("javascript:")) ||
                role == QStringLiteral("button") ||
                role == QStringLiteral("link") ||
                role == QStringLiteral("menu") ||
                role == QStringLiteral("checkbox") ||
                role == QStringLiteral("radio") ||
                role == QStringLiteral("tab");
        }
        bool IsTextInputElement() const DECL_OVERRIDE {
            if(!m_Element) return false;
            QString tag = TagName().toLower();
            QString type = Attribute(WinString(L"type")).toLower();
            return tag == QStringLiteral("textarea") ||
                (tag == QStringLiteral("input") &&
                 (type == QStringLiteral("text") ||
                  type == QStringLiteral("search") ||
                  type == QStringLiteral("password")));
        }
        bool IsQueryInputElement() const DECL_OVERRIDE {
            if(!m_Element) return false;
            QString tag = TagName().toLower();
            QString type = Attribute(WinString(L"type")).toLower();
            return tag == QStringLiteral("input") &&
                (type == QStringLiteral("text") ||
                 type == QStringLiteral("search"));
        }
        bool IsEditableElement() const DECL_OVERRIDE {
            return m_IsEditable
                || IsTextInputElement()
                || IsQueryInputElement();
        }
        bool IsFrameElement() const DECL_OVERRIDE {
            if(!m_Element) return false;
            QString tag = TagName().toLower();
            return tag == QStringLiteral("frame")
                || tag == QStringLiteral("iframe");
        }
        Action GetAction() const DECL_OVERRIDE {
            QString tag = TagName().toLower();
            QString type = Attribute(WinString(L"type")).toLower();
            QString onclick = Attribute(WinString(L"onclick"));
            QString onhover = Attribute(WinString(L"onmouseover"));
            QString href = Attribute(WinString(L"href")).toLower();
            QString src  = Attribute(WinString(L"src")).toLower();
            QString role = Attribute(WinString(L"role")).toLower();
            if(href == src) href = QString();

            if(href.startsWith(QStringLiteral("http:")) ||
               href.startsWith(QStringLiteral("https:"))){

                return None;
            }
            if(m_IsEditable ||
               tag == QStringLiteral("textarea") ||
               tag == QStringLiteral("object") ||
               tag == QStringLiteral("embed") ||
               tag == QStringLiteral("frame") ||
               tag == QStringLiteral("iframe") ||
               (tag == QStringLiteral("input") &&
                (type == QStringLiteral("text") ||
                 type == QStringLiteral("search") ||
                 type == QStringLiteral("password")))){

                return Focus;
            }
            if(!onclick.isEmpty() ||
               href.startsWith(QStringLiteral("javascript:")) ||
               tag == QStringLiteral("button") ||
               tag == QStringLiteral("select") ||
               tag == QStringLiteral("label") ||
               role == QStringLiteral("button") ||
               role == QStringLiteral("link") ||
               role == QStringLiteral("menu") ||
               role == QStringLiteral("checkbox") ||
               role == QStringLiteral("radio") ||
               role == QStringLiteral("tab") ||
               (tag == QStringLiteral("input") &&
                (type == QStringLiteral("checkbox") ||
                 type == QStringLiteral("radio") ||
                 type == QStringLiteral("file") ||
                 type == QStringLiteral("submit") ||
                 type == QStringLiteral("reset") ||
                 type == QStringLiteral("button")))){

                return Click;
            }
            if(!onhover.isEmpty()){

                return Hover;
            }
            return None;
        }
        bool Equals(const WebElement &other) const DECL_OVERRIDE {
            return m_Element == static_cast<const Element*>(&other)->m_Element;
        }

    public:
        TridentView *m_Parent;
        IHTMLElement *m_Element;
        IHTMLElement2 *m_Element2;
        bool m_IsEditable;
        QUrl m_LinkUrl;
        QUrl m_ImageUrl;
        QPixmap m_Pixmap;
        bool m_CoordinateOverridden;
        QRect m_OverriddenRectangle;
    };
}

SharedWebElementList TridentView::FindElements(Page::FindElementsOption option){
    if(!m_HtmlDocument) return SharedWebElementList();
    SharedWebElementList list = SharedWebElementList();
    QList<IHTMLElementCollection*> collections;

    MainWindow *win = Application::GetCurrentWindow();
    QSize s =
        m_TreeBank ? m_TreeBank->size() :
        win ? win->GetTreeBank()->size() :
        !size().isEmpty() ? size() :
        DEFAULT_WINDOW_SIZE;
    QRect viewport = QRect(QPoint(), s);

    std::function<bool(std::shared_ptr<Element>)> isNecessary = [](std::shared_ptr<Element>){
        return true;
    };

    if(option == Page::HaveReference){

        IHTMLElementCollection *collection;
        m_HtmlDocument->get_links(&collection);
        if(collection) collections << collection;

    } else if(option == Page::HaveSource){

        IHTMLElementCollection *collection;
        m_HtmlDocument->get_images(&collection);
        if(collection) collections << collection;

    } else {

        IHTMLElementCollection *collection;
        m_HtmlDocument->get_all(&collection);
        if(collection) collections << collection;

        if(option == Page::ForAccessKey){
            isNecessary = [](std::shared_ptr<Element> elem){
                QString tag = elem->TagName().toLower();
                QString onclick = elem->Attribute(WinString(L"onclick"));
                QString onhover = elem->Attribute(WinString(L"onmouseover"));
                QString href = elem->Attribute(WinString(L"href")).toLower();
                QString src  = elem->Attribute(WinString(L"src")).toLower();
                QString role = elem->Attribute(WinString(L"role")).toLower();
                if(href == src) href = QString();
                return !href.isEmpty() ||
                    !onclick.isEmpty() ||
                    !onhover.isEmpty() ||
                    tag == QStringLiteral("button") ||
                    tag == QStringLiteral("select") ||
                    tag == QStringLiteral("label") ||
                    tag == QStringLiteral("input") ||
                    tag == QStringLiteral("textarea") ||
                    tag == QStringLiteral("object") ||
                    tag == QStringLiteral("embed") ||
                    tag == QStringLiteral("frame") ||
                    tag == QStringLiteral("iframe") ||
                    role == QStringLiteral("button") ||
                    role == QStringLiteral("link") ||
                    role == QStringLiteral("menu") ||
                    role == QStringLiteral("checkbox") ||
                    role == QStringLiteral("radio") ||
                    role == QStringLiteral("tab");
            };
        } else if(option == Page::RelIsNext){
            isNecessary = [](std::shared_ptr<Element> elem){
                QString rel = elem->Attribute(WinString(L"rel")).toLower();
                return rel == QStringLiteral("next");
            };
        } else if(option == Page::RelIsPrev){
            isNecessary = [](std::shared_ptr<Element> elem){
                QString rel = elem->Attribute(WinString(L"rel")).toLower();
                return rel == QStringLiteral("prev");
            };
        } else {
            // invalid enum.
            Q_ASSERT(false);
        }
    }

    foreach(IHTMLElementCollection *collection, collections){
        long len;
        collection->get_length(&len);
        for(long i = 0; i < len; i++){
            WinVariant var;
            WinI4Variant index;
            V_I4(&index) = i;
            IDispatch *disp = 0;
            collection->item(index, var, &disp);
            if(disp){
                IHTMLElement *elem = 0;
                disp->QueryInterface(IID_IHTMLElement, (void**)&elem);
                if(elem){
                    std::shared_ptr<Element> e = std::make_shared<Element>();
                    *e = Element(this, elem);
                    if(!viewport.intersects(e->Rectangle()))
                        e->SetRectangle(QRect());
                    if(isNecessary(e)) list << e;
                    elem->Release();
                }
                disp->Release();
            }
        }
        collection->Release();
    }
    return list;
}

SharedWebElement TridentView::HitElement(const QPoint &pos){
    if(!m_HtmlDocument) return SharedWebElement();
    IHTMLElement *elem = 0;
    int zoom = GetZoomFactor();
    m_HtmlDocument->elementFromPoint(pos.x()*100.0/zoom, pos.y()*100.0/zoom, &elem);
    if(elem){
        std::shared_ptr<Element> e = std::make_shared<Element>();
        *e = Element(this, elem);
        elem->Release();
        return e;
    }
    return SharedWebElement();
}

QUrl TridentView::HitLinkUrl(const QPoint &pos){
    return HitElement(pos)->LinkUrl();
}

QUrl TridentView::HitImageUrl(const QPoint &pos){
    return HitElement(pos)->ImageUrl();
}

QString TridentView::SelectedText(){
    if(!m_HtmlDocument) return QString();
    QString str;
    IHTMLDocument7 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument7, (void**)&doc);
    if(doc){
        IHTMLSelection *selection = 0;
        doc->getSelection(&selection);
        if(selection){
            WinString wstr;
            selection->toString(&wstr.data);
            str = toQt(wstr);
            selection->Release();
        }
        doc->Release();
    }
    return str;
}

QString TridentView::SelectedHtml(){
    if(!m_HtmlDocument) return QString();
    QString str;
    IHTMLSelectionObject *selection = 0;
    m_HtmlDocument->get_selection(&selection);
    if(selection){
        IDispatch *disp = 0;
        selection->createRange(&disp);
        if(disp){
            IHTMLTxtRange *range = 0;
            disp->QueryInterface(IID_IHTMLTxtRange, (void**)&range);
            if(range){
                WinString wstr;
                range->get_htmlText(&wstr.data);
                str = toQt(wstr);
                range->Release();
            }
            disp->Release();
        }
        selection->Release();
    }
    return str;
}

QString TridentView::WholeText(){
    if(!m_HtmlDocument) return QString();
    QString str;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            WinString wstr;
            elem->get_innerText(&wstr.data);
            str = toQt(wstr);
            elem->Release();
        }
        doc->Release();
    }
    return str;
}

QString TridentView::WholeHtml(){
    if(!m_HtmlDocument) return QString();
    QString str;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            WinString wstr;
            elem->get_outerHTML(&wstr.data);
            str = toQt(wstr);
            elem->Release();
        }
        doc->Release();
    }
    return str;
}

QRegion TridentView::SelectionRegion(){
    // not yet implemented.
    return QRegion();
}

QVariant TridentView::EvaluateJavaScript(const QString &source){
    // cannot get return value completely yet.
    // e.g. array, map, object.
    if(!m_HtmlDocument) return QVariant();
    IHTMLWindow2* htmlWindow = 0;
    m_HtmlDocument->get_parentWindow(&htmlWindow);
    if(htmlWindow){
        WinVariant var;
        WinStrVariant result;
        IHTMLElement *elem = 0;
        m_HtmlDocument->createElement(WinString(L"DIV").data, &elem);
        if(elem){
            elem->put_id(WinString(L"__VANILLA_VALUE").data);
            IHTMLElement *body = 0;
            m_HtmlDocument->get_body(&body);
            if(body){
                IHTMLDOMNode *node = 0;
                body->QueryInterface(IID_IHTMLDOMNode, (void**)&node);
                if(node){
                    IHTMLDOMNode *child = 0;
                    elem->QueryInterface(IID_IHTMLDOMNode, (void**)&child);
                    if(child){
                        node->appendChild(child, &child);
                        htmlWindow->execScript
                            (WinString(("document.all[\"__VANILLA_VALUE\"].setAttribute(\"data-value\", (function(){return " + source + ";}()));").toStdWString().c_str()).data,
                             WinString(L"JavaScript").data,
                             &var);
                        IHTMLDocument3 *doc = 0;
                        m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
                        if(doc){
                            IHTMLElement *elem_ = 0;
                            doc->getElementById(WinString(L"__VANILLA_VALUE").data, &elem_);
                            if(elem_){
                                elem_->getAttribute(WinString(L"data-value").data, 0, &result);
                                elem_->Release();
                            }
                            doc->Release();
                        }
                        child->Release();
                    }
                    node->Release();
                }
                body->Release();
            }
            elem->Release();
        }
        htmlWindow->Release();
        QVariant ret = VARIANTToQVariant(result, "QVariant");
        return ret;
    }
    return QVariant();
}

void TridentView::OnSetViewNode(ViewNode*){}

void TridentView::OnSetHistNode(HistNode*){}

void TridentView::OnSetThis(WeakView){}

void TridentView::OnSetMaster(WeakView){}

void TridentView::OnSetSlave(WeakView){}

void TridentView::OnSetJsObject(_View*){}

void TridentView::OnSetJsObject(_Vanilla*){}

void TridentView::OnLoadStarted(){
    // needless to emit statusBarMessage because Trident send message by default.
    View::OnLoadStarted();
    if(m_Icon.isNull() && url() != QUrl(QStringLiteral("about:blank")))
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
}

void TridentView::OnLoadProgress(int progress){
    // needless to emit statusBarMessage because Trident send message by default.
    View::OnLoadProgress(progress);
}

void TridentView::OnLoadFinished(bool ok){
    // needless to emit statusBarMessage because Trident send message by default.
    View::OnLoadFinished(ok);

    RestoreViewState();
    emit ViewChanged();
    if(visible()){
        setFocus();
        if(m_TreeBank &&
           m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
            UpdateThumbnail();
        }
    }
}

void TridentView::OnTitleChanged(const QString &title){
    m_Title = title;
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void TridentView::OnUrlChanged(const QUrl &url){
    m_Url = url;
    if(!GetHistNode()) return;
    ChangeNodeUrl(url);
}

void TridentView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void TridentView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void TridentView::EmitScrollChanged(){
    emit ScrollChanged(GetScroll());
}

void TridentView::EmitScrollChangedIfNeed(){
    EmitScrollChanged();
}

QPointF TridentView::GetScroll(){
    if(!m_HtmlDocument) return QPointF(0.5f, 0.5f);
    QPointF pos = QPointF(0.5f, 0.5f);
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollLeft;
                long scrollTop;
                long scrollWidth;
                long scrollHeight;
                long clientWidth;
                long clientHeight;
                elem2->get_scrollLeft(&scrollLeft);
                elem2->get_scrollTop(&scrollTop);
                elem2->get_scrollWidth(&scrollWidth);
                elem2->get_scrollHeight(&scrollHeight);
                elem2->get_clientWidth(&clientWidth);
                elem2->get_clientHeight(&clientHeight);
                float hval = static_cast<float>(scrollLeft);
                float hmax = static_cast<float>(scrollWidth  - clientWidth);
                float vval = static_cast<float>(scrollTop);
                float vmax = static_cast<float>(scrollHeight - clientHeight);
                if(scrollWidth && scrollHeight){
                    pos = QPointF(hmax == 0.0f ? 0.5f : hval / hmax,
                                  vmax == 0.0f ? 0.5f : vval / vmax);
                }
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    return pos;
}

void TridentView::SetScroll(QPointF pos){
    if(!m_HtmlDocument) return;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollWidth;
                long scrollHeight;
                long clientWidth;
                long clientHeight;
                elem2->get_scrollWidth(&scrollWidth);
                elem2->get_scrollHeight(&scrollHeight);
                elem2->get_clientWidth(&clientWidth);
                elem2->get_clientHeight(&clientHeight);
                float hmax = static_cast<float>(scrollWidth  - clientWidth);
                float vmax = static_cast<float>(scrollHeight - clientHeight);
                elem2->put_scrollLeft(hmax * pos.x());
                elem2->put_scrollTop(vmax * pos.y());
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    emit ScrollChanged(pos);
}

bool TridentView::SaveScroll(){
    if(!m_HtmlDocument) return false;
    QPointF pos = QPointF(0.5f, 0.5f);
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                long scrollLeft;
                long scrollTop;
                elem2->get_scrollLeft(&scrollLeft);
                elem2->get_scrollTop(&scrollTop);
                GetHistNode()->SetScrollX(scrollLeft);
                GetHistNode()->SetScrollY(scrollTop);
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    return true;
}

bool TridentView::RestoreScroll(){
    if(!m_HtmlDocument) return false;
    IHTMLDocument3 *doc = 0;
    m_HtmlDocument->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
    if(doc){
        IHTMLElement *elem = 0;
        doc->get_documentElement(&elem);
        if(elem){
            IHTMLElement2 *elem2 = 0;
            elem->QueryInterface(IID_IHTMLElement2, (void**)&elem2);
            if(elem2){
                elem2->put_scrollLeft(GetHistNode()->GetScrollX());
                elem2->put_scrollTop(GetHistNode()->GetScrollY());
                elem2->Release();
            }
            elem->Release();
        }
        doc->Release();
    }
    EmitScrollChangedIfNeed();
    return true;
}

bool TridentView::SaveZoom(){
    if(!GetHistNode()) return false;
    int zoom = GetZoomFactor();
    if(zoom) GetHistNode()->SetZoom(zoom/100.0);
    return true;
}

bool TridentView::RestoreZoom(){
    if(!GetHistNode()) return false;
    int zoom = GetHistNode()->GetZoom()*100.0;
    if(zoom) SetZoomFactor(zoom);
    return true;
}

bool TridentView::SaveHistory(){
    // not yet implemented.
    return false;
}

bool TridentView::RestoreHistory(){
    // not yet implemented.
    return false;
}

void TridentView::DisplayContextMenu(QWidget *parent, SharedWebElement elem, QPoint globalPos){

    QMenu *menu = new QMenu(parent);
    menu->setToolTipsVisible(true);

    QUrl linkUrl  = elem ? elem->LinkUrl()  : QUrl();
    QUrl imageUrl = elem ? elem->ImageUrl() : QUrl();

    if(linkUrl.isEmpty() && imageUrl.isEmpty() && SelectedText().isEmpty()){
        if(CanGoBack())
            menu->addAction(Action(Page::_Back));
        if(CanGoForward())
            menu->addAction(Action(Page::_Forward));

        if(!View::EnableDestinationInferrer()){
            if(CanGoBack())
                menu->addAction(Action(Page::_Rewind));
            menu->addAction(Action(Page::_FastForward));
        }

        if(IsLoading())
            menu->addAction(Action(Page::_Stop));
        else
            menu->addAction(Action(Page::_Reload));
    }

    AddContextMenu(menu, elem);
    AddRegularMenu(menu, elem);
    menu->exec(globalPos);
    delete menu;
}

void TridentView::Copy(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_COPY, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

void TridentView::Cut(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_CUT, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

void TridentView::Paste(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_PASTE, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

void TridentView::Undo(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_UNDO, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

void TridentView::Redo(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_REDO, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

void TridentView::SelectAll(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_SELECTALL, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

void TridentView::Unselect(){
    EvaluateJavaScript(QStringLiteral(
                           "(function(){\n"
                           VV"    document.activeElement.blur();\n"
                           VV"    getSelection().removeAllRanges();\n"
                           VV"}());"));
}

void TridentView::Reload(){
    dynamicCall("Refresh()");
}

void TridentView::ReloadAndBypassCache(){
    // not yet implemented.
}

void TridentView::Stop(){
    dynamicCall("Stop()");
}

void TridentView::StopAndUnselect(){
    Stop(); Unselect();
}

void TridentView::Print(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_PRINT, OLECMDEXECOPT_PROMPTUSER, 0, 0);
}

void TridentView::Save(){
    if(m_Interface) m_Interface->ExecWB(OLECMDID_SAVEAS, OLECMDEXECOPT_PROMPTUSER, 0, 0);
}

void TridentView::ZoomIn(){
    if(!m_Interface) return;
    float zoom = PrepareForZoomIn();
    WinI4Variant zoomVariant;
    V_I4(&zoomVariant) = zoom*100;
    m_Interface->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &zoomVariant, 0);
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void TridentView::ZoomOut(){
    if(!m_Interface) return;
    float zoom = PrepareForZoomOut();
    WinI4Variant zoomVariant;
    V_I4(&zoomVariant) = zoom*100;
    m_Interface->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &zoomVariant, 0);
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void TridentView::InspectElement(){
    // not yet implemented.
}

void TridentView::AddSearchEngine(QPoint pos){
    // not yet implemented.
    Q_UNUSED(pos);
}

void TridentView::AddBookmarklet(QPoint pos){
    // not yet implemented.
    Q_UNUSED(pos);
}

void TridentView::OnSignal(const QString &str, int i, void *v){
    Q_UNUSED(str); Q_UNUSED(i); Q_UNUSED(v);
}

void TridentView::OnException(int code, QString source, QString description, QString help){
    Q_UNUSED(code); Q_UNUSED(source); Q_UNUSED(description); Q_UNUSED(help);
    // ignore exception.
}

void TridentView::OnBeforeNavigate(QString str,int,QString,QVariant&,QString,bool&){
    ResetDocument();
    emit urlChanged(QUrl::fromPercentEncoding(str.toLatin1()));
    emit loadStarted();
}

void TridentView::OnBeforeNavigate2(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&){
}

void TridentView::OnBeforeScriptExecute(IDispatch*){
}

void TridentView::OnCommandStateChange(int i, bool b){
    Q_UNUSED(i); Q_UNUSED(b);
}

void TridentView::OnDocumentComplete(IDispatch*, QVariant &var){
    Q_UNUSED(var);
}

void TridentView::OnDownloadBegin(){
}

void TridentView::OnDownloadComplete(){
}

void TridentView::OnFileDownload(bool b, bool &ok){
    Q_UNUSED(b); Q_UNUSED(ok);
}

void TridentView::OnFrameBeforeNavigate(QString,int,QString,QVariant&,QString,bool&){
}

void TridentView::OnFrameNavigateComplete(QString){
}

void TridentView::OnNavigateComplete(QString str){
    ResetDocument();
    emit urlChanged(QUrl::fromPercentEncoding(str.toLatin1()));
    emit loadFinished(true);
}

void TridentView::OnNavigateComplete2(IDispatch*, QVariant &var){
    Q_UNUSED(var);
}

void TridentView::OnNavigateError(IDispatch*,QVariant&,QVariant&,QVariant&,bool&){
}

void TridentView::OnNewWindow(QString,int,QString,QVariant&,QString,bool&){
}

void TridentView::OnNewWindow2(IDispatch**,bool&){
}

void TridentView::OnNewWindow3(IDispatch **disp, bool &ok, uint i, QString referer, QString target){
    Q_UNUSED(disp); Q_UNUSED(i); Q_UNUSED(referer);
    page()->OpenInNew(QUrl(QUrl::fromPercentEncoding(target.toLatin1())));
    ok = true;
}

void TridentView::OnPrivacyImpactedStateChange(bool){
}

void TridentView::OnProgressChange(int i, int j){
    emit loadProgress(j ? 100*i/j : 0);
}

void TridentView::OnPropertyChange(QString str){
    Q_UNUSED(str);
}

void TridentView::OnSetSecureLockIcon(int i){
    Q_UNUSED(i);
}

void TridentView::OnStatusTextChange(QString str){
    if(!str.isEmpty()) emit statusBarMessage(str);
}

void TridentView::OnTitleChange(QString str){
    emit titleChanged(str);
}

void TridentView::OnOnFullScreen(bool){
}

void TridentView::OnOnMenuBar(bool){
}

void TridentView::OnOnQuit(){
}

void TridentView::OnOnStatusBar(bool){
}

void TridentView::OnOnTheaterMode(bool){
}

void TridentView::OnOnToolBar(bool){
}

void TridentView::OnOnVisible(bool){
}

bool TridentView::translateKeyEvent(int message, int keycode) const {
    Q_UNUSED(message); Q_UNUSED(keycode);
    return true;
}

void TridentView::UpdateIcon(const QUrl &iconUrl){
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

void TridentView::hideEvent(QHideEvent *ev){
    SaveViewState();
    QAxWidget::hideEvent(ev);
}

void TridentView::showEvent(QShowEvent *ev){
    QAxWidget::showEvent(ev);
    RestoreViewState();
}

void TridentView::keyPressEvent(QKeyEvent *ev){
    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QAxWidget::keyPressEvent(ev);

    if(!m_HtmlDocument) return;

    QString tag;
    VARIANT_BOOL editable = VARIANT_FALSE;
    IHTMLElement *elem = 0;
    m_HtmlDocument->get_activeElement(&elem);
    if(elem){
        WinString wstr;
        elem->get_tagName(&wstr.data);
        tag = toQt(wstr);

        IHTMLElement3 *elem3 = 0;
        elem->QueryInterface(IID_IHTMLElement3, (void**)&elem3);
        if(elem3){
            elem3->get_isContentEditable(&editable);
            elem3->Release();
        }
        elem->Release();
    }

    if(tag.isEmpty() ||
       (editable == VARIANT_FALSE &&
        tag != QStringLiteral("BUTTON") &&
        tag != QStringLiteral("SELECT") &&
        tag != QStringLiteral("INPUT") &&
        tag != QStringLiteral("TEXTAREA") &&
        !Application::IsOnlyModifier(ev))){

        TriggerKeyEvent(ev);
    }
    ev->setAccepted(true);
}

void TridentView::keyReleaseEvent(QKeyEvent *ev){
    QAxWidget::keyReleaseEvent(ev);
    for(int i = 1; i < 6; i++){
        QTimer::singleShot(i*200, this, &TridentView::EmitScrollChangedIfNeed);
    }
}

void TridentView::resizeEvent(QResizeEvent *ev){
    QAxWidget::resizeEvent(ev);
}

void TridentView::contextMenuEvent(QContextMenuEvent *ev){
    /* when mouse pressed, do nothing(except WebEngineView). */
    ev->setAccepted(true);
}

void TridentView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QAxWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
        return;
    }
    if(ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(LocalPos(ev));
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
}

void TridentView::mousePressEvent(QMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, LocalPos(ev))){
                ev->setAccepted(false);
                qDebug() << "Invalid mouse event: " << str;
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(LocalPos(ev));
    QAxWidget::mousePressEvent(ev);
    ev->setAccepted(true);
}

void TridentView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    if(ev->button() == Qt::RightButton){

        if(!m_Gesture.isEmpty()){
            QPoint pos = LocalPos(ev);
            Qt::MouseButton button = ev->button();
            QTimer::singleShot(0, [this, pos, button](){ GestureFinished(pos, button);});
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            DisplayContextMenu(m_TreeBank, elem, GlobalPos(ev));
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QAxWidget::mouseReleaseEvent(ev);
    EmitScrollChangedIfNeed();
    ev->setAccepted(true);
}

void TridentView::mouseDoubleClickEvent(QMouseEvent *ev){
    QAxWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void TridentView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QAxWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void TridentView::dragMoveEvent(QDragMoveEvent *ev){
    if(m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(LocalPos(ev));
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_LeftGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_LeftGestureMap[gesture])
              ? Action(Page::StringToAction(m_LeftGestureMap[gesture]))->text()
            : m_LeftGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QAxWidget::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void TridentView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    QPoint pos = LocalPos(ev);
    QList<QUrl> urls = ev->mimeData()->urls();
    QObject *source = ev->source();
    SharedWebElement elem = HitElement(pos);
    QWidget *widget = this;

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        QAxWidget::dropEvent(ev);
        ev->setAccepted(true);
        return;
    }

    if(!m_Gesture.isEmpty() && source == widget){
        QTimer::singleShot(0, [this, pos](){ GestureFinished(pos, Qt::LeftButton);});
        ev->setAccepted(true);
        return;
    }

    GestureAborted();

    if(urls.isEmpty() || source == widget){
        // do nothing.
    } else if(qobject_cast<TreeBank*>(source) || dynamic_cast<View*>(source)){
        QList<QUrl> filtered;
        foreach(QUrl u, urls){ if(!u.isLocalFile()) filtered << u;}
        m_TreeBank->OpenInNewViewNode(filtered, true, GetViewNode());
        return;
    } else {
        // foreign drag.
        m_TreeBank->OpenInNewViewNode(ev->mimeData()->urls(), true, GetViewNode());
    }
    ev->setAccepted(true);
}

void TridentView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QAxWidget::dragLeaveEvent(ev);
}

void TridentView::wheelEvent(QWheelEvent *ev){
    QString wheel;
    bool up = ev->delta() > 0;

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(m_MouseMap.contains(wheel)){

        QString str = m_MouseMap[wheel];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, LocalPos(ev)))
                qDebug() << "Invalid mouse event: " << str;
        }
        ev->setAccepted(true);

    } else {
        QWheelEvent *new_ev = new QWheelEvent(ev->pos(),
                                              ev->delta()*Application::WheelScrollRate(),
                                              ev->buttons(),
                                              ev->modifiers(),
                                              ev->orientation());
        QAxWidget::wheelEvent(new_ev);
        ev->setAccepted(true);
        delete new_ev;
    }

    for(int i = 1; i < 6; i++){
        QTimer::singleShot(i*200, this, &TridentView::EmitScrollChangedIfNeed);
    }
}

void TridentView::focusInEvent(QFocusEvent *ev){
    QAxWidget::focusInEvent(ev);
    OnFocusIn();
}

void TridentView::focusOutEvent(QFocusEvent *ev){
    QAxWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool TridentView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QAxWidget::focusNextPrevChild(next);
    return false;
}
