#include "switch.hpp"

#include "permissiontable.hpp"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace {

    int DefaultPort(const QString &scheme){
        return scheme == QStringLiteral("https") ? 443 : 80;
    }

    QString OriginHost(const QUrl &url){
        const QString host = url.host();
        if(host.isEmpty()) return QString();

        QHostAddress address;
        if(address.setAddress(host) &&
           address.protocol() == QAbstractSocket::IPv6Protocol)
            return QStringLiteral("[") + address.toString().toLower() + QStringLiteral("]");

        return QString::fromLatin1(QUrl::toAce(host)).toLower();
    }

    int KindOrder(const QString &kind){
        const int index = PermissionTable::Kinds().indexOf(kind);
        return index == -1 ? PermissionTable::Kinds().length() : index;
    }
}

QString PermissionTable::Origin(const QUrl &url){
    if(!url.isValid()) return QString();
    const QString scheme = url.scheme().toLower();
    if(scheme != QStringLiteral("https") && scheme != QStringLiteral("http"))
        return QString();
    const QString host = OriginHost(url);
    if(host.isEmpty()) return QString();
    const int port = url.port(DefaultPort(scheme));
    if(port == DefaultPort(scheme)) return scheme + QStringLiteral("://") + host;
    return scheme + QStringLiteral("://") + host + QStringLiteral(":") + QString::number(port);
}

QStringList PermissionTable::Kinds(){
    static const QStringList kinds = {
        QStringLiteral("notifications"),
        QStringLiteral("geolocation"),
        QStringLiteral("microphone"),
        QStringLiteral("camera"),
        QStringLiteral("local-fonts"),
        QStringLiteral("clipboard"),
        QStringLiteral("mouse-lock"),
        QStringLiteral("screen"),
        QStringLiteral("screen-audio"),
        QStringLiteral("clipboard-read"),
        QStringLiteral("sensors"),
        QStringLiteral("automatic-downloads"),
        QStringLiteral("file-read-write"),
        QStringLiteral("autoplay"),
        QStringLiteral("midi-sysex"),
        QStringLiteral("window-management"),
        QStringLiteral("persistent-storage"),
    };
    return kinds;
}

QString PermissionTable::StateName(State state){
    switch(state){
    case Allow: return QStringLiteral("allow");
    case Block: return QStringLiteral("block");
    case Ask:   break;
    }
    return QStringLiteral("ask");
}

PermissionTable::State PermissionTable::StateFromName(const QString &name){
    if(name == QStringLiteral("allow")) return Allow;
    if(name == QStringLiteral("block")) return Block;
    return Ask;
}

PermissionTable::State PermissionTable::Both(State a, State b){
    if(a == Block || b == Block) return Block;
    if(a == Allow && b == Allow) return Allow;
    return Ask;
}

PermissionTable PermissionTable::FromJson(const QByteArray &json, bool *broken){
    if(broken) *broken = false;
    PermissionTable table;
    if(json.trimmed().isEmpty()) return table;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    const QJsonValue version = document.object().value(QStringLiteral("version"));
    const QJsonValue rows = document.object().value(QStringLiteral("permissions"));
    if(error.error != QJsonParseError::NoError || !document.isObject() ||
       !rows.isArray() || (!version.isUndefined() && version.toInt(-1) != 1)){
        if(broken) *broken = true;
        return table;
    }

    foreach(const QJsonValue &value, rows.toArray()){
        const QJsonObject row = value.toObject();
        const QString spelled = row.value(QStringLiteral("origin")).toString();
        const QString origin = Origin(spelled);
        const QString kind = row.value(QStringLiteral("kind")).toString();
        const State state = StateFromName(row.value(QStringLiteral("state")).toString());
        if(origin.isEmpty() || origin != spelled || kind.isEmpty() || state == Ask){
            if(broken) *broken = true;
            continue;
        }
        table.m_Rows.insert(qMakePair(origin, kind), state);
    }
    return table;
}

QByteArray PermissionTable::ToJson() const {
    QJsonArray rows;
    foreach(const Row &row, Rows()){
        QJsonObject object;
        object[QStringLiteral("origin")] = row.origin;
        object[QStringLiteral("kind")]   = row.kind;
        object[QStringLiteral("state")]  = StateName(row.state);
        rows.append(object);
    }
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("permissions")] = rows;
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

PermissionTable::State PermissionTable::Lookup(const QString &origin, const QString &kind) const {
    return m_Rows.value(qMakePair(Origin(origin), kind), Ask);
}

bool PermissionTable::Set(const QString &spelled, const QString &kind, State state){
    const QString origin = Origin(spelled);
    if(origin.isEmpty()) return false;
    const QPair<QString, QString> key = qMakePair(origin, kind);

    if(state == Ask) return m_Rows.remove(key) > 0;
    if(!IsKind(kind)) return false;
    if(m_Rows.value(key, Ask) == state) return false;
    m_Rows.insert(key, state);
    return true;
}

bool PermissionTable::Clear(){
    if(m_Rows.isEmpty()) return false;
    m_Rows.clear();
    return true;
}

QList<PermissionTable::Row> PermissionTable::Rows() const {
    QList<Row> rows;
    for(auto it = m_Rows.constBegin(); it != m_Rows.constEnd(); ++it)
        rows << Row{ it.key().first, it.key().second, it.value() };
    std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b){
        if(a.origin != b.origin) return a.origin < b.origin;
        return KindOrder(a.kind) < KindOrder(b.kind);
    });
    return rows;
}

QList<PermissionTable::Row> PermissionTable::Delta(const PermissionTable &before) const {
    QList<Row> delta;
    foreach(const Row &row, Rows())
        if(before.m_Rows.value(qMakePair(row.origin, row.kind), Ask) != row.state)
            delta << row;
    foreach(const Row &row, before.Rows())
        if(!m_Rows.contains(qMakePair(row.origin, row.kind)))
            delta << Row{ row.origin, row.kind, Ask };
    return delta;
}

QList<PermissionTable::Row> PermissionTable::Changes(const QList<Row> &held,
                                                     const QStringList &storable) const {
    QList<Row> changes;
    QSet<QPair<QString, QString>> present;

    foreach(const Row &row, held){
        if(row.state == Ask) continue;
        const QString origin = Origin(row.origin);
        const QPair<QString, QString> key = qMakePair(origin, row.kind);
        const State wanted = (!origin.isEmpty() && storable.contains(row.kind))
            ? m_Rows.value(key, Ask) : Ask;

        if(wanted == Ask){
            changes << Row{ row.origin, row.kind, Ask };
            continue;
        }
        present.insert(key);
        if(wanted != row.state) changes << Row{ origin, row.kind, wanted };
    }

    foreach(const Row &row, Rows()){
        if(!storable.contains(row.kind)) continue;
        if(present.contains(qMakePair(row.origin, row.kind))) continue;
        changes << row;
    }
    return changes;
}
