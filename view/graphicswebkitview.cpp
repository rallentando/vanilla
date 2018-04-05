#include "switch.hpp"
#include "const.hpp"

#ifdef WEBKITVIEW

#include "graphicswebkitview.hpp"

#include <QGraphicsWebView>
#include <QWebPage>
#include <QWebHistory>
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

GraphicsWebKitView::GraphicsWebKitView(TreeBank *parent, QString id, QStringList set)
    : QGraphicsWebView(0)
    , View(parent, id, set)
{
    Initialize();
    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebKitPage(nam, this);
    ApplySpecificSettings(set);
    setPage(page());

    if(parent) setParent(parent);
    setAcceptHoverEvents(true);
    setAcceptDrops(true);
}

GraphicsWebKitView::~GraphicsWebKitView(){
    setPage(0);
}

QGraphicsWebView *GraphicsWebKitView::base(){
    return static_cast<QGraphicsWebView*>(this);
}

WebKitPage *GraphicsWebKitView::page(){
    return static_cast<WebKitPage*>(View::page());
}

QUrl GraphicsWebKitView::url(){
    return base()->url();
}

QString GraphicsWebKitView::html(){
    return WholeHtml();
}

TreeBank *GraphicsWebKitView::parent(){
    return m_TreeBank;
}

void GraphicsWebKitView::setUrl(const QUrl &url){
    base()->setUrl(url);
    emit urlChanged(url);
}

void GraphicsWebKitView::setHtml(const QString &html, const QUrl &url){
    base()->setHtml(html, url);
    emit urlChanged(url);
}

void GraphicsWebKitView::setParent(TreeBank* tb){
    View::SetTreeBank(tb);
    if(base()->scene()) base()->scene()->removeItem(this);
    if(page()) page()->AddJsObject();
    if(tb) tb->GetScene()->addItem(this);
}

void GraphicsWebKitView::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(this, SIGNAL(titleChanged(const QString&)),
            tb->parent(), SLOT(SetWindowTitle(const QString&)));
    connect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
            this, SLOT(RestoreScroll()));
    connect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
            this, SIGNAL(ViewChanged()));
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

void GraphicsWebKitView::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(this, SIGNAL(titleChanged(const QString&)),
               tb->parent(), SLOT(SetWindowTitle(const QString&)));
    disconnect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
               this, SLOT(RestoreScroll()));
    disconnect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
               this, SIGNAL(ViewChanged()));
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

void GraphicsWebKitView::OnSetViewNode(ViewNode*){}

void GraphicsWebKitView::OnSetHistNode(HistNode*){}

void GraphicsWebKitView::OnSetThis(WeakView){}

void GraphicsWebKitView::OnSetMaster(WeakView){}

void GraphicsWebKitView::OnSetSlave(WeakView){}

void GraphicsWebKitView::OnSetJsObject(_View*){}

void GraphicsWebKitView::OnSetJsObject(_Vanilla*){}

void GraphicsWebKitView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
}

void GraphicsWebKitView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void GraphicsWebKitView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

    if(history()->count()){
        QUrl historyUrl = history()->currentItem().url();
        if(!historyUrl.isEmpty()){
            emit urlChanged(historyUrl);
        } else if(!url().isEmpty()){
            emit urlChanged(url());
        }
    }
    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

    CallWithWholeHtml([this](QString html){

    QString data = html.toLower();

    if(html.startsWith(QStringLiteral("<?xml"))){
        if(// for xhtml ~version 4.
           data.contains(QStringLiteral("<!doctype")) ||
           data.contains(QStringLiteral("<script"))   ||
           data.contains(QStringLiteral("<html"))     ||
           data.contains(QStringLiteral("<head"))     ||
           data.contains(QStringLiteral("<iframe"))   ||
           data.contains(QStringLiteral("<h1"))       ||
           data.contains(QStringLiteral("<div"))      ||
           data.contains(QStringLiteral("<font"))     ||
           data.contains(QStringLiteral("<table"))    ||
           data.contains(QStringLiteral("<a"))        ||
           data.contains(QStringLiteral("<style"))    ||
           data.contains(QStringLiteral("<title"))    ||
           data.contains(QStringLiteral("<b"))        ||
           data.contains(QStringLiteral("<body"))     ||
           data.contains(QStringLiteral("<br"))       ||
           data.contains(QStringLiteral("<p"))        ||
           // for xhtml version 5,
           //ignore tag if its length is less than 4.
           data.contains(QStringLiteral(":script"))   ||
           data.contains(QStringLiteral(":html"))     ||
           data.contains(QStringLiteral(":head"))     ||
           data.contains(QStringLiteral(":iframe"))   ||
           data.contains(QStringLiteral(":font"))     ||
           data.contains(QStringLiteral(":table"))    ||
           data.contains(QStringLiteral(":style"))    ||
           data.contains(QStringLiteral(":title"))    ||
           data.contains(QStringLiteral(":body"))){
            /* do nothing for xhtml. */
        } else {
            SetSource(html);
        }
    }
    });

    if(visible() && m_TreeBank &&
       m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
        UpdateThumbnail();
    }
}

