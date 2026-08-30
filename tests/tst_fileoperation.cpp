#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

#include "fileoperation.hpp"

class tst_fileoperation : public QObject {
    Q_OBJECT

private slots:
    void aDirectoryIsRemovedWithEverythingUnderIt();
    void removeReportsTheMissingRatherThanPretending();
    void theReachIsCountedBeforeAnythingIsDeleted();
    void aCopiedDirectoryArrivesWithItsContents();
    void aDirectoryIsNotCopiedIntoItself();
    void aReadOnlyFileIsStillDeleted();
    void whatCouldNotBeTakenIsNamed();
    void aDirectoryHeldOpenByItsContentsIsNotNamedTwice();
    void aRefusedCopyNamesItsSource();
    void aCaseOnlyDifferenceIsStillTheSameDirectory();
    void aJunctionIsOneEntryAndNotAWayIn();
    void aShortcutIsTheOrdinaryFileItIs();
    void aSymbolicLinkToADirectoryIsRemovedAsItself();
    void aFreeNameIsLeftAlone();
    void aTakenNameIsNumberedAfterTheBaseName();
    void aDoubleExtensionIsNotSplitInTheMiddle();
    void aNameThatIsAllExtensionIsNumberedAtTheEnd();

private:
    static bool MakeLink(const QString &option,
                         const QString &link, const QString &target){
        QProcess process;
        process.start(QStringLiteral("cmd"), QStringList()
                      << QStringLiteral("/c") << QStringLiteral("mklink") << option
                      << QDir::toNativeSeparators(link)
                      << QDir::toNativeSeparators(target));
        if(!process.waitForFinished(10000)) return false;
        return process.exitCode() == 0;
    }

    static bool MakeDirectorySymbolicLink(const QString &link, const QString &target){
#if defined(Q_OS_WIN)
        return MakeLink(QStringLiteral("/D"), link, target);
#else
        return QFile::link(target, link);
#endif
    }

    static bool IsListed(const QString &dir, const QString &name){
        return QDir(dir).entryList(QDir::AllEntries | QDir::Hidden |
                                   QDir::System | QDir::NoDotAndDotDot)
            .contains(name);
    }

    static QString Put(const QString &root, const QString &relative,
                       const QByteArray &content = QByteArray("x")){
        const QString path = root + QStringLiteral("/") + relative;
        if(!QDir().mkpath(QFileInfo(path).absolutePath())) return QString();
        QFile file(path);
        if(!file.open(QIODevice::WriteOnly)) return QString();
        file.write(content);
        file.close();
        return path;
    }
};

void tst_fileoperation::aDirectoryIsRemovedWithEverythingUnderIt(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral("tree/deep/inner.txt"));
    Put(tmp.path(), QStringLiteral("tree/top.txt"));
    const QString keep = Put(tmp.path(), QStringLiteral("beside.txt"));

    QVERIFY(FileOperation::Remove(tmp.path() + QStringLiteral("/tree")));

    QVERIFY(!QDir(tmp.path() + QStringLiteral("/tree")).exists());
    QVERIFY(QFile::exists(keep));
}

void tst_fileoperation::removeReportsTheMissingRatherThanPretending(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    QVERIFY(!FileOperation::Remove(tmp.path() + QStringLiteral("/not-there")));
    QVERIFY(!FileOperation::Remove(QString()));
}

void tst_fileoperation::theReachIsCountedBeforeAnythingIsDeleted(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral("tree/deep/inner.txt"));
    Put(tmp.path(), QStringLiteral("tree/top.txt"));
    const QString loose = Put(tmp.path(), QStringLiteral("loose.txt"));

    const FileOperation::Reach reach = FileOperation::ReachOf
        (QStringList() << tmp.path() + QStringLiteral("/tree") << loose);

    QCOMPARE(reach.directories, 2);
    QCOMPARE(reach.files, 3);

    QVERIFY(QFile::exists(loose));
    QVERIFY(QDir(tmp.path() + QStringLiteral("/tree")).exists());
}

void tst_fileoperation::aCopiedDirectoryArrivesWithItsContents(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral("from/deep/inner.txt"), QByteArray("inner"));
    Put(tmp.path(), QStringLiteral("from/top.txt"), QByteArray("top"));

    QVERIFY(FileOperation::Copy(tmp.path() + QStringLiteral("/from"),
                                tmp.path() + QStringLiteral("/to")));

    QFile inner(tmp.path() + QStringLiteral("/to/deep/inner.txt"));
    QVERIFY(inner.open(QIODevice::ReadOnly));
    QCOMPARE(inner.readAll(), QByteArray("inner"));
    QVERIFY(QFile::exists(tmp.path() + QStringLiteral("/to/top.txt")));

    QVERIFY(QFile::exists(tmp.path() + QStringLiteral("/from/top.txt")));
}

