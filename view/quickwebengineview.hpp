#ifndef QUICKWEBENGINEVIEW_HPP
#define QUICKWEBENGINEVIEW_HPP

#include "switch.hpp"

#include "webenginepage.hpp"
#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#include <QWindow>
#include <QMenuBar>
#include <QQuickItem>
#include <QNetworkRequest>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QWebEngineView>

class QuickWebEngineView : public QQuickWidget, public View{
    Q_OBJECT

public:
    QuickWebEngineView(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    ~QuickWebEngineView();

    void ApplySpecificSettings(QStringList set) DECL_OVERRIDE;

    QQuickWidget *base() DECL_OVERRIDE;
    WebEnginePage *page() DECL_OVERRIDE;

    QUrl url() DECL_OVERRIDE;
    QString html() DECL_OVERRIDE;
    TreeBank *parent() DECL_OVERRIDE;
    void setUrl(const QUrl &url) DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) DECL_OVERRIDE;
    void setParent(TreeBank* t) DECL_OVERRIDE;

    void Connect(TreeBank *tb) DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) DECL_OVERRIDE;

    QUrl BaseUrl() DECL_OVERRIDE {
        return GetBaseUrl();
    }
    QUrl CurrentBaseUrl() DECL_OVERRIDE {
        return GetBaseUrl();
    }

    bool ForbidToOverlap() DECL_OVERRIDE { return true;}

    bool CanGoBack() DECL_OVERRIDE {
        return m_QmlWebEngineView->property("canGoBack").toBool();
    }
    bool CanGoForward() DECL_OVERRIDE {
        return m_QmlWebEngineView->property("canGoForward").toBool();
    }
#if QT_VERSION >= 0x050700
    bool RecentlyAudible() DECL_OVERRIDE {
        return m_QmlWebEngineView->property("recentlyAudible").toBool();
    }
    bool IsAudioMuted() DECL_OVERRIDE {
        return m_QmlWebEngineView->property("audioMuted").toBool();
    }
    void SetAudioMuted(bool muted) DECL_OVERRIDE {
        m_QmlWebEngineView->setProperty("audioMuted", muted);
    }
