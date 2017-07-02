#include "switch.hpp"
#include "const.hpp"

#ifdef NATIVEWEBVIEW

#include "quicknativewebview.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>

#include "view.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "dialog.hpp"

#include <memory>

QuickNativeWebView::QuickNativeWebView(TreeBank *parent, QString id, QStringList set)
    : QQuickWidget(QUrl(QStringLiteral("qrc:/view/quicknativewebview.qml")), parent)
    , View(parent, id, set)
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlNativeWebView = rootObject();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new Page(this, nam);
    page()->SetView(this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_PreventScrollRestoration = false;
#ifdef PASSWORD_MANAGER
    m_PreventAuthRegistration = false;
#endif

    m_ActionTable = QMap<Page::CustomAction, QAction*>();
    m_RequestId = 0;

    m_Icon = QIcon();
    connect(this, SIGNAL(iconUrlChanged(const QUrl&)),
            this, SLOT(UpdateIcon(const QUrl&)));

    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(HandleWindowClose()));
    connect(this, SIGNAL(javascriptConsoleMessage(int, const QString&)),
            this, SLOT(HandleJavascriptConsoleMessage(int, const QString&)));
    connect(this, SIGNAL(featurePermissionRequested(const QUrl&, int)),
            this, SLOT(HandleFeaturePermission(const QUrl&, int)));
    connect(this, SIGNAL(renderProcessTerminated(int, int)),
            this, SLOT(HandleRenderProcessTermination(int, int)));
    connect(this, SIGNAL(fullScreenRequested(bool)),
            this, SLOT(HandleFullScreen(bool)));
    connect(this, SIGNAL(downloadRequested(QObject*)),
            this, SLOT(HandleDownload(QObject*)));

    connect(m_QmlNativeWebView, SIGNAL(callBackResult(int, QVariant)),
            this,               SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlNativeWebView, SIGNAL(viewChanged()),
            this,               SIGNAL(ViewChanged()));
    connect(m_QmlNativeWebView, SIGNAL(scrollChanged(QPointF)),
            this,               SIGNAL(ScrollChanged(QPointF)));
}

QuickNativeWebView::~QuickNativeWebView(){
}

void QuickNativeWebView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);
}

QQuickWidget *QuickNativeWebView::base(){
    return static_cast<QQuickWidget*>(this);
}

Page *QuickNativeWebView::page(){
    return static_cast<Page*>(View::page());
}

QUrl QuickNativeWebView::url(){
    return m_QmlNativeWebView->property("url").toUrl();
}

QString QuickNativeWebView::html(){
    return WholeHtml();
}

TreeBank *QuickNativeWebView::parent(){
    return m_TreeBank;
}

void QuickNativeWebView::setUrl(const QUrl &url){
    m_QmlNativeWebView->setProperty("url", url);
    emit urlChanged(url);
}

void QuickNativeWebView::setHtml(const QString &html, const QUrl &url){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "loadHtml",
                              Q_ARG(QString, html),
                              Q_ARG(QUrl,    url));
    emit urlChanged(url);
}

void QuickNativeWebView::setParent(TreeBank* t){
    View::SetTreeBank(t);
    base()->setParent(t);
}

