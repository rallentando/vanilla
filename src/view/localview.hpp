#ifndef LOCALVIEW_HPP
#define LOCALVIEW_HPP

#include "switch.hpp"

#ifdef LOCALVIEW

#include "lightnode.hpp"

#include <QGraphicsPixmapItem>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QGraphicsVideoItem>
#include <QFutureWatcher>

#include <functional>

#include "fileoperation.hpp"

#include "gadgets.hpp"
#include "networkcontroller.hpp"

class QString;
class QNetworkRequest;
class QMediaPlayer;
class QAudioOutput;

class PixmapItem;
class VideoItem;

class LocalView : public GraphicsTableView , public View {
    Q_OBJECT

public:
    LocalView(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    virtual ~LocalView();

    virtual QGraphicsObject *base() Q_DECL_OVERRIDE;
    virtual Page *page() Q_DECL_OVERRIDE;

    virtual TreeBank *parent() Q_DECL_OVERRIDE;
    virtual void setParent(TreeBank *tb) Q_DECL_OVERRIDE;

    virtual QSize size() Q_DECL_OVERRIDE { return Size().toSize();}
    virtual void resize(QSize size) Q_DECL_OVERRIDE { ResizeNotify(size);}

    virtual void repaint() Q_DECL_OVERRIDE { QGraphicsObject::update();}
    virtual bool visible() Q_DECL_OVERRIDE { return QGraphicsObject::isVisible();}
    virtual void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {
        QGraphicsObject::setFocus(reason);
    }

    void Load(const QUrl &url) Q_DECL_OVERRIDE;
    void Load(const QNetworkRequest &req) Q_DECL_OVERRIDE;

    void Resize(QSizeF size) Q_DECL_OVERRIDE;

    virtual QMenu *CreateNodeMenu() Q_DECL_OVERRIDE;
    virtual bool DirectoriesFirst() const Q_DECL_OVERRIDE { return true;}
    virtual void RenderBackground(QPainter *painter) Q_DECL_OVERRIDE;

    static bool IsSupported(QUrl url);
    static bool IsSupportedImage(QUrl url);
    static bool IsSupportedAudio(QUrl url);
    static bool IsSupportedVideo(QUrl url);
    static bool IsSupported(QString path);
    static bool IsSupportedImage(QString path);
    static bool IsSupportedAudio(QString path);
    static bool IsSupportedVideo(QString path);

    virtual void RegisterNodes(const QUrl &url);

    virtual void SetNodeCollectionType(NodeCollectionType type) Q_DECL_OVERRIDE {
        Q_UNUSED(type);
    }
    virtual NodeCollectionType GetNodeCollectionType() const Q_DECL_OVERRIDE {
        return Flat;
    }

    virtual void Activate(DisplayType type) Q_DECL_OVERRIDE;
    virtual void Deactivate() Q_DECL_OVERRIDE;

    virtual void show() Q_DECL_OVERRIDE;
    virtual void hide() Q_DECL_OVERRIDE;

    virtual void raise() Q_DECL_OVERRIDE {
        setZValue(COVERING_VIEW_CONTENTS_LAYER);
    }
    virtual void lower() Q_DECL_OVERRIDE {
        setZValue(HIDDEN_CONTENTS_LAYER);
    }

public:
    static void ClearCache();
    static void LoadSettings();
    void SwapMediaItem(int index);

    void OnBeforeStartingDisplayGadgets() Q_DECL_OVERRIDE {}
    void OnAfterFinishingDisplayGadgets() Q_DECL_OVERRIDE {}

    void Connect(TreeBank*) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank*) Q_DECL_OVERRIDE;

    void ZoomIn()  Q_DECL_OVERRIDE { ThumbList_ZoomIn();}
    void ZoomOut() Q_DECL_OVERRIDE { ThumbList_ZoomOut();}

