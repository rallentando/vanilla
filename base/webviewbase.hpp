#ifndef WEBVIEWBASE_HPP
#define WEBVIEWBASE_HPP

#include "switch.hpp"

//[[!WEV]]
#ifdef QTWEBKIT
//[[/!WEV]]

#include "webpagebase.hpp"
#include "view.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "networkcontroller.hpp"
#include "mainwindow.hpp"

#include <stdlib.h>

#include <QWebViewBase>
#include <QWebPageBase>
#include <QWebHistoryBase>

//[[!WEV]]
#include <QWebFrameBase>
#include <QWebElementBase>
//[[/!WEV]]

class QKeySequence;
//[[WEV]]
class EventEater;
//[[/WEV]]

class WebViewBase : public QWebViewBase, public View{
    Q_OBJECT

public:
    WebViewBase(TreeBank *parent = 0, QString id = "", QStringList set = QStringList());
    ~WebViewBase();

    QWebViewBase *base() DECL_OVERRIDE {
        return static_cast<QWebViewBase*>(this);
    }
    WebPageBase *page() DECL_OVERRIDE {
        return static_cast<WebPageBase*>(View::page());
    }

    QUrl      url() DECL_OVERRIDE { return base()->url();}
    void   setUrl(const QUrl &url) DECL_OVERRIDE {
        base()->setUrl(url);
        emit urlChanged(url);
    }

    QString   html() DECL_OVERRIDE { return WholeHtml();}
    void   setHtml(const QString &html, const QUrl &url) DECL_OVERRIDE {
        base()->setHtml(html, url);
        emit urlChanged(url);
    }

    TreeBank *parent() DECL_OVERRIDE { return m_TreeBank;}
    void   setParent(TreeBank* tb) DECL_OVERRIDE {
        View::SetTreeBank(tb);
        //[[GWV]]
        if(base()->scene()) base()->scene()->removeItem(this);
        if(page()) page()->AddJsObject();
        if(tb) tb->GetScene()->addItem(this);
        //[[/GWV]]
        //[[!GWV]]
        if(!TreeBank::PurgeView()) base()->setParent(tb);
        if(page()) page()->AddJsObject();
        if(tb) resize(size());
        //[[/!GWV]]
    }

    void Connect(TreeBank *tb) DECL_OVERRIDE;
    void Disconnect(TreeBank *tb) DECL_OVERRIDE;

    void ZoomIn() DECL_OVERRIDE;
    void ZoomOut() DECL_OVERRIDE;

    QUrl BaseUrl() DECL_OVERRIDE {
        //[[!WEV]]
        return page() ? page()->mainFrame()->baseUrl() : QUrl();
        //[[/!WEV]]
        //[[WEV]]
        return GetBaseUrl();
        //[[/WEV]]
    }
    QUrl CurrentBaseUrl() DECL_OVERRIDE {
        QUrl url;
        //[[!WEV]]
        if(page() && page()->currentFrame())
            url = page()->currentFrame()->baseUrl();
        if(!url.isEmpty() && page() && page()->mainFrame())
            url = page()->mainFrame()->baseUrl();
        //[[/!WEV]]
        //[[WEV]]
        // cannot access to sub frame.
        url = GetBaseUrl();
        //[[/WEV]]
        return url;
    }

    bool ForbidToOverlap() DECL_OVERRIDE {
        //[[!WEV]]
        // for QTBUG-28854 or QTBUG-33053 or QTBUG-42588 or QTBUG-43024 or QTBUG-44401.
        // https://bugreports.qt.io/browse/QTBUG-28854
        // https://bugreports.qt.io/browse/QTBUG-33053
        // https://bugreports.qt.io/browse/QTBUG-42588
        // https://bugreports.qt.io/browse/QTBUG-43024
        // https://bugreports.qt.io/browse/QTBUG-44401
        return (url().toString().toLower().endsWith(QStringLiteral(".swf")) ||
                url().toString().toLower().endsWith(QStringLiteral(".pdf")));
        //[[/!WEV]]
        //[[WEV]]
        return false;
        //[[/WEV]]
    }

    bool CanGoBack() DECL_OVERRIDE {
        return page() ? page()->history()->canGoBack() : false;
    }
    bool CanGoForward() DECL_OVERRIDE {
        return page() ? page()->history()->canGoForward() : false;
    }

