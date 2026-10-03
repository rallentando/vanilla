#include "switch.hpp"

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QCryptographicHash>

#include "extensioncopy.hpp"
#include "extensionhostwire.hpp"
#include "cdpshims.hpp"

#include "testsupport.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class tst_extensioncopy : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void theManifestChangesInThreePlaces();
    void aClassicWorkerInAFolderIsWrappedBesideItself();
    void theWorkersAddressIsTheSameWhateverTheStamp();
    void whatCannotBeCopiedIsSaidWhy_data();
    void whatCannotBeCopiedIsSaidWhy();
    void aFourthNumberOfTheVersionGivesWayToTheStamp();
    void theCopyHasTheShimsAndTheOriginalIsUntouched();
    void aCopyAlreadyMadeIsUsedAndAnOldOneRemoved();
    void whatGoesWrongAnswersWithTheOriginal();
    void theStampFollowsWhatTheCopyIsMadeOf();
    void theEngineSpellsAPathWithItsLinksFollowed();
    void aCopyIsNotMadeOfACopy();
    void whatVanillaAddsTakesNobodysPlace();
    void whatAnotherProcessIsMakingIsLeftToIt();
    void aRefusalIsNotRemembered();
    void onlyAddingAnExtensionMakesACopy();
    void theKeyGoesIntoTheShimAndTheStamp();
    void whereAPageNamesItsShim_data();
    void whereAPageNamesItsShim();
    void thePagesOfTheCopyNameTheirShimAndNoKeyIsInIt();
    void aPatternOfTheManifestsIsReadAsTheEngineReadsIt_data();
    void aPatternOfTheManifestsIsReadAsTheEngineReadsIt();
    void whatTheWebMayReadGetsNoKey_data();
    void whatTheWebMayReadGetsNoKey();
    void aPageTheWebMayEmbedNamesTheShimWithoutTheKey();
    void thePolicyLetsTheApplicationsSchemeThrough_data();
    void thePolicyLetsTheApplicationsSchemeThrough();
    void whatAPatternReachesIsReadByItsLetter_data();
    void whatAPatternReachesIsReadByItsLetter();
    void whatIsLongerThanAPageIsLeftAsItIs();
    void aCopyMadeTheWayOfBeforeIsNotTakenForOne();
    void theMessagesOfTheUsersLocaleGoAheadOfTheShims();
    void theBackendIsToldWhenWhatItHoldsIsNotWhatItWouldBeGiven();
    void theRelayOfTheWorkerIsInTheKeyedCopyAndInNoOther();
    void thePinnedPathStaysWhileTheCopyBehindItChanges();
    void whatIsNotACopyIsNotPinned();
    void theCopiesBeforeGoAndTheJunctionStaysWithoutBeingWalkedThrough();
    void aJunctionWhoseCopyHasGoneIsPointedAgain();
    void aRegistrationGivenTheJunctionIsKeptAndRegisteredAgain();
    void aCarrierGoesAheadWhereTheExtensionMayRegisterScripts();

private:
    static QJsonObject Manifest(){
        return QJsonDocument::fromJson(R"({
            "manifest_version": 3, "name": "fixture", "version": "2.4.2", "key": "MIIBIjANBg",
            "background": { "service_worker": "background_scripts/main.js", "type": "module" },
            "content_scripts": [
              { "matches": ["<all_urls>"], "js": ["lib/types.js", "lib/utils.js"], "run_at": "document_start", "all_frames": true },
              { "matches": ["<all_urls>"], "css": ["content.css"] },
              { "matches": ["<all_urls>"], "js": ["page.js"], "world": "MAIN" }
            ] })").object();
    }
    static void Write(const QString &path, const QByteArray &bytes){
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
    }
    static QByteArray Read(const QString &path){
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray("<unreadable>");
    }
    static void Extension(const QString &root, const QJsonObject &manifest = Manifest()){
        Write(root + QStringLiteral("/manifest.json"), QJsonDocument(manifest).toJson());
        Write(root + QStringLiteral("/background_scripts/main.js"), "import './other.js';\n");
        Write(root + QStringLiteral("/background_scripts/other.js"), "self.other = 1;\n");
        Write(root + QStringLiteral("/lib/types.js"), "var types = 1;\n");
        Write(root + QStringLiteral("/_metadata/verified_contents.json"), "[]");
        Write(root + QStringLiteral("/.git/HEAD"), "ref: refs/heads/master\n");
    }
#ifdef Q_OS_WIN
    static quint64 EntryOf(const QString &path){
        const HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(path).utf16()), 0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if(handle == INVALID_HANDLE_VALUE) return 0;
        BY_HANDLE_FILE_INFORMATION info;
        const bool ok = GetFileInformationByHandle(handle, &info);
        CloseHandle(handle);
        return ok ? (quint64(info.nFileIndexHigh) << 32) | info.nFileIndexLow : 0;
    }
#endif
    static QStringList Tree(const QString &root){
        QStringList files;
        QDirIterator it(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while(it.hasNext()){ it.next(); files << QDir(root).relativeFilePath(it.filePath()) + QLatin1Char('=') + QString::fromLatin1(QCryptographicHash::hash(Read(it.filePath()), QCryptographicHash::Sha1).toHex().left(8)); }
        files.sort();
        return files;
    }
};

void tst_extensioncopy::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_extensioncopy::init(){
    ExtensionCopy::ForgetForTesting();
}

void tst_extensioncopy::theManifestChangesInThreePlaces(){
    const ExtensionCopy::Rewritten r = ExtensionCopy::Rewrite(Manifest(), 4711);
    QVERIFY2(r.refusal.isEmpty(), qPrintable(r.refusal));

    const QJsonArray entries = r.manifest[QStringLiteral("content_scripts")].toArray();
    QCOMPARE(entries.at(0).toObject()[QStringLiteral("js")].toArray(),
             QJsonArray() << QStringLiteral("vanilla_content_shim.js") << QStringLiteral("lib/types.js") << QStringLiteral("lib/utils.js"));
    QJsonObject first = entries.at(0).toObject(), firstBefore = Manifest()[QStringLiteral("content_scripts")].toArray().at(0).toObject();
    first.remove(QStringLiteral("js")); firstBefore.remove(QStringLiteral("js"));
    QCOMPARE(first, firstBefore);
    QCOMPARE(entries.at(1).toObject(), Manifest()[QStringLiteral("content_scripts")].toArray().at(1).toObject());
    QCOMPARE(entries.at(2).toObject(), Manifest()[QStringLiteral("content_scripts")].toArray().at(2).toObject());

    QCOMPARE(r.manifest[QStringLiteral("background")].toObject()[QStringLiteral("service_worker")].toString(),
             QStringLiteral("background_scripts/vanilla_worker.js"));
    QCOMPARE(r.workerWrapper, QStringLiteral("background_scripts/vanilla_worker.js"));
    QCOMPARE(r.workerShim, QStringLiteral("background_scripts/vanilla_worker_shim.js"));
    QCOMPARE(r.workerWrapperText, QStringLiteral("// vanilla copy 4711\nimport './vanilla_worker_shim.js';\nimport './main.js';\n"));
    QCOMPARE(r.manifest[QStringLiteral("background")].toObject()[QStringLiteral("type")].toString(), QStringLiteral("module"));
    QCOMPARE(r.manifest[QStringLiteral("background")].toObject().keys(), QStringList() << QStringLiteral("service_worker") << QStringLiteral("type"));

    QCOMPARE(r.manifest[QStringLiteral("version")].toString(), QStringLiteral("2.4.2.4711"));

    QJsonObject rest = r.manifest, before = Manifest();
    foreach(const QString &changed, QStringList() << QStringLiteral("content_scripts") << QStringLiteral("background") << QStringLiteral("version")){
        rest.remove(changed); before.remove(changed);
    }
    QCOMPARE(rest, before);
}

void tst_extensioncopy::aClassicWorkerInAFolderIsWrappedBesideItself(){
    QJsonObject manifest = Manifest();
    QJsonObject background; background[QStringLiteral("service_worker")] = QStringLiteral("background.js");
    manifest[QStringLiteral("background")] = background;
    const ExtensionCopy::Rewritten root = ExtensionCopy::Rewrite(manifest, 7);
    QCOMPARE(root.workerWrapper, QStringLiteral("vanilla_worker.js"));
    QCOMPARE(root.workerWrapperText, QStringLiteral("// vanilla copy 7\nimportScripts('./vanilla_worker_shim.js', './background.js');\n"));
    const ExtensionCopy::Rewritten again = ExtensionCopy::Rewrite(manifest, 8);
    QCOMPARE(again.workerWrapper, root.workerWrapper);
    QVERIFY(again.workerWrapperText != root.workerWrapperText);

    background[QStringLiteral("service_worker")] = QStringLiteral("a/b/sw.js");
    manifest[QStringLiteral("background")] = background;
    const ExtensionCopy::Rewritten deep = ExtensionCopy::Rewrite(manifest, 7);
    QCOMPARE(deep.workerWrapper, QStringLiteral("a/b/vanilla_worker.js"));
    QCOMPARE(deep.workerShim, QStringLiteral("a/b/vanilla_worker_shim.js"));
    QCOMPARE(deep.workerWrapperText, QStringLiteral("// vanilla copy 7\nimportScripts('./vanilla_worker_shim.js', './sw.js');\n"));

    manifest.remove(QStringLiteral("background"));
    const ExtensionCopy::Rewritten pageOnly = ExtensionCopy::Rewrite(manifest, 7);
    QVERIFY(pageOnly.refusal.isEmpty());
    QVERIFY(pageOnly.workerWrapper.isEmpty());
    QVERIFY(!pageOnly.manifest.contains(QStringLiteral("background")));
    QJsonObject workerOnly = Manifest();
    workerOnly.remove(QStringLiteral("content_scripts"));
    QVERIFY(!ExtensionCopy::Rewrite(workerOnly, 7).manifest.contains(QStringLiteral("content_scripts")));
    QJsonObject classic = Manifest();
    classic[QStringLiteral("background")] = background;
    QCOMPARE(ExtensionCopy::Rewrite(classic, 7).manifest[QStringLiteral("background")].toObject().keys(), QStringList() << QStringLiteral("service_worker"));
    QCOMPARE(pageOnly.contentShim, QStringLiteral("vanilla_content_shim.js"));

    foreach(const QString &spelled, QStringList() << QStringLiteral("/js/background.js") << QStringLiteral("//js/background.js")){
        QJsonObject fromRoot = Manifest();
        QJsonObject slashed; slashed[QStringLiteral("service_worker")] = spelled; slashed[QStringLiteral("type")] = QStringLiteral("module");
        fromRoot[QStringLiteral("background")] = slashed;
        const ExtensionCopy::Rewritten r = ExtensionCopy::Rewrite(fromRoot, 7);
        QVERIFY2(r.refusal.isEmpty(), qPrintable(spelled + QLatin1String(": ") + r.refusal));
        QCOMPARE(r.workerWrapper, QStringLiteral("js/vanilla_worker.js"));
        QCOMPARE(r.workerShim, QStringLiteral("js/vanilla_worker_shim.js"));
        QCOMPARE(r.workerWrapperText, QStringLiteral("// vanilla copy 7\nimport './vanilla_worker_shim.js';\nimport './background.js';\n"));
        QCOMPARE(r.manifest[QStringLiteral("background")].toObject()[QStringLiteral("service_worker")].toString(), QStringLiteral("js/vanilla_worker.js"));
    }
    QJsonObject onlySlashes = Manifest();
    QJsonObject slashes; slashes[QStringLiteral("service_worker")] = QStringLiteral("/");
    onlySlashes[QStringLiteral("background")] = slashes;
    QVERIFY(ExtensionCopy::Rewrite(onlySlashes, 7).workerWrapper.isEmpty());
}

