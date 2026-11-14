#include "switch.hpp"
#include "ownedraise.hpp"

#include <QtTest>

#if defined(Q_OS_WIN)
#  include <windows.h>

namespace {
    bool g_Hook = true;
    HWND g_Foreground = nullptr;
    quintptr Foreground(){ return reinterpret_cast<quintptr>(g_Foreground);}

    LRESULT CALLBACK Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
        if(g_Hook && message == WM_WINDOWPOSCHANGING)
            OwnedRaise::Changing(reinterpret_cast<WINDOWPOS*>(lParam));
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    const wchar_t *ClassName(){
        static const wchar_t *name = [](){
            WNDCLASSW wc = {};
            wc.lpfnWndProc = Proc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"tst_ownedraise";
            RegisterClassW(&wc);
            return L"tst_ownedraise";
        }();
        return name;
    }

    HWND Make(HWND owner = nullptr, DWORD exStyle = 0){
        const HWND hwnd = CreateWindowExW(exStyle | WS_EX_NOACTIVATE, ClassName(), L"", WS_POPUP,
                                          -30000, -30000, 8, 8, owner, nullptr,
                                          GetModuleHandleW(nullptr), nullptr);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        return hwnd;
    }

    const UINT Flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;

    void Raise(HWND hwnd){ SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, Flags);}
    void RaiseOwner(HWND hwnd){
        OwnedRaise::OwnerRaise owner(reinterpret_cast<quintptr>(hwnd));
        Raise(hwnd);
    }

    bool Above(HWND upper, HWND lower){
        for(HWND h = GetWindow(lower, GW_HWNDPREV); h; h = GetWindow(h, GW_HWNDPREV))
            if(h == upper) return true;
        return false;
    }
}
#endif

class tst_OwnedRaise : public QObject {
    Q_OBJECT

private slots:
#if defined(Q_OS_WIN)
    void init(){ OwnedRaise::SetForegroundSource(&Foreground);}
    void cleanup(){
        g_Hook = true;
        g_Foreground = nullptr;
        OwnedRaise::SetForegroundSource(nullptr);
    }

    void raisingAnOwnedWindowKeepsItsOwnerBehind(){
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        Raise(b);
        QVERIFY(Above(b, a));

        g_Hook = false;
        Raise(c);
        QVERIFY(Above(a, b));

        g_Hook = true;
        Raise(b);
        QVERIFY(Above(b, a));
        Raise(c);
        QVERIFY(Above(b, a));
        QVERIFY(Above(c, a));
        QVERIFY(Above(b, c));

        DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }

    void raisingGoesToTheTopOfTheGroup(){
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW), d = Make(a, WS_EX_TOOLWINDOW);
        Raise(b);
        Raise(c); Raise(d);
        QVERIFY(Above(d, c));
        QVERIFY(Above(c, a));
        QVERIFY(Above(b, d));
        Raise(c);
        QVERIFY(Above(c, d));
        QVERIFY(Above(d, a));
        QVERIFY(Above(b, c));

        RaiseOwner(a);
        QVERIFY(Above(a, b));
        Raise(d);
        QVERIFY(Above(d, c));
        QVERIFY(Above(c, a));
        QVERIFY(Above(a, b));

