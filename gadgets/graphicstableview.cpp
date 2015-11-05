#include "switch.hpp"
#include "const.hpp"

#include "graphicstableview.hpp"

#include <QtCore>
#include <QGraphicsObject>
#include <QtWidgets>

#include "application.hpp"
#include "treebank.hpp"
#include "mainwindow.hpp"
#include "view.hpp"
#include "gadgetsstyle.hpp"

/*
  ZValue
 -10.0 : hidden contents layer.
   0.0 : 'view' contents layer.
   5.0 : 'localview' contents layer.
  10.0 : gadgets' main contents layer.
  20.0 : gadgets' spot light layer.
  30.0 : gadgets' dragging contents layer.
  40.0 : gadgets' selector layer.
  50.0 : gadgets' in place notifier layer.
  60.0 : localview's multimedia item layer.

  Scroll is displayed by only using boundingRect.

  ZoomFactor controls Thumbnail's max width(whole).
 */

NodeList GraphicsTableView::m_NodesRegister = NodeList();

GadgetsStyle *GraphicsTableView::m_Style = 0;

bool GraphicsTableView::m_ScrollToChangeDirectory = false;
bool GraphicsTableView::m_RightClickToRenameNode  = false;
bool GraphicsTableView::m_EnablePrimarySpotLight  = false;
bool GraphicsTableView::m_EnableHoveredSpotLight  = false;
bool GraphicsTableView::m_EnableLoadedSpotLight   = false;
bool GraphicsTableView::m_EnableInPlaceNotifier   = false;

GraphicsTableView::GraphicsTableView(TreeBank *parent)
    : QGraphicsObject(0)
{
    m_TreeBank = parent;

    setAcceptHoverEvents(true);
    setAcceptDrops(true);
    setEnabled(true);
    setFlag(QGraphicsItem::ItemIsFocusable);

    setZValue(MAIN_CONTENTS_LAYER);

    class DummyViewNode : public ViewNode{
        bool IsDummy() DECL_OVERRIDE { return true;}
    };

    class DummyHistNode : public HistNode{
        bool IsDummy() DECL_OVERRIDE { return true;}
    };

    m_DummyViewNode = new DummyViewNode();
    m_DummyHistNode = new DummyHistNode();
    m_LoadedSpotLights = QList<SpotLight*>();

    m_PrimarySpotLight =
        m_EnablePrimarySpotLight ? new SpotLight(PrimarySpotLight, this) : 0;
    m_HoveredSpotLight =
        m_EnableHoveredSpotLight ? new SpotLight(HoveredSpotLight, this) : 0;
    m_InPlaceNotifier =
        m_EnableInPlaceNotifier  ? new InPlaceNotifier(this) : 0;

    m_ScrollController = new ScrollController(this);
    m_UpDirectoryButton = new UpDirectoryButton(this);

    m_CurrentThumbnailZoomFactor = 1.0f;
    m_CurrentOffsetValue = 0;
    m_SelectRect = 0;
    m_CurrentNode = 0;
    m_StatusBarMessageIsSuspended = false;

    // reset scroll.
    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    m_SortFlags = NoSort;
    m_ToggleSortReverseAction = 0;
    m_ToggleSortByUrlAction = 0;
    m_ToggleSortByTitleAction = 0;
    m_ToggleSortByCreateDateAction = 0;
    m_ToggleSortByLastUpdateDateAction = 0;
    m_ToggleSortByLastAccessDateAction = 0;
    SetSortPredicate();
}

GraphicsTableView::~GraphicsTableView(){
    m_DummyViewNode->Delete();
    m_DummyHistNode->Delete();

    // needless to delete, becouse 'QAction' inherits 'QObject',
    // and action's parent is specified 'GraphicsTableView'.
    //foreach(QAction *action, m_ActionTable.values()){
    //    delete action;
    //}
}

void GraphicsTableView::LoadSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("gadgets"));{
        QString style = settings->value(QStringLiteral("@Style"), QStringLiteral("GlassStyle")).value<QString>();

        GadgetsStyle *style_ = m_Style;

        if(style == QStringLiteral("GlassStyle")){
            m_Style = new GlassStyle;
        }
        if(style == QStringLiteral("FlatStyle")){
            m_Style = new FlatStyle;
        }

        if(style_) delete style_;

        settings->beginGroup(QStringLiteral("thumblist"));{
            m_ScrollToChangeDirectory = settings->value(QStringLiteral("@ScrollToChangeDirectory"), false).value<bool>();
            m_RightClickToRenameNode  = settings->value(QStringLiteral("@RightClickToRenameNode"),  false).value<bool>();
            m_EnablePrimarySpotLight  = settings->value(QStringLiteral("@EnablePrimarySpotLight"),  false).value<bool>();
            m_EnableHoveredSpotLight  = settings->value(QStringLiteral("@EnableHoveredSpotLight"),   true).value<bool>();
            m_EnableLoadedSpotLight   = settings->value(QStringLiteral("@EnableLoadedSpotLight"),   false).value<bool>();
            m_EnableInPlaceNotifier   = settings->value(QStringLiteral("@EnableInPlaceNotifier"),    true).value<bool>();
        }
        settings->endGroup();
    }
    settings->endGroup();
    Thumbnail::Initialize();
    NodeTitle::Initialize();
}

void GraphicsTableView::SaveSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("gadgets"));{

        settings->setValue(QStringLiteral("@Style"), GetStyle()->StyleName());

        settings->beginGroup(QStringLiteral("thumblist"));{
            settings->setValue(QStringLiteral("@ScrollToChangeDirectory"), m_ScrollToChangeDirectory);
            settings->setValue(QStringLiteral("@RightClickToRenameNode"),  m_RightClickToRenameNode);
            settings->setValue(QStringLiteral("@EnablePrimarySpotLight"),  m_EnablePrimarySpotLight);
            settings->setValue(QStringLiteral("@EnableHoveredSpotLight"),  m_EnableHoveredSpotLight);
            settings->setValue(QStringLiteral("@EnableLoadedSpotLight"),   m_EnableLoadedSpotLight);
            settings->setValue(QStringLiteral("@EnableInPlaceNotifier"),   m_EnableInPlaceNotifier);
        }
        settings->endGroup();
    }
    settings->endGroup();
}

void GraphicsTableView::Activate(DisplayType type){
    if(m_TreeBank){
        m_TreeBank->BeforeStartingDisplayGadgets();
    }
    show();
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::ArrowCursor);
    m_DisplayType = type;

    // reset scroll.
    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;
}

void GraphicsTableView::Deactivate(){
    // for multi mode(thumblist and accesskey).
    foreach(QGraphicsItem *item, childItems()){
        item->setSelected(false);
        item->setVisible(false);
        item->setEnabled(false);
    }
    m_NodesRegister.clear();

    if(m_TreeBank){
        m_TreeBank->AfterFinishingDisplayGadgets();
    }
    hide();
}

void GraphicsTableView::SetCurrent(Node *nd){
    if(!nd || !nd->GetParent()){
        if(IsDisplayingViewNode() && (!nd || nd->IsViewNode())){
            m_DummyViewNode->SetParent(m_TreeBank->GetViewRoot());
            nd = m_DummyViewNode;
        } else if(IsDisplayingHistNode() && (!nd || nd->IsHistNode())){
            m_DummyHistNode->SetParent(m_TreeBank->GetHistRoot());
            nd = m_DummyHistNode;
        } else {
            return;
        }
    }
    CollectNodes(nd);
}

// this calls 'SetScroll', 'SetScroll' calls 'update()'.
void GraphicsTableView::CollectNodes(Node *nd, QString filter){
    if(!nd) return;

    foreach(QGraphicsItem *item, childItems()){
        item->setSelected(false);
        item->setVisible(false);
        item->setEnabled(false);
    }
    m_DisplayThumbnails.clear();
    m_DisplayNodeTitles.clear();

    if(!nd->GetParent()){
        if(nd->GetPrimary()){
            nd = nd->GetPrimary();
        } else if(!nd->HasNoChildren()){
            nd = nd->GetFirstChild();
        } else if(nd->IsViewNode()){
            m_DummyViewNode->SetParent(nd);
            nd = m_DummyViewNode;
        } else if(nd->IsHistNode()){
            m_DummyHistNode->SetParent(nd);
            nd = m_DummyHistNode;
        } else {
            return;
        }
    }

    if(!nd->IsDummy())
        nd->ResetPrimaryPath();

    QString title = nd->GetParent()->GetTitle();
    if(!title.isEmpty()){
        emit titleChanged(QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first());
    }

    QUrl url = nd->GetParent()->GetUrl();
    if(!url.isEmpty()){
        emit urlChanged(url);
    }

    // register thumbnail and nodetitle object.
    // if exists in cache, use it.
    std::function<void(Node*, int)> convertOneNode = [filter, this](Node *nd, int nest){
        if(!filter.isEmpty()){
            if(filter.startsWith(QStringLiteral("/")) && filter.endsWith(QStringLiteral("/"))){
                QRegExp reg = QRegExp(filter.mid(1, filter.length() - 2));
                if(!nd->GetTitle().contains(reg))
                    return;
            } else {
                QStringList keys = filter.split(QRegExp(QStringLiteral("[\n\r\t ]+")));
                foreach(QString key, keys){
                    if(!nd->GetTitle().toLower().contains(key.toLower()))
                        return;
                }
            }
        }
        Thumbnail *thumb = 0;
        NodeTitle *title = 0;

        if(m_ThumbnailCache.contains(nd)){
            thumb = m_ThumbnailCache[nd];
            thumb->SetNest(nest);
        } else {
            thumb = new Thumbnail(nd, nest, this);
            m_ThumbnailCache[nd] = thumb;
        }

        if(m_NodeTitleCache.contains(nd)){
            title = m_NodeTitleCache[nd];
            title->SetNest(nest);
        } else {
            title = new NodeTitle(nd, nest, this);
            m_NodeTitleCache[nd] = title;
        }

        thumb->SetPrimary(nd->IsPrimaryOfParent());

        m_DisplayThumbnails << thumb;
        m_DisplayNodeTitles << title;
    };

    NodeCollectionType type = GetNodeCollectionType();

    switch (type){
    case Flat:{

        foreach(Node *nd, nd->GetSiblings()){
            convertOneNode(nd, 0);
        }
        break;
    }
    case Straight:{

        // initialize.
        Node *now = TreeBank::GetRoot(nd);

        if(m_DisplayType == ViewTree || m_DisplayType == TrashTree){
            if(now->GetPrimary())
                now = now->GetPrimary();
            else if(!now->HasNoChildren())
                now = now->GetFirstChild();
        }

        for(int i = 0; now != 0; i++){
            convertOneNode(now, i);
            if(now->GetPrimary())
                now = now->GetPrimary();
            else if(!now->HasNoChildren())
                now = now->GetFirstChild();
            else
                now = 0;
        }
        break;
    }
    case Recursive:{

        std::function<void (Node*, int)> convertNodeRec;

        convertNodeRec = [&](Node *nd, int nest){
            convertOneNode(nd, nest);
            foreach(Node *n, nd->GetChildren()){
                convertNodeRec(n, nest + 1);
            }
        };

        if(IsDisplayingViewNode()){
            foreach(Node *nd, nd->GetSiblings()){
                convertNodeRec(nd, 0);
            }
        } else if(IsDisplayingHistNode()){
            convertNodeRec(TreeBank::GetRoot(nd), 0);
        }
        break;
    }
    case Foldable:{
        std::function<void (Node*, int)> convertNodeRec;

        convertNodeRec = [&](Node *nd, int nest){
            convertOneNode(nd, nest);
            if(!nd->GetFolded()){
                foreach(Node *n, nd->GetChildren()){
                    convertNodeRec(n, nest + 1);
                }
            }
        };

        if(IsDisplayingViewNode()){
            foreach(Node *nd, nd->GetSiblings()){
                convertNodeRec(nd, 0);
            }
        } else if(IsDisplayingHistNode()){
            convertNodeRec(TreeBank::GetRoot(nd), 0);
        }
        break;
    }
    default: return;
    }

    bool hasSuspended = StatusBarMessageIsSuspended();
    if(!hasSuspended) SuspendStatusBarMessage();

    // pardon 'm_HoveredItemIndex == -1'
    SetHoveredItem(m_HoveredItemIndex);

    // 'm_CurrentThumbnailColumnCount' is 0,
    // when first call of here.

    qSort(m_DisplayThumbnails.begin(), m_DisplayThumbnails.end(),
          [this](Thumbnail *t1, Thumbnail *t2) -> bool {
              return m_SortPredicate(t1->GetNode(), t2->GetNode());});

    qSort(m_DisplayNodeTitles.begin(), m_DisplayNodeTitles.end(),
          [this](NodeTitle *t1, NodeTitle *t2) -> bool {
              return m_SortPredicate(t1->GetNode(), t2->GetNode());});

    int loadcount = 0;

    for(int i = 0; i < m_DisplayThumbnails.length(); i++){
        if(m_EnableLoadedSpotLight &&
           m_DisplayThumbnails[i]->GetNode()->GetView()){

            if(loadcount >= m_LoadedSpotLights.length()){
                m_LoadedSpotLights << new SpotLight(LoadedSpotLight, this);
            }
            SpotLight *light = m_LoadedSpotLights[loadcount];
            light->SetIndex(i);
            if(!light->isEnabled()) light->setEnabled(true);
            if(!light->isVisible()) light->setVisible(true);

            loadcount++;
        }

        switch (type){
        case Flat:
            if(m_DisplayThumbnails[i]->GetNode()->IsPrimaryOfParent())
                m_PrimaryItemIndex = i;
            break;
        case Straight:
            if(m_DisplayThumbnails[i]->GetNode() == nd)
                m_PrimaryItemIndex = i;
            break;
        case Recursive:
            if(m_DisplayThumbnails[i]->GetNode() == nd)
                m_PrimaryItemIndex = i;
            break;
        case Foldable:
            if(m_DisplayThumbnails[i]->GetNode() == nd)
                m_PrimaryItemIndex = i;
            break;
        }
    }

    while(m_LoadedSpotLights.length() > loadcount){
        delete m_LoadedSpotLights.takeLast();
    }

    if(m_PrimarySpotLight){
        m_PrimarySpotLight->setEnabled(true);
        m_PrimarySpotLight->setVisible(true);
    }
    if(m_HoveredSpotLight){
        m_HoveredSpotLight->setEnabled(true);
        m_HoveredSpotLight->setVisible(true);
    }

    m_ScrollController->setEnabled(true);
    m_ScrollController->setVisible(true);

    if(IsDisplayingViewNode() && nd && nd->GetParent() && nd->GetParent()->GetParent()){
        m_UpDirectoryButton->SetHovered(false);
        m_UpDirectoryButton->setEnabled(true);
        m_UpDirectoryButton->setVisible(true);
    } else {
        m_UpDirectoryButton->setEnabled(false);
        m_UpDirectoryButton->setVisible(false);
    }

    RelocateContents();
    RelocateScrollBar();

    if(m_HoveredItemIndex == -1){
        // when activating gadgets.
        SetHoveredItem(m_PrimaryItemIndex);
        SetScrollSoft(m_PrimaryItemIndex);
        // caller : 'Activate'.
    } else if(m_CurrentNode != nd){
        // when changing directory.
        SetScrollSoft(m_CurrentOffsetValue);
        // caller : 'UpDirectory', 'DownDirectory', 'ToggleTrash'.
    } else {
        // when manipulating node.
        // do nothing.
        // caller : 'Refresh', 'DeleteNode', 'PasteNode', 'RestoreNode',
        // 'MakeDirectory', 'RenameNode', 'ApplyChildrenOrder'.
    }
    m_CurrentNode = nd;

    if(!hasSuspended) ResumeStatusBarMessage();

    if(!StatusBarMessageIsSuspended())
        emit statusBarMessage(tr("Displaying %1 nodes.").arg(m_DisplayThumbnails.length()));
}

