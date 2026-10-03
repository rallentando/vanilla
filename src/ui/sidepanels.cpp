#include "switch.hpp"
#include "const.hpp"

#include "sidepanels.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDockWidget>
#include <QEvent>
#include <QStackedWidget>
#include <QTimer>
#ifdef WEBENGINEVIEW
#include <QWebEngineView>
#endif
#include <functional>
#include <memory>

#include "application.hpp"
#include "extensioncontroller.hpp"
#include "extensionhost.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "view.hpp"

namespace {

    const int FIRST_WIDTH = 360;
    const char NO_WINDOW[] = "No window with id: 1.";
    const char NO_PANEL[] = "No active side panel for windowId: 1";

    bool AnyViewOf(Node *node, const ExtensionController *controller){
        if(!node) return false;
        foreach(Node *child, node->GetChildren()){
            ViewNode *vn = child->ToViewNode();
            if(!vn) continue;
            if(vn->IsDirectory()){
                if(AnyViewOf(vn, controller)) return true;
                continue;
            }
            if(View *view = vn->GetView()) if(view->Extensions() == controller) return true;
        }
        return false;
    }

    ViewNode *TabOf(qint64 serial){
        return serial > 0 ? TreeBank::TabOfSerial(static_cast<quint64>(serial)) : nullptr;
    }

    SharedView CurrentViewOf(MainWindow *window){
        TreeBank *bank = window ? window->GetTreeBank() : nullptr;
        return bank ? bank->GetCurrentView() : SharedView();
    }
    qint64 CurrentTabOf(MainWindow *window){
        const SharedView view = CurrentViewOf(window);
        return view && view->GetViewNode() ? static_cast<qint64>(view->GetViewNode()->GetSerial()) : 0;
    }

    QHash<QPair<const void*, QString>, qint64> &ClosedByUser(){
        static QHash<QPair<const void*, QString>, qint64> closed;
        return closed;
    }
}

SidePanels::SidePanels(MainWindow *window)
    : QObject(window)
    , m_Window(window)
{
    m_Dock = new QDockWidget(window);
    m_Dock->setObjectName(QStringLiteral("sidepanel"));
    m_TabDock = new QDockWidget(window);
    m_TabDock->setObjectName(QStringLiteral("sidepaneltab"));
    m_Stack = new QStackedWidget(m_Dock);
    m_TabStack = new QStackedWidget(m_TabDock);
    m_Dock->setWidget(m_Stack);
    m_TabDock->setWidget(m_TabStack);
    for(QDockWidget *dock : { m_Dock, m_TabDock }){
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->installEventFilter(this);
        window->addDockWidget(Qt::RightDockWidgetArea, dock);
    }
    window->tabifyDockWidget(m_Dock, m_TabDock);
    m_Dock->hide();
    m_TabDock->hide();
    All().append(this);
}

SidePanels::~SidePanels(){
    All().removeAll(this);
    for(const Panel &panel : m_Panels)
        if(panel.controller) ExtensionHost::SidePanelEvent(panel.controller, panel.id, false, panel.path, panel.tab);
}

QList<QDockWidget*> SidePanels::Docks() const {
    return QList<QDockWidget*>() << m_Dock << m_TabDock;
}

QList<SidePanels*> &SidePanels::All(){
    static QList<SidePanels*> all;
    return all;
}

bool SidePanels::Shows(const ExtensionController *controller) const {
    const SharedView view = CurrentViewOf(m_Window);
    return controller && view && view->Extensions() == controller;
}

int SidePanels::IndexOf(const ExtensionController *controller) const {
    for(int i = 0; i < m_Panels.size(); i++)
        if(m_Panels.at(i).tab == 0 && m_Panels.at(i).controller == controller) return i;
    return -1;
}

int SidePanels::IndexOfTab(qint64 tab) const {
    for(int i = 0; i < m_Panels.size(); i++)
        if(tab > 0 && m_Panels.at(i).tab == tab) return i;
    return -1;
}

int SidePanels::IndexOfPage(const QWidget *page) const {
    for(int i = 0; i < m_Panels.size(); i++)
        if(page && m_Panels.at(i).page == page) return i;
    return -1;
}

