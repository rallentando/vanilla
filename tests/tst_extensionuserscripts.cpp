#include "switch.hpp"

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "extensionuserscripts.hpp"

class tst_extensionuserscripts : public QObject {
    Q_OBJECT

private slots:
    void aRegistrationIsReadAsChromeReadsIt();
    void aBookIsChangedWholeOrNotAtAll();
    void anUpdateChangesWhatItGives();
    void aFileIsToBeThereWhenItIsGiven();
    void aBookIsKeptAndReadBack();
    void aBookHasItsLimits();
    void theWorldsAreGivenWholeOrNotAtAll();
    void theScriptsAreEachSourceInItsWorld();
    void aGlobIsMatchedAsChromeMatchesIt_data();
    void aGlobIsMatchedAsChromeMatchesIt();
    void whatIsKeptIsHeldAgainstEachVersion();
    void theWorldsAreConfiguredAndKept();
    void aWorldWhichSendsMessagesOpensWithItsPrelude();

private:
    static QJsonValue Json(const char *text){
        const QJsonDocument document = QJsonDocument::fromJson(QByteArray(text));
        return document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object());
    }
    static ExtensionUserScripts::Reader Copy(){
        return [](const QString &name, QByteArray *bytes){
            if(name == QStringLiteral("a.js")){ *bytes = "A_FILE"; return true; }
            if(name == QStringLiteral("b.js")){ *bytes = "B_FILE"; return true; }
            return false;
        };
    }
    static ExtensionUserScripts::Reader Nothing(){
        return [](const QString &, QByteArray *){ return false; };
    }
    static bool Runs(const ExtensionMainScripts::Registration &where, const QString &url){
        const QString header = ExtensionMainScripts::Header(where, QStringList() << QStringLiteral("<all_urls>"));
        const QString begin = QStringLiteral("// @include /");
        const int from = header.indexOf(begin) + begin.size(), to = header.indexOf(QStringLiteral("/\n// ==/UserScript=="));
        QRegularExpression re(header.mid(from, to - from), QRegularExpression::CaseInsensitiveOption);
        return re.isValid() && re.match(url).hasMatch();
    }
};

