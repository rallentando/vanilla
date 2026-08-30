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

    void ApplySpecificSettings(QStringList set) Q_DECL_OVERRIDE;

    QWidget *base() Q_DECL_OVERRIDE;
    Page *page() Q_DECL_OVERRIDE;

    QUrl url() Q_DECL_OVERRIDE;
    QString html() Q_DECL_OVERRIDE;
    TreeBank *parent() Q_DECL_OVERRIDE;
    void setUrl(const QUrl &url) Q_DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) Q_DECL_OVERRIDE;
    void setParent(TreeBank* t) Q_DECL_OVERRIDE;

    void Connect(TreeBank *tb) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) Q_DECL_OVERRIDE;

    bool ForbidToOverlap() Q_DECL_OVERRIDE {
        return true;
    }

    bool IsOwnDragSource(QObject *source) const Q_DECL_OVERRIDE {
        return source && (source == this ||
                          source == m_QmlNativeWebView ||
                          source == m_QuickView);
    }

    bool CanGoBack() Q_DECL_OVERRIDE {
        return m_History.CanGoBack() && !m_History.Busy(NativeIsLoading());
    }
    bool CanGoForward() Q_DECL_OVERRIDE {
        return m_History.CanGoForward() && !m_History.Busy(NativeIsLoading());
    }
    void GoBackToInferedUrl() Q_DECL_OVERRIDE {
        if(m_History.Busy(NativeIsLoading())) return;
        View::GoBackToInferedUrl();
    }
    void GoForwardToInferedUrl() Q_DECL_OVERRIDE {
        if(m_History.Busy(NativeIsLoading())) return;
        View::GoForwardToInferedUrl();
    }

    bool IsRenderable() Q_DECL_OVERRIDE {
        return m_QuickView->status() == QQuickView::Ready && (visible() || !m_GrabedDisplayData.isNull());
    }
    void Render(QPainter *painter) Q_DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = GrabView();
        painter->drawImage(QPoint(), m_GrabedDisplayData);
    }
    void Render(QPainter *painter, const QRegion &clip) Q_DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = GrabView();
        foreach(QRect rect, clip.rects()){
            painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        }
    }
    QImage GrabView();
    QSize GetViewportSize() Q_DECL_OVERRIDE {
        return visible() ? size() : m_GrabedDisplayData.deviceIndependentSize().toSize();
    }
    void SetViewportSize(QSize) Q_DECL_OVERRIDE {}
    void SetSource(const QUrl &url) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(url);
    }
    void SetSource(const QByteArray &html) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(html);
    }
    void SetSource(const QString &html) Q_DECL_OVERRIDE {
        if(page()) page()->SetSource(html);
    }
    QString GetTitle() Q_DECL_OVERRIDE {
        return m_QmlNativeWebView->property("title").toString();
    }
    QIcon GetIcon() Q_DECL_OVERRIDE {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        Action(a, data)->trigger();
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) Q_DECL_OVERRIDE {
        AskForLoadOf(url);
        emit urlChanged(url);
        m_QmlNativeWebView->setProperty("url", url);
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) Q_DECL_OVERRIDE {
        Q_UNUSED(operation) Q_UNUSED(body)
        AskForLoadOf(req.url());
        emit urlChanged(req.url());
        m_QmlNativeWebView->setProperty("url", req.url());
    }
    void TriggerNativeGoBackAction() Q_DECL_OVERRIDE;
    void TriggerNativeGoForwardAction() Q_DECL_OVERRIDE;
    void TriggerNativeRewindAction() Q_DECL_OVERRIDE;
    void TriggerNativeFastForwardAction() Q_DECL_OVERRIDE;

    void UpKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(UpKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void DownKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(DownKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void RightKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(RightKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void LeftKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(LeftKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void PageDownKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(PageDownKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void PageUpKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(PageUpKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void HomeKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(HomeKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
    }
    void EndKeyEvent() Q_DECL_OVERRIDE {
        CallWithEvaluatedJavaScriptResult(EndKeyEventJsCode(), [this](QVariant){ EmitScrollChanged();});
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
        m_QmlNativeWebView->setProperty("width", size.width());
        m_QmlNativeWebView->setProperty("height", size.height());
        if(TreeBank::PurgeView() && m_TreeBank){
            base()->setGeometry(QRect(m_TreeBank->mapToGlobal(QPoint()), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
        m_Container->resize(size);
    }
    void show() Q_DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
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

    void OnBeforeStartingDisplayGadgets() Q_DECL_OVERRIDE {
        if(visible()) m_GrabedDisplayData = GrabView();
        hide();
    }
    void OnAfterFinishingDisplayGadgets() Q_DECL_OVERRIDE { show();}

    void OnSetViewNode(ViewNode*) Q_DECL_OVERRIDE;
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
    bool SaveHistory() Q_DECL_OVERRIDE;
    bool RestoreHistory() Q_DECL_OVERRIDE;
#ifdef MEDIATIME
    bool SaveMediaTime() Q_DECL_OVERRIDE;
    bool RestoreMediaTime() Q_DECL_OVERRIDE;
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

    void Copy() Q_DECL_OVERRIDE;
    void Cut() Q_DECL_OVERRIDE;
    void Paste() Q_DECL_OVERRIDE;

#define VANILLA_EDIT_ACTION(name) void name() Q_DECL_OVERRIDE;
    FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

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

    void ToggleMediaControls() Q_DECL_OVERRIDE;
    void ToggleMediaLoop() Q_DECL_OVERRIDE;
    void ToggleMediaPlayPause() Q_DECL_OVERRIDE;
    void ToggleMediaMute() Q_DECL_OVERRIDE;

    void ExitFullScreen() Q_DECL_OVERRIDE;
    void AddSearchEngine(QPoint pos) Q_DECL_OVERRIDE;
    void AddBookmarklet(QPoint pos) Q_DECL_OVERRIDE;

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
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
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

    void AskForLoadOf(const QUrl &target){
        if(NativeHistory::StartsALoad(target, url())) m_History.Release();
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
