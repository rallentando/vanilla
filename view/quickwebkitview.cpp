#include "switch.hpp"
#include "const.hpp"

#ifdef WEBKITVIEW

#include "quickwebkitview.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>

#include "view.hpp"
#include "webkitpage.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

#include <memory>

QuickWebKitView::QuickWebKitView(TreeBank *parent, QString id, QStringList set)
    : QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebkitview.qml")), parent)
    , View(parent, id, set)
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlWebKitView = rootObject();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebKitPage(nam, this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_ActionTable = QMap<Page::CustomAction, QAction*>();
    m_RequestId = 0;

    connect(m_QmlWebKitView, SIGNAL(callBackResult(int, QVariant)),
            this,            SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlWebKitView, SIGNAL(viewChanged()),
            this,            SIGNAL(ViewChanged()));
    connect(m_QmlWebKitView, SIGNAL(scrollChanged(QPointF)),
            this,            SIGNAL(ScrollChanged(QPointF)));
}

QuickWebKitView::~QuickWebKitView(){
}

void QuickWebKitView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    if(!page()) return;

    SetPreference(QWebSettings::AutoLoadImages,                    "AutoLoadImages");
    SetPreference(QWebSettings::CaretBrowsingEnabled,              "CaretBrowsingEnabled");
    SetPreference(QWebSettings::DeveloperExtrasEnabled,            "DeveloperExtrasEnabled");
    SetPreference(QWebSettings::DnsPrefetchEnabled,                "DnsPrefetchEnabled");
    SetPreference(QWebSettings::FrameFlatteningEnabled,            "FrameFlatteningEnabled");
    SetPreference(QWebSettings::JavascriptEnabled,                 "JavascriptEnabled");
    SetPreference(QWebSettings::LocalStorageEnabled,               "LocalStorageEnabled");
    SetPreference(QWebSettings::NotificationsEnabled,              "NotificationsEnabled");
    SetPreference(QWebSettings::OfflineWebApplicationCacheEnabled, "OfflineWebApplicationCacheEnabled");
    SetPreference(QWebSettings::PluginsEnabled,                    "PluginsEnabled");
    SetPreference(QWebSettings::PrivateBrowsingEnabled,            "PrivateBrowsingEnabled");
    SetPreference(QWebSettings::LocalContentCanAccessFileUrls,     "LocalContentCanAccessFileUrls");
    SetPreference(QWebSettings::LocalContentCanAccessRemoteUrls,   "LocalContentCanAccessRemoteUrls");
    SetPreference(QWebSettings::XSSAuditingEnabled,                "XSSAuditingEnabled");
    SetPreference(QWebSettings::WebAudioEnabled,                   "WebAudioEnabled");
    SetPreference(QWebSettings::WebGLEnabled,                      "WebGLEnabled");

    // for only QuickWebEngineView.
    SetPreference(QWebSettings::HyperlinkAuditingEnabled,          "HyperlinkAuditingEnabled");
    SetPreference(QWebSettings::JavascriptCanAccessClipboard,      "JavascriptCanAccessClipboard");
    SetPreference(QWebSettings::JavascriptCanOpenWindows,          "JavascriptCanOpenWindows");
    SetPreference(QWebSettings::LinksIncludedInFocusChain,         "LinksIncludedInFocusChain");

    SetFontFamily(QWebSettings::StandardFont,  "StandardFont");
    SetFontFamily(QWebSettings::FixedFont,     "FixedFont");
    SetFontFamily(QWebSettings::SerifFont,     "SerifFont");
    SetFontFamily(QWebSettings::SansSerifFont, "SansSerifFont");
    SetFontFamily(QWebSettings::CursiveFont,   "CursiveFont");
    SetFontFamily(QWebSettings::FantasyFont,   "FantasyFont");

    SetFontSize(QWebSettings::MinimumFontSize,        "MinimumFontSize");
    SetFontSize(QWebSettings::MinimumLogicalFontSize, "MinimumLogicalFontSize");
    SetFontSize(QWebSettings::DefaultFontSize,        "DefaultFontSize");
    SetFontSize(QWebSettings::DefaultFixedFontSize,   "DefaultFixedFontSize");

    QMetaObject::invokeMethod(m_QmlWebKitView, "setUserAgent",
                              Q_ARG(QVariant, QVariant::fromValue(page()->userAgentForUrl(QUrl()))));
    QMetaObject::invokeMethod(m_QmlWebKitView, "setDefaultTextEncoding",
                              Q_ARG(QVariant, QVariant::fromValue(page()->settings()->defaultTextEncoding())));
}

