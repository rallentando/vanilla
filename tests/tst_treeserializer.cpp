#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDateTime>
#include <QLocale>
#include <QTimeZone>
#include <QUrl>

#include "lightnode.hpp"
#include "treeserializer.hpp"

#include "testsupport.hpp"

namespace {

class Booting {
public:
    Booting(){ Node::SetBooting(true);}
    ~Booting(){ Node::SetBooting(false);}
};

const QString BASE_DATE = QStringLiteral("20260101120000");

QDateTime Date(int secs = 0){
    return QDateTime::fromString(BASE_DATE, NODE_DATETIME_FORMAT).addSecs(secs);
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

QString ReadAll(const QString &path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    QString text = QString::fromUtf8(file.readAll());
    file.close();
    return text;
}

void WriteAll(const QString &path, const QString &text){
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(text.toUtf8());
    file.close();
}

}

class tst_treeserializer : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_Dir;

    QString Path(const QString &name) const {
        return m_Dir.path() + QStringLiteral("/") + name;
    }

    QString RoundTrip(ViewNode *root, const TreeSerializer::Hooks &hooks = TreeSerializer::Hooks()){
        const QString first  = Path(QStringLiteral("first.json"));
        const QString second = Path(QStringLiteral("second.json"));

        if(!TreeSerializer::WriteJsonFile(first, root, hooks)) return QString();

        ViewNode *reread = new ViewNode();
        {
            Booting booting;
            if(!TreeSerializer::ReadJsonFile(first, reread, hooks)){
                delete reread;
                return QString();
            }
        }
        bool written = TreeSerializer::WriteJsonFile(second, reread, hooks);
        delete reread;
        if(!written) return QString();

        const QString a = ReadAll(first);
        const QString b = ReadAll(second);
        if(a != b) return QString();
        return a;
    }

private slots:
    void initTestCase();

    void escapesTheStringsItWrites();
    void escapesTheStringsItWrites_data();

    void roundTripKeepsTheTree();
    void roundTripKeepsAwkwardTitles();
    void roundTripKeepsAwkwardTitles_data();
    void roundTripKeepsAnEmptyTree();

    void readsAHandWrittenFile();
    void readsZoomWrittenAsAPercentage();
    void readMarksUnreadNodesRead();

    void refusesAMissingFile();
    void refusesABrokenFile();

    void trashLeavesOutTheSideFiles();
    void windowIdsGoThroughTheHooks();
    void theTickIsSpacedOutInTime();

    void stampsWithTheZoneAgreeWithThePlainForm();
    void theStampReaderAgreesWithTheZoneForm();

};

void tst_treeserializer::initTestCase(){
    TestSupport::SilenceDebugOutput();
    QVERIFY2(m_Dir.isValid(), "could not create a temporary directory.");
}

void tst_treeserializer::escapesTheStringsItWrites_data(){
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain")        << QStringLiteral("hello")  << QStringLiteral("hello");
    QTest::newRow("quote")        << QStringLiteral("a\"b")   << QStringLiteral("a\\\"b");
    QTest::newRow("backslash")    << QStringLiteral("a\\b")   << QStringLiteral("a\\\\b");
    QTest::newRow("both")         << QStringLiteral("\\\"")   << QStringLiteral("\\\\\\\"");
    QTest::newRow("newline")      << QStringLiteral("a\nb")   << QStringLiteral("a\\nb");
    QTest::newRow("tab")          << QStringLiteral("a\tb")   << QStringLiteral("a\\tb");
    QTest::newRow("control")      << QStringLiteral("a\x01""b") << QStringLiteral("a\\u0001b");
    QTest::newRow("japanese")     << QStringLiteral("\u30bf\u30d6") << QStringLiteral("\u30bf\u30d6");
    QTest::newRow("percent")      << QStringLiteral("100%2")  << QStringLiteral("100%2");
}

void tst_treeserializer::escapesTheStringsItWrites(){
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(TreeSerializer::JsonEscape(input), expected);
}

