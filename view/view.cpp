#include "switch.hpp"
#include "const.hpp"
#include "keymap.hpp"
#include "mousemap.hpp"

#include "view.hpp"

#include <QNetworkRequest>
#include <QTextCodec>
#include <QKeyEvent>
#include <QKeySequence>
#include <QSettings>
#include <QPixmap>
#include <QMenu>

#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#ifdef QTWEBKIT
#  include "webpage.hpp"
#endif
#include "webenginepage.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "mainwindow.hpp"
#include "application.hpp"
#include "networkcontroller.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

bool View::m_EnableLoadHack = false;
bool View::m_EnableDragHack = false;

QMap<QKeySequence, QString> View::m_KeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> View::m_MouseMap = QMap<QString, QString>();
QMap<QString, QString> View::m_LeftGestureMap = QMap<QString, QString>();
QMap<QString, QString> View::m_RightGestureMap = QMap<QString, QString>();

QKeyEvent *View::m_UpKey       = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, 0);
QKeyEvent *View::m_DownKey     = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, 0);
QKeyEvent *View::m_RightKey    = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, 0);
QKeyEvent *View::m_LeftKey     = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, 0);
QKeyEvent *View::m_PageUpKey   = new QKeyEvent(QEvent::KeyPress, Qt::Key_PageUp, 0);
QKeyEvent *View::m_PageDownKey = new QKeyEvent(QEvent::KeyPress, Qt::Key_PageDown, 0);
QKeyEvent *View::m_HomeKey     = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, 0);
QKeyEvent *View::m_EndKey      = new QKeyEvent(QEvent::KeyPress, Qt::Key_End, 0);

SharedWebElement View::m_ClickedElement = 0;
QRegion View::m_SelectionRegion = QRegion();

bool View::m_ActivateNewViewDefault = false;
bool View::m_NavigationBySpaceKey = false;
bool View::m_DragStarted = false;
bool View::m_HadSelection = false;
bool View::m_Switching = false;
QPoint View::m_GestureStartedPos = QPoint();
QPoint View::m_BeforeGesturePos = QPoint();
View::Gesture View::m_Gesture = QList<View::GestureVector>();
View::GestureVector View::m_CurrentGestureVector = Gv_NoMove;
View::GestureVector View::m_BeforeGestureVector = Gv_NoMove;
int View::m_SameGestureVectorCount = 0;
View::ScrollBarState View::m_ScrollBarState = NoScrollBarEnabled;

int View::m_GestureMode = 0;

const QList<float> View::m_ZoomFactorLevels = QList<float>()

    //`(0.1 ,@(map (^n (expt 5 (/ n 24))) (iota 49 -24)) 10.0)
    // or
    //[0.1] ++ map ((5**).(/24)) [-24..24] ++ [10.0]

    <<  0.10000000000000000f
    <<  0.20000000000000000f
    <<  0.21387190896262373f
    <<  0.22870596721658407f
    <<  0.24456890899877037f
    <<  0.26153209720236610f
    <<  0.27967184441834253f
    <<  0.29906975624424410f
    <<  0.31981309840471520f
    <<  0.34199518933533940f
    <<  0.36571581999591485f
    <<  0.39108170280178794f
    <<  0.41820695169285930f
    <<  0.44721359549995790f
    <<  0.47823212691807320f
    <<  0.51140208955612030f
    <<  0.54687270570421050f
    <<  0.58480354764257330f
    <<  0.62536525551215900f
    <<  0.66874030497642200f
    <<  0.71512382812777280f
    <<  0.76472449133173000f
    <<  0.81776543395794250f
    <<  0.87448527222116790f
    <<  0.93513917264820420f
    <<  1.00000000000000000f
    <<  1.06935954481311860f
    <<  1.14352983608292020f
    <<  1.22284454499385190f
    <<  1.30766048601183060f
    <<  1.39835922209171270f
    <<  1.49534878122122050f
    <<  1.59906549202357630f
    <<  1.70997594667669680f
    <<  1.82857909997957440f
    <<  1.95540851400894000f
    <<  2.09103475846429630f
    <<  2.23606797749979000f
    <<  2.39116063459036580f
    <<  2.55701044778060200f
    <<  2.73436352852105300f
    <<  2.92401773821286600f
    <<  3.12682627756079560f
    <<  3.34370152488211000f
    <<  3.57561914063886380f
    <<  3.82362245665865030f
    <<  4.08882716978971200f
    <<  4.37242636110583850f
    <<  4.67569586324102100f
    <<  5.00000000000000000f
    << 10.00000000000000000f
    ;

#ifdef QTWEBKIT
const QMap<QWebSettings::WebAttribute, QString> View::m_WebSwitches = QMap<QWebSettings::WebAttribute, QString>()
    << qMakePair(QWebSettings::AutoLoadImages,           QString::fromLatin1("[iI]mage"))
    << qMakePair(QWebSettings::DeveloperExtrasEnabled,   QString::fromLatin1("[iI]nspector"))
    << qMakePair(QWebSettings::DnsPrefetchEnabled,       QString::fromLatin1("[dD][nN][sS][pP]refetch"))
    << qMakePair(QWebSettings::FrameFlatteningEnabled,   QString::fromLatin1("[fF]rame[fF]latten"))
    << qMakePair(QWebSettings::JavascriptEnabled,        QString::fromLatin1("[jJ](?:ava)?[sS](?:cript)?"))
    << qMakePair(QWebSettings::PluginsEnabled,           QString::fromLatin1("[pP]lugin"))
    << qMakePair(QWebSettings::PrivateBrowsingEnabled,   QString::fromLatin1("(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"))
    << qMakePair(QWebSettings::SpatialNavigationEnabled, QString::fromLatin1("[sS]patial(?:[nN]avigation)?"))
    << qMakePair(QWebSettings::TiledBackingStoreEnabled, QString::fromLatin1("[tT]iled[bB]acking[sS]tore"))
    << qMakePair(QWebSettings::ZoomTextOnly,             QString::fromLatin1("[zZ]oom[tT]ext[oO]nly"))
    << qMakePair(QWebSettings::CaretBrowsingEnabled,     QString::fromLatin1("[cC]aret[bB]rowse"))
    << qMakePair(QWebSettings::ScrollAnimatorEnabled,    QString::fromLatin1("[sS]croll[aA]nimator"))
    << qMakePair(QWebSettings::WebAudioEnabled,          QString::fromLatin1("[wW]eb[aA]udio"))
    << qMakePair(QWebSettings::WebGLEnabled,             QString::fromLatin1("[wW]eb[gG][lL]"))
    ;
#endif

const QMap<QWebEngineSettings::WebAttribute, QString> View::m_WebEngineSwitches = QMap<QWebEngineSettings::WebAttribute, QString>()
    << qMakePair(QWebEngineSettings::AutoLoadImages,           QString::fromLatin1("[iI]mage"))
  //<< qMakePair(QWebEngineSettings::DeveloperExtrasEnabled,   QString::fromLatin1("[iI]nspector"))
  //<< qMakePair(QWebEngineSettings::DnsPrefetchEnabled,       QString::fromLatin1("[dD][nN][sS][pP]refetch"))
  //<< qMakePair(QWebEngineSettings::FrameFlatteningEnabled,   QString::fromLatin1("[fF]rame[fF]latten"))
    << qMakePair(QWebEngineSettings::JavascriptEnabled,        QString::fromLatin1("[jJ](?:ava)?[sS](?:cript)?"))
#if QT_VERSION >= 0x050600
    << qMakePair(QWebEngineSettings::PluginsEnabled,           QString::fromLatin1("[pP]lugin"))
#endif
  //<< qMakePair(QWebEngineSettings::PrivateBrowsingEnabled,   QString::fromLatin1("(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"))
    << qMakePair(QWebEngineSettings::SpatialNavigationEnabled, QString::fromLatin1("[sS]patial(?:[nN]avigation)?"))
  //<< qMakePair(QWebEngineSettings::TiledBackingStoreEnabled, QString::fromLatin1("[tT]iled[bB]acking[sS]tore"))
  //<< qMakePair(QWebEngineSettings::ZoomTextOnly,             QString::fromLatin1("[zZ]oom[tT]ext[oO]nly"))
  //<< qMakePair(QWebEngineSettings::CaretBrowsingEnabled,     QString::fromLatin1("[cC]aret[bB]rowse"))
    << qMakePair(QWebEngineSettings::ScrollAnimatorEnabled,    QString::fromLatin1("[sS]croll[aA]nimator"))
  //<< qMakePair(QWebEngineSettings::WebAudioEnabled,          QString::fromLatin1("[wW]eb[aA]udio"))
  //<< qMakePair(QWebEngineSettings::WebGLEnabled,             QString::fromLatin1("[wW]eb[gG][lL]"))
    ;

QString View::m_LinkMenu = QString();
QString View::m_ImageMenu = QString();
QString View::m_SelectionMenu = QString();
QString View::m_RegularMenu = QString();

View::View(TreeBank *parent, QString id, QStringList set){
    m_TreeBank = parent;
    m_This = WeakView();
    m_Master = WeakView();
    m_Slave = WeakView();
    m_ViewNode = 0;
    m_HistNode = 0;
    m_Page = 0;
    m_EnableLoadHackLocal = m_EnableLoadHack;
    m_EnableDragHackLocal = m_EnableDragHack;
    m_JsObject = new _View(this);
}

View::~View(){
    if(m_TreeBank && m_TreeBank->GetCurrentView().get() == this){
        m_TreeBank->SetCurrentView(SharedView());
        m_TreeBank->SetCurrentViewNode(0);
        m_TreeBank->SetCurrentHistNode(0);
    }
    if(m_HistNode && m_HistNode->GetView() == this){
        m_HistNode->SetView(0);
    }
    if(m_ViewNode && m_ViewNode->GetView() == this){
        m_ViewNode->SetView(0);
    }
}

QObject *View::base(){ return 0;}
QObject *View::page(){ return m_Page;}

void View::Initialize(){
    QObject::connect(base(), SIGNAL(titleChanged(const QString&)),
                     base(), SLOT(OnTitleChanged(const QString&)));
    QObject::connect(base(), SIGNAL(urlChanged(const QUrl&)),
                     base(), SLOT(OnUrlChanged(const QUrl&)));
    QObject::connect(base(), SIGNAL(loadStarted()),
                     base(), SLOT(OnLoadStarted()));
    QObject::connect(base(), SIGNAL(loadProgress(int)),
                     base(), SLOT(OnLoadProgress(int)));
    QObject::connect(base(), SIGNAL(loadFinished(bool)),
                     base(), SLOT(OnLoadFinished(bool)));
    QObject::connect(base(), SIGNAL(ViewChanged()),
                     base(), SLOT(OnViewChanged()));
    QObject::connect(base(), SIGNAL(ScrollChanged(QPointF)),
                     base(), SLOT(OnViewChanged()));
    QObject::connect(base(), SIGNAL(ScrollChanged(QPointF)),
                     base(), SLOT(OnScrollChanged()));
}
void View::DeleteLater(){
    if(m_HistNode && m_HistNode->GetView() == this){
        m_HistNode->SetView(0);
    }
    if(m_ViewNode && m_ViewNode->GetView() == this){
        m_ViewNode->SetView(0);
    }
    m_TreeBank = 0;
    m_HistNode = 0;
    m_ViewNode = 0;
    if(base()) base()->deleteLater();
}

TreeBank *View::GetTreeBank(){
    return m_TreeBank;
}

ViewNode *View::GetViewNode(){
    return m_ViewNode;
}

HistNode* View::GetHistNode(){
    return m_HistNode;
}

WeakView View::GetThis(){
    return m_This;
}

WeakView View::GetMaster(){
    return m_Master;
}

WeakView View::GetSlave(){
    return m_Slave;
}

_View *View::GetJsObject(){
    return m_JsObject;
}

void View::SetTreeBank(TreeBank* t){
    m_TreeBank = t;
}

void View::SetViewNode(ViewNode* vn){
    m_ViewNode = vn;
    OnSetViewNode(vn);
}

void View::SetHistNode(HistNode* hn){
    m_HistNode = hn;
    OnSetHistNode(hn);
}

void View::SetThis(WeakView view){
    m_This = view;
    OnSetThis(view);
}

void View::SetMaster(WeakView view){
    m_Master = view;
    OnSetMaster(view);
}

void View::SetSlave(WeakView view){
    m_Slave = view;
    OnSetSlave(view);
}

static QList<int> Compare(QString str1, QString str2){
    if(str1.isEmpty() || str2.isEmpty())
        return QList<int>() << 0;

    if(str1.contains(QRegExp(QStringLiteral(".\\.[a-z]+$"))) &&
       str2.contains(QRegExp(QStringLiteral(".\\.[a-z]+$")))){
        // accept subdomain.
        if(str1.startsWith(str2) || str2.startsWith(str1) ||
           str1.endsWith(str2)   || str2.endsWith(str1)){
            return QList<int>() << 0;
        } else {
            return QList<int>() << INT_MAX;
        }
    }

    QRegExp reg =
        QRegExp(QRegExp::escape(str1).replace(QRegExp(QStringLiteral("[0-9]+")),
                                              QString(QStringLiteral("([0-9]+)"))));

    if(!reg.exactMatch(str1)) return QList<int>() << INT_MAX;
    QStringList nums1 = reg.capturedTexts();
    if(nums1.length() > 1) nums1.removeFirst();

    if(!reg.exactMatch(str2)) return QList<int>() << INT_MAX;
    QStringList nums2 = reg.capturedTexts();
    if(nums2.length() > 1) nums2.removeFirst();

    Q_ASSERT(nums1.length() == nums2.length());

    QList<int> list;
    for(int i = 0; i < nums1.length(); i++){
        list << (nums2[i].toInt() - nums1[i].toInt());
    }
    return list;
}

static QList<int> Compare(QStringList list1, QStringList list2){
    QList<int> list;
    for(int i = 0; i < qMax(list1.length(), list2.length()); i++){
        QString str1 = list1.value(i);
        QString str2 = list2.value(i);

        if(str1.isEmpty() && str2.isEmpty()) list << 0;
        else if(str1.isEmpty()) list <<  str2.toInt();
        else if(str2.isEmpty()) list << -str1.toInt();
        else list << Compare(str1, str2);
    }
    return list;
}

