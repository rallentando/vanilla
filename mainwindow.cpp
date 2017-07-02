#include "switch.hpp"
#include "const.hpp"

#include "mainwindow.hpp"

#include <QIcon>
#include <QStyle>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QDesktopWidget>
#include <QWindow>
#include <QMenuBar>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "view.hpp"
#include "application.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "gadgets.hpp"
#include "graphicstableview.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "quicknativewebview.hpp"
#include "tridentview.hpp"
#include "dialog.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#  include <QtWin>
#endif

MainWindow::MainWindow(int id, QPoint pos, QWidget *parent)
    : QMainWindow(parent)
{
    m_Index = id;

    m_TreeBank = new TreeBank(this);
    setCentralWidget(m_TreeBank);

    m_TreeBar = new TreeBar(m_TreeBank, this);
    addToolBar(m_TreeBar);

    addToolBarBreak();

    m_ToolBar = new ToolBar(m_TreeBank, this);
    addToolBar(m_ToolBar);

    m_DialogFrame = Application::GetTemporaryDialogFrame();
    if(m_DialogFrame){
        Application::SetTemporaryDialogFrame(0);
    } else {
        m_DialogFrame = new ModelessDialogFrame();
    }
    m_DialogFrame->show();

    // make MenuBar and hide.
    menuBar()->hide();
    if(Application::EnableTransparentBar()){
        menuBar()->setStyleSheet
            ("QMenuBar{ background-color: transparent;}"
             "QMenuBar::item{ background-color:rgba(255, 255, 255, 185);}"
             "QMenuBar::item:selected{ background-color:rgba(255, 255, 255, 225);}");
    }

    move(pos);
    LoadSettings();

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
        AdjustAllEdgeWidgets();
        connect(Application::GetInstance(), &Application::focusChanged,
                this, &MainWindow::UpdateAllEdgeWidgets);

        show();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        QtWin::setCompositionEnabled(true);
        QtWin::enableBlurBehindWindow(windowHandle());
#endif
    } else {
        m_TitleBar        = 0;
        m_NorthWidget     = 0;
        m_SouthWidget     = 0;
        m_WestWidget      = 0;
        m_EastWidget      = 0;
        m_NorthWestWidget = 0;
        m_NorthEastWidget = 0;
        m_SouthWestWidget = 0;
        m_SouthEastWidget = 0;

        show();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
        setAttribute(Qt::WA_TranslucentBackground);
        QtWin::extendFrameIntoClientArea(this, QMargins(-1, -1, -1, -1));
#endif
    }
}

