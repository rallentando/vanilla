#ifndef EDGEWEBVIEW_HPP
#define EDGEWEBVIEW_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

struct IDataObject;
struct EdgePendingLoad;
class EdgeCookieClearLedger;
class QPainter;
struct ICoreWebView2Controller;
struct ICoreWebView2ContextMenuRequestedEventArgs;
struct ICoreWebView2Deferral;
struct ICoreWebView2DownloadOperation;
template <class Controller> class EdgeUnadoptedControllerOf;

#include "view.hpp"
#include "nativehistory.hpp"
#include "edgemenuitem.hpp"

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

    void ApplySpecificSettings(QStringList set) override;
    bool IsOffTheRecord() override;
    QString EngineProfileName() override;

    void ApplyTheme() override;

    QWidget *base() override;
    Page *page() override;
    ExtensionController *Extensions() const override;
    QString ExtensionStatus() const override;
    QWidget *CreateExtensionView(const QUrl &url, ExtensionPage page, QWidget *parent,
                                 const std::function<void()> &closed = std::function<void()>()) override;
    void SetupExtensions();
    QUrl ReportedSource() const;

    QUrl url() override;
    TreeBank *parent() override;
    void setUrl(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &url) override;
    void setParent(TreeBank *t) override;

    void Connect(TreeBank *tb) override;
    void Disconnect(TreeBank *tb) override;

    bool ForbidToOverlap() override { return true;}

    QSize size() override { return QWidget::size();}
    void resize(QSize size) override { QWidget::resize(size);}
    void show() override { QWidget::show(); WakeUp();}
    void hide() override { QWidget::hide(); Suspend();}
    void raise() override { QWidget::raise();}
    void lower() override { QWidget::lower();}
    void repaint() override { QWidget::repaint();}
    bool visible() override { return QWidget::isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) override;
    void TakeKeyboardBack() override;

    void OnBeforeStartingDisplayGadgets() override;
    void OnAfterFinishingDisplayGadgets() override { show();}
    bool IsRenderable() override;
    void Render(QPainter *painter) override;
    void Render(QPainter *painter, const QRegion &clip) override;
    QImage GrabView();
    QImage CaptureVisible() override;
    bool MakesSidePanels() const override { return true; }
    QUrl CommittedUrl() override { return ReportedSource();}

    QSize GetViewportSize() override;
    void SetViewportSize(QSize) override {}

    QString GetTitle() override;
    QIcon GetIcon() override;

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) override {
        if(QAction *action = Action(a, data)) action->trigger();
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) override {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) override {
        Navigate(url);
    }

    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) override {
        NavigateWithRequest(req, operation, body);
    }
    void TriggerNativeGoBackAction() override;
    void TriggerNativeGoForwardAction() override;
    void TriggerNativeRewindAction() override;
    void TriggerNativeFastForwardAction() override;

    bool CanGoBack() override;
    bool CanGoForward() override;
    void GoBackToInferedUrl() override;
    void GoForwardToInferedUrl() override;

    bool SaveHistory() override;
    bool RestoreHistory() override;

    void Reload() override;
    void ReloadAndBypassCache() override;
    void Stop() override;

    void ZoomIn() override;
    void ZoomOut() override;

    QPointF GetScroll() override;
    bool PageGeometry(QSizeF *contents, QRectF *viewport) override;
    bool SaveScroll() override;
    bool RestoreScroll() override;
    bool SaveZoom() override;
    bool RestoreZoom() override;
    float MinimumZoom() const override { return ChromiumMinimumZoom;}
    float MaximumZoom() const override { return ChromiumMaximumZoom;}

    void DeleteLater();

    void EnvironmentReady();
    void EnvironmentFailed(long result);

    static void ClearCookies();
    static void ClearHttpCache();
    static void ClearVisitedLinks();
    static void ClearPasswords();

    static QString ProfilesFolder();
    static QString ExtensionJournal(const QString &profileName);
    static QObject *ReleaseExtensionsOf(const QString &profileName);
    static QStringList ProfilesWithExtensions();
    static QObject *BrowserExitIfNoViews();
    static void ForgetMissingProfiles();
    static void CopyCookiesForMerge(const QString &fromProfile, const QString &toProfile,
                                    std::function<void(bool, int)> done);

    static void MirrorPermissions();
    static QString TablePermissionKind(int kind);

    static void ReleaseDownloadCarriers();

    static bool RuntimeAvailable();
    static QString RuntimeVersion();
    static void StartExtensionCopiesAhead();
    static void HurryExtensionCopiesAhead();

private:
    void AskAgainAboutAbort(int generation, qint64 after);
    void LeaveOutOfSaves();
    void PutBackIntoSaves();

public:

    static bool CanCompleteAction(const QString &action);

