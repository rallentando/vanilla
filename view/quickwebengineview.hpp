#ifndef QUICKWEBENGINEVIEW_HPP
#define QUICKWEBENGINEVIEW_HPP

#include "switch.hpp"

#ifdef WEBENGINEVIEW

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

    void ApplySpecificSettings(QStringList set) Q_DECL_OVERRIDE;

    QQuickWidget *base() Q_DECL_OVERRIDE;
    WebEnginePage *page() Q_DECL_OVERRIDE;

    QUrl url() Q_DECL_OVERRIDE;
    QString html() Q_DECL_OVERRIDE;
    TreeBank *parent() Q_DECL_OVERRIDE;
    void setUrl(const QUrl &url) Q_DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) Q_DECL_OVERRIDE;
    void setParent(TreeBank* t) Q_DECL_OVERRIDE;

    void Connect(TreeBank *tb) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) Q_DECL_OVERRIDE;

    bool ForbidToOverlap() Q_DECL_OVERRIDE {
        return false;
    }

    bool CanGoBack() Q_DECL_OVERRIDE {
        return m_QmlWebEngineView->property("canGoBack").toBool();
    }
    bool CanGoForward() Q_DECL_OVERRIDE {
        return m_QmlWebEngineView->property("canGoForward").toBool();
    }
#if QT_VERSION >= 0x050700
    bool RecentlyAudible() Q_DECL_OVERRIDE {
        return m_QmlWebEngineView->property("recentlyAudible").toBool();
    }
    bool IsAudioMuted() Q_DECL_OVERRIDE {
        return m_QmlWebEngineView->property("audioMuted").toBool();
    }
    void SetAudioMuted(bool muted) Q_DECL_OVERRIDE {
        m_QmlWebEngineView->setProperty("audioMuted", muted);
    }
