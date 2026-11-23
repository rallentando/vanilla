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
#include <QHash>
#include <QPointer>

#include <functional>

class QWebEngineHistory;
class WebEnginePasswords;

class QuickWebEngineView : public QQuickWidget, public View {
    Q_OBJECT

public:
    QuickWebEngineView(TreeBank *parent = nullptr, QString id = QString(), QStringList set = QStringList());
    ~QuickWebEngineView() override;

    void ApplySpecificSettings(QStringList set) override;
    bool IsOffTheRecord() override;
    QObject *EngineProfile() override;
    void ApplyTheme() override;

    QQuickWidget *base() override;
    WebEnginePage *page() override;

    QUrl url() override;
    QUrl CommittedUrl() override;
    QString html() override;
    TreeBank *parent() override;
    void setUrl(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &url) override;
    void setParent(TreeBank* t) override;

    void Connect(TreeBank *tb) override;
    void Disconnect(TreeBank *tb) override;

    void Suspend();
    void WakeUp();

    bool ForbidToOverlap() override {
        return false;
    }

    bool IsOwnDragSource(QObject *source) const override {
        return source && (source == this ||
                          source == m_QmlWebEngineView ||
                          source == quickWindow());
    }

    QWidget *InspectorPane() override {
        if(!m_Inspector || TreeBank::PurgeView() || !InspectorInMainWindow())
            return nullptr;
        return m_Inspector;
    }

    bool CanGoBack() override {
        return m_QmlWebEngineView->property("canGoBack").toBool();
    }
    bool CanGoForward() override {
        return m_QmlWebEngineView->property("canGoForward").toBool();
    }
    bool RecentlyAudible() override {
        return m_QmlWebEngineView->property("recentlyAudible").toBool();
    }
    bool IsAudioMuted() override {
        return m_QmlWebEngineView->property("audioMuted").toBool();
    }
    void SetAudioMuted(bool muted) override {
        m_QmlWebEngineView->setProperty("audioMuted", muted);
        TreeBank::TabsLookChanged();
    }

