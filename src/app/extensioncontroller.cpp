#include "switch.hpp"

#include "extensioncontroller.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>

#include <algorithm>

#include "application.hpp"
#include "networkcontroller.hpp"
#include "extensioncopy.hpp"
#include "extensionhost.hpp"

namespace {

    QList<QPointer<ExtensionController>> g_Controllers;

    const qint64 MAX_MANIFEST_SIZE = 1024 * 1024;
    const qint64 MENUS_FILE_LIMIT = 64 * 1024 * 1024;
    const qint64 RULES_FILE_LIMIT = 256 * 1024 * 1024;

    bool PutAside(const QString &path){
        const QString bad = path + QStringLiteral(".bad"), fresh = bad + QStringLiteral(".new");
        QFile::remove(fresh);
        if(!QFile::copy(path, fresh)) return false;
        QFile::remove(bad);
        QFile::rename(fresh, bad);
        return true;
    }

    const QString PATHS_KEY    = QStringLiteral("network/@Extensions");
    const QString DISABLED_KEY = QStringLiteral("network/@DisabledExtensions");
    const QString PINNED_KEY   = QStringLiteral("network/@PinnedExtensions");

    QStringList Normalized(const QStringList &paths){
        QStringList result;
        foreach(const QString &path, paths){
            const QString trimmed = path.trimmed();
            if(trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) continue;
            const QString clean = ExtensionManifest::NormalizePath(path);
            bool duplicate = false;
            foreach(const QString &old, result){
                if(ExtensionManifest::PathKey(old) == ExtensionManifest::PathKey(clean))
                    duplicate = true;
            }
            if(!duplicate) result.append(clean);
        }
        return result;
    }

    QString IdFor(const QByteArray &bytes){
        QString result;
        const QByteArray hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).first(16);
        for(int i = 0; i < hash.size(); i++){
            const unsigned char c = static_cast<unsigned char>(hash.at(i));
            result += QChar('a' + (c >> 4));
            result += QChar('a' + (c & 15));
        }
        return result;
    }

    bool ValidId(const QString &id){
        if(id.size() != 32) return false;
        foreach(const QChar ch, id){
            if(ch < QLatin1Char('a') || ch > QLatin1Char('p')) return false;
        }
        return true;
    }

    bool VersionFolder(const QString &name, QList<int> *order){
        static const QRegularExpression pattern(QStringLiteral("^(\\d+(?:\\.\\d+){0,3})_(\\d+)$"));
        const QRegularExpressionMatch match = pattern.match(name);
        if(!match.hasMatch()) return false;
        QList<int> parts;
        bool ok = true;
        foreach(const QString &part, match.captured(1).split(QLatin1Char('.'))){
            parts << part.toInt(&ok);
            if(!ok) return false;
        }
        while(parts.size() < 4) parts << 0;
        parts << match.captured(2).toInt(&ok);
        if(!ok) return false;
        *order = parts;
        return true;
    }

    QJsonObject JsonFile(const QString &path){
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly) || file.size() > MAX_MANIFEST_SIZE) return QJsonObject();
        return QJsonDocument::fromJson(file.readAll()).object();
    }

    void ChangeList(const QString &key, const QString &path, bool present){
        Settings &s = Application::GlobalSettings();
        QStringList list = s.value(key).toStringList();
        const QString clean = ExtensionManifest::NormalizePath(path);
        const QString keyPath = ExtensionManifest::PathKey(clean);
        for(qsizetype i = list.size(); i-- > 0;){
            const QString entry = list.at(i).trimmed();
            if(entry.isEmpty() || entry.startsWith(QLatin1Char('#'))) continue;
            if(ExtensionManifest::PathKey(entry) != keyPath) continue;
            if(present) return;
            list.removeAt(i);
        }
        if(present) list.append(clean);
        s.setValue(key, list);
        ExtensionController::ReloadAll();
    }
}

QString ExtensionManifest::NormalizePath(const QString &path){
    QFileInfo info(QDir::fromNativeSeparators(path.trimmed()));
    QString clean = info.canonicalFilePath();
    if(clean.isEmpty()) clean = info.absoluteFilePath();
    clean = QDir::cleanPath(clean);
#ifdef Q_OS_WIN
    if(clean.size() > 1 && clean.at(1) == QLatin1Char(':')) clean[0] = clean[0].toUpper();
#endif
    return clean;
}

QString ExtensionManifest::PathKey(const QString &path){
    const QString normalized = NormalizePath(path);
#ifdef Q_OS_WIN
    return normalized.toCaseFolded();
#else
    return normalized;
#endif
}

QString ExtensionManifest::Resource(const QString &root, const QString &relative){
    const QUrl url(relative);
    if(relative.isEmpty() || !url.isRelative() || relative.startsWith(QLatin1Char('/')) ||
       relative.contains(QLatin1Char('\\')) || !url.query().isEmpty() || !url.fragment().isEmpty())
        return QString();
    const QString path =
        NormalizePath(root + QLatin1Char('/') + QUrl::fromPercentEncoding(relative.toUtf8()));
    if(!PathKey(path).startsWith(PathKey(root) + QLatin1Char('/')) || !QFileInfo(path).isFile())
        return QString();
    return path;
}

QString ExtensionManifest::CurrentFolder(const QString &path){
    const QString clean = NormalizePath(path);
    if(QFileInfo::exists(clean + QStringLiteral("/manifest.json"))) return clean;
    const QFileInfo info(clean);
    QList<int> order;
    if(!VersionFolder(info.fileName(), &order) || !ValidId(QFileInfo(info.path()).fileName())) return clean;
    QString best;
    QList<int> bestOrder;
    foreach(const QFileInfo &sibling,
            QDir(info.path()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks)){
        QList<int> each;
        if(sibling.isJunction() || !VersionFolder(sibling.fileName(), &each) ||
           !QFileInfo(sibling.filePath() + QStringLiteral("/manifest.json")).isFile()) continue;
        if(best.isEmpty() ||
           std::lexicographical_compare(bestOrder.cbegin(), bestOrder.cend(), each.cbegin(), each.cend())){
            best = NormalizePath(sibling.filePath());
            bestOrder = each;
        }
    }
    return best.isEmpty() ? clean : best;
}

