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
    ~WebEngineView() Q_DECL_OVERRIDE;

    static void SetUpEventFilterInstaller();

    QWebEngineView *base() Q_DECL_OVERRIDE;
    WebEnginePage *page() Q_DECL_OVERRIDE;

    QUrl url() Q_DECL_OVERRIDE;
    QString html() Q_DECL_OVERRIDE;
    TreeBank *parent() Q_DECL_OVERRIDE;
    void setUrl(const QUrl &url) Q_DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) Q_DECL_OVERRIDE;
    void setParent(TreeBank* tb) Q_DECL_OVERRIDE;

    void Connect(TreeBank *tb) Q_DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) Q_DECL_OVERRIDE;

    void ApplySpecificSettings(QStringList set) Q_DECL_OVERRIDE;

    void ApplyTheme() Q_DECL_OVERRIDE;

    void AddSpellCheckMenu(QMenu *menu) Q_DECL_OVERRIDE;

    void Suspend();
    void WakeUp();

    bool ForbidToOverlap() Q_DECL_OVERRIDE {
        return false;
    }

    bool IsOwnDragSource(QObject *source) const Q_DECL_OVERRIDE {
        return source && source == this;
    }

    QWidget *InspectorPane() Q_DECL_OVERRIDE {
        if(!m_Inspector || TreeBank::PurgeView() || !InspectorInMainWindow())
            return nullptr;
        return m_Inspector;
    }

    bool CanGoBack() Q_DECL_OVERRIDE {
        return page() ? page()->history()->canGoBack() : false;
    }
    bool CanGoForward() Q_DECL_OVERRIDE {
        return page() ? page()->history()->canGoForward() : false;
    }
    bool RecentlyAudible() Q_DECL_OVERRIDE {
        return page() ? page()->recentlyAudible() : false;
    }
    bool IsAudioMuted() Q_DECL_OVERRIDE {
        return page() ? page()->isAudioMuted() : false;
    }
    void SetAudioMuted(bool muted) Q_DECL_OVERRIDE {
        if(page()) page()->setAudioMuted(muted);
    }

    bool IsRenderable() Q_DECL_OVERRIDE {
        return page() != 0 && (visible() || !m_GrabedDisplayData.isNull());
    }
    bool MakesSidePanels() const Q_DECL_OVERRIDE { return true; }
    QUrl CommittedUrl() Q_DECL_OVERRIDE {
        return page() && page()->history() ? page()->history()->currentItem().url() : QUrl();
    }
    QImage CaptureVisible() Q_DECL_OVERRIDE {
        if(!page() || !visible() || !window() || window()->isMinimized()) return QImage();
        return grab().toImage();
    }
    void Render(QPainter *painter) Q_DECL_OVERRIDE {
        if(visible()){
            QImage image(size(), QImage::Format_ARGB32);
            QPainter p(&image);
            render(&p);
            p.end();
            m_GrabedDisplayData = image;

            render(painter);

        } else if(!m_GrabedDisplayData.isNull()){
            painter->drawImage(QPoint(), m_GrabedDisplayData);
        }
    }
    void Render(QPainter *painter, const QRegion &clip) Q_DECL_OVERRIDE {
        if(visible()){
            QImage image(size(), QImage::Format_ARGB32);
            QPainter p(&image);
            render(&p);
            p.end();
            m_GrabedDisplayData = image;

            if(!clip.isEmpty())
                render(painter, clip.boundingRect().topLeft(), clip);
        } else if(!m_GrabedDisplayData.isNull()){
            foreach(QRect rect, clip){
                painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
            }
        }
    }
    QSize GetViewportSize() Q_DECL_OVERRIDE {
        return size();
    }
    void SetViewportSize(QSize size) Q_DECL_OVERRIDE {
        if(!visible()) resize(size);
    }
    bool PageGeometry(QSizeF *contents, QRectF *viewport) Q_DECL_OVERRIDE {
        if(!page()) return false;
        const QSizeF whole = page()->contentsSize();
        if(whole.isEmpty()) return false;
        *contents = whole;
        *viewport = QRectF(page()->scrollPosition(), QSizeF(size()));
        return true;
    }
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
        return title();
    }
    QIcon GetIcon() Q_DECL_OVERRIDE {
        return page() ? page()->icon() : QIcon();
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) Q_DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    ExtensionController *Extensions() const override;
    QWidget *CreateExtensionView(const QUrl &url, ExtensionPage page, QWidget *parent,
                                 const std::function<void()> &closed = std::function<void()>()) override;
    void TriggerNativeLoadAction(const QUrl &url) Q_DECL_OVERRIDE;
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) Q_DECL_OVERRIDE {
        emit urlChanged(req.url());

        QWebEngineHttpRequest request
            (req.url(), operation == QNetworkAccessManager::PostOperation
             ? QWebEngineHttpRequest::Post
             : QWebEngineHttpRequest::Get);

        foreach(const QByteArray &name, req.rawHeaderList())
            request.setHeader(name, req.rawHeader(name));

        if(operation == QNetworkAccessManager::PostOperation)
            request.setPostData(body);

        LoadAfterExtensions(request);
    }
    void TriggerNativeGoBackAction() Q_DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebEnginePage::Back);
    }
    void TriggerNativeGoForwardAction() Q_DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebEnginePage::Forward);
    }
    void TriggerNativeRewindAction() Q_DECL_OVERRIDE {
        QWebEngineHistory *h = history();
        h->goToItem(h->itemAt(0));
    }
    void TriggerNativeFastForwardAction() Q_DECL_OVERRIDE {
        QWebEngineHistory *h = history();
        h->goToItem(h->itemAt(h->count()-1));
    }

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
    void WheelEvent(QWheelEvent *ev) Q_DECL_OVERRIDE;
    void EngineWheel(QWheelEvent *ev);

    void CallWithGotBaseUrl(UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithGotCurrentBaseUrl(UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithFoundElements(Page::FindElementsOption option, WebElementListCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithHitElement(const QPoint &pos, WebElementCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithSelectedText(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithSelectedHtml(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithWholeText(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithWholeHtml(StringCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithSelectionRegion(RegionCallBack callBack) Q_DECL_OVERRIDE;
    void CallWithEvaluatedJavaScriptResult(const QString &code, VariantCallBack callBack) Q_DECL_OVERRIDE;

public slots:
    QSize size() Q_DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) Q_DECL_OVERRIDE {
        if(TreeBank::PurgeView() && m_TreeBank){
            base()->setGeometry(QRect(m_TreeBank->mapToGlobal(QPoint()), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
    }
    void show() Q_DECL_OVERRIDE {
        const bool wasVisible = base()->isVisible();
        WakeUp();
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
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
    void hide()    Q_DECL_OVERRIDE { base()->hide(); Suspend();}
    void raise()   Q_DECL_OVERRIDE { base()->raise();}
    void lower()   Q_DECL_OVERRIDE { base()->lower();}
    void repaint() Q_DECL_OVERRIDE { base()->repaint();}
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
    }

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
    float MinimumZoom() const Q_DECL_OVERRIDE { return ChromiumMinimumZoom / DeviceZoomScale();}
    float MaximumZoom() const Q_DECL_OVERRIDE { return ChromiumMaximumZoom / DeviceZoomScale();}
    bool SaveHistory() Q_DECL_OVERRIDE;
    bool RestoreHistory() Q_DECL_OVERRIDE;
#ifdef MEDIATIME
    bool SaveMediaTime(VoidCallBack settled = VoidCallBack()) Q_DECL_OVERRIDE;
    bool RestoreMediaTime() Q_DECL_OVERRIDE;
#endif

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    void SetFocusToElement(QString);
    void FireClickEvent(QString, QPoint);
    void SetTextValue(QString, QString);
    void OnIconChanged(const QIcon &icon);

    void Copy() Q_DECL_OVERRIDE;
    void Cut() Q_DECL_OVERRIDE;
    void Paste() Q_DECL_OVERRIDE;

#define VANILLA_EDIT_ACTION(name)                                       \
    void name() Q_DECL_OVERRIDE {                                       \
        if(page()) page()->triggerAction(QWebEnginePage::name);         \
    }
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
    void InspectElement() Q_DECL_OVERRIDE;
    void CloseInspector();
    void AddSearchEngine(QPoint pos) Q_DECL_OVERRIDE;
    void AddBookmarklet(QPoint pos)  Q_DECL_OVERRIDE;

    void LoadAfterExtensions(const QWebEngineHttpRequest &request);
signals:
    void ExtensionContextChanged();
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

protected:
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void childEvent(QChildEvent *ev) Q_DECL_OVERRIDE;
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
    bool eventFilter(QObject *obj, QEvent *ev) Q_DECL_OVERRIDE {
        QWidget *widget = qobject_cast<QWidget*>(obj);

        switch(ev->type()){
        case QEvent::FocusIn:{
            QFocusEvent *fe = static_cast<QFocusEvent*>(ev);
            m_View->focusInEvent(fe);
            return fe->isAccepted();
        }
        case QEvent::FocusOut:{
            QFocusEvent *fe = static_cast<QFocusEvent*>(ev);
            m_View->focusOutEvent(fe);
            return fe->isAccepted();
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
                        QString action =
                            !m_View->m_ScrollGestureMap.contains(gesture)
                              ? tr("NoAction")
                            : Page::IsValidAction(m_View->m_ScrollGestureMap[gesture])
                              ? m_View->Action(Page::StringToAction(m_View->m_ScrollGestureMap[gesture]))->text()
                            : m_View->m_ScrollGestureMap[gesture];
                        emit m_View->statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
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
