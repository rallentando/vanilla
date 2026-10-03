#include "switch.hpp"

#include "dnrrules.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace Dnr {

namespace {

    const char *const TYPE_NAMES[ResourceTypeCount] = {
        "main_frame", "sub_frame", "stylesheet", "script", "image", "font",
        "object", "xmlhttprequest", "ping", "csp_report", "media",
        "websocket", "webtransport", "webbundle", "other"
    };

    const quint32 ALL_TYPES = (1u << ResourceTypeCount) - 1;
    const quint32 DEFAULT_TYPES = ALL_TYPES & ~(1u << MainFrame);

    bool IsSeparator(QChar c){
        const ushort u = c.unicode();
        if(u >= 'a' && u <= 'z') return false;
        if(u >= 'A' && u <= 'Z') return false;
        if(u >= '0' && u <= '9') return false;
        return u != '_' && u != '-' && u != '.' && u != '%';
    }

    bool IsTokenChar(QChar c){
        const ushort u = c.unicode();
        return (u >= 'a' && u <= 'z') || (u >= '0' && u <= '9');
    }

    bool IsCommonToken(const QString &token){
        static const QSet<QString> common = {
            QStringLiteral("http"), QStringLiteral("https"), QStringLiteral("www"),
            QStringLiteral("com"), QStringLiteral("net"), QStringLiteral("org"),
            QStringLiteral("js"), QStringLiteral("css"), QStringLiteral("html"),
            QStringLiteral("php"), QStringLiteral("png"), QStringLiteral("gif"),
            QStringLiteral("jpg"), QStringLiteral("min"), QStringLiteral("cdn"),
        };
        return common.contains(token);
    }

    bool PieceAt(const QString &url, int pos, const QString &piece, bool last, int *end){
        const int size = url.size();
        for(int i = 0; i < piece.size(); i++){
            const QChar want = piece.at(i);
            if(want == QLatin1Char('^')){
                if(pos == size){
                    if(last && i == piece.size() - 1) continue;
                    return false;
                }
                if(!IsSeparator(url.at(pos))) return false;
                pos++;
            } else {
                if(pos >= size || url.at(pos) != want) return false;
                pos++;
            }
        }
        *end = pos;
        return true;
    }

    bool RestMatches(const QString &url, int from, const QStringList &pieces,
                     int index, bool right){
        for(int i = index; i < pieces.size(); i++){
            const bool last = i == pieces.size() - 1;
            bool placed = false;
            for(int pos = from; pos <= url.size(); pos++){
                int end = 0;
                if(!PieceAt(url, pos, pieces.at(i), last, &end)) continue;
                if(last && right && end != url.size()) continue;
                from = end;
                placed = true;
                break;
            }
            if(!placed) return false;
        }
        return true;
    }

    QStringList StringList(const QJsonValue &value){
        QStringList list;
        foreach(const QJsonValue &each, value.toArray())
            if(each.isString()) list << each.toString().toLower();
        return list;
    }

    bool UnderAny(const QString &host, const QSet<QString> &domains){
        if(domains.isEmpty() || host.isEmpty()) return false;
        int from = 0;
        while(true){
            if(domains.contains(from ? host.mid(from) : host)) return true;
            const int dot = host.indexOf(QLatin1Char('.'), from);
            if(dot < 0) return false;
            from = dot + 1;
        }
    }

    const QSet<QString> &ReadConditions(){
        static const QSet<QString> keys = {
            QStringLiteral("urlFilter"), QStringLiteral("regexFilter"),
            QStringLiteral("isUrlFilterCaseSensitive"),
            QStringLiteral("resourceTypes"), QStringLiteral("excludedResourceTypes"),
            QStringLiteral("requestMethods"), QStringLiteral("excludedRequestMethods"),
            QStringLiteral("requestDomains"), QStringLiteral("excludedRequestDomains"),
            QStringLiteral("initiatorDomains"), QStringLiteral("excludedInitiatorDomains"),
            QStringLiteral("domains"), QStringLiteral("excludedDomains"),
            QStringLiteral("domainType"),
        };
        return keys;
    }

    QSet<QString> DomainSet(const QJsonValue &value){
        QSet<QString> set;
        foreach(const QJsonValue &each, value.toArray())
            if(each.isString()) set.insert(each.toString().toLower());
        return set;
    }