void tst_fileoperation::aDirectoryIsNotCopiedIntoItself(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral("from/top.txt"));

    QVERIFY(!FileOperation::Copy(tmp.path() + QStringLiteral("/from"),
                                 tmp.path() + QStringLiteral("/from/inside")));
    QVERIFY(!FileOperation::Copy(tmp.path() + QStringLiteral("/from"),
                                 tmp.path() + QStringLiteral("/from")));
    QVERIFY(!QDir(tmp.path() + QStringLiteral("/from/inside")).exists());
}

void tst_fileoperation::aReadOnlyFileIsStillDeleted(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString locked = Put(tmp.path(), QStringLiteral("tree/locked.txt"));
    QVERIFY(!locked.isEmpty());
    QVERIFY(QFile::setPermissions(locked, QFile::ReadOwner | QFile::ReadUser));

    QVERIFY(FileOperation::Remove(tmp.path() + QStringLiteral("/tree")));
    QVERIFY(!QDir(tmp.path() + QStringLiteral("/tree")).exists());
}

void tst_fileoperation::whatCouldNotBeTakenIsNamed(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString missing = tmp.path() + QStringLiteral("/not-there");
    QStringList failed;
    QVERIFY(!FileOperation::Remove(missing, &failed));
    QCOMPARE(failed, QStringList() << missing);

    const QString real = Put(tmp.path(), QStringLiteral("real.txt"));
    QVERIFY(!real.isEmpty());
    failed.clear();
    QVERIFY(FileOperation::Remove(real, &failed));
    QVERIFY(failed.isEmpty());
}

void tst_fileoperation::aDirectoryHeldOpenByItsContentsIsNotNamedTwice(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString locked = Put(tmp.path(), QStringLiteral("tree/locked.txt"));
    QVERIFY(!locked.isEmpty());
    QVERIFY(!Put(tmp.path(), QStringLiteral("tree/loose.txt")).isEmpty());

    QFile held(locked);
    QVERIFY(held.open(QIODevice::ReadOnly));
    if(FileOperation::Remove(locked)){
        held.close();
        QSKIP("a file open for reading can still be deleted here");
    }

    QStringList failed;
    QVERIFY(!FileOperation::Remove(tmp.path() + QStringLiteral("/tree"), &failed));
    held.close();

    QCOMPARE(failed, QStringList() << locked);
    QVERIFY(!QFile::exists(tmp.path() + QStringLiteral("/tree/loose.txt")));
}

void tst_fileoperation::aRefusedCopyNamesItsSource(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    QVERIFY(!Put(tmp.path(), QStringLiteral("from/top.txt")).isEmpty());
    const QString from = tmp.path() + QStringLiteral("/from");

    QStringList failed;
    QVERIFY(!FileOperation::Copy(from, from + QStringLiteral("/inside"), &failed));
    QCOMPARE(failed, QStringList() << from);

    failed.clear();
    QVERIFY(!FileOperation::Copy(tmp.path() + QStringLiteral("/not-there"),
                                 tmp.path() + QStringLiteral("/anywhere"), &failed));
    QCOMPARE(failed, QStringList() << tmp.path() + QStringLiteral("/not-there"));
}

void tst_fileoperation::aCaseOnlyDifferenceIsStillTheSameDirectory(){
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    QSKIP("here the case is part of the name, so these are two directories");
#else
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    QVERIFY(!Put(tmp.path(), QStringLiteral("from/top.txt")).isEmpty());

    const QString shouted = tmp.path() + QStringLiteral("/FROM/inside");
    QVERIFY(!FileOperation::Copy(tmp.path() + QStringLiteral("/from"), shouted));
    QVERIFY(!QDir(shouted).exists());

    QVERIFY(FileOperation::Copy(tmp.path() + QStringLiteral("/from"),
                                tmp.path() + QStringLiteral("/fromage")));
    QVERIFY(QFile::exists(tmp.path() + QStringLiteral("/fromage/top.txt")));
#endif
}

void tst_fileoperation::aJunctionIsOneEntryAndNotAWayIn(){
#if !defined(Q_OS_WIN)
    QSKIP("junctions are an NTFS thing");
#else
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString kept = Put(tmp.path(), QStringLiteral("outside/kept.txt"));
    QVERIFY(!kept.isEmpty());
    QVERIFY(!Put(tmp.path(), QStringLiteral("tree/own.txt")).isEmpty());

    if(!MakeLink(QStringLiteral("/J"),
                 tmp.path() + QStringLiteral("/tree/link"),
                 tmp.path() + QStringLiteral("/outside")))
        QSKIP("this file system would not take a junction");

    const QFileInfo junction(tmp.path() + QStringLiteral("/tree/link"));
    QVERIFY(junction.isJunction());
    qInfo("junction: isDir=%d isSymLink=%d isSymbolicLink=%d isShortcut=%d",
          junction.isDir(), junction.isSymLink(),
          junction.isSymbolicLink(), junction.isShortcut());

    const FileOperation::Reach reach = FileOperation::ReachOf
        (QStringList() << tmp.path() + QStringLiteral("/tree"));
    QCOMPARE(reach.directories, 1);
    QCOMPARE(reach.files, 2);

    FileOperation::Copy(tmp.path() + QStringLiteral("/tree"),
                        tmp.path() + QStringLiteral("/copy"));
    QVERIFY(QFile::exists(tmp.path() + QStringLiteral("/copy/own.txt")));
    QVERIFY(!QFile::exists(tmp.path() + QStringLiteral("/copy/link/kept.txt")));

    QVERIFY(FileOperation::Remove(tmp.path() + QStringLiteral("/tree")));
    QVERIFY(!QDir(tmp.path() + QStringLiteral("/tree")).exists());
    QVERIFY(QFile::exists(kept));
#endif
}

