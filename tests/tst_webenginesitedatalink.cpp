#include "webenginesitedatalink.hpp"
#include "testsupport.hpp"

#include <QJsonArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebChannel>
#include <QApplication>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtWebEngineQuick/QtWebEngineQuick>
#include <QtTest/QtTest>

namespace {
    class Server : public QTcpServer {
    public:
        Server(){
            connect(this, &QTcpServer::newConnection, this, [this](){
                while(QTcpSocket *socket = nextPendingConnection()){
                    connect(socket, &QTcpSocket::readyRead, socket, [socket](){
                        const QByteArray request = socket->readAll();
                        QByteArray head = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n";
                        QByteArray body;
                        if(request.startsWith("GET /set")){
                            head += "Set-Cookie: a=1; Path=/; Max-Age=600; HttpOnly\r\n";
                            head += "Set-Cookie: b=2; Path=/deep; Max-Age=600\r\n";
                            body = "<title>set</title>";
                        } else if(request.startsWith("GET /count")){
                            body = "<script>var n=0;setInterval(function(){n++;debugger;document.title='n'+n},100)</script>";
                        } else {
                            body = "<title>plain</title>";
                        }
                        head += "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
                        socket->write(head + body);
                        socket->disconnectFromHost();
                    });
                }
            });
        }
        QUrl Url(const QString &path) const {
            return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(serverPort()).arg(path));
        }
    };

    bool Load(QWebEnginePage &page, const QUrl &url){
        QSignalSpy finished(&page, &QWebEnginePage::loadFinished);
        page.load(url);
        return finished.wait(15000) && finished.first().first().toBool();
    }

    struct Answered {
        bool done = false;
        bool ok = false;
        QJsonObject result;
        qint64 msec = 0;
    };
    Answered Ask(SiteDataLink *link, const QString &method, const QJsonObject &params = QJsonObject()){
        auto answered = std::make_shared<Answered>();
        link->Call(method, params, [answered](bool ok, const QJsonObject &result){
            answered->done = true;
            answered->ok = ok;
            answered->result = result;
        });
        QElapsedTimer waited;
        waited.start();
        while(!answered->done && waited.elapsed() < 10000) QTest::qWait(20);
        answered->msec = waited.elapsed();
        return *answered;
    }
    QVariant Run(QWebEnginePage *page, const QString &script){
        auto result = std::make_shared<QVariant>();
        auto done = std::make_shared<bool>(false);
        page->runJavaScript(script, [result, done](const QVariant &value){ *result = value; *done = true;});
        QElapsedTimer waited;
        waited.start();
        while(!*done && waited.elapsed() < 10000) QTest::qWait(20);
        return *result;
    }
    int OurCookies(const QJsonObject &result){
        int count = 0;
        for(const QJsonValue &cookie : result.value(QStringLiteral("cookies")).toArray())
            if(cookie.toObject().value(QStringLiteral("domain")).toString() == QStringLiteral("127.0.0.1")) count++;
        return count;
    }

    struct QuickPage {
        QQmlEngine engine;
        QQuickWindow window;
        QObject *profile = nullptr;
        QQuickItem *view = nullptr;

        QuickPage(){
            profile = Create(QStringLiteral("import QtWebEngine\nWebEngineProfile {}\n"));
            view = Make(QStringLiteral("import QtWebEngine\nWebEngineView { width: 400; height: 300 }\n"), profile);
            view->setParentItem(window.contentItem());
            window.resize(400, 300);
            window.show();
        }
        ~QuickPage(){
            delete view;
            delete profile;
        }
        QObject *Create(const QString &qml, QObject *withProfile = nullptr){
            QQmlComponent component(&engine);
            component.setData(qml.toUtf8(), QUrl());
            QObject *made = component.beginCreate(engine.rootContext());
            if(!made){
                qWarning() << component.errorString();
                return nullptr;
            }
            QQmlEngine::setObjectOwnership(made, QQmlEngine::CppOwnership);
            if(withProfile) made->setProperty("profile", QVariant::fromValue(withProfile));
            component.completeCreate();
            return made;
        }
        QQuickItem *Make(const QString &qml, QObject *withProfile = nullptr){
            return qobject_cast<QQuickItem*>(Create(qml, withProfile));
        }
        QString Title() const { return view->property("title").toString();}
        bool Load(const QUrl &url, const QString &title){
            view->setProperty("url", url);
            QElapsedTimer waited;
            waited.start();
            while(Title() != title && waited.elapsed() < 15000) QTest::qWait(20);
            return Title() == title;
        }
        QVariant Run(QQuickItem *frontend, const QString &script){
            QSignalSpy spy(frontend, SIGNAL(callBackResult(int, QVariant)));
            QMetaObject::invokeMethod(frontend, "evaluateJavaScript",
                                      Q_ARG(QVariant, QVariant(-5)), Q_ARG(QVariant, QVariant(script)));
            QElapsedTimer waited;
            waited.start();
            while(waited.elapsed() < 10000){
                for(const QList<QVariant> &args : std::as_const(spy))
                    if(args.value(0).toInt() == -5) return args.value(1);
                QTest::qWait(20);
            }
            return QVariant();
        }
    };

    QQuickItem *DevToolsView(QQuickItem *view){
        return qobject_cast<QQuickItem*>(view->property("devToolsView").value<QObject*>());
    }
}

