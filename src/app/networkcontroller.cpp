#include "switch.hpp"
#include "const.hpp"

#include "networkcontroller.hpp"
#include "downloadhistory.hpp"

#include <QPointer>

#include <QStringList>
#include <QNetworkProxy>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QSslConfiguration>
#include <QSslCipher>
#include <QSslCertificate>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QLocale>
#include <QSslSocket>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QAbstractButton>
#include <QRegularExpression>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include <QMimeDatabase>
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#  include <QQuickWebEngineProfile>
#  include <QQmlEngine>
#  include <QQmlContext>
#  include "webengineextensions.hpp"
#  include "extensionhost.hpp"
#  include <QtWebEngineCore/qtwebenginecoreglobal.h>
#  include "settingspage.hpp"
#  include <QWebEngineDownloadRequest>
#  include <QWebEngineNotification>
#  include <QWebEngineCookieStore>
#  if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#    include <QWebEnginePermission>
#  endif
#  if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0) && QT_CONFIG(webengine_extensions)
#    include <QWebEngineExtensionManager>
#    include <QWebEngineExtensionInfo>
#  endif
#  include <QWebEngineUrlRequestInfo>
#  include <QWebEngineUrlRequestInterceptor>
#  if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#    include <QWebEngineClientHints>
#  endif
#  include <QWebEngineScript>
#  include <QWebEngineScriptCollection>
#  include "webengineview.hpp"
#  include "webenginepage.hpp"
#  include "quickwebengineview.hpp"
#endif
#include "page.hpp"

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "saver.hpp"
#include "extensioncontroller.hpp"
#include "application.hpp"
#ifdef EDGEWEBVIEW
#  include "edgewebview.hpp"
#endif
#include "useragent.hpp"
#include "downloadname.hpp"
#include "certificatepolicy.hpp"
#include "fileexchange.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "dialog.hpp"
#include "view.hpp"
#include "passwordstore.hpp"
#include "webenginepasswords.hpp"
#include "sitepermissions.hpp"

const QList<QEvent::Type> NetworkAccessManager::m_EventTypes =
    QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease;

NetworkCookieJar::NetworkCookieJar()
    : QNetworkCookieJar(nullptr)
{
}

NetworkCookieJar::~NetworkCookieJar(){}

QString NetworkCookieJar::Identity(const QNetworkCookie &cookie){
    QString domain = cookie.domain().trimmed().toLower();
    while(domain.startsWith(QLatin1Char('.'))) domain.remove(0, 1);
    const QString path = cookie.path().isEmpty() ? QStringLiteral("/") : cookie.path();
    static const QString separator = QString(QChar(QChar::LineFeed));
    return QString::fromLatin1(cookie.name()) + separator + domain + separator + path;
}

void NetworkCookieJar::SetAllCookies(const QList<QNetworkCookie> &cookies){
    setAllCookies(cookies);

    if(m_Mirrored.isEmpty()) return;
    QHash<QString, QString> kept;
    foreach(const QNetworkCookie &cookie, cookies){
        const QString id = Identity(cookie);
        const QHash<QString, QString>::const_iterator it = m_Mirrored.constFind(id);
        if(it != m_Mirrored.constEnd()) kept.insert(id, it.value());
    }
    m_Mirrored = kept;
}

QList<QNetworkCookie> NetworkCookieJar::GetAllCookies(){
    return allCookies();
}

namespace {

    QList<QNetworkCookie> WithoutIdentity(const QList<QNetworkCookie> &cookies,
                                          const QString &identity){
        QList<QNetworkCookie> kept;
        foreach(const QNetworkCookie &cookie, cookies){
            if(NetworkCookieJar::Identity(cookie) == identity) continue;
            kept.append(cookie);
        }
        return kept;
    }
}

void NetworkCookieJar::MirrorCookie(const QString &source, const QNetworkCookie &cookie){
    const QString id = Identity(cookie);
    QList<QNetworkCookie> all = WithoutIdentity(allCookies(), id);
    all.append(cookie);
    setAllCookies(all);
    m_Mirrored.insert(id, source);
}

void NetworkCookieJar::UnmirrorCookie(const QString &source, const QNetworkCookie &cookie){
    const QString id = Identity(cookie);
    if(m_Mirrored.value(id) != source) return;
    setAllCookies(WithoutIdentity(allCookies(), id));
    m_Mirrored.remove(id);
}

namespace {

    QString ScopeKey(const QString &source, const QUrl &url){
        return source + QString(QChar(QChar::LineFeed)) + url.toString();
    }
}

quint64 NetworkCookieJar::BeginMirrorScope(const QString &source, const QUrl &url){
    const QString key = ScopeKey(source, url);
    const quint64 ticket = m_ScopeTicket.value(key) + 1;
    m_ScopeTicket.insert(key, ticket);
    return ticket;
}

void NetworkCookieJar::MirrorScope(const QString &source, const QUrl &url,
                                   quint64 ticket, const QList<QNetworkCookie> &fresh){
    if(m_ScopeTicket.value(ScopeKey(source, url)) != ticket) return;

    QSet<QString> arrived;
    foreach(const QNetworkCookie &cookie, fresh) arrived.insert(Identity(cookie));

    QSet<QString> inScope;
    foreach(const QNetworkCookie &cookie, cookiesForUrl(url))
        inScope.insert(Identity(cookie));

    QList<QNetworkCookie> kept;
    foreach(const QNetworkCookie &cookie, allCookies()){
        const QString id = Identity(cookie);
        if(arrived.contains(id)) continue;
        if(m_Mirrored.value(id) == source && inScope.contains(id)){
            m_Mirrored.remove(id);
            continue;
        }
        kept.append(cookie);
    }

    foreach(const QNetworkCookie &cookie, fresh){
        kept.append(cookie);
        m_Mirrored.insert(Identity(cookie), source);
    }

    setAllCookies(kept);
}

QList<QNetworkCookie> NetworkCookieJar::GetPersistableCookies(){
    if(m_Mirrored.isEmpty()) return allCookies();
    QList<QNetworkCookie> kept;
    foreach(const QNetworkCookie &cookie, allCookies()){
        if(m_Mirrored.contains(Identity(cookie))) continue;
        kept.append(cookie);
    }
    return kept;
}

bool NetworkCookieJar::insertCookie(const QNetworkCookie &cookie){
    const QString id = Identity(cookie);
    m_Mirrored.remove(id);

    const QList<QNetworkCookie> all = allCookies();
    const QList<QNetworkCookie> single = WithoutIdentity(all, id);
    if(single.count() != all.count()) setAllCookies(single);

    return QNetworkCookieJar::insertCookie(cookie);
}

bool NetworkCookieJar::updateCookie(const QNetworkCookie &cookie){
    if(!QNetworkCookieJar::updateCookie(cookie)) return false;
    m_Mirrored.remove(Identity(cookie));
    return true;
}

bool NetworkCookieJar::deleteCookie(const QNetworkCookie &cookie){
    if(!QNetworkCookieJar::deleteCookie(cookie)) return false;
    m_Mirrored.remove(Identity(cookie));
    return true;
}

#ifdef WEBENGINEVIEW
namespace {

    const char PROFILE_KEY_PROPERTY[] = "vanillaProfileKey";

    const char CREDENTIAL_SPACE_PROPERTY[] = "vanillaCredentialSpace";

    void SetProfileCache(QWebEngineProfile *profile, const QString &id){
        profile->setCachePath(Application::DataDirectory()
                              + QStringLiteral("webenginecache/")
                              + NetworkController::ProfileStorageName(id));
    }

    SharedProfile WrapProfile(QWebEngineProfile *profile){
        return SharedProfile(profile,
                             [](QWebEngineProfile *p){ ExtensionHost::CloseOffscreenOf(p); p->deleteLater();});
    }

}

RequestInterceptor::RequestInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
    UrlBlockRules::ReloadRules();
}

RequestInterceptor *RequestInterceptor::Instance(){
    static RequestInterceptor *instance = new RequestInterceptor();
    return instance;
}

RequestInterceptor *RequestInterceptor::PrivateInstance(){
    static RequestInterceptor *instance = [](){
        RequestInterceptor *made = new RequestInterceptor();
        made->m_AsksExtensionRules = false;
        return made;
    }();
    return instance;
}

Dnr::ResourceType RequestInterceptor::DnrTypeOf(QWebEngineUrlRequestInfo::ResourceType type){
    static_assert(QWebEngineUrlRequestInfo::ResourceTypeLast == QWebEngineUrlRequestInfo::ResourceTypeJson,
                  "a resource type was added: decide what an extension's rule calls it");
    switch(type){
    case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
    case QWebEngineUrlRequestInfo::ResourceTypeNavigationPreloadMainFrame: return Dnr::MainFrame;
    case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
    case QWebEngineUrlRequestInfo::ResourceTypeNavigationPreloadSubFrame:  return Dnr::SubFrame;
    case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:   return Dnr::Stylesheet;
    case QWebEngineUrlRequestInfo::ResourceTypeScript:
    case QWebEngineUrlRequestInfo::ResourceTypeWorker:
    case QWebEngineUrlRequestInfo::ResourceTypeSharedWorker:
    case QWebEngineUrlRequestInfo::ResourceTypeServiceWorker:
    case QWebEngineUrlRequestInfo::ResourceTypeJson:         return Dnr::Script;
    case QWebEngineUrlRequestInfo::ResourceTypeImage:
    case QWebEngineUrlRequestInfo::ResourceTypeFavicon:      return Dnr::Image;
    case QWebEngineUrlRequestInfo::ResourceTypeFontResource: return Dnr::Font;
    case QWebEngineUrlRequestInfo::ResourceTypeObject:
    case QWebEngineUrlRequestInfo::ResourceTypePluginResource: return Dnr::Object;
    case QWebEngineUrlRequestInfo::ResourceTypeMedia:        return Dnr::Media;
    case QWebEngineUrlRequestInfo::ResourceTypeXhr:          return Dnr::XmlHttpRequest;
    case QWebEngineUrlRequestInfo::ResourceTypePing:         return Dnr::Ping;
    case QWebEngineUrlRequestInfo::ResourceTypeCspReport:    return Dnr::CspReport;
    case QWebEngineUrlRequestInfo::ResourceTypeWebSocket:    return Dnr::WebSocket;
    default:                                                 return Dnr::Other;
    }
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info){
    if(info.requestUrl().scheme() == QStringLiteral("vanilla-extension")) return;

    if(UrlBlockRules::SendDoNotTrack()){
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));
        info.setHttpHeader(QByteArrayLiteral("Sec-GPC"), QByteArrayLiteral("1"));
    }

    const QString requested = info.requestUrl().toString();

    if(m_AsksExtensionRules) AskExtensionRules(info);

    if(info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) return;

    if(UrlBlockRules::IsBlocked(requested)) info.block(true);
}

void RequestInterceptor::AskExtensionRules(QWebEngineUrlRequestInfo &info){
    Q_ASSERT(!QCoreApplication::instance() ||
             QThread::currentThread() == QCoreApplication::instance()->thread());
    quint64 generation = 0;
    const QSharedPointer<const Dnr::Rules> rules = ExtensionNetRules::Current(&generation);
    if(rules->IsEmpty()) return;

    Dnr::Request request;
    request.url = info.requestUrl();
    request.initiator = info.initiator();
    request.type = DnrTypeOf(info.resourceType());
    request.method = info.requestMethod();

    if(request.type != Dnr::MainFrame){
        const QString key = Dnr::Framed::KeyOf(info.firstPartyUrl());
        if(!key.isEmpty() && !m_Framed.Find(generation, key, &request.frameAllows)){
            Dnr::Request document;
            document.url = info.firstPartyUrl().adjusted(QUrl::RemoveFragment);
            document.type = Dnr::MainFrame;
            document.method = QByteArrayLiteral("GET");
            request.frameAllows = rules->Evaluate(document).frameAllows;
            m_Framed.Put(generation, key, request.frameAllows);
        }
    }

    const Dnr::Decision decision = rules->Evaluate(request);
    if(decision.kind == Dnr::Decision::Block) info.block(true);
    else if(decision.kind == Dnr::Decision::Redirect) info.redirect(decision.redirect);
}
#endif

