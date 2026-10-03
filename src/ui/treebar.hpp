#ifndef TREEBAR_HPP
#define TREEBAR_HPP

#include "switch.hpp"
#include "const.hpp"

#include "lightnode.hpp"
#include "devicescale.hpp"

#include "view.hpp"

#include <float.h>

#include <QToolBar>
#include <QGraphicsView>
#include <QGraphicsObject>

class Node;
class TreeBank;
class NodeItem;
class LayerItem;
class QPaintEvent;
class QResizeEvent;
class QGraphicsScene;
class QGraphicsLineItem;
class QPropertyAnimation;

class TreeBar : public QToolBar {
    Q_OBJECT

public:
    TreeBar(TreeBank *tb, QWidget *parent = nullptr);
    ~TreeBar() Q_DECL_OVERRIDE;

    enum LastAction {
        None,
        TreeStructureChanged,
        NodeCreated,
        NodeDeleted,
        FoldedChanged,
        CurrentChanged,
    };

    class GraphicsView : public QGraphicsView {
    public:
        GraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
            : QGraphicsView(scene, parent)
            , m_MouseEventSource(Qt::MouseEventNotSynthesized)
        {
        }
        void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE {
            m_MouseEventSource = ev->source();
            QGraphicsView::wheelEvent(ev);
            m_MouseEventSource = Qt::MouseEventNotSynthesized;
        }
        QSize minimumSizeHint() const Q_DECL_OVERRIDE {
            return QSize(0, 0);
        }
        Qt::MouseEventSource MouseEventSource() const {
            return m_MouseEventSource;
        }
    private:
        Qt::MouseEventSource m_MouseEventSource;
    };

    static void Initialize();

    static void LoadSettings();
    static void SaveSettings();

    void ApplyTheme();

    static bool EnableFrameRate();
    static bool EnableAnimation();
    static bool EnableCloseButton();
    static bool EnableCloneButton();
    static bool ScrollToSwitchNode();
    static bool WheelClickToClose();
    static void ToggleEnableAnimation();
    static void ToggleEnableCloseButton();
    static void ToggleEnableCloneButton();

    void SetHorizontalNodeWidth(int width);
    void SetVerticalNodeHeight(int height);

    int GetHorizontalNodeWidth() const;
    int GetHorizontalNodeHeight() const;
    int GetVerticalNodeHeight() const;
    int GetVerticalNodeWidth() const;

    int MaxWidth() const;
    int MinWidth() const;
    int MaxHeight() const;
    int MinHeight() const;

    void SetStat(QStringList);
    QStringList GetStat() const;

    void ShowTabWindow(const QPoint &cursorPos, Node *node);
    void MoveTabWindow(const QPoint &cursorPos);
    void HideTabWindow();
    void ClearTabWindow();
    bool TabWindowVisible();

    void Adjust();

    void ClearLowerLayer(int index);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    GraphicsView *GetView(){ return m_View;}

    QList<LayerItem*> &GetLayerList(){ return m_LayerList;}

    int CurrentFrameRate(){ return m_CurrentFrameRate;}

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    void AddTreeBarMenu(QMenu *menu);
    QMenu *TreeBarMenu();

public slots:
    void CollectNodes();
    void OnTreeStructureChanged();
    void OnNodeCreated(NodeList &nds);
    void OnNodeDeleted(NodeList &nds);

    void ForgetNodes(NodeList &nds);
    void OnFoldedChanged(NodeList &nds);
    void OnCurrentChanged(Node *nd);

    void IncrementFrame(const QList<QRectF> &region);

    void SyncFrameRateTimer();

    void StartFrameRateTimer();
    void StopFrameRateTimer();

    void StartAutoUpdateTimer();
    void StopAutoUpdateTimer();
    void RestartAutoUpdateTimer();

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;

private:
    QRect HandlePaintRect(const QRect &rect) const;
    typedef QPair<Node*, int> VisibleNode;
    static QList<VisibleNode> VisibleSubtree(Node *root, int nest, const NodeList &currentPath);

    TreeBank *m_TreeBank;
    GraphicsView *m_View;
    friend class tst_treebaritem;

    QGraphicsScene *m_Scene;
    QWidget *m_ResizeGrip;
    QSize m_OverrideSize;
    QList<LayerItem*> m_LayerList;
    int m_AutoUpdateTimerId;
    int m_Frame;
    int m_FrameRateTimerId;
    int m_CurrentFrameRate;
    int m_HorizontalNodeWidth;
    int m_HorizontalNodeHeight;
    int m_VerticalNodeHeight;
    int m_VerticalNodeWidth;
    LastAction m_LastAction;
    MainWindow *m_TabWindow;
    QPoint m_HotSpot;

