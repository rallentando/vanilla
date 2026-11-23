#ifndef VIEW_HPP
#define VIEW_HPP

#include "switch.hpp"

#include "inputmap.hpp"

#include <functional>
#include <QEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QMenu>
#include <QContextMenuEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QNetworkAccessManager>
#include <QFile>

#include <memory>

#include "callback.hpp"
#include "spentbuttons.hpp"
#include "application.hpp"
#include "page.hpp"
#include "webelement.hpp"

class QKeySequence;
class QMimeData;
class QWidgetAction;

class View;
class ExtensionController;
class SiteDataLink;

typedef std::shared_ptr<View> SharedView;
typedef std::weak_ptr<View>   WeakView;
typedef QList<std::shared_ptr<View>> SharedViewList;
typedef QList<std::weak_ptr<View>>   WeakViewList;

class RenderProcessLedger {

public:
    enum class Verdict {
        Recover,
        GiveUp,
        Silent,
    };

    static constexpr int Limit = 10;

    Verdict Count(){
        if(m_GaveUp) return Verdict::Silent;
        if(++m_Deaths >= Limit){
            m_GaveUp = true;
            return Verdict::GiveUp;
        }
        return Verdict::Recover;
    }

    void Clear(){
        m_Deaths = 0;
        m_GaveUp = false;
    }

    int Deaths() const { return m_Deaths;}
    bool GaveUp() const { return m_GaveUp;}

private:
    int m_Deaths = 0;
    bool m_GaveUp = false;
};

