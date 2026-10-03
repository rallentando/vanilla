#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "mainwindow.hpp"

#include <QIcon>
#include <QStyle>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QWindow>
#include <QMenuBar>
#include <QScreen>
#include <QTimer>
#include <QCursor>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  ifdef EDGEWEBVIEW
#    include <dwmapi.h>
#  endif
#endif

#include "view.hpp"
#include "application.hpp"
#include "saver.hpp"
#include "treebank.hpp"
#include "sidepanels.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "gadgets.hpp"
#include "graphicstableview.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "quicknativewebview.hpp"
#include "edgewebview.hpp"
#include "dialog.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"

MainWindow::MainWindow(int id, QPoint pos, QWidget *parent)
    :
    QMainWindow(parent)
    , m_ContentFullScreen(false)
    , m_StateBeforeFullScreen(Qt::WindowNoState)
    , m_MenuBarVisibleBeforeFullScreen(false)
    , m_TreeBarVisibleBeforeFullScreen(false)
    , m_ToolBarVisibleBeforeFullScreen(false)
    , m_InspectorDock(nullptr)
    , m_SidePanels(nullptr)
    , m_InspectorDockClosed(false)
    , m_InspectorDockSuspended(false)
    , m_InspectorDockPlaced(false)
    , m_TitleBar(nullptr)
    , m_TitleBarTimer(nullptr)
    , m_NorthWidget(nullptr)
    , m_SouthWidget(nullptr)
    , m_WestWidget(nullptr)
    , m_EastWidget(nullptr)
    , m_NorthWestWidget(nullptr)
    , m_NorthEastWidget(nullptr)
    , m_SouthWestWidget(nullptr)
    , m_SouthEastWidget(nullptr)
{
    m_Index = id;

    m_TreeBank = new TreeBank(this);
    setCentralWidget(m_TreeBank);

    m_TreeBar = new TreeBar(m_TreeBank, this);
    addToolBar(m_TreeBar);

    addToolBarBreak();

    m_ToolBar = new ToolBar(m_TreeBank, this);
    addToolBar(m_ToolBar);

    m_InspectorDock = new QDockWidget(tr("Inspector"), this);
    m_InspectorDock->setObjectName(QStringLiteral("inspector"));
    m_InspectorDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_InspectorDock->installEventFilter(this);
#if defined(Q_OS_WIN) && defined(EDGEWEBVIEW)
    installEventFilter(this);
    connect(m_InspectorDock, &QDockWidget::topLevelChanged, this, [this](bool floating){
        if(floating){
            ApplyInspectorDockFrame();
        } else {
            QTimer::singleShot(0, this, [this](){
                if(QWidget *title = m_InspectorDock->titleBarWidget()) title->update();
                m_InspectorDock->update();
            });
        }
    });
#endif
    addDockWidget(Qt::RightDockWidgetArea, m_InspectorDock);
    m_InspectorDock->hide();
    m_SidePanels = new SidePanels(this);

    m_DialogFrame = Application::GetTemporaryDialogFrame();
    if(m_DialogFrame){
        Application::SetTemporaryDialogFrame(nullptr);
    } else {
        m_DialogFrame = new ModelessDialogFrame();
    }
    m_DialogFrame->show();

    menuBar()->hide();
    if(Application::EnableTransparentBar()){
        menuBar()->setStyleSheet(QStringLiteral("QMenuBar{ background-color: transparent;}"));
    }

    if(Application::EnableFramelessWindow()){
        setWindowFlags(Qt::FramelessWindowHint);
        m_TitleBar        = new TitleBar                  (this);
        m_NorthWidget     = new MainWindowNorthWidget     (this);
        m_SouthWidget     = new MainWindowSouthWidget     (this);
        m_WestWidget      = new MainWindowWestWidget      (this);
        m_EastWidget      = new MainWindowEastWidget      (this);
        m_NorthWestWidget = new MainWindowNorthWestWidget (this);
        m_NorthEastWidget = new MainWindowNorthEastWidget (this);
        m_SouthWestWidget = new MainWindowSouthWestWidget (this);
        m_SouthEastWidget = new MainWindowSouthEastWidget (this);
        m_TitleBarTimer = new QTimer(this);
        m_TitleBarTimer->setObjectName(QStringLiteral("TitleBarRevealTimer"));
        m_TitleBarTimer->setInterval(100);
        connect(m_TitleBarTimer, &QTimer::timeout, this, &MainWindow::UpdateTitleBarVisibility);
        AdjustAllEdgeWidgets();
        connect(Application::GetInstance(), &Application::focusChanged,
                this, &MainWindow::UpdateAllEdgeWidgets);
    }

    move(pos);
    LoadSettings();

    m_InspectorDock->hide();
    for(QDockWidget *dock : m_SidePanels->Docks()) dock->hide();

#if defined(Q_OS_MAC)
    QSize s = size();
    show();
    if(Application::ProductVersion().startsWith(QStringLiteral("10.14")))
        resize(s);
#else
    show();
#endif

}

MainWindow::~MainWindow(){
    if(m_TitleBarTimer) m_TitleBarTimer->stop();
    SetInspectorPane(nullptr);
    m_DialogFrame->deleteLater();
    if(m_TitleBar){
        m_TitleBar        ->deleteLater();
        m_NorthWidget     ->deleteLater();
        m_SouthWidget     ->deleteLater();
        m_WestWidget      ->deleteLater();
        m_EastWidget      ->deleteLater();
        m_NorthWestWidget ->deleteLater();
        m_NorthEastWidget ->deleteLater();
        m_SouthWestWidget ->deleteLater();
        m_SouthEastWidget ->deleteLater();
    }
}

int MainWindow::GetIndex(){
    return m_Index;
}

void MainWindow::SaveSettings(){
    Settings &s = Application::GlobalSettings();

    s.setValue(QStringLiteral("mainwindow/tableview%1").arg(m_Index), m_TreeBank->GetGadgets()->GetStat());
    s.setValue(QStringLiteral("mainwindow/notifier%1").arg(m_Index), m_TreeBank->GetNotifier() ? true : false);
    s.setValue(QStringLiteral("mainwindow/receiver%1").arg(m_Index), m_TreeBank->GetReceiver() ? true : false);
    s.setValue(QStringLiteral("mainwindow/menubar%1").arg(m_Index), !IsMenuBarEmpty());
    s.setValue(QStringLiteral("mainwindow/toolbar%1").arg(m_Index), saveState());
    s.setValue(QStringLiteral("mainwindow/inspectordock%1").arg(m_Index), m_InspectorDockPlaced);
    s.setValue(QStringLiteral("mainwindow/sidepaneldock%1").arg(m_Index), m_SidePanels->Placed());
    s.setValue(QStringLiteral("mainwindow/treebar%1").arg(m_Index), m_TreeBar->GetStat());
    s.setValue(QStringLiteral("mainwindow/status%1").arg(m_Index), static_cast<int>(windowState()));
    if(!isFullScreen() && !isMaximized() && !isMinimized() && width() && height())
        s.setValue(QStringLiteral("mainwindow/geometry%1").arg(m_Index), geometry());
}

