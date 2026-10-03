#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#ifdef WEBENGINEVIEW

#include "webengineview.hpp"

#include <QQuickWidget>

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineHistory>
#include <QWebEngineContextMenuRequest>
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
#include "directorypage.hpp"
#include "extensioncontroller.hpp"
#include "webengineextensions.hpp"
#include "extensionhost.hpp"

WebEngineView::WebEngineView(TreeBank *parent, QString id, QStringList set)
    : QWebEngineView(TreeBank::PurgeView() ? 0 : static_cast<QWidget*>(parent))
    , View(parent, id, set)
    , m_BaseBackgroundColor(QColor())
{
    SetUpEventFilterInstaller();

    Initialize();
    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebEnginePage(nam, DirectoryPage::SaysPrivate(set), this);
    ExtensionHost::StampRequestsOf(page(), this);
    ApplySpecificSettings(set);
    setPage(page());
    ApplyTheme();

    if(TreeBank::PurgeView()){
        setWindowFlags(Qt::FramelessWindowHint);
    } else {
        if(parent) setParent(parent);
    }
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_AcceptTouchEvents);

    m_Inspector = 0;
    m_PreventScrollRestoration = false;
    m_ReportedZoomFactor = 0.0;
    m_ScrollSignalTimer = 0;
#ifdef MEDIATIME
    m_MediaTimeSaveTimer = 0;
#endif
    connect(this, SIGNAL(iconChanged(const QIcon&)),
            this, SLOT(OnIconChanged(const QIcon&)));
    connect(page(), &QWebEnginePage::zoomFactorChanged,
            this, [this](qreal factor){ m_ReportedZoomFactor = factor; });

#ifdef MEDIATIME
    connect(page(), &QWebEnginePage::recentlyAudibleChanged,
            this, [this](bool audible){
                if(audible){
                    if(!m_MediaTimeSaveTimer)
                        m_MediaTimeSaveTimer = startTimer(5000);
                } else {
                    if(m_MediaTimeSaveTimer){
                        killTimer(m_MediaTimeSaveTimer);
                        m_MediaTimeSaveTimer = 0;
                    }
                    SaveMediaTime();
                }
            });
#endif

}

WebEngineView::~WebEngineView(){
    if(m_Inspector){
        if(m_TreeBank)
            if(MainWindow *win = m_TreeBank->GetMainWindow())
                if(win->DockedInspectorPane() == m_Inspector)
                    win->SetInspectorPane(nullptr);
        m_Inspector->deleteLater();
    }

}

void WebEngineView::SetUpEventFilterInstaller(){
    static bool checked = false;
    if(checked) return;
    checked = true;

    connect(Application::GetInstance(), &Application::focusChanged, [](QWidget*, QWidget* widget){

        if(widget && widget->parentWidget() &&
           widget->inherits("QQuickWidget")){

            if(WebEngineView *wev = qobject_cast<WebEngineView*>(widget->parentWidget())){

                bool haveEventEater = false;

                foreach(QObject *obj, widget->children()){
                    if(0 == strcmp(obj->metaObject()->className(), "EventEater")){
                        haveEventEater = true;
                        break;
                    }
                }
                if(!haveEventEater)
                    widget->installEventFilter(new EventEater(wev, widget));
            }
        }
    });
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
    ExtensionNavigation::Of(this)->Request(Extensions(), [this, url] { base()->setUrl(url); });
    emit urlChanged(url);
}

void WebEngineView::setHtml(const QString &html, const QUrl &url){
    ExtensionNavigation::Of(this)->Request(Extensions(), [this, html, url] { base()->setHtml(html, url); });
    emit urlChanged(url);
}

ExtensionController *WebEngineView::Extensions() const {
    auto *page = qobject_cast<WebEnginePage*>(m_Page);
    return page ? ExtensionController::Of(page->profile()) : nullptr;
}