void tst_treeserializer::roundTripKeepsTheTree(){
    ViewNode *root = new ViewNode();
    {
        Booting booting;

        ViewNode *tab = MakeTab(root, QStringLiteral("first"), QStringLiteral("https://example.com/"));
        tab->SetScrollX(12);
        tab->SetScrollY(345);
        tab->SetZoom(1.25f);
#ifdef MEDIATIME
        tab->SetMediaTime(78.5f);
#endif
        root->SetPrimary(tab);

        ViewNode *folder = MakeFolder(root, QStringLiteral("folder"));
        folder->SetFolded(false);
        ViewNode *nested = MakeFolder(folder, QStringLiteral("nested"));
        MakeTab(nested, QStringLiteral("deep"), QStringLiteral("https://example.com/deep?a=1&b=2"));
        folder->SetPrimary(nested);

        MakeFolder(root, QStringLiteral("empty"));
    }

    const QString text = RoundTrip(root);
    QVERIFY2(!text.isEmpty(), "the two passes did not produce the same text.");

    QVERIFY(text.contains(QStringLiteral("\"url\": \"https://example.com/\"")));
    QVERIFY(text.contains(QStringLiteral("\"scrollx\": 12")));
    QVERIFY(text.contains(QStringLiteral("\"scrolly\": 345")));
    QVERIFY(text.contains(QStringLiteral("\"zoom\": 1.25")));
#ifdef MEDIATIME
    QVERIFY(text.contains(QStringLiteral("\"mediatime\": 78.5")));
#endif
    QVERIFY(text.contains(QStringLiteral("\"title\": \"nested\"")));
    QVERIFY(text.contains(QStringLiteral("\"create\": \"") + BASE_DATE + QStringLiteral("\"")));
    QVERIFY(text.contains(QStringLiteral("\"children\": []")));

    QCOMPARE(text.count(QStringLiteral("\"primary\": true")), 2);
    QVERIFY(text.contains(QStringLiteral("deep?a=1&b=2")));

    delete root;
}

void tst_treeserializer::roundTripKeepsAwkwardTitles_data(){
    QTest::addColumn<QString>("title");

    QTest::newRow("quote")     << QStringLiteral("a \"quoted\" title");
    QTest::newRow("backslash") << QStringLiteral("C:\\path\\to");
    QTest::newRow("both")      << QStringLiteral("\\\"");
    QTest::newRow("newline")   << QStringLiteral("two\nlines");
    QTest::newRow("control")   << QStringLiteral("bell\x07here");
    QTest::newRow("japanese")  << QStringLiteral("\u65e5\u672c\u8a9e\u306e\u30bf\u30a4\u30c8\u30eb");
    QTest::newRow("percent")   << QStringLiteral("50%% off %1");
    QTest::newRow("markup")    << QStringLiteral("a <b>bold</b> title");
    QTest::newRow("empty")     << QString();
}

void tst_treeserializer::roundTripKeepsAwkwardTitles(){
    QFETCH(QString, title);

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        MakeFolder(root, title);
        MakeTab(root, title, QStringLiteral("https://example.com/"));
    }

    QVERIFY2(!RoundTrip(root).isEmpty(), "the two passes did not produce the same text.");

    ViewNode *reread = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(Path(QStringLiteral("first.json")), reread));
    }
    QCOMPARE(reread->ChildrenLength(), 2);
    QCOMPARE(reread->GetChildAt(0)->GetTitle(), title);
    QCOMPARE(reread->GetChildAt(1)->GetTitle(), title);

    delete reread;
    delete root;
}

void tst_treeserializer::roundTripKeepsAnEmptyTree(){
    ViewNode *root = new ViewNode();

    const QString path = Path(QStringLiteral("empty.json"));
    QVERIFY(TreeSerializer::WriteJsonFile(path, root));

    ViewNode *reread = new ViewNode();
    {
        Booting booting;
        QVERIFY2(TreeSerializer::ReadJsonFile(path, reread),
                 "an empty tree has to read back as valid json.");
    }
    QCOMPARE(reread->ChildrenLength(), 0);

    delete reread;
    delete root;
}

