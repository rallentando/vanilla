#include <QtTest>

#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "networkcontroller.hpp"
#include "page.hpp"
#include "application.hpp"
#ifdef WEBENGINEVIEW
#  include <QWebEngineProfile>
#  include <QQuickWebEngineProfile>
#endif

#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick>
#  include <QWebEngineProfile>
#  include <QQuickItem>
#endif

class tst_profilepair : public QObject {
    Q_OBJECT

private slots:
    void onlyTheWordInForceMakesAViewPrivate();
#ifdef WEBENGINEVIEW
    void aPrivateViewLeavesTheDiskProfileWhereItWas();
    void privateViewsOfOneSpaceShareOneProfile();
    void theManagerMakesThePrivateProfileOnlyWhenAsked();
    void theUserAgentReachesBothProfiles();
    void aQuickViewStartsOnItsOwnWord();
    void aLiveQuickViewFollowsItsWordsBothWays();
    void aLiveWidgetsViewAsksToBeMadeAgainBothWays();
    void aDownloadWritesNoAddressWhileTheProfileIsStillPrivate();
    void aDownloadRequestIsPrivateByTheProfileWhichMadeIt();
    void aPrivateViewLeavesNoIconAndNoAnswerToABadCertificate();
    void aFetchByAddressSaysWhetherAPrivateViewAsked();
#endif
};

void tst_profilepair::onlyTheWordInForceMakesAViewPrivate(){
    using DirectoryPage::SaysPrivate;
    QVERIFY(!SaysPrivate(QStringList()));
    QVERIFY(!SaysPrivate(QStringList() << QStringLiteral("WebEngineView")));
    foreach(const QString &word, QStringList() << QStringLiteral("Private") << QStringLiteral("private")
                                               << QStringLiteral("OffTheRecord") << QStringLiteral("offTheRecord"))
        QVERIFY2(SaysPrivate(QStringList() << word), qPrintable(word));
    QVERIFY(!SaysPrivate(QStringList() << QStringLiteral("!Private")));
    QVERIFY(!SaysPrivate(QStringList() << QStringLiteral("Private") << QStringLiteral("!Private")));
    QVERIFY(!SaysPrivate(QStringList() << QStringLiteral("PrivateNotes")));
}

#ifdef WEBENGINEVIEW
namespace {
    QStringList Words(const char *word = nullptr){
        return word ? QStringList() << QString::fromLatin1(word) : QStringList();
    }

    class WidgetsView : public WebEngineView {
    public:
        WidgetsView(const QString &id, const QStringList &set)
            : WebEngineView(nullptr, id, set) {}
        QWebEngineProfile *Profile(){ return page()->profile(); }
        int m_Rebuilds = 0;
    private:
        void RebuildForOffTheRecord() Q_DECL_OVERRIDE { ++m_Rebuilds; }
    };

    class QuickView : public QuickWebEngineView {
    public:
        QuickView(const QString &id, const QStringList &set)
            : QuickWebEngineView(nullptr, id, set) {}
        QObject *Profile(){ return rootObject()->property("profile").value<QObject*>(); }
    };
}

void tst_profilepair::aPrivateViewLeavesTheDiskProfileWhereItWas(){
    const QString id = QStringLiteral("pair-disk");
    WidgetsView before(id, Words());
    WidgetsView hidden(id, Words("Private"));
    WidgetsView after(id, Words());
    WidgetsView negated(id, Words("!Private"));

    QVERIFY(!before.Profile()->isOffTheRecord());
    QVERIFY(hidden.Profile()->isOffTheRecord());
    QCOMPARE(after.Profile(), before.Profile());
    QCOMPARE(negated.Profile(), before.Profile());
    NetworkAccessManager *nam = NetworkController::FindNetworkAccessManager(id);
    QVERIFY(nam);
    QCOMPARE(nam->GetProfile(false), before.Profile());
    QCOMPARE(nam->GetProfile(true), hidden.Profile());
    QCOMPARE(NetworkController::ProfileKey(before.Profile()), id);
    QVERIFY(NetworkController::ProfileKey(hidden.Profile()).isEmpty());
}

void tst_profilepair::privateViewsOfOneSpaceShareOneProfile(){
    const QString id = QStringLiteral("pair-private");
    WidgetsView one(id, Words("Private"));
    WidgetsView between(id, Words("!Private"));
    WidgetsView two(id, Words("OffTheRecord"));
    QVERIFY(one.Profile()->isOffTheRecord());
    QCOMPARE(two.Profile(), one.Profile());
    QVERIFY(!between.Profile()->isOffTheRecord());

    WidgetsView elsewhere(QStringLiteral("pair-private-other"), Words("Private"));
    QVERIFY(elsewhere.Profile()->isOffTheRecord());
    QVERIFY(elsewhere.Profile() != one.Profile());
}

