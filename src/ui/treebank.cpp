#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "keymap.hpp"
#include "mousemap.hpp"

#include "treebank.hpp"

#include <QMenu>
#include <QThread>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QOpenGLWidget>
#include <QFile>
#include <QDir>
#include <QImageReader>
#include <QFileInfo>
#include <QStyle>
#include <QTimer>

#include <memory>

#include <QGraphicsRotation>
#include <QGraphicsScale>

#include "application.hpp"
#include "treeserializer.hpp"
#include "fileexchange.hpp"
#include "inputmap.hpp"
#include "networkcontroller.hpp"
#include "saver.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "minimap.hpp"
#include "view.hpp"
#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "quicknativewebview.hpp"
#include "edgewebview.hpp"
#include "webenginepage.hpp"
#include "gadgets.hpp"
#include "mainwindow.hpp"
#include "sidepanels.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "localview.hpp"
#include "jsobject.hpp"
#include "dialog.hpp"

QString TreeBank::m_RootName = QString();
ViewNode *TreeBank::m_ViewRoot  = nullptr;
ViewNode *TreeBank::m_TrashRoot = nullptr;

ViewNode *TreeBank::m_ViewIterForward  = nullptr;
ViewNode *TreeBank::m_ViewIterBackward = nullptr;

int TreeBank::ChangeScope::m_Depth = 0;
bool TreeBank::m_TraverseAllView = false;
bool TreeBank::m_PurgeNotifier = false;
bool TreeBank::m_PurgeReceiver = false;
bool TreeBank::m_PurgeView = false;
bool TreeBank::m_EnableMiniMap = false;
int  TreeBank::m_MaxViewCount = 0;
int  TreeBank::m_MaxTrashEntryCount = 0;
int  TreeBank::m_TraverseCondition = 0;

TreeBank::Viewport     TreeBank::m_Viewport = Widget;
SharedViewList TreeBank::m_AllViews = SharedViewList();
SharedViewList TreeBank::m_ViewUpdateBox = SharedViewList();
NodeList TreeBank::m_NodeDeleteBox = NodeList();

QMap<QKeySequence, QString> TreeBank::m_KeyMap = QMap<QKeySequence, QString>();
QMap<QString, QString> TreeBank::m_MouseMap = QMap<QString, QString>();

static SharedView LoadWithLink(QNetworkRequest req, ViewNode *vn);
static SharedView LoadWithLink(ViewNode *vn);
static SharedView AutoLoadWithLink(QNetworkRequest req, ViewNode *vn);
static SharedView AutoLoadWithLink(ViewNode *vn);
static void SetViewProp(QUrl url, ViewNode *vn, SharedView view = nullptr);

static QString GetNetworkSpaceId(ViewNode*);
static QStringList GetNodeSettings(ViewNode*);

TreeBank::TreeBank(QWidget *parent)
    : QWidget(parent)
    , m_Scene(new QGraphicsScene(this))
    , m_View(new GraphicsView(m_Scene, this))
    , m_Notifier(new Notifier(this, m_PurgeNotifier || m_PurgeView))
    , m_Receiver(new Receiver(this, m_PurgeReceiver || m_PurgeView))
    , m_JsObject(new _Vanilla(this))
{
    m_MiniMap = (m_EnableMiniMap && !m_PurgeView) ? new MiniMap(this) : nullptr;
    m_Gadgets_ = SharedView(new Gadgets(this), &DeleteView);
    m_Gadgets_->SetThis(WeakView(m_Gadgets_));
    m_Gadgets = static_cast<Gadgets*>(m_Gadgets_.get());

    if(m_Viewport == GLWidget){
        m_View->setViewport(new QOpenGLWidget());

    } else if(m_Viewport == OpenGLWidget){
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
    m_CurrentViewNode = nullptr;
    m_ActionTable = QMap<TreeBankAction, QAction*>();

    connect(m_Gadgets, &Gadgets::titleChanged,
            GetMainWindow(), &MainWindow::SetWindowTitle);

    connect(this, &TreeBank::TreeStructureChanged, m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::NodeCreated,          m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::NodeDeleted,          m_Gadgets, &Gadgets::ThumbList_RefreshNoScroll);
    connect(this, &TreeBank::FoldedChanged,        m_Gadgets, &Gadgets::OnFoldedChanged);
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

    m_ViewRoot  = new ViewNode();
    m_TrashRoot = new ViewNode();
    m_ViewRoot->SetTitle(m_RootName);
    m_TrashRoot->SetTitle(QStringLiteral("trash;noload"));
}

namespace {
    ViewNode *FindSerial(ViewNode *parent, quint64 serial){
        foreach(Node *child, parent->GetChildren()){
            ViewNode *vn = child->ToViewNode();
            if(!vn) continue;
            if(vn->IsDirectory()){
                if(ViewNode *found = FindSerial(vn, serial)) return found;
            }
            else if(vn->GetSerial() == serial) return vn;
        }
        return nullptr;
    }
}

ViewNode *TreeBank::TabOfSerial(quint64 serial){
    return m_ViewRoot && serial ? FindSerial(m_ViewRoot, serial) : nullptr;
}

namespace {
    std::function<void()> &ToldOfChanges(){
        static std::function<void()> told;
        return told;
    }
}

void TreeBank::WhenSomethingChanges(std::function<void()> told){
    Q_ASSERT(!ToldOfChanges());
    ToldOfChanges() = told;
}

void TreeBank::SomethingChanged(){
    if(ToldOfChanges()) ToldOfChanges()();
}

void TreeBank::EmitTreeStructureChanged(){
    SomethingChanged();
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->TreeStructureChanged();
    }
}

void TreeBank::EmitNodeCreated(NodeList &nds){
    SomethingChanged();
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->NodeCreated(nds);
    }
}

void TreeBank::EmitNodeDeleted(NodeList &nds){
    SomethingChanged();
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->NodeDeleted(nds);
    }
}

