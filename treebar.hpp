#ifndef TREEBAR_HPP
#define TREEBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QToolBar>
#include <QGraphicsObject>

class Node;
class TreeBank;
class NodeItem;
class LayerItem;
class QPaintEvent;
class QResizeEvent;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsLineItem;
class QPropertyAnimation;

class TreeBar : public QToolBar{
    Q_OBJECT

public:
    TreeBar(TreeBank *tb, QWidget *parent = 0);
    ~TreeBar();

    static void Initialize();

    static void LoadSettings();
    static void SaveSettings();

    static bool EnableAnimation();
    static bool EnableCloseButton();
    static bool ScrollToSwitchNode();
    static bool DoubleClickToClose();
    static bool WheelClickToClose();

    QSize sizeHint() const DECL_OVERRIDE;
    QSize minimumSizeHint() const DECL_OVERRIDE;

    QList<LayerItem*> &GetLayerList(){ return m_LayerList;}

public slots:
    void CollectNodes();

protected:
    void paintEvent(QPaintEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;

    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void enterEvent(QEvent *ev) DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;

private:
    TreeBank *m_TreeBank;
    QGraphicsView *m_View;
    QGraphicsScene *m_Scene;
    QList<LayerItem*> m_LayerList;

    static bool m_EnableAnimation;
    static bool m_EnableCloseButton;
    static bool m_ScrollToSwitchNode;
    static bool m_DoubleClickToClose;
    static bool m_WheelClickToClose;
};

class LayerItem : public QGraphicsObject {
    Q_OBJECT

public:
    LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, QGraphicsItem *parent = 0);
    ~LayerItem();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE;
    QRectF boundingRect() const DECL_OVERRIDE;

    int Index() const;

    int GetNest();
    void SetNest(int);

    int MaxOffset();
    int MinOffset();

    int GetOffset();
    void SetOffset(int);
    void PlusOffset(int step);
    void MinusOffset(int step);

    void OnResized();
    void OnScrolled();

    QList<NodeItem*> &GetNodeItems(){ return m_NodeItems;}

    Node *GetNode();

    void SetLine(qreal x1, qreal y1, qreal x2, qreal y2);

    void ApplyChildrenOrder();

    void TransferNodeItem(NodeItem *item, LayerItem *other);

    void AppendToNodeItems(NodeItem *item);
    void PrependToNodeItems(NodeItem *item);
    void RemoveFromNodeItems(NodeItem *item);
    void SwapWithNext(NodeItem *item);
    void SwapWithPrev(NodeItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;

private:
    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    Node *m_Node;
    int m_Nest;
    int m_Offset;
    QGraphicsItem *m_PrevScrollButton;
    QGraphicsItem *m_NextScrollButton;
    QList<NodeItem*> m_NodeItems;
    QGraphicsLineItem *m_Line;
};

class NodeItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(QRectF rect READ GetRect WRITE SetRect NOTIFY RectChanged)

public:
    NodeItem(TreeBank *tb, TreeBar *bar, Node *nd, QGraphicsItem *parent = 0);
    ~NodeItem();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE;
    QRectF boundingRect() const DECL_OVERRIDE;

    int GetNest();
    void SetNest(int);

    QRectF GetRect() const;
    void SetRect(QRectF rect);

    LayerItem *Layer() const;
    Node *GetNode();

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;

    void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
    QPointF ScheduledPosition();
    void MoveToNext();
    void MoveToPrev();

public slots:
    void ResetTargetPosition();

signals:
    void RectChanged();

private:
    enum CloseButtonState{
        NotHovered,
        Hovered,
        Pressed,
    } m_CloseButtonState;

    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    Node *m_Node;
    int m_Nest;
    QRectF m_Rect;
    bool m_IsPrimary;
    bool m_IsHovered;
    QPropertyAnimation *m_Animation;
    QPointF m_TargetPosition;
};

#endif
