#ifndef WEBENGINEVIEW_HPP
#define WEBENGINEVIEW_HPP

#include "switch.hpp"

#include "webenginepage.hpp"
#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#include <stdlib.h>

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineHistory>

#ifdef PASSWORD_MANAGER
# include <QWebEngineProfile>
#endif

class QKeySequence;
class EventEater;

class WebEngineView : public QWebEngineView, public View{
    Q_OBJECT

public:
    WebEngineView(TreeBank *parent = 0, QString id = "", QStringList set = QStringList());
    ~WebEngineView();

    QWebEngineView *base() DECL_OVERRIDE;
    WebEnginePage *page() DECL_OVERRIDE;

    QUrl url() DECL_OVERRIDE;
    QString html() DECL_OVERRIDE;
    TreeBank *parent() DECL_OVERRIDE;
    void setUrl(const QUrl &url) DECL_OVERRIDE;
    void setHtml(const QString &html, const QUrl &url) DECL_OVERRIDE;
    void setParent(TreeBank* tb) DECL_OVERRIDE;

    void Connect(TreeBank *tb) DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) DECL_OVERRIDE;

    QUrl BaseUrl() DECL_OVERRIDE {
        return GetBaseUrl();
    }
    QUrl CurrentBaseUrl() DECL_OVERRIDE {
        QUrl url;
        // cannot access to sub frame.
        url = GetBaseUrl();
        return url;
    }

    bool ForbidToOverlap() DECL_OVERRIDE {
        return false;
    }

    bool CanGoBack() DECL_OVERRIDE {
        return page() ? page()->history()->canGoBack() : false;
    }
    bool CanGoForward() DECL_OVERRIDE {
        return page() ? page()->history()->canGoForward() : false;
    }
#if QT_VERSION >= 0x050700
    bool RecentlyAudible() DECL_OVERRIDE {
        return page() ? page()->recentlyAudible() : false;
    }
    bool IsAudioMuted() DECL_OVERRIDE {
        return page() ? page()->isAudioMuted() : false;
    }
    void SetAudioMuted(bool muted) DECL_OVERRIDE {
        if(page()) page()->setAudioMuted(muted);
    }
