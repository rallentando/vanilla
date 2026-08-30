#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include <QTimer>
#include <QDebug>

#include "treebank.hpp"

static QString UrlFromDataObject(IDataObject *object){
    if(!object) return QString();

    static const CLIPFORMAT inetUrlW =
        static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_INETURLW));
    if(!inetUrlW) return QString();

    FORMATETC format = {};
    format.cfFormat = inetUrlW;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex   = -1;
    format.tymed    = TYMED_HGLOBAL;

    if(object->QueryGetData(&format) != S_OK) return QString();

    STGMEDIUM medium = {};
    if(FAILED(object->GetData(&format, &medium))) return QString();

    QString url;
    if(medium.tymed == TYMED_HGLOBAL && medium.hGlobal){
        if(const void *bytes = GlobalLock(medium.hGlobal)){
            const SIZE_T size = GlobalSize(medium.hGlobal);
            const int chars = static_cast<int>(size / sizeof(wchar_t));
            const wchar_t *text = reinterpret_cast<const wchar_t*>(bytes);
            int length = 0;
            while(length < chars && text[length] != L'\0') length++;
            url = QString::fromWCharArray(text, length);
            GlobalUnlock(medium.hGlobal);
        }
    }
    ReleaseStgMedium(&medium);
    return url;
}

EdgeDragOutLedger s_DragOut;

