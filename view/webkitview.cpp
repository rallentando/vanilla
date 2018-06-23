#include "switch.hpp"
#include "const.hpp"

#ifdef WEBKITVIEW

#include "webkitview.hpp"

#include <QWebView>
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

WebKitView::WebKitView(TreeBank *parent, QString id, QStringList set)
    : QWebView(TreeBank::PurgeView() ? 0 : static_cast<QWidget*>(parent))
    , View(parent, id, set)
{
    Initialize();
    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebKitPage(nam, this);
    ApplySpecificSettings(set);
    setPage(page());

    if(TreeBank::PurgeView()){
        setWindowFlags(Qt::FramelessWindowHint);
    } else {
        if(parent) setParent(parent);
    }
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_AcceptTouchEvents);
}

WebKitView::~WebKitView(){
    setPage(0);
}

QWebView *WebKitView::base(){
    return static_cast<QWebView*>(this);
}

WebKitPage *WebKitView::page(){
    return static_cast<WebKitPage*>(View::page());
}

QUrl WebKitView::url(){
    return base()->url();
}

QString WebKitView::html(){
    return WholeHtml();
}

TreeBank *WebKitView::parent(){
    return m_TreeBank;
}

void WebKitView::setUrl(const QUrl &url){
    base()->setUrl(url);
    emit urlChanged(url);
}

void WebKitView::setHtml(const QString &html, const QUrl &url){
    base()->setHtml(html, url);
    emit urlChanged(url);
}

void WebKitView::setParent(TreeBank* tb){
    View::SetTreeBank(tb);
    if(!TreeBank::PurgeView()) base()->setParent(tb);
    if(page()) page()->AddJsObject();
    if(tb) resize(size());
}

void WebKitView::Connect(TreeBank *tb){
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

void WebKitView::Disconnect(TreeBank *tb){
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

void WebKitView::OnSetViewNode(ViewNode*){}

void WebKitView::OnSetHistNode(HistNode*){}

void WebKitView::OnSetThis(WeakView){}

void WebKitView::OnSetMaster(WeakView){}

void WebKitView::OnSetSlave(WeakView){}

void WebKitView::OnSetJsObject(_View*){}

void WebKitView::OnSetJsObject(_Vanilla*){}

void WebKitView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
}

void WebKitView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void WebKitView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

    QUrl historyUrl;
    if(history()->count()){
        historyUrl = history()->currentItem().url();
    }
    if(!historyUrl.isEmpty()){
        emit urlChanged(historyUrl);
    } else if(!url().isEmpty()){
        emit urlChanged(url());
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

void WebKitView::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void WebKitView::OnUrlChanged(const QUrl &url){
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

void WebKitView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void WebKitView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void WebKitView::EmitScrollChanged(){
    if(!page()) return;
    emit ScrollChanged(GetScroll());
}

void WebKitView::EmitScrollChangedIfNeed(){
    if(!page()) return;
    if(GetHistNode()->GetScrollX() != page()->mainFrame()->scrollBarValue(Qt::Horizontal) ||
       GetHistNode()->GetScrollY() != page()->mainFrame()->scrollBarValue(Qt::Vertical))
        EmitScrollChanged();
}

void WebKitView::SetScrollBarState(){
    if(!page()) return;
    int hmax = page()->mainFrame()->scrollBarMaximum(Qt::Horizontal);
    int vmax = page()->mainFrame()->scrollBarMaximum(Qt::Vertical);

    if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
    else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
    else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
    else             m_ScrollBarState = NoScrollBarEnabled;
}

QPointF WebKitView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    float hval = static_cast<float>(page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    float hmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal));
    float vval = static_cast<float>(page()->mainFrame()->scrollBarValue(Qt::Vertical));
    float vmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    return QPointF(hmax == 0.0f ? 0.5f : hval / hmax,
                   vmax == 0.0f ? 0.5f : vval / vmax);
}

void WebKitView::SetScroll(QPointF pos){
    if(!page()) return;
    float hmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal));
    float vmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    page()->mainFrame()->setScrollBarValue(Qt::Horizontal, static_cast<int>(hmax * pos.x()));
    page()->mainFrame()->setScrollBarValue(Qt::Vertical,   static_cast<int>(vmax * pos.y()));
    EmitScrollChanged();
}

