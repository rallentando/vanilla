#include "switch.hpp"
#include "const.hpp"
#include "keymap.hpp"
#include "mousemap.hpp"

#include "treebank.hpp"

#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QDomElement>
#include <QUrl>
#include <QNetworkRequest>
#include <QThread>
#include <QGLWidget>
#include <QOpenGLWidget>
#include <QFile>
#include <QDir>
#include <QFuture>
#include <QImageReader>
#include <QFileInfo>
#include <QStyle>

#include <QGraphicsRotation>
#include <QGraphicsScale>
#include <QtConcurrent/QtConcurrent>

#include "application.hpp"
#include "networkcontroller.hpp"
#include "saver.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "view.hpp"
#ifdef QTWEBKIT
#  include "webview.hpp"
#  include "graphicswebview.hpp"
#  include "quickwebview.hpp"
#  include "webpage.hpp"
#endif
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "webenginepage.hpp"
#include "gadgets.hpp"
#include "mainwindow.hpp"
#include "localview.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

/*

  Directory specific settings.

  "[iI][dD]" :
  use directory name as networkaccessmanager/profile ID.


  "[nN](?:o)?[lL](?:oad)?" :
  disable auto loading.


  "^(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*"
  :
  set encoding.


                     "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?"
  "[gG](?:raphics)?" "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?"
  "[qQ](?:uick)?"    "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?"
                     "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?"
  "[qQ](?:uick)?"    "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?"
  "[lL](?:ocal)?"                                   "(?:[vV](?:iew)?)?"
  "[fF](?:tp)?"                                     "(?:[vV](?:iew)?)?"
  "[tT](?:rident)?"                                 "(?:[vV](?:iew)?)?"
  :
  set object type.


  "^[uU](?:ser)?[aA](?:gent)? [^ ]+"
  :
  set user agent.


  "^[pP][rR][oO][xX][yY] [^ ].*"
  :
  set proxy.


  "^[sS][sS][lL] [^ ]+"
  :
  set ssl protocol.


  "(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"
  :
  set off the record.


  "[lL](?:oad)?[hH](?:ack)?"
  "[hH](?:ist)?[nN](?:ode)?"
  :
  click link or go back or go forward => new hist node.


  "[dD](?:rag)?[hH](?:ack)?"
  "[dD](?:rag)?[gG](?:esture)?"
  :
  enable drag gesture.


  "[iI]mage"
  "[iI]nspector"
  "[dD][nN][sS][pP]refetch"
  "[fF]rame[fF]latten"
  "[jJ](?:ava)?[sS](?:cript)?"
  "[pP]lugin"
  "[pP]rivate"
  "[sS]patial(?:[nN]avigation)?"
  "[tT]iled[bB]acking[sS]tore"
  "[zZ]oom[tT]ext[oO]nly"
  "[cC]aret[bB]rowse"
  "[sS]croll[aA]nimator"
  "[wW]eb[aA]udio"
  "[wW]eb[gG][lL]"
  :
  set web attribute.

 */

QString TreeBank::m_RootName = QString();
ViewNode *TreeBank::m_ViewRoot  = 0;
ViewNode *TreeBank::m_TrashRoot = 0;
HistNode *TreeBank::m_HistRoot  = 0;

ViewNode *TreeBank::m_ViewIterForward  = 0;
ViewNode *TreeBank::m_ViewIterBackward = 0;
HistNode *TreeBank::m_HistIterForward  = 0;
HistNode *TreeBank::m_HistIterBackward = 0;

bool TreeBank::m_TraverseAllView = false;
bool TreeBank::m_PurgeNotifier = false;
bool TreeBank::m_PurgeReceiver = false;
bool TreeBank::m_PurgeView = false;
int  TreeBank::m_MaxViewCount = 0;
int  TreeBank::m_MaxTrashEntryCount = 0;
int  TreeBank::m_TraverseCondition = 0;

enum TreeBank::TraverseMode TreeBank::m_TraverseMode = Neutral;
enum TreeBank::Viewport     TreeBank::m_Viewport = Widget;
SharedViewList TreeBank::m_AllViews = SharedViewList();
SharedViewList TreeBank::m_ViewUpdateBox = SharedViewList();
QList<Node*> TreeBank::m_NodeDeleteBox = QList<Node*>();

QMap<QKeySequence, QString> TreeBank::m_KeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> TreeBank::m_MouseMap = QMap<QString, QString>();

static void CollectViewDom(QDomElement &elem, ViewNode *parent);
static void CollectHistDom(QDomElement &elem, HistNode *parent, ViewNode *partner);
static void CollectViewNode(ViewNode *nd, QDomElement &elem);
static void CollectHistNode(HistNode *nd, QDomElement &elem);
static void CollectViewNodeFlat(ViewNode *nd, QTextStream &out);
static void CollectHistNodeFlat(HistNode *nd, QTextStream &out);

