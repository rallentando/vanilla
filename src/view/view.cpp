#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "view.hpp"
#include "menuschema.hpp"
#include "inputmap.hpp"
#include "inputmapschema.hpp"
#include "devicescale.hpp"
#include "menuiconrow.hpp"
#include "extensioncontroller.hpp"

#include <functional>

#include <QNetworkRequest>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QKeyEvent>
#include <QKeySequence>
#include <QSettings>
#include <QPixmap>
#include <QMenu>
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDrag>
#include <QMimeData>
#include <QLineF>
#include <QStyle>
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#endif

#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "webenginepage.hpp"
#include "edgewebview.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "mainwindow.hpp"
#include "application.hpp"
#include "runninguploads.hpp"
#include "networkcontroller.hpp"
#include "dialog.hpp"
#include "directorypage.hpp"

bool View::m_EnableDestinationInferrer = false;
bool View::m_EnableDragGesture = false;
bool View::m_EnableMouseGesture = true;
bool View::m_EnableScrollGesture = false;
bool View::m_InspectorInMainWindow = true;
QString View::m_SuspendHiddenViews = QStringLiteral("Active");
bool View::m_SavePasswords = true;
QString View::m_TrackingPrevention = QStringLiteral("Balanced");
bool View::m_SmartScreen = true;

QMap<QKeySequence, QString> View::m_KeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> View::m_MouseMap = QMap<QString, QString>();
QMap<QString, QString> View::m_DragGestureMap = QMap<QString, QString>();
QMap<QString, QString> View::m_RightGestureMap = QMap<QString, QString>();
QMap<QString, QString> View::m_ScrollGestureMap = QMap<QString, QString>();

SharedWebElement View::m_ClickedElement = 0;
QRegion View::m_SelectionRegion = QRegion();
QUrl View::m_CurrentBaseUrl = QUrl();
QString View::m_SelectedText = QString();
QString View::m_SelectedHtml = QString();

bool View::m_ActivateNewViewDefault = false;
bool View::m_EnableSingleKeyShortcut = false;
bool View::m_DragToStartDownload = false;
bool View::m_DragStarted = false;
bool View::m_HadSelection = false;
bool View::m_Switching = false;
int View::m_DraggingOut = 0;
bool View::m_RightButtonConsumed = false;
QElapsedTimer View::m_OwnDropTimer = QElapsedTimer();
QPoint View::m_GestureStartedPos = QPoint();
SpentButtons View::m_SpentButtons;
QPoint View::m_BeforeGesturePos = QPoint();
View::Gesture View::m_Gesture = QList<View::GestureVector>();
View::GestureVector View::m_CurrentGestureVector = Gv_NoMove;
View::GestureVector View::m_BeforeGestureVector = Gv_NoMove;
int View::m_SameGestureVectorCount = 0;
View::ScrollBarState View::m_ScrollBarState = NoScrollBarEnabled;

int View::m_GestureMode = 0;

const QList<float> View::m_ZoomFactorLevels = QList<float>()

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

qreal View::DeviceZoomScale(){
    return DeviceScale::PrimaryFactor();
}

#ifdef WEBENGINEVIEW
static const QMap<QWebEngineSettings::WebAttribute, QString> m_WebEngineSwitches = {
    { QWebEngineSettings::AutoLoadImages,           QString::fromLatin1("[iI]mage") },
    { QWebEngineSettings::JavascriptEnabled,        QString::fromLatin1("[jJ](?:ava)?[sS](?:cript)?") },
    { QWebEngineSettings::PluginsEnabled,           QString::fromLatin1("[pP]lugins?") },
    { QWebEngineSettings::SpatialNavigationEnabled, QString::fromLatin1("[sS]patial(?:[nN]avigation)?") },
    { QWebEngineSettings::ScrollAnimatorEnabled,    QString::fromLatin1("[sS]croll[aA]nimator") },
    { QWebEngineSettings::WebGLEnabled,             QString::fromLatin1("[wW]eb[gG][lL]") },
};
#endif

QString View::m_LinkMenu = QString();
QString View::m_ImageMenu = QString();
QString View::m_MediaMenu = QString();
QString View::m_TextMenu = QString();
QString View::m_SelectionMenu = QString();
QString View::m_RegularMenu = QString();

View::View(TreeBank *parent, QString id, QStringList set){
    Q_UNUSED(id) Q_UNUSED(set)
    m_TreeBank = parent;
    m_This = WeakView();
    m_Master = WeakView();
    m_Slave = WeakView();
    m_ViewNode = 0;
    m_Page = 0;
    m_EnableDragGestureLocal = m_EnableDragGesture;
    m_EnableRightGestureLocal = m_EnableMouseGesture;
    m_IsLoading = false;
    m_DisplayObscured = false;
}

View::~View(){
    RunningUploads::Instance()->Ended(this);

    if(m_TreeBank && m_TreeBank->GetCurrentView().get() == this){
        m_TreeBank->SetCurrentView(SharedView());
        m_TreeBank->SetCurrentViewNode(0);
    }
    if(m_ViewNode && m_ViewNode->GetView() == this){
        m_ViewNode->SetView(0);
    }
}

QObject *View::base(){ return 0;}
QObject *View::page(){ return m_Page;}

QString View::GetViewTypeName(){
    QObject *object = base();
    return object ? QString::fromLatin1(object->metaObject()->className()) : QString();
}

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
    if(m_ViewNode && m_ViewNode->GetView() == this){
        m_ViewNode->SetView(0);
    }
    m_TreeBank = 0;
    m_ViewNode = 0;
    if(base()) base()->deleteLater();
}

void View::Orphan(){
    if(m_ViewNode && m_ViewNode->GetView() == this){
        m_ViewNode->SetView(0);
    }
    m_TreeBank = 0;
    m_ViewNode = 0;
    if(QWidget *w = qobject_cast<QWidget*>(base())){
        w->lower();
        w->hide();
    }
}

TreeBank *View::GetTreeBank() const {
    return m_TreeBank;
}