void MainWindow::LoadSettings(){
    Settings &s = Application::GlobalSettings();

    MainWindow *current = Application::GetCurrentWindow();
    QVariant tableview_data = s.value(QStringLiteral("mainwindow/tableview%1").arg(m_Index), QVariant());
    QVariant geometry_data  = s.value(QStringLiteral("mainwindow/geometry%1").arg(m_Index), QVariant());
    QVariant notifier_data  = s.value(QStringLiteral("mainwindow/notifier%1").arg(m_Index), QVariant());
    QVariant receiver_data  = s.value(QStringLiteral("mainwindow/receiver%1").arg(m_Index), QVariant());
    QVariant menubar_data   = s.value(QStringLiteral("mainwindow/menubar%1").arg(m_Index), QVariant());
    QVariant toolbar_data   = s.value(QStringLiteral("mainwindow/toolbar%1").arg(m_Index), QVariant());
    QVariant treebar_data   = s.value(QStringLiteral("mainwindow/treebar%1").arg(m_Index), QVariant());

    m_InspectorDockPlaced = s.value(QStringLiteral("mainwindow/inspectordock%1").arg(m_Index), false).toBool();
    m_SidePanels->SetPlaced(s.value(QStringLiteral("mainwindow/sidepaneldock%1").arg(m_Index), 0).toInt());

    if(tableview_data.isNull()){

        QStringList list = current
            ? current->GetTreeBank()->GetGadgets()->GetStat()
            : QStringList()
            << QStringLiteral("%1").arg(static_cast<int>(GraphicsTableView::DefaultNodeCollectionType()))
            << QStringLiteral("%1").arg(1.0);

        m_TreeBank->GetGadgets()->SetStat(list);

    } else if(tableview_data.canConvert<QStringList>()){
        m_TreeBank->GetGadgets()->SetStat
            (tableview_data.toStringList());
    }

    if(geometry_data.isNull()){

        QRect rect = current
            ? current->geometry()
            : QRect();

        if(rect.isNull()){
            Application *instance = Application::GetInstance();
            if(instance->screens().length()){
                rect = instance->primaryScreen()->geometry();
                rect = QRect(rect.x() + rect.width()  / 6,
                             rect.y() + rect.height() / 6,
                             rect.width() * 2 / 3, rect.height() * 2 / 3);
            }
        } else {

            if(pos().isNull())
                rect.translate(current->ScaleByDevice(20), current->ScaleByDevice(20));
            else
                rect.moveTopLeft(pos());
        }

        if(pos().isNull())
            setGeometry(rect);
        else
            resize(rect.size());

    } else if(geometry_data.canConvert<QRect>()){
        setGeometry(geometry_data.toRect());
    }

    if(notifier_data.isNull()){

        bool notifierEnabled = current
            ? static_cast<bool>(current->GetTreeBank()->GetNotifier())
            : true;

        if(notifierEnabled != static_cast<bool>(GetTreeBank()->GetNotifier()))
            GetTreeBank()->ToggleNotifier();

    } else if(notifier_data.canConvert<bool>()){
        if(notifier_data.toBool() != static_cast<bool>(GetTreeBank()->GetNotifier()))
            GetTreeBank()->ToggleNotifier();
    }

    if(receiver_data.isNull()){

        bool receiverEnabled = current
            ? static_cast<bool>(current->GetTreeBank()->GetReceiver())
            : true;

        if(receiverEnabled != static_cast<bool>(GetTreeBank()->GetReceiver()))
            GetTreeBank()->ToggleReceiver();

    } else if(receiver_data.canConvert<bool>()){
        if(receiver_data.toBool() != static_cast<bool>(GetTreeBank()->GetReceiver()))
            GetTreeBank()->ToggleReceiver();
    }

    if(toolbar_data.isNull()){

        QByteArray state = current
            ? current->saveState()
            : QByteArray();

        if(!state.isEmpty())
            restoreState(state);

    } else if(toolbar_data.canConvert<QByteArray>()){
        restoreState(toolbar_data.toByteArray());
    }

    if(treebar_data.isNull()){

        QStringList stat = current
            ? current->GetTreeBar()->GetStat()
            : QStringList();

        if(!stat.isEmpty())
            m_TreeBar->SetStat(stat);
        else m_TreeBar->SetStat(stat);

    } else if(treebar_data.canConvert<QStringList>()){
        m_TreeBar->SetStat(treebar_data.toStringList());
    }

    if(menubar_data.isNull()){

        bool menubarEnabled = current
            ? !current->IsMenuBarEmpty()
            : true;

        if(menubarEnabled && IsMenuBarEmpty())
            CreateMenuBar();

    } else if(menubar_data.canConvert<bool>()){
        if(menubar_data.toBool() && IsMenuBarEmpty())
            CreateMenuBar();
    }

    QVariant status = s.value(QStringLiteral("mainwindow/status%1").arg(m_Index), QVariant());

    if(status.isNull()){
    } else if(status.canConvert<int>()){
        setWindowState(static_cast<Qt::WindowStates>(status.toInt()));
    }

    QTimer::singleShot(0, this, [this](){

    bool contains = false;
    Application *instance = Application::GetInstance();
    if(instance->screens().length()){
        for(int i = 0; i < instance->screens().length(); i++){
            if(instance->screens()[i]->geometry().intersects(geometry())){
                contains = true;
                break;
            }
        }
        if(!contains){
            QRect rect = instance->primaryScreen()->geometry();
            setGeometry(rect.x() + rect.width()  / 6,
                        rect.y() + rect.height() / 6,
                        rect.width() * 2 / 3, rect.height() * 2 / 3);
        }
    }
    });
}

void MainWindow::RemoveSettings(){
    Settings &s = Application::GlobalSettings();

    s.remove(QStringLiteral("mainwindow/tableview%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/geometry%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/notifier%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/receiver%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/menubar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/toolbar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/treebar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/status%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/inspectordock%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/sidepaneldock%1").arg(m_Index));
}

TreeBank *MainWindow::GetTreeBank() const {
    return m_TreeBank;
}

TreeBar *MainWindow::GetTreeBar() const {
    return m_TreeBar;
}

ToolBar *MainWindow::GetToolBar() const {
    return m_ToolBar;
}

