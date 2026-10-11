#include "switch.hpp"

#include <QtTest>

#include "passwordpolicy.hpp"

class tst_passwordpolicy : public QObject {
    Q_OBJECT

private slots:

    void theOriginIsMadeFromTheAddress_data(){
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("origin");
        QTest::addColumn<QString>("scriptOrigin");

        QTest::newRow("https, the default port filled in")
            << QStringLiteral("https://example.com/login?x=1#top")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("the default port written out is the same origin")
            << QStringLiteral("https://example.com:443/")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("another port is another origin")
            << QStringLiteral("https://example.com:8443/")
            << QStringLiteral("https://example.com:8443")
            << QStringLiteral("https://example.com:8443");
        QTest::newRow("upper case comes out lower")
            << QStringLiteral("HTTPS://Example.COM/")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("an IDN is its punycode")
            << QStringLiteral("https://\u4F8B\u3048.jp/")
            << QStringLiteral("https://xn--r8jz45g.jp:443")
            << QStringLiteral("https://xn--r8jz45g.jp");
        QTest::newRow("user info is not part of it")
            << QStringLiteral("https://user:pass@example.com/")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("http on 127.0.0.1")
            << QStringLiteral("http://127.0.0.1:8765/login.html")
            << QStringLiteral("http://127.0.0.1:8765")
            << QStringLiteral("http://127.0.0.1:8765");
        QTest::newRow("http anywhere in 127.0.0.0/8")
            << QStringLiteral("http://127.1.2.3/")
            << QStringLiteral("http://127.1.2.3:80")
            << QStringLiteral("http://127.1.2.3");
        QTest::newRow("http on localhost")
            << QStringLiteral("http://localhost/")
            << QStringLiteral("http://localhost:80")
            << QStringLiteral("http://localhost");
        QTest::newRow("http on ::1, in brackets")
            << QStringLiteral("http://[::1]:8080/")
            << QStringLiteral("http://[::1]:8080")
            << QStringLiteral("http://[::1]:8080");
        QTest::newRow("https on IPv6")
            << QStringLiteral("https://[2001:DB8::1]/")
            << QStringLiteral("https://[2001:db8::1]:443")
            << QStringLiteral("https://[2001:db8::1]");

        QTest::newRow("http over the network: none")
            << QStringLiteral("http://example.com/") << QString() << QString();
        QTest::newRow("a name which only ends like localhost: none")
            << QStringLiteral("http://evil-localhost/") << QString() << QString();
        QTest::newRow("localhost as a subdomain: none")
            << QStringLiteral("http://localhost.example.com/") << QString() << QString();
        QTest::newRow("file: none")
            << QStringLiteral("file:///C:/login.html") << QString() << QString();
        QTest::newRow("data: none")
            << QStringLiteral("data:text/html,<form>") << QString() << QString();
        QTest::newRow("about:blank: none")
            << QStringLiteral("about:blank") << QString() << QString();
        QTest::newRow("vanilla:// none")
            << QStringLiteral("vanilla://settings") << QString() << QString();
        QTest::newRow("empty: none")
            << QString() << QString() << QString();
    }
    void theOriginIsMadeFromTheAddress(){
        QFETCH(QString, url);
        QFETCH(QString, origin);
        QFETCH(QString, scriptOrigin);
        QCOMPARE(PasswordPolicy::Origin(QUrl(url)), origin);
        QCOMPARE(PasswordPolicy::ScriptOrigin(QUrl(url)), scriptOrigin);
    }

    void theKeyKeepsItsPartsApart(){
        const QString prefix = QStringLiteral("vanilla-0123");
        const QString a = PasswordPolicy::VaultKey(prefix, QStringLiteral("space"),
                                                   QStringLiteral("https://a.example:443"),
                                                   QStringLiteral("alice"));
        QCOMPARE(a, QStringLiteral("vanilla-0123|space|https://a.example:443|alice"));

        const QString forged = PasswordPolicy::VaultKey(prefix, QStringLiteral("space"),
                                                        QStringLiteral("https://a.example:443"),
                                                        QStringLiteral("x|https://b.example:443|alice"));
        const QString other = PasswordPolicy::VaultKey(prefix, QStringLiteral("space"),
                                                       QStringLiteral("https://b.example:443"),
                                                       QStringLiteral("alice"));
        QVERIFY(forged != other);
        QCOMPARE(forged, QStringLiteral("vanilla-0123|space|https://a.example:443|x\\|https://b.example:443\\|alice"));

        QCOMPARE(PasswordPolicy::VaultKey(prefix, QStringLiteral("s"), QStringLiteral("o"),
                                          QStringLiteral("a\\")),
                 QStringLiteral("vanilla-0123|s|o|a\\\\"));
        QVERIFY(PasswordPolicy::VaultKey(prefix, QStringLiteral("s\\"), QStringLiteral("|o"), QString()) !=
                PasswordPolicy::VaultKey(prefix, QStringLiteral("s"), QStringLiteral("\\|o"), QString()));

        QVERIFY(PasswordPolicy::VaultKey(prefix, QStringLiteral("one"), QStringLiteral("o"), QStringLiteral("u")) !=
                PasswordPolicy::VaultKey(prefix, QStringLiteral("two"), QStringLiteral("o"), QStringLiteral("u")));
    }
};

QTEST_MAIN(tst_passwordpolicy)
#include "tst_passwordpolicy.moc"