void tst_treeserializer::readsAHandWrittenFile(){
    const QString path = Path(QStringLiteral("hand.json"));
    WriteAll(path, QStringLiteral(R"json({
    "children": [
        {
            "primary": true,
            "holdview": false,
            "folded": false,
            "title": "folder",
            "create": "20260101120000",
            "lastupdate": "20260101120100",
            "lastaccess": "20260101120200",
            "children": [
                {
                    "primary": true,
                    "holdview": true,
                    "folded": true,
                    "title": "tab",
                    "create": "20260101120000",
                    "lastupdate": "20260101120100",
                    "lastaccess": "20260101120200",
                    "index": 0,
                    "url": "https://example.com/",
                    "scrollx": 7,
                    "scrolly": 8,
                    "zoom": 1.5,
                    "mediatime": 78.5,
                    "history": "hist.dat",
                    "thumb": "thumb.jpg"
                }
            ]
        }
    ]
}
)json"));

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(path, root));
    }

    QCOMPARE(root->ChildrenLength(), 1);

    ViewNode *folder = root->GetChildAt(0)->ToViewNode();
    QVERIFY(folder);
    QCOMPARE(folder->GetTitle(), QStringLiteral("folder"));
    QVERIFY(!folder->HoldsView());
    QVERIFY(folder->IsDirectory());
    QVERIFY(!folder->GetFolded());
    QVERIFY(folder->IsPrimaryOfParent());
    QCOMPARE(folder->GetCreateDate(), Date(0));
    QCOMPARE(folder->GetLastUpdateDate(), Date(60));
    QCOMPARE(folder->GetLastAccessDate(), Date(120));
    QCOMPARE(folder->ChildrenLength(), 1);

    ViewNode *tab = folder->GetChildAt(0)->ToViewNode();
    QVERIFY(tab);
    QCOMPARE(tab->GetTitle(), QStringLiteral("tab"));
    QVERIFY(tab->HoldsView());
    QVERIFY(!tab->IsDirectory());
    QVERIFY(tab->GetFolded());
    QVERIFY(tab->IsPrimaryOfParent());
    QCOMPARE(tab->GetUrl(), QUrl(QStringLiteral("https://example.com/")));
    QCOMPARE(tab->GetScrollX(), 7);
    QCOMPARE(tab->GetScrollY(), 8);
    QCOMPARE(tab->GetZoom(), 1.5f);
#ifdef MEDIATIME
    QCOMPARE(tab->GetMediaTime(), 78.5f);
#endif
    QCOMPARE(tab->GetHistoryFileName(), QStringLiteral("hist.dat"));
    QCOMPARE(tab->GetImageFileName(), QStringLiteral("thumb.jpg"));

    delete root;
}

void tst_treeserializer::readsZoomWrittenAsAPercentage(){
    const QString path = Path(QStringLiteral("zoom.json"));
    WriteAll(path, QStringLiteral(R"json({
    "children": [
        {"holdview": true, "title": "a", "zoom": 125, "url": "https://example.com/"},
        {"holdview": true, "title": "b", "zoom": 1.25, "url": "https://example.com/"},
        {"holdview": true, "title": "c", "url": "https://example.com/"}
    ]
}
)json"));

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(path, root));
    }

    QCOMPARE(root->ChildrenLength(), 3);
    QCOMPARE(root->GetChildAt(0)->GetZoom(), 1.25f);
    QCOMPARE(root->GetChildAt(1)->GetZoom(), 1.25f);
    QCOMPARE(root->GetChildAt(2)->GetZoom(), 1.0f);
#ifdef MEDIATIME
    QCOMPARE(root->GetChildAt(2)->ToViewNode()->GetMediaTime(), 0.0f);
#endif

    delete root;
}

void tst_treeserializer::readMarksUnreadNodesRead(){
    const QString path = Path(QStringLiteral("unread.json"));
    WriteAll(path, QStringLiteral(R"json({
    "children": [
        {"holdview": true, "title": "a", "url": "https://example.com/",
         "create": "20260101120000", "lastupdate": "20260101120000", "lastaccess": "20260101120000"}
    ]
}
)json"));

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(path, root));
    }

    QCOMPARE(root->ChildrenLength(), 1);
    QCOMPARE(root->GetChildAt(0)->GetLastAccessDate(), Date(1));
    QVERIFY(root->GetChildAt(0)->IsRead());

    delete root;
}

