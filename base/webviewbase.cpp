#include "switch.hpp"
#include "const.hpp"

//[[!WEV]]
#ifdef QTWEBKIT
//[[/!WEV]]

#include "webviewbase.hpp"

#include <QWebViewBase>
#include <QWebPageBase>
#include <QWebHistoryBase>
#include <QNetworkRequest>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QTextDocument>
#include <QClipboard>
#include <QTimer>
#include <QStyle>
#include <QKeySequence>
#include <QtConcurrent/QtConcurrent>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

namespace {
    //[[GWV]]
    inline QPoint ToPoint(QPointF p){
        return p.toPoint();
    }

    inline QSize ToSize(QSizeF s){
        return s.toSize();
    }

    template <class T>
    QPoint LocalPos(T *t){
        return ToPoint(t->pos());
    }

    template <class T>
    QPoint GlobalPos(T *t){
        return ToPoint(t->screenPos());
    }
    //[[/GWV]]
    //[[!GWV]]
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
    //[[/!GWV]]
}

//[[WEV]]
QKeySequence WebViewBase::m_PressedKey = QKeySequence();
//[[/WEV]]

WebViewBase::WebViewBase(TreeBank *parent, QString id, QStringList set)
    : View(parent, id, set)
      //[[GWV]]
    , QWebViewBase(0)
      //[[/GWV]]
      //[[!GWV]]
    , QWebViewBase(TreeBank::PurgeView() ? 0 : static_cast<QWidget*>(parent))
      //[[/!GWV]]
{
    Initialize();
    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebPageBase(nam, this);
    ApplySpecificSettings(set);
    setPage(page());

    //[[GWV]]
    if(parent) setParent(parent);
    setAcceptHoverEvents(true);
    //[[/GWV]]

    //[[!GWV]]
    if(TreeBank::PurgeView()){
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    } else {
        if(parent) setParent(parent);
    }
    setMouseTracking(true);
    //[[/!GWV]]

    setAcceptDrops(true);
}

WebViewBase::~WebViewBase(){
    setPage(0);
}

void WebViewBase::Connect(TreeBank *tb){
    View::Connect(tb);

    if(!tb || !page()) return;

    connect(this, SIGNAL(titleChanged(const QString&)),
            tb->parent(), SLOT(SetWindowTitle(const QString&)));
    //[[!WEV]]
    connect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
            this, SLOT(RestoreScroll()));
    connect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
            this, SIGNAL(ViewChanged()));
    //[[/!WEV]]
    //[[WEV]]
    connect(this, SIGNAL(loadProgress(int)),
            this, SLOT(RestoreScroll()));
    //[[/WEV]]
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

void WebViewBase::Disconnect(TreeBank *tb){
    View::Disconnect(tb);

    if(!tb || !page()) return;

    disconnect(this, SIGNAL(titleChanged(const QString&)),
               tb->parent(), SLOT(SetWindowTitle(const QString&)));
    //[[!WEV]]
    disconnect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
               this, SLOT(RestoreScroll()));
    disconnect(page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)),
               this, SIGNAL(ViewChanged()));
    //[[/!WEV]]
    //[[WEV]]
    disconnect(this, SIGNAL(loadProgress(int)),
               this, SLOT(RestoreScroll()));
    //[[/WEV]]
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

