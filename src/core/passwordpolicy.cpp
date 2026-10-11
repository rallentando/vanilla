#include "switch.hpp"

#include "passwordpolicy.hpp"

#include <QHostAddress>

namespace {

    int DefaultPort(const QString &scheme){
        return scheme == QStringLiteral("https") ? 443 : 80;
    }

    QString OriginHost(const QUrl &url){
        const QString host = url.host();
        if(host.isEmpty()) return QString();

        QHostAddress address;
        if(address.setAddress(host) &&
           address.protocol() == QAbstractSocket::IPv6Protocol)
            return QStringLiteral("[") + address.toString().toLower() + QStringLiteral("]");

        return QString::fromLatin1(QUrl::toAce(host)).toLower();
    }

    QString EligibleScheme(const QUrl &url){
        if(!url.isValid()) return QString();
        const QString scheme = url.scheme().toLower();
        if(scheme == QStringLiteral("https")) return scheme;
        if(scheme == QStringLiteral("http") && PasswordPolicy::IsLoopbackHost(url.host()))
            return scheme;
        return QString();
    }

    QString Escape(QString part){
        part.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
        part.replace(QStringLiteral("|"),  QStringLiteral("\\|"));
        return part;
    }
}

namespace PasswordPolicy {

QString Origin(const QUrl &url){
    const QString scheme = EligibleScheme(url);
    if(scheme.isEmpty()) return QString();
    const QString host = OriginHost(url);
    if(host.isEmpty()) return QString();
    return scheme + QStringLiteral("://") + host + QStringLiteral(":") +
        QString::number(url.port(DefaultPort(scheme)));
}

QString ScriptOrigin(const QUrl &url){
    const QString scheme = EligibleScheme(url);
    if(scheme.isEmpty()) return QString();
    const QString host = OriginHost(url);
    if(host.isEmpty()) return QString();
    const int port = url.port(DefaultPort(scheme));
    if(port == DefaultPort(scheme)) return scheme + QStringLiteral("://") + host;
    return scheme + QStringLiteral("://") + host + QStringLiteral(":") + QString::number(port);
}

bool IsLoopbackHost(const QString &host){
    if(host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) return true;
    QHostAddress address;
    return address.setAddress(host) && address.isLoopback();
}

QString VaultKey(const QString &prefix, const QString &space,
                 const QString &origin, const QString &username){
    return Escape(prefix) + QStringLiteral("|") + Escape(space) + QStringLiteral("|") +
        Escape(origin) + QStringLiteral("|") + Escape(username);
}

}
