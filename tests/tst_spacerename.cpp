#include "switch.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonObject>

#include "spacerename.hpp"
#include "extensionui.hpp"

namespace {
    SpaceRename::Places PlacesIn(const QString &root, bool edge){
        SpaceRename::Places places;
        places.widgetStorage = root + QStringLiteral("/qt/");
        places.data = root + QStringLiteral("/data/");
        places.state = root + QStringLiteral("/state/");
        places.storageName = [](const QString &id){ return QStringLiteral("h-") + id;};
        if(edge){
            places.edgeProfiles = root + QStringLiteral("/edge/");
            places.edgeJournal = [root](const QString &name){
                return root + QStringLiteral("/data/extension-registry/j-") + name + QStringLiteral(".json");};
        }
        return places;
    }

    bool Touch(const QString &path){
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write("x") == 1;
    }
}

class tst_spacerename : public QObject {
    Q_OBJECT

private slots:

    void everyPlaceOfTheNameIsListed(){
        const SpaceRename::Places places = PlacesIn(QStringLiteral("R"), true);
        const QList<SpaceRename::Move> moves = SpaceRename::Candidates(QStringLiteral("a"), QStringLiteral("b"), places);
        const auto has = [&](SpaceRename::What what, const QString &from, const QString &to){
            return moves.contains(SpaceRename::Move{what, from, to});
        };
        QVERIFY(has(SpaceRename::WidgetStorage, QStringLiteral("R/qt/h-a"), QStringLiteral("R/qt/h-b")));
        QVERIFY(has(SpaceRename::WidgetCache, QStringLiteral("R/data/webenginecache/h-a"), QStringLiteral("R/data/webenginecache/h-b")));
        QVERIFY(has(SpaceRename::QuickStorage, QStringLiteral("R/data/webengine/h-quick:a"), QStringLiteral("R/data/webengine/h-quick:b")));
        QVERIFY(has(SpaceRename::QuickCache, QStringLiteral("R/data/webenginecache/h-quick:a"), QStringLiteral("R/data/webenginecache/h-quick:b")));
        QVERIFY(has(SpaceRename::EdgeProfile, QStringLiteral("R/edge/WV2Profile_h-a"), QStringLiteral("R/edge/WV2Profile_h-b")));
        QVERIFY(has(SpaceRename::ExtensionState,
                    QStringLiteral("R/state/") + ExtensionUi::MenusFileName(QStringLiteral("a")),
                    QStringLiteral("R/state/") + ExtensionUi::MenusFileName(QStringLiteral("b"))));
        QVERIFY(has(SpaceRename::ExtensionState,
                    QStringLiteral("R/state/") + ExtensionUi::RegisteredFileName(QStringLiteral("quick:a")),
                    QStringLiteral("R/state/") + ExtensionUi::RegisteredFileName(QStringLiteral("quick:b"))));
        QVERIFY(has(SpaceRename::ExtensionState,
                    QStringLiteral("R/data/extension-registry/j-h-a-copies.json"),
                    QStringLiteral("R/data/extension-registry/j-h-b-copies.json")));
        QVERIFY(has(SpaceRename::ExtensionState,
                    QStringLiteral("R/state/") + ExtensionUi::MenusFileName(QStringLiteral("edge:h-a")),
                    QStringLiteral("R/state/") + ExtensionUi::MenusFileName(QStringLiteral("edge:h-b"))));
        QCOMPARE(moves.first().what, SpaceRename::WidgetStorage);
        QCOMPARE(moves.last().what, SpaceRename::ExtensionState);

        foreach(const SpaceRename::Move &move,
                SpaceRename::Candidates(QStringLiteral("a"), QStringLiteral("b"), PlacesIn(QStringLiteral("R"), false))){
            QVERIFY(move.what != SpaceRename::EdgeProfile);
            QVERIFY(!move.from.contains(QStringLiteral("edge")));
        }
    }

