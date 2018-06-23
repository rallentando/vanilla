#include "switch.hpp"
#include "const.hpp"

#ifdef WEBENGINEVIEW

#include "webenginepage.hpp"
#include "page.hpp"

#ifdef USE_WEBCHANNEL
#  include <QWebChannel>
#endif
#ifdef PASSWORD_MANAGER
# include <QWebEngineProfile>
#endif

#include <QTimer>
#include <QNetworkReply>
#include <QAction>
#include <QPrinter>
#include <QPrintDialog>
#include <QClipboard>
#include <QTextCodec>
#include <QAuthenticator>
#include <QPushButton>
#include <QPair>
#include <QSet>
#include <QMenu>
#include <QCursor>
#include <QUrlQuery>

#include <functional>
#include <memory>

#include "view.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "gadgets.hpp"
#include "networkcontroller.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

WebEnginePage::WebEnginePage(NetworkAccessManager *nam, QObject *parent)
    : QWebEnginePage(nam->GetProfile(), parent)
{
    setNetworkAccessManager(nam);

    connect(this,   SIGNAL(ViewChanged()),
            parent, SIGNAL(ViewChanged()));
    connect(this,   SIGNAL(ScrollChanged(QPointF)),
            parent, SIGNAL(ScrollChanged(QPointF)));

    // these are self defined signals.
    connect(this,   SIGNAL(urlChanged(const QUrl&)),
            parent, SIGNAL(urlChanged(const QUrl&)));
    connect(this,   SIGNAL(titleChanged(const QString&)),
            parent, SIGNAL(titleChanged(const QString&)));
    connect(this,   SIGNAL(statusBarMessage2(const QString&, const QString&)),
            parent, SIGNAL(statusBarMessage2(const QString&, const QString&)));
    connect(this,   SIGNAL(iconChanged(const QIcon&)),
            parent, SIGNAL(iconChanged(const QIcon&)));

    // needless, because it's connected yet.
    //connect(this,   SIGNAL(loadStarted()),
    //        parent, SIGNAL(loadStarted()));
    //connect(this,   SIGNAL(loadProgress(int)),
    //        parent, SIGNAL(loadProgress(int)));
    //connect(this,   SIGNAL(loadFinished(bool)),
    //        parent, SIGNAL(loadFinished(bool)));
    //connect(this,   SIGNAL(statusBarMessage(const QString&)),
    //        parent, SIGNAL(statusBarMessage(const QString&)));
    //connect(this,   SIGNAL(linkClicked(QUrl)),
    //        parent, SIGNAL(linkClicked(QUrl)));
    //connect(this,   SIGNAL(selectionChanged()),
    //        parent, SIGNAL(selectionChanged()));

    connect(this, SIGNAL(linkHovered(const QString&)),
            this, SLOT(OnLinkHovered(const QString&)));
    connect(this, SIGNAL(featurePermissionRequested(const QUrl&, QWebEnginePage::Feature)),
            this, SLOT(HandleFeaturePermission(const QUrl&, QWebEnginePage::Feature)));
    connect(this, SIGNAL(authenticationRequired(const QUrl&, QAuthenticator*)),
            this, SLOT(HandleAuthentication(const QUrl&, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QUrl&, QAuthenticator*, const QString&)),
            this, SLOT(HandleProxyAuthentication(const QUrl&, QAuthenticator*, const QString&)));
    connect(this, SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
            this, SLOT(HandleFullScreen(QWebEngineFullScreenRequest)));
    connect(this, SIGNAL(renderProcessTerminated(RenderProcessTerminationStatus, int)),
            this, SLOT(HandleProcessTermination(RenderProcessTerminationStatus, int)));
    connect(this, SIGNAL(contentsSizeChanged(const QSizeF&)),
            this, SLOT(HandleContentsSizeChange(const QSizeF&)));
    connect(this, SIGNAL(scrollPositionChanged(const QPointF&)),
            this, SLOT(HandleScrollPositionChange(const QPointF&)));

    // instead of this.
    //m_View = (View *)parent;
    if(parent){
        if(WebEngineView *w = qobject_cast<WebEngineView*>(parent))
            m_View = w;
        else if(QuickWebEngineView *w = qobject_cast<QuickWebEngineView*>(parent))
            m_View = w;
        else m_View = 0;
    } else m_View = 0;

    m_Page = new Page(this);
    m_Page->SetView(m_View);

    connect(this, SIGNAL(windowCloseRequested()), m_Page, SLOT(Close()));

#ifdef USE_WEBCHANNEL
    AddJsObject();
#endif
}

