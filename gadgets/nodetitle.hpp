#ifndef NODETITLE_HPP
#define NODETITLE_HPP

#include "switch.hpp"

#include "abstractnodeitem.hpp"

class Node;
class QString;

class NodeTitle : public AbstractNodeItem {

public:
    NodeTitle(Node *nd, int nest, QGraphicsItem *parent = 0);
    ~NodeTitle();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

    QRectF boundingRect() const DECL_OVERRIDE;

    void SetNest(int) DECL_OVERRIDE;

    bool IsPrimary() DECL_OVERRIDE;
    bool IsHovered() DECL_OVERRIDE;

    void SetPrimary() DECL_OVERRIDE;
    void SetHovered() DECL_OVERRIDE;
    void SetSelectionRange() DECL_OVERRIDE;

    void ClearOtherSectionSelection() DECL_OVERRIDE;
    void ApplyChildrenOrder(QPointF pos) DECL_OVERRIDE;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
};

#endif
