#ifndef URLORIGIN_HPP
#define URLORIGIN_HPP

#include <QHostAddress>
#include <QString>
#include <QUrl>

namespace UrlOrigin {

    inline int DefaultPort(const QString &scheme){
        return scheme == QStringLiteral("https") ? 443 : 80;
    }

    inline QString Host(const QUrl &url){
        const QString host = url.host();
        if(host.isEmpty()) return QString();

        QHostAddress address;
        if(address.setAddress(host) &&
           address.protocol() == QAbstractSocket::IPv6Protocol)
            return QStringLiteral("[") + address.toString().toLower() + QStringLiteral("]");

        return QString::fromLatin1(QUrl::toAce(host)).toLower();
    }

}

#endif