void tst_extensioncopy::whatCannotBeCopiedIsSaidWhy_data(){
    QTest::addColumn<QString>("key");
    QTest::addColumn<QJsonValue>("value");
    QTest::newRow("no key") << QStringLiteral("key") << QJsonValue();
    QTest::newRow("manifest version 2") << QStringLiteral("manifest_version") << QJsonValue(2);
    QTest::newRow("a version of words") << QStringLiteral("version") << QJsonValue(QStringLiteral("1.0-beta"));
    QTest::newRow("a version of five") << QStringLiteral("version") << QJsonValue(QStringLiteral("1.2.3.4.5"));
    QTest::newRow("a version too large") << QStringLiteral("version") << QJsonValue(QStringLiteral("1.70000"));
    QTest::newRow("a version with a zero in front") << QStringLiteral("version") << QJsonValue(QStringLiteral("1.02"));
    QJsonObject outside; outside[QStringLiteral("service_worker")] = QStringLiteral("../elsewhere/sw.js");
    QTest::newRow("a worker outside it") << QStringLiteral("background") << QJsonValue(outside);
#ifdef Q_OS_WIN
    QJsonObject elsewhere; elsewhere[QStringLiteral("service_worker")] = QStringLiteral("C:/elsewhere/sw.js");
    QTest::newRow("a worker elsewhere on the disk") << QStringLiteral("background") << QJsonValue(elsewhere);
#endif
}

void tst_extensioncopy::whatCannotBeCopiedIsSaidWhy(){
    QFETCH(QString, key);
    QFETCH(QJsonValue, value);
    QJsonObject manifest = Manifest();
    if(value.isNull() || value.isUndefined()) manifest.remove(key);
    else manifest[key] = value;
    const ExtensionCopy::Rewritten r = ExtensionCopy::Rewrite(manifest, 7);
    QVERIFY(!r.refusal.isEmpty());
    QVERIFY(r.manifest.isEmpty());

    QJsonObject bare = Manifest();
    bare.remove(QStringLiteral("background")); bare.remove(QStringLiteral("content_scripts"));
    QVERIFY(!ExtensionCopy::Rewrite(bare, 7).refusal.isEmpty());
    QVERIFY(!ExtensionCopy::Rewrite(Manifest(), 0).refusal.isEmpty());
    QVERIFY(!ExtensionCopy::Rewrite(Manifest(), 65536).refusal.isEmpty());
}

void tst_extensioncopy::aFourthNumberOfTheVersionGivesWayToTheStamp(){
    QJsonObject manifest = Manifest();
    manifest[QStringLiteral("version")] = QStringLiteral("1.2.3.4");
    QCOMPARE(ExtensionCopy::Rewrite(manifest, 9).manifest[QStringLiteral("version")].toString(), QStringLiteral("1.2.3.9"));
    manifest[QStringLiteral("version")] = QStringLiteral("5");
    QCOMPARE(ExtensionCopy::Rewrite(manifest, 9).manifest[QStringLiteral("version")].toString(), QStringLiteral("5.9"));
}

void tst_extensioncopy::theCopyHasTheShimsAndTheOriginalIsUntouched(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("registered/vimium")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QStringList before = Tree(source);

    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("abcdefghijklmnop"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QVERIFY(made.path.startsWith(QDir(root).absolutePath() + QStringLiteral("/abcdefghijklmnop/")));
    QCOMPARE(Tree(source), before);

    const QJsonObject manifest = QJsonDocument::fromJson(Read(made.path + QStringLiteral("/manifest.json"))).object();
    const QString worker = manifest[QStringLiteral("background")].toObject()[QStringLiteral("service_worker")].toString();
    QCOMPARE(worker, QStringLiteral("background_scripts/vanilla_worker.js"));
    QCOMPARE(Read(made.path + QLatin1Char('/') + worker),
             "// vanilla copy " + QFileInfo(made.path).fileName().toLatin1() + "\nimport './vanilla_worker_shim.js';\nimport './main.js';\n");
    QCOMPARE(Read(made.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js")), Cdp::WorkerShim().toUtf8() + ";\n");
    QCOMPARE(Read(made.path + QStringLiteral("/vanilla_content_shim.js")), Cdp::ContentShim().toUtf8() + ";\n");
    QCOMPARE(Read(made.path + QStringLiteral("/background_scripts/main.js")), QByteArray("import './other.js';\n"));
    QCOMPARE(Read(made.path + QStringLiteral("/lib/types.js")), QByteArray("var types = 1;\n"));
    QVERIFY(!QFile::exists(made.path + QStringLiteral("/_metadata")));
    QVERIFY(!QFile::exists(made.path + QStringLiteral("/.git")));

    QCOMPARE(ExtensionCopy::SourceOf(made.path), source);
    QCOMPARE(ExtensionCopy::SourceOf(QDir::toNativeSeparators(made.path)), source);
    QCOMPARE(ExtensionCopy::SourceOf(QFileInfo(made.path).canonicalFilePath()), source);
    QCOMPARE(ExtensionCopy::SourceOf(QFileInfo(made.path).absolutePath() + QStringLiteral("/./") + QFileInfo(made.path).fileName() + QLatin1Char('/')), source);
    QCOMPARE(ExtensionCopy::SourceOf(source), source);
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("abcdefghijklmnop")).path, made.path);
}

void tst_extensioncopy::aCopyAlreadyMadeIsUsedAndAnOldOneRemoved(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString first = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    ExtensionCopy::ForgetForTesting();
    Write(first + QStringLiteral("/marker-of-the-first-run"), "x");
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("idid")).path, first);
    QVERIFY(QFile::exists(first + QStringLiteral("/marker-of-the-first-run")));

    ExtensionCopy::ForgetForTesting();
    QVERIFY(QFile::remove(first + QStringLiteral("/.vanilla-copy-complete")));
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("idid")).path, first);
    QVERIFY(!QFile::exists(first + QStringLiteral("/marker-of-the-first-run")));
    QVERIFY(QFile::exists(first + QStringLiteral("/.vanilla-copy-complete")));

    ExtensionCopy::ForgetForTesting();
    Write(dir.filePath(QStringLiteral("copies/another/1/manifest.json")), "{}");
    Write(dir.filePath(QStringLiteral("beside.txt")), "x");
    QJsonObject changed = Manifest(); changed[QStringLiteral("version")] = QStringLiteral("2.4.3");
    Write(source + QStringLiteral("/manifest.json"), QJsonDocument(changed).toJson());
    const QString second = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    QVERIFY(second != first);
    QVERIFY(!QFile::exists(first));
    QVERIFY(QFile::exists(second + QStringLiteral("/manifest.json")));
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("copies/another/1/manifest.json"))));
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("beside.txt"))));
    QVERIFY(QFile::exists(source + QStringLiteral("/lib/types.js")));
}

void tst_extensioncopy::whatGoesWrongAnswersWithTheOriginal(){
    QTemporaryDir dir;
    const QString root = dir.filePath(QStringLiteral("copies"));

    const QString keyless = dir.filePath(QStringLiteral("keyless"));
    QJsonObject manifest = Manifest(); manifest.remove(QStringLiteral("key"));
    Extension(keyless, manifest);
    const ExtensionCopy::Made made = ExtensionCopy::Make(keyless, root, QStringLiteral("idid"));
    QCOMPARE(made.path, keyless);
    QVERIFY(made.note.contains(QStringLiteral("key")));
    QVERIFY(!QFile::exists(root + QStringLiteral("/idid")));

    const QString missing = dir.filePath(QStringLiteral("not-there"));
    QCOMPARE(ExtensionCopy::Make(missing, root, QStringLiteral("idid")).path, missing);

    const QString fine = dir.filePath(QStringLiteral("fine"));
    Extension(fine);
    foreach(const QString &id, QStringList() << QString() << QStringLiteral("..") << QStringLiteral("a/b") << QStringLiteral("a\\b")){
        ExtensionCopy::ForgetForTesting();
        QCOMPARE(ExtensionCopy::Make(fine, root, id).path, fine);
    }

    ExtensionCopy::ForgetForTesting();
    Write(dir.filePath(QStringLiteral("a-file")), "x");
    const ExtensionCopy::Made blocked = ExtensionCopy::Make(fine, dir.filePath(QStringLiteral("a-file")), QStringLiteral("idid"));
    QCOMPARE(blocked.path, fine);
    QVERIFY(!blocked.note.isEmpty());
}

void tst_extensioncopy::theStampFollowsWhatTheCopyIsMadeOf(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext"));
    Extension(source);
    const QByteArray bytes = Read(source + QStringLiteral("/manifest.json"));
    const int stamp = ExtensionCopy::Stamp(source, bytes);
    QVERIFY(stamp >= 1 && stamp <= 65535);
    QCOMPARE(ExtensionCopy::Stamp(source, bytes), stamp);

    QVERIFY(ExtensionCopy::Stamp(source, bytes + " ") != stamp);
    QFile worker(source + QStringLiteral("/background_scripts/main.js"));
    QVERIFY(worker.open(QIODevice::ReadWrite));
    QVERIFY(worker.setFileTime(QDateTime::currentDateTime().addSecs(3600), QFileDevice::FileModificationTime));
    worker.close();
    const int afterTheWorker = ExtensionCopy::Stamp(source, bytes);
    QVERIFY(afterTheWorker != stamp);

    Write(source + QStringLiteral("/lib/types.js"), "var types = 2; // longer\n");
    const int afterTheScript = ExtensionCopy::Stamp(source, bytes);
    QVERIFY(afterTheScript != afterTheWorker);
    Write(source + QStringLiteral("/lib/added.js"), "");
    const int afterTheFile = ExtensionCopy::Stamp(source, bytes);
    QVERIFY(afterTheFile != afterTheScript);
    Write(source + QStringLiteral("/.git/index"), "x");
    Write(source + QStringLiteral("/_metadata/computed_hashes.json"), "{}");
    QCOMPARE(ExtensionCopy::Stamp(source, bytes), afterTheFile);
}