QList<QRegularExpression> UrlBlockRules::m_Blocked = QList<QRegularExpression>();
bool UrlBlockRules::m_SendDoNotTrack = false;

QList<QRegularExpression> UrlBlockRules::Compile(const QStringList &patterns){
    QList<QRegularExpression> rules;
    foreach(const QString &pattern, patterns){
        const QString trimmed = pattern.trimmed();
        if(trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) continue;

        QRegularExpression re(QRegularExpression::wildcardToRegularExpression
                              (trimmed, QRegularExpression::UnanchoredWildcardConversion));
        re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if(re.isValid()) rules << re;
        else qWarning() << "ignoring" << trimmed
                        << "in network/@BlockedUrlPatterns: not a pattern";
    }
    return rules;
}

bool UrlBlockRules::Matches(const QList<QRegularExpression> &rules, const QString &url){
    foreach(const QRegularExpression &re, rules)
        if(re.match(url).hasMatch()) return true;
    return false;
}

namespace {
    QReadWriteLock &BlockRulesLock(){
        static QReadWriteLock lock;
        return lock;
    }
}

void UrlBlockRules::ReloadRules(){
    Settings &s = Application::GlobalSettings();

    const bool doNotTrack =
        s.value(QStringLiteral("network/@SendDoNotTrack"), false).value<bool>();
    const QList<QRegularExpression> compiled = Compile
        (s.value(QStringLiteral("network/@BlockedUrlPatterns"),
                 QStringList()).value<QStringList>());

    QWriteLocker locker(&BlockRulesLock());
    m_SendDoNotTrack = doNotTrack;
    m_Blocked = compiled;
}

namespace {
    struct NetRulesState {
        QReadWriteLock lock;
        QSharedPointer<const Dnr::Rules> current{new Dnr::Rules()};
        QStringList key;
        quint64 asked = 0;
        quint64 generation = 0;
        QThreadPool pool;
    };
    NetRulesState &NetRules(){
        static NetRulesState *state = [](){
            NetRulesState *made = new NetRulesState();
            made->pool.setMaxThreadCount(1);
            qAddPostRoutine([](){ NetRules().pool.waitForDone();});
            return made;
        }();
        return *state;
    }

    QStringList KeyOf(const QList<ExtensionNetRules::Source> &sources){
        QStringList key;
        foreach(const ExtensionNetRules::Source &source, sources){
            key << QStringLiteral("id:") + source.id;
            key << QStringLiteral("hosts:") + QString::number(source.hostAccessAlways)
                   + QLatin1Char('|') + source.hostPermissions.join(QLatin1Char(' '));
            key << QStringLiteral("session:") + QString::fromLatin1(QCryptographicHash::hash(source.session, QCryptographicHash::Sha1).toHex())
                 << QStringLiteral("dynamic:") + QString::fromLatin1(QCryptographicHash::hash(source.dynamic, QCryptographicHash::Sha1).toHex());
            foreach(const QString &file, source.files){
                const QFileInfo info(file);
                key << file + QLatin1Char('|') + QString::number(info.size())
                     + QLatin1Char('|') + QString::number(info.lastModified().toMSecsSinceEpoch());
            }
        }
        return key;
    }

    void ReadAndPublish(const QList<ExtensionNetRules::Source> &sources, quint64 ticket){
        {
            QReadLocker locker(&NetRules().lock);
            if(ticket != NetRules().asked) return;
        }
        QSharedPointer<Dnr::Rules> rules(new Dnr::Rules());
        Dnr::Skipped skipped;
        foreach(const ExtensionNetRules::Source &source, sources){
            rules->SetHostAccess(source.id, source.hostPermissions, source.hostAccessAlways);
            if(!source.session.isEmpty()) rules->AddRuleset(source.id, source.session, &skipped);
            if(!source.dynamic.isEmpty()) rules->AddRuleset(source.id, source.dynamic, &skipped);
            foreach(const QString &path, source.files){
                QFile file(path);
                if(!file.open(QIODevice::ReadOnly)){
                    skipped.malformed++;
                    continue;
                }
                rules->AddRuleset(source.id, file.readAll(), &skipped);
            }
        }

        NetRulesState &state = NetRules();
        QSharedPointer<const Dnr::Rules> replaced;
        QWriteLocker locker(&state.lock);
        if(ticket != state.asked) return;
        replaced = state.current;
        state.current = rules;
        state.generation++;
        locker.unlock();
        replaced.clear();

        if(QCoreApplication::instance() && (rules->Count() || skipped.Total()))
            qInfo() << "extension rules:" << rules->Count() << "in force;"
                    << skipped.Total() << "not evaluated (headers" << skipped.modifyHeaders
                    << "computed redirects" << skipped.computedRedirect
                    << "tab bound" << skipped.tabBound << "bad regex" << skipped.badRegex
                    << "unread conditions" << skipped.unreadCondition
                    << "unreadable" << skipped.malformed << ")";
    }
}

void ExtensionNetRules::Reload(){
    QList<Source> sources;
    foreach(const ExtensionRuleFiles &each, ExtensionController::EnabledRuleFiles()){
        Source source;
        source.id = each.id;
        source.files = each.files;
        source.session = each.session;
        source.dynamic = each.dynamic;
        source.hostPermissions = each.hostPermissions;
        source.hostAccessAlways = each.hostAccessAlways;
        sources << source;
    }
    Load(sources);
}

void ExtensionNetRules::Load(const QList<Source> &sources, bool async){
    Q_ASSERT(!QCoreApplication::instance() ||
             QThread::currentThread() == QCoreApplication::instance()->thread());
    NetRulesState &state = NetRules();
    const QStringList key = KeyOf(sources);
    quint64 ticket = 0;
    {
        QWriteLocker locker(&state.lock);
        if(key == state.key) return;
        state.key = key;
        ticket = ++state.asked;
    }
    if(async) state.pool.start([sources, ticket](){ ReadAndPublish(sources, ticket); });
    else ReadAndPublish(sources, ticket);
}

QSharedPointer<const Dnr::Rules> ExtensionNetRules::Current(quint64 *generation){
    NetRulesState &state = NetRules();
    QReadLocker locker(&state.lock);
    if(generation) *generation = state.generation;
    return state.current;
}

void ExtensionNetRules::WaitForLoads(){
    NetRules().pool.waitForDone();
}

bool UrlBlockRules::SendDoNotTrack(){
    QReadLocker locker(&BlockRulesLock());
    return m_SendDoNotTrack;
}

bool UrlBlockRules::IsBlocked(const QString &url){
    QList<QRegularExpression> rules;
    {
        QReadLocker locker(&BlockRulesLock());
        if(m_Blocked.isEmpty()) return false;
        rules = m_Blocked;
    }
    return Matches(rules, url);
}

#ifdef WEBENGINEVIEW
static void MirrorProfileCookies(QWebEngineCookieStore *store, const QString &id,
                                 const QString &source, QObject *context,
                                 QPointer<NetworkAccessManager> owner);
#endif

NetworkAccessManager::NetworkAccessManager(QString id)
    : QNetworkAccessManager(nullptr)
    , m_Id(id)
    , m_UserAgent(QString())
    , m_UserAgentBrand(QString())
    , m_UserAgentFullVersion(QString())
    , m_SslProtocol(QSsl::UnknownProtocol)
#ifdef WEBENGINEVIEW
    , m_Profile(WrapProfile(new QWebEngineProfile(NetworkController::ProfileStorageName(id))))
#endif
{
#ifdef WEBENGINEVIEW
    SetupProfile(m_Profile.get());

    MirrorProfileCookies(m_Profile->cookieStore(), m_Id,
                         QStringLiteral("web:") + m_Profile->storageName(),
                         this, this);
#endif
    connect(this, &NetworkAccessManager::authenticationRequired,
            this, &NetworkAccessManager::HandleAuthentication);
#ifndef QT_NO_NETWORKPROXY
    connect(this, &NetworkAccessManager::proxyAuthenticationRequired,
            this, &NetworkAccessManager::HandleProxyAuthentication);
#endif
}

NetworkAccessManager::~NetworkAccessManager(){
}

void NetworkAccessManager::HandleError(QNetworkReply::NetworkError code){
    Q_UNUSED(code)
}

namespace {

    QString SslErrorLines(const QList<QSslError> &errors){
        QStringList lines;
        foreach(const QSslError &error, errors)
            lines << QStringLiteral("Ssl error : ") + error.errorString();
        return lines.join(QStringLiteral("\n"));
    }
}

void NetworkAccessManager::HandleSslErrors(const QList<QSslError> &errors){
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if(!reply){
        return;
    }
    const bool offTheRecord = IsOffTheRecord(reply->request());

    Application::AskSslErrorPolicyIfNeed();

    switch(Application::GetSslErrorPolicy()){
    case Application::BlockAccess:
        break;
    case Application::IgnoreSslErrors:
        reply->ignoreSslErrors();
        break;
    case Application::AskForEachAccess:{

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("SSL errors."));
        dialog->SetCaption(tr("SSL errors."));
        dialog->SetInformativeText(tr("Ignore errors in this access?"));

        dialog->SetDetailedText(SslErrorLines(errors));
        dialog->SetButtons(Dialog::Allow | Dialog::Block);
        if(dialog->Execute() && dialog->ClickedButton() == Dialog::Allow)
            reply->ignoreSslErrors();
        break;
    }
    case Application::AskForEachHost:{

        QString host = reply->url().host();

        if(Application::GetBlockedHosts().contains(host)){
        } else if(Application::GetAllowedHosts().contains(host)){
            reply->ignoreSslErrors();
        } else {
            const Dialog::Button clicked = ModalDialog::AllowBlockOrCancel
                (tr("SSL errors on host: %1").arg(host),
                 tr("Allow or Block this host?"), SslErrorLines(errors));
            if(clicked == Dialog::Allow){
                Application::AppendToAllowedHosts(host, offTheRecord);
                reply->ignoreSslErrors();
            } else if(clicked == Dialog::Block){
                Application::AppendToBlockedHosts(host, offTheRecord);
            }
        }
        break;
    }
    case Application::AskForEachCertificate:{
        const QString host = reply->url().host();
        const QList<QSslCertificate> chain =
            reply->sslConfiguration().peerCertificateChain();
        const QString fingerprint = CertificatePolicy::Fingerprint(chain);
        const QString key =
            CertificatePolicy::KeyFromFingerprint(host, fingerprint);
        switch(CertificatePolicy::Find(key,
                                       Application::GetAllowedCertificates(),
                                       Application::GetBlockedCertificates())){
        case CertificatePolicy::Allow:
            reply->ignoreSslErrors();
            break;
        case CertificatePolicy::Block:
            break;
        case CertificatePolicy::Ask:{
            QString detail = SslErrorLines(errors);
            if(!fingerprint.isEmpty())
                detail += QStringLiteral("\nSHA-256: ") + fingerprint;
            const Dialog::Button clicked = ModalDialog::AllowBlockOrCancel
                (tr("SSL errors on host: %1").arg(host),
                 tr("Allow or Block this certificate?"), detail);
            if(clicked == Dialog::Allow){
                Application::RememberCertificate(key, true, offTheRecord);
                reply->ignoreSslErrors();
            } else if(clicked == Dialog::Block){
                Application::RememberCertificate(key, false, offTheRecord);
            }
            break;
        }
        }
        break;
    }
    default: break;
    }
}