void tst_extensionuserscripts::aRegistrationIsReadAsChromeReadsIt(){
    ExtensionUserScripts::Registration r;
    QString error;
    QVERIFY(ExtensionUserScripts::Read(Json("{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"},{\"file\":\"/a.js\"}]}"), &r, &error));
    QCOMPARE(r.id, QStringLiteral("a"));
    QCOMPARE(r.js.size(), 2);
    QCOMPARE(r.js.at(0).code, QStringLiteral("1"));
    QVERIFY(r.js.at(1).isFile);
    QCOMPARE(r.js.at(1).file, QStringLiteral("a.js"));
    QVERIFY(!r.main);
    QVERIFY(r.worldId.isEmpty());
    QCOMPARE(r.runAt, ExtensionMainScripts::DocumentIdle);
    QVERIFY(!r.allFrames);
    QCOMPARE(QJsonDocument(ExtensionUserScripts::Written(r)).toJson(QJsonDocument::Compact),
             QByteArray("{\"allFrames\":false,\"id\":\"a\",\"js\":[{\"code\":\"1\"},{\"file\":\"a.js\"}],\"matches\":[\"*://*/*\"],\"runAt\":\"document_idle\",\"world\":\"USER_SCRIPT\"}"));
    QVERIFY(ExtensionUserScripts::Read(Json("{\"id\":\"w\",\"matches\":[\"https://a.example/*\"],\"excludeMatches\":[\"*://*.b.example/*\"],"
                                            "\"includeGlobs\":[\"*x*\"],\"excludeGlobs\":[\"*y*\"],\"worldId\":\"w1\","
                                            "\"js\":[{\"code\":\"x\"}],\"runAt\":\"document_start\",\"allFrames\":true}"), &r, &error));
    QCOMPARE(r.worldId, QStringLiteral("w1"));
    QCOMPARE(r.includeGlobs, QStringList() << QStringLiteral("*x*"));
    QCOMPARE(r.excludeGlobs, QStringList() << QStringLiteral("*y*"));
    QCOMPARE(ExtensionUserScripts::Written(r).value(QStringLiteral("worldId")).toString(), QStringLiteral("w1"));
    QVERIFY(ExtensionUserScripts::Read(Json("{\"id\":\"m\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"x\"}],\"world\":\"MAIN\",\"worldId\":\"\"}"), &r, &error));
    QVERIFY(r.main);

    const char *refused[][2] = {
        { "{\"id\":\"\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}]}", "Script's ID must not be empty" },
        { "{\"id\":\"_a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}]}", "Script's ID '_a' must not start with '_'" },
        { "{\"id\":\"a\",\"js\":[{\"code\":\"1\"}]}", "Script with ID 'a' must specify 'matches'" },
        { "{\"id\":\"a\",\"matches\":[],\"js\":[{\"code\":\"1\"}]}", "Script with ID 'a' must specify 'matches'" },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[]}", "Script with ID 'a' must specify at least one js source." },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\",\"file\":\"a.js\"}]}",
          "Script with ID 'a' must specify exactly one of 'code' or 'file' in each js source." },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{}]}",
          "Script with ID 'a' must specify exactly one of 'code' or 'file' in each js source." },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"file\":\"../x.js\"}]}", "Could not load javascript '../x.js' for script." },
        { "{\"id\":\"a\",\"matches\":[\"no pattern\"],\"js\":[{\"code\":\"1\"}]}",
          "Script with ID 'a' has invalid value for matches[0]: Invalid match pattern 'no pattern'" },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}],\"worldId\":\"_w\"}", "World IDs beginning with '_' are reserved." },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}],\"world\":\"MAIN\",\"worldId\":\"w\"}",
          "World ID can only be specified for USER_SCRIPT worlds." },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}],\"world\":\"ISOLATED\"}",
          "Error in invocation of userScripts.register: No matching signature." },
        { "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}],\"runAt\":\"now\"}",
          "Error in invocation of userScripts.register: No matching signature." },
        { "{\"id\":1,\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}]}",
          "Error in invocation of userScripts.register: No matching signature." },
    };
    for(const auto &one : refused){
        QVERIFY2(!ExtensionUserScripts::Read(Json(one[0]), &r, &error), one[0]);
        QCOMPARE(error, QString::fromUtf8(one[1]));
    }
    QJsonObject o = Json("{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"1\"}]}").toObject();
    o[QStringLiteral("includeGlobs")] = QJsonArray{ QString(ExtensionMainScripts::GLOB_LIMIT + 1, QLatin1Char('x')) };
    QVERIFY(!ExtensionUserScripts::Read(o, &r, &error));
    QJsonArray many;
    for(int i = 0; i <= ExtensionUserScripts::SOURCES_LIMIT; i++) many.append(QJsonObject{ { QStringLiteral("code"), QStringLiteral("1") } });
    o.remove(QStringLiteral("includeGlobs"));
    o[QStringLiteral("js")] = many;
    QVERIFY(!ExtensionUserScripts::Read(o, &r, &error));
    QCOMPARE(error, QStringLiteral("Script with ID 'a' has too many js sources."));
}

void tst_extensionuserscripts::aBookIsChangedWholeOrNotAtAll(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"A\"}]},"
                               "{\"id\":\"b\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"B\"}],\"world\":\"MAIN\"}]"), Copy(), &error));
    QVERIFY(error.isEmpty());
    QVERIFY(!book.Register(Json("[{\"id\":\"c\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"C\"}]},"
                                "{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"A2\"}]}]"), Copy(), &error));
    QCOMPARE(error, QStringLiteral("Duplicate script ID 'a'"));
    QVERIFY(!book.Register(Json("[{\"id\":\"c\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"C\"}]},"
                                "{\"id\":\"c\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"C\"}]}]"), Copy(), &error));
    QCOMPARE(error, QStringLiteral("Duplicate script ID 'c'"));
    QVERIFY(!book.Register(Json("[{\"id\":\"c\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"C\"}]},{\"id\":\"d\"}]"), Copy(), &error));
    QVERIFY(!book.Register(Json("{\"id\":\"c\"}"), Copy(), &error));
    QCOMPARE(book.List().size(), 2);

    QCOMPARE(book.Get(QJsonValue(), &error).size(), 2);
    const QJsonArray one = book.Get(Json("{\"ids\":[\"b\",\"z\"]}"), &error);
    QCOMPARE(one.size(), 1);
    QCOMPARE(one.at(0).toObject().value(QStringLiteral("world")).toString(), QStringLiteral("MAIN"));
    QVERIFY(book.Get(Json("{\"ids\":\"b\"}"), &error).isEmpty());
    QVERIFY(!error.isEmpty());

    QVERIFY(!book.Unregister(Json("{\"ids\":[\"a\",\"z\"]}"), &error));
    QCOMPARE(error, QStringLiteral("Nonexistent script ID 'z'"));
    QCOMPARE(book.List().size(), 2);
    QVERIFY(book.Unregister(Json("{\"ids\":[\"a\"]}"), &error));
    QCOMPARE(book.List().size(), 1);
    QCOMPARE(book.List().at(0).id, QStringLiteral("b"));
    QVERIFY(book.WorldIds().isEmpty());
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"A\"}]},"
                               "{\"id\":\"c\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"C\"}],\"worldId\":\"w\"},"
                               "{\"id\":\"d\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"D\"}],\"worldId\":\"w\"}]"), Copy(), &error));
    QCOMPARE(book.WorldIds(), QStringList() << QString() << QStringLiteral("w"));
    QVERIFY(book.Unregister(QJsonValue(), &error));
    QVERIFY(book.IsEmpty());
}

