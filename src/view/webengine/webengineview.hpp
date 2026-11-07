#ifndef WEBENGINEVIEW_HPP
#define WEBENGINEVIEW_HPP

#include "switch.hpp"

#ifdef WEBENGINEVIEW

#include "webenginepage.hpp"
#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"
#include "settingspage.hpp"
#include "referrer.hpp"

#include <stdlib.h>

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineHistory>
#include <QWebEngineHttpRequest>
#include <QPointer>
#include <QGuiApplication>
#include <QInputMethod>
#include <QTransform>

class QKeySequence;
class EventEater;

class WebEngineView : public QWebEngineView, public View {
    Q_OBJECT

public:
    WebEngineView(TreeBank *parent = nullptr, QString id = "", QStringList set = QStringList());
    ~WebEngineView() override;

    static void SetUpEventFilterInstaller();

    QWebEngineView *base() override;
    WebEnginePage *page() override;

    QUrl url() override;
    QString html() override;
    TreeBank *parent() override;
    void setUrl(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &url) override;
    void setParent(TreeBank* tb) override;

    void Connect(TreeBank *tb) override;
    void Disconnect(TreeBank *tb) override;

    void ApplySpecificSettings(QStringList set) override;
    bool IsOffTheRecord() override;
    QObject *EngineProfile() override;

    void ApplyTheme() override;
    void KeepDrawingWidgetClear(QWidget *drawing);

    void AddSpellCheckMenu(QMenu *menu) override;

    void Suspend();
    void WakeUp();

    bool ForbidToOverlap() override {
        return false;
    }

    bool IsOwnDragSource(QObject *source) const override {
        return source && source == this;
    }

    QWidget *InspectorPane() override {
        if(!m_Inspector || TreeBank::PurgeView() || !InspectorInMainWindow())
            return nullptr;
        return m_Inspector;
    }

    bool CanGoBack() override {
        return page() ? page()->history()->canGoBack() : false;
    }
    bool CanGoForward() override {
        return page() ? page()->history()->canGoForward() : false;
    }
    bool RecentlyAudible() override {
        return page() ? page()->recentlyAudible() : false;
    }
    bool IsAudioMuted() override {
        return page() ? page()->isAudioMuted() : false;
    }
    void SetAudioMuted(bool muted) override {
        if(page()) page()->setAudioMuted(muted);
    }