bool GraphicsTableView::DeleteNodes(NodeList list){

    if(m_DisplayThumbnails.isEmpty()) return false;

    Node::NodeType type;
    Node *parent = m_CurrentNode->GetParent();
    int scroll = m_CurrentOffsetValue;
    bool success = false;

    type = m_DisplayThumbnails.first()->GetNode()->GetType();

#ifdef USE_LIGHTNODE
    foreach(Thumbnail *thumb, m_DisplayThumbnails){
        thumb->setEnabled(false);
        thumb->setVisible(false);
    }
    foreach(NodeTitle *title, m_DisplayNodeTitles){
        title->setEnabled(false);
        title->setVisible(false);
    }
    m_DisplayThumbnails.clear();
    m_DisplayNodeTitles.clear();

    Node *backup = m_CurrentNode;
    m_CurrentNode = 0;

    success = m_TreeBank->DeleteNode(list);

    if(!success) m_CurrentNode = backup;
#else
    success = m_TreeBank->DeleteNode(list);
#endif

    // no deletion, needless to refresh.
    if(!success) return false;

    // when CollectNodes search root,
    // it's enough in 'SetCurrent(m_TreeBank->GetCurrentNode())'.
    if(type == Node::ViewTypeNode){
        if(GetNodeCollectionType() == Straight){
            SetCurrent(m_TreeBank->GetCurrentViewNode());
        } else {
            m_DummyViewNode->SetParent(parent);
            SetCurrent(m_DummyViewNode);
        }
    } else if(type == Node::HistTypeNode){
        if(GetNodeCollectionType() != Flat){
            SetCurrent(m_TreeBank->GetCurrentHistNode());
        } else {
            m_DummyHistNode->SetParent(parent);
            SetCurrent(m_DummyHistNode);
        }
    }

    SetScroll(scroll);
    if(scroll == m_CurrentOffsetValue){
        Update();
    }
    m_NodesRegister.clear();
    return true;
}

QMenu *GraphicsTableView::CreateSortMenu(){
    QMenu *menu = new QMenu(m_TreeBank);

    if(!m_ToggleSortReverseAction){
        m_ToggleSortReverseAction = new QAction(this);
        m_ToggleSortReverseAction->setText(tr("Reverse"));
        m_ToggleSortReverseAction->setCheckable(true);
        connect(m_ToggleSortReverseAction, SIGNAL(triggered()),
                this, SLOT(ToggleSortReverse()));
    }
    if(!m_ToggleSortByUrlAction){
        m_ToggleSortByUrlAction = new QAction(this);
        m_ToggleSortByUrlAction->setText(tr("SortByUrl"));
        m_ToggleSortByUrlAction->setCheckable(true);
        connect(m_ToggleSortByUrlAction, SIGNAL(triggered()),
                this, SLOT(ToggleSortByUrl()));
    }
    if(!m_ToggleSortByTitleAction){
        m_ToggleSortByTitleAction = new QAction(this);
        m_ToggleSortByTitleAction->setText(tr("SortByTitle"));
        m_ToggleSortByTitleAction->setCheckable(true);
        connect(m_ToggleSortByTitleAction, SIGNAL(triggered()),
                this, SLOT(ToggleSortByTitle()));
    }
    if(!m_ToggleSortByCreateDateAction){
        m_ToggleSortByCreateDateAction = new QAction(this);
        m_ToggleSortByCreateDateAction->setText(tr("SortByCreateDate"));
        m_ToggleSortByCreateDateAction->setCheckable(true);
        connect(m_ToggleSortByCreateDateAction, SIGNAL(triggered()),
                this, SLOT(ToggleSortByCreateDate()));
    }
    if(!m_ToggleSortByLastUpdateDateAction){
        m_ToggleSortByLastUpdateDateAction = new QAction(this);
        m_ToggleSortByLastUpdateDateAction->setText(tr("SortByLastUpdateDate"));
        m_ToggleSortByLastUpdateDateAction->setCheckable(true);
        connect(m_ToggleSortByLastUpdateDateAction, SIGNAL(triggered()),
                this, SLOT(ToggleSortByLastUpdateDate()));
    }
    if(!m_ToggleSortByLastAccessDateAction){
        m_ToggleSortByLastAccessDateAction = new QAction(this);
        m_ToggleSortByLastAccessDateAction->setText(tr("SortByLastAccessDate"));
        m_ToggleSortByLastAccessDateAction->setCheckable(true);
        connect(m_ToggleSortByLastAccessDateAction, SIGNAL(triggered()),
                this, SLOT(ToggleSortByLastAccessDate()));
    }

    m_ToggleSortReverseAction->setChecked(m_SortFlags & Reverse);
    m_ToggleSortByUrlAction->setChecked(m_SortFlags & ByUrl);
    m_ToggleSortByTitleAction->setChecked(m_SortFlags & ByTitle);
    m_ToggleSortByCreateDateAction->setChecked(m_SortFlags & ByCreateDate);
    m_ToggleSortByLastUpdateDateAction->setChecked(m_SortFlags & ByLastUpdateDate);
    m_ToggleSortByLastAccessDateAction->setChecked(m_SortFlags & ByLastAccessDate);

    menu->addAction(m_ToggleSortReverseAction);
    menu->addAction(m_ToggleSortByUrlAction);
    menu->addAction(m_ToggleSortByTitleAction);
    menu->addAction(m_ToggleSortByCreateDateAction);
    menu->addAction(m_ToggleSortByLastUpdateDateAction);
    menu->addAction(m_ToggleSortByLastAccessDateAction);
    return menu;
}

void GraphicsTableView::SetSortPredicate(){
    if(m_SortFlags & ByUrl){
        if(m_SortFlags & Reverse){
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetUrl().toString() > n2->GetUrl().toString();};
        } else {
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetUrl().toString() < n2->GetUrl().toString();};
        }
    } else if(m_SortFlags & ByTitle){
        if(m_SortFlags & Reverse){
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetTitle() > n2->GetTitle();};
        } else {
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetTitle() < n2->GetTitle();};
        }
    } else if(m_SortFlags & ByCreateDate){
        if(m_SortFlags & Reverse){
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetCreateDate() < n2->GetCreateDate();};
        } else {
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetCreateDate() > n2->GetCreateDate();};
        }
    } else if(m_SortFlags & ByLastUpdateDate){
        if(m_SortFlags & Reverse){
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetLastUpdateDate() < n2->GetLastUpdateDate();};
        } else {
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetLastUpdateDate() > n2->GetLastUpdateDate();};
        }
    } else if(m_SortFlags & ByLastAccessDate){
        if(m_SortFlags & Reverse){
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetLastAccessDate() < n2->GetLastAccessDate();};
        } else {
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                return n1->GetLastAccessDate() > n2->GetLastAccessDate();};
        }
    } else {
        if(m_SortFlags & Reverse){
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                if(n1->GetParent() == n2->GetParent())
                    return n1->SiblingsIndexOf(n1) > n2->SiblingsIndexOf(n2);
                if(n1->IsAncestorOf(n2)) return false;
                if(n2->IsAncestorOf(n1)) return true;

                NodeList l1 = NodeList() << n1 << n1->GetAncestors();
                NodeList l2 = NodeList() << n2 << n2->GetAncestors();
                // tail is root.
                while(l1.last() == l2.last()){
                    l1.takeLast();
                    l2.takeLast();
                }
                n1 = l1.last();
                n2 = l2.last();

                return n1->SiblingsIndexOf(n1) > n2->SiblingsIndexOf(n2);
            };
        } else {
            m_SortPredicate = [](Node *n1, Node *n2) -> bool {
                if(n1->GetParent() == n2->GetParent())
                    return n1->SiblingsIndexOf(n1) < n2->SiblingsIndexOf(n2);
                if(n1->IsAncestorOf(n2)) return true;
                if(n2->IsAncestorOf(n1)) return false;

                NodeList l1 = NodeList() << n1 << n1->GetAncestors();
                NodeList l2 = NodeList() << n2 << n2->GetAncestors();
                // tail is root.
                while(l1.last() == l2.last()){
                    l1.takeLast();
                    l2.takeLast();
                }
                n1 = l1.last();
                n2 = l2.last();

                return n1->SiblingsIndexOf(n1) < n2->SiblingsIndexOf(n2);
            };
        }
    }
}

void GraphicsTableView::ApplySort(){
    SetSortPredicate();
    ThumbList_RefreshNoScroll();
}

void GraphicsTableView::ToggleSortReverse(){
    m_SortFlags ^= Reverse;
    ApplySort();
}

void GraphicsTableView::ToggleSortByUrl(){
    m_SortFlags ^= ByUrl;
    m_SortFlags &= (ByUrl | Reverse);
    ApplySort();
}

void GraphicsTableView::ToggleSortByTitle(){
    m_SortFlags ^= ByTitle;
    m_SortFlags &= (ByTitle | Reverse);
    ApplySort();
}

void GraphicsTableView::ToggleSortByCreateDate(){
    m_SortFlags ^= ByCreateDate;
    m_SortFlags &= (ByCreateDate | Reverse);
    ApplySort();
}

void GraphicsTableView::ToggleSortByLastUpdateDate(){
    m_SortFlags ^= ByLastUpdateDate;
    m_SortFlags &= (ByLastUpdateDate | Reverse);
    ApplySort();
}

void GraphicsTableView::ToggleSortByLastAccessDate(){
    m_SortFlags ^= ByLastAccessDate;
    m_SortFlags &= (ByLastAccessDate | Reverse);
    ApplySort();
}

QList<SpotLight*> GraphicsTableView::GetLoadedSpotLights(){
    return m_LoadedSpotLights;
}

QString GraphicsTableView::GetDirectoryPrefix(Node *nd){
    if(!nd->IsDirectory()) return QString();

    if(IsDisplayingNode()){
        if(GetNodeCollectionType() == Foldable){
            return nd->GetFolded() ? QStringLiteral("+") : QStringLiteral("-");
        }
    }
    return QString();
}

SpotLight *GraphicsTableView::GetPrimarySpotLight(){
    return m_PrimarySpotLight;
}

