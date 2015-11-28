#include "switch.hpp"
#include "const.hpp"

#include "treebar.hpp"

#include "treebank.hpp"
#include "gadgets.hpp"

#include <QPainter>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QPropertyAnimation>

/*
  collecting model.
    horizontal(like Tungsten):
      * all siblings of all ancestors of current view node.
      * grid layout.
      * movable(to other directory also).
      * scroll siblings line, when mouse cursor is out of widget.
    vertical(like 'Tree Style Tab' of Firefox):
      * collect siblings of current view node,
        and collect parent's siblings if not that is not folded,
        repeat above.
      * movable(to other directory also).
      * foldable.
      * scroll siblings line, when mouse cursor is out of widget.

  context menu.
    * TODO: display menu(show/hide MenuBar and TreeBar).
    * new view(add to neighbor of selected node).
    * clone view.
    * reload(if not directory).
    * rename node(if directory).
    * delete node.
    * delete right node.
    * delete left node.
    * delete other node.
    * make directory.
    * make directory with selected node.
    * make directory with same domain node.

  add view node menu.
    horizontal:
      * new view in selected line.
      * clone primary view of selected line.
      * recently closed views(restore to selected line).
    vertical:
      * new view on neighbor of primary view.
      * clone primary view.
      * recently closed views(restore to neighbor of primary view).

  display view tree button.

  close button.

  TODO: resizable(when vertical).

  TODO: auto hide.

  TODO: save scroll or refreshment without delete.
    signal candidate.
      * NodeTitleChanged.
        * View::titleChanged.
      * NodeAdded.
        * TreeBank::CreateView.
      * NodeDeleted.
        * TreeBank::DeleteNode.
      * CurrentChanged.
        * TreeBank::SetCurrent.
      * ChildrenOrderChanged(moved also).
        * TreeBank::SetChildrenOrder.
      * TreeStructureChanged.
        * Application::Import.
        * TreeBank::LoadTree.
 */

#define TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH 150
#define TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT 23

#define TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH 300
#define TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT 23

bool TreeBar::m_EnableAnimation    = false;
bool TreeBar::m_EnableCloseButton  = false;
bool TreeBar::m_ScrollToSwitchNode = false;
bool TreeBar::m_DoubleClickToClose = false;
bool TreeBar::m_WheelClickToClose  = false;

namespace {

    class GraphicsButton : public QGraphicsItem {
    public:
        GraphicsButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : QGraphicsItem(parent)
        {
            m_TreeBank = tb;
            m_TreeBar = bar;
            m_ButtonState = NotHovered;
            setZValue(40);
            setAcceptHoverEvents(true);
        }

