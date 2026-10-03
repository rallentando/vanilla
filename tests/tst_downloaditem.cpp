#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTemporaryDir>
#include <QMap>
#include <QUrl>

#include <QFile>
#include <QDir>

#include "networkcontroller.hpp"

namespace {

class AbortingReply : public QNetworkReply {
    Q_OBJECT

public:
    AbortingReply(const QUrl &url, QObject *parent = nullptr)
        : QNetworkReply(parent)
        , m_Aborted(0)
    {
        setUrl(url);
        setRequest(QNetworkRequest(url));
        open(QIODevice::ReadOnly);
    }

    int Aborted() const { return m_Aborted;}

    void abort() Q_DECL_OVERRIDE {
        m_Aborted++;
        emit finished();
    }

protected:
    qint64 readData(char*, qint64) Q_DECL_OVERRIDE { return -1;}

private:
    int m_Aborted;
};

class FakeEngineDownload : public QObject {
    Q_OBJECT

    Q_PROPERTY(QUrl url READ GetUrl CONSTANT)
    Q_PROPERTY(QString path READ GetPath CONSTANT)
    Q_PROPERTY(int state READ GetState NOTIFY stateChanged)
    Q_PROPERTY(int interruptReason READ GetReason NOTIFY stateChanged)
    Q_PROPERTY(qint64 receivedBytes READ GetReceived NOTIFY receivedBytesChanged)
    Q_PROPERTY(qint64 totalBytes READ GetTotal NOTIFY receivedBytesChanged)

public:
    enum { InProgress = 1, Completed = 2, Interrupted = 4 };

    FakeEngineDownload(const QString &path, QObject *parent = nullptr)
        : QObject(parent)
        , m_Path(path)
        , m_State(InProgress)
        , m_Reason(0)
        , m_Received(0)
        , m_Total(-1)
        , m_Cancelled(0)
        , m_Resumed(0)
    {}

    QUrl GetUrl() const { return QUrl(QStringLiteral("https://example.com/file.zip"));}
    QString GetPath() const { return m_Path;}
    int GetState() const { return m_State;}
    int GetReason() const { return m_Reason;}
    qint64 GetReceived() const { return m_Received;}
    qint64 GetTotal() const { return m_Total;}

    int Cancelled() const { return m_Cancelled;}
    int Resumed() const { return m_Resumed;}

    void Write(int state, int reason){ m_State = state; m_Reason = reason;}
    void WriteProgress(qint64 received, qint64 total){ m_Received = received; m_Total = total;}

    void Say(int state, int reason){ Write(state, reason); emit stateChanged();}
    void SayProgress(qint64 received, qint64 total){
        WriteProgress(received, total);
        emit receivedBytesChanged();
    }

    void Replay(){ emit receivedBytesChanged(); emit stateChanged();}

public slots:
    void cancel(){ m_Cancelled++;}
    void resume(){ m_Resumed++;}

signals:
    void stateChanged();
    void receivedBytesChanged();

private:
    QString m_Path;
    int m_State;
    int m_Reason;
    qint64 m_Received;
    qint64 m_Total;
    int m_Cancelled;
    int m_Resumed;
};

class NotifierRows : public QObject {
    Q_OBJECT

public:
    NotifierRows() : m_LowestTotal(-1) {}

    void Register(DownloadItem *item){
        connect(item, &DownloadItem::Progress, this, &NotifierRows::OnProgress);
    }

    int Rows() const { return m_Rows.size();}
    int PerCent(const QString &file) const { return m_Rows.value(file, -1);}
    qint64 LowestTotal() const { return m_LowestTotal;}

private slots:
    void OnProgress(QString file, qint64 received, qint64 total){
        if(m_LowestTotal == -1 || total < m_LowestTotal) m_LowestTotal = total;
        if(total <= 0) return;
        m_Rows[file] = static_cast<int>(100 * (static_cast<float>(received) /
                                               static_cast<float>(total)));
        if(received == 100 && total == 100) m_Rows.remove(file);
    }

private:
    QMap<QString, int> m_Rows;
    qint64 m_LowestTotal;
};

}

