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
#include <QWebEngineProfile>

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

bool View::m_EnableDestinationInferrer = false;
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
QUrl View::m_CurrentBaseUrl = QUrl();
QString View::m_SelectedText = QString();
QString View::m_SelectedHtml = QString();

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
    << qMakePair(QWebSettings::PluginsEnabled,           QString::fromLatin1("[pP]lugins?"))
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
    << qMakePair(QWebEngineSettings::PluginsEnabled,           QString::fromLatin1("[pP]lugins?"))
#endif
  //<< qMakePair(QWebEngineSettings::PrivateBrowsingEnabled,   QString::fromLatin1("(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"))
    << qMakePair(QWebEngineSettings::SpatialNavigationEnabled, QString::fromLatin1("[sS]patial(?:[nN]avigation)?"))
  //<< qMakePair(QWebEngineSettings::TiledBackingStoreEnabled, QString::fromLatin1("[tT]iled[bB]acking[sS]tore"))
  //<< qMakePair(QWebEngineSettings::ZoomTextOnly,             QString::fromLatin1("[zZ]oom[tT]ext[oO]nly"))
  //<< qMakePair(QWebEngineSettings::CaretBrowsingEnabled,     QString::fromLatin1("[cC]aret[bB]rowse"))
    << qMakePair(QWebEngineSettings::ScrollAnimatorEnabled,    QString::fromLatin1("[sS]croll[aA]nimator"))
#if QT_VERSION >= 0x050700
    << qMakePair(QWebEngineSettings::WebAudioEnabled,          QString::fromLatin1("[wW]eb[aA]udio"))
    << qMakePair(QWebEngineSettings::WebGLEnabled,             QString::fromLatin1("[wW]eb[gG][lL]"))
#endif
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
    m_LoadProgress = 0;
    m_IsLoading = false;
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

TreeBank *View::GetTreeBank() const {
    return m_TreeBank;
}

ViewNode *View::GetViewNode() const {
    return m_ViewNode;
}

HistNode* View::GetHistNode() const {
    return m_HistNode;
}

WeakView View::GetThis() const {
    return m_This;
}

WeakView View::GetMaster() const {
    return m_Master;
}

WeakView View::GetSlave() const {
    return m_Slave;
}