    const QSet<QString> &TwoLabelSuffixes(){
        static const QSet<QString> suffixes = {
            QStringLiteral("co.jp"), QStringLiteral("ne.jp"), QStringLiteral("or.jp"),
            QStringLiteral("ac.jp"), QStringLiteral("go.jp"), QStringLiteral("ed.jp"),
            QStringLiteral("lg.jp"), QStringLiteral("gr.jp"), QStringLiteral("ad.jp"),
            QStringLiteral("co.uk"), QStringLiteral("org.uk"), QStringLiteral("ac.uk"),
            QStringLiteral("gov.uk"), QStringLiteral("me.uk"), QStringLiteral("ltd.uk"),
            QStringLiteral("com.au"), QStringLiteral("net.au"), QStringLiteral("org.au"),
            QStringLiteral("edu.au"), QStringLiteral("gov.au"),
            QStringLiteral("co.nz"), QStringLiteral("org.nz"), QStringLiteral("net.nz"),
            QStringLiteral("co.kr"), QStringLiteral("or.kr"), QStringLiteral("go.kr"),
            QStringLiteral("com.cn"), QStringLiteral("net.cn"), QStringLiteral("org.cn"),
            QStringLiteral("gov.cn"), QStringLiteral("com.tw"), QStringLiteral("org.tw"),
            QStringLiteral("com.hk"), QStringLiteral("org.hk"), QStringLiteral("com.sg"),
            QStringLiteral("co.in"), QStringLiteral("net.in"), QStringLiteral("org.in"),
            QStringLiteral("co.id"), QStringLiteral("co.th"), QStringLiteral("com.my"),
            QStringLiteral("com.ph"), QStringLiteral("com.vn"),
            QStringLiteral("com.br"), QStringLiteral("net.br"), QStringLiteral("org.br"),
            QStringLiteral("com.mx"), QStringLiteral("com.ar"), QStringLiteral("com.co"),
            QStringLiteral("com.tr"), QStringLiteral("co.za"), QStringLiteral("co.il"),
            QStringLiteral("com.ua"), QStringLiteral("com.pl"), QStringLiteral("com.ru"),
            QStringLiteral("com.sa"), QStringLiteral("com.eg"), QStringLiteral("com.pk"),
        };
        return suffixes;
    }
}

static int ResourceTypeFromName(const QString &name){
    for(int i = 0; i < ResourceTypeCount; i++)
        if(name == QLatin1String(TYPE_NAMES[i])) return i;
    return -1;
}

QString RegistrableDomain(const QString &host){
    if(host.isEmpty() || host.contains(QLatin1Char(':'))) return host;
    const QStringList labels = host.split(QLatin1Char('.'));
    if(labels.size() <= 2) return host;
    bool numeric = true;
    foreach(const QString &label, labels){
        bool ok = false;
        label.toUInt(&ok);
        if(!ok){ numeric = false; break; }
    }
    if(numeric) return host;

    const QString lastTwo = labels.at(labels.size() - 2) + QLatin1Char('.') + labels.last();
    const int keep = TwoLabelSuffixes().contains(lastTwo) ? 3 : 2;
    return QStringList(labels.mid(labels.size() - keep)).join(QLatin1Char('.'));
}

UrlFilter::UrlFilter(const QString &filter, bool caseSensitive)
    : m_CaseSensitive(caseSensitive)
{
    QString text = caseSensitive ? filter : filter.toLower();

    if(text.startsWith(QStringLiteral("||"))){
        m_Left = AtLabel;
        text = text.mid(2);
    } else if(text.startsWith(QLatin1Char('|'))){
        m_Left = AtStart;
        text = text.mid(1);
    }
    if(text.endsWith(QLatin1Char('|'))){
        m_Right = true;
        text.chop(1);
    }
    if(text.startsWith(QLatin1Char('*'))) m_Left = Anywhere;
    if(text.endsWith(QLatin1Char('*'))) m_Right = false;

    m_Pieces = text.split(QLatin1Char('*'), Qt::SkipEmptyParts);

    QString best, fallback;
    for(int p = 0; p < m_Pieces.size(); p++){
        const QString piece = m_Pieces.at(p).toLower();
        const bool tiedLeft = p == 0 && m_Left != Anywhere;
        const bool tiedRight = p == m_Pieces.size() - 1 && m_Right;
        int i = 0;
        while(i < piece.size()){
            if(!IsTokenChar(piece.at(i))){ i++; continue; }
            const int begin = i;
            while(i < piece.size() && IsTokenChar(piece.at(i))) i++;
            const bool closedLeft = begin > 0 || tiedLeft;
            const bool closedRight = i < piece.size() || tiedRight;
            if(!closedLeft || !closedRight) continue;
            const QString token = piece.mid(begin, i - begin);
            QString &slot = IsCommonToken(token) ? fallback : best;
            if(token.size() > slot.size()) slot = token;
        }
    }
    m_IndexToken = best.isEmpty() ? fallback : best;
}