QQuickWidget *QuickWebKitView::base(){
    return static_cast<QQuickWidget*>(this);
}

WebKitPage *QuickWebKitView::page(){
    return static_cast<WebKitPage*>(View::page());
}

QUrl QuickWebKitView::url(){
    return m_QmlWebKitView->property("url").toUrl();
}

QString QuickWebKitView::html(){
    return WholeHtml();
}

TreeBank *QuickWebKitView::parent(){
    return m_TreeBank;
}

void QuickWebKitView::setUrl(const QUrl &url){
    m_QmlWebKitView->setProperty("url", url);
    emit urlChanged(url);
}

void QuickWebKitView::setHtml(const QString &html, const QUrl &url){
    QMetaObject::invokeMethod(m_QmlWebKitView, "loadHtml",
                              Q_ARG(QString, html),
                              Q_ARG(QUrl,    url));
    emit urlChanged(url);
}

void QuickWebKitView::setParent(TreeBank* t){
    View::SetTreeBank(t);
    base()->setParent(t);
}

void QuickWebKitView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(m_QmlWebKitView, SIGNAL(titleChanged_(const QString&)),
            tb->parent(), SLOT(setWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        connect(this, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(m_QmlWebKitView, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(m_QmlWebKitView, SIGNAL(linkHovered_(const QString&, const QString&, const QString&)),
                notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        connect(this, SIGNAL(ScrollChanged(QPointF)),
                notifier, SLOT(SetScroll(QPointF)));
        connect(notifier, SIGNAL(ScrollRequest(QPointF)),
                this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        connect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                this, SLOT(Load(const QString&)));
        connect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                this, SLOT(SeekText(const QString&, View::FindFlags)));
        connect(receiver, SIGNAL(KeyEvent(QString)),
                this, SLOT(KeyEvent(QString)));

        connect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                page(), SLOT(DownloadSuggest(const QUrl&)));
        connect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void QuickWebKitView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(m_QmlWebKitView, SIGNAL(titleChanged_(const QString&)),
               tb->parent(), SLOT(setWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                   notifier, SLOT(SetStatus(const QString&, const QString&)));
        disconnect(m_QmlWebKitView, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(m_QmlWebKitView, SIGNAL(linkHovered_(const QString&, const QString&, const QString&)),
                   notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        disconnect(this, SIGNAL(ScrollChanged(QPointF)),
                   notifier, SLOT(SetScroll(QPointF)));
        disconnect(notifier, SIGNAL(ScrollRequest(QPointF)),
                   this, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        disconnect(receiver, SIGNAL(OpenBookmarklet(const QString&)),
                   this, SLOT(Load(const QString&)));
        disconnect(receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
                   this, SLOT(SeekText(const QString&, View::FindFlags)));
        disconnect(receiver, SIGNAL(KeyEvent(QString)),
                   this, SLOT(KeyEvent(QString)));

        disconnect(receiver, SIGNAL(SuggestRequest(const QUrl&)),
                   page(), SLOT(DownloadSuggest(const QUrl&)));
        disconnect(page(), SIGNAL(SuggestResult(const QByteArray&)),
                   receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

QAction *QuickWebKitView::Action(Page::CustomAction a, QVariant data){
    QAction *action = m_ActionTable[a];

    if(action) return action;

    if(a == Page::_Reload ||
       a == Page::_Stop ||
       a == Page::_Back ||
       a == Page::_Forward){

        m_ActionTable[a] = action = new QAction(this);

        switch(a){
        case Page::_Reload:
            action->setText(tr("Reload"));
            connect(action, SIGNAL(triggered()), m_QmlWebKitView, SLOT(reload()));
            break;
        case Page::_Stop:
            action->setText(tr("Stop"));
            connect(action, SIGNAL(triggered()), m_QmlWebKitView, SLOT(stop()));
            break;
        case Page::_Back:
            action->setText(tr("Back"));
            connect(action, SIGNAL(triggered()), m_QmlWebKitView, SLOT(goBack()));
            break;
        case Page::_Forward:
            action->setText(tr("Forward"));
            connect(action, SIGNAL(triggered()), m_QmlWebKitView, SLOT(goForward()));
            break;
        default: break;
        }
        action->setData(data);
        return action;
    }
    return page()->Action(a, data);
}

void QuickWebKitView::OnSetViewNode(ViewNode*){}

void QuickWebKitView::OnSetHistNode(HistNode*){}

void QuickWebKitView::OnSetThis(WeakView){}

void QuickWebKitView::OnSetMaster(WeakView){}

void QuickWebKitView::OnSetSlave(WeakView){}

void QuickWebKitView::OnSetJsObject(_View*){}

void QuickWebKitView::OnSetJsObject(_Vanilla*){}

void QuickWebKitView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
}

void QuickWebKitView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    // loadProgress: 100% signal is emitted after loadFinished.
    if(progress != 100)
        emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void QuickWebKitView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

}

void QuickWebKitView::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void QuickWebKitView::OnUrlChanged(const QUrl &url){
    if(!GetHistNode()) return;
    ChangeNodeUrl(url);
}

void QuickWebKitView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickWebKitView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void QuickWebKitView::EmitScrollChanged(){
    /*
    if(!m_ScrollSignalTimer)
        m_ScrollSignalTimer = startTimer(200);
    */
}

void QuickWebKitView::CallWithScroll(PointFCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebKitView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result.toPointF());
                });

    QMetaObject::invokeMethod(m_QmlWebKitView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(GetScrollRatioPointJsCode())));
}

void QuickWebKitView::SetScrollBarState(){
    m_ScrollBarState = NoScrollBarEnabled;
}

QPointF QuickWebKitView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    // this function does not return actual value on WebEngineView.
    return QPointF(0.5f, 0.5f);
}

void QuickWebKitView::SetScroll(QPointF pos){
    QMetaObject::invokeMethod(m_QmlWebKitView, "setScroll",
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

bool QuickWebKitView::SaveScroll(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebKitView, "saveScroll");
    return true;
}

bool QuickWebKitView::RestoreScroll(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebKitView, "restoreScroll");
    return true;
}

bool QuickWebKitView::SaveZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebKitView, "saveZoom");
    return true;
}