WebEnginePage::~WebEnginePage(){
#ifdef USE_WEBCHANNEL
    if(webChannel()) delete webChannel();
#endif
    setNetworkAccessManager(0);
}

View* WebEnginePage::GetView(){
    return m_View;
}

WebEnginePage* WebEnginePage::createWindow(WebWindowType type){

    static const QUrl blank = BLANK_URL;

    View *view = type == QWebEnginePage::WebBrowserBackgroundTab
        ? OpenInNewBackground(blank)
        : OpenInNew(blank);

    if(WebEngineView *w = qobject_cast<WebEngineView*>(view->base())){
        return w->page();
    }
    return this;
}

void WebEnginePage::triggerAction(WebAction action, bool checked){
    QWebEnginePage::triggerAction(action, checked);
}

bool WebEnginePage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame){
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

QStringList WebEnginePage::chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                                       const QStringList &acceptedMimeTypes){
    Q_UNUSED(acceptedMimeTypes);

    QStringList suggestedFiles = oldFiles;

    if(suggestedFiles.isEmpty() || suggestedFiles.first().isEmpty()){
        suggestedFiles = QStringList() << Application::GetUploadDirectory();
    }

    QStringList files;

    if(mode == QWebEnginePage::FileSelectOpenMultiple){

        files = ModalDialog::GetOpenFileNames(QString(), suggestedFiles.first());

    } else if(mode == QWebEnginePage::FileSelectOpen){

        files << ModalDialog::GetOpenFileName_(QString(), suggestedFiles.first());
    }

    if(!files.isEmpty() && !files.first().isEmpty()){
        QString file = files.first();
        if(!file.contains(QStringLiteral("/"))) return files;
        QStringList path = file.split(QStringLiteral("/"));
        path.removeLast();
        Application::SetUploadDirectory(path.join(QStringLiteral("/")));
        foreach(QString file, files){
            Application::AppendChosenFile(file);
        }
    }
    return files;
}