void GraphicsWebKitView::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void GraphicsWebKitView::OnUrlChanged(const QUrl &url){
    if(!GetHistNode()) return;
    QString before = GetHistNode()->GetUrl().toString().toLower();
    QString after  = url.toString().toLower();
    if(m_TreeBank && m_TreeBank->GetCurrentView().get() == this){
        if(!(before.endsWith(QStringLiteral(".pdf")) || before.endsWith(QStringLiteral(".swf"))) &&
           (after.endsWith(QStringLiteral(".pdf")) || after.endsWith(QStringLiteral(".swf"))))
            m_TreeBank->PurgeChildWidgetsIfNeed();

        if((before.endsWith(QStringLiteral(".pdf")) || before.endsWith(QStringLiteral(".swf"))) &&
           !(after.endsWith(QStringLiteral(".pdf")) || after.endsWith(QStringLiteral(".swf"))))
            m_TreeBank->JoinChildWidgetsIfNeed();
    }
    SaveHistory();
    ChangeNodeUrl(url);
}

void GraphicsWebKitView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void GraphicsWebKitView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void GraphicsWebKitView::EmitScrollChanged(){
    if(!page()) return;
    emit ScrollChanged(GetScroll());
}

void GraphicsWebKitView::EmitScrollChangedIfNeed(){
    if(!page()) return;
    if(GetHistNode()->GetScrollX() != page()->mainFrame()->scrollBarValue(Qt::Horizontal) ||
       GetHistNode()->GetScrollY() != page()->mainFrame()->scrollBarValue(Qt::Vertical))
        EmitScrollChanged();
}

void GraphicsWebKitView::SetScrollBarState(){
    if(!page()) return;
    int hmax = page()->mainFrame()->scrollBarMaximum(Qt::Horizontal);
    int vmax = page()->mainFrame()->scrollBarMaximum(Qt::Vertical);

    if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
    else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
    else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
    else             m_ScrollBarState = NoScrollBarEnabled;
}

QPointF GraphicsWebKitView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    float hval = static_cast<float>(page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    float hmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal));
    float vval = static_cast<float>(page()->mainFrame()->scrollBarValue(Qt::Vertical));
    float vmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    return QPointF(hmax == 0.0f ? 0.5f : hval / hmax,
                   vmax == 0.0f ? 0.5f : vval / vmax);
}

void GraphicsWebKitView::SetScroll(QPointF pos){
    if(!page()) return;
    float hmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal));
    float vmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    page()->mainFrame()->setScrollBarValue(Qt::Horizontal, static_cast<int>(hmax * pos.x()));
    page()->mainFrame()->setScrollBarValue(Qt::Vertical,   static_cast<int>(vmax * pos.y()));
    EmitScrollChanged();
}