QWidget *WebEngineView::CreateExtensionView(const QUrl &url, ExtensionPage kind, QWidget *parent, const std::function<void()> &closed) {
    auto *page = qobject_cast<WebEnginePage*>(m_Page);
    return page ? WebEngineExtensions::CreatePopup(page->GetSharedProfile(), url, kind == ExtensionActionPage, GetThis(), parent,
                                                   kind == ExtensionSidePanelPage ? closed : std::function<void()>(),
                                                   kind == ExtensionSidePanelPage)
                : nullptr;
}

void WebEngineView::TriggerNativeLoadAction(const QUrl &url) {
    setUrl(url);
}

void WebEngineView::LoadAfterExtensions(const QWebEngineHttpRequest &request) {
    ExtensionNavigation::Of(this)->Request(Extensions(), [this, request] { load(request); });
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

void WebEngineView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    if(page() && page()->profile() &&
       page()->profile()->isOffTheRecord() != DirectoryPage::SaysPrivate(set))
        RebuildForOffTheRecord();
}

void WebEngineView::ApplyTheme(){
    if(!page()) return;
    const QUrl uri = url();
    const bool blank = uri.isEmpty() || uri == BLANK_URL ||
        uri.scheme() == VANILLA_SCHEME;
    const QColor color = blank ? Theme::Color(Theme::PageBackground)
                               : QColor(Qt::white);
    if(color == m_BaseBackgroundColor) return;
    m_BaseBackgroundColor = color;
    page()->setBackgroundColor(color);
}

void WebEngineView::AddSpellCheckMenu(QMenu *menu){
    if(!menu || !page()) return;

    QWebEngineContextMenuRequest *request = lastContextMenuRequest();
    if(!request || request->misspelledWord().isEmpty()) return;

    const QStringList suggestions = request->spellCheckerSuggestions();
    if(suggestions.isEmpty()) return;

    foreach(const QString &suggestion, suggestions){
        QAction *action = menu->addAction(suggestion);
        connect(action, &QAction::triggered, this, [this, suggestion](){
            if(page()) page()->replaceMisspelledWord(suggestion);
        });
    }
    menu->addSeparator();
}

void WebEngineView::Suspend(){
    const QString setting = SuspendHiddenViews();
    if(HiddenViewsStayActive()) return;
    if(!page()) return;

    const QWebEnginePage::LifecycleState state =
        setting == QStringLiteral("Discarded")
        ? QWebEnginePage::LifecycleState::Discarded
        : QWebEnginePage::LifecycleState::Frozen;

    QPointer<WebEngineView> self = this;
    QTimer::singleShot(0, this, [self, state](){
        if(!self || !self->page()) return;
        if(self->visible()) return;
        if(self->RecentlyAudible()) return;
        if(self->IsLoading()) return;

        self->page()->setLifecycleState(state);
    });
}

void WebEngineView::WakeUp(){
    if(!page()) return;
    if(page()->lifecycleState() == QWebEnginePage::LifecycleState::Active) return;
    page()->setLifecycleState(QWebEnginePage::LifecycleState::Active);
}

void WebEngineView::OnSetViewNode(ViewNode*){}

void WebEngineView::OnSetThis(WeakView){}

void WebEngineView::OnSetMaster(WeakView){}

void WebEngineView::OnSetSlave(WeakView){}

void WebEngineView::OnSetJsObject(_View*){}

void WebEngineView::OnSetJsObject(_Vanilla*){}

void WebEngineView::OnLoadStarted(){
    if(!GetViewNode()) return;

    View::OnLoadStarted();
    Application::ReassertColorSchemeForWeb();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;

#ifdef USE_WEBCHANNEL
#endif
}

