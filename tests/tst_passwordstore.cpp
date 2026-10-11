#include "switch.hpp"

#include <QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "passwordstore.hpp"

namespace {

    class FakeVault : public PasswordStore::Vault {
    public:
        QMap<QString, QString> secrets;
        QSet<QString> failWrite;
        QSet<QString> failRemove;
        bool late = false;
        QList<std::function<void()>> held;
        QStringList calls;

        void Answer(std::function<void()> answer){
            if(late) held << answer;
            else answer();
        }
        bool ReleaseOne(){
            if(held.isEmpty()) return false;
            held.takeFirst()();
            return true;
        }

        void Write(const QString &key, const QString &secret,
                   std::function<void(PasswordStore::Result)> done) override {
            calls << QStringLiteral("write ") + key;
            Answer([this, key, secret, done](){
                if(failWrite.contains(key)){ done(PasswordStore::Failed); return;}
                secrets[key] = secret;
                done(PasswordStore::Ok);
            });
        }
        void Read(const QString &key,
                  std::function<void(PasswordStore::Result, const QString&)> done) override {
            calls << QStringLiteral("read ") + key;
            Answer([this, key, done](){
                if(!secrets.contains(key)){ done(PasswordStore::NotFound, QString()); return;}
                done(PasswordStore::Ok, secrets.value(key));
            });
        }
        void Remove(const QString &key, std::function<void(PasswordStore::Result)> done) override {
            calls << QStringLiteral("remove ") + key;
            Answer([this, key, done](){
                if(failRemove.contains(key)){ done(PasswordStore::Failed); return;}
                if(!secrets.remove(key)){ done(PasswordStore::NotFound); return;}
                done(PasswordStore::Ok);
            });
        }
    };

    const QString S = QStringLiteral("space");
    const QString A = QStringLiteral("https://a.example:443");
    const QString B = QStringLiteral("https://b.example:443");

    QString Key(const QString &origin, const QString &username){
        return QStringLiteral("p|space|") + origin + QStringLiteral("|") + username;
    }
}

class tst_passwordstore : public QObject {
    Q_OBJECT

    QTemporaryDir m_Dir;

    QString IndexPath() const { return m_Dir.filePath(QStringLiteral("passwords-index.json"));}

    PasswordStore *Make(FakeVault **vault){
        QFile::remove(IndexPath());
        *vault = new FakeVault();
        return new PasswordStore(std::unique_ptr<PasswordStore::Vault>(*vault),
                                 IndexPath(), QStringLiteral("p"), this);
    }

private slots:

    void aSavedPasswordIsInTheIndexAndComesBack(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        bool saved = false;
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("secret1"),
                    [&](bool ok){ saved = ok;});
        QVERIFY(saved);
        QCOMPARE(store->Usernames(S, A), QStringList() << QStringLiteral("alice"));
        QVERIFY(store->Usernames(S, B).isEmpty());
        QVERIFY(store->Usernames(QStringLiteral("other"), A).isEmpty());
        QCOMPARE(vault->secrets.value(Key(A, QStringLiteral("alice"))), QStringLiteral("secret1"));

        QString password;
        store->Read(S, A, QStringLiteral("alice"), true,
                    [&](bool ok, const QString &p){ QVERIFY(ok); password = p;});
        QCOMPARE(password, QStringLiteral("secret1"));

        QFile file(IndexPath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray bytes = file.readAll();
        QVERIFY(bytes.contains("alice"));
        QVERIFY(!bytes.contains("secret1"));

        PasswordStore again(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                            IndexPath(), QStringLiteral("p"));
        QCOMPARE(again.Usernames(S, A), QStringList() << QStringLiteral("alice"));
        delete store;
    }

