#include "switch.hpp"
#include "const.hpp"

#include "treebar.hpp"

#include "treebank.hpp"
#include "gadgets.hpp"
#include "webengineview.hpp"

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
    * display menu(show/hide MenuBar and TreeBar).
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

  clone button.

  resizable(when vertical).

  TODO: save scroll or refreshment without delete.
    signal candidate.
      * TitleChanged.
        * View::titleChanged.
      * IconChanged.
        * View::iconChanged.
      * CurrentChanged.
        * TreeBank::SetCurrent.
      * TreeStructureChanged.
        * Application::Import.
        * TreeBank::LoadTree.
        * TreeBank::SetChildrenOrder.
        * TreeBank::CreateView.
        * TreeBank::DeleteNode.

  ZValue
   0.0 : layer item layer.
  10.0 : normal node item layer.
  15.0 : border line layer.
  20.0 : primary node item layer.
  30.0 : dragging node layer.
  40.0 : fringe button layer.

 */

#define LAYER_ITEM_LAYER 0.0
#define NORMAL_NODE_LAYER 10.0
#define BORDEF_LINE_LAYER 15.0
#define FOCUSED_NODE_LAYER 20.0
#define DRAGGING_NODE_LAYER 30.0
#define FRINGE_BUTTON_LAYER 40.0
#define SCROLL_INDICATOR_LAYER 50.0

#define FRINGE_BUTTON_SIZE 19

#define TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH 150
#define TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT 23

#define TREEBAR_NODE_VERTICAL_DEFAULT_WIDTH 250
#define TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT 23

bool TreeBar::m_EnableAnimation    = false;
bool TreeBar::m_EnableCloseButton  = false;
bool TreeBar::m_EnableCloneButton  = false;
bool TreeBar::m_ScrollToSwitchNode = false;
bool TreeBar::m_DoubleClickToClose = false;
bool TreeBar::m_WheelClickToClose  = false;

namespace {

    class GraphicsView : public QGraphicsView {
    public:
        GraphicsView(QGraphicsScene *scene, QWidget *parent = 0)
            : QGraphicsView(scene, parent)
        {
        }
        QSize sizeHint() const DECL_OVERRIDE {
            return QSize(1, 1);
        }
        QSize minimumSizeHint() const DECL_OVERRIDE {
            return QSize(1, 1);
        }
    };

    class ResizeGrip : public QWidget {
    public:
        ResizeGrip(QWidget *parent = 0)
            : QWidget(parent)
        {
        }
    protected:
        void paintEvent(QPaintEvent *ev) DECL_OVERRIDE {
            Q_UNUSED(ev);
            QPainter painter(this);
            static const QBrush b = QBrush(QColor(240, 240, 240, 1));
            painter.setPen(Qt::NoPen);
            painter.setBrush(b);
            painter.drawRect(-1, -1, width()+1, height()+1);
        }
        void enterEvent(QEvent *ev){
            QWidget::enterEvent(ev);
            setCursor(Qt::SizeHorCursor);
        }
        void leaveEvent(QEvent *ev){
            QWidget::leaveEvent(ev);
            setCursor(Qt::ArrowCursor);
        }
        void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE {
            QWidget::mouseMoveEvent(ev);
            setCursor(Qt::SizeHorCursor);
            TreeBar *bar = static_cast<TreeBar*>(parentWidget());
            MainWindow *win = static_cast<MainWindow*>(bar->parentWidget());
            switch(win->toolBarArea(bar)){
            case Qt::LeftToolBarArea:
                bar->resize(mapToParent(ev->pos()).x(),
                            bar->height());
                break;
            case Qt::RightToolBarArea:
                bar->resize(bar->width() - mapToParent(ev->pos()).x(),
                            bar->height());
                break;
            }
            bar->Adjust();
        }
        void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE {
            QWidget::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE {
            QWidget::mouseReleaseEvent(ev);
        }
    };

    class GraphicsButton : public QGraphicsItem {
    public:
        GraphicsButton(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : QGraphicsItem(parent)
        {
            m_TreeBank = tb;
            m_TreeBar = bar;
            m_ButtonState = NotHovered;
            setZValue(FRINGE_BUTTON_LAYER);
            setAcceptHoverEvents(true);
        }

    protected:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            Q_UNUSED(ev);
            m_ButtonState = Pressed;
            update();
        }
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            Q_UNUSED(ev);
            m_ButtonState = Hovered;
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