static SharedView LoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView LoadWithLink(HistNode *hn);
static SharedView LoadWithLink(ViewNode *vn);
static SharedView LoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView LoadWithNoLink(HistNode *hn);
static SharedView LoadWithNoLink(ViewNode *vn);
static SharedView AutoLoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView AutoLoadWithLink(HistNode *hn);
static SharedView AutoLoadWithLink(ViewNode *vn);
static SharedView AutoLoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView AutoLoadWithNoLink(HistNode *hn);
static SharedView AutoLoadWithNoLink(ViewNode *vn);
static void LinkNode(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static void LinkNode(HistNode* hn);
static void SetHistProp(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static void SetHistProp(HistNode* hn);
static void SetPartner(QUrl url, ViewNode *vn, HistNode *hn, SharedView view = 0);
static void SetPartner(QUrl url, HistNode *hn, ViewNode *vn, SharedView view = 0);

// for renaming directory
static QString GetNetworkSpaceID(ViewNode*);
static QString GetNetworkSpaceID(HistNode*);
static QString GetNetworkSpaceID(SharedView);
static QStringList GetNodeSettings(ViewNode*);
static QStringList GetNodeSettings(HistNode*);
static QStringList GetNodeSettings(SharedView);

TreeBank::TreeBank(QWidget *parent)
    : QGraphicsView(new QGraphicsScene(parent), parent)
    , m_Scene(scene())
  //, m_Gadgets(new Gadgets(this))
    , m_JsObject(new _Vanilla(this))
      // if m_PurgeView is true, Notifier and Receiver should be purged.
    , m_Notifier(new Notifier(this, m_PurgeNotifier || m_PurgeView))
    , m_Receiver(new Receiver(this, m_PurgeReceiver || m_PurgeView))
{
    m_Gadgets_ = SharedView(new Gadgets(this), &DeleteView);
    m_Gadgets_->SetThis(WeakView(m_Gadgets_));
    m_Gadgets = static_cast<Gadgets*>(m_Gadgets_.get());

    if(m_Viewport == GLWidget){
        // notifier and receiver become black (if not purged)...
        // scrollbar become invisible...
        setViewport(new QGLWidget(QGLFormat(QGL::DoubleBuffer)));

        // if displaying WebView, become not to repaint.
        // after resized, return to normal.

        // if displaying WeEngineView, it become transparent.
        // because QGLWidget not supports multi-thread,
        // but WebEngineView requests to paint in multi-thread.
    } else if(m_Viewport == OpenGLWidget){
        // font become dirty...
        setViewport(new QOpenGLWidget());
    }

    setAcceptDrops(true);
    setFrameShape(NoFrame);
    setBackgroundBrush(Qt::black);
    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing |
                         QGraphicsView::DontSavePainterState);

    m_Scene->addItem(m_Gadgets);
    m_Scene->setSceneRect(QRect(0, 0, parent->width(), parent->height()));

    m_CurrentView = SharedView();
    m_CurrentViewNode = 0;
    m_CurrentHistNode = 0;
    m_ActionTable = QMap<TreeBankAction, QAction*>();

    // gadgets become too slow when using reconnection.
    connect(m_Gadgets,  SIGNAL(titleChanged(QString)),
            parent,     SLOT(SetWindowTitle(QString)));

    ConnectToNotifier();
    ConnectToReceiver();
}

TreeBank::~TreeBank(){
    if(m_Notifier) m_Notifier->deleteLater();
    if(m_Receiver) m_Receiver->deleteLater();
    //m_Gadgets->deleteLater();
}

void TreeBank::Initialize(){
    LoadSettings();

    m_HistRoot  = new HistNode();
    m_ViewRoot  = new ViewNode();
    m_TrashRoot = new ViewNode();
    m_ViewRoot->SetTitle(m_RootName);
    m_TrashRoot->SetTitle(QStringLiteral("trash;noload"));
    m_HistRoot->SetTitle(QStringLiteral("history"));
    m_TraverseMode = Neutral;
}

void TreeBank::ConnectToNotifier(){
    if(m_Notifier){
        connect(m_Gadgets,  SIGNAL(statusBarMessage(const QString&)),
                m_Notifier, SLOT(SetStatus(const QString&)));
        connect(m_Gadgets,  SIGNAL(statusBarMessage2(const QString&, const QString&)),
                m_Notifier, SLOT(SetStatus(const QString&, const QString&)));
        connect(m_Gadgets,  SIGNAL(ItemHovered(const QString&, const QString&, const QString&)),
                m_Notifier, SLOT(SetLink(const QString&, const QString&, const QString&)));

        connect(m_Gadgets,  SIGNAL(ScrollChanged(QPointF)),
                m_Notifier, SLOT(SetScroll(QPointF)));
        connect(m_Notifier, SIGNAL(ScrollRequest(QPointF)),
                m_Gadgets,  SLOT(SetScroll(QPointF)));

        connect(Application::GetAutoSaver(), SIGNAL(Started()),
                m_Notifier, SLOT(AutoSaveStarted()));
        connect(Application::GetAutoSaver(), SIGNAL(Failed()),
                m_Notifier, SLOT(AutoSaveFailed()));
        connect(Application::GetAutoSaver(), SIGNAL(Finished(const QString&)),
                m_Notifier, SLOT(AutoSaveFinished(const QString&)));
    }
}

void TreeBank::ConnectToReceiver(){
    if(m_Receiver){
        connect(m_Receiver, SIGNAL(OpenUrl(QUrl)),
                this, SLOT(OpenInNewIfNeed(QUrl)));
        connect(m_Receiver, SIGNAL(OpenUrl(QList<QUrl>)),
                this, SLOT(OpenInNewIfNeed(QList<QUrl>)));
        connect(m_Receiver, SIGNAL(OpenQueryUrl(QString)),
                this, SLOT(OpenInNewIfNeed(QString)));
        connect(m_Receiver, SIGNAL(SearchWith(QString, QString)),
                this, SLOT(OpenInNewIfNeed(QString, QString)));
    }
}

bool TreeBank::RenameNode(Node *nd){
    // The return value is whether it was actually updated.
    bool ok;
    QString before = nd->GetTitle();

    QString title = ModalDialog::GetText
        (tr("Input new node name."),
         tr("Node name:"),
         before, &ok);

    QString after = title;

    if(!ok || before == after)
        return false;

    if(title.isEmpty() ||
       title.contains(QRegExp(QStringLiteral("[<>\":\\?\\|\\*/\\\\]")))){

        ModelessDialog::Information
            (tr("Invalid node name."),
             tr("Cannot change to empty title, and cannot use following charactor.\n"
                "\\	/	:	*	?	\"	<	>	|"));
        return false;
    }

    nd->SetTitle(title);

    if(nd->IsViewNode() && nd->IsDirectory())
        ReconfigureDirectory(nd->ToViewNode(), before, after);

    return ok;
}

void TreeBank::ReconfigureDirectory(ViewNode *vn, QString before, QString after){

    // reset network access manager and
    // apply view specific settings.

    QString parentid = GetNetworkSpaceID(vn->GetParent()->ToViewNode());
    QString befname = before.split(QStringLiteral(";")).first();
    QString aftname = after .split(QStringLiteral(";")).first();
    QStringList befset = before.split(QStringLiteral(";")).mid(1);
    QStringList aftset = after .split(QStringLiteral(";")).mid(1);
    // 'QStringList::indexOf' uses 'QRegExp::exactMatch'.
    bool bef = befset.indexOf(QRegExp(QStringLiteral("[iI][dD]"))) != -1;
    bool aft = aftset.indexOf(QRegExp(QStringLiteral("[iI][dD]"))) != -1;

    NetworkAccessManager *nam = bef ?
        (aft ? NetworkController::MoveNetworkAccessManager(befname, aftname, aftset) :
               NetworkController::KillNetworkAccessManager(befname)) :
        (aft ? NetworkController::CopyNetworkAccessManager(parentid, aftname, aftset) :
               NetworkController::GetNetworkAccessManager(parentid, aftset));

    if(!nam) nam = NetworkController::GetNetworkAccessManager(parentid, aftset);

    foreach(Node *nd, vn->GetChildren()){
        ApplySpecificSettings(nd->ToViewNode(), nam, aftset, aftname, parentid);
    }
}

void TreeBank::ApplySpecificSettings(ViewNode *vn, ViewNode *dir){
    if(!dir) dir = vn->GetParent()->ToViewNode();

    ApplySpecificSettings
        (vn,
         NetworkController::GetNetworkAccessManager(GetNetworkSpaceID(vn),
                            GetNodeSettings(vn)),
         GetNodeSettings(vn),
         GetNetworkSpaceID(vn),
         GetNetworkSpaceID(dir));
}

void TreeBank::ApplySpecificSettings(ViewNode *vn, NetworkAccessManager *nam, QStringList set,
                                     QString id, QString parentid){
    if(GetNetworkSpaceID(vn) == id || GetNetworkSpaceID(vn) == parentid){
        if(vn->GetPartner()){
            ApplySpecificSettings(GetRoot(vn->GetPartner())->ToHistNode(), nam, set);
        }
        foreach(Node *nd, vn->GetChildren()){
            ApplySpecificSettings(nd->ToViewNode(), nam, set, id, parentid);
        }
    }
}

void TreeBank::ApplySpecificSettings(HistNode *hn, NetworkAccessManager *nam, QStringList set){
    if(hn->GetView()){
        hn->GetView()->ApplySpecificSettings(set);
    }
    foreach(Node *nd, hn->GetChildren()){
        ApplySpecificSettings(nd->ToHistNode(), nam, set);
    }
}

// for renaming directory
static QString GetNetworkSpaceID(ViewNode* vn){
    QString title;
    forever{
        title = vn->GetTitle();
        if(vn == TreeBank::GetViewRoot() || vn == TreeBank::GetTrashRoot() ||
           (vn->IsDirectory() && !title.isEmpty() &&
            // 'QStringList::indexOf' uses 'QRegExp::exactMatch'.
            title.split(QStringLiteral(";")).indexOf(QRegExp(QStringLiteral("[iI][dD]"))) != -1)){

            return title.split(QStringLiteral(";")).first();

        } else {
            vn = vn->GetParent()->ToViewNode();
        }
    }
}

static QString GetNetworkSpaceID(HistNode* hn){
    return GetNetworkSpaceID(hn->GetPartner()->ToViewNode());
}

static QString GetNetworkSpaceID(SharedView view){
    return GetNetworkSpaceID(view->GetViewNode());
}

static QStringList GetNodeSettings(ViewNode* vn){
    if(!vn) return QStringList();
    QString title;
    forever{
        title = vn->GetTitle();
        if(vn->IsDirectory() && !title.isEmpty()){
            QStringList set = title.split(QStringLiteral(";")).mid(1);
            if(!set.isEmpty())
                return set;
            else if(vn->IsRoot())
                return QStringList();
        }
        vn = vn->GetParent()->ToViewNode();
    }
}

static QStringList GetNodeSettings(HistNode* hn){
    return GetNodeSettings(hn->GetPartner()->ToViewNode());
}

static QStringList GetNodeSettings(SharedView view){
    return GetNodeSettings(view->GetViewNode());
}

void TreeBank::DoUpdate(){
    foreach(SharedView view, m_ViewUpdateBox){
        view->UpdateThumbnail();
        // write access violation in 'render' function.
        //QtConcurrent::run(View::UpdateThumbnail, view);
    }
    m_ViewUpdateBox.clear();
}

void TreeBank::DoDelete(){
    foreach(Node *nd, m_NodeDeleteBox){
        nd->Delete();
    }
    m_NodeDeleteBox.clear();
}

void TreeBank::AddToUpdateBox(SharedView view){
    if(!m_ViewUpdateBox.contains(view)){
        m_ViewUpdateBox << view;
    }
}

void TreeBank::AddToDeleteBox(Node *nd){
    if(!m_NodeDeleteBox.contains(nd)){
        m_NodeDeleteBox << nd;
    }
}

void TreeBank::RemoveFromUpdateBox(SharedView view){
    if(m_ViewUpdateBox.contains(view)){
        m_ViewUpdateBox.removeOne(view);
    }
}

void TreeBank::RemoveFromDeleteBox(Node *nd){
    if(m_NodeDeleteBox.contains(nd)){
        m_NodeDeleteBox.removeOne(nd);
    }
}

// for auto loading
void TreeBank::AutoLoad(){
    if(!m_MaxViewCount || m_AllViews.length() < m_MaxViewCount){
        switch (m_TraverseMode == Neutral ? m_TraverseCondition :
                m_TraverseMode == ViewMode && m_TraverseCondition < 2 ? m_TraverseCondition + 2 :
                m_TraverseMode == HistMode && m_TraverseCondition > 1 ? m_TraverseCondition - 2 :
                m_TraverseCondition){
        case 0:
            if(m_HistIterForward) { LoadHistForward();  break;}
            if(m_HistIterBackward){ LoadHistBackward(); break;}
        case 1:
            if(m_ViewIterForward) { LoadViewForward();  break;}
            if(m_ViewIterBackward){ LoadViewBackward(); break;}
        case 2:
            if(m_HistIterBackward){ LoadHistBackward(); break;}
            if(m_HistIterForward) { LoadHistForward();  break;}
        case 3:
            if(m_ViewIterBackward){ LoadViewBackward(); break;}
            if(m_ViewIterForward) { LoadViewForward();  break;}
        default: m_TraverseCondition = 0;
        }
    }

    if(m_TraverseMode == ViewMode ? (!m_ViewIterForward && !m_ViewIterBackward) :
       m_TraverseMode == HistMode ? (!m_HistIterForward && !m_HistIterBackward) :
       m_TraverseMode == Neutral ?
       (!m_HistIterForward && !m_HistIterBackward && !m_ViewIterForward && !m_ViewIterBackward) :
       (!m_HistIterForward && !m_HistIterBackward && !m_ViewIterForward && !m_ViewIterBackward)){
        Application::StopAutoLoadTimer();
    }
    if(m_AllViews.length() >= m_MaxViewCount){
        Application::StopAutoLoadTimer();
    }

    if(m_TraverseCondition == 3)
        m_TraverseCondition = 0;
    else
        m_TraverseCondition++;
}

void TreeBank::LoadHistForward(){
    if(!m_HistIterForward) return;
    HistNode *hn = m_HistIterForward;

    do m_HistIterForward = hn = hn->Next();
    while(hn && hn->GetView());

    if(hn && !hn->GetUrl().isEmpty() && !hn->GetView())
        AutoLoadWithNoLink(hn);
}

void TreeBank::LoadHistBackward(){
    if(!m_HistIterBackward) return;
    HistNode *hn = m_HistIterBackward;

    do m_HistIterBackward = hn = hn->Prev();
    while(hn && hn->GetView());

    if(hn && !hn->GetUrl().isEmpty() && !hn->GetView())
        AutoLoadWithNoLink(hn);
}

void TreeBank::LoadViewForward(){
    if(!m_ViewIterForward) return;
    ViewNode *vn = m_ViewIterForward;

    if(IsTrash(vn))
        m_ViewIterForward = 0;
    else
        do m_ViewIterForward = vn = vn->Next();
        while(vn && (vn->GetView() || vn->IsDirectory() || vn->GetUrl().isEmpty()));

    if(vn && !vn->GetUrl().isEmpty() && !vn->GetView() && !AutoLoadWithLink(vn))
        LoadViewForward();
}

void TreeBank::LoadViewBackward(){
    if(!m_ViewIterBackward) return;
    ViewNode *vn = m_ViewIterBackward;

    if(IsTrash(vn))
        m_ViewIterBackward = 0;
    else
        do m_ViewIterBackward = vn = vn->Prev();
        while(vn && (vn->GetView() || vn->IsDirectory() || vn->GetUrl().isEmpty()));

    if(vn && !vn->GetUrl().isEmpty() && !vn->GetView() && !AutoLoadWithLink(vn))
        LoadViewBackward();
}

// predicates
Node* TreeBank::GetRoot(Node* nd){
    if(!nd) return 0;
    while(nd->GetParent() && !nd->IsRoot())
        nd = nd->GetParent();
    return nd;
}

Node* TreeBank::GetOtherRoot(Node* nd){
    Node *root = GetRoot(nd);
    if(root->IsHistNode())  return nd;
    if(root == m_ViewRoot)  return m_TrashRoot;
    if(root == m_TrashRoot) return m_ViewRoot;
    return 0;
}

bool TreeBank::IsTrash(Node* nd){
    if(nd->IsViewNode())
        return GetRoot(nd) == m_TrashRoot;
    if(nd->IsHistNode())
        return GetRoot(nd->GetPartner()) == m_TrashRoot;
    return false;
}

bool TreeBank::IsDisplayingTableView(){
    return (m_Gadgets && m_Gadgets->IsActive()) ||
        (m_CurrentView &&
         qobject_cast<LocalView*>(m_CurrentView->base()));
}

bool TreeBank::IsCurrent(Node* nd){
    return nd == m_CurrentViewNode || nd == m_CurrentHistNode;
}

bool TreeBank::IsCurrent(SharedView view){
    return view == m_CurrentView;
}

int TreeBank::WinIndex(){
    WinMap windows = Application::GetMainWindows();
    foreach(int key, windows.keys()){
        if(windows[key] && windows[key]->GetTreeBank() == this)
            return key;
    }
    return 0;
}

int TreeBank::WinIndex(Node* nd){
    WinMap windows = Application::GetMainWindows();
    foreach(int key, windows.keys()){
        if(windows[key] && windows[key]->GetTreeBank()->IsCurrent(nd))
            return key;
    }
    return 0;
}

int TreeBank::WinIndex(SharedView view){
    WinMap windows = Application::GetMainWindows();
    foreach(int key, windows.keys()){
        if(windows[key] && windows[key]->GetTreeBank()->IsCurrent(view))
            return key;
    }
    return 0;
}

void TreeBank::LiftMaxViewCountIfNeed(int now){
    if(m_MaxViewCount <= now)
        m_MaxViewCount = now + 1;
}

// initialize
void TreeBank::LoadTree(){
    Node::SetBooting(true);

    QMap<ViewNode*, QString> m;
    m[m_ViewRoot]  = Application::PrimaryTreeFileName();
    m[m_TrashRoot] = Application::SecondaryTreeFileName();
    foreach(ViewNode *root, QList<ViewNode*>() << m_ViewRoot << m_TrashRoot){

        QString filename = m[root];
        QString path = Application::DataDirectory() + filename;
        QDomDocument doc;
        QFile file(path);
        bool check = doc.setContent(&file);
        file.close();

        if(!check){

            QDir dir = QDir(Application::DataDirectory());
            QStringList list =
                dir.entryList(Application::BackUpFileFilters(),
                              QDir::NoFilter, QDir::Name | QDir::Reversed);

            foreach(QString backup, list){

                if(!backup.contains(filename)) continue;

                QFile backupfile(Application::DataDirectory() + backup);
                check = doc.setContent(&backupfile);
                backupfile.close();

                if(!check) continue;

                ModelessDialog::Information
                    (tr("Restored from a back up file")+ QStringLiteral(" [") + backup + QStringLiteral("]."),
                     tr("Because of a failure to read the latest file, it was restored from a backup file."));
                break;
            }
        }

        QDomNodeList children = doc.documentElement().childNodes();
        for(uint i = 0; i < static_cast<uint>(children.length()); i++){
            CollectViewDom(children.item(i).toElement(), root);
        }
    }
    Node::SetBooting(false);
}

static void CollectViewDom(QDomElement &elem, ViewNode *parent){
    // antifreeze.
    Application::processEvents();

    ViewNode *vn = parent->MakeChild();
    if(elem.attribute(QStringLiteral("primary"), QStringLiteral("false")) == QStringLiteral("true"))
        parent->SetPrimary(vn);
    if(elem.attribute(QStringLiteral("folded"), QStringLiteral("true")) == QStringLiteral("true"))
        vn->SetFolded(true);
    else vn->SetFolded(false);
    if(elem.attribute(QStringLiteral("title"), QString()) != QString())
        vn->SetTitle(elem.attribute(QStringLiteral("title")));

    QString create     = elem.attribute(QStringLiteral("create"),     QString());
    QString lastUpdate = elem.attribute(QStringLiteral("lastupdate"), QString());
    QString lastAccess = elem.attribute(QStringLiteral("lastaccess"), QString());
    vn->SetCreateDate    (create.isEmpty()     ? QDateTime::currentDateTime() : QDateTime::fromString(create,     NODE_DATETIME_FORMAT));
    vn->SetLastUpdateDate(lastUpdate.isEmpty() ? QDateTime::currentDateTime() : QDateTime::fromString(lastUpdate, NODE_DATETIME_FORMAT));

    if(lastAccess.isEmpty())
        vn->SetLastAccessDate(QDateTime::currentDateTime());
    else if(create == lastAccess)
        vn->SetLastAccessDate(QDateTime::fromString(lastAccess, NODE_DATETIME_FORMAT).addSecs(1));
    else
        vn->SetLastAccessDate(QDateTime::fromString(lastAccess, NODE_DATETIME_FORMAT));

    if(elem.attribute(QStringLiteral("holdview"), QStringLiteral("false")) == QStringLiteral("true")){
        CollectHistDom(elem.firstChildElement(), TreeBank::GetHistRoot(), vn);
    } else {
        QDomNodeList children = elem.childNodes();

        for(uint i = 0; i < static_cast<uint>(children.length()); i++){
            CollectViewDom(children.item(i).toElement(), vn);
        }
    }
}

static void CollectHistDom(QDomElement &elem, HistNode *parent, ViewNode *partner){
    HistNode *hn = parent->MakeChild();
    int  id  = elem.attribute(QStringLiteral("index"), QStringLiteral("0")).toInt();
    bool prt = elem.attribute(QStringLiteral("partner"), QStringLiteral("false")) == QStringLiteral("true");
    bool cur = id != 0;

    hn->SetFolded(elem.attribute(QStringLiteral("folded"), QStringLiteral("true")) == QStringLiteral("true") ? true : false);

    hn->SetTitle(elem.attribute(QStringLiteral("title")));
    hn->SetUrl(QUrl::fromEncoded(elem.attribute(QStringLiteral("url")).toLatin1()));

    hn->SetScrollX(elem.attribute(QStringLiteral("scrollx"), QStringLiteral("0")).toInt());
    hn->SetScrollY(elem.attribute(QStringLiteral("scrolly"), QStringLiteral("0")).toInt());
    float zoom = elem.attribute(QStringLiteral("zoom"), QStringLiteral("1.0")).toFloat();
    if(zoom > 10) hn->SetZoom(zoom/100);
    else          hn->SetZoom(zoom);

    QString create     = elem.attribute(QStringLiteral("create"),     QString());
    QString lastUpdate = elem.attribute(QStringLiteral("lastupdate"), QString());
    QString lastAccess = elem.attribute(QStringLiteral("lastaccess"), QString());
    hn->SetCreateDate    (create.isEmpty()     ? QDateTime::currentDateTime() : QDateTime::fromString(create,     NODE_DATETIME_FORMAT));
    hn->SetLastUpdateDate(lastUpdate.isEmpty() ? QDateTime::currentDateTime() : QDateTime::fromString(lastUpdate, NODE_DATETIME_FORMAT));

    if(lastAccess.isEmpty())
        hn->SetLastAccessDate(QDateTime::currentDateTime());
    else if(create == lastAccess)
        hn->SetLastAccessDate(QDateTime::fromString(lastAccess, NODE_DATETIME_FORMAT).addSecs(1));
    else
        hn->SetLastAccessDate(QDateTime::fromString(lastAccess, NODE_DATETIME_FORMAT));

    hn->SetPartner(partner);

    if(!elem.attribute(QStringLiteral("history")).isEmpty()){
        hn->SetHistoryFileName(elem.attribute(QStringLiteral("history")));
    }

    if(!elem.attribute(QStringLiteral("thumb")).isEmpty()){
        hn->SetImageFileName(elem.attribute(QStringLiteral("thumb")));
    }

    if(cur){
        partner->SetPartner(hn);
        LoadWithLink(hn);

        MainWindow *win = Application::NewWindow(id);
        TreeBank *tb = win->GetTreeBank();
        tb->SetCurrent(hn);

    } else if(prt){
        LinkNode(hn);
    } else {
        SetHistProp(hn);
    }

    QDomNodeList children = elem.childNodes();
    for(uint i = 0; i < static_cast<uint>(children.length()); i++){
        CollectHistDom(children.item(i).toElement(), hn, partner);
    }
}

void TreeBank::SaveTree(){
    DoDelete();

    QString primary =
        Application::DataDirectory() +
        Application::PrimaryTreeFileName(false);

    QString primaryb =
        Application::DataDirectory() +
        Application::PrimaryTreeFileName(true);

    QString secondary =
        Application::DataDirectory() +
        Application::SecondaryTreeFileName(false);

    QString secondaryb =
        Application::DataDirectory() +
        Application::SecondaryTreeFileName(true);

    if(QFile::exists(primaryb))   QFile::remove(primaryb);
    if(QFile::exists(secondaryb)) QFile::remove(secondaryb);

    QMap<ViewNode*, QStringList> map;
    map[m_ViewRoot]  = QStringList() << QStringLiteral("viewnode") << Application::PrimaryTreeFileName(true);
    map[m_TrashRoot] = QStringList() << QStringLiteral("trash")    << Application::SecondaryTreeFileName(true);

    foreach(ViewNode *vn, QList<ViewNode*>()
            << m_ViewRoot << m_TrashRoot){
        QString name = map[vn][0];
        QString filename = map[vn][1];

        QString path = Application::DataDirectory() + filename;
        QFile file(path);

#ifdef FAST_SAVER
        if(file.open(QIODevice::WriteOnly)){
            QTextStream out(&file);
            out.setCodec(QTextCodec::codecForName("UTF-8"));
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            out << "<viewnode root=\"true\">\n";
            foreach(Node *child, vn->GetChildren()){
                CollectViewNodeFlat(child->ToViewNode(), out);
            }
            out << "</viewnode>\n";
        }
        file.close();
#else
        QDomDocument doc;
        doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
        QDomElement root = doc.createElement(QStringLiteral("viewnode"));
        root.setAttribute(QStringLiteral("root"), QStringLiteral("true"));
        doc.appendChild(root);
        foreach(Node *child, vn->GetChildren()){
            CollectViewNode(child->ToViewNode(), root);
        }
        if(file.open(QIODevice::WriteOnly)){
            QTextStream out(&file);
            doc.save(out, 2);
        }
        file.close();
#endif
    }

    if(QFile::exists(primary))   QFile::remove(primary);
    if(QFile::exists(secondary)) QFile::remove(secondary);

    QFile::rename(primaryb,   primary);
    QFile::rename(secondaryb, secondary);
}

static void CollectViewNode(ViewNode *nd, QDomElement &elem){
    QDomElement child = elem.ownerDocument().createElement(QStringLiteral("viewnode"));
    child.setAttribute(QStringLiteral("primary"),    nd->IsPrimaryOfParent() ? QStringLiteral("true") : QStringLiteral("false"));
    child.setAttribute(QStringLiteral("holdview"),   nd->GetPartner()        ? QStringLiteral("true") : QStringLiteral("false"));
    child.setAttribute(QStringLiteral("folded"),     nd->GetFolded()         ? QStringLiteral("true") : QStringLiteral("false"));
    child.setAttribute(QStringLiteral("title"),      nd->GetTitle());

    child.setAttribute(QStringLiteral("create"),     nd->GetCreateDate().toString(NODE_DATETIME_FORMAT));
    child.setAttribute(QStringLiteral("lastupdate"), nd->GetLastUpdateDate().toString(NODE_DATETIME_FORMAT));
    child.setAttribute(QStringLiteral("lastaccess"), nd->GetLastAccessDate().toString(NODE_DATETIME_FORMAT));

    elem.appendChild(child);
    if(nd->GetPartner()){
        CollectHistNode(TreeBank::GetRoot(nd->GetPartner())->ToHistNode(), child);
    } else {
        foreach(Node *childnode, nd->GetChildren()){
            CollectViewNode(childnode->ToViewNode(), child);
        }
    }
}

static void CollectHistNode(HistNode *nd, QDomElement &elem){
    QDomElement child = elem.ownerDocument().createElement(QStringLiteral("histnode"));
    child.setAttribute(QStringLiteral("primary"), nd->IsPrimaryOfParent()  ? QStringLiteral("true") : QStringLiteral("false"));
    child.setAttribute(QStringLiteral("partner"), nd->IsPartnerOfPartner() ? QStringLiteral("true") : QStringLiteral("false"));
    child.setAttribute(QStringLiteral("folded"),  nd->GetFolded()          ? QStringLiteral("true") : QStringLiteral("false"));
    child.setAttribute(QStringLiteral("index"), TreeBank::WinIndex(nd));
    child.setAttribute(QStringLiteral("title"), nd->GetTitle());
    child.setAttribute(QStringLiteral("url"), QString::fromUtf8(nd->GetUrl().toEncoded()));
    child.setAttribute(QStringLiteral("scrollx"), nd->GetScrollX());
    child.setAttribute(QStringLiteral("scrolly"), nd->GetScrollY());
    child.setAttribute(QStringLiteral("zoom"), nd->GetZoom());

    child.setAttribute(QStringLiteral("create"),     nd->GetCreateDate().toString(NODE_DATETIME_FORMAT));
    child.setAttribute(QStringLiteral("lastupdate"), nd->GetLastUpdateDate().toString(NODE_DATETIME_FORMAT));
    child.setAttribute(QStringLiteral("lastaccess"), nd->GetLastAccessDate().toString(NODE_DATETIME_FORMAT));

    if(!TreeBank::IsTrash(nd) && nd->IsPartnerOfPartner()){

        // 'm_HistoryFileName' and 'm_ImageFileName' will be generated automatically,
        // when calling save method.
        nd->SaveHistoryIfNeed();
        nd->SaveImageIfNeed();
        child.setAttribute(QStringLiteral("history"), nd->GetHistoryFileName());
        child.setAttribute(QStringLiteral("thumb"), nd->GetImageFileName());
    }

    elem.appendChild(child);
    foreach(Node *childnode, nd->GetChildren()){
        CollectHistNode(childnode->ToHistNode(), child);
    }
}

static void CollectViewNodeFlat(ViewNode *nd, QTextStream &out){
    out << "<viewnode"
        " primary=\""    << (nd->IsPrimaryOfParent() ? "true" : "false")           << "\""
        " holdview=\""   << (nd->GetPartner()        ? "true" : "false")           << "\""
        " folded=\""     << (nd->GetFolded()         ? "true" : "false")           << "\""
        " title=\""      << nd->GetTitle().toHtmlEscaped()                         << "\""
        " create=\""     << nd->GetCreateDate().toString(NODE_DATETIME_FORMAT)     << "\""
        " lastupdate=\"" << nd->GetLastUpdateDate().toString(NODE_DATETIME_FORMAT) << "\""
        " lastaccess=\"" << nd->GetLastAccessDate().toString(NODE_DATETIME_FORMAT) << "\""
        ">\n";

    if(nd->GetPartner()){
        CollectHistNodeFlat(TreeBank::GetRoot(nd->GetPartner())->ToHistNode(), out);
    } else {
        foreach(Node *childnode, nd->GetChildren()){
            CollectViewNodeFlat(childnode->ToViewNode(), out);
        }
    }
    out << "</viewnode>\n";
}

static void CollectHistNodeFlat(HistNode *nd, QTextStream &out){
    out << "<histnode"
        " primary=\""    << (nd->IsPrimaryOfParent()  ? "true" : "false")          << "\""
        " partner=\""    << (nd->IsPartnerOfPartner() ? "true" : "false")          << "\""
        " folded=\""     << (nd->GetFolded()          ? "true" : "false")          << "\""
        " index=\""      << TreeBank::WinIndex(nd)                                 << "\""
        " title=\""      << nd->GetTitle().toHtmlEscaped()                         << "\""
        " url=\""        << QString::fromUtf8(nd->GetUrl().toEncoded()).toHtmlEscaped() << "\""
        " scrollx=\""    << nd->GetScrollX()                                       << "\""
        " scrolly=\""    << nd->GetScrollY()                                       << "\""
        " zoom=\""       << nd->GetZoom()                                          << "\""
        " create=\""     << nd->GetCreateDate().toString(NODE_DATETIME_FORMAT)     << "\""
        " lastupdate=\"" << nd->GetLastUpdateDate().toString(NODE_DATETIME_FORMAT) << "\""
        " lastaccess=\"" << nd->GetLastAccessDate().toString(NODE_DATETIME_FORMAT) << "\""
        ;

    if(!TreeBank::IsTrash(nd) && nd->IsPartnerOfPartner()){

        // 'm_HistoryFileName' and 'm_ImageFileName' will be generated automatically,
        // when calling save method.
        nd->SaveImageIfNeed();
        nd->SaveHistoryIfNeed();
        out <<
            " history=\"" << nd->GetHistoryFileName() << "\""
            " thumb=\""   << nd->GetImageFileName()   << "\"";
    }
    out << ">\n";
    foreach(Node *childnode, nd->GetChildren()){
        CollectHistNodeFlat(childnode->ToHistNode(), out);
    }
    out << "</histnode>\n";
}

void TreeBank::LoadSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("application"));{
        m_RootName             = settings->value(QStringLiteral("@RootName"), QStringLiteral("root;id")).value<QString>();
        m_MaxViewCount         = settings->value(QStringLiteral("@MaxViewCount")         , -1)   .value<int>();
        m_MaxTrashEntryCount   = settings->value(QStringLiteral("@MaxTrashEntryCount")   , -1)   .value<int>();
        m_TraverseAllView      = settings->value(QStringLiteral("@TraverseAllView")      , false).value<bool>();
        m_PurgeNotifier        = settings->value(QStringLiteral("@PurgeNotifier")        , false).value<bool>();
        m_PurgeReceiver        = settings->value(QStringLiteral("@PurgeReceiver")        , false).value<bool>();
        m_PurgeView            = settings->value(QStringLiteral("@PurgeView")            , false).value<bool>();

        QString viewport = settings->value(QStringLiteral("@Viewport"), QStringLiteral("Widget")).value<QString>();
        if(viewport == QStringLiteral("Widget"))       m_Viewport = Widget;
        if(viewport == QStringLiteral("GLWidget"))     m_Viewport = GLWidget;
        if(viewport == QStringLiteral("OpenGLWidget")) m_Viewport = OpenGLWidget;

        if(m_MaxViewCount == -1)       m_MaxViewCount       = settings->value(QStringLiteral("@MaxView")  , -1).value<int>();
        if(m_MaxViewCount == -1)       m_MaxViewCount       = 10;
        if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = settings->value(QStringLiteral("@MaxTrash") , -1).value<int>();
        if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = 100;

        settings->beginGroup(QStringLiteral("keymap"));{
            QStringList keys = settings->allKeys();
            if(keys.isEmpty()){
                /* default key map. */
                TREEBANK_KEYMAP
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
                TREEBANK_MOUSEMAP
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
    }
    settings->endGroup();
    Node::LoadSettings();
    View::LoadSettings();
    Gadgets::LoadSettings();
}

void TreeBank::SaveSettings(){
    QSettings *settings = Application::GlobalSettings();
    settings->beginGroup(QStringLiteral("application"));{
        settings->setValue(QStringLiteral("@RootName"),             m_RootName);
        settings->setValue(QStringLiteral("@MaxViewCount"),         m_MaxViewCount);
        settings->setValue(QStringLiteral("@MaxTrashEntryCount"),   m_MaxTrashEntryCount);
        settings->setValue(QStringLiteral("@TraverseAllView"),      m_TraverseAllView);
        settings->setValue(QStringLiteral("@PurgeNotifier"),        m_PurgeNotifier);
        settings->setValue(QStringLiteral("@PurgeReceiver"),        m_PurgeReceiver);
        settings->setValue(QStringLiteral("@PurgeView"),            m_PurgeView);

        if(m_Viewport == Widget)       settings->setValue(QStringLiteral("@Viewport"), QStringLiteral("Widget"));
        if(m_Viewport == GLWidget)     settings->setValue(QStringLiteral("@Viewport"), QStringLiteral("GLWidget"));
        if(m_Viewport == OpenGLWidget) settings->setValue(QStringLiteral("@Viewport"), QStringLiteral("OpenGLWidget"));

        settings->beginGroup(QStringLiteral("keymap"));{
            QList<QKeySequence> seqs = m_KeyMap.keys();
            if(seqs.isEmpty()){
                /* default key map. */
            } else {
                foreach(QKeySequence seq, seqs){
                    if(!seq.isEmpty() && !seq.toString().isEmpty()){
                        settings->setValue(seq.toString()
                                           // cannot use slashes on QSettings.
                                             .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                                             .replace(QStringLiteral("/"), QStringLiteral("Slash")),
                                           m_KeyMap[seq]);
                    }
                }
            }
        }
        settings->endGroup();
        settings->beginGroup(QStringLiteral("mouse"));{
            QStringList buttons = m_MouseMap.keys();
            if(buttons.isEmpty()){
                /* 'm_KeyMap' will be construct, when next startup. */
            } else {
                foreach(QString button, buttons){
                    if(!button.isEmpty())
                        settings->setValue(button, m_MouseMap[button]);
                }
            }
        }
        settings->endGroup();
    }
    settings->endGroup();
    Node::SaveSettings();
    View::SaveSettings();
    Gadgets::SaveSettings();
}

// for deletion.
void TreeBank::QuarantineViewNode(ViewNode *vn){
    TreeBank *tb = vn->GetView() ? vn->GetView()->GetTreeBank() : 0;
    if(!tb) return;

    if(vn == tb->GetCurrentViewNode())  tb->SetCurrentViewNode(0);
    if(vn == tb->GetViewIterForward())  tb->SetViewIterForward(0);
    if(vn == tb->GetViewIterBackward()) tb->SetViewIterBackward(0);
}

void TreeBank::QuarantineHistNode(HistNode *hn){
    HistNode *parent  = hn->IsRoot() ? 0 : hn->GetParent()->ToHistNode();
    ViewNode *partner = hn->GetPartner()->ToViewNode();
    TreeBank *tb = hn->GetView() ? hn->GetView()->GetTreeBank() : 0;

    if(partner && partner->GetPartner() &&
       (partner->GetPartner() == hn ||
        partner->GetPartner()->IsDescendantOf(hn))){
        partner->SetPartner(parent);
    }

    if(!tb) return;

    if(tb->GetCurrentHistNode() &&
       (tb->GetCurrentHistNode() == hn ||
        tb->GetCurrentHistNode()->IsDescendantOf(hn))){
        tb->SetCurrentHistNode(parent);
    }
    if(tb->GetHistIterForward() &&
       (tb->GetHistIterForward() == hn ||
        tb->GetHistIterForward()->IsDescendantOf(hn))){
        tb->SetHistIterForward(parent);
    }
    if(tb->GetHistIterBackward() &&
       (tb->GetHistIterBackward() == hn ||
        tb->GetHistIterBackward()->IsDescendantOf(hn))){
        tb->SetHistIterBackward(parent);
    }
}

void TreeBank::DisownNode(Node *nd){
    nd->GetParent()->RemoveChild(nd);
    if(nd->IsPrimaryOfParent()){
        if(nd->HasNoSiblings())
            nd->GetParent()->SetPrimary(0);
        else
            nd->GetParent()->SetPrimary(nd->GetFirstSibling());
    }
}

void TreeBank::DislinkView(HistNode *hn){
    if(View *view = hn->GetView()){
        if(SharedView v = view->GetThis().lock()){

            Node *partner = hn->GetPartner();
            TreeBank *tb = v->GetTreeBank();

            if(tb && v == tb->GetCurrentView()){
                v->Disconnect(tb);
                tb->SetCurrentView(0);
            }
            if(partner && partner->GetPartner() == hn){
                partner->SetView(0);
            }
            hn->SetView(0);

            ReleaseView(v);
        }
    }
}

void TreeBank::DietSubTree(HistNode *hn, QList<HistNode*> ignore){
    if(!ignore.contains(hn)){
        DislinkView(hn);
    }
    foreach(Node *child, hn->GetChildren()){
        DietSubTree(child->ToHistNode(), ignore);
    }
}

bool TreeBank::DeleteHistory(HistNode *hn){
    if(hn->IsRoot()){
        // when deleting root of hist tree,
        // delete root and replace to primary.
        // (root need to have only one node.)

        if(hn->HasNoChildren()) return false;

        if(hn->ChildrenLength() > 1){
            HistNode *primary = hn->GetPrimary()->ToHistNode();

            if(!primary)
                primary = hn->GetFirstChild()->ToHistNode();

            Q_ASSERT(primary);

            foreach(Node *child, hn->GetChildren()){
                if(child != primary){
                    // child's descendants must be deleted.
                    // so 'hn' must be root.
                    // root can be deleted, because all of sub tree were deleted.
                    DeleteHistory(child->ToHistNode());
                }
            }
        }

        // replace head of history.
        HistNode *primary = hn->GetFirstChild()->ToHistNode();
        HistNode *parent  = hn->GetParent()->ToHistNode();
        ViewNode *partner = hn->GetPartner()->ToViewNode();
        TreeBank *tb = hn->GetView() ? hn->GetView()->GetTreeBank() : 0;

        // if allow non root node, the length of children is not 1.
        Q_ASSERT(hn->ChildrenLength() == 1);
        Q_ASSERT(m_HistRoot->ChildrenContains(hn));
        Q_ASSERT(primary->GetParent() == hn);
        Q_ASSERT(parent == m_HistRoot);
        Q_ASSERT(partner);

        // strip(not recursive).
        hn->SetImage(QImage());
        DislinkView(hn); // using partner.

        // quarantine(to use primary).
        if(parent->GetPrimary() == hn)  parent->SetPrimary(primary);
        if(partner->GetPartner() == hn) partner->SetPartner(primary);
        if(tb){
            if(tb->GetCurrentHistNode()  == hn) tb->SetCurrentHistNode(primary);
            if(tb->GetHistIterForward()  == hn) tb->SetHistIterForward(primary);
            if(tb->GetHistIterBackward() == hn) tb->SetHistIterBackward(primary);
        }

        // replace child(instead of disown).
        parent->RemoveChild(hn);
        parent->AppendChild(primary);
        primary->SetParent(parent);
        hn->SetPrimary(0);
        hn->SetPartner(0);
        hn->SetParent(0);
        hn->ClearChildren();
    } else {
        // delete recursively and completely.
        QuarantineHistNode(hn);
        DisownNode(hn);
        StripSubTree(hn);
    }

    if(Application::EnableAutoSave()){
        AddToDeleteBox(hn);
    } else {
        hn->Delete();
    }

    return true;
}

bool TreeBank::MoveToTrash(ViewNode *vn){
    // 'm_ViewRoot' and 'm_TrashRoot' are not deletable,
    // because they are base of all data.
    if(vn->IsRoot()) return false;

    // node in trash is not deletable.
    // should be deleted completely or be restored?
    // it's ambiguous.
    if(GetRoot(vn) == m_TrashRoot) return false;

    // when deleting node in main tree,
    // move node to trash.
    MoveNode(vn, m_TrashRoot, 0);
    while(m_TrashRoot->ChildrenLength() > m_MaxTrashEntryCount){
        Node *needless = m_TrashRoot->TakeLastChild();
        DisownNode(needless);
        StripSubTree(needless->ToViewNode());

        if(Application::EnableAutoSave()){
            AddToDeleteBox(needless);
        } else {
            needless->Delete();
        }
    }
    StripSubTree(vn);
    return true;
}

void TreeBank::StripSubTree(Node *nd){
    if(nd->IsViewNode()){
        QuarantineViewNode(nd->ToViewNode());

        if(nd->GetPartner())
            StripSubTree(GetRoot(nd->GetPartner()));

    } else if(nd->IsHistNode()){
        // it's not 'QuarantineHistNode'.
        // want to clear, not to reset to other node.

        TreeBank *tb = nd->GetView() ? nd->GetView()->GetTreeBank() : 0;

        if(tb){
            if(nd == tb->GetCurrentHistNode())  tb->SetCurrentHistNode(0);
            if(nd == tb->GetHistIterForward())  tb->SetHistIterForward(0);
            if(nd == tb->GetHistIterBackward()) tb->SetHistIterBackward(0);
        }

        DislinkView(nd->ToHistNode()); // using partner.
        nd->SetImage(QImage());
    }
    foreach(Node *child, nd->GetChildren()){
        StripSubTree(child);
    }
}

void TreeBank::ReleaseView(SharedView view){
    m_AllViews.removeOne(view);
    m_ViewUpdateBox.removeOne(view);
}

void TreeBank::ReleaseAllView(){
    // when quit application.
    foreach(SharedView view, m_AllViews){
        view->DeleteLater();
    }
    m_AllViews.clear();
}

void TreeBank::ClearCache(Node *nd){
    if(!nd) return;

    if(m_CurrentHistNode && !nd->IsHistNode() && nd != m_CurrentViewNode){
        QList<HistNode*> ignore;
        HistNode *hist = m_CurrentHistNode;
        HistNode *root = 0;
        while(!hist->IsRoot())
            hist = hist->GetParent()->ToHistNode();
        root = hist;
        ignore << hist;
        while(hist->GetPrimary()){
            ignore << hist->GetPrimary()->ToHistNode();
            hist = hist->GetPrimary()->ToHistNode();
        }
        DietSubTree(root, ignore);
    }
}

void TreeBank::RaiseDisplayedViewPriority(){
    if(m_AllViews.length() > 1){
        foreach(MainWindow *win, Application::GetMainWindows()){
            if(SharedView view = win->GetTreeBank()->GetCurrentView()){
                m_AllViews.move(m_AllViews.indexOf(view), 0);
            }
        }
    }
}

// deleting function.
// histnode : delete histnode(recursive) completely.
// viewnode(not trash) : move to trash.
// viewnode(trash) : do nothing.

bool TreeBank::DeleteNode(Node *nd){
    if(!nd) return false;
    return DeleteNode(NodeList() << nd);
}

bool TreeBank::DeleteNode(NodeList list){
    if(list.isEmpty()) return false;

    Application::RestartAutoLoadTimer();

    //sample for predicate.
    Node *sample = list.first();
    Node *prevpartner = sample->GetPartner();
    Node *prevparent  = sample->GetParent();

    if(sample->IsViewNode()){
        bool deleted = false;

        foreach(Node *nd, list){
            Q_ASSERT(nd->IsViewNode());
            // always do 'MoveToTrash'.
            deleted = MoveToTrash(nd->ToViewNode()) || deleted;
        }
        if(!deleted) return false;

    } else if(sample->IsHistNode()){
        // delete completely.
        bool shortcut = false;

        // remove duplicates.
        foreach(Node *nd, list){
            // error handling and filter nodes.
            Q_ASSERT(nd->IsHistNode());
            Q_ASSERT(nd != m_HistRoot);

            foreach(Node *n, nd->GetDescendants()){
                list.removeOne(n);
            }
        }

        foreach(Node *nd, list){
            if(!DeleteHistory(nd->ToHistNode())) continue;

            if(!m_CurrentView && m_CurrentHistNode &&
               m_CurrentHistNode != m_HistRoot){
                // always do 'SetCurrent'.
                shortcut = SetCurrent(m_CurrentHistNode) || shortcut;
            }
        }
        if(shortcut) return true;

    } else {
        // other node.
        return false;
    }

    if(!m_CurrentView){
        if(m_AllViews.isEmpty()){
            SetCurrent(m_ViewRoot);
        } else if(sample->IsHistNode()){
            foreach(SharedView view, m_AllViews){
                if(prevpartner == view->GetViewNode())
                    if(SetCurrent(view->GetHistNode()))
                        return true;
            }
            foreach(SharedView view, m_AllViews){
                if(SetCurrent(view->GetHistNode()))
                    return true;
            }
        } else if(sample->IsViewNode()){
            foreach(SharedView view, m_AllViews){
                if(prevparent == view->GetViewNode()->GetParent())
                    if(SetCurrent(view->GetViewNode()))
                        return true;
            }
            foreach(SharedView view, m_AllViews){
                if(SetCurrent(view->GetViewNode()))
                    return true;
            }

            NodeList list = prevparent->GetChildren();
            qSort(list.begin(), list.end(), [](Node *n1, Node *n2){
                    return n1->GetLastAccessDate() > n2->GetLastAccessDate();
                });
            foreach(Node *nd, list){
                if(SetCurrent(nd))
                    return true;
            }
        }
    }
    // return value is whether or not to have removed the node actually.
    return true;
}

// moving function(sort, move to other folder, move to trash).
// this function 'not' call delete view function.
bool TreeBank::MoveNode(Node *nd, Node *dir, int n){
    if(nd->GetParent() == dir){
        // same directory(should use 'SetChildrenOrder', instead of this.).
        // using 'QList::move'.
        dir->MoveChild(dir->ChildrenIndexOf(nd),
                       (n < 0 || n >= dir->ChildrenLength()) ?
                         dir->ChildrenLength() - 1 :
                       (n > dir->ChildrenIndexOf(nd)) ?
                         n - 1 : n);
    } else {
        // move to other directory.
        DisownNode(nd);
        nd->SetParent(dir);
        dir->InsertChild((n < 0 || n > dir->ChildrenLength()) ?
                           dir->ChildrenLength() : n,
                         nd);

        if(nd->IsViewNode())
            ApplySpecificSettings(nd->ToViewNode(), dir->ToViewNode());
    }
    return true;
}

// set dir's children to list.
// this is 'not' removing node api!
// this can only append or sort nodes.
bool TreeBank::SetChildrenOrder(Node *parent, NodeList children){
    if(children.isEmpty() || !parent ||
       children.length() < parent->ChildrenLength() ||
       children.contains(parent) ||
       children.toSet().intersect(parent->GetAncestors().toSet()).count())
        return false;

    parent->ClearChildren();
    parent->SetChildren(children);

    foreach(Node *child, children){
        if(child->GetParent() && child->GetParent() != parent){
            DisownNode(child);
            child->SetParent(parent);

            if(child->IsViewNode())
                ApplySpecificSettings(child->ToViewNode(), parent->ToViewNode());
        }
    }
    return true;
}

// switching function.
bool TreeBank::SetCurrent(Node *nd){
    if(!nd) return false;

    if(!nd->GetPartner()){
        if(nd->HasNoChildren()){
            return false;
        } else if(nd->GetPrimary()){
            return SetCurrent(nd->GetPrimary());
        } else {
            return SetCurrent(nd->GetFirstChild());
        }
    }

    //trying to share same view from different window.
    if(nd->GetView()){
        QList<int> ids = Application::GetMainWindows().keys();
        if(ids.length() > 1){
            int id = WinIndex();
            if(id){
                ids.removeOne(id);
                if(View *view = nd->GetView())
                    if(ids.contains(WinIndex(nd->GetView()->GetThis().lock())))
                        return false;
            }
        }
    }

    ClearCache(nd);

    if(m_Receiver) m_Receiver->hide();
    if(m_Notifier) m_Notifier->ResetStatus();

    SharedView prev = m_CurrentView;

    //node setting
    if(nd->IsHistNode()){
        nd->GetPartner()->SetView(nd->GetView());
        nd->GetPartner()->SetTitle(nd->GetTitle());
        m_CurrentView = nd->GetView() ? nd->GetView()->GetThis().lock() : SharedView();
        m_CurrentHistNode = nd->ToHistNode();
        m_CurrentViewNode = nd->GetPartner()->ToViewNode();
        m_CurrentViewNode->SetPartner(nd);
    }
    if(nd->IsViewNode()){
        m_CurrentView = nd->GetView() ? nd->GetView()->GetThis().lock() : SharedView();
        m_CurrentViewNode = nd->ToViewNode();
        m_CurrentHistNode = nd->GetPartner()->ToHistNode();
        if(!m_CurrentView)
            if(View *v = nd->GetPartner()->GetView())
                m_CurrentView = v->GetThis().lock();
    }

    //reset primary mark
    m_CurrentHistNode->ResetPrimaryPath();
    m_CurrentViewNode->ResetPrimaryPath();

    //reset iterator for auto loading
    m_ViewIterForward = m_ViewIterBackward = m_CurrentViewNode;
    m_HistIterForward = m_HistIterBackward = m_CurrentHistNode;

    if(!m_CurrentView){
        // bind using to load with activate,
        // and reset property.
        m_CurrentView = LoadWithLink(m_CurrentHistNode);

        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }
    Q_ASSERT(m_CurrentView);

    //save current view to image if need.
    if(prev != m_CurrentView){
        if(prev){
            AddToUpdateBox(prev);
            prev->Disconnect(this);
        }
        m_CurrentView->Connect(this);
    }
    if(m_AllViews.length() > 1){
        m_AllViews.move(m_AllViews.indexOf(m_CurrentView), 0);
    }
    if(m_CurrentView->parent() != this){
        m_CurrentView->setParent(this);
    }

    if(m_CurrentView->size() != size() || TreeBank::PurgeView())
        m_CurrentView->resize(size());

    if(!m_CurrentView->GetTitle().isEmpty()){
        GetMainWindow()->SetWindowTitle(m_CurrentView->GetTitle());
    } else if(!m_CurrentView->url().isEmpty()){
        GetMainWindow()->SetWindowTitle(m_CurrentView->url().toString());
    }

    if(SharedView v = m_CurrentView->GetSlave().lock()){
        // why cannot catch lovalview object?
        if(qobject_cast<LocalView*>(v->base())){
            v->lower();
            v->hide();
            v->SetMaster(WeakView());
            m_CurrentView->SetSlave(WeakView());
        }
    }
    if(prev && (prev != m_CurrentView)){
        if(SharedView v = prev->GetMaster().lock()){
            v->lower();
            v->hide();
            v->SetSlave(WeakView());
            prev->SetMaster(WeakView());
        }
    }
    m_Gadgets->SetMaster(m_CurrentView->GetThis());
    m_CurrentView->SetSlave(m_Gadgets->GetThis());

    if(!(prev && prev->ForbidToOverlap()) &&
       m_CurrentView->ForbidToOverlap()){
        PurgeChildWidgetsIfNeed();
    }
    if((prev && prev->ForbidToOverlap()) &&
       !m_CurrentView->ForbidToOverlap()){
        JoinChildWidgetsIfNeed();
    }

    GetMainWindow()->AdjustAllEdgeWidgets();
    if(m_Notifier) m_Notifier->ResizeNotify(size());
    if(m_Receiver) m_Receiver->ResizeNotify(size());

    //                            high
    // QuickWeb(Engine)View        ^
    // Web(Engine)View             |
    // Gadgets               order of layer
    // LocalView                   |
    // GraphicsWebView             V
    //                            low

#ifdef QTWEBKIT
    if(qobject_cast<GraphicsWebView*>(m_CurrentView->base())){

        m_CurrentView->show();
        DoUpdate();
        if(prev && prev != m_CurrentView){
            prev->lower();
            prev->hide();
        }
        m_CurrentView->raise();

    } else
#endif
    if(qobject_cast<LocalView*>(m_CurrentView->base())){

        m_CurrentView->show();
        DoUpdate();
        if(prev && prev != m_CurrentView){
            m_CurrentView->SetMaster(prev);
            prev->SetSlave(m_CurrentView);
#ifdef QTWEBKIT
            if(!qobject_cast<GraphicsWebView*>(prev->base())){
                prev->hide();
            }
#endif
        }
        m_CurrentView->raise();

    } else { // Web(Engine)View or QuickWeb(Engine)View.

        if(m_Gadgets && m_Gadgets->IsActive()){
            /* do nothing. */
            DoUpdate();
        } else {
            m_CurrentView->show();
            DoUpdate();
            if(prev && prev != m_CurrentView){
                prev->hide();
            }
            m_CurrentView->raise();
        }
    }

    if(PurgeView()){
        parentWidget()->raise();
        m_CurrentView->raise();
    }
    GetMainWindow()->RaiseAllEdgeWidgets();
    if(m_Notifier) m_Notifier->raise();
    if(m_Receiver) m_Receiver->raise();

    if(m_Gadgets && !m_Gadgets->IsActive())
        m_CurrentView->setFocus(Qt::OtherFocusReason);

    AddToUpdateBox(m_CurrentView);

    return true;
}

bool TreeBank::SetCurrent(SharedView view){
    return SetCurrent(view->GetHistNode());
}

void TreeBank::BeforeStartingDisplayGadgets(){
    DoUpdate();

    // if current focus is other widget(e.g. 'WebView'),
    // cannot set focus to 'Gadgets' directly.
    parentWidget()->setFocus();
    setFocus();

    if(m_CurrentView)
        m_CurrentView->Disconnect(this);

    m_Gadgets->Connect(this);
    m_Gadgets->setParent(this);

    if(m_CurrentView)
        m_CurrentView->OnBeforeStartingDisplayGadgets();
}

void TreeBank::AfterFinishingDisplayGadgets(){
    // when sender is 'm_Gadgets'.
    if(m_Gadgets && m_Gadgets->IsActive())
        m_Gadgets->Disconnect(this);

    if(m_CurrentView){
        m_CurrentView->Connect(this);

        if(!m_CurrentView->GetTitle().isEmpty())
            GetMainWindow()->SetWindowTitle(m_CurrentView->GetTitle());
        else if(!m_CurrentView->url().isEmpty())
            GetMainWindow()->SetWindowTitle(m_CurrentView->url().toString());
    }

    if(m_CurrentView)
        m_CurrentView->OnAfterFinishingDisplayGadgets();
}

void TreeBank::MousePressEvent(QMouseEvent *ev){
    QGraphicsView::mousePressEvent(ev);
}

void TreeBank::MouseReleaseEvent(QMouseEvent *ev){
    QGraphicsView::mouseReleaseEvent(ev);
}

void TreeBank::MouseMoveEvent(QMouseEvent *ev){
    QGraphicsView::mouseMoveEvent(ev);
}

void TreeBank::MouseDoubleClickEvent(QMouseEvent *ev){
    QGraphicsView::mouseDoubleClickEvent(ev);
}

void TreeBank::WheelEvent(QWheelEvent *ev){
    QGraphicsView::wheelEvent(ev);
}

void TreeBank::DragEnterEvent(QDragEnterEvent *ev){
    QGraphicsView::dragEnterEvent(ev);
}

void TreeBank::DragMoveEvent(QDragMoveEvent *ev){
    QGraphicsView::dragMoveEvent(ev);
}

void TreeBank::DragLeaveEvent(QDragLeaveEvent *ev){
    QGraphicsView::dragLeaveEvent(ev);
}

void TreeBank::DropEvent(QDropEvent *ev){
    QGraphicsView::dropEvent(ev);
}

void TreeBank::ContextMenuEvent(QContextMenuEvent *ev){
    QGraphicsView::contextMenuEvent(ev);
}

void TreeBank::KeyPressEvent(QKeyEvent *ev){
    QGraphicsView::keyPressEvent(ev);
}

void TreeBank::KeyReleaseEvent(QKeyEvent *ev){
    QGraphicsView::keyReleaseEvent(ev);
}

// link event.
////////////////////////////////////////////////////////////////

SharedView TreeBank::OpenInNewViewNode(QNetworkRequest req, bool activate, ViewNode *older){
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    m_TraverseMode = ViewMode;
    if(!older) older = m_CurrentViewNode;
    // failed to open link(bad TreeBank status).
    if(!older){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return 0;
    }
    HistNode *hist  = m_HistRoot->MakeChild();
    // Should be written here?
    hist->SetZoom(older->GetZoom());
    ViewNode *young = older->MakeSibling();
    SharedView view = LoadWithLink(req, hist, young);
    if(activate){
        SetCurrent(hist);
    } else {
        view->hide();

        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenInNewViewNode(QUrl url, bool activate, ViewNode *older){
    return OpenInNewViewNode(QNetworkRequest(url), activate, older);
}

SharedView TreeBank::OpenInNewViewNode(QList<QNetworkRequest> reqs, bool activate, ViewNode *older){
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = reqs.length()-1; i >= 0; i--){
        bool first = (i == reqs.length()-1);
        SharedView view = OpenInNewViewNode(reqs[i], false, older);
        // failed to open link(bad TreeBank status).
        if(!view){
            View::SetSwitchingState(false);
            return 0;
        }
        if(first) v = view;
    }
    if(v && activate) SetCurrent(v->GetViewNode());

    View::SetSwitchingState(false);

    return v;
}

SharedView TreeBank::OpenInNewViewNode(QList<QUrl> urls, bool activate, ViewNode *older){
    QList<QNetworkRequest> reqs;
    foreach(QUrl url, urls) reqs << QNetworkRequest(url);
    return OpenInNewViewNode(reqs, activate, older);
}

////////////////////////////////////////////////////////////////

SharedView TreeBank::OpenOnSuitableNode(QNetworkRequest req, bool activate, ViewNode *parent){
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    m_TraverseMode = ViewMode;
    if(!parent) parent = m_ViewRoot;
    // failed to open link(bad TreeBank status).
    if(!parent){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return 0;
    }
    HistNode *hist = m_HistRoot->MakeChild();
    ViewNode *page = parent->MakeChild();
    SharedView view = LoadWithLink(req, hist, page);
    if(activate){
        SetCurrent(hist);
    } else {
        view->hide();

        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenOnSuitableNode(QUrl url, bool activate, ViewNode *parent){
    return OpenOnSuitableNode(QNetworkRequest(url), activate, parent);
}

SharedView TreeBank::OpenOnSuitableNode(QList<QNetworkRequest> reqs, bool activate, ViewNode *parent){
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = 0; i < reqs.length(); i++){
        bool last = (i == (reqs.length() - 1));
        SharedView view = OpenOnSuitableNode(reqs[i], last && activate, parent);
        // failed to open link(bad TreeBank status).
        if(!view){
            View::SetSwitchingState(false);
            return 0;
        }
        if(last) v = view;
    }

    View::SetSwitchingState(false);

    return v;
}

SharedView TreeBank::OpenOnSuitableNode(QList<QUrl> urls, bool activate, ViewNode *parent){
    QList<QNetworkRequest> reqs;
    foreach(QUrl url, urls) reqs << QNetworkRequest(url);
    return OpenOnSuitableNode(reqs, activate, parent);
}

////////////////////////////////////////////////////////////////

SharedView TreeBank::OpenInNewDirectory(QNetworkRequest req, bool activate, ViewNode *older){
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    m_TraverseMode = ViewMode;
    if(!older) older = m_CurrentViewNode;
    // failed to open link(bad TreeBank status).
    if(!older){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return 0;
    }
    ViewNode *young  = older->NewDir();
    HistNode *hist   = m_HistRoot->MakeChild();
    SharedView view = LoadWithLink(req, hist, young);
    if(activate){
        SetCurrent(hist);
    } else {
        view->hide();

        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenInNewDirectory(QUrl url, bool activate, ViewNode *older){
    return OpenInNewDirectory(QNetworkRequest(url), activate, older);
}

SharedView TreeBank::OpenInNewDirectory(QList<QNetworkRequest> reqs, bool activate, ViewNode *older){
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = 0; i < reqs.length(); i++){
        bool last = (i == (reqs.length() - 1));
        bool first = (i == 0);
        SharedView view = SharedView();
        if(first){
            view = OpenInNewDirectory(reqs[i], last && activate, older);
        } else {
            NodeList sibling = older->GetSiblings();
            view = OpenOnSuitableNode(reqs[i], last && activate,
                                      sibling[sibling.indexOf(older)+1]->ToViewNode());
        }
        // failed to open link(bad TreeBank status).
        if(!view){
            View::SetSwitchingState(false);
            return 0;
        }
        if(last) v = view;
    }

    View::SetSwitchingState(false);

    return v;
}

SharedView TreeBank::OpenInNewDirectory(QList<QUrl> urls, bool activate, ViewNode *older){
    QList<QNetworkRequest> reqs;
    foreach(QUrl url, urls) reqs << QNetworkRequest(url);
    return OpenInNewDirectory(reqs, activate, older);
}

////////////////////////////////////////////////////////////////

SharedView TreeBank::OpenInNewHistNode(QNetworkRequest req, bool activate, HistNode *parent){
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    m_TraverseMode = HistMode;
    ViewNode *partner = m_CurrentViewNode;
    if(!parent) parent = m_CurrentHistNode;
    if(parent && !partner)
        partner = parent->GetPartner()->ToViewNode();
    // failed to open link(bad TreeBank status).
    if(!parent || !partner){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return 0;
    }
    HistNode *hist = parent->MakeChild();
    // it's needless, because 'HistNode::MakeChild' has that.
    //hist->SetZoom(parent->GetZoom());
    SharedView view = LoadWithNoLink(req, hist, partner);
    if(activate){
        SetCurrent(hist);
    } else {
        view->hide();

        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenInNewHistNode(QUrl url, bool activate, HistNode *parent){
    return OpenInNewHistNode(QNetworkRequest(url), activate, parent);
}

SharedView TreeBank::OpenInNewHistNode(QList<QNetworkRequest> reqs, bool activate, HistNode *parent){
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = 0; i < reqs.length(); i++){
        bool last = (i == (reqs.length() - 1));
        SharedView view = OpenInNewHistNode(reqs[i], last && activate, parent);
        // failed to open link(bad TreeBank status).
        if(!view){
            View::SetSwitchingState(false);
            return 0;
        }
        if(last) v = view;
    }

    View::SetSwitchingState(false);

    return v;
}

SharedView TreeBank::OpenInNewHistNode(QList<QUrl> urls, bool activate, HistNode *parent){
    QList<QNetworkRequest> reqs;
    foreach(QUrl url, urls) reqs << QNetworkRequest(url);
    return OpenInNewHistNode(reqs, activate, parent);
}

SharedView TreeBank::OpenInNewHistNodeBackward(QNetworkRequest req, bool activate, HistNode *child){
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    View::SetSwitchingState(true);
    m_TraverseMode = HistMode;
    if(!child) child = m_CurrentHistNode;
    // failed to open link(bad TreeBank status).
    if(!child){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return 0;
    }
    HistNode *hist = child->MakeParent();
    // it's needless, because 'HistNode::MakeChild' has that.
    //hist->SetZoom(parent->GetZoom());
    SharedView view = LoadWithNoLink(req, hist, m_CurrentViewNode);
    if(activate){
        SetCurrent(hist);
    } else {
        view->hide();

        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenInNewHistNodeBackward(QUrl url, bool activate, HistNode *parent){
    return OpenInNewHistNodeBackward(QNetworkRequest(url), activate, parent);
}

////////////////////////////////////////////////////////////////

// manual loading should add to first of 'm_AllViews',
// and auto loading should add to last of 'm_AllViews',
// because want to sort with order of recentness.

static SharedView LoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = TreeBank::CreateView(req, hn, vn);
    SetPartner(u, hn, vn, view);
    SetPartner(u, vn, hn, view);
    TreeBank::PrependToAllViews(view);
    return view;
}

static SharedView LoadWithLink(HistNode *hn){
    if(hn->GetPartner())
        return LoadWithLink(QNetworkRequest(hn->GetUrl()),
                            hn, hn->GetPartner()->ToViewNode());
    return 0;
}

static SharedView LoadWithLink(ViewNode *vn){
    if(vn->GetPartner())
        return LoadWithLink(QNetworkRequest(vn->GetUrl()),
                            vn->GetPartner()->ToHistNode(), vn);
    return 0;
}

static SharedView LoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = TreeBank::CreateView(req, hn, vn);
    SetPartner(u, hn, vn, view);
    TreeBank::PrependToAllViews(view);
    return view;
}

static SharedView LoadWithNoLink(HistNode *hn){
    if(hn->GetPartner())
        return LoadWithNoLink(QNetworkRequest(hn->GetUrl()),
                              hn, hn->GetPartner()->ToViewNode());
    return 0;
}

static SharedView LoadWithNoLink(ViewNode *vn){
    if(vn->GetPartner())
        return LoadWithNoLink(QNetworkRequest(vn->GetUrl()),
                              vn->GetPartner()->ToHistNode(), vn);
    return 0;
}

static SharedView AutoLoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = SharedView();
    // 'QStringList::indexOf' uses 'QRegExp::exactMatch'.
    if(GetNodeSettings(vn).indexOf(QRegExp(QStringLiteral("[nN](?:o)?[lL](?:oad)?"))) == -1)
        view = TreeBank::CreateView(req, hn, vn);
    SetPartner(u, hn, vn, view);
    SetPartner(u, vn, hn, view);
    if(view){
        TreeBank::AppendToAllViews(view);
        view->RestoreZoom();
        view->hide();
    }
    return view;
}

static SharedView AutoLoadWithLink(HistNode *hn){
    if(hn->GetPartner())
        return AutoLoadWithLink(QNetworkRequest(hn->GetUrl()),
                                hn, hn->GetPartner()->ToViewNode());
    return 0;
}

static SharedView AutoLoadWithLink(ViewNode *vn){
    if(vn->GetPartner())
        return AutoLoadWithLink(QNetworkRequest(vn->GetUrl()),
                                vn->GetPartner()->ToHistNode(), vn);
    return 0;
}

static SharedView AutoLoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = SharedView();
    // 'QStringList::indexOf' uses 'QRegExp::exactMatch'.
    if(GetNodeSettings(vn).indexOf(QRegExp(QStringLiteral("[nN](?:o)?[lL](?:oad)?"))) == -1)
        view = TreeBank::CreateView(req, hn, vn);
    SetPartner(u, hn, vn, view);
    if(view){
        TreeBank::AppendToAllViews(view);
        view->RestoreZoom();
        view->hide();
    }
    return view;
}

static SharedView AutoLoadWithNoLink(HistNode *hn){
    if(hn->GetPartner())
        return AutoLoadWithNoLink(QNetworkRequest(hn->GetUrl()),
                                  hn, hn->GetPartner()->ToViewNode());
    return 0;
}

static SharedView AutoLoadWithNoLink(ViewNode *vn){
    if(vn->GetPartner())
        return AutoLoadWithNoLink(QNetworkRequest(vn->GetUrl()),
                                  vn->GetPartner()->ToHistNode(), vn);
    return 0;
}

////////////////////////////////////////////////////////////////

static void LinkNode(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SetPartner(u, vn, hn);
    SetPartner(u, hn, vn);
}

static void LinkNode(HistNode *hn){
    if(hn->GetPartner())
        return LinkNode(QNetworkRequest(hn->GetUrl()),
                        hn, hn->GetPartner()->ToViewNode());
}

static void SetHistProp(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    SetPartner(req.url(), hn, vn);
}

static void SetHistProp(HistNode *hn){
    if(hn->GetPartner())
        return SetHistProp(QNetworkRequest(hn->GetUrl()),
                           hn, hn->GetPartner()->ToViewNode());
}

static void SetPartner(QUrl, ViewNode *vn, HistNode *hn, SharedView view){
    vn->SetPartner(hn);
    vn->SetTitle(hn->GetTitle());
    vn->SetView(view.get());
    if(view) view->SetViewNode(vn);
}

static void SetPartner(QUrl url, HistNode *hn, ViewNode *vn, SharedView view){
    hn->SetPartner(vn);
    hn->SetUrl(url);
    hn->SetView(view.get());
    if(view) view->SetHistNode(hn);
}

QMenu *TreeBank::NodeMenu(){
    QMenu *menu = new QMenu(tr("Node"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Te_DisplayViewTree));
    menu->addAction(Action(Te_DisplayHistTree));
    menu->addAction(Action(Te_DisplayTrashTree));
    menu->addSeparator();
    menu->addAction(Action(Te_Close));
    menu->addAction(Action(Te_Restore));
    menu->addAction(Action(Te_NextView));
    menu->addAction(Action(Te_PrevView));
    menu->addAction(Action(Te_DigView));
    menu->addAction(Action(Te_BuryView));
    menu->addAction(Action(Te_NewViewNode));
    menu->addAction(Action(Te_NewHistNode));
    menu->addAction(Action(Te_CloneViewNode));
    menu->addAction(Action(Te_CloneHistNode));

    return menu;
}

QMenu *TreeBank::WindowMenu(){
    QMenu *menu = new QMenu(tr("Window"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Te_ToggleFullScreen));
    menu->addAction(Action(Te_ToggleMaximized));
    menu->addAction(Action(Te_ToggleMinimized));
    if(Application::EnableFramelessWindow())
        menu->addAction(Action(Te_ToggleShaded));
    menu->addAction(Action(Te_NewWindow));
    menu->addAction(Action(Te_CloseWindow));
    menu->addAction(Action(Te_SwitchWindow));
    menu->addAction(Action(Te_NextWindow));
    menu->addAction(Action(Te_PrevWindow));

    return menu;
}

QMenu *TreeBank::PageMenu(){
    QMenu *menu = new QMenu(tr("Page"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Te_Copy));
    menu->addAction(Action(Te_Cut));
    menu->addAction(Action(Te_Paste));
    menu->addAction(Action(Te_Undo));
    menu->addAction(Action(Te_Redo));
    menu->addAction(Action(Te_SelectAll));
    menu->addAction(Action(Te_Reload));
    menu->addAction(Action(Te_Stop));
    menu->addSeparator();
    menu->addAction(Action(Te_ZoomIn));
    menu->addAction(Action(Te_ZoomOut));
    menu->addSeparator();
    menu->addAction(Action(Te_Load));
    menu->addAction(Action(Te_Save));

    return menu;
}

QMenu *TreeBank::ApplicationMenu(bool expanded){
    QMenu *menu = new QMenu(tr("Application"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Te_Import));
    menu->addAction(Action(Te_Export));
    menu->addAction(Action(Te_AboutVanilla));
    menu->addAction(Action(Te_AboutQt));

    if(expanded){
        menu->addSeparator();
        menu->addAction(Action(Te_OpenTextSeeker));
        menu->addAction(Action(Te_OpenQueryEditor));
        menu->addAction(Action(Te_OpenUrlEditor));
        menu->addAction(Action(Te_OpenCommand));
        menu->addSeparator();
        menu->addAction(Action(Te_ToggleMenuBar));
        menu->addSeparator();
        menu->addAction(Action(Te_Quit));
    }

    return menu;
}

QMenu *TreeBank::CreateGlobalContextMenu(){
    QMenu *menu = new QMenu(this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(Te_Load));
    menu->addSeparator();
    menu->addAction(Action(Te_Close));
    menu->addAction(Action(Te_Restore));
    menu->addSeparator();
    menu->addAction(Action(Te_NextView));
    menu->addAction(Action(Te_PrevView));
    menu->addAction(Action(Te_NewWindow));
    menu->addAction(Action(Te_CloseWindow));
    menu->addAction(Action(Te_SwitchWindow));
    menu->addSeparator();
    menu->addAction(Action(Te_DisplayHistTree));
    menu->addAction(Action(Te_DisplayViewTree));
    menu->addAction(Action(Te_DisplayTrashTree));
    menu->addAction(Action(Te_DisplayAccessKey));
    menu->addSeparator();
    menu->addAction(Action(Te_Import));
    menu->addAction(Action(Te_Export));
    menu->addAction(Action(Te_AboutVanilla));
    menu->addAction(Action(Te_AboutQt));
    menu->addSeparator();
    menu->addAction(Action(Te_OpenQueryEditor));
    menu->addAction(Action(Te_OpenUrlEditor));
    menu->addAction(Action(Te_OpenTextSeeker));
    menu->addAction(Action(Te_OpenCommand));
    menu->addSeparator();
    menu->addAction(Action(Te_ToggleMenuBar));
    menu->addSeparator();
    menu->addAction(Action(Te_Quit));

    return menu;
}

QMenu *TreeBank::CreateTitlebarMenu(){
    QMenu *menu = new QMenu(this);
    menu->setToolTipsVisible(true);

    menu->addMenu(NodeMenu());
    menu->addMenu(WindowMenu());
    menu->addMenu(PageMenu());
    menu->addMenu(ApplicationMenu(false));
    menu->addSeparator();
    menu->addAction(Action(Te_OpenTextSeeker));
    menu->addAction(Action(Te_OpenQueryEditor));
    menu->addAction(Action(Te_OpenUrlEditor));
    menu->addAction(Action(Te_OpenCommand));
    menu->addSeparator();
    menu->addAction(Action(Te_ToggleMenuBar));
    menu->addSeparator();
    menu->addAction(Action(Te_Quit));

    return menu;
}

void TreeBank::PurgeChildWidgetsIfNeed(){
    // if m_PurgeView is true, Notifier and Receiver should be purged.
    if(m_Notifier && !m_PurgeNotifier && !m_PurgeView) m_Notifier->Purge();
    if(m_Receiver && !m_PurgeReceiver && !m_PurgeView) m_Receiver->Purge();
}

void TreeBank::JoinChildWidgetsIfNeed(){
    // if m_PurgeView is true, Notifier and Receiver should be purged.
    if(m_Notifier && !m_PurgeNotifier && !m_PurgeView) m_Notifier->Join();
    if(m_Receiver && !m_PurgeReceiver && !m_PurgeView) m_Receiver->Join();
}

void TreeBank::resizeEvent(QResizeEvent *ev){
    setSceneRect(0.0, 0.0,
                 ev->size().width(),
                 ev->size().height());
    if(m_CurrentView){
        m_CurrentView->resize(ev->size());
    }
    GetMainWindow()->AdjustAllEdgeWidgets();
    if(m_Notifier) m_Notifier->ResizeNotify(ev->size());
    if(m_Receiver) m_Receiver->ResizeNotify(ev->size());
    m_Gadgets->ResizeNotify(ev->size());
    ev->setAccepted(true);
}

void TreeBank::timerEvent(QTimerEvent *ev){
    Q_UNUSED(ev);
    /* do nothing. */
}

void TreeBank::wheelEvent(QWheelEvent *ev){
    QWheelEvent *new_ev = new QWheelEvent(ev->pos(),
                                          ev->delta()*Application::WheelScrollRate(),
                                          ev->buttons(),
                                          ev->modifiers(),
                                          ev->orientation());
    QGraphicsView::wheelEvent(new_ev);
    ev->setAccepted(true);
    delete new_ev;
}

void TreeBank::mouseMoveEvent(QMouseEvent *ev){
    QGraphicsView::mouseMoveEvent(ev);
}

void TreeBank::mousePressEvent(QMouseEvent *ev){
    QGraphicsView::mousePressEvent(ev);
}

void TreeBank::dragEnterEvent(QDragEnterEvent *ev){
    QGraphicsView::dragEnterEvent(ev);
    if(ev->isAccepted()) return;

    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    ev->setAccepted(true);
}

void TreeBank::dragMoveEvent(QDragMoveEvent *ev){
    QGraphicsView::dragMoveEvent(ev);
    if(ev->isAccepted()) return;
    ev->setAccepted(true);
}

void TreeBank::dragLeaveEvent(QDragLeaveEvent *ev){
    QGraphicsView::dragLeaveEvent(ev);
    if(ev->isAccepted()) return;
    ev->setAccepted(true);
}

void TreeBank::dropEvent(QDropEvent *ev){
    QGraphicsView::dropEvent(ev);

    // always accepted?
    //if(ev->isAccepted()) return;
    if(m_CurrentView) return;

    if(!ev->mimeData()->urls().isEmpty()){
        if(OpenOnSuitableNode(ev->mimeData()->urls(), true))
            ev->setAccepted(true);
    }
}

void TreeBank::mouseReleaseEvent(QMouseEvent *ev){
    QGraphicsView::mouseReleaseEvent(ev);

    // always accepted?
    //if(ev->isAccepted()) return;

    // not decidable from 'ev'.
    //if(ev->button() == Qt::RightButton){
    //    QMenu *menu = CreateGlobalContextMenu();
    //    menu->exec(ev->globalPos());
    //    delete menu;
    //    ev->setAccepted(true);
    //}

    /*

      that is difficult, to create context menu,
      when mouse button is released.

      because if right click is activated twice,
      mouse event occured in following order.

      previous mouse release event won't be finished,
      when second mouse press event.

      TreeBank mousePressEvent start
      GraphicsWebView mousePressEvent start
      GraphicsWebView mousePressEvent finish
      TreeBank mousePressEvent finish

      TreeBank contextMenuEvent start
      GraphicsWebView contextMenuEvent start
      GraphicsWebView contextMenuEvent finish
      TreeBank contextMenuEvent finish

      TreeBank mouseReleaseEvent start
      GraphicsWebView mouseReleaseEvent start

      TreeBank mousePressEvent start
      GraphicsWebView mousePressEvent start
      GraphicsWebView mousePressEvent finish
      TreeBank mousePressEvent finish

      TreeBank contextMenuEvent start
      GraphicsWebView contextMenuEvent start
      GraphicsWebView contextMenuEvent finish
      TreeBank contextMenuEvent finish

      GraphicsWebView mouseReleaseEvent finish
      TreeBank mouseReleaseEvent finish

      TreeBank mouseReleaseEvent start    /- GraphicsWebView is ignored. press event count and
      TreeBank mouseReleaseEvent finish   \- release event count is not always one-to-one.

     */
}

void TreeBank::contextMenuEvent(QContextMenuEvent *ev){
    /* when mouse pressed, do nothing. */
    QGraphicsView::contextMenuEvent(ev);
    if(ev->isAccepted()) return;

    QMenu *menu = CreateGlobalContextMenu();
    menu->exec(ev->globalPos());
    delete menu;
    ev->setAccepted(true);
}

void TreeBank::keyPressEvent(QKeyEvent *ev){
    QGraphicsView::keyPressEvent(ev);
    if(ev->isAccepted()) return;

    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return;

    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){

        TriggerKeyEvent(ev);
        ev->setAccepted(true);
        return;
    }
    if(!Application::IsOnlyModifier(ev) &&
       Application::HasNoModifier(ev)){

        TriggerKeyEvent(ev);
        ev->setAccepted(true);
    }
}

void TreeBank::OpenInNewIfNeed(QUrl url){
    if(!m_CurrentView) OpenOnSuitableNode(url, true);
}

void TreeBank::OpenInNewIfNeed(QList<QUrl> urls){
    if(!m_CurrentView) OpenOnSuitableNode(urls, true);
}

void TreeBank::OpenInNewIfNeed(QString query){
    if(!m_CurrentView) OpenOnSuitableNode(Page::CreateQueryUrl(query), true);
}

void TreeBank::OpenInNewIfNeed(QString query, QString key){
    if(!m_CurrentView) OpenOnSuitableNode(Page::CreateQueryUrl(query, key), true);
}

void TreeBank::Repaint(){
    if(m_CurrentView) m_CurrentView->repaint();
}

void TreeBank::Reconfigure(){
    Application::Reconfigure();
}

void TreeBank::Up(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_Up);
}

void TreeBank::Down(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_Down);
}

