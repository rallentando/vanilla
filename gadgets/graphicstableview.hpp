#ifndef GRAPHICSTABLEVIEW_HPP
#define GRAPHICSTABLEVIEW_HPP

#include "switch.hpp"

#include <QGraphicsObject>
#include <QtConcurrent/QtConcurrent>

#include "thumbnail.hpp"
#include "nodetitle.hpp"

#include <functional>

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

class QAction;
class QMenu;
class QPropertyAnimation;
class TreeBank;

class GadgetsStyle;
class SpotLight;
class ScrollIndicator;
class InPlaceNotifier;
class CloseButton;
class CloneButton;
class UpDirectoryButton;

class GraphicsTableView : public QGraphicsObject{
    Q_OBJECT
    Q_PROPERTY(qreal scroll READ GetScroll WRITE SetScroll NOTIFY ScrollChanged)

public:
    GraphicsTableView(TreeBank *parent = 0);
    virtual ~GraphicsTableView();

    void SetTreeBank(TreeBank *tb){ m_TreeBank = tb;}

    enum DisplayType {
        HistTree,
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
        Flat,
        Straight,
        Recursive,
        Foldable
    };

    static void LoadSettings();
    static void SaveSettings();

    void SetZoomFactor(float factor){
        m_CurrentThumbnailZoomFactor = factor;
    }
    float GetZoomFactor(){
        return m_CurrentThumbnailZoomFactor;
    }

    virtual void SetNodeCollectionType(NodeCollectionType){}
    virtual NodeCollectionType GetNodeCollectionType(){ return Flat;}

public slots:
    virtual void Activate(DisplayType type);
    virtual void Deactivate();

    void StartAutoUpdateTimer();
    void StopAutoUpdateTimer();
    void RestartAutoUpdateTimer();

public:
    void Update(QRectF rect = QRectF());

    QString GetDirectoryPrefix(Node *nd);

    SpotLight *GetPrimarySpotLight();
    SpotLight *GetHoveredSpotLight();
    QList<SpotLight*> GetLoadedSpotLights();
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

    inline int        GetScrolledItemIndex() const { return m_TargetScroll;}
    inline Thumbnail *GetScrolledThumbnail() const { return m_DisplayThumbnails.value(m_TargetScroll);}
    inline NodeTitle *GetScrolledNodeTitle() const { return m_DisplayNodeTitles.value(m_TargetScroll);}

    // for thumbnail and nodetitle.
    inline bool IsHovered(Thumbnail *thumb) const { return thumb && thumb == GetHoveredThumbnail();}
    inline bool IsHovered(NodeTitle *title) const { return title && title == GetHoveredNodeTitle();}
    inline bool IsPrimary(Thumbnail *thumb) const { return thumb && thumb == GetPrimaryThumbnail();}
    inline bool IsPrimary(NodeTitle *title) const { return title && title == GetPrimaryNodeTitle();}
    inline bool IsLoaded (Thumbnail *thumb) const { return thumb && thumb->GetNode()->GetView();}
    inline bool IsLoaded (NodeTitle *title) const { return title && title->GetNode()->GetView();}

    void ClearThumbnailSelection();
    void ClearNodeTitleSelection();
    void ClearScrollIndicatorSelection();

    void ResizeNotify(QSize size);
    virtual void Resize(QSizeF size);
    QSizeF Size();
    QRectF ComputeRect(const Thumbnail*, const int) const;
    QRectF ComputeRect(const NodeTitle*, const int) const;
    void RelocateContents();
    void RelocateScrollBar();
    void AppendToSelection(Node*);
    void RemoveFromSelection(Node*);
    void SetSelectedByIndex(int);
    void SetSelectionRange(Node*, Node*);
    void SetSelectionRange(Thumbnail*);
    void SetSelectionRange(NodeTitle*);

    bool MoveTo(bool, std::function<int(int, int)>);
    bool SelectTo(std::function<void()>);
    bool TransferTo(bool, bool, std::function<int(Thumbnail*, int, int)>);

    virtual QMenu *CreateNodeMenu(){ return 0;}
    virtual void RenderBackground(QPainter *painter);
    virtual QRectF boundingRect() const DECL_OVERRIDE;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *item = 0, QWidget *widget = 0) DECL_OVERRIDE;
    QRectF ThumbnailAreaRect();
    QRectF NodeTitleAreaRect();
    QRectF ScrollBarAreaRect();

    static GadgetsStyle *GetStyle();

    static bool ScrollToChangeDirectory();
    static bool RightClickToRenameNode();

    static bool EnableCloseButton();
    static bool EnableCloneButton();
    static bool EnableAnimation();

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

    QMenu *CreateSortMenu();
    void SetSortPredicate();
    void ApplySort();
    QAction *m_ToggleSortReverseAction;
    QAction *m_ToggleSortByUrlAction;
    QAction *m_ToggleSortByTitleAction;
    QAction *m_ToggleSortByCreateDateAction;
    QAction *m_ToggleSortByLastUpdateDateAction;
    QAction *m_ToggleSortByLastAccessDateAction;