            if(m_ButtonState == Hovered || m_ButtonState == Pressed){
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

            if(ev->button() == Qt::LeftButton){
                if((ev->buttonDownScreenPos(Qt::LeftButton) - ev->screenPos()).manhattanLength() < 4){
                    Gadgets *g = m_TreeBank->GetGadgets();
                    if(g && g->IsActive()) g->Deactivate();
                    else m_TreeBank->DisplayViewTree();
                }
            } else if(ev->button() == Qt::RightButton){
                if((ev->buttonDownScreenPos(Qt::RightButton) - ev->screenPos()).manhattanLength() < 4){
                    QMenu *menu = m_TreeBar->TreeBarMenu();
                    menu->exec(ev->screenPos());
                    delete menu;
                }
            }
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

            if(m_ButtonState == Hovered || m_ButtonState == Pressed){
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
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);

            if(ev->button() == Qt::LeftButton){
                if((ev->buttonDownScreenPos(Qt::LeftButton) - ev->screenPos()).manhattanLength() < 4){
                    QMenu *menu = static_cast<LayerItem*>(parentItem())->AddNodeMenu();
                    menu->exec(ev->screenPos());
                    delete menu;
                }
            } else if(ev->button() == Qt::RightButton){
                if((ev->buttonDownScreenPos(Qt::RightButton) - ev->screenPos()).manhattanLength() < 4){
                    QMenu *menu = static_cast<LayerItem*>(parentItem())->LayerMenu();
                    menu->exec(ev->screenPos());
                    delete menu;
                }
            }
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

        void SetBGBrush(QPainter *painter, QPointF start, QPointF stop){
            m_Gradient.setStart(start);
            m_Gradient.setFinalStop(stop);
            QBrush brush = QBrush(m_Gradient);
            painter->setBrush(brush);
        }
        void SetFGBrush(QPainter *painter, QPointF start, QPointF stop){
            QBrush brush;
            switch(m_ButtonState){
            case NotHovered:
                painter->setBrush(Qt::NoBrush);
                return;
            case Hovered:
                m_HoveredGradient.setStart(start);
                m_HoveredGradient.setFinalStop(stop);
                brush = QBrush(m_HoveredGradient);
                break;
            case Pressed:
                m_PressedGradient.setStart(start);
                m_PressedGradient.setFinalStop(stop);
                brush = QBrush(m_PressedGradient);
                break;
            }
            painter->setBrush(brush);
        }
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
            QRectF rect = boundingRect();
            painter->setPen(Qt::NoPen);
            SetBGBrush(painter, QPointF(rect.left(), 0), QPointF(rect.right(), 0));
            painter->drawRect(rect);
            SetFGBrush(painter, QPointF(rect.left(), 0), QPointF(rect.right(), 0));
            rect.setTop(rect.top() + 4);
            rect.setBottom(rect.bottom() - 6);
            painter->drawRect(rect);

            static QPixmap left;
            if(left.isNull())
                left = QPixmap(":/resources/treebar/left.png");
            painter->drawPixmap
                (QRect(boundingRect().topLeft().toPoint() +
                       QPoint(-1, TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT / 2 - 5),
                       left.size()),
                 left, QRect(QPoint(), left.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
            static_cast<LayerItem*>(parentItem())->StartScrollUpTimer();
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->AutoScrollStopOrScrollUp(TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH);
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
            QRectF rect = boundingRect();
            painter->setPen(Qt::NoPen);
            SetBGBrush(painter, QPointF(rect.right(), 0), QPoint(rect.left(), 0));
            painter->drawRect(rect);
            SetFGBrush(painter, QPointF(rect.right(), 0), QPoint(rect.left(), 0));
            rect.setTop(rect.top() + 4);
            rect.setBottom(rect.bottom() - 6);
            painter->drawRect(rect);

            static QPixmap right;
            if(right.isNull())
                right = QPixmap(":/resources/treebar/right.png");
            painter->drawPixmap
                (QRect(boundingRect().topRight().toPoint() +
                       QPoint(-10, TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT/2 - 5),
                       right.size()),
                 right, QRect(QPoint(), right.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
            static_cast<LayerItem*>(parentItem())->StartScrollDownTimer();
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->AutoScrollStopOrScrollDown(TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH);
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
            QRectF rect = boundingRect();
            painter->setPen(Qt::NoPen);
            SetBGBrush(painter, QPointF(0, rect.top()), QPointF(0, rect.bottom()));
            painter->drawRect(rect);
            SetFGBrush(painter, QPointF(0, rect.top()), QPointF(0, rect.bottom()));
            rect.setLeft(rect.left() + m_TreeBar->GetVerticalNodeWidth() / 2 - 6);
            rect.setRight(rect.right() - m_TreeBar->GetVerticalNodeWidth() / 2 + 4);
            painter->drawRect(rect);

            static QPixmap up;
            if(up.isNull())
                up = QPixmap(":/resources/treebar/up.png");
            painter->drawPixmap
                (QRect(boundingRect().topLeft().toPoint() +
                       QPoint(m_TreeBar->GetVerticalNodeWidth() / 2 - 5, -1),
                       up.size()),
                 up, QRect(QPoint(), up.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
            static_cast<LayerItem*>(parentItem())->StartScrollUpTimer();
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->AutoScrollStopOrScrollUp(TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT * 3.0);
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
            QRectF rect = boundingRect();
            painter->setPen(Qt::NoPen);
            SetBGBrush(painter, QPointF(0, rect.bottom()), QPointF(0, rect.top()));
            painter->drawRect(rect);
            SetFGBrush(painter, QPointF(0, rect.bottom()), QPointF(0, rect.top()));
            rect.setLeft(rect.left() + m_TreeBar->GetVerticalNodeWidth() / 2 - 6);
            rect.setRight(rect.right() - m_TreeBar->GetVerticalNodeWidth() / 2 + 4);
            painter->drawRect(rect);

            static QPixmap down;
            if(down.isNull())
                down = QPixmap(":/resources/treebar/down.png");
            painter->drawPixmap
                (QRect(boundingRect().bottomLeft().toPoint() +
                       QPoint(m_TreeBar->GetVerticalNodeWidth() / 2 - 5, -10),
                       down.size()),
                 down, QRect(QPoint(), down.size()));
        }
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mousePressEvent(ev);
            static_cast<LayerItem*>(parentItem())->StartScrollDownTimer();
        }
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseReleaseEvent(ev);
            static_cast<LayerItem*>(parentItem())->AutoScrollStopOrScrollDown(TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT * 3.0);
        }
    };

    class LayerScroller : public GraphicsButton {
    public:
        LayerScroller(TreeBank *tb, TreeBar *bar, QGraphicsItem *parent = 0)
            : GraphicsButton(tb, bar, parent)
        {
            setZValue(FRINGE_BUTTON_LAYER);
        }

        QRectF boundingRect() const DECL_OVERRIDE {
            LayerItem *layer = static_cast<LayerItem*>(parentItem());
            if(layer->MaxScroll() < 0) return QRectF();

            QRectF rect = layer->boundingRect();
            qreal rate =
                static_cast<qreal>(layer->GetScroll()) /
                static_cast<qreal>(layer->MaxScroll());

            switch(m_TreeBar->orientation()){
            case Qt::Horizontal:
                if(m_ButtonState == NotHovered)
                    rect.setTop(rect.top() + 14);
                else rect.setTop(rect.top() + 11);
                rect.setBottom(rect.bottom() - 1);
                rect.setLeft((rect.width()
                              - FRINGE_BUTTON_SIZE * 2.0 - 20) * rate
                             + FRINGE_BUTTON_SIZE);
                rect.setWidth(20);
                break;
            case Qt::Vertical:
                if(m_ButtonState == NotHovered)
                    rect.setLeft(rect.width() - 12);
                else rect.setLeft(rect.width() - 15);
                rect.setRight(rect.right() - 1);
                rect.setTop((rect.height()
                             - FRINGE_BUTTON_SIZE * 2.0 - 20) * rate
                            + FRINGE_BUTTON_SIZE);
                rect.setHeight(20);
                break;
            }
            return rect;
        }

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE {
            Q_UNUSED(option); Q_UNUSED(widget);
            QRectF rect = boundingRect();
            if(!rect.isValid()) return;
            painter->setPen(Qt::NoPen);
            static const QBrush nb = QBrush(QColor(0, 0, 0, 50));
            static const QBrush hb = QBrush(QColor(0, 0, 0, 80));
            static const QBrush pb = QBrush(QColor(0, 0, 0, 110));
            switch(m_ButtonState){
            case NotHovered: painter->setBrush(nb); break;
            case Hovered:    painter->setBrush(hb); break;
            case Pressed:    painter->setBrush(pb); break;
            }
            painter->drawRect(rect);
        }
    protected:
        void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE {
            GraphicsButton::mouseMoveEvent(ev);
            LayerItem *layer = static_cast<LayerItem*>(parentItem());
            if(layer->MaxScroll() < 0) return;
            qreal rate;
            switch(m_TreeBar->orientation()){
            case Qt::Horizontal:
                rate = (ev->scenePos().x() - FRINGE_BUTTON_SIZE - 10) /
                    (layer->boundingRect().width() - FRINGE_BUTTON_SIZE * 2.0 - 20);
                break;
            case Qt::Vertical:
                rate = (ev->scenePos().y() - FRINGE_BUTTON_SIZE - 10) /
                    (layer->boundingRect().height() - FRINGE_BUTTON_SIZE * 2.0 - 20);
                break;
            }
            layer->SetScroll(layer->MaxScroll() * rate);
            layer->ResetTargetScroll();
        }
        void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE {
            QRectF rect = boundingRect();
            GraphicsButton::hoverLeaveEvent(ev);
            scene()->update(rect);
        }
    };
}

TreeBar::TreeBar(TreeBank *tb, QWidget *parent)
    : QToolBar(tr("TreeBar"), parent)
{
    m_TreeBank = tb;
    m_Scene = new QGraphicsScene(this);
    m_Scene->setSceneRect(QRect(0, 0, width(), height()));
    m_View = new GraphicsView(m_Scene, this);
    m_View->setFrameShape(QGraphicsView::NoFrame);
    m_View->setBackgroundBrush(QColor(240, 240, 240, 255));
    m_ResizeGrip = new ResizeGrip(this);
    m_OverrideSize = QSize();
    m_LayerList = QList<LayerItem*>();
    m_AutoUpdateTimer = 0;
    addWidget(m_View);
    setObjectName(QStringLiteral("TreeBar"));
    setContextMenuPolicy(Qt::PreventContextMenu);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMouseTracking(true);
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
        m_EnableCloneButton  = settings->value(QStringLiteral("@EnableCloneButton"),  false).value<bool>();
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
        settings->setValue(QStringLiteral("@EnableCloneButton"),  m_EnableCloneButton);
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

bool TreeBar::EnableCloneButton(){
    return m_EnableCloneButton;
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

void TreeBar::ToggleEnableAnimation(){
    m_EnableAnimation = !m_EnableAnimation;
}

void TreeBar::ToggleEnableCloseButton(){
    m_EnableCloseButton = !m_EnableCloseButton;
}

void TreeBar::ToggleEnableCloneButton(){
    m_EnableCloneButton = !m_EnableCloneButton;
}

void TreeBar::ToggleScrollToSwitchNode(){
    m_ScrollToSwitchNode = !m_ScrollToSwitchNode;
}

void TreeBar::ToggleDoubleClickToClose(){
    m_DoubleClickToClose = !m_DoubleClickToClose;
}

void TreeBar::ToggleWheelClickToClose(){
    m_WheelClickToClose = !m_WheelClickToClose;
}

int TreeBar::GetVerticalNodeWidth(){
    if(orientation() == Qt::Vertical) return width() - 7;
    return 0;
}

void TreeBar::Adjust(){
    if(orientation() == Qt::Vertical)
        m_OverrideSize = QSize(size().width(), 300);
    updateGeometry();
}

QSize TreeBar::sizeHint() const {
    if(orientation() == Qt::Vertical && m_OverrideSize.isValid()) return m_OverrideSize;
    return QSize(TREEBAR_NODE_VERTICAL_DEFAULT_WIDTH + 7,
                 (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) * m_LayerList.length() + 4);
}

QSize TreeBar::minimumSizeHint() const {
    if(orientation() == Qt::Vertical && m_OverrideSize.isValid()) return m_OverrideSize;
    return QSize(TREEBAR_NODE_VERTICAL_DEFAULT_WIDTH + 7,
                 (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) * m_LayerList.length() + 4);
}

QMenu *TreeBar::TreeBarMenu(){
    QMenu *menu = new QMenu(this);

    menu->addAction(m_TreeBank->Action(TreeBank::Te_ToggleMenuBar));
    menu->addAction(m_TreeBank->Action(TreeBank::Te_ToggleTreeBar));
    menu->addAction(m_TreeBank->Action(TreeBank::Te_ToggleToolBar));

    menu->addSeparator();

    QMenu *settings = new QMenu(tr("TreeBarSettings"), menu);

    QAction *animation = new QAction(settings);
    animation->setText(tr("EnableAnimation"));
    animation->setCheckable(true);
    animation->setChecked(m_EnableAnimation);
    animation->connect(animation, &QAction::triggered,
                       [this](){ m_EnableAnimation = !m_EnableAnimation;});
    settings->addAction(animation);

    QAction *closeButton = new QAction(settings);
    closeButton->setText(tr("EnableCloseButton"));
    closeButton->setCheckable(true);
    closeButton->setChecked(m_EnableCloseButton);
    closeButton->connect(closeButton, &QAction::triggered,
                         [this](){ m_EnableCloseButton = !m_EnableCloseButton;});
    settings->addAction(closeButton);

    QAction *cloneButton = new QAction(menu);
    cloneButton->setText(tr("EnableCloneButton"));
    cloneButton->setCheckable(true);
    cloneButton->setChecked(m_EnableCloneButton);
    cloneButton->connect(cloneButton, &QAction::triggered,
                         [this](){ m_EnableCloneButton = !m_EnableCloneButton;});
    settings->addAction(cloneButton);

    menu->addMenu(settings);

    return menu;
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
            NodeList siblings = layer->GetNode()
                ? layer->GetNode()->GetSiblings()
                : TreeBank::GetViewRoot()->GetChildren();
            for(int j = 0; j < siblings.length(); j++){
                NodeItem *item = new NodeItem(m_TreeBank, this, siblings[j], m_LayerList[i]);
                item->SetRect(QRectF(j * TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH + FRINGE_BUTTON_SIZE,
                                     i * (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) + 1,
                                     TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH,
                                     TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT));
                layer->AppendToNodeItems(item);
            }
        }
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
                                 i * TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT + FRINGE_BUTTON_SIZE,
                                 GetVerticalNodeWidth() - nest * 20,
                                 TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT));
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
        break;
    }
    }

