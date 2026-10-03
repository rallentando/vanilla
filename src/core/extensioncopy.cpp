#include "switch.hpp"

#include "extensioncopy.hpp"

#include "cdpshims.hpp"
#include "extensionhostwire.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QLocale>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#endif

namespace ExtensionCopy {

namespace {

const QString SHIM_PREFIX = QStringLiteral("vanilla_");
const QString CONTENT_SHIM = QStringLiteral("vanilla_content_shim.js");
const QString CARRIER = QStringLiteral("vanilla_carrier.js");
const char CARRIER_MARK[] = "self.__vanillaCarrier = 1;\n";
const QString WORKER_SHIM = QStringLiteral("vanilla_worker_shim.js");
const QString PAGE_SHIM = QStringLiteral("vanilla_page_shim.js");
const QString PAGE_SHIM_OPEN = QStringLiteral("vanilla_page_shim_open.js");
const QString RELAY_PAGE = QStringLiteral("vanilla_relay.html");
const QString RELAY_SCRIPT = QStringLiteral("vanilla_relay.js");
const char RELAY_HTML[] = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Vanilla relay</title></head>"
                          "<body><script src=\"/vanilla_relay.js\"></script></body></html>\n";
const QString WAKE_PAGE = QStringLiteral("vanilla_wake.html");
const QString WAKE_SCRIPT = QStringLiteral("vanilla_wake.js");
const char WAKE_HTML[] = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Vanilla wake</title></head>"
                         "<body><script src=\"/vanilla_wake.js\"></script></body></html>\n";
const QString COMPLETE = QStringLiteral(".vanilla-copy-complete");
const int PAGE_LIMIT = 8 * 1024 * 1024;
const QString MAKING = QStringLiteral(".making-");
const QString PINNED = QStringLiteral("edge");
const qint64 MESSAGES_LIMIT = 4 * 1024 * 1024;

bool Numbers(const QString &version, QStringList *parts){
    *parts = version.split(QLatin1Char('.'));
    if(parts->isEmpty() || parts->size() > 4) return false;
    foreach(const QString &part, *parts){
        bool ok = false;
        const int n = part.toInt(&ok);
        if(!ok || n < 0 || n > 65535 || part != QString::number(n)) return false;
    }
    return true;
}

QHash<QString, QString> g_Sources;
QHash<QString, Made> g_Made;
QMutex g_Mutex;

QString Key(const QString &path){
    const QFileInfo info(QDir::fromNativeSeparators(path));
    QString clean = info.canonicalFilePath();
    if(clean.isEmpty()) clean = info.absoluteFilePath();
    clean = QDir::cleanPath(clean);
#ifdef Q_OS_WIN
    return clean.toLower();
#else
    return clean;
#endif
}

bool Write(const QString &path, const QByteArray &bytes){
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}

bool Blank(char c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

bool StartsTag(const QByteArray &html, int at, const char *name){
    const int length = static_cast<int>(qstrlen(name));
    if(html.size() < at + length + 1) return false;
    if(qstrnicmp(html.constData() + at, name, length) != 0) return false;
    const char next = html.at(at + length);
    return Blank(next) || next == '>' || next == '/';
}

int PastTag(const QByteArray &html, int at){
    enum State { TagName, BeforeName, Name, AfterName, BeforeValue, Quoted, AfterQuoted, Unquoted };
    State state = TagName;
    char quote = 0;
    for(int i = at + 1; i < html.size(); i++){
        const char c = html.at(i);
        if(state == Quoted){
            if(c == quote) state = AfterQuoted;
            continue;
        }
        if(c == '>') return i + 1;
        switch(state){
        case TagName:
            if(Blank(c) || c == '/') state = BeforeName;
            break;
        case Name:
            if(Blank(c)) state = AfterName;
            else if(c == '/') state = BeforeName;
            else if(c == '=') state = BeforeValue;
            break;
        case BeforeName:
            if(!Blank(c) && c != '/') state = Name;
            break;
        case AfterName:
            if(c == '=') state = BeforeValue;
            else if(c == '/') state = BeforeName;
            else if(!Blank(c)) state = Name;
            break;
        case BeforeValue:
            if(c == '"' || c == '\''){ quote = c; state = Quoted; }
            else if(!Blank(c)) state = Unquoted;
            break;
        case AfterQuoted:
            if(Blank(c) || c == '/') state = BeforeName;
            else state = Name;
            break;
        case Unquoted:
            if(Blank(c)) state = BeforeName;
            break;
        case Quoted:
            break;
        }
    }
    return -1;
}

bool Star(const QString &p, const QString &s){
    int pi = 0, si = 0, star = -1, mark = 0;
    while(si < s.size()){
        if(pi < p.size() && p.at(pi) == QLatin1Char('*')){ star = pi++; mark = si; }
        else if(pi < p.size() && p.at(pi) == s.at(si)){ pi++; si++; }
        else if(star >= 0){ pi = star + 1; si = ++mark; }
        else return false;
    }
    while(pi < p.size() && p.at(pi) == QLatin1Char('*')) pi++;
    return pi == p.size();
}

const char COPY_FORMAT[] = "copy format 13";

QByteArray CompleteMark(int stamp){
    return QByteArray(COPY_FORMAT) + ' ' + QByteArray::number(stamp);
}

const char KEYLESS_NOTE[] =
    "try { console.warn('Vanilla: the shims of this extension were written without their key, because its"
    " web_accessible_resources let the web read a file which would carry it; it asks the application for nothing'); } catch (e) {}\n";

bool Completed(const QString &copy, int stamp){
    QFile mark(copy + QLatin1Char('/') + COMPLETE);
    return mark.open(QIODevice::ReadOnly) && mark.read(64) == CompleteMark(stamp);
}

struct Reach {
    enum Kind { Unforeseeable, File, Folder };
    Kind kind = Unforeseeable;
    QString path;
};

Reach ReachOf(const QString &pattern){
    Reach reach;
    QString p = pattern;
    while(p.startsWith(QLatin1Char('/'))) p.remove(0, 1);
    QString path = p;
    Reach::Kind kind = Reach::File;
    if(p == QLatin1String("*")){ path.clear(); kind = Reach::Folder; }
    else if(p.endsWith(QStringLiteral("/*"))){ path = p.chopped(2); kind = Reach::Folder; }
    static const QRegularExpression plain(QStringLiteral("\\A[A-Za-z0-9_.-]+\\z"));
    if(!path.isEmpty()){
        const QStringList parts = path.split(QLatin1Char('/'));
        for(int i = 0; i < parts.size(); i++){
            if(plain.match(parts.at(i)).hasMatch() && !parts.at(i).endsWith(QLatin1Char('.'))) continue;
            if(i == 0) return reach;
            reach.kind = Reach::Folder;
            reach.path = QStringList(parts.mid(0, i)).join(QLatin1Char('/')).toLower();
            return reach;
        }
    } else if(kind == Reach::File){
        return reach;
    }
    reach.kind = kind;
    reach.path = path.toLower();
    return reach;
}

bool AnyGlobbed(const QStringList &patterns, const QString &relative){
    foreach(const QString &pattern, patterns)
        if(Globbed(pattern, relative)) return true;
    return false;
}

int PastComment(const QByteArray &html, int at){
    const int from = at + 4;
    if(html.mid(from, 1) == ">") return from + 1;
    if(html.mid(from, 2) == "->") return from + 2;
    const int plain = html.indexOf("-->", from), banged = html.indexOf("--!>", from);
    if(plain < 0 && banged < 0) return -1;
    if(banged < 0 || (plain >= 0 && plain < banged)) return plain + 3;
    return banged + 4;
}

bool Skipped(const QString &relative){
    foreach(const QString &name, QStringList() << QStringLiteral("_metadata") << QStringLiteral(".git"))
        if(relative == name || relative.startsWith(name + QLatin1Char('/'))) return true;
    return false;
}

bool CopyTree(const QString &from, const QString &to, QString *why, QString *detail){
    QDirIterator it(from, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    const QDir base(from);
    while(it.hasNext()){
        it.next();
        const QFileInfo info = it.fileInfo();
        const QString relative = base.relativeFilePath(info.absoluteFilePath());
        if(Skipped(relative)) continue;
        if(info.isSymLink() || info.isJunction()){
            *why = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "it contains a link (%1)"));
            *detail = relative;
            return false;
        }
        if(info.fileName().startsWith(SHIM_PREFIX, Qt::CaseInsensitive)){
            *why = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "it has a file named like the ones Vanilla adds (%1)"));
            *detail = relative;
            return false;
        }
        if(info.isDir()){
            if(!QDir().mkpath(to + QLatin1Char('/') + relative)){
                *why = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "a directory could not be made"));
                return false;
            }
            continue;
        }
        QDir().mkpath(QFileInfo(to + QLatin1Char('/') + relative).absolutePath());
        if(!QFile::copy(info.absoluteFilePath(), to + QLatin1Char('/') + relative)){
            *why = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "a file could not be copied (%1)"));
            *detail = relative;
            return false;
        }
    }
    return true;
}

}

