#include "switch.hpp"

#include "extensionuserscripts.hpp"

#include <QSet>

namespace ExtensionUserScripts {

namespace {

using ExtensionMainScripts::Strings;

QString Invocation(const QString &method){
    return QStringLiteral("Error in invocation of userScripts.%1: No matching signature.").arg(method);
}

bool IdsOf(const QJsonValue &filter, bool *all, QStringList *ids){
    *all = true;
    if(filter.isUndefined() || filter.isNull()) return true;
    if(!filter.isObject()) return false;
    const QJsonValue given = filter.toObject().value(QStringLiteral("ids"));
    if(given.isUndefined() || given.isNull()) return true;
    if(!Strings(given, ids)) return false;
    *all = false;
    return true;
}

qint64 SizeOf(const Registration &r){
    qint64 size = 0;
    foreach(const Source &s, r.js) if(!s.isFile) size += s.code.toUtf8().size();
    return size;
}

bool Readable(const Registration &r, const Reader &read, QHash<QString, bool> *known, QString *error){
    foreach(const Source &s, r.js){
        if(!s.isFile) continue;
        if(!known->contains(s.file)){
            QByteArray bytes;
            known->insert(s.file, read && read(s.file, &bytes) && bytes.size() <= ExtensionMainScripts::FILE_LIMIT);
        }
        if(!known->value(s.file)){
            *error = QStringLiteral("Could not load javascript '%1' for script.").arg(s.file);
            return false;
        }
    }
    return true;
}

bool Admissible(const QList<Registration> &list, QString *error){
    if(list.size() > SCRIPTS_LIMIT){ *error = QStringLiteral("Too many user scripts."); return false; }
    qint64 total = 0;
    foreach(const Registration &r, list) total += SizeOf(r);
    if(total > TOTAL_LIMIT){ *error = QStringLiteral("The user scripts are too large."); return false; }
    return true;
}

}

bool Read(const QJsonValue &value, Registration *out, QString *error, const QString &method){
    *out = Registration();
    if(!value.isObject()){ *error = Invocation(method); return false; }
    const QJsonObject o = value.toObject();
    const QJsonValue id = o.value(QStringLiteral("id"));
    if(!id.isString()){ *error = Invocation(method); return false; }
    out->id = id.toString();
    if(out->id.isEmpty()){ *error = QStringLiteral("Script's ID must not be empty"); return false; }
    if(out->id.startsWith(QLatin1Char('_'))){
        *error = QStringLiteral("Script's ID '%1' must not start with '_'").arg(out->id);
        return false;
    }
    const QJsonValue matches = o.value(QStringLiteral("matches"));
    if(!Strings(matches, &out->matches) || out->matches.isEmpty()){
        *error = QStringLiteral("Script with ID '%1' must specify 'matches'").arg(out->id);
        return false;
    }
    foreach(const QString &key, QStringList() << QStringLiteral("excludeMatches") << QStringLiteral("includeGlobs")
                                              << QStringLiteral("excludeGlobs")){
        const QJsonValue given = o.value(key);
        QStringList *into = key == QStringLiteral("excludeMatches") ? &out->excludeMatches
            : key == QStringLiteral("includeGlobs") ? &out->includeGlobs : &out->excludeGlobs;
        if(!given.isUndefined() && !given.isNull() && !Strings(given, into)){
            *error = Invocation(method);
            return false;
        }
    }
    foreach(const QString &p, out->matches + out->excludeMatches){
        if(ExtensionMainScripts::PatternRegex(p).isEmpty()){
            *error = QStringLiteral("Script with ID '%1' has invalid value for matches[0]: Invalid match pattern '%2'")
                .arg(out->id, p);
            return false;
        }
    }
    foreach(const QString &g, out->includeGlobs + out->excludeGlobs){
        if(ExtensionMainScripts::GlobRegex(g).isEmpty()){
            *error = QStringLiteral("Script with ID '%1' has a glob which is too long.").arg(out->id);
            return false;
        }
    }
    const QJsonValue js = o.value(QStringLiteral("js"));
    if(!js.isArray() || js.toArray().isEmpty()){
        *error = QStringLiteral("Script with ID '%1' must specify at least one js source.").arg(out->id);
        return false;
    }
    if(js.toArray().size() > SOURCES_LIMIT){
        *error = QStringLiteral("Script with ID '%1' has too many js sources.").arg(out->id);
        return false;
    }
    foreach(const QJsonValue &source, js.toArray()){
        const QJsonObject s = source.toObject();
        const bool code = s.value(QStringLiteral("code")).isString(), file = s.value(QStringLiteral("file")).isString();
        if(!source.isObject() || code == file || (code && s.contains(QStringLiteral("file")))
           || (file && s.contains(QStringLiteral("code")))){
            *error = QStringLiteral("Script with ID '%1' must specify exactly one of 'code' or 'file' in each js source.").arg(out->id);
            return false;
        }
        Source one;
        if(file){
            one.isFile = true;
            one.file = s.value(QStringLiteral("file")).toString();
            while(one.file.startsWith(QLatin1Char('/'))) one.file.remove(0, 1);
            if(!ExtensionMainScripts::FileNameOk(one.file)){
                *error = QStringLiteral("Could not load javascript '%1' for script.").arg(s.value(QStringLiteral("file")).toString());
                return false;
            }
        } else {
            one.code = s.value(QStringLiteral("code")).toString();
        }
        out->js << one;
    }
    const QJsonValue world = o.value(QStringLiteral("world"));
    if(!world.isUndefined() && !world.isNull()){
        if(world == QJsonValue(QStringLiteral("MAIN"))) out->main = true;
        else if(world != QJsonValue(QStringLiteral("USER_SCRIPT"))){ *error = Invocation(method); return false; }
    }
    const QJsonValue worldId = o.value(QStringLiteral("worldId"));
    if(!worldId.isUndefined() && !worldId.isNull()){
        if(!worldId.isString()){ *error = Invocation(method); return false; }
        out->worldId = worldId.toString();
        if(out->worldId.startsWith(QLatin1Char('_'))){
            *error = QStringLiteral("World IDs beginning with '_' are reserved.");
            return false;
        }
        if(out->main && !out->worldId.isEmpty()){
            *error = QStringLiteral("World ID can only be specified for USER_SCRIPT worlds.");
            return false;
        }
    }
    const QJsonValue allFrames = o.value(QStringLiteral("allFrames"));
    if(!allFrames.isUndefined() && !allFrames.isNull()){
        if(!allFrames.isBool()){ *error = Invocation(method); return false; }
        out->allFrames = allFrames.toBool();
    }
    const QJsonValue runAt = o.value(QStringLiteral("runAt"));
    if(!runAt.isUndefined() && !runAt.isNull()){
        bool ok = false;
        out->runAt = ExtensionMainScripts::RunAtOf(runAt.toString(), &ok);
        if(!ok){ *error = Invocation(method); return false; }
    }
    return true;
}

QJsonObject Written(const Registration &r){
    QJsonObject o;
    o[QStringLiteral("id")] = r.id;
    o[QStringLiteral("matches")] = QJsonArray::fromStringList(r.matches);
    if(!r.excludeMatches.isEmpty()) o[QStringLiteral("excludeMatches")] = QJsonArray::fromStringList(r.excludeMatches);
    if(!r.includeGlobs.isEmpty()) o[QStringLiteral("includeGlobs")] = QJsonArray::fromStringList(r.includeGlobs);
    if(!r.excludeGlobs.isEmpty()) o[QStringLiteral("excludeGlobs")] = QJsonArray::fromStringList(r.excludeGlobs);
    QJsonArray js;
    foreach(const Source &s, r.js){
        QJsonObject source;
        if(s.isFile) source[QStringLiteral("file")] = s.file;
        else source[QStringLiteral("code")] = s.code;
        js.append(source);
    }
    o[QStringLiteral("js")] = js;
    o[QStringLiteral("allFrames")] = r.allFrames;
    o[QStringLiteral("runAt")] = r.runAt == ExtensionMainScripts::DocumentStart ? QStringLiteral("document_start")
        : r.runAt == ExtensionMainScripts::DocumentEnd ? QStringLiteral("document_end") : QStringLiteral("document_idle");
    o[QStringLiteral("world")] = r.main ? QStringLiteral("MAIN") : QStringLiteral("USER_SCRIPT");
    if(!r.worldId.isEmpty()) o[QStringLiteral("worldId")] = r.worldId;
    return o;
}

Book Book::Of(const QJsonArray &kept){
    Book book;
    QSet<QString> ids;
    foreach(const QJsonValue &value, kept){
        const QJsonObject o = value.toObject();
        if(value.isObject() && o.contains(QStringLiteral("worlds")) && !o.contains(QStringLiteral("id"))){
            QString error;
            foreach(const QJsonValue &one, o.value(QStringLiteral("worlds")).toArray()) book.Configure(one, &error);
            continue;
        }
        Registration r;
        QString error;
        if(!Read(value, &r, &error) || ids.contains(r.id)) continue;
        ids.insert(r.id);
        book.m_List << r;
    }
    return book;
}

QJsonArray Book::Kept() const {
    QJsonArray out;
    foreach(const Registration &r, m_List) out.append(Written(r));
    if(!m_Worlds.isEmpty()){
        QJsonObject worlds;
        worlds[QStringLiteral("worlds")] = Worlds();
        out.append(worlds);
    }
    return out;
}

bool Book::Configure(const QJsonValue &properties, QString *error){
    if(!properties.isObject()){ *error = Invocation(QStringLiteral("configureWorld")); return false; }
    const QJsonObject o = properties.toObject();
    WorldConfig c;
    const QJsonValue id = o.value(QStringLiteral("worldId")), csp = o.value(QStringLiteral("csp")),
                     messaging = o.value(QStringLiteral("messaging"));
    if(!id.isUndefined() && !id.isNull()){
        if(!id.isString()){ *error = Invocation(QStringLiteral("configureWorld")); return false; }
        c.worldId = id.toString();
        if(c.worldId.startsWith(QLatin1Char('_'))){ *error = QStringLiteral("World IDs beginning with '_' are reserved."); return false; }
    }
    if(!csp.isUndefined() && !csp.isNull()){
        if(!csp.isString()){ *error = Invocation(QStringLiteral("configureWorld")); return false; }
        c.hasCsp = true;
        c.csp = csp.toString();
    }
    if(!messaging.isUndefined() && !messaging.isNull()){
        if(!messaging.isBool()){ *error = Invocation(QStringLiteral("configureWorld")); return false; }
        c.hasMessaging = true;
        c.messaging = messaging.toBool();
    }
    for(int i = 0; i < m_Worlds.size(); i++){
        if(m_Worlds.at(i).worldId != c.worldId) continue;
        m_Worlds[i] = c;
        error->clear();
        return true;
    }
    if(m_Worlds.size() >= EXTENSION_WORLDS){ *error = QStringLiteral("Too many worlds of user scripts."); return false; }
    m_Worlds << c;
    error->clear();
    return true;
}

bool Book::Reset(const QJsonValue &worldId, QString *error){
    QString id;
    if(!worldId.isUndefined() && !worldId.isNull()){
        if(!worldId.isString()){ *error = Invocation(QStringLiteral("resetWorldConfiguration")); return false; }
        id = worldId.toString();
    }
    for(int i = 0; i < m_Worlds.size(); i++) if(m_Worlds.at(i).worldId == id){ m_Worlds.removeAt(i); break; }
    error->clear();
    return true;
}

QJsonArray Book::Worlds() const {
    QJsonArray out;
    foreach(const WorldConfig &c, m_Worlds){
        QJsonObject o;
        if(!c.worldId.isEmpty()) o[QStringLiteral("worldId")] = c.worldId;
        if(c.hasCsp) o[QStringLiteral("csp")] = c.csp;
        if(c.hasMessaging) o[QStringLiteral("messaging")] = c.messaging;
        out.append(o);
    }
    return out;
}

bool Book::Messaging(const QString &worldId) const {
    foreach(const WorldConfig &c, m_Worlds) if(c.worldId == worldId) return c.messaging;
    foreach(const WorldConfig &c, m_Worlds) if(c.worldId.isEmpty()) return c.messaging;
    return false;
}

bool Book::Register(const QJsonValue &scripts, const Reader &read, QString *error){
    if(!scripts.isArray()){ *error = Invocation(QStringLiteral("register")); return false; }
    QSet<QString> ids;
    foreach(const Registration &r, m_List) ids.insert(r.id);
    QList<Registration> all = m_List;
    QHash<QString, bool> known;
    foreach(const QJsonValue &value, scripts.toArray()){
        Registration r;
        if(!Read(value, &r, error)) return false;
        if(ids.contains(r.id)){ *error = QStringLiteral("Duplicate script ID '%1'").arg(r.id); return false; }
        if(!Readable(r, read, &known, error)) return false;
        ids.insert(r.id);
        all << r;
    }
    if(!Admissible(all, error)) return false;
    m_List = all;
    error->clear();
    return true;
}

bool Book::Update(const QJsonValue &scripts, const Reader &read, QString *error){
    if(!scripts.isArray()){ *error = Invocation(QStringLiteral("update")); return false; }
    QList<Registration> all = m_List;
    QSet<QString> seen;
    QHash<QString, bool> known;
    foreach(const QJsonValue &value, scripts.toArray()){
        const QJsonObject given = value.toObject();
        const QJsonValue id = given.value(QStringLiteral("id"));
        if(!value.isObject() || !id.isString()){ *error = Invocation(QStringLiteral("update")); return false; }
        if(seen.contains(id.toString())){ *error = QStringLiteral("Duplicate script ID '%1'").arg(id.toString()); return false; }
        seen.insert(id.toString());
        int at = -1;
        for(int i = 0; i < all.size(); i++) if(all.at(i).id == id.toString()) at = i;
        if(at < 0){ *error = QStringLiteral("Nonexistent script ID '%1'").arg(id.toString()); return false; }
        QJsonObject merged = Written(all.at(at));
        for(auto it = given.constBegin(); it != given.constEnd(); ++it)
            if(!it.value().isNull()) merged.insert(it.key(), it.value());
        if(given.value(QStringLiteral("world")) == QJsonValue(QStringLiteral("MAIN"))
           && (given.value(QStringLiteral("worldId")).isUndefined() || given.value(QStringLiteral("worldId")).isNull()))
            merged.remove(QStringLiteral("worldId"));
        Registration r;
        if(!Read(merged, &r, error, QStringLiteral("update"))) return false;
        if(given.contains(QStringLiteral("js")) && !given.value(QStringLiteral("js")).isNull() && !Readable(r, read, &known, error))
            return false;
        all[at] = r;
    }
    if(!Admissible(all, error)) return false;
    m_List = all;
    error->clear();
    return true;
}

bool Book::Unregister(const QJsonValue &filter, QString *error){
    bool all = true;
    QStringList ids;
    if(!IdsOf(filter, &all, &ids)){ *error = Invocation(QStringLiteral("unregister")); return false; }
    if(all){ m_List.clear(); error->clear(); return true; }
    QSet<QString> there;
    foreach(const Registration &r, m_List) there.insert(r.id);
    foreach(const QString &id, ids){
        if(!there.contains(id)){ *error = QStringLiteral("Nonexistent script ID '%1'").arg(id); return false; }
    }
    const QSet<QString> gone(ids.begin(), ids.end());
    QList<Registration> kept;
    foreach(const Registration &r, m_List) if(!gone.contains(r.id)) kept << r;
    m_List = kept;
    error->clear();
    return true;
}

QJsonArray Book::Get(const QJsonValue &filter, QString *error) const {
    bool all = true;
    QStringList ids;
    if(!IdsOf(filter, &all, &ids)){ *error = Invocation(QStringLiteral("getScripts")); return QJsonArray(); }
    error->clear();
    const QSet<QString> wanted(ids.begin(), ids.end());
    QJsonArray out;
    foreach(const Registration &r, m_List) if(all || wanted.contains(r.id)) out.append(Written(r));
    return out;
}

QStringList Book::WorldIds() const {
    QStringList out;
    foreach(const Registration &r, m_List) if(!r.main && !out.contains(r.worldId)) out << r.worldId;
    return out;
}

bool Worlds::Take(const QString &extensionId, const QStringList &worldIds){
    QStringList missing;
    foreach(const QString &w, worldIds){
        const QString key = extensionId + QLatin1Char('\n') + w;
        if(!m_Of.contains(key) && !missing.contains(key)) missing << key;
    }
    if(missing.isEmpty()) return true;
    if(m_Given.value(extensionId) + missing.size() > EXTENSION_WORLDS
       || m_Next + quint32(missing.size()) - 1 > LAST_WORLD) return false;
    foreach(const QString &key, missing) m_Of.insert(key, m_Next++);
    m_Given[extensionId] += missing.size();
    return true;
}

quint32 Worlds::Of(const QString &extensionId, const QString &worldId) const {
    return m_Of.value(extensionId + QLatin1Char('\n') + worldId, 0);
}

QList<ExtensionMainScripts::Script> ScriptsOf(const QString &extensionId, const Book &book,
                                             const QStringList &hostPermissions, const Worlds &worlds,
                                             const Reader &read, QStringList *skipped, const Prelude &prelude){
    QList<ExtensionMainScripts::Script> out;
    qint64 total = 0;
    if(prelude){
        ExtensionMainScripts::Registration everywhere;
        everywhere.id = QStringLiteral("_world");
        everywhere.matches = QStringList() << QStringLiteral("<all_urls>");
        const QString header = ExtensionMainScripts::Header(everywhere, hostPermissions);
        const QStringList ids = book.WorldIds();
        for(int i = 0; i < ids.size() && !header.isEmpty(); i++){
            const quint32 world = worlds.Of(extensionId, ids.at(i));
            if(!world || !book.Messaging(ids.at(i))) continue;
            const QString source = prelude(ids.at(i));
            if(source.isEmpty()) continue;
            ExtensionMainScripts::Script s;
            s.name = QStringLiteral("vanilla-us/%1/_world/%2").arg(extensionId).arg(i);
            s.source = header + source;
            s.runAt = ExtensionMainScripts::DocumentStart;
            s.subFrames = true;
            s.world = world;
            out << s;
        }
    }
    QHash<QString, QByteArray> files;
    QSet<QString> unreadable;
    foreach(const Registration &r, book.List()){
        ExtensionMainScripts::Registration where;
        where.id = r.id;
        where.matches = r.matches;
        where.excludeMatches = r.excludeMatches;
        where.includeGlobs = r.includeGlobs;
        where.excludeGlobs = r.excludeGlobs;
        const QString header = ExtensionMainScripts::Header(where, hostPermissions);
        if(header.isEmpty()) continue;
        const quint32 world = r.main ? 0 : worlds.Of(extensionId, r.worldId);
        if(!r.main && !world){
            if(skipped) *skipped << QStringLiteral("'%1' has no world").arg(r.id);
            continue;
        }
        QStringList bodies;
        qint64 size = 0;
        QString why;
        foreach(const Source &s, r.js){
            QByteArray bytes;
            if(!s.isFile) bytes = s.code.toUtf8();
            else if(files.contains(s.file)) bytes = files.value(s.file);
            else if(!unreadable.contains(s.file)){
                if(read && read(s.file, &bytes) && bytes.size() <= ExtensionMainScripts::FILE_LIMIT) files.insert(s.file, bytes);
                else { unreadable.insert(s.file); bytes.clear(); }
            }
            if(s.isFile && unreadable.contains(s.file)){ why = QStringLiteral("'%1' could not read '%2'").arg(r.id, s.file); break; }
            if(total + size + bytes.size() > TOTAL_LIMIT){ why = QStringLiteral("'%1' is past what may be put in").arg(r.id); break; }
            size += bytes.size();
            bodies << QString::fromUtf8(bytes);
        }
        if(!why.isEmpty()){
            if(skipped) *skipped << why;
            continue;
        }
        total += size;
        for(int i = 0; i < bodies.size(); i++){
            ExtensionMainScripts::Script s;
            s.name = QStringLiteral("vanilla-us/%1/%2/%3").arg(extensionId, r.id).arg(i);
            s.source = header + bodies.at(i);
            s.runAt = r.runAt;
            s.subFrames = r.allFrames;
            s.world = world;
            out << s;
        }
    }
    return out;
}

bool ThrowKept(QHash<QString, QString> *checked, const QString &id, const QString &version,
               bool kept, const QString &keptVersion){
    if(checked->contains(id) && checked->value(id) == version) return false;
    checked->insert(id, version);
    return kept && keptVersion != version;
}

QString StoreFileName(const QString &profileKey){
    return ExtensionMainScripts::ProfileFileName(QStringLiteral("extension-user-scripts-"), profileKey);
}

}
