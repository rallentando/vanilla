#include "switch.hpp"
#include "const.hpp"

#include "settingspage.hpp"

#include <algorithm>

#include <QAction>
#include <QBuffer>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

#include "application.hpp"
#include "directorypage.hpp"
#include "settingsschema.hpp"
#include "inputmapschema.hpp"
#include "menuschema.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "view.hpp"
#include "gadgets.hpp"
#include "dialog.hpp"
#include "extensioncontroller.hpp"
#include "theme.hpp"
#include "networkcontroller.hpp"
#include "profiles.hpp"
#include "profilelist.hpp"
#include "passwordstore.hpp"
#include "downloadhistory.hpp"
#include "sitepermissions.hpp"
#include "page.hpp"

namespace {
    QDeadlineTimer s_KeyRecording(0);

    const QByteArray SCHEME = VANILLA_SCHEME.toLatin1();
    const char HOST[] = "settings";
    const char START_HOST[] = "start";

    QByteArray Compact(const QJsonObject &object){
        return QJsonDocument(object).toJson(QJsonDocument::Compact);
    }

    QByteArray Error(const QString &message){
        QJsonObject object;
        object[QStringLiteral("error")] = message;
        return Compact(object);
    }

    VanillaPageResponse Ok(const QByteArray &contentType, const QByteArray &body){
        VanillaPageResponse response;
        response.m_Status = 200;
        response.m_ContentType = contentType;
        response.m_Body = body;
        return response;
    }

    VanillaPageResponse Json(const QByteArray &body){
        return Ok(QByteArrayLiteral("application/json"), body);
    }

    VanillaPageResponse Failure(int status){
        VanillaPageResponse response;
        response.m_Status = status;
        return response;
    }