    virtual void UpKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_MoveToUpperItem();
    }
    virtual void DownKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_MoveToLowerItem();
    }
    virtual void RightKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_MoveToRightItem();
    }
    virtual void LeftKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_MoveToLeftItem();
    }
    virtual void PageDownKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_NextPage();
    }
    virtual void PageUpKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_PrevPage();
    }
    virtual void HomeKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_MoveToFirstItem();
    }
    virtual void EndKeyEvent() Q_DECL_OVERRIDE {
        ThumbList_MoveToLastItem();
    }

    virtual void UpdateThumbnail() Q_DECL_OVERRIDE;

    virtual bool TriggerKeyEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    virtual bool TriggerKeyEvent(QString str) Q_DECL_OVERRIDE;

    bool TriggerAction(QString str, QVariant data = QVariant()) Q_DECL_OVERRIDE;
    void TriggerAction(Gadgets::GadgetsAction a);
    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        Action(a, data)->trigger();
    }

    QAction *Action(QString str, QVariant data = QVariant()) Q_DECL_OVERRIDE;
    QAction *Action(Gadgets::GadgetsAction a);
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        Q_UNUSED(data);
        switch(a){
        case Page::_Reload:      return Action(Gadgets::_Refresh);
        case Page::_Back:        return Action(Gadgets::_UpDirectory);
        case Page::_Forward:     return Action(Gadgets::_DownDirectory);
        case Page::_NewViewNode: return Action(Gadgets::_NewNode);
        default: break;
        }
        return Action(Gadgets::StringToAction(Page::ActionToString(a)));
    }

    PixmapItem *GetPixmapItem();
    VideoItem *GetVideoItem();
    QMediaPlayer *GetMediaPlayer();
    QAudioOutput *GetAudioOutput();

    void ChangeMediaVolume(int diff);

    bool ThumbList_Refresh() Q_DECL_OVERRIDE;
    bool ThumbList_RefreshNoScroll() Q_DECL_OVERRIDE;
    bool ThumbList_OpenNode() Q_DECL_OVERRIDE;
    bool ThumbList_OpenNodeOnNewWindow() Q_DECL_OVERRIDE;
    bool ThumbList_DeleteNode() Q_DECL_OVERRIDE;
    bool ThumbList_DeleteRightNode() Q_DECL_OVERRIDE;
    bool ThumbList_DeleteLeftNode() Q_DECL_OVERRIDE;
    bool ThumbList_DeleteOtherNode() Q_DECL_OVERRIDE;
    bool ThumbList_PasteNode() Q_DECL_OVERRIDE;
    bool ThumbList_RestoreNode() Q_DECL_OVERRIDE;
    bool ThumbList_NewNode() Q_DECL_OVERRIDE;
    bool ThumbList_CloneNode() Q_DECL_OVERRIDE;
    bool ThumbList_UpDirectory() Q_DECL_OVERRIDE;
    bool ThumbList_DownDirectory() Q_DECL_OVERRIDE;
    bool ThumbList_MakeLocalNode() Q_DECL_OVERRIDE;
    bool ThumbList_MakeDirectory() Q_DECL_OVERRIDE;
    bool ThumbList_MakeDirectoryWithSelectedNode() Q_DECL_OVERRIDE;
    bool ThumbList_MakeDirectoryWithSameDomainNode() Q_DECL_OVERRIDE;
    bool ThumbList_RenameNode() Q_DECL_OVERRIDE;
    bool ThumbList_CopyNodeUrl() Q_DECL_OVERRIDE;
    bool ThumbList_CopyNodeTitle() Q_DECL_OVERRIDE;
    bool ThumbList_CopyNodeAsLink() Q_DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithDefault() Q_DECL_OVERRIDE;
    bool ThumbList_OpenNodeWithCommand(QString name = QString()) Q_DECL_OVERRIDE;
    bool ThumbList_ToggleTrash() Q_DECL_OVERRIDE;
    bool ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos = QPointF()) Q_DECL_OVERRIDE;
    bool ThumbList_ScrollUp() Q_DECL_OVERRIDE;
    bool ThumbList_ScrollDown() Q_DECL_OVERRIDE;
    bool ThumbList_NextPage() Q_DECL_OVERRIDE;
    bool ThumbList_PrevPage() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToUpperItem() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToLowerItem() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToRightItem() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToLeftItem() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToPrevPage() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToNextPage() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToFirstItem() Q_DECL_OVERRIDE;
    bool ThumbList_MoveToLastItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToUpperItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToLowerItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToRightItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToLeftItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToPrevPage() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToNextPage() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToFirstItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectToLastItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectItem() Q_DECL_OVERRIDE;
    bool ThumbList_SelectRange() Q_DECL_OVERRIDE;
    bool ThumbList_SelectAll() Q_DECL_OVERRIDE;
    bool ThumbList_ClearSelection() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToUpper() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToLower() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToRight() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToLeft() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToPrevPage() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToNextPage() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToFirst() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToLast() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToUpDirectory() Q_DECL_OVERRIDE;
    bool ThumbList_TransferToDownDirectory() Q_DECL_OVERRIDE;
    bool ThumbList_ZoomIn() Q_DECL_OVERRIDE;
    bool ThumbList_ZoomOut() Q_DECL_OVERRIDE;

