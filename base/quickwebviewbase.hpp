#ifndef QUICKWEBVIEWBASE_HPP
#define QUICKWEBVIEWBASE_HPP

#include "switch.hpp"

//[[!QWEV]]
#ifdef QTWEBKIT
//[[/!QWEV]]

#include "webpagebase.hpp"
#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#ifdef USE_QQUICKWIDGET
#  include <QQuickWidget>
#  define QQuickBase QQuickWidget
#else
#  include <QQuickView>
#  define QQuickBase QQuickView
#endif

#include <QWindow>
#include <QMenuBar>
#include <QQuickItem>
#include <QNetworkRequest>
#include <QQmlEngine>

class QuickWebViewBase : public QQuickBase, public View{
    Q_OBJECT

public:
    QuickWebViewBase(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    ~QuickWebViewBase();

    void ApplySpecificSettings(QStringList set) DECL_OVERRIDE;

    QQuickBase *base() DECL_OVERRIDE {
        return static_cast<QQuickBase*>(this);
    }
    WebPageBase *page() DECL_OVERRIDE {
        return static_cast<WebPageBase*>(View::page());
    }

    QUrl      url() DECL_OVERRIDE { return m_QmlWebViewBase->property("url").toUrl();}
    void   setUrl(const QUrl &url) DECL_OVERRIDE {
        m_QmlWebViewBase->setProperty("url", url);
        emit urlChanged(url);
    }

    QString   html() DECL_OVERRIDE { return WholeHtml();}
    void   setHtml(const QString &html, const QUrl &url) DECL_OVERRIDE {
        QMetaObject::invokeMethod(m_QmlWebViewBase, "loadHtml",
                                  Q_ARG(QString, html),
                                  Q_ARG(QUrl,    url));
        emit urlChanged(url);
    }

    TreeBank *parent() DECL_OVERRIDE { return m_TreeBank;}
    void   setParent(TreeBank* t) DECL_OVERRIDE {
        View::SetTreeBank(t);
#ifdef USE_QQUICKWIDGET
        base()->setParent(t);
#else
        base()->setParent(t && t->parentWidget() ? t->parentWidget()->windowHandle() : 0);
#endif
    }

    void Connect(TreeBank *tb) DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) DECL_OVERRIDE;

    //[[QWEV]]
    void ZoomIn() DECL_OVERRIDE;
    void ZoomOut() DECL_OVERRIDE;
    //[[/QWEV]]

    QUrl BaseUrl() DECL_OVERRIDE {
        return GetBaseUrl();
    }
    QUrl CurrentBaseUrl() DECL_OVERRIDE {
        return GetBaseUrl();
    }

    bool ForbidToOverlap() DECL_OVERRIDE { return true;}

    bool CanGoBack() DECL_OVERRIDE {
        return m_QmlWebViewBase->property("canGoBack").toBool();
    }
    bool CanGoForward() DECL_OVERRIDE {
        return m_QmlWebViewBase->property("canGoForward").toBool();
    }

    bool IsRenderable() DECL_OVERRIDE {
        return status() == QQuickBase::Ready && (visible() || !m_GrabedDisplayData.isNull());
    }
    void Render(QPainter *painter) DECL_OVERRIDE {
#ifdef USE_QQUICKWIDGET
        if(visible()) m_GrabedDisplayData = grabFramebuffer();
        painter->drawImage(QPoint(), m_GrabedDisplayData);
#else
        // TODO: cannot grab window? when switching view.
        Q_UNUSED(painter);
        //if(visible()) m_GrabedDisplayData = grabWindow();
        //painter->drawImage(QPoint(), m_GrabedDisplayData);
#endif
    }
    void Render(QPainter *painter, const QRegion &clip) DECL_OVERRIDE {
#ifdef USE_QQUICKWIDGET
        if(visible()) m_GrabedDisplayData = grabFramebuffer();
        foreach(QRect rect, clip.rects()){
            painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        }
#else
        // TODO: cannot grab window? when switching view.
        Q_UNUSED(painter); Q_UNUSED(clip);
        //if(visible()) m_GrabedDisplayData = grabWindow();
        //foreach(QRect rect, clip.rects()){
        //    painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        //}
#endif
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
        return m_QmlWebViewBase->property("title").toString();
    }

