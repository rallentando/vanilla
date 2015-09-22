#ifndef GADGETS_HPP
#define GADGETS_HPP

#include "switch.hpp"

#include <QGraphicsObject>
#include <QtConcurrent/QtConcurrent>

#include "view.hpp"
#include "graphicstableview.hpp"

class Node;
class ViewNode;
class HistNode;

class Thumbnail;
class NodeTitle;

class SpotLight;
class ScrollController;

class MainWindow;
class TreeBank;
class AccessibleWebElement;
class AccessKeyBlockLabel;
class QAction;
class QObject;

class Gadgets : public GraphicsTableView , public View{
    Q_OBJECT

public:
    Gadgets(TreeBank *parent = 0);
    ~Gadgets();

    enum GadgetsAction {
        Ge_NoAction,

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
        Ge_Import,
        Ge_Export,
        Ge_AboutVanilla,
        Ge_AboutQt,
        Ge_Quit,

        // window events.
        Ge_ToggleNotifier,
        Ge_ToggleReceiver,
        Ge_ToggleMenuBar,
        Ge_ToggleFullScreen,
        Ge_ToggleMaximized,
        Ge_ToggleMinimized,
        Ge_ToggleShaded,
        Ge_ShadeWindow,
        Ge_UnshadeWindow,
        Ge_NewWindow,
        Ge_CloseWindow,
        Ge_SwitchWindow,
        Ge_NextWindow,
        Ge_PrevWindow,

        // treebank events.
        Ge_Close,
        Ge_Restore,
        Ge_Recreate,
        Ge_NextView,
        Ge_PrevView,
        Ge_BuryView,
        Ge_DigView,
        Ge_NewViewNode,
        Ge_NewHistNode,
        Ge_CloneViewNode,
        Ge_CloneHistNode,
        Ge_DisplayAccessKey,
        Ge_DisplayViewTree,
        Ge_DisplayHistTree,
        Ge_DisplayTrashTree,
        Ge_OpenTextSeeker,
        Ge_OpenQueryEditor,
        Ge_OpenUrlEditor,
        Ge_OpenCommand,

        // gadgets events.
        Ge_Deactivate,
        Ge_Refresh,
        Ge_RefreshNoScroll,
        Ge_OpenNode,
        Ge_DeleteNode,
        Ge_DeleteRightNode,
        Ge_DeleteLeftNode,
        Ge_DeleteOtherNode,
        Ge_PasteNode,
        Ge_RestoreNode,
        Ge_NewNode,
        Ge_CloneNode,
        Ge_UpDirectory,
        Ge_DownDirectory,
        Ge_MakeLocalNode,
        Ge_MakeDirectory,
        Ge_MakeDirectoryWithSelectedNode,
        Ge_MakeDirectoryWithSameDomainNode,
        Ge_RenameNode,
        Ge_CopyNodeUrl,
        Ge_CopyNodeTitle,
        Ge_CopyNodeAsLink,
        Ge_OpenNodeWithIE,
        Ge_OpenNodeWithFF,
        Ge_OpenNodeWithOpera,
        Ge_OpenNodeWithOPR,
        Ge_OpenNodeWithSafari,
        Ge_OpenNodeWithChrome,
        Ge_OpenNodeWithSleipnir,
        Ge_OpenNodeWithVivaldi,
        Ge_OpenNodeWithCustom,
        Ge_ToggleTrash,
        Ge_ScrollUp,
        Ge_ScrollDown,
        Ge_PageUp,
        Ge_PageDown,
        Ge_ZoomIn,
        Ge_ZoomOut,
        Ge_MoveToUpperItem,
        Ge_MoveToLowerItem,
        Ge_MoveToRightItem,
        Ge_MoveToLeftItem,
        Ge_MoveToPrevPage,
        Ge_MoveToNextPage,
        Ge_MoveToFirstItem,
        Ge_MoveToLastItem,
        Ge_SelectToUpperItem,
        Ge_SelectToLowerItem,
        Ge_SelectToRightItem,
        Ge_SelectToLeftItem,
        Ge_SelectToPrevPage,
        Ge_SelectToNextPage,
        Ge_SelectToFirstItem,
        Ge_SelectToLastItem,
        Ge_SelectItem,
        Ge_SelectRange,
        Ge_SelectAll,
        Ge_ClearSelection,
        Ge_TransferToUpper,
        Ge_TransferToLower,
        Ge_TransferToRight,
        Ge_TransferToLeft,
        Ge_TransferToPrevPage,
        Ge_TransferToNextPage,
        Ge_TransferToFirst,
        Ge_TransferToLast,
        Ge_TransferToUpDirectory,
        Ge_TransferToDownDirectory,
        Ge_SwitchNodeCollectionType,
        Ge_SwitchNodeCollectionTypeReverse,
    };