    Adjust();
    foreach(LayerItem *layer, m_LayerList){
        layer->Adjust();
    }
}

void TreeBar::OnUpdateRequested(){
    if(WebEngineView *view = qobject_cast<WebEngineView*>(sender())){
        ViewNode *vn = view->GetViewNode();
        foreach(LayerItem *layer, m_LayerList){
            foreach(NodeItem *item, layer->GetNodeItems()){
                if(item->GetNode() == vn){
                    item->update();
                    return;
                }
            }
        }
    }
}

void TreeBar::OnCurrentChanged(SharedView from, SharedView to){
    Q_UNUSED(from); Q_UNUSED(to);
    // not yet implemented.
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
        height = (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) * m_LayerList.length() + 4;
        m_ResizeGrip->setGeometry(QRect());
        m_ResizeGrip->hide();
    }
    if(orientation() == Qt::Vertical){
        //width = TREEBAR_NODE_VERTICAL_DEFAULT_WIDTH + 7;
        width = ev->size().width();
        height = ev->size().height();
        switch(static_cast<MainWindow*>(parentWidget())->toolBarArea(this)){
        case Qt::LeftToolBarArea:
            m_ResizeGrip->setGeometry(width-4, 0, 4, height);
            break;
        case Qt::RightToolBarArea:
            m_ResizeGrip->setGeometry(0, 0, 4, height);
            break;
        }
        m_ResizeGrip->show();
        m_ResizeGrip->raise();
    }
    QResizeEvent newev = QResizeEvent(QSize(width, height), ev->oldSize());
    QToolBar::resizeEvent(&newev);
    m_Scene->setSceneRect(0.0, 0.0, m_View->width(), m_View->height());
    foreach(LayerItem *layer, m_LayerList){ layer->Adjust();}
}

void TreeBar::timerEvent(QTimerEvent *ev){
    QToolBar::timerEvent(ev);
    if(isVisible() && parentWidget()->isActiveWindow() &&
       m_AutoUpdateTimer && ev->timerId() == m_AutoUpdateTimer)
        m_Scene->update();
}

void TreeBar::showEvent(QShowEvent *ev){
    QToolBar::showEvent(ev);
    connect(m_TreeBank, &TreeBank::TreeStructureChanged,
            this, &TreeBar::CollectNodes);
    connect(this, &QToolBar::orientationChanged,
            this, &TreeBar::CollectNodes);
    if(m_LayerList.isEmpty())
        CollectNodes();
    m_AutoUpdateTimer = startTimer(1000);
}

void TreeBar::hideEvent(QHideEvent *ev){
    QToolBar::hideEvent(ev);
    disconnect(m_TreeBank, &TreeBank::TreeStructureChanged,
               this, &TreeBar::CollectNodes);
    disconnect(this, &QToolBar::orientationChanged,
               this, &TreeBar::CollectNodes);
    m_LayerList.clear();
    m_Scene->clear();
    killTimer(m_AutoUpdateTimer);
    m_AutoUpdateTimer = 0;
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

LayerItem::LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, Node *pnd, QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    m_TreeBank = tb;
    m_TreeBar = bar;
    m_Node = nd;
    m_FocusedNode = 0;
    m_Nest = 0;
    m_ScrollUpTimer = 0;
    m_ScrollDownTimer = 0;
    m_Scroll = 0.0;
    m_TargetScroll = 0.0;
    m_NodeItems = QList<NodeItem*>();

    new PlusButton(tb, bar, this);
    new LayerScroller(tb, bar, this);

    m_PrevScrollButton = 0;
    m_NextScrollButton = 0;

    m_Line = new QGraphicsLineItem(this);
    static const QPen p = QPen(QColor(80, 80, 200, 255));
    m_Line->setPen(p);
    m_Line->setZValue(BORDEF_LINE_LAYER);

    m_ScrollAnimation = new QPropertyAnimation(this, "scroll");

    class DummyNode : public ViewNode{
        bool IsDummy() DECL_OVERRIDE { return true;}
    };

    m_DummyNode = new DummyNode();
    if(!nd) m_Node = m_DummyNode;
    if(pnd) m_DummyNode->SetParent(pnd);
}

LayerItem::~LayerItem(){
    m_DummyNode->Delete();
}

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

qreal LayerItem::MaxScroll(){
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        return TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH * m_NodeItems.length()
            - scene()->sceneRect().width() + FRINGE_BUTTON_SIZE * 2;
    }
    case Qt::Vertical:{
        return TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT * m_NodeItems.length()
            - scene()->sceneRect().height() + FRINGE_BUTTON_SIZE * 2;
    }
    }
    return 0.0;
}

