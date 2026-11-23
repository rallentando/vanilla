#ifndef GRAPHICSTABLEVIEW_HPP
#define GRAPHICSTABLEVIEW_HPP

#include <optional>
#include "switch.hpp"
#include "spentbuttons.hpp"

#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QElapsedTimer>

#include "treebank.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"

#include <functional>

#include "lightnode.hpp"
#include "devicescale.hpp"

class QAction;
class QMenu;
class QPropertyAnimation;
class QVariantAnimation;
class TreeBank;

class GadgetsStyle;
class SpotLight;
class ScrollIndicator;
class InPlaceNotifier;
class CloseButton;
class CloneButton;
class SoundButton;
class UpDirectoryButton;
class ToggleTrashButton;
class CloseTableButton;
class TableHeader;
class HeaderFilter;
class GraphicsButton;

class GraphicsTableView : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(qreal scroll READ GetScroll WRITE SetScroll NOTIFY ScrollChanged)
    friend class tst_gadgetsframe;

public:
    GraphicsTableView(TreeBank *parent = 0);
    virtual ~GraphicsTableView();

    void SetTreeBank(TreeBank *tb){ m_TreeBank = tb;}

    enum DisplayType {
        ViewTree,
        TrashTree,
        OtherTree,
        AccessKey,
        LocalFolderTree
    } m_DisplayType;

    enum ThumbnailStatus {
        Default,
        Primary,
        Hovered,
        Loaded,
        Selected,
        Dragging
    };

    enum DisplayArea {
        Margin,
        ThumbnailArea,
        NodeTitleArea,
        ScrollBarArea
    };

    enum SpotLightType {
        PrimarySpotLight,
        HoveredSpotLight,
        LoadedSpotLight
    };

    enum NodeCollectionType {
        Flat = 0,
        Recursive = 2,
        Foldable = 3
    };

    static void LoadSettings();
    static void SaveSettings();

    void SetZoomFactor(float factor){
        m_CurrentThumbnailZoomFactor = factor;
    }
    float GetZoomFactor() const {
        return m_CurrentThumbnailZoomFactor;
    }
    template <class T> T ScaleByDevice(T t) const {
        if(!m_TreeBank) return t;
        return DeviceScale::FromDpi(t, static_cast<int>(m_TreeBank->logicalDpiY()));
    }

    virtual void SetNodeCollectionType(NodeCollectionType){}
    virtual NodeCollectionType GetNodeCollectionType() const { return Flat;}

    static NodeCollectionType DefaultNodeCollectionType();
    static QString NodeCollectionTypeName(NodeCollectionType);
    static NodeCollectionType NodeCollectionTypeFromName(const QString&);

public slots:
    virtual void Activate(DisplayType type);
    virtual void Deactivate();

    void IncrementFrame(const QList<QRectF> &region);

    void StartFrameRateTimer();
    void StopFrameRateTimer();

    void StartAutoUpdateTimer();
    void StopAutoUpdateTimer();
    void RestartAutoUpdateTimer();

    bool IsAutoUpdating() const { return m_AutoUpdateTimerId != 0;}
    bool IsCountingFrames() const { return m_FrameRateTimerId != 0;}