ExtensionManifest ExtensionManifest::Read(const QString &input){
    ExtensionManifest m;
    m.path = NormalizePath(input);
    m.folder = CurrentFolder(m.path);
    m.name = QFileInfo(m.path).fileName();

    QFile file(m.folder + QStringLiteral("/manifest.json"));
    if(!file.exists()){
        m.error = ExtensionController::tr("There is no manifest.json in this folder. "
                                          "The folder may have been removed, or replaced by an update.");
        return m;
    }
    if(file.size() > MAX_MANIFEST_SIZE){
        m.error = ExtensionController::tr("manifest.json is too large (maximum 1 MiB).");
        return m;
    }
    if(!file.open(QIODevice::ReadOnly)){
        m.error = ExtensionController::tr("Cannot read manifest.json.");
        return m;
    }
    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if(parse.error != QJsonParseError::NoError || !doc.isObject()){
        m.error = ExtensionController::tr("Invalid manifest.json.");
        return m;
    }
    const QJsonObject json = doc.object();
    if(json.value(QStringLiteral("manifest_version")).toInt() != 3){
        m.error = ExtensionController::tr("Only Manifest V3 extensions are supported.");
        return m;
    }

    QJsonObject messages;
    bool read = false;
    auto localized = [&m, &json, &messages, &read](const QString &text){
        if(!text.contains(QStringLiteral("__MSG_"))) return text;
        if(!read){
            read = true;
            messages = ExtensionMessages::Messages(m.folder, json, ExtensionMessages::UiLocale());
        }
        static const QRegularExpression key(QStringLiteral("__MSG_([A-Za-z0-9_@]+?)__"));
        static const QRegularExpression holder(QStringLiteral("\\$([A-Za-z0-9_@]+)\\$"));
        QString out;
        int from = 0;
        QRegularExpressionMatchIterator it = key.globalMatch(text);
        while(it.hasNext()){
            const QRegularExpressionMatch found = it.next();
            const QJsonObject entry = messages.value(found.captured(1).toLower()).toObject();
            QString translated = entry.value(QStringLiteral("message")).toString();
            const QJsonObject holders = entry.value(QStringLiteral("placeholders")).toObject();
            if(!holders.isEmpty()){
                QString filled;
                int at = 0;
                QRegularExpressionMatchIterator h = holder.globalMatch(translated);
                while(h.hasNext()){
                    const QRegularExpressionMatch one = h.next();
                    const QJsonValue content = holders.value(one.captured(1).toLower()).toObject().value(QStringLiteral("content"));
                    filled += translated.mid(at, one.capturedStart() - at);
                    filled += content.isString() ? content.toString() : one.captured(0);
                    at = one.capturedEnd();
                }
                translated = filled + translated.mid(at);
            }
            out += text.mid(from, found.capturedStart() - from);
            out += translated.isEmpty() ? found.captured(0) : translated;
            from = found.capturedEnd();
        }
        return out + text.mid(from);
    };
    m.name = localized(json.value(QStringLiteral("name")).toString(m.name));

    const QString key = json.value(QStringLiteral("key")).toString();
    if(!key.isEmpty()){
        const auto decoded =
            QByteArray::fromBase64Encoding(key.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        if(!decoded || decoded.decoded.isEmpty()){
            m.error = ExtensionController::tr("Invalid extension public key.");
            return m;
        }
        m.id = IdFor(decoded.decoded);
    } else {
#ifdef Q_OS_WIN
        const QString native = QDir::toNativeSeparators(m.folder);
        m.id = IdFor(QByteArray(reinterpret_cast<const char*>(native.utf16()), native.size() * 2));
#else
        m.id = IdFor(QFile::encodeName(m.folder));
#endif
    }

    const QJsonObject action = json.value(QStringLiteral("action")).toObject();
    auto resourceUrl = [&m](const QString &relative){
        if(Resource(m.folder, relative).isEmpty()) return QUrl();
        return QUrl(QStringLiteral("chrome-extension://") + m.id + QLatin1Char('/') + relative);
    };
    m.popup = resourceUrl(action.value(QStringLiteral("default_popup")).toString());
    m.title = localized(action.value(QStringLiteral("default_title")).toString());
    m.options = resourceUrl(json.value(QStringLiteral("options_ui")).toObject()
                            .value(QStringLiteral("page")).toString());
    if(m.options.isEmpty())
        m.options = resourceUrl(json.value(QStringLiteral("options_page")).toString());
    const QUrl homepage(json.value(QStringLiteral("homepage_url")).toString(), QUrl::StrictMode);
    const QUrl update(json.value(QStringLiteral("update_url")).toString(), QUrl::StrictMode);
    if(homepage.isValid() && !homepage.host().isEmpty() &&
       (homepage.scheme() == QStringLiteral("https") || homepage.scheme() == QStringLiteral("http")))
        m.homepage = homepage;
    else if(!key.isEmpty() && update.scheme() == QStringLiteral("https")){
        if(update.host() == QStringLiteral("clients2.google.com") &&
           update.path() == QStringLiteral("/service/update2/crx"))
            m.homepage = QUrl(QStringLiteral("https://chromewebstore.google.com/detail/") + m.id);
        else if(update.host() == QStringLiteral("edge.microsoft.com") &&
                update.path() == QStringLiteral("/extensionwebstorebase/v1/crx"))
            m.homepage = QUrl(QStringLiteral("https://microsoftedge.microsoft.com/addons/detail/") + m.id);
    }
    m.sidePanelPath = json.value(QStringLiteral("side_panel")).toObject().value(QStringLiteral("default_path")).toString();
    if(resourceUrl(m.sidePanelPath).isEmpty()) m.sidePanelPath.clear();
    foreach(const QJsonValue &permission, json.value(QStringLiteral("permissions")).toArray()){
        if(permission.toString() == QStringLiteral("tabs")) m.tabsPermission = true;
        if(permission.isString()) m.permissions << permission.toString();
    }
    foreach(const QJsonValue &pattern, json.value(QStringLiteral("host_permissions")).toArray())
        if(pattern.isString()) m.hostPermissions << pattern.toString();
    m.version = json.value(QStringLiteral("version")).toString();
#if defined(Q_OS_WIN)
    static const QString platform = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    static const QString platform = QStringLiteral("mac");
#else
    static const QString platform = QStringLiteral("linux");
#endif
    m.commands = ExtensionUi::CommandsOf(json.value(QStringLiteral("commands")).toObject(), platform,
                                         json.value(QStringLiteral("action")).isObject());
    foreach(const QJsonValue &value, json.value(QStringLiteral("declarative_net_request")).toObject()
                                         .value(QStringLiteral("rule_resources")).toArray()){
        const QJsonObject resource = value.toObject();
        RuleResource one;
        one.id = resource.value(QStringLiteral("id")).toString();
        one.enabled = resource.value(QStringLiteral("enabled")).toBool(true);
        QString path = resource.value(QStringLiteral("path")).toString();
        if(path.startsWith(QLatin1Char('/')) && !path.startsWith(QStringLiteral("//"))) path = path.mid(1);
        one.file = Resource(m.folder, path);
        if(!one.id.isEmpty()) m.ruleResources << one;
    }

    QJsonValue icons = action.value(QStringLiteral("default_icon"));
    if(icons.isUndefined()) icons = json.value(QStringLiteral("icons"));
    QString icon = icons.toString();
    if(icons.isObject()){
        int best = 0;
        const QJsonObject sizes = icons.toObject();
        for(QJsonObject::const_iterator i = sizes.begin(); i != sizes.end(); ++i){
            const int size = i.key().toInt();
            if(size > best && size <= 256){
                best = size;
                icon = i.value().toString();
            }
        }
    }
    const QString iconPath = Resource(m.folder, icon);
    if(!iconPath.isEmpty() && QFileInfo(iconPath).size() <= 4 * MAX_MANIFEST_SIZE){
        QImageReader reader(iconPath);
        const QSize size = reader.size();
        if(size.isValid() && size.width() <= 2048 && size.height() <= 2048){
            reader.setScaledSize(QSize(32, 32));
            m.icon = QIcon(QPixmap::fromImage(reader.read()));
        }
    }
    return m;
}

QList<Dnr::Held::Ruleset> ExtensionManifest::Rulesets() const {
    QList<Dnr::Held::Ruleset> rulesets;
    foreach(const RuleResource &resource, ruleResources){
        Dnr::Held::Ruleset ruleset;
        ruleset.id = resource.id;
        ruleset.enabled = resource.enabled;
        rulesets << ruleset;
    }
    return rulesets;
}

QString ExtensionController::DefaultPickDirectory(){
    QString root;
#if defined(Q_OS_WIN)
    root = qEnvironmentVariable("LOCALAPPDATA");
    if(!root.isEmpty())
        root = QDir(root).filePath(QStringLiteral("Google/Chrome/User Data"));
#elif defined(Q_OS_MACOS)
    root = QDir::home().filePath(QStringLiteral("Library/Application Support/Google/Chrome"));
#else
    root = QDir::home().filePath(QStringLiteral(".config/google-chrome"));
#endif
    return PickDirectoryFrom(root, QDir::homePath());
}

QString ExtensionController::PickDirectoryFrom(const QString &root, const QString &home){
    if(root.isEmpty()) return home;
    const QString extensions = QDir(root).filePath(QStringLiteral("Default/Extensions"));
    if(QFileInfo(extensions).isDir()) return extensions;
    if(QFileInfo(root).isDir()) return root;
    return home;
}

ExtensionController::ExtensionController(QObject *parent, const QString &journal, int timeoutMs)
    : QObject(parent)
    , m_Journal(journal)
    , m_TimeoutMs(qMax(1, timeoutMs))
{
    setObjectName(QStringLiteral("ExtensionController"));
    g_Controllers.append(this);
    m_Fatal = !ReadOwned();
    ReadSettings();
}

ExtensionController::~ExtensionController(){
    if(m_MenusDirty) SaveMenusNow();
    if(m_RulesDirty) SaveRulesNow();
    const bool heldRules = !m_RulesFile.isEmpty() && !m_Rules.Extensions().isEmpty();
    g_Controllers.removeAll(this);
    if(heldRules && qApp) QTimer::singleShot(0, qApp, [](){ ExtensionNetRules::Reload(); });
}

ExtensionController *ExtensionController::Of(QObject *profile){
    if(!profile) return nullptr;
    return profile->findChild<ExtensionController*>(QStringLiteral("ExtensionController"),
                                                    Qt::FindDirectChildrenOnly);
}

bool ExtensionController::ReadOwned(){
    if(m_Journal.isEmpty() || !QFileInfo::exists(m_Journal)) return true;

    QFile file(m_Journal);
    QJsonParseError parse;
    if(!file.open(QIODevice::ReadOnly) || file.size() > MAX_MANIFEST_SIZE){
        m_Error = tr("Cannot read the extension ownership journal.");
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    const QJsonObject obj = doc.object();
    if(parse.error != QJsonParseError::NoError ||
       obj.value(QStringLiteral("version")).toInt() != 1 ||
       !obj.value(QStringLiteral("owned")).isObject()){
        m_Error = tr("The extension ownership journal is invalid. No extensions were changed.");
        return false;
    }
    const QJsonObject owned = obj.value(QStringLiteral("owned")).toObject();
    QSet<QString> ids;
    for(QJsonObject::const_iterator i = owned.begin(); i != owned.end(); ++i){
        const QString id = i.value().toString();
        if(!QDir::isAbsolutePath(i.key()) || !ValidId(id) || ids.contains(id) ||
           ExtensionManifest::PathKey(i.key()) != i.key()){
            m_Error = tr("The extension ownership journal is invalid. No extensions were changed.");
            return false;
        }
        m_Owned.insert(i.key(), id);
        ids.insert(id);
    }
    return true;
}

bool ExtensionController::SaveOwned(){
    if(m_Journal.isEmpty()) return true;

    QJsonObject owned;
    for(QMap<QString, QString>::const_iterator i = m_Owned.begin(); i != m_Owned.end(); ++i)
        owned.insert(i.key(), i.value());
    const QByteArray data =
        QJsonDocument(QJsonObject{{QStringLiteral("version"), 1},
                                  {QStringLiteral("owned"), owned}}).toJson();
    QSaveFile file(m_Journal);
    if(!QDir().mkpath(QFileInfo(m_Journal).absolutePath()) ||
       !file.open(QIODevice::WriteOnly) ||
       file.write(data) != data.size() || !file.commit()){
        m_Error = tr("Cannot save the extension ownership journal. Management is paused.");
        m_Fatal = true;
        return false;
    }
    return true;
}

void ExtensionController::ReadSettings(){
    const Settings &s = Application::GlobalSettings();
    Reconcile(s.value(QStringLiteral("network/@Extensions"), QStringList()).toStringList(),
              s.value(DISABLED_KEY).toStringList(),
              s.value(PINNED_KEY).toStringList());
}

void ExtensionController::ReloadAll(){
    foreach(const QPointer<ExtensionController> &c, g_Controllers){
        if(!c) continue;
        QTimer::singleShot(0, c, [c](){ if(c) c->ReadSettings();});
    }
    if(qApp) QTimer::singleShot(0, qApp, [](){ ExtensionNetRules::Reload(); });
    else ExtensionNetRules::Reload();
}

namespace {
    QList<ExtensionManifest> EnabledManifests(){
        const Settings &s = Application::GlobalSettings();
        QSet<QString> disabled;
        foreach(const QString &path, Normalized(s.value(DISABLED_KEY).toStringList()))
            disabled.insert(ExtensionManifest::PathKey(path));

        QList<ExtensionManifest> result;
        foreach(const QString &path, Normalized(s.value(PATHS_KEY).toStringList())){
            if(disabled.contains(ExtensionManifest::PathKey(path))) continue;
            const ExtensionManifest manifest = ExtensionManifest::Read(path);
            if(!manifest.error.isEmpty() || manifest.id.isEmpty()) continue;
            result << manifest;
        }
        return result;
    }
}

QSet<QString> ExtensionController::EnabledIds(){
    QSet<QString> ids;
    foreach(const ExtensionManifest &manifest, EnabledManifests()) ids.insert(manifest.id);
    return ids;
}

ExtensionHostWire::Sight ExtensionController::SightOf(const QString &id) const {
    foreach(const ExtensionManifest &manifest, m_Desired){
        if(manifest.id != id) continue;
        ExtensionHostWire::Sight sight = ExtensionHostWire::Sight::Of(manifest.tabsPermission, manifest.id, manifest.hostPermissions);
        sight.history = manifest.permissions.contains(QStringLiteral("history"));
        return sight;
    }
    return ExtensionHostWire::Sight();
}

bool ExtensionController::HasPermission(const QString &id, const QString &name) const {
    foreach(const ExtensionManifest &manifest, m_Desired)
        if(manifest.id == id) return manifest.permissions.contains(name);
    return false;
}

ExtensionRow ExtensionController::RowOf(const QString &id) const {
    foreach(const ExtensionRow &row, Rows())
        if(row.manifest.id == id) return row;
    return ExtensionRow();
}

ExtensionUi::Action &ExtensionController::ActionFor(const QString &id){
    return m_Actions[id];
}

ExtensionUi::SidePanel &ExtensionController::SidePanelFor(const QString &id){
    return m_SidePanels[id];
}

QUrl ExtensionController::ResourceUrlOf(const QString &id, const QString &path) const {
    const ExtensionRow row = RowOf(id);
    QString relative = path;
    while(relative.startsWith(QLatin1Char('/'))) relative.remove(0, 1);
    int cut = relative.size();
    for(const QChar c : { QLatin1Char('?'), QLatin1Char('#') }){ const int at = relative.indexOf(c); if(at >= 0) cut = qMin(cut, at); }
    if(row.manifest.id != id || ExtensionManifest::Resource(row.manifest.folder, relative.left(cut)).isEmpty()) return QUrl();
    return QUrl(QStringLiteral("chrome-extension://") + id + QLatin1Char('/') + relative);
}

QJsonObject ExtensionController::SidePanelCall(const QString &id, const QString &api, const QJsonArray &args){
    ExtensionUi::SidePanel &panel = m_SidePanels[id];
    const ExtensionUi::SidePanel::Resolve resolve = [this, id](const QString &path){ return ResourceUrlOf(id, path); };
    if(api == QStringLiteral("sidePanel.setOptions")) return panel.SetOptions(args, resolve);
    if(api == QStringLiteral("sidePanel.getOptions")) return panel.GetOptions(args, RowOf(id).manifest.sidePanelPath);
    if(api == QStringLiteral("sidePanel.setPanelBehavior")) return panel.SetBehavior(args);
    if(api == QStringLiteral("sidePanel.getPanelBehavior")) return panel.GetBehavior();
    return ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(api));
}

QUrl ExtensionController::SidePanelUrl(const QString &id){
    return m_SidePanels[id].Url(RowOf(id).manifest.sidePanelPath, [this, id](const QString &path){ return ResourceUrlOf(id, path); });
}

QString ExtensionController::SidePanelPath(const QString &id){
    return m_SidePanels[id].Path(RowOf(id).manifest.sidePanelPath);
}

QUrl ExtensionController::SidePanelTabUrl(const QString &id, qint64 tab){
    return m_SidePanels[id].TabUrl(tab, [this, id](const QString &path){ return ResourceUrlOf(id, path); });
}

ExtensionUi::Action *ExtensionController::ActionOf(const QString &id){
    QHash<QString, ExtensionUi::Action>::iterator i = m_Actions.find(id);
    return i == m_Actions.end() ? nullptr : &i.value();
}

void ExtensionController::ActionSet(const QString &id){
    emit ActionChanged(id);
}

void ExtensionController::ClickAction(const QString &id, qint64 tab){
    emit WorkerEvent(id, QStringLiteral("action.onClicked"), QJsonArray(), tab);
}

const ExtensionUi::Menus *ExtensionController::MenusOf(const QString &id) const {
    QHash<QString, ExtensionUi::Menus>::const_iterator i = m_Menus.constFind(id);
    return i == m_Menus.constEnd() ? nullptr : &i.value();
}

QJsonObject ExtensionController::MenuCall(const QString &id, const QString &api, const QJsonArray &args){
    ExtensionUi::Menus &menus = m_Menus[id];
    const QJsonValue first = args.size() > 0 ? args.at(0) : QJsonValue();
    const QJsonValue second = args.size() > 1 ? args.at(1) : QJsonValue();
    QJsonObject reply;
    if(api == QStringLiteral("contextMenus.create")) reply = menus.Create(first.toObject());
    else if(api == QStringLiteral("contextMenus.update")) reply = menus.Update(ExtensionUi::MenuIdOf(first), second.toObject());
    else if(api == QStringLiteral("contextMenus.remove")) reply = menus.Remove(ExtensionUi::MenuIdOf(first));
    else if(api == QStringLiteral("contextMenus.removeAll")) reply = menus.RemoveAll();
    else return ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(api));
    if(reply.value(QStringLiteral("ok")).toBool()) SaveMenusSoon();
    return reply;
}