void tst_extensionuserscripts::anUpdateChangesWhatItGives(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"excludeMatches\":[\"*://x.example/*\"],"
                               "\"js\":[{\"code\":\"A\"}],\"worldId\":\"w\",\"runAt\":\"document_start\"},"
                               "{\"id\":\"b\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"B\"}],\"world\":\"MAIN\"}]"), Copy(), &error));
    QVERIFY(book.Update(Json("[{\"id\":\"a\",\"excludeMatches\":[\"*://y.example/*\"],\"runAt\":null}]"), Copy(), &error));
    const ExtensionUserScripts::Registration a = book.List().at(0);
    QCOMPARE(a.excludeMatches, QStringList() << QStringLiteral("*://y.example/*"));
    QCOMPARE(a.runAt, ExtensionMainScripts::DocumentStart);
    QCOMPARE(a.worldId, QStringLiteral("w"));
    QCOMPARE(a.js.at(0).code, QStringLiteral("A"));
    QCOMPARE(book.List().at(1).js.at(0).code, QStringLiteral("B"));

    const QJsonArray before = book.Kept();
    const char *refused[][2] = {
        { "[{\"id\":\"b\",\"js\":[{\"code\":\"B2\"}]},{\"id\":\"z\",\"js\":[{\"code\":\"Z\"}]}]", "Nonexistent script ID 'z'" },
        { "[{\"id\":\"a\",\"js\":[{\"code\":\"1\"}]},{\"id\":\"a\",\"js\":[{\"code\":\"2\"}]}]", "Duplicate script ID 'a'" },
        { "[{\"id\":\"a\",\"js\":[]}]", "Script with ID 'a' must specify at least one js source." },
        { "[{\"id\":\"a\",\"matches\":[]}]", "Script with ID 'a' must specify 'matches'" },
        { "[{\"id\":\"a\",\"world\":\"MAIN\",\"worldId\":\"w2\"}]", "World ID can only be specified for USER_SCRIPT worlds." },
        { "[{\"id\":\"b\",\"worldId\":\"w2\"}]", "World ID can only be specified for USER_SCRIPT worlds." },
        { "[{\"id\":\"a\",\"js\":[{\"file\":\"gone.js\"}]}]", "Could not load javascript 'gone.js' for script." },
        { "{\"id\":\"a\"}", "Error in invocation of userScripts.update: No matching signature." },
    };
    for(const auto &one : refused){
        QVERIFY2(!book.Update(Json(one[0]), Copy(), &error), one[0]);
        QCOMPARE(error, QString::fromUtf8(one[1]));
        QCOMPARE(book.Kept(), before);
    }
    QVERIFY(book.Update(Json("[{\"id\":\"a\",\"world\":\"MAIN\"}]"), Copy(), &error));
    QVERIFY(book.List().at(0).main);
    QVERIFY(book.List().at(0).worldId.isEmpty());
    QVERIFY(book.Update(Json("[{\"id\":\"a\",\"world\":\"USER_SCRIPT\",\"worldId\":\"w3\"}]"), Copy(), &error));
    QVERIFY(!book.List().at(0).main);
    QCOMPARE(book.List().at(0).worldId, QStringLiteral("w3"));
}

void tst_extensionuserscripts::aFileIsToBeThereWhenItIsGiven(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(!book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"file\":\"gone.js\"}]}]"), Copy(), &error));
    QCOMPARE(error, QStringLiteral("Could not load javascript 'gone.js' for script."));
    QVERIFY(book.IsEmpty());
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"file\":\"a.js\"},{\"code\":\"C\"}]}]"), Copy(), &error));
    QVERIFY(book.Update(Json("[{\"id\":\"a\",\"allFrames\":true}]"), Nothing(), &error));
    QVERIFY(book.List().at(0).allFrames);
    QVERIFY(!book.Update(Json("[{\"id\":\"a\",\"js\":[{\"file\":\"a.js\"}]}]"), Nothing(), &error));
    QVERIFY(book.Update(Json("[{\"id\":\"a\",\"js\":[{\"file\":\"b.js\"}]}]"), Copy(), &error));
    QCOMPARE(book.List().at(0).js.at(0).file, QStringLiteral("b.js"));
}