#endif

    bool IsRenderable() Q_DECL_OVERRIDE {
        return status() == QQuickWidget::Ready && (visible() || !m_GrabedDisplayData.isNull());
    }
    void Render(QPainter *painter) Q_DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = grabFramebuffer();
        painter->drawImage(QPoint(), m_GrabedDisplayData);
    }
    void Render(QPainter *painter, const QRegion &clip) Q_DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = grabFramebuffer();
        foreach(QRect rect, clip.rects()){
            painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        }
    }
    // if this is hidden(or minimized), cannot render.
    // using display cache, instead of real time rendering.
    QSize GetViewportSize() Q_DECL_OVERRIDE {
        return visible() ? size() : m_GrabedDisplayData.size();
    }
    void SetViewportSize(QSize size) Q_DECL_OVERRIDE {
        if(!visible()) resize(size);
    }
    void SetSource(const QUrl &url) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(url);
    }
    void SetSource(const QByteArray &html){
        if(page()) page()->SetSource(html);
    }
    void SetSource(const QString &html){
        if(page()) page()->SetSource(html);
    }
    QString GetTitle() Q_DECL_OVERRIDE {
        return m_QmlWebEngineView->property("title").toString();
    }
    QIcon GetIcon() Q_DECL_OVERRIDE {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) Q_DECL_OVERRIDE { m_QmlWebEngineView->setProperty("url", url);}
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray & = QByteArray()) Q_DECL_OVERRIDE { m_QmlWebEngineView->setProperty("url", req.url());}
    void TriggerNativeGoBackAction() Q_DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebEngineView, "goBack");}
    void TriggerNativeGoForwardAction() Q_DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebEngineView, "goForward");}
    void TriggerNativeRewindAction() Q_DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebEngineView, "rewind");}
    void TriggerNativeFastForwardAction() Q_DECL_OVERRIDE { QMetaObject::invokeMethod(m_QmlWebEngineView, "fastForward");}

    void UpKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollTop-=40;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void DownKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollTop+=40;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void RightKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollLeft+=40;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void LeftKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollLeft-=40;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void PageDownKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollTop+=document.documentElement.clientHeight*0.9;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void PageUpKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollTop-=document.documentElement.clientHeight*0.9;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void HomeKeyEvent() Q_DECL_OVERRIDE {
        qDebug() << "home";
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){ document.body.scrollTop=0;})()"),
             [this](QVariant){ EmitScrollChanged();});
    }
    void EndKeyEvent() Q_DECL_OVERRIDE {
        qDebug() << "end";
        CallWithEvaluatedJavaScriptResult
            (QStringLiteral("(function(){\n"
                          VV"    document.body.scrollTop = \n"
                          VV"        (document.documentElement.scrollHeight - \n"
                          VV"         document.documentElement.clientHeight);\n"
                          VV"})()"),
             [this](QVariant){ EmitScrollChanged();});
    }

    void KeyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE { mouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE { wheelEvent(ev);}

    void CallWithGotBaseUrl(UrlCallBack) Q_DECL_OVERRIDE;
    void CallWithGotCurrentBaseUrl(UrlCallBack) Q_DECL_OVERRIDE;
    void CallWithFoundElements(Page::FindElementsOption, WebElementListCallBack) Q_DECL_OVERRIDE;
    void CallWithHitElement(const QPoint&, WebElementCallBack) Q_DECL_OVERRIDE;
    void CallWithHitLinkUrl(const QPoint&, UrlCallBack) Q_DECL_OVERRIDE;
    void CallWithHitImageUrl(const QPoint&, UrlCallBack) Q_DECL_OVERRIDE;
    void CallWithSelectedText(StringCallBack) Q_DECL_OVERRIDE;
    void CallWithSelectedHtml(StringCallBack) Q_DECL_OVERRIDE;
    void CallWithWholeText(StringCallBack) Q_DECL_OVERRIDE;
    void CallWithWholeHtml(StringCallBack) Q_DECL_OVERRIDE;
    void CallWithSelectionRegion(RegionCallBack) Q_DECL_OVERRIDE;
    void CallWithEvaluatedJavaScriptResult(const QString&, VariantCallBack) Q_DECL_OVERRIDE;

public slots:
    QSize size() Q_DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) Q_DECL_OVERRIDE {
        m_QmlWebEngineView->setProperty("width", size.width());
        m_QmlWebEngineView->setProperty("height", size.height());
        base()->setGeometry(QRect(QPoint(), size));
    }
    void show() Q_DECL_OVERRIDE {
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
    void hide()    Q_DECL_OVERRIDE { base()->hide();}
    void raise()   Q_DECL_OVERRIDE { base()->raise();}
    void lower()   Q_DECL_OVERRIDE { base()->lower();}
    void repaint() Q_DECL_OVERRIDE {
        QSize s = size();
        if(s.isEmpty()) return;
        resize(QSize(s.width(), s.height()+1));
        resize(s);
    }
    bool visible() Q_DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {
        QTimer::singleShot(0, this, [this, reason](){ base()->setFocus(reason);});
    }

    void Load()                           Q_DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) Q_DECL_OVERRIDE { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() Q_DECL_OVERRIDE {}
    void OnAfterFinishingDisplayGadgets() Q_DECL_OVERRIDE {
#if defined(Q_OS_WIN)
        if(TreeBank::TridentViewExist()) show(); // for force update.
#endif
    }

    void OnSetViewNode(ViewNode*) Q_DECL_OVERRIDE;
    void OnSetHistNode(HistNode*) Q_DECL_OVERRIDE;
    void OnSetThis(WeakView) Q_DECL_OVERRIDE;
    void OnSetMaster(WeakView) Q_DECL_OVERRIDE;
    void OnSetSlave(WeakView) Q_DECL_OVERRIDE;
    void OnSetJsObject(_View*) Q_DECL_OVERRIDE;
    void OnSetJsObject(_Vanilla*) Q_DECL_OVERRIDE;
    void OnLoadStarted() Q_DECL_OVERRIDE;
    void OnLoadProgress(int) Q_DECL_OVERRIDE;
    void OnLoadFinished(bool) Q_DECL_OVERRIDE;
    void OnTitleChanged(const QString&) Q_DECL_OVERRIDE;
    void OnUrlChanged(const QUrl&) Q_DECL_OVERRIDE;
    void OnViewChanged() Q_DECL_OVERRIDE;
    void OnScrollChanged() Q_DECL_OVERRIDE;

    void EmitScrollChanged() Q_DECL_OVERRIDE;

    void CallWithScroll(PointFCallBack callBack);
    void SetScrollBarState() Q_DECL_OVERRIDE;
    QPointF GetScroll() Q_DECL_OVERRIDE;
    void SetScroll(QPointF pos) Q_DECL_OVERRIDE;
    bool SaveScroll() Q_DECL_OVERRIDE;
    bool RestoreScroll() Q_DECL_OVERRIDE;
    bool SaveZoom() Q_DECL_OVERRIDE;
    bool RestoreZoom() Q_DECL_OVERRIDE;

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
    void HandleDownload(QObject*);
#if QT_VERSION >= 0x050700
    void HandleContentsSizeChange(const QSizeF &size);
    void HandleScrollPositionChange(const QPointF &pos);
#endif

    void Copy() Q_DECL_OVERRIDE;
    void Cut() Q_DECL_OVERRIDE;
    void Paste() Q_DECL_OVERRIDE;
    void Undo() Q_DECL_OVERRIDE;
    void Redo() Q_DECL_OVERRIDE;
    void SelectAll() Q_DECL_OVERRIDE;
    void Unselect() Q_DECL_OVERRIDE;
    void Reload() Q_DECL_OVERRIDE;
    void ReloadAndBypassCache() Q_DECL_OVERRIDE;
    void Stop() Q_DECL_OVERRIDE;
    void StopAndUnselect() Q_DECL_OVERRIDE;
    void Print() Q_DECL_OVERRIDE;
    void Save() Q_DECL_OVERRIDE;
    void ZoomIn() Q_DECL_OVERRIDE;
    void ZoomOut() Q_DECL_OVERRIDE;

    void ExitFullScreen() Q_DECL_OVERRIDE;
    void InspectElement() Q_DECL_OVERRIDE;
    void AddSearchEngine(QPoint pos) Q_DECL_OVERRIDE;
    void AddBookmarklet(QPoint pos) Q_DECL_OVERRIDE;

    // for qml object.
    int findBackwardIntValue()            { return static_cast<int>(FindBackward);}
    int caseSensitivelyIntValue()         { return static_cast<int>(CaseSensitively);}
    int wrapsAroundDocumentIntValue()     { return static_cast<int>(WrapsAroundDocument);}
    int highlightAllOccurrencesIntValue() { return static_cast<int>(HighlightAllOccurrences);}

    QString getScrollValuePointJsCode(){ return GetScrollValuePointJsCode();}
    QString setScrollValuePointJsCode(const QPoint &pos){ return SetScrollValuePointJsCode(pos);}
    QString getScrollRatioPointJsCode(){ return GetScrollRatioPointJsCode();}
    QString setScrollRatioPointJsCode(const QPointF &pos){ return SetScrollRatioPointJsCode(pos);}

    QQuickItem *newView(){
        View *view = this;
        if(page()) view = page()->OpenInNew(QUrl(QStringLiteral("about:blank")));
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return m_QmlWebEngineView;
    }
    QQuickItem *newViewBackground(){
        View *view = this;
        if(page()) view = page()->OpenInNewBackground(QUrl(QStringLiteral("about:blank")));
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return m_QmlWebEngineView;
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
    void downloadRequested(QObject*);
#if QT_VERSION >= 0x050700
    void contentsSizeChanged(const QSizeF &size);
    void scrollPositionChanged(const QPointF &pos);
#endif

protected:
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *ev) Q_DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *ev) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *ev) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *ev) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *ev) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) Q_DECL_OVERRIDE;

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

#endif //ifdef WEBENGINEVIEW
#endif //ifndef QUICKWEBENGINEVIEW_HPP
