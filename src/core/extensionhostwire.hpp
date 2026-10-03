#ifndef EXTENSIONHOSTWIRE_HPP
#define EXTENSIONHOSTWIRE_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <functional>

namespace ExtensionHostWire {

const char SCHEME[] = "vanilla-extension";
const char KEY_HEADER[] = "X-Vanilla-Key";
const char CALL_HEADER[] = "X-Vanilla-Call";
const int CALL_LIMIT = 16 * 1024;
const qint64 BODY_LIMIT = 4 * 1024 * 1024;
bool TakesBody(const QString &api);
QJsonArray ArgsOfBody(const QByteArray &body, QString *error);
const char KEY_PLACE[] = "__VANILLA_HOST_KEY__";

const char NONCE_HEADER[] = "X-Vanilla-Nonce";
const char VIEW_HEADER[] = "X-Vanilla-View";
const int BIND_LIMIT = 128;

bool IsCallUrl(const QUrl &url);
bool IsBindUrl(const QUrl &url);
bool IsUserScriptMessageUrl(const QUrl &url);
const char WORLD_HEADER[] = "X-Vanilla-World";
bool WithinHostPermissions(const QUrl &url, const QStringList &hostPermissions);
QByteArray WorldSecret(const QByteArray &processSecret, const QString &profileSpace, const QString &extensionId,
                       const QString &folder, const QString &worldId);
bool IsBindRequest(const QByteArray &method, const QByteArray &nonce);
QByteArray HeaderOf(const QMap<QByteArray, QByteArray> &headers, const char *name);

QString ExtensionIdOf(const QUrl &initiator);

QByteArray SecretOf(const QByteArray &fileBytes);
QByteArray KeyFor(const QByteArray &secret, const QString &id);
bool SameKey(const QByteArray &a, const QByteArray &b);

QString Admit(const QByteArray &method, const QUrl &initiator, const QByteArray &key,
              const QSet<QString> &shimmed, const QByteArray &secret);

struct Call {
    QString error;
    QString api;
    QJsonArray args;
};
Call ParseCall(const QByteArray &header);

struct Tab {
    qint64 id = 0;
    QUrl url;
    QString title;
    bool active = false;
    bool discarded = false;
    bool audible = false;
    bool muted = false;
    qint64 added = 0;
    qint64 visited = 0;
    bool loading = false;
    quint64 loads = 0;
    double zoom = 0;
};

struct Sight {
    bool tabs = false;
    QString self;
    QStringList hosts;
    bool history = false;

