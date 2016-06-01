#include "switch.hpp"
#include "const.hpp"

#include "quickwebengineview.hpp"

#include <QQmlContext>
#include <QAction>
#include <QVariant>
#include <QDrag>
#ifdef PASSWORD_MANAGER
# include <QWebEngineProfile>
#endif

#include "view.hpp"
#include "webenginepage.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "mainwindow.hpp"

#include <memory>

QMap<View*, QUrl> QuickWebEngineView::m_InspectorTable = QMap<View*, QUrl>();

QuickWebEngineView::QuickWebEngineView(TreeBank *parent, QString id, QStringList set)
    : View(parent, id, set)
#if QT_VERSION >= 0x050700
    , QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebengineview5.7.qml")), parent)
#else
    , QQuickWidget(QUrl(QStringLiteral("qrc:/view/quickwebengineview5.6.qml")), parent)
#endif
{
    Initialize();
    rootContext()->setContextProperty(QStringLiteral("viewInterface"), this);

    m_QmlWebEngineView = rootObject()->childItems().first();

    NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id, set);
    m_Page = new WebEnginePage(nam, this);
    ApplySpecificSettings(set);

    if(parent) setParent(parent);

    m_Inspector = 0;
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

    connect(m_QmlWebEngineView, SIGNAL(callBackResult(int, QVariant)),
            this,               SIGNAL(CallBackResult(int, QVariant)));

    connect(m_QmlWebEngineView, SIGNAL(viewChanged()),
            this,               SIGNAL(ViewChanged()));
    connect(m_QmlWebEngineView, SIGNAL(scrollChanged(QPointF)),
            this,               SIGNAL(ScrollChanged(QPointF)));
}

QuickWebEngineView::~QuickWebEngineView(){
    m_InspectorTable.remove(this);
    if(m_Inspector) m_Inspector->deleteLater();
}

void QuickWebEngineView::ApplySpecificSettings(QStringList set){
    View::ApplySpecificSettings(set);

    if(!page()) return;

    SetPreference(QWebEngineSettings::AutoLoadImages,                    "AutoLoadImages");
    SetPreference(QWebEngineSettings::JavascriptCanAccessClipboard,      "JavascriptCanAccessClipboard");
    SetPreference(QWebEngineSettings::JavascriptCanOpenWindows,          "JavascriptCanOpenWindows");
    SetPreference(QWebEngineSettings::JavascriptEnabled,                 "JavascriptEnabled");
    SetPreference(QWebEngineSettings::LinksIncludedInFocusChain,         "LinksIncludedInFocusChain");
    SetPreference(QWebEngineSettings::LocalContentCanAccessFileUrls,     "LocalContentCanAccessFileUrls");
    SetPreference(QWebEngineSettings::LocalContentCanAccessRemoteUrls,   "LocalContentCanAccessRemoteUrls");
    SetPreference(QWebEngineSettings::LocalStorageEnabled,               "LocalStorageEnabled");
    SetPreference(QWebEngineSettings::PluginsEnabled,                    "PluginsEnabled");
    SetPreference(QWebEngineSettings::SpatialNavigationEnabled,          "SpatialNavigationEnabled");
    SetPreference(QWebEngineSettings::HyperlinkAuditingEnabled,          "HyperlinkAuditingEnabled");
    SetPreference(QWebEngineSettings::ScrollAnimatorEnabled,             "ScrollAnimatorEnabled");
#if QT_VERSION >= 0x050700
    SetPreference(QWebEngineSettings::ScreenCaptureEnabled,              "ScreenCaptureEnabled");
    SetPreference(QWebEngineSettings::WebAudioEnabled,                   "WebAudioEnabled");
    SetPreference(QWebEngineSettings::WebGLEnabled,                      "WebGLEnabled");
    SetPreference(QWebEngineSettings::Accelerated2dCanvasEnabled,        "Accelerated2dCanvasEnabled");
    SetPreference(QWebEngineSettings::AutoLoadIconsForPage,              "AutoLoadIconsForPage");
    SetPreference(QWebEngineSettings::TouchIconsEnabled,                 "TouchIconsEnabled");
#endif
    SetPreference(QWebEngineSettings::ErrorPageEnabled,                  "ErrorPageEnabled");
    SetPreference(QWebEngineSettings::FullScreenSupportEnabled,          "FullScreenSupportEnabled");

    SetFontFamily(QWebEngineSettings::StandardFont,  "StandardFont");
    SetFontFamily(QWebEngineSettings::FixedFont,     "FixedFont");
    SetFontFamily(QWebEngineSettings::SerifFont,     "SerifFont");
    SetFontFamily(QWebEngineSettings::SansSerifFont, "SansSerifFont");
    SetFontFamily(QWebEngineSettings::CursiveFont,   "CursiveFont");
    SetFontFamily(QWebEngineSettings::FantasyFont,   "FantasyFont");

    SetFontSize(QWebEngineSettings::MinimumFontSize,        "MinimumFontSize");
    SetFontSize(QWebEngineSettings::MinimumLogicalFontSize, "MinimumLogicalFontSize");
    SetFontSize(QWebEngineSettings::DefaultFontSize,        "DefaultFontSize");
    SetFontSize(QWebEngineSettings::DefaultFixedFontSize,   "DefaultFixedFontSize");

    QMetaObject::invokeMethod(m_QmlWebEngineView, "setUserAgent",
                              Q_ARG(QVariant, QVariant::fromValue(page()->userAgentForUrl(QUrl()))));
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setDefaultTextEncoding",
                              Q_ARG(QVariant, QVariant::fromValue(page()->settings()->defaultTextEncoding())));
}

