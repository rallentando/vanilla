#ifndef TREEBANK_HPP
#define TREEBANK_HPP

#include "switch.hpp"

#ifdef USE_LIGHTNODE
#  include "lightnode.hpp"
#else
#  include "node.hpp"
#endif

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

class TreeBank : public QGraphicsView {
    Q_OBJECT

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
    void NodeAttributeChanged(Node *nd, QVariant before, QVariant after);

private:
    void ConnectToNotifier();
    void ConnectToReceiver();

    // for rename.
public:
    bool RenameNode(Node*);
    void ReconfigureDirectory(ViewNode*, QString, QString);
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
    static void EmitTreeStructureChangedForAll();

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

public:
    enum TreeBankAction {
        Te_NoAction,

        // key events.
        Ke_Up,
        Ke_Down,
        Ke_Right,
        Ke_Left,
        Ke_Home,
        Ke_End,
        Ke_PageUp,
        Ke_PageDown,

        // application events.
        Te_Import,
        Te_Export,
        Te_AboutVanilla,
        Te_AboutQt,
        Te_Quit,

        // window events.
        Te_ToggleNotifier,
        Te_ToggleReceiver,
        Te_ToggleMenuBar,
        Te_ToggleTreeBar,
        Te_ToggleFullScreen,
        Te_ToggleMaximized,
        Te_ToggleMinimized,
        Te_ToggleShaded,
        Te_ShadeWindow,
        Te_UnshadeWindow,
        Te_NewWindow,
        Te_CloseWindow,
        Te_SwitchWindow,
        Te_NextWindow,
        Te_PrevWindow,

        // treebank events.
        Te_Back,
        Te_Forward,
        Te_UpDirectory,
        Te_Close,
        Te_Restore,
        Te_Recreate,
        Te_NextView,
        Te_PrevView,
        Te_BuryView,
        Te_DigView,
        Te_NewViewNode,
        Te_NewHistNode,
        Te_CloneViewNode,
        Te_CloneHistNode,
        Te_DisplayAccessKey,
        Te_DisplayViewTree,
        Te_DisplayHistTree,
        Te_DisplayTrashTree,
        Te_OpenTextSeeker,
        Te_OpenQueryEditor,
        Te_OpenUrlEditor,
        Te_OpenCommand,
        Te_Load,

        // web events.
        Te_Copy,
        Te_Cut,
        Te_Paste,
        Te_Undo,
        Te_Redo,
        Te_SelectAll,
        Te_Unselect,
        Te_Reload,
        Te_ReloadAndBypassCache,
        Te_Stop,
        Te_StopAndUnselect,

        Te_Print,
        Te_Save,
        Te_ZoomIn,
        Te_ZoomOut,
        Te_ViewSource,
        Te_ApplySource,

        Te_CopyUrl,
        Te_CopyTitle,
        Te_CopyPageAsLink,
        Te_CopySelectedHtml,
        Te_OpenWithIE,
        Te_OpenWithFF,
        Te_OpenWithOpera,
        Te_OpenWithOPR,
        Te_OpenWithSafari,
        Te_OpenWithChrome,
        Te_OpenWithSleipnir,
        Te_OpenWithVivaldi,
        Te_OpenWithCustom,
    };

public:
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
    SharedView OpenOnSuitableNode (QNetworkRequest         req, bool activate, ViewNode *older  = 0);
    SharedView OpenOnSuitableNode (QUrl                    url, bool activate, ViewNode *older  = 0);
    SharedView OpenOnSuitableNode (QList<QNetworkRequest> reqs, bool activate, ViewNode *older  = 0);
    SharedView OpenOnSuitableNode (QList<QUrl>            urls, bool activate, ViewNode *older  = 0);
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

    // events
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