SpotLight *GraphicsTableView::GetHoveredSpotLight(){
    return m_HoveredSpotLight;
}

void GraphicsTableView::SetHoveredItem(int index){
    const int len = m_DisplayThumbnails.length();

    if(index >= len) index = len - 1;
    if(index < -1)   index = -1;

    if(m_HoveredItemIndex >= len) m_HoveredItemIndex = len - 1;
    if(m_HoveredItemIndex < -1)   m_HoveredItemIndex = -1;

    if(m_HoveredItemIndex == index) return;

    QList<QRectF> list;
    QList<QGraphicsItem*> glist;

    if(m_HoveredItemIndex != -1){
        if(m_HoveredSpotLight) list << m_HoveredSpotLight->boundingRect();
        if(GetStyle()->UseGraphicsItemUpdate()){
            glist << GetHoveredThumbnail();
            glist << GetHoveredNodeTitle();
        } else {
            list << GetHoveredThumbnail()->boundingRect();
            list << GetHoveredNodeTitle()->boundingRect();
        }
    }

    m_HoveredItemIndex = index;

    if(m_HoveredItemIndex != -1){
        if(m_HoveredSpotLight) list << m_HoveredSpotLight->boundingRect();
        if(GetStyle()->UseGraphicsItemUpdate()){
            glist << GetHoveredThumbnail();
            glist << GetHoveredNodeTitle();
        } else {
            list << GetHoveredThumbnail()->boundingRect();
            list << GetHoveredNodeTitle()->boundingRect();
        }
    }

    foreach(QRectF rect, list){
        Update(rect);
    }
    foreach(QGraphicsItem *item, glist){
        item->update();
    }

    if(Node *nd = GetHoveredNode()){
        QString title = nd->GetTitle();
        QString url = nd->GetUrl().toString();

        if(!StatusBarMessageIsSuspended()){

            if(nd->IsDirectory())
                emit ItemHovered(QStringLiteral("Directory"), QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first(), QString());
            else
                emit ItemHovered(url, title, QString());
        }
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void GraphicsTableView::SetHoveredItem(Thumbnail *thumb){
    SetHoveredItem(m_DisplayThumbnails.indexOf(thumb));
}

void GraphicsTableView::SetHoveredItem(NodeTitle *title){
    SetHoveredItem(m_DisplayNodeTitles.indexOf(title));
}

void GraphicsTableView::SetPrimaryItem(int index){
    const int len = m_DisplayThumbnails.length();

    if(index >= len) index = len - 1;
    if(index < -1)   index = -1;

    if(m_PrimaryItemIndex >= len) m_PrimaryItemIndex = len - 1;
    if(m_PrimaryItemIndex < -1)   m_PrimaryItemIndex = -1;

    if(m_PrimaryItemIndex != index) return;

    QList<QRectF> list;

    if(m_PrimaryItemIndex != -1){
        if(m_PrimarySpotLight) list << m_PrimarySpotLight->boundingRect();
        list << GetPrimaryThumbnail()->boundingRect();
        list << GetPrimaryNodeTitle()->boundingRect();
    }

    m_PrimaryItemIndex = index;

    if(m_PrimaryItemIndex != -1){
        if(m_PrimarySpotLight) list << m_PrimarySpotLight->boundingRect();
        list << GetPrimaryThumbnail()->boundingRect();
        list << GetPrimaryNodeTitle()->boundingRect();
    }

    foreach(QRectF rect, list){
        Update(rect);
    }
}

void GraphicsTableView::SetPrimaryItem(Thumbnail *thumb){
    SetPrimaryItem(m_DisplayThumbnails.indexOf(thumb));
}

void GraphicsTableView::SetPrimaryItem(NodeTitle *title){
    SetPrimaryItem(m_DisplayNodeTitles.indexOf(title));
}

void GraphicsTableView::SetInPlaceNotifierContent(Node *nd){
    if(!m_InPlaceNotifier) return;
    m_InPlaceNotifier->SetNode(nd);
    if(nd){
        m_InPlaceNotifier->setVisible(true);
        m_InPlaceNotifier->setEnabled(true);
    } else {
        m_InPlaceNotifier->setVisible(false);
        m_InPlaceNotifier->setEnabled(false);
    }
}

void GraphicsTableView::SetInPlaceNotifierPosition(QPointF after){
    if(!m_InPlaceNotifier) return;

    QRectF nrect = m_InPlaceNotifier->rect();
    QRectF rect = boundingRect();

    if(after.x() > ScrollBarAreaRect().center().x()){

        after = after - QPointF(nrect.size().width(), 0);

        if(rect.size().width() > nrect.size().width()){
            if(after.x() < rect.left())
                after.setX(rect.left());
        } else {
            after.setX(rect.left());
        }
    } else {

        if(rect.size().width() > nrect.size().width()){
            if(after.x() > rect.right() - nrect.size().width())
                after.setX(rect.right() - nrect.size().width());
        } else {
            after.setX(rect.left());
        }
    }
    if(rect.size().height() > nrect.size().height()){
        if(after.y() > rect.bottom() - nrect.size().height())
            after.setY(rect.bottom() - nrect.size().height());
    } else {
        after.setY(rect.top());
    }

    QPointF before = m_InPlaceNotifier->pos();
    // for case of same position and different node.
    //if(before == after) return;
    m_InPlaceNotifier->setPos(after);

    Update(QRectF(before, m_InPlaceNotifier->rect().size()));
}

void GraphicsTableView::UpdateInPlaceNotifier(QPointF pos, QPointF scenePos, bool ignoreStatusBarMessage){
    bool hasSuspended = StatusBarMessageIsSuspended();

    if(ignoreStatusBarMessage && !hasSuspended) SuspendStatusBarMessage();

    if(ThumbnailAreaRect().contains(pos)){
        // to avoid InPlaceNotifier.
        SetInPlaceNotifierContent(0);
        if(QGraphicsItem *item = scene()->itemAt(pos, transform())){
            if(Thumbnail* thumb = dynamic_cast<Thumbnail*>(item)){
                SetHoveredItem(thumb);
                SetInPlaceNotifierContent(thumb->GetNode());
                SetInPlaceNotifierPosition(scenePos);
                if(ignoreStatusBarMessage && !hasSuspended) ResumeStatusBarMessage();
                return;
            }
        }
    } else if(NodeTitleAreaRect().contains(pos)){
        // to avoid InPlaceNotifier.
        SetInPlaceNotifierContent(0);
        if(QGraphicsItem *item = scene()->itemAt(pos, transform())){
            if(NodeTitle* title = dynamic_cast<NodeTitle*>(item)){
                SetHoveredItem(title);
                SetInPlaceNotifierContent(title->GetNode());
                SetInPlaceNotifierPosition(scenePos);
                if(ignoreStatusBarMessage && !hasSuspended) ResumeStatusBarMessage();
                return;
            }
        }
    }
    SetHoveredItem(-1);
    SetInPlaceNotifierContent(0);
    if(ignoreStatusBarMessage && !hasSuspended) ResumeStatusBarMessage();
}

void GraphicsTableView::ClearThumbnailSelection(){
    if(scene()->selectedItems().length() < m_DisplayThumbnails.length()){
        foreach(QGraphicsItem *item, scene()->selectedItems()){
            if(Thumbnail *thumb = dynamic_cast<Thumbnail*>(item)){
                if(thumb->isSelected()){
                    RemoveFromSelection(thumb->GetNode());
                    thumb->setSelected(false);
                }
            }
        }
    } else {
        foreach(Thumbnail *thumb, m_DisplayThumbnails){
            if(thumb->isSelected()){
                RemoveFromSelection(thumb->GetNode());
                thumb->setSelected(false);
            }
        }
    }
}

void GraphicsTableView::ClearNodeTitleSelection(){
    if(scene()->selectedItems().length() < m_DisplayNodeTitles.length()){
        foreach(QGraphicsItem *item, scene()->selectedItems()){
            if(NodeTitle *title = dynamic_cast<NodeTitle*>(item)){
                if(title->isSelected()){
                    RemoveFromSelection(title->GetNode());
                    title->setSelected(false);
                }
            }
        }
    } else {
        foreach(NodeTitle *title, m_DisplayNodeTitles){
            if(title->isSelected()){
                RemoveFromSelection(title->GetNode());
                title->setSelected(false);
            }
        }
    }
}

void GraphicsTableView::ClearScrollControllerSelection(){
    m_ScrollController->setSelected(false);
}

void GraphicsTableView::ResizeNotify(QSize size){
    Resize(QSizeF(size));
}

void GraphicsTableView::Resize(QSizeF size){
    scene()->setSceneRect(0.0, 0.0, size.width(), size.height());
    m_Size = size;
    if(IsDisplayingNode()){
        RelocateContents();
        RelocateScrollBar();
    }
}

QSizeF GraphicsTableView::Size(){
    return m_Size;
}

void GraphicsTableView::RelocateContents(){

    GetStyle()->ComputeContentsLayout
        (this,
         m_CurrentThumbnailColumnCount,
         m_CurrentThumbnailLineCount,
         m_CurrentThumbnailWidth,
         m_CurrentThumbnailHeight);

    int length = m_DisplayThumbnails.length();
    for(int i = 0; i < length; i++){
        Thumbnail *thumb = m_DisplayThumbnails[i];
        NodeTitle *title = m_DisplayNodeTitles[i];

        thumb->setParentItem(this);
        thumb->setVisible(true);
        thumb->setEnabled(true);
        if(!thumb->isSelected() || thumb->pos() == QPointF(0,0)){
            thumb->setPos(0,0);
            thumb->setRect
                (QRectF(QPointF(DISPLAY_PADDING_X + (i % m_CurrentThumbnailColumnCount) * m_CurrentThumbnailWidth,
                                DISPLAY_PADDING_Y + (i / m_CurrentThumbnailColumnCount) * m_CurrentThumbnailHeight)
                        + CurrentThumbnailOffset(),
                        QSizeF(m_CurrentThumbnailWidth, m_CurrentThumbnailHeight)));
        }

        title->setParentItem(this);
        title->setEnabled(true);
        title->setVisible(true);
        if(!title->isSelected() || title->pos() == QPointF(0,0)){
            title->setPos(0,0);
            title->setRect
                (QRectF(QPointF(DISPLAY_PADDING_X
                                + m_CurrentThumbnailWidth * m_CurrentThumbnailColumnCount
                                + GADGETS_SCROLL_BAR_MARGIN * 2
                                + GADGETS_SCROLL_BAR_WIDTH,
                                DISPLAY_PADDING_Y + GetStyle()->NodeTitleHeight() * i)
                        + CurrentNodeTitleOffset(),

                        QSizeF(m_Size.width()
                               - (GetStyle()->NodeTitleDrawBorder() ?
                                  DISPLAY_PADDING_X * 2 :
                                  DISPLAY_PADDING_X)
                               - m_CurrentThumbnailWidth * m_CurrentThumbnailColumnCount
                               - GADGETS_SCROLL_BAR_MARGIN * 2
                               - GADGETS_SCROLL_BAR_WIDTH,
                               GetStyle()->NodeTitleHeight())));
        }
    }
}

void GraphicsTableView::RelocateScrollBar(){
    QRectF back = ScrollBarAreaRect();
    int width = back.size().width() - 5;
    int height = GADGETS_SCROLL_CONTROLER_HEIGHT;
    if(height > back.size().height() - 5)
        height = back.size().height() - 5;

    QRectF rect = QRectF(back.topLeft() + QPointF(3,3), QSizeF(width, height));

    m_ScrollController->setRect(rect);

    if(!m_ScrollController->isSelected()){

        if(int len = m_DisplayThumbnails.length()){

            float areaHeight = ScrollBarAreaRect().height();
            float controllerHeight = m_ScrollController->rect().size().height();

            m_ScrollController->setPos
                (0, (areaHeight - controllerHeight) * m_CurrentOffsetValue / len);

        } else {

            m_ScrollController->setPos(0, 0);
        }
    }
}

void GraphicsTableView::AppendToSelection(Node *nd){
    // allow duplicates.
    m_NodesRegister << nd;
}

void GraphicsTableView::RemoveFromSelection(Node *nd){
    // allow duplicates.
    if(m_NodesRegister.contains(nd))
        m_NodesRegister.removeOne(nd);
}

void GraphicsTableView::SetSelectedByIndex(int i){
    if(i < 0 || i >= m_DisplayThumbnails.length()) return;

    Q_ASSERT(m_DisplayThumbnails[i]->GetNode() ==
             m_DisplayNodeTitles[i]->GetNode());

    m_DisplayThumbnails[i]->setSelected(true);
    AppendToSelection(m_DisplayThumbnails[i]->GetNode());

    m_DisplayNodeTitles[i]->setSelected(true);
    AppendToSelection(m_DisplayNodeTitles[i]->GetNode());
}

void GraphicsTableView::SetSelectionRange(Node *from, Node *to){
    bool inRange = false;
    for(int i = 0; i < m_DisplayThumbnails.length(); i++){
        if(m_DisplayThumbnails[i]->GetNode() == from) inRange = true;
        if(inRange) SetSelectedByIndex(i);
        if(m_DisplayThumbnails[i]->GetNode() == to) break;
    }
}

void GraphicsTableView::SetSelectionRange(Thumbnail* thumb){

    bool selected = false;
    Node *pivot =
        m_NodesRegister.isEmpty() ? thumb->GetNode() : m_NodesRegister.first();

    ClearThumbnailSelection();
    ClearNodeTitleSelection();
    ClearScrollControllerSelection();

    AppendToSelection(pivot);
    AppendToSelection(pivot);

    for(int i = 0; i < m_DisplayThumbnails.length(); i++){

        if(m_DisplayThumbnails[i]->GetNode() == pivot){

            m_DisplayThumbnails[i]->setSelected(true);
            m_DisplayNodeTitles[i]->setSelected(true);
            if(m_DisplayThumbnails[i] == thumb) break;

        } else {

            if(m_DisplayThumbnails[i] == thumb || selected)
                SetSelectedByIndex(i);
            if(m_DisplayThumbnails[i] != thumb) continue;
        }
        if(selected) break;
        selected = true;
    }
}

void GraphicsTableView::SetSelectionRange(NodeTitle *title){

    bool selected = false;
    Node *pivot =
        m_NodesRegister.isEmpty() ? title->GetNode() : m_NodesRegister.first();

    ClearThumbnailSelection();
    ClearNodeTitleSelection();
    ClearScrollControllerSelection();

    AppendToSelection(pivot);
    AppendToSelection(pivot);

    for(int i = 0; i < m_DisplayNodeTitles.length(); i++){

        if(m_DisplayNodeTitles[i]->GetNode() == pivot){

            m_DisplayThumbnails[i]->setSelected(true);
            m_DisplayNodeTitles[i]->setSelected(true);
            if(m_DisplayNodeTitles[i] == title) break;

        } else {

            if(m_DisplayNodeTitles[i] == title || selected)
                SetSelectedByIndex(i);
            if(m_DisplayNodeTitles[i] != title) continue;
        }
        if(selected) break;
        selected = true;
    }
}

bool GraphicsTableView::MoveTo(bool basedOnScroll, std::function<int(int, int)> compute){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    int cur = basedOnScroll ? m_CurrentOffsetValue : m_HoveredItemIndex;
    int len = m_DisplayThumbnails.length();

    SetHoveredItem(compute(cur, len));
    ClearScrollControllerSelection();

    if(basedOnScroll)
        SetScroll(m_HoveredItemIndex);
    else
        SetScrollSoft(m_HoveredItemIndex);

    return true;
}

bool GraphicsTableView::SelectTo(std::function<void()> move){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(m_NodesRegister.isEmpty()) SetSelectedByIndex(m_HoveredItemIndex);
    move();
    SetSelectionRange(GetHoveredThumbnail());
    return true;
}

bool GraphicsTableView::TransferTo(bool toRight, bool basedOnScroll,
                                   std::function<int(Thumbnail*, int, int)> compute){
    if(!m_CurrentNode ||
       !IsDisplayingViewNode() ||
       GetNodeCollectionType() == Straight ||
       m_NodesRegister.isEmpty()){
        return false;
    }

    m_NodesRegister.clear();

    Thumbnail *thumb = basedOnScroll ? GetScrolledThumbnail() : GetHoveredThumbnail();

    if(!thumb) return false;

    Thumbnail *neighbor = 0;
    QList<Thumbnail*> &thumbs = m_DisplayThumbnails;
    QList<Thumbnail*> selected = QList<Thumbnail*>();
    foreach(QGraphicsItem *item, scene()->selectedItems()){
        if(Thumbnail *t = dynamic_cast<Thumbnail*>(item)){
            selected << t;
        } else if(NodeTitle *n = dynamic_cast<NodeTitle*>(item)){
            selected << thumbs[m_DisplayNodeTitles.indexOf(n)];
        }
    }

    if(toRight){
        for(int i = thumbs.indexOf(thumb); i < thumbs.length(); i++){
            if(!selected.contains(thumbs[i])){
                neighbor = thumbs[i];
                break;
            }
        }
    } else {
        for(int i = thumbs.indexOf(thumb); i >= 0; i--){
            if(!selected.contains(thumbs[i])){
                neighbor = thumbs[i];
                break;
            }
        }
    }

    foreach(Thumbnail *t, selected){
        if(thumbs.contains(t))
            thumbs.removeOne(t);
        else
            selected.removeOne(t);
    }

    qSort(selected.begin(), selected.end(),
          [this](Thumbnail *t1, Thumbnail *t2) -> bool {
              return m_SortPredicate(t1->GetNode(), t2->GetNode());});

    int cur = thumbs.indexOf(neighbor);
    int len = thumbs.length();

    int insertPoint = compute(neighbor, cur, len);

    Node *parent = 0;
    NodeList children1;
    NodeList children2;
    NodeList reg;
    NodeList list;
    foreach(Thumbnail *t, thumbs.mid(0, insertPoint)){
        children1 << t->GetNode();
    }
    foreach(Thumbnail *t, thumbs.mid(insertPoint)){
        children2 << t->GetNode();
    }
    foreach(Thumbnail *t, selected){
        reg << t->GetNode();
    }

    if(!children2.isEmpty())
        parent = children2.first()->GetParent();
    else if(!children1.isEmpty())
        parent = children1.last()->GetParent();

    if(GetNodeCollectionType() == Flat){

        if(!parent) parent = reg[0]->GetParent();
        list = children1 + reg + children2;

        m_SortFlags = NoSort; // reset sort flags.
        SetSortPredicate();

    } else if(parent && m_SortFlags == NoSort){

        foreach(Node *nd, children1){
            if(nd->GetParent() == parent) list << nd;
        }

        list << reg;

        foreach(Node *nd, children2){
            if(nd->GetParent() == parent) list << nd;
        }
    } else {
        return false;
    }

    bool success = m_TreeBank->SetChildrenOrder(parent, list);

    Node *from = reg.first();
    Node *to   = reg.last();

    bool hasSuspended = StatusBarMessageIsSuspended();
    if(!hasSuspended) SuspendStatusBarMessage();

    // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
    ThumbList_Refresh();

    if(!hasSuspended) ResumeStatusBarMessage();

    if(success){
        SetHoveredItem(insertPoint);
        ClearScrollControllerSelection();

        if(basedOnScroll)
            SetScroll(m_HoveredItemIndex);
        else
            SetScrollSoft(m_HoveredItemIndex);
    }

    SetSelectionRange(from, to);
    return true;
}

void GraphicsTableView::RenderBackground(QPainter *painter){
    GetStyle()->RenderBackground(this, painter);
}

QRectF GraphicsTableView::boundingRect() const{
    return QRectF(QPointF(), m_Size);
}

void GraphicsTableView::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);

    // set anti alias false by default.
    painter->setRenderHint(QPainter::Antialiasing, false);

    RenderBackground(painter);
    if(!IsDisplayingNode()) return;

    painter->save();

    if(GADGETS_SCROLL_BAR_DRAW_BORDER || GetStyle()->NodeTitleDrawBorder())
        painter->setRenderHint(QPainter::Antialiasing, false);

    if(GADGETS_SCROLL_BAR_DRAW_BORDER){
        // scroll bar area.
        const QColor white(255,255,255,255);
        const QPen pen(white);
        painter->setPen(pen);

        const QColor gray(127,127,127,170);
        const QBrush brush(gray);
        painter->setBrush(brush);

        painter->drawRect(ScrollBarAreaRect());
    }

    if(GetStyle()->NodeTitleDrawBorder()){
        // node title list area.
        const QColor white(255,255,255,255);
        const QPen pen(white);
        painter->setPen(pen);

        const QColor black(0,0,0,50);
        const QBrush brush(black);
        painter->setBrush(brush);

        const QRectF rect = NodeTitleAreaRect();

        if(rect.isValid()){
            painter->drawRect(rect);
        }
    }
    painter->restore();
}

