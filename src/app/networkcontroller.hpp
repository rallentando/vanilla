#ifndef NETCONTROL_HPP
#define NETCONTROL_HPP

#include "switch.hpp"
#include "const.hpp"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QPointer>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QObject>
#include <QRegularExpression>
#include <QHash>
#include <QSet>
#ifdef WEBENGINEVIEW
#  include <QWebEngineUrlRequestInterceptor>
#endif

#include <QFile>
#include <QStringList>

#include <memory>

#ifdef WEBENGINEVIEW
class QWebEngineProfile;
class QQuickWebEngineProfile;
typedef std::shared_ptr<QWebEngineProfile> SharedProfile;
#endif

class NetworkCookieJar : public QNetworkCookieJar {
    Q_OBJECT

public:
    NetworkCookieJar();
    ~NetworkCookieJar();
    void SetAllCookies(const QList<QNetworkCookie> &cookies);
    QList<QNetworkCookie> GetAllCookies();

    void MirrorCookie(const QString &source, const QNetworkCookie &cookie);
    void UnmirrorCookie(const QString &source, const QNetworkCookie &cookie);

    quint64 BeginMirrorScope(const QString &source, const QUrl &url);
    void MirrorScope(const QString &source, const QUrl &url, quint64 ticket,
                     const QList<QNetworkCookie> &fresh);

    QList<QNetworkCookie> GetPersistableCookies();

    static QString Identity(const QNetworkCookie &cookie);

protected:

    friend class tst_cookiefilter;

    bool insertCookie(const QNetworkCookie &cookie) Q_DECL_OVERRIDE;
    bool updateCookie(const QNetworkCookie &cookie) Q_DECL_OVERRIDE;
    bool deleteCookie(const QNetworkCookie &cookie) Q_DECL_OVERRIDE;

private:
    QHash<QString, QString> m_Mirrored;
    QHash<QString, quint64> m_ScopeTicket;
};

#ifdef WEBENGINEVIEW

class RequestInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT

public:
    RequestInterceptor(QObject *parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) Q_DECL_OVERRIDE;

    static RequestInterceptor *Instance();
};
#endif

class UrlBlockRules {

public:
    static void ReloadRules();

    static bool SendDoNotTrack();
    static bool IsBlocked(const QString &url);

    static QList<QRegularExpression> Compile(const QStringList &patterns);
    static bool Matches(const QList<QRegularExpression> &rules, const QString &url);

private:
    static QList<QRegularExpression> m_Blocked;
    static bool m_SendDoNotTrack;
};

class NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT

public:
    NetworkAccessManager(QString id);
    ~NetworkAccessManager() Q_DECL_OVERRIDE;
    QNetworkReply* createRequest(Operation op, const QNetworkRequest &req, QIODevice *out = nullptr) Q_DECL_OVERRIDE;
    QString GetId();
    void SetNetworkCookieJar(NetworkCookieJar *);
    NetworkCookieJar *GetNetworkCookieJar() const;
    void SetUserAgent(QString ua);
    QString GetUserAgent() const;

    QString ResolvedUserAgent() const;

    static QNetworkRequest CompletedRequest(const QNetworkRequest &request,
                                            const QString &userAgent);

    enum RequestPurpose {
        Subresource = 0,
        Navigation  = 1,
    };
    static void SetRequestPurpose(QNetworkRequest &request, RequestPurpose purpose);
    static RequestPurpose PurposeOf(const QNetworkRequest &request);
    void SetProxy(QString proxySet);
    void SetSslProtocol(QString sslSet);
    static QSsl::SslProtocol SslProtocolForSetting(const QString &sslSet);
    void SetOffTheRecord(QString offTheRecordSet);
#ifdef WEBENGINEVIEW
    QWebEngineProfile *GetProfile() const;
    SharedProfile GetSharedProfile() const;
#endif

private slots:
    void HandleError(QNetworkReply::NetworkError code);
    void HandleSslErrors(const QList<QSslError> &errors);

    void HandleAuthentication(QNetworkReply *reply,
                              QAuthenticator *authenticator);
#ifndef QT_NO_NETWORKPROXY
    void HandleProxyAuthentication(const QNetworkProxy &proxy,
                                   QAuthenticator *authenticator);
#endif
public slots:
    void HandleDownload(QObject *download);

private:
    QString m_Id;
    QString m_UserAgent;
    QString m_UserAgentBrand;
    QString m_UserAgentFullVersion;
    QSsl::SslProtocol m_SslProtocol;
#ifdef WEBENGINEVIEW
    SharedProfile m_Profile;
    QMetaObject::Connection m_CookieMirrorAdded;
    QMetaObject::Connection m_CookieMirrorRemoved;
    void SetupProfile();
    void SetupClientHints(const QString &browser, const QString &ua,
                          const QString &full);
#endif
    static const QList<QEvent::Type> m_EventTypes;
};

class DownloadItem : public QObject {
    Q_OBJECT

public:

    DownloadItem(QNetworkReply *reply, QString defaultfilename);
    DownloadItem(QObject *object);
    ~DownloadItem();
    void SetRemoteUrl(QUrl url);
    QUrl GetRemoteUrl() const;
    QUrl GetLocalUrl() const;
    QList<QUrl> GetUrls() const;

