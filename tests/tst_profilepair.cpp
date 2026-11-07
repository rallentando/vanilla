#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QScopeGuard>

#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "networkcontroller.hpp"
#include "treebank.hpp"
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
#  include <QWebEngineScript>
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
    void theKeyFilterIsTheViewsAboveAndTheProfilesBelow();
    void aLiveWidgetsViewAsksToBeMadeAgainBothWays();
    void aDownloadWritesNoAddressWhileTheProfileIsStillPrivate();
    void aDownloadRequestIsPrivateByTheProfileWhichMadeIt();
    void aPrivateViewLeavesNoIconAndNoAnswerToABadCertificate();
    void aFetchByAddressSaysWhetherAPrivateViewAsked();
    void clearingReachesTheManagersNoSpaceNamesAnyMore();
    void aRenamedSpaceIsLetGoOfWholeAndMadeAgain();
    void aReleasedStorageIsInUseUntilItsProfileHasGone();
    void aTabMovedInIsMadeAgainWhenTheIdGoes();
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
        void RebuildForOffTheRecord() override { ++m_Rebuilds; }
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

void tst_profilepair::theKeyFilterIsTheViewsAboveAndTheProfilesBelow(){
    const QString id = QStringLiteral("pair-quick-keys");
    auto filters = [](QObject *owner){
        QObject *scripts = owner->property("userScripts").value<QObject*>();
        QList<QWebEngineScript> found;
        if(!scripts) return found;
        const QList<QWebEngineScript> all =
            scripts->property("collection").value<QList<QWebEngineScript>>();
        for(const QWebEngineScript &script : all)
            if(script.sourceCode().contains(QuickWebEngineView::DefaultScript())) found << script;
        return found;
    };
    QuickView plain(id, Words());
    QuickView secret(id, Words("Private"));
    foreach(QObject *profile, QList<QObject*>()
            << NetworkController::QuickProfile(id) << NetworkController::QuickPrivateProfile(id)){
        const QList<QWebEngineScript> found = filters(profile);
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().sourceCode(), QuickWebEngineView::DefaultFrameScript());
        QVERIFY(found.first().runsOnSubFrames());
        QCOMPARE(found.first().worldId(), quint32(QWebEngineScript::ApplicationWorld));
    }
    QVERIFY(QuickWebEngineView::DefaultFrameScript().contains(
                QStringLiteral("if(window.top === window) return;")));
    foreach(QuickView *view, QList<QuickView*>() << &plain << &secret){
        const QList<QWebEngineScript> found = filters(view->rootObject());
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().sourceCode(), QuickWebEngineView::DefaultScript());
        QVERIFY(!found.first().runsOnSubFrames());
        QCOMPARE(found.first().worldId(), quint32(QWebEngineScript::ApplicationWorld));
    }
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

void tst_profilepair::clearingReachesTheManagersNoSpaceNamesAnyMore(){
    const QNetworkCookie cookie(QByteArrayLiteral("kept"), QByteArrayLiteral("1"));
    const auto made = [&](const QString &id){
        NetworkAccessManager *nam = NetworkController::GetNetworkAccessManager(id);
        nam->GetNetworkCookieJar()->SetAllCookies(QList<QNetworkCookie>() << cookie);
        return nam;
    };
    NetworkAccessManager *killed  = made(QStringLiteral("pair-killed"));
    NetworkAccessManager *merged  = made(QStringLiteral("pair-merged"));
    NetworkAccessManager *into    = made(QStringLiteral("pair-into"));
    NetworkAccessManager *covered = made(QStringLiteral("pair-covered"));
    NetworkAccessManager *moved   = made(QStringLiteral("pair-moved"));

    NetworkController::KillNetworkAccessManager(QStringLiteral("pair-killed"));
    NetworkController::MoveNetworkAccessManager(QStringLiteral("pair-merged"), QStringLiteral("pair-into"), QStringList());
    NetworkController::CopyNetworkAccessManager(QStringLiteral("pair-into"), QStringLiteral("pair-covered"), QStringList());
    NetworkController::MoveNetworkAccessManager(QStringLiteral("pair-moved"), QStringLiteral("pair-moved-to"), QStringList());

    const QMap<QString, NetworkAccessManager*> table = NetworkController::AllNetworkAccessManager();
    const QList<NetworkAccessManager*> live = NetworkController::LiveNetworkAccessManagers();
    foreach(NetworkAccessManager *gone, QList<NetworkAccessManager*>() << killed << merged << covered){
        QVERIFY(!table.values().contains(gone));
        QVERIFY(live.contains(gone));
    }
    QCOMPARE(table.value(QStringLiteral("pair-moved-to")), moved);
    QCOMPARE(live.count(moved), 1);
    QCOMPARE(live.count(into), 1);

    NetworkController::ClearCookies();
    foreach(NetworkAccessManager *nam, QList<NetworkAccessManager*>() << killed << merged << into << covered << moved)
        QVERIFY2(nam->GetNetworkCookieJar()->GetAllCookies().isEmpty(), qPrintable(nam->objectName()));
}
#endif