    VanillaPageResponse Resource(const QString &path, const QByteArray &contentType){
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) return Failure(404);
        return Ok(contentType, file.readAll());
    }

    VanillaPageResponse StyleSheet(const QString &path){
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) return Failure(404);
        return Ok(QByteArrayLiteral("text/css"),
                  Theme::PageVariables() + file.readAll());
    }

    VanillaPageResponse ResourcePage(const QString &path){
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) return Failure(404);
        return Ok(QByteArrayLiteral("text/html"),
                  VanillaPage::WithInlinePalette(file.readAll()));
    }

    VanillaPageResponse InputApi(const QString &endpoint,
                                 const QByteArray &method,
                                 const QByteArray &body){
        if(endpoint == QStringLiteral("inputs")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(InputMapSchema::Describe(Application::GlobalSettings())));
        }

        if(method != QByteArrayLiteral("POST")) return Failure(403);

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        const QString group = request[QStringLiteral("group")].toString();
        const InputMapSchema::Table *table = InputMapSchema::Find(group);
        if(!table) return Json(Error(QStringLiteral("unknown table: \"%1\"").arg(group)));

        Settings &settings = Application::GlobalSettings();
        if(endpoint == QStringLiteral("input-set")){
            const QString error = InputMapSchema::Write
                (settings, *table, request[QStringLiteral("entries")].toArray());
            if(!error.isEmpty()) return Json(Error(error));
        } else {
            InputMapSchema::Reset(settings, *table);
        }

        Application::Reconfigure();
        return Json(Compact(InputMapSchema::DescribeTable(settings, *table)));
    }

    VanillaPageResponse MenuApi(const QString &endpoint,
                                const QByteArray &method,
                                const QByteArray &body){
        if(endpoint == QStringLiteral("menus")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(MenuSchema::Describe(Application::GlobalSettings())));
        }

        if(method != QByteArrayLiteral("POST")) return Failure(403);

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        const QString name = request[QStringLiteral("group")].toString();
        const MenuSchema::Table *table = MenuSchema::Find(name);
        if(!table) return Json(Error(QStringLiteral("unknown menu: \"%1\"").arg(name)));

        Settings &settings = Application::GlobalSettings();
        if(endpoint == QStringLiteral("menu-set")){
            if(!request[QStringLiteral("names")].isArray())
                return Json(Error(QStringLiteral("names is not a list")));
            const QString error = MenuSchema::Write
                (settings, *table, request[QStringLiteral("names")].toArray());
            if(!error.isEmpty()) return Json(Error(error));
        } else {
            MenuSchema::Reset(settings, *table);
        }

        Application::Reconfigure();
        return Json(Compact(MenuSchema::DescribeTable(settings, *table)));
    }

    VanillaPageResponse Deferred(std::function<QByteArray()> work){
        VanillaPageResponse response = Json(QByteArrayLiteral("{}"));
        response.m_UserInteraction = VanillaPageResponse::UserInteraction::Deferred;
        response.m_Deferred = work;
        return response;
    }

    QJsonObject DescribePasswords(){
        QJsonObject strings;
        strings[QStringLiteral("origin")]       = QCoreApplication::translate("SettingsPage", "Site");
        strings[QStringLiteral("username")]     = QCoreApplication::translate("SettingsPage", "Username");
        strings[QStringLiteral("space")]        = QCoreApplication::translate("SettingsPage", "Profile");
        strings[QStringLiteral("noUsername")]   = QCoreApplication::translate("SettingsPage", "(no username)");
        strings[QStringLiteral("remove")]       = QCoreApplication::translate("SettingsPage", "Remove");
        strings[QStringLiteral("removeAll")]    = QCoreApplication::translate("SettingsPage", "Remove all");
        strings[QStringLiteral("confirmAll")]   = QCoreApplication::translate("SettingsPage", "Remove every saved password?");
        strings[QStringLiteral("removing")]     = QCoreApplication::translate("SettingsPage", "Removing...");
        strings[QStringLiteral("removeFailed")] = QCoreApplication::translate("SettingsPage", "The password could not be removed.");
        strings[QStringLiteral("none")]         = QCoreApplication::translate("SettingsPage", "No passwords are saved.");
        strings[QStringLiteral("neverHeading")] = QCoreApplication::translate("SettingsPage", "Never saved for these sites");
        strings[QStringLiteral("neverAbout")]   =
            QCoreApplication::translate("SettingsPage", "A removed site is asked about again.");
        strings[QStringLiteral("edge")]         =
            QCoreApplication::translate("SettingsPage", "Passwords an Edge view saved are not listed here. "
                   "\"Remove all\" removes them too.");
        strings[QStringLiteral("unavailable")]  =
            QCoreApplication::translate("SettingsPage", "This build does not save passwords in a WebEngine view.");

        QJsonArray entries;
        QJsonArray never;
        bool available = false;
        int pending = 0;
#ifdef VANILLA_PASSWORDS
        if(PasswordStore *store = PasswordStore::Instance()){
            available = true;
            pending = store->PendingRemovals();
            foreach(const PasswordStore::Entry &entry, store->Entries()){
                QJsonObject obj;
                obj[QStringLiteral("space")]    = entry.space;
                obj[QStringLiteral("origin")]   = entry.origin;
                obj[QStringLiteral("username")] = entry.username;
                entries.append(obj);
            }
            foreach(const PasswordStore::Entry &entry, store->NeverEntries()){
                QJsonObject obj;
                obj[QStringLiteral("space")]  = entry.space;
                obj[QStringLiteral("origin")] = entry.origin;
                never.append(obj);
            }
        }
#endif
        QJsonObject reply;
        reply[QStringLiteral("strings")] = strings;
        reply[QStringLiteral("entries")] = entries;
        reply[QStringLiteral("never")] = never;
        reply[QStringLiteral("available")] = available;
        reply[QStringLiteral("pending")] = pending;
#ifdef EDGEWEBVIEW
        reply[QStringLiteral("edge")] = true;
#else
        reply[QStringLiteral("edge")] = false;
#endif
        return reply;
    }

    VanillaPageResponse PasswordsApi(const QString &endpoint,
                                     const QByteArray &method,
                                     const QByteArray &body){
        if(endpoint == QStringLiteral("passwords")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(DescribePasswords()));
        }

        if(method != QByteArrayLiteral("POST")) return Failure(403);

        if(endpoint == QStringLiteral("passwords-remove-all")){
            return Deferred([](){
                NetworkController::ClearPasswords();
                return QByteArrayLiteral("{\"queued\":true}");
            });
        }

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        const QString space    = request[QStringLiteral("space")].toString();
        const QString origin   = request[QStringLiteral("origin")].toString();
        const QString username = request[QStringLiteral("username")].toString();
        if(origin.isEmpty()) return Json(Error(QStringLiteral("no origin")));

        if(endpoint == QStringLiteral("passwords-never-remove")){
            return Deferred([space, origin](){
#ifdef VANILLA_PASSWORDS
                if(PasswordStore *store = PasswordStore::Instance())
                    store->SetNever(space, origin, false);
#else
                Q_UNUSED(space) Q_UNUSED(origin)
#endif
                return QByteArrayLiteral("{\"queued\":true}");
            });
        }
        return Deferred([space, origin, username](){
#ifdef VANILLA_PASSWORDS
            if(PasswordStore *store = PasswordStore::Instance())
                store->Remove(space, origin, username, nullptr);
#else
            Q_UNUSED(space) Q_UNUSED(origin) Q_UNUSED(username)
#endif
            return QByteArrayLiteral("{\"queued\":true}");
        });
    }

    QJsonObject DescribePermissions(){
        QJsonObject strings;
        strings[QStringLiteral("allow")]      = QCoreApplication::translate("SettingsPage", "Allow");
        strings[QStringLiteral("block")]      = QCoreApplication::translate("SettingsPage", "Block");
        strings[QStringLiteral("remove")]     = QCoreApplication::translate("SettingsPage", "Remove");
        strings[QStringLiteral("removeAll")]  = QCoreApplication::translate("SettingsPage", "Remove all");
        strings[QStringLiteral("removeSite")] = QCoreApplication::translate("SettingsPage", "Remove all for this site");
        strings[QStringLiteral("confirmAll")] = QCoreApplication::translate("SettingsPage", "Remove every site permission?");
        strings[QStringLiteral("failed")]     = QCoreApplication::translate("SettingsPage", "The permission could not be changed.");
        strings[QStringLiteral("none")]       =
            QCoreApplication::translate("SettingsPage", "No site has been allowed or blocked.");
        strings[QStringLiteral("about")]      =
            QCoreApplication::translate("SettingsPage", "Shared by every view. A removed one is asked again.");
        strings[QStringLiteral("edgeOnly")]   = QCoreApplication::translate("SettingsPage", "Enabled in Edge");
        strings[QStringLiteral("engineOnly")] = QCoreApplication::translate("SettingsPage", "Enabled in WebEngine");
        strings[QStringLiteral("addHeading")] = QCoreApplication::translate("SettingsPage", "Add a site");
        strings[QStringLiteral("addSite")]    = QCoreApplication::translate("SettingsPage", "Site (example.com)");
        strings[QStringLiteral("add")]        = QCoreApplication::translate("SettingsPage", "Add");
        strings[QStringLiteral("badSite")]    =
            QCoreApplication::translate("SettingsPage", "Enter the address of a site, such as https://example.com.");

        const QStringList kinds = PermissionTable::Kinds();
        QJsonObject labels;
        QJsonObject scopes;
        for(const QString &kind : kinds){
            labels[kind] = VanillaPage::PermissionKindLabel(kind);
            scopes[kind] = VanillaPage::PermissionKindScope(kind);
        }

        QJsonArray entries;
        foreach(const PermissionTable::Row &row, SitePermissions::Table().Rows()){
            QJsonObject entry;
            entry[QStringLiteral("origin")] = row.origin;
            entry[QStringLiteral("kind")]   = row.kind;
            entry[QStringLiteral("state")]  = PermissionTable::StateName(row.state);
            const QUrl address(row.origin);
            if(address.scheme() == QStringLiteral("chrome-extension"))
                entry[QStringLiteral("name")] = ExtensionController::NameOfId(address.host());
            entries.append(entry);
        }

        QJsonObject reply;
        reply[QStringLiteral("strings")] = strings;
        reply[QStringLiteral("labels")]  = labels;
        reply[QStringLiteral("scopes")]  = scopes;
        QStringList offered;
        const QStringList order = QStringList() << QString()
#ifdef EDGEWEBVIEW
            << QStringLiteral("edge") << QStringLiteral("engine");
#else
            << QStringLiteral("engine") << QStringLiteral("edge");
#endif
        for(const QString &wanted : order)
            for(int i = 0; i < kinds.length(); i++)
                if(VanillaPage::PermissionKindScope(kinds[i]) == wanted) offered << kinds[i];
        reply[QStringLiteral("kinds")]   = QJsonArray::fromStringList(offered);
        reply[QStringLiteral("entries")] = entries;
        return reply;
    }

    VanillaPageResponse ProfilesApi(const QString &endpoint,
                                    const QByteArray &method,
                                    const QByteArray &body){
        if(endpoint == QStringLiteral("profiles")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(Profiles::Describe()));
        }
        if(method != QByteArrayLiteral("POST")) return Failure(403);
        if(endpoint == QStringLiteral("profiles-remove-all")){
            return Deferred([](){
                const QString refused = Profiles::RemoveAll();
                if(!refused.isEmpty()) return Error(refused);
                return QByteArrayLiteral("{\"queued\":true}");
            });
        }
        const QString id = QJsonDocument::fromJson(body).object()[QStringLiteral("id")].toString();
        if(!ProfileList::IsDigest(id)) return Json(Error(QStringLiteral("bad id")));
        return Deferred([id](){
            const QString refused = Profiles::Remove(id);
            if(!refused.isEmpty()) return Error(refused);
            return QByteArrayLiteral("{\"queued\":true}");
        });
    }

    VanillaPageResponse PermissionsApi(const QString &endpoint,
                                       const QByteArray &method,
                                       const QByteArray &body){
        if(endpoint == QStringLiteral("permissions")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(DescribePermissions()));
        }

        if(method != QByteArrayLiteral("POST")) return Failure(403);

        if(endpoint == QStringLiteral("permissions-remove-all")){
            return Deferred([](){
                SitePermissions::Clear();
                return QByteArrayLiteral("{\"queued\":true}");
            });
        }

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        QString origin = request[QStringLiteral("origin")].toString();
        const QString kind   = request[QStringLiteral("kind")].toString();
        if(endpoint == QStringLiteral("permissions-set") && request.contains(QStringLiteral("site"))){
            if(request.contains(QStringLiteral("origin"))) return Json(Error(QStringLiteral("no such permission")));
            origin = PermissionTable::OriginFromUserText(request[QStringLiteral("site")].toString());
            if(origin.isEmpty()) return Json(Error(QStringLiteral("no such site")));
        }
        PermissionTable::State state = PermissionTable::Ask;
        if(endpoint == QStringLiteral("permissions-set")){
            state = PermissionTable::StateFromName(request[QStringLiteral("state")].toString());
            if(state == PermissionTable::Ask) return Json(Error(QStringLiteral("no state")));
        }
        if(origin.isEmpty() || PermissionTable::Origin(origin) != origin || kind.isEmpty() ||
           (state != PermissionTable::Ask && !PermissionTable::IsKind(kind)))
            return Json(Error(QStringLiteral("no such permission")));
        return Deferred([origin, kind, state](){
            SitePermissions::Set(origin, kind, state);
            return QByteArrayLiteral("{\"queued\":true}");
        });
    }

    QJsonObject DescribeDownloads(){
        QJsonObject strings;
        strings[QStringLiteral("running")]     = QCoreApplication::translate("SettingsPage", "Downloading");
        strings[QStringLiteral("completed")]   = QCoreApplication::translate("SettingsPage", "Completed");
        strings[QStringLiteral("cancelled")]   = QCoreApplication::translate("SettingsPage", "Cancelled");
        strings[QStringLiteral("failed")]      = QCoreApplication::translate("SettingsPage", "Failed");
        strings[QStringLiteral("interrupted")] = QCoreApplication::translate("SettingsPage", "Interrupted");
        strings[QStringLiteral("open")]        = QCoreApplication::translate("SettingsPage", "Open");
        strings[QStringLiteral("show")]        = QCoreApplication::translate("SettingsPage", "Show in folder");
        strings[QStringLiteral("cancel")]      = QCoreApplication::translate("SettingsPage", "Cancel");
        strings[QStringLiteral("remove")]      = QCoreApplication::translate("SettingsPage", "Remove from list");
        strings[QStringLiteral("clear")]       = QCoreApplication::translate("SettingsPage", "Clear list");
        strings[QStringLiteral("none")]        = QCoreApplication::translate("SettingsPage", "Nothing has been downloaded.");
        strings[QStringLiteral("history")]     = QCoreApplication::translate("SettingsPage", "History");
        strings[QStringLiteral("missing")]     = QCoreApplication::translate("SettingsPage", "The file is not there any more.");
        strings[QStringLiteral("source")]      = QCoreApplication::translate("SettingsPage", "Open the page it came from");

        QJsonArray entries;
        foreach(const DownloadHistory::Entry &entry, DownloadHistory::Instance()->Entries()){
            const QFileInfo info(entry.path);
            QJsonObject obj;
            obj[QStringLiteral("id")]       = entry.id;
            obj[QStringLiteral("name")]     = info.fileName();
            obj[QStringLiteral("url")]      = entry.url.toString();
            obj[QStringLiteral("state")]    = DownloadHistory::StateName(entry.state);
            obj[QStringLiteral("received")] = static_cast<double>(entry.received);
            obj[QStringLiteral("total")]    = static_cast<double>(entry.total);
            obj[QStringLiteral("started")]  = entry.started.toString(Qt::ISODate);
            obj[QStringLiteral("exists")]   = info.isFile();
            obj[QStringLiteral("canOpen")]  = DownloadHistory::CanOpen(entry);
            obj[QStringLiteral("hasSource")] = !entry.page.isEmpty();
            entries.append(obj);
        }
        QJsonObject reply;
        reply[QStringLiteral("strings")] = strings;
        reply[QStringLiteral("entries")] = entries;
        return reply;
    }

    QByteArray Done(bool ok){
        return ok ? QByteArrayLiteral("{\"ok\":true}") : QByteArrayLiteral("{\"ok\":false}");
    }

    bool OpenDownloadSource(int id);

    QByteArray DownloadAction(const QString &endpoint, int id){
        DownloadHistory *history = DownloadHistory::Instance();
        if(endpoint == QStringLiteral("downloads-clear")){
            history->Clear();
            return Done(true);
        }
        const DownloadHistory::Entry *entry = history->Find(id);
        if(!entry) return Done(false);
        if(endpoint == QStringLiteral("downloads-remove")) return Done(history->Remove(id));
        if(endpoint == QStringLiteral("downloads-cancel")) return Done(history->Cancel(id));

        if(endpoint == QStringLiteral("downloads-open")) return Done(history->Open(id));
        if(endpoint == QStringLiteral("downloads-show")) return Done(history->ShowInFolder(id));
        return Done(false);
    }

    VanillaPageResponse DownloadsApi(const QString &endpoint,
                                     const QByteArray &method,
                                     const QByteArray &body){
        if(endpoint == QStringLiteral("downloads")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(DescribeDownloads()));
        }
        if(method != QByteArrayLiteral("POST")) return Failure(403);
        const int id = QJsonDocument::fromJson(body).object()[QStringLiteral("id")].toInt();
        if(endpoint == QStringLiteral("downloads-source"))
            return Json(Done(OpenDownloadSource(id)));
        return Deferred([endpoint, id](){ return DownloadAction(endpoint, id);});
    }

    VanillaPageResponse DirectorySectionApi(const QString &endpoint, const QByteArray &method,
                                            const QByteArray &body, const QString &node);

    QByteArray SchemaJson(){
        QJsonObject schema = SettingsSchema::Describe();
        schema[QStringLiteral("inputs")] =
            InputMapSchema::Describe(Application::GlobalSettings());
        schema[QStringLiteral("menus")] =
            MenuSchema::Describe(Application::GlobalSettings());
        schema[QStringLiteral("translucent")] = Application::EnableTransparentWindow();
        return Compact(schema);
    }

    void OfferRestart(){
        static QPointer<ModelessDialog> offered;
        if(offered){
            if(offered->isVisible() && offered->window()->isVisible()){
                offered->Renew();
                return;
            }
            offered->Discard();
        }
        ModelessDialog *dialog = new ModelessDialog();
        offered = dialog;
        dialog->SetTitle(QCoreApplication::translate("SettingsPage",
                             "A setting takes effect on the next launch."));
        dialog->SetCaption(QCoreApplication::translate("SettingsPage",
                               "Restart now?"));
        dialog->SetButtons(Dialog::Restart | Dialog::Close);
        dialog->SetAutoCancel(false);
        dialog->SetCallBack([](bool restart){
            if(restart) Application::Restart();
        });
        QTimer::singleShot(0, dialog, &ModelessDialog::Execute);
    }

    VanillaPageResponse SettingsApi(const QString &endpoint,
                                    const QByteArray &method,
                                    const QByteArray &body,
                                    const QString &node){
        if(endpoint == QStringLiteral("directory") ||
           endpoint == QStringLiteral("directory-set"))
            return DirectorySectionApi(endpoint, method, body, node);

        if(endpoint == QStringLiteral("schema")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(SchemaJson());
        }

        if(endpoint == QStringLiteral("input-recording")){
            if(method != QByteArrayLiteral("POST")) return Failure(403);
            const bool on = QJsonDocument::fromJson(body).object()[QStringLiteral("on")].toBool();
            s_KeyRecording.setRemainingTime(on ? 5 * 60 * 1000 : 0);
            return Json(QByteArrayLiteral("{}"));
        }

        if(endpoint == QStringLiteral("inputs") ||
           endpoint == QStringLiteral("input-set") ||
           endpoint == QStringLiteral("input-reset"))
            return InputApi(endpoint, method, body);

        if(endpoint == QStringLiteral("menus") ||
           endpoint == QStringLiteral("menu-set") ||
           endpoint == QStringLiteral("menu-reset"))
            return MenuApi(endpoint, method, body);

        if(endpoint == QStringLiteral("passwords") ||
           endpoint == QStringLiteral("passwords-remove") ||
           endpoint == QStringLiteral("passwords-never-remove") ||
           endpoint == QStringLiteral("passwords-remove-all"))
            return PasswordsApi(endpoint, method, body);

        if(endpoint == QStringLiteral("profiles") ||
           endpoint == QStringLiteral("profiles-remove") ||
           endpoint == QStringLiteral("profiles-remove-all"))
            return ProfilesApi(endpoint, method, body);

        if(endpoint == QStringLiteral("permissions") ||
           endpoint == QStringLiteral("permissions-set") ||
           endpoint == QStringLiteral("permissions-remove") ||
           endpoint == QStringLiteral("permissions-remove-all"))
            return PermissionsApi(endpoint, method, body);

        if(endpoint == QStringLiteral("downloads") ||
           endpoint == QStringLiteral("downloads-open") ||
           endpoint == QStringLiteral("downloads-show") ||
           endpoint == QStringLiteral("downloads-cancel") ||
           endpoint == QStringLiteral("downloads-remove") ||
           endpoint == QStringLiteral("downloads-clear") ||
           endpoint == QStringLiteral("downloads-source"))
            return DownloadsApi(endpoint, method, body);

        const bool set   = endpoint == QStringLiteral("set");
        const bool reset = endpoint == QStringLiteral("reset");
        const bool pick  = endpoint == QStringLiteral("pick-directory");
        const bool pickColor = endpoint == QStringLiteral("pick-color");
        if(!set && !reset && !pick && !pickColor) return Failure(404);
        if(method != QByteArrayLiteral("POST")) return Failure(403);

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        const QString key = request[QStringLiteral("key")].toString();

        const SettingsSchema::Item *item = SettingsSchema::Find(key);
        if(!item){
            return Json(Error(QStringLiteral("unknown key: \"%1\"").arg(key)));
        }

        if(pickColor){
            if(item->type != SettingsSchema::Color)
                return Json(Error(QStringLiteral("no colour picker for: \"%1\"").arg(key)));
            const QString title =
                QCoreApplication::translate("SettingsSchema", item->label);
            const QColor start(Application::GlobalSettings()
                                   .value(key, SettingsSchema::Fallback(*item)).toString());
            VanillaPageResponse response = Json(QByteArrayLiteral("{}"));
            response.m_UserInteraction = VanillaPageResponse::UserInteraction::Deferred;
            response.m_Deferred = [title, start](){
                const QColor color = ModalDialog::GetColor(start.isValid() ? start : QColor(Qt::black), title);
                QJsonObject reply;
                reply[QStringLiteral("color")] = color.isValid() ? color.name() : QString();
                return Compact(reply);
            };
            return response;
        }

        if(pick){
            const QString kind = item->picker ? QString::fromLatin1(item->picker) : QString();
            if(kind == QStringLiteral("folder") && item->type == SettingsSchema::Directory){
                const QString title =
                    QCoreApplication::translate("SettingsSchema", item->label);
                QString start = Application::GlobalSettings().value(key).toString();
                if(start.isEmpty()) start = SettingsSchema::EmptyDirectory(*item);
                VanillaPageResponse response = Json(QByteArrayLiteral("{}"));
                response.m_UserInteraction = VanillaPageResponse::UserInteraction::Deferred;
                response.m_Deferred = [title, start](){
                    const QString path = ModalDialog::GetExistingDirectory(title, start);
                    QJsonObject reply;
                    reply[QStringLiteral("path")] = path.isEmpty()
                        ? QString() : QDir::toNativeSeparators(QDir::cleanPath(path));
                    return Compact(reply);
                };
                return response;
            }
            if(kind != QStringLiteral("chrome-extension"))
                return Json(Error(QStringLiteral("no directory picker for: \"%1\"")
                                  .arg(key)));

            VanillaPageResponse response = Json(QByteArrayLiteral("{}"));
            response.m_UserInteraction =
                VanillaPageResponse::UserInteraction::PickChromeExtensionDirectory;
            return response;
        }

        const QVariant fallback = SettingsSchema::Fallback(*item);
        const QVariant before = Application::GlobalSettings().value(key, fallback);
        if(reset){
            Application::GlobalSettings().remove(key);
        } else {
            const QVariant value =
                SettingsSchema::FromJson(*item, request[QStringLiteral("value")]);
            if(!value.isValid()) return Json(Error(QStringLiteral("bad value")));
            Application::GlobalSettings().setValue(key, value);
        }

        Application::Reconfigure();
        if(item->needsRestart &&
           SettingsSchema::ToJson(*item, Application::GlobalSettings().value(key, fallback)) !=
           SettingsSchema::ToJson(*item, before))
            OfferRestart();
        if(key == QStringLiteral("network/@ExtensionTabScope")) TreeBank::SomethingChanged();
        if(key == QStringLiteral("gadgets/thumblist/@NodeCollectionType")){
            for(MainWindow *window : Application::GetMainWindows().values()){
                if(TreeBank *bank = window->GetTreeBank()){
                    if(Gadgets *gadgets = bank->GetGadgets())
                        gadgets->ApplyNodeCollectionTypeSetting();
                }
            }
        }

        QJsonObject reply;
        reply[QStringLiteral("key")] = key;
        reply[QStringLiteral("value")] =
            SettingsSchema::ToJson(*item, Application::GlobalSettings()
                                   .value(key, SettingsSchema::Fallback(*item)));
        return Json(Compact(reply));
    }

    VanillaPageResponse DirectorySectionApi(const QString &endpoint, const QByteArray &method,
                                            const QByteArray &body, const QString &node){
        if(endpoint == QStringLiteral("directory")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            ViewNode *pending = DirectoryPage::TakePendingSubject();
            ViewNode *subject = nullptr;
            bool fallback = false;
            if(!node.isEmpty()){
                subject = DirectoryPage::FindDirectory(node);
                fallback = !subject;
            } else {
                subject = pending;
            }
            QJsonObject reply = subject ? DirectoryPage::Describe(subject)
                                        : DirectoryPage::Describe();
            reply[QStringLiteral("requested")] = node;
            reply[QStringLiteral("fallback")] = fallback;
            return Json(Compact(reply));
        }

        if(method != QByteArrayLiteral("POST")) return Failure(403);
        return Deferred([body](){
            const QJsonObject request = QJsonDocument::fromJson(body).object();
            ViewNode *target = DirectoryPage::FindDirectory(request[QStringLiteral("node")].toString());
            const QByteArray reply = DirectoryPage::HandleSet(request);
            if(target && !QJsonDocument::fromJson(reply).object().contains(QStringLiteral("error")))
                DirectoryPage::SetPendingSubject(target);
            return reply;
        });
    }

    VanillaPageResponse Directory(const QUrl &url){
        const QString path = url.path();
        if(path.isEmpty() || path == QStringLiteral("/")){
            return Ok(QByteArrayLiteral("text/html"), QByteArrayLiteral(
                "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">"
                "<meta http-equiv=\"refresh\" content=\"0; url=vanilla://settings/#directory\">"
                "</head><body></body></html>\n"));
        }
        return Failure(404);
    }
}