    static Sight All(){ Sight sight; sight.tabs = true; return sight; }
    static Sight Of(bool tabs, const QString &self, const QStringList &patterns);
    bool Any() const { return tabs || !hosts.isEmpty(); }
    bool Sees(const QUrl &url) const;
};

QString SpaceOfProfileKey(const QString &profileKey);

bool NodeIsOfProfile(const QStringList &settings, const QString &space, const QString &profileSpace);

enum class TabOwner { Asking, Other, NotYet };
bool TabIsTold(TabOwner owner, const std::function<bool()> &directoryIsOfProfile);

bool IsNonce(const QByteArray &nonce);
QByteArray Stamp(const QByteArray &processSecret, quint64 view);
quint64 ViewOfStamp(const QByteArray &processSecret, const QByteArray &stamp);

class BindTable {
public:
    bool Bind(quint64 view, const QByteArray &nonce);
    quint64 ViewOf(const QByteArray &nonce);
    void Forget(quint64 view);
    int Count() const { return m_Views.size(); }
private:
    QHash<QByteArray, quint64> m_Views;
    QHash<quint64, QList<QByteArray> > m_Nonces;
};

QByteArray NonceOfTabOf(const Call &call);
QJsonObject TabOfAnswer(qint64 id, int index);

QJsonObject InstalledAnswer(const QJsonObject &details);
QJsonObject StartupAnswer(bool owed);

class MenuPicks {
public:
    static const int KEPT = 8;
    static const qint64 FRESH = 10000;
    void Push(qint64 tab, const QString &pageUrl, qint64 now);
    qint64 Take(const QString &pageUrl, qint64 now);
    int Count() const { return m_Picks.size(); }
private:
    struct Pick { qint64 tab; QString pageUrl; qint64 at; };
    QList<Pick> m_Picks;
};

struct ZoomCall {
    bool set = false;
    qint64 tab = 0;
    double factor = 0;
    QString error;
};
ZoomCall ParseZoom(const Call &call);
QJsonObject ZoomAnswer(double factor);

QString PageUrlOfMenuTab(const Call &call, bool *ok);

struct AuthFlow {
    QUrl url;
    bool interactive = false;
    bool abortOnLoad = true;
    int timeoutMs = 60000;
    QString error;
};
AuthFlow ParseAuthFlow(const Call &call);
bool IsAuthRedirect(const QString &id, const QUrl &url);
extern const char AUTH_NOT_APPROVED[];
extern const char AUTH_INTERACTION[];
extern const char AUTH_NOT_LOADED[];

struct Capture {
    QByteArray format = "jpeg";
    int quality = 92;
    QString error;
};
Capture ParseCapture(const Call &call);
QString CaptureRefusal(const QUrl &page, const QString &id, const QStringList &hostPermissions, bool granted);
bool CaptureAllowedAt(qint64 last, qint64 now);
const char CAPTURE_QUOTA[] = "This request exceeds the MAX_CAPTURE_VISIBLE_TAB_CALLS_PER_SECOND quota.";
class ActiveTabs {
public:
    static const int KEPT = 16;
    void Grant(const QString &id, qint64 tab, quint64 load);
    bool Granted(const QString &id, qint64 tab, quint64 load) const;
    void Forget(const QString &id);
    QStringList Ids() const;
private:
    QHash<QString, QList<QPair<qint64, quint64> > > m_Tabs;
};

bool Matches(const QString &pattern, const QUrl &url, bool *ok);

struct Window {
    bool focused = false;
    QString state = QStringLiteral("normal");
    int left = 0, top = 0, width = 0, height = 0;
};

QJsonObject Answer(const Call &call, const QList<Tab> &tabs, const Sight &sight, const Window *window = nullptr);
QString NotAvailable(const QString &api);

struct Bookmark {
    qint64 id = 0;
    bool folder = false;
    bool told = false;
    QString title;
    QUrl url;
    qint64 added = 0;
    QList<Bookmark> children;
};
Bookmark Shown(const Bookmark &root);
QJsonObject BookmarkTree(const Call &call, const Bookmark &root);
bool IsBookmarksRead(const QString &api);
QJsonObject Bookmarks(const Call &call, const Bookmark &root);

QJsonObject FontList(const Call &call, const QStringList &families);
QJsonObject History(const Call &call, const QList<Tab> &tabs, qint64 now);

QJsonObject TopSites(const Call &call, const QList<Tab> &tabs);

struct Act {
    enum Kind { None, Create, Update, Remove, Reload, Search, Restore, Duplicate, Move };
    Kind kind = None;
    QString error;

    bool current = false;
    qint64 tab = 0;
    QList<qint64> tabs;