QRectF GraphicsTableView::ThumbnailAreaRect(){
    return GetStyle()->ThumbnailAreaRect(this);
}

QRectF GraphicsTableView::NodeTitleAreaRect(){
    return GetStyle()->NodeTitleAreaRect(this);
}

QRectF GraphicsTableView::ScrollBarAreaRect(){
    return GetStyle()->ScrollBarAreaRect(this);
}

void GraphicsTableView::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsObject::dragEnterEvent(ev);
}

void GraphicsTableView::dropEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsObject::dropEvent(ev);
}

void GraphicsTableView::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsObject::dragMoveEvent(ev);
}

void GraphicsTableView::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsObject::dragLeaveEvent(ev);
}

void GraphicsTableView::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    switch(ev->button()){
    case Qt::LeftButton:
        if(m_SelectRect){
            GetStyle()->OnReshow(m_SelectRect);
            m_SelectRect->setRect(QRectF(ev->pos(), ev->pos()));
            m_SelectRect->show();
        } else {
            m_SelectRect = GetStyle()->CreateSelectRect(this, ev->pos());
        }
        m_SelectRect->setParentItem(this);
        m_SelectRect->setZValue(SELECT_RECT_LAYER);
        ev->setAccepted(true);
        return;
    case Qt::RightButton:
        ev->setAccepted(true);
        return;
    default:
        QGraphicsObject::mousePressEvent(ev);
        return;
    }
}

void GraphicsTableView::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        QRectF rect;
        QList<QGraphicsItem*> collidings;
        if(m_SelectRect){
            rect = m_SelectRect->rect().normalized();
            m_SelectRect->setRect(rect);
            collidings = scene()->collidingItems(m_SelectRect);
        }

        if(collidings.isEmpty() &&
           (Application::keyboardModifiers() & Qt::ControlModifier ||
            Application::keyboardModifiers() & Qt::ShiftModifier)){

            ThumbList_UpDirectory();

        } else if(Application::keyboardModifiers() & Qt::ControlModifier){
            foreach(QGraphicsItem *item, collidings){
                if(item->parentItem() != this) continue;
                if(Thumbnail *thumb = dynamic_cast<Thumbnail*>(item)){
                    thumb->setSelected(!thumb->isSelected());

                    if(thumb->isSelected()){
                        AppendToSelection(thumb->GetNode());
                    } else {
                        RemoveFromSelection(thumb->GetNode());
                    }
                } else if(NodeTitle *title = dynamic_cast<NodeTitle*>(item)){
                    title->setSelected(!title->isSelected());

                    if(title->isSelected()){
                        AppendToSelection(title->GetNode());
                    } else {
                        RemoveFromSelection(title->GetNode());
                    }
                }
            }
        } else {
            m_NodesRegister.clear();
            ClearThumbnailSelection();
            ClearNodeTitleSelection();
            ClearScrollControllerSelection();

            foreach(QGraphicsItem *item, collidings){
                if(item->parentItem() != this) continue;
                if(Thumbnail *thumb = dynamic_cast<Thumbnail*>(item)){
                    thumb->setSelected(true);
                    AppendToSelection(thumb->GetNode());
                } else if(NodeTitle *title = dynamic_cast<NodeTitle*>(item)){
                    title->setSelected(true);
                    AppendToSelection(title->GetNode());
                }
            }

            if(rect.isNull() && ScrollBarAreaRect().contains(ev->pos())){

                if(ev->pos().y() <
                   (m_ScrollController->rect().center().y() +
                    m_ScrollController->pos().y()))

                    ThumbList_ScrollUp();
                else
                    ThumbList_ScrollDown();
            }
        }
        if(m_SelectRect) m_SelectRect->hide();
        Update(rect);
        ev->setAccepted(true);

    } else if(ev->button() == Qt::RightButton){
        QMenu *menu = 0;

        if(Application::keyboardModifiers() & Qt::ControlModifier)
            menu = CreateSortMenu();
        else
            menu = CreateNodeMenu();

        if(!menu)
            menu = m_TreeBank->CreateGlobalContextMenu();

        if(menu){
            menu->exec(ev->screenPos());
            delete menu;
            ev->setAccepted(true);
        }

    } else if(ev->button() == Qt::MidButton){

        ev->setAccepted(ThumbList_PasteNode());
    }
}

void GraphicsTableView::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    if(m_SelectRect){
        QRectF rect1 = m_SelectRect->rect().normalized();

        m_SelectRect->setRect(QRectF(ev->pos(), ev->buttonDownPos(Qt::LeftButton)));

        QRectF rect2 = m_SelectRect->rect().normalized();

        if(rect1.contains(rect2)){
            update(rect1);
        } else if(rect2.contains(rect1)){
            update(rect2);
        } else {
            update(rect1);
            update(rect2);
        }

        //sometimes rubbish remains...
        //if(rect1.contains(rect2)){
        //    Update(rect1);
        //} else if(rect2.contains(rect1)){
        //    Update(rect2);
        //} else {
        //    Update(rect1);
        //    Update(rect2);
        //}

        ev->setAccepted(true);
    }
}

