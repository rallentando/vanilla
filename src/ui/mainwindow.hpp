#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "devicescale.hpp"
#include "windowframereveal.hpp"
#include "windowframehit.hpp"
#include "titlebar.hpp"

#include <QMainWindow>
#include <QPointer>

class QGraphicsScene;
class QDockWidget;
class SidePanels;
class QTimer;

class TreeBank;
class TreeBar;
class ToolBar;
class ModelessDialogFrame;

class TitleBar;
class MainWindowNorthWidget;
class MainWindowSouthWidget;
class MainWindowWestWidget;
class MainWindowEastWidget;
class MainWindowNorthWestWidget;
class MainWindowNorthEastWidget;
class MainWindowSouthWestWidget;
class MainWindowSouthEastWidget;

class MainWindow : public
    QMainWindow
{
    Q_OBJECT

public:
    MainWindow(int id, QPoint pos = QPoint(), bool showNow = true, QWidget *parent = nullptr);
    ~MainWindow() override;

    template <class T> T ScaleByDevice(T v) const {
        return DeviceScale::FromDpi(v, static_cast<int>(logicalDpiY()));
    }

    int GetIndex();

    void SaveSettings();
    void LoadSettings();
    void RemoveSettings();

    TreeBank *GetTreeBank() const;
    SidePanels *GetSidePanels() const { return m_SidePanels;}
    TreeBar *GetTreeBar() const;
    ToolBar *GetToolBar() const;
    ModelessDialogFrame *DialogFrame() const;

    void SetInspectorPane(QWidget *pane, bool request = false);
    QWidget *DockedInspectorPane() const;
    void SuspendInspectorPane(bool suspend);
    void CloseInspectorPane();
#if defined(Q_OS_WIN) && defined(EDGEWEBVIEW)
    void ApplyInspectorDockFrame();
#endif

    QMenu *createPopupMenu() override;
    void AddDisplayMenuActions(QMenu *menu);

    bool IsMenuBarEmpty() const;
    void ClearMenuBar();
    void CreateMenuBar();
    bool IsShaded() const;
    void Shade();
    void Unshade();
    void ShowAllEdgeWidgets();
    void HideAllEdgeWidgets();
    void RaiseAllEdgeWidgets();
    void AdjustAllEdgeWidgets();
    void CurrentViewChanged();
    bool IsStripExpanded() const;
    void SetStripExpanded(bool on);
    bool IsStripPinned() const { return m_StripPinned;}
    void PinStrip();
    void UnpinStrip();
    void HoldStripReveal();
public slots:
    void UpdateAllEdgeWidgets();
    void SetWindowTitle(const QString&);
    void ToggleMenuBar();
    void ToggleTreeBar();
    void ToggleToolBar();
    void ToggleFullScreen();
    void ToggleMaximized();
    void ToggleMinimized();
    void ToggleShaded();
    void SetMenuBar(bool on);
    void SetFullScreen(bool on);
    void SetFocus();
    void raise();

public:
    void ActivateIfNeeded();
    void ShowWithFirstFrame();
    static void ActivateWindowIfNeeded(QWidget *window);
    static void ActivateWindowIfNeeded(QWidget *window, WId front);
    bool IsContentFullScreen() const { return m_ContentFullScreen;}

    static void SetOwner(QWidget *window, MainWindow *owner);
    static MainWindow *DialogOwner();

    bool IsFrameActive() const;
    void ApplyTheme();
    void RepaintWindowEdge();

protected:
    void changeEvent(QEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;
    void closeEvent(QCloseEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void moveEvent(QMoveEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    void ApplyTranslucentFrame();
    friend class tst_windowframe;
    void ShowWindowFrameWidgets();
    void HideWindowFrameWidgets();
    void UpdateTitleBarVisibility();
    void ApplyTitleBarVisibility(const QPoint &cursor, bool active, bool buttonsDown);
    QList<QWidget*> EdgeWidgets() const;
    bool ApplyInnerMargin();
    friend class InnerMarginWatch;
#if defined(Q_OS_WIN)
    bool FramelessNativeEvent(void *message, qintptr *result);
    void ApplyFramelessFrame();
    void ApplyEdgeCaption();
    WindowFrameHit::Layout FrameHitLayout() const;
    void ReleaseMaximizeButton();
    void DropFullScreenCover();
#endif
    void CloakWhileSwitchingFullScreen();

    void UpdateInspectorDock();

    int m_Index;
    bool m_ContentFullScreen;
    Qt::WindowStates m_StateBeforeFullScreen;
    bool m_MenuBarVisibleBeforeFullScreen;
    bool m_TreeBarVisibleBeforeFullScreen;
    bool m_ToolBarVisibleBeforeFullScreen;
#if defined(Q_OS_WIN)
    quintptr m_FullScreenCover = 0;
    bool m_FullScreenCloaked = false;
    int m_FullScreenSwitch = 0;
#endif
    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    ToolBar *m_ToolBar;
    QDockWidget *m_InspectorDock;
    SidePanels *m_SidePanels;
    bool m_InspectorDockClosed;
    bool m_InspectorDockSuspended;
    bool m_InspectorDockPlaced;
    ModelessDialogFrame *m_DialogFrame;
    TitleBar *m_TitleBar;
    TitleBar *m_TitleStrip;
    bool m_StripExpanded = false;
    bool m_StripPinned = false;
    bool m_StripWasShaded = false;
    bool WantsInnerBar() const;
    bool HasInnerBar() const;
    TitleBar *SeparateStrip() const { return m_TitleStrip != m_TitleBar ? m_TitleStrip : nullptr;}
    bool IsFrameShown() const {
        return isVisible() && !isMinimized() && !isFullScreen() && !m_ContentFullScreen;
    }
    bool IsSimplyMaximized() const {
        return isMaximized() && !isMinimized() && !isFullScreen() && !m_ContentFullScreen;
    }
    QObject *m_StripPressWatch = nullptr;
    void SetStripPinned(bool on);
    QTimer *m_TitleBarTimer;
    WindowFrameReveal m_TitleBarReveal;
    MainWindowNorthWidget *m_NorthWidget;
    MainWindowSouthWidget *m_SouthWidget;
    MainWindowWestWidget *m_WestWidget;
    MainWindowEastWidget *m_EastWidget;
    MainWindowNorthWestWidget *m_NorthWestWidget;
    MainWindowNorthEastWidget *m_NorthEastWidget;
    MainWindowSouthWestWidget *m_SouthWestWidget;
    MainWindowSouthEastWidget *m_SouthEastWidget;
    QPointer<QMenu> m_SystemMenu;
    QList<QPointer<QAction>> m_SystemMenuActions;
};

class MainWindowEdgeWidget : public QWidget {
    Q_OBJECT
public:
    MainWindowEdgeWidget(MainWindow *mainwindow);

    template <class T> T ScaleByDevice(T v) const {
        return DeviceScale::FromDpi(v, static_cast<int>(logicalDpiY()));
    }

protected:
    const int e = ScaleByDevice(EDGE_WIDGET_SIZE);
    const int t = ScaleByDevice(TITLE_BAR_HEIGHT);
    const int et = e+t;

    virtual void paintEvent(QPaintEvent *ev) override;
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void mouseMoveEvent(QMouseEvent *ev) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    virtual void enterEvent(QEnterEvent *ev) override;
    virtual void leaveEvent(QEvent *ev) override;
    virtual QRect ComputeNewRect(QRect rect, QPoint pos) = 0;
    virtual Qt::CursorShape CursorShape() = 0;
    void PaintSide(QPaintEvent *ev, Qt::Edge side);
    MainWindow *m_MainWindow;
    QPoint m_Pos;
};

class MainWindowNorthWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowNorthWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowSouthWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowSouthWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowWestWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowEastWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowNorthWestWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowNorthWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowNorthEastWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowNorthEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowSouthWestWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowSouthWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

class MainWindowSouthEastWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowSouthEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) override;
    QRect ComputeNewRect(QRect rect, QPoint pos) override;
    Qt::CursorShape CursorShape() override;
};

#endif
