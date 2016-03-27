#include "switch.hpp"
#include "const.hpp"

#include "page.hpp"

#ifdef QTWEBKIT
#  include <QWebPage>
#  include <QWebFrame>
#  include <QWebElement>
#endif

#include <QClipboard>
#include <QList>
#include <QSet>
#include <QNetworkRequest>
#include <QStyle>

#include "view.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#if defined(Q_OS_WIN)
#  include "tridentview.hpp"
#endif
#ifdef QTWEBKIT
#  include "webpage.hpp"
#endif
#include "webenginepage.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "dialog.hpp"

QMap<QString, SearchEngine> Page::m_SearchEngineMap = QMap<QString, SearchEngine>();
QMap<QString, Bookmarklet> Page::m_BookmarkletMap = QMap<QString, Bookmarklet>();

Page::OpenCommandOperation Page::m_OpenCommandOperation = Page::InNewViewNode;

Page::Page(QObject *parent, NetworkAccessManager *nam)
    : QObject(parent)
{
    m_ActionTable = QMap<Page::CustomAction, QAction*>();
    m_NetworkAccessManager = nam;

    switch(m_OpenCommandOperation){
    case InNewViewNode:
        m_OpenInNewMethod0 = &Page::OpenInNewViewNode;
        m_OpenInNewMethod1 = &Page::OpenInNewViewNode;
        m_OpenInNewMethod2 = &Page::OpenInNewViewNode;
        m_OpenInNewMethod3 = &Page::OpenInNewViewNode;
        break;
    case InNewHistNode:
        m_OpenInNewMethod0 = &Page::OpenInNewHistNode;
        m_OpenInNewMethod1 = &Page::OpenInNewHistNode;
        m_OpenInNewMethod2 = &Page::OpenInNewHistNode;
        m_OpenInNewMethod3 = &Page::OpenInNewHistNode;
        break;
    case InNewDirectory:
        m_OpenInNewMethod0 = &Page::OpenInNewDirectory;
        m_OpenInNewMethod1 = &Page::OpenInNewDirectory;
        m_OpenInNewMethod2 = &Page::OpenInNewDirectory;
        m_OpenInNewMethod3 = &Page::OpenInNewDirectory;
        break;
    case OnRoot:
        m_OpenInNewMethod0 = &Page::OpenOnRoot;
        m_OpenInNewMethod1 = &Page::OpenOnRoot;
        m_OpenInNewMethod2 = &Page::OpenOnRoot;
        m_OpenInNewMethod3 = &Page::OpenOnRoot;
        break;

    case InNewViewNodeBackground:
        m_OpenInNewMethod0 = &Page::OpenInNewViewNodeBackground;
        m_OpenInNewMethod1 = &Page::OpenInNewViewNodeBackground;
        m_OpenInNewMethod2 = &Page::OpenInNewViewNodeBackground;
        m_OpenInNewMethod3 = &Page::OpenInNewViewNodeBackground;
        break;
    case InNewHistNodeBackground:
        m_OpenInNewMethod0 = &Page::OpenInNewHistNodeBackground;
        m_OpenInNewMethod1 = &Page::OpenInNewHistNodeBackground;
        m_OpenInNewMethod2 = &Page::OpenInNewHistNodeBackground;
        m_OpenInNewMethod3 = &Page::OpenInNewHistNodeBackground;
        break;
    case InNewDirectoryBackground:
        m_OpenInNewMethod0 = &Page::OpenInNewDirectoryBackground;
        m_OpenInNewMethod1 = &Page::OpenInNewDirectoryBackground;
        m_OpenInNewMethod2 = &Page::OpenInNewDirectoryBackground;
        m_OpenInNewMethod3 = &Page::OpenInNewDirectoryBackground;
        break;
    case OnRootBackground:
        m_OpenInNewMethod0 = &Page::OpenOnRootBackground;
        m_OpenInNewMethod1 = &Page::OpenOnRootBackground;
        m_OpenInNewMethod2 = &Page::OpenOnRootBackground;
        m_OpenInNewMethod3 = &Page::OpenOnRootBackground;
        break;

    case InNewViewNodeNewWindow:
        m_OpenInNewMethod0 = &Page::OpenInNewViewNodeNewWindow;
        m_OpenInNewMethod1 = &Page::OpenInNewViewNodeNewWindow;
        m_OpenInNewMethod2 = &Page::OpenInNewViewNodeNewWindow;
        m_OpenInNewMethod3 = &Page::OpenInNewViewNodeNewWindow;
        break;
    case InNewHistNodeNewWindow:
        m_OpenInNewMethod0 = &Page::OpenInNewHistNodeNewWindow;
        m_OpenInNewMethod1 = &Page::OpenInNewHistNodeNewWindow;
        m_OpenInNewMethod2 = &Page::OpenInNewHistNodeNewWindow;
        m_OpenInNewMethod3 = &Page::OpenInNewHistNodeNewWindow;
        break;
    case InNewDirectoryNewWindow:
        m_OpenInNewMethod0 = &Page::OpenInNewDirectoryNewWindow;
        m_OpenInNewMethod1 = &Page::OpenInNewDirectoryNewWindow;
        m_OpenInNewMethod2 = &Page::OpenInNewDirectoryNewWindow;
        m_OpenInNewMethod3 = &Page::OpenInNewDirectoryNewWindow;
        break;
    case OnRootNewWindow:
        m_OpenInNewMethod0 = &Page::OpenOnRootNewWindow;
        m_OpenInNewMethod1 = &Page::OpenOnRootNewWindow;
        m_OpenInNewMethod2 = &Page::OpenOnRootNewWindow;
        m_OpenInNewMethod3 = &Page::OpenOnRootNewWindow;
        break;

    default:
        m_OpenInNewMethod0 = &Page::OpenInNewViewNode;
        m_OpenInNewMethod1 = &Page::OpenInNewViewNode;
        m_OpenInNewMethod2 = &Page::OpenInNewViewNode;
        m_OpenInNewMethod3 = &Page::OpenInNewViewNode;
    }

    connect(this,   SIGNAL(urlChanged(const QUrl&)),
            parent, SIGNAL(urlChanged(const QUrl&)));
    connect(this,   SIGNAL(titleChanged(const QString&)),
            parent, SIGNAL(titleChanged(const QString&)));
    connect(this,   SIGNAL(loadStarted()),
            parent, SIGNAL(loadStarted()));
    connect(this,   SIGNAL(loadProgress(int)),
            parent, SIGNAL(loadProgress(int)));
    connect(this,   SIGNAL(loadFinished(bool)),
            parent, SIGNAL(loadFinished(bool)));
    connect(this,   SIGNAL(statusBarMessage(const QString&)),
            parent, SIGNAL(statusBarMessage(const QString&)));
    connect(this,   SIGNAL(statusBarMessage2(const QString&, const QString&)),
            parent, SIGNAL(statusBarMessage2(const QString&, const QString&)));
    connect(this,   SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
            parent, SIGNAL(linkHovered(const QString&, const QString&, const QString&)));

    connect(this,   SIGNAL(ViewChanged()),
            parent, SIGNAL(ViewChanged()));
    connect(this,   SIGNAL(ScrollChanged(QPointF)),
            parent, SIGNAL(ScrollChanged(QPointF)));
    connect(this,   SIGNAL(ButtonCleared()),
            parent, SIGNAL(ButtonCleared()));
    connect(this,   SIGNAL(RenderFinished()),
            parent, SIGNAL(RenderFinished()));
}

Page::~Page(){}

NetworkAccessManager *Page::GetNetworkAccessManager(){
    NetworkAccessManager *nam;
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page()))
        nam = qobject_cast<NetworkAccessManager*>(page->networkAccessManager());
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page()))
        nam = qobject_cast<NetworkAccessManager*>(page->networkAccessManager());
#endif
    else nam = m_NetworkAccessManager;
    return nam;
}

QUrl Page::CreateQueryUrl(QString query, QString key){
    SearchEngine engine;
    if(key.isEmpty())
        engine = PrimarySearchEngine();
    else if(m_SearchEngineMap.contains(key))
        engine = m_SearchEngineMap[key];
    else
        engine = PrimarySearchEngine();

    QString format = engine[0];
    QByteArray encode = engine[1].toLatin1();

    if(QTextCodec *codec = QTextCodec::codecForName(encode))
        query = QString::fromLatin1(codec->fromUnicode(query).toPercentEncoding());
    else
        query = QString::fromLatin1(query.toUtf8().toPercentEncoding());
    return QUrl::fromEncoded(format.arg(query).toLatin1());
}

QUrl Page::UpDirectoryUrl(QUrl url){
    QStringList urlstr = QString::fromUtf8(url.toEncoded()).split(QStringLiteral("/"));
    if(( urlstr.endsWith(QString()) && urlstr.length() > 4) ||
       (!urlstr.endsWith(QString()) && urlstr.length() > 3))
        urlstr.removeLast();
    if(urlstr.length() > 3)
        urlstr.removeLast();
    return QUrl::fromEncoded(urlstr.join(QStringLiteral("/")).toLatin1());
}

QUrl Page::StringToUrl(QString str, QUrl baseUrl){
    if(str.isEmpty()) return QUrl();
    if(!baseUrl.isEmpty() &&
       !baseUrl.scheme().isEmpty()){
        QStringList base = QString::fromUtf8(baseUrl.toEncoded()).split(QStringLiteral("/"));
        if(str.startsWith(QStringLiteral("about:"))        ||
           str.startsWith(QStringLiteral("aim:"))          ||
           str.startsWith(QStringLiteral("callto:"))       ||
           str.startsWith(QStringLiteral("chrome:"))       ||
           str.startsWith(QStringLiteral("clsid:"))        ||
           str.startsWith(QStringLiteral("data:"))         ||
           str.startsWith(QStringLiteral("disk:"))         ||
           str.startsWith(QStringLiteral("feed:"))         ||
           str.startsWith(QStringLiteral("file:"))         ||
           str.startsWith(QStringLiteral("ftp:"))          ||
           str.startsWith(QStringLiteral("gopher:"))       ||
           str.startsWith(QStringLiteral("hcp:"))          ||
           str.startsWith(QStringLiteral("help:"))         ||
           str.startsWith(QStringLiteral("http:"))         ||
           str.startsWith(QStringLiteral("https:"))        ||
           str.startsWith(QStringLiteral("irc:"))          ||
           str.startsWith(QStringLiteral("javascript:"))   ||
           str.startsWith(QStringLiteral("livescript:"))   ||
           str.startsWith(QStringLiteral("lynxcgi:"))      ||
           str.startsWith(QStringLiteral("lynxexec:"))     ||
           str.startsWith(QStringLiteral("mailto:"))       ||
           str.startsWith(QStringLiteral("mhtml:"))        ||
           str.startsWith(QStringLiteral("mk:"))           ||
           str.startsWith(QStringLiteral("mocha:"))        ||
           str.startsWith(QStringLiteral("montulli:"))     ||
           str.startsWith(QStringLiteral("ms-help:"))      ||
           str.startsWith(QStringLiteral("ms-its:"))       ||
           str.startsWith(QStringLiteral("news:"))         ||
           str.startsWith(QStringLiteral("nntp:"))         ||
           str.startsWith(QStringLiteral("opera:"))        ||
           str.startsWith(QStringLiteral("phone:"))        ||
           str.startsWith(QStringLiteral("prospero:"))     ||
           str.startsWith(QStringLiteral("res:"))          ||
           str.startsWith(QStringLiteral("resource:"))     ||
           str.startsWith(QStringLiteral("sftp:"))         ||
           str.startsWith(QStringLiteral("shell:"))        ||
           str.startsWith(QStringLiteral("ssh:"))          ||
           str.startsWith(QStringLiteral("sstp:"))         ||
           str.startsWith(QStringLiteral("tel:"))          ||
           str.startsWith(QStringLiteral("telnet:"))       ||
           str.startsWith(QStringLiteral("vbscript:"))     ||
           str.startsWith(QStringLiteral("view-source:"))  ||
           str.startsWith(QStringLiteral("vnd.ms.radio:")) ||
           str.startsWith(QStringLiteral("wais:"))         ||
           str.startsWith(QStringLiteral("webcal:"))       ||
           str.startsWith(QStringLiteral("wimg:"))         ||
           str.startsWith(QStringLiteral("worldwind:"))    ||
           str.startsWith(QStringLiteral("wysiwyg:"))      ||
           str.startsWith(QStringLiteral("qrc:/"))         ||
           str.startsWith(QStringLiteral(":/"))){
            /*do nothing.*/
        } else if(str.startsWith(QStringLiteral("//"))){
            str = base[0] + str;
        } else if(str.startsWith(QStringLiteral("/"))){
            base = base.mid(0, 3);
            str = base.join(QStringLiteral("/")) + str;
        } else {
            base.removeLast();
            base.append(str);
            str = base.join(QStringLiteral("/"));
        }
    }

    QString reconverted = QString::fromLatin1(str.toLatin1());
    if(str == reconverted)
        return QUrl::fromEncoded(str.toLatin1());
    else
        return QUrl(str);
}

