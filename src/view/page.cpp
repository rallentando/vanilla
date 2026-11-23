#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "page.hpp"
#include "actiontext.hpp"

#include <QClipboard>
#include <QList>
#include <QSet>
#include <QNetworkRequest>
#include <QStyle>
#include <QTimer>

#include "view.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "webenginepage.hpp"
#include "edgewebview.hpp"
#include "treebank.hpp"
#include "menukeys.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "networkcontroller.hpp"
#include "application.hpp"
#include "dialog.hpp"
#include "directorypage.hpp"

#undef LoadImage

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
}

Page::~Page(){}

NetworkAccessManager *Page::GetNetworkAccessManager(){
    NetworkAccessManager *nam = 0;
#ifdef WEBENGINEVIEW
    if(WebEnginePage *page = qobject_cast<WebEnginePage*>(m_View->page()))
        nam = qobject_cast<NetworkAccessManager*>(page->networkAccessManager());
#endif
    if(!nam) nam = m_NetworkAccessManager;

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
    return CreateQueryUrl(engine, query);
}

QUrl Page::CreateQueryUrl(const SearchEngine &engine, QString query){
    if(engine.isEmpty()) return QUrl();
    const QString format = engine.at(0);
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
           str.startsWith(QStringLiteral("edge:"))         ||
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
           str.startsWith(VANILLA_SCHEME + QStringLiteral(":")) ||
           str.startsWith(QStringLiteral(":/"))){
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

QList<QUrl> Page::ExtractUrlsFromHtml(QString html, QUrl baseUrl, FindElementsOption option){
    QList<QUrl> list;
    int pos = 0;
    QRegularExpression reg;
    if(option == HaveSource)
        reg = QRegularExpression(QStringLiteral("src=(\"?)([^<>\\{\\}\\(\\)\"\\`\\'\\^\\|\n\r\t \\\\]+)\\1"));
    if(option == HaveReference)
        reg = QRegularExpression(QStringLiteral("href=(\"?)([^<>\\{\\}\\(\\)\"\\`\\'\\^\\|\n\r\t \\\\]+)\\1"));
    if(reg.isValid()){
        QRegularExpressionMatch match;
        while((match = reg.match(html, pos)).hasMatch()){
            list << StringToUrl(match.captured(2), baseUrl);
            pos = match.capturedEnd();
        }
    }
    return QSet<QUrl>(list.begin(), list.end()).values();
}

QList<QUrl> Page::ExtractUrlsFromText(QString text, QUrl baseUrl){
    QList<QUrl> list;
    QStringList strs = text.split(QRegularExpression(QStringLiteral("[<>\\{\\}\\(\\)\"\\`\\'\\^\\|\n\r\t \\\\]+")));
    for(QString str : std::as_const(strs)){
        str = str.trimmed();
        if(str.startsWith(QStringLiteral("/")) || str.startsWith(QStringLiteral("./")) || str.startsWith(QStringLiteral("../"))) ;
        else if(str.startsWith(VANILLA_SCHEME + QStringLiteral(":"))) ;
        else if(str.startsWith(QStringLiteral("edge:"))) ;
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
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*s://")))) str = QStringLiteral("http"     ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "s://"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*file:")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "file:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*ile:")))) str = QStringLiteral("f"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "ile:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(         "^.*le:")))) str = QStringLiteral("fi"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(         "le:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(      "^.*about:")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(      "about:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*bout:")))) str = QStringLiteral("a"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "bout:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*out:")))) str = QStringLiteral("ab"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "out:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(         "^.*ut:")))) str = QStringLiteral("abo"      ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(         "ut:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(          "^.*t:")))) str = QStringLiteral("abou"     ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(          "t:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(     "^.*chrome:")))) str = QString       (           ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(     "chrome:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(      "^.*hrome:")))) str = QStringLiteral("c"        ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(      "hrome:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(       "^.*rome:")))) str = QStringLiteral("ch"       ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(       "rome:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(        "^.*ome:")))) str = QStringLiteral("chr"      ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(        "ome:"))));
        else if(str.contains(QRegularExpression(QStringLiteral(         "^.*me:")))) str = QStringLiteral("chro"     ) + str.mid(str.indexOf(QRegularExpression(QStringLiteral(         "me:"))));
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
    return QSet<QUrl>(list.begin(), list.end()).values();
}

QList<QUrl> Page::DirtyStringToUrls(QString str){
    QList<QUrl> urls = ExtractUrlsFromText(str);
    if(!urls.isEmpty()) return urls;
    urls << CreateQueryUrl(str);
    return urls;
}

QList<QUrl> Page::MimeDataToUrls(const QMimeData *mime, QObject *source){
    QList<QUrl> urls;
    if(!mime->urls().isEmpty()){
        if(qobject_cast<TreeBank*>(source) || dynamic_cast<View*>(source))
            foreach(QUrl u, mime->urls()){ if(!u.isLocalFile()) urls << u;}
        else urls = mime->urls();
    }
    if(urls.isEmpty() && !mime->html().isEmpty()){
        urls = ExtractUrlsFromHtml(mime->html(), QUrl(), Page::HaveReference);
    }
    if(urls.isEmpty() && !mime->text().isEmpty()){
        urls = DirtyStringToUrls(mime->text());
    }
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
    google << tr("https://www.google.com/search?c&q=%1&ie=UTF-8&oe=UTF-8") << QStringLiteral("UTF-8") << QStringLiteral("true");
    SearchEngine yahoo;
    yahoo << tr("https://search.yahoo.com/search?ei=UTF-8&p=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine bing;
    bing << tr("https://www.bing.com/search?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine amazon;
    amazon << tr("https://www.amazon.com/s/?field-keywords=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine wiki;
    wiki << tr("https://en.wikipedia.org/w/index.php?search=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine ddg;
    ddg << QStringLiteral("https://duckduckgo.com/?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine brave;
    brave << QStringLiteral("https://search.brave.com/search?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine ecosia;
    ecosia << QStringLiteral("https://www.ecosia.org/search?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");
    SearchEngine yt;
    yt << QStringLiteral("https://www.youtube.com/results?search_query=%1") << QStringLiteral("UTF-8") << QStringLiteral("false");

    m_SearchEngineMap[QStringLiteral("google")] = google;
    m_SearchEngineMap[QStringLiteral("yahoo")] = yahoo;
    m_SearchEngineMap[QStringLiteral("bing")] = bing;
    m_SearchEngineMap[QStringLiteral("amazon")] = amazon;
    m_SearchEngineMap[QStringLiteral("wiki")] = wiki;
    m_SearchEngineMap[QStringLiteral("ddg")] = ddg;
    m_SearchEngineMap[QStringLiteral("brave")] = brave;
    m_SearchEngineMap[QStringLiteral("ecosia")] = ecosia;
    m_SearchEngineMap[QStringLiteral("yt")] = yt;
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
    for(QString key : m_SearchEngineMap.keys()){
        if(m_SearchEngineMap[key].length() < 3)
            m_SearchEngineMap[key] << QStringLiteral("false");
        else if(m_SearchEngineMap[key][2] == QStringLiteral("true"))
            return m_SearchEngineMap[key];
    }
    const SearchEngine google = m_SearchEngineMap.value(QStringLiteral("google"));
    if(!google.isEmpty()) return google;
    return m_SearchEngineMap.isEmpty() ? SearchEngine() : m_SearchEngineMap.first();
}

bool Page::ShiftMod(){
    if(Application::ByKey()) return false;
    return Application::keyboardModifiers() & Qt::ShiftModifier;
}

bool Page::CtrlMod(){
    if(Application::ByKey()) return false;
    return Application::keyboardModifiers() & Qt::ControlModifier
#if defined(Q_OS_MAC)
        || Application::keyboardModifiers() & Qt::MetaModifier
#endif
        ;
}

std::function<void(QList<QNetworkRequest>)>
Page::AsAskedBy(std::function<void(QList<QNetworkRequest>)> callBack){
    const bool byKey = Application::ByKey();
    return [callBack, byKey](QList<QNetworkRequest> reqs){
        Application::InputScope asked(byKey);
        callBack(reqs);
    };
}

bool Page::Activate(){
    if(View::ActivateNewViewDefault())
        return !CtrlMod();
    else return CtrlMod();
}

bool Page::OffTheRecord(View *view){
    if(!view) return true;
    return DirectoryPage::SaysPrivate(view->SpecificSettings()) || view->IsOffTheRecord();
}

void Page::Download(const QNetworkRequest &req,
                    const QString &file){

    NetworkAccessManager *nam = GetNetworkAccessManager();
    if(!nam) return;

    QNetworkRequest navigation(req);
    NetworkAccessManager::SetRequestPurpose(navigation,
                                            NetworkAccessManager::Navigation);

    DownloadItem *item = 0;

    const QUrl source = m_View->url();
    const bool offTheRecord = OffTheRecord(m_View);
    NetworkAccessManager::SetRequestOffTheRecord(navigation, offTheRecord);
    if(file.isEmpty()){
        item = NetworkController::Download(nam, navigation);
        if(item) item->SetSource(source, offTheRecord);
    } else {
        item = NetworkController::Download(nam, navigation, NetworkController::SelectedDirectory);
        if(item){
            item->SetSource(source, offTheRecord);
            item->SetPathAndReady(file);
        }
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

    Download(DirtyStringToUrls(url).first(),
             m_View->url(), file);
}

void Page::SetSource(const QUrl &url){
    m_View->GetViewNode()->SetUrl(QUrl());
    QUrl other = QUrl::fromEncoded(url.toEncoded().mid(12));
    QNetworkRequest req(other);
    req.setRawHeader("Referer", other.toEncoded());
    NetworkAccessManager::SetRequestPurpose(req, NetworkAccessManager::Navigation);
    NetworkAccessManager::SetRequestOffTheRecord(req, OffTheRecord(m_View));
    NetworkAccessManager *nam = GetNetworkAccessManager();
    if(!nam) return;
    DownloadItem *item =
        NetworkController::Download(nam, req, NetworkController::ToVariable);

    if(!item){
        SetSource(QByteArray());
        return;
    }

    connect(item, SIGNAL(DownloadResult(const QByteArray&)),
            this, SLOT(SetSource(const QByteArray&)));
}

void Page::SetSource(const QByteArray &html){
    QByteArray encoding = "UTF-8";
        SetSource(QString::fromUtf8(html));
}

void Page::SetSource(const QString &html){
    QUrl url = QUrl(m_View->GetViewNode()->GetUrl());
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
        "<body contenteditable=\"true\">") +
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

void Page::OpenSettings(){
    if(TreeBank *tb = GetTB()) tb->OpenSettings();
}

void Page::OpenDirectorySettings(){
    if(TreeBank *tb = GetTB()) tb->OpenDirectorySettings();
}

void Page::Quit(){
    if(m_View->GetDisplayObscured())
        m_View->ExitFullScreen();
    QTimer::singleShot(0, [](){
        Application::Quit();
    });
}

void Page::ClearCookies(){
#ifdef WEBENGINEVIEW
    if(!ModalDialog::Question(tr("Clear cookies."),
                              tr("Every cookie of every network space is deleted.\n"
                                 "Anything a site kept you signed in with goes with them."))) return;
    NetworkController::ClearCookies();
#endif
}

void Page::ClearHttpCache(){
#ifdef WEBENGINEVIEW
    NetworkController::ClearHttpCache();
#endif
}

void Page::ClearVisitedLinks(){
#ifdef WEBENGINEVIEW
    NetworkController::ClearVisitedLinks();
#endif
}

void Page::OpenPasswords(){
    if(TreeBank *tb = GetTB()) tb->OpenPasswords();
}

void Page::OpenDownloads(){
    if(TreeBank *tb = GetTB()) tb->OpenDownloads();
}

void Page::ShowDownloads(){
    if(TreeBank *tb = GetTB()) tb->ShowDownloads();
}

void Page::ClearPasswords(){
    if(!ModalDialog::Question(tr("Clear saved passwords."),
                              tr("Every saved password is deleted."))) return;
    NetworkController::ClearPasswords();
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

void Page::ToggleMiniMap(){
    GetTB()->ToggleMiniMap();
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

void Page::Rewind(){
    View::SetSwitchingState(true);
    GetTB()->Rewind();
    View::SetSwitchingState(false);
}

void Page::FastForward(){
    View::SetSwitchingState(true);
    GetTB()->FastForward();
    View::SetSwitchingState(false);
}

void Page::UpDirectory(){
    View::SetSwitchingState(true);
    GetTB()->UpDirectory();
    View::SetSwitchingState(false);
}

void Page::Close(){
    if(m_View->GetDisplayObscured())
        m_View->ExitFullScreen();
    View::CloseLater(m_View->GetThis());
}

void Page::Restore(){
    View::SetSwitchingState(true);
    GetTB()->Restore();
    View::SetSwitchingState(false);
}

void Page::Recreate(){
    if(m_View->GetDisplayObscured())
        m_View->ExitFullScreen();
    WeakView weak = m_View->GetThis();
    TreeBank::WhenItMayChange([weak](){
        SharedView view = weak.lock();
        if(!view) return;
        ViewNode *vn = view->GetViewNode();
        TreeBank *tb = view->GetTreeBank();
        if(!vn || vn->GetView() != view.get() || !TreeBank::IsLive(tb)) return;
        View::SetSwitchingState(true);
        tb->Recreate(vn);
        View::SetSwitchingState(false);
    });
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
    View::SetSwitchingState(true);
    GetTB()->TenthView();
    View::SetSwitchingState(false);
}

void Page::LastView(){
    View::SetSwitchingState(true);
    GetTB()->LastView();
    View::SetSwitchingState(false);
}

void Page::NewViewNode(){
    View::SetSwitchingState(true);
    SuitTB()->NewViewNode(m_View->GetViewNode());
    View::SetSwitchingState(false);
}

void Page::CloneViewNode(){
    View::SetSwitchingState(true);
    SuitTB()->CloneViewNode(m_View->GetViewNode());
    View::SetSwitchingState(false);
}

void Page::DisplayAccessKey(){
    GetTB()->DisplayAccessKey();
}

void Page::DisplayViewTree(){
    GetTB()->DisplayViewTree();
}

void Page::DisplayTrashTree(){
    GetTB()->DisplayTrashTree();
}

void Page::CallWithHeldSelection(StringCallBack callBack){
    if(m_MenuTarget.up){
        callBack(View::MenuSelectedText());
        return;
    }
    const QPointer<Page> alive(this);
    m_View->CallWithSelectedText([alive, callBack](QString text){
        if(alive) callBack(text);
    });
}

void Page::OpenTextSeeker(){
    CallWithHeldSelection([this](const QString &words){
        TreeBank *tb = GetTB();
        if(!words.isEmpty() && tb->GetReceiver()) tb->GetReceiver()->OpenTextSeeker(m_View, words);
        else tb->OpenTextSeeker();
    });
}

void Page::OpenQueryEditor(){
    CallWithHeldSelection([this](const QString &words){
        TreeBank *tb = GetTB();
        if(!words.isEmpty() && tb->GetReceiver()) tb->GetReceiver()->OpenQueryEditor(m_View, words);
        else tb->OpenQueryEditor();
    });
}

void Page::OpenUrlEditor(){
    GetTB()->OpenUrlEditor();
}

void Page::ReleaseHiddenView(){
    GetTB()->ReleaseHiddenView();
}

void Page::Load(){
    m_View->Load();
}

void Page::Copy(){
    m_View->Copy();
}

void Page::Cut(){
    m_View->Cut();
}

void Page::Paste(){
    m_View->Paste();
}

#define VANILLA_EDIT_ACTION(name)               \
    void Page::name(){                          \
        m_View->name();                         \
    }
FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

void Page::Undo(){
    m_View->Undo();
}

void Page::Redo(){
    m_View->Redo();
}

void Page::SelectAll(){
    m_View->SelectAll();
}

void Page::Unselect(){
    m_View->Unselect();
}

void Page::Reload(){
    m_View->Reload();
}

void Page::ReloadAndBypassCache(){
    m_View->ReloadAndBypassCache();
}

void Page::Stop(){
    m_View->Stop();
}

void Page::StopAndUnselect(){
    m_View->StopAndUnselect();
}

void Page::Print(){
    m_View->Print();
}

void Page::Save(){
    m_View->Save();
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

    if(TreeBank::DrawsViewSource(m_View)){
        newView = SuitTB()->OpenInNewViewNode(newUrl, Activate()||ShiftMod(), m_View->GetViewNode());
    } else {
        newView = SuitTB()->OpenInNewViewNode(QUrl(), Activate()||ShiftMod(), m_View->GetViewNode());
        ViewNode *h = newView->GetViewNode();
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
#ifdef WEBENGINEVIEW
                    if(qobject_cast<QuickWebEngineView*>(m_View->base())){
                        AddSearchEngineAt(e->Position());
                        return;
                    }
#endif
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
                    m_View->AddBookmarklet(e->Position());
                }
            }
        } else {
            m_View->AddBookmarklet(action->data().toPoint());
        }
    }
}

void Page::AddSearchEngineAt(QPoint pos){
    m_View->CallWithGotCurrentBaseUrl([this, pos](QUrl base){

    m_View->CallWithEvaluatedJavaScriptResult((QStringLiteral(
"(function(){\n"
"    var x = %1;\n"
"    var y = %2;\n")
+ View::ElementAtPointJsCode() +
QStringLiteral(
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
"})();")).arg(pos.x()).arg(pos.y()),
[this, base](QVariant var){

    if(!var.isValid()) return;
    QVariantList list = var.toMap().values();
    if(list.isEmpty()) return;

    QString encode = list[0].toString();
    QString method = list[1].toString();
    QUrl result = Page::StringToUrl(list[2].toString(), base);
    QUrlQuery queries = QUrlQuery(result);
    QMap<QString, QString> engines;
    for(QString key : list[3].toMap().keys()){
        QString k = QString::fromLatin1(QUrl::toPercentEncoding(key));
        QString v = QString::fromLatin1(QUrl::toPercentEncoding(list[3].toMap()[key].toString()));
        queries.addQueryItem(k, v);
    }
    for(QString key : list[4].toMap().keys()){
        QString k = QString::fromLatin1(QUrl::toPercentEncoding(key));
        QString v = QString::fromLatin1(QUrl::toPercentEncoding(list[4].toMap()[key].toString()));
        engines[k] = v;
    }
    QString tag = list[5].toString();

    QTimer::singleShot(0, this, [encode, result, queries, engines, tag]() mutable {
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
    });});});
}

void Page::AddBookmarkletAt(QPoint pos){
    m_View->CallWithHitElement(pos, [this](SharedWebElement elem){
    m_View->CallWithSelectedText([this, elem](QString text){
    QTimer::singleShot(0, this, [elem, text](){

    QUrl link  = elem ? elem->LinkUrl()  : QUrl();
    QUrl image = elem ? elem->ImageUrl() : QUrl();

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

        int count = 2;

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
    });});});
}

void Page::InspectElement(){
    m_View->InspectElement();
}

void Page::CopyUrl(){
    Application::clipboard()->setText(QString::fromUtf8(m_View->GetViewNode()->GetUrl().toEncoded()));
}

void Page::CopyTitle(){
    Application::clipboard()->setText(m_View->GetViewNode()->GetTitle());
}

void Page::CopyPageAsLink(){
    QString title = m_View->GetTitle();
    QString url = QString::fromUtf8(m_View->GetViewNode()->GetUrl().toEncoded());
    Application::clipboard()->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(url, title));
}

void Page::CopySelectedHtml(){
    m_View->CallWithSelectedHtml([](QString html){
        if(!html.isEmpty())
            Application::clipboard()->setText(html);
    });
}

void Page::OpenWithDefault(){
    Application::OpenUrlWithDefaultBrowser(m_View->GetViewNode()->GetUrl());
}

void Page::OpenWithCommand(){
    if(QAction *action = qobject_cast<QAction*>(sender()))
        Application::RunExternalCommand
            (action->property(EXTERNAL_COMMAND_PROPERTY).toString(),
             m_View->GetViewNode()->GetUrl());
}

void Page::ClickElement(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        QPoint pos;
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){

                    if(e->ClickEvent()) return;

#ifdef WEBENGINEVIEW
                    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base()))
                        pos = e->Position() / w->zoomFactor();
                    else
#endif
                        pos = e->Position();
                }
            }
        }
        if(pos.isNull()){
            pos = action->data().toPoint();
        }

        QMouseEvent pressEvent  (QEvent::MouseButtonPress,   pos, pos, Qt::LeftButton, Qt::MouseButtons(), Qt::KeyboardModifiers());
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, pos, pos, Qt::LeftButton, Qt::MouseButtons(), Qt::KeyboardModifiers());

        m_View->MousePressEvent(&pressEvent);
        m_View->MouseReleaseEvent(&releaseEvent);
    }
}

