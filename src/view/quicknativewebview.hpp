#ifndef QUICKNATIVEWEBVIEW_HPP
#define QUICKNATIVEWEBVIEW_HPP

#include "switch.hpp"

#ifdef NATIVEWEBVIEW

#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"
#include "nativehistory.hpp"

#include <QWindow>
#include <QMenuBar>
#include <QQuickItem>
#include <QNetworkRequest>
#include <QQmlEngine>
#include <QQuickView>

class QuickNativeWebView : public QWidget, public View {
    Q_OBJECT

public:
    QuickNativeWebView(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    ~QuickNativeWebView();

    void ApplySpecificSettings(QStringList set) override;

    QWidget *base() override;
    Page *page() override;

    QUrl url() override;
    QString html() override;
    TreeBank *parent() override;
    void setUrl(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &url) override;
    void setParent(TreeBank* t) override;

    void Connect(TreeBank *tb) override;
    void Disconnect(TreeBank *tb) override;

    bool ForbidToOverlap() override {
        return true;
    }

    bool IsOwnDragSource(QObject *source) const override {
        return source && (source == this ||
                          source == m_QmlNativeWebView ||
                          source == m_QuickView);
    }

    bool CanGoBack() override {
        return m_History.CanGoBack() && !m_History.Busy(NativeIsLoading());
    }
    bool CanGoForward() override {
        return m_History.CanGoForward() && !m_History.Busy(NativeIsLoading());
    }
    void GoBackToInferedUrl() override {
        if(m_History.Busy(NativeIsLoading())) return;
        View::GoBackToInferedUrl();
    }
    void GoForwardToInferedUrl() override {
        if(m_History.Busy(NativeIsLoading())) return;
        View::GoForwardToInferedUrl();
    }

    bool IsRenderable() override {
        return m_QuickView->status() == QQuickView::Ready && (visible() || !m_GrabedDisplayData.isNull());
    }
    void Render(QPainter *painter) override {
        if(visible()) m_GrabedDisplayData = GrabView();
        painter->drawImage(QPoint(), m_GrabedDisplayData);
    }
    void Render(QPainter *painter, const QRegion &clip) override {
        if(visible()) m_GrabedDisplayData = GrabView();
        foreach(QRect rect, clip.rects()){
            painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        }
    }
    QImage GrabView();
    QSize GetViewportSize() override {
        return visible() ? size() : m_GrabedDisplayData.deviceIndependentSize().toSize();
    }
    void SetViewportSize(QSize) override {}
    void SetSource(const QUrl &url) override {
        if(page()) page()->SetSource(url);
    }
    void SetSource(const QByteArray &html) override {
        if(page()) page()->SetSource(html);
    }
    void SetSource(const QString &html) override {
        if(page()) page()->SetSource(html);
    }
    QString GetTitle() override {
        return m_QmlNativeWebView->property("title").toString();
    }
    QIcon GetIcon() override {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) override {
        Action(a, data)->trigger();
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) override {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) override {
        AskForLoadOf(url);
        emit urlChanged(url);
        m_QmlNativeWebView->setProperty("url", url);
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) override {
        Q_UNUSED(operation) Q_UNUSED(body)
        AskForLoadOf(req.url());
        emit urlChanged(req.url());
        m_QmlNativeWebView->setProperty("url", req.url());
    }
    void TriggerNativeGoBackAction() override;
    void TriggerNativeGoForwardAction() override;
    void TriggerNativeRewindAction() override;
    void TriggerNativeFastForwardAction() override;

    void UpKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(UpKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void DownKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(DownKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void RightKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(RightKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void LeftKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(LeftKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void PageDownKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(PageDownKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void PageUpKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(PageUpKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void HomeKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(HomeKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void EndKeyEvent() override {
        CallWithEvaluatedJavaScriptResult(EndKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }

    void KeyPressEvent(QKeyEvent *ev) override { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) override { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) override { mousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) override { mouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) override { mouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) override { mouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) override { wheelEvent(ev);}

    void CallWithGotBaseUrl(UrlCallBack) override;
    void CallWithGotCurrentBaseUrl(UrlCallBack) override;
    void CallWithFoundElements(Page::FindElementsOption, WebElementListCallBack) override;
    void CallWithHitElement(const QPoint&, WebElementCallBack) override;
    void CallWithHitLinkUrl(const QPoint&, UrlCallBack) override;
    void CallWithHitImageUrl(const QPoint&, UrlCallBack) override;
    void CallWithSelectedText(StringCallBack) override;
    void CallWithSelectedHtml(StringCallBack) override;
    void CallWithWholeText(StringCallBack) override;
    void CallWithWholeHtml(StringCallBack) override;
    void CallWithSelectionRegion(RegionCallBack) override;
    void CallWithEvaluatedJavaScriptResult(const QString&, VariantCallBack) override;

public slots:
    QSize size() override { return base()->size();}
    void resize(QSize size) override {
        m_QmlNativeWebView->setProperty("width", size.width());
        m_QmlNativeWebView->setProperty("height", size.height());
        if(TreeBank::PurgeView() && m_TreeBank && base()->isWidgetType() &&
           static_cast<QWidget*>(base())->isWindow()){
            base()->setGeometry(QRect(m_TreeBank->mapToGlobal(QPoint()), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
        m_Container->resize(size);
    }
    void show() override {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();

        MainWindow *win = Application::GetCurrentWindow();
        QSize s =
            m_TreeBank ? m_TreeBank->ViewSize() :
            win ? win->GetTreeBank()->ViewSize() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;
        resize(QSize(s.width(), s.height()+1));
        resize(s);

        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        CallWithScroll([this](QPointF pos){
            if(m_TreeBank){
                if(Notifier *notifier = m_TreeBank->GetNotifier()){
                    notifier->SetScroll(pos);
                }
            }
        });
    }
    void hide()    override { base()->hide();}
    void raise()   override { base()->raise();}
    void lower()   override { base()->lower();}
    void repaint() override {
        QSize s = size();
        if(s.isEmpty()) return;
        resize(QSize(s.width(), s.height()+1));
        resize(s);
    }
    bool visible() override { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) override {
        QTimer::singleShot(0, this, [this, reason](){ base()->setFocus(reason);});
    }

    void Load()                           override { View::Load();}
    void Load(const QString &url)         override { View::Load(url);}
    void Load(const QUrl &url)            override { View::Load(url);}
    void Load(const QNetworkRequest &req) override { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() override {
        if(visible()) m_GrabedDisplayData = GrabView();
        hide();
    }
    void OnAfterFinishingDisplayGadgets() override { show();}

    void OnSetViewNode(ViewNode*) override;
    void OnSetThis(WeakView) override;
    void OnSetMaster(WeakView) override;
    void OnSetSlave(WeakView) override;
    void OnLoadStarted() override;
    void OnLoadProgress(int) override;
    void OnLoadFinished(bool) override;

    void loadStopped();

    void OnTitleChanged(const QString&) override;
    void OnUrlChanged(const QUrl&) override;
    void OnViewChanged() override;
    void OnScrollChanged() override;

    void EmitScrollChanged() override;

private:
    bool EndLoad(bool ok);

public slots:

    void CallWithScroll(PointFCallBack callBack);
    void SetScrollBarState() override;
    QPointF GetScroll() override;
    void SetScroll(QPointF pos) override;
    bool SaveScroll() override;
    bool RestoreScroll() override;
    bool SaveZoom() override;
    bool RestoreZoom() override;
    bool SaveHistory() override;
    bool RestoreHistory() override;
#ifdef MEDIATIME
    bool SaveMediaTime(VoidCallBack settled = VoidCallBack()) override;
    bool RestoreMediaTime() override;
#endif

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    void SetFocusToElement(QString xpath);
    void FireClickEvent(QString xpath, QPoint pos);
    void SetTextValue(QString xpath, QString text);
    void UpdateIcon(const QUrl &iconUrl);

    void HandleWindowClose();
    void HandleJavascriptConsoleMessage(int, const QString&);
    void HandleFeaturePermission(const QUrl&, int);
    void HandleRenderProcessTermination(int, int);
    void HandleFullScreen(bool);
    void HandleDownload(QObject*);

    void Copy() override;
    void Cut() override;
    void Paste() override;

#define VANILLA_EDIT_ACTION(name) void name() override;
    FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

    void Undo() override;
    void Redo() override;
    void SelectAll() override;
    void Unselect() override;
    void Reload() override;
    void ReloadAndBypassCache() override;
    void Stop() override;
    void StopAndUnselect() override;
    void Print() override;
    void Save() override;
    void ZoomIn() override;
    void ZoomOut() override;

    void ToggleMediaControls() override;
    void ToggleMediaLoop() override;
    void ToggleMediaPlayPause() override;
    void ToggleMediaMute() override;

    void ExitFullScreen() override;
    void AddSearchEngine(QPoint pos) override;
    void AddBookmarklet(QPoint pos) override;

    int findBackwardIntValue()            { return static_cast<int>(FindBackward);}
    int caseSensitivelyIntValue()         { return static_cast<int>(CaseSensitively);}

    QString getScrollValuePointJsCode(){ return GetScrollValuePointJsCode();}
    QString setScrollValuePointJsCode(const QPoint &pos){ return SetScrollValuePointJsCode(pos);}
    QString getScrollRatioPointJsCode(){ return GetScrollRatioPointJsCode();}
    QString setScrollRatioPointJsCode(const QPointF &pos){ return SetScrollRatioPointJsCode(pos);}

    QQuickItem *newView(){
        View *view = this;
        if(page()) view = page()->OpenInNew(BLANK_URL);
        if(QuickNativeWebView *v = qobject_cast<QuickNativeWebView*>(view->base()))
            return v->m_QmlNativeWebView;
        return m_QmlNativeWebView;
    }

    void saveScrollToNode(QPoint pos){
        if(!GetViewNode()) return;
        GetViewNode()->SetScrollX(pos.x());
        GetViewNode()->SetScrollY(pos.y());
    }
    QPoint restoreScrollFromNode(){
        if(!GetViewNode()) return QPoint();
        return QPoint(GetViewNode()->GetScrollX(),
                      GetViewNode()->GetScrollY());
    }
    void saveZoomToNode(float zoom){
        if(!GetViewNode()) return;
        GetViewNode()->SetZoom(zoom);
    }
    float restoreZoomFromNode(){
        if(!GetViewNode()) return 0.0f;
        return GetViewNode()->GetZoom();
    }

signals:
    void CallBackResult(int, QVariant);
    void ViewChanged();
    void ScrollChanged(QPointF);
    void FindCount(int active, int total);

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

protected:
    void timerEvent(QTimerEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;

    void mouseMoveEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void dragEnterEvent(QDragEnterEvent *ev) override;
    void dragMoveEvent(QDragMoveEvent *ev) override;
    void dropEvent(QDropEvent *ev) override;
    void dragLeaveEvent(QDragLeaveEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    bool focusNextPrevChild(bool next) override;

private:
    QQuickView *m_QuickView;
    QWidget *m_Container;
    QQuickItem *m_QmlNativeWebView;
    NativeHistory m_History;
    QIcon m_Icon;
    QImage m_GrabedDisplayData;
    QStringList m_SpecificSet;
    int m_ScrollSignalTimer;
#ifdef MEDIATIME
    int m_MediaTimePollTimer;
#endif
    bool m_PreventScrollRestoration;
    bool m_EverShown;
    bool m_LoadedWhileHidden;

    QuickNativeLoadTail m_LoadTail;

    void AskForLoadOf(const QUrl &target){
        if(NativeHistory::StartsALoad(target, url())) ReleaseTail();
    }

    void ReleaseTail(){
        if(m_LoadTail.Take(m_History.Pending() != NativeHistory::NoMove))
            m_History.Release();
    }

    bool NativeIsLoading() const {
        return m_QmlNativeWebView &&
               m_QmlNativeWebView->property("loading").toBool();
    }

    void ConnectQmlSignals();
    void RebuildQmlItem();
    void ApplyUserAgent();
    void ApplyPageSettings();

    void PerformHistoryMove(const NativeHistory::Request &request);

    QMap<Page::CustomAction, QAction*> m_ActionTable;

    int m_RequestId;
};

#endif
#endif