    protected:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            Q_UNUSED(ev);
            m_ButtonState = Pressed;
            if(ev->button() == Qt::RightButton)
                QGraphicsItem::mousePressEvent(ev);
            update();
        }
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            Q_UNUSED(ev);
            m_ButtonState = Hovered;
            if(ev->button() == Qt::RightButton)
                QGraphicsItem::mouseReleaseEvent(ev);
            update();
        }
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            m_ButtonState = Hovered;
            QGraphicsItem::mouseMoveEvent(ev);
        }
        virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            QGraphicsItem::mouseDoubleClickEvent(ev);
        }
        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE {
            m_ButtonState = Hovered;
            QGraphicsItem::hoverEnterEvent(ev);
            update();
        }
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE {
            m_ButtonState = NotHovered;
            QGraphicsItem::hoverLeaveEvent(ev);
            update();
        }
        virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE {
            m_ButtonState = Hovered;
            QGraphicsItem::hoverMoveEvent(ev);
        }

        enum ButtonState{
            NotHovered,
            Hovered,
            Pressed,
        } m_ButtonState;

        TreeBank *m_TreeBank;
        TreeBar *m_TreeBar;
    };

    class TableButton : public GraphicsButton {
    public:
        TableButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : GraphicsButton(tb, bar, parent)
        {
        }
        QRectF boundingRect() const DECL_OVERRIDE {
            QRectF rect = scene()->sceneRect();
            switch(m_TreeBar->orientation()){
            case Qt::Horizontal: rect.setWidth(17);  break;
            case Qt::Vertical:   rect.setHeight(17); break;
            }
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            const QBrush brush(QColor(240,240,240,255));
            painter->setPen(Qt::NoPen);
            painter->setBrush(brush);
            painter->drawRect(boundingRect());

            if(m_ButtonState == Hovered ||
               m_ButtonState == Pressed){
                static const QBrush h = QBrush(QColor(180, 180, 180, 255));
                static const QBrush p = QBrush(QColor(150, 150, 150, 255));
                painter->setBrush(m_ButtonState == Hovered ? h : p);
                painter->setPen(Qt::NoPen);
                painter->setRenderHint(QPainter::Antialiasing, true);
                QPoint offset;
                switch(m_TreeBar->orientation()){
                case Qt::Horizontal: offset = QPoint(-9, -8); break;
                case Qt::Vertical:   offset = QPoint(-8, -9); break;
                }
                painter->drawRoundedRect(QRect(boundingRect().center().toPoint() + offset,
                                               QSize(13, 13)), 1, 1);
            }
            static QPixmap table;
            if(table.isNull())
                table = QPixmap(":/resources/treebar/table.png");
            QPoint offset;
            switch(m_TreeBar->orientation()){
            case Qt::Horizontal: offset = QPoint(-8, -7); break;
            case Qt::Vertical:   offset = QPoint(-7, -8); break;
            }
            painter->drawPixmap
                (QRect(boundingRect().center().toPoint() + offset,
                       table.size()),
                 table, QRect(QPoint(), table.size()));
        }

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);

            Gadgets *g = m_TreeBank->GetGadgets();
            if(g && g->IsActive()) g->Deactivate();
            else m_TreeBank->DisplayViewTree();
        }
    };

    class PlusButton : public GraphicsButton {
    public:
        PlusButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : GraphicsButton(tb, bar, parent)
        {
        }
        QRectF boundingRect() const DECL_OVERRIDE {
            QRectF rect = parentItem()->boundingRect();
            switch(m_TreeBar->orientation()){
            case Qt::Horizontal: rect.setLeft(rect.right() - 17); break;
            case Qt::Vertical:   rect.setTop(rect.bottom() - 17); break;
            }
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            const QBrush brush(QColor(240,240,240,255));
            painter->setPen(Qt::NoPen);
            painter->setBrush(brush);
            painter->drawRect(boundingRect());

            if(m_ButtonState == Hovered ||
               m_ButtonState == Pressed){
                static const QBrush h = QBrush(QColor(180, 180, 180, 255));
                static const QBrush p = QBrush(QColor(150, 150, 150, 255));
                painter->setBrush(m_ButtonState == Hovered ? h : p);
                painter->setPen(Qt::NoPen);
                painter->setRenderHint(QPainter::Antialiasing, true);
                QPoint offset;
                switch(m_TreeBar->orientation()){
                case Qt::Horizontal: offset = QPoint(-7, -8); break;
                case Qt::Vertical:   offset = QPoint(-8, -7); break;
                }
                painter->drawRoundedRect(QRect(boundingRect().center().toPoint() + offset,
                                               QSize(13, 13)), 1, 1);
            }
            static QPixmap plus;
            if(plus.isNull())
                plus = QPixmap(":/resources/treebar/plus.png");
            QPoint offset;
            switch(m_TreeBar->orientation()){
            case Qt::Horizontal: offset = QPoint(-6, -7); break;
            case Qt::Vertical:   offset = QPoint(-7, -6); break;
            }
            painter->drawPixmap
                (QRect(boundingRect().center().toPoint() + offset,
                       plus.size()),
                 plus, QRect(QPoint(), plus.size()));
        }
    private:
        ViewNode *GetViewNode(){
            LayerItem *layer = static_cast<LayerItem*>(parentItem());
            return layer->GetNode() ? layer->GetNode()->ToViewNode() : 0;
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);

            QMenu *menu = new QMenu(m_TreeBar);

            TreeBank *tb = m_TreeBank;
            ViewNode *vn = GetViewNode();

            QAction *newViewNode = new QAction(menu);
            newViewNode->setText(QObject::tr("NewViewNode"));
            newViewNode->connect(newViewNode, &QAction::triggered,
                                 [tb, vn](){
                                     if(vn) tb->NewViewNode(vn);
                                     else tb->OpenOnSuitableNode(QUrl("about:blank"), true);
                                 });
            menu->addAction(newViewNode);

            QAction *cloneViewNode = new QAction(menu);
            cloneViewNode->setText(QObject::tr("CloneViewNode"));
            cloneViewNode->connect(cloneViewNode, &QAction::triggered,
                                   [tb, vn](){
                                       if(vn) tb->CloneViewNode(vn);
                                       else tb->OpenOnSuitableNode(QUrl("about:blank"), true);
                                   });
            menu->addAction(cloneViewNode);

            NodeList trash = TreeBank::GetTrashRoot()->GetChildren();

            QMenu *restoreMenu = menu;
            int max = 20;
            int i = 0;

            if(trash.length())
                restoreMenu->addSeparator();

            for(int j = 0; j < 10; j++, max+=20){
                for(; i < qMin(max, trash.length()); i++){
                    ViewNode *t = trash[i]->ToViewNode();
                    QAction *restore = new QAction(restoreMenu);
                    QString title = t->GetTitle();

                    if(title.isEmpty()){
                        QUrl url = t->GetUrl();
                        if(url.isEmpty()){
                            if(t->IsDirectory()){
                                title = QStringLiteral("Directory");
                            } else {
                                title = QStringLiteral("No Title");
                            }
                        } else {
                            title = url.toString();
                        }
                    } else if(t->IsDirectory()){
                        title = title.split(QStringLiteral(";")).first();
                    }

                    if(title.length() > 25)
                        title = title.left(25) + QStringLiteral("...");
                    restore->setText(title);
                    restore->connect
                        (restore, &QAction::triggered,
                         [t, vn, tb](){
                            // both function emits TreeStructureChanged for current.
                            if(vn)
                                tb->MoveNode(t, vn->GetParent(), vn->SiblingsIndexOf(vn) + 1);
                            else
                                tb->MoveNode(t, TreeBank::GetViewRoot());
                            tb->SetCurrent(t);
                        });
                    restoreMenu->addAction(restore);
                }
                if(i >= trash.length()) break;
                if(j < 9){
                    QMenu *child = new QMenu(QObject::tr("More"), restoreMenu);
                    restoreMenu->addMenu(child);
                    restoreMenu = child;
                }
            }
            QAction *displayTrashTree = new QAction(restoreMenu);
            displayTrashTree->setText(QObject::tr("DisplayTrashTree"));
            displayTrashTree->connect(displayTrashTree, &QAction::triggered,
                                      [tb](){ tb->DisplayTrashTree();});
            restoreMenu->addAction(displayTrashTree);

            menu->exec(ev->screenPos());
            delete menu;
        }
    };

    class ScrollButton : public GraphicsButton {
    public:
        ScrollButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : GraphicsButton(tb, bar, parent)
        {
            {
                static const QColor beg = QColor(240,240,240,255);
                static const QColor end = QColor(240,240,240,0);
                m_Gradient.setColorAt(static_cast<qreal>(0), beg);
                m_Gradient.setColorAt(static_cast<qreal>(0.5), beg);
                m_Gradient.setColorAt(static_cast<qreal>(1), end);
            }
            {
                static const QColor beg = QColor(210,210,210,255);
                static const QColor end = QColor(210,210,210,0);
                m_HoveredGradient.setColorAt(static_cast<qreal>(0), beg);
                m_HoveredGradient.setColorAt(static_cast<qreal>(0.5), beg);
                m_HoveredGradient.setColorAt(static_cast<qreal>(1), end);
            }
            {
                static const QColor beg = QColor(128,128,128,255);
                static const QColor end = QColor(128,128,128,0);
                m_PressedGradient.setColorAt(static_cast<qreal>(0), beg);
                m_PressedGradient.setColorAt(static_cast<qreal>(0.5), beg);
                m_PressedGradient.setColorAt(static_cast<qreal>(1), end);
            }
        }
        ~ScrollButton(){}

    protected:
        QLinearGradient m_Gradient;
        QLinearGradient m_HoveredGradient;
        QLinearGradient m_PressedGradient;
    };

    class LeftScrollButton : public ScrollButton {
    public:
        LeftScrollButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : ScrollButton(tb, bar, parent)
        {
        }
        QRectF boundingRect() const DECL_OVERRIDE {
            QRectF rect = parentItem()->boundingRect();
            rect.setLeft(17);
            rect.setWidth(15);
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            QBrush brush;
            switch(m_ButtonState){
            case NotHovered:
                m_Gradient.setStart(boundingRect().left(), 0);
                m_Gradient.setFinalStop(boundingRect().right(), 0);
                brush = QBrush(m_Gradient);
                break;
            case Hovered:
                m_HoveredGradient.setStart(boundingRect().left(), 0);
                m_HoveredGradient.setFinalStop(boundingRect().right(), 0);
                brush = QBrush(m_HoveredGradient);
                break;
            case Pressed:
                m_PressedGradient.setStart(boundingRect().left(), 0);
                m_PressedGradient.setFinalStop(boundingRect().right(), 0);
                brush = QBrush(m_PressedGradient);
                break;
            }
            painter->setPen(Qt::NoPen);
            painter->setBrush(brush);
            painter->drawRect(boundingRect());
            static QPixmap left;
            if(left.isNull())
                left = QPixmap(":/resources/treebar/left.png");
            painter->drawPixmap
                (QRect(boundingRect().topLeft().toPoint() +
                       QPoint(-1, TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT/2 - 5),
                       left.size()),
                 left, QRect(QPoint(), left.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->MinusOffset(TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH);
        }
    };

    class RightScrollButton : public ScrollButton {
    public:
        RightScrollButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : ScrollButton(tb, bar, parent)
        {
        }
        QRectF boundingRect() const DECL_OVERRIDE {
            QRectF rect = parentItem()->boundingRect();
            rect.setLeft(rect.right() - 15 - 17);
            rect.setWidth(15);
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            QBrush brush;
            switch(m_ButtonState){
            case NotHovered:
                m_Gradient.setStart(boundingRect().right(), 0);
                m_Gradient.setFinalStop(boundingRect().left(), 0);
                brush = QBrush(m_Gradient);
                break;
            case Hovered:
                m_HoveredGradient.setStart(boundingRect().right(), 0);
                m_HoveredGradient.setFinalStop(boundingRect().left(), 0);
                brush = QBrush(m_HoveredGradient);
                break;
            case Pressed:
                m_PressedGradient.setStart(boundingRect().right(), 0);
                m_PressedGradient.setFinalStop(boundingRect().left(), 0);
                brush = QBrush(m_PressedGradient);
                break;
            }
            painter->setPen(Qt::NoPen);
            painter->setBrush(brush);
            painter->drawRect(boundingRect());
            static QPixmap right;
            if(right.isNull())
                right = QPixmap(":/resources/treebar/right.png");
            painter->drawPixmap
                (QRect(boundingRect().topRight().toPoint() +
                       QPoint(-10, TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT/2 - 5),
                       right.size()),
                 right, QRect(QPoint(), right.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->PlusOffset(TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH);
        }
    };

    class UpScrollButton : public ScrollButton {
    public:
        UpScrollButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : ScrollButton(tb, bar, parent)
        {
        }
        QRectF boundingRect() const DECL_OVERRIDE {
            QRectF rect = parentItem()->boundingRect();
            rect.setTop(17);
            rect.setHeight(15);
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            QBrush brush;
            switch(m_ButtonState){
            case NotHovered:
                m_Gradient.setStart(0, boundingRect().top());
                m_Gradient.setFinalStop(0, boundingRect().bottom());
                brush = QBrush(m_Gradient);
                break;
            case Hovered:
                m_HoveredGradient.setStart(0, boundingRect().top());
                m_HoveredGradient.setFinalStop(0, boundingRect().bottom());
                brush = QBrush(m_HoveredGradient);
                break;
            case Pressed:
                m_PressedGradient.setStart(0, boundingRect().top());
                m_PressedGradient.setFinalStop(0, boundingRect().bottom());
                brush = QBrush(m_PressedGradient);
                break;
            }
            painter->setPen(Qt::NoPen);
            painter->setBrush(brush);
            painter->drawRect(boundingRect());
            static QPixmap up;
            if(up.isNull())
                up = QPixmap(":/resources/treebar/up.png");
            painter->drawPixmap
                (QRect(boundingRect().topLeft().toPoint() +
                       QPoint(TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH/2 - 5, -1),
                       up.size()),
                 up, QRect(QPoint(), up.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->MinusOffset(TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT);
        }
    };

    class DownScrollButton : public ScrollButton {
    public:
        DownScrollButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : ScrollButton(tb, bar, parent)
        {
        }
        QRectF boundingRect() const DECL_OVERRIDE {
            QRectF rect = parentItem()->boundingRect();
            rect.setTop(rect.bottom() - 15 - 17);
            rect.setHeight(15);
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            QBrush brush;
            switch(m_ButtonState){
            case NotHovered:
                m_Gradient.setStart(0, boundingRect().bottom());
                m_Gradient.setFinalStop(0, boundingRect().top());
                brush = QBrush(m_Gradient);
                break;
            case Hovered:
                m_HoveredGradient.setStart(0, boundingRect().bottom());
                m_HoveredGradient.setFinalStop(0, boundingRect().top());
                brush = QBrush(m_HoveredGradient);
                break;
            case Pressed:
                m_PressedGradient.setStart(0, boundingRect().bottom());
                m_PressedGradient.setFinalStop(0, boundingRect().top());
                brush = QBrush(m_PressedGradient);
                break;
            }
            painter->setPen(Qt::NoPen);
            painter->setBrush(brush);
            painter->drawRect(boundingRect());
            static QPixmap down;
            if(down.isNull())
                down = QPixmap(":/resources/treebar/down.png");
            painter->drawPixmap
                (QRect(boundingRect().bottomLeft().toPoint() +
                       QPoint(TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH/2 - 5, -10),
                       down.size()),
                 down, QRect(QPoint(), down.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->PlusOffset(TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT);
        }
    };
}

TreeBar::TreeBar(TreeBank *tb, QWidget *parent)
    : QToolBar(tr("TreeBar"), parent)
{
    m_TreeBank = tb;
    m_Scene = new QGraphicsScene(this);
    m_Scene->setSceneRect(QRect(0, 0, width(), height()));
    m_View = new QGraphicsView(m_Scene, this);
    m_View->setFrameShape(QGraphicsView::NoFrame);
    m_View->setBackgroundBrush(QColor(240, 240, 240, 255));
    m_LayerList = QList<LayerItem*>();
    addWidget(m_View);
    setObjectName(QStringLiteral("TreeBar"));
    setContextMenuPolicy(Qt::PreventContextMenu);
    setMouseTracking(true);
    resize(width(), TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 7);
}

TreeBar::~TreeBar(){}

void TreeBar::Initialize(){
    LoadSettings();
}

void TreeBar::LoadSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup("treebar");{
        m_EnableAnimation    = settings->value(QStringLiteral("@EnableAnimation"),    true ).value<bool>();
        m_EnableCloseButton  = settings->value(QStringLiteral("@EnableCloseButton"),  true ).value<bool>();
        m_ScrollToSwitchNode = settings->value(QStringLiteral("@ScrollToSwitchNode"), false).value<bool>();
        m_DoubleClickToClose = settings->value(QStringLiteral("@DoubleClickToClose"), false).value<bool>();
        m_WheelClickToClose  = settings->value(QStringLiteral("@WheelClickToClose"),  true ).value<bool>();
    }
    settings->endGroup();
}

void TreeBar::SaveSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup("treebar");{
        settings->setValue(QStringLiteral("@EnableAnimation"),    m_EnableAnimation);
        settings->setValue(QStringLiteral("@EnableCloseButton"),  m_EnableCloseButton);
        settings->setValue(QStringLiteral("@ScrollToSwitchNode"), m_ScrollToSwitchNode);
        settings->setValue(QStringLiteral("@DoubleClickToClose"), m_DoubleClickToClose);
        settings->setValue(QStringLiteral("@WheelClickToClose"),  m_WheelClickToClose);
    }
    settings->endGroup();
}

bool TreeBar::EnableAnimation(){
    return m_EnableAnimation;
}

bool TreeBar::EnableCloseButton(){
    return m_EnableCloseButton;
}

bool TreeBar::ScrollToSwitchNode(){
    return m_ScrollToSwitchNode;
}

bool TreeBar::DoubleClickToClose(){
    return m_DoubleClickToClose;
}

bool TreeBar::WheelClickToClose(){
    return m_WheelClickToClose;
}

QSize TreeBar::sizeHint() const {
    return QSize(TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH + 7,
                 (TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 3) * m_LayerList.length() + 4);
}

QSize TreeBar::minimumSizeHint() const {
    return QSize(TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH + 7,
                 (TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 3) * m_LayerList.length() + 4);
}

void TreeBar::CollectNodes(){
    m_LayerList.clear();
    m_Scene->clear();
    m_Scene->addItem(new TableButton(m_TreeBank, this));

    Node *cur = m_TreeBank->GetCurrentViewNode();

    switch(orientation()){
    case Qt::Horizontal:{
        if(!cur || TreeBank::IsTrash(cur)){
            LayerItem *layer = new LayerItem(m_TreeBank, this, 0);
            m_LayerList.prepend(layer);
            m_Scene->addItem(layer);
        } else {
            while(cur && cur->GetParent()){
                LayerItem *layer = new LayerItem(m_TreeBank, this, cur);
                m_LayerList.prepend(layer);
                m_Scene->addItem(layer);
                cur = cur->GetParent();
            }
        }
        for(int i = 0; i < m_LayerList.length(); i++){
            LayerItem *layer = m_LayerList[i];
            NodeList list = layer->GetNode()
                ? layer->GetNode()->GetSiblings()
                : TreeBank::GetViewRoot()->GetChildren();
            for(int j = 0; j < list.length(); j++){
                NodeItem *item = new NodeItem(m_TreeBank, this, list[j], m_LayerList[i]);
                item->SetRect(QRectF(j * TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH + 19,
                                     i * (TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 3) + 1,
                                     TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH,
                                     TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT));
                layer->AppendToNodeItems(item);
            }
        }
        resize(0,0);
        updateGeometry();
        break;
    }
    case Qt::Vertical:{
        Node *nd = cur;
        NodeList list;
        if(!cur || TreeBank::IsTrash(cur)){
            list << TreeBank::GetViewRoot();
            cur = nd = 0;
        } else {
            while(nd){
                list << nd;
                nd = nd->GetParent();
            }
        }
        LayerItem *layer = new LayerItem(m_TreeBank, this, cur);
        m_LayerList.append(layer);
        m_Scene->addItem(layer);
        std::function<void(Node*, int)> collectNode;

        int max = 0;
        int i = 0;
        collectNode = [&](Node *nd, int nest){
            NodeItem *item = new NodeItem(m_TreeBank, this, nd, layer);
            item->SetRect(QRectF(nest * 20 + 1,
                                 i * TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT + 19,
                                 TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH - nest * 20,
                                 TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT));
            item->SetNest(nest);
            if(max < nest) max = nest;
            layer->AppendToNodeItems(item);
            i++;
            if(nd->IsDirectory()){
                if(nd->GetFolded()){
                    if(list.contains(nd) && nd->GetPrimary())
                        collectNode(nd->GetPrimary(), nest + 1);
                } else {
                    foreach(Node *child, nd->GetChildren()){
                        collectNode(child, nest + 1);
                    }
                }
            }
        };
        Node *root = list.last();
        foreach(Node *nd, root->GetChildren()){
            collectNode(nd, 0);
        }
        resize(0,0);
        updateGeometry();
        break;
    }
    }
}

void TreeBar::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    static const QBrush b = QBrush(QColor(240, 240, 240, 255));
    painter.setPen(Qt::NoPen);
    painter.setBrush(b);
    foreach(QRect rect, ev->region().rects()){
        painter.drawRect(rect);
    }
    QStyle *s = style();
    QStyleOptionToolBar opt;
    initStyleOption(&opt);
    opt.rect = s->subElementRect(QStyle::SE_ToolBarHandle, &opt, this);
    if(opt.rect.isValid() && ev->region().contains(opt.rect))
        s->drawPrimitive(QStyle::PE_IndicatorToolBarHandle, &opt, &painter, this);
}

void TreeBar::resizeEvent(QResizeEvent *ev){
    int width, height;
    if(orientation() == Qt::Horizontal){
        width = ev->size().width();
        height = (TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 3) * m_LayerList.length() + 4;
    }
    if(orientation() == Qt::Vertical){
        width = TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH + 7;
        height = ev->size().height();
    }
    QResizeEvent newev = QResizeEvent(QSize(width, height), ev->oldSize());
    QToolBar::resizeEvent(&newev);
    m_Scene->setSceneRect(0.0, 0.0, m_View->width(), m_View->height());
    foreach(LayerItem *layer, m_LayerList){ layer->OnResized();}
}

void TreeBar::showEvent(QShowEvent *ev){
    QToolBar::showEvent(ev);
    connect(m_TreeBank, &TreeBank::TreeStructureChanged,
            this, &TreeBar::CollectNodes);
    connect(this, &QToolBar::orientationChanged,
            this, &TreeBar::CollectNodes);
    if(m_LayerList.isEmpty())
        CollectNodes();
}

void TreeBar::hideEvent(QHideEvent *ev){
    QToolBar::hideEvent(ev);
    disconnect(m_TreeBank, &TreeBank::TreeStructureChanged,
               this, &TreeBar::CollectNodes);
    disconnect(this, &QToolBar::orientationChanged,
               this, &TreeBar::CollectNodes);
    m_LayerList.clear();
    m_Scene->clear();
}

void TreeBar::enterEvent(QEvent *ev){
    QToolBar::enterEvent(ev);
}

void TreeBar::leaveEvent(QEvent *ev){
    QToolBar::leaveEvent(ev);
}

void TreeBar::mouseMoveEvent(QMouseEvent *ev){
    QToolBar::mouseMoveEvent(ev);
}

void TreeBar::mousePressEvent(QMouseEvent *ev){
    QToolBar::mousePressEvent(ev);
}

void TreeBar::mouseReleaseEvent(QMouseEvent *ev){
    QToolBar::mouseReleaseEvent(ev);
}

LayerItem::LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    m_TreeBank = tb;
    m_TreeBar = bar;
    m_Node = nd;
    m_Nest = 0;
    m_Offset = 0;
    m_NodeItems = QList<NodeItem*>();

    new PlusButton(tb, bar, this);

    m_PrevScrollButton = 0;
    m_NextScrollButton = 0;

    m_Line = new QGraphicsLineItem(this);
    static const QPen p = QPen(QColor(80, 80, 200, 255));
    m_Line->setPen(p);
    m_Line->setZValue(MAIN_CONTENTS_LAYER + 1.0);
}

LayerItem::~LayerItem(){}

void LayerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(painter); Q_UNUSED(option); Q_UNUSED(widget);
    //painter->setPen(QPen(QColor(0,0,0,255)));
    //painter->setBrush(Qt::NoBrush);
    //QRectF rect = boundingRect();
    //painter->drawRect(rect.left(), rect.top(), rect.width()-1, rect.height()-1);
}

QRectF LayerItem::boundingRect() const {
    if(!scene()) return QRectF();
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:
        return QRectF(0, Index() * 26, scene()->sceneRect().width(), 26);
    case Qt::Vertical:
        return QRectF(QPointF(), scene()->sceneRect().size());
    }
    return QRectF();
}

int LayerItem::Index() const {
    return m_TreeBar->GetLayerList().indexOf(const_cast<LayerItem* const>(this));
}

int LayerItem::GetNest(){
    return m_Nest;
}

void LayerItem::SetNest(int nest){
    m_Nest = nest;
}

int LayerItem::MaxOffset(){
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        return TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH * m_NodeItems.length()
            - scene()->sceneRect().width() + 38;
    }
    case Qt::Vertical:{
        return TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT * m_NodeItems.length()
            - scene()->sceneRect().height() + 38;
    }
    }
    return 0;
}