void Page::FocusElement(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        QPoint pos;
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){

                    e->SetFocus();

#ifdef WEBENGINEVIEW
                    if(WebEngineView *w = qobject_cast<WebEngineView*>(m_View->base()))
                        pos = e->Position() / w->zoomFactor();
                    else
#endif
                        pos = e->Position();
                }
            }
        }
        if(pos.isNull()){
            pos = action->data().toPoint();
        }
        m_View->CallWithHitElement(pos, [this](SharedWebElement e){
            if(!e || e->IsNull()) return;

            e->SetFocus();

            QKeyEvent tabPress = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
            QKeyEvent tabRelease = QKeyEvent(QEvent::KeyRelease, Qt::Key_Tab, Qt::NoModifier);
            QKeyEvent backTabPress = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier);
            QKeyEvent backTabRelease = QKeyEvent(QEvent::KeyRelease, Qt::Key_Tab, Qt::ShiftModifier);

            m_View->KeyPressEvent(&tabPress);
            m_View->KeyReleaseEvent(&tabRelease);
            m_View->KeyPressEvent(&backTabPress);
            m_View->KeyReleaseEvent(&backTabRelease);
        });
    }
}

void Page::HoverElement(){
    if(QAction *action = qobject_cast<QAction*>(sender())){
        QPoint pos;
        if(action->data().canConvert<SharedWebElement>()){
            if(SharedWebElement e = action->data().value<SharedWebElement>()){
                if(!e->IsNull()){
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

void Page::OpenCommand(){
    TreeBank *tb = GetTB();
    if(!tb) return;
    QAction *action = qobject_cast<QAction*>(sender());
    SharedWebElement e = action && action->data().canConvert<SharedWebElement>()
        ? action->data().value<SharedWebElement>() : SharedWebElement();
    if(!e || e->IsNull() || !tb->GetReceiver() || !m_View->GetViewNode()){
        tb->OpenCommand();
        return;
    }

    Receiver::TargetArgs target;
    target.element = e;
    target.view = m_View->GetThis();
    target.node = m_View->GetViewNode()->GetSerial();
    target.loadSerial = m_MenuTarget.up ? m_MenuTarget.load : m_View->LoadSerial();
    target.base = e->BaseUrl().isEmpty() ? m_View->url() : e->BaseUrl();
    if(m_MenuTarget.up){
        target.selection = View::MenuSelectedText();
        target.hitPos = m_MenuTarget.pos;
        target.playable = m_MenuTarget.type == MediaTypePlayable;
    } else {
        target.hitPos = e->Position();
        const QString tag = e->TagName().toUpper();
        target.playable = tag == QStringLiteral("VIDEO") || tag == QStringLiteral("AUDIO");
        tb->GetReceiver()->OpenTargetWhenFocused(target, qobject_cast<QWidget*>(m_View->base()));
        return;
    }
    tb->GetReceiver()->OpenTarget(target);
}

#define ELEMENT_ACTION(BLOCK)                                           \
    if(QAction *action = qobject_cast<QAction*>(sender())){             \
        if(action->data().canConvert<SharedWebElement>()){              \
            if(SharedWebElement e = action->data().value<SharedWebElement>()){ \
                if(!e->IsNull()){ BLOCK;}                               \
            }                                                           \
        } else {                                                        \
            m_View->CallWithHitElement                                  \
                (action->data().toPoint(), [this](SharedWebElement e){  \
                    if(e && !e->IsNull()){ Q_UNUSED(this) BLOCK;}      \
                });                                                     \
        }                                                               \
    }

#define NAMED_ELEMENT_ACTION(BLOCK)                                     \
    if(QAction *action = qobject_cast<QAction*>(sender())){             \
        const QString name =                                            \
            action->property(EXTERNAL_COMMAND_PROPERTY).toString();     \
        if(action->data().canConvert<SharedWebElement>()){              \
            if(SharedWebElement e = action->data().value<SharedWebElement>()){ \
                if(!e->IsNull()){ BLOCK;}                               \
            }                                                           \
        } else {                                                        \
            m_View->CallWithHitElement                                  \
                (action->data().toPoint(), [name](SharedWebElement e){  \
                    if(e && !e->IsNull()){ BLOCK;}                       \
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

void Page::OpenLinkWithDefault(){
    ELEMENT_ACTION(Application::OpenUrlWithDefaultBrowser(e->LinkUrl()));
}

void Page::OpenLinkWithCommand(){
    NAMED_ELEMENT_ACTION(Application::RunExternalCommand(name, e->LinkUrl()));
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

void Page::OpenImageWithDefault(){
    ELEMENT_ACTION(Application::OpenUrlWithDefaultBrowser(e->ImageUrl()));
}

void Page::OpenImageWithCommand(){
    NAMED_ELEMENT_ACTION(Application::RunExternalCommand(name, e->ImageUrl()));
}

void Page::LoadMedia(){
    ELEMENT_ACTION(m_View->Load(e->ImageUrl()));
}

void Page::OpenMedia(){
    ELEMENT_ACTION(OpenInNew(e->ImageUrl()));
}

void Page::DownloadMedia(){
    ELEMENT_ACTION(Download(e->ImageUrl(), e->BaseUrl()));
}

void Page::ToggleMediaControls(){
    m_View->ToggleMediaControls();
}

void Page::ToggleMediaLoop(){
    m_View->ToggleMediaLoop();
}

void Page::ToggleMediaPlayPause(){
    m_View->ToggleMediaPlayPause();
}

void Page::ToggleMediaMute(){
    m_View->ToggleMediaMute();
}

void Page::CopyMediaUrl(){
    ELEMENT_ACTION(Application::clipboard()->setText(e->ImageUrl().toString()));
}

void Page::CopyMediaHtml(){
    ELEMENT_ACTION(Application::clipboard()->setText(e->ImageHtml()));
}

void Page::OpenMediaWithDefault(){
    ELEMENT_ACTION(Application::OpenUrlWithDefaultBrowser(e->ImageUrl()));
}

void Page::OpenMediaWithCommand(){
    NAMED_ELEMENT_ACTION(Application::RunExternalCommand(name, e->ImageUrl()));
}
#undef NAMED_ELEMENT_ACTION
#undef ELEMENT_ACTION

void Page::OpenInNewViewNode                 (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenInNewDirectory                (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenOnRoot                        (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs, Activate()||ShiftMod()                       );});}

void Page::OpenInNewViewNodeForeground       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenInNewDirectoryForeground      (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenOnRootForeground              (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenInNewViewNodeBackground       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenInNewDirectoryBackground      (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenOnRootBackground              (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,             ShiftMod()                       );});}

void Page::OpenInNewViewNodeThisWindow       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewViewNode  (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenInNewDirectoryThisWindow      (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewDirectory (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenOnRootThisWindow              (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenOnSuitableNode (reqs, Activate()                                   );});}

void Page::OpenInNewViewNodeNewWindow        (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenInNewDirectoryNewWindow       (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenOnRootNewWindow               (){  LinkReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenImageInNewViewNode            (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageInNewDirectory           (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageOnRoot                   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs, Activate()||ShiftMod()                       );});}

void Page::OpenImageInNewViewNodeForeground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryForeground (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageOnRootForeground         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenImageInNewViewNodeBackground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryBackground (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenImageOnRootBackground         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,             ShiftMod()                       );});}

void Page::OpenImageInNewViewNodeThisWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewViewNode  (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryThisWindow (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewDirectory (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenImageOnRootThisWindow         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenOnSuitableNode (reqs, Activate()                                   );});}

void Page::OpenImageInNewViewNodeNewWindow   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageInNewDirectoryNewWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenImageOnRootNewWindow          (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenMediaInNewViewNode            (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenMediaInNewDirectory           (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs, Activate()||ShiftMod(), m_View->GetViewNode());});}
void Page::OpenMediaOnRoot                   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs, Activate()||ShiftMod()                       );});}

void Page::OpenMediaInNewViewNodeForeground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenMediaInNewDirectoryForeground (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenMediaOnRootForeground         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,          true                                );});}

void Page::OpenMediaInNewViewNodeBackground  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewViewNode  (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenMediaInNewDirectoryBackground (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenInNewDirectory (reqs,             ShiftMod(), m_View->GetViewNode());});}
void Page::OpenMediaOnRootBackground         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ SuitTB()->OpenOnSuitableNode (reqs,             ShiftMod()                       );});}

void Page::OpenMediaInNewViewNodeThisWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewViewNode  (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenMediaInNewDirectoryThisWindow (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenInNewDirectory (reqs, Activate()            , m_View->GetViewNode());});}
void Page::OpenMediaOnRootThisWindow         (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){  GetTB()->OpenOnSuitableNode (reqs, Activate()                                   );});}

void Page::OpenMediaInNewViewNodeNewWindow   (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewViewNode  (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenMediaInNewDirectoryNewWindow  (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenInNewDirectory (reqs,          true         , m_View->GetViewNode());});}
void Page::OpenMediaOnRootNewWindow          (){ ImageReq(qobject_cast<QAction*>(sender()),[this](QList<QNetworkRequest> reqs){ MakeTB()->OpenOnSuitableNode (reqs,          true                                );});}

View *Page::OpenInNewViewNode            (QUrl url)                   { return GetTB()->OpenInNewViewNode  (url,                        true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectory           (QUrl url)                   { return GetTB()->OpenInNewDirectory (url,                        true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QUrl url)                   { return GetTB()->OpenOnSuitableNode (url,                        true                        ).get();}

View *Page::OpenInNewViewNode            (QList<QUrl> urls)           { return GetTB()->OpenInNewViewNode  (urls,                       true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectory           (QList<QUrl> urls)           { return GetTB()->OpenInNewDirectory (urls,                       true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QList<QUrl> urls)           { return GetTB()->OpenOnSuitableNode (urls,                       true                        ).get();}

View *Page::OpenInNewViewNode            (QString query)              { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query),      true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectory           (QString query)              { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query),      true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QString query)              { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query),      true                        ).get();}

View *Page::OpenInNewViewNode            (QString key, QString query) { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query, key), true,  m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectory           (QString key, QString query) { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query, key), true,  m_View->GetViewNode()).get();}
View *Page::OpenOnRoot                   (QString key, QString query) { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query, key), true                        ).get();}

