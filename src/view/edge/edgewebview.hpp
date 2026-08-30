#ifndef EDGEWEBVIEW_HPP
#define EDGEWEBVIEW_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

struct IDataObject;
struct EdgePendingLoad;
class EdgeCookieClearLedger;
class QPainter;
struct ICoreWebView2Controller;
template <class Controller> class EdgeUnadoptedControllerOf;

#include "view.hpp"
#include "nativehistory.hpp"

#include <QWidget>
#include <QImage>
#include <QNetworkRequest>
#include <functional>
#include <memory>

class EdgeWebView : public QWidget, public View {
    Q_OBJECT

public:
    EdgeWebView(TreeBank *parent = 0, QString id = QString(), QStringList set = QStringList());
    ~EdgeWebView();

    void ApplySpecificSettings(QStringList set) Q_DECL_OVERRIDE;

    void ApplyTheme() Q_DECL_OVERRIDE;

    QWidget *base() Q_DECL_OVERRIDE;
    Page *page() Q_DECL_OVERRIDE;

    QUrl url() Q_DECL_OVERRIDE;
    TreeBank *parent() Q_DECL_OVERRIDE;
    void setUrl(const QUrl &url) Q_DECL_OVERRIDE;
    void setParent(TreeBank *t) Q_DECL_OVERRIDE;

    void Connect(TreeBank *tb) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) Q_DECL_OVERRIDE;

    bool ForbidToOverlap() Q_DECL_OVERRIDE { return true;}

    QSize size() Q_DECL_OVERRIDE { return QWidget::size();}
    void resize(QSize size) Q_DECL_OVERRIDE { QWidget::resize(size);}
    void show() Q_DECL_OVERRIDE { QWidget::show(); WakeUp();}
    void hide() Q_DECL_OVERRIDE { QWidget::hide(); Suspend();}
    void raise() Q_DECL_OVERRIDE { QWidget::raise();}
    void lower() Q_DECL_OVERRIDE { QWidget::lower();}
    void repaint() Q_DECL_OVERRIDE { QWidget::repaint();}
    bool visible() Q_DECL_OVERRIDE { return QWidget::isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) Q_DECL_OVERRIDE;
    void TakeKeyboardBack() Q_DECL_OVERRIDE;

    void OnBeforeStartingDisplayGadgets() Q_DECL_OVERRIDE;
    void OnAfterFinishingDisplayGadgets() Q_DECL_OVERRIDE { show();}
    bool IsRenderable() Q_DECL_OVERRIDE;
    void Render(QPainter *painter) Q_DECL_OVERRIDE;
    void Render(QPainter *painter, const QRegion &clip) Q_DECL_OVERRIDE;
    QImage GrabView();

    QSize GetViewportSize() Q_DECL_OVERRIDE;
    void SetViewportSize(QSize) Q_DECL_OVERRIDE {}

    QString GetTitle() Q_DECL_OVERRIDE;
    QIcon GetIcon() Q_DECL_OVERRIDE;

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        if(QAction *action = Action(a, data)) action->trigger();
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) Q_DECL_OVERRIDE {
        Navigate(url);
    }

    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) Q_DECL_OVERRIDE {
        NavigateWithRequest(req, operation, body);
    }
    void TriggerNativeGoBackAction() Q_DECL_OVERRIDE;
    void TriggerNativeGoForwardAction() Q_DECL_OVERRIDE;
    void TriggerNativeRewindAction() Q_DECL_OVERRIDE;
    void TriggerNativeFastForwardAction() Q_DECL_OVERRIDE;

    bool CanGoBack() Q_DECL_OVERRIDE;
    bool CanGoForward() Q_DECL_OVERRIDE;
    void GoBackToInferedUrl() Q_DECL_OVERRIDE;
    void GoForwardToInferedUrl() Q_DECL_OVERRIDE;

    bool SaveHistory() Q_DECL_OVERRIDE;
    bool RestoreHistory() Q_DECL_OVERRIDE;

    void Reload() Q_DECL_OVERRIDE;
    void ReloadAndBypassCache() Q_DECL_OVERRIDE;
    void Stop() Q_DECL_OVERRIDE;

    void ZoomIn() Q_DECL_OVERRIDE;
    void ZoomOut() Q_DECL_OVERRIDE;

    QPointF GetScroll() Q_DECL_OVERRIDE;
    bool PageGeometry(QSizeF *contents, QRectF *viewport) Q_DECL_OVERRIDE;
    bool SaveScroll() Q_DECL_OVERRIDE;
    bool RestoreScroll() Q_DECL_OVERRIDE;
    bool SaveZoom() Q_DECL_OVERRIDE;
    bool RestoreZoom() Q_DECL_OVERRIDE;

    void DeleteLater();

    void EnvironmentReady();
    void EnvironmentFailed(long result);

    static void ClearCookies();
    static void ClearHttpCache();
    static void ClearVisitedLinks();

    static bool CanCompleteAction(const QString &action);

    int GetToken() const;
    QString GetProfileName() const;
    bool IsPrivateMode() const;

