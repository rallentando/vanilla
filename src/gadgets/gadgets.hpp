#ifndef GADGETS_HPP
#define GADGETS_HPP

#include "switch.hpp"

#include <QGraphicsObject>

#include "actionmapper.hpp"
#include "view.hpp"
#include "graphicstableview.hpp"

class Node;
class ViewNode;

class Thumbnail;
class NodeTitle;

class SpotLight;

class MainWindow;
class TreeBank;
class AccessibleWebElement;
class AccessKeyBlockLabel;
class QAction;
class QObject;

class Gadgets : public GraphicsTableView , public View {
    Q_OBJECT
    INSTALL_ACTION_MAP(GADGETS, GadgetsAction)

public:
    Gadgets(TreeBank *parent = nullptr);
    ~Gadgets() override;

    enum AccessKeyMode {
        BothHands,
        LeftHand,
        RightHand,
        Custom,
    };

    enum AccessKeyAction {
        OpenMenu,

        ClickElement,
        FocusElement,
        HoverElement,

        OpenInNewViewNode,
        OpenInNewDirectory,
        OpenOnRoot,

        OpenInNewViewNodeBackground,
        OpenInNewDirectoryBackground,
        OpenOnRootBackground,

        OpenInNewViewNodeNewWindow,
        OpenInNewDirectoryNewWindow,
        OpenOnRootNewWindow,

        OpenCommandBar,
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
    void ApplyNodeCollectionTypeSetting();

    void SetNodeCollectionType(NodeCollectionType type) override {
        if(IsDisplayingViewNode()){
            m_ViewNodeCollectionType = type;
        }
    }
    NodeCollectionType GetNodeCollectionType() const override {
        if(IsDisplayingViewNode()){
            return m_ViewNodeCollectionType;
        }
        return Flat;
    }
    void SetStat(QStringList list){
        if(list.isEmpty()) return;
        const int collection = list[0].toInt();
        switch(collection){
        case Flat:
        case Recursive:
        case Foldable:
            m_ViewNodeCollectionType = static_cast<NodeCollectionType>(collection);
            break;
        default:
            m_ViewNodeCollectionType = Flat;
            break;
        }
        SetZoomFactor(list.last().toFloat());
    }
    QStringList GetStat() const {
        return QStringList()
            << QStringLiteral("%1").arg(static_cast<int>(m_ViewNodeCollectionType))
            << QStringLiteral("%1").arg(static_cast<double>(GetZoomFactor()));
    }

public slots:
    void Activate(DisplayType type) override;
    void Deactivate() override;
    void Dismiss();

private slots:
    void DisableAccessKey();
    void OpenNodeTargetCommand();

    void OpenInNew(QUrl);
    void OpenInNew(QList<QUrl>);
    void OpenInNew(QString);
    void OpenInNew(QString, QString);

    void Load() override { View::Load();}
    void Load(const QString &url) override { View::Load(url);}
    void Load(const QUrl &url) override { View::Load(url);}
    void Load(const QNetworkRequest &req) override { View::Load(req);}

    void Download(QString, QString);
    void SeekText(const QString&, View::FindFlags);
    void KeyEvent(QString);

    void TriggerNativeLoadAction(const QUrl&) override;
    void TriggerNativeLoadAction(const QNetworkRequest&,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) override;

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
    bool IsDisplaying(DisplayType type);

    static QMap<QKeySequence, QString> GetThumbListKeyMap();
    static QMap<QKeySequence, QString> GetAccessKeyKeyMap();
    static InputMap::KeyLayers KeyLayers();
    static QMap<QString, QString> GetMouseMap();
    static bool IsValidAccessKey(int);
    static bool IsValidAccessKey(QString);
    static int AccessKeyBlockSize();
    QString IndexToString(int) const;
    int KeyToIndex(int) const;
    int KeyToIndex(QString) const;
    void SetCurrentView(View*);

    bool IsCurrentBlock(const AccessibleWebElement*) const;

    QMenu *CreateNodeMenu() override;
    void RenderBackground(QPainter *painter) override;

    static bool DrawsBackdrop(bool viewToDraw, bool behind, bool blurring);
    static QImage BlurBackdrop(const QImage &image, int radius);

    QGraphicsObject *base() override { return static_cast<QGraphicsObject*>(this);}
    Page *page() override { return nullptr;}

    QUrl url() override { return QUrl();}
    QString html() override { return QString();}
    TreeBank *parent() override;
    void setUrl(const QUrl &) override {}
    void setHtml(const QString &, const QUrl &) override {}
    void setParent(TreeBank *tb) override;

    QSize size() override { return Size().toSize();}
    void resize(QSize size) override { ResizeNotify(size);}

