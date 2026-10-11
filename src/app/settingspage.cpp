#include "switch.hpp"
#include "const.hpp"

#include "settingspage.hpp"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

#include "application.hpp"
#include "directorypage.hpp"
#include "settingsschema.hpp"
#include "inputmapschema.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "gadgets.hpp"
#include "dialog.hpp"
#include "extensioncontroller.hpp"
#include "theme.hpp"
#include "networkcontroller.hpp"
#include "passwordstore.hpp"
#include "downloadhistory.hpp"

namespace {

    const QByteArray SCHEME = VANILLA_SCHEME.toLatin1();
    const char HOST[] = "settings";

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
        strings[QStringLiteral("edge")]         =
            QCoreApplication::translate("SettingsPage", "Passwords an Edge view saved are not listed here. "
                   "\"Remove all\" removes them too.");
        strings[QStringLiteral("unavailable")]  =
            QCoreApplication::translate("SettingsPage", "This build does not save passwords in a WebEngine view.");

        QJsonArray entries;
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
        }
#endif
        QJsonObject reply;
        reply[QStringLiteral("strings")] = strings;
        reply[QStringLiteral("entries")] = entries;
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
        strings[QStringLiteral("missing")]     = QCoreApplication::translate("SettingsPage", "The file is not there any more.");

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
        return Deferred([endpoint, id](){ return DownloadAction(endpoint, id);});
    }

    VanillaPageResponse DirectorySectionApi(const QString &endpoint, const QByteArray &method,
                                            const QByteArray &body, const QString &node);

    VanillaPageResponse SettingsApi(const QString &endpoint,
                                    const QByteArray &method,
                                    const QByteArray &body,
                                    const QString &node){
        if(endpoint == QStringLiteral("directory") ||
           endpoint == QStringLiteral("directory-set"))
            return DirectorySectionApi(endpoint, method, body, node);

        if(endpoint == QStringLiteral("schema")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            QJsonObject schema = SettingsSchema::Describe();
            schema[QStringLiteral("inputs")] =
                InputMapSchema::Describe(Application::GlobalSettings());
            return Json(Compact(schema));
        }

        if(endpoint == QStringLiteral("inputs") ||
           endpoint == QStringLiteral("input-set") ||
           endpoint == QStringLiteral("input-reset"))
            return InputApi(endpoint, method, body);

        if(endpoint == QStringLiteral("passwords") ||
           endpoint == QStringLiteral("passwords-remove") ||
           endpoint == QStringLiteral("passwords-remove-all"))
            return PasswordsApi(endpoint, method, body);

        if(endpoint == QStringLiteral("downloads") ||
           endpoint == QStringLiteral("downloads-open") ||
           endpoint == QStringLiteral("downloads-show") ||
           endpoint == QStringLiteral("downloads-cancel") ||
           endpoint == QStringLiteral("downloads-remove") ||
           endpoint == QStringLiteral("downloads-clear"))
            return DownloadsApi(endpoint, method, body);

        const bool set   = endpoint == QStringLiteral("set");
        const bool reset = endpoint == QStringLiteral("reset");
        const bool pick  = endpoint == QStringLiteral("pick-directory");
        if(!set && !reset && !pick) return Failure(404);
        if(method != QByteArrayLiteral("POST")) return Failure(403);

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        const QString key = request[QStringLiteral("key")].toString();

        const SettingsSchema::Item *item = SettingsSchema::Find(key);
        if(!item){
            return Json(Error(QStringLiteral("unknown key: \"%1\"").arg(key)));
        }

        if(pick){
            if(!item->picker ||
               QString::fromLatin1(item->picker) != QStringLiteral("chrome-extension"))
                return Json(Error(QStringLiteral("no directory picker for: \"%1\"")
                                  .arg(key)));

            VanillaPageResponse response = Json(QByteArrayLiteral("{}"));
            response.m_UserInteraction =
                VanillaPageResponse::UserInteraction::PickChromeExtensionDirectory;
            return response;
        }

        if(reset){
            Application::GlobalSettings().remove(key);
        } else {
            const QVariant value =
                SettingsSchema::FromJson(*item, request[QStringLiteral("value")]);
            if(!value.isValid()) return Json(Error(QStringLiteral("bad value")));
            Application::GlobalSettings().setValue(key, value);
        }

        Application::Reconfigure();
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

QUrl VanillaPage::SettingsUrl(){
    return QUrl(VANILLA_SCHEME + QStringLiteral("://") +
                QLatin1String(HOST) + QStringLiteral("/"));
}

bool VanillaPage::IsSettingsUrl(const QUrl &url){
    return url.scheme() == QLatin1String(SCHEME) &&
           url.host()   == QLatin1String(HOST);
}

static QUrl DocumentOf(const QUrl &url){
    QUrl document = url.adjusted(QUrl::RemoveFragment);
    if(document.path().isEmpty()) document.setPath(QStringLiteral("/"));
    return document;
}

bool VanillaPage::SameDocument(const QUrl &a, const QUrl &b){
    return DocumentOf(a) == DocumentOf(b);
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
    return IsSettingsUrl(url) || DirectoryPage::IsPageUrl(url);
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

    if(!IsSettingsUrl(url)) return Failure(400);

    const QString path = url.path();

    if(path.startsWith(QStringLiteral("/api/"))){
        if(!initiator.isEmpty() && !IsSettingsUrl(initiator))
            return Failure(403);
        return SettingsApi(path.mid(5), method, body,
                           QUrlQuery(url).queryItemValue(QStringLiteral("node")));
    }

    if(path.isEmpty() || path == QStringLiteral("/"))
        return ResourcePage(QStringLiteral(":/resources/settings/settings.html"));
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