public:
    void Update(QRectF rect = QRectF());
    void ScheduleRepaint();

    SpotLight *GetPrimarySpotLight() const;
    SpotLight *GetHoveredSpotLight() const;
    QList<SpotLight*> GetLoadedSpotLights() const;
    void SetCurrent(Node*);

    void SetHoveredItem(int);
    void SetHoveredItem(Thumbnail*);
    void SetHoveredItem(NodeTitle*);

    void SetPrimaryItem(int);
    void SetPrimaryItem(Thumbnail*);
    void SetPrimaryItem(NodeTitle*);

    void SetInPlaceNotifierContent(Node* nd);
    void SetInPlaceNotifierPosition(QPointF after);
    void UpdateInPlaceNotifier(QPointF pos, QPointF scenePos, bool ignoreStatusBarMessage);

    inline int        GetHoveredItemIndex() const { return m_HoveredItemIndex;}
    inline Thumbnail *GetHoveredThumbnail() const { return m_DisplayThumbnails.value(m_HoveredItemIndex);}
    inline NodeTitle *GetHoveredNodeTitle() const { return m_DisplayNodeTitles.value(m_HoveredItemIndex);}

    inline int        GetPrimaryItemIndex() const { return m_PrimaryItemIndex;}
    inline Thumbnail *GetPrimaryThumbnail() const { return m_DisplayThumbnails.value(m_PrimaryItemIndex);}
    inline NodeTitle *GetPrimaryNodeTitle() const { return m_DisplayNodeTitles.value(m_PrimaryItemIndex);}

    inline Thumbnail *GetScrolledThumbnail() const { return m_DisplayThumbnails.value(m_TargetScroll);}

    inline bool IsHovered(Thumbnail *thumb) const { return thumb && thumb == GetHoveredThumbnail();}
    inline bool IsHovered(NodeTitle *title) const { return title && title == GetHoveredNodeTitle();}
    inline bool IsPrimary(Thumbnail *thumb) const { return thumb && thumb == GetPrimaryThumbnail();}
    inline bool IsPrimary(NodeTitle *title) const { return title && title == GetPrimaryNodeTitle();}

    void ClearThumbnailSelection();
    void ClearNodeTitleSelection();
    void ClearScrollIndicatorSelection();

    void ResizeNotify(QSize size);
    virtual void Resize(QSizeF size);
    QSizeF Size();
    QRectF ComputeRect(const Thumbnail*, const int) const;
    QRectF ComputeRect(const NodeTitle*, const int) const;
    void RelocateContents();
    void FinishFoldAnimation();

    void LetGoOf(const QSet<Node*> &going, Node *formerParent);
    void RelocateScrollBar();
    void AppendToSelection(Node*);
    NodeList NodesToPutAside();
    bool PasteTakesSelection() const;
    void TakeSelectionForPaste();
    bool PasteNodeAt(const QPointF &pos);
    QPointF PastePosition() const;
    std::optional<QPointF> m_MenuPos;
    void RemoveFromSelection(Node*);
    void SetSelectedByIndex(int);
    void SetSelectionRange(Node*, Node*);
    void SetSelectionRange(Thumbnail*);
    void SetSelectionRange(NodeTitle*);

    bool MoveTo(bool, std::function<int(int, int)>);
    bool SelectTo(std::function<void()>);
    bool TransferTo(bool, bool, std::function<int(Thumbnail*, int, int)>);
    bool TransferToDirectory(bool up);

    virtual QMenu *CreateNodeMenu(){ return 0;}

    virtual bool TakeMappedPress(QGraphicsSceneMouseEvent*){ return false;}
    void ForgetSpentPress(Qt::MouseButton button){ m_TakenButtons.Press(button, false);}
    bool SettleSpentPress(Qt::MouseButton button){ return m_TakenButtons.Settle(button);}
    virtual void RenderBackground(QPainter *painter);
    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *item = 0, QWidget *widget = 0) override;
    QRectF ThumbnailAreaRect();
    QRectF NodeTitleAreaRect();
    QRectF ScrollBarAreaRect();

    static GadgetsStyle *GetStyle();

    static bool ScrollToChangeDirectory();
    static bool RightClickToRenameNode();

    static bool EnableCloseButton();
    bool IsDisplayingTrashTree() const { return m_DisplayType == TrashTree;}
    static bool EnableCloneButton();
    static bool EnableAnimation();
    static bool EnableFrameRate();

    bool HasHeader() const {
        return m_DisplayType == ViewTree
            || m_DisplayType == TrashTree
            || m_DisplayType == LocalFolderTree;
    }
    int ContentsTop() const;
    QRectF HeaderRect() const;
    QRectF HeaderButtonRect(const GraphicsButton *button) const;
    QString HeaderButtonLabel(const GraphicsButton *button) const;
    QRectF HeaderCloseRect() const;
    virtual QStringList BreadcrumbTitles();
    virtual void BreadcrumbActivated(int index);
    QRectF HeaderFilterRect() const;
    QString HeaderFilterPlaceholder() const;
    void SetFilter(const QString &text);
    virtual void OnFilterChanged();

    enum SortFlag {
        NoSort           = 0,
        Reverse          = 1 << 0,
        ByUrl            = 1 << 1,
        ByTitle          = 1 << 2,
        ByCreateDate     = 1 << 3,
        ByLastUpdateDate = 1 << 4,
        ByLastAccessDate = 1 << 5
    };
    Q_DECLARE_FLAGS(SortFlags, SortFlag);

private:
    bool DeleteNodes(NodeList list);

    SortFlags m_SortFlags;