bool UrlFilter::Matches(const QString &url, const QString &lower,
                        int hostStart, int hostEnd) const {
    const QString &text = m_CaseSensitive ? url : lower;
    if(m_Pieces.isEmpty()) return !(m_Left == AtStart && m_Right) || text.isEmpty();

    if(m_Left == Anywhere) return RestMatches(text, 0, m_Pieces, 0, m_Right);

    const bool only = m_Pieces.size() == 1;
    if(m_Left == AtStart){
        int end = 0;
        if(!PieceAt(text, 0, m_Pieces.first(), only, &end)) return false;
        if(only) return !m_Right || end == text.size();
        return RestMatches(text, end, m_Pieces, 1, m_Right);
    }

    for(int pos = hostStart; pos < hostEnd; pos++){
        if(pos != hostStart && text.at(pos - 1) != QLatin1Char('.')) continue;
        int end = 0;
        if(!PieceAt(text, pos, m_Pieces.first(), only, &end)) continue;
        if(only){
            if(!m_Right || end == text.size()) return true;
            continue;
        }
        if(RestMatches(text, end, m_Pieces, 1, m_Right)) return true;
    }
    return false;
}

struct Rules::Asked {
    QString url;
    QString lower;
    int hostStart = 0;
    int hostEnd = 0;
    QString host;
    QString initiatorHost;
    bool thirdParty = true;
    quint32 typeBit = 0;
    QByteArray method;
    bool http = true;
};

