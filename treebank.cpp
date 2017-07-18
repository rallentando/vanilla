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
#include <QGLWidget>
#include <QOpenGLWidget>
#include <QFile>
#include <QDir>
#include <QImageReader>
#include <QFileInfo>
#include <QStyle>

#include <QGraphicsRotation>
#include <QGraphicsScale>

#include "application.hpp"
#include "networkcontroller.hpp"
#include "saver.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "view.hpp"
#ifdef WEBKITVIEW
#  include "webkitview.hpp"
#  include "graphicswebkitview.hpp"
#  include "quickwebkitview.hpp"
#  include "webkitpage.hpp"
#endif
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "quicknativewebview.hpp"
#include "tridentview.hpp"
#include "webenginepage.hpp"
#include "gadgets.hpp"
#include "mainwindow.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "localview.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

/*

  Directory specific settings.

  "[iI][dD](?:entif(?:y|ier|ication))?" :
  use directory name as networkaccessmanager/profile ID.


  "[nN](?:o)?(?:[aA](?:uto)?)?[lL](?:oad)?" :
  disable auto loading.


  "^(?:[dD]efault)?(?:[tT]ext)?(?:[eE]ncod(?:e|ing)|[cC]odecs?) [^ ].*"
  :
  set encoding.


                     "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?"
  "[gG](?:raphics)?" "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?"
  "[qQ](?:uick)?"    "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?"
                     "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?"
  "[gG](?:raphics)?" "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?"
  "[qQ](?:uick)?"    "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?"
                     "[wW](?:eb)?" "[kK](?:it)?"    "(?:[vV](?:iew)?)?"
  "[gG](?:raphics)?" "[wW](?:eb)?" "[kK](?:it)?"    "(?:[vV](?:iew)?)?"
  "[qQ](?:uick)?"    "[wW](?:eb)?" "[kK](?:it)?"    "(?:[vV](?:iew)?)?"

                     "[nN](?:ative)?" "[wW](?:ev)?" "(?:[vV](?:iew)?)?"
  "[qQ](?:uick)?"    "[nN](?:ative)?" "[wW](?:ev)?" "(?:[vV](?:iew)?)?"
  "[lL](?:ocal)?"                                   "(?:[vV](?:iew)?)?"
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
  "[pP]lugins?"
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

TreeBank::TraverseMode TreeBank::m_TraverseMode = Neutral;
TreeBank::Viewport     TreeBank::m_Viewport = Widget;
SharedViewList TreeBank::m_AllViews = SharedViewList();
SharedViewList TreeBank::m_ViewUpdateBox = SharedViewList();
NodeList TreeBank::m_NodeDeleteBox = NodeList();

QMap<QKeySequence, QString> TreeBank::m_KeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> TreeBank::m_MouseMap = QMap<QString, QString>();

static void CollectViewDom(QDomElement &elem, ViewNode *parent);
static void CollectHistDom(QDomElement &elem, HistNode *parent, ViewNode *partner);
#ifndef FAST_SAVER
static void CollectViewNode(ViewNode *nd, QDomElement &elem);
static void CollectHistNode(HistNode *nd, QDomElement &elem);
#else
static void CollectViewNodeFlat(ViewNode *nd, QTextStream &out);
static void CollectHistNodeFlat(HistNode *nd, QTextStream &out);
#endif

static SharedView LoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView LoadWithLink(HistNode *hn);
static SharedView LoadWithLink(ViewNode *vn);
static SharedView LoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView AutoLoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView AutoLoadWithLink(ViewNode *vn);
static SharedView AutoLoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static SharedView AutoLoadWithNoLink(HistNode *hn);
static void LinkNode(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static void LinkNode(HistNode* hn);
static void SetHistProp(QNetworkRequest req, HistNode *hn, ViewNode *vn);
static void SetHistProp(HistNode* hn);
static void SetPartner(QUrl url, ViewNode *vn, HistNode *hn, SharedView view = 0);
static void SetPartner(QUrl url, HistNode *hn, ViewNode *vn, SharedView view = 0);

// for renaming directory
static QString GetNetworkSpaceId(ViewNode*);
static QStringList GetNodeSettings(ViewNode*);

TreeBank::TreeBank(QWidget *parent)
    : QWidget(parent)
    , m_Scene(new QGraphicsScene(this))
    , m_View(new GraphicsView(m_Scene, this))
      // if m_PurgeView is true, Notifier and Receiver should be purged.
    , m_Notifier(new Notifier(this, m_PurgeNotifier || m_PurgeView))
    , m_Receiver(new Receiver(this, m_PurgeReceiver || m_PurgeView))
    , m_JsObject(new _Vanilla(this))
{
    m_Gadgets_ = SharedView(new Gadgets(this), &DeleteView);
    m_Gadgets_->SetThis(WeakView(m_Gadgets_));
    m_Gadgets = static_cast<Gadgets*>(m_Gadgets_.get());

    if(m_Viewport == GLWidget){
        // notifier and receiver become black (if not purged)...
        // scrollbar become invisible...
        m_View->setViewport(new QGLWidget(QGLFormat(QGL::DoubleBuffer)));

        // if displaying WebView, become not to repaint.
        // after resized, return to normal.

        // if displaying WeEngineView, it become transparent.
        // because QGLWidget not supports multi-thread,
        // but WebEngineView requests to paint in multi-thread.
    } else if(m_Viewport == OpenGLWidget){
        // font become dirty...
        m_View->setViewport(new QOpenGLWidget());
    }

    setAttribute(Qt::WA_TranslucentBackground);

    m_View->setAcceptDrops(true);
    m_View->setFrameShape(QFrame::NoFrame);
    m_View->setBackgroundBrush(Qt::transparent);
    m_View->setStyleSheet("QGraphicsView{ background: transparent}");
    m_View->setCacheMode(QGraphicsView::CacheBackground);
    m_View->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    m_View->setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing |
                                 QGraphicsView::DontSavePainterState);

    m_Scene->addItem(m_Gadgets);
    m_Scene->setSceneRect(QRect(0, 0, parent->width(), parent->height()));

    m_CurrentView = SharedView();
    m_CurrentViewNode = 0;
    m_CurrentHistNode = 0;
    m_ActionTable = QMap<TreeBankAction, QAction*>();

    // gadgets become too slow when using reconnection.
    connect(m_Gadgets, &Gadgets::titleChanged,
            GetMainWindow(), &MainWindow::SetWindowTitle);

    connect(this, &TreeBank::TreeStructureChanged, m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::NodeCreated,          m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::NodeDeleted,          m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::FoldedChanged,        m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::CurrentChanged,       m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);

    ConnectToNotifier();
    ConnectToReceiver();
}

TreeBank::~TreeBank(){
    if(m_Notifier) m_Notifier->deleteLater();
    if(m_Receiver) m_Receiver->deleteLater();
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

void TreeBank::EmitTreeStructureChanged(){
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->TreeStructureChanged();
    }
}

void TreeBank::EmitNodeCreated(NodeList &nds){
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->NodeCreated(nds);
    }
}

void TreeBank::EmitNodeDeleted(NodeList &nds){
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->NodeDeleted(nds);
    }
}

void TreeBank::EmitFoldedChanged(NodeList &nds){
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->FoldedChanged(nds);
    }
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
       title.contains(QRegularExpression(QStringLiteral("[<>\":\\?\\|\\*/\\\\]")))){

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

    QString parentid = GetNetworkSpaceId(vn->GetParent()->ToViewNode());
    QString befname = before.split(QStringLiteral(";")).first();
    QString aftname = after .split(QStringLiteral(";")).first();
    QStringList befset = before.split(QStringLiteral(";")).mid(1);
    QStringList aftset = after .split(QStringLiteral(";")).mid(1);
    bool bef = befset.indexOf(QRegularExpression(QStringLiteral("\\A[iI][dD](?:entif(?:y|ier|ication))?\\Z"))) != -1;
    bool aft = aftset.indexOf(QRegularExpression(QStringLiteral("\\A[iI][dD](?:entif(?:y|ier|ication))?\\Z"))) != -1;

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
         NetworkController::GetNetworkAccessManager(GetNetworkSpaceId(vn),
                            GetNodeSettings(vn)),
         GetNodeSettings(vn),
         GetNetworkSpaceId(vn),
         GetNetworkSpaceId(dir));
}

