#ifndef VIEW_HPP
#define VIEW_HPP

#include "switch.hpp"

#include <QTimer>
#include <QElapsedTimer>
#include <QMenu>
#include <QContextMenuEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QNetworkAccessManager>
#include <QFile>

#include <memory>

#include "callback.hpp"
#include "application.hpp"
#include "page.hpp"
#include "webelement.hpp"

class QKeySequence;
class QMimeData;

class View;
class _View;
class _Vanilla;

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

    virtual QObject *base();
    virtual QObject *page();

    virtual QString GetViewTypeName();

    void Initialize();
    void DeleteLater();

    TreeBank  *GetTreeBank() const;
    ViewNode  *GetViewNode() const;
    WeakView   GetThis() const;
    WeakView   GetMaster() const;
    WeakView   GetSlave() const;
    _View     *GetJsObject() const;

    void SetTreeBank(TreeBank*);
    void SetViewNode(ViewNode*);
    void SetThis(WeakView);
    void SetMaster(WeakView);
    void SetSlave(WeakView);

    void GoBackTo(QUrl);
    void GoForwardTo(QUrl);
    virtual void GoBackToInferedUrl();
    virtual void GoForwardToInferedUrl();

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
    void AddRegularMenu(QMenu *menu, SharedWebElement elem);

    virtual void AddSpellCheckMenu(QMenu*){}

    bool GetDisplayObscured(){ return m_DisplayObscured;}
    void SetDisplayObscured(bool obscured){ m_DisplayObscured = obscured;}

    static void LoadSettings();
    static void SaveSettings();
    virtual void ApplySpecificSettings(QStringList set);
    QStringList SpecificSettings() const { return m_SpecificSettings;}
    void RebuildForOffTheRecord();

    static bool ActivateNewViewDefault(){ return m_ActivateNewViewDefault;}
    static bool NavigationBySpaceKey(){ return m_NavigationBySpaceKey;}
    static bool DragToStartDownload(){ return m_DragToStartDownload;}
    static bool EnableDestinationInferrer(){ return m_EnableDestinationInferrer;}
    static bool EnableDragGesture(){ return m_EnableDragGesture;}
    bool EnableDragGestureLocal() const { return m_EnableDragGestureLocal;}
    static bool InspectorInMainWindow(){ return m_InspectorInMainWindow;}
    static QString SuspendHiddenViews(){ return m_SuspendHiddenViews;}
    static bool HiddenViewsStayActive(){
        return m_SuspendHiddenViews != QStringLiteral("Frozen") &&
               m_SuspendHiddenViews != QStringLiteral("Discarded");
    }

    static QString GetLinkMenu(){ return m_LinkMenu;}
    static QString GetImageMenu(){ return m_ImageMenu;}
    static QString GetMediaMenu(){ return m_MediaMenu;}
    static QString GetTextMenu(){ return m_TextMenu;}
    static QString GetSelectionMenu(){ return m_SelectionMenu;}
    static QString GetRegularMenu(){ return m_RegularMenu;}

    static const QList<float> &GetZoomFactorLevels(){ return m_ZoomFactorLevels;}

    static qreal DeviceZoomScale();

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

    virtual void Connect(TreeBank*);
    virtual void Disconnect(TreeBank*);

    virtual void UpdateThumbnail();

    virtual bool ForbidToOverlap(){ return false;}

    int LoadProgress(){ return m_LoadProgress;}
    bool IsLoading(){ return m_IsLoading;}
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
    virtual void AddSearchEngine(QPoint){}
    virtual void AddBookmarklet(QPoint){}

    virtual bool IsRenderable(){ return false;}
    virtual void Render(QPainter*){}
    virtual void Render(QPainter*, const QRegion&){}
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

    virtual void UpKeyEvent(){}
    virtual void DownKeyEvent(){}
    virtual void RightKeyEvent(){}
    virtual void LeftKeyEvent(){}
    virtual void PageDownKeyEvent(){}
    virtual void PageUpKeyEvent(){}
    virtual void HomeKeyEvent(){}
    virtual void EndKeyEvent(){}

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
    virtual void OnSetJsObject(_View*){}
    virtual void OnSetJsObject(_Vanilla*){}
    virtual void OnLoadStarted(){ m_LoadProgress = 0; m_IsLoading = true;}
    virtual void OnLoadProgress(int progress){ m_LoadProgress = progress;}
    virtual void OnLoadFinished(bool ok){
        m_LoadProgress = 100;
        m_IsLoading = false;
        if(ok) m_RenderProcessLedger.Clear();
    }
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
    virtual bool SaveHistory(){ return false;}
    virtual bool RestoreHistory(){ return false;}
#ifdef MEDIATIME
    virtual bool SaveMediaTime(){ return false;}
    virtual bool RestoreMediaTime(){ return false;}
#endif