void TreeBank::Right(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_Right);
}

void TreeBank::Left(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_Left);
}

void TreeBank::PageUp(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_PageUp);
}

void TreeBank::PageDown(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_PageDown);
}

void TreeBank::Home(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_Home);
}

void TreeBank::End(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::Ke_End);
}

void TreeBank::Import(){
    Application::Import(this);
}

void TreeBank::Export(){
    Application::Export(this);
}

void TreeBank::AboutVanilla(){
    Application::AboutVanilla(this);
}

void TreeBank::AboutQt(){
    Application::AboutQt(this);
}

void TreeBank::Quit(){
    Application::Quit();
}

void TreeBank::ToggleNotifier(){
    if(m_Notifier){
        m_Notifier->deleteLater();
        m_Notifier = 0;
    } else {
        bool purge = m_PurgeNotifier || m_PurgeView || m_CurrentView->ForbidToOverlap();
        // View::Connect refer to TreeBank::m_Notifier

        if(m_Gadgets && m_Gadgets->IsActive())
            m_Gadgets->Disconnect(this);
        else if(m_CurrentView)
            m_CurrentView->Disconnect(this);

        m_Notifier = new Notifier(this, purge);

        if(m_Gadgets && m_Gadgets->IsActive())
            m_Gadgets->Connect(this);
        else if(m_CurrentView)
            m_CurrentView->Connect(this);

        ConnectToNotifier();

        if(m_Notifier->IsPurged())
            GetMainWindow()->SetFocus();
    }
}