void ExtensionController::ClickMenu(const QString &id, const QString &itemId, const ExtensionUi::MenuContext &context, qint64 tab){
    if(!m_Menus.contains(id) || !HasPermission(id, QStringLiteral("contextMenus"))) return;
    ExtensionUi::Menus &menus = m_Menus[id];
    const ExtensionUi::Menus::Clicked clicked = menus.Click(itemId);
    if(!clicked.found) return;
    if(clicked.changed) SaveMenusSoon();
    ExtensionUi::MenuItem item;
    foreach(const ExtensionUi::MenuItem &one, menus.Items())
        if(one.id == itemId) item = one;
    ExtensionHost::Invoked(this, id, tab);
    emit WorkerEvent(id, m_MenusMirrored ? QStringLiteral("vanilla.actionMenuClicked") : QStringLiteral("contextMenus.onClicked"),
                     QJsonArray() << ExtensionUi::ClickInfo(item, clicked, context), tab);
}

QJsonObject ExtensionController::MirrorCall(const QString &id, const QJsonArray &args){
    static const QStringList ops = QStringList() << QStringLiteral("create") << QStringLiteral("update")
                                                 << QStringLiteral("remove") << QStringLiteral("removeAll");
    if(!m_MenusMirrored) return ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(QStringLiteral("vanilla.menuMirror")));
    if(args.size() != 1 || !args.at(0).isArray() || args.at(0).toArray().size() > 256)
        return ExtensionHostWire::Refused(QStringLiteral("vanilla.menuMirror takes a list of calls"));
    foreach(const QJsonValue &one, args.at(0).toArray()){
        const QJsonArray call = one.toArray();
        if(call.size() != 2 || !ops.contains(call.at(0).toString()) || !call.at(1).isArray()) continue;
        const QString api = QStringLiteral("contextMenus.") + call.at(0).toString();
        const QJsonArray callArgs = call.at(1).toArray();
        if(!MenuCall(id, api, callArgs).value(QStringLiteral("ok")).toBool() && api == QStringLiteral("contextMenus.create")){
            const QString made = ExtensionUi::MenuIdOf(callArgs.at(0).toObject().value(QStringLiteral("id")));
            const ExtensionUi::Menus *menus = MenusOf(id);
            bool had = false;
            if(menus) foreach(const ExtensionUi::MenuItem &item, menus->Items()) if(item.id == made) had = true;
            if(had && MenuCall(id, QStringLiteral("contextMenus.remove"), QJsonArray() << made).value(QStringLiteral("ok")).toBool())
                MenuCall(id, api, callArgs);
        }
    }
    return ExtensionHostWire::Done();
}

