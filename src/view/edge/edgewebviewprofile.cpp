#include "switch.hpp"
#include "const.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include "treebank.hpp"

#include <QTimer>
#include <QDebug>
#ifdef WEBENGINEVIEW
#  include <QWebEngineSettings>
#  include <QWebEngineProfile>
#endif

#include "directorypage.hpp"
#include "page.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "theme.hpp"

void EdgeWebView::Suspend(){
    if(HiddenViewsStayActive()) return;
    if(!m_Impl->m_WebView) return;

    QPointer<EdgeWebView> alive(this);
    QTimer::singleShot(0, this, [alive](){
        if(!alive || !alive->m_Impl || !alive->m_Impl->m_WebView) return;

        if(!EdgeSuspendPolicy::ShouldSuspend(HiddenViewsStayActive(),
                                             alive->visible(),
                                             alive->RecentlyAudible(),
                                             alive->IsLoading(),
                                             alive->m_Impl->m_Suspended)) return;

        ComPtr<ICoreWebView2_3> webview3;
        if(FAILED(alive->m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview3))) ||
           !webview3) return;

        QPointer<EdgeWebView> inner(alive);
        webview3->TrySuspend
            (Callback<ICoreWebView2TrySuspendCompletedHandler>
             ([inner](HRESULT result, BOOL suspended) -> HRESULT {
                 if(!inner) return S_OK;
                 inner->m_Impl->m_Suspended = SUCCEEDED(result) && suspended;
                 if(EdgeSuspendPolicy::ShouldResumeAtOnce(inner->m_Impl->m_Suspended,
                                                          inner->visible()))
                     inner->WakeUp();
                 return S_OK;
             }).Get());
    });
}

void EdgeWebView::WakeUp(){
    if(!m_Impl->m_Suspended || !m_Impl->m_WebView) return;

    ComPtr<ICoreWebView2_3> webview3;
    if(FAILED(m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview3))) || !webview3)
        return;
    m_Impl->m_Suspended = false;
    webview3->Resume();
}

QString EdgeWebView::ProfileKey() const {
    if(!m_Impl->m_ProfileMeasured)
        return m_Impl->m_ProfileName + QStringLiteral("|unmeasured#") + QString::number(m_Impl->m_Token);
    return m_Impl->m_ActualProfileName +
        (m_Impl->m_ActualPrivate ? QStringLiteral("|private") : QString());
}

bool EdgeWebView::MeasureProfile(ICoreWebView2Controller *controller,
                                 QString *name, bool *isPrivate){
    if(!controller || !name || !isPrivate) return false;

    ComPtr<ICoreWebView2> webview;
    ComPtr<ICoreWebView2_13> webview13;
    ComPtr<ICoreWebView2Profile> profile;
    if(FAILED(controller->get_CoreWebView2(&webview)) || !webview ||
       FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview13))) || !webview13 ||
       FAILED(webview13->get_Profile(&profile)) || !profile) return false;

    BOOL priv = FALSE;
    if(FAILED(profile->get_IsInPrivateModeEnabled(&priv))) return false;

    LPWSTR raw = nullptr;
    if(FAILED(profile->get_ProfileName(&raw)) || !raw) return false;
    *name = QString::fromWCharArray(raw);
    CoTaskMemFree(raw);
    *isPrivate = priv == TRUE;
    return true;
}

void EdgeWebView::ReportPrivateWipeFailure(){
    emit statusBarMessage
        (tr("This private view cannot start empty and will not open a page."));
}

void EdgeWebView::SettlePrivateWipe(const QString &key, bool ok){
    const QList<QPointer<EdgeWebView>> waiters = s_Profiles.SettlePrivateWipe(key, ok);

    if(!ok) qWarning() << "edge: the private profile" << key << "could not be emptied";

    foreach(const QPointer<EdgeWebView> &waiter, waiters){
        if(waiter.isNull() || waiter->m_Impl->m_State.IsRetired()) continue;
        if(ok) waiter->TryFlushPendingNavigation();
        else waiter->ReportPrivateWipeFailure();
    }
}

bool EdgeWebView::ClearBrowsingData(unsigned int kinds,
                                    std::function<void(bool)> completed){
    if(!m_Impl->m_WebView) return false;

    ComPtr<ICoreWebView2_13> webview13;
    ComPtr<ICoreWebView2Profile> profile;
    ComPtr<ICoreWebView2Profile2> profile2;

    if(FAILED(m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview13))) || !webview13 ||
       FAILED(webview13->get_Profile(&profile)) || !profile ||
       FAILED(profile->QueryInterface(IID_PPV_ARGS(&profile2))) || !profile2) return false;

    return SUCCEEDED(profile2->ClearBrowsingData
        (static_cast<COREWEBVIEW2_BROWSING_DATA_KINDS>(kinds),
         Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>
         ([completed](HRESULT result) -> HRESULT {
             if(completed) completed(SUCCEEDED(result));
             return S_OK;
         }).Get()));
}

bool EdgeWebView::DeleteAllCookies(){
    if(!m_Impl->m_WebView) return false;

    ComPtr<ICoreWebView2_2> webview2;
    ComPtr<ICoreWebView2CookieManager> cookies;
    if(FAILED(m_Impl->m_WebView.As(&webview2)) || !webview2 ||
       FAILED(webview2->get_CookieManager(&cookies)) || !cookies) return false;

    return SUCCEEDED(cookies->DeleteAllCookies());
}

