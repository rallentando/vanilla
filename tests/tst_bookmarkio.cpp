#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTimeZone>
#include <QUrl>

#include "lightnode.hpp"
#include "bookmarkio.hpp"

#include "testsupport.hpp"

namespace {

const int UTC_OFFSET = 9*60*60;

QDateTime Date(int secs = 0){
    return QDateTime::fromString(QStringLiteral("20260101120000"), NODE_DATETIME_FORMAT).addSecs(secs);
}

qint64 Epoch(int secs = 0){
    return QDateTime(QDate(2026, 1, 1), QTime(12, 0, 0),
                     QTimeZone::fromSecondsAheadOfUtc(UTC_OFFSET)).toSecsSinceEpoch() + secs;
}

void SetDates(ViewNode *nd){
    nd->SetCreateDate(Date(0));
    nd->SetLastUpdateDate(Date(60));
    nd->SetLastAccessDate(Date(120));
}

ViewNode *MakeFolder(ViewNode *parent, const QString &title){
    ViewNode *vn = parent->MakeChild();
    vn->SetTitle(title);
    SetDates(vn);
    return vn;
}

ViewNode *MakeTab(ViewNode *parent, const QString &title, const QString &url){
    ViewNode *vn = parent->MakeChild();
    vn->SetHoldView(true);
    vn->SetTitle(title);
    vn->SetUrl(QUrl(url));
    SetDates(vn);
    return vn;
}

QString Dump(ViewNode *nd, int depth = 0){
    QString text;
    foreach(Node *child, nd->GetChildren()){
        ViewNode *vn = child->ToViewNode();
        text += QString(2*depth, ' ');
        if(vn->HoldsView())
            text += QStringLiteral("- %1 [%2]\n").arg(vn->GetTitle(), vn->GetUrl().toString());
        else
            text += QStringLiteral("+ %1\n").arg(vn->GetTitle());
        text += Dump(vn, depth+1);
    }
    return text;
}

QString ReadAll(const QString &path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    QString text = QString::fromUtf8(file.readAll());
    file.close();
    return text;
}

void WriteAll(const QString &path, const QByteArray &data){
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data);
    file.close();
}

}

