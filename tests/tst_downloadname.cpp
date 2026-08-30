#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QTemporaryDir>

#include "downloadname.hpp"

class tst_downloadname : public QObject {
    Q_OBJECT

private slots:

    void aNameIsOneComponentSpelledWithCharactersAFileSystemTakes_data(){
        QTest::addColumn<QString>("given");
        QTest::addColumn<QString>("expected");

        QTest::newRow("a name is left alone")
            << QStringLiteral("report.pdf") << QStringLiteral("report.pdf");
        QTest::newRow("the characters windows refuses")
            << QStringLiteral("a?b*c<d>e|f") << QStringLiteral("a_b_c_d_e_f");
        QTest::newRow("a colon is one of them")
            << QStringLiteral("C:report.pdf") << QStringLiteral("C_report.pdf");
        QTest::newRow("a quote is another")
            << QStringLiteral("say \"what\".txt") << QStringLiteral("say _what_.txt");
        QTest::newRow("one component, posix separator")
            << QStringLiteral("../../etc/passwd") << QStringLiteral("passwd");
        QTest::newRow("one component, windows separator")
            << QStringLiteral("..\\..\\etc\\passwd") << QStringLiteral("passwd");
        QTest::newRow("a control character is dropped")
            << (QStringLiteral("re") + QChar(0x0a) + QStringLiteral("port.pdf"))
            << QStringLiteral("report.pdf");
        QTest::newRow("surrounding space")
            << QStringLiteral("  report.pdf  ") << QStringLiteral("report.pdf");
        QTest::newRow("a name cannot end in a dot")
            << QStringLiteral("report.pdf...") << QStringLiteral("report.pdf");
        QTest::newRow("nothing usable")
            << QStringLiteral("...") << QString();
        QTest::newRow("nothing at all")
            << QString() << QString();
        QTest::newRow("a leading dot is a name")
            << QStringLiteral(".gitignore") << QStringLiteral(".gitignore");
    }

    void aNameIsOneComponentSpelledWithCharactersAFileSystemTakes(){
        QFETCH(QString, given);
        QFETCH(QString, expected);
        QCOMPARE(DownloadName::Sanitize(given), expected);
    }

    void aLongNameIsCutAndKeepsItsExtension(){
        const QString cut =
            DownloadName::Sanitize(QString(300, QLatin1Char('a')) + QStringLiteral(".pdf"));
        QCOMPARE(cut.length(), 128);
        QVERIFY(cut.endsWith(QStringLiteral(".pdf")));

        const QString bare = DownloadName::Sanitize(QString(300, QLatin1Char('a')));
        QCOMPARE(bare.length(), 128);
        QVERIFY(!bare.endsWith(QLatin1Char('.')));

        const QString wide =
            DownloadName::Sanitize(QString(300, QChar(0x3042)) + QStringLiteral(".pdf"));
        QVERIFY(wide.length() <= 128);
        QVERIFY2(wide.toUtf8().size() <= 200, QByteArray::number(wide.toUtf8().size()));
        QVERIFY(wide.endsWith(QStringLiteral(".pdf")));
    }

    void aCutNeverLandsInsideACharacter(){
        const char32_t outsideTheBasicPlane = 0x1f600;
        QString wide;
        for(int i = 0; i < 200; i++)
            wide += QString::fromUcs4(&outsideTheBasicPlane, 1);

        for(const QString &name : {DownloadName::Sanitize(wide + QStringLiteral(".pdf")),
                                   DownloadName::Sanitize(wide)}){
            QVERIFY(!name.isEmpty());
            for(int i = 0; i < name.length(); i++){
                if(name.at(i).isHighSurrogate()){
                    QVERIFY2(i + 1 < name.length() && name.at(i + 1).isLowSurrogate(),
                             "a high surrogate with nothing after it");
                    i++;
                } else {
                    QVERIFY2(!name.at(i).isLowSurrogate(),
                             "a low surrogate with nothing before it");
                }
            }
            QVERIFY(name.toUtf8().size() <= 200);
            QCOMPARE(QString::fromUtf8(name.toUtf8()), name);
        }

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile f(QDir(dir.path()).filePath(DownloadName::Sanitize(wide + QStringLiteral(".pdf"))));
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.close();
    }