namespace {

    struct StartAction {
        const char *name;
        const char *label;
        const char *icon;
    };
    const StartAction StartActions[] = {
        { "DisplayViewTree",  QT_TRANSLATE_NOOP("StartPage", "Tab list"),
                              ":/resources/start/table.png" },
        { "DisplayTrashTree", QT_TRANSLATE_NOOP("StartPage", "Trash"),
                              ":/resources/start/trash.png" },
        { "OpenSettings",     QT_TRANSLATE_NOOP("StartPage", "Settings"),
                              ":/resources/settings/settings.png" },
        { "ShowDownloads",    QT_TRANSLATE_NOOP("StartPage", "Download panel"),
                              ":/resources/toolbar/download.png" },
        { "NewWindow",        QT_TRANSLATE_NOOP("StartPage", "New window"),
                              ":/resources/start/window.png" },
    };

    QString DataUrl(const QString &path){
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) return QString();
        return QStringLiteral("data:image/png;base64,") +
            QString::fromLatin1(file.readAll().toBase64());
    }

    bool IsStartAction(const QString &name){
        for(const StartAction &action : StartActions)
            if(name == QLatin1String(action.name)) return true;
        return false;
    }

    TreeBank *CurrentTreeBank(){
        MainWindow *win = Application::GetCurrentWindow();
        return win ? win->GetTreeBank() : nullptr;
    }

    ViewNode *ShowingStartPage(TreeBank *tb){
        ViewNode *vn = tb ? tb->GetCurrentViewNode() : nullptr;
        return vn && VanillaPage::IsStartUrl(vn->GetUrl()) ? vn : nullptr;
    }

    struct StartOrigin {
        QPointer<TreeBank> bank;
        quint64 serial = 0;
    };
    StartOrigin TakeStartOrigin(){
        StartOrigin origin;
        origin.bank = CurrentTreeBank();
        if(ViewNode *self = ShowingStartPage(origin.bank)) origin.serial = self->GetSerial();
        return origin;
    }
    ViewNode *StillShowingStartPage(const StartOrigin &origin){
        ViewNode *self = ShowingStartPage(origin.bank);
        return self && origin.serial && self->GetSerial() == origin.serial ? self : nullptr;
    }

    void ByLastLook(QList<ViewNode*> &tabs){
        std::sort(tabs.begin(), tabs.end(), [](ViewNode *a, ViewNode *b){
            const QDateTime x = a->GetLastAccessDate(), y = b->GetLastAccessDate();
            if(x != y) return x > y;
            return a->GetSerial() < b->GetSerial();
        });
    }

    bool ListableUrl(const QUrl &url){
        return !url.isEmpty() && url != BLANK_URL && !VanillaPage::IsStartUrl(url);
    }

    QList<ViewNode*> CollectTabs(ViewNode *from, const QStringList &words, int here, bool sorted = true){
        QList<ViewNode*> tabs;
        if(!from) return tabs;
        foreach(Node *nd, from->GetDescendants()){
            if(!nd->IsViewNode() || nd->IsDirectory()) continue;
            ViewNode *vn = nd->ToViewNode();
            const QUrl url = vn->GetUrl();
            if(!ListableUrl(url)) continue;
            if(words.isEmpty() && VanillaPage::IsPageUrl(url)) continue;
            if(View *view = vn->GetView()){
                const int there = TreeBank::WinIndex(view->GetThis().lock());
                if(there && there != here) continue;
            }
            if(!words.isEmpty()){
                const QString title = vn->GetTitle();
                const QString address = url.toString();
                bool all = true;
                for(const QString &word : words){
                    if(!title.contains(word, Qt::CaseInsensitive) &&
                       !address.contains(word, Qt::CaseInsensitive)){
                        all = false;
                        break;
                    }
                }
                if(!all) continue;
            }
            tabs << vn;
        }
        if(sorted) ByLastLook(tabs);
        return tabs;
    }

    QHash<quint64, ViewNode*> ViewNodesBySerial(bool trashToo){
        QHash<quint64, ViewNode*> map;
        QList<ViewNode*> roots = QList<ViewNode*>() << TreeBank::GetViewRoot();
        if(trashToo) roots << TreeBank::GetTrashRoot();
        for(ViewNode *root : std::as_const(roots)){
            if(!root) continue;
            foreach(Node *nd, root->GetDescendants())
                if(nd->IsViewNode() && !nd->IsDirectory()) map.insert(nd->GetSerial(), nd->ToViewNode());
        }
        return map;
    }

    const int HistoryLimit = 5000;

    const int StartTabCount = 12;
    const int StartTabLimit = StartTabCount * 20;
    struct StartTabList {
        QList<ViewNode*> tabs;
        bool more = false;
    };
    StartTabList StartTabs(const QString &query, TreeBank *asking, int limit = StartTabCount){
        StartTabList result;
        ViewNode *root = TreeBank::GetViewRoot();
        if(!root) return result;
        const int here = asking ? asking->WinIndex() : 0;
        const QStringList words = query.split(QRegularExpression(QStringLiteral("\\s+"),
                                                                 QRegularExpression::UseUnicodePropertiesOption),
                                              Qt::SkipEmptyParts);
        QList<ViewNode*> tabs = CollectTabs(root, words, here);
        limit = qBound(StartTabCount, limit, StartTabLimit);
        if(words.isEmpty()){
            result.more = tabs.size() > StartTabCount ||
                !CollectTabs(TreeBank::GetTrashRoot(), words, here).isEmpty();
            result.tabs = tabs.mid(0, StartTabCount);
        } else {
            result.more = tabs.size() > limit;
            result.tabs = tabs.mid(0, limit);
        }
        return result;
    }

    StartTabList HistoryTabs(TreeBank *asking, int offset, int limit, const QString &query = QString()){
        StartTabList result;
        const int here = asking ? asking->WinIndex() : 0;
        const QStringList words = query.split(QRegularExpression(QStringLiteral("\\s+"),
                                                                 QRegularExpression::UseUnicodePropertiesOption),
                                              Qt::SkipEmptyParts);
        QList<ViewNode*> tabs = CollectTabs(TreeBank::GetViewRoot(), words, here, false)
            + CollectTabs(TreeBank::GetTrashRoot(), words, here, false);
        ByLastLook(tabs);
        offset = qMax(0, offset);
        limit = qBound(1, limit, HistoryLimit);
        result.more = tabs.size() > offset + limit;
        result.tabs = tabs.mid(offset, limit);
        return result;
    }

    QHash<quint64, int> PendingCloses;

    QJsonArray TabsJson(const QList<ViewNode*> &tabs){
        QJsonArray array;
        foreach(ViewNode *vn, tabs){
            QJsonObject tab;
            const QUrl url = vn->GetUrl();
            const QString title = vn->GetTitle();
            tab[QStringLiteral("node")] = QString::number(vn->GetSerial());
            tab[QStringLiteral("closed")] = TreeBank::IsTrash(vn);
            tab[QStringLiteral("pending")] = PendingCloses.value(vn->GetSerial()) > 0;
            tab[QStringLiteral("seen")] = vn->GetLastAccessDate().toString(Qt::ISODate);
            tab[QStringLiteral("icon")] = !Application::GetIcon(url).isNull();
            tab[QStringLiteral("title")] = title.isEmpty() ? url.toString() : title;
            tab[QStringLiteral("host")] = url.host().isEmpty() ? url.toString() : url.host();
            tab[QStringLiteral("address")] = url.toString();
            QJsonArray path;
            for(Node *nd = vn->GetParent(); nd && nd->GetParent(); nd = nd->GetParent())
                path.prepend(nd->ReadableTitle());
            tab[QStringLiteral("path")] = path;
            const QString image = vn->GetImageFileName();
            tab[QStringLiteral("thumb")] = image.isEmpty()
                ? !vn->GetImage().isNull()
                : QFileInfo::exists(Application::ThumbnailDirectory() + image);
            array.append(tab);
        }
        return array;
    }

    VanillaPageResponse Icon(ViewNode *vn){
        if(!vn || !ListableUrl(vn->GetUrl())) return Failure(404);
        const QIcon icon = Application::GetIcon(vn->GetUrl());
        if(icon.isNull()) return Failure(404);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        if(!icon.pixmap(QSize(16, 16)).save(&buffer, "PNG")) return Failure(404);
        return Ok(QByteArrayLiteral("image/png"), png);
    }

    VanillaPageResponse Thumbnail(ViewNode *vn){
        if(!vn) return Failure(404);
        const QString name = vn->GetImageFileName();
        if(name.isEmpty()){
            const QImage image = vn->GetImage();
            if(image.isNull()) return Failure(404);
            QByteArray jpeg;
            QBuffer buffer(&jpeg);
            buffer.open(QIODevice::WriteOnly);
            if(!image.save(&buffer, "JPEG", THUMBNAIL_JPEG_QUALITY)) return Failure(404);
            return Ok(QByteArrayLiteral("image/jpeg"), jpeg);
        }
        QFile file(Application::ThumbnailDirectory() + name);
        if(!file.open(QIODevice::ReadOnly)) return Failure(404);
        return Ok(QByteArrayLiteral("image/jpeg"), file.readAll());
    }

    ViewNode *ViewNodeOfSerial(quint64 serial, bool trashToo = false){
        if(!serial) return nullptr;
        QList<ViewNode*> roots = QList<ViewNode*>() << TreeBank::GetViewRoot();
        if(trashToo) roots << TreeBank::GetTrashRoot();
        for(ViewNode *root : std::as_const(roots)){
            if(!root) continue;
            foreach(Node *nd, root->GetDescendants())
                if(nd->IsViewNode() && !nd->IsDirectory() && nd->GetSerial() == serial)
                    return nd->ToViewNode();
        }
        return nullptr;
    }

    bool OpenDownloadSource(int id){
        const DownloadHistory::Entry *entry = DownloadHistory::Instance()->Find(id);
        if(!entry || entry->page.isEmpty()) return false;
        const QPointer<TreeBank> bank = CurrentTreeBank();
        ViewNode *self = bank ? bank->GetCurrentViewNode() : nullptr;
        if(!self || !VanillaPage::IsSettingsUrl(self->GetUrl())) return false;
        const quint64 serial = self->GetSerial();
        TreeBank::WhenItMayChange([bank, serial, id](){
            TreeBank *tb = bank;
            if(!tb || !TreeBank::IsLive(tb) || tb != CurrentTreeBank()) return;
            ViewNode *settings = ViewNodeOfSerial(serial);
            if(!settings) return;
            const DownloadHistory::Entry *entry = DownloadHistory::Instance()->Find(id);
            if(!entry || entry->page.isEmpty()) return;
            tb->OpenInNewViewNode(entry->page, true, settings);
        });
        return true;
    }

    QString PrimarySearchEngineKey(){
        const QMap<QString, SearchEngine> engines = Page::GetSearchEngineMap();
        for(auto it = engines.cbegin(); it != engines.cend(); ++it)
            if(it.value().length() >= 3 && it.value()[2] == QStringLiteral("true"))
                return it.key();
        if(engines.contains(QStringLiteral("google"))) return QStringLiteral("google");
        return engines.isEmpty() ? QString() : engines.firstKey();
    }

    QUrl ResolveStartQuery(const QString &text, const QString &engine = QString()){
        const QString trimmed = text.trimmed();
        if(trimmed.isEmpty()) return QUrl();
        if(!trimmed.contains(QRegularExpression(QStringLiteral("\\s")))){
            const QList<QUrl> urls = Page::ExtractUrlsFromText(trimmed);
            if(urls.length() == 1){
                const QUrl url = urls.first();
                const QString scheme = url.scheme();
                if(scheme == QStringLiteral("http") || scheme == QStringLiteral("https") ||
                   url == BLANK_URL || VanillaPage::IsPageUrl(url))
                    return url;
            }
        }
        return Page::CreateQueryUrl(trimmed, engine);
    }

    void LeaveStartPage(TreeBank *tb, ViewNode *self){
        if(!tb || !self || !VanillaPage::IsStartUrl(self->GetUrl())) return;
        if(tb->GetCurrentViewNode() == self) return;
        tb->CloseWithoutTrash(self);
    }

    QByteArray StartData(){
        QJsonObject data;
        data[QStringLiteral("title")] =
            QCoreApplication::translate("StartPage", "New tab");
        data[QStringLiteral("placeholder")] =
            QCoreApplication::translate("StartPage", "Search or type a web address");
        data[QStringLiteral("translucent")] = Application::EnableTransparentWindow();
        QJsonArray actions;
        for(const StartAction &entry : StartActions){
            QJsonObject action;
            const QString key = QLatin1String(entry.name);
            action[QStringLiteral("name")] = key;
            action[QStringLiteral("text")] = QCoreApplication::translate("StartPage", entry.label);
            if(entry.icon){
                const QString icon = QLatin1String(entry.icon);
                action[QStringLiteral("icon")] = DataUrl(icon);
                action[QStringLiteral("icon2x")] =
                    DataUrl(QString(icon).replace(QStringLiteral(".png"), QStringLiteral("@2x.png")));
            }
            actions.append(action);
        }
        data[QStringLiteral("actions")] = actions;
        QJsonArray engines;
        for(const QString &key : Page::GetSearchEngineMap().keys())
            engines.append(key);
        data[QStringLiteral("engines")] = engines;
        data[QStringLiteral("engine")] = PrimarySearchEngineKey();
        const StartTabList recent = StartTabs(QString(), CurrentTreeBank());
        data[QStringLiteral("recent")] = TabsJson(recent.tabs);
        data[QStringLiteral("more")] = recent.more;
        data[QStringLiteral("recentTitle")] =
            QCoreApplication::translate("StartPage", "Recent tabs");
        data[QStringLiteral("foundTitle")] =
            QCoreApplication::translate("StartPage", "Tabs found");
        data[QStringLiteral("closeTitle")] =
            QCoreApplication::translate("StartPage", "Move to trash");
        data[QStringLiteral("moreTitle")] =
            QCoreApplication::translate("StartPage", "Show more");
        data[QStringLiteral("closedBadge")] =
            QCoreApplication::translate("StartPage", "In the trash");
        data[QStringLiteral("restoreTitle")] =
            QCoreApplication::translate("StartPage", "Restore");
        data[QStringLiteral("historyTitle")] =
            QCoreApplication::translate("StartPage", "Tab history");
        data[QStringLiteral("todayLabel")] =
            QCoreApplication::translate("StartPage", "Today");
        data[QStringLiteral("yesterdayLabel")] =
            QCoreApplication::translate("StartPage", "Yesterday");
        data[QStringLiteral("deleteTitle")] =
            QCoreApplication::translate("StartPage", "Delete");
        data[QStringLiteral("openTitle")] =
            QCoreApplication::translate("StartPage", "Open");
        data[QStringLiteral("backTitle")] =
            QCoreApplication::translate("StartPage", "Back");
        data[QStringLiteral("restoreHint")] =
            QCoreApplication::translate("StartPage", "Click to restore");
        QByteArray json = Compact(data);
        json.replace("<", "\\u003c");
        return json;
    }

    VanillaPageResponse Start(const QUrl &url, const QByteArray &method,
                              const QUrl &initiator){
        const QString path = url.path();

        if(path.startsWith(QStringLiteral("/api/"))){
            if(!initiator.isEmpty() && !VanillaPage::IsStartUrl(initiator))
                return Failure(403);
            const QString endpoint = path.mid(5);
            const QUrlQuery query(url);
            if(endpoint == QStringLiteral("resolve")){
                if(method != QByteArrayLiteral("GET")) return Failure(403);
                QJsonObject answer;
                answer[QStringLiteral("url")] = ResolveStartQuery
                    (query.queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded),
                     query.queryItemValue(QStringLiteral("engine"), QUrl::FullyDecoded))
                    .toString(QUrl::FullyEncoded);
                return Json(Compact(answer));
            }
            if(endpoint == QStringLiteral("tabs")){
                if(method != QByteArrayLiteral("GET")) return Failure(403);
                QJsonObject answer;
                const StartTabList tabs = query.queryItemValue(QStringLiteral("view")) == QStringLiteral("history")
                    ? HistoryTabs(CurrentTreeBank(),
                                  query.queryItemValue(QStringLiteral("offset")).toInt(),
                                  query.queryItemValue(QStringLiteral("limit")).toInt(),
                                  query.queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded))
                    : StartTabs(query.queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded),
                                CurrentTreeBank(),
                                query.queryItemValue(QStringLiteral("limit")).toInt());
                answer[QStringLiteral("tabs")] = TabsJson(tabs.tabs);
                answer[QStringLiteral("more")] = tabs.more;
                return Json(Compact(answer));
            }
            if(endpoint == QStringLiteral("thumb")){
                if(method != QByteArrayLiteral("GET")) return Failure(403);
                return Thumbnail(ViewNodeOfSerial
                    (query.queryItemValue(QStringLiteral("node")).toULongLong(), true));
            }
            if(endpoint == QStringLiteral("icon")){
                if(method != QByteArrayLiteral("GET")) return Failure(403);
                return Icon(ViewNodeOfSerial
                    (query.queryItemValue(QStringLiteral("node")).toULongLong(), true));
            }
            if(endpoint == QStringLiteral("discard")){
                if(method != QByteArrayLiteral("POST")) return Failure(403);
                if(!VanillaPage::IsStartUrl(initiator)) return Failure(403);
                const QStringList asked = query.queryItemValue(QStringLiteral("nodes"), QUrl::FullyDecoded)
                    .split(QLatin1Char(','), Qt::SkipEmptyParts);
                if(asked.isEmpty() || asked.size() > HistoryLimit) return Failure(400);
                const QHash<quint64, ViewNode*> known = ViewNodesBySerial(true);
                QList<quint64> serials;
                QSet<quint64> seen;
                for(const QString &one : asked){
                    const quint64 serial = one.toULongLong();
                    ViewNode *vn = known.value(serial);
                    if(!vn || !TreeBank::IsTrash(vn)) return Failure(400);
                    if(!seen.contains(serial)){ seen << serial; serials << serial; }
                }
                for(quint64 serial : serials) PendingCloses[serial]++;
                TreeBank::DiscardFromTrashAsked
                    (serials, serials.size(),
                     QCoreApplication::translate("StartPage", "Tab history"),
                     [serials](){
                         for(quint64 serial : serials)
                             if(--PendingCloses[serial] <= 0) PendingCloses.remove(serial);
                     });
                return Json(QByteArrayLiteral("{}"));
            }
            if(endpoint == QStringLiteral("action")){
                if(method != QByteArrayLiteral("POST")) return Failure(403);
                if(!VanillaPage::IsStartUrl(initiator)) return Failure(403);
                const QString name = query.queryItemValue(QStringLiteral("name"));
                if(!IsStartAction(name)) return Failure(400);
                const StartOrigin origin = TakeStartOrigin();
                QTimer::singleShot(0, qApp, [name, origin](){
                    TreeBank *tb = origin.bank;
                    if(!tb) return;
                    if(name != QStringLiteral("NewWindow")){
                        tb->TriggerAction(name);
                        LeaveStartPage(tb, ViewNodeOfSerial(origin.serial));
                        return;
                    }
                    if(ViewNode *self = StillShowingStartPage(origin)){
                        if(ViewNode *next = tb->FindCurrentAfterDelete(self->GetParent(), self)){
                            tb->SetCurrent(next);
                            LeaveStartPage(tb, self);
                        }
                    }
                    tb->TriggerAction(QStringLiteral("NewWindow"));
                });
                return Json(QByteArrayLiteral("{}"));
            }
            if(endpoint == QStringLiteral("open")){
                if(method != QByteArrayLiteral("POST")) return Failure(403);
                if(!VanillaPage::IsStartUrl(initiator)) return Failure(403);
                const quint64 serial = query.queryItemValue(QStringLiteral("node")).toULongLong();
                if(!ViewNodeOfSerial(serial, true)) return Failure(400);
                const StartOrigin origin = TakeStartOrigin();
                TreeBank::WhenItMayChange([serial, origin](){
                    TreeBank *tb = origin.bank;
                    ViewNode *target = ViewNodeOfSerial(serial, true);
                    if(!tb || !TreeBank::IsLive(tb) || !target) return;
                    ViewNode *self = StillShowingStartPage(origin);
                    if(TreeBank::IsTrash(target)) TreeBank::RestoreBeside(target, self);
                    const bool leaving = self && self != target;
                    if(!tb->SetCurrent(target)) return;
                    if(leaving)
                        if(ViewNode *page = ViewNodeOfSerial(origin.serial)) LeaveStartPage(tb, page);
                });
                return Json(QByteArrayLiteral("{}"));
            }
            if(endpoint == QStringLiteral("close")){
                if(method != QByteArrayLiteral("POST")) return Failure(403);
                if(!VanillaPage::IsStartUrl(initiator)) return Failure(403);
                const quint64 serial = query.queryItemValue(QStringLiteral("node")).toULongLong();
                if(!ViewNodeOfSerial(serial)) return Failure(400);
                const StartOrigin origin = TakeStartOrigin();
                PendingCloses[serial]++;
                TreeBank::WhenItMayChange([serial, origin](){
                    if(--PendingCloses[serial] <= 0) PendingCloses.remove(serial);
                    TreeBank *tb = origin.bank;
                    ViewNode *target = ViewNodeOfSerial(serial);
                    if(!tb || !TreeBank::IsLive(tb) || !target) return;
                    if(TreeBank::WinIndex(target)) return;
                    tb->DeleteNode(target);
                });
                return Json(QByteArrayLiteral("{}"));
            }
            return Failure(404);
        }

        if(!path.isEmpty() && path != QStringLiteral("/")) return Failure(404);

        QFile file(QStringLiteral(":/resources/start/start.html"));
        if(!file.open(QIODevice::ReadOnly)) return Failure(404);
        QByteArray html = file.readAll();
        html.replace("<title>New tab</title>",
                     QByteArrayLiteral("<title>") +
                     QCoreApplication::translate("StartPage", "New tab").toHtmlEscaped().toUtf8() +
                     QByteArrayLiteral("</title>"));
        html.replace("<!--palette-->",
                     QByteArrayLiteral("<style>\n") + Theme::PageVariables() +
                     QByteArrayLiteral("</style>"));
        html.replace("<!--data-->",
                     QByteArrayLiteral("<script id=\"start\" type=\"application/json\">") +
                     StartData() + QByteArrayLiteral("</script>"));
        return Ok(QByteArrayLiteral("text/html"), html);
    }
}