void tst_extensioncopy::theEngineSpellsAPathWithItsLinksFollowed(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext"));
    const QString real = dir.filePath(QStringLiteral("real")), linked = dir.filePath(QStringLiteral("linked"));
    Extension(source);
    QVERIFY(QDir().mkpath(real));
#ifdef Q_OS_WIN
    Q_UNUSED(linked)
    QSKIP("no link which Qt follows can be made here without a privilege");
#else
    QVERIFY(QFile::link(real, linked));
#endif

    const ExtensionCopy::Made made = ExtensionCopy::Make(source, linked + QStringLiteral("/copies"), QStringLiteral("idid"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QVERIFY(made.path.startsWith(linked));
    const QString followed = QFileInfo(made.path).canonicalFilePath();
    QVERIFY2(followed.startsWith(QFileInfo(real).canonicalFilePath()), qPrintable(followed));
    QCOMPARE(ExtensionCopy::SourceOf(followed), source);
    QCOMPARE(ExtensionCopy::SourceOf(made.path), source);
}

void tst_extensioncopy::aCopyIsNotMadeOfACopy(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString copy = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    QVERIFY(copy != source);
    ExtensionCopy::ForgetForTesting();
    const QStringList before = Tree(root);
    const ExtensionCopy::Made made = ExtensionCopy::Make(copy, root, QStringLiteral("idid"));
    QCOMPARE(made.path, copy);
    QVERIFY(!made.note.isEmpty());
    QCOMPARE(Tree(root), before);

    ExtensionCopy::ForgetForTesting();
    const ExtensionCopy::Made inside = ExtensionCopy::Make(source, source + QStringLiteral("/copies"), QStringLiteral("idid"));
    QCOMPARE(inside.path, source);
    QVERIFY(!QFile::exists(source + QStringLiteral("/copies")));
}

void tst_extensioncopy::whatVanillaAddsTakesNobodysPlace(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    Write(source + QStringLiteral("/background_scripts/vanilla_worker_shim.js"), "the extension's own\n");
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"));
    QCOMPARE(made.path, source);
    QVERIFY(made.note.contains(QStringLiteral("%1")));
    QCOMPARE(made.detail, QStringLiteral("background_scripts/vanilla_worker_shim.js"));
    QVERIFY(!QFile::exists(root + QStringLiteral("/idid")) || QDir(root + QStringLiteral("/idid")).isEmpty());
    QTemporaryDir cased;
    const QString casedSource = cased.filePath(QStringLiteral("ext"));
    Extension(casedSource);
    Write(casedSource + QStringLiteral("/Vanilla_Page_Shim.js"), "the extension's own\n");
    const ExtensionCopy::Made casedMade = ExtensionCopy::Make(casedSource, cased.filePath(QStringLiteral("copies")), QStringLiteral("idid"));
    QCOMPARE(casedMade.path, casedSource);
    QCOMPARE(casedMade.detail, QStringLiteral("Vanilla_Page_Shim.js"));

#ifndef Q_OS_WIN
    ExtensionCopy::ForgetForTesting();
    const QString linked = dir.filePath(QStringLiteral("linked"));
    Extension(linked);
    Write(dir.filePath(QStringLiteral("outside.js")), "x");
    QVERIFY(QFile::link(dir.filePath(QStringLiteral("outside.js")), linked + QStringLiteral("/lib/outside.js")));
    const ExtensionCopy::Made refused = ExtensionCopy::Make(linked, root, QStringLiteral("linked"));
    QCOMPARE(refused.path, linked);
    QCOMPARE(refused.detail, QStringLiteral("lib/outside.js"));
#endif
}

void tst_extensioncopy::whatAnotherProcessIsMakingIsLeftToIt(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString theirs = root + QStringLiteral("/idid/.making-4711-1");
    Write(theirs + QStringLiteral("/manifest.json"), "{}");
    const int stamp = ExtensionCopy::Stamp(QDir(source).absolutePath(), Read(source + QStringLiteral("/manifest.json")));
    const QString debris = root + QStringLiteral("/idid/") + QString::number(stamp);
    Write(debris + QStringLiteral("/lib/held-open.js"), "x");

    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QCOMPARE(made.path, debris);
    QVERIFY(QFile::exists(made.path + QStringLiteral("/manifest.json")));
    QVERIFY(!QFile::exists(made.path + QStringLiteral("/lib/held-open.js")));
    QVERIFY(QFile::exists(theirs + QStringLiteral("/manifest.json")));
    QCOMPARE(QDir(root + QStringLiteral("/idid")).entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).size(), 2);
}

void tst_extensioncopy::aRefusalIsNotRemembered(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    QJsonObject keyless = Manifest(); keyless.remove(QStringLiteral("key"));
    Extension(source, keyless);
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("idid")).path, source);
    Write(source + QStringLiteral("/manifest.json"), QJsonDocument(Manifest()).toJson());
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QVERIFY(made.path != source);
}

void tst_extensioncopy::onlyAddingAnExtensionMakesACopy(){
    const QString root = QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR));
    int files = 0;
    QStringList users;
    QDirIterator it(root, QStringList() << QStringLiteral("*.cpp") << QStringLiteral("*.hpp"), QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext()){
        it.next();
        files++;
        if(it.fileName().startsWith(QStringLiteral("extensioncopy."))) continue;
        if(Read(it.filePath()).contains("ExtensionCopy::")) users << QDir(root).relativeFilePath(it.filePath());
    }
    QVERIFY2(files > 20, "the sources were not found; check VANILLA_SOURCE_DIR");
    users.sort();
    QVERIFY2(users == (QStringList() << QStringLiteral("view/edge/edgeextensions.cpp")
                                     << QStringLiteral("view/webengine/webengineextensions.cpp")),
             qPrintable(QStringLiteral("'ExtensionCopy::' is named in: ") + users.join(QStringLiteral(", "))));

    foreach(const QString &user, users){
        const QByteArray text = Read(root + QLatin1Char('/') + user);
        const bool edge = user.contains(QStringLiteral("edge"));
        QCOMPARE(text.count("ExtensionCopy::Make"), edge ? 2 : 1);
        const int add = text.indexOf("void Add(");
        int next = text.indexOf("void Enable(", add);
        const int remove = text.indexOf("void Remove(", add);
        if(remove > 0 && (next < 0 || remove < next)) next = remove;
        QVERIFY(add > 0 && next > add);
        int pinned = -1, pinnedEnd = -1;
        if(edge){
            pinned = text.indexOf("QString PinnedCopy(");
            pinnedEnd = text.indexOf("\n    }", pinned);
            QVERIFY(pinned > 0 && pinnedEnd > pinned);
        }
        for(int make = text.indexOf("ExtensionCopy::Make"); make >= 0; make = text.indexOf("ExtensionCopy::Make", make + 1))
            QVERIFY2((make > add && make < next) || (make > pinned && make < pinnedEnd),
                     qPrintable(QStringLiteral("a copy is made somewhere other than in 'Add' in ") + user));
        if(edge){
            QCOMPARE(text.count("PinnedCopy("), 3);
            const int migrate = text.indexOf("void Migrate("), after = text.indexOf("\n        void ", migrate + 1);
            const int start = text.indexOf("void EdgeEnvironment::PinExtensionCopies("), startEnd = text.indexOf("\n}", start);
            QVERIFY(migrate > 0 && after > migrate && start > 0 && startEnd > start);
            for(int call = text.indexOf("PinnedCopy(", pinnedEnd); call >= 0; call = text.indexOf("PinnedCopy(", call + 1))
                QVERIFY2((call > migrate && call < after) || (call > start && call < startEnd),
                         qPrintable(QStringLiteral("the second copy is made somewhere other than in 'Migrate' or before the backend starts in ") + user));
        }
    }
}