ViewNode *View::GetViewNode() const {
    return m_ViewNode;
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

void View::SetTreeBank(TreeBank* t){
    m_TreeBank = t;
}

void View::SetViewNode(ViewNode* vn){
    m_ViewNode = vn;
    OnSetViewNode(vn);
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
    list.removeAll(INT_MAX);
    return list;
}

void View::SetScrollBarStateFrom(const QVariant &answer){
    if(!answer.isValid()) return;
    const QVariantList list = answer.toList();
    if(list.size() < 2) return;
    const int hmax = qMax(0, list[0].toInt());
    const int vmax = qMax(0, list[1].toInt());
    if(hmax && vmax) m_ScrollBarState = BothScrollBarEnabled;
    else if(hmax)    m_ScrollBarState = HorizontalScrollBarEnabled;
    else if(vmax)    m_ScrollBarState = VerticalScrollBarEnabled;
    else             m_ScrollBarState = NoScrollBarEnabled;
}

SharedWebElementList View::ElementsFrom(View *provider, const QVariant &answer,
                                       const QRect &viewport, const RectPlacer &place){
    SharedWebElementList result;
    if(!answer.isValid()) return result;
    for(const QVariant &value : answer.toMap().values()){
        std::shared_ptr<JsWebElement> e = std::make_shared<JsWebElement>();
        *e = JsWebElement(provider, value);
        const QRect rect = place ? place(e->Rectangle()) : e->Rectangle();
        e->SetRectangle(viewport.intersects(rect) ? rect : QRect());
        result << e;
    }
    return result;
}

QRegion View::SelectionRegionFrom(const QVariant &answer, const QRect &viewport,
                                  const RectPlacer &place){
    QRegion region;
    if(!answer.isValid() || !answer.canConvert<QVariantMap>()) return region;
    const QVariantMap map = answer.toMap();
    for(const QVariant &value : map.values()){
        const QVariantMap m = value.toMap();
        const QRect rect(m[QStringLiteral("x")].toInt(),
                         m[QStringLiteral("y")].toInt(),
                         m[QStringLiteral("width")].toInt(),
                         m[QStringLiteral("height")].toInt());
        region |= (place ? place(rect) : rect).intersected(viewport);
    }
    return region;
}

void View::FollowInferedUrl(const QUrl &target){
    if(target.isEmpty()) return;

    QNetworkRequest req(target);
    req.setRawHeader("Referer", url().toEncoded());

    SetScroll(QPointF());
    Load(req);
}

QUrl View::InferedUrl(const QUrl &current, const SharedWebElementList &elements, bool forward){
    QUrl base = current;
    base.setFragment(QString());

    QUrlQuery basequery = QUrlQuery(base);
    QStringList basebody = base.toString(QUrl::RemoveQuery).split(QStringLiteral("/"));
    basebody.removeFirst();

    SharedWebElement currentHighValidityElement = 0;
    QList<int> currentHighValidityDistance = QList<int>();

    const QRegularExpression reg = forward
        ? QRegularExpression(QObject::tr("(?:>>.*|.*>>|>|.*forward.*|.*next.*|.*front.*|.*beyond.*|.*after.*|.*more.*)"))
        : QRegularExpression(QObject::tr("(?:<<.*|.*<<|<|.*back(?:ward)?.*|.*prev(?:ious)?.*|.*rear.*|.*behind.*|.*before.*)"));

    auto wrongWay = [forward](int r){
        return r == INT_MAX || (forward ? r < 0 : r > 0);
    };
    auto pageStep = [forward](int r){
        static const int steps[] = { 1, 2, 10, 20, 100, 200, 4, 5, 40, 50, 400, 500 };
        for(const int step : steps)
            if(r == (forward ? step : -step)) return true;
        return false;
    };
    auto further = [forward](int r, int c){
        return forward ? r > c : r < c;
    };

    foreach(SharedWebElement elem, elements){
        QString text = elem->InnerText();
        int length = text.length();
        QUrl url = elem->LinkUrl();

        if(url.toString().startsWith(QStringLiteral("javascript:")))
            continue;

        if(reg.match(text.toLower()).hasMatch())
            return url;

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
            if(i >= result.length()){
                if(i == result.length() - 1){
                    goto entry;
                } else {
                    continue;
                }
            } else {
                if(i < delim && wrongWay(result[i]))
                    goto escape;
                if(i >= delim && pageStep(result[i])){

                    if(0 < length && length < 20){
                        if(Application::ExactMatch(QStringLiteral("[0-9]+"), text.trimmed())){
                            url.setFragment(fragment);
                            return url;
                        } else {
                            goto entry;
                        }
                    } else {
                        continue;
                    }
                }
                if(wrongWay(result[i]))
                    goto escape;

                if(i >= currentHighValidityDistance.length()){
                    if(i == currentHighValidityDistance.length() - 1){
                        goto escape;
                    } else {
                        continue;
                    }
                } else {
                    if(further(result[i], currentHighValidityDistance[i])){
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

    if(currentHighValidityElement && !currentHighValidityElement->IsNull())
        return currentHighValidityElement->LinkUrl();
    return QUrl();
}

namespace {

    void GoToInferedUrl(View *view, bool forward){
        view->CallWithFoundElements(forward ? Page::RelIsNext : Page::RelIsPrev,
                                    [view, forward](SharedWebElementList rels){

        if(!rels.isEmpty() && !rels[0]->LinkUrl().isEmpty())
            return view->FollowInferedUrl(rels[0]->LinkUrl());

        view->CallWithFoundElements(Page::HaveReference, [view, forward](SharedWebElementList elements){
            view->FollowInferedUrl(View::InferedUrl(view->url(), elements, forward));
        });});
    }
}

void View::GoBackToInferedUrl(){
    GoToInferedUrl(this, false);
}

void View::GoForwardToInferedUrl(){
    GoToInferedUrl(this, true);
}

QMimeData *View::CreateMimeDataFromSelection(NetworkAccessManager *nam){
    if(!nam) return 0;

    QMimeData *mime = new QMimeData;

    QList<QUrl>        urls = Page::ExtractUrlsFromHtml(m_SelectedHtml, m_CurrentBaseUrl, Page::HaveReference);
    if(urls.isEmpty()) urls = Page::ExtractUrlsFromText(m_SelectedText, m_CurrentBaseUrl);
    if(urls.isEmpty()) urls = Page::ExtractUrlsFromHtml(m_SelectedHtml, m_CurrentBaseUrl, Page::HaveSource);

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

    const bool offTheRecord = Page::OffTheRecord(this);
    if(!imageUrl.isEmpty()){
        DownloadItem *item = m_DragToStartDownload
            ? NetworkController::Download
              (nam, imageUrl, url(), NetworkController::TemporaryDirectory, offTheRecord)
            : nullptr;

        if(item) urls << item->GetUrls();
        else urls << imageUrl;

        mime->setText(imageUrl.toString());
        mime->setHtml(m_ClickedElement->ImageHtml());
    }
    if(!linkUrl.isEmpty()){
        DownloadItem *item = m_DragToStartDownload
            ? NetworkController::Download
              (nam, linkUrl, url(), NetworkController::TemporaryDirectory, offTheRecord)
            : nullptr;

        if(item) urls << item->GetUrls();
        else urls << linkUrl;

        mime->setText(linkUrl.toString());
        mime->setHtml(m_ClickedElement->LinkHtml());
    }
    mime->setUrls(urls);

    return mime;
}

QPixmap View::CreatePixmapFromSelection(){
    QPixmap pixmap = QPixmap(size());
    pixmap.fill(Theme::Color(Theme::TransparentWhite));
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
        pixmap.fill(Theme::Color(Theme::TransparentWhite));
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
        pixmap.fill(Theme::Color(Theme::TransparentWhite));
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
        for(QString key : Page::GetBookmarkletMap().keys()){
            menu->addAction(key, page, SLOT(OpenBookmarklet()));
        }
    }
    return menu;
}

QMenu *View::SearchMenu(){
    QMenu *menu = new QMenu(QObject::tr("SearchWith"));
    if(Page *page = base()->findChild<Page*>()){
        for(QString key : Page::GetSearchEngineMap().keys()){
            QAction *action = menu->addAction(key, page, SLOT(SearchWith()));
            QUrl url = QUrl::fromEncoded(Page::GetSearchEngine(key).first().toLatin1());
            action->setIcon(Application::GetIcon(url));
        }
    }
    return menu;
}

void View::AddExternalCommandActions(QMenu *menu, const char *slot, QVariant data){
    Page *page = base()->findChild<Page*>();
    if(!page) return;
    const QList<QPair<QString, QString> > commands = Application::ExternalCommands();
    for(int i = 0; i < commands.length(); i++){
        QAction *action = menu->addAction(commands[i].first, page, slot);
        action->setData(data);
        action->setProperty(EXTERNAL_COMMAND_PROPERTY, commands[i].first);
    }
}

QMenu *View::OpenWithOtherBrowserMenu(){
    QMenu *menu = new QMenu(QObject::tr("OpenWithOtherBrowser"));
    menu->addAction(Action(Page::_OpenWithDefault));
    AddExternalCommandActions(menu, SLOT(OpenWithCommand()), QVariant());
    return menu;
}

QMenu *View::OpenLinkWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenLinkWithOtherBrowser"));
    menu->addAction(Action(Page::_OpenLinkWithDefault, data));
    AddExternalCommandActions(menu, SLOT(OpenLinkWithCommand()), data);
    return menu;
}

QMenu *View::OpenImageWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenImageWithOtherBrowser"));
    menu->addAction(Action(Page::_OpenImageWithDefault, data));
    AddExternalCommandActions(menu, SLOT(OpenImageWithCommand()), data);
    return menu;
}

QMenu *View::OpenMediaWithOtherBrowserMenu(QVariant data){
    QMenu *menu = new QMenu(QObject::tr("OpenMediaWithOtherBrowser"));
    menu->addAction(Action(Page::_OpenMediaWithDefault, data));
    AddExternalCommandActions(menu, SLOT(OpenMediaWithCommand()), data);
    return menu;
}

QWidgetAction *View::MenuCaption(QMenu *menu, const QString &text, Qt::Alignment align){
    QWidgetAction *action = new QWidgetAction(menu);
    QWidget *widget = new QWidget(menu);
    QHBoxLayout *layout = new QHBoxLayout();
    QLabel *label = new QLabel(text);
    label->setAlignment(align);
    label->setForegroundRole(QPalette::PlaceholderText);
    layout->setContentsMargins(DeviceScale::Primary(15), DeviceScale::Primary(3),
                               DeviceScale::Primary(15), DeviceScale::Primary(3));
    layout->addWidget(label);
    widget->setLayout(layout);
    action->setDefaultWidget(widget);
    return action;
}

static bool IsHeldByIconRow(QMenu *menu, const QString &name){
    if(name == QStringLiteral("OpenCommand")) return true;
    MenuIconRow *row = MenuIconRow::Of(menu);
    return row && row->Holds(name);
}

void View::AddMenuCommandCell(QMenu *menu, QAction *action){
    if(MenuIconRow *row = MenuIconRow::Of(menu)) row->AddCommandCell(action);
}

void View::AddContextMenu(QMenu *menu, SharedWebElement elem, Page::MediaType type){
    QVariant data = QVariant::fromValue(elem);

    QUrl linkUrl  = elem ? elem->LinkUrl()  : QUrl();
    QUrl imageUrl = elem ? elem->ImageUrl() : QUrl();
    bool editable = elem ? elem->IsEditableElement() : false;

    static const int max_filename_length_at_contextmenu = 25;
    const QString comma = QStringLiteral(",");
    const QString slash = QStringLiteral("/");
    const QString quest = QStringLiteral("?");

    {
        QStringList parts;
        if(!linkUrl.isEmpty()) parts << QStringLiteral("LinkMenu");
        if(!imageUrl.isEmpty())
            parts << (type == Page::MediaTypePlayable ? QStringLiteral("MediaMenu") : QStringLiteral("ImageMenu"));
        if(editable) parts << QStringLiteral("TextMenu");
        if(!m_SelectedText.isEmpty()) parts << QStringLiteral("SelectionMenu");
        if(parts.isEmpty()) parts << QStringLiteral("RegularMenu");
        MenuIconRow *row = new MenuIconRow(menu);
        for(const QString &lines : parts){
            const MenuSchema::Table *icons = MenuSchema::IconsOf(lines);
            if(!icons) continue;
            row->AddDivider();
            for(const QString &name : MenuSchema::Current(Application::GlobalSettings(), *icons)){
                if(!MenuSchema::IsKnown(*icons, name) || row->Holds(name)) continue;
                QString icon = MenuIconRow::IconOf(name);
                bool enabled = true;
                QAction *action = nullptr;
                if(name == QStringLiteral("Back")){
                    action = Action(Page::_Back);
                    enabled = CanGoBack();
                } else if(name == QStringLiteral("Forward")){
                    action = Action(Page::_Forward);
                    enabled = CanGoForward();
                } else if(name == QStringLiteral("Rewind")){
                    action = Action(Page::_Rewind);
                    enabled = CanGoBack() && !EnableDestinationInferrer();
                } else if(name == QStringLiteral("FastForward")){
                    action = Action(Page::_FastForward);
                    enabled = !EnableDestinationInferrer();
                } else if(name == QStringLiteral("Reload")){
                    action = Action(IsLoading() ? Page::_Stop : Page::_Reload);
                    if(IsLoading()) icon = QStringLiteral("Stop");
                } else {
                    action = Action(name, data);
                    if(name == QStringLiteral("ToggleMediaMute") && action && action->isChecked())
                        icon = QStringLiteral("Muted");
                }
                if(action) row->AddCell(name, action, icon, enabled);
            }
        }
        row->Install();
    }
    const auto held = [menu](const QString &name){
        return IsHeldByIconRow(menu, name);
    };
    const auto separate = [menu](){
        const QList<QAction*> actions = menu->actions();
        if(!actions.isEmpty() && qobject_cast<QWidgetAction*>(actions.last())) return;
        menu->addSeparator();
    };

    if(!linkUrl.isEmpty()){
        if(!menu->isEmpty()) menu->addSeparator();
        QStringList list = linkUrl.toString().split(slash);
        QString name = QString();
        while(name.isEmpty() && !list.isEmpty())
            name = list.takeLast();
        if(name.length() > max_filename_length_at_contextmenu)
            name = name.left(max_filename_length_at_contextmenu) + QStringLiteral("...");
        name = QStringLiteral("(") + name + QStringLiteral(")");

        for(QString str : View::GetLinkMenu().split(comma, Qt::SkipEmptyParts)){
            if(str == QStringLiteral("LinkUrl")){
                menu->addAction(MenuCaption(menu, name, Qt::AlignLeft));
            }
            else if(str == QStringLiteral("Separator")) separate();
            else if(held(str)) continue;
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
        list = name.split(quest);
        name = QString();
        while(name.isEmpty() && !list.isEmpty())
            name = list.takeFirst();
        if(name.length() > max_filename_length_at_contextmenu)
            name = name.left(max_filename_length_at_contextmenu) + QStringLiteral("...");
        name = QStringLiteral("(") + name + QStringLiteral(")");

        for(QString str : type == Page::MediaTypePlayable ? View::GetMediaMenu().split(comma, Qt::SkipEmptyParts) : View::GetImageMenu().split(comma, Qt::SkipEmptyParts)){
            if(str == QStringLiteral("ImageUrl") || str == QStringLiteral("MediaUrl")){
                menu->addAction(MenuCaption(menu, name, Qt::AlignLeft));
            }
            else if(str == QStringLiteral("Separator")) separate();
            else if(held(str)) continue;
            else if(str == QStringLiteral("OpenImageWithOtherBrowser"))
                menu->addMenu(OpenImageWithOtherBrowserMenu(data));
            else if(str == QStringLiteral("OpenMediaWithOtherBrowser"))
                menu->addMenu(OpenMediaWithOtherBrowserMenu(data));
            else menu->addAction(Action(str, data));
        }
    }

    if(editable){
        if(!menu->isEmpty()) menu->addSeparator();

        for(QString str : View::GetTextMenu().split(comma, Qt::SkipEmptyParts)){
            if(str == QStringLiteral("Separator")) menu->addSeparator();
            else if(held(str)) continue;
            else menu->addAction(Action(str, data));
        }
    }

    if(!m_SelectedText.isEmpty()){
        if(!menu->isEmpty() && !editable)
            menu->addSeparator();

        for(QString str : View::GetSelectionMenu().split(comma, Qt::SkipEmptyParts)){
            if     (str == QStringLiteral("Separator"))  menu->addSeparator();
            else if(held(str)) continue;
            else if(str == QStringLiteral("SearchMenu")) menu->addMenu(SearchMenu());
            else menu->addAction(Action(str, data));
        }
    }
}

void View::AddExtensionMenu(QMenu *menu, SharedWebElement elem, Page::MediaType type){
    ExtensionController *controller = Extensions();
    if(!controller || controller->MenusMirrored()) return;

    ExtensionUi::MenuContext context;
    const QUrl linkUrl = elem ? elem->LinkUrl() : QUrl();
    const QUrl mediaUrl = elem ? elem->ImageUrl() : QUrl();
    if(!linkUrl.isEmpty()){
        context.contexts.insert(QStringLiteral("link"));
        context.linkUrl = linkUrl;
    }
    if(type == Page::MediaTypePlayable){
        context.contexts.insert(QStringLiteral("video"));
        context.srcUrl = mediaUrl;
    } else if(type == Page::MediaTypeImage || !mediaUrl.isEmpty()){
        context.contexts.insert(QStringLiteral("image"));
        context.srcUrl = mediaUrl;
    }
    if(elem && elem->IsEditableElement()) context.contexts.insert(QStringLiteral("editable"));
    if(!m_SelectedText.isEmpty()){
        context.contexts.insert(QStringLiteral("selection"));
        context.selectionText = m_SelectedText;
    }
    if(context.contexts.isEmpty()) context.contexts.insert(QStringLiteral("page"));
    context.pageUrl = url();
    const qint64 tab = GetViewNode() ? static_cast<qint64>(GetViewNode()->GetSerial()) : 0;

    QPointer<ExtensionController> weak(controller);
    std::function<void(QMenu*, const QString&, const QList<ExtensionUi::MenuShown>&)> add;
    add = [&add, weak, context, tab](QMenu *into, const QString &id, const QList<ExtensionUi::MenuShown> &items){
        foreach(const ExtensionUi::MenuShown &one, items){
            if(one.item.type == QStringLiteral("separator")){ into->addSeparator(); continue; }
            if(!one.children.isEmpty()){
                QMenu *sub = into->addMenu(one.title);
                sub->setEnabled(one.item.enabled);
                add(sub, id, one.children);
                continue;
            }
            QAction *action = into->addAction(one.title);
            action->setEnabled(one.item.enabled);
            if(one.item.type == QStringLiteral("checkbox") || one.item.type == QStringLiteral("radio")){
                action->setCheckable(true);
                action->setChecked(one.item.checked);
            }
            const QString itemId = one.item.id;
            QObject::connect(action, &QAction::triggered, into, [weak, id, itemId, context, tab](){
                if(weak) weak->ClickMenu(id, itemId, context, tab);
            });
        }
    };

    bool any = false;
    foreach(const ExtensionRow &row, controller->Rows()){
        if(!row.loaded || !row.enabled || !controller->HasPermission(row.manifest.id, QStringLiteral("contextMenus"))) continue;
        const ExtensionUi::Menus *menus = controller->MenusOf(row.manifest.id);
        if(!menus) continue;
        const QList<ExtensionUi::MenuShown> shown = menus->Shown(context);
        if(shown.isEmpty()) continue;
        if(!any && !menu->isEmpty()) menu->addSeparator();
        any = true;
        if(shown.size() == 1 && shown.first().item.type != QStringLiteral("separator")){
            add(menu, row.manifest.id, shown);
        } else {
            QMenu *sub = menu->addMenu(row.manifest.icon, row.manifest.name);
            add(sub, row.manifest.id, shown);
        }
    }
}

void View::AddRegularMenu(QMenu *menu, SharedWebElement elem){
    QVariant data = QVariant::fromValue(elem);

    const QString comma = QStringLiteral(",");
    const QString slash = QStringLiteral("/");

    if(!menu->isEmpty()) menu->addSeparator();

    for(QString str : View::GetRegularMenu().split(comma, Qt::SkipEmptyParts)){
        if     (str == QStringLiteral("Separator"))       menu->addSeparator();
        else if(IsHeldByIconRow(menu, str)) continue;
        else if(str == QStringLiteral("BookmarkletMenu")) menu->addMenu(BookmarkletMenu());
        else if(str == QStringLiteral("OpenWithOtherBrowser"))
            menu->addMenu(OpenWithOtherBrowserMenu());
        else if(str == QStringLiteral("ViewOrApplySource")){
            if(GetViewNode()->GetUrl().toEncoded().startsWith("view-source:")){
                menu->addAction(Action(Page::_ApplySource));
            } else {
                menu->addAction(Action(Page::_ViewSource));
            }
        } else menu->addAction(Action(str, data));
    }
    QString name = QStringLiteral("(") + GetViewTypeName() + QStringLiteral(")");

    menu->addAction(MenuCaption(menu, name, Qt::AlignRight));
}

#ifdef WEBENGINEVIEW

static bool EngineDefault(QWebEngineSettings *g, QWebEngineSettings::WebAttribute a){
    g->resetAttribute(a);
    return g->testAttribute(a);
}

static int EngineDefault(QWebEngineSettings *g, QWebEngineSettings::FontSize size){
    g->resetFontSize(size);
    return g->fontSize(size);
}

static void ApplyFontFamily(QWebEngineSettings *g, QWebEngineSettings::FontFamily family,
                            const QString &name){
    if(name.isEmpty()) g->resetFontFamily(family);
    else               g->setFontFamily(family, name);
}
#endif

void View::LoadSettings(){
    Settings &s = Application::GlobalSettings();

#ifdef WEBENGINEVIEW
    QWebEngineSettings *gwes = QWebEngineProfile::defaultProfile()->settings();
#endif

    m_GestureMode               = s.value(QStringLiteral("webview/@GestureMode"),           4).value<int>();
    m_EnableDestinationInferrer = s.value(QStringLiteral("webview/@EnableDestinationInferrer"), false).value<bool>();
    m_EnableDragGesture         = s.value(QStringLiteral("webview/@EnableDragGesture"), false).value<bool>();
    m_EnableMouseGesture        = s.value(QStringLiteral("webview/@EnableMouseGesture"), true).value<bool>();
    m_EnableScrollGesture       = s.value(QStringLiteral("webview/@EnableScrollGesture"), false).value<bool>();
    m_ActivateNewViewDefault    = s.value(QStringLiteral("webview/@ActivateNewViewDefault"), true).value<bool>();
    m_EnableSingleKeyShortcut   = s.value(QStringLiteral("webview/@EnableSingleKeyShortcut"), false).value<bool>();
    m_DragToStartDownload       = s.value(QStringLiteral("webview/@DragToStartDownload"), false).value<bool>();
    m_InspectorInMainWindow     = s.value(QStringLiteral("webview/@InspectorInMainWindow"), true).value<bool>();
    m_SuspendHiddenViews        = s.value(QStringLiteral("webview/@SuspendHiddenViews"), QStringLiteral("Active")).value<QString>();
    m_SavePasswords             = s.value(QStringLiteral("webview/@SavePasswords"), true).value<bool>();
    m_TrackingPrevention        = s.value(QStringLiteral("webview/@TrackingPrevention"), QStringLiteral("Balanced")).value<QString>();
    m_SmartScreen               = s.value(QStringLiteral("webview/@SmartScreen"), true).value<bool>();

    UrlBlockRules::ReloadRules();

    QString operation = s.value(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNode")).value<QString>();
    if(operation == QStringLiteral("InNewViewNode"))  Page::SetOpenCommandOperation(Page::InNewViewNode);
    if(operation == QStringLiteral("InNewDirectory")) Page::SetOpenCommandOperation(Page::InNewDirectory);
    if(operation == QStringLiteral("OnRoot"))         Page::SetOpenCommandOperation(Page::OnRoot);

    if(operation == QStringLiteral("InNewViewNodeBackground"))  Page::SetOpenCommandOperation(Page::InNewViewNodeBackground);
    if(operation == QStringLiteral("InNewDirectoryBackground")) Page::SetOpenCommandOperation(Page::InNewDirectoryBackground);
    if(operation == QStringLiteral("OnRootBackground"))         Page::SetOpenCommandOperation(Page::OnRootBackground);

    if(operation == QStringLiteral("InNewViewNodeNewWindow"))  Page::SetOpenCommandOperation(Page::InNewViewNodeNewWindow);
    if(operation == QStringLiteral("InNewDirectoryNewWindow")) Page::SetOpenCommandOperation(Page::InNewDirectoryNewWindow);
    if(operation == QStringLiteral("OnRootNewWindow"))         Page::SetOpenCommandOperation(Page::OnRootNewWindow);

#ifdef WEBENGINEVIEW
    gwes->setAttribute(QWebEngineSettings::AutoLoadImages,                    s.value(QStringLiteral("webview/preferences/AutoLoadImages"),                    EngineDefault(gwes, QWebEngineSettings::AutoLoadImages)                   ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard,      s.value(QStringLiteral("webview/preferences/JavascriptCanAccessClipboard"),      EngineDefault(gwes, QWebEngineSettings::JavascriptCanAccessClipboard)     ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,          s.value(QStringLiteral("webview/preferences/JavascriptCanOpenWindows"),          true ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptEnabled,                 s.value(QStringLiteral("webview/preferences/JavascriptEnabled"),                 EngineDefault(gwes, QWebEngineSettings::JavascriptEnabled)                ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain,         s.value(QStringLiteral("webview/preferences/LinksIncludedInFocusChain"),         EngineDefault(gwes, QWebEngineSettings::LinksIncludedInFocusChain)        ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,     s.value(QStringLiteral("webview/preferences/LocalContentCanAccessFileUrls"),     EngineDefault(gwes, QWebEngineSettings::LocalContentCanAccessFileUrls)    ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,   s.value(QStringLiteral("webview/preferences/LocalContentCanAccessRemoteUrls"),   EngineDefault(gwes, QWebEngineSettings::LocalContentCanAccessRemoteUrls)  ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::LocalStorageEnabled,               s.value(QStringLiteral("webview/preferences/LocalStorageEnabled"),               EngineDefault(gwes, QWebEngineSettings::LocalStorageEnabled)              ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::PluginsEnabled,                    s.value(QStringLiteral("webview/preferences/PluginsEnabled"),                              true ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::SpatialNavigationEnabled,          s.value(QStringLiteral("webview/preferences/SpatialNavigationEnabled"),          EngineDefault(gwes, QWebEngineSettings::SpatialNavigationEnabled)         ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled,          s.value(QStringLiteral("webview/preferences/HyperlinkAuditingEnabled"),          EngineDefault(gwes, QWebEngineSettings::HyperlinkAuditingEnabled)         ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,             s.value(QStringLiteral("webview/preferences/ScrollAnimatorEnabled"),             true).value<bool>());
    gwes->setAttribute(QWebEngineSettings::ScreenCaptureEnabled,              s.value(QStringLiteral("webview/preferences/ScreenCaptureEnabled"),              EngineDefault(gwes, QWebEngineSettings::ScreenCaptureEnabled)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::WebGLEnabled,                      s.value(QStringLiteral("webview/preferences/WebGLEnabled"),                      EngineDefault(gwes, QWebEngineSettings::WebGLEnabled)                     ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,        s.value(QStringLiteral("webview/preferences/Accelerated2dCanvasEnabled"),        EngineDefault(gwes, QWebEngineSettings::Accelerated2dCanvasEnabled)       ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::AutoLoadIconsForPage,              s.value(QStringLiteral("webview/preferences/AutoLoadIconsForPage"),              EngineDefault(gwes, QWebEngineSettings::AutoLoadIconsForPage)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::TouchIconsEnabled,                 s.value(QStringLiteral("webview/preferences/TouchIconsEnabled"),                 EngineDefault(gwes, QWebEngineSettings::TouchIconsEnabled)                ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled,          s.value(QStringLiteral("webview/preferences/FocusOnNavigationEnabled"),          EngineDefault(gwes, QWebEngineSettings::FocusOnNavigationEnabled)         ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::PrintElementBackgrounds,           s.value(QStringLiteral("webview/preferences/PrintElementBackgrounds"),           EngineDefault(gwes, QWebEngineSettings::PrintElementBackgrounds)          ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::AllowRunningInsecureContent,       s.value(QStringLiteral("webview/preferences/AllowRunningInsecureContent"),       EngineDefault(gwes, QWebEngineSettings::AllowRunningInsecureContent)      ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, s.value(QStringLiteral("webview/preferences/AllowGeolocationOnInsecureOrigins"), EngineDefault(gwes, QWebEngineSettings::AllowGeolocationOnInsecureOrigins)).value<bool>());
    gwes->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, s.value(QStringLiteral("webview/preferences/AllowWindowActivationFromJavaScript"), EngineDefault(gwes, QWebEngineSettings::AllowWindowActivationFromJavaScript)).value<bool>());
    gwes->setAttribute(QWebEngineSettings::ShowScrollBars,                    s.value(QStringLiteral("webview/preferences/ShowScrollBars"),                    EngineDefault(gwes, QWebEngineSettings::ShowScrollBars)                   ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,       s.value(QStringLiteral("webview/preferences/PlaybackRequiresUserGesture"),       true).value<bool>());
    gwes->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly,        s.value(QStringLiteral("webview/preferences/WebRTCPublicInterfacesOnly"),        EngineDefault(gwes, QWebEngineSettings::WebRTCPublicInterfacesOnly)       ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::JavascriptCanPaste,                s.value(QStringLiteral("webview/preferences/JavascriptCanPaste"),                EngineDefault(gwes, QWebEngineSettings::JavascriptCanPaste)               ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,                s.value(QStringLiteral("webview/preferences/DnsPrefetchEnabled"),                EngineDefault(gwes, QWebEngineSettings::DnsPrefetchEnabled)               ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::PdfViewerEnabled,                  s.value(QStringLiteral("webview/preferences/PdfViewerEnabled"),                  EngineDefault(gwes, QWebEngineSettings::PdfViewerEnabled)                 ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::ErrorPageEnabled,                  s.value(QStringLiteral("webview/preferences/ErrorPageEnabled"),                  EngineDefault(gwes, QWebEngineSettings::ErrorPageEnabled)                 ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::FullScreenSupportEnabled,          s.value(QStringLiteral("webview/preferences/FullScreenSupportEnabled"),          true ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::NavigateOnDropEnabled,             s.value(QStringLiteral("webview/preferences/NavigateOnDropEnabled"),             EngineDefault(gwes, QWebEngineSettings::NavigateOnDropEnabled)            ).value<bool>());
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    gwes->setAttribute(QWebEngineSettings::ReadingFromCanvasEnabled,          s.value(QStringLiteral("webview/preferences/ReadingFromCanvasEnabled"),          EngineDefault(gwes, QWebEngineSettings::ReadingFromCanvasEnabled)         ).value<bool>());
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    gwes->setAttribute(QWebEngineSettings::ForceDarkMode,                     s.value(QStringLiteral("webview/preferences/ForceDarkMode"),                     EngineDefault(gwes, QWebEngineSettings::ForceDarkMode)                    ).value<bool>());
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    gwes->setAttribute(QWebEngineSettings::PrintHeaderAndFooter,              s.value(QStringLiteral("webview/preferences/PrintHeaderAndFooter"),              EngineDefault(gwes, QWebEngineSettings::PrintHeaderAndFooter)             ).value<bool>());
    gwes->setAttribute(QWebEngineSettings::PreferCSSMarginsForPrinting,       s.value(QStringLiteral("webview/preferences/PreferCSSMarginsForPrinting"),       EngineDefault(gwes, QWebEngineSettings::PreferCSSMarginsForPrinting)      ).value<bool>());
    if(s.contains(QStringLiteral("webview/preferences/TouchEventsApiEnabled")))
        gwes->setAttribute(QWebEngineSettings::TouchEventsApiEnabled,         s.value(QStringLiteral("webview/preferences/TouchEventsApiEnabled"),             EngineDefault(gwes, QWebEngineSettings::TouchEventsApiEnabled)            ).value<bool>());
    else
        gwes->resetAttribute(QWebEngineSettings::TouchEventsApiEnabled);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    gwes->setAttribute(QWebEngineSettings::BackForwardCacheEnabled,           s.value(QStringLiteral("webview/preferences/BackForwardCacheEnabled"),           EngineDefault(gwes, QWebEngineSettings::BackForwardCacheEnabled)          ).value<bool>());
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    gwes->setAttribute(QWebEngineSettings::TrimAccessibilityIdentifiers,      s.value(QStringLiteral("webview/preferences/TrimAccessibilityIdentifiers"),      EngineDefault(gwes, QWebEngineSettings::TrimAccessibilityIdentifiers)     ).value<bool>());
#endif
#endif

#ifdef WEBENGINEVIEW
    gwes->setDefaultTextEncoding
        (s.value(QStringLiteral("webview/detail/DefaultTextEncoding"),
                  gwes->defaultTextEncoding()).value<QString>());
#endif

#if defined(WEBENGINEVIEW)
    QString schemePolicy = s.value(QStringLiteral("webview/detail/UnknownUrlSchemePolicy"), QStringLiteral("AllowUnknownUrlSchemesFromUserInteraction")).value<QString>();
    if(schemePolicy == QStringLiteral("DisallowUnknownUrlSchemes"))
        gwes->setUnknownUrlSchemePolicy(QWebEngineSettings::DisallowUnknownUrlSchemes);
    if(schemePolicy == QStringLiteral("AllowUnknownUrlSchemesFromUserInteraction"))
        gwes->setUnknownUrlSchemePolicy(QWebEngineSettings::AllowUnknownUrlSchemesFromUserInteraction);
    if(schemePolicy == QStringLiteral("AllowAllUnknownUrlSchemes"))
        gwes->setUnknownUrlSchemePolicy(QWebEngineSettings::AllowAllUnknownUrlSchemes);
#endif

#ifdef WEBENGINEVIEW
    ApplyFontFamily(gwes, QWebEngineSettings::StandardFont,   s.value(QStringLiteral("webview/font/StandardFont")).value<QString>());
    ApplyFontFamily(gwes, QWebEngineSettings::SansSerifFont,  s.value(QStringLiteral("webview/font/SansSerifFont")).value<QString>());
    ApplyFontFamily(gwes, QWebEngineSettings::SerifFont,      s.value(QStringLiteral("webview/font/SerifFont")).value<QString>());
    ApplyFontFamily(gwes, QWebEngineSettings::FixedFont,      s.value(QStringLiteral("webview/font/FixedFont")).value<QString>());
    ApplyFontFamily(gwes, QWebEngineSettings::CursiveFont,    s.value(QStringLiteral("webview/font/CursiveFont")).value<QString>());
    ApplyFontFamily(gwes, QWebEngineSettings::FantasyFont,    s.value(QStringLiteral("webview/font/FantasyFont")).value<QString>());
    ApplyFontFamily(gwes, QWebEngineSettings::PictographFont, s.value(QStringLiteral("webview/font/PictographFont")).value<QString>());
    gwes->setFontSize(QWebEngineSettings::MinimumFontSize,        s.value(QStringLiteral("webview/font/MinimumFontSize"),        EngineDefault(gwes, QWebEngineSettings::MinimumFontSize)        ).value<int>());
    gwes->setFontSize(QWebEngineSettings::MinimumLogicalFontSize, s.value(QStringLiteral("webview/font/MinimumLogicalFontSize"), EngineDefault(gwes, QWebEngineSettings::MinimumLogicalFontSize) ).value<int>());
    gwes->setFontSize(QWebEngineSettings::DefaultFontSize,        s.value(QStringLiteral("webview/font/DefaultFontSize"),        EngineDefault(gwes, QWebEngineSettings::DefaultFontSize)        ).value<int>());
    gwes->setFontSize(QWebEngineSettings::DefaultFixedFontSize,   s.value(QStringLiteral("webview/font/DefaultFixedFontSize"),   EngineDefault(gwes, QWebEngineSettings::DefaultFixedFontSize)   ).value<int>());
#endif

    const auto read = [&s](const char *name){
        return MenuSchema::Current(s, *MenuSchema::Find(QString::fromLatin1(name))).join(QLatin1Char(','));
    };
    m_LinkMenu      = read("LinkMenu");
    m_ImageMenu     = read("ImageMenu");
    m_MediaMenu     = read("MediaMenu");
    m_TextMenu      = read("TextMenu");
    m_SelectionMenu = read("SelectionMenu");
    m_RegularMenu   = read("RegularMenu");

    {
        m_KeyMap = InputMapSchema::DefaultKeyMap(QStringLiteral("webview/keymap"));
        InputMap::LoadKeyMap(s, QStringLiteral("webview/keymap"), m_KeyMap, InputMap::Hooks(Page::IsValidAction));
    }
    {
        m_MouseMap = InputMapSchema::DefaultGestureMap(QStringLiteral("webview/mouse"));
        InputMap::LoadGestureMap(s, QStringLiteral("webview/mouse"), m_MouseMap, InputMap::Hooks(Page::IsValidAction));
    }
    {
        m_RightGestureMap = InputMapSchema::DefaultGestureMap(QStringLiteral("webview/rightgesture"));
        InputMap::LoadGestureMap(s, QStringLiteral("webview/rightgesture"), m_RightGestureMap, InputMap::Hooks(Page::IsValidAction));
    }
    {
        m_DragGestureMap = InputMapSchema::DefaultGestureMap(QStringLiteral("webview/leftgesture"));
        InputMap::LoadGestureMap(s, QStringLiteral("webview/leftgesture"), m_DragGestureMap, InputMap::Hooks(Page::IsValidAction));
    }
    {
        m_ScrollGestureMap = InputMapSchema::DefaultGestureMap(QStringLiteral("webview/scrollgesture"));
        InputMap::LoadGestureMap(s, QStringLiteral("webview/scrollgesture"), m_ScrollGestureMap, InputMap::Hooks(Page::IsValidAction));
    }
    {   QStringList keys = s.allKeys(QStringLiteral("webview/searchengine"));

        if(!Page::GetSearchEngineMap().isEmpty()) Page::ClearSearchEngine();

        foreach(QString key, keys){
            QString last = key.split(QStringLiteral("/")).last();
            QVariant searchengine = s.value(key);
            if(searchengine.canConvert<QString>())
                Page::RegisterSearchEngine(last, QStringList() << searchengine.value<QString>());
            if(searchengine.canConvert<QStringList>())
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
            if(bookmarklet.canConvert<QString>())
                Page::RegisterBookmarklet(last, QStringList() << bookmarklet.value<QString>());
            if(bookmarklet.canConvert<QStringList>())
                Page::RegisterBookmarklet(last, bookmarklet.value<QStringList>());
        }
    }
}

void View::SaveSettings(){
    Settings &s = Application::GlobalSettings();

#ifdef WEBENGINEVIEW
    QWebEngineSettings *gwes = QWebEngineProfile::defaultProfile()->settings();
#endif

    s.setValue(QStringLiteral("webview/@GestureMode"),          m_GestureMode);
    s.setValue(QStringLiteral("webview/@EnableDestinationInferrer"), m_EnableDestinationInferrer);
    s.setValue(QStringLiteral("webview/@EnableDragGesture"),    m_EnableDragGesture);
    s.setValue(QStringLiteral("webview/@EnableMouseGesture"),   m_EnableMouseGesture);
    s.setValue(QStringLiteral("webview/@EnableScrollGesture"),  m_EnableScrollGesture);
    s.setValue(QStringLiteral("webview/@ActivateNewViewDefault"), m_ActivateNewViewDefault);
    s.setValue(QStringLiteral("webview/@EnableSingleKeyShortcut"), m_EnableSingleKeyShortcut);
    s.setValue(QStringLiteral("webview/@DragToStartDownload"),  m_DragToStartDownload);
    s.setValue(QStringLiteral("webview/@InspectorInMainWindow"), m_InspectorInMainWindow);

    Page::OpenCommandOperation operation = Page::GetOpenOparation();
    if(operation == Page::InNewViewNode)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNode"));
    if(operation == Page::InNewDirectory) s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewDirectory"));
    if(operation == Page::OnRoot)         s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("OnRoot"));

    if(operation == Page::InNewViewNodeBackground)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNodeBackground"));
    if(operation == Page::InNewDirectoryBackground) s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewDirectoryBackground"));
    if(operation == Page::OnRootBackground)         s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("OnRootBackground"));

    if(operation == Page::InNewViewNodeNewWindow)  s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewViewNodeNewWindow"));
    if(operation == Page::InNewDirectoryNewWindow) s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("InNewDirectoryNewWindow"));
    if(operation == Page::OnRootNewWindow)         s.setValue(QStringLiteral("webview/@OpenCommandOperation"), QStringLiteral("OnRootNewWindow"));

#ifdef WEBENGINEVIEW
    s.setValue(QStringLiteral("webview/detail/DefaultTextEncoding"),             gwes->defaultTextEncoding());
#endif

#if defined(WEBENGINEVIEW)
    QWebEngineSettings::UnknownUrlSchemePolicy schemePolicy = gwes->unknownUrlSchemePolicy();
    if(schemePolicy == QWebEngineSettings::DisallowUnknownUrlSchemes)
        s.setValue(QStringLiteral("webview/detail/UnknownUrlSchemePolicy"), QStringLiteral("DisallowUnknownUrlSchemes"));
    if(schemePolicy == QWebEngineSettings::AllowUnknownUrlSchemesFromUserInteraction)
        s.setValue(QStringLiteral("webview/detail/UnknownUrlSchemePolicy"), QStringLiteral("AllowUnknownUrlSchemesFromUserInteraction"));
    if(schemePolicy == QWebEngineSettings::AllowAllUnknownUrlSchemes)
        s.setValue(QStringLiteral("webview/detail/UnknownUrlSchemePolicy"), QStringLiteral("AllowAllUnknownUrlSchemes"));
#endif

    InputMap::SaveKeyMap    (s, QStringLiteral("webview/keymap"),        m_KeyMap);
    InputMap::SaveGestureMap(s, QStringLiteral("webview/mouse"),         m_MouseMap);
    InputMap::SaveGestureMap(s, QStringLiteral("webview/rightgesture"),  m_RightGestureMap);
    InputMap::SaveGestureMap(s, QStringLiteral("webview/leftgesture"),   m_DragGestureMap);
    InputMap::SaveGestureMap(s, QStringLiteral("webview/scrollgesture"), m_ScrollGestureMap);

    QMap<QString, SearchEngine> search = Page::GetSearchEngineMap();
    for(QString key : search.keys()){
        if(search[key].length() == 1)
            s.setValue(QStringLiteral("webview/searchengine/") + key, search[key].first());
        else
            s.setValue(QStringLiteral("webview/searchengine/") + key, search[key]);
    }

    QMap<QString, Bookmarklet> bookmark = Page::GetBookmarkletMap();
    for(QString key : bookmark.keys()){
        if(bookmark[key].length() == 1)
            s.setValue(QStringLiteral("webview/bookmarklet/") + key, bookmark[key].first());
        else
            s.setValue(QStringLiteral("webview/bookmarklet/") + key, bookmark[key]);
    }
}

void View::CloseLater(WeakView weak, std::function<bool()> stillWanted){
    TreeBank::WhenItMayChange([weak, stillWanted](){
        SharedView view = weak.lock();
        if(!view) return;

        ViewNode *vn = view->GetViewNode();
        TreeBank *tb = view->GetTreeBank();
        if(!vn || !TreeBank::IsLive(tb) || vn->GetView() != view.get()) return;

        if(stillWanted && !stillWanted()) return;

        View::SetSwitchingState(true);
        tb->Close(vn);
        View::SetSwitchingState(false);
    });
}

void View::RebuildForOffTheRecord(){
    WeakView weak = GetThis();
    TreeBank::WhenItMayChange([weak](){
        SharedView view = weak.lock();
        if(!view) return;
        ViewNode *vn = view->GetViewNode();
        if(!vn || vn->GetView() != view.get()) return;
        TreeBank::RebuildViewForOffTheRecord(vn);
    });
}

bool View::IsOffTheRecord(){
    return DirectoryPage::SaysPrivate(m_SpecificSettings);
}

bool View::KeepsOffDisk(){
    return Page::OffTheRecord(this) ||
        (m_ViewNode && TreeBank::IsOffTheRecord(m_ViewNode));
}

void View::ApplySpecificSettings(QStringList set){
    m_SpecificSettings = set;

    if(!page()) return;

#ifdef WEBENGINEVIEW
    if(WebEnginePage *p = qobject_cast<WebEnginePage*>(page())){
        QWebEngineSettings *s = p->settings();
        QWebEngineSettings *g = QWebEngineProfile::defaultProfile()->settings();

        s->setDefaultTextEncoding(g->defaultTextEncoding());

        s->setAttribute(QWebEngineSettings::AutoLoadImages, g->testAttribute(QWebEngineSettings::AutoLoadImages));
        s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, g->testAttribute(QWebEngineSettings::JavascriptCanAccessClipboard));
        s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, g->testAttribute(QWebEngineSettings::JavascriptCanOpenWindows));
        s->setAttribute(QWebEngineSettings::JavascriptEnabled, g->testAttribute(QWebEngineSettings::JavascriptEnabled));
        s->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain, g->testAttribute(QWebEngineSettings::LinksIncludedInFocusChain));
        s->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, g->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls));
        s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, g->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls));
        s->setAttribute(QWebEngineSettings::LocalStorageEnabled, g->testAttribute(QWebEngineSettings::LocalStorageEnabled));
        s->setAttribute(QWebEngineSettings::PluginsEnabled, g->testAttribute(QWebEngineSettings::PluginsEnabled));
        s->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, g->testAttribute(QWebEngineSettings::SpatialNavigationEnabled));
        s->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, g->testAttribute(QWebEngineSettings::HyperlinkAuditingEnabled));
        s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, g->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled));
        s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, g->testAttribute(QWebEngineSettings::ScreenCaptureEnabled));
        s->setAttribute(QWebEngineSettings::WebGLEnabled, g->testAttribute(QWebEngineSettings::WebGLEnabled));
        s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, g->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled));
        s->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, g->testAttribute(QWebEngineSettings::AutoLoadIconsForPage));
        s->setAttribute(QWebEngineSettings::TouchIconsEnabled, g->testAttribute(QWebEngineSettings::TouchIconsEnabled));
        s->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, g->testAttribute(QWebEngineSettings::FocusOnNavigationEnabled));
        s->setAttribute(QWebEngineSettings::PrintElementBackgrounds, g->testAttribute(QWebEngineSettings::PrintElementBackgrounds));
        s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, g->testAttribute(QWebEngineSettings::AllowRunningInsecureContent));
        s->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, g->testAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins));
        s->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, g->testAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript));
        s->setAttribute(QWebEngineSettings::ShowScrollBars, g->testAttribute(QWebEngineSettings::ShowScrollBars));
        s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, g->testAttribute(QWebEngineSettings::PlaybackRequiresUserGesture));
        s->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, g->testAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly));
        s->setAttribute(QWebEngineSettings::JavascriptCanPaste, g->testAttribute(QWebEngineSettings::JavascriptCanPaste));
        s->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, g->testAttribute(QWebEngineSettings::DnsPrefetchEnabled));
        s->setAttribute(QWebEngineSettings::PdfViewerEnabled, g->testAttribute(QWebEngineSettings::PdfViewerEnabled));
        s->setAttribute(QWebEngineSettings::ErrorPageEnabled, g->testAttribute(QWebEngineSettings::ErrorPageEnabled));
        s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, g->testAttribute(QWebEngineSettings::FullScreenSupportEnabled));
        s->setAttribute(QWebEngineSettings::NavigateOnDropEnabled, g->testAttribute(QWebEngineSettings::NavigateOnDropEnabled));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        s->setAttribute(QWebEngineSettings::ReadingFromCanvasEnabled, g->testAttribute(QWebEngineSettings::ReadingFromCanvasEnabled));
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        s->setAttribute(QWebEngineSettings::ForceDarkMode, g->testAttribute(QWebEngineSettings::ForceDarkMode));
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        s->setAttribute(QWebEngineSettings::PrintHeaderAndFooter, g->testAttribute(QWebEngineSettings::PrintHeaderAndFooter));
        s->setAttribute(QWebEngineSettings::PreferCSSMarginsForPrinting, g->testAttribute(QWebEngineSettings::PreferCSSMarginsForPrinting));
        if(Application::GlobalSettings().contains(QStringLiteral("webview/preferences/TouchEventsApiEnabled")))
            s->setAttribute(QWebEngineSettings::TouchEventsApiEnabled, g->testAttribute(QWebEngineSettings::TouchEventsApiEnabled));
        else
            s->resetAttribute(QWebEngineSettings::TouchEventsApiEnabled);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        s->setAttribute(QWebEngineSettings::BackForwardCacheEnabled, g->testAttribute(QWebEngineSettings::BackForwardCacheEnabled));
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
        s->setAttribute(QWebEngineSettings::TrimAccessibilityIdentifiers, g->testAttribute(QWebEngineSettings::TrimAccessibilityIdentifiers));
#endif

        s->setFontFamily(QWebEngineSettings::StandardFont, g->fontFamily(QWebEngineSettings::StandardFont));
        s->setFontFamily(QWebEngineSettings::FixedFont, g->fontFamily(QWebEngineSettings::FixedFont));
        s->setFontFamily(QWebEngineSettings::SerifFont, g->fontFamily(QWebEngineSettings::SerifFont));
        s->setFontFamily(QWebEngineSettings::SansSerifFont, g->fontFamily(QWebEngineSettings::SansSerifFont));
        s->setFontFamily(QWebEngineSettings::CursiveFont, g->fontFamily(QWebEngineSettings::CursiveFont));
        s->setFontFamily(QWebEngineSettings::FantasyFont, g->fontFamily(QWebEngineSettings::FantasyFont));
        s->setFontFamily(QWebEngineSettings::PictographFont, g->fontFamily(QWebEngineSettings::PictographFont));

        s->setFontSize(QWebEngineSettings::MinimumFontSize, g->fontSize(QWebEngineSettings::MinimumFontSize));
        s->setFontSize(QWebEngineSettings::MinimumLogicalFontSize, g->fontSize(QWebEngineSettings::MinimumLogicalFontSize));
        s->setFontSize(QWebEngineSettings::DefaultFontSize, g->fontSize(QWebEngineSettings::DefaultFontSize));
        s->setFontSize(QWebEngineSettings::DefaultFixedFontSize, g->fontSize(QWebEngineSettings::DefaultFixedFontSize));

        int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*")));
        if(pos != -1)
            s->setDefaultTextEncoding(set[pos].split(QStringLiteral(" ")).last());

        for(QWebEngineSettings::WebAttribute attr : m_WebEngineSwitches.keys()){
            const int state = DirectoryPage::StateIn(set, m_WebEngineSwitches[attr]);
            s->setAttribute(attr, state == -1 ? g->testAttribute(attr) : state == 1);
        }
    }