#endif

    bool IsRenderable() DECL_OVERRIDE {
        return status() == QQuickWidget::Ready && (visible() || !m_GrabedDisplayData.isNull());
    }
    void Render(QPainter *painter) DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = grabFramebuffer();
        painter->drawImage(QPoint(), m_GrabedDisplayData);
    }
    void Render(QPainter *painter, const QRegion &clip) DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = grabFramebuffer();
        foreach(QRect rect, clip.rects()){
            painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        }
    }
    // if this is hidden(or minimized), cannot render.
    // using display cache, instead of real time rendering.
    QSize GetViewportSize() DECL_OVERRIDE {
        return visible() ? size() : m_GrabedDisplayData.size();
    }
    void SetViewportSize(QSize size) DECL_OVERRIDE {
        if(!visible()) resize(size);
    }
    void SetSource(const QUrl &url) DECL_OVERRIDE {
        if(page()) page()->SetSource(url);
    }
    void SetSource(const QByteArray &html){
        if(page()) page()->SetSource(html);
    }
    void SetSource(const QString &html){
        if(page()) page()->SetSource(html);
    }
    QString GetTitle() DECL_OVERRIDE {
        return m_QmlWebEngineView->property("title").toString();
    }
    QIcon GetIcon() DECL_OVERRIDE {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) DECL_OVERRIDE { m_QmlWebEngineView->setProperty("url", url);}
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray & = QByteArray()) DECL_OVERRIDE { m_QmlWebEngineView->setProperty("url", req.url());}
    void TriggerNativeGoBackAction() DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebEngineView, "goBack");}
    void TriggerNativeGoForwardAction() DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebEngineView, "goForward");}

    void UpKeyEvent()       DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_UpKey);       QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void DownKeyEvent()     DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_DownKey);     QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void RightKeyEvent()    DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_RightKey);    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void LeftKeyEvent()     DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_LeftKey);     QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void PageDownKeyEvent() DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_PageDownKey); QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void PageUpKeyEvent()   DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_PageUpKey);   QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void HomeKeyEvent()     DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_HomeKey);     QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}
    void EndKeyEvent()      DECL_OVERRIDE { QQuickWidget::keyPressEvent(m_EndKey);      QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");}

    void KeyPressEvent(QKeyEvent *ev) DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) DECL_OVERRIDE { mousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) DECL_OVERRIDE { wheelEvent(ev);}

    void CallWithGotBaseUrl(UrlCallBack) DECL_OVERRIDE;
    void CallWithGotCurrentBaseUrl(UrlCallBack) DECL_OVERRIDE;
    void CallWithFoundElements(Page::FindElementsOption, WebElementListCallBack) DECL_OVERRIDE;
    void CallWithHitElement(const QPoint&, WebElementCallBack) DECL_OVERRIDE;
    void CallWithHitLinkUrl(const QPoint&, UrlCallBack) DECL_OVERRIDE;
    void CallWithHitImageUrl(const QPoint&, UrlCallBack) DECL_OVERRIDE;
    void CallWithSelectedText(StringCallBack) DECL_OVERRIDE;
    void CallWithSelectedHtml(StringCallBack) DECL_OVERRIDE;
    void CallWithWholeText(StringCallBack) DECL_OVERRIDE;
    void CallWithWholeHtml(StringCallBack) DECL_OVERRIDE;
    void CallWithSelectionRegion(RegionCallBack) DECL_OVERRIDE;
    void CallWithEvaluatedJavaScriptResult(const QString&, VariantCallBack) DECL_OVERRIDE;

