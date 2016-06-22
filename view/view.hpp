#ifndef VIEW_HPP
#define VIEW_HPP

#include "switch.hpp"

#include <QTimer>
#include <QMenu>
#include <QContextMenuEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QNetworkAccessManager>

#include <memory>

#include "callback.hpp"
#include "application.hpp"
#include "page.hpp"

class QKeySequence;
class QMimeData;

class View;
class _View;
class _Vanilla;

typedef std::shared_ptr<View> SharedView;
typedef std::weak_ptr<View>   WeakView;
typedef QList<std::shared_ptr<View>> SharedViewList;
typedef QList<std::weak_ptr<View>>   WeakViewList;

/*

  contenteditable=true: focus
  textarea: focus
  object: focus
  embed: focus
  frame: focus
  iframe: focus
  input type=text: focsu
  input type=search: focus
  input type=password: focus

  onclick: click
  href=javascript: click
  button: click
  select: click
  label: click
  role=button: click
  role=link: click
  role=menu: click
  role=checkbox: click
  role=radio: click
  role=tab: click
  input type=checkbox: clcik
  input type=radio: click
  input type=file: click
  input type=submit: click
  input type=reset: click
  input type=button: clcik

  href=*: menu

 */

class WebElement{
public:
    explicit WebElement(){
    }
    virtual ~WebElement(){
    }
    enum Action {
        Click,
        Focus,
        Hover,
        None,
    };
    virtual bool SetFocus(){ return false;}
    virtual bool ClickEvent(){ return false;}
    virtual QString TagName() const { return QString();}
    virtual QString InnerText() const { return QString();}
    virtual QUrl BaseUrl() const { return QUrl();}
    virtual QUrl LinkUrl() const { return QUrl();}
    virtual QUrl ImageUrl() const { return QUrl();}
    virtual QString LinkHtml() const { return QString();}
    virtual QString ImageHtml() const { return QString();}
    virtual QPoint Position() const { return QPoint();}
    virtual QRect Rectangle() const { return QRect();}
    virtual QRegion Region() const { return QRegion();}
    virtual void SetPosition(QPoint){}
    virtual void SetRectangle(QRect){}
    virtual void SetText(QString){}
    virtual QPixmap Pixmap(){ return QPixmap();}
    virtual bool IsNull() const { return true;}
    virtual bool IsEditableElement() const { return false;}
    virtual bool IsJsCommandElement() const { return false;}
    virtual bool IsTextInputElement() const { return false;}
    virtual bool IsQueryInputElement() const { return false;}
    virtual bool IsFrameElement() const { return false;}
    virtual Action GetAction() const { return None;}
    virtual bool Equals(const WebElement&) const { return false;}
};

typedef std::shared_ptr<WebElement> SharedWebElement;
typedef QList<std::shared_ptr<WebElement>> SharedWebElementList;
typedef std::function<void(SharedWebElement)> WebElementCallBack;
typedef std::function<void(SharedWebElementList)> WebElementListCallBack;

