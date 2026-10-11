#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTemporaryDir>

#include "qtlog.hpp"
#include "edgehostwindow.hpp"

namespace {

QString Read(const QString &path){
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(f.readAll());
}

void Quiet(QtMsgType, const QMessageLogContext &, const QString &){}

}

class tst_qtlog : public QObject {
    Q_OBJECT

private slots:
    void warningsAndWorseAreWrittenAndTheRestIsNot();
    void theHandlerBeforeStillGetsEveryMessage();
    void aFileOverTheLimitIsMovedAsideAtStart();
    void oneRunStopsWritingAtItsLimit();
    void aMessageStaysOnOneLine();
    void theEdgeHostRefusingTheFocusGoesNowhere();
};

void tst_qtlog::warningsAndWorseAreWrittenAndTheRestIsNot(){
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("qt-warnings.log"));
    QVERIFY(QtLog::Install(path));
    QTest::ignoreMessage(QtWarningMsg, "a warning");
    QTest::ignoreMessage(QtCriticalMsg, "a critical");
    qInfo("an info");
    qWarning("a warning");
    qCritical("a critical");
    QtLog::Uninstall();

    const QString text = Read(path);
    QVERIFY(text.startsWith(QStringLiteral("--- ")));
    QVERIFY(text.contains(QStringLiteral(" warning: a warning")));
    QVERIFY(text.contains(QStringLiteral(" critical: a critical")));
    QVERIFY(!text.contains(QStringLiteral("a debug")));
    QVERIFY(!text.contains(QStringLiteral("an info")));

    QTest::ignoreMessage(QtWarningMsg, "after");
    qWarning("after");
    QVERIFY(!Read(path).contains(QStringLiteral("after")));
}

void tst_qtlog::theHandlerBeforeStillGetsEveryMessage(){
    QTemporaryDir dir;
    QVERIFY(QtLog::Install(dir.filePath(QStringLiteral("qt-warnings.log"))));
    QTest::ignoreMessage(QtWarningMsg, "handed on");
    QTest::ignoreMessage(QtInfoMsg, "handed on too");
    qWarning("handed on");
    qInfo("handed on too");
    QtLog::Uninstall();
}

void tst_qtlog::aFileOverTheLimitIsMovedAsideAtStart(){
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("qt-warnings.log"));
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(QtLog::RotateLimit + 1, 'x'));
    }
    QVERIFY(QtLog::Install(path));
    QtLog::Uninstall();

    QCOMPARE(QFileInfo(path + QStringLiteral(".old")).size(), QtLog::RotateLimit + 1);
    QVERIFY(QFileInfo(path).size() < 1024);
    QVERIFY(Read(path).startsWith(QStringLiteral("--- ")));

    QVERIFY(QtLog::Install(path));
    QtLog::Uninstall();
    QCOMPARE(Read(path).count(QStringLiteral("--- ")), 2);
}

void tst_qtlog::oneRunStopsWritingAtItsLimit(){
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("qt-warnings.log"));
    QtMessageHandler test = qInstallMessageHandler(Quiet);
    QVERIFY(QtLog::Install(path));
    const QByteArray line(1000, 'w');
    for(int i = 0; i < 1500; i++) qWarning("%s", line.constData());
    QtLog::Uninstall();
    qInstallMessageHandler(test);

    const QString text = Read(path);
    QVERIFY(QFileInfo(path).size() <= QtLog::RunLimit + 100);
    QCOMPARE(text.count(QStringLiteral("--- the rest of this run is not written")), 1);
}

void tst_qtlog::aMessageStaysOnOneLine(){
    QMessageLogContext context("file.cpp", 12, "function", "qt.rhi.general");
    const QString line = QtLog::Format(QtWarningMsg, context, QStringLiteral("first\nsecond"));
    QVERIFY(!line.contains(QLatin1Char('\n')));
    QVERIFY(line.contains(QStringLiteral(" warning qt.rhi.general: first\\nsecond (file.cpp:12)")));

    QMessageLogContext plain(nullptr, 0, nullptr, "default");
    QVERIFY(QtLog::Format(QtCriticalMsg, plain, QStringLiteral("x")).endsWith(QStringLiteral(" critical: x")));
}

void tst_qtlog::theEdgeHostRefusingTheFocusGoesNowhere(){
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("qt-warnings.log"));
    QVERIFY(QtLog::Install(path));
    QTest::failOnWarning(QRegularExpression(QStringLiteral("EdgeHostWindow")));
    EdgeHostWindow host;
    host.requestActivate();
    qWarning("requestActivate() called for  EdgeHostWindow(0x1e97ea75c00)  which has Qt::WindowDoesNotAcceptFocus set.");
    const char *other = "requestActivate() called for  QWindow(0x1)  which has Qt::WindowDoesNotAcceptFocus set.";
    QTest::ignoreMessage(QtWarningMsg, other);
    qWarning("%s", other);
    QtLog::Uninstall();

    const QString text = Read(path);
    QVERIFY(!text.contains(QStringLiteral("EdgeHostWindow")));
    QVERIFY(text.contains(QStringLiteral("QWindow(0x1)")));
    QVERIFY(!QtLog::Dropped(QtCriticalMsg, QStringLiteral(
        "requestActivate() called for  EdgeHostWindow(0x1)  which has Qt::WindowDoesNotAcceptFocus set.")));
}

QTEST_MAIN(tst_qtlog)
#include "tst_qtlog.moc"