View *Page::OpenInNewViewNodeBackground  (QUrl url)                   { return GetTB()->OpenInNewViewNode  (url,                        false, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryBackground (QUrl url)                   { return GetTB()->OpenInNewDirectory (url,                        false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QUrl url)                   { return GetTB()->OpenOnSuitableNode (url,                        false                       ).get();}

View *Page::OpenInNewViewNodeBackground  (QList<QUrl> urls)           { return GetTB()->OpenInNewViewNode  (urls,                       false, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryBackground (QList<QUrl> urls)           { return GetTB()->OpenInNewDirectory (urls,                       false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QList<QUrl> urls)           { return GetTB()->OpenOnSuitableNode (urls,                       false                       ).get();}

View *Page::OpenInNewViewNodeBackground  (QString query)              { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query),      false, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryBackground (QString query)              { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query),      false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QString query)              { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query),      false                       ).get();}

View *Page::OpenInNewViewNodeBackground  (QString key, QString query) { return GetTB()->OpenInNewViewNode  (CreateQueryUrl(query, key), false, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryBackground (QString key, QString query) { return GetTB()->OpenInNewDirectory (CreateQueryUrl(query, key), false, m_View->GetViewNode()).get();}
View *Page::OpenOnRootBackground         (QString key, QString query) { return GetTB()->OpenOnSuitableNode (CreateQueryUrl(query, key), false                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QUrl url)                   { return MakeTB()->OpenInNewViewNode  (url,                        true, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QUrl url)                   { return MakeTB()->OpenInNewDirectory (url,                        true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QUrl url)                   { return MakeTB()->OpenOnSuitableNode (url,                        true                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QList<QUrl> urls)           { return MakeTB()->OpenInNewViewNode  (urls,                       true, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QList<QUrl> urls)           { return MakeTB()->OpenInNewDirectory (urls,                       true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QList<QUrl> urls)           { return MakeTB()->OpenOnSuitableNode (urls,                       true                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QString query)              { return MakeTB()->OpenInNewViewNode  (CreateQueryUrl(query),      true, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QString query)              { return MakeTB()->OpenInNewDirectory (CreateQueryUrl(query),      true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QString query)              { return MakeTB()->OpenOnSuitableNode (CreateQueryUrl(query),      true                       ).get();}

View *Page::OpenInNewViewNodeNewWindow   (QString key, QString query) { return MakeTB()->OpenInNewViewNode  (CreateQueryUrl(query, key), true, m_View->GetViewNode()).get();}
View *Page::OpenInNewDirectoryNewWindow  (QString key, QString query) { return MakeTB()->OpenInNewDirectory (CreateQueryUrl(query, key), true, m_View->GetViewNode()).get();}
View *Page::OpenOnRootNewWindow          (QString key, QString query) { return MakeTB()->OpenOnSuitableNode (CreateQueryUrl(query, key), true                       ).get();}

void Page::OpenAllUrl(){
    auto open = AsAskedBy([this](QList<QNetworkRequest> reqs){
        SuitTB()->OpenInNewViewNode(reqs, Activate()||ShiftMod(), m_View->GetViewNode());
    });
    m_View->CallWithGotCurrentBaseUrl([this, open](QUrl base){
    m_View->CallWithSelectedHtml([this, open, base](QString html){

    QList<QUrl> urls = ExtractUrlsFromHtml(html, base, HaveReference);

    UrlCountCheck(urls.length(), [this, open, urls](bool result){

    if(result) open(RequestsFor(urls, true));
});});});
}

void Page::OpenAllImage(){
    auto open = AsAskedBy([this](QList<QNetworkRequest> reqs){
        SuitTB()->OpenInNewViewNode(reqs, Activate()||ShiftMod(), m_View->GetViewNode());
    });
    m_View->CallWithGotCurrentBaseUrl([this, open](QUrl base){
    m_View->CallWithSelectedHtml([this, open, base](QString html){

    QList<QUrl> urls = ExtractUrlsFromHtml(html, base, HaveSource);

    UrlCountCheck(urls.length(), [this, open, urls](bool result){

    if(result) open(RequestsFor(urls, true));
});});});
}

void Page::OpenTextAsUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedText([this, base](QString text){
        OpenHeldUrls(AddressesOfText, text, base);
    });});
}

namespace {
    QList<QUrl> HeldUrlList(Page::HeldUrls kind, const QString &held, const QUrl &base){
        switch(kind){
        case Page::AddressesOfText: return Page::ExtractUrlsFromText(held, base);
        case Page::LinksOfHtml:     return Page::ExtractUrlsFromHtml(held, base, Page::HaveReference);
        case Page::ImagesOfHtml:    return Page::ExtractUrlsFromHtml(held, base, Page::HaveSource);
        }
        return QList<QUrl>();
    }
}

void Page::OpenHeldUrls(HeldUrls kind, const QString &held, const QUrl &base, const std::function<bool()> &still){
    auto open = AsAskedBy([this](QList<QNetworkRequest> reqs){
        SuitTB()->OpenInNewViewNode(reqs, Activate()||ShiftMod(), m_View->GetViewNode());
    });

    const QList<QUrl> urls = HeldUrlList(kind, held, base);
    const bool referer = kind != AddressesOfText;

    UrlCountCheck(urls.length(), [open, urls, still, referer, this](bool result){

    if(result && (!still || still())) open(RequestsFor(urls, referer));
});
}

void Page::SaveHeldUrls(HeldUrls kind, const QString &held, const QUrl &base){
    DownloadInto(HeldUrlList(kind, held, base), kind != AddressesOfText);
}

void Page::SaveAllUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedHtml([this, base](QString html){

    QList<QUrl> urls = ExtractUrlsFromHtml(html, base, HaveReference);

    DownloadInto(urls, true);
});});
}

void Page::SaveAllImage(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedHtml([this, base](QString html){

    QList<QUrl> urls = ExtractUrlsFromHtml(html, base, HaveSource);

    DownloadInto(urls, true);
});});
}

void Page::SaveTextAsUrl(){
    m_View->CallWithGotCurrentBaseUrl([this](QUrl base){
    m_View->CallWithSelectedText([this, base](QString text){

    QList<QUrl> urls = ExtractUrlsFromText(text, base);

    DownloadInto(urls, false);
});});
}

QList<QNetworkRequest> Page::RequestsFor(const QList<QUrl> &urls, bool referer) const {
    QList<QNetworkRequest> reqs;
    foreach(QUrl url, urls){
        QNetworkRequest req(url);
        if(referer) req.setRawHeader("Referer", m_View->url().toEncoded());
        reqs << req;
    }
    return reqs;
}

void Page::DownloadInto(const QList<QUrl> &urls, bool referer){
    if(urls.isEmpty()) return;
    QString directory =
        ModalDialog::GetExistingDirectory(QString(), Application::GetDownloadDirectory());
    if(directory.isEmpty()) return;
    const Application::DownloadPolicy policy = Application::GetDownloadPolicy();
    if(policy == Application::Undefined_ || policy == Application::AskForEachDownload)
        Application::SetDownloadDirectory(directory + QStringLiteral("/"));
    const QList<QNetworkRequest> reqs = RequestsFor(urls, referer);
    for(int i = 0; i < reqs.size(); i++)
        Download(reqs[i], directory + QStringLiteral("/") + urls[i].toString().split(QStringLiteral("/")).last());
}

QAction *Page::Action(CustomAction a, QVariant data){
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
        webaction->setObjectName(ActionToString(a));
    }

    webaction->setData(data);

    switch(a){
    case _Back:    webaction->setEnabled(m_View->CanGoBack()); break;
    case _Forward: webaction->setEnabled(m_View->CanGoForward()); break;
    case _Rewind:  webaction->setEnabled(m_View->CanGoBack()); break;
    case _Reload:  webaction->setEnabled(!m_View->IsLoading()); break;
    case _Stop:    webaction->setEnabled(m_View->IsLoading()); break;
    default: break;
    }

    switch(a){
    case _Up:          webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));    break;
    case _Down:        webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));  break;
    case _Right:       webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight)); break;
    case _Left:        webaction->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));  break;
    case _Back:        webaction->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/back.png"), Theme::BarIcon)));        break;
    case _Forward:     webaction->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/forward.png"), Theme::BarIcon)));     break;
    case _Rewind:      webaction->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/rewind.png"), Theme::BarIcon)));      break;
    case _FastForward: webaction->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/fastforward.png"), Theme::BarIcon))); break;
    case _Reload:      webaction->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/reload.png"), Theme::BarIcon)));      break;
    case _Stop:        webaction->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/stop.png"), Theme::BarIcon)));        break;
    default: break;
    }

    if(!create){
        switch(a){
        case _ToggleMenuBar:  webaction->setChecked(!GetTB()->GetMainWindow()->IsMenuBarEmpty()); break;
        case _ToggleTreeBar:  webaction->setChecked(GetTB()->GetMainWindow()->GetTreeBar()->isVisible()); break;
        case _ToggleToolBar:  webaction->setChecked(GetTB()->GetMainWindow()->GetToolBar()->isVisible()); break;
        case _ToggleMiniMap:  webaction->setChecked(GetTB()->GetMiniMap()); break;
        default: break;
        }
        return webaction;
    }

    if(a != _NoAction){
        const QString label = ActionText::MenuLabel(ActionToString(a));
        webaction->setText(label);
        webaction->setToolTip(label);
    }

    switch(a){
    case _NoAction: break;

#define CONNECT_ACTION(name)                                            \
        case _##name:                                                   \
            connect(webaction, SIGNAL(triggered()),                     \
                    this,      SLOT(name##Key()));                      \
            break;

        FOR_EACH_KEYBOARD_EVENTS(CONNECT_ACTION)

#undef  CONNECT_ACTION
#define CONNECT_ACTION(name)                                            \
        case _##name:                                                   \
            connect(webaction, SIGNAL(triggered()),                     \
                    this,      SLOT(name()));                           \
            break;

        FOR_EACH_APPLICATION_EVENTS(CONNECT_ACTION)
        FOR_EACH_NAVIGATION_EVENTS(CONNECT_ACTION)
        FOR_EACH_VIEW_EVENTS(CONNECT_ACTION)
        FOR_EACH_WEB_EVENTS1(CONNECT_ACTION)
        FOR_EACH_WEB_EVENTS2(CONNECT_ACTION)

#undef  CONNECT_ACTION
    }
    switch(a){

    case _ToggleMenuBar:
        webaction->setCheckable(true);
        webaction->setChecked(!GetTB()->GetMainWindow()->IsMenuBarEmpty());
        break;
    case _ToggleTreeBar:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetMainWindow()->GetTreeBar()->isVisible());
        break;
    case _ToggleToolBar:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetMainWindow()->GetToolBar()->isVisible());
        break;
    case _ToggleMiniMap:
        webaction->setCheckable(true);
        webaction->setChecked(GetTB()->GetMiniMap());
        webaction->setEnabled(TreeBank::CanHaveMiniMap());
        break;

    case _NewViewNode:
    case _CloneViewNode:
    case _OpenInNewViewNode:
    case _OpenInNewDirectory:
    case _OpenOnRoot:
    case _OpenImageInNewViewNode:
    case _OpenImageInNewDirectory:
    case _OpenImageOnRoot:
    case _OpenMediaInNewViewNode:
    case _OpenMediaInNewDirectory:
    case _OpenMediaOnRoot:
    case _ViewSource:
    case _OpenAllUrl:
    case _OpenAllImage:
    case _OpenTextAsUrl:
        webaction->setToolTip(webaction->toolTip() +
                              (View::ActivateNewViewDefault()
                               ? tr("\n Shift+Click: InNewWindow"
                                    "\n Ctrl+Click: InBackground")
                               : tr("\n Shift+Click: InNewWindow"
                                    "\n Ctrl+Click: InForeground")));
        break;
    case _OpenInNewViewNodeForeground:
    case _OpenInNewDirectoryForeground:
    case _OpenOnRootForeground:
    case _OpenInNewViewNodeBackground:
    case _OpenInNewDirectoryBackground:
    case _OpenOnRootBackground:
    case _OpenImageInNewViewNodeForeground:
    case _OpenImageInNewDirectoryForeground:
    case _OpenImageOnRootForeground:
    case _OpenImageInNewViewNodeBackground:
    case _OpenImageInNewDirectoryBackground:
    case _OpenImageOnRootBackground:
    case _OpenMediaInNewViewNodeForeground:
    case _OpenMediaInNewDirectoryForeground:
    case _OpenMediaOnRootForeground:
    case _OpenMediaInNewViewNodeBackground:
    case _OpenMediaInNewDirectoryBackground:
    case _OpenMediaOnRootBackground:
        webaction->setToolTip(webaction->toolTip() +
                              tr("\n Shift+Click: InNewWindow"));
        break;
    case _OpenInNewViewNodeThisWindow:
    case _OpenInNewDirectoryThisWindow:
    case _OpenOnRootThisWindow:
    case _OpenImageInNewViewNodeThisWindow:
    case _OpenImageInNewDirectoryThisWindow:
    case _OpenImageOnRootThisWindow:
    case _OpenMediaInNewViewNodeThisWindow:
    case _OpenMediaInNewDirectoryThisWindow:
    case _OpenMediaOnRootThisWindow:
        webaction->setToolTip(webaction->toolTip() +
                              (View::ActivateNewViewDefault()
                               ? tr("\n Ctrl+Click: InBackground")
                               : tr("\n Ctrl+Click: InForeground")));
        break;
    default: break;
    }

    if(data.canConvert<SharedWebElement>()){
        if(SharedWebElement e = data.value<SharedWebElement>()){
            switch(a){
            case _ToggleMediaLoop:
                webaction->setCheckable(true);
                webaction->setChecked(e->IsLooped());
                break;
            case _ToggleMediaPlayPause:
                webaction->setCheckable(true);
                webaction->setChecked(e->IsPaused());
                break;
            case _ToggleMediaMute:
                webaction->setCheckable(true);
                webaction->setChecked(e->IsMuted());
                break;
            default: break;
            }
        }
    }
    return webaction;
}

