#ifndef EXTENSIONMAINSCRIPTS_HPP
#define EXTENSIONMAINSCRIPTS_HPP

#include "switch.hpp"

#include <functional>
#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace ExtensionMainScripts {

const int SCRIPTS_LIMIT = 256;
const int FILES_LIMIT = 16;
const qint64 FILE_LIMIT = 8 * 1024 * 1024;
const qint64 TOTAL_LIMIT = 32 * 1024 * 1024;

enum RunAt { DocumentStart, DocumentEnd, DocumentIdle };

RunAt RunAtOf(const QString &text, bool *ok);
bool Strings(const QJsonValue &value, QStringList *out);

struct Registration {
    QString id;
    QStringList js;
    QStringList matches;
    QStringList excludeMatches;
    bool allFrames = false;
    RunAt runAt = DocumentIdle;
    bool persist = true;
    QStringList includeGlobs;
    QStringList excludeGlobs;
};

QList<Registration> Parse(const QJsonArray &args, QString *error);

bool FileNameOk(const QString &name);

QString PatternRegex(const QString &pattern, bool hostPermission = false);

const int GLOB_LIMIT = 1024;
QString GlobRegex(const QString &glob);

bool ReadInside(const QString &folder, const QString &name, QByteArray *bytes);

QString Header(const Registration &registration, const QStringList &hostPermissions);

struct Script {
    QString name;
    QString source;
    RunAt runAt = DocumentIdle;
    bool subFrames = false;
    quint32 world = 0;
    bool operator==(const Script &other) const {
        return name == other.name && runAt == other.runAt && subFrames == other.subFrames && world == other.world
            && source == other.source;
    }
    bool operator!=(const Script &other) const { return !(*this == other); }
};

typedef std::function<bool(const QString &name, QByteArray *bytes)> Reader;
QList<Script> ScriptsOf(const QString &extensionId, const QList<Registration> &list,
                        const QStringList &hostPermissions, const Reader &read, QString *error);

struct Store {
    struct Entry {
        QString version;
        QJsonArray list;
        bool operator==(const Entry &other) const { return version == other.version && list == other.list; }
    };
    QHash<QString, Entry> entries;
    static Store FromJson(const QByteArray &bytes);
    QByteArray ToJson() const;
    void Put(const QString &id, const QString &version, const QJsonArray &sent);
    void Drop(const QString &id) { entries.remove(id); }
};
QString ProfileFileName(const QString &prefix, const QString &profileKey);
QString StoreFileName(const QString &profileKey);

enum Restore { LeaveIt, PutIn, ThrowAway };
Restore RestoreOf(bool wanted, const QString &keptVersion, const QString &version);

class Table;
struct Now {
    std::function<QString(const QString &id)> folder;
    std::function<bool(const QString &id, const QString &folder)> wanted;
    std::function<QString(const QString &id)> version;
    std::function<QStringList(const QString &id)> hostPermissions;
};
struct Restored;
Restored RestoreInto(Table &table, Store &store, const Now &now);

class Table {
public:
    struct Change {
        QList<Script> removed;
        QList<Script> inserted;
        bool isEmpty() const { return removed.isEmpty() && inserted.isEmpty(); }
    };
    Change Put(const QString &id, const QString &folder, const QList<Script> &scripts);
    Change Keep(const std::function<bool(const QString &id, const QString &folder)> &wanted);
    QList<Script> Of(const QString &id) const { return m_Held.value(id).scripts; }
    bool Has(const QString &id) const { return m_Held.contains(id); }
    QString FolderOf(const QString &id) const { return m_Held.value(id).folder; }
private:
    struct Held {
        QString folder;
        QList<Script> scripts;
    };
    QHash<QString, Held> m_Held;
};

}

namespace ExtensionMainScripts {
struct Restored {
    Table::Change change;
    bool thrown = false;
};
}

#endif
