#include "switch.hpp"
#include "const.hpp"

#ifdef WEBKITVIEW

#include "webkitpage.hpp"
#include "page.hpp"

#include <QWebHistory>
#include <QWebElement>

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
#include <QWebInspector>

#include <functional>
#include <memory>

#include "view.hpp"
#include "webkitview.hpp"
#include "graphicswebkitview.hpp"
#include "quickwebkitview.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "networkcontroller.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

#ifdef QT_DEBUG
#  include <QDebug>
#endif

WebKitPage::WebKitPage(NetworkAccessManager *nam, QObject *parent)
    : QWebPage(parent)
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

    connect(this, SIGNAL(downloadRequested(QNetworkRequest)),
            this, SLOT(Download(QNetworkRequest)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(HandleUnsupportedContent(QNetworkReply*)));
    connect(mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
            this, SLOT(AddJsObject()));

    // instead of this.
    //m_View = (View *)parent;
    if(parent){
        if(WebKitView *w = qobject_cast<WebKitView*>(parent))
            m_View = w;
        else if(GraphicsWebKitView *w = qobject_cast<GraphicsWebKitView*>(parent))
            m_View = w;
        else if(QuickWebKitView *w = qobject_cast<QuickWebKitView*>(parent))
            m_View = w;
        else m_View = 0;
    } else m_View = 0;

    m_Page = new Page(this);
    m_Page->SetView(m_View);

    connect(this, SIGNAL(windowCloseRequested()), m_Page, SLOT(Close()));

    setForwardUnsupportedContent(true);
}

WebKitPage::~WebKitPage(){
    setNetworkAccessManager(0);
}

View* WebKitPage::GetView(){
    return m_View;
}

WebKitPage* WebKitPage::createWindow(WebWindowType type){
    Q_UNUSED(type);
    View *view = m_Page->OpenInNew(QUrl(QStringLiteral("about:blank")));

    if(WebKitView *w = qobject_cast<WebKitView*>(view->base()))
        return w->page();
    else if(GraphicsWebKitView *w = qobject_cast<GraphicsWebKitView*>(view->base()))
        return w->page();
    return 0;
}

void WebKitPage::triggerAction(WebAction action, bool checked){
    QWebPage::triggerAction(action, checked);
}

bool WebKitPage::acceptNavigationRequest(QWebFrame *frame,
                                          const QNetworkRequest &req,
                                          NavigationType type){
    // is there any good way?
    return QWebPage::acceptNavigationRequest(frame, req, type);

    // QWebPage::NavigationTypeBackOrForward
    // QWebPage::NavigationTypeReload
    // QWebPage::NavigationTypeFormResubmitted
    // QWebPage::NavigationTypeFormSubmitted
    // QWebPage::NavigationTypeOther
    // :
    // default navigation

    // QWebPage::NavigationTypeLinkClicked
    // :
    // call 'OpenInNewHistNode' or 'OpenInNewViewNode' if 'EnableLoadHackLocal' is true;

    if(type == QWebPage::NavigationTypeBackOrForward ||
       type == QWebPage::NavigationTypeReload ||
       // cannot make a distinction of search engine or authentication.
       type == QWebPage::NavigationTypeFormSubmitted ||
       type == QWebPage::NavigationTypeFormResubmitted ||
       // cannot make a distinction of javascript or redirect.
       type == QWebPage::NavigationTypeOther ||
       !frame || frame->toHtml() == EMPTY_FRAME_HTML ||
       mainFrame()->toHtml() == EMPTY_FRAME_HTML){

        return QWebPage::acceptNavigationRequest(frame, req, type);
    }

    // this is not useful yet...
    // if request is search engine or javascript, want to make new node,
    // but cannot identify that.
    if(type == QWebPage::NavigationTypeLinkClicked){

        TreeBank *tb = m_View->GetTreeBank();
        ViewNode *vn = m_View->GetViewNode();
        HistNode *hn = m_View->GetHistNode();

        if(Page::ShiftMod()){

            return !tb->OpenInNewViewNode(req, Page::Activate(), vn);

        } else if(m_View->EnableLoadHackLocal() && Page::Activate()){

            return !tb->OpenInNewHistNode(req, true, hn);
        }
    }
    return QWebPage::acceptNavigationRequest(frame, req, type);
}

