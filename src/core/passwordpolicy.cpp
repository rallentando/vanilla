#include "switch.hpp"

#include "passwordpolicy.hpp"

#include "urlorigin.hpp"

namespace {

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
    const QString host = UrlOrigin::Host(url);
    if(host.isEmpty()) return QString();
    return scheme + QStringLiteral("://") + host + QStringLiteral(":") +
        QString::number(url.port(UrlOrigin::DefaultPort(scheme)));
}

QString ScriptOrigin(const QUrl &url){
    const QString scheme = EligibleScheme(url);
    if(scheme.isEmpty()) return QString();
    const QString host = UrlOrigin::Host(url);
    if(host.isEmpty()) return QString();
    const int port = url.port(UrlOrigin::DefaultPort(scheme));
    if(port == UrlOrigin::DefaultPort(scheme)) return scheme + QStringLiteral("://") + host;
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