    static bool m_EnableFrameRate;
    static bool m_EnableAnimation;
    static bool m_EnableCloseButton;
    static bool m_EnableCloneButton;
    static bool m_ScrollToSwitchNode;
    static bool m_WheelClickToClose;
};

class LayerItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(qreal scroll READ GetScroll WRITE SetScroll)

public:
    LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, Node *pnd = nullptr, QGraphicsItem *parent = nullptr);
    ~LayerItem() Q_DECL_OVERRIDE;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    QRectF boundingRect() const Q_DECL_OVERRIDE;

    void ApplyTheme();

    int Index() const;

    int GetNest() const;
    void SetNest(int);

    QPropertyAnimation *GetAnimation() const;
    bool IsLocked() const;

    qreal MaxScroll() const;
    qreal MinScroll() const;
    qreal GetScroll() const;
    void SetScroll(qreal scroll);
    void Scroll(qreal delta);
    void ScrollForDelete(int count);
    void LockWhileAnimating();
    bool FinishAnimations();
    void ResetTargetScroll();
    void AutoScrollDown();
    void AutoScrollUp();
    void AutoScrollStop();
    void AutoScrollStopOrScroll(qreal delta);

    void StartScrollDownTimer();
    void StartScrollUpTimer();
    void StopScrollDownTimer();
    void StopScrollUpTimer();

    void Adjust(qreal scroll = -DBL_MAX);
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
    bool HasItemOf(Node *nd) const;
    QList<NodeItem*> &GetNodeItems();
    void AppendToNodeItems(NodeItem *item);
    void PrependToNodeItems(NodeItem *item);
    void RemoveFromNodeItems(NodeItem *item);
    void SwapWithNext(int index);
    void SwapWithPrev(int index);

    QMenu *MakeNodeMenu();

public slots:
    void NewViewNode();
    void CloneViewNode();
    void DisplayTrashTree();

protected:
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void dropEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *ev) Q_DECL_OVERRIDE;

private:
    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    Node *m_Node;
    NodeItem *m_FocusedNode;
    int m_Nest;
    int m_ScrollUpTimerId;
    int m_ScrollDownTimerId;
    qreal m_CurrentScroll;
    qreal m_TargetScroll;
    QGraphicsItem *m_PrevScrollButton;
    QGraphicsItem *m_NextScrollButton;
    QGraphicsItem *m_InsertPosition;
    QList<NodeItem*> m_NodeItems;
    QGraphicsLineItem *m_Line;
    QPropertyAnimation *m_Animation;

    Node *m_DummyNode;
};

class NodeItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(QRectF rect READ GetRect WRITE SetRect)

public:
    NodeItem(TreeBank *tb, TreeBar *bar, Node *nd, QGraphicsItem *parent = nullptr);
    ~NodeItem() Q_DECL_OVERRIDE;

    enum ButtonState {
        NotHovered,
        CloseHovered,
        ClosePressed,
        CloneHovered,
        ClonePressed,
        SoundHovered,
        SoundPressed,
    } m_ButtonState;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QRectF CloseButtonRect() const;
    QRectF CloneButtonRect() const;
    QRectF SoundButtonRect() const;
    QRect CloseIconRect() const;
    QRect CloneIconRect() const;
    QRect SoundIconRect() const;

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
    bool IsLocked() const;

    void SetButtonState(ButtonState state);
    void SetHoveredWithItem(bool hovered);

    void UnfoldDirectory();

    void OnCreated(QRectF target, QRectF start = QRectF());
    void StayUnderWhileGrowing();
    void OnDeleted(QRectF target, QRectF start = QRectF());

    void ForgetNode();
    void OnNestChanged();
    void OnUngrabbed();
    void Slide(int step);
    bool OutOfSight(const QRectF &rect) const;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE;

    QRect GlobalRect() const;
    void RequestPreview();

    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
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

    void OpenViewNodeWithDefault();
    void OpenViewNodeWithCommand();

    void ResetTargetPosition();

    void ApplySiblingsOrder();

private:
    QRectF ThumbnailRect(const QRectF &bound) const;
    QRect TitleBandRect(const QRectF &bound) const;
    QRectF TitleTextRect(const QRectF &bound, const QRect &titleBand) const;
    static int CloneSlot();
    QRectF RightChipRect(int slot) const;
    QRect RightIconRect(int slot) const;

    static QRect CenteredIconRect(const QRect &area, const QSize &size);
    friend class tst_treebaritem;

    TreeBank *m_TreeBank;
    TreeBar *m_TreeBar;
    Node *m_Node;
    int m_Nest;
    QRectF m_Rect;
    bool m_IsFocused;
    bool m_IsHovered;
    int m_HoveredTimerId;
    QPropertyAnimation *m_Animation;
    QPointF m_TargetPosition;
};

#endif
