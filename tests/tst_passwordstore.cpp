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
        QSet<QString> failRead;
        bool late = false;
        bool folds = false;
        bool FoldsCase() const override { return folds;}
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
                if(failRead.contains(key)){ done(PasswordStore::Failed, QString()); return;}
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
    QString KeyIn(const QString &space, const QString &origin, const QString &username){
        return QStringLiteral("p|") + space + QStringLiteral("|") + origin + QStringLiteral("|") + username;
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

    void aSaveIsWrittenOnlyOverWhatTheUserSaw(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        typedef PasswordStore P;
        P::SaveResult result = P::SaveFailed;
        auto save = [&](const QString &user, const QString &password, P::Expect expect){
            result = P::SaveFailed;
            store->SaveExpecting(S, A, user, password, expect, [&](P::SaveResult r){ result = r;});
        };
        const QString alice = QStringLiteral("alice");

        save(alice, QStringLiteral("one"), P::Absent);
        QCOMPARE(result, P::Saved);
        QCOMPARE(vault->secrets.value(Key(A, alice)), QStringLiteral("one"));
        QVERIFY(store->Has(S, A, alice));
        save(alice, QStringLiteral("two"), P::Absent);
        QCOMPARE(result, P::Changed);
        QCOMPARE(vault->secrets.value(Key(A, alice)), QStringLiteral("one"));
        save(alice, QStringLiteral("two"), P::Present);
        QCOMPARE(result, P::Saved);
        QCOMPARE(vault->secrets.value(Key(A, alice)), QStringLiteral("two"));
        save(alice, QString(), P::Present);
        QCOMPARE(result, P::Changed);
        save(alice, QString(), P::Absent);
        QCOMPARE(result, P::Changed);
        QCOMPARE(vault->secrets.value(Key(A, alice)), QStringLiteral("two"));
        const QString bob = QStringLiteral("bob");
        save(bob, QStringLiteral("b"), P::Present);
        QCOMPARE(result, P::Changed);
        QVERIFY(!vault->secrets.contains(Key(A, bob)));
        save(bob, QString(), P::Absent);
        QCOMPARE(result, P::Saved);
        QVERIFY(vault->secrets.contains(Key(A, bob)));
        QVERIFY(store->Has(S, A, bob));
        QVERIFY(!store->HasPassword(S, A, bob));
        QVERIFY(store->HasPassword(S, A, alice));
        {
            PasswordStore again(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                                IndexPath(), QStringLiteral("p"));
            QVERIFY(again.Has(S, A, bob));
            QVERIFY(!again.HasPassword(S, A, bob));
            QVERIFY(again.HasPassword(S, A, alice));
        }
        save(bob, QStringLiteral("b"), P::Present);
        QCOMPARE(result, P::Changed);
        QVERIFY(!store->HasPassword(S, A, bob));
        save(bob, QStringLiteral("b"), P::Bare);
        QCOMPARE(result, P::Saved);
        QVERIFY(store->HasPassword(S, A, bob));
        save(bob, QStringLiteral("c"), P::Bare);
        QCOMPARE(result, P::Changed);
        QCOMPARE(vault->secrets.value(Key(A, bob)), QStringLiteral("b"));
        const QString frank = QStringLiteral("frank");
        vault->secrets[Key(A, frank)] = QString();
        save(frank, QStringLiteral("f"), P::Absent);
        QCOMPARE(result, P::Changed);
        QVERIFY(store->Has(S, A, frank));
        QVERIFY(!store->HasPassword(S, A, frank));

        const QString carol = QStringLiteral("carol");
        vault->secrets[Key(A, carol)] = QStringLiteral("kept");
        QVERIFY(!store->Has(S, A, carol));
        save(carol, QString(), P::Absent);
        QCOMPARE(result, P::Changed);
        QCOMPARE(vault->secrets.value(Key(A, carol)), QStringLiteral("kept"));
        QVERIFY(store->Has(S, A, carol));
        vault->secrets.remove(Key(A, carol));
        save(carol, QStringLiteral("p"), P::Present);
        QCOMPARE(result, P::Changed);
        QVERIFY(!store->Has(S, A, carol));

        const QString dave = QStringLiteral("dave");
        vault->late = true;
        store->Save(S, A, dave, QStringLiteral("real"), [](bool){});
        save(dave, QString(), P::Absent);
        while(vault->ReleaseOne()){}
        vault->late = false;
        QCOMPARE(result, P::Changed);
        QCOMPARE(vault->secrets.value(Key(A, dave)), QStringLiteral("real"));
        delete store;
    }

    void aSaveIsNotWrittenWhenTheVaultCannotBeRead(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        vault->failRead << Key(A, QStringLiteral("erin"));
        PasswordStore::SaveResult result = PasswordStore::Saved;
        store->SaveExpecting(S, A, QStringLiteral("erin"), QStringLiteral("p"), PasswordStore::Absent,
                             [&](PasswordStore::SaveResult r){ result = r;});
        QCOMPARE(result, PasswordStore::SaveFailed);
        QVERIFY(!vault->secrets.contains(Key(A, QStringLiteral("erin"))));
        QVERIFY(!vault->calls.contains(QStringLiteral("write ") + Key(A, QStringLiteral("erin"))));
        delete store;
    }

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

    void aNeverSiteIsKeptInItsOwnFile(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        const QString neverPath = m_Dir.filePath(QStringLiteral("passwords-never.json"));
        QFile::remove(neverPath);
        QVERIFY(!store->IsNever(S, A));
        QVERIFY(store->SetNever(S, A, true));
        QVERIFY(store->SetNever(S, A, true));
        QVERIFY(store->IsNever(S, A));
        QVERIFY(!store->IsNever(S, B));
        QVERIFY(!store->IsNever(QStringLiteral("other"), A));
        QCOMPARE(store->NeverEntries().size(), 1);
        QVERIFY(store->SetNever(S, B, true));
        QVERIFY(!store->SetNever(S, QString(), true));
        QVERIFY(vault->calls.isEmpty());
        QVERIFY(!QFile::exists(IndexPath()));

        {
            QFile index(IndexPath());
            QVERIFY(index.open(QIODevice::WriteOnly));
            index.write("{\"version\":1,\"entries\":[]}");
        }
        PasswordStore again(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                            IndexPath(), QStringLiteral("p"));
        QVERIFY(again.IsNever(S, A));
        QVERIFY(again.IsNever(S, B));
        QVERIFY(again.SetNever(S, A, false));
        QVERIFY(!again.IsNever(S, A));
        PasswordStore third(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                            IndexPath(), QStringLiteral("p"));
        QVERIFY(!third.IsNever(S, A));
        QVERIFY(third.IsNever(S, B));

        QFile::remove(neverPath);
        QVERIFY(QDir(m_Dir.path()).mkdir(QStringLiteral("passwords-never.json")));
        PasswordStore blocked(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                              IndexPath(), QStringLiteral("p"));
        QVERIFY(!blocked.SetNever(S, A, true));
        QVERIFY(!blocked.IsNever(S, A));
        QVERIFY(QDir(m_Dir.path()).rmdir(QStringLiteral("passwords-never.json")));
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

    void aDeletedSpaceTakesItsPasswordsWithIt(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        const QString alice = QStringLiteral("alice");
        store->Save(QStringLiteral("gone"), A, alice, QStringLiteral("1"), nullptr);
        store->Save(QStringLiteral("kept"), A, alice, QStringLiteral("2"), nullptr);
        QVERIFY(store->SetNever(QStringLiteral("gone"), B, true));
        QVERIFY(store->SetNever(QStringLiteral("kept"), B, true));
        int failed = -1;
        store->RemoveSpace(QStringLiteral("gone"), [&](int n){ failed = n;});
        QCOMPARE(failed, 0);
        QVERIFY(store->Usernames(QStringLiteral("gone"), A).isEmpty());
        QVERIFY(!vault->secrets.contains(KeyIn(QStringLiteral("gone"), A, alice)));
        QVERIFY(!store->IsNever(QStringLiteral("gone"), B));
        QCOMPARE(store->Usernames(QStringLiteral("kept"), A), QStringList() << alice);
        QCOMPARE(vault->secrets.value(KeyIn(QStringLiteral("kept"), A, alice)), QStringLiteral("2"));
        QVERIFY(store->IsNever(QStringLiteral("kept"), B));
        delete store;
    }

    void aRenamedSpaceTakesItsPasswordsAlong(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        const QString alice = QStringLiteral("alice"), bob = QStringLiteral("bob");
        const QString from = QStringLiteral("old"), to = QStringLiteral("new"), other = QStringLiteral("other");
        store->Save(from, A, alice, QStringLiteral("1"), nullptr);
        store->Save(from, B, bob, QStringLiteral("2"), nullptr);
        store->Save(other, A, alice, QStringLiteral("3"), nullptr);
        QVERIFY(store->SetNever(from, B, true));

        vault->late = true;
        int stayed = -1;
        store->MoveSpace(from, to, [&](int n){ stayed = n;});
        QVERIFY(store->IsNever(to, B));
        QVERIFY(!store->IsNever(from, B));
        while(vault->ReleaseOne()){}

        QCOMPARE(stayed, 0);
        QCOMPARE(store->Usernames(to, A), QStringList() << alice);
        QCOMPARE(store->Usernames(to, B), QStringList() << bob);
        QVERIFY(store->Usernames(from, A).isEmpty());
        QCOMPARE(vault->secrets.value(KeyIn(to, A, alice)), QStringLiteral("1"));
        QCOMPARE(vault->secrets.value(KeyIn(to, B, bob)), QStringLiteral("2"));
        QVERIFY(!vault->secrets.contains(KeyIn(from, A, alice)));
        QVERIFY(!vault->secrets.contains(KeyIn(from, B, bob)));
        QCOMPARE(vault->secrets.value(KeyIn(other, A, alice)), QStringLiteral("3"));
        QCOMPARE(store->Usernames(other, A), QStringList() << alice);

        delete store;
        store = new PasswordStore(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                                  IndexPath(), QStringLiteral("p"), this);
        QCOMPARE(store->Usernames(to, A), QStringList() << alice);
        QVERIFY(store->Usernames(from, A).isEmpty());
        delete store;
    }

    void aSaveAskedBeforeTheRenameMovesWithIt(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        vault->late = true;
        const QString alice = QStringLiteral("alice");
        store->Save(QStringLiteral("old"), A, alice, QStringLiteral("1"), nullptr);
        int stayed = -1;
        store->MoveSpace(QStringLiteral("old"), QStringLiteral("new"), [&](int n){ stayed = n;});
        while(vault->ReleaseOne()){}
        QCOMPARE(stayed, 0);
        QCOMPARE(store->Usernames(QStringLiteral("new"), A), QStringList() << alice);
        QCOMPARE(vault->secrets.value(KeyIn(QStringLiteral("new"), A, alice)), QStringLiteral("1"));
        QVERIFY(!vault->secrets.contains(KeyIn(QStringLiteral("old"), A, alice)));
        delete store;
    }

    void aPasswordWhichCouldNotBeMovedStaysWhereItWas(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        const QString alice = QStringLiteral("alice"), bob = QStringLiteral("bob"), carol = QStringLiteral("carol");
        const QString from = QStringLiteral("old"), to = QStringLiteral("new");
        store->Save(from, A, alice, QStringLiteral("1"), nullptr);
        store->Save(from, A, bob, QStringLiteral("2"), nullptr);
        store->Save(from, A, carol, QStringLiteral("3"), nullptr);
        vault->failRead << KeyIn(from, A, alice);
        vault->failWrite << KeyIn(to, A, bob);
        vault->secrets.remove(KeyIn(from, A, carol));

        int stayed = -1;
        store->MoveSpace(from, to, [&](int n){ stayed = n;});
        QCOMPARE(stayed, 2);
        QCOMPARE(store->Usernames(from, A), QStringList() << alice << bob);
        QVERIFY(store->Usernames(to, A).isEmpty());
        QCOMPARE(vault->secrets.value(KeyIn(from, A, alice)), QStringLiteral("1"));
        QCOMPARE(vault->secrets.value(KeyIn(from, A, bob)), QStringLiteral("2"));
        delete store;
    }

    void aRenameOfCaseOnlyLeavesAFoldingVaultAlone(){
        const QString alice = QStringLiteral("alice");
        for(bool folds : {true, false}){
            FakeVault *vault;
            PasswordStore *store = Make(&vault);
            vault->folds = folds;
            store->Save(QStringLiteral("Work"), A, alice, QStringLiteral("1"), nullptr);
            vault->calls.clear();
            int stayed = -1;
            store->MoveSpace(QStringLiteral("Work"), QStringLiteral("work"), [&](int n){ stayed = n;});
            QCOMPARE(stayed, 0);
            QCOMPARE(store->Usernames(QStringLiteral("work"), A), QStringList() << alice);
            QVERIFY(store->Usernames(QStringLiteral("Work"), A).isEmpty());
            if(folds){
                QVERIFY(vault->calls.isEmpty());
            } else {
                QCOMPARE(vault->secrets.value(KeyIn(QStringLiteral("work"), A, alice)), QStringLiteral("1"));
                QVERIFY(!vault->secrets.contains(KeyIn(QStringLiteral("Work"), A, alice)));
            }
            delete store;
        }
    }

    void aRenameKeepsTheOldKeyWhenTheIndexCannotBeWritten(){
        for(bool folds : {false, true}){
            FakeVault *vault;
            PasswordStore *store = Make(&vault);
            vault->folds = folds;
            const QString from = QStringLiteral("Case"), to = QStringLiteral("case");
            const QString user = QStringLiteral("alice");
            store->Save(from, A, user, QStringLiteral("secret"), nullptr);
            const QString kept = IndexPath() + QStringLiteral(".kept");
            QVERIFY(QFile::rename(IndexPath(), kept));
            QVERIFY(QDir().mkdir(IndexPath()));
            int failed = -1;
            store->MoveSpace(from, to, [&](int n){ failed = n;});
            QCOMPARE(failed, 1);
            QVERIFY(store->Has(from, A, user));
            QVERIFY(!store->Has(to, A, user));
            QCOMPARE(vault->secrets.value(KeyIn(from, A, user)), QStringLiteral("secret"));
            QVERIFY(!vault->calls.contains(QStringLiteral("remove ") + KeyIn(from, A, user)));
            QVERIFY(QDir().rmdir(IndexPath()));
            QVERIFY(QFile::rename(kept, IndexPath()));
            delete store;
            store = new PasswordStore(std::unique_ptr<PasswordStore::Vault>(new FakeVault()),
                                      IndexPath(), QStringLiteral("p"), this);
            QVERIFY(store->Has(from, A, user));
            QVERIFY(!store->Has(to, A, user));
            delete store;
        }
    }

    void aRenameDoesNotAdoptAnUnindexedDestinationKey(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        const QString from = QStringLiteral("retry-old"), to = QStringLiteral("retry-new");
        const QString user = QStringLiteral("alice");
        store->Save(from, A, user, QStringLiteral("newer source"), nullptr);
        vault->secrets[KeyIn(to, A, user)] = QStringLiteral("stale destination");
        int failed = -1;
        store->MoveSpace(from, to, [&](int n){ failed = n;});
        QCOMPARE(failed, 1);
        QVERIFY(store->Has(from, A, user));
        QVERIFY(!store->Has(to, A, user));
        QCOMPARE(vault->secrets.value(KeyIn(from, A, user)), QStringLiteral("newer source"));
        QCOMPARE(vault->secrets.value(KeyIn(to, A, user)), QStringLiteral("stale destination"));
        delete store;
    }

    void aRenameReportsANeverListWhichCouldNotBeWritten(){
        FakeVault *vault;
        PasswordStore *store = Make(&vault);
        const QString from = QStringLiteral("never-old"), to = QStringLiteral("never-new");
        QVERIFY(store->SetNever(from, A, true));
        const QString path = m_Dir.filePath(QStringLiteral("passwords-never.json"));
        const QString kept = path + QStringLiteral(".kept");
        QVERIFY(QFile::rename(path, kept));
        QVERIFY(QDir().mkdir(path));
        int failed = -1;
        store->MoveSpace(from, to, [&](int n){ failed = n;});
        QCOMPARE(failed, 1);
        QVERIFY(store->IsNever(from, A));
        QVERIFY(!store->IsNever(to, A));
        QVERIFY(QDir().rmdir(path));
        QVERIFY(QFile::rename(kept, path));
        delete store;
    }
};

QTEST_MAIN(tst_passwordstore)
#include "tst_passwordstore.moc"