QList<QUrl> Page::ExtractUrlFromHtml(QString html, QUrl baseUrl, FindElementsOption option){
#ifdef QTWEBKIT
    QString selector;
    QString attribute;
    if(option == HaveSource){
        selector = HAVE_SOURCE_CSS_SELECTOR;
        attribute = "src";
    } else if(option == HaveReference){
        selector = HAVE_REFERENCE_CSS_SELECTOR;
        attribute = "href";
    }
    QWebPage page;
    page.mainFrame()->setHtml(html, baseUrl);
    QList<QUrl> list;
    foreach(QWebElement elem, page.mainFrame()->findAllElements(selector)){
        list << StringToUrl(elem.attribute(attribute), baseUrl);
    }
    return list.toSet().toList();
#else
    QList<QUrl> list;
    int pos = 0;
    QRegularExpression reg;
    if(option == HaveSource)
        reg = QRegularExpression(QStringLiteral("src=(\"?)([^<>\\{\\}(\\)\"\\`\\'\\^\\|\n\r\t \\\\]+)\\1"));
    if(option == HaveReference)
        reg = QRegularExpression(QStringLiteral("href=(\"?)([^<>\\{\\}(\\)\"\\`\\'\\^\\|\n\r\t \\\\]+)\\1"));
    if(reg.isValid()){
        QRegularExpressionMatch match;
        while((match = reg.match(html, pos)).hasMatch()){
            list << StringToUrl(match.captured(2), baseUrl);
            pos = match.capturedEnd();
        }
    }
    return list.toSet().toList();
#endif //ifdef QTWEBKIT
}

QList<QUrl> Page::ExtractUrlFromText(QString text, QUrl baseUrl){
    QList<QUrl> list;
    // '(', ')' are usable in url, but it's thought to be undesirable...
    QStringList strs = text.split(QRegularExpression(QStringLiteral("[<>\\{\\}(\\)\"\\`\\'\\^\\|\n\r\t \\\\]+")));
    foreach(QString str, strs){
        str = str.trimmed();
        if(str.startsWith(QStringLiteral("/")) || str.startsWith(QStringLiteral("./")) || str.startsWith(QStringLiteral("../"))) ; // do nothing.
        else if(str.contains(QRegularExpression(QStringLiteral("^.*view-source:")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral("view-source:"))));
        else if(str.contains(QRegularExpression(QStringLiteral( "^.*iew-source:")))) str = QStringLiteral("v"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral( "iew-source:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(  "^.*ew-source:")))) str = QStringLiteral("vi"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(  "ew-source:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(   "^.*w-source:")))) str = QStringLiteral("vie"      ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(   "w-source:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(    "^.*-source:")))) str = QStringLiteral("view"     ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(    "-source:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(     "^.*source:")))) str = QStringLiteral("view-"    ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(     "source:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(      "^.*ource:")))) str = QStringLiteral("view-s"   ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(      "ource:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*urce:")))) str = QStringLiteral("view-so"  ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "urce:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*rce:")))) str = QStringLiteral("view-sou" ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "rce:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(         "^.*ce:")))) str = QStringLiteral("view-sour") + str.mid(str.indexOf(QRegularExpression(QStringLiteral(         "ce:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(     "^.*sftp://")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(     "sftp://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(      "^.*ftp://")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(      "ftp://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(   "^.*https?://")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(   "https?://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(    "^.*ttps?://")))) str = QStringLiteral("h"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(    "ttps?://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(     "^.*tps?://")))) str = QStringLiteral("ht"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(     "tps?://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(      "^.*ps?://")))) str = QStringLiteral("htt"      ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(      "ps?://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*s?://")))) str = QStringLiteral("http"     ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "s?://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*file:")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "file:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*ile:")))) str = QStringLiteral("f"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "ile:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(         "^.*le:")))) str = QStringLiteral("fi"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(         "le:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(          "^.*e:")))) str = QStringLiteral("fil"      ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(          "e:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(      "^.*about:")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(      "about:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*bout:")))) str = QStringLiteral("a"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "bout:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*out:")))) str = QStringLiteral("ab"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "out:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(         "^.*ut:")))) str = QStringLiteral("abo"      ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(         "ut:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(          "^.*t:")))) str = QStringLiteral("abou"     ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(          "t:"))));
        else if(str.split(QStringLiteral("/")).first().contains(QRegularExpression(QStringLiteral(".\\.[a-z]+(?::[0-9]+)?$"))))
            str = QStringLiteral("http://") + str;
        else if(str.split(QStringLiteral("/")).first().contains(QRegularExpression(QStringLiteral("(?:[1-9]|[1-9][0-9]|[1-2][0-9][0-9])(?:\\.(?:[1-9]|[1-9][0-9]|[1-2][0-9][0-9])){3}(?::[0-9]+)?$"))))
            str = QStringLiteral("http://") + str;
        else if(str.split(QStringLiteral("/")).first().contains(QRegularExpression(QStringLiteral("\\[[0-9a-f]{1,4}(?::[0-9a-f]{0,4}){1,7}\\](?::[0-9]+)?$"))))
            str = QStringLiteral("http://") + str;
        else if(str.split(QStringLiteral("/")).first().contains(QRegularExpression(QStringLiteral("[0-9a-f]{1,4}(?::[0-9a-f]{0,4}){1,7}$")))){
            QStringList l = str.split(QStringLiteral("/"));
            QString host = l.takeFirst();
            str = QStringLiteral("http://[") + host + QStringLiteral("]");
            if(!l.isEmpty()) str = str + QStringLiteral("/") + l.join(QStringLiteral("/"));
        }
        else continue;
        if(str.contains(QStringLiteral("file://localhost/"))) str = str.replace(QStringLiteral("file://localhost/"), QStringLiteral("file:///"));
        list << StringToUrl(str, baseUrl);
    }
    return list.toSet().toList();
}

QList<QUrl> Page::DirtyStringToUrlList(QString str){
    QList<QUrl> urls = ExtractUrlFromText(str);
    if(!urls.isEmpty()) return urls;
    urls << CreateQueryUrl(str);
    return urls;
}

void Page::RegisterBookmarklet(QString key, Bookmarklet bookmark){
    m_BookmarkletMap[key] = bookmark;
}

void Page::RemoveBookmarklet(QString key){
    m_BookmarkletMap.remove(key);
}

void Page::ClearBookmarklet(){
    m_BookmarkletMap.clear();
}

QMap<QString, Bookmarklet> Page::GetBookmarkletMap(){
    return m_BookmarkletMap;
}

Bookmarklet Page::GetBookmarklet(QString key){
    return m_BookmarkletMap[key];
}

void Page::RegisterDefaultSearchEngines(){
    SearchEngine google;
    google << QStringLiteral("https://www.google.co.jp/search?c&q=%1&ie=UTF-8&oe=UTF-8") << QStringLiteral("UTF-8") << QStringLiteral("true");
    SearchEngine yahoo;
    yahoo << QStringLiteral("http://search.yahoo.co.jp/search?ei=UTF-8&p=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine bing;
    bing << QStringLiteral("https://www.bing.com/search?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine amazon;
    amazon << QStringLiteral("https://amazon.co.jp/s/?field-keywords=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine wiki;
    wiki << QStringLiteral("https://ja.wikipedia.org/w/index.php?search=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine ifl;
    ifl << QStringLiteral("https://www.google.co.jp/search?btnI=I%27m+Feeling+Lucky&ie=UTF-8&oe=UTF-8&q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");

    m_SearchEngineMap[QStringLiteral("google")] = google;
    m_SearchEngineMap[QStringLiteral("yahoo")] = yahoo;
    m_SearchEngineMap[QStringLiteral("bing")] = bing;
    m_SearchEngineMap[QStringLiteral("amazon")] = amazon;
    m_SearchEngineMap[QStringLiteral("wiki")] = wiki;
    m_SearchEngineMap[QStringLiteral("ifl")] = ifl;
}

void Page::RegisterSearchEngine(QString key, SearchEngine engine){
    m_SearchEngineMap[key] = engine;
}

void Page::RemoveSearchEngine(QString key){
    m_SearchEngineMap.remove(key);
}

void Page::ClearSearchEngine(){
    m_SearchEngineMap.clear();
}

QMap<QString, SearchEngine> Page::GetSearchEngineMap(){
    return m_SearchEngineMap;
}

SearchEngine Page::GetSearchEngine(QString key){
    return m_SearchEngineMap[key];
}

SearchEngine Page::PrimarySearchEngine(){
    foreach(QString key, m_SearchEngineMap.keys()){
        if(m_SearchEngineMap[key].length() < 3)
            m_SearchEngineMap[key] << QStringLiteral("false");
        else if(m_SearchEngineMap[key][2] == QStringLiteral("true"))
            return m_SearchEngineMap[key];
    }
    if(m_SearchEngineMap[QStringLiteral("google")].isEmpty()){
        return *m_SearchEngineMap.begin();
    } else {
        return m_SearchEngineMap[QStringLiteral("google")];
    }
}

bool Page::ShiftMod(){
    return Application::keyboardModifiers() & Qt::ShiftModifier;
}

bool Page::CtrlMod(){
    return Application::keyboardModifiers() & Qt::ControlModifier;
}

bool Page::Activate(){
    if(View::ActivateNewViewDefault())
        return !CtrlMod();
    else
        return CtrlMod();
}

void Page::Download(const QNetworkRequest &req,
                    const QString &file){

    NetworkAccessManager *nam = GetNetworkAccessManager();
    if(!nam) return;

    DownloadItem *item = 0;

    if(file.isEmpty()){
        item = NetworkController::Download(nam, req);
    } else {
        item = NetworkController::Download(nam, req, NetworkController::SelectedDirectory);
        if(item) item->SetPathAndReady(file);
    }

    if(item && GetTB() && GetTB()->GetNotifier()){
        GetTB()->GetNotifier()->RegisterDownload(item);
    }
}

void Page::Download(const QUrl &target,
                    const QUrl &referer,
                    const QString &file){

    QNetworkRequest req(target);
    req.setRawHeader("Referer", referer.toEncoded());
    Download(req, file);
}

void Page::Download(const QString &url,
                    const QString &file){

    Download(DirtyStringToUrlList(url).first(),
             m_View->url(), file);
}

void Page::SetSource(const QUrl &url){
    m_View->GetHistNode()->SetUrl(QUrl());
    QUrl other = QUrl::fromEncoded(url.toEncoded().mid(12));
    QNetworkRequest req(other);
    req.setRawHeader("Referer", other.toEncoded());
    NetworkAccessManager *nam = GetNetworkAccessManager();
    if(!nam) return;
    DownloadItem *item =
        NetworkController::Download(nam, req, NetworkController::ToVariable);
    connect(item, SIGNAL(DownloadResult(const QByteArray&)),
            this, SLOT(SetSource(const QByteArray&)));
}

void Page::SetSource(const QByteArray &html){
    // undesirable dependency on 'QWebSetting'.
    QByteArray encoding = "UTF-8"; //settings()->defaultTextEncoding().toLatin1();
    if(QTextCodec *codec = QTextCodec::codecForHtml(html, QTextCodec::codecForName(encoding)))
        SetSource(codec->toUnicode(html));
    else
        SetSource(QString::fromUtf8(html));
}

void Page::SetSource(const QString &html){
    QUrl url = QUrl(m_View->GetHistNode()->GetUrl());
    QString viewable = QString(html);
    viewable.replace(QStringLiteral("&lt;"), QStringLiteral("{{{lt}}}"));
    viewable.replace(QStringLiteral("&gt;"), QStringLiteral("{{{gt}}}"));
    viewable.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
    viewable.replace(QStringLiteral(">"), QStringLiteral("&gt;"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!([dD][oO][cC][tT][yY][pP][eE](?:(?!&gt;).)*)&gt;")),
                     QStringLiteral("{{{doctype}}}!\\1{{{/doctype}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!(--+)&gt;")),
                     QStringLiteral("{{{lt}}}!\\1{{{gt}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!--((?:(?:(?!&gt;|&lt;|\\[).)*)\\[(?:(?:(?!&gt;|&lt;|\\]).)*)\\](?:(?:(?!&gt;).)*))--&gt;")),
                     QStringLiteral("{{{switch0}}}!--\\1--{{{/switch0}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!--\\[((?:(?!&gt;|\\]).)*)\\]&gt;")),
                     QStringLiteral("{{{switch1}}}\\1{{{/switch1}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!--&lt;!\\[((?:(?!&gt;|\\]).)*)\\]--&gt;")),
                     QStringLiteral("{{{lt}}}!--{{{switch2}}}\\1{{{/switch2}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!--\\[((?:(?!\\]|&gt;).)*)\\]&gt;")),
                     QStringLiteral("{{{switch1}}}\\1{{{/switch1}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!\\[((?:(?!\\]|&gt;).)*)\\]--&gt;")),
                     QStringLiteral("{{{switch2}}}\\1{{{/switch2}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;!--((?:(?!--&gt;).)*)--&gt;")),
                     QStringLiteral("{{{comment}}}\\1{{{/comment}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;(/?script(?:(?!&gt;).)*)&gt;")),
                     QStringLiteral("{{{script}}}\\1{{{/script}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;(/?style(?:(?!&gt;).)*)&gt;")),
                     QStringLiteral("{{{style}}}\\1{{{/style}}}"));

    viewable.replace(QRegularExpression(QStringLiteral("&lt;(/?[a-zA-Z][a-zA-Z0-9]*(?:[ \t\n\r\f](?:(?!&gt;).)*)?)&gt;")),
                     QStringLiteral("{{{alltag}}}\\1{{{/alltag}}}"));

    viewable.replace(QStringLiteral("{{{doctype}}}")  , QStringLiteral("<font color=\"purple\">&lt;"));
    viewable.replace(QStringLiteral("{{{/doctype}}}") , QStringLiteral("&gt;</font>"));

    viewable.replace(QStringLiteral("{{{switch0}}}")  , QStringLiteral("<font color=\"red\">&lt;"));
    viewable.replace(QStringLiteral("{{{/switch0}}}") , QStringLiteral("&gt;</font>"));

    viewable.replace(QStringLiteral("{{{switch1}}}")  , QStringLiteral("<font color=\"red\">&lt;!--["));
    viewable.replace(QStringLiteral("{{{/switch1}}}") , QStringLiteral("]&gt;</font>"));

    viewable.replace(QStringLiteral("{{{switch2}}}")  , QStringLiteral("<font color=\"red\">&lt;!["));
    viewable.replace(QStringLiteral("{{{/switch2}}}") , QStringLiteral("]--&gt;</font>"));

    viewable.replace(QStringLiteral("{{{comment}}}")  , QStringLiteral("<font color=\"gray\">&lt;!--"));
    viewable.replace(QStringLiteral("{{{/comment}}}") , QStringLiteral("--&gt;</font>"));

    viewable.replace(QStringLiteral("{{{script}}}")   , QStringLiteral("<font color=\"olive\">&lt;"));
    viewable.replace(QStringLiteral("{{{/script}}}")  , QStringLiteral("&gt;</font>"));

    viewable.replace(QStringLiteral("{{{style}}}")    , QStringLiteral("<font color=\"teal\">&lt;"));
    viewable.replace(QStringLiteral("{{{/style}}}")   , QStringLiteral("&gt;</font>"));

    viewable.replace(QStringLiteral("{{{alltag}}}")   , QStringLiteral("<font color=\"blue\">&lt;"));
    viewable.replace(QStringLiteral("{{{/alltag}}}")  , QStringLiteral("&gt;</font>"));

    viewable.replace(QStringLiteral("{{{lt}}}")       , QStringLiteral("&amp;lt;"));
    viewable.replace(QStringLiteral("{{{gt}}}")       , QStringLiteral("&amp;gt;"));

    viewable.replace(QStringLiteral("\n"), QStringLiteral("<br>\n"));
    viewable.replace(QRegularExpression(QStringLiteral("^(.*)$")),
                     QStringLiteral("<pre style=\"white-space:normal;\">\\1</pre>"));
    viewable = QStringLiteral(
        "<html><head></head>"
      VV"<body contenteditable=\"true\">") +
        viewable +
        QStringLiteral("</body></html>");

    m_View->setHtml(viewable, url);
    emit urlChanged(url);
    emit titleChanged(QString::fromUtf8(url.toEncoded()));
    emit loadFinished(true);
}