MainWindow::~MainWindow(){
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

    if(tableview_data.isNull()){

        QStringList list = current
            ? current->GetTreeBank()->GetGadgets()->GetStat()
            : QStringList()
            << QStringLiteral("%1").arg(static_cast<int>(GraphicsTableView::Flat))
            << QStringLiteral("%1").arg(static_cast<int>(GraphicsTableView::Recursive))
            << QStringLiteral("%1").arg(1.0f);

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
            QDesktopWidget desktop;
            if(desktop.screenCount()){
                rect = desktop.screenGeometry(desktop.primaryScreen());
                rect = QRect(rect.x() + rect.width()  / 6,
                             rect.y() + rect.height() / 6,
                             rect.width() * 2 / 3, rect.height() * 2 / 3);
            }
        } else {

            if(pos().isNull())
                rect.translate(20, 20);
            else
                rect.moveTopLeft(pos());
        }

        setGeometry(rect);

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

    QTimer::singleShot(0, this, [this, s](){

    // 'setGeometry' and 'restoreGeometry' are asynchronous API.
    bool contains = false;
    QDesktopWidget desktop;
    if(desktop.screenCount()){
        for(int i = 0; i < desktop.screenCount(); i++){
            if(desktop.screenGeometry(i).intersects(geometry())){
                contains = true;
                break;
            }
        }
        if(!contains){
            QRect rect = desktop.screenGeometry(desktop.primaryScreen());
            setGeometry(rect.x() + rect.width()  / 6,
                        rect.y() + rect.height() / 6,
                        rect.width() * 2 / 3, rect.height() * 2 / 3);
        }
    }

    QVariant status = s.value(QStringLiteral("mainwindow/status%1").arg(m_Index), QVariant());

    if(status.isNull()){
        /* do nothing. */
    } else if(status.canConvert<int>()){
        setWindowState(static_cast<Qt::WindowStates>(status.toInt()));
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
    //s.sync();
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

bool MainWindow::IsMenuBarEmpty() const {
    return menuBar()->actions().isEmpty();
}

void MainWindow::ClearMenuBar(){
    menuBar()->clear();
    menuBar()->hide();
}

void MainWindow::CreateMenuBar(){
    menuBar()->addMenu(m_TreeBank->ApplicationMenu(true));
    menuBar()->addMenu(m_TreeBank->NodeMenu());
    menuBar()->addMenu(m_TreeBank->DisplayMenu());
    menuBar()->addMenu(m_TreeBank->WindowMenu());
    menuBar()->addMenu(m_TreeBank->PageMenu());
    menuBar()->show();
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
#ifdef TRIDENTVIEW
            else if(TridentView *w = qobject_cast<TridentView*>(view->base()))
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
#ifdef TRIDENTVIEW
            else if(TridentView *w = qobject_cast<TridentView*>(view->base()))
                w->show();
#endif
            else;
        }
    }
    AdjustAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->show();
    //Receiver *receiver = m_TreeBank->GetReceiver();
    //if(receiver && receiver->IsPurged()) receiver->show();
    SetFocus();
}

void MainWindow::ShowAllEdgeWidgets(){
    m_DialogFrame->show();
    if(!m_TitleBar) return;
    m_TitleBar        ->show();
    m_NorthWidget     ->show();
    m_SouthWidget     ->show();
    m_WestWidget      ->show();
    m_EastWidget      ->show();
    m_NorthWestWidget ->show();
    m_NorthEastWidget ->show();
    m_SouthWestWidget ->show();
    m_SouthEastWidget ->show();
}

void MainWindow::HideAllEdgeWidgets(){
    m_DialogFrame->hide();
    if(!m_TitleBar) return;
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

void MainWindow::RaiseAllEdgeWidgets(){
    m_DialogFrame->raise();
    if(!m_TitleBar) return;
    m_NorthWidget     ->raise();
    m_SouthWidget     ->raise();
    m_WestWidget      ->raise();
    m_EastWidget      ->raise();

    // for widget's stack order.
    m_TitleBar        ->raise();

    m_NorthWestWidget ->raise();
    m_NorthEastWidget ->raise();
    m_SouthWestWidget ->raise();
    m_SouthEastWidget ->raise();
}

void MainWindow::AdjustAllEdgeWidgets(){
    QRect rect = geometry();
    m_DialogFrame->setGeometry(rect.left(), rect.top(), rect.width(), m_DialogFrame->height());
    m_DialogFrame->Adjust();
    if(!m_TitleBar) return;
    if(IsShaded()) rect.setHeight(0);
    static const int e = EDGE_WIDGET_SIZE;
    static const int t = TITLE_BAR_HEIGHT;
    static const int et = e+t;
    int t0 = isMaximized() ? 0 : t;
    m_TitleBar        ->setGeometry(QRect(rect.left(), rect.top()-t0, rect.width(), t));
    m_NorthWidget     ->setGeometry(QRect(rect.left()-1, rect.top()-et, rect.width()+2, e));
    m_SouthWidget     ->setGeometry(QRect(rect.left()-1, rect.bottom()+1, rect.width()+2, e));
    m_WestWidget      ->setGeometry(QRect(rect.left()-e, rect.top()-t-1, e, rect.height()+t+2));
    m_EastWidget      ->setGeometry(QRect(rect.right()+1, rect.top()-t-1, e, rect.height()+t+2));

    m_NorthWestWidget ->setGeometry(QRect(rect.left()-e, rect.top()-et, e+3, e+3));
    m_NorthEastWidget ->setGeometry(QRect(rect.right()+1-3, rect.top()-et, e+3, e+3));
    m_SouthWestWidget ->setGeometry(QRect(rect.left()-e, rect.bottom()+1-3, e+3, e+3));
    m_SouthEastWidget ->setGeometry(QRect(rect.right()+1-3, rect.bottom()+1-3, e+3, e+3));
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
    } else if(!IsMenuBarEmpty()){
        ClearMenuBar();
    }
}

void MainWindow::SetTreeBar(bool on){
    if(on && m_TreeBar->isHidden()){
        m_TreeBar->show();
    } else if(!m_TreeBar->isVisible()){
        m_TreeBar->hide();
    }
}

void MainWindow::SetToolBar(bool on){
    if(on && m_ToolBar->isHidden()){
        m_ToolBar->show();
    } else if(!m_ToolBar->isVisible()){
        m_ToolBar->hide();
    }
}

void MainWindow::SetFullScreen(bool on){
    if(on && !isFullScreen()){
        showFullScreen();
    } else if(isFullScreen()){
        showNormal();
    }
    SetFocus();
}

void MainWindow::SetMaximized(bool on){
    if(on && !isMaximized()){
        showMaximized();
    } else if(isMaximized()){
        showNormal();
    }
    SetFocus();
}

void MainWindow::SetMinimized(bool on){
    if(on && !isMinimized()){
        showMinimized();
    } else if(isMinimized()){
        showNormal();
        SetFocus();
    }
}

void MainWindow::SetShaded(bool on){
    if(on && !IsShaded()){
        Shade();
    } else if(IsShaded()){
        Unshade();
        SetFocus();
    }
}

void MainWindow::SetFocus(){
    raise();
    activateWindow();
    if(GetTreeBank()->GetGadgets()->IsActive()){
        GetTreeBank()->GetView()->setFocus();
        GetTreeBank()->GetGadgets()->setFocus();
    } else if(SharedView view = GetTreeBank()->GetCurrentView()){
        view->setFocus();
    }
}

void MainWindow::paintEvent(QPaintEvent *ev){
#if defined(Q_OS_WIN)
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    foreach(QRect rect, ev->region().rects()){
        painter.fillRect(rect, Qt::BrushStyle::SolidPattern);
    }
#endif
    QMainWindow::paintEvent(ev);
}

void MainWindow::closeEvent(QCloseEvent *ev){

    if(Application::GetMainWindows().count() == 1){
        QMainWindow::closeEvent(ev);
        Application::Quit();
    } else {
        Application::RemoveWindow(this);

        foreach(QObject *child, m_TreeBank->children()){
            if(View *view = dynamic_cast<View*>(child)){
                view->setParent(Application::GetCurrentWindow()->GetTreeBank());
                // 'setParent' of 'QWidget' is not actually performed until calling 'show'.
                view->lower();
                view->show();
                view->hide();
            }
        }
        foreach(QGraphicsItem *item, m_TreeBank->GetScene()->items()){
            if(View *view = dynamic_cast<View*>(item)){
                view->setParent(0);
            }
        }
        foreach(QObject *child, windowHandle()->children()){
            if(View *view = dynamic_cast<View*>(child)){
                view->setParent(Application::GetCurrentWindow()->GetTreeBank());
                view->hide();
            }
        }
        // segv on destructor of treebank.
        //foreach(View *view, TreeBank::GetAllViews()){
        //    if(view->parent() == m_TreeBank)
        //        view->setParent(0);
        //}

        QMainWindow::closeEvent(ev);
        RemoveSettings();
        TreeBank::SaveSettings();
        Application::SaveGlobalSettings();
        Application::SaveSettingsFile();
        // 'RemoveWindow' doesn't call 'SwitchWindow',
        // but calls 'setFocus' manually.
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
            if(0);
#ifdef WEBENGINEVIEW
            else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                w->setGeometry(geometry());
            else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                w->setGeometry(geometry());
#endif
#ifdef NATIVEWEBVIEW
            else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                w->setGeometry(geometry());
#endif
#ifdef TRIDENTVIEW
            else if(TridentView *w = qobject_cast<TridentView*>(view->base()))
                w->setGeometry(geometry());
#endif
            else;
        }
    }
    AdjustAllEdgeWidgets();
    RaiseAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()){
        notifier->ResizeNotify(m_TreeBank->size());
        notifier->raise();
    }
    Receiver *receiver = m_TreeBank->GetReceiver();
    if(receiver && receiver->IsPurged()){
        receiver->ResizeNotify(m_TreeBank->size());
        receiver->raise();
    }
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
#ifdef TRIDENTVIEW
            else if(TridentView *w = qobject_cast<TridentView*>(view->base()))
                w->show();
#endif
            else;
        }
    }
    ShowAllEdgeWidgets();
    Notifier *notifier = m_TreeBank->GetNotifier();
    if(notifier && notifier->IsPurged()) notifier->show();
    //Receiver *receiver = m_TreeBank->GetReceiver();
    //if(receiver && receiver->IsPurged()) receiver->show();
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
#ifdef TRIDENTVIEW
            else if(TridentView *w = qobject_cast<TridentView*>(view->base()))
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