protected:
    QMenu *CreateSortMenu(QWidget *parent = nullptr);
    QMenu *CreateNodeCollectionTypeMenu(QWidget *parent = nullptr);
    void SetSortPredicate();
    void SortNodesByDisplayOrder(NodeList &list);
    virtual bool DirectoriesFirst() const { return false;}

private:
    void ApplySort();
    QAction *m_ToggleSortReverseAction;
    QAction *m_ToggleSortByUrlAction;
    QAction *m_ToggleSortByTitleAction;
    QAction *m_ToggleSortByCreateDateAction;
    QAction *m_ToggleSortByLastUpdateDateAction;
    QAction *m_ToggleSortByLastAccessDateAction;

private slots:
    void OnNodeCollectionTypeChosen();
    void ToggleSortReverse();
    void ToggleSortByUrl();
    void ToggleSortByTitle();
    void ToggleSortByCreateDate();
    void ToggleSortByLastUpdateDate();
    void ToggleSortByLastAccessDate();

protected:
    virtual Node *DisplayedViewNode() const {
        return m_TreeBank ? m_TreeBank->GetCurrentViewNode() : nullptr;
    }
    void CollectNodes(Node *root);
    void ResetFilter();

public:
    static qreal ScrollIndicatorY(qreal maxY, qreal scroll, qreal maxScroll);
    static qreal ScrollFromIndicatorY(qreal y, qreal maxY, qreal maxScroll);
    static QRectF ScrollIndicatorRect(const QRectF &bar, qreal inset, qreal height);
    static qreal ScrollIndicatorTravel(qreal barHeight, qreal indicatorHeight, qreal inset);

    static qreal MaxScrollOf(int count, int columns, int lines, int titles);

    qreal MaxScroll() const;
    qreal MinScroll() const;
    int NodeTitleCountInScreen() const;

    qreal GetScroll() const;
    void Scroll(qreal delta);
    qreal ScrollForItem(qreal target) const;
    void ScrollToItem(qreal target);
    void ResetTargetScroll();

public slots:
    void SetScroll(QPointF pos);
    void SetScroll(qreal target);
    void SetScrollToItem(QPointF pos);
    void SetScrollToItem(qreal target);

signals:
    void ViewChanged();
    void ScrollChanged(QPointF);
    void ItemHovered(const QString&, const QString&, const QString&);
    void loadStarted();
    void loadFinished(bool);
    void loadProgress(int);
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void urlChanged(const QUrl&);
    void titleChanged(const QString&);

protected:
    virtual void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) override;
    virtual void dropEvent             (QGraphicsSceneDragDropEvent *ev) override;
    virtual void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) override;
    virtual void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) override;
    virtual void mousePressEvent       (QGraphicsSceneMouseEvent *ev) override;
    virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) override;
    virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) override;
    virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) override;
    virtual void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) override;
    virtual void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) override;
    virtual void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) override;
    virtual void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) override;
    virtual void wheelEvent            (QGraphicsSceneWheelEvent *ev) override;
    virtual void focusInEvent          (QFocusEvent *ev) override;
    virtual void focusOutEvent         (QFocusEvent *ev) override;
    void timerEvent(QTimerEvent *ev) override;