void tst_extensioncopy::theKeyGoesIntoTheShimAndTheStamp(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QByteArray key = QByteArray(64, 'c'), place = "__VANILLA_HOST_KEY__";
    QVERIFY(Cdp::WorkerShim().toUtf8().contains(place));

    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"), key);
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    const QByteArray shim = Read(made.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js"));
    QVERIFY(shim.contains(key));
    QVERIFY(!shim.contains(place));
    QCOMPARE(shim, Cdp::WorkerShim().toUtf8().replace(place, key) + ";\n");

    const QByteArray bytes = Read(source + QStringLiteral("/manifest.json"));
    QVERIFY(ExtensionCopy::Stamp(source, bytes, key) != ExtensionCopy::Stamp(source, bytes, QByteArray(64, 'd')));
    QVERIFY(ExtensionCopy::Stamp(source, bytes, key) != ExtensionCopy::Stamp(source, bytes));

    foreach(const QByteArray &bad, QList<QByteArray>() << QByteArray() << QByteArray(63, 'c') << QByteArray(64, 'C')
                                                       << QByteArray("'; self.stolen = 1; '").leftJustified(64, 'c')){
        ExtensionCopy::ForgetForTesting();
        const ExtensionCopy::Made unkeyed = ExtensionCopy::Make(source, dir.filePath(QStringLiteral("copies-") + QString::number(qHash(bad))), QStringLiteral("idid"), bad);
        QVERIFY2(unkeyed.note.isEmpty(), qPrintable(unkeyed.note));
        QCOMPARE(Read(unkeyed.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js")), Cdp::WorkerShim().toUtf8() + ";\n");
    }
}

void tst_extensioncopy::whereAPageNamesItsShim_data(){
    QTest::addColumn<QByteArray>("marked");
    QTest::newRow("as real pages are: a line break between every two") << QByteArray("<!DOCTYPE html>\n<html lang=\"en\">\n  <head>@\n    <title>t</title>\n    <script src=\"a.js\" type=\"module\"></script>\n  </head>\n<body></body></html>");
    QTest::newRow("carriage returns and tabs") << QByteArray("<!doctype html>\r\n<html>\r\n\t<head>@\r\n<script src=a.js></script>");
    QTest::newRow("in capitals") << QByteArray("<HTML><HEAD>@<SCRIPT SRC=a.js></SCRIPT>");
    QTest::newRow("a head with attributes over two lines") << QByteArray("<html><head\n class=x>@<script src=a.js></script>");
    QTest::newRow("a '>' in a quoted value") << QByteArray("<html lang=\"a>b\" data-x='c>d'><head>@<script src=a.js></script>");
    QTest::newRow("a quote which opens no value") << QByteArray("<html lang=a\"b><head>@<meta name=\"x\">");
    QTest::newRow("a quote inside a value which began without one") << QByteArray("<html a=b=\">@<script>const x=\"a>b\";</script>");
    QTest::newRow("a name which begins with '='") << QByteArray("<html =\"x>@\"><head>");
    QTest::newRow("blanks around the '='") << QByteArray("<html a = \"x>y\"><head>@<script></script>");
    QTest::newRow("a name right after a quoted value") << QByteArray("<html a=\"x\"b='y>z'><head>@");
    QTest::newRow("a name and no value, then a quote in a name") << QByteArray("<html hidden a\"b>@<script>var s = \"<head>\";</script>");
    QTest::newRow("a slash before a value") << QByteArray("<html a/=\"x>@y\">");
    QTest::newRow("a doctype with quoted identifiers") << QByteArray("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\"><html><head>@");
    QTest::newRow("a comment which speaks of a head, with a quote in it") << QByteArray("<!-- <head> isn't here -->\n<!DOCTYPE html>\n<!-- nor \"here\" -->\n<html>\n<!-- 3 --><head>@<script></script>");
    QTest::newRow("comments which end at once") << QByteArray("<!--><!---><!----><head>@");
    QTest::newRow("a comment of nothing, and a later one") << QByteArray("<!--><head>@<!-- x -->");
    QTest::newRow("a comment of a dash, and a later one") << QByteArray("<!---><head>@<!-- x -->");
    QTest::newRow("a comment ended with a bang") << QByteArray("<!-- a --!><head>@<!-- b -->");
    QTest::newRow("a processing instruction") << QByteArray("<?xml version=\"1.0\"?><html><head>@");
    QTest::newRow("a head and no html") << QByteArray("<!doctype html><head>@<script src=a.js></script></head>");
    QTest::newRow("no head") << QByteArray("<!doctype html>\n@<body><script src=a.js></script></body>");
    QTest::newRow("no head, a script first") << QByteArray("<html>@<script src=a.js></script>");
    QTest::newRow("a header is no head") << QByteArray("<html>@<header>h</header>");
    QTest::newRow("an htmlx is no html") << QByteArray("@<htmlx><head>");
    QTest::newRow("a second html is not skipped") << QByteArray("<html>@<html><head>");
    QTest::newRow("a base of another origin comes after the line") << QByteArray("<html><head>@<base href=\"https://other.example/\"><script src=a.js></script>");
    QTest::newRow("a self closed head") << QByteArray("<head/>@<body>");
    QTest::newRow("nothing") << QByteArray("@");
    QTest::newRow("blanks") << QByteArray("  \n@");
    QTest::newRow("text") << QByteArray("@hello");
    QTest::newRow("after the mark of UTF-8") << QByteArray("\xEF\xBB\xBF<!doctype html><html><head>@");
    QTest::newRow("after the mark of UTF-8, with nothing") << QByteArray("\xEF\xBB\xBF@");
    QTest::newRow("UTF-16, little end first") << QByteArray("\xFF\xFE<\0h\0t\0m\0l\0>\0", 14);
    QTest::newRow("UTF-16, big end first") << QByteArray("\xFE\xFF\0<\0h\0t\0m\0l\0>", 14);
    QTest::newRow("UTF-16 which begins with no ASCII, little end first") << QByteArray("\xFF\xFE\x42\x30\x44\x30<\0", 8);
    QTest::newRow("UTF-16 which begins with no ASCII, big end first") << QByteArray("\xFE\xFF\x30\x42\x30\x44\0<", 8);
    QTest::newRow("UTF-16 without a mark") << QByteArray("<\0h\0t\0m\0l\0>\0", 12);
    QTest::newRow("UTF-32, big end first") << QByteArray("\0\0\xFE\xFF\0\0\0<", 8);
    QTest::newRow("a comment which does not end") << QByteArray("<!doctype html><!-- <head>");
    QTest::newRow("a comment which ends in a dash too few") << QByteArray("<!-- a ->");
    QTest::newRow("an html which does not end") << QByteArray("<html lang=\"en");
    QTest::newRow("a head which ends inside a quoted value") << QByteArray("<html><head class=\"a>");
    QTest::newRow("a doctype which does not end") << QByteArray("<!doctype html");
}

void tst_extensioncopy::whereAPageNamesItsShim(){
    QFETCH(QByteArray, marked);
    const int mark = marked.indexOf('@');
    QByteArray page = marked;
    if(mark >= 0) page.remove(mark, 1);
    QCOMPARE(ExtensionCopy::PlaceOfPageShim(page), mark);
}

void tst_extensioncopy::thePagesOfTheCopyNameTheirShimAndNoKeyIsInIt(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    QJsonObject manifest = Manifest();
    manifest[QStringLiteral("sandbox")] = QJsonDocument::fromJson(R"({ "pages": ["pages/sandboxed.html", "/pages/Other.html", "pages/sand/*.html"] })").object();
    Extension(source, manifest);
    const QByteArray options = "<!DOCTYPE html>\n<html>\n  <head>\n    <meta charset=\"UTF-8\">\n    <script src=\"options.js\" type=\"module\"></script>\n  </head>\n  <body>\xE8\xA8\xAD\xE5\xAE\x9A</body>\n</html>\n";
    const QByteArray wide("\xFF\xFE<\0h\0e\0a\0d\0>\0", 14), open = "<!-- <head>";
    Write(source + QStringLiteral("/pages/options.html"), options);
    Write(source + QStringLiteral("/POPUP.HTM"), "<body>popup</body>");
    Write(source + QStringLiteral("/pages/sandboxed.html"), options);
    Write(source + QStringLiteral("/pages/other.html"), options);
    Write(source + QStringLiteral("/pages/sand/a.html"), options);
    Write(source + QStringLiteral("/pages/sand/deep/B.HTML"), options);
    Write(source + QStringLiteral("/pages/sandy.html"), options);
    Write(source + QStringLiteral("/pages/wide.html"), wide);
    Write(source + QStringLiteral("/pages/open.html"), open);
    Write(source + QStringLiteral("/pages/page.xhtml"), options);
    Write(source + QStringLiteral("/pages/html.js"), "var html = '<head>';\n");
    const QStringList before = Tree(source);

    const QByteArray key = QByteArray(64, 'c'), place = "__VANILLA_HOST_KEY__";
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"), key);
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QCOMPARE(Tree(source), before);

    const QByteArray line = "<script src=\"/vanilla_page_shim.js\"></script>";
    QCOMPARE(ExtensionCopy::PageShimLine(), line);
    QByteArray expected = options;
    expected.insert(expected.indexOf("<head>") + 6, line);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/options.html")), expected);
    QVERIFY(expected.indexOf(line) < expected.indexOf("options.js"));
    QCOMPARE(Read(made.path + QStringLiteral("/POPUP.HTM")), line + "<body>popup</body>");

    QCOMPARE(Read(made.path + QStringLiteral("/pages/sandboxed.html")), options);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/other.html")), options);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/sand/a.html")), options);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/sand/deep/B.HTML")), options);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/sandy.html")), expected);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/wide.html")), wide);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/open.html")), open);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/page.xhtml")), options);
    QCOMPARE(Read(made.path + QStringLiteral("/pages/html.js")), QByteArray("var html = '<head>';\n"));

    QVERIFY(Read(made.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js")).contains(key));
    const QByteArray shim = Read(made.path + QStringLiteral("/vanilla_page_shim.js"));
    QCOMPARE(shim, Cdp::PageShim().toUtf8().replace(place, key) + ";\n");
    QVERIFY(!shim.contains(place));
    const QByteArray open_ = Read(made.path + QStringLiteral("/vanilla_page_shim_open.js"));
    QCOMPARE(open_, Cdp::PageShim().toUtf8() + ";\n");
    QVERIFY(open_.contains(place) && !open_.contains(key));
    QVERIFY(!Read(made.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js")).startsWith("try { console.warn"));
}

void tst_extensioncopy::whatIsLongerThanAPageIsLeftAsItIs(){
    const int limit = 8 * 1024 * 1024;
    QByteArray page = QByteArray("<head>") + QByteArray(limit - 6, ' ');
    QCOMPARE(ExtensionCopy::PlaceOfPageShim(page), 6);
    page.append(' ');
    QCOMPARE(ExtensionCopy::PlaceOfPageShim(page), -1);
}

void tst_extensioncopy::aCopyMadeTheWayOfBeforeIsNotTakenForOne(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    const QString mark = made.path + QStringLiteral("/.vanilla-copy-complete"), shim = made.path + QStringLiteral("/vanilla_page_shim.js");
    const QByteArray whole = Read(mark), stamp = QFileInfo(made.path).fileName().toLatin1();
    QVERIFY(whole.contains(stamp) && whole != stamp);

    QVERIFY(QFile::remove(shim));
    Write(mark, stamp);
    ExtensionCopy::ForgetForTesting();
    const ExtensionCopy::Made again = ExtensionCopy::Make(source, root, QStringLiteral("idid"));
    QCOMPARE(again.path, made.path);
    QVERIFY(QFile::exists(shim));
    QCOMPARE(Read(mark), whole);

    QVERIFY(QFile::remove(shim));
    Write(mark, QByteArray("copy format 6 ") + stamp);
    ExtensionCopy::ForgetForTesting();
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("idid")).path, made.path);
    QVERIFY(QFile::exists(shim));
    QCOMPARE(Read(mark), whole);

    Write(made.path + QStringLiteral("/lib/types.js"), "touched");
    ExtensionCopy::ForgetForTesting();
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("idid")).path, made.path);
    QCOMPARE(Read(made.path + QStringLiteral("/lib/types.js")), QByteArray("touched"));
}

void tst_extensioncopy::aPatternOfTheManifestsIsReadAsTheEngineReadsIt_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("matches");
    QTest::newRow("itself") << "pages/a.html" << "pages/a.html" << true;
    QTest::newRow("another") << "pages/a.html" << "pages/b.html" << false;
    QTest::newRow("a star for a run") << "pages/*.html" << "pages/deep/a.html" << true;
    QTest::newRow("a star for nothing") << "pages/*a.html" << "pages/a.html" << true;
    QTest::newRow("everything") << "*" << "vanilla_page_shim.js" << true;
    QTest::newRow("two stars") << "*shim*" << "vanilla_page_shim.js" << true;
    QTest::newRow("a star which must not be a run") << "*.js" << "a.jsx" << false;
    QTest::newRow("a question mark is a letter") << "a?.js" << "ab.js" << false;
    QTest::newRow("a question mark is a letter, itself") << "a?.js" << "a?.js" << true;
    QTest::newRow("in any case") << "VANILLA_PAGE_SHIM.JS" << "vanilla_page_shim.js" << true;
    QTest::newRow("in any case, the other way") << "pages/*.html" << "PAGES/A.HTML" << true;
    QTest::newRow("one leading slash") << "/vanilla_page_shim.js" << "vanilla_page_shim.js" << true;
    QTest::newRow("every leading slash") << "///vanilla_page_shim.js" << "vanilla_page_shim.js" << true;
    QTest::newRow("a folder and its contents") << "background/*" << "background/vanilla_worker_shim.js" << true;
    QTest::newRow("a folder pattern matches the folder itself") << "x/*" << "x" << true;
    QTest::newRow("a file pattern as a folder") << "vanilla_page_shim.js/*" << "vanilla_page_shim.js" << true;
    QTest::newRow("a folder pattern does not match a sibling") << "x/*" << "xy" << false;
    QTest::newRow("empty pattern") << "" << "a" << false;
    QTest::newRow("empty path") << "*" << "" << true;
}

void tst_extensioncopy::aPatternOfTheManifestsIsReadAsTheEngineReadsIt(){
    QFETCH(QString, pattern);
    QFETCH(QString, path);
    QFETCH(bool, matches);
    QCOMPARE(ExtensionCopy::Globbed(pattern, path), matches);
}

void tst_extensioncopy::whatTheWebMayReadGetsNoKey_data(){
    QTest::addColumn<QString>("accessible");
    QTest::addColumn<bool>("keyless");
    QTest::newRow("none") << "" << false;
    QTest::newRow("empty") << "[]" << false;
    QTest::newRow("as Vimium: pages by name") << R"([{"matches": ["<all_urls>"], "resources": ["pages/vomnibar_page.html", "content_scripts/vimium.css", "pages/hud_page.html", "_favicon/*"]}])" << false;
    QTest::newRow("as Stands: a folder") << R"([{"matches": ["<all_urls>"], "resources": ["content/popups-script.js", "views/web_accessible/*"]}])" << false;
    QTest::newRow("everything") << R"([{"matches": ["<all_urls>"], "resources": ["*"]}])" << true;
    QTest::newRow("every script") << R"([{"matches": ["<all_urls>"], "resources": ["*.js"]}])" << true;
    QTest::newRow("the pages' shim by name") << R"([{"matches": ["<all_urls>"], "resources": ["/vanilla_page_shim.js"]}])" << true;
    QTest::newRow("the pages' shim by name, two slashes") << R"([{"matches": ["<all_urls>"], "resources": ["//vanilla_page_shim.js"]}])" << true;
    QTest::newRow("the pages' shim in capitals") << R"([{"matches": ["<all_urls>"], "resources": ["VANILLA_PAGE_SHIM.JS"]}])" << true;
    QTest::newRow("the pages' shim as a folder") << R"([{"matches": ["<all_urls>"], "resources": ["vanilla_page_shim.js/*"]}])" << true;
    QTest::newRow("the worker's folder") << R"([{"matches": ["<all_urls>"], "resources": ["background_scripts/*"]}])" << true;
    QTest::newRow("the second entry") << R"([{"matches": ["<all_urls>"], "resources": ["icons/*"]}, {"matches": ["https://a.example/*"], "resources": ["*"]}])" << true;
    QTest::newRow("for other extensions only") << R"([{"extension_ids": ["abcdefghijklmnopabcdefghijklmnop"], "resources": ["*"]}])" << true;
    QTest::newRow("under a dynamic url") << R"([{"matches": ["<all_urls>"], "resources": ["*"], "use_dynamic_url": true}])" << true;
    QTest::newRow("not a list") << R"("*")" << true;
    QTest::newRow("an entry which is no object") << R"(["*"])" << true;
    QTest::newRow("an entry without resources") << R"([{"matches": ["<all_urls>"]}])" << true;
    QTest::newRow("resources which are no list") << R"([{"matches": ["<all_urls>"], "resources": "*"}])" << true;
    QTest::newRow("a resource which is no string") << R"([{"matches": ["<all_urls>"], "resources": [1]}])" << true;
    QTest::newRow("a short name") << R"([{"matches": ["<all_urls>"], "resources": ["__VANI~*.JS"]}])" << true;
    QTest::newRow("a short name of a folder") << R"([{"matches": ["<all_urls>"], "resources": ["BACKGR~1/*"]}])" << true;
    QTest::newRow("a folder with a trailing dot") << R"([{"matches": ["<all_urls>"], "resources": ["background_scripts./*"]}])" << true;
    QTest::newRow("a folder with a trailing blank") << R"([{"matches": ["<all_urls>"], "resources": ["background_scripts /*"]}])" << true;
    QTest::newRow("a stream") << R"([{"matches": ["<all_urls>"], "resources": ["background_scripts/*::$DATA"]}])" << true;
    QTest::newRow("a stream in a harmless folder") << R"([{"matches": ["<all_urls>"], "resources": ["icons/*::$DATA"]}])" << false;
    QTest::newRow("a tilde in a harmless folder") << R"([{"matches": ["<all_urls>"], "resources": ["icons/~x.png"]}])" << false;
    QTest::newRow("a dot inside a name is no alias") << R"([{"matches": ["<all_urls>"], "resources": ["icons/x.min.png"]}])" << false;
    QTest::newRow("the pages' shim, escaped") << R"([{"matches": ["<all_urls>"], "resources": ["%5F%5Fvanilla%5Fpage%5Fshim.js"]}])" << true;
    QTest::newRow("the pages' shim, escaped and starred") << R"([{"matches": ["<all_urls>"], "resources": ["%5*%5*vanilla%5*page%5*shim.js"]}])" << true;
    QTest::newRow("a short name, escaped") << R"([{"matches": ["<all_urls>"], "resources": ["__VANI%7E2.JS"]}])" << true;
    QTest::newRow("a short name, starred") << R"([{"matches": ["<all_urls>"], "resources": ["__VANI*2.JS"]}])" << true;
    QTest::newRow("a folder with a trailing dot, starred") << R"([{"matches": ["<all_urls>"], "resources": ["background_scripts.*/vanilla_worker_shim.js"]}])" << true;
    QTest::newRow("a star, escaped") << R"([{"matches": ["<all_urls>"], "resources": ["%2A"]}])" << true;
    QTest::newRow("a stream, escaped") << R"([{"matches": ["<all_urls>"], "resources": ["icons/x.png%3A%3A%24DATA"]}])" << false;
    QTest::newRow("a harmless name, escaped") << R"([{"matches": ["<all_urls>"], "resources": ["pages/%76omnibar_page.html"]}])" << false;
    QTest::newRow("a star in the middle, harmless") << R"([{"matches": ["<all_urls>"], "resources": ["icons/*.png"]}])" << false;
    QTest::newRow("a star in the middle of a folder") << R"([{"matches": ["<all_urls>"], "resources": ["icons/*/x.png"]}])" << false;
    QTest::newRow("a question mark") << R"([{"matches": ["<all_urls>"], "resources": ["icons/x?.png"]}])" << false;
    QTest::newRow("a blank, still") << R"([{"matches": ["<all_urls>"], "resources": ["my icons/x.png"]}])" << true;
    QTest::newRow("a letter beyond ASCII") << "[{\"matches\": [\"<all_urls>\"], \"resources\": [\"icons/\u00e9.png\"]}]" << false;
    QTest::newRow("a letter beyond ASCII in the first folder, still") << "[{\"matches\": [\"<all_urls>\"], \"resources\": [\"\u00e9/x.png\"]}]" << true;
    QTest::newRow("a dot part") << R"([{"matches": ["<all_urls>"], "resources": ["icons/./x.png"]}])" << false;
    QTest::newRow("a dot dot part") << R"([{"matches": ["<all_urls>"], "resources": ["icons/../x.png"]}])" << false;
    QTest::newRow("an empty part") << R"([{"matches": ["<all_urls>"], "resources": ["icons//x.png"]}])" << false;
    QTest::newRow("a star in the worker's folder") << R"([{"matches": ["<all_urls>"], "resources": ["background_scripts/*.png"]}])" << true;
    QTest::newRow("a star in the worker's folder, in capitals") << R"([{"matches": ["<all_urls>"], "resources": ["BACKGROUND_SCRIPTS/x*"]}])" << true;
    QTest::newRow("as DeepL 1.99.0") << R"([{"matches": ["<all_urls>"], "resources": ["images/*.svg", "images/**/*.svg"]}, {"matches": ["<all_urls>"], "resources": ["icons/sprite.svg"]}, {"matches": ["<all_urls>"], "resources": ["build/content.css", "build/deepl-ui.shadow.css"]}, {"matches": ["<all_urls>"], "resources": ["onboarding.html"]}, {"matches": ["chrome-extension://ocpdpnakdghopjcifldjidbdmfobmmoi/*"], "resources": ["tesseract/*"]}])" << false;
    QTest::newRow("a folder pattern of a folder which is not there") << R"([{"matches": ["<all_urls>"], "resources": ["nowhere/*"]}])" << false;
    QTest::newRow("the pages' shim of the open kind") << R"([{"matches": ["<all_urls>"], "resources": ["vanilla_page_shim_open.js"]}])" << false;
    QTest::newRow("the content shim, which carries no key") << R"([{"matches": ["<all_urls>"], "resources": ["vanilla_content_shim.js"]}])" << false;
}

void tst_extensioncopy::whatTheWebMayReadGetsNoKey(){
    QFETCH(QString, accessible);
    QFETCH(bool, keyless);
    QJsonObject manifest = Manifest();
    if(!accessible.isEmpty()){
        const QJsonDocument doc = QJsonDocument::fromJson(("{\"w\": " + accessible + "}").toUtf8());
        manifest[QStringLiteral("web_accessible_resources")] = doc.object().value(QStringLiteral("w"));
    }
    const ExtensionCopy::Rewritten r = ExtensionCopy::Rewrite(manifest, 7);
    QVERIFY2(r.refusal.isEmpty(), qPrintable(r.refusal));
    QCOMPARE(r.keyless, keyless);

    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source, manifest);
    const QByteArray key = QByteArray(64, 'c'), place = "__VANILLA_HOST_KEY__";
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"), key);
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QCOMPARE(made.withheld, keyless);
    const QByteArray worker = Read(made.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js"));
    const QByteArray page = Read(made.path + QStringLiteral("/vanilla_page_shim.js"));
    QCOMPARE(worker.contains(key), !keyless);
    QCOMPARE(page.contains(key), !keyless);
    QCOMPARE(worker.contains(place), keyless);
    QCOMPARE(page.contains(place), keyless);
    QCOMPARE(worker.startsWith("try { console.warn('Vanilla: the shims of this extension were written without their key"), keyless);
    QVERIFY(worker.endsWith(Cdp::WorkerShim().toUtf8().replace(place, keyless ? place : key) + ";\n"));
    ExtensionCopy::ForgetForTesting();
    const ExtensionCopy::Made unkeyed = ExtensionCopy::Make(source, dir.filePath(QStringLiteral("copies2")), QStringLiteral("idid"));
    QVERIFY(!Read(unkeyed.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js")).startsWith("try { console.warn"));
    QVERIFY(!unkeyed.withheld);
}

void tst_extensioncopy::aPageTheWebMayEmbedNamesTheShimWithoutTheKey(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    QJsonObject manifest = Manifest();
    manifest[QStringLiteral("web_accessible_resources")] = QJsonDocument::fromJson(
        R"([{"matches": ["<all_urls>"], "resources": ["pages/vomnibar_page.html", "/pages/HUD_page.html", "views/web_accessible/*"]}])").array();
    Extension(source, manifest);
    const QByteArray html = "<html><head><script src=\"a.js\"></script></head></html>";
    foreach(const QString &page, QStringList() << QStringLiteral("pages/options.html") << QStringLiteral("pages/vomnibar_page.html")
                                               << QStringLiteral("pages/hud_page.html") << QStringLiteral("views/web_accessible/block-element/view.html")
                                               << QStringLiteral("views/other.html"))
        Write(source + QLatin1Char('/') + page, html);
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"), QByteArray(64, 'c'));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QVERIFY(Read(made.path + QStringLiteral("/vanilla_page_shim.js")).contains(QByteArray(64, 'c')));

    const QByteArray keyedLine = "<script src=\"/vanilla_page_shim.js\"></script>", openLine = "<script src=\"/vanilla_page_shim_open.js\"></script>";
    QCOMPARE(ExtensionCopy::PageShimLine(), keyedLine);
    QCOMPARE(ExtensionCopy::PageShimLine(true), openLine);
    auto lineOf = [&](const QString &page){ const QByteArray text = Read(made.path + QLatin1Char('/') + page); return text.mid(12, text.indexOf("</script>") + 9 - 12); };
    QCOMPARE(lineOf(QStringLiteral("pages/options.html")), keyedLine);
    QCOMPARE(lineOf(QStringLiteral("views/other.html")), keyedLine);
    QCOMPARE(lineOf(QStringLiteral("pages/vomnibar_page.html")), openLine);
    QCOMPARE(lineOf(QStringLiteral("pages/hud_page.html")), openLine);
    QCOMPARE(lineOf(QStringLiteral("views/web_accessible/block-element/view.html")), openLine);

    foreach(const QString &pattern, QStringList() << QStringLiteral("*.html") << QStringLiteral("%5*%5*vanilla%5*page%5*shim.js")){
        QJsonObject wide = Manifest();
        wide[QStringLiteral("web_accessible_resources")] = QJsonDocument::fromJson(
            QStringLiteral(R"([{"matches": ["<all_urls>"], "resources": ["%1"]}])").arg(pattern).toUtf8()).array();
        const QString each = dir.filePath(QStringLiteral("ext-") + QString::number(qHash(pattern)));
        Extension(each, wide);
        Write(each + QStringLiteral("/pages/options.html"), html);
        Write(each + QStringLiteral("/views/other.html"), html);
        ExtensionCopy::ForgetForTesting();
        const ExtensionCopy::Made m = ExtensionCopy::Make(each, dir.filePath(QStringLiteral("copies-") + QString::number(qHash(pattern))), QStringLiteral("idid"), QByteArray(64, 'c'));
        QVERIFY2(m.note.isEmpty(), qPrintable(m.note));
        QVERIFY(!Read(m.path + QStringLiteral("/vanilla_page_shim.js")).contains(QByteArray(64, 'c')));
        foreach(const QString &page, QStringList() << QStringLiteral("pages/options.html") << QStringLiteral("views/other.html")){
            const QByteArray text = Read(m.path + QLatin1Char('/') + page);
            QVERIFY2(text.contains(openLine) && !text.contains(keyedLine), qPrintable(pattern + QLatin1Char(' ') + page));
        }
    }
    {
        QJsonObject behind = Manifest();
        behind[QStringLiteral("web_accessible_resources")] = QJsonDocument::fromJson(
            R"([{"matches": ["<all_urls>"], "resources": ["pages/*.svg"]}])").array();
        const QString each = dir.filePath(QStringLiteral("ext-behind"));
        Extension(each, behind);
        Write(each + QStringLiteral("/pages/options.html"), html);
        Write(each + QStringLiteral("/views/other.html"), html);
        ExtensionCopy::ForgetForTesting();
        const ExtensionCopy::Made m = ExtensionCopy::Make(each, dir.filePath(QStringLiteral("copies-behind")), QStringLiteral("idid"), QByteArray(64, 'c'));
        QVERIFY2(m.note.isEmpty(), qPrintable(m.note));
        QVERIFY(!m.keyless);
        QVERIFY(Read(m.path + QStringLiteral("/vanilla_page_shim.js")).contains(QByteArray(64, 'c')));
        QVERIFY(Read(m.path + QStringLiteral("/pages/options.html")).contains(openLine));
        QVERIFY(Read(m.path + QStringLiteral("/views/other.html")).contains(keyedLine));
    }
}

void tst_extensioncopy::thePolicyLetsTheApplicationsSchemeThrough_data(){
    QTest::addColumn<QString>("policy");
    QTest::addColumn<QString>("widened");
    QTest::newRow("as Dark Reader") << "default-src 'none'; script-src 'self'; style-src 'self'; img-src * data:; connect-src *; navigate-to 'self'; media-src 'none'"
                                    << "default-src 'none'; script-src 'self'; style-src 'self'; img-src * data:; connect-src * vanilla-extension:; navigate-to 'self'; media-src 'none'";
    QTest::newRow("connect-src of nothing") << "connect-src 'none'" << "connect-src vanilla-extension:";
    QTest::newRow("connect-src of self") << "script-src 'self'; connect-src 'self' https://a.example" << "script-src 'self'; connect-src 'self' https://a.example vanilla-extension:";
    QTest::newRow("by fallback to default-src") << "default-src 'self'; script-src 'self'" << "default-src 'self'; script-src 'self'; connect-src 'self' vanilla-extension:";
    QTest::newRow("by fallback, of nothing") << "default-src 'none'; script-src 'self';" << "default-src 'none'; script-src 'self'; connect-src vanilla-extension:";
    QTest::newRow("neither: no restriction") << "script-src 'self'; object-src 'self'" << "script-src 'self'; object-src 'self'";
    QTest::newRow("the scheme is there already") << "connect-src * vanilla-extension:" << "connect-src * vanilla-extension:";
    QTest::newRow("the scheme is there already, in capitals") << "connect-src VANILLA-EXTENSION: *" << "connect-src VANILLA-EXTENSION: *";
    QTest::newRow("the name in capitals, with blanks") << "  Connect-Src   *  ;script-src 'self'" << "Connect-Src * vanilla-extension:;script-src 'self'";
    QTest::newRow("the first connect-src, not the second") << "connect-src 'self'; connect-src *" << "connect-src 'self' vanilla-extension:; connect-src *";
    QTest::newRow("empty") << "" << "";
}

void tst_extensioncopy::thePolicyLetsTheApplicationsSchemeThrough(){
    QFETCH(QString, policy);
    QFETCH(QString, widened);
    QCOMPARE(ExtensionCopy::ConnectingPolicy(policy), widened);
    QCOMPARE(ExtensionCopy::ConnectingPolicy(widened), widened);

    QJsonObject manifest = Manifest();
    QJsonObject policies; policies[QStringLiteral("extension_pages")] = policy; policies[QStringLiteral("sandbox")] = QStringLiteral("sandbox allow-scripts");
    manifest[QStringLiteral("content_security_policy")] = policies;
    const ExtensionCopy::Rewritten r = ExtensionCopy::Rewrite(manifest, 7);
    QVERIFY2(r.refusal.isEmpty(), qPrintable(r.refusal));
    const QJsonObject out = r.manifest[QStringLiteral("content_security_policy")].toObject();
    QCOMPARE(out[QStringLiteral("extension_pages")].toString(), widened);
    QCOMPARE(out[QStringLiteral("sandbox")].toString(), QStringLiteral("sandbox allow-scripts"));
    QCOMPARE(out.keys(), QStringList() << QStringLiteral("extension_pages") << QStringLiteral("sandbox"));
    QVERIFY(!ExtensionCopy::Rewrite(Manifest(), 7).manifest.contains(QStringLiteral("content_security_policy")));
}

void tst_extensioncopy::whatAPatternReachesIsReadByItsLetter_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("readable");
    QTest::newRow("itself") << "pages/a.html" << "pages/a.html" << true;
    QTest::newRow("itself, in any case") << "PAGES/A.HTML" << "pages/a.html" << true;
    QTest::newRow("itself, with a leading slash") << "//pages/a.html" << "pages/a.html" << true;
    QTest::newRow("another") << "pages/a.html" << "pages/b.html" << false;
    QTest::newRow("a prefix which is no folder") << "pages/a" << "pages/a.html" << false;
    QTest::newRow("under the folder") << "pages/*" << "pages/deep/a.html" << true;
    QTest::newRow("the folder itself") << "pages/*" << "pages" << true;
    QTest::newRow("a sibling of the folder") << "pages/*" << "pagesx/a.html" << false;
    QTest::newRow("above the folder") << "pages/deep/*" << "pages/a.html" << false;
    QTest::newRow("everything") << "*" << "vanilla_page_shim.js" << true;
    QTest::newRow("everything, with a slash") << "/*" << "vanilla_page_shim.js" << true;
    QTest::newRow("a star in the middle") << "*.js" << "nothing" << true;
    QTest::newRow("a tilde") << "PAGES~1/a.html" << "nothing" << true;
    QTest::newRow("a trailing dot") << "pages./a.html" << "nothing" << true;
    QTest::newRow("a backslash") << "pages\\a.html" << "nothing" << true;
    QTest::newRow("an empty pattern") << "" << "nothing" << true;
    QTest::newRow("a slash alone") << "/" << "nothing" << true;
    QTest::newRow("a star in the first folder") << "pages*/a.html" << "nothing" << true;
    QTest::newRow("a trailing dot, starred (R-158)") << "background_scripts.*/x" << "nothing" << true;
    QTest::newRow("a dot inside a name") << "pages/a.b.html" << "pages/a.b.html" << true;
    QTest::newRow("a name which begins with a dash") << "pages/-a.html" << "pages/-a.html" << true;
    QTest::newRow("a star at the end of a name") << "pages/a*" << "nothing" << false;
    QTest::newRow("a star at the end of a name, under it") << "pages/a*" << "pages/b.html" << true;
    QTest::newRow("two stars") << "pages/*/*" << "nothing" << false;
    QTest::newRow("two stars, under the folder") << "pages/*/*" << "pages/a.html" << true;
    QTest::newRow("an escape") << "pages/a%2Ehtml" << "nothing" << false;
    QTest::newRow("an escape, under the folder") << "pages/a%2Ehtml" << "pages/x.js" << true;
    QTest::newRow("a colon") << "pages/a.html::$DATA" << "nothing" << false;
    QTest::newRow("a dot part") << "pages/./a.html" << "nothing" << false;
    QTest::newRow("three dots") << "pages/.../a.html" << "nothing" << false;
    QTest::newRow("three dots, the folder itself") << "pages/.../a.html" << "pages" << true;
    QTest::newRow("an empty part before the star") << "pages//*" << "nothing" << false;
    QTest::newRow("a folder with a slash alone after it") << "pages/" << "nothing" << false;
    QTest::newRow("a folder with a slash alone after it, the folder") << "pages/" << "pages" << true;
    QTest::newRow("a line break") << "pages/a\nb.html" << "nothing" << false;
    QTest::newRow("as DeepL") << "images/*.svg" << "vanilla_worker_shim.js" << false;
    QTest::newRow("as DeepL, under the folder") << "images/*.svg" << "images/a/b.svg" << true;
    QTest::newRow("as DeepL, a sibling of the folder") << "images/*.svg" << "imagesx/a.svg" << false;
    QTest::newRow("as DeepL, two stars") << "images/**/*.svg" << "vanilla_page_shim.js" << false;
    QTest::newRow("the folders ahead of the star") << "pages/deep/a*.html" << "pages/a.html" << false;
    QTest::newRow("the folders ahead of the star, under them") << "pages/deep/a*.html" << "pages/deep/x/y.js" << true;
    QTest::newRow("a trailing dot after a folder") << "pages/deep./a.html" << "pages/x.js" << true;
    QTest::newRow("a trailing dot after a folder, beside it") << "pages/deep./a.html" << "vanilla_page_shim.js" << false;
    QTest::newRow("a tilde after a folder") << "pages/DEEP~1/a.html" << "pages/deep/a.html" << true;
    QTest::newRow("a tilde after a folder, beside it") << "pages/DEEP~1/a.html" << "a.html" << false;
    QTest::newRow("a folder in capitals") << "Images/*.svg" << "images/x.svg" << true;
}

void tst_extensioncopy::whatAPatternReachesIsReadByItsLetter(){
    QFETCH(QString, pattern);
    QFETCH(QString, path);
    QFETCH(bool, readable);
    QCOMPARE(ExtensionCopy::Readable(QStringList() << pattern, path), readable);
    QCOMPARE(ExtensionCopy::Readable(QStringList() << QStringLiteral("icons/x.png") << pattern << QStringLiteral("nowhere/*"), path), readable);
    QCOMPARE(ExtensionCopy::Readable(QStringList(), path), false);
}

void tst_extensioncopy::theMessagesOfTheUsersLocaleGoAheadOfTheShims(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    QJsonObject manifest = Manifest();
    manifest[QStringLiteral("default_locale")] = QStringLiteral("en");
    Extension(source, manifest);
    Write(source + QStringLiteral("/_locales/en/messages.json"),
          "\xEF\xBB\xBF{ \"Greeting\": { \"message\": \"Hello $NAME$\", \"placeholders\": { \"NAME\": { \"content\": \"$1\", \"example\": \"Ann\" } } },"
          "  \"onlyEn\": { \"message\": \"en\" }, \"broken\": { \"message\": 5 }, \"noMessage\": { \"description\": \"x\" } }");
    Write(source + QStringLiteral("/_locales/ja/messages.json"), "{ \"GREETING\": { \"message\": \"Konnichiwa $1\" } }");
    Write(source + QStringLiteral("/pages/options.html"), "<!doctype html><html><head><title>o</title></head></html>");

    const QJsonObject ja = ExtensionCopy::Messages(source, manifest, QStringLiteral("ja_JP"));
    QCOMPARE(ja.keys(), QStringList() << QStringLiteral("greeting") << QStringLiteral("onlyen"));
    QCOMPARE(ja.value(QStringLiteral("greeting")).toObject().value(QStringLiteral("message")).toString(), QStringLiteral("Konnichiwa $1"));
    QVERIFY(!ja.value(QStringLiteral("greeting")).toObject().contains(QStringLiteral("placeholders")));
    const QJsonObject de = ExtensionCopy::Messages(source, manifest, QStringLiteral("de_DE"));
    QCOMPARE(de.value(QStringLiteral("greeting")).toObject().value(QStringLiteral("message")).toString(), QStringLiteral("Hello $NAME$"));
    QCOMPARE(QJsonDocument(de.value(QStringLiteral("greeting")).toObject().value(QStringLiteral("placeholders")).toObject()).toJson(QJsonDocument::Compact),
             QByteArray("{\"name\":{\"content\":\"$1\"}}"));
    QCOMPARE(ExtensionCopy::Messages(source, manifest, QString()).keys(), de.keys());
    QCOMPARE(ExtensionCopy::Messages(source, manifest, QStringLiteral("../ja")).keys(), de.keys());
    QVERIFY(ExtensionCopy::Messages(source, Manifest(), QStringLiteral("ja_JP")).isEmpty());
    QVERIFY(ExtensionCopy::MessagesLine(QJsonObject(), QStringLiteral("ja_JP")).isEmpty());

    const QByteArray key(64, 'a');
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"), key, QStringLiteral("ja_JP"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    const QByteArray line = ExtensionCopy::MessagesLine(ja, QStringLiteral("ja_JP"));
    QVERIFY(line.startsWith("self.__vanillaMessages = {\"locale\":\"ja_JP\",\"messages\":{"));
    QVERIFY(line.endsWith("};\n"));
    QCOMPARE(Read(made.path + QStringLiteral("/vanilla_content_shim.js")), line + Cdp::ContentShim().toUtf8() + ";\n");
    QCOMPARE(Read(made.path + QStringLiteral("/vanilla_page_shim_open.js")), line + Cdp::PageShim().toUtf8() + ";\n");
    QVERIFY(Read(made.path + QStringLiteral("/vanilla_page_shim.js")).startsWith(line + "(() => {"));
    QVERIFY(Read(made.path + QStringLiteral("/background_scripts/vanilla_worker_shim.js")).startsWith(line + "(() => {"));
    QVERIFY(QFile::exists(made.path + QStringLiteral("/_locales/ja/messages.json")));

    const QByteArray bytes = Read(source + QStringLiteral("/manifest.json"));
    QVERIFY(ExtensionCopy::Stamp(source, bytes, key, QStringLiteral("ja_JP")) != ExtensionCopy::Stamp(source, bytes, key, QStringLiteral("de_DE")));
    ExtensionCopy::ForgetForTesting();
    const ExtensionCopy::Made again = ExtensionCopy::Make(source, root, QStringLiteral("idid"), key, QStringLiteral("de_DE"));
    QVERIFY(again.path != made.path);
    QVERIFY(Read(again.path + QStringLiteral("/vanilla_content_shim.js")).startsWith("self.__vanillaMessages = {\"locale\":\"de_DE\","));
}

void tst_extensioncopy::theBackendIsToldWhenWhatItHoldsIsNotWhatItWouldBeGiven(){
    QVERIFY(ExtensionCopy::Stale(true, false, QString(), QStringLiteral("C:/copies/id/7")));
    QVERIFY(!ExtensionCopy::Stale(false, false, QString(), QStringLiteral("C:/ext")));
    QVERIFY(!ExtensionCopy::Stale(true, true, QStringLiteral("C:/copies/id/7"), QStringLiteral("C:/copies/id/7")));
    QVERIFY(!ExtensionCopy::Stale(true, true, QStringLiteral("C:/copies/id/7/"), QStringLiteral("C:/copies/id/7")));
    QVERIFY(!ExtensionCopy::Stale(false, true, QStringLiteral("C:/ext"), QStringLiteral("C:/ext")));
    QVERIFY(ExtensionCopy::Stale(true, true, QStringLiteral("C:/copies/id/7"), QStringLiteral("C:/copies/id/8")));
    QVERIFY(ExtensionCopy::Stale(false, true, QStringLiteral("C:/copies/id/7"), QStringLiteral("C:/ext")));
    QVERIFY(ExtensionCopy::Stale(true, true, QStringLiteral("C:/ext"), QStringLiteral("C:/copies/id/7")));
    QVERIFY(ExtensionCopy::Stale(true, true, QString(), QStringLiteral("C:/copies/id/7")));
#ifdef Q_OS_WIN
    QVERIFY(!ExtensionCopy::Stale(true, true, QStringLiteral("c:/Copies/ID/7"), QStringLiteral("C:/copies/id/7")));
#endif
}

void tst_extensioncopy::theRelayOfTheWorkerIsInTheKeyedCopyAndInNoOther(){
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QByteArray key(64, 'c');
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"), key);
    QVERIFY(made.path != source);
    QVERIFY(!made.keyless);
    const QByteArray page = Read(made.path + QStringLiteral("/vanilla_relay.html"));
    const QByteArray script = Read(made.path + QStringLiteral("/vanilla_relay.js"));
    QVERIFY(page.contains("<script src=\"/vanilla_relay.js\"></script>"));
    QVERIFY(!page.contains("vanilla_page_shim"));
    QVERIFY(script.contains(key));
    QVERIFY(!script.contains(QByteArray(ExtensionHostWire::KEY_PLACE)));
    QCOMPARE(script, QString(Cdp::RelayScript()).replace(QLatin1String(ExtensionHostWire::KEY_PLACE), QString::fromLatin1(key)).toUtf8() + ";\n");
    QVERIFY(Read(made.path + QStringLiteral("/.vanilla-copy-complete")).startsWith("copy format 13 "));
    const QByteArray wake = Read(made.path + QStringLiteral("/vanilla_wake.html"));
    QVERIFY(wake.contains("<script src=\"/vanilla_wake.js\"></script>"));
    QVERIFY(!wake.contains("vanilla_page_shim"));
    QCOMPARE(Read(made.path + QStringLiteral("/vanilla_wake.js")), Cdp::WakeScript().toUtf8());
    QVERIFY(!Read(made.path + QStringLiteral("/vanilla_wake.js")).contains(key));

    QTemporaryDir bare;
    const QString bareSource = bare.filePath(QStringLiteral("ext"));
    Extension(bareSource);
    const ExtensionCopy::Made unkeyed = ExtensionCopy::Make(bareSource, bare.filePath(QStringLiteral("copies")), QStringLiteral("idid"));
    QVERIFY(unkeyed.path != bareSource);
    QVERIFY(!QFile::exists(unkeyed.path + QStringLiteral("/vanilla_relay.html")));
    QVERIFY(!QFile::exists(unkeyed.path + QStringLiteral("/vanilla_relay.js")));
    QCOMPARE(Read(unkeyed.path + QStringLiteral("/vanilla_wake.js")), Cdp::WakeScript().toUtf8());

    QTemporaryDir open;
    const QString openSource = open.filePath(QStringLiteral("ext"));
    QJsonObject manifest = Manifest();
    manifest[QStringLiteral("web_accessible_resources")] = QJsonArray() << QJsonObject{
        {QStringLiteral("resources"), QJsonArray() << QStringLiteral("vanilla_relay.js")},
        {QStringLiteral("matches"), QJsonArray() << QStringLiteral("<all_urls>")}};
    Extension(openSource, manifest);
    const ExtensionCopy::Made readable = ExtensionCopy::Make(openSource, open.filePath(QStringLiteral("copies")), QStringLiteral("idid"), key);
    QVERIFY(readable.keyless);
    QVERIFY(!QFile::exists(readable.path + QStringLiteral("/vanilla_relay.js")));
    QTemporaryDir openPage;
    const QString pageSource = openPage.filePath(QStringLiteral("ext"));
    manifest[QStringLiteral("web_accessible_resources")] = QJsonArray() << QJsonObject{
        {QStringLiteral("resources"), QJsonArray() << QStringLiteral("vanilla_relay.html")},
        {QStringLiteral("matches"), QJsonArray() << QStringLiteral("<all_urls>")}};
    Extension(pageSource, manifest);
    const ExtensionCopy::Made pageReadable = ExtensionCopy::Make(pageSource, openPage.filePath(QStringLiteral("copies")), QStringLiteral("idid"), key);
    QVERIFY(pageReadable.keyless);
    QVERIFY(!QFile::exists(pageReadable.path + QStringLiteral("/vanilla_relay.html")));
}

void tst_extensioncopy::theWorkersAddressIsTheSameWhateverTheStamp(){
    QJsonObject manifest = Manifest();
    QJsonObject background; background[QStringLiteral("service_worker")] = QStringLiteral("background.js");
    manifest[QStringLiteral("background")] = background;
    const ExtensionCopy::Rewritten seven = ExtensionCopy::Rewrite(manifest, 7);
    const ExtensionCopy::Rewritten eight = ExtensionCopy::Rewrite(manifest, 8);
    QCOMPARE(seven.workerWrapper, eight.workerWrapper);
    QCOMPARE(seven.manifest[QStringLiteral("background")].toObject()[QStringLiteral("service_worker")].toString(),
             eight.manifest[QStringLiteral("background")].toObject()[QStringLiteral("service_worker")].toString());
    QVERIFY(!seven.workerWrapper.contains(QLatin1Char('7')));
    QVERIFY(seven.manifest[QStringLiteral("version")].toString().endsWith(QStringLiteral(".7")));
    QVERIFY(eight.manifest[QStringLiteral("version")].toString().endsWith(QStringLiteral(".8")));
}

void tst_extensioncopy::thePinnedPathStaysWhileTheCopyBehindItChanges(){
#ifndef Q_OS_WIN
    QSKIP("a junction is Windows' own");
#else
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString first = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    const QString pinned = ExtensionCopy::Pin(root, QStringLiteral("idid"), first);
    QCOMPARE(pinned, QDir(root).absolutePath() + QStringLiteral("/idid/edge"));
    QVERIFY(QFileInfo(pinned).isJunction());
    QCOMPARE(QDir::cleanPath(QFileInfo(pinned).junctionTarget()).toLower(), QDir::cleanPath(first).toLower());
    QCOMPARE(Read(pinned + QStringLiteral("/manifest.json")), Read(first + QStringLiteral("/manifest.json")));
    const quint64 entry = EntryOf(pinned);
    QVERIFY(entry != 0);
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), first), pinned);
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), QDir::toNativeSeparators(first).toUpper()), pinned);
    QCOMPARE(EntryOf(pinned), entry);
    QCOMPARE(QDir::cleanPath(QFileInfo(pinned).junctionTarget()).toLower(), QDir::cleanPath(first).toLower());

    ExtensionCopy::ForgetForTesting();
    QJsonObject changed = Manifest(); changed[QStringLiteral("version")] = QStringLiteral("2.4.3");
    Write(source + QStringLiteral("/manifest.json"), QJsonDocument(changed).toJson());
    const QString second = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    QVERIFY(second != first);
    QVERIFY(!QFile::exists(first));
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), second), pinned);
    QVERIFY(QFileInfo(pinned).isJunction());
    QCOMPARE(QDir::cleanPath(QFileInfo(pinned).junctionTarget()).toLower(), QDir::cleanPath(second).toLower());
    QVERIFY(Read(pinned + QStringLiteral("/manifest.json")).contains("2.4.3"));

    const QString other = QDir(root).absolutePath() + QStringLiteral("/idid/12345");
    Write(other + QStringLiteral("/marker"), "x");
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), other), pinned);
    QCOMPARE(Read(pinned + QStringLiteral("/marker")), QByteArray("x"));
    QVERIFY(QFile::exists(second + QStringLiteral("/manifest.json")));
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), second), pinned);
    QVERIFY(QFile::exists(other + QStringLiteral("/marker")));
