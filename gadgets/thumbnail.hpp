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

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

    QRectF boundingRect() const DECL_OVERRIDE;

    void SetNest(int nest) DECL_OVERRIDE;

    void OnSetPrimary(bool primary);

    bool IsPrimary() const DECL_OVERRIDE;
    bool IsHovered() const DECL_OVERRIDE;

    void SetPrimary() DECL_OVERRIDE;
    void SetHovered() DECL_OVERRIDE;
    void SetSelectionRange() DECL_OVERRIDE;

    void ClearOtherSectionSelection() DECL_OVERRIDE;
    void ApplyChildrenOrder(QPointF pos) DECL_OVERRIDE;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
};

#endif