int PlaceOfPageShim(const QByteArray &html){
    if(html.size() > PAGE_LIMIT) return -1;
    if(html.startsWith("\xFF\xFE") || html.startsWith("\xFE\xFF") || html.left(4).contains('\0')) return -1;
    int at = html.startsWith("\xEF\xBB\xBF") ? 3 : 0;
    bool root = false;
    for(;;){
        while(at < html.size() && Blank(html.at(at))) at++;
        if(at >= html.size() || html.at(at) != '<') return at;
        int past;
        if(html.mid(at, 4) == "<!--"){
            past = PastComment(html, at);
        } else if(html.mid(at, 2) == "<!" || html.mid(at, 2) == "<?"){
            past = html.indexOf('>', at);
            if(past >= 0) past++;
        } else if(StartsTag(html, at, "<head")){
            return PastTag(html, at);
        } else if(!root && StartsTag(html, at, "<html")){
            root = true;
            past = PastTag(html, at);
        } else {
            return at;
        }
        if(past < 0) return -1;
        at = past;
    }
}

QByteArray PageShimLine(bool open){
    return QByteArrayLiteral("<script src=\"/") + (open ? PAGE_SHIM_OPEN : PAGE_SHIM).toLatin1() + QByteArrayLiteral("\"></script>");
}

