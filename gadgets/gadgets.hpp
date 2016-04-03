#ifndef GADGETS_HPP
#define GADGETS_HPP

#include "switch.hpp"

#include <QGraphicsObject>

#include "actionmapper.hpp"
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
    INSTALL_ACTION_MAP(GADGETS, GadgetsAction)

public:
    Gadgets(TreeBank *parent = 0);
    ~Gadgets();

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
    NodeCollectionType GetNodeCollectionType() const DECL_OVERRIDE {
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
    QStringList GetStat() const {
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

public:
    bool IsActive();

    static QMap<QKeySequence, QString> GetThumbListKeyMap();
    static QMap<QKeySequence, QString> GetAccessKeyKeyMap();
    static QMap<QString, QString> GetMouseMap();
    static bool IsValidAccessKey(int);
    static bool IsValidAccessKey(QString);
    static int AccessKeyBlockSize();
    QString IndexToString(int) const;
    int KeyToIndex(int) const;
    int KeyToIndex(QString) const;
    void SetCurrentView(View*);

    bool IsCurrentBlock(const AccessibleWebElement*) const;

    QMenu *CreateNodeMenu() DECL_OVERRIDE;
    void RenderBackground(QPainter *painter) DECL_OVERRIDE;

    QGraphicsObject *base() DECL_OVERRIDE { return static_cast<QGraphicsObject*>(this);}
    Page *page() DECL_OVERRIDE { return 0;}

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
            AccessKey_NextBlock();
        else
            ThumbList_NextPage();
    }
    void PageUpKeyEvent() DECL_OVERRIDE {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_PrevPage();
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

    bool TriggerKeyEvent(QKeyEvent *ev) DECL_OVERRIDE;
    bool TriggerKeyEvent(QString str) DECL_OVERRIDE;

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

#endif //ifndef GADGETS_HPP