signals:
    void ViewChanged();
    void ScrollChanged(QPointF);
    void PageGeometryChanged();
    void fullScreenRequested(bool);

    void titleChanged(const QString&);
    void urlChanged(const QUrl&);
    void loadStarted();
    void loadProgress(int);
    void loadFinished(bool);
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void linkHovered(const QString&, const QString&, const QString&);

protected:
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    bool event(QEvent *ev) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *ev) Q_DECL_OVERRIDE;

private slots:
    void OnLoadStarted() Q_DECL_OVERRIDE;
    void OnLoadProgress(int progress) Q_DECL_OVERRIDE;
    void OnLoadFinished(bool ok) Q_DECL_OVERRIDE;

    void OnTitleChanged(const QString &title) Q_DECL_OVERRIDE;
    void OnUrlChanged(const QUrl &url) Q_DECL_OVERRIDE;
    void OnViewChanged() Q_DECL_OVERRIDE;
    void OnScrollChanged() Q_DECL_OVERRIDE;

public slots:
    void SetScroll(QPointF pos) Q_DECL_OVERRIDE;

    void SetFocusToElement(QString xpath);
    void FireClickEvent(QString xpath, QPoint pos);
    void SetTextValue(QString xpath, QString text);

    bool SeekText(const QString &str, View::FindFlags opt);
    void KeyEvent(QString key);

    void Load()                           Q_DECL_OVERRIDE { View::Load();}
    void Load(const QString &url)         Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QUrl &url)            Q_DECL_OVERRIDE { View::Load(url);}
    void Load(const QNetworkRequest &req) Q_DECL_OVERRIDE { View::Load(req);}

public:

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
    void StopAndUnselect() Q_DECL_OVERRIDE;
    void Save() Q_DECL_OVERRIDE;

    void Print() Q_DECL_OVERRIDE;
    void InspectElement() Q_DECL_OVERRIDE;
    QWidget *InspectorPane() Q_DECL_OVERRIDE;
    void ExitFullScreen() Q_DECL_OVERRIDE;

    void ToggleMediaControls() Q_DECL_OVERRIDE;
    void ToggleMediaLoop() Q_DECL_OVERRIDE;
    void ToggleMediaPlayPause() Q_DECL_OVERRIDE;
    void ToggleMediaMute() Q_DECL_OVERRIDE;

    bool RecentlyAudible() Q_DECL_OVERRIDE;
    void HandleAudioStateChanged(bool playing);
    void RestoreStateAfterLoad();
    bool IsAudioMuted() Q_DECL_OVERRIDE;
    void SetAudioMuted(bool muted) Q_DECL_OVERRIDE;

#ifdef MEDIATIME
    bool SaveMediaTime() Q_DECL_OVERRIDE;
    bool RestoreMediaTime() Q_DECL_OVERRIDE;
#endif

private:

private:
public:
    enum class InspectorRelease { Close, FrontendClosed, GiveBack };
private:
    void ProbeInspectorWindow();
    void AdoptInspectorWindow(WId id);
    void WatchInspectorWindow();
    void ReleaseInspector(InspectorRelease why);

    void ApplyBounds();

    void CreateController();
    bool CreateCompositionTree();
    void ReleaseCompositionTree();

    void ApplyCursor(unsigned int systemCursorId);

public:
    void OnDragEnter(IDataObject *object, unsigned long keyState,
                     long screenX, long screenY,
                     unsigned long allowed, unsigned long *effect);
    void OnDragOver(unsigned long keyState, long screenX, long screenY,
                    unsigned long allowed, unsigned long *effect);
    void OnDragLeave();
    void OnDrop(IDataObject *object, unsigned long keyState,
                long screenX, long screenY,
                unsigned long allowed, unsigned long *effect);