class EdgeDropSource : public IDropSource {
public:
    explicit EdgeDropSource(int owner)
        : m_Ref(1)
        , m_Owner(owner)
        , m_Buttons(0)
    {
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override {
        if(!object) return E_POINTER;
        if(riid == IID_IUnknown || riid == IID_IDropSource){
            *object = static_cast<IDropSource*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&m_Ref));
    }
    ULONG STDMETHODCALLTYPE Release() override {
        const LONG count = InterlockedDecrement(&m_Ref);
        if(count == 0) delete this;
        return static_cast<ULONG>(count);
    }

    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escapePressed, DWORD keyState) override {
        const DWORD buttons = MK_LBUTTON | MK_RBUTTON | MK_MBUTTON;
        if(m_Buttons == 0){
            m_Buttons = keyState & buttons;
            if(m_Buttons == 0) m_Buttons = MK_LBUTTON;
        }
        if(escapePressed || s_DragOut.IsCancelled()) return DRAGDROP_S_CANCEL;
        if((keyState & m_Buttons) == 0) return DRAGDROP_S_DROP;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override {
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

private:
    ~EdgeDropSource(){}

    LONG m_Ref;
    int m_Owner;
    DWORD m_Buttons;
};

struct EdgeDragOutSession {
    ComPtr<IDataObject> m_Data;
    ComPtr<IDropSource> m_Source;
    ComPtr<ICoreWebView2Deferral> m_Deferral;
    DWORD m_Allowed;
    int m_Owner;
    bool m_Completed;

    EdgeDragOutSession() : m_Allowed(0), m_Owner(0), m_Completed(false) {}

    void CompleteOnce(){
        if(m_Completed) return;
        m_Completed = true;
        if(m_Deferral) m_Deferral->Complete();
    }
};

static void RunDragOut(std::shared_ptr<EdgeDragOutSession> session){
    if(!session) return;

    if(!s_DragOut.IsCancelled() && session->m_Data && session->m_Source){
        DWORD effect = 0;
        DoDragDrop(session->m_Data.Get(), session->m_Source.Get(),
                   session->m_Allowed, &effect);
    }

    s_DragOut.End(session->m_Owner);
    session->CompleteOnce();
}

static quintptr IdentityOf(IDataObject *object){
    if(!object) return 0;
    ComPtr<IUnknown> identity;
    if(FAILED(object->QueryInterface(IID_PPV_ARGS(&identity))) || !identity) return 0;
    return reinterpret_cast<quintptr>(identity.Get());
}

class EdgeDropTarget : public IDropTarget {
public:
    explicit EdgeDropTarget(EdgeWebView *view)
        : m_Ref(1)
        , m_View(view)
    {
    }

    void Detach(){ m_View = nullptr;}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override {
        if(!object) return E_POINTER;
        if(riid == IID_IUnknown || riid == IID_IDropTarget){
            *object = static_cast<IDropTarget*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&m_Ref));
    }
    ULONG STDMETHODCALLTYPE Release() override {
        const LONG count = InterlockedDecrement(&m_Ref);
        if(count == 0) delete this;
        return static_cast<ULONG>(count);
    }

    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *object, DWORD keyState,
                                        POINTL point, DWORD *effect) override {
        ComPtr<IDropTarget> alive(this);
        if(!effect) return E_POINTER;
        const DWORD allowed = *effect;
        *effect = DROPEFFECT_NONE;
        if(!m_View) return S_OK;
        m_View->OnDragEnter(object, keyState, point.x, point.y, allowed, effect);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE DragOver(DWORD keyState, POINTL point,
                                       DWORD *effect) override {
        ComPtr<IDropTarget> alive(this);
        if(!effect) return E_POINTER;
        const DWORD allowed = *effect;
        *effect = DROPEFFECT_NONE;
        if(!m_View) return S_OK;
        m_View->OnDragOver(keyState, point.x, point.y, allowed, effect);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE DragLeave() override {
        ComPtr<IDropTarget> alive(this);
        if(m_View) m_View->OnDragLeave();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Drop(IDataObject *object, DWORD keyState,
                                   POINTL point, DWORD *effect) override {
        ComPtr<IDropTarget> alive(this);
        if(!effect) return E_POINTER;
        const DWORD allowed = *effect;
        *effect = DROPEFFECT_NONE;
        if(!m_View) return S_OK;
        m_View->OnDrop(object, keyState, point.x, point.y, allowed, effect);
        return S_OK;
    }

private:
    ~EdgeDropTarget(){}

    LONG m_Ref;
    EdgeWebView *m_View;
};

void EdgeWebView::RegisterDropTarget(){
    if(!m_Impl->m_Composition || !m_Impl->m_Hwnd ||
       m_Impl->m_State.IsRetired()) return;
    if(m_Impl->m_DropTarget) return;

    ComPtr<ICoreWebView2CompositionController3> drag;
    if(FAILED(m_Impl->m_Composition->QueryInterface(IID_PPV_ARGS(&drag))) || !drag){
        qWarning() << "edge e-3: no ICoreWebView2CompositionController3";
        return;
    }
    VANILLA_STOP_IF_RETIRED();

    ComPtr<IDropTarget> target;
    target.Attach(new EdgeDropTarget(this));

    const HRESULT hr = RegisterDragDrop(m_Impl->m_Hwnd, target.Get());
    if(FAILED(hr)){
        qWarning() << "edge e-3: RegisterDragDrop failed" << Qt::hex << hr;
        return;
    }

    if(m_Impl->m_State.IsRetired()){
        static_cast<EdgeDropTarget*>(target.Get())->Detach();
        RevokeDragDrop(m_Impl->m_Hwnd);
        return;
    }

    m_Impl->m_Drag = drag;
    m_Impl->m_DropTarget = target;
    m_Impl->m_DropTargetWindow = m_Impl->m_Hwnd;
    m_Impl->m_Drop.SetReady(visible());

    RegisterDragStarting();
}

void EdgeWebView::RegisterDragStarting(){
    if(!m_Impl->m_Composition || m_Impl->m_State.IsRetired()) return;

    ComPtr<ICoreWebView2CompositionController5> dragOut;
    if(FAILED(m_Impl->m_Composition->QueryInterface(IID_PPV_ARGS(&dragOut))) || !dragOut){
        m_Impl->m_DragOut.Reset();
        qWarning() << "edge e-3b: no ICoreWebView2CompositionController5";
        return;
    }
    VANILLA_STOP_IF_RETIRED();
    m_Impl->m_DragOut = dragOut;

    EventRegistrationToken token = {};
    if(FAILED(m_Impl->m_DragOut->add_DragStarting
        (Callback<ICoreWebView2DragStartingEventHandler>
         ([this](ICoreWebView2CompositionController*,
                 ICoreWebView2DragStartingEventArgs *args) -> HRESULT {
             if(!args) return S_OK;

             ComPtr<IDataObject> data;
             DWORD allowed = 0;
             if(FAILED(args->get_Data(&data)) || !data) return S_OK;
             if(FAILED(args->get_AllowedDropEffects(&allowed)) || allowed == 0) return S_OK;

             const quintptr key = IdentityOf(data.Get());

             if(!s_DragOut.Begin(m_Impl->m_Token, key)){
                 args->put_Handled(TRUE);
                 return S_OK;
             }

             ComPtr<ICoreWebView2Deferral> deferral;
             if(FAILED(args->GetDeferral(&deferral)) || !deferral){
                 s_DragOut.End(m_Impl->m_Token);
                 return S_OK;
             }

             std::shared_ptr<EdgeDragOutSession> session =
                 std::make_shared<EdgeDragOutSession>();
             session->m_Data = data;
             session->m_Deferral = deferral;
             session->m_Allowed = allowed;
             session->m_Owner = m_Impl->m_Token;
             session->m_Source.Attach(new EdgeDropSource(m_Impl->m_Token));

             if(FAILED(args->put_Handled(TRUE))){
                 s_DragOut.End(m_Impl->m_Token);
                 session->CompleteOnce();
                 return S_OK;
             }

             QTimer::singleShot(0, EdgeEnvironment::Instance(),
                                [session](){ RunDragOut(session);});
             return S_OK;
         }).Get(), &token)))
        return;

    VANILLA_KEEP_COMPOSITION_EVENT
        (m_Impl->m_DragOut.Get(), DragStarting, token);
}

void EdgeWebView::RevokeDropTarget(){

    AbandonDrag();

    if(m_Impl->m_DropTarget){
        static_cast<EdgeDropTarget*>(m_Impl->m_DropTarget.Get())->Detach();
        if(m_Impl->m_DropTargetWindow) RevokeDragDrop(m_Impl->m_DropTargetWindow);
    }
    m_Impl->m_DropTargetWindow = nullptr;
    m_Impl->m_DropTarget.Reset();
    m_Impl->m_Drag.Reset();
    m_Impl->m_Drop.SetReady(false);
}

void EdgeWebView::AbandonDrag(){
    if(m_Impl->m_Drop.Abandon() != EdgeDropLedger::Send::ToBackend) return;
    if(m_Impl && m_Impl->m_Drag) m_Impl->m_Drag->DragLeave();
}

bool EdgeWebView::DragPointOf(long screenX, long screenY, long *outX, long *outY) const {
    if(!outX || !outY || !m_Impl->m_DropTargetWindow) return false;
    POINT p = {static_cast<LONG>(screenX), static_cast<LONG>(screenY)};
    if(!ScreenToClient(m_Impl->m_DropTargetWindow, &p)) return false;
    *outX = p.x;
    *outY = p.y;
    return true;
}

void EdgeWebView::OnDragEnter(IDataObject *object, unsigned long keyState,
                              long screenX, long screenY,
                              unsigned long allowed, unsigned long *effect){
    *effect = DROPEFFECT_NONE;
    m_Impl->m_Drop.SetReady(m_Impl->m_Drag && visible());

    const bool own = s_DragOut.IsActive()
        ? s_DragOut.Matches(IdentityOf(object))
        : GetCapture() != nullptr;
    const QString url = UrlFromDataObject(object);
    const EdgeDropLedger::Take take = url.isEmpty()
        ? EdgeDropLedger::Take::Forward
        : own ? EdgeDropLedger::Take::ForwardOwnUrl
              : EdgeDropLedger::Take::Consume;

    if(m_Impl->m_Drop.Enter(take) != EdgeDropLedger::Send::ToBackend){
        if(m_Impl->m_Drop.IsConsuming())
            *effect = (allowed & DROPEFFECT_LINK) ? DROPEFFECT_LINK
                    : (allowed & DROPEFFECT_COPY) ? DROPEFFECT_COPY
                    : DROPEFFECT_NONE;
        return;
    }

    POINT p = {};
    if(!DragPointOf(screenX, screenY, &p.x, &p.y)){
        AbandonDrag();
        return;
    }
    DWORD answer = static_cast<DWORD>(allowed);
    if(FAILED(m_Impl->m_Drag->DragEnter(object, static_cast<DWORD>(keyState), p, &answer))){
        AbandonDrag();
        return;
    }
    *effect = answer;
}

void EdgeWebView::OnDragOver(unsigned long keyState, long screenX, long screenY,
                             unsigned long allowed, unsigned long *effect){
    *effect = DROPEFFECT_NONE;

    if(m_Impl->m_Drop.Over() != EdgeDropLedger::Send::ToBackend){
        if(m_Impl->m_Drop.IsConsuming())
            *effect = (allowed & DROPEFFECT_LINK) ? DROPEFFECT_LINK
                    : (allowed & DROPEFFECT_COPY) ? DROPEFFECT_COPY
                    : DROPEFFECT_NONE;
        return;
    }

    POINT p = {};
    if(!DragPointOf(screenX, screenY, &p.x, &p.y)) return;
    DWORD answer = static_cast<DWORD>(allowed);
    if(FAILED(m_Impl->m_Drag->DragOver(static_cast<DWORD>(keyState), p, &answer))) return;
    *effect = answer;
}

void EdgeWebView::OnDragLeave(){
    if(m_Impl->m_Drop.Leave() != EdgeDropLedger::Send::ToBackend) return;
    if(m_Impl && m_Impl->m_Drag) m_Impl->m_Drag->DragLeave();
}

void EdgeWebView::OnDrop(IDataObject *object, unsigned long keyState,
                         long screenX, long screenY,
                         unsigned long allowed, unsigned long *effect){
    *effect = DROPEFFECT_NONE;

    const bool consuming = m_Impl->m_Drop.IsConsuming();
    const bool ownUrl = m_Impl->m_Drop.IsForwardingOwnUrl();
    const QString url = (consuming || ownUrl) ? UrlFromDataObject(object) : QString();

    const EdgeDropLedger::Send send = m_Impl->m_Drop.Drop();

    if(send == EdgeDropLedger::Send::ToBackend){
        POINT p = {};
        if(!DragPointOf(screenX, screenY, &p.x, &p.y)){
            if(m_Impl && m_Impl->m_Drag) m_Impl->m_Drag->DragLeave();
            return;
        }

        const bool arming = ownUrl && !url.isEmpty();
        if(arming)
            m_Impl->m_DropEcho.Arm(QUrl(url).adjusted(QUrl::StripTrailingSlash),
                           m_Impl->m_DropEchoClock.elapsed());

        DWORD answer = static_cast<DWORD>(allowed);
        if(FAILED(m_Impl->m_Drag->Drop(object, static_cast<DWORD>(keyState), p, &answer))){
            if(arming) m_Impl->m_DropEcho.Disarm();
            return;
        }
        *effect = answer;
        return;
    }

    if(!consuming || url.isEmpty()) return;

    *effect = (allowed & DROPEFFECT_LINK) ? DROPEFFECT_LINK
            : (allowed & DROPEFFECT_COPY) ? DROPEFFECT_COPY
            : DROPEFFECT_NONE;

    const QUrl opened = QUrl::fromUserInput(url);
    if(!opened.isValid() || !m_TreeBank || !GetViewNode()) return;
    m_TreeBank->OpenInNewViewNode(QList<QUrl>() << opened, true, GetViewNode());
}

#endif