void Page::DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                              QPoint localPos, QPoint globalPos, MediaType type,
                              const std::function<void(QMenu*)> &extra){

    const quint64 load = m_View->LoadSerial();
    QMenu *menu = new QMenu(parent);
    menu->setToolTipsVisible(true);

    m_View->AddSpellCheckMenu(menu);
    m_View->AddContextMenu(menu, elem, type);
    m_View->AddExtensionMenu(menu, elem, type);

    if(extra){
        if(!menu->isEmpty()) menu->addSeparator();
        extra(menu);
    }

    if(elem && !elem->IsNull() && elem->IsQueryInputElement()){
        if(!menu->isEmpty()) menu->addSeparator();
        menu->addAction(Action(Page::_AddSearchEngine, localPos));
    }

    m_View->AddRegularMenu(menu, elem);

    QHash<QString, QKeySequence> keys = InputMap::ShownKeys(View::KeyLayers());
    keys.remove(QStringLiteral("OpenCommand"));
    MenuKeys::Show(menu, keys);

    View::AddMenuCommandCell(menu, Action(Page::_OpenCommand, QVariant::fromValue(elem)));

    m_MenuTarget.up = true;
    m_MenuTarget.pos = localPos;
    m_MenuTarget.type = type;
    m_MenuTarget.load = load;
    const QPointer<Page> alive(this);
    menu->exec(globalPos);
    if(alive) m_MenuTarget = MenuTarget();
    delete menu;
}