QObject* WebKitPage::createPlugin(const QString &id, const QUrl &url,
                                   const QStringList &names, const QStringList &values){
    return QWebPage::createPlugin(id, url, names, values);
}

QString WebKitPage::userAgentForUrl(const QUrl &url) const{
    QString ua =
        static_cast<NetworkAccessManager*>(networkAccessManager())->GetUserAgent();
    if(ua.isEmpty()) return QWebPage::userAgentForUrl(url);
    return ua;
}

QString WebKitPage::chooseFile(QWebFrame *parentframe, const QString &suggested){
    Q_UNUSED(parentframe);

    QString file = ModalDialog::GetOpenFileName_
        (QString::null,
         suggested.isEmpty() ?
         Application::GetUploadDirectory() :
         suggested);

    if(file.isEmpty() || !file.contains(QStringLiteral("/"))) return file;
    QStringList path = file.split(QStringLiteral("/"));
    path.removeLast();
    Application::SetUploadDirectory(path.join(QStringLiteral("/")));
    Application::AppendChosenFile(file);
    return file;
}

bool WebKitPage::extension(Extension extension,
                            const ExtensionOption *option,
                            ExtensionReturn *output){

    if(extension == ChooseMultipleFilesExtension){

        QStringList suggestedFiles =
            static_cast<const ChooseMultipleFilesExtensionOption*>(option)->suggestedFileNames;

        if(suggestedFiles.isEmpty() || suggestedFiles.first().isEmpty()){
            suggestedFiles = QStringList() << Application::GetUploadDirectory();
        }

        QStringList files = ModalDialog::GetOpenFileNames(QString::null, suggestedFiles.first());

        static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = files;

        if(!files.isEmpty() && !files.first().isEmpty()){
            QString file = files.first();
            if(!file.contains(QStringLiteral("/"))) return true;
            QStringList path = file.split(QStringLiteral("/"));
            path.removeLast();
            Application::SetUploadDirectory(path.join(QStringLiteral("/")));
            foreach(QString file, files){
                Application::AppendChosenFile(file);
            }
        }
        return true;
    }
    if(extension == ErrorPageExtension){
        return QWebPage::extension(extension, option, output);
    }
    return QWebPage::extension(extension, option, output);
}

bool WebKitPage::supportsExtension(Extension extension) const{
    if(extension == ChooseMultipleFilesExtension)
        return true;
    if(extension == ErrorPageExtension)
        return false;
    return QWebPage::supportsExtension(extension);
}

void WebKitPage::DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                                    QPoint localPos, QPoint globalPos, Page::MediaType type){
    m_Page->DisplayContextMenu(parent, elem, localPos, globalPos, type);
}

void WebKitPage::TriggerAction(Page::CustomAction action, QVariant data){
    Action(action, data)->trigger();
}

QAction *WebKitPage::Action(Page::CustomAction a, QVariant data){
    QAction *result = 0;
    switch(a){
    // set text manually.
    case Page::_Copy:          result = action(Copy);                    result->setText(tr("Copy"));          return result;
    case Page::_Cut:           result = action(Cut);                     result->setText(tr("Cut"));           return result;
    case Page::_Paste:         result = action(Paste);                   result->setText(tr("Paste"));         return result;
    case Page::_Undo:          result = action(Undo);                    result->setText(tr("Undo"));          return result;
    case Page::_Redo:          result = action(Redo);                    result->setText(tr("Redo"));          return result;
    case Page::_SelectAll:     result = action(SelectAll);               result->setText(tr("SelectAll"));     return result;
    case Page::_Reload:        result = action(Reload);                  result->setText(tr("Reload"));        return result;
    case Page::_Stop:          result = action(Stop);                    result->setText(tr("Stop"));          return result;

  //case Page::_LoadLink:      result = action(OpenLink);                result->setText(tr("LoadLink"));      return result;
    case Page::_OpenLink:      result = action(OpenLinkInNewWindow);     result->setText(tr("OpenLink"));      return result;
  //case Page::_DownloadLink:  result = action(DownloadLinkToDisk);      result->setText(tr("DownloadLink"));  return result;
    case Page::_CopyLinkUrl:   result = action(CopyLinkToClipboard);     result->setText(tr("CopyLinkUrl"));   return result;

    case Page::_OpenImage:     result = action(OpenImageInNewWindow);    result->setText(tr("OpenImage"));     return result;
  //case Page::_DownloadImage: result = action(DownloadImageToDisk);     result->setText(tr("DownloadImage")); return result;
    case Page::_CopyImage:     result = action(CopyImageToClipboard);    result->setText(tr("CopyImage"));     return result;
  //case Page::_CopyImageUrl:  result = action(CopyImageUrlToClipboard); result->setText(tr("CopyImageUrl"));  return result;
    default: break;
    }
    return m_Page->Action(a, data);
}