void TreeBank::ToggleReceiver(){
    if(m_Receiver){
        m_Receiver->deleteLater();
        m_Receiver = 0;
    } else {
        bool purge = m_PurgeReceiver || m_PurgeView || m_CurrentView->ForbidToOverlap();
        // View::Connect refer to TreeBank::m_Receiver

        if(m_Gadgets && m_Gadgets->IsActive())
            m_Gadgets->Disconnect(this);
        else if(m_CurrentView)
            m_CurrentView->Disconnect(this);

        m_Receiver = new Receiver(this, purge);

        if(m_Gadgets && m_Gadgets->IsActive())
            m_Gadgets->Connect(this);
        else if(m_CurrentView)
            m_CurrentView->Connect(this);

        ConnectToReceiver();
    }
}

void TreeBank::ToggleMenuBar(){
    GetMainWindow()->ToggleMenuBar();
}

void TreeBank::ToggleFullScreen(){
    GetMainWindow()->ToggleFullScreen();
}

void TreeBank::ToggleMaximized(){
    GetMainWindow()->ToggleMaximized();
}

void TreeBank::ToggleMinimized(){
    GetMainWindow()->ToggleMinimized();
}

void TreeBank::ToggleShaded(){
    GetMainWindow()->ToggleShaded();
}

MainWindow *TreeBank::ShadeWindow(MainWindow *win){
    return Application::ShadeWindow(win ? win : GetMainWindow());
}

