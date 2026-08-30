#include <QtTest>

#include <QBuffer>
#include <QDataStream>

#include "commandframe.hpp"

namespace {

QByteArray Frame(const QString &command){
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    stream << command;
    return bytes;
}

}

class tst_commandframe : public QObject {
    Q_OBJECT

private slots:
    void waitsForTheWholeFrameWithoutConsumingIt(){
        const QByteArray whole = Frame(QStringLiteral("open https://example.com"));
        QBuffer buffer;
        buffer.setData(whole.first(whole.size() - 1));
        QVERIFY(buffer.open(QIODevice::ReadOnly));

        QString command;
        QCOMPARE(CommandFrame::Read(&buffer, &command), CommandFrame::Incomplete);
        QCOMPARE(buffer.pos(), qint64(0));
    }

    void readsACompleteCommand(){
        QBuffer buffer;
        buffer.setData(Frame(QStringLiteral("blank")));
        QVERIFY(buffer.open(QIODevice::ReadOnly));

        QString command;
        QCOMPARE(CommandFrame::Read(&buffer, &command), CommandFrame::Complete);
        QCOMPARE(command, QStringLiteral("blank"));
    }

    void rejectsAnOversizedFrameFromItsHeader(){
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream << quint32(CommandFrame::MaximumPayloadBytes + 2U);
        QBuffer buffer(&bytes);
        QVERIFY(buffer.open(QIODevice::ReadOnly));

        QString command;
        QCOMPARE(CommandFrame::Read(&buffer, &command), CommandFrame::TooLarge);
        QCOMPARE(buffer.pos(), qint64(0));
    }
};

QTEST_MAIN(tst_commandframe)
#include "tst_commandframe.moc"
