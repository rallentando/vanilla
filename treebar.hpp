#ifndef TREEBAR_HPP
#define TREEBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

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

    enum LastAction{
        None,
        TreeStructureChanged,
        NodeCreated,
        NodeDeleted,
        FoldedChanged,
        CurrentChanged,
    };

    static void Initialize();

    static void LoadSettings();
    static void SaveSettings();

    static int HorizontalNodeWidth();
    static int VerticalNodeHeight();

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

    int GetHorizontalNodeHeight() const;
    int GetVerticalNodeWidth() const;

    int MaxWidth() const;
    int MinWidth() const;
    int MaxHeight() const;
    int MinHeight() const;

    void SetStat(QStringList);
    QStringList GetStat() const;

    void Adjust();

    void ClearLowerLayer(int index);

    QSize sizeHint() const DECL_OVERRIDE;
    QSize minimumSizeHint() const DECL_OVERRIDE;

    QList<LayerItem*> &GetLayerList(){ return m_LayerList;}

    QMenu *TreeBarMenu();

public slots:
    void CollectNodes();
    void OnTreeStructureChanged();
    void OnNodeCreated(NodeList &nds);
    void OnNodeDeleted(NodeList &nds);
    void OnFoldedChanged(NodeList &nds);
    void OnCurrentChanged(Node *nd);

    void StartAutoUpdateTimer();
    void StopAutoUpdateTimer();
    void RestartAutoUpdateTimer();

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
    int m_AutoUpdateTimerID;
    int m_HorizontalNodeHeight;
    int m_VerticalNodeWidth;
    LastAction m_LastAction;

    static int m_HorizontalNodeWidth;
    static int m_VerticalNodeHeight;

    static bool m_EnableAnimation;
    static bool m_EnableCloseButton;
    static bool m_EnableCloneButton;
    static bool m_ScrollToSwitchNode;
    static bool m_DoubleClickToClose;
    static bool m_WheelClickToClose;
};

class LayerItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(qreal scroll READ GetScroll WRITE SetScroll)

public:
    LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, Node *pnd = 0, QGraphicsItem *parent = 0);
    ~LayerItem();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE;
    QRectF boundingRect() const DECL_OVERRIDE;

    int Index() const;

    int GetNest() const;
    void SetNest(int);

    QPropertyAnimation *GetAnimation() const;

    qreal MaxScroll() const;
    qreal MinScroll() const;
    qreal GetScroll() const;
    void SetScroll(qreal scroll);
    void Scroll(qreal delta);
    void ScrollForDelete(int count);
    void ResetTargetScroll();
    void AutoScrollDown();
    void AutoScrollUp();
    void AutoScrollStop();
    void AutoScrollStopOrScroll(qreal delta);

    void StartScrollDownTimer();
    void StartScrollUpTimer();
    void StopScrollDownTimer();
    void StopScrollUpTimer();

    void Adjust();
    void OnScrolled();

    NodeItem *GetFocusedNode() const;
    void SetFocusedNode(NodeItem *item);
    void CorrectOrder();

    void SetNode(Node *nd);
    Node *GetNode() const;

    void SetLine(qreal x1, qreal y1, qreal x2, qreal y2);

    void ApplyChildrenOrder();

    void TransferNodeItem(NodeItem *item, LayerItem *other);

    NodeItem *CreateNodeItem(Node *nd, int i, int j, int nest, int size);
    QList<NodeItem*> &GetNodeItems();
    void AppendToNodeItems(NodeItem *item);
    void PrependToNodeItems(NodeItem *item);
    void RemoveFromNodeItems(NodeItem *item);
    void SwapWithNext(int index);
    void SwapWithPrev(int index);

    QMenu *LayerMenu();
    QMenu *AddNodeMenu();

public slots:
    void NewViewNode();
    void CloneViewNode();
    void MakeDirectory();
    void MakeDirectoryWithSelectedNode();
    void MakeDirectoryWithSameDomainNode();
    void ToggleEnableAnimation();
    void ToggleEnableCloseButton();
    void ToggleEnableCloneButton();
    void DisplayTrashTree();

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
    int m_ScrollUpTimerID;
    int m_ScrollDownTimerID;
    qreal m_CurrentScroll;
    qreal m_TargetScroll;
    QGraphicsItem *m_PrevScrollButton;
    QGraphicsItem *m_NextScrollButton;
    QList<NodeItem*> m_NodeItems;
    QGraphicsLineItem *m_Line;
    QPropertyAnimation *m_Animation;

    // for empty directory.
    Node *m_DummyNode;
};

class NodeItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(QRectF rect READ GetRect WRITE SetRect)

public:
    NodeItem(TreeBank *tb, TreeBar *bar, Node *nd, QGraphicsItem *parent = 0);
    ~NodeItem();

    enum ButtonState{
        NotHovered,
        CloseHovered,
        ClosePressed,
        CloneHovered,
        ClonePressed,
#if QT_VERSION >= 0x050700
        SoundHovered,
        SoundPressed,
#endif
    } m_ButtonState;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) DECL_OVERRIDE;
    QRectF boundingRect() const DECL_OVERRIDE;
    QRectF CloseButtonRect() const;
    QRectF CloneButtonRect() const;
#if QT_VERSION >= 0x050700
    QRectF SoundButtonRect() const;
#endif
    QRect CloseIconRect() const;
    QRect CloneIconRect() const;
#if QT_VERSION >= 0x050700
    QRect SoundIconRect() const;
#endif

    int GetNest() const;
    void SetNest(int);

    bool GetFocused() const;
    void SetFocused(bool);

    QRectF GetRect() const;
    void SetRect(QRectF rect);

    QPointF GetTargetPosition() const;
    void SetTargetPosition(QPointF pos);

    LayerItem *Layer() const;
    Node *GetNode() const;

    QPropertyAnimation *GetAnimation() const;

    void SetButtonState(ButtonState state);
    void SetHoveredWithItem(bool hovered);

    void UnfoldDirectory();

    void OnCreated(QRectF target, QRectF start = QRectF());
    void OnDeleted(QRectF target, QRectF start = QRectF());
    void OnNestChanged();
    void Slide(int step);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
    QPointF ScheduledPosition();

    QMenu *NodeMenu();

public slots:
    void NewViewNode();
    void CloneViewNode();
    void RenameViewNode();
    void ReloadViewNode();
    void OpenViewNode();
    void OpenViewNodeOnNewWindow();
    void DeleteViewNode();
    void DeleteRightViewNode();
    void DeleteLeftViewNode();
    void DeleteOtherViewNode();
    void MakeDirectory();
    void MakeDirectoryWithSelectedNode();
    void MakeDirectoryWithSameDomainNode();

    void OpenViewNodeWithIE();
    void OpenViewNodeWithEdge();
    void OpenViewNodeWithFF();
    void OpenViewNodeWithOpera();
    void OpenViewNodeWithOPR();
    void OpenViewNodeWithSafari();
    void OpenViewNodeWithChrome();
    void OpenViewNodeWithSleipnir();
    void OpenViewNodeWithVivaldi();
    void OpenViewNodeWithCustom();

    void ResetTargetPosition();

private:
    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    Node *m_Node;
    int m_Nest;
    QRectF m_Rect;
    bool m_IsFocused;
    bool m_IsHovered;
    int m_HoveredTimerID;
    QPropertyAnimation *m_Animation;
    QPointF m_TargetPosition;
};

#endif //ifndef TREEBAR_HPP