#endif
}

void tst_extensioncopy::whatIsNotACopyIsNotPinned(){
#ifndef Q_OS_WIN
    QSKIP("a junction is Windows' own");
#else
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString home = QDir(root).absolutePath() + QStringLiteral("/idid");
    Write(root + QStringLiteral("/another/7/manifest.json"), "{}");
    Write(home + QStringLiteral("/named/manifest.json"), "{}");
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), source), source);
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), root + QStringLiteral("/another/7")), root + QStringLiteral("/another/7"));
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), home + QStringLiteral("/named")), home + QStringLiteral("/named"));
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), home + QStringLiteral("/99")), home + QStringLiteral("/99"));
    QCOMPARE(ExtensionCopy::Pin(root, QString(), home + QStringLiteral("/99")), home + QStringLiteral("/99"));
    QVERIFY(!QFileInfo::exists(home + QStringLiteral("/edge")) && !QFileInfo(home + QStringLiteral("/edge")).isJunction());
    const QString copy = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    Write(home + QStringLiteral("/edge/mine"), "x");
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), copy), copy);
    QCOMPARE(Read(home + QStringLiteral("/edge/mine")), QByteArray("x"));
    QVERIFY(!QFileInfo(home + QStringLiteral("/edge")).isJunction());
#endif
}

