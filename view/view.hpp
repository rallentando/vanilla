#ifndef VIEW_HPP
#define VIEW_HPP

#include "switch.hpp"

#include <QTimer>
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

class View{

public:
    View(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    virtual ~View();

    enum GestureVector{
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

    enum ScrollBarState{
        NoScrollBarEnabled,
        HorizontalScrollBarEnabled,
        VerticalScrollBarEnabled,
        BothScrollBarEnabled,
    };

    virtual QObject *base();
    virtual QObject *page();

    void Initialize();
    void DeleteLater();

    TreeBank  *GetTreeBank() const;
    ViewNode  *GetViewNode() const;
    HistNode  *GetHistNode() const;
    WeakView   GetThis() const;
    WeakView   GetMaster() const;
    WeakView   GetSlave() const;
    _View     *GetJsObject() const;

    void SetTreeBank(TreeBank*);
    void SetViewNode(ViewNode*);
    void SetHistNode(HistNode*);
    void SetThis(WeakView);
    void SetMaster(WeakView);
    void SetSlave(WeakView);

    void GoBackTo(QUrl);
    void GoForwardTo(QUrl);
    void GoBackToInferedUrl();
    void GoForwardToInferedUrl();

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

    void AddContextMenu(QMenu *menu, SharedWebElement elem, Page::MediaType type = Page::MediaTypeNone);
    void AddRegularMenu(QMenu *menu, SharedWebElement elem);

    bool GetDisplayObscured(){ return m_DisplayObscured;}
    void SetDisplayObscured(bool obscured){ m_DisplayObscured = obscured;}

    static void LoadSettings();
    static void SaveSettings();
    virtual void ApplySpecificSettings(QStringList set);

    static bool ActivateNewViewDefault(){ return m_ActivateNewViewDefault;}
    static bool NavigationBySpaceKey(){ return m_NavigationBySpaceKey;}
    static bool DragToStartDownload(){ return m_DragToStartDownload;}
    static bool EnableDestinationInferrer(){ return m_EnableDestinationInferrer;}
    static bool EnableLoadHack(){ return m_EnableLoadHack;}
    static bool EnableDragHack(){ return m_EnableDragHack;}
    bool EnableLoadHackLocal() const { return m_EnableLoadHackLocal;}
    bool EnableDragHackLocal() const { return m_EnableDragHackLocal;}

    static QString GetLinkMenu(){ return m_LinkMenu;}
    static QString GetImageMenu(){ return m_ImageMenu;}
    static QString GetMediaMenu(){ return m_MediaMenu;}
    static QString GetTextMenu(){ return m_TextMenu;}
    static QString GetSelectionMenu(){ return m_SelectionMenu;}
    static QString GetRegularMenu(){ return m_RegularMenu;}

    static const QList<float> &GetZoomFactorLevels(){ return m_ZoomFactorLevels;}

    static bool GetSwitchingState(){ return m_Switching;}
    static void SetSwitchingState(bool switching){ m_Switching = switching;}

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

    // page's function.
    virtual int LoadProgress(){ return m_LoadProgress;}
    virtual bool IsLoading(){ return m_IsLoading;}
    virtual bool CanGoBack(){ return false;}
    virtual bool CanGoForward(){ return false;}
    virtual bool RecentlyAudible(){ return false;}
    virtual bool IsAudioMuted(){ return false;}
    virtual void SetAudioMuted(bool){}

    virtual void Copy(){}
    virtual void Cut(){}
    virtual void Paste(){}
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

    virtual void ExitFullScreen(){}
    virtual void InspectElement(){}
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

    virtual QUrl GetBaseUrl(){
        return WaitForResult<QUrl>([&](UrlCallBack callBack){
            CallWithGotBaseUrl(callBack);});
    }
    virtual void CallWithGotBaseUrl(UrlCallBack callBack){
        callBack(GetBaseUrl());
    }
    virtual QUrl GetCurrentBaseUrl(){
        return WaitForResult<QUrl>([&](UrlCallBack callBack){
            CallWithGotCurrentBaseUrl(callBack);});
    }
    virtual void CallWithGotCurrentBaseUrl(UrlCallBack callBack){
        callBack(GetCurrentBaseUrl());
    }

    virtual SharedWebElementList FindElements(Page::FindElementsOption option){
        return WaitForResult<SharedWebElementList>([&](WebElementListCallBack callBack){
            CallWithFoundElements(option, callBack);});
    }
    virtual void CallWithFoundElements(Page::FindElementsOption option, WebElementListCallBack callBack){
        callBack(FindElements(option));
    }
    virtual SharedWebElement HitElement(const QPoint &pos){
        return WaitForResult<SharedWebElement>([&](WebElementCallBack callBack){
            CallWithHitElement(pos, callBack);});
    }
    virtual void CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
        callBack(HitElement(pos));
    }

    virtual QUrl HitLinkUrl(const QPoint &pos){
        return WaitForResult<QUrl>([&](UrlCallBack callBack){
            CallWithHitLinkUrl(pos, callBack);});
    }
    virtual void CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
        callBack(HitLinkUrl(pos));
    }
    virtual QUrl HitImageUrl(const QPoint &pos){
        return WaitForResult<QUrl>([&](UrlCallBack callBack){
            CallWithHitImageUrl(pos, callBack);});
    }
    virtual void CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
        callBack(HitImageUrl(pos));
    }

    virtual QString SelectedText(){
        return WaitForResult<QString>([&](StringCallBack callBack){
            CallWithSelectedText(callBack);});
    }
    virtual void CallWithSelectedText(StringCallBack callBack){
        callBack(SelectedText());
    }
    virtual QString SelectedHtml(){
        return WaitForResult<QString>([&](StringCallBack callBack){
            CallWithSelectedHtml(callBack);});
    }
    virtual void CallWithSelectedHtml(StringCallBack callBack){
        callBack(SelectedHtml());
    }

    virtual QString WholeText(){
        return WaitForResult<QString>([&](StringCallBack callBack){
            CallWithWholeText(callBack);});
    }
    virtual void CallWithWholeText(StringCallBack callBack){
        callBack(WholeText());
    }
    virtual QString WholeHtml(){
        return WaitForResult<QString>([&](StringCallBack callBack){
            CallWithWholeHtml(callBack);});
    }
    virtual void CallWithWholeHtml(StringCallBack callBack){
        callBack(WholeHtml());
    }

    virtual QRegion SelectionRegion(){
        return WaitForResult<QRegion>([&](RegionCallBack callBack){
            CallWithSelectionRegion(callBack);});
    }
    virtual void CallWithSelectionRegion(RegionCallBack callBack){
        callBack(SelectionRegion());
    }

    virtual QVariant EvaluateJavaScript(const QString &source){
        return WaitForResult<QVariant>([&](VariantCallBack callBack){
            CallWithEvaluatedJavaScriptResult(source, callBack);});
    }
    virtual void CallWithEvaluatedJavaScriptResult(const QString &source, VariantCallBack callBack){
        callBack(EvaluateJavaScript(source));
    }

    // for slots.
    virtual QSize size() = 0;
    virtual void resize(QSize) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void raise() = 0;
    virtual void lower() = 0;
    virtual void repaint() = 0;
    virtual bool visible() = 0;
    virtual void setFocus(Qt::FocusReason = Qt::OtherFocusReason) = 0;

    virtual void Load();
    virtual void Load(const QString &url);
    virtual void Load(const QUrl &url);
    virtual void Load(const QNetworkRequest &req);

    virtual void OnFocusIn();
    virtual void OnFocusOut();

    virtual void OnBeforeStartingDisplayGadgets(){}
    virtual void OnAfterFinishingDisplayGadgets(){}

    virtual void OnSetViewNode(ViewNode*){}
    virtual void OnSetHistNode(HistNode*){}
    virtual void OnSetThis(WeakView){}
    virtual void OnSetMaster(WeakView){}
    virtual void OnSetSlave(WeakView){}
    virtual void OnSetJsObject(_View*){}
    virtual void OnSetJsObject(_Vanilla*){}
    virtual void OnLoadStarted(){ m_LoadProgress = 0; m_IsLoading = true;}
    virtual void OnLoadProgress(int progress){ m_LoadProgress = progress;}
    virtual void OnLoadFinished(bool){ m_LoadProgress = 100; m_IsLoading = false;}
    virtual void OnTitleChanged(const QString&){}
    virtual void OnUrlChanged(const QUrl&){}
    virtual void OnViewChanged(){}
    virtual void OnScrollChanged(){}

    virtual void EmitScrollChanged(){}

    // using scroll ratio position.
    virtual void SetScrollBarState(){}
    virtual QPointF GetScroll(){ return QPointF();}
    virtual void SetScroll(QPointF){}

    virtual bool SaveScroll(){ return false;}
    virtual bool RestoreScroll(){ return false;}
    virtual bool SaveZoom(){ return false;}
    virtual bool RestoreZoom(){ return false;}
    virtual bool SaveHistory(){ return false;}
    virtual bool RestoreHistory(){ return false;}

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
        /* U */if(dy >  dx && dy > -dx) return Gv_Up;
        /* D */if(dy <  dx && dy < -dx) return Gv_Down;
        /* R */if(dy >  dx && dy < -dx) return Gv_Right;
        /* L */if(dy <  dx && dy > -dx) return Gv_Left;
        return Gv_NoMove;
    }

    static inline GestureVector GetGestureVector8(int dx, int dy){
        // sin(pi/8) = sqrt(2.0-sqrt(2.0))/2.0
        //           = 2.5*sqrt(2.0-sqrt(2.0))/5.0
        //
        // 2.5*sqrt(2.0-sqrt(2.0)) ~ 1.91341716183
        //                         ~ 2.0
        // ~ : nealy equal

        /*  U */if(2*dy >  5*dx && 2*dy > -5*dx) return Gv_Up;
        /*  D */if(2*dy <  5*dx && 2*dy < -5*dx) return Gv_Down;
        /*  R */if(5*dy >  2*dx && 5*dy < -2*dx) return Gv_Right;
        /*  L */if(5*dy <  2*dx && 5*dy > -2*dx) return Gv_Left;
        /* UR */if(5*dy > -2*dx && 2*dy < -5*dx) return Gv_UpperRight;
        /* UL */if(2*dy <  5*dx && 5*dy >  2*dx) return Gv_UpperLeft;
        /* DR */if(2*dy >  5*dx && 5*dy <  2*dx) return Gv_LowerRight;
        /* DL */if(5*dy < -2*dx && 2*dy > -5*dx) return Gv_LowerLeft;
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
        QString quoted = QString(xpath).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
            "    var elem;\n"
            "    var doc = document;\n"
            "    var xpaths = \"%1\".split(\",\");\n"
            "    for(var i = 0; i < xpaths.length; i++){\n"
            //       7 : XPathResult.ORDERED_NODE_SNAPSHOT_TYPE
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
        QString quoted = QString(xpath).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
            "    var elem;\n"
            "    var doc = document;\n"
            "    var xpaths = \"%1\".split(\",\");\n"
            "    for(var i = 0; i < xpaths.length; i++){\n"
            //       7 : XPathResult.ORDERED_NODE_SNAPSHOT_TYPE
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
            "})();").arg(quoted).arg(pos.x()).arg(pos.y());
    }

    // return value is js array. and it'll be callbacked.
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

    // return value is js array. and it'll be callbacked.
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

    // return value is js array. and it'll be callbacked.
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
        QString quoted = Page::OptionToSelector(option).replace(QStringLiteral("\""), QStringLiteral("\\\""));
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
            // scroll bar value is unused.
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
            "        data.linkHtml = elems[i].outerHTML;\n" // roughly capture.
            "        data.imageUrl = elems[i].src;\n"
            "        data.imageHtml = elems[i].innerHTML;\n" // roughly capture.
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
          //"            var r1 = elems[i].ownerDocument.documentElement.getBoundingClientRect();\n"
            "            var w = elems[i].ownerDocument.defaultView;\n"
            "            var r1 = { left: 0, top: 0, right: w.innerWidth, bottom: w.innerHeight};\n"
            "            var r2 = elems[i].getBoundingClientRect();\n"
            "            if(%2 &&\n" // ignore out of view.
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
            //       1 : document.ELEMENT_NODE
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
            //           9 : document.DOCUMENT_NODE
            "            if(iter && iter.nodeType == 9 && iter !== document){\n"
            "                xpath = \",//\" + xpath.toLowerCase();\n"
            "                iter = iter.defaultView.frameElement;\n"
            "            }\n"
            "        }\n"
            "        data.xPath = \"//\" + xpath.toLowerCase();\n"
            "        map[i] = data;\n"
            "    }\n"
            "    return map;\n"
            "})();").arg(quoted).arg(ignoreOutOfView).arg(fix);
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
            // scroll bar value is unused.
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
            //   1 : document.ELEMENT_NODE
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
            //       9 : document.DOCUMENT_NODE
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
        QString quotedXpath = QString(xpath).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        QString quotedText = QString(text).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
            "    var elem;\n"
            "    var doc = document;\n"
            "    var xpaths = \"%1\".split(\",\");\n"
            "    for(var i = 0; i < xpaths.length; i++){\n"
            //       7 : XPathResult.ORDERED_NODE_SNAPSHOT_TYPE
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
            "})();").arg(quotedXpath).arg(quotedText);
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
        QString quoted = QString(data).replace(QStringLiteral("\""), QStringLiteral("\\\""));
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
        QString quoted = QString(data).replace(QStringLiteral("\""), QStringLiteral("\\\""));
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
              /*"        console.info(\"keyPressEvent%1,\" + \n"
                "                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
                "                     e.ctrlKey.toString() + \",\" + e.altKey.toString() + \",\" + e.metaKey.toString());\n"*/
                //       on press key for scroll, only prevent scroll restoration.
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
    // default :: Void -> Result
    // callWith :: (Result -> Void) -> Void

    // callBack :: Result -> Void
    // callBack2 :: (Result -> Void) -> Void

    // WaitForResult :: ((Result -> Void) -> Void) -> Result

    // when calling method using 'WaitForResult' from '~Event',
    // 'callBack' is not called in that event and to crash,
    // because of 'callBack' is called after that event finished.
    // this issue is fixed at least Qt5.7 or newer.
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
    static QPoint m_GestureStartedPos;
    static QPoint m_BeforeGesturePos;
    static Gesture m_Gesture;
    static GestureVector m_CurrentGestureVector;
    static GestureVector m_BeforeGestureVector;
    static int m_SameGestureVectorCount;
    static int m_GestureMode; // count of vector(4 or 8).
    static ScrollBarState m_ScrollBarState;

    static QMap<QKeySequence, QString> m_KeyMap;
    static QMap<QString, QString> m_MouseMap;
    static QMap<QString, QString> m_LeftGestureMap;
    static QMap<QString, QString> m_RightGestureMap;

    static bool m_ActivateNewViewDefault;
    static bool m_NavigationBySpaceKey;
    static bool m_DragToStartDownload;
    static bool m_EnableDestinationInferrer;
    static bool m_EnableLoadHack;
    static bool m_EnableDragHack;
    // for view instance specific settings.
    bool m_EnableLoadHackLocal;
    bool m_EnableDragHackLocal;

    static const QList<float> m_ZoomFactorLevels;

    static QString m_LinkMenu;
    static QString m_ImageMenu;
    static QString m_MediaMenu;
    static QString m_TextMenu;
    static QString m_SelectionMenu;
    static QString m_RegularMenu;

    TreeBank *m_TreeBank;
    ViewNode *m_ViewNode;
    HistNode *m_HistNode;
    WeakView  m_This;
    WeakView  m_Master;
    WeakView  m_Slave;
    QObject  *m_Page;
    _View    *m_JsObject;
    int m_LoadProgress;
    bool m_IsLoading;
    bool m_DisplayObscured;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(View::FindFlags);

#endif //ifndef VIEW_HPP