bool QuickWebKitView::RestoreZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebKitView, "restoreZoom");
    return true;
}

void QuickWebKitView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickWebKitView::SeekText(const QString &str, View::FindFlags opt){
    QMetaObject::invokeMethod(m_QmlWebKitView, "seekText",
                              Q_ARG(QVariant, QVariant::fromValue(str)),
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<int>(opt))));
    return true;
}

void QuickWebKitView::SetFocusToElement(QString xpath){
    QMetaObject::invokeMethod(m_QmlWebKitView, "setFocusToElement",
                              Q_ARG(QVariant, QVariant::fromValue(xpath)));
}

void QuickWebKitView::FireClickEvent(QString xpath, QPoint pos){
    QMetaObject::invokeMethod(m_QmlWebKitView, "fireClickEvent",
                              Q_ARG(QVariant, QVariant::fromValue(xpath)),
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

void QuickWebKitView::SetTextValue(QString xpath, QString text){
    QMetaObject::invokeMethod(m_QmlWebKitView, "setTextValue",
                              Q_ARG(QVariant, QVariant::fromValue(xpath)),
                              Q_ARG(QVariant, QVariant::fromValue(text)));
}

void QuickWebKitView::Copy(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "copy");
}

void QuickWebKitView::Cut(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "cut");
}

