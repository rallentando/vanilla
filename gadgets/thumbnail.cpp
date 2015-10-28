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

void Thumbnail::Initialize(){
    // settings
}

void Thumbnail::SetNest(int nest){
    m_NestLevel = nest;
    qreal opacity = 1.0;
    for(int i = 0; i < nest && opacity >= 0.3; i++)
        opacity *= 0.8;
    setOpacity(opacity);
}

int Thumbnail::GetNest(){
    return m_NestLevel;
}

void Thumbnail::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option); Q_UNUSED(widget);

    if(!m_Node ||
       !boundingRect().translated(pos()).intersects(scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    const QRectF bound = boundingRect();
    const QSize size = bound.size().toSize();
    const QRectF rect = QRectF(bound.topLeft(), bound.size() - QSizeF(1,1));

    QImage image = m_Node->GetImage();
    const QString title = m_Node->GetTitle();
    const QUrl url = m_Node->GetUrl();

    if(image.isNull()){
        Node *tempnode = m_Node;
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    const qreal start = boundingRect().top();
    const qreal stop  = boundingRect().bottom();

    if(m_Node->GetView()){
        QLinearGradient hasviewgrad;
        hasviewgrad.setStart(0, start);
        hasviewgrad.setFinalStop(0, stop);
        hasviewgrad.setColorAt(static_cast<qreal>(0), QColor(255,255,200,0));
        hasviewgrad.setColorAt(static_cast<qreal>(1), QColor(255,255,200,44));
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

    if(THUMBNAIL_DRAW_BORDER){
        painter->setPen(QColor(255,255,255,255));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    const QRect image_rect =
        QRect(boundingRect().topLeft().toPoint() +
              QPoint(THUMBNAIL_PADDING_X,
                     THUMBNAIL_PADDING_Y),
              QSize(size.width()
                    - (THUMBNAIL_PADDING_X * 2),
                    size.height()
                    - (THUMBNAIL_PADDING_Y * 2
                       + THUMBNAIL_TITLE_HEIGHT))
              - QSize(1,1));

    const QRect title_rect =
        QRect(boundingRect().topLeft().toPoint() +
              QPoint(THUMBNAIL_PADDING_X,
                     size.height()
                     - THUMBNAIL_PADDING_Y
                     - THUMBNAIL_TITLE_HEIGHT),
              QSize(size.width()
                    - (THUMBNAIL_PADDING_X * 2),
                    THUMBNAIL_TITLE_HEIGHT));

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        painter->drawImage(QRect(image_rect.topLeft()
                                 + QPoint(width_diff/2, height_diff/2),
                                 size),
                           image,
                           QRect(QPoint(), image.size()));
    } else if(m_Node->IsDirectory()){
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 100, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(50, 100, 120, 150));
        painter->drawRect(image_rect);

        painter->setPen(QColor(255, 255, 255, 255));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    painter->setFont(THUMBNAIL_TITLE_FONT);
    painter->setPen(QPen(QColor(255,255,255,255)));
    painter->setBrush(Qt::NoBrush);

    if(!title.isEmpty())
        painter->drawText(title_rect, Qt::AlignLeft,
                          m_Node->IsDirectory() ?
                          QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first() : title);
    else
        painter->drawText(title_rect, Qt::AlignLeft,
                          !url.isEmpty() ? url.toString() :
                          m_Node->IsDirectory() ? QStringLiteral("Directory") : QStringLiteral("No Title"));
    painter->restore();
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