MainWindow *TreeBank::UnshadeWindow(MainWindow *win){
    return Application::UnshadeWindow(win ? win : GetMainWindow());
}

MainWindow *TreeBank::NewWindow(int id){
    return Application::NewWindow(id);
}

MainWindow *TreeBank::CloseWindow(MainWindow *win){
    return Application::CloseWindow(win ? win : GetMainWindow());
}

MainWindow *TreeBank::SwitchWindow(bool next){
    return Application::SwitchWindow(next);
}

MainWindow *TreeBank::NextWindow(){
    return Application::NextWindow();
}

MainWindow *TreeBank::PrevWindow(){
    return Application::PrevWindow();
}

void TreeBank::Back(HistNode *hist){
    m_TraverseMode = HistMode;
    if(!hist) hist = m_CurrentHistNode;
    if(!hist) return;

    if(hist->IsRoot() && hist->HasNoChildren() &&
       hist->GetView() && hist->GetView()->CanGoBack()){

        hist->GetView()->SetScroll(QPointF());
        hist->GetView()->TriggerNativeGoBackAction();

    } else if(Node *nd = hist->Prev()){

        SetCurrent(nd);

    } else if(hist->GetView()){

        hist->GetView()->GoBackToInferedUrl();
    }
}

void TreeBank::Forward(HistNode *hist){
    m_TraverseMode = HistMode;
    if(!hist) hist = m_CurrentHistNode;
    if(!hist) return;

    if(hist->IsRoot() && hist->HasNoChildren() &&
       hist->GetView() && hist->GetView()->CanGoForward()){

        hist->GetView()->SetScroll(QPointF());
        hist->GetView()->TriggerNativeGoForwardAction();

    } else if(Node *nd = hist->Next()){

        SetCurrent(nd);

    } else if(hist->GetView()){

        hist->GetView()->GoForwardToInferedUrl();
    }
}