#endif

    bool IsRenderable() DECL_OVERRIDE {
        return page() != 0 && (visible() || !m_GrabedDisplayData.isNull());
    }
    void Render(QPainter *painter) DECL_OVERRIDE {
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
    void Render(QPainter *painter, const QRegion &clip) DECL_OVERRIDE {
        // to investigate.
        //QTransform transform;
        //transform.scale(zoomFactor(), zoomFactor());
        //render(painter, QPoint(), (clip * transform).intersected(QRect(QPoint(), size())));
        if(visible()){
            QImage image(size(), QImage::Format_ARGB32);
            QPainter p(&image);
            render(&p);
            p.end();
            m_GrabedDisplayData = image;

            //render(painter, QPoint(), clip);
        }
        if(!m_GrabedDisplayData.isNull()){
            foreach(QRect rect, clip.rects()){
                painter->drawImage(rect, m_GrabedDisplayData.copy(rect));
            }
        }
    }
    QSize GetViewportSize() DECL_OVERRIDE {
        return size();
    }
    void SetViewportSize(QSize size) DECL_OVERRIDE {
        if(!visible()) resize(size);
    }
    void SetSource(const QUrl &url) DECL_OVERRIDE {
        if(page()) page()->SetSource(url);
    }
    void SetSource(const QByteArray &html) DECL_OVERRIDE {
        if(page()) page()->SetSource(html);
    }
    void SetSource(const QString &html) DECL_OVERRIDE {
        if(page()) page()->SetSource(html);
    }

    QString GetTitle() DECL_OVERRIDE {
        return title();
    }
    QIcon GetIcon() DECL_OVERRIDE {
#if QT_VERSION >= 0x050700
        return page() ? page()->icon() : QIcon();
#else
        return m_Icon;
#endif
    }

    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        return page() ? page()->Action(a, data) : 0;
    }

    void TriggerNativeLoadAction(const QUrl &url) DECL_OVERRIDE {
        load(url);
    }
    void TriggerNativeLoadAction(const QNetworkRequest &req,
                                 QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
                                 const QByteArray &body = QByteArray()) DECL_OVERRIDE {
        Q_UNUSED(operation);
        Q_UNUSED(body);
        load(req.url());
    }
    void TriggerNativeGoBackAction() DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebEnginePage::Back);
    }
    void TriggerNativeGoForwardAction() DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebEnginePage::Forward);
    }
    void TriggerNativeRewindAction() DECL_OVERRIDE {
        QWebEngineHistory *h = history();
        h->goToItem(h->itemAt(0));
    }
    void TriggerNativeFastForwardAction() DECL_OVERRIDE {
        QWebEngineHistory *h = history();
        h->goToItem(h->itemAt(h->count()-1));
    }

    void UpKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollTop-=40;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void DownKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollTop+=40;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void RightKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollLeft+=40;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void LeftKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollLeft-=40;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void PageDownKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollTop+=document.documentElement.clientHeight*0.9;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void PageUpKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollTop-=document.documentElement.clientHeight*0.9;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void HomeKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript(QStringLiteral("(function(){ document.body.scrollTop=0;})()"),
                              [this](QVariant){ EmitScrollChanged();});
    }
    void EndKeyEvent() DECL_OVERRIDE {
        page()->runJavaScript
            (QStringLiteral("(function(){\n"
                          VV"    document.body.scrollTop = \n"
                          VV"        (document.documentElement.scrollHeight - \n"
                          VV"         document.documentElement.clientHeight);\n"
                          VV"})()"),
             [this](QVariant){ EmitScrollChanged();});
    }

    void KeyPressEvent(QKeyEvent *ev) DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE { keyReleaseEvent(ev);}
    void MousePressEvent(QMouseEvent *ev) DECL_OVERRIDE { mousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE { mouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) DECL_OVERRIDE {
        QWheelEvent *newev = new QWheelEvent(ev->pos(),
                                             ev->delta()/Application::WheelScrollRate(),
                                             ev->buttons(),
                                             ev->modifiers(),
                                             ev->orientation());
        wheelEvent(newev);
        ev->setAccepted(true);
        delete newev;
    }

    void CallWithGotBaseUrl(UrlCallBack callBack) DECL_OVERRIDE;
    void CallWithGotCurrentBaseUrl(UrlCallBack callBack) DECL_OVERRIDE;
    void CallWithFoundElements(Page::FindElementsOption option, WebElementListCallBack callBack) DECL_OVERRIDE;
    void CallWithHitElement(const QPoint &pos, WebElementCallBack callBack) DECL_OVERRIDE;
    void CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack) DECL_OVERRIDE;
    void CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack) DECL_OVERRIDE;
    void CallWithSelectedText(StringCallBack callBack) DECL_OVERRIDE;
    void CallWithSelectedHtml(StringCallBack callBack) DECL_OVERRIDE;
    void CallWithWholeText(StringCallBack callBack) DECL_OVERRIDE;
    void CallWithWholeHtml(StringCallBack callBack) DECL_OVERRIDE;
    void CallWithSelectionRegion(RegionCallBack callBack) DECL_OVERRIDE;
    void CallWithEvaluatedJavaScriptResult(const QString &code, VariantCallBack callBack) DECL_OVERRIDE;
#ifdef PASSWORD_MANAGER
    bool PreventAuthRegistration(){ return m_PreventAuthRegistration;}
#endif