        DestroyWindow(d); DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }

    void aWindowOwnedTwiceDownKeepsTheFirstOwnerBehind(){
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        const HWND e = Make(c, WS_EX_TOOLWINDOW);
        Raise(b);
        Raise(e);
        QVERIFY(Above(b, a));
        QVERIFY(Above(b, c));
        QVERIFY(Above(e, c));
        QVERIFY(Above(c, a));
        RaiseOwner(a);
        QVERIFY(Above(a, b));
        QVERIFY(Above(e, c));
        QVERIFY(Above(c, a));

        DestroyWindow(e); DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }

    void anOwnedWindowUnderItsOwnerComesOverIt(){
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        SetWindowPos(c, a, 0, 0, 0, 0, Flags | SWP_NOOWNERZORDER);
        Raise(b);
        if(!Above(a, c)) QSKIP("the window manager put it back over its owner by itself");
        Raise(c);
        QVERIFY(Above(c, a));
        QVERIFY(Above(b, c));
        QVERIFY(Above(b, a));

        DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }

    void aTopmostWindowAboveStaysAbove(){
        const HWND a = Make(), t = Make(nullptr, WS_EX_TOPMOST);
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        Raise(c);
        QVERIFY(Above(t, c));
        QVERIFY(Above(c, a));
        QVERIFY(!(GetWindowLongPtr(c, GWL_EXSTYLE) & WS_EX_TOPMOST));

        DestroyWindow(c); DestroyWindow(t); DestroyWindow(a);
    }

    void aTopmostOwnedWindowStaysTopmost(){
        const HWND a = Make(), b = Make(), t = Make(nullptr, WS_EX_TOPMOST);
        const HWND c = Make(a, WS_EX_TOOLWINDOW | WS_EX_TOPMOST);
        Raise(b);
        Raise(c);
        QVERIFY(GetWindowLongPtr(c, GWL_EXSTYLE) & WS_EX_TOPMOST);
        QVERIFY(Above(c, t));
        QVERIFY(Above(c, b));

        DestroyWindow(c); DestroyWindow(t); DestroyWindow(b); DestroyWindow(a);
    }

    void theForegroundWindowsGroupComesForward(){
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        Raise(b);
        g_Foreground = a;
        Raise(c);
        QVERIFY(Above(a, b));
        QVERIFY(Above(c, a));
        g_Foreground = c;
        Raise(b);
        Raise(c);
        QVERIFY(Above(a, b));

        g_Foreground = b;
        Raise(b);
        Raise(c);
        QVERIFY(Above(b, a));

        DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }

    void theOwnerRaisedOnPurposeComesForward(){
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        Raise(b);
        Raise(c);
        QVERIFY(Above(b, a));
        RaiseOwner(a);
        QVERIFY(Above(a, b));
        QVERIFY(Above(c, a));

        Raise(b);
        QVERIFY(Above(b, c));
        Raise(a);
        QVERIFY(Above(b, a));
        Raise(c);
        QVERIFY(Above(b, a));

        DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }

    void otherPositionsAreLeftAlone(){
        g_Hook = false;
        const HWND a = Make(), b = Make();
        const HWND c = Make(a, WS_EX_TOOLWINDOW);
        const HWND child = CreateWindowExW(0, ClassName(), L"", WS_CHILD, 0, 0, 1, 1,
                                           a, nullptr, GetModuleHandleW(nullptr), nullptr);
        Raise(b);

        const auto untouched = [&](HWND hwnd, HWND after, UINT flags){
            WINDOWPOS pos = {};
            pos.hwnd = hwnd;
            pos.hwndInsertAfter = after;
            pos.flags = flags;
            const bool changed = OwnedRaise::Changing(&pos);
            return !changed && pos.hwndInsertAfter == after && pos.flags == flags;
        };
        QVERIFY(!untouched(c, HWND_TOP, Flags));
        QVERIFY(untouched(c, HWND_TOP, Flags | SWP_NOZORDER));
        QVERIFY(untouched(c, HWND_TOP, Flags | SWP_NOOWNERZORDER));
        QVERIFY(untouched(c, HWND_TOP, SWP_NOMOVE | SWP_NOSIZE));
        QVERIFY(untouched(c, HWND_BOTTOM, Flags));
        QVERIFY(untouched(c, HWND_TOPMOST, Flags));
        QVERIFY(untouched(a, HWND_TOP, Flags));
        QVERIFY(untouched(child, HWND_TOP, Flags));
        {
            OwnedRaise::OwnerRaise owner(reinterpret_cast<quintptr>(a));
            QVERIFY(untouched(c, HWND_TOP, Flags));
        }
        QVERIFY(!OwnedRaise::Changing(nullptr));

        DestroyWindow(child); DestroyWindow(c); DestroyWindow(b); DestroyWindow(a);
    }
#else
    void notWindows(){ QSKIP("Windows only");}
#endif
};

QTEST_MAIN(tst_OwnedRaise)
#include "tst_ownedraise.moc"