public slots:
    QSize size() DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) DECL_OVERRIDE {
        rootObject()->setProperty("width", size.width());
        rootObject()->setProperty("height", size.height());
        base()->setGeometry(QRect(QPoint(), size));
    }
    void show() DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
        if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();

        // view become to stop updating, when only call show method.
        // e.g. in coming back after making other view.
        MainWindow *win = Application::GetCurrentWindow();
        QSize s =
            m_TreeBank ? m_TreeBank->size() :
            win ? win->GetTreeBank()->size() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;
        resize(QSize(s.width(), s.height()+1));
        resize(s);

        // set only notifier.
        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        CallWithScroll([this](QPointF pos){
                if(m_TreeBank){
                    if(Notifier *notifier = m_TreeBank->GetNotifier()){
                        notifier->SetScroll(pos);
                    }
                }
            });
    }
    void hide()    DECL_OVERRIDE { base()->hide();}
    void raise()   DECL_OVERRIDE { base()->raise();}
    void lower()   DECL_OVERRIDE { base()->lower();}
    void repaint() DECL_OVERRIDE {
        QSize s = size();
        if(s.isEmpty()) return;
        resize(QSize(s.width(), s.height()+1));
        resize(s);
    }
    bool visible() DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) DECL_OVERRIDE {
        base()->setFocus(reason);
        m_QmlWebEngineView->setProperty("focus", true);
    }

    void Load()                           DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) DECL_OVERRIDE { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() DECL_OVERRIDE {}
    void OnAfterFinishingDisplayGadgets() DECL_OVERRIDE {
#if defined(Q_OS_WIN)
        if(TreeBank::TridentViewExist()) show(); // for force update.
#endif
    }

    void OnSetViewNode(ViewNode*) DECL_OVERRIDE;
    void OnSetHistNode(HistNode*) DECL_OVERRIDE;
    void OnSetThis(WeakView) DECL_OVERRIDE;
    void OnSetMaster(WeakView) DECL_OVERRIDE;
    void OnSetSlave(WeakView) DECL_OVERRIDE;
    void OnSetJsObject(_View*) DECL_OVERRIDE;
    void OnSetJsObject(_Vanilla*) DECL_OVERRIDE;
    void OnLoadStarted() DECL_OVERRIDE;
    void OnLoadProgress(int) DECL_OVERRIDE;
    void OnLoadFinished(bool) DECL_OVERRIDE;
    void OnTitleChanged(const QString&) DECL_OVERRIDE;
    void OnUrlChanged(const QUrl&) DECL_OVERRIDE;
    void OnViewChanged() DECL_OVERRIDE;
    void OnScrollChanged() DECL_OVERRIDE;

    void CallWithScroll(PointFCallBack callBack);
    void SetScrollBarState() DECL_OVERRIDE;
    QPointF GetScroll() DECL_OVERRIDE;
    void SetScroll(QPointF pos) DECL_OVERRIDE;
    bool SaveScroll() DECL_OVERRIDE;
    bool RestoreScroll() DECL_OVERRIDE;
    bool SaveZoom() DECL_OVERRIDE;
    bool RestoreZoom() DECL_OVERRIDE;

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    void SetFocusToElement(QString xpath);
    void FireClickEvent(QString xpath, QPoint pos);
    void SetTextValue(QString xpath, QString text);
    void AssignInspector();
    void UpdateIcon(const QUrl &iconUrl);

    void HandleWindowClose();
    void HandleJavascriptConsoleMessage(int, const QString&);
    void HandleFeaturePermission(const QUrl&, int);
    void HandleRenderProcessTermination(int, int);
    void HandleFullScreen(bool);

    void Copy() DECL_OVERRIDE;
    void Cut() DECL_OVERRIDE;
    void Paste() DECL_OVERRIDE;
    void Undo() DECL_OVERRIDE;
    void Redo() DECL_OVERRIDE;
    void SelectAll() DECL_OVERRIDE;
    void Unselect() DECL_OVERRIDE;
    void Reload() DECL_OVERRIDE;
    void ReloadAndBypassCache() DECL_OVERRIDE;
    void Stop() DECL_OVERRIDE;
    void StopAndUnselect() DECL_OVERRIDE;
    void Print() DECL_OVERRIDE;
    void Save() DECL_OVERRIDE;
    void ZoomIn() DECL_OVERRIDE;
    void ZoomOut() DECL_OVERRIDE;

    void ExitFullScreen() DECL_OVERRIDE;
    void InspectElement() DECL_OVERRIDE;
    void AddSearchEngine(QPoint pos) DECL_OVERRIDE;
    void AddBookmarklet(QPoint pos) DECL_OVERRIDE;

    // dirty...
    int findBackwardIntValue()            { return static_cast<int>(FindBackward);}
    int caseSensitivelyIntValue()         { return static_cast<int>(CaseSensitively);}
    int wrapsAroundDocumentIntValue()     { return static_cast<int>(WrapsAroundDocument);}
    int highlightAllOccurrencesIntValue() { return static_cast<int>(HighlightAllOccurrences);}

    // for qml object.
    void triggerAction(QString str){ View::TriggerAction(str);}

    QString setFocusToElementJsCode(const QString &xpath){ return SetFocusToElementJsCode(xpath);}
    QString fireClickEventJsCode(const QString &xpath, const QPoint &pos){ return FireClickEventJsCode(xpath, pos);}
    QString setTextValueJsCode(const QString &xpath, const QString &text){ return SetTextValueJsCode(xpath, text);}
    QString getScrollValuePointJsCode(){ return GetScrollValuePointJsCode();}
    QString setScrollValuePointJsCode(const QPoint &pos){ return SetScrollValuePointJsCode(pos);}
    QString getScrollRatioPointJsCode(){ return GetScrollRatioPointJsCode();}
    QString setScrollRatioPointJsCode(const QPointF &pos){ return SetScrollRatioPointJsCode(pos);}

    bool enableLoadHack(){
        return m_EnableLoadHackLocal;
    }

    void openInNewViewNode(QUrl url, bool changefocus){
        GetTreeBank()->OpenInNewViewNode(QNetworkRequest(url), changefocus, GetViewNode());
    }
    void openInNewHistNode(QUrl url, bool changefocus){
        GetTreeBank()->OpenInNewHistNode(QNetworkRequest(url), changefocus, GetHistNode());
    }

    QQuickItem *newViewNodeForeground(){
        SharedView view = GetTreeBank()->OpenInNewViewNode(QUrl(), true, GetViewNode());
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return 0;
    }
    QQuickItem *newViewNodeBackground(){
        SharedView view = GetTreeBank()->OpenInNewViewNode(QUrl(), false, GetViewNode());
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return 0;
    }
    QQuickItem *newHistNodeForeground(){
        SharedView view = GetTreeBank()->OpenInNewHistNode(QUrl(), true, GetHistNode());
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return 0;
    }
    QQuickItem *newHistNodeBackground(){
        SharedView view = GetTreeBank()->OpenInNewHistNode(QUrl(), false, GetHistNode());
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return 0;
    }

    // save to or restore from 'm_HistNode'.
    void saveScrollToNode(QPoint pos){
        if(!GetHistNode()) return;
        GetHistNode()->SetScrollX(pos.x());
        GetHistNode()->SetScrollY(pos.y());
    }
    QPoint restoreScrollFromNode(){
        if(!GetHistNode()) return QPoint();
        return QPoint(GetHistNode()->GetScrollX(),
                      GetHistNode()->GetScrollY());
    }
    void saveZoomToNode(float zoom){
        if(!GetHistNode()) return;
        GetHistNode()->SetZoom(zoom);
    }
    float restoreZoomFromNode(){
        if(!GetHistNode()) return 0.0f;
        return GetHistNode()->GetZoom();
    }

signals:
    void CallBackResult(int, QVariant);
    void ViewChanged();
    void ScrollChanged(QPointF);

    void titleChanged(const QString&);
    void urlChanged(const QUrl&);
    void iconChanged(const QIcon&);
    void iconUrlChanged(const QUrl&);
    void loadStarted();
    void loadProgress(int);
    void loadFinished(bool);
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);
    void windowCloseRequested();
    void javascriptConsoleMessage(int, const QString&);
    void featurePermissionRequested(const QUrl&, int);
    void renderProcessTerminated(int, int);
    void fullScreenRequested(bool);