    bool hasUrl = false;
    QUrl url;
    bool activate = false;
    qint64 opener = 0;
    bool hasMuted = false, muted = false;
    bool bypassCache = false;
    QString text;
    bool newTab = false;
    qint64 index = -1;
    bool many = false;
};

bool IsAct(const QString &api);
Act ParseAct(const Call &call, const QString &extensionId);

QUrl TabUrlOf(const QString &extensionId, const QString &text, bool *ok);
QUrl PopupWindowUrlOf(const QString &extensionId, const QUrl &shown, const QString &text, bool *ok);
QUrl OffscreenUrlOf(const QString &extensionId, const QString &text, bool *ok);

QJsonObject Done();
QJsonObject TabAnswer(qint64 id, const QList<Tab> &tabs, const Sight &sight);
QJsonObject Refused(const QString &error);
QJsonObject Accepted(const QJsonValue &value);
QString NoTab(qint64 id);
QString NoCurrentWindow();
QString NotEditable();

QString WhyNotSearchable(const QString &format, const QUrl &built);

QJsonObject SessionAnswer(qint64 lastModified, qint64 id, const QList<Tab> &tabs, const Sight &sight);
QJsonObject TabsAnswer(const QList<qint64> &ids, const QList<Tab> &tabs, const Sight &sight);

QByteArray TokenOfEvents(const Call &call);
QSet<QString> NamesOfEvents(const Call &call);
QByteArray TokenOfEventNames(const Call &call);
qint64 NumberOfEventNames(const Call &call);
QByteArray TokenOfAbort(const Call &call);
QJsonArray Diff(const QList<Tab> &before, const QList<Tab> &after,
                bool focusedBefore, bool focusedAfter, const Sight &sight);
bool SameOrder(const QList<Tab> &before, const QList<Tab> &after);
QJsonArray Greeting(const QList<Tab> &tabs);
QJsonObject FiredEvent(const QString &name, const QJsonArray &args);
QJsonObject EventsAnswer(const QJsonArray &events, const QList<Tab> &tabs);
QJsonObject StaleAnswer();
QJsonObject AbortedAnswer();
QString TooManySubscribers();

const int SUBSCRIBERS_OF_ONE = 4;
const int SUBSCRIBERS_KEPT = 64;
const qint64 SUBSCRIBER_IDLE_MS = 60 * 1000;
const qint64 SUBSCRIBER_BETWEEN_MS = 2000;
const int SUBSCRIBER_QUEUE = 32;

template <class Waiter>
class Subscribers {
public:
    struct Entry {
        QString extension;
        QByteArray token;
        Waiter waiter = Waiter();
        bool holding = false;
        qint64 seen = 0;
        bool told = false;
        QList<Tab> tabs;
        bool focused = false;
        QList<QJsonObject> queued;
        QSet<QString> names;
        qint64 listed = 0;
    };
    enum Taken { Held, Replaced, Refused };

    int Count() const { return m_Entries.size(); }
    const QList<Entry> &Entries() const { return m_Entries; }

    template <class Alive>
    Taken Take(const QString &extension, const QByteArray &token, const Waiter &waiter,
               qint64 now, Alive alive, Waiter *old,
               const QSet<QString> &names = QSet<QString>(), bool resume = false, qint64 listed = 0){
        Prune(alive, now);
        if(Retired(extension, token)) return Refused;
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &entry = m_Entries[i];
            if(entry.extension != extension || entry.token != token) continue;
            const bool replaced = entry.holding && alive(entry.waiter);
            if(replaced && old) *old = entry.waiter;
            entry.waiter = waiter;
            entry.holding = true;
            entry.seen = now;
            Names(extension, token, names, listed);
            return replaced ? Replaced : Held;
        }
        int ofThisOne = 0;
        foreach(const Entry &entry, m_Entries)
            if(entry.extension == extension) ofThisOne++;
        if(ofThisOne >= SUBSCRIBERS_OF_ONE || m_Entries.size() >= SUBSCRIBERS_KEPT) return Refused;
        Entry entry;
        entry.extension = extension;
        entry.token = token;
        entry.waiter = waiter;
        entry.holding = true;
        entry.seen = now;
        entry.names = names;
        entry.listed = listed;
        const auto last = m_Last.constFind(extension);
        if(resume && last != m_Last.constEnd() && last.value().told && !Holding(extension, alive)){
            entry.told = true;
            entry.tabs = last.value().tabs;
            entry.focused = last.value().focused;
            int from = -1;
            for(int i = 0; i < m_Entries.size(); i++){
                if(m_Entries.at(i).extension != extension) continue;
                if(from < 0 || m_Entries.at(from).token != last.value().token) from = i;
            }
            if(from >= 0)
                foreach(const QJsonObject &event, m_Entries.at(from).queued)
                    if(event.value(QStringLiteral("name")) != QJsonValue(QStringLiteral("runtime.onUserScriptMessage"))
                       && entry.queued.size() < SUBSCRIBER_QUEUE) entry.queued.append(event);
            for(int i = m_Entries.size() - 1; i >= 0; i--){
                if(m_Entries.at(i).extension != extension) continue;
                QList<QByteArray> &gone = m_Retired[extension];
                gone.append(m_Entries.takeAt(i).token);
                while(gone.size() > SUBSCRIBERS_KEPT) gone.removeFirst();
            }
        }
        m_Entries.append(entry);
        Last &kept = m_Last[extension];
        kept.token = token;
        kept.names = names;
        kept.listed = listed;
        return Held;
    }