void QuickWebKitView::Paste(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "paste");
}

void QuickWebKitView::Undo(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "undo");
}

void QuickWebKitView::Redo(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "redo");
}

void QuickWebKitView::SelectAll(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "selectAll");
}

void QuickWebKitView::Unselect(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "unselect");
}

void QuickWebKitView::Reload(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "reload");
}

void QuickWebKitView::ReloadAndBypassCache(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "reloadAndBypassCache");
}

void QuickWebKitView::Stop(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "stop");
}

void QuickWebKitView::StopAndUnselect(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "stopAndUnselect");
}

void QuickWebKitView::Print(){

    QString filename = ModalDialog::GetSaveFileName_
        (QString(), QString(),
         QStringLiteral("Pdf document (*.pdf);;Images (*.jpg *.jpeg *.gif *.png *.bmp *.xpm)"));

    if(filename.isEmpty()) return;

    if(filename.toLower().endsWith(".pdf")){

        QMetaObject::invokeMethod(m_QmlWebKitView, "printToPdf",
                                  Q_ARG(QString, filename));
    } else {
        QSize origSize = size();
        QPointF origPos = m_QmlWebKitView->property("scrollPosition").toPointF();
        QSizeF contentsSize = m_QmlWebKitView->property("contentsSize").toSizeF();
        resize(contentsSize.toSize());

        QTimer::singleShot(700, this, [this, filename, origSize, origPos](){

        grabFramebuffer().save(filename);

        resize(origSize);
        CallWithEvaluatedJavaScriptResult
            (SetScrollValuePointJsCode(origPos.toPoint()), [](QVariant){});

        });
    }
}

void QuickWebKitView::Save(){
    QMetaObject::invokeMethod(m_QmlWebKitView, "save");
}