void GraphicsTableView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    if(!ScrollBarAreaRect().contains(ev->pos())){

        if(IsDisplayingHistNode() ||
           ScrollToChangeDirectory() ||
           !ThumbList_UpDirectory()){

            Deactivate();
        }
    }
    ev->setAccepted(true);
}

void GraphicsTableView::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    SetHoveredItem(-1);
    SetInPlaceNotifierContent(0);
    ev->setAccepted(true);
}

void GraphicsTableView::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    // disable for node menu.
    //SetHoveredItem(-1);
    ev->setAccepted(true);
}

void GraphicsTableView::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    SetHoveredItem(-1);
    SetInPlaceNotifierContent(0);
    ev->setAccepted(true);
}

void GraphicsTableView::contextMenuEvent(QGraphicsSceneContextMenuEvent *ev){
    /* when mouse pressed, do nothing. */
    ev->setAccepted(true);
}

void GraphicsTableView::wheelEvent(QGraphicsSceneWheelEvent *ev){
    if(!IsDisplayingNode()) return;

    bool up = ev->delta() > 0;
    bool ignoreStatusBarMessage = true;

    if(ScrollToChangeDirectory() &&
       ThumbnailAreaRect().contains(ev->pos())){

        // don't want to overwrite statusBarMessage in this event.
        // because these method updates statusBarMessage.
        if(up) ThumbList_UpDirectory();
        else   ThumbList_DownDirectory();

    } else {

        ignoreStatusBarMessage = false;
        if(up) ThumbList_ScrollUp();
        else   ThumbList_ScrollDown();
    }

    UpdateInPlaceNotifier(ev->pos(), ev->scenePos(), ignoreStatusBarMessage);
    ev->setAccepted(true);
}

void GraphicsTableView::focusInEvent(QFocusEvent *ev){
    QGraphicsObject::focusInEvent(ev);
}

void GraphicsTableView::focusOutEvent(QFocusEvent *ev){
    QGraphicsObject::focusOutEvent(ev);
}

bool GraphicsTableView::ThumbList_Refresh(){
    // some times this method sets scroll and hovered item to primary item.
    if(!IsDisplayingNode()) return false;

    // recollect nodes and reset scroll.
    int scroll = m_CurrentOffsetValue;
    SetCurrent(m_CurrentNode);
    if(scroll == m_CurrentOffsetValue){
        Update();
    }

    m_NodesRegister.clear();
    return true;
}

bool GraphicsTableView::ThumbList_RefreshNoScroll(){
    if(!IsDisplayingNode()) return false;

    // recollect nodes and reset scroll.
    int scroll = m_CurrentOffsetValue;
    bool hovered = m_HoveredItemIndex != -1;

    if(!hovered) m_HoveredItemIndex = scroll;

    SetCurrent(m_CurrentNode);

    if(!hovered) SetHoveredItem(-1);

    if(scroll == m_CurrentOffsetValue){
        Update();
    }

    m_NodesRegister.clear();
    return true;
}

bool GraphicsTableView::ThumbList_OpenNode(){
    if(!IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        if(GetNodeCollectionType() == Foldable && nd->IsDirectory()){

            nd->SetFolded(!nd->GetFolded());
            ThumbList_RefreshNoScroll();
            return true;
        }

        // return value is not accurate.
        if(!nd->IsDirectory() ||
           ScrollToChangeDirectory() ||
           !ThumbList_DownDirectory()){

            if(m_TreeBank->SetCurrent(nd)){
                // 'Deactivate' doesn't touch
                // 'm_DisplayThumbnails' and 'm_HoveredItemIndex'.
                // their initialization is 'CollectNodes'(near activation).
                Deactivate();
                return true;
            }
        }
    }
    return false;
}

bool GraphicsTableView::ThumbList_DeleteNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    // delete selected nodes or hovered node.

    SetInPlaceNotifierContent(0);

    if(!m_NodesRegister.isEmpty()){

        // delete duplicates.
        m_NodesRegister = m_NodesRegister.toSet().toList();
        return DeleteNodes(m_NodesRegister);

    } else if(Node *nd = GetHoveredNode()){

        return DeleteNodes(NodeList() << nd);
    }
    // no target to be deleted, needless to refresh.
    return false;
}

bool GraphicsTableView::ThumbList_DeleteRightNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    // delete right of hovered node in current directory.

    if(m_HoveredItemIndex != -1 &&
       m_HoveredItemIndex < m_DisplayThumbnails.length() - 1){

        NodeList list;
        for(int i = m_HoveredItemIndex + 1; i < m_DisplayThumbnails.length(); i++){
            list << m_DisplayThumbnails[i]->GetNode();
        }
        //SetInPlaceNotifierContent(0);
        return DeleteNodes(list);
    }
    // no target to be deleted, needless to refresh.
    return false;
}

bool GraphicsTableView::ThumbList_DeleteLeftNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    // delete left of hovered node in current directory.

    if(m_HoveredItemIndex > 0 &&
       m_HoveredItemIndex < m_DisplayThumbnails.length()){

        NodeList list;
        for(int i = 0; i < m_HoveredItemIndex; i++){
            list << m_DisplayThumbnails[i]->GetNode();
        }
        SetInPlaceNotifierContent(0);
        return DeleteNodes(list);
    }
    // no target to be deleted, needless to refresh.
    return false;
}

bool GraphicsTableView::ThumbList_DeleteOtherNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    // delete all nodes of current directory except selected nodes or hovered node.

    SetInPlaceNotifierContent(0);

    NodeList list;

    if(!m_NodesRegister.isEmpty()){
        m_NodesRegister = m_NodesRegister.toSet().toList();
        for(int i = 0; i < m_DisplayThumbnails.length(); i++){
            Node *nd = m_DisplayThumbnails[i]->GetNode();
            if(!m_NodesRegister.contains(nd)) list << nd;
        }
    } else if(Node *hovered = GetHoveredNode()){
        for(int i = 0; i < m_DisplayThumbnails.length(); i++){
            Node *nd = m_DisplayThumbnails[i]->GetNode();
            if(nd != hovered) list << nd;
        }
    }
    // no target to be deleted, needless to refresh.
    if(list.isEmpty()) return false;
    return DeleteNodes(list);
}

bool GraphicsTableView::ThumbList_PasteNode(){
    if(!m_CurrentNode || !IsDisplayingViewNode()) return false;

    Node *nd = GetHoveredNode() ? GetHoveredNode() : m_CurrentNode;
    Node *parent = nd->GetParent();
    NodeList sibling = nd->GetSiblings();
    NodeList neworder;

    // delete duplicates.
    m_NodesRegister = m_NodesRegister.toSet().toList();

    if(m_NodesRegister.isEmpty())
        // no nodes to paste, nothing to do.
        return false;

    foreach(Node *nd, nd->GetAncestors()){
        if(m_NodesRegister.contains(nd)){
            m_NodesRegister.removeOne(nd);
        }
    }

    foreach(Node *nd, m_NodesRegister){
        if(sibling.contains(nd)){
            sibling.removeOne(nd);
        }
    }

    qSort(m_NodesRegister.begin(), m_NodesRegister.end(),
          m_SortPredicate);

    int insertPoint = sibling.indexOf(nd);
    if(GetHoveredNode() && insertPoint != -1){

        neworder =
            sibling.mid(0, insertPoint+1) +
            m_NodesRegister +
            sibling.mid(insertPoint+1);

    } else {
        neworder = sibling + m_NodesRegister;
    }

    m_TreeBank->SetChildrenOrder(parent, neworder);

    // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
    ThumbList_RefreshNoScroll();

    if(m_HoveredItemIndex == -1){
        ThumbList_MoveToLastItem();
    }
    return true;
}

bool GraphicsTableView::ThumbList_RestoreNode(){
    if(!m_CurrentNode || !IsDisplayingViewNode()) return false;

    ViewNode *vn = GetHoveredViewNode();
    if(!vn && m_CurrentNode)
        vn = m_CurrentNode->ToViewNode();

    m_TreeBank->Restore(vn, vn ? vn->GetParent()->ToViewNode() : 0);

    ThumbList_RefreshNoScroll();

    if(m_HoveredItemIndex == -1){
        ThumbList_MoveToLastItem();
    }
    return true;
}

bool GraphicsTableView::ThumbList_NewNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(IsDisplayingViewNode()){
        ViewNode *vn = GetHoveredViewNode();
        if(!vn && m_CurrentNode)
            vn = m_CurrentNode->ToViewNode();
        m_TreeBank->NewViewNode(vn);
    } else if(IsDisplayingHistNode()){
        HistNode *hn = GetHoveredHistNode();
        if(!hn && m_CurrentNode)
            hn = m_CurrentNode->ToHistNode();
        m_TreeBank->NewHistNode(hn);
    }
    ThumbList_RefreshNoScroll();
    return true;
}

bool GraphicsTableView::ThumbList_CloneNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(IsDisplayingViewNode()){
        if(!m_NodesRegister.isEmpty()){
            m_NodesRegister = m_NodesRegister.toSet().toList();
            foreach(Node *nd, m_NodesRegister){
                ViewNode *vn = nd->ToViewNode();
                if(vn) m_TreeBank->CloneViewNode(vn);
            }
        } else if(ViewNode *vn = GetHoveredViewNode()){
            if(!vn && m_CurrentNode)
                vn = m_CurrentNode->ToViewNode();
            m_TreeBank->CloneViewNode(vn);
        }
    } else if(IsDisplayingHistNode()){
        if(!m_NodesRegister.isEmpty()){
            m_NodesRegister = m_NodesRegister.toSet().toList();
            foreach(Node *nd, m_NodesRegister){
                HistNode *hn = nd->ToHistNode();
                if(hn) m_TreeBank->CloneHistNode(hn);
            }
        } else if(HistNode *hn = GetHoveredHistNode()){
            if(!hn && m_CurrentNode)
                hn = m_CurrentNode->ToHistNode();
            m_TreeBank->CloneHistNode(hn);
        }
    }
    ThumbList_RefreshNoScroll();
    return true;
}

bool GraphicsTableView::ThumbList_UpDirectory(){
    if(!IsDisplayingNode() ||
       !m_CurrentNode ||
       !m_CurrentNode->GetParent() ||
       !m_CurrentNode->GetParent()->GetParent())
        return false;

    // reset scroll.
    // want to scroll to 'PrimaryItem'.
    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    int scroll = m_CurrentOffsetValue;
    SetCurrent(m_CurrentNode->GetParent());
    if(scroll == m_CurrentOffsetValue) Update();
    return true;
}

bool GraphicsTableView::ThumbList_DownDirectory(){
    if(!IsDisplayingNode() ||
       !m_CurrentNode ||
       m_HoveredItemIndex == -1 ||
       m_HoveredItemIndex >= m_DisplayThumbnails.length())
        return false;

    Node *nd = GetHoveredNode();
    if(!nd) return false;
    if(nd->IsViewNode() && !nd->IsDirectory()) return false;
    if(nd->IsHistNode() && nd->HasNoChildren()) return false;

    Node *target = 0;

    if(nd->HasNoChildren()){
        if(IsDisplayingViewNode()){
            m_DummyViewNode->SetParent(nd);
            target = m_DummyViewNode;
        } else if(IsDisplayingHistNode()){
            m_DummyHistNode->SetParent(nd);
            target = m_DummyHistNode;
        }
    } else if(nd->GetPrimary()){
        target = nd->GetPrimary();
    } else {
        target = nd->GetFirstChild();
    }

    // reset scroll.
    // want to scroll to 'PrimaryItem'.
    m_HoveredItemIndex = -1;
    m_PrimaryItemIndex = -1;

    int scroll = m_CurrentOffsetValue;
    SetCurrent(target);
    if(scroll == m_CurrentOffsetValue) Update();
    return true;
}

bool GraphicsTableView::ThumbList_MakeLocalNode(){
    if(!m_CurrentNode || !IsDisplayingViewNode()) return false;

    // 'Deactivate' doesn't touch
    // 'm_DisplayThumbnails' and 'm_HoveredItemIndex'.
    // their initialization is 'CollectNodes'(near activation).
    Deactivate();
    ViewNode *vn = GetHoveredViewNode();
    if(!vn && m_CurrentNode)
        vn = m_CurrentNode->ToViewNode();

    m_TreeBank->MakeLocalNode(vn);

    return true;
}

bool GraphicsTableView::ThumbList_MakeDirectory(){
    if(!m_CurrentNode || !IsDisplayingViewNode()) return false;

    if(ViewNode *vn = GetHoveredViewNode()){
        m_TreeBank->MakeSiblingDirectory(vn);
    } else {
        m_TreeBank->MakeChildDirectory(m_CurrentNode->GetParent()->ToViewNode());
    }

    ThumbList_RefreshNoScroll();

    if(m_HoveredItemIndex == -1){
        ThumbList_MoveToLastItem();
    }
    return true;
}