    enum AccessKeyMode {
        BothHands,
        LeftHand,
        RightHand,
        Custom,
    };

    enum AccessKeyAction{
        OpenMenu,

        ClickElement,
        FocusElement,
        HoverElement,

        OpenInNewViewNode,
        OpenInNewHistNode,
        OpenInNewDirectory,
        OpenOnRoot,

        OpenInNewViewNodeBackground,
        OpenInNewHistNodeBackground,
        OpenInNewDirectoryBackground,
        OpenOnRootBackground,

        OpenInNewViewNodeNewWindow,
        OpenInNewHistNodeNewWindow,
        OpenInNewDirectoryNewWindow,
        OpenOnRootNewWindow,
    };

    enum AccessKeySortOrientation {
        Vertical,
        Horizontal
    };

    enum AccessKeySelectBlockMethod {
        Number,
        ShiftedChar,
        CtrledChar,
        AltedChar,
        MetaChar
    };

    static void LoadSettings();
    static void SaveSettings();

    void SetNodeCollectionType(NodeCollectionType type) DECL_OVERRIDE {
        if(IsDisplayingViewNode()){
            m_ViewNodeCollectionType = type;
        } else if(IsDisplayingHistNode()){
            m_HistNodeCollectionType = type;
        }
    }
    NodeCollectionType GetNodeCollectionType() DECL_OVERRIDE {
        if(IsDisplayingViewNode()){
            return m_ViewNodeCollectionType;
        } else if(IsDisplayingHistNode()){
            return m_HistNodeCollectionType;
        }
        return Flat;
    }
    void SetStat(QStringList list){
        m_ViewNodeCollectionType =
            static_cast<NodeCollectionType>(list[0].toInt());
        m_HistNodeCollectionType =
            static_cast<NodeCollectionType>(list[1].toInt());
        SetZoomFactor(list[2].toFloat());
    }
    QStringList GetStat(){
        return QStringList()
            << QStringLiteral("%1").arg(static_cast<int>(m_ViewNodeCollectionType))
            << QStringLiteral("%1").arg(static_cast<int>(m_HistNodeCollectionType))
            << QStringLiteral("%1").arg(GetZoomFactor());
    }

public slots:
    void Activate(DisplayType type) DECL_OVERRIDE;
    void Deactivate() DECL_OVERRIDE;

private slots:
    void DisableAccessKey();

    void OpenInNew(QUrl);
    void OpenInNew(QList<QUrl>);
    void OpenInNew(QString);
    void OpenInNew(QString, QString);

    // lift to slot.
    void Load() DECL_OVERRIDE { View::Load();}
    void Load(const QString &url) DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url) DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) DECL_OVERRIDE { View::Load(req);}

    void Download(QString, QString);
    void SeekText(const QString&, View::FindFlags);
    void KeyEvent(QString);

    void TriggerNativeLoadAction(const QUrl&) DECL_OVERRIDE;
    void TriggerNativeLoadAction(const QNetworkRequest&,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) DECL_OVERRIDE;
    QVariant EvaluateJavaScript(const QString &) DECL_OVERRIDE { return QVariant();}

    void UpKey       ();
    void DownKey     ();
    void RightKey    ();
    void LeftKey     ();
    void HomeKey     ();
    void EndKey      ();
    void PageUpKey   ();
    void PageDownKey ();

private:
    void Update_(QRectF rect){ update(rect);}

