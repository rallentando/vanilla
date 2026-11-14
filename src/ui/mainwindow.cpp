#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "blurbehind.hpp"
#include "ownedraise.hpp"

#include "mainwindow.hpp"

#include <QIcon>
#include <QDockWidget>
#include "docktitlebar.hpp"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QWindow>
#include <QMenuBar>
#include <QPainter>
#include <QScreen>
#include <QLayout>
#include <QTimer>
#include <QElapsedTimer>
#include <QCursor>
#include <climits>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <dwmapi.h>
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
#include "dialog.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "systemmenu.hpp"

#if defined(Q_OS_WIN)
class InnerMarginWatch : public QObject {
public:
    explicit InnerMarginWatch(MainWindow *window) : QObject(window), m_Window(window){}
    bool eventFilter(QObject *watched, QEvent *ev) override {
        if(ev->type() == QEvent::Resize && watched == m_Window) m_Window->ApplyInnerMargin();
        return false;
    }
private:
    MainWindow *m_Window;
};
#endif

MainWindow::MainWindow(int id, QPoint pos, bool showNow, QWidget *parent)
    :
    QMainWindow(parent)
    , m_TreeBank(nullptr)
    , m_TreeBar(nullptr)
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
    , m_TitleStrip(nullptr)
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

    if(Application::EnableTransparentWindow()){
        setAttribute(Qt::WA_TranslucentBackground);
#if defined(Q_OS_WIN)
        installEventFilter(this);
#endif
    }

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
    m_InspectorDock->setTitleBarWidget(new DockTitleBar(m_InspectorDock));
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
    if(Application::EnableTransparentWindow()){
        menuBar()->setStyleSheet(QStringLiteral("QMenuBar{ background-color: transparent;}"));
        menuBar()->installEventFilter(this);
    }

    if(Application::EnableCustomTitleBar()){
#if defined(Q_OS_WIN)
        m_TitleBar        = new TitleBar(this, TitleBar::Inner);
        m_TitleStrip      = new TitleBar(this, TitleBar::Strip);
        installEventFilter(this);
        if(internalWinId()) ApplyFramelessFrame();
        qApp->installEventFilter(new InnerMarginWatch(this));
#else
        setWindowFlags(Qt::FramelessWindowHint);
        m_TitleBar        = new TitleBar(this, TitleBar::Outer);
        m_TitleStrip      = m_TitleBar;
        m_NorthWidget     = new MainWindowNorthWidget     (this);
        m_SouthWidget     = new MainWindowSouthWidget     (this);
        m_WestWidget      = new MainWindowWestWidget      (this);
        m_EastWidget      = new MainWindowEastWidget      (this);
        m_NorthWestWidget = new MainWindowNorthWestWidget (this);
        m_NorthEastWidget = new MainWindowNorthEastWidget (this);
        m_SouthWestWidget = new MainWindowSouthWestWidget (this);
        m_SouthEastWidget = new MainWindowSouthEastWidget (this);
#endif
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

    {
        Settings &s = Application::GlobalSettings();
        const QString once = QStringLiteral("mainwindow/titlebartoolbar%1").arg(m_Index);
        if(!Application::EnableCustomTitleBar() || !HasInnerBar()){
            s.remove(once);
        } else if(!s.value(once, false).toBool()){
            m_ToolBar->hide();
            s.setValue(once, true);
        }
    }

    m_InspectorDock->hide();
    for(QDockWidget *dock : m_SidePanels->Docks()) dock->hide();

    if(!showNow){
        createWinId();
        return;
    }

#if defined(Q_OS_MAC)
    QSize s = size();
    show();
    if(Application::ProductVersion().startsWith(QStringLiteral("10.14")))
        resize(s);
#else
    ShowWithFirstFrame();
#endif

}

MainWindow::~MainWindow(){
#if defined(Q_OS_WIN)
    DropFullScreenCover();
#endif
    if(m_TitleBarTimer) m_TitleBarTimer->stop();
    SetInspectorPane(nullptr);
    m_DialogFrame->deleteLater();
    if(m_TitleStrip) m_TitleStrip->deleteLater();
    for(QWidget *edge : EdgeWidgets()) edge->deleteLater();
}

int MainWindow::GetIndex(){
    return m_Index;
}

void MainWindow::SaveSettings(){
    Settings &s = Application::GlobalSettings();

    s.setValue(QStringLiteral("mainwindow/tableview%1").arg(m_Index), m_TreeBank->GetGadgets()->GetStat());
    if(TreeBank::CanHaveMiniMap())
        s.setValue(QStringLiteral("mainwindow/minimap%1").arg(m_Index), m_TreeBank->GetMiniMap() ? true : false);
    s.setValue(QStringLiteral("mainwindow/menubar%1").arg(m_Index), !IsMenuBarEmpty());
    s.setValue(QStringLiteral("mainwindow/toolbar%1").arg(m_Index), saveState());
    s.setValue(QStringLiteral("mainwindow/inspectordock%1").arg(m_Index), m_InspectorDockPlaced);
    s.setValue(QStringLiteral("mainwindow/sidepaneldock%1").arg(m_Index), m_SidePanels->Placed());
    s.setValue(QStringLiteral("mainwindow/treebar%1").arg(m_Index), m_TreeBar->GetStat());
    s.setValue(QStringLiteral("mainwindow/status%1").arg(m_Index), static_cast<int>(windowState()));
    s.setValue(QStringLiteral("mainwindow/stripexpanded%1").arg(m_Index), m_StripExpanded);
    if(!isFullScreen() && !isMaximized() && !isMinimized() && width() && height())
        s.setValue(QStringLiteral("mainwindow/geometry%1").arg(m_Index), geometry());
}