static QList<int> Compare(QUrlQuery query1, QUrlQuery query2){
    QSet<QString> keys;
    typedef QPair<QString, QString> StringPair;
    foreach(StringPair item, query1.queryItems()){
        keys.insert(item.first);
    }
    foreach(StringPair item, query2.queryItems()){
        keys.insert(item.first);
    }
    QList<int> list;

    foreach(QString key, keys){
        QString value1 = query1.queryItemValue(key);
        QString value2 = query2.queryItemValue(key);

        if(value1.isEmpty() && value2.isEmpty()) list << 0;
        else if(value1.isEmpty()) list <<  value2.toInt();
        else if(value2.isEmpty()) list << -value1.toInt();
        else  list << Compare(value1, value2);
    }
    // ignore different query.
    list.removeAll(INT_MAX);
    return list;
}

void View::GoBackTo(QUrl target){
    if(target.isEmpty()) return;

    QNetworkRequest req(target);
    req.setRawHeader("Referer", url().toEncoded());

    if(m_EnableLoadHackLocal){
        m_TreeBank->OpenInNewHistNodeBackward(req, true, m_HistNode);
    } else {
        /* do nothing */
        // instead of load backward url.
        //SetScroll(QPointF());
        //Load(req);
    }
}

void View::GoForwardTo(QUrl target){
    if(target.isEmpty()) return;

    QNetworkRequest req(target);
    req.setRawHeader("Referer", url().toEncoded());

    if(m_EnableLoadHackLocal){
        m_TreeBank->OpenInNewHistNode(req, true, m_HistNode);
    } else {
        SetScroll(QPointF());
        Load(req);
    }
}

void View::GoBackToInferedUrl(){
    if(!m_EnableLoadHackLocal) return;

    CallWithFoundElements(Page::RelIsPrev, [this](SharedWebElementList prevs){

    if(!prevs.isEmpty() && !prevs[0]->LinkUrl().isEmpty()){
        GoBackTo(prevs[0]->LinkUrl());
        return;
    }

    CallWithFoundElements(Page::HaveReference, [this](SharedWebElementList elements){

    QUrl base = url();
    QUrl copy = base;
    base.setFragment(QString()); // ignore fragment.

    QUrlQuery basequery = QUrlQuery(base);
    QStringList basebody = base.toString(QUrl::RemoveQuery).split(QStringLiteral("/"));
    basebody.removeFirst(); // ignore schema.

    SharedWebElement currentHighValidityElement = 0;
    QList<int> currentHighValidityDistance = QList<int>();

    QRegExp reg =
        QRegExp(QObject::tr("(?:<<.*|.*<<|<|.*back(?:ward)?.*|.*prev(?:ious)?.*|.*reer.*|.*behind.*|.*before.*)"));

    /*
      [0] > [-1] > [-2]
      [0,0] > [0,-1] > [0,-2]
      [-1] > [-1,0] > [-1,0,0] > [-1,0,-1]
           > [-1,-1] > [-1,-1,0] > [-1,-1,-1] > [-1,-2]

           back, backward, prev, previous, reer, behind, before
     */

    foreach(SharedWebElement elem, elements){
        QString text = elem->InnerText();
        int length = text.length();
        QUrl url = elem->LinkUrl();

        if(reg.exactMatch(text.toLower())){
            GoBackTo(url);
            return;
        }
        QString fragment = url.fragment();
        url.setFragment(QString());

        QUrlQuery query = QUrlQuery(url);
        QStringList body = url.toString(QUrl::RemoveQuery).split(QStringLiteral("/"));
        body.removeFirst();
        QList<int> result;
        result << Compare(basebody, body);
        int delim = result.length();
        result << Compare(basequery, query);

        int max = qMax(currentHighValidityDistance.length(), result.length());

        if(url.toString().startsWith(QStringLiteral("javascript:")))
            goto escape;

        // ignore if all 0.
        for(int i = 0; i < result.length(); i++){
            if(result[i] != 0){
                break;
            } else if(i != result.length() - 1){
                continue;
            } else {
                goto escape;
            }
        }

        for(int i = 0; i < max; i++){
            // current > result
            if(i >= result.length()){
                if(i == result.length() - 1){
                    goto entry;
                } else {
                    continue;
                }
            } else {
                if(i < delim && result[i] > 0)
                    goto escape;
                if(i >= delim &&
                   (result[i] ==   -1 || result[i] ==   -2 ||
                    result[i] ==  -10 || result[i] ==  -20 ||
                    result[i] == -100 || result[i] == -200 ||

                    result[i] ==   -4 || result[i] ==   -5 ||
                    result[i] ==  -40 || result[i] ==  -50 ||
                    result[i] == -400 || result[i] == -500)){

                    if(0 < length && length < 20){
                        if(QRegExp(QStringLiteral("[0-9]+")).exactMatch(text.trimmed())){
                            // url residue or part of query.
                            // restore url.
                            //url.setQuery(query);
                            url.setFragment(fragment);
                            GoBackTo(url);
                            return;
                        } else {
                            goto entry;
                        }
                    } else {
                        // text is too long or empty(image is used?).
                        continue;
                    }
                }
                if(result[i] > 0)
                    goto escape;

                // result > current
                if(i >= currentHighValidityDistance.length()){
                    if(i == currentHighValidityDistance.length() - 1){
                        goto escape;
                    } else {
                        continue;
                    }
                } else {
                    if(result[i] < currentHighValidityDistance[i]){
                        goto escape;
                    } else {
                        continue;
                    }
                }
            }
        }
    entry:
        currentHighValidityElement  = elem;
        currentHighValidityDistance = result;
    escape:
        ;
    }

    if(currentHighValidityElement && !currentHighValidityElement->IsNull()){
        GoBackTo(currentHighValidityElement->LinkUrl());
        return;
    }
    });});
}

void View::GoForwardToInferedUrl(){

    CallWithFoundElements
        (Page::RelIsNext,
         [this](SharedWebElementList nexts){

    if(!nexts.isEmpty() && !nexts[0]->LinkUrl().isEmpty()){
        GoForwardTo(nexts[0]->LinkUrl());
        return;
    }

    CallWithFoundElements
        (Page::HaveReference,
         [this](SharedWebElementList elements){

    QUrl base = url();
    QUrl copy = base;
    base.setFragment(QString()); // ignore fragment.

    QUrlQuery basequery = QUrlQuery(base);
    QStringList basebody = base.toString(QUrl::RemoveQuery).split(QStringLiteral("/"));
    basebody.removeFirst(); // ignore schema;

    SharedWebElement currentHighValidityElement = 0;
    QList<int> currentHighValidityDistance = QList<int>();

    QRegExp reg =
        QRegExp(QObject::tr("(?:>>.*|.*>>|>|.*forward.*|.*next.*|.*front.*|.*beyond.*|.*after.*|.*more.*)"));

    /*
      [0] > [1] > [2]
      [0,0] > [0,1] > [0,2]
      [1] > [1,0] > [1,0,0] > [1,0,1]
          > [1,1] > [1,1,0] > [1,1,1] > [1,2]

          fore, forward, next, front, beyond, after, more
     */

    foreach(SharedWebElement elem, elements){
        QString text = elem->InnerText();
        int length = text.length();
        QUrl url = elem->LinkUrl();

        if(reg.exactMatch(text.toLower())){
            GoForwardTo(url);
            return;
        }
        QString fragment = url.fragment();
        url.setFragment(QString());

        QUrlQuery query = QUrlQuery(url);
        QStringList body = url.toString(QUrl::RemoveQuery).split(QStringLiteral("/"));
        body.removeFirst();
        QList<int> result;
        result << Compare(basebody, body);
        int delim = result.length();
        result << Compare(basequery, query);

        int max = qMax(currentHighValidityDistance.length(), result.length());

        if(url.toString().startsWith(QStringLiteral("javascript:")))
            goto escape;

        // ignore if all 0.
        for(int i = 0; i < result.length(); i++){
            if(result[i] != 0){
                break;
            } else if(i != result.length() - 1){
                continue;
            } else {
                goto escape;
            }
        }

        for(int i = 0; i < max; i++){
            // current > result
            if(i >= result.length()){
                if(i == result.length() - 1){
                    goto entry;
                } else {
                    continue;
                }
            } else {
                if(i < delim && (result[i] < 0 || result[i] == INT_MAX))
                    goto escape;
                if(i >= delim &&
                   (result[i] ==   1 || result[i] ==   2 ||
                    result[i] ==  10 || result[i] ==  20 ||
                    result[i] == 100 || result[i] == 200 ||

                    result[i] ==   4 || result[i] ==   5 ||
                    result[i] ==  40 || result[i] ==  50 ||
                    result[i] == 400 || result[i] == 500)){

                    if(0 < length && length < 20){
                        if(QRegExp(QStringLiteral("[0-9]+")).exactMatch(text.trimmed())){
                            // url residue or part of query.
                            // restore url.
                            //url.setQuery(query);
                            url.setFragment(fragment);
                            GoForwardTo(url);
                            return;
                        } else {
                            goto entry;
                        }
                    } else {
                        // text is too long or empty(image is used?).
                        continue;
                    }
                }
                if(result[i] < 0 || result[i] == INT_MAX)
                    goto escape;

                // result > current
                if(i >= currentHighValidityDistance.length()){
                    if(i == currentHighValidityDistance.length() - 1){
                        goto escape;
                    } else {
                        continue;
                    }
                } else {
                    if(result[i] > currentHighValidityDistance[i]){
                        goto escape;
                    } else {
                        continue;
                    }
                }
            }
        }
    entry:
        currentHighValidityElement  = elem;
        currentHighValidityDistance = result;
    escape:
        ;
    }

    if(currentHighValidityElement && !currentHighValidityElement->IsNull()){
        GoForwardTo(currentHighValidityElement->LinkUrl());
        return;
    }
    });});
}

QMimeData *View::CreateMimeDataFromSelection(NetworkAccessManager *nam){
    // when loading another page, drag and drop cause crash on WebEngineView.
    if(!nam) return 0;

    QMimeData *mime = new QMimeData;

    CallWithGotCurrentBaseUrl([this, nam, mime](QUrl base){
    CallWithSelectedText([this, nam, mime, base](QString text){
    CallWithSelectedHtml([this, nam, mime, base, text](QString html){

    QList<QUrl>        urls = Page::ExtractUrlFromHtml(html, base, Page::HaveReference);
    if(urls.isEmpty()) urls = Page::ExtractUrlFromText(text, base);
    if(urls.isEmpty()) urls = Page::ExtractUrlFromHtml(html, base, Page::HaveSource);

    // mime become untouchable on WebEngineView...
    mime->setText(text);
    mime->setHtml(html);
    mime->setUrls(urls);

    });});});
    return mime;
}

QMimeData *View::CreateMimeDataFromElement(NetworkAccessManager *nam){
    // when loading another page, drag and drop cause crash on WebEngineView.
    if(!nam) return 0;

    QUrl linkUrl  = m_ClickedElement ? m_ClickedElement->LinkUrl()  : QUrl();
    QUrl imageUrl = m_ClickedElement ? m_ClickedElement->ImageUrl() : QUrl();

    if(m_ClickedElement->TagName().toLower() != QStringLiteral("img")) imageUrl = QUrl();

    if(linkUrl.isEmpty() && imageUrl.isEmpty()) return 0;

    QMimeData *mime = new QMimeData;

    QList<QUrl> urls;

    if(!imageUrl.isEmpty()){
        urls << NetworkController::Download(nam, imageUrl, url(), NetworkController::TemporaryDirectory)->GetUrls();
        mime->setText(imageUrl.toString());
        mime->setHtml(m_ClickedElement->ImageHtml());
    }
    // overwrite text and html, if linkUrl is not empty.
    if(!linkUrl.isEmpty()){
        urls << NetworkController::Download(nam, linkUrl, url(), NetworkController::TemporaryDirectory)->GetUrls();
        mime->setText(linkUrl.toString());
        mime->setHtml(m_ClickedElement->LinkHtml());
    }
    mime->setUrls(urls);

    return mime;
}

QPixmap View::CreatePixmapFromSelection(){
    QPixmap pixmap = QPixmap(size());
    pixmap.fill(QColor(255,255,255,0));
    QPainter painter(&pixmap);
    painter.setOpacity(0.5);
    Render(&painter, m_SelectionRegion);
    painter.end();
    pixmap = pixmap.copy(m_SelectionRegion.boundingRect());
    return pixmap;
}

QPixmap View::CreatePixmapFromElement(){
    QPixmap source = m_ClickedElement->Pixmap();
    QPixmap pixmap;

    if(source.isNull()){
        pixmap = QPixmap(size());
        pixmap.fill(QColor(255,255,255,0));
        QPainter painter(&pixmap);
        painter.setOpacity(0.5);
        QRect r;
        QRegion region = m_ClickedElement->Region();
        QRect rect = m_ClickedElement->Rectangle();
        if(!region.isNull()){
            Render(&painter, region);
            r = region.boundingRect();
        } else if(!rect.isNull()){
            Render(&painter, rect);
            r = rect;
        }
        painter.end();
        pixmap = pixmap.copy(r);
    } else {
        pixmap = QPixmap(source.size());
        pixmap.fill(QColor(255,255,255,0));
        QPainter painter(&pixmap);
        painter.setOpacity(0.5);
        painter.drawPixmap(QRect(QPoint(), pixmap.size()), source);
        painter.end();
    }
    return pixmap;
}

QMenu *View::BookmarkletMenu(){
    QMenu *menu = new QMenu(QObject::tr("Bookmarklet"));
    if(Page *page = base()->findChild<Page*>()){
        foreach(QString key, Page::GetBookmarkletMap().keys()){
            menu->addAction(key, page, SLOT(OpenBookmarklet()));
        }
    }
    return menu;
}

QMenu *View::SearchMenu(){
    QMenu *menu = new QMenu(QObject::tr("SearchWith"));
    if(Page *page = base()->findChild<Page*>()){
        foreach(QString key, Page::GetSearchEngineMap().keys()){
            menu->addAction(key, page, SLOT(SearchWith()));
        }
    }
    return menu;
}

