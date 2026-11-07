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
    ~AccessibleWebElement() override;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

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

    static QRectF CapCenteredRect(const QRectF &rect, const QFont &font){
        const QFontMetricsF metrics(font);
        return rect.translated(0, (metrics.capHeight() - metrics.ascent()
                                   + metrics.descent()) / 2.0);
    }

    bool IsCurrentBlock() const;
    bool IsSelected() const;

    QRect CharChipRect() const;
    QFont CharChipFont() const;

    void UpdateMinimal();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) override;

private:
    Gadgets *m_Gadgets;
    int m_Index;
    SharedWebElement m_Element;
    QPoint m_Pos;
};

#endif