bool GraphicsTableView::ThumbList_MakeDirectoryWithSelectedNode(){
    if(!m_CurrentNode || !IsDisplayingViewNode()) return false;

    Node *nd = GetHoveredNode() ? GetHoveredNode() : m_CurrentNode;
    Node *parent = 0;

    m_NodesRegister = m_NodesRegister.toSet().toList();

    if(m_NodesRegister.isEmpty())
        // no nodes to move, nothing to do.
        return false;

    foreach(Node *nd, nd->GetAncestors()){
        if(m_NodesRegister.contains(nd)){
            m_NodesRegister.removeOne(nd);
        }
    }

    qSort(m_NodesRegister.begin(), m_NodesRegister.end(),
          m_SortPredicate);

    if(ViewNode *vn = GetHoveredViewNode()){
        parent = m_TreeBank->MakeSiblingDirectory(vn);
    } else {
        parent = m_TreeBank->MakeChildDirectory(m_CurrentNode->GetParent()->ToViewNode());
    }

    m_TreeBank->SetChildrenOrder(parent, m_NodesRegister);

    // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
    ThumbList_RefreshNoScroll();

    if(m_HoveredItemIndex == -1){
        ThumbList_MoveToLastItem();
    }
    return true;
}

bool GraphicsTableView::ThumbList_MakeDirectoryWithSameDomainNode(){
    if(!m_CurrentNode || !IsDisplayingViewNode()) return false;

    ViewNode *parent = m_CurrentNode->GetParent()->ToViewNode();
    QMap<QString, QList<Node*>> groups;

    foreach(Node *nd, m_CurrentNode->GetSiblings()){
        // QUrl("about:blank") and invalid url has empty host,
        // so empty title directory will be made.
        if(!nd->IsDirectory()) groups[nd->GetUrl().host()] << nd;
    }

    if(groups.count() <= 1){
        ModelessDialog::Information
            (tr("Invalid directory contents."),
             tr("This directory has no contents or all nodes have same domain."));
        return false;
    }

    foreach(QString domain, groups.keys()){
        Node *directory = parent->MakeChild();
        directory->SetTitle(domain);
        m_TreeBank->SetChildrenOrder(directory, groups[domain]);
    }

    m_CurrentNode = parent->GetFirstChild();

    // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
    ThumbList_RefreshNoScroll();

    if(m_HoveredItemIndex == -1){
        ThumbList_MoveToLastItem();
    }
    return true;
}

bool GraphicsTableView::ThumbList_RenameNode(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        if(nd->TitleEditable()){
            int hovered = m_HoveredItemIndex;

            if(m_TreeBank->RenameNode(nd)){
                // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
                // which is natural movement of
                // 'ThumbList_Refresh' and 'SetHoveredItem' and 'update'?

                //SetHoveredItem(hovered);
            }
            // or set always?
            SetHoveredItem(hovered);
            return true;
        }
    }
    m_NodesRegister.clear();
    return false;
}

bool GraphicsTableView::ThumbList_CopyNodeUrl(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        Application::clipboard()->setText(QString::fromUtf8(nd->GetUrl().toEncoded()));
        return true;
    }
    return false;
}

bool GraphicsTableView::ThumbList_CopyNodeTitle(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        Application::clipboard()->setText(nd->GetTitle());
        return true;
    }
    return false;
}

