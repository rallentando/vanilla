#ifndef EXTENSIONUSERSCRIPTS_HPP
#define EXTENSIONUSERSCRIPTS_HPP

#include "switch.hpp"

#include <functional>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>

#include "extensionmainscripts.hpp"

namespace ExtensionUserScripts {

const int SCRIPTS_LIMIT = 256;
const int SOURCES_LIMIT = 64;
const qint64 TOTAL_LIMIT = 32 * 1024 * 1024;
const quint32 FIRST_WORLD = 128;
const quint32 LAST_WORLD = 255;
const int EXTENSION_WORLDS = 16;

struct Source {
    QString code;
    QString file;
    bool isFile = false;
};

struct Registration {
    QString id;
    QList<Source> js;
    QStringList matches;
    QStringList excludeMatches;
    QStringList includeGlobs;
    QStringList excludeGlobs;
    bool allFrames = false;
    ExtensionMainScripts::RunAt runAt = ExtensionMainScripts::DocumentIdle;
    bool main = false;
    QString worldId;
};

bool Read(const QJsonValue &value, Registration *out, QString *error,
          const QString &method = QStringLiteral("register"));
QJsonObject Written(const Registration &registration);

typedef ExtensionMainScripts::Reader Reader;

struct WorldConfig {
    QString worldId;
    bool hasCsp = false;
    QString csp;
    bool hasMessaging = false;
    bool messaging = false;
};

class Book {
public:
    static Book Of(const QJsonArray &kept);
    QJsonArray Kept() const;
    bool Register(const QJsonValue &scripts, const Reader &read, QString *error);
    bool Update(const QJsonValue &scripts, const Reader &read, QString *error);
    bool Unregister(const QJsonValue &filter, QString *error);
    QJsonArray Get(const QJsonValue &filter, QString *error) const;
    bool Configure(const QJsonValue &properties, QString *error);
    bool Reset(const QJsonValue &worldId, QString *error);
    QJsonArray Worlds() const;
    bool Messaging(const QString &worldId) const;
    const QList<Registration> &List() const { return m_List; }
    bool IsEmpty() const { return m_List.isEmpty() && m_Worlds.isEmpty(); }
    QStringList WorldIds() const;
private:
    QList<Registration> m_List;
    QList<WorldConfig> m_Worlds;
};

class Worlds {
public:
    bool Take(const QString &extensionId, const QStringList &worldIds);
    quint32 Of(const QString &extensionId, const QString &worldId) const;
private:
    QHash<QString, quint32> m_Of;
    QHash<QString, int> m_Given;
    quint32 m_Next = FIRST_WORLD;
};

typedef std::function<QString(const QString &worldId)> Prelude;
QList<ExtensionMainScripts::Script> ScriptsOf(const QString &extensionId, const Book &book,
                                             const QStringList &hostPermissions, const Worlds &worlds,
                                             const Reader &read, QStringList *skipped,
                                             const Prelude &prelude = Prelude());

bool ThrowKept(QHash<QString, QString> *checked, const QString &id, const QString &version,
               bool kept, const QString &keptVersion);

QString StoreFileName(const QString &profileKey);

}

#endif
