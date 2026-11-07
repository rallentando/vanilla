#include "switch.hpp"
#include "const.hpp"

#include "profiles.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QPointer>
#include <QTimer>

#include "application.hpp"
#include "networkcontroller.hpp"
#include "passwordstore.hpp"
#include "saver.hpp"
#include "spacerename.hpp"
#include "treebank.hpp"
#include "lightnode.hpp"
#include "dialog.hpp"
#ifdef EDGEWEBVIEW
#  include "edgewebview.hpp"
#endif

namespace Profiles {

namespace {

    bool s_Removing = false;
    QStringList s_Queue;
    QStringList s_Left;
    int s_Stopped = 0;
    bool s_EdgeRemoved = false;

    QString EdgeFolder(){
#ifdef EDGEWEBVIEW
        return EdgeWebView::ProfilesFolder();
#else
        return QString();
#endif
    }

    QString RootOf(ProfileList::Kind kind){
        switch(kind){
        case ProfileList::Widget:      return NetworkController::WidgetStorageDirectory();
        case ProfileList::WidgetCache: return Application::DataDirectory() + QStringLiteral("webenginecache/");
        case ProfileList::QuickCache:  return Application::DataDirectory() + QStringLiteral("webenginecache/");
        case ProfileList::Quick:       return Application::DataDirectory() + QStringLiteral("webengine/");
        case ProfileList::Edge:        return EdgeFolder();
        }
        return QString();
    }

    QStringList Present(ProfileList::Kind kind){
        const QString root = RootOf(kind);
        if(root.isEmpty()) return QStringList();
        QStringList found;
        for(const QString &name : QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot)){
            const QString storage = kind == ProfileList::Edge
                ? (name.startsWith(QStringLiteral("WV2Profile_")) ? name.mid(11) : QString())
                : name;
            if(ProfileList::IsDigest(storage)) found << storage;
        }
        return found;
    }

    QStringList ExtensionFiles(const QString &name){
        SpaceRename::Places places;
        places.widgetStorage = NetworkController::WidgetStorageDirectory();
        places.data = Application::DataDirectory();
        places.state = Application::StateDirectory();
        places.storageName = [](const QString &id){ return NetworkController::ProfileStorageName(id);};
#ifdef EDGEWEBVIEW
        places.edgeProfiles = EdgeWebView::ProfilesFolder();
        places.edgeJournal = [](const QString &profile){ return EdgeWebView::ExtensionJournal(profile);};
#endif
        QStringList files;
        foreach(const SpaceRename::Move &move, SpaceRename::Candidates(name, name, places))
            if(move.what == SpaceRename::ExtensionState) files << move.from;
        return files;
    }

    QString KindName(ProfileList::Kind kind){
        switch(kind){
        case ProfileList::Widget:      return QCoreApplication::translate("Profiles", "WebEngineView");
        case ProfileList::WidgetCache: return QCoreApplication::translate("Profiles", "WebEngineView cache");
        case ProfileList::Quick:       return QCoreApplication::translate("Profiles", "QuickWebEngineView");
        case ProfileList::QuickCache:  return QCoreApplication::translate("Profiles", "QuickWebEngineView cache");
        case ProfileList::Edge:        return QCoreApplication::translate("Profiles", "EdgeWebView");
        }
        return QString();
    }

    void Finish(const QString &id);
    QString Begin(const QString &id);

    void Report(){
#ifdef EDGEWEBVIEW
        if(s_EdgeRemoved) EdgeWebView::ForgetMissingProfiles();
#endif
        if(AutoSaver *saver = Application::GetAutoSaver()) saver->SaveAllAsync();
        const QStringList left = s_Left;
        const int stopped = s_Stopped;
        s_Left.clear();
        s_Stopped = 0;
        s_EdgeRemoved = false;
        QStringList lines;
        if(!left.isEmpty()){
            lines << QCoreApplication::translate("Profiles", "These folders could not be deleted; another program may have "
                                                             "them open. Nothing else of the profile was deleted.")
                     + QStringLiteral("\n\n") + left.join(QStringLiteral("\n"));
        }
        if(stopped){
            lines << QCoreApplication::translate("Profiles", "Some profiles were not deleted: they were still being closed, "
                                                             "or a rename or a download began.");
        }
        if(!lines.isEmpty()){
            ModelessDialog::Information
                (left.isEmpty() ? QCoreApplication::translate("Profiles", "It could not be deleted.")
                                : QCoreApplication::translate("Profiles", "Some of the profile was not deleted."),
                 lines.join(QStringLiteral("\n\n")));
        }
    }