_View *View::GetJsObject() const {
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

    if(str1.contains(QRegularExpression(QStringLiteral(".\\.[a-z]+$"))) &&
       str2.contains(QRegularExpression(QStringLiteral(".\\.[a-z]+$")))){
        // accept subdomain.
        if(str1.startsWith(str2) || str2.startsWith(str1) ||
           str1.endsWith(str2)   || str2.endsWith(str1)){
            return QList<int>() << 0;
        } else {
            return QList<int>() << INT_MAX;
        }
    }

    QRegularExpression reg =
        QRegularExpression(QRegularExpression::escape(str1)
                           .replace(QRegularExpression(QStringLiteral("[0-9]+")),
                                    QString(QStringLiteral("([0-9]+)"))));
    QRegularExpressionMatch match;

    match = reg.match(str1);
    if(!match.hasMatch()) return QList<int>() << INT_MAX;
    QStringList nums1 = match.capturedTexts();
    if(nums1.length() > 1) nums1.removeFirst();

    match = reg.match(str2);
    if(!match.hasMatch()) return QList<int>() << INT_MAX;
    QStringList nums2 = match.capturedTexts();
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

    QRegularExpression reg =
        QRegularExpression(QObject::tr("(?:<<.*|.*<<|<|.*back(?:ward)?.*|.*prev(?:ious)?.*|.*reer.*|.*behind.*|.*before.*)"));

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

        if(url.toString().startsWith(QStringLiteral("javascript:")))
            continue;

        if(reg.match(text.toLower()).hasMatch()){
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
                        if(Application::ExactMatch(QStringLiteral("[0-9]+"), text.trimmed())){
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

    CallWithFoundElements(Page::RelIsNext, [this](SharedWebElementList nexts){

    if(!nexts.isEmpty() && !nexts[0]->LinkUrl().isEmpty()){
        GoForwardTo(nexts[0]->LinkUrl());
        return;
    }

    CallWithFoundElements(Page::HaveReference, [this](SharedWebElementList elements){

    QUrl base = url();
    QUrl copy = base;
    base.setFragment(QString()); // ignore fragment.

    QUrlQuery basequery = QUrlQuery(base);
    QStringList basebody = base.toString(QUrl::RemoveQuery).split(QStringLiteral("/"));
    basebody.removeFirst(); // ignore schema;

    SharedWebElement currentHighValidityElement = 0;
    QList<int> currentHighValidityDistance = QList<int>();

    QRegularExpression reg =
        QRegularExpression(QObject::tr("(?:>>.*|.*>>|>|.*forward.*|.*next.*|.*front.*|.*beyond.*|.*after.*|.*more.*)"));

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

        if(url.toString().startsWith(QStringLiteral("javascript:")))
            continue;

        if(reg.match(text.toLower()).hasMatch()){
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
                        if(Application::ExactMatch(QStringLiteral("[0-9]+"), text.trimmed())){
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
    if(!nam) return 0;

    QMimeData *mime = new QMimeData;

    QList<QUrl>        urls = Page::ExtractUrlFromHtml(m_SelectedHtml, m_CurrentBaseUrl, Page::HaveReference);
    if(urls.isEmpty()) urls = Page::ExtractUrlFromText(m_SelectedText, m_CurrentBaseUrl);
    if(urls.isEmpty()) urls = Page::ExtractUrlFromHtml(m_SelectedHtml, m_CurrentBaseUrl, Page::HaveSource);

    mime->setText(m_SelectedText);
    mime->setHtml(m_SelectedHtml);
    mime->setUrls(urls);

    return mime;
}

QMimeData *View::CreateMimeDataFromElement(NetworkAccessManager *nam){
    if(!nam) return 0;

    QUrl linkUrl  = m_ClickedElement ? m_ClickedElement->LinkUrl()  : QUrl();
    QUrl imageUrl = m_ClickedElement ? m_ClickedElement->ImageUrl() : QUrl();

    if(m_ClickedElement->TagName().toLower() != QStringLiteral("img")) imageUrl = QUrl();

    if(linkUrl.isEmpty() && imageUrl.isEmpty()) return 0;

    QMimeData *mime = new QMimeData;

    QList<QUrl> urls;

    if(!imageUrl.isEmpty()){
        // should be switchable?
        //urls << NetworkController::Download(nam, imageUrl, url(), NetworkController::TemporaryDirectory)->GetUrls();
        urls << imageUrl;
        mime->setText(imageUrl.toString());
        mime->setHtml(m_ClickedElement->ImageHtml());
    }
    // overwrite text and html, if linkUrl is not empty.
    if(!linkUrl.isEmpty()){
        // should be switchable?
        //urls << NetworkController::Download(nam, linkUrl, url(), NetworkController::TemporaryDirectory)->GetUrls();
        urls << linkUrl;
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
            QAction *action = menu->addAction(key, page, SLOT(SearchWith()));
            QUrl url = QUrl::fromEncoded(Page::GetSearchEngine(key).first().toLatin1());
            action->setIcon(Application::GetIcon(url.host()));
        }
    }
    return menu;
}

QMenu *View::OpenWithOtherBrowserMenu(){
    QMenu *menu = new QMenu(QObject::tr("OpenWithOtherBrowser"));
    if(!Application::BrowserPath_IE().isEmpty())       menu->addAction(Action(Page::_OpenWithIE));
    if(!Application::BrowserPath_Edge().isEmpty())     menu->addAction(Action(Page::_OpenWithEdge));
    if(!Application::BrowserPath_FF().isEmpty())       menu->addAction(Action(Page::_OpenWithFF));
    if(!Application::BrowserPath_Opera().isEmpty())    menu->addAction(Action(Page::_OpenWithOpera));
    if(!Application::BrowserPath_OPR().isEmpty())      menu->addAction(Action(Page::_OpenWithOPR));
    if(!Application::BrowserPath_Safari().isEmpty())   menu->addAction(Action(Page::_OpenWithSafari));
    if(!Application::BrowserPath_Chrome().isEmpty())   menu->addAction(Action(Page::_OpenWithChrome));
    if(!Application::BrowserPath_Sleipnir().isEmpty()) menu->addAction(Action(Page::_OpenWithSleipnir));
    if(!Application::BrowserPath_Vivaldi().isEmpty())  menu->addAction(Action(Page::_OpenWithVivaldi));
    if(!Application::BrowserPath_Custom().isEmpty())   menu->addAction(Action(Page::_OpenWithCustom));
    return menu;
}

QMenu *View::OpenLinkWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenLinkWithOtherBrowser"));
    if(!Application::BrowserPath_IE().isEmpty())       menu->addAction(Action(Page::_OpenLinkWithIE, data));
    if(!Application::BrowserPath_Edge().isEmpty())     menu->addAction(Action(Page::_OpenLinkWithEdge, data));
    if(!Application::BrowserPath_FF().isEmpty())       menu->addAction(Action(Page::_OpenLinkWithFF, data));
    if(!Application::BrowserPath_Opera().isEmpty())    menu->addAction(Action(Page::_OpenLinkWithOpera, data));
    if(!Application::BrowserPath_OPR().isEmpty())      menu->addAction(Action(Page::_OpenLinkWithOPR, data));
    if(!Application::BrowserPath_Safari().isEmpty())   menu->addAction(Action(Page::_OpenLinkWithSafari, data));
    if(!Application::BrowserPath_Chrome().isEmpty())   menu->addAction(Action(Page::_OpenLinkWithChrome, data));
    if(!Application::BrowserPath_Sleipnir().isEmpty()) menu->addAction(Action(Page::_OpenLinkWithSleipnir, data));
    if(!Application::BrowserPath_Vivaldi().isEmpty())  menu->addAction(Action(Page::_OpenLinkWithVivaldi, data));
    if(!Application::BrowserPath_Custom().isEmpty())   menu->addAction(Action(Page::_OpenLinkWithCustom, data));
    return menu;
}

QMenu *View::OpenImageWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenImageWithOtherBrowser"));
    if(!Application::BrowserPath_IE().isEmpty())       menu->addAction(Action(Page::_OpenImageWithIE, data));
    if(!Application::BrowserPath_Edge().isEmpty())     menu->addAction(Action(Page::_OpenImageWithEdge, data));
    if(!Application::BrowserPath_FF().isEmpty())       menu->addAction(Action(Page::_OpenImageWithFF, data));
    if(!Application::BrowserPath_Opera().isEmpty())    menu->addAction(Action(Page::_OpenImageWithOpera, data));
    if(!Application::BrowserPath_OPR().isEmpty())      menu->addAction(Action(Page::_OpenImageWithOPR, data));
    if(!Application::BrowserPath_Safari().isEmpty())   menu->addAction(Action(Page::_OpenImageWithSafari, data));
    if(!Application::BrowserPath_Chrome().isEmpty())   menu->addAction(Action(Page::_OpenImageWithChrome, data));
    if(!Application::BrowserPath_Sleipnir().isEmpty()) menu->addAction(Action(Page::_OpenImageWithSleipnir, data));
    if(!Application::BrowserPath_Vivaldi().isEmpty())  menu->addAction(Action(Page::_OpenImageWithVivaldi, data));
    if(!Application::BrowserPath_Custom().isEmpty())   menu->addAction(Action(Page::_OpenImageWithCustom, data));
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
            else menu->addAction(Action(str, data));
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
            else menu->addAction(Action(str, data));
        }
    }

    if(!SelectedText().isEmpty()){
        if(!menu->isEmpty()) menu->addSeparator();

        foreach(QString str, View::GetSelectionMenu().split(comma)){
            if     (str == QStringLiteral("Separator"))  menu->addSeparator();
            else if(str == QStringLiteral("SearchMenu")) menu->addMenu(SearchMenu());
            else menu->addAction(Action(str, data));
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
                menu->addAction(Action(Page::_ApplySource));
            } else {
                menu->addAction(Action(Page::_ViewSource));
            }
        } else menu->addAction(Action(str, data));
    }
}

void View::LoadSettings(){
    Settings &s = Application::GlobalSettings();

#ifdef QTWEBKIT
    // GlobalWebSettings
    QWebSettings *gws = QWebSettings::globalSettings();
#endif
    // GlobalWebEngineSettings
    QWebEngineSettings *gwes = QWebEngineSettings::defaultSettings();

    m_GestureMode               = s.value(QStringLiteral("webview/@GestureMode"),           4).value<int>();
    m_EnableDestinationInferrer = s.value(QStringLiteral("webview/@EnableDestinationInferrer"), false).value<bool>();
    m_EnableLoadHack            = s.value(QStringLiteral("webview/@EnableLoadHack"),    false).value<bool>()
        ||                        s.value(QStringLiteral("webview/@EnableHistNode"),    false).value<bool>();
    m_EnableDragHack            = s.value(QStringLiteral("webview/@EnableDragHack"),    false).value<bool>()
        ||                        s.value(QStringLiteral("webview/@EnableDragGesture"), false).value<bool>();
    m_ActivateNewViewDefault    = s.value(QStringLiteral("webview/@ActivateNewViewDefault"), true).value<bool>();
    m_NavigationBySpaceKey      = s.value(QStringLiteral("webview/@NavigationBySpaceKey"), false).value<bool>();

    QString operation = s.value(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNode")).value<QString>();
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

#ifdef QTWEBKIT
    gws->setAttribute(QWebSettings::AcceleratedCompositingEnabled,     s.value(QStringLiteral("webview/preferences/AcceleratedCompositingEnabled"),     gws->testAttribute(QWebSettings::AcceleratedCompositingEnabled)          ).value<bool>());
    gws->setAttribute(QWebSettings::AutoLoadImages,                    s.value(QStringLiteral("webview/preferences/AutoLoadImages"),                    gws->testAttribute(QWebSettings::AutoLoadImages)                         ).value<bool>());
    gws->setAttribute(QWebSettings::DeveloperExtrasEnabled,            s.value(QStringLiteral("webview/preferences/DeveloperExtrasEnabled"),          /*gws->testAttribute(QWebSettings::DeveloperExtrasEnabled)*/          true ).value<bool>());
    gws->setAttribute(QWebSettings::DnsPrefetchEnabled,                s.value(QStringLiteral("webview/preferences/DnsPrefetchEnabled"),                gws->testAttribute(QWebSettings::DnsPrefetchEnabled)                     ).value<bool>());
    gws->setAttribute(QWebSettings::FrameFlatteningEnabled,            s.value(QStringLiteral("webview/preferences/FrameFlatteningEnabled"),            gws->testAttribute(QWebSettings::FrameFlatteningEnabled)                 ).value<bool>());
    gws->setAttribute(QWebSettings::JavaEnabled,                       s.value(QStringLiteral("webview/preferences/JavaEnabled"),                       gws->testAttribute(QWebSettings::JavaEnabled)                            ).value<bool>());
    gws->setAttribute(QWebSettings::JavascriptCanAccessClipboard,      s.value(QStringLiteral("webview/preferences/JavascriptCanAccessClipboard"),      gws->testAttribute(QWebSettings::JavascriptCanAccessClipboard)           ).value<bool>());
    gws->setAttribute(QWebSettings::JavascriptCanCloseWindows,         s.value(QStringLiteral("webview/preferences/JavascriptCanCloseWindows"),       /*gws->testAttribute(QWebSettings::JavascriptCanCloseWindows)*/       true ).value<bool>());
    gws->setAttribute(QWebSettings::JavascriptCanOpenWindows,          s.value(QStringLiteral("webview/preferences/JavascriptCanOpenWindows"),        /*gws->testAttribute(QWebSettings::JavascriptCanOpenWindows)*/        true ).value<bool>());
    gws->setAttribute(QWebSettings::JavascriptEnabled,                 s.value(QStringLiteral("webview/preferences/JavascriptEnabled"),                 gws->testAttribute(QWebSettings::JavascriptEnabled)                      ).value<bool>());
    gws->setAttribute(QWebSettings::LinksIncludedInFocusChain,         s.value(QStringLiteral("webview/preferences/LinksIncludedInFocusChain"),         gws->testAttribute(QWebSettings::LinksIncludedInFocusChain)              ).value<bool>());
    gws->setAttribute(QWebSettings::LocalContentCanAccessFileUrls,     s.value(QStringLiteral("webview/preferences/LocalContentCanAccessFileUrls"),     gws->testAttribute(QWebSettings::LocalContentCanAccessFileUrls)          ).value<bool>());
    gws->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls,   s.value(QStringLiteral("webview/preferences/LocalContentCanAccessRemoteUrls"),   gws->testAttribute(QWebSettings::LocalContentCanAccessRemoteUrls)        ).value<bool>());
  //gws->setAttribute(QWebSettings::LocalStorageDatabaseEnabled,       s.value(QStringLiteral("webview/preferences/LocalStorageDatabaseEnabled"),       gws->testAttribute(QWebSettings::LocalStorageDatabaseEnabled)            ).value<bool>());
    gws->setAttribute(QWebSettings::LocalStorageEnabled,               s.value(QStringLiteral("webview/preferences/LocalStorageEnabled"),               gws->testAttribute(QWebSettings::LocalStorageEnabled)                    ).value<bool>());
    gws->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled,     s.value(QStringLiteral("webview/preferences/OfflineStorageDatabaseEnabled"),     gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled)          ).value<bool>());
    gws->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, s.value(QStringLiteral("webview/preferences/OfflineWebApplicationCacheEnabled"), gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled)      ).value<bool>());
    gws->setAttribute(QWebSettings::PluginsEnabled,                    s.value(QStringLiteral("webview/preferences/PluginsEnabled"),                  /*gws->testAttribute(QWebSettings::PluginsEnabled)*/                  true ).value<bool>());
    gws->setAttribute(QWebSettings::PrintElementBackgrounds,           s.value(QStringLiteral("webview/preferences/PrintElementBackgrounds"),           gws->testAttribute(QWebSettings::PrintElementBackgrounds)                ).value<bool>());
    gws->setAttribute(QWebSettings::PrivateBrowsingEnabled,            s.value(QStringLiteral("webview/preferences/PrivateBrowsingEnabled"),            gws->testAttribute(QWebSettings::PrivateBrowsingEnabled)                 ).value<bool>());
    gws->setAttribute(QWebSettings::SiteSpecificQuirksEnabled,         s.value(QStringLiteral("webview/preferences/SiteSpecificQuirksEnabled"),         gws->testAttribute(QWebSettings::SiteSpecificQuirksEnabled)              ).value<bool>());
    gws->setAttribute(QWebSettings::SpatialNavigationEnabled,          s.value(QStringLiteral("webview/preferences/SpatialNavigationEnabled"),          gws->testAttribute(QWebSettings::SpatialNavigationEnabled)               ).value<bool>());
    gws->setAttribute(QWebSettings::TiledBackingStoreEnabled,          s.value(QStringLiteral("webview/preferences/TiledBackingStoreEnabled"),          gws->testAttribute(QWebSettings::TiledBackingStoreEnabled)               ).value<bool>());
    gws->setAttribute(QWebSettings::XSSAuditingEnabled,                s.value(QStringLiteral("webview/preferences/XSSAuditingEnabled"),                gws->testAttribute(QWebSettings::XSSAuditingEnabled)                     ).value<bool>());
    gws->setAttribute(QWebSettings::ZoomTextOnly,                      s.value(QStringLiteral("webview/preferences/ZoomTextOnly"),                      gws->testAttribute(QWebSettings::ZoomTextOnly)                           ).value<bool>());
    gws->setAttribute(QWebSettings::CSSGridLayoutEnabled,              s.value(QStringLiteral("webview/preferences/CSSGridLayoutEnabled"),              gws->testAttribute(QWebSettings::CSSGridLayoutEnabled)                   ).value<bool>());
    gws->setAttribute(QWebSettings::CSSRegionsEnabled,                 s.value(QStringLiteral("webview/preferences/CSSRegionsEnabled"),                 gws->testAttribute(QWebSettings::CSSRegionsEnabled)                      ).value<bool>());
    gws->setAttribute(QWebSettings::CaretBrowsingEnabled,              s.value(QStringLiteral("webview/preferences/CaretBrowsingEnabled"),              gws->testAttribute(QWebSettings::CaretBrowsingEnabled)                   ).value<bool>());
    gws->setAttribute(QWebSettings::HyperlinkAuditingEnabled,          s.value(QStringLiteral("webview/preferences/HyperlinkAuditingEnabled"),          gws->testAttribute(QWebSettings::HyperlinkAuditingEnabled)               ).value<bool>());
    gws->setAttribute(QWebSettings::NotificationsEnabled,              s.value(QStringLiteral("webview/preferences/NotificationsEnabled"),              gws->testAttribute(QWebSettings::NotificationsEnabled)                   ).value<bool>());
    gws->setAttribute(QWebSettings::ScrollAnimatorEnabled,             s.value(QStringLiteral("webview/preferences/ScrollAnimatorEnabled"),             gws->testAttribute(QWebSettings::ScrollAnimatorEnabled)                  ).value<bool>());
    gws->setAttribute(QWebSettings::WebAudioEnabled,                   s.value(QStringLiteral("webview/preferences/WebAudioEnabled"),                   gws->testAttribute(QWebSettings::WebAudioEnabled)                        ).value<bool>());
    gws->setAttribute(QWebSettings::WebGLEnabled,                      s.value(QStringLiteral("webview/preferences/WebGLEnabled"),                      gws->testAttribute(QWebSettings::WebGLEnabled)                           ).value<bool>());
    gws->setAttribute(QWebSettings::Accelerated2dCanvasEnabled,        s.value(QStringLiteral("webview/preferences/Accelerated2dCanvasEnabled"),        gws->testAttribute(QWebSettings::Accelerated2dCanvasEnabled)             ).value<bool>());
  //gws->setAttribute(QWebSettings::ErrorPageEnabled,                  s.value(QStringLiteral("webview/preferences/ErrorPageEnabled"),                  gws->testAttribute(QWebSettings::ErrorPageEnabled)                       ).value<bool>());
  //gws->setAttribute(QWebSettings::FullScreenSupportEnabled,          s.value(QStringLiteral("webview/preferences/FullScreenSupportEnabled"),          gws->testAttribute(QWebSettings::FullScreenSupportEnabled)               ).value<bool>());