QString SidePanels::Open(ExtensionController *controller, const QString &id, qint64 tab){
    if(!controller) return QString::fromLatin1(NO_WINDOW);
    if(tab > 0){
        const QUrl url = controller->SidePanelTabUrl(id, tab);
        if(!url.isEmpty()){
            ViewNode *vn = TabOf(tab);
            View *view = vn ? vn->GetView() : nullptr;
            if(!view || view->Extensions() != controller) return QStringLiteral("No tab with id: %1.").arg(tab);
            SidePanels *target = nullptr;
            for(SidePanels *one : All()) if(!target && CurrentTabOf(one->m_Window) == tab) target = one;
            if(!target && Application::GetCurrentWindow()) target = Application::GetCurrentWindow()->GetSidePanels();
            if(!target && !All().isEmpty()) target = All().first();
            if(!target) return QString::fromLatin1(NO_WINDOW);
            for(SidePanels *one : QList<SidePanels*>(All())){
                if(one == target) continue;
                const int at = one->IndexOfTab(tab);
                if(at < 0) continue;
                one->Drop(at, false);
                one->Update();
            }
            return target->OpenHere(controller, id, url, tab, view);
        }
    }
    const QUrl url = controller->SidePanelUrl(id);
    if(url.isEmpty()) return QString::fromLatin1(NO_PANEL);
    SidePanels *target = nullptr;
    if(MainWindow *current = Application::GetCurrentWindow())
        if(current->GetSidePanels() && current->GetSidePanels()->Shows(controller)) target = current->GetSidePanels();
    for(SidePanels *one : All()) if(!target && one->Shows(controller)) target = one;
    if(!target) return QString::fromLatin1(NO_WINDOW);
    return target->OpenHere(controller, id, url, 0, CurrentViewOf(target->m_Window).get());
}

QString SidePanels::Close(ExtensionController *controller, const QString &id, qint64 tab){
    if(tab > 0){
        bool closed = false;
        for(SidePanels *one : QList<SidePanels*>(All())){
            const int at = one->IndexOfTab(tab);
            if(at < 0 || one->m_Panels.at(at).controller != controller || one->m_Panels.at(at).id != id) continue;
            one->Drop(at, false);
            one->Update();
            closed = true;
        }
        if(closed) return QString();
    }
    bool closed = false;
    for(SidePanels *one : QList<SidePanels*>(All())){
        const int at = one->IndexOf(controller);
        if(at < 0 || one->m_Panels.at(at).id != id) continue;
        one->Drop(at, false);
        one->Update();
        closed = true;
    }
    return closed ? QString() : QString::fromLatin1(NO_PANEL);
}

QString SidePanels::Toggle(ExtensionController *controller, const QString &id){
    if(!Shows(controller)) return QString::fromLatin1(NO_WINDOW);
    const int at = IndexOf(controller);
    if(at >= 0 && m_Panels.at(at).id == id){
        Drop(at, true);
        Update();
        return QString();
    }
    const QUrl url = controller->SidePanelUrl(id);
    if(url.isEmpty()) return QString::fromLatin1(NO_PANEL);
    return OpenHere(controller, id, url, 0, CurrentViewOf(m_Window).get());
}

void SidePanels::ShutdownPages(){
    QList<QPointer<QWidget> > pages;
    for(const Panel &panel : m_Panels) if(panel.page) pages << panel.page;
    for(const QPointer<QWidget> &page : pages){
        if(!page) continue;
        QEvent shutdown(View::SidePanelShutdownEvent());
        QCoreApplication::sendEvent(page.data(), &shutdown);
    }
}

void SidePanels::ShutdownEverywhere(){
    for(SidePanels *one : QList<SidePanels*>(All())) one->ShutdownPages();
}

void SidePanels::UpdateAll(){
    for(SidePanels *one : QList<SidePanels*>(All())) one->Update();
}

qint64 SidePanels::ClosedByUserAt(const ExtensionController *controller, const QString &id){
    return ClosedByUser().value(qMakePair(static_cast<const void*>(controller), id));
}

