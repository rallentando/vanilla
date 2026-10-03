#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "receiver.hpp"
#include "testsupport.hpp"

#include <QtTest>
#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <memory>

class tst_receiver : public QObject {
    Q_OBJECT
    static bool connectOnce(QLocalServer *server){
        QSignalSpy accepted(server, &QLocalServer::newConnection);
        QLocalSocket client;
        client.connectToServer(Application::LocalServerName());
        if(!client.waitForConnected(2000)) return false;
        return QTest::qWaitFor([&]{ return accepted.count() == 1; }, 2000);
    }
    static bool commandReaches(Receiver *receiver, const QString &command){
        QSignalSpy keys(receiver, &Receiver::KeyEvent);
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        stream << QStringLiteral("key ") + command;
        QLocalSocket client;
        client.connectToServer(Application::LocalServerName());
        if(!client.waitForConnected(2000)) return false;
        client.write(frame);
        client.waitForBytesWritten(2000);
        return QTest::qWaitFor([&]{ return keys.count() == 1; }, 2000) &&
            keys.first().first().toString() == command;
    }
private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }

    void serverIsReleasedWithTheLastReceiverAndMadeAgain(){
        QPointer<QLocalServer> first;
        {
            MainWindow window(931, QPoint(100, 100));
            first = Receiver::m_LocalServer;
            QVERIFY(first);
            QVERIFY(first->isListening());
        }
        QVERIFY(!first.isNull());
        QVERIFY(!first->isListening());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(first.isNull());
        QVERIFY(!Receiver::m_LocalServer);

        MainWindow again(932, QPoint(100, 100));
        const QPointer<QLocalServer> second = Receiver::m_LocalServer;
        QVERIFY(second);
        QVERIFY(second->isListening());
    }

    void aWindowMadeBeforeTheDeferredDeleteGetsItsOwnServer(){
        QPointer<QLocalServer> old;
        {
            MainWindow window(933, QPoint(100, 100));
            old = Receiver::m_LocalServer;
            QVERIFY(old);
        }
        MainWindow next(934, QPoint(100, 100));
        const QPointer<QLocalServer> fresh = Receiver::m_LocalServer;
        QVERIFY(fresh);
        QVERIFY(fresh != old);
        QVERIFY(!old.isNull() && !old->isListening());
        QVERIFY(fresh->isListening());
        QVERIFY(connectOnce(fresh));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(old.isNull());
        QVERIFY(!fresh.isNull() && fresh->isListening());
        QVERIFY(connectOnce(fresh));
    }

    void twoUnregisteredWindowsReleaseTheServerOnce(){
        auto first = std::make_unique<MainWindow>(935, QPoint(100, 100));
        auto second = std::make_unique<MainWindow>(936, QPoint(100, 100));
        const QPointer<QLocalServer> server = Receiver::m_LocalServer;
        QVERIFY(server);
        first.reset();
        QVERIFY(!Receiver::m_LocalServer);
        second.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(server.isNull());
        QVERIFY(!Receiver::m_LocalServer);
    }

    void anOlderReceiverDoesNotReleaseTheCurrentServer(){
        auto first = std::make_unique<MainWindow>(939, QPoint(100, 100));
        auto second = std::make_unique<MainWindow>(940, QPoint(100, 100));
        const QPointer<QLocalServer> old = Receiver::m_LocalServer;
        QVERIFY(old);
        first.reset();
        QVERIFY(!Receiver::m_LocalServer);
        MainWindow third(941, QPoint(100, 100));
        const QPointer<QLocalServer> fresh = Receiver::m_LocalServer;
        QVERIFY(fresh && fresh != old);
        second.reset();
        QCOMPARE(Receiver::m_LocalServer, fresh.data());
        QVERIFY(fresh->isListening());
        QVERIFY(connectOnce(fresh));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(old.isNull());
        QVERIFY(!fresh.isNull() && fresh->isListening());
    }

    void aReceiverDeletedByItsOwnCommandStopsTakingConnections(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> window(Application::NewWindow(942));
        const auto unregister = qScopeGuard([&]{
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(942);
        });
        Application::SetCurrentWindow(942);
        TreeBank *treeBank = window->GetTreeBank();
        Receiver *receiver = treeBank->GetReceiver();
        QVERIFY(receiver);
        const QPointer<QLocalServer> server = Receiver::m_LocalServer;
        QVERIFY(server);
        QObject::disconnect(server, &QLocalServer::newConnection, receiver, nullptr);
        QSignalSpy accepted(server, &QLocalServer::newConnection);
        const QPointer<Receiver> alive(receiver);
        connect(receiver, &Receiver::KeyEvent, receiver, [&](){
            treeBank->ToggleReceiver();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        QLocalSocket first, second;
        for(QLocalSocket *client : {&first, &second}){
            QByteArray frame;
            QDataStream stream(&frame, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
            stream << QStringLiteral("key F5");
            client->connectToServer(Application::LocalServerName());
            QVERIFY(client->waitForConnected(2000));
            client->write(frame);
            client->waitForBytesWritten(2000);
        }
        QVERIFY(QTest::qWaitFor([&]{ return accepted.count() == 2; }, 2000));
        QTest::qWait(100);
        receiver->ForeignCommandReceived();
        QVERIFY(alive.isNull());
        QVERIFY(!treeBank->GetReceiver());
        QVERIFY(server->hasPendingConnections());
        delete server->nextPendingConnection();
        QVERIFY(!server->hasPendingConnections());
        new Receiver(treeBank, false);
    }

    void registeredWindowsKeepTheServerUntilTheLastOneGoes(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        std::unique_ptr<MainWindow> first(Application::NewWindow(937));
        std::unique_ptr<MainWindow> second(Application::NewWindow(938));
        const auto unregister = qScopeGuard([&]{
            Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
            Application::RemoveWindow(937);
            Application::RemoveWindow(938);
        });
        const QPointer<QLocalServer> server = Receiver::m_LocalServer;
        QVERIFY(server);
        QVERIFY(server->isListening());

        Application::RemoveWindow(937);
        first.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!server.isNull());
        QCOMPARE(Receiver::m_LocalServer, server.data());
        QVERIFY(server->isListening());
        Application::SetCurrentWindow(938);
        QVERIFY(commandReaches(second->GetTreeBank()->GetReceiver(), QStringLiteral("F5")));

        Application::RemoveWindow(938);
        second.reset();
        QVERIFY(!Receiver::m_LocalServer);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(server.isNull());
    }
};

int main(int argc, char **argv){
    Application app(argc, argv);
    tst_receiver test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_receiver.moc"