#endif //ifdef QTWEBKIT
  //gwes->setAttribute(QWebEngineSettings::AcceleratedCompositingEnabled,     s.value(QStringLiteral("webview/preferences/AcceleratedCompositingEnabled"),     gwes->testAttribute(QWebEngineSettings::AcceleratedCompositingEnabled)    ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::AutoLoadImages,                    s.value(QStringLiteral("webview/preferences/AutoLoadImages"),                    gwes->testAttribute(QWebEngineSettings::AutoLoadImages)                   ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled,            s.value(QStringLiteral("webview/preferences/DeveloperExtrasEnabled"),            gwes->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled)           ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,                s.value(QStringLiteral("webview/preferences/DnsPrefetchEnabled"),                gwes->testAttribute(QWebEngineSettings::DnsPrefetchEnabled)               ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::FrameFlatteningEnabled,            s.value(QStringLiteral("webview/preferences/FrameFlatteningEnabled"),            gwes->testAttribute(QWebEngineSettings::FrameFlatteningEnabled)           ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::JavaEnabled,                       s.value(QStringLiteral("webview/preferences/JavaEnabled"),                       gwes->testAttribute(QWebEngineSettings::JavaEnabled)                      ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard,      s.value(QStringLiteral("webview/preferences/JavascriptCanAccessClipboard"),      gwes->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard)     ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::JavascriptCanCloseWindows,         s.value(QStringLiteral("webview/preferences/JavascriptCanCloseWindows"),         gwes->testAttribute(QWebEngineSettings::JavascriptCanCloseWindows)        ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,          s.value(QStringLiteral("webview/preferences/JavascriptCanOpenWindows"),        /*gwes->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows)*/  true ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptEnabled,                 s.value(QStringLiteral("webview/preferences/JavascriptEnabled"),                 gwes->testAttribute(QWebEngineSettings::JavascriptEnabled)                ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain,         s.value(QStringLiteral("webview/preferences/LinksIncludedInFocusChain"),         gwes->testAttribute(QWebEngineSettings::LinksIncludedInFocusChain)        ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,     s.value(QStringLiteral("webview/preferences/LocalContentCanAccessFileUrls"),     gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls)    ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,   s.value(QStringLiteral("webview/preferences/LocalContentCanAccessRemoteUrls"),   gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls)  ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled,       s.value(QStringLiteral("webview/preferences/LocalStorageDatabaseEnabled"),       gwes->testAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled)      ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LocalStorageEnabled,               s.value(QStringLiteral("webview/preferences/LocalStorageEnabled"),               gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled)              ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled,     s.value(QStringLiteral("webview/preferences/OfflineStorageDatabaseEnabled"),     gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled)    ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled, s.value(QStringLiteral("webview/preferences/OfflineWebApplicationCacheEnabled"), gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled)).value<bool>());
#if QT_VERSION >= 0x050600
    gwes->setAttribute(QWebEngineSettings::PluginsEnabled,                    s.value(QStringLiteral("webview/preferences/PluginsEnabled"),                  /*gwes->testAttribute(QWebEngineSettings::PluginsEnabled)*/            true ).value<bool>());
#endif
  //gwes->setAttribute(QWebEngineSettings::PrintElementBackgrounds,           s.value(QStringLiteral("webview/preferences/PrintElementBackgrounds"),           gwes->testAttribute(QWebEngineSettings::PrintElementBackgrounds)          ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::PrivateBrowsingEnabled,            s.value(QStringLiteral("webview/preferences/PrivateBrowsingEnabled"),            gwes->testAttribute(QWebEngineSettings::PrivateBrowsingEnabled)           ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::SiteSpecificQuirksEnabled,         s.value(QStringLiteral("webview/preferences/SiteSpecificQuirksEnabled"),         gwes->testAttribute(QWebEngineSettings::SiteSpecificQuirksEnabled)        ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::SpatialNavigationEnabled,          s.value(QStringLiteral("webview/preferences/SpatialNavigationEnabled"),          gwes->testAttribute(QWebEngineSettings::SpatialNavigationEnabled)         ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::TiledBackingStoreEnabled,          s.value(QStringLiteral("webview/preferences/TiledBackingStoreEnabled"),          gwes->testAttribute(QWebEngineSettings::TiledBackingStoreEnabled)         ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::XSSAuditingEnabled,                s.value(QStringLiteral("webview/preferences/XSSAuditingEnabled"),                gwes->testAttribute(QWebEngineSettings::XSSAuditingEnabled)               ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::ZoomTextOnly,                      s.value(QStringLiteral("webview/preferences/ZoomTextOnly"),                      gwes->testAttribute(QWebEngineSettings::ZoomTextOnly)                     ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::CSSGridLayoutEnabled,              s.value(QStringLiteral("webview/preferences/CSSGridLayoutEnabled"),              gwes->testAttribute(QWebEngineSettings::CSSGridLayoutEnabled)             ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::CSSRegionsEnabled,                 s.value(QStringLiteral("webview/preferences/CSSRegionsEnabled"),                 gwes->testAttribute(QWebEngineSettings::CSSRegionsEnabled)                ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::CaretBrowsingEnabled,              s.value(QStringLiteral("webview/preferences/CaretBrowsingEnabled"),              gwes->testAttribute(QWebEngineSettings::CaretBrowsingEnabled)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled,          s.value(QStringLiteral("webview/preferences/HyperlinkAuditingEnabled"),          gwes->testAttribute(QWebEngineSettings::HyperlinkAuditingEnabled)         ).value<bool>());
  //gwes->setAttribute(QWebEngineSettings::NotificationsEnabled,              s.value(QStringLiteral("webview/preferences/NotificationsEnabled"),              gwes->testAttribute(QWebEngineSettings::NotificationsEnabled)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,             s.value(QStringLiteral("webview/preferences/ScrollAnimatorEnabled"),             gwes->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled)            ).value<bool>());