void TreeBank::ForgetNodeItems(NodeList &nds){
    foreach(MainWindow *win, Application::GetMainWindows()){
        if(TreeBar *bar = win->GetTreeBar()) bar->ForgetNodes(nds);
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
    const QString before = nd->GetTitle();

    const bool directory = nd->IsViewNode() && nd->IsDirectory();
    const QStringList tokens = directory ? DirectoryPage::TitleTokens(before)
                                         : QStringList();

    bool ok;
    const QString name = ModalDialog::GetText
        (tr("Input new node name."),
         tr("Node name:"),
         directory ? DirectoryPage::TitleName(before) : before, &ok);

    if(!ok) return false;

    const bool nameless = directory && !nd->IsRoot() &&
        !DirectoryPage::SaysId(before) &&
        !DirectoryPage::NameSpellsId(DirectoryPage::TitleName(before));

    if((name.isEmpty() && !nameless) ||
       name.contains(QRegularExpression(QStringLiteral("[<>\":\\?\\|\\*/\\\\]")))){

        ModelessDialog::Information
            (tr("Invalid node name."),
             tr("Cannot change to empty title, and cannot use following charactor.") +
             QStringLiteral("\n") +
             QStringList({QStringLiteral("\\"), QStringLiteral("/"), QStringLiteral(":"),
                          QStringLiteral("*"), QStringLiteral("?"), QStringLiteral("\""),
                          QStringLiteral("<"), QStringLiteral(">"), QStringLiteral("|")})
             .join(QChar(0x3000)));
        return false;
    }

    if(directory && name.contains(QLatin1Char(';'))){

        ModelessDialog::Information
            (tr("Invalid directory name."),
             tr("A directory name cannot contain ';'. What comes after it is "
                "this directory's settings, which are edited in the "
                "DirectorySettings menu."));
        return false;
    }

    const QString after = directory ? DirectoryPage::ComposeTitle(name, tokens)
                                    : name;

    if(before == after) return false;

    nd->SetTitle(after);

    if(directory)
        ReconfigureDirectory(nd->ToViewNode(), before, after);

    return true;
}

void TreeBank::ReconfigureDirectory(ViewNode *vn, QString before, QString after){

    Node *parent = vn->GetParent();
    QString parentid = parent ? GetNetworkSpaceId(parent->ToViewNode()) : QString();
    QString befname = before.split(QStringLiteral(";")).first();
    QString aftname = after .split(QStringLiteral(";")).first();
    QStringList aftset = after .split(QStringLiteral(";")).mid(1);
    bool bef = !parent || DirectoryPage::SaysId(before);
    bool aft = !parent || DirectoryPage::SaysId(after);

    if(vn == m_ViewRoot) m_RootName = after;

    NetworkAccessManager *nam = bef ?
        (aft ? NetworkController::MoveNetworkAccessManager(befname, aftname, aftset) :
               NetworkController::KillNetworkAccessManager(befname)) :
        (aft ? NetworkController::CopyNetworkAccessManager(parentid, aftname, aftset) :
               NetworkController::GetNetworkAccessManager(parentid, aftset));

    if(!nam) nam = NetworkController::GetNetworkAccessManager(parentid, aftset);
    Q_UNUSED(nam)

    foreach(Node *nd, vn->GetChildren()){
        ReapplySpecificSettings(nd->ToViewNode());
    }
}

void TreeBank::ApplySpecificSettings(ViewNode *vn){
    if(!vn) return;

    NetworkController::GetNetworkAccessManager(GetNetworkSpaceId(vn),
                                               GetNodeSettings(vn));
    ReapplySpecificSettings(vn);
}

void TreeBank::ReapplySpecificSettings(ViewNode *vn){
    if(!vn) return;

    if(vn->GetView()){

        vn->GetView()->ApplySpecificSettings(GetNodeSettings(vn));
    }
    foreach(Node *nd, vn->GetChildren()){
        ReapplySpecificSettings(nd->ToViewNode());
    }
}

static QString GetNetworkSpaceId(ViewNode* vn){
    static const QRegularExpression IDENTITY(QStringLiteral("\\A[iI][dD](?:entif(?:y|ier|ication))?\\Z"));
    QString title;
    forever{
        title = vn->GetTitle();
        if(vn == TreeBank::GetViewRoot() || vn == TreeBank::GetTrashRoot() ||
           (vn->IsDirectory() && !title.isEmpty() &&
            title.split(QStringLiteral(";")).indexOf(IDENTITY) != -1)){

            return title.split(QStringLiteral(";")).first();

        } else {
            vn = vn->GetParent()->ToViewNode();
        }
    }
}
static QStringList GetNodeSettings(ViewNode* vn){
    if(!vn) return QStringList();
    QStringList titles;
    forever{
        if(vn->IsDirectory() && !vn->GetTitle().isEmpty())
            titles << vn->GetTitle();
        if(vn->IsRoot() || !vn->GetParent()) break;
        vn = vn->GetParent()->ToViewNode();
        if(!vn) break;
    }
    return DirectoryPage::InheritTokens(titles);
}
QStringList TreeBank::SettingsOf(ViewNode *vn){
    return GetNodeSettings(vn);
}

QString TreeBank::NetworkSpaceOf(ViewNode *vn){
    return vn ? GetNetworkSpaceId(vn) : QString();
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

void TreeBank::AutoLoad(){
    if(!MayChangeFromOutside()) return;
    ChangeScope changing;
    if(!m_MaxViewCount || m_AllViews.length() < m_MaxViewCount){
        if(m_TraverseCondition == 0){
            if(m_ViewIterForward)       LoadViewForward();
            else if(m_ViewIterBackward) LoadViewBackward();
        } else {
            if(m_ViewIterBackward)      LoadViewBackward();
            else if(m_ViewIterForward)  LoadViewForward();
        }
    }

    if(!m_ViewIterForward && !m_ViewIterBackward){
        Application::StopAutoLoadTimer();
    }
    if(m_AllViews.length() >= m_MaxViewCount){
        Application::StopAutoLoadTimer();
    }

    m_TraverseCondition = m_TraverseCondition ? 0 : 1;
}

void TreeBank::WalkToAutoLoad(ViewNode *&iter, bool forward,
                              const std::function<bool(ViewNode*)> &load){
    while(ViewNode *vn = iter){
        if(IsTrash(vn))
            iter = nullptr;
        else
            do iter = vn = forward ? vn->Next() : vn->Prev();
            while(vn && (vn->GetView() || vn->IsDirectory() || vn->GetUrl().isEmpty()));

        if(!vn || vn->GetUrl().isEmpty() || vn->GetView() || load(vn)) return;
    }
}

void TreeBank::LoadViewForward(){
    ChangeScope changing;
    WalkToAutoLoad(m_ViewIterForward, true,
                   [](ViewNode *vn){ return !!AutoLoadWithLink(vn);});
}

void TreeBank::LoadViewBackward(){
    ChangeScope changing;
    WalkToAutoLoad(m_ViewIterBackward, false,
                   [](ViewNode *vn){ return !!AutoLoadWithLink(vn);});
}

Node* TreeBank::GetRoot(Node* nd){
    if(!nd) return nullptr;
    while(nd->GetParent() && !nd->IsRoot())
        nd = nd->GetParent();
    return nd;
}

Node* TreeBank::GetOtherRoot(Node* nd){
    Node *root = GetRoot(nd);
    if(root == m_ViewRoot)  return m_TrashRoot;
    if(root == m_TrashRoot) return m_ViewRoot;
    return nullptr;
}

bool TreeBank::IsTrash(Node* nd){
    if(nd->IsViewNode())
        return GetRoot(nd) == m_TrashRoot;
    return false;
}

bool TreeBank::IsDisplayingTableView(){
    return (m_Gadgets && m_Gadgets->IsActive())
#ifdef LOCALVIEW
        || (m_CurrentView && qobject_cast<LocalView*>(m_CurrentView->base()))
#endif
        ;
}

bool TreeBank::IsCurrent(Node* nd){
    return nd == m_CurrentViewNode;
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

static TreeSerializer::Hooks TreeHooks(){
    TreeSerializer::Hooks hooks;

    hooks.KeepsSideFiles = [](ViewNode *nd){ return !TreeBank::IsTrash(nd);};

    hooks.RestoreIntoWindow = [](ViewNode *nd, int id){
        LoadWithLink(nd);

        MainWindow *win = Application::NewWindow(id);
        TreeBank *tb = win->GetTreeBank();
        tb->blockSignals(true);
        tb->SetCurrent(nd);
        tb->blockSignals(false);
    };

    hooks.Tick = [](){ Application::processEvents();};

    return hooks;
}

static TreeSerializer::Hooks TreeSaveHooks(const TreeBank::WindowIndexMap &windowIndices){
    TreeSerializer::Hooks hooks;
    hooks.WindowIndexOf = [windowIndices](ViewNode *nd){
        return windowIndices.value(nd, 0);
    };
    hooks.KeepsSideFiles = [](ViewNode *nd){ return !TreeBank::IsTrash(nd);};
    return hooks;
}

void TreeBank::LoadTree(){
    ChangeScope changing;
    Node::SetBooting(true);

    QString datadir = Application::StateDirectory();
    TreeSerializer::Hooks hooks = TreeHooks();

    QMap<ViewNode*, QString> map;
    map[m_ViewRoot]  = Application::PrimaryTreeFileName();
    map[m_TrashRoot] = Application::SecondaryTreeFileName();
    foreach(ViewNode *root, QList<ViewNode*>() << m_ViewRoot << m_TrashRoot){

        QString filename = map[root];
        bool check = TreeSerializer::ReadJsonFile(datadir + filename, root, hooks);

        const QString previous = datadir + filename + QStringLiteral(".prev");
        if(!check && QFile::exists(previous)){
            check = TreeSerializer::ReadJsonFile(previous, root, hooks);
            if(check){
                const QString backup = filename + QStringLiteral(".prev");
                ModelessDialog::Information
                    (tr("Restored from a back up file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
                     tr("Because of a failure to read the latest file, it was restored from a backup file."));
            }
        }

        if(!check){

            QDir dir = QDir(datadir);
            QStringList list =
                dir.entryList(Application::BackUpFileFilters(),
                              QDir::NoFilter, QDir::Name | QDir::Reversed);

            foreach(QString backup, list){

                if(!backup.endsWith(filename)) continue;
                check = TreeSerializer::ReadJsonFile(datadir + backup, root, hooks);

                if(!check) continue;

                ModelessDialog::Information
                    (tr("Restored from a back up file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
                     tr("Because of a failure to read the latest file, it was restored from a backup file."));
                break;
            }
        }

        if(root == m_ViewRoot)
            EmitTreeStructureChanged();
    }
    Node::SetBooting(false);
}

void TreeBank::UpdateCurrentThumbnails(){
    foreach(MainWindow *win, Application::GetMainWindows().values()){
        if(TreeBank *tb = win->GetTreeBank()){
            if(SharedView view = tb->GetCurrentView()){
                if(view->visible()) view->UpdateThumbnail();
            }
        }
    }
}

TreeBank::WindowIndexMap TreeBank::WindowIndexSnapshot(){
    WindowIndexMap snapshot;
    const WinMap windows = Application::GetMainWindows();
    for(auto it = windows.constBegin(); it != windows.constEnd(); ++it){
        MainWindow *window = it.value();
        TreeBank *bank = window ? window->GetTreeBank() : nullptr;
        ViewNode *current = bank ? bank->GetCurrentViewNode() : nullptr;
        if(current) snapshot[current] = it.key();
    }
    return snapshot;
}

bool TreeBank::SaveTree(const WindowIndexMap &windowIndices){

    QString datadir = Application::StateDirectory();
    TreeSerializer::Hooks hooks = TreeSaveHooks(windowIndices);

    QString primary  = datadir + Application::PrimaryTreeFileName(false);
    QString primaryb = datadir + Application::PrimaryTreeFileName(true);

    QString secondary  = datadir + Application::SecondaryTreeFileName(false);
    QString secondaryb = datadir + Application::SecondaryTreeFileName(true);

    if(QFile::exists(primaryb))   QFile::remove(primaryb);
    if(QFile::exists(secondaryb)) QFile::remove(secondaryb);

    QMap<ViewNode*, QString> map;
    map[m_ViewRoot]  = Application::PrimaryTreeFileName(true);
    map[m_TrashRoot] = Application::SecondaryTreeFileName(true);
    QMap<ViewNode*, bool> written;
    foreach(ViewNode *root, QList<ViewNode*>() << m_ViewRoot << m_TrashRoot){

        QString filename = map[root];

        written[root] = TreeSerializer::WriteJsonFile(datadir + filename, root, hooks);
    }

    const bool primarySaved = written[m_ViewRoot] && FileExchange::Replace(primaryb, primary);
    const bool secondarySaved = written[m_TrashRoot] && FileExchange::Replace(secondaryb, secondary);

    return primarySaved && secondarySaved;
}

#ifdef MEDIATIME
void TreeBank::SaveMediaTimesForQuit(VoidCallBack finished){
    static const int FinalMediaTimeTimeout = 2000;

    std::shared_ptr<SharedViewList> views =
        std::make_shared<SharedViewList>(m_AllViews);
    std::shared_ptr<CompletionBarrier> barrier =
        std::make_shared<CompletionBarrier>([views, finished](){
            if(finished) finished();
        });

    foreach(SharedView view, *views){
        barrier->Add();
        view->SaveMediaTime([barrier](){ barrier->Complete();});
    }
    barrier->Seal();

    QTimer::singleShot(FinalMediaTimeTimeout, Application::GetInstance(),
                       [barrier](){ barrier->Expire();});
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
    m_EnableMiniMap        = s.value(QStringLiteral("application/@EnableMiniMap")        , false).value<bool>();

    QString viewport = s.value(QStringLiteral("application/@Viewport"), QStringLiteral("Widget")).value<QString>();
    if(viewport == QStringLiteral("Widget"))       m_Viewport = Widget;
    if(viewport == QStringLiteral("GLWidget"))     m_Viewport = GLWidget;
    if(viewport == QStringLiteral("OpenGLWidget")) m_Viewport = OpenGLWidget;

    if(m_MaxViewCount == -1)       m_MaxViewCount       = s.value(QStringLiteral("application/@MaxView")  , -1).value<int>();
    if(m_MaxViewCount == -1)       m_MaxViewCount       = 10;
    if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = s.value(QStringLiteral("application/@MaxTrash") , -1).value<int>();
    if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = 100;

    {
        TREEBANK_KEYMAP
        InputMap::LoadKeyMap(s, QStringLiteral("application/keymap"), m_KeyMap, InputMap::Hooks(IsValidAction));
    }
    {
        TREEBANK_MOUSEMAP
        InputMap::LoadGestureMap(s, QStringLiteral("application/mouse"), m_MouseMap, InputMap::Hooks(Page::IsValidAction));
    }
    Node::LoadSettings();
    View::LoadSettings();
    Gadgets::LoadSettings();
#ifdef LOCALVIEW
    LocalView::LoadSettings();
#endif

    foreach(SharedView view, m_AllViews){
        ViewNode *vn = view->GetViewNode();
        view->ApplySpecificSettings(vn ? GetNodeSettings(vn) : view->SpecificSettings());
    }
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
    s.setValue(QStringLiteral("application/@EnableMiniMap"),        m_EnableMiniMap);

    if(m_Viewport == Widget)       s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("Widget"));
    if(m_Viewport == GLWidget)     s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("GLWidget"));
    if(m_Viewport == OpenGLWidget) s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("OpenGLWidget"));

    InputMap::SaveKeyMap(s, QStringLiteral("application/keymap"), m_KeyMap);

    InputMap::SaveGestureMap(s, QStringLiteral("application/mouse"), m_MouseMap);
    Node::SaveSettings();
    View::SaveSettings();
    Gadgets::SaveSettings();
}

void TreeBank::QuarantineViewNode(ViewNode *vn){
    TreeBank *tb = vn->GetView() ? vn->GetView()->GetTreeBank() : nullptr;
    if(!tb) return;

    if(vn == tb->GetCurrentViewNode())  tb->SetCurrentViewNode(nullptr);
    if(vn == tb->GetViewIterForward())  tb->SetViewIterForward(nullptr);
    if(vn == tb->GetViewIterBackward()) tb->SetViewIterBackward(nullptr);
}

void TreeBank::DisownNode(Node *nd){
    nd->GetParent()->RemoveChild(nd);
    if(nd->IsPrimaryOfParent()){
        if(nd->HasNoSiblings())
            nd->GetParent()->SetPrimary(nullptr);
        else
            nd->GetParent()->SetPrimary(nd->GetFirstSibling());
    }
}

void TreeBank::RebuildViewForOffTheRecord(ViewNode *vn){
    ChangeScope changing;
    if(!vn || !vn->GetView()) return;
    TreeBank *tb = vn->GetView()->GetTreeBank();
    if(tb && tb->GetCurrentViewNode() == vn)
        tb->Recreate(vn);
    else
        DislinkView(vn);
}

bool TreeBank::MayChangeFromOutside(){
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    return ChangeScope::Depth() == 0
        && !qobject_cast<QMenu*>(QApplication::activePopupWidget()) && !QApplication::activeModalWidget()
        && !ModalDialog::AnyRunning() && !ModalDialog::InFileDialog()
        && !View::IsDraggingOut();
}

void TreeBank::WhenItMayChange(std::function<void()> action, int milliseconds){
    QTimer::singleShot(milliseconds, qApp, [action](){
        if(!MayChangeFromOutside()){ WhenItMayChange(action, 50); return;}
        action();
    });
}

bool TreeBank::IsLive(TreeBank *bank){
    foreach(MainWindow *window, Application::GetMainWindows())
        if(bank && window && window->GetTreeBank() == bank) return true;
    return false;
}

void TreeBank::DislinkView(ViewNode *vn){
    Q_ASSERT(ChangeScope::Depth() > 0);
    if(View *view = vn->GetView()){
        if(SharedView v = view->GetThis().lock()){

            TreeBank *tb = v->GetTreeBank();
            ToolBar *bar = tb ? tb->GetMainWindow()->GetToolBar() : nullptr;

            if(tb && v == tb->GetCurrentView()){
                v->Disconnect(tb);
                tb->SetCurrentView(nullptr);
                if(bar && bar->isVisible())
                    bar->Disconnect(v);
            }
            vn->SetView(nullptr);

            ReleaseView(v);
        }
    }
}

bool TreeBank::MoveToTrash(ViewNode *vn){
    if(vn->IsRoot()) return false;

    if(GetRoot(vn) == m_TrashRoot) return false;

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
        DislinkView(vn);
    }
    foreach(Node *child, nd->GetChildren()){
        StripSubTree(child);
    }
}

void TreeBank::ReleaseView(SharedView view){
    if(view->GetDisplayObscured()){
        view->ExitFullScreen();
        view->SetDisplayObscured(false);
        if(TreeBank *tb = view->GetTreeBank())
            if(MainWindow *win = tb->GetMainWindow())
                win->SetFullScreen(false);
    }
    RemoveFromAllViews(view);
    RemoveFromUpdateBox(view);
}

void TreeBank::ReleaseAllView(){
    ChangeScope changing;

    m_ViewUpdateBox.clear();

    foreach(SharedView view, m_AllViews){
        view->DeleteLater();
    }
    m_AllViews.clear();
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

static Node *ResolveCurrentAfterDelete(Node *nd, ViewNode *leaving,
                                       const std::function<bool(Node*)> &refused,
                                       int depth){
    if(!nd) return nullptr;
    if(depth > 64) return nullptr;
    if(leaving && nd == leaving) return nullptr;

    if(!nd->HoldsView()){
        if(nd->HasNoChildren()) return nullptr;
        if(nd->GetPrimary())
            return ResolveCurrentAfterDelete(nd->GetPrimary(), leaving, refused, depth + 1);
        return ResolveCurrentAfterDelete(nd->GetFirstChild(), leaving, refused, depth + 1);
    }
    if(refused && refused(nd)) return nullptr;
    return nd;
}

Node *TreeBank::ChooseCurrentAfterDelete(const NodeList &views,
                                         Node *prevparent,
                                         ViewNode *leaving,
                                         const std::function<bool(Node*)> &refused){
    foreach(Node *nd, views){
        if(!nd || !prevparent || nd->GetParent() != prevparent) continue;
        if(Node *found = ResolveCurrentAfterDelete(nd, leaving, refused, 0))
            return found;
    }

    if(prevparent){
        NodeList sorted = prevparent->GetChildren();
        std::sort(sorted.begin(), sorted.end(), [](Node *n1, Node *n2){
            return n1->GetLastAccessDate() > n2->GetLastAccessDate();
        });
        foreach(Node *nd, sorted){
            if(nd->IsDirectory()) continue;
            if(Node *found = ResolveCurrentAfterDelete(nd, leaving, refused, 0))
                return found;
        }
    }

    foreach(Node *nd, views){
        if(Node *found = ResolveCurrentAfterDelete(nd, leaving, refused, 0))
            return found;
    }
    return ResolveCurrentAfterDelete(m_ViewRoot, leaving, refused, 0);
}

ViewNode *TreeBank::FindCurrentAfterDelete(Node *prevparent, ViewNode *leaving){
    NodeList views;
    foreach(SharedView view, m_AllViews)
        if(view && view->GetViewNode()) views << view->GetViewNode();

    const int here = WinIndex();
    Node *found = ChooseCurrentAfterDelete
        (views, prevparent, leaving,
         [here](Node *nd) -> bool {
             if(!nd->GetView()) return false;
             QList<int> ids = Application::GetMainWindows().keys();
             if(ids.length() <= 1) return false;
             if(!here) return false;
             ids.removeOne(here);
             return ids.contains(WinIndex(nd->GetView()->GetThis().lock()));
         });
    return found ? found->ToViewNode() : nullptr;
}

bool TreeBank::SelectCurrentAfterDelete(Node *prevparent){
    foreach(SharedView view, m_AllViews){
        if(prevparent == view->GetViewNode()->GetParent())
            if(SetCurrent(view->GetViewNode()))
                return true;
    }

    {   NodeList sorted = prevparent->GetChildren();
        std::sort(sorted.begin(), sorted.end(), [](Node *n1, Node *n2){
            return n1->GetLastAccessDate() > n2->GetLastAccessDate();
        });
        foreach(Node *nd, sorted){
            if(!nd->IsDirectory())
                if(SetCurrent(nd))
                    return true;
        }
    }

    foreach(SharedView view, m_AllViews){
        if(SetCurrent(view))
            return true;
    }
    return SetCurrent(m_ViewRoot);
}

SharedView TreeBank::ExtractDownloadCarrier(ViewNode *vn){
    ChangeScope changing;
    if(!vn) return SharedView();
    View *view = vn->GetView();
    if(!view) return SharedView();

    SharedView held = view->GetThis().lock();
    if(!held) return SharedView();

    Node *prevparent = vn->GetParent();
    if(!prevparent) return SharedView();

    if(!vn->HasNoChildren()) return SharedView();

    ViewNode *next = FindCurrentAfterDelete(prevparent, vn);
    if(!next) return SharedView();

    QuarantineViewNode(vn);
    DislinkView(vn);

    held->Orphan();

    DisownNode(vn);
    NodeList deleted = NodeList() << vn;
    EmitNodeDeleted(deleted);

    ForgetNodeItems(deleted);

    if(Application::EnableAutoSave()){
        AddToDeleteBox(vn);
    } else {
        vn->Delete();
    }

    SetCurrent(next);
    return held;
}

bool TreeBank::DeleteNode(Node *nd){
    if(!nd) return false;
    return DeleteNode(NodeList() << nd);
}

bool TreeBank::DeleteNode(NodeList list){
    ChangeScope changing;
    if(list.isEmpty()) return false;

    Application::RestartAutoLoadTimer();

    Node *sample = list.first();
    Node *prevparent  = sample->GetParent();

    if(sample->IsViewNode()){
        bool deleted = false;

        foreach(Node *nd, list){
            Q_ASSERT(nd->IsViewNode());
            deleted = MoveToTrash(nd->ToViewNode()) || deleted;
        }
        if(!deleted) return false;

    } else {
        return false;
    }

    if(!m_CurrentView){
        if(SelectCurrentAfterDelete(prevparent)) return true;
    }
    return true;
}

bool TreeBank::MoveNode(Node *nd, Node *dir, int n){
    if(nd->GetParent() == dir){
        dir->MoveChild(dir->ChildrenIndexOf(nd),
                       (n < 0 || n >= dir->ChildrenLength()) ?
                         dir->ChildrenLength() - 1 :
                       (n > dir->ChildrenIndexOf(nd)) ?
                         n - 1 : n);
        EmitTreeStructureChanged();
    } else {
        bool wasTrash = IsTrash(nd);
        DisownNode(nd);
        nd->SetParent(dir);
        dir->InsertChild((n < 0 || n > dir->ChildrenLength()) ?
                           dir->ChildrenLength() : n,
                         nd);

        bool isTrash = IsTrash(nd);

        if(nd->IsViewNode() && !isTrash)
            ApplySpecificSettings(nd->ToViewNode());

        if(wasTrash && !isTrash) EmitNodeCreated(NodeList() << nd);
        if(!wasTrash && isTrash) EmitNodeDeleted(NodeList() << nd);
    }
    return true;
}

bool TreeBank::SetChildrenOrder(Node *parent, NodeList children){
    if(children.isEmpty() || !parent ||
       children.length() < parent->ChildrenLength() ||
       children.contains(parent))
        return false;

    NodeList ancestors = parent->GetAncestors();
    if(QSet<Node*>(children.begin(), children.end()).intersects(QSet<Node*>(ancestors.begin(), ancestors.end())))
        return false;

    parent->ClearChildren();
    parent->SetChildren(children);

    foreach(Node *child, children){
        if(child->GetParent() && child->GetParent() != parent){
            DisownNode(child);
            child->SetParent(parent);

            if(child->IsViewNode())
                ApplySpecificSettings(child->ToViewNode());
        }
    }
    EmitTreeStructureChanged();
    return true;
}

bool TreeBank::SetCurrent(Node *nd){
    ChangeScope changing;
    if(!nd) return false;

    if(!nd->HoldsView()){
        if(nd->HasNoChildren()){
            return false;
        } else if(nd->GetPrimary()){
            return SetCurrent(nd->GetPrimary());
        } else {
            return SetCurrent(nd->GetFirstChild());
        }
    }

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

    if(m_Receiver) m_Receiver->hide();
    if(m_Notifier) m_Notifier->ResetStatus();

    if(m_Gadgets && m_Gadgets->IsActive()) m_Gadgets->Dismiss();

    SharedView prev = m_CurrentView;

    m_CurrentView = nd->GetView() ? nd->GetView()->GetThis().lock() : SharedView();
    m_CurrentViewNode = nd->ToViewNode();

    m_CurrentViewNode->ResetPrimaryPath();

    m_ViewIterForward = m_ViewIterBackward = m_CurrentViewNode;

    if(!m_CurrentView){
        m_CurrentView = LoadWithLink(m_CurrentViewNode);

        RaiseDisplayedViewPriority();
    }
    Q_ASSERT(m_CurrentView);

    if(prev != m_CurrentView){
        ToolBar *bar = GetMainWindow()->GetToolBar();
        if(prev){
            prev->UpdateThumbnail();
            RemoveFromUpdateBox(prev);
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

    if(m_MiniMap)
        m_MiniMap->SetView(m_CurrentView.get());

    const QSize viewSize = ViewSize();
    ResizeViewArea(viewSize);
    if(m_CurrentView->size() != viewSize || TreeBank::PurgeView())
        m_CurrentView->resize(TreeBank::PurgeView() ? size() : viewSize);

    if(!m_CurrentView->GetTitle().isEmpty()){
        GetMainWindow()->SetWindowTitle(m_CurrentView->GetTitle());
    } else if(!m_CurrentView->url().isEmpty()){
        GetMainWindow()->SetWindowTitle(m_CurrentView->url().toString());
    }

#ifdef LOCALVIEW
    if(SharedView v = m_CurrentView->GetSlave().lock()){
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
    if(m_MiniMap) m_MiniMap->ResizeNotify(size());

    if(m_Gadgets && m_Gadgets->IsActive()){
        DoUpdate();
    } else {
        DoUpdate();
        m_CurrentView->show();

#ifdef LOCALVIEW
        if(qobject_cast<LocalView*>(m_CurrentView->base())){
            if(prev && prev != m_CurrentView){
                m_CurrentView->SetMaster(prev);
                prev->SetSlave(m_CurrentView);
                prev->OnBeforeStartingDisplayGadgets();
            }
        } else
#endif
        {
            if(prev && prev != m_CurrentView){
                prev->lower();
                prev->hide();
            }
        }
    }

    if(PurgeView()){
        parentWidget()->raise();
        m_CurrentView->raise();
    }
    GetMainWindow()->SetInspectorPane(m_CurrentView->InspectorPane());
    if(SidePanels *panels = GetMainWindow()->GetSidePanels()) panels->Update();

    GetMainWindow()->RaiseAllEdgeWidgets();
    RestackChildWidgets();

    if(m_Gadgets && !m_Gadgets->IsActive() &&
       !GetMainWindow()->GetTreeBar()->TabWindowVisible()){

        QTimer::singleShot(0, this, [this](){
            GetMainWindow()->ActivateIfNeeded();
            if(TreeBank::PurgeView())
                if(QWidget *w = qobject_cast<QWidget*>(m_CurrentView->base()))
                    MainWindow::ActivateWindowIfNeeded(w);
            m_CurrentView->setFocus();
        });
    }

    AddToUpdateBox(m_CurrentView);

    emit CurrentChanged(m_CurrentViewNode);
    SomethingChanged();
    return true;
}

bool TreeBank::SetCurrent(SharedView view){
    return SetCurrent(view->GetViewNode());
}

void TreeBank::NthView(int n, ViewNode *vn){
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    NodeList siblings = vn->GetSiblings();
    if(n >= 0 && n < siblings.length())
        SetCurrent(siblings[n]);
}

void TreeBank::GoBackOrCloseForDownload(View *view){
    if(!view) return;

    WeakView weak = view->GetThis();
    view->CallWithWholeHtml([weak](const QString &html){

        SharedView v = weak.lock();
        if(!v) return;
        ViewNode *vn = v->GetViewNode();
        if(!vn || vn->GetView() != v.get() || !v->GetTreeBank()) return;

        if(html.isEmpty() || html == EMPTY_FRAME_HTML ||
           (html.length() < 1000 && html.endsWith(QStringLiteral("</head></html>")))){

            if(v->CanGoBack() && !v->CanGoForward()){

                v->TriggerNativeGoBackAction();

            } else if(!v->CanGoBack() && !v->CanGoForward()){

                v->TriggerAction(Page::_Close);
            }
        }
    });
}

void TreeBank::BeforeStartingDisplayGadgets(){
    DoUpdate();

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

    RestackChildWidgets(OverviewUp);
    m_View->viewport()->repaint();
    m_View->setFocus();
}

void TreeBank::AfterFinishingDisplayGadgets(){
    if(m_Gadgets && m_Gadgets->IsActive())
        m_Gadgets->Disconnect(this);

    if(m_CurrentView){
        m_CurrentView->Connect(this);

        if(!m_CurrentView->GetTitle().isEmpty())
            GetMainWindow()->SetWindowTitle(m_CurrentView->GetTitle());
        else if(!m_CurrentView->url().isEmpty())
            GetMainWindow()->SetWindowTitle(m_CurrentView->url().toString());
    }

    if(m_CurrentView && m_CurrentView->size() != ViewSize() && !TreeBank::PurgeView())
        m_CurrentView->resize(ViewSize());

    if(m_CurrentView)
        m_CurrentView->OnAfterFinishingDisplayGadgets();

    RestackChildWidgets(OverviewDown);
}

void TreeBank::MousePressEvent(QMouseEvent *ev){
    QWidget::mousePressEvent(ev);
    m_View->MousePressEvent(ev);
}

void TreeBank::MouseReleaseEvent(QMouseEvent *ev){
    QWidget::mouseReleaseEvent(ev);
    m_View->MouseReleaseEvent(ev);
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

SharedView TreeBank::OpenInNewViewNode(QNetworkRequest req, bool activate, ViewNode *older){
    ChangeScope changing;
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    if(!older) older = m_CurrentViewNode;
    if(!older){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return nullptr;
    }
    ViewNode *young = older->MakeSibling();
    young->SetHoldView(true);
    young->SetZoom(older->GetZoom());
    SharedView view = LoadWithLink(req, young);
    EmitNodeCreated(NodeList() << young);
    if(activate){
        SetCurrent(young);
    } else {
        view->hide();
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenInNewViewNode(QUrl url, bool activate, ViewNode *older){
    return OpenInNewViewNode(QNetworkRequest(url), activate, older);
}

SharedView TreeBank::OpenInNewViewNode(QList<QNetworkRequest> reqs, bool activate, ViewNode *older){
    ChangeScope changing;
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = reqs.length()-1; i >= 0; i--){
        bool first = (i == reqs.length()-1);
        SharedView view = OpenInNewViewNode(reqs[i], false, older);
        if(!view){
            View::SetSwitchingState(false);
            return nullptr;
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

SharedView TreeBank::OpenOnSuitableNode(QNetworkRequest req, bool activate, ViewNode *parent, int position){
    ChangeScope changing;
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    if(!parent) parent = m_ViewRoot;
    if(!parent){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return nullptr;
    }
    ViewNode *page = parent->MakeChild(position);
    page->SetHoldView(true);
    SharedView view = LoadWithLink(req, page);
    EmitNodeCreated(NodeList() << page);
    if(activate){
        SetCurrent(page);
    } else {
        view->hide();
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenOnSuitableNode(QUrl url, bool activate, ViewNode *parent, int position){
    return OpenOnSuitableNode(QNetworkRequest(url), activate, parent, position);
}

SharedView TreeBank::OpenOnSuitableNode(QList<QNetworkRequest> reqs, bool activate, ViewNode *parent, int position){
    ChangeScope changing;
    View::SetSwitchingState(true);

    SharedView v = SharedView();
    for(int i = 0; i < reqs.length(); i++){
        bool last = (i == (reqs.length() - 1));
        SharedView view = OpenOnSuitableNode(reqs[i], last && activate, parent, position);
        if(!view){
            View::SetSwitchingState(false);
            return nullptr;
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

SharedView TreeBank::OpenInNewDirectory(QNetworkRequest req, bool activate, ViewNode *older){
    ChangeScope changing;
    bool had_been_switching = View::GetSwitchingState();
    if(!had_been_switching) View::SetSwitchingState(true);

    if(!older) older = m_CurrentViewNode;
    if(!older){
        if(!had_been_switching)
            View::SetSwitchingState(false);
        return nullptr;
    }
    ViewNode *young  = older->NewDir();
    young->SetHoldView(true);
    SharedView view = LoadWithLink(req, young);
    EmitNodeCreated(NodeList() << young->GetParent() << young);
    if(activate){
        SetCurrent(young);
    } else {
        view->hide();
        RaiseDisplayedViewPriority();
    }

    if(!had_been_switching) View::SetSwitchingState(false);

    return view;
}

SharedView TreeBank::OpenInNewDirectory(QUrl url, bool activate, ViewNode *older){
    return OpenInNewDirectory(QNetworkRequest(url), activate, older);
}

SharedView TreeBank::OpenInNewDirectory(QList<QNetworkRequest> reqs, bool activate, ViewNode *older){
    ChangeScope changing;
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
        if(!view){
            View::SetSwitchingState(false);
            return nullptr;
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

static SharedView LoadWithLink(QNetworkRequest req, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = TreeBank::CreateView(req, vn);
    SetViewProp(u, vn, view);
    TreeBank::PrependToAllViews(view);
    return view;
}

static SharedView LoadWithLink(ViewNode *vn){
    if(vn->HoldsView())
        return LoadWithLink(QNetworkRequest(vn->GetUrl()), vn);
    return nullptr;
}

static SharedView AutoLoadWithLink(QNetworkRequest req, ViewNode *vn){
    QUrl u = req.url();
    SharedView view = SharedView();
    if(View::HiddenViewsStayActive() &&
       DirectoryPage::StateIn(GetNodeSettings(vn),
                              QStringLiteral("[nN](?:o)?(?:[aA](?:uto)?)?[lL](?:oad)?")) == 0)
        view = TreeBank::CreateView(req, vn);
    SetViewProp(u, vn, view);
    if(view){
        TreeBank::AppendToAllViews(view);
        view->RestoreZoom();
        view->hide();
    }
    return view;
}

static SharedView AutoLoadWithLink(ViewNode *vn){
    if(vn->HoldsView())
        return AutoLoadWithLink(QNetworkRequest(vn->GetUrl()), vn);
    return nullptr;
}

static void SetViewProp(QUrl url, ViewNode *vn, SharedView view){
    vn->SetUrl(url);
    vn->SetView(view.get());
    if(view) view->SetViewNode(vn);
}

QMenu *TreeBank::NodeMenu(){
    QMenu *menu = new QMenu(tr("TabList"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_DisplayViewTree));
    menu->addAction(Action(_DisplayTrashTree));
    menu->addSeparator();
    menu->addAction(Action(_Close));
    menu->addAction(Action(_Restore));
    menu->addAction(Action(_NextView));
    menu->addAction(Action(_PrevView));
    menu->addAction(Action(_DigView));
    menu->addAction(Action(_BuryView));
    menu->addAction(Action(_NewViewNode));
    menu->addAction(Action(_CloneViewNode));
    menu->addSeparator();
    menu->addAction(Action(_OpenDirectorySettings));

    return menu;
}

QMenu *TreeBank::DisplayMenu(){
    QMenu *menu = new QMenu(tr("Display"), this);
    menu->setToolTipsVisible(true);

    menu->addAction(Action(_ToggleNotifier));
    menu->addAction(Action(_ToggleReceiver));
    GetMainWindow()->AddDisplayMenuActions(menu);
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
    menu->addAction(Action(_OpenSettings));
    menu->addAction(Action(_OpenDirectorySettings));
    menu->addSeparator();
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
    if(m_Notifier && !m_PurgeNotifier && !m_PurgeView) m_Notifier->Purge();
    if(m_Receiver && !m_PurgeReceiver && !m_PurgeView) m_Receiver->Purge();
}

void TreeBank::JoinChildWidgetsIfNeed(){
    if(m_Notifier && !m_PurgeNotifier && !m_PurgeView) m_Notifier->Join();
    if(m_Receiver && !m_PurgeReceiver && !m_PurgeView) m_Receiver->Join();
}

QSize TreeBank::ViewSize() const {
    if(m_MiniMap && m_MiniMap->IsActive())
        return QSize(width() - m_MiniMap->MapWidth(), height());
    return size();
}

void TreeBank::ResizeViewArea(QSize size){
    m_View->setGeometry(QRect(QPoint(), size));
    m_View->setSceneRect(QRect(QPoint(), size));
    m_Gadgets->ResizeNotify(size);
}

void TreeBank::SetMiniMapShelved(bool shelved){
    if(!m_MiniMap) return;
    m_MiniMap->SetShelved(shelved);
    const QSize viewSize = ViewSize();
    ResizeViewArea(viewSize);
    if(m_CurrentView) m_CurrentView->resize(viewSize);
}

void TreeBank::RestackChildWidgets(OverviewState overview){
    QMap<int, QWidget*> stack;

    const bool covering =
        overview == OverviewUp ||
        (overview == OverviewAsIs && m_Gadgets && m_Gadgets->IsActive()) ||
        (m_CurrentView && !qobject_cast<QWidget*>(m_CurrentView->base()));

    stack[covering ? COVERING_SCENE_WIDGET_LAYER : SCENE_WIDGET_LAYER] = m_View;

    if(m_CurrentView){
        if(QWidget *w = qobject_cast<QWidget*>(m_CurrentView->base())){
            if(w->parentWidget() == this) stack[VIEW_WIDGET_LAYER] = w;
        } else {
            m_CurrentView->raise();
        }
    }

    if(m_Notifier) stack[NOTIFIER_WIDGET_LAYER] = m_Notifier;
    if(m_Receiver) stack[RECEIVER_WIDGET_LAYER] = m_Receiver;
    if(m_MiniMap) stack[MINIMAP_WIDGET_LAYER] = m_MiniMap;

    foreach(QWidget *w, stack){ w->raise();}
}

void TreeBank::resizeEvent(QResizeEvent *ev){
    const QSize viewSize = ViewSize();
    ResizeViewArea(viewSize);
    if(m_CurrentView) m_CurrentView->resize(viewSize);
    GetMainWindow()->AdjustAllEdgeWidgets();
    if(m_Notifier) m_Notifier->ResizeNotify(ev->size());
    if(m_Receiver) m_Receiver->ResizeNotify(ev->size());
    if(m_MiniMap) m_MiniMap->ResizeNotify(ev->size());
    ev->setAccepted(true);
}

void TreeBank::timerEvent(QTimerEvent *ev){
    Q_UNUSED(ev)
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

    if(m_CurrentView) return;

    if(!ev->mimeData()->urls().isEmpty()){
        if(OpenOnSuitableNode(ev->mimeData()->urls(), true))
            ev->setAccepted(true);
    }
}

void TreeBank::mouseReleaseEvent(QMouseEvent *ev){
    QWidget::mouseReleaseEvent(ev);

}

void TreeBank::contextMenuEvent(QContextMenuEvent *ev){
    QWidget::contextMenuEvent(ev);
    if(ev->isAccepted()) return;

    if(View::TakeRightButtonConsumed()){
        ev->setAccepted(true);
        return;
    }

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
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Up);
}

void TreeBank::Down(SharedView view){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Down);
}

void TreeBank::Right(SharedView view){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Right);
}

void TreeBank::Left(SharedView view){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Left);
}

void TreeBank::PageUp(SharedView view){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_PageUp);
}

void TreeBank::PageDown(SharedView view){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_PageDown);
}

void TreeBank::Home(SharedView view){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_Home);
}

void TreeBank::End(SharedView view){
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

static ViewNode *FindByUrl(Node *nd, const QUrl &url){
    foreach(Node *child, nd->GetChildren()){
        ViewNode *vn = child->ToViewNode();
        if(!vn) continue;
        if(VanillaPage::SameDocument(vn->GetUrl(), url)) return vn;
        if(ViewNode *found = FindByUrl(vn, url)) return found;
    }
    return nullptr;
}

void TreeBank::OpenByCommandOperation(const QUrl &url){
    if(m_CurrentView){
        QObject *page = m_CurrentView->page();
        if(Page *p = qobject_cast<Page*>(page)){
            p->OpenInNew(url);
            return;
        }
#ifdef WEBENGINEVIEW
        if(WebEnginePage *p = qobject_cast<WebEnginePage*>(page)){
            p->OpenInNew(url);
            return;
        }
#endif
    }
    OpenOnSuitableNode(url, true);
}

void TreeBank::OpenSettings(const QString &category){
#ifdef WEBENGINEVIEW
    QUrl url = SettingsSchemeHandler::SettingsUrl();
    if(!category.isEmpty()) url.setFragment(category);

    if(ViewNode *found = FindByUrl(m_ViewRoot, url)){
        if(!category.isEmpty()) found->SetUrl(url);
        Recreate(found);
        return;
    }
    OpenByCommandOperation(url);
#endif
}

static ViewNode *DirectoryOf(Node *nd){
    for(; nd; nd = nd->GetParent()){
        ViewNode *vn = nd->ToViewNode();
        if(vn && vn->IsDirectory()) return vn;
    }
    return TreeBank::GetViewRoot();
}

#ifdef WEBENGINEVIEW
static ViewNode *DirectoryPageIn(Node *directory){
    if(!directory) return nullptr;
    foreach(Node *nd, directory->GetChildren()){
        ViewNode *vn = nd->ToViewNode();
        if(vn && DirectoryPage::IsPageUrl(vn->GetUrl())) return vn;
    }
    return nullptr;
}
#endif

void TreeBank::OpenDirectorySettings(){
#ifdef WEBENGINEVIEW
    if(ViewNode *found = DirectoryPageIn(DirectoryOf(m_CurrentViewNode))){
        Recreate(found);
        return;
    }
    OpenByCommandOperation(DirectoryPage::PageUrl());
#endif
}

void TreeBank::OpenDirectorySettings(ViewNode *subject){
#ifdef WEBENGINEVIEW
    ViewNode *directory = DirectoryOf(subject);
    if(ViewNode *found = DirectoryPageIn(directory)){
        Recreate(found);
        return;
    }
    OpenOnSuitableNode(DirectoryPage::PageUrl(), true, directory);
#else
    Q_UNUSED(subject)
#endif
}

void TreeBank::AboutQt(){
    Application::AboutQt(this);
}

void TreeBank::Quit(){
    Application::Quit();
}

void TreeBank::ClearCookies(){
    if(!ModalDialog::Question(tr("Clear cookies."),
                              tr("Every cookie of every network space is deleted.\n"
                                 "Anything a site kept you signed in with goes with them."))) return;
    NetworkController::ClearCookies();
}

void TreeBank::ClearHttpCache(){
    NetworkController::ClearHttpCache();
}

void TreeBank::ClearVisitedLinks(){
    NetworkController::ClearVisitedLinks();
}

void TreeBank::ToggleNotifier(){
    if(m_Notifier){
        m_Notifier->deleteLater();
        m_Notifier = nullptr;
    } else {
        bool purge = m_PurgeNotifier || m_PurgeView || m_CurrentView->ForbidToOverlap();

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
        m_Receiver = nullptr;
    } else {
        bool purge = m_PurgeReceiver || m_PurgeView || m_CurrentView->ForbidToOverlap();

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

void TreeBank::Back(){
    View *view = m_CurrentView.get();
    if(!view) return;

    if(view->CanGoBack()){

        view->SetScroll(QPointF());
        view->TriggerNativeGoBackAction();

    } else if(View::EnableDestinationInferrer()){

        view->GoBackToInferedUrl();
    }
}

void TreeBank::Forward(){
    View *view = m_CurrentView.get();
    if(!view) return;

    if(view->CanGoForward()){

        view->SetScroll(QPointF());
        view->TriggerNativeGoForwardAction();

    } else if(View::EnableDestinationInferrer()){

        view->GoForwardToInferedUrl();
    }
}

void TreeBank::Rewind(){
    View *view = m_CurrentView.get();
    if(!view || !view->CanGoBack()) return;

    view->SetScroll(QPointF());
    view->TriggerNativeRewindAction();
}

void TreeBank::FastForward(){
    View *view = m_CurrentView.get();
    if(!view) return;

    if(view->CanGoForward()){

        view->SetScroll(QPointF());
        view->TriggerNativeFastForwardAction();

    } else {

        view->GoForwardToInferedUrl();
    }
}

void TreeBank::UpDirectory(){
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    View *view = m_CurrentView.get();
    if(!view) return;

    QUrl url = view->GetViewNode() ? view->GetViewNode()->GetUrl() : QUrl();
    if(url.isEmpty()) return;

    QNetworkRequest req(Page::UpDirectoryUrl(url));
    req.setRawHeader("Referer", url.toEncoded());

    view->SetScroll(QPointF());
    view->Load(req);
}

void TreeBank::Close(ViewNode *vn){
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;
    DeleteNode(vn);
}

void TreeBank::Restore(ViewNode *vn, ViewNode *dir){
    ChangeScope changing;
    if(m_TrashRoot->HasNoChildren()) return;

    if(vn){
        if(GetRoot(vn) == m_ViewRoot){
            ViewNode *rest = m_TrashRoot->TakeFirstChild()->ToViewNode();
            if(!dir) dir = m_ViewRoot;
            if(dir->ChildrenContains(vn))
                dir->InsertChild(dir->ChildrenIndexOf(vn) + 1, rest);
            else
                dir->AppendChild(rest);

            rest->SetParent(dir);
            ApplySpecificSettings(rest);
            EmitNodeCreated(NodeList() << rest);
        } else {
            MoveNode(vn, m_ViewRoot);
        }
    } else {
        if(dir){
            ViewNode *rest = m_TrashRoot->GetFirstChild()->ToViewNode();
            MoveNode(rest, GetRoot(dir) == m_ViewRoot ? dir : m_ViewRoot);
        } else {
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
    ChangeScope changing;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;
    DislinkView(vn);
    SetCurrent(vn);
}

void TreeBank::NextView(ViewNode *vn){
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    Node *next = vn;
    if(m_TraverseAllView){
        do next = next->Next();
        while(next &&
              ((!next->GetView() && next->GetUrl().isEmpty()) ||
               ( next->GetView() && next->GetView()->GetTreeBank() &&
                 next->GetView()->GetTreeBank()->GetCurrentViewNode() == next)));

        if(next && SetCurrent(next)){
        } else {
            next = m_ViewRoot;
            while(!next->HasNoChildren())
                next = next->GetFirstChild();

            while(next &&
                  ((!next->GetView() && next->GetUrl().isEmpty()) ||
                   ( next->GetView() && next->GetView()->GetTreeBank() &&
                     next->GetView()->GetTreeBank()->GetCurrentViewNode() == next))){
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
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    Node* prev = vn;
    if(m_TraverseAllView){
        do prev = prev->Prev();
        while(prev &&
              ((!prev->GetView() && prev->GetUrl().isEmpty()) ||
               ( prev->GetView() && prev->GetView()->GetTreeBank() &&
                 prev->GetView()->GetTreeBank()->GetCurrentViewNode() == prev)));

        if(prev && SetCurrent(prev)){
        } else {
            prev = m_ViewRoot;
            while(!prev->HasNoChildren())
                prev = prev->GetLastChild();

            while(prev &&
                  ((!prev->GetView() && prev->GetUrl().isEmpty()) ||
                   ( prev->GetView() && prev->GetView()->GetTreeBank() &&
                     prev->GetView()->GetTreeBank()->GetCurrentViewNode() == prev))){
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
    Q_UNUSED(vn)

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
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    NodeList siblings = vn->GetSiblings();
    if(!siblings.isEmpty()){
        SetCurrent(siblings.last());
    }
}

ViewNode *TreeBank::NewViewNode(ViewNode *vn){
    ChangeScope changing;
    if(!vn) vn = m_CurrentViewNode;

    if(!vn || !vn->GetParent() || IsTrash(vn)){
        SharedView view = OpenOnSuitableNode(QNetworkRequest(BLANK_URL),
                                             Page::Activate(), m_ViewRoot);
        return view ? view->GetViewNode() : nullptr;
    }

    ViewNode *newNode = vn->New();
    if(newNode){
        if(m_Gadgets && m_Gadgets->IsActive()){
            EmitNodeCreated(NodeList() << newNode);
            return newNode;
        }
    } else {
        return nullptr;
    }
    SharedView view = LoadWithLink(newNode);
    EmitNodeCreated(NodeList() << newNode);
    if(Page::Activate()){
        SetCurrent(newNode);
    } else {
        view->hide();
        RaiseDisplayedViewPriority();
    }
    return newNode;
}

ViewNode *TreeBank::CloneViewNode(ViewNode *vn){
    ChangeScope changing;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return nullptr;

    ViewNode *clone = vn->Clone();
    if(clone){
        if(m_Gadgets && m_Gadgets->IsActive()){
            EmitNodeCreated(NodeList() << clone);
            return clone;
        }
    } else {
        return nullptr;
    }
    SharedView view = LoadWithLink(clone);
    EmitNodeCreated(NodeList() << clone);
    if(Page::Activate()){
        SetCurrent(clone);
    } else {
        if(view) view->hide();
        RaiseDisplayedViewPriority();
    }
    return clone;
}

ViewNode *TreeBank::MakeLocalNode(ViewNode *older){
    ChangeScope changing;
    if(m_Gadgets && m_Gadgets->IsActive()) return nullptr;

    if(!older) older = m_CurrentViewNode;
    if(!older) return nullptr;

    QFileDialog::Options options =
        QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;

    QString path =
        ModalDialog::GetExistingDirectory(QString(), QStringLiteral("."), options);

    if(!path.isEmpty()){
        ViewNode *young = older->NewDir();
        ViewNode *parent = young->GetParent()->ToViewNode();

        QUrl url = QUrl::fromLocalFile(path);
        QString title = QStringLiteral("filer;localview");
        parent->SetTitle(title);

        young->SetHoldView(true);
        young->SetUrl(url);

        LoadWithLink(young);
        EmitNodeCreated(NodeList() << young->GetParent() << young);
        SetCurrent(young);
        RaiseDisplayedViewPriority();
        return young;
    }
    return nullptr;
}

ViewNode *TreeBank::MakeChildDirectory(ViewNode *vn){
    if(!vn) vn = m_ViewRoot;
    if(vn){
        ViewNode *child = vn->MakeChild();
        EmitNodeCreated(NodeList() << child);
        return child;
    }
    return nullptr;
}

ViewNode *TreeBank::MakeSiblingDirectory(ViewNode *vn){
    if(!vn) vn = m_ViewRoot;
    if(vn){
        ViewNode *sibling = vn->MakeSibling();
        EmitNodeCreated(NodeList() << sibling);
        return sibling;
    }
    return nullptr;
}

void TreeBank::DisplayViewTree(ViewNode *vn){
    if(m_Gadgets->IsDisplaying(Gadgets::ViewTree))
        return m_Gadgets->Deactivate();
    m_Gadgets->Activate(Gadgets::ViewTree);
    if(!vn) vn = m_CurrentViewNode;
    if(!vn && !m_ViewRoot->HasNoChildren())
        vn = m_ViewRoot->GetFirstChild()->ToViewNode();
    m_Gadgets->SetCurrent(vn);
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
    if(m_Gadgets->IsDisplaying(Gadgets::AccessKey))
        return m_Gadgets->Deactivate();
    if(m_Gadgets->IsActive())
        return;
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
    if(!view) view = m_CurrentView;
    if(!m_Receiver) ToggleReceiver();
    m_Receiver->OpenCommand(view.get());
}

void TreeBank::ReleaseHiddenView(SharedView){
    ChangeScope changing;
    foreach(SharedView view, m_AllViews){
        if(TreeBank *tb = view->GetTreeBank())
            if(!tb->IsCurrent(view))
                DislinkView(view->GetViewNode());
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

#define VANILLA_EDIT_ACTION(name)                       \
    void TreeBank::name(SharedView view){               \
        if(!view) view = m_CurrentView;                 \
        if(view) view->TriggerAction(Page::_##name);    \
    }
FOR_EACH_EDIT_EVENTS(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION

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
    if(m_Gadgets && m_Gadgets->IsActive()) return;

    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_ZoomIn);
}

void TreeBank::ZoomOut(SharedView view){
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

void TreeBank::OpenWithDefault(SharedView view){
    if(!view) view = m_CurrentView;
    if(view) view->TriggerAction(Page::_OpenWithDefault);
}

void TreeBank::OpenWithCommand(QString name, SharedView view){
    if(!view) view = m_CurrentView;
    if(view) Application::RunExternalCommand(name, view->GetViewNode()->GetUrl());
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
    return nullptr;
}

void TreeBank::SetActionIcon(QAction *action, TreeBankAction a){
    switch(a){
    case _Up:          action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowUp));    break;
    case _Down:        action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowDown));  break;
    case _Right:       action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowRight)); break;
    case _Left:        action->setIcon(Application::style()->standardIcon(QStyle::SP_ArrowLeft));  break;
    case _Back:        action->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/back.png"),        Theme::BarIcon))); break;
    case _Forward:     action->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/forward.png"),     Theme::BarIcon))); break;
    case _Rewind:      action->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/rewind.png"),      Theme::BarIcon))); break;
    case _FastForward: action->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/fastforward.png"), Theme::BarIcon))); break;
    case _Reload:      action->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/reload.png"),      Theme::BarIcon))); break;
    case _Stop:        action->setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/menu/stop.png"),        Theme::BarIcon))); break;
    default: break;
    }
}

void TreeBank::ApplyTheme(){
    foreach(TreeBankAction a, m_ActionTable.keys())
        SetActionIcon(m_ActionTable[a], a);

    foreach(SharedView view, m_AllViews){
        if(view) view->ApplyTheme();
    }
}

QAction *TreeBank::Action(TreeBankAction a){
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
        case _ToggleNotifier: action->setChecked(m_Notifier); break;
        case _ToggleReceiver: action->setChecked(m_Receiver); break;
        case _ToggleMenuBar:  action->setChecked(!GetMainWindow()->IsMenuBarEmpty()); break;
        case _ToggleTreeBar:  action->setChecked(GetMainWindow()->GetTreeBar()->isVisible()); break;
        case _ToggleToolBar:  action->setChecked(GetMainWindow()->GetToolBar()->isVisible()); break;
        default: break;
        }
        return action;
    }

    m_ActionTable[a] = action = new QAction(this);

    SetActionIcon(action, a);

    switch(a){
    case _NoAction: break;

#define DEFINE_ACTION(name, text)                                       \
        case _##name:                                                   \
            action->setText(text);                                      \
            action->setToolTip(text);                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name##Key()));                         \
            break

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
            break

        DEFINE_ACTION(Import,       tr("Import"));
        DEFINE_ACTION(Export,       tr("Export"));
        DEFINE_ACTION(AboutVanilla, tr("AboutVanilla"));
        DEFINE_ACTION(AboutQt,      tr("AboutQt"));
        DEFINE_ACTION(OpenSettings, tr("Settings"));
        DEFINE_ACTION(OpenDirectorySettings, tr("DirectorySettings"));
        DEFINE_ACTION(Quit,         tr("Quit"));

        DEFINE_ACTION(ClearCookies,      tr("ClearCookies"));
        DEFINE_ACTION(ClearHttpCache,    tr("ClearHttpCache"));
        DEFINE_ACTION(ClearVisitedLinks, tr("ClearVisitedLinks"));

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
        DEFINE_ACTION(CloneViewNode,    tr("CloneViewNode"));

        DEFINE_ACTION(DisplayAccessKey, tr("DisplayAccessKey"));
        DEFINE_ACTION(DisplayViewTree,  tr("DisplayViewTree"));
        DEFINE_ACTION(DisplayTrashTree, tr("DisplayTrashTree"));

        DEFINE_ACTION(OpenTextSeeker,   tr("OpenTextSeeker"));
        DEFINE_ACTION(OpenQueryEditor,  tr("OpenQueryEditor"));
        DEFINE_ACTION(OpenUrlEditor,    tr("OpenUrlEditor"));
        DEFINE_ACTION(OpenCommand,      tr("OpenCommand"));
        DEFINE_ACTION(ReleaseHiddenView,tr("ReleaseHiddenView"));
        DEFINE_ACTION(Load,             tr("Load"));

        DEFINE_ACTION(Copy,                 tr("Copy"));
        DEFINE_ACTION(Cut,                  tr("Cut"));
        DEFINE_ACTION(Paste,                tr("Paste"));

        DEFINE_ACTION(PasteAndMatchStyle,     tr("PasteAndMatchStyle"));
        DEFINE_ACTION(ToggleBold,             tr("ToggleBold"));
        DEFINE_ACTION(ToggleItalic,           tr("ToggleItalic"));
        DEFINE_ACTION(ToggleUnderline,        tr("ToggleUnderline"));
        DEFINE_ACTION(ToggleStrikethrough,    tr("ToggleStrikethrough"));
        DEFINE_ACTION(AlignLeft,              tr("AlignLeft"));
        DEFINE_ACTION(AlignCenter,            tr("AlignCenter"));
        DEFINE_ACTION(AlignRight,             tr("AlignRight"));
        DEFINE_ACTION(AlignJustified,         tr("AlignJustified"));
        DEFINE_ACTION(Indent,                 tr("Indent"));
        DEFINE_ACTION(Outdent,                tr("Outdent"));
        DEFINE_ACTION(InsertOrderedList,      tr("InsertOrderedList"));
        DEFINE_ACTION(InsertUnorderedList,    tr("InsertUnorderedList"));
        DEFINE_ACTION(ChangeTextDirectionLTR, tr("ChangeTextDirectionLTR"));
        DEFINE_ACTION(ChangeTextDirectionRTL, tr("ChangeTextDirectionRTL"));

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
        DEFINE_ACTION(OpenWithDefault,  tr("OpenWithDefault"));

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

    default: break;
    }
    return action;
}

QString TreeBank::KeyAction(const QKeySequence &seq){
    if(!View::EnableSingleKeyShortcut() && InputMap::IsSingleKey(seq)) return QString();
    return m_KeyMap.value(seq);
}

bool TreeBank::TriggerKeyEvent(QKeyEvent *ev){
    QKeySequence seq = Application::MakeKeySequence(ev);
    if(seq.isEmpty()) return false;
    QString str = KeyAction(seq);
    if(str.isEmpty()) return false;

    if(!TriggerAction(str)){
        return false;
    }
    return true;
}

bool TreeBank::TriggerKeyEvent(QString str){
    QKeySequence seq = Application::MakeKeySequence(str);
    if(seq.isEmpty()) return false;
    str = m_KeyMap[seq];
    if(str.isEmpty()) return false;

    if(!TriggerAction(str)){
        return false;
    }
    return true;
}

void TreeBank::DeleteView(View *view){
    view->DeleteLater();
}

#define QRE(str) QRegularExpression(QStringLiteral(str))

bool TreeBank::NeedsEngineForVanillaPage(const QUrl &url, const QStringList &set){
    if(!VanillaPage::IsPageUrl(url)) return false;
    return set.indexOf(QRE("\\A"              "[nN](?:ative)?" "[wW](?:eb)?" "(?:[vV](?:iew)?)?\\Z")) != -1
        || set.indexOf(QRE("\\A[qQ](?:uick)?" "[nN](?:ative)?" "[wW](?:eb)?" "(?:[vV](?:iew)?)?\\Z")) != -1;
}

SharedView TreeBank::CreateView(QNetworkRequest req, ViewNode *vn){
    MainWindow *win = Application::GetCurrentWindow();
    TreeBank *tb = win ? win->GetTreeBank() : nullptr;
    QStringList set = GetNodeSettings(vn);
    QString id = GetNetworkSpaceId(vn);

    QUrl url = req.url();
    QString urlstr = url.toString();

    SharedView view = SharedView();

    view = SharedView(
#ifdef LOCALVIEW
        (urlstr.startsWith(QStringLiteral("file:///")) && (QFileInfo(url.toLocalFile()).isDir() || LocalView::IsSupported(url))) ? new LocalView(tb, id, set) :
#endif
#ifdef WEBENGINEVIEW
        set.indexOf(QRE("\\A"                 "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?\\Z")) != -1 ? new WebEngineView(tb, id, set) :
        set.indexOf(QRE("\\A[gG](?:raphics)?" "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?\\Z")) != -1 ? new WebEngineView(tb, id, set) :
        set.indexOf(QRE("\\A[qQ](?:uick)?"    "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?\\Z")) != -1 ? new QuickWebEngineView(tb, id, set) :
        set.indexOf(QRE("\\A"                 "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?\\Z")) != -1 ? new WebEngineView(tb, id, set) :
        set.indexOf(QRE("\\A[gG](?:raphics)?" "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?\\Z")) != -1 ? new WebEngineView(tb, id, set) :
        set.indexOf(QRE("\\A[qQ](?:uick)?"    "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?\\Z")) != -1 ? new QuickWebEngineView(tb, id, set) :
#endif
#if defined(NATIVEWEBVIEW) && defined(WEBENGINEVIEW)
        NeedsEngineForVanillaPage(url, set) ? new WebEngineView(tb, id, set) :
#endif
#ifdef NATIVEWEBVIEW
        set.indexOf(QRE("\\A"                 "[nN](?:ative)?" "[wW](?:eb)?" "(?:[vV](?:iew)?)?\\Z")) != -1 ? new QuickNativeWebView(tb, id, set) :
        set.indexOf(QRE("\\A[qQ](?:uick)?"    "[nN](?:ative)?" "[wW](?:eb)?" "(?:[vV](?:iew)?)?\\Z")) != -1 ? new QuickNativeWebView(tb, id, set) :
#endif
#ifdef EDGEWEBVIEW
        set.indexOf(QRE("\\A"                 "[eE](?:dge)?" "(?:[wW](?:eb)?)?" "(?:[vV](?:iew)?)?\\Z")) != -1 ? new EdgeWebView(tb, id, set) :
#endif
#ifdef LOCALVIEW
        set.indexOf(QRE("\\A"                 "[lL](?:ocal)?"                "(?:[vV](?:iew)?)?\\Z")) != -1 ? new LocalView(tb, id, set) :
#endif
#if defined(EDGEWEBVIEW)
        EdgeWebView::RuntimeAvailable() ? static_cast<View*>(new EdgeWebView(tb, id, set)) :
#endif
#if defined(WEBENGINEVIEW)
        static_cast<View*>(new WebEngineView(tb, id, set))
#elif defined(NATIVEWEBVIEW)
        static_cast<View*>(new QuickNativeWebView(tb, id, set))
#else
        static_cast<View*>(nullptr)
#endif
        , &DeleteView);

    if(vn->GetTitle().isEmpty())
        vn->SetTitle(req.url().toString());

    SharedView first = m_AllViews.length() ? m_AllViews.first() : nullptr;

    Q_ASSERT(ChangeScope::Depth() > 0);
    while(m_MaxViewCount && m_AllViews.length() > m_MaxViewCount){
        SharedView v = m_AllViews.takeLast();

        if(first && v == first)
            LiftMaxViewCountIfNeed(m_MaxViewCount);

        if(WinIndex(v) == 0 && !v->RecentlyAudible()){
            v->GetViewNode()->SetView(nullptr);

            ReleaseView(v);

        } else {
            PrependToAllViews(v);
        }
    }

    view->SetThis(WeakView(view));
    view->SetViewNode(vn);
    view->RestoreHistoryOrLoad(req);
    return view;
}
#undef QRE
