#include "switch.hpp"
#include "const.hpp"

#include "networkcontroller.hpp"

#include <QSettings>
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
#include <QLocale>
#include <QSslSocket>
#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QTextCodec>
#include <QAbstractButton>
#include <QRegularExpression>
#include <QMimeDatabase>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "saver.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "receiver.hpp"
#include "dialog.hpp"

// NetworkCookieJar
////////////////////////////////////////////////////////////////

NetworkCookieJar::NetworkCookieJar()
    : QNetworkCookieJar(0)
{
}

NetworkCookieJar::~NetworkCookieJar(){}

void NetworkCookieJar::SetAllCookies(const QList<QNetworkCookie> &cookies){
    setAllCookies(cookies);
}

QList<QNetworkCookie> NetworkCookieJar::GetAllCookies(){
    return allCookies();
}

// NetworkAccessManager
////////////////////////////////////////////////////////////////

NetworkAccessManager::NetworkAccessManager(QString id)
    : QNetworkAccessManager(0)
    , m_Id(id)
    , m_UserAgent(QString())
    , m_SslProtocol(QSsl::UnknownProtocol)
    , m_Profile(new QWebEngineProfile(id))
{
    if(Application::SaveSessionCookie())
        m_Profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    else
        m_Profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);

#if QT_VERSION >= 0x050600
    m_Profile->setHttpAcceptLanguage(Application::GetAcceptLanguage());
#endif

    connect(m_Profile, &QWebEngineProfile::downloadRequested,
            this, &NetworkAccessManager::HandleDownload);
    connect(this, &NetworkAccessManager::authenticationRequired,
            this, &NetworkAccessManager::HandleAuthentication);
    connect(this, &NetworkAccessManager::proxyAuthenticationRequired,
            this, &NetworkAccessManager::HandleProxyAuthentication);
}

NetworkAccessManager::~NetworkAccessManager(){
    m_Profile->deleteLater();
}

void NetworkAccessManager::HandleError(QNetworkReply::NetworkError code){
    Q_UNUSED(code);
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
        dialog->SetButtons(QStringList() << tr("Allow") << tr("Block"));
        if(dialog->Execute() && dialog->ClickedButton() == tr("Allow"))
            reply->ignoreSslErrors();
        break;
    }
    case Application::AskForEachHost:
    case Application::AskForEachCertificate:{

        QString host = reply->url().host();

        if(Application::GetBlockedHosts().contains(host)){
            break;
        } else if(Application::GetAllowedHosts().contains(host)){
            reply->ignoreSslErrors();
            break;
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

            dialog->SetButtons(QStringList() << tr("Allow") << tr("Block") << tr("Cancel"));
            dialog->Execute();
            QString text = dialog->ClickedButton();
            if(text == tr("Allow")){
                Application::AppendToAllowedHosts(host);
                reply->ignoreSslErrors();
            } else if(text == tr("Block")){
                Application::AppendToBlockedHosts(host);
            }
            break;
        }
    }
    }
}

void NetworkAccessManager::HandleAuthentication(QNetworkReply *reply,
                                                QAuthenticator *authenticator){
    Q_UNUSED(reply);

    ModalDialog::Authentication(authenticator);
}

void NetworkAccessManager::HandleProxyAuthentication(const QNetworkProxy &proxy,
                                                     QAuthenticator *authenticator){
    Q_UNUSED(proxy);

    ModalDialog::Authentication(authenticator);
}