void MainWindow::LoadSettings(){
    Settings &s = Application::GlobalSettings();

    MainWindow *current = Application::GetCurrentWindow();
    QVariant tableview_data = s.value(QStringLiteral("mainwindow/tableview%1").arg(m_Index), QVariant());
    QVariant geometry_data  = s.value(QStringLiteral("mainwindow/geometry%1").arg(m_Index), QVariant());
    QVariant minimap_data   = s.value(QStringLiteral("mainwindow/minimap%1").arg(m_Index), QVariant());
    QVariant menubar_data   = s.value(QStringLiteral("mainwindow/menubar%1").arg(m_Index), QVariant());
    QVariant toolbar_data   = s.value(QStringLiteral("mainwindow/toolbar%1").arg(m_Index), QVariant());
    QVariant treebar_data   = s.value(QStringLiteral("mainwindow/treebar%1").arg(m_Index), QVariant());

    m_InspectorDockPlaced = s.value(QStringLiteral("mainwindow/inspectordock%1").arg(m_Index), false).toBool();
    m_SidePanels->SetPlaced(s.value(QStringLiteral("mainwindow/sidepaneldock%1").arg(m_Index), 0).toInt());
    m_StripExpanded = s.value(QStringLiteral("mainwindow/stripexpanded%1").arg(m_Index), false).toBool();

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

    if(TreeBank::CanHaveMiniMap()){
        const bool minimapEnabled = minimap_data.isNull()
            ? (current ? static_cast<bool>(current->GetTreeBank()->GetMiniMap()) : false)
            : minimap_data.toBool();
        if(minimapEnabled != static_cast<bool>(GetTreeBank()->GetMiniMap()))
            GetTreeBank()->ToggleMiniMap();
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
    s.remove(QStringLiteral("mainwindow/minimap%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/menubar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/toolbar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/treebar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/status%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/inspectordock%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/sidepaneldock%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/titlebartoolbar%1").arg(m_Index));
    s.remove(QStringLiteral("mainwindow/stripexpanded%1").arg(m_Index));
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
    if(ev->type() == QEvent::Paint && qobject_cast<QMenuBar*>(watched) &&
       testAttribute(Qt::WA_TranslucentBackground)){
        QWidget *bar = static_cast<QWidget*>(watched);
        QPainter painter(bar);
        painter.fillRect(bar->rect(), Theme::Brush(Application::EnableTransparentWindow()
                                                   ? Theme::BarBackgroundTranslucent
                                                   : Theme::BarBackground));
    }
#if defined(Q_OS_WIN)
    if(watched == this && ev->type() == QEvent::WinIdChange && internalWinId() && HasInnerBar())
        ApplyFramelessFrame();
    if(watched == this && ev->type() == QEvent::WinIdChange &&
       Application::EnableTransparentWindow() && internalWinId() && windowHandle()){
        if(BlurBehind::SurfaceMayBeTranslucent(windowHandle()->surfaceType())){
            BlurBehind::Blur(this);
            ApplyTranslucentFrame();
        } else {
            Application::TranslucentWindowUnsupported();
        }
    }
#endif
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
    m_TreeBank->ShowKeys(menu);
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
    if(TreeBank::CanHaveMiniMap())
        menu->addAction(m_TreeBank->Action(TreeBank::_ToggleMiniMap));
}

void MainWindow::CreateMenuBar(){
    menuBar()->addMenu(m_TreeBank->ApplicationMenu(true));
    menuBar()->addMenu(m_TreeBank->NodeMenu());
    menuBar()->addMenu(m_TreeBank->DisplayMenu());
    menuBar()->addMenu(m_TreeBank->WindowMenu());
    menuBar()->addMenu(m_TreeBank->PageMenu());
    menuBar()->show();
}

namespace {
    QWidget *PurgedPageWidget(const SharedView &view){
        if(!TreeBank::PurgeView() || !view) return nullptr;
        return qobject_cast<QWidget*>(view->base());
    }
}

bool MainWindow::IsShaded() const {
    return windowOpacity() == 0.0;
}

void MainWindow::Shade(){
    if(!m_TitleBar) return;
    UnpinStrip();
    if(TitleBar *strip = SeparateStrip()) strip->EndEdit();
    setWindowOpacity(0.0);
    {
        SharedView view = m_TreeBank->GetCurrentView();
        if(PurgedPageWidget(view)) view->hide();
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
    {
        SharedView view = m_TreeBank->GetCurrentView();
        if(PurgedPageWidget(view)) view->show();
    }
    AdjustAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->ResizeNotify(m_TreeBank->size());
    SetFocus();
}

void MainWindow::ShowWindowFrameWidgets(){
    AdjustAllEdgeWidgets();
}

void MainWindow::HideWindowFrameWidgets(){
    if(!m_TitleBar) return;
    SetStripPinned(false);
    m_TitleBarTimer->stop();
    m_TitleBarReveal.Reset();
    m_TitleStrip->Reveal(false, false);
    for(QWidget *edge : EdgeWidgets()) edge->hide();
}

void MainWindow::CurrentViewChanged(){
    if(m_TitleBar) m_TitleBar->EndEdit();
    if(TitleBar *strip = SeparateStrip()) strip->EndEdit();
}

bool MainWindow::IsStripExpanded() const {
    return m_StripExpanded && IsSimplyMaximized();
}

void MainWindow::SetStripExpanded(bool on){
    if(on && !IsSimplyMaximized()) return;
    if(on == m_StripExpanded) return;
    m_StripExpanded = on;
    SetStripPinned(on);
    if(!on && m_TitleStrip) m_TitleStrip->EndEdit();
    UpdateTitleBarVisibility();
    if(m_TitleStrip) m_TitleStrip->update();
}

namespace {
class StripPressWatch : public QObject {
public:
    StripPressWatch(MainWindow *window, QWidget *strip)
        : QObject(window), m_Window(window), m_Strip(strip){}
    bool eventFilter(QObject *watched, QEvent *ev) override {
        if(ev->type() == QEvent::MouseButtonPress && watched->isWidgetType()){
            QWidget *top = static_cast<QWidget*>(watched)->window();
            Receiver *receiver = m_Window->GetTreeBank() ? m_Window->GetTreeBank()->GetReceiver() : nullptr;
            const bool line = receiver && top == receiver && receiver->IsAnchored();
            if(top != m_Strip && top->windowType() != Qt::Popup && !line)
                m_Window->UnpinStrip();
        }
        return false;
    }
private:
    MainWindow *m_Window;
    QWidget *m_Strip;
};
}

void MainWindow::SetStripPinned(bool on){
    m_StripPinned = on;
    if(on && !m_StripPressWatch){
        m_StripPressWatch = new StripPressWatch(this, m_TitleStrip);
        qApp->installEventFilter(m_StripPressWatch);
    } else if(!on && m_StripPressWatch){
        qApp->removeEventFilter(m_StripPressWatch);
        m_StripPressWatch->deleteLater();
        m_StripPressWatch = nullptr;
    }
}

void MainWindow::PinStrip(){
    if(m_StripPinned || !IsStripExpanded()) return;
    SetStripPinned(true);
    UpdateTitleBarVisibility();
}

void MainWindow::UnpinStrip(){
    if(!m_StripPinned) return;
    SetStripPinned(false);
    UpdateTitleBarVisibility();
}

void MainWindow::HoldStripReveal(){
    m_TitleBarReveal.Hold();
}

bool MainWindow::WantsInnerBar() const {
    WindowFrameMargin::State state;
    state.qtMaximized = isMaximized();
    state.fullScreen = isFullScreen() || m_ContentFullScreen;
    state.current = contentsMargins().top() > 0;
#if defined(Q_OS_WIN)
    const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
    state.nativeKnown = hwnd && ::IsWindowVisible(hwnd);
    if(state.nativeKnown){
        state.nativeZoomed = ::IsZoomed(hwnd);
        state.nativeIconic = ::IsIconic(hwnd);
    }
#endif
    return WindowFrameMargin::Inner(state);
}

bool MainWindow::HasInnerBar() const {
    return m_TitleBar && m_TitleBar->GetMode() == TitleBar::Inner;
}

bool MainWindow::ApplyInnerMargin(){
    if(!HasInnerBar()) return false;
    const int t = ScaleByDevice(TITLE_BAR_HEIGHT);
    const bool inner = WantsInnerBar();
    const QMargins margins(0, inner ? t : 0, 0, 0);
    if(contentsMargins() == margins) return false;
    setContentsMargins(margins);
    return true;
}

QList<QWidget*> MainWindow::EdgeWidgets() const {
    if(!m_NorthWidget) return QList<QWidget*>();
    return QList<QWidget*>{m_NorthWidget, m_SouthWidget, m_WestWidget, m_EastWidget,
                           m_NorthWestWidget, m_NorthEastWidget,
                           m_SouthWestWidget, m_SouthEastWidget};
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
    if(!m_TitleStrip || !m_TitleStrip->isVisible()) return;
    const QList<QWidget*> edges = EdgeWidgets();
    for(int i = 0; i < edges.size() && i < 4; i++) edges[i]->raise();
    m_TitleStrip->raise();
    for(int i = 4; i < edges.size(); i++) edges[i]->raise();
}

void MainWindow::AdjustAllEdgeWidgets(){
    QRect rect = geometry();
    const int dialogWidth = rect.width() / MODELESS_DIALOG_WIDTH_DIVISOR;
    m_DialogFrame->setGeometry(rect.left() + rect.width() - dialogWidth, rect.top(),
                               dialogWidth, m_DialogFrame->height());
    m_DialogFrame->Adjust();
    if(!m_TitleBar) return;
    const bool frameVisible = IsFrameShown();
    const int t = ScaleByDevice(TITLE_BAR_HEIGHT);
    if(HasInnerBar()){
        if(ApplyInnerMargin()){
            QTimer::singleShot(0, this, [this](){
                QLayout *l = layout();
                if(!l) return;
                int top = INT_MAX;
                for(QWidget *w : {menuWidget(), static_cast<QWidget*>(m_TreeBar),
                                  static_cast<QWidget*>(m_ToolBar), centralWidget()})
                    if(w && w->isVisible() && !w->isWindow()) top = qMin(top, w->geometry().top());
                if(top == INT_MAX || top == contentsMargins().top()) return;
                l->invalidate();
                l->activate();
            });
        }
        const bool inner = WantsInnerBar();
        m_TitleBar->setGeometry(0, 0, width(), t);
        m_TitleBar->setVisible(inner);
        if(inner) m_TitleBar->raise();
    }
    if(m_NorthWidget){
        for(QWidget *edge : EdgeWidgets())
            edge->setVisible(frameVisible && !isMaximized());
        if(IsShaded()) rect.setHeight(0);
        const int e = ScaleByDevice(EDGE_WIDGET_SIZE);
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
    }
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
                                 static_cast<const QWidget*>(m_TitleStrip),
                                 static_cast<const QWidget*>(purged)}){
        if(widget && widget->internalWinId() &&
           root == reinterpret_cast<HWND>(widget->window()->internalWinId()))
            return true;
    }
    return false;
#else
    return isActiveWindow() || (m_TitleStrip && m_TitleStrip->isActiveWindow()) ||
           (purged && purged->isActiveWindow());
#endif
}