QMenu *View::OpenWithOtherBrowserMenu(){
    QMenu *menu = new QMenu(QObject::tr("OpenWithOtherBrowser"));
    if(!Application::BrowserPath_IE().isEmpty())       menu->addAction(Action(Page::We_OpenWithIE));
    if(!Application::BrowserPath_FF().isEmpty())       menu->addAction(Action(Page::We_OpenWithFF));
    if(!Application::BrowserPath_Opera().isEmpty())    menu->addAction(Action(Page::We_OpenWithOpera));
    if(!Application::BrowserPath_OPR().isEmpty())      menu->addAction(Action(Page::We_OpenWithOPR));
    if(!Application::BrowserPath_Safari().isEmpty())   menu->addAction(Action(Page::We_OpenWithSafari));
    if(!Application::BrowserPath_Chrome().isEmpty())   menu->addAction(Action(Page::We_OpenWithChrome));
    if(!Application::BrowserPath_Sleipnir().isEmpty()) menu->addAction(Action(Page::We_OpenWithSleipnir));
    if(!Application::BrowserPath_Vivaldi().isEmpty())  menu->addAction(Action(Page::We_OpenWithVivaldi));
    if(!Application::BrowserPath_Custom().isEmpty())   menu->addAction(Action(Page::We_OpenWithCustom));
    return menu;
}

QMenu *View::OpenLinkWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenLinkWithOtherBrowser"));
    if(!Application::BrowserPath_IE().isEmpty())       menu->addAction(Action(Page::We_OpenLinkWithIE, data));
    if(!Application::BrowserPath_FF().isEmpty())       menu->addAction(Action(Page::We_OpenLinkWithFF, data));
    if(!Application::BrowserPath_Opera().isEmpty())    menu->addAction(Action(Page::We_OpenLinkWithOpera, data));
    if(!Application::BrowserPath_OPR().isEmpty())      menu->addAction(Action(Page::We_OpenLinkWithOPR, data));
    if(!Application::BrowserPath_Safari().isEmpty())   menu->addAction(Action(Page::We_OpenLinkWithSafari, data));
    if(!Application::BrowserPath_Chrome().isEmpty())   menu->addAction(Action(Page::We_OpenLinkWithChrome, data));
    if(!Application::BrowserPath_Sleipnir().isEmpty()) menu->addAction(Action(Page::We_OpenLinkWithSleipnir, data));
    if(!Application::BrowserPath_Vivaldi().isEmpty())  menu->addAction(Action(Page::We_OpenLinkWithVivaldi, data));
    if(!Application::BrowserPath_Custom().isEmpty())   menu->addAction(Action(Page::We_OpenLinkWithCustom, data));
    return menu;
}

QMenu *View::OpenImageWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenImageWithOtherBrowser"));
    if(!Application::BrowserPath_IE().isEmpty())       menu->addAction(Action(Page::We_OpenImageWithIE, data));
    if(!Application::BrowserPath_FF().isEmpty())       menu->addAction(Action(Page::We_OpenImageWithFF, data));
    if(!Application::BrowserPath_Opera().isEmpty())    menu->addAction(Action(Page::We_OpenImageWithOpera, data));
    if(!Application::BrowserPath_OPR().isEmpty())      menu->addAction(Action(Page::We_OpenImageWithOPR, data));
    if(!Application::BrowserPath_Safari().isEmpty())   menu->addAction(Action(Page::We_OpenImageWithSafari, data));
    if(!Application::BrowserPath_Chrome().isEmpty())   menu->addAction(Action(Page::We_OpenImageWithChrome, data));
    if(!Application::BrowserPath_Sleipnir().isEmpty()) menu->addAction(Action(Page::We_OpenImageWithSleipnir, data));
    if(!Application::BrowserPath_Vivaldi().isEmpty())  menu->addAction(Action(Page::We_OpenImageWithVivaldi, data));
    if(!Application::BrowserPath_Custom().isEmpty())   menu->addAction(Action(Page::We_OpenImageWithCustom, data));
    return menu;
}

void View::AddContextMenu(QMenu *menu, SharedWebElement elem){
    QVariant data = QVariant::fromValue(elem);

    QUrl linkUrl  = elem ? elem->LinkUrl()  : QUrl();
    QUrl imageUrl = elem ? elem->ImageUrl() : QUrl();

    static const int max_filename_length_at_contextmenu = 25;
    const QString comma = QStringLiteral(",");
    const QString slash = QStringLiteral("/");

    if(!linkUrl.isEmpty()){
        if(!menu->isEmpty()) menu->addSeparator();
        QStringList list = linkUrl.toString().split(slash);
        QString name = QString();
        while(name.isEmpty() && !list.isEmpty())
            name = list.takeLast();
        if(name.length() > max_filename_length_at_contextmenu)
            name = name.left(max_filename_length_at_contextmenu) + QStringLiteral("...");

        foreach(QString str, View::GetLinkMenu().split(comma)){
            if     (str == QStringLiteral("Separator")) menu->addSeparator();
            else if(str == QStringLiteral("LinkUrl"))   menu->addAction(QStringLiteral("(") + name + QStringLiteral(")"));
            else if(str == QStringLiteral("OpenLinkWithOtherBrowser"))
                menu->addMenu(OpenLinkWithOtherBrowserMenu(data));
            else menu->addAction(Action(Page::StringToAction(str), data));
        }
    }

    if(!imageUrl.isEmpty()){
        if(!menu->isEmpty()) menu->addSeparator();
        QStringList list = imageUrl.toString().split(slash);
        QString name = QString();
        while(name.isEmpty() && !list.isEmpty())
            name = list.takeLast();
        if(name.length() > max_filename_length_at_contextmenu)
            name = name.left(max_filename_length_at_contextmenu) + QStringLiteral("...");

        foreach(QString str, View::GetImageMenu().split(comma)){
            if     (str == QStringLiteral("Separator")) menu->addSeparator();
            else if(str == QStringLiteral("ImageUrl"))  menu->addAction(QStringLiteral("(") + name + QStringLiteral(")"));
            else if(str == QStringLiteral("OpenImageWithOtherBrowser"))
                menu->addMenu(OpenImageWithOtherBrowserMenu(data));
            else menu->addAction(Action(Page::StringToAction(str), data));
        }
    }

    if(!SelectedText().isEmpty()){
        if(!menu->isEmpty()) menu->addSeparator();

        foreach(QString str, View::GetSelectionMenu().split(comma)){
            if     (str == QStringLiteral("Separator"))  menu->addSeparator();
            else if(str == QStringLiteral("SearchMenu")) menu->addMenu(SearchMenu());
            else menu->addAction(Action(Page::StringToAction(str), data));
        }
    }
}

void View::AddRegularMenu(QMenu *menu, SharedWebElement elem){
    QVariant data = QVariant::fromValue(elem);

    const QString comma = QStringLiteral(",");
    const QString slash = QStringLiteral("/");

    if(!menu->isEmpty()) menu->addSeparator();

    foreach(QString str, View::GetRegularMenu().split(comma)){
        if     (str == QStringLiteral("Separator"))       menu->addSeparator();
        else if(str == QStringLiteral("BookmarkletMenu")) menu->addMenu(BookmarkletMenu());
        else if(str == QStringLiteral("OpenWithOtherBrowser"))
            menu->addMenu(OpenWithOtherBrowserMenu());
        else if(str == QStringLiteral("ViewOrApplySource")){
            if(GetHistNode()->GetUrl().toEncoded().startsWith("view-source:")){
                menu->addAction(Action(Page::We_ApplySource));
            } else {
                menu->addAction(Action(Page::We_ViewSource));
            }
        } else menu->addAction(Action(Page::StringToAction(str), data));
    }
}

void View::LoadSettings(){
    QSettings *settings = Application::GlobalSettings();

#ifdef QTWEBKIT
    // GlobalWebSettings
    QWebSettings *gws = QWebSettings::globalSettings();
#endif
    // GlobalWebEngineSettings
    QWebEngineSettings *gwes = QWebEngineSettings::globalSettings();

    settings->beginGroup(QStringLiteral("webview"));{
        m_GestureMode          = settings->value(QStringLiteral("@GestureMode"),           4).value<int>();
        m_EnableLoadHack       = settings->value(QStringLiteral("@EnableLoadHack"),    false).value<bool>()
            ||                   settings->value(QStringLiteral("@EnableHistNode"),    false).value<bool>();
        m_EnableDragHack       = settings->value(QStringLiteral("@EnableDragHack"),    false).value<bool>()
            ||                   settings->value(QStringLiteral("@EnableDragGesture"), false).value<bool>();
        m_ActivateNewViewDefault = settings->value(QStringLiteral("@ActivateNewViewDefault"), true).value<bool>();
        m_NavigationBySpaceKey = settings->value(QStringLiteral("@NavigationBySpaceKey"), false).value<bool>();

        QString operation = settings->value(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewViewNode")).value<QString>();
        if(operation == QStringLiteral("InNewViewNode"))  Page::SetOpenCommandOperation(Page::InNewViewNode);
        if(operation == QStringLiteral("InNewHistNode"))  Page::SetOpenCommandOperation(Page::InNewHistNode);
        if(operation == QStringLiteral("InNewDirectory")) Page::SetOpenCommandOperation(Page::InNewDirectory);
        if(operation == QStringLiteral("OnRoot"))         Page::SetOpenCommandOperation(Page::OnRoot);

        if(operation == QStringLiteral("InNewViewNodeBackground"))  Page::SetOpenCommandOperation(Page::InNewViewNodeBackground);
        if(operation == QStringLiteral("InNewHistNodeBackground"))  Page::SetOpenCommandOperation(Page::InNewHistNodeBackground);
        if(operation == QStringLiteral("InNewDirectoryBackground")) Page::SetOpenCommandOperation(Page::InNewDirectoryBackground);
        if(operation == QStringLiteral("OnRootBackground"))         Page::SetOpenCommandOperation(Page::OnRootBackground);

        if(operation == QStringLiteral("InNewViewNodeNewWindow"))  Page::SetOpenCommandOperation(Page::InNewViewNodeNewWindow);
        if(operation == QStringLiteral("InNewHistNodeNewWindow"))  Page::SetOpenCommandOperation(Page::InNewHistNodeNewWindow);
        if(operation == QStringLiteral("InNewDirectoryNewWindow")) Page::SetOpenCommandOperation(Page::InNewDirectoryNewWindow);
        if(operation == QStringLiteral("OnRootNewWindow"))         Page::SetOpenCommandOperation(Page::OnRootNewWindow);

        settings->beginGroup(QStringLiteral("preferences"));{
#ifdef QTWEBKIT
            gws->setAttribute(QWebSettings::AcceleratedCompositingEnabled,     settings->value(QStringLiteral("AcceleratedCompositingEnabled"),     gws->testAttribute(QWebSettings::AcceleratedCompositingEnabled)          ).value<bool>());
            gws->setAttribute(QWebSettings::AutoLoadImages,                    settings->value(QStringLiteral("AutoLoadImages"),                    gws->testAttribute(QWebSettings::AutoLoadImages)                         ).value<bool>());
            gws->setAttribute(QWebSettings::DeveloperExtrasEnabled,            settings->value(QStringLiteral("DeveloperExtrasEnabled"),          /*gws->testAttribute(QWebSettings::DeveloperExtrasEnabled)*/          true ).value<bool>());
            gws->setAttribute(QWebSettings::DnsPrefetchEnabled,                settings->value(QStringLiteral("DnsPrefetchEnabled"),                gws->testAttribute(QWebSettings::DnsPrefetchEnabled)                     ).value<bool>());
            gws->setAttribute(QWebSettings::FrameFlatteningEnabled,            settings->value(QStringLiteral("FrameFlatteningEnabled"),            gws->testAttribute(QWebSettings::FrameFlatteningEnabled)                 ).value<bool>());
            gws->setAttribute(QWebSettings::JavaEnabled,                       settings->value(QStringLiteral("JavaEnabled"),                       gws->testAttribute(QWebSettings::JavaEnabled)                            ).value<bool>());
            gws->setAttribute(QWebSettings::JavascriptCanAccessClipboard,      settings->value(QStringLiteral("JavascriptCanAccessClipboard"),      gws->testAttribute(QWebSettings::JavascriptCanAccessClipboard)           ).value<bool>());
            gws->setAttribute(QWebSettings::JavascriptCanCloseWindows,         settings->value(QStringLiteral("JavascriptCanCloseWindows"),       /*gws->testAttribute(QWebSettings::JavascriptCanCloseWindows)*/       true ).value<bool>());
            gws->setAttribute(QWebSettings::JavascriptCanOpenWindows,          settings->value(QStringLiteral("JavascriptCanOpenWindows"),        /*gws->testAttribute(QWebSettings::JavascriptCanOpenWindows)*/        true ).value<bool>());
            gws->setAttribute(QWebSettings::JavascriptEnabled,                 settings->value(QStringLiteral("JavascriptEnabled"),                 gws->testAttribute(QWebSettings::JavascriptEnabled)                      ).value<bool>());
            gws->setAttribute(QWebSettings::LinksIncludedInFocusChain,         settings->value(QStringLiteral("LinksIncludedInFocusChain"),         gws->testAttribute(QWebSettings::LinksIncludedInFocusChain)              ).value<bool>());
            gws->setAttribute(QWebSettings::LocalContentCanAccessFileUrls,     settings->value(QStringLiteral("LocalContentCanAccessFileUrls"),     gws->testAttribute(QWebSettings::LocalContentCanAccessFileUrls)          ).value<bool>());
            gws->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls,   settings->value(QStringLiteral("LocalContentCanAccessRemoteUrls"),   gws->testAttribute(QWebSettings::LocalContentCanAccessRemoteUrls)        ).value<bool>());
          //gws->setAttribute(QWebSettings::LocalStorageDatabaseEnabled,       settings->value(QStringLiteral("LocalStorageDatabaseEnabled"),       gws->testAttribute(QWebSettings::LocalStorageDatabaseEnabled)            ).value<bool>());
            gws->setAttribute(QWebSettings::LocalStorageEnabled,               settings->value(QStringLiteral("LocalStorageEnabled"),               gws->testAttribute(QWebSettings::LocalStorageEnabled)                    ).value<bool>());
            gws->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled,     settings->value(QStringLiteral("OfflineStorageDatabaseEnabled"),     gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled)          ).value<bool>());
            gws->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, settings->value(QStringLiteral("OfflineWebApplicationCacheEnabled"), gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled)      ).value<bool>());
            gws->setAttribute(QWebSettings::PluginsEnabled,                    settings->value(QStringLiteral("PluginsEnabled"),                  /*gws->testAttribute(QWebSettings::PluginsEnabled)*/                  true ).value<bool>());
            gws->setAttribute(QWebSettings::PrintElementBackgrounds,           settings->value(QStringLiteral("PrintElementBackgrounds"),           gws->testAttribute(QWebSettings::PrintElementBackgrounds)                ).value<bool>());
            gws->setAttribute(QWebSettings::PrivateBrowsingEnabled,            settings->value(QStringLiteral("PrivateBrowsingEnabled"),            gws->testAttribute(QWebSettings::PrivateBrowsingEnabled)                 ).value<bool>());
            gws->setAttribute(QWebSettings::SiteSpecificQuirksEnabled,         settings->value(QStringLiteral("SiteSpecificQuirksEnabled"),         gws->testAttribute(QWebSettings::SiteSpecificQuirksEnabled)              ).value<bool>());
            gws->setAttribute(QWebSettings::SpatialNavigationEnabled,          settings->value(QStringLiteral("SpatialNavigationEnabled"),          gws->testAttribute(QWebSettings::SpatialNavigationEnabled)               ).value<bool>());
            gws->setAttribute(QWebSettings::TiledBackingStoreEnabled,          settings->value(QStringLiteral("TiledBackingStoreEnabled"),          gws->testAttribute(QWebSettings::TiledBackingStoreEnabled)               ).value<bool>());
            gws->setAttribute(QWebSettings::XSSAuditingEnabled,                settings->value(QStringLiteral("XSSAuditingEnabled"),                gws->testAttribute(QWebSettings::XSSAuditingEnabled)                     ).value<bool>());
            gws->setAttribute(QWebSettings::ZoomTextOnly,                      settings->value(QStringLiteral("ZoomTextOnly"),                      gws->testAttribute(QWebSettings::ZoomTextOnly)                           ).value<bool>());
            gws->setAttribute(QWebSettings::CSSGridLayoutEnabled,              settings->value(QStringLiteral("CSSGridLayoutEnabled"),              gws->testAttribute(QWebSettings::CSSGridLayoutEnabled)                   ).value<bool>());
            gws->setAttribute(QWebSettings::CSSRegionsEnabled,                 settings->value(QStringLiteral("CSSRegionsEnabled"),                 gws->testAttribute(QWebSettings::CSSRegionsEnabled)                      ).value<bool>());
            gws->setAttribute(QWebSettings::CaretBrowsingEnabled,              settings->value(QStringLiteral("CaretBrowsingEnabled"),              gws->testAttribute(QWebSettings::CaretBrowsingEnabled)                   ).value<bool>());
            gws->setAttribute(QWebSettings::HyperlinkAuditingEnabled,          settings->value(QStringLiteral("HyperlinkAuditingEnabled"),          gws->testAttribute(QWebSettings::HyperlinkAuditingEnabled)               ).value<bool>());
            gws->setAttribute(QWebSettings::NotificationsEnabled,              settings->value(QStringLiteral("NotificationsEnabled"),              gws->testAttribute(QWebSettings::NotificationsEnabled)                   ).value<bool>());
            gws->setAttribute(QWebSettings::ScrollAnimatorEnabled,             settings->value(QStringLiteral("ScrollAnimatorEnabled"),             gws->testAttribute(QWebSettings::ScrollAnimatorEnabled)                  ).value<bool>());
            gws->setAttribute(QWebSettings::WebAudioEnabled,                   settings->value(QStringLiteral("WebAudioEnabled"),                   gws->testAttribute(QWebSettings::WebAudioEnabled)                        ).value<bool>());
            gws->setAttribute(QWebSettings::WebGLEnabled,                      settings->value(QStringLiteral("WebGLEnabled"),                      gws->testAttribute(QWebSettings::WebGLEnabled)                           ).value<bool>());
            gws->setAttribute(QWebSettings::Accelerated2dCanvasEnabled,        settings->value(QStringLiteral("Accelerated2dCanvasEnabled"),        gws->testAttribute(QWebSettings::Accelerated2dCanvasEnabled)             ).value<bool>());
          //gws->setAttribute(QWebSettings::ErrorPageEnabled,                  settings->value(QStringLiteral("ErrorPageEnabled"),                  gws->testAttribute(QWebSettings::ErrorPageEnabled)                       ).value<bool>());
          //gws->setAttribute(QWebSettings::FullScreenSupportEnabled,          settings->value(QStringLiteral("FullScreenSupportEnabled"),          gws->testAttribute(QWebSettings::FullScreenSupportEnabled)               ).value<bool>());