private slots:
    void ToggleSortReverse();
    void ToggleSortByUrl();
    void ToggleSortByTitle();
    void ToggleSortByCreateDate();
    void ToggleSortByLastUpdateDate();
    void ToggleSortByLastAccessDate();

protected:
    void CollectNodes(Node *root, QString filter = QString());

public:
    qreal MaxScroll();
    qreal MinScroll();

    qreal GetScroll();
    void Scroll(qreal delta);
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
    virtual void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void dropEvent             (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void mousePressEvent       (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    virtual void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    virtual void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    virtual void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) DECL_OVERRIDE;
    virtual void wheelEvent            (QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
    virtual void focusInEvent          (QFocusEvent *ev) DECL_OVERRIDE;
    virtual void focusOutEvent         (QFocusEvent *ev) DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;

public slots:
    virtual bool ThumbList_Refresh();
    virtual bool ThumbList_RefreshNoScroll();
    virtual bool ThumbList_OpenNode();
    virtual bool ThumbList_OpenNodeOnNewWindow();
    virtual bool ThumbList_DeleteNode();
    virtual bool ThumbList_DeleteRightNode();
    virtual bool ThumbList_DeleteLeftNode();
    virtual bool ThumbList_DeleteOtherNode();
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
    virtual bool ThumbList_OpenNodeWithIE();
    virtual bool ThumbList_OpenNodeWithFF();
    virtual bool ThumbList_OpenNodeWithOpera();
    virtual bool ThumbList_OpenNodeWithOPR();
    virtual bool ThumbList_OpenNodeWithSafari();
    virtual bool ThumbList_OpenNodeWithChrome();
    virtual bool ThumbList_OpenNodeWithSleipnir();
    virtual bool ThumbList_OpenNodeWithVivaldi();
    virtual bool ThumbList_OpenNodeWithCustom();
    virtual bool ThumbList_ToggleTrash();
    virtual bool ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos = QPointF());
    virtual bool ThumbList_ScrollUp();
    virtual bool ThumbList_ScrollDown();
    virtual bool ThumbList_PageUp();
    virtual bool ThumbList_PageDown();
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
    virtual bool ThumbList_SwitchNodeCollectionType();
    virtual bool ThumbList_SwitchNodeCollectionTypeReverse();

    // for scroll.
    QPointF CurrentThumbnailOffset();
    QPointF CurrentNodeTitleOffset();

private:
    TreeBank *m_TreeBank;
    QSizeF m_Size;

    std::function<bool(Node*, Node*)> m_SortPredicate;

    // Thumbnail object is about 600 bytes.
    // so needless to make static....?
    QMap<Node*, Thumbnail*> m_ThumbnailCache;
    QMap<Node*, NodeTitle*> m_NodeTitleCache;

    // for auto update.
    int m_AutoUpdateTimerID;

    // for empty directory.
    ViewNode *m_DummyViewNode;
    HistNode *m_DummyHistNode;

    // settings.
    static GadgetsStyle *m_Style;
    static bool m_ScrollToChangeDirectory;
    static bool m_RightClickToRenameNode;
    static bool m_EnablePrimarySpotLight;
    static bool m_EnableHoveredSpotLight;
    static bool m_EnableLoadedSpotLight;
    static bool m_EnableInPlaceNotifier;
    static bool m_EnableCloseButton;
    static bool m_EnableCloneButton;
    static bool m_EnableAnimation;

    SpotLight *m_PrimarySpotLight;
    SpotLight *m_HoveredSpotLight;
    QList<SpotLight*> m_LoadedSpotLights;

protected:
    Node *m_CurrentNode;

    bool m_StatusBarMessageIsSuspended;
    bool StatusBarMessageIsSuspended(){
        return m_StatusBarMessageIsSuspended;
    }
    void SuspendStatusBarMessage(){
        m_StatusBarMessageIsSuspended = true;
    }
    void ResumeStatusBarMessage(){
        m_StatusBarMessageIsSuspended = false;
    }

    // for thumblist layout.
    QPropertyAnimation *m_ScrollAnimation;
    qreal m_CurrentScroll;
    qreal m_TargetScroll;
    int m_CurrentThumbnailLineCount;
    int m_CurrentThumbnailColumnCount;
    int m_CurrentThumbnailWidth;
    int m_CurrentThumbnailHeight;
    float m_CurrentThumbnailZoomFactor;

    // for thumbnail selection.
    QGraphicsRectItem *m_SelectRect;

    // for drag (to other window).
    // allow duplicates.
    static NodeList m_NodesRegister;

    QList<Thumbnail*> m_DisplayThumbnails;
    QList<NodeTitle*> m_DisplayNodeTitles;

    // for spot light.
    int m_HoveredItemIndex;
    int m_PrimaryItemIndex;

    // scroll controller.
    ScrollIndicator *m_ScrollIndicator;

    // in place notifier.
    InPlaceNotifier *m_InPlaceNotifier;

    // close, clone button.
    CloseButton *m_CloseButton;
    CloneButton *m_CloneButton;

    // up directory button.
    UpDirectoryButton *m_UpDirectoryButton;

    inline bool IsDisplayingNode() const {
        if(!isVisible()) return false;
        return m_DisplayType == HistTree
            || m_DisplayType == ViewTree
            || m_DisplayType == TrashTree
            || m_DisplayType == LocalFolderTree;
    }

    inline bool IsDisplayingViewNode() const {
        if(!isVisible()) return false;
        return m_DisplayType == ViewTree || m_DisplayType == TrashTree;
    }

    inline bool IsDisplayingHistNode() const {
        if(!isVisible()) return false;
        return m_DisplayType == HistTree;
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

    inline Node *GetPrimaryNode() const {
        if(Thumbnail *thumb = m_DisplayThumbnails.value(m_PrimaryItemIndex))
            return thumb->GetNode();
        return 0;
    }

    inline Node *GetScrolledNode() const {
        if(Thumbnail *thumb = m_DisplayThumbnails.value(m_TargetScroll))
            return thumb->GetNode();
        return 0;
    }

    inline ViewNode *GetHoveredViewNode() const {
        if(IsDisplayingViewNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_HoveredItemIndex))
                return thumb->GetNode()->ToViewNode();
        return 0;
    }

    inline ViewNode *GetPrimaryViewNode() const {
        if(IsDisplayingViewNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_PrimaryItemIndex))
                return thumb->GetNode()->ToViewNode();
        return 0;
    }

    inline ViewNode *GetScrolledViewNode() const {
        if(IsDisplayingViewNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_TargetScroll))
                return thumb->GetNode()->ToViewNode();
        return 0;
    }

    inline HistNode *GetHoveredHistNode() const {
        if(IsDisplayingHistNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_HoveredItemIndex))
                return thumb->GetNode()->ToHistNode();
        return 0;
    }

    inline HistNode *GetPrimaryHistNode() const {
        if(IsDisplayingHistNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_PrimaryItemIndex))
                return thumb->GetNode()->ToHistNode();
        return 0;
    }

    inline HistNode *GetScrolledHistNode() const {
        if(IsDisplayingHistNode())
            if(Thumbnail *thumb = m_DisplayThumbnails.value(m_TargetScroll))
                return thumb->GetNode()->ToHistNode();
        return 0;
    }

    friend class SpotLight;
    friend class ScrollIndicator;
    friend class GadgetsStyle;
    friend class GlassStyle;
    friend class FlatStyle;
};