    void theLengthIsAskedAgainAfterTheExtension(){
        QMimeDatabase db;
        if(!db.mimeTypeForName(QStringLiteral("image/jpeg")).isValid())
            QSKIP("no shared mime-info database on this system");

        const QUrl url(QStringLiteral("https://example.com/dir/"));

        const QString wide =
            DownloadName::Suggest(QString(300, QChar(0x3042)),
                                  QStringLiteral("image/jpeg"), url);
        QVERIFY(wide.length() <= 128);
        QVERIFY2(wide.toUtf8().size() <= 200, QByteArray::number(wide.toUtf8().size()));
        QVERIFY(wide.endsWith(QStringLiteral(".jpg")));

        const QString narrow =
            DownloadName::Suggest(QString(300, QLatin1Char('a')),
                                  QStringLiteral("image/jpeg"), url);
        QVERIFY(narrow.length() <= 128);
        QVERIFY(narrow.endsWith(QStringLiteral(".jpg")));
    }

#ifdef Q_OS_WIN
    void aNameADeviceIsCalledByIsMovedOutOfTheWay(){
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("CON")), QStringLiteral("_CON"));
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("con.txt")), QStringLiteral("_con.txt"));
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("COM1")), QStringLiteral("_COM1"));
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("LPT9.log")), QStringLiteral("_LPT9.log"));

        QCOMPARE(DownloadName::Sanitize(QStringLiteral("COM") + QChar(0x00b9)),
                 QStringLiteral("_COM") + QChar(0x00b9));
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("LPT") + QChar(0x00b3)),
                 QStringLiteral("_LPT") + QChar(0x00b3));

        QCOMPARE(DownloadName::Sanitize(QStringLiteral("COM") + QChar(0x0663)),
                 QStringLiteral("COM") + QChar(0x0663));

        QCOMPARE(DownloadName::Sanitize(QStringLiteral("COM0")), QStringLiteral("COM0"));
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("CONTENTS")), QStringLiteral("CONTENTS"));
        QCOMPARE(DownloadName::Sanitize(QStringLiteral("console.js")), QStringLiteral("console.js"));
    }
