#ifndef TREEBANK_HPP
#define TREEBANK_HPP

#include "switch.hpp"

#include "lightnode.hpp"

#include "actionmapper.hpp"
#include "view.hpp"
#include "mainwindow.hpp"

#include <QList>
#include <QMap>
#include <QGraphicsView>

class QString;
class QUrl;
class QNetworkRequest;
class QMenu;
class QAction;
class QDomDocument;
class QDomElement;

class _Vanilla;

class NetworkAccessManager;
class View;
class Notifier;
class Receiver;
class Gadgets;
class MainWindow;
class MiniMap;

class TreeBank : public QWidget {
    Q_OBJECT
    INSTALL_ACTION_MAP(TREEBANK, TreeBankAction)

public:
    TreeBank(QWidget *parent = nullptr);
    ~TreeBank() Q_DECL_OVERRIDE;

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
        Qt::MouseEventSource MouseEventSource() const {
            return m_MouseEventSource;
        }
        void MousePressEvent(QMouseEvent *ev){ QGraphicsView::mousePressEvent(ev);}
        void MouseReleaseEvent(QMouseEvent *ev){ QGraphicsView::mouseReleaseEvent(ev);}
    private:
        Qt::MouseEventSource m_MouseEventSource;
    };

    static void Initialize();

    static inline ViewNode *GetViewRoot() { return m_ViewRoot;}
    static inline ViewNode *GetTrashRoot(){ return m_TrashRoot;}

    static inline void AppendToAllViews(SharedView view){ m_AllViews.append(view);}
    static inline void PrependToAllViews(SharedView view){ m_AllViews.prepend(view);}
    static inline void RemoveFromAllViews(SharedView view){ m_AllViews.removeOne(view);}

    inline MainWindow *GetMainWindow() const { return static_cast<MainWindow*>(parentWidget());}
    inline Notifier *GetNotifier() const { return m_Notifier;}
    inline Receiver *GetReceiver() const { return m_Receiver;}
    inline MiniMap *GetMiniMap() const { return m_MiniMap;}
    QSize ViewSize() const;
    void SetMiniMapShelved(bool shelved);
    inline _Vanilla *GetJsObject() const { return m_JsObject;}
    inline Gadgets  *GetGadgets()  const { return m_Gadgets;}
    inline QGraphicsScene *GetScene() const { return m_Scene;}
    inline GraphicsView *GetView() const { return m_View;}

    inline SharedView GetCurrentView()     const { return m_CurrentView;}
    inline ViewNode *GetCurrentViewNode()  const { return m_CurrentViewNode;}
    inline ViewNode *GetViewIterForward()  const { return m_ViewIterForward;}
    inline ViewNode *GetViewIterBackward() const { return m_ViewIterBackward;}

    inline void SetCurrentView(SharedView view){ m_CurrentView = view;}
    inline void SetCurrentViewNode(ViewNode *vn){ m_CurrentViewNode = vn;}
    inline void SetViewIterForward(ViewNode *vn){ m_ViewIterForward = vn;}
    inline void SetViewIterBackward(ViewNode *vn){ m_ViewIterBackward = vn;}

signals:
    void TreeStructureChanged();
    void NodeCreated(NodeList &nds);
    void NodeDeleted(NodeList &nds);
    void FoldedChanged(NodeList &nds);
    void CurrentChanged(Node *nd);

public:
    static void EmitTreeStructureChanged();
    static void EmitNodeCreated(NodeList &nds);
    static void EmitNodeDeleted(NodeList &nds);
    static void EmitFoldedChanged(NodeList &nds);

private:
    void ConnectToNotifier();
    void ConnectToReceiver();

public:
    bool RenameNode(Node*);
    static void ReconfigureDirectory(ViewNode*, QString, QString);
private:
    static void ApplySpecificSettings(ViewNode *vn, ViewNode *dir = nullptr);

    static void ApplySpecificSettings(ViewNode*, QString, QString);

private:
    static void DoUpdate();