bool WebKitView::SaveScroll(){
    if(!page()) return false;
    if(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
        GetHistNode()->SetScrollX(page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    if(page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
        GetHistNode()->SetScrollY(page()->mainFrame()->scrollBarValue(Qt::Vertical));
    return true;
}

bool WebKitView::RestoreScroll(){
    if(!page() || !GetHistNode()) return false;
    if(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
        page()->mainFrame()->setScrollBarValue(Qt::Horizontal, GetHistNode()->GetScrollX());
    if(page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
        page()->mainFrame()->setScrollBarValue(Qt::Vertical,   GetHistNode()->GetScrollY());
    return true;
}

bool WebKitView::SaveZoom(){
    if(!GetHistNode()) return false;
    GetHistNode()->SetZoom(zoomFactor());
    return true;
}

bool WebKitView::RestoreZoom(){
    if(!GetHistNode()) return false;
    setZoomFactor(static_cast<qreal>(GetHistNode()->GetZoom()));
    return true;
}

bool WebKitView::SaveHistory(){
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

bool WebKitView::RestoreHistory(){
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

void WebKitView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool WebKitView::SeekText(const QString &str, View::FindFlags opt){
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
        QWebView::findText(QString(), flags);
    }
    bool ret = QWebView::findText(str, flags);
    EmitScrollChangedIfNeed();
    return ret;
}

void WebKitView::OnIconChanged(const QIcon &icon){
    Application::RegisterIcon(url().host(), icon);
}

void WebKitView::Copy(){
    if(page()) page()->triggerAction(QWebPage::Copy);
}

void WebKitView::Cut(){
    if(page()) page()->triggerAction(QWebPage::Cut);
}

void WebKitView::Paste(){
    if(page()) page()->triggerAction(QWebPage::Paste);
}

void WebKitView::Undo(){
    if(page()) page()->triggerAction(QWebPage::Undo);
}

void WebKitView::Redo(){
    if(page()) page()->triggerAction(QWebPage::Redo);
}

void WebKitView::SelectAll(){
    if(page()) page()->triggerAction(QWebPage::SelectAll);
}

void WebKitView::Unselect(){
    if(page()){
        page()->triggerAction(QWebPage::Unselect);
        page()->mainFrame()->evaluateJavaScript(QStringLiteral(
            "(function(){\n"
            "    document.activeElement.blur();\n"
            "}());"));
    }
}

void WebKitView::Reload(){
    if(page()) page()->triggerAction(QWebPage::Reload);
}

void WebKitView::ReloadAndBypassCache(){
    if(page()) page()->triggerAction(QWebPage::ReloadAndBypassCache);
}

void WebKitView::Stop(){
    if(page()) page()->triggerAction(QWebPage::Stop);
}

void WebKitView::StopAndUnselect(){
    Stop(); Unselect();
}

void WebKitView::Print(){
    if(page()) page()->Print();
}

void WebKitView::Save(){
    if(page()) page()->Download(url(), url());
}

void WebKitView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebKitView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebKitView::ToggleMediaControls(){
    if(page())
        page()->triggerAction(QWebPage::ToggleMediaControls);
}

void WebKitView::ToggleMediaLoop(){
    if(page())
         page()->triggerAction(QWebPage::ToggleMediaLoop);
}

void WebKitView::ToggleMediaPlayPause(){
    if(page())
        page()->triggerAction(QWebPage::ToggleMediaPlayPause);
}

void WebKitView::ToggleMediaMute(){
    if(page())
        page()->triggerAction(QWebPage::ToggleMediaMute);
}

void WebKitView::ExitFullScreen(){
    if(page()){
        // this doesn't work...
        //page()->triggerAction(QWebEnginePage::ToggleVideoFullscreen);
        if(TreeBank *tb = GetTreeBank())
            tb->GetMainWindow()->SetFullScreen(false);
        SetDisplayObscured(false);
    }
}

void WebKitView::InspectElement(){
    if(page()) page()->InspectElement();
}

void WebKitView::AddSearchEngine(QPoint pos){
    if(page()) page()->AddSearchEngine(pos);
}

void WebKitView::AddBookmarklet(QPoint pos){
    if(page()) page()->AddBookmarklet(pos);
}

void WebKitView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QWebView::hideEvent(ev);
}

void WebKitView::showEvent(QShowEvent *ev){
    QWebView::showEvent(ev);
    RestoreViewState();
}

void WebKitView::keyPressEvent(QKeyEvent *ev){

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

    // 'HasAnyModifier' ignores ShiftModifier.
    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){
        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QWebView::keyPressEvent(ev);

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

void WebKitView::keyReleaseEvent(QKeyEvent *ev){
    QWebView::keyReleaseEvent(ev);

    if(page()->settings()->testAttribute(QWebSettings::ScrollAnimatorEnabled) &&
       Application::IsMoveKey(ev)){

        for(int i = 1; i < 6; i++){
            QTimer::singleShot(i*200, this, &WebKitView::EmitScrollChangedIfNeed);
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void WebKitView::resizeEvent(QResizeEvent *ev){
    QWebView::resizeEvent(ev);
}

void WebKitView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void WebKitView::mouseMoveEvent(QMouseEvent *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QWebView::mouseMoveEvent(ev);
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
            QWebView::mouseMoveEvent(ev);
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
            QWebView::mouseMoveEvent(ev);
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
        QWebView::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void WebKitView::mousePressEvent(QMouseEvent *ev){
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
    QWebView::mousePressEvent(ev);
    ev->setAccepted(true);
}

void WebKitView::mouseReleaseEvent(QMouseEvent *ev){
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
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            m_SelectedText = page()->selectedText();
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos());
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QWebView::mouseReleaseEvent(ev);
    EmitScrollChangedIfNeed();
    ev->setAccepted(true);
}

void WebKitView::mouseDoubleClickEvent(QMouseEvent *ev){
    QWebView::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void WebKitView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QWebView::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void WebKitView::dragMoveEvent(QDragMoveEvent *ev){
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
    QWebView::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void WebKitView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    bool isLocal = false;
    QPoint pos = ev->pos();
    QObject *source = ev->source();
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

    SharedWebElement elem = HitElement(pos);

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        QWebView::dropEvent(ev);
        ev->setAccepted(true);
        return;
    }

    if(!m_Gesture.isEmpty() && source == this){
        GestureFinished(pos, Qt::LeftButton);
        ev->setAccepted(true);
        return;
    }

    GestureAborted();

    if(!urls.isEmpty() && source != this){
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }
    ev->setAccepted(true);
}

void WebKitView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QWebView::dragLeaveEvent(ev);
}

void WebKitView::wheelEvent(QWheelEvent *ev){
    if(ev->source() != Qt::MouseEventSynthesizedBySystem){
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
            return;
        }
    }

    QWebView::wheelEvent(ev);
    ev->setAccepted(true);

    if(page()->settings()->testAttribute(QWebSettings::ScrollAnimatorEnabled) &&
       ev->source() != Qt::MouseEventSynthesizedBySystem){
        for(int i = 1; i < 6; i++){
            QTimer::singleShot(i*200, this, &WebKitView::EmitScrollChangedIfNeed);
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void WebKitView::focusInEvent(QFocusEvent *ev){
    QWebView::focusInEvent(ev);
    OnFocusIn();
}

void WebKitView::focusOutEvent(QFocusEvent *ev){
    QWebView::focusOutEvent(ev);
    OnFocusOut();
}

bool WebKitView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QWebView::focusNextPrevChild(next);
    return false;
}

#if defined(Q_OS_WIN)
bool WebKitView::nativeEvent(const QByteArray &eventType, void *message, long *result){
    bool ret = QWebView::nativeEvent(eventType, message, result);
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

SharedWebElementList WebKitView::FindElements(Page::FindElementsOption option){

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

SharedWebElement WebKitView::HitElement(const QPoint &pos){

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