    static inline TreeBankAction StringToAction(QString str){
        if(str == QStringLiteral("NoAction"))             return Te_NoAction;

        // key events.
        if(str == QStringLiteral("Up"))                   return Ke_Up;
        if(str == QStringLiteral("Down"))                 return Ke_Down;
        if(str == QStringLiteral("Right"))                return Ke_Right;
        if(str == QStringLiteral("Left"))                 return Ke_Left;
        if(str == QStringLiteral("Home"))                 return Ke_Home;
        if(str == QStringLiteral("End"))                  return Ke_End;
        if(str == QStringLiteral("PageUp"))               return Ke_PageUp;
        if(str == QStringLiteral("PageDown"))             return Ke_PageDown;

        // application events.
        if(str == QStringLiteral("Import"))               return Te_Import;
        if(str == QStringLiteral("Export"))               return Te_Export;
        if(str == QStringLiteral("AboutVanilla"))         return Te_AboutVanilla;
        if(str == QStringLiteral("AboutQt"))              return Te_AboutQt;
        if(str == QStringLiteral("Quit"))                 return Te_Quit;

        // window events.
        if(str == QStringLiteral("ToggleNotifier"))       return Te_ToggleNotifier;
        if(str == QStringLiteral("ToggleReceiver"))       return Te_ToggleReceiver;
        if(str == QStringLiteral("ToggleMenuBar"))        return Te_ToggleMenuBar;
        if(str == QStringLiteral("ToggleTreeBar"))        return Te_ToggleTreeBar;
        if(str == QStringLiteral("ToggleFullScreen"))     return Te_ToggleFullScreen;
        if(str == QStringLiteral("ToggleMaximized"))      return Te_ToggleMaximized;
        if(str == QStringLiteral("ToggleMinimized"))      return Te_ToggleMinimized;
        if(str == QStringLiteral("ToggleShaded"))         return Te_ToggleShaded;
        if(str == QStringLiteral("ShadeWindow"))          return Te_ShadeWindow;
        if(str == QStringLiteral("UnshadeWindow"))        return Te_UnshadeWindow;
        if(str == QStringLiteral("NewWindow"))            return Te_NewWindow;
        if(str == QStringLiteral("CloseWindow"))          return Te_CloseWindow;
        if(str == QStringLiteral("SwitchWindow"))         return Te_SwitchWindow;
        if(str == QStringLiteral("NextWindow"))           return Te_NextWindow;
        if(str == QStringLiteral("PrevWindow"))           return Te_PrevWindow;

        // treebank events.
        if(str == QStringLiteral("Back"))                 return Te_Back;
        if(str == QStringLiteral("Forward"))              return Te_Forward;
        if(str == QStringLiteral("UpDirectory"))          return Te_UpDirectory;
        if(str == QStringLiteral("Close"))                return Te_Close;
        if(str == QStringLiteral("Restore"))              return Te_Restore;
        if(str == QStringLiteral("Recreate"))             return Te_Recreate;
        if(str == QStringLiteral("NextView"))             return Te_NextView;
        if(str == QStringLiteral("PrevView"))             return Te_PrevView;
        if(str == QStringLiteral("BuryView"))             return Te_BuryView;
        if(str == QStringLiteral("DigView"))              return Te_DigView;
        if(str == QStringLiteral("NewViewNode"))          return Te_NewViewNode;
        if(str == QStringLiteral("NewHistNode"))          return Te_NewHistNode;
        if(str == QStringLiteral("CloneViewNode"))        return Te_CloneViewNode;
        if(str == QStringLiteral("CloneHistNode"))        return Te_CloneHistNode;
        if(str == QStringLiteral("DisplayAccessKey"))     return Te_DisplayAccessKey;
        if(str == QStringLiteral("DisplayViewTree"))      return Te_DisplayViewTree;
        if(str == QStringLiteral("DisplayHistTree"))      return Te_DisplayHistTree;
        if(str == QStringLiteral("DisplayTrashTree"))     return Te_DisplayTrashTree;
        if(str == QStringLiteral("OpenTextSeeker"))       return Te_OpenTextSeeker;
        if(str == QStringLiteral("OpenQueryEditor"))      return Te_OpenQueryEditor;
        if(str == QStringLiteral("OpenUrlEditor"))        return Te_OpenUrlEditor;
        if(str == QStringLiteral("OpenCommand"))          return Te_OpenCommand;
        if(str == QStringLiteral("Load"))                 return Te_Load;

        // web events.
        if(str == QStringLiteral("Copy"))                 return Te_Copy;
        if(str == QStringLiteral("Cut"))                  return Te_Cut;
        if(str == QStringLiteral("Paste"))                return Te_Paste;
        if(str == QStringLiteral("Undo"))                 return Te_Undo;
        if(str == QStringLiteral("Redo"))                 return Te_Redo;
        if(str == QStringLiteral("SelectAll"))            return Te_SelectAll;
        if(str == QStringLiteral("Unselect"))             return Te_Unselect;
        if(str == QStringLiteral("Reload"))               return Te_Reload;
        if(str == QStringLiteral("ReloadAndBypassCache")) return Te_ReloadAndBypassCache;
        if(str == QStringLiteral("Stop"))                 return Te_Stop;
        if(str == QStringLiteral("StopAndUnselect"))      return Te_StopAndUnselect;

        if(str == QStringLiteral("Print"))                return Te_Print;
        if(str == QStringLiteral("Save"))                 return Te_Save;
        if(str == QStringLiteral("ZoomIn"))               return Te_ZoomIn;
        if(str == QStringLiteral("ZoomOut"))              return Te_ZoomOut;
        if(str == QStringLiteral("ViewSource"))           return Te_ViewSource;
        if(str == QStringLiteral("ApplySource"))          return Te_ApplySource;

        if(str == QStringLiteral("CopyUrl"))              return Te_CopyUrl;
        if(str == QStringLiteral("CopyTitle"))            return Te_CopyTitle;
        if(str == QStringLiteral("CopyPageAsLink"))       return Te_CopyPageAsLink;
        if(str == QStringLiteral("CopySelectedHtml"))     return Te_CopySelectedHtml;
        if(str == QStringLiteral("OpenWithIE"))           return Te_OpenWithIE;
        if(str == QStringLiteral("OpenWithFF"))           return Te_OpenWithFF;
        if(str == QStringLiteral("OpenWithOpera"))        return Te_OpenWithOpera;
        if(str == QStringLiteral("OpenWithOPR"))          return Te_OpenWithOPR;
        if(str == QStringLiteral("OpenWithSafari"))       return Te_OpenWithSafari;
        if(str == QStringLiteral("OpenWithChrome"))       return Te_OpenWithChrome;
        if(str == QStringLiteral("OpenWithSleipnir"))     return Te_OpenWithSleipnir;
        if(str == QStringLiteral("OpenWithVivaldi"))      return Te_OpenWithVivaldi;
        if(str == QStringLiteral("OpenWithCustom"))       return Te_OpenWithCustom;
                                                          return Te_NoAction;
    }

