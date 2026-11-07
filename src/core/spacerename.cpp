#include "switch.hpp"

#include "spacerename.hpp"

#include "extensionui.hpp"
#include "extensionmainscripts.hpp"
#include "extensionuserscripts.hpp"

#include <QCoreApplication>
#include <QJsonArray>
#include <QDir>
#include <QSet>
#include <QFile>
#include <QFileInfo>

#if defined(Q_OS_WIN)
#  include <windows.h>
#else
#  include <cstdio>
#endif

namespace SpaceRename {

QList<Move> Candidates(const QString &from, const QString &to, const Places &places){
    QList<Move> moves;
    const QString quickFrom = QStringLiteral("quick:") + from;
    const QString quickTo   = QStringLiteral("quick:") + to;

    moves << Move{WidgetStorage, places.widgetStorage + places.storageName(from),
                                 places.widgetStorage + places.storageName(to)};
    moves << Move{WidgetCache, places.data + QStringLiteral("webenginecache/") + places.storageName(from),
                               places.data + QStringLiteral("webenginecache/") + places.storageName(to)};
    moves << Move{QuickStorage, places.data + QStringLiteral("webengine/") + places.storageName(quickFrom),
                                places.data + QStringLiteral("webengine/") + places.storageName(quickTo)};
    moves << Move{QuickCache, places.data + QStringLiteral("webenginecache/") + places.storageName(quickFrom),
                              places.data + QStringLiteral("webenginecache/") + places.storageName(quickTo)};
    if(!places.edgeProfiles.isEmpty())
        moves << Move{EdgeProfile, places.edgeProfiles + QStringLiteral("WV2Profile_") + places.storageName(from),
                                   places.edgeProfiles + QStringLiteral("WV2Profile_") + places.storageName(to)};

    const QList<QPair<QString, QString>> keys =
        { qMakePair(from, to), qMakePair(quickFrom, quickTo) };
    typedef QString (*FileName)(const QString&);
    const QList<FileName> names = {
        &ExtensionUi::MenusFileName, &ExtensionUi::RulesFileName,
        &ExtensionUi::InstalledFileName, &ExtensionUi::RegisteredFileName,
        &ExtensionMainScripts::StoreFileName, &ExtensionUserScripts::StoreFileName,
    };
    for(const auto &key : keys)
        for(FileName name : names)
            moves << Move{ExtensionState, places.state + name(key.first), places.state + name(key.second)};
    if(places.edgeJournal){
        const QString edgeFrom = places.storageName(from), edgeTo = places.storageName(to);
        const QString journalFrom = places.edgeJournal(edgeFrom), journalTo = places.edgeJournal(edgeTo);
        const QString copies = QStringLiteral("-copies.json");
        moves << Move{ExtensionState, journalFrom, journalTo};
        moves << Move{ExtensionState, QString(journalFrom).replace(QStringLiteral(".json"), copies),
                                      QString(journalTo).replace(QStringLiteral(".json"), copies)};
        moves << Move{ExtensionState,
                      places.state + ExtensionUi::MenusFileName(QStringLiteral("edge:") + edgeFrom),
                      places.state + ExtensionUi::MenusFileName(QStringLiteral("edge:") + edgeTo)};
    }
    return moves;
}

QList<Move> Occupied(const QList<Move> &moves){
    QList<Move> occupied;
    foreach(const Move &move, moves)
        if(QFileInfo::exists(move.to)) occupied << move;
    return occupied;
}

QList<Cookie> CookiesToAdd(const QList<Cookie> &from, const QList<Cookie> &to){
    const auto key = [](const Cookie &c){
        return c.name + QLatin1Char('\n') + c.domain.toLower() + QLatin1Char('\n') + c.path;
    };
    QSet<QString> there;
    foreach(const Cookie &cookie, to) there.insert(key(cookie));
    QList<Cookie> add;
    foreach(const Cookie &cookie, from){
        if(cookie.session || there.contains(key(cookie))) continue;
        there.insert(key(cookie));
        add << cookie;
    }
    return add;
}

namespace {
    void PollGone(QList<QPointer<QObject>> going, std::shared_ptr<QElapsedTimer> waited,
                  int limitMs, std::function<void(bool)> done){
        bool gone = true;
        foreach(const QPointer<QObject> &object, going) if(object){ gone = false; break;}
        if(gone){ done(true); return;}
        if(waited->elapsed() >= limitMs){ done(false); return;}
        QTimer::singleShot(20, QCoreApplication::instance(),
                           [going, waited, limitMs, done](){ PollGone(going, waited, limitMs, done);});
    }
}

bool ForgetMissingProfiles(QJsonObject &localState, std::function<bool(const QString&)> exists){
    QJsonObject profile = localState.value(QStringLiteral("profile")).toObject();
    bool changed = false;
    const QString last = profile.value(QStringLiteral("last_used")).toString();
    if(!last.isEmpty() && !exists(last)){
        profile.remove(QStringLiteral("last_used"));
        changed = true;
    }
    const QJsonArray active = profile.value(QStringLiteral("last_active_profiles")).toArray();
    QJsonArray kept;
    for(const QJsonValue &name : active) if(exists(name.toString())) kept << name;
    if(kept.size() != active.size()){
        profile.insert(QStringLiteral("last_active_profiles"), kept);
        changed = true;
    }
    if(changed) localState.insert(QStringLiteral("profile"), profile);
    return changed;
}

bool BrowserHoldsFolder(const QString &folder){
    const QString lock = QDir(folder).filePath(QStringLiteral("lockfile"));
    if(!QFileInfo::exists(lock)) return false;
#if defined(Q_OS_WIN)
    const QString path = QDir::toNativeSeparators(lock);
    const HANDLE file = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_READ, 0, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(file == INVALID_HANDLE_VALUE) return true;
    CloseHandle(file);
    return false;
#else
    return true;
#endif
}

void WhenGone(const QList<QPointer<QObject>> &going, int limitMs, std::function<void(bool)> done){
    std::shared_ptr<QElapsedTimer> waited(new QElapsedTimer);
    waited->start();
    PollGone(going, waited, limitMs, done);
}

Mover::Mover(const QList<Move> &moves, MoveFunction move,
             int retryMs, int limitMs, QObject *parent)
    : QObject(parent)
    , m_Rest(moves)
    , m_Move(move)
    , m_RetryMs(retryMs)
    , m_LimitMs(limitMs)
    , m_Started(false)
{
}

void Mover::Start(std::function<void(const QList<Move>&)> done){
    m_Done = done;
    m_Started = false;
    Step();
}

void Mover::Step(){
    while(!m_Rest.isEmpty()){
        const Move &move = m_Rest.first();
        if(!QFileInfo::exists(move.from)){
            m_Rest.removeFirst();
            m_Started = false;
            continue;
        }
        if(QFileInfo::exists(move.to)){
            m_Failed << m_Rest.takeFirst();
            m_Started = false;
            continue;
        }
        if(!m_Started){ m_Trying.start(); m_Started = true;}
        if(m_Move(move.from, move.to)){
            m_Rest.removeFirst();
            m_Started = false;
            continue;
        }
        if(m_Trying.elapsed() >= m_LimitMs){
            m_Failed << m_Rest.takeFirst();
            m_Started = false;
            continue;
        }
        QTimer::singleShot(m_RetryMs, this, [this](){ Step();});
        return;
    }
    std::function<void(const QList<Move>&)> done = m_Done;
    const QList<Move> failed = m_Failed;
    if(done) done(failed);
}

bool Mover::DefaultMove(const QString &from, const QString &to){
#if defined(Q_OS_WIN)
    const QString a = QDir::toNativeSeparators(from), b = QDir::toNativeSeparators(to);
    return MoveFileExW(reinterpret_cast<LPCWSTR>(a.utf16()),
                       reinterpret_cast<LPCWSTR>(b.utf16()), 0);
#else
    return std::rename(QFile::encodeName(from).constData(),
                       QFile::encodeName(to).constData()) == 0;
#endif
}

}