void WebEngineView::OnLoadProgress(int progress){
    if(!GetViewNode()) return;
    View::OnLoadProgress(progress);
    if(progress != 100)
        emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void WebEngineView::OnLoadFinished(bool ok){
    ApplyTheme();

    if(!GetViewNode()) return;

    View::OnLoadFinished(ok);

    QUrl historyUrl;
    if(history()->count()){
        historyUrl = history()->currentItem().url();
    }
    if(!url().isEmpty() && url().toEncoded().startsWith("view-source:")){
        emit urlChanged(url());
    } else if(!historyUrl.isEmpty()){
        emit urlChanged(historyUrl);
    } else if(!url().isEmpty()){
        emit urlChanged(url());
    }
    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
#ifdef MEDIATIME
    RestoreMediaTime();
#endif
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

    if(visible() && m_TreeBank){
        UpdateThumbnail();
    }
}

void WebEngineView::OnTitleChanged(const QString &title){
    if(!GetViewNode()) return;
    ChangeNodeTitle(title);
}

void WebEngineView::OnUrlChanged(const QUrl &uri){
    if(!GetViewNode()) return;
    SaveHistory();
#ifdef MEDIATIME
    if(uri != GetViewNode()->GetUrl())
        GetViewNode()->SetMediaTime(0);
#endif
    ChangeNodeUrl(uri);
}

void WebEngineView::OnViewChanged(){
    if(!GetViewNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void WebEngineView::OnScrollChanged(){
    if(!GetViewNode()) return;
    SaveScroll();
}

void WebEngineView::EmitScrollChanged(){
    if(!page()) return;
    if(!m_ScrollSignalTimer)
        m_ScrollSignalTimer = startTimer(200);
}

void WebEngineView::CallWithScroll(PointFCallBack callBack){
    if(!page() || url().isEmpty() || url() == BLANK_URL){
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
            Q_UNUSED(this)
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
    return QPointF(0.5f, 0.5f);
}

void WebEngineView::SetScroll(QPointF pos){
    if(!page()) return;
    page()->runJavaScript(SetScrollRatioPointJsCode(pos), [](QVariant){});
}

bool WebEngineView::SaveScroll(){
    if(!page()) return false;
    page()->runJavaScript
        (GetScrollValuePointJsCode(), [this](QVariant var){
            if(!var.isValid() || !GetViewNode()) return;
            QVariantList list = var.toList();
            GetViewNode()->SetScrollX(list[0].toInt());
            GetViewNode()->SetScrollY(list[1].toInt());
        });
    return true;
}

bool WebEngineView::RestoreScroll(){
    if(!page() || !GetViewNode()) return false;
    if(m_PreventScrollRestoration) return false;
    if(VanillaPage::IsSettingsUrl(url())) return false;
    QPoint pos = QPoint(GetViewNode()->GetScrollX(),
                        GetViewNode()->GetScrollY());
    page()->runJavaScript(SetScrollValuePointJsCode(pos));
    return true;
}

#ifdef MEDIATIME
bool WebEngineView::SaveMediaTime(VoidCallBack settled){
    if(!page()){
        if(settled) settled();
        return false;
    }
    if(IsLoading()){
        if(settled) settled();
        return false;
    }
    const QUrl source = url();
    QPointer<WebEngineView> alive(this);
    page()->runJavaScript
        (GetMediaTimeJsCode(), [alive, source, settled](QVariant var){
            if(!alive){
                if(settled) settled();
                return;
            }
            if(var.isValid() && alive->GetViewNode() && alive->url() == source)
                alive->GetViewNode()->SetMediaTime(var.toFloat());
            if(settled) settled();
        });
    return true;
}

bool WebEngineView::RestoreMediaTime(){
    if(!page() || !GetViewNode()) return false;
    float time = GetViewNode()->GetMediaTime();
    if(time <= 1.0f) return false;
    page()->runJavaScript(SetMediaTimeJsCode(time));
    return true;
}
#endif

bool WebEngineView::SaveZoom(){
    if(!GetViewNode()) return false;
    if(m_ReportedZoomFactor <= 0.0) return false;
    GetViewNode()->SetZoom(static_cast<float>(m_ReportedZoomFactor / DeviceZoomScale()));
    return true;
}

bool WebEngineView::RestoreZoom(){
    if(!GetViewNode()) return false;
    setZoomFactor(static_cast<qreal>(GetViewNode()->GetZoom()) * DeviceZoomScale());
    return true;
}

bool WebEngineView::SaveHistory(){
    if(!GetViewNode()) return false;
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << (*history());
    if(!ba.isEmpty()){
        GetViewNode()->SetHistoryData(ba);
        return true;
    }
    return false;
}

bool WebEngineView::RestoreHistory(){
    if(!GetViewNode()) return false;

    QByteArray ba = GetViewNode()->GetHistoryData();
    if(!ba.isEmpty()){
        QDataStream stream(&ba, QIODevice::ReadOnly);
        stream >> (*history());
        ExtensionHost::StampRequestsOf(page(), this);
#ifdef USE_WEBCHANNEL
#endif
        return history()->count() > 0;
    }
    return false;
}

void WebEngineView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool WebEngineView::SeekText(const QString &str, View::FindFlags opt){
    QWebEnginePage::FindFlags flags = QWebEnginePage::FindFlag();
    if(opt & FindBackward)    flags |= QWebEnginePage::FindBackward;
    if(opt & CaseSensitively) flags |= QWebEnginePage::FindCaseSensitively;

    bool ret = true;
    QWebEngineView::findText(str, flags, [](const QWebEngineFindTextResult &){});
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

void WebEngineView::OnIconChanged(const QIcon &icon){
    Application::RegisterIcon(url().host(), icon);
}

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
    if(page()){
        page()->triggerAction(QWebEnginePage::Unselect);
        page()->runJavaScript(QStringLiteral(
            "(function(){\n"
            "    document.activeElement.blur();\n"
            "}());"));
    }
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
    if(!page()) return;

    QString filename = ModalDialog::GetSaveFileName_
        (QString(), QString(),
         QStringLiteral("Pdf document (*.pdf);;Images (*.jpg *.jpeg *.gif *.png *.bmp *.xpm)"));

    if(filename.isEmpty()) return;

    if(filename.toLower().endsWith(QStringLiteral(".pdf"))){

        page()->printToPdf(filename);

    } else {

        QQuickWidget *widget = findChild<QQuickWidget*>();

        if(!widget) return;

        QSize origSize = size();
        QPointF origPos = page()->scrollPosition();
        resize(page()->contentsSize().toSize());

        QTimer::singleShot(700, this, [this, widget, filename, origSize, origPos](){

        widget->grabFramebuffer().save(filename);

        resize(origSize);
        page()->runJavaScript(SetScrollValuePointJsCode(origPos.toPoint()));

        });
    }
}

void WebEngineView::Save(){
    if(page()) page()->triggerAction(QWebEnginePage::SavePage);
}

void WebEngineView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    setZoomFactor(static_cast<qreal>(zoom) * DeviceZoomScale());
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebEngineView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    setZoomFactor(static_cast<qreal>(zoom) * DeviceZoomScale());
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void WebEngineView::ToggleMediaControls(){
    if(page() && lastContextMenuRequest())
        page()->triggerAction(QWebEnginePage::ToggleMediaControls);
}

void WebEngineView::ToggleMediaLoop(){
    if(page() && lastContextMenuRequest())
         page()->triggerAction(QWebEnginePage::ToggleMediaLoop);
}

void WebEngineView::ToggleMediaPlayPause(){
    if(page() && lastContextMenuRequest())
        page()->triggerAction(QWebEnginePage::ToggleMediaPlayPause);
}

void WebEngineView::ToggleMediaMute(){
    if(page() && lastContextMenuRequest())
        page()->triggerAction(QWebEnginePage::ToggleMediaMute);
}

void WebEngineView::ExitFullScreen(){
    if(page()) page()->triggerAction(QWebEnginePage::ExitFullScreen);
}

void WebEngineView::InspectElement(){
    if(!page()) return;
    if(!m_Inspector){
        m_Inspector = new QWebEngineView();
        m_Inspector->setAttribute(Qt::WA_DeleteOnClose, false);
        m_Inspector->setPage(new QWebEnginePage(NetworkController::InspectorProfile(),
                                                m_Inspector));
        m_Inspector->setZoomFactor(DeviceZoomScale());
        connect(m_Inspector->page(), &QWebEnginePage::loadFinished,
                m_Inspector, [inspector = m_Inspector](bool ok){
                    if(!ok) return;
                    inspector->page()->runJavaScript(QStringLiteral(
                        "(function(){\n"
                        "    var url = new URL(\"core/common/common.js\", location.href).href;\n"
                        "    var tries = 20;\n"
                        "    function nudge(){\n"
                        "        import(url).then(function(mod){\n"
                        "            var theme = mod.Settings.Settings.instance().moduleSetting(\"ui-theme\");\n"
                        "            theme.set(theme.get());\n"
                        "        }).catch(function(){});\n"
                        "        if(--tries > 0) setTimeout(nudge, 500);\n"
                        "    }\n"
                        "    setTimeout(nudge, 500);\n"
                        "})();"));
                });
        connect(m_Inspector->page(), &QWebEnginePage::windowCloseRequested,
                this, &WebEngineView::CloseInspector);
        page()->setDevToolsPage(m_Inspector->page());
    } else {
        m_Inspector->reload();
    }

    MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr;

    if(win && InspectorPane()){
        win->SetInspectorPane(m_Inspector, true);
        return;
    }
    if(win && win->DockedInspectorPane() == m_Inspector)
        win->SetInspectorPane(nullptr);
    m_Inspector->show();
    m_Inspector->raise();
}

void WebEngineView::CloseInspector(){
    if(!m_Inspector) return;
    MainWindow *win = m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr;

    if(win && win->DockedInspectorPane() == m_Inspector)
        win->CloseInspectorPane();
    else
        m_Inspector->hide();
}

void WebEngineView::AddSearchEngine(QPoint pos){
    if(page()) page()->AddSearchEngine(pos);
}

void WebEngineView::AddBookmarklet(QPoint pos){
    if(page()) page()->AddBookmarklet(pos);
}

void WebEngineView::timerEvent(QTimerEvent *ev){
    QWebEngineView::timerEvent(ev);
    if(ev->timerId() == m_ScrollSignalTimer){
        CallWithScroll([this](QPointF pos){ emit ScrollChanged(pos);});
        killTimer(m_ScrollSignalTimer);
        m_ScrollSignalTimer = 0;
    }
#ifdef MEDIATIME
    if(ev->timerId() == m_MediaTimeSaveTimer){
        SaveMediaTime();
    }
#endif
}

void WebEngineView::childEvent(QChildEvent *ev){
    QWebEngineView::childEvent(ev);
    if(ev->added() &&
       ev->child()->inherits("QQuickWidget")){
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
    if(GetDisplayObscured()){
        if(ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_F11){
            ExitFullScreen();
            ev->setAccepted(true);
            return;
        }
    }

    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){
        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }

    if(!m_PreventScrollRestoration &&
       Application::IsMoveKey(ev)){
        m_PreventScrollRestoration = true;
    }

    if(!Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void WebEngineView::keyReleaseEvent(QKeyEvent *ev){
    QWebEngineView::keyReleaseEvent(ev);
}

void WebEngineView::resizeEvent(QResizeEvent *ev){
    QWebEngineView::resizeEvent(ev);
}

void WebEngineView::contextMenuEvent(QContextMenuEvent *ev){
    if(TakeRightButtonConsumed()){
        ev->setAccepted(true);
        return;
    }

    SharedWebElement elem = m_ClickedElement;
    Page::MediaType type = Page::MediaTypeNone;
    QWebEngineContextMenuRequest *data = lastContextMenuRequest();
    if(data){
        if(data->mediaType() == QWebEngineContextMenuRequest::MediaTypeVideo ||
           data->mediaType() == QWebEngineContextMenuRequest::MediaTypeAudio)
            type = Page::MediaTypePlayable;
        else if(data->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage)
            type = Page::MediaTypeImage;
        if(!elem){
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, data->position(), data->linkUrl(), data->mediaUrl(), data->isContentEditable());
            elem = e;
        }
        m_SelectedText = data->selectedText();
    }
    page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos(), type);
    GestureAborted();
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
    if(m_EnableRightGestureLocal &&
       ev->buttons() & Qt::RightButton &&
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
            QWebEngineView::mouseMoveEvent(ev);
            ev->setAccepted(false);
            return;
        }
        QDrag *drag = new QDrag(this);

        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam =
            static_cast<NetworkAccessManager*>(page()->networkAccessManager());

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

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
        if(m_EnableDragGestureLocal){
            GestureMoved(ev->pos());
        } else {
            GestureAborted();
        }
        m_DragStarted = true;
        drag->setMimeData(mime);
        drag->setPixmap(pixmap);
        drag->setHotSpot(pos);
        {
            View::DragOutScope dragging;
            drag->exec(Qt::CopyAction | Qt::MoveAction);
        }
        drag->deleteLater();
        ev->setAccepted(true);
    } else {
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
            if(!View::TriggerAction(str, ev->pos())){
                m_SpentButtons.Press(ev->button(), false);
                ev->setAccepted(false);
                return;
            }
            m_SpentButtons.Press(ev->button(), true);
            GestureAborted();
            ev->setAccepted(true);
            return;
        }
    }

    m_SpentButtons.Press(ev->button(), false);
    GestureStarted(ev->pos());
    QWebEngineView::mousePressEvent(ev);
    ev->setAccepted(false);
}

void WebEngineView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    if(m_SpentButtons.Settle(ev->button())){
        ev->setAccepted(true);
        return;
    }

    if(m_DragStarted){
        m_DragStarted = false;
        ev->setAccepted(true);
        return;
    }

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MiddleButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(Application::HasShiftModifier(ev) ||
           Application::HasCtrlModifier(ev) ||
           ev->button() == Qt::MiddleButton){

            GestureAborted();
            m_TreeBank->OpenInNewViewNode(req, Page::Activate(), GetViewNode());
            ev->setAccepted(true);
            return;

        }
    }

    if(ev->button() == Qt::RightButton){

        if(!m_Gesture.isEmpty()){
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            ev->setAccepted(false);
            return;
        }
        ev->setAccepted(true);
        return;
    }

    if(ev->button() == Qt::LeftButton && !m_Gesture.isEmpty())
        ;
    else
        GestureAborted();

    QWebEngineView::mouseReleaseEvent(ev);
    ev->setAccepted(false);
}

