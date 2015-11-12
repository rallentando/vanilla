#include "switch.hpp"
#include "const.hpp"

#include "thumbnail.hpp"

#include "graphicstableview.hpp"

Thumbnail::Thumbnail(Node *nd, int nest, QGraphicsItem *parent)
    : AbstractNodeItem(nd, nest, parent)
{
}

Thumbnail::~Thumbnail(){
}

void Thumbnail::SetNest(int nest){
    AbstractNodeItem::SetNest(nest);
    m_TableView->GetStyle()->OnSetNest(this, nest);
}

void Thumbnail::OnSetPrimary(bool primary){
    m_TableView->GetStyle()->OnSetPrimary(this, primary);
}

bool Thumbnail::IsPrimary(){
    return m_TableView->IsPrimary(this);
}

bool Thumbnail::IsHovered(){
    return m_TableView->IsHovered(this);
}

void Thumbnail::SetPrimary(){
    m_TableView->SetPrimaryItem(this);
}

void Thumbnail::SetHovered(){
    m_TableView->SetHoveredItem(this);
}

void Thumbnail::SetSelectionRange(){
    m_TableView->SetSelectionRange(this);
}

void Thumbnail::ClearOtherSectionSelection(){
    m_TableView->ClearNodeTitleSelection();
}

void Thumbnail::ApplyChildrenOrder(QPointF pos){
    m_TableView->ThumbList_ApplyChildrenOrder(GraphicsTableView::ThumbnailArea, pos);
}

void Thumbnail::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);
    m_TableView->GetStyle()->Render(this, painter);
}