#ifdef WEBENGINEVIEW
void tst_profilepair::aRenamedSpaceIsLetGoOfWholeAndMadeAgain(){
    const QString from = QStringLiteral("pair-rename"), to = QStringLiteral("pair-renamed");
    QNetworkCookie cookie(QByteArrayLiteral("held"), QByteArrayLiteral("1"));
    cookie.setExpirationDate(QDateTime::currentDateTime().addDays(1));
    NetworkAccessManager *first = NetworkController::GetNetworkAccessManager(from);
    NetworkController::KillNetworkAccessManager(from);
    NetworkController::GetNetworkAccessManager(QStringLiteral("pair-rename-source"));
    NetworkAccessManager *second = NetworkController::CopyNetworkAccessManager
        (QStringLiteral("pair-rename-source"), from, QStringList());
    second->GetNetworkCookieJar()->SetAllCookies(QList<QNetworkCookie>() << cookie);
    QQuickWebEngineProfile *quick = NetworkController::QuickProfile(from);

    const QList<QObject*> profiles = NetworkController::SpaceProfiles(from);
    QVERIFY(profiles.contains(first->GetProfile(false)));
    QVERIFY(profiles.contains(second->GetProfile(false)));
    QVERIFY(profiles.contains(quick));
    QVERIFY(!profiles.contains(NetworkController::FindNetworkAccessManager
                               (QStringLiteral("pair-rename-source"))->GetProfile(false)));

    QPointer<NetworkAccessManager> a(first), b(second);
    QPointer<QQuickWebEngineProfile> q(quick);
    NetworkController::ReleaseSpace(from);
    QVERIFY(!NetworkController::FindNetworkAccessManager(from));
    QVERIFY(!NetworkController::LiveNetworkAccessManagers().contains(first));
    QVERIFY(!NetworkController::LiveNetworkAccessManagers().contains(second));
    QTRY_VERIFY(!a && !b && !q);

    const auto saved = [](){ return QJsonDocument::fromJson(NetworkController::CookieSnapshot()).object();};
    QCOMPARE(saved().value(from).toArray().size(), 1);

    NetworkAccessManager *made = NetworkController::PlaceSpace(from, to, QStringList());
    QCOMPARE(NetworkController::FindNetworkAccessManager(to), made);
    QCOMPARE(made->GetNetworkCookieJar()->GetAllCookies().size(), 1);
    QVERIFY(!saved().contains(from));

    NetworkController::ReleaseSpace(to);
    QCOMPARE(saved().value(to).toArray().size(), 1);
    NetworkController::ClearCookies();
    QCOMPARE(saved().value(to).toArray().size(), 0);
    NetworkController::PlaceSpace(to, from, QStringList());
    QVERIFY(NetworkController::FindNetworkAccessManager(from)->GetNetworkCookieJar()->GetAllCookies().isEmpty());
}
#endif

#ifdef WEBENGINEVIEW
void tst_profilepair::aReleasedStorageIsInUseUntilItsProfileHasGone(){
    const QString id = QStringLiteral("pair-releasing");
    QPointer<QWebEngineProfile> profile(NetworkController::GetNetworkAccessManager(id)->GetProfile(false));
    QVERIFY(!NetworkController::StorageInUse(id));
    NetworkController::ReleaseSpace(id);
    NetworkController::DropHeldCookies(id);
    QVERIFY(profile);
    QVERIFY(NetworkController::StorageInUse(id));
    QTRY_VERIFY(!profile);
    QVERIFY(!NetworkController::StorageInUse(id));
}
#endif

#ifdef WEBENGINEVIEW
void tst_profilepair::aTabMovedInIsMadeAgainWhenTheIdGoes(){
    TreeBank::Initialize();
    ViewNode *directory = TreeBank::GetViewRoot()->MakeChild();
    const QString with = QStringLiteral("pair-moved-in;ID");
    directory->SetTitle(with);
    ViewNode *tab = directory->MakeChild();
    WidgetsView view(QStringLiteral("pair-moved-from"), Words());
    tab->SetView(&view);
    auto unlink = qScopeGuard([tab](){ tab->SetView(nullptr);});

    directory->SetTitle(QStringLiteral("pair-moved-in"));
    QVERIFY(TreeBank::ReconfigureDirectory(directory, with, QStringLiteral("pair-moved-in")));
    QTRY_VERIFY(!TreeBank::IsMovingSpace());
    QCOMPARE(view.m_Rebuilds, 1);
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
