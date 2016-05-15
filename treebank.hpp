#ifndef TREEBANK_HPP
#define TREEBANK_HPP

#include "switch.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

#include "actionmapper.hpp"
#include "view.hpp"
#include "mainwindow.hpp"

#include <QList>
#include <QMap>
#include <QGraphicsView>

class QString;
class QStringList;
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

class TreeBank : public QWidget {
    Q_OBJECT
    INSTALL_ACTION_MAP(TREEBANK, TreeBankAction)

public:
    TreeBank(QWidget *parent = 0);
    ~TreeBank();

    static void Initialize();

    static inline HistNode *GetHistRoot() { return m_HistRoot;}
    static inline ViewNode *GetViewRoot() { return m_ViewRoot;}
    static inline ViewNode *GetTrashRoot(){ return m_TrashRoot;}

    static inline SharedViewList GetAllViews(){ return m_AllViews;}
    static inline void AppendToAllViews(SharedView view){ m_AllViews.append(view);}
    static inline void PrependToAllViews(SharedView view){ m_AllViews.prepend(view);}
    static inline void RemoveFromAllViews(SharedView view){ m_AllViews.removeOne(view);}

    inline MainWindow *GetMainWindow() const { return static_cast<MainWindow*>(parentWidget());}
    inline Notifier *GetNotifier() const { return m_Notifier;}
    inline Receiver *GetReceiver() const { return m_Receiver;}
    inline _Vanilla *GetJsObject() const { return m_JsObject;}
    inline Gadgets  *GetGadgets()  const { return m_Gadgets;}
    inline QGraphicsScene *GetScene() const { return m_Scene;}
    inline QGraphicsView *GetView() const { return m_View;}

    inline SharedView GetCurrentView()     const { return m_CurrentView;}
    inline ViewNode *GetCurrentViewNode()  const { return m_CurrentViewNode;}
    inline HistNode *GetCurrentHistNode()  const { return m_CurrentHistNode;}
    inline ViewNode *GetViewIterForward()  const { return m_ViewIterForward;}
    inline ViewNode *GetViewIterBackward() const { return m_ViewIterBackward;}
    inline HistNode *GetHistIterForward()  const { return m_HistIterForward;}
    inline HistNode *GetHistIterBackward() const { return m_HistIterBackward;}

    inline void SetCurrentView(SharedView view){ m_CurrentView = view;}
    inline void SetCurrentViewNode(ViewNode *vn){ m_CurrentViewNode = vn;}
    inline void SetCurrentHistNode(HistNode *hn){ m_CurrentHistNode = hn;}
    inline void SetViewIterForward(ViewNode *vn){ m_ViewIterForward = vn;}
    inline void SetViewIterBackward(ViewNode *vn){ m_ViewIterBackward = vn;}
    inline void SetHistIterForward(HistNode *hn){ m_HistIterForward = hn;}
    inline void SetHistIterBackward(HistNode *hn){ m_HistIterBackward = hn;}

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

    // for rename.
public:
    bool RenameNode(Node*);
    static void ReconfigureDirectory(ViewNode*, QString, QString);
private:
    static void ApplySpecificSettings(ViewNode *vn, ViewNode *dir = 0);
    static void ApplySpecificSettings(ViewNode*, NetworkAccessManager*, QStringList, QString, QString);
    static void ApplySpecificSettings(HistNode*, NetworkAccessManager*, QStringList);

    // for update.
private:
    static void DoUpdate();
    static void DoDelete();
public:
    static void AddToUpdateBox(SharedView);
    static void AddToDeleteBox(Node*);
    static void RemoveFromUpdateBox(SharedView);
    static void RemoveFromDeleteBox(Node*);

    // for auto loading.
public:
    static void AutoLoad();
private:
    static void LoadHistForward();
    static void LoadHistBackward();
    static void LoadViewForward();
    static void LoadViewBackward();

    // predicates.
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

    // save and load.
    static void LoadTree();
    static void SaveTree();

    static void LoadSettings();
    static void SaveSettings();

    // for deletion.
private:
    static void QuarantineViewNode(ViewNode *vn);
    static void QuarantineHistNode(HistNode *hn);
    static void DisownNode(Node *nd);
    static void DislinkView(HistNode *hn);
    static void DietSubTree(HistNode *hn, QList<HistNode*> ignore);
    static bool MoveToTrash(ViewNode *vn);
    static bool DeleteHistory(HistNode *hn);
    static void StripSubTree(Node *nd);
    static void ReleaseView(SharedView view);
public:
    static void ReleaseAllView();

private:
    void ClearCache(Node *nd);
    static void RaiseDisplayedViewPriority();

public:
    // deleting function.
    bool DeleteNode(Node *nd);
    bool DeleteNode(NodeList list);

    // moving function.
    static bool MoveNode(Node *nd, Node *dir, int n = -1);
    static bool SetChildrenOrder(Node *parent, NodeList children);

    // switching function.
    // this is heavy function.
    bool SetCurrent(Node *nd);
    bool SetCurrent(SharedView view);

    void NthView(int n, ViewNode *vn = 0);

