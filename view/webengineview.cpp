#include "switch.hpp"
#include "const.hpp"

#include "webengineview.hpp"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineHistory>
#include <QNetworkRequest>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QTextDocument>
#include <QClipboard>
#include <QTimer>
#include <QStyle>
#include <QKeySequence>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

namespace {
    inline QPoint ToPoint(QPoint p){
        return p;
    }

    inline QSize ToSize(QSize s){
        return s;
    }

    template <class T>
    QPoint LocalPos(T *t){
        return t->pos();
    }

    template <class T>
    QPoint GlobalPos(T *t){
        return t->globalPos();
    }
}

QMap<View*, QUrl> WebEngineView::m_InspectorTable = QMap<View*, QUrl>();

WebEngineView::WebEngineView(TreeBank *parent, QString id, QStringList set)
    : View(parent, id, set)
    , QWebEngineView(TreeBank::PurgeView() ? 0 : static_cast<QWidget*>(parent))
{
    Initialize();
    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebEnginePage(nam, this);
    ApplySpecificSettings(set);
    setPage(page());

    if(TreeBank::PurgeView()){
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    } else {
        if(parent) setParent(parent);
    }
    setMouseTracking(true);
    m_Inspector = 0;
    m_PreventScrollRestoration = false;
#ifdef PASSWORD_MANAGER
    m_PreventAuthRegistration = false;
#endif
#if QT_VERSION >= 0x050700
    connect(this, SIGNAL(iconChanged(const QIcon&)),
            this, SLOT(OnIconChanged(const QIcon&)));
#else
    m_Icon = QIcon();
    connect(this, SIGNAL(iconUrlChanged(const QUrl&)),
            this, SLOT(UpdateIcon(const QUrl&)));
#endif

    setAcceptDrops(true);
}

WebEngineView::~WebEngineView(){
    setPage(0);
    m_InspectorTable.remove(this);
    if(m_Inspector) m_Inspector->deleteLater();
}

QWebEngineView *WebEngineView::base(){
    return static_cast<QWebEngineView*>(this);
}

WebEnginePage *WebEngineView::page(){
    return static_cast<WebEnginePage*>(View::page());
}

QUrl WebEngineView::url(){
    return base()->url();
}

QString WebEngineView::html(){
    return WholeHtml();
}

TreeBank *WebEngineView::parent(){
    return m_TreeBank;
}

void WebEngineView::setUrl(const QUrl &url){
    base()->setUrl(url);
    emit urlChanged(url);
}

void WebEngineView::setHtml(const QString &html, const QUrl &url){
    base()->setHtml(html, url);
    emit urlChanged(url);
}

void WebEngineView::setParent(TreeBank* tb){
    View::SetTreeBank(tb);
    if(!TreeBank::PurgeView()) base()->setParent(tb);
    if(page()) page()->AddJsObject();
    if(tb) resize(size());
}