void NetworkAccessManager::HandleAuthentication(QNetworkReply *reply,
                                                QAuthenticator *authenticator){
    ModalDialog::Authentication(authenticator, ModalDialog::AuthenticationCaption(reply->url()));
}

#ifndef QT_NO_NETWORKPROXY
void NetworkAccessManager::HandleProxyAuthentication(const QNetworkProxy &proxy,
                                                     QAuthenticator *authenticator){
    ModalDialog::Authentication(authenticator, ModalDialog::AuthenticationCaption(QUrl(), proxy.hostName()));
}
#endif

namespace {
    View *ViewOfDownloadRequest(QObject *request){
#ifdef WEBENGINEVIEW
        if(QWebEngineDownloadRequest *widgets = qobject_cast<QWebEngineDownloadRequest*>(request))
            if(WebEnginePage *page = qobject_cast<WebEnginePage*>(widgets->page()))
                return page->GetView();
        if(QObject *item = request ? request->property("view").value<QObject*>() : nullptr){
            for(QQmlContext *context = qmlContext(item); context; context = context->parentContext()){
                const QVariant found = context->contextProperty(QStringLiteral("viewInterface"));
                if(QObject *owner = found.value<QObject*>()) return dynamic_cast<View*>(owner);
            }
        }
#else
        Q_UNUSED(request)
#endif
        return nullptr;
    }
}

bool NetworkController::MadeOffTheRecord(QObject *request){
    QObject *maker = request ? request->parent() : nullptr;
    if(!maker) return false;
#ifdef WEBENGINEVIEW
    if(QWebEngineProfile *profile = qobject_cast<QWebEngineProfile*>(maker))
        return profile->isOffTheRecord();
#endif
    return maker->property("offTheRecord").toBool();
}

void NetworkAccessManager::HandleDownload(QObject *object){
#ifdef WEBENGINEVIEW
    static QSet<QObject*> set;
    if(set.contains(object)) return;
    set << object;
    connect(object, &QObject::destroyed, [object](){ set.remove(object);});

    QWebEngineDownloadRequest *orig_item = qobject_cast<QWebEngineDownloadRequest*>(object);

    QUrl sourcePage;
    bool sourcePrivate = NetworkController::MadeOffTheRecord(object);
    if(View *starter = ViewOfDownloadRequest(object))
        sourcePrivate = sourcePrivate || Page::OffTheRecord(starter);
    if(orig_item && orig_item->page()){
        sourcePage = orig_item->page()->url();
        sourcePrivate = sourcePrivate || orig_item->page()->profile()->isOffTheRecord();
    } else if(QObject *view = object->property("view").value<QObject*>()){
        sourcePage = view->property("url").toUrl();
        if(QObject *profile = view->property("profile").value<QObject*>())
            sourcePrivate = sourcePrivate || profile->property("offTheRecord").toBool();
    }

    Application::AskDownloadPolicyIfNeed();

    DownloadItem *item = new DownloadItem(object);
    item->SetSource(sourcePage, sourcePrivate);
    QString dir = Application::GetDownloadDirectory();
    QString filename;
    QString mime;
    QUrl url;

    if(View *w = dynamic_cast<View*>(Application::CurrentWidget()))
        if(TreeBank *tb = w->GetTreeBank()) tb->GoBackOrCloseForDownload(w);

    if(orig_item){
        filename = QDir::cleanPath(QDir(orig_item->downloadDirectory()).filePath(orig_item->downloadFileName()));
        mime = orig_item->mimeType();
        url = orig_item->url();
    } else {
        filename = object->property("path").toString();
        mime = object->property("mimeType").toString();
        url = object->property("url").toUrl();
    }

    const bool savesPage = orig_item && orig_item->isSavePageDownload();
    QWebEngineDownloadRequest::SavePageFormat pageFormat = QWebEngineDownloadRequest::MimeHtmlSaveFormat;
    if(savesPage){
        const QString format =
            Application::GlobalSettings()
            .value(QStringLiteral("webview/@SavePageFormat"),
                   QStringLiteral("MimeHtmlSaveFormat")).value<QString>();
        if(format == QStringLiteral("SingleHtmlSaveFormat"))
            pageFormat = QWebEngineDownloadRequest::SingleHtmlSaveFormat;
        else if(format == QStringLiteral("CompleteHtmlSaveFormat"))
            pageFormat = QWebEngineDownloadRequest::CompleteHtmlSaveFormat;
    }
    const bool savesHtml = savesPage && pageFormat != QWebEngineDownloadRequest::MimeHtmlSaveFormat;
    if(savesHtml) mime = QStringLiteral("text/html");

    QString name = DownloadName::Sanitize(QFileInfo(filename).fileName());
    if(savesHtml) name = DownloadName::AsHtml(name);

    if(name.isEmpty()) name = DownloadName::Suggest(QString(), mime, url);

    filename = DownloadName::Unique(QDir::cleanPath(QDir(dir).filePath(name)));

    if(filename.isEmpty() ||
       Application::GetDownloadPolicy() == Application::Undefined_ ||
       Application::GetDownloadPolicy() == Application::AskForEachDownload){

        QString filter;

        QMimeDatabase db;
        QMimeType mimeType = db.mimeTypeForName(mime);
        if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForFile(filename);
        if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(url);

        if(mimeType.isValid() && !mimeType.isDefault()) filter = mimeType.filterString();

        filename = ModalDialog::GetSaveFileName_(QString(), filename, filter);
    }

    if(filename.isEmpty()){
        QMetaObject::invokeMethod(object, "cancel");
        item->deleteLater();
        return;
    }

    item->SetPath(filename);
    if(orig_item){
        if(savesPage) orig_item->setSavePageFormat(pageFormat);
        orig_item->setDownloadDirectory(QFileInfo(filename).path());
        orig_item->setDownloadFileName(QFileInfo(filename).fileName());
    } else {
        object->setProperty("path", filename);
    }
    {
        bool offTheRecord = false;
        if(orig_item && orig_item->page()){
            offTheRecord = orig_item->page()->profile()->isOffTheRecord();
        } else if(QObject *view = object->property("view").value<QObject*>()){
            if(QObject *profile = view->property("profile").value<QObject*>())
                offTheRecord = profile->property("offTheRecord").toBool();
        }
        item->Record(offTheRecord);
    }
    QMetaObject::invokeMethod(object, "accept");

    QStringList path = filename.split(QStringLiteral("/"));
    path.removeLast();
    Application::SetDownloadDirectory(path.join(QStringLiteral("/")) + QStringLiteral("/"));
#else
    Q_UNUSED(object)
#endif
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op,
                                                   const QNetworkRequest &req,
                                                   QIODevice *out){
    QNetworkRequest newreq = CompletedRequest(req, ResolvedUserAgent());

    if(m_SslProtocol != QSsl::UnknownProtocol &&
       (req.url().scheme() == QStringLiteral("https") || req.url().scheme() == QStringLiteral("ftps"))){
        QSslConfiguration sslConfig = newreq.sslConfiguration();
        sslConfig.setProtocol(m_SslProtocol);
        newreq.setSslConfiguration(sslConfig);
    }

    QNetworkReply *rep = QNetworkAccessManager::createRequest(op, newreq, out);

    connect(rep,  &QNetworkReply::errorOccurred,
            this, &NetworkAccessManager::HandleError);
    connect(rep,  &QNetworkReply::sslErrors,
            this, &NetworkAccessManager::HandleSslErrors);

    return rep;
}

void NetworkAccessManager::SetNetworkCookieJar(NetworkCookieJar *ncj){
    setCookieJar(ncj);
}

NetworkCookieJar *NetworkAccessManager::GetNetworkCookieJar() const {
    return static_cast<NetworkCookieJar*>(cookieJar());
}

void NetworkAccessManager::SetUserAgent(QString ua){
    ua = ua.split(QStringLiteral(" ")).last();

    const QString browser = UserAgent::NameOf(ua);

    if(!browser.isEmpty()) ua = Application::UserAgentFor(browser);
    if(!UserAgent::LooksLikeUserAgent(ua)) return;

    QString system;

#if defined(Q_OS_WIN)
    system = QStringLiteral("Windows NT 10.0; Win64; x64");
#elif defined(Q_OS_MAC)
    const QString product = Application::ProductVersion();
    const QString sysInfo = QSysInfo::productVersion();
    if(product.isEmpty() && sysInfo.isEmpty()){
        system = QStringLiteral("Mac OS X");
    } else if(browser == QStringLiteral("Firefox")){
        system = QStringLiteral("Intel Mac OS X ") + (sysInfo.isEmpty() ? product : sysInfo);
    } else {
        system = QStringLiteral("Intel Mac OS X ") +
            QString(product.isEmpty() ? sysInfo : product).replace(QLatin1Char('.'), QLatin1Char('_'));
    }

    system = QStringLiteral("Macintosh; ") + system;
#else
    system = QStringLiteral("X11; Linux x86_64");
#endif

    QString chromium = QStringLiteral("140.0.0.0");
    QString full     = chromium;
#ifdef WEBENGINEVIEW
    const QString engine = QString::fromLatin1(qWebEngineChromiumVersion());
    if(!engine.isEmpty()){
        chromium = engine.section(QLatin1Char('.'), 0, 0) + QStringLiteral(".0.0.0");
        full     = engine;
    }
#endif

    ua = UserAgent::Expand(ua, system, chromium);

    m_UserAgent = ua;
    m_UserAgentBrand = browser;
    m_UserAgentFullVersion = full;

#ifdef WEBENGINEVIEW
    foreach(QWebEngineProfile *profile, Profiles()){
        profile->setHttpUserAgent(ua);
        SetupClientHints(profile, browser, ua, full);
    }
#endif
}

#ifdef WEBENGINEVIEW

void NetworkAccessManager::SetupClientHints(QWebEngineProfile *profile,
                                            const QString &browser,
                                            const QString &ua,
                                            const QString &full){
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QWebEngineClientHints *hints = profile->clientHints();
    if(!hints) return;

    hints->resetAll();

    if(!ua.contains(QStringLiteral("Chrome/"))){
        hints->setAllClientHintsEnabled(false);
        return;
    }
    hints->setAllClientHintsEnabled(true);
    hints->setFullVersion(full);

    static const QMap<QString, QString> brands = {
        { QStringLiteral("Chrome"),  QStringLiteral("Google Chrome")  },
        { QStringLiteral("Edge"),    QStringLiteral("Microsoft Edge") },
        { QStringLiteral("OPR"),     QStringLiteral("Opera")          },
        { QStringLiteral("Vivaldi"), QStringLiteral("Vivaldi")        },
    };
    if(brands.contains(browser)){
        QVariantMap list;
        list.insert(brands.value(browser), full);
        hints->setFullVersionList(list);
    }
#else
    Q_UNUSED(profile) Q_UNUSED(browser) Q_UNUSED(ua) Q_UNUSED(full)
#endif
}
#endif

QString NetworkAccessManager::GetUserAgent() const {
    return m_UserAgent;
}

QString NetworkAccessManager::ResolvedUserAgent() const {
    if(!m_UserAgent.isEmpty()) return m_UserAgent;
#ifdef WEBENGINEVIEW
    if(m_Profile) return m_Profile->httpUserAgent();
#endif
    return QString();
}

void NetworkAccessManager::SetRequestPurpose(QNetworkRequest &request,
                                             RequestPurpose purpose){
    request.setAttribute(QNetworkRequest::User, int(purpose));
}

NetworkAccessManager::RequestPurpose
NetworkAccessManager::PurposeOf(const QNetworkRequest &request){
    const QVariant said = request.attribute(QNetworkRequest::User);
    return said.isValid() && said.toInt() == int(Navigation) ? Navigation : Subresource;
}

void NetworkAccessManager::SetRequestOffTheRecord(QNetworkRequest &request,
                                                  bool offTheRecord){
    request.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), offTheRecord);
}