#if QT_VERSION >= 0x050700
    gwes->setAttribute(QWebEngineSettings::ScreenCaptureEnabled,              s.value(QStringLiteral("webview/preferences/ScreenCaptureEnabled"),              gwes->testAttribute(QWebEngineSettings::ScreenCaptureEnabled)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::WebAudioEnabled,                   s.value(QStringLiteral("webview/preferences/WebAudioEnabled"),                   gwes->testAttribute(QWebEngineSettings::WebAudioEnabled)                  ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::WebGLEnabled,                      s.value(QStringLiteral("webview/preferences/WebGLEnabled"),                      gwes->testAttribute(QWebEngineSettings::WebGLEnabled)                     ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,        s.value(QStringLiteral("webview/preferences/Accelerated2dCanvasEnabled"),        gwes->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled)       ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::AutoLoadIconsForPage,              s.value(QStringLiteral("webview/preferences/AutoLoadIconsForPage"),              gwes->testAttribute(QWebEngineSettings::AutoLoadIconsForPage)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::TouchIconsEnabled,                 s.value(QStringLiteral("webview/preferences/TouchIconsEnabled"),                 gwes->testAttribute(QWebEngineSettings::TouchIconsEnabled)                ).value<bool>());
#endif
    gwes->setAttribute(QWebEngineSettings::ErrorPageEnabled,                  s.value(QStringLiteral("webview/preferences/ErrorPageEnabled"),                  gwes->testAttribute(QWebEngineSettings::ErrorPageEnabled)                 ).value<bool>());
#if QT_VERSION >= 0x050600
    gwes->setAttribute(QWebEngineSettings::FullScreenSupportEnabled,          s.value(QStringLiteral("webview/preferences/FullScreenSupportEnabled"),        /*gwes->testAttribute(QWebEngineSettings::FullScreenSupportEnabled)*/  true ).value<bool>());
#endif

#ifdef QTWEBKIT
    if(gws->testAttribute(QWebSettings::LocalStorageEnabled) ||
       gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled) ||
       gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled)){

        QWebSettings::enablePersistentStorage();
    }

    gws->setDefaultTextEncoding
        (s.value(QStringLiteral("webview/detail/DefaultTextEncoding"),
                  gws->defaultTextEncoding()).value<QString>());

    gws->setMaximumPagesInCache
        (s.value(QStringLiteral("webview/detail/MaximumPagesInCache"),
                  gws->maximumPagesInCache()).value<int>());

    if(gws->testAttribute(QWebSettings::LocalStorageEnabled)){
        QString iconDatabasePath =
            s.value(QStringLiteral("webview/detail/IconDatabasePath"),
                     gws->iconDatabasePath()).value<QString>();
        QString localStoragePath =
            s.value(QStringLiteral("webview/detail/LocalStoragePath"),
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
            s.value(QStringLiteral("webview/detail/OfflineStoragePath"),
                     gws->offlineStoragePath()).value<QString>();
        if(!path.isEmpty()){
            QDir dir = path;
            dir.mkpath(path);
            gws->setOfflineStoragePath(path);
        }
        gws->setOfflineStorageDefaultQuota
            (s.value(QStringLiteral("webview/detail/OfflineStorageDefaultQuota"),
                      gws->offlineStorageDefaultQuota()).value<qint64>());
    }

    if(gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled)){
        QString path =
            s.value(QStringLiteral("webview/detail/OfflineWebApplicationCachePath"),
                     gws->offlineWebApplicationCachePath()).value<QString>();
        if(!path.isEmpty()){
            QDir dir = path;
            dir.mkpath(path);
            // setting value to crash...
            //gws->setOfflineWebApplicationCachePath(path);
        }
        gws->setOfflineWebApplicationCacheQuota
            (s.value(QStringLiteral("webview/detail/OfflineWebApplicationCacheQuota"),
                      gws->offlineWebApplicationCacheQuota()).value<qint64>());
    }

    gws->setUserStyleSheetUrl
        (s.value(QStringLiteral("webview/detail/UserStyleSheetUrl"),
                  gws->userStyleSheetUrl()).value<QUrl>());
#endif //ifdef QTWEBKIT
    //if(gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled) ||
    //   gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled) ||
    //   gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled)){
    //
    //    QWebEngineSettings::enablePersistentStorage();
    //}

    gwes->setDefaultTextEncoding
        (s.value(QStringLiteral("webview/detail/DefaultTextEncoding"),
                  gwes->defaultTextEncoding()).value<QString>());

    //gwes->setMaximumPagesInCache
    //    (s.value(QStringLiteral("webview/detail/MaximumPagesInCache"),
    //              gwes->maximumPagesInCache()).value<int>());
    //
    //if(gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled)){
    //    QString iconDatabasePath =
    //        s.value(QStringLiteral("webview/detail/IconDatabasePath"),
    //                 gwes->iconDatabasePath()).value<QString>();
    //    QString localStoragePath =
    //        s.value(QStringLiteral("webview/detail/LocalStoragePath"),
    //                 gwes->localStoragePath()).value<QString>();
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
    //        s.value(QStringLiteral("webview/detail/OfflineStoragePath"),
    //                 gwes->offlineStoragePath()).value<QString>();
    //    if(!path.isEmpty()){
    //        QDir dir = path;
    //        dir.mkpath(path);
    //        gwes->setOfflineStoragePath(path);
    //    }
    //    gwes->setOfflineStorageDefaultQuota
    //        (s.value(QStringLiteral("webview/detail/OfflineStorageDefaultQuota"),
    //                  gwes->offlineStorageDefaultQuota()).value<qint64>());
    //}
    //
    //if(gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled)){
    //    QString path =
    //        s.value(QStringLiteral("webview/detail/OfflineWebApplicationCachePath"),
    //                 gwes->offlineWebApplicationCachePath()).value<QString>();
    //    if(!path.isEmpty()){
    //        QDir dir = path;
    //        dir.mkpath(path);
    //        gwes->setOfflineWebApplicationCachePath(path);
    //    }
    //    gwes->setOfflineWebApplicationCacheQuota
    //        (s.value(QStringLiteral("webview/detail/OfflineWebApplicationCacheQuota"),
    //                  gwes->offlineWebApplicationCacheQuota()).value<qint64>());
    //}
    //
    //gwes->setUserStyleSheetUrl
    //    (s.value(QStringLiteral("webview/detail/UserStyleSheetUrl"),
    //              gwes->userStyleSheetUrl()).value<QUrl>());

    QString policy = s.value(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AlwaysAllowThirdPartyCookies")).value<QString>();
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

#ifdef QTWEBKIT
    gws->setFontFamily(QWebSettings::StandardFont,         s.value(QStringLiteral("webview/font/StandardFont"),           gws->fontFamily(QWebSettings::StandardFont)         ).value<QString>());
    gws->setFontFamily(QWebSettings::SansSerifFont,        s.value(QStringLiteral("webview/font/SansSerifFont"),          gws->fontFamily(QWebSettings::SansSerifFont)        ).value<QString>());
    gws->setFontFamily(QWebSettings::SerifFont,            s.value(QStringLiteral("webview/font/SerifFont"),              gws->fontFamily(QWebSettings::SerifFont)            ).value<QString>());
    gws->setFontFamily(QWebSettings::FixedFont,            s.value(QStringLiteral("webview/font/FixedFont"),              gws->fontFamily(QWebSettings::FixedFont)            ).value<QString>());
    gws->setFontFamily(QWebSettings::CursiveFont,          s.value(QStringLiteral("webview/font/CursiveFont"),            gws->fontFamily(QWebSettings::CursiveFont)          ).value<QString>());
    gws->setFontFamily(QWebSettings::FantasyFont,          s.value(QStringLiteral("webview/font/FantasyFont"),            gws->fontFamily(QWebSettings::FantasyFont)          ).value<QString>());
    gws->setFontSize(QWebSettings::MinimumFontSize,        s.value(QStringLiteral("webview/font/MinimumFontSize"),        gws->fontSize(QWebSettings::MinimumFontSize)        ).value<int>());
    gws->setFontSize(QWebSettings::MinimumLogicalFontSize, s.value(QStringLiteral("webview/font/MinimumLogicalFontSize"), gws->fontSize(QWebSettings::MinimumLogicalFontSize) ).value<int>());
    gws->setFontSize(QWebSettings::DefaultFontSize,        s.value(QStringLiteral("webview/font/DefaultFontSize"),        gws->fontSize(QWebSettings::DefaultFontSize)        ).value<int>());
    gws->setFontSize(QWebSettings::DefaultFixedFontSize,   s.value(QStringLiteral("webview/font/DefaultFixedFontSize"),   gws->fontSize(QWebSettings::DefaultFixedFontSize)   ).value<int>());
#endif
    gwes->setFontFamily(QWebEngineSettings::StandardFont,         s.value(QStringLiteral("webview/font/StandardFont"),           gwes->fontFamily(QWebEngineSettings::StandardFont)         ).value<QString>());
    gwes->setFontFamily(QWebEngineSettings::SansSerifFont,        s.value(QStringLiteral("webview/font/SansSerifFont"),          gwes->fontFamily(QWebEngineSettings::SansSerifFont)        ).value<QString>());
    gwes->setFontFamily(QWebEngineSettings::SerifFont,            s.value(QStringLiteral("webview/font/SerifFont"),              gwes->fontFamily(QWebEngineSettings::SerifFont)            ).value<QString>());
    gwes->setFontFamily(QWebEngineSettings::FixedFont,            s.value(QStringLiteral("webview/font/FixedFont"),              gwes->fontFamily(QWebEngineSettings::FixedFont)            ).value<QString>());
    gwes->setFontFamily(QWebEngineSettings::CursiveFont,          s.value(QStringLiteral("webview/font/CursiveFont"),            gwes->fontFamily(QWebEngineSettings::CursiveFont)          ).value<QString>());
    gwes->setFontFamily(QWebEngineSettings::FantasyFont,          s.value(QStringLiteral("webview/font/FantasyFont"),            gwes->fontFamily(QWebEngineSettings::FantasyFont)          ).value<QString>());
#if QT_VERSION >= 0x050700
    gwes->setFontFamily(QWebEngineSettings::PictographFont,       s.value(QStringLiteral("webview/font/PictographFont"),         gwes->fontFamily(QWebEngineSettings::PictographFont)       ).value<QString>());
#endif
    gwes->setFontSize(QWebEngineSettings::MinimumFontSize,        s.value(QStringLiteral("webview/font/MinimumFontSize"),        gwes->fontSize(QWebEngineSettings::MinimumFontSize)        ).value<int>());
    gwes->setFontSize(QWebEngineSettings::MinimumLogicalFontSize, s.value(QStringLiteral("webview/font/MinimumLogicalFontSize"), gwes->fontSize(QWebEngineSettings::MinimumLogicalFontSize) ).value<int>());
    gwes->setFontSize(QWebEngineSettings::DefaultFontSize,        s.value(QStringLiteral("webview/font/DefaultFontSize"),        gwes->fontSize(QWebEngineSettings::DefaultFontSize)        ).value<int>());
    gwes->setFontSize(QWebEngineSettings::DefaultFixedFontSize,   s.value(QStringLiteral("webview/font/DefaultFixedFontSize"),   gwes->fontSize(QWebEngineSettings::DefaultFixedFontSize)   ).value<int>());

    m_LinkMenu = s.value(QStringLiteral("webview/menu/LinkMenu"), QStringLiteral(
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

    m_ImageMenu = s.value(QStringLiteral("webview/menu/ImageMenu"), QStringLiteral(
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

    m_SelectionMenu = s.value(QStringLiteral("webview/menu/SelectionMenu"), QStringLiteral(
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

    m_RegularMenu = s.value(QStringLiteral("webview/menu/RegularMenu"), QStringLiteral(
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

    {   QStringList keys = s.allKeys(QStringLiteral("webview/keymap"));

        if(keys.isEmpty()){
            /* default key map. */
            WEBVIEW_KEYMAP
        } else {
            if(!m_KeyMap.isEmpty()) m_KeyMap.clear();
            foreach(QString key, keys){
                QString last = key.split(QStringLiteral("/")).last();
                if(last.isEmpty()) continue;
                m_KeyMap[Application::MakeKeySequence(last)] =
                    s.value(key, QStringLiteral("NoAction")).value<QString>()
                    // cannot use slashes on QSettings.
                      .replace(QStringLiteral("Backslash"), QStringLiteral("\\"))
                      .replace(QStringLiteral("Slash"), QStringLiteral("/"));
            }
        }
    }
    {   QStringList keys = s.allKeys(QStringLiteral("webview/mouse"));

        if(keys.isEmpty()){
            /* default mouse map. */
            WEBVIEW_MOUSEMAP
        } else {
            if(!m_MouseMap.isEmpty()) m_MouseMap.clear();
            foreach(QString key, keys){
                QString last = key.split(QStringLiteral("/")).last();
                if(last.isEmpty()) continue;
                m_MouseMap[last] =
                    s.value(key, QStringLiteral("NoAction")).value<QString>();
            }
        }
    }
    {   QStringList keys = s.allKeys(QStringLiteral("webview/rightgesture"));

        if(keys.isEmpty()){
            /* default right gesture. */
            WEBVIEW_RIGHTGESTURE
        } else {
            if(!m_RightGestureMap.isEmpty()) m_RightGestureMap.clear();
            foreach(QString key, keys){
                QString last = key.split(QStringLiteral("/")).last();
                if(last.isEmpty()) continue;
                m_RightGestureMap[last] =
                    s.value(key, QStringLiteral("NoAction")).value<QString>();
            }
        }
    }
    {   QStringList keys = s.allKeys(QStringLiteral("webview/leftgesture"));

        if(keys.isEmpty()){
            /* default left gesture. */
            WEBVIEW_LEFTGESTURE
        } else {
            if(!m_LeftGestureMap.isEmpty()) m_LeftGestureMap.clear();
            foreach(QString key, keys){
                QString last = key.split(QStringLiteral("/")).last();
                if(last.isEmpty()) continue;
                m_LeftGestureMap[last] =
                    s.value(key, QStringLiteral("NoAction")).value<QString>();
            }
        }
    }
    {   QStringList keys = s.allKeys(QStringLiteral("webview/searchengine"));

        if(!Page::GetSearchEngineMap().isEmpty()) Page::ClearSearchEngine();

        foreach(QString key, keys){
            QString last = key.split(QStringLiteral("/")).last();
            QVariant searchengine = s.value(key);
            if(searchengine.canConvert(QVariant::String))
                Page::RegisterSearchEngine(last, QStringList() << searchengine.value<QString>());
            if(searchengine.canConvert(QVariant::StringList))
                Page::RegisterSearchEngine(last, searchengine.value<QStringList>());
        }
        if(Page::GetSearchEngineMap().isEmpty()){
            Page::RegisterDefaultSearchEngines();
        }
    }
    {   QStringList keys = s.allKeys(QStringLiteral("webview/bookmarklet"));

        if(!Page::GetBookmarkletMap().isEmpty()) Page::ClearBookmarklet();
        foreach(QString key, keys){
            QString last = key.split(QStringLiteral("/")).last();
            QVariant bookmarklet = s.value(key);
            if(bookmarklet.canConvert(QVariant::String))
                Page::RegisterBookmarklet(last, QStringList() << bookmarklet.value<QString>());
            if(bookmarklet.canConvert(QVariant::StringList))
                Page::RegisterBookmarklet(last, bookmarklet.value<QStringList>());
        }
    }
}

void View::SaveSettings(){
    Settings &s = Application::GlobalSettings();

#ifdef QTWEBKIT
    // GlobalWebSettings
    QWebSettings *gws = QWebSettings::globalSettings();
#endif
    // GlobalWebEngineSettings
    QWebEngineSettings *gwes = QWebEngineSettings::defaultSettings();

    s.setValue(QStringLiteral("webview/@GestureMode"),          m_GestureMode);
    s.setValue(QStringLiteral("webview/@EnableDestinationInferrer"), m_EnableDestinationInferrer);
    s.setValue(QStringLiteral("webview/@EnableHistNode"),       m_EnableLoadHack);
    s.setValue(QStringLiteral("webview/@EnableDragGesture"),    m_EnableDragHack);
    s.setValue(QStringLiteral("webview/@ActivateNewViewDefault"), m_ActivateNewViewDefault);
    s.setValue(QStringLiteral("webview/@NavigationBySpaceKey"), m_NavigationBySpaceKey);

    Page::OpenCommandOperation operation = Page::GetOpenOparation();
    if(operation == Page::InNewViewNode)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNode"));
    if(operation == Page::InNewHistNode)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewHistNode"));
    if(operation == Page::InNewDirectory) s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewDirectory"));
    if(operation == Page::OnRoot)         s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("OnRoot"));

    if(operation == Page::InNewViewNodeBackground)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNodeBackground"));
    if(operation == Page::InNewHistNodeBackground)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewHistNodeBackground"));
    if(operation == Page::InNewDirectoryBackground) s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewDirectoryBackground"));
    if(operation == Page::OnRootBackground)         s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("OnRootBackground"));

    if(operation == Page::InNewViewNodeNewWindow)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNodeNewWindow"));
    if(operation == Page::InNewHistNodeNewWindow)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewHistNodeNewWindow"));
    if(operation == Page::InNewDirectoryNewWindow) s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewDirectoryNewWindow"));
    if(operation == Page::OnRootNewWindow)         s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("OnRootNewWindow"));

#ifdef QTWEBKIT
    s.setValue(QStringLiteral("webview/preferences/AcceleratedCompositingEnabled"),     gws->testAttribute(QWebSettings::AcceleratedCompositingEnabled)     );
    s.setValue(QStringLiteral("webview/preferences/AutoLoadImages"),                    gws->testAttribute(QWebSettings::AutoLoadImages)                    );
    s.setValue(QStringLiteral("webview/preferences/DeveloperExtrasEnabled"),            gws->testAttribute(QWebSettings::DeveloperExtrasEnabled)            );
    s.setValue(QStringLiteral("webview/preferences/DnsPrefetchEnabled"),                gws->testAttribute(QWebSettings::DnsPrefetchEnabled)                );
    s.setValue(QStringLiteral("webview/preferences/FrameFlatteningEnabled"),            gws->testAttribute(QWebSettings::FrameFlatteningEnabled)            );
    s.setValue(QStringLiteral("webview/preferences/JavaEnabled"),                       gws->testAttribute(QWebSettings::JavaEnabled)                       );
    s.setValue(QStringLiteral("webview/preferences/JavascriptCanAccessClipboard"),      gws->testAttribute(QWebSettings::JavascriptCanAccessClipboard)      );
    s.setValue(QStringLiteral("webview/preferences/JavascriptCanCloseWindows"),         gws->testAttribute(QWebSettings::JavascriptCanCloseWindows)         );
    s.setValue(QStringLiteral("webview/preferences/JavascriptCanOpenWindows"),          gws->testAttribute(QWebSettings::JavascriptCanOpenWindows)          );
    s.setValue(QStringLiteral("webview/preferences/JavascriptEnabled"),                 gws->testAttribute(QWebSettings::JavascriptEnabled)                 );
    s.setValue(QStringLiteral("webview/preferences/LinksIncludedInFocusChain"),         gws->testAttribute(QWebSettings::LinksIncludedInFocusChain)         );
    s.setValue(QStringLiteral("webview/preferences/LocalContentCanAccessFileUrls"),     gws->testAttribute(QWebSettings::LocalContentCanAccessFileUrls)     );
    s.setValue(QStringLiteral("webview/preferences/LocalContentCanAccessRemoteUrls"),   gws->testAttribute(QWebSettings::LocalContentCanAccessRemoteUrls)   );
    s.setValue(QStringLiteral("webview/preferences/LocalStorageDatabaseEnabled"),       gws->testAttribute(QWebSettings::LocalStorageDatabaseEnabled)       );
    s.setValue(QStringLiteral("webview/preferences/LocalStorageEnabled"),               gws->testAttribute(QWebSettings::LocalStorageEnabled)               );
    s.setValue(QStringLiteral("webview/preferences/OfflineStorageDatabaseEnabled"),     gws->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled)     );
    s.setValue(QStringLiteral("webview/preferences/OfflineWebApplicationCacheEnabled"), gws->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled) );
    s.setValue(QStringLiteral("webview/preferences/PluginsEnabled"),                    gws->testAttribute(QWebSettings::PluginsEnabled)                    );
    s.setValue(QStringLiteral("webview/preferences/PrintElementBackgrounds"),           gws->testAttribute(QWebSettings::PrintElementBackgrounds)           );
    s.setValue(QStringLiteral("webview/preferences/PrivateBrowsingEnabled"),            gws->testAttribute(QWebSettings::PrivateBrowsingEnabled)            );
    s.setValue(QStringLiteral("webview/preferences/SiteSpecificQuirksEnabled"),         gws->testAttribute(QWebSettings::SiteSpecificQuirksEnabled)         );
    s.setValue(QStringLiteral("webview/preferences/SpatialNavigationEnabled"),          gws->testAttribute(QWebSettings::SpatialNavigationEnabled)          );
    s.setValue(QStringLiteral("webview/preferences/TiledBackingStoreEnabled"),          gws->testAttribute(QWebSettings::TiledBackingStoreEnabled)          );
    s.setValue(QStringLiteral("webview/preferences/XSSAuditingEnabled"),                gws->testAttribute(QWebSettings::XSSAuditingEnabled)                );
    s.setValue(QStringLiteral("webview/preferences/ZoomTextOnly"),                      gws->testAttribute(QWebSettings::ZoomTextOnly)                      );
    s.setValue(QStringLiteral("webview/preferences/CSSGridLayoutEnabled"),              gws->testAttribute(QWebSettings::CSSGridLayoutEnabled)              );
    s.setValue(QStringLiteral("webview/preferences/CSSRegionsEnabled"),                 gws->testAttribute(QWebSettings::CSSRegionsEnabled)                 );
    s.setValue(QStringLiteral("webview/preferences/CaretBrowsingEnabled"),              gws->testAttribute(QWebSettings::CaretBrowsingEnabled)              );
    s.setValue(QStringLiteral("webview/preferences/HyperlinkAuditingEnabled"),          gws->testAttribute(QWebSettings::HyperlinkAuditingEnabled)          );
    s.setValue(QStringLiteral("webview/preferences/NotificationsEnabled"),              gws->testAttribute(QWebSettings::NotificationsEnabled)              );
    s.setValue(QStringLiteral("webview/preferences/ScrollAnimatorEnabled"),             gws->testAttribute(QWebSettings::ScrollAnimatorEnabled)             );
    s.setValue(QStringLiteral("webview/preferences/WebAudioEnabled"),                   gws->testAttribute(QWebSettings::WebAudioEnabled)                   );
    s.setValue(QStringLiteral("webview/preferences/WebGLEnabled"),                      gws->testAttribute(QWebSettings::WebGLEnabled)                      );
    s.setValue(QStringLiteral("webview/preferences/Accelerated2dCanvasEnabled"),        gws->testAttribute(QWebSettings::Accelerated2dCanvasEnabled)        );
    s.setValue(QStringLiteral("webview/preferences/ErrorPageEnabled"),                 gwes->testAttribute(QWebEngineSettings::ErrorPageEnabled)            );
#  if QT_VERSION >= 0x050600
    s.setValue(QStringLiteral("webview/preferences/FullScreenSupportEnabled"),         gwes->testAttribute(QWebEngineSettings::FullScreenSupportEnabled)    );
#  endif
#else //ifdef QTWEBKIT
  //s.setValue(QStringLiteral("webview/preferences/AcceleratedCompositingEnabled"),     gwes->testAttribute(QWebEngineSettings::AcceleratedCompositingEnabled)     );
    s.setValue(QStringLiteral("webview/preferences/AutoLoadImages"),                    gwes->testAttribute(QWebEngineSettings::AutoLoadImages)                    );
  //s.setValue(QStringLiteral("webview/preferences/DeveloperExtrasEnabled"),            gwes->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled)            );
  //s.setValue(QStringLiteral("webview/preferences/DnsPrefetchEnabled"),                gwes->testAttribute(QWebEngineSettings::DnsPrefetchEnabled)                );
  //s.setValue(QStringLiteral("webview/preferences/FrameFlatteningEnabled"),            gwes->testAttribute(QWebEngineSettings::FrameFlatteningEnabled)            );
  //s.setValue(QStringLiteral("webview/preferences/JavaEnabled"),                       gwes->testAttribute(QWebEngineSettings::JavaEnabled)                       );
    s.setValue(QStringLiteral("webview/preferences/JavascriptCanAccessClipboard"),      gwes->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard)      );
  //s.setValue(QStringLiteral("webview/preferences/JavascriptCanCloseWindows"),         gwes->testAttribute(QWebEngineSettings::JavascriptCanCloseWindows)         );
    s.setValue(QStringLiteral("webview/preferences/JavascriptCanOpenWindows"),          gwes->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows)          );
    s.setValue(QStringLiteral("webview/preferences/JavascriptEnabled"),                 gwes->testAttribute(QWebEngineSettings::JavascriptEnabled)                 );
    s.setValue(QStringLiteral("webview/preferences/LinksIncludedInFocusChain"),         gwes->testAttribute(QWebEngineSettings::LinksIncludedInFocusChain)         );
    s.setValue(QStringLiteral("webview/preferences/LocalContentCanAccessFileUrls"),     gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls)     );
    s.setValue(QStringLiteral("webview/preferences/LocalContentCanAccessRemoteUrls"),   gwes->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls)   );
  //s.setValue(QStringLiteral("webview/preferences/LocalStorageDatabaseEnabled"),       gwes->testAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled)       );
    s.setValue(QStringLiteral("webview/preferences/LocalStorageEnabled"),               gwes->testAttribute(QWebEngineSettings::LocalStorageEnabled)               );
  //s.setValue(QStringLiteral("webview/preferences/OfflineStorageDatabaseEnabled"),     gwes->testAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled)     );
  //s.setValue(QStringLiteral("webview/preferences/OfflineWebApplicationCacheEnabled"), gwes->testAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled) );
#  if QT_VERSION >= 0x050600
    s.setValue(QStringLiteral("webview/preferences/PluginsEnabled"),                    gwes->testAttribute(QWebEngineSettings::PluginsEnabled)                    );
#  endif
  //s.setValue(QStringLiteral("webview/preferences/PrintElementBackgrounds"),           gwes->testAttribute(QWebEngineSettings::PrintElementBackgrounds)           );
  //s.setValue(QStringLiteral("webview/preferences/PrivateBrowsingEnabled"),            gwes->testAttribute(QWebEngineSettings::PrivateBrowsingEnabled)            );
  //s.setValue(QStringLiteral("webview/preferences/SiteSpecificQuirksEnabled"),         gwes->testAttribute(QWebEngineSettings::SiteSpecificQuirksEnabled)         );
    s.setValue(QStringLiteral("webview/preferences/SpatialNavigationEnabled"),          gwes->testAttribute(QWebEngineSettings::SpatialNavigationEnabled)          );
  //s.setValue(QStringLiteral("webview/preferences/TiledBackingStoreEnabled"),          gwes->testAttribute(QWebEngineSettings::TiledBackingStoreEnabled)          );
    s.setValue(QStringLiteral("webview/preferences/XSSAuditingEnabled"),                gwes->testAttribute(QWebEngineSettings::XSSAuditingEnabled)                );
  //s.setValue(QStringLiteral("webview/preferences/ZoomTextOnly"),                      gwes->testAttribute(QWebEngineSettings::ZoomTextOnly)                      );
  //s.setValue(QStringLiteral("webview/preferences/CSSGridLayoutEnabled"),              gwes->testAttribute(QWebEngineSettings::CSSGridLayoutEnabled)              );
  //s.setValue(QStringLiteral("webview/preferences/CSSRegionsEnabled"),                 gwes->testAttribute(QWebEngineSettings::CSSRegionsEnabled)                 );
  //s.setValue(QStringLiteral("webview/preferences/CaretBrowsingEnabled"),              gwes->testAttribute(QWebEngineSettings::CaretBrowsingEnabled)              );
    s.setValue(QStringLiteral("webview/preferences/HyperlinkAuditingEnabled"),          gwes->testAttribute(QWebEngineSettings::HyperlinkAuditingEnabled)          );
  //s.setValue(QStringLiteral("webview/preferences/NotificationsEnabled"),              gwes->testAttribute(QWebEngineSettings::NotificationsEnabled)              );
    s.setValue(QStringLiteral("webview/preferences/ScrollAnimatorEnabled"),             gwes->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled)             );
#  if QT_VERSION >= 0x050700
    s.setValue(QStringLiteral("webview/preferences/ScreenCaptureEnabled"),              gwes->testAttribute(QWebEngineSettings::ScreenCaptureEnabled)              );
    s.setValue(QStringLiteral("webview/preferences/WebAudioEnabled"),                   gwes->testAttribute(QWebEngineSettings::WebAudioEnabled)                   );
    s.setValue(QStringLiteral("webview/preferences/WebGLEnabled"),                      gwes->testAttribute(QWebEngineSettings::WebGLEnabled)                      );
    s.setValue(QStringLiteral("webview/preferences/Accelerated2dCanvasEnabled"),        gwes->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled)        );
    s.setValue(QStringLiteral("webview/preferences/AutoLoadIconsForPage"),              gwes->testAttribute(QWebEngineSettings::AutoLoadIconsForPage)              );
    s.setValue(QStringLiteral("webview/preferences/TouchIconsEnabled"),                 gwes->testAttribute(QWebEngineSettings::TouchIconsEnabled)                 );
