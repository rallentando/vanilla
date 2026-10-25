#include "switch.hpp"

#include <QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>

#include "const.hpp"
#include "downloadhistory.hpp"
#include "application.hpp"
#include "downloadbar.hpp"

#include <QToolButton>
#include <QDir>
#include <QFile>

class tst_downloadhistory : public QObject {
    Q_OBJECT

    QTemporaryDir m_Dir;

    QString FilePath() const { return m_Dir.filePath(QStringLiteral("downloads.json"));}

    void Fresh(){ QFile::remove(FilePath());}

    static QUrl Url(int n){ return QUrl(QStringLiteral("https://example.com/%1.zip").arg(n));}
    static QString Path(int n){ return QStringLiteral("C:/Downloads/%1.zip").arg(n);}

private slots:

    void anEntryStartsRunningAndEndsOnce(){
        Fresh();
        DownloadHistory history(FilePath());
        const int id = history.Started(Url(1), Path(1), nullptr);
        QVERIFY(id > 0);
        QCOMPARE(history.Find(id)->state, DownloadHistory::Running);

        history.Progress(id, 50, 100);
        QCOMPARE(history.Find(id)->received, qint64(50));

        history.Ended(id, DownloadHistory::Completed);
        QCOMPARE(history.Find(id)->state, DownloadHistory::Completed);
        QCOMPARE(history.Find(id)->received, qint64(100));

        history.Ended(id, DownloadHistory::Failed);
        QCOMPARE(history.Find(id)->state, DownloadHistory::Completed);
        history.Progress(id, 1, 2);
        QCOMPARE(history.Find(id)->received, qint64(100));
    }

    void anEmptyPathIsNotRecorded(){
        Fresh();
        DownloadHistory history(FilePath());
        QCOMPARE(history.Started(Url(1), QString(), nullptr), 0);
        QVERIFY(history.Entries().isEmpty());
    }

    void theFileHoldsWhatEndedAndWhatWasRunningComesBackInterrupted(){
        Fresh();
        int done, running, last;
        {
            DownloadHistory history(FilePath());
            done = history.Started(Url(1), Path(1), nullptr);
            history.Ended(done, DownloadHistory::Completed);
            running = history.Started(Url(2), Path(2), nullptr);
            history.SetClosing();
            history.Ended(running, DownloadHistory::Failed);
            last = running;
        }
        DownloadHistory again(FilePath());
        QCOMPARE(again.Entries().size(), 2);
        QCOMPARE(again.Find(done)->state, DownloadHistory::Completed);
        QCOMPARE(again.Find(running)->state, DownloadHistory::Interrupted);
        QCOMPARE(again.Find(done)->url, Url(1));
        QCOMPARE(again.Find(done)->path, Path(1));

        QVERIFY(again.Started(Url(3), Path(3), nullptr) > last);
    }