    void Names(const QString &extension, const QByteArray &token, const QSet<QString> &names, qint64 listed = 0){
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &entry = m_Entries[i];
            if(entry.extension != extension || entry.token != token || (listed && listed <= entry.listed)) continue;
            entry.names = names;
            entry.listed = listed;
        }
        auto last = m_Last.find(extension);
        if(last == m_Last.end() || last.value().token != token || (listed && listed <= last.value().listed)) return;
        last.value().names = names;
        last.value().listed = listed;
    }
    QSet<QString> NamesOf(const QString &extension) const { return m_Last.value(extension).names; }
    template <class Alive>
    bool Holding(const QString &extension, Alive alive) const {
        foreach(const Entry &entry, m_Entries)
            if(entry.extension == extension && entry.holding && alive(entry.waiter)) return true;
        return false;
    }
    bool Watches() const {
        for(auto it = m_Last.constBegin(); it != m_Last.constEnd(); ++it)
            if(it.value().told && DiffNamed(it.value().names)) return true;
        return false;
    }
    QStringList Watched() const {
        QStringList out;
        for(auto it = m_Last.constBegin(); it != m_Last.constEnd(); ++it)
            if(it.value().told && DiffNamed(it.value().names)) out << it.key();
        return out;
    }
    template <class Alive, class Sees>
    QStringList Watch(const QList<Tab> &tabs, bool focused, qint64 now, Alive alive, Sees sees){
        Prune(alive, now);
        QStringList wake;
        for(auto it = m_Last.begin(); it != m_Last.end(); ++it){
            Last &last = it.value();
            if(!last.told || !DiffNamed(last.names) || Holding(it.key(), alive)) continue;
            const QJsonArray events = Diff(last.tabs, tabs, last.focused, focused, sees(it.key()));
            bool heard = false;
            foreach(const QJsonValue &event, events)
                if(last.names.contains(event.toObject().value(QStringLiteral("name")).toString())) heard = true;
            if(heard){ wake << it.key(); continue; }
            last.tabs = tabs;
            last.focused = focused;
        }
        return wake;
    }
    bool Retired(const QString &extension, const QByteArray &token) const {
        return m_Retired.value(extension).contains(token);
    }
    QStringList Kept() const { return m_Last.keys(); }
    void Forget(const QString &extension){ m_Last.remove(extension); m_Retired.remove(extension); }

    bool Abort(const QString &extension, const QByteArray &token, Waiter *waiter){
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &entry = m_Entries[i];
            if(entry.extension != extension || entry.token != token || !entry.holding) continue;
            if(waiter) *waiter = entry.waiter;
            entry.waiter = Waiter();
            entry.holding = false;
            return true;
        }
        return false;
    }

    template <class Alive>
    QSet<QByteArray> FireTo(const QString &extension, const QSet<QByteArray> &tokens, const QJsonObject &event,
                            Alive alive, qint64 now){
        Prune(alive, now);
        QSet<QByteArray> told;
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &entry = m_Entries[i];
            if(entry.extension != extension || !tokens.contains(entry.token)) continue;
            if(entry.queued.size() >= SUBSCRIBER_QUEUE) continue;
            entry.queued.append(event);
            told.insert(entry.token);
        }
        return told;
    }
    template <class Alive>
    bool Listening(const QString &extension, Alive alive, qint64 now) const {
        foreach(const Entry &entry, m_Entries){
            if(entry.extension != extension) continue;
            if(entry.holding ? alive(entry.waiter) : now - entry.seen < SUBSCRIBER_BETWEEN_MS) return true;
        }
        return false;
    }
    bool Has(const QString &extension, const QByteArray &token) const {
        foreach(const Entry &entry, m_Entries)
            if(entry.extension == extension && entry.token == token) return true;
        return false;
    }

    bool Fire(const QString &extension, const QJsonObject &event){
        bool any = false;
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &entry = m_Entries[i];
            if(entry.extension != extension) continue;
            any = true;
            if(entry.queued.size() >= SUBSCRIBER_QUEUE){
                for(int j = 0; j < entry.queued.size(); j++){
                    if(entry.queued.at(j).value(QStringLiteral("name")) == QJsonValue(QStringLiteral("runtime.onUserScriptMessage"))) continue;
                    entry.queued.removeAt(j);
                    break;
                }
            }
            entry.queued.append(event);
        }
        return any;
    }

    template <class Alive>
    void Prune(Alive alive, qint64 now){
        QList<Entry> gone;
        for(int i = m_Entries.size() - 1; i >= 0; i--){
            const Entry &entry = m_Entries.at(i);
            const bool going = entry.holding ? !alive(entry.waiter)
                                             : now - entry.seen > SUBSCRIBER_IDLE_MS;
            if(going) gone.append(m_Entries.takeAt(i));
        }
    }

    template <class Alive>
    bool AnyWaiting(Alive alive) const {
        foreach(const Entry &entry, m_Entries)
            if(entry.holding && alive(entry.waiter)) return true;
        return false;
    }

    template <class Alive, class Sees, class Answer>
    void Publish(const QList<Tab> &tabs, bool focused, qint64 now, Alive alive, Sees sees, Answer answer){
        Prune(alive, now);
        QList<QPair<Waiter, QJsonObject> > answers;
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &entry = m_Entries[i];
            if(!entry.holding) continue;
            QJsonArray events = entry.told
                ? Diff(entry.tabs, tabs, entry.focused, focused, sees(entry.extension))
                : Greeting(tabs);
            foreach(const QJsonObject &fired, entry.queued) events.append(fired);
            entry.queued.clear();
            const bool moved = entry.told && !SameOrder(entry.tabs, tabs);
            entry.told = true;
            entry.tabs = tabs;
            entry.focused = focused;
            Last &last = m_Last[entry.extension];
            last.told = true;
            last.tabs = tabs;
            last.focused = focused;
            if(events.isEmpty() && !moved) continue;
            const Waiter waiter = entry.waiter;
            entry.waiter = Waiter();
            entry.holding = false;
            entry.seen = now;
            answers.append(qMakePair(waiter, EventsAnswer(events, tabs)));
        }
        for(int i = 0; i < answers.size(); i++)
            answer(answers.at(i).first, answers.at(i).second);
    }

