#ifndef ABSTRACTNODEITEM_HPP_HPP
#define ABSTRACTNODEITEM_HPP_HPP

#include "switch.hpp"

#include <QGraphicsRectItem>

class Node;
class GraphicsTableView;
class QString;

class AbstractNodeItem : public QGraphicsRectItem {

public:
    explicit AbstractNodeItem(Node *nd, int nest, QGraphicsItem *parent = 0);
    virtual ~AbstractNodeItem();

    Node *GetNode();
    GraphicsTableView *GetTableView();

    static void Initialize();

    virtual int GetNest();
    virtual void SetNest(int nest);

    virtual bool IsPrimary();
    virtual bool IsHovered();

    virtual void SetPrimary();
    virtual void SetHovered();
    virtual void SetSelectionRange();

    virtual void ClearOtherSectionSelection();
    virtual void ApplyChildrenOrder(QPointF);

    QPointF RealTopLeft()    { return pos() + rect().topLeft();}
    QPointF RealTopRight()   { return pos() + rect().topRight();}
    QPointF RealBottomLeft() { return pos() + rect().bottomLeft();}
    QPointF RealBottomRight(){ return pos() + rect().bottomRight();}

protected:
    GraphicsTableView *m_TableView;
    Node *m_Node;
    int m_NestLevel;

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
    void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dropEvent             (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void mousePressEvent       (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void wheelEvent            (QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
};

#endif