QUrl VanillaPage::StartUrl(){
    return QUrl(VANILLA_SCHEME + QStringLiteral("://") +
                QLatin1String(START_HOST) + QStringLiteral("/"));
}

bool VanillaPage::IsStartUrl(const QUrl &url){
    return url.scheme() == QLatin1String(SCHEME) &&
           url.host()   == QLatin1String(START_HOST);
}

namespace {
    const char *const PermissionWords[] = {
        QT_TRANSLATE_NOOP("SettingsPage", "Notifications"),
        QT_TRANSLATE_NOOP("SettingsPage", "Location"),
        QT_TRANSLATE_NOOP("SettingsPage", "Microphone"),
        QT_TRANSLATE_NOOP("SettingsPage", "Camera"),
        QT_TRANSLATE_NOOP("SettingsPage", "Fonts on this device"),
        QT_TRANSLATE_NOOP("SettingsPage", "Clipboard (read and write)"),
        QT_TRANSLATE_NOOP("SettingsPage", "Mouse lock"),
        QT_TRANSLATE_NOOP("SettingsPage", "Screen"),
        QT_TRANSLATE_NOOP("SettingsPage", "Screen and audio"),
        QT_TRANSLATE_NOOP("SettingsPage", "Clipboard (read)"),
        QT_TRANSLATE_NOOP("SettingsPage", "Motion and other sensors"),
        QT_TRANSLATE_NOOP("SettingsPage", "Automatic downloads"),
        QT_TRANSLATE_NOOP("SettingsPage", "Files and folders"),
        QT_TRANSLATE_NOOP("SettingsPage", "Autoplay"),
        QT_TRANSLATE_NOOP("SettingsPage", "MIDI devices"),
        QT_TRANSLATE_NOOP("SettingsPage", "Window management"),
        QT_TRANSLATE_NOOP("SettingsPage", "Persistent storage"),
    };
    const char *const PermissionScopes[] = {
        "", "", "", "", "",
        "engine", "engine", "engine", "engine",
        "edge", "edge", "edge", "edge", "edge", "edge", "edge", "edge",
    };
    static_assert(sizeof(PermissionWords) == sizeof(PermissionScopes), "a scope for every kind");
}