    static inline QString ActionToString(TreeBankAction action){
        if(action == Te_NoAction)             return QStringLiteral("NoAction");

        // key events.
        if(action == Ke_Up)                   return QStringLiteral("Up");
        if(action == Ke_Down)                 return QStringLiteral("Down");
        if(action == Ke_Right)                return QStringLiteral("Right");
        if(action == Ke_Left)                 return QStringLiteral("Left");
        if(action == Ke_Home)                 return QStringLiteral("Home");
        if(action == Ke_End)                  return QStringLiteral("End");
        if(action == Ke_PageUp)               return QStringLiteral("PageUp");
        if(action == Ke_PageDown)             return QStringLiteral("PageDown");

        // application events.
        if(action == Te_Import)               return QStringLiteral("Import");
        if(action == Te_Export)               return QStringLiteral("Export");
        if(action == Te_AboutVanilla)         return QStringLiteral("AboutVanilla");
        if(action == Te_AboutQt)              return QStringLiteral("AboutQt");
        if(action == Te_Quit)                 return QStringLiteral("Quit");

        // window events.
        if(action == Te_ToggleNotifier)       return QStringLiteral("ToggleNotifier");
        if(action == Te_ToggleReceiver)       return QStringLiteral("ToggleReceiver");
        if(action == Te_ToggleMenuBar)        return QStringLiteral("ToggleMenuBar");
        if(action == Te_ToggleTreeBar)        return QStringLiteral("ToggleTreeBar");
        if(action == Te_ToggleFullScreen)     return QStringLiteral("ToggleFullScreen");
        if(action == Te_ToggleMaximized)      return QStringLiteral("ToggleMaximized");
        if(action == Te_ToggleMinimized)      return QStringLiteral("ToggleMinimized");
        if(action == Te_ToggleShaded)         return QStringLiteral("ToggleShaded");
        if(action == Te_ShadeWindow)          return QStringLiteral("ShadeWindow");
        if(action == Te_UnshadeWindow)        return QStringLiteral("UnshadeWindow");
        if(action == Te_NewWindow)            return QStringLiteral("NewWindow");
        if(action == Te_CloseWindow)          return QStringLiteral("CloseWindow");
        if(action == Te_SwitchWindow)         return QStringLiteral("SwitchWindow");
        if(action == Te_NextWindow)           return QStringLiteral("NextWindow");
        if(action == Te_PrevWindow)           return QStringLiteral("PrevWindow");

        // treebank events.
        if(action == Te_Back)                 return QStringLiteral("Back");
        if(action == Te_Forward)              return QStringLiteral("Forward");
        if(action == Te_UpDirectory)          return QStringLiteral("UpDirectory");
        if(action == Te_Close)                return QStringLiteral("Close");
        if(action == Te_Restore)              return QStringLiteral("Restore");
        if(action == Te_Recreate)             return QStringLiteral("Recreate");
        if(action == Te_NextView)             return QStringLiteral("NextView");
        if(action == Te_PrevView)             return QStringLiteral("PrevView");
        if(action == Te_BuryView)             return QStringLiteral("BuryView");
        if(action == Te_DigView)              return QStringLiteral("DigView");
        if(action == Te_NewViewNode)          return QStringLiteral("NewViewNode");
        if(action == Te_NewHistNode)          return QStringLiteral("NewHistNode");
        if(action == Te_CloneViewNode)        return QStringLiteral("CloneViewNode");
        if(action == Te_CloneHistNode)        return QStringLiteral("CloneHistNode");
        if(action == Te_DisplayAccessKey)     return QStringLiteral("DisplayAccessKey");
        if(action == Te_DisplayViewTree)      return QStringLiteral("DisplayViewTree");
        if(action == Te_DisplayHistTree)      return QStringLiteral("DisplayHistTree");
        if(action == Te_DisplayTrashTree)     return QStringLiteral("DisplayTrashTree");
        if(action == Te_OpenTextSeeker)       return QStringLiteral("OpenTextSeeker");
        if(action == Te_OpenQueryEditor)      return QStringLiteral("OpenQueryEditor");
        if(action == Te_OpenUrlEditor)        return QStringLiteral("OpenUrlEditor");
        if(action == Te_OpenCommand)          return QStringLiteral("OpenCommand");
        if(action == Te_Load)                 return QStringLiteral("Load");

        // default web events.
        if(action == Te_Copy)                 return QStringLiteral("Copy");
        if(action == Te_Cut)                  return QStringLiteral("Cut");
        if(action == Te_Paste)                return QStringLiteral("Paste");
        if(action == Te_Undo)                 return QStringLiteral("Undo");
        if(action == Te_Redo)                 return QStringLiteral("Redo");
        if(action == Te_SelectAll)            return QStringLiteral("SelectAll");
        if(action == Te_Unselect)             return QStringLiteral("Unselect");
        if(action == Te_Reload)               return QStringLiteral("Reload");
        if(action == Te_ReloadAndBypassCache) return QStringLiteral("ReloadAndBypassCache");
        if(action == Te_Stop)                 return QStringLiteral("Stop");
        if(action == Te_StopAndUnselect)      return QStringLiteral("StopAndUnselect");

        if(action == Te_Print)                return QStringLiteral("Print");
        if(action == Te_Save)                 return QStringLiteral("Save");
        if(action == Te_ZoomIn)               return QStringLiteral("ZoomIn");
        if(action == Te_ZoomOut)              return QStringLiteral("ZoomOut");
        if(action == Te_ViewSource)           return QStringLiteral("ViewSource");
        if(action == Te_ApplySource)          return QStringLiteral("ApplySource");

        if(action == Te_CopyUrl)              return QStringLiteral("CopyUrl");
        if(action == Te_CopyTitle)            return QStringLiteral("CopyTitle");
        if(action == Te_CopyPageAsLink)       return QStringLiteral("CopyPageAsLink");
        if(action == Te_CopySelectedHtml)     return QStringLiteral("CopySelectedHtml");
        if(action == Te_OpenWithIE)           return QStringLiteral("OpenWithIE");
        if(action == Te_OpenWithFF)           return QStringLiteral("OpenWithFF");
        if(action == Te_OpenWithOpera)        return QStringLiteral("OpenWithOpera");
        if(action == Te_OpenWithOPR)          return QStringLiteral("OpenWithOPR");
        if(action == Te_OpenWithSafari)       return QStringLiteral("OpenWithSafari");
        if(action == Te_OpenWithChrome)       return QStringLiteral("OpenWithChrome");
        if(action == Te_OpenWithSleipnir)     return QStringLiteral("OpenWithSleipnir");
        if(action == Te_OpenWithVivaldi)      return QStringLiteral("OpenWithVivaldi");
        if(action == Te_OpenWithCustom)       return QStringLiteral("OpenWithCustom");
                                              return QStringLiteral("NoAction");
    }

    static inline bool IsValidAction(QString str){
        return str == ActionToString(StringToAction(str));
    }

    static inline bool IsValidAction(TreeBankAction action){
        return action == StringToAction(ActionToString(action));
    }

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
    void UpDirectory          (HistNode *hn = 0);
    void Close                (ViewNode *vn = 0);
    void Restore              (ViewNode *vn = 0, ViewNode *dir = 0);
    void Recreate             (ViewNode *vn = 0);
    void NextView             (ViewNode *vn = 0);
    void PrevView             (ViewNode *vn = 0);
    void BuryView             (ViewNode *vn = 0);
    void DigView              (ViewNode *vn = 0);
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

    void CopyUrl              (SharedView view = 0);
    void CopyTitle            (SharedView view = 0);
    void CopyPageAsLink       (SharedView view = 0);
    void CopySelectedHtml     (SharedView view = 0);
    void OpenWithIE           (SharedView view = 0);
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
    static QList<Node*> m_NodeDeleteBox;

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


#endif