void QuickNativeWebView::Connect(TreeBank *tb){
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
        connect(this, SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
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

void QuickNativeWebView::Disconnect(TreeBank *tb){
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
        disconnect(this, SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
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

void QuickNativeWebView::OnSetViewNode(ViewNode*){}

void QuickNativeWebView::OnSetHistNode(HistNode*){}

void QuickNativeWebView::OnSetThis(WeakView){}

void QuickNativeWebView::OnSetMaster(WeakView){}

void QuickNativeWebView::OnSetSlave(WeakView){}

void QuickNativeWebView::OnSetJsObject(_View*){}

void QuickNativeWebView::OnSetJsObject(_Vanilla*){}

void QuickNativeWebView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;

    if(m_Icon.isNull() && url() != BLANK_URL)
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
}

void QuickNativeWebView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    // loadProgress: 100% signal is emitted after loadFinished.
    if(progress != 100)
        emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void QuickNativeWebView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

    if(!ok){
        emit statusBarMessage(tr("Failed to load."));
        return;
    }

    RestoreScroll();
    emit ViewChanged();
    emit statusBarMessage(tr("Finished loading."));

#ifdef PASSWORD_MANAGER
    QString data = Application::GetAuthDataWithNoDialog
        (page()->GetNetworkAccessManager()->GetId() +
         QStringLiteral(":") + url().host());

    if(!data.isEmpty())
        CallWithEvaluatedJavaScriptResult(DecorateFormFieldJsCode(data), [](QVariant){});

    CallWithEvaluatedJavaScriptResult(InstallSubmitEventJsCode(), [](QVariant){});
#endif //ifdef PASSWORD_MANAGER

    static const QList<QEvent::Type> types =
        QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease;

    CallWithEvaluatedJavaScriptResult(InstallEventFilterJsCode(types), [](QVariant){});

    if(visible() && m_TreeBank &&
       m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
        UpdateThumbnail();
    }
}

void QuickNativeWebView::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void QuickNativeWebView::OnUrlChanged(const QUrl &url){
    if(!GetHistNode()) return;
    ChangeNodeUrl(url);
}

void QuickNativeWebView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickNativeWebView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void QuickNativeWebView::EmitScrollChanged(){
    if(!m_ScrollSignalTimer)
        m_ScrollSignalTimer = startTimer(200);
}

void QuickNativeWebView::CallWithScroll(PointFCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetScrollRatioPointJsCode(), [callBack](QVariant var){
            if(!var.isValid()) return callBack(QPointF(0.5f, 0.5f));
            QVariantList list = var.toList();
            callBack(QPointF(list[0].toFloat(), list[1].toFloat()));
        });
}

void QuickNativeWebView::SetScrollBarState(){
    CallWithEvaluatedJavaScriptResult
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

QPointF QuickNativeWebView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    // this function does not return actual value on NativeWebView.
    return QPointF(0.5f, 0.5f);
}

void QuickNativeWebView::SetScroll(QPointF pos){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "setScroll",
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

bool QuickNativeWebView::SaveScroll(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "saveScroll");
    return true;
}

bool QuickNativeWebView::RestoreScroll(){
    if(size().isEmpty()) return false;
    if(m_PreventScrollRestoration) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "restoreScroll");
    return true;
}

bool QuickNativeWebView::SaveZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "saveZoom");
    return true;
}

bool QuickNativeWebView::RestoreZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlNativeWebView, "restoreZoom");
    return true;
}

void QuickNativeWebView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickNativeWebView::SeekText(const QString &str, View::FindFlags opt){
    //QMetaObject::invokeMethod(m_QmlNativeWebView, "seekText",
    //                          Q_ARG(QVariant, QVariant::fromValue(str)),
    //                          Q_ARG(QVariant, QVariant::fromValue(static_cast<int>(opt))));
    //return true;

    // not yet implemented.
    Q_UNUSED(str); Q_UNUSED(opt);
    return false;
}

void QuickNativeWebView::SetFocusToElement(QString xpath){
    CallWithEvaluatedJavaScriptResult(SetFocusToElementJsCode(xpath), [](QVariant){});
}

void QuickNativeWebView::FireClickEvent(QString xpath, QPoint pos){
    //qreal zoom = m_QmlNativeWebView->property("zoomFactor").toReal();
    //CallWithEvaluatedJavaScriptResult(FireClickEventJsCode(xpath, pos/zoom), [](QVariant){});

    // not yet implemented.
    Q_UNUSED(xpath); Q_UNUSED(pos);
}

void QuickNativeWebView::SetTextValue(QString xpath, QString text){
    CallWithEvaluatedJavaScriptResult(SetTextValueJsCode(xpath, text), [](QVariant){});
}