private:
    struct Last {
        QByteArray token;
        QSet<QString> names;
        qint64 listed = 0;
        bool told = false;
        QList<Tab> tabs;
        bool focused = false;
    };
    static bool DiffNamed(const QSet<QString> &names){
        foreach(const QString &name, names)
            if(name.startsWith(QStringLiteral("tabs.")) || name.startsWith(QStringLiteral("history."))
               || name == QStringLiteral("windows.onFocusChanged")) return true;
        return false;
    }
    QList<Entry> m_Entries;
    QHash<QString, Last> m_Last;
    QHash<QString, QList<QByteArray> > m_Retired;
};

class UserScriptListeners {
public:
    template <class There>
    void Name(const QString &extension, const QByteArray &token, qint64 now, There there){
        Prune(now, there);
        for(int i = 0; i < m_Entries.size(); i++)
            if(m_Entries.at(i).extension == extension && m_Entries.at(i).token == token){ m_Entries[i].since = now; return; }
        int of = 0, oldest = -1;
        for(int i = 0; i < m_Entries.size(); i++){
            if(m_Entries.at(i).extension != extension) continue;
            of++;
            if(oldest < 0 || m_Entries.at(i).since < m_Entries.at(oldest).since) oldest = i;
        }
        if(of >= SUBSCRIBERS_OF_ONE && oldest >= 0) m_Entries.removeAt(oldest);
        Entry e;
        e.extension = extension; e.token = token; e.since = now;
        m_Entries.append(e);
    }
    template <class There>
    void Prune(qint64 now, There there){
        for(int i = m_Entries.size() - 1; i >= 0; i--){
            Entry &e = m_Entries[i];
            if(there(e.extension, e.token)){ e.seen = true; continue; }
            if(e.seen || now - e.since > SUBSCRIBER_IDLE_MS) m_Entries.removeAt(i);
        }
    }
    QSet<QByteArray> Of(const QString &extension) const {
        QSet<QByteArray> out;
        foreach(const Entry &e, m_Entries) if(e.extension == extension) out.insert(e.token);
        return out;
    }
private:
    struct Entry {
        QString extension;
        QByteArray token;
        qint64 since = 0;
        bool seen = false;
    };
    QList<Entry> m_Entries;
};