public:
    static void DoDelete();
    static void AddToUpdateBox(SharedView);
    static void AddToDeleteBox(Node*);
    static void RemoveFromUpdateBox(SharedView);
    static void RemoveFromDeleteBox(Node*);

public:
    static void AutoLoad();
private:
    static void LoadViewForward();
    static void LoadViewBackward();

public:
    static Node* GetRoot(Node*);
    static Node* GetOtherRoot(Node*);
    static bool  IsTrash(Node*);
    bool IsDisplayingTableView();
    bool IsCurrent(Node*);
    bool IsCurrent(SharedView);

    int WinIndex();
    static int WinIndex(Node*);
    static int WinIndex(SharedView);
    static void LiftMaxViewCountIfNeed(int now);

    static void LoadTree();
    static void SaveTree();
    static void UpdateCurrentThumbnails();

    static void LoadSettings();
    static void SaveSettings();

    static void RebuildViewForOffTheRecord(ViewNode *vn);

private:
    static void QuarantineViewNode(ViewNode *vn);
    static void DisownNode(Node *nd);
    static void DislinkView(ViewNode *vn);
    static bool MoveToTrash(ViewNode *vn);
    static void StripSubTree(Node *nd);
    static void ReleaseView(SharedView view);
public:
    static void ReleaseAllView();

private:
    static void RaiseDisplayedViewPriority();

public:
    bool DeleteNode(Node *nd);
    bool DeleteNode(NodeList list);

    static bool MoveNode(Node *nd, Node *dir, int n = -1);
    static bool SetChildrenOrder(Node *parent, NodeList children);

    bool SetCurrent(Node *nd);
    bool SetCurrent(SharedView view);

    void NthView(int n, ViewNode *vn = nullptr);

    void GoBackOrCloseForDownload(View *view);

public:
    void BeforeStartingDisplayGadgets();
    void AfterFinishingDisplayGadgets();

    void MousePressEvent(QMouseEvent *ev);
    void MouseReleaseEvent(QMouseEvent *ev);
    void MouseMoveEvent(QMouseEvent *ev);
    void MouseDoubleClickEvent(QMouseEvent *ev);
    void WheelEvent(QWheelEvent *ev);
    void DragEnterEvent(QDragEnterEvent *ev);
    void DragMoveEvent(QDragMoveEvent *ev);
    void DragLeaveEvent(QDragLeaveEvent *ev);
    void DropEvent(QDropEvent *ev);
    void ContextMenuEvent(QContextMenuEvent *ev);
    void KeyPressEvent(QKeyEvent *ev);
    void KeyReleaseEvent(QKeyEvent *ev);

    SharedView OpenInNewViewNode  (QNetworkRequest         req, bool activate, ViewNode *older  = nullptr);
    SharedView OpenInNewViewNode  (QUrl                    url, bool activate, ViewNode *older  = nullptr);
    SharedView OpenInNewViewNode  (QList<QNetworkRequest> reqs, bool activate, ViewNode *older  = nullptr);
    SharedView OpenInNewViewNode  (QList<QUrl>            urls, bool activate, ViewNode *older  = nullptr);
    SharedView OpenInNewDirectory (QNetworkRequest         req, bool activate, ViewNode *parent = nullptr);
    SharedView OpenInNewDirectory (QUrl                    url, bool activate, ViewNode *parent = nullptr);
    SharedView OpenInNewDirectory (QList<QNetworkRequest> reqs, bool activate, ViewNode *parent = nullptr);
    SharedView OpenInNewDirectory (QList<QUrl>            urls, bool activate, ViewNode *parent = nullptr);
    SharedView OpenOnSuitableNode (QNetworkRequest         req, bool activate, ViewNode *parent = nullptr, int position = -1);
    SharedView OpenOnSuitableNode (QUrl                    url, bool activate, ViewNode *parent = nullptr, int position = -1);
    SharedView OpenOnSuitableNode (QList<QNetworkRequest> reqs, bool activate, ViewNode *parent = nullptr, int position = -1);
    SharedView OpenOnSuitableNode (QList<QUrl>            urls, bool activate, ViewNode *parent = nullptr, int position = -1);
    void OpenByCommandOperation(const QUrl &url);


    QMenu *NodeMenu();
    QMenu *DisplayMenu();
    QMenu *WindowMenu();
    QMenu *PageMenu();
    QMenu *ApplicationMenu(bool expanded = false);

    QMenu *GlobalContextMenu();

    void PurgeChildWidgetsIfNeed();
    void JoinChildWidgetsIfNeed();

    enum OverviewState { OverviewAsIs, OverviewUp, OverviewDown };
    void RestackChildWidgets(OverviewState overview = OverviewAsIs);