class SpotLight : public QGraphicsItem {

public:
    SpotLight(GraphicsTableView::SpotLightType type, QGraphicsItem *parent = 0);
    ~SpotLight();

    QRectF boundingRect() const DECL_OVERRIDE;
    QPainterPath shape() const DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

private:
    GraphicsTableView::SpotLightType m_Type;
    int m_Index; // for only LoadedSpotLight.
public:
    GraphicsTableView::SpotLightType GetType(){ return m_Type;}
    int GetIndex(){ return m_Index;}
    void SetIndex(int index){ m_Index = index;}
};

class ScrollIndicator : public QGraphicsRectItem {

public:
    ScrollIndicator(QGraphicsItem *parent = 0);
    ~ScrollIndicator();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

private:
    GraphicsTableView *m_TableView;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
    void dragEnterEvent    (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dropEvent         (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dragMoveEvent     (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dragLeaveEvent    (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
};

class InPlaceNotifier : public QGraphicsRectItem {

public:
    InPlaceNotifier(QGraphicsItem *parent = 0);
    ~InPlaceNotifier();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

    Node *GetNode(){ return m_Node;}
    void SetNode(Node *nd);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) DECL_OVERRIDE;
    void wheelEvent        (QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;

private:
    GraphicsTableView *m_TableView;
    Node *m_Node;
};

class GraphicsButton : public QGraphicsItem {

public:
    GraphicsButton(QGraphicsItem *parent = 0);
    ~GraphicsButton();

    enum ButtonState{
        NotHovered,
        Hovered,
        Pressed,
    };

    ButtonState GetState();
    void SetState(ButtonState state);

protected:
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent    (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;

protected:
    GraphicsTableView *m_TableView;
    QGraphicsItem *m_Item;
    ButtonState m_ButtonState;
};

class CloseButton : public GraphicsButton {

public:
    CloseButton(QGraphicsItem *parent = 0);
    ~CloseButton();

    void UnsetItem();
    void SetItem(Thumbnail *thumb);
    void SetItem(NodeTitle *title);

    QRectF boundingRect() const DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

protected:
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;

private:
    Node *m_Node;
};

class CloneButton : public GraphicsButton {

public:
    CloneButton(QGraphicsItem *parent = 0);
    ~CloneButton();

    void UnsetItem();
    void SetItem(Thumbnail *thumb);
    void SetItem(NodeTitle *title);

    QRectF boundingRect() const DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

protected:
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;

private:
    Node *m_Node;
};

class UpDirectoryButton : public GraphicsButton {

public:
    UpDirectoryButton(QGraphicsItem *parent = 0);
    ~UpDirectoryButton();

    QRectF boundingRect() const DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

protected:
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GraphicsTableView::SortFlags);

#endif