void WebViewBase::ZoomIn(){
    float zoom = PrepareForZoomIn();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebViewBase::ZoomOut(){
    float zoom = PrepareForZoomOut();
    setZoomFactor(static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebViewBase::OnSetViewNode(ViewNode*){}

void WebViewBase::OnSetHistNode(HistNode*){}

void WebViewBase::OnSetThis(WeakView){}

void WebViewBase::OnSetMaster(WeakView){}

void WebViewBase::OnSetSlave(WeakView){}

void WebViewBase::OnSetJsObject(_View*){}

void WebViewBase::OnSetJsObject(_Vanilla*){}

void WebViewBase::OnLoadStarted(){
    if(!GetHistNode()) return;
    if(history()->count()){
        QUrl historyUrl = history()->currentItem().url();
        if(!historyUrl.isEmpty() && historyUrl != url()){
            emit urlChanged(historyUrl);
        }
    }
    emit statusBarMessage(tr("Started loading."));
}

void WebViewBase::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void WebViewBase::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;
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

    //[[!WEV]]
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
    //[[/!WEV]]
}

void WebViewBase::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void WebViewBase::OnUrlChanged(const QUrl &url){
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

void WebViewBase::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void WebViewBase::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void WebViewBase::EmitScrollChanged(){
    if(!page()) return;
    //[[!WEV]]
    emit ScrollChanged(GetScroll());
    //[[/!WEV]]
    //[[WEV]]
    CallWithScroll([this](QPointF pos){ emit ScrollChanged(pos);});
    //[[/WEV]]
}

void WebViewBase::EmitScrollChangedIfNeed(){
    if(!page()) return;
    //[[!WEV]]
    if(GetHistNode()->GetScrollX() != page()->mainFrame()->scrollBarValue(Qt::Horizontal) ||
       GetHistNode()->GetScrollY() != page()->mainFrame()->scrollBarValue(Qt::Vertical))
        EmitScrollChanged();
    //[[/!WEV]]
    //[[WEV]]
    EmitScrollChanged();
    //[[/WEV]]
}

//[[WEV]]
void WebViewBase::CallWithScroll(PointFCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        // sometime cause crash.
        //callBack(QPointF(0.5f, 0.5f));
        return;
    }
    page()->runJavaScript
        (GetScrollRatioPointJsCode(),
         [this, callBack](QVariant var){
            if(!var.isValid()){
                callBack(QPointF(0.5f, 0.5f));
                return;
            }
            QVariantList list = var.toList();
            callBack(QPointF(list[0].toFloat(),list[1].toFloat()));
        });
}
//[[/WEV]]

void WebViewBase::SetScrollBarState(){
    if(!page()) return;
    //[[!WEV]]
    int hmax = page()->mainFrame()->scrollBarMaximum(Qt::Horizontal);
    int vmax = page()->mainFrame()->scrollBarMaximum(Qt::Vertical);
    //[[/!WEV]]
    //[[WEV]]
    page()->runJavaScript(GetScrollBarStateJsCode(), [this](QVariant var){
    if(!var.isValid()) return;
    QVariantList list = var.toList();
    int hmax = list[0].toInt();
    int vmax = list[1].toInt();
    if(hmax < 0) hmax = 0;
    if(vmax < 0) vmax = 0;
    //[[/WEV]]

    if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
    else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
    else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
    else             m_ScrollBarState = NoScrollBarEnabled;

    //[[WEV]]
    });
    //[[/WEV]]
}

QPointF WebViewBase::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    //[[!WEV]]
    float hval = static_cast<float>(page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    float hmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal));
    float vval = static_cast<float>(page()->mainFrame()->scrollBarValue(Qt::Vertical));
    float vmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    return QPointF(hmax == 0.0f ? 0.5f : hval / hmax,
                   vmax == 0.0f ? 0.5f : vval / vmax);
    //[[/!WEV]]
    //[[WEV]]
    // this function does not return actual value on WebEngineView.
    return QPointF(0.5f, 0.5f);
    //[[/WEV]]
}

void WebViewBase::SetScroll(QPointF pos){
    if(!page()) return;
    //[[!WEV]]
    float hmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal));
    float vmax = static_cast<float>(page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    page()->mainFrame()->setScrollBarValue(Qt::Horizontal, static_cast<int>(hmax * pos.x()));
    page()->mainFrame()->setScrollBarValue(Qt::Vertical,   static_cast<int>(vmax * pos.y()));
    EmitScrollChanged();
    //[[/!WEV]]
    //[[WEV]]
    page()->runJavaScript
        (SetScrollRatioPointJsCode(pos),
         [this](QVariant){
            EmitScrollChanged();
        });
    //[[/WEV]]
}