void tst_fileoperation::aShortcutIsTheOrdinaryFileItIs(){
#if !defined(Q_OS_WIN)
    QSKIP("'.lnk' shortcuts are a Windows thing");
#else
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString target = Put(tmp.path(), QStringLiteral("target.txt"));
    QVERIFY(!target.isEmpty());

    const QString shortcut = tmp.path() + QStringLiteral("/point.lnk");
    QVERIFY(QFile::link(target, shortcut));

    const QFileInfo info(shortcut);
    QVERIFY(info.isSymLink());
    QVERIFY(info.isShortcut());
    QVERIFY(!info.isSymbolicLink());

    const QString copy = tmp.path() + QStringLiteral("/other.lnk");
    QVERIFY(FileOperation::Copy(shortcut, copy));
    QVERIFY(IsListed(tmp.path(), QStringLiteral("other.lnk")));
    QVERIFY(QFileInfo(copy).isShortcut());
    QCOMPARE(QFileInfo(copy).symLinkTarget(), QFileInfo(shortcut).symLinkTarget());

    QVERIFY(FileOperation::Remove(shortcut));
    QVERIFY(!IsListed(tmp.path(), QStringLiteral("point.lnk")));
    QVERIFY(QFile::exists(target));
#endif
}

void tst_fileoperation::aSymbolicLinkToADirectoryIsRemovedAsItself(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString kept = Put(tmp.path(), QStringLiteral("outside/kept.txt"));
    QVERIFY(!kept.isEmpty());

    const QString link = tmp.path() + QStringLiteral("/link");
    if(!MakeDirectorySymbolicLink(link, tmp.path() + QStringLiteral("/outside")))
        QSKIP("a symbolic link cannot be made here "
              "(on Windows it needs Developer Mode or an administrator)");

    const QFileInfo info(link);
    QVERIFY(info.isSymbolicLink());
    QVERIFY(!info.isJunction());
    QVERIFY(info.isDir());

    QVERIFY(!FileOperation::Copy(link, tmp.path() + QStringLiteral("/copy")));
    QVERIFY(!QDir(tmp.path() + QStringLiteral("/copy")).exists());

    QVERIFY(FileOperation::Remove(link));
    QVERIFY(!IsListed(tmp.path(), QStringLiteral("link")));
    QVERIFY(QFile::exists(kept));
    QVERIFY(QDir(tmp.path() + QStringLiteral("/outside")).exists());
}

void tst_fileoperation::aFreeNameIsLeftAlone(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    QCOMPARE(FileOperation::UniqueName(tmp.path(), QStringLiteral("a.txt")),
             QStringLiteral("a.txt"));
    QCOMPARE(FileOperation::UniqueName(tmp.path(), QString()), QString());
}

void tst_fileoperation::aTakenNameIsNumberedAfterTheBaseName(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral("a.txt"));
    QCOMPARE(FileOperation::UniqueName(tmp.path(), QStringLiteral("a.txt")),
             QStringLiteral("a (2).txt"));

    Put(tmp.path(), QStringLiteral("a (2).txt"));
    QCOMPARE(FileOperation::UniqueName(tmp.path(), QStringLiteral("a.txt")),
             QStringLiteral("a (3).txt"));

    QDir(tmp.path()).mkdir(QStringLiteral("folder"));
    QCOMPARE(FileOperation::UniqueName(tmp.path(), QStringLiteral("folder")),
             QStringLiteral("folder (2)"));
}

void tst_fileoperation::aDoubleExtensionIsNotSplitInTheMiddle(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral("a.tar.gz"));
    QCOMPARE(FileOperation::UniqueName(tmp.path(), QStringLiteral("a.tar.gz")),
             QStringLiteral("a (2).tar.gz"));
}

void tst_fileoperation::aNameThatIsAllExtensionIsNumberedAtTheEnd(){
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    Put(tmp.path(), QStringLiteral(".gitignore"));
    QCOMPARE(FileOperation::UniqueName(tmp.path(), QStringLiteral(".gitignore")),
             QStringLiteral(".gitignore (2)"));
}

QTEST_MAIN(tst_fileoperation)
#include "tst_fileoperation.moc"