    void Next(){
        while(!s_Queue.isEmpty()){
            const QString refused = Begin(s_Queue.takeFirst());
            if(refused.isEmpty()) return;
            if(refused == QStringLiteral("busy")){
                s_Queue.clear();
                s_Stopped++;
            } else if(refused == QStringLiteral("closing")){
                s_Stopped++;
            }
        }
        Report();
    }

    void Done(){
        s_Removing = false;
        if(s_Queue.isEmpty()) Report();
        else QTimer::singleShot(0, qApp, [](){ Next();});
    }

    void Stop(){
        s_Stopped++;
        Done();
    }
}

QString PathOf(const ProfileList::Folder &folder){
    const QString root = RootOf(folder.kind);
    return root + (folder.kind == ProfileList::Edge ? QStringLiteral("WV2Profile_") : QString()) + folder.storage;
}

QList<ProfileList::Row> Rows(){
    ProfileList::Input input;
    for(ProfileList::Kind kind : {ProfileList::Widget, ProfileList::WidgetCache, ProfileList::Quick,
                                  ProfileList::Edge})
        input.present[kind] = Present(kind);
    input.usedNames = TreeBank::SpaceNamesInUse();
    input.usedNames << QStringLiteral("inspector") << QStringLiteral("quick-inspector");
    input.knownNames = NetworkController::KnownSpaceNames();
#ifdef VANILLA_PASSWORDS
    if(PasswordStore *store = PasswordStore::Instance()){
        foreach(const PasswordStore::Entry &entry, store->Entries() + store->NeverEntries())
            if(!input.knownNames.contains(entry.space)) input.knownNames << entry.space;
    }
#endif
    input.heldStorages = TreeBank::StoragesOfViews();
    const QSet<QObject*> viewed = TreeBank::ProfilesOfViews();
    foreach(const QString &name, input.knownNames){
        foreach(QObject *profile, NetworkController::SpaceProfiles(name)){
            if(viewed.contains(profile)){ input.heldNames << name; break;}
        }
    }
#ifdef EDGEWEBVIEW
    foreach(const QString &name, EdgeWebView::ProfilesWithExtensions()) input.heldStorages << name;
#endif
    input.storageName = [](const QString &id){ return NetworkController::ProfileStorageName(id);};
    return ProfileList::Rows(input);
}

QJsonObject Describe(){
    const auto nameOf = [](ViewNode *root){
        return root ? root->GetTitle().split(QStringLiteral(";")).first() : QString();
    };
    const QString tabs = nameOf(TreeBank::GetViewRoot());
    const QString trash = nameOf(TreeBank::GetTrashRoot());
    QJsonArray rows;
    foreach(const ProfileList::Row &row, Rows()){
        qint64 total = 0;
        QDateTime modified;
        QJsonArray folders;
        foreach(const ProfileList::Folder &folder, row.folders){
            qint64 bytes = 0;
            const QString path = PathOf(folder);
            QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
            while(it.hasNext()){
                it.next();
                bytes += it.fileInfo().size();
            }
            const QDateTime when = QFileInfo(path).lastModified();
            if(!modified.isValid() || when > modified) modified = when;
            total += bytes;
            QJsonObject f;
            f[QStringLiteral("kind")] = KindName(folder.kind);
            f[QStringLiteral("bytes")] = double(bytes);
            folders.append(f);
        }
        QJsonObject r;
        r[QStringLiteral("id")] = row.id;
        r[QStringLiteral("name")] = row.name;
        r[QStringLiteral("used")] = row.used;
        r[QStringLiteral("held")] = row.held;
        if(row.name == QStringLiteral("inspector") || row.name == QStringLiteral("quick-inspector"))
            r[QStringLiteral("role")] = QStringLiteral("inspector");
        else if(!row.name.isEmpty() && row.name == tabs)
            r[QStringLiteral("role")] = QStringLiteral("tabs");
        else if(!row.name.isEmpty() && row.name == trash)
            r[QStringLiteral("role")] = QStringLiteral("trash");
        r[QStringLiteral("bytes")] = double(total);
        r[QStringLiteral("modified")] = modified.toString(Qt::ISODate);
        r[QStringLiteral("folders")] = folders;
        rows.append(r);
    }
    QJsonObject strings;
    const auto tr = [](const char *text){ return QCoreApplication::translate("Profiles", text);};
    strings[QStringLiteral("none")]      = tr("No profile is on disk.");
    strings[QStringLiteral("used")]      = tr("In use by the tree");
    strings[QStringLiteral("held")]      = tr("Open in a tab");
    strings[QStringLiteral("unused")]    = tr("Not used by the tree");
    strings[QStringLiteral("tabs")]      = tr("In use by the tab list");
    strings[QStringLiteral("trash")]     = tr("In use by the trash");
    strings[QStringLiteral("inspector")] = tr("For the inspector");
    strings[QStringLiteral("noName")]    = tr("No name: this may belong to another installation or build of vanilla.");
    strings[QStringLiteral("remove")]    = tr("Delete");
    strings[QStringLiteral("removing")]  = tr("Deleting...");
    strings[QStringLiteral("removeFailed")] = tr("It could not be deleted.");
    strings[QStringLiteral("removeAll")] = tr("Delete all");
    strings[QStringLiteral("allHint")]   = tr("Every profile neither used by the tree nor open in a tab: %1.");
    strings[QStringLiteral("confirmAll")] =
        tr("Delete all %1 profiles neither used by the tree nor open in a tab? Those with no name may belong to "
           "another installation or build of vanilla. Their site data, cookies and saved passwords are deleted "
           "and cannot be restored.");
    strings[QStringLiteral("confirm")]   = tr("Delete the profile \"%1\"? Its site data, cookies and saved passwords are deleted and cannot be restored.");
    strings[QStringLiteral("confirmNoName")] =
        tr("Delete this profile with no name? It may belong to another installation or build of vanilla, "
           "whose site data is then lost. This cannot be undone.");
    QJsonObject reply;
    reply[QStringLiteral("rows")] = rows;
    reply[QStringLiteral("removing")] = Removing();
    reply[QStringLiteral("strings")] = strings;
    return reply;
}

