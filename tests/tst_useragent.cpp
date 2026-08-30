#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QStringList>
#include <QSet>

#include "useragent.hpp"
#include "networkcontroller.hpp"

#include <QNetworkRequest>

#include "testsupport.hpp"

static QStringList Expand(const QString &spelling){
    QStringList out;
    out << QString();

    int i = 0;
    while(i < spelling.length()){
        QStringList tails;

        if(spelling[i] == QLatin1Char('[')){
            const int close = spelling.indexOf(QLatin1Char(']'), i);
            for(int j = i + 1; j < close; j++)
                tails << spelling.mid(j, 1);
            i = close + 1;

        } else if(spelling.mid(i, 3) == QStringLiteral("(?:")){
            int depth = 0, close = i;
            for(int j = i; j < spelling.length(); j++){
                if(spelling[j] == QLatin1Char('(')) depth++;
                if(spelling[j] == QLatin1Char(')')){
                    depth--;
                    if(!depth){ close = j; break;}
                }
            }
            tails << Expand(spelling.mid(i + 3, close - i - 3));
            i = close + 1;

        } else {
            tails << spelling.mid(i, 1);
            i++;
        }

        if(i < spelling.length() && spelling[i] == QLatin1Char('?')){
            tails << QString();
            i++;
        }

        QStringList next;
        foreach(QString head, out)
            foreach(QString tail, tails)
                next << head + tail;
        out = next;
    }
    return out;
}

class tst_useragent : public QObject {
    Q_OBJECT

private slots:

    void thetableHoldsTheBrowsersTheIfElseChainHeld(){
        QStringList names;
        foreach(UserAgent::Entry entry, UserAgent::Entries())
            names << QString::fromLatin1(entry.name);

        QCOMPARE(names, QStringList()
                 << QStringLiteral("IE")        << QStringLiteral("Edge")
                 << QStringLiteral("Firefox")   << QStringLiteral("Opera")
                 << QStringLiteral("OPR")       << QStringLiteral("Safari")
                 << QStringLiteral("Chrome")    << QStringLiteral("Sleipnir")
                 << QStringLiteral("Vivaldi")   << QStringLiteral("NetScape")
                 << QStringLiteral("SeaMonkey") << QStringLiteral("iCab")
                 << QStringLiteral("Camino")    << QStringLiteral("Gecko")
                 << QStringLiteral("Custom"));
    }

    void thenamesEachSpellingAccepts(){
        QCOMPARE(UserAgent::NameOf(QStringLiteral("ff")),       QStringLiteral("Firefox"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("firefox")),  QStringLiteral("Firefox"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("FireFox")),  QStringLiteral("Firefox"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("ie")),       QStringLiteral("IE"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("InternetExplorer")), QStringLiteral("IE"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("chrome")),   QStringLiteral("Chrome"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("opr")),      QStringLiteral("OPR"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("opera")),    QStringLiteral("Opera"));
        QCOMPARE(UserAgent::NameOf(QStringLiteral("iCab")),     QStringLiteral("iCab"));
    }

    void nospellingReachesAnotherBrowsersRow(){
        int names = 0;
        foreach(UserAgent::Entry entry, UserAgent::Entries()){
            const QString expected = QString::fromLatin1(entry.name);

            foreach(QString name, Expand(QString::fromLatin1(entry.spelling))){
                names++;
                QVERIFY2(UserAgent::NameOf(name) == expected,
                         qPrintable(QStringLiteral("'%1' is '%2', not '%3'")
                                    .arg(name).arg(UserAgent::NameOf(name)).arg(expected)));
            }
        }
        QCOMPARE(names, 70);
    }