QJsonObject ExtensionController::RulesCall(const QString &id, const QString &api, const QJsonArray &args){
    ExtensionManifest manifest;
    foreach(const ExtensionManifest &m, m_Desired) if(m.id == id && m.error.isEmpty()){ manifest = m; break; }
    const bool permitted = manifest.permissions.contains(QStringLiteral("declarativeNetRequest"))
        || manifest.permissions.contains(QStringLiteral("declarativeNetRequestWithHostAccess"));
    const bool getting = api == QStringLiteral("declarativeNetRequest.getEnabledRulesets");
    const bool updating = api == QStringLiteral("declarativeNetRequest.updateEnabledRulesets");
    static const QHash<QString, QPair<Dnr::Held::Kind, bool>> held = {
        { QStringLiteral("declarativeNetRequest.getDynamicRules"), qMakePair(Dnr::Held::Dynamic, false) },
        { QStringLiteral("declarativeNetRequest.updateDynamicRules"), qMakePair(Dnr::Held::Dynamic, true) },
        { QStringLiteral("declarativeNetRequest.getSessionRules"), qMakePair(Dnr::Held::Session, false) },
        { QStringLiteral("declarativeNetRequest.updateSessionRules"), qMakePair(Dnr::Held::Session, true) } };
    if(m_RulesFile.isEmpty() || manifest.id.isEmpty() || !permitted || (!getting && !updating && !held.contains(api)))
        return ExtensionHostWire::Refused(ExtensionHostWire::NotAvailable(api));

    if(m_Rules.Settle(id, manifest.version)) SaveRulesSoon();
    const QJsonValue first = args.isEmpty() ? QJsonValue() : args.at(0);
    if(held.contains(api)){
        const Dnr::Held::Kind kind = held.value(api).first;
        if(!held.value(api).second){
            QJsonObject reply;
            reply[QStringLiteral("ok")] = true;
            reply[QStringLiteral("value")] = m_Rules.Get(id, kind, first, manifest.version);
            return reply;
        }
        const QString error = m_Rules.Update(id, kind, first, manifest.version);
        if(!error.isEmpty()) return ExtensionHostWire::Refused(error);
        if(kind == Dnr::Held::Dynamic) SaveRulesSoon();
        ExtensionNetRules::Reload();
        return ExtensionHostWire::Done();
    }
    const QList<Dnr::Held::Ruleset> rulesets = manifest.Rulesets();
    if(getting){
        QJsonObject reply;
        reply[QStringLiteral("ok")] = true;
        reply[QStringLiteral("value")] = QJsonArray::fromStringList(m_Rules.Enabled(id, rulesets, manifest.version));
        return reply;
    }
    const QString error = m_Rules.UpdateEnabled(id, first, rulesets, manifest.version);
    if(!error.isEmpty()) return ExtensionHostWire::Refused(error);
    SaveRulesSoon();
    ExtensionNetRules::Reload();
    return ExtensionHostWire::Done();
}

