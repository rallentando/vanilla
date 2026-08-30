#ifndef ACCESSIBLEWEBELEMNT_HPP
#define ACCESSIBLEWEBELEMNT_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QPoint>
#include <QGraphicsItem>
#include <QFontMetrics>

#include "view.hpp"

class Gadgets;

class AccessibleWebElement : public QGraphicsItem {

public:
    AccessibleWebElement(QGraphicsItem *parent = nullptr);
    ~AccessibleWebElement() Q_DECL_OVERRIDE;

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QPainterPath shape() const Q_DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) Q_DECL_OVERRIDE;

    void SetIndex(int index);
    void SetBoundingPos(QPoint pos);
    void SetElement(SharedWebElement elem);

    int GetIndex() const;
    QPoint GetBoundingPos() const;
    SharedWebElement GetElement() const;
    QPoint KeyExplanationBasePos() const;
    QMap<QString, QRect> KeyRects() const;
    QMap<QString, QRect> ExpRects() const;

    Gadgets *GetGadgets(){ return m_Gadgets;}

    static const QFontMetrics &GetInfoMetrics(){
        static const QFontMetrics metrics(AccessKeyInfoFont());
        return metrics;
    }
    static const QFontMetrics &GetSmallMetrics(){
        static const QFontMetrics metrics(AccessKeyChipSFont());
        return metrics;
    }
    static const QFontMetrics &GetMediumMetrics(){
        static const QFontMetrics metrics(AccessKeyChipMFont());
        return metrics;
    }
    static const QFontMetrics &GetLargeMetrics(){
        static const QFontMetrics metrics(AccessKeyChipLFont());
        return metrics;
    }

    bool IsCurrentBlock() const;
    bool IsSelected() const;

    QRect CharChipRect() const;
    QFont CharChipFont() const;

    void UpdateMinimal();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;

private:
    Gadgets *m_Gadgets;
    int m_Index;
    SharedWebElement m_Element;
    QPoint m_Pos;
};

#endif
