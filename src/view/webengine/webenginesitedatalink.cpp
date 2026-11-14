#include "switch.hpp"

#include "webenginesitedatalink.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>

#include <climits>

namespace {
    const qint64 ID_RANGE = Q_INT64_C(1) << 20;
    const int SLOTS = 1023;
    const int POLL_MSEC = 50;
    const int SEND_CUTOFF_MSEC = SiteDataLink::TIMEOUT_MSEC - 500;

    class HiddenDevToolsPage : public QWebEnginePage {
    public:
        explicit HiddenDevToolsPage(QWebEngineProfile *profile, QObject *parent)
            : QWebEnginePage(profile, parent){}
    protected:
        bool acceptNavigationRequest(const QUrl &url, NavigationType, bool isMainFrame) override {
            return isMainFrame && url == WebEngineSiteDataLink::HiddenPageUrl();
        }
    };

    QString Literal(const QString &text){
        return QString::fromUtf8(QJsonDocument(QJsonArray{ text }).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
    }

    const char *HUB_SCRIPT =
        "(function(key, floor, range, slots){"
        "  var hub = window.__vanillaSiteData;"
        "  if(!hub){"
        "    if(!window.DevToolsHost || !DevToolsHost.sendMessageToEmbedder) return false;"
        "    hub = window.__vanillaSiteData = { links: {}, chunk: null };"
        "    var api = window.DevToolsAPI || (window.DevToolsAPI = {});"
        "    var before = api.dispatchMessage, beforeChunk = api.dispatchMessageChunk;"
        "    var ours = function(id){ return typeof id === 'number' && id >= floor; };"
        "    var route = function(object){"
        "      for(var name in hub.links){"
        "        var link = hub.links[name];"
        "        if(object.id >= link.low && object.id < link.high){ link.queue.push(object); return; }"
        "      }"
        "    };"
        "    api.dispatchMessage = function(message){"
        "      try {"
        "        var object = typeof message === 'string' ? JSON.parse(message) : message;"
        "        if(object && ours(object.id)){ route(object); return; }"
        "      } catch(e){}"
        "      if(before) return before.apply(this, arguments);"
        "    };"
        "    api.dispatchMessageChunk = function(chunk, size){"
        "      if(size){"
        "        var start = /^\\{\"id\":(\\d+)/.exec(chunk);"
        "        hub.chunk = start && ours(Number(start[1])) ? '' : null;"
        "      }"
        "      if(hub.chunk === null){"
        "        if(beforeChunk) return beforeChunk.apply(this, arguments);"
        "        return;"
        "      }"
        "      hub.chunk += chunk;"
        "      try { var whole = JSON.parse(hub.chunk); hub.chunk = null; route(whole); } catch(e){}"
        "    };"
        "    hub.next = 0;"
        "    hub.send = function(name, text, deadline){"
        "      if(!hub.links[name] || performance.now() > deadline) return;"
        "      DevToolsHost.sendMessageToEmbedder(JSON.stringify({id: 0, method: 'dispatchProtocolMessage', params: [text]}));"
        "    };"
        "  }"
        "  for(var tried = 0; tried < slots; tried++){"
        "    var low = floor + ((hub.next + tried) % slots) * range, taken = false;"
        "    for(var other in hub.links) if(hub.links[other].low === low) taken = true;"
        "    if(taken) continue;"
        "    hub.next = (hub.next + tried + 1) % slots;"
        "    hub.links[key] = { low: low, high: low + range, queue: [] };"
        "    return low + ':' + performance.now();"
        "  }"
        "  return false;"
        "})(%1, %2, %3, %4)";
}

QUrl DevToolsSiteDataLink::HiddenPageUrl(){
    return QUrl(QStringLiteral("devtools://devtools/bundled/devtools_compatibility.js"));
}

DevToolsSiteDataLink::DevToolsSiteDataLink(QObject *parent)
    : SiteDataLink(parent)
    , m_Ready(false)
    , m_Broken(false)
    , m_Base(0)
    , m_Next(0)
    , m_PageClock(0)
{
    m_Key = QString::number(QRandomGenerator::global()->generate64(), 16);
    m_Clock.start();
    m_Poll.setInterval(POLL_MSEC);
    connect(&m_Poll, &QTimer::timeout, this, [this](){ Poll();});
}

DevToolsSiteDataLink::~DevToolsSiteDataLink(){
    Shutdown();
}

void DevToolsSiteDataLink::Shutdown(){
    m_Broken = true;
    m_Queued.clear();
    m_Waiting.clear();
    m_Poll.stop();
}

QString DevToolsSiteDataLink::ForgetScript() const {
    return QStringLiteral("window.__vanillaSiteData && delete window.__vanillaSiteData.links[%1]").arg(Literal(m_Key));
}

WebEngineSiteDataLink::WebEngineSiteDataLink(QWebEnginePage *page, QObject *parent)
    : DevToolsSiteDataLink(parent)
    , m_Page(page)
    , m_Hidden(nullptr)
{
    if(!m_Page){
        Break();
        return;
    }
    if(QWebEnginePage *existing = m_Page->devToolsPage()){
        m_Target = existing;
    } else {
        m_Hidden = new HiddenDevToolsPage(m_Page->profile(), nullptr);
        m_Target = m_Hidden;
    }
    if(!m_Hidden){
        Install();
        return;
    }
    connect(m_Hidden, &QWebEnginePage::loadFinished, this, [this](bool ok){
        if(Settled()) return;
        if(m_Hidden->url() != HiddenPageUrl()) return;
        if(!ok){
            Break();
            return;
        }
        Install();
    });
    m_Page->setDevToolsPage(m_Hidden);
    m_Hidden->load(HiddenPageUrl());
}

WebEngineSiteDataLink::~WebEngineSiteDataLink(){
    Shutdown();
    if(m_Hidden){
        delete m_Hidden;
    } else if(m_Target){
        Evaluate(ForgetScript(), Evaluated());
    }
}

void WebEngineSiteDataLink::Evaluate(const QString &script, Evaluated done){
    if(!m_Target) return;
    if(done) m_Target->runJavaScript(script, QWebEngineScript::MainWorld, done);
    else m_Target->runJavaScript(script, QWebEngineScript::MainWorld);
}

bool WebEngineSiteDataLink::FrontendAttached() const {
    return m_Page && m_Target && m_Page->devToolsPage() == m_Target;
}

bool DevToolsSiteDataLink::StillAttached() const {
    return !m_Broken && FrontendAttached();
}

void DevToolsSiteDataLink::Install(){
    if(m_Ready || m_Broken) return;
    if(!StillAttached()){
        Break();
        return;
    }
    const QString script = QString::fromLatin1(HUB_SCRIPT)
        .arg(Literal(m_Key)).arg(ID_FLOOR).arg(ID_RANGE).arg(SLOTS);
    QPointer<DevToolsSiteDataLink> self(this);
    Evaluate(script, [self](const QVariant &installed){
        if(!self || self->m_Broken) return;
        const QStringList given = installed.toString().split(QLatin1Char(':'));
        const qint64 low = given.value(0).toLongLong();
        bool clock = false;
        const double pageNow = given.value(1).toDouble(&clock);
        if(given.length() != 2 || !clock || low < ID_FLOOR){
            self->Break();
            return;
        }
        self->m_Base = low;
        self->m_PageClock = pageNow - self->m_Clock.elapsed();
        self->m_Ready = true;
        const QList<Pending> queued = self->m_Queued;
        self->m_Queued.clear();
        for(const Pending &pending : queued){
            if(!self) return;
            self->Send(pending);
        }
    });
}

void DevToolsSiteDataLink::Call(const QString &method, const QJsonObject &params, Answer done){
    Pending pending{ method, params, done, QElapsedTimer() };
    pending.asked.start();
    if(m_Broken){
        QTimer::singleShot(0, this, [done](){ done(false, QJsonObject());});
        return;
    }
    if(!m_Ready){
        m_Queued << pending;
        return;
    }
    Send(pending);
}

void DevToolsSiteDataLink::Send(const Pending &pending){
    if(!StillAttached() || pending.asked.elapsed() >= SEND_CUTOFF_MSEC){
        QTimer::singleShot(0, this, [done = pending.done](){ done(false, QJsonObject());});
        return;
    }
    const qint64 id = m_Base + (m_Next++ % ID_RANGE);
    QJsonObject command;
    command.insert(QStringLiteral("id"), id);
    command.insert(QStringLiteral("method"), pending.method);
    command.insert(QStringLiteral("params"), pending.params);
    const QString message = QString::fromUtf8(QJsonDocument(command).toJson(QJsonDocument::Compact));
    m_Waiting.insert(id, pending);
    const double deadline = m_PageClock + m_Clock.elapsed() + (SEND_CUTOFF_MSEC - pending.asked.elapsed());
    Evaluate(QStringLiteral("(function(){ var hub = window.__vanillaSiteData;"
                            " if(hub) hub.send(%1, %2, %3); })()")
             .arg(Literal(m_Key), Literal(message)).arg(deadline, 0, 'f', 0),
             Evaluated());
    if(!m_Poll.isActive()) m_Poll.start();
}

void DevToolsSiteDataLink::Poll(){
    QList<Answer> expired;
    for(auto it = m_Waiting.begin(); it != m_Waiting.end();){
        if(it->asked.elapsed() >= TIMEOUT_MSEC){
            expired << it->done;
            it = m_Waiting.erase(it);
        } else {
            ++it;
        }
    }
    QPointer<DevToolsSiteDataLink> alive(this);
    for(const Answer &done : expired){
        if(!alive) return;
        done(false, QJsonObject());
    }
    if(!alive) return;
    if(m_Waiting.isEmpty()){
        m_Poll.stop();
        return;
    }
    if(!StillAttached()){
        Break();
        return;
    }
    QPointer<DevToolsSiteDataLink> self(this);
    Evaluate(
        QStringLiteral("(function(){ var hub = window.__vanillaSiteData, link = hub && hub.links[%1];"
                       " if(!link) return 'gone'; var taken = JSON.stringify(link.queue); link.queue.length = 0; return taken; })()")
        .arg(Literal(m_Key)),
        [self](const QVariant &taken){
            if(!self || !taken.isValid()) return;
            if(taken.toString() == QStringLiteral("gone")){
                self->Break();
                return;
            }
            const QJsonArray answers = QJsonDocument::fromJson(taken.toString().toUtf8()).array();
            for(const QJsonValue &value : answers){
                if(!self) return;
                const QJsonObject answer = value.toObject();
                const qint64 id = static_cast<qint64>(answer.value(QStringLiteral("id")).toDouble());
                if(!self->m_Waiting.contains(id)) continue;
                const Answer done = self->m_Waiting.take(id).done;
                const bool ok = !answer.contains(QStringLiteral("error")) &&
                    answer.value(QStringLiteral("result")).isObject();
                done(ok, answer.value(ok ? QStringLiteral("result") : QStringLiteral("error")).toObject());
            }
        });
}

void DevToolsSiteDataLink::Break(){
    m_Broken = true;
    m_Poll.stop();
    QList<Answer> failed;
    for(const Pending &pending : std::as_const(m_Waiting)) failed << pending.done;
    m_Waiting.clear();
    for(const Pending &pending : std::as_const(m_Queued)) failed << pending.done;
    m_Queued.clear();
    QPointer<DevToolsSiteDataLink> self(this);
    for(const Answer &done : failed){
        if(!self) return;
        done(false, QJsonObject());
    }
}

namespace {
    const char *HIDDEN_VIEW_QML =
        "import QtQuick\n"
        "import QtWebEngine\n"
        "WebEngineView {\n"
        "    property url hiddenUrl\n"
        "    signal hiddenLoaded(bool ok)\n"
        "    signal callBackResult(int id, variant result)\n"
        "    onNavigationRequested: function(request){\n"
        "        if(!request.isMainFrame || request.url.toString() !== hiddenUrl.toString())\n"
        "            request.reject()\n"
        "    }\n"
        "    onLoadingChanged: function(info){\n"
        "        if(info.url.toString() !== hiddenUrl.toString()) return\n"
        "        if(info.status === WebEngineView.LoadSucceededStatus) hiddenLoaded(true)\n"
        "        else if(info.status === WebEngineView.LoadFailedStatus) hiddenLoaded(false)\n"
        "    }\n"
        "    function evaluateJavaScript(id, code){\n"
        "        runJavaScript(code, WebEngineScript.MainWorld, function(result){ callBackResult(id, result) })\n"
        "    }\n"
        "}\n";

