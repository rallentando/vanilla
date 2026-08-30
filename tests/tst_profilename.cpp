#include "switch.hpp"

#include <QtTest>

#include "networkcontroller.hpp"

#include "testsupport.hpp"

class tst_profilename : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
    }

    void anyNameIsOneBothEnginesAccept(){
        const QStringList ids = QStringList()
            << QStringLiteral("root")
            << QString()
            << QStringLiteral("with/slash")
            << QStringLiteral("two words")
            << QStringLiteral("../..")
            << QStringLiteral("CON")
            << QStringLiteral(".hidden")
            << QStringLiteral("trailing...")
            << QStringLiteral("日本語のディレクトリ")
            << QString(200, QLatin1Char('x'));

        static const QRegularExpression accepted
            (QStringLiteral("\\A[a-zA-Z0-9][a-zA-Z0-9 _.-]{0,63}\\z"));

        foreach(const QString &id, ids){
            const QString name = NetworkController::ProfileStorageName
                (QStringLiteral("C:/app"), id);
            QVERIFY2(accepted.match(name).hasMatch(),
                     qPrintable(QStringLiteral("'%1' is not a name both engines take (from '%2')")
                                .arg(name, id)));
            QVERIFY(!name.endsWith(QLatin1Char('.')));
            QCOMPARE(name.length(), 32);
        }
    }

    void twoInstallsInDifferentDirectoriesDoNotShareAProfile(){
        QVERIFY(NetworkController::ProfileStorageName(QStringLiteral("C:/Program Files/vanilla"),
                                                      QStringLiteral("root")) !=
                NetworkController::ProfileStorageName(QStringLiteral("C:/Program Files/vanilla2"),
                                                      QStringLiteral("root")));
        QVERIFY(NetworkController::ProfileStorageName(QStringLiteral("C:/app"),
                                                      QStringLiteral("root")) !=
                NetworkController::ProfileStorageName(QStringLiteral("C:/app"),
                                                      QStringLiteral("prime")));
    }

    void theSameSpaceAlwaysGetsTheSameName(){
        QCOMPARE(NetworkController::ProfileStorageName(QStringLiteral("C:/app"),
                                                       QStringLiteral("root")),
                 NetworkController::ProfileStorageName(QStringLiteral("C:/app"),
                                                       QStringLiteral("root")));

        QCOMPARE(NetworkController::ProfileStorageName(QStringLiteral("C:/app/x"),
                                                       QStringLiteral("y")),
                 NetworkController::ProfileStorageName(QStringLiteral("C:/app"),
                                                       QStringLiteral("x/y")));
    }

    void cleanupTestCase(){}
};

QTEST_MAIN(tst_profilename)
#include "tst_profilename.moc"