QString VanillaPage::PermissionKindLabel(const QString &kind){
    const int index = PermissionTable::Kinds().indexOf(kind);
    Q_ASSERT(PermissionTable::Kinds().length() == int(sizeof(PermissionWords) / sizeof(PermissionWords[0])));
    if(index < 0) return QString();
    return QCoreApplication::translate("SettingsPage", PermissionWords[index]);
}

QString VanillaPage::PermissionKindScope(const QString &kind){
    const int index = PermissionTable::Kinds().indexOf(kind);
    return index < 0 ? QString() : QLatin1String(PermissionScopes[index]);
}

QUrl VanillaPage::SettingsUrl(){
    return QUrl(VANILLA_SCHEME + QStringLiteral("://") +
                QLatin1String(HOST) + QStringLiteral("/"));
}

bool VanillaPage::IsRecordingKey(){
    return !s_KeyRecording.hasExpired();
}

bool VanillaPage::IsSettingsUrl(const QUrl &url){
    return url.scheme() == QLatin1String(SCHEME) &&
           url.host()   == QLatin1String(HOST);
}

bool VanillaPage::IsTranslucentPage(const QUrl &url){
    return IsStartUrl(url) || IsSettingsUrl(url);
}

static QUrl DocumentOf(const QUrl &url){
    QUrl document = url.adjusted(QUrl::RemoveFragment);
    if(document.path().isEmpty()) document.setPath(QStringLiteral("/"));
    return document;
}