    void avalueWhichNamesNoBrowserIsLeftAlone(){
        const QString full = QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) "
                                            "AppleWebKit/537.36 (KHTML, like Gecko) "
                                            "Chrome/140.0.0.0 Safari/537.36");
        QVERIFY(UserAgent::NameOf(full).isEmpty());
        QVERIFY(UserAgent::NameOf(QStringLiteral("chromium")).isEmpty());
        QVERIFY(UserAgent::NameOf(QString()).isEmpty());
    }

    void thesettingsKeyIsTheCanonicalName(){
        QCOMPARE(UserAgent::SettingsKey(QStringLiteral("Firefox")),
                 QStringLiteral("application/UserAgent_Firefox"));
        QVERIFY(UserAgent::Find(QStringLiteral("FF")) == nullptr);
        QVERIFY(UserAgent::Find(QStringLiteral("Firefox")) != nullptr);
    }

    void expandFillsThePlaceholdersThenDecodes(){
        QCOMPARE(UserAgent::Expand(QStringLiteral("A/%SYSTEM% B/%LOCATION% C/%CHROMIUM%"),
                                   QStringLiteral("Windows NT 10.0; Win64; x64"),
                                   QStringLiteral("ja-JP"),
                                   QStringLiteral("140.0.0.0")),
                 QStringLiteral("A/Windows NT 10.0; Win64; x64 B/ja-JP C/140.0.0.0"));

        QCOMPARE(UserAgent::Expand(QStringLiteral("My%20Browser/1.0"),
                                   QString(), QString(), QString()),
                 QStringLiteral("My Browser/1.0"));

        QCOMPARE(UserAgent::Expand(QStringLiteral("Mozilla/5.0 (compatible; iCab 3.0.3)"),
                                   QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z")),
                 QStringLiteral("Mozilla/5.0 (compatible; iCab 3.0.3)"));
    }

    void everyBrowserButCustomHasABuiltInTemplate(){
        foreach(UserAgent::Entry entry, UserAgent::Entries()){
            const QString name = QString::fromLatin1(entry.name);
            if(name == QStringLiteral("Custom")){
                QVERIFY(UserAgent::Fallback(name).isEmpty());
                continue;
            }
            QVERIFY2(!UserAgent::Fallback(name).isEmpty(), qPrintable(name));
            QVERIFY2(UserAgent::Fallback(name).startsWith(QStringLiteral("Mozilla/5.0")) ||
                     UserAgent::Fallback(name).startsWith(QStringLiteral("Opera/")),
                     qPrintable(UserAgent::Fallback(name)));
        }
        QVERIFY(UserAgent::Fallback(QStringLiteral("NoSuchBrowser")).isEmpty());
    }

    void loadingAnEmptySettingsMapGivesTheBuiltInOnes(){
        SettingsIO::Map settings;
        const UserAgent::Map agents = UserAgent::Load(settings);

        QCOMPARE(agents.count(), UserAgent::Entries().length());
        foreach(UserAgent::Entry entry, UserAgent::Entries()){
            const QString name = QString::fromLatin1(entry.name);
            QCOMPARE(agents[name], UserAgent::Fallback(name));
        }
    }

    void anemptyValueMeansTheBuiltInOne(){
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("application/UserAgent_Chrome"), QString());

        const UserAgent::Map agents = UserAgent::Load(settings);
        QCOMPARE(agents[QStringLiteral("Chrome")], UserAgent::Fallback(QStringLiteral("Chrome")));
    }

    void avalueTheUserSetSurvivesTheRoundTrip(){
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("application/UserAgent_Chrome"), QStringLiteral("mine/1.0"));
        settings.insert(QStringLiteral("application/UserAgent_Custom"), QStringLiteral("also mine"));

        UserAgent::Map agents = UserAgent::Load(settings);
        QCOMPARE(agents[QStringLiteral("Chrome")], QStringLiteral("mine/1.0"));
        QCOMPARE(agents[QStringLiteral("Custom")], QStringLiteral("also mine"));

        SettingsIO::Map written;
        UserAgent::Save(written, agents);
        QCOMPARE(written[QStringLiteral("application/UserAgent_Chrome")].toString(),
                 QStringLiteral("mine/1.0"));
        QCOMPARE(written[QStringLiteral("application/UserAgent_Custom")].toString(),
                 QStringLiteral("also mine"));

        QCOMPARE(UserAgent::Load(written), agents);
    }

    void abuiltInTemplateIsWrittenAsNothing(){
        const UserAgent::Map agents = UserAgent::Load(SettingsIO::Map());

        SettingsIO::Map written;
        UserAgent::Save(written, agents);

        QCOMPARE(written.count(), UserAgent::Entries().length());
        foreach(UserAgent::Entry entry, UserAgent::Entries()){
            const QString key = UserAgent::SettingsKey(QString::fromLatin1(entry.name));
            QVERIFY2(written[key].toString().isEmpty(), qPrintable(key));
        }
        QCOMPARE(UserAgent::Load(written), agents);
    }

    void thekeysWrittenAreTheKeysRead(){
        SettingsIO::Map written;
        UserAgent::Save(written, UserAgent::Load(SettingsIO::Map()));

        QSet<QString> keys;
        foreach(UserAgent::Entry entry, UserAgent::Entries())
            keys << UserAgent::SettingsKey(QString::fromLatin1(entry.name));

        QCOMPARE(QSet<QString>(written.keyBegin(), written.keyEnd()), keys);
    }

    void themanagerCompletesANavigationWithWhatABrowserCarries(){
        QNetworkRequest bare(QUrl(QStringLiteral("https://example.com/file.bin")));
        NetworkAccessManager::SetRequestPurpose(bare, NetworkAccessManager::Navigation);
        const QNetworkRequest done =
            NetworkAccessManager::CompletedRequest(bare, QStringLiteral("Vanilla/1.0"));

        QCOMPARE(done.rawHeader("User-Agent"), QByteArray("Vanilla/1.0"));
        QVERIFY(done.rawHeader("Accept").startsWith("text/html"));
        QCOMPARE(done.rawHeader("Upgrade-Insecure-Requests"), QByteArray("1"));
        QCOMPARE(done.rawHeader("Sec-Fetch-Dest"), QByteArray("document"));
        QCOMPARE(done.rawHeader("Sec-Fetch-Mode"), QByteArray("navigate"));
        QCOMPARE(done.rawHeader("Sec-Fetch-User"), QByteArray("?1"));

        QVERIFY(!done.hasRawHeader("Accept-Encoding"));
    }

    void everythingElseGetsTheUserAgentAndNothingMore(){
        QNetworkRequest quiet(QUrl(QStringLiteral("https://example.com/suggest?q=x")));
        quiet.setRawHeader("Referer", "https://example.com/page.html");
        const QNetworkRequest done =
            NetworkAccessManager::CompletedRequest(quiet, QStringLiteral("Vanilla/1.0"));

        QCOMPARE(done.rawHeader("User-Agent"), QByteArray("Vanilla/1.0"));
        QVERIFY(!done.hasRawHeader("Accept"));
        QVERIFY(!done.hasRawHeader("Upgrade-Insecure-Requests"));
        QVERIFY(!done.hasRawHeader("Sec-Fetch-Dest"));
        QVERIFY(!done.hasRawHeader("Sec-Fetch-Mode"));
        QVERIFY(!done.hasRawHeader("Sec-Fetch-User"));
        QVERIFY(!done.hasRawHeader("Sec-Fetch-Site"));

        QNetworkRequest said(quiet);
        NetworkAccessManager::SetRequestPurpose(said, NetworkAccessManager::Subresource);
        QVERIFY(!NetworkAccessManager::CompletedRequest(said, QStringLiteral("Vanilla/1.0"))
                .hasRawHeader("Sec-Fetch-Dest"));
    }

    void whereTheRequestSaysItCameFrom(){
        const QUrl file(QStringLiteral("https://example.com/file.bin"));

        QNetworkRequest same(file);
        NetworkAccessManager::SetRequestPurpose(same, NetworkAccessManager::Navigation);
        same.setRawHeader("Referer", "https://example.com/page.html");
        QCOMPARE(NetworkAccessManager::CompletedRequest(same, QString())
                 .rawHeader("Sec-Fetch-Site"), QByteArray("same-origin"));

        QNetworkRequest port(file);
        NetworkAccessManager::SetRequestPurpose(port, NetworkAccessManager::Navigation);
        port.setRawHeader("Referer", "https://example.com:443/page.html");
        QCOMPARE(NetworkAccessManager::CompletedRequest(port, QString())
                 .rawHeader("Sec-Fetch-Site"), QByteArray("same-origin"));

        foreach(const QByteArray &referer, QList<QByteArray>()
                << QByteArray()
                << "https://sub.example.com/page.html"
                << "https://other.com/page.html"
                << "http://example.com/page.html"
                << "https://example.com:8443/page.html"){
            QNetworkRequest req(file);
            NetworkAccessManager::SetRequestPurpose(req, NetworkAccessManager::Navigation);
            if(!referer.isEmpty()) req.setRawHeader("Referer", referer);
            QVERIFY2(!NetworkAccessManager::CompletedRequest(req, QString())
                     .hasRawHeader("Sec-Fetch-Site"),
                     qPrintable(QStringLiteral("said where it came from, from '%1'")
                                .arg(QString::fromLatin1(referer))));
        }
    }

    void thecallersOwnHeadersAreNotOverwritten(){
        QNetworkRequest mine(QUrl(QStringLiteral("https://example.com/file.bin")));
        NetworkAccessManager::SetRequestPurpose(mine, NetworkAccessManager::Navigation);
        mine.setRawHeader("User-Agent", "Something/2.0");
        mine.setRawHeader("Accept", "application/json");
        mine.setRawHeader("Sec-Fetch-Mode", "cors");
        const QNetworkRequest done =
            NetworkAccessManager::CompletedRequest(mine, QStringLiteral("Vanilla/1.0"));
        QCOMPARE(done.rawHeader("User-Agent"), QByteArray("Something/2.0"));
        QCOMPARE(done.rawHeader("Accept"), QByteArray("application/json"));
        QCOMPARE(done.rawHeader("Sec-Fetch-Mode"), QByteArray("cors"));
        QVERIFY(!done.hasRawHeader("Sec-Fetch-Dest"));
        QVERIFY(!done.hasRawHeader("Sec-Fetch-User"));

        QNetworkRequest local(QUrl(QStringLiteral("file:///C:/tmp/x.bin")));
        NetworkAccessManager::SetRequestPurpose(local, NetworkAccessManager::Navigation);
        const QNetworkRequest untouched =
            NetworkAccessManager::CompletedRequest(local, QStringLiteral("Vanilla/1.0"));
        QVERIFY(!untouched.hasRawHeader("User-Agent"));
        QVERIFY(!untouched.hasRawHeader("Sec-Fetch-Site"));
    }

};

QTEST_MAIN(tst_useragent)
#include "tst_useragent.moc"
