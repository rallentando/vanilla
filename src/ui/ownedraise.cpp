#include "switch.hpp"
#include "ownedraise.hpp"

#include <QCoreApplication>

#if defined(Q_OS_WIN)
#  include <QAbstractNativeEventFilter>
#  include <windows.h>

namespace {
    quintptr g_Owner = 0;
    OwnedRaise::ForegroundSource g_Foreground = nullptr;

    bool InGroup(HWND window, HWND root){
        for(HWND h = window; h; h = GetWindow(h, GW_OWNER))
            if(h == root) return true;
        return false;
    }

    class OwnerKeeper : public QAbstractNativeEventFilter {
    public:
        bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override {
            Q_UNUSED(result)
            if(eventType != "windows_generic_MSG" || !message) return false;
            const MSG *msg = static_cast<const MSG*>(message);
            if(msg->message == WM_WINDOWPOSCHANGING)
                OwnedRaise::Changing(reinterpret_cast<WINDOWPOS*>(msg->lParam));
            return false;
        }
    };
}

bool OwnedRaise::Changing(WINDOWPOS *pos){
    if(!pos || !pos->hwnd) return false;
    if(pos->flags & (SWP_NOZORDER | SWP_NOOWNERZORDER)) return false;
    if(!(pos->flags & SWP_NOACTIVATE)) return false;
    const HWND after = pos->hwndInsertAfter;
    if(after == HWND_BOTTOM || after == HWND_TOPMOST || after == HWND_NOTOPMOST) return false;
    if(!GetWindow(pos->hwnd, GW_OWNER)) return false;
    if(GetWindowLongPtr(pos->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) return false;
    const HWND root = GetAncestor(pos->hwnd, GA_ROOTOWNER);
    if(!root || root == pos->hwnd) return false;
    if(reinterpret_cast<quintptr>(root) == g_Owner) return false;
    if(const HWND foreground = g_Foreground ? reinterpret_cast<HWND>(g_Foreground())
                                            : GetForegroundWindow())
        if(GetAncestor(foreground, GA_ROOTOWNER) == root) return false;

    HWND top = root;
    for(HWND above = GetWindow(top, GW_HWNDPREV); above && InGroup(above, root);
        above = GetWindow(above, GW_HWNDPREV))
        top = above;
    const HWND overGroup = GetWindow(top, GW_HWNDPREV);
    if(!overGroup) return false;
    bool over = after == HWND_TOP;
    for(HWND h = overGroup; h && !over; h = GetWindow(h, GW_HWNDPREV))
        if(h == after) over = true;
    if(!over || after == overGroup) return false;
    pos->hwndInsertAfter = overGroup;
    return true;
}

void OwnedRaise::SetForegroundSource(ForegroundSource source){
    g_Foreground = source;
}

OwnedRaise::OwnerRaise::OwnerRaise(quintptr owner)
    : m_Previous(g_Owner)
{
    g_Owner = owner;
}

OwnedRaise::OwnerRaise::~OwnerRaise(){
    g_Owner = m_Previous;
}

void OwnedRaise::Install(){
    static OwnerKeeper keeper;
    if(QCoreApplication *app = QCoreApplication::instance())
        app->installNativeEventFilter(&keeper);
}
#else
OwnedRaise::OwnerRaise::OwnerRaise(quintptr owner) : m_Previous(owner) {}
OwnedRaise::OwnerRaise::~OwnerRaise(){}
void OwnedRaise::Install(){}
#endif