#endif
          //gwes->setAttribute(QWebEngineSettings::AcceleratedCompositingEnabled,     settings->value(QStringLiteral("AcceleratedCompositingEnabled"),     gwes->testAttribute(QWebEngineSettings::AcceleratedCompositingEnabled)    ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::AutoLoadImages,                    settings->value(QStringLiteral("AutoLoadImages"),                    gwes->testAttribute(QWebEngineSettings::AutoLoadImages)                   ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled,            settings->value(QStringLiteral("DeveloperExtrasEnabled"),            gwes->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled)           ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,                settings->value(QStringLiteral("DnsPrefetchEnabled"),                gwes->testAttribute(QWebEngineSettings::DnsPrefetchEnabled)               ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::FrameFlatteningEnabled,            settings->value(QStringLiteral("FrameFlatteningEnabled"),            gwes->testAttribute(QWebEngineSettings::FrameFlatteningEnabled)           ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::JavaEnabled,                       settings->value(QStringLiteral("JavaEnabled"),                       gwes->testAttribute(QWebEngineSettings::JavaEnabled)                      ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard,      settings->value(QStringLiteral("JavascriptCanAccessClipboard"),      gwes->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard)     ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::JavascriptCanCloseWindows,         settings->value(QStringLiteral("JavascriptCanCloseWindows"),         gwes->testAttribute(QWebEngineSettings::JavascriptCanCloseWindows)        ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,          settings->value(QStringLiteral("JavascriptCanOpenWindows"),          gwes->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows)         ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::JavascriptEnabled,                 settings->value(QStringLiteral("JavascriptEnabled"),                 gwes->testAttribute(QWebEngineSettings::JavascriptEnabled)                ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain,         settings->value(QStringLiteral("LinksIncludedInFocusChain"),         gwes->testAttribute(QWebEngineSettings::LinksIncludedInFocusChain)        ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,     settings->value(QStringLiteral("LocalContentCanAccessFileUrls"),     gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls)    ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,   settings->value(QStringLiteral("LocalContentCanAccessRemoteUrls"),   gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls)  ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled,       settings->value(QStringLiteral("LocalStorageDatabaseEnabled"),       gwes->testAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled)      ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::LocalStorageEnabled,               settings->value(QStringLiteral("LocalStorageEnabled"),               gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled)              ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled,     settings->value(QStringLiteral("OfflineStorageDatabaseEnabled"),     gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled)    ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled, settings->value(QStringLiteral("OfflineWebApplicationCacheEnabled"), gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled)).value<bool>());
#if QT_VERSION >= 0x050600
            gwes->setAttribute(QWebEngineSettings::PluginsEnabled,                    settings->value(QStringLiteral("PluginsEnabled"),                    gwes->testAttribute(QWebEngineSettings::PluginsEnabled)                   ).value<bool>());
#endif
          //gwes->setAttribute(QWebEngineSettings::PrintElementBackgrounds,           settings->value(QStringLiteral("PrintElementBackgrounds"),           gwes->testAttribute(QWebEngineSettings::PrintElementBackgrounds)          ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::PrivateBrowsingEnabled,            settings->value(QStringLiteral("PrivateBrowsingEnabled"),            gwes->testAttribute(QWebEngineSettings::PrivateBrowsingEnabled)           ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::SiteSpecificQuirksEnabled,         settings->value(QStringLiteral("SiteSpecificQuirksEnabled"),         gwes->testAttribute(QWebEngineSettings::SiteSpecificQuirksEnabled)        ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::SpatialNavigationEnabled,          settings->value(QStringLiteral("SpatialNavigationEnabled"),          gwes->testAttribute(QWebEngineSettings::SpatialNavigationEnabled)         ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::TiledBackingStoreEnabled,          settings->value(QStringLiteral("TiledBackingStoreEnabled"),          gwes->testAttribute(QWebEngineSettings::TiledBackingStoreEnabled)         ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::XSSAuditingEnabled,                settings->value(QStringLiteral("XSSAuditingEnabled"),                gwes->testAttribute(QWebEngineSettings::XSSAuditingEnabled)               ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::ZoomTextOnly,                      settings->value(QStringLiteral("ZoomTextOnly"),                      gwes->testAttribute(QWebEngineSettings::ZoomTextOnly)                     ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::CSSGridLayoutEnabled,              settings->value(QStringLiteral("CSSGridLayoutEnabled"),              gwes->testAttribute(QWebEngineSettings::CSSGridLayoutEnabled)             ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::CSSRegionsEnabled,                 settings->value(QStringLiteral("CSSRegionsEnabled"),                 gwes->testAttribute(QWebEngineSettings::CSSRegionsEnabled)                ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::CaretBrowsingEnabled,              settings->value(QStringLiteral("CaretBrowsingEnabled"),              gwes->testAttribute(QWebEngineSettings::CaretBrowsingEnabled)             ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled,          settings->value(QStringLiteral("HyperlinkAuditingEnabled"),          gwes->testAttribute(QWebEngineSettings::HyperlinkAuditingEnabled)         ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::NotificationsEnabled,              settings->value(QStringLiteral("NotificationsEnabled"),              gwes->testAttribute(QWebEngineSettings::NotificationsEnabled)             ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,             settings->value(QStringLiteral("ScrollAnimatorEnabled"),             gwes->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled)            ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::WebAudioEnabled,                   settings->value(QStringLiteral("WebAudioEnabled"),                   gwes->testAttribute(QWebEngineSettings::WebAudioEnabled)                  ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::WebGLEnabled,                      settings->value(QStringLiteral("WebGLEnabled"),                      gwes->testAttribute(QWebEngineSettings::WebGLEnabled)                     ).value<bool>());
          //gwes->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,        settings->value(QStringLiteral("Accelerated2dCanvasEnabled"),        gwes->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled)       ).value<bool>());
            gwes->setAttribute(QWebEngineSettings::ErrorPageEnabled,                  settings->value(QStringLiteral("ErrorPageEnabled"),                  gwes->testAttribute(QWebEngineSettings::ErrorPageEnabled)                 ).value<bool>());
#if QT_VERSION >= 0x050600
            gwes->setAttribute(QWebEngineSettings::FullScreenSupportEnabled,          settings->value(QStringLiteral("FullScreenSupportEnabled"),          gwes->testAttribute(QWebEngineSettings::FullScreenSupportEnabled)         ).value<bool>());
#endif
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("detail"));{

#ifdef QTWEBKIT
            if(gws->testAttribute(QWebSettings::LocalStorageEnabled) ||
               gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled) ||
               gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled)){

                QWebSettings::enablePersistentStorage();
            }

            gws->setDefaultTextEncoding
                (settings->value(QStringLiteral("DefaultTextEncoding"),
                                 gws->defaultTextEncoding()).value<QString>());

            gws->setMaximumPagesInCache
                (settings->value(QStringLiteral("MaximumPagesInCache"),
                                 gws->maximumPagesInCache()).value<int>());

            if(gws->testAttribute(QWebSettings::LocalStorageEnabled)){
                QString iconDatabasePath =
                    settings->value(QStringLiteral("IconDatabasePath"),
                                    gws->iconDatabasePath()).value<QString>();
                QString localStoragePath =
                    settings->value(QStringLiteral("LocalStoragePath"),
                                    gws->localStoragePath()).value<QString>();
                if(!iconDatabasePath.isEmpty()){
                    // setting non-default value to crash...

                    // QWebSettings::iconDatabasePath returns path with file name.
                    // QWebSettings::setIconDatabasePath expects path without file name.
                    //QStringList list = iconDatabasePath.split(QDir::separator());
                    //list.removeLast();
                    //QString path = list.join(QDir::separator());
                    //QDir dir = path;
                    //dir.mkpath(path);
                    //gws->setIconDatabasePath(path);

                    //gws->setIconDatabasePath(iconDatabasePath);
                }
                if(!localStoragePath.isEmpty()){
                    QDir dir = localStoragePath;
                    dir.mkpath(localStoragePath);
                    gws->setLocalStoragePath(localStoragePath);
                }
            }

            if(gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled)){
                QString path =
                    settings->value(QStringLiteral("OfflineStoragePath"),
                                    gws->offlineStoragePath()).value<QString>();
                if(!path.isEmpty()){
                    QDir dir = path;
                    dir.mkpath(path);
                    gws->setOfflineStoragePath(path);
                }
                gws->setOfflineStorageDefaultQuota
                    (settings->value(QStringLiteral("OfflineStorageDefaultQuota"),
                                     gws->offlineStorageDefaultQuota()).value<qint64>());
            }

            if(gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled)){
                QString path =
                    settings->value(QStringLiteral("OfflineWebApplicationCachePath"),
                                    gws->offlineWebApplicationCachePath()).value<QString>();
                if(!path.isEmpty()){
                    QDir dir = path;
                    dir.mkpath(path);
                    // setting value to crash...
                    //gws->setOfflineWebApplicationCachePath(path);
                }
                gws->setOfflineWebApplicationCacheQuota
                    (settings->value(QStringLiteral("OfflineWebApplicationCacheQuota"),
                                     gws->offlineWebApplicationCacheQuota()).value<qint64>());
            }

            gws->setUserStyleSheetUrl
                (settings->value(QStringLiteral("UserStyleSheetUrl"),
                                 gws->userStyleSheetUrl()).value<QUrl>());
