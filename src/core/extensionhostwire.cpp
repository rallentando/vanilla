#include "switch.hpp"

#include "extensionhostwire.hpp"
#include "extensionmainscripts.hpp"

#include <algorithm>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMessageAuthenticationCode>
#include <QRegularExpression>

namespace ExtensionHostWire {

namespace {

QJsonObject Failure(const QString &error){
    QJsonObject reply;
    reply[QStringLiteral("ok")] = false;
    reply[QStringLiteral("error")] = error;
    return reply;
}

QJsonObject Success(const QJsonValue &value){
    QJsonObject reply;
    reply[QStringLiteral("ok")] = true;
    reply[QStringLiteral("value")] = value;
    return reply;
}

const int WINDOW = 1;
const int WINDOW_ID_CURRENT = -2;
const int WINDOW_ID_NONE = -1;

QString StatusOf(const Tab &tab){
    return tab.discarded ? QStringLiteral("unloaded")
         : tab.loading ? QStringLiteral("loading") : QStringLiteral("complete");
}

QJsonObject Describe(const Tab &tab, int index, const Sight &sight){
    QJsonObject out;
    out[QStringLiteral("id")] = tab.id;
    out[QStringLiteral("index")] = index;
    out[QStringLiteral("windowId")] = WINDOW;
    out[QStringLiteral("active")] = tab.active;
    out[QStringLiteral("highlighted")] = tab.active;
    out[QStringLiteral("pinned")] = false;
    out[QStringLiteral("incognito")] = false;
    out[QStringLiteral("discarded")] = tab.discarded;
    out[QStringLiteral("autoDiscardable")] = true;
    out[QStringLiteral("audible")] = tab.audible;
    QJsonObject muted;
    muted[QStringLiteral("muted")] = tab.muted;
    out[QStringLiteral("mutedInfo")] = muted;
    out[QStringLiteral("groupId")] = -1;
    out[QStringLiteral("status")] = StatusOf(tab);
    if(sight.Sees(tab.url)){
        out[QStringLiteral("url")] = tab.url.toString(QUrl::FullyEncoded);
        out[QStringLiteral("title")] = tab.title;
    }
    return out;
}

qint64 Whole(const QJsonValue &value, bool *ok){
    const double number = value.toDouble();
    *ok = value.isDouble() && number > -9007199254740992.0 && number < 9007199254740992.0 &&
          number == static_cast<double>(static_cast<qint64>(number));
    return *ok ? static_cast<qint64>(number) : 0;
}

bool Glob(const QString &pattern, const QString &text){
    QString expression;
    foreach(const QString &piece, pattern.split(QLatin1Char('*')))
        expression += QRegularExpression::escape(piece) + QStringLiteral(".*");
    expression.chop(2);
    return QRegularExpression(QRegularExpression::anchoredPattern(expression),
                              QRegularExpression::DotMatchesEverythingOption).match(text).hasMatch();
}

const QStringList ALL_URLS = QStringList()
    << QStringLiteral("http") << QStringLiteral("https") << QStringLiteral("file") << QStringLiteral("ftp");

QJsonObject Query(const QJsonArray &args, const QList<Tab> &tabs, const Sight &sight){
    if(args.size() != 1 || !args.at(0).isObject())
        return Failure(QStringLiteral("tabs.query takes one object"));
    const QJsonObject filter = args.at(0).toObject();

    QStringList patterns;
    for(QJsonObject::const_iterator it = filter.constBegin(); it != filter.constEnd(); ++it){
        const QString key = it.key();
        const QJsonValue value = it.value();
        static const QStringList flags = QStringList()
            << QStringLiteral("active") << QStringLiteral("highlighted") << QStringLiteral("currentWindow")
            << QStringLiteral("lastFocusedWindow") << QStringLiteral("pinned") << QStringLiteral("audible")
            << QStringLiteral("muted") << QStringLiteral("discarded") << QStringLiteral("autoDiscardable");
        bool ok = true;
        if(flags.contains(key)) ok = value.isBool();
        else if(key == QStringLiteral("windowId") || key == QStringLiteral("index") || key == QStringLiteral("groupId")) Whole(value, &ok);
        else if(key == QStringLiteral("status"))
            ok = value.isString() && (QStringList() << QStringLiteral("loading") << QStringLiteral("complete") << QStringLiteral("unloaded")).contains(value.toString());
        else if(key == QStringLiteral("windowType"))
            ok = value.isString() && (QStringList() << QStringLiteral("normal") << QStringLiteral("popup") << QStringLiteral("panel")
                                                    << QStringLiteral("app") << QStringLiteral("devtools")).contains(value.toString());
        else if(key == QStringLiteral("url")){
            if(!sight.Any()) return Failure(QStringLiteral("tabs.query: 'url' needs the \"tabs\" permission"));
            const QJsonArray list = value.isArray() ? value.toArray() : QJsonArray() << value;
            foreach(const QJsonValue &one, list){
                bool readable = false;
                if(one.isString()) Matches(one.toString(), QUrl(), &readable);
                if(!readable) return Failure(QStringLiteral("tabs.query: not a match pattern: %1").arg(one.toVariant().toString()));
                patterns << one.toString();
            }
            ok = !list.isEmpty();
        }
        else return Failure(QStringLiteral("tabs.query: '%1' is not supported by this browser").arg(key));
        if(!ok) return Failure(QStringLiteral("tabs.query: '%1' has a value of the wrong kind").arg(key));
    }

    auto says = [&](const char *key, bool actual){
        const QJsonValue value = filter.value(QLatin1String(key));
        return value.isUndefined() || value.toBool() == actual;
    };
    auto isNumber = [&](const char *key, qint64 actual, qint64 alias = -1){
        const QJsonValue value = filter.value(QLatin1String(key));
        if(value.isUndefined()) return true;
        bool ok = false;
        const qint64 asked = Whole(value, &ok);
        return asked == actual || (alias != -1 && asked == alias);
    };
    const bool window = says("currentWindow", true) && says("lastFocusedWindow", true)
                     && isNumber("windowId", WINDOW, WINDOW_ID_CURRENT)
                     && (filter.value(QStringLiteral("windowType")).isUndefined()
                         || filter.value(QStringLiteral("windowType")).toString() == QStringLiteral("normal"));

    QJsonArray found;
    for(int index = 0; window && index < tabs.size(); index++){
        const Tab &tab = tabs.at(index);
        if(!says("active", tab.active) || !says("highlighted", tab.active)) continue;
        if(!says("pinned", false) || !says("autoDiscardable", true)) continue;
        if(!says("audible", tab.audible) || !says("muted", tab.muted) || !says("discarded", tab.discarded)) continue;
        if(!isNumber("index", index) || !isNumber("groupId", -1)) continue;
        if(filter.contains(QStringLiteral("status")) && filter.value(QStringLiteral("status")).toString() != StatusOf(tab)) continue;
        if(!patterns.isEmpty()){
            if(!sight.Sees(tab.url)) continue;
            bool any = false, ok = false;
            foreach(const QString &pattern, patterns) any = any || Matches(pattern, tab.url, &ok);
            if(!any) continue;
        }
        found.append(Describe(tab, index, sight));
    }
    return Success(found);
}

QJsonObject Get(const QJsonArray &args, const QList<Tab> &tabs, const Sight &sight){
    bool ok = false;
    const qint64 id = args.size() == 1 ? Whole(args.at(0), &ok) : 0;
    if(!ok) return Failure(QStringLiteral("tabs.get takes the id of a tab"));
    for(int index = 0; index < tabs.size(); index++)
        if(tabs.at(index).id == id) return Success(Describe(tabs.at(index), index, sight));
    return Failure(NoTab(id));
}

QJsonObject DescribeWindow(const Window &window, const QList<Tab> &tabs, const Sight &sight, bool populate){
    QJsonObject out;
    out[QStringLiteral("id")] = WINDOW;
    out[QStringLiteral("focused")] = window.focused;
    out[QStringLiteral("left")] = window.left;
    out[QStringLiteral("top")] = window.top;
    out[QStringLiteral("width")] = window.width;
    out[QStringLiteral("height")] = window.height;
    out[QStringLiteral("incognito")] = false;
    out[QStringLiteral("type")] = QStringLiteral("normal");
    out[QStringLiteral("state")] = window.state;
    out[QStringLiteral("alwaysOnTop")] = false;
    if(populate){
        QJsonArray all;
        for(int index = 0; index < tabs.size(); index++) all.append(Describe(tabs.at(index), index, sight));
        out[QStringLiteral("tabs")] = all;
    }
    return out;
}

QJsonObject Windows(const QString &api, const QJsonArray &args, const QList<Tab> &tabs, const Sight &sight, const Window *window){
    const bool all = api == QStringLiteral("windows.getAll");
    if(args.size() > 1 || (args.size() == 1 && !args.at(0).isObject() && !args.at(0).isNull()))
        return Failure(QStringLiteral("%1 takes one object, or nothing").arg(api));
    const QJsonObject options = args.size() == 1 ? args.at(0).toObject() : QJsonObject();
    bool populate = false, normal = true;
    for(QJsonObject::const_iterator it = options.constBegin(); it != options.constEnd(); ++it){
        const QJsonValue value = it.value();
        if(value.isNull()) continue;
        if(it.key() == QStringLiteral("populate") && value.isBool()) populate = value.toBool();
        else if(it.key() == QStringLiteral("windowTypes") && value.isArray()){
            normal = false;
            foreach(const QJsonValue &type, value.toArray()){
                if(!type.isString()) return Failure(QStringLiteral("%1: 'windowTypes' has a value of the wrong kind").arg(api));
                normal = normal || type.toString() == QStringLiteral("normal");
            }
        }
        else if(it.key() == QStringLiteral("populate") || it.key() == QStringLiteral("windowTypes"))
            return Failure(QStringLiteral("%1: '%2' has a value of the wrong kind").arg(api, it.key()));
        else return Failure(QStringLiteral("%1: '%2' is not supported by this browser").arg(api, it.key()));
    }
    const bool there = window && normal;
    if(all) return Success(there ? QJsonArray() << DescribeWindow(*window, tabs, sight, populate) : QJsonArray());
    return there ? Success(DescribeWindow(*window, tabs, sight, populate)) : Failure(NoCurrentWindow());
}

QJsonObject UpdateWindow(const QJsonArray &args, const Window *window){
    const QString api = QStringLiteral("windows.update");
    if(args.size() != 2 || !args.at(1).isObject())
        return Failure(QStringLiteral("%1 takes a window's id and one object").arg(api));
    bool whole = false;
    const qint64 id = Whole(args.at(0), &whole);
    if(!whole) return Failure(QStringLiteral("%1 takes a window's id and one object").arg(api));
    if(id != WINDOW && id != WINDOW_ID_CURRENT) return Failure(QStringLiteral("No window with id: %1.").arg(id));
    const QJsonObject info = args.at(1).toObject();
    for(QJsonObject::const_iterator it = info.constBegin(); it != info.constEnd(); ++it){
        if(it.value().isNull()) continue;
        if(it.key() != QStringLiteral("focused"))
            return Failure(QStringLiteral("%1: '%2' is not supported by this browser").arg(api, it.key()));
        if(!it.value().isBool())
            return Failure(QStringLiteral("%1: 'focused' has a value of the wrong kind").arg(api));
        if(!it.value().toBool())
            return Failure(QStringLiteral("%1: a window cannot be moved back by this browser").arg(api));
    }
    return window ? Success(DescribeWindow(*window, QList<Tab>(), Sight(), false)) : Failure(NoCurrentWindow());
}

bool Given(const QJsonObject &object, const char *key){
    const QJsonValue value = object.value(QLatin1String(key));
    return !value.isUndefined() && !value.isNull();
}

QString KeysOf(const QString &api, const QJsonObject &object, const QStringList &flags,
               const QStringList &numbers, const QStringList &texts, const QStringList &refused){
    for(QJsonObject::const_iterator it = object.constBegin(); it != object.constEnd(); ++it){
        if(it.value().isNull()) continue;
        bool ok = true;
        if(flags.contains(it.key())) ok = it.value().isBool();
        else if(numbers.contains(it.key())) Whole(it.value(), &ok);
        else if(texts.contains(it.key())) ok = it.value().isString();
        else if(refused.contains(it.key())) return QStringLiteral("%1: '%2' is not supported by this browser").arg(api, it.key());
        else return QStringLiteral("%1: '%2' is not a key of it").arg(api, it.key());
        if(!ok) return QStringLiteral("%1: '%2' has a value of the wrong kind").arg(api, it.key());
    }
    return QString();
}

QString ReadUrl(const QString &api, const QJsonObject &object, const QString &extensionId, Act *act){
    if(!Given(object, "url")) return QString();
    bool ok = false;
    act->url = TabUrlOf(extensionId, object.value(QStringLiteral("url")).toString(), &ok);
    act->hasUrl = ok;
    return ok ? QString() : QStringLiteral("%1: this browser does not let an extension open that address").arg(api);
}

}

bool IsCallUrl(const QUrl &url){
    return url.scheme() == QLatin1String(SCHEME) && url.host() == QStringLiteral("host") && url.path() == QStringLiteral("/call")
        && url.port() == -1 && url.userInfo().isEmpty() && !url.hasQuery() && !url.hasFragment();
}

bool IsBindUrl(const QUrl &url){
    return url.scheme() == QLatin1String(SCHEME) && url.host() == QStringLiteral("host") && url.path() == QStringLiteral("/bind")
        && url.port() == -1 && url.userInfo().isEmpty() && !url.hasQuery() && !url.hasFragment();
}

bool IsUserScriptMessageUrl(const QUrl &url){
    return url.scheme() == QLatin1String(SCHEME) && url.host() == QStringLiteral("host") && url.path() == QStringLiteral("/userScriptMessage")
        && url.port() == -1 && url.userInfo().isEmpty() && !url.hasQuery() && !url.hasFragment();
}

QByteArray WorldSecret(const QByteArray &processSecret, const QString &profileSpace, const QString &extensionId,
                       const QString &folder, const QString &worldId){
    if(processSecret.size() != 32 || extensionId.isEmpty()) return QByteArray();
    QByteArray all;
    foreach(const QString &part, QStringList() << QStringLiteral("user-script-world") << profileSpace << extensionId << folder << worldId){
        const QByteArray bytes = part.toUtf8();
        all += QByteArray::number(bytes.size()) + ':' + bytes;
    }
    return QMessageAuthenticationCode::hash(all, processSecret, QCryptographicHash::Sha256).toHex();
}

bool WithinHostPermissions(const QUrl &url, const QStringList &hostPermissions){
    const QString address = url.toString(QUrl::FullyEncoded);
    foreach(const QString &p, hostPermissions){
        const QString re = ExtensionMainScripts::PatternRegex(p, true);
        if(re.isEmpty()) continue;
        if(QRegularExpression(QStringLiteral("^") + re, QRegularExpression::CaseInsensitiveOption).match(address).hasMatch()) return true;
    }
    return false;
}

QString NoReceiver(){ return QStringLiteral("Could not establish connection. Receiving end does not exist."); }
QString PortClosed(){ return QStringLiteral("The message port closed before a response was received."); }

bool IsNonce(const QByteArray &nonce){
    if(nonce.size() != 32) return false;
    for(char c : nonce)
        if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

bool IsBindRequest(const QByteArray &method, const QByteArray &nonce){
    return method == QByteArrayLiteral("POST") && IsNonce(nonce);
}

QByteArray Stamp(const QByteArray &processSecret, quint64 view){
    if(processSecret.size() != 32 || !view) return QByteArray();
    const QByteArray number = QByteArray::number(view);
    return number + '.' + QMessageAuthenticationCode::hash(number, processSecret, QCryptographicHash::Sha256).toHex();
}

quint64 ViewOfStamp(const QByteArray &processSecret, const QByteArray &stamp){
    const int dot = stamp.indexOf('.');
    if(dot <= 0 || dot > 20) return 0;
    bool ok = false;
    const quint64 view = stamp.left(dot).toULongLong(&ok);
    if(!ok || !view) return 0;
    return SameKey(stamp, Stamp(processSecret, view)) ? view : 0;
}

bool BindTable::Bind(quint64 view, const QByteArray &nonce){
    if(!view || !IsNonce(nonce) || m_Views.contains(nonce)) return false;
    QList<QByteArray> &mine = m_Nonces[view];
    if(mine.size() >= BIND_LIMIT) m_Views.remove(mine.takeFirst());
    mine.append(nonce);
    m_Views.insert(nonce, view);
    return true;
}

quint64 BindTable::ViewOf(const QByteArray &nonce){
    const quint64 view = m_Views.value(nonce, 0);
    if(!view) return 0;
    const auto mine = m_Nonces.find(view);
    if(mine == m_Nonces.end()) return view;
    mine->removeOne(nonce);
    mine->append(nonce);
    return view;
}

void BindTable::Forget(quint64 view){
    foreach(const QByteArray &nonce, m_Nonces.take(view)) m_Views.remove(nonce);
}

QByteArray NonceOfTabOf(const Call &call){
    if(!call.error.isEmpty() || call.api != QStringLiteral("vanilla.tabOf")) return QByteArray();
    if(call.args.size() != 1 || !call.args.at(0).isString()) return QByteArray();
    const QByteArray nonce = call.args.at(0).toString().toLatin1();
    return IsNonce(nonce) ? nonce : QByteArray();
}

QJsonObject TabOfAnswer(qint64 id, int index){
    if(id <= 0 || index < 0) return Failure(QStringLiteral("no tab is known for that document"));
    QJsonObject tab;
    tab[QStringLiteral("id")] = id;
    tab[QStringLiteral("index")] = index;
    return Success(tab);
}

QJsonObject InstalledAnswer(const QJsonObject &details){
    return Success(details.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(details));
}

QJsonObject StartupAnswer(bool owed){
    return Success(owed);
}

void MenuPicks::Push(qint64 tab, const QString &pageUrl, qint64 now){
    if(tab <= 0 || pageUrl.isEmpty()) return;
    while(m_Picks.size() >= KEPT) m_Picks.removeFirst();
    m_Picks.append(Pick{tab, pageUrl, now});
}

qint64 MenuPicks::Take(const QString &pageUrl, qint64 now){
    for(int i = m_Picks.size() - 1; i >= 0; i--)
        if(now - m_Picks.at(i).at > FRESH) m_Picks.removeAt(i);
    if(pageUrl.isEmpty()) return 0;
    for(int i = 0; i < m_Picks.size(); i++){
        if(m_Picks.at(i).pageUrl != pageUrl) continue;
        const qint64 tab = m_Picks.at(i).tab;
        m_Picks.removeAt(i);
        return tab;
    }
    return 0;
}

ZoomCall ParseZoom(const Call &call){
    ZoomCall zoom;
    zoom.set = call.api == QStringLiteral("tabs.setZoom");
    const QString api = zoom.set ? QStringLiteral("tabs.setZoom") : QStringLiteral("tabs.getZoom");
    if(!call.error.isEmpty() || (!zoom.set && call.api != api)){
        zoom.error = QStringLiteral("not a zoom call");
        return zoom;
    }
    QJsonArray args = call.args;
    const int given = args.size();
    const int wanted = zoom.set ? 2 : 1;
    if(given > wanted || (zoom.set && given < 1)){
        zoom.error = QStringLiteral("%1 takes a tab's id%2").arg(api, zoom.set ? QStringLiteral(" and a factor") : QString());
        return zoom;
    }
    const bool hasTab = given == wanted;
    if(hasTab && !args.at(0).isNull()){
        const double id = args.at(0).toDouble(-1);
        if(!args.at(0).isDouble() || id != qint64(id) || id <= 0){
            zoom.error = QStringLiteral("%1 takes a tab's id%2").arg(api, zoom.set ? QStringLiteral(" and a factor") : QString());
            return zoom;
        }
        zoom.tab = qint64(id);
    }
    if(!zoom.set) return zoom;
    const QJsonValue factor = args.at(given - 1);
    if(!factor.isDouble()){
        zoom.error = QStringLiteral("tabs.setZoom takes a tab's id and a factor");
        return zoom;
    }
    zoom.factor = factor.toDouble();
    if(zoom.factor == 0) zoom.factor = 1.0;
    else if(!(zoom.factor >= 0.25 && zoom.factor <= 5.0)) zoom.error = QStringLiteral("Zoom value is out of range.");
    return zoom;
}

QJsonObject ZoomAnswer(double factor){
    return Success(factor);
}

QString PageUrlOfMenuTab(const Call &call, bool *ok){
    *ok = call.error.isEmpty() && call.api == QStringLiteral("vanilla.menuTab")
        && call.args.size() == 1 && call.args.at(0).isString();
    return *ok ? call.args.at(0).toString() : QString();
}

const char AUTH_NOT_APPROVED[] = "The user did not approve access.";
const char AUTH_INTERACTION[] = "User interaction required.";
const char AUTH_NOT_LOADED[] = "Authorization page could not be loaded.";

AuthFlow ParseAuthFlow(const Call &call){
    AuthFlow flow;
    const QJsonObject details = call.args.size() == 1 ? call.args.at(0).toObject() : QJsonObject();
    if(!call.error.isEmpty() || call.api != QStringLiteral("identity.launchWebAuthFlow") || details.isEmpty()){
        flow.error = QStringLiteral("identity.launchWebAuthFlow takes the details of a flow");
        return flow;
    }
    const QUrl url(details.value(QStringLiteral("url")).toString(), QUrl::StrictMode);
    if(!url.isValid() || url.host().isEmpty()
       || (url.scheme() != QStringLiteral("https") && url.scheme() != QStringLiteral("http"))){
        flow.error = QString::fromLatin1(AUTH_NOT_LOADED);
        return flow;
    }
    flow.url = url;
    const QJsonValue interactive = details.value(QStringLiteral("interactive"));
    if(interactive.isBool()) flow.interactive = interactive.toBool();
    const QJsonValue abort = details.value(QStringLiteral("abortOnLoadForNonInteractive"));
    if(abort.isBool()) flow.abortOnLoad = abort.toBool();
    if(details.contains(QStringLiteral("timeoutMsForNonInteractive"))){
        const double ms = details.value(QStringLiteral("timeoutMsForNonInteractive")).toDouble(0);
        flow.timeoutMs = ms > 0 ? int(qMin(ms, 60000.0)) : 0;
    }
    return flow;
}

bool IsAuthRedirect(const QString &id, const QUrl &url){
    return !id.isEmpty() && url.scheme() == QStringLiteral("https")
        && url.host().compare(id + QStringLiteral(".chromiumapp.org"), Qt::CaseInsensitive) == 0;
}

Capture ParseCapture(const Call &call){
    Capture capture;
    const QString shape = QStringLiteral("Error in invocation of tabs.captureVisibleTab(optional integer windowId,"
                                         " optional extensionTypes.ImageDetails options, optional function callback):"
                                         " No matching signature.");
    if(!call.error.isEmpty() || call.api != QStringLiteral("tabs.captureVisibleTab") || call.args.size() > 2){
        capture.error = shape;
        return capture;
    }
    QJsonValue options;
    if(call.args.size() == 2){
        if(!call.args.at(0).isDouble() && !call.args.at(0).isNull()){ capture.error = shape; return capture; }
        options = call.args.at(1);
    } else if(call.args.size() == 1){
        if(call.args.at(0).isObject() || call.args.at(0).isNull()) options = call.args.at(0);
        else if(!call.args.at(0).isDouble()){ capture.error = shape; return capture; }
    }
    if(options.isNull() || options.isUndefined()) return capture;
    if(!options.isObject()){ capture.error = shape; return capture; }
    const QJsonObject details = options.toObject();
    const QJsonValue format = details.value(QStringLiteral("format"));
    if(!format.isUndefined() && !format.isNull()){
        if(format.toString() == QStringLiteral("png")) capture.format = "png";
        else if(format.toString() != QStringLiteral("jpeg")){ capture.error = shape; return capture; }
    }
    const QJsonValue quality = details.value(QStringLiteral("quality"));
    if(!quality.isUndefined() && !quality.isNull()){
        const double q = quality.toDouble(-1);
        if(!quality.isDouble() || q != int(q) || q < 0 || q > 100){ capture.error = shape; return capture; }
        capture.quality = int(q);
    }
    return capture;
}

QString CaptureRefusal(const QUrl &page, const QString &id, const QStringList &hostPermissions, bool granted){
    const bool everything = granted || hostPermissions.contains(QStringLiteral("<all_urls>"));
    const bool web = everything || hostPermissions.contains(QStringLiteral("*://*/*"));
    if(!web) return QStringLiteral("Either the '<all_urls>' or 'activeTab' permission is required.");
    const QString scheme = page.scheme();
    if(scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) return QString();
    if(everything && scheme == QStringLiteral("data")) return QString();
    if(everything && scheme == QStringLiteral("chrome-extension") && !id.isEmpty() && page.host() == id) return QString();
    return QStringLiteral("Cannot access contents of the page. Extension manifest must request permission to access the respective host.");
}

bool CaptureAllowedAt(qint64 last, qint64 now){
    return last == 0 || now - last >= 500 || now < last;
}

void ActiveTabs::Grant(const QString &id, qint64 tab, quint64 load){
    if(id.isEmpty() || tab <= 0 || load == 0) return;
    QList<QPair<qint64, quint64> > &tabs = m_Tabs[id];
    for(int i = tabs.size() - 1; i >= 0; i--) if(tabs.at(i).first == tab) tabs.removeAt(i);
    tabs.append(qMakePair(tab, load));
    while(tabs.size() > KEPT) tabs.removeFirst();
}

bool ActiveTabs::Granted(const QString &id, qint64 tab, quint64 load) const {
    if(load == 0) return false;
    return m_Tabs.value(id).contains(qMakePair(tab, load));
}

void ActiveTabs::Forget(const QString &id){
    m_Tabs.remove(id);
}

QStringList ActiveTabs::Ids() const {
    return m_Tabs.keys();
}

QByteArray TokenOfEvents(const Call &call){
    if(!call.error.isEmpty() || call.api != QStringLiteral("vanilla.events")) return QByteArray();
    if(call.args.isEmpty() || call.args.size() > 3 || !call.args.at(0).isString()) return QByteArray();
    if(call.args.size() >= 2 && !call.args.at(1).isArray()) return QByteArray();
    if(call.args.size() == 3 && !call.args.at(2).isDouble()) return QByteArray();
    const QByteArray token = call.args.at(0).toString().toLatin1();
    return IsNonce(token) ? token : QByteArray();
}

QByteArray TokenOfEventNames(const Call &call){
    if(!call.error.isEmpty() || call.api != QStringLiteral("vanilla.eventNames")) return QByteArray();
    if(call.args.size() < 2 || call.args.size() > 3 || !call.args.at(0).isString() || !call.args.at(1).isArray()) return QByteArray();
    if(call.args.size() == 3 && !call.args.at(2).isDouble()) return QByteArray();
    const QByteArray token = call.args.at(0).toString().toLatin1();
    return IsNonce(token) ? token : QByteArray();
}

qint64 NumberOfEventNames(const Call &call){
    if(!call.error.isEmpty() || call.args.size() != 3 || !call.args.at(2).isDouble()) return 0;
    if(call.api != QStringLiteral("vanilla.events") && call.api != QStringLiteral("vanilla.eventNames")) return 0;
    const double n = call.args.at(2).toDouble();
    return n >= 1 && n <= 9007199254740991.0 ? static_cast<qint64>(n) : 0;
}

QSet<QString> NamesOfEvents(const Call &call){
    static const QSet<QString> known = QSet<QString>()
        << QStringLiteral("tabs.onCreated") << QStringLiteral("tabs.onRemoved") << QStringLiteral("tabs.onUpdated")
        << QStringLiteral("tabs.onActivated") << QStringLiteral("tabs.onMoved") << QStringLiteral("tabs.onHighlighted")
        << QStringLiteral("tabs.onZoomChange") << QStringLiteral("windows.onFocusChanged")
        << QStringLiteral("action.onClicked") << QStringLiteral("contextMenus.onClicked")
        << QStringLiteral("downloads.onCreated") << QStringLiteral("downloads.onChanged") << QStringLiteral("downloads.onErased")
        << QStringLiteral("history.onVisited") << QStringLiteral("history.onVisitRemoved")
        << QStringLiteral("commands.onCommand") << QStringLiteral("sidePanel.onOpened") << QStringLiteral("sidePanel.onClosed");
    QSet<QString> names;
    if(!call.error.isEmpty() || call.args.size() < 2 || !call.args.at(1).isArray()) return names;
    if(call.api != QStringLiteral("vanilla.events") && call.api != QStringLiteral("vanilla.eventNames")) return names;
    foreach(const QJsonValue &name, call.args.at(1).toArray())
        if(known.contains(name.toString())) names.insert(name.toString());
    return names;
}

QByteArray TokenOfAbort(const Call &call){
    if(!call.error.isEmpty() || call.api != QStringLiteral("vanilla.abort")) return QByteArray();
    if(call.args.size() != 1 || !call.args.at(0).isString()) return QByteArray();
    const QByteArray token = call.args.at(0).toString().toLatin1();
    return IsNonce(token) ? token : QByteArray();
}

namespace {

QJsonObject Event(const char *name, const QJsonArray &args){
    QJsonObject event;
    event[QStringLiteral("name")] = QString::fromLatin1(name);
    event[QStringLiteral("args")] = args;
    return event;
}

qint64 ActiveOf(const QList<Tab> &tabs){
    foreach(const Tab &tab, tabs) if(tab.active) return tab.id;
    return 0;
}

QJsonObject Activated(qint64 id){
    QJsonObject info;
    info[QStringLiteral("tabId")] = id;
    info[QStringLiteral("windowId")] = WINDOW;
    return Event("tabs.onActivated", QJsonArray() << info);
}

double ZoomOf(const Tab &tab){
    return tab.zoom > 0 ? tab.zoom : 1.0;
}

QJsonArray Moves(const QList<qint64> &before, const QList<qint64> &after){
    QHash<qint64, int> place;
    for(int i = 0; i < after.size(); i++) place.insert(after.at(i), i);
    QList<int> tails, previous;
    for(int i = 0; i < before.size(); i++){
        const int p = place.value(before.at(i));
        int low = 0, high = tails.size();
        while(low < high){
            const int mid = (low + high) / 2;
            if(place.value(before.at(tails.at(mid))) < p) low = mid + 1; else high = mid;
        }
        previous.append(low > 0 ? tails.at(low - 1) : -1);
        if(low == tails.size()) tails.append(i); else tails[low] = i;
    }
    QSet<qint64> stayed;
    for(int i = tails.isEmpty() ? -1 : tails.last(); i >= 0; i = previous.at(i)) stayed.insert(before.at(i));

    QJsonArray moves;
    QList<qint64> order = before;
    for(int k = 0; k < after.size(); k++){
        const qint64 id = after.at(k);
        if(stayed.contains(id)) continue;
        const int from = order.indexOf(id);
        order.removeAt(from);
        const int to = k > 0 ? order.indexOf(after.at(k - 1)) + 1 : 0;
        order.insert(to, id);
        QJsonObject info;
        info[QStringLiteral("windowId")] = WINDOW;
        info[QStringLiteral("fromIndex")] = from;
        info[QStringLiteral("toIndex")] = to;
        moves.append(Event("tabs.onMoved", QJsonArray() << id << info));
    }
    return moves;
}

qint64 VisitOf(const Tab &tab){
    return tab.visited > 0 ? tab.visited : tab.added;
}

QJsonObject HistoryItemOf(const Tab &tab){
    QJsonObject out;
    out[QStringLiteral("id")] = QString::number(tab.id);
    out[QStringLiteral("url")] = tab.url.toString(QUrl::FullyEncoded);
    out[QStringLiteral("title")] = tab.title;
    out[QStringLiteral("lastVisitTime")] = static_cast<double>(VisitOf(tab));
    return out;
}

}

QJsonArray Diff(const QList<Tab> &before, const QList<Tab> &after,
                bool focusedBefore, bool focusedAfter, const Sight &sight){
    QHash<qint64, int> was, is;
    for(int i = 0; i < before.size(); i++) was.insert(before.at(i).id, i);
    for(int i = 0; i < after.size(); i++) is.insert(after.at(i).id, i);

    QJsonArray events;
    foreach(const Tab &tab, before){
        if(is.contains(tab.id)) continue;
        QJsonObject info;
        info[QStringLiteral("windowId")] = WINDOW;
        info[QStringLiteral("isWindowClosing")] = false;
        events.append(Event("tabs.onRemoved", QJsonArray() << tab.id << info));
    }
    {
        QList<qint64> stay, land;
        foreach(const Tab &tab, before) if(is.contains(tab.id)) stay.append(tab.id);
        foreach(const Tab &tab, after) if(was.contains(tab.id)) land.append(tab.id);
        foreach(const QJsonValue &move, Moves(stay, land)) events.append(move);
    }
    for(int i = 0; i < after.size(); i++){
        if(was.contains(after.at(i).id)) continue;
        events.append(Event("tabs.onCreated", QJsonArray() << Describe(after.at(i), i, sight)));
    }
    QJsonArray zooms;
    for(int i = 0; i < after.size(); i++){
        const Tab &now = after.at(i);
        if(!was.contains(now.id)) continue;
        const Tab &then = before.at(was.value(now.id));
        const bool seen = sight.Sees(now.url), saw = sight.Sees(then.url);
        const bool moved = seen && (!saw || then.url != now.url);
        bool urlSaid = false;
        QString told = StatusOf(then);
        if(now.loads && now.loads != then.loads && !now.discarded && told != QStringLiteral("loading")){
            QJsonObject begun;
            begun[QStringLiteral("status")] = QStringLiteral("loading");
            if(moved){ begun[QStringLiteral("url")] = now.url.toString(QUrl::FullyEncoded); urlSaid = true; }
            Tab loading = now;
            loading.loading = true;
            events.append(Event("tabs.onUpdated", QJsonArray() << now.id << begun << Describe(loading, i, sight)));
            told = QStringLiteral("loading");
        }
        QJsonObject change;
        if(then.discarded != now.discarded) change[QStringLiteral("discarded")] = now.discarded;
        if(moved && !urlSaid) change[QStringLiteral("url")] = now.url.toString(QUrl::FullyEncoded);
        if(seen && (!saw || then.title != now.title)) change[QStringLiteral("title")] = now.title;
        if(then.audible != now.audible) change[QStringLiteral("audible")] = now.audible;
        if(then.muted != now.muted){
            QJsonObject muted;
            muted[QStringLiteral("muted")] = now.muted;
            change[QStringLiteral("mutedInfo")] = muted;
        }
        if(StatusOf(now) != told) change[QStringLiteral("status")] = StatusOf(now);
        if(!change.isEmpty())
            events.append(Event("tabs.onUpdated", QJsonArray() << now.id << change << Describe(now, i, sight)));
        if(qAbs(ZoomOf(now) - ZoomOf(then)) >= 1e-3){
            QJsonObject zoom;
            zoom[QStringLiteral("tabId")] = now.id;
            zoom[QStringLiteral("oldZoomFactor")] = ZoomOf(then);
            zoom[QStringLiteral("newZoomFactor")] = ZoomOf(now);
            QJsonObject settings;
            settings[QStringLiteral("mode")] = QStringLiteral("automatic");
            settings[QStringLiteral("scope")] = QStringLiteral("per-tab");
            zoom[QStringLiteral("zoomSettings")] = settings;
            zooms.append(Event("tabs.onZoomChange", QJsonArray() << zoom));
        }
    }
    foreach(const QJsonValue &zoom, zooms) events.append(zoom);
    const qint64 active = ActiveOf(after);
    if(active && active != ActiveOf(before)){
        events.append(Activated(active));
        QJsonObject highlighted;
        highlighted[QStringLiteral("windowId")] = WINDOW;
        highlighted[QStringLiteral("tabIds")] = QJsonArray() << active;
        events.append(Event("tabs.onHighlighted", QJsonArray() << highlighted));
    }
    if(active && focusedBefore != focusedAfter)
        events.append(Event("windows.onFocusChanged", QJsonArray() << (focusedAfter ? WINDOW : WINDOW_ID_NONE)));
    if(sight.history){
        QSet<QString> held;
        foreach(const Tab &tab, after) if(!tab.url.isEmpty()) held.insert(tab.url.toString(QUrl::FullyEncoded));
        QJsonArray gone;
        QSet<QString> said;
        foreach(const Tab &tab, before){
            const QString url = tab.url.toString(QUrl::FullyEncoded);
            if(url.isEmpty() || held.contains(url) || said.contains(url)) continue;
            said.insert(url);
            gone.append(url);
        }
        if(!gone.isEmpty()){
            QJsonObject removed;
            removed[QStringLiteral("allHistory")] = false;
            removed[QStringLiteral("urls")] = gone;
            events.append(Event("history.onVisitRemoved", QJsonArray() << removed));
        }
        foreach(const Tab &now, after){
            if(now.url.isEmpty()) continue;
            const bool fresh = !was.contains(now.id);
            const Tab *then = fresh ? nullptr : &before.at(was.value(now.id));
            if(fresh || then->url != now.url || VisitOf(*then) != VisitOf(now))
                events.append(Event("history.onVisited", QJsonArray() << HistoryItemOf(now)));
        }
    }
    return events;
}

bool SameOrder(const QList<Tab> &before, const QList<Tab> &after){
    if(before.size() != after.size()) return false;
    for(int i = 0; i < before.size(); i++)
        if(before.at(i).id != after.at(i).id) return false;
    return true;
}

QJsonArray Greeting(const QList<Tab> &tabs){
    const qint64 active = ActiveOf(tabs);
    return active ? QJsonArray() << Activated(active) : QJsonArray();
}

QJsonObject EventsAnswer(const QJsonArray &events, const QList<Tab> &tabs){
    QJsonArray order;
    foreach(const Tab &tab, tabs) order.append(tab.id);
    QJsonObject value;
    value[QStringLiteral("events")] = events;
    value[QStringLiteral("order")] = order;
    return Success(value);
}

QJsonObject StaleAnswer(){
    QJsonObject value;
    value[QStringLiteral("events")] = QJsonArray();
    value[QStringLiteral("stale")] = true;
    return Success(value);
}

QJsonObject AbortedAnswer(){
    QJsonObject value;
    value[QStringLiteral("events")] = QJsonArray();
    value[QStringLiteral("aborted")] = true;
    return Success(value);
}

QString TooManySubscribers(){
    return QStringLiteral("Too many are listening for this extension's events.");
}

QByteArray HeaderOf(const QMap<QByteArray, QByteArray> &headers, const char *name){
    QByteArray found;
    int times = 0;
    for(QMap<QByteArray, QByteArray>::const_iterator it = headers.constBegin(); it != headers.constEnd(); ++it){
        if(it.key().compare(name, Qt::CaseInsensitive) != 0) continue;
        found = it.value().trimmed();
        times++;
    }
    return times == 1 ? found : QByteArray();
}

QByteArray SecretOf(const QByteArray &fileBytes){
    return fileBytes.size() == 32 ? fileBytes : QByteArray();
}

QString SpaceOfProfileKey(const QString &profileKey){
    if(!profileKey.contains(QLatin1Char(':'))) return profileKey;
    return profileKey.startsWith(QStringLiteral("quick:")) ? profileKey.section(QLatin1Char(':'), 1) : QString();
}

QString ExtensionIdOf(const QUrl &initiator){
    if(initiator.scheme() != QStringLiteral("chrome-extension")) return QString();
    if(initiator.port() != -1 || !initiator.userInfo().isEmpty() || initiator.hasQuery() || initiator.hasFragment()) return QString();
    if(!initiator.path().isEmpty() && initiator.path() != QStringLiteral("/")) return QString();
    const QString id = initiator.host();
    static const QRegularExpression shape(QStringLiteral("\\A[a-p]{32}\\z"));
    return shape.match(id).hasMatch() ? id : QString();
}

QByteArray KeyFor(const QByteArray &secret, const QString &id){
    if(secret.size() < 32 || id.isEmpty()) return QByteArray();
    return QMessageAuthenticationCode::hash(id.toLatin1(), secret, QCryptographicHash::Sha256).toHex();
}

bool SameKey(const QByteArray &a, const QByteArray &b){
    if(a.isEmpty() || a.size() != b.size()) return false;
    unsigned char difference = 0;
    for(int i = 0; i < a.size(); i++) difference |= static_cast<unsigned char>(a.at(i)) ^ static_cast<unsigned char>(b.at(i));
    return difference == 0;
}

QString Admit(const QByteArray &method, const QUrl &initiator, const QByteArray &key,
              const QSet<QString> &shimmed, const QByteArray &secret){
    if(method != QByteArrayLiteral("POST")) return QString();
    const QString id = ExtensionIdOf(initiator);
    if(id.isEmpty() || !shimmed.contains(id)) return QString();
    if(!SameKey(key, KeyFor(secret, id))) return QString();
    return id;
}

Call ParseCall(const QByteArray &header){
    Call call;
    if(header.isEmpty()){ call.error = QStringLiteral("nothing was asked"); return call; }
    if(header.size() > CALL_LIMIT){ call.error = QStringLiteral("what was asked is too long"); return call; }
    QJsonParseError problem;
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromPercentEncoding(header), &problem);
    if(problem.error != QJsonParseError::NoError || !document.isObject()){
        call.error = QStringLiteral("what was asked is not a JSON object");
        return call;
    }
    const QJsonObject asked = document.object();
    if(!asked.value(QStringLiteral("api")).isString() || !asked.value(QStringLiteral("args")).isArray()){
        call.error = QStringLiteral("what was asked has no 'api' or no 'args'");
        return call;
    }
    call.api = asked.value(QStringLiteral("api")).toString();
    call.args = asked.value(QStringLiteral("args")).toArray();
    return call;
}

bool NodeIsOfProfile(const QStringList &settings, const QString &space, const QString &profileSpace){
    static const QRegularExpression isPrivate(QStringLiteral("\\A(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)\\z"));
    static const QRegularExpression notPrivate(QStringLiteral("\\A!(?:[pP]rivate|[oO]ff[tT]he[rR]ecord)\\z"));
    bool said = false;
    foreach(const QString &token, settings){
        if(notPrivate.match(token).hasMatch()){ said = false; break; }
        if(isPrivate.match(token).hasMatch()) said = true;
    }
    if(said) return false;
    return !profileSpace.isEmpty() && space == profileSpace;
}

bool TabIsTold(TabOwner owner, const std::function<bool()> &directoryIsOfProfile){
    switch(owner){
    case TabOwner::Asking: return true;
    case TabOwner::Other: return false;
    case TabOwner::NotYet: return directoryIsOfProfile();
    }
    return false;
}

bool Matches(const QString &pattern, const QUrl &url, bool *ok){
    *ok = true;
    if(pattern == QStringLiteral("<all_urls>")) return ALL_URLS.contains(url.scheme());

    static const QRegularExpression shape(QStringLiteral("\\A(\\*|[a-z][a-z0-9+.-]*)://([^/]*)(/.*)\\z"));
    const QRegularExpressionMatch parts = shape.match(pattern);
    const QString scheme = parts.captured(1), host = parts.captured(2), path = parts.captured(3);
    const bool hostReadable = scheme == QStringLiteral("file") ? host.isEmpty()
        : !host.isEmpty() && !host.contains(QLatin1Char(':')) &&
          (host == QStringLiteral("*") || !host.mid(host.startsWith(QStringLiteral("*.")) ? 2 : 0).contains(QLatin1Char('*')));
    if(!parts.hasMatch() || !hostReadable || (scheme != QStringLiteral("*") && !ALL_URLS.contains(scheme))){
        *ok = false;
        return false;
    }

    if(scheme == QStringLiteral("*")){
        if(url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https")) return false;
    } else if(url.scheme() != scheme) return false;

    const QString actual = url.host(QUrl::FullyEncoded).toLower();
    if(host.startsWith(QStringLiteral("*."))){
        const QString domain = host.mid(2).toLower();
        if(actual != domain && !actual.endsWith(QLatin1Char('.') + domain)) return false;
    } else if(host != QStringLiteral("*") && actual != host.toLower()) return false;

    QString asked = url.path(QUrl::FullyEncoded);
    if(asked.isEmpty()) asked = QStringLiteral("/");
    if(url.hasQuery()) asked += QLatin1Char('?') + url.query(QUrl::FullyEncoded);
    return Glob(path, asked);
}

Sight Sight::Of(bool tabs, const QString &self, const QStringList &patterns){
    Sight sight;
    sight.tabs = tabs;
    sight.self = self;
    static const QRegularExpression shape(QStringLiteral("\\A([^:/]+://[^/]*)/"));
    foreach(const QString &pattern, patterns){
        QString host = pattern;
        if(pattern != QStringLiteral("<all_urls>")){
            const QRegularExpressionMatch parts = shape.match(pattern);
            if(!parts.hasMatch()) continue;
            host = parts.captured(1) + QStringLiteral("/*");
            if(host.startsWith(QStringLiteral("file:"))) continue;
        }
        bool ok = false;
        Matches(host, QUrl(), &ok);
        if(ok && !sight.hosts.contains(host)) sight.hosts << host;
    }
    return sight;
}

bool Sight::Sees(const QUrl &url) const {
    if(tabs) return true;
    if(url.scheme() == QStringLiteral("chrome-extension"))
        return !self.isEmpty() && url.host() == self;
    if(url.scheme() == QStringLiteral("file")) return false;
    foreach(const QString &pattern, hosts){
        bool ok = false;
        if(Matches(pattern, url, &ok) && ok) return true;
    }
    return false;
}

QJsonObject Answer(const Call &call, const QList<Tab> &tabs, const Sight &sight, const Window *window){
    if(!call.error.isEmpty()) return Failure(call.error);
    if(call.api == QStringLiteral("tabs.query")) return Query(call.args, tabs, sight);
    if(call.api == QStringLiteral("tabs.get")) return Get(call.args, tabs, sight);
    if(call.api == QStringLiteral("windows.getCurrent") || call.api == QStringLiteral("windows.getAll"))
        return Windows(call.api, call.args, tabs, sight, window);
    if(call.api == QStringLiteral("windows.update")) return UpdateWindow(call.args, window);
    return Failure(NotAvailable(call.api));
}

QString NoTab(qint64 id){ return QStringLiteral("No tab with id: %1.").arg(id); }
QString NoCurrentWindow(){ return QStringLiteral("No current window"); }
QString NotEditable(){ return QStringLiteral("Tabs cannot be edited right now (user may be dragging a tab)."); }

QJsonObject Done(){
    QJsonObject reply;
    reply[QStringLiteral("ok")] = true;
    return reply;
}

QJsonObject TabAnswer(qint64 id, const QList<Tab> &tabs, const Sight &sight){
    for(int index = 0; index < tabs.size(); index++)
        if(tabs.at(index).id == id) return Success(Describe(tabs.at(index), index, sight));
    return Failure(NoTab(id));
}

QJsonObject Refused(const QString &error){ return Failure(error); }
QJsonObject Accepted(const QJsonValue &value){ return Success(value); }

QUrl TabUrlOf(const QString &extensionId, const QString &text, bool *ok){
    *ok = false;
    const QUrl base(QStringLiteral("chrome-extension://") + extensionId + QLatin1Char('/'));
    const QUrl given(text);
    if(extensionId.isEmpty() || text.isEmpty() || !given.isValid()) return QUrl();
    const QUrl url = base.resolved(given);
    if(!url.isValid()) return QUrl();
    const QString scheme = url.scheme();
    if(scheme == QStringLiteral("http") || scheme == QStringLiteral("https"))
        *ok = !url.host().isEmpty();
    else if(scheme == QStringLiteral("chrome-extension"))
        *ok = url.host() == extensionId && url.port() == -1 && url.userInfo().isEmpty();
    else
        *ok = url == QUrl(QStringLiteral("about:blank"));
    return *ok ? url : QUrl();
}

QUrl PopupWindowUrlOf(const QString &extensionId, const QUrl &shown, const QString &text, bool *ok){
    const QUrl url = TabUrlOf(extensionId, text, ok);
    if(*ok && url.scheme() == QStringLiteral("chrome-extension")
       && !(shown.scheme() == QStringLiteral("chrome-extension") && shown.host() == extensionId)){
        *ok = false;
        return QUrl();
    }
    return url;
}

QJsonObject FiredEvent(const QString &name, const QJsonArray &args){
    return Event(name.toLatin1().constData(), args);
}

QUrl OffscreenUrlOf(const QString &extensionId, const QString &text, bool *ok){
    *ok = false;
    if(extensionId.isEmpty() || text.isEmpty() || text.contains(QLatin1Char('\\'))) return QUrl();
    const QUrl base(QStringLiteral("chrome-extension://") + extensionId + QLatin1Char('/'));
    const QUrl given(text);
    if(!given.isValid()) return QUrl();
    if(given.path(QUrl::FullyDecoded).split(QLatin1Char('/')).contains(QStringLiteral(".."))) return QUrl();
    if(text.contains(QStringLiteral("%2f"), Qt::CaseInsensitive) || text.contains(QStringLiteral("%5c"), Qt::CaseInsensitive)) return QUrl();
    const QUrl url = base.resolved(given);
    if(!url.isValid() || url.scheme() != QStringLiteral("chrome-extension") || url.host() != extensionId
       || url.port() != -1 || !url.userInfo().isEmpty()) return QUrl();
    const QString path = url.path();
    if(path.size() < 2 || !path.startsWith(QLatin1Char('/')) || path.split(QLatin1Char('/')).contains(QStringLiteral(".."))
       || path.contains(QStringLiteral("%2e%2e"), Qt::CaseInsensitive)) return QUrl();
    *ok = true;
    return url;
}

bool IsAct(const QString &api){
    return api == QStringLiteral("tabs.create") || api == QStringLiteral("tabs.update")
        || api == QStringLiteral("tabs.remove") || api == QStringLiteral("tabs.reload")
        || api == QStringLiteral("search.query")
        || api == QStringLiteral("sessions.restore") || api == QStringLiteral("tabs.duplicate")
        || api == QStringLiteral("tabs.move");
}

QJsonObject SessionAnswer(qint64 lastModified, qint64 id, const QList<Tab> &tabs, const Sight &sight){
    QJsonObject session;
    session[QStringLiteral("lastModified")] = static_cast<double>(lastModified);
    for(int index = 0; index < tabs.size(); index++)
        if(tabs.at(index).id == id) session[QStringLiteral("tab")] = Describe(tabs.at(index), index, sight);
    return Success(session);
}

QJsonObject TabsAnswer(const QList<qint64> &ids, const QList<Tab> &tabs, const Sight &sight){
    QJsonArray out;
    foreach(qint64 id, ids){
        bool found = false;
        for(int index = 0; index < tabs.size() && !found; index++)
            if(tabs.at(index).id == id){ out.append(Describe(tabs.at(index), index, sight)); found = true; }
        if(!found) return Failure(NoTab(id));
    }
    return Success(out);
}

QString WhyNotSearchable(const QString &format, const QUrl &built){
    if(!format.contains(QStringLiteral("%1")))
        return QStringLiteral("the default search engine has no place for the text");
    if(!built.isValid() || built.host().isEmpty()
       || (built.scheme() != QStringLiteral("http") && built.scheme() != QStringLiteral("https")))
        return QStringLiteral("the default search engine is not a web address");
    return QString();
}

Act ParseAct(const Call &call, const QString &extensionId){
    Act act;
    if(!call.error.isEmpty()){ act.error = call.error; return act; }
    const QString api = call.api;
    QJsonArray args = call.args;
    while(!args.isEmpty() && args.last().isNull()) args.removeLast();
    auto refuse = [&](const QString &why){ act.error = why; return act; };

    if(api == QStringLiteral("tabs.create")){
        if(args.size() > 1 || (args.size() == 1 && !args.at(0).isObject()))
            return refuse(QStringLiteral("tabs.create takes one object"));
        const QJsonObject asked = args.size() == 1 ? args.at(0).toObject() : QJsonObject();
        const QString wrong = KeysOf(api, asked, QStringList() << QStringLiteral("active") << QStringLiteral("pinned"),
                                     QStringList() << QStringLiteral("index") << QStringLiteral("openerTabId") << QStringLiteral("windowId"),
                                     QStringList() << QStringLiteral("url"), QStringList() << QStringLiteral("selected"));
        if(!wrong.isEmpty()) return refuse(wrong);
        bool ok = true;
        if(Given(asked, "windowId")){
            const qint64 window = Whole(asked.value(QStringLiteral("windowId")), &ok);
            if(window != WINDOW && window != WINDOW_ID_CURRENT) return refuse(QStringLiteral("No window with id: %1.").arg(window));
        }
        if(asked.value(QStringLiteral("pinned")).toBool())
            return refuse(QStringLiteral("tabs.create: 'pinned' is not supported by this browser"));
        const QString address = ReadUrl(api, asked, extensionId, &act);
        if(!address.isEmpty()) return refuse(address);
        if(!act.hasUrl){ act.hasUrl = true; act.url = QUrl(QStringLiteral("about:blank")); }
        act.activate = !Given(asked, "active") || asked.value(QStringLiteral("active")).toBool();
        if(Given(asked, "openerTabId")) act.opener = Whole(asked.value(QStringLiteral("openerTabId")), &ok);
        if(Given(asked, "openerTabId") && act.opener <= 0) return refuse(NoTab(act.opener));
        act.kind = Act::Create;
        return act;
    }

    if(api == QStringLiteral("tabs.update") || api == QStringLiteral("tabs.reload")){
        const bool update = api == QStringLiteral("tabs.update");
        bool ok = true;
        int at = 0;
        act.current = true;
        if(at < args.size() && !args.at(at).isObject()){
            if(!args.at(at).isNull()){
                act.tab = Whole(args.at(at), &ok);
                if(!ok) return refuse(QStringLiteral("%1 takes the id of a tab").arg(api));
                act.current = false;
            }
            at++;
        }
        QJsonObject asked;
        if(at < args.size()){
            if(!args.at(at).isObject()) return refuse(QStringLiteral("%1 takes an object after the id").arg(api));
            asked = args.at(at).toObject();
            at++;
        } else if(update) return refuse(QStringLiteral("tabs.update takes an object"));
        if(at < args.size()) return refuse(QStringLiteral("%1 was handed too much").arg(api));
        if(!act.current && act.tab <= 0) return refuse(NoTab(act.tab));

        if(!update){
            const QString wrong = KeysOf(api, asked, QStringList() << QStringLiteral("bypassCache"), QStringList(), QStringList(), QStringList());
            if(!wrong.isEmpty()) return refuse(wrong);
            act.bypassCache = asked.value(QStringLiteral("bypassCache")).toBool();
            act.kind = Act::Reload;
            return act;
        }
        const QString wrong = KeysOf(api, asked, QStringList() << QStringLiteral("active") << QStringLiteral("highlighted") << QStringLiteral("muted"),
                                     QStringList(), QStringList() << QStringLiteral("url"),
                                     QStringList() << QStringLiteral("selected") << QStringLiteral("pinned")
                                                   << QStringLiteral("autoDiscardable") << QStringLiteral("openerTabId"));
        if(!wrong.isEmpty()) return refuse(wrong);
        const QString address = ReadUrl(api, asked, extensionId, &act);
        if(!address.isEmpty()) return refuse(address);
        act.activate = asked.value(QStringLiteral("active")).toBool() || asked.value(QStringLiteral("highlighted")).toBool();
        act.hasMuted = Given(asked, "muted");
        act.muted = asked.value(QStringLiteral("muted")).toBool();
        act.kind = Act::Update;
        return act;
    }

    if(api == QStringLiteral("search.query")){
        if(args.size() != 1 || !args.at(0).isObject())
            return refuse(QStringLiteral("search.query takes one object"));
        const QJsonObject asked = args.at(0).toObject();
        const QString wrong = KeysOf(api, asked, QStringList(), QStringList() << QStringLiteral("tabId"),
                                     QStringList() << QStringLiteral("text") << QStringLiteral("disposition"), QStringList());
        if(!wrong.isEmpty()) return refuse(wrong);
        if(!Given(asked, "text")) return refuse(QStringLiteral("search.query: 'text' is required"));
        act.text = asked.value(QStringLiteral("text")).toString();
        if(act.text.isEmpty()) return refuse(QStringLiteral("search.query: 'text' is empty"));
        const bool where = Given(asked, "disposition"), byTab = Given(asked, "tabId");
        if(where && byTab) return refuse(QStringLiteral("search.query: 'disposition' and 'tabId' cannot both be given"));
        act.current = true;
        if(byTab){
            bool ok = true;
            act.tab = Whole(asked.value(QStringLiteral("tabId")), &ok);
            act.current = false;
            if(act.tab <= 0) return refuse(NoTab(act.tab));
        }
        if(where){
            const QString disposition = asked.value(QStringLiteral("disposition")).toString();
            if(disposition == QStringLiteral("NEW_TAB") || disposition == QStringLiteral("NEW_WINDOW")){
                act.newTab = true;
                act.current = false;
            } else if(disposition != QStringLiteral("CURRENT_TAB")){
                return refuse(QStringLiteral("search.query: 'disposition' has a value of the wrong kind"));
            }
        }
        act.kind = Act::Search;
        return act;
    }

    if(api == QStringLiteral("sessions.restore")){
        if(args.isEmpty()){ act.kind = Act::Restore; return act; }
        if(args.size() == 1 && args.at(0).isString())
            return refuse(QStringLiteral("No session with id: %1.").arg(args.at(0).toString()));
        return refuse(QStringLiteral("sessions.restore takes the id of a session, or nothing"));
    }

    if(api == QStringLiteral("tabs.duplicate")){
        bool ok = false;
        const qint64 id = args.size() == 1 ? Whole(args.at(0), &ok) : 0;
        if(!ok) return refuse(QStringLiteral("tabs.duplicate takes the id of a tab"));
        if(id <= 0) return refuse(NoTab(id));
        act.tab = id;
        act.kind = Act::Duplicate;
        return act;
    }

    if(api == QStringLiteral("tabs.move")){
        if(args.size() != 2 || !args.at(1).isObject())
            return refuse(QStringLiteral("tabs.move takes the id of a tab, or a list of them, and an object"));
        act.many = args.at(0).isArray();
        const QJsonArray list = act.many ? args.at(0).toArray() : QJsonArray() << args.at(0);
        if(list.isEmpty()) return refuse(QStringLiteral("No tabs given."));
        foreach(const QJsonValue &one, list){
            bool ok = false;
            const qint64 id = Whole(one, &ok);
            if(!ok) return refuse(QStringLiteral("tabs.move takes the id of a tab, or a list of them, and an object"));
            if(id <= 0) return refuse(NoTab(id));
            if(!act.tabs.contains(id)) act.tabs << id;
        }
        const QJsonObject asked = args.at(1).toObject();
        const QString wrong = KeysOf(api, asked, QStringList(),
                                     QStringList() << QStringLiteral("index") << QStringLiteral("windowId"), QStringList(), QStringList());
        if(!wrong.isEmpty()) return refuse(wrong);
        if(!Given(asked, "index")) return refuse(QStringLiteral("tabs.move: 'index' is required"));
        bool ok = true;
        act.index = Whole(asked.value(QStringLiteral("index")), &ok);
        if(act.index < -1) return refuse(QStringLiteral("tabs.move: 'index' has a value of the wrong kind"));
        if(Given(asked, "windowId")){
            const qint64 window = Whole(asked.value(QStringLiteral("windowId")), &ok);
            if(window != WINDOW && window != WINDOW_ID_CURRENT) return refuse(QStringLiteral("No window with id: %1.").arg(window));
        }
        act.kind = Act::Move;
        return act;
    }

    if(api == QStringLiteral("tabs.remove")){
        if(args.size() != 1) return refuse(QStringLiteral("tabs.remove takes the id of a tab, or a list of them"));
        const QJsonArray list = args.at(0).isArray() ? args.at(0).toArray() : QJsonArray() << args.at(0);
        foreach(const QJsonValue &one, list){
            bool ok = false;
            const qint64 id = Whole(one, &ok);
            if(!ok) return refuse(QStringLiteral("tabs.remove takes the id of a tab, or a list of them"));
            if(id <= 0) return refuse(NoTab(id));
            if(!act.tabs.contains(id)) act.tabs << id;
        }
        act.kind = Act::Remove;
        return act;
    }
    return refuse(NotAvailable(api));
}

bool TakesBody(const QString &api){
    return api == QStringLiteral("declarativeNetRequest.updateDynamicRules")
        || api == QStringLiteral("declarativeNetRequest.updateSessionRules")
        || api == QStringLiteral("vanilla.mainScripts")
        || api == QStringLiteral("userScripts.register")
        || api == QStringLiteral("userScripts.update")
        || api == QStringLiteral("vanilla.userScriptReply");
}

QJsonArray ArgsOfBody(const QByteArray &body, QString *error){
    QJsonParseError problem;
    const QJsonDocument document = QJsonDocument::fromJson(body, &problem);
    if(problem.error != QJsonParseError::NoError || !document.isArray()){
        if(error) *error = QStringLiteral("what was sent is not a JSON list");
        return QJsonArray();
    }
    if(error) error->clear();
    return document.array();
}

QString NotAvailable(const QString &api){
    return QStringLiteral("chrome.%1 is not available in this browser").arg(api);
}

namespace {

void Arrange(const Bookmark &node, QList<Bookmark> *into){
    foreach(const Bookmark &child, node.children){
        if(child.told){
            Bookmark shown = child;
            shown.children.clear();
            if(child.folder) Arrange(child, &shown.children);
            into->append(shown);
        } else if(child.folder){
            Arrange(child, into);
        }
    }
}

QJsonObject DescribeBookmark(const Bookmark &node, const QString &parentId, int index){
    QJsonObject out;
    const QString id = parentId.isEmpty() ? QStringLiteral("0") : QString::number(node.id);
    out[QStringLiteral("id")] = id;
    if(!parentId.isEmpty()){
        out[QStringLiteral("parentId")] = parentId;
        out[QStringLiteral("index")] = index;
    }
    out[QStringLiteral("title")] = parentId.isEmpty() ? QString() : node.title;
    if(node.added > 0) out[QStringLiteral("dateAdded")] = static_cast<double>(node.added);
    if(node.folder){
        QJsonArray children;
        for(int i = 0; i < node.children.size(); i++)
            children.append(DescribeBookmark(node.children.at(i), id, i));
        out[QStringLiteral("children")] = children;
    } else {
        out[QStringLiteral("url")] = node.url.toString(QUrl::FullyEncoded);
    }
    return out;
}

QJsonArray Given(const QJsonArray &args){
    QJsonArray given = args;
    while(!given.isEmpty() && given.last().isNull()) given.removeLast();
    return given;
}

}

Bookmark Shown(const Bookmark &root){
    Bookmark shown = root;
    shown.told = true;
    shown.folder = true;
    shown.children.clear();
    Arrange(root, &shown.children);
    return shown;
}

QJsonObject BookmarkTree(const Call &call, const Bookmark &root){
    if(!call.error.isEmpty()) return Failure(call.error);
    if(!Given(call.args).isEmpty()) return Failure(QStringLiteral("bookmarks.getTree takes no argument"));
    return Success(QJsonArray() << DescribeBookmark(Shown(root), QString(), 0));
}

QJsonObject History(const Call &call, const QList<Tab> &tabs, qint64 now){
    if(!call.error.isEmpty()) return Failure(call.error);
    const QJsonArray args = Given(call.args);
    if(args.size() != 1 || !args.at(0).isObject())
        return Failure(QStringLiteral("history.search takes one object"));
    const QJsonObject query = args.at(0).toObject();

    if(!query.value(QStringLiteral("text")).isString())
        return Failure(QStringLiteral("history.search: 'text' is required"));
    double start = static_cast<double>(now) - 24.0 * 60 * 60 * 1000, end = 0;
    bool bounded = false;
    qint64 most = 100;
    for(QJsonObject::const_iterator it = query.constBegin(); it != query.constEnd(); ++it){
        const QString key = it.key();
        const QJsonValue value = it.value();
        if(key == QStringLiteral("text")) continue;
        const bool known = key == QStringLiteral("startTime") || key == QStringLiteral("endTime") || key == QStringLiteral("maxResults");
        if(!known) return Failure(QStringLiteral("history.search: '%1' is not a key of it").arg(key));
        if(value.isNull()) continue;
        bool ok = false;
        if(key == QStringLiteral("startTime") || key == QStringLiteral("endTime")){
            ok = value.isDouble() && qIsFinite(value.toDouble());
            if(key == QStringLiteral("startTime")) start = value.toDouble();
            else { end = value.toDouble(); bounded = true; }
        } else {
            most = Whole(value, &ok);
            ok = ok && most >= 0;
        }
        if(!ok) return Failure(QStringLiteral("history.search: '%1' has a value of the wrong kind").arg(key));
    }
    const QStringList words = query.value(QStringLiteral("text")).toString()
        .split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);

    struct Item { const Tab *tab; QString url; qint64 visited; };
    QList<Item> items;
    for(const Tab &tab : tabs){
        if(tab.url.isEmpty()) continue;
        const qint64 visited = VisitOf(tab);
        const double at = static_cast<double>(visited);
        if(at < start || (bounded && at >= end)) continue;
        const QString url = tab.url.toString(QUrl::FullyEncoded);
        bool all = true;
        foreach(const QString &word, words)
            all = all && (url.contains(word, Qt::CaseInsensitive) || tab.title.contains(word, Qt::CaseInsensitive));
        if(!all) continue;
        items.append(Item{&tab, url, visited});
    }
    std::stable_sort(items.begin(), items.end(), [](const Item &a, const Item &b){ return a.visited > b.visited; });

    QJsonArray found;
    foreach(const Item &item, items){
        if(most > 0 && found.size() >= most) break;
        found.append(HistoryItemOf(*item.tab));
    }
    return Success(found);
}

QJsonObject TopSites(const Call &call, const QList<Tab> &tabs){
    if(!call.error.isEmpty()) return Failure(call.error);
    if(!Given(call.args).isEmpty()) return Failure(QStringLiteral("topSites.get takes no argument"));

    struct Site { QString url; QString title; qint64 visited; };
    QList<Site> sites;
    QHash<QString, int> at;
    foreach(const Tab &tab, tabs){
        const QString scheme = tab.url.scheme();
        if(scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) continue;
        const QString url = tab.url.toString(QUrl::FullyEncoded);
        const qint64 visited = VisitOf(tab);
        const int known = at.value(url, -1);
        if(known < 0){
            at.insert(url, sites.size());
            sites.append(Site{url, tab.title, visited});
        } else if(visited > sites.at(known).visited){
            sites[known].title = tab.title;
            sites[known].visited = visited;
        }
    }
    std::stable_sort(sites.begin(), sites.end(), [](const Site &a, const Site &b){ return a.visited > b.visited; });

    QJsonArray found;
    foreach(const Site &site, sites){
        if(found.size() >= 10) break;
        QJsonObject one;
        one[QStringLiteral("url")] = site.url;
        one[QStringLiteral("title")] = site.title;
        found.append(one);
    }
    return Success(found);
}

namespace {

struct Placed { const Bookmark *node; QString id; QString parentId; int index; };

void Place(const Bookmark &node, const QString &id, const QString &parentId, int index, QList<Placed> *into){
    into->append(Placed{ &node, id, parentId, index });
    for(int i = 0; i < node.children.size(); i++)
        Place(node.children.at(i), QString::number(node.children.at(i).id), id, i, into);
}

QJsonObject DescribeShallow(const Placed &placed){
    QJsonObject out = DescribeBookmark(*placed.node, placed.parentId, placed.index);
    out.remove(QStringLiteral("children"));
    return out;
}

qint64 IdOf(const QJsonValue &value, bool *ok){
    *ok = false;
    if(!value.isString()) return -1;
    const QString text = value.toString();
    if(text.isEmpty() || text.size() > 18) return -1;
    foreach(const QChar &c, text) if(c < QLatin1Char('0') || c > QLatin1Char('9')) return -1;
    *ok = true;
    return text.toLongLong();
}

const Placed *Find(const QList<Placed> &placed, qint64 id){
    if(id == 0) return &placed.first();
    for(int i = 1; i < placed.size(); i++)
        if(placed.at(i).node->id == id) return &placed.at(i);
    return nullptr;
}

QString NoSuchBookmark(){ return QStringLiteral("Can't find bookmark for id."); }
QString BadBookmarkId(){ return QStringLiteral("Bookmark id is invalid."); }

bool HasWords(const Bookmark &node, const QStringList &words){
    const QString url = node.url.toString(QUrl::FullyEncoded);
    foreach(const QString &word, words)
        if(!node.title.contains(word, Qt::CaseInsensitive) && !url.contains(word, Qt::CaseInsensitive)) return false;
    return true;
}

}

bool IsBookmarksRead(const QString &api){
    return api == QStringLiteral("bookmarks.get") || api == QStringLiteral("bookmarks.getChildren")
        || api == QStringLiteral("bookmarks.getSubTree") || api == QStringLiteral("bookmarks.search");
}

QJsonObject Bookmarks(const Call &call, const Bookmark &root){
    if(!call.error.isEmpty()) return Failure(call.error);
    if(!IsBookmarksRead(call.api)) return Failure(NotAvailable(call.api));
    const QJsonArray args = Given(call.args);
    if(args.size() != 1) return Failure(QStringLiteral("%1 takes one argument").arg(call.api));
    const QJsonValue given = args.at(0);
    const Bookmark shown = Shown(root);
    QList<Placed> placed;
    Place(shown, QStringLiteral("0"), QString(), 0, &placed);

    if(call.api == QStringLiteral("bookmarks.search")){
        QStringList words;
        QString url, title;
        bool byUrl = false, byTitle = false;
        if(given.isString()){
            words = given.toString().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if(words.isEmpty()) return Success(QJsonArray());
        } else if(given.isObject()){
            const QJsonObject query = given.toObject();
            for(QJsonObject::const_iterator it = query.constBegin(); it != query.constEnd(); ++it){
                const QString key = it.key();
                const QJsonValue value = it.value();
                const bool known = key == QStringLiteral("query") || key == QStringLiteral("url") || key == QStringLiteral("title");
                if(!known) return Failure(QStringLiteral("bookmarks.search: '%1' is not a key of it").arg(key));
                if(value.isNull()) continue;
                if(!value.isString()) return Failure(QStringLiteral("bookmarks.search: '%1' has a value of the wrong kind").arg(key));
                if(key == QStringLiteral("query")){
                    words = value.toString().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                    if(words.isEmpty()) return Success(QJsonArray());
                } else if(key == QStringLiteral("url")){
                    byUrl = true;
                    url = QUrl(value.toString()).toString(QUrl::FullyEncoded);
                } else {
                    byTitle = true;
                    title = value.toString();
                }
            }
        } else {
            return Failure(QStringLiteral("bookmarks.search takes words or an object"));
        }
        QJsonArray found;
        for(int i = 1; i < placed.size(); i++){
            const Bookmark &node = *placed.at(i).node;
            if(!HasWords(node, words)) continue;
            if(byUrl && (node.folder || node.url.toString(QUrl::FullyEncoded) != url)) continue;
            if(byTitle && node.title != title) continue;
            found.append(DescribeShallow(placed.at(i)));
        }
        return Success(found);
    }

    if(call.api == QStringLiteral("bookmarks.get")){
        QJsonArray ids;
        if(given.isArray()) ids = given.toArray();
        else if(given.isString()) ids.append(given);
        else return Failure(QStringLiteral("bookmarks.get takes an id or a list of ids"));
        QJsonArray found;
        foreach(const QJsonValue &one, ids){
            bool ok = false;
            const qint64 id = IdOf(one, &ok);
            if(!ok) return Failure(BadBookmarkId());
            const Placed *node = Find(placed, id);
            if(!node) return Failure(NoSuchBookmark());
            found.append(DescribeShallow(*node));
        }
        return Success(found);
    }

    bool ok = false;
    const qint64 id = IdOf(given, &ok);
    if(!ok) return Failure(BadBookmarkId());
    const Placed *node = Find(placed, id);
    if(!node) return Failure(NoSuchBookmark());
    if(call.api == QStringLiteral("bookmarks.getSubTree"))
        return Success(QJsonArray() << DescribeBookmark(*node->node, node->parentId, node->index));
    QJsonArray children;
    for(int i = 0; i < node->node->children.size(); i++)
        children.append(DescribeShallow(Placed{ &node->node->children.at(i), QString::number(node->node->children.at(i).id), node->id, i }));
    return Success(children);
}

QJsonObject FontList(const Call &call, const QStringList &families){
    if(!call.error.isEmpty()) return Failure(call.error);
    if(!Given(call.args).isEmpty()) return Failure(QStringLiteral("fontSettings.getFontList takes no argument"));
    QJsonArray fonts;
    QSet<QString> said;
    foreach(const QString &given, families){
        QString family = given;
        const int bracket = family.indexOf(QStringLiteral(" ["));
        if(bracket > 0 && family.endsWith(QLatin1Char(']'))) family.truncate(bracket);
        if(family.isEmpty() || said.contains(family)) continue;
        said.insert(family);
        QJsonObject font;
        font[QStringLiteral("fontId")] = family;
        font[QStringLiteral("displayName")] = family;
        fonts.append(font);
    }
    return Success(fonts);
}

}