public slots:
    virtual bool ThumbList_Refresh();
    void OnFoldedChanged(const NodeList &nodes);
    virtual bool ThumbList_RefreshNoScroll();
    virtual bool ThumbList_OpenNode();
    virtual bool ThumbList_OpenNodeOnNewWindow();
    virtual bool ThumbList_DeleteNode();
    virtual bool ThumbList_DeleteRightNode();
    virtual bool ThumbList_DeleteLeftNode();
    virtual bool ThumbList_DeleteOtherNode();
    virtual bool ThumbList_CutNode();
    virtual bool ThumbList_CopyNode();
    virtual bool ThumbList_PasteNode();
    virtual bool ThumbList_RestoreNode();
    virtual bool ThumbList_NewNode();
    virtual bool ThumbList_CloneNode();
    virtual bool ThumbList_UpDirectory();
    virtual bool ThumbList_DownDirectory();
    virtual bool ThumbList_MakeLocalNode();
    virtual bool ThumbList_MakeDirectory();
    virtual bool ThumbList_MakeDirectoryWithSelectedNode();
    virtual bool ThumbList_MakeDirectoryWithSameDomainNode();
    virtual bool ThumbList_RenameNode();
    virtual bool ThumbList_CopyNodeUrl();
    virtual bool ThumbList_CopyNodeTitle();
    virtual bool ThumbList_CopyNodeAsLink();
    virtual bool ThumbList_OpenNodeWithDefault();
    virtual bool ThumbList_OpenNodeWithCommand(QString name = QString());
    virtual bool ThumbList_ToggleTrash();
    virtual bool ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos = QPointF());
    virtual bool ThumbList_ScrollUp();
    virtual bool ThumbList_ScrollDown();
    virtual bool ThumbList_NextPage();
    virtual bool ThumbList_PrevPage();
    virtual bool ThumbList_ZoomIn();
    virtual bool ThumbList_ZoomOut();
    virtual bool ThumbList_MoveToUpperItem();
    virtual bool ThumbList_MoveToLowerItem();
    virtual bool ThumbList_MoveToRightItem();
    virtual bool ThumbList_MoveToLeftItem();
    virtual bool ThumbList_MoveToPrevPage();
    virtual bool ThumbList_MoveToNextPage();
    virtual bool ThumbList_MoveToFirstItem();
    virtual bool ThumbList_MoveToLastItem();
    virtual bool ThumbList_SelectToUpperItem();
    virtual bool ThumbList_SelectToLowerItem();
    virtual bool ThumbList_SelectToRightItem();
    virtual bool ThumbList_SelectToLeftItem();
    virtual bool ThumbList_SelectToPrevPage();
    virtual bool ThumbList_SelectToNextPage();
    virtual bool ThumbList_SelectToFirstItem();
    virtual bool ThumbList_SelectToLastItem();
    virtual bool ThumbList_SelectItem();
    virtual bool ThumbList_SelectRange();
    virtual bool ThumbList_SelectAll();
    virtual bool ThumbList_ClearSelection();
    virtual bool ThumbList_TransferToUpper();
    virtual bool ThumbList_TransferToLower();
    virtual bool ThumbList_TransferToRight();
    virtual bool ThumbList_TransferToLeft();
    virtual bool ThumbList_TransferToPrevPage();
    virtual bool ThumbList_TransferToNextPage();
    virtual bool ThumbList_TransferToFirst();
    virtual bool ThumbList_TransferToLast();
    virtual bool ThumbList_TransferToUpDirectory();
    virtual bool ThumbList_TransferToDownDirectory();
    virtual bool ThumbList_SetNodeCollectionType(NodeCollectionType);
    virtual bool ThumbList_SwitchNodeCollectionType();
    virtual bool ThumbList_SwitchNodeCollectionTypeReverse();

private:
    TreeBank *m_TreeBank;
    QSizeF m_Size;

    std::function<bool(Node*, Node*)> m_SortPredicate;

    QMap<Node*, Thumbnail*> m_ThumbnailCache;
    QMap<Node*, NodeTitle*> m_NodeTitleCache;

    int m_Frame;
    int m_FrameRateTimerId;
    int m_CurrentFrameRate;

    int m_AutoUpdateTimerId;
    QTimer m_RepaintTimer;

    ViewNode *m_DummyViewNode;

    static GadgetsStyle *m_Style;
    static NodeCollectionType m_DefaultNodeCollectionType;
    static bool m_ScrollToChangeDirectory;
    static bool m_RightClickToRenameNode;
    static bool m_EnablePrimarySpotLight;
    static bool m_EnableHoveredSpotLight;
    static bool m_EnableLoadedSpotLight;
    static bool m_EnableInPlaceNotifier;
    static bool m_EnableCloseButton;
    static bool m_EnableCloneButton;
    static QString m_SelectionOperation;
    static bool m_EnableAnimation;
    static bool m_EnableFrameRate;

    SpotLight *m_PrimarySpotLight;
    SpotLight *m_HoveredSpotLight;
    QList<SpotLight*> m_LoadedSpotLights;