    //[[!WEV]]
    void Print() DECL_OVERRIDE { if(page()) page()->Print();}
    void AddSearchEngine(QPoint pos) DECL_OVERRIDE { if(page()) page()->AddSearchEngine(pos);}
    void AddBookmarklet(QPoint pos)  DECL_OVERRIDE { if(page()) page()->AddBookmarklet(pos);}
    void InspectElement()            DECL_OVERRIDE { if(page()) page()->InspectElement();}
    void ReloadAndBypassCache()      DECL_OVERRIDE { if(page()) page()->ReloadAndBypassCache();}
    //[[/!WEV]]
    //[[WEV]]
#if QT_VERSION >= 0x050600
    void InspectElement()            DECL_OVERRIDE {
        if(!m_Inspector){
            m_Inspector = new QWebViewBase();
            m_Inspector->setAttribute(Qt::WA_DeleteOnClose, false);
            m_Inspector->load(m_InspectorTable[this]);
        } else {
            m_Inspector->reload();
        }
        m_Inspector->show();
        m_Inspector->raise();
        //if(page()) page()->InspectElement();
    }
#endif
    void AddSearchEngine(QPoint pos) DECL_OVERRIDE { if(page()) page()->AddSearchEngine(pos);}
    void AddBookmarklet(QPoint pos)  DECL_OVERRIDE { if(page()) page()->AddBookmarklet(pos);}
    //[[/WEV]]

    bool IsRenderable() DECL_OVERRIDE {
        //[[!WEV]]
        return page() != 0;
        //[[/!WEV]]
        //[[WEV]]
        return page() != 0 && (visible() || !m_GrabedDisplayData.isNull());
        //[[/WEV]]
    }
    void Render(QPainter *painter) DECL_OVERRIDE {
        //[[!WEV]]
        if(page()) page()->mainFrame()->render(painter);
        //[[/!WEV]]
        //[[WEV]]
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
        //[[/WEV]]
    }
    void Render(QPainter *painter, const QRegion &clip) DECL_OVERRIDE {
        //[[!WEV]]
        if(page()) page()->mainFrame()->render(painter, clip);
        //[[/!WEV]]
        //[[WEV]]
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
        //[[/WEV]]
    }
    QSize GetViewportSize() DECL_OVERRIDE {
        //[[!WEV]]
        return page() ? page()->viewportSize() : QSize();
        //[[/!WEV]]
        //[[WEV]]
        return size();
        //[[/WEV]]
    }
    void SetViewportSize(QSize size) DECL_OVERRIDE {
        //[[!WEV]]
        if(page()) page()->setViewportSize(size);
        //[[/!WEV]]
        //[[WEV]]
        if(!visible()) resize(size);
        //[[/WEV]]
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
        return title();
    }
    QIcon GetIcon() DECL_OVERRIDE {
        //[[WEV]]
        return m_Icon;
        //[[/WEV]]
        //[[!WEV]]
        return icon();
        //[[/!WEV]]
    }

