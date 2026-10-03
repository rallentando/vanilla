#ifndef EXTENSIONHOST_HPP
#define EXTENSIONHOST_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QTimer>
#include <functional>
#include <memory>

#include "extensionhostwire.hpp"
#include "extensionmainscripts.hpp"
#include "extensionuserscripts.hpp"
#include "extensionui.hpp"

class ExtensionController;
class ModelessDialog;
class QWebEngineDownloadRequest;
class QQuickWebEngineProfile;
class QWebEnginePage;
class QWebEngineProfile;
class TreeBank;
class View;
class ViewNode;

class ExtensionHost : public QObject {
    Q_OBJECT

public:

    class Ask {
    public:
        virtual ~Ask() {}
        virtual QByteArray Method() const = 0;
        virtual QUrl Url() const = 0;
        virtual QUrl Initiator() const = 0;
        virtual QMap<QByteArray, QByteArray> Headers() const = 0;
        virtual bool Body(qint64 limit, QByteArray *body) = 0;
        virtual bool Alive() const = 0;
        virtual void Reply(const QJsonObject &answer) = 0;
        virtual void ReplyNothing() = 0;
        virtual void Fail() = 0;
    };
    typedef std::shared_ptr<Ask> HeldAsk;

#ifdef WEBENGINEVIEW
    static void RegisterScheme();
    static void Install(QWebEngineProfile *profile, ExtensionController *controller);
    static void Install(QQuickWebEngineProfile *profile, ExtensionController *controller);

    static void StampRequestsOf(QWebEnginePage *page, View *view);
#endif

    static ExtensionHost *Install(QObject *owner, ExtensionController *controller, const QString &profileSpace);
    static ExtensionHost *Of(const ExtensionController *controller);

    static QByteArray KeyFor(const QString &id);
    static bool ShimsOn();
    static QString CopyRoot();

    static quint64 NumberView(QObject *owner, View *view);
    static void ForgetView(quint64 number);

    static void MenuPicked(const ExtensionController *controller, qint64 tab, const QString &pageUrl);

    static void Invoked(const ExtensionController *controller, const QString &id, qint64 tab);
    static void SidePanelEvent(const ExtensionController *controller, const QString &id, bool opened, const QString &path, qint64 tab = 0);

    void Handle(const HeldAsk &ask, quint64 view);

    ~ExtensionHost();

    struct Pending;