bool NetworkAccessManager::IsOffTheRecord(const QNetworkRequest &request){
    return request.attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toBool();
}

QNetworkRequest NetworkAccessManager::CompletedRequest(const QNetworkRequest &request,
                                                       const QString &userAgent){
    QNetworkRequest completed(request);

    const QUrl url = completed.url();
    const QString scheme = url.scheme();
    if(scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))
        return completed;

    if(!userAgent.isEmpty() && !completed.hasRawHeader("User-Agent"))
        completed.setRawHeader("User-Agent", userAgent.toLatin1());

    if(PurposeOf(completed) != Navigation) return completed;

    if(!completed.hasRawHeader("Accept"))
        completed.setRawHeader("Accept",
                               "text/html,application/xhtml+xml,application/xml;q=0.9,"
                               "image/avif,image/webp,image/apng,*/*;q=0.8,"
                               "application/signed-exchange;v=b3;q=0.7");

    if(!completed.hasRawHeader("Upgrade-Insecure-Requests"))
        completed.setRawHeader("Upgrade-Insecure-Requests", "1");

    if(completed.hasRawHeader("Sec-Fetch-Dest") ||
       completed.hasRawHeader("Sec-Fetch-Mode") ||
       completed.hasRawHeader("Sec-Fetch-User") ||
       completed.hasRawHeader("Sec-Fetch-Site")) return completed;

    completed.setRawHeader("Sec-Fetch-Dest", "document");
    completed.setRawHeader("Sec-Fetch-Mode", "navigate");
    completed.setRawHeader("Sec-Fetch-User", "?1");

    const QUrl referer = QUrl::fromEncoded(completed.rawHeader("Referer"));
    if(referer.isValid() && !referer.isEmpty()){
        const int here  = url.port(scheme == QStringLiteral("https") ? 443 : 80);
        const int there = referer.port(referer.scheme() == QStringLiteral("https") ? 443 : 80);
        if(referer.scheme() == scheme && referer.host() == url.host() && there == here)
            completed.setRawHeader("Sec-Fetch-Site", "same-origin");
    }

    return completed;
}

void NetworkAccessManager::SetProxy(QString proxySet){
#ifndef QT_NO_NETWORKPROXY
    QStringList set = proxySet.split(QStringLiteral(" "));
    set.takeFirst();
    if(set.length() < 1) return;

    QString type = QString();
    QString host = QString();
    QString user = QString();
    QString pass = QString();
    int port = 1080;

    QStringList addrplus;
    QStringList hostplus;
    QStringList userplus;

    if(set.length() == 2){
        type = set[0];
        addrplus = set[1].split(QStringLiteral("@"));
    } else {
        type = QStringLiteral("default");
        addrplus = set[0].split(QStringLiteral("@"));
    }

    if(addrplus.length() == 2){
        userplus = addrplus[0].split(QStringLiteral(":"));
        hostplus = addrplus[1].split(QStringLiteral(":"));
        if(userplus.length() == 2){
            user = userplus[0];
            pass = userplus[1];
        } else {
            user = userplus[0];
        }
    } else {
        hostplus = addrplus[0].split(QStringLiteral(":"));
    }

    if(hostplus.length() == 2){
        host = hostplus[0];
        port = hostplus[1].toInt();
    } else {
        host = hostplus[0];
    }

    if(!host.isEmpty() && proxy().hostName() != host){
        QNetworkProxy proxy;
        if     (Application::ExactMatch(QStringLiteral("[dD]efault(?:[pP]roxy)?"), type))
            proxy.setType(QNetworkProxy::DefaultProxy);
        else if(Application::ExactMatch(QStringLiteral("[sS]ocks5?(?:[pP]roxy)?"), type))
            proxy.setType(QNetworkProxy::Socks5Proxy);
        else if(Application::ExactMatch(QStringLiteral("[hH]ttp(?:[pP]roxy)?"), type))
            proxy.setType(QNetworkProxy::HttpProxy);
        else if(Application::ExactMatch(QStringLiteral("[hH]ttp[cC]aching(?:[pP]roxy)?"), type))
            proxy.setType(QNetworkProxy::HttpCachingProxy);
        else if(Application::ExactMatch(QStringLiteral("[fF]tp[cC]aching(?:[pP]roxy)?"), type))
            proxy.setType(QNetworkProxy::FtpCachingProxy);
        else
            proxy.setType(QNetworkProxy::DefaultProxy);

        if(!host.isEmpty()) proxy.setHostName(host);
        if(!user.isEmpty()) proxy.setUser(user);
        if(!pass.isEmpty()) proxy.setPassword(pass);
        proxy.setPort(static_cast<quint16>(port));
        setProxy(proxy);
    }
#else
    Q_UNUSED(proxySet)
#endif
}

QSsl::SslProtocol NetworkAccessManager::SslProtocolForSetting(const QString &sslSet){
    const QString version = sslSet.split(QStringLiteral(" ")).last();

    if     (Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1.2"), version))
        return QSsl::TlsV1_2;
    else if(Application::ExactMatch(QStringLiteral("[aA]ny(?:[pP]rotocol)?"), version))
        return QSsl::AnyProtocol;
    else if(Application::ExactMatch(QStringLiteral("[sS]ecure(?:[pP]rotocol)?"), version))
        return QSsl::SecureProtocols;

    else if(Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1(?:.0)?"), version) ||
            Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1.1"), version) ||
            Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1(?:.0)?"
                                                   "[sS][sS][lL][vV](?:ersion)?3(?:.0)?"), version))
        return QSsl::SecureProtocols;

    return QSsl::UnknownProtocol;
}

void NetworkAccessManager::SetSslProtocol(QString sslSet){
    m_SslProtocol = SslProtocolForSetting(sslSet);
}

#ifdef WEBENGINEVIEW

static void PresentNotification(QWebEngineNotification *notification){
    if(!notification) return;

    std::shared_ptr<QWebEngineNotification> held(notification);

    QQmlEngine::setObjectOwnership(held.get(), QQmlEngine::CppOwnership);

    held->show();

    ModelessDialog *dialog = new ModelessDialog();
    dialog->SetTitle(held->title().isEmpty()
                     ? held->origin().host() : held->title());
    dialog->SetCaption(held->message());
    dialog->SetButtons(Dialog::Open | Dialog::Close);
    dialog->SetDefaultValue(false);
    dialog->SetCallBack([dialog, held](bool ok){
        if(ok && dialog->ClickedButton() == Dialog::Open)
            held->click();
        held->close();
    });
    QTimer::singleShot(0, dialog, [dialog](){ dialog->Execute();});
}

static void MirrorProfileCookies(QWebEngineCookieStore *store, const QString &id,
                                 const QString &source, QObject *context,
                                 QPointer<NetworkAccessManager> owner){
    if(!store || !context) return;

    const auto reach = [owner, id]() -> NetworkCookieJar* {
        NetworkAccessManager *nam =
            owner ? owner.data() : NetworkController::FindNetworkAccessManager(id);
        return nam ? nam->GetNetworkCookieJar() : nullptr;
    };

    QObject::connect(store, &QWebEngineCookieStore::cookieAdded, context,
                         [reach, source](const QNetworkCookie &cookie){
                             if(NetworkCookieJar *jar = reach())
                                 jar->MirrorCookie(source, cookie);
                         });
    QObject::connect(store, &QWebEngineCookieStore::cookieRemoved, context,
                         [reach, source](const QNetworkCookie &cookie){
                             if(NetworkCookieJar *jar = reach())
                                 jar->UnmirrorCookie(source, cookie);
                         });
}

void NetworkAccessManager::SetupProfile(QWebEngineProfile *profile){
    Settings &s = Application::GlobalSettings();

    profile->setProperty(PROFILE_KEY_PROPERTY,
                           profile->isOffTheRecord() ? QString() : m_Id);
    profile->setProperty(CREDENTIAL_SPACE_PROPERTY, m_Id);

    if(!profile->isOffTheRecord()){
        SetProfileCache(profile, m_Id);

        if(Application::SaveSessionCookie())
            profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
        else
            profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);

        const QString cache = s.value(QStringLiteral("network/@HttpCacheType"),
                                      QStringLiteral("DiskHttpCache")).value<QString>();
        if(cache == QStringLiteral("NoCache"))
            profile->setHttpCacheType(QWebEngineProfile::NoCache);
        else if(cache == QStringLiteral("MemoryHttpCache"))
            profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
        else
            profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);

        const int megabytes = s.value(QStringLiteral("network/@HttpCacheMaximumSize"), 0).value<int>();
        profile->setHttpCacheMaximumSize(megabytes > 0 ? megabytes * 1024 * 1024 : 0);

    }
    NetworkController::AdoptPermissions(profile);

    profile->setDownloadPath(Application::GetDownloadDirectory());

    const QStringList dictionaries =
        s.value(QStringLiteral("network/@SpellCheckLanguages"), QStringList()).value<QStringList>();
    profile->setSpellCheckLanguages(dictionaries);
    profile->setSpellCheckEnabled(!dictionaries.isEmpty());

    profile->setPushServiceEnabled
        (s.value(QStringLiteral("network/@EnablePushService"), false).value<bool>());

    profile->setNotificationPresenter(
        [profile](std::unique_ptr<QWebEngineNotification> notification){
            if(notification && ExtensionHost::HasOffscreenOf(profile, notification->origin())){
                notification->close();
                return;
            }
            PresentNotification(notification.release());
        });

    SettingsSchemeHandler::Install(profile);

    profile->setHttpAcceptLanguage(Application::GetAcceptLanguage());

    static QString source;
    if(source.isEmpty()){
        QString inner;
        inner += View::InstallEventFilterJsCode(m_EventTypes);
        source = QStringLiteral
            ("(function(){\n"
             "%1\n"
             "})();").arg(inner);
    }
    QWebEngineScript script;
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    script.setRunsOnSubFrames(true);
    script.setSourceCode(source);
    profile->scripts()->insert(script);

#ifdef VANILLA_PASSWORDS
    WebEnginePasswords::InstallScript(profile);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0) && QT_CONFIG(webengine_extensions)
    WebEngineExtensions::Install(profile);
    if(QWebEngineExtensionManager *extensions = profile->extensionManager()){

        if(s.value(QStringLiteral("network/@UnloadHangoutsExtension"), true).value<bool>()){
            static const QString hangouts =
                QStringLiteral("nkeimhogjdpnpccoofpliimaahmaaome");
            foreach(const QWebEngineExtensionInfo &e, extensions->extensions()){
                if(e.id() == hangouts) extensions->unloadExtension(e);
            }
        }
    }
#endif

    UrlBlockRules::ReloadRules();
    ExtensionNetRules::Reload();
    profile->setUrlRequestInterceptor(RequestInterceptor::For(profile->isOffTheRecord()));

    connect(profile, &QWebEngineProfile::downloadRequested,
            this, &NetworkAccessManager::HandleDownload);

    if(!m_UserAgent.isEmpty()){
        profile->setHttpUserAgent(m_UserAgent);
        SetupClientHints(profile, m_UserAgentBrand, m_UserAgent, m_UserAgentFullVersion);
    }
}
#endif

#ifdef WEBENGINEVIEW
QWebEngineProfile *NetworkAccessManager::GetProfile(bool offTheRecord){
    return GetSharedProfile(offTheRecord).get();
}

SharedProfile NetworkAccessManager::GetSharedProfile(bool offTheRecord){
    if(!offTheRecord) return m_Profile;
    if(!m_PrivateProfile){
        m_PrivateProfile = WrapProfile(new QWebEngineProfile());
        SetupProfile(m_PrivateProfile.get());
    }
    return m_PrivateProfile;
}

QList<QWebEngineProfile*> NetworkAccessManager::Profiles() const {
    QList<QWebEngineProfile*> profiles;
    if(m_Profile) profiles << m_Profile.get();
    if(m_PrivateProfile) profiles << m_PrivateProfile.get();
    return profiles;
}
#endif