const int WAKE_QUEUE = 16;
const int WAKE_TRIES = 3;
const qint64 WAKE_KEEP = 15 * 1000;
const qint64 WAKE_WAIT = 10 * 1000;
const qint64 WAKE_REST = 60 * 1000;
class Wakes {
public:
    QList<QJsonObject> Subscribed(const QString &extension, qint64 now){
        Of &of = m_Of[extension];
        of.waking = false;
        of.failures = 0;
        QList<QJsonObject> out;
        foreach(const Held &h, of.held) if(now - h.since <= WAKE_KEEP) out << h.event;
        of.held.clear();
        return out;
    }
    bool Hold(const QString &extension, const QJsonObject &event, qint64 now){
        if(!m_Of.contains(extension)) return false;
        Of &of = m_Of[extension];
        if(now < of.restUntil) return false;
        Drop(of, now);
        if(of.held.size() >= WAKE_QUEUE) of.held.removeFirst();
        of.held.append(Held{ event, now });
        return true;
    }
    bool ShouldWake(const QString &extension, qint64 now, bool waiting = false) const {
        if(!m_Of.contains(extension)) return false;
        const Of &of = m_Of.value(extension);
        return (!of.held.isEmpty() || waiting) && !of.waking && now >= of.restUntil;
    }
    bool Resting(const QString &extension, qint64 now) const {
        return m_Of.contains(extension) && now < m_Of.value(extension).restUntil;
    }
    qint64 RestLeft(const QString &extension, qint64 now) const {
        return Resting(extension, now) ? m_Of.value(extension).restUntil - now : 0;
    }
    void Woke(const QString &extension, qint64 now){
        if(!m_Of.contains(extension)) return;
        Of &of = m_Of[extension];
        of.waking = true;
        of.since = now;
    }
    bool Waking(const QString &extension) const { return m_Of.value(extension).waking; }
    QStringList Extensions() const { return m_Of.keys(); }
    bool AnyWaking() const {
        for(auto it = m_Of.constBegin(); it != m_Of.constEnd(); ++it) if(it.value().waking) return true;
        return false;
    }
    QStringList Expire(qint64 now){
        QStringList out;
        for(auto it = m_Of.begin(); it != m_Of.end(); ++it){
            Of &of = it.value();
            Drop(of, now);
            if(!of.waking || now - of.since < WAKE_WAIT) continue;
            of.waking = false;
            out << it.key();
            if(++of.failures >= WAKE_TRIES){
                of.failures = 0;
                of.restUntil = now + WAKE_REST;
                of.held.clear();
            }
        }
        return out;
    }
    void Forget(const QString &extension){
        if(!m_Of.contains(extension)) return;
        Of &of = m_Of[extension];
        of.held.clear();
        of.waking = false;
    }
private:
    struct Held {
        QJsonObject event;
        qint64 since;
    };
    struct Of {
        bool waking = false;
        qint64 since = 0;
        int failures = 0;
        qint64 restUntil = 0;
        QList<Held> held;
    };
    static void Drop(Of &of, qint64 now){
        while(!of.held.isEmpty() && now - of.held.first().since > WAKE_KEEP) of.held.removeFirst();
    }
    QHash<QString, Of> m_Of;
};