bool Readable(const QStringList &patterns, const QString &relative){
    const QString file = relative.toLower();
    foreach(const QString &pattern, patterns){
        const Reach reach = ReachOf(pattern);
        if(reach.kind == Reach::Unforeseeable) return true;
        if(reach.kind == Reach::File && reach.path == file) return true;
        if(reach.kind == Reach::Folder && (reach.path.isEmpty() || file == reach.path || file.startsWith(reach.path + QLatin1Char('/')))) return true;
    }
    return false;
}

bool Globbed(const QString &pattern, const QString &path){
    QString p = pattern.toLower();
    while(p.startsWith(QLatin1Char('/'))) p.remove(0, 1);
    const QString s = path.toLower();
    if(Star(p, s)) return true;
    return p.endsWith(QStringLiteral("/*")) && p.chopped(2) == s;
}

QString ConnectingPolicy(const QString &policy){
    const QString scheme = QString::fromLatin1(ExtensionHostWire::SCHEME) + QLatin1Char(':');
    QStringList directives = policy.split(QLatin1Char(';'));
    int connect = -1, fallback = -1;
    for(int i = 0; i < directives.size(); i++){
        const QString name = directives.at(i).trimmed().section(QLatin1Char(' '), 0, 0).toLower();
        if(name == QLatin1String("connect-src") && connect < 0) connect = i;
        if(name == QLatin1String("default-src") && fallback < 0) fallback = i;
    }
    const int which = connect >= 0 ? connect : fallback;
    if(which < 0) return policy;
    QStringList sources = directives.at(which).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    const QString name = sources.takeFirst();
    if(sources.contains(scheme, Qt::CaseInsensitive)) return policy;
    sources.removeAll(QStringLiteral("'none'"));
    sources << scheme;
    if(connect >= 0){
        directives[connect] = QLatin1Char(' ') + name + QLatin1Char(' ') + sources.join(QLatin1Char(' '));
        return directives.join(QLatin1Char(';')).trimmed();
    }
    return policy.trimmed() + (policy.trimmed().endsWith(QLatin1Char(';')) ? QStringLiteral(" ") : QStringLiteral("; "))
        + QStringLiteral("connect-src ") + sources.join(QLatin1Char(' '));
}