void WebEngineView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(this, SIGNAL(titleChanged(const QString&)),
            tb->parent(), SLOT(SetWindowTitle(const QString&)));
    //connect(this, SIGNAL(loadProgress(int)),
    //        this, SLOT(RestoreScroll()));
    if(Notifier *notifier = tb->GetNotifier()){
        connect(this, SIGNAL(statusBarMessage(const QString&)),
                notifier, SLOT(SetStatus(const QString&)));
        connect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(page(), SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
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

void WebEngineView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(this, SIGNAL(titleChanged(const QString&)),
               tb->parent(), SLOT(SetWindowTitle(const QString&)));
    //disconnect(this, SIGNAL(loadProgress(int)),
    //           this, SLOT(RestoreScroll()));
    if(Notifier *notifier = tb->GetNotifier()){
        disconnect(this, SIGNAL(statusBarMessage(const QString&)),
                   notifier, SLOT(SetStatus(const QString&)));
        disconnect(this, SIGNAL(statusBarMessage2(const QString&, const QString&)),
                   notifier, SLOT(SetStatus(const QString&, const QString&)));
        disconnect(page(), SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
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

void WebEngineView::OnSetViewNode(ViewNode*){}

void WebEngineView::OnSetHistNode(HistNode*){}

void WebEngineView::OnSetThis(WeakView){}

void WebEngineView::OnSetMaster(WeakView){}

void WebEngineView::OnSetSlave(WeakView){}

void WebEngineView::OnSetJsObject(_View*){}

void WebEngineView::OnSetJsObject(_Vanilla*){}

void WebEngineView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    if(history()->count()){
        QUrl historyUrl = history()->currentItem().url();
        if(!historyUrl.isEmpty() && historyUrl != url()){
            emit urlChanged(historyUrl);
        }
    }
    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;
    AssignInspector();

#if QT_VERSION < 0x050700
    if(m_Icon.isNull() && url() != QUrl(QStringLiteral("about:blank")))
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
#endif
}

void WebEngineView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void WebEngineView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

    if(history()->count()){
        QUrl historyUrl = history()->currentItem().url();
        if(!historyUrl.isEmpty() && historyUrl != url()){
            emit urlChanged(historyUrl);
        }
    }
    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

    AssignInspector();

#ifdef PASSWORD_MANAGER
    QString data = Application::GetAuthDataWithNoDialog
        (page()->profile()->storageName() +
         QStringLiteral(":") + url().host());

    if(!data.isEmpty()){
        page()->runJavaScript(DecorateFormFieldJsCode(data));
    }
#endif //ifdef PASSWORD_MANAGER

    if(visible() && m_TreeBank &&
       m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
        UpdateThumbnail();
    }
}

void WebEngineView::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void WebEngineView::OnUrlChanged(const QUrl &url){
    if(!GetHistNode()) return;
    SaveHistory();
    ChangeNodeUrl(url);
}

void WebEngineView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void WebEngineView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void WebEngineView::EmitScrollChanged(){
    if(!page()) return;
    CallWithScroll([this](QPointF pos){ emit ScrollChanged(pos);});
}

void WebEngineView::EmitScrollChangedIfNeed(){
    if(!page()) return;
    EmitScrollChanged();
}

void WebEngineView::CallWithScroll(PointFCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        // sometime cause crash.
        //callBack(QPointF(0.5f, 0.5f));
        return;
    }
    page()->runJavaScript
        (GetScrollRatioPointJsCode(), [callBack](QVariant var){
            if(!var.isValid()) return callBack(QPointF(0.5f, 0.5f));
            QVariantList list = var.toList();
            callBack(QPointF(list[0].toFloat(),list[1].toFloat()));
        });
}

void WebEngineView::SetScrollBarState(){
    if(!page()) return;
    page()->runJavaScript
        (GetScrollBarStateJsCode(), [this](QVariant var){
            if(!var.isValid()) return;
            QVariantList list = var.toList();
            int hmax = list[0].toInt();
            int vmax = list[1].toInt();
            if(hmax < 0) hmax = 0;
            if(vmax < 0) vmax = 0;
            if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
            else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
            else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
            else             m_ScrollBarState = NoScrollBarEnabled;
        });
}

QPointF WebEngineView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    // this function does not return actual value on WebEngineView.
    return QPointF(0.5f, 0.5f);
}

void WebEngineView::SetScroll(QPointF pos){
    if(!page()) return;
    page()->runJavaScript
        (SetScrollRatioPointJsCode(pos), [this](QVariant){
            EmitScrollChanged();
        });
}

bool WebEngineView::SaveScroll(){
    if(!page()) return false;
    page()->runJavaScript
        (GetScrollValuePointJsCode(), [this](QVariant var){
            if(!var.isValid() || !GetHistNode()) return;
            QVariantList list = var.toList();
            GetHistNode()->SetScrollX(list[0].toInt());
            GetHistNode()->SetScrollY(list[1].toInt());
        });
    return true;
}