protected:
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *ev) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *ev) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *ev) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;

public slots:
    void OpenInNewIfNeed(QUrl);
    void OpenInNewIfNeed(QList<QUrl>);
    void OpenInNewIfNeed(QString);
    void OpenInNewIfNeed(QString, QString);

    void Repaint();
    void Reconfigure();

    void Up                   (SharedView view = nullptr);
    void Down                 (SharedView view = nullptr);
    void Right                (SharedView view = nullptr);
    void Left                 (SharedView view = nullptr);
    void PageUp               (SharedView view = nullptr);
    void PageDown             (SharedView view = nullptr);
    void Home                 (SharedView view = nullptr);
    void End                  (SharedView view = nullptr);

    void Import();
    void Export();
    void AboutVanilla();
    void OpenSettings();
    void OpenDirectorySettings();
    void OpenDirectorySettings(ViewNode *subject);
    void AboutQt();
    void Quit();

    void ClearCookies();
    void ClearHttpCache();
    void ClearVisitedLinks();

    void ToggleNotifier();
    void ToggleReceiver();
    void ToggleMenuBar();
    void ToggleTreeBar();
    void ToggleToolBar();
    void ToggleFullScreen();
    void ToggleMaximized();
    void ToggleMinimized();
    void ToggleShaded();

    MainWindow *ShadeWindow(MainWindow *win = nullptr);
    MainWindow *UnshadeWindow(MainWindow *win = nullptr);
    MainWindow *NewWindow(int id = 0);
    MainWindow *CloseWindow(MainWindow *win = nullptr);
    MainWindow *SwitchWindow(bool next = true);
    MainWindow *NextWindow();
    MainWindow *PrevWindow();

    void Back                 ();
    void Forward              ();
    void Rewind               ();
    void FastForward          ();
    void UpDirectory          ();
    void Close                (ViewNode *vn = nullptr);
    void Restore              (ViewNode *vn = nullptr, ViewNode *dir = nullptr);
    void Recreate             (ViewNode *vn = nullptr);
    void NextView             (ViewNode *vn = nullptr);
    void PrevView             (ViewNode *vn = nullptr);
    void BuryView             (ViewNode *vn = nullptr);
    void DigView              (ViewNode *vn = nullptr);
    void FirstView            (ViewNode *vn = nullptr);
    void SecondView           (ViewNode *vn = nullptr);
    void ThirdView            (ViewNode *vn = nullptr);
    void FourthView           (ViewNode *vn = nullptr);
    void FifthView            (ViewNode *vn = nullptr);
    void SixthView            (ViewNode *vn = nullptr);
    void SeventhView          (ViewNode *vn = nullptr);
    void EighthView           (ViewNode *vn = nullptr);
    void NinthView            (ViewNode *vn = nullptr);
    void TenthView            (ViewNode *vn = nullptr);
    void LastView             (ViewNode *vn = nullptr);
    ViewNode *NewViewNode     (ViewNode *vn = nullptr);
    ViewNode *CloneViewNode   (ViewNode *vn = nullptr);
    ViewNode *MakeLocalNode   (ViewNode *vn = nullptr);
    ViewNode *MakeChildDirectory(ViewNode *vn = nullptr);
    ViewNode *MakeSiblingDirectory(ViewNode *vn = nullptr);

    void DisplayViewTree      (ViewNode *vn = nullptr);
    void DisplayTrashTree     (ViewNode *vn = nullptr);
    void DisplayAccessKey     (SharedView view = nullptr);

    void OpenTextSeeker       (SharedView view = nullptr);
    void OpenQueryEditor      (SharedView view = nullptr);
    void OpenUrlEditor        (SharedView view = nullptr);
    void OpenCommand          (SharedView view = nullptr);
    void ReleaseHiddenView    (SharedView view = nullptr);
    void Load                 (SharedView view = nullptr);

    void Copy                 (SharedView view = nullptr);
    void Cut                  (SharedView view = nullptr);
    void Paste                (SharedView view = nullptr);