#if defined(Q_OS_WIN)

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result){

    bool ret = QMainWindow::nativeEvent(eventType, message, result);

    if(eventType == "windows_generic_MSG"){
        MSG *msg = static_cast<MSG*>(message);

        switch(msg->message){
        case WM_SYSCOMMAND:{
            if(IsMenuBarEmpty() && (msg->wParam & 0xFFF0) == SC_MOUSEMENU){
                QTimer::singleShot(0, this, [this](){
                    QMenu *menu = GetTreeBank()->GlobalContextMenu();
                    menu->exec(QCursor::pos());
                    delete menu;
                });
                return true;
            }
            break;
        }
        case WM_WINDOWPOSCHANGED:{
            if(TreeBank::PurgeView()){
                if(SharedView view = m_TreeBank->GetCurrentView()){
                    if(0);
#ifdef WEBENGINEVIEW
                    else if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base()))
                        w->raise();
                    else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(view->base()))
                        w->raise();
#endif
#ifdef NATIVEWEBVIEW
                    else if(QuickNativeWebView *w = qobject_cast<QuickNativeWebView*>(view->base()))
                        w->raise();
#endif
#ifdef TRIDENTVIEW
                    else if(TridentView *w = qobject_cast<TridentView*>(view->base()))
                        w->raise();
#endif
                    else;
                }
            }
            RaiseAllEdgeWidgets();
            Notifier *notifier = m_TreeBank->GetNotifier();
            if(notifier && notifier->IsPurged()) notifier->raise();
            Receiver *receiver = m_TreeBank->GetReceiver();
            if(receiver && receiver->IsPurged()) receiver->raise();
            break;
        }
        }
    }
    return ret;
}