#  endif
    s.setValue(QStringLiteral("webview/preferences/ErrorPageEnabled"),                  gwes->testAttribute(QWebEngineSettings::ErrorPageEnabled)                  );
#  if QT_VERSION >= 0x050600
    s.setValue(QStringLiteral("webview/preferences/FullScreenSupportEnabled"),          gwes->testAttribute(QWebEngineSettings::FullScreenSupportEnabled)          );
#  endif
#endif //ifdef QTWEBKIT

#ifdef QTWEBKIT
    s.setValue(QStringLiteral("webview/detail/DefaultTextEncoding"),             gws->defaultTextEncoding());
    s.setValue(QStringLiteral("webview/detail/MaximumPagesInCache"),             gws->maximumPagesInCache());
    //setting non-default value to crash...
  //s.setValue(QStringLiteral("webview/detail/IconDatabasePath"),                gws->iconDatabasePath());
    s.setValue(QStringLiteral("webview/detail/LocalStoragePath"),                gws->localStoragePath());
    s.setValue(QStringLiteral("webview/detail/OfflineStoragePath"),              gws->offlineStoragePath());
    s.setValue(QStringLiteral("webview/detail/OfflineStorageDefaultQuota"),      gws->offlineStorageDefaultQuota());
    //setting value to crash...
  //s.setValue(QStringLiteral("webview/detail/OfflineWebApplicationCachePath"),  gws->offlineWebApplicationCachePath());
    s.setValue(QStringLiteral("webview/detail/OfflineWebApplicationCacheQuota"), gws->offlineWebApplicationCacheQuota());
    s.setValue(QStringLiteral("webview/detail/UserStyleSheetUrl"),               gws->userStyleSheetUrl());