#define VANILLA_EDIT_ACTION(name) void name(SharedView view = nullptr);
    FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

    void Undo                 (SharedView view = nullptr);
    void Redo                 (SharedView view = nullptr);
    void SelectAll            (SharedView view = nullptr);
    void Unselect             (SharedView view = nullptr);
    void Reload               (SharedView view = nullptr);
    void ReloadAndBypassCache (SharedView view = nullptr);
    void Stop                 (SharedView view = nullptr);
    void StopAndUnselect      (SharedView view = nullptr);

    void Print                (SharedView view = nullptr);
    void Save                 (SharedView view = nullptr);
    void ZoomIn               (SharedView view = nullptr);
    void ZoomOut              (SharedView view = nullptr);
    void ViewSource           (SharedView view = nullptr);
    void ApplySource          (SharedView view = nullptr);

    void InspectElement       (SharedView view = nullptr);

    void CopyUrl              (SharedView view = nullptr);
    void CopyTitle            (SharedView view = nullptr);
    void CopyPageAsLink       (SharedView view = nullptr);
    void CopySelectedHtml     (SharedView view = nullptr);
    void OpenWithDefault      (SharedView view = nullptr);
    void OpenWithCommand      (QString name, SharedView view = nullptr);

private slots:
    void UpKey()       { Up();}
    void DownKey()     { Down();}
    void RightKey()    { Right();}
    void LeftKey()     { Left();}
    void HomeKey()     { Home();}
    void EndKey()      { End();}
    void PageUpKey()   { PageUp();}
    void PageDownKey() { PageDown();}

public:
    void UpdateAction();
    bool TriggerAction(QString str);
    void TriggerAction(TreeBankAction a);
    void SetActionIcon(QAction *action, TreeBankAction a);
    void ApplyTheme();
    QAction *Action(QString str);
    QAction *Action(TreeBankAction a);
    bool TriggerKeyEvent(QKeyEvent *ev);
    bool TriggerKeyEvent(QString str);

private:
    QGraphicsScene *m_Scene;
    GraphicsView *m_View;
    Notifier *m_Notifier;
    Receiver *m_Receiver;
    MiniMap *m_MiniMap;
    Gadgets  *m_Gadgets;
    SharedView m_Gadgets_;
    _Vanilla *m_JsObject;

    SharedView m_CurrentView;
    ViewNode *m_CurrentViewNode;

    QMap<TreeBankAction, QAction*> m_ActionTable;

    static QString m_RootName;
    static ViewNode *m_ViewRoot;
    static ViewNode *m_TrashRoot;

    static SharedViewList m_AllViews;
    static SharedViewList m_ViewUpdateBox;
    static NodeList m_NodeDeleteBox;

    static int m_TraverseCondition;
    static ViewNode *m_ViewIterForward;
    static ViewNode *m_ViewIterBackward;

    static bool m_TraverseAllView;
    static bool m_PurgeNotifier;
    static bool m_PurgeReceiver;
    static bool m_PurgeView;
    static bool m_EnableMiniMap;
    static int  m_MaxViewCount;
    static int  m_MaxTrashEntryCount;
    static enum Viewport {
        Widget, GLWidget, OpenGLWidget
    } m_Viewport;

    static QMap<QKeySequence, QString> m_KeyMap;
    static QMap<QString, QString> m_MouseMap;

public:
    static void DeleteView(View *view);
    static SharedView CreateView(QNetworkRequest req, ViewNode *vn);
    static bool NeedsEngineForVanillaPage(const QUrl &url, const QStringList &set);
    static bool PurgeView(){ return m_PurgeView;}
};

#endif
