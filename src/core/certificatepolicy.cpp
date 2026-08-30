#include "certificatepolicy.hpp"

#include <QCryptographicHash>

namespace CertificatePolicy {

QString Fingerprint(const QList<QSslCertificate> &chain){
    if(chain.isEmpty() || chain.first().isNull()) return QString();
    return QString::fromLatin1
        (QCryptographicHash::hash(chain.first().toDer(), QCryptographicHash::Sha256).toHex());
}

QString KeyFromFingerprint(const QString &host, const QString &fingerprint){
    if(host.isEmpty() || fingerprint.isEmpty()) return QString();
    return host.toLower() + QLatin1Char(':') + fingerprint;
}

QString Key(const QString &host, const QByteArray &leafDer){
    if(host.isEmpty() || leafDer.isEmpty()) return QString();
    const QString fingerprint = QString::fromLatin1
        (QCryptographicHash::hash(leafDer, QCryptographicHash::Sha256).toHex());
    return KeyFromFingerprint(host, fingerprint);
}

QString Key(const QString &host, const QList<QSslCertificate> &chain){
    return KeyFromFingerprint(host, Fingerprint(chain));
}

Decision Find(const QString &key, const QStringList &allowed, const QStringList &blocked){
    if(key.isEmpty()) return Ask;
    if(blocked.contains(key)) return Block;
    if(allowed.contains(key)) return Allow;
    return Ask;
}

void Remember(const QString &key, bool allow, QStringList *allowed, QStringList *blocked){
    if(key.isEmpty() || !allowed || !blocked) return;
    allowed->removeAll(key);
    blocked->removeAll(key);
    (allow ? allowed : blocked)->append(key);
}

}