#endif
            //if(gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled) ||
            //   gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled) ||
            //   gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled)){
            //
            //    QWebEngineSettings::enablePersistentStorage();
            //}

            gwes->setDefaultTextEncoding
                (settings->value(QStringLiteral("DefaultTextEncoding"),
                                 gwes->defaultTextEncoding()).value<QString>());

            //gwes->setMaximumPagesInCache
            //    (settings->value(QStringLiteral("MaximumPagesInCache"),
            //                     gwes->maximumPagesInCache()).value<int>());
            //
            //if(gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled)){
            //    QString iconDatabasePath =
            //        settings->value(QStringLiteral("IconDatabasePath"),
            //                        gwes->iconDatabasePath()).value<QString>();
            //    QString localStoragePath =
            //        settings->value(QStringLiteral("LocalStoragePath"),
            //                        gwes->localStoragePath()).value<QString>();
            //    if(!iconDatabasePath.isEmpty()){
            //        gwes->setIconDatabasePath(iconDatabasePath);
            //    }
            //    if(!localStoragePath.isEmpty()){
            //        QDir dir = localStoragePath;
            //        dir.mkpath(localStoragePath);
            //        gwes->setLocalStoragePath(localStoragePath);
            //    }
            //}
            //
            //if(gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled)){
            //    QString path =
            //        settings->value(QStringLiteral("OfflineStoragePath"),
            //                        gwes->offlineStoragePath()).value<QString>();
            //    if(!path.isEmpty()){
            //        QDir dir = path;
            //        dir.mkpath(path);
            //        gwes->setOfflineStoragePath(path);
            //    }
            //    gwes->setOfflineStorageDefaultQuota
            //        (settings->value(QStringLiteral("OfflineStorageDefaultQuota"),
            //                         gwes->offlineStorageDefaultQuota()).value<qint64>());
            //}
            //
            //if(gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled)){
            //    QString path =
            //        settings->value(QStringLiteral("OfflineWebApplicationCachePath"),
            //                        gwes->offlineWebApplicationCachePath()).value<QString>();
            //    if(!path.isEmpty()){
            //        QDir dir = path;
            //        dir.mkpath(path);
            //        gwes->setOfflineWebApplicationCachePath(path);
            //    }
            //    gwes->setOfflineWebApplicationCacheQuota
            //        (settings->value(QStringLiteral("OfflineWebApplicationCacheQuota"),
            //                         gwes->offlineWebApplicationCacheQuota()).value<qint64>());
            //}
            //
            //gwes->setUserStyleSheetUrl
            //    (settings->value(QStringLiteral("UserStyleSheetUrl"),
            //                     gwes->userStyleSheetUrl()).value<QUrl>());

            QString policy = settings->value(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AlwaysAllowThirdPartyCookies")).value<QString>();
#ifdef QTWEBKIT
            if(policy == QStringLiteral("AlwaysAllowThirdPartyCookies"))
                gws->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
            if(policy == QStringLiteral("AlwaysBlockThirdPartyCookies"))
                gws->setThirdPartyCookiePolicy(QWebSettings::AlwaysBlockThirdPartyCookies);
            if(policy == QStringLiteral("AllowThirdPartyWithExistingCookies"))
                gws->setThirdPartyCookiePolicy(QWebSettings::AllowThirdPartyWithExistingCookies);
#endif
            //if(policy == QStringLiteral("AlwaysAllowThirdPartyCookies"))
            //    gwes->setThirdPartyCookiePolicy(QWebEngineSettings::AlwaysAllowThirdPartyCookies);
            //if(policy == QStringLiteral("AlwaysBlockThirdPartyCookies"))
            //    gwes->setThirdPartyCookiePolicy(QWebEngineSettings::AlwaysBlockThirdPartyCookies);
            //if(policy == QStringLiteral("AllowThirdPartyWithExistingCookies"))
            //    gwes->setThirdPartyCookiePolicy(QWebEngineSettings::AllowThirdPartyWithExistingCookies);
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("font"));{
#ifdef QTWEBKIT
            gws->setFontFamily(QWebSettings::StandardFont,         settings->value(QStringLiteral("StandardFont"),           gws->fontFamily(QWebSettings::StandardFont)         ).value<QString>());
            gws->setFontFamily(QWebSettings::FixedFont,            settings->value(QStringLiteral("FixedFont"),              gws->fontFamily(QWebSettings::FixedFont)            ).value<QString>());
            gws->setFontFamily(QWebSettings::SerifFont,            settings->value(QStringLiteral("SerifFont"),              gws->fontFamily(QWebSettings::SerifFont)            ).value<QString>());
            gws->setFontFamily(QWebSettings::SansSerifFont,        settings->value(QStringLiteral("SansSerifFont"),          gws->fontFamily(QWebSettings::SansSerifFont)        ).value<QString>());
            gws->setFontFamily(QWebSettings::CursiveFont,          settings->value(QStringLiteral("CursiveFont"),            gws->fontFamily(QWebSettings::CursiveFont)          ).value<QString>());
            gws->setFontFamily(QWebSettings::FantasyFont,          settings->value(QStringLiteral("FantasyFont"),            gws->fontFamily(QWebSettings::FantasyFont)          ).value<QString>());
            gws->setFontSize(QWebSettings::MinimumFontSize,        settings->value(QStringLiteral("MinimumFontSize"),        gws->fontSize(QWebSettings::MinimumFontSize)        ).value<int>());
            gws->setFontSize(QWebSettings::MinimumLogicalFontSize, settings->value(QStringLiteral("MinimumLogicalFontSize"), gws->fontSize(QWebSettings::MinimumLogicalFontSize) ).value<int>());
            gws->setFontSize(QWebSettings::DefaultFontSize,        settings->value(QStringLiteral("DefaultFontSize"),        gws->fontSize(QWebSettings::DefaultFontSize)        ).value<int>());
            gws->setFontSize(QWebSettings::DefaultFixedFontSize,   settings->value(QStringLiteral("DefaultFixedFontSize"),   gws->fontSize(QWebSettings::DefaultFixedFontSize)   ).value<int>());
#endif
            gwes->setFontFamily(QWebEngineSettings::StandardFont,         settings->value(QStringLiteral("StandardFont"),           gwes->fontFamily(QWebEngineSettings::StandardFont)         ).value<QString>());
            gwes->setFontFamily(QWebEngineSettings::FixedFont,            settings->value(QStringLiteral("FixedFont"),              gwes->fontFamily(QWebEngineSettings::FixedFont)            ).value<QString>());
            gwes->setFontFamily(QWebEngineSettings::SerifFont,            settings->value(QStringLiteral("SerifFont"),              gwes->fontFamily(QWebEngineSettings::SerifFont)            ).value<QString>());
            gwes->setFontFamily(QWebEngineSettings::SansSerifFont,        settings->value(QStringLiteral("SansSerifFont"),          gwes->fontFamily(QWebEngineSettings::SansSerifFont)        ).value<QString>());
            gwes->setFontFamily(QWebEngineSettings::CursiveFont,          settings->value(QStringLiteral("CursiveFont"),            gwes->fontFamily(QWebEngineSettings::CursiveFont)          ).value<QString>());
            gwes->setFontFamily(QWebEngineSettings::FantasyFont,          settings->value(QStringLiteral("FantasyFont"),            gwes->fontFamily(QWebEngineSettings::FantasyFont)          ).value<QString>());
            gwes->setFontSize(QWebEngineSettings::MinimumFontSize,        settings->value(QStringLiteral("MinimumFontSize"),        gwes->fontSize(QWebEngineSettings::MinimumFontSize)        ).value<int>());
            gwes->setFontSize(QWebEngineSettings::MinimumLogicalFontSize, settings->value(QStringLiteral("MinimumLogicalFontSize"), gwes->fontSize(QWebEngineSettings::MinimumLogicalFontSize) ).value<int>());
            gwes->setFontSize(QWebEngineSettings::DefaultFontSize,        settings->value(QStringLiteral("DefaultFontSize"),        gwes->fontSize(QWebEngineSettings::DefaultFontSize)        ).value<int>());
            gwes->setFontSize(QWebEngineSettings::DefaultFixedFontSize,   settings->value(QStringLiteral("DefaultFixedFontSize"),   gwes->fontSize(QWebEngineSettings::DefaultFixedFontSize)   ).value<int>());
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("menu"));{
            m_LinkMenu = settings->value(QStringLiteral("LinkMenu"), QStringLiteral(
                "LinkUrl,"
              VV"OpenInNewViewNode,"
              VV"OpenInNewHistNode,"
              VV"OpenInNewDirectory,"
              VV"OpenOnRoot,"
              VV"Separator,"
              VV"OpenLink,"
              VV"DownloadLink,"
              VV"CopyLinkUrl,"
              VV"CopyLinkHtml,"
              VV"OpenLinkWithOtherBrowser")).value<QString>();

            m_ImageMenu = settings->value(QStringLiteral("ImageMenu"), QStringLiteral(
                "ImageUrl,"
              VV"OpenImageInNewViewNode,"
              VV"OpenImageInNewHistNode,"
              VV"OpenImageInNewDirectory,"
              VV"OpenImageOnRoot,"
              VV"Separator,"
              VV"OpenImage,"
              VV"DownloadImage,"
              VV"CopyImage,"
              VV"CopyImageUrl,"
              VV"CopyImageHtml,"
              VV"OpenImageWithOtherBrowser")).value<QString>();

            m_SelectionMenu = settings->value(QStringLiteral("SelectionMenu"), QStringLiteral(
                "Copy,"
              VV"CopySelectedHtml,"
              VV"Separator,"
              VV"SearchMenu,"
              VV"OpenAllUrl,"
              VV"SaveAllUrl,"
              VV"OpenAllImage,"
              VV"SaveAllImage,"
              VV"OpenTextAsUrl,"
              VV"SaveTextAsUrl")).value<QString>();

            m_RegularMenu = settings->value(QStringLiteral("RegularMenu"), QStringLiteral(
                "AddBookmarklet,"
              VV"BookmarkletMenu,"
              VV"Separator,"
              VV"NewViewNode,"
              VV"NewHistNode,"
              VV"CloneViewNode,"
              VV"CloneHistNode,"
              VV"CopyUrl,"
              VV"CopyPageAsLink,"
              VV"OpenWithOtherBrowser,"
              VV"ViewOrApplySource")).value<QString>();
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("keymap"));{
            QStringList keys = settings->allKeys();
            if(keys.isEmpty()){
                /* default key map. */
                WEBVIEW_KEYMAP
            } else {
                if(!m_KeyMap.isEmpty()) m_KeyMap.clear();
                foreach(QString key, keys){
                    if(key.isEmpty()) continue;
                    m_KeyMap[Application::MakeKeySequence(key)] =
                        settings->value(key, QStringLiteral("NoAction")).value<QString>()
                        // cannot use slashes on QSettings.
                          .replace(QStringLiteral("Backslash"), QStringLiteral("\\"))
                          .replace(QStringLiteral("Slash"), QStringLiteral("/"));
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("mouse"));{
            QStringList keys = settings->allKeys();
            if(keys.isEmpty()){
                /* default mouse map. */
                WEBVIEW_MOUSEMAP
            } else {
                if(!m_MouseMap.isEmpty()) m_MouseMap.clear();
                foreach(QString key, keys){
                    if(key.isEmpty()) continue;
                    m_MouseMap[key] =
                        settings->value(key, QStringLiteral("NoAction")).value<QString>();
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("rightgesture"));{
            QStringList keys = settings->allKeys();
            if(keys.isEmpty()){
                /* default right gesture. */
                WEBVIEW_RIGHTGESTURE
            } else {
                if(!m_RightGestureMap.isEmpty()) m_RightGestureMap.clear();
                foreach(QString key, keys){
                    if(key.isEmpty()) continue;
                    m_RightGestureMap[key] = settings->value(key, QStringLiteral("NoAction")).value<QString>();
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("leftgesture"));{
            QStringList keys = settings->allKeys();
            if(keys.isEmpty()){
                /* default left gesture. */
                WEBVIEW_LEFTGESTURE
            } else {
                if(!m_LeftGestureMap.isEmpty()) m_LeftGestureMap.clear();
                foreach(QString key, keys){
                    if(key.isEmpty()) continue;
                    m_LeftGestureMap[key] = settings->value(key, QStringLiteral("NoAction")).value<QString>();
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("searchengine"));{
            if(!Page::GetSearchEngineMap().isEmpty()) Page::ClearSearchEngine();
            foreach(QString key, settings->allKeys()){
                QVariant searchengine = settings->value(key);
                if(searchengine.canConvert(QVariant::String))
                    Page::RegisterSearchEngine(key, QStringList() << searchengine.value<QString>());
                if(searchengine.canConvert(QVariant::StringList))
                    Page::RegisterSearchEngine(key, searchengine.value<QStringList>());
            }
            if(Page::GetSearchEngineMap().isEmpty()){
                Page::RegisterDefaultSearchEngines();
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("bookmarklet"));{
            if(!Page::GetBookmarkletMap().isEmpty()) Page::ClearBookmarklet();
            foreach(QString key, settings->allKeys()){
                QVariant bookmarklet = settings->value(key);
                if(bookmarklet.canConvert(QVariant::String))
                    Page::RegisterBookmarklet(key, QStringList() << bookmarklet.value<QString>());
                if(bookmarklet.canConvert(QVariant::StringList))
                    Page::RegisterBookmarklet(key, bookmarklet.value<QStringList>());
            }
        }
        settings->endGroup();
    }
    settings->endGroup();
}

void View::SaveSettings(){
    QSettings *settings = Application::GlobalSettings();

#ifdef QTWEBKIT
    // GlobalWebSettings
    QWebSettings *gws = QWebSettings::globalSettings();
#endif
    // GlobalWebEngineSettings
    QWebEngineSettings *gwes = QWebEngineSettings::globalSettings();

    settings->beginGroup(QStringLiteral("webview"));{
        settings->setValue(QStringLiteral("@GestureMode"),          m_GestureMode);
        settings->setValue(QStringLiteral("@EnableHistNode"),       m_EnableLoadHack);
        settings->setValue(QStringLiteral("@EnableDragGesture"),    m_EnableDragHack);
        settings->setValue(QStringLiteral("@ActivateNewViewDefault"), m_ActivateNewViewDefault);
        settings->setValue(QStringLiteral("@NavigationBySpaceKey"), m_NavigationBySpaceKey);

        Page::OpenCommandOperation operation = Page::GetOpenOparation();
        if(operation == Page::InNewViewNode)  settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewViewNode"));
        if(operation == Page::InNewHistNode)  settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewHistNode"));
        if(operation == Page::InNewDirectory) settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewDirectory"));
        if(operation == Page::OnRoot)         settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("OnRoot"));

        if(operation == Page::InNewViewNodeBackground)  settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewViewNodeBackground"));
        if(operation == Page::InNewHistNodeBackground)  settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewHistNodeBackground"));
        if(operation == Page::InNewDirectoryBackground) settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewDirectoryBackground"));
        if(operation == Page::OnRootBackground)         settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("OnRootBackground"));

        if(operation == Page::InNewViewNodeNewWindow)  settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewViewNodeNewWindow"));
        if(operation == Page::InNewHistNodeNewWindow)  settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewHistNodeNewWindow"));
        if(operation == Page::InNewDirectoryNewWindow) settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("InNewDirectoryNewWindow"));
        if(operation == Page::OnRootNewWindow)         settings->setValue(QStringLiteral("@OpenCommandOperation"), QStringLiteral("OnRootNewWindow"));

        settings->beginGroup(QStringLiteral("preferences"));{
#ifdef QTWEBKIT
            settings->setValue(QStringLiteral("AcceleratedCompositingEnabled"),     gws->testAttribute(QWebSettings::AcceleratedCompositingEnabled)     );
            settings->setValue(QStringLiteral("AutoLoadImages"),                    gws->testAttribute(QWebSettings::AutoLoadImages)                    );
            settings->setValue(QStringLiteral("DeveloperExtrasEnabled"),            gws->testAttribute(QWebSettings::DeveloperExtrasEnabled)            );
            settings->setValue(QStringLiteral("DnsPrefetchEnabled"),                gws->testAttribute(QWebSettings::DnsPrefetchEnabled)                );
            settings->setValue(QStringLiteral("FrameFlatteningEnabled"),            gws->testAttribute(QWebSettings::FrameFlatteningEnabled)            );
            settings->setValue(QStringLiteral("JavaEnabled"),                       gws->testAttribute(QWebSettings::JavaEnabled)                       );
            settings->setValue(QStringLiteral("JavascriptCanAccessClipboard"),      gws->testAttribute(QWebSettings::JavascriptCanAccessClipboard)      );
            settings->setValue(QStringLiteral("JavascriptCanCloseWindows"),         gws->testAttribute(QWebSettings::JavascriptCanCloseWindows)         );
            settings->setValue(QStringLiteral("JavascriptCanOpenWindows"),          gws->testAttribute(QWebSettings::JavascriptCanOpenWindows)          );
            settings->setValue(QStringLiteral("JavascriptEnabled"),                 gws->testAttribute(QWebSettings::JavascriptEnabled)                 );
            settings->setValue(QStringLiteral("LinksIncludedInFocusChain"),         gws->testAttribute(QWebSettings::LinksIncludedInFocusChain)         );
            settings->setValue(QStringLiteral("LocalContentCanAccessFileUrls"),     gws->testAttribute(QWebSettings::LocalContentCanAccessFileUrls)     );
            settings->setValue(QStringLiteral("LocalContentCanAccessRemoteUrls"),   gws->testAttribute(QWebSettings::LocalContentCanAccessRemoteUrls)   );
            settings->setValue(QStringLiteral("LocalStorageDatabaseEnabled"),       gws->testAttribute(QWebSettings::LocalStorageDatabaseEnabled)       );
            settings->setValue(QStringLiteral("LocalStorageEnabled"),               gws->testAttribute(QWebSettings::LocalStorageEnabled)               );
            settings->setValue(QStringLiteral("OfflineStorageDatabaseEnabled"),     gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled)     );
            settings->setValue(QStringLiteral("OfflineWebApplicationCacheEnabled"), gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled) );
            settings->setValue(QStringLiteral("PluginsEnabled"),                    gws->testAttribute(QWebSettings::PluginsEnabled)                    );
            settings->setValue(QStringLiteral("PrintElementBackgrounds"),           gws->testAttribute(QWebSettings::PrintElementBackgrounds)           );
            settings->setValue(QStringLiteral("PrivateBrowsingEnabled"),            gws->testAttribute(QWebSettings::PrivateBrowsingEnabled)            );
            settings->setValue(QStringLiteral("SiteSpecificQuirksEnabled"),         gws->testAttribute(QWebSettings::SiteSpecificQuirksEnabled)         );
            settings->setValue(QStringLiteral("SpatialNavigationEnabled"),          gws->testAttribute(QWebSettings::SpatialNavigationEnabled)          );
            settings->setValue(QStringLiteral("TiledBackingStoreEnabled"),          gws->testAttribute(QWebSettings::TiledBackingStoreEnabled)          );
            settings->setValue(QStringLiteral("XSSAuditingEnabled"),                gws->testAttribute(QWebSettings::XSSAuditingEnabled)                );
            settings->setValue(QStringLiteral("ZoomTextOnly"),                      gws->testAttribute(QWebSettings::ZoomTextOnly)                      );
            settings->setValue(QStringLiteral("CSSGridLayoutEnabled"),              gws->testAttribute(QWebSettings::CSSGridLayoutEnabled)              );
            settings->setValue(QStringLiteral("CSSRegionsEnabled"),                 gws->testAttribute(QWebSettings::CSSRegionsEnabled)                 );
            settings->setValue(QStringLiteral("CaretBrowsingEnabled"),              gws->testAttribute(QWebSettings::CaretBrowsingEnabled)              );
            settings->setValue(QStringLiteral("HyperlinkAuditingEnabled"),          gws->testAttribute(QWebSettings::HyperlinkAuditingEnabled)          );
            settings->setValue(QStringLiteral("NotificationsEnabled"),              gws->testAttribute(QWebSettings::NotificationsEnabled)              );
            settings->setValue(QStringLiteral("ScrollAnimatorEnabled"),             gws->testAttribute(QWebSettings::ScrollAnimatorEnabled)             );
            settings->setValue(QStringLiteral("WebAudioEnabled"),                   gws->testAttribute(QWebSettings::WebAudioEnabled)                   );
            settings->setValue(QStringLiteral("WebGLEnabled"),                      gws->testAttribute(QWebSettings::WebGLEnabled)                      );
            settings->setValue(QStringLiteral("Accelerated2dCanvasEnabled"),        gws->testAttribute(QWebSettings::Accelerated2dCanvasEnabled)        );
            settings->setValue(QStringLiteral("ErrorPageEnabled"),                 gwes->testAttribute(QWebEngineSettings::ErrorPageEnabled)            );
#  if QT_VERSION >= 0x050600
            settings->setValue(QStringLiteral("FullScreenSupportEnabled"),         gwes->testAttribute(QWebEngineSettings::FullScreenSupportEnabled)    );
#  endif
#else
          //settings->setValue(QStringLiteral("AcceleratedCompositingEnabled"),     gwes->testAttribute(QWebEngineSettings::AcceleratedCompositingEnabled)     );
            settings->setValue(QStringLiteral("AutoLoadImages"),                    gwes->testAttribute(QWebEngineSettings::AutoLoadImages)                    );
          //settings->setValue(QStringLiteral("DeveloperExtrasEnabled"),            gwes->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled)            );
          //settings->setValue(QStringLiteral("DnsPrefetchEnabled"),                gwes->testAttribute(QWebEngineSettings::DnsPrefetchEnabled)                );
          //settings->setValue(QStringLiteral("FrameFlatteningEnabled"),            gwes->testAttribute(QWebEngineSettings::FrameFlatteningEnabled)            );
          //settings->setValue(QStringLiteral("JavaEnabled"),                       gwes->testAttribute(QWebEngineSettings::JavaEnabled)                       );
            settings->setValue(QStringLiteral("JavascriptCanAccessClipboard"),      gwes->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard)      );
          //settings->setValue(QStringLiteral("JavascriptCanCloseWindows"),         gwes->testAttribute(QWebEngineSettings::JavascriptCanCloseWindows)         );
            settings->setValue(QStringLiteral("JavascriptCanOpenWindows"),          gwes->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows)          );
            settings->setValue(QStringLiteral("JavascriptEnabled"),                 gwes->testAttribute(QWebEngineSettings::JavascriptEnabled)                 );
            settings->setValue(QStringLiteral("LinksIncludedInFocusChain"),         gwes->testAttribute(QWebEngineSettings::LinksIncludedInFocusChain)         );
            settings->setValue(QStringLiteral("LocalContentCanAccessFileUrls"),     gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls)     );
            settings->setValue(QStringLiteral("LocalContentCanAccessRemoteUrls"),   gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls)   );
          //settings->setValue(QStringLiteral("LocalStorageDatabaseEnabled"),       gwes->testAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled)       );
            settings->setValue(QStringLiteral("LocalStorageEnabled"),               gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled)               );
          //settings->setValue(QStringLiteral("OfflineStorageDatabaseEnabled"),     gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled)     );
          //settings->setValue(QStringLiteral("OfflineWebApplicationCacheEnabled"), gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled) );