void EdgeWebView::ClearOncePerProfile(unsigned int kinds,
                                      EdgeCookieClearLedger *ledger){
    EdgeClearRoster roster;
    foreach(EdgeWebView *view, s_Profiles.LiveViews()){
        if(!view) continue;
        const QString key = view->ProfileKey();
        if(!roster.ShouldAsk(key)) continue;

        if(!ledger){
            roster.Asked(key, view->ClearBrowsingData(kinds));
            continue;
        }

        QPointer<EdgeWebView> weak(view);
        ledger->AskPlaced();
        const bool placed = view->ClearBrowsingData
            (kinds, [ledger, key, weak](bool ok){
                if(!ok && weak) ok = weak->DeleteAllCookies();
                if(!ok) qWarning() << "edge: cookies remain in profile" << key;
                ledger->AskSettled(key, ok);
            });

        if(!placed){
            const bool ok = view->DeleteAllCookies();
            if(!ok) qWarning() << "edge: cookies remain in profile" << key;
            ledger->AskSettled(key, ok);
            roster.Asked(key, ok);
            continue;
        }
        roster.Asked(key, true);
    }
}

void EdgeWebView::ClearCookies(){
    s_Profiles.Cookies().ClearStarted();
    ClearOncePerProfile(COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES, &s_Profiles.Cookies());
}

void EdgeWebView::ClearHttpCache(){
    ClearOncePerProfile(COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE |
                        COREWEBVIEW2_BROWSING_DATA_KINDS_CACHE_STORAGE);
}

void EdgeWebView::ClearVisitedLinks(){
    ClearOncePerProfile(COREWEBVIEW2_BROWSING_DATA_KINDS_BROWSING_HISTORY);
}

void EdgeWebView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);
    m_Impl->m_SpecificSet = set;
    ApplyPageSettings();
    ApplyUserAgent();

    if(m_Impl->m_PrivateMode != DirectoryPage::SaysPrivate(set))
        RebuildForOffTheRecord();
}

void EdgeWebView::ApplyPageSettings(){
    if(!m_Impl->m_WebView) return;

    ComPtr<ICoreWebView2Settings> settings;
    if(FAILED(m_Impl->m_WebView->get_Settings(&settings)) || !settings) return;

    bool script = true;
    bool errorPage = true;
#ifdef WEBENGINEVIEW
    QWebEngineSettings *g = QWebEngineProfile::defaultProfile()->settings();
    script    = g->testAttribute(QWebEngineSettings::JavascriptEnabled);
    errorPage = g->testAttribute(QWebEngineSettings::ErrorPageEnabled);
#endif

    static const QString token = QStringLiteral("[jJ](?:ava)?[sS](?:cript)?");
    const int state = DirectoryPage::StateIn(m_Impl->m_SpecificSet, token);
    if(state != -1) script = state == 1;

    settings->put_IsScriptEnabled(script ? TRUE : FALSE);
    settings->put_IsBuiltInErrorPageEnabled(errorPage ? TRUE : FALSE);
    settings->put_AreDevToolsEnabled(TRUE);

}

QString BlankBackgroundJsCode(const QColor &color){
    return QStringLiteral(
        "(function(){"
        " if(location.href === 'about:blank')"
        "     document.documentElement.style.backgroundColor = 'rgb(%1,%2,%3)';"
        "})();")
        .arg(color.red()).arg(color.green()).arg(color.blue());
}

void EdgeWebView::ApplyTheme(){
    const QUrl uri = url();
    const bool blank = uri.isEmpty() || uri == BLANK_URL ||
        uri.scheme() == VANILLA_SCHEME;
    m_Impl->m_BaseBackgroundColor = blank ? Theme::Color(Theme::PageBackground)
                                  : QColor(Qt::white);
    ApplyThemeToBackend();

    if(uri == BLANK_URL && m_Impl && m_Impl->m_WebView)
        RunScript(BlankBackgroundJsCode(m_Impl->m_BaseBackgroundColor));
}

void EdgeWebView::ApplyThemeToBackend(){
    if(!m_Impl->m_Controller) return;

    ComPtr<ICoreWebView2Controller2> controller2;
    if(SUCCEEDED(m_Impl->m_Controller->QueryInterface(IID_PPV_ARGS(&controller2))) &&
       controller2 && m_Impl->m_BaseBackgroundColor.isValid()){
        COREWEBVIEW2_COLOR color = {};
        color.A = 255;
        color.R = static_cast<BYTE>(m_Impl->m_BaseBackgroundColor.red());
        color.G = static_cast<BYTE>(m_Impl->m_BaseBackgroundColor.green());
        color.B = static_cast<BYTE>(m_Impl->m_BaseBackgroundColor.blue());
        controller2->put_DefaultBackgroundColor(color);
    }

    if(!m_Impl->m_WebView) return;
    ComPtr<ICoreWebView2_13> webview13;
    if(FAILED(m_Impl->m_WebView->QueryInterface(IID_PPV_ARGS(&webview13))) ||
       !webview13) return;
    ComPtr<ICoreWebView2Profile> profile;
    if(FAILED(webview13->get_Profile(&profile)) || !profile) return;

    const QString value = Application::ColorScheme().trimmed().toLower();
    profile->put_PreferredColorScheme
        (value == QStringLiteral("dark")  ? COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK
       : value == QStringLiteral("light") ? COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT
       :                                    COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO);
}

void EdgeWebView::ApplyUserAgent(){
    if(!m_Impl->m_WebView) return;
    if(!page() || !page()->GetNetworkAccessManager()) return;

    const QString ua = page()->GetNetworkAccessManager()->GetUserAgent();
    if(ua.isEmpty()) return;

    ComPtr<ICoreWebView2Settings2> settings2;
    ComPtr<ICoreWebView2Settings> settings;
    if(FAILED(m_Impl->m_WebView->get_Settings(&settings)) || !settings) return;
    if(FAILED(settings->QueryInterface(IID_PPV_ARGS(&settings2))) || !settings2) return;
    settings2->put_UserAgent(reinterpret_cast<PCWSTR>(ua.utf16()));
}

#endif
