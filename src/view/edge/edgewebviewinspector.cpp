#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"

#include <QTimer>
#include <QDockWidget>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QShowEvent>
#include <QPlatformSurfaceEvent>
#include <QElapsedTimer>
#include <climits>
#include <cstdio>

#include "treebank.hpp"
#include "mainwindow.hpp"

bool EdgeInspectorTraceOn(){
    static const bool on = qEnvironmentVariableIsSet("VANILLA_EDGE_INSPECTOR_TRACE");
    return on;
}
static void EdgeInspectorTrace(const char *what, const QWidget *pane, WId id){
    if(!EdgeInspectorTraceOn()) return;
    const HWND paneHwnd = pane ? reinterpret_cast<HWND>(pane->internalWinId()) : nullptr;
    const HWND hwnd = reinterpret_cast<HWND>(id);
    const bool alive = hwnd && ::IsWindow(hwnd);
    const LONG_PTR style = alive ? ::GetWindowLongPtr(hwnd, GWL_STYLE) : 0;
    RECT r = {0, 0, 0, 0};
    if(alive) ::GetWindowRect(hwnd, &r);
    fprintf(stderr, "edge-inspector: %-22s pane=%p paneParent=%p foreign=%p GetParent=%p child=%d visible=%d alive=%d at=(%ld,%ld)\n",
            what, static_cast<void*>(paneHwnd),
            static_cast<void*>(paneHwnd ? ::GetParent(paneHwnd) : nullptr),
            static_cast<void*>(hwnd), static_cast<void*>(alive ? ::GetParent(hwnd) : nullptr),
            alive && (style & WS_CHILD) ? 1 : 0, alive && (style & WS_VISIBLE) ? 1 : 0, alive ? 1 : 0,
            r.left, r.top);
    fflush(stderr);
}