void QuickWebKitView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    m_QmlWebKitView->setProperty("zoomFactor", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebKitView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    m_QmlWebKitView->setProperty("zoomFactor", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebKitView::hideEvent(QHideEvent *ev){
    SaveViewState();
    QQuickWidget::hideEvent(ev);
}

void QuickWebKitView::showEvent(QShowEvent *ev){
    QQuickWidget::showEvent(ev);
    RestoreViewState();
}

void QuickWebKitView::keyPressEvent(QKeyEvent *ev){
    if(!visible()) return;

    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.

    // 'HasAnyModifier' ignores ShiftModifier.
    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QQuickWidget::keyPressEvent(ev);

    if(!ev->isAccepted() &&
       !Application::IsOnlyModifier(ev)){

        if(NavigationBySpaceKey() &&
           ev->key() == Qt::Key_Space){

            if(ev->modifiers() & Qt::ShiftModifier){
                GetTreeBank()->Back(GetHistNode());
            } else {
                GetTreeBank()->Forward(GetHistNode());
            }
            ev->setAccepted(true);
            return;
        }

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void QuickWebKitView::keyReleaseEvent(QKeyEvent *ev){
    if(!visible()) return;

    QQuickWidget::keyReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebKitView, "emitScrollChangedIfNeed");
}

void QuickWebKitView::resizeEvent(QResizeEvent *ev){
    QQuickWidget::resizeEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebKitView, "adjustContents");
}

void QuickWebKitView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickWebKitView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
        return;
    }
    if(ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_RightGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_RightGestureMap[gesture])
              ? Action(Page::StringToAction(m_RightGestureMap[gesture]))->text()
            : m_RightGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
        ev->setAccepted(false);
        return;
    }

    int scrollBarWidth = Application::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    bool horizontal = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == HorizontalScrollBarEnabled;
    bool vertical   = m_ScrollBarState == BothScrollBarEnabled
        ||            m_ScrollBarState == VerticalScrollBarEnabled;
    QRect touchableRect =
        QRect(QPoint(),
              size() - QSize(vertical   ? scrollBarWidth : 0,
                             horizontal ? scrollBarWidth : 0));

    if(ev->buttons() & Qt::LeftButton &&
       !m_GestureStartedPos.isNull() &&
       touchableRect.contains(m_GestureStartedPos) &&
       (m_ClickedElement &&
        !m_ClickedElement->IsNull() &&
        !m_ClickedElement->IsEditableElement())){

        if(QLineF(ev->pos(), m_GestureStartedPos).length() < 2){
            // gesture not aborted.
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QDrag *drag = new QDrag(this);

        // clear or make directory if need.
        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam =
            static_cast<NetworkAccessManager*>(page()->networkAccessManager());

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

            // call default behavior.
            GestureAborted();
            QQuickWidget::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = ev->pos() - rect.topLeft();

        if(pixmap.size().width()  > MAX_DRAGGING_PIXMAP_WIDTH ||
           pixmap.size().height() > MAX_DRAGGING_PIXMAP_HEIGHT){

            pos /= qMax(static_cast<float>(pixmap.size().width()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_WIDTH),
                        static_cast<float>(pixmap.size().height()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_HEIGHT));
            pixmap = pixmap.scaled(MAX_DRAGGING_PIXMAP_WIDTH,
                                   MAX_DRAGGING_PIXMAP_HEIGHT,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }

        if(m_HadSelection){
            mime->setImageData(pixmap.toImage());
        } else {
            QPixmap element = m_ClickedElement->Pixmap();
            if(element.isNull())
                mime->setImageData(pixmap.toImage());
            else
                mime->setImageData(element.toImage());
        }
        if(m_EnableDragHackLocal){
            GestureMoved(ev->pos());
        } else {
            GestureAborted();
        }
        m_DragStarted = true;
        drag->setMimeData(mime);
        drag->setPixmap(pixmap);
        drag->setHotSpot(pos);
        drag->exec(Qt::CopyAction | Qt::MoveAction);
        drag->deleteLater();
        ev->setAccepted(true);
    } else {
        // call default behavior.
        GestureAborted();
        QQuickWidget::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void QuickWebKitView::mousePressEvent(QMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, ev->pos())){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(ev->pos());
    QQuickWidget::mousePressEvent(ev);
    ev->setAccepted(true);
}

void QuickWebKitView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MidButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(ev->modifiers() & Qt::ShiftModifier
           || ev->modifiers() & Qt::ControlModifier
#if defined(Q_OS_MAC)
           || ev->modifiers() & Qt::MetaModifier
#endif
           || ev->button() == Qt::MidButton){

            GestureAborted();
            m_TreeBank->OpenInNewViewNode(req, Page::Activate(), GetViewNode());
            ev->setAccepted(true);
            return;

        } else if(
            // it's requirements of starting loadhack.
            // loadhack uses new hist node instead of same view's `load()'.
            m_EnableLoadHackLocal
            // url is not empty.
            && !url().isEmpty()
            // link doesn't hold jump command.
            && !link.toEncoded().contains("#")
            // m_ClickedElement doesn't hold javascript function.
            && !m_ClickedElement->IsJsCommandElement()){

            GestureAborted();
            m_TreeBank->OpenInNewHistNode(req, true, GetHistNode());
            ev->setAccepted(true);
            return;
        }
    }

    if(ev->button() == Qt::RightButton){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos());
        }
        ev->setAccepted(true);
        return;
    }
    if(ev->button() == Qt::LeftButton &&
       m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else {
            GestureAborted();
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QQuickWidget::mouseReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebKitView, "emitScrollChangedIfNeed");
    ev->setAccepted(true);
}