signals:
    void ExtensionContextChanged();
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
    void timerEvent(QTimerEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    bool event(QEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *ev) override;

private slots:
    void OnLoadStarted() override;
    void OnLoadProgress(int progress) override;
    void OnLoadFinished(bool ok) override;

    void OnTitleChanged(const QString &title) override;
    void OnUrlChanged(const QUrl &url) override;
    void OnViewChanged() override;
    void OnScrollChanged() override;

public slots:
    void SetScroll(QPointF pos) override;

    void SetFocusToElement(QString xpath);
    void FireClickEvent(QString xpath, QPoint pos);
    void SetTextValue(QString xpath, QString text);

    bool SeekText(const QString &str, View::FindFlags opt);
    void KeyEvent(QString key);

    void Load()                           override { View::Load();}
    void Load(const QString &url)         override { View::Load(url);}
    void Load(const QUrl &url)            override { View::Load(url);}
    void Load(const QNetworkRequest &req) override { View::Load(req);}

public:

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
    void StopAndUnselect() override;
    void Save() override;

    void Print() override;
    void InspectElement() override;
    QWidget *InspectorPane() override;
    void AddSearchEngine(QPoint pos) override;
    void AddBookmarklet(QPoint pos) override;
    void ExitFullScreen() override;

    void ToggleMediaControls() override;
    void ToggleMediaLoop() override;
    void ToggleMediaPlayPause() override;
    void ToggleMediaMute() override;

    bool RecentlyAudible() override;
    void HandleAudioStateChanged(bool playing);
    void RestoreStateAfterLoad();
    bool IsAudioMuted() override;
    void SetAudioMuted(bool muted) override;

#ifdef MEDIATIME
    bool SaveMediaTime(VoidCallBack settled = VoidCallBack()) override;
    bool RestoreMediaTime() override;
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
    bool InspectorHoldsKeyboard() const;

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
    bool KeepForDownloads() override;
    void WireDownload(ICoreWebView2DownloadOperation *operation,
                      const QString &name, const QString &path,
                      const QUrl &source, bool offTheRecord);

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
        QList<EdgeMenuItem> m_ExtensionItems;
        int m_PasteCommandId;
        int m_Generation;
        bool m_Superseded;
        int m_Sequence;

        ContextTarget()
            : m_Position(QPoint()), m_LinkUrl(QUrl()), m_SourceUrl(QUrl())
            , m_SelectedText(QString()), m_Editable(false), m_MediaType(0)
            , m_ExtensionItems(QList<EdgeMenuItem>()), m_PasteCommandId(-1), m_Generation(0)
            , m_Superseded(false), m_Sequence(0) {}
    };
    void DisplayContextMenuFor(const ContextTarget &target);
    void CompleteContextMenu(int generation, int commandId, bool extension);
    void FinishContextMenu(int commandId, bool extension);
    void CountedComplete(ICoreWebView2ContextMenuRequestedEventArgs *args,
                         ICoreWebView2Deferral *deferral, int commandId, bool extension);

    void PullCookiesIntoJar();

    typedef std::function<void(const QVariant&)> VariantCallBack;
    void CallWithScriptResult(const QString &script, VariantCallBack callBack,
                              VoidCallBack discarded = VoidCallBack());
    void RunScript(const QString &script);

public:

    void CallWithGotBaseUrl(UrlCallBack callBack) override;
    void CallWithGotCurrentBaseUrl(UrlCallBack callBack) override;
    void CallWithFoundElements(Page::FindElementsOption option,
                               WebElementListCallBack callBack) override;
    void CallWithHitElement(const QPoint &pos, WebElementCallBack callBack) override;
    void CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack) override;
    void CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack) override;
    void CallWithSelectedText(StringCallBack callBack) override;
    void CallWithSelectedHtml(StringCallBack callBack) override;
    void CallWithWholeText(StringCallBack callBack) override;
    void CallWithWholeHtml(StringCallBack callBack) override;
    void CallWithSelectionRegion(RegionCallBack callBack) override;
    void CallWithEvaluatedJavaScriptResult(const QString &code,
                                           VariantCallBack callBack) override;

    void WheelEvent(QWheelEvent *ev) override;
    void UpKeyEvent()       override { RunScript(UpKeyEventJsCode());}
    void DownKeyEvent()     override { RunScript(DownKeyEventJsCode());}
    void RightKeyEvent()    override { RunScript(RightKeyEventJsCode());}
    void LeftKeyEvent()     override { RunScript(LeftKeyEventJsCode());}
    void PageDownKeyEvent() override { RunScript(PageDownKeyEventJsCode());}
    void PageUpKeyEvent()   override { RunScript(PageUpKeyEventJsCode());}
    void HomeKeyEvent()     override { RunScript(HomeKeyEventJsCode());}
    void EndKeyEvent()      override { RunScript(EndKeyEventJsCode());}

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
    bool IsOwnReportedSource(const QUrl &reported) const;
    void TryFlushPendingNavigation();
    QString ProfileKey() const;
    bool MeasureProfile(struct ICoreWebView2Controller *controller,
                        QString *name, bool *isPrivate);
    void ReportPrivateWipeFailure();
    static void SettlePrivateWipe(const QString &key, bool ok);
    void EnterPermissionSync();
    static void RunPermissionSync(const QString &key, int run);
    static void SettlePermissionSync(const QString &key, int run, bool ok);
    void RegisterResourceFilter();

    long AnswerVanillaPage(struct ICoreWebView2WebResourceRequestedEventArgs *args,
                           struct ICoreWebView2WebResourceRequest *request,
                           const QUrl &url, bool document);
    void Retire(bool force = false);
    bool IsGoing() const;
    void PerformHistoryMove(const NativeHistory::Request &request);
    void ReportCreationFailure(long result);
    void ShowFailure(const QString &text);
    void PaintHostWindow(bool leavingClear = false);

    bool BuildControllerOptions(struct ICoreWebView2Environment *environment,
                                struct ICoreWebView2Environment10 **environment10,
                                struct ICoreWebView2ControllerOptions **options);
    void FailCreation(const char *why, long result);
    void UnwireComposition();
    void TakeController(EdgeUnadoptedControllerOf<ICoreWebView2Controller> &made);

    struct Private;
    friend class EdgeDownloadCarriers;

    std::unique_ptr<Private> m_Impl;
};

#endif
#endif