DownloadItem::DownloadItem(QNetworkReply *reply, QString defaultfilename)
    : QObject(nullptr)
{
    m_DefaultFileName = QString();

    m_DownloadReply = reply;
    m_DownloadItem = nullptr;
    connect(m_DownloadReply, &QNetworkReply::readyRead, this, &DownloadItem::ReadyRead);
    connect(m_DownloadReply, &QNetworkReply::finished,  this, &DownloadItem::Finished);
    connect(m_DownloadReply, &QNetworkReply::downloadProgress,
            this, &DownloadItem::DownloadProgress);
    m_GettingPath = false;

    defaultfilename = DownloadName::Sanitize(defaultfilename);
    if(defaultfilename.isEmpty())
        defaultfilename = DownloadName::FromUrl(reply->url());
    if(defaultfilename.isEmpty())
        defaultfilename = DownloadName::FromUrl(reply->request().url());
    m_DefaultFileName = defaultfilename;
    m_RemoteUrl = QUrl();
    m_Path = QString();
    m_BAOut = QByteArray();
    m_FinishedFlag = false;
}

DownloadItem::DownloadItem(QObject *object)
    : QObject(nullptr)
{
#if defined(WEBENGINEVIEW) || defined(EDGEWEBVIEW)
    m_DefaultFileName = QString();
    m_DownloadItem = object;
    m_DownloadReply = nullptr;
#ifdef WEBENGINEVIEW
    if(
       QWebEngineDownloadRequest *item = qobject_cast<QWebEngineDownloadRequest*>(object)
       ){

        connect(item, &QWebEngineDownloadRequest::stateChanged,
                this, &DownloadItem::StateChanged);
        connect(item, &QWebEngineDownloadRequest::receivedBytesChanged,
                this, &DownloadItem::ReceivedBytesChanged);
        connect(item, &QWebEngineDownloadRequest::totalBytesChanged,
                this, &DownloadItem::ReceivedBytesChanged);
        m_RemoteUrl = item->url();
        m_Path = QDir::cleanPath(QDir(item->downloadDirectory()).filePath(item->downloadFileName()));
    } else
#endif
    if(object){
        connect(object, SIGNAL(stateChanged()),
                this, SLOT(StateChanged()));
        connect(object, SIGNAL(receivedBytesChanged()),
                this, SLOT(ReceivedBytesChanged()));
        m_RemoteUrl = object->property("url").toUrl();
        m_Path = object->property("path").toString();
    }

    if(object) connect(object, &QObject::destroyed, this, &DownloadItem::Finished);

    m_GettingPath = true;
    m_BAOut = QByteArray();
    m_FinishedFlag = false;
#else
    Q_UNUSED(object)
#endif
}

DownloadItem::~DownloadItem(){
    NetworkController::RemoveItem(this);

    if(m_DownloadReply) m_DownloadReply->deleteLater();
}

void DownloadItem::SetRemoteUrl(QUrl url){
    m_RemoteUrl = url;
}

QUrl DownloadItem::GetRemoteUrl() const {
    return m_RemoteUrl;
}

QUrl DownloadItem::GetLocalUrl() const {
    return QUrl::fromLocalFile(m_Path);
}

QList<QUrl> DownloadItem::GetUrls() const {
    QList<QUrl> list;
    if(!GetRemoteUrl().isEmpty()) list << GetRemoteUrl();
    if(!GetLocalUrl() .isEmpty()) list << GetLocalUrl();
    return list;
}

QString DownloadItem::GetPath() const {
    return m_Path;
}

void DownloadItem::SetPath(QString path){
    m_Path = path;
}

void DownloadItem::SetPathAndReady(QString path){
    if(path.isEmpty()) {
        Stop();
        return;
    }

    SetPath(path);

    if(m_Path == DISABLE_FILENAME) return;

    Record();

    m_FileOut.setFileName(m_Path);
    bool opened = m_FileOut.open(QIODevice::WriteOnly);
    if(!opened){
    }
}

QByteArray DownloadItem::HeaderOfReplyOrRequest(QNetworkRequest::KnownHeaders known,
                                                const char *name,
                                                const char *lowercase) const {
    if(!m_DownloadReply) return QByteArray();

    const QVariant data = m_DownloadReply->header(known);
    if(data.isValid()) return data.toByteArray();

    if(m_DownloadReply->hasRawHeader(name))
        return m_DownloadReply->rawHeader(name);
    if(m_DownloadReply->request().hasRawHeader(name))
        return m_DownloadReply->request().rawHeader(name);
    if(m_DownloadReply->hasRawHeader(lowercase))
        return m_DownloadReply->rawHeader(lowercase);
    if(m_DownloadReply->request().hasRawHeader(lowercase))
        return m_DownloadReply->request().rawHeader(lowercase);

    return QByteArray();
}

QString DownloadItem::ContentTypeOfReplyOrRequest() const {
    return QString::fromUtf8
        (HeaderOfReplyOrRequest(QNetworkRequest::ContentTypeHeader,
                                "Content-Type", "content-type"));
}

QString DownloadItem::CreateDefaultFromReplyOrRequest(){
    QString filename =
        DownloadName::FromContentDisposition
        (HeaderOfReplyOrRequest(QNetworkRequest::ContentDispositionHeader,
                                "Content-Disposition", "content-disposition"));

    if(filename.isEmpty())
        filename = m_DefaultFileName;

    return DownloadName::Unique
        (QDir::cleanPath
         (QDir(Application::GetDownloadDirectory())
          .filePath(DownloadName::Suggest(filename,
                                          ContentTypeOfReplyOrRequest(),
                                          m_DownloadReply->url()))));
}

#if defined(WEBENGINEVIEW) || defined(EDGEWEBVIEW)

void DownloadItem::StateChanged(){
    if(!m_DownloadItem) return;
#ifdef WEBENGINEVIEW
    if(
       QWebEngineDownloadRequest *item = qobject_cast<QWebEngineDownloadRequest*>(m_DownloadItem.data())
       ){

        QWebEngineDownloadRequest::DownloadState state = item->state();
        QWebEngineDownloadRequest::DownloadInterruptReason reason = item->interruptReason();

        if(state == QWebEngineDownloadRequest::DownloadCompleted){
            EndRecord(DownloadHistory::Completed);
            Finished();
        } else if(state == QWebEngineDownloadRequest::DownloadInterrupted &&
           40 > reason && reason >= 20){

            ModelessDialog::Information(QString("resumed."), QString("download failure, and resumed."));

            item->resume();
        } else if(state == QWebEngineDownloadRequest::DownloadInterrupted){
            EndRecord(DownloadHistory::Failed);
            Finished();
        }
    } else
#endif
    {
        int state = m_DownloadItem->property("state").toInt();
        int reason = m_DownloadItem->property("interruptReason").toInt();
        if(state == 2){
            EndRecord(DownloadHistory::Completed);
            Finished();
        } else if(
            state == 4 &&
            40 > reason && reason >= 20){

            ModelessDialog::Information(QString("resumed."), QString("download failure, and resumed."));

            QMetaObject::invokeMethod(m_DownloadItem.data(), "resume");
        } else if(state == 4){
            EndRecord(DownloadHistory::Failed);
            Finished();
        }
    }
}

void DownloadItem::ReceivedBytesChanged(){
    if(!m_DownloadItem) return;
    DownloadProgress(m_DownloadItem->property("receivedBytes").toLongLong(),
                     m_DownloadItem->property("totalBytes").toLongLong());
}
#endif

void DownloadItem::ReadyRead(){
    if(m_GettingPath) return;

    if(m_Path == DISABLE_FILENAME) {
        m_BAOut = m_BAOut + m_DownloadReply->readAll();
        return;
    }

    if(m_Path.isEmpty() && !m_FinishedFlag){
        m_GettingPath = true;

        Application::AskDownloadPolicyIfNeed();

        QString filename = CreateDefaultFromReplyOrRequest();

        if(filename.isEmpty() ||
           Application::GetDownloadPolicy() == Application::Undefined_ ||
           Application::GetDownloadPolicy() == Application::AskForEachDownload){

            const QString type =
                ContentTypeOfReplyOrRequest().section(QLatin1Char(';'), 0, 0).trimmed();

            QString filter;

            QMimeDatabase db;
            QMimeType mimeType;
            if(!type.isEmpty()) mimeType = db.mimeTypeForName(type);
            if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForFile(filename);
            if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(m_DownloadReply->url());
            if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(m_DownloadReply->request().url());

            if(mimeType.isValid() && !mimeType.isDefault()) filter = mimeType.filterString();

            filename = ModalDialog::GetSaveFileName_(QString(), filename, filter);
        }

        SetPathAndReady(filename);

        if(m_Path.isEmpty()){
            Stop();
            return;
        }

        QStringList path = m_Path.split(QStringLiteral("/"));
        path.removeLast();
        Application::SetDownloadDirectory(path.join(QStringLiteral("/")) + QStringLiteral("/"));
        m_GettingPath = false;
    }

    if(m_FileOut.isOpen()){
        m_FileOut.write(m_DownloadReply->readAll());
    }
    if(m_FinishedFlag){
        EndRecord(m_DownloadReply && m_DownloadReply->error() == QNetworkReply::NoError
                  ? DownloadHistory::Completed : DownloadHistory::Failed);
        deleteLater();
    }
}

void DownloadItem::Finished(){
    if(m_FinishedFlag) return;

    ReadyRead();

    EndRecord(m_DownloadReply
              ? (m_DownloadReply->error() == QNetworkReply::NoError
                 ? DownloadHistory::Completed : DownloadHistory::Failed)
              : DownloadHistory::Failed);

    if(m_FileOut.isOpen()){
        m_FileOut.close();
    }

    if(m_Path == DISABLE_FILENAME){
        emit DownloadResult(m_BAOut);
    }
    emit Progress(m_Path, 100, 100);
    disconnect();
    m_FinishedFlag = true;
    if(!m_Path.isEmpty()){
        deleteLater();
    }
}

void DownloadItem::Stop(){

    if(m_FinishedFlag){
        deleteLater();
        return;
    }
    EndRecord(DownloadHistory::Cancelled);
    m_FinishedFlag = true;

    if(m_FileOut.isOpen()) m_FileOut.close();
    if(m_DownloadReply) m_DownloadReply->abort();
    if(m_DownloadItem){
        QMetaObject::invokeMethod(m_DownloadItem.data(), "cancel");
    }
    emit Progress(m_Path, 100, 100);
    disconnect();

    deleteLater();
}

void DownloadItem::Record(bool hideUrl){
    if(m_HistoryId || m_Path.isEmpty() || m_Path == DISABLE_FILENAME) return;
    const QString temporary = QDir::cleanPath(Application::TemporaryDirectory()) + QStringLiteral("/");
    if(QDir::cleanPath(m_Path).startsWith(temporary, Qt::CaseInsensitive)) return;
    const bool hidden = hideUrl || m_SourceOffTheRecord;
    const QUrl url = hidden ? QUrl()
        : m_DownloadReply ? m_DownloadReply->url() : m_RemoteUrl;
    m_HistoryId = DownloadHistory::Instance()->Started
        (url, m_Path, this, hidden ? QUrl() : m_SourcePage);
}

void DownloadItem::SetSource(const QUrl &page, bool offTheRecord){
    m_SourcePage = page;
    m_SourceOffTheRecord = offTheRecord;
}

void DownloadItem::EndRecord(int state){
    if(m_HistoryId)
        DownloadHistory::Instance()->Ended(m_HistoryId, static_cast<DownloadHistory::State>(state));
}