void tst_extensioncopy::theCopiesBeforeGoAndTheJunctionStaysWithoutBeingWalkedThrough(){
#ifndef Q_OS_WIN
    QSKIP("a junction is Windows' own");
#else
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString copy = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    const QString pinned = ExtensionCopy::Pin(root, QStringLiteral("idid"), copy);
    QVERIFY(QFileInfo(pinned).isJunction());
    ExtensionCopy::ForgetForTesting();
    QCOMPARE(ExtensionCopy::Make(source, root, QStringLiteral("idid")).path, copy);
    QVERIFY(QFileInfo(pinned).isJunction());
    QVERIFY(QFile::exists(copy + QStringLiteral("/manifest.json")));
    QVERIFY(QFile::exists(copy + QStringLiteral("/lib/types.js")));
#endif
}

void tst_extensioncopy::aJunctionWhoseCopyHasGoneIsPointedAgain(){
#ifndef Q_OS_WIN
    QSKIP("a junction is Windows' own");
#else
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source);
    const QString first = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    const QString pinned = ExtensionCopy::Pin(root, QStringLiteral("idid"), first);
    QVERIFY(QDir(first).removeRecursively());
    QVERIFY(QFileInfo(pinned).isJunction());
    QVERIFY(!QFile::exists(pinned + QStringLiteral("/manifest.json")));
    ExtensionCopy::ForgetForTesting();
    QJsonObject changed = Manifest(); changed[QStringLiteral("version")] = QStringLiteral("2.4.3");
    Write(source + QStringLiteral("/manifest.json"), QJsonDocument(changed).toJson());
    const QString second = ExtensionCopy::Make(source, root, QStringLiteral("idid")).path;
    QCOMPARE(ExtensionCopy::Pin(root, QStringLiteral("idid"), second), pinned);
    QVERIFY(Read(pinned + QStringLiteral("/manifest.json")).contains("2.4.3"));