ModelessDialogFrame *MainWindow::DialogFrame() const {
    return m_DialogFrame;
}

QWidget *MainWindow::DockedInspectorPane() const {
    return m_InspectorDock->widget();
}

void MainWindow::SetInspectorPane(QWidget *pane, bool request){
    QWidget *old = m_InspectorDock->widget();

    if(old != pane){
        if(old){
            m_InspectorDock->setWidget(nullptr);
            old->setParent(nullptr);
            old->hide();
        }
        if(pane) m_InspectorDock->setWidget(pane);
    }

    if(request) m_InspectorDockClosed = false;

    UpdateInspectorDock();

#if defined(Q_OS_WIN) && defined(EDGEWEBVIEW)
    static bool driven = false;
    if(pane && old != pane && !driven && qEnvironmentVariableIsSet("VANILLA_EDGE_INSPECTOR_SELFTEST")){
        driven = true;
        QTimer::singleShot(9000, this, [this](){ m_InspectorDock->setFloating(true);});
        QTimer::singleShot(20000, this, [this](){ m_InspectorDock->setFloating(false);});
    }
#endif

    if(pane && !m_InspectorDockPlaced && m_InspectorDock->isVisible()){
        if(!m_InspectorDock->isFloating())
            resizeDocks(QList<QDockWidget*>() << m_InspectorDock,
                        QList<int>() << width() / 3, Qt::Horizontal);
        m_InspectorDockPlaced = true;
    }
}

void MainWindow::CloseInspectorPane(){
    m_InspectorDock->close();
}

void MainWindow::SuspendInspectorPane(bool suspend){
    if(m_InspectorDockSuspended == suspend) return;
    m_InspectorDockSuspended = suspend;
    UpdateInspectorDock();
}

void MainWindow::UpdateInspectorDock(){
    const bool visible = m_InspectorDock->widget()
        && !m_InspectorDockClosed
        && !m_InspectorDockSuspended;

    if(visible == m_InspectorDock->isVisible()) return;
    if(visible) m_InspectorDock->show();
    else        m_InspectorDock->hide();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *ev){
    if(watched == m_InspectorDock && ev->type() == QEvent::Close)
        m_InspectorDockClosed = true;
#if defined(Q_OS_WIN) && defined(EDGEWEBVIEW)
    if(watched == this && ev->type() == QEvent::WinIdChange &&
       qEnvironmentVariableIsSet("VANILLA_EDGE_INSPECTOR_TRACE")){
        fprintf(stderr, "edge-inspector: mainwindow WinIdChange hwnd=%p\n",
                reinterpret_cast<void*>(internalWinId()));
        fflush(stderr);
    }
    if(watched == m_InspectorDock && m_InspectorDock->isFloating() &&
       (ev->type() == QEvent::Show || ev->type() == QEvent::WinIdChange))
        QTimer::singleShot(0, this, [this](){ ApplyInspectorDockFrame();});
#endif
    return QMainWindow::eventFilter(watched, ev);
}

#if defined(Q_OS_WIN) && defined(EDGEWEBVIEW)
void MainWindow::ApplyInspectorDockFrame(){
    if(!m_InspectorDock->isFloating() || !m_InspectorDock->internalWinId()) return;
    const HWND hwnd = reinterpret_cast<HWND>(m_InspectorDock->internalWinId());
    const BOOL dark = Theme::IsDark() ? TRUE : FALSE;
    const HRESULT hr = ::DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    if(FAILED(hr) && qEnvironmentVariableIsSet("VANILLA_EDGE_INSPECTOR_TRACE"))
        fprintf(stderr, "edge-inspector: DwmSetWindowAttribute failed hr=%08lx\n", static_cast<unsigned long>(hr));
}
#endif

bool MainWindow::IsMenuBarEmpty() const {
    return menuBar()->actions().isEmpty();
}

void MainWindow::ClearMenuBar(){
    menuBar()->clear();
    menuBar()->hide();
}

QMenu *MainWindow::createPopupMenu(){
    QMenu *menu = new QMenu(this);
    AddDisplayMenuActions(menu);
    return menu;
}

void MainWindow::AddDisplayMenuActions(QMenu *menu){
    menu->addAction(m_TreeBank->Action(TreeBank::_ToggleMenuBar));
    menu->addAction(m_TreeBank->Action(TreeBank::_ToggleTreeBar));
    menu->addAction(m_TreeBank->Action(TreeBank::_ToggleToolBar));
    QAction *inspector = menu->addAction(tr("Inspector"));
    inspector->setCheckable(true);
    inspector->setChecked(m_InspectorDock->isVisible());
    connect(m_InspectorDock, &QDockWidget::visibilityChanged,
            inspector, &QAction::setChecked);
    connect(menu, &QMenu::aboutToShow, inspector, [this, inspector](){
        inspector->setChecked(m_InspectorDock->isVisible());
    });
    connect(inspector, &QAction::triggered, this, [this](){
        if(!m_InspectorDock->isVisible()) m_TreeBank->InspectElement();
        else CloseInspectorPane();
    });
}

void MainWindow::CreateMenuBar(){
    menuBar()->addMenu(m_TreeBank->ApplicationMenu(true));
    menuBar()->addMenu(m_TreeBank->NodeMenu());
    menuBar()->addMenu(m_TreeBank->DisplayMenu());
    menuBar()->addMenu(m_TreeBank->WindowMenu());
    menuBar()->addMenu(m_TreeBank->PageMenu());
    menuBar()->show();
}

bool MainWindow::IsContentFullScreen() const {
    return m_ContentFullScreen;
}

bool MainWindow::IsShaded() const {
    return windowOpacity() == 0.0;
}

void MainWindow::Shade(){
    if(!m_TitleBar) return;
    setWindowOpacity(0.0);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            if(0);
#ifdef WEBENGINEVIEW
            else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->hide();
            else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                w->hide();
#endif
#ifdef NATIVEWEBVIEW
            else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                w->hide();
#endif
#ifdef EDGEWEBVIEW
            else if(EdgeWebView *w = qobject_cast<EdgeWebView*>(view->base()))
                w->hide();
#endif
            else;
        }
    }
    AdjustAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->hide();
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged()) receiver->hide();
}

void MainWindow::Unshade(){
    if(!m_TitleBar) return;
    setWindowOpacity(1.0);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            if(0);
#ifdef WEBENGINEVIEW
            else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->show();
            else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                w->show();
#endif
#ifdef NATIVEWEBVIEW
            else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                w->show();
#endif
#ifdef EDGEWEBVIEW
            else if(EdgeWebView *w = qobject_cast<EdgeWebView*>(view->base()))
                w->show();
#endif
            else;
        }
    }
    AdjustAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->show();
    SetFocus();
}