bool WebEnginePage::certificateError(const QWebEngineCertificateError& error){
    QString errorTypeString;
    switch(error.error()){
    case QWebEngineCertificateError::SslPinnedKeyNotInCertificateChain:
        errorTypeString = QStringLiteral("SslPinnedKeyNotInCertificateChain");  break;
    case QWebEngineCertificateError::CertificateCommonNameInvalid:
        errorTypeString = QStringLiteral("CertificateCommonNameInvalid");       break;
    case QWebEngineCertificateError::CertificateDateInvalid:
        errorTypeString = QStringLiteral("CertificateDateInvalid");             break;
    case QWebEngineCertificateError::CertificateAuthorityInvalid:
        errorTypeString = QStringLiteral("CertificateAuthorityInvalid");        break;
    case QWebEngineCertificateError::CertificateContainsErrors:
        errorTypeString = QStringLiteral("CertificateContainsErrors");          break;
    case QWebEngineCertificateError::CertificateNoRevocationMechanism:
        errorTypeString = QStringLiteral("CertificateNoRevocationMechanism");   break;
    case QWebEngineCertificateError::CertificateUnableToCheckRevocation:
        errorTypeString = QStringLiteral("CertificateUnableToCheckRevocation"); break;
    case QWebEngineCertificateError::CertificateRevoked:
        errorTypeString = QStringLiteral("CertificateRevoked");                 break;
    case QWebEngineCertificateError::CertificateInvalid:
        errorTypeString = QStringLiteral("CertificateInvalid");                 break;
    case QWebEngineCertificateError::CertificateWeakSignatureAlgorithm:
        errorTypeString = QStringLiteral("CertificateWeakSignatureAlgorithm");  break;
    case QWebEngineCertificateError::CertificateNonUniqueName:
        errorTypeString = QStringLiteral("CertificateNonUniqueName");           break;
    case QWebEngineCertificateError::CertificateWeakKey:
        errorTypeString = QStringLiteral("CertificateWeakKey");                 break;
    case QWebEngineCertificateError::CertificateNameConstraintViolation:
        errorTypeString = QStringLiteral("CertificateNameConstraintViolation"); break;
    default: return false;
    }

    Application::AskSslErrorPolicyIfNeed();

    // false : block, true : allow.

    switch(Application::GetSslErrorPolicy()){
    case Application::BlockAccess:
        return false;
    case Application::IgnoreSslErrors:
        return true;
    case Application::AskForEachAccess:{

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Certificate error."));
        dialog->SetCaption(tr("Certificate error."));
        dialog->SetInformativeText(tr("Ignore this error?"));
        dialog->SetDetailedText
            (tr("Url: ") + error.url().toString() + QStringLiteral("\n") +
             tr("Type: ") + errorTypeString + QStringLiteral("\n") +
             error.errorDescription());
        dialog->SetButtons(QStringList() << tr("Allow") << tr("Block"));
        if(dialog->Execute() && dialog->ClickedButton() == tr("Allow"))
            return true;
        break;
    }
    case Application::AskForEachHost:
    case Application::AskForEachCertificate:{

        QString host = error.url().host();

        if(Application::GetBlockedHosts().contains(host)){
            return false;
        } else if(Application::GetAllowedHosts().contains(host)){
            return true;
        } else {
            ModalDialog *dialog = new ModalDialog();
            dialog->SetTitle(tr("Certificate error on host:%1").arg(host));
            dialog->SetCaption(tr("Certificate error on host:%1").arg(host));
            dialog->SetInformativeText(tr("Allow or Block this host?"));
            dialog->SetDetailedText
                (tr("Url: ") + error.url().toString() + QStringLiteral("\n") +
                 tr("Type: ") + errorTypeString + QStringLiteral("\n") +
                 error.errorDescription());

            dialog->SetButtons(QStringList() << tr("Allow") << tr("Block") << tr("Cancel"));
            dialog->Execute();
            QString text = dialog->ClickedButton();
            if(text == tr("Allow")){
                Application::AppendToAllowedHosts(host);
                return true;
            } else if(text == tr("Block")){
                Application::AppendToBlockedHosts(host);
                return false;
            }
            break;
        }
    }
    default: break;
    }
    return QWebEnginePage::certificateError(error);
}

void WebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &msg,
                                             int lineNumber, const QString &sourceId){
#ifdef PASSWORD_MANAGER
    static QString reg;
    if(reg.isEmpty()) reg = QStringLiteral("submit%1,([^,]+)").arg(Application::EventKey());

    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base())){
        if(level == QWebEnginePage::InfoMessageLevel &&
           !profile()->isOffTheRecord() &&
           !w->PreventAuthRegistration() &&
           Application::ExactMatch(reg, msg)){

            Application::RegisterAuthData
                (profile()->storageName() +
                 QStringLiteral(":") + url().host(),
                 msg.split(QStringLiteral(","))[1]);
            return;
        }
    }