void Rules::AddRuleset(const QString &extensionId, const QByteArray &json, Skipped *skipped){
    Skipped none;
    Skipped &left = skipped ? *skipped : none;

    const QJsonDocument document = QJsonDocument::fromJson(json);
    if(!document.isArray()){
        left.malformed++;
        return;
    }

    int extension = m_Extensions.indexOf(extensionId);
    if(extension < 0){
        extension = m_Extensions.size();
        m_Extensions << extensionId;
    }

    foreach(const QJsonValue &value, document.array()){
        const QJsonObject object = value.toObject();
        const QJsonObject action = object[QStringLiteral("action")].toObject();
        const QJsonObject condition = object[QStringLiteral("condition")].toObject();
        const QString type = action[QStringLiteral("type")].toString();

        Rule rule;
        rule.extension = extension;

        if(type == QStringLiteral("block")) rule.action = Block;
        else if(type == QStringLiteral("allow")) rule.action = Allow;
        else if(type == QStringLiteral("allowAllRequests")) rule.action = AllowAllRequests;
        else if(type == QStringLiteral("upgradeScheme")) rule.action = UpgradeScheme;
        else if(type == QStringLiteral("redirect")){
            rule.action = RedirectTo;
            const QJsonObject redirect = action[QStringLiteral("redirect")].toObject();
            if(redirect.contains(QStringLiteral("url"))){
                rule.redirect = QUrl(redirect[QStringLiteral("url")].toString());
            } else if(redirect.contains(QStringLiteral("extensionPath"))){
                const QString path = redirect[QStringLiteral("extensionPath")].toString();
                if(!path.startsWith(QLatin1Char('/'))){
                    left.malformed++;
                    continue;
                }
                rule.redirect = QUrl(QStringLiteral("chrome-extension://") + extensionId + path);
            } else {
                left.computedRedirect++;
                continue;
            }
            if(!rule.redirect.isValid() || rule.redirect.isEmpty() ||
               rule.redirect.scheme() == QStringLiteral("javascript")){
                left.malformed++;
                continue;
            }
        } else if(type == QStringLiteral("modifyHeaders")){
            left.modifyHeaders++;
            continue;
        } else {
            left.malformed++;
            continue;
        }

        if(condition.contains(QStringLiteral("tabIds")) ||
           condition.contains(QStringLiteral("excludedTabIds"))){
            left.tabBound++;
            continue;
        }
        bool unread = false;
        foreach(const QString &key, condition.keys())
            if(!ReadConditions().contains(key)) unread = true;
        if(unread){
            left.unreadCondition++;
            continue;
        }
        if(object.contains(QStringLiteral("priority")) &&
           object[QStringLiteral("priority")].toInt(0) < 1){
            left.malformed++;
            continue;
        }
        if(UrlFilter::IsRefused(condition[QStringLiteral("urlFilter")].toString())){
            left.malformed++;
            continue;
        }

        static const int ACTION_RANK[] = { 5, 4, 3, 2, 1 };
        const int priority = object[QStringLiteral("priority")].toInt(1);
        rule.rank = (static_cast<quint64>(priority) << 8) | ACTION_RANK[rule.action];

        quint32 included = 0, excluded = 0;
        foreach(const QString &name, StringList(condition[QStringLiteral("resourceTypes")])){
            const int index = ResourceTypeFromName(name);
            if(index >= 0) included |= 1u << index;
        }
        foreach(const QString &name, StringList(condition[QStringLiteral("excludedResourceTypes")])){
            const int index = ResourceTypeFromName(name);
            if(index >= 0) excluded |= 1u << index;
        }
        if(included) rule.types = included;
        else if(excluded) rule.types = ALL_TYPES & ~excluded;
        else rule.types = DEFAULT_TYPES;

        foreach(const QString &method, StringList(condition[QStringLiteral("requestMethods")]))
            rule.methods << method.toLatin1();
        foreach(const QString &method, StringList(condition[QStringLiteral("excludedRequestMethods")]))
            rule.excludedMethods << method.toLatin1();

        rule.requestDomains = DomainSet(condition[QStringLiteral("requestDomains")]);
        rule.excludedRequestDomains = DomainSet(condition[QStringLiteral("excludedRequestDomains")]);
        rule.initiatorDomains = DomainSet(condition[QStringLiteral("initiatorDomains")]);
        rule.excludedInitiatorDomains = DomainSet(condition[QStringLiteral("excludedInitiatorDomains")]);
        if(!condition.contains(QStringLiteral("initiatorDomains")))
            rule.initiatorDomains = DomainSet(condition[QStringLiteral("domains")]);
        if(!condition.contains(QStringLiteral("excludedInitiatorDomains")))
            rule.excludedInitiatorDomains = DomainSet(condition[QStringLiteral("excludedDomains")]);

        const QString party = condition[QStringLiteral("domainType")].toString();
        if(party == QStringLiteral("firstParty")) rule.domainType = FirstParty;
        else if(party == QStringLiteral("thirdParty")) rule.domainType = ThirdParty;

        const bool caseSensitive =
            condition[QStringLiteral("isUrlFilterCaseSensitive")].toBool(false);

        if(condition.contains(QStringLiteral("regexFilter"))){
            rule.hasRegex = true;
            const QString pattern = condition[QStringLiteral("regexFilter")].toString();
            if(pattern.size() > 512){
                left.badRegex++;
                continue;
            }
            rule.regex = QRegularExpression
                (pattern,
                 caseSensitive ? QRegularExpression::NoPatternOption
                               : QRegularExpression::CaseInsensitiveOption);
            if(!rule.regex.isValid()){
                left.badRegex++;
                continue;
            }
            rule.regex.optimize();
        } else if(condition.contains(QStringLiteral("urlFilter"))){
            rule.hasFilter = true;
            rule.filter = UrlFilter(condition[QStringLiteral("urlFilter")].toString(), caseSensitive);
        }

        const int index = m_Rules.size();
        m_Rules << rule;
        const QString token = rule.hasFilter ? rule.filter.IndexToken() : QString();
        if(token.isEmpty()) m_Unindexed << index;
        else m_ByToken[token] << index;
    }
}

