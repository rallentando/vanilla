#ifndef NETCONTROL_HPP
#define NETCONTROL_HPP

#include "switch.hpp"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QObject>

#include <QFile>
#include <QStringList> // default argument

class QWebEngineProfile;
class QWebEngineDownloadItem;

// NetworkCookieJar
////////////////////////////////////////////////////////////////

class NetworkCookieJar : public QNetworkCookieJar{
    Q_OBJECT

public:
    NetworkCookieJar();
    ~NetworkCookieJar();
    void SetAllCookies(const QList<QNetworkCookie> &cookies);
    QList<QNetworkCookie> GetAllCookies();
};

// NetworkAccessManager
////////////////////////////////////////////////////////////////

class NetworkAccessManager : public QNetworkAccessManager{
    Q_OBJECT

public:
    NetworkAccessManager(QString id);
    ~NetworkAccessManager();
    QNetworkReply* createRequest(Operation op, const QNetworkRequest &req, QIODevice *out = 0) DECL_OVERRIDE;
    void SetNetworkCookieJar(NetworkCookieJar *);
    NetworkCookieJar *GetNetworkCookieJar() const;
    void SetUserAgent(QString ua);
    QString GetUserAgent() const;
    void SetProxy(QString proxySet);
    void SetSslProtocol(QString sslSet);
    void SetOffTheRecord(QString offTheRecordSet);
    QWebEngineProfile *GetProfile() const;

private slots:
    void HandleError(QNetworkReply::NetworkError code);
    void HandleSslErrors(const QList<QSslError> &errors);

    void HandleAuthentication(QNetworkReply *reply,
                              QAuthenticator *authenticator);
    void HandleProxyAuthentication(const QNetworkProxy &proxy,
                                   QAuthenticator *authenticator);
    void HandleDownload(QWebEngineDownloadItem *item);

private:
    QString m_Id;
    QString m_UserAgent;
    QSsl::SslProtocol m_SslProtocol;
    QWebEngineProfile *m_Profile;
};

// DownloadItem
////////////////////////////////////////////////////////////////

class DownloadItem : public QObject{
    Q_OBJECT

public:
    DownloadItem(QNetworkReply *reply, QString defaultfilename);
    DownloadItem(QWebEngineDownloadItem *item);
    ~DownloadItem();
    void SetRemoteUrl(QUrl url);
    QUrl GetRemoteUrl() const;
    QUrl GetLocalUrl() const;
    QList<QUrl> GetUrls() const;

    QString GetPath() const;
    void SetPath(QString name);
    void SetPathAndReady(QString name);

private:
    QString CreateDefaultFromReplyOrRequest();

    QNetworkReply *m_DownloadReply;
    QWebEngineDownloadItem *m_DownloadItem;

    bool       m_GettingPath;
    QString    m_Path;
    QString    m_DefaultFileName;
    QFile      m_FileOut;
    QByteArray m_BAOut;
    QUrl       m_RemoteUrl;
    bool       m_FinishedFlag;

private slots:
    void ReadyRead();
    void Finished();
    void Stop();
    void DownloadProgress(qint64 received, qint64 total);

signals:
    void Progress(QString, qint64, qint64);
    void DownloadResult(const QByteArray&);
};

// UploadItem
////////////////////////////////////////////////////////////////

class UploadItem : public QObject{
    Q_OBJECT

public:
    UploadItem(QNetworkReply *reply, qint64 size);
    UploadItem(QNetworkReply *reply, QString name);
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
    void Stop();
    void UploadProgress(qint64 sent, qint64 total);

signals:
    void Progress(QString, qint64, qint64);
};

// NetworkController
////////////////////////////////////////////////////////////////

class NetworkController : public QObject{
    Q_OBJECT

public:
    NetworkController();
    ~NetworkController();

    enum DownloadType {
        SelectedDirectory,
        TemporaryDirectory,
        ToVariable
    };

    static DownloadItem* Download(NetworkAccessManager *nam, QUrl &url,
                                  QUrl &referer = QUrl(),
                                  DownloadType type = SelectedDirectory);
    static DownloadItem* Download(NetworkAccessManager *nam,
                                  const QNetworkRequest &request,
                                  DownloadType type = SelectedDirectory);
    static DownloadItem* Download(QNetworkReply *reply,
                                  QString filename = QString(),
                                  DownloadType type = SelectedDirectory);
    static UploadItem* Upload(QNetworkReply *reply, qint64 filesize);
    static UploadItem* Upload(QNetworkReply *reply, QString filename);
    static void RemoveItem(DownloadItem *item);
    static void RemoveItem(UploadItem *item);

    static void SetUserAgent(NetworkAccessManager *nam, QStringList set);
    static void SetProxy(NetworkAccessManager *nam, QStringList set);
    static void SetSslProtocol(NetworkAccessManager *nam, QStringList set);
    static void SetOffTheRecord(NetworkAccessManager *nam, QStringList set);

    static NetworkAccessManager* GetNetworkAccessManager(QString id, QStringList set = QStringList());
    static NetworkAccessManager* CopyNetworkAccessManager(QString bef, QString aft, QStringList set);
    static NetworkAccessManager* MoveNetworkAccessManager(QString bef, QString aft, QStringList set);
    static NetworkAccessManager* MergeNetworkAccessManager(QString bef, QString aft, QStringList set);
    static NetworkAccessManager* KillNetworkAccessManager(QString id);
    static QMap<QString, NetworkAccessManager*> AllNetworkAccessManager();
    static void InitializeNetworkAccessManager(QString id, const QList<QNetworkCookie> &cookies = QList<QNetworkCookie>());
    static void LoadAllCookies();
    static void SaveAllCookies();

private:
    static QMap<QString, NetworkAccessManager*> m_NetworkAccessManagerTable;
    static QList<DownloadItem*> m_DownloadList;
    static QList<UploadItem*>   m_UploadList;
};

#endif //ifndef NETCONTROL_HPP
