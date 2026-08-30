#include <QtTest>

#include "certificatepolicy.hpp"

class tst_certificatepolicy : public QObject {
    Q_OBJECT

private slots:
    void theLeafCertificateAndHostNameTheDecision();
    void aDifferentCertificateDoesNotInheritTheHostDecision();
    void rememberingOneAnswerRemovesTheOppositeOne();
    void aMissingCertificateCanNeverBeRemembered();
};

void tst_certificatepolicy::theLeafCertificateAndHostNameTheDecision(){
    const QString key = CertificatePolicy::Key
        (QStringLiteral("EXAMPLE.com"), QByteArray("the leaf certificate's DER"));
    QVERIFY(key.startsWith(QStringLiteral("example.com:")));
    QCOMPARE(key.length(), QStringLiteral("example.com:").length() + 64);
    QCOMPARE(CertificatePolicy::KeyFromFingerprint
             (QStringLiteral("EXAMPLE.com"), key.section(QLatin1Char(':'), 1)), key);
}

void tst_certificatepolicy::aDifferentCertificateDoesNotInheritTheHostDecision(){
    const QString first = QStringLiteral("example.com:1111");
    const QString second = QStringLiteral("example.com:2222");
    QCOMPARE(CertificatePolicy::Find(first, QStringList() << first, QStringList()),
             CertificatePolicy::Allow);
    QCOMPARE(CertificatePolicy::Find(second, QStringList() << first, QStringList()),
             CertificatePolicy::Ask);
}

void tst_certificatepolicy::rememberingOneAnswerRemovesTheOppositeOne(){
    const QString key = QStringLiteral("example.com:abcd");
    QStringList allowed;
    QStringList blocked;
    CertificatePolicy::Remember(key, true, &allowed, &blocked);
    QCOMPARE(CertificatePolicy::Find(key, allowed, blocked), CertificatePolicy::Allow);
    CertificatePolicy::Remember(key, false, &allowed, &blocked);
    QCOMPARE(CertificatePolicy::Find(key, allowed, blocked), CertificatePolicy::Block);
    QVERIFY(!allowed.contains(key));
}

void tst_certificatepolicy::aMissingCertificateCanNeverBeRemembered(){
    QCOMPARE(CertificatePolicy::Key(QStringLiteral("example.com"), QByteArray()), QString());
    QStringList allowed;
    QStringList blocked;
    CertificatePolicy::Remember(QString(), true, &allowed, &blocked);
    QVERIFY(allowed.isEmpty());
    QVERIFY(blocked.isEmpty());
}

QTEST_MAIN(tst_certificatepolicy)
#include "tst_certificatepolicy.moc"