void WebEngineView::mouseDoubleClickEvent(QMouseEvent *ev){
    QString mouse;
    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());
    if(m_MouseMap.contains(mouse) && !m_MouseMap[mouse].isEmpty()){
        mousePressEvent(ev);
        return;
    }
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
    if(m_EnableDragGestureLocal && !m_GestureStartedPos.isNull()){

        GestureMoved(ev->position().toPoint());
        QString gesture = GestureToString(m_Gesture);
        QString action =
            !m_DragGestureMap.contains(gesture)
              ? tr("NoAction")
            : Page::IsValidAction(m_DragGestureMap[gesture])
              ? Action(Page::StringToAction(m_DragGestureMap[gesture]))->text()
            : m_DragGestureMap[gesture];
        emit statusBarMessage(gesture + QStringLiteral(" (") + action + QStringLiteral(")"));
    }
    QWebEngineView::dragMoveEvent(ev);
    ev->setAccepted(true);
}

void WebEngineView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    bool isLocal = false;
    QPoint pos = ev->position().toPoint();
    QObject *source = ev->source();
    QString text = ev->mimeData()->text();
    QList<QUrl> urls = Page::MimeDataToUrls(ev->mimeData(), source);

    foreach(QUrl u, urls){ if(u.isLocalFile()) isLocal = true;}

    if(text.isEmpty() && !urls.isEmpty()){
        foreach(QUrl u, urls){
            if(text.isEmpty()) text = u.toString();
            else text += QStringLiteral("\n") + u.toString();
        }
    }

    const bool own = IsOwnDragSource(source);

    CallWithHitElement(pos, [this, pos, own, text, urls](SharedWebElement elem){

    if(elem && !elem->IsNull() && (elem->IsEditableElement() || elem->IsTextInputElement())){

        GestureAborted();
        elem->SetText(text);
        return;
    }

    if(!m_Gesture.isEmpty() && own){
        GestureFinished(pos, Qt::LeftButton);
        return;
    }

    GestureAborted();

    if(!urls.isEmpty() && !own){
        m_TreeBank->OpenInNewViewNode(urls, true, GetViewNode());
    }

    });

    const bool consumed = (!m_Gesture.isEmpty() && own) || (!urls.isEmpty() && !own);