void Page::UpKey()       { m_View->UpKeyEvent();}
void Page::DownKey()     { m_View->DownKeyEvent();}
void Page::RightKey()    { m_View->RightKeyEvent();}
void Page::LeftKey()     { m_View->LeftKeyEvent();}
void Page::HomeKey()     { m_View->HomeKeyEvent();}
void Page::EndKey()      { m_View->EndKeyEvent();}
void Page::PageUpKey()   { m_View->PageUpKeyEvent();}
void Page::PageDownKey() { m_View->PageDownKeyEvent();}

void Page::Import(){
    Application::Import(GetTB());
}

void Page::Export(){
    Application::Export(GetTB());
}

void Page::AboutVanilla(){
    Application::AboutVanilla(GetTB());
}

void Page::AboutQt(){
    Application::AboutQt(GetTB());
}

void Page::Quit(){
    Application::Quit();
}

void Page::ToggleNotifier(){
    GetTB()->ToggleNotifier();
}

void Page::ToggleReceiver(){
    GetTB()->ToggleReceiver();
}

void Page::ToggleMenuBar(){
    GetTB()->GetMainWindow()->ToggleMenuBar();
}

void Page::ToggleTreeBar(){
    GetTB()->GetMainWindow()->ToggleTreeBar();
}

void Page::ToggleToolBar(){
    GetTB()->GetMainWindow()->ToggleToolBar();
}

void Page::ToggleFullScreen(){
    GetTB()->GetMainWindow()->ToggleFullScreen();
}

void Page::ToggleMaximized(){
    GetTB()->GetMainWindow()->ToggleMaximized();
}

void Page::ToggleMinimized(){
    GetTB()->GetMainWindow()->ToggleMinimized();
}

void Page::ToggleShaded(){
    GetTB()->GetMainWindow()->ToggleShaded();
}

MainWindow *Page::ShadeWindow(MainWindow *win){
    return Application::ShadeWindow(win ? win : GetTB()->GetMainWindow());
}

MainWindow *Page::UnshadeWindow(MainWindow *win){
    return Application::UnshadeWindow(win ? win : GetTB()->GetMainWindow());
}

MainWindow *Page::NewWindow(int id){
    return Application::NewWindow(id);
}

MainWindow *Page::CloseWindow(MainWindow *win){
    return Application::CloseWindow(win ? win : GetTB()->GetMainWindow());
}

MainWindow *Page::SwitchWindow(bool next){
    return Application::SwitchWindow(next);
}

MainWindow *Page::NextWindow(){
    return Application::NextWindow();
}

MainWindow *Page::PrevWindow(){
    return Application::PrevWindow();
}

void Page::Back(){
    View::SetSwitchingState(true);
    GetTB()->Back();
    View::SetSwitchingState(false);
}

void Page::Forward(){
    View::SetSwitchingState(true);
    GetTB()->Forward();
    View::SetSwitchingState(false);
}

void Page::UpDirectory(){
    View::SetSwitchingState(true);
    GetTB()->UpDirectory();
    View::SetSwitchingState(false);
}

void Page::Close(){
#if QT_VERSION >= 0x050600
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        if(page->ObscureDisplay()){
            page->triggerAction(QWebEnginePage::ExitFullScreen);
        }
    }
#endif
    View::SetSwitchingState(true);
    GetTB()->Close();
    View::SetSwitchingState(false);
}

void Page::Restore(){
    View::SetSwitchingState(true);
    GetTB()->Restore();
    View::SetSwitchingState(false);
}

void Page::Recreate(){
    View::SetSwitchingState(true);
    GetTB()->Recreate();
    View::SetSwitchingState(false);
}

void Page::NextView(){
    View::SetSwitchingState(true);
    GetTB()->NextView();
    View::SetSwitchingState(false);
}

void Page::PrevView(){
    View::SetSwitchingState(true);
    GetTB()->PrevView();
    View::SetSwitchingState(false);
}

void Page::BuryView(){
    View::SetSwitchingState(true);
    GetTB()->BuryView();
    View::SetSwitchingState(false);
}

void Page::DigView(){
    View::SetSwitchingState(true);
    GetTB()->DigView();
    View::SetSwitchingState(false);
}

void Page::FirstView(){
    View::SetSwitchingState(true);
    GetTB()->FirstView();
    View::SetSwitchingState(false);
}

void Page::SecondView(){
    View::SetSwitchingState(true);
    GetTB()->SecondView();
    View::SetSwitchingState(false);
}

void Page::ThirdView(){
    View::SetSwitchingState(true);
    GetTB()->ThirdView();
    View::SetSwitchingState(false);
}

void Page::FourthView(){
    View::SetSwitchingState(true);
    GetTB()->FourthView();
    View::SetSwitchingState(false);
}

void Page::FifthView(){
    View::SetSwitchingState(true);
    GetTB()->FifthView();
    View::SetSwitchingState(false);
}