void tst_extensionuserscripts::aBookIsKeptAndReadBack(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"excludeMatches\":[\"*://x.example/*\"],\"js\":[{\"code\":\"A\"},{\"file\":\"a.js\"}],"
                               "\"runAt\":\"document_start\",\"allFrames\":true,\"worldId\":\"w\",\"includeGlobs\":[\"*a*\"],\"excludeGlobs\":[\"*b*\"]},"
                               "{\"id\":\"b\",\"matches\":[\"https://b.example/*\"],\"js\":[{\"code\":\"B1\"},{\"code\":\"B2\"}],\"world\":\"MAIN\"}]"), Copy(), &error));
    const QJsonArray kept = book.Kept();
    const ExtensionUserScripts::Book again = ExtensionUserScripts::Book::Of(kept);
    QCOMPARE(again.Kept(), kept);
    QCOMPARE(again.List().at(0).js.at(1).file, QStringLiteral("a.js"));
    QCOMPARE(again.List().at(0).includeGlobs, QStringList() << QStringLiteral("*a*"));
    QJsonArray broken = kept;
    broken.append(Json("{\"id\":\"c\"}"));
    broken.append(kept.at(0));
    QCOMPARE(ExtensionUserScripts::Book::Of(broken).Kept(), kept);
    QVERIFY(ExtensionUserScripts::StoreFileName(QStringLiteral("k")) != ExtensionMainScripts::StoreFileName(QStringLiteral("k")));
    QVERIFY(ExtensionUserScripts::StoreFileName(QStringLiteral("k")) != ExtensionUserScripts::StoreFileName(QStringLiteral("l")));
}

void tst_extensionuserscripts::aBookHasItsLimits(){
    ExtensionUserScripts::Book book;
    QString error;
    QJsonArray many;
    for(int i = 0; i <= ExtensionUserScripts::SCRIPTS_LIMIT; i++){
        QJsonObject o;
        o[QStringLiteral("id")] = QString::number(i);
        o[QStringLiteral("matches")] = QJsonArray{ QStringLiteral("*://*/*") };
        o[QStringLiteral("js")] = QJsonArray{ QJsonObject{ { QStringLiteral("code"), QStringLiteral("1") } } };
        many.append(o);
    }
    QVERIFY(!book.Register(many, Copy(), &error));
    QCOMPARE(error, QStringLiteral("Too many user scripts."));
    many.removeLast();
    QVERIFY(book.Register(many, Copy(), &error));
    QVERIFY(book.Unregister(QJsonValue(), &error));
    const QString big(ExtensionUserScripts::TOTAL_LIMIT / 2 + 1, QLatin1Char('x'));
    QJsonObject o;
    o[QStringLiteral("id")] = QStringLiteral("big");
    o[QStringLiteral("matches")] = QJsonArray{ QStringLiteral("*://*/*") };
    o[QStringLiteral("js")] = QJsonArray{ QJsonObject{ { QStringLiteral("code"), big } } };
    QVERIFY(book.Register(QJsonArray{ o }, Copy(), &error));
    o[QStringLiteral("id")] = QStringLiteral("bigger");
    QVERIFY(!book.Register(QJsonArray{ o }, Copy(), &error));
    QCOMPARE(error, QStringLiteral("The user scripts are too large."));
    QVERIFY(!book.Update(Json("[{\"id\":\"big\",\"runAt\":\"now\"}]"), Copy(), &error));
    QCOMPARE(error, QStringLiteral("Error in invocation of userScripts.update: No matching signature."));
    o[QStringLiteral("id")] = QStringLiteral("small");
    o[QStringLiteral("js")] = QJsonArray{ QJsonObject{ { QStringLiteral("code"), QStringLiteral("1") } } };
    QVERIFY(book.Register(QJsonArray{ o }, Copy(), &error));
    o[QStringLiteral("js")] = QJsonArray{ QJsonObject{ { QStringLiteral("code"), big } } };
    QVERIFY(!book.Update(QJsonArray{ o }, Copy(), &error));
    QCOMPARE(error, QStringLiteral("The user scripts are too large."));
    QCOMPARE(book.List().size(), 2);
}