qreal LayerItem::MinScroll(){
    return 0.0;
}

qreal LayerItem::GetScroll(){
    return m_Scroll;
}

void LayerItem::SetScroll(qreal scroll){
    qreal max = MaxScroll();
    qreal min = MinScroll();
    if(max < min) return;

    if(scroll > max) scroll = max;
    if(scroll < min) scroll = min;
    if(m_Scroll == scroll) return;
    m_Scroll = scroll;
    OnScrolled();
    update();
}

void LayerItem::ScrollDown(qreal step){
    qreal max = MaxScroll();
    qreal min = MinScroll();
    if(max < min) return;

    if(TreeBar::EnableAnimation()){

        qreal orig = m_TargetScroll;
        m_TargetScroll += step;
        if(m_TargetScroll > max) m_TargetScroll = max;

        if(m_ScrollAnimation->state() == QAbstractAnimation::Running &&
           m_ScrollAnimation->easingCurve() == QEasingCurve::OutCubic){

            if(m_TargetScroll == orig) return;

            m_ScrollAnimation->stop();
        } else {
            m_ScrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
            m_ScrollAnimation->setDuration(500);
        }
        m_ScrollAnimation->setStartValue(m_Scroll);
        m_ScrollAnimation->setEndValue(m_TargetScroll);
        m_ScrollAnimation->start();
        m_ScrollAnimation->setCurrentTime(16);
    } else {
        m_Scroll+=step;
        if(m_Scroll > max) m_Scroll = max;
        OnScrolled();
        update();
    }
}

void LayerItem::ScrollUp(qreal step){
    qreal max = MaxScroll();
    qreal min = MinScroll();
    if(max < min) return;

    if(TreeBar::EnableAnimation()){

        qreal orig = m_TargetScroll;
        m_TargetScroll -= step;
        if(m_TargetScroll < min) m_TargetScroll = min;

        if(m_ScrollAnimation->state() == QAbstractAnimation::Running &&
           m_ScrollAnimation->easingCurve() == QEasingCurve::OutCubic){

            if(m_TargetScroll == orig) return;

            m_ScrollAnimation->stop();
        } else {
            m_ScrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
            m_ScrollAnimation->setDuration(500);
        }
        m_ScrollAnimation->setStartValue(m_Scroll);
        m_ScrollAnimation->setEndValue(m_TargetScroll);
        m_ScrollAnimation->start();
        m_ScrollAnimation->setCurrentTime(16);
    } else {
        m_Scroll-=step;
        if(m_Scroll < min) m_Scroll = min;
        OnScrolled();
        update();
    }
}

void LayerItem::ResetTargetScroll(){
    m_TargetScroll = m_Scroll;
}

void LayerItem::AutoScrollDown(){
    if(MaxScroll() <= 0.0 ||
       GetScroll() == MaxScroll() ||
       (m_ScrollAnimation->state() == QAbstractAnimation::Running &&
        m_ScrollAnimation->easingCurve() == QEasingCurve::Linear))
        return;

    m_ScrollAnimation->setEasingCurve(QEasingCurve::Linear);
    m_ScrollAnimation->setStartValue(GetScroll());
    m_ScrollAnimation->setEndValue(MaxScroll());
    m_ScrollAnimation->setDuration((MaxScroll() - GetScroll()) * 2);
    m_ScrollAnimation->start();
}

void LayerItem::AutoScrollUp(){
    if(MaxScroll() <= 0.0 ||
       GetScroll() == MinScroll() ||
       (m_ScrollAnimation->state() == QAbstractAnimation::Running &&
        m_ScrollAnimation->easingCurve() == QEasingCurve::Linear))
        return;

    m_ScrollAnimation->setEasingCurve(QEasingCurve::Linear);
    m_ScrollAnimation->setStartValue(GetScroll());
    m_ScrollAnimation->setEndValue(MinScroll());
    m_ScrollAnimation->setDuration((GetScroll() - MinScroll()) * 2);
    m_ScrollAnimation->start();
}

void LayerItem::AutoScrollStop(){
    m_ScrollAnimation->stop();
    m_TargetScroll = m_Scroll;
    StopScrollDownTimer();
    StopScrollUpTimer();
}

void LayerItem::AutoScrollStopOrScrollDown(qreal step){
    StopScrollDownTimer();

    if(m_ScrollAnimation->state() == QAbstractAnimation::Running){
        m_ScrollAnimation->stop();
        m_TargetScroll = m_Scroll;
    }
    else ScrollDown(step);
}

void LayerItem::AutoScrollStopOrScrollUp(qreal step){
    StopScrollUpTimer();

    if(m_ScrollAnimation->state() == QAbstractAnimation::Running){
        m_ScrollAnimation->stop();
        m_TargetScroll = m_Scroll;
    }
    else ScrollUp(step);
}

void LayerItem::StartScrollDownTimer(){
    if(!m_ScrollDownTimer){
        m_ScrollDownTimer = startTimer(200);
    }
}

void LayerItem::StartScrollUpTimer(){
    if(!m_ScrollUpTimer){
        m_ScrollUpTimer = startTimer(200);
    }
}