bool GraphicsTableView::ThumbList_CopyNodeAsLink(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(!m_NodesRegister.isEmpty()){

        // delete duplicates.
        m_NodesRegister = m_NodesRegister.toSet().toList();

        qSort(m_NodesRegister.begin(), m_NodesRegister.end(),
              m_SortPredicate);

        QString html;
        foreach(Node *nd, m_NodesRegister){
            QString title = nd->GetTitle();
            QString url = QString::fromUtf8(nd->GetUrl().toEncoded());
            html += QStringLiteral("<a href=\"%1\">%2</a>\n").arg(url, title);
        }
        Application::clipboard()->setText(html);
        return true;

    } else if(Node *nd = GetHoveredNode()){

        QString title = nd->GetTitle();
        QString url = QString::fromUtf8(nd->GetUrl().toEncoded());
        Application::clipboard()->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(url, title));
        return true;
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithIE(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_IE(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithFF(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_FF(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithOpera(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_Opera(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithOPR(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_OPR(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithSafari(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_Safari(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithChrome(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_Chrome(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithSleipnir(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_Sleipnir(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithVivaldi(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_Vivaldi(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_OpenNodeWithCustom(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(Node *nd = GetHoveredNode()){
        return Application::OpenUrlWith_Custom(nd->GetUrl());
    }
    return false;
}

bool GraphicsTableView::ThumbList_ToggleTrash(){
    m_NodesRegister.clear();
    switch (m_DisplayType){
    case HistTree: // fall through.
    case TrashTree:{
        if(TreeBank::GetViewRoot()->HasNoChildren()) return false;

        // reset scroll.
        // want to scroll to 'PrimaryItem'.
        m_HoveredItemIndex = -1;
        m_PrimaryItemIndex = -1;

        m_DisplayType = ViewTree;

        if(TreeBank::GetViewRoot()->GetPrimary()){
            SetCurrent(TreeBank::GetViewRoot()->GetPrimary());
        } else {
            SetCurrent(TreeBank::GetViewRoot()->GetFirstChild());
        }
        return true;
    }
    case ViewTree:{
        if(TreeBank::GetTrashRoot()->HasNoChildren()) return false;

        // reset scroll.
        // want to scroll to 'PrimaryItem'.
        m_HoveredItemIndex = -1;
        m_PrimaryItemIndex = -1;

        m_DisplayType = TrashTree;

        if(TreeBank::GetTrashRoot()->GetPrimary()){
            SetCurrent(TreeBank::GetTrashRoot()->GetPrimary());
        } else {
            SetCurrent(TreeBank::GetTrashRoot()->GetFirstChild());
        }
        return true;
    }
    case AccessKey: break;
    }
    return false;
}

bool GraphicsTableView::ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos){
    if(!m_CurrentNode ||
       !IsDisplayingViewNode() ||
       GetNodeCollectionType() == Straight){
        ThumbList_RefreshNoScroll();
        return false;
    }

    // delete duplicates.
    m_NodesRegister = m_NodesRegister.toSet().toList();

    bool success = false;
    bool notOnlyOneNode = m_NodesRegister.length() > 1;
    m_NodesRegister.clear();

    NodeList list;
    switch (area){
    case ThumbnailArea:{
        std::function<bool(Thumbnail*, Thumbnail*)> lessThan =
            [this, basepos, notOnlyOneNode]
            (Thumbnail *t1, Thumbnail *t2) -> bool {
            QPointF p1 = t1->boundingRect().center() + t1->pos();
            QPointF p2 = t2->boundingRect().center() + t2->pos();

            if((t1->isSelected() != t2->isSelected()) && notOnlyOneNode){
                if(t1->isSelected()) p1 = basepos;
                if(t2->isSelected()) p2 = basepos;
            }

            // decide order with using rough position(thumbnail center) on grid.
            if(ceil((p1.y() - DISPLAY_PADDING_Y) / m_CurrentThumbnailHeight) ==
               ceil((p2.y() - DISPLAY_PADDING_Y) / m_CurrentThumbnailHeight)){
                return p1.x() < p2.x();
            } else {
                return p1.y() < p2.y();
            }
        };

        qSort(m_DisplayThumbnails.begin(),
              m_DisplayThumbnails.end(), lessThan);

        if(GetNodeCollectionType() == Flat){

            foreach(Thumbnail *thumb, m_DisplayThumbnails){
                list << thumb->GetNode();
            }
            success = m_TreeBank->SetChildrenOrder(m_CurrentNode->GetParent(), list);

            m_SortFlags = NoSort; // reset sort flags.
            SetSortPredicate();

        } else if(m_SortFlags == NoSort){

            Thumbnail *thumb = 0;
            foreach(QGraphicsItem *item, scene()->selectedItems()){
                if(thumb = dynamic_cast<Thumbnail*>(item)){
                    break;
                }
            }
            if(!thumb) break;

            int index = m_DisplayThumbnails.indexOf(thumb);
            while(index != m_DisplayThumbnails.length() &&
                  m_DisplayThumbnails[index]->isSelected()){
                index++;
            }
            if(index == m_DisplayThumbnails.length()){
                index = m_DisplayThumbnails.indexOf(thumb);
                while(index != -1 &&
                      m_DisplayThumbnails[index]->isSelected()){
                    index--;
                }
            }
            if(index == -1) break;

            Node *parent = m_DisplayThumbnails[index]->GetNode()->GetParent();

            foreach(Thumbnail *thumb, m_DisplayThumbnails){
                if(thumb->GetNode()->GetParent() == parent || thumb->isSelected())
                    list << thumb->GetNode();
            }
            success = m_TreeBank->SetChildrenOrder(parent, list);
        }
        break;
    }
    case NodeTitleArea:{
        std::function<bool(NodeTitle*, NodeTitle*)> lessThan =
            [this, basepos, notOnlyOneNode]
            (NodeTitle *n1, NodeTitle *n2) -> bool {
            QPointF p1 = n1->boundingRect().center() + n1->pos();
            QPointF p2 = n2->boundingRect().center() + n2->pos();

            if((n1->isSelected() != n2->isSelected()) && notOnlyOneNode){
                if(n1->isSelected()) p1 = basepos;
                if(n2->isSelected()) p2 = basepos;
            }
            return p1.y() < p2.y();
        };

        qSort(m_DisplayNodeTitles.begin(),
              m_DisplayNodeTitles.end(), lessThan);

        if(GetNodeCollectionType() == Flat){

            foreach(NodeTitle *title, m_DisplayNodeTitles){
                list << title->GetNode();
            }
            success = m_TreeBank->SetChildrenOrder(m_CurrentNode->GetParent(), list);

            m_SortFlags = NoSort; // reset sort flags.
            SetSortPredicate();

        } else if(m_SortFlags == NoSort){

            NodeTitle *title = 0;
            foreach(QGraphicsItem *item, scene()->selectedItems()){
                if(title = dynamic_cast<NodeTitle*>(item)){
                    break;
                }
            }
            if(!title) break;

            int index = m_DisplayNodeTitles.indexOf(title);
            while(index != m_DisplayNodeTitles.length() &&
                  m_DisplayNodeTitles[index]->isSelected()){
                index++;
            }
            if(index == m_DisplayNodeTitles.length()){
                index = m_DisplayNodeTitles.indexOf(title);
                while(index != -1 &&
                      m_DisplayNodeTitles[index]->isSelected()){
                    index--;
                }
            }
            if(index == -1) break;

            Node *parent = m_DisplayNodeTitles[index]->GetNode()->GetParent();

            foreach(NodeTitle *title, m_DisplayNodeTitles){
                if(title->GetNode()->GetParent() == parent || title->isSelected())
                    list << title->GetNode();
            }
            success = m_TreeBank->SetChildrenOrder(parent, list);
        }
        break;
    }
    }

    ThumbList_RefreshNoScroll();
    return success;
}

void GraphicsTableView::SetScroll(QPointF pos){
    ClearScrollControllerSelection();
    SetScroll(static_cast<int>(m_DisplayThumbnails.length() * pos.y()));
}

void GraphicsTableView::SetScroll(int offset){
    if(!isVisible()) return;

    const int len = m_DisplayThumbnails.length();
    const int before = m_CurrentOffsetValue;
    const int after =
        len    ==   0 ?       0 :
        offset <    0 ?       0 :
        offset >= len ? len - 1 :
        offset;

    if(after != before){
        m_CurrentOffsetValue = after;
        RelocateContents();
        RelocateScrollBar();
        Update();
        emit ScrollChanged(QPointF(0.5,
                                   static_cast<double>(after) /
                                   static_cast<double>(len)));
    }
}

void GraphicsTableView::SetScrollSoft(QPointF pos){
    ClearScrollControllerSelection();
    SetScrollSoft(static_cast<int>(m_DisplayThumbnails.length() * pos.y()));
}

void GraphicsTableView::SetScrollSoft(int offset){
    if(offset > m_CurrentOffsetValue){

        int thumbnailscroll = offset -
            m_CurrentThumbnailColumnCount *
            (m_CurrentThumbnailLineCount - 1);
        if(thumbnailscroll < 0) thumbnailscroll = 0;

        int nodetitlescroll = offset + m_CurrentThumbnailColumnCount -
            (static_cast<int>(m_Size.height()) - DISPLAY_PADDING_Y) /
            GetStyle()->NodeTitleHeight();
        if(nodetitlescroll < 0) nodetitlescroll = 0;

        int thumbnailcount =
            m_CurrentThumbnailColumnCount *
            m_CurrentThumbnailLineCount;

        int nodetitlecount =
            (static_cast<int>(m_Size.height()) - DISPLAY_PADDING_Y) /
            GetStyle()->NodeTitleHeight();

        SetScroll(qMax(m_CurrentOffsetValue,
                       thumbnailcount  < nodetitlecount ?
                       thumbnailscroll : nodetitlescroll));
    } else {
        SetScroll(offset);
    }
}

bool GraphicsTableView::ThumbList_ScrollUp(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;
    ClearScrollControllerSelection();
    // not change hovered item.
    SetScroll(m_CurrentOffsetValue - m_CurrentThumbnailColumnCount);
    return true;
}

bool GraphicsTableView::ThumbList_ScrollDown(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;
    ClearScrollControllerSelection();
    // not change hovered item.
    SetScroll(m_CurrentOffsetValue + m_CurrentThumbnailColumnCount);
    return true;
}

bool GraphicsTableView::ThumbList_PageUp(){
    return ThumbList_MoveToPrevPage();
}

bool GraphicsTableView::ThumbList_PageDown(){
    return ThumbList_MoveToNextPage();
}

bool GraphicsTableView::ThumbList_ZoomIn(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;
    static const float eps = 0.01f;
    int len   = View::GetZoomFactorLevels().length();
    int level = View::GetZoomFactorLevels().indexOf(m_CurrentThumbnailZoomFactor);
    float zoom;
    if(level == -1){
        for(int i = 0; i < len; i++){
            zoom = View::GetZoomFactorLevels()[i];
            if((zoom - m_CurrentThumbnailZoomFactor) > eps) break;
        }
    } else if(level < len - 1){
        zoom = View::GetZoomFactorLevels()[level + 1];
    } else {
        zoom = View::GetZoomFactorLevels()[level];
    }
    m_CurrentThumbnailZoomFactor = zoom;
    RelocateContents();
    RelocateScrollBar();
    if(!StatusBarMessageIsSuspended())
        emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
    Update();
    return true;
}

bool GraphicsTableView::ThumbList_ZoomOut(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;
    static const float eps = 0.01f;
    int len   = View::GetZoomFactorLevels().length();
    int level = View::GetZoomFactorLevels().indexOf(m_CurrentThumbnailZoomFactor);
    float zoom;
    if(level == -1){
        for(int i = len - 1; i >= 0; i--){
            zoom = View::GetZoomFactorLevels()[i];
            if((m_CurrentThumbnailZoomFactor - zoom) > eps) break;
        }
    } else if(level > 0){
        zoom = View::GetZoomFactorLevels()[level - 1];
    } else {
        zoom = View::GetZoomFactorLevels()[level];
    }
    m_CurrentThumbnailZoomFactor = zoom;
    RelocateContents();
    RelocateScrollBar();
    if(!StatusBarMessageIsSuspended())
        emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
    Update();
    return true;
}

bool GraphicsTableView::ThumbList_MoveToUpperItem(){
    return MoveTo(false, [this](int cur, int len){

            int col = m_CurrentThumbnailColumnCount;
            int index = cur - col;
            int base = (len / col * col) + cur;

            if     (cur == -1)  return len - 1;
            else if(index >= 0) return index;
            else if((len % col) <= cur) return base - col;
            else                return base;
        });
}

bool GraphicsTableView::ThumbList_MoveToLowerItem(){
    return MoveTo(false, [this](int cur, int len){

            int col = m_CurrentThumbnailColumnCount;
            int index = cur + col;

            if     (cur == -1)    return 0;
            else if(index >= len) return cur % col;
            else                  return index;
        });
}

bool GraphicsTableView::ThumbList_MoveToRightItem(){
    return MoveTo(false, [this](int cur, int len){

            int index = cur + 1;

            if(index >= len) return 0;
            else             return index;
        });
}

bool GraphicsTableView::ThumbList_MoveToLeftItem(){
    return MoveTo(false, [this](int cur, int len){

            int index = cur - 1;

            if(index < 0) return len - 1;
            else          return index;
        });
}

bool GraphicsTableView::ThumbList_MoveToPrevPage(){
    return MoveTo(true, [this](int cur, int len){
            Q_UNUSED(len);

            int col = m_CurrentThumbnailColumnCount;
            int line = m_CurrentThumbnailLineCount;
            int index = cur - col * line;

            if(index < 0) return 0;
            else          return index;
        });
}

bool GraphicsTableView::ThumbList_MoveToNextPage(){
    return MoveTo(true, [this](int cur, int len){

            int col = m_CurrentThumbnailColumnCount;
            int line = m_CurrentThumbnailLineCount;
            int index = cur + col * line;

            if(index >= len) return len - 1;
            else             return index;
        });
}

bool GraphicsTableView::ThumbList_MoveToFirstItem(){
    return MoveTo(false, [this](int cur, int len){
            Q_UNUSED(cur); Q_UNUSED(len);
            return 0;
        });
}

bool GraphicsTableView::ThumbList_MoveToLastItem(){
    return MoveTo(false, [this](int cur, int len){
            Q_UNUSED(cur);
            return len - 1;
        });
}

bool GraphicsTableView::ThumbList_SelectToUpperItem(){
    return SelectTo([this](){ ThumbList_MoveToUpperItem();});
}

bool GraphicsTableView::ThumbList_SelectToLowerItem(){
    return SelectTo([this](){ ThumbList_MoveToLowerItem();});
}

bool GraphicsTableView::ThumbList_SelectToRightItem(){
    return SelectTo([this](){ ThumbList_MoveToRightItem();});
}

bool GraphicsTableView::ThumbList_SelectToLeftItem(){
    return SelectTo([this](){ ThumbList_MoveToLeftItem();});
}

bool GraphicsTableView::ThumbList_SelectToPrevPage(){
    return SelectTo([this](){ ThumbList_MoveToPrevPage();});
}

bool GraphicsTableView::ThumbList_SelectToNextPage(){
    return SelectTo([this](){ ThumbList_MoveToNextPage();});
}

bool GraphicsTableView::ThumbList_SelectToFirstItem(){
    return SelectTo([this](){ ThumbList_MoveToFirstItem();});
}

bool GraphicsTableView::ThumbList_SelectToLastItem(){
    return SelectTo([this](){ ThumbList_MoveToLastItem();});
}

bool GraphicsTableView::ThumbList_SelectItem(){
    if(!m_CurrentNode || !IsDisplayingNode() || !GetHoveredNode()) return false;

    if(GetHoveredThumbnail()->isSelected())
        RemoveFromSelection(GetHoveredNode());
    if(GetHoveredNodeTitle()->isSelected())
        RemoveFromSelection(GetHoveredNode());

    if(GetHoveredThumbnail()->isSelected() ||
       GetHoveredNodeTitle()->isSelected()){

        GetHoveredThumbnail()->setSelected(false);
        GetHoveredNodeTitle()->setSelected(false);

    } else {

        GetHoveredThumbnail()->setSelected(true);
        AppendToSelection(GetHoveredNode());

        GetHoveredNodeTitle()->setSelected(true);
        AppendToSelection(GetHoveredNode());
    }
    return true;
}

bool GraphicsTableView::ThumbList_SelectRange(){
    if(!m_CurrentNode || !IsDisplayingNode() || !GetHoveredNode()) return false;

    SetSelectionRange(GetHoveredThumbnail());
    return true;
}

bool GraphicsTableView::ThumbList_SelectAll(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    if(m_NodesRegister.length() ==
       (m_DisplayThumbnails.length() + m_DisplayNodeTitles.length())){
        ClearThumbnailSelection();
        ClearNodeTitleSelection();
        ClearScrollControllerSelection();
        return true;
    }

    foreach(Thumbnail *thumb, m_DisplayThumbnails){
        if(!thumb->isSelected()){
            thumb->setSelected(true);
            AppendToSelection(thumb->GetNode());
        }
    }
    foreach(NodeTitle *title, m_DisplayNodeTitles){
        if(!title->isSelected()){
            title->setSelected(true);
            AppendToSelection(title->GetNode());
        }
    }
    return true;
}

bool GraphicsTableView::ThumbList_ClearSelection(){
    if(!m_CurrentNode || !IsDisplayingNode()) return false;

    ClearThumbnailSelection();
    ClearNodeTitleSelection();
    ClearScrollControllerSelection();
    return true;
}

bool GraphicsTableView::ThumbList_TransferToUpper(){
    return TransferTo(false, false, [this](Thumbnail *neighbor, int cur, int len){

            int col = m_CurrentThumbnailColumnCount;
            int index = neighbor ? cur - col : -1;
            int base = neighbor ? (len / col * col) + cur : -1;

            if     (!neighbor)   return len / col * col;
            else if(cur == -1)   return len;
            else if(index >= 0)  return index + 1;
            else if(index == -1) return 0;
            else if((len % col) <= cur) return base - col + 1;
            else                 return base + 1;
        });
}

bool GraphicsTableView::ThumbList_TransferToLower(){
    return TransferTo(true, false, [this](Thumbnail *neighbor, int cur, int len){

            int col = m_CurrentThumbnailColumnCount;
            int index = neighbor ? cur + col : -1;

            if     (!neighbor)    return len % col;
            else if(cur == -1)    return 0;
            else if(index == len) return index;
            else if(index >= len) return cur % col;
            else                  return index;
        });
}

bool GraphicsTableView::ThumbList_TransferToRight(){
    return TransferTo(true, false, [this](Thumbnail *neighbor, int cur, int len){
            Q_UNUSED(len);

            if(!neighbor) return 0;
            else          return cur + 1;
        });
}

bool GraphicsTableView::ThumbList_TransferToLeft(){
    return TransferTo(false, false, [this](Thumbnail *neighbor, int cur, int len){

            if(!neighbor) return len;
            else          return cur;
        });
}

bool GraphicsTableView::ThumbList_TransferToPrevPage(){
    return TransferTo(false, true, [this](Thumbnail *neighbor, int cur, int len){
            Q_UNUSED(len);

            int col = m_CurrentThumbnailColumnCount;
            int line = m_CurrentThumbnailLineCount;
            int index = neighbor ? cur - col * line : -1;

            if     (!neighbor) return 0;
            else if(index < 0) return 0;
            else               return index + 1;
        });
}

bool GraphicsTableView::ThumbList_TransferToNextPage(){
    return TransferTo(true, true, [this](Thumbnail *neighbor, int cur, int len){

            int col = m_CurrentThumbnailColumnCount;
            int line = m_CurrentThumbnailLineCount;
            int index = neighbor ? cur + col * line : -1;

            if     (!neighbor)   return len;
            else if(index > len) return len;
            else                 return index;
        });
}

bool GraphicsTableView::ThumbList_TransferToFirst(){
    return TransferTo(false, false, [this](Thumbnail *neighbor, int cur, int len){
            Q_UNUSED(neighbor); Q_UNUSED(cur); Q_UNUSED(len);
            return 0;
        });
}

bool GraphicsTableView::ThumbList_TransferToLast(){
    return TransferTo(true, false, [this](Thumbnail *neighbor, int cur, int len){
            Q_UNUSED(neighbor); Q_UNUSED(cur); Q_UNUSED(len);
            return len;
        });
}

bool GraphicsTableView::ThumbList_TransferToUpDirectory(){
    if(!m_CurrentNode || !IsDisplayingNode() || m_NodesRegister.isEmpty())
        return false;

    m_NodesRegister = m_NodesRegister.toSet().toList();

    Node *nd = GetHoveredNode() ? GetHoveredNode() : m_CurrentNode;
    NodeList sibling = nd->GetSiblings();

    foreach(Node *nd, m_NodesRegister){
        if(!sibling.contains(nd))
            m_NodesRegister.removeOne(nd);
    }

    qSort(m_NodesRegister.begin(), m_NodesRegister.end(),
          m_SortPredicate);

    int len = m_NodesRegister.length();

    if(!ThumbList_UpDirectory()) return false;

    // reuse variable.
    nd = GetHoveredNode() ? GetHoveredNode() : m_CurrentNode;
    m_TreeBank->SetChildrenOrder
        (nd->GetParent(), nd->GetSiblings() + m_NodesRegister);

    bool hasSuspended = StatusBarMessageIsSuspended();
    if(!hasSuspended) SuspendStatusBarMessage();

    // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
    ThumbList_Refresh();

    if(!hasSuspended) ResumeStatusBarMessage();

    for(int i = 1; i <= len; i++){
        SetSelectedByIndex(m_DisplayThumbnails.length() - i);
    }
    return true;
}

bool GraphicsTableView::ThumbList_TransferToDownDirectory(){
    if(!m_CurrentNode || !IsDisplayingNode() || m_NodesRegister.isEmpty())
        return false;

    m_NodesRegister = m_NodesRegister.toSet().toList();

    Node *nd = GetHoveredNode() ? GetHoveredNode() : m_CurrentNode;
    NodeList sibling = nd->GetSiblings();

    foreach(Node *nd, m_NodesRegister){
        if(!sibling.contains(nd))
            m_NodesRegister.removeOne(nd);
    }

    qSort(m_NodesRegister.begin(), m_NodesRegister.end(),
          m_SortPredicate);

    int len = m_NodesRegister.length();

    if(!ThumbList_DownDirectory()) return false;

    // reuse variable.
    nd = GetHoveredNode() ? GetHoveredNode() : m_CurrentNode;
    m_TreeBank->SetChildrenOrder
        (nd->GetParent(), nd->GetSiblings() + m_NodesRegister);

    bool hasSuspended = StatusBarMessageIsSuspended();
    if(!hasSuspended) SuspendStatusBarMessage();

    // 'ThumbList_Refresh' calls 'm_NodesRegister.clear()'.
    ThumbList_Refresh();

    if(!hasSuspended) ResumeStatusBarMessage();

    for(int i = 1; i <= len; i++){
        SetSelectedByIndex(m_DisplayThumbnails.length() - i);
    }
    return true;
}

bool GraphicsTableView::ThumbList_SwitchNodeCollectionType(){
    // NodeCollectionType cycle is
    // Flat => Straight => Recursive => Foldable => Flat ...
    if(IsDisplayingNode()){
        switch (GetNodeCollectionType()){
        case Flat:      SetNodeCollectionType(Straight);  break;
        case Straight:  SetNodeCollectionType(Recursive); break;
        case Recursive: SetNodeCollectionType(Foldable);  break;
        case Foldable:  SetNodeCollectionType(Flat);      break;
        default: return false;
        }
    } else {
        return false;
    }

    bool hasSuspended = StatusBarMessageIsSuspended();

    if(!hasSuspended){
        SuspendStatusBarMessage();
    }

    ThumbList_Refresh();

    if(!hasSuspended){
        ResumeStatusBarMessage();

        QString s = tr("Displaying %1 nodes.").arg(m_DisplayThumbnails.length());
        switch(GetNodeCollectionType()){
        case Flat:      emit statusBarMessage2(s, QStringLiteral("[Flat] Straight  Recursive  Foldable ")); break;
        case Straight:  emit statusBarMessage2(s, QStringLiteral(" Flat [Straight] Recursive  Foldable ")); break;
        case Recursive: emit statusBarMessage2(s, QStringLiteral(" Flat  Straight [Recursive] Foldable ")); break;
        case Foldable:  emit statusBarMessage2(s, QStringLiteral(" Flat  Straight  Recursive [Foldable]")); break;
        }
    }
    return true;
}

bool GraphicsTableView::ThumbList_SwitchNodeCollectionTypeReverse(){
    // NodeCollectionType cycle is
    // Flat => Foldable => Recursive => Straight => Flat ...
    if(IsDisplayingNode()){
        switch (GetNodeCollectionType()){
        case Flat:      SetNodeCollectionType(Foldable);  break;
        case Foldable:  SetNodeCollectionType(Recursive); break;
        case Straight:  SetNodeCollectionType(Flat);      break;
        case Recursive: SetNodeCollectionType(Straight);  break;
        default: return false;
        }
    } else {
        return false;
    }

    bool hasSuspended = StatusBarMessageIsSuspended();

    if(!hasSuspended){
        SuspendStatusBarMessage();
    }

    ThumbList_Refresh();

    if(!hasSuspended){
        ResumeStatusBarMessage();

        QString s = tr("Displaying %1 nodes.").arg(m_DisplayThumbnails.length());
        switch(GetNodeCollectionType()){
        case Flat:      emit statusBarMessage2(s, QStringLiteral("[Flat] Straight  Recursive  Foldable ")); break;
        case Straight:  emit statusBarMessage2(s, QStringLiteral(" Flat [Straight] Recursive  Foldable ")); break;
        case Recursive: emit statusBarMessage2(s, QStringLiteral(" Flat  Straight [Recursive] Foldable ")); break;
        case Foldable:  emit statusBarMessage2(s, QStringLiteral(" Flat  Straight  Recursive [Foldable]")); break;
        }
    }
    return true;
}

QPointF GraphicsTableView::CurrentThumbnailOffset(){
    return QPointF(0,
                   - m_CurrentOffsetValue
                   / m_CurrentThumbnailColumnCount
                   * m_CurrentThumbnailHeight);
}

QPointF GraphicsTableView::CurrentNodeTitleOffset(){
    return QPointF(0,
                   - m_CurrentOffsetValue
                   / m_CurrentThumbnailColumnCount
                   * m_CurrentThumbnailColumnCount
                   * GetStyle()->NodeTitleHeight());
}

SpotLight::SpotLight(GraphicsTableView::SpotLightType type, QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    m_Type = type;
    m_Index = -1;
    setZValue(SPOT_LIGHT_LAYER);
    setAcceptHoverEvents(false);
    setAcceptDrops(false);
    hide();
}

SpotLight::~SpotLight(){
}

QRectF SpotLight::boundingRect() const{

    const GraphicsTableView* parent = static_cast<GraphicsTableView*>(parentItem());

    const bool p = m_Type == GraphicsTableView::PrimarySpotLight;
    const bool h = m_Type == GraphicsTableView::HoveredSpotLight;
    const bool l = m_Type == GraphicsTableView::LoadedSpotLight;

    const int index =
        p ? parent->m_PrimaryItemIndex :
        h ? parent->m_HoveredItemIndex :
        l ? m_Index : -1; // default : -1

    if(index == -1 ||
       index >= parent->m_DisplayThumbnails.length())
        return QRectF();

    const Thumbnail *thumb = parent->m_DisplayThumbnails[index];
    const NodeTitle *title = parent->m_DisplayNodeTitles[index];

    const int x1  = thumb->pos().x() + thumb->rect().right() - 1; // need if optimization 'QGraphicsView::DontAdjustForAntialiasing' is on.
    const int x2  = title->pos().x() + title->rect().left();

    const int y1b = thumb->pos().y() + thumb->rect().top();
    const int y1e = thumb->pos().y() + thumb->rect().bottom();

    const int y2b = title->pos().y() + title->rect().top();
    const int y2e = title->pos().y() + title->rect().bottom();

    return parent->boundingRect().
        intersected(QRectF(qMin(x1,  x2 ),  qMin(y1b, y2b),
                           qMax(x1,  x2 ) - qMin(x1,  x2 ),
                           qMax(y1e, y2e) - qMin(y1b, y2b)));
}

QPainterPath SpotLight::shape() const{

    const GraphicsTableView* parent = static_cast<GraphicsTableView*>(parentItem());

    const bool p = m_Type == GraphicsTableView::PrimarySpotLight;
    const bool h = m_Type == GraphicsTableView::HoveredSpotLight;
    const bool l = m_Type == GraphicsTableView::LoadedSpotLight;

    const int index =
        p ? parent->m_PrimaryItemIndex :
        h ? parent->m_HoveredItemIndex :
        l ? m_Index : -1; // default : -1

    if(index == -1 ||
       index >= parent->m_DisplayThumbnails.length())
        return QPainterPath();

    const Thumbnail *thumb = parent->m_DisplayThumbnails[index];
    const NodeTitle *title = parent->m_DisplayNodeTitles[index];

    const int x1  = thumb->pos().x() + thumb->rect().right() - 1; // need if optimization 'QGraphicsView::DontAdjustForAntialiasing' is on.
    const int x2  = title->pos().x() + title->rect().left();

    const int y1b = thumb->pos().y() + thumb->rect().top();
    const int y1e = thumb->pos().y() + thumb->rect().bottom();

    const int y2b = title->pos().y() + title->rect().top();
    const int y2e = title->pos().y() + title->rect().bottom();

    QPainterPath path;
    QPolygonF polygon;

    polygon
        << QPointF(x1, y1b) << QPointF(x2, y2b)
        << QPointF(x2, y2e) << QPointF(x1, y1e);

    polygon = QPolygonF(parent->boundingRect()).intersected(polygon);

    path.addPolygon(polygon);

    return path;
}

void SpotLight::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);
    static_cast<GraphicsTableView*>(parentItem())->GetStyle()->Render(this, painter);
}

ScrollController::ScrollController(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    m_TableView = static_cast<GraphicsTableView*>(parent);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setPen(QPen(QColor(0,0,0,0)));
    setBrush(QBrush(QColor(0,0,0,0)));
    setZValue(MAIN_CONTENTS_LAYER);
    hide();
}

ScrollController::~ScrollController(){}

void ScrollController::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);

    painter->save();

    painter->setPen(QColor(255,255,255,255));
    if(isSelected())
        painter->setBrush(QColor(200,200,255,100));
    else
        painter->setBrush(QColor(255,255,255,100));

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->drawRect(boundingRect());

    painter->restore();
}

QVariant ScrollController::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemPositionChange && scene()){
        QPointF newPos = value.toPointF();
        newPos.setX(0);
        if(newPos.y() < 0)
            newPos.setY(0);
        if(newPos.y() + boundingRect().size().height() + 4 > m_TableView->ScrollBarAreaRect().size().height())
            newPos.setY(m_TableView->ScrollBarAreaRect().size().height()
                        - boundingRect().size().height() - 4);

        const int offset =
            m_TableView->m_DisplayThumbnails.length() * newPos.y() /
            (m_TableView->ScrollBarAreaRect().size().height() - boundingRect().size().height());
        if(isSelected()) m_TableView->SetScroll(offset);
        return newPos;
    }
    return QGraphicsRectItem::itemChange(change, value);
}

void ScrollController::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragEnterEvent(ev);
}

void ScrollController::dropEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dropEvent(ev);
}

void ScrollController::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragMoveEvent(ev);
}

void ScrollController::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragLeaveEvent(ev);
}

void ScrollController::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    m_TableView->ClearThumbnailSelection();
    m_TableView->ClearNodeTitleSelection();
    QGraphicsRectItem::mousePressEvent(ev);
    setCursor(Qt::ClosedHandCursor);
}

void ScrollController::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsRectItem::mouseReleaseEvent(ev);
    setCursor(Qt::OpenHandCursor);
}

void ScrollController::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    QRectF rect1 = rect().translated(pos());

    // after calling 'ApplyChildrenOrder',
    // this 'mouseMoveEvent' sets wrong position.
    //QGraphicsRectItem::mouseMoveEvent(ev);

    setPos(0, ev->scenePos().y() - rect().top() - rect().height()/2.0);

    QRectF rect2 = rect().translated(pos());

    m_TableView->Update(rect1);
    m_TableView->Update(rect2);
}

void ScrollController::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverMoveEvent(ev);
    setCursor(Qt::OpenHandCursor);
}

void ScrollController::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverLeaveEvent(ev);
    setCursor(Qt::ArrowCursor);
}

void ScrollController::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverMoveEvent(ev);
}

