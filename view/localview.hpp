#ifndef LOCALVIEW_HPP
#define LOCALVIEW_HPP

#include "switch.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

#include <QGraphicsPixmapItem>
#include <QMediaPlayer>
#include <QGraphicsVideoItem>
#include <QtConcurrent/QtConcurrent>

#include <functional>

#include "gadgets.hpp"
#include "networkcontroller.hpp"

class QString;
class QStringList;
class QNetworkRequest;
class QMediaPlayer;

class PixmapItem;
class VideoItem;

class LocalView : public GraphicsTableView , public View{
    Q_OBJECT

public:
    LocalView(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    virtual ~LocalView();

    virtual QGraphicsObject *base() DECL_OVERRIDE {
        return static_cast<QGraphicsObject*>(this);
    }
    virtual Page *page() DECL_OVERRIDE {
        return static_cast<Page*>(View::page());
    }

    virtual TreeBank *parent() DECL_OVERRIDE;
    virtual void setParent(TreeBank *tb) DECL_OVERRIDE;

    virtual QSize size() DECL_OVERRIDE { return Size().toSize();}
    virtual void resize(QSize size) DECL_OVERRIDE { ResizeNotify(size);}

    virtual void repaint() DECL_OVERRIDE { QGraphicsObject::update();}
    virtual bool visible() DECL_OVERRIDE { return QGraphicsObject::isVisible();}
    virtual void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) DECL_OVERRIDE {
        QGraphicsObject::setFocus(reason);
    }

    void Load(const QUrl &url) DECL_OVERRIDE;
    void Load(const QNetworkRequest &req) DECL_OVERRIDE;

    void Resize(QSizeF size) DECL_OVERRIDE;

    virtual QMenu *CreateNodeMenu() DECL_OVERRIDE;
    virtual void RenderBackground(QPainter *painter) DECL_OVERRIDE;

    static bool IsSupported(QUrl url);
    static bool IsSupportedImage(QUrl url);
    static bool IsSupportedVideo(QUrl url);
    static bool IsSupported(QString path);
    static bool IsSupportedImage(QString path);
    static bool IsSupportedVideo(QString path);

    virtual void RegisterNodes(const QUrl &url);

    virtual void SetNodeCollectionType(NodeCollectionType type) DECL_OVERRIDE {
        Q_UNUSED(type);
        // cannot set NodeCollectionType.
    }
    virtual NodeCollectionType GetNodeCollectionType() DECL_OVERRIDE {
        return Flat;
    }

    virtual void Activate(DisplayType type) DECL_OVERRIDE;
    virtual void Deactivate() DECL_OVERRIDE;

    virtual void show() DECL_OVERRIDE;
    virtual void hide() DECL_OVERRIDE;

    virtual void raise() DECL_OVERRIDE {
        setZValue(COVERING_VIEW_CONTENTS_LAYER);
    }
    virtual void lower() DECL_OVERRIDE {
        setZValue(HIDDEN_CONTENTS_LAYER);
    }

private:
    void Update_(QRectF rect){ update(rect);}

public:
    inline void Update(QRectF rect = QRectF()){
#ifdef OUT_OF_THREAD_UPDATE
        QtConcurrent::run(this, &LocalView::Update_, rect);
#else
        update(rect);
#endif
    }

    static void ClearCache();
    void SwapMediaItem(int index);

    void OnBeforeStartingDisplayGadgets() DECL_OVERRIDE {}
    void OnAfterFinishingDisplayGadgets() DECL_OVERRIDE {}

    // dummy DECL_OVERRIDE to avoid infinity loop.
    SharedWebElementList FindElements(Page::FindElementsOption) DECL_OVERRIDE { return SharedWebElementList();}
    SharedWebElement HitElement(const QPoint&) DECL_OVERRIDE { return SharedWebElement();}
    QUrl HitLinkUrl(const QPoint&) DECL_OVERRIDE { return QUrl();}
    QUrl HitImageUrl(const QPoint&) DECL_OVERRIDE { return QUrl();}
    QString SelectedText() DECL_OVERRIDE { return QString();}
    QString SelectedHtml() DECL_OVERRIDE { return QString();}
    QString WholeText() DECL_OVERRIDE { return QString();}
    QString WholeHtml() DECL_OVERRIDE { return QString();}
    QVariant EvaluateJavaScript(const QString&) DECL_OVERRIDE { return QVariant();}