    bool IsRenderable() override {
        return status() == QQuickWidget::Ready && (visible() || !m_GrabedDisplayData.isNull());
    }
    QImage CaptureVisible() override {
        if(status() != QQuickWidget::Ready || !visible() || !window() || window()->isMinimized()) return QImage();
        return grabFramebuffer();
    }
    void Render(QPainter *painter) override {
        if(EngineShown()) m_GrabedDisplayData = grabFramebuffer();
        painter->drawImage(QPoint(), m_GrabedDisplayData);
    }
    void Render(QPainter *painter, const QRegion &clip) override {
        if(EngineShown()) m_GrabedDisplayData = grabFramebuffer();
        foreach(QRect rect, clip){
            painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
        }
    }
    QSize GetViewportSize() override {
        return visible() ? size() : m_GrabedDisplayData.size();
    }
    void SetViewportSize(QSize size) override {
        if(!visible()) resize(size);
    }
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
        return m_QmlWebEngineView->property("title").toString();
    }
    QIcon GetIcon() override {
        return m_Icon;
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) override {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) override {
        return page() ? page()->Action(a, data) : nullptr;
    }

    void TriggerNativeLoadAction(const QUrl &url) override {
        setUrl(url);
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) override {
        Q_UNUSED(operation) Q_UNUSED(body)
        setUrl(req.url());
    }
    void TriggerNativeGoBackAction() override { QMetaObject::invokeMethod(m_QmlWebEngineView, "goBack");}
    void TriggerNativeGoForwardAction() override { QMetaObject::invokeMethod(m_QmlWebEngineView, "goForward");}
    void TriggerNativeRewindAction() override { QMetaObject::invokeMethod(m_QmlWebEngineView, "rewind");}
    void TriggerNativeFastForwardAction() override { QMetaObject::invokeMethod(m_QmlWebEngineView, "fastForward");}

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
        m_QmlWebEngineView->setProperty("width", size.width());
        m_QmlWebEngineView->setProperty("height", size.height());
        if(TreeBank::PurgeView() && m_TreeBank && base()->isWidgetType() &&
           static_cast<QWidget*>(base())->isWindow()){
            base()->setGeometry(QRect(m_TreeBank->mapToGlobal(QPoint()), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
    }
    void show() override {
        const bool wasVisible = base()->isVisible();
        WakeUp();
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();

        if(!wasVisible){
            MainWindow *win = Application::GetCurrentWindow();
            QSize s =
                m_TreeBank ? m_TreeBank->ViewSize() :
                win ? win->GetTreeBank()->ViewSize() :
                !size().isEmpty() ? size() :
                DEFAULT_WINDOW_SIZE;
            resize(QSize(s.width(), s.height()+1));
            resize(s);
        }

        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        CallWithScroll([this](QPointF pos){
            if(m_TreeBank){
                if(Notifier *notifier = m_TreeBank->GetNotifier()){
                    notifier->SetScroll(pos);
                }
            }
        });
    }
    void hide()    override { base()->hide(); Suspend();}
    void raise()   override { base()->raise();}
    void lower()   override { base()->lower();}
    void repaint() override {
        QSize s = size();
        if(s.isEmpty()) return;
        resize(QSize(s.width(), s.height()+1));
        resize(s);
    }
    bool visible() override { return base()->isVisible();}
    bool EngineShown() const { return m_QmlWebEngineView && m_QmlWebEngineView->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) override {
        QTimer::singleShot(0, this, [this, reason](){ base()->setFocus(reason);});
    }

    void Load()                           override { View::Load();}
    void Load(const QString &url)         override { View::Load(url);}
    void Load(const QUrl &url)            override { View::Load(url);}
    void Load(const QNetworkRequest &req) override { View::Load(req);}

    void OnBeforeStartingDisplayGadgets() override {}
    void OnAfterFinishingDisplayGadgets() override {
    }

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
    bool PageGeometry(QSizeF *contents, QRectF *viewport) override;
    bool SaveScroll() override;
    bool RestoreScroll() override;
    bool SaveZoom() override;
    bool RestoreZoom() override;
    float MinimumZoom() const override { return ChromiumMinimumZoom / DeviceZoomScale();}
    float MaximumZoom() const override { return ChromiumMaximumZoom / DeviceZoomScale();}
    bool SaveHistory() override;
    bool RestoreHistory() override;
    bool SaveMediaTime(VoidCallBack settled = VoidCallBack()) override;
    bool RestoreMediaTime() override;

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
    void HandleContextMenu(QObject*, bool);
    void HandleJavaScriptDialog(QObject*);
    void HandleDownload(QObject*);
    void HandleContentsSizeChange(const QSizeF &size);
    void HandleScrollPositionChange(const QPointF &pos);

    void Copy() override;
    void Cut() override;
    void Paste() override;

#define VANILLA_EDIT_ACTION(name, qmlName)                              \
    void name() override {                                       \
        QMetaObject::invokeMethod(m_QmlWebEngineView, #qmlName);        \
    }
    FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(VANILLA_EDIT_ACTION)
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
    void InspectElement() override;
    SiteDataLink *OpenSiteDataLink() override;
    void CloseInspector();
    void AddSearchEngine(QPoint pos) override;
    void AddBookmarklet(QPoint pos) override;

    int findBackwardIntValue()            { return static_cast<int>(FindBackward);}
    int caseSensitivelyIntValue()         { return static_cast<int>(CaseSensitively);}

    QString getScrollValuePointJsCode(){ return GetScrollValuePointJsCode();}
    QString setScrollValuePointJsCode(const QPoint &pos){ return SetScrollValuePointJsCode(pos);}
    QString getScrollRatioPointJsCode(){ return GetScrollRatioPointJsCode();}
    QString setScrollRatioPointJsCode(const QPointF &pos){ return SetScrollRatioPointJsCode(pos);}

    QQuickItem *newView(){
        if(TakeOwnDrop()) return nullptr;
        View *view = this;
        if(page()) view = page()->OpenInNew(BLANK_URL);
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return m_QmlWebEngineView;
    }
    QQuickItem *newViewBackground(){
        if(TakeOwnDrop()) return nullptr;
        View *view = this;
        if(page()) view = page()->OpenInNewBackground(BLANK_URL);
        if(QuickWebEngineView *v = qobject_cast<QuickWebEngineView*>(view->base()))
            return v->m_QmlWebEngineView;
        return m_QmlWebEngineView;
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
        GetViewNode()->SetZoom(static_cast<float>(zoom / View::DeviceZoomScale()));
    }
    float restoreZoomFromNode(){
        if(!GetViewNode()) return 0.0f;
        return static_cast<float>(GetViewNode()->GetZoom() * View::DeviceZoomScale());
    }

    qreal engineZoomFactor() const {
        const qreal zoom = m_QmlWebEngineView->property("zoomFactor").toReal();
        return zoom > 0.0 ? zoom : 1.0;
    }

    void suspendSpecificSettingsIfNeed(const QUrl &url);

    void certificateError(QWebEngineCertificateError error);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    void desktopMediaRequested(const QWebEngineDesktopMediaRequest &request);
    void webAuthUxRequested(QWebEngineWebAuthUxRequest *request);
#endif

public:
    static QString DefaultScript(){
        static QString source;
        if(source.isEmpty()){
            QString inner;
            static const QList<QEvent::Type> types =
                QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease;
            inner += View::InstallEventFilterJsCode(types);

            source = QStringLiteral
                ("(function(){\n"
                 "%1\n"
                 "})()").arg(inner);
        }
        return source;
    }
    static QString DefaultFrameScript(){
        static const QString source = QStringLiteral
            ("(function(){\n"
             "if(window.top === window) return;\n"
             "%1\n"
             "})()").arg(DefaultScript());
        return source;
    }

public slots:
    QString defaultScript(){ return DefaultScript(); }

    QString passwordScript();
    void applicationWorldResult(int id, const QVariant &result);

    ExtensionController *Extensions() const override;
    bool DeferExtensionNavigation(const QUrl &url);
    QString ExtensionStatus() const override;
    QWidget *CreateExtensionView(const QUrl &url, ExtensionPage page, QWidget *parent,
                                 const std::function<void()> &closed = std::function<void()>()) override;
signals:
    void ExtensionContextChanged();
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
    void printRequested();
    void renderProcessTerminated(int, int);
    void fullScreenRequested(bool);
    void contextMenuRequested(QObject*, bool);
    void javaScriptDialogRequested(QObject*);
    void downloadRequested(QObject*);
    void contentsSizeChanged(const QSizeF &size);
    void scrollPositionChanged(const QPointF &pos);
    void recentlyAudibleChanged(bool);

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
    ButtonHooks Buttons();
    void dragEnterEvent(QDragEnterEvent *ev) override;
    void dragMoveEvent(QDragMoveEvent *ev) override;
    void dropEvent(QDropEvent *ev) override;
    void dragLeaveEvent(QDragLeaveEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    bool focusNextPrevChild(bool next) override;

private:
    QPoint m_ScriptMediaPoint = QPoint(-1, -1);
    quint64 m_ScriptMediaLoad = 0;
    QPoint ScriptMediaPoint() const {
        return m_ScriptMediaLoad == LoadSerial() ? m_ScriptMediaPoint : QPoint(-1, -1);
    }
    virtual void ForwardMouseReleaseToEngine(QMouseEvent *ev);

    QQuickItem *m_QmlWebEngineView;
    QIcon m_Icon;
    QImage m_GrabedDisplayData;
    QColor m_BaseBackgroundColor;
    QQuickWidget *m_Inspector;
    int m_ScrollSignalTimer;
    int m_MediaTimeSaveTimer;
    bool m_PreventScrollRestoration;
    bool m_SuspendedSpecificSettings;

    QWebEngineHistory *history();

    QMap<Page::CustomAction, QAction*> m_ActionTable;

    int m_RequestId;

#ifdef VANILLA_QUICK_PASSWORDS
    QPointer<WebEnginePasswords> m_Passwords;
    int m_ApplicationWorldSerial;
    QHash<int, std::function<void(const QVariant&)>> m_ApplicationWorldWaiting;
    void InstallPasswords();
#endif

    inline void SetPreference(QWebEngineSettings::WebAttribute item, const char *text){
        QMetaObject::invokeMethod
            (m_QmlWebEngineView, "setPreference",
             Q_ARG(QVariant, QVariant::fromValue(QString(QLatin1String(text)))),
             Q_ARG(QVariant, QVariant::fromValue(page()->settings()->testAttribute(item))));
    }
};

#endif
#endif