void LayerItem::StopScrollDownTimer(){
    if(m_ScrollDownTimer){
        killTimer(m_ScrollDownTimer);
        m_ScrollDownTimer = 0;
    }
}

void LayerItem::StopScrollUpTimer(){
    if(m_ScrollUpTimer){
        killTimer(m_ScrollUpTimer);
        m_ScrollUpTimer = 0;
    }
}

void LayerItem::Adjust(){
    int i = 0;
    if(m_Node && !m_Node->IsDummy()){
        for(; i < m_NodeItems.length(); i++){
            if(m_NodeItems[i]->GetNode() == m_Node) break;
        }
    }
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{

        SetScroll(TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH * (i + 0.5)
                  - scene()->sceneRect().width() / 2.0 + FRINGE_BUTTON_SIZE);
        ResetTargetScroll();

        int width = boundingRect().width();
        SetLine(0.0,   (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) * (Index() + 1) - 2,
                width, (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) * (Index() + 1) - 2);
        break;
    }
    case Qt::Vertical:{
        foreach(NodeItem *item, m_NodeItems){
            QRectF rect = item->GetRect();
            rect.setRight(m_TreeBar->GetVerticalNodeWidth() + 1);
            item->SetRect(rect);
        }

        SetScroll(TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT * (i + 0.5)
                  - scene()->sceneRect().height() / 2.0 + FRINGE_BUTTON_SIZE);
        ResetTargetScroll();

        int height = boundingRect().height();
        SetLine(m_TreeBar->GetVerticalNodeWidth() + 1, 0,
                m_TreeBar->GetVerticalNodeWidth() + 1, height);
        break;
    }
    }
}

void LayerItem::OnScrolled(){
    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        if(MaxScroll() >= 0 && m_Scroll > MinScroll()){
            if(!m_PrevScrollButton)
                m_PrevScrollButton = new LeftScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_PrevScrollButton){
            scene()->removeItem(m_PrevScrollButton);
            m_PrevScrollButton = 0;
        }
        if(MaxScroll() >= 0 && m_Scroll < MaxScroll()){
            if(!m_NextScrollButton)
                m_NextScrollButton = new RightScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_NextScrollButton){
            scene()->removeItem(m_NextScrollButton);
            m_NextScrollButton = 0;
        }
        break;
    }
    case Qt::Vertical:{
        if(MaxScroll() >= 0 && m_Scroll > MinScroll()){
            if(!m_PrevScrollButton)
                m_PrevScrollButton = new UpScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_PrevScrollButton){
            scene()->removeItem(m_PrevScrollButton);
            m_PrevScrollButton = 0;
        }
        if(MaxScroll() >= 0 && m_Scroll < MaxScroll()){
            if(!m_NextScrollButton)
                m_NextScrollButton = new DownScrollButton(m_TreeBank, m_TreeBar, this);
        } else if(m_NextScrollButton){
            scene()->removeItem(m_NextScrollButton);
            m_NextScrollButton = 0;
        }
        break;
    }
    }
    CorrectOrder();
}

void LayerItem::SetFocusedNode(NodeItem *item){
    m_FocusedNode = item;
}

void LayerItem::CorrectOrder(){
    if(!m_FocusedNode ||
       m_NodeItems.length() < 2) return;

    int i = m_NodeItems.indexOf(m_FocusedNode);
    bool moved = false;

    static const int halfWidth  = TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH / 2.0;
    static const int halfHeight = TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT  / 2.0;

    switch(m_TreeBar->orientation()){
    case Qt::Horizontal:{
        while(i+1 < m_NodeItems.length() &&
              halfWidth
              >= (m_NodeItems[i+1]->ScheduledPosition().x() -
                  m_NodeItems[i]->ScheduledPosition().x())){
            moved = true;
            m_NodeItems[i+1]->MoveToPrev();
            i++;
        }
        if(moved) break;
        while(i > 0 &&
              halfWidth
              >= (m_NodeItems[i]->ScheduledPosition().x() -
                  m_NodeItems[i-1]->ScheduledPosition().x())){
            m_NodeItems[i-1]->MoveToNext();
            i--;
        }
        break;
    }
    case Qt::Vertical:{
        while(i+1 < m_NodeItems.length() &&
              halfHeight
              >= (m_NodeItems[i+1]->ScheduledPosition().y() -
                  m_NodeItems[i]->ScheduledPosition().y())){
            moved = true;

            int nest = m_NodeItems[i+1]->GetNode()->IsDirectory()
                ? m_NodeItems[i+1]->GetNest()+1
                : m_NodeItems[i+1]->GetNest();

            if(m_NodeItems[i]->GetNest() != nest){

                m_NodeItems[i]->SetNest(nest);

                QRectF rect = m_NodeItems[i]->GetRect();

                if(TreeBar::EnableAnimation()){
                    m_NodeItems[i]->GetAnimation()->stop();
                    m_NodeItems[i]->GetAnimation()->setStartValue(rect);
                    rect.setLeft(1+m_NodeItems[i]->GetNest()*20);
                    m_NodeItems[i]->GetAnimation()->setEndValue(rect);
                    m_NodeItems[i]->GetAnimation()->start();
                } else {
                    rect.setLeft(1+m_NodeItems[i]->GetNest()*20);
                    m_NodeItems[i]->SetRect(rect);
                }
            }
            m_NodeItems[i+1]->MoveToPrev();
            i++;
        }
        if(moved) break;
        while(i > 0 &&
              halfHeight
              >= (m_NodeItems[i]->ScheduledPosition().y() -
                  m_NodeItems[i-1]->ScheduledPosition().y())){

            int nest = m_NodeItems[i-1]->GetNest();

            if(m_NodeItems[i]->GetNest() != nest){

                m_NodeItems[i]->SetNest(nest);

                QRectF rect = m_NodeItems[i]->GetRect();

                if(TreeBar::EnableAnimation()){
                    m_NodeItems[i]->GetAnimation()->stop();
                    m_NodeItems[i]->GetAnimation()->setStartValue(rect);
                    rect.setLeft(1+m_NodeItems[i]->GetNest()*20);
                    m_NodeItems[i]->GetAnimation()->setEndValue(rect);
                    m_NodeItems[i]->GetAnimation()->start();
                } else {
                    rect.setLeft(1+m_NodeItems[i]->GetNest()*20);
                    m_NodeItems[i]->SetRect(rect);
                }
            }
            m_NodeItems[i-1]->MoveToNext();
            i--;
        }
        break;
    }
    }
}

void LayerItem::SetNode(Node *nd){
    m_Node = nd;
}

Node *LayerItem::GetNode(){
    return (!m_Node || m_Node->IsDummy()) ? 0 : m_Node;
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
    other->m_FocusedNode = m_FocusedNode;
    m_FocusedNode = 0;
    m_NodeItems.removeOne(item);
    item->setParentItem(other);
    other->m_NodeItems.append(item);
    QRectF rect = item->GetRect();
    rect.moveTop(other->Index() * 26 + 1);
    item->SetRect(rect);
}