    void TriggerAction(QWebPageBase::WebAction a) DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a);
    }
    void TriggerAction(Page::CustomAction a, QVariant data = QVariant()) DECL_OVERRIDE {
        if(page()) page()->TriggerAction(a, data);
    }
    QAction *Action(QWebPageBase::WebAction a) DECL_OVERRIDE {
        return page() ? page()->Action(a) : 0;
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
        //[[!WEV]]
        load(req, operation, body);
        //[[/!WEV]]
        //[[WEV]]
        Q_UNUSED(operation);
        Q_UNUSED(body);
        load(req.url());
        //[[/WEV]]
    }
    void TriggerNativeGoBackAction() DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebPageBase::Back);
    }
    void TriggerNativeGoForwardAction() DECL_OVERRIDE {
        if(page()) page()->triggerAction(QWebPageBase::Forward);
    }

    //[[!WEV]]
    void UpKeyEvent()       DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_UpKey);       EmitScrollChanged();}
    void DownKeyEvent()     DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_DownKey);     EmitScrollChanged();}
    void RightKeyEvent()    DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_RightKey);    EmitScrollChanged();}
    void LeftKeyEvent()     DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_LeftKey);     EmitScrollChanged();}
    void PageDownKeyEvent() DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_PageDownKey); EmitScrollChanged();}
    void PageUpKeyEvent()   DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_PageUpKey);   EmitScrollChanged();}
    void HomeKeyEvent()     DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_HomeKey);     EmitScrollChanged();}
    void EndKeyEvent()      DECL_OVERRIDE { QWebViewBase::keyPressEvent(m_EndKey);      EmitScrollChanged();}
    //[[/!WEV]]
    //[[WEV]]
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
    //[[/WEV]]

    void KeyPressEvent(QKeyEvent *ev) DECL_OVERRIDE { keyPressEvent(ev);}
    void KeyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE { keyReleaseEvent(ev);}
    //[[GWV]]
    void MousePressEvent(QMouseEvent *ev) DECL_OVERRIDE { m_TreeBank->MousePressEvent(ev);}
    void MouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE { m_TreeBank->MouseReleaseEvent(ev);}
    void MouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE { m_TreeBank->MouseMoveEvent(ev);}
    void MouseDoubleClickEvent(QMouseEvent *ev) DECL_OVERRIDE { m_TreeBank->MouseDoubleClickEvent(ev);}
    void WheelEvent(QWheelEvent *ev) DECL_OVERRIDE { m_TreeBank->WheelEvent(ev);}
    //[[/GWV]]
    //[[!GWV]]
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
    //[[/!GWV]]

    //[[!WEV]]
    QUrl GetBaseUrl() DECL_OVERRIDE {
        return BaseUrl();
    }
    QUrl GetCurrentBaseUrl() DECL_OVERRIDE {
        return CurrentBaseUrl();
    }
    SharedWebElementList FindElements(Page::FindElementsOption option) DECL_OVERRIDE;
    SharedWebElement HitElement(const QPoint &pos) DECL_OVERRIDE;
    QUrl HitLinkUrl(const QPoint &pos) DECL_OVERRIDE {
        return page() ? page()->mainFrame()->hitTestContent(pos).linkUrl() : QUrl();
    }
    QUrl HitImageUrl(const QPoint &pos) DECL_OVERRIDE {
        return page() ? page()->mainFrame()->hitTestContent(pos).imageUrl() : QUrl();
    }
    QString SelectedText() DECL_OVERRIDE {
        return page() ? page()->selectedText() : QString();
    }
    QString SelectedHtml() DECL_OVERRIDE {
        return page() ? page()->selectedHtml() : QString();
    }
    QString WholeText() DECL_OVERRIDE {
        return page() ? page()->mainFrame()->toPlainText() : QString();
    }
    QString WholeHtml() DECL_OVERRIDE {
        return page() ? page()->mainFrame()->toHtml() : QString();
    }
    QRegion SelectionRegion() DECL_OVERRIDE {
        QRegion region;
        QVariant var = EvaluateJavaScript(SelectionRegionJsCode());
        if(!var.isValid() || !var.canConvert(QMetaType::QVariantMap)) return region;
        QVariantMap map = var.toMap();
        QRect viewport = QRect(QPoint(), size());
        foreach(QString key, map.keys()){
            QVariantMap m = map[key].toMap();
            region |= QRect(m["x"].toInt()*zoomFactor(),
                            m["y"].toInt()*zoomFactor(),
                            m["width"].toInt()*zoomFactor(),
                            m["height"].toInt()*zoomFactor()).intersected(viewport);
        }
        return region;
    }
    QVariant EvaluateJavaScript(const QString &code) DECL_OVERRIDE {
        return page() ? page()->mainFrame()->evaluateJavaScript(code) : QVariant();
    }
    //[[/!WEV]]
    //[[WEV]]
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
    //[[/WEV]]

public slots:
    QSize size() DECL_OVERRIDE {
        //[[GWV]]
        return base()->size().toSize();
        //[[/GWV]]
        //[[!GWV]]
        return base()->size();
        //[[/!GWV]]
    }
    void resize(QSize size) DECL_OVERRIDE {
        //[[GWV]]
        base()->resize(size);
        //[[/GWV]]
        //[[!GWV]]
        if(TreeBank::PurgeView()){
            MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : 0;
            base()->setGeometry(win ? win->geometry() : QRect(QPoint(), size));
        } else {
            base()->setGeometry(QRect(QPoint(), size));
        }
        //[[/!GWV]]
    }
    void show() DECL_OVERRIDE {
        base()->show();
        if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
        if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();

        //[[WEV]]
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
        //[[/WEV]]

        if(!m_TreeBank || !m_TreeBank->GetNotifier()) return;
        //[[!WEV]]
        // set only notifier.
        m_TreeBank->GetNotifier()->SetScroll(GetScroll());
        //[[/!WEV]]
        //[[WEV]]
        // set only notifier.
        CallWithScroll([this](QPointF pos){
                if(m_TreeBank){
                    if(Notifier *notifier = m_TreeBank->GetNotifier()){
                        notifier->SetScroll(pos);
                    }
                }
            });
        //[[/WEV]]
    }
    void hide() DECL_OVERRIDE {
        base()->hide();
        //[[!WEV]]
        // there is no sense to resize because plugins paint to window directly, but...
        if(ForbidToOverlap()) resize(QSize(0,0));
        //[[/!WEV]]
    }

    //[[GWV]]
    void raise()   DECL_OVERRIDE { setZValue(VIEW_CONTENTS_LAYER);}
    void lower()   DECL_OVERRIDE { setZValue(HIDDEN_CONTENTS_LAYER);}
    void repaint() DECL_OVERRIDE { update(boundingRect());}
    //[[/GWV]]
    //[[!GWV]]
    void raise()   DECL_OVERRIDE { base()->raise();}
    void lower()   DECL_OVERRIDE { base()->lower();}
    void repaint() DECL_OVERRIDE { base()->repaint();}
    //[[/!GWV]]

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
        //[[WEV]]