class View {

public:
    View(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    virtual ~View();
    virtual ExtensionController *Extensions() const { return nullptr; }
    virtual QString ExtensionStatus() const;
    enum ExtensionPage { ExtensionActionPage, ExtensionOptionsPage, ExtensionSidePanelPage };
    virtual bool MakesSidePanels() const { return false; }
    static QEvent::Type SidePanelShutdownEvent(){
        static const QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
        return type;
    }
    class SidePanelNavigate : public QEvent {
    public:
        explicit SidePanelNavigate(const QUrl &url) : QEvent(Type()), m_Url(url) { setAccepted(false); }
        static QEvent::Type Type(){
            static const QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
            return type;
        }
        QUrl Url() const { return m_Url;}
    private:
        QUrl m_Url;
    };
    virtual QWidget *CreateExtensionView(const QUrl &, ExtensionPage, QWidget *,
                                         const std::function<void()> & = std::function<void()>()) { return nullptr; }

    enum GestureVector {
        Gv_Up,
        Gv_Down,
        Gv_Right,
        Gv_Left,
        Gv_UpperRight,
        Gv_UpperLeft,
        Gv_LowerRight,
        Gv_LowerLeft,
        Gv_NoMove
    };
    typedef QList<GestureVector> Gesture;

    enum FindFlag {
        FindBackward                      = 1 << 0,
        CaseSensitively                   = 1 << 1,
        WrapsAroundDocument               = 1 << 2,
        HighlightAllOccurrences           = 1 << 3,
        FindAtWordBeginningsOnly          = 1 << 4,
        TreatMedialCapitalAsWordBeginning = 1 << 5,
        FindBeginsInSelection             = 1 << 6
    };
    Q_DECLARE_FLAGS(FindFlags, FindFlag);

    enum ScrollBarState {
        NoScrollBarEnabled,
        HorizontalScrollBarEnabled,
        VerticalScrollBarEnabled,
        BothScrollBarEnabled,
    };
    static void SetScrollBarStateFrom(const QVariant &answer);
    static ScrollBarState GetScrollBarState(){ return m_ScrollBarState;}
    typedef std::function<QRect(const QRect&)> RectPlacer;
    static SharedWebElementList ElementsFrom(View *provider, const QVariant &answer,
                                             const QRect &viewport, const RectPlacer &place = nullptr);
    static QRegion SelectionRegionFrom(const QVariant &answer, const QRect &viewport,
                                       const RectPlacer &place = nullptr);

    virtual QObject *base();
    virtual QObject *page();

    virtual QString GetViewTypeName();

    void Initialize();
    virtual void DeleteLater();

    void Orphan();
    void ForgetViewNode(ViewNode *vn){ if(m_ViewNode == vn) m_ViewNode = nullptr;}

    virtual bool KeepForDownloads(){ return false;}

    TreeBank  *GetTreeBank() const;
    ViewNode  *GetViewNode() const;
    WeakView   GetThis() const;
    WeakView   GetMaster() const;
    WeakView   GetSlave() const;

    void SetTreeBank(TreeBank*);
    void SetViewNode(ViewNode*);
    void SetThis(WeakView);
    void SetMaster(WeakView);
    void SetSlave(WeakView);

    void FollowInferedUrl(const QUrl &target);
    virtual void GoBackToInferedUrl();
    virtual void GoForwardToInferedUrl();
    static QUrl InferedUrl(const QUrl &current, const SharedWebElementList &elements, bool forward);

    QMimeData *CreateMimeDataFromSelection(NetworkAccessManager *nam);
    QMimeData *CreateMimeDataFromElement(NetworkAccessManager *nam);
    QPixmap CreatePixmapFromSelection();
    QPixmap CreatePixmapFromElement();

    QMenu *BookmarkletMenu();
    QMenu *SearchMenu();
    QMenu *OpenWithOtherBrowserMenu();
    QMenu *OpenLinkWithOtherBrowserMenu(QVariant data);
    QMenu *OpenImageWithOtherBrowserMenu(QVariant data);
    QMenu *OpenMediaWithOtherBrowserMenu(QVariant data);
    void AddExternalCommandActions(QMenu *menu, const char *slot, QVariant data);

    void AddContextMenu(QMenu *menu, SharedWebElement elem, Page::MediaType type = Page::MediaTypeNone);
    void AddExtensionMenu(QMenu *menu, SharedWebElement elem, Page::MediaType type = Page::MediaTypeNone);
    void AddRegularMenu(QMenu *menu, SharedWebElement elem);
    static QWidgetAction *MenuCaption(QMenu *menu, const QString &text, Qt::Alignment align);
    static void AddMenuCommandCell(QMenu *menu, QAction *action);

    virtual void AddSpellCheckMenu(QMenu*){}

    bool GetDisplayObscured(){ return m_DisplayObscured;}
    void SetDisplayObscured(bool obscured){ m_DisplayObscured = obscured;}

    static void LoadSettings();
    static void SaveSettings();
    virtual void ApplySpecificSettings(QStringList set);
    QStringList SpecificSettings() const { return m_SpecificSettings;}
    virtual void RebuildForOffTheRecord();
    virtual bool IsOffTheRecord();
    virtual QObject *EngineProfile(){ return nullptr;}
    virtual QString EngineProfileName(){ return QString();}
    bool KeepsOffDisk();

    static bool ActivateNewViewDefault(){ return m_ActivateNewViewDefault;}
    static bool EnableSingleKeyShortcut(){ return m_EnableSingleKeyShortcut;}
    static bool DragToStartDownload(){ return m_DragToStartDownload;}
    static bool EnableDestinationInferrer(){ return m_EnableDestinationInferrer;}
    static bool EnableDragGesture(){ return m_EnableDragGesture;}
    static bool EnableRightGesture(){ return m_EnableMouseGesture;}
    static bool InspectorInMainWindow(){ return m_InspectorInMainWindow;}
    static QString SuspendHiddenViews(){ return m_SuspendHiddenViews;}
    static bool HiddenViewsStayActive(){
        return m_SuspendHiddenViews != QStringLiteral("Frozen") &&
               m_SuspendHiddenViews != QStringLiteral("Discarded");
    }

    static bool SavePasswords(){ return m_SavePasswords;}
    static QString TrackingPrevention(){ return m_TrackingPrevention;}
    static bool SmartScreen(){ return m_SmartScreen;}

    static QString GetLinkMenu(){ return m_LinkMenu;}
    static QString GetImageMenu(){ return m_ImageMenu;}
    static QString GetMediaMenu(){ return m_MediaMenu;}
    static QString GetTextMenu(){ return m_TextMenu;}
    static QString GetSelectionMenu(){ return m_SelectionMenu;}
    static QString GetRegularMenu(){ return m_RegularMenu;}
    static QString MenuSelectedText(){ return m_SelectedText;}

    static const QList<float> &GetZoomFactorLevels(){ return m_ZoomFactorLevels;}

    static qreal DeviceZoomScale();

    class DragOutScope {
    public:
        DragOutScope(){ m_DraggingOut++;}
        ~DragOutScope(){ m_DraggingOut--;}
        DragOutScope(const DragOutScope&) = delete;
        DragOutScope &operator=(const DragOutScope&) = delete;
    };
    static bool IsDraggingOut(){ return m_DraggingOut > 0;}
    static bool GetSwitchingState(){ return m_Switching;}
    static void SetSwitchingState(bool switching){ m_Switching = switching;}

    static void CloseLater(WeakView weak,
                           std::function<bool()> stillWanted = std::function<bool()>());

    static void ConsumeRightButton(){ m_RightButtonConsumed = true;}
    static bool TakeRightButtonConsumed(){
        const bool consumed = m_RightButtonConsumed;
        m_RightButtonConsumed = false;
        return consumed;
    }

    static QPoint EngineWheelAngle(const QPoint &angle){
        return QPoint(angle.x() * 5 / 3, angle.y() * 5 / 3);
    }

    virtual QUrl url(){ return QUrl();}
    virtual QString html(){ return QString();}
    virtual TreeBank *parent(){ return 0;}
    virtual void setUrl(const QUrl&){}
    virtual void setHtml(const QString&, const QUrl&){}
    virtual void setParent(TreeBank*){}

    QString GestureStatusLine(const QMap<QString, QString> &map, const QString &noAction);

    struct MoveHooks {
        QObject *dragParent = nullptr;
        std::function<void(QMouseEvent*)> forward;
        std::function<NetworkAccessManager*()> networkAccessManager;
        std::function<void()> status;
        bool acceptForwardedWhileDragging = false;
    };
    void MouseMoveWithGestures(QMouseEvent *ev, const MoveHooks &hooks);

    struct ButtonHooks {
        std::function<void(QMouseEvent*)> pressToEngine;
        std::function<void(QMouseEvent*)> releaseToEngine;
        std::function<void(QMouseEvent*)> doubleClickToEngine;
        bool acceptForwarded = false;
        std::function<void()> clearStatus;
    };
    void MousePressWithMap(QMouseEvent *ev, const ButtonHooks &hooks);
    void MouseDoubleClickWithMap(QMouseEvent *ev, const ButtonHooks &hooks);
    void MouseReleaseWithGestures(QMouseEvent *ev, const ButtonHooks &hooks);

    struct DropHooks {
        std::function<void(QDropEvent*)> toEngine;
        std::function<void()> clearStatus;
    };
    void DropWithGestures(QDropEvent *ev, const DropHooks &hooks);

    struct WheelHooks {
        std::function<void(QWheelEvent*)> toEngine;
        std::function<QPointF(QWheelEvent*)> actionPos;
    };
    void WheelWithMap(QWheelEvent *ev, const WheelHooks &hooks);

    virtual void Connect(TreeBank*);
    virtual void Disconnect(TreeBank*);
    void WireWebView(QObject *view, QObject *linkSource, TreeBank *tb, bool on);

    virtual void UpdateThumbnail();

    virtual bool ForbidToOverlap(){ return false;}

    bool IsLoading(){ return m_IsLoading;}
    quint64 LoadSerial() const { return m_LoadSerial;}
    static bool TakesKey(const QKeySequence &seq){ return !KeyAction(seq).isEmpty();}
    static QString KeyAction(const QKeySequence &seq);
    static QMap<QKeySequence, QString> GetKeyMap(){ return m_KeyMap;}
    static InputMap::KeyLayers KeyLayers();
    RenderProcessLedger &RenderProcessDeaths(){ return m_RenderProcessLedger;}
    virtual bool CanGoBack(){ return false;}
    virtual bool CanGoForward(){ return false;}
    virtual bool RecentlyAudible(){ return false;}
    virtual bool IsAudioMuted(){ return false;}
    virtual void SetAudioMuted(bool){}

    virtual void Copy(){}
    virtual void Cut(){}
    virtual void Paste(){}
#define VANILLA_EDIT_ACTION(name) virtual void name(){}
    FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION
    virtual void Undo(){}
    virtual void Redo(){}
    virtual void Unselect(){}
    virtual void SelectAll(){}
    virtual void Reload(){}
    virtual void ReloadAndBypassCache(){}
    virtual void Stop(){}
    void UploadStarted(const QUrl &url);
    virtual void StopAndUnselect(){}
    virtual void Print(){}
    virtual void Save(){}
    virtual void ZoomIn(){}
    virtual void ZoomOut(){}

    virtual void ToggleMediaControls(){}
    virtual void ToggleMediaLoop(){}
    virtual void ToggleMediaPlayPause(){}
    virtual void ToggleMediaMute(){}

    virtual bool IsOwnDragSource(QObject*) const { return false;}

    static void MarkOwnDrop(){ m_OwnDropTimer.start();}
    static bool TakeOwnDrop(){
        if(!m_OwnDropTimer.isValid()) return false;
        const bool fresh = m_OwnDropTimer.elapsed() < OWN_DROP_TO_NEW_WINDOW_MSEC;
        m_OwnDropTimer.invalidate();
        return fresh;
    }

    virtual void ExitFullScreen(){}
    virtual void InspectElement(){}
    virtual QWidget *InspectorPane(){ return nullptr;}
    virtual SiteDataLink *OpenSiteDataLink(){ return nullptr;}
    virtual void AddSearchEngine(QPoint){}
    virtual void AddBookmarklet(QPoint){}

    virtual bool IsRenderable(){ return false;}
    virtual void Render(QPainter*){}
    virtual void Render(QPainter*, const QRegion&){}
    virtual QImage CaptureVisible(){ return QImage();}
    virtual QUrl CommittedUrl(){ return IsLoading() ? QUrl() : url();}
    virtual QSize GetViewportSize(){ return QSize();}
    virtual void SetViewportSize(QSize){}
    virtual void SetSource(const QUrl&){}
    virtual void SetSource(const QByteArray&){}
    virtual void SetSource(const QString&){}
    virtual QString GetTitle(){ return QString();}
    virtual QIcon GetIcon(){ return QIcon();}

    virtual bool TriggerAction(QString str, QVariant data = QVariant());
    virtual void TriggerAction(Page::CustomAction, QVariant = QVariant()){}

    virtual QAction *Action(QString str, QVariant data = QVariant());
    virtual QAction *Action(Page::CustomAction, QVariant = QVariant()){ return 0;}

    virtual void TriggerNativeLoadAction(const QUrl&){}
    virtual void TriggerNativeLoadAction(const QNetworkRequest&,
                                         QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation,
                                         const QByteArray & = QByteArray()){}
    virtual void TriggerNativeGoBackAction(){}
    virtual void TriggerNativeGoForwardAction(){}
    virtual void TriggerNativeRewindAction(){}
    virtual void TriggerNativeFastForwardAction(){}

    virtual void UpKeyEvent(){ RunMoveScript(UpKeyEventJsCode());}
    virtual void DownKeyEvent(){ RunMoveScript(DownKeyEventJsCode());}
    virtual void RightKeyEvent(){ RunMoveScript(RightKeyEventJsCode());}
    virtual void LeftKeyEvent(){ RunMoveScript(LeftKeyEventJsCode());}
    virtual void PageDownKeyEvent(){ RunMoveScript(PageDownKeyEventJsCode());}
    virtual void PageUpKeyEvent(){ RunMoveScript(PageUpKeyEventJsCode());}
    virtual void HomeKeyEvent(){ RunMoveScript(HomeKeyEventJsCode());}
    virtual void EndKeyEvent(){ RunMoveScript(EndKeyEventJsCode());}
    virtual void RunMoveScript(const QString &code){
        CallWithEvaluatedJavaScriptResult(code, [this](QVariant){ EmitScrollChanged();});
    }

    virtual void KeyPressEvent(QKeyEvent*){}
    virtual void KeyReleaseEvent(QKeyEvent*){}
    virtual void MousePressEvent(QMouseEvent*){}
    virtual void MouseReleaseEvent(QMouseEvent*){}
    virtual void MouseMoveEvent(QMouseEvent*){}
    virtual void MouseDoubleClickEvent(QMouseEvent*){}
    virtual void WheelEvent(QWheelEvent*){}

    virtual void CallWithGotBaseUrl(UrlCallBack callBack){ callBack(QUrl());}
    virtual void CallWithGotCurrentBaseUrl(UrlCallBack callBack){ callBack(QUrl());}

    virtual void CallWithFoundElements(Page::FindElementsOption, WebElementListCallBack callBack){
        callBack(SharedWebElementList());
    }
    virtual void CallWithHitElement(const QPoint&, WebElementCallBack callBack){
        callBack(SharedWebElement());
    }

    virtual void CallWithHitLinkUrl(const QPoint&, UrlCallBack callBack){ callBack(QUrl());}
    virtual void CallWithHitImageUrl(const QPoint&, UrlCallBack callBack){ callBack(QUrl());}

    virtual void CallWithSelectedText(StringCallBack callBack){ callBack(QString());}
    virtual void CallWithSelectedHtml(StringCallBack callBack){ callBack(QString());}

    virtual void CallWithWholeText(StringCallBack callBack){ callBack(QString());}
    virtual void CallWithWholeHtml(StringCallBack callBack){ callBack(QString());}

    virtual void CallWithSelectionRegion(RegionCallBack callBack){ callBack(QRegion());}

    virtual void CallWithEvaluatedJavaScriptResult(const QString&, VariantCallBack callBack){
        callBack(QVariant());
    }

    QString WholeHtml(){
        return WaitForResult<QString>([&](StringCallBack callBack){
            CallWithWholeHtml(callBack);});
    }

    virtual QSize size() = 0;
    virtual void resize(QSize) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void raise() = 0;
    virtual void lower() = 0;
    virtual void repaint() = 0;
    virtual bool visible() = 0;
    virtual void setFocus(Qt::FocusReason = Qt::OtherFocusReason) = 0;
    virtual void TakeKeyboardBack(){}

    virtual void Load();
    virtual void Load(const QString &url);
    virtual void Load(const QUrl &url);
    virtual void Load(const QNetworkRequest &req);

    void OnFocusIn();
    void OnFocusOut();

    virtual void OnBeforeStartingDisplayGadgets(){}
    virtual void OnAfterFinishingDisplayGadgets(){}

    virtual void ApplyTheme(){}

    virtual void OnSetViewNode(ViewNode*){}
    virtual void OnSetThis(WeakView){}
    virtual void OnSetMaster(WeakView){}
    virtual void OnSetSlave(WeakView){}
    virtual void OnLoadStarted();
    virtual void OnLoadProgress(int){}
    virtual void OnLoadFinished(bool ok);
    virtual void OnTitleChanged(const QString&){}
    virtual void OnUrlChanged(const QUrl&){}
    virtual void OnViewChanged(){}
    virtual void OnScrollChanged(){}

    virtual void EmitScrollChanged(){}

    virtual void SetScrollBarState(){}
    virtual QPointF GetScroll(){ return QPointF();}
    virtual void SetScroll(QPointF){}

    virtual bool PageGeometry(QSizeF*, QRectF*){ return false;}

    virtual bool SaveScroll(){ return false;}
    virtual bool RestoreScroll(){ return false;}
    virtual bool SaveZoom(){ return false;}
    virtual bool RestoreZoom(){ return false;}
    virtual float MinimumZoom() const { return GetZoomFactorLevels().first();}
    virtual float MaximumZoom() const { return GetZoomFactorLevels().last();}
    float FitZoom(float zoom) const { return qBound(MinimumZoom(), zoom, MaximumZoom());}
    static constexpr float ChromiumMinimumZoom = 0.25f;
    static constexpr float ChromiumMaximumZoom = 5.0f;
    virtual bool SaveHistory(){ return false;}
    virtual bool RestoreHistory(){ return false;}
    void RestoreHistoryOrLoad(const QNetworkRequest &request);
    virtual bool SaveMediaTime(VoidCallBack settled = VoidCallBack()){
        if(settled) settled();
        return false;
    }
    virtual bool RestoreMediaTime(){ return false;}

protected:
    void GestureStarted(QPoint);
    void GestureMoved(QPoint);
    void GestureAborted();
    void GestureFinished(QPoint, Qt::MouseButton);

    virtual bool TriggerKeyEvent(QKeyEvent*);
    virtual bool TriggerKeyEvent(QString);
    bool RunExtensionCommand(const QKeySequence &seq, const QString &key, bool repeat);

    void ChangeNodeTitle(const QString &title);
    void ChangeNodeUrl(const QUrl &url);

    void SaveViewState();
    void RestoreViewState();

    float PrepareForZoomIn();
    float PrepareForZoomOut();

public:
    static float StepZoom(float zoom, bool in, float min, float max);

protected:
    static inline GestureVector GetGestureVector4(int dx, int dy){
        if(dy >  dx && dy > -dx) return Gv_Up;
        if(dy <  dx && dy < -dx) return Gv_Down;
        if(dy >  dx && dy < -dx) return Gv_Right;
        if(dy <  dx && dy > -dx) return Gv_Left;
        return Gv_NoMove;
    }

    static inline GestureVector GetGestureVector8(int dx, int dy){

        if(2*dy >  5*dx && 2*dy > -5*dx) return Gv_Up;
        if(2*dy <  5*dx && 2*dy < -5*dx) return Gv_Down;
        if(5*dy >  2*dx && 5*dy < -2*dx) return Gv_Right;
        if(5*dy <  2*dx && 5*dy > -2*dx) return Gv_Left;
        if(5*dy > -2*dx && 2*dy < -5*dx) return Gv_UpperRight;
        if(2*dy <  5*dx && 5*dy >  2*dx) return Gv_UpperLeft;
        if(2*dy >  5*dx && 5*dy <  2*dx) return Gv_LowerRight;
        if(5*dy < -2*dx && 2*dy > -5*dx) return Gv_LowerLeft;
        return Gv_NoMove;
    }

    static inline GestureVector GetGestureVector(int dx, int dy){
        if(m_GestureMode == 4){
            return GetGestureVector4(dx, dy);
        }
        if(m_GestureMode == 8){
            return GetGestureVector8(dx, dy);
        }
        return Gv_NoMove;
    }

    static inline QString GestureToChar(GestureVector vec){
        if(vec == Gv_Up)         return QStringLiteral("U");
        if(vec == Gv_Down)       return QStringLiteral("D");
        if(vec == Gv_Right)      return QStringLiteral("R");
        if(vec == Gv_Left)       return QStringLiteral("L");
        if(vec == Gv_UpperRight) return QStringLiteral("UR");
        if(vec == Gv_UpperLeft)  return QStringLiteral("UL");
        if(vec == Gv_LowerRight) return QStringLiteral("DR");
        if(vec == Gv_LowerLeft)  return QStringLiteral("DL");
        return QStringLiteral("N");
    }

    static inline QString GestureToString(Gesture vecs){
        const QString comma = QStringLiteral(",");
        QStringList lis;
        foreach(GestureVector vec, vecs){
            lis << GestureToChar(vec);
        }
        return lis.join(comma);
    }

public:

    static inline QString EscapeJsStringLiteral(const QString &str){
        QString escaped;
        escaped.reserve(str.size());
        for(int i = 0; i < str.size(); i++){
            const QChar c = str.at(i);
            switch(c.unicode()){
            case '\\': escaped += QStringLiteral("\\\\"); continue;
            case '"' : escaped += QStringLiteral("\\\"");  continue;
            case '\b': escaped += QStringLiteral("\\b");  continue;
            case '\f': escaped += QStringLiteral("\\f");  continue;
            case '\n': escaped += QStringLiteral("\\n");  continue;
            case '\r': escaped += QStringLiteral("\\r");  continue;
            case '\t': escaped += QStringLiteral("\\t");  continue;
            default: break;
            }
            if(c.unicode() < 0x20 || c.unicode() == 0x2028 || c.unicode() == 0x2029){
                escaped += QStringLiteral("\\u%1")
                    .arg(static_cast<uint>(c.unicode()), 4, 16, QLatin1Char('0'));
                continue;
            }
            escaped += c;
        }
        return escaped;
    }

    static QString GetBaseUrlJsCode();

    static QString GetCurrentBaseUrlJsCode();

    static QString UpKeyEventJsCode();
    static QString DownKeyEventJsCode();
    static QString RightKeyEventJsCode();
    static QString LeftKeyEventJsCode();
    static QString PageDownKeyEventJsCode();
    static QString PageUpKeyEventJsCode();
    static QString HomeKeyEventJsCode();
    static QString EndKeyEventJsCode();

    static QString ElementAtPointJsCode();

    static QString ElementPathJsCode();

    static QString ResolveElementPathJsCode();

    static QString CollectElementsJsCode();

    static QString SetFocusToElementJsCode(const QString &xpath);

    static QString FireClickEventJsCode(const QString &xpath, const QPoint &pos);

    static QString GetScrollValuePointJsCode();

    static QString SetScrollValuePointJsCode(const QPoint &pos);

    static QString GetMediaTimeJsCode();

    static QString SetMediaTimeJsCode(float time);

    static QString GetScrollBarStateJsCode();

    static QString GetScrollRatioPointJsCode();

    static QString SetScrollRatioPointJsCode(const QPointF &pos);

    static QString FindElementsJsCode(Page::FindElementsOption option);

    static QString HitElementJsCode(QPoint pos);

    static QString HitLinkUrlJsCode(QPoint pos);

    static QString HitImageUrlJsCode(QPoint pos);

    static QString SelectedTextJsCode();

    static QString SelectedHtmlJsCode();

    static QString WholeTextJsCode();

    static QString WholeHtmlJsCode();

    static QString SelectionRegionJsCode();

    static QString SetTextValueJsCode(const QString &xpath, const QString &text);

    static QString ExecCommandJsCode(const QString &command);

    static QString ChangeTextDirectionJsCode(const QString &direction);

    static QString MediaAtPointJsCode(const QPoint &point);

    static QString MediaAtJsFunction();
    static QString MediaElementJsCode(const QString &statement,
                                      const QPoint &point = QPoint(-1, -1));

    static QString ToggleMediaControlsJsCode(const QPoint &point = QPoint(-1, -1));
    static QString ToggleMediaLoopJsCode(const QPoint &point = QPoint(-1, -1));
    static QString ToggleMediaPlayPauseJsCode(const QPoint &point = QPoint(-1, -1));
    static QString ToggleMediaMuteAtJsCode(const QPoint &point);

    static QString JsKeyCodeSet(const QList<int> &codes);

    static QString FocusedElementJsCode();

    static inline QString EdgeBridgeTag(){
        return QLatin1Char('"') + Application::EdgeEventToken() + QLatin1Char('"');
    }

    static QString EdgeInputBridgeJsCode(const QList<int> &plain,
                                         const QList<int> &shifted);

    static QString EdgeScrollReportJsCode();

    static QString EdgeHideWebViewJsCode();

    static QString ExtensionPopupSizeJsCode();

    static QString EdgeExtensionTabQueryJsCode();

    static QString InstallEventFilterJsCode(const QList<QEvent::Type> &types);

private:

    template <class T>
    T WaitForResult(std::function<void(std::function<void(T)>)> callBack){
        T result;
        QTimer timer;
        QEventLoop loop;
        bool called = false;

        timer.setSingleShot(true);
        QObject::connect(base(), &QObject::destroyed, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout,    &loop, &QEventLoop::quit);

        callBack([&](T t){
            result = t;
            called = true;
            loop.quit();
        });
        if(!called){
            timer.start(10000);
            loop.exec();
        } else {
        }
        return result;
    }

protected:
    static SharedWebElement m_ClickedElement;
    static QRegion m_SelectionRegion;
    static QUrl m_CurrentBaseUrl;
    static QString m_SelectedText;
    static QString m_SelectedHtml;
    static bool m_DragStarted;
    static bool m_HadSelection;
    static bool m_Switching;
    static int m_DraggingOut;
    static bool m_RightButtonConsumed;
    static QElapsedTimer m_OwnDropTimer;
    static QPoint m_GestureStartedPos;
    static SpentButtons m_SpentButtons;
    static QPoint m_BeforeGesturePos;
    static Gesture m_Gesture;
    static GestureVector m_CurrentGestureVector;
    static GestureVector m_BeforeGestureVector;
    static int m_SameGestureVectorCount;
    static int m_GestureMode;
    static ScrollBarState m_ScrollBarState;

    static QMap<QKeySequence, QString> m_KeyMap;
    static QMap<QString, QString> m_MouseMap;
    static QMap<QString, QString> m_DragGestureMap;
    static QMap<QString, QString> m_RightGestureMap;
    static QMap<QString, QString> m_ScrollGestureMap;

    static bool m_ActivateNewViewDefault;
    static bool m_EnableSingleKeyShortcut;
    static bool m_DragToStartDownload;
    static bool m_EnableDestinationInferrer;
    static bool m_EnableDragGesture;
    static bool m_EnableMouseGesture;
    static bool m_EnableScrollGesture;
    static bool m_InspectorInMainWindow;
    static QString m_SuspendHiddenViews;
    static bool m_SavePasswords;
    static QString m_TrackingPrevention;
    static bool m_SmartScreen;
    bool m_EnableDragGestureLocal;
    bool m_EnableRightGestureLocal;

    static const QList<float> m_ZoomFactorLevels;

    static QString m_LinkMenu;
    static QString m_ImageMenu;
    static QString m_MediaMenu;
    static QString m_TextMenu;
    static QString m_SelectionMenu;
    static QString m_RegularMenu;

    TreeBank *m_TreeBank;
    ViewNode *m_ViewNode;
    WeakView  m_This;
    WeakView  m_Master;
    WeakView  m_Slave;
    QObject  *m_Page;
    bool m_IsLoading;
    quint64 m_LoadSerial = 0;
    bool m_DisplayObscured;
    RenderProcessLedger m_RenderProcessLedger;
    QStringList m_SpecificSettings;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(View::FindFlags);

#endif