const int USER_SCRIPT_PENDING = 64;
const qint64 USER_SCRIPT_WAIT = 5 * 60 * 1000;
QString NoReceiver();
QString PortClosed();
template <class Waiter>
class UserScriptMessages {
public:
    enum Kind { Value, NoAnswer, NoListener };
    struct Done {
        Waiter waiter;
        QJsonObject answer;
        QString unheardOf = QString();
    };
    bool Add(const QByteArray &ticket, const QString &extension, const Waiter &waiter,
             const QSet<QByteArray> &tokens, qint64 now){
        int of = 0;
        foreach(const Entry &e, m_Entries) if(e.extension == extension) of++;
        if(of >= USER_SCRIPT_PENDING || tokens.isEmpty()) return false;
        Entry e;
        e.ticket = ticket; e.extension = extension; e.waiter = waiter; e.left = tokens; e.since = now;
        m_Entries.append(e);
        return true;
    }
    bool Wait(const QByteArray &ticket, const QString &extension, const Waiter &waiter, const QJsonObject &event, qint64 now){
        int of = 0;
        foreach(const Entry &e, m_Entries) if(e.extension == extension) of++;
        if(of >= USER_SCRIPT_PENDING) return false;
        Entry e;
        e.ticket = ticket; e.extension = extension; e.waiter = waiter; e.since = now;
        e.unaddressed = true; e.event = event;
        m_Entries.append(e);
        return true;
    }
    QList<QPair<QByteArray, QJsonObject> > Waiting(const QString &extension) const {
        QList<QPair<QByteArray, QJsonObject> > out;
        foreach(const Entry &e, m_Entries) if(e.extension == extension && e.unaddressed) out << qMakePair(e.ticket, e.event);
        return out;
    }
    bool HasWaiting(const QString &extension) const {
        foreach(const Entry &e, m_Entries) if(e.extension == extension && e.unaddressed) return true;
        return false;
    }
    void Addressed(const QByteArray &ticket, const QByteArray &token, qint64 now){
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &e = m_Entries[i];
            if(e.ticket != ticket || !e.unaddressed) continue;
            e.unaddressed = false;
            e.event = QJsonObject();
            e.left = QSet<QByteArray>() << token;
            e.since = now;
            return;
        }
    }
    QList<Done> Reply(const QString &extension, const QByteArray &ticket, const QByteArray &token,
                      Kind kind, const QJsonValue &value){
        QList<Done> out;
        for(int i = 0; i < m_Entries.size(); i++){
            Entry &e = m_Entries[i];
            if(e.ticket != ticket || e.extension != extension || !e.left.contains(token)) continue;
            e.left.remove(token);
            if(kind == Value){ out << Done{ e.waiter, Accepted(value) }; m_Entries.removeAt(i); return out; }
            if(kind == NoAnswer) e.heard = true;
            if(e.left.isEmpty()){ out << Done{ e.waiter, Ended(e.heard) }; m_Entries.removeAt(i); }
            return out;
        }
        return out;
    }
    template <class There, class Alive>
    QList<Done> Sweep(qint64 now, There there, Alive alive){
        QList<Done> out;
        for(int i = m_Entries.size() - 1; i >= 0; i--){
            Entry &e = m_Entries[i];
            if(!alive(e.waiter)){ m_Entries.removeAt(i); continue; }
            if(e.unaddressed){
                if(now - e.since > WAKE_KEEP){ out << Done{ e.waiter, Refused(NoReceiver()), e.extension }; m_Entries.removeAt(i); }
                continue;
            }
            foreach(const QByteArray &token, QSet<QByteArray>(e.left))
                if(!there(e.extension, token)){ e.left.remove(token); e.heard = true; }
            if(now - e.since > USER_SCRIPT_WAIT){ out << Done{ e.waiter, Refused(PortClosed()) }; m_Entries.removeAt(i); continue; }
            if(e.left.isEmpty()){ out << Done{ e.waiter, Ended(e.heard) }; m_Entries.removeAt(i); }
        }
        return out;
    }
    QList<Done> Drop(const QString &extension = QString()){
        QList<Done> out;
        for(int i = m_Entries.size() - 1; i >= 0; i--){
            if(!extension.isEmpty() && m_Entries.at(i).extension != extension) continue;
            out << Done{ m_Entries.at(i).waiter, Refused(PortClosed()) };
            m_Entries.removeAt(i);
        }
        return out;
    }
    int Count() const { return m_Entries.size(); }
    QStringList Extensions() const {
        QStringList out;
        foreach(const Entry &e, m_Entries) if(!out.contains(e.extension)) out << e.extension;
        return out;
    }