Q_DECLARE_METATYPE(SharedWebElement);

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

    void AddContextMenu(QMenu *menu, SharedWebElement elem);
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
    static QString GetSelectionMenu(){ return m_SelectionMenu;}
    static QString GetRegularMenu(){ return m_RegularMenu;}

    static QList<float> GetZoomFactorLevels(){ return m_ZoomFactorLevels;}

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
    virtual void EmitScrollChangedIfNeed(){}

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

    static inline QString GetBaseUrlJsCode(){
        return QStringLiteral(
            "(function(){\n"
          VV"    var baseUrl = \"\";\n"
          VV"    var baseDocument = \"index.html\";\n"
          VV"    var base = document.getElementsByTagName(\"base\");\n"
          VV"    if(base.length > 0 && base[0].href){\n"
          VV"        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
          VV"    } else {\n"
          VV"        baseUrl = location.href;\n"
          VV"    }\n"
          VV"    return baseUrl;\n"
          VV"})()");
    }

    static inline QString GetCurrentBaseUrlJsCode(){
        return QStringLiteral(
            "(function(){\n"
          VV"    var doc = document;\n"
          VV"    for(var i = 0; i < frames.length; i++){\n"
          VV"        try{\n"
          VV"            if(frames[i].document.hasFocus()){\n"
          VV"                doc = frames[i].document;\n"
          VV"            }\n"
          VV"        }\n"
          VV"        catch(e){}\n"
          VV"    }\n"
          VV"    var baseUrl = \"\";\n"
          VV"    var baseDocument = \"index.html\";\n"
          VV"    var base = doc.getElementsByTagName(\"base\");\n"
          VV"    if(base.length > 0 && base[0].href){\n"
          VV"        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
          VV"    } else {\n"
          VV"        baseUrl = doc.location.href;\n"
          VV"    }\n"
          VV"    return baseUrl;\n"
          VV"})()");
    }

    static inline QString SetFocusToElementJsCode(const QString &xpath){
        QString quoted = QString(xpath).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
          VV"    var elem;\n"
          VV"    var doc = document;\n"
          VV"    var xpaths = \"%1\".split(\",\");\n"
          VV"    for(var i = 0; i < xpaths.length; i++){\n"
            //       7 : XPathResult.ORDERED_NODE_SNAPSHOT_TYPE
          VV"        elem = doc.evaluate(xpaths[i], doc, null, 7, null).snapshotItem(0);\n"
          VV"        try{\n"
          VV"            if(elem.contentDocument){\n"
          VV"                doc = elem.contentDocument;\n"
          VV"            }\n"
          VV"        }\n"
          VV"        catch(e){ break;}\n"
          VV"    }\n"
          VV"    elem.focus();\n"
          VV"})()").arg(quoted);
    }

    static inline QString FireClickEventJsCode(const QString &xpath, const QPoint &pos){
        QString quoted = QString(xpath).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
          VV"    var elem;\n"
          VV"    var doc = document;\n"
          VV"    var xpaths = \"%1\".split(\",\");\n"
          VV"    for(var i = 0; i < xpaths.length; i++){\n"
            //       7 : XPathResult.ORDERED_NODE_SNAPSHOT_TYPE
          VV"        elem = doc.evaluate(xpaths[i], doc, null, 7, null).snapshotItem(0);\n"
          VV"        try{\n"
          VV"            if(elem.contentDocument){\n"
          VV"                doc = elem.contentDocument;\n"
          VV"            }\n"
          VV"        }\n"
          VV"        catch(e){ break;}\n"
          VV"    }\n"
          VV"    var event = doc.createEvent(\"mouseEvents\");\n"
          VV"    event.initMouseEvent(\"click\", true, true, doc.defaultView, 0, 0, 0, %2, %3);\n"
          VV"    elem.dispatchEvent(event);\n"
          VV"})()").arg(quoted).arg(pos.x()).arg(pos.y());
    }

    // return value is js array. and it'll be callbacked.
    static inline QString GetScrollValuePointJsCode(){
        return QStringLiteral(
            "document.body && \n"
          VV"[document.body.scrollLeft, document.body.scrollTop]");
    }

    static inline QString SetScrollValuePointJsCode(const QPoint &pos){
        return QStringLiteral("scrollTo(%1, %2);").arg(pos.x()).arg(pos.y());
    }

    // return value is js array. and it'll be callbacked.
    static inline QString GetScrollBarStateJsCode(){
        return QStringLiteral(
            "document.documentElement &&\n"
          VV"[document.documentElement.scrollWidth - \n"
          VV" document.documentElement.clientWidth,\n"
          VV" document.documentElement.scrollHeight - \n"
          VV" document.documentElement.clientHeight];\n");
    }

    // return value is js array. and it'll be callbacked.
    static inline QString GetScrollRatioPointJsCode(){
        return QStringLiteral(
            "(function(){\n"
          VV"    if(!document.body || !document.documentElement) return;\n"
          VV"    var hval = document.body.scrollLeft;\n"
          VV"    var vval = document.body.scrollTop;\n"
          VV"    var hmax = document.documentElement.scrollWidth - \n"
          VV"               document.documentElement.clientWidth;\n"
          VV"    var vmax = document.documentElement.scrollHeight - \n"
          VV"               document.documentElement.clientHeight;\n"
          VV"    return [hmax <= 0 ? 0.5 : hval / hmax,\n"
          VV"            vmax <= 0 ? 0.5 : vval / vmax];\n"
          VV"})()");
    }

    static inline QString SetScrollRatioPointJsCode(const QPointF &pos){
        return QStringLiteral(
            "scrollTo(\n"
          VV"    (document.documentElement.scrollWidth - \n"
          VV"     document.documentElement.clientWidth) * \n"
          VV"     %1,\n"
          VV"    (document.documentElement.scrollHeight - \n"
          VV"     document.documentElement.clientHeight) * \n"
          VV"     %2);").arg(pos.x()).arg(pos.y());
    }

    static inline QString FindElementsJsCode(Page::FindElementsOption option){
        QString quoted = Page::OptionToSelector(option).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        QString ignoreOutOfView = option == Page::ForAccessKey ? QStringLiteral("true") : QStringLiteral("false");

        return QStringLiteral(
            "(function(){\n"
            // scroll bar value is unused.
          VV"    var scrollX = document.body.scrollLeft;\n"
          VV"    var scrollY = document.body.scrollTop;\n"
          VV"    var baseUrl = \"\";\n"
          VV"    var baseDocument = \"index.html\";\n"
          VV"    var base = document.getElementsByTagName(\"base\");\n"
          VV"    if(base.length > 0 && base[0].href){\n"
          VV"        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
          VV"    } else {\n"
          VV"        baseUrl = \n"
          VV"            location.protocol + \"//\" + location.hostname + \n"
          VV"            (location.port && \":\" + location.port) + \"/\";\n"
          VV"    }\n"
          VV"    var elems = Array.from(document.querySelectorAll(\"%1\"));\n"
          VV"    var map = {};\n"
          VV"    for(var i = 0; i < elems.length; i++){\n"
          VV"        var data = {};\n"
          VV"        data.tagName = elems[i].tagName;\n"
          VV"        data.innerText = elems[i].innerText;\n"
          VV"        data.linkUrl = elems[i].href;\n"
          VV"        data.linkHtml = elems[i].outerHTML;\n" // roughly capture.
          VV"        data.imageUrl = elems[i].src;\n"
          VV"        data.imageHtml = elems[i].innerHTML;\n" // roughly capture.
          VV"        data.baseUrl = baseUrl;\n"
          VV"        var offsetX = 0;\n"
          VV"        var offsetY = 0;\n"
          VV"        var win = elems[i].ownerDocument.defaultView;\n"
          VV"        while(win && top !== win){\n"
          VV"            var rect = win.frameElement.getBoundingClientRect();\n"
          VV"            offsetX += rect.left;\n"
          VV"            offsetY += rect.top;\n"
          VV"            win = win.parent;\n"
          VV"        }\n"
          VV"        var rect = elems[i].getBoundingClientRect();\n"
          VV"        data.x = (rect.left + offsetX) * devicePixelRatio;\n"
          VV"        data.y = (rect.top  + offsetY) * devicePixelRatio;\n"
          VV"        data.width  = rect.width  * devicePixelRatio;\n"
          VV"        data.height = rect.height * devicePixelRatio;\n"
          VV"        data.region = {};\n"
          VV"        if(data.width && data.height){\n"
          VV"            var rects = elems[i].getClientRects();\n"
          VV"            for(var j = 0; j < rects.length; j++){\n"
          VV"                data.region[j] = {};\n"
          VV"                data.region[j].x = (rects[j].left + offsetX) * devicePixelRatio;\n"
          VV"                data.region[j].y = (rects[j].top  + offsetY) * devicePixelRatio;\n"
          VV"                data.region[j].width  = rects[j].width  * devicePixelRatio;\n"
          VV"                data.region[j].height = rects[j].height * devicePixelRatio;\n"
          VV"            }\n"
        //VV"            var r1 = elems[i].ownerDocument.documentElement.getBoundingClientRect();\n"
          VV"            var w = elems[i].ownerDocument.defaultView;\n"
          VV"            var r1 = { left: 0, top: 0, right: w.innerWidth, bottom: w.innerHeight};\n"
          VV"            var r2 = elems[i].getBoundingClientRect();\n"
          VV"            if(%2 &&\n" // ignore out of view.
          VV"               (Math.max(r1.left, r2.left) >= Math.min(r1.right,  r2.right) ||\n"
          VV"                Math.max(r1.top,  r2.top)  >= Math.min(r1.bottom, r2.bottom))){\n"
          VV"                continue;\n"
          VV"            }\n"
          VV"        }\n"
          VV"        data.isJsCommand = \n"
          VV"            (elems[i].onclick ||\n"
          VV"             (elems[i].href &&\n"
          VV"              elems[i].href.lastIndexOf &&\n"
          VV"              elems[i].href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
          VV"             (elems[i].getAttribute &&\n"
          VV"              elems[i].getAttribute(\"role\") &&\n"
          VV"              (elems[i].getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"tab\"))) ? true : false;\n"
          VV"        data.isTextInput = \n"
          VV"            (elems[i].tagName == \"TEXTAREA\" ||\n"
          VV"             (elems[i].tagName == \"INPUT\" &&\n"
          VV"              elems[i].type &&\n"
          VV"              (elems[i].type.toLowerCase() == \"text\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"search\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"password\"))) ? true : false;\n"
          VV"        data.isQueryInput =\n"
          VV"            (elems[i].tagName == \"INPUT\" &&\n"
          VV"             elems[i].type &&\n"
          VV"             (elems[i].type.toLowerCase() == \"text\" ||\n"
          VV"              elems[i].type.toLowerCase() == \"search\")) ? true : false;\n"
          VV"        data.isEditable = \n"
          VV"            (elems[i].isContentEditable ||\n"
          VV"             data.isTextInput ||\n"
          VV"             data.isQueryInput) ? true : false;\n"
          VV"        data.isFrame = \n"
          VV"            (elems[i].tagName == \"FRAME\" ||\n"
          VV"             elems[i].tagName == \"IFRAME\") ? true : false;\n"
          VV"        data.action = \n"
          VV"            (elems[i].href &&\n"
          VV"             elems[i].href.lastIndexOf &&\n"
          VV"             (elems[i].href.lastIndexOf(\"http:\", 0) === 0 ||\n"
          VV"              elems[i].href.lastIndexOf(\"https:\", 0) === 0)) ? \"None\" :\n"
          VV"            (elems[i].isContentEditable ||\n"
          VV"             elems[i].tagName == \"TEXTAREA\" ||\n"
          VV"             elems[i].tagName == \"OBJECT\"   ||\n"
          VV"             elems[i].tagName == \"EMBED\"    ||\n"
          VV"             elems[i].tagName == \"FRAME\"    ||\n"
          VV"             elems[i].tagName == \"IFRAME\"   ||\n"
          VV"             (elems[i].tagName == \"INPUT\" &&\n"
          VV"              elems[i].type &&\n"
          VV"              (elems[i].type.toLowerCase() == \"text\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"search\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"password\"))) ? \"Focus\" :\n"
          VV"            (elems[i].onclick ||\n"
          VV"             (elems[i].href &&\n"
          VV"              elems[i].href.lastIndexOf &&\n"
          VV"              elems[i].href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
          VV"             elems[i].tagName == \"BUTTON\" ||\n"
          VV"             elems[i].tagName == \"SELECT\" ||\n"
          VV"             elems[i].tagName == \"LABEL\"  ||\n"
          VV"             (elems[i].getAttribute &&\n"
          VV"              elems[i].getAttribute(\"role\") &&\n"
          VV"              (elems[i].getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
          VV"               elems[i].getAttribute(\"role\").toLowerCase() == \"tab\")) ||\n"
          VV"             (elems[i].tagName == \"INPUT\" &&\n"
          VV"              elems[i].type &&\n"
          VV"              (elems[i].type.toLowerCase() == \"checkbox\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"radio\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"file\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"submit\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"reset\" ||\n"
          VV"               elems[i].type.toLowerCase() == \"button\"))) ? \"Click\" :\n"
          VV"            (elems[i].onmouseover) ? \"Hover\" :\n"
          VV"            \"None\";\n"
          VV"        if(elems[i].tagName == \"FRAME\" || elems[i].tagName == \"IFRAME\"){\n"
          VV"            try{\n"
          VV"                var frameDocument = elems[i].contentDocument;\n"
          VV"                elems = elems.concat(Array.from(frameDocument.querySelectorAll(\"%1\")));\n"
          VV"            }\n"
          VV"            catch(e){}\n"
          VV"        }\n"
          VV"        var xpath = \"\";\n"
          VV"        var iter = elems[i];\n"
            //       1 : document.ELEMENT_NODE
          VV"        while(iter && iter.nodeType == 1){\n"
          VV"            var str = iter.tagName;\n"
          VV"            var siblings = iter.parentNode.childNodes;\n"
          VV"            var synonym = [];\n"
          VV"            for(var j = 0; j < siblings.length; j++){\n"
          VV"                if(siblings[j].nodeType == 1 &&\n"
          VV"                   siblings[j].tagName == iter.tagName){\n"
          VV"                    synonym.push(siblings[j]);\n"
          VV"                }\n"
          VV"            }\n"
          VV"            if(synonym.length > 1 && synonym.indexOf(iter) != -1){\n"
          VV"                str += \"[\" + (synonym.indexOf(iter) + 1) + \"]\";\n"
          VV"            }\n"
          VV"            if(xpath && !xpath.startsWith(\",\")){\n"
          VV"                xpath = str + \"/\" + xpath;\n"
          VV"            } else {\n"
          VV"                xpath = str + xpath;\n"
          VV"            }\n"
          VV"            iter = iter.parentNode;\n"
            //           9 : document.DOCUMENT_NODE
          VV"            if(iter && iter.nodeType == 9 && iter !== document){\n"
          VV"                xpath = \",//\" + xpath.toLowerCase();\n"
          VV"                iter = iter.defaultView.frameElement;\n"
          VV"            }\n"
          VV"        }\n"
          VV"        data.xPath = \"//\" + xpath.toLowerCase();\n"
          VV"        map[i] = data;\n"
          VV"    }\n"
          VV"    return map;\n"
          VV"})()").arg(quoted).arg(ignoreOutOfView);
    }

    static inline QString HitElementJsCode(QPoint pos){
        return QStringLiteral(
            "(function(){\n"
            // scroll bar value is unused.
          VV"    var scrollX = document.body.scrollLeft;\n"
          VV"    var scrollY = document.body.scrollTop;\n"
          VV"    var baseUrl = \"\";\n"
          VV"    var baseDocument = \"index.html\";\n"
          VV"    var base = document.getElementsByTagName(\"base\");\n"
          VV"    if(base.length > 0 && base[0].href){\n"
          VV"        baseUrl = base[0].href.replace(baseDocument, \"\");\n"
          VV"    } else {\n"
          VV"        baseUrl = \n"
          VV"            location.protocol + \"//\" + location.hostname + \n"
          VV"            (location.port && \":\" + location.port) + \"/\";\n"
          VV"    }\n"
          VV"    var x = %1;\n"
          VV"    var y = %2;\n"
          VV"    var elem = document.elementFromPoint(x, y);\n"
          VV"    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
          VV"        try{\n"
          VV"            var frameDocument = elem.contentDocument;\n"
          VV"            var rect = elem.getBoundingClientRect();\n"
          VV"            x -= rect.left;\n"
          VV"            y -= rect.top;\n"
          VV"            elem = frameDocument.elementFromPoint(x, y);\n"
          VV"        }\n"
          VV"        catch(e){ break;}\n"
          VV"    }\n"
          VV"    if(!elem) return;\n"
          VV"    var data = {};\n"
          VV"    data.tagName = elem.tagName;\n"
          VV"    data.innerText = elem.innerText;\n"
          VV"    var link = elem;\n"
          VV"    while(link){\n"
          VV"        if(link.href){\n"
          VV"            data.linkUrl = link.href;\n"
          VV"            data.linkHtml = link.outerHTML;\n"
          VV"            break;\n"
          VV"        }\n"
          VV"        link = link.parentNode;\n"
          VV"    }\n"
          VV"    var image = elem;\n"
          VV"    while(image){\n"
          VV"        if(image.src){\n"
          VV"            data.imageUrl = image.src;\n"
          VV"            data.imageHtml = image.outerHTML;\n"
          VV"            break;\n"
          VV"        }\n"
          VV"        image = image.parentNode;\n"
          VV"    }\n"
          VV"    data.baseUrl = baseUrl;\n"
          VV"    var offsetX = 0;\n"
          VV"    var offsetY = 0;\n"
          VV"    win = elem.ownerDocument.defaultView;\n"
          VV"    while(win && top !== win){\n"
          VV"        var rect = win.frameElement.getBoundingClientRect();\n"
          VV"        offsetX += rect.left;\n"
          VV"        offsetY += rect.top;\n"
          VV"        win = win.parent;\n"
          VV"    }\n"
          VV"    var rect = elem.getBoundingClientRect();\n"
          VV"    data.x = (rect.left + offsetX) * devicePixelRatio;\n"
          VV"    data.y = (rect.top  + offsetY) * devicePixelRatio;\n"
          VV"    data.width = rect.width   * devicePixelRatio;\n"
          VV"    data.height = rect.height * devicePixelRatio;\n"
          VV"    data.region = {};\n"
          VV"    var rects = elem.getClientRects();\n"
          VV"    for(var i = 0; i < rects.length; i++){\n"
          VV"        data.region[i] = {};\n"
          VV"        data.region[i].x = (rects[i].left + offsetX) * devicePixelRatio;\n"
          VV"        data.region[i].y = (rects[i].top  + offsetY) * devicePixelRatio;\n"
          VV"        data.region[i].width  = rects[i].width  * devicePixelRatio;\n"
          VV"        data.region[i].height = rects[i].height * devicePixelRatio;\n"
          VV"    }\n"
          VV"    data.isJsCommand = \n"
          VV"        (elem.onclick ||\n"
          VV"         (elem.href &&\n"
          VV"          elem.href.lastIndexOf &&\n"
          VV"          elem.href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
          VV"         (elem.getAttribute &&\n"
          VV"          elem.getAttribute(\"role\") &&\n"
          VV"          (elem.getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"tab\"))) ? true : false;\n"
          VV"    data.isTextInput = \n"
          VV"        (elem.tagName == \"TEXTAREA\" ||\n"
          VV"         (elem.tagName == \"INPUT\" &&\n"
          VV"          elem.type &&\n"
          VV"          (elem.type.toLowerCase() == \"text\" ||\n"
          VV"           elem.type.toLowerCase() == \"search\" ||\n"
          VV"           elem.type.toLowerCase() == \"password\"))) ? true : false;\n"
          VV"    data.isQueryInput = \n"
          VV"        (elem.tagName == \"INPUT\" &&\n"
          VV"         elem.type &&\n"
          VV"         (elem.type.toLowerCase() == \"text\" ||\n"
          VV"          elem.type.toLowerCase() == \"search\")) ? true : false;\n"
          VV"    data.isEditable = \n"
          VV"        (elem.isContentEditable ||\n"
          VV"         data.isTextInput ||\n"
          VV"         data.isQueryInput) ? true : false;\n"
          VV"    data.isFrame = \n"
          VV"        (elem.tagName == \"FRAME\" ||\n"
          VV"         elem.tagName == \"IFRAME\") ? true : false;\n"
          VV"    data.action = \n"
          VV"        (elem.href &&\n"
          VV"         elem.href.lastIndexOf &&\n"
          VV"         (elem.href.lastIndexOf(\"http:\", 0) === 0 ||\n"
          VV"          elem.href.lastIndexOf(\"https:\", 0) === 0)) ? \"None\" :\n"
          VV"        (elem.isContentEditable ||\n"
          VV"         elem.tagName == \"TEXTAREA\" ||\n"
          VV"         elem.tagName == \"OBJECT\"   ||\n"
          VV"         elem.tagName == \"EMBED\"    ||\n"
          VV"         elem.tagName == \"FRAME\"    ||\n"
          VV"         elem.tagName == \"IFRAME\"   ||\n"
          VV"         (elem.tagName == \"INPUT\" &&\n"
          VV"          elem.type &&\n"
          VV"          (elem.type.toLowerCase() == \"text\" ||\n"
          VV"           elem.type.toLowerCase() == \"search\" ||\n"
          VV"           elem.type.toLowerCase() == \"password\"))) ? \"Focus\" :\n"
          VV"        (elem.onclick ||\n"
          VV"         (elem.href &&\n"
          VV"          elem.href.lastIndexOf &&\n"
          VV"          elem.href.lastIndexOf(\"javascript:\", 0) === 0) ||\n"
          VV"         elem.tagName == \"BUTTON\" ||\n"
          VV"         elem.tagName == \"SELECT\" ||\n"
          VV"         elem.tagName == \"LABEL\"  ||\n"
          VV"         (elem.getAttribute &&\n"
          VV"          elem.getAttribute(\"role\") &&\n"
          VV"          (elem.getAttribute(\"role\").toLowerCase() == \"button\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"link\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"menu\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"checkbox\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"radio\" ||\n"
          VV"           elem.getAttribute(\"role\").toLowerCase() == \"tab\")) ||\n"
          VV"         (elem.tagName == \"INPUT\" &&\n"
          VV"          elem.type &&\n"
          VV"          (elem.type.toLowerCase() == \"checkbox\" ||\n"
          VV"           elem.type.toLowerCase() == \"radio\" ||\n"
          VV"           elem.type.toLowerCase() == \"file\" ||\n"
          VV"           elem.type.toLowerCase() == \"submit\" ||\n"
          VV"           elem.type.toLowerCase() == \"reset\" ||\n"
          VV"           elem.type.toLowerCase() == \"button\"))) ? \"Click\" :\n"
          VV"         (elem.onmouseover) ? \"Hover\" :\n"
          VV"         \"None\";\n"
          VV"    var xpath = \"\";\n"
          VV"    var iter = elem;\n"
            //   1 : document.ELEMENT_NODE
          VV"    while(iter && iter.nodeType == 1){\n"
          VV"        var str = iter.tagName;\n"
          VV"        var siblings = iter.parentNode.childNodes;\n"
          VV"        var synonym = [];\n"
          VV"        for(var j = 0; j < siblings.length; j++){\n"
          VV"            if(siblings[j].nodeType == 1 &&\n"
          VV"               siblings[j].tagName == iter.tagName){\n"
          VV"                synonym.push(siblings[j]);\n"
          VV"            }\n"
          VV"        }\n"
          VV"        if(synonym.length > 1 && synonym.indexOf(iter) != -1){\n"
          VV"            str += \"[\" + (synonym.indexOf(iter) + 1) + \"]\";\n"
          VV"        }\n"
          VV"        if(xpath && !xpath.startsWith(\",\")){\n"
          VV"            xpath = str + \"/\" + xpath;\n"
          VV"        } else {\n"
          VV"            xpath = str + xpath;\n"
          VV"        }\n"
          VV"        iter = iter.parentNode;\n"
            //       9 : document.DOCUMENT_NODE
          VV"        if(iter && iter.nodeType == 9 && iter !== document){\n"
          VV"            xpath = \",//\" + xpath.toLowerCase();\n"
          VV"            iter = iter.defaultView.frameElement;\n"
          VV"        }\n"
          VV"    }\n"
          VV"    data.xPath = \"//\" + xpath.toLowerCase();\n"
          VV"    return data;\n"
          VV"})()").arg(pos.x()).arg(pos.y());
    }

    static inline QString HitLinkUrlJsCode(QPoint pos){
        return QStringLiteral(
            "(function(){\n"
          VV"    var x = %1;\n"
          VV"    var y = %2;\n"
          VV"    var elem = document.elementFromPoint(x, y);\n"
          VV"    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
          VV"        try{\n"
          VV"            var frameDocument = elem.contentDocument;\n"
          VV"            var rect = elem.getBoundingClientRect();\n"
          VV"            x -= rect.left;\n"
          VV"            y -= rect.top;\n"
          VV"            elem = frameDocument.elementFromPoint(x, y);\n"
          VV"        }\n"
          VV"        catch(e){ break;}\n"
          VV"    }\n"
          VV"    while(elem){\n"
          VV"        if(elem.href) return elem.href;\n"
          VV"        elem = elem.parentNode;\n"
          VV"    }\n"
          VV"    return \"\";\n"
          VV"})()").arg(pos.x()).arg(pos.y());
    }

    static inline QString HitImageUrlJsCode(QPoint pos){
        return QStringLiteral(
            "(function(){\n"
          VV"    var x = %1;\n"
          VV"    var y = %2;\n"
          VV"    var elem = document.elementFromPoint(x, y);\n"
          VV"    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
          VV"        try{\n"
          VV"            var frameDocument = elem.contentDocument;\n"
          VV"            var rect = elem.getBoundingClientRect();\n"
          VV"            x -= rect.left;\n"
          VV"            y -= rect.top;\n"
          VV"            elem = frameDocument.elementFromPoint(x, y);\n"
          VV"        }\n"
          VV"        catch(e){ break;}\n"
          VV"    }\n"
          VV"    while(elem){\n"
          VV"        if(elem.src) return elem.src;\n"
          VV"        elem = elem.parentNode;\n"
          VV"    }\n"
          VV"    return \"\";\n"
          VV"})()").arg(pos.x()).arg(pos.y());
    }

    static inline QString SelectedTextJsCode(){
        return QStringLiteral("getSelection().toString()");
    }

    static inline QString SelectedHtmlJsCode(){
        return QStringLiteral(
            "(function(){\n"
          VV"    var div = document.createElement(\"div\");\n"
          VV"    var selection = getSelection();\n"
          VV"    if(!selection.rangeCount) return \"\";\n"
          VV"    div.appendChild(selection.getRangeAt(0).cloneContents());\n"
          VV"    return div.innerHTML;\n"
          VV"})()");
    }

    static inline QString WholeTextJsCode(){
        return QStringLiteral("document.documentElement.innerText");
    }

    static inline QString WholeHtmlJsCode(){
        return QStringLiteral("document.documentElement.outerHTML");
    }

    static inline QString SelectionRegionJsCode(){
        return QStringLiteral(
            "(function(){\n"
          VV"    var map = {};\n"
          VV"    var selection = getSelection();\n"
          VV"    if(!selection.rangeCount) return map;\n"
          VV"    var rects = selection.getRangeAt(0).getClientRects();\n"
          VV"    for(var i = 0; i < rects.length; i++){\n"
          VV"        map[i] = {};\n"
          VV"        map[i].x = rects[i].left * devicePixelRatio;\n"
          VV"        map[i].y = rects[i].top  * devicePixelRatio;\n"
          VV"        map[i].width  = rects[i].width  * devicePixelRatio;\n"
          VV"        map[i].height = rects[i].height * devicePixelRatio;\n"
          VV"    }\n"
          VV"    return map;\n"
          VV"})()");
    }

    static inline QString SetTextValueJsCode(const QString &xpath, const QString &text){
        QString quotedXpath = QString(xpath).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        QString quotedText = QString(text).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
          VV"    var elem;\n"
          VV"    var doc = document;\n"
          VV"    var xpaths = \"%1\".split(\",\");\n"
          VV"    for(var i = 0; i < xpaths.length; i++){\n"
            //       7 : XPathResult.ORDERED_NODE_SNAPSHOT_TYPE
          VV"        elem = doc.evaluate(xpaths[i], doc, null, 7, null).snapshotItem(0);\n"
          VV"        try{\n"
          VV"            if(elem.contentDocument){\n"
          VV"                doc = elem.contentDocument;\n"
          VV"            }\n"
          VV"        }\n"
          VV"        catch(e){ break;}\n"
          VV"    }\n"
          VV"    elem.setAttribute(\"value\", \"%2\");\n"
          VV"    elem.focus();\n"
          VV"})()").arg(quotedXpath).arg(quotedText);
    }

    static inline QString DecorateFormFieldJsCode(const QString &data){
        QString quoted = QString(data).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
          VV"    var data = \"%1\".split(\"&\");\n"
          VV"    var forms = document.querySelectorAll(\"form\");\n"
          VV"    for(var i = 0; i < forms.length; i++){\n"
          VV"        var form = forms[i];\n"
          VV"        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
          VV"        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
          VV"        if(!submit || !password) continue;\n"
          VV"        for(var j = 0; j < data.length; j++){\n"
          VV"            var pair = data[j].split(\"=\");\n"
          VV"            var name = decodeURIComponent(pair[0]);\n"
          VV"            var val  = decodeURIComponent(pair[1]);\n"
          VV"            var field = form.querySelector(\"*[name=\\\"\" + name + \"\\\"]\");\n"
          VV"            if(!field) continue;\n"
          VV"            field.style.boxShadow = \"inset 0 0 2px 2px #eca\";\n"
          VV"            field.style.border = \"1px\";\n"
          VV"        }\n"
          VV"    }\n"
          VV"})()").arg(quoted);
    }

    static inline QString SubmitFormDataJsCode(const QString &data){
        QString quoted = QString(data).replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return QStringLiteral(
            "(function(){\n"
          VV"    var submitted = false;\n"
          VV"    var data = \"%1\".split(\"&\");\n"
          VV"    var forms = document.querySelectorAll(\"form\");\n"
          VV"    for(var i = 0; i < forms.length; i++){\n"
          VV"        var form = forms[i];\n"
          VV"        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
          VV"        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
          VV"        if(!submit || !password) continue;\n"
          VV"        for(var j = 0; j < data.length; j++){\n"
          VV"            var pair = data[j].split(\"=\");\n"
          VV"            var name = decodeURIComponent(pair[0]);\n"
          VV"            var val  = decodeURIComponent(pair[1]);\n"
          VV"            var field = form.querySelector(\"*[name=\\\"\" + name + \"\\\"]\");\n"
          VV"            if(!field) continue;\n"
          VV"            submitted = true;\n"
          VV"            field.value = val;\n"
          VV"        }\n"
          VV"        if(!submitted) continut;\n"
          VV"        if(submit.click){\n"
          VV"            submit.click();\n"
          VV"        } else if(typeof submit == \"function\"){\n"
          VV"            submit();\n"
          VV"        }\n"
          VV"    }\n"
          VV"})()").arg(quoted);
    }

    static inline QString InstallEventFilterJsCode(const QList<QEvent::Type> &types){
        QString inner;
        if(types.contains(QEvent::KeyPress))
            inner += QStringLiteral(
                "\n"
              VV"doc.addEventListener(\"keydown\", function(e){\n"
              VV"    var prevent = false;\n"
              VV"    var elem = e.target.ownerDocument.activeElement;\n"
              VV"    if(e.keyCode == 9 || e.keyCode == 13){\n"
              VV"        prevent = false;\n"
              VV"        console.info(\"keyPressEvent%1,\" + \n"
              VV"                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
              VV"                     e.ctrlKey.toString() + \",\" + e.altKey.toString());\n"
              VV"    } else if(!e.altKey && !e.ctrlKey &&\n"
              VV"       32 <= e.keyCode && e.keyCode <= 40){\n"
              VV"        prevent = false;\n"
            /*VV"        console.info(\"keyPressEvent%1,\" + \n"
              VV"                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
              VV"                     e.ctrlKey.toString() + \",\" + e.altKey.toString());\n"*/
              VV"    } else if(elem.isContentEditable ||\n"
              VV"              elem.tagName == \"BUTTON\" ||\n"
              VV"              elem.tagName == \"SELECT\" ||\n"
              VV"              elem.tagName == \"INPUT\"  ||\n"
              VV"              elem.tagName == \"TEXTAREA\" ||\n"
              VV"              elem.tagName == \"FRAME\" ||\n"
              VV"              elem.tagName == \"IFRAME\"){\n"
              VV"        if(e.ctrlKey || e.altKey){\n"
              VV"            prevent = true;\n"
              VV"        }\n"
              VV"    } else {\n"
              VV"        prevent = true;\n"
              VV"    }\n"
              VV"    if(prevent){\n"
              VV"        console.info(\"keyPressEvent%1,\" + \n"
              VV"                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
              VV"                     e.ctrlKey.toString() + \",\" + e.altKey.toString());\n"
              VV"        e.preventDefault();\n"
              VV"    }\n"
              VV"}, false);\n");

        if(types.contains(QEvent::KeyRelease))
            inner += QStringLiteral(
                "\n"
              VV"doc.addEventListener(\"keyup\", function(e){\n"
              VV"    var prevent = false;\n"
              VV"    var elem = e.target.ownerDocument.activeElement;\n"
              VV"    if(e.keyCode == 9 || e.keyCode == 13){\n"
              VV"        prevent = false;\n"
              VV"        console.info(\"keyReleaseEvent%1,\" + \n"
              VV"                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
              VV"                     e.ctrlKey.toString() + \",\" + e.altKey.toString());\n"
              VV"    } else if(!e.altKey && !e.ctrlKey &&\n"
              VV"       32 <= e.keyCode && e.keyCode <= 40){\n"
              VV"        prevent = false;\n"
              VV"        console.info(\"keyReleaseEvent%1,\" + \n"
              VV"                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
              VV"                     e.ctrlKey.toString() + \",\" + e.altKey.toString());\n"
              VV"    } else if(elem.isContentEditable ||\n"
              VV"              elem.tagName == \"BUTTON\" ||\n"
              VV"              elem.tagName == \"SELECT\" ||\n"
              VV"              elem.tagName == \"INPUT\"  ||\n"
              VV"              elem.tagName == \"TEXTAREA\" ||\n"
              VV"              elem.tagName == \"FRAME\" ||\n"
              VV"              elem.tagName == \"IFRAME\"){\n"
              VV"        if(e.ctrlKey || e.altKey){\n"
              VV"            prevent = true;\n"
              VV"        }\n"
              VV"    } else {\n"
              VV"        prevent = true;\n"
              VV"    }\n"
              VV"    if(prevent){\n"
              VV"        console.info(\"keyReleaseEvent%1,\" + \n"
              VV"                     e.keyCode.toString() + \",\" + e.shiftKey.toString() + \",\" + \n"
              VV"                     e.ctrlKey.toString() + \",\" + e.altKey.toString());\n"
              VV"        e.preventDefault();\n"
              VV"    }\n"
              VV"}, false);\n");

        if(types.contains(QEvent::MouseMove))
            inner += QStringLiteral(
                "\n"
              VV"doc.addEventListener(\"mousemove\", function(e){\n"
              VV"    console.info(\"mouseMoveEvent%1,\" + \n"
              VV"                 e.button.toString() + \",\" + \n"
              VV"                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
              VV"                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
              VV"                 e.altKey.toString());\n"
              VV"}, false);\n");

        if(types.contains(QEvent::MouseButtonPress))
            inner += QStringLiteral(
                "\n"
              VV"doc.addEventListener(\"mousedown\", function(e){\n"
              VV"    console.info(\"mousePressEvent%1,\" + \n"
              VV"                 e.button.toString() + \",\" + \n"
              VV"                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
              VV"                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
              VV"                 e.altKey.toString());\n"
              VV"}, false);\n");

        if(types.contains(QEvent::MouseButtonRelease))
            inner += QStringLiteral(
                "\n"
              VV"doc.addEventListener(\"mouseup\", function(e){\n"
              VV"    console.info(\"mouseReleaseEvent%1,\" + \n"
              VV"                 e.button.toString() + \",\" + \n"
              VV"                 e.clientX.toString() + \",\" + e.clientY.toString() + \",\" + \n"
              VV"                 e.shiftKey.toString() + \",\" + e.ctrlKey.toString() + \",\" + \n"
              VV"                 e.altKey.toString());\n"
              VV"}, false);\n");

        if(types.contains(QEvent::Wheel))
            inner += QStringLiteral(
                "\n"
              VV"doc.addEventListener(\"mousewheel\", function(e){\n"
              VV"    console.info(\"wheelEvent%1,\" + e.wheelDelta.toString());\n"
              VV"}, false);\n");

        return QStringLiteral(
                "(function(){\n"
              VV"    var wins = [window];\n"
              VV"    wins = wins.concat(Array.from(frames));\n"
              VV"    for(var i = 0; i < wins.length; i++){\n"
              VV"        try{\n"
              VV"            var doc = wins[i].document;\n"
              VV"            %1\n"
              VV"        }\n"
              VV"        catch(e){}\n"
              VV"    }\n"
              VV"})()").arg(inner.arg(Application::EventKey()));
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
    template <class T> T WaitForResult(std::function<void(std::function<void(T)>)> callBack){
        QTimer timer;
        QEventLoop loop;
        T result;
        bool called = false;

        timer.setSingleShot(true);
        QObject::connect(base(), SIGNAL(destroyed()), &loop, SLOT(quit()));
        QObject::connect(&timer, SIGNAL(timeout()),   &loop, SLOT(quit()));

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

class JsWebElement : public WebElement{
public:
    explicit JsWebElement();
    explicit JsWebElement(View *provider, QVariant var);
    virtual ~JsWebElement();

    virtual bool SetFocus() DECL_OVERRIDE;
    virtual bool ClickEvent() DECL_OVERRIDE;
    virtual QString TagName() const DECL_OVERRIDE;
    virtual QString InnerText() const DECL_OVERRIDE;
    virtual QUrl BaseUrl() const DECL_OVERRIDE;
    virtual QUrl LinkUrl() const DECL_OVERRIDE;
    virtual QUrl ImageUrl() const DECL_OVERRIDE;
    virtual QString LinkHtml() const DECL_OVERRIDE;
    virtual QString ImageHtml() const DECL_OVERRIDE;
    virtual QPoint Position() const DECL_OVERRIDE;
    virtual QRect Rectangle() const DECL_OVERRIDE;
    virtual QRegion Region() const DECL_OVERRIDE;
    virtual void SetPosition(QPoint) DECL_OVERRIDE;
    virtual void SetRectangle(QRect) DECL_OVERRIDE;
    virtual void SetText(QString) DECL_OVERRIDE;
    virtual QPixmap Pixmap() DECL_OVERRIDE;
    virtual bool IsNull() const DECL_OVERRIDE;
    virtual bool IsEditableElement() const DECL_OVERRIDE;
    virtual bool IsJsCommandElement() const DECL_OVERRIDE;
    virtual bool IsTextInputElement() const DECL_OVERRIDE;
    virtual bool IsQueryInputElement() const DECL_OVERRIDE;
    virtual bool IsFrameElement() const DECL_OVERRIDE;
    virtual Action GetAction() const DECL_OVERRIDE;
    virtual bool Equals(const WebElement&) const DECL_OVERRIDE;

protected:
    View *m_Provider;
    QString m_TagName;
    QString m_InnerText;
    QUrl m_BaseUrl;
    QUrl m_LinkUrl;
    QUrl m_ImageUrl;
    QString m_LinkHtml;
    QString m_ImageHtml;
    QRect m_Rectangle;
    QRegion m_Region;
    bool m_IsJsCommand;
    bool m_IsTextInput;
    bool m_IsQueryInput;
    bool m_IsEditable;
    bool m_IsFrame;
    QString m_XPath;
    QString m_Action;
};

#endif //ifndef VIEW_HPP
