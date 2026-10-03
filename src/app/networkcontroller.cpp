#include "switch.hpp"
#include "const.hpp"

#include "networkcontroller.hpp"

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
#  include "webengineextensions.hpp"
#  include "extensionhost.hpp"
#  include <QtWebEngineCore/qtwebenginecoreglobal.h>
#  include "settingspage.hpp"
#  include <QWebEngineDownloadRequest>
#  include <QWebEngineNotification>
#  include <QWebEngineCookieStore>
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
#  include "quickwebengineview.hpp"
#endif

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

void NetworkAccessManager::HandleSslErrors(const QList<QSslError> &errors){
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if(!reply){
        return;
    }

    Application::AskSslErrorPolicyIfNeed();

    switch(Application::GetSslErrorPolicy()){
    case Application::BlockAccess:
        break;
    case Application::IgnoreSslErrors:
        reply->ignoreSslErrors();
        break;
    case Application::AskForEachAccess:{

        ModalDialog *dialog = new ModalDialog();
        dialog->SetTitle(tr("Ssl errors."));
        dialog->SetCaption(tr("Ssl errors."));
        dialog->SetInformativeText(tr("Ignore errors in this access?"));

        QString detail;
        foreach(QSslError error, errors){
            if(!detail.isEmpty()) detail += QStringLiteral("\n");
            detail += QStringLiteral("Ssl error : ") + error.errorString();
        }
        dialog->SetDetailedText(detail);
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
            ModalDialog *dialog = new ModalDialog();
            dialog->SetTitle(tr("Ssl errors on host:%1").arg(host));
            dialog->SetCaption(tr("Ssl errors on host:%1").arg(host));
            dialog->SetInformativeText(tr("Allow or Block this host?"));

            QString detail;
            foreach(QSslError error, errors){
                if(!detail.isEmpty()) detail += QStringLiteral("\n");
                detail += QStringLiteral("Ssl error : ") + error.errorString();
            }
            dialog->SetDetailedText(detail);

            dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
            dialog->Execute();
            const Dialog::Button clicked = dialog->ClickedButton();
            if(clicked == Dialog::Allow){
                Application::AppendToAllowedHosts(host);
                reply->ignoreSslErrors();
            } else if(clicked == Dialog::Block){
                Application::AppendToBlockedHosts(host);
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
            ModalDialog *dialog = new ModalDialog();
            dialog->SetTitle(tr("Ssl errors on host:%1").arg(host));
            dialog->SetCaption(tr("Ssl errors on host:%1").arg(host));
            dialog->SetInformativeText(tr("Allow or Block this certificate?"));

            QString detail;
            foreach(const QSslError &error, errors){
                if(!detail.isEmpty()) detail += QStringLiteral("\n");
                detail += QStringLiteral("Ssl error : ") + error.errorString();
            }
            if(!fingerprint.isEmpty())
                detail += QStringLiteral("\nSHA-256: ") + fingerprint;
            dialog->SetDetailedText(detail);
            dialog->SetButtons(Dialog::Allow | Dialog::Block | Dialog::Cancel);
            dialog->Execute();

            const Dialog::Button clicked = dialog->ClickedButton();
            if(clicked == Dialog::Allow){
                Application::RememberCertificate(key, true);
                reply->ignoreSslErrors();
            } else if(clicked == Dialog::Block){
                Application::RememberCertificate(key, false);
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
    Q_UNUSED(reply)
    ModalDialog::Authentication(authenticator);
}

#ifndef QT_NO_NETWORKPROXY
void NetworkAccessManager::HandleProxyAuthentication(const QNetworkProxy &proxy,
                                                     QAuthenticator *authenticator){
    Q_UNUSED(proxy)
    ModalDialog::Authentication(authenticator);
}
#endif

void NetworkAccessManager::HandleDownload(QObject *object){
#ifdef WEBENGINEVIEW
    static QSet<QObject*> set;
    if(set.contains(object)) return;
    set << object;
    connect(object, &QObject::destroyed, [object](){ set.remove(object);});

    Application::AskDownloadPolicyIfNeed();

    DownloadItem *item = new DownloadItem(object);
    QWebEngineDownloadRequest *orig_item = qobject_cast<QWebEngineDownloadRequest*>(object);
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

    QString name = DownloadName::Sanitize(QFileInfo(filename).fileName());

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
        if(orig_item->isSavePageDownload()){
            const QString format =
                Application::GlobalSettings()
                .value(QStringLiteral("webview/@SavePageFormat"),
                       QStringLiteral("MimeHtmlSaveFormat")).value<QString>();
            if(format == QStringLiteral("SingleHtmlSaveFormat"))
                orig_item->setSavePageFormat(QWebEngineDownloadRequest::SingleHtmlSaveFormat);
            else if(format == QStringLiteral("CompleteHtmlSaveFormat"))
                orig_item->setSavePageFormat(QWebEngineDownloadRequest::CompleteHtmlSaveFormat);
            else
                orig_item->setSavePageFormat(QWebEngineDownloadRequest::MimeHtmlSaveFormat);
        }
        orig_item->setDownloadDirectory(QFileInfo(filename).path());
        orig_item->setDownloadFileName(QFileInfo(filename).fileName());
    } else {
        object->setProperty("path", filename);
    }
    QMetaObject::invokeMethod(object, "accept");
    MainWindow *win = Application::GetCurrentWindow();
    if(win && win->GetTreeBank()->GetNotifier())
        win->GetTreeBank()->GetNotifier()->RegisterDownload(item);

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

    connect(rep,  SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(HandleError(QNetworkReply::NetworkError)));
    connect(rep,  &QNetworkReply::sslErrors,
            this, &NetworkAccessManager::HandleSslErrors);

    QString type = newreq.header(QNetworkRequest::ContentTypeHeader).value<QString>();
    if(type.toLower().contains(QStringLiteral("multipart/form-data;"))){
        UploadItem *item = NetworkController::Upload(rep, newreq.header(QNetworkRequest::ContentLengthHeader).value<qint64>());
        if(item){
            MainWindow *win = Application::GetCurrentWindow();
            if(win && win->GetTreeBank()->GetNotifier())
                win->GetTreeBank()->GetNotifier()->RegisterUpload(item);
        }
    }
    return rep;
}

QString NetworkAccessManager::GetId(){
    return m_Id;
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

    QString system;

#if defined(Q_OS_WIN)
    QOperatingSystemVersion current = QOperatingSystemVersion::current();
    if     (current >= QOperatingSystemVersion::Windows10)  system = QStringLiteral("Windows NT 10.0");
    else if(current >= QOperatingSystemVersion::Windows8_1) system = QStringLiteral("Windows NT 6.3");
    else if(current >= QOperatingSystemVersion::Windows8)   system = QStringLiteral("Windows NT 6.2");
    else if(current >= QOperatingSystemVersion::Windows7)   system = QStringLiteral("Windows NT 6.1");
    else                                                    system = QStringLiteral("Windows");
#  if _WIN64
    system = system + QStringLiteral("; Win64; x64");
#  elif _WIN32
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    BOOL isWow64 = FALSE;
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
        GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(fnIsWow64Process){
        if(fnIsWow64Process(GetCurrentProcess(), &isWow64)){
            if(isWow64){
                system = system + QStringLiteral("; WOW64");
            } else {
            }
        } else {
        }
    } else {
    }
#  else
#  endif
#elif defined(Q_OS_MAC)
    if(Application::ProductVersion().isEmpty() && QSysInfo::productVersion().isEmpty()){
        system = QStringLiteral("Mac OS X");
    } else if(browser == QStringLiteral("Firefox")){
        if(!QSysInfo::productVersion().isEmpty()){
            system = QStringLiteral("Intel Mac OS X ") + QSysInfo::productVersion();
        } else if(!Application::ProductVersion().isEmpty()){
            system = QStringLiteral("Intel Mac OS X ") + Application::ProductVersion();
        }
    } else {
        if(!Application::ProductVersion().isEmpty()){
            system = QStringLiteral("Intel Mac OS X ") + Application::ProductVersion().replace(".", "_");
        } else if(!QSysInfo::productVersion().isEmpty()){
            system = QStringLiteral("Intel Mac OS X ") + QSysInfo::productVersion().replace(".", "_");
        }
    }

    system = QStringLiteral("Macintosh; ") + system;
#else
    system = QStringLiteral("Linux/Unix");
#endif

    QString location = QLocale::system().name().replace(QStringLiteral("_"), QStringLiteral("-"));

    if(ua.isEmpty()) return;
    if(!browser.isEmpty()) ua = Application::UserAgentFor(browser);

    QString chromium = QStringLiteral("140.0.0.0");
    QString full     = chromium;
#ifdef WEBENGINEVIEW
    const QString engine = QString::fromLatin1(qWebEngineChromiumVersion());
    if(!engine.isEmpty()){
        chromium = engine.section(QLatin1Char('.'), 0, 0) + QStringLiteral(".0.0.0");
        full     = engine;
    }
#endif

    ua = UserAgent::Expand(ua, system, location, chromium);

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

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        profile->setPersistentPermissionsPolicy
            (s.value(QStringLiteral("network/@RememberPermissions"), true).value<bool>()
             ? QWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk
             : QWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
#endif
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    else {
        profile->setPersistentPermissionsPolicy
            (QWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
    }
#endif

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

#  ifdef USE_WEBCHANNEL
        inner += View::InstallWebChannelJsCode();
#  endif
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
            Finished();
        } else if(state == QWebEngineDownloadRequest::DownloadInterrupted &&
           40 > reason && reason >= 20){

            ModelessDialog::Information(QString("resumed."), QString("download failure, and resumed."));

            item->resume();
        } else if(state == QWebEngineDownloadRequest::DownloadInterrupted){
            Finished();
        }
    } else
#endif
    {
        int state = m_DownloadItem->property("state").toInt();
        int reason = m_DownloadItem->property("interruptReason").toInt();
        if(state == 2){
            Finished();
        } else if(
            state == 4 &&
            40 > reason && reason >= 20){

            ModelessDialog::Information(QString("resumed."), QString("download failure, and resumed."));

            QMetaObject::invokeMethod(m_DownloadItem.data(), "resume");
        } else if(state == 4){
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
        deleteLater();
    }
}

void DownloadItem::Finished(){
    if(m_FinishedFlag) return;

    ReadyRead();

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

void DownloadItem::DownloadProgress(qint64 received, qint64 total){
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

int UploadItem::m_UnknownCount = 0;

UploadItem::UploadItem(QNetworkReply *reply, qint64 size)
    : QObject(nullptr)
{
    m_UploadReply = reply;
    if(m_UploadReply){
        connect(m_UploadReply, &QNetworkReply::finished, this, &UploadItem::Finished);
        connect(m_UploadReply, &QNetworkReply::uploadProgress,
                this, &UploadItem::UploadProgress);
    }
    m_FileSize = size;
    m_Path = ExpectFileName();
}

UploadItem::UploadItem(QNetworkReply *reply, QString name)
    : QObject(nullptr)
{
    m_UploadReply = reply;
    if(m_UploadReply){
        connect(m_UploadReply, &QNetworkReply::finished, this, &UploadItem::Finished);
        connect(m_UploadReply, &QNetworkReply::uploadProgress,
                this, &UploadItem::UploadProgress);
    }
    m_FileSize = -1;
    m_Path = name;
}

UploadItem::~UploadItem(){
    NetworkController::RemoveItem(this);
}

QString UploadItem::GetPath() const {
    return m_Path;
}

void UploadItem::SetPath(QString name){
    m_Path = name;
}

QString UploadItem::ExpectFileName(){
    QString path = Application::ChosenFiles().last();
    QFile file(path);
    qint64 diff = m_FileSize - file.size();
    qint64 max = 500 + m_FileSize / 20000;
    if(-max < diff && diff < max){
        Application::RemoveChosenFile(path);
        return path;
    }
    return tr("Unknown file (%1)").arg(m_UnknownCount++);
}

void UploadItem::Finished(){
    emit Progress(m_Path, 100, 100);
    disconnect();
    deleteLater();
}

void UploadItem::Stop(){
    m_UploadReply->abort();
    emit Progress(m_Path, 100, 100);
    disconnect();
    deleteLater();
}

void UploadItem::UploadProgress(qint64 sent, qint64 total){
    if(!m_Path.isEmpty()){
        if(m_FileSize != -1){
            emit Progress(m_Path, sent, m_FileSize);
        } else if(total != -1){
            emit Progress(m_Path, sent, total);
        } else {
            qint64 dummy = 10;
            qint64 s = sent;
            while((s /= 10) > 0) dummy *= 10;
            emit Progress(m_Path, sent, dummy);
        }
    } else {
        qint64 dummy = 10;
        qint64 s = sent;
        while((s /= 10) > 0) dummy *= 10;
        emit Progress(tr("Unknown file"), sent, dummy);
    }

    if(sent == m_FileSize || m_UploadReply->isFinished())
        Finished();
}

QMap<QString, NetworkAccessManager*> NetworkController::m_NetworkAccessManagerTable = QMap<QString, NetworkAccessManager*>();
QList<DownloadItem*> NetworkController::m_DownloadList = QList<DownloadItem*>();
QList<UploadItem*>   NetworkController::m_UploadList   = QList<UploadItem*>();

NetworkController::NetworkController()
    : QObject(nullptr)
{
    LoadAllCookies();

    Application::ClearTemporaryDirectory();
    QMap<QString, NetworkAccessManager*> nams = m_NetworkAccessManagerTable;
    if(!nams.isEmpty()){
        Download(nams.first(), BLANK_URL, EMPTY_URL, NetworkController::TemporaryDirectory);
    }
}

NetworkController::~NetworkController(){}

DownloadItem *NetworkController::Download(NetworkAccessManager *nam,
                                          const QUrl &url, const QUrl &referer,
                                          DownloadType type){
    QNetworkRequest req(url);
    req.setRawHeader("Referer", referer.toEncoded());
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

UploadItem* NetworkController::Upload(QNetworkReply *reply, qint64 filesize){
    UploadItem *item = new UploadItem(reply, filesize);
    m_UploadList << item;
    return item;
}

UploadItem* NetworkController::Upload(QNetworkReply *reply, QString filename){
    UploadItem *item = new UploadItem(reply, filename);
    m_UploadList << item;
    return item;
}

void NetworkController::RemoveItem(DownloadItem *item){
        m_DownloadList.removeOne(item);
}

void NetworkController::RemoveItem(UploadItem *item){
        m_UploadList.removeOne(item);
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

void NetworkController::ApplyQuickPermissionsPolicy(QQuickWebEngineProfile *profile){
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if(!profile) return;

    if(profile->isOffTheRecord()){
        profile->setPersistentPermissionsPolicy
            (QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
        return;
    }

    Settings &s = Application::GlobalSettings();
    profile->setPersistentPermissionsPolicy
        (s.value(QStringLiteral("network/@RememberPermissions"), true).value<bool>()
         ? QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk
         : QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
#else
    Q_UNUSED(profile)
#endif
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
    foreach(NetworkAccessManager *nam, AllNetworkAccessManager()){
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
                s.value(QStringLiteral("network/@RememberPermissions"), true).toBool()
                ? QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk
                : QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory)}
#endif
        });
        profile->setProperty(PROFILE_KEY_PROPERTY, key);
        WebEngineExtensions::Install(profile);
        if(profile->isOffTheRecord()){
            SettingsSchemeHandler::Install(profile);
            ApplyQuickPermissionsPolicy(profile);
            ApplyQuickBlockRules(profile);
            ApplyQuickCommonSettings(profile);
            ConnectQuickNotifications(profile);
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

        ApplyQuickPermissionsPolicy(profile);
        ApplyQuickBlockRules(profile);
        ApplyQuickCommonSettings(profile);

        ConnectQuickNotifications(profile);

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
        profile->setOffTheRecord(true);
        SettingsSchemeHandler::Install(profile);
        ApplyQuickPermissionsPolicy(profile);
        ApplyQuickBlockRules(profile);
        ApplyQuickCommonSettings(profile);
        ConnectQuickNotifications(profile);
        profiles[id] = profile;
    }
    return profiles[id];
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
    m_NetworkAccessManagerTable.remove(bef);
    return m_NetworkAccessManagerTable[aft];
}

NetworkAccessManager* NetworkController::KillNetworkAccessManager(QString id){
    m_NetworkAccessManagerTable.remove(id);
    return nullptr;
}

QMap<QString, NetworkAccessManager*> NetworkController::AllNetworkAccessManager(){
    return m_NetworkAccessManagerTable;
}

void NetworkController::ClearCookies(){
#ifdef EDGEWEBVIEW
    EdgeWebView::ClearCookies();
#endif
    foreach(NetworkAccessManager *nam, m_NetworkAccessManagerTable){
        if(!nam) continue;
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
    if(AutoSaver *saver = Application::GetAutoSaver()) saver->SaveAllAsync();
}

void NetworkController::ClearHttpCache(){
#ifdef EDGEWEBVIEW
    EdgeWebView::ClearHttpCache();
#endif
#ifdef WEBENGINEVIEW
    foreach(NetworkAccessManager *nam, m_NetworkAccessManagerTable){
        if(!nam) continue;
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
    foreach(NetworkAccessManager *nam, m_NetworkAccessManagerTable){
        if(!nam) continue;
        foreach(QWebEngineProfile *profile, nam->Profiles())
            profile->clearAllVisitedLinks();
    }
#endif
}

void NetworkController::InitializeNetworkAccessManager(QString id, const QList<QNetworkCookie> &cookies){
    NetworkCookieJar *ncj = new NetworkCookieJar();
    NetworkAccessManager *nam = new NetworkAccessManager(id);
    ncj->SetAllCookies(cookies);
    nam->SetNetworkCookieJar(ncj);
    m_NetworkAccessManagerTable[id] = nam;
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
        foreach(QJsonValue raw, it.value().toArray()){
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

    foreach(QString backup, list){

        if(!backup.endsWith(filename)) continue;
        if(!LoadCookieFile(datadir + backup)) continue;

        ModelessDialog::Information
            (tr("Restored from a back up file")+ QStringLiteral("\n[") + backup + QStringLiteral("]."),
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
    foreach(QString id, table.keys()){
        QJsonArray rawdata;
        foreach(QNetworkCookie cookie, table[id]->GetNetworkCookieJar()->GetPersistableCookies()){
            if(!ShouldSaveCookie(cookie, saveSessionCookie, now)) continue;
            rawdata.append(QString::fromLatin1(cookie.toRawForm()));
        }
        doc[id] = rawdata;
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

