#include "switch.hpp"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>

#include "extensionmainscripts.hpp"

#include "testsupport.hpp"

class tst_extensionmainscripts : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void aPatternIsMatchedAgainstTheWholeAddress_data();
    void aPatternIsMatchedAgainstTheWholeAddress();
    void whatCouldEndItsLineIsNoPattern();
    void theHeaderIsOneIncludeLineOfAllThree();
    void aListIsTakenWholeOrNotAtAll();
    void aFileNameStaysInsideTheCopy();
    void aFileIsReadOnlyFromInsideTheFolder();
    void theScriptsAreTheFilesInTheirOrder();
    void theProfileIsGivenOnlyWhatChanged();
    void whatPersistsIsKeptAsItWasSent();
    void whatWasKeptIsPutInOnlyForTheSameVersion();
    void whatWasKeptIsReadAgainOutOfTheCopy();
};

void tst_extensionmainscripts::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

namespace {
    bool Runs(const QString &regex, const QString &url){
        QRegularExpression re(regex, QRegularExpression::CaseInsensitiveOption);
        return re.isValid() && re.match(url).hasMatch();
    }
    QString IncludeOf(const QString &header){
        static const QRegularExpression line(QStringLiteral("^// @include /(.*)/$"), QRegularExpression::MultilineOption);
        return line.match(header).captured(1);
    }
    QString *Ignored(){ static QString error; return &error; }
    QJsonArray ListOf(const char *json){
        return QJsonArray() << QJsonDocument::fromJson(QByteArray(json)).array();
    }
}

void tst_extensionmainscripts::aPatternIsMatchedAgainstTheWholeAddress_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("host");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("runs");
    auto row = [](const char *name, const char *pattern, bool host, const char *url, bool runs){
        QTest::newRow(name) << QString::fromUtf8(pattern) << host << QString::fromUtf8(url) << runs; };
    row("under: the host itself", "*://*.a.com/*", false, "http://a.com/", true);
    row("under: a name under it", "*://*.a.com/*", false, "https://x.a.com/p?q=1", true);
    row("under: a port", "*://*.a.com/*", false, "http://a.com:8080/x", true);
    row("under: a user", "*://*.a.com/*", false, "http://u:p@a.com/", true);
    row("under: a fragment", "*://*.a.com/*", false, "http://a.com/#f", true);
    row("under: not a longer name", "*://*.a.com/*", false, "http://ba.com/", false);
    row("under: not another scheme", "*://*.a.com/*", false, "ftp://a.com/", false);
    row("under: not a name it begins", "*://*.a.com/*", false, "http://a.com.evil/", false);
    row("under: not in the path", "*://*.a.com/*", false, "https://evil.com/x.a.com/", false);
    row("under: not in the query", "*://*.a.com/*", false, "https://evil.com/?https://a.com/", false);
    row("under: not as the user", "*://*.a.com/*", false, "http://a.com@evil.com/", false);
    row("exact: the path", "https://a.com/exact", false, "https://a.com/exact", true);
    row("exact: and a fragment", "https://a.com/exact", false, "https://a.com/exact#f", true);
    row("exact: not with a query", "https://a.com/exact", false, "https://a.com/exact?q", false);
    row("exact: not longer", "https://a.com/exact", false, "https://a.com/exactly", false);
    row("exact: not http", "https://a.com/exact", false, "http://a.com/exact", false);
    row("all: http", "<all_urls>", false, "http://x/", true);
    row("all: not a file", "<all_urls>", false, "file:///c:/a.html", false);
    row("all: not qrc", "<all_urls>", false, "qrc:/a.html", false);
    row("file: read, matches nothing", "file:///*", false, "file:///c:/a.html", false);
    row("port: the default is not written", "http://a.com:80/*", false, "http://a.com/", true);
    row("port: another is", "http://a.com:8080/*", false, "http://a.com/", false);
    row("port: that one", "http://a.com:8080/*", false, "http://a.com:8080/", true);
    row("port: not http's default for https", "https://a.com:80/*", false, "https://a.com/", false);
    row("port: not https's default for http", "*://a.com:443/*", false, "http://a.com/", false);
    row("port: https's default for https", "*://a.com:443/*", false, "https://a.com/", true);
    row("any host: a v6 literal", "*://*/*", false, "http://[::1]:8080/", true);
    row("host permission: the path is not read", "https://a.com/", true, "https://a.com/any/path", true);
    row("pattern: the path is", "https://a.com/", false, "https://a.com/any/path", false);
}

