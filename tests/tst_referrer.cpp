#include "switch.hpp"

#include <QtTest>

#include "referrer.hpp"

class tst_referrer : public QObject {
    Q_OBJECT

private slots:

    void theHeaderIsWhatThePolicyWouldHaveSent_data(){
        QTest::addColumn<QString>("referrer");
        QTest::addColumn<QString>("target");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("the report: a search sent to another site is its origin")
            << QStringLiteral("https://www.google.co.jp/search?q=%E3%83%A6&ie=UTF-8")
            << QStringLiteral("https://testpage.jp/tool/ip_user_agent.php")
            << QByteArray("https://www.google.co.jp/");
        QTest::newRow("the same origin keeps the whole address")
            << QStringLiteral("https://example.com/a/b?c=d")
            << QStringLiteral("https://example.com/e")
            << QByteArray("https://example.com/a/b?c=d");
        QTest::newRow("without the fragment and the user info")
            << QStringLiteral("https://user:pass@example.com/a#top")
            << QStringLiteral("https://example.com/e")
            << QByteArray("https://example.com/a");
        QTest::newRow("another port is another origin")
            << QStringLiteral("http://127.0.0.1:8771/page")
            << QStringLiteral("http://127.0.0.1:8772/page")
            << QByteArray("http://127.0.0.1:8771/");
        QTest::newRow("a default port written out is the same origin")
            << QStringLiteral("https://example.com:443/a")
            << QStringLiteral("https://example.com/e")
            << QByteArray("https://example.com:443/a");
        QTest::newRow("a default port written out is not kept in the origin")
            << QStringLiteral("https://example.com:443/a")
            << QStringLiteral("https://example.org/")
            << QByteArray("https://example.com/");
        QTest::newRow("another subdomain is another origin")
            << QStringLiteral("https://a.example.com/x")
            << QStringLiteral("https://b.example.com/y")
            << QByteArray("https://a.example.com/");
        QTest::newRow("from http to https is the origin")
            << QStringLiteral("http://example.com/x")
            << QStringLiteral("https://example.com/x")
            << QByteArray("http://example.com/");
        QTest::newRow("from https to http is nothing")
            << QStringLiteral("https://example.com/x")
            << QStringLiteral("http://example.com/x")
            << QByteArray();
        QTest::newRow("from https to a local http server is nothing too")
            << QStringLiteral("https://example.com/x")
            << QStringLiteral("http://localhost/x")
            << QByteArray();
        QTest::newRow("a file is not a referrer")
            << QStringLiteral("file:///C:/page.html")
            << QStringLiteral("https://example.com/")
            << QByteArray();
        QTest::newRow("nor is another scheme with a host")
            << QStringLiteral("ftp://example.com/file")
            << QStringLiteral("https://example.com/")
            << QByteArray();
        QTest::newRow("nor is no address")
            << QString()
            << QStringLiteral("https://example.com/")
            << QByteArray();
    }

    void theHeaderIsWhatThePolicyWouldHaveSent(){
        QFETCH(QString, referrer);
        QFETCH(QString, target);
        QFETCH(QByteArray, expected);
        QCOMPARE(Referrer::ForNavigation(QUrl(referrer), QUrl(target)), expected);
    }

    void aLongAddressIsItsOrigin(){
        const QString base = QStringLiteral("https://example.com/?q=");
        const QUrl fits(base + QString(4096 - base.size(), QLatin1Char('a')));
        const QUrl over(base + QString(4097 - base.size(), QLatin1Char('a')));
        const QUrl target(QStringLiteral("https://example.com/next"));

        QCOMPARE(Referrer::ForNavigation(fits, target), fits.toEncoded());
        QCOMPARE(Referrer::ForNavigation(over, target), QByteArray("https://example.com/"));
    }
};

QTEST_MAIN(tst_referrer)
#include "tst_referrer.moc"
