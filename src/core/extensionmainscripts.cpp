#include "switch.hpp"

#include "extensionmainscripts.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

namespace ExtensionMainScripts {

namespace {

bool Printable(const QString &text){
    foreach(const QChar c, text)
        if(c.isSpace() || c.category() == QChar::Other_Control || c.category() == QChar::Other_Format
           || c == QChar::LineSeparator || c == QChar::ParagraphSeparator) return false;
    return true;
}

QString Escaped(const QString &text){
    return QRegularExpression::escape(text);
}

QString PathRegex(const QString &path){
    QStringList pieces;
    foreach(const QString &piece, path.split(QLatin1Char('*'))) pieces << Escaped(piece);
    return pieces.join(QStringLiteral("[^#]*"));
}

bool IsDevice(const QString &segment){
    static const QRegularExpression device(QStringLiteral("^(con|prn|aux|nul|com[0-9]|lpt[0-9]|conin\\$|conout\\$)(\\..*)?$"),
                                           QRegularExpression::CaseInsensitiveOption);
    return device.match(segment).hasMatch();
}

}

RunAt RunAtOf(const QString &text, bool *ok){
    *ok = true;
    if(text == QStringLiteral("document_start")) return DocumentStart;
    if(text == QStringLiteral("document_end")) return DocumentEnd;
    if(text == QStringLiteral("document_idle")) return DocumentIdle;
    *ok = false;
    return DocumentIdle;
}

bool Strings(const QJsonValue &value, QStringList *out){
    if(!value.isArray()) return false;
    foreach(const QJsonValue &v, value.toArray()){
        if(!v.isString()) return false;
        out->append(v.toString());
    }
    return true;
}

bool FileNameOk(const QString &name){
    if(name.isEmpty() || name.size() > 1024) return false;
    if(name.startsWith(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) || name.contains(QLatin1Char(':'))
       || name.contains(QLatin1Char('?')) || name.contains(QLatin1Char('#'))) return false;
    foreach(const QChar c, name)
        if(c.unicode() < 0x20 || c.unicode() == 0x7f || c.category() == QChar::Other_Format) return false;
    foreach(const QString &segment, name.split(QLatin1Char('/'))){
        if(segment.isEmpty() || segment == QStringLiteral(".") || segment == QStringLiteral("..")) return false;
        if(segment.endsWith(QLatin1Char('.')) || segment.endsWith(QLatin1Char(' '))) return false;
        if(IsDevice(segment)) return false;
    }
    return true;
}

QString PatternRegex(const QString &pattern, bool hostPermission){
    if(!Printable(pattern)) return QString();
    if(pattern == QStringLiteral("<all_urls>")) return QStringLiteral("https?:\\/\\/.*$");
    static const QRegularExpression shape(QStringLiteral("^(\\*|[a-z][a-z0-9+.-]*)://([^/]*)(/.*)$"),
                                          QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = shape.match(pattern);
    if(!m.hasMatch()) return QString();
    const QString scheme = m.captured(1).toLower();
    static const QStringList schemes = QStringList() << QStringLiteral("*") << QStringLiteral("http") << QStringLiteral("https")
        << QStringLiteral("file") << QStringLiteral("ftp") << QStringLiteral("ws") << QStringLiteral("wss");
    if(!schemes.contains(scheme)) return QString();
    static const QRegularExpression hostPort(QStringLiteral("^(\\[[^\\]]*\\]|[^:]*)(?::(\\*|\\d+))?$"));
    const QRegularExpressionMatch hp = hostPort.match(m.captured(2).toLower());
    if(!hp.hasMatch()) return QString();
    const QString host = hp.captured(1), port = hp.captured(2);
    static const QRegularExpression name(QStringLiteral("^(\\*\\.)?[a-z0-9_-]+(\\.[a-z0-9_-]+)*\\.?$"));
    static const QRegularExpression v6(QStringLiteral("^\\[[0-9a-f:.]+\\]$"));
    if(scheme == QStringLiteral("file") ? !host.isEmpty()
       : !(host == QStringLiteral("*") || name.match(host).hasMatch() || v6.match(host).hasMatch())) return QString();
    const QString path = hostPermission ? QStringLiteral("/*") : m.captured(3);

    if(scheme != QStringLiteral("*") && scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))
        return QStringLiteral("(?!)");

    auto of = [&](const QString &one){
        QString out = one + QStringLiteral(":\\/\\/(?:[^\\/?#@]*@)?");
        if(host == QStringLiteral("*"))
            out += QStringLiteral("(?:\\[[^\\]\\/?#@]*\\]|[^\\/?#@:]*)");
        else if(host.startsWith(QStringLiteral("*.")))
            out += QStringLiteral("(?:[^\\/?#@:]*\\.)?") + Escaped(host.mid(2));
        else
            out += Escaped(host);
        const QString fallback = one == QStringLiteral("http") ? QStringLiteral("80") : QStringLiteral("443");
        if(port.isEmpty() || port == QStringLiteral("*")) out += QStringLiteral("(?::\\d+)?");
        else if(port == fallback) out += QStringLiteral("(?::") + port + QStringLiteral(")?");
        else out += QStringLiteral(":") + port;
        return out + PathRegex(path) + QStringLiteral("(?:#.*)?$");
    };
    if(scheme == QStringLiteral("*"))
        return QStringLiteral("(?:") + of(QStringLiteral("http")) + QLatin1Char('|') + of(QStringLiteral("https")) + QLatin1Char(')');
    return of(scheme);
}

QString Header(const Registration &registration, const QStringList &hostPermissions){
    auto any = [](const QStringList &patterns, bool host, bool *ok){
        QStringList each;
        foreach(const QString &p, patterns){
            const QString re = PatternRegex(p, host);
            if(re.isEmpty()){ *ok = false; return QString(); }
            each << re;
        }
        return QStringLiteral("(?:") + each.join(QLatin1Char('|')) + QStringLiteral(")");
    };
    QStringList hosts;
    foreach(const QString &p, hostPermissions) if(!PatternRegex(p, true).isEmpty()) hosts << p;
    if(registration.matches.isEmpty() || hosts.isEmpty()) return QString();
    bool ok = true;
    QString line = QStringLiteral("^(?=") + any(registration.matches, false, &ok) + QStringLiteral(")(?=")
        + any(hosts, true, &ok) + QStringLiteral(")");
    if(!registration.excludeMatches.isEmpty())
        line += QStringLiteral("(?!") + any(registration.excludeMatches, false, &ok) + QStringLiteral(")");
    auto globs = [](const QStringList &list, bool *ok){
        QStringList each;
        foreach(const QString &g, list){
            const QString re = GlobRegex(g);
            if(re.isEmpty()){ *ok = false; return QString(); }
            each << re;
        }
        return QStringLiteral("(?-i:") + each.join(QLatin1Char('|')) + QStringLiteral(")$");
    };
    if(!registration.includeGlobs.isEmpty())
        line += QStringLiteral("(?=") + globs(registration.includeGlobs, &ok) + QStringLiteral(")");
    if(!registration.excludeGlobs.isEmpty())
        line += QStringLiteral("(?!") + globs(registration.excludeGlobs, &ok) + QStringLiteral(")");
    if(!ok) return QString();
    return QStringLiteral("// ==UserScript==\n// @include /") + line + QStringLiteral("/\n// ==/UserScript==\n");
}

QString GlobRegex(const QString &glob){
    if(glob.size() > GLOB_LIMIT) return QString();
    QString out = QStringLiteral("(?:");
    bool star = false;
    for(int i = 0; i < glob.size(); i++){
        QChar c = glob.at(i);
        if(c == QLatin1Char('*')){
            if(!star) out += QStringLiteral(".*");
            star = true;
            continue;
        }
        star = false;
        if(c == QLatin1Char('?')){ out += QStringLiteral(".?"); continue; }
        if(c == QLatin1Char('\\') && i + 1 < glob.size()) c = glob.at(++i);
        if(c.unicode() <= 0x20 || c.unicode() >= 0x7f){
            uint point = c.unicode();
            if(c.isHighSurrogate() && i + 1 < glob.size() && glob.at(i + 1).isLowSurrogate())
                point = QChar::surrogateToUcs4(c, glob.at(++i));
            out += QStringLiteral("\\x{%1}").arg(point, 0, 16);
        } else {
            out += QRegularExpression::escape(QString(c));
        }
    }
    return out + QLatin1Char(')');
}

bool ReadInside(const QString &folder, const QString &name, QByteArray *bytes){
    if(!FileNameOk(name)) return false;
    const QString root = folder.isEmpty() ? QString() : QDir(folder).canonicalPath();
    if(root.isEmpty()) return false;
    QString step = root;
    foreach(const QString &segment, name.split(QLatin1Char('/'))){
        step += QLatin1Char('/') + segment;
        const QFileInfo each(step);
        if(each.isSymLink() || each.isJunction() || each.isShortcut()) return false;
    }
    const QString real = QFileInfo(root + QLatin1Char('/') + name).canonicalFilePath();
    if(real.isEmpty() || !real.startsWith(root + QLatin1Char('/'))) return false;
    const QFileInfo info(real);
    if(!info.isFile() || info.size() > FILE_LIMIT) return false;
    QFile file(real);
    if(!file.open(QIODevice::ReadOnly)) return false;
    *bytes = file.read(FILE_LIMIT + 1);
    return bytes->size() <= FILE_LIMIT;
}

QList<Registration> Parse(const QJsonArray &args, QString *error){
    QList<Registration> out;
    auto fail = [&](const QString &what){ *error = what; return QList<Registration>(); };
    error->clear();
    if(args.size() != 1 || !args.at(0).isArray()) return fail(QStringLiteral("Expected a list of scripts."));
    const QJsonArray list = args.at(0).toArray();
    if(list.size() > SCRIPTS_LIMIT) return fail(QStringLiteral("Too many content scripts are registered."));
    QSet<QString> seen;
    foreach(const QJsonValue &value, list){
        if(!value.isObject()) return fail(QStringLiteral("Expected a list of scripts."));
        const QJsonObject o = value.toObject();
        Registration r;
        r.id = o.value(QStringLiteral("id")).toString();
        if(r.id.isEmpty() || !Printable(r.id) || seen.contains(r.id)) return fail(QStringLiteral("A script's id is missing, unreadable or repeated."));
        seen.insert(r.id);
        const QString wrong = QStringLiteral("Script with ID '%1' has invalid value for '%2'.");
        if(!Strings(o.value(QStringLiteral("js")), &r.js) || r.js.isEmpty() || r.js.size() > FILES_LIMIT)
            return fail(wrong.arg(r.id, QStringLiteral("js")));
        foreach(const QString &f, r.js) if(!FileNameOk(f)) return fail(wrong.arg(r.id, QStringLiteral("js")));
        if(!Strings(o.value(QStringLiteral("matches")), &r.matches) || r.matches.isEmpty())
            return fail(wrong.arg(r.id, QStringLiteral("matches")));
        foreach(const QString &p, r.matches) if(PatternRegex(p).isEmpty()) return fail(wrong.arg(r.id, QStringLiteral("matches")));
        const QJsonValue excludes = o.value(QStringLiteral("excludeMatches"));
        if(!excludes.isUndefined()){
            if(!Strings(excludes, &r.excludeMatches)) return fail(wrong.arg(r.id, QStringLiteral("excludeMatches")));
            foreach(const QString &p, r.excludeMatches) if(PatternRegex(p).isEmpty()) return fail(wrong.arg(r.id, QStringLiteral("excludeMatches")));
        }
        const QJsonValue all = o.value(QStringLiteral("allFrames"));
        if(!all.isUndefined() && !all.isBool()) return fail(wrong.arg(r.id, QStringLiteral("allFrames")));
        r.allFrames = all.toBool(false);
        const QJsonValue at = o.value(QStringLiteral("runAt"));
        if(!at.isUndefined()){
            bool ok = at.isString();
            if(ok) r.runAt = RunAtOf(at.toString(), &ok);
            if(!ok) return fail(wrong.arg(r.id, QStringLiteral("runAt")));
        }
        const QJsonValue persist = o.value(QStringLiteral("persistAcrossSessions"));
        if(!persist.isUndefined() && !persist.isBool()) return fail(wrong.arg(r.id, QStringLiteral("persistAcrossSessions")));
        r.persist = persist.toBool(true);
        out << r;
    }
    return out;
}

QList<Script> ScriptsOf(const QString &extensionId, const QList<Registration> &list,
                        const QStringList &hostPermissions, const Reader &read, QString *error){
    error->clear();
    QList<Script> out;
    qint64 total = 0;
    foreach(const Registration &r, list){
        const QString header = Header(r, hostPermissions);
        if(header.isEmpty()) continue;
        for(int i = 0; i < r.js.size(); i++){
            QByteArray bytes;
            if(!FileNameOk(r.js.at(i)) || !read(r.js.at(i), &bytes) || bytes.size() > FILE_LIMIT){
                *error = QStringLiteral("Could not load javascript '%1' for content script.").arg(r.js.at(i));
                return QList<Script>();
            }
            total += bytes.size();
            if(total > TOTAL_LIMIT){
                *error = QStringLiteral("The content scripts are too large.");
                return QList<Script>();
            }
            Script s;
            s.name = QStringLiteral("vanilla-ext/%1/%2/%3").arg(extensionId, r.id).arg(i);
            s.source = header + QString::fromUtf8(bytes);
            s.runAt = r.runAt;
            s.subFrames = r.allFrames;
            out << s;
        }
    }
    return out;
}

Store Store::FromJson(const QByteArray &bytes){
    Store store;
    const QJsonObject all = QJsonDocument::fromJson(bytes).object();
    for(auto it = all.constBegin(); it != all.constEnd(); ++it){
        const QJsonObject o = it.value().toObject();
        const QJsonValue version = o.value(QStringLiteral("version")), list = o.value(QStringLiteral("list"));
        if(it.key().isEmpty() || !version.isString() || !list.isArray() || list.toArray().isEmpty()) continue;
        store.entries.insert(it.key(), Entry{ version.toString(), list.toArray() });
    }
    return store;
}

QByteArray Store::ToJson() const {
    QJsonObject all;
    for(auto it = entries.constBegin(); it != entries.constEnd(); ++it){
        QJsonObject o;
        o[QStringLiteral("version")] = it->version;
        o[QStringLiteral("list")] = it->list;
        all[it.key()] = o;
    }
    return QJsonDocument(all).toJson(QJsonDocument::Compact);
}

void Store::Put(const QString &id, const QString &version, const QJsonArray &sent){
    QJsonArray kept;
    foreach(const QJsonValue &one, sent.size() == 1 ? sent.at(0).toArray() : QJsonArray())
        if(one.toObject().value(QStringLiteral("persistAcrossSessions")).toBool(true)) kept.append(one);
    if(kept.isEmpty()) entries.remove(id);
    else entries.insert(id, Entry{ version, kept });
}

QString ProfileFileName(const QString &prefix, const QString &profileKey){
    const QByteArray digest = QCryptographicHash::hash(profileKey.toUtf8(), QCryptographicHash::Sha256).toHex().left(24);
    return prefix + QString::fromLatin1(digest) + QStringLiteral(".json");
}

QString StoreFileName(const QString &profileKey){
    return ProfileFileName(QStringLiteral("extension-main-scripts-"), profileKey);
}

Restore RestoreOf(bool wanted, const QString &keptVersion, const QString &version){
    if(!wanted) return LeaveIt;
    return keptVersion == version ? PutIn : ThrowAway;
}

Restored RestoreInto(Table &table, Store &store, const Now &now){
    Restored out;
    foreach(const QString &id, store.entries.keys()){
        if(table.Has(id)) continue;
        const QString folder = now.folder(id);
        const Store::Entry kept = store.entries.value(id);
        const Restore restore = RestoreOf(now.wanted(id, folder), kept.version, now.version(id));
        if(restore == LeaveIt) continue;
        QString error;
        QList<Script> scripts;
        if(restore == PutIn){
            const QList<Registration> list = Parse(QJsonArray() << kept.list, &error);
            const QString root = QDir(folder).canonicalPath();
            if(error.isEmpty())
                scripts = ScriptsOf(id, list, now.hostPermissions(id),
                    [&root](const QString &name, QByteArray *bytes){ return ReadInside(root, name, bytes); }, &error);
        }
        if(restore == ThrowAway || !error.isEmpty() || scripts.isEmpty()){
            store.Drop(id);
            out.thrown = true;
            continue;
        }
        const Table::Change put = table.Put(id, folder, scripts);
        out.change.removed += put.removed;
        out.change.inserted += put.inserted;
    }
    return out;
}

Table::Change Table::Put(const QString &id, const QString &folder, const QList<Script> &scripts){
    Change change;
    const auto it = m_Held.constFind(id);
    if(it != m_Held.constEnd() && it->folder == folder && it->scripts == scripts) return change;
    if(it != m_Held.constEnd()) change.removed = it->scripts;
    change.inserted = scripts;
    if(scripts.isEmpty()) m_Held.remove(id);
    else m_Held.insert(id, Held{ folder, scripts });
    return change;
}

Table::Change Table::Keep(const std::function<bool(const QString &id, const QString &folder)> &wanted){
    Change change;
    for(auto it = m_Held.begin(); it != m_Held.end(); ){
        if(wanted(it.key(), it->folder)){ ++it; continue; }
        change.removed += it->scripts;
        it = m_Held.erase(it);
    }
    return change;
}

}
