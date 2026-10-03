#include <QtTest>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include "extensioncontroller.hpp"
#include "extensioncopy.hpp"
#include "application.hpp"
#include "view.hpp"
#include <deque>

namespace {
class HistoryView : public QObject, public View {
public:
    explicit HistoryView(ExtensionController *controller) : View(nullptr), controller(controller) {}
    QObject *base() override { return this; }
    QSize size() override { return {}; }
    void resize(QSize) override {}
    void show() override {}
    void hide() override {}
    void raise() override {}
    void lower() override {}
    void repaint() override {}
    bool visible() override { return false; }
    void setFocus(Qt::FocusReason = Qt::OtherFocusReason) override {}
    ExtensionController *Extensions() const override { return controller; }
    bool RestoreHistory() override { ++restores; return valid; }
    void Load(const QNetworkRequest &value) override { ++loads; request = value; }
    ExtensionController *controller;
    QNetworkRequest request;
    bool valid = true;
    int restores = 0, loads = 0;
};

struct Operation {
    QString kind;
    std::function<void()> effect, complete;
};
struct Engine {
    QMap<QString, ExtensionItem> items;
    std::deque<Operation> pending;
    QStringList calls;
    bool addEnabled = true;
    QString error;
    QString returnedId;
    bool workerOfThisRun = true;
    bool handlesCommands = true;
};

class Deferred : public ExtensionController {
public:
    Deferred(std::shared_ptr<Engine> engine, const QString &journal = {}, int timeoutMs = 20000)
        : ExtensionController(nullptr, journal, timeoutMs), engine(engine) {}
    std::shared_ptr<Engine> engine;
protected:
    void Snapshot(SnapshotDone done) override {
        auto state = engine;
        Queue({"snapshot", [] {}, [state, done] { done(state->items.values(), state->error); }});
    }
    void Add(const ExtensionManifest &m, ItemDone done) override {
        auto state = engine;
        const QString id = state->returnedId.isEmpty() ? m.id : state->returnedId;
        Queue({"add", [state, m, id] {
            if (state->error.isEmpty()) state->items.insert(id, {id, m.name, {}, state->addEnabled, m.popup});
        }, [state, done, id] { done(state->items.value(id), state->error); }});
    }
    void Enable(const QString &id, bool enabled, ItemDone done) override {
        auto state = engine;
        Queue({enabled ? "enable" : "disable", [state, id, enabled] {
            if (state->error.isEmpty() && state->items.contains(id)) state->items[id].enabled = enabled;
        }, [state, id, done] {
            if (!state->items.contains(id)) {
                ExtensionItem gone; gone.gone = true;
                done(gone, QStringLiteral("gone"));
                return;
            }
            done(state->items.value(id), state->error);
        }});
    }
    void Remove(const QString &id, Done done) override {
        auto state = engine;
        Queue({"remove", [state, id] {
            if (state->error.isEmpty()) state->items.remove(id);
        }, [state, done] { done(state->error); }});
    }
    void Queue(Operation op) {
        engine->calls.append(op.kind);
        engine->pending.push_back(std::move(op));
    }
    bool WorkerIsOfThisRun(const QString &) const override { return engine->workerOfThisRun; }
    bool HandlesCommands() const override { return engine->handlesCommands; }
};

QString Fixture() { return QString::fromUtf8(VANILLA_FIXTURE_DIR); }
void Desired(bool registered = true, bool enabled = true) {
    auto &s = Application::GlobalSettings();
    s.setValue(QStringLiteral("network/@Extensions"), registered ? QStringList{Fixture()} : QStringList{});
    s.setValue(QStringLiteral("network/@DisabledExtensions"), enabled ? QStringList{} : QStringList{Fixture()});
}
QString Next(const std::shared_ptr<Engine> &engine) {
    return engine->pending.empty() ? QString() : engine->pending.front().kind;
}
Operation Take(const std::shared_ptr<Engine> &engine) {
    Operation op = std::move(engine->pending.front()); engine->pending.pop_front(); return op;
}
void Finish(const std::shared_ptr<Engine> &engine) { auto op = Take(engine); op.effect(); op.complete(); }
QJsonObject Owned(const QString &journal) {
    QFile f(journal); if (!f.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("owned")).toObject();
}
void Write(const QString &path, const QByteArray &data) {
    QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); QCOMPARE(f.write(data), data.size());
}
}