void QuickWebKitView::mouseDoubleClickEvent(QMouseEvent *ev){
    QQuickWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickWebKitView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QQuickWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickWebKitView::dragMoveEvent(QDragMoveEvent *ev){
    if(m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_LeftGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_LeftGestureMap[gesture])
              ? Action(Page::StringToAction(m_LeftGestureMap[gesture]))->text()
            : m_LeftGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QQuickWidget::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void QuickWebKitView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    bool isLocal = false;
    QPoint pos = ev->pos();
    QObject *source = ev->source();
    QQuickWindow *window = quickWindow();
    QString text = ev->mimeData()->text();
    QList<QUrl> urls = Page::MimeDataToUrls(ev->mimeData(), source);

    foreach(QUrl u, urls){ if(u.isLocalFile()) isLocal = true;}

    if(!text.isEmpty()){
        text.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    } else if(!urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    CallWithHitElement(pos, [this, pos, source, window, text, urls](SharedWebElement elem){

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        elem->SetText(text);
        return;
    }

    if(!m_Gesture.isEmpty() && source == this){
        GestureFinished(pos, Qt::LeftButton);
        return;
    }

    GestureAborted();

    if(!urls.isEmpty() && source != this && source != window){
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }

    });

    if(isLocal ||
       (DragToStartDownload() && !urls.isEmpty() && source == this))
        ; // do nothing.
    else
        QQuickWidget::dropEvent(ev);
    ev->setAccepted(true);
}

void QuickWebKitView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QQuickWidget::dragLeaveEvent(ev);
}

void QuickWebKitView::wheelEvent(QWheelEvent *ev){
    if(!visible()) return;

    QString wheel;
    bool up = ev->delta() > 0;

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(m_MouseMap.contains(wheel)){

        QString str = m_MouseMap[wheel];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, ev->pos()))
                qDebug() << "Invalid mouse event: " << str;
        }
        ev->setAccepted(true);

    } else {
        QQuickWidget::wheelEvent(ev);
        ev->setAccepted(true);
    }
    QMetaObject::invokeMethod(m_QmlWebKitView, "emitScrollChangedIfNeed");
}

void QuickWebKitView::focusInEvent(QFocusEvent *ev){
    QQuickWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickWebKitView::focusOutEvent(QFocusEvent *ev){
    QQuickWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickWebKitView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QQuickWidget::focusNextPrevChild(next);
    return false;
}

void QuickWebKitView::CallWithGotBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebKitView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetCurrentBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebKitView::CallWithFoundElements(Page::FindElementsOption option,
                                            WebElementListCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (FindElementsJsCode(option), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElementList());
            QVariantList list = var.toMap().values();
            SharedWebElementList result;

            MainWindow *win = Application::GetCurrentWindow();
            QSize s =
                m_TreeBank ? m_TreeBank->size() :
                win ? win->GetTreeBank()->size() :
                !size().isEmpty() ? size() :
                DEFAULT_WINDOW_SIZE;
            QRect viewport = QRect(QPoint(), s);

            for(int i = 0; i < list.length(); i++){
                std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
                *e = JsWebElement(this, list[i]);
                if(!viewport.intersects(e->Rectangle()))
                    e->SetRectangle(QRect());
                result << e;
            }
            callBack(result);
        });
}

void QuickWebKitView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(pos.isNull()) return callBack(SharedWebElement());
    CallWithEvaluatedJavaScriptResult
        (HitElementJsCode(pos / m_HistNode->GetZoom()), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElement());
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void QuickWebKitView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitLinkUrlJsCode(pos / m_HistNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebKitView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitImageUrlJsCode(pos / m_HistNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebKitView::CallWithSelectedText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebKitView::CallWithSelectedHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebKitView::CallWithWholeText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebKitView::CallWithWholeHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebKitView::CallWithSelectionRegion(RegionCallBack callBack){
    QRect viewport = QRect(QPoint(), size());
    CallWithEvaluatedJavaScriptResult
        (SelectionRegionJsCode(), [viewport, callBack](QVariant var){
            if(!var.isValid() || !var.canConvert(QMetaType::QVariantMap))
                return callBack(QRegion());
            QRegion region;
            QVariantMap map = var.toMap();
            foreach(QString key, map.keys()){
                QVariantMap m = map[key].toMap();
                region |= QRect(m["x"].toInt(),
                                m["y"].toInt(),
                                m["width"].toInt(),
                                m["height"].toInt()).intersected(viewport);
            }
            callBack(region);
        });
}

void QuickWebKitView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                        VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebKitView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebKitView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

#endif //ifdef WEBKITVIEW
