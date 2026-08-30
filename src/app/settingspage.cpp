#include "switch.hpp"
#include "const.hpp"

#include "settingspage.hpp"

#include <QBuffer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include "application.hpp"
#include "directorypage.hpp"
#include "settingsschema.hpp"
#include "theme.hpp"

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

    VanillaPageResponse Page(const QString &path){
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) return Failure(404);
        return Ok(QByteArrayLiteral("text/html"),
                  VanillaPage::WithInlinePalette(file.readAll()));
    }

    VanillaPageResponse SettingsApi(const QString &endpoint,
                                    const QByteArray &method,
                                    const QByteArray &body){
        if(endpoint == QStringLiteral("schema")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(SettingsSchema::Describe()));
        }

        const bool set   = endpoint == QStringLiteral("set");
        const bool reset = endpoint == QStringLiteral("reset");
        if(!set && !reset) return Failure(404);
        if(method != QByteArrayLiteral("POST")) return Failure(403);

        const QJsonObject request = QJsonDocument::fromJson(body).object();
        const QString key = request[QStringLiteral("key")].toString();

        const SettingsSchema::Item *item = SettingsSchema::Find(key);
        if(!item){
            return Json(Error(QStringLiteral("unknown key: \"%1\"").arg(key)));
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

        QJsonObject reply;
        reply[QStringLiteral("key")] = key;
        reply[QStringLiteral("value")] =
            SettingsSchema::ToJson(*item, Application::GlobalSettings()
                                   .value(key, SettingsSchema::Fallback(*item)));
        return Json(Compact(reply));
    }

    VanillaPageResponse DirectoryApi(const QString &endpoint,
                                     const QByteArray &method,
                                     const QByteArray &body){
        if(endpoint == QStringLiteral("tree")){
            if(method != QByteArrayLiteral("GET")) return Failure(403);
            return Json(Compact(DirectoryPage::Describe()));
        }

        if(endpoint != QStringLiteral("set")) return Failure(404);
        if(method != QByteArrayLiteral("POST")) return Failure(403);

        return Json(DirectoryPage::HandleSet(QJsonDocument::fromJson(body).object()));
    }

    VanillaPageResponse Directory(const QUrl &url, const QByteArray &method,
                                  const QByteArray &body, const QUrl &initiator){
        const QString path = url.path();

        if(path.startsWith(QStringLiteral("/api/"))){
            if(!initiator.isEmpty() && !DirectoryPage::IsPageUrl(initiator))
                return Failure(403);
            return DirectoryApi(path.mid(5), method, body);
        }

        if(path.isEmpty() || path == QStringLiteral("/"))
            return Page(QStringLiteral(":/resources/directory/directory.html"));
        if(path == QStringLiteral("/directory.css"))
            return StyleSheet(QStringLiteral(":/resources/directory/directory.css"));
        if(path == QStringLiteral("/directory.js"))
            return Resource(QStringLiteral(":/resources/directory/directory.js"),
                            QByteArrayLiteral("text/javascript"));
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

VanillaPageResponse VanillaPage::Answer(const QUrl &url, const QByteArray &method,
                                        const QByteArray &body, const QUrl &initiator){
    if(DirectoryPage::IsPageUrl(url))
        return Directory(url, method, body, initiator);

    if(!IsSettingsUrl(url)) return Failure(400);

    const QString path = url.path();

    if(path.startsWith(QStringLiteral("/api/"))){
        if(!initiator.isEmpty() && !IsSettingsUrl(initiator))
            return Failure(403);
        return SettingsApi(path.mid(5), method, body);
    }

    if(path.isEmpty() || path == QStringLiteral("/"))
        return Page(QStringLiteral(":/resources/settings/settings.html"));
    if(path == QStringLiteral("/settings.css"))
        return StyleSheet(QStringLiteral(":/resources/settings/settings.css"));
    if(path == QStringLiteral("/settings.js"))
        return Resource(QStringLiteral(":/resources/settings/settings.js"),
                        QByteArrayLiteral("text/javascript"));
    return Failure(404);
}

#ifdef WEBENGINEVIEW

#include <QWebEngineUrlScheme>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineProfile>
#include <QQuickWebEngineProfile>

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
    if(QIODevice *device = job->requestBody()){
        if(!device->isOpen()) device->open(QIODevice::ReadOnly);
        body = device->readAll();
    }

    const VanillaPageResponse response = VanillaPage::Answer
        (job->requestUrl(), job->requestMethod(), body, job->initiator());

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

#endif
