#include "switch.hpp"
#include "const.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include <QDir>
#include <QTimer>
#include <QWebEngineSettings>
#include <QWebEngineProfile>

#include "application.hpp"

namespace {

    LPWSTR CoString(const QString &value){
        const size_t bytes = (static_cast<size_t>(value.length()) + 1) * sizeof(wchar_t);
        LPWSTR copy = static_cast<LPWSTR>(CoTaskMemAlloc(bytes));
        if(!copy) return nullptr;
        value.toWCharArray(copy);
        copy[value.length()] = L'\0';
        return copy;
    }

    class EdgeSchemeRegistration : public ICoreWebView2CustomSchemeRegistration {

    public:
        EdgeSchemeRegistration(const QString &scheme, const QString &origin)
            : m_Ref(1), m_Scheme(scheme), m_Origin(origin) {}

        ULONG STDMETHODCALLTYPE AddRef() override {
            return static_cast<ULONG>(InterlockedIncrement(&m_Ref));
        }
        ULONG STDMETHODCALLTYPE Release() override {
            const LONG count = InterlockedDecrement(&m_Ref);
            if(!count) delete this;
            return static_cast<ULONG>(count);
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **out) override {
            if(!out) return E_POINTER;
            if(id == IID_IUnknown || id == IID_ICoreWebView2CustomSchemeRegistration){
                *out = static_cast<ICoreWebView2CustomSchemeRegistration*>(this);
                AddRef();
                return S_OK;
            }
            *out = nullptr;
            return E_NOINTERFACE;
        }