    // for gadgets.
public:
#if defined(Q_OS_WIN)
    // TridentView(s) (have) existed.
    static bool TridentViewExist();
#endif
    void BeforeStartingDisplayGadgets();
    void AfterFinishingDisplayGadgets();

    // events
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

    SharedView OpenInNewViewNode  (QNetworkRequest         req, bool activate, ViewNode *older  = 0);
    SharedView OpenInNewViewNode  (QUrl                    url, bool activate, ViewNode *older  = 0);
    SharedView OpenInNewViewNode  (QList<QNetworkRequest> reqs, bool activate, ViewNode *older  = 0);
    SharedView OpenInNewViewNode  (QList<QUrl>            urls, bool activate, ViewNode *older  = 0);
    SharedView OpenInNewDirectory (QNetworkRequest         req, bool activate, ViewNode *parent = 0);
    SharedView OpenInNewDirectory (QUrl                    url, bool activate, ViewNode *parent = 0);
    SharedView OpenInNewDirectory (QList<QNetworkRequest> reqs, bool activate, ViewNode *parent = 0);
    SharedView OpenInNewDirectory (QList<QUrl>            urls, bool activate, ViewNode *parent = 0);
    SharedView OpenOnSuitableNode (QNetworkRequest         req, bool activate, ViewNode *parent = 0, int position = -1);
    SharedView OpenOnSuitableNode (QUrl                    url, bool activate, ViewNode *parent = 0, int position = -1);
    SharedView OpenOnSuitableNode (QList<QNetworkRequest> reqs, bool activate, ViewNode *parent = 0, int position = -1);
    SharedView OpenOnSuitableNode (QList<QUrl>            urls, bool activate, ViewNode *parent = 0, int position = -1);
    SharedView OpenInNewHistNode  (QNetworkRequest         req, bool activate, HistNode *parent = 0);
    SharedView OpenInNewHistNode  (QUrl                    url, bool activate, HistNode *parent = 0);
    SharedView OpenInNewHistNode  (QList<QNetworkRequest> reqs, bool activate, HistNode *parent = 0);
    SharedView OpenInNewHistNode  (QList<QUrl>            urls, bool activate, HistNode *parent = 0);

    SharedView OpenInNewHistNodeBackward(QNetworkRequest req, bool activate, HistNode *child = 0);
    SharedView OpenInNewHistNodeBackward(QUrl            url, bool activate, HistNode *child = 0);

    QMenu *NodeMenu();
    QMenu *DisplayMenu();
    QMenu *WindowMenu();
    QMenu *PageMenu();
    QMenu *ApplicationMenu(bool expanded = false);

    QMenu *GlobalContextMenu();

