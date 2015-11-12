#include "switch.hpp"
#include "const.hpp"

#include "nodetitle.hpp"

#include "graphicstableview.hpp"

NodeTitle::NodeTitle(Node *nd, int nest, QGraphicsItem *parent)
    : AbstractNodeItem(nd, nest, parent)
{
}

NodeTitle::~NodeTitle(){
}

void NodeTitle::SetNest(int nest){
    m_NestLevel = nest;
    m_TableView->GetStyle()->OnSetNest(this, nest);
}

bool NodeTitle::IsPrimary(){
    return m_TableView->IsPrimary(this);
}

bool NodeTitle::IsHovered(){
    return m_TableView->IsHovered(this);
}

void NodeTitle::SetPrimary(){
    m_TableView->SetPrimaryItem(this);
}

void NodeTitle::SetHovered(){
    m_TableView->SetHoveredItem(this);
}

void NodeTitle::SetSelectionRange(){
    m_TableView->SetSelectionRange(this);
}

void NodeTitle::ClearOtherSectionSelection(){
    m_TableView->ClearThumbnailSelection();
}

void NodeTitle::ApplyChildrenOrder(QPointF pos){
    m_TableView->ThumbList_ApplyChildrenOrder(GraphicsTableView::NodeTitleArea, pos);
}

void NodeTitle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);
    m_TableView->GetStyle()->Render(this, painter);
}

QVariant NodeTitle::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemPositionChange && scene()){
        QPointF newPos = value.toPointF();
        newPos.setX(0);
        return newPos;
    }
    return AbstractNodeItem::itemChange(change, value);
}