#else
    s.setValue(QStringLiteral("webview/detail/DefaultTextEncoding"),             gwes->defaultTextEncoding());
  //s.setValue(QStringLiteral("webview/detail/MaximumPagesInCache"),             gwes->maximumPagesInCache());
    //setting non-default value to crash...
  //s.setValue(QStringLiteral("webview/detail/IconDatabasePath"),                gwes->iconDatabasePath());
  //s.setValue(QStringLiteral("webview/detail/LocalStoragePath"),                gwes->localStoragePath());
  //s.setValue(QStringLiteral("webview/detail/OfflineStoragePath"),              gwes->offlineStoragePath());
  //s.setValue(QStringLiteral("webview/detail/OfflineStorageDefaultQuota"),      gwes->offlineStorageDefaultQuota());
    //setting value to crash...
  //s.setValue(QStringLiteral("webview/detail/OfflineWebApplicationCachePath"),  gwes->offlineWebApplicationCachePath());
  //s.setValue(QStringLiteral("webview/detail/OfflineWebApplicationCacheQuota"), gwes->offlineWebApplicationCacheQuota());
  //s.setValue(QStringLiteral("webview/detail/UserStyleSheetUrl"),               gwes->userStyleSheetUrl());
#endif

#ifdef QTWEBKIT
    QWebSettings::ThirdPartyCookiePolicy policy = gws->thirdPartyCookiePolicy();
    if(policy == QWebSettings::AlwaysAllowThirdPartyCookies)
        s.setValue(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AlwaysAllowThirdPartyCookies"));
    if(policy == QWebSettings::AlwaysBlockThirdPartyCookies)
        s.setValue(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AlwaysBlockThirdPartyCookies"));
    if(policy == QWebSettings::AllowThirdPartyWithExistingCookies)
        s.setValue(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AllowThirdPartyWithExistingCookies"));
#else
  //QWebEngineSettings::ThirdPartyCookiePolicy policy = gwes->thirdPartyCookiePolicy();
  //if(policy == QWebSettings::AlwaysAllowThirdPartyCookies)
  //    s.setValue(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AlwaysAllowThirdPartyCookies"));
  //if(policy == QWebSettings::AlwaysBlockThirdPartyCookies)
  //    s.setValue(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AlwaysBlockThirdPartyCookies"));
  //if(policy == QWebSettings::AllowThirdPartyWithExistingCookies)
  //    s.setValue(QStringLiteral("webview/detail/ThirdPartyCookiePolicy"), QStringLiteral("AllowThirdPartyWithExistingCookies"));
#endif

#ifdef QTWEBKIT
    s.setValue(QStringLiteral("webview/font/StandardFont"),           gws->fontFamily(QWebSettings::StandardFont)         );
    s.setValue(QStringLiteral("webview/font/FixedFont"),              gws->fontFamily(QWebSettings::FixedFont)            );
    s.setValue(QStringLiteral("webview/font/SerifFont"),              gws->fontFamily(QWebSettings::SerifFont)            );
    s.setValue(QStringLiteral("webview/font/SansSerifFont"),          gws->fontFamily(QWebSettings::SansSerifFont)        );
    s.setValue(QStringLiteral("webview/font/CursiveFont"),            gws->fontFamily(QWebSettings::CursiveFont)          );
    s.setValue(QStringLiteral("webview/font/FantasyFont"),            gws->fontFamily(QWebSettings::FantasyFont)          );
    s.setValue(QStringLiteral("webview/font/MinimumFontSize"),        gws->fontSize(QWebSettings::MinimumFontSize)        );
    s.setValue(QStringLiteral("webview/font/MinimumLogicalFontSize"), gws->fontSize(QWebSettings::MinimumLogicalFontSize) );
    s.setValue(QStringLiteral("webview/font/DefaultFontSize"),        gws->fontSize(QWebSettings::DefaultFontSize)        );
    s.setValue(QStringLiteral("webview/font/DefaultFixedFontSize"),   gws->fontSize(QWebSettings::DefaultFixedFontSize)   );
#else
    s.setValue(QStringLiteral("webview/font/StandardFont"),           gwes->fontFamily(QWebEngineSettings::StandardFont)         );
    s.setValue(QStringLiteral("webview/font/FixedFont"),              gwes->fontFamily(QWebEngineSettings::FixedFont)            );
    s.setValue(QStringLiteral("webview/font/SerifFont"),              gwes->fontFamily(QWebEngineSettings::SerifFont)            );
    s.setValue(QStringLiteral("webview/font/SansSerifFont"),          gwes->fontFamily(QWebEngineSettings::SansSerifFont)        );
    s.setValue(QStringLiteral("webview/font/CursiveFont"),            gwes->fontFamily(QWebEngineSettings::CursiveFont)          );
    s.setValue(QStringLiteral("webview/font/FantasyFont"),            gwes->fontFamily(QWebEngineSettings::FantasyFont)          );
#if QT_VERSION >= 0x050700
    s.setValue(QStringLiteral("webview/font/PictographFont"),         gwes->fontFamily(QWebEngineSettings::PictographFont)       );
#endif
    s.setValue(QStringLiteral("webview/font/MinimumFontSize"),        gwes->fontSize(QWebEngineSettings::MinimumFontSize)        );
    s.setValue(QStringLiteral("webview/font/MinimumLogicalFontSize"), gwes->fontSize(QWebEngineSettings::MinimumLogicalFontSize) );
    s.setValue(QStringLiteral("webview/font/DefaultFontSize"),        gwes->fontSize(QWebEngineSettings::DefaultFontSize)        );
    s.setValue(QStringLiteral("webview/font/DefaultFixedFontSize"),   gwes->fontSize(QWebEngineSettings::DefaultFixedFontSize)   );
#endif

    s.setValue(QStringLiteral("webview/menu/LinkMenu"),      m_LinkMenu);
    s.setValue(QStringLiteral("webview/menu/ImageMenu"),     m_ImageMenu);
    s.setValue(QStringLiteral("webview/menu/SelectionMenu"), m_SelectionMenu);
    s.setValue(QStringLiteral("webview/menu/RegularMenu"),   m_RegularMenu);

    foreach(QKeySequence seq, m_KeyMap.keys()){
        if(!seq.isEmpty() && !seq.toString().isEmpty())
            s.setValue(QStringLiteral("webview/keymap/") + seq.toString()
                        // cannot use slashes on QSettings.
                          .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                          .replace(QStringLiteral("/"), QStringLiteral("Slash")),
                        m_KeyMap[seq]);
    }

    foreach(QString button, m_MouseMap.keys()){
        if(!button.isEmpty())
            s.setValue(QStringLiteral("webview/mouse/") + button, m_MouseMap[button]);
    }

    foreach(QString gesture, m_RightGestureMap.keys()){
        if(!gesture.isEmpty())
            s.setValue(QStringLiteral("webview/rightgesture/") + gesture, m_RightGestureMap[gesture]);
    }

    foreach(QString gesture, m_LeftGestureMap.keys()){
        if(!gesture.isEmpty())
            s.setValue(QStringLiteral("webview/leftgesture/") + gesture, m_LeftGestureMap[gesture]);
    }

    QMap<QString, SearchEngine> search = Page::GetSearchEngineMap();
    foreach(QString key, search.keys()){
        if(search[key].length() == 1)
            s.setValue(QStringLiteral("webview/searchengine/") + key, search[key].first());
        else
            s.setValue(QStringLiteral("webview/searchengine/") + key, search[key]);
    }

    QMap<QString, Bookmarklet> bookmark = Page::GetBookmarkletMap();
    foreach(QString key, bookmark.keys()){
        if(bookmark[key].length() == 1)
            s.setValue(QStringLiteral("webview/bookmarklet/") + key, bookmark[key].first());
        else
            s.setValue(QStringLiteral("webview/bookmarklet/") + key, bookmark[key]);
    }
}

void View::ApplySpecificSettings(QStringList set){
    if(!page()) return;

#ifdef QTWEBKIT
    if(WebPage *p = qobject_cast<WebPage*>(page())){
        QWebSettings *s = p->settings();
        QWebSettings *g = QWebSettings::globalSettings();

        int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*")));
        if(pos != -1)
            s->setDefaultTextEncoding(set[pos].split(QStringLiteral(" ")).last());

        foreach(QWebSettings::WebAttribute attr, m_WebSwitches.keys()){
            QString str = m_WebSwitches[attr];
            s->setAttribute(attr,
                            set.indexOf(QRegularExpression(QStringLiteral("\\A!%1\\Z").arg(str))) != -1 ? false :
                            set.indexOf(QRegularExpression(QStringLiteral("\\A%1\\Z").arg(str)))  != -1 ? true  :
                            g->testAttribute(attr));
        }
    } else
#endif
    if(WebEnginePage *p = qobject_cast<WebEnginePage*>(page())){
        QWebEngineSettings *s = p->settings();
        QWebEngineSettings *g = QWebEngineSettings::defaultSettings();

        // why not copied from defaultSettings?

        s->setDefaultTextEncoding(g->defaultTextEncoding());

        s->setAttribute(QWebEngineSettings::AutoLoadImages, g->testAttribute(QWebEngineSettings::AutoLoadImages));
        s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, g->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard));
        s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, g->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows));
        s->setAttribute(QWebEngineSettings::JavascriptEnabled, g->testAttribute(QWebEngineSettings::JavascriptEnabled));
        s->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain, g->testAttribute(QWebEngineSettings::LinksIncludedInFocusChain));
        s->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, g->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls));
        s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, g->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls));
        s->setAttribute(QWebEngineSettings::LocalStorageEnabled, g->testAttribute(QWebEngineSettings::LocalStorageEnabled));
