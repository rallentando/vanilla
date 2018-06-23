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

    class GraphicsView : public QGraphicsView {
    public:
        GraphicsView(QGraphicsScene *scene, QWidget *parent = 0)
            : QGraphicsView(scene, parent)
            , m_MouseEventSource(Qt::MouseEventNotSynthesized)
        {
        }
        void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE {
            m_MouseEventSource = ev->source();
            QGraphicsView::wheelEvent(ev);
            m_MouseEventSource = Qt::MouseEventNotSynthesized;
        }
        QSize sizeHint() const Q_DECL_OVERRIDE {
            return QGraphicsView::sizeHint();
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

    static bool EnableAnimation();
    static bool EnableCloseButton();
    static bool EnableCloneButton();
    static bool ScrollToSwitchNode();
    static bool WheelClickToClose();
    static void ToggleEnableAnimation();
    static void ToggleEnableCloseButton();
    static void ToggleEnableCloneButton();
    static void ToggleScrollToSwitchNode();
    static void ToggleWheelClickToClose();

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

    template <class T> T ScaleByDevice(T t) const {
        if(logicalDpiY() > 72)
            return  t * logicalDpiY() / 96;
        return t;
    }

    void AddTreeBarMenu(QMenu *menu);
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
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

private:
    TreeBank *m_TreeBank;
    GraphicsView *m_View;
    QGraphicsScene *m_Scene;
    QWidget *m_ResizeGrip;
    QSize m_OverrideSize;
    QList<LayerItem*> m_LayerList;
    int m_AutoUpdateTimerId;
    int m_HorizontalNodeWidth;
    int m_HorizontalNodeHeight;
    int m_VerticalNodeHeight;
    int m_VerticalNodeWidth;
    LastAction m_LastAction;
    MainWindow *m_TabWindow;
    QPoint m_HotSpot;

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
    LayerItem(TreeBank *tb, TreeBar *bar, Node *nd, Node *pnd = 0, QGraphicsItem *parent = 0);
    ~LayerItem();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    QRectF boundingRect() const Q_DECL_OVERRIDE;

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
    QList<NodeItem*> &GetNodeItems();
    void AppendToNodeItems(NodeItem *item);
    void PrependToNodeItems(NodeItem *item);
    void RemoveFromNodeItems(NodeItem *item);
    void SwapWithNext(int index);
    void SwapWithPrev(int index);

    QMenu *LayerMenu();
    QMenu *MakeNodeMenu();

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
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dropEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
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
    void OnDeleted(QRectF target, QRectF start = QRectF());
    void OnNestChanged();
    void OnUngrabbed();
    void Slide(int step);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE;

    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent *ev) Q_DECL_OVERRIDE;
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

    void ApplySiblingsOrder();

private:
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

#endif //ifndef TREEBAR_HPP
