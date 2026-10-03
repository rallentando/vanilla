#include "switch.hpp"

#include "useragent.hpp"

#include <QRegularExpression>
#include <QUrl>

#include <iterator>

namespace UserAgent {

static const Entry TheEntries[] = {
    { "[iI](?:nternet)?[eE](?:xplorer)?", "IE",
      "Mozilla/5.0 (%SYSTEM%; Trident/7.0; rv:11.0) like Gecko" },

    { "[eE]dge", "Edge",
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/%CHROMIUM% Safari/537.36 Edg/%CHROMIUM%" },

    { "[fF](?:ire)?[fF](?:ox)?", "Firefox",
      "Mozilla/5.0 (%SYSTEM%; rv:140.0) Gecko/20100101 Firefox/140.0" },

    { "[oO]pera", "Opera",
      "Opera/9.80 (%SYSTEM%) Presto/2.12.388 Version/12.18" },

    { "[oO][pP][rR]", "OPR",
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/%CHROMIUM% Safari/537.36 OPR/126.0.0.0" },

    { "[sS]afari", "Safari",
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15" },

    { "[cC]hrome", "Chrome",
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/%CHROMIUM% Safari/537.36" },

    { "[sS]leipnir", "Sleipnir",
#if defined(Q_OS_MAC)
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15 Sleipnir/4.7.2064" },
#else
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/%CHROMIUM% Safari/537.36 Sleipnir/6.5.4" },
#endif

    { "[vV]ivaldi", "Vivaldi",
      "Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/%CHROMIUM% Safari/537.36 Vivaldi/7.5" },

    { "[nN]et[sS]cape", "NetScape",
      "Mozilla/5.0 (Windows; U; %SYSTEM%; en-US; rv:1.7.2) Gecko/20040804 Netscape/7.2 (ax)" },

    { "[sS]ea[mM]onkey", "SeaMonkey",
      "Mozilla/5.0 (%SYSTEM%; rv:60.0) Gecko/20100101 Firefox/60.0 SeaMonkey/2.53.21" },

    { "[iI][cC]ab", "iCab",
      "Mozilla/5.0 (compatible; iCab 3.0.3; Macintosh; U; PPC Mac OS X)" },

    { "[cC]amino", "Camino",
      "Mozilla/5.0 (Macintosh; U; PPC Mac OS X 10.4; en; rv:1.9.2.24) Gecko/20111114 Camino/2.1 (like Firefox/3.6.24)" },

    { "[gG]ecko", "Gecko",
      "Mozilla/5.0 (%SYSTEM%; rv:140.0) Gecko/20100101" },

    { "[cC]ustom", "Custom", "" },
};

const QVector<Entry> &Entries(){
    static const QVector<Entry> entries(std::begin(TheEntries), std::end(TheEntries));
    return entries;
}

static const QVector<QRegularExpression> &Patterns(){
    static const QVector<QRegularExpression> patterns = []{
        QVector<QRegularExpression> list;
        for(const Entry &entry : Entries())
            list << QRegularExpression(QStringLiteral("\\A(?:%1)\\Z")
                                       .arg(QString::fromLatin1(entry.spelling)));
        return list;
    }();
    return patterns;
}

QString NameOf(const QString &value){
    if(value.isEmpty()) return QString();

    const QVector<Entry> &entries = Entries();
    const QVector<QRegularExpression> &patterns = Patterns();

    for(int i = 0; i < entries.length(); i++){
        if(patterns[i].match(value).hasMatch())
            return QString::fromLatin1(entries[i].name);
    }
    return QString();
}

const Entry *Find(const QString &name){
    for(const Entry &entry : Entries()){
        if(name == QString::fromLatin1(entry.name)) return &entry;
    }
    return nullptr;
}

QString Fallback(const QString &name){
    const Entry *entry = Find(name);
    return entry ? QString::fromLatin1(entry->fallback) : QString();
}

QString SettingsKey(const QString &name){
    return QStringLiteral("application/UserAgent_") + name;
}

QString Expand(QString tmpl, const QString &system,
               const QString &location, const QString &chromium){
    tmpl = tmpl.replace(QStringLiteral("%SYSTEM%"),   system)
               .replace(QStringLiteral("%LOCATION%"), location)
               .replace(QStringLiteral("%CHROMIUM%"), chromium);

    return QUrl::fromPercentEncoding(tmpl.toLatin1());
}

QString DefaultAcceptLanguage(const QStringList &uiLanguages){
    QString head = uiLanguages.isEmpty() ? QString() : uiLanguages.first();
    head.replace(QLatin1Char('_'), QLatin1Char('-'));

    if(head.isEmpty() || head == QLatin1String("C") || head == QLatin1String("en"))
        head = QStringLiteral("en-US");

    const QStringList subtags = head.split(QLatin1Char('-'));
    const QString language = subtags.first();
    head = language;
    for(int i = 1; i < subtags.size(); i++){
        const QString &subtag = subtags[i];
        if(subtag.size() == 2 || (subtag.size() == 3 && subtag[0].isDigit())){
            head += QLatin1Char('-') + subtag;
            break;
        }
    }

    QStringList tags;
    tags << head;
    if(language != head) tags << language;
    for(const QString &english : { QStringLiteral("en-US"), QStringLiteral("en") })
        if(!tags.contains(english)) tags << english;

    QStringList parts;
    for(int i = 0; i < tags.size(); i++){
        parts << (i == 0 ? tags[i]
                         : QStringLiteral("%1;q=%2").arg(tags[i]).arg(1.0 - 0.1 * i, 0, 'f', 1));
    }
    return parts.join(QLatin1Char(','));
}

Map Load(const SettingsIO::Map &settings){
    Map agents;
    for(const Entry &entry : Entries()){
        const QString name     = QString::fromLatin1(entry.name);
        const QString fallback = QString::fromLatin1(entry.fallback);

        QString value = settings.value(SettingsKey(name), fallback).toString();
        if(value.isEmpty()) value = fallback;

        agents[name] = value;
    }
    return agents;
}

void Save(SettingsIO::Map &settings, const Map &agents){
    for(const Entry &entry : Entries()){
        const QString name     = QString::fromLatin1(entry.name);
        const QString fallback = QString::fromLatin1(entry.fallback);
        const QString value    = agents.value(name);

        settings.insert(SettingsKey(name), value == fallback ? QString() : value);
    }
}

}
