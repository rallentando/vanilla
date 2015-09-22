#include "switch.hpp"
#include "const.hpp"

#include "nodetitle.hpp"

#include <QtCore>
#include <QGraphicsRectItem>
#include <QString>
#include <QLinearGradient>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneWheelEvent>
#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>

#include "application.hpp"
#include "graphicstableview.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

NodeTitle::NodeTitle(Node *nd, int nest, QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    m_TableView = static_cast<GraphicsTableView*>(parent);
    m_Node = nd;
    SetNest(nest);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setPen(QPen(QColor(0,0,0,0)));
    setBrush(QBrush(QColor(0,0,0,0)));
    setZValue(MAIN_CONTENTS_LAYER);
}

NodeTitle::~NodeTitle(){
}

Node *NodeTitle::GetNode(){
    return m_Node;
}

void NodeTitle::Initialize(){
    // settings
}

void NodeTitle::SetNest(int nest){
    m_NestLevel = nest;
    qreal opacity = 1.0;
    for(int i = 0; i < nest && opacity >= 0.3; i++)
        opacity *= 0.8;
    setOpacity(opacity);
}

int NodeTitle::GetNest(){
    return m_NestLevel;
}

void NodeTitle::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);

    QRectF bound = boundingRect();
    QSize size = bound.size().toSize();
    QRectF rect = m_TableView->NodeTitleAreaRect();

    if(!m_Node || !rect.isValid() ||
       !bound.translated(pos()).intersects(scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    rect = NODE_TITLE_DRAW_BORDER ?
        rect.intersected(QRectF(bound.topLeft(), bound.size() - QSizeF(1,1))):
        QRectF(bound.topLeft(), bound.size() - QSizeF(1,1));

    QString title = m_Node->GetTitle();
    const QUrl url = m_Node->GetUrl();

    const qreal start = boundingRect().top();
    const qreal stop  = boundingRect().bottom();

    if(m_Node->GetView()){
        QLinearGradient hasviewgrad;
        hasviewgrad.setStart(0, start);
        hasviewgrad.setFinalStop(0, stop);
        hasviewgrad.setColorAt(static_cast<qreal>(0), QColor(0,125,100,0));
        hasviewgrad.setColorAt(static_cast<qreal>(1), QColor(0,125,100,77));
        const QBrush hasviewbrush(hasviewgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hasviewbrush);
        painter->drawRect(rect);
    }

    if(m_TableView->IsPrimary(this)){
        QLinearGradient primarygrad;
        primarygrad.setStart(0, start);
        primarygrad.setFinalStop(0, stop);
        primarygrad.setColorAt(static_cast<qreal>(0), QColor(0,100,255,0));
        primarygrad.setColorAt(static_cast<qreal>(1), QColor(0,100,255,77));
        const QBrush primarybrush(primarygrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(primarybrush);
        painter->drawRect(rect);
    }

    if(m_TableView->IsHovered(this)){
        QLinearGradient hoveredgrad;
        hoveredgrad.setStart(0, start);
        hoveredgrad.setFinalStop(0, stop);
        hoveredgrad.setColorAt(static_cast<qreal>(0), QColor(255,255,255,0));
        hoveredgrad.setColorAt(static_cast<qreal>(1), QColor(255,255,255,77));
        const QBrush hoveredbrush(hoveredgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hoveredbrush);
        painter->drawRect(rect);
    }

    if(isSelected()){
        QLinearGradient selectedgrad;
        selectedgrad.setStart(0, start);
        selectedgrad.setFinalStop(0, stop);
        selectedgrad.setColorAt(static_cast<qreal>(0), QColor(255,200,220,0));
        selectedgrad.setColorAt(static_cast<qreal>(1), QColor(255,200,220,170));
        const QBrush selectedbrush(selectedgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(selectedbrush);
        painter->drawRect(rect);
    }

    painter->setFont(NODE_TITLE_FONT);
    painter->setPen(QPen(QColor(255,255,255,255)));
    painter->setBrush(Qt::NoBrush);

    QRectF title_rect = rect.
        intersected(QRectF(boundingRect().topLeft() +
                           QPointF(m_NestLevel * 20 + 5, 0),
                           QSizeF(size.width()-5, NODE_TITLE_HEIGHT)));

    if(title.isEmpty())
        if(url.isEmpty())
            title = m_Node->IsDirectory() ? QStringLiteral("Directory") : QStringLiteral("No Title");
        else
            title = url.toString();

    const QString prefix = m_TableView->GetDirectoryPrefix(m_Node);

    if(!prefix.isEmpty()){
        painter->drawText(title_rect, Qt::AlignLeft, prefix);
        title_rect = QRect(title_rect.x() + 15,
                           title_rect.y(),
                           title_rect.width() - 15,
                           title_rect.height());
    }
    painter->drawText(title_rect, Qt::AlignLeft,
                      m_Node->IsDirectory() ?
                      QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first() : title);
    painter->restore();
}

QVariant NodeTitle::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemPositionChange && scene()){
        QPointF newPos = value.toPointF();
        newPos.setX(0);
        return newPos;
    }
    if(change == ItemSelectedChange && scene()){

        // needless to call 'GraphicsTableView::AppendToSelection'
        // or 'GraphicsTableView::RemoveFromSelection',
        // because want to save selection, even if moving directory.

        if(value.toBool()){
            setZValue(DRAGGING_CONTENTS_LAYER);
        } else {
            setZValue(MAIN_CONTENTS_LAYER);
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}

void NodeTitle::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragEnterEvent(ev);
}

void NodeTitle::dropEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dropEvent(ev);
}

void NodeTitle::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragMoveEvent(ev);
}

void NodeTitle::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragLeaveEvent(ev);
}