QQuickWidget *QuickWebEngineView::base(){
    return static_cast<QQuickWidget*>(this);
}

WebEnginePage *QuickWebEngineView::page(){
    return static_cast<WebEnginePage*>(View::page());
}

QUrl QuickWebEngineView::url(){
    return m_QmlWebEngineView->property("url").toUrl();
}

QString QuickWebEngineView::html(){
    return WholeHtml();
}

TreeBank *QuickWebEngineView::parent(){
    return m_TreeBank;
}

void QuickWebEngineView::setUrl(const QUrl &url){
    m_QmlWebEngineView->setProperty("url", url);
    emit urlChanged(url);
}

void QuickWebEngineView::setHtml(const QString &html, const QUrl &url){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "loadHtml",
                              Q_ARG(QString, html),
                              Q_ARG(QUrl,    url));
    emit urlChanged(url);
}

void QuickWebEngineView::setParent(TreeBank* t){
    View::SetTreeBank(t);
    base()->setParent(t);
}

void QuickWebEngineView::Connect(TreeBank *tb){
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

void QuickWebEngineView::Disconnect(TreeBank *tb){
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

void QuickWebEngineView::OnSetViewNode(ViewNode*){}

void QuickWebEngineView::OnSetHistNode(HistNode*){}

void QuickWebEngineView::OnSetThis(WeakView){}

void QuickWebEngineView::OnSetMaster(WeakView){}

void QuickWebEngineView::OnSetSlave(WeakView){}

void QuickWebEngineView::OnSetJsObject(_View*){}

void QuickWebEngineView::OnSetJsObject(_Vanilla*){}

void QuickWebEngineView::OnLoadStarted(){
    if(!GetHistNode()) return;

    View::OnLoadStarted();

    emit statusBarMessage(tr("Started loading."));
    m_PreventScrollRestoration = false;
    AssignInspector();
#ifdef USE_WEBCHANNEL
    //page()->AddJsObject();
#endif

    if(m_Icon.isNull() && url() != QUrl(QStringLiteral("about:blank")))
        UpdateIcon(QUrl(url().resolved(QUrl("/favicon.ico"))));
}

void QuickWebEngineView::OnLoadProgress(int progress){
    if(!GetHistNode()) return;
    View::OnLoadProgress(progress);
    emit statusBarMessage(tr("Loading ... (%1 percent)").arg(progress));
}

void QuickWebEngineView::OnLoadFinished(bool ok){
    if(!GetHistNode()) return;

    View::OnLoadFinished(ok);

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
        CallWithEvaluatedJavaScriptResult(DecorateFormFieldJsCode(data),
                                          [](QVariant){});
    }

    CallWithEvaluatedJavaScriptResult(QStringLiteral(
        "(function(){\n"
      VV"    var forms = document.querySelectorAll(\"form\");\n"
      VV"    for(var i = 0; i < forms.length; i++){\n"
      VV"        var form = forms[i];\n"
      VV"        var submit   = form.querySelector(\"*[type=\\\"submit\\\"]\")   || form.submit;\n"
      VV"        var password = form.querySelector(\"*[type=\\\"password\\\"]\") || form.password;\n"
      VV"        if(!submit || !password) continue;\n"
      VV"        form.addEventListener(\"submit\", function(e){\n"
      VV"            var data = \"\";\n"
      VV"            var inputs = e.target.querySelectorAll(\"input,textarea\");\n"
      VV"            for(var j = 0; j < inputs.length; j++){\n"
      VV"                var field = inputs[j];\n"
      VV"                var type = (field.type || \"hidden\").toLowerCase();\n"
      VV"                var name = field.name;\n"
      VV"                var val = field.value;\n"
      VV"                if(!name || type == \"hidden\" || type == \"submit\"){\n"
      VV"                    continue;\n"
      VV"                }\n"
      VV"                if(data) data = data + \"&\";\n"
      VV"                data = data + encodeURIComponent(name) + \"=\" + encodeURIComponent(val);\n"
      VV"            }\n"
      VV"            console.info(\"submit%1,\" + data);\n"
      VV"        }, false);\n"
      VV"    }\n"
      VV"})()").arg(Application::EventKey()), [](QVariant){});
#endif //ifdef PASSWORD_MANAGER

    static const QList<QEvent::Type> types =
        QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease;

    CallWithEvaluatedJavaScriptResult(InstallEventFilterJsCode(types),
                                      [](QVariant){});

    if(visible() && m_TreeBank &&
       m_TreeBank->GetMainWindow()->GetTreeBar()->isVisible()){
        UpdateThumbnail();
    }
}

void QuickWebEngineView::OnTitleChanged(const QString &title){
    if(!GetHistNode()) return;
    ChangeNodeTitle(title);
}

void QuickWebEngineView::OnUrlChanged(const QUrl &url){
    if(!GetHistNode()) return;
    ChangeNodeUrl(url);
}

void QuickWebEngineView::OnViewChanged(){
    if(!GetHistNode()) return;
    TreeBank::AddToUpdateBox(GetThis().lock());
}

void QuickWebEngineView::OnScrollChanged(){
    if(!GetHistNode()) return;
    SaveScroll();
}

void QuickWebEngineView::CallWithScroll(PointFCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetScrollRatioPointJsCode(), [callBack](QVariant var){
            if(!var.isValid()) return callBack(QPointF(0.5f, 0.5f));
            QVariantList list = var.toList();
            callBack(QPointF(list[0].toFloat(), list[1].toFloat()));
        });
}