    static void CloseOffscreenOf(QObject *profile);
    static bool HasOffscreenOf(QObject *profile, const QUrl &origin);

private:
    ExtensionHost(QObject *owner, ExtensionController *controller, const QString &profileSpace);
    QList<ExtensionHostWire::Tab> Tabs(bool dated = false) const;
    ExtensionHostWire::Bookmark Tree() const;
    ViewNode *NodeOf(qint64 id) const;
    ViewNode *CurrentNode() const;
    QJsonObject Perform(const Pending &pending);
    QJsonObject Create(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight);
    QJsonObject Update(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight);
    QJsonObject Remove(const ExtensionHostWire::Act &act, bool whatever);
    QJsonObject Reload(const ExtensionHostWire::Act &act);
    QJsonObject Search(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight);
    QJsonObject Restore(const ExtensionHostWire::Sight &sight);
    QJsonObject Duplicate(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight);
    QJsonObject Move(const ExtensionHostWire::Act &act, const ExtensionHostWire::Sight &sight);
    static void DrainSoon(int milliseconds);
    static void Drain();
    void Subscribe(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id);
    void Fire(const QString &id, const QString &name, const QJsonArray &args, qint64 tab);
    QJsonObject CaptureCall(const ExtensionHostWire::Call &asked, const QString &id);
    ExtensionHostWire::ActiveTabs m_ActiveTabs;
    QHash<QString, qint64> m_LastCapture;
    QHash<QString, qint64> m_InvokedAt;
    QJsonObject SidePanelCall(const ExtensionHostWire::Call &asked, const QString &id);
    QJsonObject NotificationCall(const ExtensionHostWire::Call &asked, const QString &id);
    void ShowNotice(const QString &id, const QString &notice, quint64 serial);
    void NoticeEnded(const QString &id, const QString &notice, quint64 serial, bool clicked, bool byUser);
    void DismissNotice(quint64 serial);
    void CloseNoticesOfOthers();
    bool NoticesMayRun(const QString &id) const;
    bool Runs(const QString &id) const;
    void CloseNotices();
    ExtensionUi::Notices m_Notices;
    QHash<quint64, QPointer<ModelessDialog> > m_NoticeDialogs;
    bool m_Qt = false;
    QJsonObject ActionCall(const ExtensionHostWire::Call &asked, const QString &id);
    void OffscreenCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id);
    void OffscreenLoaded(const QString &id, QObject *page, bool loaded);
    void CloseOffscreenOfOthers();
    void CloseOffscreen(bool now);
    void IdentityCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id);
    struct Flow {
        quint64 serial = 0;
        HeldAsk ask;
        bool interactive = false;
        bool abortOnLoad = true;
        qint64 deadline = 0;
        QElapsedTimer started;
        QPointer<QObject> page;
        QPointer<QObject> window;
        QPointer<QObject> watch;
    };
    QHash<QString, Flow> m_Flows;
    quint64 m_FlowSerial = 0;
    void AuthEnd(const QString &id, quint64 serial, const QJsonObject &answer);
    void AuthGone(Flow &flow, bool now);
    void AuthLoaded(const QString &id, quint64 serial, bool loaded);
    void AuthWatch(const QString &id, quint64 serial);
    void CloseFlowsOfOthers();
    void CloseFlows(bool now);
    QJsonObject DownloadCall(const ExtensionHostWire::Call &asked, const QString &id);
    void DownloadRequested(QWebEngineDownloadRequest *request);
    QWebEnginePage *Downloader();
    qint64 Publish();
    static void Changed();
    static void LookSoon(int milliseconds);
    static void Look();
    static QByteArray Secret(bool make);
    View *ViewOf(quint64 number) const;
    void Bind(const HeldAsk &ask, quint64 view);

    QObject *m_Owner;
    QString m_ProfileSpace;
    QPointer<ExtensionController> m_Controller;
    ExtensionHostWire::BindTable m_Documents;
    ExtensionHostWire::MenuPicks m_MenuPicks;
    ExtensionHostWire::Subscribers<HeldAsk> m_Subscribers;
    struct Offscreen {
        QPointer<QObject> page;
        QUrl url;
        QString documentId;
        QString contextId;
        bool loaded = false;
        HeldAsk making;
    };
    QHash<QString, Offscreen> m_Offscreen;
    QHash<QString, Offscreen>::iterator OffscreenGone(QHash<QString, Offscreen>::iterator gone, const QString &why);
    ExtensionUi::Downloads m_Downloads;
    QPointer<QObject> m_Downloader;
    QHash<qint64, QPointer<QObject> > m_Requests;
    bool m_Closed = false;

    ExtensionMainScripts::Table m_MainScripts;
    std::function<void(const ExtensionMainScripts::Table::Change &)> m_ApplyMainScripts;
    void MainScriptsCall(const HeldAsk &ask, const QJsonArray &args, const QString &id);
    void KeepMainScripts();
    bool MainScriptsWanted(const QString &id, const QString &folder) const { return ScriptsWanted(id, folder, QStringLiteral("scripting")); }
    bool UserScriptsWanted(const QString &id, const QString &folder) const { return ScriptsWanted(id, folder, QStringLiteral("userScripts")); }
    bool ScriptsWanted(const QString &id, const QString &folder, const QString &permission) const;
    ExtensionMainScripts::Store m_MainStore;
    QString m_MainStoreFile;
    QByteArray m_MainStoreWritten;
    void ReadMainStore(const QString &path);
    void WriteMainStore();

    ExtensionMainScripts::Store m_UserStore;
    QString m_UserStoreFile;
    QByteArray m_UserStoreWritten;
    bool m_UserStoreSoon = false;
    ExtensionMainScripts::Table m_UserScripts;
    QHash<QString, QString> m_UserChecked;
    ExtensionUserScripts::Worlds m_UserWorlds;
    QHash<QByteArray, QPair<QString, QString> > m_WorldSecrets;
    ExtensionHostWire::UserScriptListeners m_UserScriptListeners;
    QSet<QString> m_UserScriptNamedEver;
    void DeliverWaitingUserScriptMessages(const QString &id, const QByteArray &token, bool named, bool subscribed);
    ExtensionHostWire::UserScriptMessages<HeldAsk> m_UserScriptMessages;
    QTimer *m_UserScriptSweep = nullptr;
    ExtensionHostWire::Wakes m_Wakes;
    QHash<QString, QPointer<QObject> > m_Wakers;
    QTimer *m_WakeSweep = nullptr;
    void Wake(const QString &id, bool wanted = false);
    bool CanWake() const;
    void CloseWaker(const QString &id);
    void SweepWakes();
    void UserScriptMessage(const HeldAsk &ask, quint64 view);
    bool UserScriptMessageCall(const HeldAsk &ask, const ExtensionHostWire::Call &asked, const QString &id);
    void SweepUserScriptMessages();
    void AnswerUserScriptMessages(const QList<ExtensionHostWire::UserScriptMessages<HeldAsk>::Done> &done);
    ExtensionUserScripts::Reader CopyReaderOf(const QString &id) const;
    void UserScriptsCall(const HeldAsk &ask, const QString &api, const QJsonArray &args, const QString &id);
    void CheckUserBook(const QString &id);
    void PutUserScripts(const QString &id);
    void KeepUserScripts();
    void ReadUserStore(const QString &path);
    void WriteUserStoreSoon();
};

#endif