void WebKitPage::HandleUnsupportedContent(QNetworkReply *reply){
#ifdef NETWORK_ACCESS_DEBUG
    qDebug() << "unsupported content.";
#endif

    if(!reply) return;
    QUrl url = reply->url();
    if(url.isEmpty()) return;
    if(url.scheme() == QStringLiteral("abp")) return;

    switch (reply->error()){
    case QNetworkReply::NoError:{
        if(reply->header(QNetworkRequest::ContentTypeHeader).isValid()){

            TreeBank *tb = m_View->GetTreeBank();

            if(DownloadItem *item = NetworkController::Download(reply)){
                if(tb && tb->GetNotifier())
                    tb->GetNotifier()->RegisterDownload(item);
            }
            if(tb) tb->GoBackOrCloseForDownload(m_View);
            return;
        }
        break;
    }
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::SslHandshakeFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentReSendError:
    case QNetworkReply::ProtocolUnknownError:
    case QNetworkReply::ProtocolInvalidOperationError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::UnknownProxyError:
    case QNetworkReply::UnknownContentError:
    case QNetworkReply::ProtocolFailure:
        return;
    default: break;
    }

    QWebFrame *frame = mainFrame();
    if(!frame) return;
    if(reply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()){
        QByteArray data = reply->readAll().toLower();
        if(// for html or xhtml ~version 4.
           data.contains("<!doctype") ||
           data.contains("<script")   ||
           data.contains("<html")     ||
           data.contains("<html")     ||
           data.contains("<head")     ||
           data.contains("<iframe")   ||
           data.contains("<h1")       ||
           data.contains("<div")      ||
           data.contains("<font")     ||
           data.contains("<table")    ||
           data.contains("<a")        ||
           data.contains("<style")    ||
           data.contains("<title")    ||
           data.contains("<b")        ||
           data.contains("<body")     ||
           data.contains("<br")       ||
           data.contains("<p")        ||
           // for xhtml version 5,
           //ignore tag if its length is less than 4.
           data.contains(":script")   ||
           data.contains(":html")     ||
           data.contains(":head")     ||
           data.contains(":iframe")   ||
           data.contains(":font")     ||
           data.contains(":table")    ||
           data.contains(":style")    ||
           data.contains(":title")    ||
           data.contains(":body")){

            frame->setHtml(QLatin1String(data), url);
            return;
        }
    }
    /*not found*/
}

void WebKitPage::AddJsObject(){
    if(!mainFrame()) return;

    if(m_View && m_View->GetJsObject()){
        mainFrame()->addToJavaScriptWindowObject(QStringLiteral("_view"), m_View->GetJsObject());
        m_View->OnSetJsObject(m_View->GetJsObject());
    }
    if(m_View && m_View->GetTreeBank() && m_View->GetTreeBank()->GetJsObject()){
        mainFrame()->addToJavaScriptWindowObject(QStringLiteral("_vanilla"), m_View->GetTreeBank()->GetJsObject());
        m_View->OnSetJsObject(m_View->GetTreeBank()->GetJsObject());
    }
}

////////////////////////////////////////////////////////////////////////////////