void QuickWebEngineView::SetScrollBarState(){
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

QPointF QuickWebEngineView::GetScroll(){
    if(!page()) return QPointF(0.5f, 0.5f);
    // this function does not return actual value on WebEngineView.
    return QPointF(0.5f, 0.5f);
}

void QuickWebEngineView::SetScroll(QPointF pos){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setScroll",
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

bool QuickWebEngineView::SaveScroll(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "saveScroll");
    return true;
}

bool QuickWebEngineView::RestoreScroll(){
    if(size().isEmpty()) return false;
    if(m_PreventScrollRestoration) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "restoreScroll");
    return true;
}

bool QuickWebEngineView::SaveZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "saveZoom");
    return true;
}

bool QuickWebEngineView::RestoreZoom(){
    if(size().isEmpty()) return false;
    QMetaObject::invokeMethod(m_QmlWebEngineView, "restoreZoom");
    return true;
}

void QuickWebEngineView::KeyEvent(QString key){
    TriggerKeyEvent(key);
}

bool QuickWebEngineView::SeekText(const QString &str, View::FindFlags opt){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "seekText",
                              Q_ARG(QVariant, QVariant::fromValue(str)),
                              Q_ARG(QVariant, QVariant::fromValue(static_cast<int>(opt))));
    return true;
}

void QuickWebEngineView::SetFocusToElement(QString xpath){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setFocusToElement",
                              Q_ARG(QVariant, QVariant::fromValue(xpath)));
}