void tst_extensionuserscripts::theWorldsAreGivenWholeOrNotAtAll(){
    ExtensionUserScripts::Worlds worlds;
    QVERIFY(worlds.Take(QStringLiteral("e"), QStringList() << QString() << QStringLiteral("w")));
    QCOMPARE(worlds.Of(QStringLiteral("e"), QString()), ExtensionUserScripts::FIRST_WORLD);
    QCOMPARE(worlds.Of(QStringLiteral("e"), QStringLiteral("w")), ExtensionUserScripts::FIRST_WORLD + 1);
    QVERIFY(worlds.Take(QStringLiteral("f"), QStringList() << QStringLiteral("w")));
    QCOMPARE(worlds.Of(QStringLiteral("f"), QStringLiteral("w")), ExtensionUserScripts::FIRST_WORLD + 2);
    QVERIFY(worlds.Take(QStringLiteral("e"), QStringList() << QStringLiteral("w")));
    QCOMPARE(worlds.Of(QStringLiteral("e"), QStringLiteral("x")), quint32(0));
    QStringList many;
    for(int i = 0; i < ExtensionUserScripts::EXTENSION_WORLDS - 1; i++) many << QString::number(i);
    QVERIFY(!worlds.Take(QStringLiteral("e"), many));
    QCOMPARE(worlds.Of(QStringLiteral("e"), QStringLiteral("0")), quint32(0));
    many.removeLast();
    QVERIFY(worlds.Take(QStringLiteral("e"), many));
    QVERIFY(!worlds.Take(QStringLiteral("e"), QStringList() << QStringLiteral("one more")));
    int extension = 0;
    while(worlds.Take(QStringLiteral("x%1").arg(extension), QStringList() << QStringLiteral("a") << QStringLiteral("b"))) extension++;
    QVERIFY(worlds.Of(QStringLiteral("x%1").arg(extension - 1), QStringLiteral("b")) <= ExtensionUserScripts::LAST_WORLD);
    QCOMPARE(worlds.Of(QStringLiteral("x%1").arg(extension), QStringLiteral("a")), quint32(0));
}