    void aPasswordTheStoreRefusedIsNotInTheIndex(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        vault->failWrite << Key(A, QStringLiteral("alice"));
        bool answered = false, saved = true;
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("x"),
                    [&](bool ok){ answered = true; saved = ok;});
        QVERIFY(answered);
        QVERIFY(!saved);
        QVERIFY(store->Usernames(S, A).isEmpty());
        QVERIFY(!QFile::exists(IndexPath()));
        delete store;
    }

    void anEntryTheStoreLostIsDroppedWhenRead(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("x"), nullptr);
        vault->secrets.clear();

        bool ok = true;
        store->Read(S, A, QStringLiteral("alice"), false, [&](bool r, const QString&){ ok = r;});
        QVERIFY(!ok);
        QCOMPARE(store->Usernames(S, A), QStringList() << QStringLiteral("alice"));

        store->Read(S, A, QStringLiteral("alice"), true, [&](bool r, const QString&){ ok = r;});
        QVERIFY(!ok);
        QVERIFY(store->Usernames(S, A).isEmpty());
        delete store;
    }

    void savingAgainReplacesThePasswordAndKeepsOneEntry(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("old"), nullptr);
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("new"), nullptr);
        QCOMPARE(store->Usernames(S, A), QStringList() << QStringLiteral("alice"));
        QCOMPARE(vault->secrets.value(Key(A, QStringLiteral("alice"))), QStringLiteral("new"));
        delete store;
    }

    void removingAllKeepsWhatTheStoreCouldNotRemove(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("1"), nullptr);
        store->Save(S, A, QStringLiteral("bob"), QStringLiteral("2"), nullptr);
        store->Save(S, B, QStringLiteral("carol"), QStringLiteral("3"), nullptr);
        vault->failRemove << Key(A, QStringLiteral("bob"));
        vault->secrets.remove(Key(B, QStringLiteral("carol")));

        int failed = -1;
        store->RemoveAll([&](int f){ failed = f;});
        QCOMPARE(failed, 1);
        QCOMPARE(store->Usernames(S, A), QStringList() << QStringLiteral("bob"));
        QVERIFY(store->Usernames(S, B).isEmpty());
        QVERIFY(!vault->secrets.contains(Key(A, QStringLiteral("alice"))));

        PasswordStore again(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                            IndexPath(), QStringLiteral("p"));
        QCOMPARE(again.Entries().size(), 1);
        delete store;
    }

    void removingManyWithAStoreWhichAnswersAtOnce(){
        const int count = 20000;
        QJsonArray entries;
        FakeVault *vault = new FakeVault();
        for(int i = 0; i < count; i++){
            const QString user = QStringLiteral("u%1").arg(i);
            QJsonObject obj;
            obj.insert(QStringLiteral("space"), S);
            obj.insert(QStringLiteral("origin"), A);
            obj.insert(QStringLiteral("username"), user);
            entries.append(obj);
            vault->secrets[Key(A, user)] = QStringLiteral("x");
        }
        QJsonObject root;
        root.insert(QStringLiteral("entries"), entries);
        QFile file(IndexPath());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(root).toJson());
        file.close();

        PasswordStore store(std::unique_ptr<PasswordStore::Vault>(vault),
                            IndexPath(), QStringLiteral("p"));
        QCOMPARE(store.Entries().size(), count);
        int failed = -1;
        store.RemoveAll([&](int f){ failed = f;});
        QCOMPARE(failed, 0);
        QVERIFY(store.Entries().isEmpty());
        QVERIFY(vault->secrets.isEmpty());
    }

    void removingOneKeepsWhatTheStoreRefused(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("1"), nullptr);
        store->Save(S, A, QStringLiteral("bob"), QStringLiteral("2"), nullptr);
        store->Save(S, B, QStringLiteral("carol"), QStringLiteral("3"), nullptr);

        PasswordStore::Result result = PasswordStore::Failed;
        store->Remove(S, A, QStringLiteral("alice"), [&](PasswordStore::Result r){ result = r;});
        QCOMPARE(result, PasswordStore::Ok);
        QCOMPARE(store->Usernames(S, A), QStringList() << QStringLiteral("bob"));
        QVERIFY(!vault->secrets.contains(Key(A, QStringLiteral("alice"))));

        vault->failRemove << Key(A, QStringLiteral("bob"));
        store->Remove(S, A, QStringLiteral("bob"), [&](PasswordStore::Result r){ result = r;});
        QCOMPARE(result, PasswordStore::Failed);
        QCOMPARE(store->Usernames(S, A), QStringList() << QStringLiteral("bob"));

        vault->secrets.remove(Key(B, QStringLiteral("carol")));
        store->Remove(S, B, QStringLiteral("carol"), [&](PasswordStore::Result r){ result = r;});
        QCOMPARE(result, PasswordStore::NotFound);
        QVERIFY(store->Usernames(S, B).isEmpty());

        vault->calls.clear();
        store->Remove(S, A, QStringLiteral("mallory"), [&](PasswordStore::Result r){ result = r;});
        QCOMPARE(result, PasswordStore::NotFound);
        QVERIFY(vault->calls.isEmpty());

        PasswordStore again(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                            IndexPath(), QStringLiteral("p"));
        QCOMPARE(again.Entries().size(), 1);
        QCOMPARE(store->PendingRemovals(), 0);
        delete store;
    }

    void aRemovalWaitingIsCounted(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("1"), nullptr);
        vault->late = true;
        store->Remove(S, A, QStringLiteral("alice"), nullptr);
        store->Remove(S, A, QStringLiteral("alice"), nullptr);
        QCOMPARE(store->PendingRemovals(), 2);
        while(vault->ReleaseOne()){}
        QCOMPARE(store->PendingRemovals(), 0);
        QVERIFY(store->Usernames(S, A).isEmpty());
        delete store;
    }

    void oneStepAtATimeInTheOrderAsked(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        vault->late = true;

        QStringList order;
        store->Save(S, A, QStringLiteral("alice"), QStringLiteral("1"),
                    [&](bool){ order << QStringLiteral("saved");});
        store->Read(S, A, QStringLiteral("alice"), true,
                    [&](bool ok, const QString &p){
                        order << (ok ? QStringLiteral("read ") + p : QStringLiteral("read nothing"));});
        store->RemoveAll([&](int){ order << QStringLiteral("removed");});

        QCOMPARE(vault->calls, QStringList() << QStringLiteral("write ") + Key(A, QStringLiteral("alice")));
        QVERIFY(store->Usernames(S, A).isEmpty());

        while(vault->ReleaseOne()){}
        QCOMPARE(order, QStringList() << QStringLiteral("saved") << QStringLiteral("read 1")
                                      << QStringLiteral("removed"));
        QCOMPARE(vault->calls, QStringList()
                 << QStringLiteral("write ") + Key(A, QStringLiteral("alice"))
                 << QStringLiteral("read ") + Key(A, QStringLiteral("alice"))
                 << QStringLiteral("remove ") + Key(A, QStringLiteral("alice")));
        QVERIFY(store->Usernames(S, A).isEmpty());
        delete store;
    }
};

QTEST_MAIN(tst_passwordstore)
#include "tst_passwordstore.moc"