#if defined(Q_OS_MAC)
#else
    if(isLocal || consumed ||
       (DragToStartDownload() && !urls.isEmpty() && own))
        ;
    else {
        if(own && !urls.isEmpty()) MarkOwnDrop();
        QWebEngineView::dropEvent(ev);
    }
#endif

    ev->setAccepted(true);
}

void WebEngineView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QWebEngineView::dragLeaveEvent(ev);
}

void WebEngineView::wheelEvent(QWheelEvent *ev){
    m_PreventScrollRestoration = true;
    ev->setAccepted(false);
}

void WebEngineView::EngineWheel(QWheelEvent *ev){
    QWidget *widget = focusProxy();
    if(!widget){
        ev->setAccepted(false);
        return;
    }
    m_PreventScrollRestoration = true;
    const QPointF local = widget->mapFromGlobal(ev->globalPosition());
    QWheelEvent scaled(local, ev->globalPosition(),
                       ev->pixelDelta(),
                       View::EngineWheelAngle(ev->angleDelta()),
                       ev->buttons(), ev->modifiers(), ev->phase(),
                       ev->inverted(), ev->source(),
                       ev->pointingDevice());
    scaled.setTimestamp(ev->timestamp());
    QApplication::sendEvent(widget, &scaled);
    ev->setAccepted(true);
}

