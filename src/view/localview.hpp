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

    virtual QGraphicsObject *base() override;
    virtual Page *page() override;

    virtual TreeBank *parent() override;
    virtual void setParent(TreeBank *tb) override;

    QStringList BreadcrumbTitles() override;
    void BreadcrumbActivated(int index) override;
    void OnFilterChanged() override;

    virtual QSize size() override { return Size().toSize();}
    virtual void resize(QSize size) override { ResizeNotify(size);}

    virtual void repaint() override { QGraphicsObject::update();}
    virtual bool visible() override { return QGraphicsObject::isVisible();}
    virtual void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) override {
        QGraphicsObject::setFocus(reason);
    }

    void Load(const QUrl &url) override;
    void Load(const QNetworkRequest &req) override;

    void Resize(QSizeF size) override;

    virtual QMenu *CreateNodeMenu() override;
    virtual bool DirectoriesFirst() const override { return true;}
    virtual void RenderBackground(QPainter *painter) override;

    static bool IsSupported(QUrl url);
    static bool IsSupportedImage(QUrl url);
    static bool IsSupportedAudio(QUrl url);
    static bool IsSupportedVideo(QUrl url);
    static bool IsSupported(QString path);
    static bool IsSupportedImage(QString path);
    static bool IsSupportedAudio(QString path);
    static bool IsSupportedVideo(QString path);

    static QString UpDirectoryPath(const QString &path);
    static QStringList PathFolders(const QString &path);
    static QString PathOfFolder(const QStringList &folders, int index);

    virtual void RegisterNodes(const QUrl &url);

    virtual void SetNodeCollectionType(NodeCollectionType type) override {
        Q_UNUSED(type);
    }
    virtual NodeCollectionType GetNodeCollectionType() const override {
        return Flat;
    }

    virtual void Activate(DisplayType type) override;
    virtual void Deactivate() override;

    virtual void show() override;
    virtual void hide() override;

    virtual void raise() override {
        setZValue(COVERING_VIEW_CONTENTS_LAYER);
    }
    virtual void lower() override {
        setZValue(HIDDEN_CONTENTS_LAYER);
    }