void QuickNativeWebView::UpdateIcon(const QUrl &iconUrl){
    m_Icon = QIcon();
    if(!page()) return;
    QString host = url().host();
    QNetworkRequest req(iconUrl);
    DownloadItem *item = NetworkController::Download
        (page()->GetNetworkAccessManager(),
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

void QuickNativeWebView::HandleWindowClose(){
    QTimer::singleShot(0, page(), &Page::Close);
}

void QuickNativeWebView::HandleJavascriptConsoleMessage(int level, const QString &msg){
    // 0: InfoMessageLevel
    if(level != 0) return;
#ifdef PASSWORD_MANAGER
    static QString reg;
    if(reg.isEmpty()) reg = QStringLiteral("submit%1,([^,]+)").arg(Application::EventKey());

    if(!m_PreventAuthRegistration &&
       Application::ExactMatch(reg, msg)){

        Application::RegisterAuthData
            (page()->GetNetworkAccessManager()->GetId() +
             QStringLiteral(":") + url().host(),
             msg.split(QStringLiteral(","))[1]);
        return;
    }
#endif //ifdef PASSWORD_MANAGER
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        KeyPressEvent(&ke);
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyRelease, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        KeyReleaseEvent(&ke);
    } else if(Application::ExactMatch(QStringLiteral("preventScrollRestoration%1").arg(Application::EventKey()), msg)){
        m_PreventScrollRestoration = true;
    }
}

void QuickNativeWebView::HandleFeaturePermission(const QUrl &securityOrigin, int feature){
    QString featureString;
    switch(feature){
    case 0: //QQuickNativeWebView::MediaAudioCapture:
        featureString = QStringLiteral("MediaAudioCapture");      break;
    case 1: //QQuickNativeWebView::MediaVideoCapture:
        featureString = QStringLiteral("MediaVideoCapture");      break;
    case 2: //QQuickNativeWebView::MediaAudioVideoCapture:
        featureString = QStringLiteral("MediaAudioVideoCapture"); break;
    case 3: //QQuickNativeWebView::Geolocation:
        featureString = QStringLiteral("Geolocation");            break;
    default: return;
    }

    ModalDialog *dialog = new ModalDialog();
    dialog->SetTitle(tr("Feature Permission Requested."));
    dialog->SetCaption(tr("Feature Permission Requested."));
    dialog->SetInformativeText
        (tr("Url: ") + securityOrigin.toString() + QStringLiteral("\n") +
         tr("Feature: ") + featureString + QStringLiteral("\n\n") +
         tr("Allow this feature?"));
    dialog->SetButtons(QStringList() << tr("Yes") << tr("No") << tr("Cancel"));
    dialog->Execute();

    QString text = dialog->ClickedButton();
    if(text == tr("Yes")){
        QMetaObject::invokeMethod(m_QmlNativeWebView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(true)));
    } else if(text == tr("No")){
        QMetaObject::invokeMethod(m_QmlNativeWebView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    } else if(text == tr("Cancel")){
        // nothing to do.
    }
}

void QuickNativeWebView::HandleRenderProcessTermination(int status, int code){
    QString info = tr("A page is reloaded, because that's process is terminated.\n");
    switch(status){
    case 0: //QQuickNativeWebView::NormalTerminationStatus:
        info += tr("Normal termination. (code: %1)");   break;
    case 1: //QQuickNativeWebView::AbnormalTerminationStatus:
        info += tr("Abnormal termination. (code: %1)"); break;
    case 2: //QQuickNativeWebView::CrashedTerminationStatus:
        info += tr("Crashed termination. (code: %1)");  break;
    case 3: //QQuickNativeWebView::KilledTerminationStatus:
        info += tr("Killed termination. (code: %1)");   break;
    }
    ModelessDialog::Information(tr("Render process terminated."),
                                info.arg(code), base());
    QTimer::singleShot(0, m_QmlNativeWebView, SLOT(reload()));
}

void QuickNativeWebView::HandleFullScreen(bool on){
    if(TreeBank *tb = GetTreeBank()){
        tb->GetMainWindow()->SetFullScreen(on);
        SetDisplayObscured(on);
        if(!on) return;
        ModelessDialog *dialog = new ModelessDialog();
        // connect to 'Returned', because default value is true.
        connect(this, &QuickNativeWebView::destroyed, dialog, &ModelessDialog::Returned);
        connect(this, &QuickNativeWebView::fullScreenRequested, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(tr("This page becomes full screen mode."));
        dialog->SetCaption(tr("Press Esc to exit."));
        dialog->SetButtons(QStringList() << tr("OK") << tr("Cancel"));
        dialog->SetDefaultValue(true);
        dialog->SetCallBack([this](bool ok){ if(!ok) ExitFullScreen();});
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    }
}

void QuickNativeWebView::HandleDownload(QObject *object){
    page()->GetNetworkAccessManager()->HandleDownload(object);
}

void QuickNativeWebView::Copy(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "copy");
}

void QuickNativeWebView::Cut(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "cut");
}