class tst_downloaditem : public QObject {
    Q_OBJECT

private slots:

    void aStoppedItemIsDeletedButNotYet(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AbortingReply *reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item = new DownloadItem(reply, QStringLiteral("report.pdf"));
        item->SetPathAndReady(QDir(dir.path()).filePath(QStringLiteral("report.pdf")));

        QMetaObject::invokeMethod(item, "Stop");

        QVERIFY2(item, "the caller which stopped it still holds the pointer");
        QCOMPARE(item->GetPath(), QDir(dir.path()).filePath(QStringLiteral("report.pdf")));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(item.isNull(), "and it is gone by the next turn of the loop");
    }

    void anItemStoppedBeforeItHasAPathIsDeletedToo(){
        AbortingReply *reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item = new DownloadItem(reply, QStringLiteral("report.pdf"));
        QVERIFY(item->GetPath().isEmpty());

        QMetaObject::invokeMethod(item, "Stop");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY(item.isNull());
    }

    void stoppingDoesNotRunTheEndingTwice(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AbortingReply *reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item = new DownloadItem(reply, QStringLiteral("report.pdf"));
        item->SetPathAndReady(QDir(dir.path()).filePath(QStringLiteral("report.pdf")));

        int endings = 0;
        connect(item, &DownloadItem::Progress, this,
                [&endings](QString, qint64 received, qint64 total){
                    if(received == 100 && total == 100) endings++;
                });

        QMetaObject::invokeMethod(item, "Stop");

        QCOMPARE(endings, 1);
        QCOMPARE(reply->Aborted(), 1);

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(item.isNull());
    }

    void stoppingTwiceDoesNothingTheSecondTime(){
        AbortingReply *reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item = new DownloadItem(reply, QStringLiteral("report.pdf"));

        int endings = 0;
        connect(item, &DownloadItem::Progress, this,
                [&endings](QString, qint64 received, qint64 total){
                    if(received == 100 && total == 100) endings++;
                });

        QMetaObject::invokeMethod(item, "Stop");
        QMetaObject::invokeMethod(item, "Stop");

        QCOMPARE(endings, 1);
        QCOMPARE(reply->Aborted(), 1);

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(item.isNull());
    }

    void aReplyWhichFinishedInsideTheDialogStillGoesWhenCancelled(){
        AbortingReply *reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item = new DownloadItem(reply, QStringLiteral("report.pdf"));

        item->m_GettingPath = true;

        QMetaObject::invokeMethod(item, "Finished");

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(item, "nothing deletes an item which has no path yet");

        item->SetPathAndReady(QString());

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(item.isNull(), "the item goes, whatever it was told before");
    }

    void theReplyGoesWithTheItem(){
        QPointer<QNetworkReply> reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item =
            new DownloadItem(reply.data(), QStringLiteral("report.pdf"));

        QMetaObject::invokeMethod(item, "Stop");

        QVERIFY(item);
        QVERIFY(reply);

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(item.isNull());

        QVERIFY(reply);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(reply.isNull(), "the reply the item was made from goes with it");
    }

    void theReplyGoesWhenTheDownloadEndsByItself(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QPointer<QNetworkReply> reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item =
            new DownloadItem(reply.data(), QStringLiteral("report.pdf"));
        item->SetPathAndReady(QDir(dir.path()).filePath(QStringLiteral("report.pdf")));

        QMetaObject::invokeMethod(item, "Finished");

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(item.isNull());

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(reply.isNull());
    }

    void theReplyGoesWhenTheItemsParentDoes(){
        QPointer<QNetworkReply> reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")));
        QPointer<DownloadItem> item =
            new DownloadItem(reply.data(), QStringLiteral("report.pdf"));

        {
            QObject parent;
            item->setParent(&parent);
            QVERIFY(item);
        }

        QVERIFY2(item.isNull(), "the parent deleted it outright");
        QVERIFY2(reply, "and the reply is only asked for");

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(reply.isNull());
    }