#  if QT_VERSION >= 0x050600
            settings->setValue(QStringLiteral("PluginsEnabled"),                    gwes->testAttribute(QWebEngineSettings::PluginsEnabled)                    );
#  endif
          //settings->setValue(QStringLiteral("PrintElementBackgrounds"),           gwes->testAttribute(QWebEngineSettings::PrintElementBackgrounds)           );
          //settings->setValue(QStringLiteral("PrivateBrowsingEnabled"),            gwes->testAttribute(QWebEngineSettings::PrivateBrowsingEnabled)            );
          //settings->setValue(QStringLiteral("SiteSpecificQuirksEnabled"),         gwes->testAttribute(QWebEngineSettings::SiteSpecificQuirksEnabled)         );
            settings->setValue(QStringLiteral("SpatialNavigationEnabled"),          gwes->testAttribute(QWebEngineSettings::SpatialNavigationEnabled)          );
          //settings->setValue(QStringLiteral("TiledBackingStoreEnabled"),          gwes->testAttribute(QWebEngineSettings::TiledBackingStoreEnabled)          );
            settings->setValue(QStringLiteral("XSSAuditingEnabled"),                gwes->testAttribute(QWebEngineSettings::XSSAuditingEnabled)                );
          //settings->setValue(QStringLiteral("ZoomTextOnly"),                      gwes->testAttribute(QWebEngineSettings::ZoomTextOnly)                      );
          //settings->setValue(QStringLiteral("CSSGridLayoutEnabled"),              gwes->testAttribute(QWebEngineSettings::CSSGridLayoutEnabled)              );
          //settings->setValue(QStringLiteral("CSSRegionsEnabled"),                 gwes->testAttribute(QWebEngineSettings::CSSRegionsEnabled)                 );
          //settings->setValue(QStringLiteral("CaretBrowsingEnabled"),              gwes->testAttribute(QWebEngineSettings::CaretBrowsingEnabled)              );
            settings->setValue(QStringLiteral("HyperlinkAuditingEnabled"),          gwes->testAttribute(QWebEngineSettings::HyperlinkAuditingEnabled)          );
          //settings->setValue(QStringLiteral("NotificationsEnabled"),              gwes->testAttribute(QWebEngineSettings::NotificationsEnabled)              );
            settings->setValue(QStringLiteral("ScrollAnimatorEnabled"),             gwes->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled)             );
          //settings->setValue(QStringLiteral("WebAudioEnabled"),                   gwes->testAttribute(QWebEngineSettings::WebAudioEnabled)                   );
          //settings->setValue(QStringLiteral("WebGLEnabled"),                      gwes->testAttribute(QWebEngineSettings::WebGLEnabled)                      );
          //settings->setValue(QStringLiteral("Accelerated2dCanvasEnabled"),        gwes->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled)        );
            settings->setValue(QStringLiteral("ErrorPageEnabled"),                  gwes->testAttribute(QWebEngineSettings::ErrorPageEnabled)                  );
#  if QT_VERSION >= 0x050600
            settings->setValue(QStringLiteral("FullScreenSupportEnabled"),          gwes->testAttribute(QWebEngineSettings::FullScreenSupportEnabled)          );