void MainWindow::UpdateTitleBarVisibility(){
    if(!m_TitleBar) return;
    ApplyTitleBarVisibility(QCursor::pos(), IsFrameActive(),
                           QApplication::mouseButtons() != Qt::NoButton);
}

void MainWindow::ApplyTitleBarVisibility(const QPoint &cursor, bool active, bool buttonsDown){
    if(!m_TitleStrip) return;
    const QRect rect = geometry();
    const bool outer = m_TitleStrip->GetMode() == TitleBar::Outer;
    const int t = m_TitleStrip->ScaleByDevice(TITLE_BAR_HEIGHT);
    const int titleWidth = isMaximized() && !IsStripExpanded()
        ? qMin(rect.width(), m_TitleStrip->CompactWidth()) : rect.width();
    const int titleTop = isMaximized() || !outer ? rect.top() : rect.top()-t;
    m_TitleStrip->SetStripGeometry(QRect(rect.right()+1-titleWidth, titleTop, titleWidth, t));
    const bool frameVisible = IsFrameShown();
    const bool enabled = frameVisible && isMaximized();
    const QRect monitor = screen() ? screen()->geometry() : QRect();
    const QRect strip = QRect(QPoint(geometry().left(), monitor.top()),
                               QPoint(geometry().right(), titleTop + t - 1))
                            .intersected(monitor);
    const bool revealed = m_TitleBarReveal.Update(enabled, active, strip, cursor, buttonsDown);
    const bool visible = frameVisible &&
        (IsShaded() || (isMaximized() ? revealed || (IsStripExpanded() && m_StripPinned && active) : outer));
    const bool animate = frameVisible && isMaximized() && !IsShaded() && !m_StripWasShaded;
    m_StripWasShaded = IsShaded();
    if(m_TitleStrip->Reveal(visible, animate)){
        RaiseAllEdgeWidgets();
        m_TitleStrip->update();
    }
}