#endif

    m_EnableDragGestureLocal = m_EnableDragGesture;
    m_EnableRightGestureLocal = m_EnableMouseGesture;

    const int right = DirectoryPage::StateIn
        (set, QStringLiteral("(?:[rR](?:ight)?[gG](?:esture)?|[mM](?:ouse)?[gG](?:esture)?)"));
    if(right != -1) m_EnableRightGestureLocal = right == 1;

    const int drag = DirectoryPage::StateIn
        (set, QStringLiteral("[dD](?:rag)?[hH](?:ack)?|[dD](?:rag)?[gG](?:esture)?"));
    if(drag != -1) m_EnableDragGestureLocal = drag == 1;
}

namespace {

    void Wire(bool on, QObject *sender, const char *signal, QObject *receiver, const char *slot){
        if(on) QObject::connect(sender, signal, receiver, slot, Qt::UniqueConnection);
        else   QObject::disconnect(sender, signal, receiver, slot);
    }

    void WirePage(QObject *page, TreeBank *tb, bool on){
        if(Receiver *receiver = tb->GetReceiver()){
            Wire(on, receiver, SIGNAL(OpenUrl(QUrl)),
                 page, SLOT(OpenInNew(QUrl)));
            Wire(on, receiver, SIGNAL(OpenUrl(QList<QUrl>)),
                 page, SLOT(OpenInNew(QList<QUrl>)));
            Wire(on, receiver, SIGNAL(OpenQueryUrl(QString)),
                 page, SLOT(OpenInNew(QString)));
            Wire(on, receiver, SIGNAL(SearchWith(QString, QString)),
                 page, SLOT(OpenInNew(QString, QString)));
            Wire(on, receiver, SIGNAL(Download(QString, QString)),
                 page, SLOT(Download(QString, QString)));
        }
    }
}