    QString GetPath() const;
    void SetPath(QString name);
    void SetPathAndReady(QString name);

private:
    friend class tst_downloaditem;

    QByteArray HeaderOfReplyOrRequest(QNetworkRequest::KnownHeaders known,
                                      const char *name,
                                      const char *lowercase) const;
    QString ContentTypeOfReplyOrRequest() const;
    QString CreateDefaultFromReplyOrRequest();

    QPointer<QNetworkReply> m_DownloadReply = nullptr;
    QObject *m_DownloadItem = nullptr;

    bool       m_GettingPath = false;
    QString    m_Path;
    QString    m_DefaultFileName;
    QFile      m_FileOut;
    QByteArray m_BAOut;
    QUrl       m_RemoteUrl;
    bool       m_FinishedFlag = false;

private slots:
#ifdef WEBENGINEVIEW
    void StateChanged();
    void ReceivedBytesChanged();
#endif

    void ReadyRead();
    void Finished();
    void DownloadProgress(qint64 received, qint64 total);

public slots:
    void Stop();

signals:
    void Progress(QString, qint64, qint64);
    void DownloadResult(const QByteArray&);
};

class UploadItem : public QObject {
    Q_OBJECT

public:
    UploadItem(QNetworkReply *reply, qint64 size);
    UploadItem(QNetworkReply *reply, QString name = QString());
    ~UploadItem();
    QString GetPath() const;
    void SetPath(QString name);

private:
    QString ExpectFileName();
    QNetworkReply *m_UploadReply;
    QString        m_Path;
    qint64         m_FileSize;
    static int     m_UnknownCount;

private slots:
    void Finished();
    void UploadProgress(qint64 sent, qint64 total);

public slots:
    void Stop();

signals:
    void Progress(QString, qint64, qint64);
};

class NetworkController : public QObject {
    Q_OBJECT

public:
    NetworkController();
    ~NetworkController();

    enum DownloadType {
        SelectedDirectory,
        TemporaryDirectory,
        ToVariable
    };

    static DownloadItem* Download(NetworkAccessManager *nam, const QUrl &url,
                                  const QUrl &referer = EMPTY_URL,
                                  DownloadType type = SelectedDirectory);
    static DownloadItem* Download(NetworkAccessManager *nam,
                                  const QNetworkRequest &request,
                                  DownloadType type = SelectedDirectory);
    static UploadItem* Upload(QNetworkReply *reply, qint64 filesize);
    static UploadItem* Upload(QNetworkReply *reply, QString filename);
    static void RemoveItem(DownloadItem *item);
    static void RemoveItem(UploadItem *item);

    static void SetUserAgent(NetworkAccessManager *nam, QStringList set);
    static void SetProxy(NetworkAccessManager *nam, QStringList set);
    static void SetSslProtocol(NetworkAccessManager *nam, QStringList set);
    static void SetOffTheRecord(NetworkAccessManager *nam, QStringList set);

#ifdef WEBENGINEVIEW
    static QWebEngineProfile* InspectorProfile();

    static QQuickWebEngineProfile* QuickInspectorProfile();

    static QQuickWebEngineProfile* QuickProfile(const QString &id);
    static QQuickWebEngineProfile* QuickPrivateProfile(const QString &id);
    static void ApplyQuickPermissionsPolicy(QQuickWebEngineProfile *profile);
    static void ApplyQuickBlockRules(QQuickWebEngineProfile *profile);
    static void ApplyQuickCommonSettings(QQuickWebEngineProfile *profile);
    static void ConnectQuickNotifications(QQuickWebEngineProfile *profile);

    static QString ProfileKey(const QObject *profile);

#endif
    static void ClearCookies();
    static void ClearHttpCache();
    static void ClearVisitedLinks();

    static QString ProfileStorageName(const QString &id);
    static QString ProfileStorageName(const QString &applicationDir, const QString &id);

    static NetworkAccessManager* GetNetworkAccessManager(QString id, QStringList set = QStringList());
    static NetworkAccessManager* FindNetworkAccessManager(const QString &id);
    static NetworkAccessManager* CopyNetworkAccessManager(QString bef, QString aft, QStringList set);
    static NetworkAccessManager* MoveNetworkAccessManager(QString bef, QString aft, QStringList set);
    static NetworkAccessManager* MergeNetworkAccessManager(QString bef, QString aft, QStringList set);
    static NetworkAccessManager* KillNetworkAccessManager(QString id);
    static QMap<QString, NetworkAccessManager*> AllNetworkAccessManager();
    static void InitializeNetworkAccessManager(QString id, const QList<QNetworkCookie> &cookies);
    static bool LoadCookieFile(QString path);
    static bool LoadLegacyCookieFile(QString path);
    static void LoadAllCookies();
    static void SaveAllCookies();
    static bool ShouldSaveCookie(const QNetworkCookie &cookie,
                                 bool saveSessionCookie, const QDateTime &now);

private:
    friend class tst_downloaditem;

    static DownloadItem* Download(QNetworkReply *reply,
                                  QString filename = QString(),
                                  DownloadType type = SelectedDirectory);

    static QMap<QString, NetworkAccessManager*> m_NetworkAccessManagerTable;
    static QList<DownloadItem*> m_DownloadList;
    static QList<UploadItem*>   m_UploadList;
};

#endif