#endif
#if QT_VERSION >= 0x050900
    if(Application::ExactMatch(QStringLiteral("keyPressEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyPress, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        m_View->KeyPressEvent(&ke);
    } else if(Application::ExactMatch(QStringLiteral("keyReleaseEvent%1,([0-9]+),(true|false),(true|false),(true|false),(true|false)").arg(Application::EventKey()), msg)){
        QStringList args = msg.split(QStringLiteral(","));
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(args[2] == QStringLiteral("true")) modifiers |= Qt::ShiftModifier;
        if(args[3] == QStringLiteral("true")) modifiers |= Qt::ControlModifier;
        if(args[4] == QStringLiteral("true")) modifiers |= Qt::AltModifier;
        if(args[5] == QStringLiteral("true")) modifiers |= Qt::MetaModifier;
        QKeyEvent ke = QKeyEvent(QEvent::KeyRelease, Application::JsKeyToQtKey(args[1].toInt()), modifiers);
        m_View->KeyReleaseEvent(&ke);
    }
#endif
    QWebEnginePage::javaScriptConsoleMessage(level, msg, lineNumber, sourceId);
}

QNetworkAccessManager *WebEnginePage::networkAccessManager() const {
    return m_NetworkAccessManager;
}

void WebEnginePage::setNetworkAccessManager(QNetworkAccessManager *nam){
    m_NetworkAccessManager = nam;
}

QString WebEnginePage::userAgentForUrl(const QUrl &url) const {
    Q_UNUSED(url);
    return static_cast<NetworkAccessManager*>(networkAccessManager())->GetUserAgent();
}

void WebEnginePage::DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                                       QPoint localPos, QPoint globalPos, Page::MediaType type){
    m_Page->DisplayContextMenu(parent, elem, localPos, globalPos, type);
}

void WebEnginePage::TriggerAction(Page::CustomAction action, QVariant data){
    Action(action, data)->trigger();
}

QAction *WebEnginePage::Action(Page::CustomAction a, QVariant data){
    return m_Page->Action(a, data);
}

void WebEnginePage::OnLinkHovered(const QString &url){
    emit linkHovered(url, QString(), QString());
}

void WebEnginePage::HandleFeaturePermission(const QUrl &securityOrigin,
                                            QWebEnginePage::Feature feature){
    QString featureString;
    switch(feature){
    case QWebEnginePage::Notifications:
        featureString = QStringLiteral("Notifications");          break;
    case QWebEnginePage::Geolocation:
        featureString = QStringLiteral("Geolocation");            break;
    case QWebEnginePage::MediaAudioCapture:
        featureString = QStringLiteral("MediaAudioCapture");      break;
    case QWebEnginePage::MediaVideoCapture:
        featureString = QStringLiteral("MediaVideoCapture");      break;
    case QWebEnginePage::MediaAudioVideoCapture:
        featureString = QStringLiteral("MediaAudioVideoCapture"); break;
    case QWebEnginePage::MouseLock:
        featureString = QStringLiteral("MouseLock");              break;
#if QT_VERSION >= 0x050A00
    case QWebEnginePage::DesktopVideoCapture:
        featureString = QStringLiteral("DesktopVideoCapture");    break;
    case QWebEnginePage::DesktopAudioVideoCapture:
        featureString = QStringLiteral("DesktopAudioVideoCapture"); break;
#endif
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
        setFeaturePermission(securityOrigin, feature,
                             QWebEnginePage::PermissionGrantedByUser);
    } else if(text == tr("No")){
        setFeaturePermission(securityOrigin, feature,
                             QWebEnginePage::PermissionDeniedByUser);
    } else if(text == tr("Cancel")){
        emit featurePermissionRequestCanceled(securityOrigin, feature);
    }
}

void WebEnginePage::HandleAuthentication(const QUrl &requestUrl,
                                         QAuthenticator *authenticator){
    Q_UNUSED(requestUrl);

    ModalDialog::Authentication(authenticator);
}

void WebEnginePage::HandleProxyAuthentication(const QUrl &requestUrl,
                                              QAuthenticator *authenticator,
                                              const QString &proxyHost){
    Q_UNUSED(requestUrl);
    Q_UNUSED(proxyHost);

    ModalDialog::Authentication(authenticator);
}