void TreeBank::ApplySpecificSettings(ViewNode *vn, NetworkAccessManager *nam, QStringList set,
                                     QString id, QString parentid){
    if(GetNetworkSpaceId(vn) == id || GetNetworkSpaceId(vn) == parentid){
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
static QString GetNetworkSpaceId(ViewNode* vn){
    QString title;
    forever{
        title = vn->GetTitle();
        if(vn == TreeBank::GetViewRoot() || vn == TreeBank::GetTrashRoot() ||
           (vn->IsDirectory() && !title.isEmpty() &&
            title.split(QStringLiteral(";")).indexOf(QRegularExpression(QStringLiteral("\\A[iI][dD](?:entif(?:y|ier|ication))?\\Z"))) != -1)){

            return title.split(QStringLiteral(";")).first();

        } else {
            vn = vn->GetParent()->ToViewNode();
        }
    }
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

void TreeBank::DoUpdate(){
    foreach(SharedView view, m_ViewUpdateBox){
        view->UpdateThumbnail();
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
    m_ViewUpdateBox.removeOne(view);
}

void TreeBank::RemoveFromDeleteBox(Node *nd){
    m_NodeDeleteBox.removeOne(nd);
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
    return (m_Gadgets && m_Gadgets->IsActive())
#ifdef LocalView
        || (m_CurrentView &&
            qobject_cast<LocalView*>(m_CurrentView->base()))
#endif
        ;
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

    QString datadir = Application::DataDirectory();

    QMap<ViewNode*, QString> map;
    map[m_ViewRoot]  = Application::PrimaryTreeFileName();
    map[m_TrashRoot] = Application::SecondaryTreeFileName();
    foreach(ViewNode *root, QList<ViewNode*>() << m_ViewRoot << m_TrashRoot){

        QString filename = map[root];
        QFile file(datadir + filename);
        QDomDocument doc;
        bool check = doc.setContent(&file);
        file.close();

        if(!check){

            QDir dir = QDir(datadir);
            QStringList list =
                dir.entryList(Application::BackUpFileFilters(),
                              QDir::NoFilter, QDir::Name | QDir::Reversed);
            if(list.isEmpty()) break;

            foreach(QString backup, list){

                if(!backup.contains(filename)) continue;

                QFile backupfile(datadir + backup);
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
        QDomElement de;
        for(uint i = 0; i < static_cast<uint>(children.length()); i++){
            de = children.item(i).toElement();
            CollectViewDom(de, root);
        }

        if(root == m_ViewRoot)
            EmitTreeStructureChanged();
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
        QDomElement de = elem.firstChildElement();
        CollectHistDom(de, TreeBank::GetHistRoot(), vn);
    } else {
        QDomNodeList children = elem.childNodes();
        QDomElement de;
        for(uint i = 0; i < static_cast<uint>(children.length()); i++){
            de = children.item(i).toElement();
            CollectViewDom(de, vn);
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
        tb->blockSignals(true);
        tb->SetCurrent(hn);
        tb->blockSignals(false);

    } else if(prt){
        LinkNode(hn);
    } else {
        SetHistProp(hn);
    }

    QDomNodeList children = elem.childNodes();
    QDomElement de;
    for(uint i = 0; i < static_cast<uint>(children.length()); i++){
        de = children.item(i).toElement();
        CollectHistDom(de, hn, partner);
    }
}

void TreeBank::SaveTree(){
    DoDelete();

    QString datadir = Application::DataDirectory();

    QString primary  = datadir + Application::PrimaryTreeFileName(false);
    QString primaryb = datadir + Application::PrimaryTreeFileName(true);

    QString secondary  = datadir + Application::SecondaryTreeFileName(false);
    QString secondaryb = datadir + Application::SecondaryTreeFileName(true);

    if(QFile::exists(primaryb))   QFile::remove(primaryb);
    if(QFile::exists(secondaryb)) QFile::remove(secondaryb);

    QMap<ViewNode*, QString> map;
    map[m_ViewRoot]  = Application::PrimaryTreeFileName(true);
    map[m_TrashRoot] = Application::SecondaryTreeFileName(true);
    foreach(ViewNode *root, QList<ViewNode*>() << m_ViewRoot << m_TrashRoot){

        QString filename = map[root];
        QFile file(datadir + filename);

#ifdef FAST_SAVER
        if(file.open(QIODevice::WriteOnly)){
            QTextStream out(&file);
            out.setCodec(QTextCodec::codecForName("UTF-8"));
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            out << "<viewnode root=\"true\">\n";
            foreach(Node *child, root->GetChildren()){
                CollectViewNodeFlat(child->ToViewNode(), out);
            }
            out << "</viewnode>\n";
        }
        file.close();
#else //ifdef FAST_SAVER
        QDomDocument doc;
        doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
        QDomElement elem = doc.createElement(QStringLiteral("viewnode"));
        elem.setAttribute(QStringLiteral("root"), QStringLiteral("true"));
        doc.appendChild(elem);
        foreach(Node *child, root->GetChildren()){
            CollectViewNode(child->ToViewNode(), elem);
        }
        if(file.open(QIODevice::WriteOnly)){
            QTextStream out(&file);
            doc.save(out, 2);
        }
        file.close();
#endif //ifdef FAST_SAVER
    }

    if(QFile::exists(primary))   QFile::remove(primary);
    if(QFile::exists(secondary)) QFile::remove(secondary);

    QFile::rename(primaryb,   primary);
    QFile::rename(secondaryb, secondary);
}

#ifndef FAST_SAVER
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
#else
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
#endif

void TreeBank::LoadSettings(){
    Settings &s = Application::GlobalSettings();

    m_RootName             = s.value(QStringLiteral("application/@RootName"), QStringLiteral("root;id")).value<QString>();
    m_MaxViewCount         = s.value(QStringLiteral("application/@MaxViewCount")         , -1)   .value<int>();
    m_MaxTrashEntryCount   = s.value(QStringLiteral("application/@MaxTrashEntryCount")   , -1)   .value<int>();
    m_TraverseAllView      = s.value(QStringLiteral("application/@TraverseAllView")      , false).value<bool>();
    m_PurgeNotifier        = s.value(QStringLiteral("application/@PurgeNotifier")        , false).value<bool>();
    m_PurgeReceiver        = s.value(QStringLiteral("application/@PurgeReceiver")        , false).value<bool>();
    m_PurgeView            = s.value(QStringLiteral("application/@PurgeView")            , false).value<bool>();

    QString viewport = s.value(QStringLiteral("application/@Viewport"), QStringLiteral("Widget")).value<QString>();
    if(viewport == QStringLiteral("Widget"))       m_Viewport = Widget;
    if(viewport == QStringLiteral("GLWidget"))     m_Viewport = GLWidget;
    if(viewport == QStringLiteral("OpenGLWidget")) m_Viewport = OpenGLWidget;

    if(m_MaxViewCount == -1)       m_MaxViewCount       = s.value(QStringLiteral("application/@MaxView")  , -1).value<int>();
    if(m_MaxViewCount == -1)       m_MaxViewCount       = 10;
    if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = s.value(QStringLiteral("application/@MaxTrash") , -1).value<int>();
    if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = 100;

    {   QStringList keys = s.allKeys(QStringLiteral("application/keymap"));

        if(keys.isEmpty()){
            /* default key map. */
            TREEBANK_KEYMAP
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
    {   QStringList keys = s.allKeys(QStringLiteral("application/mouse"));

        if(keys.isEmpty()){
            /* default mouse map. */
            TREEBANK_MOUSEMAP
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
    Node::LoadSettings();
    View::LoadSettings();
    Gadgets::LoadSettings();
}

void TreeBank::SaveSettings(){
    Settings &s = Application::GlobalSettings();

    s.setValue(QStringLiteral("application/@RootName"),             m_RootName);
    s.setValue(QStringLiteral("application/@MaxViewCount"),         m_MaxViewCount);
    s.setValue(QStringLiteral("application/@MaxTrashEntryCount"),   m_MaxTrashEntryCount);
    s.setValue(QStringLiteral("application/@TraverseAllView"),      m_TraverseAllView);
    s.setValue(QStringLiteral("application/@PurgeNotifier"),        m_PurgeNotifier);
    s.setValue(QStringLiteral("application/@PurgeReceiver"),        m_PurgeReceiver);
    s.setValue(QStringLiteral("application/@PurgeView"),            m_PurgeView);

    if(m_Viewport == Widget)       s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("Widget"));
    if(m_Viewport == GLWidget)     s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("GLWidget"));
    if(m_Viewport == OpenGLWidget) s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("OpenGLWidget"));

    foreach(QKeySequence seq, m_KeyMap.keys()){
        if(!seq.isEmpty() && !seq.toString().isEmpty()){
            s.setValue(QStringLiteral("application/keymap/") + seq.toString()
                        // cannot use slashes on QSettings.
                          .replace(QStringLiteral("\\"), QStringLiteral("Backslash"))
                          .replace(QStringLiteral("/"), QStringLiteral("Slash")),
                        m_KeyMap[seq]);
        }
    }

    foreach(QString button, m_MouseMap.keys()){
        if(!button.isEmpty())
            s.setValue(QStringLiteral("application/mouse/") + button, m_MouseMap[button]);
    }
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
            ToolBar *bar = tb ? tb->GetMainWindow()->GetToolBar() : 0;

            if(tb && v == tb->GetCurrentView()){
                v->Disconnect(tb);
                tb->SetCurrentView(0);
                if(bar && bar->isVisible())
                    bar->Disconnect(v);
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
        //hn->SetImage(QImage());
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
    if(ViewNode *vn = nd->ToViewNode()){
        QuarantineViewNode(vn);

        if(vn->GetPartner())
            StripSubTree(GetRoot(vn->GetPartner()));

    } else if(HistNode *hn = nd->ToHistNode()){
        // it's not 'QuarantineHistNode'.
        // want to clear, not to reset to other node.

        TreeBank *tb = hn->GetView() ? hn->GetView()->GetTreeBank() : 0;

        if(tb){
            if(hn == tb->GetCurrentHistNode())  tb->SetCurrentHistNode(0);
            if(hn == tb->GetHistIterForward())  tb->SetHistIterForward(0);
            if(hn == tb->GetHistIterBackward()) tb->SetHistIterBackward(0);
        }

        DislinkView(hn);
        //hn->SetImage(QImage());
    }
    foreach(Node *child, nd->GetChildren()){
        StripSubTree(child);
    }
}

void TreeBank::ReleaseView(SharedView view){
    RemoveFromAllViews(view);
    RemoveFromUpdateBox(view);
}

void TreeBank::ReleaseAllView(){
    // when quit application.

    m_ViewUpdateBox.clear();

    foreach(SharedView view, m_AllViews){
#ifdef TRIDENTVIEW
        if(TridentView *w = qobject_cast<TridentView*>(view->base()))
            w->Clear();
#endif
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
        if(sample->IsHistNode()){
            foreach(SharedView view, m_AllViews){
                if(prevpartner == view->GetViewNode())
                    if(SetCurrent(view->GetHistNode()))
                        return true;
            }
        } else if(sample->IsViewNode()){
            foreach(SharedView view, m_AllViews){
                if(prevparent == view->GetViewNode()->GetParent())
                    if(SetCurrent(view->GetViewNode()))
                        return true;
            }

            NodeList list = prevparent->GetChildren();
            qSort(list.begin(), list.end(), [](Node *n1, Node *n2){
                return n1->GetLastAccessDate() > n2->GetLastAccessDate();
            });
            foreach(Node *nd, list){
                if(!nd->IsDirectory())
                    if(SetCurrent(nd))
                        return true;
            }
        }

        foreach(SharedView view, m_AllViews){
            if(SetCurrent(view))
                return true;
        }
        if(SetCurrent(m_ViewRoot)){
            return true;
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
        EmitTreeStructureChanged();
    } else {
        // move to other directory.
        bool wasTrash = IsTrash(nd);
        DisownNode(nd);
        nd->SetParent(dir);
        dir->InsertChild((n < 0 || n > dir->ChildrenLength()) ?
                           dir->ChildrenLength() : n,
                         nd);

        if(nd->IsViewNode())
            ApplySpecificSettings(nd->ToViewNode(), dir->ToViewNode());

        bool isTrash = IsTrash(nd);
        if(wasTrash && !isTrash) EmitNodeCreated(NodeList() << nd);
        if(!wasTrash && isTrash) EmitNodeDeleted(NodeList() << nd);
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
    EmitTreeStructureChanged();
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
        ToolBar *bar = GetMainWindow()->GetToolBar();
        if(prev){
            AddToUpdateBox(prev);
            prev->Disconnect(this);
            if(bar->isVisible())
                bar->Disconnect(prev);
        }
        if(bar->isVisible())
            bar->Connect(m_CurrentView);
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

#ifdef LocalView
    if(SharedView v = m_CurrentView->GetSlave().lock()){
        // why cannot catch lovalview object?
        if(qobject_cast<LocalView*>(v->base())){
            v->lower();
            v->hide();
            v->SetMaster(WeakView());
            m_CurrentView->SetSlave(WeakView());
        }
    }
#endif
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

    //                                high
    // QuickWeb(Engine|Kit)View        ^
    // Web(Engine|Kit)View             |
    // Gadgets                   order of layer
    // LocalView                       |
    // GraphicsWebKitView              V
    //                                low

#ifdef WEBKITVIEW
    if(qobject_cast<GraphicsWebKitView*>(m_CurrentView->base())){

        m_CurrentView->show();
        DoUpdate();
        if(prev && prev != m_CurrentView){
            prev->lower();
            prev->hide();
        }
        m_View->raise();
        m_CurrentView->raise();

    } else
#endif
#ifdef LocalView
    if(qobject_cast<LocalView*>(m_CurrentView->base())){

        m_CurrentView->show();
        DoUpdate();
        if(prev && prev != m_CurrentView){
            m_CurrentView->SetMaster(prev);
            prev->SetSlave(m_CurrentView);
        }
        m_View->raise();
        m_CurrentView->raise();

    } else
#endif
    { // Web(Engine|Kit)View or QuickWeb(Engine|Kit)View.

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
        QTimer::singleShot(0, this, [this](){
            if(!parentWidget()->isActiveWindow())
                parentWidget()->activateWindow();
            m_CurrentView->setFocus(Qt::OtherFocusReason);
        });

    AddToUpdateBox(m_CurrentView);

    emit CurrentChanged(m_CurrentViewNode);
    return true;
}

bool TreeBank::SetCurrent(SharedView view){
    return SetCurrent(view->GetHistNode());
}

void TreeBank::NthView(int n, ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    NodeList siblings = vn->GetSiblings();
    if(n >= 0 && n < siblings.length())
        SetCurrent(siblings[n]);
}

void TreeBank::GoBackOrCloseForDownload(View *view){
    view->CallWithWholeHtml([this, view](const QString &html){

        if(html.isEmpty() || html == EMPTY_FRAME_HTML ||
           (html.length() < 1000 && html.endsWith(QStringLiteral("</head></html>")))){

            HistNode *hist = view->GetHistNode();

            if(hist->IsRoot() && hist->HasNoChildren()){

                if(view->CanGoBack() && !view->CanGoForward()){

                    view->TriggerNativeGoBackAction();

                } else if(!view->CanGoBack() && !view->CanGoForward()){

                    view->TriggerAction(Page::_Close);
                }
            } else if(Node *nd = hist->Prev()){

                DeleteNode(nd);
            }
        }
    });
}

#if defined(TRIDENTVIEW)
bool TreeBank::TridentViewExist(){
    static bool exists = false;
    if(exists) return true;
    foreach(SharedView view, m_AllViews){
        if(qobject_cast<TridentView*>(view->base())){
            exists = true;
            return true;
        }
    }
    return false;
}
#endif

void TreeBank::BeforeStartingDisplayGadgets(){
    DoUpdate();

    // if current focus is other widget(e.g. 'WebEngineView'),
    // cannot set focus to 'Gadgets' directly.
    parentWidget()->setFocus();
    setFocus();

    if(m_CurrentView)
        m_CurrentView->Disconnect(this);

    m_Gadgets->Connect(this);
    m_Gadgets->setParent(this);

    if(m_CurrentView){
        m_Gadgets->SetMaster(m_CurrentView->GetThis());
        m_CurrentView->SetSlave(m_Gadgets->GetThis());
        m_CurrentView->OnBeforeStartingDisplayGadgets();
    }

    m_View->raise();
    m_View->setFocus();
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

    m_View->lower();
}

void TreeBank::MousePressEvent(QMouseEvent *ev){
    QWidget::mousePressEvent(ev);
}

void TreeBank::MouseReleaseEvent(QMouseEvent *ev){
    QWidget::mouseReleaseEvent(ev);
}

void TreeBank::MouseMoveEvent(QMouseEvent *ev){
    QWidget::mouseMoveEvent(ev);
}

void TreeBank::MouseDoubleClickEvent(QMouseEvent *ev){
    QWidget::mouseDoubleClickEvent(ev);
}

void TreeBank::WheelEvent(QWheelEvent *ev){
    QWidget::wheelEvent(ev);
}

void TreeBank::DragEnterEvent(QDragEnterEvent *ev){
    QWidget::dragEnterEvent(ev);
}

void TreeBank::DragMoveEvent(QDragMoveEvent *ev){
    QWidget::dragMoveEvent(ev);
}

void TreeBank::DragLeaveEvent(QDragLeaveEvent *ev){
    QWidget::dragLeaveEvent(ev);
}

void TreeBank::DropEvent(QDropEvent *ev){
    QWidget::dropEvent(ev);
}

void TreeBank::ContextMenuEvent(QContextMenuEvent *ev){
    QWidget::contextMenuEvent(ev);
}

void TreeBank::KeyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
}

void TreeBank::KeyReleaseEvent(QKeyEvent *ev){
    QWidget::keyReleaseEvent(ev);
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
    EmitNodeCreated(NodeList() << young);
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

SharedView TreeBank::OpenOnSuitableNode(QNetworkRequest req, bool activate, ViewNode *parent, int position){
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
    ViewNode *page = parent->MakeChild(position);
    SharedView view = LoadWithLink(req, hist, page);
    EmitNodeCreated(NodeList() << page);
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

SharedView TreeBank::OpenOnSuitableNode(QUrl url, bool activate, ViewNode *parent, int position){
    return OpenOnSuitableNode(QNetworkRequest(url), activate, parent, position);
}

SharedView TreeBank::OpenOnSuitableNode(QList<QNetworkRequest> reqs, bool activate, ViewNode *parent, int position){
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = 0; i < reqs.length(); i++){
        bool last = (i == (reqs.length() - 1));
        SharedView view = OpenOnSuitableNode(reqs[i], last && activate, parent, position);
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

SharedView TreeBank::OpenOnSuitableNode(QList<QUrl> urls, bool activate, ViewNode *parent, int position){
    QList<QNetworkRequest> reqs;
    foreach(QUrl url, urls) reqs << QNetworkRequest(url);
    return OpenOnSuitableNode(reqs, activate, parent, position);
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
    EmitNodeCreated(NodeList() << young->GetParent() << young);
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

static SharedView AutoLoadWithLink(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = SharedView();
    if(GetNodeSettings(vn).indexOf(QRegularExpression(QStringLiteral("\\A[nN](?:o)?(?:[aA](?:uto)?)?[lL](?:oad)?\\Z"))) == -1)
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

static SharedView AutoLoadWithLink(ViewNode *vn){
    if(vn->GetPartner())
        return AutoLoadWithLink(QNetworkRequest(vn->GetUrl()),
                                vn->GetPartner()->ToHistNode(), vn);
    return 0;
}

static SharedView AutoLoadWithNoLink(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = SharedView();
    if(GetNodeSettings(vn).indexOf(QRegularExpression(QStringLiteral("\\A[nN](?:o)?(?:[aA](?:uto)?)?[lL](?:oad)?\\Z"))) == -1)
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

    menu->addAction(Action(_DisplayViewTree));
    menu->addAction(Action(_DisplayHistTree));
    menu->addAction(Action(_DisplayTrashTree));
    menu->addSeparator();
    menu->addAction(Action(_Close));
    menu->addAction(Action(_Restore));
    menu->addAction(Action(_NextView));
    menu->addAction(Action(_PrevView));
    menu->addAction(Action(_DigView));
    menu->addAction(Action(_BuryView));
    menu->addAction(Action(_NewViewNode));
    menu->addAction(Action(_NewHistNode));
    menu->addAction(Action(_CloneViewNode));
    menu->addAction(Action(_CloneHistNode));

    return menu;
}

QMenu *TreeBank::DisplayMenu(){
    QMenu *menu = new QMenu(tr("Display"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_ToggleNotifier));
    menu->addAction(Action(_ToggleReceiver));
    menu->addAction(Action(_ToggleMenuBar));
    menu->addAction(Action(_ToggleTreeBar));
    menu->addAction(Action(_ToggleToolBar));
    UpdateAction();

    return menu;
}

QMenu *TreeBank::WindowMenu(){
    QMenu *menu = new QMenu(tr("Window"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_ToggleFullScreen));
    menu->addAction(Action(_ToggleMaximized));
    menu->addAction(Action(_ToggleMinimized));
    if(Application::EnableFramelessWindow())
        menu->addAction(Action(_ToggleShaded));
    menu->addAction(Action(_NewWindow));
    menu->addAction(Action(_CloseWindow));
    menu->addAction(Action(_SwitchWindow));
    menu->addAction(Action(_NextWindow));
    menu->addAction(Action(_PrevWindow));

    return menu;
}

QMenu *TreeBank::PageMenu(){
    QMenu *menu = new QMenu(tr("Page"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_Copy));
    menu->addAction(Action(_Cut));
    menu->addAction(Action(_Paste));
    menu->addAction(Action(_Undo));
    menu->addAction(Action(_Redo));
    menu->addAction(Action(_SelectAll));
    menu->addAction(Action(_Reload));
    menu->addAction(Action(_Stop));
    menu->addSeparator();
    menu->addAction(Action(_ZoomIn));
    menu->addAction(Action(_ZoomOut));
    menu->addSeparator();
    menu->addAction(Action(_Load));
    menu->addAction(Action(_Save));
    menu->addAction(Action(_Print));
    return menu;
}

QMenu *TreeBank::ApplicationMenu(bool expanded){
    QMenu *menu = new QMenu(tr("Application"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_Import));
    menu->addAction(Action(_Export));
    menu->addAction(Action(_AboutVanilla));
    menu->addAction(Action(_AboutQt));

    if(expanded){
        menu->addSeparator();
        menu->addAction(Action(_OpenTextSeeker));
        menu->addAction(Action(_OpenQueryEditor));
        menu->addAction(Action(_OpenUrlEditor));
        menu->addAction(Action(_OpenCommand));
        menu->addAction(Action(_ReleaseHiddenView));
        menu->addSeparator();
        menu->addAction(Action(_Quit));
    }

    return menu;
}

QMenu *TreeBank::GlobalContextMenu(){
    QMenu *menu = new QMenu(this);
    menu->setToolTipsVisible(true);

    menu->addMenu(ApplicationMenu(false));
    menu->addMenu(NodeMenu());
    menu->addMenu(DisplayMenu());
    menu->addMenu(WindowMenu());
    menu->addMenu(PageMenu());
    menu->addSeparator();
    menu->addAction(Action(_OpenTextSeeker));
    menu->addAction(Action(_OpenQueryEditor));
    menu->addAction(Action(_OpenUrlEditor));
    menu->addAction(Action(_OpenCommand));
    menu->addAction(Action(_ReleaseHiddenView));
    menu->addSeparator();
    menu->addAction(Action(_Quit));

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
    m_View->setGeometry(QRect(QPoint(), ev->size()));
    m_View->setSceneRect(0.0, 0.0,
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
    QWidget::wheelEvent(ev);
}

void TreeBank::mouseMoveEvent(QMouseEvent *ev){
    QWidget::mouseMoveEvent(ev);
}

void TreeBank::mousePressEvent(QMouseEvent *ev){
    QWidget::mousePressEvent(ev);
}

void TreeBank::dragEnterEvent(QDragEnterEvent *ev){
    QWidget::dragEnterEvent(ev);
    if(ev->isAccepted()) return;

    ev->setDropAction(Qt::MoveAction);
    ev->acceptProposedAction();
    ev->setAccepted(true);
}

void TreeBank::dragMoveEvent(QDragMoveEvent *ev){
    QWidget::dragMoveEvent(ev);
    if(ev->isAccepted()) return;
    ev->setAccepted(true);
}

void TreeBank::dragLeaveEvent(QDragLeaveEvent *ev){
    QWidget::dragLeaveEvent(ev);
    if(ev->isAccepted()) return;
    ev->setAccepted(true);
}

void TreeBank::dropEvent(QDropEvent *ev){
    QWidget::dropEvent(ev);

    // always accepted?
    //if(ev->isAccepted()) return;
    if(m_CurrentView) return;

    if(!ev->mimeData()->urls().isEmpty()){
        if(OpenOnSuitableNode(ev->mimeData()->urls(), true))
            ev->setAccepted(true);
    }
}

void TreeBank::mouseReleaseEvent(QMouseEvent *ev){
    QWidget::mouseReleaseEvent(ev);

    // always accepted?
    //if(ev->isAccepted()) return;

    // not decidable from 'ev'.
    //if(ev->button() == Qt::RightButton){
    //    QMenu *menu = GlobalContextMenu();
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
    QWidget::contextMenuEvent(ev);
    if(ev->isAccepted()) return;

    QMenu *menu = GlobalContextMenu();
    menu->exec(ev->globalPos());
    delete menu;
    ev->setAccepted(true);
}

void TreeBank::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
    if(ev->isAccepted()) return;

    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return;

    if(Application::HasAnyModifier(ev) ||
       Application::IsFunctionKey(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
        return;
    }
    if(!Application::IsOnlyModifier(ev)){

        ev->setAccepted(TriggerKeyEvent(ev));
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
    if(view) view->TriggerAction(Page::_Up);
}

void TreeBank::Down(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Down);
}

void TreeBank::Right(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Right);
}

void TreeBank::Left(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Left);
}

void TreeBank::PageUp(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_PageUp);
}

void TreeBank::PageDown(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_PageDown);
}

void TreeBank::Home(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Home);
}

void TreeBank::End(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_End);
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
    UpdateAction();
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
    UpdateAction();
}

void TreeBank::ToggleMenuBar(){
    GetMainWindow()->ToggleMenuBar();
    UpdateAction();
}

void TreeBank::ToggleTreeBar(){
    GetMainWindow()->ToggleTreeBar();
    UpdateAction();
}

void TreeBank::ToggleToolBar(){
    GetMainWindow()->ToggleToolBar();
    UpdateAction();
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

    View *view = hist->GetView();

    if(hist->IsRoot() && hist->HasNoChildren() &&
       view && view->CanGoBack()){

        view->SetScroll(QPointF());
        view->TriggerNativeGoBackAction();

    } else if(Node *nd = hist->Prev()){

        SetCurrent(nd);

    } else if(view){

        if(View::EnableDestinationInferrer())
            view->GoBackToInferedUrl();
    }
}

void TreeBank::Forward(HistNode *hist){
    m_TraverseMode = HistMode;
    if(!hist) hist = m_CurrentHistNode;
    if(!hist) return;

    View *view = hist->GetView();

    if(hist->IsRoot() && hist->HasNoChildren() &&
       view && view->CanGoForward()){

        view->SetScroll(QPointF());
        view->TriggerNativeGoForwardAction();

    } else if(Node *nd = hist->Next()){

        SetCurrent(nd);

    } else if(view){

        if(View::EnableDestinationInferrer())
            view->GoForwardToInferedUrl();
    }
}

void TreeBank::Rewind(HistNode *hist){
    m_TraverseMode = HistMode;
    if(!hist) hist = m_CurrentHistNode;
    if(!hist) return;

    View *view = hist->GetView();

    if(hist->IsRoot() && hist->HasNoChildren() &&
       view && view->CanGoBack()){

        view->SetScroll(QPointF());
        view->TriggerNativeRewindAction();

    } else if(Node *nd = hist->Prev()){

        while(nd->Prev()) nd = nd->Prev();
        SetCurrent(nd);
    }
}

void TreeBank::FastForward(HistNode *hist){
    m_TraverseMode = HistMode;
    if(!hist) hist = m_CurrentHistNode;
    if(!hist) return;

    View *view = hist->GetView();

    if(hist->IsRoot() && hist->HasNoChildren() &&
       view && view->CanGoForward()){

        view->SetScroll(QPointF());
        view->TriggerNativeFastForwardAction();

    } else if(Node *nd = hist->Next()){

        while(nd->Next()) nd = nd->Next();
        SetCurrent(nd);

    } else if(view){

        view->GoForwardToInferedUrl();
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
            EmitNodeCreated(NodeList() << rest);
        } else {
            // on tableview(trash).
            MoveNode(vn, m_ViewRoot);
        }
    } else {
        if(dir){
            ViewNode *rest = m_TrashRoot->GetFirstChild()->ToViewNode();
            MoveNode(rest, GetRoot(dir) == m_ViewRoot ? dir : m_ViewRoot);
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
            EmitNodeCreated(NodeList() << young);
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
            RemoveFromAllViews(view);
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

void TreeBank::FirstView(ViewNode *vn){
    NthView(0, vn);
}

void TreeBank::SecondView(ViewNode *vn){
    NthView(1, vn);
}

void TreeBank::ThirdView(ViewNode *vn){
    NthView(2, vn);
}

void TreeBank::FourthView(ViewNode *vn){
    NthView(3, vn);
}

void TreeBank::FifthView(ViewNode *vn){
    NthView(4, vn);
}

void TreeBank::SixthView(ViewNode *vn){
    NthView(5, vn);
}

void TreeBank::SeventhView(ViewNode *vn){
    NthView(6, vn);
}

void TreeBank::EighthView(ViewNode *vn){
    NthView(7, vn);
}

void TreeBank::NinthView(ViewNode *vn){
    NthView(8, vn);
}

void TreeBank::TenthView(ViewNode *vn){
    NthView(9, vn);
}

void TreeBank::LastView(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    NodeList siblings = vn->GetSiblings();
    if(!siblings.isEmpty()){
        SetCurrent(siblings.last());
    }
}

ViewNode *TreeBank::NewViewNode(ViewNode *vn){
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return 0;

    ViewNode *newNode = vn->New();
    if(newNode){
        if(m_Gadgets && m_Gadgets->IsActive()){
            EmitNodeCreated(NodeList() << newNode);
            return newNode;
        }
    } else {
        return 0;
    }
    SharedView view = LoadWithLink(newNode);
    EmitNodeCreated(NodeList() << newNode);
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

    HistNode *newNode = hn->New();
    if(newNode){
        if(m_Gadgets && m_Gadgets->IsActive()){
            return newNode;
        }
    } else {
        return 0;
    }
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

    ViewNode *clone = vn->Clone();
    if(clone){
        if(m_Gadgets && m_Gadgets->IsActive()){
            EmitNodeCreated(NodeList() << clone);
            return clone;
        }
    } else {
        return 0;
    }
    SharedView view = LoadWithLink(clone);
    EmitNodeCreated(NodeList() << clone);
    if(Page::Activate()){
        SetCurrent(clone);
    } else {
        if(view) view->hide();
        // need to tune order, because 'LoadWithLink' and 'LoadWithNoLink' add new view to head of 'm_AllViews'.
        RaiseDisplayedViewPriority();
    }
    return clone;
}

HistNode *TreeBank::CloneHistNode(HistNode *hn){
    m_TraverseMode = HistMode;
    if(!hn) hn = m_CurrentHistNode;
    if(!hn) return 0;

    HistNode *clone = hn->Clone();
    if(clone){
        if(m_Gadgets && m_Gadgets->IsActive()){
            return clone;
        }
    } else {
         return 0;
    }
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
        ModalDialog::GetExistingDirectory(QString(), QStringLiteral("."), options);

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
        EmitNodeCreated(NodeList() << young->GetParent() << young);
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
    if(vn){
        ViewNode *child = vn->MakeChild();
        EmitNodeCreated(NodeList() << child);
        return child;
    }
    return 0;
}

ViewNode *TreeBank::MakeSiblingDirectory(ViewNode *vn){
    // make empty directory.
    m_TraverseMode = ViewMode;
    if(!vn) vn = m_ViewRoot;
    if(vn){
        ViewNode *sibling = vn->MakeSibling();
        EmitNodeCreated(NodeList() << sibling);
        return sibling;
    }
    return 0;
}

// gadgets.
////////////////////////////////////////////////////////////////

void TreeBank::DisplayViewTree(ViewNode *vn){
    if(m_Gadgets->IsDisplaying(Gadgets::ViewTree))
        return m_Gadgets->Deactivate();
    m_Gadgets->Activate(Gadgets::ViewTree);
    if(!vn) vn = m_CurrentViewNode;
    if(!vn && !m_ViewRoot->HasNoChildren())
        vn = m_ViewRoot->GetFirstChild()->ToViewNode();
    m_Gadgets->SetCurrent(vn);
}

void TreeBank::DisplayHistTree(HistNode *hn){
    if(m_Gadgets->IsDisplaying(Gadgets::HistTree))
        return m_Gadgets->Deactivate();
    m_Gadgets->Activate(Gadgets::HistTree);
    if(!hn) hn = m_CurrentHistNode;
    if(!hn && !m_HistRoot->HasNoChildren())
        hn = m_HistRoot->GetFirstChild()->ToHistNode();
    m_Gadgets->SetCurrent(hn);
}

void TreeBank::DisplayTrashTree(ViewNode *vn){
    if(m_Gadgets->IsDisplaying(Gadgets::TrashTree))
        return m_Gadgets->Deactivate();
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

void TreeBank::ReleaseHiddenView(SharedView){
    foreach(SharedView view, m_AllViews){
        if(TreeBank *tb = view->GetTreeBank())
            if(!tb->IsCurrent(view))
                DislinkView(view->GetHistNode());
    }
}

void TreeBank::Load(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Load);
}

void TreeBank::Copy(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Copy);
}

void TreeBank::Cut(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Cut);
}

void TreeBank::Paste(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Paste);
}

void TreeBank::Undo(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Undo);
}

void TreeBank::Redo(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Redo);
}

void TreeBank::SelectAll(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_SelectAll);
}

void TreeBank::Unselect(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Unselect);
}

void TreeBank::Reload(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Reload);
}

void TreeBank::ReloadAndBypassCache(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_ReloadAndBypassCache);
}

void TreeBank::Stop(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Stop);
}

void TreeBank::StopAndUnselect(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_StopAndUnselect);
}

void TreeBank::Print(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Print);
}

void TreeBank::Save(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Save);
}