protected:
    virtual void keyPressEvent         (QKeyEvent *ev) Q_DECL_OVERRIDE;
    virtual void keyReleaseEvent       (QKeyEvent *ev) Q_DECL_OVERRIDE;
    virtual void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    virtual void dropEvent             (QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mousePressEvent       (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    virtual void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    virtual void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    virtual void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) Q_DECL_OVERRIDE;
    virtual void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) Q_DECL_OVERRIDE;
    virtual void wheelEvent            (QGraphicsSceneWheelEvent *ev) Q_DECL_OVERRIDE;
    virtual void focusInEvent          (QFocusEvent *ev) Q_DECL_OVERRIDE;
    virtual void focusOutEvent         (QFocusEvent *ev) Q_DECL_OVERRIDE;

public:
    void KeyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void KeyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void MousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void MouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void MouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void MouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void WheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;

public slots:
    void OnSetViewNode(ViewNode*) Q_DECL_OVERRIDE;
    void OnSetThis(WeakView) Q_DECL_OVERRIDE;
    void OnSetMaster(WeakView) Q_DECL_OVERRIDE;
    void OnSetSlave(WeakView) Q_DECL_OVERRIDE;
    void OnSetJsObject(_View*) Q_DECL_OVERRIDE;
    void OnSetJsObject(_Vanilla*) Q_DECL_OVERRIDE;
    void OnLoadStarted() Q_DECL_OVERRIDE;
    void OnLoadProgress(int) Q_DECL_OVERRIDE;
    void OnLoadFinished(bool) Q_DECL_OVERRIDE;
    void OnTitleChanged(const QString&) Q_DECL_OVERRIDE;
    void OnUrlChanged(const QUrl&) Q_DECL_OVERRIDE;
    void OnViewChanged() Q_DECL_OVERRIDE;
    void OnScrollChanged() Q_DECL_OVERRIDE;

    void EmitScrollChanged() Q_DECL_OVERRIDE;

    QPointF GetScroll() Q_DECL_OVERRIDE;
    void SetScroll(QPointF pos) Q_DECL_OVERRIDE;

    bool SaveScroll()     Q_DECL_OVERRIDE;
    bool RestoreScroll()  Q_DECL_OVERRIDE;
    bool SaveZoom()       Q_DECL_OVERRIDE;
    bool RestoreZoom()    Q_DECL_OVERRIDE;
    bool SaveHistory()    Q_DECL_OVERRIDE;
    bool RestoreHistory() Q_DECL_OVERRIDE;

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

private:
    static int m_MediaVolume;
    static bool m_AutoPlayMedia;

    QMap<Gadgets::GadgetsAction, QAction*> m_ActionTable;

    QFuture<void> m_CollectingFuture;

    QFutureWatcher<FileOperation::Reach> m_ReachWatcher;
    QFutureWatcher<QStringList> m_WorkWatcher;
    bool m_FileOperationRunning;
    QStringList m_PendingPaths;
    QString m_PendingDirectory;
    QString m_FailureTitle;
    QString m_FailureCaption;

    LocalNode *m_ParentNode;
    LocalNode *m_DummyLocalNode;

    PixmapItem *m_PixmapItem;
    QPixmap m_StillPixmap;
    bool m_StillFitsWindow;
    VideoItem *m_VideoItem;
    QMediaPlayer *m_MediaPlayer;
    QAudioOutput *m_AudioOutput;

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

    void StartFileOperation(const QString &status,
                            std::function<QStringList()> work,
                            const QString &failureTitle,
                            const QString &failureCaption);
    void AskThenDelete();
    void ReportFileOperation();
    void EndFileOperation();

    void StartImageCollector(bool reverse = true);
    void StopImageCollector();
    void RestartImageCollector();
    void RaiseMaxCostIfNeed();

    bool SelectMediaItem(int index, std::function<void()> defaultAction);

    void StopPlayback();
    void ShowStillPixmap(const QPixmap &pixmap, bool fitToWindow);
    void RelayoutStillPixmap();
    static QPixmap FileIconPixmap(const QString &path);

    void OnMediaMetaDataChanged();
    void OnMediaError();
};

class PixmapItem : public QGraphicsPixmapItem {

public:
    PixmapItem(LocalView *parent = 0);
    ~PixmapItem();

protected:
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;

private:
    LocalView *m_LocalView;
};

class VideoItem : public QGraphicsVideoItem {
    Q_OBJECT

public:
    VideoItem(LocalView *parent = 0);
    ~VideoItem();

signals:
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);

public:
    bool HandleMediaKey(QKeyEvent *ev);

public slots:
    void Play();
    void Pause();
    void Stop();
    void VolumeUp();
    void VolumeDown();
    void SetPositionRelative(qint64 diff);

protected:
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) Q_DECL_OVERRIDE;

private:
    LocalView *m_LocalView;
};

#endif
#endif