class ExtensionsTest : public QObject {
    Q_OBJECT
private slots:
    void init() { Application::GlobalSettings().clear(); }
    void theManifestsNameIsReadThroughTheMessagesFiles() {
        QTemporaryDir dir;
        const QString folder = dir.filePath("ext");
        QString language = ExtensionMessages::UiLocale().section(QLatin1Char('_'), 0, 0);
        if (language.isEmpty()) language = QStringLiteral("C");
        const QString fallback = language == QStringLiteral("en") ? QStringLiteral("de") : QStringLiteral("en");
        QVERIFY(QDir().mkpath(folder + "/_locales/" + fallback));
        QVERIFY(QDir().mkpath(folder + "/_locales/" + language));
        Write(folder + "/_locales/" + fallback + "/messages.json",
              R"({"Probe": {"message": "Default"}, "only_default": {"message": "Only $WHAT$", "placeholders": {"what": {"content": "one"}}},)"
              R"("a": {"message": "A!"}, "b": {"message": "B!"}})");
        Write(folder + "/_locales/" + language + "/messages.json", R"({"probe": {"message": "Preferred"}})");
        Write(folder + "/manifest.json",
              R"({"manifest_version":3,"name":"A __MSG_probe__ B __MSG_only_default__ __MSG_nosuch__ __MSG_a____MSG_b__ Z","version":"1","key":"YWJj",)"
              R"("default_locale":")" + fallback.toUtf8() + R"(","action":{"default_title":"__MSG_PROBE__"}})");
        const auto m = ExtensionManifest::Read(folder);
        QVERIFY2(m.error.isEmpty(), qPrintable(m.error));
        QCOMPARE(m.name, QStringLiteral("A Preferred B Only one __MSG_nosuch__ A!B! Z"));
        QCOMPARE(m.title, QStringLiteral("Preferred"));
        Write(folder + "/manifest.json", R"({"manifest_version":3,"name":"__MSG_probe__","version":"1","key":"YWJj"})");
        QCOMPARE(ExtensionManifest::Read(folder).name, QStringLiteral("__MSG_probe__"));
    }
    void theHomePageIsTheManifestsOrTheStoresPage() {
        QTemporaryDir dir;
        const QString folder = dir.filePath("ext");
        QVERIFY(QDir().mkpath(folder));
        const QByteArray https = "https:" "//", http = "http:" "//";
        const QByteArray key = ",\"key\":\"YWJj\"";
        auto homepage = [](const QByteArray &url){ return ",\"homepage_url\":\"" + url + "\""; };
        auto update = [](const QByteArray &url){ return ",\"update_url\":\"" + url + "\""; };
        const QByteArray chrome = update(https + "clients2.google.com/service/update2/crx");
        const QByteArray edge = update(https + "edge.microsoft.com/extensionwebstorebase/v1/crx");
        auto U = [](const QByteArray &url){ return QUrl(QString::fromLatin1(url)); };
        auto read = [&](const QByteArray &rest){
            Write(folder + "/manifest.json", "{\"manifest_version\":3,\"name\":\"home\",\"version\":\"1\"" + rest + "}");
            const auto m = ExtensionManifest::Read(folder);
            return std::make_pair(m.homepage, m.id);
        };
        QCOMPARE(read(homepage(https + "example.com/x")).first, U(https + "example.com/x"));
        QCOMPARE(read(homepage(http + "example.com/")).first, U(http + "example.com/"));
        QCOMPARE(read(homepage("javascript:alert(1)")).first, QUrl());
        QCOMPARE(read(homepage("file:" "///C:/x")).first, QUrl());
        QCOMPARE(read(homepage("ftp:" "//example.com/")).first, QUrl());
        QCOMPARE(read(homepage("example.com")).first, QUrl());
        QCOMPARE(read("").first, QUrl());
        auto keyed = read(key + chrome);
        QCOMPARE(keyed.first, U(https + "chromewebstore.google.com/detail/" + keyed.second.toLatin1()));
        keyed = read(key + edge);
        QCOMPARE(keyed.first, U(https + "microsoftedge.microsoft.com/addons/detail/" + keyed.second.toLatin1()));
        QCOMPARE(read(key + homepage(https + "example.com/") + chrome).first, U(https + "example.com/"));
        QCOMPARE(read(chrome).first, QUrl());
        QCOMPARE(read(key + update(https + "example.com/service/update2/crx")).first, QUrl());
        QCOMPARE(read(key + update(https + "clients2.google.com/other")).first, QUrl());
        QCOMPARE(read(key + update(http + "clients2.google.com/service/update2/crx")).first, QUrl());
    }
    void ruleFilesFollowTheTwoListsAndTheManifest() {
        QTemporaryDir dir;
        const QString on = dir.filePath("on"), off = dir.filePath("off");
        const QByteArray manifest = R"({"manifest_version":3,"name":"rules","version":"1",
            "declarative_net_request":{"rule_resources":[
              {"id":"a","enabled":true,"path":"rules/a.json"},
              {"id":"b","enabled":false,"path":"rules/b.json"},
              {"id":"c","enabled":true,"path":"../outside.json"},
              {"id":"d","enabled":true,"path":"rules/missing.json"},
              {"id":"e","enabled":true,"path":"/rules/e.json"},
              {"id":"f","enabled":true,"path":"//rules/e.json"}]}})";
        foreach(const QString &root, QStringList() << on << off){
            QVERIFY(QDir().mkpath(root + "/rules"));
            Write(root + "/manifest.json", manifest);
            Write(root + "/rules/a.json", "[]");
            Write(root + "/rules/b.json", "[]");
            Write(root + "/rules/e.json", "[]");
        }
        Write(dir.filePath("outside.json"), "[]");

        Settings &s = Application::GlobalSettings();
        s.setValue("network/@Extensions", QStringList{on, off});
        s.setValue("network/@DisabledExtensions", QStringList{off});

        const QList<ExtensionRuleFiles> files = ExtensionController::EnabledRuleFiles();
        QCOMPARE(files.size(), 1);
        QCOMPARE(files.first().id, ExtensionManifest::Read(on).id);
        QCOMPARE(files.first().files.size(), 2);
        QVERIFY(files.first().files.first().endsWith("/rules/a.json"));
        QVERIFY(files.first().files.last().endsWith("/rules/e.json"));

        s.setValue("network/@DisabledExtensions", QStringList{off, on});
        QVERIFY(ExtensionController::EnabledRuleFiles().isEmpty());
    }
    void onStartupIsOwedOncePerRunToWhatRanAtTheStart() {
        QTemporaryDir dir;
        const QString ext = dir.filePath("ext"), other = dir.filePath("other"), off = dir.filePath("off");
        auto copy = [&](const QString &to, const QString &version){
            QDir(to).removeRecursively();
            QVERIFY(QDir().mkpath(to + "/_locales/en"));
            foreach(const QFileInfo &f, QDir(Fixture()).entryInfoList(QDir::Files))
                QVERIFY(QFile::copy(f.absoluteFilePath(), to + "/" + f.fileName()));
            QVERIFY(QFile::copy(Fixture() + "/_locales/en/messages.json", to + "/_locales/en/messages.json"));
            QFile m(to + "/manifest.json"); QVERIFY(m.open(QIODevice::ReadOnly));
            QJsonObject manifest = QJsonDocument::fromJson(m.readAll()).object(); m.close();
            manifest["version"] = version;
            QFile::remove(to + "/manifest.json");
            Write(to + "/manifest.json", QJsonDocument(manifest).toJson());
        };
        copy(ext, "1.0"); copy(other, "2.0"); copy(off, "3.0");
        const QString ledger = dir.filePath("installed.json");
        const QString id = ExtensionManifest::Read(ext).id, second = ExtensionManifest::Read(other).id, third = ExtensionManifest::Read(off).id;
        QVERIFY(!id.isEmpty() && id != second && second != third && id != third);
        Settings &s = Application::GlobalSettings();
        s.setValue("network/@Extensions", QStringList{ext, off});
        s.setValue("network/@DisabledExtensions", QStringList{off});

        auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
        e->workerOfThisRun = false;
        QVERIFY(!c.TakeStartup(id));
        e->workerOfThisRun = true;
        QCOMPARE(c.TakeInstalled(id, nullptr).value("reason").toString(), QStringLiteral("update"));
        QVERIFY(c.TakeStartup(id));
        QVERIFY(!c.TakeStartup(id));
        QVERIFY(!c.TakeStartup(third));
        c.Reconcile({ext, off}, {}, {});
        QVERIFY(!c.TakeStartup(third));
        c.Reconcile({ext, off, other}, {}, {});
        QVERIFY(!c.TakeStartup(second));
        QVERIFY(!c.TakeStartup(QStringLiteral("abcdefghijklmnopabcdefghijklmnop")));

        s.setValue("network/@Extensions", QStringList{ext, off, other});
        s.setValue("network/@DisabledExtensions", QStringList{});
        {
            auto e2 = std::make_shared<Engine>(); Deferred c2(e2); c2.SetInstalledFile(ledger); c2.Start();
            for(int i = 0; i < 200 && (c2.IsBusy() || !e2->pending.empty() || !c2.IsReady()); i++){
                if(!e2->pending.empty()) Finish(e2); else QTest::qWait(1);
            }
            QVERIFY(!c2.IsBusy() && c2.IsReady());
            copy(other, "2.1");
            c2.Retry(other);
            QCOMPARE(c2.Rows().last().manifest.version, QStringLiteral("2.1"));
            QVERIFY(!c2.TakeStartup(second));
            c2.Reconcile({ext, off, other}, {ext}, {});
            c2.Reconcile({ext, off, other}, {}, {});
            QVERIFY(!c2.TakeStartup(id));
            QVERIFY(c2.TakeStartup(third));
        }
        {
            auto e3 = std::make_shared<Engine>(); Deferred c3(e3); c3.Start();
            QVERIFY(!c3.TakeStartup(id));
        }
        {
            const QString first = dir.filePath("a-keyed"), twin = dir.filePath("b-keyed");
            copy(first, "1.0"); copy(twin, "0.9");
            const QStringList both{first, twin};
            for(const QString &folder : both){
                QFile m(folder + "/manifest.json"); QVERIFY(m.open(QIODevice::ReadOnly));
                QJsonObject manifest = QJsonDocument::fromJson(m.readAll()).object(); m.close();
                manifest["key"] = "YWJj";
                QFile::remove(folder + "/manifest.json");
                Write(folder + "/manifest.json", QJsonDocument(manifest).toJson());
            }
            const QString keyed = ExtensionManifest::Read(first).id;
            QCOMPARE(ExtensionManifest::Read(twin).id, keyed);
            s.setValue("network/@Extensions", QStringList{first});
            s.setValue("network/@DisabledExtensions", QStringList{});
            auto e4 = std::make_shared<Engine>(); Deferred c4(e4); c4.SetInstalledFile(dir.filePath("installed4.json")); c4.Start();
            c4.Reconcile({first, twin}, {}, {});
            QVERIFY(c4.TakeStartup(keyed));
        }
    }
    void aShortcutIsTheFirstExtensionsAndRunsOnce() {
        QTemporaryDir dir;
        auto make = [&](const QString &name, const QByteArray &commands, bool action){
            const QString folder = dir.filePath(name);
            QVERIFY(QDir().mkpath(folder));
            Write(folder + "/manifest.json", R"({"manifest_version":3,"name":")" + name.toUtf8() + R"(","version":"1",)"
                  + (action ? QByteArray(R"("action":{},)") : QByteArray()) + R"("commands":)" + commands + "}");
        };
        make("one", R"({"go": {"suggested_key": "Ctrl+Shift+Y", "description": "Go"}, "_execute_action": {"suggested_key": "Alt+Shift+A"}})", true);
        make("two", R"({"also": {"suggested_key": "Ctrl+Shift+Y"}, "other": {"suggested_key": "Ctrl+Shift+U"}})", false);
        make("off", R"({"late": {"suggested_key": "Ctrl+Shift+K"}})", false);
        const QString one = dir.filePath("one"), two = dir.filePath("two"), off = dir.filePath("off");
        const QString first = ExtensionManifest::Read(one).id, second = ExtensionManifest::Read(two).id;
        auto e = std::make_shared<Engine>(); Deferred c(e);
        c.Reconcile({one, two, off}, {off}, {});
        QVERIFY(c.HasCommand(QStringLiteral("Ctrl+Shift+Y")));
        QVERIFY(c.HasCommand(QStringLiteral("Ctrl+Shift+U")));
        QVERIFY(!c.HasCommand(QStringLiteral("Ctrl+Shift+K")));
        QVERIFY(!c.HasCommand(QString()));

        QSignalSpy worker(&c, &ExtensionController::WorkerEvent), button(&c, &ExtensionController::ActionCommand);
        QVERIFY(c.RunCommand(QStringLiteral("Ctrl+Shift+Y"), 42));
        QCOMPARE(worker.size(), 1);
        QCOMPARE(worker.at(0).at(0).toString(), first);
        QCOMPARE(worker.at(0).at(1).toString(), QStringLiteral("commands.onCommand"));
        QCOMPARE(worker.at(0).at(2).toJsonArray(), QJsonArray() << QStringLiteral("go"));
        QCOMPARE(worker.at(0).at(3).toLongLong(), 42);
        QVERIFY(c.RunCommand(QStringLiteral("Alt+Shift+A"), 7));
        QCOMPARE(button.size(), 1);
        QCOMPARE(button.at(0).at(0).toString(), first);
        QCOMPARE(button.at(0).at(1).toLongLong(), 7);
        QCOMPARE(worker.size(), 1);
        QVERIFY(!c.RunCommand(QStringLiteral("Ctrl+Shift+K"), 7));

        auto said = [](const QJsonArray &a){ return QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)); };
        QCOMPARE(said(c.CommandsOf(second, nullptr)),
                 QStringLiteral(R"([{"description":"","name":"also","shortcut":""},{"description":"","name":"other","shortcut":"Ctrl+Shift+U"}])"));
        QCOMPARE(said(c.CommandsOf(first, [](const QString &key){ return key == QStringLiteral("Alt+Shift+A"); })),
                 QStringLiteral(R"([{"description":"","name":"_execute_action","shortcut":""},{"description":"Go","name":"go","shortcut":"Ctrl+Shift+Y"}])"));

        c.Reconcile({one, two, off}, {one, off}, {});
        QVERIFY(c.RunCommand(QStringLiteral("Ctrl+Shift+Y"), 42));
        QCOMPARE(worker.last().at(0).toString(), second);
        QCOMPARE(worker.last().at(2).toJsonArray(), QJsonArray() << QStringLiteral("also"));
        QVERIFY(!c.HasCommand(QStringLiteral("Alt+Shift+A")));

        {
            Settings &s = Application::GlobalSettings();
            s.setValue("network/@Extensions", QStringList{one, two});
            s.setValue("network/@DisabledExtensions", QStringList{});
            auto e2 = std::make_shared<Engine>(); Deferred c2(e2); c2.Start();
            QVERIFY(c2.RunCommand(QStringLiteral("Ctrl+Shift+Y"), 1));
            int adds = 0;
            for(int i = 0; i < 200 && (c2.IsBusy() || !e2->pending.empty() || !c2.IsReady()); i++){
                if(e2->pending.empty()){ QTest::qWait(1); continue; }
                const bool adding = Next(e2) == QStringLiteral("add");
                e2->error = adding && adds++ == 0 ? QStringLiteral("boom") : QString();
                Finish(e2);
            }
            QCOMPARE(adds, 2);
            QSignalSpy heard(&c2, &ExtensionController::WorkerEvent);
            QVERIFY(c2.RunCommand(QStringLiteral("Ctrl+Shift+Y"), 1));
            QCOMPARE(heard.size(), 1);
            QCOMPARE(heard.at(0).at(0).toString(), second);
            QCOMPARE(heard.at(0).at(2).toJsonArray(), QJsonArray() << QStringLiteral("also"));
        }

        e->handlesCommands = false;
        QVERIFY(!c.HasCommand(QStringLiteral("Ctrl+Shift+Y")));
        QVERIFY(!c.RunCommand(QStringLiteral("Ctrl+Shift+Y"), 42));
    }
    void onInstalledIsOwedFromTheStartAndTakenOnce() {
        QTemporaryDir dir;
        const QString ext = dir.filePath("ext"), other = dir.filePath("other");
        const QString ledger = dir.filePath("installed.json"), menus = dir.filePath("menus.json");
        auto copy = [&](const QString &to, const QString &version){
            QDir(to).removeRecursively();
            QVERIFY(QDir().mkpath(to));
            foreach(const QFileInfo &f, QDir(Fixture()).entryInfoList(QDir::Files))
                QVERIFY(QFile::copy(f.absoluteFilePath(), to + "/" + f.fileName()));
            QVERIFY(QDir(Fixture() + "/_locales/en").exists());
            QVERIFY(QDir().mkpath(to + "/_locales/en"));
            QVERIFY(QFile::copy(Fixture() + "/_locales/en/messages.json", to + "/_locales/en/messages.json"));
            QFile m(to + "/manifest.json"); QVERIFY(m.open(QIODevice::ReadOnly));
            QJsonObject manifest = QJsonDocument::fromJson(m.readAll()).object(); m.close();
            manifest["version"] = version;
            QFile::remove(to + "/manifest.json");
            Write(to + "/manifest.json", QJsonDocument(manifest).toJson());
        };
        copy(ext, "1.0.0");
        copy(other, "3.0");
        Settings &s = Application::GlobalSettings();
        auto registered = [&](const QStringList &paths){
            s.setValue("network/@Extensions", paths);
            s.setValue("network/@DisabledExtensions", QStringList{});
        };
        const QString id = ExtensionManifest::Read(ext).id, second = ExtensionManifest::Read(other).id;
        QVERIFY(!id.isEmpty() && !second.isEmpty() && id != second);
        auto said = [](const QJsonObject &o){ return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)); };
        bool written = false;

        registered({ext});
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetMenusFile(menus); c.Start();
            QVERIFY(c.MenuCall(id, "contextMenus.create", QJsonArray() << QJsonObject{{"id", "x"}, {"title", "X"}}).value("ok").toBool());
            c.SaveMenusNow();
            QVERIFY(c.TakeInstalled(id, &written).isEmpty());
        }
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetMenusFile(menus); c.SetInstalledFile(ledger);
            QVERIFY(c.MenusOf(id));
            c.Start();
            QVERIFY(!c.MenusOf(id));
            QCOMPARE(said(c.TakeInstalled(id, &written)), QStringLiteral(R"({"previousVersion":"1.0.0","reason":"update"})"));
            QVERIFY(written);
            QVERIFY(c.TakeInstalled(id, &written).isEmpty());
            QFile kept(ledger); QVERIFY(kept.open(QIODevice::ReadOnly));
            QVERIFY(ExtensionUi::Installed::FromJson(QJsonDocument::fromJson(kept.readAll()).object(), nullptr).OwedTo(id).reason.isEmpty());
        }
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QVERIFY(c.TakeInstalled(id, &written).isEmpty());
        }
        copy(ext, "1.1.0");
        registered({ext, other});
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
        }
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QCOMPARE(said(c.TakeInstalled(id, &written)), QStringLiteral(R"({"previousVersion":"1.0.0","reason":"update"})"));
            QCOMPARE(said(c.TakeInstalled(second, &written)), QStringLiteral(R"({"reason":"install"})"));
        }
        registered({ext});
        { auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start(); QVERIFY(!c.InstalledLedger().Contains(second)); }
        registered({ext, other});
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QCOMPARE(said(c.TakeInstalled(second, &written)), QStringLiteral(R"({"reason":"install"})"));
        }
        copy(ext, "1.1.5");
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            e->workerOfThisRun = false;
            QVERIFY(c.TakeInstalled(id, &written).isEmpty());
            QVERIFY(written);
            QCOMPARE(c.InstalledLedger().OwedTo(id).reason, QStringLiteral("update"));
            e->workerOfThisRun = true;
            QCOMPARE(said(c.TakeInstalled(id, &written)), QStringLiteral(R"({"previousVersion":"1.1.0","reason":"update"})"));
        }
        registered({ext});
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QVERIFY(!c.InstalledLedger().Contains(second));
            c.Reconcile({ext, other}, {}, {});
            QCOMPARE(said(c.TakeInstalled(second, &written)), QStringLiteral(R"({"reason":"install"})"));
            for(int i = 0; i < 200 && (c.IsBusy() || !e->pending.empty() || !c.IsReady()); i++){
                if(!e->pending.empty()) Finish(e); else QTest::qWait(1);
            }
            QVERIFY(!c.IsBusy() && c.IsReady());
            copy(other, "3.1");
            c.Retry(other);
            QCOMPARE(c.Rows().last().manifest.version, QStringLiteral("3.1"));
            QVERIFY(c.InstalledLedger().OwedTo(second).reason.isEmpty());
            QVERIFY(c.TakeInstalled(second, &written).isEmpty());
        }
        registered({ext, other});
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QCOMPARE(said(c.TakeInstalled(second, &written)), QStringLiteral(R"({"previousVersion":"3.0","reason":"update"})"));
        }
        copy(other, "3.0");
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            c.TakeInstalled(second, &written);
        }
        registered({ext, other});
        QVERIFY(QFile::rename(other + "/manifest.json", other + "/manifest.away"));
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QVERIFY(c.InstalledLedger().Contains(second));
        }
        QVERIFY(QFile::rename(other + "/manifest.away", other + "/manifest.json"));
        Write(ledger, "{not json");
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QVERIFY(QFile::exists(ledger + ".bad"));
            QCOMPARE(said(c.TakeInstalled(second, &written)), QStringLiteral(R"({"previousVersion":"3.0","reason":"update"})"));
        }
        {
            QFile f(ledger); QVERIFY(f.open(QIODevice::ReadOnly));
            QJsonObject all = QJsonDocument::fromJson(f.readAll()).object(); f.close();
            QVERIFY(all.contains(second));
            all[second] = QJsonObject{{"version", 3}};
            Write(ledger, QJsonDocument(all).toJson());
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QCOMPARE(said(c.TakeInstalled(second, &written)), QStringLiteral(R"({"previousVersion":"3.0","reason":"update"})"));
        }
        copy(ext, "1.2.0");
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetInstalledFile(ledger); c.Start();
            QVERIFY(QFile::remove(ledger));
            QVERIFY(QDir().mkpath(ledger));
            QVERIFY(c.TakeInstalled(id, &written).isEmpty());
            QVERIFY(!written);
            QCOMPARE(c.InstalledLedger().OwedTo(id).reason, QStringLiteral("update"));
            QVERIFY(QDir(ledger).removeRecursively());
        }
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.Start();
            QVERIFY(c.TakeInstalled(id, &written).isEmpty());
            QVERIFY(written);
        }
    }
    void theRowSaysWhatBecameOfTheCopy() {
        QVERIFY(ExtensionController::CopyNote(QString(), QString(), false).isEmpty());
        const QString refused = ExtensionController::CopyNote
            (QStringLiteral("a file could not be copied (%1)"), QStringLiteral("x/y.js"), true);
        QVERIFY2(refused.startsWith(QStringLiteral("Runs without the compatibility layer: ")), qPrintable(refused));
        QVERIFY2(refused.contains(QStringLiteral("(x/y.js)")), qPrintable(refused));
        const QString withheld = ExtensionController::CopyNote(QString(), QString(), true);
        QVERIFY2(withheld.contains(QStringLiteral("web_accessible_resources")), qPrintable(withheld));
    }
    void hostPermissionsAreTheirOwnKey() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        Write(dir.filePath("manifest.json"),
              "{\"manifest_version\":3,\"name\":\"h\",\"version\":\"1\","
              "\"permissions\":[\"storage\",\"https://p.example/*\"],"
              "\"host_permissions\":[\"https://h.example/*\",\"<all_urls>\"],"
              "\"optional_host_permissions\":[\"https://o.example/*\"],"
              "\"content_scripts\":[{\"matches\":[\"https://c.example/*\"],\"js\":[\"c.js\"]}]}");
        const auto m = ExtensionManifest::Read(dir.path());
        QVERIFY2(m.error.isEmpty(), qPrintable(m.error));
        QVERIFY(!m.tabsPermission);
        QCOMPARE(m.hostPermissions, QStringList() << QStringLiteral("https://h.example/*") << QStringLiteral("<all_urls>"));
    }
    void manifestAndResourceBoundary() {
        const auto m = ExtensionManifest::Read(Fixture());
        QVERIFY2(m.error.isEmpty(), qPrintable(m.error));
        QCOMPARE(m.id.size(), 32);
        QCOMPARE(m.popup.scheme(), QStringLiteral("chrome-extension"));
        QCOMPARE(m.popup.host(), m.id);
        QVERIFY(m.tabsPermission);
        QVERIFY(m.permissions.contains(QStringLiteral("tabs")));
        QVERIFY(m.permissions.contains(QStringLiteral("webNavigation")));
        QVERIFY(!m.permissions.contains(QStringLiteral("history")));
        QVERIFY(!ExtensionManifest::Resource(m.path, "popup.html").isEmpty());
        QVERIFY(ExtensionManifest::Resource(m.path, "../manifest.json").isEmpty());
        QVERIFY(ExtensionManifest::Resource(m.path, "%2e%2e/manifest.json").isEmpty());
        QVERIFY(ExtensionManifest::Resource(m.path, "https://example.com/x").isEmpty());
        QVERIFY(ExtensionManifest::Resource(m.path, "file:///C:/x").isEmpty());
        QTemporaryDir outside;
        QVERIFY(QDir().mkdir(outside.filePath("extension")));
        Write(outside.filePath("secret.txt"), "outside");
        QVERIFY(ExtensionManifest::Resource(ExtensionManifest::NormalizePath(outside.filePath("extension")), "../secret.txt").isEmpty());
        QCOMPARE(ExtensionManifest::PathKey(Fixture() + "/./"), ExtensionManifest::PathKey(Fixture()));
#ifdef Q_OS_WIN
        QCOMPARE(ExtensionManifest::PathKey(Fixture().toUpper()), ExtensionManifest::PathKey(Fixture()));
        const QString native = QDir::toNativeSeparators(m.path);
        const QByteArray hash = QCryptographicHash::hash(
            QByteArray(reinterpret_cast<const char*>(native.utf16()), native.size() * 2),
            QCryptographicHash::Sha256).first(16);
        QString chromium;
        for (const char c : hash)
            chromium += QString(QChar('a' + (static_cast<unsigned char>(c) >> 4))) + QChar('a' + (c & 15));
        QCOMPARE(m.id, chromium);
#endif
    }
    void chromeVersionFolderFollowsAnUpdate() {
        QTemporaryDir dir;
        const QString home = dir.filePath("Extensions/ddkjiahejlhfcafbddmgiahcphecmpfh");
        auto make = [](const QString &folder, bool withManifest) {
            QVERIFY(QDir().mkpath(folder));
            if (!withManifest) return;
            Write(folder + "/manifest.json", R"({"manifest_version":3,"name":")" + QFileInfo(folder).fileName().toUtf8() +
                  R"(","version":"1","key":"YWJj","action":{"default_popup":"popup.html"}})");
            Write(folder + "/popup.html", "");
        };
        make(home + "/1.9.9_0", true);
        make(home + "/1.10.0_0", true);
        make(home + "/1.10.0_1", true);
        make(home + "/2.0.0_0", false);
        make(home + "/9.9.9", true);
        const QString registered = home + "/1.2.0_0";
        const auto m = ExtensionManifest::Read(registered);
        QVERIFY2(m.error.isEmpty(), qPrintable(m.error));
        QCOMPARE(m.path, ExtensionManifest::NormalizePath(registered));
        QCOMPARE(m.folder, ExtensionManifest::NormalizePath(home + "/1.10.0_1"));
        QCOMPARE(m.name, QStringLiteral("1.10.0_1"));
        QCOMPARE(m.popup.host(), m.id);
        QCOMPARE(ExtensionManifest::Read(home + "/1.9.9_0").folder, ExtensionManifest::NormalizePath(home + "/1.9.9_0"));
        QVERIFY(!ExtensionManifest::Read(home + "/current").error.isEmpty());
        make(dir.filePath("plain/1.3.0_0"), true);
        const auto plain = ExtensionManifest::Read(dir.filePath("plain/1.2.0_0"));
        QVERIFY(!plain.error.isEmpty());
        QCOMPARE(plain.folder, plain.path);
        QVERIFY(QDir(home).removeRecursively());
        const QString gone = ExtensionManifest::Read(registered).error;
        QVERIFY(!gone.isEmpty());
        make(dir.filePath("large"), false);
        Write(dir.filePath("large/manifest.json"), QByteArray(1024 * 1024 + 1, ' '));
        const QString large = ExtensionManifest::Read(dir.filePath("large")).error;
        QVERIFY(!large.isEmpty());
        QVERIFY(large != gone);
    }
    void anExtensionLoadedFromTheNewestVersionIsTheRegistrations() {
        QTemporaryDir dir;
        const QString home = dir.filePath("Extensions/ddkjiahejlhfcafbddmgiahcphecmpfh");
        QVERIFY(QDir().mkpath(home + "/2.0.0_0"));
        Write(home + "/2.0.0_0/manifest.json", R"({"manifest_version":3,"name":"updated","version":"2","key":"YWJj"})");
        const QString registered = home + "/1.0.0_0";
        Application::GlobalSettings().setValue("network/@Extensions", QStringList{registered});
        const auto m = ExtensionManifest::Read(registered);
        auto e = std::make_shared<Engine>();
        e->items.insert(m.id, {m.id, "updated", m.folder, true, {}});
        Deferred c(e); c.Start(); Finish(e); QTRY_VERIFY(c.IsReady());
        QCOMPARE(e->calls, QStringList{"snapshot"});
        QVERIFY2(c.Rows().first().error.isEmpty(), qPrintable(c.Rows().first().error));
        QVERIFY(c.Rows().first().loaded);
    }
    void twoRegistrationsOfOneFolderOwnItOnce() {
        QTemporaryDir dir;
        const QString home = dir.filePath("Extensions/ddkjiahejlhfcafbddmgiahcphecmpfh");
        QVERIFY(QDir().mkpath(home + "/2.0.0_0"));
        Write(home + "/2.0.0_0/manifest.json", R"({"manifest_version":3,"name":"updated","version":"2","key":"YWJj"})");
        const QString old = home + "/1.0.0_0", current = home + "/2.0.0_0";
        const auto m = ExtensionManifest::Read(current);
        auto e = std::make_shared<Engine>();
        e->items.insert(m.id, {m.id, "updated", m.folder, true, {}});
        Application::GlobalSettings().setValue("network/@Extensions", QStringList{current, old});
        Deferred c(e); c.Start(); Finish(e); QTRY_VERIFY(c.IsReady());
        QCOMPARE(c.Rows().size(), 2);
        QVERIFY(c.Rows().at(0).loaded);
        QVERIFY(!c.Rows().at(1).loaded);
        QVERIFY(!c.Rows().at(1).error.isEmpty());
        c.Reconcile({current}, {}, {}); QTest::qWait(20);
        QCOMPARE(e->calls, QStringList{"snapshot"});
        QVERIFY(c.Rows().first().loaded);
    }
    void pickDirectoryFallsBackFromChromesExtensions() {
        QTemporaryDir dir;
        const QString home = dir.filePath("home"); QVERIFY(QDir().mkpath(home));
        const QString root = dir.filePath("User Data");
        QCOMPARE(ExtensionController::PickDirectoryFrom(QString(), home), home);
        QCOMPARE(ExtensionController::PickDirectoryFrom(root, home), home);
        QVERIFY(QDir().mkpath(root));
        QCOMPARE(ExtensionController::PickDirectoryFrom(root, home), root);
        const QString extensions = QDir(root).filePath("Default/Extensions");
        QVERIFY(QDir().mkpath(extensions));
        QCOMPARE(ExtensionController::PickDirectoryFrom(root, home), extensions);
    }
    void disabledBeforeFirstPage() {
        Desired(true, false);
        auto e = std::make_shared<Engine>(); Deferred c(e); c.Start();
        QCOMPARE(Next(e), QStringLiteral("snapshot")); Finish(e);
        QTRY_VERIFY(c.IsReady()); QVERIFY(!c.Rows().first().enabled);
        QCOMPARE(e->calls, QStringList{"snapshot"});
    }
    void menusAreKeptAndGoWithTheRegistration() {
        QTemporaryDir dir;
        const QString file = dir.filePath("menus.json");
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.SetMenusFile(file); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_VERIFY(c.IsReady());
        const QString id = c.Rows().first().manifest.id;
        QVERIFY(!id.isEmpty());

        QStringList events;
        QObject::connect(&c, &ExtensionController::WorkerEvent, [&](const QString &who, const QString &name, const QJsonArray &args, qint64 tab){
            events << who.left(4) + " " + name + " " + QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact)) + " @" + QString::number(tab);
        });

        QCOMPARE(c.MenuCall(id, "contextMenus.create", QJsonArray() << QJsonDocument::fromJson(R"({"id": "top", "title": "Top", "contexts": ["all"]})").object()).value("ok").toBool(), true);
        QCOMPARE(c.MenuCall(id, "contextMenus.create", QJsonArray() << QJsonDocument::fromJson(R"({"id": "box", "title": "Box", "type": "checkbox", "parentId": "top"})").object()).value("ok").toBool(), true);
        QCOMPARE(c.MenuCall(id, "contextMenus.create", QJsonArray() << QJsonDocument::fromJson(R"({"id": "top", "title": "again"})").object()).value("error").toString(),
                 QStringLiteral("Cannot create item with duplicate id top"));
        QVERIFY(!c.MenuCall(id, "contextMenus.getAll", QJsonArray()).value("ok").toBool());
        QVERIFY(c.MenusOf(id) && c.MenusOf(id)->Items().size() == 2);
        QVERIFY(!c.MenusOf("nobody"));
        QVERIFY(!QFile::exists(file));
        QTRY_VERIFY(QFile::exists(file));

        ExtensionUi::MenuContext context;
        context.contexts.insert("page");
        context.pageUrl = QUrl("https://a.example/");
        c.ClickMenu(id, "box", context, 7);
        QCOMPARE(events.size(), 1);
        QVERIFY2(events.first().endsWith("contextMenus.onClicked [{\"checked\":true,\"editable\":false,\"frameId\":0,\"menuItemId\":\"box\",\"pageUrl\":\"https://a.example/\",\"parentMenuItemId\":\"top\",\"wasChecked\":false}] @7"), qPrintable(events.first()));
        QVERIFY(c.MenusOf(id)->Items().at(1).checked);
        c.ClickMenu(id, "nosuch", context, 7);
        c.ClickMenu("nobody", "box", context, 7);
        QCOMPARE(events.size(), 1);
        c.SaveMenusNow();

        {
            auto e2 = std::make_shared<Engine>(); Deferred again(e2); again.SetMenusFile(file);
            QVERIFY(again.MenusOf(id));
            QCOMPARE(again.MenusOf(id)->Items().size(), 2);
            QVERIFY(again.MenusOf(id)->Items().at(1).checked);
        }
        {
            Write(dir.filePath("broken.json"), "{not json");
            auto e3 = std::make_shared<Engine>(); Deferred broken(e3); broken.SetMenusFile(dir.filePath("broken.json"));
            QVERIFY(!broken.MenusOf(id));
        }

        c.Reconcile({}, {}, {});
        QTRY_COMPARE(Next(e), QStringLiteral("remove")); Finish(e);
        QTRY_VERIFY(c.Rows().isEmpty());
        QVERIFY(!c.MenusOf(id));
        c.SaveMenusNow();
        QFile f(file); QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("{}"));
    }
    void aCopiedMenuTakesTheEnginesCalls() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_VERIFY(c.IsReady());
        const QString id = c.Rows().first().manifest.id;
        auto json = [](const char *text){ return QJsonDocument::fromJson(text).object(); };
        auto call = [](const QString &op, const QJsonArray &args){ return QJsonArray() << op << args; };
        auto titles = [&](){
            QStringList out;
            if(c.MenusOf(id)) foreach(const ExtensionUi::MenuItem &item, c.MenusOf(id)->Items()) out << item.id + "=" + item.title;
            return out.join(',');
        };
        const QJsonArray first = QJsonArray()
            << call("create", QJsonArray() << json(R"({"id": "a", "title": "A", "contexts": ["action"]})"))
            << call("create", QJsonArray() << json(R"({"id": "t", "title": "T", "contexts": ["tab"]})"))
            << call("create", QJsonArray() << json(R"({"id": "b", "title": "B", "type": "checkbox", "contexts": ["action"]})"))
            << call("update", QJsonArray() << "a" << json(R"({"title": "A2"})"))
            << call("nosuch", QJsonArray())
            << QJsonValue(5);
        QVERIFY(!c.MirrorCall(id, QJsonArray() << first).value("ok").toBool());
        QCOMPARE(titles(), QString());
        c.SetMenusMirrored(true);
        QVERIFY(!c.MirrorCall(id, QJsonArray() << "not a list").value("ok").toBool());
        QVERIFY(c.MirrorCall(id, QJsonArray() << first).value("ok").toBool());
        QCOMPARE(titles(), QStringLiteral("a=A2,b=B"));
        QVERIFY(c.MirrorCall(id, QJsonArray() << (QJsonArray() << call("create", QJsonArray() << json(R"({"id": "a", "title": "A3", "contexts": ["action"]})")))).value("ok").toBool());
        QCOMPARE(titles(), QStringLiteral("b=B,a=A3"));
        QStringList events;
        QObject::connect(&c, &ExtensionController::WorkerEvent, [&](const QString &, const QString &name, const QJsonArray &args, qint64 tab){
            events << name + " " + QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact)) + " @" + QString::number(tab);
        });
        c.ClickMenu(id, "b", ExtensionUi::Menus::ActionContext(QUrl("https://a.example/")), 7);
        QCOMPARE(events.size(), 1);
        QVERIFY2(events.first().startsWith("vanilla.actionMenuClicked [{\"checked\":true,") && events.first().endsWith(" @7"), qPrintable(events.first()));
        QVERIFY(c.MenusOf(id)->Items().first().checked);
        c.SetMenusMirrored(false);
        c.ClickMenu(id, "b", ExtensionUi::Menus::ActionContext(QUrl("https://a.example/")), 7);
        QVERIFY2(events.last().startsWith("contextMenus.onClicked "), qPrintable(events.last()));
    }
    void rulesetsChosenAreKeptPerProfile() {
        QTemporaryDir dir;
        const QString root = dir.filePath("ext"), plain = dir.filePath("plain");
        QVERIFY(QDir().mkpath(root + "/rules"));
        QVERIFY(QDir().mkpath(plain));
        const QByteArray manifest = R"({"manifest_version":3,"name":"rules","version":"1","key":"AAAA",
            "permissions":["declarativeNetRequest"],
            "declarative_net_request":{"rule_resources":[
              {"id":"a","enabled":true,"path":"rules/a.json"},
              {"id":"b","enabled":false,"path":"rules/b.json"}]}})";
        Write(root + "/manifest.json", manifest);
        Write(root + "/rules/a.json", "[]");
        Write(root + "/rules/b.json", "[]");
        Write(plain + "/manifest.json", R"({"manifest_version":3,"name":"plain","version":"1"})");
        Settings &s = Application::GlobalSettings();
        s.setValue("network/@Extensions", QStringList{root, plain});
        const QString id = ExtensionManifest::Read(root).id, plainId = ExtensionManifest::Read(plain).id;
        const QString one = dir.filePath("one.json"), two = dir.filePath("two.json");
        auto files = [](){
            QStringList names;
            foreach(const ExtensionRuleFiles &each, ExtensionController::EnabledRuleFiles())
                foreach(const QString &file, each.files) names << QFileInfo(file).fileName();
            return names.join(",");
        };
        auto call = [](ExtensionController &c, const QString &who, const char *api, const char *options = nullptr){
            QJsonArray args;
            if(options) args << QJsonDocument::fromJson(options).object();
            return c.RulesCall(who, QString::fromLatin1(api), args);
        };
        auto value = [](const QJsonObject &reply){
            return QString::fromUtf8(QJsonDocument(reply.value("value").toArray()).toJson(QJsonDocument::Compact));
        };
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetRulesFile(one);
            QCOMPARE(value(call(c, id, "declarativeNetRequest.getEnabledRulesets")), QStringLiteral("[\"a\"]"));
            QCOMPARE(files(), QStringLiteral("a.json"));
            QVERIFY(call(c, id, "declarativeNetRequest.updateEnabledRulesets", R"({"enableRulesetIds":["b"],"disableRulesetIds":["a"]})").value("ok").toBool());
            QCOMPARE(value(call(c, id, "declarativeNetRequest.getEnabledRulesets")), QStringLiteral("[\"b\"]"));
            QCOMPARE(files(), QStringLiteral("b.json"));
            QCOMPARE(call(c, id, "declarativeNetRequest.updateEnabledRulesets", R"({"enableRulesetIds":["z"]})").value("error").toString(),
                     QStringLiteral("Invalid ruleset id: z."));
            QVERIFY(!call(c, plainId, "declarativeNetRequest.getEnabledRulesets").value("ok").toBool());
            QVERIFY(!call(c, id, "declarativeNetRequest.getMatchedRules").value("ok").toBool());
            {
                auto e2 = std::make_shared<Engine>(); Deferred nofile(e2);
                QVERIFY(!call(nofile, id, "declarativeNetRequest.getEnabledRulesets").value("ok").toBool());
            }
            {
                auto e2 = std::make_shared<Engine>(); Deferred other(e2); other.SetRulesFile(two);
                QCOMPARE(value(call(other, id, "declarativeNetRequest.getEnabledRulesets")), QStringLiteral("[\"a\"]"));
                QCOMPARE(files(), QStringLiteral("b.json"));
                QVERIFY(call(other, id, "declarativeNetRequest.updateEnabledRulesets", R"({"enableRulesetIds":["a"]})").value("ok").toBool());
                QCOMPARE(files(), QStringLiteral("a.json,b.json"));
            }
            QVERIFY(!QFile::exists(one));
        }
        QVERIFY(QFile::exists(one));
        {
            auto e = std::make_shared<Engine>(); Deferred again(e); again.SetRulesFile(one);
            QCOMPARE(value(call(again, id, "declarativeNetRequest.getEnabledRulesets")), QStringLiteral("[\"b\"]"));
        }
        Write(root + "/manifest.json", QByteArray(manifest).replace("\"version\":\"1\"", "\"version\":\"2\""));
        {
            auto e = std::make_shared<Engine>(); Deferred newer(e); newer.SetRulesFile(one);
            QCOMPARE(value(call(newer, id, "declarativeNetRequest.getEnabledRulesets")), QStringLiteral("[\"a\"]"));
            QVERIFY(!newer.HeldRules().Holds(id));
            QVERIFY(call(newer, id, "declarativeNetRequest.updateEnabledRulesets", R"({"enableRulesetIds":["b"]})").value("ok").toBool());
            newer.Reconcile({plain}, {}, {});
            QVERIFY(!newer.HeldRules().Holds(id));
            newer.SaveRulesNow();
            QFile f(one); QVERIFY(f.open(QIODevice::ReadOnly));
            QCOMPARE(f.readAll(), QByteArray("{}"));
        }
        Write(one, QByteArray("{\"") + id.toLatin1() + "\":{\"enabled\":[\"b\"],\"enabledVersion\":\"2\"},\"gone\":{\"enabled\":[\"x\"],\"enabledVersion\":\"1\"}}");
        {
            auto e = std::make_shared<Engine>(); Deferred read(e); read.SetRulesFile(one);
            QVERIFY(read.HeldRules().Holds(id));
            QVERIFY(!read.HeldRules().Holds("gone"));
        }
        Write(one, "{\"gone\":{\"enabled\":[\"x\"],\"enabledVersion\":\"1\"}}");
        QVERIFY(QFile::remove(plain + "/manifest.json"));
        {
            auto e = std::make_shared<Engine>(); Deferred unread(e); unread.SetRulesFile(one);
            QVERIFY(unread.HeldRules().Holds("gone"));
        }
        {
            const QString copy = dir.filePath("copy");
            QVERIFY(QDir().mkpath(copy + "/rules"));
            Write(copy + "/manifest.json", manifest);
            QCOMPARE(ExtensionManifest::Read(copy).id, id);
            Write(one, QByteArray("{\"") + id.toLatin1() + "\":{\"enabled\":[\"b\"],\"enabledVersion\":\"2\"}}");
            s.setValue("network/@Extensions", QStringList{root, copy});
            auto e = std::make_shared<Engine>(); Deferred twice(e); twice.SetRulesFile(one);
            QVERIFY(twice.HeldRules().Holds(id));
            twice.Reconcile({copy}, {}, {});
            QVERIFY(twice.HeldRules().Holds(id));
            twice.Reconcile({}, {}, {});
            QVERIFY(!twice.HeldRules().Holds(id));
        }
        {
            const QString locked = dir.filePath("locked.json");
            QVERIFY(QDir().mkpath(locked));
            auto e = std::make_shared<Engine>(); Deferred cannot(e); cannot.SetRulesFile(locked);
            QVERIFY(cannot.HeldRules().Extensions().isEmpty());
            cannot.SaveRulesNow();
            QVERIFY(QFileInfo(locked).isDir());
            QVERIFY(!QFile::exists(locked + ".bad"));
        }
        Write(one, "{not json");
        {
            auto e = std::make_shared<Engine>(); Deferred broken(e); broken.SetRulesFile(one);
            QVERIFY(broken.HeldRules().Extensions().isEmpty());
            QFile bad(one + ".bad"); QVERIFY(bad.open(QIODevice::ReadOnly));
            QCOMPARE(bad.readAll(), QByteArray("{not json"));
        }
    }
    void rulesPutInAreHeldAndInForce() {
        QTemporaryDir dir;
        const QString root = dir.filePath("ext");
        QVERIFY(QDir().mkpath(root));
        Write(root + "/manifest.json", R"({"manifest_version":3,"name":"rules","version":"1",
            "permissions":["declarativeNetRequestWithHostAccess"]})");
        Settings &s = Application::GlobalSettings();
        s.setValue("network/@Extensions", QStringList{root});
        const QString id = ExtensionManifest::Read(root).id;
        const QString one = dir.filePath("one.json"), two = dir.filePath("two.json");
        auto call = [](ExtensionController &c, const QString &who, const char *api, const char *options = nullptr){
            QJsonArray args;
            if(options) args << QJsonDocument::fromJson(options).object();
            return c.RulesCall(who, QString::fromLatin1(api), args);
        };
        auto ids = [](const QJsonObject &reply){
            QStringList list;
            for(const QJsonValue &rule : reply.value("value").toArray()) list << QString::number(rule.toObject().value("id").toInt());
            return list.join(",");
        };
        auto inForce = [&id](){
            foreach(const ExtensionRuleFiles &each, ExtensionController::EnabledRuleFiles())
                if(each.id == id) return QString::fromUtf8(each.session) + " / " + QString::fromUtf8(each.dynamic);
            return QString();
        };
        const char *rule1 = R"({"addRules":[{"id":1,"action":{"type":"block"},"condition":{"urlFilter":"a"}}]})";
        const char *rule2 = R"({"addRules":[{"id":2,"action":{"type":"block"},"condition":{"urlFilter":"b"}}]})";
        {
            auto e = std::make_shared<Engine>(); Deferred c(e); c.SetRulesFile(one);
            QCOMPARE(inForce(), QString());
            QVERIFY(call(c, id, "declarativeNetRequest.updateDynamicRules", rule1).value("ok").toBool());
            QVERIFY(call(c, id, "declarativeNetRequest.updateSessionRules", rule2).value("ok").toBool());
            QCOMPARE(ids(call(c, id, "declarativeNetRequest.getDynamicRules")), QStringLiteral("1"));
            QCOMPARE(ids(call(c, id, "declarativeNetRequest.getSessionRules")), QStringLiteral("2"));
            QCOMPARE(call(c, id, "declarativeNetRequest.updateDynamicRules", rule1).value("error").toString(),
                     QStringLiteral("Rule with id 1 does not have a unique ID."));
            QCOMPARE(inForce(), QStringLiteral("[{\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"b\"},\"id\":2}] / "
                                               "[{\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"a\"},\"id\":1}]"));
            {
                auto e2 = std::make_shared<Engine>(); Deferred other(e2); other.SetRulesFile(two);
                QCOMPARE(ids(call(other, id, "declarativeNetRequest.getDynamicRules")), QString());
                QVERIFY(call(other, id, "declarativeNetRequest.updateDynamicRules", rule2).value("ok").toBool());
                QVERIFY(inForce().endsWith("\"id\":1},{\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"b\"},\"id\":2}]"));
            }
            c.Reconcile({root}, {root}, {});
            s.setValue("network/@DisabledExtensions", QStringList{root});
            QCOMPARE(ids(call(c, id, "declarativeNetRequest.getSessionRules")), QString());
            QCOMPARE(ids(call(c, id, "declarativeNetRequest.getDynamicRules")), QStringLiteral("1"));
            QCOMPARE(inForce(), QString());
            s.setValue("network/@DisabledExtensions", QStringList{});
            c.Reconcile({root}, {}, {});
            QVERIFY(call(c, id, "declarativeNetRequest.updateSessionRules", rule2).value("ok").toBool());
        }
        {
            auto e = std::make_shared<Engine>(); Deferred again(e); again.SetRulesFile(one);
            QCOMPARE(ids(call(again, id, "declarativeNetRequest.getDynamicRules")), QStringLiteral("1"));
            QCOMPARE(ids(call(again, id, "declarativeNetRequest.getSessionRules")), QString());
        }
    }
    void removalWhileAddIsPending() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add"));
        c.Reconcile({}, {}, {});
        QCOMPARE(e->pending.size(), size_t(1)); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("remove")); Finish(e);
        QTRY_VERIFY(c.IsReady()); QVERIFY(c.Rows().isEmpty()); QVERIFY(e->items.isEmpty());
        QCOMPARE(e->calls, (QStringList{"snapshot", "add", "remove"}));
    }
    void latestEnabledWishWins() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); c.Reconcile({Fixture()}, {Fixture()}, {}); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("disable"));
        c.Reconcile({Fixture()}, {}, {});
        QCOMPARE(e->pending.size(), size_t(1)); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("enable")); Finish(e);
        QTRY_VERIFY(c.IsReady()); QVERIFY(c.Rows().first().enabled);
    }
    void failureNeedsExplicitRetry() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); e->error = "test refusal"; Finish(e);
        QTRY_VERIFY(c.IsReady()); QCOMPARE(c.Rows().first().error, QStringLiteral("test refusal"));
        c.Reconcile({Fixture()}, {}, {}); QTest::qWait(20); QVERIFY(e->pending.empty());
        e->error.clear(); c.Retry(Fixture()); QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_VERIFY(!c.IsBusy()); QTRY_VERIFY(c.Rows().first().enabled);
    }
    void enableFindingTheExtensionGoneDropsItAndRetryAddsAgain() {
        QTemporaryDir dir; const QString journal = dir.filePath("owned.json");
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e, journal); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); e->addEnabled = false; Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("enable"));
        e->items.clear(); Finish(e);
        QTRY_VERIFY(c.IsReady());
        const ExtensionRow row = c.Rows().first();
        QVERIFY(row.registered); QVERIFY(!row.loaded); QCOMPARE(row.error, QStringLiteral("gone"));
        QCOMPARE(Owned(journal).value(ExtensionManifest::PathKey(Fixture())).toString(), ExtensionManifest::Read(Fixture()).id);
        QTest::qWait(20); QVERIFY(e->pending.empty());
        e->addEnabled = true; c.Retry(Fixture()); QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_VERIFY(c.IsReady()); QVERIFY(c.Rows().first().loaded); QVERIFY(c.Rows().first().enabled);
        QCOMPARE(e->calls, (QStringList{"snapshot", "add", "enable", "add"}));
    }
    void unmanagedIdIsNeverReplaced() {
        Desired(); auto e = std::make_shared<Engine>(); const auto m = ExtensionManifest::Read(Fixture());
        e->items.insert(m.id, {m.id, "unmanaged", {}, true, {}});
        Deferred c(e); c.Start(); Finish(e); QTRY_VERIFY(c.IsReady());
        QVERIFY(!c.Rows().first().error.isEmpty()); QCOMPARE(e->calls, QStringList{"snapshot"});
        c.Reconcile({}, {}, {}); QTest::qWait(20); QCOMPARE(e->items.size(), 1);
    }
    void duplicateIdRejectedBeforeAdd() {
        QTemporaryDir one, two;
        const QByteArray manifest = R"({"manifest_version":3,"name":"duplicate","version":"1","key":"YWJj"})";
        Write(one.filePath("manifest.json"), manifest); Write(two.filePath("manifest.json"), manifest);
        Application::GlobalSettings().setValue("network/@Extensions", QStringList{one.path(), two.path()});
        auto e = std::make_shared<Engine>(); Deferred c(e); c.Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add"));
        QVERIFY(!c.Rows().at(1).error.isEmpty());
        Finish(e); QTRY_VERIFY(c.IsReady());
        QCOMPARE(e->calls.count("add"), 1); QVERIFY(!c.Rows().at(1).error.isEmpty());
    }
    void intentSurvivesAddCrash_data() {
        QTest::addColumn<bool>("applied");
        QTest::newRow("before-add-effect") << false;
        QTest::newRow("after-add-before-callback") << true;
    }
    void intentSurvivesAddCrash() {
        QFETCH(bool, applied);
        QTemporaryDir dir; const QString journal = dir.filePath("owned.json");
        Desired(); auto e = std::make_shared<Engine>(); auto *old = new Deferred(e, journal); old->Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add"));
        QCOMPARE(Owned(journal).value(ExtensionManifest::PathKey(Fixture())).toString(), ExtensionManifest::Read(Fixture()).id);
        auto delayed = Take(e); if (applied) delayed.effect(); delete old;
        delayed.complete();
        Desired(false); Deferred restarted(e, journal); restarted.Start(); Finish(e);
        if (applied) { QTRY_COMPARE(Next(e), QStringLiteral("remove")); Finish(e); }
        QTRY_VERIFY(restarted.IsReady()); QVERIFY(e->items.isEmpty()); QVERIFY(Owned(journal).isEmpty());
    }
    void intentSurvivesRemoveCrash() {
        QTemporaryDir dir; const QString journal = dir.filePath("owned.json");
        Desired(); auto e = std::make_shared<Engine>(); auto *old = new Deferred(e, journal); old->Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e); QTRY_VERIFY(old->IsReady());
        old->Reconcile({}, {}, {}); QTRY_COMPARE(Next(e), QStringLiteral("remove"));
        auto delayed = Take(e); delayed.effect(); QVERIFY(!Owned(journal).isEmpty()); delete old; delayed.complete();
        Desired(false); Deferred restarted(e, journal); restarted.Start(); Finish(e);
        QTRY_VERIFY(restarted.IsReady()); QVERIFY(Owned(journal).isEmpty()); QCOMPARE(e->calls.count("remove"), 1);
    }
    void journalSaveFailurePreventsAdd() {
        QTemporaryDir dir; Write(dir.filePath("blocker"), "file");
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e, dir.filePath("blocker/owned.json")); c.Start(); Finish(e);
        QTRY_VERIFY(c.IsReady()); QVERIFY(!c.Error().isEmpty()); QCOMPARE(e->calls, QStringList{"snapshot"});
    }
    void settingsEditsPreserveUnrelatedRows() {
        Settings &s = Application::GlobalSettings();
        const QString other = QStringLiteral("relative/../unchanged");
        const QStringList original{QStringLiteral("# work extensions"), QString(), other, QDir::toNativeSeparators(Fixture())};
        s.setValue("network/@Extensions", original);
        ExtensionController::RegisterPath(Fixture());
        QCOMPARE(s.value("network/@Extensions").toStringList(), original);
        ExtensionController::UnregisterPath(Fixture());
        const QStringList kept = original.mid(0, 3);
        QCOMPARE(s.value("network/@Extensions").toStringList(), kept);
        ExtensionController::RegisterPath(Fixture());
        QCOMPARE(s.value("network/@Extensions").toStringList(), kept + QStringList{ExtensionManifest::NormalizePath(Fixture())});
        for (const QString &key : {QStringLiteral("network/@DisabledExtensions"), QStringLiteral("network/@PinnedExtensions")}) {
            s.setValue(key, kept);
            if (key.endsWith("DisabledExtensions")) ExtensionController::SetEnabled(Fixture(), false);
            else ExtensionController::SetPinned(Fixture(), true);
            QCOMPARE(s.value(key).toStringList(), kept + QStringList{ExtensionManifest::NormalizePath(Fixture())});
            if (key.endsWith("DisabledExtensions")) ExtensionController::SetEnabled(Fixture(), true);
            else ExtensionController::SetPinned(Fixture(), false);
            QCOMPARE(s.value(key).toStringList(), kept);
        }
    }
    void journalCorruptionPreventsMutation() {
        QTemporaryDir dir; Write(dir.filePath("owned.json"), "{bad");
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e, dir.filePath("owned.json")); c.Start();
        QVERIFY(c.IsReady()); QVERIFY(!c.Error().isEmpty()); QVERIFY(e->calls.isEmpty());
    }
    void journalFailureAfterRemoveRetainsRecoverableIntent() {
        QTemporaryDir dir; const QString journal = dir.filePath("owned.json");
        Desired(); auto e = std::make_shared<Engine>();
        auto old = std::make_unique<Deferred>(e, journal); old->Start(); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e); QTRY_VERIFY(old->IsReady());
        old->Reconcile({}, {}, {}); QTRY_COMPARE(Next(e), QStringLiteral("remove"));
        QVERIFY(QFile::rename(journal, journal + ".saved")); QVERIFY(QDir().mkdir(journal));
        Finish(e); QTRY_VERIFY(!old->Error().isEmpty()); QVERIFY(e->items.isEmpty());
        old.reset();
        QVERIFY(QDir().rmdir(journal)); QVERIFY(QFile::rename(journal + ".saved", journal));
        Desired(false); Deferred restarted(e, journal); restarted.Start(); Finish(e);
        QTRY_VERIFY(restarted.IsReady()); QVERIFY(Owned(journal).isEmpty());
    }
    void unexpectedIdIsCleanedUp() {
        Desired(); auto e = std::make_shared<Engine>(); e->returnedId = QString(32, QLatin1Char('p'));
        Deferred c(e); c.Start(); Finish(e); QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("remove")); Finish(e);
        QTRY_VERIFY(c.IsReady()); QVERIFY(!c.Error().isEmpty()); QVERIFY(e->items.isEmpty());
    }
    void timeoutReleasesNavigationWithoutOverlappingOperations() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e, {}, 20); c.Start();
        QTRY_VERIFY(c.IsReady()); QVERIFY(c.IsBusy()); QVERIFY(!c.Error().isEmpty());
        c.Reconcile({}, {}, {}); QTest::qWait(25);
        QCOMPARE(e->calls, QStringList{"snapshot"}); QCOMPARE(e->pending.size(), size_t(1));
        Finish(e); QTRY_VERIFY(!c.IsBusy()); QVERIFY(c.Error().isEmpty());
    }
    void navigationKeepsLatestAndCancels() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start();
        QObject target; auto *navigation = ExtensionNavigation::Of(&target);
        int loaded = 0;
        navigation->Request(&c, [&] { loaded = 1; });
        navigation->Request(&c, [&] { loaded = 2; });
        QCOMPARE(loaded, 0); Finish(e); QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_COMPARE(loaded, 2);
        Desired(); auto e2 = std::make_shared<Engine>(); Deferred c2(e2); c2.Start();
        navigation->Request(&c2, [&] { loaded = 3; }); navigation->Cancel();
        Finish(e2); QTRY_COMPARE(Next(e2), QStringLiteral("add")); Finish(e2); QTRY_VERIFY(c2.IsReady()); QCOMPARE(loaded, 2);
    }
    void historyWaitsAndPreservesFallback_data() {
        QTest::addColumn<bool>("valid");
        QTest::newRow("valid-history") << true;
        QTest::newRow("failed-history-fallback") << false;
    }
    void historyWaitsAndPreservesFallback() {
        QFETCH(bool, valid);
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start();
        auto view = std::make_shared<HistoryView>(&c); view->SetThis(view); view->valid = valid;
        QNetworkRequest request(QUrl("http://127.0.0.1/fallback")); request.setRawHeader("X-Test", "retained");
        view->RestoreHistoryOrLoad(request);
        QCOMPARE(view->restores, 0); QCOMPARE(view->loads, 0);
        Finish(e); QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_COMPARE(view->restores, 1); QCOMPARE(view->loads, valid ? 0 : 1);
        if (!valid) QCOMPARE(view->request, request);
    }
    void pendingHistoryDoesNotRetainView() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); c.Start();
        auto view = std::make_shared<HistoryView>(&c); view->SetThis(view);
        const WeakView weak = view;
        view->RestoreHistoryOrLoad(QNetworkRequest(QUrl("http://127.0.0.1/closed")));
        view.reset(); QVERIFY(weak.expired());
        Finish(e); QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e); QTRY_VERIFY(c.IsReady());
    }
    void popupInvalidatedBeforeBackendMutation() {
        Desired(); auto e = std::make_shared<Engine>(); Deferred c(e); bool closed = false;
        connect(&c, &ExtensionController::InvalidatePopups, &c, [&] { closed = true; });
        c.Start(); QVERIFY(closed); Finish(e); closed = false;
        QTRY_COMPARE(Next(e), QStringLiteral("add")); QVERIFY(closed); Finish(e); QTRY_VERIFY(c.IsReady());
        closed = false; c.Reconcile({Fixture()}, {Fixture()}, {}); QVERIFY(closed);
        QTRY_COMPARE(Next(e), QStringLiteral("disable")); Finish(e);
    }

    void nothingIsAskedOfTheEngineUntilAViewAsksToLoad() {
        Desired();
        auto e = std::make_shared<Engine>(); Deferred c(e);
        QCoreApplication::processEvents();
        QVERIFY(!c.IsStarted()); QVERIFY(!c.IsReady()); QVERIFY(e->calls.isEmpty());
        QVERIFY(!c.Rows().isEmpty()); QVERIFY(!c.Rows().first().loaded); QVERIFY(c.Rows().first().registered);
        c.Reconcile({Fixture()}, {}, {});
        QCoreApplication::processEvents();
        QVERIFY(e->calls.isEmpty()); QVERIFY(!c.Rows().first().loaded);

        QObject target; bool ran = false;
        ExtensionNavigation::Of(&target)->Request(&c, [&ran] { ran = true; });
        QVERIFY(c.IsStarted()); QVERIFY(!ran);
        QCOMPARE(Next(e), QStringLiteral("snapshot")); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); QVERIFY(!ran); Finish(e);
        QTRY_VERIFY(ran); QVERIFY(c.IsReady()); QVERIFY(c.Rows().first().loaded);
    }
    void aRequestOnAControllerWhichCannotStartStillRuns() {
        Desired();
        QTemporaryDir dir; QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("journal")));
        auto e = std::make_shared<Engine>(); Deferred c(e, dir.filePath(QStringLiteral("journal")));
        QObject target; bool ran = false;
        ExtensionNavigation::Of(&target)->Request(&c, [&ran] { ran = true; });
        QVERIFY(ran); QVERIFY(c.IsReady()); QVERIFY(e->calls.isEmpty());
    }
    void whatListensToReadyFromTheStartHearsItBeforeTheFirstLoad() {
        Desired();
        auto e = std::make_shared<Engine>(); Deferred c(e);
        QStringList order;
        QObject::connect(&c, &ExtensionController::Ready, [&order] { order << QStringLiteral("host"); });
        QObject target;
        ExtensionNavigation::Of(&target)->Request(&c, [&order] { order << QStringLiteral("load"); });
        QCOMPARE(Next(e), QStringLiteral("snapshot")); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_COMPARE(order, QStringList() << QStringLiteral("host") << QStringLiteral("load"));
    }
    void anExtensionTakenOffTheListIsSaidToBeUnlisted() {
        Desired();
        auto e = std::make_shared<Engine>(); Deferred c(e);
        QStringList unlisted;
        QObject::connect(&c, &ExtensionController::Unlisted, [&unlisted](const QString &id) { unlisted << id; });
        QObject target; bool ran = false;
        ExtensionNavigation::Of(&target)->Request(&c, [&ran] { ran = true; });
        QCOMPARE(Next(e), QStringLiteral("snapshot")); Finish(e);
        QTRY_COMPARE(Next(e), QStringLiteral("add")); Finish(e);
        QTRY_VERIFY(ran);
        const QString id = c.Rows().first().manifest.id;
        QVERIFY(!id.isEmpty());
        QTemporaryDir nowhere;
        c.Reconcile({Fixture(), nowhere.filePath(QStringLiteral("none"))}, {}, {});
        c.Reconcile({Fixture()}, {}, {});
        QVERIFY(unlisted.isEmpty());
        c.Reconcile({Fixture()}, {Fixture()}, {});
        QVERIFY(unlisted.isEmpty());
        c.Reconcile({}, {}, {});
        QCOMPARE(unlisted, QStringList() << id);
    }
    void aRetryBeforeTheStartStarts() {
        Desired();
        auto e = std::make_shared<Engine>(); Deferred c(e);
        c.Retry(Fixture());
        QVERIFY(c.IsStarted()); QCOMPARE(Next(e), QStringLiteral("snapshot"));
    }
};
QTEST_MAIN(ExtensionsTest)
#include "tst_extensions.moc"