    void anItemWhoseReplyDiedFirstStillGoesAway(){
        QObject *owner = new QObject;
        QPointer<QNetworkReply> reply =
            new AbortingReply(QUrl(QStringLiteral("https://example.com/dir/report.pdf")), owner);
        QPointer<DownloadItem> item =
            new DownloadItem(reply.data(), QStringLiteral("report.pdf"));

        delete owner;
        QVERIFY(reply.isNull());

        item->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(item.isNull());
    }

    void aReplyCanKnowItIsEmptyBeforeTheEventLoop(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = QDir(dir.path()).filePath(QStringLiteral("empty.bin"));
        QFile empty(path);
        QVERIFY(empty.open(QIODevice::WriteOnly));
        empty.close();
        QCOMPARE(QFileInfo(path).size(), 0);

        QNetworkAccessManager nam;

        QNetworkReply *file = nam.get(QNetworkRequest(QUrl::fromLocalFile(path)));
        const QVariant fileLength = file->header(QNetworkRequest::ContentLengthHeader);

        QNetworkReply *data =
            nam.get(QNetworkRequest(QUrl(QStringLiteral("data:text/plain,"))));
        const QVariant dataLength = data->header(QNetworkRequest::ContentLengthHeader);

        bool ok = false;
        QVERIFY2(fileLength.isValid(), "a file reply knows its length at once");
        QCOMPARE(fileLength.toInt(&ok), 0);
        QVERIFY(ok);

        ok = false;
        QVERIFY2(dataLength.isValid(), "and so does a 'data:' reply");
        QCOMPARE(dataLength.toInt(&ok), 0);
        QVERIFY(ok);

        QNetworkReply *http =
            nam.get(QNetworkRequest(QUrl(QStringLiteral("http://localhost:1/x"))));
        QVERIFY(!http->header(QNetworkRequest::ContentLengthHeader).isValid());

        file->abort();
        data->abort();
        http->abort();
        file->deleteLater();
        data->deleteLater();
        http->deleteLater();
    }

    void anEmptyReplyGetsNoItem(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = QDir(dir.path()).filePath(QStringLiteral("empty.bin"));
        QFile empty(path);
        QVERIFY(empty.open(QIODevice::WriteOnly));
        empty.close();

        QNetworkAccessManager nam;
        QNetworkReply *reply = nam.get(QNetworkRequest(QUrl::fromLocalFile(path)));

        QPointer<QNetworkReply> watched = reply;

        QCOMPARE(NetworkController::Download(reply, QStringLiteral("empty.bin"),
                                             NetworkController::TemporaryDirectory),
                 nullptr);

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(watched.isNull());
    }

    void aCompletedDownloadTakesItsRowAndItselfAway();
    void bytesReachingTheTotalIsNotTheEnd();
    void anInterruptionWorthResumingIsResumedOnce();
    void anInterruptionOutsideThatRangeIsTheEnd();
    void anInterruptionOutsideThatRangeIsTheEnd_data();
    void theRowGoesWhenTheEnginesItemGoes();
    void aTotalWhichIsNotALengthIsNeverPassedOn();
    void aTotalWhichIsNotALengthIsNeverPassedOn_data();
    void anEndingWhichArrivedBeforeTheRowLeavesNothingBehind();
    void anInterruptionWhichArrivedBeforeTheRowLeavesNothingBehind();

    void theViewWhoseNavigationBecameADownloadIsAskedOnce();
};

void tst_downloaditem::aCompletedDownloadTakesItsRowAndItselfAway(){
    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);

    engine.SayProgress(512, 4096);
    QCOMPARE(rows.Rows(), 1);
    QCOMPARE(rows.PerCent(path), 12);

    engine.Say(FakeEngineDownload::Completed, 0);

    QCOMPARE(rows.Rows(), 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
}

void tst_downloaditem::bytesReachingTheTotalIsNotTheEnd(){
    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);

    engine.SayProgress(4096, 4096);

    QCOMPARE(rows.Rows(), 1);
    QCOMPARE(rows.PerCent(path), 100);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!watched.isNull());

    engine.Say(FakeEngineDownload::Completed, 0);
    QCOMPARE(rows.Rows(), 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
}

