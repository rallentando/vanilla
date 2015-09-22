#ifndef ACCESSIBLEWEBELEMNT_HPP
#define ACCESSIBLEWEBELEMNT_HPP

#include "switch.hpp"

#include <QPoint>
#include <QGraphicsItem>

#include "view.hpp"

class Gadgets;

class AccessibleWebElement : public QGraphicsItem{

public:
    AccessibleWebElement(QGraphicsItem *parent = 0);
    ~AccessibleWebElement();

    static void Initialize();

    QRectF boundingRect() const DECL_OVERRIDE;
    QPainterPath shape() const DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

    void SetIndex(int index);
    void SetBoundingPos(QPoint pos);
    void SetElement(SharedWebElement elem);

    int GetIndex() const;
    QPoint GetBoundingPos() const;
    SharedWebElement GetElement() const;
    QPoint KeyExplanationBasePos() const;
    QMap<QString, QRect> KeyRects() const;
    QMap<QString, QRect> ExpRects() const;

    bool IsCurrentBlock() const;
    bool IsSelected() const;

    QRect CharChipRect() const;
    QFont CharChipFont() const;

    void UpdateMinimal();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;

private:
    static QFontMetrics m_InfoMetrics;
    static QFontMetrics m_SmallMetrics;
    static QFontMetrics m_MediumMetrics;
    static QFontMetrics m_LargeMetrics;

    Gadgets *m_Gadgets;
    int m_Index;
    SharedWebElement m_Element;
    QPoint m_Pos;
};

#endif