Rewritten Rewrite(const QJsonObject &manifest, int stamp){
    Rewritten result;
    if(manifest[QStringLiteral("manifest_version")].toInt() != 3){
        result.refusal = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "it is not a manifest version 3 extension"));
        return result;
    }
    if(manifest[QStringLiteral("key")].toString().isEmpty()){
        result.refusal = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "its manifest has no 'key', so a copy would be another extension with none of its data"));
        return result;
    }
    QStringList parts;
    if(!Numbers(manifest[QStringLiteral("version")].toString(), &parts) || stamp < 1 || stamp > 65535){
        result.refusal = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "its version is not one to four numbers"));
        return result;
    }
    QJsonObject out = manifest;

    QJsonArray entries = manifest.value(QStringLiteral("content_scripts")).toArray();
    for(int i = 0; i < entries.size(); i++){
        QJsonObject entry = entries.at(i).toObject();
        QJsonArray scripts = entry.value(QStringLiteral("js")).toArray();
        if(scripts.isEmpty() || entry.value(QStringLiteral("world")).toString() == QStringLiteral("MAIN")) continue;
        scripts.prepend(CONTENT_SHIM);
        entry[QStringLiteral("js")] = scripts;
        entries[i] = entry;
        result.contentShim = CONTENT_SHIM;
    }
    QJsonArray where;
    foreach(const QJsonValue &permission, manifest.value(QStringLiteral("host_permissions")).toArray()){
        if(!permission.isString() || !CarriablePattern(permission.toString())) continue;
        const QJsonValue host(HostPatternOf(permission.toString()));
        if(!where.contains(host)) where << host;
    }
    const bool hasWorker = !manifest.value(QStringLiteral("background")).toObject().value(QStringLiteral("service_worker")).toString().isEmpty();
    if(!where.isEmpty() && hasWorker && manifest.value(QStringLiteral("permissions")).toArray().contains(QJsonValue(QStringLiteral("scripting")))){
        QJsonObject carrier;
        carrier[QStringLiteral("js")] = QJsonArray() << CARRIER;
        carrier[QStringLiteral("matches")] = where;
        carrier[QStringLiteral("all_frames")] = true;
        carrier[QStringLiteral("run_at")] = QStringLiteral("document_start");
        entries.prepend(carrier);
        result.carrier = CARRIER;
    }
    if(!result.contentShim.isEmpty() || !result.carrier.isEmpty()) out[QStringLiteral("content_scripts")] = entries;

    QJsonObject background = manifest.value(QStringLiteral("background")).toObject();
    QString original = background.value(QStringLiteral("service_worker")).toString();
    while(original.startsWith(QLatin1Char('/'))) original.remove(0, 1);
    if(!original.isEmpty()){
        if(original.contains(QLatin1Char('\\')) || QDir::isAbsolutePath(original) ||
           original.split(QLatin1Char('/')).contains(QStringLiteral(".."))){
            result.refusal = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "its worker is not a path inside it"));
            return result;
        }
        const int slash = original.lastIndexOf(QLatin1Char('/'));
        const QString folder = slash < 0 ? QString() : original.left(slash + 1);
        const QString name = original.mid(slash + 1);
        result.workerWrapper = folder + SHIM_PREFIX + QStringLiteral("worker.js");
        result.workerShim = folder + WORKER_SHIM;
        const QString copyLine = QStringLiteral("// vanilla copy %1\n").arg(stamp);
        result.workerWrapperText = copyLine + (background.value(QStringLiteral("type")).toString() == QStringLiteral("module")
            ? QStringLiteral("import './%1';\nimport './%2';\n").arg(WORKER_SHIM, name)
            : QStringLiteral("importScripts('./%1', './%2');\n").arg(WORKER_SHIM, name));
        background[QStringLiteral("service_worker")] = result.workerWrapper;
        out[QStringLiteral("background")] = background;
    }
    if(result.contentShim.isEmpty() && result.carrier.isEmpty() && result.workerWrapper.isEmpty()){
        result.refusal = QString::fromLatin1(QT_TRANSLATE_NOOP("ExtensionCopy", "it has neither a worker nor a content script"));
        return result;
    }

    result.pageShim = PAGE_SHIM;
    result.pageShimOpen = PAGE_SHIM_OPEN;
    result.relayPage = RELAY_PAGE;
    result.relayScript = RELAY_SCRIPT;
    foreach(const QJsonValue &page, manifest.value(QStringLiteral("sandbox")).toObject().value(QStringLiteral("pages")).toArray()){
        QString path = page.toString();
        while(path.startsWith(QLatin1Char('/'))) path.remove(0, 1);
        if(!path.isEmpty()) result.sandboxed << path;
    }

    const QJsonValue accessible = manifest.value(QStringLiteral("web_accessible_resources"));
    if(!accessible.isUndefined() && !accessible.isNull()){
        if(!accessible.isArray()) result.keyless = true;
        foreach(const QJsonValue &entry, accessible.toArray()){
            const QJsonValue resources = entry.toObject().value(QStringLiteral("resources"));
            if(!entry.isObject() || !resources.isArray()){ result.keyless = true; continue; }
            foreach(const QJsonValue &resource, resources.toArray()){
                if(!resource.isString()){ result.keyless = true; continue; }
                result.webAccessible << resource.toString();
            }
        }
    }
    foreach(const QString &carrier, QStringList() << result.workerShim << result.pageShim << result.relayScript << result.relayPage){
        if(!carrier.isEmpty() && Readable(result.webAccessible, carrier)) result.keyless = true;
    }

    const QJsonObject policies = manifest.value(QStringLiteral("content_security_policy")).toObject();
    const QJsonValue pages = policies.value(QStringLiteral("extension_pages"));
    if(pages.isString() && ConnectingPolicy(pages.toString()) != pages.toString()){
        QJsonObject widened = policies;
        widened[QStringLiteral("extension_pages")] = ConnectingPolicy(pages.toString());
        out[QStringLiteral("content_security_policy")] = widened;
    }

    if(parts.size() == 4) parts[3] = QString::number(stamp);
    else parts << QString::number(stamp);
    out[QStringLiteral("version")] = parts.join(QLatin1Char('.'));
    result.manifest = out;
    return result;
}