void WebEnginePage::HandleFullScreen(QWebEngineFullScreenRequest request){
    if(TreeBank *tb = m_View->GetTreeBank()){
        bool on = request.toggleOn();
        tb->GetMainWindow()->SetFullScreen(on);
        m_View->SetDisplayObscured(on);
        request.accept();
        if(!on) return;
        ModelessDialog *dialog = new ModelessDialog();
        // connect to 'Returned', because default value is true.
        connect(this, &WebEnginePage::destroyed, dialog, &ModelessDialog::Returned);
        connect(this, &WebEnginePage::fullScreenRequested, dialog, &ModelessDialog::Returned);
        dialog->SetTitle(tr("This page becomes full screen mode."));
        dialog->SetCaption(tr("Press Esc to exit."));
        dialog->SetButtons(QStringList() << tr("OK") << tr("Cancel"));
        dialog->SetDefaultValue(true);
        dialog->SetCallBack([this](bool ok){ if(!ok) triggerAction(ExitFullScreen);});
        QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
    } else {
        request.reject();
    }
}

void WebEnginePage::HandleProcessTermination(RenderProcessTerminationStatus status, int code){
    QString info = tr("A page is reloaded, because that's process is terminated.\n");
    switch(status){
    case QWebEnginePage::NormalTerminationStatus:
        info += tr("Normal termination. (code: %1)");   break;
    case QWebEnginePage::AbnormalTerminationStatus:
        info += tr("Abnormal termination. (code: %1)"); break;
    case QWebEnginePage::CrashedTerminationStatus:
        info += tr("Crashed termination. (code: %1)");  break;
    case QWebEnginePage::KilledTerminationStatus:
        info += tr("Killed termination. (code: %1)");   break;
    }
    ModelessDialog::Information(tr("Render process terminated."),
                                info.arg(code), m_View->base());
    QTimer::singleShot(0, m_Page, SLOT(Reload()));
}

void WebEnginePage::HandleContentsSizeChange(const QSizeF &size){
    Q_UNUSED(size);
    m_View->RestoreScroll();
}

void WebEnginePage::HandleScrollPositionChange(const QPointF &pos){
    Q_UNUSED(pos);
    m_View->EmitScrollChanged();
}

void WebEnginePage::HandleUnsupportedContent(QNetworkReply *reply){
    Q_UNUSED(reply);
}

