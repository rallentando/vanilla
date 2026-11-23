#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"

#include "treebank.hpp"
#include "actiontext.hpp"

#include <QApplication>

#include <QMenu>
#include <QThread>
#include <atomic>
#include <QMutex>
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
#include <QElapsedTimer>
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#endif

#include <memory>

#include <QGraphicsRotation>
#include <QGraphicsScale>

#include "application.hpp"
#include "treeserializer.hpp"
#include "fileexchange.hpp"
#include "inputmap.hpp"
#include "inputmapschema.hpp"
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
#include "menukeys.hpp"
#include "mainwindow.hpp"
#include "downloadbar.hpp"
#include "sidepanels.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"
#include "localview.hpp"
#include "dialog.hpp"
#include "spacerename.hpp"
#include "passwordstore.hpp"

QString TreeBank::m_RootName = QString();
ViewNode *TreeBank::m_ViewRoot  = nullptr;
ViewNode *TreeBank::m_TrashRoot = nullptr;

ViewNode *TreeBank::m_ViewIterForward  = nullptr;
ViewNode *TreeBank::m_ViewIterBackward = nullptr;
bool TreeBank::m_ViewIterForwardPending  = false;
bool TreeBank::m_ViewIterBackwardPending = false;

int TreeBank::ChangeScope::m_Depth = 0;
bool TreeBank::m_TraverseAllView = false;
bool TreeBank::m_PurgeNotifier = false;
bool TreeBank::m_PurgeReceiver = false;
bool TreeBank::m_PurgeView = false;
bool TreeBank::m_MovingSpace = false;
QString TreeBank::m_MovingFrom = QString();
QString TreeBank::m_MovingTo = QString();
bool TreeBank::m_EnableNotifierScrollArea = false;
int  TreeBank::m_MaxViewCount = 0;
int  TreeBank::m_MaxTrashEntryCount = 0;
int  TreeBank::m_TraverseCondition = 0;