    void Connect(TreeBank*) DECL_OVERRIDE;
    void Disconnect(TreeBank*) DECL_OVERRIDE;

    void ZoomIn()  DECL_OVERRIDE { ThumbList_ZoomIn();}
    void ZoomOut() DECL_OVERRIDE { ThumbList_ZoomOut();}

    QUrl BaseUrl() DECL_OVERRIDE { return QUrl();}
    QUrl CurrentBaseUrl() DECL_OVERRIDE { return QUrl();}

    virtual void UpKeyEvent() DECL_OVERRIDE {
        ThumbList_MoveToUpperItem();
    }
    virtual void DownKeyEvent() DECL_OVERRIDE {
        ThumbList_MoveToLowerItem();
    }
    virtual void RightKeyEvent() DECL_OVERRIDE {
        ThumbList_MoveToRightItem();
    }
    virtual void LeftKeyEvent() DECL_OVERRIDE {
        ThumbList_MoveToLeftItem();
    }
    virtual void PageDownKeyEvent() DECL_OVERRIDE {
        ThumbList_PageDown();
    }
    virtual void PageUpKeyEvent() DECL_OVERRIDE {
        ThumbList_PageUp();
    }
    virtual void HomeKeyEvent() DECL_OVERRIDE {
        ThumbList_MoveToFirstItem();
    }
    virtual void EndKeyEvent() DECL_OVERRIDE {
        ThumbList_MoveToLastItem();
    }

    virtual void UpdateThumbnail() DECL_OVERRIDE;

    virtual void TriggerKeyEvent(QKeyEvent *ev) DECL_OVERRIDE;
    virtual void TriggerKeyEvent(QString str) DECL_OVERRIDE;