bool VanillaPage::SameDocument(const QUrl &a, const QUrl &b){
    return DocumentOf(a) == DocumentOf(b);
}

QByteArray VanillaPage::WithEmbeddedSchema(const QByteArray &html, const QByteArray &schemaJson){
    const QByteArray script = QByteArrayLiteral("<script src=\"/settings.js\">");
    const int at = html.indexOf(script);
    if(at < 0) return html;
    QByteArray json = schemaJson;
    json.replace("<", "\\u003c");
    QByteArray result = html;
    result.insert(at, QByteArrayLiteral("<script id=\"schema\" type=\"application/json\">")
                      + json + QByteArrayLiteral("</script>\n    "));
    return result;
}

QByteArray VanillaPage::WithInlineResources(const QByteArray &html, const QByteArray &css,
                                            const QByteArray &js){
    QByteArray result = html;
    const QByteArray link = QByteArrayLiteral("<link rel=\"stylesheet\" href=\"/settings.css\">");
    const int atLink = result.indexOf(link);
    if(atLink >= 0 && !css.contains("</style"))
        result.replace(atLink, link.size(),
                       QByteArrayLiteral("<style>\n") + css + QByteArrayLiteral("</style>"));
    const QByteArray script = QByteArrayLiteral("<script src=\"/settings.js\"></script>");
    const int atScript = result.indexOf(script);
    if(atScript >= 0 && !js.contains("</script"))
        result.replace(atScript, script.size(),
                       QByteArrayLiteral("<script>\n") + js + QByteArrayLiteral("</script>"));
    return result;
}