void tst_downloaditem::anInterruptionWorthResumingIsResumedOnce(){
    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);
    engine.SayProgress(512, 4096);

    engine.Say(FakeEngineDownload::Interrupted, 20);

    QCOMPARE(engine.Resumed(), 1);
    QCOMPARE(rows.Rows(), 1);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!watched.isNull());
}

void tst_downloaditem::anInterruptionOutsideThatRangeIsTheEnd(){
    QFETCH(int, reason);

    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);
    engine.SayProgress(512, 4096);

    engine.Say(FakeEngineDownload::Interrupted, reason);

    QCOMPARE(engine.Resumed(), 0);
    QCOMPARE(rows.Rows(), 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
}

void tst_downloaditem::anInterruptionOutsideThatRangeIsTheEnd_data(){
    QTest::addColumn<int>("reason");
    QTest::newRow("none") << 0;
    QTest::newRow("file") << 2;
    QTest::newRow("cancelled") << 40;
}

void tst_downloaditem::theRowGoesWhenTheEnginesItemGoes(){
    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    NotifierRows rows;

    FakeEngineDownload *engine = new FakeEngineDownload(path);
    DownloadItem *item = new DownloadItem(engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);

    engine->SayProgress(512, 4096);
    QCOMPARE(rows.Rows(), 1);

    delete engine;

    QCOMPARE(rows.Rows(), 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
}

void tst_downloaditem::aTotalWhichIsNotALengthIsNeverPassedOn(){
    QFETCH(qint64, total);

    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);

    engine.SayProgress(500, total);

    QVERIFY2(rows.LowestTotal() > 0, qPrintable(QString::number(rows.LowestTotal())));
    QCOMPARE(rows.Rows(), 1);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!watched.isNull());
}

void tst_downloaditem::aTotalWhichIsNotALengthIsNeverPassedOn_data(){
    QTest::addColumn<qint64>("total");
    QTest::newRow("unknown") << qint64(-1);
    QTest::newRow("zero") << qint64(0);
}

void tst_downloaditem::anEndingWhichArrivedBeforeTheRowLeavesNothingBehind(){
    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    engine.WriteProgress(4096, 4096);
    engine.Write(FakeEngineDownload::Completed, 0);

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);

    engine.Replay();

    QCOMPARE(rows.Rows(), 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
}

void tst_downloaditem::anInterruptionWhichArrivedBeforeTheRowLeavesNothingBehind(){
    const QString path = QDir::tempPath() + QStringLiteral("/vanilla-download.zip");
    FakeEngineDownload engine(path);
    NotifierRows rows;

    engine.WriteProgress(512, 4096);
    engine.Write(FakeEngineDownload::Interrupted, 0);

    DownloadItem *item = new DownloadItem(&engine);
    QPointer<DownloadItem> watched = item;
    rows.Register(item);

    engine.Replay();

    QCOMPARE(rows.Rows(), 0);
    QCOMPARE(engine.Resumed(), 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(watched.isNull());
}

void tst_downloaditem::theViewWhoseNavigationBecameADownloadIsAskedOnce(){
    QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR "/app/networkcontroller.cpp")));
    QVERIFY2(file.open(QIODevice::ReadOnly), "check VANILLA_SOURCE_DIR");
    const QString source = QString::fromUtf8(file.readAll());

    QCOMPARE(source.count(QStringLiteral("GoBackOrCloseForDownload")), 1);
    QVERIFY2(source.contains(QStringLiteral("dynamic_cast<View*>(Application::CurrentWidget())")),
             "the question is not asked of the view");
    QVERIFY2(!source.contains(QStringLiteral("qobject_cast<WebEngineView*>(Application::CurrentWidget())")),
             "the question is still asked of one kind of view");
    QVERIFY2(!source.contains(QStringLiteral("qobject_cast<QuickWebEngineView*>(Application::CurrentWidget())")),
             "the question is still asked of one kind of view");
}

QTEST_MAIN(tst_downloaditem)
#include "tst_downloaditem.moc"
