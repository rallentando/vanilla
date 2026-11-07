#include "switch.hpp"
#include "const.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include "spacerename.hpp"

#include <QPointer>
#include <QTimer>
#include <QWindow>

#include <functional>
#include <memory>

namespace {

    class HiddenProfile : public QObject {
    public:
        explicit HiddenProfile(const QString &name, QObject *parent)
            : QObject(parent)
            , m_Name(name)
            , m_Window(new QWindow())
        {
            m_Window->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                               Qt::WindowStaysOnBottomHint | Qt::WindowTransparentForInput);
            m_Window->setGeometry(QRect(-30000, -30000, 8, 8));
        }
        ~HiddenProfile() override {
            Close();
            delete m_Window;
        }

        void Open(std::function<void(ComPtr<ICoreWebView2CookieManager>)> done){
            QPointer<HiddenProfile> self(this);
            m_Window->show();
            const HWND window = reinterpret_cast<HWND>(m_Window->winId());
            const QString name = m_Name;
            ComPtr<ICoreWebView2Environment10> environment;
            ComPtr<ICoreWebView2ControllerOptions> options;
            const ComPtr<ICoreWebView2Environment> base = EdgeEnvironment::Instance()->GetEnvironment();
            HRESULT hr = base ? base.As(&environment) : E_POINTER;
            if(SUCCEEDED(hr)) hr = environment->CreateCoreWebView2ControllerOptions(&options);
            if(SUCCEEDED(hr)) hr = options->put_ProfileName(reinterpret_cast<LPCWSTR>(name.utf16()));
            if(SUCCEEDED(hr)) hr = options->put_IsInPrivateModeEnabled(FALSE);
            if(!self) return;
            if(FAILED(hr)){ done(nullptr); return;}
            hr = environment->CreateCoreWebView2ControllerWithOptions(
                window, options.Get(),
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [self, done](HRESULT hr, ICoreWebView2Controller *raw) -> HRESULT {
                        const ComPtr<ICoreWebView2Controller> controller = raw;
                        if(!self){ if(controller) controller->Close(); return S_OK;}
                        ComPtr<ICoreWebView2> web;
                        ComPtr<ICoreWebView2_2> web2;
                        ComPtr<ICoreWebView2CookieManager> cookies;
                        if(SUCCEEDED(hr) && controller) hr = controller->get_CoreWebView2(&web);
                        if(SUCCEEDED(hr) && web) hr = web.As(&web2);
                        if(SUCCEEDED(hr) && web2) hr = web2->get_CookieManager(&cookies);
                        self->m_Controller = controller;
                        done(SUCCEEDED(hr) ? cookies : nullptr);
                        return S_OK;
                    }).Get());
            if(self && FAILED(hr)) done(nullptr);
        }

        void Close(){
            const ComPtr<ICoreWebView2Controller> controller = std::move(m_Controller);
            if(controller) controller->Close();
        }

    private:
        QString m_Name;
        QWindow *m_Window;
        ComPtr<ICoreWebView2Controller> m_Controller;
    };

    QString Take(LPWSTR text){
        const QString string = text ? QString::fromWCharArray(text) : QString();
        CoTaskMemFree(text);
        return string;
    }

    void ListCookies(ComPtr<ICoreWebView2CookieManager> manager,
                     std::function<void(bool, QList<SpaceRename::Cookie>)> done){
        const HRESULT hr = manager->GetCookies(L"", Callback<ICoreWebView2GetCookiesCompletedHandler>(
            [done](HRESULT hr, ICoreWebView2CookieList *list) -> HRESULT {
                QList<SpaceRename::Cookie> cookies;
                UINT count = 0;
                if(FAILED(hr) || !list || FAILED(list->get_Count(&count))){ done(false, cookies); return S_OK;}
                for(UINT i = 0; i < count; i++){
                    ComPtr<ICoreWebView2Cookie> c;
                    if(FAILED(list->GetValueAtIndex(i, &c)) || !c){ done(false, QList<SpaceRename::Cookie>()); return S_OK;}
                    SpaceRename::Cookie cookie;
                    LPWSTR name = nullptr, value = nullptr, domain = nullptr, path = nullptr;
                    BOOL session = FALSE, httpOnly = FALSE, secure = FALSE;
                    double expires = -1;
                    COREWEBVIEW2_COOKIE_SAME_SITE_KIND sameSite = COREWEBVIEW2_COOKIE_SAME_SITE_KIND_LAX;
                    const bool read =
                        SUCCEEDED(c->get_Name(&name)) && SUCCEEDED(c->get_Value(&value)) &&
                        SUCCEEDED(c->get_Domain(&domain)) && SUCCEEDED(c->get_Path(&path)) &&
                        SUCCEEDED(c->get_IsSession(&session)) && SUCCEEDED(c->get_IsHttpOnly(&httpOnly)) &&
                        SUCCEEDED(c->get_IsSecure(&secure)) && SUCCEEDED(c->get_Expires(&expires)) &&
                        SUCCEEDED(c->get_SameSite(&sameSite));
                    cookie.name = Take(name);
                    cookie.value = Take(value);
                    cookie.domain = Take(domain);
                    cookie.path = Take(path);
                    if(!read){ done(false, QList<SpaceRename::Cookie>()); return S_OK;}
                    cookie.session = session;
                    cookie.httpOnly = httpOnly;
                    cookie.secure = secure;
                    cookie.expires = expires;
                    cookie.sameSite = static_cast<int>(sameSite);
                    cookies << cookie;
                }
                done(true, cookies);
                return S_OK;
            }).Get());
        if(FAILED(hr)) done(false, QList<SpaceRename::Cookie>());
    }

    class CookieCopy : public QObject {
    public:
        CookieCopy(const QString &from, const QString &to, std::function<void(bool, int)> done)
            : QObject(EdgeEnvironment::Instance())
            , m_From(new HiddenProfile(from, this))
            , m_To(new HiddenProfile(to, this))
            , m_Done(done)
        {
            QTimer::singleShot(15000, this, [this](){ Finish(false);});
        }

        void Start(){
            QPointer<CookieCopy> self(this);
            m_From->Open([self](ComPtr<ICoreWebView2CookieManager> from){
                if(!self || self->m_Answered) return;
                if(!from){ self->Finish(false); return;}
                self->m_To->Open([self, from](ComPtr<ICoreWebView2CookieManager> to){
                    if(!self || self->m_Answered) return;
                    if(!to){ self->Finish(false); return;}
                    ListCookies(from, [self, from, to](bool ok, QList<SpaceRename::Cookie> source){
                        if(!self || self->m_Answered) return;
                        if(!ok){ self->Finish(false); return;}
                        ListCookies(to, [self, to, source](bool ok, QList<SpaceRename::Cookie> there){
                            if(!self || self->m_Answered) return;
                            if(!ok){ self->Finish(false); return;}
                            self->Add(to, SpaceRename::CookiesToAdd(source, there));
                        });
                    });
                });
            });
        }

    private:
        void Add(ComPtr<ICoreWebView2CookieManager> to, const QList<SpaceRename::Cookie> &cookies){
            bool ok = true;
            int added = 0;
            foreach(const SpaceRename::Cookie &cookie, cookies){
                ComPtr<ICoreWebView2Cookie> c;
                HRESULT hr = to->CreateCookie(reinterpret_cast<LPCWSTR>(cookie.name.utf16()),
                                              reinterpret_cast<LPCWSTR>(cookie.value.utf16()),
                                              reinterpret_cast<LPCWSTR>(cookie.domain.utf16()),
                                              reinterpret_cast<LPCWSTR>(cookie.path.utf16()), &c);
                if(SUCCEEDED(hr)) hr = c->put_Expires(cookie.expires);
                if(SUCCEEDED(hr)) hr = c->put_IsHttpOnly(cookie.httpOnly);
                if(SUCCEEDED(hr)) hr = c->put_IsSecure(cookie.secure);
                if(SUCCEEDED(hr)) hr = c->put_SameSite(static_cast<COREWEBVIEW2_COOKIE_SAME_SITE_KIND>(cookie.sameSite));
                if(SUCCEEDED(hr)) hr = to->AddOrUpdateCookie(c.Get());
                if(SUCCEEDED(hr)) added++;
                else ok = false;
            }
            Finish(ok, added);
        }

        void Finish(bool ok, int added = 0){
            if(m_Answered) return;
            m_Answered = true;
            QPointer<CookieCopy> self(this);
            QTimer::singleShot(0, EdgeEnvironment::Instance(), [self, ok, added](){
                if(!self) return;
                self->m_From->Close();
                self->m_To->Close();
                const std::function<void(bool, int)> done = self->m_Done;
                self->deleteLater();
                if(done) done(ok, added);
            });
        }

        HiddenProfile *m_From;
        HiddenProfile *m_To;
        std::function<void(bool, int)> m_Done;
        bool m_Answered = false;
    };
}

void EdgeWebView::CopyCookiesForMerge(const QString &fromProfile, const QString &toProfile,
                                      std::function<void(bool, int)> done){
    if(!EdgeEnvironment::Instance() || !EdgeEnvironment::Instance()->GetEnvironment()){
        if(done) done(false, 0);
        return;
    }
    (new CookieCopy(fromProfile, toProfile, done))->Start();
}

#endif