#endif
}

void tst_extensioncopy::aRegistrationGivenTheJunctionIsKeptAndRegisteredAgain(){
#ifdef Q_OS_WIN
    const QString root = QStringLiteral("C:/copies");
#else
    const QString root = QStringLiteral("/copies");
#endif
    const QString pinned = ExtensionCopy::PinnedPath(root, QStringLiteral("id"));
    QCOMPARE(pinned, root + QStringLiteral("/id/edge"));
    QVERIFY(!ExtensionCopy::Stale(true, true, pinned, pinned));
    QVERIFY(ExtensionCopy::Held(true, pinned, pinned, QStringLiteral("C:/copies/id/8")));
    QVERIFY(ExtensionCopy::Held(true, pinned, pinned, QStringLiteral("C:/ext")));
    QVERIFY(!ExtensionCopy::Held(false, pinned, pinned, QStringLiteral("C:/ext")));
    QVERIFY(!ExtensionCopy::Held(true, pinned, pinned, pinned));
    QVERIFY(!ExtensionCopy::Held(true, QStringLiteral("C:/copies/id/7"), pinned, pinned));
    QVERIFY(!ExtensionCopy::Held(true, QString(), pinned, QStringLiteral("C:/copies/id/8")));
    QVERIFY(ExtensionCopy::Stale(true, true, QStringLiteral("C:/copies/id/7"), pinned));

    QVERIFY(ExtensionCopy::Behind(pinned, pinned, QStringLiteral("C:/copies/id/7"), QStringLiteral("C:/copies/id/8")));
    QVERIFY(ExtensionCopy::Behind(pinned, pinned, QString(), QStringLiteral("C:/copies/id/8")));
    QVERIFY(!ExtensionCopy::Behind(pinned, pinned, QStringLiteral("C:/copies/id/8"), QStringLiteral("C:/copies/id/8")));
    QVERIFY(!ExtensionCopy::Behind(pinned, pinned, QStringLiteral("C:/copies/id/8/"), QStringLiteral("C:/copies/id/8")));
    QVERIFY(!ExtensionCopy::Behind(QStringLiteral("C:/copies/id/7"), pinned, QStringLiteral("C:/copies/id/7"), QStringLiteral("C:/copies/id/8")));
    QVERIFY(!ExtensionCopy::Behind(QString(), pinned, QString(), QStringLiteral("C:/copies/id/8")));
    QVERIFY(!ExtensionCopy::Behind(pinned, pinned, QStringLiteral("C:/copies/id/7"), QString()));
#ifdef Q_OS_WIN
    QVERIFY(!ExtensionCopy::Behind(QStringLiteral("c:/Copies/ID/edge"), pinned, QStringLiteral("c:/copies/id/8"), QStringLiteral("C:/copies/id/8")));
    QVERIFY(ExtensionCopy::Held(true, QStringLiteral("c:/Copies/ID/edge"), pinned, QStringLiteral("C:/copies/id/8")));
#endif

    const QString seven = QStringLiteral("C:/copies/id/7"), eight = QStringLiteral("C:/copies/id/8");
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(true, eight, eight, false), false);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(false, eight, eight, true), true);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(true, eight + QLatin1Char('/'), eight, false), false);
#ifdef Q_OS_WIN
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(true, QStringLiteral("c:/Copies/ID/8"), eight, false), false);
#endif
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(false, seven, eight, true), true);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(true, seven, eight, false), true);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(false, seven, eight, false), false);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(true, QString(), eight, false), true);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(false, QString(), eight, true), true);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(true, eight, QString(), false), true);
    QCOMPARE(ExtensionCopy::KeylessOfLoaded(false, QString(), QString(), false), false);
}