bool Removing(){ return s_Removing || !s_Queue.isEmpty();}

QString Remove(const QString &id){
    if(!s_Queue.isEmpty()) return QStringLiteral("busy");
    return Begin(id);
}

QString RemoveAll(){
    if(Removing() || TreeBank::IsMovingSpace() || DownloadItem::Running()) return QStringLiteral("busy");
    foreach(const ProfileList::Row &row, Rows())
        if(!row.used && !row.held) s_Queue << row.id;
    if(s_Queue.isEmpty()) return QStringLiteral("no such profile");
    Next();
    return QString();
}

namespace {

QString Begin(const QString &id){
    if(s_Removing || TreeBank::IsMovingSpace() || DownloadItem::Running()) return QStringLiteral("busy");
    const QList<ProfileList::Row> rows = Rows();
    const ProfileList::Row *row = ProfileList::Find(rows, id);
    if(!row) return QStringLiteral("no such profile");
    if(row->used || row->held) return QStringLiteral("in use");
    if(!row->name.isEmpty() && NetworkController::StorageInUse(row->name)) return QStringLiteral("closing");
    s_Removing = true;
    const QString name = row->name;
    QList<QPointer<QObject>> going;
    if(!name.isEmpty())
        foreach(QObject *profile, NetworkController::SpaceProfiles(name)) going << QPointer<QObject>(profile);
    if(going.isEmpty()){
        Finish(id);
        return QString();
    }
    NetworkController::ReleaseSpace(name);
    SpaceRename::WhenGone(going, 10000, [id](bool gone){
        if(gone) Finish(id);
        else Stop();
    });
    return QString();
}

void Finish(const QString &id){
    const QList<ProfileList::Row> rows = Rows();
    const ProfileList::Row *row = ProfileList::Find(rows, id);
    if(!row || row->used || row->held || TreeBank::IsMovingSpace() || DownloadItem::Running() ||
       (!row->name.isEmpty() && NetworkController::StorageInUse(row->name))){
        Stop();
        return;
    }
    QStringList left;
    bool edge = false;
    foreach(const ProfileList::Folder &folder, row->folders){
        const QString path = PathOf(folder);
        QDir(path).removeRecursively();
        if(QFileInfo::exists(path)) left << QDir::toNativeSeparators(path);
        else if(folder.kind == ProfileList::Edge) edge = true;
    }
    const QString name = row->name;
    if(left.isEmpty() && !name.isEmpty()){
        NetworkController::DropHeldCookies(name);
        foreach(const QString &file, ExtensionFiles(name)) QFile::remove(file);
#ifdef VANILLA_PASSWORDS
        if(PasswordStore *store = PasswordStore::Instance()) store->RemoveSpace(name, nullptr);
#endif
    }
    if(edge) s_EdgeRemoved = true;
    s_Left << left;
    Done();
}

}

}