void WebEngineView::WheelEvent(QWheelEvent *ev){
    if(ev->source() != Qt::MouseEventSynthesizedBySystem){
        QString wheel;
        bool up = Application::WheelWentUp(ev);

        Application::AddModifiersToString(wheel, ev->modifiers());
        Application::AddMouseButtonsToString(wheel, ev->buttons());
        Application::AddWheelDirectionToString(wheel, up);

        if(m_MouseMap.contains(wheel)){
            QString str = m_MouseMap[wheel];
            if(!str.isEmpty()){
                GestureAborted();
                if(ev->buttons() & Qt::RightButton)
                    View::ConsumeRightButton();
                View::TriggerAction(str, mapFromGlobal(ev->globalPosition()));
            }
            ev->setAccepted(true);
            return;
        }
    }
    EngineWheel(ev);
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

void WebEngineView::CallWithGotBaseUrl(UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == BLANK_URL)
        return callBack(QUrl());

    page()->runJavaScript
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == BLANK_URL)
        return callBack(QUrl());

    page()->runJavaScript
        (GetCurrentBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithFoundElements(Page::FindElementsOption option,
                                          WebElementListCallBack callBack){
    if(!page() || url().isEmpty() || url() == BLANK_URL)
        return callBack(SharedWebElementList());

    page()->runJavaScript
        (FindElementsJsCode(option), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElementList());
            QVariantList list = var.toMap().values();
            SharedWebElementList result;

            MainWindow *win = Application::GetCurrentWindow();
            QSize s =
                m_TreeBank ? m_TreeBank->ViewSize() :
                win ? win->GetTreeBank()->ViewSize() :
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
    if(!page() || url().isEmpty() || url() == BLANK_URL || pos.isNull())
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
    if(!page() || url().isEmpty() || url() == BLANK_URL || pos.isNull())
        return callBack(QUrl());

    page()->runJavaScript
        (HitLinkUrlJsCode(pos/zoomFactor()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void WebEngineView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(!page() || url().isEmpty() || url() == BLANK_URL || pos.isNull())
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
    if(!page() || url().isEmpty() || url() == BLANK_URL)
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
            if(!var.isValid() || !var.canConvert<QVariantMap>())
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
    if(!page() || url().isEmpty() || url() == BLANK_URL)
        return callBack(QVariant());

    page()->runJavaScript(code, callBack);
}

#endif