void Page::SixthView(){
    View::SetSwitchingState(true);
    GetTB()->SixthView();
    View::SetSwitchingState(false);
}

void Page::SeventhView(){
    View::SetSwitchingState(true);
    GetTB()->SeventhView();
    View::SetSwitchingState(false);
}

void Page::EighthView(){
    View::SetSwitchingState(true);
    GetTB()->EighthView();
    View::SetSwitchingState(false);
}

void Page::NinthView(){
    View::SetSwitchingState(true);
    GetTB()->NinthView();
    View::SetSwitchingState(false);
}

void Page::TenthView(){
    View::SetSwitchingState(True);
    GetTB()->TenthView();
    View::SetSwitchingState(False);
}

void Page::NewViewNode(){
    View::SetSwitchingState(true);
    SuitTB()->NewViewNode(m_View->GetViewNode());
    View::SetSwitchingState(false);
}

void Page::NewHistNode(){
    View::SetSwitchingState(true);
    SuitTB()->NewHistNode(m_View->GetHistNode());
    View::SetSwitchingState(false);
}

void Page::CloneViewNode(){
    View::SetSwitchingState(true);
    SuitTB()->CloneViewNode(m_View->GetViewNode());
    View::SetSwitchingState(false);
}

void Page::CloneHistNode(){
    View::SetSwitchingState(true);
    SuitTB()->CloneHistNode(m_View->GetHistNode());
    View::SetSwitchingState(false);
}

void Page::DisplayAccessKey(){
    GetTB()->DisplayAccessKey();
}

void Page::DisplayViewTree(){
    GetTB()->DisplayViewTree();
}

void Page::DisplayHistTree(){
    GetTB()->DisplayHistTree();
}

void Page::DisplayTrashTree(){
    GetTB()->DisplayTrashTree();
}

void Page::OpenTextSeeker(){
    GetTB()->OpenTextSeeker();
}

void Page::OpenQueryEditor(){
    GetTB()->OpenQueryEditor();
}

void Page::OpenUrlEditor(){
    GetTB()->OpenUrlEditor();
}

void Page::OpenCommand(){
    GetTB()->OpenCommand();
}

void Page::ReleaseHiddenView(){
    GetTB()->ReleaseHiddenView();
}

void Page::Load(){
    m_View->Load();
}

void Page::Copy(){
    m_View->CallWithSelectedText
        ([](QString text){
            if(!text.isEmpty())
                Application::clipboard()->setText(text);
        });
}

void Page::Cut(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::Cut);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::Cut);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativeCutAction();
    }
#endif
}

void Page::Paste(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::Paste);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::Paste);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativePasteAction();
    }
#endif
}

void Page::Undo(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::Undo);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::Undo);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativeUndoAction();
    }
#endif
}

void Page::Redo(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::Redo);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::Redo);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativeRedoAction();
    }
#endif
}

void Page::Unselect(){
    m_View->EvaluateJavaScript(
        "(function(){\n"
        "    document.activeElement.blur();\n"
        "    getSelection().removeAllRanges();\n"
        "}());");
}

void Page::SelectAll(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::SelectAll);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::SelectAll);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativeSelectAllAction();
    }
#endif
}

void Page::Reload(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::Reload);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::Reload);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativeReloadAction();
    }
#endif
}

void Page::ReloadAndBypassCache(){
    m_View->ReloadAndBypassCache();
}

void Page::Stop(){
    // can implement without WebPage and WebEnginePage?
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page())){
        page->triggerAction(QWebEnginePage::Stop);
    }
#ifdef QTWEBKIT
    else if(WebPage *page = qobject_cast<WebPage*>(m_View->page())){
        page->triggerAction(QWebPage::Stop);
    }
#endif
#if defined(Q_OS_WIN)
    else if(TridentView *view = qobject_cast<TridentView*>(m_View->base())){
        view->TriggerNativeStopAction();
    }
#endif
}

void Page::StopAndUnselect(){
    Stop();
    Unselect();
}

void Page::Print(){
    m_View->Print();
}

void Page::Save(){
    QUrl url = m_View->url();
    QNetworkRequest req(url);
    req.setRawHeader("Referer", url.toEncoded());
    Download(req);
}

void Page::ZoomIn(){
    m_View->ZoomIn();
}

void Page::ZoomOut(){
    m_View->ZoomOut();
}

void Page::ViewSource(){
    QUrl newUrl = QUrl::fromEncoded("view-source:" + m_View->url().toEncoded());
    SharedView newView = SharedView();

    if(qobject_cast<WebEngineView*>(m_View->base()) ||
       qobject_cast<QuickWebEngineView*>(m_View->base())){
        newView = SuitTB()->OpenInNewViewNode(newUrl, Activate()||ShiftMod(), m_View->GetViewNode());
    } else {
        newView = SuitTB()->OpenInNewViewNode(QUrl(), Activate()||ShiftMod(), m_View->GetViewNode());
        HistNode *h = newView->GetHistNode();
        h->SetUrl(newUrl);
        m_View->CallWithWholeHtml([newView](QString html){ newView->SetSource(html);});
    }

    newView->SetMaster(m_View->GetThis());
    m_View->SetSlave(newView->GetThis());
}

void Page::ApplySource(){
    if(SharedView view = m_View->GetMaster().lock()){
        QUrl url = QUrl(view->url());
        m_View->CallWithWholeText([view](QString text){ view->setHtml(text, view->url());});
    }
}

void Page::OpenBookmarklet(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        m_View->Load(GetBookmarklet(action->text()).first());
    }
}

void Page::SearchWith(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        m_View->CallWithSelectedText([this, action](QString text){
                OpenInNew(action->text(), text);
            });
    }
}

void Page::AddSearchEngine(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){
                    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base()))
                        // QWebEngineView's zoomFactor uses devicePixelRatio.
                        m_View->AddSearchEngine(e->Position() / w->zoomFactor());
                    else
                        m_View->AddSearchEngine(e->Position());
                }
            }
        } else {
            m_View->AddSearchEngine(action->data().toPoint());
        }
    }
}

void Page::AddBookmarklet(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){
                    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base()))
                        // QWebEngineView's zoomFactor uses devicePixelRatio.
                        m_View->AddBookmarklet(e->Position() / w->zoomFactor());
                    else
                        m_View->AddBookmarklet(e->Position());
                }
            }
        } else {
            m_View->AddBookmarklet(action->data().toPoint());
        }
    }
}

void Page::InspectElement(){
    m_View->InspectElement();
}

void Page::CopyUrl(){
    Application::clipboard()->setText(QString::fromUtf8(m_View->GetHistNode()->GetUrl().toEncoded()));
}

void Page::CopyTitle(){
    Application::clipboard()->setText(m_View->GetHistNode()->GetTitle());
}

void Page::CopyPageAsLink(){
    QString title = m_View->GetTitle();
    QString url = QString::fromUtf8(m_View->GetHistNode()->GetUrl().toEncoded());
    Application::clipboard()->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(url, title));
}

void Page::CopySelectedHtml(){
    m_View->CallWithSelectedHtml
        ([](QString html){
            if(!html.isEmpty())
                Application::clipboard()->setText(html);
        });
}

void Page::OpenWithIE(){
    Application::OpenUrlWith_IE(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithEdge(){
    Application::OpenUrlWith_Edge(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithFF(){
    Application::OpenUrlWith_FF(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithOpera(){
    Application::OpenUrlWith_Opera(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithOPR(){
    Application::OpenUrlWith_OPR(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithSafari(){
    Application::OpenUrlWith_Safari(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithChrome(){
    Application::OpenUrlWith_Chrome(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithSleipnir(){
    Application::OpenUrlWith_Sleipnir(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithVivaldi(){
    Application::OpenUrlWith_Vivaldi(m_View->GetHistNode()->GetUrl());
}

void Page::OpenWithCustom(){
    Application::OpenUrlWith_Custom(m_View->GetHistNode()->GetUrl());
}

void Page::ClickElement(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        QPoint pos;
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){

                    if(e->ClickEvent()) return;

                    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base()))
                        // QWebEngineView's zoomFactor uses devicePixelRatio.
                        pos = e->Position() / w->zoomFactor();
                    else
                        pos = e->Position();
                }
            }
        }
        if(pos.isNull()){
            pos = action->data().toPoint();
        }

        QMouseEvent pressEvent  (QEvent::MouseButtonPress,   pos, Qt::LeftButton, 0, 0);
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, pos, Qt::LeftButton, 0, 0);

        m_View->MousePressEvent(&pressEvent);
        m_View->MouseReleaseEvent(&releaseEvent);
    }
}

void Page::FocusElement(){
    // too redundant?
    if(QAction *action = qobject_cast<QAction*>(sender())){
        QPoint pos;
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){

                    e->SetFocus();

                    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base()))
                        // QWebEngineView's zoomFactor uses devicePixelRatio.
                        pos = e->Position() / w->zoomFactor();
                    else
                        pos = e->Position();
                }
            }
        }
        if(pos.isNull()){
            pos = action->data().toPoint();
        }
        m_View->CallWithHitElement
            (pos, [this](SharedWebElement e){
                if(e && !e->IsNull()){

                    e->SetFocus();

                    // 'hack-ish' solution...
                    QKeyEvent tabPress = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
                    QKeyEvent tabRelease = QKeyEvent(QEvent::KeyRelease, Qt::Key_Tab, Qt::NoModifier);
                    QKeyEvent backTabPress = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier);
                    QKeyEvent backTabRelease = QKeyEvent(QEvent::KeyRelease, Qt::Key_Tab, Qt::ShiftModifier);

                    m_View->KeyPressEvent(&tabPress);
                    m_View->KeyReleaseEvent(&tabRelease);
                    m_View->KeyPressEvent(&backTabPress);
                    m_View->KeyReleaseEvent(&backTabRelease);
                }
            });
    }
}

void Page::HoverElement(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        QPoint pos;
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){
                    // real client position.
                    pos = e->Position();
                }
            }
        }
        if(pos.isNull()){
            pos = action->data().toPoint();
        }

        if(TreeBank *tb = GetTB()){
            QCursor::setPos(tb->mapToGlobal(pos));
        }
    }
}

#define ELEMENT_ACTION(BLOCK)                                           \
    if(QAction *action = qobject_cast<QAction*>(sender())){             \
        if(action->data().canConvert<SharedWebElement>()){              \
            if(SharedWebElement e = action->data().value<SharedWebElement>()){ \
                if(!e->IsNull()){ BLOCK;}                               \
            }                                                           \
        } else {                                                        \
            m_View->CallWithHitElement                                  \
                (action->data().toPoint(),                              \
                 [this](SharedWebElement e){                            \
                    if(e && !e->IsNull()){ BLOCK;}                      \
                });                                                     \
        }                                                               \
    }

void Page::LoadLink(){
    ELEMENT_ACTION(m_View->Load(e->LinkUrl()));
}

void Page::OpenLink(){
    ELEMENT_ACTION(OpenInNew(e->LinkUrl()));
}

void Page::DownloadLink(){
    ELEMENT_ACTION(Download(e->LinkUrl(), e->BaseUrl()));
}

void Page::CopyLinkUrl(){
    ELEMENT_ACTION(Application::clipboard()->setText(e->LinkUrl().toString()));
}

void Page::CopyLinkHtml(){
    ELEMENT_ACTION(Application::clipboard()->setText(e->LinkHtml()));
}