void TreeBank::ZoomIn(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_ZoomIn);
}

void TreeBank::ZoomOut(SharedView view){
    // for name duplication.
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_ZoomOut);
}

void TreeBank::ViewSource(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_ViewSource);
}

void TreeBank::ApplySource(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_ApplySource);
}

void TreeBank::InspectElement(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_InspectElement);
}

void TreeBank::CopyUrl(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_CopyUrl);
}

void TreeBank::CopyTitle(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_CopyTitle);
}

void TreeBank::CopyPageAsLink(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_CopyPageAsLink);
}

void TreeBank::CopySelectedHtml(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_CopySelectedHtml);
}

void TreeBank::OpenWithIE(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithIE);
}

void TreeBank::OpenWithEdge(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithEdge);
}

void TreeBank::OpenWithFF(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithFF);
}

void TreeBank::OpenWithOpera(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithOpera);
}

void TreeBank::OpenWithOPR(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithOPR);
}

void TreeBank::OpenWithSafari(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithSafari);
}

void TreeBank::OpenWithChrome(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithChrome);
}

void TreeBank::OpenWithSleipnir(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithSleipnir);
}

void TreeBank::OpenWithVivaldi(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithVivaldi);
}

void TreeBank::OpenWithCustom(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithCustom);
}