public:
    inline void Update(QRectF rect = QRectF()){
#ifdef OUT_OF_THREAD_UPDATE
        QtConcurrent::run(this, &Gadgets::Update_, rect);
#else
        update(rect);
#endif
    }
    bool IsActive();

    static QMap<QKeySequence, QString> GetThumbListKeyMap();
    static QMap<QKeySequence, QString> GetAccessKeyKeyMap();
    static QMap<QString, QString> GetMouseMap();
    static bool IsValidAccessKey(int);
    static bool IsValidAccessKey(QString);
    static int AccessKeyBlockSize();
    QString IndexToString(int);
    int KeyToIndex(int);
    int KeyToIndex(QString);
    void SetCurrentView(View*);

    bool IsCurrentBlock(const AccessibleWebElement*) const;

    QMenu *CreateNodeMenu() DECL_OVERRIDE;
    void RenderBackground(QPainter *painter) DECL_OVERRIDE;

    QGraphicsObject *base() DECL_OVERRIDE { return static_cast<QGraphicsObject*>(this);}
    WebPage *page() DECL_OVERRIDE { return 0;}

    QUrl url() DECL_OVERRIDE { return QUrl();}
    void setUrl(const QUrl &) DECL_OVERRIDE {}
    QString html() DECL_OVERRIDE { return QString();}
    void setHtml(const QString &, const QUrl &) DECL_OVERRIDE {}
    TreeBank *parent() DECL_OVERRIDE;
    void setParent(TreeBank *tb) DECL_OVERRIDE;

    QSize size() DECL_OVERRIDE { return Size().toSize();}
    void resize(QSize size) DECL_OVERRIDE { ResizeNotify(size);}

    void show() DECL_OVERRIDE { QGraphicsObject::show();}
    void hide() DECL_OVERRIDE { QGraphicsObject::hide();}
    void raise() DECL_OVERRIDE { setZValue(MAIN_CONTENTS_LAYER);}
    void lower() DECL_OVERRIDE { setZValue(HIDDEN_CONTENTS_LAYER);}
    void repaint() DECL_OVERRIDE {
        QGraphicsObject::update();
        //QtConcurrent::run(this, &GraphicsTableView::Update, QRectF());
    }
    bool visible() DECL_OVERRIDE { return QGraphicsObject::isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) DECL_OVERRIDE {
        QGraphicsObject::setFocus(reason);
    }

    // dummy DECL_OVERRIDE to avoid infinity loop.
    SharedWebElementList FindElements(Page::FindElementsOption) DECL_OVERRIDE { return SharedWebElementList();}
    SharedWebElement HitElement(const QPoint&) DECL_OVERRIDE { return SharedWebElement();}
    QUrl HitLinkUrl(const QPoint&) DECL_OVERRIDE { return QUrl();}
    QUrl HitImageUrl(const QPoint&) DECL_OVERRIDE { return QUrl();}
    QString SelectedText() DECL_OVERRIDE { return QString();}
    QString SelectedHtml() DECL_OVERRIDE { return QString();}
    QString WholeText() DECL_OVERRIDE { return QString();}
    QString WholeHtml() DECL_OVERRIDE { return QString();}

    void Connect(TreeBank*) DECL_OVERRIDE;
    void Disconnect(TreeBank*) DECL_OVERRIDE;

    void ZoomIn()  DECL_OVERRIDE { ThumbList_ZoomIn();}
    void ZoomOut() DECL_OVERRIDE { ThumbList_ZoomOut();}

    QUrl BaseUrl() DECL_OVERRIDE { return QUrl();}
    QUrl CurrentBaseUrl() DECL_OVERRIDE { return QUrl();}

    void UpKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_MoveToUpperItem();
    }
    void DownKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_MoveToLowerItem();
    }
    void RightKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_MoveToRightItem();
    }
    void LeftKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_MoveToLeftItem();
    }
    void PageDownKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_PageDown();
    }
    void PageUpKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_PageUp();
    }
    void HomeKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_MoveToFirstItem();
    }
    void EndKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_MoveToLastItem();
    }

    void UpdateThumbnail() DECL_OVERRIDE;

    void TriggerKeyEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void TriggerKeyEvent(QString str) DECL_OVERRIDE;

protected:
    void CreateLabelSequence();
    void CollectAccessibleWebElement(View *view);

public slots:
    void OnTitleChanged(const QString&);
    void OnUrlChanged(const QUrl&);