public slots:
    QSize size() DECL_OVERRIDE { return base()->size();}
    void resize(QSize size) DECL_OVERRIDE {
        if(TreeBank::PurgeView()){
            MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : 0;
            base()->setGeometry(win ? win->geometry() : QRect(QPoint(), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
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

        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        // set only notifier.
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
    void repaint() DECL_OVERRIDE { base()->repaint();}
    bool visible() DECL_OVERRIDE { return base()->isVisible();}
    void setFocus(Qt::FocusReason reason = Qt::OtherFocusReason) DECL_OVERRIDE {
        base()->setFocus(reason);
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

    void EmitScrollChanged() DECL_OVERRIDE;
    void EmitScrollChangedIfNeed() DECL_OVERRIDE;

    void CallWithScroll(PointFCallBack callBack);
    void SetScrollBarState() DECL_OVERRIDE;
    QPointF GetScroll() DECL_OVERRIDE;
    void SetScroll(QPointF pos) DECL_OVERRIDE;
    bool SaveScroll() DECL_OVERRIDE;
    bool RestoreScroll() DECL_OVERRIDE;
    bool SaveZoom() DECL_OVERRIDE;
    bool RestoreZoom() DECL_OVERRIDE;
    bool SaveHistory() DECL_OVERRIDE;
    bool RestoreHistory() DECL_OVERRIDE;

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    void SetFocusToElement(QString);
    void FireClickEvent(QString, QPoint);
    void SetTextValue(QString, QString);
    void AssignInspector();
#if QT_VERSION >= 0x050700
    void OnIconChanged(const QIcon &icon);
#else
    void UpdateIcon(const QUrl &iconUrl);
#endif

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
    void AddBookmarklet(QPoint pos)  DECL_OVERRIDE;

signals:
#if QT_VERSION >= 0x050700
    void iconChanged(const QIcon&);
#endif
    void statusBarMessage(const QString&);
    void statusBarMessage2(const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

protected:
    void childEvent(QChildEvent *ev) DECL_OVERRIDE;
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
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) DECL_OVERRIDE;
#endif

private:
#if QT_VERSION < 0x050700
    QIcon m_Icon;
#endif
    QImage m_GrabedDisplayData;
    static QMap<View*, QUrl> m_InspectorTable;
    QWebEngineView *m_Inspector;
    bool m_PreventScrollRestoration;
#ifdef PASSWORD_MANAGER
    bool m_PreventAuthRegistration;
#endif

    friend class EventEater;
};

class EventEater : public QObject{
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
    bool eventFilter(QObject *obj, QEvent *ev) DECL_OVERRIDE {
        QWidget *widget = qobject_cast<QWidget*>(obj);

        switch(ev->type()){
        case QEvent::KeyPress:{
            QKeyEvent *ke = static_cast<QKeyEvent*>(ev);

            if(m_View->GetDisplayObscured()){
                if(ke->key() == Qt::Key_Escape || ke->key() == Qt::Key_F11){
                    m_View->ExitFullScreen();
                    return true;
                }
            }

#ifdef PASSWORD_MANAGER
            if(ke->modifiers() & Qt::ControlModifier &&
               ke->key() == Qt::Key_Return){

                QString data = Application::GetAuthData
                    (m_View->page()->profile()->storageName() +
                     QStringLiteral(":") + m_View->url().host());

                if(!data.isEmpty()){
                    m_View->m_PreventAuthRegistration = true;
                    m_View->page()->runJavaScript
                        (View::SubmitFormDataJsCode(data),
                         [this](QVariant){
                            m_View->m_PreventAuthRegistration = false;
                        });
                    return true;
                }
            }
#endif //ifdef PASSWORD_MANAGER

            if(Application::HasAnyModifier(ke) ||
               Application::IsFunctionKey(ke)){
                return m_View->TriggerKeyEvent(ke);
            }

            int k = ke->key();
            if(!m_View->m_PreventScrollRestoration &&
               (k == Qt::Key_Space ||
                k == Qt::Key_Up ||
                k == Qt::Key_Down ||
                k == Qt::Key_Right ||
                k == Qt::Key_Left ||
                k == Qt::Key_PageUp ||
                k == Qt::Key_PageDown ||
                k == Qt::Key_Home ||
                k == Qt::Key_End)){

                m_View->m_PreventScrollRestoration = true;
            }
            return false;
        }
        case QEvent::KeyRelease:{
            QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
            int k = ke->key();
            int delay = m_View->page()->settings()->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled)
                ? 500
                : 100;
            if(k == Qt::Key_Space ||
               k == Qt::Key_Up ||
               k == Qt::Key_Down ||
               k == Qt::Key_Right ||
               k == Qt::Key_Left ||
               k == Qt::Key_PageUp ||
               k == Qt::Key_PageDown ||
               k == Qt::Key_Home ||
               k == Qt::Key_End){

                QTimer::singleShot(delay, m_View, &WebEngineView::EmitScrollChangedIfNeed);
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
                            MapToView(widget, me->localPos()),
                            me->screenPos(),
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
            }
            break;
        }
        case QEvent::Wheel:{
            QWheelEvent *we = static_cast<QWheelEvent*>(ev);
#if QT_VERSION >= 0x050700
            QString wheel;
            bool up = we->delta() > 0;
            Application::AddModifiersToString(wheel, we->modifiers());
            Application::AddMouseButtonsToString(wheel, we->buttons());
            Application::AddWheelDirectionToString(wheel, up);
            if(m_View->m_MouseMap.contains(wheel)){
                QString str = m_View->m_MouseMap[wheel];
                if(!str.isEmpty()){
                    m_View->GestureAborted();
                    m_View->View::TriggerAction
                        (str, MapToView(widget, we->pos()));
                }
                return true;
            }
#endif
            QWheelEvent we_ =
                QWheelEvent(MapToView(widget, we->pos()),
                            we->delta(),
                            we->buttons(),
                            we->modifiers(),
                            we->orientation());
            m_View->wheelEvent(&we_);
            return we_.isAccepted();
        }
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    WebEngineView *m_View;
};

#endif //ifndef WEBENGINEVIEW_HPP