void QuickNativeWebView::Paste(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "paste");
}

void QuickNativeWebView::Undo(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "undo");
}

void QuickNativeWebView::Redo(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "redo");
}

void QuickNativeWebView::SelectAll(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "selectAll");
}

void QuickNativeWebView::Unselect(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "unselect");
}

void QuickNativeWebView::Reload(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "reload");
}

void QuickNativeWebView::ReloadAndBypassCache(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "reloadAndBypassCache");
}

void QuickNativeWebView::Stop(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "stop");
}

void QuickNativeWebView::StopAndUnselect(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "stopAndUnselect");
}

void QuickNativeWebView::Print(){

    QString filename = ModalDialog::GetSaveFileName_
        (QString(), QString(),
         QStringLiteral("Pdf document (*.pdf);;Images (*.jpg *.jpeg *.gif *.png *.bmp *.xpm)"));

    if(filename.isEmpty()) return;

    if(filename.toLower().endsWith(".pdf")){

        QMetaObject::invokeMethod(m_QmlNativeWebView, "printToPdf",
                                  Q_ARG(QString, filename));
    } else {
        QSize origSize = size();
        QPointF origPos = m_QmlNativeWebView->property("scrollPosition").toPointF();
        QSizeF contentsSize = m_QmlNativeWebView->property("contentsSize").toSizeF();
        resize(contentsSize.toSize());

        QTimer::singleShot(700, this, [this, filename, origSize, origPos](){

        grabFramebuffer().save(filename);

        resize(origSize);
        CallWithEvaluatedJavaScriptResult
            (SetScrollValuePointJsCode(origPos.toPoint()), [](QVariant){});

        });
    }
}

void QuickNativeWebView::Save(){
    if(!page()) return;
    QNetworkRequest req(url());
    req.setRawHeader("Referer", url().toEncoded());
    page()->Download(req);
}

void QuickNativeWebView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    m_QmlNativeWebView->setProperty("zoomFactor", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickNativeWebView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    m_QmlNativeWebView->setProperty("zoomFactor", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickNativeWebView::ToggleMediaControls(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaControls");
}

void QuickNativeWebView::ToggleMediaLoop(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaLoop");
}

void QuickNativeWebView::ToggleMediaPlayPause(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaPlayPause");
}

void QuickNativeWebView::ToggleMediaMute(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "toggleMediaMute");
}

void QuickNativeWebView::ExitFullScreen(){
    QMetaObject::invokeMethod(m_QmlNativeWebView, "fullScreenCancelled");
}

void QuickNativeWebView::AddSearchEngine(QPoint pos){
    Q_UNUSED(pos);
    // not yet implemented.
    //if(page()) page()->AddSearchEngine(pos);
}

void QuickNativeWebView::AddBookmarklet(QPoint pos){
    Q_UNUSED(pos);
    // not yet implemented.
    //if(page()) page()->AddBookmarklet(pos);
}

void QuickNativeWebView::timerEvent(QTimerEvent *ev){
    QQuickWidget::timerEvent(ev);
    if(ev->timerId() == m_ScrollSignalTimer){
        QMetaObject::invokeMethod(m_QmlNativeWebView, "emitScrollChanged");
        killTimer(m_ScrollSignalTimer);
        m_ScrollSignalTimer = 0;
    }
}

void QuickNativeWebView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QQuickWidget::hideEvent(ev);
}

void QuickNativeWebView::showEvent(QShowEvent *ev){
    m_PreventScrollRestoration = false;
    QQuickWidget::showEvent(ev);
    RestoreViewState();
}