    bool IsRenderable() override {
        return page() != 0 && (visible() || !m_GrabedDisplayData.isNull());
    }
    bool MakesSidePanels() const override { return true; }
    QUrl CommittedUrl() override {
        return page() && page()->history() ? page()->history()->currentItem().url() : QUrl();
    }
    QImage CaptureVisible() override {
        if(!page() || !visible() || !window() || window()->isMinimized()) return QImage();
        return grab().toImage();
    }
    void Render(QPainter *painter) override {
        if(visible()){
            QImage image(size(), QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            QPainter p(&image);
            render(&p, QPoint(), QRegion(), QWidget::DrawChildren);
            p.end();
            m_GrabedDisplayData = image;

            render(painter, QPoint(), QRegion(), QWidget::DrawChildren);

        } else if(!m_GrabedDisplayData.isNull()){
            painter->drawImage(QPoint(), m_GrabedDisplayData);
        }
    }
    void Render(QPainter *painter, const QRegion &clip) override {
        if(visible()){
            QImage image(size(), QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            QPainter p(&image);
            render(&p, QPoint(), QRegion(), QWidget::DrawChildren);
            p.end();
            m_GrabedDisplayData = image;

            if(!clip.isEmpty())
                render(painter, clip.boundingRect().topLeft(), clip, QWidget::DrawChildren);
        } else if(!m_GrabedDisplayData.isNull()){
            foreach(QRect rect, clip){
                painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
            }
        }
    }
    QSize GetViewportSize() override {
        return size();
    }
    void SetViewportSize(QSize size) override {
        if(!visible()) resize(size);
    }
    bool PageGeometry(QSizeF *contents, QRectF *viewport) override {
        if(!page()) return false;
        const QSizeF whole = page()->contentsSize();
        if(whole.isEmpty()) return false;
        *contents = whole;
        *viewport = QRectF(page()->scrollPosition(), QSizeF(size()));
        return true;
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
        return title();
    }
    QIcon GetIcon() override {
        return page() ? page()->icon() : QIcon();
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) override {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) override {
        return page() ? page()->Action(a, data) : 0;
    }

    ExtensionController *Extensions() const override;
    QWidget *CreateExtensionView(const QUrl &url, ExtensionPage page, QWidget *parent,
                                 const std::function<void()> &closed = std::function<void()>()) override;
    void TriggerNativeLoadAction(const QUrl &url) override;
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) override {
        emit urlChanged(req.url());

        QWebEngineHttpRequest request
            (req.url(), operation == QNetworkAccessManager::PostOperation
             ? QWebEngineHttpRequest::Post
             : QWebEngineHttpRequest::Get);

        foreach(const QByteArray &name, req.rawHeaderList()){
            if(name.compare("Referer", Qt::CaseInsensitive) != 0){
                request.setHeader(name, req.rawHeader(name));
                continue;
            }
            const QByteArray referer = Referrer::ForNavigation
                (QUrl::fromEncoded(req.rawHeader(name)), req.url());
            if(!referer.isEmpty()) request.setHeader(name, referer);
        }

        if(operation == QNetworkAccessManager::PostOperation)
            request.setPostData(body);

        LoadAfterExtensions(request);
    }
    void TriggerNativeGoBackAction() override {
        if(page()) page()->triggerAction(QWebEnginePage::Back);
    }
    void TriggerNativeGoForwardAction() override {
        if(page()) page()->triggerAction(QWebEnginePage::Forward);
    }
    void TriggerNativeRewindAction() override {
        QWebEngineHistory *h = history();
        h->goToItem(h->itemAt(0));
    }
    void TriggerNativeFastForwardAction() override {
        QWebEngineHistory *h = history();
        h->goToItem(h->itemAt(h->count()-1));
    }

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
    void WheelEvent(QWheelEvent *ev) override;
    void EngineWheel(QWheelEvent *ev);

    void CallWithGotBaseUrl(UrlCallBack callBack) override;
    void CallWithGotCurrentBaseUrl(UrlCallBack callBack) override;
    void CallWithFoundElements(Page::FindElementsOption option, WebElementListCallBack callBack) override;
    void CallWithHitElement(const QPoint &pos, WebElementCallBack callBack) override;
    void CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack) override;
    void CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack) override;
    void CallWithSelectedText(StringCallBack callBack) override;
    void CallWithSelectedHtml(StringCallBack callBack) override;
    void CallWithWholeText(StringCallBack callBack) override;
    void CallWithWholeHtml(StringCallBack callBack) override;
    void CallWithSelectionRegion(RegionCallBack callBack) override;
    void CallWithEvaluatedJavaScriptResult(const QString &code, VariantCallBack callBack) override;