    void show() override { QGraphicsObject::show();}
    void hide() override { QGraphicsObject::hide();}
    void raise() override { setZValue(MAIN_CONTENTS_LAYER);}
    void lower() override { setZValue(HIDDEN_CONTENTS_LAYER);}
    void repaint() override {
        QGraphicsObject::update();
    }
    bool visible() override { return QGraphicsObject::isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) override {
        QGraphicsObject::setFocus(reason);
    }

    void Connect(TreeBank*) override;
    void Disconnect(TreeBank*) override;

    void ZoomIn()  override { ThumbList_ZoomIn();}
    void ZoomOut() override { ThumbList_ZoomOut();}

    void UpKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_MoveToUpperItem();
    }
    void DownKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_MoveToLowerItem();
    }
    void RightKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_MoveToRightItem();
    }
    void LeftKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_MoveToLeftItem();
    }
    void PageDownKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_NextPage();
    }
    void PageUpKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_PrevPage();
    }
    void HomeKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_PrevBlock();
        else
            ThumbList_MoveToFirstItem();
    }
    void EndKeyEvent() override {
        if(IsDisplayingAccessKey())
            AccessKey_NextBlock();
        else
            ThumbList_MoveToLastItem();
    }

    void UpdateThumbnail() override;

    bool TriggerKeyEvent(QKeyEvent *ev) override;
    bool TriggerKeyEvent(QString str) override;

protected:
    void CreateLabelSequence();
    void PrepareAccessKeyBlockLabels();
    void CollectAccessibleWebElement(View *view);

public slots:
    void OnTitleChanged(const QString&) override;
    void OnUrlChanged(const QUrl&) override;

protected:
    void keyPressEvent         (QKeyEvent *ev) override;
    void keyReleaseEvent       (QKeyEvent *ev) override;
    void dragEnterEvent        (QGraphicsSceneDragDropEvent *ev) override;
    void dropEvent             (QGraphicsSceneDragDropEvent *ev) override;
    void dragMoveEvent         (QGraphicsSceneDragDropEvent *ev) override;
    void dragLeaveEvent        (QGraphicsSceneDragDropEvent *ev) override;
    void mousePressEvent       (QGraphicsSceneMouseEvent *ev) override;
    void mouseReleaseEvent     (QGraphicsSceneMouseEvent *ev) override;
    void mouseMoveEvent        (QGraphicsSceneMouseEvent *ev) override;
    void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *ev) override;
    bool TakeMappedPress       (QGraphicsSceneMouseEvent *ev) override;
    void hoverEnterEvent       (QGraphicsSceneHoverEvent *ev) override;
    void hoverLeaveEvent       (QGraphicsSceneHoverEvent *ev) override;
    void hoverMoveEvent        (QGraphicsSceneHoverEvent *ev) override;
    void contextMenuEvent      (QGraphicsSceneContextMenuEvent *ev) override;
    void wheelEvent            (QGraphicsSceneWheelEvent *ev) override;
    void focusInEvent          (QFocusEvent *ev) override;
    void focusOutEvent         (QFocusEvent *ev) override;

private:
    bool TriggerAction(QString str, QVariant data = QVariant()) override;
    using View::TriggerAction;
    void TriggerAction(GadgetsAction a);
    using View::Action;
    QAction *Action(GadgetsAction a);

public:
    static GadgetsAction WithoutRepeat(GadgetsAction a, GadgetsAction &previous, int &count);
    static void InstallAction(QAction *action, GadgetsAction a, QObject *table, TreeBank *tb);
    static void SyncToggle(QAction *action, GadgetsAction a, TreeBank *tb);

    void KeyPressEvent(QKeyEvent *ev) override;
    void KeyReleaseEvent(QKeyEvent *ev) override;
    void MousePressEvent(QMouseEvent *ev) override;
    void MouseReleaseEvent(QMouseEvent *ev) override;
    void MouseMoveEvent(QMouseEvent *ev) override;
    void MouseDoubleClickEvent(QMouseEvent *ev) override;
    void WheelEvent(QWheelEvent *ev) override;

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

    QPixmap m_Backdrop;
    View *m_BackdropOwner;
    QSize m_BackdropSize;
    QSize m_BackdropViewport;
    qreal m_BackdropRatio;
    bool m_BackdropBlurred;
    QRgb m_BackdropVeil;

    int m_AccessKeyCount;
    int m_AccessKeyLastBlockIndex;
    int m_CurrentAccessKeyBlockIndex;

    QStringList m_AccessKeyLabels;
    QString m_AccessKeyCurrentSelection;

    QList<AccessKeyBlockLabel*> m_AccessKeyBlockLabels;
    QList<AccessibleWebElement*> m_AccessibleWebElementCache;

    QMap<GadgetsAction, QAction*> m_ActionTable;

    static bool m_EnableMultiStroke;
    static bool m_EnableBackdropBlur;
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
    ~AccessKeyBlockLabel() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    bool IsNumber() const { return m_IsNumber;}

private:
    int m_Index;
    bool m_IsNumber;
};

#endif