private:
    void ApplyPageSettings();
    void ApplyUserAgent();
    void ApplyThemeToBackend();
    void RemoveWebViewHandlers();
    void RemoveControllerHandlers();

    void Suspend();
    void WakeUp();
    bool ClearBrowsingData(unsigned int kinds,
                           std::function<void(bool)> completed =
                               std::function<void(bool)>());

    bool DeleteAllCookies();
    static void ClearOncePerProfile(unsigned int kinds,
                                    EdgeCookieClearLedger *ledger = nullptr);
    void HandlePagePrintRequest();

    int DecideCertificateError(const QUrl &url, const QString &description,
                               const QString &fingerprint);
    int DecidePermission(const QUrl &origin, int kind, bool *remember);
    void ShowPageNotification(const QString &title, const QString &body,
                              const QUrl &origin,
                              std::function<void(bool)> answer);
    void HandleFullScreen(bool on);

    void RemoveCompositionHandlers();
    void RegisterDropTarget();
    void RegisterDragStarting();
    void RevokeDropTarget();
    void AbandonDrag();
    bool DragPointOf(long screenX, long screenY, long *outX, long *outY) const;

    bool ForwardMouseEvent(QEvent *ev);
    void ShowGestureInProgress();
    void SendMouse(int kind, unsigned int data, const QPointF &pos,
                   Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons);
    void AbandonMouse();
    void RegisterHandlers();
    void RegisterNavigationHandlers();
    void RegisterDocumentHandlers();
    void RegisterAskingHandlers();
    void RegisterInputHandlers();
    void RegisterInputBridge();
    void AdoptReportedSource(const QUrl &reported);
    void HandleWebMessage(const QString &json, const QString &source);

    struct ContextTarget {
        QPoint m_Position;
        QUrl m_LinkUrl;
        QUrl m_SourceUrl;
        QString m_SelectedText;
        bool m_Editable;
        int m_MediaType;

        ContextTarget()
            : m_Position(QPoint()), m_LinkUrl(QUrl()), m_SourceUrl(QUrl())
            , m_SelectedText(QString()), m_Editable(false), m_MediaType(0) {}
    };
    void DisplayContextMenuFor(const ContextTarget &target);

    void PullCookiesIntoJar();

    typedef std::function<void(const QVariant&)> VariantCallBack;
    void CallWithScriptResult(const QString &script, VariantCallBack callBack);
    void RunScript(const QString &script);

public:

    void CallWithGotBaseUrl(UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithGotCurrentBaseUrl(UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithFoundElements(Page::FindElementsOption option,
                               WebElementListCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithHitElement(const QPoint &pos, WebElementCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithSelectedText(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithSelectedHtml(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithWholeText(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithWholeHtml(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithSelectionRegion(RegionCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithEvaluatedJavaScriptResult(const QString &code,
                                           VariantCallBack callBack) Q_DECL_OVERRIDE;

    void WheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
    void UpKeyEvent()       Q_DECL_OVERRIDE { RunScript(UpKeyEventJsCode());}
    void DownKeyEvent()     Q_DECL_OVERRIDE { RunScript(DownKeyEventJsCode());}
    void RightKeyEvent()    Q_DECL_OVERRIDE { RunScript(RightKeyEventJsCode());}
    void LeftKeyEvent()     Q_DECL_OVERRIDE { RunScript(LeftKeyEventJsCode());}
    void PageDownKeyEvent() Q_DECL_OVERRIDE { RunScript(PageDownKeyEventJsCode());}
    void PageUpKeyEvent()   Q_DECL_OVERRIDE { RunScript(PageUpKeyEventJsCode());}
    void HomeKeyEvent()     Q_DECL_OVERRIDE { RunScript(HomeKeyEventJsCode());}
    void EndKeyEvent()      Q_DECL_OVERRIDE { RunScript(EndKeyEventJsCode());}

private:

    qreal PageScale() const;
    QPoint PagePointOf(const QPoint &widgetPoint) const;
    QRect WidgetRectOf(const QRect &pageRect) const;
    bool HandleAcceleratorKey(int virtualKey, bool down, bool repeat);
    void ApplyPendingState();
    void ApplyVisibility();
    void Navigate(const QUrl &url);
    void NavigateWithRequest(const QNetworkRequest &req,
                             QNetworkAccessManager::Operation operation,
                             const QByteArray &body);

    bool MayStartLoad() const;
    void StartLoad(const EdgePendingLoad &load);
    void PerformLoad(const EdgePendingLoad &load);
    void TryFlushPendingNavigation();
    QString ProfileKey() const;
    bool MeasureProfile(struct ICoreWebView2Controller *controller,
                        QString *name, bool *isPrivate);
    void ReportPrivateWipeFailure();
    static void SettlePrivateWipe(const QString &key, bool ok);
    void RegisterResourceFilter();

    long AnswerVanillaPage(struct ICoreWebView2WebResourceRequestedEventArgs *args,
                           struct ICoreWebView2WebResourceRequest *request,
                           const QUrl &url, bool document);
    void Retire();
    void PerformHistoryMove(const NativeHistory::Request &request);
    void ReportCreationFailure();

    bool BuildControllerOptions(struct ICoreWebView2Environment *environment,
                                struct ICoreWebView2Environment10 **environment10,
                                struct ICoreWebView2ControllerOptions **options);
    void FailCreation(const char *why);
    void UnwireComposition();
    void TakeController(EdgeUnadoptedControllerOf<ICoreWebView2Controller> &made);

    struct Private;
    std::unique_ptr<Private> m_Impl;
};

#endif
#endif