int LayerItem::MinOffset(){
    return 0;
}

int LayerItem::GetOffset(){
    return m_Offset;
}

void LayerItem::SetOffset(int offset){
    int max = MaxOffset();
    if(max < 0) return;

    m_Offset = offset;
    if(m_Offset > max) m_Offset = max;
    if(m_Offset < 0) m_Offset = 0;
    OnScrolled();
    scene()->update();
}

void LayerItem::PlusOffset(int step){
    int max = MaxOffset();
    if(max < 0) return;

    m_Offset+=step;
    if(m_Offset > max) m_Offset = max;
    OnScrolled();
    scene()->update();
}

void LayerItem::MinusOffset(int step){
    int min = MinOffset();

    m_Offset-=step;
    if(m_Offset < min) m_Offset = min;
    OnScrolled();
    scene()->update();
}

void LayerItem::OnResized(){
    int i = 0;
    for(; i < m_NodeItems.length(); i++){
        if(m_NodeItems[i]->GetNode() == m_Node) break;
    }
    i++;
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        if(TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH * i > scene()->sceneRect().width() - 38){
            SetOffset(TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH * i - scene()->sceneRect().width() + 38);
        } else {
            SetOffset(0);
        }
        int width = boundingRect().width();
        SetLine(0.0,   (TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 3) * (Index() + 1) - 2,
                width, (TREEBAR_DEFAULT_NODE_HORIZONTAL_HEIGHT + 3) * (Index() + 1) - 2);
        break;
    }
    case Qt::Vertical:{
        if(TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT * i > scene()->sceneRect().height() - 38){
            SetOffset(TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT * i - scene()->sceneRect().height() + 38);
        } else {
            SetOffset(0);
        }
        int height = boundingRect().height();
        SetLine(TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH + 1, 0,
                TREEBAR_DEFAULT_NODE_VERTICAL_WIDTH + 1, height);
        break;
    }
    }
}