void tst_extensionmainscripts::aPatternIsMatchedAgainstTheWholeAddress(){
    QFETCH(QString, pattern);
    QFETCH(bool, host);
    QFETCH(QString, url);
    QFETCH(bool, runs);
    const QString re = ExtensionMainScripts::PatternRegex(pattern, host);
    QVERIFY(!re.isEmpty());
    QCOMPARE(Runs(QStringLiteral("^(?:") + re + QStringLiteral(")"), url), runs);
}

void tst_extensionmainscripts::whatCouldEndItsLineIsNoPattern(){
    foreach(const QString &p, QStringList()
            << QStringLiteral("http://a.com/*\n// @include *")
            << QStringLiteral("http://a.com/*\r")
            << QString::fromUtf8("http://a.com/*\u2028x")
            << QStringLiteral("http://a.com/a b")
            << QStringLiteral("http://a.com")
            << QStringLiteral("chrome://a/*")
            << QStringLiteral("http://a b/*")
            << QStringLiteral("http://*.a*b/*")
            << QStringLiteral("file://host/*")
            << QString())
        QVERIFY2(ExtensionMainScripts::PatternRegex(p).isEmpty(), qPrintable(p));
}

void tst_extensionmainscripts::theHeaderIsOneIncludeLineOfAllThree(){
    ExtensionMainScripts::Registration r;
    r.id = QStringLiteral("x");
    r.js << QStringLiteral("a.js");
    r.matches << QStringLiteral("<all_urls>");
    r.excludeMatches << QStringLiteral("*://*.a.com/skip/*");
    const QString header = ExtensionMainScripts::Header(r, QStringList() << QStringLiteral("https://*.a.com/"));
    const QStringList lines = header.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines.at(0), QStringLiteral("// ==UserScript=="));
    QVERIFY(lines.at(1).startsWith(QStringLiteral("// @include /^(?=")));
    QCOMPARE(lines.at(2), QStringLiteral("// ==/UserScript=="));
    const QString re = IncludeOf(header);
    QVERIFY(Runs(re, QStringLiteral("https://x.a.com/p")));
    QVERIFY(!Runs(re, QStringLiteral("http://x.a.com/p")));
    QVERIFY(!Runs(re, QStringLiteral("https://b.com/")));
    QVERIFY(!Runs(re, QStringLiteral("https://x.a.com/skip/1")));
    QVERIFY(Runs(re, QStringLiteral("https://x.a.com/skipper")));

    QVERIFY(ExtensionMainScripts::Header(r, QStringList()).isEmpty());
    QVERIFY(ExtensionMainScripts::Header(r, QStringList() << QStringLiteral("chrome://favicon/*")).isEmpty());
    const QString mixed = IncludeOf(ExtensionMainScripts::Header(r, QStringList() << QStringLiteral("chrome://favicon/*") << QStringLiteral("https://*.a.com/")));
    QVERIFY(Runs(mixed, QStringLiteral("https://x.a.com/p")));
    QVERIFY(!Runs(mixed, QStringLiteral("https://b.com/")));
    r.excludeMatches << QStringLiteral("nothing");
    QVERIFY(ExtensionMainScripts::Header(r, QStringList() << QStringLiteral("<all_urls>")).isEmpty());
}