protected:
    void GestureStarted(QPoint);
    void GestureMoved(QPoint);
    void GestureAborted();
    void GestureFinished(QPoint, Qt::MouseButton);

    virtual bool TriggerKeyEvent(QKeyEvent*);
    virtual bool TriggerKeyEvent(QString);

    void ChangeNodeTitle(const QString &title);
    void ChangeNodeUrl(const QUrl &url);

    void SaveViewState();
    void RestoreViewState();

    float PrepareForZoomIn();
    float PrepareForZoomOut();

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

    static inline GestureVector CharToGesture(QString str){
        if(str == QStringLiteral("U"))  return Gv_Up;
        if(str == QStringLiteral("D"))  return Gv_Down;
        if(str == QStringLiteral("R"))  return Gv_Right;
        if(str == QStringLiteral("L"))  return Gv_Left;
        if(str == QStringLiteral("UR")) return Gv_UpperRight;
        if(str == QStringLiteral("UL")) return Gv_UpperLeft;
        if(str == QStringLiteral("DR")) return Gv_LowerRight;
        if(str == QStringLiteral("DL")) return Gv_LowerLeft;
        return Gv_NoMove;
    }

    static inline QString GestureToString(Gesture vecs){
        const QString comma = QStringLiteral(",");
        QStringList lis;
        foreach(GestureVector vec, vecs){
            lis << GestureToChar(vec);
        }
        return lis.join(comma);
    }

    static inline Gesture StringToGesture(QString strs){
        const QString comma = QStringLiteral(",");
        Gesture lis;
        foreach(QString str, strs.split(comma)){
            lis << CharToGesture(str);
        }
        return lis;
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

    static inline QString GetBaseUrlJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    var baseUrl = \"\";\n"
            "    var baseDocument = \"index.html\";\n"
            "    var base = document.getElementsByTagName(\"base\");\n"
            "    if(base.length > 0 && base[0].href){\n"
            "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
            "    } else {\n"
            "        baseUrl = location.href;\n"
            "    }\n"
            "    return baseUrl;\n"
            "})();");
    }

    static inline QString GetCurrentBaseUrlJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    var doc = document;\n"
            "    for(var i = 0; i < frames.length; i++){\n"
            "        try{\n"
            "            if(frames[i].document.hasFocus()){\n"
            "                doc = frames[i].document;\n"
            "            }\n"
            "        }\n"
            "        catch(e){}\n"
            "    }\n"
            "    var baseUrl = \"\";\n"
            "    var baseDocument = \"index.html\";\n"
            "    var base = doc.getElementsByTagName(\"base\");\n"
            "    if(base.length > 0 && base[0].href){\n"
            "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
            "    } else {\n"
            "        baseUrl = doc.location.href;\n"
            "    }\n"
            "    return baseUrl;\n"
            "})();");
    }

    static inline QString UpKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollTop -= 40;\n"
            "    body.scrollTop -= 40;\n"
            "})();");
    }
    static inline QString DownKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollTop += 40;\n"
            "    body.scrollTop += 40;\n"
            "})();");
    }
    static inline QString RightKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollLeft += 40;\n"
            "    body.scrollLeft += 40;\n"
            "})();");
    }
    static inline QString LeftKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollLeft -= 40;\n"
            "    body.scrollLeft -= 40;\n"
            "})();");
    }
    static inline QString PageDownKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollTop += elem.clientHeight * 0.9;\n"
            "    body.scrollTop += body.clientHeight * 0.9;\n"
            "})();");
    }
    static inline QString PageUpKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollTop -= elem.clientHeight * 0.9;\n"
            "    body.scrollTop -= body.clientHeight * 0.9;\n"
            "})();");
    }
    static inline QString HomeKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    elem.scrollTop = 0;\n"
            "    body.scrollTop = 0;\n"
            "})();");
    }
    static inline QString EndKeyEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
            "    if(vmax <= 0) \n"
            "        vmax = body.scrollHeight - body.clientHeight;\n"
            "    elem.scrollTop = vmax;\n"
            "    body.scrollTop = vmax;\n"
            "})();");
    }

    static inline QString SetFocusToElementJsCode(const QString &xpath){
        QString quoted = EscapeJsStringLiteral(xpath);
        return QStringLiteral(
            "(function(){\n"
            "    var elem;\n"
            "    var doc = document;\n"
            "    var xpaths = \"%1\".split(\",\");\n"
            "    for(var i = 0; i < xpaths.length; i++){\n"
            "        elem = doc.evaluate(xpaths[i], doc, null, 7, null).snapshotItem(0);\n"
            "        try{\n"
            "            if(elem.contentDocument){\n"
            "                doc = elem.contentDocument;\n"
            "            }\n"
            "        }\n"
            "        catch(e){ break;}\n"
            "    }\n"
            "    elem.focus();\n"
            "})();").arg(quoted);
    }

    static inline QString FireClickEventJsCode(const QString &xpath, const QPoint &pos){
        QString quoted = EscapeJsStringLiteral(xpath);
        return QStringLiteral(
            "(function(){\n"
            "    var elem;\n"
            "    var doc = document;\n"
            "    var xpaths = \"%1\".split(\",\");\n"
            "    for(var i = 0; i < xpaths.length; i++){\n"
            "        elem = doc.evaluate(xpaths[i], doc, null, 7, null).snapshotItem(0);\n"
            "        try{\n"
            "            if(elem.contentDocument){\n"
            "                doc = elem.contentDocument;\n"
            "            }\n"
            "        }\n"
            "        catch(e){ break;}\n"
            "    }\n"
            "    var event = new MouseEvent(\"click\", {\n"
            "        bubbles: true,\n"
            "        cancelable: true,\n"
            "        view: doc.defaultView,\n"
            "        clientX: %2,\n"
            "        clientY: %3,\n"
            "    });\n"
            "    elem.dispatchEvent(event);\n"
            "})();").arg(quoted, QString::number(pos.x()), QString::number(pos.y()));
    }

    static inline QString GetScrollValuePointJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    var hval = elem.scrollLeft || body.scrollLeft;\n"
            "    var vval = elem.scrollTop || body.scrollTop;\n"
            "    return [hval, vval];\n"
            "})();");
    }

    static inline QString SetScrollValuePointJsCode(const QPoint &pos){
        return QStringLiteral("scrollTo(%1, %2);").arg(pos.x()).arg(pos.y());
    }

#ifdef MEDIATIME
    static inline QString GetMediaTimeJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    var vids = document.querySelectorAll(\"video\");\n"
            "    var time = 0;\n"
            "    for(var i = 0; i < vids.length; i++){\n"
            "        var v = vids[i];\n"
            "        if(!isFinite(v.duration) || v.duration <= 0) continue;\n"
            "        if(v.currentTime > time) time = v.currentTime;\n"
            "    }\n"
            "    return time;\n"
            "})();");
    }

    static inline QString SetMediaTimeJsCode(float time){
        return QStringLiteral(
            "(function(){\n"
            "    var target = %1;\n"
            "    var tries = 40;\n"
            "    function apply(){\n"
            "        var vids = document.querySelectorAll(\"video\");\n"
            "        for(var i = 0; i < vids.length; i++){\n"
            "            var v = vids[i];\n"
            "            if(!isFinite(v.duration) || v.duration < target) continue;\n"
            "            v.currentTime = target;\n"
            "            return true;\n"
            "        }\n"
            "        return false;\n"
            "    }\n"
            "    function tick(){\n"
            "        if(apply() || --tries <= 0) return;\n"
            "        setTimeout(tick, 500);\n"
            "    }\n"
            "    tick();\n"
            "})();").arg(static_cast<double>(time));
    }