public slots:
    QSize size() override { return base()->size();}
    void resize(QSize size) override {
        if(TreeBank::PurgeView() && m_TreeBank){
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
    void repaint() override { base()->repaint();}
    bool visible() override { return base()->isVisible();}
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
    void OnTitleChanged(const QString&) override;
    void OnUrlChanged(const QUrl&) override;
    void OnViewChanged() override;
    void OnScrollChanged() override;

    void EmitScrollChanged() override;

    void CallWithScroll(PointFCallBack callBack);
    void SetScrollBarState() override;
    QPointF GetScroll() override;
    void SetScroll(QPointF pos) override;
    bool SaveScroll() override;
    bool RestoreScroll() override;
    bool SaveZoom() override;
    bool RestoreZoom() override;
    float MinimumZoom() const override { return ChromiumMinimumZoom / DeviceZoomScale();}
    float MaximumZoom() const override { return ChromiumMaximumZoom / DeviceZoomScale();}
    bool SaveHistory() override;
    bool RestoreHistory() override;
#ifdef MEDIATIME
    bool SaveMediaTime(VoidCallBack settled = VoidCallBack()) override;
    bool RestoreMediaTime() override;
#endif

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    void SetFocusToElement(QString);
    void FireClickEvent(QString, QPoint);
    void SetTextValue(QString, QString);
    void OnIconChanged(const QIcon &icon);

    void Copy() override;
    void Cut() override;
    void Paste() override;

#define VANILLA_EDIT_ACTION(name)                                       \
    void name() override {                                       \
        if(page()) page()->triggerAction(QWebEnginePage::name);         \
    }
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
    void InspectElement() override;
    void CloseInspector();
    void AddSearchEngine(QPoint pos) override;
    void AddBookmarklet(QPoint pos)  override;

    void LoadAfterExtensions(const QWebEngineHttpRequest &request);
signals:
    void ExtensionContextChanged();
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

protected:
    void timerEvent(QTimerEvent *ev) override;
    void childEvent(QChildEvent *ev) override;
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
    QImage m_GrabedDisplayData;
    QWebEngineView *m_Inspector;
    QColor m_BaseBackgroundColor;
    int m_ScrollSignalTimer;
#ifdef MEDIATIME
    int m_MediaTimeSaveTimer;
#endif
    bool m_PreventScrollRestoration;
    qreal m_ReportedZoomFactor;

    friend class EventEater;
};

class EventEater : public QObject {
    Q_OBJECT

public:
    EventEater(WebEngineView *view, QObject *parent)
        : QObject(parent){
        m_View = view;
    }

private:
    QPointF MapToView(QWidget *widget, QPointF localPos){
        return QPointF(widget->mapTo(m_View->base(), localPos.toPoint()));
    }
    QPoint MapToView(QWidget *widget, QPoint pos){
        return widget->mapTo(m_View->base(), pos);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override {
        QWidget *widget = qobject_cast<QWidget*>(obj);

        switch(ev->type()){
        case QEvent::Paint: case QEvent::Show: case QEvent::Resize:
            m_View->KeepDrawingWidgetClear(widget);
            break;
        case QEvent::FocusIn:{
            m_View->focusInEvent(static_cast<QFocusEvent*>(ev));
            return false;
        }
        case QEvent::FocusOut:{
            m_View->focusOutEvent(static_cast<QFocusEvent*>(ev));
            return false;
        }
        case QEvent::KeyPress:{
            QKeyEvent *ke = static_cast<QKeyEvent*>(ev);

            if(m_View->GetDisplayObscured()){
                if(ke->key() == Qt::Key_Escape || ke->key() == Qt::Key_F11){
                    m_View->ExitFullScreen();
                    return true;
                }
            }

            if(VanillaPage::IsSettingsUrl(m_View->url())){
                if(Application::IsMoveKey(ke)) m_View->m_PreventScrollRestoration = true;
                return false;
            }

            if(Application::HasAnyModifier(ke) ||
               Application::IsFunctionKey(ke)){
                return m_View->TriggerKeyEvent(ke);
            }

            if(!m_View->m_PreventScrollRestoration &&
               Application::IsMoveKey(ke)){
                m_View->m_PreventScrollRestoration = true;
            }
            return false;
        }
        case QEvent::KeyRelease:{
            return false;
        }
        case QEvent::InputMethodQuery:{
            if(widget && widget == QGuiApplication::focusObject()){
                const QPoint origin = widget->mapTo(widget->window(), QPoint(0, 0));
                const QTransform transform = QTransform::fromTranslate(origin.x(), origin.y());
                QInputMethod *method = QGuiApplication::inputMethod();
                if(method->inputItemTransform() != transform){
                    method->setInputItemTransform(transform);
                    method->setInputItemRectangle(widget->rect());
                }
            }
            return false;
        }
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:{
            QMouseEvent *me = static_cast<QMouseEvent*>(ev);
            QMouseEvent me_ =
                QMouseEvent(me->type(),
                            MapToView(widget, me->position()),
                            me->globalPosition(),
                            me->button(),
                            me->buttons(),
                            me->modifiers());
            switch(ev->type()){
            case QEvent::MouseMove:
                m_View->mouseMoveEvent(&me_);
                return me_.isAccepted();
            case QEvent::MouseButtonPress:
                m_View->mousePressEvent(&me_);
                return me_.isAccepted();
            case QEvent::MouseButtonRelease:
                m_View->mouseReleaseEvent(&me_);
                return me_.isAccepted();
            case QEvent::MouseButtonDblClick:
                m_View->mouseDoubleClickEvent(&me_);
                return me_.isAccepted();
            default: break;
            }
            break;
        }
        case QEvent::Wheel:{
            QWheelEvent *we = static_cast<QWheelEvent*>(ev);

            if(!ev->spontaneous()) return false;

            if(m_View->m_EnableScrollGesture){
                if(m_View->m_GestureStartedPos == QPoint(-1, -1)){
                    static QString gesture;
                    m_View->GestureMoved(m_View->m_BeforeGesturePos + we->pixelDelta());
                    QString cur = m_View->GestureToString(m_View->m_Gesture);
                    if(gesture != cur){
                        gesture = cur;
                        emit m_View->statusBarMessage
                            (m_View->GestureStatusLine(m_View->m_ScrollGestureMap, tr("NoAction")));
                    }
                }
                m_View->EngineWheel(we);
                return true;
            }
            m_View->WheelEvent(we);
            return true;
        }
        case QEvent::TouchBegin:{
            if(m_View->m_EnableScrollGesture){
                m_View->GestureStarted(QPoint(-1, -1));
                return false;
            }
            break;
        }
        case QEvent::TouchEnd:{
            if(m_View->m_EnableScrollGesture && !m_View->m_Gesture.isEmpty()){
                m_View->GestureFinished(QPoint(-1, -1), Qt::NoButton);
                return false;
            }
            break;
        }
        default: break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    WebEngineView *m_View;
};

#endif
#endif