void NodeTitle::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    m_TableView->SetInPlaceNotifierContent(0);

    if(ev->button() == Qt::RightButton){
        // for sort menu.
        ev->setAccepted(!(Application::keyboardModifiers() & Qt::ControlModifier));
        return;
    }

    if(ev->button() != Qt::LeftButton){
        // call GraphicsTableView's mouse event.
        QGraphicsRectItem::mousePressEvent(ev);
        return;
    }

    if(Application::keyboardModifiers() & Qt::ControlModifier){
        setSelected(!isSelected());
        if(isSelected()){
            m_TableView->AppendToSelection(m_Node);
        } else {
            m_TableView->RemoveFromSelection(m_Node);
        }
    } else if(Application::keyboardModifiers() & Qt::ShiftModifier){
        m_TableView->SetSelectionRange(this);
    } else {
        m_TableView->ClearThumbnailSelection();
        m_TableView->ClearScrollControllerSelection();
        QGraphicsRectItem::mousePressEvent(ev);
    }
    ev->setAccepted(true);
}

void NodeTitle::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->button() == Qt::RightButton){

        if(GraphicsTableView::RightClickToRenameNode() &&
           m_Node->TitleEditable()){

            ev->setAccepted(m_TableView->ThumbList_RenameNode());

        } else if(QMenu *menu = m_TableView->CreateNodeMenu()){

            menu->exec(ev->screenPos());
            delete menu;
            ev->setAccepted(true);
        }
        return;
    }
    if(ev->button() == Qt::MidButton){
        ev->setAccepted(m_TableView->ThumbList_DeleteNode());
        return;
    }
    if(ev->button() != Qt::LeftButton){
        return;
    }
    if(pos().manhattanLength() > Application::startDragDistance()){

        // needless to call 'update',
        // because
        // 'ApplyChildrenOrder' calls 'SetCurrent'.
        // 'SetCurrent' calls 'CollectNodes'.
        // 'CollectNodes' calls 'SetScrollSoft'.
        // 'SetScrollSoft' calls 'SetScroll'.
        // 'SetScroll' calls 'update()'.

        m_TableView->ThumbList_ApplyChildrenOrder(GraphicsTableView::NodeTitleArea, ev->scenePos());
        ev->setAccepted(true);

    } else if(pos().manhattanLength() != 0){
        foreach(QGraphicsItem *item, scene()->selectedItems()){ item->setPos(QPoint(0,0));}
    } else if(Application::keyboardModifiers() & Qt::ControlModifier){
        /* do nothing.*/
    } else if(Application::keyboardModifiers() & Qt::ShiftModifier){
        /* do nothing.*/
    } else {
        m_TableView->ThumbList_OpenNode();
        ev->setAccepted(true);
    }
}

void NodeTitle::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->buttons() & Qt::RightButton){
        ev->setAccepted(false); return;
    }

    m_TableView->ClearThumbnailSelection();
    m_TableView->ClearScrollControllerSelection();

    QList<QRectF> list;

    if(m_TableView->GetHoveredSpotLight()){
        list << m_TableView->GetHoveredSpotLight()->boundingRect();
    }

    if(m_TableView->GetPrimarySpotLight() &&
       m_TableView->GetHoveredItemIndex() != m_TableView->GetPrimaryItemIndex() &&
       m_TableView->GetPrimaryNodeTitle() &&
       m_TableView->GetPrimaryNodeTitle()->isSelected()){
        list << m_TableView->GetPrimarySpotLight()->boundingRect();
    }

    foreach(SpotLight *light, m_TableView->GetLoadedSpotLights()){
        if(scene()->selectedItems().contains(light)){
            list << light->boundingRect();
        }
    }

    foreach(QGraphicsItem *item, scene()->selectedItems()){
        list << item->boundingRect().translated(item->pos());
    }

    QGraphicsRectItem::mouseMoveEvent(ev);

    if(m_TableView->GetHoveredSpotLight()){
        list << m_TableView->GetHoveredSpotLight()->boundingRect();
    }

    if(m_TableView->GetPrimarySpotLight() &&
       m_TableView->GetHoveredItemIndex() != m_TableView->GetPrimaryItemIndex() &&
       m_TableView->GetPrimaryNodeTitle() &&
       m_TableView->GetPrimaryNodeTitle()->isSelected()){
        list << m_TableView->GetPrimarySpotLight()->boundingRect();
    }

    foreach(SpotLight *light, m_TableView->GetLoadedSpotLights()){
        if(scene()->selectedItems().contains(light)){
            list << light->boundingRect();
        }
    }

    foreach(QGraphicsItem *item, scene()->selectedItems()){
        list << item->boundingRect().translated(item->pos());
    }

    foreach(QRectF rect, list){
        m_TableView->Update(rect);
    }

    ev->setAccepted(true);
}

void NodeTitle::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsRectItem::mouseDoubleClickEvent(ev);
}

void NodeTitle::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverEnterEvent(ev);
    m_TableView->SetHoveredItem(this);
    m_TableView->SetInPlaceNotifierContent(m_Node);
    m_TableView->SetInPlaceNotifierPosition(ev->scenePos());
}

void NodeTitle::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverLeaveEvent(ev);
}

void NodeTitle::hoverMoveEvent (QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverMoveEvent(ev);
    m_TableView->SetHoveredItem(this);
    m_TableView->SetInPlaceNotifierContent(m_Node);
    m_TableView->SetInPlaceNotifierPosition(ev->scenePos());
}

void NodeTitle::wheelEvent(QGraphicsSceneWheelEvent *ev){
    // call GraphicsTableView's wheel event.
    QGraphicsRectItem::wheelEvent(ev);
}