#if QT_VERSION >= 0x050600
        s->setAttribute(QWebEngineSettings::PluginsEnabled, g->testAttribute(QWebEngineSettings::PluginsEnabled));
#endif
        s->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, g->testAttribute(QWebEngineSettings::SpatialNavigationEnabled));
        s->setAttribute(QWebEngineSettings::XSSAuditingEnabled, g->testAttribute(QWebEngineSettings::XSSAuditingEnabled));
        s->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, g->testAttribute(QWebEngineSettings::HyperlinkAuditingEnabled));
        s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, g->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled));
#if QT_VERSION >= 0x050700
        s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, g->testAttribute(QWebEngineSettings::ScreenCaptureEnabled));
        s->setAttribute(QWebEngineSettings::WebAudioEnabled, g->testAttribute(QWebEngineSettings::WebAudioEnabled));
        s->setAttribute(QWebEngineSettings::WebGLEnabled, g->testAttribute(QWebEngineSettings::WebGLEnabled));
        s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, g->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled));
        s->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, g->testAttribute(QWebEngineSettings::AutoLoadIconsForPage));
        s->setAttribute(QWebEngineSettings::TouchIconsEnabled, g->testAttribute(QWebEngineSettings::TouchIconsEnabled));
#endif
        s->setAttribute(QWebEngineSettings::ErrorPageEnabled, g->testAttribute(QWebEngineSettings::ErrorPageEnabled));