QByteArray VanillaPage::WithInlinePalette(const QByteArray &html){
    const QByteArray head = QByteArrayLiteral("<head>");
    const int at = html.indexOf(head);
    if(at < 0) return html;
    QByteArray inlined = html;
    inlined.insert(at + head.size(),
                   QByteArrayLiteral("\n    <style>\n") + Theme::PageColorVariables() +
                   QByteArrayLiteral("html { background: var(--bg); }\n    </style>"));
    return inlined;
}

bool VanillaPage::IsPageUrl(const QUrl &url){
    return IsSettingsUrl(url) || DirectoryPage::IsPageUrl(url) || IsStartUrl(url);
}

QUrl VanillaPage::InitiatorFromHeaders(const QString &origin, const QString &referer,
                                       const QUrl &viewUrl, bool documentPost){
    if(!origin.isEmpty())  return QUrl(origin);
    if(!referer.isEmpty()) return QUrl(referer);
    if(documentPost) return QUrl(QStringLiteral("form://unnamed"));
    return IsPageUrl(viewUrl) ? viewUrl : QUrl();
}

bool VanillaPage::ReadsBody(const QUrl &url, const QByteArray &method, const QUrl &initiator){
    if(method != QByteArrayLiteral("POST")) return false;
    if(!url.path().startsWith(QStringLiteral("/api/"))) return false;
    if(DirectoryPage::IsPageUrl(url)) return false;
    return IsSettingsUrl(url) && IsSettingsUrl(initiator);
}

