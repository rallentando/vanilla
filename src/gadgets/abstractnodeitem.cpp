#include "switch.hpp"
#include "const.hpp"

#include "abstractnodeitem.hpp"

#include <QtCore>
#include <QGraphicsItem>
#include <QString>
#include <QLinearGradient>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsDropShadowEffect>
#include <QtWidgets>

#include "application.hpp"
#include "graphicstableview.hpp"

#include "lightnode.hpp"

namespace {
    bool NearlyEqual(const qreal a, const qreal b){
        return qFabs(a - b) < 0.001;
    }
}

AbstractNodeItem::AbstractNodeItem(Node *nd, int nest, QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    m_TableView = static_cast<GraphicsTableView*>(parent);
    m_Node = nd;
    m_LockedRect = QRectF();
    m_Index = -1;
    SetNest(nest);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setZValue(MAIN_CONTENTS_LAYER);
}

AbstractNodeItem::~AbstractNodeItem(){
}

Node *AbstractNodeItem::GetNode() const {
    return m_Node;
}

GraphicsTableView *AbstractNodeItem::GetTableView() const {
    return m_TableView;
}

void AbstractNodeItem::SetIndex(int index){
    m_Index = index;
}

const QStaticText &AbstractNodeItem::TitleText(const QString &text, const QFont &font){
    if(m_TitleText.text() != text || m_TitleTextFont != font){
        m_TitleText.setTextFormat(Qt::PlainText);
        QTextOption option;
        option.setWrapMode(QTextOption::NoWrap);
        m_TitleText.setTextOption(option);
        m_TitleText.setPerformanceHint(QStaticText::AggressiveCaching);
        m_TitleText.setText(text);
        m_TitleTextFont = font;
        m_TitleText.prepare(QTransform(), font);
    }
    return m_TitleText;
}

void AbstractNodeItem::SetNest(int nest){
    m_NestLevel = nest;
}

int AbstractNodeItem::GetNest() const {
    return m_NestLevel;
}

bool AbstractNodeItem::IsPrimary() const {
    return false;
}

bool AbstractNodeItem::IsHovered() const {
    return false;
}

void AbstractNodeItem::SetPrimary(){
}

void AbstractNodeItem::SetHovered(){
}

void AbstractNodeItem::SetSelectionRange(){
}

void AbstractNodeItem::ClearOtherSectionSelection(){
}

void AbstractNodeItem::ApplyChildrenOrder(QPointF pos){
    Q_UNUSED(pos)
}

void AbstractNodeItem::LockRect(){
    m_LockedRect = boundingRect();
}

void AbstractNodeItem::UnlockRect(){
    m_LockedRect = QRectF();
}

QVariant AbstractNodeItem::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemSelectedChange && scene()){

        if(value.toBool()){
            setZValue(DRAGGING_CONTENTS_LAYER);
        } else {
            setZValue(MAIN_CONTENTS_LAYER);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void AbstractNodeItem::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsItem::dragEnterEvent(ev);
}

void AbstractNodeItem::dropEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsItem::dropEvent(ev);
}

void AbstractNodeItem::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsItem::dragMoveEvent(ev);
}

void AbstractNodeItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    QGraphicsItem::dragLeaveEvent(ev);
}

void AbstractNodeItem::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    m_TableView->SetInPlaceNotifierContent(nullptr);

    if(ev->button() == Qt::RightButton){
        ev->setAccepted(!Application::HasCtrlModifier(ev));
        return;
    }

    if(ev->button() != Qt::LeftButton){
        QGraphicsItem::mousePressEvent(ev);
        return;
    }

    if(Application::HasCtrlModifier(ev)){
        setSelected(!isSelected());
        if(isSelected()){
            m_TableView->AppendToSelection(m_Node);
        } else {
            m_TableView->RemoveFromSelection(m_Node);
        }
    } else if(Application::HasShiftModifier(ev)){
        SetSelectionRange();
    } else {
        ClearOtherSectionSelection();
        m_TableView->ClearScrollIndicatorSelection();
        QGraphicsItem::mousePressEvent(ev);
    }
    SetHovered();
    ev->setAccepted(true);
}

void AbstractNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
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
    if(
       ev->button() == Qt::MiddleButton
       ){
        ev->setAccepted(m_TableView->ThumbList_DeleteNode());
        return;
    }
    if(ev->button() != Qt::LeftButton){
        return;
    }
    if(pos().manhattanLength() > Application::startDragDistance()){

        ApplyChildrenOrder(ev->scenePos());
        ev->setAccepted(true);
        return;
    }

    if(!NearlyEqual(pos().manhattanLength(), 0)){
        foreach(QGraphicsItem *item, scene()->selectedItems()){ item->setPos(QPoint(0,0));}
    }

    if(Application::HasCtrlModifier(ev)){
    } else if(Application::HasShiftModifier(ev)){
    } else {
        m_TableView->ThumbList_OpenNode();
        ev->setAccepted(true);
    }
}

void AbstractNodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    if(ev->buttons() & Qt::RightButton){
        ev->setAccepted(false); return;
    }

    ClearOtherSectionSelection();
    m_TableView->ClearScrollIndicatorSelection();

    QList<QRectF> list;

    if(m_TableView->GetHoveredSpotLight()){
        list << m_TableView->GetHoveredSpotLight()->boundingRect();
    }

    if(m_TableView->GetPrimarySpotLight() &&
       m_TableView->GetHoveredItemIndex() != m_TableView->GetPrimaryItemIndex() &&
       ((m_TableView->GetPrimaryThumbnail() &&
         m_TableView->GetPrimaryThumbnail()->isSelected()) ||
        (m_TableView->GetPrimaryNodeTitle() &&
         m_TableView->GetPrimaryNodeTitle()->isSelected()))){
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

    QGraphicsItem::mouseMoveEvent(ev);

    if(m_TableView->GetHoveredSpotLight()){
        list << m_TableView->GetHoveredSpotLight()->boundingRect();
    }

    if(m_TableView->GetPrimarySpotLight() &&
       m_TableView->GetHoveredItemIndex() != m_TableView->GetPrimaryItemIndex() &&
       ((m_TableView->GetPrimaryThumbnail() &&
         m_TableView->GetPrimaryThumbnail()->isSelected()) ||
        (m_TableView->GetPrimaryNodeTitle() &&
         m_TableView->GetPrimaryNodeTitle()->isSelected()))){
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
        m_TableView->update(rect);
    }

    ev->setAccepted(true);
}

void AbstractNodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsItem::mouseDoubleClickEvent(ev);
}

void AbstractNodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsItem::hoverEnterEvent(ev);
    SetHovered();
    foreach(QGraphicsItem *item, scene()->items(ev->scenePos())){
        if(dynamic_cast<GraphicsButton*>(item)) return;
    }
    m_TableView->SetInPlaceNotifierContent(m_Node);
    m_TableView->SetInPlaceNotifierPosition(ev->scenePos());
}

void AbstractNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsItem::hoverLeaveEvent(ev);
}

void AbstractNodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    QGraphicsItem::hoverMoveEvent(ev);
    SetHovered();
    foreach(QGraphicsItem *item, scene()->items(ev->scenePos())){
        if(dynamic_cast<GraphicsButton*>(item)) return;
    }
    m_TableView->SetInPlaceNotifierContent(m_Node);
    m_TableView->SetInPlaceNotifierPosition(ev->scenePos());
}

void AbstractNodeItem::wheelEvent(QGraphicsSceneWheelEvent *ev){
    QGraphicsItem::wheelEvent(ev);
}