bool Rules::RuleMatches(const Rule &rule, const Asked &asked) const {
    if(!(rule.types & asked.typeBit)) return false;
    if(asked.http){
        if(!rule.methods.isEmpty() && !rule.methods.contains(asked.method)) return false;
        if(rule.excludedMethods.contains(asked.method)) return false;
    } else if(!rule.methods.isEmpty()) return false;

    if(rule.domainType == ThirdParty && !asked.thirdParty) return false;
    if(rule.domainType == FirstParty && asked.thirdParty) return false;

    if(!rule.requestDomains.isEmpty() && !UnderAny(asked.host, rule.requestDomains)) return false;
    if(UnderAny(asked.host, rule.excludedRequestDomains)) return false;
    if(!rule.initiatorDomains.isEmpty() &&
       (asked.initiatorHost.isEmpty() || !UnderAny(asked.initiatorHost, rule.initiatorDomains)))
        return false;
    if(!asked.initiatorHost.isEmpty() &&
       UnderAny(asked.initiatorHost, rule.excludedInitiatorDomains)) return false;

    if(rule.hasRegex) return rule.regex.match(asked.url).hasMatch();
    if(rule.hasFilter) return rule.filter.Matches(asked.url, asked.lower, asked.hostStart, asked.hostEnd);
    return true;
}

Decision Rules::Evaluate(const Request &request) const {
    Decision decision;
    if(m_Rules.isEmpty()) return decision;

    const int type = request.type;
    if(type < 0 || type >= ResourceTypeCount) return decision;

    const QString scheme = request.url.scheme();
    const bool http = scheme == QStringLiteral("http") || scheme == QStringLiteral("https");
    if(!http && scheme != QStringLiteral("ws") && scheme != QStringLiteral("wss") &&
       scheme != QStringLiteral("ftp")) return decision;
    if(type != MainFrame && request.initiator.scheme() == QStringLiteral("chrome-extension"))
        return decision;

    Asked asked;
    asked.http = http;
    asked.url = QString::fromLatin1(request.url.toEncoded());
    asked.lower = asked.url.toLower();
    asked.host = request.url.host(QUrl::FullyEncoded).toLower();
    asked.initiatorHost = request.initiator.host(QUrl::FullyEncoded).toLower();
    asked.typeBit = 1u << type;
    asked.method = request.method.toLower();
    asked.thirdParty = asked.initiatorHost.isEmpty()
        || RegistrableDomain(asked.host) != RegistrableDomain(asked.initiatorHost);

    const int colon = asked.lower.indexOf(QStringLiteral("://"));
    int start = colon < 0 ? 0 : colon + 3;
    int authorityEnd = asked.lower.size();
    for(int i = start; i < asked.lower.size(); i++){
        const QChar c = asked.lower.at(i);
        if(c == QLatin1Char('/') || c == QLatin1Char('?') || c == QLatin1Char('#')){
            authorityEnd = i;
            break;
        }
    }
    const int at = asked.lower.lastIndexOf(QLatin1Char('@'), authorityEnd - 1);
    if(at >= start) start = at + 1;
    int end = authorityEnd;
    if(start < authorityEnd && asked.lower.at(start) == QLatin1Char('[')){
        const int close = asked.lower.indexOf(QLatin1Char(']'), start);
        if(close >= 0 && close < authorityEnd) end = close + 1;
    } else {
        const int port = asked.lower.indexOf(QLatin1Char(':'), start);
        if(port >= 0 && port < authorityEnd) end = port;
    }
    asked.hostStart = start;
    asked.hostEnd = end;

    QVector<int> best(m_Extensions.size(), -1);
    const bool document = type == MainFrame || type == SubFrame;
    QVector<quint64> allowAll(m_Extensions.size(), 0);
    QVector<quint64> under(m_Extensions.size(), 0);
    if(!request.frameAllows.isEmpty())
        for(int i = 0; i < m_Extensions.size(); i++) under[i] = request.frameAllows.value(m_Extensions.at(i), 0);
    auto consider = [&](int index){
        const Rule &rule = m_Rules.at(index);
        const bool shielded = under[rule.extension] && rule.rank <= under[rule.extension] &&
            (rule.action == Block || rule.action == RedirectTo || rule.action == UpgradeScheme);
        const bool aimed = document && rule.action == AllowAllRequests && rule.rank > allowAll[rule.extension];
        int &held = best[rule.extension];
        bool contends = !shielded;
        if(contends && held >= 0){
            const quint64 rank = m_Rules.at(held).rank;
            if(rank > rule.rank || (rank == rule.rank && held < index)) contends = false;
        }
        if(!contends && !aimed) return;
        if(!RuleMatches(rule, asked)) return;
        if(aimed) allowAll[rule.extension] = rule.rank;
        if(contends) held = index;
    };

    if(!m_ByToken.isEmpty()){
        QSet<QString> seen;
        const QString &text = asked.lower;
        int i = 0;
        while(i < text.size()){
            if(!IsTokenChar(text.at(i))){ i++; continue; }
            const int begin = i;
            while(i < text.size() && IsTokenChar(text.at(i))) i++;
            const QString token = text.mid(begin, i - begin);
            if(seen.contains(token)) continue;
            seen.insert(token);
            const auto found = m_ByToken.constFind(token);
            if(found == m_ByToken.constEnd()) continue;
            foreach(int index, found.value()) consider(index);
        }
    }
    foreach(int index, m_Unindexed) consider(index);

    if(document)
        for(int i = 0; i < m_Extensions.size(); i++)
            if(allowAll.at(i)) decision.frameAllows.insert(m_Extensions.at(i), allowAll.at(i));

    foreach(int index, best){
        if(index < 0) continue;
        const Rule *rule = &m_Rules.at(index);
        if(rule->action == Block){
            decision.kind = Decision::Block;
            decision.redirect = QUrl();
            return decision;
        }
        if(decision.kind != Decision::None) continue;
        if(rule->action == RedirectTo){
            decision.kind = Decision::Redirect;
            decision.redirect = rule->redirect;
        } else if(rule->action == UpgradeScheme &&
                  request.url.scheme() == QStringLiteral("http")){
            decision.kind = Decision::Redirect;
            decision.redirect = request.url;
            decision.redirect.setScheme(QStringLiteral("https"));
        }
    }
    if(decision.kind == Decision::Redirect &&
       (decision.redirect == request.url || type == WebSocket)){
        Decision none;
        none.frameAllows = decision.frameAllows;
        return none;
    }
    return decision;
}