QList<NodeItem*> &LayerItem::GetNodeItems(){
    return m_NodeItems;
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

QMenu *LayerItem::LayerMenu(){
    QMenu *menu = new QMenu(m_TreeBar);

    TreeBank *tb = m_TreeBank;
    Node *nd = GetNode();
    Node *pnd = m_DummyNode->GetParent();

    QAction *newViewNode = new QAction(menu);
    newViewNode->setText(QObject::tr("NewViewNode"));
    newViewNode->connect(newViewNode, &QAction::triggered,
                         [tb, nd, pnd](){
                             if(nd) tb->NewViewNode(nd->ToViewNode());
                             else if(pnd) tb->OpenOnSuitableNode(QUrl("about:blank"), true, pnd->ToViewNode());
                             else tb->OpenOnSuitableNode(QUrl("about:blank"), true);
                         });
    menu->addAction(newViewNode);

    QAction *cloneViewNode = new QAction(menu);
    cloneViewNode->setText(QObject::tr("CloneViewNode"));
    cloneViewNode->connect(cloneViewNode, &QAction::triggered,
                           [tb, nd](){ if(nd) tb->CloneViewNode(nd->ToViewNode());});
    menu->addAction(cloneViewNode);

    menu->addSeparator();

    QAction *makeDirectory = new QAction(menu);
    makeDirectory->setText(QObject::tr("MakeDirectory"));
    makeDirectory->connect(makeDirectory, &QAction::triggered,
                           [tb, nd, pnd](){
                               if(nd) tb->MakeSiblingDirectory(nd->ToViewNode());
                               else if(pnd) tb->MakeChildDirectory(pnd->ToViewNode());
                               else tb->MakeChildDirectory(TreeBank::GetViewRoot());
                           });
    menu->addAction(makeDirectory);
    QAction *makeDirectoryWithSelected = new QAction(menu);
    makeDirectoryWithSelected->setText(QObject::tr("MakeDirectoryWithSelectedNode"));
    makeDirectoryWithSelected->connect
        (makeDirectoryWithSelected, &QAction::triggered,
         [tb, nd](){
            if(!nd) return;
            Node *parent = tb->MakeSiblingDirectory(nd->ToViewNode());
            tb->SetChildrenOrder(parent, NodeList() << nd);
        });
    menu->addAction(makeDirectoryWithSelected);
    QAction *makeDirectoryWithSameDomain = new QAction(menu);
    makeDirectoryWithSameDomain->setText(QObject::tr("MakeDirectoryWithSameDomainNode"));
    makeDirectoryWithSameDomain->connect
        (makeDirectoryWithSameDomain, &QAction::triggered,
         [tb, nd, pnd](){
            ViewNode *parent =
                nd ? nd->GetParent()->ToViewNode() :
                pnd ? pnd->ToViewNode() : 0;
            if(!parent) return;
            QMap<QString, QList<Node*>> groups;
            foreach(Node *n, parent->GetChildren()){
                if(!n->IsDirectory()) groups[n->GetUrl().host()] << n;
            }
            if(groups.count() <= 1) return;
            foreach(QString domain, groups.keys()){
                Node *directory = parent->MakeChild();
                directory->SetTitle(domain);
                tb->SetChildrenOrder(directory, groups[domain]);
            }
        });
    menu->addAction(makeDirectoryWithSameDomain);

    menu->addSeparator();

    menu->addAction(tb->Action(TreeBank::Te_ToggleMenuBar));
    menu->addAction(tb->Action(TreeBank::Te_ToggleTreeBar));
    menu->addAction(tb->Action(TreeBank::Te_ToggleToolBar));

    menu->addSeparator();

    QMenu *settings = new QMenu(tr("TreeBarSettings"), menu);

    QAction *animation = new QAction(settings);
    animation->setText(tr("EnableAnimation"));
    animation->setCheckable(true);
    animation->setChecked(TreeBar::EnableAnimation());
    animation->connect(animation, &QAction::triggered,
                       [this](){ TreeBar::ToggleEnableAnimation();});
    settings->addAction(animation);

    QAction *closeButton = new QAction(settings);
    closeButton->setText(tr("EnableCloseButton"));
    closeButton->setCheckable(true);
    closeButton->setChecked(TreeBar::EnableCloseButton());
    closeButton->connect(closeButton, &QAction::triggered,
                         [this](){ TreeBar::ToggleEnableCloseButton();});
    settings->addAction(closeButton);

    QAction *cloneButton = new QAction(menu);
    cloneButton->setText(tr("EnableCloneButton"));
    cloneButton->setCheckable(true);
    cloneButton->setChecked(TreeBar::EnableCloneButton());
    cloneButton->connect(cloneButton, &QAction::triggered,
                         [this](){ TreeBar::ToggleEnableCloneButton();});
    settings->addAction(cloneButton);

    menu->addMenu(settings);

    return menu;
}

QMenu *LayerItem::AddNodeMenu(){
    QMenu *menu = new QMenu(m_TreeBar);

    TreeBank *tb = m_TreeBank;
    Node *nd = GetNode();
    Node *pnd = m_DummyNode->GetParent();

    QAction *newViewNode = new QAction(menu);
    newViewNode->setText(QObject::tr("NewViewNode"));
    newViewNode->connect(newViewNode, &QAction::triggered,
                         [tb, nd, pnd](){
                             if(nd) tb->NewViewNode(nd->ToViewNode());
                             else if(pnd) tb->OpenOnSuitableNode(QUrl("about:blank"), true, pnd->ToViewNode());
                             else tb->OpenOnSuitableNode(QUrl("about:blank"), true);
                         });
    menu->addAction(newViewNode);

    QAction *cloneViewNode = new QAction(menu);
    cloneViewNode->setText(QObject::tr("CloneViewNode"));
    cloneViewNode->connect(cloneViewNode, &QAction::triggered,
                           [tb, nd](){ if(nd) tb->CloneViewNode(nd->ToViewNode());});
    menu->addAction(cloneViewNode);

    NodeList trash = TreeBank::GetTrashRoot()->GetChildren();

    QMenu *restoreMenu = menu;

    if(trash.length())
        restoreMenu->addSeparator();

    int max = 20;
    int i = 0;
    int j = 0;

    for(; j < 10; j++, max+=20){
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
                 [t, nd, pnd, tb](){
                    // both function emits TreeStructureChanged for current.
                    if(nd)
                        tb->MoveNode(t, nd->GetParent(), nd->SiblingsIndexOf(nd) + 1);
                    else if(pnd)
                        tb->MoveNode(t, pnd);
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

    return menu;
}

void LayerItem::timerEvent(QTimerEvent *ev){
    QGraphicsObject::timerEvent(ev);
    if(ev->timerId() == m_ScrollDownTimer){
        killTimer(m_ScrollDownTimer);
        m_ScrollDownTimer = 0;
        AutoScrollDown();
    }
    if(ev->timerId() == m_ScrollUpTimer){
        killTimer(m_ScrollUpTimer);
        m_ScrollUpTimer = 0;
        AutoScrollUp();
    }
}

void LayerItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    Q_UNUSED(ev);
    //QGraphicsObject::mousePressEvent(ev);
}

void LayerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    //QGraphicsObject::mouseReleaseEvent(ev);

    if(ev->button() == Qt::RightButton &&
       (ev->buttonDownScreenPos(Qt::RightButton) - ev->screenPos()).manhattanLength() < 4){
        QMenu *menu = LayerMenu();
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
            if(up) ScrollUp(TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH);
            else   ScrollDown(TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH);
            break;
        case Qt::Vertical:
            if(up) ScrollUp(TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT * 3.0);
            else   ScrollDown(TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT * 3.0);
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
    m_IsFocused = static_cast<LayerItem*>(parent)->GetNode() == nd;
    m_IsHovered = false;
    m_ButtonState = NotHovered;
    m_TargetPosition = QPoint();
    m_Animation = new QPropertyAnimation(this, "rect");
    m_Animation->setDuration(300);
    m_Animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_Animation, &QPropertyAnimation::finished,
            this, &NodeItem::ResetTargetPosition);

    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    if(m_IsFocused)
        setZValue(FOCUSED_NODE_LAYER);
    else
        setZValue(NORMAL_NODE_LAYER);
}

NodeItem::~NodeItem(){}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);

    painter->save();

    QRectF bound = boundingRect();
    QRect title_rect = bound.toRect();
    title_rect.setLeft(title_rect.left() + 4);
    title_rect.setTop(title_rect.top() + 1);
    title_rect.setWidth(title_rect.width()
                        - ((!m_IsHovered ||
                            (!m_TreeBar->EnableCloseButton() &&
                             !m_TreeBar->EnableCloneButton())) ? 4 :
                           (m_TreeBar->EnableCloseButton() !=
                            m_TreeBar->EnableCloneButton()) ? 21 : 39));

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

    if(View *view = m_Node->GetView()){
        QIcon icon = view->GetIcon();
        if(!icon.isNull()){
            QPixmap pixmap = icon.pixmap(QSize(16, 16));
            painter->drawPixmap(QRect(title_rect.topLeft() + QPoint(0, 3), QSize(16, 16)),
                                pixmap, QRect(QPoint(), pixmap.size()));
            title_rect.setLeft(title_rect.left() + 20);
        }
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

    if(m_IsFocused){
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

    if(TreeBar::EnableCloneButton()){
        if(m_ButtonState == CloneHovered || m_ButtonState == ClonePressed){
            static const QBrush h = QBrush(QColor(180, 180, 180, 255));
            static const QBrush p = QBrush(QColor(150, 150, 150, 255));
            painter->setBrush(m_ButtonState == CloneHovered ? h : p);
            painter->setPen(Qt::NoPen);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawRoundedRect(QRectF(bound.topRight()
                                            + QPointF(TreeBar::EnableCloseButton() ? -36 : -18, 4),
                                            QSizeF(14, 14)), 2, 2);
        }

        static QPixmap clone;
        if(clone.isNull())
            clone = QPixmap(":/resources/treebar/clone.png");

        if(m_IsHovered)
            painter->drawPixmap(QRect(bound.topRight().toPoint()
                                      + QPoint(TreeBar::EnableCloseButton() ? -34 : -16, 6),
                                      clone.size()),
                                clone, QRect(QPoint(), clone.size()));
    }

    if(TreeBar::EnableCloseButton()){
        if(m_ButtonState == CloseHovered || m_ButtonState == ClosePressed){
            static const QBrush h = QBrush(QColor(180, 180, 180, 255));
            static const QBrush p = QBrush(QColor(150, 150, 150, 255));
            painter->setBrush(m_ButtonState == CloseHovered ? h : p);
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
        rect.moveLeft(rect.left() - static_cast<int>(Layer()->GetScroll()));
        return rect;
    case Qt::Vertical:
        rect.moveTop(rect.top() - static_cast<int>(Layer()->GetScroll()));
        return rect;
    }
    return m_Rect;
}

void NodeItem::SetRect(QRectF rect){
    m_Rect = rect;
    Layer()->update();
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

bool NodeItem::GetFocused(){
    return m_IsFocused;
}

void NodeItem::SetFocused(bool focused){
    m_IsFocused = focused;
}

Node *NodeItem::GetNode(){
    return m_Node;
}

QPropertyAnimation *NodeItem::GetAnimation(){
    return m_Animation;
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemPositionChange && scene()){
        QPointF newPos = value.toPointF();
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:{
            int min = -boundingRect().left() + FRINGE_BUTTON_SIZE;
            int max = -boundingRect().left() - FRINGE_BUTTON_SIZE
                + scene()->sceneRect().width() - TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH;
            if(newPos.x() < min) newPos.setX(min);
            if(newPos.x() > max) newPos.setX(max);
            newPos.setY(0);
            break;
        }
        case Qt::Vertical:{
            int min = -boundingRect().top() + FRINGE_BUTTON_SIZE;
            int max = -boundingRect().top() - FRINGE_BUTTON_SIZE
                + scene()->sceneRect().height() - TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT;
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
            setZValue(DRAGGING_NODE_LAYER);
        } else if(m_IsFocused){
            setZValue(FOCUSED_NODE_LAYER);
        } else {
            setZValue(NORMAL_NODE_LAYER);
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){

    if(ev->button() == Qt::LeftButton){

        Layer()->SetFocusedNode(this);

        if(TreeBar::EnableCloseButton() &&
           QRectF(boundingRect().topRight() + QPointF(-18, 4),
                  QSizeF(14, 14)).contains(ev->pos())){
            m_ButtonState = ClosePressed;
            update();
        }
        if(TreeBar::EnableCloneButton() &&
           QRectF(boundingRect().topRight()
                  + QPointF(TreeBar::EnableCloseButton() ? -36 : -18, 4),
                  QSizeF(14, 14)).contains(ev->pos())){
            m_ButtonState = ClonePressed;
            update();
        }

        // m_Rect is used for boundingRect.
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            m_Rect.moveLeft(m_Rect.left() - static_cast<int>(Layer()->GetScroll()));
            break;
        case Qt::Vertical:
            m_Rect.moveTop(m_Rect.top() - static_cast<int>(Layer()->GetScroll()));
            break;
        }
    }
    QGraphicsObject::mousePressEvent(ev);
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){

        Layer()->AutoScrollStop();
        Layer()->SetFocusedNode(0);

        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            m_Rect.moveLeft(m_Rect.left() + static_cast<int>(Layer()->GetScroll()));
            break;
        case Qt::Vertical:
            m_Rect.moveTop(m_Rect.top() + static_cast<int>(Layer()->GetScroll()));
            break;
        }

        if((ev->buttonDownScreenPos(Qt::LeftButton) - ev->screenPos()).manhattanLength() < 4){
            if(m_ButtonState == ClosePressed){
                m_TreeBank->DeleteNode(m_Node);
            } else if(m_ButtonState == ClonePressed){
                m_TreeBank->CloneViewNode(m_Node->ToViewNode());
            } else {
                if(m_TreeBar->orientation() == Qt::Vertical && m_Node->IsDirectory()){
                    m_Node->SetFolded(!m_Node->GetFolded());
                    m_TreeBar->CollectNodes();
                    return;
                }
                if(!m_TreeBank->SetCurrent(m_Node)){

                    setSelected(false);

                    if(m_Node->IsDirectory()){
                        QList<LayerItem*> &layers = m_TreeBar->GetLayerList();
                        int index = layers.indexOf(Layer());
                        int i = 0;
                        foreach(LayerItem *layer, layers){
                            if(i > index){
                                layers.removeOne(layer);
                                layer->deleteLater();
                            }
                            i++;
                        }
                        Layer()->SetNode(GetNode());
                        foreach(NodeItem *item, Layer()->GetNodeItems()){
                            if(item == this){
                                item->SetFocused(true);
                                item->setZValue(FOCUSED_NODE_LAYER);
                            } else {
                                item->SetFocused(false);
                                item->setZValue(NORMAL_NODE_LAYER);
                            }
                        }
                        LayerItem *layer = new LayerItem(m_TreeBank, m_TreeBar, 0, m_Node);
                        layers.append(layer);
                        scene()->addItem(layer);
                        NodeList list = m_Node->GetChildren();
                        i = index + 1;
                        for(int j = 0; j < list.length(); j++){
                            NodeItem *item = new NodeItem(m_TreeBank, m_TreeBar, list[j], layer);
                            item->SetRect(QRectF(j * TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH + FRINGE_BUTTON_SIZE,
                                                 i * (TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT + 3) + 1,
                                                 TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH,
                                                 TREEBAR_NODE_HORIZONTAL_DEFAULT_HEIGHT));
                            layer->AppendToNodeItems(item);
                        }
                        m_TreeBar->Adjust();
                    }
                }
            }
            return;
        }

        Layer()->ApplyChildrenOrder();

    } else if(ev->button() == Qt::RightButton){
        if((ev->buttonDownScreenPos(Qt::RightButton) - ev->screenPos()).manhattanLength() < 4){
            QMenu *menu = NodeMenu();
            menu->exec(ev->screenPos());
            delete menu;
            return;
        }
    } else if(ev->button() == Qt::MidButton &&
              TreeBar::WheelClickToClose()){
        if((ev->buttonDownScreenPos(Qt::MidButton) - ev->screenPos()).manhattanLength() < 4){
            m_TreeBank->DeleteNode(m_Node);
            return;
        }
    }
    // some time cause crash.
    //QGraphicsObject::mouseReleaseEvent(ev);
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){

    if(m_ButtonState == ClosePressed || m_ButtonState == ClonePressed) return;

    QGraphicsObject::mouseMoveEvent(ev);

    LayerItem *newLayer = 0;

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
        if(ev->scenePos().x() < scene()->sceneRect().left() + FRINGE_BUTTON_SIZE)
            Layer()->AutoScrollUp();
        else if(ev->scenePos().x() > scene()->sceneRect().right() - FRINGE_BUTTON_SIZE)
            Layer()->AutoScrollDown();
        else
            Layer()->AutoScrollStop();
        break;
    case Qt::Vertical:
        if(ev->scenePos().y() < scene()->sceneRect().top() + FRINGE_BUTTON_SIZE)
            Layer()->AutoScrollUp();
        else if(ev->scenePos().y() > scene()->sceneRect().bottom() - FRINGE_BUTTON_SIZE)
            Layer()->AutoScrollDown();
        else
            Layer()->AutoScrollStop();
        break;
    }

    Layer()->CorrectOrder();
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
        m_ButtonState = CloseHovered;
        update();
    }
    if(TreeBar::EnableCloneButton() &&
       QRectF(boundingRect().topRight()
              + QPointF(TreeBar::EnableCloseButton() ? -36 : -18, 4),
              QSizeF(14, 14)).contains(ev->pos())){
        m_ButtonState = CloneHovered;
        update();
    }

    QGraphicsObject::hoverEnterEvent(ev);
}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    m_IsHovered = false;
    if(m_ButtonState != NotHovered){
        m_ButtonState = NotHovered;
        update();
    }
    QGraphicsObject::hoverLeaveEvent(ev);
}

void NodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    m_IsHovered = true;
    if(TreeBar::EnableCloseButton() &&
       QRectF(boundingRect().topRight() + QPointF(-18, 4),
              QSizeF(14, 14)).contains(ev->pos())){
        if(m_ButtonState == NotHovered){
            m_ButtonState = CloseHovered;
            update();
        }
    } else if(TreeBar::EnableCloneButton() &&
       QRectF(boundingRect().topRight()
              + QPointF(TreeBar::EnableCloseButton() ? -36 : -18, 4),
              QSizeF(14, 14)).contains(ev->pos())){
        if(m_ButtonState == NotHovered){
            m_ButtonState = CloneHovered;
            update();
        }
    } else {
        if(m_ButtonState != NotHovered){
            m_ButtonState = NotHovered;
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
    case Qt::Horizontal: return m_TargetPosition - QPointF(Layer()->GetScroll(), 0);
    case Qt::Vertical:   return m_TargetPosition - QPointF(0, Layer()->GetScroll());
    }
    return m_TargetPosition;
}

void NodeItem::MoveToNext(){
    if(TreeBar::EnableAnimation()){
        m_Animation->stop();
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.rx() += TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH;
            break;
        case Qt::Vertical:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.ry() += TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT;
            break;
        }
        m_Animation->setStartValue(m_Rect);
        m_Animation->setEndValue(QRectF(m_TargetPosition, m_Rect.size()));
        m_Animation->start();
    } else {
        QRectF rect = m_Rect;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            rect.moveLeft(rect.left()+TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH);
            SetRect(rect);
            break;
        case Qt::Vertical:
            rect.moveTop(rect.top()+TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT);
            SetRect(rect);
            break;
        }
    }
    Layer()->SwapWithNext(this);
}

