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
    static QFontMetrics GetInfoMetrics(){ return m_InfoMetrics;}
    static QFontMetrics GetSmallMetrics(){ return m_SmallMetrics;}
    static QFontMetrics GetMediumMetrics(){ return m_MediumMetrics;}
    static QFontMetrics GetLargeMetrics(){ return m_LargeMetrics;}

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
    static QFontMetrics m_InfoMetrics;
    static QFontMetrics m_SmallMetrics;
    static QFontMetrics m_MediumMetrics;
    static QFontMetrics m_LargeMetrics;

    Gadgets *m_Gadgets;
    int m_Index;
    SharedWebElement m_Element;
    QPoint m_Pos;
};

#endif //ifndef ACCESSIBLEWEBELEMNT_HPP