void TreeBank::UpDirectory(HistNode *hist){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    m_TraverseMode = HistMode;
    if(!hist) hist = m_CurrentHistNode;
    if(!hist) return;

    QNetworkRequest req(Page::UpDirectoryUrl(hist->GetUrl()));
    req.setRawHeader("Referer", hist->GetUrl().toEncoded());

    // should refer node settings directly?
    if(!hist->GetView() ||
       hist->GetView()->EnableLoadHackLocal() ||
       View::EnableLoadHack()){

        OpenInNewHistNode(req, true, hist);

    } else {

        // hist->GetView() is always non empty.
        hist->GetView()->SetScroll(QPointF());
        hist->GetView()->Load(req);
    }
}

void TreeBank::Close(ViewNode *vn){
    m_TraverseMode = Neutral;
    if(!vn) vn = m_CurrentViewNode;
    if(vn) DeleteNode(vn);
}

void TreeBank::Restore(ViewNode *vn, ViewNode *dir){
    m_TraverseMode = Neutral;
    if(m_TrashRoot->HasNoChildren()) return;

    if(vn){
        if(GetRoot(vn) == m_ViewRoot){
            // on tableview(viewnode).
            ViewNode *rest = m_TrashRoot->TakeFirstChild()->ToViewNode();
            if(!dir) dir = m_ViewRoot;
            if(dir->ChildrenContains(vn))
                dir->InsertChild(dir->ChildrenIndexOf(vn) + 1, rest);
            else
                dir->AppendChild(rest);

            rest->SetParent(dir);

            ApplySpecificSettings(rest, dir);
        } else {
            // on tableview(trash).
            MoveNode(vn, m_ViewRoot);
        }
    } else {
        if(dir){
            MoveNode(m_TrashRoot->GetFirstChild(),
                     GetRoot(dir) == m_ViewRoot ? dir : m_ViewRoot);
        } else {
            // on view.
            ViewNode *older = m_CurrentViewNode;
            ViewNode *young = m_TrashRoot->TakeFirstChild()->ToViewNode();
            if(older){
                Node *parent = older->GetParent();
                young->SetParent(parent);
                parent->InsertChild(parent->ChildrenIndexOf(older) + 1, young);
            } else {
                young->SetParent(m_ViewRoot);
                m_ViewRoot->AppendChild(young);
            }

            ApplySpecificSettings(young);
            SetCurrent(young);
        }
    }
}