    void theMoverNeverWritesOverAndGivesUpOnlyOnWhatFails(){
        QTemporaryDir dir;
        const QString r = dir.path();
        QList<SpaceRename::Move> moves = {
            {SpaceRename::WidgetStorage, r + QStringLiteral("/absent"), r + QStringLiteral("/absent-to")},
            {SpaceRename::WidgetCache, r + QStringLiteral("/taken"), r + QStringLiteral("/taken-to")},
            {SpaceRename::QuickStorage, r + QStringLiteral("/late"), r + QStringLiteral("/late-to")},
            {SpaceRename::QuickCache, r + QStringLiteral("/stuck"), r + QStringLiteral("/stuck-to")},
            {SpaceRename::ExtensionState, r + QStringLiteral("/file"), r + QStringLiteral("/file-to")},
        };
        foreach(const QString &name, QStringList() << QStringLiteral("taken") << QStringLiteral("taken-to")
                                                   << QStringLiteral("late") << QStringLiteral("stuck")
                                                   << QStringLiteral("file"))
            QVERIFY(Touch(r + QStringLiteral("/") + name));
        QCOMPARE(SpaceRename::Occupied(moves).size(), 1);

        QStringList tried;
        int lateFailures = 3;
        SpaceRename::Mover mover(moves, [&](const QString &from, const QString &to){
            tried << QFileInfo(from).fileName();
            if(from.endsWith(QStringLiteral("/stuck"))) return false;
            if(from.endsWith(QStringLiteral("/late")) && lateFailures-- > 0) return false;
            return QFile::rename(from, to);
        }, 5, 200);
        bool done = false;
        QList<SpaceRename::Move> failed;
        mover.Start([&](const QList<SpaceRename::Move> &f){ failed = f; done = true;});
        QTRY_VERIFY(done);

        QCOMPARE(failed.size(), 2);
        QCOMPARE(failed.at(0).what, SpaceRename::WidgetCache);
        QCOMPARE(failed.at(1).what, SpaceRename::QuickCache);
        QVERIFY(!tried.contains(QStringLiteral("absent")));
        QVERIFY(!tried.contains(QStringLiteral("taken")));
        QCOMPARE(tried.count(QStringLiteral("late")), 4);
        QVERIFY(tried.count(QStringLiteral("stuck")) > 1);
        QVERIFY(QFileInfo::exists(r + QStringLiteral("/late-to")));
        QVERIFY(QFileInfo::exists(r + QStringLiteral("/file-to")));
        QVERIFY(QFileInfo::exists(r + QStringLiteral("/stuck")));
        QVERIFY(QFileInfo::exists(r + QStringLiteral("/taken")));
    }

    void aMergeAddsOnlyWhatTheOtherStoreHasNot(){
        const auto cookie = [](const char *name, const char *domain, const char *path, bool session = false){
            SpaceRename::Cookie c;
            c.name = QString::fromLatin1(name); c.value = QStringLiteral("a");
            c.domain = QString::fromLatin1(domain); c.path = QString::fromLatin1(path);
            c.session = session; c.expires = session ? -1 : 2000000000;
            return c;
        };
        QList<SpaceRename::Cookie> from = {
            cookie("kept", "example.com", "/"),
            cookie("taken", "Example.COM", "/"),
            cookie("taken", "example.com", "/sub"),
            cookie("session", "example.com", "/", true),
            cookie("twice", "example.com", "/"),
            cookie("twice", "example.com", "/"),
        };
        SpaceRename::Cookie theirs = cookie("taken", "example.com", "/");
        theirs.value = QStringLiteral("b");
        const QList<SpaceRename::Cookie> add = SpaceRename::CookiesToAdd(from, {theirs});
        QStringList names;
        for(const SpaceRename::Cookie &c : add) names << c.name + QLatin1Char('@') + c.path;
        QCOMPARE(names, QStringList() << QStringLiteral("kept@/") << QStringLiteral("taken@/sub")
                                      << QStringLiteral("twice@/"));
    }