void LayerItem::OnScrolled(){
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        if(MaxOffset() >= 0 && m_Offset > MinOffset()){
            if(!m_PrevScrollButton)
                m_PrevScrollButton = new LeftScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_PrevScrollButton){
            scene()->removeItem(m_PrevScrollButton);
            m_PrevScrollButton = 0;
        }
        if(MaxOffset() >= 0 && m_Offset < MaxOffset()){
            if(!m_NextScrollButton)
                m_NextScrollButton = new RightScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_NextScrollButton){
            scene()->removeItem(m_NextScrollButton);
            m_NextScrollButton = 0;
        }
        break;
    }
    case Qt::Vertical:{
        if(MaxOffset() >= 0 && m_Offset > MinOffset()){
            if(!m_PrevScrollButton)
                m_PrevScrollButton = new UpScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_PrevScrollButton){
            scene()->removeItem(m_PrevScrollButton);
            m_PrevScrollButton = 0;
        }
        if(MaxOffset() >= 0 && m_Offset < MaxOffset()){
            if(!m_NextScrollButton)
                m_NextScrollButton = new DownScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_NextScrollButton){
            scene()->removeItem(m_NextScrollButton);
            m_NextScrollButton = 0;
        }
        break;
    }
    }
}

Node *LayerItem::GetNode(){
    return m_Node;
}