bool WebEngineView::RestoreScroll(){
    if(!page() || !GetHistNode()) return false;
    if(m_PreventScrollRestoration) return false;
    QPoint pos = QPoint(GetHistNode()->GetScrollX(),
                        GetHistNode()->GetScrollY());
    page()->runJavaScript(SetScrollValuePointJsCode(pos));
    return true;
}

bool WebEngineView::SaveZoom(){
    if(!GetHistNode()) return false;
    GetHistNode()->SetZoom(zoomFactor());
    return true;
}

bool WebEngineView::RestoreZoom(){
    if(!GetHistNode()) return false;
    setZoomFactor(static_cast<qreal>(GetHistNode()->GetZoom()));
    return true;
}

bool WebEngineView::SaveHistory(){
    if(!GetHistNode()) return false;
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << (*history());
    if(!ba.isEmpty()){
        GetHistNode()->SetHistoryData(ba);
        return true;
    }
    return false;
}

bool WebEngineView::RestoreHistory(){
    if(!GetHistNode()) return false;
    QByteArray ba = GetHistNode()->GetHistoryData();
    if(!ba.isEmpty()){
        QDataStream stream(&ba, QIODevice::ReadOnly);
        stream >> (*history());
        return history()->count() > 0;
    }
    return false;
}

void WebEngineView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool WebEngineView::SeekText(const QString &str, View::FindFlags opt){
    QWebEnginePage::FindFlags flags = 0;
    if(opt & FindBackward)    flags |= QWebEnginePage::FindBackward;
    if(opt & CaseSensitively) flags |= QWebEnginePage::FindCaseSensitively;

    bool ret = true; // dummy value.
    QWebEngineView::findText
        (str, flags, [this](bool){
            EmitScrollChangedIfNeed();
        });
    return ret;
}

void WebEngineView::SetFocusToElement(QString xpath){
    page()->runJavaScript(SetFocusToElementJsCode(xpath));
}

void WebEngineView::FireClickEvent(QString xpath, QPoint pos){
    page()->runJavaScript(FireClickEventJsCode(xpath, pos/zoomFactor()));
}

void WebEngineView::SetTextValue(QString xpath, QString text){
    page()->runJavaScript(SetTextValueJsCode(xpath, text));
}

void WebEngineView::AssignInspector(){
    if(m_InspectorTable.contains(this)) return;

    QString addr = QStringLiteral("http://localhost:%1").arg(Application::RemoteDebuggingPort());
    QNetworkRequest req(QUrl(addr + QStringLiteral("/json")));
    DownloadItem *item = NetworkController::Download
        (static_cast<NetworkAccessManager*>(page()->networkAccessManager()),
         req, NetworkController::ToVariable);

    if(!item) return;

    item->setParent(base());

    connect(item, &DownloadItem::DownloadResult, [this, addr](const QByteArray &result){

            foreach(QJsonValue value, QJsonDocument::fromJson(result).array()){

                QString debuggeeValue = value.toObject()["url"].toString();
                QString debuggerValue = value.toObject()["devtoolsFrontendUrl"].toString();

                if(debuggeeValue.isEmpty() || debuggerValue.isEmpty()) break;

                QUrl debuggee = QUrl(debuggeeValue);
                QUrl debugger = QUrl(addr + debuggerValue);

                if(url() == debuggee && !m_InspectorTable.values().contains(debugger)){
                    m_InspectorTable[this] = debugger;
                    break;
                }
            }
        });
}

#if QT_VERSION >= 0x050700
void WebEngineView::OnIconChanged(const QIcon &icon){
    Application::RegisterIcon(url().host(), icon);
}
#else
void WebEngineView::UpdateIcon(const QUrl &iconUrl){
    m_Icon = QIcon();
    if(!page()) return;
    QString host = url().host();
    QNetworkRequest req(iconUrl);
    DownloadItem *item = NetworkController::Download
        (static_cast<NetworkAccessManager*>(page()->networkAccessManager()),
         req, NetworkController::ToVariable);

    if(!item) return;

    item->setParent(base());

    connect(item, &DownloadItem::DownloadResult, [this, host](const QByteArray &result){
            QPixmap pixmap;
            if(pixmap.loadFromData(result)){
                QIcon icon = QIcon(pixmap);
                Application::RegisterIcon(host, icon);
                if(url().host() == host) m_Icon = icon;
            }
        });
}
#endif