    void theWaitSaysWhetherAllWent(){
        QObject *stays = new QObject(this);
        QObject *goes = new QObject(this);
        int answers = 0;
        bool answer = true;
        SpaceRename::WhenGone({QPointer<QObject>(stays), QPointer<QObject>(goes)}, 100,
                              [&](bool all){ answer = all; answers++;});
        goes->deleteLater();
        QTRY_COMPARE(answers, 1);
        QCOMPARE(answer, false);
        QTest::qWait(100);
        QCOMPARE(answers, 1);

        answers = 0;
        QObject *other = new QObject(this);
        SpaceRename::WhenGone({QPointer<QObject>(stays), QPointer<QObject>(other)}, 5000,
                              [&](bool all){ answer = all; answers++;});
        delete stays;
        other->deleteLater();
        QTRY_COMPARE(answers, 1);
        QCOMPARE(answer, true);
        QTest::qWait(100);
        QCOMPARE(answers, 1);
    }

    void aMovedProfileIsNoLongerTheLastUsed(){
        const auto exists = [](const QString &name){ return name == QStringLiteral("WV2Profile_there");};
        QJsonObject profile;
        profile.insert(QStringLiteral("last_used"), QStringLiteral("WV2Profile_moved"));
        profile.insert(QStringLiteral("last_active_profiles"),
                       QJsonArray{QStringLiteral("WV2Profile_moved"), QStringLiteral("WV2Profile_there")});
        profile.insert(QStringLiteral("info_cache"), QJsonObject{{QStringLiteral("WV2Profile_moved"), QJsonObject()}});
        QJsonObject state{{QStringLiteral("profile"), profile}, {QStringLiteral("other"), 1}};
        QVERIFY(SpaceRename::ForgetMissingProfiles(state, exists));
        const QJsonObject after = state.value(QStringLiteral("profile")).toObject();
        QVERIFY(!after.contains(QStringLiteral("last_used")));
        QCOMPARE(after.value(QStringLiteral("last_active_profiles")).toArray(),
                 QJsonArray{QStringLiteral("WV2Profile_there")});
        QVERIFY(after.contains(QStringLiteral("info_cache")));
        QCOMPARE(state.value(QStringLiteral("other")).toInt(), 1);

        QJsonObject kept{{QStringLiteral("profile"), QJsonObject{{QStringLiteral("last_used"), QStringLiteral("WV2Profile_there")}}}};
        QVERIFY(!SpaceRename::ForgetMissingProfiles(kept, exists));
        QCOMPARE(kept.value(QStringLiteral("profile")).toObject().value(QStringLiteral("last_used")).toString(),
                 QStringLiteral("WV2Profile_there"));
    }

    void aHeldLockfileMeansARunningBrowser(){
        QTemporaryDir dir;
        QVERIFY(!SpaceRename::BrowserHoldsFolder(dir.path()));
        QFile lock(dir.filePath(QStringLiteral("lockfile")));
        QVERIFY(lock.open(QIODevice::WriteOnly));
        QVERIFY(SpaceRename::BrowserHoldsFolder(dir.path()));
        lock.close();
#ifdef Q_OS_WIN
        QVERIFY(!SpaceRename::BrowserHoldsFolder(dir.path()));
#endif
    }

    void theDefaultMoveRenamesAndDoesNotCopy(){
        QTemporaryDir dir;
        const QString r = dir.path();
        QVERIFY(Touch(r + QStringLiteral("/a/inner/file")));
        QVERIFY(SpaceRename::Mover::DefaultMove(r + QStringLiteral("/a"), r + QStringLiteral("/b")));
        QVERIFY(QFileInfo::exists(r + QStringLiteral("/b/inner/file")));
        QVERIFY(!QFileInfo::exists(r + QStringLiteral("/a")));
        QVERIFY(Touch(r + QStringLiteral("/c/file")));
        QVERIFY(!SpaceRename::Mover::DefaultMove(r + QStringLiteral("/c"), r + QStringLiteral("/b")));
        QVERIFY(QFileInfo::exists(r + QStringLiteral("/c/file")));
    }
};

QTEST_MAIN(tst_spacerename)
#include "tst_spacerename.moc"