void MainWindow::ShowWindowFrameWidgets(){
    AdjustAllEdgeWidgets();
}

void MainWindow::HideWindowFrameWidgets(){
    if(!m_TitleBar) return;
    m_TitleBarTimer->stop();
    m_TitleBarReveal.Reset();
    m_TitleBar        ->hide();
    m_NorthWidget     ->hide();
    m_SouthWidget     ->hide();
    m_WestWidget      ->hide();
    m_EastWidget      ->hide();
    m_NorthWestWidget ->hide();
    m_NorthEastWidget ->hide();
    m_SouthWestWidget ->hide();
    m_SouthEastWidget ->hide();
}

void MainWindow::ShowAllEdgeWidgets(){
    m_DialogFrame->show();
    ShowWindowFrameWidgets();
}

void MainWindow::HideAllEdgeWidgets(){
    m_DialogFrame->hide();
    HideWindowFrameWidgets();
}

void MainWindow::RaiseAllEdgeWidgets(){
    m_DialogFrame->raise();
    if(!m_TitleBar || !m_TitleBar->isVisible()) return;
    m_NorthWidget     ->raise();
    m_SouthWidget     ->raise();
    m_WestWidget      ->raise();
    m_EastWidget      ->raise();

    m_TitleBar        ->raise();

    m_NorthWestWidget ->raise();
    m_NorthEastWidget ->raise();
    m_SouthWestWidget ->raise();
    m_SouthEastWidget ->raise();
}

void MainWindow::AdjustAllEdgeWidgets(){
    QRect rect = geometry();
    const int dialogWidth = rect.width() / MODELESS_DIALOG_WIDTH_DIVISOR;
    m_DialogFrame->setGeometry(rect.left() + rect.width() - dialogWidth, rect.top(),
                               dialogWidth, m_DialogFrame->height());
    m_DialogFrame->Adjust();
    if(!m_TitleBar) return;
    const bool frameVisible = isVisible() && !isMinimized() &&
                              !isFullScreen() && !m_ContentFullScreen;
    const bool edgesVisible = frameVisible && !isMaximized();
    for(QWidget *edge : QList<QWidget*>{m_NorthWidget, m_SouthWidget, m_WestWidget, m_EastWidget,
                         m_NorthWestWidget, m_NorthEastWidget,
                         m_SouthWestWidget, m_SouthEastWidget})
        edge->setVisible(edgesVisible);
    if(IsShaded()) rect.setHeight(0);
    const int e = ScaleByDevice(EDGE_WIDGET_SIZE);
    const int t = ScaleByDevice(TITLE_BAR_HEIGHT);
    const int et = e+t;
    m_NorthWidget     ->setGeometry(QRect(rect.left()-1, rect.top()-et, rect.width()+2, e));
    m_SouthWidget     ->setGeometry(QRect(rect.left()-1, rect.bottom()+1, rect.width()+2, e));
    m_WestWidget      ->setGeometry(QRect(rect.left()-e, rect.top()-t-1, e, rect.height()+t+2));
    m_EastWidget      ->setGeometry(QRect(rect.right()+1, rect.top()-t-1, e, rect.height()+t+2));

    const int c = ScaleByDevice(3);
    m_NorthWestWidget ->setGeometry(QRect(rect.left()-e, rect.top()-et, e+c, e+c));
    m_NorthEastWidget ->setGeometry(QRect(rect.right()+1-c, rect.top()-et, e+c, e+c));
    m_SouthWestWidget ->setGeometry(QRect(rect.left()-e, rect.bottom()+1-c, e+c, e+c));
    m_SouthEastWidget ->setGeometry(QRect(rect.right()+1-c, rect.bottom()+1-c, e+c, e+c));
    if(frameVisible && isMaximized()){
        if(!m_TitleBarTimer->isActive()) m_TitleBarTimer->start();
    } else {
        m_TitleBarTimer->stop();
    }
    UpdateTitleBarVisibility();
}

bool MainWindow::IsFrameActive() const {
    QWidget *purged = nullptr;
    if(TreeBank::PurgeView() && m_TreeBank){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            if(view->visible())
                purged = qobject_cast<QWidget*>(view->base());
        }
    }
#if defined(Q_OS_WIN)
    const HWND foreground = GetForegroundWindow();
    if(!foreground) return false;
    const HWND root = GetAncestor(foreground, GA_ROOTOWNER);
    for(const QWidget *widget : {static_cast<const QWidget*>(this),
                                 static_cast<const QWidget*>(m_TitleBar),
                                 static_cast<const QWidget*>(purged)}){
        if(widget && widget->internalWinId() &&
           root == reinterpret_cast<HWND>(widget->window()->internalWinId()))
            return true;
    }
    return false;
#else
    return isActiveWindow() || (m_TitleBar && m_TitleBar->isActiveWindow()) ||
           (purged && purged->isActiveWindow());
#endif
}

void MainWindow::UpdateTitleBarVisibility(){
    if(!m_TitleBar) return;
    ApplyTitleBarVisibility(QCursor::pos(), IsFrameActive(),
                           QApplication::mouseButtons() != Qt::NoButton);
}

void MainWindow::ApplyTitleBarVisibility(const QPoint &cursor, bool active, bool buttonsDown){
    if(!m_TitleBar) return;
    const QRect rect = geometry();
    const int t = m_TitleBar->ScaleByDevice(TITLE_BAR_HEIGHT);
    const int titleWidth = isMaximized() ? qMin(rect.width(), m_TitleBar->CompactWidth()) : rect.width();
    const int titleTop = isMaximized() ? rect.top() : rect.top()-t;
    m_TitleBar->setGeometry(rect.right()+1-titleWidth, titleTop, titleWidth, t);
    const bool frameVisible = isVisible() && !isMinimized() &&
                              !isFullScreen() && !m_ContentFullScreen;
    const bool enabled = frameVisible && isMaximized();
    const QRect monitor = screen() ? screen()->geometry() : QRect();
    const QRect strip = QRect(QPoint(geometry().left(), monitor.top()),
                               QPoint(geometry().right(), m_TitleBar->geometry().bottom()))
                            .intersected(monitor);
    const bool revealed = m_TitleBarReveal.Update(enabled, active, strip, cursor, buttonsDown);
    const bool visible = frameVisible && (!isMaximized() || revealed);
    if(visible != m_TitleBar->isVisible()){
        m_TitleBar->setVisible(visible);
        if(visible){
            RaiseAllEdgeWidgets();
            m_TitleBar->update();
        }
    }
}