void QuickNativeWebView::keyPressEvent(QKeyEvent *ev){
    if(!visible()) return;

    if(GetDisplayObscured()){
        if(ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_F11){
            ExitFullScreen();
            ev->setAccepted(true);
            return;
        }
    }

#ifdef PASSWORD_MANAGER
    if(ev->modifiers() & Qt::ControlModifier &&
       ev->key() == Qt::Key_Return){

        QString data = Application::GetAuthData
            (page()->GetNetworkAccessManager()->GetId() +
             QStringLiteral(":") + url().host());

        if(!data.isEmpty()){
            m_PreventAuthRegistration = true;
            CallWithEvaluatedJavaScriptResult
                (View::SubmitFormDataJsCode(data),
                 [this](QVariant){
                    m_PreventAuthRegistration = false;
                });
            ev->setAccepted(true);
            return;
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

    int k = ev->key();
    if(!m_PreventScrollRestoration &&
       (k == Qt::Key_Space ||
        k == Qt::Key_Up ||
        k == Qt::Key_Down ||
        k == Qt::Key_Right ||
        k == Qt::Key_Left ||
        k == Qt::Key_PageUp ||
        k == Qt::Key_PageDown ||
        k == Qt::Key_Home ||
        k == Qt::Key_End)){

        m_PreventScrollRestoration = true;
        return;
    }

    //QQuickWidget::keyPressEvent(ev);

    if(/*!ev->isAccepted() &&*/
       !Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void QuickNativeWebView::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev);

    if(!visible()) return;

    //QQuickWidget::keyReleaseEvent(ev);
}

void QuickNativeWebView::resizeEvent(QResizeEvent *ev){
    QQuickWidget::resizeEvent(ev);
}

void QuickNativeWebView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickNativeWebView::mouseMoveEvent(QMouseEvent *ev){
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
            page()->GetNetworkAccessManager();

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

void QuickNativeWebView::mousePressEvent(QMouseEvent *ev){
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

void QuickNativeWebView::mouseReleaseEvent(QMouseEvent *ev){
    emit statusBarMessage(QString());

    QUrl link = m_ClickedElement ? m_ClickedElement->LinkUrl() : QUrl();

    if(!link.isEmpty() &&
       m_Gesture.isEmpty() &&
       (ev->button() == Qt::LeftButton ||
        ev->button() == Qt::MidButton)){

        QNetworkRequest req(link);
        req.setRawHeader("Referer", url().toEncoded());

        if(ev->modifiers() & Qt::ShiftModifier ||
           ev->modifiers() & Qt::ControlModifier ||
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
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos());
            GestureAborted();
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QQuickWidget::mouseReleaseEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::mouseDoubleClickEvent(QMouseEvent *ev){
    QQuickWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickNativeWebView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QQuickWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::dragMoveEvent(QDragMoveEvent *ev){
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

void QuickNativeWebView::dropEvent(QDropEvent *ev){
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

    CallWithHitElement(pos, [this, pos, source, text, urls](SharedWebElement elem){

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

    if(!urls.isEmpty() && source != this){
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

void QuickNativeWebView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QQuickWidget::dragLeaveEvent(ev);
}

void QuickNativeWebView::wheelEvent(QWheelEvent *ev){
    if(!visible()) return;

    if(ev->source() != Qt::MouseEventSynthesizedBySystem){
        QString wheel;
        bool up = ev->delta() > 0;

        Application::AddModifiersToString(wheel, ev->modifiers());
        Application::AddMouseButtonsToString(wheel, ev->buttons());
        Application::AddWheelDirectionToString(wheel, up);

        if(m_MouseMap.contains(wheel)){

            QString str = m_MouseMap[wheel];
            if(!str.isEmpty()){
                View::TriggerAction(str, ev->pos());
            }
            ev->setAccepted(true);
            return;
        }
    }
    m_PreventScrollRestoration = true;
    QQuickWidget::wheelEvent(ev);
    ev->setAccepted(true);
}

void QuickNativeWebView::focusInEvent(QFocusEvent *ev){
    QQuickWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickNativeWebView::focusOutEvent(QFocusEvent *ev){
    QQuickWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickNativeWebView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QQuickWidget::focusNextPrevChild(next);
    return false;
}

void QuickNativeWebView::CallWithGotBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetCurrentBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithFoundElements(Page::FindElementsOption option,
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

void QuickNativeWebView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(pos.isNull()) return callBack(SharedWebElement());
    CallWithEvaluatedJavaScriptResult
        (HitElementJsCode(pos / m_HistNode->GetZoom()), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElement());
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void QuickNativeWebView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitLinkUrlJsCode(pos / m_HistNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitImageUrlJsCode(pos / m_HistNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickNativeWebView::CallWithSelectedText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithSelectedHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithWholeText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithWholeHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickNativeWebView::CallWithSelectionRegion(RegionCallBack callBack){
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

void QuickNativeWebView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                           VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickNativeWebView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlNativeWebView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

#endif //ifdef NATIVEWEBVIEW