void LayerItem::SetLine(qreal x1, qreal y1, qreal x2, qreal y2){
    m_Line->setLine(x1, y1, x2, y2);
}

void LayerItem::ApplyChildrenOrder(){
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        NodeList list;
        foreach(NodeItem *item, m_NodeItems){
            list << item->GetNode();
        }
        if(!m_TreeBank->SetChildrenOrder(m_Node->GetParent(), list)){
            // reset.
            m_TreeBar->CollectNodes();
        }
        break;
    }
    case Qt::Vertical:{
        int i = 0;
        for(; i < m_NodeItems.length(); i++){
            if(m_NodeItems[i]->isSelected())
                break;
        }
        Node *nd = m_NodeItems[i]->GetNode();
        Node *above = 0;
        Node *below = 0;
        if(i > 0){
            above = m_NodeItems[i-1]->GetNode();
        }
        if(i < m_NodeItems.length() - 1){
            below = m_NodeItems[i+1]->GetNode();
        }
        if(above && m_NodeItems[i]->GetNest() == m_NodeItems[i-1]->GetNest()){
            NodeList list = above->GetSiblings();
            if(list.contains(nd)) list.removeOne(nd);
            list.insert(list.indexOf(above) + 1, nd);
            if(!m_TreeBank->SetChildrenOrder(above->GetParent(), list)){
                // reset.
                m_TreeBar->CollectNodes();
            }
            break;
        }
        if(below && m_NodeItems[i]->GetNest() == m_NodeItems[i+1]->GetNest()){
            NodeList list = below->GetSiblings();
            if(list.contains(nd)) list.removeOne(nd);
            list.insert(list.indexOf(below), nd);
            if(!m_TreeBank->SetChildrenOrder(below->GetParent(), list)){
                // reset.
                m_TreeBar->CollectNodes();
            }
            break;
        }
        if(above && m_NodeItems[i-1]->GetNode()->IsDirectory() &&
           m_NodeItems[i]->GetNest() == m_NodeItems[i-1]->GetNest()+1){
            NodeList list = above->GetChildren();
            if(list.contains(nd)) list.removeOne(nd);
            list.prepend(nd);
            if(!m_TreeBank->SetChildrenOrder(above, list)){
                // reset.
                m_TreeBar->CollectNodes();
            }
            break;
        }
        break;
    }
    }
}

void LayerItem::TransferNodeItem(NodeItem *item, LayerItem *other){
    for(int i = m_NodeItems.indexOf(item) + 1;
        i < m_NodeItems.length(); i++){
        m_NodeItems[i]->MoveToPrev();
    }
    m_NodeItems.removeOne(item);
    item->setParentItem(other);
    other->m_NodeItems.append(item);
    QRectF rect = item->GetRect();
    rect.moveTop(other->Index() * 26 + 1);
    item->SetRect(rect);
}

void LayerItem::AppendToNodeItems(NodeItem *item){
    m_NodeItems.append(item);
}

void LayerItem::PrependToNodeItems(NodeItem *item){
    m_NodeItems.prepend(item);
}

void LayerItem::RemoveFromNodeItems(NodeItem *item){
    m_NodeItems.removeOne(item);
}

void LayerItem::SwapWithNext(NodeItem *item){
    m_NodeItems.swap(m_NodeItems.indexOf(item),
                     m_NodeItems.indexOf(item)+1);
}

void LayerItem::SwapWithPrev(NodeItem *item){
    m_NodeItems.swap(m_NodeItems.indexOf(item)-1,
                     m_NodeItems.indexOf(item));
}

void LayerItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    //QGraphicsObject::mousePressEvent(ev);
}

void LayerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    //QGraphicsObject::mouseReleaseEvent(ev);

    if(ev->button() == Qt::RightButton){

        QMenu *menu = new QMenu(m_TreeBar);

        TreeBank *tb = m_TreeBank;
        ViewNode *vn = GetNode() ? GetNode()->ToViewNode() : 0;

        menu->addAction(tb->Action(TreeBank::Te_ToggleMenuBar));
        menu->addAction(tb->Action(TreeBank::Te_ToggleTreeBar));

        menu->addSeparator();

        QAction *newViewNode = new QAction(menu);
        newViewNode->setText(QObject::tr("NewViewNode"));
        newViewNode->connect(newViewNode, &QAction::triggered,
                             [tb, vn](){ tb->NewViewNode(vn);});
        menu->addAction(newViewNode);

        QAction *cloneViewNode = new QAction(menu);
        cloneViewNode->setText(QObject::tr("CloneViewNode"));
        cloneViewNode->connect(cloneViewNode, &QAction::triggered,
                               [tb, vn](){ tb->CloneViewNode(vn);});
        menu->addAction(cloneViewNode);

        menu->addSeparator();

        QAction *makeDirectory = new QAction(menu);
        makeDirectory->setText(QObject::tr("MakeDirectory"));
        makeDirectory->connect(makeDirectory, &QAction::triggered,
                               [tb, vn](){ tb->MakeSiblingDirectory(vn);});
        menu->addAction(makeDirectory);
        QAction *makeDirectoryWithSelected = new QAction(menu);
        makeDirectoryWithSelected->setText(QObject::tr("MakeDirectoryWithSelectedNode"));
        makeDirectoryWithSelected->connect
            (makeDirectoryWithSelected, &QAction::triggered,
             [tb, vn](){
                Node *parent = tb->MakeSiblingDirectory(vn->ToViewNode());
                tb->SetChildrenOrder(parent, NodeList() << vn);
            });
        menu->addAction(makeDirectoryWithSelected);
        QAction *makeDirectoryWithSameDomain = new QAction(menu);
        makeDirectoryWithSameDomain->setText(QObject::tr("MakeDirectoryWithSameDomainNode"));
        makeDirectoryWithSameDomain->connect
            (makeDirectoryWithSameDomain, &QAction::triggered,
             [tb, vn](){
                ViewNode *parent = vn->GetParent()->ToViewNode();
                QMap<QString, QList<Node*>> groups;
                foreach(Node *nd, vn->GetSiblings()){
                    if(!nd->IsDirectory()) groups[nd->GetUrl().host()] << nd;
                }
                if(groups.count() <= 1) return;
                foreach(QString domain, groups.keys()){
                    Node *directory = parent->MakeChild();
                    directory->SetTitle(domain);
                    tb->SetChildrenOrder(directory, groups[domain]);
                }
            });
        menu->addAction(makeDirectoryWithSameDomain);

        menu->exec(ev->screenPos());
        delete menu;
    }
}