void tst_extensionmainscripts::aListIsTakenWholeOrNotAtAll(){
    QString error;
    const QList<ExtensionMainScripts::Registration> taken = ExtensionMainScripts::Parse(ListOf(R"([
        {"id": "ublock-filters.main", "js": ["rulesets/scripting/scriptlet/main/ublock-filters.js"],
         "matches": ["<all_urls>"], "excludeMatches": ["*://*.example.com/*"], "allFrames": true, "runAt": "document_start"},
        {"id": "b", "js": ["b.js"], "matches": ["*://*.b.com/*"]}])"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(taken.size(), 2);
    QCOMPARE(taken.at(0).js, QStringList() << QStringLiteral("rulesets/scripting/scriptlet/main/ublock-filters.js"));
    QCOMPARE(taken.at(0).excludeMatches, QStringList() << QStringLiteral("*://*.example.com/*"));
    QVERIFY(taken.at(0).allFrames);
    QCOMPARE(taken.at(0).runAt, ExtensionMainScripts::DocumentStart);
    QVERIFY(!taken.at(1).allFrames);
    QCOMPARE(taken.at(1).runAt, ExtensionMainScripts::DocumentIdle);
    QVERIFY(ExtensionMainScripts::Parse(ListOf("[]"), &error).isEmpty() && error.isEmpty());

    foreach(const char *json, QList<const char*>()
            << R"([{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"]}, {"id": "a", "js": ["b.js"], "matches": ["<all_urls>"]}])"
            << R"([{"id": "", "js": ["a.js"], "matches": ["<all_urls>"]}])"
            << R"([{"id": "a", "js": [], "matches": ["<all_urls>"]}])"
            << R"([{"id": "a", "js": ["../a.js"], "matches": ["<all_urls>"]}])"
            << R"([{"id": "a", "js": ["/a.js"], "matches": ["<all_urls>"]}])"
            << R"([{"id": "a", "js": ["a.js"], "matches": []}])"
            << R"([{"id": "a", "js": ["a.js"], "matches": ["http://a.com/*\n// @include *"]}])"
            << R"([{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "excludeMatches": ["nothing"]}])"
            << R"([{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "allFrames": "yes"}])"
            << R"([{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "runAt": "later"}])"
            << R"([{"id": "a", "js": "a.js", "matches": ["<all_urls>"]}])"
            << R"(["a"])"){
        const QList<ExtensionMainScripts::Registration> refused = ExtensionMainScripts::Parse(ListOf(json), &error);
        QVERIFY2(refused.isEmpty() && !error.isEmpty(), json);
    }
    QVERIFY(ExtensionMainScripts::Parse(QJsonArray(), &error).isEmpty() && !error.isEmpty());
    QJsonArray many;
    for(int i = 0; i <= ExtensionMainScripts::SCRIPTS_LIMIT; i++)
        many.append(QJsonDocument::fromJson(QStringLiteral("{\"id\": \"s%1\", \"js\": [\"a.js\"], \"matches\": [\"<all_urls>\"]}").arg(i).toUtf8()).object());
    QVERIFY(ExtensionMainScripts::Parse(QJsonArray() << many, &error).isEmpty() && !error.isEmpty());
    many.removeLast();
    QCOMPARE(ExtensionMainScripts::Parse(QJsonArray() << many, &error).size(), ExtensionMainScripts::SCRIPTS_LIMIT);
}

void tst_extensionmainscripts::aFileNameStaysInsideTheCopy(){
    QVERIFY(ExtensionMainScripts::FileNameOk(QStringLiteral("a.js")));
    QVERIFY(ExtensionMainScripts::FileNameOk(QStringLiteral("rulesets/scripting/scriptlet/main/ublock-filters.js")));
    foreach(const QString &name, QStringList()
            << QString() << QStringLiteral("/a.js") << QStringLiteral("../a.js") << QStringLiteral("a/../../b.js")
            << QStringLiteral("./a.js") << QStringLiteral("a//b.js") << QStringLiteral("a\\b.js") << QStringLiteral("c:/a.js")
            << QStringLiteral("a.js:stream") << QStringLiteral("a.js?x") << QStringLiteral("a.js#x")
            << QStringLiteral("con.js") << QStringLiteral("a/NUL") << QStringLiteral("com1.txt")
            << QStringLiteral("a.js.") << QStringLiteral("a.js ") << QStringLiteral("a\tb.js"))
        QVERIFY2(!ExtensionMainScripts::FileNameOk(name), qPrintable(name));
}