void ExtensionController::SetRulesFile(const QString &path){
    m_RulesFile = path;
    LoadRules();
    if(!m_Rules.Extensions().isEmpty()) ExtensionNetRules::Reload();
}

void ExtensionController::LoadRules(){
    m_Rules = Dnr::Held();
    m_RulesKept = false;
    if(m_RulesFile.isEmpty()) return;
    QFile file(m_RulesFile);
    if(!file.exists()) return;
    QJsonParseError parse;
    bool damaged = true;
    if(file.size() <= RULES_FILE_LIMIT && file.open(QIODevice::ReadOnly)){
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse);
        file.close();
        if(parse.error == QJsonParseError::NoError && document.isObject())
            m_Rules = Dnr::Held::FromJson(document.object(), &damaged);
    }
    if(damaged){
        if(PutAside(m_RulesFile)){
            qWarning() << "the extensions' rules could not all be read from" << m_RulesFile << "; it is kept as .bad";
            SaveRulesSoon();
        } else {
            m_RulesKept = true;
            qWarning() << "the extensions' rules could not be read from" << m_RulesFile << "; it is not written in this run";
        }
    }
    bool allRead = true;
    QSet<QString> registered;
    foreach(const ExtensionManifest &m, m_Desired){
        if(!m.error.isEmpty() || m.id.isEmpty()) allRead = false;
        else registered.insert(m.id);
    }
    if(allRead)
        foreach(const QString &id, m_Rules.Extensions())
            if(!registered.contains(id)){ m_Rules.Forget(id); SaveRulesSoon(); }
}

void ExtensionController::SaveRulesSoon(){
    if(m_RulesFile.isEmpty() || m_RulesKept || m_RulesDirty) return;
    m_RulesDirty = true;
    QTimer::singleShot(300, this, [this](){ SaveRulesNow(); });
}

void ExtensionController::SaveRulesNow(){
    m_RulesDirty = false;
    if(m_RulesFile.isEmpty() || m_RulesKept) return;
    QSaveFile file(m_RulesFile);
    if(!file.open(QIODevice::WriteOnly) ||
       file.write(QJsonDocument(m_Rules.ToJson()).toJson(QJsonDocument::Compact)) < 0 || !file.commit()){
        qWarning() << "the extensions' rules could not be written to" << m_RulesFile;
    }
}

void ExtensionController::SetInstalledFile(const QString &path){
    m_InstalledFile = path;
}

void ExtensionController::DecideInstalled(){
    if(m_InstalledFile.isEmpty()) return;
    bool migrating = true;
    QFile file(m_InstalledFile);
    if(file.exists()){
        QJsonParseError parse;
        bool damaged = true;
        if(file.size() <= MENUS_FILE_LIMIT && file.open(QIODevice::ReadOnly)){
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse);
            file.close();
            if(parse.error == QJsonParseError::NoError && document.isObject()){
                m_Installed = ExtensionUi::Installed::FromJson(document.object(), &damaged);
                migrating = damaged;
            }
        }
        if(damaged){
            if(PutAside(m_InstalledFile)){
                qWarning() << "what the extensions are owed could not all be read from" << m_InstalledFile << "; it is kept as .bad";
            } else {
                m_InstalledKept = true;
                qWarning() << "what the extensions are owed could not be read from" << m_InstalledFile << "; it is not written in this run";
            }
        }
    }
    m_InstalledRead = true;
    ApplyInstalled(true, migrating);
}

void ExtensionController::ApplyInstalled(bool starting, bool migrating){
    QList<QPair<QString, QString>> registered;
    QSet<QString> ids;
    bool allRead = true;
    foreach(const QString &path, m_Order){
        const ExtensionManifest m = m_Desired.value(path);
        if(!m.error.isEmpty() || m.id.isEmpty()){ allRead = false; continue; }
        ids.insert(m.id);
        if(starting || !m_Installed.Contains(m.id)) registered.append(qMakePair(m.id, m.version));
        if(starting && !m_Disabled.contains(path)) m_StartupOwed.insert(m.id);
    }
    bool changed = allRead && m_Installed.Forget(ids);
    bool menus = false;
    foreach(const QString &id, m_Installed.Decide(registered, migrating)){
        changed = true;
        if(m_Menus.remove(id)) menus = true;
    }
    if(menus) SaveMenusSoon();
    if(changed || migrating) SaveInstalledNow();
}