void LayerItem::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsObject::mouseMoveEvent(ev);
}

void LayerItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsObject::mouseDoubleClickEvent(ev);
}

void LayerItem::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsObject::hoverEnterEvent(ev);
}

void LayerItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsObject::hoverLeaveEvent(ev);
}

void LayerItem::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsObject::hoverMoveEvent(ev);
}

void LayerItem::wheelEvent(QGraphicsSceneWheelEvent *ev){
    bool up = ev->delta() > 0;
    if(TreeBar::ScrollToSwitchNode()){

        NodeList siblings = m_Node->GetSiblings();
        int index = siblings.indexOf(m_Node);
        if(up){
            if(index >= 1)
                m_TreeBank->SetCurrent(siblings[index-1]);
        } else {
            if(index < siblings.length()-1)
                m_TreeBank->SetCurrent(siblings[index+1]);
        }

    } else {

        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            if(up) MinusOffset(TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH);
            else   PlusOffset (TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH);
            break;
        case Qt::Vertical:
            if(up) MinusOffset(TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT);
            else   PlusOffset (TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT);
            break;
        }
    }
}

NodeItem::NodeItem(TreeBank *tb, TreeBar *bar, Node *nd, QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    m_TreeBank = tb;
    m_TreeBar = bar;
    m_Node = nd;
    m_Nest = 0;
    m_IsPrimary = nd->IsPrimaryOfParent();
    m_IsHovered = false;
    m_CloseButtonState = NotHovered;
    m_TargetPosition = QPoint();
    if(TreeBar::EnableAnimation()){
        m_Animation = new QPropertyAnimation(this, "rect");
        m_Animation->setDuration(300);
        m_Animation->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_Animation, &QPropertyAnimation::finished,
                this, &NodeItem::ResetTargetPosition);
    } else {
        m_Animation = 0;
    }

    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    if(m_IsPrimary)
        setZValue((MAIN_CONTENTS_LAYER + DRAGGING_CONTENTS_LAYER) / 2.0 + 2.0);
    else
        setZValue(MAIN_CONTENTS_LAYER);
}

NodeItem::~NodeItem(){}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);

    painter->save();

    QRectF bound = boundingRect();
    QRect title_rect = bound.toRect();
    title_rect.setLeft(bound.left() + 4);
    title_rect.setWidth(bound.width() - (m_IsHovered ? 23 : 8));

    {
        static const QBrush b = QBrush(QColor(240,240,240,255));
        painter->setBrush(b);
        painter->setPen(Qt::NoPen);
        QRectF rect = bound;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal: rect.setHeight(rect.height() + 1); break;
        case Qt::Vertical:   rect.setWidth(rect.width() + 1);   break;
        }
        painter->drawRect(rect);
    }

    {
        static const QPen p = QPen(QColor(0,0,0,255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont("Meiryo", 9));
        QString title = m_Node->GetTitle();
        title = title.split(QStringLiteral(";")).first();
        if(m_TreeBar->orientation() == Qt::Vertical){
            if(m_Node->IsDirectory()){
                QString prefix = m_Node->GetFolded() ? QStringLiteral("+") : QStringLiteral("-");
                painter->drawText(title_rect, Qt::AlignLeft, prefix);
                title_rect.setLeft(title_rect.left() + 15);
            }
        }
        if(m_Node->IsDirectory()){
            if(title.isEmpty()){
                title = QStringLiteral("Directory");
            } else {
                title = QStringLiteral("Dir - ") + title;
            }
        }
        painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title);
    }

    if(m_IsHovered){
        static const QBrush b = QBrush(QColor(100,100,255,50));
        painter->setBrush(b);
        painter->setPen(Qt::NoPen);
        QRectF rect = bound;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal: rect.setHeight(rect.height() + 1); break;
        case Qt::Vertical:   rect.setWidth(rect.width() + 1);   break;
        }
        painter->drawRect(rect);
    }

    if(m_IsPrimary){
        static const QPen p = QPen(QColor(80,80,200,255));
        painter->setBrush(Qt::NoBrush);
        painter->setPen(p);
        QRectF rect = bound;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            rect.setWidth(rect.width() - 1);
            painter->drawLine(rect.bottomLeft(),
                              rect.topLeft());
            painter->drawLine(rect.topLeft(),
                              rect.topRight());
            painter->drawLine(rect.topRight(),
                              rect.bottomRight());
            break;
        case Qt::Vertical:
            rect.setHeight(rect.height() - 1);
            painter->drawLine(rect.topRight(),
                              rect.topLeft());
            painter->drawLine(rect.topLeft(),
                              rect.bottomLeft());
            painter->drawLine(rect.bottomLeft(),
                              rect.bottomRight());
            break;
        }
    }

    if(TreeBar::EnableCloseButton()){
        if(m_CloseButtonState == Hovered ||
           m_CloseButtonState == Pressed){
            static const QBrush h = QBrush(QColor(180, 180, 180, 255));
            static const QBrush p = QBrush(QColor(150, 150, 150, 255));
            painter->setBrush(m_CloseButtonState == Hovered ? h : p);
            painter->setPen(Qt::NoPen);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawRoundedRect(QRectF(bound.topRight() + QPointF(-18, 4),
                                            QSizeF(14, 14)), 2, 2);
        }

        static QPixmap close;
        if(close.isNull())
            close = QPixmap(":/resources/treebar/close.png");

        if(m_IsHovered)
            painter->drawPixmap(QRect(bound.topRight().toPoint() + QPoint(-16, 6),
                                      close.size()),
                                close, QRect(QPoint(), close.size()));
    }

    painter->restore();
}

QRectF NodeItem::boundingRect() const {
    if(isSelected()) return m_Rect;
    QRectF rect = m_Rect;
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:
        rect.moveLeft(rect.left() - Layer()->GetOffset());
        return rect;
    case Qt::Vertical:
        rect.moveTop(rect.top() - Layer()->GetOffset());
        return rect;
    }
    return m_Rect;
}

void NodeItem::SetRect(QRectF rect){
    m_Rect = rect;
    if(scene()) scene()->update();
}

QRectF NodeItem::GetRect() const {
    return m_Rect;
}

LayerItem *NodeItem::Layer() const {
    return static_cast<LayerItem*>(parentItem());
}

int NodeItem::GetNest(){
    return m_Nest;
}

void NodeItem::SetNest(int nest){
    m_Nest = nest;
}