QString SidePanels::OpenHere(ExtensionController *controller, const QString &id, const QUrl &url, qint64 tab, View *from){
    const int have = tab > 0 ? IndexOfTab(tab) : IndexOf(controller);
    if(have >= 0 && m_Panels.at(have).controller == controller && m_Panels.at(have).id == id &&
       m_Panels.at(have).page && m_Panels.at(have).url == url){
        Update();
        return QString();
    }
    if(!from || from->Extensions() != controller) return QString::fromLatin1(NO_WINDOW);
    const QPair<const void*, qint64> slot = qMakePair(static_cast<const void*>(controller), tab);
    const quint64 serial = ++m_Serial;
    m_Making.insert(slot, serial);
    QPointer<SidePanels> self(this);
    QPointer<ExtensionController> of(controller);
    const std::shared_ptr<QPointer<QWidget> > made = std::make_shared<QPointer<QWidget> >();
    const std::function<void()> closed = [self, made](){
        QTimer::singleShot(0, qApp, [self, made](){ if(self && *made) self->ClosePage(made->data(), false); });
    };
    QStackedWidget *stack = tab > 0 ? m_TabStack : m_Stack;
    QWidget *page = from->CreateExtensionView(url, View::ExtensionSidePanelPage, stack, closed);
    *made = page;
    if(!self) return QString::fromLatin1(NO_WINDOW);
    if(!page){
        m_Making.remove(slot);
        return QStringLiteral("chrome.sidePanel.open is not available in this view");
    }
    if(!of || m_Making.value(slot) != serial){
        page->deleteLater();
        return QString();
    }
    m_Making.remove(slot);
    const int old = tab > 0 ? IndexOfTab(tab) : IndexOf(controller);
    if(old >= 0) Drop(old, false);
    Panel panel;
    panel.controller = controller;
    panel.id = id;
    panel.tab = tab;
    panel.version = controller->RowOf(id).manifest.version;
    panel.path = tab > 0 ? controller->SidePanelFor(id).TabPath(tab) : controller->SidePanelPath(id);
    panel.url = url;
    panel.page = page;
    m_Panels.append(panel);
    stack->addWidget(page);
    m_Raise = page;
    ExtensionHost::SidePanelEvent(controller, id, true, panel.path, tab);
    Update();
    return QString();
}

void SidePanels::Drop(int index, bool byUser){
    const Panel panel = m_Panels.takeAt(index);
    if(byUser) ClosedByUser().insert(qMakePair(static_cast<const void*>(panel.controller.data()), panel.id),
                                     QDateTime::currentMSecsSinceEpoch());
    if(panel.page){
        (panel.tab > 0 ? m_TabStack : m_Stack)->removeWidget(panel.page);
        panel.page->hide();
        QCoreApplication::postEvent(panel.page, new QEvent(View::SidePanelShutdownEvent()));
        panel.page->deleteLater();
    }
    if(panel.controller) ExtensionHost::SidePanelEvent(panel.controller, panel.id, false, panel.path, panel.tab);
}

void SidePanels::ClosePage(const QWidget *page, bool byUser){
    const int at = IndexOfPage(page);
    if(at < 0) return;
    Drop(at, byUser);
    Update();
}

void SidePanels::Prune(){
    QList<QPair<QPointer<QWidget>, QString> > moved;
    for(int i = m_Panels.size() - 1; i >= 0; i--){
        Panel &panel = m_Panels[i];
        bool keep = panel.controller && panel.page;
        QString path;
        if(keep){
            const ExtensionRow row = panel.controller->RowOf(panel.id);
            const ExtensionUi::SidePanel &options = panel.controller->SidePanelFor(panel.id);
            path = panel.tab > 0 ? options.TabPath(panel.tab) : panel.controller->SidePanelPath(panel.id);
            keep = row.manifest.id == panel.id && row.registered && row.wanted && row.loaded && row.enabled
                && row.manifest.version == panel.version
                && panel.controller->HasPermission(panel.id, QStringLiteral("sidePanel"))
                && !path.isEmpty()
                && (panel.tab > 0 ? options.EnabledFor(panel.tab) && TabOf(panel.tab)
                                  : options.Enabled())
                && AnyViewOf(TreeBank::GetViewRoot(), panel.controller);
        }
        if(!keep){
            Drop(i, false);
            continue;
        }
        if(path != panel.path) moved << qMakePair(QPointer<QWidget>(panel.page), path);
    }
    for(const QPair<QPointer<QWidget>, QString> &one : moved){
        int at = IndexOfPage(one.first.data());
        if(at < 0) continue;
        const QUrl url = m_Panels.at(at).controller ? m_Panels.at(at).controller->SidePanelResource(m_Panels.at(at).id, one.second) : QUrl();
        bool read = false;
        if(!url.isEmpty()){
#ifdef WEBENGINEVIEW
            if(QWebEngineView *view = qobject_cast<QWebEngineView*>(one.first.data())){
                view->setUrl(url);
                read = true;
            }
#endif
            if(!read && one.first){
                View::SidePanelNavigate navigate(url);
                QCoreApplication::sendEvent(one.first.data(), &navigate);
                read = navigate.isAccepted();
            }
        }
        at = IndexOfPage(one.first.data());
        if(at < 0) continue;
        if(!read){
            Drop(at, false);
            continue;
        }
        m_Panels[at].path = one.second;
        m_Panels[at].url = url;
    }
}