    void PurgeChildWidgetsIfNeed();
    void JoinChildWidgetsIfNeed();

protected:
    // events
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *ev) DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *ev) DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *ev) DECL_OVERRIDE;
    void dropEvent(QDropEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;

public slots:
    void OpenInNewIfNeed(QUrl);
    void OpenInNewIfNeed(QList<QUrl>);
    void OpenInNewIfNeed(QString);
    void OpenInNewIfNeed(QString, QString);

    void Repaint();
    void Reconfigure();

    void Up                   (SharedView view = 0);
    void Down                 (SharedView view = 0);
    void Right                (SharedView view = 0);
    void Left                 (SharedView view = 0);
    void PageUp               (SharedView view = 0);
    void PageDown             (SharedView view = 0);
    void Home                 (SharedView view = 0);
    void End                  (SharedView view = 0);

    void Import();
    void Export();
    void AboutVanilla();
    void AboutQt();
    void Quit();

    void ToggleNotifier();
    void ToggleReceiver();
    void ToggleMenuBar();
    void ToggleTreeBar();
    void ToggleToolBar();
    void ToggleFullScreen();
    void ToggleMaximized();
    void ToggleMinimized();
    void ToggleShaded();

    MainWindow *ShadeWindow(MainWindow *win = 0);
    MainWindow *UnshadeWindow(MainWindow *win = 0);
    MainWindow *NewWindow(int id = 0);
    MainWindow *CloseWindow(MainWindow *win = 0);
    MainWindow *SwitchWindow(bool next = true);
    MainWindow *NextWindow();
    MainWindow *PrevWindow();

    void Back                 (HistNode *hn = 0);
    void Forward              (HistNode *hn = 0);
    void Rewind               (HistNode *hn = 0);
    void FastForward          (HistNode *hn = 0);
    void UpDirectory          (HistNode *hn = 0);
    void Close                (ViewNode *vn = 0);
    void Restore              (ViewNode *vn = 0, ViewNode *dir = 0);
    void Recreate             (ViewNode *vn = 0);
    void NextView             (ViewNode *vn = 0);
    void PrevView             (ViewNode *vn = 0);
    void BuryView             (ViewNode *vn = 0);
    void DigView              (ViewNode *vn = 0);
    void FirstView            (ViewNode *vn = 0);
    void SecondView           (ViewNode *vn = 0);
    void ThirdView            (ViewNode *vn = 0);
    void FourthView           (ViewNode *vn = 0);
    void FifthView            (ViewNode *vn = 0);
    void SixthView            (ViewNode *vn = 0);
    void SeventhView          (ViewNode *vn = 0);
    void EighthView           (ViewNode *vn = 0);
    void NinthView            (ViewNode *vn = 0);
    void TenthView            (ViewNode *vn = 0);
    ViewNode *NewViewNode     (ViewNode *vn = 0);
    HistNode *NewHistNode     (HistNode *hn = 0);
    ViewNode *CloneViewNode   (ViewNode *vn = 0);
    HistNode *CloneHistNode   (HistNode *hn = 0);
    ViewNode *MakeLocalNode   (ViewNode *vn = 0);
    ViewNode *MakeChildDirectory(ViewNode *vn = 0);
    ViewNode *MakeSiblingDirectory(ViewNode *vn = 0);

    void DisplayViewTree      (ViewNode *vn = 0);
    void DisplayHistTree      (HistNode *hn = 0);
    void DisplayTrashTree     (ViewNode *vn = 0);
    void DisplayAccessKey     (SharedView view = 0);

    void OpenTextSeeker       (SharedView view = 0);
    void OpenQueryEditor      (SharedView view = 0);
    void OpenUrlEditor        (SharedView view = 0);
    void OpenCommand          (SharedView view = 0);
    void ReleaseHiddenView    (SharedView view = 0);
    void Load                 (SharedView view = 0);

    void Copy                 (SharedView view = 0);
    void Cut                  (SharedView view = 0);
    void Paste                (SharedView view = 0);
    void Undo                 (SharedView view = 0);
    void Redo                 (SharedView view = 0);
    void SelectAll            (SharedView view = 0);
    void Unselect             (SharedView view = 0);
    void Reload               (SharedView view = 0);
    void ReloadAndBypassCache (SharedView view = 0);
    void Stop                 (SharedView view = 0);
    void StopAndUnselect      (SharedView view = 0);

    void Print                (SharedView view = 0);
    void Save                 (SharedView view = 0);
    void ZoomIn               (SharedView view = 0);
    void ZoomOut              (SharedView view = 0);
    void ViewSource           (SharedView view = 0);
    void ApplySource          (SharedView view = 0);

    void InspectElement       (SharedView view = 0);

    void CopyUrl              (SharedView view = 0);
    void CopyTitle            (SharedView view = 0);
    void CopyPageAsLink       (SharedView view = 0);
    void CopySelectedHtml     (SharedView view = 0);
    void OpenWithIE           (SharedView view = 0);
    void OpenWithEdge         (SharedView view = 0);
    void OpenWithFF           (SharedView view = 0);
    void OpenWithOpera        (SharedView view = 0);
    void OpenWithOPR          (SharedView view = 0);
    void OpenWithSafari       (SharedView view = 0);
    void OpenWithChrome       (SharedView view = 0);
    void OpenWithSleipnir     (SharedView view = 0);
    void OpenWithVivaldi      (SharedView view = 0);
    void OpenWithCustom       (SharedView view = 0);

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
    QAction *Action(QString str);
    QAction *Action(TreeBankAction a);
    bool TriggerKeyEvent(QKeyEvent *ev);
    bool TriggerKeyEvent(QString str);

private:
    QGraphicsScene *m_Scene;
    QGraphicsView *m_View;
    Notifier *m_Notifier;
    Receiver *m_Receiver;
    Gadgets  *m_Gadgets;
    SharedView m_Gadgets_;
    _Vanilla *m_JsObject;

    SharedView m_CurrentView;
    ViewNode *m_CurrentViewNode;
    HistNode *m_CurrentHistNode;

    QMap<TreeBankAction, QAction*> m_ActionTable;

    static QString m_RootName;
    static ViewNode *m_ViewRoot;
    static ViewNode *m_TrashRoot;
    static HistNode *m_HistRoot;

    static SharedViewList m_AllViews;
    static SharedViewList m_ViewUpdateBox;
    static NodeList m_NodeDeleteBox;

    // update thumbnail and auto load view.
    static int m_TraverseCondition;
    static ViewNode *m_ViewIterForward;
    static ViewNode *m_ViewIterBackward;
    static HistNode *m_HistIterForward;
    static HistNode *m_HistIterBackward;

    static bool m_TraverseAllView;
    static bool m_PurgeNotifier;
    static bool m_PurgeReceiver;
    static bool m_PurgeView;
    static int  m_MaxViewCount;
    static int  m_MaxTrashEntryCount;
    static enum TraverseMode {
        HistMode, ViewMode, Neutral
    } m_TraverseMode;
    static enum Viewport {
        Widget, GLWidget, OpenGLWidget
    } m_Viewport;

    static QMap<QKeySequence, QString> m_KeyMap;
    static QMap<QString, QString> m_MouseMap;

public:
    static void DeleteView(View *view);
    static SharedView CreateView(QNetworkRequest req, HistNode *hn, ViewNode *vn);
    static bool PurgeView(){ return m_PurgeView;}
};

#endif //ifndef TREEBANK_HPP