    bool TriggerAction(QString str, QVariant data = QVariant()) DECL_OVERRIDE;
    void TriggerAction(Gadgets::GadgetsAction a);
    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        Action(a, data)->trigger();
    }

    QAction *Action(Gadgets::GadgetsAction a);
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        Q_UNUSED(data);
        switch(a){
        case Page::We_Reload:      return Action(Gadgets::Ge_Refresh);
        case Page::We_Back:        return Action(Gadgets::Ge_UpDirectory);
        case Page::We_Forward:     return Action(Gadgets::Ge_DownDirectory);
        case Page::We_NewViewNode: return Action(Gadgets::Ge_NewNode);
        case Page::We_NewHistNode: return Action(Gadgets::Ge_NewNode);
        }
        return Action(Gadgets::StringToAction(Page::ActionToString(a)));
    }

    PixmapItem *GetPixmapItem();
    VideoItem *GetVideoItem();
    QMediaPlayer *GetMediaPlayer();

    bool ThumbList_Refresh() DECL_OVERRIDE;
    bool ThumbList_RefreshNoScroll() DECL_OVERRIDE;
    bool ThumbList_OpenNode() DECL_OVERRIDE;
    bool ThumbList_DeleteNode() DECL_OVERRIDE;
    bool ThumbList_DeleteRightNode() DECL_OVERRIDE;
    bool ThumbList_DeleteLeftNode() DECL_OVERRIDE;
    bool ThumbList_DeleteOtherNode() DECL_OVERRIDE;
    bool ThumbList_PasteNode() DECL_OVERRIDE;
    bool ThumbList_RestoreNode() DECL_OVERRIDE;
    bool ThumbList_NewNode() DECL_OVERRIDE;
    bool ThumbList_CloneNode() DECL_OVERRIDE;
    bool ThumbList_UpDirectory() DECL_OVERRIDE;
    bool ThumbList_DownDirectory() DECL_OVERRIDE;
    bool ThumbList_MakeLocalNode() DECL_OVERRIDE;
    bool ThumbList_MakeDirectory() DECL_OVERRIDE;
    bool ThumbList_MakeDirectoryWithSelectedNode() DECL_OVERRIDE;
    bool ThumbList_MakeDirectoryWithSameDomainNode() DECL_OVERRIDE;
    bool ThumbList_RenameNode() DECL_OVERRIDE;
    bool ThumbList_CopyNodeUrl() DECL_OVERRIDE;
    bool ThumbList_CopyNodeTitle() DECL_OVERRIDE;
    bool ThumbList_CopyNodeAsLink() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithIE() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithFF() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithOpera() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithOPR() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithSafari() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithChrome() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithSleipnir() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithVivaldi() DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithCustom() DECL_OVERRIDE;
    bool ThumbList_ToggleTrash() DECL_OVERRIDE;
    bool ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos = QPointF()) DECL_OVERRIDE;
    bool ThumbList_ScrollUp() DECL_OVERRIDE;
    bool ThumbList_ScrollDown() DECL_OVERRIDE;
    bool ThumbList_PageUp() DECL_OVERRIDE;
    bool ThumbList_PageDown() DECL_OVERRIDE;
    bool ThumbList_MoveToUpperItem() DECL_OVERRIDE;
    bool ThumbList_MoveToLowerItem() DECL_OVERRIDE;
    bool ThumbList_MoveToRightItem() DECL_OVERRIDE;
    bool ThumbList_MoveToLeftItem() DECL_OVERRIDE;
    bool ThumbList_MoveToPrevPage() DECL_OVERRIDE;
    bool ThumbList_MoveToNextPage() DECL_OVERRIDE;
    bool ThumbList_MoveToFirstItem() DECL_OVERRIDE;
    bool ThumbList_MoveToLastItem() DECL_OVERRIDE;
    bool ThumbList_SelectToUpperItem() DECL_OVERRIDE;
    bool ThumbList_SelectToLowerItem() DECL_OVERRIDE;
    bool ThumbList_SelectToRightItem() DECL_OVERRIDE;
    bool ThumbList_SelectToLeftItem() DECL_OVERRIDE;
    bool ThumbList_SelectToPrevPage() DECL_OVERRIDE;
    bool ThumbList_SelectToNextPage() DECL_OVERRIDE;
    bool ThumbList_SelectToFirstItem() DECL_OVERRIDE;
    bool ThumbList_SelectToLastItem() DECL_OVERRIDE;
    bool ThumbList_SelectItem() DECL_OVERRIDE;
    bool ThumbList_SelectRange() DECL_OVERRIDE;
    bool ThumbList_SelectAll() DECL_OVERRIDE;
    bool ThumbList_ClearSelection() DECL_OVERRIDE;
    bool ThumbList_TransferToUpper() DECL_OVERRIDE;
    bool ThumbList_TransferToLower() DECL_OVERRIDE;
    bool ThumbList_TransferToRight() DECL_OVERRIDE;
    bool ThumbList_TransferToLeft() DECL_OVERRIDE;
    bool ThumbList_TransferToPrevPage() DECL_OVERRIDE;
    bool ThumbList_TransferToNextPage() DECL_OVERRIDE;
    bool ThumbList_TransferToFirst() DECL_OVERRIDE;
    bool ThumbList_TransferToLast() DECL_OVERRIDE;
    bool ThumbList_TransferToUpDirectory() DECL_OVERRIDE;
    bool ThumbList_TransferToDownDirectory() DECL_OVERRIDE;
    bool ThumbList_ZoomIn() DECL_OVERRIDE;
    bool ThumbList_ZoomOut() DECL_OVERRIDE;