void tst_treeserializer::refusesAMissingFile(){
    ViewNode *root = new ViewNode();
    MakeFolder(root, QStringLiteral("kept"));

    QVERIFY(!TreeSerializer::ReadJsonFile(Path(QStringLiteral("nothing.json")), root));

    QCOMPARE(root->ChildrenLength(), 1);

    delete root;
}

void tst_treeserializer::refusesABrokenFile(){
    ViewNode *root = new ViewNode();
    MakeFolder(root, QStringLiteral("kept"));

    const QString truncated = Path(QStringLiteral("truncated.json"));
    WriteAll(truncated, QStringLiteral("{\n    \"children\": [\n        {\"title\": \"a\""));
    QVERIFY(!TreeSerializer::ReadJsonFile(truncated, root));

    const QString array = Path(QStringLiteral("array.json"));
    WriteAll(array, QStringLiteral("[1, 2, 3]"));
    QVERIFY(!TreeSerializer::ReadJsonFile(array, root));

    const QString empty = Path(QStringLiteral("nothing_at_all.json"));
    WriteAll(empty, QString());
    QVERIFY(!TreeSerializer::ReadJsonFile(empty, root));

    QCOMPARE(root->ChildrenLength(), 1);

    delete root;
}

void tst_treeserializer::trashLeavesOutTheSideFiles(){
    TreeSerializer::Hooks trash;
    trash.KeepsSideFiles = [](ViewNode*){ return false;};

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        MakeTab(root, QStringLiteral("gone"), QStringLiteral("https://example.com/"));
    }

    const QString text = RoundTrip(root, trash);
    QVERIFY2(!text.isEmpty(), "the two passes did not produce the same text.");
    QVERIFY(!text.contains(QStringLiteral("\"history\"")));
    QVERIFY(!text.contains(QStringLiteral("\"thumb\"")));
    QVERIFY(text.contains(QStringLiteral("\"holdview\": true")));

    delete root;
}

void tst_treeserializer::windowIdsGoThroughTheHooks(){
    TreeSerializer::Hooks hooks;
    hooks.WindowIndexOf = [](ViewNode *nd){
        return nd->GetTitle() == QStringLiteral("shown") ? 7 : 0;
    };

    ViewNode *root = new ViewNode();
    {
        Booting booting;
        MakeTab(root, QStringLiteral("shown"),  QStringLiteral("https://example.com/a"));
        MakeTab(root, QStringLiteral("hidden"), QStringLiteral("https://example.com/b"));
    }

    const QString path = Path(QStringLiteral("windows.json"));
    QVERIFY(TreeSerializer::WriteJsonFile(path, root, hooks));

    const QString text = ReadAll(path);
    QVERIFY(text.contains(QStringLiteral("\"index\": 7")));
    QVERIFY(text.contains(QStringLiteral("\"index\": 0")));

    QList<int> restored;
    QStringList titles;
    TreeSerializer::Hooks reading;
    reading.RestoreIntoWindow = [&](ViewNode *nd, int id){
        restored << id;
        titles << nd->GetTitle();
    };

    ViewNode *reread = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(path, reread, reading));
    }

    QCOMPARE(restored, QList<int>() << 7);
    QCOMPARE(titles, QStringList() << QStringLiteral("shown"));

    delete reread;
    delete root;
}

void tst_treeserializer::theTickIsSpacedOutInTime(){
    const int count = 2000;
    ViewNode *root = new ViewNode();
    {
        Booting booting;
        for(int i = 0; i < count; i++)
            MakeTab(root, QStringLiteral("tab %1").arg(i), QStringLiteral("https://example.com/%1").arg(i));
    }
    const QString path = Path(QStringLiteral("ticks.json"));
    QVERIFY(TreeSerializer::WriteJsonFile(path, root));

    int everyNode = 0;
    TreeSerializer::Hooks perNode;
    perNode.TickIntervalMs = 0;
    perNode.Tick = [&](){ everyNode++; };
    ViewNode *first = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(path, first, perNode));
    }
    QCOMPARE(everyNode, count);

    int spaced = 0;
    TreeSerializer::Hooks timed;
    timed.TickIntervalMs = 1000;
    timed.Tick = [&](){ spaced++; };
    ViewNode *second = new ViewNode();
    {
        Booting booting;
        QVERIFY(TreeSerializer::ReadJsonFile(path, second, timed));
    }
    QCOMPARE(spaced, 1);

    delete second;
    delete first;
    delete root;
}