#if defined(Q_OS_WIN)
        if(TreeBank::TridentViewExist()) show(); // for force update.
#endif
        //[[/WEV]]
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

    //[[WEV]]
    void CallWithScroll(PointFCallBack callBack);
    //[[/WEV]]
    void SetScrollBarState() DECL_OVERRIDE;
    QPointF GetScroll() DECL_OVERRIDE;
    void SetScroll(QPointF pos) DECL_OVERRIDE;
    bool SaveScroll()     DECL_OVERRIDE;
    bool RestoreScroll()  DECL_OVERRIDE;
    bool SaveZoom()       DECL_OVERRIDE;
    bool RestoreZoom()    DECL_OVERRIDE;
    bool SaveHistory()    DECL_OVERRIDE;
    bool RestoreHistory() DECL_OVERRIDE;

    void KeyEvent(QString);
    bool SeekText(const QString&, View::FindFlags);

    //[[WEV]]
    void SetFocusToElement(QString);
    void FireClickEvent(QString, QPoint);
    void SetTextValue(QString, QString);
    void AssignInspector();
    void UpdateIcon(const QUrl &iconUrl);
    //[[/WEV]]

signals:
    //[[WEV]]
    void iconChanged();
    void statusBarMessage(const QString&);
    //[[/WEV]]
    void statusBarMessage2(const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);

private:
    //[[WEV]]
    QIcon m_Icon;
    QImage m_GrabedDisplayData;
    static QMap<View*, QUrl> m_InspectorTable;
    QWebViewBase *m_Inspector;
    bool m_PreventScrollRestoration;
    //[[/WEV]]

protected:
    //[[WEV]]
    void childEvent(QChildEvent *ev) DECL_OVERRIDE;
    //[[/WEV]]
    void hideEvent(QHideEvent *ev) DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void resizeEvent(QResizeEventBase *ev) DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEventBase *ev) DECL_OVERRIDE;
    //[[GWV]]
    void hoverMoveEvent(QHoverEventBase *ev) DECL_OVERRIDE;
    //[[/GWV]]
    void mouseMoveEvent(QMouseEventBase *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEventBase *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEventBase *ev) DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEventBase *ev) DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEventBase *ev) DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEventBase *ev) DECL_OVERRIDE;
    void dropEvent(QDropEventBase *ev) DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEventBase *ev) DECL_OVERRIDE;
    void wheelEvent(QWheelEventBase *ev) DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) DECL_OVERRIDE;
    //[[!GWV]]
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) DECL_OVERRIDE;
#endif
    //[[/!GWV]]

    //[[WEV]]
    friend class EventEater;
    //[[/WEV]]
};

//[[WEV]]
class EventEater : public QObject{
    Q_OBJECT

public:
    EventEater(WebViewBase *view, QObject *parent)
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

#if QT_VERSION >= 0x050600
            if(m_View->page()->ObscureDisplay()){
                if(ke->key() == Qt::Key_Escape || ke->key() == Qt::Key_F11){
                    m_View->page()->triggerAction(QWebEnginePage::ExitFullScreen);
                    return true;
                }
            }
#endif
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
            int delay = m_View->page()->settings()->testAttribute(QWebSettingsBase::ScrollAnimatorEnabled)
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

                QTimer::singleShot(delay, m_View, &WebViewBase::EmitScrollChangedIfNeed);
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
    WebViewBase *m_View;
};
//[[/WEV]]

//[[!WEV]]
#endif //ifdef QTWEBKIT
//[[/!WEV]]

#endif //ifndef WEBVIEWBASE_HPP