bool ExtensionController::SaveInstalledNow(){
    if(m_InstalledFile.isEmpty() || m_InstalledKept) return false;
    QSaveFile file(m_InstalledFile);
    if(!file.open(QIODevice::WriteOnly) ||
       file.write(QJsonDocument(m_Installed.ToJson()).toJson(QJsonDocument::Compact)) < 0 || !file.commit()){
        qWarning() << "what the extensions are owed could not be written to" << m_InstalledFile;
        return false;
    }
    return true;
}

QJsonObject ExtensionController::TakeInstalled(const QString &id, bool *written){
    if(written) *written = true;
    if(!WorkerIsOfThisRun(id)) return QJsonObject();
    const ExtensionUi::Installed::Owed owed = m_Installed.Take(id);
    if(owed.reason.isEmpty()) return QJsonObject();
    if(!SaveInstalledNow()){
        m_Installed.Restore(id, owed);
        if(written) *written = false;
        return QJsonObject();
    }
    QJsonObject details;
    details[QStringLiteral("reason")] = owed.reason;
    if(owed.reason == QStringLiteral("update")) details[QStringLiteral("previousVersion")] = owed.previousVersion;
    return details;
}

bool ExtensionController::HasCommand(const QString &key) const {
    return HandlesCommands() && !key.isEmpty() && CommandFor(key);
}

const ExtensionController::CommandOwner *ExtensionController::CommandFor(const QString &key) const {
    QHash<QString, QList<CommandOwner>>::const_iterator it = m_Commands.constFind(key);
    if(it == m_Commands.constEnd()) return nullptr;
    for(const CommandOwner &owner : it.value())
        if(!m_Errors.contains(owner.path)) return &owner;
    return nullptr;
}

bool ExtensionController::RunCommand(const QString &key, qint64 serial){
    if(!HasCommand(key)) return false;
    const CommandOwner owner = *CommandFor(key);
    ExtensionHost::Invoked(this, owner.id, serial);
    if(owner.name == QStringLiteral("_execute_action")) emit ActionCommand(owner.id, serial);
    else emit WorkerEvent(owner.id, QStringLiteral("commands.onCommand"), QJsonArray() << owner.name, serial);
    return true;
}

QJsonArray ExtensionController::CommandsOf(const QString &id, const std::function<bool(const QString &)> &taken) const {
    QJsonArray out;
    foreach(const QString &path, m_Order){
        const ExtensionManifest m = m_Desired.value(path);
        if(m.id != id || !m.error.isEmpty()) continue;
        foreach(const ExtensionUi::Command &command, m.commands){
            QJsonObject one;
            one[QStringLiteral("name")] = command.name;
            one[QStringLiteral("description")] = command.description;
            const CommandOwner *owner = command.key.isEmpty() ? nullptr : CommandFor(command.key);
            const bool mine = owner && owner->id == id && owner->name == command.name;
            one[QStringLiteral("shortcut")] = mine && !(taken && taken(command.key)) ? command.key : QString();
            out.append(one);
        }
        break;
    }
    return out;
}

bool ExtensionController::TakeStartup(const QString &id){
    if(!m_StartupOwed.contains(id) || !WorkerIsOfThisRun(id)) return false;
    m_StartupOwed.remove(id);
    return true;
}

void ExtensionController::SetMenusFile(const QString &path){
    m_MenusFile = path;
    LoadMenus();
}

void ExtensionController::LoadMenus(){
    m_Menus.clear();
    if(m_MenusFile.isEmpty()) return;
    QFile file(m_MenusFile);
    if(!file.exists()) return;
    QJsonParseError parse;
    if(!file.open(QIODevice::ReadOnly) || file.size() > MENUS_FILE_LIMIT){
        qWarning() << "the extensions' menus could not be read from" << m_MenusFile;
        return;
    }
    const QJsonObject all = QJsonDocument::fromJson(file.readAll(), &parse).object();
    if(parse.error != QJsonParseError::NoError){
        qWarning() << "the extensions' menus are not JSON in" << m_MenusFile << parse.errorString();
        return;
    }
    for(QJsonObject::const_iterator i = all.begin(); i != all.end(); ++i){
        if(!i.value().isArray()) continue;
        const ExtensionUi::Menus menus = ExtensionUi::Menus::FromJson(i.value().toArray());
        if(!menus.IsEmpty()) m_Menus.insert(i.key(), menus);
    }
}

void ExtensionController::SaveMenusSoon(){
    if(m_MenusFile.isEmpty() || m_MenusDirty) return;
    m_MenusDirty = true;
    QTimer::singleShot(300, this, [this](){ SaveMenusNow(); });
}

void ExtensionController::SaveMenusNow(){
    m_MenusDirty = false;
    if(m_MenusFile.isEmpty()) return;
    QJsonObject all;
    for(QHash<QString, ExtensionUi::Menus>::const_iterator i = m_Menus.constBegin(); i != m_Menus.constEnd(); ++i)
        if(!i.value().IsEmpty()) all[i.key()] = i.value().ToJson();
    QSaveFile file(m_MenusFile);
    if(!file.open(QIODevice::WriteOnly) ||
       file.write(QJsonDocument(all).toJson(QJsonDocument::Compact)) < 0 || !file.commit()){
        qWarning() << "the extensions' menus could not be written to" << m_MenusFile;
    }
}

QString ExtensionController::CopyNote(const QString &note, const QString &detail, bool withheld){
    if(!note.isEmpty()){
        QString why = QCoreApplication::translate("ExtensionCopy", note.toLatin1().constData());
        if(why.contains(QStringLiteral("%1"))) why = why.arg(detail);
        return tr("Runs without the compatibility layer: %1.").arg(why);
    }
    if(withheld)
        return tr("Runs with part of the compatibility layer: its web_accessible_resources let web pages "
                  "read a file the layer's key would be in, so it cannot ask Vanilla for tabs, bookmarks, "
                  "downloads and the like.");
    return QString();
}

void ExtensionController::Note(const QString &path, const QString &text){
    const QString key = ExtensionManifest::PathKey(path);
    if(text.isEmpty()) m_Notes.remove(key);
    else m_Notes.insert(key, text);
}