void tst_extensionuserscripts::theScriptsAreEachSourceInItsWorld(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"A1\"},{\"file\":\"a.js\"}],\"runAt\":\"document_start\",\"allFrames\":true},"
                               "{\"id\":\"w\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"W\"}],\"worldId\":\"other\"},"
                               "{\"id\":\"b\",\"matches\":[\"https://b.example/*\"],\"js\":[{\"code\":\"B\"}],\"world\":\"MAIN\"}]"), Copy(), &error));
    ExtensionUserScripts::Worlds worlds;
    QVERIFY(worlds.Take(QStringLiteral("ext"), book.WorldIds()));
    const QStringList all = QStringList() << QStringLiteral("<all_urls>");
    QStringList skipped;
    const QList<ExtensionMainScripts::Script> scripts = ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, all, worlds, Copy(), &skipped);
    QVERIFY(skipped.isEmpty());
    QCOMPARE(scripts.size(), 4);
    QCOMPARE(scripts.at(0).name, QStringLiteral("vanilla-us/ext/a/0"));
    QCOMPARE(scripts.at(1).name, QStringLiteral("vanilla-us/ext/a/1"));
    QCOMPARE(scripts.at(2).name, QStringLiteral("vanilla-us/ext/w/0"));
    QCOMPARE(scripts.at(3).name, QStringLiteral("vanilla-us/ext/b/0"));
    QCOMPARE(scripts.at(0).world, worlds.Of(QStringLiteral("ext"), QString()));
    QCOMPARE(scripts.at(1).world, worlds.Of(QStringLiteral("ext"), QString()));
    QCOMPARE(scripts.at(2).world, worlds.Of(QStringLiteral("ext"), QStringLiteral("other")));
    QVERIFY(scripts.at(0).world != scripts.at(2).world);
    QCOMPARE(scripts.at(3).world, quint32(0));
    QCOMPARE(scripts.at(0).runAt, ExtensionMainScripts::DocumentStart);
    QVERIFY(scripts.at(0).subFrames);
    ExtensionMainScripts::Registration where;
    where.id = QStringLiteral("b");
    where.matches = QStringList() << QStringLiteral("https://b.example/*");
    QCOMPARE(scripts.at(3).source, ExtensionMainScripts::Header(where, all) + QStringLiteral("B"));
    QVERIFY(scripts.at(0).source.endsWith(QStringLiteral("\nA1")));
    QVERIFY(scripts.at(1).source.endsWith(QStringLiteral("\nA_FILE")));

    QVERIFY(ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, QStringList(), worlds, Copy(), &skipped).isEmpty());
    skipped.clear();
    QList<ExtensionMainScripts::Script> without = ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, all, worlds, Nothing(), &skipped);
    QCOMPARE(without.size(), 2);
    QCOMPARE(without.at(0).name, QStringLiteral("vanilla-us/ext/w/0"));
    QCOMPARE(skipped, QStringList() << QStringLiteral("'a' could not read 'a.js'"));
    skipped.clear();
    without = ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, all, ExtensionUserScripts::Worlds(), Copy(), &skipped);
    QCOMPARE(without.size(), 1);
    QCOMPARE(without.at(0).name, QStringLiteral("vanilla-us/ext/b/0"));
    QCOMPARE(skipped.size(), 2);
    const ExtensionUserScripts::Reader huge = [](const QString &, QByteArray *bytes){
        *bytes = QByteArray(ExtensionMainScripts::FILE_LIMIT, 'x');
        return true;
    };
    ExtensionUserScripts::Book files;
    QJsonArray list;
    for(int i = 0; i < 5; i++){
        QJsonObject o;
        o[QStringLiteral("id")] = QString::number(i);
        o[QStringLiteral("matches")] = QJsonArray{ QStringLiteral("*://*/*") };
        o[QStringLiteral("world")] = QStringLiteral("MAIN");
        o[QStringLiteral("js")] = QJsonArray{ QJsonObject{ { QStringLiteral("file"), QStringLiteral("a.js") } } };
        list.append(o);
    }
    int reads = 0;
    const ExtensionUserScripts::Reader counted = [&reads](const QString &, QByteArray *bytes){
        reads++;
        *bytes = QByteArray(ExtensionMainScripts::FILE_LIMIT, 'x');
        return true;
    };
    QVERIFY(files.Register(list, counted, &error));
    QCOMPARE(reads, 1);
    skipped.clear();
    reads = 0;
    QCOMPARE(ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), files, all, worlds, counted, &skipped).size(), 4);
    QCOMPARE(reads, 1);
    QCOMPARE(skipped, QStringList() << QStringLiteral("'4' is past what may be put in"));
    ExtensionUserScripts::Book long_;
    QJsonObject o;
    o[QStringLiteral("id")] = QStringLiteral("long");
    o[QStringLiteral("matches")] = QJsonArray{ QStringLiteral("*://*/*") };
    o[QStringLiteral("world")] = QStringLiteral("MAIN");
    QJsonArray five;
    for(int i = 0; i < 5; i++) five.append(QJsonObject{ { QStringLiteral("file"), QStringLiteral("a.js") } });
    o[QStringLiteral("js")] = five;
    QJsonObject small = o;
    small[QStringLiteral("id")] = QStringLiteral("small");
    small[QStringLiteral("js")] = QJsonArray{ QJsonObject{ { QStringLiteral("code"), QStringLiteral("S") } } };
    QVERIFY(long_.Register(QJsonArray{ o, small }, huge, &error));
    skipped.clear();
    const QList<ExtensionMainScripts::Script> made = ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), long_, all, worlds, huge, &skipped);
    QCOMPARE(made.size(), 1);
    QCOMPARE(made.at(0).name, QStringLiteral("vanilla-us/ext/small/0"));
    QCOMPARE(skipped, QStringList() << QStringLiteral("'long' is past what may be put in"));
}

void tst_extensionuserscripts::aGlobIsMatchedAsChromeMatchesIt_data(){
    QTest::addColumn<QString>("include");
    QTest::addColumn<QString>("exclude");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("runs");
    QTest::newRow("none") << QString() << QString() << QStringLiteral("https://a.example/x") << true;
    QTest::newRow("star") << QStringLiteral("*a.example/x*") << QString() << QStringLiteral("https://a.example/xyz") << true;
    QTest::newRow("star miss") << QStringLiteral("*a.example/x*") << QString() << QStringLiteral("https://a.example/y") << false;
    QTest::newRow("whole address") << QStringLiteral("a.example/x*") << QString() << QStringLiteral("https://a.example/x") << false;
    QTest::newRow("case matters") << QStringLiteral("*/X*") << QString() << QStringLiteral("https://a.example/x") << false;
    QTest::newRow("question one") << QStringLiteral("*/a?c") << QString() << QStringLiteral("https://a.example/abc") << true;
    QTest::newRow("question none") << QStringLiteral("*/a?c") << QString() << QStringLiteral("https://a.example/ac") << true;
    QTest::newRow("question not two") << QStringLiteral("*/a?c") << QString() << QStringLiteral("https://a.example/abbc") << false;
    QTest::newRow("escaped star") << QStringLiteral("*/a\\*") << QString() << QStringLiteral("https://a.example/a*") << true;
    QTest::newRow("escaped star is no star") << QStringLiteral("*/a\\*") << QString() << QStringLiteral("https://a.example/ab") << false;
    QTest::newRow("dot is a dot") << QStringLiteral("*/a.c") << QString() << QStringLiteral("https://a.example/abc") << false;
    QTest::newRow("exclude") << QString() << QStringLiteral("*secret*") << QStringLiteral("https://a.example/secret/1") << false;
    QTest::newRow("exclude miss") << QString() << QStringLiteral("*secret*") << QStringLiteral("https://a.example/open") << true;
    QTest::newRow("a space") << QStringLiteral("*a b*") << QString() << QStringLiteral("https://a.example/a b") << true;
    QTest::newRow("past ascii") << QStringLiteral("*/\u00e9*") << QString() << QStringLiteral("https://a.example/\u00e9") << true;
    QTest::newRow("a pair") << QStringLiteral("*/\U0001F600*") << QString() << QStringLiteral("https://a.example/\U0001F600") << true;
}