class tst_bookmarkio : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_Dir;

    QString Path(const QString &name) const {
        return m_Dir.path() + QStringLiteral("/") + name;
    }

    ViewNode *MakeTree(){
        ViewNode *root = new ViewNode();
        ViewNode *news = MakeFolder(root, QStringLiteral("News & \"Weather\""));
        MakeTab(news, QStringLiteral("Example"), QStringLiteral("https://example.com/"));
        MakeTab(news, QStringLiteral("日本語のタイトル"), QStringLiteral("https://example.jp/?q=%E3%81%82"));
        ViewNode *deep = MakeFolder(news, QStringLiteral("Nested"));
        MakeTab(deep, QStringLiteral("Deep"), QStringLiteral("https://example.org/deep"));
        MakeFolder(root, QStringLiteral("Empty"));
        MakeTab(root, QStringLiteral("Top level"), QStringLiteral("https://example.net/"));
        return root;
    }

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        QVERIFY(m_Dir.isValid());
        Node::SetBooting(true);
    }

    void cleanupTestCase(){
        Node::SetBooting(false);
    }

    void internalXmlSurvivesAWriteAndARead(){
        ViewNode *root = MakeTree();
        const QByteArray first = BookmarkIO::WriteInternalXml(root);

        ViewNode *reread = new ViewNode();
        QVERIFY(BookmarkIO::ReadInternalXml(first, reread));
        QCOMPARE(Dump(reread), Dump(root));
        QCOMPARE(BookmarkIO::WriteInternalXml(reread), first);

        delete reread;
        delete root;
    }

    void xbelSurvivesAWriteAndARead(){
        ViewNode *root = MakeTree();
        const QByteArray first = BookmarkIO::WriteXbel(root, UTC_OFFSET);

        ViewNode *reread = new ViewNode();
        QVERIFY(BookmarkIO::ReadXbel(first, reread));
        QCOMPARE(Dump(reread), Dump(root));
        QCOMPARE(BookmarkIO::WriteXbel(reread, UTC_OFFSET), first);

        delete reread;
        delete root;
    }

    void netscapeHtmlSurvivesAWriteAndARead(){
        ViewNode *root = MakeTree();
        const QString first = BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET);

        ViewNode *reread = new ViewNode();
        QVERIFY(BookmarkIO::ReadNetscapeHtml(first, reread));
        QCOMPARE(Dump(reread), Dump(root));
        QCOMPARE(BookmarkIO::WriteNetscapeHtml(reread, UTC_OFFSET), first);

        delete reread;
        delete root;
    }

    void aTitleWithMarkupSurvivesTheHtmlRoundTrip(){
        ViewNode *root = new ViewNode();
        ViewNode *folder = MakeFolder(root, QStringLiteral("a < b & c"));
        MakeTab(folder, QStringLiteral("<script>\"x\" & 'y'</script>"),
                QStringLiteral("https://example.com/?a=1&b=2"));

        const QString html = BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET);
        QVERIFY(html.contains(QStringLiteral("a &lt; b &amp; c")));
        QVERIFY(!html.contains(QStringLiteral("<script>")));
        QVERIFY(html.contains(QStringLiteral("?a=1&amp;b=2")));

        ViewNode *reread = new ViewNode();
        QVERIFY(BookmarkIO::ReadNetscapeHtml(html, reread));
        QCOMPARE(Dump(reread), Dump(root));

        delete reread;
        delete root;
    }

    void theHtmlReaderUndoesTheEntitiesOtherBrowsersWrite(){
        const QString html =
            "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
            "<DL><p>\n"
            "    <DT><A HREF=\"https://example.com/\">R&amp;D &#38; co &#x26; more &apos;q&apos;</A>\n"
            "</DL><p>\n";

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadNetscapeHtml(html, root));
        QCOMPARE(root->GetChildren().first()->ToViewNode()->GetTitle(),
                 QStringLiteral("R&D & co & more 'q'"));
        delete root;
    }

    void ananchorOutsideAnyItemIsNotACrash(){
        const QString html =
            "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
            "<DL><p>\n"
            "<A HREF=\"https://example.com/\">Loose</A>\n"
            "    <DT><A HREF=\"https://example.org/\">Proper</A>\n"
            "</DL><p>\n";

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadNetscapeHtml(html, root));
        QVERIFY(!root->HoldsView());
        QCOMPARE(Dump(root), QStringLiteral("- Proper [https://example.org/]\n"));
        delete root;
    }

    void theRoundTripKeepsTheDates(){
        ViewNode *root = new ViewNode();
        MakeTab(root, QStringLiteral("Dated"), QStringLiteral("https://example.com/"));

        ViewNode *xbel = new ViewNode();
        QVERIFY(BookmarkIO::ReadXbel(BookmarkIO::WriteXbel(root, UTC_OFFSET), xbel));
        ViewNode *tab = xbel->GetChildren().first()->ToViewNode();
        QCOMPARE(tab->GetCreateDate().toSecsSinceEpoch(),     Epoch(0));
        QCOMPARE(tab->GetLastUpdateDate().toSecsSinceEpoch(), Epoch(60));
        QCOMPARE(tab->GetLastAccessDate().toSecsSinceEpoch(), Epoch(120));

        ViewNode *html = new ViewNode();
        QVERIFY(BookmarkIO::ReadNetscapeHtml(BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET), html));
        tab = html->GetChildren().first()->ToViewNode();
        QCOMPARE(tab->GetCreateDate().toSecsSinceEpoch(),     Epoch(0));
        QCOMPARE(tab->GetLastUpdateDate().toSecsSinceEpoch(), Epoch(60));
        QCOMPARE(tab->GetLastAccessDate().toSecsSinceEpoch(), Epoch(120));

        delete html;
        delete xbel;
        delete root;
    }

    void theInternalFormatKeepsWhatOnlyItHas(){
        ViewNode *root = new ViewNode();
        ViewNode *folder = MakeFolder(root, QStringLiteral("Folder"));
        MakeTab(folder, QStringLiteral("First"), QStringLiteral("https://example.com/"));
        ViewNode *second = MakeTab(folder, QStringLiteral("Second"), QStringLiteral("https://example.org/"));
        second->SetScrollX(120);
        second->SetScrollY(3400);
        second->SetZoom(1.25f);
        folder->SetPrimary(second);

        ViewNode *reread = new ViewNode();
        QVERIFY(BookmarkIO::ReadInternalXml(BookmarkIO::WriteInternalXml(root), reread));
        ViewNode *tab = reread->GetChildren().first()->ToViewNode()->GetChildren().at(1)->ToViewNode();
        QCOMPARE(tab->GetTitle(), QStringLiteral("Second"));
        QCOMPARE(tab->GetScrollX(), 120);
        QCOMPARE(tab->GetScrollY(), 3400);
        QCOMPARE(static_cast<double>(tab->GetZoom()), 1.25);
        QVERIFY(tab->IsPrimaryOfParent());

        delete reread;
        delete root;
    }

    void writesTheInternalFormat(){
        ViewNode *root = new ViewNode();
        ViewNode *folder = MakeFolder(root, QStringLiteral("Folder"));
        MakeTab(folder, QStringLiteral("Tab"), QStringLiteral("https://example.com/"));

        const QString expected =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<viewnode root=\"true\">\n"
            "  <viewnode holdview=\"false\" primary=\"false\" title=\"Folder\">\n"
            "    <viewnode holdview=\"true\" index=\"0\" primary=\"false\" title=\"Tab\""
            " url=\"https://example.com/\" zoom=\"1\"/>\n"
            "  </viewnode>\n"
            "</viewnode>\n";
        QCOMPARE(QString::fromUtf8(BookmarkIO::WriteInternalXml(root)), expected);

        delete root;
    }

    void theWindowIndexComesFromTheHook(){
        ViewNode *root = new ViewNode();
        MakeTab(root, QStringLiteral("Tab"), QStringLiteral("https://example.com/"));

        BookmarkIO::Hooks hooks;
        hooks.WindowIndexOf = [](ViewNode*){ return 7;};

        QVERIFY(QString::fromUtf8(BookmarkIO::WriteInternalXml(root, hooks))
                .contains(QStringLiteral("index=\"7\"")));
        QVERIFY(QString::fromUtf8(BookmarkIO::WriteInternalXml(root))
                .contains(QStringLiteral("index=\"0\"")));

        delete root;
    }

    void writesXbel(){
        ViewNode *root = new ViewNode();
        ViewNode *folder = MakeFolder(root, QStringLiteral("Folder"));
        MakeTab(folder, QStringLiteral("Tab"), QStringLiteral("https://example.com/"));

        const QString expected =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE xbel>\n"
            "<xbel version=\"1.0\">\n"
            "  <folder added=\"2026-01-01T12:00:00+09:00\" folded=\"yes\">\n"
            "    <title>Folder</title>\n"
            "    <bookmark added=\"2026-01-01T12:00:00+09:00\" href=\"https://example.com/\""
            " modified=\"2026-01-01T12:01:00+09:00\" visited=\"2026-01-01T12:02:00+09:00\">\n"
            "      <title>Tab</title>\n"
            "    </bookmark>\n"
            "  </folder>\n"
            "</xbel>\n";
        QCOMPARE(QString::fromUtf8(BookmarkIO::WriteXbel(root, UTC_OFFSET)), expected);

        delete root;
    }

    void writesNetscapeHtml(){
        ViewNode *root = new ViewNode();
        ViewNode *folder = MakeFolder(root, QStringLiteral("Folder"));
        MakeTab(folder, QStringLiteral("Tab"), QStringLiteral("https://example.com/"));

        const QString expected =
            "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
            "<!-- This is an automatically generated file.\n"
            "     It will be read and overwritten.\n"
            "     DO NOT EDIT! -->\n"
            "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
            "<TITLE>Bookmarks</TITLE>\n"
            "<H1>Bookmarks</H1>\n"
            "\n"
            "<DL><p>\n"
            "    <DT><H3 ADD_DATE=\"%1\" LAST_VISIT=\"%2\" LAST_MODIFIED=\"%3\" FOLDED=\"true\">Folder</H3>\n"
            "    <DL><p>\n"
            "        <DT><A HREF=\"https://example.com/\" ADD_DATE=\"%1\" LAST_VISIT=\"%2\""
            " LAST_MODIFIED=\"%3\">Tab</A>\n"
            "    </DL><p>\n"
            "</DL><p>\n";
        QCOMPARE(BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET),
                 expected.arg(Epoch(0)).arg(Epoch(120)).arg(Epoch(60)));

        delete root;
    }

    void aurlWhichNeedsEncodingIsNotMangled(){
        const QString encoded = QStringLiteral("https://example.jp/?q=%E3%81%82");

        ViewNode *root = new ViewNode();
        MakeTab(root, QStringLiteral("Tab"), encoded);

        const QByteArray xbel = BookmarkIO::WriteXbel(root, UTC_OFFSET);
        const QString html = BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET);
        QVERIFY(QString::fromUtf8(xbel).contains(encoded));
        QVERIFY(html.contains(encoded));

        ViewNode *fromXbel = new ViewNode();
        ViewNode *fromHtml = new ViewNode();
        QVERIFY(BookmarkIO::ReadXbel(xbel, fromXbel));
        QVERIFY(BookmarkIO::ReadNetscapeHtml(html, fromHtml));
        QCOMPARE(fromXbel->GetChildren().first()->ToViewNode()->GetUrl().toEncoded(), encoded.toUtf8());
        QCOMPARE(fromHtml->GetChildren().first()->ToViewNode()->GetUrl().toEncoded(), encoded.toUtf8());

        ViewNode *decoded = new ViewNode();
        QVERIFY(BookmarkIO::ReadXbel(QStringLiteral("<xbel><bookmark href=\"https://example.jp/?q=あ\">"
                                                    "<title>Tab</title></bookmark></xbel>").toUtf8(), decoded));
        QCOMPARE(decoded->GetChildren().first()->ToViewNode()->GetUrl().toEncoded(), encoded.toUtf8());

        delete decoded;
        delete fromHtml;
        delete fromXbel;
        delete root;
    }

    void anUntitledNodeIsWrittenAsNoTitle(){
        ViewNode *root = new ViewNode();
        MakeTab(root, QString(), QStringLiteral("https://example.com/"));

        QVERIFY(BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET).contains(QStringLiteral(">NoTitle</A>")));

        delete root;
    }

    void readsAChromeBookmarkFile(){
        const QByteArray json =
            "{\"checksum\":\"0\",\"version\":1,\"roots\":{"
            "  \"bookmark_bar\":{\"name\":\"Bookmarks bar\",\"type\":\"folder\",\"children\":["
            "    {\"name\":\"Example\",\"type\":\"url\",\"url\":\"https://example.com/\"},"
            "    {\"name\":\"Folder\",\"type\":\"folder\",\"children\":["
            "      {\"name\":\"Inner\",\"type\":\"url\",\"url\":\"https://example.org/\"}]}]},"
            "  \"other\":{\"name\":\"Other bookmarks\",\"type\":\"folder\",\"children\":[]}"
            "}}";

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadChromeJson(json, root));

        QCOMPARE(Dump(root),
                 QStringLiteral("+ Favorites\n"
                                "  + Bookmarks bar\n"
                                "    - Example [https://example.com/]\n"
                                "    + Folder\n"
                                "      - Inner [https://example.org/]\n"
                                "  + Other bookmarks\n"));
        delete root;
    }

    void chromesbookkeepingUnderRootsIsNotImportedAsATab(){
        const QByteArray json =
            "{\"roots\":{"
            "  \"bookmark_bar\":{\"name\":\"Bookmarks bar\",\"type\":\"folder\",\"children\":["
            "    {\"name\":\"Example\",\"type\":\"url\",\"url\":\"https://example.com/\"}]},"
            "  \"sync_transaction_version\":\"1\""
            "}}";

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadChromeJson(json, root));
        QCOMPARE(Dump(root),
                 QStringLiteral("+ Favorites\n"
                                "  + Bookmarks bar\n"
                                "    - Example [https://example.com/]\n"));
        delete root;
    }

    void readsAFirefoxBookmarkBackup(){
        const QByteArray json =
            "{\"guid\":\"root________\",\"title\":\"\",\"type\":\"text/x-moz-place-container\",\"children\":["
            "  {\"title\":\"toolbar\",\"type\":\"text/x-moz-place-container\",\"children\":["
            "    {\"title\":\"Example\",\"type\":\"text/x-moz-place\",\"uri\":\"https://example.com/\"}]}"
            "]}";

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadFirefoxJson(json, root));

        QCOMPARE(Dump(root),
                 QStringLiteral("+ \n"
                                "  + toolbar\n"
                                "    - Example [https://example.com/]\n"));
        delete root;
    }

    void readsTheIeFavoritesDirectory(){
        const QString favorites = Path(QStringLiteral("Favorites"));
        QVERIFY(QDir().mkpath(favorites + QStringLiteral("/Sub")));

        WriteAll(favorites + QStringLiteral("/Example.url"),
                 "[InternetShortcut]\r\nURL=https://example.com/\r\n");
        WriteAll(favorites + QStringLiteral("/Sub/Inner.website"),
                 "[InternetShortcut]\r\nURL=https://example.org/\r\n");

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadIeFavorites(favorites, root));

        QCOMPARE(Dump(root),
                 QStringLiteral("+ Favorites\n"
                                "  - Example [https://example.com/]\n"
                                "  + Sub\n"
                                "    - Inner [https://example.org/]\n"));
        delete root;
    }

    void readsXbelWrittenByAnotherBrowser(){
        const QByteArray xbel =
            "<?xml version=\"1.0\"?>\n"
            "<xbel version=\"1.0\">\n"
            "  <info><metadata owner=\"somebody\"/></info>\n"
            "  <folder folded=\"no\">\n"
            "    <title>Folder</title>\n"
            "    <separator/>\n"
            "    <bookmark href=\"https://example.com/\"><title>Example</title></bookmark>\n"
            "    <alias ref=\"nowhere\"/>\n"
            "  </folder>\n"
            "</xbel>\n";

        ViewNode *root = new ViewNode();
        QVERIFY(BookmarkIO::ReadXbel(xbel, root));

        QCOMPARE(Dump(root),
                 QStringLiteral("+ Folder\n"
                                "  - Example [https://example.com/]\n"));
        delete root;
    }

    void abrokenFileIsRefusedWithoutTouchingTheTree(){
        ViewNode *root = new ViewNode();
        MakeTab(root, QStringLiteral("Already here"), QStringLiteral("https://example.com/"));
        const QString before = Dump(root);

        QVERIFY(!BookmarkIO::ReadChromeJson("{\"roots\": ", root));
        QVERIFY(!BookmarkIO::ReadFirefoxJson("not json at all", root));
        QVERIFY(!BookmarkIO::ReadInternalXml("<viewnode><unclosed>", root));
        QVERIFY(!BookmarkIO::ReadXbel("this is not xml", root));
        QVERIFY(!BookmarkIO::ReadIeFavorites(Path(QStringLiteral("no-such-directory")), root));
        QVERIFY(!BookmarkIO::ReadNetscapeHtml(QStringLiteral("this is not html"), root));
        QVERIFY(!BookmarkIO::ReadNetscapeHtml(QString(), root));

        QCOMPARE(Dump(root), before);
        delete root;
    }

    void afileWhichIsNotThereIsRefused(){
        ViewNode *root = new ViewNode();
        const QString missing = Path(QStringLiteral("no-such-file"));

        QVERIFY(!BookmarkIO::ReadInternalXmlFile(missing, root));
        QVERIFY(!BookmarkIO::ReadXbelFile(missing, root));
        QVERIFY(!BookmarkIO::ReadNetscapeHtmlFile(missing, root));
        QVERIFY(!BookmarkIO::ReadChromeJsonFile(missing, root));
        QVERIFY(!BookmarkIO::ReadFirefoxJsonFile(missing, root));

        QCOMPARE(root->GetChildren().length(), 0);
        delete root;
    }

    void animportIsAppendedAfterWhatIsAlreadyThere(){
        ViewNode *root = new ViewNode();
        MakeTab(root, QStringLiteral("Open tab"), QStringLiteral("https://example.com/"));

        QVERIFY(BookmarkIO::ReadXbel("<xbel><bookmark href=\"https://example.org/\">"
                                     "<title>Imported</title></bookmark></xbel>", root));
        QCOMPARE(Dump(root),
                 QStringLiteral("- Open tab [https://example.com/]\n"
                                "- Imported [https://example.org/]\n"));
        delete root;
    }

    void thefileCallsAreTheSameAsTheInMemoryOnes(){
        ViewNode *root = MakeTree();

        const QString xml  = Path(QStringLiteral("tree.xml"));
        const QString xbel = Path(QStringLiteral("tree.xbel"));
        const QString html = Path(QStringLiteral("tree.html"));

        QVERIFY(BookmarkIO::WriteInternalXmlFile(xml, root));
        QVERIFY(BookmarkIO::WriteXbelFile(xbel, root, UTC_OFFSET));
        QVERIFY(BookmarkIO::WriteNetscapeHtmlFile(html, root, UTC_OFFSET));

        QCOMPARE(ReadAll(xml),  QString::fromUtf8(BookmarkIO::WriteInternalXml(root)));
        QCOMPARE(ReadAll(xbel), QString::fromUtf8(BookmarkIO::WriteXbel(root, UTC_OFFSET)));
        QCOMPARE(ReadAll(html), BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET));

        ViewNode *reread = new ViewNode();
        QVERIFY(BookmarkIO::ReadInternalXmlFile(xml, reread));
        QCOMPARE(Dump(reread), Dump(root));

        delete reread;
        delete root;
    }

    void thelocalOffsetIsTheOneTheOldCodeComputed(){
        QDateTime local = QDateTime::currentDateTime();
        QDateTime utc = local.toUTC();
        utc.setTimeZone(QTimeZone::LocalTime);

        QCOMPARE(BookmarkIO::LocalUtcOffset(), static_cast<int>(utc.secsTo(local)));
    }

    void adateIsTheSameInstantInBothFormats(){
        ViewNode *root = new ViewNode();
        MakeTab(root, QStringLiteral("Tab"), QStringLiteral("https://example.com/"));

        QVERIFY(QString::fromUtf8(BookmarkIO::WriteXbel(root, UTC_OFFSET))
                .contains(QStringLiteral("added=\"2026-01-01T12:00:00+09:00\"")));
        QVERIFY(BookmarkIO::WriteNetscapeHtml(root, UTC_OFFSET)
                .contains(QStringLiteral("ADD_DATE=\"%1\"").arg(Epoch(0))));

        delete root;
    }

    void everyBrowserHasAPlaceToLookOnThisPlatform(){
#if defined(Q_OS_WIN)
        QVERIFY(!BookmarkIO::IeFavoritesDirectory().isEmpty());
#endif
        QVERIFY(!BookmarkIO::FirefoxProfileDirectory().isEmpty());
        QVERIFY(!BookmarkIO::ChromeBookmarkFile().isEmpty());
        QVERIFY(!BookmarkIO::OperaBookmarkFile().isEmpty());
        QVERIFY(!BookmarkIO::VivaldiBookmarkFile().isEmpty());
        QVERIFY(!BookmarkIO::EdgeBookmarkFile().isEmpty());

        const QStringList chromeFamily = {
            BookmarkIO::ChromeBookmarkFile(),
            BookmarkIO::OperaBookmarkFile(),
            BookmarkIO::VivaldiBookmarkFile(),
            BookmarkIO::EdgeBookmarkFile(),
        };
        for(const QString &file : chromeFamily)
            QVERIFY2(file.endsWith(QStringLiteral("/Bookmarks")), qPrintable(file));
        QCOMPARE(QSet<QString>(chromeFamily.begin(), chromeFamily.end()).size(),
                 chromeFamily.size());
    }
};

QTEST_MAIN(tst_bookmarkio)
#include "tst_bookmarkio.moc"