QString HostPatternOf(const QString &pattern){
    if(pattern == QStringLiteral("<all_urls>")) return pattern;
    const int scheme = pattern.indexOf(QStringLiteral("://"));
    if(scheme < 0) return pattern;
    const int slash = pattern.indexOf(QLatin1Char('/'), scheme + 3);
    return (slash < 0 ? pattern : pattern.left(slash)) + QStringLiteral("/*");
}

bool CarriablePattern(const QString &pattern){
    if(pattern == QStringLiteral("<all_urls>")) return true;
    static const QRegularExpression shape(QStringLiteral(
        "\\A(\\*|https?)://(\\*|(\\*\\.)?[A-Za-z0-9_-]+(\\.[A-Za-z0-9_-]+)*\\.?)(:(\\*|[0-9]{1,5}))?/[^\\s]*\\z"));
    return shape.match(pattern).hasMatch();
}

QJsonObject Messages(const QString &source, const QJsonObject &manifest, const QString &locale){
    QJsonObject table;
    const QString fallback = manifest.value(QStringLiteral("default_locale")).toString();
    if(fallback.isEmpty()) return table;
    static const QRegularExpression shape(QStringLiteral("\\A[A-Za-z0-9_]{1,16}\\z"));
    QStringList names;
    foreach(const QString &name, QStringList() << locale << locale.section(QLatin1Char('_'), 0, 0) << fallback)
        if(!name.isEmpty() && !names.contains(name) && shape.match(name).hasMatch()) names << name;
    for(int i = names.size() - 1; i >= 0; i--){
        QFile file(source + QStringLiteral("/_locales/") + names.at(i) + QStringLiteral("/messages.json"));
        if(!file.open(QIODevice::ReadOnly) || file.size() > MESSAGES_LIMIT) continue;
        const QByteArray bytes = file.readAll();
        const QJsonObject read = QJsonDocument::fromJson(bytes.startsWith("\xEF\xBB\xBF") ? bytes.mid(3) : bytes).object();
        for(QJsonObject::const_iterator it = read.begin(); it != read.end(); ++it){
            const QJsonObject entry = it.value().toObject();
            if(!entry.value(QStringLiteral("message")).isString()) continue;
            QJsonObject kept;
            kept[QStringLiteral("message")] = entry.value(QStringLiteral("message"));
            const QJsonObject holders = entry.value(QStringLiteral("placeholders")).toObject();
            QJsonObject folded;
            for(QJsonObject::const_iterator h = holders.begin(); h != holders.end(); ++h){
                const QJsonValue content = h.value().toObject().value(QStringLiteral("content"));
                if(!content.isString()) continue;
                QJsonObject one;
                one[QStringLiteral("content")] = content;
                folded[h.key().toLower()] = one;
            }
            if(!folded.isEmpty()) kept[QStringLiteral("placeholders")] = folded;
            table[it.key().toLower()] = kept;
        }
    }
    return table;
}

QByteArray MessagesLine(const QJsonObject &messages, const QString &locale){
    if(messages.isEmpty()) return QByteArray();
    QJsonObject carried;
    carried[QStringLiteral("locale")] = locale;
    carried[QStringLiteral("messages")] = messages;
    return QByteArrayLiteral("self.__vanillaMessages = ") + QJsonDocument(carried).toJson(QJsonDocument::Compact) + ";\n";
}