bool WebViewBase::SaveScroll(){
    if(!page()) return false;
    //[[!WEV]]
    if(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
        GetHistNode()->SetScrollX(page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    if(page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
        GetHistNode()->SetScrollY(page()->mainFrame()->scrollBarValue(Qt::Vertical));
    //[[/!WEV]]
    //[[WEV]]
    page()->runJavaScript
        (GetScrollValuePointJsCode(),
         [this](QVariant var){
            if(!var.isValid() || !GetHistNode()) return;
            QVariantList list = var.toList();
            GetHistNode()->SetScrollX(list[0].toInt());
            GetHistNode()->SetScrollY(list[1].toInt());
        });
    //[[/WEV]]
    return true;
}

bool WebViewBase::RestoreScroll(){
    if(!page() || !GetHistNode()) return false;
    //[[!WEV]]
    if(page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
        page()->mainFrame()->setScrollBarValue(Qt::Horizontal, GetHistNode()->GetScrollX());
    if(page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
        page()->mainFrame()->setScrollBarValue(Qt::Vertical,   GetHistNode()->GetScrollY());
    //[[/!WEV]]
    //[[WEV]]
    QPoint pos = QPoint(GetHistNode()->GetScrollX(),
                        GetHistNode()->GetScrollY());
    page()->runJavaScript(SetScrollValuePointJsCode(pos));
    //[[/WEV]]
    return true;
}

bool WebViewBase::SaveZoom(){
    GetHistNode()->SetZoom(zoomFactor());
    return true;
}

bool WebViewBase::RestoreZoom(){
    setZoomFactor(static_cast<qreal>(GetHistNode()->GetZoom()));
    return true;
}

bool WebViewBase::SaveHistory(){
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << (*history());
    if(!ba.isEmpty()){
        GetHistNode()->SetHistoryData(ba);
        return true;
    }
    return false;
}

bool WebViewBase::RestoreHistory(){
    QByteArray ba = GetHistNode()->GetHistoryData();
    if(!ba.isEmpty()){
        QDataStream stream(&ba, QIODevice::ReadOnly);
        stream >> (*history());
        return history()->count() > 0;
    }
    return false;
}

void WebViewBase::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool WebViewBase::SeekText(const QString &str, View::FindFlags opt){
    QWebPageBase::FindFlags flags = 0;
    if(opt & FindBackward)                      flags |= QWebPageBase::FindBackward;
    if(opt & CaseSensitively)                   flags |= QWebPageBase::FindCaseSensitively;
    //[[!WEV]]
    if(opt & WrapsAroundDocument)               flags |= QWebPageBase::FindWrapsAroundDocument;
    if(opt & HighlightAllOccurrences)           flags |= QWebPageBase::HighlightAllOccurrences;
    if(opt & FindAtWordBeginningsOnly)          flags |= QWebPageBase::FindAtWordBeginningsOnly;
    if(opt & TreatMedialCapitalAsWordBeginning) flags |= QWebPageBase::TreatMedialCapitalAsWordBeginning;
    if(opt & FindBeginsInSelection)             flags |= QWebPageBase::FindBeginsInSelection;
    //[[/!WEV]]

    //[[!WEV]]
    // for deleting previous seek.
    if(!str.isEmpty() && str.toLower() != page()->selectedText().toLower()){
        QWebViewBase::findText(QString(), flags);
    }
    bool ret = QWebViewBase::findText(str, flags);
    EmitScrollChangedIfNeed();
    //[[/!WEV]]
    //[[WEV]]
    bool ret = true; // dummy value.
    QWebViewBase::findText(str, flags,
                           [this](bool){
                               EmitScrollChangedIfNeed();
                           });
    //[[/WEV]]
    return ret;
}

//[[WEV]]
void WebViewBase::SetFocusToElement(QString xpath){
    page()->runJavaScript(SetFocusToElementJsCode(xpath));
}

void WebViewBase::FireClickEvent(QString xpath, QPoint pos){
    page()->runJavaScript(FireClickEventJsCode(xpath, pos/zoomFactor()));
}

void WebViewBase::SetTextValue(QString xpath, QString text){
    page()->runJavaScript(SetTextValueJsCode(xpath, text));
}

void WebViewBase::childEvent(QChildEvent *ev){
    QWebViewBase::childEvent(ev);
    if(ev->added() &&
       0 == strcmp(ev->child()->metaObject()->className(),
                   "QtWebEngineCore::RenderWidgetHostViewQtDelegateWidget")){
        ev->child()->installEventFilter(new EventEater(this, ev->child()));
    }
}
//[[/WEV]]

void WebViewBase::hideEvent(QHideEvent *ev){
    //[[WEV]]
#if QT_VERSION >= 0x050600
    if(page()->ObscureDisplay()) page()->triggerAction(QWebEnginePage::ExitFullScreen);
#endif
    //[[/WEV]]
    SaveViewState();
    QWebViewBase::hideEvent(ev);
}

void WebViewBase::showEvent(QShowEvent *ev){
    QWebViewBase::showEvent(ev);
    RestoreViewState();
}

void WebViewBase::keyPressEvent(QKeyEvent *ev){
    // all key events are ignored, if input method is activated.
    // so input method specific keys are accepted.
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    QWebViewBase::keyPressEvent(ev);

    if(!ev->isAccepted() &&
       !Application::IsOnlyModifier(ev)){

        //[[!WEV]]
        if(NavigationBySpaceKey() &&
           ev->key() == Qt::Key_Space){

            if(ev->modifiers() & Qt::ShiftModifier){
                m_TreeBank->Back(GetHistNode());
            } else {
                m_TreeBank->Forward(GetHistNode());
            }
            ev->setAccepted(true);
            return;
        }
        //[[/!WEV]]

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void WebViewBase::keyReleaseEvent(QKeyEvent *ev){
    QWebViewBase::keyReleaseEvent(ev);

    int k = ev->key();

    if(page()->settings()->testAttribute(QWebSettingsBase::ScrollAnimatorEnabled) &&
       (k == Qt::Key_Space ||
      //k == Qt::Key_Up ||
      //k == Qt::Key_Down ||
      //k == Qt::Key_Right ||
      //k == Qt::Key_Left ||
        k == Qt::Key_PageUp ||
        k == Qt::Key_PageDown ||
        k == Qt::Key_Home ||
        k == Qt::Key_End)){

        for(int i = 0; i < 12; i++){
            QTimer::singleShot(i*100, this, SLOT(EmitScrollChangedIfNeed()));
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void WebViewBase::resizeEvent(QResizeEventBase *ev){
    QWebViewBase::resizeEvent(ev);
}

void WebViewBase::contextMenuEvent(QContextMenuEventBase *ev){
    ev->setAccepted(true);
}

//[[GWV]]
void WebViewBase::hoverMoveEvent(QHoverEventBase *ev){
    // danger in closing view. if it's last one.
    //Application::SetCurrentWindow(m_TreeBank->GetMainWindow());
    QWebViewBase::hoverMoveEvent(ev);
}
//[[/GWV]]

void WebViewBase::mouseMoveEvent(QMouseEventBase *ev){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        QWebViewBase::mouseMoveEvent(ev);
        ev->setAccepted(false);
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
            QWebViewBase::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }
        //[[GWV]]
        QDrag *drag = new QDrag(m_TreeBank);
        //[[/GWV]]
        //[[!GWV]]
        QDrag *drag = new QDrag(this);
        //[[/!GWV]]

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
            QWebViewBase::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }

        QPixmap pixmap = m_HadSelection
            ? CreatePixmapFromSelection()
            : CreatePixmapFromElement();

        //[[WEV]]
        QRect rect = m_HadSelection
            ? m_SelectionRegion.boundingRect()
            : m_ClickedElement->Rectangle().intersected(QRect(QPoint(), size()));
        QPoint pos = LocalPos(ev) - rect.topLeft();
        //[[/WEV]]
        //[[!WEV]]
        QRect rect;
        QPoint pos;
        if(m_HadSelection){

            rect = m_SelectionRegion.boundingRect();
            pos = LocalPos(ev) - rect.topLeft();

        } else {

            rect = m_ClickedElement->Rectangle();

            if(rect.topLeft().x() && rect.topLeft().y()){

                pos = LocalPos(ev) - rect.topLeft();

            } else if(rect.topRight().x() != size().width() &&
                      rect.topRight().y()){

                pos = LocalPos(ev) - QPoint(rect.topRight().x() - pixmap.width(),
                                            rect.topRight().y());

            } else if(rect.bottomLeft().x() &&
                      rect.bottomLeft().y() != size().height()){

                pos = LocalPos(ev) - QPoint(rect.bottomLeft().x(),
                                            rect.bottomLeft().y() - pixmap.height());

            } else if(rect.bottomRight().x() != size().width() &&
                      rect.bottomRight().y() != size().height()){

                pos = LocalPos(ev) - QPoint(rect.bottomRight().x() - pixmap.width(),
                                            rect.bottomRight().y() - pixmap.height());
            } else {

                pos = LocalPos(ev) - rect.topLeft();
            }
        }
        //[[/!WEV]]

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
        QWebViewBase::mouseMoveEvent(ev);
        ev->setAccepted(false);
    }
}

void WebViewBase::mousePressEvent(QMouseEventBase *ev){
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
    QWebViewBase::mousePressEvent(ev);
    //[[WEV]]
    ev->setAccepted(false);
    //[[/WEV]]
    //[[!WEV]]
    ev->setAccepted(true);
    //[[/!WEV]]
}

void WebViewBase::mouseReleaseEvent(QMouseEventBase *ev){
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
    QWebViewBase::mouseReleaseEvent(ev);
    EmitScrollChangedIfNeed();
    //[[WEV]]
    ev->setAccepted(false);
    //[[/WEV]]
    //[[!WEV]]
    ev->setAccepted(true);
    //[[/!WEV]]
}

void WebViewBase::mouseDoubleClickEvent(QMouseEventBase *ev){
    QWebViewBase::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void WebViewBase::dragEnterEvent(QDragEnterEventBase *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QWebViewBase::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void WebViewBase::dragMoveEvent(QDragMoveEventBase *ev){
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
    QWebViewBase::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void WebViewBase::dropEvent(QDropEventBase *ev){
    emit statusBarMessage(QString());
    QPoint pos = LocalPos(ev);
    QList<QUrl> urls = ev->mimeData()->urls();
    QObject *source = ev->source();
    //[[!GWV]]
    QWidget *widget = this;
    //[[/!GWV]]
    //[[GWV]]
    QWidget *widget = m_TreeBank;
    //[[/GWV]]
    QString text;
    if(!ev->mimeData()->text().isEmpty()){
        text = ev->mimeData()->text().replace(QStringLiteral("\""), QStringLiteral("\\\""));
    } else if(!urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    //[[!WEV]]
    SharedWebElement elem = HitElement(pos);

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        QWebViewBase::dropEvent(ev);
        ev->setAccepted(true);
        return;
    }

    if(!m_Gesture.isEmpty() && source == widget){
        GestureFinished(pos, Qt::LeftButton);
        ev->setAccepted(true);
        return;
    }

    GestureAborted();

    if(urls.isEmpty() || source == widget){
        // do nothing.
    } else if(qobject_cast<TreeBank*>(source) || dynamic_cast<View*>(source)){
        QList<QUrl> filtered;
        foreach(QUrl u, urls){ if(!u.isLocalFile()) filtered << u;}
        m_TreeBank->OpenInNewViewNode(filtered, true, GetViewNode());
    } else {
        // foreign drag.
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }
    ev->setAccepted(true);
    //[[/!WEV]]
    //[[WEV]]
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

    QWebViewBase::dropEvent(ev);
    ev->setAccepted(true);
    //[[/WEV]]
}

void WebViewBase::dragLeaveEvent(QDragLeaveEventBase *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QWebViewBase::dragLeaveEvent(ev);
}

void WebViewBase::wheelEvent(QWheelEventBase *ev){

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

    } else {
        //[[GWV]]
        QWebViewBase::wheelEvent(ev);
        //[[/GWV]]
        //[[WV]]
        QWheelEvent *new_ev = new QWheelEvent(ev->pos(),
                                              ev->delta()*Application::WheelScrollRate(),
                                              ev->buttons(),
                                              ev->modifiers(),
                                              ev->orientation());
        QWebViewBase::wheelEvent(new_ev);
        ev->setAccepted(true);
        delete new_ev;
        //[[/WV]]
        //[[WEV]]
        ev->setAccepted(false);
        //[[/WEV]]
    }

    if(page()->settings()->testAttribute(QWebSettingsBase::ScrollAnimatorEnabled)){
        for(int i = 1; i < 6; i++){
            QTimer::singleShot(i*200, this, SLOT(EmitScrollChangedIfNeed()));
        }
    } else {
        EmitScrollChangedIfNeed();
    }
}

void WebViewBase::focusInEvent(QFocusEvent *ev){
    QWebViewBase::focusInEvent(ev);
    OnFocusIn();
}

void WebViewBase::focusOutEvent(QFocusEvent *ev){
    QWebViewBase::focusOutEvent(ev);
    OnFocusOut();
}

bool WebViewBase::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QWebViewBase::focusNextPrevChild(next);
    return false;
}

//[[!GWV]]
#if defined(Q_OS_WIN)
bool WebViewBase::nativeEvent(const QByteArray &eventType, void *message, long *result){
    bool ret = QWebViewBase::nativeEvent(eventType, message, result);
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
//[[/!GWV]]

//[[!WEV]]
namespace{
    class Element : public WebElement{
    public:
        Element()
            : WebElement()
        {
            m_Element = QWebElementBase();
            m_IsEditable = false;
            m_LinkUrl = QUrl();
            m_ImageUrl = QUrl();
            m_Pixmap = QPixmap();
            m_CoordinateOverridden = false;
            m_OverriddenRectangle = QRect();
        }
        Element(QWebElementBase elem)
            : WebElement()
        {
            m_Element = elem;
            m_IsEditable = false;
            m_LinkUrl = QUrl();
            m_ImageUrl = QUrl();
            m_Pixmap = QPixmap();
            m_CoordinateOverridden = false;
            m_OverriddenRectangle = QRect();

            while(!elem.isNull()){
                if(elem.attribute(QStringLiteral("contenteditable")) == QStringLiteral("true")){
                    m_IsEditable = true;
                    break;
                }
                elem = elem.parent();
            }
        }
        Element(QWebElementBase elem, bool editable, QUrl link, QUrl image, QPixmap pixmap)
            : WebElement()
        {
            m_Element = elem;
            m_IsEditable = editable;
            m_LinkUrl = link;
            m_ImageUrl = image;
            m_Pixmap = pixmap;
            m_CoordinateOverridden = false;
            m_OverriddenRectangle = QRect();
        }
        ~Element(){
        }

        bool SetFocus() DECL_OVERRIDE {
            m_Element.setFocus();
            return true;
        }
        bool ClickEvent() DECL_OVERRIDE {
            return false;
        }
        QString TagName() const DECL_OVERRIDE {
            return m_Element.tagName();
        }
        QString InnerText() const DECL_OVERRIDE {
            return m_Element.toPlainText();
        }
        QUrl BaseUrl() const DECL_OVERRIDE {
            return m_Element.webFrame()->baseUrl();
        }
        QUrl LinkUrl() const DECL_OVERRIDE {
            if(m_LinkUrl.isEmpty()){
                QWebElementBase elem = m_Element;
                while(!elem.isNull()){
                    QString href = elem.attribute(QStringLiteral("href"));
                    if(!href.isEmpty())
                        return Page::StringToUrl(href, elem.webFrame()->baseUrl());
                    elem = elem.parent();
                }
            }
            return m_LinkUrl;
        }
        QUrl ImageUrl() const DECL_OVERRIDE {
            if(m_ImageUrl.isEmpty()){
                QWebElementBase elem = m_Element;
                while(!elem.isNull()){
                    QString src = elem.attribute(QStringLiteral("src"));
                    if(!src.isEmpty())
                        return Page::StringToUrl(src, elem.webFrame()->baseUrl());
                    elem = elem.parent();
                }
            }
            return m_ImageUrl;
        }
        QString LinkHtml() const DECL_OVERRIDE {
            QWebElementBase elem = m_Element;
            while(!elem.isNull()){
                QString href = elem.attribute(QStringLiteral("href"));
                if(!href.isEmpty()) return elem.toOuterXml();
                elem = elem.parent();
            }
            return QString();
        }
        QString ImageHtml() const DECL_OVERRIDE {
            QWebElementBase elem = m_Element;
            while(!elem.isNull()){
                QString src = elem.attribute(QStringLiteral("src"));
                if(!src.isEmpty()) return elem.toOuterXml();
                elem = elem.parent();
            }
            return QString();
        }
        QPoint Position() const DECL_OVERRIDE {
            if(m_CoordinateOverridden)
                return m_OverriddenRectangle.center();
            return Rectangle().center();
        }
        QRect Rectangle() const DECL_OVERRIDE {
            if(m_CoordinateOverridden)
                return m_OverriddenRectangle;
            QRect r = m_Element.geometry();
            QWebFrameBase *f = m_Element.webFrame();
            while(f){
                r.translate(-f->scrollPosition());
                r.translate(f->geometry().topLeft());
                r = r.intersected(f->geometry());
                f = f->parentFrame();
            }
            return r;
        }
        void SetPosition(QPoint pos){
            m_CoordinateOverridden = true;
            m_OverriddenRectangle.moveCenter(pos);
        }
        void SetRectangle(QRect rect){
            m_CoordinateOverridden = true;
            m_OverriddenRectangle = rect;
        }
        void SetText(QString text){
            m_Element.setAttribute("value", text);
        }
        QPixmap Pixmap() DECL_OVERRIDE {
            if(m_Pixmap.isNull()){
                QPixmap pixmap(m_Element.geometry().size());
                pixmap.fill(QColor(255,255,255,0));
                QImage before = pixmap.toImage();
                QPainter painter(&pixmap);
                m_Element.render(&painter);
                painter.end();
                if(pixmap.toImage() == before)
                    return QPixmap();
                return pixmap;
            }
            return m_Pixmap;
        }
        bool IsNull() const DECL_OVERRIDE {
            return m_Element.isNull() || Rectangle().isNull() || Position().isNull();
        }
        bool IsJsCommandElement() const DECL_OVERRIDE {
            QString onclick = m_Element.attribute(QStringLiteral("onclick"));
            QString href = m_Element.attribute(QStringLiteral("href")).toLower();
            QString role = m_Element.attribute(QStringLiteral("role")).toLower();
            return !onclick.isEmpty() ||
                href.startsWith(QStringLiteral("javascript:")) ||
                role == QStringLiteral("button") ||
                role == QStringLiteral("link") ||
                role == QStringLiteral("menu") ||
                role == QStringLiteral("checkbox") ||
                role == QStringLiteral("radio") ||
                role == QStringLiteral("tab");
        }
        bool IsTextInputElement() const DECL_OVERRIDE {
            QString tag = m_Element.tagName().toLower();
            QString type = m_Element.attribute(QStringLiteral("type")).toLower();
            return tag == QStringLiteral("textaret") ||
                (tag == QStringLiteral("input") &&
                 (type == QStringLiteral("text") ||
                  type == QStringLiteral("search") ||
                  type == QStringLiteral("password")));
        }
        bool IsQueryInputElement() const DECL_OVERRIDE {
            QString tag = m_Element.tagName().toLower();
            QString type = m_Element.attribute(QStringLiteral("type")).toLower();
            return tag == QStringLiteral("input") &&
                (type == QStringLiteral("text") ||
                 type == QStringLiteral("search"));
        }
        bool IsEditableElement() const DECL_OVERRIDE {
            return m_IsEditable
                || IsTextInputElement()
                || IsQueryInputElement();
        }
        bool IsFrameElement() const DECL_OVERRIDE {
            QString tag = m_Element.tagName().toLower();
            return tag == QStringLiteral("frame")
                || tag == QStringLiteral("iframe");
        }
        Action GetAction() const DECL_OVERRIDE {
            QString tag = m_Element.tagName().toLower();
            QString type = m_Element.attribute(QStringLiteral("type")).toLower();
            QString onclick = m_Element.attribute(QStringLiteral("onclick"));
            QString onhover = m_Element.attribute(QStringLiteral("onmouseover"));
            QString href = m_Element.attribute(QStringLiteral("href")).toLower();
            QString role = m_Element.attribute(QStringLiteral("role")).toLower();

            if(href.startsWith(QStringLiteral("http:")) ||
               href.startsWith(QStringLiteral("https:"))){

                return None;
            }
            if(m_IsEditable ||
               tag == QStringLiteral("textaret") ||
               tag == QStringLiteral("object") ||
               tag == QStringLiteral("embed") ||
               tag == QStringLiteral("frame") ||
               tag == QStringLiteral("iframe") ||
               (tag == QStringLiteral("input") &&
                (type == QStringLiteral("text") ||
                 type == QStringLiteral("search") ||
                 type == QStringLiteral("password")))){

                return Focus;
            }
            if(!onclick.isEmpty() ||
               href.startsWith(QStringLiteral("javascript:")) ||
               tag == QStringLiteral("button") ||
               tag == QStringLiteral("select") ||
               tag == QStringLiteral("label") ||
               role == QStringLiteral("button") ||
               role == QStringLiteral("link") ||
               role == QStringLiteral("menu") ||
               role == QStringLiteral("checkbox") ||
               role == QStringLiteral("radio") ||
               role == QStringLiteral("tab") ||
               (tag == QStringLiteral("input") &&
                (type == QStringLiteral("checkbox") ||
                 type == QStringLiteral("radio") ||
                 type == QStringLiteral("file") ||
                 type == QStringLiteral("submit") ||
                 type == QStringLiteral("reset") ||
                 type == QStringLiteral("button")))){

                return Click;
            }
            if(!onhover.isEmpty()){

                return Hover;
            }
            return None;
        }
        bool Equals(const WebElement &other) const DECL_OVERRIDE {
            return m_Element == static_cast<const Element*>(&other)->m_Element;
        }

    private:
        QWebElementBase m_Element;
        bool m_IsEditable;
        QUrl m_LinkUrl;
        QUrl m_ImageUrl;
        QPixmap m_Pixmap;
        bool m_CoordinateOverridden;
        QRect m_OverriddenRectangle;
    };
}

SharedWebElementList WebViewBase::FindElements(Page::FindElementsOption option){

    SharedWebElementList list;

    std::function<void (QWebFrameBase*, QRect)> traverseFrame;

    traverseFrame = [&](QWebFrameBase *frame, QRect viewport){
        foreach(QWebElementBase elem, frame->findAllElements(Page::OptionToSelector(option))){
            std::shared_ptr<Element> e = std::make_shared<Element>();
            *e = Element(elem);
            if(!viewport.intersects(e->Rectangle()))
                e->SetRectangle(QRect());
            list << e;
        }

        foreach(QWebFrameBase *child, frame->childFrames()){
            QPoint p;
            QWebFrameBase *f = frame;
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

SharedWebElement WebViewBase::HitElement(const QPoint &pos){

    QWebHitTestResult r = page()->mainFrame()->hitTestContent(pos);
    QWebElementBase elem =
        !r.element().isNull()     ? r.element() :
        !r.linkElement().isNull() ? r.linkElement() :
        r.enclosingBlockElement();
    std::shared_ptr<Element> e = std::make_shared<Element>();
    *e = Element(elem, r.isContentEditable(), r.linkUrl(), r.imageUrl(), r.pixmap());
    return e;
}
//[[/!WEV]]
//[[WEV]]
void WebViewBase::CallWithGotBaseUrl(UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        callBack(QUrl());
        return;
    }
    page()->runJavaScript
        (GetBaseUrlJsCode(),
         [this, callBack](QVariant url){
            callBack(url.isValid() ? url.toUrl() : QUrl());
        });
}

void WebViewBase::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    // this implementation is same as baseurl...
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        callBack(QUrl());
        return;
    }
    page()->runJavaScript
        (GetBaseUrlJsCode(),
         [this, callBack](QVariant url){
            callBack(url.isValid() ? url.toUrl() : QUrl());
        });
}

void WebViewBase::CallWithFoundElements(Page::FindElementsOption option,
                                        WebElementListCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        callBack(SharedWebElementList());
        return;
    }
    page()->runJavaScript
        (FindElementsJsCode(option),
         [this, callBack](QVariant var){
            if(!var.isValid()){
                callBack(SharedWebElementList());
                return;
            }
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

void WebViewBase::CallWithHitElement(const QPoint &pos,
                                     WebElementCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")) || pos.isNull()){
        callBack(SharedWebElement());
        return;
    }
    page()->runJavaScript
        (HitElementJsCode(pos/zoomFactor()),
         [this, callBack](QVariant var){
            if(!var.isValid()){
                callBack(SharedWebElement());
                return;
            }
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void WebViewBase::CallWithHitLinkUrl(const QPoint &pos,
                                     UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")) || pos.isNull()){
        callBack(QUrl());
        return;
    }
    page()->runJavaScript
        (HitLinkUrlJsCode(pos/zoomFactor()),
         [this, callBack](QVariant url){
            callBack(url.isValid() ? url.toUrl() : QUrl());
        });
}

void WebViewBase::CallWithHitImageUrl(const QPoint &pos,
                                      UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank")) || pos.isNull()){
        callBack(QUrl());
        return;
    }
    page()->runJavaScript
        (HitImageUrlJsCode(pos/zoomFactor()),
         [this, callBack](QVariant url){
            callBack(url.isValid() ? url.toUrl() : QUrl());
        });
}

void WebViewBase::CallWithSelectedText(StringCallBack callBack){
    if(page()) callBack(page()->selectedText());
}

void WebViewBase::CallWithSelectedHtml(StringCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        callBack(QString());
        return;
    }
    page()->runJavaScript
        (SelectedHtmlJsCode(),
         [callBack](QVariant result){
            callBack(result.isValid() ? result.toString() : QString());
        });
}

void WebViewBase::CallWithWholeText(StringCallBack callBack){
    if(page()) page()->toPlainText(callBack);
}

void WebViewBase::CallWithWholeHtml(StringCallBack callBack){
    if(page()) page()->toHtml(callBack);
}

void WebViewBase::CallWithSelectionRegion(RegionCallBack callBack){
    if(!page() || !hasSelection()) callBack(QRegion());
    page()->runJavaScript
        (SelectionRegionJsCode(),
         [this, callBack](QVariant var){
            QRegion region;
            if(!var.isValid() || !var.canConvert(QMetaType::QVariantMap)){
                callBack(region);
                return;
            }
            QVariantMap map = var.toMap();
            QRect viewport = QRect(QPoint(), size());
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

void WebViewBase::CallWithEvaluatedJavaScriptResult(const QString &code, VariantCallBack callBack){
    if(!page() || url().isEmpty() || url() == QUrl(QStringLiteral("about:blank"))){
        callBack(QVariant());
        return;
    }
    page()->runJavaScript(code, callBack);
}
//[[/WEV]]

//[[!WEV]]
#endif
//[[/!WEV]]