void Page::OpenLinkWithIE(){
    ELEMENT_ACTION(Application::OpenUrlWith_IE(e->LinkUrl()));
}

void Page::OpenLinkWithEdge(){
    ELEMENT_ACTION(Application::OpenUrlWith_Edge(e->LinkUrl()));
}

void Page::OpenLinkWithFF(){
    ELEMENT_ACTION(Application::OpenUrlWith_FF(e->LinkUrl()));
}

void Page::OpenLinkWithOpera(){
    ELEMENT_ACTION(Application::OpenUrlWith_Opera(e->LinkUrl()));
}

void Page::OpenLinkWithOPR(){
    ELEMENT_ACTION(Application::OpenUrlWith_OPR(e->LinkUrl()));
}

void Page::OpenLinkWithSafari(){
    ELEMENT_ACTION(Application::OpenUrlWith_Safari(e->LinkUrl()));
}

void Page::OpenLinkWithChrome(){
    ELEMENT_ACTION(Application::OpenUrlWith_Chrome(e->LinkUrl()));
}

void Page::OpenLinkWithSleipnir(){
    ELEMENT_ACTION(Application::OpenUrlWith_Sleipnir(e->LinkUrl()));
}

void Page::OpenLinkWithVivaldi(){
    ELEMENT_ACTION(Application::OpenUrlWith_Vivaldi(e->LinkUrl()));
}

void Page::OpenLinkWithCustom(){
    ELEMENT_ACTION(Application::OpenUrlWith_Custom(e->LinkUrl()));
}

void Page::LoadImage(){
    ELEMENT_ACTION(m_View->Load(e->ImageUrl()));
}

void Page::OpenImage(){
    ELEMENT_ACTION(OpenInNew(e->ImageUrl()));
}

void Page::DownloadImage(){
    ELEMENT_ACTION(Download(e->ImageUrl(), e->BaseUrl()));
}

void Page::CopyImage(){
    ELEMENT_ACTION(Application::clipboard()->setPixmap(e->Pixmap()));
}

void Page::CopyImageUrl(){
    ELEMENT_ACTION(Application::clipboard()->setText(e->ImageUrl().toString()));
}

void Page::CopyImageHtml(){
    ELEMENT_ACTION(Application::clipboard()->setText(e->ImageHtml()));
}

void Page::OpenImageWithIE(){
    ELEMENT_ACTION(Application::OpenUrlWith_IE(e->ImageUrl()));
}

void Page::OpenImageWithEdge(){
    ELEMENT_ACTION(Application::OpenUrlWith_Edge(e->ImageUrl()));
}

void Page::OpenImageWithFF(){
    ELEMENT_ACTION(Application::OpenUrlWith_FF(e->ImageUrl()));
}

void Page::OpenImageWithOpera(){
    ELEMENT_ACTION(Application::OpenUrlWith_Opera(e->ImageUrl()));
}

void Page::OpenImageWithOPR(){
    ELEMENT_ACTION(Application::OpenUrlWith_OPR(e->ImageUrl()));
}

void Page::OpenImageWithSafari(){
    ELEMENT_ACTION(Application::OpenUrlWith_Safari(e->ImageUrl()));
}

void Page::OpenImageWithChrome(){
    ELEMENT_ACTION(Application::OpenUrlWith_Chrome(e->ImageUrl()));
}

void Page::OpenImageWithSleipnir(){
    ELEMENT_ACTION(Application::OpenUrlWith_Sleipnir(e->ImageUrl()));
}

void Page::OpenImageWithVivaldi(){
    ELEMENT_ACTION(Application::OpenUrlWith_Vivaldi(e->ImageUrl()));
}

void Page::OpenImageWithCustom(){
    ELEMENT_ACTION(Application::OpenUrlWith_Custom(e->ImageUrl()));
}

#undef ELEMENT_ACTION

void Page::OpenInNewHistNode                 (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewHistNode  (reqs, Activate()||ShiftMod(), m_View->GetHistNode());});}
void Page::OpenInNewViewNode                 (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenInNewDirectory                (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenOnRoot                        (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs, Activate()||ShiftMod()                       );});}

void Page::OpenInNewHistNodeForeground       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewHistNode  (reqs,          true         , m_View->GetHistNode());});}
void Page::OpenInNewViewNodeForeground       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenInNewDirectoryForeground      (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenOnRootForeground              (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenInNewHistNodeBackground       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewHistNode  (reqs,             ShiftMod(), m_View->GetHistNode());});}
void Page::OpenInNewViewNodeBackground       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenInNewDirectoryBackground      (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenOnRootBackground              (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,             ShiftMod()                       );});}

void Page::OpenInNewHistNodeThisWindow       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewHistNode  (reqs, Activate()            , m_View->GetHistNode());});}
void Page::OpenInNewViewNodeThisWindow       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewViewNode  (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenInNewDirectoryThisWindow      (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewDirectory (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenOnRootThisWindow              (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenOnSuitableNode (reqs, Activate()                                   );});}

void Page::OpenInNewHistNodeNewWindow        (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewHistNode  (reqs,          true         , m_View->GetHistNode());});}
void Page::OpenInNewViewNodeNewWindow        (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenInNewDirectoryNewWindow       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenOnRootNewWindow               (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenImageInNewHistNode            (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewHistNode  (reqs, Activate()||ShiftMod(), m_View->GetHistNode());});}
void Page::OpenImageInNewViewNode            (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageInNewDirectory           (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageOnRoot                   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs, Activate()||ShiftMod()                       );});}

void Page::OpenImageInNewHistNodeForeground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewHistNode  (reqs,          true         , m_View->GetHistNode());});}
void Page::OpenImageInNewViewNodeForeground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryForeground (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageOnRootForeground         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenImageInNewHistNodeBackground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewHistNode  (reqs,             ShiftMod(), m_View->GetHistNode());});}
void Page::OpenImageInNewViewNodeBackground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryBackground (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageOnRootBackground         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,             ShiftMod()                       );});}

void Page::OpenImageInNewHistNodeThisWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewHistNode  (reqs, Activate()            , m_View->GetHistNode());});}
void Page::OpenImageInNewViewNodeThisWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewViewNode  (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryThisWindow (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewDirectory (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenImageOnRootThisWindow         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenOnSuitableNode (reqs, Activate()                                   );});}

void Page::OpenImageInNewHistNodeNewWindow   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewHistNode  (reqs,          true         , m_View->GetHistNode());});}
void Page::OpenImageInNewViewNodeNewWindow   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryNewWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageOnRootNewWindow          (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenOnSuitableNode (reqs,          true                                );});}