class EdgeInspectorPane : public QWidget {
public:
    EdgeInspectorPane(WId hwnd, int captionHeight)
        : QWidget(nullptr)
        , m_Hwnd(hwnd)
        , m_CaptionHeight(captionHeight)
        , m_State(State::Orphaned)
        , m_HiddenByPane(false)
        , m_Generation(0)
        , m_AttachRetries(0)
        , m_ScreenPosition(INT_MIN, INT_MIN)
    {
        setObjectName(QStringLiteral("EdgeInspectorPane"));
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_DontCreateNativeAncestors);
        setFocusPolicy(Qt::StrongFocus);
        winId();
        Attach();
        if(qEnvironmentVariableIsSet("VANILLA_EDGE_INSPECTOR_SELFTEST")) ScheduleSelfTest();
    }
    ~EdgeInspectorPane(){
        Detach(EdgeWebView::InspectorRelease::FrontendClosed);
    }

    void Detach(EdgeWebView::InspectorRelease why){
        m_Generation++;
        if(m_State == State::Released) return;
        m_State = State::Released;
        const HWND hwnd = reinterpret_cast<HWND>(m_Hwnd);
        m_Hwnd = 0;
        EdgeInspectorTrace("Detach before", this, reinterpret_cast<WId>(hwnd));
        const bool alive = hwnd && ::IsWindow(hwnd);
        if(alive)
            ::SetWindowPos(hwnd, nullptr, -32000, -32000, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        DropWrapper(true);
        if(!alive) return;
        switch(why){
        case EdgeWebView::InspectorRelease::Close:
            ::ShowWindow(hwnd, SW_HIDE);
            ::PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case EdgeWebView::InspectorRelease::FrontendClosed:
            ::ShowWindow(hwnd, SW_HIDE);
            break;
        case EdgeWebView::InspectorRelease::GiveBack:
            if(m_ScreenPosition.x() != INT_MIN){
                ::SetWindowPos(hwnd, nullptr, m_ScreenPosition.x(), m_ScreenPosition.y(), 0, 0,
                               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                ::ShowWindow(hwnd, SW_SHOWNOACTIVATE);
            } else {
                ::ShowWindow(hwnd, SW_HIDE);
            }
            break;
        }
        m_HiddenByPane = false;
        EdgeInspectorTrace("Detach after", this, reinterpret_cast<WId>(hwnd));
    }

    bool FrontendClosed(){
        if(m_State == State::Released) return true;
        const HWND hwnd = reinterpret_cast<HWND>(m_Hwnd);
        if(!hwnd || !::IsWindow(hwnd)) return true;
        if(m_State == State::Orphaned){
            if(m_OrphanedSince.isValid() && m_OrphanedSince.elapsed() > 1000)
                EdgeInspectorTrace("Orphaned > 1s", this, m_Hwnd);
            if(++m_AttachRetries > 20){
                EdgeInspectorTrace("Orphaned: giving up", this, m_Hwnd);
                return true;
            }
            Attach();
            return false;
        }
        if(m_HiddenByPane) return false;
        return !(::GetWindowLongPtr(hwnd, GWL_STYLE) & WS_VISIBLE);
    }

    static int MeasureCaptionHeight(HWND hwnd, int limit = 120){
        RECT r;
        if(!::GetWindowRect(hwnd, &r)) return 0;
        const int x = (r.left + r.right) / 2;
        for(int y = 0; y < limit; y++){
            DWORD_PTR answer = 0;
            const LPARAM at = MAKELPARAM(x, r.top + y);
            if(!::SendMessageTimeoutW(hwnd, WM_NCHITTEST, 0, at,
                                      SMTO_ABORTIFHUNG, 200, &answer))
                return 0;
            if(static_cast<LRESULT>(answer) == HTCLIENT) return y;
        }
        return 0;
    }

protected:
    bool event(QEvent *ev) Q_DECL_OVERRIDE {
        switch(ev->type()){
        case QEvent::ParentAboutToChange: EdgeInspectorTrace("ParentAboutToChange", this, m_Hwnd); break;
        case QEvent::ParentChange:
            EdgeInspectorTrace("ParentChange", this, m_Hwnd);
            WatchDock();
            break;
        case QEvent::WinIdChange:
            EdgeInspectorTrace("WinIdChange", this, m_Hwnd);
            if(internalWinId()) ScheduleAttach();
            break;
        case QEvent::Show:
            EdgeInspectorTrace("Show", this, m_Hwnd);
            if(internalWinId()) ScheduleAttach();
            break;
        case QEvent::Hide:                EdgeInspectorTrace("Hide", this, m_Hwnd); break;
        default: break;
        }
        return QWidget::event(ev);
    }
    bool eventFilter(QObject *watched, QEvent *ev) Q_DECL_OVERRIDE {
        if(watched == m_Dock.data() &&
           (ev->type() == QEvent::Move || ev->type() == QEvent::Resize ||
            ev->type() == QEvent::LayoutRequest || ev->type() == QEvent::Show)){
            if(!m_SyncPending){
                m_SyncPending = true;
                QTimer::singleShot(0, this, [this](){
                    m_SyncPending = false;
                    SyncNativeGeometry();
                });
            }
        }
        if(watched == m_Foreign.data() && ev->type() == QEvent::PlatformSurface &&
           static_cast<QPlatformSurfaceEvent*>(ev)->surfaceEventType()
               == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed){
            EdgeInspectorTrace("SurfaceAboutToBeDestroyed", this, m_Hwnd);
            Orphan();
        }
        return QWidget::eventFilter(watched, ev);
    }
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE {
        QWidget::resizeEvent(ev);
        SyncNativeGeometry();
        Place();
    }
    void moveEvent(QMoveEvent *ev) Q_DECL_OVERRIDE {
        QWidget::moveEvent(ev);
        SyncNativeGeometry();
    }
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE {
        QWidget::showEvent(ev);
        SyncNativeGeometry();
    }

private:
    enum class State { Attached, Orphaned, Released };

    void SyncNativeGeometry(){
        if(!internalWinId() || !isVisible()) return;
        QWidget *top = window();
        if(!top || top == this || !top->internalWinId()) return;
        const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
        const HWND topHwnd = reinterpret_cast<HWND>(top->internalWinId());
        if(::GetParent(hwnd) != topHwnd) return;
        const QPoint offset = parentWidget() ? parentWidget()->mapTo(top, pos()) : pos();
        if(QWindow *self = windowHandle()){
            if(self->parent() != top->windowHandle()){
                EdgeInspectorTrace("window parent corrected", this, m_Hwnd);
                self->setParent(top->windowHandle());
            }
            self->setGeometry(QRect(offset, size()));
        }
        const qreal dpr = top->devicePixelRatioF();
        const int x = qRound(offset.x() * dpr), y = qRound(offset.y() * dpr);
        const int w = qRound(width() * dpr), h = qRound(height() * dpr);
        RECT r;
        if(!::GetWindowRect(hwnd, &r)) return;
        POINT at = {r.left, r.top};
        ::ScreenToClient(topHwnd, &at);
        if(at.x == x && at.y == y && r.right - r.left == w && r.bottom - r.top == h) return;
        if(EdgeInspectorTraceOn())
            fprintf(stderr, "edge-inspector: geometry corrected (%ld,%ld %ldx%ld) -> (%d,%d %dx%d) [physical]\n",
                    at.x, at.y, r.right - r.left, r.bottom - r.top, x, y, w, h);
        ::SetWindowPos(hwnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void Attach(){
        if(m_State != State::Orphaned) return;
        const HWND hwnd = reinterpret_cast<HWND>(m_Hwnd);
        if(!hwnd || !::IsWindow(hwnd)) return;
        if(!internalWinId() || !windowHandle()) return;
        QWindow *wrapper = QWindow::fromWinId(m_Hwnd);
        if(!wrapper){
            EdgeInspectorTrace("fromWinId failed", this, m_Hwnd);
            return;
        }
        m_Foreign = wrapper;
        wrapper->installEventFilter(this);
        wrapper->setParent(windowHandle());
        m_State = State::Attached;
        m_OrphanedSince.invalidate();
        m_AttachRetries = 0;
        Place();
        wrapper->setVisible(true);
        m_HiddenByPane = false;
        EdgeInspectorTrace("attached", this, m_Hwnd);
        SyncNativeGeometry();
    }

    void Orphan(){
        if(m_State != State::Attached) return;
        m_State = State::Orphaned;
        m_OrphanedSince.start();
        m_Generation++;
        const HWND hwnd = reinterpret_cast<HWND>(m_Hwnd);
        if(hwnd && ::IsWindow(hwnd)){
            ::SetWindowPos(hwnd, nullptr, -32000, -32000, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            ::ShowWindow(hwnd, SW_HIDE);
            m_HiddenByPane = true;
        }
        DropWrapper(false);
        EdgeInspectorTrace("orphaned", this, m_Hwnd);
        ScheduleAttach();
    }

    void DropWrapper(bool now){
        QWindow *wrapper = m_Foreign.data();
        m_Foreign.clear();
        if(!wrapper) return;
        wrapper->removeEventFilter(this);
        if(now){
            wrapper->setParent(nullptr);
            delete wrapper;
        } else {
            wrapper->deleteLater();
        }
    }

    void WatchDock(){
        QDockWidget *dock = qobject_cast<QDockWidget*>(parentWidget());
        if(dock == m_Dock.data()) return;
        if(m_Dock){
            disconnect(m_Dock.data(), nullptr, this, nullptr);
            m_Dock->removeEventFilter(this);
        }
        m_Dock = dock;
        if(!dock) return;
        dock->installEventFilter(this);
        connect(dock, &QDockWidget::topLevelChanged, this, [this, dock](bool floating){
            if(floating) return;
            const QPointer<QDockWidget> expected(dock);
            QTimer::singleShot(0, this, [this, expected](){
                if(expected && m_Dock == expected && expected->widget() == this) RemakeWindow();
            });
        });
    }
    void RemakeWindow(){
        if(m_State == State::Released) return;
        QDockWidget *dock = qobject_cast<QDockWidget*>(parentWidget());
        if(!dock || dock->isFloating() || !internalWinId()) return;
        EdgeInspectorTrace("remake after dock-in", this, m_Hwnd);
        const bool shown = isVisible();
        destroy(true, true);
        winId();
        if(shown) show();
    }

    void ScheduleAttach(){
        const int generation = ++m_Generation;
        QTimer::singleShot(0, this, [this, generation](){
            if(generation == m_Generation) Attach();
        });
    }

    void RememberScreenPosition(){
        RECT r;
        if(internalWinId() && ::GetWindowRect(reinterpret_cast<HWND>(internalWinId()), &r))
            m_ScreenPosition = QPoint(r.left, r.top);
    }

    void Place(){
        if(m_State != State::Attached || !m_Foreign) return;
        RememberScreenPosition();
        const int cap = qRound(m_CaptionHeight / devicePixelRatioF());
        m_Foreign->setGeometry(0, -cap, width(), height() + cap);
    }

    void ScheduleSelfTest(){
        QTimer::singleShot(3000, this, [this](){
            EdgeInspectorTrace("selftest: wrapper destroy()", this, m_Hwnd);
            if(m_Foreign) m_Foreign->destroy();
        });
        QTimer::singleShot(6000, this, [this](){
            EdgeInspectorTrace("selftest: pane destroy/create", this, m_Hwnd);
            const bool shown = isVisible();
            destroy(true, true);
            create();
            if(shown) show();
        });
    }

    WId m_Hwnd;
    QPointer<QWindow> m_Foreign;
    int m_CaptionHeight;
    State m_State;
    bool m_HiddenByPane;
    int m_Generation;
    int m_AttachRetries;
    QElapsedTimer m_OrphanedSince;
    QPoint m_ScreenPosition;
    QPointer<QDockWidget> m_Dock;
    bool m_SyncPending = false;
};

void EdgeWebView::InspectElement(){
    if(!m_Impl->m_WebView) return;

    MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr;

    if(m_Impl->m_InspectorPane && ::IsWindow(reinterpret_cast<HWND>(m_Impl->m_InspectorWinId))){
        if(win && InspectorPane()){
            win->SetInspectorPane(m_Impl->m_InspectorPane, true);
            return;
        }
        ReleaseInspector(InspectorRelease::GiveBack);
    }

    UINT32 pid = 0;
    if(SUCCEEDED(m_Impl->m_WebView->get_BrowserProcessId(&pid)) && pid)
        AllowSetForegroundWindow(pid);

    const bool adopt = win && !TreeBank::PurgeView() && InspectorInMainWindow() && pid;
    const bool probing = m_Impl->m_InspectorProbe && m_Impl->m_InspectorProbe->isActive();
    if(adopt && !probing){
        m_Impl->m_InspectorBefore.clear();
        struct Ctx { UINT32 pid; QSet<WId> *set; } ctx{pid, &m_Impl->m_InspectorBefore};
        ::EnumWindows([](HWND h, LPARAM lp) -> BOOL {
            auto *c = reinterpret_cast<Ctx*>(lp);
            DWORD p = 0; ::GetWindowThreadProcessId(h, &p);
            if(p == c->pid && ::IsWindowVisible(h))
                c->set->insert(reinterpret_cast<WId>(h));
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
    }

    if(FAILED(m_Impl->m_WebView->OpenDevToolsWindow())){
        emit statusBarMessage(tr("The developer tools could not be opened."));
        return;
    }
    if(!adopt || m_Impl->m_State.IsRetired() || !m_Impl->m_WebView) return;

    if(!m_Impl->m_InspectorProbe){
        m_Impl->m_InspectorProbe = new QTimer(this);
        m_Impl->m_InspectorProbe->setInterval(50);
        connect(m_Impl->m_InspectorProbe, &QTimer::timeout,
                this, &EdgeWebView::ProbeInspectorWindow);
    }
    m_Impl->m_InspectorProbeTicks = 0;
    m_Impl->m_InspectorProbe->start();
}

QWidget *EdgeWebView::InspectorPane(){
    if(!m_Impl->m_InspectorPane || TreeBank::PurgeView() || !InspectorInMainWindow())
        return nullptr;
    return m_Impl->m_InspectorPane;
}

void EdgeWebView::ProbeInspectorWindow(){
    if(!m_Impl->m_WebView || m_Impl->m_State.IsRetired()){
        m_Impl->m_InspectorProbe->stop();
        return;
    }
    ComPtr<ICoreWebView2> webview = m_Impl->m_WebView;
    UINT32 pid = 0;
    const HRESULT hr = webview->get_BrowserProcessId(&pid);
    if(FAILED(hr) || !pid || m_Impl->m_State.IsRetired() || !m_Impl->m_WebView){
        m_Impl->m_InspectorProbe->stop();
        return;
    }
    struct Ctx { UINT32 pid; const QSet<WId> *before; HWND found; } ctx{pid, &m_Impl->m_InspectorBefore, nullptr};
    ::EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto *c = reinterpret_cast<Ctx*>(lp);
        DWORD p = 0; ::GetWindowThreadProcessId(h, &p);
        if(p != c->pid) return TRUE;
        if(c->before->contains(reinterpret_cast<WId>(h))) return TRUE;
        if(!::IsWindowVisible(h)) return TRUE;
        RECT r;
        if(!::GetWindowRect(h, &r) || r.right - r.left < 100 || r.bottom - r.top < 100)
            return TRUE;
        c->found = h;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&ctx));

    if(ctx.found){
        m_Impl->m_InspectorProbe->stop();
        AdoptInspectorWindow(reinterpret_cast<WId>(ctx.found));
        return;
    }
    if(++m_Impl->m_InspectorProbeTicks >= 60) m_Impl->m_InspectorProbe->stop();
}

void EdgeWebView::AdoptInspectorWindow(WId id){
    if(m_Impl->m_State.IsRetired() || TreeBank::PurgeView() || !InspectorInMainWindow()) return;
    MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr;
    if(!win) return;

    ReleaseInspector(InspectorRelease::FrontendClosed);

    const int cap = EdgeInspectorPane::MeasureCaptionHeight(reinterpret_cast<HWND>(id));
    m_Impl->m_InspectorWinId = id;
    m_Impl->m_InspectorPane = new EdgeInspectorPane(id, cap);

    if(!m_Impl->m_InspectorWatch){
        m_Impl->m_InspectorWatch = new QTimer(this);
        m_Impl->m_InspectorWatch->setInterval(500);
        connect(m_Impl->m_InspectorWatch, &QTimer::timeout,
                this, &EdgeWebView::WatchInspectorWindow);
    }
    m_Impl->m_InspectorWatch->start();

    win->SetInspectorPane(m_Impl->m_InspectorPane, true);
}

void EdgeWebView::WatchInspectorWindow(){
    if(!m_Impl->m_InspectorPane) { m_Impl->m_InspectorWatch->stop(); return;}
    if(!static_cast<EdgeInspectorPane*>(m_Impl->m_InspectorPane)->FrontendClosed()) return;
    ReleaseInspector(InspectorRelease::FrontendClosed);
}

void EdgeWebView::ReleaseInspector(InspectorRelease why){
    if(m_Impl->m_InspectorWatch) m_Impl->m_InspectorWatch->stop();
    if(m_Impl->m_InspectorProbe) m_Impl->m_InspectorProbe->stop();
    if(!m_Impl->m_InspectorPane) return;

    QWidget *pane = m_Impl->m_InspectorPane;
    EdgeInspectorTrace(why == InspectorRelease::Close ? "Release(close)"
                       : why == InspectorRelease::GiveBack ? "Release(give back)" : "Release", pane, m_Impl->m_InspectorWinId);
    m_Impl->m_InspectorPane = nullptr;
    m_Impl->m_InspectorWinId = 0;

    if(m_TreeBank)
        if(MainWindow *win = m_TreeBank->GetMainWindow())
            if(win->DockedInspectorPane() == pane)
                win->SetInspectorPane(nullptr);
    pane->hide();
    static_cast<EdgeInspectorPane*>(pane)->Detach(why);
    pane->deleteLater();
}

#endif