QString Framed::KeyOf(const QUrl &url){
    if(url.isEmpty() || !url.isValid()) return QString();
    return QString::fromLatin1(url.adjusted(QUrl::RemoveFragment).toEncoded());
}

bool Framed::Find(quint64 generation, const QString &key, QHash<QString, quint64> *allows){
    if(generation != m_Generation) return false;
    const auto found = m_Held.constFind(key);
    if(found == m_Held.constEnd()) return false;
    if(allows) *allows = found.value();
    if(m_Order.isEmpty() || m_Order.last() != key){
        m_Order.removeOne(key);
        m_Order.append(key);
    }
    return true;
}

void Framed::Put(quint64 generation, const QString &key, const QHash<QString, quint64> &allows){
    if(generation != m_Generation){
        m_Held.clear();
        m_Order.clear();
        m_Generation = generation;
    }
    if(key.isEmpty()) return;
    if(m_Held.contains(key)) m_Order.removeOne(key);
    else while(m_Held.size() >= m_Kept && !m_Order.isEmpty()) m_Held.remove(m_Order.takeFirst());
    m_Held.insert(key, allows);
    m_Order.append(key);
}

namespace {
    bool RuleIdOf(const QJsonValue &value, int *id){
        if(!value.isDouble()) return false;
        const double number = value.toDouble();
        if(number < 1 || number > 2147483647.0 || number != static_cast<double>(static_cast<qint64>(number))) return false;
        *id = static_cast<int>(number);
        return true;
    }
    bool IsRegexRule(const QJsonValue &rule){
        return rule.toObject()[QStringLiteral("condition")].toObject().contains(QStringLiteral("regexFilter"));
    }
    int RegexCount(const QJsonArray &rules){
        int count = 0;
        for(const QJsonValue &rule : rules) if(IsRegexRule(rule)) count++;
        return count;
    }
}