InPlaceNotifier::InPlaceNotifier(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    m_TableView = static_cast<GraphicsTableView*>(parent);
    setPen(QPen(QColor(0,0,0,0)));
    setBrush(QBrush(QColor(0,0,0,0)));
    setZValue(IN_PLACE_NOTIFIER_LAYER);
    setAcceptHoverEvents(false);
    setAcceptDrops(false);
    setRect(QRectF(QPoint(0, 0),
                   QSize(m_TableView->GetStyle()->InPlaceNotifierWidth(),
                         m_TableView->GetStyle()->InPlaceNotifierHeight())));
    hide();
}

InPlaceNotifier::~InPlaceNotifier(){
}

void InPlaceNotifier::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);
    m_TableView->GetStyle()->Render(this, painter);
}

void InPlaceNotifier::SetNode(Node *nd){
    m_Node = nd;
}

QVariant InPlaceNotifier::itemChange(GraphicsItemChange change, const QVariant &value){
    return QGraphicsRectItem::itemChange(change, value);
}

void InPlaceNotifier::wheelEvent        (QGraphicsSceneWheelEvent *ev){ ev->setAccepted(false);}
void InPlaceNotifier::mousePressEvent   (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void InPlaceNotifier::mouseReleaseEvent (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void InPlaceNotifier::mouseMoveEvent    (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void InPlaceNotifier::hoverEnterEvent   (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
void InPlaceNotifier::hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
void InPlaceNotifier::hoverMoveEvent    (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}

UpDirectoryButton::UpDirectoryButton(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    m_TableView = static_cast<GraphicsTableView*>(parent);
    m_Icon = Application::style()->standardIcon(QStyle::SP_TitleBarShadeButton).pixmap(QSize(10, 10));
    setAcceptHoverEvents(true);
    setZValue(MAIN_CONTENTS_LAYER);
    setPos(0,0);
    setRect(QRectF(-5, -5, 23, 23));
    SetHovered(false);
    hide();
}

UpDirectoryButton::~UpDirectoryButton(){}

void UpDirectoryButton::SetHovered(bool hovered){
    m_Hovered = hovered;
    m_TableView->GetStyle()->OnSetHovered(this, hovered);
}

void UpDirectoryButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget){
    Q_UNUSED(item); Q_UNUSED(widget);
    m_TableView->GetStyle()->Render(this, painter);
}

void UpDirectoryButton::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsRectItem::mousePressEvent(ev);
    ev->setAccepted(true);
}

void UpDirectoryButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsRectItem::mousePressEvent(ev);
    m_TableView->ThumbList_UpDirectory();
    ev->setAccepted(true);
}

void UpDirectoryButton::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsRectItem::mouseMoveEvent(ev);
    ev->setAccepted(true);
}

void UpDirectoryButton::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverMoveEvent(ev);
    SetHovered(true);
    ev->setAccepted(true);
}

void UpDirectoryButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverLeaveEvent(ev);
    SetHovered(false);
    ev->setAccepted(true);
}

void UpDirectoryButton::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverMoveEvent(ev);
    ev->setAccepted(true);
}