#  endif
#endif
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("detail"));{
#ifdef QTWEBKIT
            settings->setValue(QStringLiteral("DefaultTextEncoding"),             gws->defaultTextEncoding());
            settings->setValue(QStringLiteral("MaximumPagesInCache"),             gws->maximumPagesInCache());
            // setting non-default value to crash...
          //settings->setValue(QStringLiteral("IconDatabasePath"),                gws->iconDatabasePath());
            settings->setValue(QStringLiteral("LocalStoragePath"),                gws->localStoragePath());
            settings->setValue(QStringLiteral("OfflineStoragePath"),              gws->offlineStoragePath());
            settings->setValue(QStringLiteral("OfflineStorageDefaultQuota"),      gws->offlineStorageDefaultQuota());
            // setting value to crash...
          //settings->setValue(QStringLiteral("OfflineWebApplicationCachePath"),  gws->offlineWebApplicationCachePath());
            settings->setValue(QStringLiteral("OfflineWebApplicationCacheQuota"), gws->offlineWebApplicationCacheQuota());
            settings->setValue(QStringLiteral("UserStyleSheetUrl"),               gws->userStyleSheetUrl());
#else
            settings->setValue(QStringLiteral("DefaultTextEncoding"),             gwes->defaultTextEncoding());
          //settings->setValue(QStringLiteral("MaximumPagesInCache"),             gwes->maximumPagesInCache());
            // setting non-default value to crash...
          //settings->setValue(QStringLiteral("IconDatabasePath"),                gwes->iconDatabasePath());
          //settings->setValue(QStringLiteral("LocalStoragePath"),                gwes->localStoragePath());
          //settings->setValue(QStringLiteral("OfflineStoragePath"),              gwes->offlineStoragePath());
          //settings->setValue(QStringLiteral("OfflineStorageDefaultQuota"),      gwes->offlineStorageDefaultQuota());
            // setting value to crash...
          //settings->setValue(QStringLiteral("OfflineWebApplicationCachePath"),  gwes->offlineWebApplicationCachePath());
          //settings->setValue(QStringLiteral("OfflineWebApplicationCacheQuota"), gwes->offlineWebApplicationCacheQuota());
          //settings->setValue(QStringLiteral("UserStyleSheetUrl"),               gwes->userStyleSheetUrl());
#endif

#ifdef QTWEBKIT
            QWebSettings::ThirdPartyCookiePolicy policy = gws->thirdPartyCookiePolicy();
            if(policy == QWebSettings::AlwaysAllowThirdPartyCookies)
                settings->setValue(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AlwaysAllowThirdPartyCookies"));
            if(policy == QWebSettings::AlwaysBlockThirdPartyCookies)
                settings->setValue(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AlwaysBlockThirdPartyCookies"));
            if(policy == QWebSettings::AllowThirdPartyWithExistingCookies)
                settings->setValue(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AllowThirdPartyWithExistingCookies"));
#else
          //QWebEngineSettings::ThirdPartyCookiePolicy policy = gwes->thirdPartyCookiePolicy();
          //if(policy == QWebSettings::AlwaysAllowThirdPartyCookies)
          //    settings->setValue(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AlwaysAllowThirdPartyCookies"));
          //if(policy == QWebSettings::AlwaysBlockThirdPartyCookies)
          //    settings->setValue(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AlwaysBlockThirdPartyCookies"));
          //if(policy == QWebSettings::AllowThirdPartyWithExistingCookies)
          //    settings->setValue(QStringLiteral("ThirdPartyCookiePolicy"), QStringLiteral("AllowThirdPartyWithExistingCookies"));
#endif
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("font"));{
#ifdef QTWEBKIT
            settings->setValue(QStringLiteral("StandardFont"),           gws->fontFamily(QWebSettings::StandardFont)         );
            settings->setValue(QStringLiteral("FixedFont"),              gws->fontFamily(QWebSettings::FixedFont)            );
            settings->setValue(QStringLiteral("SerifFont"),              gws->fontFamily(QWebSettings::SerifFont)            );
            settings->setValue(QStringLiteral("SansSerifFont"),          gws->fontFamily(QWebSettings::SansSerifFont)        );
            settings->setValue(QStringLiteral("CursiveFont"),            gws->fontFamily(QWebSettings::CursiveFont)          );
            settings->setValue(QStringLiteral("FantasyFont"),            gws->fontFamily(QWebSettings::FantasyFont)          );
            settings->setValue(QStringLiteral("MinimumFontSize"),        gws->fontSize(QWebSettings::MinimumFontSize)        );
            settings->setValue(QStringLiteral("MinimumLogicalFontSize"), gws->fontSize(QWebSettings::MinimumLogicalFontSize) );
            settings->setValue(QStringLiteral("DefaultFontSize"),        gws->fontSize(QWebSettings::DefaultFontSize)        );
            settings->setValue(QStringLiteral("DefaultFixedFontSize"),   gws->fontSize(QWebSettings::DefaultFixedFontSize)   );
#else
            settings->setValue(QStringLiteral("StandardFont"),           gwes->fontFamily(QWebEngineSettings::StandardFont)         );
            settings->setValue(QStringLiteral("FixedFont"),              gwes->fontFamily(QWebEngineSettings::FixedFont)            );
            settings->setValue(QStringLiteral("SerifFont"),              gwes->fontFamily(QWebEngineSettings::SerifFont)            );
            settings->setValue(QStringLiteral("SansSerifFont"),          gwes->fontFamily(QWebEngineSettings::SansSerifFont)        );
            settings->setValue(QStringLiteral("CursiveFont"),            gwes->fontFamily(QWebEngineSettings::CursiveFont)          );
            settings->setValue(QStringLiteral("FantasyFont"),            gwes->fontFamily(QWebEngineSettings::FantasyFont)          );
            settings->setValue(QStringLiteral("MinimumFontSize"),        gwes->fontSize(QWebEngineSettings::MinimumFontSize)        );
            settings->setValue(QStringLiteral("MinimumLogicalFontSize"), gwes->fontSize(QWebEngineSettings::MinimumLogicalFontSize) );
            settings->setValue(QStringLiteral("DefaultFontSize"),        gwes->fontSize(QWebEngineSettings::DefaultFontSize)        );
            settings->setValue(QStringLiteral("DefaultFixedFontSize"),   gwes->fontSize(QWebEngineSettings::DefaultFixedFontSize)   );
#endif
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("menu"));{
            settings->setValue(QStringLiteral("LinkMenu"),      m_LinkMenu);
            settings->setValue(QStringLiteral("ImageMenu"),     m_ImageMenu);
            settings->setValue(QStringLiteral("SelectionMenu"), m_SelectionMenu);
            settings->setValue(QStringLiteral("RegularMenu"),   m_RegularMenu);
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("keymap"));{
            QList<QKeySequence> seqs = m_KeyMap.keys();
            if(seqs.isEmpty()){
                /* 'm_KeyMap' will be construct, when next startup. */
            } else {
                foreach(QKeySequence seq, seqs){
                    if(!seq.isEmpty() && !seq.toString().isEmpty())
                        settings->setValue(seq.toString()
                                           // cannot use slashes on QSettings.
                                             .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                                             .replace(QStringLiteral("/"), QStringLiteral("Slash")),
                                           m_KeyMap[seq]);
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("mouse"));{
            QStringList buttons = m_MouseMap.keys();
            if(buttons.isEmpty()){
                /* 'm_MouseMap' will be construct, when next startup. */
            } else {
                foreach(QString button, buttons){
                    if(!button.isEmpty())
                        settings->setValue(button, m_MouseMap[button]);
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("rightgesture"));{
            QStringList gestures = m_RightGestureMap.keys();
            if(gestures.isEmpty()){
                /* 'm_RightGestureMap' will be construct, when next startup. */
            } else {
                foreach(QString gesture, gestures){
                    if(!gesture.isEmpty())
                        settings->setValue(gesture, m_RightGestureMap[gesture]);
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("leftgesture"));{
            QStringList gestures = m_LeftGestureMap.keys();
            if(gestures.isEmpty()){
                /* 'm_LeftGestureMap' will be construct, when next startup. */
            } else {
                foreach(QString gesture, gestures){
                    if(!gesture.isEmpty())
                        settings->setValue(gesture, m_LeftGestureMap[gesture]);
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("searchengine"));{
            QMap<QString, SearchEngine> search = Page::GetSearchEngineMap();
            foreach(QString key, search.keys()){
                if(search[key].length() == 1)
                    settings->setValue(key, search[key].first());
                else
                    settings->setValue(key, search[key]);
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("bookmarklet"));{
            QMap<QString, Bookmarklet> bookmark = Page::GetBookmarkletMap();
            foreach(QString key, bookmark.keys()){
                if(bookmark[key].length() == 1)
                    settings->setValue(key, bookmark[key].first());
                else
                    settings->setValue(key, bookmark[key]);
            }
        }
        settings->endGroup();
    }
    settings->endGroup();
}

void View::ApplySpecificSettings(QStringList set){
    if(!page()) return;

#ifdef QTWEBKIT
    if(WebPage *p = qobject_cast<WebPage*>(page())){
        QWebSettings *s = p->settings();
        QWebSettings *g = QWebSettings::globalSettings();

        int pos = set.indexOf(QRegExp(QStringLiteral("^(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*")));
        if(pos != -1)
            s->setDefaultTextEncoding(set[pos].split(QStringLiteral(" ")).last());

        foreach(QWebSettings::WebAttribute attr, m_WebSwitches.keys()){
            QString str = m_WebSwitches[attr];
            s->setAttribute(attr,
                            set.indexOf(QRegExp(QStringLiteral("!")+str)) != -1 ? false :
                            set.indexOf(QRegExp(str))     != -1 ? true  :
                            g->testAttribute(attr));
        }
    } else
#endif
    if(WebEnginePage *p = qobject_cast<WebEnginePage*>(page())){
        QWebEngineSettings *s = p->settings();
        QWebEngineSettings *g = QWebEngineSettings::globalSettings();

        int pos = set.indexOf(QRegExp(QStringLiteral("^(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*")));
        if(pos != -1)
            s->setDefaultTextEncoding(set[pos].split(QStringLiteral(" ")).last());

        foreach(QWebEngineSettings::WebAttribute attr, m_WebEngineSwitches.keys()){
            QString str = m_WebEngineSwitches[attr];
            s->setAttribute(attr,
                            set.indexOf(QRegExp(QStringLiteral("!")+str)) != -1 ? false :
                            set.indexOf(QRegExp(str))     != -1 ? true  :
                            g->testAttribute(attr));
        }
    }

    if     (set.indexOf(QRegExp(QStringLiteral( "[lL](?:oad)?[hH](?:ack)?"))) != -1) m_EnableLoadHackLocal = true;
    else if(set.indexOf(QRegExp(QStringLiteral("![lL](?:oad)?[hH](?:ack)?"))) != -1) m_EnableLoadHackLocal = false;
    if     (set.indexOf(QRegExp(QStringLiteral( "[dD](?:rag)?[hH](?:ack)?"))) != -1) m_EnableDragHackLocal = true;
    else if(set.indexOf(QRegExp(QStringLiteral("![dD](?:rag)?[hH](?:ack)?"))) != -1) m_EnableDragHackLocal = false;

    if     (set.indexOf(QRegExp(QStringLiteral( "[hH](?:ist)?[nN](?:ode)?"))) != -1) m_EnableLoadHackLocal = true;
    else if(set.indexOf(QRegExp(QStringLiteral("![hH](?:ist)?[nN](?:ode)?"))) != -1) m_EnableLoadHackLocal = false;
    if     (set.indexOf(QRegExp(QStringLiteral( "[dD](?:rag)?[gG](?:esture)?"))) != -1) m_EnableDragHackLocal = true;
    else if(set.indexOf(QRegExp(QStringLiteral("![dD](?:rag)?[gG](?:esture)?"))) != -1) m_EnableDragHackLocal = false;
}

QUrl View::url(){ return QUrl();}
void View::setUrl(const QUrl&){}

QString View::html(){ return QString();}
void View::setHtml(const QString&, const QUrl&){}

TreeBank *View::parent(){ return 0;}
void View::setParent(TreeBank*){}

void View::Connect(TreeBank *tb){
    if(!tb || !page()) return;

    if(Receiver *receiver = tb->GetReceiver()){
        QObject::connect(receiver, SIGNAL(OpenUrl(QUrl)),
                         page(), SLOT(OpenInNew(QUrl)));
        QObject::connect(receiver, SIGNAL(OpenUrl(QList<QUrl>)),
                         page(), SLOT(OpenInNew(QList<QUrl>)));
        QObject::connect(receiver, SIGNAL(OpenQueryUrl(QString)),
                         page(), SLOT(OpenInNew(QString)));
        QObject::connect(receiver, SIGNAL(SearchWith(QString, QString)),
                         page(), SLOT(OpenInNew(QString, QString)));
        QObject::connect(receiver, SIGNAL(Download(QString, QString)),
                         page(), SLOT(Download(QString, QString)));
    }
}

void View::Disconnect(TreeBank *tb){
    if(!tb || !page()) return;

    if(Receiver *receiver = tb->GetReceiver()){
        QObject::disconnect(receiver, SIGNAL(OpenUrl(QUrl)),
                            page(), SLOT(OpenInNew(QUrl)));
        QObject::disconnect(receiver, SIGNAL(OpenUrl(QList<QUrl>)),
                            page(), SLOT(OpenInNew(QList<QUrl>)));
        QObject::disconnect(receiver, SIGNAL(OpenQueryUrl(QString)),
                            page(), SLOT(OpenInNew(QString)));
        QObject::disconnect(receiver, SIGNAL(SearchWith(QString, QString)),
                            page(), SLOT(OpenInNew(QString, QString)));
        QObject::disconnect(receiver, SIGNAL(Download(QString, QString)),
                            page(), SLOT(Download(QString, QString)));
    }
}

void View::UpdateThumbnail(){
    if(!IsRenderable()) return;

    if(m_HistNode && page()){
        MainWindow *win = Application::GetCurrentWindow();
        QSize parentsize =
            m_TreeBank ? m_TreeBank->size() :
            win ? win->GetTreeBank()->size() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;

        if(!visible()){
            int x = m_HistNode->GetScrollX();
            int y = m_HistNode->GetScrollY();
            SetViewportSize(parentsize);
            m_HistNode->SetScrollX(x);
            m_HistNode->SetScrollY(y);
            RestoreScroll();
        }

        QImage image(parentsize, QImage::Format_ARGB32);
        QPainter painter(&image);
        Render(&painter,
               QRegion(QRect(QPoint(0, 0),
                             parentsize)));
        painter.end();

        parentsize.scale(SAVING_THUMBNAIL_SIZE,
                         Qt::KeepAspectRatioByExpanding);

        int width_diff  = parentsize.width()  - SAVING_THUMBNAIL_SIZE.width();
        int height_diff = parentsize.height() - SAVING_THUMBNAIL_SIZE.height();

        /*

      SAVING_THUMBNAIL_SIZE:

          +---------+
          |         |
          |         |
          +---------+


          window width
       <--------------->

       +--+---------+--+
       |  |         |  |
       |  |         |  |
       +--+---------+--+

          <--------->
       saved image width


       ^  +---------+
       |  |         |
       |  +---------+  ^
window |  |         |  | saved image
height |  |         |  | height
       |  +---------+  v
       |  |         |
       v  +---------+

         */

        if(width_diff == 0 && height_diff == 0){
            m_HistNode->SetImage(
                image.
                scaled(SAVING_THUMBNAIL_SIZE,
                       Qt::KeepAspectRatioByExpanding,
                       Qt::SmoothTransformation));
        } else {
            m_HistNode->SetImage(
                image.
                scaled(parentsize,
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation).
                copy(width_diff / 2, height_diff / 2,
                     SAVING_THUMBNAIL_SIZE.width(),
                     SAVING_THUMBNAIL_SIZE.height()));
        }
    }
}

void View::SetSwitchingState(bool switching){
    m_Switching = switching;
}

bool View::GetSwitchingState(){
    return m_Switching;
}

// no referer.
void View::Load(){
    bool ok;
    QString str =
        ModalDialog::GetText(QObject::tr("Url or Javascript"),
                             QObject::tr("Input Url or Javascript"),
                             url().toString(), &ok);
    if(!str.isEmpty()) Load(str);
}

// no referer.
void View::Load(const QString &url){
    if(!page()) return;

    if(url.startsWith(QStringLiteral("javascript:"))){
        CallWithEvaluatedJavaScriptResult(url.mid(11), [](QVariant){});
    } else {
        QString reconverted = QString::fromLatin1(url.toLatin1());
        if(url == reconverted)
            TriggerNativeLoadAction(QUrl::fromEncoded(url.toLatin1()));
        else
            TriggerNativeLoadAction(QUrl(url));
    }
}

// with referer.
void View::Load(const QUrl &url){
    QNetworkRequest req(url);
    Node *parent = m_HistNode->GetParent();
    if(parent && !parent->GetUrl().isEmpty())
        req.setRawHeader("Referer", parent->GetUrl().toEncoded());
    Load(req);
}

// with referer.
void View::Load(const QNetworkRequest &req){
    TreeBank::AddToUpdateBox(GetThis().lock());
    bool forbidden = ForbidToOverlap();
    QString str = QString::fromUtf8(req.url().toEncoded());

    if(str.startsWith(QStringLiteral("javascript:"))){
        QString code = req.url().toString(QUrl::None).mid(11);

        int count = 2;

        // too dirty...
        for(int i = count; i > 0; i--){
            code.replace(QRegExp(QStringLiteral("%%%%%%%")), QStringLiteral("%25%25%25%25%25%25%25"));
            code.replace(QRegExp(QStringLiteral("%%%%%")), QStringLiteral("%25%25%25%25%25"));
            code.replace(QRegExp(QStringLiteral("%%%")), QStringLiteral("%25%25%25"));
            code.replace(QRegExp(QStringLiteral("%%")), QStringLiteral("%25%25"));
            code.replace(QRegExp(QStringLiteral("%([^0-9A-F][0-9A-F]|[0-9A-F][^0-9A-F]|[^0-9A-F][^0-9A-F])")),
                         QStringLiteral("%25\\1"));
            code = QUrl::fromPercentEncoding(code.toLatin1());
        }

        CallWithEvaluatedJavaScriptResult(code, [](QVariant){});

    } else {
        QNetworkRequest newreq(req);
        Node *parent = m_HistNode->GetParent();

        // set referer.
        if(req.rawHeader("Referer").length() == 0 && parent && !parent->GetUrl().isEmpty())
            newreq.setRawHeader("Referer", parent->GetUrl().toEncoded());

        if(str.startsWith(QStringLiteral("about:"))){
            if(str == QStringLiteral("about:blank"))
                setUrl(QUrl(QStringLiteral("about:blank")));
            else
                TriggerNativeLoadAction(QNetworkRequest(QUrl(QStringLiteral("about:blank"))));
        } else if(str.startsWith(QStringLiteral("view-source:"))){
            if(qobject_cast<WebEngineView*>(base()) ||
               qobject_cast<QuickWebEngineView*>(base())){
                TriggerNativeLoadAction(newreq.url());
            } else {
                SetSource(QUrl::fromEncoded(str.toLatin1()));
            }
        } else {
            TriggerNativeLoadAction(newreq);
        }

        if(m_TreeBank && m_TreeBank->GetCurrentView().get() == this){
            if(!forbidden && ForbidToOverlap()) m_TreeBank->PurgeChildWidgetsIfNeed();
            if(forbidden && !ForbidToOverlap()) m_TreeBank->JoinChildWidgetsIfNeed();
        }
    }
}

void View::OnFocusIn(){
    m_ClickedElement = 0;
    m_SelectionRegion = QRegion();

    MainWindow *win;
    if(m_TreeBank){
        win = m_TreeBank->GetMainWindow();
        Application::SetCurrentWindow(win);
    }
    if(!m_TreeBank) return;
    if(TreeBank::PurgeView()) raise();
    if(win) win->RaiseAllEdgeWidgets();
    if(m_TreeBank->GetNotifier()) m_TreeBank->GetNotifier()->raise();
    if(m_TreeBank->GetReceiver()) m_TreeBank->GetReceiver()->raise();
    if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
    if(HistNode *hn = GetHistNode()) hn->SetLastAccessDateToCurrent();
}

void View::OnFocusOut(){
    m_ClickedElement = 0;
    m_SelectionRegion = QRegion();
}

void View::GestureStarted(QPoint pos){
    m_Gesture.clear();
    m_GestureStartedPos = pos;
    m_BeforeGesturePos  = pos;
    m_SameGestureVectorCount = 0;
    m_HadSelection = !SelectedText().isEmpty();
    SetScrollBarState();
    CallWithHitElement(pos, [this](SharedWebElement elem){ m_ClickedElement = elem;});
    if(m_HadSelection)
        CallWithSelectionRegion([this, pos](QRegion region){
                if(region.contains(pos))
                    m_SelectionRegion = region;
                else
                    m_HadSelection = false;
            });
}

void View::GestureMoved(QPoint pos){
    if(m_GestureStartedPos != QPoint()){
        Action(Page::We_NoAction); // for action guard.
        QPoint delta = m_BeforeGesturePos - pos;
        m_BeforeGestureVector = m_CurrentGestureVector;
        m_CurrentGestureVector = GetGestureVector(delta.x(), delta.y());

        if(m_CurrentGestureVector == Gv_NoMove){
            m_BeforeGesturePos = pos;
            return;
        }

        if(!m_Gesture.isEmpty() &&
           m_Gesture.last() == m_CurrentGestureVector){
            m_BeforeGesturePos = pos;
            return;
        }

        if(m_BeforeGestureVector == m_CurrentGestureVector){
            m_SameGestureVectorCount++;
        } else {
            m_SameGestureVectorCount = 0;
        }

        // on flash context menu, mouse cursor jumps.
        if(m_SameGestureVectorCount <= 1){
            m_BeforeGesturePos = pos;
            return;
        }

        if(m_SameGestureVectorCount >= GESTURE_TRIGGER_COUNT ||
           delta.manhattanLength() >= GESTURE_TRIGGER_LENGTH){
            m_SameGestureVectorCount = 0;
            m_Gesture << m_CurrentGestureVector;
        }
        m_BeforeGesturePos = pos;
    }
}

void View::GestureAborted(){
    m_Gesture.clear();
    m_GestureStartedPos = QPoint();
    m_BeforeGesturePos  = QPoint();
    m_SameGestureVectorCount = 0;
    m_ClickedElement = 0;
    m_SelectionRegion = QRegion();
    m_HadSelection = false;
    m_DragStarted = false;
}

void View::GestureFinished(QPoint pos, Qt::MouseButton button){
    GestureMoved(pos);
    QString ges = GestureToString(m_Gesture);

    if(button == Qt::RightButton && m_RightGestureMap.contains(ges)){

        QString str = m_RightGestureMap[ges];
        if(!str.isEmpty()){
            TriggerAction(str, m_GestureStartedPos);
        }
    }

    if(button == Qt::LeftButton && m_LeftGestureMap.contains(ges)){

        QString str = m_LeftGestureMap[ges];
        if(!str.isEmpty()){
            TriggerAction(str, m_GestureStartedPos);
        }
    }
    m_Gesture.clear();
    m_GestureStartedPos = QPoint();
    m_BeforeGesturePos  = QPoint();
    m_SameGestureVectorCount = 0;
    m_ClickedElement = 0;
    m_SelectionRegion = QRegion();
    m_HadSelection = false;
    m_DragStarted = false;
}

void View::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return;
    QString str = m_KeyMap[seq];
    if(str.isEmpty()) return;

    TriggerAction(str);
}

void View::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return;
    str = m_KeyMap[seq]; // sequence => action
    if(str.isEmpty()) return;

    TriggerAction(str);
}

void View::ChangeNodeTitle(const QString &title){
    if(m_ViewNode && m_HistNode && !title.isEmpty()){
        if(m_ViewNode->GetPartner() == m_HistNode)
            m_ViewNode->SetTitle(title);
        m_HistNode->SetTitle(title);
    }
}

void View::ChangeNodeUrl(const QUrl &url){
    if(m_ViewNode && m_HistNode && !url.isEmpty())
        m_HistNode->SetUrl(url);
}

void View::SaveViewState(){
    SaveScroll();
    SaveZoom();
}

void View::RestoreViewState(){
    RestoreScroll();
    RestoreZoom();
}

float View::PrepareForZoomIn(){
    static const float eps = 0.01f;
    HistNode *hn = GetHistNode();
    int len   = GetZoomFactorLevels().length();
    int level = GetZoomFactorLevels().indexOf(hn->GetZoom());
    float zoom;
    if(level == -1){
        for(int i = 0; i < len; i++){
            zoom = GetZoomFactorLevels()[i];
            if((zoom - hn->GetZoom()) > eps) break;
        }
    } else if(level < len - 1){
        zoom = GetZoomFactorLevels()[level + 1];
    } else {
        zoom = GetZoomFactorLevels()[level];
    }
    hn->SetZoom(zoom);
    return zoom;
}

float View::PrepareForZoomOut(){
    static const float eps = 0.01f;
    HistNode *hn = GetHistNode();
    int len   = GetZoomFactorLevels().length();
    int level = GetZoomFactorLevels().indexOf(hn->GetZoom());
    float zoom;
    if(level == -1){
        for(int i = len - 1; i >= 0; i--){
            zoom = GetZoomFactorLevels()[i];
            if((hn->GetZoom() - zoom) > eps) break;
        }
    } else if(level > 0){
        zoom = GetZoomFactorLevels()[level - 1];
    } else {
        zoom = GetZoomFactorLevels()[level];
    }
    hn->SetZoom(zoom);
    return zoom;
}

JsWebElement::JsWebElement()
    : WebElement()
{
    m_Provider     = 0;
    m_TagName      = QString();
    m_InnerText    = QString();
    m_BaseUrl      = QUrl();
    m_LinkUrl      = QUrl();
    m_ImageUrl     = QUrl();
    m_LinkHtml     = QString();
    m_ImageHtml    = QString();
    m_Rectangle    = QRect();
    m_IsEditable   = false;
    m_IsJsCommand  = false;
    m_IsTextInput  = false;
    m_IsQueryInput = false;
    m_IsFrame      = false;
    m_XPath        = QString();
    m_Action       = QStringLiteral("None");
}

JsWebElement::JsWebElement(View *provider, QVariant var)
    : WebElement()
{
    QVariantMap map = var.toMap();
    m_Provider  = provider;
    m_TagName   = map[QStringLiteral("tagName")].toString();
    m_InnerText = map[QStringLiteral("innerText")].toString();
    m_BaseUrl   = Page::StringToUrl(map[QStringLiteral("baseUrl")].toString());
    m_LinkUrl   = Page::StringToUrl(map[QStringLiteral("linkUrl")].toString(), m_BaseUrl);
    m_ImageUrl  = Page::StringToUrl(map[QStringLiteral("imageUrl")].toString(), m_BaseUrl);
    m_LinkHtml  = map[QStringLiteral("linkHtml")].toString();
    m_ImageHtml = map[QStringLiteral("imageHtml")].toString();
    m_Rectangle = QRect(map[QStringLiteral("x")].toInt(),     map[QStringLiteral("y")].toInt(),
                        map[QStringLiteral("width")].toInt(), map[QStringLiteral("height")].toInt());
    m_IsEditable   = map[QStringLiteral("isEditable")].toBool();
    m_IsJsCommand  = map[QStringLiteral("isJsCommand")].toBool();
    m_IsTextInput  = map[QStringLiteral("isTextInput")].toBool();
    m_IsQueryInput = map[QStringLiteral("isQueryInput")].toBool();
    m_IsFrame      = map[QStringLiteral("isFrame")].toBool();
    m_XPath        = map[QStringLiteral("xPath")].toString();
    m_Action       = map[QStringLiteral("action")].toString();

    m_Region = QRegion();
    QVariantMap regionMap = map[QStringLiteral("region")].toMap();
    QRect viewport = QRect(QPoint(), m_Provider->size());
    foreach(QString key, regionMap.keys()){
        QVariantMap m = regionMap[key].toMap();
        m_Region |= QRect(m["x"].toInt(),
                          m["y"].toInt(),
                          m["width"].toInt(),
                          m["height"].toInt()).intersected(viewport);
    }
}

JsWebElement::~JsWebElement(){
}

bool JsWebElement::SetFocus(){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "SetFocusToElement",
                                  Q_ARG(QString, m_XPath));
        return true;
    }
    return false;
}

bool JsWebElement::ClickEvent(){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "FireClickEvent",
                                  Q_ARG(QString, m_XPath),
                                  Q_ARG(QPoint, Position()));
        return true;
    }
    return false;
}

QString JsWebElement::TagName() const {
    return m_TagName;
}

QString JsWebElement::InnerText() const {
    return m_InnerText;
}

QUrl JsWebElement::BaseUrl() const {
    return m_BaseUrl;
}

QUrl JsWebElement::LinkUrl() const {
    return m_LinkUrl;
}

QUrl JsWebElement::ImageUrl() const {
    return m_ImageUrl;
}

QString JsWebElement::LinkHtml() const {
    if(!m_LinkHtml.isEmpty()) return m_LinkHtml;
    return QStringLiteral("<a href=\"") + m_LinkUrl.toString() + QStringLiteral("\"></a>");
}

QString JsWebElement::ImageHtml() const {
    if(!m_ImageHtml.isEmpty()) return m_ImageHtml;
    return QStringLiteral("<img src=\"") + m_ImageUrl.toString() + QStringLiteral("\">");
}

QPoint JsWebElement::Position() const {
    return m_Rectangle.center();
}

QRect JsWebElement::Rectangle() const {
    return m_Rectangle;
}

QRegion JsWebElement::Region() const {
    return m_Region;
}

void JsWebElement::SetPosition(QPoint pos){
    m_Rectangle.moveCenter(pos);
}

void JsWebElement::SetRectangle(QRect rect){
    m_Rectangle = rect;
}

void JsWebElement::SetText(QString text){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "SetTextValue",
                                  Q_ARG(QString, m_XPath),
                                  Q_ARG(QString, text));
    }
}

QPixmap JsWebElement::Pixmap(){
    if(!m_Provider || IsNull()) return QPixmap();
    QPixmap pixmap(m_Provider->size());
    QPainter painter(&pixmap);
    QRect r;
    if(!Region().isNull()){
        m_Provider->Render(&painter, Region());
        r = Region().boundingRect();
    } else if(!Rectangle().isNull()){
        m_Provider->Render(&painter, Rectangle());
        r = Rectangle();
    }
    painter.end();
    return pixmap.copy(r);
}

bool JsWebElement::IsNull() const {
    return Position().isNull() || Rectangle().isEmpty();
}

bool JsWebElement::IsEditableElement() const {
    return m_IsEditable;
}

bool JsWebElement::IsJsCommandElement() const {
    return m_IsJsCommand;
}

bool JsWebElement::IsTextInputElement() const {
    return m_IsTextInput;
}

bool JsWebElement::IsQueryInputElement() const {
    return m_IsQueryInput;
}

bool JsWebElement::IsFrameElement() const {
    return m_IsFrame;
}

WebElement::Action JsWebElement::GetAction() const {
    if(m_Action == QStringLiteral("Focus")) return Focus;
    if(m_Action == QStringLiteral("Click")) return Click;
    if(m_Action == QStringLiteral("Hover")) return Hover;
    return None;
}

bool JsWebElement::Equals(const WebElement &other) const {
    return m_XPath == static_cast<const JsWebElement*>(&other)->m_XPath;
}