#endif //defined(Q_OS_WIN)

namespace{
    static const QPen tpen = QPen(QColor(0,0,0,1));
    static const QPen bpen = QPen(QColor(0,0,0,255));
    static const QBrush tbrush = QBrush(QColor(0,0,0,1));
    static const QBrush bbrush = QBrush(QColor(0,0,0,255));
    static const int none   = 0;
    static const int line   = 1 << 0;
    static const int edge   = 1 << 1;
    static const int shadow = 1 << 2;
    static const int disp = shadow;
}

TitleBar::TitleBar(MainWindow *mainwindow)
    : QWidget(0)
    , m_MainWindow(mainwindow)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    m_HoveredButton = NoButton;
    m_Shade = Application::style()->standardIcon(QStyle::SP_TitleBarShadeButton).pixmap(MinimizeAreaRect().size());
    m_Unshade = Application::style()->standardIcon(QStyle::SP_TitleBarUnshadeButton).pixmap(MinimizeAreaRect().size());
    m_Minimize = Application::style()->standardIcon(QStyle::SP_TitleBarMinButton).pixmap(MinimizeAreaRect().size());
    m_Maximize = Application::style()->standardIcon(QStyle::SP_TitleBarMaxButton).pixmap(MaximizeAreaRect().size());
    m_Normal = Application::style()->standardIcon(QStyle::SP_TitleBarNormalButton).pixmap(MaximizeAreaRect().size());
    m_Close = Application::style()->standardIcon(QStyle::SP_TitleBarCloseButton).pixmap(CloseAreaRect().size());
}