#endif

    void aUrlNamesTheLastSegmentOfItsPath_data(){
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("expected");

        QTest::newRow("a plain file")
            << QStringLiteral("https://example.com/dir/report.pdf")
            << QStringLiteral("report.pdf");
        QTest::newRow("a query is not part of the name")
            << QStringLiteral("https://example.com/dir/img.php?w=1&h=2")
            << QStringLiteral("img.php");
        QTest::newRow("a fragment is not either")
            << QStringLiteral("https://example.com/dir/report.pdf#page=3")
            << QStringLiteral("report.pdf");
        QTest::newRow("percent encoding is the name the server meant")
            << QStringLiteral("https://example.com/dir/%E3%81%82.txt")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));
        QTest::newRow("an encoded separator does not make a path")
            << QStringLiteral("https://example.com/dir/a%2Fb.txt")
            << QStringLiteral("b.txt");
        QTest::newRow("a directory names nothing")
            << QStringLiteral("https://example.com/dir/") << QString();
        QTest::newRow("a host names nothing")
            << QStringLiteral("https://example.com") << QString();
    }

    void aUrlNamesTheLastSegmentOfItsPath(){
        QFETCH(QString, url);
        QFETCH(QString, expected);
        QCOMPARE(DownloadName::FromUrl(QUrl(url)), expected);
    }

    void theExtensionFollowsTheTypeTheServerDeclared_data(){
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("expected");

        QTest::newRow("no extension")
            << QStringLiteral("photo") << QStringLiteral("image/png")
            << QStringLiteral("photo.png");
        QTest::newRow("an extension which belongs to the type")
            << QStringLiteral("photo.htm") << QStringLiteral("text/html")
            << QStringLiteral("photo.htm");
        QTest::newRow("an extension which does not")
            << QStringLiteral("img.php") << QStringLiteral("image/jpeg")
            << QStringLiteral("img.php.jpg");
        QTest::newRow("the parameters are not part of the type")
            << QStringLiteral("index") << QStringLiteral("text/html; charset=utf-8")
            << QStringLiteral("index.html");
        QTest::newRow("the default type says nothing")
            << QStringLiteral("data.bin") << QStringLiteral("application/octet-stream")
            << QStringLiteral("data.bin");
        QTest::newRow("and neither does a type nobody knows")
            << QStringLiteral("data.bin") << QStringLiteral("application/x-nothing-at-all")
            << QStringLiteral("data.bin");
        QTest::newRow("no type at all")
            << QStringLiteral("photo") << QString() << QStringLiteral("photo");
        QTest::newRow("case is not what tells two extensions apart")
            << QStringLiteral("PHOTO.PNG") << QStringLiteral("image/png")
            << QStringLiteral("PHOTO.PNG");
    }

    void theExtensionFollowsTheTypeTheServerDeclared(){
        QFETCH(QString, name);
        QFETCH(QString, type);
        QFETCH(QString, expected);

        QMimeDatabase db;
        if(!db.mimeTypeForName(QStringLiteral("image/png")).isValid())
            QSKIP("no shared mime-info database on this system");

        QCOMPARE(DownloadName::WithSuffixForMimeType(name, type), expected);
    }

    void theNameAHeaderSuggests_data(){
        QTest::addColumn<QByteArray>("header");
        QTest::addColumn<QString>("expected");

        QTest::newRow("quoted")
            << QByteArrayLiteral("attachment; filename=\"a.txt\"")
            << QStringLiteral("a.txt");
        QTest::newRow("unquoted")
            << QByteArrayLiteral("attachment; filename=a.txt")
            << QStringLiteral("a.txt");
        QTest::newRow("a space before the equals")
            << QByteArrayLiteral("attachment; filename = a.txt")
            << QStringLiteral("a.txt");
        QTest::newRow("the parameter name is not asked with case")
            << QByteArrayLiteral("attachment; FileName=\"a.txt\"")
            << QStringLiteral("a.txt");
        QTest::newRow("a semicolon inside the quotes is part of the name")
            << QByteArrayLiteral("attachment; filename=\"a;b.txt\"")
            << QStringLiteral("a;b.txt");
        QTest::newRow("an escaped quote is part of the name")
            << QByteArrayLiteral("attachment; filename=\"a\\\"b.txt\"")
            << QStringLiteral("a\"b.txt");
        QTest::newRow("single quotes, which nothing defines and this read before")
            << QByteArrayLiteral("attachment; filename='a.txt'")
            << QStringLiteral("a.txt");
        QTest::newRow("the plain form is read as utf-8")
            << QByteArrayLiteral("attachment; filename=\"\xE3\x81\x82.txt\"")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));

        QTest::newRow("the extended form")
            << QByteArrayLiteral("attachment; filename*=UTF-8''%E3%81%82.txt")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));
        QTest::newRow("the extended form names a language")
            << QByteArrayLiteral("attachment; filename*=UTF-8'ja'%E3%81%82.txt")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));
        QTest::newRow("the extended form wins, written second")
            << QByteArrayLiteral("attachment; filename=\"b.txt\"; filename*=UTF-8''%E3%81%82.txt")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));
        QTest::newRow("the extended form wins, written first")
            << QByteArrayLiteral("attachment; filename*=UTF-8''%E3%81%82.txt; filename=\"b.txt\"")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));
        QTest::newRow("iso-8859-1 is the other charset it is defined with")
            << QByteArrayLiteral("attachment; filename*=ISO-8859-1''na%EFve.txt")
            << (QStringLiteral("na") + QChar(0x00ef) + QStringLiteral("ve.txt"));
        QTest::newRow("the charset is not asked with case")
            << QByteArrayLiteral("attachment; filename*=utf-8''%E3%81%82.txt")
            << (QString(QChar(0x3042)) + QStringLiteral(".txt"));

        QTest::newRow("a charset this does not have")
            << QByteArrayLiteral("attachment; filename*=UTF-16''%00a; filename=\"b.txt\"")
            << QStringLiteral("b.txt");
        QTest::newRow("bytes which are not the charset they claim")
            << QByteArrayLiteral("attachment; filename*=UTF-8''%E3%81; filename=\"b.txt\"")
            << QStringLiteral("b.txt");
        QTest::newRow("an escape which is not one")
            << QByteArrayLiteral("attachment; filename*=UTF-8''a%ZZ.txt; filename=\"b.txt\"")
            << QStringLiteral("b.txt");
        QTest::newRow("an escape whose byte is an unfinished character")
            << QByteArrayLiteral("attachment; filename*=UTF-8''a%E3; filename=\"b.txt\"")
            << QStringLiteral("b.txt");
        QTest::newRow("half an escape at the end")
            << QByteArrayLiteral("attachment; filename*=UTF-8''a%E; filename=\"b.txt\"")
            << QStringLiteral("b.txt");
        QTest::newRow("no quotes to say where the charset ends")
            << QByteArrayLiteral("attachment; filename*=%E3%81%82.txt; filename=\"b.txt\"")
            << QStringLiteral("b.txt");
        QTest::newRow("an unreadable extended form and nothing beside it")
            << QByteArrayLiteral("attachment; filename*=UTF-16''%00a")
            << QString();

        QTest::newRow("the first of two, which is a broken header")
            << QByteArrayLiteral("attachment; filename=\"a.txt\"; filename=\"b.txt\"")
            << QStringLiteral("a.txt");
        QTest::newRow("a filename beats the name of the form control")
            << QByteArrayLiteral("form-data; name=\"file\"; filename=\"x.txt\"")
            << QStringLiteral("x.txt");
        QTest::newRow("the form control, when that is all there is")
            << QByteArrayLiteral("form-data; name=\"x\"")
            << QStringLiteral("x");
        QTest::newRow("a disposition and nothing else")
            << QByteArrayLiteral("attachment") << QString();
        QTest::newRow("a parameter with no value")
            << QByteArrayLiteral("attachment; filename") << QString();
        QTest::newRow("nothing at all")
            << QByteArray() << QString();

        QTest::newRow("a path is handed back as it was written")
            << QByteArrayLiteral("attachment; filename*=UTF-8''%2Fetc%2Fpasswd")
            << QStringLiteral("/etc/passwd");
    }

    void theNameAHeaderSuggests(){
        QFETCH(QByteArray, header);
        QFETCH(QString, expected);
        QCOMPARE(DownloadName::FromContentDisposition(header), expected);
    }

    void whatAHeaderSuggestsIsStillOnlyASuggestion(){
        const QString raw = DownloadName::FromContentDisposition
            (QByteArrayLiteral("attachment; filename*=UTF-8''%2Fetc%2Fpasswd"));
        QCOMPARE(DownloadName::Suggest(raw, QString(), QUrl(QStringLiteral("http://x/y"))),
                 QStringLiteral("passwd"));
    }

    void theSuggestionComesFirstThenTheUrlThenAName(){
        const QUrl url(QStringLiteral("https://example.com/dir/img.php?w=1"));
        const QUrl directory(QStringLiteral("https://example.com/dir/"));

        QCOMPARE(DownloadName::Suggest(QStringLiteral("photo?.jpg"),
                                       QStringLiteral("image/jpeg"), url),
                 QStringLiteral("photo_.jpg"));

        QCOMPARE(DownloadName::Suggest(QString(), QString(), url),
                 QStringLiteral("img.php"));

        QCOMPARE(DownloadName::Suggest(QString(), QString(), directory),
                 QStringLiteral("download"));

        QMimeDatabase db;
        if(!db.mimeTypeForName(QStringLiteral("image/jpeg")).isValid())
            QSKIP("no shared mime-info database on this system");

        QCOMPARE(DownloadName::Suggest(QString(), QStringLiteral("image/jpeg"), url),
                 QStringLiteral("img.php.jpg"));

        QCOMPARE(DownloadName::Suggest(QStringLiteral("???"),
                                       QStringLiteral("image/jpeg"), directory),
                 QStringLiteral("___.jpg"));

        QCOMPARE(DownloadName::Suggest(QStringLiteral("   "),
                                       QStringLiteral("image/jpeg"), directory),
                 QStringLiteral("download.jpg"));
    }

    void aNameWhichIsTakenGetsANumber(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QDir at(dir.path());
        auto make = [&at](const QString &name){
            QFile f(at.filePath(name));
            const bool ok = f.open(QIODevice::WriteOnly);
            f.close();
            return ok;
        };

        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral("report.pdf"))),
                 at.filePath(QStringLiteral("report.pdf")));

        QVERIFY(make(QStringLiteral("report.pdf")));
        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral("report.pdf"))),
                 at.filePath(QStringLiteral("report_1.pdf")));

        QVERIFY(make(QStringLiteral("report_1.pdf")));
        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral("report.pdf"))),
                 at.filePath(QStringLiteral("report_2.pdf")));

        QVERIFY(make(QStringLiteral("notes")));
        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral("notes"))),
                 at.filePath(QStringLiteral("notes_1")));

        QVERIFY(make(QStringLiteral("archive.tar.gz")));
        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral("archive.tar.gz"))),
                 at.filePath(QStringLiteral("archive_1.tar.gz")));

        QVERIFY(make(QStringLiteral(".gitignore")));
        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral(".gitignore"))),
                 at.filePath(QStringLiteral(".gitignore_1")));

        QVERIFY(at.mkdir(QStringLiteral("stuff")));
        QCOMPARE(DownloadName::Unique(at.filePath(QStringLiteral("stuff"))),
                 at.filePath(QStringLiteral("stuff_1")));

        QCOMPARE(DownloadName::Unique(QString()), QString());
    }

    void aNameWhichCannotBeFreedIsNotAnswered(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QDir at(dir.path());
        for(const QString &name : {QStringLiteral("report.pdf"),
                                   QStringLiteral("report_1.pdf"),
                                   QStringLiteral("report_2.pdf")}){
            QFile f(at.filePath(name));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("kept");
            f.close();
        }

        const QString path = at.filePath(QStringLiteral("report.pdf"));
        QCOMPARE(DownloadName::Unique(path, 2), QString());

        QCOMPARE(DownloadName::Unique(path, 3),
                 at.filePath(QStringLiteral("report_3.pdf")));

        QFile kept(path);
        QVERIFY(kept.open(QIODevice::ReadOnly));
        QCOMPARE(kept.readAll(), QByteArray("kept"));
    }

    void theShapeOfThePathIsKept(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString native =
            QDir::toNativeSeparators(QDir(dir.path()).filePath(QStringLiteral("report.pdf")));
        QFile f(native);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();

        const QString next = DownloadName::Unique(native);
        QCOMPARE(next, QDir::toNativeSeparators
                 (QDir(dir.path()).filePath(QStringLiteral("report_1.pdf"))));
    }

    void theNameFromAUrlCanBeOpenedForWriting(){
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QUrl url(QStringLiteral("https://example.com/dir/img.php?w=1&h=2"));

        const QString raw = url.toString().section(QLatin1Char('/'), -1);
        QCOMPARE(raw, QStringLiteral("img.php?w=1&h=2"));

        const QString name =
            DownloadName::Suggest(QString(), QStringLiteral("image/jpeg"), url);
        QVERIFY(!name.contains(QLatin1Char('?')));

        QFile good(QDir(dir.path()).filePath(name));
        QVERIFY2(good.open(QIODevice::WriteOnly), qPrintable(good.errorString()));
        good.close();

#ifdef Q_OS_WIN
        QFile bad(QDir(dir.path()).filePath(raw));
        QVERIFY(!bad.open(QIODevice::WriteOnly));
#endif
    }

};

QTEST_MAIN(tst_downloadname)
#include "tst_downloadname.moc"