int Stamp(const QString &source, const QByteArray &manifestBytes, const QByteArray &key, const QString &locale){
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray(COPY_FORMAT));
    hash.addData(Cdp::WorkerShim().toUtf8());
    hash.addData(Cdp::ContentShim().toUtf8());
    hash.addData(Cdp::PageShim().toUtf8());
    hash.addData(Cdp::RelayScript().toUtf8());
    hash.addData(QByteArray(RELAY_HTML));
    hash.addData(QByteArray(WAKE_HTML));
    hash.addData(Cdp::WakeScript().toUtf8());
    hash.addData(PageShimLine());
    hash.addData(PageShimLine(true));
    hash.addData(KEYLESS_NOTE);
    hash.addData(CARRIER_MARK);
    hash.addData(key);
    hash.addData(locale.toUtf8());
    hash.addData(manifestBytes);
    QStringList files;
    const QDir base(source);
    QDirIterator it(source, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while(it.hasNext()){
        it.next();
        const QFileInfo info = it.fileInfo();
        const QString relative = base.relativeFilePath(info.absoluteFilePath());
        if(Skipped(relative)) continue;
        files << relative + QLatin1Char('|') + QString::number(info.size()) + QLatin1Char('|')
                 + QString::number(info.lastModified().toMSecsSinceEpoch());
    }
    files.sort();
    hash.addData(files.join(QLatin1Char('\n')).toUtf8());
    const QByteArray digest = hash.result();
    const int n = (static_cast<quint8>(digest.at(0)) << 8) | static_cast<quint8>(digest.at(1));
    return n ? n : 1;
}

