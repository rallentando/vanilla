#include "switch.hpp"

#include "downloadhistory.hpp"

#include "application.hpp"
#include "networkcontroller.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QProcess>
#include <QSaveFile>
#include <QSet>

#include <iterator>

namespace {

    const char *const StateWords[] = { "running", "completed", "cancelled", "failed", "interrupted" };

    DownloadHistory::State StateOf(const QString &word){
        for(int i = 0; i < int(std::size(StateWords)); i++)
            if(word == QLatin1String(StateWords[i])) return static_cast<DownloadHistory::State>(i);
        return DownloadHistory::Interrupted;
    }
}

DownloadHistory *DownloadHistory::Instance(){
    static DownloadHistory *history = nullptr;
    if(!history){
        QCoreApplication *application = QCoreApplication::instance();
        history = new DownloadHistory
            (Application::StateDirectory() + QStringLiteral("downloads.json"), application);
        if(application)
            connect(application, &QCoreApplication::aboutToQuit,
                    history, &DownloadHistory::SetClosing);
    }
    return history;
}

DownloadHistory::DownloadHistory(const QString &path, QObject *parent)
    : QObject(parent)
    , m_Path(path)
    , m_NextId(1)
    , m_Closing(false)
{
    Load();
}

DownloadHistory::~DownloadHistory(){}

QString DownloadHistory::StateName(State state){
    return QLatin1String(StateWords[state]);
}

bool DownloadHistory::IsRunnable(const QString &path, bool byContent){
    static const QSet<QString> extensions = QSet<QString>()
        << QStringLiteral("exe") << QStringLiteral("bat") << QStringLiteral("cmd")
        << QStringLiteral("com") << QStringLiteral("msi") << QStringLiteral("msp")
        << QStringLiteral("ps1") << QStringLiteral("psm1") << QStringLiteral("js")
        << QStringLiteral("jse") << QStringLiteral("vbs") << QStringLiteral("vbe")
        << QStringLiteral("wsf") << QStringLiteral("wsh") << QStringLiteral("scr")
        << QStringLiteral("lnk") << QStringLiteral("pif") << QStringLiteral("reg")
        << QStringLiteral("hta") << QStringLiteral("cpl") << QStringLiteral("jar")
        << QStringLiteral("sh") << QStringLiteral("command") << QStringLiteral("app")
        << QStringLiteral("appimage") << QStringLiteral("run") << QStringLiteral("desktop")
        << QStringLiteral("py") << QStringLiteral("pyw") << QStringLiteral("rb")
        << QStringLiteral("pl") << QStringLiteral("php") << QStringLiteral("ahk")
        << QStringLiteral("vb") << QStringLiteral("wsc") << QStringLiteral("sct")
        << QStringLiteral("msc") << QStringLiteral("inf") << QStringLiteral("url")
        << QStringLiteral("website") << QStringLiteral("appx") << QStringLiteral("msix")
        << QStringLiteral("appxbundle") << QStringLiteral("msixbundle")
        << QStringLiteral("application") << QStringLiteral("gadget") << QStringLiteral("scf")
        << QStringLiteral("xll") << QStringLiteral("library-ms") << QStringLiteral("settingcontent-ms")
        << QStringLiteral("appref-ms") << QStringLiteral("chm") << QStringLiteral("hlp")
        << QStringLiteral("diagcab") << QStringLiteral("search-ms") << QStringLiteral("searchconnector-ms")
        << QStringLiteral("jnlp")
        << QStringLiteral("diagpkg") << QStringLiteral("diagcfg") << QStringLiteral("xbap")
        << QStringLiteral("ps1xml") << QStringLiteral("ps2") << QStringLiteral("ps2xml")
        << QStringLiteral("psc1") << QStringLiteral("psc2") << QStringLiteral("psd1")
        << QStringLiteral("msh") << QStringLiteral("msh1") << QStringLiteral("msh2")
        << QStringLiteral("mshxml") << QStringLiteral("msh1xml") << QStringLiteral("msh2xml")
        << QStringLiteral("shb") << QStringLiteral("shs") << QStringLiteral("ins") << QStringLiteral("isp")
        << QStringLiteral("mde") << QStringLiteral("ade") << QStringLiteral("adp") << QStringLiteral("mst")
        << QStringLiteral("msu");
    if(extensions.contains(QFileInfo(path).suffix().toLower())) return true;

    static const QStringList programs = QStringList()
        << QStringLiteral("application/x-executable")
        << QStringLiteral("application/x-msdownload")
        << QStringLiteral("application/x-ms-dos-executable")
        << QStringLiteral("application/x-msi")
        << QStringLiteral("application/x-shellscript")
        << QStringLiteral("application/x-sharedlib")
        << QStringLiteral("text/x-python")
        << QStringLiteral("text/x-perl")
        << QStringLiteral("application/x-ruby")
        << QStringLiteral("application/x-php")
        << QStringLiteral("application/javascript");
    const QMimeType type = QMimeDatabase().mimeTypeForFile
        (path, byContent ? QMimeDatabase::MatchDefault : QMimeDatabase::MatchExtension);
    foreach(const QString &program, programs)
        if(type.inherits(program)) return true;
    return false;
}