        HRESULT STDMETHODCALLTYPE get_SchemeName(LPWSTR *value) override {
            if(!value) return E_POINTER;
            *value = CoString(m_Scheme);
            return *value ? S_OK : E_OUTOFMEMORY;
        }
        HRESULT STDMETHODCALLTYPE get_TreatAsSecure(BOOL *value) override {
            if(!value) return E_POINTER;
            *value = TRUE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE put_TreatAsSecure(BOOL) override { return S_OK;}

        HRESULT STDMETHODCALLTYPE GetAllowedOrigins(UINT32 *count,
                                                    LPWSTR **origins) override {
            if(!count || !origins) return E_POINTER;
            *count = 0;
            *origins = static_cast<LPWSTR*>(CoTaskMemAlloc(sizeof(LPWSTR)));
            if(!*origins) return E_OUTOFMEMORY;
            (*origins)[0] = CoString(m_Origin);
            if(!(*origins)[0]){
                CoTaskMemFree(*origins);
                *origins = nullptr;
                return E_OUTOFMEMORY;
            }
            *count = 1;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE SetAllowedOrigins(UINT32, LPCWSTR*) override {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HasAuthorityComponent(BOOL *value) override {
            if(!value) return E_POINTER;
            *value = TRUE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE put_HasAuthorityComponent(BOOL) override { return S_OK;}

    private:
        LONG m_Ref;
        const QString m_Scheme;
        const QString m_Origin;
    };

    class EdgeEnvironmentOptions : public ICoreWebView2EnvironmentOptions,
                                   public ICoreWebView2EnvironmentOptions4 {

    public:
        EdgeEnvironmentOptions() : m_Ref(1) {}

        ULONG STDMETHODCALLTYPE AddRef() override {
            return static_cast<ULONG>(InterlockedIncrement(&m_Ref));
        }
        ULONG STDMETHODCALLTYPE Release() override {
            const LONG count = InterlockedDecrement(&m_Ref);
            if(!count) delete this;
            return static_cast<ULONG>(count);
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **out) override {
            if(!out) return E_POINTER;
            if(id == IID_IUnknown || id == IID_ICoreWebView2EnvironmentOptions){
                *out = static_cast<ICoreWebView2EnvironmentOptions*>(this);
                AddRef();
                return S_OK;
            }
            if(id == IID_ICoreWebView2EnvironmentOptions4){
                *out = static_cast<ICoreWebView2EnvironmentOptions4*>(this);
                AddRef();
                return S_OK;
            }
            *out = nullptr;
            return E_NOINTERFACE;
        }

        HRESULT STDMETHODCALLTYPE get_AdditionalBrowserArguments(LPWSTR *value) override {
            if(!value) return E_POINTER;
            bool gesture = false;
#ifdef WEBENGINEVIEW
            gesture = QWebEngineProfile::defaultProfile()->settings()
                ->testAttribute(QWebEngineSettings::PlaybackRequiresUserGesture);
#endif
            gesture = Application::GlobalSettings().value
                (QStringLiteral("webview/preferences/PlaybackRequiresUserGesture"),
                 gesture).value<bool>();
            *value = CoString
                (gesture ? QString()
                         : QStringLiteral("--autoplay-policy=no-user-gesture-required"));
            return *value ? S_OK : E_OUTOFMEMORY;
        }
        HRESULT STDMETHODCALLTYPE put_AdditionalBrowserArguments(LPCWSTR) override {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_Language(LPWSTR *value) override {
            return Empty(value);
        }
        HRESULT STDMETHODCALLTYPE put_Language(LPCWSTR) override { return S_OK;}
        HRESULT STDMETHODCALLTYPE get_TargetCompatibleBrowserVersion(LPWSTR *value) override {
            if(!value) return E_POINTER;
            *value = CoString(QStringLiteral("151.0.4129.50"));
            return *value ? S_OK : E_OUTOFMEMORY;
        }
        HRESULT STDMETHODCALLTYPE put_TargetCompatibleBrowserVersion(LPCWSTR) override {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_AllowSingleSignOnUsingOSPrimaryAccount(BOOL *value) override {
            if(!value) return E_POINTER;
            *value = FALSE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE put_AllowSingleSignOnUsingOSPrimaryAccount(BOOL) override {
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetCustomSchemeRegistrations(
            UINT32 *count, ICoreWebView2CustomSchemeRegistration ***registrations) override {
            if(!count || !registrations) return E_POINTER;
            *count = 0;
            *registrations = static_cast<ICoreWebView2CustomSchemeRegistration**>
                (CoTaskMemAlloc(sizeof(ICoreWebView2CustomSchemeRegistration*)));
            if(!*registrations) return E_OUTOFMEMORY;

            EdgeSchemeRegistration *registration = new EdgeSchemeRegistration
                (VANILLA_SCHEME, VANILLA_SCHEME + QStringLiteral("://*"));
            (*registrations)[0] = registration;
            *count = 1;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE SetCustomSchemeRegistrations(
            UINT32, ICoreWebView2CustomSchemeRegistration**) override {
            return S_OK;
        }

    private:
        static HRESULT Empty(LPWSTR *value){
            if(!value) return E_POINTER;
            *value = CoString(QString());
            return *value ? S_OK : E_OUTOFMEMORY;
        }

        LONG m_Ref;
    };
}

EdgeEnvironment::EdgeEnvironment()
    : QObject(Application::GetInstance())
    , m_State(EdgeEnvironmentState())
    , m_Environment(nullptr)
    , m_Waiters(QHash<int, QPointer<EdgeWebView>>())
{
}

EdgeEnvironment *EdgeEnvironment::Instance(){
    static EdgeEnvironment *instance = nullptr;
    if(!instance) instance = new EdgeEnvironment();
    return instance;
}

QString EdgeEnvironment::UserDataFolder(){
    return QDir::toNativeSeparators
        (Application::DataDirectory() + QStringLiteral("edgewebview"));
}

void EdgeEnvironment::Request(int token, EdgeWebView *view){
    if(!token || !view) return;

    m_Waiters.insert(token, QPointer<EdgeWebView>(view));

    switch(m_State.AddWaiter(token)){
    case EdgeEnvironmentState::Effect::StartCreation:
        StartCreation();
        break;
    case EdgeEnvironmentState::Effect::NotifyReady:
        m_Waiters.remove(token);
        QTimer::singleShot(0, view, [view](){ view->EnvironmentReady();});
        break;
    case EdgeEnvironmentState::Effect::NotifyFailed:
    case EdgeEnvironmentState::Effect::None:
        break;
    }
}

void EdgeEnvironment::Forget(int token){
    m_State.RemoveWaiter(token);
    m_Waiters.remove(token);
}

void EdgeEnvironment::StartCreation(){
    const QString folder = UserDataFolder();

    ComPtr<ICoreWebView2EnvironmentOptions> options;
    options.Attach(static_cast<ICoreWebView2EnvironmentOptions*>
                   (new EdgeEnvironmentOptions()));

    const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions
        (nullptr,
         reinterpret_cast<PCWSTR>(folder.utf16()),
         options.Get(),
         Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
         ([this](HRESULT result, ICoreWebView2Environment *environment) -> HRESULT {
             Finish(result, environment);
             return S_OK;
         }).Get());

    if(FAILED(hr)){
        QTimer::singleShot(0, this, [this, hr](){ Finish(hr, nullptr);});
    }
}

void EdgeEnvironment::Finish(HRESULT result, ICoreWebView2Environment *environment){
    const bool ok = SUCCEEDED(result) && environment;
    if(ok) m_Environment = environment;

    const QList<int> waiters = ok
        ? m_State.CreationSucceeded()
        : m_State.CreationFailed();

    foreach(int token, waiters){
        const QPointer<EdgeWebView> view = m_Waiters.take(token);
        if(!view) continue;
        if(ok) view->EnvironmentReady();
        else   view->EnvironmentFailed(result);
    }
}

#endif