void MainWindow::UpdateAllEdgeWidgets(){
    if(!m_TitleBar) return;
    m_TitleBar        ->update();
    m_NorthWidget     ->update();
    m_SouthWidget     ->update();
    m_WestWidget      ->update();
    m_EastWidget      ->update();
    m_NorthWestWidget ->update();
    m_NorthEastWidget ->update();
    m_SouthWestWidget ->update();
    m_SouthEastWidget ->update();
}

void MainWindow::SetWindowTitle(const QString &title){
    if(title.isEmpty())
        setWindowTitle(QStringLiteral("vanilla"));
    else if(m_TitleBar)
        setWindowTitle(title);
    else
        setWindowTitle(QStringLiteral("vanilla - ") + title);

    if(m_TitleBar) m_TitleBar->repaint();
}

void MainWindow::ToggleNotifier(){
    m_TreeBank->ToggleNotifier();
}

void MainWindow::ToggleReceiver(){
    m_TreeBank->ToggleReceiver();
}

void MainWindow::ToggleMenuBar(){
    if(IsMenuBarEmpty()){
        CreateMenuBar();
    } else {
        ClearMenuBar();
    }
}

void MainWindow::ToggleTreeBar(){
    if(m_TreeBar->isVisible()){
        m_TreeBar->hide();
    } else {
        m_TreeBar->show();
    }
}

void MainWindow::ToggleToolBar(){
    if(m_ToolBar->isVisible()){
        m_ToolBar->hide();
    } else {
        m_ToolBar->show();
    }
}

void MainWindow::ToggleFullScreen(){
    if(m_ContentFullScreen){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            view->ExitFullScreen();
            return;
        }
        SetFullScreen(false);
        return;
    }
    if(isFullScreen()){
        showNormal();
    } else {
        showFullScreen();
    }
    SetFocus();
}

void MainWindow::ToggleMaximized(){
    if(isMaximized()){
        showNormal();
    } else {
        showMaximized();
    }
    SetFocus();
}

void MainWindow::ToggleMinimized(){
    if(isMinimized()){
        showNormal();
        SetFocus();
    } else {
        showMinimized();
    }
}

void MainWindow::ToggleShaded(){
    if(IsShaded()){
        Unshade();
        SetFocus();
    } else {
        Shade();
    }
}

void MainWindow::SetMenuBar(bool on){
    if(on && IsMenuBarEmpty()){
        CreateMenuBar();
    } else if(!on && !IsMenuBarEmpty()){
        ClearMenuBar();
    }
}

void MainWindow::SetTreeBar(bool on){
    if(on && m_TreeBar->isHidden()){
        m_TreeBar->show();
    } else if(!on && !m_TreeBar->isVisible()){
        m_TreeBar->hide();
    }
}

void MainWindow::SetToolBar(bool on){
    if(on && m_ToolBar->isHidden()){
        m_ToolBar->show();
    } else if(!on && !m_ToolBar->isVisible()){
        m_ToolBar->hide();
    }
}

void MainWindow::SetFullScreen(bool on){
    if(on == m_ContentFullScreen) return;

    if(on){
        m_StateBeforeFullScreen = windowState();
        m_MenuBarVisibleBeforeFullScreen = !menuBar()->isHidden();
        m_TreeBarVisibleBeforeFullScreen = !m_TreeBar->isHidden();
        m_ToolBarVisibleBeforeFullScreen = !m_ToolBar->isHidden();
        m_ContentFullScreen = true;

        menuBar()->hide();
        m_TreeBar->hide();
        m_ToolBar->hide();
        HideWindowFrameWidgets();
        m_TreeBank->SetMiniMapShelved(true);

        if(!isFullScreen()) showFullScreen();

    } else {
        m_ContentFullScreen = false;

        if(m_MenuBarVisibleBeforeFullScreen) menuBar()->show();
        if(m_TreeBarVisibleBeforeFullScreen) m_TreeBar->show();
        if(m_ToolBarVisibleBeforeFullScreen) m_ToolBar->show();
        ShowWindowFrameWidgets();
        m_TreeBank->SetMiniMapShelved(false);

        if(!(m_StateBeforeFullScreen & Qt::WindowFullScreen)){
            if(m_StateBeforeFullScreen & Qt::WindowMaximized)
                showMaximized();
            else
                showNormal();
        }
    }
    AdjustAllEdgeWidgets();
    RaiseAllEdgeWidgets();
    SetFocus();
}

void MainWindow::SetMaximized(bool on){
    if(on && !isMaximized()){
        showMaximized();
    } else if(!on && isMaximized()){
        showNormal();
    }
    SetFocus();
}

void MainWindow::SetMinimized(bool on){
    if(on && !isMinimized()){
        showMinimized();
    } else if(!on && isMinimized()){
        showNormal();
        SetFocus();
    }
}

void MainWindow::SetShaded(bool on){
    if(on && !IsShaded()){
        Shade();
    } else if(!on && IsShaded()){
        Unshade();
        SetFocus();
    }
}

void MainWindow::SetFocus(){
    raise();
    ActivateIfNeeded();
    if(GetTreeBank()->GetGadgets()->IsActive()){
        GetTreeBank()->GetView()->setFocus();
        GetTreeBank()->GetGadgets()->setFocus();
    } else if(SharedView view = GetTreeBank()->GetCurrentView()){
        if(TreeBank::PurgeView())
            if(QWidget *w = qobject_cast<QWidget*>(view->base()))
                ActivateWindowIfNeeded(w);
        view->setFocus();
    }
    RaiseAllEdgeWidgets();
    UpdateAllEdgeWidgets();
}

void MainWindow::ActivateWindowIfNeeded(QWidget *window){
#if defined(Q_OS_WIN)
    ActivateWindowIfNeeded(window, reinterpret_cast<WId>(GetForegroundWindow()));
#else
    if(window) window->activateWindow();
#endif
}

void MainWindow::ActivateWindowIfNeeded(QWidget *window, WId front){
    if(!window) return;
    const WId id = window->window()->internalWinId();
    if(id && id == front) return;
    window->activateWindow();
}

void MainWindow::ActivateIfNeeded(){
    ActivateWindowIfNeeded(this);
}

void MainWindow::changeEvent(QEvent *ev){
    QMainWindow::changeEvent(ev);
    if(ev->type() == QEvent::WindowStateChange){
        AdjustAllEdgeWidgets();
        RaiseAllEdgeWidgets();
    } else if(ev->type() == QEvent::ActivationChange){
        UpdateTitleBarVisibility();
        if(isActiveWindow()){
            RaiseAllEdgeWidgets();
            UpdateAllEdgeWidgets();
        }
    }
}

void MainWindow::paintEvent(QPaintEvent *ev){
    QMainWindow::paintEvent(ev);
}