void TreeBank::UpdateAction(){
    Action(_ToggleNotifier)->setChecked(m_Notifier);
    Action(_ToggleReceiver)->setChecked(m_Receiver);
    Action(_ToggleMenuBar)->setChecked(!GetMainWindow()->IsMenuBarEmpty());
    Action(_ToggleTreeBar)->setChecked(GetMainWindow()->GetTreeBar()->isVisible());
    Action(_ToggleToolBar)->setChecked(GetMainWindow()->GetToolBar()->isVisible());
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

QAction *TreeBank::Action(QString str){
    if(IsValidAction(str))
        return Action(StringToAction(str));
    return 0;
}

QAction *TreeBank::Action(TreeBankAction a){
    // forbid many times call of same action.
    static const QList<TreeBankAction> exclude = QList<TreeBankAction>()
        << _NoAction << _End      << _Undo
        << _Up       << _PageUp   << _Redo
        << _Down     << _PageDown << _SelectAll
        << _Right    << _Cut      << _SwitchWindow
        << _Left     << _Copy     << _NextWindow
        << _Home     << _Paste    << _PrevWindow;
    static TreeBankAction previousAction = _NoAction;
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

    QAction *action = m_ActionTable[a];
    if(action){
        switch(a){
        case _ToggleNotifier:
            action->setChecked(m_Notifier);
            break;
        case _ToggleReceiver:
            action->setChecked(m_Receiver);
            break;
        case _ToggleMenuBar:
            action->setChecked(!GetMainWindow()->IsMenuBarEmpty());
            break;
        case _ToggleTreeBar:
            action->setChecked(GetMainWindow()->GetTreeBar()->isVisible());
            break;
        case _ToggleToolBar:
            action->setChecked(GetMainWindow()->GetToolBar()->isVisible());
            break;
        default: break;
        }
        return action;
    }

    m_ActionTable[a] = action = new QAction(this);

    switch(a){
    case _Up:          action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));       break;
    case _Down:        action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));     break;
    case _Right:       action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight));    break;
    case _Left:        action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));     break;
    case _Back:        action->setIcon(QIcon(":/resources/menu/back.png"));        break;
    case _Forward:     action->setIcon(QIcon(":/resources/menu/forward.png"));     break;
    case _Rewind:      action->setIcon(QIcon(":/resources/menu/rewind.png"));      break;
    case _FastForward: action->setIcon(QIcon(":/resources/menu/fastforward.png")); break;
    case _Reload:      action->setIcon(QIcon(":/resources/menu/reload.png"));      break;
    case _Stop:        action->setIcon(QIcon(":/resources/menu/stop.png"));        break;
    default: break;
    }

    switch(a){
    case _NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case _##name:                                                   \
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
        case _##name:                                                   \
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
        DEFINE_ACTION(ToggleTreeBar,    tr("ToggleTreeBar"));
        DEFINE_ACTION(ToggleToolBar,    tr("ToggleToolBar"));
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
        DEFINE_ACTION(Rewind,           tr("Rewind"));
        DEFINE_ACTION(FastForward,      tr("FastForward"));
        DEFINE_ACTION(UpDirectory,      tr("UpDirectory"));
        DEFINE_ACTION(Close,            tr("Close"));
        DEFINE_ACTION(Restore,          tr("Restore"));
        DEFINE_ACTION(Recreate,         tr("Recreate"));
        DEFINE_ACTION(NextView,         tr("NextView"));
        DEFINE_ACTION(PrevView,         tr("PrevView"));
        DEFINE_ACTION(BuryView,         tr("BuryView"));
        DEFINE_ACTION(DigView,          tr("DigView"));
        DEFINE_ACTION(FirstView,        tr("FirstView"));
        DEFINE_ACTION(SecondView,       tr("SecondView"));
        DEFINE_ACTION(ThirdView,        tr("ThirdView"));
        DEFINE_ACTION(FourthView,       tr("FourthView"));
        DEFINE_ACTION(FifthView,        tr("FifthView"));
        DEFINE_ACTION(SixthView,        tr("SixthView"));
        DEFINE_ACTION(SeventhView,      tr("SeventhView"));
        DEFINE_ACTION(EighthView,       tr("EighthView"));
        DEFINE_ACTION(NinthView,        tr("NinthView"));
        DEFINE_ACTION(TenthView,        tr("TenthView"));
        DEFINE_ACTION(LastView,         tr("LastView"));
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
        DEFINE_ACTION(ReleaseHiddenView,tr("ReleaseHiddenView"));
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

        DEFINE_ACTION(InspectElement,       tr("InspectElement"));

        DEFINE_ACTION(CopyUrl,              tr("CopyUrl"));
        DEFINE_ACTION(CopyTitle,            tr("CopyTitle"));
        DEFINE_ACTION(CopyPageAsLink,       tr("CopyPageAsLink"));
        DEFINE_ACTION(CopySelectedHtml,     tr("CopySelectedHtml"));
        DEFINE_ACTION(OpenWithIE,           tr("OpenWithIE"));
        DEFINE_ACTION(OpenWithEdge,         tr("OpenWithEdge"));
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

    case _ToggleNotifier:
        action->setCheckable(true);
        action->setChecked(m_Notifier);
        action->setText(tr("Notifier"));
        action->setToolTip(tr("Notifier"));
        break;
    case _ToggleReceiver:
        action->setCheckable(true);
        action->setChecked(m_Receiver);
        action->setText(tr("Receiver"));
        action->setToolTip(tr("Receiver"));
        break;
    case _ToggleMenuBar:
        action->setCheckable(true);
        action->setChecked(!GetMainWindow()->IsMenuBarEmpty());
        action->setText(tr("MenuBar"));
        action->setToolTip(tr("MenuBar"));
        break;
    case _ToggleTreeBar:
        action->setCheckable(true);
        action->setChecked(GetMainWindow()->GetTreeBar()->isVisible());
        action->setText(tr("TreeBar"));
        action->setToolTip(tr("TreeBar"));
        break;
    case _ToggleToolBar:
        action->setCheckable(true);
        action->setChecked(GetMainWindow()->GetToolBar()->isVisible());
        action->setText(tr("ToolBar"));
        action->setToolTip(tr("ToolBar"));
        break;

    case _OpenWithIE:
        action->setIcon(Application::BrowserIcon_IE());
        break;
    case _OpenWithEdge:
        action->setIcon(Application::BrowserIcon_Edge());
        break;
    case _OpenWithFF:
        action->setIcon(Application::BrowserIcon_FF());
        break;
    case _OpenWithOpera:
        action->setIcon(Application::BrowserIcon_Opera());
        break;
    case _OpenWithOPR:
        action->setIcon(Application::BrowserIcon_OPR());
        break;
    case _OpenWithSafari:
        action->setIcon(Application::BrowserIcon_Safari());
        break;
    case _OpenWithChrome:
        action->setIcon(Application::BrowserIcon_Chrome());
        break;
    case _OpenWithSleipnir:
        action->setIcon(Application::BrowserIcon_Sleipnir());
        break;
    case _OpenWithVivaldi:
        action->setIcon(Application::BrowserIcon_Vivaldi());
        break;
    case _OpenWithCustom:
        action->setIcon(Application::BrowserIcon_Custom());
        action->setText(tr("OpenWith%1").arg(Application::BrowserPath_Custom().split("/").last().replace(".exe", "")));
        break;
    default: break;
    }
    return action;
}