Made Make(const QString &source, const QString &root, const QString &id, const QByteArray &key, const QString &locale){
    const QString from = QDir(source).absolutePath();
    QMutexLocker lock(&g_Mutex);
    if(g_Made.contains(from)) return g_Made.value(from);

    Made made;
    made.path = source;
    auto answer = [&](const char *note, const QString &detail = QString()){
        made.note = QString::fromLatin1(note);
        made.detail = detail;
        return made;
    };

    QFile file(from + QStringLiteral("/manifest.json"));
    if(!file.open(QIODevice::ReadOnly)) return answer(QT_TRANSLATE_NOOP("ExtensionCopy", "its manifest could not be read"));
    const QByteArray bytes = file.readAll();
    const QJsonObject manifest = QJsonDocument::fromJson(bytes.startsWith("\xEF\xBB\xBF") ? bytes.mid(3) : bytes).object();
    if(manifest.isEmpty()) return answer(QT_TRANSLATE_NOOP("ExtensionCopy", "its manifest could not be read"));
    if(id.isEmpty() || id.contains(QLatin1Char('/')) || id.contains(QLatin1Char('\\')) || id.contains(QLatin1Char('.')))
        return answer(QT_TRANSLATE_NOOP("ExtensionCopy", "it has no usable id"));
    const QString fromKey = Key(from) + QLatin1Char('/'), rootKey = Key(root) + QLatin1Char('/');
    if(fromKey.startsWith(rootKey) || rootKey.startsWith(fromKey))
        return answer(QT_TRANSLATE_NOOP("ExtensionCopy", "it is in the folder Vanilla keeps its copies in, or that folder is in it"));

    const int stamp = Stamp(from, bytes, key, locale);
    const Rewritten rewritten = Rewrite(manifest, stamp);
    if(!rewritten.refusal.isEmpty()) return answer(rewritten.refusal.toLatin1().constData());
    static const QRegularExpression shape(QStringLiteral("\\A[0-9a-f]{64}\\z"));
    const bool hasKey = shape.match(QString::fromLatin1(key)).hasMatch();
    const bool givesKey = hasKey && !rewritten.keyless;
    auto keyed = [&](const QString &shim){
        if(!givesKey) return shim.toUtf8();
        return QString(shim).replace(QLatin1String(ExtensionHostWire::KEY_PLACE), QString::fromLatin1(key)).toUtf8();
    };

    const QString home = QDir(root).absolutePath() + QLatin1Char('/') + id;
    const QString to = home + QLatin1Char('/') + QString::number(stamp);

    if(!Completed(to, stamp)){
        const QString making = home + QLatin1Char('/') + MAKING + QStringLiteral("%1-%2").arg(stamp).arg(QCoreApplication::applicationPid());
        QDir(making).removeRecursively();
        QDir(to).removeRecursively();
        QString why, detail;
        bool ok = QDir().mkpath(making) && CopyTree(from, making, &why, &detail);
        const QByteArray messages = MessagesLine(Messages(from, manifest, locale), locale);
        if(ok && !rewritten.contentShim.isEmpty())
            ok = Write(making + QLatin1Char('/') + rewritten.contentShim, messages + Cdp::ContentShim().toUtf8() + ";\n");
        if(ok && !rewritten.carrier.isEmpty())
            ok = Write(making + QLatin1Char('/') + rewritten.carrier, QByteArray(CARRIER_MARK) + messages + Cdp::ContentShim().toUtf8() + ";\n");
        const QByteArray note = hasKey && rewritten.keyless ? QByteArray(KEYLESS_NOTE) : QByteArray();
        if(ok && !rewritten.workerWrapper.isEmpty())
            ok = Write(making + QLatin1Char('/') + rewritten.workerShim, note + messages + keyed(Cdp::WorkerShim()) + ";\n")
              && Write(making + QLatin1Char('/') + rewritten.workerWrapper, rewritten.workerWrapperText.toUtf8());
        if(ok) ok = Write(making + QLatin1Char('/') + rewritten.pageShim, messages + keyed(Cdp::PageShim()) + ";\n")
                 && Write(making + QLatin1Char('/') + rewritten.pageShimOpen, messages + Cdp::PageShim().toUtf8() + ";\n");
        if(ok){
            const QDir base(making);
            QDirIterator pages(making, QStringList() << QStringLiteral("*.html") << QStringLiteral("*.htm"),
                               QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
            while(ok && pages.hasNext()){
                const QString path = pages.next(), relative = base.relativeFilePath(path);
                if(AnyGlobbed(rewritten.sandboxed, relative)) continue;
                if(QFileInfo(path).size() > PAGE_LIMIT) continue;
                QFile page(path);
                if(!page.open(QIODevice::ReadOnly)){ ok = false; break; }
                QByteArray html = page.readAll();
                page.close();
                const int place = PlaceOfPageShim(html);
                if(place < 0) continue;
                html.insert(place, PageShimLine(Readable(rewritten.webAccessible, relative)));
                ok = Write(path, html);
            }
        }
        if(ok && givesKey)
            ok = Write(making + QLatin1Char('/') + rewritten.relayPage, QByteArray(RELAY_HTML))
              && Write(making + QLatin1Char('/') + rewritten.relayScript, keyed(Cdp::RelayScript()) + ";\n");
        if(ok) ok = Write(making + QLatin1Char('/') + WAKE_PAGE, QByteArray(WAKE_HTML))
                 && Write(making + QLatin1Char('/') + WAKE_SCRIPT, Cdp::WakeScript().toUtf8());
        if(ok) ok = Write(making + QStringLiteral("/manifest.json"), QJsonDocument(rewritten.manifest).toJson(QJsonDocument::Indented))
                 && Write(making + QLatin1Char('/') + COMPLETE, CompleteMark(stamp));
        if(ok) ok = QDir().rename(making, to) || Completed(to, stamp);
        QDir(making).removeRecursively();
        if(!ok) return why.isEmpty() ? answer(QT_TRANSLATE_NOOP("ExtensionCopy", "the copy could not be written"))
                                     : answer(why.toLatin1().constData(), detail);
    }

    foreach(const QFileInfo &other, QDir(home).entryInfoList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)){
        if(other.absoluteFilePath() == to || other.isSymLink() || other.isJunction()) continue;
        if(other.fileName().startsWith(MAKING) && other.lastModified().secsTo(QDateTime::currentDateTime()) < 3600) continue;
        QFile::remove(other.absoluteFilePath() + QLatin1Char('/') + COMPLETE);
        QDir(other.absoluteFilePath()).removeRecursively();
    }

    made.path = to;
    made.keyless = !givesKey;
    made.withheld = hasKey && rewritten.keyless;
    g_Sources.insert(Key(to), source);
    g_Made.insert(from, made);
    return made;
}

bool Stale(bool shimsOn, bool known, const QString &registered, const QString &current){
    if(!known) return shimsOn;
#ifdef Q_OS_WIN
    return QDir::cleanPath(registered).compare(QDir::cleanPath(current), Qt::CaseInsensitive) != 0;
#else
    return QDir::cleanPath(registered) != QDir::cleanPath(current);
#endif
}

