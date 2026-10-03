#include <QtTest>

#include "webengineview.hpp"
#include "quickwebengineview.hpp"
#include "settingspage.hpp"
#include "directorypage.hpp"
#include "networkcontroller.hpp"

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