void tst_treeserializer::theStampReaderAgreesWithTheZoneForm(){
    const QString format = QStringLiteral("yyyyMMddhhmmss");
    const QLocale locale;
    auto same = [&](const QDateTime &fast, const QDateTime &slow, const QString &what){
        QVERIFY2(fast.isValid() == slow.isValid(), qPrintable(what));
        if(!slow.isValid()) return;
        QVERIFY2(fast.toMSecsSinceEpoch() == slow.toMSecsSinceEpoch(), qPrintable(what));
        QCOMPARE(fast.date(), slow.date());
        QCOMPARE(fast.time(), slow.time());
        QCOMPARE(fast.offsetFromUtc(), slow.offsetFromUtc());
        QCOMPARE(fast.toString(NODE_DATETIME_FORMAT), slow.toString(NODE_DATETIME_FORMAT));
        QCOMPARE(locale.toString(fast, QLocale::LongFormat), locale.toString(slow, QLocale::LongFormat));
        QCOMPARE(fast.addSecs(1).toMSecsSinceEpoch(), slow.addSecs(1).toMSecsSinceEpoch());
    };
    const QTime times[] = { QTime(0, 0), QTime(0, 30), QTime(2, 30), QTime(12, 0), QTime(23, 59, 59) };

    {
        const QTimeZone zone = QTimeZone::systemTimeZone();
        QVERIFY(zone.isValid());
        TreeSerializer::StampReader reader(zone);
        int plain = 0;
        for(QDate day(2024, 1, 1); day <= QDate(2025, 12, 31); day = day.addDays(1)){
            if(reader.PlainDay(day)) plain++;
            for(const QTime &time : times){
                const QString stamp = QDateTime(day, time).toString(format);
                same(reader.Stamp(stamp), NodeDateTimeFromString(stamp, zone), stamp);
            }
        }
        QVERIFY(plain >= 731 - 8);
    }
    struct Case { const char *zone; QDate day; };
    const Case cases[] = {
        { "Europe/Berlin",    QDate(2024, 3, 31) },
        { "Europe/Berlin",    QDate(2024, 10, 27) },
        { "America/Santiago", QDate(2024, 9, 8) },
        { "America/Santiago", QDate(2024, 4, 6) },
        { "Pacific/Apia",     QDate(2011, 12, 30) },
    };
    for(const Case &c : cases){
        const QTimeZone zone(QByteArray(c.zone));
        if(!zone.isValid()) continue;
        TreeSerializer::StampReader reader(zone);
        if(c.day.year() != 2011) QVERIFY2(!reader.PlainDay(c.day), c.zone);
        for(int offset = -1; offset <= 1; offset++){
            const QDate day = c.day.addDays(offset);
            for(const QTime &time : times){
                const QString stamp = QDateTime(day, time).toString(format);
                same(reader.Stamp(stamp), NodeDateTimeFromString(stamp, zone), QString::fromLatin1(c.zone) + QLatin1Char(' ') + stamp);
            }
        }
    }
    {
        const QTimeZone light = QTimeZone::fromSecondsAheadOfUtc(9 * 3600);
        QVERIFY(light.isUtcOrFixedOffset());
        TreeSerializer::StampReader reader(light);
        QVERIFY(reader.PlainDay(QDate(2024, 3, 31)));
        same(reader.Stamp(QStringLiteral("20240331120000")), NodeDateTimeFromString(QStringLiteral("20240331120000"), light), QStringLiteral("light"));
        const QTimeZone named(9 * 3600);
        QVERIFY(named.isValid() && !named.hasTransitions());
        TreeSerializer::StampReader slow(named);
        QVERIFY(!slow.PlainDay(QDate(2024, 3, 31)));
        same(slow.Stamp(QStringLiteral("20240331120000")), NodeDateTimeFromString(QStringLiteral("20240331120000"), named), QStringLiteral("named"));
    }
    {
        const QTimeZone zone = QTimeZone::systemTimeZone();
        TreeSerializer::StampReader reader(zone);
        const QString bad[] = {
            QStringLiteral("2024010112000"), QStringLiteral("202401011200000"), QStringLiteral("2024010112000a"),
            QStringLiteral("20240230120000"), QStringLiteral("20240101250000"), QString() };
        for(const QString &stamp : bad){
            QVERIFY2(!reader.Stamp(stamp).isValid(), qPrintable(stamp));
            QVERIFY2(!NodeDateTimeFromString(stamp, zone).isValid(), qPrintable(stamp));
        }
        TreeSerializer::StampReader bare((QTimeZone()));
        QCOMPARE(bare.Stamp(BASE_DATE).toMSecsSinceEpoch(), NodeDateTimeFromString(BASE_DATE, QTimeZone()).toMSecsSinceEpoch());
    }
}