void View::Connect(TreeBank *tb){
    if(!tb || !page()) return;
    WirePage(page(), tb, true);
}

void View::Disconnect(TreeBank *tb){
    if(!tb || !page()) return;
    WirePage(page(), tb, false);
}

void View::WireWebView(QObject *view, QObject *linkSource, TreeBank *tb, bool on){
    Wire(on, view, SIGNAL(titleChanged(const QString&)),
         tb->parent(), SLOT(SetWindowTitle(const QString&)));
    if(Notifier *notifier = tb->GetNotifier()){
        Wire(on, view, SIGNAL(statusBarMessage(const QString&)),
             notifier, SLOT(SetStatus(const QString&)));
        Wire(on, view, SIGNAL(statusBarMessage2(const QString&, const QString&)),
             notifier, SLOT(SetStatus(const QString&, const QString&)));
        Wire(on, linkSource, SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
             notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));
        Wire(on, view, SIGNAL(ScrollChanged(QPointF)),
             notifier, SLOT(SetScroll(QPointF)));
        Wire(on, notifier, SIGNAL(ScrollRequest(QPointF)),
             view, SLOT(SetScroll(QPointF)));
    }
    if(Receiver *receiver = tb->GetReceiver()){
        Wire(on, receiver, SIGNAL(OpenBookmarklet(const QString&)),
             view, SLOT(Load(const QString&)));
        Wire(on, receiver, SIGNAL(SeekText(const QString&, View::FindFlags)),
             view, SLOT(SeekText(const QString&, View::FindFlags)));
        Wire(on, view, SIGNAL(FindCount(int, int)),
             receiver, SLOT(SetFindCount(int, int)));
        Wire(on, receiver, SIGNAL(KeyEvent(QString)),
             view, SLOT(KeyEvent(QString)));

        Wire(on, receiver, SIGNAL(SuggestRequest(const QUrl&)),
             page(), SLOT(DownloadSuggest(const QUrl&)));
        Wire(on, page(), SIGNAL(SuggestResult(const QByteArray&)),
             receiver, SLOT(DisplaySuggest(const QByteArray&)));
    }
}