protected:
    virtual void keyPressEvent         (QKeyEvent *ev) DECL_OVERRIDE;
    virtual void keyReleaseEvent       (QKeyEvent *ev) DECL_OVERRIDE;
    virtual void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void dropEvent             (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void mousePressEvent       (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    virtual void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    virtual void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    virtual void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    virtual void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) DECL_OVERRIDE;
    virtual void wheelEvent            (QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
    virtual void focusInEvent          (QFocusEvent *ev) DECL_OVERRIDE;
    virtual void focusOutEvent         (QFocusEvent *ev) DECL_OVERRIDE;

public:
    void KeyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void KeyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void MousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void MouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void MouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void MouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void WheelEvent(QWheelEvent *ev) DECL_OVERRIDE;

public slots:
    void UpdateLater();

    void OnSetViewNode(ViewNode*) DECL_OVERRIDE;
    void OnSetHistNode(HistNode*) DECL_OVERRIDE;
    void OnSetThis(WeakView) DECL_OVERRIDE;
    void OnSetMaster(WeakView) DECL_OVERRIDE;
    void OnSetSlave(WeakView) DECL_OVERRIDE;
    void OnSetJsObject(_View*) DECL_OVERRIDE;
    void OnSetJsObject(_Vanilla*) DECL_OVERRIDE;
    void OnLoadStarted() DECL_OVERRIDE;
    void OnLoadProgress(int) DECL_OVERRIDE;
    void OnLoadFinished(bool) DECL_OVERRIDE;
    void OnTitleChanged(const QString&) DECL_OVERRIDE;
    void OnUrlChanged(const QUrl&) DECL_OVERRIDE;
    void OnViewChanged() DECL_OVERRIDE;
    void OnScrollChanged() DECL_OVERRIDE;

    void EmitScrollChanged() DECL_OVERRIDE;
    void EmitScrollChangedIfNeed() DECL_OVERRIDE;

    QPointF GetScroll() DECL_OVERRIDE;
    void SetScroll(QPointF pos) DECL_OVERRIDE;

    bool SaveScroll()     DECL_OVERRIDE;
    bool RestoreScroll()  DECL_OVERRIDE;
    bool SaveZoom()       DECL_OVERRIDE;
    bool RestoreZoom()    DECL_OVERRIDE;
    bool SaveHistory()    DECL_OVERRIDE;
    bool RestoreHistory() DECL_OVERRIDE;

    void Download(QString, QString);
    void SeekText(const QString&, View::FindFlags);
    void KeyEvent(QString);

    void UpKey       ();
    void DownKey     ();
    void RightKey    ();
    void LeftKey     ();
    void HomeKey     ();
    void EndKey      ();
    void PageUpKey   ();
    void PageDownKey ();

signals:
    void linkHovered(const QString&, const QString&, const QString&);
    void ButtonCleared();
    void RenderFinished();

private:
    QMap<Gadgets::GadgetsAction, QAction*> m_ActionTable;

    QFuture<void> m_CollectingFuture;
    LocalNode *m_ParentNode;
    LocalNode *m_DummyLocalNode;

    PixmapItem *m_PixmapItem;
    VideoItem *m_VideoItem;
    QMediaPlayer *m_MediaPlayer;

    virtual void OpenNode(Node *ln);
    virtual void OpenNodes(NodeList list);
    virtual void DeleteNode(Node *ln);
    virtual void DeleteNodes(NodeList list);
    virtual void NewNode(Node *ln);
    virtual void CloneNode(Node *ln);
    virtual void MakeDirectory(Node *ln);

    void LoadImageRequest(int scope);
    void LoadImageRequestReverse(int scope);
    virtual void LoadImageToCache(Node *nd);
    virtual void LoadImageToCache(const QString &path);

    void StartImageCollector(bool reverse = true);
    void StopImageCollector();
    void RestartImageCollector();
    void RaiseMaxCostIfNeed();

    bool SelectMediaItem(int index, std::function<void()> defaultAction);
};

class PixmapItem : public QGraphicsPixmapItem{

public:
    PixmapItem(LocalView *parent = 0);
    ~PixmapItem();

protected:
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;

private:
    LocalView *m_LocalView;
};

class VideoItem : public QGraphicsVideoItem{
    Q_OBJECT

public:
    VideoItem(LocalView *parent = 0);
    ~VideoItem();

signals:
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);

public slots:
    void Play();
    void Pause();
    void Stop();
    void VolumeUp();
    void VolumeDown();
    void SetPositionRelative(qint64 diff);

protected:
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;

private:
    LocalView *m_LocalView;

};

#endif