QList<ExtensionRuleFiles> ExtensionController::EnabledRuleFiles(){
    QList<ExtensionRuleFiles> result;
    foreach(const ExtensionManifest &manifest, EnabledManifests()){
        const QList<Dnr::Held::Ruleset> rulesets = manifest.Rulesets();
        QSet<QString> chosen;
        bool anyChose = false;
        foreach(const QPointer<ExtensionController> &c, g_Controllers){
            if(!c || c->m_RulesFile.isEmpty() || !c->m_Rules.Chose(manifest.id, manifest.version)) continue;
            anyChose = true;
            foreach(const QString &id, c->m_Rules.Enabled(manifest.id, rulesets, manifest.version)) chosen.insert(id);
        }
        if(!anyChose)
            foreach(const Dnr::Held::Ruleset &ruleset, rulesets) if(ruleset.enabled) chosen.insert(ruleset.id);

        ExtensionRuleFiles each;
        each.id = manifest.id;
        foreach(const ExtensionManifest::RuleResource &resource, manifest.ruleResources)
            if(chosen.contains(resource.id) && !resource.file.isEmpty()) each.files << resource.file;
        QJsonArray session, dynamic;
        foreach(const QPointer<ExtensionController> &c, g_Controllers){
            if(!c || c->m_RulesFile.isEmpty()) continue;
            for(const QJsonValue &rule : c->m_Rules.Get(manifest.id, Dnr::Held::Session, QJsonValue(), manifest.version)) session.append(rule);
            for(const QJsonValue &rule : c->m_Rules.Get(manifest.id, Dnr::Held::Dynamic, QJsonValue(), manifest.version)) dynamic.append(rule);
        }
        if(!session.isEmpty()) each.session = QJsonDocument(session).toJson(QJsonDocument::Compact);
        if(!dynamic.isEmpty()) each.dynamic = QJsonDocument(dynamic).toJson(QJsonDocument::Compact);
        if(!each.files.isEmpty() || !session.isEmpty() || !dynamic.isEmpty()) result << each;
    }
    return result;
}

void ExtensionController::RegisterPath(const QString &path){
    ChangeList(PATHS_KEY, path, true);
}

void ExtensionController::UnregisterPath(const QString &path){
    ChangeList(PATHS_KEY, path, false);
    ChangeList(DISABLED_KEY, path, false);
    ChangeList(PINNED_KEY, path, false);
}

void ExtensionController::SetEnabled(const QString &path, bool enabled){
    ChangeList(DISABLED_KEY, path, !enabled);
}

void ExtensionController::SetPinned(const QString &path, bool pinned){
    ChangeList(PINNED_KEY, path, pinned);
}

void ExtensionController::Reconcile(const QStringList &paths, const QStringList &disabled,
                                    const QStringList &pinned){
    const QStringList sourcePaths = Normalized(paths);
    QStringList order, disabledList, pinnedList;
    foreach(const QString &path, sourcePaths) order.append(ExtensionManifest::PathKey(path));
    foreach(const QString &path, Normalized(disabled)) disabledList.append(ExtensionManifest::PathKey(path));
    foreach(const QString &path, Normalized(pinned)) pinnedList.append(ExtensionManifest::PathKey(path));
    const QSet<QString> disabledSet(disabledList.begin(), disabledList.end());

    if(order != m_Order || disabledSet != m_Disabled) emit InvalidatePopups();

    QMap<QString, ExtensionManifest> desired;
    QSet<QString> ids;
    for(qsizetype i = 0; i < order.size(); ++i){
        const QString path = order.at(i);
        if(!m_Manifests.contains(path))
            m_Manifests.insert(path, ExtensionManifest::Read(sourcePaths.at(i)));
        ExtensionManifest m = m_Manifests.value(path);
        if(m.error.isEmpty() && ids.contains(m.id))
            m.error = tr("Another registered directory has the same extension ID.");
        if(m.error.isEmpty()) ids.insert(m.id);
        desired.insert(path, m);
        if(!m_Desired.contains(path) || m_Disabled.contains(path) != disabledSet.contains(path))
            m_Errors.remove(path);
    }
    foreach(const QString &path, m_Order){
        if(desired.contains(path)) continue;
        const QString id = m_Manifests.value(path).id;
        if(!id.isEmpty() && !ids.contains(id) && m_Rules.Holds(id)){
            m_Rules.Forget(id);
            SaveRulesSoon();
        }
        if(!id.isEmpty() && !ids.contains(id)) emit Unlisted(id);
        m_Errors.remove(path);
        m_Manifests.remove(path);
    }
    QSet<QString> running;
    for(auto i = desired.constBegin(); i != desired.constEnd(); ++i)
        if(i.value().error.isEmpty() && !disabledSet.contains(i.key())) running.insert(i.value().id);
    m_Rules.KeepSessionsOf(running);
    if(!m_StartupOwed.isEmpty()){
        QHash<QString, QString> was, is;
        for(auto i = m_Desired.constBegin(); i != m_Desired.constEnd(); ++i)
            if(i.value().error.isEmpty() && !m_Disabled.contains(i.key())) was.insert(i.value().id, i.value().version);
        for(auto i = desired.constBegin(); i != desired.constEnd(); ++i)
            if(i.value().error.isEmpty() && !disabledSet.contains(i.key())) is.insert(i.value().id, i.value().version);
        foreach(const QString &id, m_StartupOwed.values())
            if(!running.contains(id) || was.value(id) != is.value(id)) m_StartupOwed.remove(id);
    }
    m_Desired = desired;
    m_Order = order;
    m_Disabled = disabledSet;
    m_Pinned = QSet<QString>(pinnedList.begin(), pinnedList.end());
    m_Commands.clear();
    foreach(const QString &path, m_Order){
        const ExtensionManifest m = m_Desired.value(path);
        if(!m.error.isEmpty() || m.id.isEmpty() || m_Disabled.contains(path)) continue;
        foreach(const ExtensionUi::Command &command, m.commands)
            if(!command.key.isEmpty()) m_Commands[command.key].append(CommandOwner{path, m.id, command.name});
    }
    if(m_InstalledRead) ApplyInstalled(false, false);
    emit Changed();
    Schedule();
}

void ExtensionController::Start(){
    if(m_Started) return;
    m_Started = true;
    ReadSettings();
    DecideInstalled();
    if(m_Fatal){
        MarkReady();
        return;
    }
    const quint64 sequence = Begin();
    QPointer<ExtensionController> self(this);
    Snapshot([self, sequence](QList<ExtensionItem> items, QString error){
        if(!self || !self->Complete(sequence)) return;
        if(!error.isEmpty()){
            self->m_Error = error;
            self->m_Fatal = true;
        } else {
            foreach(const ExtensionItem &item, items){
                self->m_Actual.insert(item.id, item);
                if(item.path.isEmpty()) continue;
                const QString loaded = ExtensionManifest::PathKey(item.path);
                foreach(const QString &path, self->m_Order){
                    const ExtensionManifest desired = self->m_Desired.value(path);
                    if(!desired.error.isEmpty() || desired.id != item.id ||
                       ExtensionManifest::PathKey(desired.folder) != loaded) continue;
                    self->m_Owned.insert(path, item.id);
                    break;
                }
            }
        }
        self->Schedule();
    });
}

quint64 ExtensionController::Begin(){
    emit InvalidatePopups();
    m_Busy = true;
    const quint64 sequence = ++m_Sequence;
    emit Changed();
    QTimer::singleShot(m_TimeoutMs, this, [this, sequence](){
        if(!m_Busy || m_Sequence != sequence) return;
        m_Error = tr("The engine is still processing extensions. "
                     "Browsing can continue; no overlapping operation will be started.");
        MarkReady();
    });
    return sequence;
}