void MainWindow::closeEvent(QCloseEvent *ev){
    if(m_SidePanels) m_SidePanels->ShutdownPages();

    if(Application::GetMainWindows().count() == 1){
        QMainWindow::closeEvent(ev);
        Application::Quit();
    } else {
        TreeBank::ChangeScope changing;
        Application::RemoveWindow(this);

        foreach(QObject *child, m_TreeBank->children()){
            if(View *view = dynamic_cast<View*>(child)){
                view->setParent(Application::GetCurrentWindow()->GetTreeBank());
                view->SetTreeBank(Application::GetCurrentWindow()->GetTreeBank());
                view->lower();
                view->show();
                view->hide();
            }
        }
        foreach(QGraphicsItem *item, m_TreeBank->GetScene()->items()){
            if(View *view = dynamic_cast<View*>(item)){
                view->setParent(nullptr);
            }
        }
        foreach(QObject *child, windowHandle()->children()){
            if(View *view = dynamic_cast<View*>(child)){
                view->setParent(Application::GetCurrentWindow()->GetTreeBank());
                view->SetTreeBank(Application::GetCurrentWindow()->GetTreeBank());
                view->hide();
            }
        }

        QMainWindow::closeEvent(ev);
        RemoveSettings();
        Application::GetAutoSaver()->SaveAllAsync();
        deleteLater();
    }
    ev->setAccepted(true);
}

void MainWindow::resizeEvent(QResizeEvent *ev){
    QMainWindow::resizeEvent(ev);
    AdjustAllEdgeWidgets();
}

void MainWindow::moveEvent(QMoveEvent *ev){
    QMainWindow::moveEvent(ev);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            QRect rect = QRect(m_TreeBank->mapToGlobal(QPoint()), m_TreeBank->size());
            if(0);
#ifdef WEBENGINEVIEW
            else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->setGeometry(rect);
            else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                w->setGeometry(rect);
#endif
#ifdef NATIVEWEBVIEW
            else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                w->setGeometry(rect);
#endif
#ifdef EDGEWEBVIEW
            else if(EdgeWebView *w = qobject_cast<EdgeWebView*>(view->base()))
                w->setGeometry(rect);
#endif
            else;
        }
    }
    AdjustAllEdgeWidgets();
    RaiseAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged())
        notifier->ResizeNotify(m_TreeBank->size());
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged())
        receiver->ResizeNotify(m_TreeBank->size());
    m_TreeBank->RestackChildWidgets();
}

void MainWindow::showEvent(QShowEvent *ev){
    QMainWindow::showEvent(ev);
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            if(0);
#ifdef WEBENGINEVIEW
            else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->show();
            else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                w->show();
#endif
#ifdef NATIVEWEBVIEW
            else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                w->show();
#endif
#ifdef EDGEWEBVIEW
            else if(EdgeWebView *w = qobject_cast<EdgeWebView*>(view->base()))
                w->show();
#endif
            else;
        }
    }
    ShowAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->show();
}

void MainWindow::hideEvent(QHideEvent *ev){
    if(TreeBank::PurgeView()){
        if(SharedView view = m_TreeBank->GetCurrentView()){
            if(0);
#ifdef WEBENGINEVIEW
            else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->show();
            else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                w->show();
#endif
#ifdef NATIVEWEBVIEW
            else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                w->show();
#endif
#ifdef EDGEWEBVIEW
            else if(EdgeWebView *w = qobject_cast<EdgeWebView*>(view->base()))
                w->show();
#endif
            else;
        }
    }
    HideAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->hide();
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged()) receiver->hide();
    QMainWindow::hideEvent(ev);
}

namespace {
    inline QPen   GhostPen()   { return Theme::Pen(Theme::WindowEdgeGhost);}
    inline QBrush GhostBrush() { return Theme::Brush(Theme::WindowEdgeGhost);}
    inline QPen   EdgePen()    { return Theme::Pen(Theme::WindowEdgeLine);}
    static const int none   = 0;
    static const int line   = 1 << 0;
    static const int edge   = 1 << 1;
    static const int shadow = 1 << 2;
    static const int disp = shadow;
}

TitleBar::TitleBar(MainWindow *mainwindow)
    : QWidget(nullptr)
    , m_MainWindow(mainwindow)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    m_HoveredButton = NoButton;
    LoadIcons();
}

void TitleBar::LoadIcons(){
    QStyle *style = Application::style();
    const QSize minimize = MinimizeAreaRect().size();
    const QSize maximize = MaximizeAreaRect().size();

    m_Shade    = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarShadeButton).pixmap(minimize), Theme::TitleBarIcon);
    m_Unshade  = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarUnshadeButton).pixmap(minimize), Theme::TitleBarIcon);
    m_Minimize = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarMinButton).pixmap(minimize), Theme::TitleBarIcon);
    m_Maximize = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarMaxButton).pixmap(maximize), Theme::TitleBarIcon);
    m_Normal   = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarNormalButton).pixmap(maximize), Theme::TitleBarIcon);
    m_Close    = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarCloseButton).pixmap(CloseAreaRect().size()), Theme::TitleBarIcon);

    m_IconScheme = Theme::CurrentScheme();
}

