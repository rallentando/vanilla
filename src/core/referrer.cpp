#include "switch.hpp"

#include "referrer.hpp"
#include "urlorigin.hpp"

namespace {

    bool IsHttp(const QUrl &url){
        const QString scheme = url.scheme();
        return scheme == QStringLiteral("http") || scheme == QStringLiteral("https");
    }

    int EffectivePort(const QUrl &url){
        return url.port(UrlOrigin::DefaultPort(url.scheme()));
    }

    bool SameOrigin(const QUrl &a, const QUrl &b){
        return a.scheme() == b.scheme() && a.host() == b.host() &&
            EffectivePort(a) == EffectivePort(b);
    }

    QByteArray Origin(const QUrl &url){
        QUrl origin;
        origin.setScheme(url.scheme());
        origin.setHost(url.host());
        if(EffectivePort(url) != UrlOrigin::DefaultPort(url.scheme())) origin.setPort(url.port());
        origin.setPath(QStringLiteral("/"));
        return origin.toEncoded();
    }
}

QByteArray Referrer::ForNavigation(const QUrl &referrer, const QUrl &target){
    if(!referrer.isValid() || !IsHttp(referrer) || referrer.host().isEmpty()) return QByteArray();

    if(referrer.scheme() == QStringLiteral("https") &&
       target.scheme() != QStringLiteral("https")) return QByteArray();

    const QByteArray stripped =
        referrer.adjusted(QUrl::RemoveFragment | QUrl::RemoveUserInfo).toEncoded();

    if(!SameOrigin(referrer, target) || stripped.size() > 4096) return Origin(referrer);
    return stripped;
}