void NetworkAccessManager::HandleDownload(QWebEngineDownloadItem *orig_item){

    Application::AskDownloadPolicyIfNeed();

    DownloadItem *item = new DownloadItem(orig_item);
    QString dir = Application::GetDownloadDirectory();
    QString filename = orig_item->path().isEmpty() ? dir
        : dir + orig_item->path().split(QStringLiteral("/")).last();

    if(orig_item->path().isEmpty() ||
       Application::GetDownloadPolicy() == Application::Undefined_ ||
       Application::GetDownloadPolicy() == Application::AskForEachDownload){

        QString filter;

        QMimeDatabase db;
#if QT_VERSION >= 0x050600
        QMimeType mimeType = db.mimeTypeForName(orig_item->mimeType());
        if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForFile(filename);
        if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(orig_item->url());
#else
        QMimeType mimeType = db.mimeTypeForFile(filename);
        if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(orig_item->url());
#endif

        if(mimeType.isValid() && !mimeType.isDefault()) filter = mimeType.filterString();

        filename = ModalDialog::GetSaveFileName_(QString::null, filename, filter);
    }

    if(filename.isEmpty()){
        orig_item->cancel();
        item->deleteLater();
        return;
    }

    item->SetPath(filename);
    orig_item->setPath(filename);
    orig_item->accept();
    MainWindow *win = Application::GetCurrentWindow();
    if(win && win->GetTreeBank()->GetNotifier())
        win->GetTreeBank()->GetNotifier()->RegisterDownload(item);

    QStringList path = filename.split(QStringLiteral("/"));
    path.removeLast();
    Application::SetDownloadDirectory(path.join(QStringLiteral("/")) + QStringLiteral("/"));
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op,
                                                   const QNetworkRequest &req,
                                                   QIODevice *out){
    QNetworkRequest newreq(req);

    if(m_SslProtocol != QSsl::UnknownProtocol &&
       (req.url().scheme() == QStringLiteral("https") || req.url().scheme() == QStringLiteral("ftps"))){
        QSslConfiguration sslConfig = newreq.sslConfiguration();
        sslConfig.setProtocol(m_SslProtocol);
        newreq.setSslConfiguration(sslConfig);
    }

    QNetworkReply *rep = QNetworkAccessManager::createRequest(op, newreq, out);

    // QNetworkReply::error is overloaded.
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

void NetworkAccessManager::SetNetworkCookieJar(NetworkCookieJar *ncj){
    setCookieJar(ncj);
}

NetworkCookieJar *NetworkAccessManager::GetNetworkCookieJar() const {
    return static_cast<NetworkCookieJar*>(cookieJar());
}

void NetworkAccessManager::SetUserAgent(QString ua){
    ua = ua.split(QStringLiteral(" ")).last();

    QString system;

#if defined(Q_OS_WIN)
    switch(QSysInfo::WindowsVersion){
    case QSysInfo::WV_32s:        system = QStringLiteral("Windows 3.1");             break;
    case QSysInfo::WV_95:         system = QStringLiteral("Windows 95");              break;
    case QSysInfo::WV_98:         system = QStringLiteral("Windows 98");              break;
    case QSysInfo::WV_Me:         system = QStringLiteral("Windows 98; Win 9x 4.90"); break;
    case QSysInfo::WV_NT:         system = QStringLiteral("WinNT4.0");                break;
    case QSysInfo::WV_2000:       system = QStringLiteral("Windows NT 5.0");          break;
    case QSysInfo::WV_XP:         system = QStringLiteral("Windows NT 5.1");          break;
    case QSysInfo::WV_2003:       system = QStringLiteral("Windows NT 5.2");          break;
    case QSysInfo::WV_VISTA:      system = QStringLiteral("Windows NT 6.0");          break;
    case QSysInfo::WV_WINDOWS7:   system = QStringLiteral("Windows NT 6.1");          break;
    case QSysInfo::WV_WINDOWS8:   system = QStringLiteral("Windows NT 6.2");          break;
    case QSysInfo::WV_WINDOWS8_1: system = QStringLiteral("Windows NT 6.3");          break;
    case QSysInfo::WV_WINDOWS10:  system = QStringLiteral("Windows NT 10.0");         break;
    case QSysInfo::WV_CE:         system = QStringLiteral("Windows CE");              break;
    case QSysInfo::WV_CENET:      system = QStringLiteral("Windows CE .NET");         break;
    case QSysInfo::WV_CE_5:       system = QStringLiteral("Windows CE 5.x");          break;
    case QSysInfo::WV_CE_6:       system = QStringLiteral("Windows CE 6.x");          break;
    default:                      system = QStringLiteral("Windows NT based");
    }
#  if _WIN64
    // OS:64bit Application:64bit
    system = system + QStringLiteral("; Win64; x64");
#  elif _WIN32
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    BOOL isWow64 = FALSE;
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
        GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(fnIsWow64Process){
        if(fnIsWow64Process(GetCurrentProcess(), &isWow64)){
            if(isWow64){
                // OS:64bit Application:32bit
                system = system + QStringLiteral("; WOW64");
            } else {
                // OS:32bit Application:32bit
                /* do nothing. */
            }
        } else {
            // cannot specify system.
        }
    } else {
        // cannot specify system.
    }
#  else
#  endif
#elif defined(Q_OS_MAC)
    switch(QSysInfo::MacintoshVersion){
    case QSysInfo::MV_10_6:  system = QStringLiteral("Intel MacOS X 10.6");  break;
    case QSysInfo::MV_10_7:  system = QStringLiteral("Intel MacOS X 10.7");  break;
    case QSysInfo::MV_10_8:  system = QStringLiteral("Intel MacOS X 10.8");  break;
    case QSysInfo::MV_10_9:  system = QStringLiteral("Intel MacOS X 10.9");  break;
    case QSysInfo::MV_10_10: system = QStringLiteral("Intel MacOS X 10.10"); break;
    case QSysInfo::MV_10_11: system = QStringLiteral("Intel MacOS X 10.11"); break;
    default:                 system = QStringLiteral("MacOS X");
    }
#else
    system = QStringLiteral("Linux/Unix");
#endif

    QString location = QLocale::system().name().replace(QStringLiteral("_"), QStringLiteral("-"));

    if(ua.isEmpty()) return;
    else if(Application::ExactMatch(QStringLiteral("[iI](?:nternet)?[eE](?:xplorer)?"), ua)){ ua = Application::UserAgent_IE();}
    else if(Application::ExactMatch(QStringLiteral("[eE]dge"), ua))         { ua = Application::UserAgent_Edge();}
    else if(Application::ExactMatch(QStringLiteral("[fF](?:ire)?[fF](?:ox)?"), ua)){ ua = Application::UserAgent_FF();}
    else if(Application::ExactMatch(QStringLiteral("[oO]pera"), ua))        { ua = Application::UserAgent_Opera();}
    else if(Application::ExactMatch(QStringLiteral("[oO][pP][rR]"), ua))    { ua = Application::UserAgent_OPR();}
    else if(Application::ExactMatch(QStringLiteral("[sS]afari"), ua))       { ua = Application::UserAgent_Safari();}
    else if(Application::ExactMatch(QStringLiteral("[cC]hrome"), ua))       { ua = Application::UserAgent_Chrome();}
    else if(Application::ExactMatch(QStringLiteral("[sS]leipnir"), ua))     { ua = Application::UserAgent_Sleipnir();}
    else if(Application::ExactMatch(QStringLiteral("[vV]ivaldi"), ua))      { ua = Application::UserAgent_Vivaldi();}
    else if(Application::ExactMatch(QStringLiteral("[nN]et[sS]cape"), ua))  { ua = Application::UserAgent_NetScape();}
    else if(Application::ExactMatch(QStringLiteral("[sS]ea[mM]onkey"), ua)) { ua = Application::UserAgent_SeaMonkey();}
    else if(Application::ExactMatch(QStringLiteral("[gG]ecko"), ua))        { ua = Application::UserAgent_Gecko();}
    else if(Application::ExactMatch(QStringLiteral("[iI][cC]ab"), ua))      { ua = Application::UserAgent_iCab();}
    else if(Application::ExactMatch(QStringLiteral("[cC]amino"), ua))       { ua = Application::UserAgent_Camino();}
    else if(Application::ExactMatch(QStringLiteral("[cC]ustom"), ua))       { ua = Application::UserAgent_Custom();}

    ua = ua.replace(QStringLiteral("%SYSTEM%"), system).replace(QStringLiteral("%LOCATION%"), location);

    // for inline user agent.
    ua = QUrl::fromPercentEncoding(ua.toLatin1());

    m_UserAgent = ua;

    m_Profile->setHttpUserAgent(ua);
}

QString NetworkAccessManager::GetUserAgent() const {
    return m_UserAgent;
}

// proxy type user:pass@host:port;
// proxy type user@host:port;
// proxy type user@host;
// proxy type user:pass@host;
// proxy type host:port;
// proxy type host;
//
// proxy user:pass@host:port;
// proxy user@host:port;
// proxy user@host;
// proxy user:pass@host;
// proxy host:port;
// proxy host;
//
// type: default, socks, http, httpcaching, ftpcaching

void NetworkAccessManager::SetProxy(QString proxySet){
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
        proxy.setPort(port);
        setProxy(proxy);
    }
}