void TitleBar::paintEvent(QPaintEvent *ev){
    QPainter painter(this);

    bool isCurrent = m_MainWindow == Application::GetCurrentWindow();

    if(m_MainWindow->isMaximized()){
        painter.setPen(QColor(255,255,255,200));
        painter.setBrush(QColor(255,255,255,isCurrent ? 200 : 128));
        painter.drawRect(QRect(width()-32-28*4 - 6, 0,
                               32+28*4 + 5, height()-1));
    } else {
        painter.setPen(QColor(255,255,255,200));
        painter.setBrush(QColor(255,255,255, isCurrent ? 200 : 128));
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));

        painter.setPen(QColor(0,0,0,255));
        painter.setBrush(QColor(0,0,0,255));
        painter.drawRect(MenuAreaRect());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0,0,0,127));
        if(m_HoveredButton == MenuButton) painter.drawRect(MenuAreaRect1());
    }

    if(width() > 19 + 32 + 28*4 + 5){
        painter.setPen(QColor(0,0,0,255));
        painter.setBrush(QColor(0,0,0,255));
        painter.drawRect(ViewTreeAreaRect());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0,0,0,127));
        if(m_HoveredButton == ViewTreeButton) painter.drawRect(ViewTreeAreaRect1());
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0,0,0,127));

    if(width() > 19 + 32 + 28*3 + 5){
        if(m_MainWindow->IsShaded())
            painter.drawPixmap(ShadeAreaRect(), m_Unshade, QRect(QPoint(), m_Unshade.size()));
        else painter.drawPixmap(ShadeAreaRect(), m_Shade, QRect(QPoint(), m_Shade.size()));
        if(m_HoveredButton == ShadeButton) painter.drawRect(ShadeAreaRect1());
    }
    if(width() > 19 + 32 + 28*2 + 5){
        if(m_MainWindow->isMinimized())
            painter.drawPixmap(MinimizeAreaRect(), m_Normal, QRect(QPoint(), m_Normal.size()));
        else painter.drawPixmap(MinimizeAreaRect(), m_Minimize, QRect(QPoint(), m_Minimize.size()));
        if(m_HoveredButton == MinimizeButton) painter.drawRect(MinimizeAreaRect1());
    }
    if(width() > 19 + 32 + 28*1 + 5){
        if(m_MainWindow->isMaximized())
            painter.drawPixmap(MaximizeAreaRect(), m_Normal, QRect(QPoint(), m_Normal.size()));
        else painter.drawPixmap(MaximizeAreaRect(), m_Maximize, QRect(QPoint(), m_Maximize.size()));
        if(m_HoveredButton == MaximizeButton) painter.drawRect(MaximizeAreaRect1());
    }

    painter.drawPixmap(CloseAreaRect(), m_Close, QRect(QPoint(), m_Close.size()));
    if(m_HoveredButton == CloseButton) painter.drawRect(CloseAreaRect1());

    if(!m_MainWindow->isMaximized()){
        painter.setFont(TITLE_BAR_TITLE_FONT);
        painter.setPen(QColor(0,0,0,255));
        painter.setBrush(Qt::NoBrush);
        painter.drawText(QRect(QPoint(22,2), size()-QSize(171,1)),
                         Qt::TextSingleLine,
                         m_MainWindow->windowTitle());
    }
    ev->setAccepted(true);
}

void TitleBar::mousePressEvent(QMouseEvent *ev){
    m_MainWindow->raise();
    m_Moved = false;
    if(ev->button() == Qt::LeftButton){
        m_Pos = ev->globalPos() - geometry().topLeft() - QPoint(0,TITLE_BAR_HEIGHT);
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        m_Moved = true;
        if(m_HoveredButton == NoButton)
            m_MainWindow->move(ev->globalPos() - m_Pos);
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
            menu->exec(ev->globalPos());
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
        /* do nothing. */
    } else if(ev->button() == Qt::LeftButton){
        m_MainWindow->ToggleMaximized();
    }
    ev->setAccepted(true);
}

void TitleBar::enterEvent(QEvent *ev){
    Q_UNUSED(ev);
}