bool TreeBank::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return false;
    QString str = m_KeyMap[seq];
    if(str.isEmpty()) return false;

    return TriggerAction(str);
}

bool TreeBank::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = m_KeyMap[seq]; // sequence => action
    if(str.isEmpty()) return false;

    return TriggerAction(str);
}

void TreeBank::DeleteView(View *view){
#ifdef TRIDENTVIEW
    if(TridentView *w = qobject_cast<TridentView*>(view->base()))
        w->Clear();
#endif
    view->DeleteLater();
}

SharedView TreeBank::CreateView(QNetworkRequest req, HistNode *hn, ViewNode *vn){
    MainWindow *win = Application::GetCurrentWindow();
    TreeBank *tb = win ? win->GetTreeBank() : 0;
    QStringList set = GetNodeSettings(vn);
    QString id = GetNetworkSpaceId(vn);

    // for predicate.
    QUrl url = req.url();
    QString urlstr = url.toString();

    SharedView view = SharedView();

#ifdef TRIDENTVIEW
    if(set.indexOf(QRegularExpression(QStringLiteral("\\A[tT](?:rident)?(?:[vV](?:iew)?)?\\Z"))) != -1){
        TridentView::SetFeatureControl();
    }
#endif

    view = SharedView(
#ifdef LocalView
        (urlstr.startsWith(QStringLiteral("file:///")) && (QFileInfo(url.toLocalFile()).isDir() || LocalView::IsSupported(url))) ?
          new LocalView(tb, id, set) :
#endif
#ifdef WEBENGINEVIEW
        set.indexOf(QRegularExpression(QStringLiteral("\\A"                 VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new WebEngineView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[gG](?:raphics)?" VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new WebEngineView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[qQ](?:uick)?"    VV"[wW](?:eb)?"                    VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new QuickWebEngineView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A"                 VV"[wW](?:eb)?" VV"[eE](?:ngine)?" VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new WebEngineView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[gG](?:raphics)?" VV"[wW](?:eb)?" VV"[eE](?:ngine)?" VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new WebEngineView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[qQ](?:uick)?"    VV"[wW](?:eb)?" VV"[eE](?:ngine)?" VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new QuickWebEngineView(tb, id, set) :
#endif
#ifdef WEBKITVIEW
        set.indexOf(QRegularExpression(QStringLiteral("\\A"                 VV"[wW](?:eb)?" VV"[kK](?:it)?"    VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new WebKitView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[gG](?:raphics)?" VV"[wW](?:eb)?" VV"[kK](?:it)?"    VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new GraphicsWebKitView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[qQ](?:uick)?"    VV"[wW](?:eb)?" VV"[kK](?:it)?"    VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new QuickWebKitView(tb, id, set) :
#endif
#ifdef NATIVEWEBVIEW
        set.indexOf(QRegularExpression(QStringLiteral("\\A"                 VV"[nN](?:ative)?" VV"[wW](?:eb)?" VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new QuickNativeWebView(tb, id, set) :
        set.indexOf(QRegularExpression(QStringLiteral("\\A[qQ](?:uick)?"    VV"[nN](?:ative)?" VV"[wW](?:eb)?" VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new QuickNativeWebView(tb, id, set) :
#endif
#ifdef TRIDENTVIEW
        set.indexOf(QRegularExpression(QStringLiteral("\\A"                 VV"[tT](?:rident)?"                VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new TridentView(tb, id, set) :
#endif
#ifdef LocalView
        set.indexOf(QRegularExpression(QStringLiteral("\\A"                 VV"[lL](?:ocal)?"                  VV"(?:[vV](?:iew)?)?\\Z"))) != -1 ?
          new LocalView(tb, id, set) :
#endif
#if defined(WEBENGINEVIEW)
        static_cast<View*>(new WebEngineView(tb, id, set))
#elif defined(WEBKITVIEW)
        static_cast<View*>(new WebKitView(tb, id, set))
#elif defined(TRIDENTVIEW)
        static_cast<View*>(new TridentView(tb, id, set))
#elif defined(NATIVEWEBVIEW)
        static_cast<View*>(new QuickNativeWebView(tb, id, set))
#else
        static_cast<View*>(0)
#endif
        , &DeleteView);

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