void TitleBar::paintEvent(QPaintEvent *ev){
    if(m_IconScheme != Theme::CurrentScheme()) LoadIcons();

    QPainter painter(this);

    bool isCurrent = m_MainWindow == Application::GetCurrentWindow();

    painter.setPen(Theme::Pen(Theme::TitleBarBorder));
    painter.setBrush(Theme::Brush(Theme::TitleBarBackground, isCurrent ? 200 : 128));
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));

    if(!m_MainWindow->isMaximized()){
        painter.setPen(Theme::Pen(Theme::TitleBarButton));
        painter.setBrush(Theme::Brush(Theme::TitleBarButton));
        painter.drawRect(MenuAreaRect());
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::TitleBarButtonHovered));
        if(m_HoveredButton == MenuButton) painter.drawRect(MenuAreaRect1());
    }

    if(m_MainWindow->isMaximized() || width() > ButtonAreaWidth(4)){
        painter.setPen(Theme::Pen(Theme::TitleBarButton));
        painter.setBrush(Theme::Brush(Theme::TitleBarButton));
        painter.drawRect(ViewTreeAreaRect());

        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::TitleBarButtonHovered));
        if(m_HoveredButton == ViewTreeButton) painter.drawRect(ViewTreeAreaRect1());
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::Brush(Theme::TitleBarButtonHovered));

    if(m_MainWindow->isMaximized() || width() > ButtonAreaWidth(3)){
        if(m_MainWindow->IsShaded())
            painter.drawPixmap(ShadeAreaRect(), m_Unshade, QRect(QPoint(), m_Unshade.size()));
        else painter.drawPixmap(ShadeAreaRect(), m_Shade, QRect(QPoint(), m_Shade.size()));
        if(m_HoveredButton == ShadeButton) painter.drawRect(ShadeAreaRect1());
    }
    if(m_MainWindow->isMaximized() || width() > ButtonAreaWidth(2)){
        if(m_MainWindow->isMinimized())
            painter.drawPixmap(MinimizeAreaRect(), m_Normal, QRect(QPoint(), m_Normal.size()));
        else painter.drawPixmap(MinimizeAreaRect(), m_Minimize, QRect(QPoint(), m_Minimize.size()));
        if(m_HoveredButton == MinimizeButton) painter.drawRect(MinimizeAreaRect1());
    }
    if(m_MainWindow->isMaximized() || width() > ButtonAreaWidth(1)){
        if(m_MainWindow->isMaximized())
            painter.drawPixmap(MaximizeAreaRect(), m_Normal, QRect(QPoint(), m_Normal.size()));
        else painter.drawPixmap(MaximizeAreaRect(), m_Maximize, QRect(QPoint(), m_Maximize.size()));
        if(m_HoveredButton == MaximizeButton) painter.drawRect(MaximizeAreaRect1());
    }

    painter.drawPixmap(CloseAreaRect(), m_Close, QRect(QPoint(), m_Close.size()));
    if(m_HoveredButton == CloseButton) painter.drawRect(CloseAreaRect1());

    if(!m_MainWindow->isMaximized()){
        painter.setFont(TitleBarTitleFont());
        painter.setPen(Theme::Pen(Theme::TitleBarText));
        painter.setBrush(Qt::NoBrush);
        painter.drawText(QRect(ScaleByDevice(22), 0,
                               qMax(0, ViewTreeAreaRect1().left()-ScaleByDevice(26)), height()),
                         Qt::TextSingleLine | Qt::AlignVCenter,
                         m_MainWindow->windowTitle());
    }
    ev->setAccepted(true);
}

void TitleBar::mousePressEvent(QMouseEvent *ev){
    m_MainWindow->raise();
    m_Moved = false;
    if(ev->button() == Qt::LeftButton){
        m_Pos = ev->globalPosition().toPoint() - geometry().topLeft() - QPoint(0,t);
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        m_Moved = true;
        if(m_HoveredButton == NoButton)
            m_MainWindow->move(ev->globalPosition().toPoint() - m_Pos);
        return;
    }
    HoveredButton before = m_HoveredButton;
    if(MenuAreaRect1().contains(ev->pos())){
        m_HoveredButton = MenuButton;
    } else if(ViewTreeAreaRect1().contains(ev->pos())){
        m_HoveredButton = ViewTreeButton;
    } else if(ShadeAreaRect1().contains(ev->pos())){
        m_HoveredButton = ShadeButton;
    } else if(MinimizeAreaRect1().contains(ev->pos())){
        m_HoveredButton = MinimizeButton;
    } else if(MaximizeAreaRect1().contains(ev->pos())){
        m_HoveredButton = MaximizeButton;
    } else if(CloseAreaRect1().contains(ev->pos())){
        m_HoveredButton = CloseButton;
    } else {
        m_HoveredButton = NoButton;
    }
    if(before != m_HoveredButton) repaint();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *ev){
    if(m_Moved){
        m_MainWindow->SetFocus();
        m_Moved = false;
        ev->setAccepted(true);
    } else if(ev->button() == Qt::LeftButton){
        if(MenuAreaRect1().contains(ev->pos())){
            QMenu *menu = m_MainWindow->GetTreeBank()->GlobalContextMenu();
            menu->exec(ev->globalPosition().toPoint());
            delete menu;
            m_MainWindow->SetFocus();
        } else if(ViewTreeAreaRect1().contains(ev->pos())){
            Gadgets *g = m_MainWindow->GetTreeBank()->GetGadgets();
            if(g && g->IsActive()) g->Deactivate();
            else m_MainWindow->GetTreeBank()->DisplayViewTree();
            m_MainWindow->SetFocus();
        } else if(ShadeAreaRect1().contains(ev->pos())){
            m_MainWindow->ToggleShaded();
        } else if(MinimizeAreaRect1().contains(ev->pos())){
            m_MainWindow->ToggleMinimized();
        } else if(MaximizeAreaRect1().contains(ev->pos())){
            m_MainWindow->ToggleMaximized();
        } else if(CloseAreaRect1().contains(ev->pos())){
            m_MainWindow->close();
        } else {
            m_MainWindow->SetFocus();
        }
    } else if(ev->button() == Qt::RightButton){
        m_MainWindow->ToggleShaded();
    }
    repaint();
    ev->setAccepted(true);
    m_Moved = false;
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *ev){
    if(MenuAreaRect1().contains(ev->pos()) ||
       ViewTreeAreaRect1().contains(ev->pos()) ||
       ShadeAreaRect1().contains(ev->pos()) ||
       MinimizeAreaRect1().contains(ev->pos()) ||
       MaximizeAreaRect1().contains(ev->pos()) ||
       CloseAreaRect1().contains(ev->pos())){
    } else if(ev->button() == Qt::LeftButton){
        m_MainWindow->ToggleMaximized();
    }
    ev->setAccepted(true);
}

void TitleBar::enterEvent(QEnterEvent *ev)
{
    Q_UNUSED(ev)
}

void TitleBar::leaveEvent(QEvent *ev){
    Q_UNUSED(ev)
    m_HoveredButton = NoButton;
    repaint();
}

QRect TitleBar::MenuAreaRect() const {
    return QRect(ScaleByDevice(5),(height()-ScaleByDevice(10))/2,ScaleByDevice(10),ScaleByDevice(10));
}

QRect TitleBar::ViewTreeAreaRect() const {
    return QRect(width()-ScaleByDevice(29)-ScaleByDevice(28)*4,(height()-ScaleByDevice(10))/2,ScaleByDevice(10),ScaleByDevice(10));
}

QRect TitleBar::ShadeAreaRect() const {
    return QRect(width()-ScaleByDevice(28)-ScaleByDevice(28)*3,(height()-ScaleByDevice(10))/2,ScaleByDevice(10),ScaleByDevice(10));
}

QRect TitleBar::MinimizeAreaRect() const {
    return QRect(width()-ScaleByDevice(28)-ScaleByDevice(28)*2,(height()-ScaleByDevice(10))/2,ScaleByDevice(10),ScaleByDevice(10));
}

QRect TitleBar::MaximizeAreaRect() const {
    return QRect(width()-ScaleByDevice(28)-ScaleByDevice(28)*1,(height()-ScaleByDevice(10))/2,ScaleByDevice(10),ScaleByDevice(10));
}