void TitleBar::leaveEvent(QEvent *ev){
    Q_UNUSED(ev);
    m_HoveredButton = NoButton;
    repaint();
}

QRect TitleBar::MenuAreaRect() const {
    return QRect(5,5,10,10);
}

QRect TitleBar::ViewTreeAreaRect() const {
    return QRect(width()-29-28*4,5,10,10);
}

QRect TitleBar::ShadeAreaRect() const {
    return QRect(width()-28-28*3,6,10,10);
}

QRect TitleBar::MinimizeAreaRect() const {
    return QRect(width()-28-28*2,6,10,10);
}

QRect TitleBar::MaximizeAreaRect() const {
    return QRect(width()-28-28*1,6,10,10);
}

QRect TitleBar::CloseAreaRect() const {
    return QRect(width()-27-28*0,6,10,10);
}

MainWindowEdgeWidget::MainWindowEdgeWidget(MainWindow *mainwindow)
    : QWidget(0)
    , m_MainWindow(mainwindow)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

void MainWindowEdgeWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(tpen);
    painter.setBrush(tbrush);
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

void MainWindowEdgeWidget::enterEvent(QEvent *ev){
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
                 pos.x() - rect.left() - m_Pos.x() + 3,
                 rect.bottom() - pos.y() - et + m_Pos.y() + 1);
}

QRect MainWindowSouthWestWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(pos.x() + e - m_Pos.x(),
                 rect.y(),
                 rect.right() - pos.x() - e + m_Pos.x() + 1,
                 pos.y() - rect.top() - m_Pos.y() + 3);
}

QRect MainWindowSouthEastWidget::ComputeNewRect(QRect rect, QPoint pos){
    return QRect(rect.x(),
                 rect.y(),
                 pos.x() - rect.left() - m_Pos.x() + 3,
                 pos.y() - rect.top() - m_Pos.y() + 3);
}

#define EDGE_SHADOW_COLOR qRgba(0,0,0,c)

void MainWindowNorthWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                for(i=0; i < width(); i++){
                    for(j=0; j < height(); j++){
                        w = width();
                        c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(i, j, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,9), QPoint(width(),9));
        }
    }
    ev->setAccepted(true);
}

void MainWindowSouthWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                for(i=0; i < width(); i++){
                    for(j=0; j < height(); j++){
                        w = width();
                        c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(i, e-j-1, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,0), QPoint(width(),0));
        }
    }
    ev->setAccepted(true);
}

void MainWindowWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                for(i=0; i < height(); i++){
                    for(j=0; j < width(); j++){
                        w = height();
                        c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(j, i, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(9,0), QPoint(9,height()));
        }
    }
    ev->setAccepted(true);
}

void MainWindowEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    if(disp == none){
        painter.setPen(tpen);
        painter.setBrush(tbrush);
        painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    } else {
        if(disp & shadow){
            if(m_MainWindow == Application::GetCurrentWindow()){
                QImage image(size(), QImage::Format_ARGB32);
                image.fill(0);
                double i, j, w, c;
                for(i=0; i < height(); i++){
                    for(j=0; j < width(); j++){
                        w = height();
                        c = 77*j*j/e/e;
                        if(  i<25) c = c*sqrt(   i /25);
                        if(w-i<25) c = c*sqrt((w-i)/25);
                        image.setPixel(e-j-1, i, EDGE_SHADOW_COLOR);
                    }
                }
                painter.drawImage(QPoint(), image);
            } else {
                painter.setPen(tpen);
                painter.setBrush(tbrush);
                painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
            }
        }
        if(disp & edge){
            painter.setPen(bpen);
            painter.setBrush(tbrush);
            painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
        }
        if(disp & line){
            painter.setPen(bpen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPoint(0,0), QPoint(0,height()));
        }
    }
    ev->setAccepted(true);
}

void MainWindowNorthWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowNorthEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowSouthWestWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
    painter.drawRect(QRect(QPoint(), size()-QSize(1,1)));
    ev->setAccepted(true);
}

void MainWindowSouthEastWidget::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setPen(disp & edge ? bpen : tpen);
    painter.setBrush(tbrush);
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
