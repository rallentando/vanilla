#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QMainWindow>

class QGraphicsScene;

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
    MainWindow(int id, QPoint pos = QPoint(), QWidget *parent = 0);
    ~MainWindow();

    int GetIndex();

    void SaveSettings();
    void LoadSettings();
    void RemoveSettings();

    TreeBank *GetTreeBank() const;
    TreeBar *GetTreeBar() const;
    ToolBar *GetToolBar() const;
    ModelessDialogFrame *DialogFrame() const;

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
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;
    void moveEvent(QMoveEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;
#endif

private:
    int m_Index;
    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    ToolBar *m_ToolBar;
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

class TitleBar : public QWidget{
    Q_OBJECT
public:
    TitleBar(MainWindow *mainwindow);

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;

private:
    enum HoveredButton{
        NoButton,
        MenuButton,
        ViewTreeButton,
        ShadeButton,
        MinimizeButton,
        MaximizeButton,
        CloseButton
    } m_HoveredButton;

    QRect MenuAreaRect() const;
    QRect ViewTreeAreaRect() const;
    QRect ShadeAreaRect() const;
    QRect MinimizeAreaRect() const;
    QRect MaximizeAreaRect() const;
    QRect CloseAreaRect() const;
    QRect MenuAreaRect1()     const { QRect r = MenuAreaRect();     return QRect(r.x()-4,r.y()-4,r.width()+9,r.height()+9);}
    QRect ViewTreeAreaRect1() const { QRect r = ViewTreeAreaRect(); return QRect(r.x()-8,r.y()-4,r.width()+17,r.height()+9);}
    QRect ShadeAreaRect1()    const { QRect r = ShadeAreaRect();    return QRect(r.x()-9,r.y()-5,r.width()+17,r.height()+9);}
    QRect MinimizeAreaRect1() const { QRect r = MinimizeAreaRect(); return QRect(r.x()-9,r.y()-5,r.width()+17,r.height()+9);}
    QRect MaximizeAreaRect1() const { QRect r = MaximizeAreaRect(); return QRect(r.x()-9,r.y()-5,r.width()+17,r.height()+9);}
    QRect CloseAreaRect1()    const { QRect r = CloseAreaRect();    return QRect(r.x()-10,r.y()-5,r.width()+26,r.height()+9);}

    static const int e = EDGE_WIDGET_SIZE;
    static const int t = TITLE_BAR_HEIGHT;
    static const int et = e+t;
    MainWindow *m_MainWindow;
    QPoint m_Pos;
    bool m_Moved;

    QPixmap m_Shade;
    QPixmap m_Unshade;
    QPixmap m_Minimize;
    QPixmap m_Maximize;
    QPixmap m_Normal;
    QPixmap m_Close;
};

class MainWindowEdgeWidget : public QWidget{
    Q_OBJECT
public:
    MainWindowEdgeWidget(MainWindow *mainwindow);
protected:
    static const int e = EDGE_WIDGET_SIZE;
    static const int t = TITLE_BAR_HEIGHT;
    static const int et = e+t;

    virtual void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void enterEvent(QEvent *ev) Q_DECL_OVERRIDE;
    virtual void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    virtual QRect ComputeNewRect(QRect rect, QPoint pos) = 0;
    virtual Qt::CursorShape CursorShape() = 0;
    MainWindow *m_MainWindow;
    QPoint m_Pos;
};

class MainWindowNorthWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowNorthWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowSouthWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowSouthWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowWestWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowEastWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowNorthWestWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowNorthWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowNorthEastWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowNorthEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowSouthWestWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowSouthWestWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

class MainWindowSouthEastWidget : public MainWindowEdgeWidget{
    Q_OBJECT
public:
    MainWindowSouthEastWidget(MainWindow *mainwindow)
        : MainWindowEdgeWidget(mainwindow){}
protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    QRect ComputeNewRect(QRect rect, QPoint pos) Q_DECL_OVERRIDE;
    Qt::CursorShape CursorShape() Q_DECL_OVERRIDE;
};

#endif //ifndef MAINWINDOW_HPP