Node *NodeItem::GetNode(){
    return m_Node;
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemPositionChange && scene()){
        QPointF newPos = value.toPointF();
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:{
            int min = -boundingRect().left() + 19;
            int max = -boundingRect().left() - 19
                + scene()->sceneRect().width() - TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH;
            if(newPos.x() < min) newPos.setX(min);
            if(newPos.x() > max) newPos.setX(max);
            newPos.setY(0);
            break;
        }
        case Qt::Vertical:{
            int min = -boundingRect().top() + 19;
            int max = -boundingRect().top() - 19
                + scene()->sceneRect().height() - TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT;
            if(newPos.y() < min) newPos.setY(min);
            if(newPos.y() > max) newPos.setY(max);
            newPos.setX(0);
            break;
        }
        }
        return newPos;
    }
    if(change == ItemSelectedChange && scene()){
        if(value.toBool()){
            setZValue(DRAGGING_CONTENTS_LAYER);
        } else {
            setZValue(MAIN_CONTENTS_LAYER);
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){

    if(ev->button() == Qt::LeftButton){

        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            m_Rect.moveLeft(m_Rect.left() - Layer()->GetOffset());
            break;
        case Qt::Vertical:
            m_Rect.moveTop(m_Rect.top() - Layer()->GetOffset());
            break;
        }

        if(TreeBar::EnableCloseButton() &&
           QRectF(boundingRect().topRight() + QPointF(-18, 4),
                  QSizeF(14, 14)).contains(ev->pos())){
            m_CloseButtonState = Pressed;
            update();
        }
    }
    QGraphicsObject::mousePressEvent(ev);
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){

        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            m_Rect.moveLeft(m_Rect.left() + Layer()->GetOffset());
            break;
        case Qt::Vertical:
            m_Rect.moveTop(m_Rect.top() + Layer()->GetOffset());
            break;
        }

        if((ev->buttonDownScreenPos(Qt::LeftButton) - ev->screenPos()).manhattanLength() < 4){
            if(m_CloseButtonState == Pressed){
                m_TreeBank->DeleteNode(m_Node);
            } else {
                if(m_TreeBar->orientation() == Qt::Vertical && m_Node->IsDirectory()){
                    m_Node->SetFolded(!m_Node->GetFolded());
                    m_TreeBar->CollectNodes();
                    return;
                }
                m_TreeBank->SetCurrent(m_Node);
            }
            return;
        }
    } else if(ev->button() == Qt::RightButton){

        QMenu *menu = new QMenu(m_TreeBar);

        TreeBank *tb = m_TreeBank;
        ViewNode *vn = GetNode() ? GetNode()->ToViewNode() : 0;

        QAction *newViewNode = new QAction(menu);
        newViewNode->setText(QObject::tr("NewViewNode"));
        newViewNode->connect(newViewNode, &QAction::triggered,
                             [tb, vn](){ tb->NewViewNode(vn);});
        menu->addAction(newViewNode);

        QAction *cloneViewNode = new QAction(menu);
        cloneViewNode->setText(QObject::tr("CloneViewNode"));
        cloneViewNode->connect(cloneViewNode, &QAction::triggered,
                               [tb, vn](){ tb->CloneViewNode(vn);});
        menu->addAction(cloneViewNode);

        menu->addSeparator();

        if(GetNode()->IsDirectory()){
            QAction *rename = new QAction(menu);
            rename->setText(QObject::tr("RenameViewNode"));
            rename->connect(rename, &QAction::triggered,
                            [tb, vn](){ tb->RenameNode(vn);});
            menu->addAction(rename);
        } else {
            QAction *reload = new QAction(menu);
            reload->setText(QObject::tr("ReloadViewNode"));
            reload->connect(reload, &QAction::triggered,
                            [vn](){
                                if(View *view = vn->GetView())
                                    view->TriggerAction(Page::We_Reload);
                            });
            menu->addAction(reload);
        }

        menu->addSeparator();

        QAction *deleteViewNode = new QAction(menu);
        deleteViewNode->setText(QObject::tr("DeleteViewNode"));
        deleteViewNode->connect(deleteViewNode, &QAction::triggered,
                                [tb, vn](){ tb->DeleteNode(vn);});
        menu->addAction(deleteViewNode);
        QAction *deleteRightNode = new QAction(menu);
        deleteRightNode->setText(m_TreeBar->orientation() == Qt::Horizontal
                                 ? QObject::tr("DeleteRightViewNode")
                                 : QObject::tr("DeleteLowerViewNode"));
        deleteRightNode->connect(deleteRightNode, &QAction::triggered,
                                 [tb, vn](){
                                     NodeList list;
                                     NodeList siblings = vn->GetSiblings();
                                     int index = siblings.indexOf(vn);
                                     for(int i = index + 1; i < siblings.length(); i++){
                                         list << siblings[i];
                                     }
                                     tb->DeleteNode(list);
                                 });
        menu->addAction(deleteRightNode);
        QAction *deleteLeftNode = new QAction(menu);
        deleteLeftNode->setText(m_TreeBar->orientation() == Qt::Horizontal
                                ? QObject::tr("DeleteLeftViewNode")
                                : QObject::tr("DeleteUpperViewNode"));
        deleteLeftNode->connect(deleteLeftNode, &QAction::triggered,
                                [tb, vn](){
                                    NodeList list;
                                    NodeList siblings = vn->GetSiblings();
                                    int index = siblings.indexOf(vn);
                                    for(int i = 0; i < index; i++){
                                        list << siblings[i];
                                    }
                                    tb->DeleteNode(list);
                                });
        menu->addAction(deleteLeftNode);
        QAction *deleteOtherNode = new QAction(menu);
        deleteOtherNode->setText(QObject::tr("DeleteOtherViewNode"));
        deleteOtherNode->connect(deleteOtherNode, &QAction::triggered,
                                 [tb, vn](){
                                     NodeList siblings = vn->GetSiblings();
                                     siblings.removeOne(vn);
                                     tb->DeleteNode(siblings);
                                 });
        menu->addAction(deleteOtherNode);

        menu->addSeparator();

        QAction *makeDirectory = new QAction(menu);
        makeDirectory->setText(QObject::tr("MakeDirectory"));
        makeDirectory->connect(makeDirectory, &QAction::triggered,
                               [tb, vn](){ tb->MakeSiblingDirectory(vn);});
        menu->addAction(makeDirectory);
        QAction *makeDirectoryWithSelected = new QAction(menu);
        makeDirectoryWithSelected->setText(QObject::tr("MakeDirectoryWithSelectedNode"));
        makeDirectoryWithSelected->connect
            (makeDirectoryWithSelected, &QAction::triggered,
             [tb, vn](){
                Node *parent = tb->MakeSiblingDirectory(vn->ToViewNode());
                tb->SetChildrenOrder(parent, NodeList() << vn);
            });
        menu->addAction(makeDirectoryWithSelected);
        QAction *makeDirectoryWithSameDomain = new QAction(menu);
        makeDirectoryWithSameDomain->setText(QObject::tr("MakeDirectoryWithSameDomainNode"));
        makeDirectoryWithSameDomain->connect
            (makeDirectoryWithSameDomain, &QAction::triggered,
             [tb, vn](){
                ViewNode *parent = vn->GetParent()->ToViewNode();
                QMap<QString, QList<Node*>> groups;
                foreach(Node *nd, vn->GetSiblings()){
                    if(!nd->IsDirectory()) groups[nd->GetUrl().host()] << nd;
                }
                if(groups.count() <= 1) return;
                foreach(QString domain, groups.keys()){
                    Node *directory = parent->MakeChild();
                    directory->SetTitle(domain);
                    tb->SetChildrenOrder(directory, groups[domain]);
                }
            });
        menu->addAction(makeDirectoryWithSameDomain);

        menu->exec(ev->screenPos());
        delete menu;
        return;
    } else if(ev->button() == Qt::MidButton &&
              TreeBar::WheelClickToClose()){
        m_TreeBank->DeleteNode(m_Node);
    }
    Layer()->ApplyChildrenOrder();
    // some time cause crash.
    //QGraphicsObject::mouseReleaseEvent(ev);
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    if(m_CloseButtonState == Pressed) return;

    QGraphicsObject::mouseMoveEvent(ev);

    LayerItem *newLayer = 0;
    static const int halfWidth  = TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH / 2.0;
    static const int halfHeight = TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT  / 2.0;

    foreach(QGraphicsItem *item, scene()->items(ev->scenePos())){
        if(newLayer = dynamic_cast<LayerItem*>(item)) break;
    }
    if(!newLayer){
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            if(ev->scenePos().y() < scene()->sceneRect().top())
                newLayer = m_TreeBar->GetLayerList().first();
            else if(ev->scenePos().y() > scene()->sceneRect().bottom())
                newLayer = m_TreeBar->GetLayerList().last();
            break;
        case Qt::Vertical:
            if(ev->scenePos().x() < scene()->sceneRect().left())
                newLayer = m_TreeBar->GetLayerList().first();
            else if(ev->scenePos().x() > scene()->sceneRect().right())
                newLayer = m_TreeBar->GetLayerList().last();
            break;
        }
    }
    if(newLayer && Layer() != newLayer){
        Layer()->TransferNodeItem(this, newLayer);
    }

    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:
        if(ev->scenePos().x() < scene()->sceneRect().left())
            Layer()->MinusOffset(8);
        else if(ev->scenePos().x() > scene()->sceneRect().right())
            Layer()->PlusOffset(8);
        break;
    case Qt::Vertical:
        if(ev->scenePos().y() < scene()->sceneRect().top())
            Layer()->MinusOffset(8);
        else if(ev->scenePos().y() > scene()->sceneRect().bottom())
            Layer()->PlusOffset(8);
        break;
    }

    QList<NodeItem*> &line = Layer()->GetNodeItems();
    if(line.length() < 2) return;
    int i = line.indexOf(this);
    bool moved = false;

    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        while(i+1 < line.length() &&
              halfWidth
              >= (line[i+1]->ScheduledPosition().x() -
                  line[i]->ScheduledPosition().x())){
            moved = true;
            line[i+1]->MoveToPrev();
            i++;
        }
        if(moved) break;
        while(i > 0 &&
              halfWidth
              >= (line[i]->ScheduledPosition().x() -
                  line[i-1]->ScheduledPosition().x())){
            line[i-1]->MoveToNext();
            i--;
        }
        break;
    }
    case Qt::Vertical:{
        while(i+1 < line.length() &&
              halfHeight
              >= (line[i+1]->ScheduledPosition().y() -
                  line[i]->ScheduledPosition().y())){
            moved = true;

            int nest = line[i+1]->GetNode()->IsDirectory()
                ? line[i+1]->GetNest()+1
                : line[i+1]->GetNest();

            if(line[i]->GetNest() != nest){

                line[i]->SetNest(nest);

                QRectF rect = line[i]->m_Rect;

                if(line[i]->m_Animation){
                    line[i]->m_Animation->stop();
                    line[i]->m_Animation->setStartValue(rect);
                    rect.setLeft(1+line[i]->GetNest()*20);
                    line[i]->m_Animation->setEndValue(rect);
                    line[i]->m_Animation->start();
                } else {
                    rect.setLeft(1+line[i]->GetNest()*20);
                    line[i]->SetRect(rect);
                }
            }
            line[i+1]->MoveToPrev();
            i++;
        }
        if(moved) break;
        while(i > 0 &&
              halfHeight
              >= (line[i]->ScheduledPosition().y() -
                  line[i-1]->ScheduledPosition().y())){

            int nest = line[i-1]->GetNest();

            if(line[i]->GetNest() != nest){

                line[i]->SetNest(nest);

                QRectF rect = line[i]->m_Rect;

                if(line[i]->m_Animation){
                    line[i]->m_Animation->stop();
                    line[i]->m_Animation->setStartValue(rect);
                    rect.setLeft(1+line[i]->GetNest()*20);
                    line[i]->m_Animation->setEndValue(rect);
                    line[i]->m_Animation->start();
                } else {
                    rect.setLeft(1+line[i]->GetNest()*20);
                    line[i]->SetRect(rect);
                }
            }
            line[i-1]->MoveToNext();
            i--;
        }
        break;
    }
    }
}

void NodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsObject::mouseDoubleClickEvent(ev);
    if(TreeBar::DoubleClickToClose()) m_TreeBank->DeleteNode(m_Node);
}

void NodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    m_IsHovered = true;
    if(TreeBar::EnableCloseButton() &&
       QRectF(boundingRect().topRight() + QPointF(-18, 4),
              QSizeF(14, 14)).contains(ev->pos())){
        m_CloseButtonState = Hovered;
        update();
    }
    QGraphicsObject::hoverEnterEvent(ev);
}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    m_IsHovered = false;
    if(m_CloseButtonState != NotHovered){
        m_CloseButtonState = NotHovered;
        update();
    }
    QGraphicsObject::hoverLeaveEvent(ev);
}

void NodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    m_IsHovered = true;
    if(TreeBar::EnableCloseButton() &&
       QRectF(boundingRect().topRight() + QPointF(-18, 4),
              QSizeF(14, 14)).contains(ev->pos())){
        if(m_CloseButtonState == NotHovered){
            m_CloseButtonState = Hovered;
            update();
        }
    } else {
        if(m_CloseButtonState != NotHovered){
            m_CloseButtonState = NotHovered;
            update();
        }
    }
    QGraphicsObject::hoverMoveEvent(ev);
}

void NodeItem::wheelEvent(QGraphicsSceneWheelEvent *ev){
    // call LayerItem's wheel event.
    QGraphicsObject::wheelEvent(ev);
}

QPointF NodeItem::ScheduledPosition(){
    if(m_TargetPosition.isNull()) return boundingRect().topLeft() + pos();
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal: return m_TargetPosition - QPointF(Layer()->GetOffset(), 0);
    case Qt::Vertical:   return m_TargetPosition - QPointF(0, Layer()->GetOffset());
    }
    return m_TargetPosition;
}

void NodeItem::MoveToNext(){
    if(m_Animation){
        m_Animation->stop();
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.rx() += TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH;
            break;
        case Qt::Vertical:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.ry() += TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT;
            break;
        }
        m_Animation->setStartValue(m_Rect);
        m_Animation->setEndValue(QRectF(m_TargetPosition, m_Rect.size()));
        m_Animation->start();
    } else {
        QRectF rect = m_Rect;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            rect.moveLeft(rect.left()+TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH);
            SetRect(rect);
            break;
        case Qt::Vertical:
            rect.moveTop(rect.top()+TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT);
            SetRect(rect);
            break;
        }
    }
    Layer()->SwapWithNext(this);
}

void NodeItem::MoveToPrev(){
    if(m_Animation){
        m_Animation->stop();
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.rx() -= TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH;
            break;
        case Qt::Vertical:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.ry() -= TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT;
            break;
        }
        m_Animation->setStartValue(m_Rect);
        m_Animation->setEndValue(QRectF(m_TargetPosition, m_Rect.size()));
        m_Animation->start();
    } else {
        QRectF rect = m_Rect;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            rect.moveLeft(m_Rect.left()-TREEBAR_DEFAULT_NODE_HORIZONTAL_WIDTH);
            SetRect(rect);
            break;
        case Qt::Vertical:
            rect.setTop(m_Rect.top()-TREEBAR_DEFAULT_NODE_VERTICAL_HEIGHT);
            SetRect(rect);
            break;
        }
    }
    Layer()->SwapWithPrev(this);
}

void NodeItem::ResetTargetPosition(){
    m_TargetPosition = QPointF();
}