void WebEngineView::Copy(){
    if(page()) page()->triggerAction(QWebEnginePage::Copy);
}

void WebEngineView::Cut(){
    if(page()) page()->triggerAction(QWebEnginePage::Cut);
}

void WebEngineView::Paste(){
    if(page()) page()->triggerAction(QWebEnginePage::Paste);
}

void WebEngineView::Undo(){
    if(page()) page()->triggerAction(QWebEnginePage::Undo);
}

void WebEngineView::Redo(){
    if(page()) page()->triggerAction(QWebEnginePage::Redo);
}

void WebEngineView::SelectAll(){
    if(page()) page()->triggerAction(QWebEnginePage::SelectAll);
}

void WebEngineView::Unselect(){
#if QT_VERSION >= 0x050700
    if(page()) page()->triggerAction(QWebEnginePage::Unselect);
#else
    EvaluateJavaScript(QStringLiteral(
                           "(function(){\n"
                           VV"    document.activeElement.blur();\n"
                           VV"    getSelection().removeAllRanges();\n"
                           VV"}());"));
#endif
}

void WebEngineView::Reload(){
    if(page()) page()->triggerAction(QWebEnginePage::Reload);
}

void WebEngineView::ReloadAndBypassCache(){
    if(page()) page()->triggerAction(QWebEnginePage::ReloadAndBypassCache);
}

void WebEngineView::Stop(){
    if(page()) page()->triggerAction(QWebEnginePage::Stop);
}

void WebEngineView::StopAndUnselect(){
    Stop(); Unselect();
}

void WebEngineView::Print(){
    // not yet implemented.
}

void WebEngineView::Save(){
#if QT_VERSION >= 0x050700
    if(page()) page()->triggerAction(QWebEnginePage::SavePage);
#endif
}

void WebEngineView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebEngineView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebEngineView::ExitFullScreen(){
    if(page()) page()->triggerAction(QWebEnginePage::ExitFullScreen);
}

void WebEngineView::InspectElement(){
    if(!m_Inspector){
        m_Inspector = new QWebEngineView();
        m_Inspector->setAttribute(Qt::WA_DeleteOnClose, false);
        m_Inspector->load(m_InspectorTable[this]);
    } else {
        m_Inspector->reload();
    }
    m_Inspector->show();
    m_Inspector->raise();
    //if(page()) page()->InspectElement();
}

void WebEngineView::AddSearchEngine(QPoint pos){
    if(page()) page()->AddSearchEngine(pos);
}

void WebEngineView::AddBookmarklet(QPoint pos){
    if(page()) page()->AddBookmarklet(pos);
}

void WebEngineView::childEvent(QChildEvent *ev){
    QWebEngineView::childEvent(ev);
    if(ev->added() &&
       0 == strcmp(ev->child()->metaObject()->className(),
                   "QtWebEngineCore::RenderWidgetHostViewQtDelegateWidget")){
        ev->child()->installEventFilter(new EventEater(this, ev->child()));
    }
}

void WebEngineView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QWebEngineView::hideEvent(ev);
}

void WebEngineView::showEvent(QShowEvent *ev){
    m_PreventScrollRestoration = false;
    QWebEngineView::showEvent(ev);
    RestoreViewState();
}