void QuickWebEngineView::FireClickEvent(QString xpath, QPoint pos){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "fireClickEvent",
                              Q_ARG(QVariant, QVariant::fromValue(xpath)),
                              Q_ARG(QVariant, QVariant::fromValue(pos)));
}

void QuickWebEngineView::SetTextValue(QString xpath, QString text){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "setTextValue",
                              Q_ARG(QVariant, QVariant::fromValue(xpath)),
                              Q_ARG(QVariant, QVariant::fromValue(text)));
}

void QuickWebEngineView::AssignInspector(){
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

void QuickWebEngineView::UpdateIcon(const QUrl &iconUrl){
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

void QuickWebEngineView::HandleWindowClose(){
    page()->CloseLater();
}

void QuickWebEngineView::HandleJavascriptConsoleMessage(int level, const QString &msg){
    // 0: InfoMessageLevel
    if(level != 0) return;
#ifdef PASSWORD_MANAGER
    static QString reg;
    if(reg.isEmpty()) reg = QStringLiteral("submit%1,([^,]+)").arg(Application::EventKey());

    if(!page()->profile()->isOffTheRecord() &&
       !m_PreventAuthRegistration &&
       Application::ExactMatch(reg, msg)){

        Application::RegisterAuthData
            (page()->profile()->storageName() +
             QStringLiteral(":") + url().host(),
             msg.split(QStringLiteral(","))[1]);
        return;
    }
#endif //ifdef PASSWORD_MANAGER
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        KeyPressEvent(&QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers));
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        KeyReleaseEvent(&QKeyEvent(QEvent::KeyRelease, Application::JsKeyToQtKey(args[1].toInt()), modifiers));
    }
}

void QuickWebEngineView::HandleFeaturePermission(const QUrl &securityOrigin, int feature){
    QString featureString;
    switch(feature){
    case 0: //QQuickWebEngineView::MediaAudioCapture:
        featureString = QStringLiteral("MediaAudioCapture");      break;
    case 1: //QQuickWebEngineView::MediaVideoCapture:
        featureString = QStringLiteral("MediaVideoCapture");      break;
    case 2: //QQuickWebEngineView::MediaAudioVideoCapture:
        featureString = QStringLiteral("MediaAudioVideoCapture"); break;
    case 3: //QQuickWebEngineView::Geolocation:
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
        QMetaObject::invokeMethod(m_QmlWebEngineView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(true)));
    } else if(text == tr("No")){
        QMetaObject::invokeMethod(m_QmlWebEngineView, "grantFeaturePermission_",
                                  Q_ARG(QVariant, QVariant::fromValue(securityOrigin)),
                                  Q_ARG(QVariant, QVariant::fromValue(feature)),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    } else if(text == tr("Cancel")){
        // nothing to do.
    }
}

void QuickWebEngineView::HandleRenderProcessTermination(int status, int code){
    QString info = tr("A page is reloaded, because that's process is terminated.\n");
    switch(status){
    case 0: //QQuickWebEngineView::NormalTerminationStatus:
        info += tr("Normal termination. (code: %1)");   break;
    case 1: //QQuickWebEngineView::AbnormalTerminationStatus:
        info += tr("Abnormal termination. (code: %1)"); break;
    case 2: //QQuickWebEngineView::CrashedTerminationStatus:
        info += tr("Crashed termination. (code: %1)");  break;
    case 3: //QQuickWebEngineView::KilledTerminationStatus:
        info += tr("Killed termination. (code: %1)");   break;
    }
    ModelessDialog::Information(tr("Render process terminated."),
                                info.arg(code), base());
    QTimer::singleShot(0, m_QmlWebEngineView, SLOT(reload()));
}

void QuickWebEngineView::HandleFullScreen(bool on){
    if(TreeBank *tb = GetTreeBank()){
        tb->GetMainWindow()->SetFullScreen(on);
        SetDisplayObscured(on);
        if(!on) return;
        ModelessDialog *dialog = new ModelessDialog();
        // connect to 'Returned', because default value is true.
        connect(this, &QuickWebEngineView::destroyed, dialog, &ModelessDialog::Returned);
        connect(this, &QuickWebEngineView::fullScreenRequested, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(tr("This page becomes full screen mode."));
        dialog->SetCaption(tr("Press Esc to exit."));
        dialog->SetButtons(QStringList() << tr("OK") << tr("Cancel"));
        dialog->SetDefaultValue(true);
        dialog->SetCallBack([this](bool ok){ if(!ok) ExitFullScreen();});
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    }
}

void QuickWebEngineView::Copy(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "copy");
}