void tst_profilepair::theManagerMakesThePrivateProfileOnlyWhenAsked(){
    NetworkAccessManager *nam =
        NetworkController::GetNetworkAccessManager(QStringLiteral("pair-walk"));
    QCOMPARE(nam->Profiles().size(), 1);
    QCOMPARE(nam->Profiles().first(), nam->GetProfile(false));
    QWebEngineProfile *hidden = nam->GetProfile(true);
    QVERIFY(hidden->isOffTheRecord());
    QCOMPARE(nam->Profiles(), QList<QWebEngineProfile*>() << nam->GetProfile(false) << hidden);
    QCOMPARE(nam->GetProfile(true), hidden);
    QCOMPARE(nam->GetSharedProfile(true).get(), hidden);
}

void tst_profilepair::theUserAgentReachesBothProfiles(){
    NetworkAccessManager *nam =
        NetworkController::GetNetworkAccessManager(QStringLiteral("pair-agent"));
    nam->SetUserAgent(QStringLiteral("Vanilla/1.0"));
    QCOMPARE(nam->GetProfile(false)->httpUserAgent(), nam->GetUserAgent());
    QCOMPARE(nam->GetProfile(true)->httpUserAgent(), nam->GetUserAgent());
    nam->SetUserAgent(QStringLiteral("Vanilla/2.0"));
    QCOMPARE(nam->GetUserAgent(), QStringLiteral("Vanilla/2.0"));
    QCOMPARE(nam->GetProfile(false)->httpUserAgent(), nam->GetUserAgent());
    QCOMPARE(nam->GetProfile(true)->httpUserAgent(), nam->GetUserAgent());
}

void tst_profilepair::aQuickViewStartsOnItsOwnWord(){
    const QString id = QStringLiteral("pair-quick");
    WidgetsView hidden(id, Words("Private"));
    QuickView plain(id, Words());
    QuickView secret(id, Words("Private"));
    QCOMPARE(plain.Profile(), static_cast<QObject*>(NetworkController::QuickProfile(id)));
    QCOMPARE(secret.Profile(), static_cast<QObject*>(NetworkController::QuickPrivateProfile(id)));
}

void tst_profilepair::aLiveQuickViewFollowsItsWordsBothWays(){
    const QString id = QStringLiteral("pair-quick-live");
    QuickView view(id, Words("Private"));
    QCOMPARE(view.Profile(), static_cast<QObject*>(NetworkController::QuickPrivateProfile(id)));
    view.ApplySpecificSettings(Words());
    QCOMPARE(view.Profile(), static_cast<QObject*>(NetworkController::QuickProfile(id)));
    view.ApplySpecificSettings(Words("Private"));
    QCOMPARE(view.Profile(), static_cast<QObject*>(NetworkController::QuickPrivateProfile(id)));
}

void tst_profilepair::aLiveWidgetsViewAsksToBeMadeAgainBothWays(){
    const QString id = QStringLiteral("pair-widgets-live");
    WidgetsView view(id, Words("Private"));
    view.ApplySpecificSettings(Words("Private"));
    QCOMPARE(view.m_Rebuilds, 0);
    view.ApplySpecificSettings(Words());
    QCOMPARE(view.m_Rebuilds, 1);

    WidgetsView plain(id, Words());
    plain.ApplySpecificSettings(Words("!Private"));
    QCOMPARE(plain.m_Rebuilds, 0);
    plain.ApplySpecificSettings(Words("Private"));
    QCOMPARE(plain.m_Rebuilds, 1);
}

void tst_profilepair::aDownloadWritesNoAddressWhileTheProfileIsStillPrivate(){
    const QString id = QStringLiteral("pair-download");
    WidgetsView view(id, Words("Private"));
    view.ApplySpecificSettings(Words());
    QVERIFY(view.Profile()->isOffTheRecord());
    QVERIFY(!DirectoryPage::SaysPrivate(view.SpecificSettings()));
    QVERIFY(view.IsOffTheRecord());
    QVERIFY(Page::OffTheRecord(&view));

    WidgetsView plain(id, Words());
    plain.ApplySpecificSettings(Words("Private"));
    QVERIFY(!plain.IsOffTheRecord());
    QVERIFY(Page::OffTheRecord(&plain));

    WidgetsView open(id, Words());
    QVERIFY(!Page::OffTheRecord(&open));
}