    void theSourcePageIsKeptAndComesBack(){
        Fresh();
        const QUrl page(QStringLiteral("https://example.com/files.html"));
        int with, without;
        {
            DownloadHistory history(FilePath());
            with = history.Started(Url(1), Path(1), nullptr, page);
            without = history.Started(Url(2), Path(2), nullptr);
            QCOMPARE(history.Find(with)->page, page);
            QVERIFY(history.Find(without)->page.isEmpty());
        }
        QFile file(FilePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonArray entries =
            QJsonDocument::fromJson(file.readAll()).object()[QStringLiteral("entries")].toArray();
        QCOMPARE(entries.size(), 2);
        for(const QJsonValue &value : entries){
            const QJsonObject obj = value.toObject();
            QCOMPARE(obj.contains(QStringLiteral("page")), obj[QStringLiteral("id")].toInt() == with);
        }
        file.close();

        DownloadHistory again(FilePath());
        QCOMPARE(again.Find(with)->page, page);
        QVERIFY(again.Find(without)->page.isEmpty());
    }

    void onlyAPageATabCanGoBackToIsKept(){
        const QUrl url(QStringLiteral("https://example.com/a.zip"));
        for(const char *kept : {"https://example.com/", "http://example.com/x",
                                "ftp://example.com/", "file:///C:/Users/a/index.html"})
            QCOMPARE(DownloadHistory::SourcePageWorthKeeping(QUrl(QString::fromLatin1(kept)), url),
                     QUrl(QString::fromLatin1(kept)));
        for(const char *refused : {"", "about:blank", "data:text/html,a", "vanilla://settings/",
                                   "blob:https://example.com/1", "https://example.com/a.zip"})
            QVERIFY2(DownloadHistory::SourcePageWorthKeeping(QUrl(QString::fromLatin1(refused)), url).isEmpty(),
                     refused);
    }

    void newestFirst(){
        Fresh();
        DownloadHistory history(FilePath());
        const int a = history.Started(Url(1), Path(1), nullptr);
        const int b = history.Started(Url(2), Path(2), nullptr);
        QCOMPARE(history.Entries().first().id, b);
        QCOMPARE(history.Entries().last().id, a);
    }

    void removingAndClearingLeaveWhatIsRunning(){
        Fresh();
        DownloadHistory history(FilePath());
        const int a = history.Started(Url(1), Path(1), nullptr);
        const int b = history.Started(Url(2), Path(2), nullptr);
        const int c = history.Started(Url(3), Path(3), nullptr);
        history.Ended(a, DownloadHistory::Completed);
        history.Ended(c, DownloadHistory::Cancelled);

        QVERIFY(!history.Remove(b));
        QVERIFY(history.Remove(a));
        QVERIFY(!history.Find(a));
        QVERIFY(!history.Remove(12345));

        history.Clear();
        QCOMPARE(history.Entries().size(), 1);
        QCOMPARE(history.Entries().first().id, b);

        DownloadHistory again(FilePath());
        QCOMPARE(again.Entries().size(), 1);
    }

    void cancellingWithoutAnItemDoesNothing(){
        Fresh();
        DownloadHistory history(FilePath());
        const int a = history.Started(Url(1), Path(1), nullptr);
        QVERIFY(!history.Cancel(a));
        QCOMPARE(history.Find(a)->state, DownloadHistory::Running);
    }

    void theNewestFiveHundredAndNeverARunningOne(){
        Fresh();
        DownloadHistory history(FilePath());
        const int first = history.Started(Url(0), Path(0), nullptr);
        for(int i = 1; i <= DownloadHistory::Limit + 10; i++){
            const int id = history.Started(Url(i), Path(i), nullptr);
            history.Ended(id, DownloadHistory::Completed);
        }
        QCOMPARE(history.Entries().size(), DownloadHistory::Limit);
        QVERIFY(history.Find(first));
        QCOMPARE(history.Entries().first().path, Path(DownloadHistory::Limit + 10));
    }

    void aBrokenFileIsAnEmptyList(){
        Fresh();
        QFile file(FilePath());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{ this is not json");
        file.close();
        DownloadHistory history(FilePath());
        QVERIFY(history.Entries().isEmpty());
        QCOMPARE(history.Started(Url(1), Path(1), nullptr), 1);
    }

    void programsAreNotOpenedFromThePage(){
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/setup.exe")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/Setup.EXE")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/run.bat")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.ps1")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.msi")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.lnk")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("/home/u/x.sh")));
        QVERIFY(!DownloadHistory::IsRunnable(QStringLiteral("C:/d/report.pdf")));
        QVERIFY(!DownloadHistory::IsRunnable(QStringLiteral("C:/d/photo.jpg")));
        QVERIFY(!DownloadHistory::IsRunnable(QStringLiteral("C:/d/archive.zip")));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.appref-ms"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.chm"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.diagcab"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.search-ms"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.searchConnector-ms"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.hlp"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.jnlp"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.diagpkg"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.xbap"), false));
        QVERIFY(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.ps1xml"), false));
        for(const char *type : {"diagcfg", "ps2", "ps2xml", "psc1", "psc2", "psd1", "msh", "msh1", "msh2",
                                   "mshxml", "msh1xml", "msh2xml", "shb", "shs", "ins", "isp", "mde", "ade",
                                   "adp", "mst", "msu"})
            QVERIFY2(DownloadHistory::IsRunnable(QStringLiteral("C:/d/x.") + QLatin1String(type), false), type);
    }

    void theDefaultBrowserIsGivenWhatABrowserOpens(){
        auto may = [](const char *url){
            return Application::MayOpenWithDefaultBrowser(QUrl(QString::fromLatin1(url)));
        };
        QVERIFY(may("https://example.com/a?b=c"));
        QVERIFY(may("HTTP://example.com/"));
        QVERIFY(may("ftp://example.com/file.txt"));
        QVERIFY(may("file:///C:/d/page.html"));
        QVERIFY(may("file:///C:/d/"));

        QVERIFY(!may("file:///C:/d/setup.exe"));
        QVERIFY(!may("file:///C:/d/x.lnk"));
        QVERIFY(!may("file://evil.example/share/update.exe"));
        QVERIFY(!may("file://evil.example/share/page.html"));
        QVERIFY(!may("file:////evil.example/share/page.html"));
        QVERIFY(!may("file:///%5C%5Cevil.example%5Cshare%5Cpage.html"));
        QVERIFY(!may("file:///C:/d/run.cmd%00.txt"));
#if defined(Q_OS_WIN)
        QVERIFY(!may("file:///C:/d/run.cmd."));
        QVERIFY(!may("file:///C:/d/run.cmd%20"));
        QVERIFY(!may("file:///C:/d/run.cmd:x.txt"));
        QVERIFY(!may("file:///C:/d./page.html"));
        QVERIFY(may("file:///C:/d/page.html"));
#else
        QVERIFY(may("file:///home/u/report:final.pdf"));
        QVERIFY(may("file:///home/u/report."));
#endif
        QVERIFY(!may("javascript:alert(1)"));
        QVERIFY(!may("data:text/html,x"));
        QVERIFY(!may("ms-msdt:/id PCWDiagnostic"));
        QVERIFY(!may("search-ms:query=x&crumb=location:\\\\evil.example\\share"));
        QVERIFY(!may("mailto:a@example.com"));
        QVERIFY(!may("vanilla://settings/"));
        QVERIFY(!may(""));
        QVERIFY(!Application::OpenUrlWithDefaultBrowser(QUrl(QStringLiteral("ms-msdt:/id x"))));
    }

    void theListSaysWhenItChanges(){
        Fresh();
        DownloadHistory history(FilePath());
        QSignalSpy started(&history, &DownloadHistory::EntryStarted);
        QSignalSpy changed(&history, &DownloadHistory::Changed);
        QSignalSpy list(&history, &DownloadHistory::ListChanged);
        const int id = history.Started(Url(1), Path(1), nullptr);
        QCOMPARE(started.count(), 1);
        QCOMPARE(started.first().first().toInt(), id);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(list.count(), 1);
        history.Progress(id, 10, 100);
        QCOMPARE(changed.count(), 2);
        QCOMPARE(list.count(), 1);
        history.Ended(id, DownloadHistory::Completed);
        QCOMPARE(changed.count(), 3);
        QCOMPARE(list.count(), 2);
        history.Ended(id, DownloadHistory::Failed);
        QCOMPARE(changed.count(), 3);
        QCOMPARE(list.count(), 2);
        QVERIFY(history.Remove(id));
        QCOMPARE(changed.count(), 4);
        QCOMPARE(list.count(), 3);
        history.Started(Url(2), Path(2), nullptr);
        const int second = history.Entries().first().id;
        history.Ended(second, DownloadHistory::Cancelled);
        history.Clear();
        QCOMPARE(changed.count(), 7);
        QCOMPARE(list.count(), 6);
        QCOMPARE(started.count(), 2);
        history.SetClosing();
        QCOMPARE(history.Started(Url(3), Path(3), nullptr), 0);
        QCOMPARE(started.count(), 2);
        QCOMPARE(changed.count(), 7);
        QCOMPARE(list.count(), 6);
    }

    void openIsOfferedForAFinishedFileWhichIsNoProgram(){
        QTemporaryDir dir;
        auto made = [&](const char *name){
            const QString path = dir.filePath(QString::fromLatin1(name));
            QFile file(path);
            file.open(QIODevice::WriteOnly);
            file.write("x");
            return path;
        };
        DownloadHistory::Entry entry;
        entry.state = DownloadHistory::Completed;
        entry.path = made("report.pdf");
        QVERIFY(DownloadHistory::CanOpen(entry));
        entry.state = DownloadHistory::Running;
        QVERIFY(!DownloadHistory::CanOpen(entry));
        entry.state = DownloadHistory::Completed;
        entry.path = made("setup.exe");
        QVERIFY(!DownloadHistory::CanOpen(entry));
        entry.path = dir.filePath(QStringLiteral("gone.pdf"));
        QVERIFY(!DownloadHistory::CanOpen(entry));
    }

    void thePanelSaysHowEachDownloadStands(){
        QLocale::setDefault(QLocale::c());
        QCOMPARE(DownloadPanel::Size(16), QStringLiteral("16 B"));
        QCOMPARE(DownloadPanel::Size(2048), QStringLiteral("2 KB"));
        QCOMPARE(DownloadPanel::Size(1536 * 1024), QStringLiteral("1.5 MB"));
        QCOMPARE(DownloadPanel::Size(qint64(3) * 1024 * 1024 * 1024), QStringLiteral("3.0 GB"));
        QCOMPARE(DownloadPanel::Size(-1), QStringLiteral("0 B"));
        QCOMPARE(DownloadPanel::Size(1023 * 1024), QStringLiteral("1023 KB"));
        QCOMPARE(DownloadPanel::Size(1023 * 1024 + 600), QStringLiteral("1.0 MB"));
        QCOMPARE(DownloadPanel::Size(qint64(1024) * 1024 * 1024 - 1024), QStringLiteral("1.0 GB"));

        DownloadHistory::Entry entry;
        entry.state = DownloadHistory::Completed;
        entry.total = 16;
        entry.received = 16;
        QCOMPARE(DownloadPanel::Describe(entry, true),
                 QStringLiteral("16 B ") + QChar(0x2022) + QStringLiteral(" Completed"));
        QCOMPARE(DownloadPanel::Describe(entry, false), QStringLiteral("Deleted"));
        entry.state = DownloadHistory::Running;
        entry.total = 2 * 1024 * 1024;
        entry.received = 1024 * 1024;
        QCOMPARE(DownloadPanel::Describe(entry, true), QStringLiteral("1.0 MB of 2.0 MB"));
        entry.total = -1;
        QCOMPARE(DownloadPanel::Describe(entry, true), QStringLiteral("1.0 MB"));
        entry.state = DownloadHistory::Cancelled;
        QCOMPARE(DownloadPanel::Describe(entry, true), QStringLiteral("Cancelled"));
        entry.state = DownloadHistory::Failed;
        QCOMPARE(DownloadPanel::Describe(entry, true), QStringLiteral("Failed"));
        entry.state = DownloadHistory::Interrupted;
        QCOMPARE(DownloadPanel::Describe(entry, true), QStringLiteral("Interrupted"));
        QLocale::setDefault(QLocale::system());
    }

    void theRowsAreNotRemadeUnderAPress(){
        QWidget rows;
        QToolButton *cancel = new QToolButton(&rows);
        QToolButton *other = new QToolButton(&rows);
        rows.show();
        QVERIFY(!DownloadPanel::HoldsAPressedButton(nullptr));
        QVERIFY(!DownloadPanel::HoldsAPressedButton(&rows));
        QTest::mousePress(cancel, Qt::LeftButton);
        QVERIFY(cancel->isDown());
        QVERIFY(DownloadPanel::HoldsAPressedButton(&rows));
        QTest::mouseRelease(cancel, Qt::LeftButton);
        QVERIFY(!DownloadPanel::HoldsAPressedButton(&rows));
        QTest::mousePress(other, Qt::LeftButton);
        QVERIFY(DownloadPanel::HoldsAPressedButton(&rows));
        QTest::mouseRelease(other, Qt::LeftButton);
        QVERIFY(!DownloadPanel::HoldsAPressedButton(&rows));
    }

    void theButtonIsWantedWhileOneRunsAndForAMinuteAfter(){
        const QDateTime now(QDate(2026, 10, 10), QTime(12, 0, 0));
        const qint64 minute = DOWNLOAD_BUTTON_LINGER;
        QList<DownloadHistory::Entry> entries;
        QCOMPARE(DownloadBar::WantedFor(entries, now), qint64(0));

        DownloadHistory::Entry done;
        done.state = DownloadHistory::Completed;
        done.ended = now.addSecs(-20);
        entries << done;
        QCOMPARE(DownloadBar::WantedFor(entries, now), minute - 20 * 1000);

        DownloadHistory::Entry older;
        older.state = DownloadHistory::Cancelled;
        older.ended = now.addSecs(-50);
        entries << older;
        QCOMPARE(DownloadBar::WantedFor(entries, now), minute - 20 * 1000);

        QList<DownloadHistory::Entry> old;
        done.ended = now.addMSecs(-minute);
        old << done;
        QCOMPARE(DownloadBar::WantedFor(old, now), qint64(0));
        DownloadHistory::Entry interrupted;
        interrupted.state = DownloadHistory::Interrupted;
        old << interrupted;
        QCOMPARE(DownloadBar::WantedFor(old, now), qint64(0));

        DownloadHistory::Entry ahead;
        ahead.state = DownloadHistory::Failed;
        ahead.ended = now.addSecs(30);
        QCOMPARE(DownloadBar::WantedFor(QList<DownloadHistory::Entry>() << ahead, now), minute);

        DownloadHistory::Entry running;
        running.state = DownloadHistory::Running;
        old << running;
        QCOMPARE(DownloadBar::WantedFor(old, now), DownloadBar::WantedWhileRunning);

        QCOMPARE(DownloadBar::NewestEnding(entries), now.addSecs(-20));
        entries.prepend(older);
        QCOMPARE(DownloadBar::NewestEnding(entries), now.addSecs(-20));
        QVERIFY(!DownloadBar::NewestEnding(QList<DownloadHistory::Entry>() << interrupted << running).isValid());
    }

    void thePanelNeverTakesTheActivation(){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) + QStringLiteral("/ui/downloadbar.cpp"));
        QVERIFY2(file.open(QIODevice::ReadOnly), "the panel's source was not read; check VANILLA_SOURCE_DIR");
        const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

        const int ctor = source.indexOf(QStringLiteral("DownloadPanel::DownloadPanel(MainWindow *window)"));
        QVERIFY2(ctor >= 0, "the panel's constructor has gone");
        const int body = source.indexOf(QStringLiteral("\n{"), ctor);
        QVERIFY2(body > ctor, "the panel's constructor has lost its body");
        const QString initialisers = source.mid(ctor, body - ctor);
        const int flags = initialisers.indexOf(QStringLiteral("QWidget(window, "));
        QVERIFY2(flags >= 0, "the panel is no longer made as a widget of its window");
        const QString made = initialisers.mid(flags, initialisers.indexOf(QLatin1Char(')'), flags) - flags);
        QVERIFY2(made.contains(QStringLiteral("Qt::Tool")) && made.contains(QStringLiteral("Qt::WindowDoesNotAcceptFocus")),
                 qPrintable(QStringLiteral("the panel's window may take the activation: ") + made));
        QVERIFY2(!source.contains(QStringLiteral("activateWindow(")),
                 "something activates a download window: the main window's blur would go flat");
        QVERIFY2(!source.contains(QStringLiteral("setFocus(")),
                 "something gives the focus to a download window");
        const int shown = source.indexOf(QStringLiteral("void DownloadPanel::ShowFor("));
        QVERIFY2(shown >= 0, "'ShowFor' has gone");
        const QString showFor = source.mid(shown, source.indexOf(QStringLiteral("\n}"), shown) - shown);
        QVERIFY2(showFor.contains(QStringLiteral("setAttribute(Qt::WA_ShowWithoutActivating);")),
                 "the panel is not always shown without activating");
    }
};

QTEST_MAIN(tst_downloadhistory)
#include "tst_downloadhistory.moc"
