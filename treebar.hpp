#ifndef TREEBAR_HPP
#define TREEBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#include "view.hpp"

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
    static bool EnableCloneButton();
    static bool ScrollToSwitchNode();
    static bool DoubleClickToClose();
    static bool WheelClickToClose();
    static void ToggleEnableAnimation();
    static void ToggleEnableCloseButton();
    static void ToggleEnableCloneButton();
    static void ToggleScrollToSwitchNode();
    static void ToggleDoubleClickToClose();
    static void ToggleWheelClickToClose();

    int GetVerticalNodeWidth();

    void Adjust();

    QSize sizeHint() const DECL_OVERRIDE;
    QSize minimumSizeHint() const DECL_OVERRIDE;

    QList<LayerItem*> &GetLayerList(){ return m_LayerList;}

    QMenu *TreeBarMenu();

public slots:
    void CollectNodes();
    void OnUpdateRequested();
    void OnCurrentChanged(SharedView from, SharedView to);

protected:
    void paintEvent(QPaintEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
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
    QWidget *m_ResizeGrip;
    QSize m_OverrideSize;
    QList<LayerItem*> m_LayerList;
    int m_AutoUpdateTimer;

    static bool m_EnableAnimation;
    static bool m_EnableCloseButton;
    static bool m_EnableCloneButton;
    static bool m_ScrollToSwitchNode;
    static bool m_DoubleClickToClose;
    static bool m_WheelClickToClose;
};

class LayerItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(qreal scroll READ GetScroll WRITE SetScroll NOTIFY ScrollChanged)

public:
    LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, Node *pnd = 0, QGraphicsItem *parent = 0);
    ~LayerItem();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE;
    QRectF boundingRect() const DECL_OVERRIDE;

    int Index() const;

    int GetNest();
    void SetNest(int);

    qreal MaxScroll();
    qreal MinScroll();

    qreal GetScroll();
    void SetScroll(qreal scroll);
    void ScrollDown(qreal step);
    void ScrollUp(qreal step);
    void ResetTargetScroll();
    void AutoScrollDown();
    void AutoScrollUp();
    void AutoScrollStop();
    void AutoScrollStopOrScrollDown(qreal step);
    void AutoScrollStopOrScrollUp(qreal step);

    void StartScrollDownTimer();
    void StartScrollUpTimer();
    void StopScrollDownTimer();
    void StopScrollUpTimer();

    void Adjust();
    void OnScrolled();

    void SetFocusedNode(NodeItem *item);
    void CorrectOrder();

    void SetNode(Node *nd);
    Node *GetNode();

    void SetLine(qreal x1, qreal y1, qreal x2, qreal y2);

    void ApplyChildrenOrder();

    void TransferNodeItem(NodeItem *item, LayerItem *other);

    QList<NodeItem*> &GetNodeItems();
    void AppendToNodeItems(NodeItem *item);
    void PrependToNodeItems(NodeItem *item);
    void RemoveFromNodeItems(NodeItem *item);
    void SwapWithNext(NodeItem *item);
    void SwapWithPrev(NodeItem *item);

    QMenu *LayerMenu();
    QMenu *AddNodeMenu();

signals:
    void ScrollChanged();

protected:
    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
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
    NodeItem *m_FocusedNode;
    int m_Nest;
    int m_ScrollUpTimer;
    int m_ScrollDownTimer;
    qreal m_Scroll;
    qreal m_TargetScroll;
    QGraphicsItem *m_PrevScrollButton;
    QGraphicsItem *m_NextScrollButton;
    QList<NodeItem*> m_NodeItems;
    QGraphicsLineItem *m_Line;
    QPropertyAnimation *m_ScrollAnimation;

    // for empty directory.
    Node *m_DummyNode;
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

    bool GetFocused();
    void SetFocused(bool);

    QRectF GetRect() const;
    void SetRect(QRectF rect);

    LayerItem *Layer() const;
    Node *GetNode();

    QPropertyAnimation *GetAnimation();

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

    QMenu *NodeMenu();

public slots:
    void ResetTargetPosition();

signals:
    void RectChanged();

private:
    enum ButtonState{
        NotHovered,
        CloseHovered,
        ClosePressed,
        CloneHovered,
        ClonePressed,
    } m_ButtonState;

    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    Node *m_Node;
    int m_Nest;
    QRectF m_Rect;
    bool m_IsFocused;
    bool m_IsHovered;
    QPropertyAnimation *m_Animation;
    QPointF m_TargetPosition;
};

#endif