private:
    struct Entry {
        QByteArray ticket;
        QString extension;
        Waiter waiter;
        QSet<QByteArray> left;
        bool heard = false;
        qint64 since = 0;
        bool unaddressed = false;
        QJsonObject event;
    };
    static QJsonObject Ended(bool heard){
        if(!heard) return Refused(NoReceiver());
        QJsonObject o; o[QStringLiteral("ok")] = true; o[QStringLiteral("none")] = true; return o;
    }
    QList<Entry> m_Entries;
};

inline bool WaitForWorker(bool canWake, bool namedEver, bool resting, bool listening){
    return canWake && namedEver && !resting && !listening;
}

template <class Waiter, class Fire>
int DeliverWaiting(UserScriptMessages<Waiter> &messages, const QString &extension, const QByteArray &token,
                   bool named, bool subscribed, qint64 now, Fire fire){
    if(!named || !subscribed) return 0;
    int handed = 0;
    const QList<QPair<QByteArray, QJsonObject> > waiting = messages.Waiting(extension);
    for(int i = 0; i < waiting.size(); i++){
        if(!fire(waiting.at(i).second)) continue;
        messages.Addressed(waiting.at(i).first, token, now);
        handed++;
    }
    return handed;
}

template <class T>
class OneAtATime {
public:
    enum Outcome { Empty, Nested, Declined, Drained };
    explicit OneAtATime(int limit) : m_Limit(limit) {}
    bool Push(const T &item){
        if(m_Items.size() >= m_Limit) return false;
        m_Items.append(item);
        return true;
    }
    int Count() const { return m_Items.size(); }
    const T *First() const { return m_Items.isEmpty() ? nullptr : &m_Items.first(); }
    bool Reserve(){
        if(m_Reserved) return false;
        m_Reserved = true;
        m_Reservations++;
        return true;
    }
    void Fired(){ m_Reserved = false; }
    bool Reserved() const { return m_Reserved; }
    int Reservations() const { return m_Reservations; }
    template <class Keep>
    void Prune(Keep keep){
        if(m_Running) return;
        QList<T> gone;
        for(int i = m_Items.size() - 1; i >= 0; i--)
            if(!keep(m_Items.at(i))) gone.append(m_Items.takeAt(i));
    }
    template <class Run, class MayRun>
    Outcome Drain(Run run, MayRun mayRun){
        if(m_Running) return Nested;
        if(m_Items.isEmpty()) return Empty;
        Running running(&m_Running);
        while(!m_Items.isEmpty()){
            if(!mayRun()) return Declined;
            const T item = m_Items.takeFirst();
            run(item);
        }
        return Drained;
    }
private:
    struct Running {
        explicit Running(bool *flag) : m_Flag(flag) { *m_Flag = true; }
        ~Running(){ *m_Flag = false; }
        bool *m_Flag;
    };
    QList<T> m_Items;
    int m_Limit;
    bool m_Running = false;
    bool m_Reserved = false;
    int m_Reservations = 0;
};

}

#endif