class tst_webenginesitedatalink : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        QVERIFY(m_Server.listen(QHostAddress::LocalHost));
    }

    void aHiddenPageOfItsOwn(){
        QWebEngineProfile profile;
        QWebEnginePage page(&profile);
        QVERIFY(Load(page, m_Server.Url(QStringLiteral("/set"))));

        QPointer<WebEngineSiteDataLink> link = new WebEngineSiteDataLink(&page);
        QVERIFY(link->UsesHiddenPage());
        QCOMPARE(page.devToolsPage(), link->Target());
        const Answered all = Ask(link, QStringLiteral("Network.getAllCookies"));
        QVERIFY(all.ok);
        QCOMPARE(OurCookies(all.result), 2);
        QCOMPARE(link->Target()->url(), WebEngineSiteDataLink::HiddenPageUrl());

        link->Target()->load(QUrl(QStringLiteral("devtools://devtools/bundled/inspector.html")));
        link->Target()->load(m_Server.Url(QStringLiteral("/plain")));
        QTest::qWait(1000);
        QCOMPARE(link->Target()->url(), WebEngineSiteDataLink::HiddenPageUrl());

        QVERIFY(!Ask(link, QStringLiteral("Storage.getCookies")).ok);

        QJsonObject params;
        params.insert(QStringLiteral("name"), QStringLiteral("a"));
        params.insert(QStringLiteral("domain"), QStringLiteral("127.0.0.1"));
        params.insert(QStringLiteral("path"), QStringLiteral("/"));
        const Answered deleted = Ask(link, QStringLiteral("Network.deleteCookies"), params);
        QVERIFY2(deleted.ok, QJsonDocument(deleted.result).toJson().constData());
        QCOMPARE(OurCookies(Ask(link, QStringLiteral("Network.getAllCookies")).result), 1);

        QWebEnginePage other(&profile);
        page.setDevToolsPage(&other);
        {
            const Answered refused = Ask(link, QStringLiteral("Network.getAllCookies"));
            QVERIFY(refused.done && !refused.ok);
            QVERIFY(refused.msec < 3000);
        }
        delete link;
        QCOMPARE(page.devToolsPage(), &other);
        page.setDevToolsPage(nullptr);

        link = new WebEngineSiteDataLink(&page);
        QVERIFY(Ask(link, QStringLiteral("Network.getAllCookies")).ok);
        delete link;
        QVERIFY(!page.devToolsPage());
    }

    void goneWhileItIsBeingInstalled(){
        QWebEngineProfile profile;
        QWebEnginePage page(&profile);
        QVERIFY(Load(page, m_Server.Url(QStringLiteral("/set"))));
        auto answered = std::make_shared<int>(0);
        QPointer<WebEngineSiteDataLink> link = new WebEngineSiteDataLink(&page);
        link->Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [answered](bool, const QJsonObject&){
            (*answered)++;
        });
        connect(link->Target(), &QWebEnginePage::loadFinished, link, [link](bool){
            QTimer::singleShot(0, [link](){ delete link.data(); });
        });
        QTRY_VERIFY_WITH_TIMEOUT(!link, 15000);
        QTest::qWait(500);
        QCOMPARE(*answered, 0);
        QVERIFY(!page.devToolsPage());
    }

    void thePageDoesNotStopAtADebuggerStatement(){
        QWebEngineProfile profile;
        QWebEnginePage page(&profile);
        QVERIFY(Load(page, m_Server.Url(QStringLiteral("/count"))));
        WebEngineSiteDataLink link(&page);
        QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
        const QString before = page.title();
        QTRY_VERIFY_WITH_TIMEOUT(page.title() != before, 5000);
        const QString later = page.title();
        QTRY_VERIFY_WITH_TIMEOUT(page.title() != later, 5000);
    }

    void theInspectorItHasIsUsed(){
        QWebEngineProfile profile;
        QWebEnginePage page(&profile);
        QVERIFY(Load(page, m_Server.Url(QStringLiteral("/set"))));
        QWebEnginePage inspector(&profile);
        QSignalSpy loaded(&inspector, &QWebEnginePage::loadFinished);
        page.setDevToolsPage(&inspector);
        QVERIFY(loaded.wait(15000));

        QPointer<WebEngineSiteDataLink> link = new WebEngineSiteDataLink(&page);
        QVERIFY(!link->UsesHiddenPage());
        QCOMPARE(link->Target(), &inspector);
        QCOMPARE(OurCookies(Ask(link, QStringLiteral("Network.getAllCookies")).result), 2);
        delete link;
        QCOMPARE(page.devToolsPage(), &inspector);

        {
            WebEngineSiteDataLink first(&page);
            QVERIFY(Ask(&first, QStringLiteral("Network.getAllCookies")).ok);
            QVERIFY(Run(&inspector, QStringLiteral("window.__vanillaSiteData.next = %1; true")
                        .arg((first.Base() - WebEngineSiteDataLink::ID_FLOOR) >> 20)).toBool());
            WebEngineSiteDataLink second(&page);
            QVERIFY(Ask(&second, QStringLiteral("Network.getAllCookies")).ok);
            QVERIFY(first.Base() >= WebEngineSiteDataLink::ID_FLOOR);
            QVERIFY(second.Base() >= WebEngineSiteDataLink::ID_FLOOR);
            QVERIFY(first.Base() != second.Base());
        }
        QTRY_COMPARE(Run(&inspector, QStringLiteral("Object.keys(window.__vanillaSiteData.links).length")).toInt(), 0);
        for(int i = 0; i < 3; i++){
            WebEngineSiteDataLink gone(&page);
        }
        QTRY_COMPARE(Run(&inspector, QStringLiteral("Object.keys(window.__vanillaSiteData.links).length")).toInt(), 0);

        {
            WebEngineSiteDataLink link(&page);
            QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
            const QString key = QStringLiteral("'") + link.Key() + QStringLiteral("'");
            const qint64 late = link.Base() + 500;
            QVERIFY(Run(&inspector, QStringLiteral(
                "window.__vanillaSiteData.send(%1, JSON.stringify({id: %2, method: 'Network.getAllCookies', params: {}}), performance.now() - 1); true")
                .arg(key).arg(late)).toBool());
            QTest::qWait(1000);
            QCOMPARE(Run(&inspector, QStringLiteral("window.__vanillaSiteData.links[%1].queue.length").arg(key)).toInt(), 0);

            QVERIFY(Run(&inspector, QStringLiteral("var l = window.__vanillaSiteData.links[%1]; l.low = 0; l.high = 1; true").arg(key)).toBool());
            const Answered never = Ask(&link, QStringLiteral("Network.getAllCookies"));
            QVERIFY(never.done && !never.ok);
            QVERIFY(never.msec >= SiteDataLink::TIMEOUT_MSEC - 500);
            QVERIFY(never.msec < SiteDataLink::TIMEOUT_MSEC + 3000);
        }

        {
            QPointer<WebEngineSiteDataLink> dying = new WebEngineSiteDataLink(&page);
            QVERIFY(Ask(dying, QStringLiteral("Network.getAllCookies")).ok);
            QVERIFY(Run(&inspector, QStringLiteral("var l = window.__vanillaSiteData.links['%1']; l.low = 0; l.high = 1; true")
                        .arg(dying->Key())).toBool());
            dying->Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [dying](bool, const QJsonObject&){
                delete dying.data();
            });
            QTRY_VERIFY_WITH_TIMEOUT(!dying, SiteDataLink::TIMEOUT_MSEC + 3000);
            QTest::qWait(200);
        }

        {
            WebEngineSiteDataLink link(&page);
            QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
            QVERIFY(Run(&inspector, QStringLiteral(
                "window.__chunks = 0; var hub = window.__vanillaSiteData;"
                "var inner = DevToolsAPI.dispatchMessageChunk; true")).toBool());
            const qint64 low = Run(&inspector, QStringLiteral("window.__vanillaSiteData.links[%1].low")
                                   .arg(QStringLiteral("'") + link.Key() + QStringLiteral("'"))).toLongLong();
            QVERIFY(low >= WebEngineSiteDataLink::ID_FLOOR);
            const QString whole = QStringLiteral("{\"id\":%1,\"result\":{\"x\":1}}").arg(low + 999);
            QVERIFY(Run(&inspector, QStringLiteral(
                "DevToolsAPI.dispatchMessageChunk(%1, 100); DevToolsAPI.dispatchMessageChunk(%2);"
                "JSON.stringify(window.__vanillaSiteData.links[%3].queue)")
                .arg(QStringLiteral("'") + whole.left(whole.indexOf(QStringLiteral(",\"result\"")) + 3) + QStringLiteral("'"),
                     QStringLiteral("'") + whole.mid(whole.indexOf(QStringLiteral(",\"result\"")) + 3) + QStringLiteral("'"),
                     QStringLiteral("'") + link.Key() + QStringLiteral("'"))).toString().contains(QStringLiteral("\"x\":1")));
            QVERIFY(Run(&inspector, QStringLiteral(
                "window.__passed = 0; InspectorFrontendAPI.dispatchMessageChunk = function(){ window.__passed++; };"
                "DevToolsAPI.dispatchMessageChunk('{\"id\":7,\"res', 100); DevToolsAPI.dispatchMessageChunk('ult\":{}}');"
                "window.__passed === 2 && window.__vanillaSiteData.chunk === null")).toBool());
        }

        {
            WebEngineSiteDataLink link(&page);
            QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
            QSignalSpy again(&inspector, &QWebEnginePage::loadFinished);
            inspector.triggerAction(QWebEnginePage::Reload);
            QVERIFY(again.wait(15000));
            const Answered lost = Ask(&link, QStringLiteral("Network.getAllCookies"));
            QVERIFY(lost.done && !lost.ok);
            QVERIFY(lost.msec < 3000);
        }
        page.setDevToolsPage(nullptr);
    }

    void aPageWithAChannelGoesWhileAttached(){
        QWebEngineProfile profile;
        QWebEnginePage *page = new QWebEnginePage(&profile);
        QObject *owner = new QObject(page);
        QWebChannel *channel = new QWebChannel(owner);
        QObject *bridge = new QObject(owner);
        channel->registerObject(QStringLiteral("bridge"), bridge);
        page->setWebChannel(channel, QWebEngineScript::ApplicationWorld);
        QVERIFY(Load(*page, m_Server.Url(QStringLiteral("/set"))));
        QPointer<WebEngineSiteDataLink> link = new WebEngineSiteDataLink(page);
        QVERIFY(Ask(link, QStringLiteral("Network.getAllCookies")).ok);
        page->deleteLater();
        QTest::qWait(500);
        delete link;
        QTest::qWait(200);
    }

    void thePageGoesWhileItWaits(){
        QWebEngineProfile profile;
        QWebEnginePage *page = new QWebEnginePage(&profile);
        QVERIFY(Load(*page, m_Server.Url(QStringLiteral("/set"))));
        WebEngineSiteDataLink link(page);
        QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
        auto answered = std::make_shared<int>(0);
        auto failed = std::make_shared<bool>(false);
        link.Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [answered, failed](bool ok, const QJsonObject&){
            (*answered)++;
            *failed = !ok;
        });
        delete page;
        QTRY_COMPARE_WITH_TIMEOUT(*answered, 1, 5000);
        QVERIFY(*failed);
        QVERIFY(!link.Target() || !link.UsesHiddenPage() || link.Target());
    }

    void quickHiddenViewOfItsOwn(){
        QuickPage page;
        QVERIFY(page.Load(m_Server.Url(QStringLiteral("/set")), QStringLiteral("set")));

        QPointer<QuickWebEngineSiteDataLink> link = new QuickWebEngineSiteDataLink(page.view);
        QVERIFY(link->UsesHiddenView());
        QCOMPARE(DevToolsView(page.view), link->Target());
        const Answered all = Ask(link, QStringLiteral("Network.getAllCookies"));
        QVERIFY(all.ok);
        QCOMPARE(OurCookies(all.result), 2);
        QCOMPARE(link->Target()->property("url").toUrl(), DevToolsSiteDataLink::HiddenPageUrl());
        QVERIFY(!link->Target()->window());

        QQuickItem *hidden = link->Target();
        hidden->setProperty("url", QUrl(QStringLiteral("devtools://devtools/bundled/inspector.html")));
        hidden->setProperty("url", m_Server.Url(QStringLiteral("/plain")));
        QTest::qWait(1000);
        QCOMPARE(hidden->property("url").toUrl(), DevToolsSiteDataLink::HiddenPageUrl());

        QVERIFY(!Ask(link, QStringLiteral("Storage.getCookies")).ok);

        QJsonObject params;
        params.insert(QStringLiteral("name"), QStringLiteral("a"));
        params.insert(QStringLiteral("domain"), QStringLiteral("127.0.0.1"));
        params.insert(QStringLiteral("path"), QStringLiteral("/"));
        QVERIFY(Ask(link, QStringLiteral("Network.deleteCookies"), params).ok);
        QCOMPARE(OurCookies(Ask(link, QStringLiteral("Network.getAllCookies")).result), 1);

        QPointer<QQuickItem> gone = hidden;
        delete link;
        QVERIFY(!DevToolsView(page.view));
        QTRY_VERIFY(!gone);

        link = new QuickWebEngineSiteDataLink(page.view);
        QVERIFY(Ask(link, QStringLiteral("Network.getAllCookies")).ok);
        QQuickItem *other = page.Make(QStringLiteral("import QtWebEngine\nWebEngineView {}\n"));
        page.view->setProperty("devToolsView", QVariant::fromValue<QObject*>(other));
        {
            const Answered refused = Ask(link, QStringLiteral("Network.getAllCookies"));
            QVERIFY(refused.done && !refused.ok);
            QVERIFY(refused.msec < 3000);
        }
        delete link;
        QCOMPARE(DevToolsView(page.view), other);
        page.view->setProperty("devToolsView", QVariant());
        delete other;

        link = new QuickWebEngineSiteDataLink(page.view);
        QVERIFY(Ask(link, QStringLiteral("Network.getAllCookies")).ok);
        QObject *second = page.Create(QStringLiteral("import QtWebEngine\nWebEngineProfile {}\n"));
        QVERIFY(second);
        second->setParent(&page.engine);
        QVERIFY(page.view->setProperty("profile", QVariant::fromValue<QObject*>(second)));
        {
            const Answered refused = Ask(link, QStringLiteral("Network.getAllCookies"));
            QVERIFY(refused.done && !refused.ok);
            QVERIFY(refused.msec < 3000);
        }
        delete link;
        QVERIFY(!DevToolsView(page.view));
    }

    void quickGoneWhileItIsBeingInstalled(){
        QuickPage page;
        QVERIFY(page.Load(m_Server.Url(QStringLiteral("/set")), QStringLiteral("set")));
        auto answered = std::make_shared<int>(0);
        QPointer<QuickWebEngineSiteDataLink> link = new QuickWebEngineSiteDataLink(page.view);
        link->Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [answered](bool, const QJsonObject&){
            (*answered)++;
        });
        connect(link->Target(), SIGNAL(hiddenLoaded(bool)), this, SLOT(DeleteLinkLater()));
        m_Doomed = link;
        QTRY_VERIFY_WITH_TIMEOUT(!link, 15000);
        QTest::qWait(500);
        QCOMPARE(*answered, 0);
        QVERIFY(!DevToolsView(page.view));
    }

    void quickThePageDoesNotStopAtADebuggerStatement(){
        QuickPage page;
        page.view->setProperty("url", m_Server.Url(QStringLiteral("/count")));
        QTRY_VERIFY_WITH_TIMEOUT(page.Title().startsWith(QLatin1Char('n')), 15000);
        QuickWebEngineSiteDataLink link(page.view);
        QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
        const QString before = page.Title();
        QTRY_VERIFY_WITH_TIMEOUT(page.Title() != before, 5000);
        const QString later = page.Title();
        QTRY_VERIFY_WITH_TIMEOUT(page.Title() != later, 5000);
    }

    void quickTheInspectorItHasIsUsed(){
        QuickPage page;
        QVERIFY(page.Load(m_Server.Url(QStringLiteral("/set")), QStringLiteral("set")));
        QFile file(QStringLiteral(VANILLA_SOURCE_DIR "/view/webengine/quickwebengineinspector6.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QQuickItem *inspector = page.Make(QString::fromUtf8(file.readAll()));
        QVERIFY(inspector);
        page.view->setProperty("devToolsView", QVariant::fromValue<QObject*>(inspector));
        QTRY_VERIFY_WITH_TIMEOUT(inspector->property("url").toString().endsWith(QStringLiteral("inspector.html")) &&
                                 !inspector->property("loading").toBool(), 15000);

        QPointer<QuickWebEngineSiteDataLink> link = new QuickWebEngineSiteDataLink(page.view);
        QVERIFY(!link->UsesHiddenView());
        QCOMPARE(link->Target(), inspector);
        QCOMPARE(OurCookies(Ask(link, QStringLiteral("Network.getAllCookies")).result), 2);
        {
            QuickWebEngineSiteDataLink first(page.view);
            QuickWebEngineSiteDataLink second(page.view);
            auto answers = std::make_shared<QList<int>>();
            for(int i = 0; i < 3; i++){
                first.Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [answers](bool ok, const QJsonObject &result){
                    *answers << (ok ? OurCookies(result) : -1);
                });
                second.Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [answers](bool ok, const QJsonObject &result){
                    *answers << (ok ? OurCookies(result) : -1);
                });
            }
            QTRY_COMPARE_WITH_TIMEOUT(answers->size(), 6, 10000);
            QTest::qWait(300);
            QCOMPARE(*answers, QList<int>({ 2, 2, 2, 2, 2, 2 }));
            QVERIFY(first.Base() != second.Base());
        }
        delete link;
        QCOMPARE(DevToolsView(page.view), inspector);
        QTRY_COMPARE(page.Run(inspector, QStringLiteral("Object.keys(window.__vanillaSiteData.links).length")).toInt(), 0);
        page.view->setProperty("devToolsView", QVariant());
        delete inspector;
    }

    void quickTheViewGoesFirst(){
        QuickPage page;
        QVERIFY(page.Load(m_Server.Url(QStringLiteral("/set")), QStringLiteral("set")));
        QuickWebEngineSiteDataLink link(page.view);
        QVERIFY(Ask(&link, QStringLiteral("Network.getAllCookies")).ok);
        QPointer<QQuickItem> hidden = link.Target();
        auto answered = std::make_shared<int>(0);
        auto failed = std::make_shared<bool>(false);
        link.Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [answered, failed](bool ok, const QJsonObject&){
            (*answered)++;
            *failed = !ok;
        });
        delete page.view;
        page.view = nullptr;
        QVERIFY(!hidden);
        QTRY_COMPARE_WITH_TIMEOUT(*answered, 1, SiteDataLink::TIMEOUT_MSEC + 3000);
        QVERIFY(*failed);
        QVERIFY(!Ask(&link, QStringLiteral("Network.getAllCookies")).ok);

        QuickPage early;
        QVERIFY(early.Load(m_Server.Url(QStringLiteral("/set")), QStringLiteral("set")));
        QuickWebEngineSiteDataLink queued(early.view);
        auto times = std::make_shared<int>(0);
        auto okay = std::make_shared<bool>(true);
        queued.Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [times, okay](bool ok, const QJsonObject&){
            (*times)++;
            *okay = ok;
        });
        delete early.view;
        early.view = nullptr;
        QCOMPARE(*times, 1);
        QVERIFY(!*okay);
        QTest::qWait(SiteDataLink::TIMEOUT_MSEC + 500);
        QCOMPARE(*times, 1);
    }

public slots:
    void DeleteLinkLater(){
        QPointer<QObject> doomed = m_Doomed;
        QTimer::singleShot(0, [doomed](){ delete doomed.data(); });
    }

private:
    Server m_Server;
    QPointer<QObject> m_Doomed;
};

int main(int argc, char **argv){
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_webenginesitedatalink test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_webenginesitedatalink.moc"