bool ExtensionController::Complete(quint64 sequence){
    if(!m_Busy || m_Sequence != sequence) return false;
    m_Busy = false;
    if(!m_Fatal) m_Error.clear();
    return true;
}

void ExtensionController::Schedule(){
    if(m_Scheduled) return;
    m_Scheduled = true;
    QTimer::singleShot(0, this, [this](){
        m_Scheduled = false;
        Step();
    });
}

void ExtensionController::MarkReady(){
    if(!m_Ready){
        m_Ready = true;
        emit Ready();
    }
    emit Changed();
}

void ExtensionController::Step(){
    if(!m_Started || m_Busy) return;
    if(m_Fatal){
        MarkReady();
        return;
    }
    QPointer<ExtensionController> self(this);

    for(QMap<QString, QString>::const_iterator i = m_Owned.begin(); i != m_Owned.end(); ++i){
        const QString path = i.key();
        const QString id = i.value();
        if(m_Desired.contains(path) && m_Desired.value(path).id == id) continue;
        if(m_Errors.contains(path)) continue;
        if(!m_Actual.contains(id)){
            m_Owned.remove(path);
            SaveOwned();
            if(m_Menus.remove(id)) SaveMenusSoon();
            m_Actions.remove(id);
            Schedule();
            return;
        }
        const quint64 sequence = Begin();
        Remove(id, [self, sequence, path, id](QString error){
            if(!self || !self->Complete(sequence)) return;
            if(error.isEmpty()){
                self->m_Actual.remove(id);
                self->m_Owned.remove(path);
                self->SaveOwned();
                if(self->m_Menus.remove(id)) self->SaveMenusSoon();
                self->m_Actions.remove(id);
            } else {
                self->m_Errors.insert(path, error);
            }
            self->Schedule();
        });
        return;
    }

    foreach(const QString &path, m_Order){
        const ExtensionManifest manifest = m_Desired.value(path);
        if(!manifest.error.isEmpty() || m_Errors.contains(path)) continue;
        const QString id = manifest.id;
        if(m_Actual.contains(id) && m_Owned.value(path) != id){
            m_Errors.insert(path, tr("This ID is already used by an extension not owned by this registration."));
            continue;
        }
        if(!m_Actual.contains(id)){
            if(m_Disabled.contains(path)) continue;
            m_Owned.insert(path, id);
            if(!SaveOwned()){
                MarkReady();
                return;
            }
            const quint64 sequence = Begin();
            Add(manifest, [self, sequence, path, id](ExtensionItem item, QString error){
                if(!self || !self->Complete(sequence)) return;
                if(error.isEmpty() && item.id != id){
                    self->m_Error = tr("The engine returned an unexpected extension ID. Management is paused.");
                    self->m_Fatal = true;
                    if(ValidId(item.id) && !self->m_Actual.contains(item.id)){
                        self->m_Owned.insert(path, item.id);
                        self->SaveOwned();
                        const quint64 cleanup = self->Begin();
                        self->Remove(item.id, [self, cleanup, path](QString cleanupError){
                            if(!self || !self->Complete(cleanup)) return;
                            if(cleanupError.isEmpty()){
                                self->m_Owned.remove(path);
                                self->SaveOwned();
                            } else {
                                self->m_Errors.insert(path, cleanupError);
                            }
                            self->MarkReady();
                        });
                        return;
                    }
                } else if(error.isEmpty()){
                    self->m_Actual.insert(id, item);
                } else {
                    self->m_Errors.insert(path, error);
                }
                self->Schedule();
            });
            return;
        }
        const bool enabled = !m_Disabled.contains(path);
        if(m_Actual.value(id).enabled != enabled){
            const quint64 sequence = Begin();
            Enable(id, enabled, [self, sequence, path, id, enabled](ExtensionItem item, QString error){
                if(!self || !self->Complete(sequence)) return;
                if(error.isEmpty() && (item.id != id || item.enabled != enabled))
                    error = tr("The engine did not apply the requested enabled state.");
                if(error.isEmpty()) self->m_Actual.insert(id, item);
                else self->m_Errors.insert(path, error);
                if(item.gone) self->m_Actual.remove(id);
                self->Schedule();
            });
            return;
        }
    }
    MarkReady();
}

void ExtensionController::Retry(const QString &input){
    if(m_Busy || m_Fatal) return;
    if(!m_Started){
        Start();
        return;
    }
    const QString path = ExtensionManifest::PathKey(input);
    emit InvalidatePopups();
    m_Errors.remove(path);
    if(m_Desired.contains(path)){
        m_Manifests.insert(path, ExtensionManifest::Read(m_Desired.value(path).path));
        QStringList sources;
        foreach(const QString &key, m_Order) sources.append(m_Desired.value(key).path);
        Reconcile(sources, m_Disabled.values(), m_Pinned.values());
    }
    Schedule();
}

QList<ExtensionRow> ExtensionController::Rows() const {
    QList<ExtensionRow> rows;
    QStringList paths = m_Order;
    for(QMap<QString, QString>::const_iterator i = m_Owned.begin(); i != m_Owned.end(); ++i){
        if(!paths.contains(i.key())) paths.append(i.key());
    }
    foreach(const QString &path, paths){
        ExtensionRow row;
        row.registered = m_Desired.contains(path);
        row.manifest = row.registered ? m_Desired.value(path) : ExtensionManifest::Read(path);
        row.error = m_Errors.value(path, row.manifest.error);
        row.note = m_Notes.value(ExtensionManifest::PathKey(path));
        const QString id = m_Owned.value(path);
        row.loaded = !id.isEmpty() && m_Actual.contains(id);
        if(row.loaded){
            const ExtensionItem item = m_Actual.value(id);
            row.enabled = item.enabled;
            row.manifest.id = id;
            if(!item.name.isEmpty()) row.manifest.name = item.name;
            if(!item.popup.isEmpty()) row.manifest.popup = item.popup;
        }
        row.wanted = !m_Disabled.contains(path);
        row.pinned = m_Pinned.contains(path);
        rows.append(row);
    }
    return rows;
}

ExtensionNavigation *ExtensionNavigation::Of(QObject *target){
    ExtensionNavigation *pending =
        target->findChild<ExtensionNavigation*>(QString(), Qt::FindDirectChildrenOnly);
    return pending ? pending : new ExtensionNavigation(target);
}

void ExtensionNavigation::Cancel(){
    disconnect(m_Connection);
    m_Action = std::function<void()>();
}

void ExtensionNavigation::Request(ExtensionController *controller, std::function<void()> action){
    Cancel();
    if(!controller || controller->IsReady()){
        action();
        return;
    }
    m_Action = std::move(action);
    m_Connection = connect(controller, &ExtensionController::Ready, this, [this](){
        std::function<void()> action = std::move(m_Action);
        Cancel();
        if(action) action();
    });
    controller->Start();
}