#if QT_VERSION >= 0x050600
        s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, g->testAttribute(QWebEngineSettings::FullScreenSupportEnabled));
#endif

        s->setFontFamily(QWebEngineSettings::StandardFont, g->fontFamily(QWebEngineSettings::StandardFont));
        s->setFontFamily(QWebEngineSettings::FixedFont, g->fontFamily(QWebEngineSettings::FixedFont));
        s->setFontFamily(QWebEngineSettings::SerifFont, g->fontFamily(QWebEngineSettings::SerifFont));
        s->setFontFamily(QWebEngineSettings::SansSerifFont, g->fontFamily(QWebEngineSettings::SansSerifFont));
        s->setFontFamily(QWebEngineSettings::CursiveFont, g->fontFamily(QWebEngineSettings::CursiveFont));
        s->setFontFamily(QWebEngineSettings::FantasyFont, g->fontFamily(QWebEngineSettings::FantasyFont));
#if QT_VERSION >= 0x050700
        s->setFontFamily(QWebEngineSettings::PictographFont, g->fontFamily(QWebEngineSettings::PictographFont));
#endif

        s->setFontSize(QWebEngineSettings::MinimumFontSize, g->fontSize(QWebEngineSettings::MinimumFontSize));
        s->setFontSize(QWebEngineSettings::MinimumLogicalFontSize, g->fontSize(QWebEngineSettings::MinimumLogicalFontSize));
        s->setFontSize(QWebEngineSettings::DefaultFontSize, g->fontSize(QWebEngineSettings::DefaultFontSize));
        s->setFontSize(QWebEngineSettings::DefaultFixedFontSize, g->fontSize(QWebEngineSettings::DefaultFixedFontSize));

        int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*")));
        if(pos != -1)
            s->setDefaultTextEncoding(set[pos].split(QStringLiteral(" ")).last());

        foreach(QWebEngineSettings::WebAttribute attr, m_WebEngineSwitches.keys()){
            QString str = m_WebEngineSwitches[attr];
            s->setAttribute(attr,
                            set.indexOf(QRegularExpression(QStringLiteral("\\A!%1\\Z").arg(str))) != -1 ? false :
                            set.indexOf(QRegularExpression(QStringLiteral("\\A%1\\Z").arg(str)))  != -1 ? true  :
                            g->testAttribute(attr));
        }
    }

    if     (set.indexOf(QRegularExpression(QStringLiteral( "\\A[lL](?:oad)?[hH](?:ack)?\\Z"))) != -1) m_EnableLoadHackLocal = true;
    else if(set.indexOf(QRegularExpression(QStringLiteral("\\A![lL](?:oad)?[hH](?:ack)?\\Z"))) != -1) m_EnableLoadHackLocal = false;
    if     (set.indexOf(QRegularExpression(QStringLiteral( "\\A[dD](?:rag)?[hH](?:ack)?\\Z"))) != -1) m_EnableDragHackLocal = true;
    else if(set.indexOf(QRegularExpression(QStringLiteral("\\A![dD](?:rag)?[hH](?:ack)?\\Z"))) != -1) m_EnableDragHackLocal = false;

    if     (set.indexOf(QRegularExpression(QStringLiteral( "\\A[hH](?:ist)?[nN](?:ode)?\\Z"))) != -1) m_EnableLoadHackLocal = true;
    else if(set.indexOf(QRegularExpression(QStringLiteral("\\A![hH](?:ist)?[nN](?:ode)?\\Z"))) != -1) m_EnableLoadHackLocal = false;
    if     (set.indexOf(QRegularExpression(QStringLiteral( "\\A[dD](?:rag)?[gG](?:esture)?\\Z"))) != -1) m_EnableDragHackLocal = true;
    else if(set.indexOf(QRegularExpression(QStringLiteral("\\A![dD](?:rag)?[gG](?:esture)?\\Z"))) != -1) m_EnableDragHackLocal = false;
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
            code.replace(QStringLiteral("%%%%%%%"), QStringLiteral("%25%25%25%25%25%25%25"));
            code.replace(QStringLiteral("%%%%%"), QStringLiteral("%25%25%25%25%25"));
            code.replace(QStringLiteral("%%%"), QStringLiteral("%25%25%25"));
            code.replace(QStringLiteral("%%"), QStringLiteral("%25%25"));
            code.replace(QRegularExpression(QStringLiteral("%([^0-9A-F][0-9A-F]|[0-9A-F][^0-9A-F]|[^0-9A-F][^0-9A-F])")),
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
    m_CurrentBaseUrl = QUrl();
    m_SelectedText = QString();
    m_SelectedHtml = QString();

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
    m_CurrentBaseUrl = QUrl();
    m_SelectedText = QString();
    m_SelectedHtml = QString();
}

void View::GestureStarted(QPoint pos){
    m_Gesture.clear();
    m_GestureStartedPos = pos;
    m_BeforeGesturePos  = pos;
    m_SameGestureVectorCount = 0;
    m_HadSelection = !SelectedText().isEmpty();
    SetScrollBarState();
    CallWithHitElement(pos, [this](SharedWebElement elem){ m_ClickedElement = elem;});
    if(m_HadSelection){
        CallWithSelectionRegion([this, pos](QRegion region){
                if(region.contains(pos))
                    m_SelectionRegion = region;
                else m_HadSelection = false;
            });
        CallWithSelectedText([this](QString text){ m_SelectedText = text;});
        CallWithSelectedHtml([this](QString html){ m_SelectedHtml = html;});
        CallWithGotCurrentBaseUrl([this](QUrl base){ m_CurrentBaseUrl = base;});
    }
}

void View::GestureMoved(QPoint pos){
    if(m_GestureStartedPos.isNull()) return;

    Action(Page::_NoAction); // for action guard.
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

void View::GestureAborted(){
    m_Gesture.clear();
    m_GestureStartedPos = QPoint();
    m_BeforeGesturePos  = QPoint();
    m_SameGestureVectorCount = 0;
    m_ClickedElement = 0;
    m_SelectionRegion = QRegion();
    m_CurrentBaseUrl = QUrl();
    m_SelectedText = QString();
    m_SelectedHtml = QString();
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
    m_CurrentBaseUrl = QUrl();
    m_SelectedText = QString();
    m_SelectedHtml = QString();
    m_HadSelection = false;
    m_DragStarted = false;
}

bool View::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return false;
    QString str = m_KeyMap[seq];
    if(str.isEmpty()) return false;

    return TriggerAction(str);
}

bool View::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = m_KeyMap[seq]; // sequence => action
    if(str.isEmpty()) return false;

    return TriggerAction(str);
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