VanillaPageResponse VanillaPage::Answer(const QUrl &url, const QByteArray &method,
                                        const QByteArray &body, const QUrl &initiator){
    if(!initiator.isEmpty() && !IsPageUrl(initiator)) return Failure(403);

    if(DirectoryPage::IsPageUrl(url))
        return Directory(url);

    if(IsStartUrl(url))
        return Start(url, method, initiator);

    if(!IsSettingsUrl(url)) return Failure(400);

    const QString path = url.path();

    if(path.startsWith(QStringLiteral("/api/"))){
        if(!initiator.isEmpty() && !IsSettingsUrl(initiator))
            return Failure(403);
        return SettingsApi(path.mid(5), method, body,
                           QUrlQuery(url).queryItemValue(QStringLiteral("node")));
    }

    if(path.isEmpty() || path == QStringLiteral("/")){
        VanillaPageResponse page = ResourcePage(QStringLiteral(":/resources/settings/settings.html"));
        if(page.m_Status == 200){
            page.m_Body = WithEmbeddedSchema(page.m_Body, SchemaJson());
            const VanillaPageResponse css = StyleSheet(QStringLiteral(":/resources/settings/settings.css"));
            const VanillaPageResponse js = Resource(QStringLiteral(":/resources/settings/settings.js"),
                                                    QByteArrayLiteral("text/javascript"));
            if(css.m_Status == 200 && js.m_Status == 200)
                page.m_Body = WithInlineResources(page.m_Body, css.m_Body, js.m_Body);
        }
        return page;
    }
    if(path == QStringLiteral("/settings.css"))
        return StyleSheet(QStringLiteral(":/resources/settings/settings.css"));
    if(path == QStringLiteral("/settings.js"))
        return Resource(QStringLiteral(":/resources/settings/settings.js"),
                        QByteArrayLiteral("text/javascript"));
    return Failure(404);
}

VanillaPageResponse VanillaPage::CompleteUserInteraction
    (const VanillaPageResponse &pending){
    if(!pending.NeedsUserInteraction()) return pending;

    VanillaPageResponse response = pending;
    response.m_UserInteraction = VanillaPageResponse::UserInteraction::None;

    switch(pending.m_UserInteraction){
    case VanillaPageResponse::UserInteraction::PickChromeExtensionDirectory: {
        const QString path = ModalDialog::GetExistingDirectory
            (QCoreApplication::translate("SettingsPage",
                                         "Select a Chrome extension folder"),
             ExtensionController::DefaultPickDirectory());
        QJsonObject reply;
        reply[QStringLiteral("path")] = path.isEmpty()
            ? QString() : QDir::toNativeSeparators(QDir::cleanPath(path));
        return Json(Compact(reply));
    }
    case VanillaPageResponse::UserInteraction::Deferred:
        response.m_Deferred = nullptr;
        return pending.m_Deferred ? Json(pending.m_Deferred()) : Failure(500);
    case VanillaPageResponse::UserInteraction::None:
        break;
    }
    return response;
}

#ifdef WEBENGINEVIEW

#include <QWebEngineUrlScheme>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineProfile>
#include <QQuickWebEngineProfile>

namespace {

    void ReplyToWebEngineJob(QWebEngineUrlRequestJob *job,
                             const VanillaPageResponse &response){
        switch(response.m_Status){
        case 200: {
            QBuffer *buffer = new QBuffer(job);
            buffer->setData(response.m_Body);
            buffer->open(QIODevice::ReadOnly);
            job->reply(response.m_ContentType, buffer);
            return;
        }
        case 403:
            job->fail(QWebEngineUrlRequestJob::RequestDenied);
            return;
        case 400:
            job->fail(QWebEngineUrlRequestJob::UrlInvalid);
            return;
        default:
            job->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }
    }

}

SettingsSchemeHandler::SettingsSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent)
{
}

SettingsSchemeHandler::~SettingsSchemeHandler(){}

QUrl SettingsSchemeHandler::SettingsUrl(){
    return VanillaPage::SettingsUrl();
}

bool SettingsSchemeHandler::IsSettingsUrl(const QUrl &url){
    return VanillaPage::IsSettingsUrl(url);
}

void SettingsSchemeHandler::RegisterScheme(){
    QWebEngineUrlScheme scheme{SCHEME};
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    scheme.setDefaultPort(QWebEngineUrlScheme::PortUnspecified);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme |
                    QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
}

void SettingsSchemeHandler::Install(QWebEngineProfile *profile){
    if(!profile) return;
    profile->installUrlSchemeHandler(SCHEME,
                                     new SettingsSchemeHandler(profile));
}

void SettingsSchemeHandler::Install(QQuickWebEngineProfile *profile){
    if(!profile) return;
    profile->installUrlSchemeHandler(SCHEME,
                                     new SettingsSchemeHandler(profile));
}

void SettingsSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job){
    QByteArray body;
    QIODevice *device = VanillaPage::ReadsBody(job->requestUrl(), job->requestMethod(), job->initiator())
        ? job->requestBody() : nullptr;
    if(device){
        if(!device->isOpen()) device->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        body.resize(2 * VanillaPage::BodyLimit);
        const qint64 size = device->read(body.data(), VanillaPage::BodyLimit);
        if(size >= VanillaPage::BodyLimit){
            job->fail(QWebEngineUrlRequestJob::RequestDenied);
            return;
        }
        body.truncate(size > 0 ? size : 0);
    }

    const VanillaPageResponse response = VanillaPage::Answer
        (job->requestUrl(), job->requestMethod(), body, job->initiator());

    if(response.NeedsUserInteraction()){
        QCoreApplication *application = QCoreApplication::instance();
        if(!application){
            job->fail(QWebEngineUrlRequestJob::RequestDenied);
            return;
        }
        Q_ASSERT(QThread::currentThread() == application->thread());

        QPointer<QWebEngineUrlRequestJob> held(job);
        QTimer::singleShot(0, application, [held, response](){
            if(!held) return;
            const VanillaPageResponse completed =
                VanillaPage::CompleteUserInteraction(response);
            if(!held) return;
            ReplyToWebEngineJob(held.data(), completed);
        });
        return;
    }
    ReplyToWebEngineJob(job, response);
}

#endif

namespace {
    QList<ViewNode*> FindUnder(ViewNode *root, const QString &query, TreeBank *asking, int limit){
        const QStringList words = query.split(QRegularExpression(QStringLiteral("\\s+"),
                                                                 QRegularExpression::UseUnicodePropertiesOption),
                                              Qt::SkipEmptyParts);
        return CollectTabs(root, words, asking ? asking->WinIndex() : 0).mid(0, limit);
    }
}

QList<ViewNode*> VanillaPage::FindTabs(const QString &query, TreeBank *asking, int limit){
    return FindUnder(TreeBank::GetViewRoot(), query, asking, limit);
}

QList<ViewNode*> VanillaPage::FindTrashedTabs(const QString &query, TreeBank *asking, int limit){
    return FindUnder(TreeBank::GetTrashRoot(), query, asking, limit);
}

ViewNode *VanillaPage::TabOfSerial(quint64 serial){
    return ViewNodeOfSerial(serial, true);
}