    void TriggerAction(QWebPageBase::WebAction a) DECL_OVERRIDE {
        Action(a)->trigger();
    }
    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        Action(a, data)->trigger();
    }
    QAction *Action(QWebPageBase::WebAction a) DECL_OVERRIDE;
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE;

    void TriggerNativeLoadAction(const QUrl &url) DECL_OVERRIDE { m_QmlWebViewBase->setProperty("url", url);}
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray & = QByteArray()) DECL_OVERRIDE { m_QmlWebViewBase->setProperty("url", req.url());}
    void TriggerNativeGoBackAction() DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebViewBase, "goBack");}
    void TriggerNativeGoForwardAction() DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebViewBase, "goForward");}

    void UpKeyEvent()       DECL_OVERRIDE { QQuickBase::keyPressEvent(m_UpKey);       QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void DownKeyEvent()     DECL_OVERRIDE { QQuickBase::keyPressEvent(m_DownKey);     QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void RightKeyEvent()    DECL_OVERRIDE { QQuickBase::keyPressEvent(m_RightKey);    QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void LeftKeyEvent()     DECL_OVERRIDE { QQuickBase::keyPressEvent(m_LeftKey);     QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void PageDownKeyEvent() DECL_OVERRIDE { QQuickBase::keyPressEvent(m_PageDownKey); QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void PageUpKeyEvent()   DECL_OVERRIDE { QQuickBase::keyPressEvent(m_PageUpKey);   QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void HomeKeyEvent()     DECL_OVERRIDE { QQuickBase::keyPressEvent(m_HomeKey);     QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}
    void EndKeyEvent()      DECL_OVERRIDE { QQuickBase::keyPressEvent(m_EndKey);      QMetaObject::invokeMethod(m_QmlWebViewBase, "emitScrollChangedIfNeed");}

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
    void CallWithEvaluatedJavaScriptResult(const QString&, VariantCallBack) DECL_OVERRIDE;

public slots:
    QSize size() DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) DECL_OVERRIDE {
        rootObject()->setProperty("width", size.width());
        rootObject()->setProperty("height", size.height());
        MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : 0;
        if(win && win->IsMenuBarEmpty())
            base()->setGeometry(QRect(QPoint(), size));
        else
            base()->setGeometry(QRect(QPoint(0, win->menuBar()->height()), size));
    }
    void show() DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
        if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();
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
    void hide() DECL_OVERRIDE {
        base()->hide();
    }
    void raise()   DECL_OVERRIDE { base()->raise();}
    void lower()   DECL_OVERRIDE { base()->lower();}
#ifdef USE_QQUICKWIDGET
    void repaint() DECL_OVERRIDE {
        QSize s = size();
        if(s.isEmpty()) return;
        resize(QSize(s.width(), s.height()+1));
        resize(s);
    }
#else
    void repaint() DECL_OVERRIDE { base()->update();}
#endif

    bool visible() DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) DECL_OVERRIDE {
#ifdef USE_QQUICKWIDGET
        base()->setFocus(reason);
#else
        Q_UNUSED(reason);
        requestActivate();
#endif
        m_QmlWebViewBase->setProperty("focus", true);
    }

    void Load()                           DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) DECL_OVERRIDE { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() DECL_OVERRIDE { hide();}
    void OnAfterFinishingDisplayGadgets() DECL_OVERRIDE {
        show();
        // view is not updated, when only call show method.
        // e.g. on deactivate GraphicsTableView.
        MainWindow *win = Application::GetCurrentWindow();
        QSize s =
            m_TreeBank ? m_TreeBank->size() :
            win ? win->GetTreeBank()->size() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;
        resize(QSize(s.width(), s.height()+1));
        resize(s);
    }

    // dummy slots.
    void OnLoadStarted() DECL_OVERRIDE {}
    void OnLoadProgress(int) DECL_OVERRIDE {}
    void OnLoadFinished(bool) DECL_OVERRIDE {}
    void OnTitleChanged(const QString&) DECL_OVERRIDE {}
    void OnUrlChanged(const QUrl&) DECL_OVERRIDE {}

    void OnViewChanged();
    void OnScrollChanged();

    void CallWithScroll(PointFCallBack callBack);

    void SetScroll(QPointF pos) DECL_OVERRIDE {
        QMetaObject::invokeMethod(m_QmlWebViewBase, "setScroll",
                                  Q_ARG(QVariant, QVariant::fromValue(pos)));
    }

    bool SaveScroll() DECL_OVERRIDE {
        if(size().isEmpty()) return false;
        QMetaObject::invokeMethod(m_QmlWebViewBase, "saveScroll");
        return true;
    }
    bool RestoreScroll() DECL_OVERRIDE {
        if(size().isEmpty()) return false;
        QMetaObject::invokeMethod(m_QmlWebViewBase, "restoreScroll");
        return true;
    }
    bool SaveZoom() DECL_OVERRIDE {
        if(size().isEmpty()) return false;
        QMetaObject::invokeMethod(m_QmlWebViewBase, "saveZoom");
        return true;
    }
    bool RestoreZoom() DECL_OVERRIDE {
        if(size().isEmpty()) return false;
        QMetaObject::invokeMethod(m_QmlWebViewBase, "restoreZoom");
        return true;
    }

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    void SetFocusToElement(QString xpath){
        QMetaObject::invokeMethod(m_QmlWebViewBase, "setFocusToElement",
                                  Q_ARG(QVariant, QVariant::fromValue(xpath)));
    }
    void FireClickEvent(QString xpath, QPoint pos){
        QMetaObject::invokeMethod(m_QmlWebViewBase, "fireClickEvent",
                                  Q_ARG(QVariant, QVariant::fromValue(xpath)),
                                  Q_ARG(QVariant, QVariant::fromValue(pos)));
    }

    // dirty...
    int findBackwardIntValue()            { return static_cast<int>(FindBackward);}
    int caseSensitivelyIntValue()         { return static_cast<int>(CaseSensitively);}
    int wrapsAroundDocumentIntValue()     { return static_cast<int>(WrapsAroundDocument);}
    int highlightAllOccurrencesIntValue() { return static_cast<int>(HighlightAllOccurrences);}

    // for qml object.
    void changeNodeTitle(const QString &title){
        ChangeNodeTitle(title);
    }
    void changeNodeUrl(const QUrl &url){
        ChangeNodeUrl(url);
    }

    QString setFocusToElementJsCode(const QString &xpath){ return SetFocusToElementJsCode(xpath);}
    QString fireClickEventJsCode(const QString &xpath, const QPoint &pos){ return FireClickEventJsCode(xpath, pos);}
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
        if(QuickWebViewBase *v = qobject_cast<QuickWebViewBase*>(view->base()))
            return v->m_QmlWebViewBase;
        return 0;
    }
    QQuickItem *newViewNodeBackground(){
        SharedView view = GetTreeBank()->OpenInNewViewNode(QUrl(), false, GetViewNode());
        if(QuickWebViewBase *v = qobject_cast<QuickWebViewBase*>(view->base()))
            return v->m_QmlWebViewBase;
        return 0;
    }
    QQuickItem *newHistNodeForeground(){
        SharedView view = GetTreeBank()->OpenInNewHistNode(QUrl(), true, GetHistNode());
        if(QuickWebViewBase *v = qobject_cast<QuickWebViewBase*>(view->base()))
            return v->m_QmlWebViewBase;
        return 0;
    }
    QQuickItem *newHistNodeBackground(){
        SharedView view = GetTreeBank()->OpenInNewHistNode(QUrl(), false, GetHistNode());
        if(QuickWebViewBase *v = qobject_cast<QuickWebViewBase*>(view->base()))
            return v->m_QmlWebViewBase;
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
    void loadStarted();
    void loadProgress(int);
    void loadFinished(bool);
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);

protected:
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) DECL_OVERRIDE;
  //void contextMenuEvent(QContextMenuEvent *ev) DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE;
  //void dragEnterEvent(QDragEnterEvent *ev) DECL_OVERRIDE;
  //void dragMoveEvent(QDragMoveEvent *ev) DECL_OVERRIDE;
  //void dropEvent(QDropEvent *ev) DECL_OVERRIDE;
  //void dragLeaveEvent(QDragLeaveEvent *ev) DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE;
  //bool focusNextPrevChild(bool next) DECL_OVERRIDE;

private:
    QQuickItem *m_QmlWebViewBase;
    QImage m_GrabedDisplayData;

    QMap<Page::CustomAction, QAction*> m_ActionTable;

    int m_RequestId;

    inline void SetPreference(QWebSettingsBase::WebAttribute item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebViewBase, "setPreference",
             Q_ARG(QVariant, QVariant::fromValue(QString(QLatin1String(text)))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->testAttribute(item))));
    }

    inline void SetFontFamily(QWebSettingsBase::FontFamily item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebViewBase, "setFontFamily",
             Q_ARG(QVariant, QVariant::fromValue(QString::fromUtf8(text))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->fontFamily(item))));
    }

    inline void SetFontSize(QWebSettingsBase::FontSize item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebViewBase, "setFontSize",
             Q_ARG(QVariant, QVariant::fromValue(QString::fromUtf8(text))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->fontSize(item))));
    }
};

//[[!QWEV]]
#endif
//[[/!QWEV]]

#endif