void tst_extensionmainscripts::aFileIsReadOnlyFromInsideTheFolder(){
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString copy = dir.filePath(QStringLiteral("copy"));
    const QString outside = dir.filePath(QStringLiteral("outside"));
    QVERIFY(QDir().mkpath(copy + QStringLiteral("/sub")) && QDir().mkpath(copy + QStringLiteral("/folder")) && QDir().mkpath(outside));
    auto write = [](const QString &path, const QByteArray &bytes){
        QFile f(path);
        return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
    };
    QVERIFY(write(copy + QStringLiteral("/a.js"), "A();"));
    QVERIFY(write(copy + QStringLiteral("/sub/b.js"), "B();"));
    QVERIFY(write(outside + QStringLiteral("/secret.js"), "S();"));
    QVERIFY(write(copy + QStringLiteral("/large.js"), QByteArray(int(ExtensionMainScripts::FILE_LIMIT) + 1, 'x')));
    QByteArray bytes;
    QVERIFY(ExtensionMainScripts::ReadInside(copy, QStringLiteral("a.js"), &bytes));
    QCOMPARE(bytes, QByteArray("A();"));
    QVERIFY(ExtensionMainScripts::ReadInside(copy, QStringLiteral("sub/b.js"), &bytes));
    QCOMPARE(bytes, QByteArray("B();"));
    QVERIFY(!ExtensionMainScripts::ReadInside(copy, QStringLiteral("folder"), &bytes));
    QVERIFY(!ExtensionMainScripts::ReadInside(copy, QStringLiteral("nosuch.js"), &bytes));
    QVERIFY(!ExtensionMainScripts::ReadInside(copy, QStringLiteral("large.js"), &bytes));
    QVERIFY(!ExtensionMainScripts::ReadInside(copy, QStringLiteral("../outside/secret.js"), &bytes));
    QVERIFY(!ExtensionMainScripts::ReadInside(QString(), QStringLiteral("a.js"), &bytes));
    QVERIFY(!ExtensionMainScripts::ReadInside(dir.filePath(QStringLiteral("nosuch")), QStringLiteral("a.js"), &bytes));
#ifdef Q_OS_WIN
    QProcess mklink;
    mklink.start(QStringLiteral("cmd"), QStringList() << QStringLiteral("/c") << QStringLiteral("mklink") << QStringLiteral("/J")
                 << QDir::toNativeSeparators(copy + QStringLiteral("/link")) << QDir::toNativeSeparators(outside));
    QVERIFY(mklink.waitForFinished(10000) && mklink.exitCode() == 0);
    QVERIFY(QFile::exists(copy + QStringLiteral("/link/secret.js")));
    QVERIFY(!ExtensionMainScripts::ReadInside(copy, QStringLiteral("link/secret.js"), &bytes));
#endif
}