QRect TitleBar::CloseAreaRect() const {
    return QRect(width()-ScaleByDevice(27)-ScaleByDevice(28)*0,(height()-ScaleByDevice(10))/2,ScaleByDevice(10),ScaleByDevice(10));
}

MainWindowEdgeWidget::MainWindowEdgeWidget(MainWindow *mainwindow)
    : QWidget(nullptr)
    , m_MainWindow(mainwindow)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

void MainWindowEdgeWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(GhostPen());
    painter.setBrush(GhostBrush());
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::mousePressEvent(QMouseEvent *ev){
    m_MainWindow->raise();
    if(ev->button() == Qt::LeftButton)
        m_Pos = ev->pos();
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        QRect rect = m_MainWindow->geometry();
        QPoint pos = mapToGlobal(ev->pos());
        rect = ComputeNewRect(rect, pos);
        m_MainWindow->setGeometry(rect);
    }
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::mouseReleaseEvent(QMouseEvent *ev){
    m_MainWindow->SetFocus();
    m_Pos = QPoint();
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::enterEvent(QEnterEvent *ev)
{
    setCursor(CursorShape());
    ev->setAccepted(true);
}

void MainWindowEdgeWidget::leaveEvent(QEvent *ev){
    setCursor(Qt::ArrowCursor);
    ev->setAccepted(true);
}

QRect MainWindowNorthWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 pos.y() + et - m_Pos.y(),
                 rect.width(),
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowSouthWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 rect.width(),
                 pos.y() - rect.top() - m_Pos.y());
}

QRect MainWindowWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 rect.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 rect.height());
}

QRect MainWindowEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 pos.x() - rect.left() - m_Pos.x(),
                 rect.height());
}

QRect MainWindowNorthWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 pos.y() + et - m_Pos.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowNorthEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 pos.y() + et - m_Pos.y(),
                 pos.x() - rect.left() - m_Pos.x() + ScaleByDevice(3),
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowSouthWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 rect.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 pos.y() - rect.top() - m_Pos.y() + ScaleByDevice(3));
}

QRect MainWindowSouthEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 pos.x() - rect.left() - m_Pos.x() + ScaleByDevice(3),
                 pos.y() - rect.top() - m_Pos.y() + ScaleByDevice(3));
}

#define EDGE_SHADOW_COLOR qRgba(0,0,0,static_cast<int>(c))

void MainWindowNorthWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(GhostPen());
        painter.setBrush(GhostBrush());
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                const double f = ScaleByDevice(25);
                for(i=0; i < width(); i++){
                    for(j=0; j < height(); j++){
                        w = width();
                        c = 77*j*j/e/e;
                        if(  i<f) c = c*sqrt(   i /f);
                        if(w-i<f) c = c*sqrt((w-i)/f);
                        image.setPixel(static_cast<int>(i), static_cast<int>(j), EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(GhostPen());
                painter.setBrush(GhostBrush());
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(EdgePen());
            painter.setBrush(GhostBrush());
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(EdgePen());
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,e-1), QPoint(width(),e-1));
        }
    }
    ev->setAccepted(true);
}

void MainWindowSouthWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(GhostPen());
        painter.setBrush(GhostBrush());
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                const double f = ScaleByDevice(25);
                for(i=0; i < width(); i++){
                    for(j=0; j < height(); j++){
                        w = width();
                        c = 77*j*j/e/e;
                        if(  i<f) c = c*sqrt(   i /f);
                        if(w-i<f) c = c*sqrt((w-i)/f);
                        image.setPixel(static_cast<int>(i), static_cast<int>(e-j-1), EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(GhostPen());
                painter.setBrush(GhostBrush());
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(EdgePen());
            painter.setBrush(GhostBrush());
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(EdgePen());
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,0), QPoint(width(),0));
        }
    }
    ev->setAccepted(true);
}

void MainWindowWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(GhostPen());
        painter.setBrush(GhostBrush());
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                const double f = ScaleByDevice(25);
                for(i=0; i < height(); i++){
                    for(j=0; j < width(); j++){
                        w = height();
                        c = 77*j*j/e/e;
                        if(  i<f) c = c*sqrt(   i /f);
                        if(w-i<f) c = c*sqrt((w-i)/f);
                        image.setPixel(static_cast<int>(j), static_cast<int>(i), EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(GhostPen());
                painter.setBrush(GhostBrush());
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(EdgePen());
            painter.setBrush(GhostBrush());
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(EdgePen());
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(e-1,0), QPoint(e-1,height()));
        }
    }
    ev->setAccepted(true);
}

void MainWindowEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(GhostPen());
        painter.setBrush(GhostBrush());
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                const double f = ScaleByDevice(25);
                for(i=0; i < height(); i++){
                    for(j=0; j < width(); j++){
                        w = height();
                        c = 77*j*j/e/e;
                        if(  i<f) c = c*sqrt(   i /f);
                        if(w-i<f) c = c*sqrt((w-i)/f);
                        image.setPixel(static_cast<int>(e-j-1), static_cast<int>(i), EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(GhostPen());
                painter.setBrush(GhostBrush());
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(EdgePen());
            painter.setBrush(GhostBrush());
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(EdgePen());
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,0), QPoint(0,height()));
        }
    }
    ev->setAccepted(true);
}

void MainWindowNorthWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? EdgePen() : GhostPen());
    painter.setBrush(GhostBrush());
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowNorthEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? EdgePen() : GhostPen());
    painter.setBrush(GhostBrush());
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowSouthWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? EdgePen() : GhostPen());
    painter.setBrush(GhostBrush());
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowSouthEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? EdgePen() : GhostPen());
    painter.setBrush(GhostBrush());
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

Qt::CursorShape MainWindowNorthWidget::CursorShape(){ return Qt::SizeVerCursor; }
Qt::CursorShape MainWindowSouthWidget::CursorShape(){ return Qt::SizeVerCursor; }
Qt::CursorShape MainWindowWestWidget::CursorShape(){ return Qt::SizeHorCursor; }
Qt::CursorShape MainWindowEastWidget::CursorShape(){ return Qt::SizeHorCursor; }
Qt::CursorShape MainWindowNorthWestWidget::CursorShape(){ return Qt::SizeFDiagCursor; }
Qt::CursorShape MainWindowNorthEastWidget::CursorShape(){ return Qt::SizeBDiagCursor; }
Qt::CursorShape MainWindowSouthWestWidget::CursorShape(){ return Qt::SizeBDiagCursor; }
Qt::CursorShape MainWindowSouthEastWidget::CursorShape(){ return Qt::SizeFDiagCursor; }