protected:
    Node *m_CurrentNode;

    bool m_StatusBarMessageIsSuspended;
    bool StatusBarMessageIsSuspended() const {
        return m_StatusBarMessageIsSuspended;
    }
    void SuspendStatusBarMessage(){
        m_StatusBarMessageIsSuspended = true;
    }
    void ResumeStatusBarMessage(){
        m_StatusBarMessageIsSuspended = false;
    }

    QPropertyAnimation *m_ScrollAnimation;
    QTimer m_ScrollPump;
    QElapsedTimer m_ScrollClock;
    bool m_ScrollStepQueued = false;
    void PumpScrollAnimation();
    void StepScrollAnimation();
    int ScrollPumpInterval() const;
    struct FoldTransition {
        QGraphicsItem *item;
        QRectF from;
        QRectF to;
        qreal opacity;
        bool entering;
        bool ghost;
    };
    QList<FoldTransition> m_FoldTransitions;
    QVariantAnimation *m_FoldAnimation;
    qreal m_CurrentScroll;
    qreal m_TargetScroll;
    int m_CurrentThumbnailLineCount;
    int m_CurrentThumbnailColumnCount;
    int m_CurrentThumbnailWidth;
    int m_CurrentThumbnailHeight;
    float m_CurrentThumbnailZoomFactor;

    QGraphicsRectItem *m_SelectRect;
    SpentButtons m_TakenButtons;

    static NodeList m_NodesRegister;

    QList<Thumbnail*> m_DisplayThumbnails;
    QList<NodeTitle*> m_DisplayNodeTitles;

    int m_HoveredItemIndex;
    int m_PrimaryItemIndex;

    ScrollIndicator *m_ScrollIndicator;

    InPlaceNotifier *m_InPlaceNotifier;

    CloseButton *m_CloseButton;
    CloneButton *m_CloneButton;
    SoundButton *m_SoundButton;

    UpDirectoryButton *m_UpDirectoryButton;

    ToggleTrashButton *m_ToggleTrashButton;

    CloseTableButton *m_CloseTableButton;

    TableHeader *m_TableHeader;
    HeaderFilter *m_HeaderFilter;
    QString m_FilterText;
    Node *m_FilterDirectory;

    void RelocateHeader();

    inline bool IsDisplayingNode() const {
        if(!isVisible()) return false;
        return m_DisplayType == ViewTree
            || m_DisplayType == TrashTree
            || m_DisplayType == LocalFolderTree;
    }

    inline bool IsDisplayingViewNode() const {
        if(!isVisible()) return false;
        return m_DisplayType == ViewTree || m_DisplayType == TrashTree;
    }

    inline bool IsDisplayingAccessKey() const {
        if(!isVisible()) return false;
        return m_DisplayType == AccessKey;
    }

    inline Node *GetHoveredNode() const {
        if(Thumbnail *thumb = m_DisplayThumbnails.value(m_HoveredItemIndex))
            return thumb->GetNode();
        return 0;
    }

    inline ViewNode *GetHoveredViewNode() const {
        if(IsDisplayingViewNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_HoveredItemIndex))
                return thumb->GetNode()->ToViewNode();
        return 0;
    }

    friend class SpotLight;
    friend class ScrollIndicator;
    friend class GadgetsStyle;
    friend class GlassStyle;
    friend class FlatStyle;
    friend class PlainStyle;
};

class SpotLight : public QGraphicsItem {

public:
    SpotLight(GraphicsTableView::SpotLightType type, QGraphicsItem *parent = 0);
    ~SpotLight();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

private:
    GraphicsTableView::SpotLightType m_Type;
    int m_Index;
public:
    GraphicsTableView::SpotLightType GetType() const { return m_Type;}
    int GetIndex() const { return m_Index;}
    void SetIndex(int index){ m_Index = index;}
};

class ScrollIndicator : public QGraphicsRectItem {

public:
    ScrollIndicator(QGraphicsItem *parent = 0);
    ~ScrollIndicator();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

private:
    GraphicsTableView *m_TableView;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void dragEnterEvent    (QGraphicsSceneDragDropEvent *ev) override;
    void dropEvent         (QGraphicsSceneDragDropEvent *ev) override;
    void dragMoveEvent     (QGraphicsSceneDragDropEvent *ev) override;
    void dragLeaveEvent    (QGraphicsSceneDragDropEvent *ev) override;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) override;
};

class InPlaceNotifier : public QGraphicsRectItem {

public:
    InPlaceNotifier(QGraphicsItem *parent = 0);
    ~InPlaceNotifier();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    Node *GetNode() const { return m_Node;}
    void SetNode(Node *nd);

    GraphicsTableView *GetTableView() const { return m_TableView;}

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void wheelEvent        (QGraphicsSceneWheelEvent *ev) override;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) override;

private:
    GraphicsTableView *m_TableView;
    Node *m_Node;
};