protected:
    void keyPressEvent         (QKeyEvent *ev) DECL_OVERRIDE;
    void keyReleaseEvent       (QKeyEvent *ev) DECL_OVERRIDE;
    void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dropEvent             (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) DECL_OVERRIDE;
    void mousePressEvent       (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) DECL_OVERRIDE;
    void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) DECL_OVERRIDE;
    void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) DECL_OVERRIDE;
    void wheelEvent            (QGraphicsSceneWheelEvent *ev) DECL_OVERRIDE;
    void focusInEvent          (QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent         (QFocusEvent *ev) DECL_OVERRIDE;

private:
    bool TriggerAction(QString str, QVariant data = QVariant()) DECL_OVERRIDE;
    void TriggerAction(GadgetsAction a);
    QAction *Action(GadgetsAction a);

public:
    void KeyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void KeyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void MousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void MouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void MouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void MouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void WheelEvent(QWheelEvent *ev) DECL_OVERRIDE;

public slots:
    void AccessKey_TriggerElementAction(Page::CustomAction);

    void AccessKey_OpenElement(int index);
    void AccessKey_ClearSelection();
    void AccessKey_TriggerAction(AccessibleWebElement *awe, QKeySequence seq);

    void AccessKey_NextBlock();
    void AccessKey_PrevBlock();
    void AccessKey_FirstBlock();
    void AccessKey_LastBlock();
    void AccessKey_NthBlock(int n);

    void AccessKey_SieveLabel();

private:
    NodeCollectionType m_ViewNodeCollectionType;
    NodeCollectionType m_HistNodeCollectionType;

    // for accesskey.
    int m_AccessKeyCount;
    int m_AccessKeyLastBlockIndex;
    int m_CurrentAccessKeyBlockIndex;

    // for multi stroke.
    QStringList m_AccessKeyLabels;
    QString m_AccessKeyCurrentSelection;

    QList<AccessKeyBlockLabel*> m_AccessKeyBlockLabels;
    QList<AccessibleWebElement*> m_AccessibleWebElementCache;

    QMap<GadgetsAction, QAction*> m_ActionTable;

    static bool m_EnableMultiStroke;
    static QString m_AccessKeyCustomSequence;
    static AccessKeyMode m_AccessKeyMode;
    static AccessKeyAction m_AccessKeyAction;
    static AccessKeySortOrientation m_AccessKeySortOrientation;
    static AccessKeySelectBlockMethod m_AccessKeySelectBlockMethod;

    static QMap<QKeySequence, QString> m_ThumbListKeyMap;
    static QMap<QKeySequence, QString> m_AccessKeyKeyMap;
    static QMap<QString, QString> m_MouseMap;

public:
    static inline GadgetsAction StringToAction(QString str){
        if(str == QStringLiteral("NoAction"))                        return Ge_NoAction;

        // key events.
        if(str == QStringLiteral("Up"))                              return Ke_Up;
        if(str == QStringLiteral("Down"))                            return Ke_Down;
        if(str == QStringLiteral("Right"))                           return Ke_Right;
        if(str == QStringLiteral("Left"))                            return Ke_Left;
        if(str == QStringLiteral("Home"))                            return Ke_Home;
        if(str == QStringLiteral("End"))                             return Ke_End;
        if(str == QStringLiteral("PageUp"))                          return Ke_PageUp;
        if(str == QStringLiteral("PageDown"))                        return Ke_PageDown;

        // application events.
        if(str == QStringLiteral("Import"))                          return Ge_Import;
        if(str == QStringLiteral("Export"))                          return Ge_Export;
        if(str == QStringLiteral("AboutVanilla"))                    return Ge_AboutVanilla;
        if(str == QStringLiteral("AboutQt"))                         return Ge_AboutQt;
        if(str == QStringLiteral("Quit"))                            return Ge_Quit;

        // window events.
        if(str == QStringLiteral("ToggleNotifier"))                  return Ge_ToggleNotifier;
        if(str == QStringLiteral("ToggleReceiver"))                  return Ge_ToggleReceiver;
        if(str == QStringLiteral("ToggleMenuBar"))                   return Ge_ToggleMenuBar;
        if(str == QStringLiteral("ToggleFullScreen"))                return Ge_ToggleFullScreen;
        if(str == QStringLiteral("ToggleMaximized"))                 return Ge_ToggleMaximized;
        if(str == QStringLiteral("ToggleMinimized"))                 return Ge_ToggleMinimized;
        if(str == QStringLiteral("ToggleShaded"))                    return Ge_ToggleShaded;
        if(str == QStringLiteral("ShadeWindow"))                     return Ge_ShadeWindow;
        if(str == QStringLiteral("UnshadeWindow"))                   return Ge_UnshadeWindow;
        if(str == QStringLiteral("NewWindow"))                       return Ge_NewWindow;
        if(str == QStringLiteral("CloseWindow"))                     return Ge_CloseWindow;
        if(str == QStringLiteral("SwitchWindow"))                    return Ge_SwitchWindow;
        if(str == QStringLiteral("NextWindow"))                      return Ge_NextWindow;
        if(str == QStringLiteral("PrevWindow"))                      return Ge_PrevWindow;

        // treebank events.
        if(str == QStringLiteral("Close"))                           return Ge_Close;
        if(str == QStringLiteral("Restore"))                         return Ge_Restore;
        if(str == QStringLiteral("Recreate"))                        return Ge_Recreate;
        if(str == QStringLiteral("NextView"))                        return Ge_NextView;
        if(str == QStringLiteral("PrevView"))                        return Ge_PrevView;
        if(str == QStringLiteral("BuryView"))                        return Ge_BuryView;
        if(str == QStringLiteral("DigView"))                         return Ge_DigView;
        if(str == QStringLiteral("NewViewNode"))                     return Ge_NewViewNode;
        if(str == QStringLiteral("NewHistNode"))                     return Ge_NewHistNode;
        if(str == QStringLiteral("CloneViewNode"))                   return Ge_CloneViewNode;
        if(str == QStringLiteral("CloneHistNode"))                   return Ge_CloneHistNode;
        if(str == QStringLiteral("DisplayAccessKey"))                return Ge_DisplayAccessKey;
        if(str == QStringLiteral("DisplayViewTree"))                 return Ge_DisplayViewTree;
        if(str == QStringLiteral("DisplayHistTree"))                 return Ge_DisplayHistTree;
        if(str == QStringLiteral("DisplayTrashTree"))                return Ge_DisplayTrashTree;
        if(str == QStringLiteral("OpenTextSeeker"))                  return Ge_OpenTextSeeker;
        if(str == QStringLiteral("OpenQueryEditor"))                 return Ge_OpenQueryEditor;
        if(str == QStringLiteral("OpenUrlEditor"))                   return Ge_OpenUrlEditor;
        if(str == QStringLiteral("OpenCommand"))                     return Ge_OpenCommand;

        // gadgets events.
        if(str == QStringLiteral("Deactivate"))                      return Ge_Deactivate;
        if(str == QStringLiteral("Refresh"))                         return Ge_Refresh;
        if(str == QStringLiteral("RefreshNoScroll"))                 return Ge_RefreshNoScroll;
        if(str == QStringLiteral("OpenNode"))                        return Ge_OpenNode;
        if(str == QStringLiteral("DeleteNode"))                      return Ge_DeleteNode;
        if(str == QStringLiteral("DeleteRightNode"))                 return Ge_DeleteRightNode;
        if(str == QStringLiteral("DeleteLeftNode"))                  return Ge_DeleteLeftNode;
        if(str == QStringLiteral("DeleteOtherNode"))                 return Ge_DeleteOtherNode;
        if(str == QStringLiteral("PasteNode"))                       return Ge_PasteNode;
        if(str == QStringLiteral("RestoreNode"))                     return Ge_RestoreNode;
        if(str == QStringLiteral("NewNode"))                         return Ge_NewNode;
        if(str == QStringLiteral("CloneNode"))                       return Ge_CloneNode;
        if(str == QStringLiteral("UpDirectory"))                     return Ge_UpDirectory;
        if(str == QStringLiteral("DownDirectory"))                   return Ge_DownDirectory;
        if(str == QStringLiteral("MakeLocalNode"))                   return Ge_MakeLocalNode;
        if(str == QStringLiteral("MakeDirectory"))                   return Ge_MakeDirectory;
        if(str == QStringLiteral("MakeDirectoryWithSelectedNode"))   return Ge_MakeDirectoryWithSelectedNode;
        if(str == QStringLiteral("MakeDirectoryWithSameDomainNode")) return Ge_MakeDirectoryWithSameDomainNode;
        if(str == QStringLiteral("RenameNode"))                      return Ge_RenameNode;
        if(str == QStringLiteral("CopyNodeUrl"))                     return Ge_CopyNodeUrl;
        if(str == QStringLiteral("CopyNodeTitle"))                   return Ge_CopyNodeTitle;
        if(str == QStringLiteral("CopyNodeAsLink"))                  return Ge_CopyNodeAsLink;
        if(str == QStringLiteral("OpenNodeWithIE"))                  return Ge_OpenNodeWithIE;
        if(str == QStringLiteral("OpenNodeWithFF"))                  return Ge_OpenNodeWithFF;
        if(str == QStringLiteral("OpenNodeWithOpera"))               return Ge_OpenNodeWithOpera;
        if(str == QStringLiteral("OpenNodeWithOPR"))                 return Ge_OpenNodeWithOPR;
        if(str == QStringLiteral("OpenNodeWithSafari"))              return Ge_OpenNodeWithSafari;
        if(str == QStringLiteral("OpenNodeWithChrome"))              return Ge_OpenNodeWithChrome;
        if(str == QStringLiteral("OpenNodeWithSleipnir"))            return Ge_OpenNodeWithSleipnir;
        if(str == QStringLiteral("OpenNodeWithVivaldi"))             return Ge_OpenNodeWithVivaldi;
        if(str == QStringLiteral("OpenNodeWithCustom"))              return Ge_OpenNodeWithCustom;
        if(str == QStringLiteral("ToggleTrash"))                     return Ge_ToggleTrash;
        if(str == QStringLiteral("ScrollUp"))                        return Ge_ScrollUp;
        if(str == QStringLiteral("ScrollDown"))                      return Ge_ScrollDown;
        if(str == QStringLiteral("PageUp"))                          return Ge_PageUp;
        if(str == QStringLiteral("PageDown"))                        return Ge_PageDown;
        if(str == QStringLiteral("ZoomIn"))                          return Ge_ZoomIn;
        if(str == QStringLiteral("ZoomOut"))                         return Ge_ZoomOut;
        if(str == QStringLiteral("MoveToUpperItem"))                 return Ge_MoveToUpperItem;
        if(str == QStringLiteral("MoveToLowerItem"))                 return Ge_MoveToLowerItem;
        if(str == QStringLiteral("MoveToRightItem"))                 return Ge_MoveToRightItem;
        if(str == QStringLiteral("MoveToLeftItem"))                  return Ge_MoveToLeftItem;
        if(str == QStringLiteral("MoveToPrevPage"))                  return Ge_MoveToPrevPage;
        if(str == QStringLiteral("MoveToNextPage"))                  return Ge_MoveToNextPage;
        if(str == QStringLiteral("MoveToFirstItem"))                 return Ge_MoveToFirstItem;
        if(str == QStringLiteral("MoveToLastItem"))                  return Ge_MoveToLastItem;
        if(str == QStringLiteral("SelectToUpperItem"))               return Ge_SelectToUpperItem;
        if(str == QStringLiteral("SelectToLowerItem"))               return Ge_SelectToLowerItem;
        if(str == QStringLiteral("SelectToRightItem"))               return Ge_SelectToRightItem;
        if(str == QStringLiteral("SelectToLeftItem"))                return Ge_SelectToLeftItem;
        if(str == QStringLiteral("SelectToPrevPage"))                return Ge_SelectToPrevPage;
        if(str == QStringLiteral("SelectToNextPage"))                return Ge_SelectToNextPage;
        if(str == QStringLiteral("SelectToFirstItem"))               return Ge_SelectToFirstItem;
        if(str == QStringLiteral("SelectToLastItem"))                return Ge_SelectToLastItem;
        if(str == QStringLiteral("SelectItem"))                      return Ge_SelectItem;
        if(str == QStringLiteral("SelectRange"))                     return Ge_SelectRange;
        if(str == QStringLiteral("SelectAll"))                       return Ge_SelectAll;
        if(str == QStringLiteral("ClearSelection"))                  return Ge_ClearSelection;
        if(str == QStringLiteral("TransferToUpper"))                 return Ge_TransferToUpper;
        if(str == QStringLiteral("TransferToLower"))                 return Ge_TransferToLower;
        if(str == QStringLiteral("TransferToRight"))                 return Ge_TransferToRight;
        if(str == QStringLiteral("TransferToLeft"))                  return Ge_TransferToLeft;
        if(str == QStringLiteral("TransferToPrevPage"))              return Ge_TransferToPrevPage;
        if(str == QStringLiteral("TransferToNextPage"))              return Ge_TransferToNextPage;
        if(str == QStringLiteral("TransferToFirst"))                 return Ge_TransferToFirst;
        if(str == QStringLiteral("TransferToLast"))                  return Ge_TransferToLast;
        if(str == QStringLiteral("TransferToUpDirectory"))           return Ge_TransferToUpDirectory;
        if(str == QStringLiteral("TransferToDownDirectory"))         return Ge_TransferToDownDirectory;
        if(str == QStringLiteral("SwitchNodeCollectionType"))        return Ge_SwitchNodeCollectionType;
        if(str == QStringLiteral("SwitchNodeCollectionTypeReverse")) return Ge_SwitchNodeCollectionTypeReverse;
                                                                     return Ge_NoAction;
    }

    static inline QString ActionToString(GadgetsAction action){
        if(action == Ge_NoAction)                        return QStringLiteral("NoAction");

        // key events.
        if(action == Ke_Up)                              return QStringLiteral("Up");
        if(action == Ke_Down)                            return QStringLiteral("Down");
        if(action == Ke_Right)                           return QStringLiteral("Right");
        if(action == Ke_Left)                            return QStringLiteral("Left");
        if(action == Ke_Home)                            return QStringLiteral("Home");
        if(action == Ke_End)                             return QStringLiteral("End");
        if(action == Ke_PageUp)                          return QStringLiteral("PageUp");
        if(action == Ke_PageDown)                        return QStringLiteral("PageDown");

        // application events.
        if(action == Ge_Import)                          return QStringLiteral("Import");
        if(action == Ge_Export)                          return QStringLiteral("Export");
        if(action == Ge_AboutVanilla)                    return QStringLiteral("AboutVanilla");
        if(action == Ge_AboutQt)                         return QStringLiteral("AboutQt");
        if(action == Ge_Quit)                            return QStringLiteral("Quit");

        // window events.
        if(action == Ge_ToggleNotifier)                  return QStringLiteral("ToggleNotifier");
        if(action == Ge_ToggleReceiver)                  return QStringLiteral("ToggleReceiver");
        if(action == Ge_ToggleMenuBar)                   return QStringLiteral("ToggleMenuBar");
        if(action == Ge_ToggleFullScreen)                return QStringLiteral("ToggleFullScreen");
        if(action == Ge_ToggleMaximized)                 return QStringLiteral("ToggleMaximized");
        if(action == Ge_ToggleMinimized)                 return QStringLiteral("ToggleMinimized");
        if(action == Ge_ToggleShaded)                    return QStringLiteral("ToggleShaded");
        if(action == Ge_ShadeWindow)                     return QStringLiteral("ShadeWindow");
        if(action == Ge_UnshadeWindow)                   return QStringLiteral("UnshadeWindow");
        if(action == Ge_NewWindow)                       return QStringLiteral("NewWindow");
        if(action == Ge_CloseWindow)                     return QStringLiteral("CloseWindow");
        if(action == Ge_SwitchWindow)                    return QStringLiteral("SwitchWindow");
        if(action == Ge_NextWindow)                      return QStringLiteral("NextWindow");
        if(action == Ge_PrevWindow)                      return QStringLiteral("PrevWindow");

        // treebank events.
        if(action == Ge_Close)                           return QStringLiteral("Close");
        if(action == Ge_Restore)                         return QStringLiteral("Restore");
        if(action == Ge_Recreate)                        return QStringLiteral("Recreate");
        if(action == Ge_NextView)                        return QStringLiteral("NextView");
        if(action == Ge_PrevView)                        return QStringLiteral("PrevView");
        if(action == Ge_BuryView)                        return QStringLiteral("BuryView");
        if(action == Ge_DigView)                         return QStringLiteral("DigView");
        if(action == Ge_NewViewNode)                     return QStringLiteral("NewViewNode");
        if(action == Ge_NewHistNode)                     return QStringLiteral("NewHistNode");
        if(action == Ge_CloneViewNode)                   return QStringLiteral("CloneViewNode");
        if(action == Ge_CloneHistNode)                   return QStringLiteral("CloneHistNode");
        if(action == Ge_DisplayAccessKey)                return QStringLiteral("DisplayAccessKey");
        if(action == Ge_DisplayViewTree)                 return QStringLiteral("DisplayViewTree");
        if(action == Ge_DisplayHistTree)                 return QStringLiteral("DisplayHistTree");
        if(action == Ge_DisplayTrashTree)                return QStringLiteral("DisplayTrashTree");
        if(action == Ge_OpenTextSeeker)                  return QStringLiteral("OpenTextSeeker");
        if(action == Ge_OpenQueryEditor)                 return QStringLiteral("OpenQueryEditor");
        if(action == Ge_OpenUrlEditor)                   return QStringLiteral("OpenUrlEditor");
        if(action == Ge_OpenCommand)                     return QStringLiteral("OpenCommand");

        // gadgets events.
        if(action == Ge_Deactivate)                      return QStringLiteral("Deactivate");
        if(action == Ge_Refresh)                         return QStringLiteral("Refresh");
        if(action == Ge_RefreshNoScroll)                 return QStringLiteral("RefreshNoScroll");
        if(action == Ge_OpenNode)                        return QStringLiteral("OpenNode");
        if(action == Ge_DeleteNode)                      return QStringLiteral("DeleteNode");
        if(action == Ge_DeleteRightNode)                 return QStringLiteral("DeleteRightNode");
        if(action == Ge_DeleteLeftNode)                  return QStringLiteral("DeleteLeftNode");
        if(action == Ge_DeleteOtherNode)                 return QStringLiteral("DeleteOtherNode");
        if(action == Ge_PasteNode)                       return QStringLiteral("PasteNode");
        if(action == Ge_RestoreNode)                     return QStringLiteral("RestoreNode");
        if(action == Ge_NewNode)                         return QStringLiteral("NewNode");
        if(action == Ge_CloneNode)                       return QStringLiteral("CloneNode");
        if(action == Ge_UpDirectory)                     return QStringLiteral("UpDirectory");
        if(action == Ge_DownDirectory)                   return QStringLiteral("DownDirectory");
        if(action == Ge_MakeLocalNode)                   return QStringLiteral("MakeLocalNode");
        if(action == Ge_MakeDirectory)                   return QStringLiteral("MakeDirectory");
        if(action == Ge_MakeDirectoryWithSelectedNode)   return QStringLiteral("MakeDirectoryWithSelectedNode");
        if(action == Ge_MakeDirectoryWithSameDomainNode) return QStringLiteral("MakeDirectoryWithSameDomainNode");
        if(action == Ge_RenameNode)                      return QStringLiteral("RenameNode");
        if(action == Ge_CopyNodeUrl)                     return QStringLiteral("CopyNodeUrl");
        if(action == Ge_CopyNodeTitle)                   return QStringLiteral("CopyNodeTitle");
        if(action == Ge_CopyNodeAsLink)                  return QStringLiteral("CopyNodeAsLink");
        if(action == Ge_OpenNodeWithIE)                  return QStringLiteral("OpenNodeWithIE");
        if(action == Ge_OpenNodeWithFF)                  return QStringLiteral("OpenNodeWithFF");
        if(action == Ge_OpenNodeWithOpera)               return QStringLiteral("OpenNodeWithOpera");
        if(action == Ge_OpenNodeWithOPR)                 return QStringLiteral("OpenNodeWithOPR");
        if(action == Ge_OpenNodeWithSafari)              return QStringLiteral("OpenNodeWithSafari");
        if(action == Ge_OpenNodeWithChrome)              return QStringLiteral("OpenNodeWithChrome");
        if(action == Ge_OpenNodeWithSleipnir)            return QStringLiteral("OpenNodeWithSleipnir");
        if(action == Ge_OpenNodeWithVivaldi)             return QStringLiteral("OpenNodeWithVivaldi");
        if(action == Ge_OpenNodeWithCustom)              return QStringLiteral("OpenNodeWithCustom");
        if(action == Ge_ToggleTrash)                     return QStringLiteral("ToggleTrash");
        if(action == Ge_ScrollUp)                        return QStringLiteral("ScrollUp");
        if(action == Ge_ScrollDown)                      return QStringLiteral("ScrollDown");
        if(action == Ge_PageUp)                          return QStringLiteral("PageUp");
        if(action == Ge_PageDown)                        return QStringLiteral("PageDown");
        if(action == Ge_ZoomIn)                          return QStringLiteral("ZoomIn");
        if(action == Ge_ZoomOut)                         return QStringLiteral("ZoomOut");
        if(action == Ge_MoveToUpperItem)                 return QStringLiteral("MoveToUpperItem");
        if(action == Ge_MoveToLowerItem)                 return QStringLiteral("MoveToLowerItem");
        if(action == Ge_MoveToRightItem)                 return QStringLiteral("MoveToRightItem");
        if(action == Ge_MoveToLeftItem)                  return QStringLiteral("MoveToLeftItem");
        if(action == Ge_MoveToPrevPage)                  return QStringLiteral("MoveToPrevPage");
        if(action == Ge_MoveToNextPage)                  return QStringLiteral("MoveToNextPage");
        if(action == Ge_MoveToFirstItem)                 return QStringLiteral("MoveToFirstItem");
        if(action == Ge_MoveToLastItem)                  return QStringLiteral("MoveToLastItem");
        if(action == Ge_SelectToUpperItem)               return QStringLiteral("SelectToUpperItem");
        if(action == Ge_SelectToLowerItem)               return QStringLiteral("SelectToLowerItem");
        if(action == Ge_SelectToRightItem)               return QStringLiteral("SelectToRightItem");
        if(action == Ge_SelectToLeftItem)                return QStringLiteral("SelectToLeftItem");
        if(action == Ge_SelectToPrevPage)                return QStringLiteral("SelectToPrevPage");
        if(action == Ge_SelectToNextPage)                return QStringLiteral("SelectToNextPage");
        if(action == Ge_SelectToFirstItem)               return QStringLiteral("SelectToFirstItem");
        if(action == Ge_SelectToLastItem)                return QStringLiteral("SelectToLastItem");
        if(action == Ge_SelectItem)                      return QStringLiteral("SelectItem");
        if(action == Ge_SelectRange)                     return QStringLiteral("SelectRange");
        if(action == Ge_SelectAll)                       return QStringLiteral("SelectAll");
        if(action == Ge_ClearSelection)                  return QStringLiteral("ClearSelection");
        if(action == Ge_TransferToUpper)                 return QStringLiteral("TransferToUpper");
        if(action == Ge_TransferToLower)                 return QStringLiteral("TransferToLower");
        if(action == Ge_TransferToRight)                 return QStringLiteral("TransferToRight");
        if(action == Ge_TransferToLeft)                  return QStringLiteral("TransferToLeft");
        if(action == Ge_TransferToPrevPage)              return QStringLiteral("TransferToPrevPage");
        if(action == Ge_TransferToNextPage)              return QStringLiteral("TransferToNextPage");
        if(action == Ge_TransferToFirst)                 return QStringLiteral("TransferToFirst");
        if(action == Ge_TransferToLast)                  return QStringLiteral("TransferToLast");
        if(action == Ge_TransferToUpDirectory)           return QStringLiteral("TransferToUpDirectory");
        if(action == Ge_TransferToDownDirectory)         return QStringLiteral("TransferToDownDirectory");
        if(action == Ge_SwitchNodeCollectionType)        return QStringLiteral("SwitchNodeCollectionType");
        if(action == Ge_SwitchNodeCollectionTypeReverse) return QStringLiteral("SwitchNodeCollectionTypeReverse");
                                                         return QStringLiteral("NoAction");
    }

    static inline bool IsValidAction(QString str){
        return str == ActionToString(StringToAction(str));
    }

    static inline bool IsValidAction(GadgetsAction action){
        return action == StringToAction(ActionToString(action));
    }
};

class AccessKeyBlockLabel : public QGraphicsRectItem {

public:
    AccessKeyBlockLabel(QGraphicsItem *parent, int index, bool isNumber);
    ~AccessKeyBlockLabel();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) DECL_OVERRIDE;

private:
    int m_Index;
    bool m_IsNumber;
};

#endif