void NodeItem::MoveToPrev(){
    if(TreeBar::EnableAnimation()){
        m_Animation->stop();
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.rx() -= TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH;
            break;
        case Qt::Vertical:
            if(m_TargetPosition.isNull()) m_TargetPosition = m_Rect.topLeft();
            m_TargetPosition.ry() -= TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT;
            break;
        }
        m_Animation->setStartValue(m_Rect);
        m_Animation->setEndValue(QRectF(m_TargetPosition, m_Rect.size()));
        m_Animation->start();
    } else {
        QRectF rect = m_Rect;
        switch(m_TreeBar->orientation()){
        case Qt::Horizontal:
            rect.moveLeft(m_Rect.left()-TREEBAR_NODE_HORIZONTAL_DEFAULT_WIDTH);
            SetRect(rect);
            break;
        case Qt::Vertical:
            rect.setTop(m_Rect.top()-TREEBAR_NODE_VERTICAL_DEFAULT_HEIGHT);
            SetRect(rect);
            break;
        }
    }
    Layer()->SwapWithPrev(this);
}

QMenu *NodeItem::NodeMenu(){
    QMenu *menu = new QMenu(m_TreeBar);

    TreeBank *tb = m_TreeBank;
    ViewNode *vn = GetNode() ? GetNode()->ToViewNode() : 0;
    if(!vn) return menu;

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

        if(!tb->IsCurrent(vn)){
            QAction *openOnNewWindow = new QAction(menu);
            openOnNewWindow->setText(QObject::tr("OpenViewNodeOnNewWindow"));
            openOnNewWindow->connect(openOnNewWindow, &QAction::triggered,
                                     [tb, vn](){
                                         MainWindow *win = tb->NewWindow();
                                         win->GetTreeBank()->SetCurrent(vn);
                                     });
            menu->addAction(openOnNewWindow);
        }
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

    return menu;
}

void NodeItem::ResetTargetPosition(){
    m_TargetPosition = QPointF();
}