void NetworkAccessManager::SetSslProtocol(QString sslSet){
    QString version = sslSet.split(QStringLiteral(" ")).last();

    if     (Application::ExactMatch(QStringLiteral("[sS][sS][lL][vV](?:ersion)?3(?:.0)?"), version))
        m_SslProtocol = QSsl::SslV3;
    else if(Application::ExactMatch(QStringLiteral("[sS][sS][lL][vV](?:ersion)?2(?:.0)?"), version))
        m_SslProtocol = QSsl::SslV2;
    else if(Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1(?:.0)?"), version))
        m_SslProtocol = QSsl::TlsV1_0;
    else if(Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1.1"), version))
        m_SslProtocol = QSsl::TlsV1_1;
    else if(Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1.2"), version))
        m_SslProtocol = QSsl::TlsV1_2;
    else if(Application::ExactMatch(QStringLiteral("[aA]ny(?:[pP]rotocol)?"), version))
        m_SslProtocol = QSsl::AnyProtocol;
    else if(Application::ExactMatch(QStringLiteral("[sS]ecure(?:[pP]rotocol)?"), version))
        m_SslProtocol = QSsl::SecureProtocols;
    else if(Application::ExactMatch(QStringLiteral("[tT][lL][sS][vV](?:ersion)?1(?:.0)?"
                                                 VV"[sS][sS][lL][vV](?:ersion)?3(?:.0)?"), version))
        m_SslProtocol = QSsl::TlsV1SslV3;
    else
        m_SslProtocol = QSsl::UnknownProtocol;
}

void NetworkAccessManager::SetOffTheRecord(QString offTheRecordSet){
    if       (!m_Profile->isOffTheRecord() && Application::ExactMatch(QStringLiteral( "(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"), offTheRecordSet)){
        m_Profile->deleteLater(); m_Profile = new QWebEngineProfile();
    } else if( m_Profile->isOffTheRecord() && Application::ExactMatch(QStringLiteral("!(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)"), offTheRecordSet)){
        m_Profile->deleteLater(); m_Profile = new QWebEngineProfile(m_Id);
    }
}

QWebEngineProfile *NetworkAccessManager::GetProfile() const {
    return m_Profile;
}

// DownloadItem
////////////////////////////////////////////////////////////////

DownloadItem::DownloadItem(QNetworkReply *reply, QString defaultfilename)
    : QObject(0)
{
    // 'CreateDefaultFromReplyOrRequest' uses 'm_DefaultFileName'.
    m_DefaultFileName = QString();

    m_DownloadReply = reply;
    m_DownloadItem = 0;
    connect(m_DownloadReply, &QNetworkReply::readyRead, this, &DownloadItem::ReadyRead);
    connect(m_DownloadReply, &QNetworkReply::finished,  this, &DownloadItem::Finished);
    connect(m_DownloadReply, &QNetworkReply::downloadProgress,
            this, &DownloadItem::DownloadProgress);
    m_GettingPath = false;

    if(defaultfilename.isEmpty()){
        defaultfilename = reply->url().toString().split(QStringLiteral("/")).last().trimmed();
    }
    if(defaultfilename.isEmpty()){
        defaultfilename = reply->request().url().toString().split(QStringLiteral("/")).last().trimmed();
    }
    if(defaultfilename.isEmpty()){
        defaultfilename = CreateDefaultFromReplyOrRequest();
    }
    m_DefaultFileName = defaultfilename;
    m_RemoteUrl = QUrl();
    m_Path = QString();
    m_BAOut = QByteArray();
    m_FinishedFlag = false;
}

DownloadItem::DownloadItem(QWebEngineDownloadItem *item)
    : QObject(0)
{
    m_DefaultFileName = QString();
    m_DownloadItem = item;
    m_DownloadReply = 0;
    connect(m_DownloadItem, &QWebEngineDownloadItem::finished,
            this,           &DownloadItem::Finished);
    connect(m_DownloadItem, &QWebEngineDownloadItem::downloadProgress,
            this,           &DownloadItem::DownloadProgress);
    // for disabling ReadyRead.
    m_GettingPath = true;
    m_RemoteUrl = item->url();
    m_Path = item->path();
    m_BAOut = QByteArray();
    m_FinishedFlag = false;
}

DownloadItem::~DownloadItem(){
    NetworkController::RemoveItem(this);
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

    m_FileOut.setFileName(m_Path);
    m_FileOut.open(QIODevice::WriteOnly);
}

QString DownloadItem::CreateDefaultFromReplyOrRequest(){
    QByteArray ba;
    QString filename;

    QVariant dispositionData =
        m_DownloadReply->header(QNetworkRequest::ContentDispositionHeader);

    if(dispositionData.isValid()) ba = dispositionData.toByteArray();
    else if(m_DownloadReply->hasRawHeader("Content-Disposition"))
        ba = m_DownloadReply->rawHeader("Content-Disposition");
    else if(m_DownloadReply->request().hasRawHeader("Content-Disposition"))
        ba = m_DownloadReply->request().rawHeader("Content-Disposition");
    else if(m_DownloadReply->hasRawHeader("content-disposition"))
        ba = m_DownloadReply->rawHeader("content-disposition");
    else if(m_DownloadReply->request().hasRawHeader("content-disposition"))
        ba = m_DownloadReply->request().rawHeader("content-disposition");

    if(!ba.isEmpty()){

        QString value = QTextCodec::codecForName("UTF-8")->toUnicode(ba);
        QStringList list = value.split(QStringLiteral(";"));

        foreach(QString cand, list){
            cand = cand.trimmed();

            int pos;
            QString name;

            if(cand.startsWith(QStringLiteral("filename"))){

                pos = cand.indexOf(QStringLiteral("filename="));
                if(pos != -1) name = cand.mid(pos + 9).trimmed();

                pos = cand.indexOf(QStringLiteral("filename ="));
                if(pos != -1) name = cand.mid(pos + 10).trimmed();

            } else if(cand.startsWith(QStringLiteral("name"))){

                pos = cand.indexOf(QStringLiteral("name="));
                if(pos != -1) name = cand.mid(pos + 5).trimmed();

                pos = cand.indexOf(QStringLiteral("name ="));
                if(pos != -1) name = cand.mid(pos + 6).trimmed();
            }

            if(!name.isEmpty()){
                if((name.startsWith(QStringLiteral("\"")) && name.endsWith(QStringLiteral("\""))) ||
                   (name.startsWith(QStringLiteral("'"))  && name.endsWith(QStringLiteral("'")))){
                    filename = name.mid(1, name.length() - 2);
                } else {
                    filename = name.mid(0, name.length());
                }
            }
        }
    }

    if(filename.isEmpty())
        filename = m_DefaultFileName;

    return Application::GetDownloadDirectory() + filename;
}

void DownloadItem::ReadyRead(){
    if(m_GettingPath) return;

    if(m_Path == DISABLE_FILENAME) {
        m_BAOut = m_BAOut + m_DownloadReply->readAll();
        return;
    }

    if(m_Path.isEmpty()){
        m_GettingPath = true;

        Application::AskDownloadPolicyIfNeed();

        // it's too slow, depending on default directory. can't help it.
        QString filename = CreateDefaultFromReplyOrRequest();

        if(filename.isEmpty() ||
           Application::GetDownloadPolicy() == Application::Undefined_ ||
           Application::GetDownloadPolicy() == Application::AskForEachDownload){

            QByteArray ba;
            QVariant typeData =
                m_DownloadReply->header(QNetworkRequest::ContentTypeHeader);

            if(typeData.isValid()) ba = typeData.toByteArray();
            else if(m_DownloadReply->hasRawHeader("Content-Type"))
                ba = m_DownloadReply->rawHeader("Content-Type");
            else if(m_DownloadReply->request().hasRawHeader("Content-Type"))
                ba = m_DownloadReply->request().rawHeader("Content-Type");
            else if(m_DownloadReply->hasRawHeader("content-type"))
                ba = m_DownloadReply->rawHeader("content-type");
            else if(m_DownloadReply->request().hasRawHeader("content-type"))
                ba = m_DownloadReply->request().rawHeader("content-type");

            QString filter;

            QMimeDatabase db;
            QMimeType mimeType;
            if(!ba.isEmpty()) mimeType = db.mimeTypeForName(QString::fromUtf8(ba));
            if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForFile(filename);
            if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(m_DownloadReply->url());
            if(!mimeType.isValid() || mimeType.isDefault()) mimeType = db.mimeTypeForUrl(m_DownloadReply->request().url());

            if(mimeType.isValid() && !mimeType.isDefault()) filter = mimeType.filterString();

            filename = ModalDialog::GetSaveFileName_(QString::null, filename, filter);
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
    // 'ReadyRead' must be called at least once.
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
    if(m_FileOut.isOpen()) m_FileOut.close();
    if(m_DownloadReply) m_DownloadReply->abort();
    if(m_DownloadItem) m_DownloadItem->cancel();
    disconnect();
    m_FinishedFlag = true;
    if(!m_Path.isEmpty()){
        deleteLater();
    }
}

void DownloadItem::DownloadProgress(qint64 received, qint64 total){
    if(!m_Path.isEmpty()){
        if(total != -1){
            emit Progress(m_Path, received, total);
        } else {
            qint64 dummy = 10;
            qint64 r = received;
            while((r /= 10) > 0) dummy *= 10;
            emit Progress(m_Path, received, dummy);
        }
    }

    if(!received && !total) return;

    if(received == total
       || (m_DownloadReply && m_DownloadReply->isFinished())
       || (m_DownloadItem && m_DownloadItem->isFinished())){

        Finished();
    }
}

// UploadItem
////////////////////////////////////////////////////////////////

int UploadItem::m_UnknownCount = 0;

UploadItem::UploadItem(QNetworkReply *reply, qint64 size)
    : QObject(0)
{
    m_UploadReply = reply;
    connect(m_UploadReply, &QNetworkReply::finished, this, &UploadItem::Finished);
    connect(m_UploadReply, &QNetworkReply::uploadProgress,
            this, &UploadItem::UploadProgress);
    m_FileSize = size;
    m_Path = ExpectFileName();
}

UploadItem::UploadItem(QNetworkReply *reply, QString name)
    : QObject(0)
{
    m_UploadReply = reply;
    connect(m_UploadReply, &QNetworkReply::finished, this, &UploadItem::Finished);
    connect(m_UploadReply, &QNetworkReply::uploadProgress,
            this, &UploadItem::UploadProgress);
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
    qint64 max = 500 + m_FileSize / 20000; // roughly threshold.
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

// NetworkController
////////////////////////////////////////////////////////////////

QMap<QString, NetworkAccessManager*> NetworkController::m_NetworkAccessManagerTable = QMap<QString, NetworkAccessManager*>();
QList<DownloadItem*> NetworkController::m_DownloadList = QList<DownloadItem*>();
QList<UploadItem*>   NetworkController::m_UploadList   = QList<UploadItem*>();

NetworkController::NetworkController()
    : QObject(0)
{
    LoadAllCookies();

    // first download is too slow.
    Application::ClearTemporaryDirectory();
    QMap<QString, NetworkAccessManager*> nams = m_NetworkAccessManagerTable;
    if(!nams.isEmpty()){
        Download(nams.first(), QUrl(QStringLiteral("about:blank")), QUrl(), NetworkController::TemporaryDirectory);
    }
}

NetworkController::~NetworkController(){}

DownloadItem *NetworkController::Download(NetworkAccessManager *nam,
                                          QUrl &url, QUrl &referer,
                                          DownloadType type){
    QNetworkRequest req(url);
    req.setRawHeader("Referer", referer.toEncoded());
    DownloadItem *item = Download(nam, req, type);
    item->SetRemoteUrl(url);

    return item;
}

DownloadItem* NetworkController::Download(NetworkAccessManager *nam,
                                          const QNetworkRequest &request,
                                          DownloadType type){
    if(request.url().isEmpty()) return 0;

    return Download(nam->get(request),
                    request.url().toString().split(QStringLiteral("/")).last().trimmed(), type);
}

DownloadItem* NetworkController::Download(QNetworkReply *reply,
                                          QString filename,
                                          DownloadType type){
    QVariant lengthHeader = reply->header(QNetworkRequest::ContentLengthHeader);
    bool ok;
    int size = lengthHeader.toInt(&ok);
    if(ok && size == 0) return 0;
    DownloadItem *item = new DownloadItem(reply, filename);
    switch (type){
    case SelectedDirectory : {
        break;
    }
    case TemporaryDirectory : {
        if(filename.isEmpty()){
            item->SetPathAndReady(Application::TemporaryDirectory() + QStringLiteral("index.html"));
        } else {
            item->SetPathAndReady(Application::TemporaryDirectory() +
                                  filename.replace(QRegularExpression(QStringLiteral("[\\/:,;*?\"<>\\|]")),
                                                   QString()));
        }
        break;
    }
    case ToVariable : {
        item->SetPathAndReady(DISABLE_FILENAME);
        break;
    }
    default : break;
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

void NetworkController::SetOffTheRecord(NetworkAccessManager *nam, QStringList set){
    int pos = set.indexOf(QRegularExpression(QStringLiteral("\\A!?(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)\\Z")));
    if(pos == -1) return;
    nam->SetOffTheRecord(set[pos]);
}

NetworkAccessManager* NetworkController::GetNetworkAccessManager(QString id, QStringList set){
    if(!m_NetworkAccessManagerTable[id])
        InitializeNetworkAccessManager(id);
    NetworkAccessManager *nam = m_NetworkAccessManagerTable[id];
    SetUserAgent(nam, set);
    SetProxy(nam, set);
    SetSslProtocol(nam, set);
    SetOffTheRecord(nam, set);
    return nam;
}

NetworkAccessManager* NetworkController::CopyNetworkAccessManager(QString bef, QString aft, QStringList set){
    NetworkAccessManager *bnam = GetNetworkAccessManager(bef);
    InitializeNetworkAccessManager(aft, bnam->GetNetworkCookieJar()->GetAllCookies());
    NetworkAccessManager *nam = m_NetworkAccessManagerTable[aft];
    nam->setProxy(bnam->proxy());
    SetUserAgent(nam, set);
    SetProxy(nam, set);
    SetSslProtocol(nam, set);
    SetOffTheRecord(nam, set);
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
    SetOffTheRecord(nam, set);
    return nam;
}

NetworkAccessManager* NetworkController::MergeNetworkAccessManager(QString bef, QString aft, QStringList set){
    // make if not exists.
    GetNetworkAccessManager(bef);
    GetNetworkAccessManager(aft, set);
    NetworkCookieJar *b = m_NetworkAccessManagerTable[bef]->GetNetworkCookieJar();
    NetworkCookieJar *a = m_NetworkAccessManagerTable[aft]->GetNetworkCookieJar();
    a->SetAllCookies(a->GetAllCookies() + b->GetAllCookies());
    m_NetworkAccessManagerTable.remove(bef);
    return m_NetworkAccessManagerTable[aft];
}

NetworkAccessManager* NetworkController::KillNetworkAccessManager(QString id){
    //delete NAMtable[id];
    m_NetworkAccessManagerTable.remove(id);
    return 0;
}

QMap<QString, NetworkAccessManager*> NetworkController::AllNetworkAccessManager(){
    return m_NetworkAccessManagerTable;
}

void NetworkController::InitializeNetworkAccessManager(QString id, const QList<QNetworkCookie> &cookies){
    NetworkCookieJar *ncj = new NetworkCookieJar();
    NetworkAccessManager *nam = new NetworkAccessManager(id);
    ncj->SetAllCookies(cookies);
    nam->SetNetworkCookieJar(ncj);
    m_NetworkAccessManagerTable[id] = nam;
}

void NetworkController::LoadAllCookies(){
    QString filename = Application::CookieFileName();
    QString datadir = Application::DataDirectory();
    QDomDocument doc;
    QFile file(datadir + filename);
    bool check = doc.setContent(&file);
    file.close();

    if(!check){

        QDir dir = QDir(datadir);
        QStringList list =
            dir.entryList(Application::BackUpFileFilters(),
                          QDir::NoFilter, QDir::Name | QDir::Reversed);
        if(list.isEmpty()) return;

        foreach(QString backup, list){

            if(!backup.contains(filename)) continue;

            QFile backupfile(datadir + backup);
            check = doc.setContent(&backupfile);
            backupfile.close();

            if(!check) continue;

            ModelessDialog::Information
                (tr("Restored from a back up file")+ QStringLiteral(" [") + backup + QStringLiteral("]."),
                 tr("Because of a failure to read the latest file, it was restored from a backup file."));
            break;
        }
    }

    QDomNodeList children = doc.documentElement().childNodes();
    for(uint i = 0; i < static_cast<uint>(children.length()); i++){
        QDomElement child = children.item(i).toElement();
        InitializeNetworkAccessManager
            (child.attribute(QStringLiteral("id")),
             QNetworkCookie::parseCookies(child.attribute(QStringLiteral("body")).replace(QStringLiteral("\\0\\0\\0") , QStringLiteral("\n")).toLatin1()));
    }
}

void NetworkController::SaveAllCookies(){
    QString datadir = Application::DataDirectory();

    QString cookie  = datadir + Application::CookieFileName(false);
    QString cookieb = datadir + Application::CookieFileName(true);

    if(QFile::exists(cookieb)) QFile::remove(cookieb);

    QMap<QString, NetworkAccessManager*> table = AllNetworkAccessManager();
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
    QDomElement root = doc.createElement(QStringLiteral("cookietable"));
    doc.appendChild(root);
    foreach(QString id, table.keys()){
        QDomElement child = doc.createElement(QStringLiteral("cookie"));
        QByteArray rawdata;
        foreach(QNetworkCookie cookie, table[id]->GetNetworkCookieJar()->GetAllCookies()){
            if(cookie.isSessionCookie() && !Application::SaveSessionCookie()) continue;
            if(cookie.expirationDate().toLocalTime() < QDateTime::currentDateTime() &&
               cookie.expirationDate().toUTC()       < QDateTime::currentDateTime()) continue;
            rawdata.append(cookie.toRawForm() + "\\0\\0\\0");
        }
        child.setAttribute(QStringLiteral("body"), QVariant(rawdata).toString());
        child.setAttribute(QStringLiteral("id"), id);
        root.appendChild(child);
    }

    QFile file(cookieb);
    if(file.open(QIODevice::WriteOnly)){
        QTextStream out(&file);
        doc.save(out, 2);
    }
    file.close();

    if(QFile::exists(cookie)) QFile::remove(cookie);

    QFile::rename(cookieb, cookie);
}