    int NextEvaluation(){
        static int next = 0;
        next = next >= INT_MAX ? 1 : next + 1;
        return next;
    }

    QObject *DevToolsViewOf(QQuickItem *view){
        return view ? view->property("devToolsView").value<QObject*>() : nullptr;
    }
}

QuickWebEngineSiteDataLink::QuickWebEngineSiteDataLink(QQuickItem *view, QObject *parent)
    : DevToolsSiteDataLink(parent)
    , m_View(view)
    , m_Installed(false)
{
    QQmlEngine *engine = view ? qmlEngine(view) : nullptr;
    if(!engine){
        Break();
        return;
    }
    m_Profile = view->property("profile").value<QObject*>();
    connect(view, &QObject::destroyed, this, [this](){ Break();});
    if(QQuickItem *existing = qobject_cast<QQuickItem*>(DevToolsViewOf(view))){
        m_Target = existing;
        connect(existing, &QObject::destroyed, this, [this](){ Break();});
        connect(existing, SIGNAL(callBackResult(int, QVariant)),
                this,     SLOT(Answered(int, QVariant)));
        Install();
        return;
    }
    QQmlComponent component(engine);
    component.setData(HIDDEN_VIEW_QML, QUrl());
    QObject *made = component.beginCreate(engine->rootContext());
    QQuickItem *hidden = qobject_cast<QQuickItem*>(made);
    if(!hidden){
        delete made;
        Break();
        return;
    }
    QQmlEngine::setObjectOwnership(hidden, QQmlEngine::CppOwnership);
    hidden->setProperty("profile", QVariant::fromValue<QObject*>(m_Profile));
    hidden->setProperty("hiddenUrl", HiddenPageUrl());
    component.completeCreate();
    hidden->setParent(view);
    m_Hidden = hidden;
    m_Target = hidden;
    connect(hidden, SIGNAL(callBackResult(int, QVariant)),
            this,   SLOT(Answered(int, QVariant)));
    connect(hidden, SIGNAL(hiddenLoaded(bool)),
            this,   SLOT(HiddenLoaded(bool)));
    view->setProperty("devToolsView", QVariant::fromValue<QObject*>(hidden));
    hidden->setProperty("url", HiddenPageUrl());
}

QuickWebEngineSiteDataLink::~QuickWebEngineSiteDataLink(){
    Shutdown();
    m_Evaluations.clear();
    if(m_Hidden){
        if(m_View && DevToolsViewOf(m_View) == m_Hidden)
            m_View->setProperty("devToolsView", QVariant());
        disconnect(m_Hidden, nullptr, this, nullptr);
        m_Hidden->deleteLater();
    } else if(m_Target){
        Evaluate(ForgetScript(), Evaluated());
    }
}

QQuickItem *QuickWebEngineSiteDataLink::Target() const {
    return m_Target;
}

void QuickWebEngineSiteDataLink::Evaluate(const QString &script, Evaluated done){
    if(!m_Target) return;
    int id = 0;
    if(done){
        id = NextEvaluation();
        m_Evaluations.insert(id, done);
    }
    QMetaObject::invokeMethod(m_Target, "evaluateJavaScript",
                              Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, QVariant(script)));
}

bool QuickWebEngineSiteDataLink::FrontendAttached() const {
    return m_View && m_Target && DevToolsViewOf(m_View) == m_Target &&
        m_View->property("profile").value<QObject*>() == m_Profile;
}

void QuickWebEngineSiteDataLink::Answered(int id, const QVariant &result){
    auto it = m_Evaluations.find(id);
    if(it == m_Evaluations.end()) return;
    const Evaluated done = it.value();
    m_Evaluations.erase(it);
    done(result);
}

void QuickWebEngineSiteDataLink::HiddenLoaded(bool ok){
    if(m_Installed || Settled()) return;
    m_Installed = true;
    if(!ok){
        Break();
        return;
    }
    Install();
}