void SidePanels::Update(){
    Prune();
    const SharedView view = CurrentViewOf(m_Window);
    ExtensionController *controller = view ? view->Extensions() : nullptr;
    const qint64 tab = CurrentTabOf(m_Window);
    int own = IndexOfTab(tab);
    if(own >= 0 && (m_Panels.at(own).controller != controller || !m_Panels.at(own).page)) own = -1;
    int all = controller ? IndexOf(controller) : -1;
    if(all >= 0){
        const ExtensionUi::SidePanel &options = controller->SidePanelFor(m_Panels.at(all).id);
        if(!m_Panels.at(all).page || !options.TabPath(tab).isEmpty() || !options.EnabledFor(tab) ||
           (own >= 0 && m_Panels.at(own).id == m_Panels.at(all).id)) all = -1;
    }
    const bool first = !(m_Placed & 1) && (all >= 0 || own >= 0);
    if(own >= 0 && !(m_Placed & 2)){
        m_Window->tabifyDockWidget(m_Dock, m_TabDock);
        m_Placed |= 2;
    }
    Show(m_Dock, m_Stack, all >= 0 ? &m_Panels.at(all) : nullptr);
    Show(m_TabDock, m_TabStack, own >= 0 ? &m_Panels.at(own) : nullptr);
    if(m_Raise){
        QDockWidget *dock = m_Raise->parentWidget() == m_TabStack ? m_TabDock : m_Dock;
        const QStackedWidget *stack = dock == m_TabDock ? m_TabStack : m_Stack;
        if(dock->isVisible() && stack->currentWidget() == m_Raise.data()){
            dock->raise();
            m_Raise.clear();
        }
    }
    if(first){
        QList<QDockWidget*> shown;
        for(QDockWidget *dock : { m_Dock, m_TabDock }) if(dock->isVisible() && !dock->isFloating()) shown << dock;
        if(!shown.isEmpty()){
            QList<int> widths;
            for(int k = 0; k < shown.size(); k++) widths << m_Window->ScaleByDevice(FIRST_WIDTH);
            m_Window->resizeDocks(shown, widths, Qt::Horizontal);
        }
        m_Placed |= 1;
    }
}

void SidePanels::Show(QDockWidget *dock, QStackedWidget *stack, const Panel *panel){
    if(!panel || !panel->page){
        dock->hide();
        return;
    }
    stack->setCurrentWidget(panel->page);
    dock->setWindowTitle(panel->controller ? panel->controller->RowOf(panel->id).manifest.name : QString());
    if(!dock->isVisible()) dock->show();
}

bool SidePanels::eventFilter(QObject *watched, QEvent *ev){
    QDockWidget *dock = watched == m_Dock ? m_Dock : watched == m_TabDock ? m_TabDock : nullptr;
    if(dock && ev->type() == QEvent::Close && dock->isVisible()){
        const int at = IndexOfPage((dock == m_TabDock ? m_TabStack : m_Stack)->currentWidget());
        if(at >= 0){
            Drop(at, true);
            QPointer<SidePanels> self(this);
            QTimer::singleShot(0, this, [self](){ if(self) self->Update(); });
        }
    }
    return QObject::eventFilter(watched, ev);
}
