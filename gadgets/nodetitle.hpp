#ifndef NODETITLE_HPP
#define NODETITLE_HPP

#include "switch.hpp"

#include <QGraphicsRectItem>

class Node;
class GraphicsTableView;
class QString;

class NodeTitle : public QGraphicsRectItem {

public:
    NodeTitle(Node *nd, int nest, QGraphicsItem *parent = 0);
    ~NodeTitle();
    Node *GetNode();

    static void Initialize();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

    void SetNest(int);
    int GetNest();

    QPointF RealTopLeft()    { return pos() + rect().topLeft();}
    QPointF RealTopRight()   { return pos() + rect().topRight();}
    QPointF RealBottomLeft() { return pos() + rect().bottomLeft();}
    QPointF RealBottomRight(){ return pos() + rect().bottomRight();}

private:
    GraphicsTableView *m_TableView;
    Node *m_Node;
    int m_NestLevel;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
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