void tst_extensionuserscripts::aGlobIsMatchedAsChromeMatchesIt(){
    QFETCH(QString, include);
    QFETCH(QString, exclude);
    QFETCH(QString, url);
    QFETCH(bool, runs);
    ExtensionMainScripts::Registration where;
    where.id = QStringLiteral("g");
    where.matches = QStringList() << QStringLiteral("<all_urls>");
    if(!include.isEmpty()) where.includeGlobs << include;
    if(!exclude.isEmpty()) where.excludeGlobs << exclude;
    QCOMPARE(Runs(where, url), runs);
    where.includeGlobs = QStringList() << QStringLiteral("*\n// @include *\r\x0b\x7f") + QChar(0x2028) + QStringLiteral("*");
    const QString header = ExtensionMainScripts::Header(where, QStringList() << QStringLiteral("<all_urls>"));
    QCOMPARE(header.count(QLatin1Char('\n')), 3);
    QVERIFY(!header.contains(QLatin1Char('\r')));
    QVERIFY(!header.contains(QChar(0x2028)));
    const QString pair = ExtensionMainScripts::GlobRegex(QStringLiteral("\U0001F600\u00e9"));
    QCOMPARE(pair, QStringLiteral("(?:\\x{1f600}\\x{e9})"));
}

void tst_extensionuserscripts::whatIsKeptIsHeldAgainstEachVersion(){
    QHash<QString, QString> checked;
    QVERIFY(!ExtensionUserScripts::ThrowKept(&checked, QStringLiteral("e"), QStringLiteral("1"), true, QStringLiteral("1")));
    QVERIFY(!ExtensionUserScripts::ThrowKept(&checked, QStringLiteral("e"), QStringLiteral("1"), true, QStringLiteral("0")));
    QVERIFY(ExtensionUserScripts::ThrowKept(&checked, QStringLiteral("e"), QStringLiteral("2"), true, QStringLiteral("1")));
    QVERIFY(!ExtensionUserScripts::ThrowKept(&checked, QStringLiteral("e"), QStringLiteral("2"), true, QStringLiteral("1")));
    QVERIFY(ExtensionUserScripts::ThrowKept(&checked, QStringLiteral("f"), QStringLiteral("3"), true, QStringLiteral("2")));
    QVERIFY(!ExtensionUserScripts::ThrowKept(&checked, QStringLiteral("g"), QStringLiteral("3"), false, QString()));
}