protected:
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *ev) DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *ev) DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *ev) DECL_OVERRIDE;
    void dropEvent(QDropEvent *ev) DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) DECL_OVERRIDE;

private:
    QQuickItem *m_QmlWebEngineView;
    QIcon m_Icon;
    QImage m_GrabedDisplayData;
    static QMap<View*, QUrl> m_InspectorTable;
    QWebEngineView *m_Inspector;
    bool m_PreventScrollRestoration;
#ifdef PASSWORD_MANAGER
    bool m_PreventAuthRegistration;
#endif

    QMap<Page::CustomAction, QAction*> m_ActionTable;

    int m_RequestId;

    inline void SetPreference(QWebEngineSettings::WebAttribute item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebEngineView, "setPreference",
             Q_ARG(QVariant, QVariant::fromValue(QString(QLatin1String(text)))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->testAttribute(item))));
    }

    inline void SetFontFamily(QWebEngineSettings::FontFamily item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebEngineView, "setFontFamily",
             Q_ARG(QVariant, QVariant::fromValue(QString::fromUtf8(text))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->fontFamily(item))));
    }

    inline void SetFontSize(QWebEngineSettings::FontSize item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebEngineView, "setFontSize",
             Q_ARG(QVariant, QVariant::fromValue(QString::fromUtf8(text))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->fontSize(item))));
    }
};

#endif //ifndef QUICKWEBENGINEVIEW_HPP