void TreeBank::Recreate(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;
    DislinkView(vn->GetPartner()->ToHistNode());
    SetCurrent(vn);
}

void TreeBank::NextView(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    Node *next = vn;
    if(m_TraverseAllView){
        do next = next->Next();
        // skip non openable node.
        while(next &&
              // directory
              ((!next->GetView() && next->GetUrl().isEmpty()) ||
               // already opened node on other window
               ( next->GetView() && next->GetView()->GetTreeBank() &&
                 next->GetView()->GetTreeBank()->GetCurrentViewNode() == next)));

        if(next && SetCurrent(next)){
            ;
        } else {
            next = m_ViewRoot;
            while(!next->HasNoChildren())
                next = next->GetFirstChild();

            // retry without do-while.
            // skip non openable node.
            while(next &&
                  // directory
                  ((!next->GetView() && next->GetUrl().isEmpty()) ||
                   // already opened node on other window
                   ( next->GetView() && next->GetView()->GetTreeBank() &&
                     next->GetView()->GetTreeBank()->GetCurrentViewNode() == next))){
                // if go back to square one, do nothing.
                if(next == vn) return;
                next = next->Next();
            }

            SetCurrent(next);
        }
    } else {
        NodeList sibling = next->GetSiblings();

        if(sibling.indexOf(next) < sibling.length() - 1){
            next = sibling[sibling.indexOf(next) + 1];
        } else {
            next = sibling.first();
        }

        while(next &&
              (next->GetView() && next->GetView()->GetTreeBank() &&
               next->GetView()->GetTreeBank()->GetCurrentViewNode() == next)){

            if(next == vn) return;

            if(sibling.indexOf(next) < sibling.length() - 1){
                next = sibling[sibling.indexOf(next) + 1];
            } else {
                next = sibling.first();
            }
        }

        SetCurrent(next);
    }
}

void TreeBank::PrevView(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    Node* prev = vn;
    if(m_TraverseAllView){
        do prev = prev->Prev();
        // skip non openable node.
        while(prev &&
              // directory
              ((!prev->GetView() && prev->GetUrl().isEmpty()) ||
               // already opened node on other window
               ( prev->GetView() && prev->GetView()->GetTreeBank() &&
                 prev->GetView()->GetTreeBank()->GetCurrentViewNode() == prev)));

        if(prev && SetCurrent(prev)){
            ;
        } else {
            prev = m_ViewRoot;
            while(!prev->HasNoChildren())
                prev = prev->GetLastChild();

            // retry without do-while.
            // skip non openable node.
            while(prev &&
                  // directory
                  ((!prev->GetView() && prev->GetUrl().isEmpty()) ||
                   // already opened node on other window
                   ( prev->GetView() && prev->GetView()->GetTreeBank() &&
                     prev->GetView()->GetTreeBank()->GetCurrentViewNode() == prev))){
                // if go back to square one, do nothing.
                if(prev == vn) return;
                prev = prev->Prev();
            }

            SetCurrent(prev);
        }
    } else {
        NodeList sibling = prev->GetSiblings();

        if(sibling.indexOf(prev) > 0){
            prev = sibling[sibling.indexOf(prev) - 1];
        } else {
            prev = sibling.last();
        }

        while(prev &&
              (prev->GetView() && prev->GetView()->GetTreeBank() &&
               prev->GetView()->GetTreeBank()->GetCurrentViewNode() == prev)){

            if(prev == vn) return;

            if(sibling.indexOf(prev) > 0){
                prev = sibling[sibling.indexOf(prev) - 1];
            } else {
                prev = sibling.last();
            }
        }

        SetCurrent(prev);
    }
}

void TreeBank::BuryView(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    RaiseDisplayedViewPriority();

    if(View *v = vn->GetView()){
        if(SharedView view = v->GetThis().lock()){
            view->lower();
            m_AllViews.removeOne(view);
            AppendToAllViews(view);
        }
    }

    foreach(SharedView view, m_AllViews){
        TreeBank *tb = view->GetTreeBank();

        if(!tb || tb->GetCurrentView() != view || tb == this){
            SetCurrent(view);
            return;
        }
    }
}

void TreeBank::DigView(ViewNode *vn){
    Q_UNUSED(vn);
    m_TraverseMode = ViewMode;

    RaiseDisplayedViewPriority();

    for(int i = m_AllViews.length() - 1; i >= 0; i--){
        SharedView view = m_AllViews[i];
        TreeBank *tb = view->GetTreeBank();

        if(!tb || tb->GetCurrentView() != view || tb == this){
            SetCurrent(view);
            return;
        }
    }
}

ViewNode *TreeBank::NewViewNode(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return 0;

    if(m_Gadgets && m_Gadgets->IsActive()) return vn->New();

    ViewNode *newNode = vn->New();
    if(!newNode) return 0;
    SharedView view = LoadWithLink(newNode);
    if(Page::Activate()){
        SetCurrent(newNode);
    } else {
        view->hide();
        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }
    return newNode;
}

HistNode *TreeBank::NewHistNode(HistNode *hn){
    m_TraverseMode = HistMode;
    if(!hn) hn = m_CurrentHistNode;
    if(!hn) return 0;

    if(m_Gadgets && m_Gadgets->IsActive()) return hn->New();

    HistNode *newNode = hn->New();
    if(!newNode) return 0;
    SharedView view = LoadWithLink(newNode);
    if(Page::Activate()){
        SetCurrent(newNode);
    } else {
        view->hide();
        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }
    return newNode;
}

ViewNode *TreeBank::CloneViewNode(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return 0;

    if(m_Gadgets && m_Gadgets->IsActive()) return vn->Clone();

    ViewNode *clone = vn->Clone();
    if(!clone) return 0;
    SharedView view = LoadWithLink(clone);
    if(Page::Activate()){
        SetCurrent(clone);
    } else {
        view->hide();
        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }
    return clone;
}

HistNode *TreeBank::CloneHistNode(HistNode *hn){
    m_TraverseMode = HistMode;
    if(!hn) hn = m_CurrentHistNode;
    if(!hn) return 0;

    if(m_Gadgets && m_Gadgets->IsActive()) return hn->Clone();

    HistNode *clone = hn->Clone();
    if(!clone) return 0;
    SharedView view = LoadWithLink(clone);
    if(Page::Activate()){
        SetCurrent(clone);
    } else {
        view->hide();
        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }
    return clone;
}

ViewNode *TreeBank::MakeLocalNode(ViewNode *older){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return 0;

    m_TraverseMode = ViewMode;
    if(!older) older = m_CurrentViewNode;
    if(!older) return 0;

    QFileDialog::Options options =
        QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;

    QString path =
        ModalDialog::GetExistingDirectory(QString::null, QStringLiteral("."), options);

    if(!path.isEmpty()){
        HistNode *hist  = m_HistRoot->MakeChild();
        ViewNode *young = older->NewDir();
        ViewNode *parent = young->GetParent()->ToViewNode();

        QUrl url = QUrl::fromLocalFile(path);
        QString title = QStringLiteral("filer;localview");
        parent->SetTitle(title);

        hist->SetUrl(url);
        hist->SetPartner(young);
        young->SetPartner(hist);

        LoadWithLink(hist);
        SetCurrent(hist);
        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
        return young;
    }
    return 0;
}

ViewNode *TreeBank::MakeChildDirectory(ViewNode *vn){
    // make empty directory.
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_ViewRoot;
    if(vn) return vn->MakeChild();
    return 0;
}

ViewNode *TreeBank::MakeSiblingDirectory(ViewNode *vn){
    // make empty directory.
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_ViewRoot;
    if(vn) return vn->MakeSibling();
    return 0;
}

// gadgets.
////////////////////////////////////////////////////////////////

void TreeBank::DisplayViewTree(ViewNode *vn){
    m_Gadgets->Activate(Gadgets::ViewTree);
    if(!vn) vn = m_CurrentViewNode;
    if(!vn && !m_ViewRoot->HasNoChildren())
        vn = m_ViewRoot->GetFirstChild()->ToViewNode();
    m_Gadgets->SetCurrent(vn);
}

void TreeBank::DisplayHistTree(HistNode *hn){
    m_Gadgets->Activate(Gadgets::HistTree);
    if(!hn) hn = m_CurrentHistNode;
    if(!hn && !m_HistRoot->HasNoChildren())
        hn = m_HistRoot->GetFirstChild()->ToHistNode();
    m_Gadgets->SetCurrent(hn);
}

void TreeBank::DisplayTrashTree(ViewNode *vn){
    m_Gadgets->Activate(Gadgets::TrashTree);
    if(vn && IsTrash(vn))
        m_Gadgets->SetCurrent(vn);
    else if(!m_TrashRoot->HasNoChildren())
        m_Gadgets->SetCurrent(m_TrashRoot->GetFirstChild());
}

void TreeBank::DisplayAccessKey(SharedView view){
    m_Gadgets->Activate(Gadgets::AccessKey);
    if(!view) view = m_CurrentView;
    if(view) m_Gadgets->SetCurrentView(view.get());
}

void TreeBank::OpenTextSeeker(SharedView view){
    if(!view) view = m_CurrentView;
    if(m_Receiver) m_Receiver->OpenTextSeeker(view.get());
}

