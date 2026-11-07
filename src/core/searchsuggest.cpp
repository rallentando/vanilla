#include "switch.hpp"
#include "searchsuggest.hpp"

#include <QCoreApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>

QUrl SearchSuggest::Url(const QString &engine, const QString &words){
    const QString language = QCoreApplication::translate("SearchSuggest", "en");
    const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(words));
    QUrl url;
    QUrlQuery query;
    if(engine == QStringLiteral("Bing")){
        url = QUrl(QStringLiteral("https://www.bing.com/osjson.aspx"));
        query.addQueryItem(QStringLiteral("query"), encoded);
        query.addQueryItem(QStringLiteral("language"), language);
    } else if(engine == QStringLiteral("DuckDuckGo")){
        url = QUrl(QStringLiteral("https://duckduckgo.com/ac/"));
        query.addQueryItem(QStringLiteral("q"), encoded);
        query.addQueryItem(QStringLiteral("type"), QStringLiteral("list"));
    } else {
        url = QUrl(QCoreApplication::translate("SearchSuggest", "https://www.google.com/complete/search"));
        query.addQueryItem(QStringLiteral("q"), encoded);
        query.addQueryItem(QStringLiteral("output"), QStringLiteral("toolbar"));
        query.addQueryItem(QStringLiteral("hl"), language);
        query.addQueryItem(QStringLiteral("oe"), QStringLiteral("utf-8"));
    }
    url.setQuery(query);
    return url;
}

QStringList SearchSuggest::Parse(const QByteArray &answer){
    QStringList list;
    const QJsonDocument json = QJsonDocument::fromJson(answer);
    if(json.isArray()){
        const QJsonArray words = json.array().at(1).toArray();
        for(const QJsonValue &word : words) list << word.toString();
        list.removeAll(QString());
        return list;
    }
    QDomDocument doc;
    if(doc.setContent(answer)){
        const QDomNodeList nodes = doc.elementsByTagName(QStringLiteral("suggestion"));
        for(int i = 0; i < nodes.length(); i++)
            list << nodes.item(i).toElement().attribute(QStringLiteral("data"));
    }
    return list;
}