void tst_extensioncopy::aCarrierGoesAheadWhereTheExtensionMayRegisterScripts(){
    foreach(const QString &fine, QStringList() << "<all_urls>" << "*://*/*" << "http://*.a.com/*" << "https://a.com:8080/x/*"
            << "*://a.com/" << "http://*.a.com:*/*" << "https://127.0.0.1/*" << "*://*.a.com./*")
        QVERIFY2(ExtensionCopy::CarriablePattern(fine), qPrintable(fine));
    foreach(const QString &bad, QStringList() << "bogus" << "http://a.com" << "file:///*" << "ftp://a.com/*" << "http://*a.com/*"
            << "http://a.*/*" << "chrome://favicon/*" << "http:///*" << "http://a b/*" << "" << "http://[::1]/*" << "*://*/* ")
        QVERIFY2(!ExtensionCopy::CarriablePattern(bad), qPrintable(bad));

    QJsonObject manifest = Manifest();
    manifest[QStringLiteral("permissions")] = QJsonArray() << QStringLiteral("storage") << QStringLiteral("scripting");
    manifest[QStringLiteral("host_permissions")] = QJsonArray() << QStringLiteral("bogus") << QStringLiteral("<all_urls>") << QStringLiteral("file:///*")
                                                                << QStringLiteral("http://*.a.com/*") << QStringLiteral("<all_urls>")
                                                                << QStringLiteral("https://b.com/x/y") << QStringLiteral("https://b.com/");
    QCOMPARE(ExtensionCopy::HostPatternOf(QStringLiteral("https://b.com/x/y")), QStringLiteral("https://b.com/*"));
    QCOMPARE(ExtensionCopy::HostPatternOf(QStringLiteral("*://*.a.com:8080/")), QStringLiteral("*://*.a.com:8080/*"));
    QCOMPARE(ExtensionCopy::HostPatternOf(QStringLiteral("<all_urls>")), QStringLiteral("<all_urls>"));
    const ExtensionCopy::Rewritten r = ExtensionCopy::Rewrite(manifest, 4711);
    QVERIFY2(r.refusal.isEmpty(), qPrintable(r.refusal));
    QCOMPARE(r.carrier, QStringLiteral("vanilla_carrier.js"));
    const QJsonArray entries = r.manifest[QStringLiteral("content_scripts")].toArray();
    QCOMPARE(entries.size(), 4);
    QJsonObject carrier;
    carrier[QStringLiteral("js")] = QJsonArray() << QStringLiteral("vanilla_carrier.js");
    carrier[QStringLiteral("matches")] = QJsonArray() << QStringLiteral("<all_urls>") << QStringLiteral("http://*.a.com/*") << QStringLiteral("https://b.com/*");
    carrier[QStringLiteral("all_frames")] = true;
    carrier[QStringLiteral("run_at")] = QStringLiteral("document_start");
    QCOMPARE(entries.at(0).toObject(), carrier);
    const QJsonArray plain = ExtensionCopy::Rewrite(Manifest(), 4711).manifest[QStringLiteral("content_scripts")].toArray();
    QCOMPARE(entries.at(1).toObject(), plain.at(0).toObject());
    QCOMPARE(entries.at(3).toObject(), plain.at(2).toObject());
    QCOMPARE(entries.at(1).toObject()[QStringLiteral("js")].toArray().at(0).toString(), QStringLiteral("vanilla_content_shim.js"));

    QJsonObject unscripted = manifest;
    unscripted[QStringLiteral("permissions")] = QJsonArray() << QStringLiteral("storage");
    QVERIFY(ExtensionCopy::Rewrite(unscripted, 4711).carrier.isEmpty());
    QCOMPARE(ExtensionCopy::Rewrite(unscripted, 4711).manifest[QStringLiteral("content_scripts")].toArray().size(), 3);
    QJsonObject nowhere = manifest;
    nowhere[QStringLiteral("host_permissions")] = QJsonArray() << QStringLiteral("bogus") << QStringLiteral("file:///*");
    QVERIFY(ExtensionCopy::Rewrite(nowhere, 4711).carrier.isEmpty());
    nowhere.remove(QStringLiteral("host_permissions"));
    QVERIFY(ExtensionCopy::Rewrite(nowhere, 4711).carrier.isEmpty());
    QJsonObject workerless = manifest;
    workerless.remove(QStringLiteral("background"));
    const ExtensionCopy::Rewritten still = ExtensionCopy::Rewrite(workerless, 4711);
    QVERIFY(still.carrier.isEmpty());
    QCOMPARE(still.contentShim, QStringLiteral("vanilla_content_shim.js"));
    QCOMPARE(still.manifest[QStringLiteral("content_scripts")].toArray().size(), 3);
    QJsonObject bare = manifest;
    bare.remove(QStringLiteral("content_scripts"));
    const ExtensionCopy::Rewritten only = ExtensionCopy::Rewrite(bare, 4711);
    QVERIFY(only.contentShim.isEmpty());
    QCOMPARE(only.carrier, QStringLiteral("vanilla_carrier.js"));
    QCOMPARE(only.manifest[QStringLiteral("content_scripts")].toArray(), QJsonArray() << carrier);

    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("ext")), root = dir.filePath(QStringLiteral("copies"));
    Extension(source, bare);
    const ExtensionCopy::Made made = ExtensionCopy::Make(source, root, QStringLiteral("idid"));
    QVERIFY2(made.note.isEmpty(), qPrintable(made.note));
    QCOMPARE(Read(made.path + QStringLiteral("/vanilla_carrier.js")), QByteArray("self.__vanillaCarrier = 1;\n") + Cdp::ContentShim().toUtf8() + ";\n");
    QVERIFY(!QFile::exists(made.path + QStringLiteral("/vanilla_content_shim.js")));
}

QTEST_MAIN(tst_extensioncopy)
#include "tst_extensioncopy.moc"
