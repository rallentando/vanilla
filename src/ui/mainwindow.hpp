#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "devicescale.hpp"

#include <QMainWindow>


class QGraphicsScene;
class QDockWidget;

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
    MainWindow(int id, QPoint pos = QPoint(), QWidget *parent = nullptr);
    ~MainWindow() Q_DECL_OVERRIDE;

    template <class T> T ScaleByDevice(T v) const {
        return DeviceScale::FromDpi(v, static_cast<int>(logicalDpiY()));
    }

    int GetIndex();

    void SaveSettings();
    void LoadSettings();
    void RemoveSettings();

    TreeBank *GetTreeBank() const;
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

    bool IsMenuBarEmpty() const;
    void ClearMenuBar();
    void CreateMenuBar();
    bool IsContentFullScreen() const;
    bool IsShaded() const;
    void Shade();
    void Unshade();
    void ShowAllEdgeWidgets();
    void HideAllEdgeWidgets();
    void RaiseAllEdgeWidgets();
    void AdjustAllEdgeWidgets();
public slots:
    void UpdateAllEdgeWidgets();
    void SetWindowTitle(const QString&);
    void ToggleNotifier();
    void ToggleReceiver();
    void ToggleMenuBar();
    void ToggleTreeBar();
    void ToggleToolBar();
    void ToggleFullScreen();
    void ToggleMaximized();
    void ToggleMinimized();
    void ToggleShaded();
    void SetMenuBar(bool on);
    void SetTreeBar(bool on);
    void SetToolBar(bool on);
    void SetFullScreen(bool on);
    void SetMaximized(bool on);
    void SetMinimized(bool on);
    void SetShaded(bool on);
    void SetFocus();

protected:
    bool eventFilter(QObject *watched, QEvent *ev) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;
    void moveEvent(QMoveEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;

private:
    void ShowWindowFrameWidgets();
    void HideWindowFrameWidgets();

    void UpdateInspectorDock();

    int m_Index;
    bool m_ContentFullScreen;
    Qt::WindowStates m_StateBeforeFullScreen;
    bool m_MenuBarVisibleBeforeFullScreen;
    bool m_TreeBarVisibleBeforeFullScreen;
    bool m_ToolBarVisibleBeforeFullScreen;
    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    ToolBar *m_ToolBar;
    QDockWidget *m_InspectorDock;
    bool m_InspectorDockClosed;
    bool m_InspectorDockSuspended;
    bool m_InspectorDockPlaced;
    ModelessDialogFrame *m_DialogFrame;
    TitleBar *m_TitleBar;
    MainWindowNorthWidget *m_NorthWidget;
    MainWindowSouthWidget *m_SouthWidget;
    MainWindowWestWidget *m_WestWidget;
    MainWindowEastWidget *m_EastWidget;
    MainWindowNorthWestWidget *m_NorthWestWidget;
    MainWindowNorthEastWidget *m_NorthEastWidget;
    MainWindowSouthWestWidget *m_SouthWestWidget;
    MainWindowSouthEastWidget *m_SouthEastWidget;
};

class TitleBar : public QWidget {
    Q_OBJECT
public:
    TitleBar(MainWindow *mainwindow);

    template <class T> T ScaleByDevice(T v) const {
        return DeviceScale::FromDpi(v, static_cast<int>(logicalDpiY()));
    }

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void enterEvent(QEnterEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;

private:
    enum HoveredButton {
        NoButton,
        MenuButton,
        ViewTreeButton,
        ShadeButton,
        MinimizeButton,
        MaximizeButton,
        CloseButton
    } m_HoveredButton;

    void LoadIcons();

    QRect MenuAreaRect() const;
    QRect ViewTreeAreaRect() const;
    QRect ShadeAreaRect() const;
    QRect MinimizeAreaRect() const;
    QRect MaximizeAreaRect() const;
    QRect CloseAreaRect() const;
    QRect MenuAreaRect1()     const { QRect r = MenuAreaRect();     return QRect(r.x()-ScaleByDevice(4), r.y()-ScaleByDevice(4), r.width()+ScaleByDevice(9), r.height()+ScaleByDevice(9));}
    QRect ViewTreeAreaRect1() const { QRect r = ViewTreeAreaRect(); return QRect(r.x()-ScaleByDevice(8), r.y()-ScaleByDevice(4), r.width()+ScaleByDevice(17),r.height()+ScaleByDevice(9));}
    QRect ShadeAreaRect1()    const { QRect r = ShadeAreaRect();    return QRect(r.x()-ScaleByDevice(9), r.y()-ScaleByDevice(5), r.width()+ScaleByDevice(17),r.height()+ScaleByDevice(9));}
    QRect MinimizeAreaRect1() const { QRect r = MinimizeAreaRect(); return QRect(r.x()-ScaleByDevice(9), r.y()-ScaleByDevice(5), r.width()+ScaleByDevice(17),r.height()+ScaleByDevice(9));}
    QRect MaximizeAreaRect1() const { QRect r = MaximizeAreaRect(); return QRect(r.x()-ScaleByDevice(9), r.y()-ScaleByDevice(5), r.width()+ScaleByDevice(17),r.height()+ScaleByDevice(9));}
    QRect CloseAreaRect1()    const { QRect r = CloseAreaRect();    return QRect(r.x()-ScaleByDevice(10),r.y()-ScaleByDevice(5), r.width()+ScaleByDevice(26),r.height()+ScaleByDevice(9));}

    int ButtonAreaWidth(int count) const {
        return ScaleByDevice(19) + ScaleByDevice(32)
             + ScaleByDevice(28) * count + ScaleByDevice(5);
    }

    const int e = ScaleByDevice(EDGE_WIDGET_SIZE);
    const int t = ScaleByDevice(TITLE_BAR_HEIGHT);
    const int et = e+t;
    MainWindow *m_MainWindow;
    QPoint m_Pos;
    bool m_Moved;

    QPixmap m_Shade;
    QPixmap m_Unshade;
    QPixmap m_Minimize;
    QPixmap m_Maximize;
    QPixmap m_Normal;
    QPixmap m_Close;
    Theme::Scheme m_IconScheme;
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

    virtual void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void enterEvent(QEnterEvent *ev) Q_DECL_OVERRIDE;
    virtual void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    virtual QRect ComputeNewRect(QRect rect, QPoint pos) = 0;
    virtual Qt::CursorShape CursorShape() = 0;
    MainWindow *m_MainWindow;
    QPoint m_Pos;
};

class MainWindowNorthWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowNorthWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowSouthWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowSouthWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowWestWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowEastWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowNorthWestWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowNorthWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowNorthEastWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowNorthEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowSouthWestWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowSouthWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowSouthEastWidget : public MainWindowEdgeWidget {
    Q_OBJECT
public:
    MainWindowSouthEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

#endif