void DownloadItem::DownloadProgress(qint64 received, qint64 total){
    if(m_HistoryId) DownloadHistory::Instance()->Progress(m_HistoryId, received, total);

    if(!m_Path.isEmpty()){

        if(total > 0){
            emit Progress(m_Path, received, total);
        } else {
            qint64 dummy = 10;
            qint64 r = received;
            while((r /= 10) > 0) dummy *= 10;
            emit Progress(m_Path, received, dummy);
        }
    }

    if(!received && !total) return;

    bool finished = false;

    if(m_DownloadItem){

#ifdef WEBENGINEVIEW
        if(
           QWebEngineDownloadRequest *item = qobject_cast<QWebEngineDownloadRequest*>(m_DownloadItem.data())
           )
            finished = item->isFinished();
        else
#endif
            finished = m_DownloadItem->property("state").toInt() == 2;
    } else {
        if(received == total) finished = true;
        if(!finished && m_DownloadReply)
            finished = m_DownloadReply->isFinished();
    }

    if(finished){
        Finished();
    }
}

QMap<QString, NetworkAccessManager*> NetworkController::m_NetworkAccessManagerTable = QMap<QString, NetworkAccessManager*>();
QList<NetworkAccessManager*> NetworkController::m_RetiredNetworkAccessManagers = QList<NetworkAccessManager*>();
QMap<QString, QList<QNetworkCookie>> NetworkController::m_HeldCookies = QMap<QString, QList<QNetworkCookie>>();
QMap<QString, QList<QPointer<QObject>>> NetworkController::m_Releasing = QMap<QString, QList<QPointer<QObject>>>();
QList<DownloadItem*> NetworkController::m_DownloadList = QList<DownloadItem*>();

NetworkController::NetworkController()
    : QObject(nullptr)
{
    LoadAllCookies();

    Application::ClearTemporaryDirectory();
    QMap<QString, NetworkAccessManager*> nams = m_NetworkAccessManagerTable;
    if(!nams.isEmpty()){
        Download(nams.first(), BLANK_URL, EMPTY_URL, NetworkController::TemporaryDirectory, false);
    }
}

NetworkController::~NetworkController(){}

DownloadItem *NetworkController::Download(NetworkAccessManager *nam,
                                          const QUrl &url, const QUrl &referer,
                                          DownloadType type, bool offTheRecord){
    QNetworkRequest req(url);
    req.setRawHeader("Referer", referer.toEncoded());
    NetworkAccessManager::SetRequestOffTheRecord(req, offTheRecord);
    DownloadItem *item = Download(nam, req, type);

    if(item) item->SetRemoteUrl(url);

    return item;
}

DownloadItem* NetworkController::Download(NetworkAccessManager *nam,
                                          const QNetworkRequest &request,
                                          DownloadType type){
    if(request.url().isEmpty()) return nullptr;

    return Download(nam->get(request), DownloadName::FromUrl(request.url()), type);
}

DownloadItem* NetworkController::Download(QNetworkReply *reply,
                                          QString filename,
                                          DownloadType type){
    QVariant lengthHeader = reply->header(QNetworkRequest::ContentLengthHeader);
    bool ok;
    int size = lengthHeader.toInt(&ok);
    if(ok && size == 0){
        reply->deleteLater();
        return nullptr;
    }
    DownloadItem *item = new DownloadItem(reply, filename);
    switch (type){
    case SelectedDirectory : {
        break;
    }
    case TemporaryDirectory : {
        const QString name = DownloadName::Sanitize(filename);
        item->SetPathAndReady
            (DownloadName::Unique
             (QDir::cleanPath
              (QDir(Application::TemporaryDirectory())
               .filePath(name.isEmpty() ? QStringLiteral("index.html") : name))));
        break;
    }
    case ToVariable : {
        item->SetPathAndReady(DISABLE_FILENAME);
        break;
    }
    }
    m_DownloadList << item;
    return item;
}

void NetworkController::RemoveItem(DownloadItem *item){
        m_DownloadList.removeOne(item);
}

void NetworkController::SetUserAgent(NetworkAccessManager *nam, QStringList set){
    int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A[uU](?:ser)?[aA](?:gent)? [^ ]+")));
    if(pos == -1) return;
    nam->SetUserAgent(set[pos]);
}

void NetworkController::SetProxy(NetworkAccessManager *nam, QStringList set){
    int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A[pP][rR][oO][xX][yY] [^ ].*")));
    if(pos == -1) return;
    nam->SetProxy(set[pos]);
}

void NetworkController::SetSslProtocol(NetworkAccessManager *nam, QStringList set){
    int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A[sS][sS][lL] [^ ]+")));
    if(pos == -1) return;
    nam->SetSslProtocol(set[pos]);
}

#ifdef WEBENGINEVIEW
QWebEngineProfile* NetworkController::InspectorProfile(){
    static QWebEngineProfile *profile = nullptr;
    if(!profile){
        static const QString key = QStringLiteral("inspector");
        profile = new QWebEngineProfile(NetworkController::ProfileStorageName(key),
                                        Application::GetInstance());
        profile->setProperty(PROFILE_KEY_PROPERTY, key);
        SetProfileCache(profile, key);
    }
    return profile;
}

QQuickWebEngineProfile* NetworkController::QuickInspectorProfile(){
    static QQuickWebEngineProfile *profile = nullptr;
    if(!profile){
        const QString key = QStringLiteral("quick-inspector");
        const QString name = NetworkController::ProfileStorageName(key);
        profile = new QQuickWebEngineProfile(Application::GetInstance());
        profile->setProperty(PROFILE_KEY_PROPERTY, key);
        profile->setStorageName(name);
        profile->setOffTheRecord(false);
        profile->setPersistentStoragePath(
            Application::DataDirectory() + QStringLiteral("webengine/") + name);
        profile->setCachePath(
            Application::DataDirectory() + QStringLiteral("webenginecache/") + name);
    }
    return profile;
}

static QMap<QString, QQuickWebEngineProfile*> &QuickProfileTable(){
    static QMap<QString, QQuickWebEngineProfile*> profiles;
    return profiles;
}

static QMap<QString, QQuickWebEngineProfile*> &QuickPrivateProfileTable(){
    static QMap<QString, QQuickWebEngineProfile*> profiles;
    return profiles;
}

static void InstallQuickFrameScript(QQuickWebEngineProfile *profile){
    QWebEngineScript script;
    script.setName(QStringLiteral("vanilla-events-frames"));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    script.setRunsOnSubFrames(true);
    script.setSourceCode(QuickWebEngineView::DefaultFrameScript());
    QObject *scripts = profile->property("userScripts").value<QObject*>();
    if(!scripts || !QMetaObject::invokeMethod(scripts, "insert", Q_ARG(QWebEngineScript, script)))
        qWarning() << "the key filter could not be put on a quick profile.";
#ifdef VANILLA_QUICK_PASSWORD_FRAMES
    WebEnginePasswords::InstallFrameScript(profile);
#else
    Q_UNUSED(profile)
#endif
}

void NetworkController::ConnectQuickNotifications(QQuickWebEngineProfile *profile){
    if(!profile) return;

    static const char *connected = "vanillaQuickNotificationsConnected";
    if(profile->property(connected).toBool()) return;
    profile->setProperty(connected, true);

    QObject::connect(profile, &QQuickWebEngineProfile::presentNotification,
                     profile, [](QWebEngineNotification *notification){
                         PresentNotification(notification);
                     });
}

namespace {

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    using PermissionType = QWebEnginePermission::PermissionType;

    QString KindOfType(PermissionType type){
        switch(type){
        case PermissionType::Notifications:            return QStringLiteral("notifications");
        case PermissionType::Geolocation:              return QStringLiteral("geolocation");
        case PermissionType::MediaAudioCapture:        return QStringLiteral("microphone");
        case PermissionType::MediaVideoCapture:        return QStringLiteral("camera");
        case PermissionType::ClipboardReadWrite:       return QStringLiteral("clipboard");
        case PermissionType::LocalFontsAccess:         return QStringLiteral("local-fonts");
        case PermissionType::MouseLock:                return QStringLiteral("mouse-lock");
        case PermissionType::DesktopVideoCapture:      return QStringLiteral("screen");
        case PermissionType::DesktopAudioVideoCapture: return QStringLiteral("screen-audio");
        case PermissionType::MediaAudioVideoCapture:
        case PermissionType::Unsupported:              break;
        }
        return QString();
    }

    PermissionType TypeOfKind(const QString &kind){
        static const PermissionType types[] = {
            PermissionType::Notifications, PermissionType::Geolocation,
            PermissionType::MediaAudioCapture, PermissionType::MediaVideoCapture,
            PermissionType::ClipboardReadWrite, PermissionType::LocalFontsAccess,
            PermissionType::MouseLock, PermissionType::DesktopVideoCapture,
            PermissionType::DesktopAudioVideoCapture,
        };
        for(PermissionType type : types)
            if(KindOfType(type) == kind) return type;
        return PermissionType::Unsupported;
    }

    QStringList StorableKinds(){
        QStringList kinds;
        foreach(const QString &kind, PermissionTable::Kinds()){
            const PermissionType type = TypeOfKind(kind);
            if(type != PermissionType::Unsupported && QWebEnginePermission::isPersistent(type))
                kinds << kind;
        }
        return kinds;
    }

    template <class Profile>
    void MirrorInto(Profile *profile){
        const QList<QWebEnginePermission> all = profile->listAllPermissions();
        QList<PermissionTable::Row> held;
        for(const QWebEnginePermission &permission : all){
            const QString kind = KindOfType(permission.permissionType());
            const QWebEnginePermission::State state = permission.state();
            held << PermissionTable::Row{
                permission.origin().toString(),
                kind.isEmpty() ? QStringLiteral("engine:") +
                    QString::number(static_cast<int>(permission.permissionType())) : kind,
                state == QWebEnginePermission::State::Granted ? PermissionTable::Allow
                : state == QWebEnginePermission::State::Denied ? PermissionTable::Block
                : PermissionTable::Ask };
        }

        foreach(const PermissionTable::Row &change,
                SitePermissions::Table().Changes(held, StorableKinds())){
            if(change.state == PermissionTable::Ask){
                for(int i = 0; i < all.length(); i++)
                    if(held[i].origin == change.origin && held[i].kind == change.kind)
                        all[i].reset();
                continue;
            }
            const QWebEnginePermission permission =
                profile->queryPermission(QUrl(change.origin), TypeOfKind(change.kind));
            if(change.state == PermissionTable::Allow) permission.grant();
            else permission.deny();
        }
    }

    template <class Profile>
    void ApplyDelta(Profile *profile, const QList<PermissionTable::Row> &delta){
        const QStringList storable = StorableKinds();
        foreach(const PermissionTable::Row &row, delta){
            if(!storable.contains(row.kind)) continue;
            const QWebEnginePermission permission =
                profile->queryPermission(QUrl(row.origin), TypeOfKind(row.kind));
            if(row.state == PermissionTable::Allow) permission.grant();
            else if(row.state == PermissionTable::Block) permission.deny();
            else permission.reset();
        }
    }

    template <class Profile, class Policy>
    void Adopt(Profile *profile, Policy onDisk, Policy inMemory){
        if(!profile) return;
        if(!profile->isOffTheRecord() && profile->persistentPermissionsPolicy() == onDisk){
            foreach(const QWebEnginePermission &permission, profile->listAllPermissions())
                permission.reset();
        }
        if(profile->persistentPermissionsPolicy() != inMemory)
            profile->setPersistentPermissionsPolicy(inMemory);
        MirrorInto(profile);
    }
#endif
}

