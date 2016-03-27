#ifndef ABSTRACTNODEITEM_HPP_HPP
#define ABSTRACTNODEITEM_HPP_HPP

#include "switch.hpp"

#include <QGraphicsItem>

class Node;
class GraphicsTableView;
class QString;

class AbstractNodeItem : public QGraphicsItem {

public:
    explicit AbstractNodeItem(Node *nd, int nest, QGraphicsItem *parent = 0);
    virtual ~AbstractNodeItem();

    Node *GetNode() const;
    GraphicsTableView *GetTableView() const;
    void SetIndex(int index);

    static void Initialize();

    virtual int GetNest() const;
    virtual void SetNest(int nest);

    virtual bool IsPrimary() const;
    virtual bool IsHovered() const;

    virtual void SetPrimary();
    virtual void SetHovered();
    virtual void SetSelectionRange();

    virtual void ClearOtherSectionSelection();
    virtual void ApplyChildrenOrder(QPointF);

    qreal RealTop()    const { return pos().y() + boundingRect().top();}
    qreal RealBottom() const { return pos().y() + boundingRect().bottom();}
    qreal RealLeft()   const { return pos().x() + boundingRect().left();}
    qreal RealRight()  const { return pos().x() + boundingRect().right();}
    QPointF RealTopLeft()     const { return pos() + boundingRect().topLeft();}
    QPointF RealTopRight()    const { return pos() + boundingRect().topRight();}
    QPointF RealBottomLeft()  const { return pos() + boundingRect().bottomLeft();}
    QPointF RealBottomRight() const { return pos() + boundingRect().bottomRight();}

    void LockRect();
    void UnlockRect();

protected:
    GraphicsTableView *m_TableView;
    Node *m_Node;
    QRectF m_LockedRect;
    int m_Index;
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

#endif //ifndef ABSTRACTNODEITEM_HPP_HPP