QString Held::Update(const QString &extension, Kind kind, const QJsonValue &options,
                     const QString &version){
    if(!options.isObject()) return QStringLiteral("Invalid arguments: the update takes an object.");
    const QJsonObject asked = options.toObject();
    const QJsonValue removing = asked[QStringLiteral("removeRuleIds")];
    const QJsonValue adding = asked[QStringLiteral("addRules")];
    if(!removing.isUndefined() && !removing.isArray()) return QStringLiteral("Invalid arguments: 'removeRuleIds' must be a list.");
    if(!adding.isUndefined() && !adding.isArray()) return QStringLiteral("Invalid arguments: 'addRules' must be a list.");

    Entry entry = m_Entries.value(extension);
    const QJsonArray before = kind == Dynamic ? entry.dynamic : SessionOf(entry, version);

    QSet<int> removed;
    for(const QJsonValue &value : removing.toArray()){
        int id = 0;
        if(!RuleIdOf(value, &id)) return QStringLiteral("Invalid arguments: 'removeRuleIds' holds something which is no rule id.");
        removed.insert(id);
    }

    QJsonArray after;
    QSet<int> ids;
    for(const QJsonValue &rule : before){
        int id = 0;
        RuleIdOf(rule.toObject()[QStringLiteral("id")], &id);
        if(removed.contains(id)) continue;
        ids.insert(id);
        after.append(rule);
    }
    static const QSet<QString> actions = {
        QStringLiteral("block"), QStringLiteral("allow"), QStringLiteral("allowAllRequests"),
        QStringLiteral("upgradeScheme"), QStringLiteral("redirect"), QStringLiteral("modifyHeaders") };
    const QJsonArray added = adding.toArray();
    for(int i = 0; i < added.size(); i++){
        const QJsonObject rule = added.at(i).toObject();
        int id = 0;
        if(!added.at(i).isObject() || !RuleIdOf(rule[QStringLiteral("id")], &id))
            return QStringLiteral("Rule at index %1: the id must be a whole number from 1 up.").arg(i);
        if(ids.contains(id))
            return QStringLiteral("Rule with id %1 does not have a unique ID.").arg(id);
        if(!actions.contains(rule[QStringLiteral("action")].toObject()[QStringLiteral("type")].toString()))
            return QStringLiteral("Rule with id %1: the action is not one of Chrome's.").arg(id);
        if(!rule[QStringLiteral("condition")].isObject())
            return QStringLiteral("Rule with id %1: the condition must be an object.").arg(id);
        ids.insert(id);
        after.append(rule);
    }

    if(after.size() > (kind == Dynamic ? DYNAMIC_LIMIT : SESSION_LIMIT))
        return kind == Dynamic ? QStringLiteral("Dynamic rule count exceeded.")
                               : QStringLiteral("Session rule count exceeded.");
    const QJsonArray other = kind == Dynamic ? SessionOf(entry, version) : entry.dynamic;
    if(RegexCount(after) + RegexCount(other) > REGEX_LIMIT)
        return QStringLiteral("Regular expression rule count exceeded.");
    if(QJsonDocument(after).toJson(QJsonDocument::Compact).size() > BYTES_LIMIT)
        return QStringLiteral("The rules are too large.");

    if(kind == Dynamic) entry.dynamic = after;
    else {
        entry.session = after;
        entry.sessionVersion = version;
    }
    if(entry.IsEmpty()) m_Entries.remove(extension);
    else m_Entries.insert(extension, entry);
    return QString();
}

QJsonArray Held::Get(const QString &extension, Kind kind, const QJsonValue &filter,
                     const QString &version) const {
    const Entry entry = m_Entries.value(extension);
    const QJsonArray all = kind == Dynamic ? entry.dynamic : SessionOf(entry, version);
    const QJsonValue wanted = filter.toObject()[QStringLiteral("ruleIds")];
    if(!wanted.isArray()) return all;
    QSet<int> ids;
    for(const QJsonValue &value : wanted.toArray()){
        int id = 0;
        if(RuleIdOf(value, &id)) ids.insert(id);
    }
    QJsonArray some;
    for(const QJsonValue &rule : all){
        int id = 0;
        if(RuleIdOf(rule.toObject()[QStringLiteral("id")], &id) && ids.contains(id)) some.append(rule);
    }
    return some;
}

QStringList Held::Enabled(const QString &extension, const QList<Ruleset> &manifest,
                          const QString &version) const {
    const Entry entry = m_Entries.value(extension);
    const bool chosen = entry.chose && entry.enabledVersion == version;
    QStringList ids;
    for(const Ruleset &ruleset : manifest)
        if(chosen ? entry.enabled.contains(ruleset.id) : ruleset.enabled) ids << ruleset.id;
    return ids;
}