void Page::DownloadSuggest(const QUrl& url){
    QNetworkRequest req(url);
    NetworkAccessManager::SetRequestOffTheRecord(req, OffTheRecord(m_View));
    DownloadItem *item =
        NetworkController::Download(GetNetworkAccessManager(),
                                    req, NetworkController::ToVariable);

    if(!item) return;

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
    callBack = AsAskedBy(callBack);

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

    if(!url.isEmpty()){
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
        QList<QUrl> urls = ExtractUrlsFromHtml(html, base, HaveReference);

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
        QList<QUrl> urls = ExtractUrlsFromText(text, base);

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
    callBack = AsAskedBy(callBack);

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
                                [this, callBack](QUrl url){

    if(!url.isEmpty()){
        QList<QNetworkRequest> reqs;
        QNetworkRequest req(url);
        req.setRawHeader("Referer", m_View->url().toEncoded());
        reqs << req;
        callBack(reqs);
        return;
    }

    m_View->CallWithGotCurrentBaseUrl([this, callBack](QUrl base){
    m_View->CallWithSelectedHtml([this, callBack, base](QString html){

    if(!html.isEmpty()){
        QList<QNetworkRequest> reqs;
        QList<QUrl> urls = ExtractUrlsFromHtml(html, base, HaveSource);

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