class GraphicsButton : public QGraphicsItem {

public:
    GraphicsButton(QGraphicsItem *parent = 0);
    ~GraphicsButton();

    enum ButtonState {
        NotHovered,
        Hovered,
        Pressed,
    };

    ButtonState GetState() const;
    void SetState(ButtonState state);

    GraphicsTableView *GetTableView() const { return m_TableView;}

protected:
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) override;

    void HoverItem();

protected:
    GraphicsTableView *m_TableView;
    QGraphicsItem *m_Item;
    ButtonState m_ButtonState;
};

class CloseButton : public GraphicsButton {

public:
    CloseButton(QGraphicsItem *parent = 0);
    ~CloseButton();

    Node *GetNode() const { return m_Node;}

    void UnsetItem();
    void SetItem(Thumbnail *thumb);
    void SetItem(NodeTitle *title);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;

private:
    Node *m_Node;
};

class CloneButton : public GraphicsButton {

public:
    CloneButton(QGraphicsItem *parent = 0);
    ~CloneButton();

    Node *GetNode() const { return m_Node;}

    void UnsetItem();
    void SetItem(Thumbnail *thumb);
    void SetItem(NodeTitle *title);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;

private:
    Node *m_Node;
};

class SoundButton : public GraphicsButton {

public:
    SoundButton(QGraphicsItem *parent = 0);
    ~SoundButton();

    Node *GetNode() const { return m_Node;}

    void UnsetItem();
    void SetItem(Thumbnail *thumb);
    void SetItem(NodeTitle *title);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;

private:
    Node *m_Node;
};

class HeaderButton : public GraphicsButton {

public:
    HeaderButton(QGraphicsItem *parent = 0) : GraphicsButton(parent){}

    QRectF boundingRect() const override { return m_Rect;}
    void SetRect(const QRectF &rect){ prepareGeometryChange(); m_Rect = rect; update();}

private:
    QRectF m_Rect;
};

class UpDirectoryButton : public HeaderButton {

public:
    UpDirectoryButton(QGraphicsItem *parent = 0);
    ~UpDirectoryButton();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;
};

class ToggleTrashButton : public HeaderButton {

public:
    ToggleTrashButton(QGraphicsItem *parent = 0);
    ~ToggleTrashButton();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;
};

class CloseTableButton : public HeaderButton {

public:
    CloseTableButton(QGraphicsItem *parent = 0);
    ~CloseTableButton();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *ev) override;
};

class TableHeader : public QGraphicsItem {

public:
    TableHeader(GraphicsTableView *parent);
    ~TableHeader();

    GraphicsTableView *GetTableView() const { return m_TableView;}

    QRectF boundingRect() const override { return m_Rect;}
    void SetRect(const QRectF &rect){ prepareGeometryChange(); m_Rect = rect; update();}
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    qreal BreadcrumbLeft() const;
    qreal BreadcrumbRight() const;
    QList<QRectF> SegmentRects(const QStringList &titles) const;
    static QFont SegmentFont(bool last);
    static qreal TextShift(const QFont &font);
    qreal SegmentPadding() const;
    qreal SeparatorWidth() const;
    qreal EllipsisWidth() const;

    int HoveredSegment() const { return m_HoveredSegment;}

protected:
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) override;

private:
    int SegmentAt(QPointF pos) const;
    void SetHoveredSegment(int index);

    GraphicsTableView *m_TableView;
    QRectF m_Rect;
    int m_HoveredSegment;
    int m_PressedSegment;
};

class HeaderFilter : public QGraphicsItem {

public:
    HeaderFilter(GraphicsTableView *parent);
    ~HeaderFilter();

    GraphicsTableView *GetTableView() const { return m_TableView;}
    QGraphicsTextItem *TextItem() const { return m_TextItem;}

    QRectF boundingRect() const override { return m_Rect;}
    void SetRect(const QRectF &rect);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    QString Text() const;
    void SetText(const QString &text);
    bool IsEditing() const;
    QRectF ClearRect() const;

protected:
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) override;

private:
    void OnContentsChanged();

    GraphicsTableView *m_TableView;
    QGraphicsTextItem *m_TextItem;
    QRectF m_Rect;
    bool m_Silent;
};

class HeaderFilterText : public QGraphicsTextItem {

public:
    HeaderFilterText(HeaderFilter *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void keyPressEvent(QKeyEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;

private:
    HeaderFilter *m_Filter;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GraphicsTableView::SortFlags);

#endif
