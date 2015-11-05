#include "switch.hpp"
#include "const.hpp"

#include "thumbnail.hpp"

#include <QtCore>
#include <QGraphicsRectItem>
#include <QString>
#include <QLinearGradient>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsDropShadowEffect>
#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>

#include "application.hpp"
#include "graphicstableview.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

Thumbnail::Thumbnail(Node *nd, int nest, QGraphicsItem *parent)
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

Thumbnail::~Thumbnail(){
}

Node *Thumbnail::GetNode(){
    return m_Node;
}

GraphicsTableView *Thumbnail::GetTableView(){
    return m_TableView;
}

void Thumbnail::Initialize(){
    // settings
}

void Thumbnail::SetNest(int nest){
    m_NestLevel = nest;
    m_TableView->GetStyle()->OnSetNest(this, nest);
}

int Thumbnail::GetNest(){
    return m_NestLevel;
}

void Thumbnail::SetPrimary(bool primary){
    m_TableView->GetStyle()->OnSetPrimary(this, primary);
}

bool Thumbnail::IsPrimary(){
    return m_TableView->IsPrimary(this);
}

bool Thumbnail::IsHovered(){
    return m_TableView->IsHovered(this);
}

void Thumbnail::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);
    m_TableView->GetStyle()->Render(this, painter);
}

QVariant Thumbnail::itemChange(GraphicsItemChange change, const QVariant &value){
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

void Thumbnail::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragEnterEvent(ev);
}

void Thumbnail::dropEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dropEvent(ev);
}

void Thumbnail::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragMoveEvent(ev);
}

void Thumbnail::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsRectItem::dragLeaveEvent(ev);
}

void Thumbnail::mousePressEvent(QGraphicsSceneMouseEvent *ev){
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
        m_TableView->ClearNodeTitleSelection();
        m_TableView->ClearScrollControllerSelection();
        QGraphicsRectItem::mousePressEvent(ev);
    }
    ev->setAccepted(true);
}

void Thumbnail::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
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

        m_TableView->ThumbList_ApplyChildrenOrder(GraphicsTableView::ThumbnailArea, ev->scenePos());
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

void Thumbnail::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->buttons() & Qt::RightButton){
        ev->setAccepted(false); return;
    }

    m_TableView->ClearNodeTitleSelection();
    m_TableView->ClearScrollControllerSelection();

    QList<QRectF> list;

    if(m_TableView->GetHoveredSpotLight()){
        list << m_TableView->GetHoveredSpotLight()->boundingRect();
    }

    if(m_TableView->GetPrimarySpotLight() &&
       m_TableView->GetHoveredItemIndex() != m_TableView->GetPrimaryItemIndex() &&
       m_TableView->GetPrimaryThumbnail() &&
       m_TableView->GetPrimaryThumbnail()->isSelected()){
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
       m_TableView->GetPrimaryThumbnail() &&
       m_TableView->GetPrimaryThumbnail()->isSelected()){
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

void Thumbnail::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsRectItem::mouseDoubleClickEvent(ev);
}

void Thumbnail::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverEnterEvent(ev);
    m_TableView->SetHoveredItem(this);
    m_TableView->SetInPlaceNotifierContent(m_Node);
    m_TableView->SetInPlaceNotifierPosition(ev->scenePos());
}

void Thumbnail::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverLeaveEvent(ev);
}

void Thumbnail::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsRectItem::hoverMoveEvent(ev);
    m_TableView->SetHoveredItem(this);
    m_TableView->SetInPlaceNotifierContent(m_Node);
    m_TableView->SetInPlaceNotifierPosition(ev->scenePos());
}

void Thumbnail::wheelEvent(QGraphicsSceneWheelEvent *ev){
    // call GraphicsTableView's wheel event.
    QGraphicsRectItem::wheelEvent(ev);
}