void View::UpdateThumbnail(){
    if(!IsRenderable()) return;

    if(QThread::currentThread() != qApp->thread()) return;

    if(m_ViewNode && page()){
        MainWindow *win = Application::GetCurrentWindow();
        QSize parentsize =
            m_TreeBank ? m_TreeBank->ViewSize() :
            win ? win->GetTreeBank()->ViewSize() :
            !size().isEmpty() ? size() :
            DEFAULT_WINDOW_SIZE;

        if(!visible()){
            int x = m_ViewNode->GetScrollX();
            int y = m_ViewNode->GetScrollY();
            SetViewportSize(parentsize);
            m_ViewNode->SetScrollX(x);
            m_ViewNode->SetScrollY(y);
            RestoreScroll();
        }

        QImage image(parentsize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        Render(&painter,
               QRegion(QRect(QPoint(0, 0),
                             parentsize)));
        painter.end();

        parentsize.scale(SAVING_THUMBNAIL_SIZE,
                         Qt::KeepAspectRatioByExpanding);

        int width_diff  = parentsize.width()  - SAVING_THUMBNAIL_SIZE.width();
        int height_diff = parentsize.height() - SAVING_THUMBNAIL_SIZE.height();

        if(width_diff == 0 && height_diff == 0){
            m_ViewNode->SetImage(
                image.
                scaled(SAVING_THUMBNAIL_SIZE,
                       Qt::KeepAspectRatioByExpanding,
                       Qt::SmoothTransformation), KeepsOffDisk());
        } else {
            m_ViewNode->SetImage(
                image.
                scaled(parentsize,
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation).
                copy(width_diff / 2, height_diff / 2,
                     SAVING_THUMBNAIL_SIZE.width(),
                     SAVING_THUMBNAIL_SIZE.height()), KeepsOffDisk());
        }
    }
}

bool View::TriggerAction(QString str, QVariant data){
    if(Page::IsValidAction(str))
        TriggerAction(Page::StringToAction(str), data);
    else if(Page::GetBookmarkletMap().contains(str))
        Load(Page::GetBookmarklet(str).first());
    else if(Page::GetSearchEngineMap().contains(str))
        CallWithSelectedText([this, str](QString text){
            if(text.isEmpty()) return;
            QMetaObject::invokeMethod(page(), "OpenInNew",
                                      Q_ARG(QString, str),
                                      Q_ARG(QString, text));
        });
    else return false;
    return true;
}

QAction *View::Action(QString str, QVariant data){
    if(Page::IsValidAction(str))
        return Action(Page::StringToAction(str), data);
    if(!base() || !page()) return 0;
    QAction *action = 0;
    if(Page::GetBookmarkletMap().contains(str)){
        action = new QAction(base());
        action->setText(str);
        base()->connect(action, &QAction::triggered, [this, str, action](){
            Load(Page::GetBookmarklet(str).first());
            action->deleteLater();
        });
    } else if(Page::GetSearchEngineMap().contains(str)){
        QUrl url = QUrl::fromEncoded(Page::GetSearchEngine(str).first().toLatin1());
        action = new QAction(base());
        action->setText(str);
        action->setIcon(Application::GetIcon(url.host()));
        base()->connect(action, &QAction::triggered, [this, str, action](){
            CallWithSelectedText([this, str, action](QString text){
                if(!text.isEmpty())
                    QMetaObject::invokeMethod(page(), "OpenInNew",
                                              Q_ARG(QString, str),
                                              Q_ARG(QString, text));
                action->deleteLater();
            });
        });
    }
    return action;
}

QString View::ExtensionStatus() const {
    return QObject::tr("Extensions are unavailable in this view or private profile.");
}

void View::Load(){
    bool ok;
    QString str =
        ModalDialog::GetText(QObject::tr("URL or JavaScript"),
                             QObject::tr("Enter a URL or JavaScript"),
                             url().toString(), &ok);
    if(!str.isEmpty()) Load(str);
}

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

void View::Load(const QUrl &url){
    Load(QNetworkRequest(url));
}

void View::RestoreHistoryOrLoad(const QNetworkRequest &request){
    QObject *target = base();
    if(!target) return;
    const WeakView weak = GetThis();
    ExtensionNavigation::Of(target)->Request(Extensions(), [weak, request] {
        if(const auto view = weak.lock())
            if(!view->RestoreHistory()) view->Load(request);
    });
}

void View::Load(const QNetworkRequest &req){
    TreeBank::AddToUpdateBox(GetThis().lock());
    bool forbidden = ForbidToOverlap();
    QString str = QString::fromUtf8(req.url().toEncoded());

    if(str.startsWith(QStringLiteral("javascript:"))){
        QString code = req.url().toString(QUrl::None).mid(11);

        int count = 2;

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
        if(str.startsWith(QStringLiteral("about:"))){
            if(str == QStringLiteral("about:blank"))
                setUrl(BLANK_URL);
            else
                TriggerNativeLoadAction(QNetworkRequest(BLANK_URL));
        } else if(str.startsWith(QStringLiteral("view-source:"))){
            if(TreeBank::DrawsViewSource(this)) TriggerNativeLoadAction(newreq.url());
            else       SetSource(QUrl::fromEncoded(str.toLatin1()));
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
    m_TreeBank->RestackChildWidgets();
    if(ViewNode *vn = GetViewNode()) vn->SetLastAccessDateToCurrent();
}

void View::OnFocusOut(){
    m_ClickedElement = 0;
    m_SelectionRegion = QRegion();
    m_CurrentBaseUrl = QUrl();
    m_SelectedText = QString();
    m_SelectedHtml = QString();
}

QString View::GestureStatusLine(const QMap<QString, QString> &map, const QString &noAction){
    const QString shape = GestureToString(m_Gesture);
    if(!map.contains(shape))
        return shape + QStringLiteral(" (") + noAction + QStringLiteral(")");
    const QString name = map.value(shape);
    QAction *action = Page::IsValidAction(name)
        ? Action(Page::StringToAction(name)) : nullptr;
    return shape + QStringLiteral(" (") + (action ? action->text() : name) + QStringLiteral(")");
}

void View::MouseMoveWithGestures(QMouseEvent *ev, const MoveHooks &hooks){
    if(!m_TreeBank) return;

    Application::SetCurrentWindow(m_TreeBank->GetMainWindow());

    if(m_DragStarted){
        hooks.forward(ev);
        ev->setAccepted(hooks.acceptForwardedWhileDragging);
        return;
    }
    if(m_EnableRightGestureLocal &&
       ev->buttons() & Qt::RightButton &&
       !m_GestureStartedPos.isNull()){

        GestureMoved(ev->pos());
        hooks.status();
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
            hooks.forward(ev);
            ev->setAccepted(false);
            return;
        }
        QDrag *drag = new QDrag(hooks.dragParent);

        Application::ClearTemporaryDirectory();

        NetworkAccessManager *nam = hooks.networkAccessManager();

        QMimeData *mime = m_HadSelection
            ? CreateMimeDataFromSelection(nam)
            : CreateMimeDataFromElement(nam);

        if(!mime){
            drag->deleteLater();

            GestureAborted();
            hooks.forward(ev);
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
        hooks.forward(ev);
        ev->setAccepted(false);
    }
}

void View::MousePressWithMap(QMouseEvent *ev, const ButtonHooks &hooks){
    QString mouse;

    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());

    if(m_MouseMap.contains(mouse)){

        QString str = m_MouseMap[mouse];
        if(!str.isEmpty()){
            const bool right = (ev->buttons() | ev->button()) & Qt::RightButton;
            if(right) View::ConsumeRightButton();
            if(!View::TriggerAction(str, ev->pos())){
                if(right) View::TakeRightButtonConsumed();
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
    hooks.pressToEngine(ev);
    ev->setAccepted(hooks.acceptForwarded);
}

void View::MouseDoubleClickWithMap(QMouseEvent *ev, const ButtonHooks &hooks){
    QString mouse;
    Application::AddModifiersToString(mouse, ev->modifiers());
    Application::AddMouseButtonsToString(mouse, ev->buttons() & ~ev->button());
    Application::AddMouseButtonToString(mouse, ev->button());
    if(m_MouseMap.contains(mouse) && !m_MouseMap[mouse].isEmpty()){
        MousePressWithMap(ev, hooks);
        return;
    }
    hooks.doubleClickToEngine(ev);
    ev->setAccepted(false);
}

void View::MouseReleaseWithGestures(QMouseEvent *ev, const ButtonHooks &hooks){
    hooks.clearStatus();

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
            hooks.releaseToEngine(ev);
            ev->setAccepted(hooks.acceptForwarded);
            return;
        }
        ev->setAccepted(true);
        return;
    }

    if(ev->button() == Qt::LeftButton && !m_Gesture.isEmpty())
        ;
    else
        GestureAborted();

    hooks.releaseToEngine(ev);
    ev->setAccepted(hooks.acceptForwarded);
}

void View::DropWithGestures(QDropEvent *ev, const DropHooks &hooks){
    hooks.clearStatus();
    bool isLocal = false;
    QPoint pos = ev->position().toPoint();
    QObject *source = ev->source();
    QString text = ev->mimeData()->text();
    QList<QUrl> urls = Page::MimeDataToUrls(ev->mimeData(), source);

    for(QUrl u : std::as_const(urls)){ if(u.isLocalFile()) isLocal = true;}

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

    if(isLocal || consumed ||
       (DragToStartDownload() && !urls.isEmpty() && own))
        ;
    else {
        if(own && !urls.isEmpty()) MarkOwnDrop();
        hooks.toEngine(ev);
    }

    ev->setAccepted(true);
}

void View::WheelWithMap(QWheelEvent *ev, const WheelHooks &hooks){
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
                View::TriggerAction(str, hooks.actionPos(ev));
            }
            ev->setAccepted(true);
            return;
        }
    }
    hooks.toEngine(ev);
}