TreeBank::Viewport     TreeBank::m_Viewport = Widget;
SharedViewList TreeBank::m_AllViews = SharedViewList();
View *TreeBank::m_ExtractingDownloadCarrier = nullptr;
SharedViewList TreeBank::m_ViewUpdateBox = SharedViewList();
NodeList TreeBank::m_NodeDeleteBox = NodeList();
QHash<quint64, int> TreeBank::m_LeftOutOfSaves = QHash<quint64, int>();
NodeList TreeBank::m_FreshDeleteBox = NodeList();
NodeList TreeBank::m_ClipboardNodes = NodeList();
bool TreeBank::m_ClipboardIsCut = false;
QList<QPair<NodeList, Node*>> TreeBank::m_Leaving = QList<QPair<NodeList, Node*>>();
bool TreeBank::m_DeleteBarrierBooked = false;

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
{
    m_MiniMap = nullptr;
    m_Gadgets_ = SharedView(new Gadgets(this), &DeleteView);
    m_Gadgets_->SetThis(WeakView(m_Gadgets_));
    m_Gadgets = static_cast<Gadgets*>(m_Gadgets_.get());

    if(DrawsOnOpenGL()){
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
    TabsLookChanged();
    if(ToldOfChanges()) ToldOfChanges()();
}

namespace {
    QMutex &TellingLock(){
        static QMutex lock;
        return lock;
    }
    bool &Telling(){
        static bool telling = true;
        return telling;
    }
}

void TreeBank::TabsLookChanged(){
    {   QMutexLocker locker(&TellingLock());
        if(!Telling()) return;
        QCoreApplication *app = QCoreApplication::instance();
        if(!app) return;
        if(QThread::currentThread() != app->thread()){
            static std::atomic<bool> posted(false);
            if(posted.exchange(true)) return;
            QMetaObject::invokeMethod(app, [](){
                posted = false;
                TabsLookChanged();
            }, Qt::QueuedConnection);
            return;
        }
    }
    foreach(MainWindow *win, Application::GetMainWindows()){
        if(TreeBar *bar = win->GetTreeBar()) bar->ScheduleRepaint();
        if(TreeBank *bank = win->GetTreeBank())
            if(Gadgets *gadgets = bank->GetGadgets()) gadgets->ScheduleRepaint();
    }
}

void TreeBank::StopTellingTabs(){
    QMutexLocker locker(&TellingLock());
    Telling() = false;
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
    TabsLookChanged();
    foreach(MainWindow *win, Application::GetMainWindows()){
        emit win->GetTreeBank()->FoldedChanged(nds);
    }
}

void TreeBank::ConnectToNotifier(){
    if(m_Notifier){
        connect(m_Gadgets,  &Gadgets::statusBarMessage,
                m_Notifier, QOverload<QString>::of(&Notifier::SetStatus));
        connect(m_Gadgets,  &Gadgets::statusBarMessage2,
                m_Notifier, QOverload<QString, QString>::of(&Notifier::SetStatus));
        connect(m_Gadgets,  &Gadgets::ItemHovered,
                m_Notifier, &Notifier::SetLink);

        connect(m_Gadgets,  &Gadgets::ScrollChanged,
                m_Notifier, &Notifier::SetScroll);
        connect(m_Notifier, &Notifier::ScrollRequest,
                m_Gadgets,  QOverload<QPointF>::of(&GraphicsTableView::SetScroll));

        connect(Application::GetAutoSaver(), &AutoSaver::Started,
                m_Notifier, &Notifier::AutoSaveStarted);
        connect(Application::GetAutoSaver(), &AutoSaver::Failed,
                m_Notifier, &Notifier::AutoSaveFailed);
        connect(Application::GetAutoSaver(), &AutoSaver::Finished,
                m_Notifier, &Notifier::AutoSaveFinished);
    }
}

void TreeBank::ConnectToReceiver(){
    if(m_Receiver){
        connect(m_Receiver, QOverload<QUrl>::of(&Receiver::OpenUrl),
                this, QOverload<QUrl>::of(&TreeBank::OpenInNewIfNeed));
        connect(m_Receiver, QOverload<QList<QUrl>>::of(&Receiver::OpenUrl),
                this, QOverload<QList<QUrl>>::of(&TreeBank::OpenInNewIfNeed));
        connect(m_Receiver, &Receiver::OpenQueryUrl,
                this, QOverload<QString>::of(&TreeBank::OpenInNewIfNeed));
        connect(m_Receiver, &Receiver::SearchWith,
                this, QOverload<QString, QString>::of(&TreeBank::OpenInNewIfNeed));
        if(m_MiniMap) ConnectSeekToMiniMap();
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
             tr("Cannot change to an empty title or use the following characters.") +
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

    if(directory && !ReconfigureDirectory(nd->ToViewNode(), before, after))
        return false;

    return true;
}

namespace {
    int CountSpaces(Node *nd, ViewNode *except, const QString &name);
    void RebuildBelow(ViewNode *vn);
}

bool TreeBank::ReconfigureDirectory(ViewNode *vn, QString before, QString after){

    Node *parent = vn->GetParent();
    QString parentid = parent ? GetNetworkSpaceId(parent->ToViewNode()) : QString();
    QString befname = before.split(QStringLiteral(";")).first();
    QString aftname = after .split(QStringLiteral(";")).first();
    QStringList aftset = after .split(QStringLiteral(";")).mid(1);
    bool bef = !parent || DirectoryPage::SaysId(before) || DirectoryPage::NameSpellsId(befname);
    bool aft = !parent || DirectoryPage::SaysId(after) || DirectoryPage::NameSpellsId(aftname);

    if(vn == m_ViewRoot) m_RootName = after;

    if(m_MovingSpace){
        const QStringList moving = { m_MovingFrom, m_MovingTo };
        if(!bef && !aft && moving.contains(parentid)){
            MarkOffDisk(vn);
            return true;
        }
        if((bef && moving.contains(befname)) || (aft && moving.contains(aftname))){
            vn->SetTitle(before);
            if(vn == m_ViewRoot) m_RootName = before;
            return false;
        }
    }

    if(bef && aft && befname != aftname &&
       !NetworkController::FindNetworkAccessManager(aftname))
        return RenameSpace(vn, before, after);
    if(bef && aft && befname != aftname)
        return MergeSpace(vn, before, after);

    const bool shared = CountSpaces(m_ViewRoot, vn, befname) || CountSpaces(m_TrashRoot, vn, befname);
    if(bef && !aft && !shared)
        return RenameSpace(vn, before, after, true);

    if(!bef && aft && !NetworkController::FindNetworkAccessManager(aftname) &&
       NetworkController::StorageInUse(aftname)){
        vn->SetTitle(before);
        ModelessDialog::Information
            (tr("Cannot change this directory."),
             tr("The data of the network space \"%1\" is still being closed. "
                "Wait a moment and try again.").arg(aftname));
        return false;
    }
    const bool joining = !bef && aft && NetworkController::FindNetworkAccessManager(aftname);

    NetworkAccessManager *nam = bef ?
        (aft ? NetworkController::MoveNetworkAccessManager(befname, aftname, aftset) :
               NetworkController::GetNetworkAccessManager(parentid, aftset)) :
        (aft ? (joining ? NetworkController::GetNetworkAccessManager(aftname, aftset) :
                          NetworkController::CopyNetworkAccessManager(parentid, aftname, aftset)) :
               NetworkController::GetNetworkAccessManager(parentid, aftset));

    if(!nam) nam = NetworkController::GetNetworkAccessManager(parentid, aftset);
    Q_UNUSED(nam)

    foreach(Node *nd, vn->GetChildren()){
        ReapplySpecificSettings(nd->ToViewNode());
    }
    if(bef != aft) RebuildBelow(vn);
    MarkOffDisk(vn);
    return true;
}

namespace {

    bool NamesSpace(ViewNode *vn, const QString &name){
        if(!vn || !(vn->IsDirectory() || vn->IsRoot())) return false;
        const QString title = vn->GetTitle();
        const bool boundary = vn->IsRoot() || DirectoryPage::SaysId(title) ||
            DirectoryPage::NameSpellsId(DirectoryPage::TitleName(title));
        return boundary && title.split(QStringLiteral(";")).first() == name;
    }

    int CountSpaces(Node *nd, ViewNode *except, const QString &name){
        ViewNode *vn = nd ? nd->ToViewNode() : nullptr;
        if(!vn) return 0;
        int count = vn != except && NamesSpace(vn, name) ? 1 : 0;
        foreach(Node *child, vn->GetChildren()) count += CountSpaces(child, except, name);
        return count;
    }

    void CollectSpaceNames(Node *nd, QStringList &names){
        ViewNode *vn = nd ? nd->ToViewNode() : nullptr;
        if(!vn) return;
        if(vn->IsDirectory() || vn->IsRoot()){
            const QString title = vn->GetTitle();
            const bool boundary = vn->IsRoot() || DirectoryPage::SaysId(title) ||
                DirectoryPage::NameSpellsId(DirectoryPage::TitleName(title));
            const QString name = title.split(QStringLiteral(";")).first();
            if(boundary && !names.contains(name)) names << name;
        }
        foreach(Node *child, vn->GetChildren()) CollectSpaceNames(child, names);
    }

    void RebuildBelow(ViewNode *vn){
        foreach(Node *child, vn->GetChildren()){
            ViewNode *below = child->ToViewNode();
            if(!below) continue;
            if(below->IsDirectory()){
                const QString title = below->GetTitle();
                if(DirectoryPage::SaysId(title) ||
                   DirectoryPage::NameSpellsId(DirectoryPage::TitleName(title))) continue;
            }
            if(View *view = below->GetView()) view->RebuildForOffTheRecord();
            RebuildBelow(below);
        }
    }

    SpaceRename::Places SpacePlaces(const QList<QObject*> &profiles){
        SpaceRename::Places places;
        places.widgetStorage = NetworkController::WidgetStorageDirectory();
#ifdef WEBENGINEVIEW
        foreach(QObject *object, profiles){
            QWebEngineProfile *profile = qobject_cast<QWebEngineProfile*>(object);
            if(!profile || profile->isOffTheRecord()) continue;
            places.widgetStorage = QFileInfo(profile->persistentStoragePath()).absolutePath() + QStringLiteral("/");
            break;
        }
#else
        Q_UNUSED(profiles)
#endif
        places.data = Application::DataDirectory();
        places.state = Application::StateDirectory();
        places.storageName = [](const QString &id){ return NetworkController::ProfileStorageName(id);};
#ifdef EDGEWEBVIEW
        places.edgeProfiles = EdgeWebView::ProfilesFolder();
        places.edgeJournal = [](const QString &name){ return EdgeWebView::ExtensionJournal(name);};
#endif
        return places;
    }

    QString StoreName(SpaceRename::What what){
        switch(what){
        case SpaceRename::WidgetStorage: return TreeBank::tr("WebEngineView site data");
        case SpaceRename::WidgetCache:   return TreeBank::tr("WebEngineView cache");
        case SpaceRename::QuickStorage:  return TreeBank::tr("QuickWebEngineView site data");
        case SpaceRename::QuickCache:    return TreeBank::tr("QuickWebEngineView cache");
        case SpaceRename::EdgeProfile:   return TreeBank::tr("EdgeWebView site data");
        case SpaceRename::ExtensionState: return TreeBank::tr("Extension settings");
        }
        return QString();
    }

    struct SpaceWait {
        QList<QPointer<QObject>> going;
        QElapsedTimer waited;
        std::function<void()> then;
    };

    void WaitForViews(std::shared_ptr<SpaceWait> state){
        bool gone = true;
        foreach(const QPointer<QObject> &view, state->going) if(view){ gone = false; break;}
        if(!gone && state->waited.elapsed() < 10000){
            QTimer::singleShot(20, qApp, [state](){ WaitForViews(state);});
            return;
        }
        state->then();
    }

    void ReportRenameSpace(const QString &to, const QList<SpaceRename::Move> &failed, int passwords){
        if(failed.isEmpty() && !passwords) return;
        QStringList lines;
        foreach(const SpaceRename::Move &move, failed)
            lines << QStringLiteral("- ") + StoreName(move.what) + QStringLiteral("\n  ") +
                QDir::toNativeSeparators(move.from);
        if(passwords)
            lines << QStringLiteral("- ") + TreeBank::tr("Password data: %1").arg(passwords);
        ModelessDialog::Information
            (TreeBank::tr("Some data was not moved."),
             TreeBank::tr("The directory was renamed to \"%1\", but the following could not be moved "
                          "to the new name and was left where it was. Under the new name, it starts empty.")
             .arg(to) + QStringLiteral("\n\n") + lines.join(QStringLiteral("\n")));
    }
}

namespace {
    void ReportMergeSpace(const QString &from, const QString &to, bool edgeFailed,
                          bool qtLeft, int passwords){
        QStringList lines;
        if(qtLeft)
            lines << QStringLiteral("- ") + TreeBank::tr("WebEngineView and QuickWebEngineView site data "
                                                          "(sign-ins included): it cannot be carried over.");
        if(edgeFailed)
            lines << QStringLiteral("- ") + TreeBank::tr("EdgeWebView cookies: they could not be copied.");
        if(passwords)
            lines << QStringLiteral("- ") + TreeBank::tr("Password data: %1").arg(passwords);
        if(lines.isEmpty()) return;
        ModelessDialog::Information
            (TreeBank::tr("Some data was not carried over."),
             TreeBank::tr("\"%1\" was joined to \"%2\", but the following of \"%1\" was not carried over. "
                          "It is kept where it was.").arg(from, to)
             + QStringLiteral("\n\n") + lines.join(QStringLiteral("\n")));
    }
}

QStringList TreeBank::SpaceNamesInUse(){
    QStringList names;
    CollectSpaceNames(m_ViewRoot, names);
    CollectSpaceNames(m_TrashRoot, names);
    return names;
}

QSet<QString> TreeBank::StoragesOfViews(){
    QSet<QString> storages;
    foreach(SharedView view, m_AllViews){
        if(QObject *profile = view->EngineProfile()){
            QString name;
#ifdef WEBENGINEVIEW
            if(QWebEngineProfile *widget = qobject_cast<QWebEngineProfile*>(profile))
                name = widget->isOffTheRecord() ? QString() : widget->storageName();
            else
#endif
                name = profile->property("storageName").toString();
            if(!name.isEmpty()) storages << name;
        }
        const QString edge = view->EngineProfileName();
        if(!edge.isEmpty()) storages << edge;
    }
    return storages;
}

QSet<QObject*> TreeBank::ProfilesOfViews(){
    QSet<QObject*> profiles;
    foreach(SharedView view, m_AllViews)
        if(QObject *profile = view->EngineProfile()) profiles << profile;
    return profiles;
}

bool TreeBank::RenameSpace(ViewNode *vn, const QString &before, const QString &after, bool removing){
    const QString from = before.split(QStringLiteral(";")).first();
    const QString to   = removing ? QString() : after.split(QStringLiteral(";")).first();
    const QStringList set = after.split(QStringLiteral(";")).mid(1);
    const QList<QObject*> profiles = NetworkController::SpaceProfiles(from);
    const SpaceRename::Places places = SpacePlaces(profiles);

    QString refusal;
    if(m_MovingSpace)
        refusal = tr("Another directory is still being renamed.");
    else if(DownloadItem::Running())
        refusal = removing ? tr("A download is in progress. Change this directory after it has finished.")
                           : tr("A download is in progress. Rename this directory after it has finished.");
    else if(CountSpaces(m_ViewRoot, vn, from) || CountSpaces(m_TrashRoot, vn, from))
        refusal = tr("Another directory (or one in the trash) is the same network space \"%1\". "
                     "Renaming this one would leave the other without its data.").arg(from);
    else if(!removing && NetworkController::StorageInUse(to))
        refusal = tr("The data of the network space \"%1\" is still being closed. "
                     "Wait a moment and try again.").arg(to);
    else if(!removing && !SpaceRename::Occupied(SpaceRename::Candidates(from, to, places)).isEmpty())
        refusal = tr("Data of a network space named \"%1\" is already on disk. "
                     "Choose another name.").arg(to);
    if(!refusal.isEmpty()){
        vn->SetTitle(before);
        if(vn == m_ViewRoot) m_RootName = before;
        ModelessDialog::Information(removing ? tr("Cannot change this directory.")
                                             : tr("Cannot rename this directory."), refusal);
        return false;
    }

    m_MovingSpace = true;
    m_MovingFrom = from;
    m_MovingTo = to;

    BusyDialog *progress = new BusyDialog
        (removing ? tr("Closing the data of \"%1\"...").arg(from)
                  : tr("Moving the data of \"%1\" to \"%2\"...").arg(from, to));
    progress->Open();

    const QSet<QObject*> ours(profiles.begin(), profiles.end());
    const QString edgeName = NetworkController::ProfileStorageName(from);
    QList<QPair<QPointer<TreeBank>, ViewNode*>> current;
    QList<QPointer<QObject>> going;
    LetGoOfViews(ours, QSet<QString>{edgeName}, current, going);
    SidePanels::UpdateAll();
    QList<QPointer<QObject>> releasing;
    foreach(QObject *profile, profiles) releasing << QPointer<QObject>(profile);
#ifdef EDGEWEBVIEW
    releasing << QPointer<QObject>(EdgeWebView::ReleaseExtensionsOf(edgeName));
#endif

    std::shared_ptr<SpaceWait> state(new SpaceWait);
    state->going = going;
    state->waited.start();
    state->then = [=](){
        NetworkController::ReleaseSpace(from);
        foreach(const QPointer<QObject> &object, going + releasing)
            NetworkController::NoteReleasing(from, object.data());
        if(removing){
            FinishRenameSpace(vn, from, QString(), set, current, progress);
            return;
        }
        std::shared_ptr<SpaceWait> released(new SpaceWait);
        released->going = releasing;
#ifdef EDGEWEBVIEW
        if(QObject *exit = EdgeWebView::BrowserExitIfNoViews()) released->going << QPointer<QObject>(exit);
#endif
        released->waited.start();
        released->then = [=](){
            SpaceRename::Mover *mover = new SpaceRename::Mover
                (SpaceRename::Candidates(from, to, places), &SpaceRename::Mover::DefaultMove,
                 20, 10000, qApp);
            QPointer<QObject> dialog(progress);
            mover->Start([=](const QList<SpaceRename::Move> &failed){
                mover->deleteLater();
#ifdef EDGEWEBVIEW
                EdgeWebView::ForgetMissingProfiles();
#endif
                FinishRenameSpace(vn, from, to, set, current, dialog.data());
                const QList<SpaceRename::Move> left = failed;
#ifdef VANILLA_PASSWORDS
                if(PasswordStore *store = PasswordStore::Instance()){
                    store->MoveSpace(from, to, [to, left](int passwords){ ReportRenameSpace(to, left, passwords);});
                    return;
                }
#endif
                ReportRenameSpace(to, left, 0);
            });
        };
        WaitForViews(released);
    };
    WaitForViews(state);
    return true;
}

void TreeBank::LetGoOfViews(const QSet<QObject*> &profiles, const QSet<QString> &edgeNames,
                            QList<QPair<QPointer<TreeBank>, ViewNode*>> &current,
                            QList<QPointer<QObject>> &going){
    ChangeScope changing;
    foreach(SharedView view, SharedViewList(m_AllViews)){
        QObject *profile = view->EngineProfile();
        if(!(profile && profiles.contains(profile)) && !edgeNames.contains(view->EngineProfileName())) continue;
        ViewNode *node = view->GetViewNode();
        TreeBank *tb = view->GetTreeBank();
        if(tb && IsLive(tb) && tb->GetCurrentView() == view) current << qMakePair(QPointer<TreeBank>(tb), node);
        going << QPointer<QObject>(view->base());
        if(node && node->GetView() == view.get()) DislinkView(node);
        else { RemoveFromAllViews(view); RemoveFromUpdateBox(view);}
    }
}

bool TreeBank::MergeSpace(ViewNode *vn, const QString &before, const QString &after){
    const QString from = before.split(QStringLiteral(";")).first();
    const QString to   = after .split(QStringLiteral(";")).first();
    const QStringList set = after.split(QStringLiteral(";")).mid(1);
    const bool shared = CountSpaces(m_ViewRoot, vn, from) || CountSpaces(m_TrashRoot, vn, from);
    const QList<QObject*> profiles = shared ? QList<QObject*>() : NetworkController::SpaceProfiles(from);
    const SpaceRename::Places places = SpacePlaces(NetworkController::SpaceProfiles(from));

    QString refusal;
    if(m_MovingSpace)
        refusal = tr("Another directory is still being renamed.");
    else if(DownloadItem::Running())
        refusal = tr("A download is in progress. Rename this directory after it has finished.");
    if(!refusal.isEmpty()){
        vn->SetTitle(before);
        if(vn == m_ViewRoot) m_RootName = before;
        ModelessDialog::Information(tr("Cannot rename this directory."), refusal);
        return false;
    }

    m_MovingSpace = true;
    m_MovingFrom = from;
    m_MovingTo = to;
    BusyDialog *progress = new BusyDialog(tr("Joining \"%1\" to \"%2\"...").arg(from, to));
    progress->Open();

    const QString edgeFrom = NetworkController::ProfileStorageName(from);
    const QString edgeTo = NetworkController::ProfileStorageName(to);
    QSet<QString> edgeNames{edgeTo};
    if(!shared) edgeNames << edgeFrom;
    QList<QPair<QPointer<TreeBank>, ViewNode*>> current;
    QList<QPointer<QObject>> going;
    LetGoOfViews(QSet<QObject*>(profiles.begin(), profiles.end()), edgeNames, current, going);
    SidePanels::UpdateAll();
#ifdef EDGEWEBVIEW
    for(const QString &name : std::as_const(edgeNames))
        going << QPointer<QObject>(EdgeWebView::ReleaseExtensionsOf(name));
#endif

    bool qtLeft = false;
    foreach(const SpaceRename::Move &move, SpaceRename::Candidates(from, to, places))
        if((move.what == SpaceRename::WidgetStorage || move.what == SpaceRename::QuickStorage) &&
           QFileInfo::exists(move.from)) qtLeft = true;
    const bool edgeThere = !places.edgeProfiles.isEmpty() &&
        QFileInfo::exists(places.edgeProfiles + QStringLiteral("WV2Profile_") + edgeFrom);

    QPointer<QObject> dialog(progress);
    SpaceRename::WhenGone(going, 10000, [=](bool allGone){
        const auto joined = [=](bool edgeOk){
            NetworkController::JoinSpace(from, to, set, !shared);
            if(!shared)
                for(const QPointer<QObject> &object : std::as_const(going))
                    NetworkController::NoteReleasing(from, object.data());
            m_MovingSpace = false;
            m_MovingFrom.clear();
            m_MovingTo.clear();
            if(QWidget *widget = qobject_cast<QWidget*>(dialog.data())){
                widget->hide();
                widget->deleteLater();
            }
            foreach(Node *nd, vn->GetChildren()) ReapplySpecificSettings(nd->ToViewNode());
            RebuildBelow(vn);
            MarkOffDisk(vn);
            for(const auto &shown : current){
                if(!shown.first || !IsLive(shown.first) || !shown.second) continue;
                if(shown.first->GetCurrentView()) continue;
                shown.first->SetCurrent(shown.second);
            }
#ifdef VANILLA_PASSWORDS
            if(!shared){
                if(PasswordStore *store = PasswordStore::Instance()){
                    store->MoveSpace(from, to, [=](int passwords){
                        ReportMergeSpace(from, to, !edgeOk, qtLeft, passwords);
                    });
                    return;
                }
            }
#endif
            ReportMergeSpace(from, to, !edgeOk, qtLeft, 0);
        };
#ifdef EDGEWEBVIEW
        if(edgeThere){
            if(!allGone){ joined(false); return;}
            EdgeWebView::CopyCookiesForMerge(edgeFrom, edgeTo, [joined](bool ok, int){ joined(ok);});
            return;
        }
#endif
        Q_UNUSED(allGone)
        joined(true);
    });
    return true;
}

void TreeBank::FinishRenameSpace(ViewNode *vn, const QString &from, const QString &to,
                                 const QStringList &set,
                                 const QList<QPair<QPointer<TreeBank>, ViewNode*>> &current,
                                 QObject *progress){
    if(to.isEmpty()) NetworkController::DropHeldCookies(from);
    else NetworkController::PlaceSpace(from, to, set);
    m_MovingSpace = false;
    m_MovingFrom.clear();
    m_MovingTo.clear();
    if(QWidget *dialog = qobject_cast<QWidget*>(progress)){
        dialog->hide();
        dialog->deleteLater();
    }
    foreach(Node *nd, vn->GetChildren()) ReapplySpecificSettings(nd->ToViewNode());
    if(to.isEmpty()) RebuildBelow(vn);
    MarkOffDisk(vn);
    for(const auto &shown : current){
        if(!shown.first || !IsLive(shown.first) || !shown.second) continue;
        if(shown.first->GetCurrentView()) continue;
        shown.first->SetCurrent(shown.second);
    }
}

void TreeBank::ApplySpecificSettings(ViewNode *vn){
    if(!vn) return;

    NetworkController::GetNetworkAccessManager(GetNetworkSpaceId(vn),
                                               GetNodeSettings(vn));
    ReapplySpecificSettings(vn);
    MarkOffDisk(vn);
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

bool TreeBank::IsOffTheRecord(ViewNode *vn){
    return vn && DirectoryPage::SaysPrivate(GetNodeSettings(vn));
}

static bool OffTheRecordBelow(ViewNode *vn, bool parentOff){
    return vn->IsDirectory() ? TreeBank::IsOffTheRecord(vn) : parentOff;
}

static void MarkOffDiskBelow(ViewNode *vn, bool off){
    vn->SetOffDisk(off);
    foreach(Node *nd, vn->GetChildren()){
        if(ViewNode *child = nd->ToViewNode())
            MarkOffDiskBelow(child, OffTheRecordBelow(child, off));
    }
}

void TreeBank::MarkOffDisk(ViewNode *vn){
    if(vn) MarkOffDiskBelow(vn, IsOffTheRecord(vn));
}

void TreeBank::SweepOffDisk(){
    MarkOffDisk(m_ViewRoot);
}

void TreeBank::DoUpdate(){
    foreach(SharedView view, m_ViewUpdateBox){
        view->UpdateThumbnail();
    }
    m_ViewUpdateBox.clear();
}

void TreeBank::DoDelete(){
    const NodeList ripe = std::exchange(m_NodeDeleteBox, NodeList());
    for(Node *nd : ripe){
        nd->Delete();
    }
}

void TreeBank::Discard(Node *nd, Node *formerParent){
    if(!nd || m_FreshDeleteBox.contains(nd) || m_NodeDeleteBox.contains(nd)) return;
    m_FreshDeleteBox << nd;
    m_Leaving << qMakePair(NodeList() << nd << nd->GetDescendants(), formerParent);
    if(m_DeleteBarrierBooked) return;
    m_DeleteBarrierBooked = true;
    WhenItMayChange([](){
        AutoSaver *saver = Application::GetAutoSaver();
        PassDeleteBarrier(!saver || !saver->IsSaving());
    });
}

void TreeBank::PassDeleteBarrier(bool mayFree){
    m_DeleteBarrierBooked = false;
    const QList<QPair<NodeList, Node*>> leaving = std::exchange(m_Leaving, {});
    const NodeList fresh = std::exchange(m_FreshDeleteBox, NodeList());

    QList<QSet<Node*>> sets;
    QSet<Node*> going;
    for(const auto &one : leaving){
        sets << QSet<Node*>(one.first.begin(), one.first.end());
        going.unite(sets.last());
    }
    auto landing = [&](Node *parent){
        for(int guard = 0; parent && going.contains(parent) && guard <= leaving.length(); guard++){
            Node *above = nullptr;
            for(int i = 0; i < leaving.length(); i++)
                if(sets[i].contains(parent)){ above = leaving[i].second; break;}
            parent = above;
        }
        return going.contains(parent) ? nullptr : parent;
    };
    QList<QPair<QSet<Node*>, Node*>> groups;
    for(int i = 0; i < leaving.length(); i++)
        groups << qMakePair(sets[i], landing(leaving[i].second));

    m_ClipboardNodes.removeIf([&going](Node *nd){ return going.contains(nd);});

    QList<MainWindow*> windows = Application::GetMainWindows().values();
    foreach(QWidget *widget, QApplication::topLevelWidgets())
        if(MainWindow *win = qobject_cast<MainWindow*>(widget))
            if(!windows.contains(win)) windows << win;
    for(MainWindow *win : std::as_const(windows)){
        if(TreeBar *bar = win->GetTreeBar()) bar->LetGoOf(going);
        if(TreeBank *tb = win->GetTreeBank())
            if(Gadgets *g = tb->GetGadgets())
                for(const auto &group : std::as_const(groups))
                    g->LetGoOf(group.first, group.second);
    }

    m_NodeDeleteBox << fresh;
    if(mayFree) DoDelete();
}

void TreeBank::AddToUpdateBox(SharedView view){
    if(!m_ViewUpdateBox.contains(view)){
        m_ViewUpdateBox << view;
    }
}

void TreeBank::RemoveFromUpdateBox(SharedView view){
    m_ViewUpdateBox.removeOne(view);
}

void TreeBank::AutoLoad(){
    if(!MayChangeFromOutside()) return;
    ChangeScope changing;
    AutoLoadWalk walk = AutoLoadExhausted;
    if(!m_MaxViewCount || m_AllViews.length() < m_MaxViewCount){
        if(m_TraverseCondition == 0){
            if(m_ViewIterForward)       walk = LoadViewForward();
            else if(m_ViewIterBackward) walk = LoadViewBackward();
        } else {
            if(m_ViewIterBackward)      walk = LoadViewBackward();
            else if(m_ViewIterForward)  walk = LoadViewForward();
        }
    }

    if(AutoLoadStops(m_ViewIterForward, m_ViewIterBackward,
                     m_AllViews.length(), m_MaxViewCount))
        Application::StopAutoLoadTimer();
    m_TraverseCondition = AutoLoadNextCondition(walk, m_TraverseCondition);
}

bool TreeBank::AutoLoadStops(bool forwardLeft, bool backwardLeft, int views, int maxViews){
    if(!forwardLeft && !backwardLeft) return true;
    return maxViews != 0 && views >= maxViews;
}

int TreeBank::AutoLoadNextCondition(AutoLoadWalk walk, int condition){
    if(walk == AutoLoadBudgetSpent) return condition;
    return condition ? 0 : 1;
}

TreeBank::AutoLoadWalk TreeBank::WalkToAutoLoad(ViewNode *&iter, bool forward,
                                                const std::function<bool(ViewNode*)> &load,
                                                int budget){
    bool pending = false;
    return WalkToAutoLoad(iter, forward, load, budget, pending);
}

TreeBank::AutoLoadWalk TreeBank::WalkToAutoLoad(ViewNode *&iter, bool forward,
                                                const std::function<bool(ViewNode*)> &load,
                                                int budget, bool &pending){
    bool askTrash = true;
    int steps = 0;
    while(ViewNode *vn = iter){
        if(askTrash && IsTrash(vn)){
            iter = nullptr;
            pending = false;
            return AutoLoadExhausted;
        }
        askTrash = false;
        if(steps >= budget) return AutoLoadBudgetSpent;
        if(pending) pending = false;
        else iter = vn = forward ? vn->Next() : vn->Prev();
        steps++;
        if(!vn) return AutoLoadExhausted;
        if(vn->GetView() || vn->IsDirectory() || vn->GetUrl().isEmpty()) continue;
        if(load(vn)) return AutoLoadLoaded;
        askTrash = true;
    }
    return AutoLoadExhausted;
}

TreeBank::AutoLoadWalk TreeBank::LoadViewForward(){
    ChangeScope changing;
    return WalkToAutoLoad(m_ViewIterForward, true,
                          [](ViewNode *vn){ return !!AutoLoadWithLink(vn);},
                          AUTO_LOAD_WALK_BUDGET, m_ViewIterForwardPending);
}

TreeBank::AutoLoadWalk TreeBank::LoadViewBackward(){
    ChangeScope changing;
    return WalkToAutoLoad(m_ViewIterBackward, false,
                          [](ViewNode *vn){ return !!AutoLoadWithLink(vn);},
                          AUTO_LOAD_WALK_BUDGET, m_ViewIterBackwardPending);
}

void TreeBank::SteerAutoLoadWalk(Node *node, bool reorder, ViewNode *&iter, bool &pending, bool forward){
    if(!iter || !node || !node->IsViewNode()) return;
    bool under = false;
    for(Node *nd = reorder ? iter->GetParent() : iter; nd; nd = nd->GetParent()){
        if(nd == node){ under = true; break;}
    }
    if(!under) return;

    ViewNode *vn = node->ToViewNode();
    if(reorder){
        if(forward){
            iter = vn;
        } else {
            Node *last = vn;
            while(!last->HasNoChildren()) last = last->GetLastChild();
            iter = last->ToViewNode();
        }
        pending = true;
        return;
    }
    if(forward){
        Node *nd = vn;
        while(nd->GetParent() && nd->GetLastSibling() == nd)
            nd = nd->GetParent();
        if(nd->IsRoot() || !nd->GetParent()){
            iter = nullptr;
        } else {
            const NodeList siblings = nd->GetSiblings();
            iter = siblings[siblings.indexOf(nd) + 1]->ToViewNode();
        }
    } else {
        iter = vn->Prev();
    }
    pending = iter != nullptr;
}

void TreeBank::SteerAutoLoadWalks(Node *node, bool reorder){
    SteerAutoLoadWalk(node, reorder, m_ViewIterForward,  m_ViewIterForwardPending,  true);
    SteerAutoLoadWalk(node, reorder, m_ViewIterBackward, m_ViewIterBackwardPending, false);
}

bool TreeBank::AutoLoadRestsIn(Node *node){
    if(!node) return false;
    foreach(Node *iter, QList<Node*>() << m_ViewIterForward << m_ViewIterBackward){
        for(Node *nd = iter; nd; nd = nd->GetParent())
            if(nd == node) return true;
    }
    return false;
}

void TreeBank::RewalkAutoLoadFromCurrent(){
    MainWindow *win = Application::GetCurrentWindow();
    TreeBank *tb = win ? win->GetTreeBank() : nullptr;
    ViewNode *current = tb ? tb->GetCurrentViewNode() : nullptr;
    if(current && !IsTrash(current)){
        SetViewIterForward(current);
        SetViewIterBackward(current);
    } else if(m_ViewRoot){
        SetViewIterForward(m_ViewRoot);
        m_ViewIterForwardPending = true;
        SetViewIterBackward(nullptr);
    }
    Application::RestartAutoLoadTimer();
}

Node* TreeBank::GetRoot(Node* nd){
    if(!nd) return nullptr;
    while(nd->GetParent() && !nd->IsRoot())
        nd = nd->GetParent();
    return nd;
}

bool TreeBank::IsTrash(Node* nd){
    if(nd->IsViewNode())
        return GetRoot(nd) == m_TrashRoot;
    return false;
}

void TreeBank::ConnectSeekToMiniMap(){
    connect(m_Receiver, &Receiver::SeekText, m_MiniMap, [this](const QString &words){
        if(m_MiniMap && !IsDisplayingTableView()) m_MiniMap->SetSeekTerm(words);
    });
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
    for(int key : windows.keys()){
        if(windows[key] && windows[key]->GetTreeBank() == this)
            return key;
    }
    return 0;
}

int TreeBank::WinIndex(Node* nd){
    WinMap windows = Application::GetMainWindows();
    for(int key : windows.keys()){
        if(windows[key] && windows[key]->GetTreeBank()->IsCurrent(nd))
            return key;
    }
    return 0;
}

int TreeBank::WinIndex(SharedView view){
    WinMap windows = Application::GetMainWindows();
    for(int key : windows.keys()){
        if(windows[key] && windows[key]->GetTreeBank()->IsCurrent(view))
            return key;
    }
    return 0;
}

void TreeBank::LiftMaxViewCountIfNeed(int now){
    if(m_MaxViewCount <= now)
        m_MaxViewCount = now + 1;
}

static TreeSerializer::Hooks TreeHooks(TreeBank::Restoration *restored){
    TreeSerializer::Hooks hooks;

    hooks.KeepsSideFiles = [](ViewNode *nd){ return !TreeBank::IsTrash(nd);};

    hooks.RestoreIntoWindow = [restored](ViewNode *nd, int id){
        MainWindow *win = TreeBank::RestoreIntoWindow(nd, id);
        restored->windowIds << id;
        restored->lastWindow = win;
    };

    hooks.Tick = [](){ Application::processEvents();};

    return hooks;
}

static TreeSerializer::Hooks TreeSaveHooks(const TreeBank::WindowIndexMap &windowIndices,
                                           const QSet<quint64> &leftOut){
    TreeSerializer::Hooks hooks;
    hooks.WindowIndexOf = [windowIndices](ViewNode *nd){
        return windowIndices.value(nd, 0);
    };
    hooks.KeepsSideFiles = [](ViewNode *nd){ return !TreeBank::IsTrash(nd) && !nd->IsOffDisk();};
    if(!leftOut.isEmpty())
        hooks.Skips = [leftOut](ViewNode *nd){ return nd && leftOut.contains(nd->GetSerial());};
    return hooks;
}

MainWindow *TreeBank::RestoreIntoWindow(ViewNode *nd, int id){
    LoadWithLink(nd);

    MainWindow *win = Application::Window(id);
    if(win){
        Application::SetCurrentWindow(win);
    } else {
        win = Application::NewWindow(id, QPoint(), TreeBank::PurgeView());
    }
    TreeBank *tb = win->GetTreeBank();
    tb->blockSignals(true);
    tb->SetCurrent(nd);
    tb->blockSignals(false);
    win->ShowWithFirstFrame();
    return win;
}

TreeBank::Restoration TreeBank::LoadTree(){
    ChangeScope changing;
    Node::SetBooting(true);

    Restoration restored;
    QString datadir = Application::StateDirectory();
    TreeSerializer::Hooks hooks = TreeHooks(&restored);

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
                    (tr("Restored from a backup file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
                     tr("Because of a failure to read the latest file, it was restored from a backup file."));
            }
        }

        if(!check){

            QDir dir = QDir(datadir);
            QStringList list =
                dir.entryList(Application::BackUpFileFilters(),
                              QDir::NoFilter, QDir::Name | QDir::Reversed);

            for(QString backup : std::as_const(list)){

                if(!backup.endsWith(filename)) continue;
                check = TreeSerializer::ReadJsonFile(datadir + backup, root, hooks);

                if(!check) continue;

                ModelessDialog::Information
                    (tr("Restored from a backup file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
                     tr("Because of a failure to read the latest file, it was restored from a backup file."));
                break;
            }
        }

        if(root == m_ViewRoot)
            EmitTreeStructureChanged();
    }
    SweepOffDisk();
    Node::SetBooting(false);
    return restored;
}

void TreeBank::UpdateCurrentThumbnails(){
    for(MainWindow *win : Application::GetMainWindows().values()){
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

void TreeBank::LeaveOutOfSaves(quint64 serial){
    if(serial) m_LeftOutOfSaves[serial]++;
}

void TreeBank::PutBackIntoSaves(quint64 serial){
    auto it = m_LeftOutOfSaves.find(serial);
    if(it == m_LeftOutOfSaves.end()) return;
    if(--it.value() <= 0) m_LeftOutOfSaves.erase(it);
}

QSet<quint64> TreeBank::LeftOutOfSaves(){
    QSet<quint64> serials;
    for(auto it = m_LeftOutOfSaves.cbegin(); it != m_LeftOutOfSaves.cend(); ++it)
        serials.insert(it.key());
    return serials;
}

bool TreeBank::SaveTree(const WindowIndexMap &windowIndices, const QSet<quint64> &leftOut){

    QString datadir = Application::StateDirectory();
    TreeSerializer::Hooks hooks = TreeSaveHooks(windowIndices, leftOut);

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

void TreeBank::LoadSettings(){
    Settings &s = Application::GlobalSettings();

    m_RootName             = s.value(QStringLiteral("application/@RootName"), QStringLiteral("root;id")).value<QString>();
    m_MaxViewCount         = s.value(QStringLiteral("application/@MaxViewCount")         , 10)   .value<int>();
    m_MaxTrashEntryCount   = s.value(QStringLiteral("application/@MaxTrashEntryCount")   , 500)  .value<int>();
    m_TraverseAllView      = s.value(QStringLiteral("application/@TraverseAllView")      , false).value<bool>();
    m_PurgeNotifier        = s.value(QStringLiteral("application/@PurgeNotifier")        , false).value<bool>();
    m_PurgeReceiver        = s.value(QStringLiteral("application/@PurgeReceiver")        , false).value<bool>();
    m_PurgeView            = s.value(QStringLiteral("application/@PurgeView")            , false).value<bool>();
    m_EnableNotifierScrollArea = s.value(QStringLiteral("application/@EnableNotifierScrollArea"), false).value<bool>();

    QString viewport = s.value(QStringLiteral("application/@Viewport"), QStringLiteral("Widget")).value<QString>();
    if(viewport == QStringLiteral("Widget"))       m_Viewport = Widget;
    if(viewport == QStringLiteral("GLWidget"))     m_Viewport = OpenGLWidget;
    if(viewport == QStringLiteral("OpenGLWidget")) m_Viewport = OpenGLWidget;

    if(m_MaxViewCount == -1)       m_MaxViewCount       = 10;
    if(m_MaxTrashEntryCount == -1) m_MaxTrashEntryCount = 500;

    {
        m_KeyMap = InputMapSchema::DefaultKeyMap(QStringLiteral("application/keymap"));
        InputMap::LoadKeyMap(s, QStringLiteral("application/keymap"), m_KeyMap, InputMap::Hooks(IsValidAction));
    }
    {
        m_MouseMap = InputMapSchema::DefaultGestureMap(QStringLiteral("application/mouse"));
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
    s.setValue(QStringLiteral("application/@EnableNotifierScrollArea"), m_EnableNotifierScrollArea);

    if(m_Viewport == Widget)       s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("Widget"));
    if(m_Viewport == OpenGLWidget) s.setValue(QStringLiteral("application/@Viewport"), QStringLiteral("OpenGLWidget"));

    InputMap::SaveKeyMap(s, QStringLiteral("application/keymap"), m_KeyMap);

    InputMap::SaveGestureMap(s, QStringLiteral("application/mouse"), m_MouseMap);
    Node::SaveSettings();
    View::SaveSettings();
    Gadgets::SaveSettings();
}

bool TreeBank::DrawsOnOpenGL(){
    return m_Viewport == OpenGLWidget && !Application::EnableTransparentWindow();
}

void TreeBank::QuarantineViewNode(ViewNode *vn){
    ReleaseAutoLoadIterators(vn);

    TreeBank *tb = vn->GetView() ? vn->GetView()->GetTreeBank() : nullptr;
    if(!tb) return;

    if(vn == tb->GetCurrentViewNode())  tb->SetCurrentViewNode(nullptr);
}

void TreeBank::ReleaseAutoLoadIterators(ViewNode *vn){
    if(vn == m_ViewIterForward)  SetViewIterForward(nullptr);
    if(vn == m_ViewIterBackward) SetViewIterBackward(nullptr);
}

void TreeBank::DisownNode(Node *nd){
    SteerAutoLoadWalks(nd, false);
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
    StripSubTree(vn);
    const int keep = qMax(0, m_MaxTrashEntryCount);
    while(m_TrashRoot->ChildrenLength() > keep)
        DiscardFromTrash(m_TrashRoot->GetLastChild());
    return true;
}

void TreeBank::DiscardFromTree(ViewNode *vn){
    SteerAutoLoadWalks(vn, false);
    Node *prevparent = vn->GetParent();
    NodeList all = NodeList() << vn << vn->GetDescendants();
    StripSubTree(vn);
    for(Node *nd : std::as_const(all)){
        if(ViewNode *each = nd->ToViewNode()) each->SetOffDisk(true);
    }
    DisownNode(vn);
    NodeList deleted = NodeList() << vn;
    EmitNodeDeleted(deleted);
    ForgetNodeItems(all);
    Discard(vn, prevparent);
}

bool TreeBank::WhollyOffTheRecord(ViewNode *vn, bool off){
    if(!off) return false;
    foreach(Node *nd, vn->GetChildren()){
        ViewNode *child = nd->ToViewNode();
        if(child && !WhollyOffTheRecord(child, OffTheRecordBelow(child, off))) return false;
    }
    return true;
}

int TreeBank::PruneOffTheRecord(ViewNode *vn, bool off){
    int pruned = 0;
    const NodeList children = vn->GetChildren();
    for(Node *nd : children){
        ViewNode *child = nd->ToViewNode();
        if(!child) continue;
        const bool childOff = OffTheRecordBelow(child, off);
        if(WhollyOffTheRecord(child, childOff)){
            DiscardFromTree(child);
            pruned++;
        } else {
            pruned += PruneOffTheRecord(child, childOff);
        }
    }
    return pruned;
}

bool TreeBank::IsDiscarded(Node *nd){
    for(; nd; nd = nd->GetParent()){
        if(m_FreshDeleteBox.contains(nd) || m_NodeDeleteBox.contains(nd)) return true;
    }
    return false;
}

bool TreeBank::DiscardFromTrash(Node *nd){
    if(!nd || nd == m_TrashRoot || !IsTrash(nd)) return false;
    Node *parent = nd->GetParent();
    DisownNode(nd);
    StripSubTree(nd);
    NodeList forgotten = NodeList() << nd << nd->GetDescendants();
    ForgetNodeItems(forgotten);

    Discard(nd, parent);
    return true;
}

namespace {
    std::function<bool(int)> DiscardConfirmer;
}

void TreeBank::SetDiscardConfirmer(std::function<bool(int)> confirmer){
    DiscardConfirmer = confirmer;
}

void TreeBank::DiscardFromTrashAsked(const QList<quint64> &serials, int tabs, const QString &title,
                                     std::function<void()> settled, const QList<quint64> &kept){
    WhenItMayChange([serials, tabs, title, settled, kept](){
        const bool sure = DiscardConfirmer ? DiscardConfirmer(tabs)
            : ModalDialog::Question(title, tr("Delete %1 closed tabs from the wastebasket for good?\n"
                                              "This cannot be undone.").arg(tabs));
        if(!sure){
            if(settled) settled();
            return;
        }
        WhenItMayChange([serials, settled, kept](){
            ChangeScope changing;
            const QSet<quint64> asked(serials.begin(), serials.end());
            const QSet<quint64> keeping(kept.begin(), kept.end());
            QHash<quint64, Node*> now;
            foreach(Node *nd, m_TrashRoot->GetDescendants())
                if(asked.contains(nd->GetSerial())) now.insert(nd->GetSerial(), nd);
            for(quint64 serial : serials){
                Node *nd = now.value(serial);
                if(!nd || !IsTrash(nd)) continue;
                Node *parent = nd->GetParent();
                DiscardFromTrash(nd);
                while(parent && parent != m_TrashRoot && parent->HasNoChildren()
                      && !keeping.contains(parent->GetSerial())){
                    Node *above = parent->GetParent();
                    DiscardFromTrash(parent);
                    parent = above;
                }
            }
            if(settled) settled();
        });
    });
}

void TreeBank::StripSubTree(Node *nd){
    if(ViewNode *vn = nd->ToViewNode()){
        SharedView held = vn->GetView() ? vn->GetView()->GetThis().lock() : SharedView();
        QuarantineViewNode(vn);
        DislinkView(vn);
        if(held) held->ForgetViewNode(vn);
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

    if(view.get() != m_ExtractingDownloadCarrier) view->KeepForDownloads();

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

    SteerAutoLoadWalks(vn, false);
    QuarantineViewNode(vn);
    m_ExtractingDownloadCarrier = held.get();
    DislinkView(vn);
    m_ExtractingDownloadCarrier = nullptr;

    held->Orphan();

    DisownNode(vn);
    NodeList deleted = NodeList() << vn;
    EmitNodeDeleted(deleted);

    ForgetNodeItems(deleted);

    Discard(vn, prevparent);

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
    NodeList above;
    for(Node *nd = prevparent; nd; nd = nd->GetParent()) above << nd;

    if(sample->IsViewNode()){
        int moved = 0;
        int discarded = 0;

        foreach(Node *nd, list){
            Q_ASSERT(nd->IsViewNode());
            ViewNode *vn = nd->ToViewNode();
            if(vn->IsRoot() || GetRoot(vn) == m_TrashRoot || IsDiscarded(vn)) continue;
            const bool off = IsOffTheRecord(vn);
            if(WhollyOffTheRecord(vn, off)){
                DiscardFromTree(vn);
                discarded++;
                continue;
            }
            discarded += PruneOffTheRecord(vn, off);
            if(MoveToTrash(vn)) moved++;
        }
        if(!moved && !discarded) return false;
        prevparent = m_ViewRoot;
        foreach(Node *nd, above){
            if(!IsDiscarded(nd) && GetRoot(nd) == m_ViewRoot){ prevparent = nd; break;}
        }

        if(NoticesTrash(moved, m_MaxTrashEntryCount)) NoticeTrash(moved);

    } else {
        return false;
    }

    if(!m_CurrentView){
        if(SelectCurrentAfterDelete(prevparent)) return true;
    }
    return true;
}

bool TreeBank::CloseWithoutTrash(ViewNode *vn){
    ChangeScope changing;
    if(!vn || vn->IsDirectory() || !vn->HasNoChildren() || vn == m_CurrentViewNode) return false;
    Node *prevparent = vn->GetParent();
    if(!prevparent || GetRoot(vn) != m_ViewRoot) return false;
    if(vn->GetView() && vn->GetView()->GetTreeBank() != this) return false;
    foreach(MainWindow *win, Application::GetMainWindows())
        if(win->GetTreeBank() && win->GetTreeBank()->GetCurrentViewNode() == vn) return false;

    SharedView held = vn->GetView() ? vn->GetView()->GetThis().lock() : SharedView();
    SteerAutoLoadWalks(vn, false);
    QuarantineViewNode(vn);
    DislinkView(vn);
    if(held) held->ForgetViewNode(vn);

    DisownNode(vn);
    NodeList deleted = NodeList() << vn;
    EmitNodeDeleted(deleted);
    ForgetNodeItems(deleted);
    Discard(vn, prevparent);
    return true;
}

int TreeBank::KeptInTrash(int moved, int keep){
    return qBound(0, keep, qMax(0, moved));
}

bool TreeBank::NoticesTrash(int moved, int keep){
    return moved >= 2 && KeptInTrash(moved, keep) > 0;
}

void TreeBank::NoticeTrash(int moved){
    const int kept = KeptInTrash(moved, m_MaxTrashEntryCount);

    ModelessDialog *dialog = new ModelessDialog();
    connect(this, &QObject::destroyed, dialog, &ModelessDialog::Discard);
    dialog->SetTitle(tr("Moved %1 nodes to the trash.").arg(moved));
    dialog->SetCaption(kept == moved
                       ? tr("They can be restored from the trash.")
                       : tr("The trash keeps a limited number of items, "
                            "so the oldest of these are gone."));
    dialog->SetButtons(Dialog::OpenTrash);
    dialog->SetCallBack([this](bool open){
        if(open && m_Gadgets && !m_Gadgets->IsDisplaying(Gadgets::TrashTree))
            DisplayTrashTree();
    });
    QTimer::singleShot(0, dialog, &ModelessDialog::Execute);
}

bool TreeBank::MoveNode(Node *nd, Node *dir, int n){
    if(nd->GetParent() == dir){
        dir->MoveChild(dir->ChildrenIndexOf(nd),
                       (n < 0 || n >= dir->ChildrenLength()) ?
                         dir->ChildrenLength() - 1 :
                       (n > dir->ChildrenIndexOf(nd)) ?
                         n - 1 : n);
        SteerAutoLoadWalks(dir, true);
        EmitTreeStructureChanged();
    } else {
        bool wasTrash = IsTrash(nd);
        const bool walkedIn = AutoLoadRestsIn(nd);
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
        if(walkedIn && !isTrash) RewalkAutoLoadFromCurrent();
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

    bool walkedIn = false;
    for(int round = 0; round <= children.length(); round++){
        bool moved = false;
        foreach(Node *child, children){
            if(!child->GetParent() || child->GetParent() == parent || !AutoLoadRestsIn(child)) continue;
            walkedIn = true;
            moved = true;
            SteerAutoLoadWalks(child, false);
        }
        if(!moved) break;
    }

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
    SteerAutoLoadWalks(parent, true);
    if(walkedIn && !IsTrash(parent)) RewalkAutoLoadFromCurrent();
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

    if(m_Receiver) m_Receiver->Dismiss();
    if(m_Notifier){
        m_Notifier->ResetStatus();
        m_Notifier->ResetLink();
    }

    if(m_Gadgets && m_Gadgets->IsActive()) m_Gadgets->Dismiss();

    SharedView prev = m_CurrentView;

    m_CurrentView = nd->GetView() ? nd->GetView()->GetThis().lock() : SharedView();
    m_CurrentViewNode = nd->ToViewNode();

    m_CurrentViewNode->ResetPrimaryPath();

    m_CurrentViewNode->SetLastAccessDateToCurrent();

    SetViewIterForward(m_CurrentViewNode);
    SetViewIterBackward(m_CurrentViewNode);

    if(!m_CurrentView){
        m_CurrentView = LoadWithLink(m_CurrentViewNode);

        RaiseDisplayedViewPriority();
    }
    Q_ASSERT(m_CurrentView);

    if(prev != m_CurrentView){
        GetMainWindow()->CurrentViewChanged();
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
        GetMainWindow()->raise();
        m_CurrentView->raise();
    }
    GetMainWindow()->SetInspectorPane(m_CurrentView->InspectorPane());
    if(SidePanels *panels = GetMainWindow()->GetSidePanels()) panels->Update();

    GetMainWindow()->RaiseAllEdgeWidgets();
    RestackChildWidgets();

    if(m_Gadgets && !m_Gadgets->IsActive() &&
       !GetMainWindow()->GetTreeBar()->TabWindowVisible()){

        QTimer::singleShot(0, this, [this](){
            if(Application::GetCurrentWindow() != GetMainWindow()) return;
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
    }

    RestackChildWidgets(OverviewUp);
    m_View->setFocus();
}

void TreeBank::PaintGadgetsNow(){
    m_View->viewport()->repaint();
    if(m_CurrentView) m_CurrentView->OnBeforeStartingDisplayGadgets();
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

void TreeBank::KeyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
}

void TreeBank::KeyReleaseEvent(QKeyEvent *ev){
    QWidget::keyReleaseEvent(ev);
}

namespace {

    class SwitchingScope {
    public:
        SwitchingScope() : m_Was(View::GetSwitchingState()){
            if(!m_Was) View::SetSwitchingState(true);
        }
        ~SwitchingScope(){
            if(!m_Was) View::SetSwitchingState(false);
        }
        SwitchingScope(const SwitchingScope&) = delete;
        SwitchingScope &operator=(const SwitchingScope&) = delete;
    private:
        const bool m_Was;
    };
}

SharedView TreeBank::OpenInNewViewNode(QNetworkRequest req, bool activate, ViewNode *older){
    ChangeScope changing;
    SwitchingScope switching;

    if(!older) older = m_CurrentViewNode;
    if(!older){
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
    SwitchingScope switching;

    if(!parent) parent = m_ViewRoot;
    if(!parent){
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
    SwitchingScope switching;

    if(!older) older = m_CurrentViewNode;
    if(!older){
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
    connect(menu, &QMenu::aboutToShow, this, [this, menu](){ ShowKeys(menu);});

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
    connect(menu, &QMenu::aboutToShow, this, [this, menu](){ ShowKeys(menu);});

    GetMainWindow()->AddDisplayMenuActions(menu);
    UpdateAction();

    return menu;
}

QMenu *TreeBank::WindowMenu(){
    QMenu *menu = new QMenu(tr("Window"), this);
    menu->setToolTipsVisible(true);
    connect(menu, &QMenu::aboutToShow, this, [this, menu](){ ShowKeys(menu);});

    menu->addAction(Action(_ToggleFullScreen));
    menu->addAction(Action(_ToggleMaximized));
    menu->addAction(Action(_ToggleMinimized));
    if(Application::EnableCustomTitleBar())
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
    connect(menu, &QMenu::aboutToShow, this, [this, menu](){ ShowKeys(menu);});

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
    connect(menu, &QMenu::aboutToShow, this, [this, menu](){ ShowKeys(menu);});

    menu->addAction(Action(_Import));
    menu->addAction(Action(_Export));
    menu->addAction(Action(_OpenSettings));
    menu->addAction(Action(_OpenDirectorySettings));
    menu->addAction(Action(_ShowDownloads));
    menu->addAction(Action(_OpenDownloads));
    menu->addAction(Action(_OpenPasswords));
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
    connect(menu, &QMenu::aboutToShow, this, [this, menu](){ ShowKeys(menu);});

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

    foreach(QAction *action, menu->actions())
        if(QMenu *sub = action->menu()) sub->setParent(menu, sub->windowFlags());

    ShowKeys(menu);

    return menu;
}

InputMap::KeyLayers TreeBank::KeyLayers() const {
    if(m_Gadgets && m_Gadgets->IsActive() && !m_Gadgets->IsDisplaying(Gadgets::AccessKey))
        return Gadgets::KeyLayers();
    if(m_CurrentView) return View::KeyLayers();
    InputMap::KeyLayer application;
    application.map = m_KeyMap;
    application.singleKeys = View::EnableSingleKeyShortcut();
    return InputMap::KeyLayers() << application;
}

void TreeBank::ShowKeys(QMenu *menu) const {
    MenuKeys::Show(menu, InputMap::ShownKeys(KeyLayers()));
}

void TreeBank::PurgeChildWidgetsIfNeed(){
    if(m_Notifier && !m_PurgeNotifier && !m_PurgeView) m_Notifier->Purge();
    if(m_Receiver && !m_PurgeReceiver && !m_PurgeView) m_Receiver->Purge();
}

void TreeBank::JoinChildWidgetsIfNeed(){
    if(m_Notifier && !m_PurgeNotifier && !m_PurgeView) m_Notifier->Join();
    if(m_Receiver && !m_PurgeReceiver && !m_PurgeView) m_Receiver->Join();
}

int TreeBank::MiniMapStripWidth() const {
    return m_MiniMap && m_MiniMap->IsActive() ? m_MiniMap->MapWidth() : 0;
}

QSize TreeBank::ViewSize() const {
    return QSize(width() - MiniMapStripWidth(), height());
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
    if(m_Receiver){
        m_Receiver->ResizeNotify(ev->size());
        m_Receiver->RefillIfShown();
    }
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
    DirectoryPage::ClearPendingSubject();
    QUrl url = SettingsSchemeHandler::SettingsUrl();
    if(!category.isEmpty()) url.setFragment(category);

    if(ViewNode *found = FindByUrl(m_ViewRoot, url)){
        const bool already = found->GetUrl() == url;
        if(!category.isEmpty()) found->SetUrl(url);
        View *view = found->GetView();
        const bool kept = view && view->GetTreeBank() == this &&
            KindOf(view) == ViewKindFor(url, GetNodeSettings(found));
        if(kept) SetCurrent(found);
        else Recreate(found);
        if(!category.isEmpty() && !already)
            if(View *now = found->GetView()) now->Load(url);
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
static ViewNode *SettingsPageIn(Node *directory){
    if(!directory) return nullptr;
    foreach(Node *nd, directory->GetChildren()){
        ViewNode *vn = nd->ToViewNode();
        if(vn && VanillaPage::IsSettingsUrl(vn->GetUrl())) return vn;
    }
    return nullptr;
}

void TreeBank::OpenDirectorySection(ViewNode *directory, std::function<void(const QUrl&)> place){
    QUrl url = SettingsSchemeHandler::SettingsUrl();
    url.setFragment(QStringLiteral("directory"));
    DirectoryPage::ClearPendingSubject();
    if(ViewNode *found = SettingsPageIn(directory)){
        const bool already = found->GetUrl() == url;
        found->SetUrl(url);
        Recreate(found);
        if(!already)
            if(View *view = found->GetView()) view->Load(url);
        return;
    }
    place(url);
    DirectoryPage::SetPendingSubject(directory);
}
#endif

void TreeBank::OpenDirectorySettings(){
#ifdef WEBENGINEVIEW
    OpenDirectorySection(DirectoryOf(m_CurrentViewNode),
                         [this](const QUrl &url){ OpenByCommandOperation(url);});
#endif
}

void TreeBank::OpenDirectorySettings(ViewNode *subject){
#ifdef WEBENGINEVIEW
    ViewNode *directory = DirectoryOf(subject);
    OpenDirectorySection(directory, [this, directory](const QUrl &url){
        OpenOnSuitableNode(url, true, directory);
    });
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

void TreeBank::OpenPasswords(){
    OpenSettings(QStringLiteral("passwords"));
}

void TreeBank::OpenDownloads(){
    OpenSettings(QStringLiteral("downloads"));
}

void TreeBank::ShowDownloads(){
    DownloadPanel::ShowFor(GetMainWindow(), true, true);
}

void TreeBank::ClearPasswords(){
    if(!ModalDialog::Question(tr("Clear saved passwords."),
                              tr("Every saved password is deleted."))) return;
    NetworkController::ClearPasswords();
}

void TreeBank::ToggleMiniMap(){
    if(!m_MiniMap && !CanHaveMiniMap()) return;
    if(m_MiniMap){
        m_MiniMap->hide();
        m_MiniMap->deleteLater();
        m_MiniMap = nullptr;
    } else {
        m_MiniMap = new MiniMap(this);
        if(m_Receiver) ConnectSeekToMiniMap();
        m_MiniMap->SetShelved(GetMainWindow()->IsContentFullScreen());
        m_MiniMap->ResizeNotify(size());
        if(m_CurrentView) m_MiniMap->SetView(m_CurrentView.get());
        if(m_Receiver && m_Receiver->isVisible()){
            const QString words = m_Receiver->SoughtWords();
            if(!words.isEmpty()) m_MiniMap->SetSeekTerm(words);
        }
        RestackChildWidgets();
    }
    const QSize viewSize = ViewSize();
    ResizeViewArea(viewSize);
    if(m_CurrentView) m_CurrentView->resize(viewSize);
    if(m_Notifier) m_Notifier->ResizeNotify(size());
    if(m_Receiver) m_Receiver->ResizeNotify(size());
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

bool TreeBank::RestoreBeside(ViewNode *vn, ViewNode *beside){
    if(!vn || !IsTrash(vn)) return false;
    if(beside && beside->GetParent() && GetRoot(beside) == m_ViewRoot)
        return MoveNode(vn, beside->GetParent(), beside->SiblingInsertIndex());
    return MoveNode(vn, m_ViewRoot);
}

void TreeBank::Recreate(ViewNode *vn){
    ChangeScope changing;
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;
    DislinkView(vn);
    SetCurrent(vn);
}

namespace {

    bool CurrentSomewhere(Node *nd){
        TreeBank *tb = nd->GetView() ? nd->GetView()->GetTreeBank() : nullptr;
        return tb && tb->GetCurrentViewNode() == nd;
    }

    bool NotOpenable(Node *nd){
        return (!nd->GetView() && nd->GetUrl().isEmpty()) || CurrentSomewhere(nd);
    }
}

void TreeBank::NextView(ViewNode *vn){
    if(!vn) vn = m_CurrentViewNode;
    if(!vn) return;

    Node *next = vn;
    if(m_TraverseAllView){
        do next = next->Next();
        while(next && NotOpenable(next));

        if(next && SetCurrent(next)){
        } else {
            next = m_ViewRoot;
            while(!next->HasNoChildren())
                next = next->GetFirstChild();

            while(next && NotOpenable(next)){
                if(next == vn) return;
                next = next->Next();
            }

            SetCurrent(next);
        }
    } else {
        const NodeList sibling = next->GetSiblings();
        auto step = [&sibling](Node *nd){
            const auto index = sibling.indexOf(nd);
            return index < sibling.length() - 1 ? sibling[index + 1] : sibling.first();
        };

        next = step(next);
        while(next && CurrentSomewhere(next)){
            if(next == vn) return;
            next = step(next);
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
        while(prev && NotOpenable(prev));

        if(prev && SetCurrent(prev)){
        } else {
            prev = m_ViewRoot;
            while(!prev->HasNoChildren())
                prev = prev->GetLastChild();

            while(prev && NotOpenable(prev)){
                if(prev == vn) return;
                prev = prev->Prev();
            }

            SetCurrent(prev);
        }
    } else {
        const NodeList sibling = prev->GetSiblings();
        auto step = [&sibling](Node *nd){
            const auto index = sibling.indexOf(nd);
            return index > 0 ? sibling[index - 1] : sibling.last();
        };

        prev = step(prev);
        while(prev && CurrentSomewhere(prev)){
            if(prev == vn) return;
            prev = step(prev);
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
        SharedView view = OpenOnSuitableNode(QNetworkRequest(START_URL),
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

    MarkOffDisk(vn);
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

void TreeBank::SetClipboardNodes(const NodeList &nodes, bool cut){
    m_ClipboardNodes = nodes;
    m_ClipboardIsCut = cut;
}

bool TreeBank::HasClipboardNodes(){
    return !m_ClipboardNodes.isEmpty();
}

bool TreeBank::ClipboardHolds(const NodeList &nodes){
    return QSet<Node*>(nodes.begin(), nodes.end()) ==
        QSet<Node*>(m_ClipboardNodes.begin(), m_ClipboardNodes.end());
}

bool TreeBank::PasteClipboardNodes(Node *parent, Node *anchor, bool before){
    ChangeScope changing;
    if(!parent || !parent->IsViewNode() || IsTrash(parent) || IsDiscarded(parent))
        return false;

    NodeList eligible;
    foreach(Node *nd, m_ClipboardNodes){
        if(IsTrash(nd) || IsDiscarded(nd)) continue;
        if(nd == parent || parent->IsDescendantOf(nd)) continue;
        eligible << nd;
    }
    NodeList nodes;
    foreach(Node *nd, eligible){
        bool below = false;
        foreach(Node *other, eligible)
            if(nd != other && nd->IsDescendantOf(other)) below = true;
        if(!below) nodes << nd;
    }
    if(nodes.isEmpty()) return false;

    const NodeList children = parent->GetChildren();
    const NodeList leaving = m_ClipboardIsCut ? nodes : NodeList();
    NodeList rest;
    foreach(Node *nd, children)
        if(!leaving.contains(nd)) rest << nd;

    int at = rest.length();
    const int anchorAt = anchor ? children.indexOf(anchor) : -1;
    if(anchorAt != -1 && before){
        for(int i = anchorAt; i < children.length(); i++){
            if(!leaving.contains(children[i])){
                at = rest.indexOf(children[i]);
                break;
            }
        }
    } else if(anchorAt != -1){
        at = 0;
        for(int i = anchorAt; i >= 0; i--){
            if(!leaving.contains(children[i])){
                at = rest.indexOf(children[i]) + 1;
                break;
            }
        }
    }

    NodeList placed = nodes;
    if(!m_ClipboardIsCut){
        placed.clear();
        ViewNode *dir = parent->ToViewNode();
        const bool offDisk = IsOffTheRecord(dir);
        foreach(Node *nd, nodes){
            ViewNode *vn = nd->ToViewNode();
            if(!vn) continue;
            MarkOffDisk(vn);
            if(ViewNode *clone = vn->Clone(dir, offDisk)){
                ApplySpecificSettings(clone);
                EmitNodeCreated(NodeList() << clone);
                placed << clone;
            }
        }
        if(placed.isEmpty()) return false;
    }

    if(!SetChildrenOrder(parent, rest.mid(0, at) + placed + rest.mid(at)))
        return false;
    if(m_ClipboardIsCut) m_ClipboardNodes.clear();
    return true;
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
    PaintGadgetsNow();
}

void TreeBank::DisplayTrashTree(ViewNode *vn){
    if(m_Gadgets->IsDisplaying(Gadgets::TrashTree))
        return m_Gadgets->Deactivate();
    m_Gadgets->Activate(Gadgets::TrashTree);
    if(vn && IsTrash(vn))
        m_Gadgets->SetCurrent(vn);
    else if(!m_TrashRoot->HasNoChildren())
        m_Gadgets->SetCurrent(m_TrashRoot->GetFirstChild());
    PaintGadgetsNow();
}

void TreeBank::DisplayAccessKey(SharedView view){
    if(m_Gadgets->IsDisplaying(Gadgets::AccessKey))
        return m_Gadgets->Deactivate();
    if(m_Gadgets->IsActive())
        return;
    m_Gadgets->Activate(Gadgets::AccessKey);
    if(!view) view = m_CurrentView;
    if(view) m_Gadgets->SetCurrentView(view.get());
    PaintGadgetsNow();
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
    m_Receiver->OpenCommand(view.get());
}

void TreeBank::OpenCommandAtField(QWidget *field){
    m_Receiver->OpenAtField(field);
}

bool TreeBank::WantsReceiverJoined() const {
    return !m_PurgeReceiver && !m_PurgeView && !(m_CurrentView && m_CurrentView->ForbidToOverlap());
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
    Action(_ToggleMenuBar)->setChecked(!GetMainWindow()->IsMenuBarEmpty());
    Action(_ToggleTreeBar)->setChecked(GetMainWindow()->GetTreeBar()->isVisible());
    Action(_ToggleToolBar)->setChecked(GetMainWindow()->GetToolBar()->isVisible());
    Action(_ToggleMiniMap)->setChecked(m_MiniMap);
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
    for(TreeBankAction a : m_ActionTable.keys())
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
        case _ToggleMenuBar:  action->setChecked(!GetMainWindow()->IsMenuBarEmpty()); break;
        case _ToggleTreeBar:  action->setChecked(GetMainWindow()->GetTreeBar()->isVisible()); break;
        case _ToggleToolBar:  action->setChecked(GetMainWindow()->GetToolBar()->isVisible()); break;
        case _ToggleMiniMap:  action->setChecked(m_MiniMap); break;
        default: break;
        }
        return action;
    }

    m_ActionTable[a] = action = new QAction(this);

    SetActionIcon(action, a);
    action->setObjectName(ActionToString(a));

    if(a != _NoAction){
        const QString label = ActionText::MenuLabel(ActionToString(a));
        action->setText(label);
        action->setToolTip(label);
    }

    switch(a){
    case _NoAction: break;

#define CONNECT_ACTION(name)                                            \
        case _##name:                                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name##Key()));                         \
            break;

        FOR_EACH_KEYBOARD_EVENTS(CONNECT_ACTION)

#undef  CONNECT_ACTION
#define CONNECT_ACTION(name)                                            \
        case _##name:                                                   \
            connect(action, SIGNAL(triggered()),                        \
                    this,   SLOT(name()));                              \
            break;

        FOR_EACH_APPLICATION_EVENTS(CONNECT_ACTION)
        FOR_EACH_NAVIGATION_EVENTS(CONNECT_ACTION)
        FOR_EACH_VIEW_EVENTS(CONNECT_ACTION)
        FOR_EACH_WEB_EVENTS1(CONNECT_ACTION)

#undef  CONNECT_ACTION
    }
    switch(a){

    case _ToggleMenuBar:
        action->setCheckable(true);
        action->setChecked(!GetMainWindow()->IsMenuBarEmpty());
        break;
    case _ToggleTreeBar:
        action->setCheckable(true);
        action->setChecked(GetMainWindow()->GetTreeBar()->isVisible());
        break;
    case _ToggleToolBar:
        action->setCheckable(true);
        action->setChecked(GetMainWindow()->GetToolBar()->isVisible());
        break;
    case _ToggleMiniMap:
        action->setCheckable(true);
        action->setChecked(m_MiniMap);
        action->setEnabled(CanHaveMiniMap());
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
    Application::InputScope key(true);
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

TreeBank::ViewKind TreeBank::ViewKindNamedIn(const QStringList &set){
#ifdef WEBENGINEVIEW
    if(set.indexOf(QRE("\\A"                 "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?\\Z")) != -1) return WebEngineKind;
    if(set.indexOf(QRE("\\A[gG](?:raphics)?" "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?\\Z")) != -1) return WebEngineKind;
    if(set.indexOf(QRE("\\A[qQ](?:uick)?"    "[wW](?:eb)?"                  "(?:[vV](?:iew)?)?\\Z")) != -1) return QuickWebEngineKind;
    if(set.indexOf(QRE("\\A"                 "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?\\Z")) != -1) return WebEngineKind;
    if(set.indexOf(QRE("\\A[gG](?:raphics)?" "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?\\Z")) != -1) return WebEngineKind;
    if(set.indexOf(QRE("\\A[qQ](?:uick)?"    "[wW](?:eb)?" "[eE](?:ngine)?" "(?:[vV](?:iew)?)?\\Z")) != -1) return QuickWebEngineKind;
#endif
#ifdef NATIVEWEBVIEW
    if(set.indexOf(QRE("\\A"                 "[nN](?:ative)?" "[wW](?:eb)?" "(?:[vV](?:iew)?)?\\Z")) != -1) return QuickNativeKind;
    if(set.indexOf(QRE("\\A[qQ](?:uick)?"    "[nN](?:ative)?" "[wW](?:eb)?" "(?:[vV](?:iew)?)?\\Z")) != -1) return QuickNativeKind;
#endif
#ifdef EDGEWEBVIEW
    if(set.indexOf(QRE("\\A"                 "[eE](?:dge)?" "(?:[wW](?:eb)?)?" "(?:[vV](?:iew)?)?\\Z")) != -1) return EdgeKind;
#endif
#ifdef LOCALVIEW
    if(set.indexOf(QRE("\\A"                 "[lL](?:ocal)?"                "(?:[vV](?:iew)?)?\\Z")) != -1) return LocalKind;
#endif
    Q_UNUSED(set)
    return NoView;
}

TreeBank::ViewKind TreeBank::ViewKindFor(const QUrl &url, const QStringList &set){
    const QString urlstr = url.toString();
    Q_UNUSED(urlstr)

#ifdef LOCALVIEW
    if(urlstr.startsWith(QStringLiteral("file:///")) && (QFileInfo(url.toLocalFile()).isDir() || LocalView::IsSupported(url))) return LocalKind;
#endif
    const ViewKind named = ViewKindNamedIn(set);
#if defined(NATIVEWEBVIEW) && defined(WEBENGINEVIEW)
    if(named == QuickNativeKind && NeedsEngineForVanillaPage(url, set)) return WebEngineKind;
#endif
    if(named != NoView) return named;
#if defined(EDGEWEBVIEW)
    if(EdgeWebView::RuntimeAvailable()) return EdgeKind;
#endif
#if defined(WEBENGINEVIEW)
    return WebEngineKind;
#elif defined(NATIVEWEBVIEW)
    return QuickNativeKind;
#else
    return NoView;
#endif
}

TreeBank::ViewKind TreeBank::KindOf(View *view){
    if(!view) return NoView;
#ifdef LOCALVIEW
    if(dynamic_cast<LocalView*>(view)) return LocalKind;
#endif
#ifdef WEBENGINEVIEW
    if(dynamic_cast<QuickWebEngineView*>(view)) return QuickWebEngineKind;
    if(dynamic_cast<WebEngineView*>(view)) return WebEngineKind;
#endif
#ifdef NATIVEWEBVIEW
    if(dynamic_cast<QuickNativeWebView*>(view)) return QuickNativeKind;
#endif
#ifdef EDGEWEBVIEW
    if(dynamic_cast<EdgeWebView*>(view)) return EdgeKind;
#endif
    return NoView;
}

SharedView TreeBank::CreateView(QNetworkRequest req, ViewNode *vn){
    MainWindow *win = Application::GetCurrentWindow();
    TreeBank *tb = win ? win->GetTreeBank() : nullptr;
    QStringList set = GetNodeSettings(vn);
    QString id = GetNetworkSpaceId(vn);

    QUrl url = req.url();

    View *made = nullptr;
    switch(ViewKindFor(url, set)){
#ifdef LOCALVIEW
    case LocalKind:          made = new LocalView(tb, id, set);          break;
#endif
#ifdef WEBENGINEVIEW
    case WebEngineKind:      made = new WebEngineView(tb, id, set);      break;
    case QuickWebEngineKind: made = new QuickWebEngineView(tb, id, set); break;
#endif
#ifdef NATIVEWEBVIEW
    case QuickNativeKind:    made = new QuickNativeWebView(tb, id, set); break;
#endif
#ifdef EDGEWEBVIEW
    case EdgeKind:           made = new EdgeWebView(tb, id, set);        break;
#endif
    default: break;
    }
    SharedView view = SharedView(made, &DeleteView);

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
