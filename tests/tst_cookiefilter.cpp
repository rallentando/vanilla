#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QNetworkCookie>
#include <QDateTime>

#include "networkcontroller.hpp"

#include "testsupport.hpp"

namespace {

const QString WEB  = QStringLiteral("web:profile");
const QString EDGE = QStringLiteral("edge:profile");

const QDateTime NOW = QDateTime::fromString(QStringLiteral("20260101120000"), NODE_DATETIME_FORMAT);

QNetworkCookie Session(){
    return QNetworkCookie("SID", "value");
}

QNetworkCookie Persistent(int secondsFromNow){
    QNetworkCookie cookie("PID", "value");
    cookie.setExpirationDate(NOW.addSecs(secondsFromNow));
    return cookie;
}

bool Save(const QNetworkCookie &cookie, bool saveSessionCookie){
    return NetworkController::ShouldSaveCookie(cookie, saveSessionCookie, NOW);
}

}

class tst_cookiefilter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
    }

    void aninvalidExpiryComparesAsEarlierThanNow(){
        QVERIFY(!Session().expirationDate().isValid());
        QVERIFY(Session().expirationDate().toLocalTime() < NOW);
        QVERIFY(Session().expirationDate().toUTC() < NOW);
    }

    void asessionCookieIsKeptWhenTheSettingSaysSo(){
        QVERIFY(Session().isSessionCookie());
        QCOMPARE(Save(Session(), true),  true);
        QCOMPARE(Save(Session(), false), false);
    }

    void apersistentCookieIsKeptUntilItExpires(){
        QCOMPARE(Save(Persistent(3600), true),  true);
        QCOMPARE(Save(Persistent(-3600), true), false);
    }

    void thesettingDoesNotTouchPersistentCookies(){
        QCOMPARE(Save(Persistent(3600), false), true);
        QCOMPARE(Save(Persistent(3600), true),  true);
    }

    void acookieIsOnlyDroppedWhenBothReadingsAgree(){
        const int offset = QDateTime::currentDateTime().offsetFromUtc();
        if(offset == 0) QSKIP("this machine is on UTC, so the two readings cannot differ");

        QNetworkCookie cookie("PID", "value");
        cookie.setExpirationDate(NOW.addSecs(-offset / 2));
        const bool local = cookie.expirationDate().toLocalTime() < NOW;
        const bool utc   = cookie.expirationDate().toUTC() < NOW;
        if(local == utc) QSKIP("the two readings agree here; nothing to tell apart");

        QCOMPARE(Save(cookie, true), true);
    }

    void aCookieMirroredFromAnEngineIsAnsweredButNotSaved(){
        NetworkCookieJar jar;
        jar.SetAllCookies(QList<QNetworkCookie>() << Persistent(3600));

        QNetworkCookie mirrored("MID", "value");
        mirrored.setDomain(QStringLiteral("example.com"));
        mirrored.setPath(QStringLiteral("/"));
        mirrored.setExpirationDate(NOW.addSecs(3600));
        jar.MirrorCookie(WEB, mirrored);

        QCOMPARE(jar.GetAllCookies().count(), 2);
        const QList<QNetworkCookie> saved = jar.GetPersistableCookies();
        QCOMPARE(saved.count(), 1);
        QCOMPARE(saved.first().name(), QByteArray("PID"));
    }

    void aMirroredCookieReissuedReplacesItsOlderSelf(){
        NetworkCookieJar jar;
        QNetworkCookie first("SID", "v1");
        first.setDomain(QStringLiteral("example.com"));
        first.setPath(QStringLiteral("/"));
        QNetworkCookie second("SID", "v2");
        second.setDomain(QStringLiteral("example.com"));
        second.setPath(QStringLiteral("/"));
        second.setExpirationDate(NOW.addSecs(3600));

        jar.MirrorCookie(WEB, first);
        jar.MirrorCookie(WEB, second);
        QCOMPARE(jar.GetAllCookies().count(), 1);
        QCOMPARE(jar.GetAllCookies().first().value(), QByteArray("v2"));
        QVERIFY(jar.GetPersistableCookies().isEmpty());

        jar.UnmirrorCookie(WEB, second);
        QVERIFY(jar.GetAllCookies().isEmpty());
    }

    void anEnginesCopyTakesOverFromTheFilesOne(){
        NetworkCookieJar jar;
        QNetworkCookie fromFile("SID", "old");
        fromFile.setDomain(QStringLiteral(".example.com"));
        fromFile.setPath(QStringLiteral("/"));
        fromFile.setExpirationDate(NOW.addSecs(3600));
        jar.SetAllCookies(QList<QNetworkCookie>() << fromFile);
        QCOMPARE(jar.GetPersistableCookies().count(), 1);

        QNetworkCookie fromEngine("SID", "new");
        fromEngine.setDomain(QStringLiteral("example.com"));
        fromEngine.setPath(QStringLiteral("/"));
        fromEngine.setExpirationDate(NOW.addSecs(3600));
        jar.MirrorCookie(WEB, fromEngine);

        QCOMPARE(jar.GetAllCookies().count(), 1);
        QCOMPARE(jar.GetAllCookies().first().value(), QByteArray("new"));
        QVERIFY(jar.GetPersistableCookies().isEmpty());
    }

    void aCookieIsTheSameCookieByNameDomainAndPath(){
        QNetworkCookie a("SID", "x");
        a.setDomain(QStringLiteral(".Example.COM"));
        a.setPath(QStringLiteral("/"));
        QNetworkCookie b("SID", "y");
        b.setDomain(QStringLiteral("example.com"));
        b.setPath(QString());
        QCOMPARE(NetworkCookieJar::Identity(a), NetworkCookieJar::Identity(b));

        QNetworkCookie deeper("SID", "x");
        deeper.setDomain(QStringLiteral("example.com"));
        deeper.setPath(QStringLiteral("/dir"));
        QNetworkCookie other("OTHER", "x");
        other.setDomain(QStringLiteral("example.com"));
        other.setPath(QStringLiteral("/"));
        QNetworkCookie elsewhere("SID", "x");
        elsewhere.setDomain(QStringLiteral("other.com"));
        elsewhere.setPath(QStringLiteral("/"));

        QVERIFY(NetworkCookieJar::Identity(a) != NetworkCookieJar::Identity(deeper));
        QVERIFY(NetworkCookieJar::Identity(a) != NetworkCookieJar::Identity(other));
        QVERIFY(NetworkCookieJar::Identity(a) != NetworkCookieJar::Identity(elsewhere));
    }

    void replacingTheListKeepsTheMarksOfWhatSurvived(){
        NetworkCookieJar jar;
        QNetworkCookie kept("KEPT", "x");
        kept.setDomain(QStringLiteral("example.com"));
        kept.setPath(QStringLiteral("/"));
        QNetworkCookie dropped("DROPPED", "x");
        dropped.setDomain(QStringLiteral("example.com"));
        dropped.setPath(QStringLiteral("/"));
        jar.MirrorCookie(WEB, kept);
        jar.MirrorCookie(WEB, dropped);
        QVERIFY(jar.GetPersistableCookies().isEmpty());

        QList<QNetworkCookie> gathered;
        gathered << kept << Persistent(3600);
        jar.SetAllCookies(gathered);

        const QList<QNetworkCookie> saved = jar.GetPersistableCookies();
        QCOMPARE(saved.count(), 1);
        QCOMPARE(saved.first().name(), QByteArray("PID"));

        QNetworkCookie again("DROPPED", "y");
        again.setDomain(QStringLiteral("example.com"));
        again.setPath(QStringLiteral("/"));
        again.setExpirationDate(NOW.addSecs(3600));
        jar.SetAllCookies(jar.GetAllCookies() << again);
        QCOMPARE(jar.GetPersistableCookies().count(), 2);
    }

    void themirrorIsWiredToTheEngineAndKeptOutOfTheFile(){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                   QStringLiteral("/app/networkcontroller.cpp"));
        QVERIFY2(file.open(QIODevice::ReadOnly),
                 "'networkcontroller.cpp' was not read; check VANILLA_SOURCE_DIR");
        const QString source =
            QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

        QCOMPARE(source.count(QStringLiteral("MirrorProfileCookies(")), 4);
        QVERIFY2(source.contains(QStringLiteral("MirrorProfileCookies(m_Profile->cookieStore()")),
                 "the widget profile no longer feeds the jar");
        QVERIFY2(source.contains(QStringLiteral("QStringLiteral(\"web:\") + m_Profile->storageName()")),
                 "the widget mirror no longer names its store");
        QVERIFY2(source.contains(QStringLiteral("QStringLiteral(\"quick:\") + profile->storageName()")),
                 "the quick mirror no longer names its store");
        QVERIFY2(source.contains(QStringLiteral("MirrorProfileCookies(profile->cookieStore()")),
                 "the quick profile no longer feeds the jar");

        QCOMPARE(source.count(QStringLiteral("MirrorProfileCookies(m_Profile->")), 1);
        QVERIFY2(!source.contains(QStringLiteral("MirrorProfileCookies(m_PrivateProfile")),
                 "an off the record profile would now feed the shared jar");

        QVERIFY2(source.contains(QStringLiteral("GetNetworkCookieJar()->GetPersistableCookies()")),
                 "'cookie.json' is no longer written from the persistable cookies alone");

        QVERIFY2(source.contains(QStringLiteral("InitializeNetworkAccessManager(aft, bnam->GetNetworkCookieJar()->GetPersistableCookies())")),
                 "a copied space takes the mirrors of the space it came from");
        QVERIFY2(source.contains(QStringLiteral("a->GetAllCookies() + b->GetPersistableCookies()")),
                 "a merged space takes the mirrors of the space it swallowed");
    }

    void bothEnginesAskTheSettingAboutSessionCookies(){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                   QStringLiteral("/app/networkcontroller.cpp"));
        QVERIFY2(file.open(QIODevice::ReadOnly),
                 "'networkcontroller.cpp' was not read; check VANILLA_SOURCE_DIR");
        const QString source =
            QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

        QCOMPARE(source.count(QStringLiteral("setPersistentCookiesPolicy")), 3);
        QVERIFY2(source.contains(QStringLiteral("QWebEngineProfile::ForcePersistentCookies")),
                 "the widget profile no longer keeps session cookies when told to");
        QVERIFY2(source.contains(QStringLiteral("QQuickWebEngineProfile::ForcePersistentCookies")),
                 "the quick profile no longer keeps session cookies when told to");

        QVERIFY2(source.contains(QStringLiteral("if(Application::SaveSessionCookie())\n"
                                                "            profile->setPersistentCookiesPolicy")),
                 "the widget profile no longer reads the setting");
        QVERIFY2(source.contains(QStringLiteral("profile->setPersistentCookiesPolicy\n"
                                                "            (Application::SaveSessionCookie()")),
                 "the quick profile no longer reads the setting");
    }

    void cookieLoadOnlyConsidersJsonGenerations(){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                   QStringLiteral("/app/networkcontroller.cpp"));
        QVERIFY2(file.open(QIODevice::ReadOnly),
                 "'networkcontroller.cpp' was not read; check VANILLA_SOURCE_DIR");
        const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
        const int begin = source.indexOf(QStringLiteral("void NetworkController::LoadAllCookies"));
        const int end = source.indexOf(QStringLiteral("bool NetworkController::ShouldSaveCookie"), begin);
        QVERIFY(begin != -1);
        QVERIFY(end != -1);
        const QString body = source.mid(begin, end - begin);

        QVERIFY(body.contains(QStringLiteral("backup.endsWith(filename)")));
        QVERIFY(!body.contains(QStringLiteral("Legacy")));
        QVERIFY(!body.contains(QStringLiteral(".xml")));
        QVERIFY(!body.contains(QStringLiteral(".prev")));
    }

    void mirroringAnAddressReplacesWhatItAnswersFor(){
        NetworkCookieJar jar;
        const QUrl url(QStringLiteral("https://example.com/"));

        QNetworkCookie first("SID", "v1");
        first.setDomain(QStringLiteral("example.com"));
        first.setPath(QStringLiteral("/"));
        first.setExpirationDate(NOW.addYears(1));

        jar.MirrorScope(WEB, url, jar.BeginMirrorScope(WEB, url),
                        QList<QNetworkCookie>() << first);
        QCOMPARE(jar.GetAllCookies().count(), 1);
        QVERIFY(jar.GetPersistableCookies().isEmpty());

        QNetworkCookie second = first;
        second.setValue("v2");
        jar.MirrorScope(WEB, url, jar.BeginMirrorScope(WEB, url),
                        QList<QNetworkCookie>() << second);
        QCOMPARE(jar.GetAllCookies().count(), 1);
        QCOMPARE(jar.GetAllCookies().first().value(), QByteArray("v2"));

        jar.MirrorScope(WEB, url, jar.BeginMirrorScope(WEB, url),
                        QList<QNetworkCookie>());
        QVERIFY(jar.GetAllCookies().isEmpty());
    }

    void mirroringAnAddressLeavesWhatItDoesNotSpeakFor(){
        NetworkCookieJar jar;
        const QUrl url(QStringLiteral("https://example.com/"));

        QNetworkCookie elsewhere("SID", "other");
        elsewhere.setDomain(QStringLiteral("other.com"));
        elsewhere.setPath(QStringLiteral("/"));
        elsewhere.setExpirationDate(NOW.addYears(1));
        const QUrl other(QStringLiteral("https://other.com/"));
        jar.MirrorScope(WEB, other, jar.BeginMirrorScope(WEB, other),
                        QList<QNetworkCookie>() << elsewhere);

        QNetworkCookie owned("OWN", "x");
        owned.setDomain(QStringLiteral("example.com"));
        owned.setPath(QStringLiteral("/"));
        owned.setExpirationDate(NOW.addYears(1));
        jar.SetAllCookies(jar.GetAllCookies() << owned);

        jar.MirrorScope(WEB, url, jar.BeginMirrorScope(WEB, url),
                        QList<QNetworkCookie>());
        QCOMPARE(jar.GetAllCookies().count(), 2);

        const QList<QNetworkCookie> saved = jar.GetPersistableCookies();
        QCOMPARE(saved.count(), 1);
        QCOMPARE(saved.first().name(), QByteArray("OWN"));
    }

    void anAnswerAboutTheRootDoesNotSpeakForADeeperPath(){
        NetworkCookieJar jar;
        QNetworkCookie deeper("SID", "x");
        deeper.setDomain(QStringLiteral("example.com"));
        deeper.setPath(QStringLiteral("/dir"));
        deeper.setExpirationDate(NOW.addYears(1));
        const QUrl page(QStringLiteral("https://example.com/dir/page"));
        jar.MirrorScope(WEB, page, jar.BeginMirrorScope(WEB, page),
                        QList<QNetworkCookie>() << deeper);
        QCOMPARE(jar.GetAllCookies().count(), 1);

        const QUrl root(QStringLiteral("https://example.com/"));
        jar.MirrorScope(WEB, root, jar.BeginMirrorScope(WEB, root),
                        QList<QNetworkCookie>());
        QCOMPARE(jar.GetAllCookies().count(), 1);
    }

    void oneStoresAnswerDoesNotSpeakForAnothers(){
        NetworkCookieJar jar;
        const QUrl url(QStringLiteral("https://example.com/"));

        QNetworkCookie signedIn("SID", "web");
        signedIn.setDomain(QStringLiteral("example.com"));
        signedIn.setPath(QStringLiteral("/"));
        signedIn.setExpirationDate(NOW.addYears(1));
        jar.MirrorCookie(WEB, signedIn);

        jar.MirrorScope(EDGE, url, jar.BeginMirrorScope(EDGE, url),
                        QList<QNetworkCookie>());
        QCOMPARE(jar.GetAllCookies().count(), 1);
        QCOMPARE(jar.GetAllCookies().first().value(), QByteArray("web"));

        jar.UnmirrorCookie(EDGE, signedIn);
        QCOMPARE(jar.GetAllCookies().count(), 1);

        jar.UnmirrorCookie(WEB, signedIn);
        QVERIFY(jar.GetAllCookies().isEmpty());
    }

    void ananswerOlderThanTheLastOneAskedForIsDropped(){
        NetworkCookieJar jar;
        const QUrl url(QStringLiteral("https://example.com/"));

        QNetworkCookie v1("SID", "v1");
        v1.setDomain(QStringLiteral("example.com"));
        v1.setPath(QStringLiteral("/"));
        v1.setExpirationDate(NOW.addYears(1));
        QNetworkCookie v2 = v1;
        v2.setValue("v2");

        const quint64 older = jar.BeginMirrorScope(EDGE, url);
        const quint64 newer = jar.BeginMirrorScope(EDGE, url);
        jar.MirrorScope(EDGE, url, newer, QList<QNetworkCookie>() << v2);
        jar.MirrorScope(EDGE, url, older, QList<QNetworkCookie>() << v1);

        QCOMPARE(jar.GetAllCookies().count(), 1);
        QCOMPARE(jar.GetAllCookies().first().value(), QByteArray("v2"));

        const quint64 mine = jar.BeginMirrorScope(WEB, url);
        jar.MirrorScope(WEB, url, mine, QList<QNetworkCookie>());
        QCOMPARE(jar.GetAllCookies().count(), 1);
    }

    void acookieTheManagerCollectsIsNoLongerAMirror(){
        NetworkCookieJar jar;
        const QUrl url(QStringLiteral("https://example.com/"));

        QNetworkCookie mirrored("SID", "fromStore");
        mirrored.setDomain(QStringLiteral("example.com"));
        mirrored.setPath(QStringLiteral("/"));
        mirrored.setExpirationDate(NOW.addYears(1));
        jar.MirrorCookie(WEB, mirrored);
        QVERIFY(jar.GetPersistableCookies().isEmpty());

        QNetworkCookie own("SID", "fromRequest");
        own.setDomain(QStringLiteral("example.com"));
        own.setPath(QStringLiteral("/"));
        own.setExpirationDate(NOW.addYears(1));
        QVERIFY(jar.setCookiesFromUrl(QList<QNetworkCookie>() << own, url));

        QCOMPARE(jar.GetAllCookies().count(), 1);
        QCOMPARE(jar.GetAllCookies().first().value(), QByteArray("fromRequest"));
        const QList<QNetworkCookie> saved = jar.GetPersistableCookies();
        QCOMPARE(saved.count(), 1);
        QCOMPARE(saved.first().value(), QByteArray("fromRequest"));

        jar.MirrorScope(WEB, url, jar.BeginMirrorScope(WEB, url),
                        QList<QNetworkCookie>());
        QCOMPARE(jar.GetAllCookies().count(), 1);
    }

    void aremovalTheBaseRefusesLeavesTheMarkAlone(){
        NetworkCookieJar jar;
        QNetworkCookie mirrored("SID", "fromStore");
        mirrored.setDomain(QStringLiteral("example.com"));
        mirrored.setPath(QStringLiteral("/"));
        mirrored.setExpirationDate(NOW.addYears(1));
        jar.MirrorCookie(WEB, mirrored);
        QVERIFY(jar.GetPersistableCookies().isEmpty());

        QNetworkCookie absent("OTHER", "x");
        absent.setDomain(QStringLiteral("example.com"));
        absent.setPath(QStringLiteral("/"));
        QVERIFY(!jar.deleteCookie(absent));

        QNetworkCookie dotted("SID", "fromStore");
        dotted.setDomain(QStringLiteral(".example.com"));
        dotted.setPath(QStringLiteral("/"));
        dotted.setExpirationDate(NOW.addYears(1));
        QVERIFY(!jar.deleteCookie(dotted));

        QCOMPARE(jar.GetAllCookies().count(), 1);
        QVERIFY(jar.GetPersistableCookies().isEmpty());

        QVERIFY(jar.deleteCookie(mirrored));
        QVERIFY(jar.GetAllCookies().isEmpty());
    }

};

QTEST_MAIN(tst_cookiefilter)
#include "tst_cookiefilter.moc"