void tst_extensionmainscripts::theScriptsAreTheFilesInTheirOrder(){
    QString error;
    const QList<ExtensionMainScripts::Registration> list = ExtensionMainScripts::Parse(ListOf(R"([
        {"id": "one", "js": ["b.js", "a.js"], "matches": ["<all_urls>"], "allFrames": true, "runAt": "document_start"},
        {"id": "two", "js": ["a.js"], "matches": ["https://c.com/*"]}])"), &error);
    QVERIFY(error.isEmpty());
    const QHash<QString, QByteArray> files{ { QStringLiteral("a.js"), "A();" }, { QStringLiteral("b.js"), "B();" } };
    QStringList asked;
    const ExtensionMainScripts::Reader read = [&](const QString &name, QByteArray *bytes){
        asked << name;
        if(!files.contains(name)) return false;
        *bytes = files.value(name);
        return true;
    };
    const QStringList everywhere = QStringList() << QStringLiteral("<all_urls>");
    QList<ExtensionMainScripts::Script> scripts = ExtensionMainScripts::ScriptsOf(QStringLiteral("ext"), list, everywhere, read, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(scripts.size(), 3);
    QCOMPARE(scripts.at(0).name, QStringLiteral("vanilla-ext/ext/one/0"));
    QCOMPARE(scripts.at(1).name, QStringLiteral("vanilla-ext/ext/one/1"));
    QCOMPARE(scripts.at(2).name, QStringLiteral("vanilla-ext/ext/two/0"));
    QVERIFY(scripts.at(0).source.startsWith(QStringLiteral("// ==UserScript==\n// @include /")));
    QVERIFY(scripts.at(0).source.endsWith(QStringLiteral("// ==/UserScript==\nB();")));
    QVERIFY(scripts.at(1).source.endsWith(QStringLiteral("\nA();")));
    QVERIFY(scripts.at(0).subFrames && !scripts.at(2).subFrames);
    QCOMPARE(scripts.at(0).runAt, ExtensionMainScripts::DocumentStart);
    QCOMPARE(scripts.at(2).runAt, ExtensionMainScripts::DocumentIdle);

    asked.clear();
    scripts = ExtensionMainScripts::ScriptsOf(QStringLiteral("ext"), list, QStringList(), read, &error);
    QVERIFY(scripts.isEmpty() && error.isEmpty() && asked.isEmpty());

    const QList<ExtensionMainScripts::Registration> missing = ExtensionMainScripts::Parse(ListOf(R"([
        {"id": "one", "js": ["a.js"], "matches": ["<all_urls>"]}, {"id": "two", "js": ["nosuch.js"], "matches": ["<all_urls>"]}])"), &error);
    scripts = ExtensionMainScripts::ScriptsOf(QStringLiteral("ext"), missing, everywhere, read, &error);
    QVERIFY(scripts.isEmpty());
    QCOMPARE(error, QStringLiteral("Could not load javascript 'nosuch.js' for content script."));
    const ExtensionMainScripts::Reader large = [](const QString &, QByteArray *bytes){
        *bytes = QByteArray(int(ExtensionMainScripts::FILE_LIMIT) + 1, 'x');
        return true;
    };
    QVERIFY(ExtensionMainScripts::ScriptsOf(QStringLiteral("ext"), list, everywhere, large, &error).isEmpty() && !error.isEmpty());
    QJsonArray five;
    for(int i = 0; i < 5; i++)
        five.append(QJsonDocument::fromJson(QStringLiteral("{\"id\": \"s%1\", \"js\": [\"a.js\"], \"matches\": [\"<all_urls>\"]}").arg(i).toUtf8()).object());
    const ExtensionMainScripts::Reader full = [](const QString &, QByteArray *bytes){
        *bytes = QByteArray(int(ExtensionMainScripts::FILE_LIMIT), 'x');
        return true;
    };
    QVERIFY(ExtensionMainScripts::ScriptsOf(QStringLiteral("ext"), ExtensionMainScripts::Parse(QJsonArray() << five, &error),
                                            everywhere, full, &error).isEmpty());
    QCOMPARE(error, QStringLiteral("The content scripts are too large."));
}

void tst_extensionmainscripts::theProfileIsGivenOnlyWhatChanged(){
    auto script = [](const char *name, const char *source){
        ExtensionMainScripts::Script s;
        s.name = QString::fromUtf8(name);
        s.source = QString::fromUtf8(source);
        return s;
    };
    const QList<ExtensionMainScripts::Script> first{ script("e/a/0", "A"), script("e/a/1", "B") };
    const QList<ExtensionMainScripts::Script> second{ script("e/a/0", "A"), script("e/b/0", "C") };
    ExtensionMainScripts::Table table;
    ExtensionMainScripts::Table::Change change = table.Put(QStringLiteral("e"), QStringLiteral("/copy/1"), first);
    QVERIFY(change.removed.isEmpty());
    QCOMPARE(change.inserted, first);
    QVERIFY(table.Put(QStringLiteral("e"), QStringLiteral("/copy/1"), first).isEmpty());
    change = table.Put(QStringLiteral("e"), QStringLiteral("/copy/1"), second);
    QCOMPARE(change.removed, first);
    QCOMPARE(change.inserted, second);
    change = table.Put(QStringLiteral("e"), QStringLiteral("/copy/2"), second);
    QCOMPARE(change.removed, second);
    QCOMPARE(change.inserted, second);
    QCOMPARE(table.Put(QStringLiteral("f"), QStringLiteral("/f"), first).inserted, first);
    QCOMPARE(table.Of(QStringLiteral("e")), second);

    change = table.Keep([](const QString &id, const QString &folder){ return id == QStringLiteral("f") && folder == QStringLiteral("/f"); });
    QCOMPARE(change.removed, second);
    QVERIFY(change.inserted.isEmpty());
    QVERIFY(!table.Has(QStringLiteral("e")) && table.Has(QStringLiteral("f")));
    QCOMPARE(table.Put(QStringLiteral("e"), QStringLiteral("/copy/2"), second).inserted, second);
    change = table.Keep([](const QString &id, const QString &folder){ return !(id == QStringLiteral("e") && folder == QStringLiteral("/copy/2")); });
    QCOMPARE(change.removed, second);

    change = table.Put(QStringLiteral("f"), QStringLiteral("/f"), QList<ExtensionMainScripts::Script>());
    QCOMPARE(change.removed, first);
    QVERIFY(!table.Has(QStringLiteral("f")));
    QVERIFY(table.Put(QStringLiteral("f"), QStringLiteral("/f"), QList<ExtensionMainScripts::Script>()).isEmpty());
}

void tst_extensionmainscripts::whatPersistsIsKeptAsItWasSent(){
    QString error;
    QList<ExtensionMainScripts::Registration> read = ExtensionMainScripts::Parse(ListOf(R"([
        {"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "persistAcrossSessions": false},
        {"id": "b", "js": ["b.js"], "matches": ["<all_urls>"]}])"), &error);
    QVERIFY(error.isEmpty());
    QVERIFY(!read.at(0).persist && read.at(1).persist);
    QVERIFY(ExtensionMainScripts::Parse(ListOf(R"([{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "persistAcrossSessions": "no"}])"), &error).isEmpty()
            && !error.isEmpty());

    ExtensionMainScripts::Store store;
    const QJsonArray sent = ListOf(R"([
        {"id": "c", "js": ["c.js"], "matches": ["<all_urls>"], "persistAcrossSessions": true},
        {"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "persistAcrossSessions": false},
        {"id": "b", "js": ["b.js"], "matches": ["<all_urls>"]}])");
    store.Put(QStringLiteral("x"), QStringLiteral("1.2"), sent);
    QCOMPARE(store.entries.value(QStringLiteral("x")).version, QStringLiteral("1.2"));
    const QJsonArray kept = store.entries.value(QStringLiteral("x")).list;
    QCOMPARE(kept.size(), 2);
    QCOMPARE(kept.at(0).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("c"));
    QCOMPARE(kept.at(1).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("b"));
    QCOMPARE(ExtensionMainScripts::Parse(QJsonArray() << kept, &error).size(), 2);

    store.Put(QStringLiteral("y"), QStringLiteral("3"), ListOf(R"([{"id": "d", "js": ["d.js"], "matches": ["<all_urls>"]}])"));
    const ExtensionMainScripts::Store again = ExtensionMainScripts::Store::FromJson(store.ToJson());
    QCOMPARE(again.entries.size(), 2);
    QVERIFY(again.entries.value(QStringLiteral("x")) == store.entries.value(QStringLiteral("x")));
    QVERIFY(again.entries.value(QStringLiteral("y")) == store.entries.value(QStringLiteral("y")));
    QVERIFY(ExtensionMainScripts::StoreFileName(QStringLiteral("a")) != ExtensionMainScripts::StoreFileName(QStringLiteral("b")));
    QVERIFY(ExtensionMainScripts::StoreFileName(QStringLiteral("a")).startsWith(QStringLiteral("extension-main-scripts-")));

    store.Put(QStringLiteral("x"), QStringLiteral("1.2"), ListOf(R"([{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"], "persistAcrossSessions": false}])"));
    QVERIFY(!store.entries.contains(QStringLiteral("x")));
    store.Drop(QStringLiteral("y"));
    QVERIFY(store.entries.isEmpty());

    const ExtensionMainScripts::Store mixed = ExtensionMainScripts::Store::FromJson(QByteArray(R"({
        "good": {"version": "1", "list": [{"id": "a", "js": ["a.js"], "matches": ["<all_urls>"]}]},
        "noversion": {"list": [{"id": "a"}]},
        "nolist": {"version": "1", "list": {}},
        "empty": {"version": "1", "list": []},
        "notanobject": 3})"));
    QCOMPARE(mixed.entries.keys(), QStringList() << QStringLiteral("good"));
    QVERIFY(ExtensionMainScripts::Store::FromJson("{ broken").entries.isEmpty());
    QVERIFY(ExtensionMainScripts::Store::FromJson("[]").entries.isEmpty());
}

