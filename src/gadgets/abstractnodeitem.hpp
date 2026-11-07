#ifndef ABSTRACTNODEITEM_HPP_HPP
#define ABSTRACTNODEITEM_HPP_HPP

#include "switch.hpp"

#include <QGraphicsItem>
#include <QStaticText>
#include <QFont>
#include <optional>

class Node;
class GraphicsTableView;
class QString;

class AbstractNodeItem : public QGraphicsItem {
    friend class tst_gadgetsframe;

public:
    explicit AbstractNodeItem(Node *nd, int nest, QGraphicsItem *parent = nullptr);
    virtual ~AbstractNodeItem() override;

    Node *GetNode() const;
    GraphicsTableView *GetTableView() const;
    void SetIndex(int index);
    int GetIndex() const { return m_Index;}

    const QStaticText &TitleText(const QString &text, const QFont &font, int elideWidth = -1);

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

    void LockRect();
    void UnlockRect();
    void SetTransitionRect(std::optional<QRectF> rect);

protected:
    GraphicsTableView *m_TableView;
    Node *m_Node;
    QRectF m_LockedRect;
    std::optional<QRectF> m_TransitionRect;
    int m_Index;
    int m_NestLevel;
    QStaticText m_TitleText;
    QString m_TitleTextSource;
    QFont m_TitleTextFont;
    int m_TitleTextElideWidth = -1;

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) override;
    void dropEvent             (QGraphicsSceneDragDropEvent *ev) override;
    void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) override;
    void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) override;
    void mousePressEvent       (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) override;
    void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) override;
    void wheelEvent            (QGraphicsSceneWheelEvent *ev) override;
};

#endif
