#ifndef DNRRULES_HPP
#define DNRRULES_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

namespace Dnr {

enum ResourceType {
    MainFrame, SubFrame, Stylesheet, Script, Image, Font, Object,
    XmlHttpRequest, Ping, CspReport, Media, WebSocket, WebTransport,
    WebBundle, Other,
    ResourceTypeCount
};

struct Request {
    QUrl url;
    QUrl initiator;
    ResourceType type = Other;
    QByteArray method;
    QHash<QString, quint64> frameAllows;
};

struct Decision {
    enum Kind { None, Block, Redirect };
    Kind kind = None;
    QUrl redirect;
    QHash<QString, quint64> frameAllows;
};

struct Skipped {
    int modifyHeaders = 0;
    int computedRedirect = 0;
    int tabBound = 0;
    int badRegex = 0;
    int unreadCondition = 0;
    int malformed = 0;
    int Total() const {
        return modifyHeaders + computedRedirect + tabBound + badRegex
             + unreadCondition + malformed;
    }
};

QString RegistrableDomain(const QString &host);

class UrlFilter {
public:
    UrlFilter() = default;
    UrlFilter(const QString &filter, bool caseSensitive);

    bool Matches(const QString &url, const QString &lower,
                 int hostStart, int hostEnd) const;

    static bool IsRefused(const QString &filter){
        return filter.startsWith(QStringLiteral("||*"));
    }

    QString IndexToken() const { return m_IndexToken; }

private:
    enum Anchor { Anywhere, AtStart, AtLabel };
    Anchor m_Left = Anywhere;
    bool m_Right = false;
    bool m_CaseSensitive = false;
    QStringList m_Pieces;
    QString m_IndexToken;
};

class Rules {
public:
    void AddRuleset(const QString &extensionId, const QByteArray &json,
                    Skipped *skipped = nullptr);

    bool IsEmpty() const { return m_Rules.isEmpty(); }
    int Count() const { return m_Rules.size(); }

    Decision Evaluate(const Request &request) const;

private:
    enum Action { Allow, AllowAllRequests, Block, UpgradeScheme, RedirectTo };
    enum DomainType { AnyParty, FirstParty, ThirdParty };

    struct Rule {
        int extension = 0;
        quint64 rank = 0;
        Action action = Block;
        QUrl redirect;
        quint32 types = 0;
        QList<QByteArray> methods;
        QList<QByteArray> excludedMethods;
        bool hasFilter = false;
        UrlFilter filter;
        bool hasRegex = false;
        QRegularExpression regex;
        QSet<QString> requestDomains;
        QSet<QString> excludedRequestDomains;
        QSet<QString> initiatorDomains;
        QSet<QString> excludedInitiatorDomains;
        DomainType domainType = AnyParty;
    };

    struct Asked;
    bool RuleMatches(const Rule &rule, const Asked &asked) const;

    QVector<Rule> m_Rules;
    QHash<QString, QVector<int>> m_ByToken;
    QVector<int> m_Unindexed;
    QStringList m_Extensions;
};

class Framed {
public:
    explicit Framed(int kept = 256) : m_Kept(kept) {}
    static QString KeyOf(const QUrl &url);
    bool Find(quint64 generation, const QString &key, QHash<QString, quint64> *allows);
    void Put(quint64 generation, const QString &key, const QHash<QString, quint64> &allows);
    int Count() const { return m_Held.size(); }

private:
    int m_Kept;
    quint64 m_Generation = 0;
    QHash<QString, QHash<QString, quint64>> m_Held;
    QList<QString> m_Order;
};

class Held {
public:
    enum Kind { Dynamic, Session };

    static const int DYNAMIC_LIMIT = 30000;
    static const int SESSION_LIMIT = 5000;
    static const int REGEX_LIMIT = 1000;
    static const int ENABLED_LIMIT = 50;
    static const int BYTES_LIMIT = 8 * 1024 * 1024;

    struct Ruleset {
        QString id;
        bool enabled = true;
    };

    QString Update(const QString &extension, Kind kind, const QJsonValue &options,
                   const QString &version);
    QJsonArray Get(const QString &extension, Kind kind, const QJsonValue &filter,
                   const QString &version) const;

    QStringList Enabled(const QString &extension, const QList<Ruleset> &manifest,
                        const QString &version) const;
    QString UpdateEnabled(const QString &extension, const QJsonValue &options,
                          const QList<Ruleset> &manifest, const QString &version);
    bool Chose(const QString &extension, const QString &version) const {
        const auto i = m_Entries.constFind(extension);
        return i != m_Entries.constEnd() && i.value().chose && i.value().enabledVersion == version;
    }

    bool Settle(const QString &extension, const QString &version);
    void Forget(const QString &extension);
    QStringList Extensions() const { return m_Entries.keys(); }
    void KeepSessionsOf(const QSet<QString> &extensions);
    bool Holds(const QString &extension) const { return m_Entries.contains(extension); }

    QJsonObject ToJson() const;
    static Held FromJson(const QJsonObject &object, bool *damaged = nullptr);

private:
    struct Entry {
        QJsonArray dynamic;
        QJsonArray session;
        QString sessionVersion;
        bool chose = false;
        QStringList enabled;
        QString enabledVersion;
        bool IsEmpty() const { return dynamic.isEmpty() && session.isEmpty() && !chose; }
    };
    QJsonArray SessionOf(const Entry &entry, const QString &version) const {
        return entry.sessionVersion == version ? entry.session : QJsonArray();
    }
    QHash<QString, Entry> m_Entries;
};

}

#endif