void tst_treeserializer::stampsWithTheZoneAgreeWithThePlainForm(){
    const QTimeZone zone = QTimeZone::systemTimeZone();
    QVERIFY(zone.isValid());
    const QLocale locale;
    const QTime times[] = { QTime(0, 30), QTime(2, 30), QTime(12, 0), QTime(23, 30) };
    int checked = 0;
    for(QDate day(2024, 1, 1); day <= QDate(2025, 12, 31); day = day.addDays(1)){
        for(const QTime &time : times){
            const QString stamp = day.toString(QStringLiteral("yyyyMMdd")) + time.toString(QStringLiteral("hhmmss"));
            const QDateTime fast = NodeDateTimeFromString(stamp, zone);
            const QDateTime plain(day, time);
            QVERIFY2(fast.isValid() == plain.isValid(), qPrintable(stamp));
            if(!plain.isValid()) continue;
            QCOMPARE(fast.toMSecsSinceEpoch(), plain.toMSecsSinceEpoch());
            QCOMPARE(fast.date(), plain.date());
            QCOMPARE(fast.time(), plain.time());
            QCOMPARE(fast.offsetFromUtc(), plain.offsetFromUtc());
            QCOMPARE(fast.toString(NODE_DATETIME_FORMAT), plain.toString(NODE_DATETIME_FORMAT));
            QCOMPARE(locale.toString(fast, QLocale::LongFormat), locale.toString(plain, QLocale::LongFormat));
            QCOMPARE(fast.addSecs(1).toMSecsSinceEpoch(), plain.addSecs(1).toMSecsSinceEpoch());
            checked++;
        }
    }
    QVERIFY(checked > 2900);
    QCOMPARE(NodeDateTimeFromString(BASE_DATE).toMSecsSinceEpoch(), Date(0).toMSecsSinceEpoch());
    const QDateTime bare = NodeDateTimeFromString(BASE_DATE, QTimeZone());
    QCOMPARE(bare.timeSpec(), Qt::LocalTime);
    QCOMPARE(bare.toMSecsSinceEpoch(), QDateTime(QDate(2026, 1, 1), QTime(12, 0)).toMSecsSinceEpoch());
    const QTimeZone berlin(QByteArrayLiteral("Europe/Berlin"));
    if(berlin.isValid()){
        const QDateTime gap = NodeDateTimeFromString(QStringLiteral("20240331023000"), berlin);
        QCOMPARE(gap.time(), QTime(3, 30));
        QCOMPARE(gap.offsetFromUtc(), 7200);
        const QDateTime fold = NodeDateTimeFromString(QStringLiteral("20241027023000"), berlin);
        QCOMPARE(fold.time(), QTime(2, 30));
        QCOMPARE(fold.offsetFromUtc(), 7200);
    }
}

QTEST_MAIN(tst_treeserializer)
#include "tst_treeserializer.moc"
