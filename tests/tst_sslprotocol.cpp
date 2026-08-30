#include "switch.hpp"

#include <QtTest>
#include <QSsl>

#include "networkcontroller.hpp"

class tst_sslprotocol : public QObject {
    Q_OBJECT

private slots:

    void theSpellingsATokenIsReadBy_data(){
        QTest::addColumn<QString>("given");
        QTest::addColumn<int>("expected");

        QTest::newRow("the whole token")
            << QStringLiteral("Ssl TLSv1.2") << int(QSsl::TlsV1_2);
        QTest::newRow("the version alone")
            << QStringLiteral("TLSv1.2") << int(QSsl::TlsV1_2);
        QTest::newRow("written out")
            << QStringLiteral("Ssl TLSversion1.2") << int(QSsl::TlsV1_2);
        QTest::newRow("lower case")
            << QStringLiteral("ssl tlsv1.2") << int(QSsl::TlsV1_2);

        QTest::newRow("any")
            << QStringLiteral("Ssl Any") << int(QSsl::AnyProtocol);
        QTest::newRow("any, written out")
            << QStringLiteral("Ssl AnyProtocol") << int(QSsl::AnyProtocol);
        QTest::newRow("secure")
            << QStringLiteral("Ssl Secure") << int(QSsl::SecureProtocols);
        QTest::newRow("secure, written out")
            << QStringLiteral("Ssl SecureProtocol") << int(QSsl::SecureProtocols);

        QTest::newRow("tls 1.0, which is not offered any more")
            << QStringLiteral("Ssl TLSv1") << int(QSsl::SecureProtocols);
        QTest::newRow("tls 1.0 written with its zero")
            << QStringLiteral("Ssl TLSv1.0") << int(QSsl::SecureProtocols);
        QTest::newRow("tls 1.1, which is not offered any more")
            << QStringLiteral("Ssl TLSv1.1") << int(QSsl::SecureProtocols);
        QTest::newRow("tls 1.0 with ssl 3, which is both")
            << QStringLiteral("Ssl TLSv1SSLv3") << int(QSsl::SecureProtocols);

        QTest::newRow("a spelling nobody wrote")
            << QStringLiteral("Ssl TLSv9") << int(QSsl::UnknownProtocol);
        QTest::newRow("a protocol which was never offered here")
            << QStringLiteral("Ssl SSLv3") << int(QSsl::UnknownProtocol);
        QTest::newRow("the token with nothing after it")
            << QStringLiteral("Ssl") << int(QSsl::UnknownProtocol);
        QTest::newRow("nothing at all")
            << QString() << int(QSsl::UnknownProtocol);
    }

    void theSpellingsATokenIsReadBy(){
        QFETCH(QString, given);
        QFETCH(int, expected);
        QCOMPARE(int(NetworkAccessManager::SslProtocolForSetting(given)), expected);
    }

    void everySpellingAnswersOneOfTheProtocolsWhichAreOffered(){
        const QList<QSsl::SslProtocol> offered = {
            QSsl::TlsV1_2, QSsl::AnyProtocol, QSsl::SecureProtocols,
            QSsl::UnknownProtocol
        };

        const QStringList spellings = {
            QStringLiteral("Ssl TLSv1"),        QStringLiteral("Ssl TLSv1.0"),
            QStringLiteral("Ssl TLSv1.1"),      QStringLiteral("Ssl TLSv1SSLv3"),
            QStringLiteral("Ssl TLSversion1"),  QStringLiteral("Ssl TLSversion1.1"),
            QStringLiteral("Ssl tlsv1"),        QStringLiteral("Ssl TLSv1.2"),
            QStringLiteral("Ssl Any"),          QStringLiteral("Ssl Secure"),
            QStringLiteral("Ssl nonsense"),     QString()
        };

        for(const QString &spelling : spellings)
            QVERIFY2(offered.contains(NetworkAccessManager::SslProtocolForSetting(spelling)),
                     qPrintable(spelling));
    }

};

QTEST_MAIN(tst_sslprotocol)
#include "tst_sslprotocol.moc"