void TreeBank::OpenQueryEditor(SharedView view){
    if(!view) view = m_CurrentView;
    if(m_Receiver) m_Receiver->OpenQueryEditor(view.get());
}

void TreeBank::OpenUrlEditor(SharedView view){
    if(!view) view = m_CurrentView;
    if(m_Receiver) m_Receiver->OpenUrlEditor(view.get());
}

void TreeBank::OpenCommand(SharedView view){
    // only OpenCommand make Receiver (if need) when called.
    if(!view) view = m_CurrentView;
    if(!m_Receiver) ToggleReceiver();
    m_Receiver->OpenCommand(view.get());
}

void TreeBank::Load(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Load);
}

void TreeBank::Copy(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Copy);
}

void TreeBank::Cut(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Cut);
}

void TreeBank::Paste(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Paste);
}

void TreeBank::Undo(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Undo);
}

void TreeBank::Redo(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Redo);
}

void TreeBank::SelectAll(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_SelectAll);
}

void TreeBank::Unselect(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Unselect);
}

void TreeBank::Reload(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Reload);
}

void TreeBank::ReloadAndBypassCache(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_ReloadAndBypassCache);
}

void TreeBank::Stop(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Stop);
}

void TreeBank::StopAndUnselect(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_StopAndUnselect);
}

void TreeBank::Print(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Print);
}

void TreeBank::Save(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_Save);
}

void TreeBank::ZoomIn(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_ZoomIn);
}

void TreeBank::ZoomOut(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_ZoomOut);
}

void TreeBank::ViewSource(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_ViewSource);
}

void TreeBank::ApplySource(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_ApplySource);
}

void TreeBank::CopyUrl(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_CopyUrl);
}

void TreeBank::CopyTitle(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_CopyTitle);
}

void TreeBank::CopyPageAsLink(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_CopyPageAsLink);
}

void TreeBank::CopySelectedHtml(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_CopySelectedHtml);
}

void TreeBank::OpenWithIE(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithIE);
}

void TreeBank::OpenWithFF(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithFF);
}

void TreeBank::OpenWithOpera(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithOpera);
}

void TreeBank::OpenWithOPR(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithOPR);
}

void TreeBank::OpenWithSafari(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithSafari);
}

void TreeBank::OpenWithChrome(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithChrome);
}

void TreeBank::OpenWithSleipnir(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithSleipnir);
}

void TreeBank::OpenWithVivaldi(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithVivaldi);
}

void TreeBank::OpenWithCustom(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::We_OpenWithCustom);
}

bool TreeBank::TriggerAction(QString str){
    if(IsValidAction(str))
        TriggerAction(StringToAction(str));
    else return false;
    return true;
}

void TreeBank::TriggerAction(TreeBankAction a){
    Action(a)->trigger();
}

QAction *TreeBank::Action(TreeBankAction a){
    // forbid many times call of same action.
    static const QList<TreeBankAction> exclude = QList<TreeBankAction>()
        << Te_NoAction << Ke_End      << Te_Undo
        << Ke_Up       << Ke_PageUp   << Te_Redo
        << Ke_Down     << Ke_PageDown << Te_SelectAll
        << Ke_Right    << Te_Cut      << Te_SwitchWindow
        << Ke_Left     << Te_Copy     << Te_NextWindow
        << Ke_Home     << Te_Paste    << Te_PrevWindow;
    static TreeBankAction previousAction = Te_NoAction;
    static int sameActionCount = 0;
    if(exclude.contains(a)){
        sameActionCount = 0;
        previousAction = Te_NoAction;
    } else if(a == previousAction){
        if(++sameActionCount > MAX_SAME_ACTION_COUNT)
            a = Te_NoAction;
    } else {
        sameActionCount = 0;
        previousAction = a;
    }

    QAction *action = m_ActionTable[a];
    if(action) return action;

    m_ActionTable[a] = action = new QAction(this);

    switch(a){
    case Ke_Up:      action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case Ke_Down:    action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case Ke_Right:   action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case Ke_Left:    action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
    case Te_Back:    action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowBack));     break;
    case Te_Forward: action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowForward));  break;
    case Te_Reload:  action->setIcon(Application::style()->standardIcon(QStyle::SP_BrowserReload)); break;
    case Te_Stop:    action->setIcon(Application::style()->standardIcon(QStyle::SP_BrowserStop));   break;
    }

    switch(a){
    case Te_NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case Ke_##name:                                                 \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name##Key()));                         \
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
        case Te_##name:                                                 \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name()));                              \
            break;

        // application events.
        DEFINE_ACTION(Import,       tr("Import"));
        DEFINE_ACTION(Export,       tr("Export"));
        DEFINE_ACTION(AboutVanilla, tr("AboutVanilla"));
        DEFINE_ACTION(AboutQt,      tr("AboutQt"));
        DEFINE_ACTION(Quit,         tr("Quit"));

        // window events.
        DEFINE_ACTION(ToggleNotifier,   tr("ToggleNotifier"));
        DEFINE_ACTION(ToggleReceiver,   tr("ToggleReceiver"));
        DEFINE_ACTION(ToggleMenuBar,    tr("ToggleMenuBar"));
        DEFINE_ACTION(ToggleFullScreen, tr("ToggleFullScreen"));
        DEFINE_ACTION(ToggleMaximized,  tr("ToggleMaximized"));
        DEFINE_ACTION(ToggleMinimized,  tr("ToggleMinimized"));
        DEFINE_ACTION(ToggleShaded,     tr("ToggleShaded"));
        DEFINE_ACTION(ShadeWindow,      tr("ShadeWindow"));
        DEFINE_ACTION(UnshadeWindow,    tr("UnshadeWindow"));
        DEFINE_ACTION(NewWindow,        tr("NewWindow"));
        DEFINE_ACTION(CloseWindow,      tr("CloseWindow"));
        DEFINE_ACTION(SwitchWindow,     tr("SwitchWindow"));
        DEFINE_ACTION(NextWindow,       tr("NextWindow"));
        DEFINE_ACTION(PrevWindow,       tr("PrevWindow"));

        // treebank events.
        DEFINE_ACTION(Back,             tr("Back"));
        DEFINE_ACTION(Forward,          tr("Forward"));
        DEFINE_ACTION(UpDirectory,      tr("UpDirectory"));
        DEFINE_ACTION(Close,            tr("Close"));
        DEFINE_ACTION(Restore,          tr("Restore"));
        DEFINE_ACTION(Recreate,         tr("Recreate"));
        DEFINE_ACTION(NextView,         tr("NextView"));
        DEFINE_ACTION(PrevView,         tr("PrevView"));
        DEFINE_ACTION(BuryView,         tr("BuryView"));
        DEFINE_ACTION(DigView,          tr("DigView"));
        DEFINE_ACTION(NewViewNode,      tr("NewViewNode"));
        DEFINE_ACTION(NewHistNode,      tr("NewHistNode"));
        DEFINE_ACTION(CloneViewNode,    tr("CloneViewNode"));
        DEFINE_ACTION(CloneHistNode,    tr("CloneHistNode"));

        DEFINE_ACTION(DisplayAccessKey, tr("DisplayAccessKey"));
        DEFINE_ACTION(DisplayViewTree,  tr("DisplayViewTree"));
        DEFINE_ACTION(DisplayHistTree,  tr("DisplayHistTree"));
        DEFINE_ACTION(DisplayTrashTree, tr("DisplayTrashTree"));

        DEFINE_ACTION(OpenTextSeeker,   tr("OpenTextSeeker"));
        DEFINE_ACTION(OpenQueryEditor,  tr("OpenQueryEditor"));
        DEFINE_ACTION(OpenUrlEditor,    tr("OpenUrlEditor"));
        DEFINE_ACTION(OpenCommand,      tr("OpenCommand"));
        DEFINE_ACTION(Load,             tr("Load"));

        // web events.
        DEFINE_ACTION(Copy,                 tr("Copy"));
        DEFINE_ACTION(Cut,                  tr("Cut"));
        DEFINE_ACTION(Paste,                tr("Paste"));
        DEFINE_ACTION(Undo,                 tr("Undo"));
        DEFINE_ACTION(Redo,                 tr("Redo"));
        DEFINE_ACTION(SelectAll,            tr("SelectAll"));
        DEFINE_ACTION(Unselect,             tr("Unselect"));
        DEFINE_ACTION(Reload,               tr("Reload"));
        DEFINE_ACTION(ReloadAndBypassCache, tr("ReloadAndBypassCache"));
        DEFINE_ACTION(Stop,                 tr("Stop"));
        DEFINE_ACTION(StopAndUnselect,      tr("StopAndUnselect"));

        DEFINE_ACTION(Print,                tr("Print"));
        DEFINE_ACTION(Save,                 tr("Save"));
        DEFINE_ACTION(ZoomIn,               tr("ZoomIn"));
        DEFINE_ACTION(ZoomOut,              tr("ZoomOut"));
        DEFINE_ACTION(ViewSource,           tr("ViewSource"));
        DEFINE_ACTION(ApplySource,          tr("ApplySource"));

        DEFINE_ACTION(CopyUrl,              tr("CopyUrl"));
        DEFINE_ACTION(CopyTitle,            tr("CopyTitle"));
        DEFINE_ACTION(CopyPageAsLink,       tr("CopyPageAsLink"));
        DEFINE_ACTION(CopySelectedHtml,     tr("CopySelectedHtml"));
        DEFINE_ACTION(OpenWithIE,           tr("OpenWithIE"));
        DEFINE_ACTION(OpenWithFF,           tr("OpenWithFF"));
        DEFINE_ACTION(OpenWithOpera,        tr("OpenWithOpera"));
        DEFINE_ACTION(OpenWithOPR,          tr("OpenWithOPR"));
        DEFINE_ACTION(OpenWithSafari,       tr("OpenWithSafari"));
        DEFINE_ACTION(OpenWithChrome,       tr("OpenWithChrome"));
        DEFINE_ACTION(OpenWithSleipnir,     tr("OpenWithSleipnir"));
        DEFINE_ACTION(OpenWithVivaldi,      tr("OpenWithVivaldi"));
        DEFINE_ACTION(OpenWithCustom,       tr("OpenWithCustom"));

#undef  DEFINE_ACTION
    }
    switch(a){
    case Te_OpenWithIE:
        action->setIcon(Application::BrowserIcon_IE());
        break;
    case Te_OpenWithFF:
        action->setIcon(Application::BrowserIcon_FF());
        break;
    case Te_OpenWithOpera:
        action->setIcon(Application::BrowserIcon_Opera());
        break;
    case Te_OpenWithOPR:
        action->setIcon(Application::BrowserIcon_OPR());
        break;
    case Te_OpenWithSafari:
        action->setIcon(Application::BrowserIcon_Safari());
        break;
    case Te_OpenWithChrome:
        action->setIcon(Application::BrowserIcon_Chrome());
        break;
    case Te_OpenWithSleipnir:
        action->setIcon(Application::BrowserIcon_Sleipnir());
        break;
    case Te_OpenWithVivaldi:
        action->setIcon(Application::BrowserIcon_Vivaldi());
        break;
    case Te_OpenWithCustom:
        action->setIcon(Application::BrowserIcon_Custom());
        action->setText(tr("OpenWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
    }
    return action;
}

void TreeBank::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return;
    QString str = m_KeyMap[seq];
    if(str.isEmpty()) return;

    TriggerAction(str);
}

void TreeBank::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return;
    str = m_KeyMap[seq]; // sequence => action
    if(str.isEmpty()) return;

    TriggerAction(str);
}

void TreeBank::DeleteView(View *view){
    view->DeleteLater();
}

SharedView TreeBank::CreateView(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    MainWindow *win = Application::GetCurrentWindow();
    TreeBank *tb = win ? win->GetTreeBank() : 0;
    QStringList set = GetNodeSettings(vn);
    QString id = GetNetworkSpaceID(vn);

    // for predicate.
    QUrl url = req.url();
    QString urlstr = url.toString();

    SharedView view = SharedView();

    if(urlstr.startsWith(QStringLiteral("file:///")) &&
       (QFileInfo(url.toLocalFile()).isDir() ||
        LocalView::IsSupported(url))){

        view = SharedView(new LocalView(tb, id, set), &DeleteView);

    } else {
        view = SharedView(
            // 'QStringList::indexOf' uses 'QRegExp::exactMatch'.
#ifdef QTWEBKIT
            set.indexOf(QRegExp(QStringLiteral(""                 VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new WebView(tb, id, set) :
            set.indexOf(QRegExp(QStringLiteral("[gG](?:raphics)?" VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new GraphicsWebView(tb, id, set) :
            set.indexOf(QRegExp(QStringLiteral("[qQ](?:uick)?"    VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new QuickWebView(tb, id, set) :
#else
            set.indexOf(QRegExp(QStringLiteral(""                 VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new WebEngineView(tb, id, set) :
            set.indexOf(QRegExp(QStringLiteral("[gG](?:raphics)?" VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new WebEngineView(tb, id, set) :
            set.indexOf(QRegExp(QStringLiteral("[qQ](?:uick)?"    VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new QuickWebEngineView(tb, id, set) :
#endif
            set.indexOf(QRegExp(QStringLiteral(""                 VV"[wW](?:eb)?" VV"[eE](?:ngine)?" VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new WebEngineView(tb, id, set) :
            set.indexOf(QRegExp(QStringLiteral("[qQ](?:uick)?"    VV"[wW](?:eb)?" VV"[eE](?:ngine)?" VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new QuickWebEngineView(tb, id, set) :
            set.indexOf(QRegExp(QStringLiteral(""                 VV"[lL](?:ocal)?"                  VV"(?:[vV](?:iew)?)?"))) != -1 ?
              new LocalView(tb, id, set) :
#if defined(WEBENGINEVIEW_DEFAULT) || !defined(QTWEBKIT)
            static_cast<View*>(new WebEngineView(tb, id, set))
#else
            static_cast<View*>(new GraphicsWebView(tb, id, set))
#endif
            , &DeleteView);
    }

    if(hn->GetTitle().isEmpty()){
        QString title = req.url().toString();
        if(vn->GetPartner() == hn)
            vn->SetTitle(title);
        hn->SetTitle(title);
    }

    while(m_MaxViewCount && m_AllViews.length() > m_MaxViewCount){
        SharedView v = m_AllViews.takeLast();
        if(WinIndex(v) == 0){
            HistNode *hn = v->GetHistNode();
            ViewNode *vn = v->GetViewNode();

            if(hn->GetView() == vn->GetView()){
                vn->SetView(0);
            }
            hn->SetView(0);

            ReleaseView(v);

        } else {
            PrependToAllViews(v);
        }
    }

    view->SetThis(WeakView(view));
    view->SetViewNode(vn);
    view->SetHistNode(hn);
    if(!view->RestoreHistory()) view->Load(req);
    return view;
}