void QuickWebEngineView::Cut(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "cut");
}

void QuickWebEngineView::Paste(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "paste");
}

void QuickWebEngineView::Undo(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "undo");
}

void QuickWebEngineView::Redo(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "redo");
}

void QuickWebEngineView::SelectAll(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "selectAll");
}

void QuickWebEngineView::Unselect(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "unselect");
}

void QuickWebEngineView::Reload(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "reload");
}

void QuickWebEngineView::ReloadAndBypassCache(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "reloadAndBypassCache");
}

void QuickWebEngineView::Stop(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "stop");
}

void QuickWebEngineView::StopAndUnselect(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "stopAndUnselect");
}
void QuickWebEngineView::Print(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "print_");
}

void QuickWebEngineView::Save(){
#if QT_VERSION >= 0x050700
    QMetaObject::invokeMethod(m_QmlWebEngineView, "save");
#else
    if(!page()) return;
    QNetworkRequest req(url());
    req.setRawHeader("Referer", url().toEncoded());
    page()->Download(req);
#endif
}

void QuickWebEngineView::ZoomIn(){
    float zoom = PrepareForZoomIn();
    m_QmlWebEngineView->setProperty("zoomFactor", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebEngineView::ZoomOut(){
    float zoom = PrepareForZoomOut();
    m_QmlWebEngineView->setProperty("zoomFactor", static_cast<qreal>(zoom));
    emit statusBarMessage(tr("Zoom factor changed to %1 percent").arg(zoom*100.0));
}

void QuickWebEngineView::ExitFullScreen(){
    QMetaObject::invokeMethod(m_QmlWebEngineView, "fullScreenCancelled");
}

void QuickWebEngineView::InspectElement(){
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

void QuickWebEngineView::AddSearchEngine(QPoint pos){
    if(page()) page()->AddSearchEngine(pos);
}

void QuickWebEngineView::AddBookmarklet(QPoint pos){
    if(page()) page()->AddBookmarklet(pos);
}

void QuickWebEngineView::hideEvent(QHideEvent *ev){
    if(GetDisplayObscured()) ExitFullScreen();
    SaveViewState();
    QQuickWidget::hideEvent(ev);
}

void QuickWebEngineView::showEvent(QShowEvent *ev){
    m_PreventScrollRestoration = false;
    QQuickWidget::showEvent(ev);
    RestoreViewState();
}

void QuickWebEngineView::keyPressEvent(QKeyEvent *ev){
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
            (page()->profile()->storageName() +
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
    if(Application::HasAnyModifier(ev) ||
       // 'HasAnyModifier' ignores ShiftModifier.
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    //QQuickWidget::keyPressEvent(ev);

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
    }

    if(/*!ev->isAccepted() &&*/
       !Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
    }
}

void QuickWebEngineView::keyReleaseEvent(QKeyEvent *ev){
    Q_UNUSED(ev);

    if(!visible()) return;

    //QQuickWidget::keyReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");
}

void QuickWebEngineView::resizeEvent(QResizeEvent *ev){
    QQuickWidget::resizeEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebEngineView, "adjustContents");
}

void QuickWebEngineView::contextMenuEvent(QContextMenuEvent *ev){
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseMoveEvent(QMouseEvent *ev){
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

void QuickWebEngineView::mousePressEvent(QMouseEvent *ev){
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

void QuickWebEngineView::mouseReleaseEvent(QMouseEvent *ev){
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
            GestureFinished(ev->pos(), ev->button());
        } else if(!m_GestureStartedPos.isNull()){
            SharedWebElement elem = m_ClickedElement;
            GestureAborted(); // resets 'm_ClickedElement'.
            page()->DisplayContextMenu(m_TreeBank, elem, ev->pos(), ev->globalPos());
        }
        ev->setAccepted(true);
        return;
    }

    GestureAborted();
    QQuickWidget::mouseReleaseEvent(ev);
    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");
    ev->setAccepted(true);
}

void QuickWebEngineView::mouseDoubleClickEvent(QMouseEvent *ev){
    QQuickWidget::mouseDoubleClickEvent(ev);
    ev->setAccepted(false);
}

void QuickWebEngineView::dragEnterEvent(QDragEnterEvent *ev){
    m_DragStarted = true;
    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    QQuickWidget::dragEnterEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dragMoveEvent(QDragMoveEvent *ev){
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

void QuickWebEngineView::dropEvent(QDropEvent *ev){
    emit statusBarMessage(QString());
    QPoint pos = ev->pos();
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

    QQuickWidget::dropEvent(ev);
    ev->setAccepted(true);
}

void QuickWebEngineView::dragLeaveEvent(QDragLeaveEvent *ev){
    ev->setAccepted(false);
    m_DragStarted = false;
    QQuickWidget::dragLeaveEvent(ev);
}

void QuickWebEngineView::wheelEvent(QWheelEvent *ev){
    if(!visible()) return;

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

    } else {
        m_PreventScrollRestoration = true;
        QQuickWidget::wheelEvent(ev);
        ev->setAccepted(true);
    }
    QMetaObject::invokeMethod(m_QmlWebEngineView, "emitScrollChangedIfNeed");
}

void QuickWebEngineView::focusInEvent(QFocusEvent *ev){
    QQuickWidget::focusInEvent(ev);
    OnFocusIn();
}

void QuickWebEngineView::focusOutEvent(QFocusEvent *ev){
    QQuickWidget::focusOutEvent(ev);
    OnFocusOut();
}

bool QuickWebEngineView::focusNextPrevChild(bool next){
    if(!m_Switching && visible())
        return QQuickWidget::focusNextPrevChild(next);
    return false;
}

void QuickWebEngineView::CallWithGotBaseUrl(UrlCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithGotCurrentBaseUrl(UrlCallBack callBack){
    // this implementation is same as baseurl...
    CallWithEvaluatedJavaScriptResult
        (GetBaseUrlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithFoundElements(Page::FindElementsOption option,
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

void QuickWebEngineView::CallWithHitElement(const QPoint &pos, WebElementCallBack callBack){
    if(pos.isNull()) return callBack(SharedWebElement());
    CallWithEvaluatedJavaScriptResult
        (HitElementJsCode(pos / m_HistNode->GetZoom()), [this, callBack](QVariant var){
            if(!var.isValid()) return callBack(SharedWebElement());
            std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
            *e = JsWebElement(this, var);
            callBack(e);
        });
}

void QuickWebEngineView::CallWithHitLinkUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitLinkUrlJsCode(pos / m_HistNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithHitImageUrl(const QPoint &pos, UrlCallBack callBack){
    if(pos.isNull()) return callBack(QUrl());
    CallWithEvaluatedJavaScriptResult
        (HitImageUrlJsCode(pos / m_HistNode->GetZoom()), [callBack](QVariant var){
            callBack(var.isValid() ? var.toUrl() : QUrl());
        });
}

void QuickWebEngineView::CallWithSelectedText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithSelectedHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (SelectedHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithWholeText(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeTextJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithWholeHtml(StringCallBack callBack){
    CallWithEvaluatedJavaScriptResult
        (WholeHtmlJsCode(), [callBack](QVariant var){
            callBack(var.isValid() ? var.toString() : QString());
        });
}

void QuickWebEngineView::CallWithSelectionRegion(RegionCallBack callBack){
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

void QuickWebEngineView::CallWithEvaluatedJavaScriptResult(const QString &code,
                                                           VariantCallBack callBack){
    int requestId = m_RequestId++;
    std::shared_ptr<QMetaObject::Connection> connection =
        std::make_shared<QMetaObject::Connection>();
    *connection =
        connect(this, &QuickWebEngineView::CallBackResult,
                [this, requestId, callBack, connection](int id, QVariant result){
                    if(requestId != id) return;
                    QObject::disconnect(*connection);
                    callBack(result);
                });

    QMetaObject::invokeMethod(m_QmlWebEngineView, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant::fromValue(requestId)),
                              Q_ARG(QVariant, QVariant::fromValue(code)));
}