void MainWindow::UpdateAllEdgeWidgets(){
    if(!m_TitleBar) return;
    m_TitleBar->update();
    if(TitleBar *strip = SeparateStrip()) strip->update();
    for(QWidget *edge : EdgeWidgets()) edge->update();
}

void MainWindow::SetWindowTitle(const QString &title){
    if(title.isEmpty())
        setWindowTitle(QStringLiteral("vanilla"));
    else if(m_TitleBar)
        setWindowTitle(title);
    else
        setWindowTitle(QStringLiteral("vanilla - ") + title);

    if(m_TitleBar) m_TitleBar->update();
    if(TitleBar *strip = SeparateStrip()) strip->update();
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

#if defined(Q_OS_WIN)

static HWND CoverWithLastLook(HWND hwnd){
    if(!::IsWindow(hwnd) || !::IsWindowVisible(hwnd) || ::IsIconic(hwnd)) return nullptr;
    RECT r;
    if(FAILED(::DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r)))) return nullptr;
    const int w = r.right - r.left, h = r.bottom - r.top;
    if(w <= 0 || h <= 0) return nullptr;

    static const wchar_t *className = L"VanillaFullScreenCover";
    static const bool registered = [](){
        WNDCLASSW wc = {};
        wc.lpfnWndProc = ::DefWindowProcW;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.lpszClassName = className;
        return ::RegisterClassW(&wc) != 0;
    }();
    if(!registered) return nullptr;
    HWND cover = ::CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE |
                                   WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                                   className, L"", WS_POPUP, r.left, r.top, w, h,
                                   nullptr, nullptr, ::GetModuleHandleW(nullptr), nullptr);
    if(!cover) return nullptr;

    bool shown = false;
    HDC screen = ::GetDC(nullptr);
    HDC mem = ::CreateCompatibleDC(screen);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void *bits = nullptr;
    HBITMAP bitmap = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if(mem && bitmap){
        HGDIOBJ old = ::SelectObject(mem, bitmap);
        if(::BitBlt(mem, 0, 0, w, h, screen, r.left, r.top, SRCCOPY)){
            ::GdiFlush();
            quint32 *pixel = static_cast<quint32*>(bits);
            for(qint64 i = 0, n = qint64(w) * h; i < n; ++i) pixel[i] |= 0xff000000u;
            POINT to = { r.left, r.top };
            SIZE size = { w, h };
            POINT from = { 0, 0 };
            BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            shown = ::UpdateLayeredWindow(cover, screen, &to, &size, mem, &from, 0, &blend, ULW_ALPHA)
                && ::SetWindowPos(cover, HWND_TOPMOST, 0, 0, 0, 0,
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        ::SelectObject(mem, old);
    }
    if(bitmap) ::DeleteObject(bitmap);
    if(mem) ::DeleteDC(mem);
    ::ReleaseDC(nullptr, screen);
    if(!shown){
        ::DestroyWindow(cover);
        return nullptr;
    }
    return cover;
}

void MainWindow::DropFullScreenCover(){
    if(!m_FullScreenCover) return;
    ::DestroyWindow(reinterpret_cast<HWND>(m_FullScreenCover));
    m_FullScreenCover = 0;
}
#endif

void MainWindow::CloakWhileSwitchingFullScreen(){
#if defined(Q_OS_WIN)
    const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
    if(!hwnd) return;
    if(!m_FullScreenCloaked){
        m_FullScreenCover = reinterpret_cast<quintptr>(CoverWithLastLook(hwnd));
        m_FullScreenCloaked = true;
    }
    BOOL on = TRUE;
    ::DwmSetWindowAttribute(hwnd, DWMWA_CLOAK, &on, sizeof(on));
    const int generation = ++m_FullScreenSwitch;
    QTimer::singleShot(34, this, [this, generation](){
        if(generation != m_FullScreenSwitch) return;
        if(const HWND hwnd = reinterpret_cast<HWND>(internalWinId())){
            BOOL off = FALSE;
            ::DwmSetWindowAttribute(hwnd, DWMWA_CLOAK, &off, sizeof(off));
        }
        m_FullScreenCloaked = false;
        if(m_FullScreenCover){
            ::DwmFlush();
            DropFullScreenCover();
        }
    });
#endif
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
    CloakWhileSwitchingFullScreen();
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

void MainWindow::SetFullScreen(bool on){
    if(on == m_ContentFullScreen) return;

    if(on){
        m_StateBeforeFullScreen = windowState();
        m_MenuBarVisibleBeforeFullScreen = !menuBar()->isHidden();
        m_TreeBarVisibleBeforeFullScreen = !m_TreeBar->isHidden();
        m_ToolBarVisibleBeforeFullScreen = !m_ToolBar->isHidden();
        m_ContentFullScreen = true;

        const bool switching = !isFullScreen();
        if(switching) CloakWhileSwitchingFullScreen();

        menuBar()->hide();
        m_TreeBar->hide();
        m_ToolBar->hide();
        HideWindowFrameWidgets();
        m_TreeBank->SetMiniMapShelved(true);

        if(switching) showFullScreen();

    } else {
        m_ContentFullScreen = false;

        const bool switching = !(m_StateBeforeFullScreen & Qt::WindowFullScreen);
        if(switching) CloakWhileSwitchingFullScreen();

        if(m_MenuBarVisibleBeforeFullScreen) menuBar()->show();
        if(m_TreeBarVisibleBeforeFullScreen) m_TreeBar->show();
        if(m_ToolBarVisibleBeforeFullScreen) m_ToolBar->show();
        ShowWindowFrameWidgets();
        m_TreeBank->SetMiniMapShelved(false);

        if(switching){
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

void MainWindow::raise(){
    OwnedRaise::OwnerRaise owner(internalWinId());
    QMainWindow::raise();
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

void MainWindow::SetOwner(QWidget *window, MainWindow *owner){
    if(!window || !window->isWindow()) return;
    window->winId();
    QWindow *handle = window->windowHandle();
    if(!handle) return;
    QWindow *to = nullptr;
    if(owner){
        owner->winId();
        to = owner->windowHandle();
    }
    if(handle->transientParent() != to) handle->setTransientParent(to);
}

MainWindow *MainWindow::DialogOwner(){
    MainWindow *win = Application::GetCurrentWindow();
    if(!win || !win->isVisible() || win->isMinimized()) return nullptr;
    return win;
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

void MainWindow::ShowWithFirstFrame(){
    if(isVisible()) return;
#if defined(Q_OS_WIN)
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL cloak = TRUE;
    const bool cloaked = SUCCEEDED(::DwmSetWindowAttribute(hwnd, 13, &cloak, sizeof(cloak)));
    show();
    if(cloaked && Application::IsRestoringWindows()){
        QElapsedTimer waiting;
        waiting.start();
        while(!(windowHandle() && windowHandle()->isExposed()) && waiting.elapsed() < 150)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 20);
    }
    if(cloaked){
        cloak = FALSE;
        ::DwmSetWindowAttribute(hwnd, 13, &cloak, sizeof(cloak));
    }
#else
    show();
#endif
}

void MainWindow::changeEvent(QEvent *ev){
    QMainWindow::changeEvent(ev);
    if(ev->type() == QEvent::WindowStateChange){
        if(IsShaded() && (isMaximized() || isFullScreen())) Unshade();
        if(!IsSimplyMaximized()){
            SetStripPinned(false);
            if(TitleBar *strip = SeparateStrip()) strip->EndEdit();
        }
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

void MainWindow::RepaintWindowEdge(){
#if defined(Q_OS_WIN)
    if(!internalWinId()) return;
    ApplyEdgeCaption();
    ::RedrawWindow(reinterpret_cast<HWND>(internalWinId()), nullptr, nullptr,
                   RDW_FRAME | RDW_INVALIDATE);
#endif
}

#if defined(Q_OS_WIN)
void MainWindow::ApplyEdgeCaption(){
    if(!HasInnerBar() || !internalWinId()) return;
    const QColor color(Application::WindowEdgeColor());
    const DWORD caption = color.isValid()
        ? static_cast<DWORD>(color.red() | (color.green() << 8) | (color.blue() << 16))
        : 0xFFFFFFFF;
    ::DwmSetWindowAttribute(reinterpret_cast<HWND>(internalWinId()),
                            35 , &caption, sizeof(caption));
}
#endif

void MainWindow::ApplyTheme(){
    ApplyTranslucentFrame();
    if(testAttribute(Qt::WA_TranslucentBackground)) menuBar()->update();
}

void MainWindow::ApplyTranslucentFrame(){
#if defined(Q_OS_WIN)
    if(!Application::EnableTransparentWindow() || Application::EnableCustomTitleBar() || !internalWinId()) return;
    const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
    const auto colorref = [](const QColor &c){ return static_cast<DWORD>(c.red() | (c.green() << 8) | (c.blue() << 16)); };
    DWORD caption = colorref(Theme::Color(Theme::BarBackground));
    DWORD text    = colorref(Theme::Color(Theme::BarTitleText));
    ::DwmSetWindowAttribute(hwnd, 35 , &caption, sizeof(caption));
    ::DwmSetWindowAttribute(hwnd, 36 , &text, sizeof(text));
#endif
}

#if defined(Q_OS_WIN)

static bool PaintFullScreenBorder(HWND hwnd, const MSG *msg, qintptr *result){
    if(!hwnd || msg->hwnd != hwnd ||
       (msg->message != WM_NCPAINT && msg->message != WM_NCACTIVATE)) return false;
    const LONG_PTR style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
    if((style & WS_CAPTION) != WS_BORDER || (style & WS_THICKFRAME)) return false;
    const QColor color(Application::WindowEdgeColor());
    if(!color.isValid()) return false;
    if(HDC dc = ::GetWindowDC(hwnd)){
        RECT window, client;
        ::GetWindowRect(hwnd, &window);
        ::GetClientRect(hwnd, &client);
        POINT origin = { 0, 0 };
        ::ClientToScreen(hwnd, &origin);
        ::OffsetRect(&client, origin.x - window.left, origin.y - window.top);
        ::OffsetRect(&window, -window.left, -window.top);
        ::ExcludeClipRect(dc, client.left, client.top, client.right, client.bottom);
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = 1;
        info.bmiHeader.biHeight = 1;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        const DWORD pixel = 0xFF000000 | static_cast<DWORD>(color.rgb() & 0xFFFFFF);
        ::StretchDIBits(dc, 0, 0, window.right, window.bottom, 0, 0, 1, 1,
                        &pixel, &info, DIB_RGB_COLORS, SRCCOPY);
        ::ReleaseDC(hwnd, dc);
    }
    *result = msg->message == WM_NCACTIVATE ? TRUE : 0;
    return true;
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result){
    if(eventType == "windows_generic_MSG" && message && HasInnerBar() &&
       FramelessNativeEvent(message, result))
        return true;
    if(eventType == "windows_generic_MSG" && message){
        const MSG *msg = static_cast<const MSG*>(message);
        const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
        if(PaintFullScreenBorder(hwnd, msg, result)) return true;
        if(m_StripPinned && hwnd && msg->hwnd == hwnd){
            UINT button = msg->message;
            if(button == WM_PARENTNOTIFY) button = LOWORD(msg->wParam);
            if(button == WM_LBUTTONDOWN || button == WM_RBUTTONDOWN ||
               button == WM_MBUTTONDOWN || button == WM_XBUTTONDOWN)
                UnpinStrip();
        }
        if(msg->message == WM_INITMENUPOPUP && HIWORD(msg->lParam) && hwnd && msg->hwnd == hwnd &&
           reinterpret_cast<HMENU>(msg->wParam) == ::GetSystemMenu(hwnd, FALSE)){
            delete m_SystemMenu;
            m_SystemMenuActions.clear();
            QList<QAction*> actions;
            m_SystemMenu = m_TreeBank->GlobalContextMenu();
            const QList<SystemMenu::Entry> entries = SystemMenu::Entries(m_SystemMenu, actions);
            for(QAction *action : std::as_const(actions)) m_SystemMenuActions.append(QPointer<QAction>(action));
            SystemMenu::Install(internalWinId(), entries);
            return false;
        }
        if(msg->message == WM_ACTIVATE && hwnd && msg->hwnd == hwnd &&
           LOWORD(msg->wParam) != WA_INACTIVE){
            QTimer::singleShot(0, this, [this](){ RaiseAllEdgeWidgets();});
        }
        if(msg->message == WM_SYSCOMMAND){
            const int index = SystemMenu::IndexOf(msg->wParam);
            if(index >= 0){
                if(index < m_SystemMenuActions.size()){
                    if(QAction *action = m_SystemMenuActions[index])
                        QMetaObject::invokeMethod(action, &QAction::trigger, Qt::QueuedConnection);
                }
                *result = 0;
                return true;
            }
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::closeEvent(QCloseEvent *ev){
    if(Application::IsRestoringWindows()){
        ev->ignore();
        return;
    }
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
        foreach(SharedView view, TreeBank::AllViews()){
            if(!view || view->GetTreeBank() != m_TreeBank) continue;
            QWidget *widget = qobject_cast<QWidget*>(view->base());
            if(!widget || !widget->isWindow()) continue;
            MainWindow *to = Application::GetCurrentWindow();
            view->setParent(to->GetTreeBank());
            view->SetTreeBank(to->GetTreeBank());
            view->hide();
            SetOwner(widget, to);
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
    QWidget *widget = PurgedPageWidget(m_TreeBank->GetCurrentView());
    if(widget && widget->isWindow())
        widget->setGeometry(QRect(m_TreeBank->mapToGlobal(QPoint()), m_TreeBank->size()));
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
    if(Application::EnableTransparentWindow()){
        BlurBehind::Blur(this);
        ApplyTranslucentFrame();
    }
    {
        SharedView view = m_TreeBank->GetCurrentView();
        if(PurgedPageWidget(view)) view->show();
    }
    if(m_DialogFrame->windowHandle() == nullptr ||
       m_DialogFrame->windowHandle()->transientParent() != windowHandle()){
        m_DialogFrame->hide();
        SetOwner(m_DialogFrame, this);
    }
    ShowAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->ResizeNotify(m_TreeBank->size());
}

void MainWindow::hideEvent(QHideEvent *ev){
    {
        SharedView view = m_TreeBank->GetCurrentView();
        if(PurgedPageWidget(view) && view->GetTreeBank() == m_TreeBank) view->show();
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

#if defined(Q_OS_WIN)
void MainWindow::ApplyFramelessFrame(){
    const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
    if(!hwnd) return;
    const QRect wanted = geometry();
    ApplyEdgeCaption();
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                   SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    if(!isMaximized() && !isFullScreen() && !isMinimized() && wanted.isValid() && windowHandle())
        windowHandle()->setGeometry(wanted);
}

WindowFrameHit::Layout MainWindow::FrameHitLayout() const {
    WindowFrameHit::Layout layout;
    const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
    RECT client;
    ::GetClientRect(hwnd, &client);
    const UINT dpi = ::GetDpiForWindow(hwnd);
    layout.width = client.right;
    layout.border = ::GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) +
                    ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    layout.resizable = !isMaximized();
    if(m_TitleBar->isHidden()) return layout;
    const qreal ratio = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
    const QPoint origin = m_TitleBar->pos();
    auto physical = [&](const QRect &r){
        if(r.isEmpty()) return QRect();
        const QRect moved = r.translated(origin);
        return QRect(QPoint(qRound(moved.left()*ratio), qRound(moved.top()*ratio)),
                     QPoint(qRound((moved.right()+1)*ratio)-1, qRound((moved.bottom()+1)*ratio)-1));
    };
    layout.bar = physical(QRect(QPoint(), m_TitleBar->size()));
    layout.maximize = physical(m_TitleBar->ButtonRect(TitleBar::MaximizeButton));
    for(TitleBar::Button each : TitleBar::AllButtons){
        if(each == TitleBar::MaximizeButton) continue;
        const QRect r = physical(m_TitleBar->ButtonRect(each));
        if(!r.isEmpty()) layout.buttons << r;
    }
    const QRect title = physical(m_TitleBar->IsEditing() ? m_TitleBar->TitleRect()
                                                         : m_TitleBar->EditRect());
    if(!title.isEmpty()) layout.buttons << title;
    return layout;
}

void MainWindow::ReleaseMaximizeButton(){
    m_TitleBar->SetNativePressed(false);
}

bool MainWindow::FramelessNativeEvent(void *message, qintptr *result){
    const MSG *msg = static_cast<const MSG*>(message);
    const HWND hwnd = reinterpret_cast<HWND>(internalWinId());
    if(!hwnd || msg->hwnd != hwnd) return false;
    switch(msg->message){
    case WM_NCCALCSIZE: {
        if(!msg->wParam || !(::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_THICKFRAME)) return false;
        const UINT dpi = ::GetDpiForWindow(hwnd);
        const int padded = ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        const int frameX = ::GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + padded;
        const int frameY = ::GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + padded;
        RECT &client = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam)->rgrc[0];
        client.left   += frameX;
        client.right  -= frameX;
        client.bottom -= frameY;
        if(::IsZoomed(hwnd)){
            client.top += frameY;
            MONITORINFO monitor = { sizeof(monitor) };
            if(::GetMonitorInfoW(::MonitorFromRect(&client, MONITOR_DEFAULTTONEAREST), &monitor) &&
               client.left <= monitor.rcMonitor.left && client.top <= monitor.rcMonitor.top &&
               client.right >= monitor.rcMonitor.right && client.bottom >= monitor.rcMonitor.bottom)
                client.top = monitor.rcMonitor.top + 1;
        }
        *result = 0;
        return true;
    }
    case WM_NCHITTEST: {
        if(Application::IsRestoringWindows()){
            *result = HTCLIENT;
            return true;
        }
        if(isFullScreen() || m_ContentFullScreen) return false;
        const LRESULT frame = ::DefWindowProcW(hwnd, msg->message, msg->wParam, msg->lParam);
        if(frame != HTCLIENT){
            *result = frame;
            return true;
        }
        POINT point = { static_cast<short>(LOWORD(msg->lParam)), static_cast<short>(HIWORD(msg->lParam)) };
        ::ScreenToClient(hwnd, &point);
        switch(WindowFrameHit::At(FrameHitLayout(), QPoint(point.x, point.y))){
        case WindowFrameHit::Client:         *result = HTCLIENT;    break;
        case WindowFrameHit::Caption:        *result = HTCAPTION;   break;
        case WindowFrameHit::MaximizeButton: *result = HTMAXBUTTON; break;
        case WindowFrameHit::Top:            *result = HTTOP;       break;
        case WindowFrameHit::TopLeft:        *result = HTTOPLEFT;   break;
        case WindowFrameHit::TopRight:       *result = HTTOPRIGHT;  break;
        }
        return true;
    }
    case WM_NCMOUSEMOVE: {
        const bool over = msg->wParam == HTMAXBUTTON;
        if(over != m_TitleBar->IsNativeHovered()){
            m_TitleBar->SetNativeHover(over);
            if(over){
                TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE | TME_NONCLIENT, hwnd, 0 };
                ::TrackMouseEvent(&track);
            }
        }
        if(!over) ReleaseMaximizeButton();
        return false;
    }
    case WM_NCMOUSELEAVE:
        m_TitleBar->SetNativeHover(false);
        ReleaseMaximizeButton();
        return false;
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        if(msg->wParam != HTMAXBUTTON) return false;
        if(Application::IsRestoringWindows()) return false;
        m_TitleBar->SetNativePressed(true);
        *result = 0;
        return true;
    case WM_NCLBUTTONUP: {
        const bool pressed = m_TitleBar->IsNativePressed();
        ReleaseMaximizeButton();
        if(msg->wParam != HTMAXBUTTON) return false;
        if(pressed && !Application::IsRestoringWindows())
            QTimer::singleShot(0, this, &MainWindow::ToggleMaximized);
        *result = 0;
        return true;
    }
    case WM_NCRBUTTONUP:
        if(msg->wParam != HTCAPTION || Application::IsRestoringWindows()) return false;
        QTimer::singleShot(0, this, &MainWindow::ToggleShaded);
        *result = 0;
        return true;
    case WM_CAPTURECHANGED:
        ReleaseMaximizeButton();
        return false;
    case WM_ACTIVATE:
        if(LOWORD(msg->wParam) == WA_INACTIVE) ReleaseMaximizeButton();
        return false;
    }
    return false;
}
#endif

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

void MainWindowEdgeWidget::PaintSide(QPaintEvent *ev, Qt::Edge side){
    QPainter painter(this);
    auto ghost = [&](){
        painter.setPen(GhostPen());
        painter.setBrush(GhostBrush());
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    };
    const bool horizontal = side == Qt::TopEdge || side == Qt::BottomEdge;
    if(disp == none){
        ghost();
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                const int along  = horizontal ? width()  : height();
                const int across = horizontal ? height() : width();
                double i, j, w, c;
                const double f = ScaleByDevice(25);
                for(i=0; i < along; i++){
                    for(j=0; j < across; j++){
                        w = along;
                        c = 77*j*j/e/e;
                        if(  i<f) c = c*sqrt(   i /f);
                        if(w-i<f) c = c*sqrt((w-i)/f);
                        const int a = static_cast<int>(i);
                        const int b = static_cast<int>(j);
                        switch(side){
                        case Qt::TopEdge:    image.setPixel(a, b,       EDGE_SHADOW_COLOR); break;
                        case Qt::BottomEdge: image.setPixel(a, e-b-1,   EDGE_SHADOW_COLOR); break;
                        case Qt::LeftEdge:   image.setPixel(b, a,       EDGE_SHADOW_COLOR); break;
                        case Qt::RightEdge:  image.setPixel(e-b-1, a,   EDGE_SHADOW_COLOR); break;
                        }
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                ghost();
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
            switch(side){
            case Qt::TopEdge:    painter.drawLine(QPoint(0,e-1), QPoint(width(),e-1)); break;
            case Qt::BottomEdge: painter.drawLine(QPoint(0,0),   QPoint(width(),0));   break;
            case Qt::LeftEdge:   painter.drawLine(QPoint(e-1,0), QPoint(e-1,height())); break;
            case Qt::RightEdge:  painter.drawLine(QPoint(0,0),   QPoint(0,height()));   break;
            }
        }
    }
    ev->setAccepted(true);
}

void MainWindowNorthWidget::paintEvent(QPaintEvent *ev){
    PaintSide(ev, Qt::TopEdge);
}

void MainWindowSouthWidget::paintEvent(QPaintEvent *ev){
    PaintSide(ev, Qt::BottomEdge);
}

void MainWindowWestWidget::paintEvent(QPaintEvent *ev){
    PaintSide(ev, Qt::LeftEdge);
}

void MainWindowEastWidget::paintEvent(QPaintEvent *ev){
    PaintSide(ev, Qt::RightEdge);
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