void WebKitPage::Print(){
    QPrinter printer;
    QPrintDialog dialog(&printer, Application::CurrentWidget());
    if(dialog.exec() == QDialog::Accepted){
        mainFrame()->print(&printer);
    }
}

void WebKitPage::InspectElement(){
    // my inspector doesn't catch element.
    //QWebInspector *inspector = new QWebInspector;
    //inspector->setPage(this);
    //inspector->show();
    QWebPage::triggerAction(QWebPage::InspectElement);
}

void WebKitPage::AddSearchEngine(QPoint pos){
    QWebHitTestResult r = mainFrame()->hitTestContent(pos);
    QWebElement elem = r.element();
    QString name = elem.attribute(QStringLiteral("name"));
    QWebElement form = elem;
    while(!form.isNull() && form.tagName().toLower() != QStringLiteral("form")){
        form = form.parent();
    }
    if(form.isNull()) return;

    QString encode = form.attribute(QStringLiteral("accept-charset"), QStringLiteral("UTF-8"));
    QString method = form.attribute(QStringLiteral("method"), QStringLiteral("get"));
    if(method.toLower() != QStringLiteral("get")) return;

    QUrl result = Page::StringToUrl(form.attribute(QStringLiteral("action")), currentFrame()->baseUrl());
    QUrlQuery queries = QUrlQuery(result);
    QMap<QString, QString> engines;

    QWebElementCollection fields = form.findAll(QStringLiteral("input"));
    foreach(QWebElement field, fields){
        QString type = field.attribute(QStringLiteral("type"), QStringLiteral("text")).toLower();
        type = QString::fromLatin1(QUrl::toPercentEncoding(type));
        QString name = field.attribute(QStringLiteral("name"));
        name = QString::fromLatin1(QUrl::toPercentEncoding(name));
        QString val = field.evaluateJavaScript(QStringLiteral("this.value")).toString();
        val = QString::fromLatin1(QUrl::toPercentEncoding(val));
        if(type == QStringLiteral("submit")){
            engines[name] = val; continue;
        } else if(type == QStringLiteral("text") || type == QStringLiteral("search")){
            if(field == elem) val = QStringLiteral("{query}");
        } else if(type == QStringLiteral("checkbox") || type == QStringLiteral("radio")){
            if(!field.evaluateJavaScript(QStringLiteral("this.checked")).toBool()) continue;
        } else if(type != QStringLiteral("hidden")) continue;

        queries.addQueryItem(name, val);
    }

    QWebElementCollection buttons = form.findAll(QStringLiteral("button"));
    foreach(QWebElement button, buttons){
        QString name = button.attribute(QStringLiteral("name"));
        name = QString::fromLatin1(QUrl::toPercentEncoding(name));
        QString val = button.attribute(QStringLiteral("aria-label"));
        val = QString::fromLatin1(QUrl::toPercentEncoding(val));
        engines[name] = val;
    }

    QWebElementCollection selects = form.findAll(QStringLiteral("select"));
    foreach(QWebElement select, selects){
        int index = select.evaluateJavaScript(QStringLiteral("this.selectedIndex")).toInt();

        if(index != -1){
            QWebElementCollection options = select.findAll(QStringLiteral("option"));
            QString name = select.attribute(QStringLiteral("name"));
            QString val = options[index].toPlainText();
            queries.addQueryItem(name, val);
        }
    }

    QString tag;
    QWebElementCollection labels =
        form.findAll(QStringLiteral("label[for=\"%1\"]").arg(name));
    if(labels.count() > 0) tag = labels[0].toPlainText();

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
}

void WebKitPage::AddBookmarklet(QPoint pos){
    SharedWebElement elem = m_View->HitElement(pos);

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
}

void WebKitPage::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    DownloadItem *item =
        NetworkController::Download(static_cast<NetworkAccessManager*>(networkAccessManager()),
                                    req, NetworkController::ToVariable);
    connect(item, &DownloadItem::DownloadResult, this, &WebKitPage::SuggestResult);
}

#endif //ifdef WEBKITVIEW