DownloadHistory::Entry *DownloadHistory::FindMutable(int id){
    return const_cast<Entry*>(Find(id));
}

const DownloadHistory::Entry *DownloadHistory::Find(int id) const {
    foreach(const Entry &entry, m_Entries)
        if(entry.id == id) return &entry;
    return nullptr;
}

QList<DownloadHistory::Entry> DownloadHistory::Entries() const {
    return QList<Entry>(m_Entries.crbegin(), m_Entries.crend());
}

QUrl DownloadHistory::SourcePageWorthKeeping(const QUrl &page, const QUrl &url){
    if(!page.isValid() || page.isEmpty()) return QUrl();
    const QString scheme = page.scheme().toLower();
    if(scheme != QStringLiteral("http") && scheme != QStringLiteral("https") &&
       scheme != QStringLiteral("ftp") && scheme != QStringLiteral("file"))
        return QUrl();
    if(page.adjusted(QUrl::NormalizePathSegments) == url.adjusted(QUrl::NormalizePathSegments))
        return QUrl();
    return page;
}

int DownloadHistory::Started(const QUrl &url, const QString &path, DownloadItem *item,
                             const QUrl &page){
    if(m_Closing || path.isEmpty()) return 0;
    Entry entry;
    entry.id = m_NextId++;
    entry.url = url;
    entry.page = SourcePageWorthKeeping(page, url);
    entry.path = path;
    entry.started = QDateTime::currentDateTime();
    entry.state = Running;
    m_Entries << entry;
    if(item) m_Live.insert(entry.id, item);
    Trim();
    Commit();
    emit EntryStarted(entry.id);
    emit ListChanged();
    emit Changed();
    return entry.id;
}

void DownloadHistory::Progress(int id, qint64 received, qint64 total){
    if(m_Closing) return;
    Entry *entry = FindMutable(id);
    if(!entry || entry->state != Running) return;
    entry->received = received;
    entry->total = total;
    emit Changed();
}

void DownloadHistory::Ended(int id, State state){
    if(m_Closing || state == Running) return;
    Entry *entry = FindMutable(id);
    if(!entry || entry->state != Running) return;
    entry->state = state;
    entry->ended = QDateTime::currentDateTime();
    if(state == Completed && entry->total > 0) entry->received = entry->total;
    m_Live.remove(id);
    CommitListChange();
}

bool DownloadHistory::Remove(int id){
    if(m_Closing) return false;
    for(int i = 0; i < m_Entries.size(); i++){
        if(m_Entries[i].id != id) continue;
        if(m_Entries[i].state == Running) return false;
        m_Entries.removeAt(i);
        CommitListChange();
        return true;
    }
    return false;
}

void DownloadHistory::Clear(){
    if(m_Closing) return;
    if(!m_Entries.removeIf([](const Entry &entry){ return entry.state != Running;})) return;
    CommitListChange();
}