#endif

    static inline QString GetScrollBarStateJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    var hmax = elem.scrollWidth - elem.clientWidth;\n"
            "    if(hmax <= 0) \n"
            "        hmax = body.scrollWidth - body.clientWidth;\n"
            "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
            "    if(vmax <= 0) \n"
            "        vmax = body.scrollHeight - body.clientHeight;\n"
            "    return [hmax, vmax];\n"
            "})();");
    }

    static inline QString GetScrollRatioPointJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    var hval = elem.scrollLeft || body.scrollLeft;\n"
            "    var vval = elem.scrollTop || body.scrollTop;\n"
            "    var hmax = elem.scrollWidth - elem.clientWidth;\n"
            "    if(hmax <= 0) \n"
            "        hmax = body.scrollWidth - body.clientWidth;\n"
            "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
            "    if(vmax <= 0) \n"
            "        vmax = body.scrollHeight - body.clientHeight;\n"
            "    return [hmax <= 0 ? 0.5 : hval / hmax,\n"
            "            vmax <= 0 ? 0.5 : vval / vmax];\n"
            "})();");
    }

    static inline QString SetScrollRatioPointJsCode(const QPointF &pos){
        return QStringLiteral(
            "(function(){\n"
            "    if(!document.body || !document.documentElement) return;\n"
            "    var elem = document.documentElement;\n"
            "    var body = document.body;\n"
            "    var hmax = elem.scrollWidth - elem.clientWidth;\n"
            "    if(hmax <= 0) \n"
            "        hmax = body.scrollWidth - body.clientWidth;\n"
            "    var vmax = elem.scrollHeight - elem.clientHeight;\n"
            "    if(vmax <= 0) \n"
            "        vmax = body.scrollHeight - body.clientHeight;\n"
            "    var hval = hmax * %1;\n"
            "    var vval = vmax * %2;\n"
            "    scrollTo(hval, vval);\n"
            "})();").arg(pos.x()).arg(pos.y());
    }

    static inline QString FindElementsJsCode(Page::FindElementsOption option){
        QString quoted = EscapeJsStringLiteral(Page::OptionToSelector(option));
        QString ignoreOutOfView = option == Page::ForAccessKey ? QStringLiteral("true") : QStringLiteral("false");
#if defined(Q_OS_WIN)
        QString fix = QStringLiteral("devicePixelRatio");
#elif defined(Q_OS_MAC)
        QString fix = QStringLiteral("(document.body.clientWidth / innerWidth)");
#else
        QString fix = QStringLiteral("1");
#endif

        return QStringLiteral(
            "(function(){\n"
            "    var scrollX = document.documentElement.scrollLeft || document.body.scrollLeft;\n"
            "    var scrollY = document.documentElement.scrollTop || document.body.scrollTop;\n"
            "    var baseUrl = \"\";\n"
            "    var baseDocument = \"index.html\";\n"
            "    var base = document.getElementsByTagName(\"base\");\n"
            "    if(base.length > 0 && base[0].href){\n"
            "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
            "    } else {\n"
            "        baseUrl = \n"
            "            location.protocol + \"//\" + location.hostname + \n"
            "            (location.port && \":\" + location.port) + \"/\";\n"
            "    }\n"
            "    var elems = Array.from(document.querySelectorAll(\"%1\"));\n"
            "    var map = {};\n"
            "    for(var i = 0; i < elems.length; i++){\n"
            "        var data = {};\n"
            "        data.tagName = elems[i].tagName;\n"
            "        data.innerText = elems[i].innerText;\n"
            "        data.linkUrl = elems[i].href;\n"
            "        data.linkHtml = elems[i].outerHTML;\n"
            "        data.imageUrl = elems[i].src;\n"
            "        data.imageHtml = elems[i].innerHTML;\n"
            "        data.baseUrl = baseUrl;\n"
            "        var offsetX = 0;\n"
            "        var offsetY = 0;\n"
            "        var win = elems[i].ownerDocument.defaultView;\n"
            "        while(win && top !== win){\n"
            "            var rect = win.frameElement.getBoundingClientRect();\n"
            "            offsetX += rect.left;\n"
            "            offsetY += rect.top;\n"
            "            win = win.parent;\n"
            "        }\n"
            "        var rect = elems[i].getBoundingClientRect();\n"
            "        data.x = (rect.left + offsetX) * %3;\n"
            "        data.y = (rect.top  + offsetY) * %3;\n"
            "        data.width  = rect.width  * %3;\n"
            "        data.height = rect.height * %3;\n"
            "        data.region = {};\n"
            "        if(data.width && data.height){\n"
            "            var rects = elems[i].getClientRects();\n"
            "            for(var j = 0; j < rects.length; j++){\n"
            "                data.region[j] = {};\n"
            "                data.region[j].x = (rects[j].left + offsetX) * %3;\n"
            "                data.region[j].y = (rects[j].top  + offsetY) * %3;\n"
            "                data.region[j].width  = rects[j].width  * %3;\n"
            "                data.region[j].height = rects[j].height * %3;\n"
            "            }\n"
            "            var w = elems[i].ownerDocument.defaultView;\n"
            "            var r1 = { left: 0, top: 0, right: w.innerWidth, bottom: w.innerHeight};\n"
            "            var r2 = elems[i].getBoundingClientRect();\n"
            "            if(%2 &&\n"
            "               (Math.max(r1.left, r2.left) >= Math.min(r1.right,  r2.right) ||\n"
            "                Math.max(r1.top,  r2.top)  >= Math.min(r1.bottom, r2.bottom))){\n"
            "                continue;\n"
            "            }\n"
            "        }\n"
            "        data.isJsCommand = \n"
            "            (elems[i].onclick ||\n"
            "             (elems[i].href &&\n"
            "              elems[i].href.lastIndexOf &&\n"
            "              elems[i].href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
            "             (elems[i].getAttribute &&\n"
            "              elems[i].getAttribute(\"role\") &&\n"
            "              (elems[i].getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"tab\"))) ? true : false;\n"
            "        data.isTextInput = \n"
            "            (elems[i].tagName == \"TEXTAREA\" ||\n"
            "             (elems[i].tagName == \"INPUT\" &&\n"
            "              elems[i].type &&\n"
            "              (elems[i].type.toLowerCase() == \"text\" ||\n"
            "               elems[i].type.toLowerCase() == \"search\" ||\n"
            "               elems[i].type.toLowerCase() == \"password\"))) ? true : false;\n"
            "        data.isQueryInput =\n"
            "            (elems[i].tagName == \"INPUT\" &&\n"
            "             elems[i].type &&\n"
            "             (elems[i].type.toLowerCase() == \"text\" ||\n"
            "              elems[i].type.toLowerCase() == \"search\")) ? true : false;\n"
            "        data.isEditable = \n"
            "            (elems[i].isContentEditable ||\n"
            "             data.isTextInput ||\n"
            "             data.isQueryInput) ? true : false;\n"
            "        data.isFrame = \n"
            "            (elems[i].tagName == \"FRAME\" ||\n"
            "             elems[i].tagName == \"IFRAME\") ? true : false;\n"
            "        data.isLooped = elems[i].loop;\n"
            "        data.isPaused = elems[i].paused;\n"
            "        data.isMuted = elems[i].muted;\n"
            "        data.action = \n"
            "            (elems[i].href &&\n"
            "             elems[i].href.lastIndexOf &&\n"
            "             (elems[i].href.lastIndexOf(\"http:\", 0) === 0 ||\n"
            "              elems[i].href.lastIndexOf(\"https:\", 0) === 0)) ? \"None\" :\n"
            "            (elems[i].isContentEditable ||\n"
            "             elems[i].tagName == \"TEXTAREA\" ||\n"
            "             elems[i].tagName == \"OBJECT\"   ||\n"
            "             elems[i].tagName == \"EMBED\"    ||\n"
            "             elems[i].tagName == \"FRAME\"    ||\n"
            "             elems[i].tagName == \"IFRAME\"   ||\n"
            "             (elems[i].tagName == \"INPUT\" &&\n"
            "              elems[i].type &&\n"
            "              (elems[i].type.toLowerCase() == \"text\" ||\n"
            "               elems[i].type.toLowerCase() == \"search\" ||\n"
            "               elems[i].type.toLowerCase() == \"password\"))) ? \"Focus\" :\n"
            "            (elems[i].onclick ||\n"
            "             (elems[i].href &&\n"
            "              elems[i].href.lastIndexOf &&\n"
            "              elems[i].href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
            "             elems[i].tagName == \"BUTTON\" ||\n"
            "             elems[i].tagName == \"SELECT\" ||\n"
            "             elems[i].tagName == \"LABEL\"  ||\n"
            "             (elems[i].getAttribute &&\n"
            "              elems[i].getAttribute(\"role\") &&\n"
            "              (elems[i].getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
            "               elems[i].getAttribute(\"role\").toLowerCase() == \"tab\")) ||\n"
            "             (elems[i].tagName == \"INPUT\" &&\n"
            "              elems[i].type &&\n"
            "              (elems[i].type.toLowerCase() == \"checkbox\" ||\n"
            "               elems[i].type.toLowerCase() == \"radio\" ||\n"
            "               elems[i].type.toLowerCase() == \"file\" ||\n"
            "               elems[i].type.toLowerCase() == \"submit\" ||\n"
            "               elems[i].type.toLowerCase() == \"reset\" ||\n"
            "               elems[i].type.toLowerCase() == \"button\"))) ? \"Click\" :\n"
            "            (elems[i].onmouseover) ? \"Hover\" :\n"
            "            \"None\";\n"
            "        if(elems[i].tagName == \"FRAME\" || elems[i].tagName == \"IFRAME\"){\n"
            "            try{\n"
            "                var frameDocument = elems[i].contentDocument;\n"
            "                elems = elems.concat(Array.from(frameDocument.querySelectorAll(\"%1\")));\n"
            "            }\n"
            "            catch(e){}\n"
            "        }\n"
            "        var xpath = \"\";\n"
            "        var iter = elems[i];\n"
            "        while(iter && iter.nodeType == 1){\n"
            "            var str = iter.tagName;\n"
            "            var siblings = iter.parentNode.childNodes;\n"
            "            var synonym = [];\n"
            "            for(var j = 0; j < siblings.length; j++){\n"
            "                if(siblings[j].nodeType == 1 &&\n"
            "                   siblings[j].tagName == iter.tagName){\n"
            "                    synonym.push(siblings[j]);\n"
            "                }\n"
            "            }\n"
            "            if(synonym.length > 1 && synonym.indexOf(iter) != -1){\n"
            "                str += \"[\" + (synonym.indexOf(iter) + 1) + \"]\";\n"
            "            }\n"
            "            if(xpath && !xpath.startsWith(\",\")){\n"
            "                xpath = str + \"/\" + xpath;\n"
            "            } else {\n"
            "                xpath = str + xpath;\n"
            "            }\n"
            "            iter = iter.parentNode;\n"
            "            if(iter && iter.nodeType == 9 && iter !== document){\n"
            "                xpath = \",//\" + xpath.toLowerCase();\n"
            "                iter = iter.defaultView.frameElement;\n"
            "            }\n"
            "        }\n"
            "        data.xPath = \"//\" + xpath.toLowerCase();\n"
            "        map[i] = data;\n"
            "    }\n"
            "    return map;\n"
            "})();").arg(quoted, ignoreOutOfView, fix);
    }

    static inline QString HitElementJsCode(QPoint pos){
#if defined(Q_OS_WIN)
        QString fix = QStringLiteral("devicePixelRatio");
#elif defined(Q_OS_MAC)
        QString fix = QStringLiteral("(document.body.clientWidth / innerWidth)");
#else
        QString fix = QStringLiteral("1");
#endif
        return QStringLiteral(
            "(function(){\n"
            "    var scrollX = document.documentElement.scrollLeft || document.body.scrollLeft;\n"
            "    var scrollY = document.documentElement.scrollTop || document.body.scrollTop;\n"
            "    var baseUrl = \"\";\n"
            "    var baseDocument = \"index.html\";\n"
            "    var base = document.getElementsByTagName(\"base\");\n"
            "    if(base.length > 0 && base[0].href){\n"
            "        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
            "    } else {\n"
            "        baseUrl = \n"
            "            location.protocol + \"//\" + location.hostname + \n"
            "            (location.port && \":\" + location.port) + \"/\";\n"
            "    }\n"
            "    var x = %1;\n"
            "    var y = %2;\n"
            "    var elem = document.elementFromPoint(x, y);\n"
            "    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
            "        try{\n"
            "            var frameDocument = elem.contentDocument;\n"
            "            var rect = elem.getBoundingClientRect();\n"
            "            x -= rect.left;\n"
            "            y -= rect.top;\n"
            "            elem = frameDocument.elementFromPoint(x, y);\n"
            "        }\n"
            "        catch(e){ break;}\n"
            "    }\n"
            "    if(!elem) return;\n"
            "    var data = {};\n"
            "    data.tagName = elem.tagName;\n"
            "    data.innerText = elem.innerText;\n"
            "    var link = elem;\n"
            "    while(link){\n"
            "        if(link.href){\n"
            "            data.linkUrl = link.href;\n"
            "            data.linkHtml = link.outerHTML;\n"
            "            break;\n"
            "        }\n"
            "        link = link.parentNode;\n"
            "    }\n"
            "    var image = elem;\n"
            "    while(image){\n"
            "        if(image.src){\n"
            "            data.imageUrl = image.src;\n"
            "            data.imageHtml = image.outerHTML;\n"
            "            break;\n"
            "        }\n"
            "        image = image.parentNode;\n"
            "    }\n"
            "    data.baseUrl = baseUrl;\n"
            "    var offsetX = 0;\n"
            "    var offsetY = 0;\n"
            "    win = elem.ownerDocument.defaultView;\n"
            "    while(win && top !== win){\n"
            "        var rect = win.frameElement.getBoundingClientRect();\n"
            "        offsetX += rect.left;\n"
            "        offsetY += rect.top;\n"
            "        win = win.parent;\n"
            "    }\n"
            "    var rect = elem.getBoundingClientRect();\n"
            "    data.x = (rect.left + offsetX) * %3;\n"
            "    data.y = (rect.top  + offsetY) * %3;\n"
            "    data.width = rect.width   * %3;\n"
            "    data.height = rect.height * %3;\n"
            "    data.region = {};\n"
            "    var rects = elem.getClientRects();\n"
            "    for(var i = 0; i < rects.length; i++){\n"
            "        data.region[i] = {};\n"
            "        data.region[i].x = (rects[i].left + offsetX) * %3;\n"
            "        data.region[i].y = (rects[i].top  + offsetY) * %3;\n"
            "        data.region[i].width  = rects[i].width  * %3;\n"
            "        data.region[i].height = rects[i].height * %3;\n"
            "    }\n"
            "    data.isJsCommand = \n"
            "        (elem.onclick ||\n"
            "         (elem.href &&\n"
            "          elem.href.lastIndexOf &&\n"
            "          elem.href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
            "         (elem.getAttribute &&\n"
            "          elem.getAttribute(\"role\") &&\n"
            "          (elem.getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"tab\"))) ? true : false;\n"
            "    data.isTextInput = \n"
            "        (elem.tagName == \"TEXTAREA\" ||\n"
            "         (elem.tagName == \"INPUT\" &&\n"
            "          elem.type &&\n"
            "          (elem.type.toLowerCase() == \"text\" ||\n"
            "           elem.type.toLowerCase() == \"search\" ||\n"
            "           elem.type.toLowerCase() == \"password\"))) ? true : false;\n"
            "    data.isQueryInput = \n"
            "        (elem.tagName == \"INPUT\" &&\n"
            "         elem.type &&\n"
            "         (elem.type.toLowerCase() == \"text\" ||\n"
            "          elem.type.toLowerCase() == \"search\")) ? true : false;\n"
            "    data.isEditable = \n"
            "        (elem.isContentEditable ||\n"
            "         data.isTextInput ||\n"
            "         data.isQueryInput) ? true : false;\n"
            "    data.isFrame = \n"
            "        (elem.tagName == \"FRAME\" ||\n"
            "         elem.tagName == \"IFRAME\") ? true : false;\n"
            "    data.isLooped = elem.loop;\n"
            "    data.isPaused = elem.paused;\n"
            "    data.isMuted = elem.muted;\n"
            "    data.action = \n"
            "        (elem.href &&\n"
            "         elem.href.lastIndexOf &&\n"
            "         (elem.href.lastIndexOf(\"http:\", 0) === 0 ||\n"
            "          elem.href.lastIndexOf(\"https:\", 0) === 0)) ? \"None\" :\n"
            "        (elem.isContentEditable ||\n"
            "         elem.tagName == \"TEXTAREA\" ||\n"
            "         elem.tagName == \"OBJECT\"   ||\n"
            "         elem.tagName == \"EMBED\"    ||\n"
            "         elem.tagName == \"FRAME\"    ||\n"
            "         elem.tagName == \"IFRAME\"   ||\n"
            "         (elem.tagName == \"INPUT\" &&\n"
            "          elem.type &&\n"
            "          (elem.type.toLowerCase() == \"text\" ||\n"
            "           elem.type.toLowerCase() == \"search\" ||\n"
            "           elem.type.toLowerCase() == \"password\"))) ? \"Focus\" :\n"
            "        (elem.onclick ||\n"
            "         (elem.href &&\n"
            "          elem.href.lastIndexOf &&\n"
            "          elem.href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
            "         elem.tagName == \"BUTTON\" ||\n"
            "         elem.tagName == \"SELECT\" ||\n"
            "         elem.tagName == \"LABEL\"  ||\n"
            "         (elem.getAttribute &&\n"
            "          elem.getAttribute(\"role\") &&\n"
            "          (elem.getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
            "           elem.getAttribute(\"role\").toLowerCase() == \"tab\")) ||\n"
            "         (elem.tagName == \"INPUT\" &&\n"
            "          elem.type &&\n"
            "          (elem.type.toLowerCase() == \"checkbox\" ||\n"
            "           elem.type.toLowerCase() == \"radio\" ||\n"
            "           elem.type.toLowerCase() == \"file\" ||\n"
            "           elem.type.toLowerCase() == \"submit\" ||\n"
            "           elem.type.toLowerCase() == \"reset\" ||\n"
            "           elem.type.toLowerCase() == \"button\"))) ? \"Click\" :\n"
            "         (elem.onmouseover) ? \"Hover\" :\n"
            "         \"None\";\n"
            "    var xpath = \"\";\n"
            "    var iter = elem;\n"
            "    while(iter && iter.nodeType == 1){\n"
            "        var str = iter.tagName;\n"
            "        var siblings = iter.parentNode.childNodes;\n"
            "        var synonym = [];\n"
            "        for(var j = 0; j < siblings.length; j++){\n"
            "            if(siblings[j].nodeType == 1 &&\n"
            "               siblings[j].tagName == iter.tagName){\n"
            "                synonym.push(siblings[j]);\n"
            "            }\n"
            "        }\n"
            "        if(synonym.length > 1 && synonym.indexOf(iter) != -1){\n"
            "            str += \"[\" + (synonym.indexOf(iter) + 1) + \"]\";\n"
            "        }\n"
            "        if(xpath && !xpath.startsWith(\",\")){\n"
            "            xpath = str + \"/\" + xpath;\n"
            "        } else {\n"
            "            xpath = str + xpath;\n"
            "        }\n"
            "        iter = iter.parentNode;\n"
            "        if(iter && iter.nodeType == 9 && iter !== document){\n"
            "            xpath = \",//\" + xpath.toLowerCase();\n"
            "            iter = iter.defaultView.frameElement;\n"
            "        }\n"
            "    }\n"
            "    data.xPath = \"//\" + xpath.toLowerCase();\n"
            "    return data;\n"
            "})();").arg(pos.x()).arg(pos.y()).arg(fix);
    }

    static inline QString HitLinkUrlJsCode(QPoint pos){
        return QStringLiteral(
            "(function(){\n"
            "    var x = %1;\n"
            "    var y = %2;\n"
            "    var elem = document.elementFromPoint(x, y);\n"
            "    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
            "        try{\n"
            "            var frameDocument = elem.contentDocument;\n"
            "            var rect = elem.getBoundingClientRect();\n"
            "            x -= rect.left;\n"
            "            y -= rect.top;\n"
            "            elem = frameDocument.elementFromPoint(x, y);\n"
            "        }\n"
            "        catch(e){ break;}\n"
            "    }\n"
            "    while(elem){\n"
            "        if(elem.href) return elem.href;\n"
            "        elem = elem.parentNode;\n"
            "    }\n"
            "    return \"\";\n"
            "})();").arg(pos.x()).arg(pos.y());
    }

    static inline QString HitImageUrlJsCode(QPoint pos){
        return QStringLiteral(
            "(function(){\n"
            "    var x = %1;\n"
            "    var y = %2;\n"
            "    var elem = document.elementFromPoint(x, y);\n"
            "    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
            "        try{\n"
            "            var frameDocument = elem.contentDocument;\n"
            "            var rect = elem.getBoundingClientRect();\n"
            "            x -= rect.left;\n"
            "            y -= rect.top;\n"
            "            elem = frameDocument.elementFromPoint(x, y);\n"
            "        }\n"
            "        catch(e){ break;}\n"
            "    }\n"
            "    while(elem){\n"
            "        if(elem.src) return elem.src;\n"
            "        elem = elem.parentNode;\n"
            "    }\n"
            "    return \"\";\n"
            "})();").arg(pos.x()).arg(pos.y());
    }

    static inline QString SelectedTextJsCode(){
        return QStringLiteral("getSelection().toString();");
    }

    static inline QString SelectedHtmlJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    var div = document.createElement(\"div\");\n"
            "    var selection = getSelection();\n"
            "    if(!selection.rangeCount) return \"\";\n"
            "    div.appendChild(selection.getRangeAt(0).cloneContents());\n"
            "    return div.innerHTML;\n"
            "})();");
    }

    static inline QString WholeTextJsCode(){
        return QStringLiteral("document.documentElement.innerText;");
    }

    static inline QString WholeHtmlJsCode(){
        return QStringLiteral("document.documentElement.outerHTML;");
    }

    static inline QString SelectionRegionJsCode(){
#if defined(Q_OS_WIN)
        QString fix = QStringLiteral("devicePixelRatio");
#elif defined(Q_OS_MAC)
        QString fix = QStringLiteral("(document.body.clientWidth / innerWidth)");
#else
        QString fix = QStringLiteral("1");
#endif
        return QStringLiteral(
            "(function(){\n"
            "    var map = {};\n"
            "    var selection = getSelection();\n"
            "    if(!selection.rangeCount) return map;\n"
            "    var rects = selection.getRangeAt(0).getClientRects();\n"
            "    for(var i = 0; i < rects.length; i++){\n"
            "        map[i] = {};\n"
            "        map[i].x = rects[i].left * %1;\n"
            "        map[i].y = rects[i].top  * %1;\n"
            "        map[i].width  = rects[i].width  * %1;\n"
            "        map[i].height = rects[i].height * %1;\n"
            "    }\n"
            "    return map;\n"
            "})();").arg(fix);
    }

    static inline QString SetTextValueJsCode(const QString &xpath, const QString &text){
        QString quotedXpath = EscapeJsStringLiteral(xpath);
        QString quotedText = EscapeJsStringLiteral(text);
        return QStringLiteral(
            "(function(){\n"
            "    var elem;\n"
            "    var doc = document;\n"
            "    var xpaths = \"%1\".split(\",\");\n"
            "    for(var i = 0; i < xpaths.length; i++){\n"
            "        elem = doc.evaluate(xpaths[i], doc, null, 7, null).snapshotItem(0);\n"
            "        try{\n"
            "            if(elem.contentDocument){\n"
            "                doc = elem.contentDocument;\n"
            "            }\n"
            "        }\n"
            "        catch(e){ break;}\n"
            "    }\n"
            "    elem.setAttribute(\"value\", \"%2\");\n"
            "    elem.focus();\n"
            "})();").arg(quotedXpath, quotedText);
    }

    static inline QString ExecCommandJsCode(const QString &command){
        return QStringLiteral("document.execCommand(\"%1\");").arg(command);
    }

    static inline QString ChangeTextDirectionJsCode(const QString &direction){
        return QStringLiteral(
            "(function(){\n"
            "    var node = document.getSelection().anchorNode;\n"
            "    if(!node) return;\n"
            "    if(node.nodeType !== 1) node = node.parentElement;\n"
            "    while(node && !node.isContentEditable) node = node.parentElement;\n"
            "    if(!node) node = document.documentElement;\n"
            "    node.setAttribute(\"dir\", \"%1\");\n"
            "})();").arg(direction);
    }

    static inline QString MediaElementJsCode(const QString &statement){
        return QStringLiteral(
            "(function(){\n"
            "    var all = Array.prototype.slice.call(\n"
            "        document.querySelectorAll(\"video, audio\"));\n"
            "    if(!all.length) return;\n"
            "    function area(e){\n"
            "        var r = e.getBoundingClientRect();\n"
            "        return r.width * r.height;\n"
            "    }\n"
            "    var playing = all.filter(function(e){ return !e.paused;});\n"
            "    var from = playing.length ? playing : all;\n"
            "    var media = from.reduce(function(a, b){\n"
            "        return area(b) > area(a) ? b : a;\n"
            "    });\n"
            "    if(!media) return;\n"
            "    %1\n"
            "})();").arg(statement);
    }

    static inline QString ToggleMediaControlsJsCode(){
        return MediaElementJsCode(QStringLiteral("media.controls = !media.controls;"));
    }
    static inline QString ToggleMediaLoopJsCode(){
        return MediaElementJsCode(QStringLiteral("media.loop = !media.loop;"));
    }
    static inline QString ToggleMediaPlayPauseJsCode(){
        return MediaElementJsCode
            (QStringLiteral("if(media.paused) media.play(); else media.pause();"));
    }

    static inline QString InstallWebChannelJsCode(){
        QString script;
        QFile file(":/qtwebchannel/qwebchannel.js");
        if(file.open(QFile::ReadOnly)){
            script = QString::fromLatin1(file.readAll());
        }
        file.close();
        return script + QStringLiteral(
            "\n"
            "if(typeof qt === \"undefined\" && typeof top.qt !== \"undefined\"){\n"
            "    window.qt = top.qt;\n"
            "}\n"
            "new QWebChannel(qt.webChannelTransport, function(channel){\n"
            "    window._vanilla = channel.objects._vanilla;\n"
            "    window._view = channel.objects._view;\n"
            "});\n");
    }

    static inline QString InstallSubmitEventJsCode(){
        return QStringLiteral(
            "(function(){\n"
            "    var forms = document.querySelectorAll(\"form\");\n"
            "    var allInputs = Array.from(document.querySelectorAll(\"input,textarea\"));\n"
            "    for(var i = 0; i < forms.length; i++){\n"
            "        var form = forms[i];\n"
            "        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
            "        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
            "        if(!submit || !password){\n"
            "            var inputs = allInputs.filter(function(e){ return e.form == form;});\n"
            "            if(!submit) submit = inputs.find(function(e){ return e.type == \"submit\";});\n"
            "            if(!password) password = inputs.find(function(e){ return e.type == \"password\";});\n"
            "        };\n"
            "        if(!submit || !password) continue;\n"
            "        form.addEventListener(\"submit\", function(e){\n"
            "            var data = \"\";\n"
            "            var inputs = Array.from(e.target.querySelectorAll(\"input,textarea\"));\n"
            "            inputs = inputs.concat(allInputs.filter(function(el){ return el.form == e.target;}));\n"
            "            inputs = inputs.filter(function(v, i, s){ return s.indexOf(v) == i;});\n"
            "            for(var j = 0; j < inputs.length; j++){\n"
            "                var field = inputs[j];\n"
            "                var type = (field.type || \"hidden\").toLowerCase();\n"
            "                var name = field.name;\n"
            "                var val = field.value;\n"
            "                if(!name || type == \"hidden\" || type == \"submit\"){\n"
            "                    continue;\n"
            "                }\n"
            "                if(data) data = data + \"&\";\n"
            "                data = data + encodeURIComponent(name) + \"=\" + encodeURIComponent(val);\n"
            "            }\n"
            "            console.info(\"submit%1,\" + data);\n"
            "        }, false);\n"
            "    }\n"
            "})();").arg(Application::EventKey());
    }

    static inline QString DecorateFormFieldJsCode(const QString &data){
        QString quoted = EscapeJsStringLiteral(data);
        return QStringLiteral(
            "(function(){\n"
            "    var data = \"%1\".split(\"&\");\n"
            "    var forms = document.querySelectorAll(\"form\");\n"
            "    var allInputs = Array.from(document.querySelectorAll(\"input,textarea\"));\n"
            "    for(var i = 0; i < forms.length; i++){\n"
            "        var form = forms[i];\n"
            "        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
            "        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
            "        if(!submit || !password){\n"
            "            var inputs = allInputs.filter(function(e){ return e.form == form;});\n"
            "            if(!submit) submit = inputs.find(function(e){ return e.type == \"submit\";});\n"
            "            if(!password) password = inputs.find(function(e){ return e.type == \"password\";});\n"
            "        };\n"
            "        if(!submit || !password) continue;\n"
            "        for(var j = 0; j < data.length; j++){\n"
            "            var pair = data[j].split(\"=\");\n"
            "            var name = decodeURIComponent(pair[0]);\n"
            "            var val  = decodeURIComponent(pair[1]);\n"
            "            var field = form.querySelector(\"*[name=\\\"\" + name + \"\\\"]\");\n"
            "            if(!field) field = document.querySelector(\"*[name=\\\"\" + name + \"\\\"]\");\n"
            "            if(!field) continue;\n"
            "            field.style.boxShadow = \"inset 0 0 2px 2px #eca\";\n"
            "            field.style.border = \"1px\";\n"
            "        }\n"
            "    }\n"
            "})();").arg(quoted);
    }

    static inline QString SubmitFormDataJsCode(const QString &data){
        QString quoted = EscapeJsStringLiteral(data);
        return QStringLiteral(
            "(function(){\n"
            "    var data = \"%1\".split(\"&\");\n"
            "    var forms = document.querySelectorAll(\"form\");\n"
            "    var allInputs = Array.from(document.querySelectorAll(\"input,textarea\"));\n"
            "    for(var i = 0; i < forms.length; i++){\n"
            "        var inserted = false;\n"
            "        var form = forms[i];\n"
            "        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
            "        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
            "        if(!submit || !password){\n"
            "            var inputs = allInputs.filter(function(e){ return e.form == form;});\n"
            "            if(!submit) submit = inputs.find(function(e){ return e.type == \"submit\";});\n"
            "            if(!password) password = inputs.find(function(e){ return e.type == \"password\";});\n"
            "        };\n"
            "        if(!submit || !password) continue;\n"
            "        for(var j = 0; j < data.length; j++){\n"
            "            var pair = data[j].split(\"=\");\n"
            "            var name = decodeURIComponent(pair[0]);\n"
            "            var val  = decodeURIComponent(pair[1]);\n"
            "            var field = form.querySelector(\"*[name=\\\"\" + name + \"\\\"]\");\n"
            "            if(!field) field = document.querySelector(\"*[name=\\\"\" + name + \"\\\"]\");\n"
            "            if(!field) continue;\n"
            "            inserted = true;\n"
            "            field.value = val;\n"
            "        }\n"
            "        if(!inserted) continue;\n"
            "        if(submit.click){\n"
            "            submit.click();\n"
            "        } else if(typeof submit == \"function\"){\n"
            "            form.submit();\n"
            "        }\n"
            "    }\n"
            "})();").arg(quoted);
    }

    static inline QString JsKeyCodeSet(const QList<int> &codes){
        QStringList entries;
        foreach(int code, codes)
            entries << QStringLiteral("%1:1").arg(code);
        return QStringLiteral("{%1}").arg(entries.join(QStringLiteral(",")));
    }

    static inline QString EdgeInputBridgeJsCode(const QList<int> &plain,
                                                const QList<int> &shifted){
        const QString plainSet = JsKeyCodeSet(plain);
        const QString shiftedSet = JsKeyCodeSet(shifted);

        return QStringLiteral(
                "(function(){\n"
                "    if(!window.chrome || !window.chrome.webview) return;\n"
                "    var d = document;\n"
                "    if(window === window.top){\n"
                "    d.addEventListener(\"keydown\", function(e){\n"
                "        // the IME owns the keystroke while it is composing.\n"
                "        if(e.isComposing || e.keyCode == 229) return;\n"
                "        // a held key acts once.\n"
                "        if(e.repeat) return;\n"
                "        // these are the accelerator's. reporting them here is the double\n"
                "        // arrival this file exists to avoid.\n"
                "        if(e.ctrlKey || e.altKey || e.metaKey) return;\n"
                "        var c = e.keyCode;\n"
                "        var wanted = e.shiftKey ? %2 : %3;\n"
                "        if(!wanted[c]) return;\n"
                "        // where the caret is decides what the key means.\n"
                "        var el = d.activeElement;\n"
                "        if(el && (el.isContentEditable ||\n"
                "                  el.tagName == \"INPUT\" ||\n"
                "                  el.tagName == \"TEXTAREA\" ||\n"
                "                  el.tagName == \"SELECT\" ||\n"
                "                  el.tagName == \"BUTTON\" ||\n"
                "                  el.tagName == \"FRAME\" ||\n"
                "                  el.tagName == \"IFRAME\")) return;\n"
                "        window.chrome.webview.postMessage({\n"
                "            v: 1, tag: %1, kind: \"key\",\n"
                "            code: c, shift: !!e.shiftKey});\n"
                "        e.preventDefault();\n"
                "    }, false);\n"
                "    }\n"
                "    window.print = function(){\n"
                "        window.chrome.webview.postMessage({\n"
                "            v: 1, tag: %1, kind: \"print\"});\n"
                "    };\n"
                "})();\n"
            ).arg(Application::EventKey()).arg(shiftedSet, plainSet);
    }

    static inline QString EdgeScrollReportJsCode(){
        return QStringLiteral(
                "(function(){\n"
                "    if(window !== window.top) return;\n"
                "    if(!window.chrome || !window.chrome.webview) return;\n"
                "    var last = \"\";\n"
                "    function report(){\n"
                "        var de = document.documentElement;\n"
                "        if(!de) return;\n"
                "        var b = document.body;\n"
                "        var m = { v: 1, tag: %1, kind: \"scroll\",\n"
                "                  x: Math.max(0, window.pageXOffset),\n"
                "                  y: Math.max(0, window.pageYOffset),\n"
                "                  w: Math.max(de.scrollWidth, b ? b.scrollWidth : 0),\n"
                "                  h: Math.max(de.scrollHeight, b ? b.scrollHeight : 0),\n"
                "                  vw: window.innerWidth, vh: window.innerHeight };\n"
                "        var key = location.href+\"|\"+m.x+\",\"+m.y+\",\"+m.w+\",\"+m.h+\",\"+m.vw+\",\"+m.vh;\n"
                "        if(key === last) return;\n"
                "        last = key;\n"
                "        window.chrome.webview.postMessage(m);\n"
                "    }\n"
                "    var pending = false;\n"
                "    function schedule(){\n"
                "        if(pending) return;\n"
                "        pending = true;\n"
                "        requestAnimationFrame(function(){ pending = false; report();});\n"
                "    }\n"
                "    window.addEventListener(\"scroll\", schedule, {passive: true, capture: true});\n"
                "    window.addEventListener(\"resize\", schedule);\n"
                "    window.addEventListener(\"load\", schedule);\n"
                "    document.addEventListener(\"DOMContentLoaded\", schedule);\n"
                "    setInterval(schedule, 500);\n"
                "})();\n"
            ).arg(Application::EventKey());
    }

    static inline QString InstallEventFilterJsCode(const QList<QEvent::Type> &types){
        QString inner;
        if(types.contains(QEvent::KeyPress))
            inner += QStringLiteral(
                "\n"
                "doc.addEventListener(\"keydown\", function(e){\n"
                "    var prevent = false;\n"
                "    var elem = e.target.ownerDocument.activeElement;\n"
                "    if(e.keyCode == 9 || e.keyCode == 13){\n"
                "        prevent = false;\n"
                "        console.info(\"keyPressEvent%1,\" + \n"
                "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
                "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "    } else if(!e.altKey && !e.ctrlKey && !e.metaKey &&\n"
                "       32 <= e.keyCode && e.keyCode <= 40){\n"
                "        prevent = false;\n"
                "        console.info(\"preventScrollRestoration%1\");\n"
                "    } else if(elem.isContentEditable ||\n"
                "              elem.tagName == \"BUTTON\" ||\n"
                "              elem.tagName == \"SELECT\" ||\n"
                "              elem.tagName == \"INPUT\"  ||\n"
                "              elem.tagName == \"TEXTAREA\" ||\n"
                "              elem.tagName == \"FRAME\" ||\n"
                "              elem.tagName == \"IFRAME\"){\n"
                "        if(e.ctrlKey || e.altKey || e.metaKey){\n"
                "            prevent = true;\n"
                "        }\n"
                "    } else {\n"
                "        prevent = true;\n"
                "    }\n"
                "    if(prevent){\n"
                "        console.info(\"keyPressEvent%1,\" + \n"
                "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
                "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "        e.preventDefault();\n"
                "    }\n"
                "}, false);\n");

        if(types.contains(QEvent::KeyRelease))
            inner += QStringLiteral(
                "\n"
                "doc.addEventListener(\"keyup\", function(e){\n"
                "    var prevent = false;\n"
                "    var elem = e.target.ownerDocument.activeElement;\n"
                "    if(e.keyCode == 9 || e.keyCode == 13){\n"
                "        prevent = false;\n"
                "        console.info(\"keyReleaseEvent%1,\" + \n"
                "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
                "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "    } else if(!e.altKey && !e.ctrlKey && !e.metaKey &&\n"
                "       32 <= e.keyCode && e.keyCode <= 40){\n"
                "        prevent = false;\n"
                "        console.info(\"keyReleaseEvent%1,\" + \n"
                "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
                "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "    } else if(elem.isContentEditable ||\n"
                "              elem.tagName == \"BUTTON\" ||\n"
                "              elem.tagName == \"SELECT\" ||\n"
                "              elem.tagName == \"INPUT\"  ||\n"
                "              elem.tagName == \"TEXTAREA\" ||\n"
                "              elem.tagName == \"FRAME\" ||\n"
                "              elem.tagName == \"IFRAME\"){\n"
                "        if(e.ctrlKey || e.altKey || e.metaKey){\n"
                "            prevent = true;\n"
                "        }\n"
                "    } else {\n"
                "        prevent = true;\n"
                "    }\n"
                "    if(prevent){\n"
                "        console.info(\"keyReleaseEvent%1,\" + \n"
                "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
                "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "        e.preventDefault();\n"
                "    }\n"
                "}, false);\n");

        if(types.contains(QEvent::MouseMove))
            inner += QStringLiteral(
                "\n"
                "doc.addEventListener(\"mousemove\", function(e){\n"
                "    console.info(\"mouseMoveEvent%1,\" + \n"
                "                 e.button.toString() + \",\" + \n"
                "                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
                "                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
                "                 e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "}, false);\n");

        if(types.contains(QEvent::MouseButtonPress))
            inner += QStringLiteral(
                "\n"
                "doc.addEventListener(\"mousedown\", function(e){\n"
                "    console.info(\"mousePressEvent%1,\" + \n"
                "                 e.button.toString() + \",\" + \n"
                "                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
                "                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
                "                 e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "}, false);\n");

        if(types.contains(QEvent::MouseButtonRelease))
            inner += QStringLiteral(
                "\n"
                "doc.addEventListener(\"mouseup\", function(e){\n"
                "    console.info(\"mouseReleaseEvent%1,\" + \n"
                "                 e.button.toString() + \",\" + \n"
                "                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
                "                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
                "                 e.altKey.toString() + \",\" + e.metaKey.toString());\n"
                "}, false);\n");

        if(types.contains(QEvent::Wheel))
            inner += QStringLiteral(
                "\n"
                "doc.addEventListener(\"mousewheel\", function(e){\n"
                "    console.info(\"wheelEvent%1,\" + e.wheelDelta.toString());\n"
                "}, false);\n");

        return QStringLiteral(
                "(function(){\n"
                "    var wins = [window];\n"
                "    wins = wins.concat(Array.from(frames));\n"
                "    for(var i = 0; i < wins.length; i++){\n"
                "        try{\n"
                "            var doc = wins[i].document;\n"
                "            %1\n"
                "        }\n"
                "        catch(e){}\n"
                "    }\n"
                "})();").arg(inner.arg(Application::EventKey()));
    }

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
    static QKeyEvent *m_UpKey;
    static QKeyEvent *m_DownKey;
    static QKeyEvent *m_RightKey;
    static QKeyEvent *m_LeftKey;
    static QKeyEvent *m_PageUpKey;
    static QKeyEvent *m_PageDownKey;
    static QKeyEvent *m_HomeKey;
    static QKeyEvent *m_EndKey;

    static SharedWebElement m_ClickedElement;
    static QRegion m_SelectionRegion;
    static QUrl m_CurrentBaseUrl;
    static QString m_SelectedText;
    static QString m_SelectedHtml;
    static bool m_DragStarted;
    static bool m_HadSelection;
    static bool m_Switching;
    static bool m_RightButtonConsumed;
    static QElapsedTimer m_OwnDropTimer;
    static QPoint m_GestureStartedPos;
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
    static bool m_NavigationBySpaceKey;
    static bool m_DragToStartDownload;
    static bool m_EnableDestinationInferrer;
    static bool m_EnableDragGesture;
    static bool m_EnableMouseGesture;
    static bool m_EnableScrollGesture;
    static bool m_InspectorInMainWindow;
    static QString m_SuspendHiddenViews;
    bool m_EnableDragGestureLocal;

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
    _View    *m_JsObject;
    int m_LoadProgress;
    bool m_IsLoading;
    bool m_DisplayObscured;
    RenderProcessLedger m_RenderProcessLedger;
    QStringList m_SpecificSettings;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(View::FindFlags);

#endif
