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

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    QRectF boundingRect() const override;

    void SetNest(int) override;

    bool IsPrimary() const override;
    bool IsHovered() const override;

    void SetPrimary() override;
    void SetHovered() override;
    void SetSelectionRange() override;

    void ClearOtherSectionSelection() override;
    void ApplyChildrenOrder(QPointF pos) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

#endif
