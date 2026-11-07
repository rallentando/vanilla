#ifndef THUMBNAIL_HPP
#define THUMBNAIL_HPP

#include "switch.hpp"

#include "abstractnodeitem.hpp"

class Node;
class QString;

class Thumbnail : public AbstractNodeItem {

public:
    Thumbnail(Node *nd, int nest, QGraphicsItem *parent = 0);
    ~Thumbnail();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void SetNest(int nest) override;

    void OnSetPrimary(bool primary);

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
