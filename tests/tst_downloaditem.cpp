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

};

QTEST_MAIN(tst_downloaditem)
#include "tst_downloaditem.moc"