void tst_extensionuserscripts::theWorldsAreConfiguredAndKept(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(book.IsEmpty());
    QVERIFY(book.Configure(Json("{\"messaging\":true}"), &error));
    QVERIFY(!book.IsEmpty());
    QVERIFY(book.Configure(Json("{\"worldId\":\"w\",\"csp\":\"script-src 'self'\"}"), &error));
    QVERIFY(book.Configure(Json("{\"worldId\":\"w\",\"messaging\":true,\"csp\":\"x\"}"), &error));
    QCOMPARE(QJsonDocument(book.Worlds()).toJson(QJsonDocument::Compact),
             QByteArray("[{\"messaging\":true},{\"csp\":\"x\",\"messaging\":true,\"worldId\":\"w\"}]"));
    QVERIFY(book.Messaging(QString()));
    QVERIFY(book.Messaging(QStringLiteral("w")));
    QVERIFY(book.Messaging(QStringLiteral("v")));
    QVERIFY(book.Configure(Json("{\"worldId\":\"v\",\"messaging\":false}"), &error));
    QVERIFY(!book.Messaging(QStringLiteral("v")));
    QVERIFY(book.Reset(QJsonValue(QStringLiteral("v")), &error));
    QVERIFY(book.Messaging(QStringLiteral("v")));
    QVERIFY(!book.Configure(Json("{\"worldId\":\"_w\"}"), &error));
    QCOMPARE(error, QStringLiteral("World IDs beginning with '_' are reserved."));
    QVERIFY(!book.Configure(Json("{\"messaging\":\"yes\"}"), &error));
    QVERIFY(!book.Configure(Json("[1]"), &error));
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"A\"}]}]"), Copy(), &error));
    const QJsonArray kept = book.Kept();
    QCOMPARE(kept.size(), 2);
    QCOMPARE(ExtensionUserScripts::Book::Of(kept).Kept(), kept);
    ExtensionUserScripts::Registration r;
    QVERIFY(!ExtensionUserScripts::Read(kept.at(1), &r, &error));
    QVERIFY(book.Reset(Json("{\"x\":1}").toObject().value(QStringLiteral("none")), &error));
    QVERIFY(!book.Messaging(QString()));
    QVERIFY(book.Reset(QJsonValue(QStringLiteral("nowhere")), &error));
    QVERIFY(book.Reset(QJsonValue(QStringLiteral("w")), &error));
    QVERIFY(book.Worlds().isEmpty());
    QVERIFY(!book.Reset(QJsonValue(3), &error));
    for(int i = 0; i < ExtensionUserScripts::EXTENSION_WORLDS; i++)
        QVERIFY(book.Configure(QJsonObject{ { QStringLiteral("worldId"), QString::number(i) } }, &error));
    QVERIFY(!book.Configure(QJsonObject{ { QStringLiteral("worldId"), QStringLiteral("one more") } }, &error));
}

void tst_extensionuserscripts::aWorldWhichSendsMessagesOpensWithItsPrelude(){
    ExtensionUserScripts::Book book;
    QString error;
    QVERIFY(book.Register(Json("[{\"id\":\"a\",\"matches\":[\"https://a.example/*\"],\"js\":[{\"code\":\"A\"}]},"
                               "{\"id\":\"w\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"W\"}],\"worldId\":\"quiet\"},"
                               "{\"id\":\"m\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"M\"}],\"world\":\"MAIN\"}]"), Copy(), &error));
    QVERIFY(book.Configure(Json("{\"messaging\":true}"), &error));
    QVERIFY(book.Configure(Json("{\"worldId\":\"loud\",\"messaging\":true}"), &error));
    QVERIFY(book.Configure(Json("{\"worldId\":\"quiet\",\"messaging\":false}"), &error));
    ExtensionUserScripts::Worlds worlds;
    QVERIFY(worlds.Take(QStringLiteral("ext"), book.WorldIds()));
    const QStringList hosts = QStringList() << QStringLiteral("https://*.example/*");
    QStringList asked;
    const ExtensionUserScripts::Prelude prelude = [&asked](const QString &worldId){ asked << worldId; return QStringLiteral("PRELUDE(%1)").arg(worldId); };
    const QList<ExtensionMainScripts::Script> scripts = ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, hosts, worlds, Copy(), nullptr, prelude);
    QCOMPARE(asked, QStringList() << QString());
    QCOMPARE(scripts.size(), 4);
    QCOMPARE(scripts.at(0).name, QStringLiteral("vanilla-us/ext/_world/0"));
    QCOMPARE(scripts.at(0).world, worlds.Of(QStringLiteral("ext"), QString()));
    QCOMPARE(scripts.at(0).runAt, ExtensionMainScripts::DocumentStart);
    QVERIFY(scripts.at(0).subFrames);
    ExtensionMainScripts::Registration everywhere;
    everywhere.id = QStringLiteral("_world");
    everywhere.matches = QStringList() << QStringLiteral("<all_urls>");
    QCOMPARE(scripts.at(0).source, ExtensionMainScripts::Header(everywhere, hosts) + QStringLiteral("PRELUDE()"));
    QCOMPARE(scripts.at(1).name, QStringLiteral("vanilla-us/ext/a/0"));
    QCOMPARE(ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, hosts, worlds, Copy(), nullptr,
                                             [](const QString &){ return QString(); }).size(), 3);
    QCOMPARE(ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, hosts, worlds, Copy(), nullptr).size(), 3);
    QVERIFY(book.Reset(QJsonValue(QStringLiteral("quiet")), &error));
    asked.clear();
    QCOMPARE(ExtensionUserScripts::ScriptsOf(QStringLiteral("ext"), book, hosts, worlds, Copy(), nullptr, prelude).size(), 5);
    QCOMPARE(asked, QStringList() << QString() << QStringLiteral("quiet"));
}

QTEST_MAIN(tst_extensionuserscripts)
#include "tst_extensionuserscripts.moc"