bool DownloadHistory::Cancel(int id){
    const Entry *entry = Find(id);
    if(!entry || entry->state != Running) return false;
    QPointer<DownloadItem> item = m_Live.value(id);
    if(!item) return false;
    item->Stop();
    return true;
}

bool DownloadHistory::CanOpen(const Entry &entry){
    return entry.state == Completed && QFileInfo(entry.path).isFile() &&
        !IsRunnable(entry.path, false);
}

bool DownloadHistory::Open(int id){
    const Entry *entry = Find(id);
    if(!entry || !CanOpen(*entry) || IsRunnable(entry->path)) return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(entry->path).absoluteFilePath()));
}

bool DownloadHistory::ShowInFolder(int id){
    const Entry *entry = Find(id);
    if(!entry) return false;
    const QFileInfo info(entry->path);
#ifdef Q_OS_WIN
    if(info.isFile()){
        QProcess process;
        process.setProgram(QStringLiteral("explorer.exe"));
        process.setNativeArguments(QStringLiteral("/select,\"") +
                                   QDir::toNativeSeparators(info.absoluteFilePath()) +
                                   QStringLiteral("\""));
        return process.startDetached();
    }
#endif
    const QString folder = info.absolutePath();
    if(!QFileInfo(folder).isDir()) return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void DownloadHistory::SetClosing(){
    m_Closing = true;
}

void DownloadHistory::Trim(){
    int excess = m_Entries.size() - Limit;
    for(int i = 0; excess > 0 && i < m_Entries.size(); ){
        if(m_Entries[i].state == Running){ i++; continue;}
        m_Live.remove(m_Entries[i].id);
        m_Entries.removeAt(i);
        excess--;
    }
}

void DownloadHistory::CommitListChange(){
    Commit();
    emit ListChanged();
    emit Changed();
}

void DownloadHistory::Load(){
    m_Entries.clear();
    QFile file(m_Path);
    if(!file.open(QIODevice::ReadOnly)) return;
    const QJsonArray array =
        QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("entries")).toArray();
    foreach(const QJsonValue &value, array){
        const QJsonObject obj = value.toObject();
        const auto text = [&obj](const char *key){ return obj.value(QLatin1String(key)).toString();};
        Entry entry;
        entry.id = obj.value(QStringLiteral("id")).toInt();
        entry.path = text("path");
        if(entry.id <= 0 || entry.path.isEmpty()) continue;
        entry.url = QUrl(text("url"));
        entry.page = SourcePageWorthKeeping(QUrl(text("page")), entry.url);
        entry.started = QDateTime::fromString(text("started"), Qt::ISODate);
        entry.ended = QDateTime::fromString(text("ended"), Qt::ISODate);
        entry.state = StateOf(text("state"));
        if(entry.state == Running) entry.state = Interrupted;
        entry.received = obj.value(QStringLiteral("received")).toVariant().toLongLong();
        entry.total = obj.value(QStringLiteral("total")).toVariant().toLongLong();
        m_Entries << entry;
        if(entry.id >= m_NextId) m_NextId = entry.id + 1;
    }
    Trim();
}

void DownloadHistory::Commit(){
    QJsonArray array;
    foreach(const Entry &entry, m_Entries){
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), entry.id);
        obj.insert(QStringLiteral("url"), entry.url.toString());
        if(!entry.page.isEmpty())
            obj.insert(QStringLiteral("page"), entry.page.toString());
        obj.insert(QStringLiteral("path"), entry.path);
        obj.insert(QStringLiteral("started"), entry.started.toString(Qt::ISODate));
        if(entry.ended.isValid())
            obj.insert(QStringLiteral("ended"), entry.ended.toString(Qt::ISODate));
        obj.insert(QStringLiteral("state"), StateName(entry.state));
        obj.insert(QStringLiteral("received"), QString::number(entry.received));
        obj.insert(QStringLiteral("total"), QString::number(entry.total));
        array.append(obj);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("entries"), array);
    QSaveFile file(m_Path);
    if(!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(root).toJson());
    file.commit();
}