void WebEnginePage::AddJsObject(){
#ifdef USE_WEBCHANNEL

    QWebChannel *channel = webChannel();

    setWebChannel(new QWebChannel(this));
    if(channel) delete channel;

    if(m_View && m_View->GetJsObject()){
        webChannel()->registerObject(QStringLiteral("_view"), m_View->GetJsObject());
        m_View->OnSetJsObject(m_View->GetJsObject());
    }
    if(m_View && m_View->GetTreeBank() && m_View->GetTreeBank()->GetJsObject()){
        webChannel()->registerObject(QStringLiteral("_vanilla"), m_View->GetTreeBank()->GetJsObject());
        m_View->OnSetJsObject(m_View->GetTreeBank()->GetJsObject());
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////

void WebEnginePage::InspectElement(){
    // my inspector doesn't catch element.
    //QWebInspector *inspector = new QWebInspector;
    //inspector->setPage(this);
    //inspector->show();
    QWebEnginePage::triggerAction(QWebEnginePage::InspectElement);
}

void WebEnginePage::AddSearchEngine(QPoint pos){
    m_View->CallWithGotCurrentBaseUrl([this, pos](QUrl base){

    m_View->CallWithEvaluatedJavaScriptResult(QStringLiteral(
"(function(){\n"
"    var x = %1;\n"
"    var y = %2;\n"
"    var elem = document.elementFromPoint(x, y);\n"
"    while(elem && (elem.tagName == \"FRAME\" || elem.tagName == \"IFRAME\")){\n"
"        try{\n"
"            var frameDocument = elem.contentDocument;\n"
"            var rect = elem.getBoundingClientRect();\n"
"            x -= rect.left;\n"
"            y -= rect.top;\n"
"            elem = frameDocument.elementFromPoint(x, y);\n"
"        }\n"
"        catch(e){ break;}\n"
"    }\n"
"    if(!elem) return {};\n"
"    var name = elem.name;\n"
"    var form = elem;\n"
"    while(form && form.tagName != \"FORM\"){\n"
"        form = form.parentNode;\n"
"    }\n"
"    if(!form) return {};\n"
"    var encode = form.getAttribute(\"accept-charset\") || form.ownerDocument.charset || \"UTF-8\";\n"
"    var method = form.method || \"get\";\n"
"    if(method.toLowerCase() != \"get\") return {};\n"
"    var result = {};\n"
"    var queries = {};\n"
"    var engines = {};\n"
"    var inputs = form.getElementsByTagName(\"input\");\n"
"    var buttons = form.getElementsByTagName(\"button\");\n"
"    var selects = form.getElementsByTagName(\"select\");\n"
"    for(var i = 0; i < inputs.length; i++){\n"
"        var field = inputs[i];\n"
"        var type = (field.type || \"text\").toLowerCase();\n"
"        var name = field.name;\n"
"        var val = field.value;\n"
"        if(type == \"submit\"){\n"
"            engines[name] = val; continue;\n"
"        } else if(type == \"text\" || type == \"search\"){\n"
"            if(field == elem) val = \"{query}\";\n"
"        } else if(type == \"checkbox\" || type == \"radio\"){\n"
"            if(!field.checked) continue;\n"
"        } else if(type != \"hidden\") continue;\n"
"        queries[name] = val;\n"
"    }\n"
"    for(var i = 0; i < buttons.length; i++){\n"
"        var button = buttons[i];\n"
"        engines[button.name] = button.getAttribute(\"aria-label\");\n"
"    }\n"
"    for(var i = 0; i < selects.length; i++){\n"
"        var select = selects[i];\n"
"        var index = select.selectedIndex;\n"
"        if(index != -1){\n"
"            var options = select.getElementsByTagName(\"option\");\n"
"            queries[select.name] = options[index].textContent;\n"
"        }\n"
"    }\n"
"    var labels = form.querySelectorAll(\"label[for=\\\"\"+name+\"\\\"]\");\n"
"    var tag = labels.length ? labels[0].innerText : \"\";\n"
"    result[0] = encode;\n"
"    result[1] = method;\n"
"    result[2] = form.action;\n"
"    result[3] = queries;\n"
"    result[4] = engines;\n"
"    result[5] = tag;\n"
"    return result;\n"
"})();").arg(pos.x()).arg(pos.y()),
[base](QVariant var){

    if(!var.isValid()) return;
    QVariantList list = var.toMap().values();
    if(list.isEmpty()) return;

    QString encode = list[0].toString();
    QString method = list[1].toString();
    QUrl result = Page::StringToUrl(list[2].toString(), base);
    QUrlQuery queries = QUrlQuery(result);
    QMap<QString, QString> engines;
    foreach(QString key, list[3].toMap().keys()){
        QString k = QString::fromLatin1(QUrl::toPercentEncoding(key));
        QString v = QString::fromLatin1(QUrl::toPercentEncoding(list[3].toMap()[key].toString()));
        queries.addQueryItem(k, v);
    }
    foreach(QString key, list[4].toMap().keys()){
        QString k = QString::fromLatin1(QUrl::toPercentEncoding(key));
        QString v = QString::fromLatin1(QUrl::toPercentEncoding(list[4].toMap()[key].toString()));
        engines[k] = v;
    }
    QString tag = list[5].toString();

    bool ok = true;
    if(engines.count() > 1){

        QString engine = ModalDialog::GetItem
            (tr("Search button"),
             tr("Select search button."),
             engines.keys(), false, &ok);

        if(!ok) return;
        if(!engines[engine].isEmpty()){
            queries.addQueryItem(engine, engines[engine]);
        }
    }

    tag = ModalDialog::GetText
        (tr("Search tag"),
         tr("Input search tag.(It will be used as command)"),
         tag, &ok);

    if(!ok || tag.isEmpty()) return;

    QStringList format;

    result.setQuery(queries);
    format
        << result.toString()
             .replace(QStringLiteral("{query}"),     QStringLiteral("%1"))
             .replace(QStringLiteral("%7Bquery%7D"), QStringLiteral("%1"))
        << encode << QStringLiteral("false");

    Page::RegisterSearchEngine(tag, format);
    });});
}

void WebEnginePage::AddBookmarklet(QPoint pos){
    m_View->CallWithHitElement(pos, [this](SharedWebElement elem){

    QUrl link  = elem ? elem->LinkUrl()  : QUrl();
    QUrl image = elem ? elem->ImageUrl() : QUrl();
    QString text = selectedText();

    QStringList places;
    if(!link.isEmpty())  places << tr("Link at Mouse Cursor");
    if(!image.isEmpty()) places << tr("Image at Mouse Cursor");
    if(!text.isEmpty())  places << tr("Selected Text");

    places << tr("Manual Input");

    bool ok;

    QString place = ModalDialog::GetItem
        (tr("Input type"),
         tr("Select input type of bookmarklet."),
         places, false, &ok);
    if(!ok) return;

    QString bookmark;

    if(place == tr("Link at Mouse Cursor")){
        bookmark = link.toString(QUrl::None);

        //int count = ModalDialog::GetInt
        //    (tr("Multiple Decoding."),
        //     tr("How many times to Decode?"),
        //     2, 1, 10, 1, &ok);
        int count = 2;

        // too dirty...
        for(int i = count; i > 0; i--){
            bookmark.replace(QStringLiteral("%%%%%%%"), QStringLiteral("%25%25%25%25%25%25%25"));
            bookmark.replace(QStringLiteral("%%%%%"), QStringLiteral("%25%25%25%25%25"));
            bookmark.replace(QStringLiteral("%%%"), QStringLiteral("%25%25%25"));
            bookmark.replace(QStringLiteral("%%"), QStringLiteral("%25%25"));
            bookmark.replace(QRegularExpression(QStringLiteral("%([^0-9A-F][0-9A-F]|[0-9A-F][^0-9A-F]|[^0-9A-F][^0-9A-F])")),
                             QStringLiteral("%25\\1"));
            bookmark = QUrl::fromPercentEncoding(bookmark.toLatin1());
        }
    }

    if(place == tr("Image at Mouse Cursor")){
        bookmark = image.toDisplayString(QUrl::FullyDecoded);
    }

    if(place == tr("Selected Text")){
        bookmark = text;
    }

    if(place == tr("Manual Input")){
        bookmark = ModalDialog::GetText
            (tr("Bookmarklet body"),
             tr("Input bookmarklet body."),
             QString(), &ok);
    }

    if(bookmark.isEmpty()) return;

    QString tag = ModalDialog::GetText
        (tr("Bookmarklet Name"),
         tr("Input bookmarklet name.(It will be used as command)"),
         QString(), &ok);

    if(!ok || tag.isEmpty()) return;

    QStringList result;
    result << bookmark;

    Page::RegisterBookmarklet(tag, result);
    });
}

void WebEnginePage::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    DownloadItem *item =
        NetworkController::Download(static_cast<NetworkAccessManager*>(networkAccessManager()),
                                    req, NetworkController::ToVariable);
    connect(item, &DownloadItem::DownloadResult, this, &WebEnginePage::SuggestResult);
}

#endif //ifdef WEBENGINEVIEW
