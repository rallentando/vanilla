#include "switch.hpp"

#include "blurbehind.hpp"

#include "application.hpp"

#include <QWidget>
#include <QWindow>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace BlurBehind {

bool Enabled(){
    return Application::EnableTranslucentPanels();
}

Theme::Role Surface(){
    return Enabled() ? Theme::PreviewBackgroundTranslucent : Theme::PreviewBackground;
}

#if defined(Q_OS_WIN)
namespace {

    struct AccentPolicy {
        int state;
        int flags;
        unsigned gradientColor;
        int animationId;
    };
    struct CompositionAttributeData {
        int attribute;
        void *data;
        SIZE_T size;
    };
    typedef BOOL (WINAPI *SetCompositionAttribute)(HWND, CompositionAttributeData*);

    const int WCA_ACCENT_POLICY = 19;
    const int ACCENT_ENABLE_ACRYLICBLURBEHIND = 4;
    const unsigned ACCENT_TINT = 0x01000000;

    const DWORD DWMWA_SYSTEMBACKDROP_TYPE_ = 38;
    const DWORD DWMSBT_TRANSIENTWINDOW_ = 3;

    SetCompositionAttribute Resolve(){
        static const SetCompositionAttribute set =
            reinterpret_cast<SetCompositionAttribute>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
        return set;
    }

    void Accent(HWND hwnd){
        SetCompositionAttribute set = Resolve();
        if(!set) return;
        AccentPolicy policy = { ACCENT_ENABLE_ACRYLICBLURBEHIND, 0, ACCENT_TINT, 0 };
        CompositionAttributeData data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
        set(hwnd, &data);
    }

    bool SystemBackdrop(HWND hwnd){
        DWORD type = DWMSBT_TRANSIENTWINDOW_;
        return SUCCEEDED(::DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE_, &type, sizeof(type)));
    }
}
#endif

#if defined(Q_OS_WIN)
namespace {
    void Ask(Kind kind, QWidget *window){
        QWidget *top = window->window();
        const HWND hwnd = reinterpret_cast<HWND>(top->winId());
        const QWindow *handle = top->windowHandle();
        const bool layered = (::GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED) != 0;
        switch(MechanismFor(kind, handle ? handle->surfaceType() : QSurface::RasterSurface, layered)){
        case Mechanism::Accent:
            Accent(hwnd);
            break;
        case Mechanism::SystemBackdrop:
            if(!SystemBackdrop(hwnd)) Accent(hwnd);
            break;
        case Mechanism::None:
            break;
        }
    }
}
#endif

void Apply(QWidget *window){
    if(!Enabled() || !window) return;
#if defined(Q_OS_WIN)
    Ask(Kind::Card, window);
#endif
}

void RoundCorners(QWidget *window){
    if(!window) return;
#if defined(Q_OS_WIN)
    const DWORD round = 2;
    ::DwmSetWindowAttribute(reinterpret_cast<HWND>(window->window()->winId()),
                            33 , &round, sizeof(round));
#endif
}

void Blur(QWidget *window){
    if(!window) return;
#if defined(Q_OS_WIN)
    Ask(Kind::MainWindow, window);
#endif
}

Mechanism MechanismFor(Kind kind, QSurface::SurfaceType type, bool layered){
    if(kind == Kind::Card) return Mechanism::Accent;
    switch(type){
    case QSurface::RasterSurface: return Mechanism::Accent;
    case QSurface::OpenGLSurface: return layered ? Mechanism::Accent : Mechanism::SystemBackdrop;
    default:                      return Mechanism::None;
    }
}

bool Available(){
#if defined(Q_OS_WIN)
    return Resolve() != nullptr;
#else
    return false;
#endif
}

bool MainWindowMayBeTranslucent(int widgetsRhi, int highDpiDownscale, const QByteArray &backend){
    if(widgetsRhi == 0 && highDpiDownscale == 0) return true;
    return backend == QByteArrayLiteral("opengl") || backend == QByteArrayLiteral("gl");
}

bool SurfaceMayBeTranslucent(QSurface::SurfaceType type){
    return MechanismFor(Kind::MainWindow, type, false) != Mechanism::None;
}

}