public:
    static void ClearCache();
    static void LoadSettings();
    void SwapMediaItem(int index);

    void OnBeforeStartingDisplayGadgets() override {}
    void OnAfterFinishingDisplayGadgets() override {}

    void Connect(TreeBank*) override;
    void Disconnect(TreeBank*) override;

    void ZoomIn()  override { ThumbList_ZoomIn();}
    void ZoomOut() override { ThumbList_ZoomOut();}

    virtual void UpKeyEvent() override {
        ThumbList_MoveToUpperItem();
    }
    virtual void DownKeyEvent() override {
        ThumbList_MoveToLowerItem();
    }
    virtual void RightKeyEvent() override {
        ThumbList_MoveToRightItem();
    }
    virtual void LeftKeyEvent() override {
        ThumbList_MoveToLeftItem();
    }
    virtual void PageDownKeyEvent() override {
        ThumbList_NextPage();
    }
    virtual void PageUpKeyEvent() override {
        ThumbList_PrevPage();
    }
    virtual void HomeKeyEvent() override {
        ThumbList_MoveToFirstItem();
    }
    virtual void EndKeyEvent() override {
        ThumbList_MoveToLastItem();
    }

    virtual void UpdateThumbnail() override;

    virtual bool TriggerKeyEvent(QKeyEvent *ev) override;
    virtual bool TriggerKeyEvent(QString str) override;

    bool TriggerAction(QString str, QVariant data = QVariant()) override;
    void TriggerAction(Gadgets::GadgetsAction a);
    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) override {
        Action(a, data)->trigger();
    }

    QAction *Action(QString str, QVariant data = QVariant()) override;
    QAction *Action(Gadgets::GadgetsAction a);
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) override {
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

    VideoItem *GetVideoItem();
    QMediaPlayer *GetMediaPlayer();

    void ChangeMediaVolume(int diff);

    bool ThumbList_Refresh() override;
    bool ThumbList_RefreshNoScroll() override;
    bool ThumbList_OpenNode() override;
    bool ThumbList_OpenNodeOnNewWindow() override;
    bool ThumbList_DeleteNode() override;
    bool ThumbList_DeleteRightNode() override;
    bool ThumbList_DeleteLeftNode() override;
    bool ThumbList_DeleteOtherNode() override;
    bool ThumbList_PasteNode() override;
    bool ThumbList_RestoreNode() override;
    bool ThumbList_NewNode() override;
    bool ThumbList_CloneNode() override;
    bool ThumbList_UpDirectory() override;
    bool ThumbList_DownDirectory() override;
    bool ThumbList_MakeLocalNode() override;
    bool ThumbList_MakeDirectory() override;
    bool ThumbList_MakeDirectoryWithSelectedNode() override;
    bool ThumbList_MakeDirectoryWithSameDomainNode() override;
    bool ThumbList_RenameNode() override;
    bool ThumbList_CopyNodeUrl() override;
    bool ThumbList_CopyNodeTitle() override;
    bool ThumbList_CopyNodeAsLink() override;
    bool ThumbList_OpenNodeWithDefault() override;
    bool ThumbList_OpenNodeWithCommand(QString name = QString()) override;
    bool ThumbList_ToggleTrash() override;
    bool ThumbList_ApplyChildrenOrder(DisplayArea area, QPointF basepos = QPointF()) override;
    bool ThumbList_ScrollUp() override;
    bool ThumbList_ScrollDown() override;
    bool ThumbList_NextPage() override;
    bool ThumbList_PrevPage() override;
    bool ThumbList_MoveToUpperItem() override;
    bool ThumbList_MoveToLowerItem() override;
    bool ThumbList_MoveToRightItem() override;
    bool ThumbList_MoveToLeftItem() override;
    bool ThumbList_MoveToPrevPage() override;
    bool ThumbList_MoveToNextPage() override;
    bool ThumbList_MoveToFirstItem() override;
    bool ThumbList_MoveToLastItem() override;
    bool ThumbList_SelectToUpperItem() override;
    bool ThumbList_SelectToLowerItem() override;
    bool ThumbList_SelectToRightItem() override;
    bool ThumbList_SelectToLeftItem() override;
    bool ThumbList_SelectToPrevPage() override;
    bool ThumbList_SelectToNextPage() override;
    bool ThumbList_SelectToFirstItem() override;
    bool ThumbList_SelectToLastItem() override;
    bool ThumbList_SelectItem() override;
    bool ThumbList_SelectRange() override;
    bool ThumbList_SelectAll() override;
    bool ThumbList_ClearSelection() override;
    bool ThumbList_TransferToUpper() override;
    bool ThumbList_TransferToLower() override;
    bool ThumbList_TransferToRight() override;
    bool ThumbList_TransferToLeft() override;
    bool ThumbList_TransferToPrevPage() override;
    bool ThumbList_TransferToNextPage() override;
    bool ThumbList_TransferToFirst() override;
    bool ThumbList_TransferToLast() override;
    bool ThumbList_TransferToUpDirectory() override;
    bool ThumbList_TransferToDownDirectory() override;
    bool ThumbList_ZoomIn() override;
    bool ThumbList_ZoomOut() override;

protected:
    virtual void keyPressEvent         (QKeyEvent *ev) override;
    virtual void keyReleaseEvent       (QKeyEvent *ev) override;
    virtual void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) override;
    virtual void dropEvent             (QGraphicsSceneDragDropEvent *ev) override;
    virtual void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) override;
    virtual void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) override;
    virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) override;
    virtual void mousePressEvent       (QGraphicsSceneMouseEvent *ev) override;
    virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) override;
    virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) override;
    virtual void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) override;
    virtual void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) override;
    virtual void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) override;
    virtual void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) override;
    virtual void wheelEvent            (QGraphicsSceneWheelEvent *ev) override;
    virtual void focusInEvent          (QFocusEvent *ev) override;
    virtual void focusOutEvent         (QFocusEvent *ev) override;

public:
    void KeyPressEvent(QKeyEvent *ev) override;
    void KeyReleaseEvent(QKeyEvent *ev) override;
    void MousePressEvent(QMouseEvent *ev) override;
    void MouseReleaseEvent(QMouseEvent *ev) override;
    void MouseMoveEvent(QMouseEvent *ev) override;
    void MouseDoubleClickEvent(QMouseEvent *ev) override;
    void WheelEvent(QWheelEvent *ev) override;

public slots:
    void OnSetViewNode(ViewNode*) override;
    void OnSetThis(WeakView) override;
    void OnSetMaster(WeakView) override;
    void OnSetSlave(WeakView) override;
    void OnLoadStarted() override;
    void OnLoadProgress(int) override;
    void OnLoadFinished(bool) override;
    void OnTitleChanged(const QString&) override;
    void OnUrlChanged(const QUrl&) override;
    void OnViewChanged() override;
    void OnScrollChanged() override;

    void EmitScrollChanged() override;

    QPointF GetScroll() override;
    void SetScroll(QPointF pos) override;

    bool SaveScroll()     override;
    bool RestoreScroll()  override;
    bool SaveZoom()       override;
    bool RestoreZoom()    override;
    bool SaveHistory()    override;
    bool RestoreHistory() override;

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
    void keyPressEvent(QKeyEvent *ev) override;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) override;

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
    void keyPressEvent(QKeyEvent *ev) override;
    void mousePressEvent   (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent    (QGraphicsSceneMouseEvent *ev) override;

private:
    LocalView *m_LocalView;
};

#endif
#endif