void NetworkController::AdoptPermissions(QWebEngineProfile *profile){
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    Adopt(profile, QWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk,
          QWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
#else
    Q_UNUSED(profile)
#endif
}

void NetworkController::AdoptPermissions(QQuickWebEngineProfile *profile){
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    Adopt(profile, QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk,
          QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
#else
    Q_UNUSED(profile)
#endif
}

QStringList NetworkController::PermissionKindsOfType(int type){
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    const PermissionType permissionType = static_cast<PermissionType>(type);
    if(permissionType == PermissionType::MediaAudioVideoCapture)
        return QStringList() << QStringLiteral("microphone") << QStringLiteral("camera");
    const QString kind = KindOfType(permissionType);
    return kind.isEmpty() ? QStringList() : QStringList(kind);
#else
    Q_UNUSED(type)
    return QStringList();
#endif
}

QStringList NetworkController::PermissionKindsOfFeature(int feature){
    switch(feature){
    case 0: return QStringList(QStringLiteral("microphone"));
    case 1: return QStringList(QStringLiteral("camera"));
    case 2: return QStringList() << QStringLiteral("microphone") << QStringLiteral("camera");
    case 3: return QStringList(QStringLiteral("geolocation"));
    case 4: return QStringList(QStringLiteral("screen"));
    case 5: return QStringList(QStringLiteral("screen-audio"));
    case 6: return QStringList(QStringLiteral("notifications"));
    case 7: return QStringList(QStringLiteral("clipboard"));
    case 8: return QStringList(QStringLiteral("local-fonts"));
    }
    return QStringList();
}

void NetworkController::ApplyQuickBlockRules(QQuickWebEngineProfile *profile){
    if(!profile) return;
    UrlBlockRules::ReloadRules();
    ExtensionNetRules::Reload();
    profile->setUrlRequestInterceptor(RequestInterceptor::For(profile->isOffTheRecord()));
}

void NetworkController::ApplyQuickCommonSettings(QQuickWebEngineProfile *profile){
    if(!profile) return;
    Settings &s = Application::GlobalSettings();

    const QStringList dictionaries =
        s.value(QStringLiteral("network/@SpellCheckLanguages"),
                QStringList()).value<QStringList>();
    profile->setSpellCheckLanguages(dictionaries);
    profile->setSpellCheckEnabled(!dictionaries.isEmpty());

    profile->setPushServiceEnabled
        (s.value(QStringLiteral("network/@EnablePushService"), false).value<bool>());

    profile->setHttpAcceptLanguage(Application::GetAcceptLanguage());
}

void NetworkController::ApplyAcceptLanguage(){
    const QString language = Application::GetAcceptLanguage();
    foreach(NetworkAccessManager *nam, LiveNetworkAccessManagers()){
        foreach(QWebEngineProfile *profile, nam->Profiles()){
            if(profile->httpAcceptLanguage() != language)
                profile->setHttpAcceptLanguage(language);
        }
    }
    foreach(QQuickWebEngineProfile *profile, QuickProfileTable()){
        if(profile->httpAcceptLanguage() != language)
            profile->setHttpAcceptLanguage(language);
    }
    foreach(QQuickWebEngineProfile *profile, QuickPrivateProfileTable()){
        if(profile->httpAcceptLanguage() != language)
            profile->setHttpAcceptLanguage(language);
    }
}

QQuickWebEngineProfile* NetworkController::QuickProfile(const QString &id){
    QMap<QString, QQuickWebEngineProfile*> &profiles = QuickProfileTable();
    if(!profiles.contains(id)){
        Settings &s = Application::GlobalSettings();
        const QString key = QStringLiteral("quick:") + id;
        const QString name = NetworkController::ProfileStorageName(key);
        const QString cacheType = s.value(QStringLiteral("network/@HttpCacheType"),
                                          QStringLiteral("DiskHttpCache")).toString();
        const int maxMB = s.value(QStringLiteral("network/@HttpCacheMaximumSize"), 0).toInt();
        QQuickWebEngineProfile *profile = WebEngineExtensions::CreateQuickProfile(Application::GetInstance(), {
            {QStringLiteral("storageName"), name},
            {QStringLiteral("persistentStoragePath"), Application::DataDirectory() + QStringLiteral("webengine/") + name},
            {QStringLiteral("cachePath"), Application::DataDirectory() + QStringLiteral("webenginecache/") + name},
            {QStringLiteral("persistentCookiesPolicy"), static_cast<int>(Application::SaveSessionCookie()
                ? QQuickWebEngineProfile::ForcePersistentCookies : QQuickWebEngineProfile::AllowPersistentCookies)},
            {QStringLiteral("httpCacheType"), static_cast<int>(cacheType == QStringLiteral("NoCache")
                ? QQuickWebEngineProfile::NoCache : cacheType == QStringLiteral("MemoryHttpCache")
                ? QQuickWebEngineProfile::MemoryHttpCache : QQuickWebEngineProfile::DiskHttpCache)},
            {QStringLiteral("httpCacheMaximumSize"), maxMB > 0 ? maxMB * 1024 * 1024 : 0},
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            {QStringLiteral("persistentPermissionsPolicy"), static_cast<int>(
                QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk)}
#endif
        });
        profile->setProperty(PROFILE_KEY_PROPERTY, key);
        profile->setProperty(CREDENTIAL_SPACE_PROPERTY, id);
        WebEngineExtensions::Install(profile);
        if(profile->isOffTheRecord()){
            SettingsSchemeHandler::Install(profile);
            AdoptPermissions(profile);
            ApplyQuickBlockRules(profile);
            ApplyQuickCommonSettings(profile);
            ConnectQuickNotifications(profile);
            InstallQuickFrameScript(profile);
            profiles[id] = profile;
            return profile;
        }

        profile->setPersistentCookiesPolicy
            (Application::SaveSessionCookie()
             ? QQuickWebEngineProfile::ForcePersistentCookies
             : QQuickWebEngineProfile::AllowPersistentCookies);

        const QString cache = s.value(QStringLiteral("network/@HttpCacheType"),
                                      QStringLiteral("DiskHttpCache")).value<QString>();
        if(cache == QStringLiteral("NoCache"))
            profile->setHttpCacheType(QQuickWebEngineProfile::NoCache);
        else if(cache == QStringLiteral("MemoryHttpCache"))
            profile->setHttpCacheType(QQuickWebEngineProfile::MemoryHttpCache);
        else
            profile->setHttpCacheType(QQuickWebEngineProfile::DiskHttpCache);

        const int megabytes =
            s.value(QStringLiteral("network/@HttpCacheMaximumSize"), 0).value<int>();
        profile->setHttpCacheMaximumSize(megabytes > 0 ? megabytes * 1024 * 1024 : 0);

        SettingsSchemeHandler::Install(profile);

        AdoptPermissions(profile);
        ApplyQuickBlockRules(profile);
        ApplyQuickCommonSettings(profile);

        ConnectQuickNotifications(profile);
        InstallQuickFrameScript(profile);

        MirrorProfileCookies(profile->cookieStore(), id,
                             QStringLiteral("quick:") + profile->storageName(),
                             profile, nullptr);

        profiles[id] = profile;
    }
    return profiles[id];
}

QQuickWebEngineProfile* NetworkController::QuickPrivateProfile(const QString &id){
    QMap<QString, QQuickWebEngineProfile*> &profiles = QuickPrivateProfileTable();
    if(!profiles.contains(id)){
        QQuickWebEngineProfile *profile =
            new QQuickWebEngineProfile(Application::GetInstance());
        profile->setProperty(PROFILE_KEY_PROPERTY,
                             QStringLiteral("quick-private:") + id);
        profile->setProperty(CREDENTIAL_SPACE_PROPERTY, id);
        profile->setOffTheRecord(true);
        SettingsSchemeHandler::Install(profile);
        AdoptPermissions(profile);
        ApplyQuickBlockRules(profile);
        ApplyQuickCommonSettings(profile);
        ConnectQuickNotifications(profile);
        InstallQuickFrameScript(profile);
        profiles[id] = profile;
    }
    return profiles[id];
}

QString NetworkController::CredentialSpace(const QObject *profile){
    if(!profile) return QString();
    return profile->property(CREDENTIAL_SPACE_PROPERTY).toString();
}

QString NetworkController::ProfileKey(const QObject *profile){
    if(!profile) return QString();
    const QVariant key = profile->property(PROFILE_KEY_PROPERTY);
    if(!key.isValid()) return profile->property("storageName").toString();
    return key.toString();
}

#endif

QString NetworkController::ProfileStorageName(const QString &applicationDir,
                                              const QString &id){
    return QString::fromLatin1(
        QCryptographicHash::hash((applicationDir + QStringLiteral("/") + id).toUtf8(),
                                 QCryptographicHash::Md5).toHex());
}

QString NetworkController::ProfileStorageName(const QString &id){
    return ProfileStorageName(QCoreApplication::applicationDirPath(), id);
}

NetworkAccessManager* NetworkController::FindNetworkAccessManager(const QString &id){
    return m_NetworkAccessManagerTable.value(id, nullptr);
}

NetworkAccessManager* NetworkController::GetNetworkAccessManager(QString id, QStringList set){
    if(!m_NetworkAccessManagerTable[id])
        InitializeNetworkAccessManager(id, QList<QNetworkCookie>());
    NetworkAccessManager *nam = m_NetworkAccessManagerTable[id];
    SetUserAgent(nam, set);
    SetProxy(nam, set);
    SetSslProtocol(nam, set);
    return nam;
}

NetworkAccessManager* NetworkController::CopyNetworkAccessManager(QString bef, QString aft, QStringList set){
    NetworkAccessManager *bnam = GetNetworkAccessManager(bef);
    InitializeNetworkAccessManager(aft, bnam->GetNetworkCookieJar()->GetPersistableCookies());
    NetworkAccessManager *nam = m_NetworkAccessManagerTable[aft];
#ifndef QT_NO_NETWORKPROXY
    nam->setProxy(bnam->proxy());
#endif
    SetUserAgent(nam, set);
    SetProxy(nam, set);
    SetSslProtocol(nam, set);
    return nam;
}

NetworkAccessManager* NetworkController::MoveNetworkAccessManager(QString bef, QString aft, QStringList set){
    NetworkAccessManager *nam = GetNetworkAccessManager(bef);
    if(bef != aft){
        if(m_NetworkAccessManagerTable[aft])
            return MergeNetworkAccessManager(bef, aft, set);
        m_NetworkAccessManagerTable[aft] = nam;
        m_NetworkAccessManagerTable.remove(bef);
    }
    SetUserAgent(nam, set);
    SetProxy(nam, set);
    SetSslProtocol(nam, set);
    return nam;
}

NetworkAccessManager* NetworkController::MergeNetworkAccessManager(QString bef, QString aft, QStringList set){
    GetNetworkAccessManager(bef);
    GetNetworkAccessManager(aft, set);
    NetworkCookieJar *b = m_NetworkAccessManagerTable[bef]->GetNetworkCookieJar();
    NetworkCookieJar *a = m_NetworkAccessManagerTable[aft]->GetNetworkCookieJar();
    a->SetAllCookies(a->GetAllCookies() + b->GetPersistableCookies());
    RetireNetworkAccessManager(m_NetworkAccessManagerTable.take(bef));
    return m_NetworkAccessManagerTable[aft];
}

NetworkAccessManager* NetworkController::KillNetworkAccessManager(QString id){
    RetireNetworkAccessManager(m_NetworkAccessManagerTable.take(id));
    return nullptr;
}

QMap<QString, NetworkAccessManager*> NetworkController::AllNetworkAccessManager(){
    return m_NetworkAccessManagerTable;
}

QList<NetworkAccessManager*> NetworkController::LiveNetworkAccessManagers(){
    QList<NetworkAccessManager*> live;
    foreach(NetworkAccessManager *nam, m_NetworkAccessManagerTable)
        if(nam && !live.contains(nam)) live << nam;
    foreach(NetworkAccessManager *nam, m_RetiredNetworkAccessManagers)
        if(nam && !live.contains(nam)) live << nam;
    return live;
}

void NetworkController::RetireNetworkAccessManager(NetworkAccessManager *nam){
    if(!nam || m_RetiredNetworkAccessManagers.contains(nam) ||
       std::find(m_NetworkAccessManagerTable.cbegin(), m_NetworkAccessManagerTable.cend(), nam)
       != m_NetworkAccessManagerTable.cend())
        return;
    m_RetiredNetworkAccessManagers << nam;
}

QList<NetworkAccessManager*> NetworkController::SpaceManagers(const QString &id){
    QList<NetworkAccessManager*> managers;
#ifdef WEBENGINEVIEW
    const QString storage = ProfileStorageName(id);
    foreach(NetworkAccessManager *nam, LiveNetworkAccessManagers()){
        const QList<QWebEngineProfile*> profiles = nam->Profiles();
        if(!profiles.isEmpty() && profiles.first()->storageName() == storage) managers << nam;
    }
#else
    if(NetworkAccessManager *nam = FindNetworkAccessManager(id)) managers << nam;
#endif
    return managers;
}

QList<QObject*> NetworkController::SpaceProfiles(const QString &id){
    QList<QObject*> profiles;
#ifdef WEBENGINEVIEW
    foreach(NetworkAccessManager *nam, SpaceManagers(id))
        foreach(QWebEngineProfile *profile, nam->Profiles()) profiles << profile;
    if(QQuickWebEngineProfile *profile = QuickProfileTable().value(id)) profiles << profile;
    if(QQuickWebEngineProfile *profile = QuickPrivateProfileTable().value(id)) profiles << profile;
#else
    Q_UNUSED(id)
#endif
    return profiles;
}

void NetworkController::ReleaseSpace(const QString &id){
    const QList<NetworkAccessManager*> managers = SpaceManagers(id);
    foreach(QObject *profile, SpaceProfiles(id)) NoteReleasing(id, profile);
    if(NetworkAccessManager *nam = FindNetworkAccessManager(id))
        m_HeldCookies[id] = nam->GetNetworkCookieJar()->GetPersistableCookies();
    for(NetworkAccessManager *nam : managers){
        for(auto it = m_NetworkAccessManagerTable.begin(); it != m_NetworkAccessManagerTable.end();){
            if(it.value() == nam) it = m_NetworkAccessManagerTable.erase(it);
            else ++it;
        }
        m_RetiredNetworkAccessManagers.removeAll(nam);
        nam->deleteLater();
    }
#ifdef WEBENGINEVIEW
    QList<QQuickWebEngineProfile*> quick;
    if(QQuickWebEngineProfile *profile = QuickProfileTable().take(id)) quick << profile;
    if(QQuickWebEngineProfile *profile = QuickPrivateProfileTable().take(id)) quick << profile;
    for(QQuickWebEngineProfile *profile : std::as_const(quick)){
        ExtensionHost::CloseOffscreenOf(profile);
        WebEngineExtensions::DeleteQuickProfile(profile);
    }
#endif
}

NetworkAccessManager* NetworkController::JoinSpace(const QString &from, const QString &to,
                                                  QStringList set, bool release){
    NetworkAccessManager *into = GetNetworkAccessManager(to, set);
    if(NetworkAccessManager *nam = FindNetworkAccessManager(from)){
        NetworkCookieJar *jar = into->GetNetworkCookieJar();
        jar->SetAllCookies(jar->GetAllCookies() + nam->GetNetworkCookieJar()->GetPersistableCookies());
    }
    if(release){
        ReleaseSpace(from);
        DropHeldCookies(from);
    }
    return into;
}

void NetworkController::NoteReleasing(const QString &id, QObject *object){
    if(object) m_Releasing[id] << QPointer<QObject>(object);
}

bool NetworkController::StorageInUse(const QString &id){
    QList<QPointer<QObject>> &going = m_Releasing[id];
    going.removeAll(QPointer<QObject>());
    if(going.isEmpty()) m_Releasing.remove(id);
    else return true;
    NetworkAccessManager *named = FindNetworkAccessManager(id);
    foreach(NetworkAccessManager *nam, SpaceManagers(id))
        if(nam != named) return true;
    return false;
}

QStringList NetworkController::KnownSpaceNames(){
    QStringList names = m_NetworkAccessManagerTable.keys();
    for(const QString &id : m_HeldCookies.keys()) if(!names.contains(id)) names << id;
    for(const QString &id : m_Releasing.keys()) if(!names.contains(id)) names << id;
#ifdef WEBENGINEVIEW
    for(const QString &id : QuickProfileTable().keys()) if(!names.contains(id)) names << id;
#endif
    return names;
}

void NetworkController::DropHeldCookies(const QString &id){
    m_HeldCookies.remove(id);
}

NetworkAccessManager* NetworkController::PlaceSpace(const QString &from, const QString &to, QStringList set){
    InitializeNetworkAccessManager(to, m_HeldCookies.take(from));
    NetworkAccessManager *nam = m_NetworkAccessManagerTable[to];
    SetUserAgent(nam, set);
    SetProxy(nam, set);
    SetSslProtocol(nam, set);
    return nam;
}

QString NetworkController::WidgetStorageDirectory(){
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/QtWebEngine/");
}

void NetworkController::ClearCookies(){
#ifdef EDGEWEBVIEW
    EdgeWebView::ClearCookies();
#endif
    foreach(NetworkAccessManager *nam, LiveNetworkAccessManagers()){
        if(NetworkCookieJar *jar = nam->GetNetworkCookieJar())
            jar->SetAllCookies(QList<QNetworkCookie>());
#ifdef WEBENGINEVIEW
        foreach(QWebEngineProfile *profile, nam->Profiles())
            if(QWebEngineCookieStore *store = profile->cookieStore())
                store->deleteAllCookies();
#endif
    }
#ifdef WEBENGINEVIEW
    foreach(QQuickWebEngineProfile *profile,
            QuickProfileTable().values() + QuickPrivateProfileTable().values()){
        if(!profile) continue;
        if(QWebEngineCookieStore *store = profile->cookieStore())
            store->deleteAllCookies();
    }
#endif
    for(auto it = m_HeldCookies.begin(); it != m_HeldCookies.end(); ++it) it.value().clear();
    if(AutoSaver *saver = Application::GetAutoSaver()) saver->SaveAllAsync();
}

void NetworkController::ClearHttpCache(){
#ifdef EDGEWEBVIEW
    EdgeWebView::ClearHttpCache();
#endif
#ifdef WEBENGINEVIEW
    foreach(NetworkAccessManager *nam, LiveNetworkAccessManagers()){
        foreach(QWebEngineProfile *profile, nam->Profiles())
            profile->clearHttpCache();
    }
    foreach(QQuickWebEngineProfile *profile,
            QuickProfileTable().values() + QuickPrivateProfileTable().values()){
        if(profile) profile->clearHttpCache();
    }
#endif
}

void NetworkController::ClearVisitedLinks(){
#ifdef EDGEWEBVIEW
    EdgeWebView::ClearVisitedLinks();
#endif
#ifdef WEBENGINEVIEW
    foreach(NetworkAccessManager *nam, LiveNetworkAccessManagers()){
        foreach(QWebEngineProfile *profile, nam->Profiles())
            profile->clearAllVisitedLinks();
    }
#endif
}

void NetworkController::ClearPasswords(){
#ifdef EDGEWEBVIEW
    EdgeWebView::ClearPasswords();
#endif
#ifdef VANILLA_PASSWORDS
    if(PasswordStore *store = PasswordStore::Instance()){
        store->RemoveAll([](int failed){
            if(failed) qWarning() << "passwords:" << failed << "could not be removed from the store";
        });
    }
#endif
}

void NetworkController::MirrorPermissions(const QList<PermissionTable::Row> &delta){
#if defined(WEBENGINEVIEW) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    foreach(NetworkAccessManager *nam, LiveNetworkAccessManagers()){
        foreach(QWebEngineProfile *profile, nam->Profiles()) ApplyDelta(profile, delta);
    }
    foreach(QQuickWebEngineProfile *profile,
            QuickProfileTable().values() + QuickPrivateProfileTable().values())
        if(profile) ApplyDelta(profile, delta);
#else
    Q_UNUSED(delta)
#endif
}

void NetworkController::InitializeNetworkAccessManager(QString id, const QList<QNetworkCookie> &cookies){
    NetworkCookieJar *ncj = new NetworkCookieJar();
    NetworkAccessManager *nam = new NetworkAccessManager(id);
    ncj->SetAllCookies(cookies);
    nam->SetNetworkCookieJar(ncj);
    NetworkAccessManager *replaced = m_NetworkAccessManagerTable.value(id, nullptr);
    m_NetworkAccessManagerTable[id] = nam;
    RetireNetworkAccessManager(replaced);
}

bool NetworkController::LoadCookieFile(QString path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject obj = doc.object();
    for(QJsonObject::const_iterator it = obj.constBegin(); it != obj.constEnd(); it++){
        QList<QNetworkCookie> cookies;
        for(QJsonValue raw : it.value().toArray()){
            cookies << QNetworkCookie::parseCookies(raw.toString().toLatin1());
        }
        InitializeNetworkAccessManager(it.key(), cookies);
    }
    return true;
}

void NetworkController::LoadAllCookies(){
    QString datadir = Application::StateDirectory();
    QString filename = Application::CookieFileName();
    if(LoadCookieFile(datadir + filename)) return;

    QDir dir = QDir(datadir);
    QStringList list =
        dir.entryList(Application::BackUpFileFilters(),
                      QDir::NoFilter, QDir::Name | QDir::Reversed);

    for(QString backup : std::as_const(list)){

        if(!backup.endsWith(filename)) continue;
        if(!LoadCookieFile(datadir + backup)) continue;

        ModelessDialog::Information
            (tr("Restored from a backup file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
             tr("Because of a failure to read the latest file, it was restored from a backup file."));
        break;
    }
}

bool NetworkController::ShouldSaveCookie(const QNetworkCookie &cookie,
                                         bool saveSessionCookie, const QDateTime &now){
    if(cookie.isSessionCookie()) return saveSessionCookie;

    return !(cookie.expirationDate().toLocalTime() < now &&
             cookie.expirationDate().toUTC()       < now);
}

QByteArray NetworkController::CookieSnapshot(){
    const QDateTime now = QDateTime::currentDateTime();
    const bool saveSessionCookie = Application::SaveSessionCookie();

    QMap<QString, NetworkAccessManager*> table = AllNetworkAccessManager();
    QJsonObject doc;
    for(QString id : table.keys()){
        QJsonArray rawdata;
        foreach(QNetworkCookie cookie, table[id]->GetNetworkCookieJar()->GetPersistableCookies()){
            if(!ShouldSaveCookie(cookie, saveSessionCookie, now)) continue;
            rawdata.append(QString::fromLatin1(cookie.toRawForm()));
        }
        doc[id] = rawdata;
    }
    for(auto it = m_HeldCookies.cbegin(); it != m_HeldCookies.cend(); ++it){
        if(doc.contains(it.key())) continue;
        QJsonArray rawdata;
        foreach(QNetworkCookie cookie, it.value()){
            if(!ShouldSaveCookie(cookie, saveSessionCookie, now)) continue;
            rawdata.append(QString::fromLatin1(cookie.toRawForm()));
        }
        doc[it.key()] = rawdata;
    }

    return QJsonDocument(doc).toJson(QJsonDocument::Indented);
}

bool NetworkController::SaveCookieSnapshot(const QByteArray &snapshot){
    const QString datadir = Application::StateDirectory();
    const QString cookie  = datadir + Application::CookieFileName(false);
    const QString cookieb = datadir + Application::CookieFileName(true);

    if(QFile::exists(cookieb)) QFile::remove(cookieb);

    QFile file(cookieb);
    if(!file.open(QIODevice::WriteOnly)) return false;
    const bool written = file.write(snapshot) == snapshot.size() && file.flush();
    file.close();
    if(!written){
        file.remove();
        return false;
    }

    return FileExchange::Replace(cookieb, cookie);
}