bool GraphicsWebKitView::SaveScroll(){
    if(!page()) return false;
    if(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
        GetHistNode()->SetScrollX(page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    if(page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
        GetHistNode()->SetScrollY(page()->mainFrame()->scrollBarValue(Qt::Vertical));
    return true;
}

bool GraphicsWebKitView::RestoreScroll(){
    if(!page() || !GetHistNode()) return false;
    if(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
        page()->mainFrame()->setScrollBarValue(Qt::Horizontal, GetHistNode()->GetScrollX());
    if(page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
        page()->mainFrame()->setScrollBarValue(Qt::Vertical,   GetHistNode()->GetScrollY());
    return true;
}

bool GraphicsWebKitView::SaveZoom(){
    if(!GetHistNode()) return false;
    GetHistNode()->SetZoom(zoomFactor());
    return true;
}

bool GraphicsWebKitView::RestoreZoom(){
    if(!GetHistNode()) return false;
    setZoomFactor(static_cast<qreal>(GetHistNode()->GetZoom()));
    return true;
}

bool GraphicsWebKitView::SaveHistory(){
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

bool GraphicsWebKitView::RestoreHistory(){
    static const QByteArray header = QByteArray::fromHex("00000003000000020000000e0068006900730074006f00720079");
    if(!GetHistNode()) return false;
    QByteArray ba = GetHistNode()->GetHistoryData();
    if(!ba.isEmpty() && ba.startsWith(header)){
        QDataStream stream(&ba, QIODevice::ReadOnly);
        stream >> (*history());
        return history()->count() > 0;
    }
    return false;
}

void GraphicsWebKitView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool GraphicsWebKitView::SeekText(const QString &str, View::FindFlags opt){
    QWebPage::FindFlags flags = 0;
    if(opt & FindBackward)                      flags |= QWebPage::FindBackward;
    if(opt & CaseSensitively)                   flags |= QWebPage::FindCaseSensitively;
    if(opt & WrapsAroundDocument)               flags |= QWebPage::FindWrapsAroundDocument;
    if(opt & HighlightAllOccurrences)           flags |= QWebPage::HighlightAllOccurrences;
    if(opt & FindAtWordBeginningsOnly)          flags |= QWebPage::FindAtWordBeginningsOnly;
    if(opt & TreatMedialCapitalAsWordBeginning) flags |= QWebPage::TreatMedialCapitalAsWordBeginning;
    if(opt & FindBeginsInSelection)             flags |= QWebPage::FindBeginsInSelection;

    // for deleting previous seek.
    if(!str.isEmpty() && str.toLower() != page()->selectedText().toLower()){
        QGraphicsWebView::findText(QString(), flags);
    }
    bool ret = QGraphicsWebView::findText(str, flags);
    EmitScrollChangedIfNeed();
    return ret;
}

void GraphicsWebKitView::OnIconChanged(const QIcon &icon){
    Application::RegisterIcon(url().host(), icon);
}

void GraphicsWebKitView::Copy(){
    if(page()) page()->triggerAction(QWebPage::Copy);
}

void GraphicsWebKitView::Cut(){
    if(page()) page()->triggerAction(QWebPage::Cut);
}

void GraphicsWebKitView::Paste(){
    if(page()) page()->triggerAction(QWebPage::Paste);
}

void GraphicsWebKitView::Undo(){
    if(page()) page()->triggerAction(QWebPage::Undo);
}

void GraphicsWebKitView::Redo(){
    if(page()) page()->triggerAction(QWebPage::Redo);
}

void GraphicsWebKitView::SelectAll(){
    if(page()) page()->triggerAction(QWebPage::SelectAll);
}

void GraphicsWebKitView::Unselect(){
    if(page()){
        page()->triggerAction(QWebPage::Unselect);
        page()->mainFrame()->evaluateJavaScript(QStringLiteral(
            "(function(){\n"
          VV"    document.activeElement.blur();\n"
          VV"}());"));
    }
}

void GraphicsWebKitView::Reload(){
    if(page()) page()->triggerAction(QWebPage::Reload);
}

void GraphicsWebKitView::ReloadAndBypassCache(){
    if(page()) page()->triggerAction(QWebPage::ReloadAndBypassCache);
}

void GraphicsWebKitView::Stop(){
    if(page()) page()->triggerAction(QWebPage::Stop);
}

void GraphicsWebKitView::StopAndUnselect(){
    Stop(); Unselect();
}

void GraphicsWebKitView::Print(){
    if(page()) page()->Print();
}

void GraphicsWebKitView::Save(){
    if(page()) page()->Download(url(), url());
}

void GraphicsWebKitView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void GraphicsWebKitView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void GraphicsWebKitView::ToggleMediaControls(){
    if(page())
        page()->triggerAction(QWebPage::ToggleMediaControls);
}

void GraphicsWebKitView::ToggleMediaLoop(){
    if(page())
         page()->triggerAction(QWebPage::ToggleMediaLoop);
}

void GraphicsWebKitView::ToggleMediaPlayPause(){
    if(page())
        page()->triggerAction(QWebPage::ToggleMediaPlayPause);
}

void GraphicsWebKitView::ToggleMediaMute(){
    if(page())
        page()->triggerAction(QWebPage::ToggleMediaMute);
}

void GraphicsWebKitView::ExitFullScreen(){
    if(page()){
        // this doesn't work...
        //page()->triggerAction(QWebEnginePage::ToggleVideoFullscreen);
        if(TreeBank *tb = GetTreeBank())
            tb->GetMainWindow()->SetFullScreen(false);
        SetDisplayObscured(false);
    }
}

void GraphicsWebKitView::InspectElement(){
    if(page()) page()->InspectElement();
}

void GraphicsWebKitView::AddSearchEngine(QPoint pos){
    if(page()) page()->AddSearchEngine(pos);
}

void GraphicsWebKitView::AddBookmarklet(QPoint pos){
    if(page()) page()->AddBookmarklet(pos);
}

void GraphicsWebKitView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QGraphicsWebView::hideEvent(ev);
}

void GraphicsWebKitView::showEvent(QShowEvent *ev){
    QGraphicsWebView::showEvent(ev);
    RestoreViewState();
}

void GraphicsWebKitView::keyPressEvent(QKeyEvent *ev){

    if(GetDisplayObscured()){
        if(ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_F11){
            ExitFullScreen();
            ev->setAccepted(true);
            return;
        }
    }
#ifdef PASSWORD_MANAGER
    if(Application::HasCtrlModifier(ev) && ev->key() == Qt::Key_Return){

        NetworkAccessManager *nam =
            static_cast<NetworkAccessManager*>(page()->networkAccessManager());
        QString data = Application::GetAuthData(nam->GetId() + QStringLiteral(":") + url().host());
        if(!data.isEmpty()){
            QMap<QString, QString> formdata;
            QList<QString> list = data.split('&');
            foreach(QString form, list){
                QList<QString> pair = form.split('=');
                formdata[QUrl::fromPercentEncoding(pair[0].toLatin1())] = QUrl::fromPercentEncoding(pair[1].toLatin1());
            }
            QWebElement submit;
            foreach(QWebElement elem, page()->mainFrame()->findAllElements("form")){
                foreach(QWebElement f, elem.findAll("input")){
                    QString type = f.attribute("type").toLower();
                    QString name = f.attribute("name").toLower();
                    if(type == "submit"){
                        submit = f;
                    } else if(type != "hidden"){
                        if(formdata.keys().contains(name))
                            f.setAttribute("value", formdata[name]);
                    }
                }
            }
            if(!submit.isNull()){
                submit.evaluateJavaScript("this.click();");
                ev->setAccepted(true);
                return;
            }
        }
    }
#endif //ifdef PASSWORD_MANAGER

    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QGraphicsWebView::keyPressEvent(ev);

    if(!ev->isAccepted() &&
       !Application::IsOnlyModifier(ev)){

        if(NavigationBySpaceKey() &&
           ev->key() == Qt::Key_Space){

            if(Application::HasShiftModifier(ev)){
                m_TreeBank->Back(GetHistNode());
            } else {
                m_TreeBank->Forward(GetHistNode());
            }
            ev->setAccepted(true);
            return;
        }

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void GraphicsWebKitView::keyReleaseEvent(QKeyEvent *ev){
    QGraphicsWebView::keyReleaseEvent(ev);

    int k = ev->key();

    if(page()->settings()->testAttribute(QWebSettings::ScrollAnimatorEnabled) &&
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
            QTimer::singleShot(i*200, this, &GraphicsWebKitView::EmitScrollChangedIfNeed);
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void GraphicsWebKitView::resizeEvent(QGraphicsSceneResizeEvent *ev){
    QGraphicsWebView::resizeEvent(ev);
}

void GraphicsWebKitView::contextMenuEvent(QGraphicsSceneContextMenuEvent  *ev){
    ev->setAccepted(true);
}

void GraphicsWebKitView::hoverMoveEvent(QGraphicsSceneHoverEvent *ev){
    // danger in closing view. if it's last one.
    //Application::SetCurrentWindow(m_TreeBank->GetMainWindow());
    QGraphicsWebView::hoverMoveEvent(ev);
}

void GraphicsWebKitView::mouseMoveEvent(QGraphicsSceneMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QGraphicsWebView::mouseMoveEvent(ev);
        ev->setAccepted(false);
        return;
    }
    if(ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos().toPoint());
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

        if(QLineF(ev->pos().toPoint(), m_GestureStartedPos).length() < 2){
            // gesture not aborted.
            QGraphicsWebView::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }
        QDrag *drag = new QDrag(m_TreeBank);

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
            QGraphicsWebView::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = ev->pos().toPoint() - rect.topLeft();

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
            GestureMoved(ev->pos().toPoint());
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
        QGraphicsWebView::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void GraphicsWebKitView::mousePressEvent(QGraphicsSceneMouseEvent *ev){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            if(!View::TriggerAction(str, ev->pos().toPoint())){
                ev->setAccepted(false);
                return;
            }
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    GestureStarted(ev->pos().toPoint());
    QGraphicsWebView::mousePressEvent(ev);
    ev->setAccepted(true);
}

void GraphicsWebKitView::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev){
    emit statusBarMessage(QString());

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MidButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(Application::HasShiftModifier(ev) ||
           Application::HasCtrlModifier(ev) ||
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
            GestureFinished(ev->pos().toPoint(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos().toPoint(), ev->screenPos());
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QGraphicsWebView::mouseReleaseEvent(ev);
    EmitScrollChangedIfNeed();
    ev->setAccepted(true);
}

void GraphicsWebKitView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev){
    QGraphicsWebView::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void GraphicsWebKitView::dragEnterEvent(QGraphicsSceneDragDropEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QGraphicsWebView::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void GraphicsWebKitView::dragMoveEvent(QGraphicsSceneDragDropEvent *ev){
    if(m_EnableDragHackLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos().toPoint());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_LeftGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_LeftGestureMap[gesture])
              ? Action(Page::StringToAction(m_LeftGestureMap[gesture]))->text()
            : m_LeftGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QGraphicsWebView::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void GraphicsWebKitView::dropEvent(QGraphicsSceneDragDropEvent *ev){
    emit statusBarMessage(QString());
    bool isLocal = false;
    QPoint pos = ev->pos().toPoint();
    QObject *source = ev->source();
    QString text = ev->mimeData()->text();
    QList<QUrl> urls = ev->mimeData()->urls();
    QWidget *widget = m_TreeBank;

    foreach(QUrl u, urls){ if(u.isLocalFile()) isLocal = true;}

    if(!text.isEmpty()){
        text.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    } else if(!urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    SharedWebElement elem = HitElement(pos);

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        QGraphicsWebView::dropEvent(ev);
        ev->setAccepted(true);
        return;
    }

    if(!m_Gesture.isEmpty() && source == widget){
        GestureFinished(pos, Qt::LeftButton);
        ev->setAccepted(true);
        return;
    }

    GestureAborted();

    if(!urls.isEmpty() && source != widget){
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }
    ev->setAccepted(true);
}

void GraphicsWebKitView::dragLeaveEvent(QGraphicsSceneDragDropEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QGraphicsWebView::dragLeaveEvent(ev);
}

void GraphicsWebKitView::wheelEvent(QGraphicsSceneWheelEvent *ev){
    QGraphicsWebView::wheelEvent(ev);

    if(page()->settings()->testAttribute(QWebSettings::ScrollAnimatorEnabled)){
        for(int i = 1; i < 6; i++){
            QTimer::singleShot(i*200, this, &GraphicsWebKitView::EmitScrollChangedIfNeed);
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void GraphicsWebKitView::focusInEvent(QFocusEvent *ev){
    QGraphicsWebView::focusInEvent(ev);
    OnFocusIn();
}

void GraphicsWebKitView::focusOutEvent(QFocusEvent *ev){
    QGraphicsWebView::focusOutEvent(ev);
    OnFocusOut();
}

bool GraphicsWebKitView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QGraphicsWebView::focusNextPrevChild(next);
    return false;
}

SharedWebElementList GraphicsWebKitView::FindElements(Page::FindElementsOption option){

    SharedWebElementList list;

    std::function<void (QWebFrame*, QRect)> traverseFrame;

    traverseFrame = [&](QWebFrame *frame, QRect viewport){
        foreach(QWebElement elem, frame->findAllElements(Page::OptionToSelector(option))){
            std::shared_ptr<WebKitElement> e = std::make_shared<WebKitElement>();
            *e = WebKitElement(elem);
            if(!viewport.intersects(e->Rectangle()))
                e->SetRectangle(QRect());
            list << e;
        }

        foreach(QWebFrame *child, frame->childFrames()){
            QPoint p;
            QWebFrame *f = frame;
            while(f){
                p -= f->scrollPosition();
                p += f->geometry().topLeft();
                f  = f->parentFrame();
            }
            QRect port = viewport.intersected(child->geometry().translated(p));
            if(!port.isEmpty() && ! child->geometry().isEmpty()){
                traverseFrame(child, port);
            }
        }
    };

    traverseFrame(page()->mainFrame(), QRect(QPoint(), size()));

    return list;
}

SharedWebElement GraphicsWebKitView::HitElement(const QPoint &pos){

    QWebHitTestResult r = page()->mainFrame()->hitTestContent(pos);
    QWebElement elem =
        !r.element().isNull()     ? r.element() :
        !r.linkElement().isNull() ? r.linkElement() :
        r.enclosingBlockElement();
    std::shared_ptr<WebKitElement> e = std::make_shared<WebKitElement>();
    *e = WebKitElement(elem, r.isContentEditable(), r.linkUrl(), r.imageUrl(), r.pixmap());
    return e;
}

#endif //ifdef WEBKITVIEW