void tst_extensionmainscripts::whatWasKeptIsPutInOnlyForTheSameVersion(){
    using namespace ExtensionMainScripts;
    QCOMPARE(RestoreOf(true, QStringLiteral("1.0"), QStringLiteral("1.0")), PutIn);
    QCOMPARE(RestoreOf(true, QStringLiteral("1.0"), QStringLiteral("1.1")), ThrowAway);
    QCOMPARE(RestoreOf(false, QStringLiteral("1.0"), QStringLiteral("1.0")), LeaveIt);
    QCOMPARE(RestoreOf(false, QStringLiteral("1.0"), QString()), LeaveIt);
}

void tst_extensionmainscripts::whatWasKeptIsReadAgainOutOfTheCopy(){
    using namespace ExtensionMainScripts;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString copy = dir.path();
    auto write = [&copy](const char *name, const QByteArray &bytes){
        QFile f(copy + QLatin1Char('/') + QString::fromLatin1(name));
        return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
    };
    QVERIFY(write("a.js", "A();") && write("b.js", "B();"));
    const QJsonArray sent = ListOf(R"([
        {"id": "a", "js": ["a.js"], "matches": ["<all_urls>"]},
        {"id": "b", "js": ["b.js"], "matches": ["<all_urls>"], "persistAcrossSessions": false}])");

    bool wanted = true;
    QString version = QStringLiteral("1");
    QStringList hosts = QStringList() << QStringLiteral("<all_urls>");
    Now now;
    now.folder = [&copy](const QString &){ return copy; };
    now.wanted = [&wanted](const QString &, const QString &){ return wanted; };
    now.version = [&version](const QString &){ return version; };
    now.hostPermissions = [&hosts](const QString &){ return hosts; };

    Store store;
    store.Put(QStringLiteral("e"), QStringLiteral("1"), sent);
    Table table;
    Restored restored = RestoreInto(table, store, now);
    QVERIFY(!restored.thrown);
    QCOMPARE(restored.change.inserted.size(), 1);
    QVERIFY(restored.change.inserted.at(0).source.endsWith(QStringLiteral("\nA();")));
    QVERIFY(table.Has(QStringLiteral("e")));
    QCOMPARE(table.FolderOf(QStringLiteral("e")), copy);
    QVERIFY(RestoreInto(table, store, now).change.isEmpty());

    const QList<Script> full = ScriptsOf(QStringLiteral("e"), Parse(sent, Ignored()), hosts,
        [&copy](const QString &name, QByteArray *bytes){ return ReadInside(copy, name, bytes); }, Ignored());
    Table::Change change = table.Put(QStringLiteral("e"), copy, full);
    QCOMPARE(change.removed.size(), 1);
    QCOMPARE(change.inserted.size(), 2);
    QVERIFY(table.Put(QStringLiteral("e"), copy, full).isEmpty());

    Table first;
    first.Put(QStringLiteral("e"), copy, full);
    QVERIFY(RestoreInto(first, store, now).change.isEmpty());
    QCOMPARE(first.Of(QStringLiteral("e")), full);

    Table none;
    wanted = false;
    restored = RestoreInto(none, store, now);
    QVERIFY(restored.change.isEmpty() && !restored.thrown && store.entries.contains(QStringLiteral("e")));
    wanted = true;

    version = QStringLiteral("2");
    restored = RestoreInto(none, store, now);
    QVERIFY(restored.change.isEmpty() && restored.thrown && !store.entries.contains(QStringLiteral("e")));
    version = QStringLiteral("1");

    store.Put(QStringLiteral("f"), QStringLiteral("1"), ListOf(R"([{"id": "x", "js": ["gone.js"], "matches": ["<all_urls>"]}])"));
    restored = RestoreInto(none, store, now);
    QVERIFY(restored.thrown && !store.entries.contains(QStringLiteral("f")) && !none.Has(QStringLiteral("f")));
    store.Put(QStringLiteral("g"), QStringLiteral("1"), sent);
    hosts.clear();
    restored = RestoreInto(none, store, now);
    QVERIFY(restored.thrown && !store.entries.contains(QStringLiteral("g")));
}

QTEST_MAIN(tst_extensionmainscripts)
#include "tst_extensionmainscripts.moc"