QString Held::UpdateEnabled(const QString &extension, const QJsonValue &options,
                            const QList<Ruleset> &manifest, const QString &version){
    if(!options.isObject()) return QStringLiteral("Invalid arguments: the update takes an object.");
    const QJsonObject asked = options.toObject();
    QSet<QString> known;
    for(const Ruleset &ruleset : manifest) known.insert(ruleset.id);
    QStringList lists[2];
    const QString names[2] = { QStringLiteral("disableRulesetIds"), QStringLiteral("enableRulesetIds") };
    for(int i = 0; i < 2; i++){
        const QJsonValue value = asked[names[i]];
        if(value.isUndefined()) continue;
        if(!value.isArray()) return QStringLiteral("Invalid arguments: '%1' must be a list.").arg(names[i]);
        for(const QJsonValue &id : value.toArray()){
            if(!id.isString() || !known.contains(id.toString()))
                return QStringLiteral("Invalid ruleset id: %1.").arg(id.isString() ? id.toString() : QStringLiteral("?"));
            lists[i] << id.toString();
        }
    }
    QStringList now = Enabled(extension, manifest, version);
    for(const QString &id : lists[0]) now.removeAll(id);
    for(const QString &id : lists[1]) if(!now.contains(id)) now << id;
    if(now.size() > ENABLED_LIMIT) return QStringLiteral("The number of enabled static rulesets exceeds the limit.");

    Entry entry = m_Entries.value(extension);
    entry.chose = true;
    entry.enabledVersion = version;
    entry.enabled.clear();
    for(const Ruleset &ruleset : manifest) if(now.contains(ruleset.id)) entry.enabled << ruleset.id;
    m_Entries.insert(extension, entry);
    return QString();
}

bool Held::Settle(const QString &extension, const QString &version){
    auto i = m_Entries.find(extension);
    if(i == m_Entries.end()) return false;
    bool went = false;
    if(i.value().chose && i.value().enabledVersion != version){
        i.value().chose = false;
        i.value().enabled.clear();
        i.value().enabledVersion.clear();
        went = true;
    }
    if(!i.value().session.isEmpty() && i.value().sessionVersion != version){
        i.value().session = QJsonArray();
        i.value().sessionVersion.clear();
        went = true;
    }
    if(i.value().IsEmpty()) m_Entries.erase(i);
    return went;
}

void Held::Forget(const QString &extension){
    m_Entries.remove(extension);
}

void Held::KeepSessionsOf(const QSet<QString> &extensions){
    for(auto i = m_Entries.begin(); i != m_Entries.end();){
        if(!extensions.contains(i.key())){
            i.value().session = QJsonArray();
            i.value().sessionVersion.clear();
        }
        if(i.value().IsEmpty()) i = m_Entries.erase(i);
        else ++i;
    }
}

QJsonObject Held::ToJson() const {
    QJsonObject all;
    for(auto i = m_Entries.constBegin(); i != m_Entries.constEnd(); ++i){
        QJsonObject one;
        if(!i.value().dynamic.isEmpty()) one[QStringLiteral("dynamic")] = i.value().dynamic;
        if(i.value().chose){
            one[QStringLiteral("enabled")] = QJsonArray::fromStringList(i.value().enabled);
            one[QStringLiteral("enabledVersion")] = i.value().enabledVersion;
        }
        if(!one.isEmpty()) all[i.key()] = one;
    }
    return all;
}

Held Held::FromJson(const QJsonObject &object, bool *damaged){
    Held held;
    bool bad = false;
    for(auto i = object.constBegin(); i != object.constEnd(); ++i){
        if(!i.value().isObject()){ bad = true; continue; }
        const QJsonObject one = i.value().toObject();
        Entry entry;
        const QJsonValue dynamic = one[QStringLiteral("dynamic")];
        if(dynamic.isArray()) entry.dynamic = dynamic.toArray();
        else if(!dynamic.isUndefined()) bad = true;
        const QJsonValue enabled = one[QStringLiteral("enabled")];
        if(enabled.isArray()){
            entry.chose = true;
            for(const QJsonValue &id : enabled.toArray()){
                if(id.isString()) entry.enabled << id.toString();
                else bad = true;
            }
            entry.enabledVersion = one[QStringLiteral("enabledVersion")].toString();
        } else if(!enabled.isUndefined()) bad = true;
        if(!entry.IsEmpty()) held.m_Entries.insert(i.key(), entry);
    }
    if(damaged) *damaged = bad;
    return held;
}

}
