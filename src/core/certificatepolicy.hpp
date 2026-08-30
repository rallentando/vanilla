#ifndef CERTIFICATEPOLICY_HPP
#define CERTIFICATEPOLICY_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QSslCertificate>

namespace CertificatePolicy {

enum Decision {
    Ask,
    Allow,
    Block
};

QString Fingerprint(const QList<QSslCertificate> &chain);
QString KeyFromFingerprint(const QString &host, const QString &fingerprint);
QString Key(const QString &host, const QByteArray &leafDer);
QString Key(const QString &host, const QList<QSslCertificate> &chain);
Decision Find(const QString &key, const QStringList &allowed, const QStringList &blocked);
void Remember(const QString &key, bool allow, QStringList *allowed, QStringList *blocked);

}

#endif