namespace {

bool SamePath(const QString &one, const QString &other){
#ifdef Q_OS_WIN
    return QDir::cleanPath(one).compare(QDir::cleanPath(other), Qt::CaseInsensitive) == 0;
#else
    return QDir::cleanPath(one) == QDir::cleanPath(other);
#endif
}

}

bool Held(bool shimsOn, const QString &registered, const QString &pinned, const QString &current){
    return shimsOn && !registered.isEmpty() && SamePath(registered, pinned) && !SamePath(current, pinned);
}

bool Behind(const QString &registered, const QString &pinned, const QString &loaded, const QString &copy){
    if(registered.isEmpty() || copy.isEmpty() || !SamePath(registered, pinned)) return false;
    return loaded.isEmpty() || !SamePath(loaded, copy);
}

bool KeylessOfLoaded(bool was, const QString &loaded, const QString &copy, bool copyKeyless){
    if(!loaded.isEmpty() && !copy.isEmpty() && SamePath(loaded, copy)) return copyKeyless;
    return was || copyKeyless;
}

QString PinnedPath(const QString &root, const QString &id){
    return QDir(root).absolutePath() + QLatin1Char('/') + id + QLatin1Char('/') + PINNED;
}

#ifdef Q_OS_WIN
namespace {

bool MakeJunction(const QString &link, const QString &target){
    const std::wstring path = QDir::toNativeSeparators(link).toStdWString();
    const std::wstring print = QDir::toNativeSeparators(target).toStdWString();
    const std::wstring substitute = L"\\??\\" + print;
    if(!CreateDirectoryW(path.c_str(), nullptr)) return false;
    bool ok = false;
    const HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if(handle != INVALID_HANDLE_VALUE){
        const DWORD substituteBytes = DWORD(substitute.size() * sizeof(wchar_t));
        const DWORD printBytes = DWORD(print.size() * sizeof(wchar_t));
        const DWORD data = 8 + substituteBytes + sizeof(wchar_t) + printBytes + sizeof(wchar_t);
        QByteArray buffer(int(8 + data), '\0');
        char *p = buffer.data();
        const DWORD tag = IO_REPARSE_TAG_MOUNT_POINT;
        const WORD words[] = { WORD(data), 0, 0, WORD(substituteBytes),
                               WORD(substituteBytes + sizeof(wchar_t)), WORD(printBytes) };
        memcpy(p, &tag, 4);
        memcpy(p + 4, words, sizeof words);
        memcpy(p + 16, substitute.data(), substituteBytes);
        memcpy(p + 16 + substituteBytes + sizeof(wchar_t), print.data(), printBytes);
        DWORD returned = 0;
        ok = data <= 0xFFFF
            && DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, p, DWORD(buffer.size()), nullptr, 0, &returned, nullptr);
        CloseHandle(handle);
    }
    if(!ok) RemoveDirectoryW(path.c_str());
    return ok;
}

}
#endif

QString Pin(const QString &root, const QString &id, const QString &copy){
#ifdef Q_OS_WIN
    const QString home = QDir(root).absolutePath() + QLatin1Char('/') + id;
    const QFileInfo made(copy);
    bool number = false;
    made.fileName().toInt(&number);
    if(id.isEmpty() || !number || Key(made.absolutePath()) != Key(home) || !made.isDir()
       || made.isJunction() || made.isSymLink())
        return copy;
    const QString link = PinnedPath(root, id);
    const QFileInfo there(link);
    const QString target = QDir::cleanPath(made.absoluteFilePath());
    if(target.startsWith(QStringLiteral("//"))) return copy;
    if(there.isJunction()){
        if(QDir::cleanPath(there.junctionTarget()).compare(target, Qt::CaseInsensitive) == 0) return link;
        if(!RemoveDirectoryW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(link).utf16()))) return copy;
    } else if(there.exists() || there.isSymLink()){
        return copy;
    }
    return MakeJunction(link, target) ? link : copy;
#else
    Q_UNUSED(root);
    Q_UNUSED(id);
    return copy;
#endif
}

QString UiLocale(){
    return QLocale(QLocale::system().uiLanguages().value(0)).name();
}

QString SourceOf(const QString &path){
    QMutexLocker lock(&g_Mutex);
    const QString key = Key(path);
    return g_Sources.contains(key) ? g_Sources.value(key) : path;
}

void ForgetForTesting(){
    QMutexLocker lock(&g_Mutex);
    g_Sources.clear();
    g_Made.clear();
}

}
