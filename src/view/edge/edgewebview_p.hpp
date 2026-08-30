#ifndef EDGEWEBVIEW_P_HPP
#define EDGEWEBVIEW_P_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview.hpp"

#include <QSet>
#include <QHash>
#include <QIcon>
#include <QImage>
#include <QPointer>
#include <QMultiHash>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <QColor>
#include <QElapsedTimer>

#include "edgewebviewstate.hpp"
#include "nativehistory.hpp"

#include <windows.h>
#include <wrl.h>
#include "WebView2.h"
#include <dcomp.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <ole2.h>

#include <memory>

#include "edgeeventsubscriptions.hpp"
#include "edgeunadoptedcontroller.hpp"

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

using EdgeProfileCoordinator = EdgeProfileCoordinatorOf<EdgeWebView>;
extern EdgeProfileCoordinator s_Profiles;

extern EdgeDragOutLedger s_DragOut;

class EdgeEnvironment : public QObject {
public:
    static EdgeEnvironment *Instance();

    void Request(int token, EdgeWebView *view);
    void Forget(int token);

    ICoreWebView2Environment *GetEnvironment() const { return m_Environment.Get();}

    static QString UserDataFolder();

private:
    EdgeEnvironment();

    void StartCreation();
    void Finish(HRESULT result, ICoreWebView2Environment *environment);

    EdgeEnvironmentState m_State;
    ComPtr<ICoreWebView2Environment> m_Environment;
    QHash<int, QPointer<EdgeWebView>> m_Waiters;
};

struct EdgeWebView::Private {
    ComPtr<ICoreWebView2Controller> m_Controller;
    ComPtr<ICoreWebView2> m_WebView;

    HWND m_Hwnd;

    ComPtr<ICoreWebView2CompositionController> m_Composition;

    ComPtr<IDCompositionDevice> m_Device;
    ComPtr<IDCompositionTarget> m_Target;
    ComPtr<IDCompositionVisual> m_Visual;

    EdgeEventSubscriptions m_WebViewEvents;
    EdgeEventSubscriptions m_ControllerEvents;
    EdgeEventSubscriptions m_CompositionEvents;

    ComPtr<ICoreWebView2CompositionController3> m_Drag;
    ComPtr<ICoreWebView2CompositionController5> m_DragOut;
    ComPtr<IDropTarget> m_DropTarget;
    HWND m_DropTargetWindow;

    QSet<int> m_HandledKeys;

    EdgeControllerState m_State;

    int m_Token;

    NativeHistory m_History;


    QPointF m_PageScroll;
    QSizeF m_PageContents;
    QSizeF m_PageViewport;
    bool m_HasFocus;

    EdgeDownloadCloseLedger m_Downloads;

    EdgeDocumentCoordinator m_Document;
    QString m_ProfileName;
    bool m_PrivateMode;

    QString m_ActualProfileName;
    bool m_ActualPrivate;
    bool m_ProfileMeasured;
    QStringList m_SpecificSet;

    bool m_BypassCacheOnce;

    EdgeInputLedger m_Input;
    EdgeClickClock m_Clicks;
    EdgeDropLedger m_Drop;
    EdgeDropEchoLedger m_DropEcho;
    QElapsedTimer m_DropEchoClock;
    QPointF m_LastMousePos;

    QWindow *m_HostWindow;
    QWidget *m_Container;

    QWidget *m_InspectorPane;
    WId m_InspectorWinId;
    QTimer *m_InspectorProbe;
    QTimer *m_InspectorWatch;
    int m_InspectorProbeTicks;
    QSet<WId> m_InspectorBefore;

    QUrl m_Url;
    QString m_Title;
    QIcon m_Icon;
    QColor m_BaseBackgroundColor;
    QImage m_GrabbedDisplayData;
    QPointF m_Scroll;

    bool m_Audible;
    bool m_Muted;
#ifdef MEDIATIME
    int m_MediaTimeSaveTimer;
#endif
    bool m_Suspended;

    Private()
        : m_Controller(nullptr)
        , m_WebView(nullptr)
        , m_Hwnd(nullptr)
        , m_Composition(nullptr)
        , m_Device(nullptr)
        , m_Target(nullptr)
        , m_Visual(nullptr)
        , m_Drag(nullptr)
        , m_DragOut(nullptr)
        , m_DropTarget(nullptr)
        , m_DropTargetWindow(nullptr)
        , m_HandledKeys(QSet<int>())
        , m_State(EdgeControllerState())
        , m_Token(0)
        , m_HasFocus(false)
        , m_Downloads(EdgeDownloadCloseLedger())
        , m_ProfileName(QString())
        , m_PrivateMode(false)
        , m_ActualProfileName(QString())
        , m_ActualPrivate(false)
        , m_ProfileMeasured(false)
        , m_BypassCacheOnce(false)
        , m_HostWindow(nullptr)
        , m_Container(nullptr)
        , m_InspectorPane(nullptr)
        , m_InspectorWinId(0)
        , m_InspectorProbe(nullptr)
        , m_InspectorWatch(nullptr)
        , m_InspectorProbeTicks(0)
        , m_Url(QUrl())
        , m_Title(QString())
        , m_Icon(QIcon())
        , m_BaseBackgroundColor(QColor())
        , m_GrabbedDisplayData(QImage())
        , m_Scroll(QPointF(0.5, 0.5))
        , m_Audible(false)
        , m_Muted(false)
#ifdef MEDIATIME
        , m_MediaTimeSaveTimer(0)
#endif
        , m_Suspended(false)
    {
    }
};

#define VANILLA_KEEP_WEBVIEW_EVENT(source, event, token)               \
    do{ if(m_Impl->m_State.IsRetired()) break;                                 \
        VANILLA_KEEP_EVENT_TOKEN(m_Impl->m_WebViewEvents, source, event, token);\
    }while(false)
#define VANILLA_KEEP_CONTROLLER_EVENT(source, event, token)            \
    do{ if(m_Impl->m_State.IsRetired()) break;                                 \
        VANILLA_KEEP_EVENT_TOKEN(m_Impl->m_ControllerEvents, source, event, token);\
    }while(false)
#define VANILLA_KEEP_COMPOSITION_EVENT(source, event, token)           \
    do{ if(m_Impl->m_State.IsRetired()) break;                                 \
        VANILLA_KEEP_EVENT_TOKEN(m_Impl->m_CompositionEvents, source, event, token);\
    }while(false)

#define VANILLA_STOP_IF_RETIRED()                                      \
    do{ if(m_Impl->m_State.IsRetired()) return;}while(false)

using EdgeUnadoptedController = EdgeUnadoptedControllerOf<ICoreWebView2Controller>;

inline RECT PhysicalBoundsOf(QWindow *window){
    if(!window) return RECT{};
    const qreal ratio = window->devicePixelRatio();
    RECT rect = {};
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = static_cast<LONG>(qRound(window->width()  * ratio));
    rect.bottom = static_cast<LONG>(qRound(window->height() * ratio));
    return rect;
}

bool EdgeInspectorTraceOn();
QString BlankBackgroundJsCode(const QColor &color);

#endif

#endif