void WebEngineView::keyPressEvent(QKeyEvent *ev){
    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QWebEngineView::keyPressEvent(ev);

    if(!ev->isAccepted() &&
       !Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void WebEngineView::keyReleaseEvent(QKeyEvent *ev){
    QWebEngineView::keyReleaseEvent(ev);

    int k = ev->key();

    if(page()->settings()->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled) &&
       (k == Qt::Key_Space ||
      //k == Qt::Key_Up ||
      //k == Qt::Key_Down ||
      //k == Qt::Key_Right ||
      //k == Qt::Key_Left ||
        k == Qt::Key_PageUp ||
        k == Qt::Key_PageDown ||
        k == Qt::Key_Home ||
        k == Qt::Key_End)){

        for(int i = 1; i < 6; i++){
            QTimer::singleShot(i*200, this, &WebEngineView::EmitScrollChangedIfNeed);
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void WebEngineView::resizeEvent(QResizeEvent *ev){
    QWebEngineView::resizeEvent(ev);
}

void WebEngineView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void WebEngineView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QWebEngineView::mouseMoveEvent(ev);
        ev->setAccepted(true);
        return;
    }
    if(ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(LocalPos(ev));
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

        if(QLineF(LocalPos(ev), m_GestureStartedPos).length() < 2){
            // gesture not aborted.
            QWebEngineView::mouseMoveEvent(ev);
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
            QWebEngineView::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = LocalPos(ev) - rect.topLeft();

        if(pixmap.width()  > MAX_DRAGGING_PIXMAP_WIDTH ||
           pixmap.height() > MAX_DRAGGING_PIXMAP_HEIGHT){

            pos /= qMax(static_cast<float>(pixmap.width()) /
                        static_cast<float>(MAX_DRAGGING_PIXMAP_WIDTH),
                        static_cast<float>(pixmap.height()) /
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
            GestureMoved(LocalPos(ev));
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
        QWebEngineView::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void WebEngineView::mousePressEvent(QMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, LocalPos(ev))){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(LocalPos(ev));
    QWebEngineView::mousePressEvent(ev);
    ev->setAccepted(false);
}

void WebEngineView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MidButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(Application::keyboardModifiers() & Qt::ShiftModifier ||
           Application::keyboardModifiers() & Qt::ControlModifier ||
           ev->button() == Qt::MidButton){

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
            GestureFinished(LocalPos(ev), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            page()->DisplayContextMenu(m_TreeBank, elem, LocalPos(ev), GlobalPos(ev));
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QWebEngineView::mouseReleaseEvent(ev);
    EmitScrollChangedIfNeed();
    ev->setAccepted(false);
}

void WebEngineView::mouseDoubleClickEvent(QMouseEvent *ev){
    QWebEngineView::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void WebEngineView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QWebEngineView::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void WebEngineView::dragMoveEvent(QDragMoveEvent *ev){
    if(m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(LocalPos(ev));
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_LeftGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_LeftGestureMap[gesture])
              ? Action(Page::StringToAction(m_LeftGestureMap[gesture]))->text()
            : m_LeftGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QWebEngineView::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void WebEngineView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    QPoint pos = LocalPos(ev);
    QList<QUrl> urls = ev->mimeData()->urls();
    QObject *source = ev->source();
    QWidget *widget = this;
    QString text;
    if(!ev->mimeData()->text().isEmpty()){
        text = ev->mimeData()->text().replace(QStringLiteral("\""), QStringLiteral("\\\""));
    } else if(!urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    CallWithHitElement(pos, [this, pos, urls, text, source, widget](SharedWebElement elem){

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        elem->SetText(text);
        return;
    }

    if(!m_Gesture.isEmpty() && source == widget){
        GestureFinished(pos, Qt::LeftButton);
        return;
    }

    GestureAborted();

    if(urls.isEmpty() || source == widget){
        // do nothing.
    } else if(qobject_cast<TreeBank*>(source) || dynamic_cast<View*>(source)){
        QList<QUrl> filtered;
        foreach(QUrl u, urls){ if(!u.isLocalFile()) filtered << u;}
        m_TreeBank->OpenInNewViewNode(filtered, true, GetViewNode());
        return;
    } else {
        // foreign drag.
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }

    });

    QWebEngineView::dropEvent(ev);
    ev->setAccepted(true);
}

void WebEngineView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QWebEngineView::dragLeaveEvent(ev);
}

void WebEngineView::wheelEvent(QWheelEvent *ev){
    // wheel event is called twice on Qt5.7.
    // senders are EventEater and QWebEngineView.
#if QT_VERSION < 0x050700
    QString wheel;
    bool up = ev->delta() > 0;

    Application::AddModifiersToString(wheel, ev->modifiers());
    Application::AddMouseButtonsToString(wheel, ev->buttons());
    Application::AddWheelDirectionToString(wheel, up);

    if(m_MouseMap.contains(wheel)){

        QString str = m_MouseMap[wheel];
        if(!str.isEmpty()){
            GestureAborted();
            View::TriggerAction(str, LocalPos(ev));
        }
        ev->setAccepted(true);

    } else
#endif
    {
        m_PreventScrollRestoration = true;
        ev->setAccepted(false);
    }

    if(page()->settings()->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled)){
        for(int i = 1; i < 6; i++){
            QTimer::singleShot(i*200, this, &WebEngineView::EmitScrollChangedIfNeed);
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void WebEngineView::focusInEvent(QFocusEvent *ev){
    QWebEngineView::focusInEvent(ev);
    OnFocusIn();
}

void WebEngineView::focusOutEvent(QFocusEvent *ev){
    QWebEngineView::focusOutEvent(ev);
    OnFocusOut();
}

bool WebEngineView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QWebEngineView::focusNextPrevChild(next);
    return false;
}

#if defined(Q_OS_WIN)
bool WebEngineView::nativeEvent(const QByteArray &eventType, void *message, long *result){
    bool ret = QWebEngineView::nativeEvent(eventType, message, result);
    if(TreeBank::PurgeView() && eventType == "windows_generic_MSG"){
        if(static_cast<MSG*>(message)->message == WM_WINDOWPOSCHANGED){
            MainWindow *win;
            if(m_TreeBank) win = m_TreeBank->GetMainWindow();
            if(!m_TreeBank) return ret;
            if(win) win->RaiseAllEdgeWidgets();
            if(m_TreeBank->GetNotifier()) m_TreeBank->GetNotifier()->raise();
            if(m_TreeBank->GetReceiver()) m_TreeBank->GetReceiver()->raise();
        }
    }
    return ret;
}
#endif

void WebEngineView::CallWithGotBaseUrl(UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")))
        return callBack(QUrl());

    page()->runJavaScript
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    // this implementation is same as baseurl...
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")))
        return callBack(QUrl());

    page()->runJavaScript
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithFoundElements(Page::FindElementsOption option,
                                          WebElementListCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")))
        return callBack(SharedWebElementList());

    page()->runJavaScript
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

void WebEngineView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")) || pos.isNull())
        return callBack(SharedWebElement());

    page()->runJavaScript
        (HitElementJsCode(pos/zoomFactor()), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElement());
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void WebEngineView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")) || pos.isNull())
        return callBack(QUrl());

    page()->runJavaScript
        (HitLinkUrlJsCode(pos/zoomFactor()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")) || pos.isNull())
        return callBack(QUrl());

    page()->runJavaScript
        (HitImageUrlJsCode(pos/zoomFactor()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithSelectedText(StringCallBack callBack){
    if(page()) callBack(page()->selectedText());
}

void WebEngineView::CallWithSelectedHtml(StringCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")))
        return callBack(QString());

    page()->runJavaScript
        (SelectedHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void WebEngineView::CallWithWholeText(StringCallBack callBack){
    if(page()) page()->toPlainText(callBack);
}

void WebEngineView::CallWithWholeHtml(StringCallBack callBack){
    if(page()) page()->toHtml(callBack);
}

void WebEngineView::CallWithSelectionRegion(RegionCallBack callBack){
    if(!page() || !hasSelection()) return callBack(QRegion());
    QRect viewport = QRect(QPoint(), size());
    page()->runJavaScript
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

void WebEngineView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                      VariantCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")))
        return callBack(QVariant());

    page()->runJavaScript(code, callBack);
}