void tst_profilepair::aDownloadRequestIsPrivateByTheProfileWhichMadeIt(){
    QWebEngineProfile widgetsPrivate;
    QWebEngineProfile widgetsDisk(QStringLiteral("pair-made"));
    QVERIFY(widgetsPrivate.isOffTheRecord());
    QVERIFY(!widgetsDisk.isOffTheRecord());
    QObject fromPrivate(&widgetsPrivate), fromDisk(&widgetsDisk);
    QVERIFY(NetworkController::MadeOffTheRecord(&fromPrivate));
    QVERIFY(!NetworkController::MadeOffTheRecord(&fromDisk));

    QQuickWebEngineProfile quickPrivate;
    quickPrivate.setOffTheRecord(true);
    QQuickWebEngineProfile quickDisk;
    quickDisk.setStorageName(QStringLiteral("pair-made-quick"));
    quickDisk.setOffTheRecord(false);
    QObject fromQuickPrivate(&quickPrivate), fromQuickDisk(&quickDisk);
    QVERIFY(NetworkController::MadeOffTheRecord(&fromQuickPrivate));
    QVERIFY(!NetworkController::MadeOffTheRecord(&fromQuickDisk));

    QObject orphan;
    QVERIFY(!NetworkController::MadeOffTheRecord(&orphan));
    QVERIFY(!NetworkController::MadeOffTheRecord(nullptr));
}

void tst_profilepair::aPrivateViewLeavesNoIconAndNoAnswerToABadCertificate(){
    const QString id = QStringLiteral("pair-traces");
    WidgetsView secret(id, Words("Private"));
    WidgetsView open(id, Words());
    const bool hidden = Page::OffTheRecord(&secret);
    const bool shown = Page::OffTheRecord(&open);
    QVERIFY(hidden);
    QVERIFY(!shown);

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::red);
    const QIcon icon(pixmap);
    Application::RegisterIcon(QStringLiteral("secret.example"), icon, hidden);
    QVERIFY(Application::GetIcon(QStringLiteral("secret.example")).isNull());
    QVERIFY(!Application::IconDatabaseSnapshot().contains(QStringLiteral("secret.example")));
    Application::RegisterIcon(QStringLiteral("open.example"), icon, shown);
    QVERIFY(!Application::GetIcon(QStringLiteral("open.example")).isNull());

    Application::AppendToAllowedHosts(QStringLiteral("secret.example"), hidden);
    Application::AppendToBlockedHosts(QStringLiteral("secret.example"), hidden);
    QVERIFY(!Application::GetAllowedHosts().contains(QStringLiteral("secret.example")));
    QVERIFY(!Application::GetBlockedHosts().contains(QStringLiteral("secret.example")));
    Application::AppendToAllowedHosts(QStringLiteral("open.example"), shown);
    Application::AppendToBlockedHosts(QStringLiteral("blocked.example"), shown);
    QVERIFY(Application::GetAllowedHosts().contains(QStringLiteral("open.example")));
    QVERIFY(Application::GetBlockedHosts().contains(QStringLiteral("blocked.example")));

    const QString secretKey = QStringLiteral("secret.example:1111");
    const QString allowedKey = QStringLiteral("open.example:2222");
    const QString blockedKey = QStringLiteral("open.example:3333");
    Application::RememberCertificate(secretKey, true, hidden);
    Application::RememberCertificate(secretKey, false, hidden);
    QVERIFY(!Application::GetAllowedCertificates().contains(secretKey));
    QVERIFY(!Application::GetBlockedCertificates().contains(secretKey));
    Application::RememberCertificate(allowedKey, true, shown);
    Application::RememberCertificate(blockedKey, false, shown);
    QVERIFY(Application::GetAllowedCertificates().contains(allowedKey));
    QVERIFY(Application::GetBlockedCertificates().contains(blockedKey));
}

void tst_profilepair::aFetchByAddressSaysWhetherAPrivateViewAsked(){
    NetworkAccessManager *nam =
        NetworkController::GetNetworkAccessManager(QStringLiteral("pair-fetch"));
    foreach(const bool offTheRecord, QList<bool>() << true << false){
        const QList<QNetworkReply*> before = nam->findChildren<QNetworkReply*>();
        DownloadItem *item = NetworkController::Download
            (nam, QUrl(QStringLiteral("data:text/plain,x")), QUrl(),
             NetworkController::ToVariable, offTheRecord);
        QList<QNetworkReply*> made;
        foreach(QNetworkReply *reply, nam->findChildren<QNetworkReply*>())
            if(!before.contains(reply)) made << reply;
        QCOMPARE(made.size(), 1);
        QCOMPARE(NetworkAccessManager::IsOffTheRecord(made.first()->request()), offTheRecord);
        delete item;
    }
}
#endif

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    SettingsSchemeHandler::RegisterScheme();
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_profilepair test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_APPLESS_MAIN(tst_profilepair)
#endif

#include "tst_profilepair.moc"