void View::GestureStarted(QPoint pos){
    m_Gesture.clear();
    m_GestureStartedPos = pos;
    m_BeforeGesturePos  = pos;
    m_SameGestureVectorCount = 0;
    m_RightButtonConsumed = false;
    m_DragStarted = false;
    SetScrollBarState();
    CallWithHitElement(pos, [this](SharedWebElement elem){
        Q_UNUSED(this) m_ClickedElement = elem;
    });
    CallWithSelectedText([pos, this](QString text){
        m_HadSelection = !text.isEmpty();
        if(m_HadSelection){
            m_SelectedText = text;
            CallWithSelectedHtml([this](QString html){
                Q_UNUSED(this) m_SelectedHtml = html;
            });
            CallWithGotCurrentBaseUrl([this](QUrl base){
                Q_UNUSED(this) m_CurrentBaseUrl = base;
            });
            CallWithSelectionRegion([this, pos](QRegion region){
                Q_UNUSED(this)
                if(region.contains(pos))
                    m_SelectionRegion = region;
                else m_HadSelection = false;
            });
        }
    });
}

void View::GestureMoved(QPoint pos){
    if(m_GestureStartedPos.isNull()) return;

    Action(Page::_NoAction);
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

    if(button == Qt::RightButton && !m_Gesture.isEmpty())
        m_RightButtonConsumed = true;

    if(button == Qt::RightButton && m_RightGestureMap.contains(ges)){

        QString str = m_RightGestureMap[ges];
        if(!str.isEmpty()){
            if(m_ClickedElement){
                TriggerAction(str, QVariant::fromValue(m_ClickedElement));
            } else {
                TriggerAction(str, m_GestureStartedPos);
            }
        }
    }

    if(button == Qt::LeftButton && m_DragGestureMap.contains(ges)){

        QString str = m_DragGestureMap[ges];
        if(!str.isEmpty()){
            if(m_ClickedElement){
                TriggerAction(str, QVariant::fromValue(m_ClickedElement));
            } else {
                TriggerAction(str, m_GestureStartedPos);
            }
        }
    }

    if(m_GestureStartedPos == QPoint(-1, -1) &&
       button == Qt::NoButton && m_ScrollGestureMap.contains(ges)){

        QString str = m_ScrollGestureMap[ges];
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

QString View::KeyAction(const QKeySequence &seq){
    if(!m_EnableSingleKeyShortcut && InputMap::IsSingleKey(seq)) return QString();
    return m_KeyMap.value(seq);
}

InputMap::KeyLayers View::KeyLayers(){
    const bool single = EnableSingleKeyShortcut();
    InputMap::KeyLayer views;
    views.map = m_KeyMap;
    views.singleKeys = single;
    InputMap::KeyLayer application;
    application.map = TreeBank::GetKeyMap();
    application.singleKeys = single;
    return InputMap::KeyLayers() << views << application;
}

bool View::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return false;
    QString str = KeyAction(seq);
    if(str.isEmpty()){
        const Qt::KeyboardModifiers modifiers = ev->modifiers();
        if(modifiers & Qt::MetaModifier) return false;
        return RunExtensionCommand(seq, ExtensionUi::ChromeKeyOf(modifiers & Qt::ControlModifier, modifiers & Qt::AltModifier,
                                                                  modifiers & Qt::ShiftModifier,
#ifdef Q_OS_WIN
                                                                  static_cast<int>(ev->nativeVirtualKey()),
#else
                                                                  0,
#endif
                                                                  ev->key()), ev->isAutoRepeat());
    }

    if(!TriggerAction(str)){
        return false;
    }
    return true;
}

bool View::TriggerKeyEvent(QString str){
    Application::InputScope key(true);
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = m_KeyMap.value(seq);
    if(str.isEmpty()){
        if(seq.count() != 1) return false;
        const QKeyCombination combination = seq[0];
        const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
        if(modifiers & Qt::MetaModifier) return false;
        return RunExtensionCommand(seq, ExtensionUi::ChromeKeyOf(modifiers & Qt::ControlModifier, modifiers & Qt::AltModifier,
                                                                  modifiers & Qt::ShiftModifier, 0, combination.key()), false);
    }

    if(!TriggerAction(str)){
        return false;
    }
    return true;
}

bool View::RunExtensionCommand(const QKeySequence &seq, const QString &key, bool repeat){
    ExtensionController *controller = Extensions();
    const bool command = controller && controller->HasCommand(key);
    if(ExtensionUi::RouteKey(false, TreeBank::TakesKey(seq), command) != ExtensionUi::KeyRoute::Extension) return false;
    const qint64 serial = GetViewNode() ? static_cast<qint64>(GetViewNode()->GetSerial()) : 0;
    if(!repeat) controller->RunCommand(key, serial);
    return true;
}

void View::OnLoadStarted(){
    m_IsLoading = true;
    static quint64 serial = 0;
    m_LoadSerial = ++serial;
    TreeBank::SomethingChanged();
}

void View::UploadStarted(const QUrl &url){
    RunningUploads::Instance()->Started(this, url, [this](){ Stop();});
}

void View::OnLoadFinished(bool ok){
    m_IsLoading = false;
    RunningUploads::Instance()->Ended(this);
    if(ok) m_RenderProcessLedger.Clear();
    TreeBank::SomethingChanged();
}

void View::ChangeNodeTitle(const QString &title){
    if(m_ViewNode && !title.isEmpty()){
        m_ViewNode->SetTitle(title);
        TreeBank::SomethingChanged();
    }
}

void View::ChangeNodeUrl(const QUrl &url){
    if(m_ViewNode && !url.isEmpty()){
        m_ViewNode->SetUrl(url);
        TreeBank::SomethingChanged();
    }
}

void View::SaveViewState(){
    SaveScroll();
    SaveZoom();
    SaveMediaTime();
}

void View::RestoreViewState(){
    RestoreScroll();
    RestoreZoom();
}

float View::StepZoom(float zoom, bool in, float min, float max){
    static const float eps = 0.01f;
    const QList<float> &levels = GetZoomFactorLevels();
    const int len = levels.length();
    const float from = qBound(min, zoom, max);
    const int level = levels.indexOf(from);
    float to = from;
    if(level == -1){
        if(in){
            for(int i = 0; i < len; i++){
                to = levels[i];
                if((to - from) > eps) break;
            }
        } else {
            for(int i = len - 1; i >= 0; i--){
                to = levels[i];
                if((from - to) > eps) break;
            }
        }
    } else if(in && level < len - 1){
        to = levels[level + 1];
    } else if(!in && level > 0){
        to = levels[level - 1];
    }
    return qBound(min, to, max);
}

float View::PrepareForZoomIn(){
    ViewNode *vn = GetViewNode();
    const float zoom = StepZoom(vn->GetZoom(), true, MinimumZoom(), MaximumZoom());
    vn->SetZoom(zoom);
    TreeBank::SomethingChanged();
    return zoom;
}

float View::PrepareForZoomOut(){
    ViewNode *vn = GetViewNode();
    const float zoom = StepZoom(vn->GetZoom(), false, MinimumZoom(), MaximumZoom());
    vn->SetZoom(zoom);
    TreeBank::SomethingChanged();
    return zoom;
}