View *Page::OpenInNewViewNode            (QUrl url)                   { return GetTB()->OpenInNewViewNode  (url,                        true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNode            (QUrl url)                   { return GetTB()->OpenInNewHistNode  (url,                        true,  m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectory           (QUrl url)                   { return GetTB()->OpenInNewDirectory (url,                        true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QUrl url)                   { return GetTB()->OpenOnSuitableNode (url,                        true                        ).get();}

View *Page::OpenInNewViewNode            (QList<QUrl> urls)           { return GetTB()->OpenInNewViewNode  (urls,                       true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNode            (QList<QUrl> urls)           { return GetTB()->OpenInNewHistNode  (urls,                       true,  m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectory           (QList<QUrl> urls)           { return GetTB()->OpenInNewDirectory (urls,                       true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QList<QUrl> urls)           { return GetTB()->OpenOnSuitableNode (urls,                       true                        ).get();}

View *Page::OpenInNewViewNode            (QString query)              { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query),      true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNode            (QString query)              { return GetTB()->OpenInNewHistNode  (CreateQueryUrl(query),      true,  m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectory           (QString query)              { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query),      true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QString query)              { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query),      true                        ).get();}

View *Page::OpenInNewViewNode            (QString key, QString query) { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query, key), true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNode            (QString key, QString query) { return GetTB()->OpenInNewHistNode  (CreateQueryUrl(query, key), true,  m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectory           (QString key, QString query) { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query, key), true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QString key, QString query) { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query, key), true                        ).get();}

View *Page::OpenInNewViewNodeBackground  (QUrl url)                   { return GetTB()->OpenInNewViewNode  (url,                        false, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeBackground  (QUrl url)                   { return GetTB()->OpenInNewHistNode  (url,                        false, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryBackground (QUrl url)                   { return GetTB()->OpenInNewDirectory (url,                        false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QUrl url)                   { return GetTB()->OpenOnSuitableNode (url,                        false                       ).get();}

View *Page::OpenInNewViewNodeBackground  (QList<QUrl> urls)           { return GetTB()->OpenInNewViewNode  (urls,                       false, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeBackground  (QList<QUrl> urls)           { return GetTB()->OpenInNewHistNode  (urls,                       false, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryBackground (QList<QUrl> urls)           { return GetTB()->OpenInNewDirectory (urls,                       false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QList<QUrl> urls)           { return GetTB()->OpenOnSuitableNode (urls,                       false                       ).get();}

View *Page::OpenInNewViewNodeBackground  (QString query)              { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query),      false, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeBackground  (QString query)              { return GetTB()->OpenInNewHistNode  (CreateQueryUrl(query),      false, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryBackground (QString query)              { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query),      false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QString query)              { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query),      false                       ).get();}

View *Page::OpenInNewViewNodeBackground  (QString key, QString query) { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query, key), false, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeBackground  (QString key, QString query) { return GetTB()->OpenInNewHistNode  (CreateQueryUrl(query, key), false, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryBackground (QString key, QString query) { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query, key), false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QString key, QString query) { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query, key), false                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QUrl url)                   { return MakeTB()->OpenInNewViewNode  (url,                        true, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeNewWindow   (QUrl url)                   { return MakeTB()->OpenInNewHistNode  (url,                        true, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QUrl url)                   { return MakeTB()->OpenInNewDirectory (url,                        true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QUrl url)                   { return MakeTB()->OpenOnSuitableNode (url,                        true                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QList<QUrl> urls)           { return MakeTB()->OpenInNewViewNode  (urls,                       true, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeNewWindow   (QList<QUrl> urls)           { return MakeTB()->OpenInNewHistNode  (urls,                       true, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QList<QUrl> urls)           { return MakeTB()->OpenInNewDirectory (urls,                       true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QList<QUrl> urls)           { return MakeTB()->OpenOnSuitableNode (urls,                       true                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QString query)              { return MakeTB()->OpenInNewViewNode  (CreateQueryUrl(query),      true, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeNewWindow   (QString query)              { return MakeTB()->OpenInNewHistNode  (CreateQueryUrl(query),      true, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QString query)              { return MakeTB()->OpenInNewDirectory (CreateQueryUrl(query),      true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QString query)              { return MakeTB()->OpenOnSuitableNode (CreateQueryUrl(query),      true                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QString key, QString query) { return MakeTB()->OpenInNewViewNode  (CreateQueryUrl(query, key), true, m_View->GetViewNode()).get();}
View *Page::OpenInNewHistNodeNewWindow   (QString key, QString query) { return MakeTB()->OpenInNewHistNode  (CreateQueryUrl(query, key), true, m_View->GetHistNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QString key, QString query) { return MakeTB()->OpenInNewDirectory (CreateQueryUrl(query, key), true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QString key, QString query) { return MakeTB()->OpenOnSuitableNode (CreateQueryUrl(query, key), true                       ).get();}

void Page::OpenAllUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedHtml([this, base](QString html){

    QList<QUrl> urls = ExtractUrlFromHtml(html, base, HaveReference);

    UrlCountCheck(urls.length(), [this, urls](bool result){

    if(result){
        QList<QNetworkRequest> reqs;
        foreach(QUrl url, urls){
            QNetworkRequest req(url);
            req.setRawHeader("Referer", m_View->url().toEncoded());
            reqs << req;
        }
        SuitTB()->OpenInNewViewNode(reqs, Activate()||ShiftMod(), m_View->GetViewNode());
    }
});});});
}

void Page::OpenAllImage(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedHtml([this, base](QString html){

    QList<QUrl> urls = ExtractUrlFromHtml(html, base, HaveSource);

    UrlCountCheck(urls.length(), [this, urls](bool result){

    if(result){
        QList<QNetworkRequest> reqs;
        foreach(QUrl url, urls){
            QNetworkRequest req(url);
            req.setRawHeader("Referer", m_View->url().toEncoded());
            reqs << req;
        }
        SuitTB()->OpenInNewViewNode(reqs, Activate()||ShiftMod(), m_View->GetViewNode());
    }
});});});
}

void Page::OpenTextAsUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedText([this, base](QString text){

    QList<QUrl> urls = ExtractUrlFromText(text, base);

    UrlCountCheck(urls.length(), [this, urls](bool result){

    if(result){
        QList<QNetworkRequest> reqs;
        foreach(QUrl url, urls){
            QNetworkRequest req(url); // no referer.
            reqs << req;
        }
        SuitTB()->OpenInNewViewNode(reqs, Activate()||ShiftMod(), m_View->GetViewNode());
    }
});});});
}

void Page::SaveAllUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedHtml([this, base](QString html){

    QList<QUrl> urls = ExtractUrlFromHtml(html, base, HaveReference);

    if(urls.isEmpty()) return;
    QString directory =
        ModalDialog::GetExistingDirectory(QString(), Application::GetDownloadDirectory());
    if(directory.isEmpty()) return;
    if(Application::GetDownloadPolicy() == Application::Undefined_ ||
       Application::GetDownloadPolicy() == Application::AskForEachDownload)
        Application::SetDownloadDirectory(directory + QStringLiteral("/"));
    foreach(QUrl url, urls){
        QNetworkRequest req(url);
        req.setRawHeader("Referer", m_View->url().toEncoded());
        Download(req, directory + QStringLiteral("/") + url.toString().split(QStringLiteral("/")).last());
    }
});});
}

void Page::SaveAllImage(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedHtml([this, base](QString html){

    QList<QUrl> urls = ExtractUrlFromHtml(html, base, HaveSource);

    if(urls.isEmpty()) return;
    QString directory =
        ModalDialog::GetExistingDirectory(QString(), Application::GetDownloadDirectory());
    if(directory.isEmpty()) return;
    if(Application::GetDownloadPolicy() == Application::Undefined_ ||
       Application::GetDownloadPolicy() == Application::AskForEachDownload)
        Application::SetDownloadDirectory(directory + QStringLiteral("/"));
    foreach(QUrl url, urls){
        QNetworkRequest req(url);
        req.setRawHeader("Referer", m_View->url().toEncoded());
        Download(req, directory + QStringLiteral("/") + url.toString().split(QStringLiteral("/")).last());
    }
});});
}

void Page::SaveTextAsUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedText([this, base](QString text){

    QList<QUrl> urls = ExtractUrlFromText(text, base);

    if(urls.isEmpty()) return;
    QString directory =
        ModalDialog::GetExistingDirectory(QString(), Application::GetDownloadDirectory());
    if(directory.isEmpty()) return;
    if(Application::GetDownloadPolicy() == Application::Undefined_ ||
       Application::GetDownloadPolicy() == Application::AskForEachDownload)
        Application::SetDownloadDirectory(directory + QStringLiteral("/"));
    foreach(QUrl url, urls){
        QNetworkRequest req(url); // no referer.
        Download(req, directory + QStringLiteral("/") + url.toString().split(QStringLiteral("/")).last());
    }
});});
}

QAction *Page::Action(CustomAction a, QVariant data){
    // forbid many times call of same action.
    static const QList<CustomAction> exclude = QList<CustomAction>()
        << _NoAction << _End      << _Undo
        << _Up       << _PageUp   << _Redo
        << _Down     << _PageDown << _SelectAll
        << _Right    << _Cut      << _SwitchWindow
        << _Left     << _Copy     << _NextWindow
        << _Home     << _Paste    << _PrevWindow;
    static CustomAction previousAction = _NoAction;
    static int sameActionCount = 0;
    if(exclude.contains(a)){
        sameActionCount = 0;
        previousAction = _NoAction;
    } else if(a == previousAction){
        if(++sameActionCount > MAX_SAME_ACTION_COUNT)
            a = _NoAction;
    } else {
        sameActionCount = 0;
        previousAction = a;
    }

    QAction *webaction = m_ActionTable[a];
    bool create = false;
    if(!webaction){
        create = true;
        m_ActionTable[a] = webaction = new QAction(this);
    }

    webaction->setData(data);
    if(!create){
        switch(a){
        case _ToggleNotifier:
            webaction->setChecked(GetTB()->GetNotifier());
            break;
        case _ToggleReceiver:
            webaction->setChecked(GetTB()->GetReceiver());
            break;
        case _ToggleMenuBar:
            webaction->setChecked(!GetTB()->GetMainWindow()->IsMenuBarEmpty());
            break;
        case _ToggleTreeBar:
            webaction->setChecked(GetTB()->GetMainWindow()->GetTreeBar()->isVisible());
            break;
        case _ToggleToolBar:
            webaction->setChecked(GetTB()->GetMainWindow()->GetToolBar()->isVisible());
            break;
        }
        return webaction;
    }

    switch(a){
    case _Up:      webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case _Down:    webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case _Right:   webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case _Left:    webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
    case _Back:    webaction->setIcon(QIcon(":/resources/menu/back.png"));    break;
    case _Forward: webaction->setIcon(QIcon(":/resources/menu/forward.png")); break;
    case _Reload:  webaction->setIcon(QIcon(":/resources/menu/reload.png"));  break;
    case _Stop:    webaction->setIcon(QIcon(":/resources/menu/stop.png"));    break;
    }

    switch(a){
    case _NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case _##name:                                                   \
            webaction->setText(text);                                   \
            webaction->setToolTip(text);                                \
            connect(webaction, SIGNAL(triggered()),                     \
                    this,      SLOT(name##Key()));                      \
            break;

        // key events.
        DEFINE_ACTION(Up,       tr("UpKey"));
        DEFINE_ACTION(Down,     tr("DownKey"));
        DEFINE_ACTION(Right,    tr("RightKey"));
        DEFINE_ACTION(Left,     tr("LeftKey"));
        DEFINE_ACTION(Home,     tr("HomeKey"));
        DEFINE_ACTION(End,      tr("EndKey"));
        DEFINE_ACTION(PageUp,   tr("PageUpKey"));
        DEFINE_ACTION(PageDown, tr("PageDownKey"));

#undef  DEFINE_ACTION
#define DEFINE_ACTION(name, text)                                       \
        case _##name:                                                   \
            webaction->setText(text);                                   \
            webaction->setToolTip(text);                                \
            connect(webaction, SIGNAL(triggered()),                     \
                    this,      SLOT(name()));                           \
            break;

        // application events.
        DEFINE_ACTION(Import,       tr("Import"));
        DEFINE_ACTION(Export,       tr("Export"));
        DEFINE_ACTION(AboutVanilla, tr("AboutVanilla"));
        DEFINE_ACTION(AboutQt,      tr("AboutQt"));
        DEFINE_ACTION(Quit,         tr("Quit"));

        // window events.
        DEFINE_ACTION(ToggleNotifier,        tr("ToggleNotifier"));
        DEFINE_ACTION(ToggleReceiver,        tr("ToggleReceiver"));
        DEFINE_ACTION(ToggleMenuBar,         tr("ToggleMenuBar"));
        DEFINE_ACTION(ToggleTreeBar,         tr("ToggleTreeBar"));
        DEFINE_ACTION(ToggleToolBar,         tr("ToggleToolBar"));
        DEFINE_ACTION(ToggleFullScreen,      tr("ToggleFullScreen"));
        DEFINE_ACTION(ToggleMaximized,       tr("ToggleMaximized"));
        DEFINE_ACTION(ToggleMinimized,       tr("ToggleMinimized"));
        DEFINE_ACTION(ToggleShaded,          tr("ToggleShaded"));
        DEFINE_ACTION(ShadeWindow,           tr("ShadeWindow"));
        DEFINE_ACTION(UnshadeWindow,         tr("UnshadeWindow"));
        DEFINE_ACTION(NewWindow,             tr("NewWindow"));
        DEFINE_ACTION(CloseWindow,           tr("CloseWindow"));
        DEFINE_ACTION(SwitchWindow,          tr("SwitchWindow"));
        DEFINE_ACTION(NextWindow,            tr("NextWindow"));
        DEFINE_ACTION(PrevWindow,            tr("PrevWindow"));

        // treebank events.
        DEFINE_ACTION(Back,                  tr("Back"));
        DEFINE_ACTION(Forward,               tr("Forward"));
        DEFINE_ACTION(UpDirectory,           tr("UpDirectory"));
        DEFINE_ACTION(Close,                 tr("Close"));
        DEFINE_ACTION(Restore,               tr("Restore"));
        DEFINE_ACTION(Recreate,              tr("Recreate"));
        DEFINE_ACTION(NextView,              tr("NextView"));
        DEFINE_ACTION(PrevView,              tr("PrevView"));
        DEFINE_ACTION(BuryView,              tr("BuryView"));
        DEFINE_ACTION(DigView,               tr("DigView"));
        DEFINE_ACTION(FirstView,             tr("FirstView"));
        DEFINE_ACTION(SecondView,            tr("SecondView"));
        DEFINE_ACTION(ThirdView,             tr("ThirdView"));
        DEFINE_ACTION(FourthView,            tr("FourthView"));
        DEFINE_ACTION(FifthView,             tr("FifthView"));
        DEFINE_ACTION(SixthView,             tr("SixthView"));
        DEFINE_ACTION(SeventhView,           tr("SeventhView"));
        DEFINE_ACTION(EighthView,            tr("EighthView"));
        DEFINE_ACTION(NinthView,             tr("NinthView"));
        DEFINE_ACTION(TenthView,             tr("TenthView"));
        DEFINE_ACTION(NewViewNode,           tr("NewViewNode"));
        DEFINE_ACTION(NewHistNode,           tr("NewHistNode"));
        DEFINE_ACTION(CloneViewNode,         tr("CloneViewNode"));
        DEFINE_ACTION(CloneHistNode,         tr("CloneHistNode"));

        DEFINE_ACTION(DisplayAccessKey,      tr("DisplayAccessKey"));
        DEFINE_ACTION(DisplayViewTree,       tr("DisplayViewTree"));
        DEFINE_ACTION(DisplayHistTree,       tr("DisplayHistTree"));
        DEFINE_ACTION(DisplayTrashTree,      tr("DisplayTrashTree"));

        DEFINE_ACTION(OpenTextSeeker,        tr("OpenTextSeeker"));
        DEFINE_ACTION(OpenQueryEditor,       tr("OpenQueryEditor"));
        DEFINE_ACTION(OpenUrlEditor,         tr("OpenUrlEditor"));
        DEFINE_ACTION(OpenCommand,           tr("OpenCommand"));

        DEFINE_ACTION(ReleaseHiddenView,     tr("ReleaseHiddenView"));

        DEFINE_ACTION(Load,                  tr("Load"));

        // web events.
        DEFINE_ACTION(Copy,                  tr("Copy"));
        DEFINE_ACTION(Cut,                   tr("Cut"));
        DEFINE_ACTION(Paste,                 tr("Paste"));
        DEFINE_ACTION(Undo,                  tr("Undo"));
        DEFINE_ACTION(Redo,                  tr("Redo"));
        DEFINE_ACTION(SelectAll,             tr("SelectAll"));
        DEFINE_ACTION(Unselect,              tr("Unselect"));
        DEFINE_ACTION(Reload,                tr("Reload"));
        DEFINE_ACTION(ReloadAndBypassCache,  tr("ReloadAndBypassCache"));
        DEFINE_ACTION(Stop,                  tr("Stop"));
        DEFINE_ACTION(StopAndUnselect,       tr("StopAndUnselect"));

        DEFINE_ACTION(Print,                 tr("Print"));
        DEFINE_ACTION(Save,                  tr("Save"));
        DEFINE_ACTION(ZoomIn,                tr("ZoomIn"));
        DEFINE_ACTION(ZoomOut,               tr("ZoomOut"));
        DEFINE_ACTION(ViewSource,            tr("ViewSource"));
        DEFINE_ACTION(ApplySource,           tr("ApplySource"));

        DEFINE_ACTION(OpenBookmarklet,       tr("OpenBookmarklet"));
        DEFINE_ACTION(SearchWith,            tr("SearchWith"));
        DEFINE_ACTION(AddSearchEngine,       tr("AddSearchEngine"));
        DEFINE_ACTION(AddBookmarklet,        tr("AddBookmarklet"));
        DEFINE_ACTION(InspectElement,        tr("InspectElement"));

        DEFINE_ACTION(CopyUrl,               tr("CopyUrl"));
        DEFINE_ACTION(CopyTitle,             tr("CopyTitle"));
        DEFINE_ACTION(CopyPageAsLink,        tr("CopyPageAsLink"));
        DEFINE_ACTION(CopySelectedHtml,      tr("CopySelectedHtml"));
        DEFINE_ACTION(OpenWithIE,            tr("OpenWithIE"));
        DEFINE_ACTION(OpenWithEdge,          tr("OpenWithEdge"));
        DEFINE_ACTION(OpenWithFF,            tr("OpenWithFF"));
        DEFINE_ACTION(OpenWithOpera,         tr("OpenWithOpera"));
        DEFINE_ACTION(OpenWithOPR,           tr("OpenWithOPR"));
        DEFINE_ACTION(OpenWithSafari,        tr("OpenWithSafari"));
        DEFINE_ACTION(OpenWithChrome,        tr("OpenWithChrome"));
        DEFINE_ACTION(OpenWithSleipnir,      tr("OpenWithSleipnir"));
        DEFINE_ACTION(OpenWithVivaldi,       tr("OpenWithVivaldi"));
        DEFINE_ACTION(OpenWithCustom,        tr("OpenWithCustom"));

        // element events.
        DEFINE_ACTION(ClickElement,          tr("ClickElement"));
        DEFINE_ACTION(FocusElement,          tr("FocusElement"));
        DEFINE_ACTION(HoverElement,          tr("HoverElement"));

        DEFINE_ACTION(LoadLink,              tr("LoadLink"));
        DEFINE_ACTION(OpenLink,              tr("OpenLink"));
        DEFINE_ACTION(DownloadLink,          tr("DownloadLink"));
        DEFINE_ACTION(CopyLinkUrl,           tr("CopyLinkUrl"));
        DEFINE_ACTION(CopyLinkHtml,          tr("CopyLinkHtml"));
        DEFINE_ACTION(OpenLinkWithIE,        tr("OpenLinkWithIE"));
        DEFINE_ACTION(OpenLinkWithEdge,      tr("OpenLinkWithEdge"));
        DEFINE_ACTION(OpenLinkWithFF,        tr("OpenLinkWithFF"));
        DEFINE_ACTION(OpenLinkWithOpera,     tr("OpenLinkWithOpera"));
        DEFINE_ACTION(OpenLinkWithOPR,       tr("OpenLinkWithOPR"));
        DEFINE_ACTION(OpenLinkWithSafari,    tr("OpenLinkWithSafari"));
        DEFINE_ACTION(OpenLinkWithChrome,    tr("OpenLinkWithChrome"));
        DEFINE_ACTION(OpenLinkWithSleipnir,  tr("OpenLinkWithSleipnir"));
        DEFINE_ACTION(OpenLinkWithVivaldi,   tr("OpenLinkWithVivaldi"));
        DEFINE_ACTION(OpenLinkWithCustom,    tr("OpenLinkWithCustom"));

        DEFINE_ACTION(LoadImage,             tr("LoadImage"));
        DEFINE_ACTION(OpenImage,             tr("OpenImage"));
        DEFINE_ACTION(DownloadImage,         tr("DownloadImage"));
        DEFINE_ACTION(CopyImage,             tr("CopyImage"));
        DEFINE_ACTION(CopyImageUrl,          tr("CopyImageUrl"));
        DEFINE_ACTION(CopyImageHtml,         tr("CopyImageHtml"));
        DEFINE_ACTION(OpenImageWithIE,       tr("OpenImageWithIE"));
        DEFINE_ACTION(OpenImageWithEdge,     tr("OpenImageWithEdge"));
        DEFINE_ACTION(OpenImageWithFF,       tr("OpenImageWithFF"));
        DEFINE_ACTION(OpenImageWithOpera,    tr("OpenImageWithOpera"));
        DEFINE_ACTION(OpenImageWithOPR,      tr("OpenImageWithOPR"));
        DEFINE_ACTION(OpenImageWithSafari,   tr("OpenImageWithSafari"));
        DEFINE_ACTION(OpenImageWithChrome,   tr("OpenImageWithChrome"));
        DEFINE_ACTION(OpenImageWithSleipnir, tr("OpenImageWithSleipnir"));
        DEFINE_ACTION(OpenImageWithVivaldi,  tr("OpenImageWithVivaldi"));
        DEFINE_ACTION(OpenImageWithCustom,   tr("OpenImageWithCustom"));

        // link opner(follow modifier).
        DEFINE_ACTION(OpenInNewViewNode,                 tr("OpenInNewViewNode"));
        DEFINE_ACTION(OpenInNewHistNode,                 tr("OpenInNewHistNode"));
        DEFINE_ACTION(OpenInNewDirectory,                tr("OpenInNewDirectory"));
        DEFINE_ACTION(OpenOnRoot,                        tr("OpenOnRoot"));
        // link opner(foreground).
        DEFINE_ACTION(OpenInNewViewNodeForeground,       tr("OpenInNewViewNodeForeground"));
        DEFINE_ACTION(OpenInNewHistNodeForeground,       tr("OpenInNewHistNodeForeground"));
        DEFINE_ACTION(OpenInNewDirectoryForeground,      tr("OpenInNewDirectoryForeground"));
        DEFINE_ACTION(OpenOnRootForeground,              tr("OpenOnRootForeground"));
        // link opner(background).
        DEFINE_ACTION(OpenInNewViewNodeBackground,       tr("OpenInNewViewNodeBackground"));
        DEFINE_ACTION(OpenInNewHistNodeBackground,       tr("OpenInNewHistNodeBackground"));
        DEFINE_ACTION(OpenInNewDirectoryBackground,      tr("OpenInNewDirectoryBackground"));
        DEFINE_ACTION(OpenOnRootBackground,              tr("OpenOnRootBackground"));
        // link opner(same window).
        DEFINE_ACTION(OpenInNewViewNodeThisWindow,       tr("OpenInNewViewNodeThisWindow"));
        DEFINE_ACTION(OpenInNewHistNodeThisWindow,       tr("OpenInNewHistNodeThisWindow"));
        DEFINE_ACTION(OpenInNewDirectoryThisWindow,      tr("OpenInNewDirectoryThisWindow"));
        DEFINE_ACTION(OpenOnRootThisWindow,              tr("OpenOnRootThisWindow"));
        // link opner(new window).
        DEFINE_ACTION(OpenInNewViewNodeNewWindow,        tr("OpenInNewViewNodeNewWindow"));
        DEFINE_ACTION(OpenInNewHistNodeNewWindow,        tr("OpenInNewHistNodeNewWindow"));
        DEFINE_ACTION(OpenInNewDirectoryNewWindow,       tr("OpenInNewDirectoryNewWindow"));
        DEFINE_ACTION(OpenOnRootNewWindow,               tr("OpenOnRootNewWindow"));

        // image opner(follow modifier).
        DEFINE_ACTION(OpenImageInNewViewNode,            tr("OpenImageInNewViewNode"));
        DEFINE_ACTION(OpenImageInNewHistNode,            tr("OpenImageInNewHistNode"));
        DEFINE_ACTION(OpenImageInNewDirectory,           tr("OpenImageInNewDirectory"));
        DEFINE_ACTION(OpenImageOnRoot,                   tr("OpenImageOnRoot"));
        // image opner(foreground).
        DEFINE_ACTION(OpenImageInNewViewNodeForeground,  tr("OpenImageInNewViewNodeForeground"));
        DEFINE_ACTION(OpenImageInNewHistNodeForeground,  tr("OpenImageInNewHistNodeForeground"));
        DEFINE_ACTION(OpenImageInNewDirectoryForeground, tr("OpenImageInNewDirectoryForeground"));
        DEFINE_ACTION(OpenImageOnRootForeground,         tr("OpenImageOnRootForeground"));
        // image opner(background).
        DEFINE_ACTION(OpenImageInNewViewNodeBackground,  tr("OpenImageInNewViewNodeBackground"));
        DEFINE_ACTION(OpenImageInNewHistNodeBackground,  tr("OpenImageInNewHistNodeBackground"));
        DEFINE_ACTION(OpenImageInNewDirectoryBackground, tr("OpenImageInNewDirectoryBackground"));
        DEFINE_ACTION(OpenImageOnRootBackground,         tr("OpenImageOnRootBackground"));
        // image opner(same window).
        DEFINE_ACTION(OpenImageInNewViewNodeThisWindow,  tr("OpenImageInNewViewNodeThisWindow"));
        DEFINE_ACTION(OpenImageInNewHistNodeThisWindow,  tr("OpenImageInNewHistNodeThisWindow"));
        DEFINE_ACTION(OpenImageInNewDirectoryThisWindow, tr("OpenImageInNewDirectoryThisWindow"));
        DEFINE_ACTION(OpenImageOnRootThisWindow,         tr("OpenImageOnRootThisWindow"));
        // image opner(new window).
        DEFINE_ACTION(OpenImageInNewViewNodeNewWindow,   tr("OpenImageInNewViewNodeNewWindow"));
        DEFINE_ACTION(OpenImageInNewHistNodeNewWindow,   tr("OpenImageInNewHistNodeNewWindow"));
        DEFINE_ACTION(OpenImageInNewDirectoryNewWindow,  tr("OpenImageInNewDirectoryNewWindow"));
        DEFINE_ACTION(OpenImageOnRootNewWindow,          tr("OpenImageOnRootNewWindow"));

        // auto focus (solid link) opner.
        DEFINE_ACTION(OpenAllUrl,    tr("OpenAllUrl"));
        DEFINE_ACTION(OpenAllImage,  tr("OpenAllImage"));
        DEFINE_ACTION(OpenTextAsUrl, tr("OpenTextAsUrl"));
        DEFINE_ACTION(SaveAllUrl,    tr("SaveAllUrl"));
        DEFINE_ACTION(SaveAllImage,  tr("SaveAllImage"));
        DEFINE_ACTION(SaveTextAsUrl, tr("SaveTextAsUrl"));

#undef  DEFINE_ACTION
    }
    switch(a){

    case _ToggleNotifier:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetNotifier());
        webaction->setText(tr("Notifier"));
        webaction->setToolTip(tr("Notifier"));
        break;
    case _ToggleReceiver:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetReceiver());
        webaction->setText(tr("Receiver"));
        webaction->setToolTip(tr("Receiver"));
        break;
    case _ToggleMenuBar:
        webaction->setCheckable(true);
        webaction->setChecked(!GetTB()->GetMainWindow()->IsMenuBarEmpty());
        webaction->setText(tr("MenuBar"));
        webaction->setToolTip(tr("MenuBar"));
        break;
    case _ToggleTreeBar:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetMainWindow()->GetTreeBar()->isVisible());
        webaction->setText(tr("TreeBar"));
        webaction->setToolTip(tr("TreeBar"));
        break;
    case _ToggleToolBar:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetMainWindow()->GetToolBar()->isVisible());
        webaction->setText(tr("ToolBar"));
        webaction->setToolTip(tr("ToolBar"));
        break;

    case _OpenWithIE:
    case _OpenLinkWithIE:
    case _OpenImageWithIE:
        webaction->setIcon(Application::BrowserIcon_IE());
        break;
    case _OpenWithEdge:
    case _OpenLinkWithEdge:
    case _OpenImageWithEdge:
        webaction->setIcon(Application::BrowserIcon_Edge());
        break;
    case _OpenWithFF:
    case _OpenLinkWithFF:
    case _OpenImageWithFF:
        webaction->setIcon(Application::BrowserIcon_FF());
        break;
    case _OpenWithOpera:
    case _OpenLinkWithOpera:
    case _OpenImageWithOpera:
        webaction->setIcon(Application::BrowserIcon_Opera());
        break;
    case _OpenWithOPR:
    case _OpenLinkWithOPR:
    case _OpenImageWithOPR:
        webaction->setIcon(Application::BrowserIcon_OPR());
        break;
    case _OpenWithSafari:
    case _OpenLinkWithSafari:
    case _OpenImageWithSafari:
        webaction->setIcon(Application::BrowserIcon_Safari());
        break;
    case _OpenWithChrome:
    case _OpenLinkWithChrome:
    case _OpenImageWithChrome:
        webaction->setIcon(Application::BrowserIcon_Chrome());
        break;
    case _OpenWithSleipnir:
    case _OpenLinkWithSleipnir:
    case _OpenImageWithSleipnir:
        webaction->setIcon(Application::BrowserIcon_Sleipnir());
        break;
    case _OpenWithVivaldi:
    case _OpenLinkWithVivaldi:
    case _OpenImageWithVivaldi:
        webaction->setIcon(Application::BrowserIcon_Vivaldi());
        break;
    case _OpenWithCustom:
        webaction->setIcon(Application::BrowserIcon_Custom());
        webaction->setText(tr("OpenWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
    case _OpenLinkWithCustom:
        webaction->setIcon(Application::BrowserIcon_Custom());
        webaction->setText(tr("OpenLinkWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
    case _OpenImageWithCustom:
        webaction->setIcon(Application::BrowserIcon_Custom());
        webaction->setText(tr("OpenImageWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
    case _NewViewNode:
    case _NewHistNode:
    case _CloneViewNode:
    case _CloneHistNode:
    case _OpenInNewHistNode:
    case _OpenInNewViewNode:
    case _OpenInNewDirectory:
    case _OpenOnRoot:
    case _OpenImageInNewHistNode:
    case _OpenImageInNewViewNode:
    case _OpenImageInNewDirectory:
    case _OpenImageOnRoot:
    case _ViewSource:
    case _OpenAllUrl:
    case _OpenAllImage:
    case _OpenTextAsUrl:
        webaction->setToolTip(webaction->toolTip() +
                              (View::ActivateNewViewDefault()
                               ? tr("\n Shift+Click: InNewWindow"
                                    "\n Ctrl +Click: InBackground")
                               : tr("\n Shift+Click: InNewWindow"
                                    "\n Ctrl +Click: InForeground")));
        break;
    case _OpenInNewHistNodeForeground:
    case _OpenInNewViewNodeForeground:
    case _OpenInNewDirectoryForeground:
    case _OpenOnRootForeground:
    case _OpenInNewHistNodeBackground:
    case _OpenInNewViewNodeBackground:
    case _OpenInNewDirectoryBackground:
    case _OpenOnRootBackground:
    case _OpenImageInNewHistNodeForeground:
    case _OpenImageInNewViewNodeForeground:
    case _OpenImageInNewDirectoryForeground:
    case _OpenImageOnRootForeground:
    case _OpenImageInNewHistNodeBackground:
    case _OpenImageInNewViewNodeBackground:
    case _OpenImageInNewDirectoryBackground:
    case _OpenImageOnRootBackground:
        webaction->setToolTip(webaction->toolTip() +
                              tr("\n Shift+Click: InNewWindow"));
        break;
    case _OpenInNewHistNodeThisWindow:
    case _OpenInNewViewNodeThisWindow:
    case _OpenInNewDirectoryThisWindow:
    case _OpenOnRootThisWindow:
    case _OpenImageInNewHistNodeThisWindow:
    case _OpenImageInNewViewNodeThisWindow:
    case _OpenImageInNewDirectoryThisWindow:
    case _OpenImageOnRootThisWindow:
        webaction->setToolTip(webaction->toolTip() +
                              (View::ActivateNewViewDefault()
                               ? tr("\n Ctrl+Click: InBackground")
                               : tr("\n Ctrl+Click: InForeground")));
        break;
    }
    return webaction;
}

void Page::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    DownloadItem *item =
        NetworkController::Download(GetNetworkAccessManager(),
                                    req, NetworkController::ToVariable);
    connect(item, SIGNAL(DownloadResult(const QByteArray&)),
            this, SIGNAL(SuggestResult(const QByteArray&)));
}

TreeBank *Page::GetTB(){
    return m_View->GetTreeBank();
}

TreeBank *Page::MakeTB(){
    return NewWindow()->GetTreeBank();
}

TreeBank *Page::SuitTB(){
    return ShiftMod() ? MakeTB() : GetTB();
}

void Page::LinkReq(QAction *action,
                   std::function<void(QList<QNetworkRequest>)> callBack){

    // order of priority :
    //
    // link at cursor or element
    //   > exacted by css "*[href]" from selected html
    //     > exacted from selected text
    //       > query of selected text
    //         > image

    if(action->data().canConvert<SharedWebElement>()){
        if(SharedWebElement e = action->data().value<SharedWebElement>()){
            if(!e->IsNull() && !e->LinkUrl().isEmpty()){
                QList<QNetworkRequest> reqs;
                QNetworkRequest req(e->LinkUrl());
                req.setRawHeader("Referer", m_View->url().toEncoded());
                reqs << req;
                callBack(reqs);
                return;
            }
        }
    }

    m_View->CallWithHitLinkUrl(action ? action->data().toPoint() : QPoint(),
                               [this, action, callBack](QUrl url){

    if(action && !url.isEmpty()){
        QList<QNetworkRequest> reqs;
        QNetworkRequest req(url);
        req.setRawHeader("Referer", m_View->url().toEncoded());
        reqs << req;
        callBack(reqs);
        return;
    }

    m_View->CallWithGotCurrentBaseUrl([this, action, callBack](QUrl base){
    m_View->CallWithSelectedHtml([this, action, callBack, base](QString html){

    if(!html.isEmpty()){
        QList<QNetworkRequest> reqs;
        QList<QUrl> urls = ExtractUrlFromHtml(html, base, HaveReference);

        if(!urls.isEmpty()){
            foreach(QUrl u, urls){
                QNetworkRequest req = QNetworkRequest(u);
                req.setRawHeader("Referer", m_View->url().toEncoded());
                reqs << req;
            }

            UrlCountCheck(reqs.length(), [callBack, reqs](bool result){
                    if(result) callBack(reqs);
                    else callBack(QList<QNetworkRequest>());
                });
            return;
        }
    }

    m_View->CallWithSelectedText([this, action, callBack, base](QString text){

    if(!text.isEmpty()){
        QList<QNetworkRequest> reqs;
        QList<QUrl> urls = ExtractUrlFromText(text, base);

        if(!urls.isEmpty()){
            foreach(QUrl u, urls){
                reqs << QNetworkRequest(u);
            }
            UrlCountCheck(reqs.length(), [callBack, reqs](bool result){
                    if(result) callBack(reqs);
                    else callBack(QList<QNetworkRequest>());
                });
            return;
        }
        reqs << QNetworkRequest(Page::CreateQueryUrl(text));
        callBack(reqs);
        return;
    }

    ImageReq(action, callBack);

});});});});
}

void Page::ImageReq(QAction *action,
                    std::function<void(QList<QNetworkRequest>)> callBack){

    // order of priority :
    //
    // image at cursor or element
    //   > exacted by css "*[src]" from selected html

    if(action && action->data().canConvert<SharedWebElement>()){
        if(SharedWebElement e = action->data().value<SharedWebElement>()){
            if(!e->IsNull() && !e->ImageUrl().isEmpty()){
                QList<QNetworkRequest> reqs;
                QNetworkRequest req(e->ImageUrl());
                req.setRawHeader("Referer", m_View->url().toEncoded());
                reqs << req;
                callBack(reqs);
                return;
            }
        }
    }

    m_View->CallWithHitImageUrl(action ? action->data().toPoint() : QPoint(),
                                [this, action, callBack](QUrl url){

    if(action && !url.isEmpty()){
        QList<QNetworkRequest> reqs;
        QNetworkRequest req(url);
        req.setRawHeader("Referer", m_View->url().toEncoded());
        reqs << req;
        callBack(reqs);
        return;
    }

    m_View->CallWithGotCurrentBaseUrl([this, action, callBack](QUrl base){
    m_View->CallWithSelectedHtml([this, action, callBack, base](QString html){

    if(!html.isEmpty()){
        QList<QNetworkRequest> reqs;
        QList<QUrl> urls = ExtractUrlFromHtml(html, base, HaveSource);

        if(!urls.isEmpty()){
            foreach(QUrl u, urls){
                QNetworkRequest req = QNetworkRequest(u);
                req.setRawHeader("Referer", m_View->url().toEncoded());
                reqs << req;
            }
            UrlCountCheck(reqs.length(), [callBack, reqs](bool result){
                    if(result) callBack(reqs);
                    else callBack(QList<QNetworkRequest>());
                });
            return;
        }
    }

    callBack(QList<QNetworkRequest>());

});});});
}

void Page::UrlCountCheck(int count, BoolCallBack callBack){
    if(count > OPEN_LINK_WARNING_THRESHOLD){
        ModelessDialog::Question
            (tr("Too many links or images."), tr("Open anyway?"), callBack, this);
    } else {
        callBack(true);
    }
}
