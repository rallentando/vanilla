#include "switch.hpp"

#include "passwordstore.hpp"

#include "passwordpolicy.hpp"
#include "application.hpp"

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSaveFile>

#ifdef VANILLA_PASSWORDS
#  include "keychain.h"
#endif

namespace {

#ifdef VANILLA_PASSWORDS
    PasswordStore::Result FromJob(QKeychain::Error error){
        switch(error){
        case QKeychain::NoError:       return PasswordStore::Ok;
        case QKeychain::EntryNotFound: return PasswordStore::NotFound;
        default:                       return PasswordStore::Failed;
        }
    }

    class KeychainVault : public PasswordStore::Vault {
    public:
        void Write(const QString &key, const QString &secret,
                   std::function<void(PasswordStore::Result)> done) override {
            QKeychain::WritePasswordJob *job = new QKeychain::WritePasswordJob(Service());
            job->setInsecureFallback(false);
            job->setKey(key);
            job->setTextData(secret);
            QObject::connect(job, &QKeychain::Job::finished, [done](QKeychain::Job *job){
                done(FromJob(job->error()));
            });
            job->start();
        }
        void Read(const QString &key,
                  std::function<void(PasswordStore::Result, const QString&)> done) override {
            QKeychain::ReadPasswordJob *job = new QKeychain::ReadPasswordJob(Service());
            job->setInsecureFallback(false);
            job->setKey(key);
            QObject::connect(job, &QKeychain::Job::finished, [done, job](QKeychain::Job*){
                const PasswordStore::Result result = FromJob(job->error());
                done(result, result == PasswordStore::Ok ? job->textData() : QString());
            });
            job->start();
        }
        void Remove(const QString &key,
                    std::function<void(PasswordStore::Result)> done) override {
            QKeychain::DeletePasswordJob *job = new QKeychain::DeletePasswordJob(Service());
            job->setInsecureFallback(false);
            job->setKey(key);
            QObject::connect(job, &QKeychain::Job::finished, [done](QKeychain::Job *job){
                done(FromJob(job->error()));
            });
            job->start();
        }
#ifdef Q_OS_WIN
        bool FoldsCase() const override { return true;}
#endif
    private:
        static QString Service(){ return QStringLiteral("vanilla");}
    };
#endif
}

PasswordStore *PasswordStore::Instance(){
#ifdef VANILLA_PASSWORDS
    static PasswordStore *store = new PasswordStore
        (std::unique_ptr<Vault>(new KeychainVault()),
         Application::StateDirectory() + QStringLiteral("passwords-index.json"),
         QStringLiteral("vanilla-") +
         QString::fromLatin1(QCryptographicHash::hash(Application::applicationDirPath().toUtf8(),
                                                      QCryptographicHash::Md5).toHex()),
         Application::GetInstance());
    return store;
#else
    return nullptr;
#endif
}

PasswordStore::PasswordStore(std::unique_ptr<Vault> vault, const QString &indexPath,
                             const QString &prefix, QObject *parent)
    : QObject(parent)
    , m_Vault(std::move(vault))
    , m_IndexPath(indexPath)
    , m_NeverPath(QFileInfo(indexPath).absolutePath() + QStringLiteral("/passwords-never.json"))
    , m_Prefix(prefix)
    , m_Running(false)
    , m_PendingRemovals(0)
{
    Load();
    LoadNever();
}

PasswordStore::~PasswordStore(){}

bool PasswordStore::IsNever(const QString &space, const QString &origin) const {
    return m_Never.contains(Entry{space, origin, QString()});
}

bool PasswordStore::SetNever(const QString &space, const QString &origin, bool never){
    if(origin.isEmpty()) return false;
    const Entry entry{space, origin, QString()};
    if(m_Never.contains(entry) == never) return true;
    const QList<Entry> before = m_Never;
    if(never) m_Never << entry;
    else      m_Never.removeAll(entry);
    if(CommitNever()) return true;
    m_Never = before;
    return false;
}

void PasswordStore::LoadNever(){
    m_Never.clear();
    QFile file(m_NeverPath);
    if(!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    for(const QJsonValue &value : doc.object().value(QStringLiteral("never")).toArray()){
        const QJsonObject obj = value.toObject();
        const Entry entry{obj.value(QStringLiteral("space")).toString(),
                          obj.value(QStringLiteral("origin")).toString(), QString()};
        if(entry.origin.isEmpty() || m_Never.contains(entry)) continue;
        m_Never << entry;
    }
}

bool PasswordStore::CommitNever() const {
    QJsonArray never;
    foreach(const Entry &entry, m_Never){
        QJsonObject obj;
        obj.insert(QStringLiteral("space"), entry.space);
        obj.insert(QStringLiteral("origin"), entry.origin);
        never.append(obj);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("never"), never);
    QSaveFile file(m_NeverPath);
    if(!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    return file.commit();
}

QString PasswordStore::Key(const Entry &entry) const {
    return PasswordPolicy::VaultKey(m_Prefix, entry.space, entry.origin, entry.username);
}

QStringList PasswordStore::Usernames(const QString &space, const QString &origin) const {
    QStringList list;
    foreach(const Entry &entry, m_Entries){
        if(entry.space == space && entry.origin == origin) list << entry.username;
    }
    return list;
}

bool PasswordStore::Has(const QString &space, const QString &origin, const QString &username) const {
    return m_Entries.contains(Entry{space, origin, username});
}

bool PasswordStore::HasPassword(const QString &space, const QString &origin, const QString &username) const {
    const int at = m_Entries.indexOf(Entry{space, origin, username});
    return at >= 0 && !m_Entries.at(at).passwordless;
}

void PasswordStore::Save(const QString &space, const QString &origin, const QString &username,
                         const QString &password, std::function<void(bool)> done){
    const Entry entry{space, origin, username};
    QPointer<PasswordStore> self(this);
    Enqueue([self, entry, password, done](std::function<void()> next){
        self->m_Vault->Write(self->Key(entry), password, [self, entry, password, done, next](Result result){
            if(!self) return;
            const bool ok = result == Ok;
            if(ok) self->Mark(entry, password.isEmpty());
            if(done) done(ok);
            next();
        });
    });
}

void PasswordStore::SaveExpecting(const QString &space, const QString &origin,
                                  const QString &username, const QString &password,
                                  Expect expect, std::function<void(SaveResult)> done){
    const Entry entry{space, origin, username};
    QPointer<PasswordStore> self(this);
    Enqueue([self, entry, password, expect, done](std::function<void()> next){
        self->m_Vault->Read(self->Key(entry), [self, entry, password, expect, done, next]
                            (Result read, const QString &secret){
            if(!self) return;
            if(read == Failed){
                if(done) done(SaveFailed);
                next();
                return;
            }
            const Expect found = read != Ok ? Absent : secret.isEmpty() ? Bare : Present;
            if(found != expect || (password.isEmpty() && found != Absent)){
                if(found == Absent){
                    if(self->m_Entries.removeAll(entry)) self->Commit();
                } else {
                    self->Mark(entry, found == Bare);
                }
                if(done) done(Changed);
                next();
                return;
            }
            self->m_Vault->Write(self->Key(entry), password, [self, entry, password, done, next](Result result){
                if(!self) return;
                const bool ok = result == Ok;
                if(ok) self->Mark(entry, password.isEmpty());
                if(done) done(ok ? Saved : SaveFailed);
                next();
            });
        });
    });
}

void PasswordStore::Read(const QString &space, const QString &origin, const QString &username,
                         bool mayPrune, std::function<void(bool, const QString&)> done){
    const Entry entry{space, origin, username};
    QPointer<PasswordStore> self(this);
    Enqueue([self, entry, mayPrune, done](std::function<void()> next){
        self->m_Vault->Read(self->Key(entry), [self, entry, mayPrune, done, next]
                            (Result result, const QString &password){
            if(!self) return;
            if(result == NotFound && mayPrune && self->m_Entries.removeAll(entry))
                self->Commit();
            if(done) done(result == Ok, password);
            next();
        });
    });
}

void PasswordStore::RemoveAll(std::function<void(int)> done){
    m_PendingRemovals++;
    QPointer<PasswordStore> self(this);
    Enqueue([self, done](std::function<void()> next){
        std::shared_ptr<RemoveWalk> walk(new RemoveWalk);
        walk->rest = self->m_Entries;
        walk->done = done;
        walk->next = next;
        self->RemoveNext(walk);
    });
}

void PasswordStore::RemoveSpace(const QString &space, std::function<void(int)> done){
    const QList<Entry> beforeNever = m_Never;
    for(int i = m_Never.size() - 1; i >= 0; i--)
        if(m_Never.at(i).space == space) m_Never.removeAt(i);
    if(m_Never.size() != beforeNever.size() && !CommitNever()) m_Never = beforeNever;
    m_PendingRemovals++;
    QPointer<PasswordStore> self(this);
    Enqueue([self, space, done](std::function<void()> next){
        std::shared_ptr<RemoveWalk> walk(new RemoveWalk);
        foreach(const Entry &entry, self->m_Entries)
            if(entry.space == space) walk->rest << entry;
        walk->done = done;
        walk->next = next;
        self->RemoveNext(walk);
    });
}

void PasswordStore::RemoveNext(std::shared_ptr<RemoveWalk> walk){
    if(walk->running){
        walk->again = true;
        return;
    }
    walk->running = true;
    do {
        walk->again = false;
        if(walk->rest.isEmpty()){
            Commit();
            m_PendingRemovals--;
            walk->running = false;
            if(walk->done) walk->done(walk->failed);
            walk->next();
            return;
        }
        const Entry entry = walk->rest.takeFirst();
        QPointer<PasswordStore> self(this);
        m_Vault->Remove(Key(entry), [self, walk, entry](Result result){
            if(!self) return;
            if(result == Failed) walk->failed++;
            else self->m_Entries.removeAll(entry);
            self->RemoveNext(walk);
        });
    } while(walk->again);
    walk->running = false;
}

void PasswordStore::Remove(const QString &space, const QString &origin, const QString &username,
                           std::function<void(Result)> done){
    const Entry entry{space, origin, username};
    m_PendingRemovals++;
    QPointer<PasswordStore> self(this);
    Enqueue([self, entry, done](std::function<void()> next){
        if(!self->m_Entries.contains(entry)){
            self->m_PendingRemovals--;
            if(done) done(NotFound);
            next();
            return;
        }
        self->m_Vault->Remove(self->Key(entry), [self, entry, done, next](Result result){
            if(!self) return;
            if(result != Failed && self->m_Entries.removeAll(entry)) self->Commit();
            self->m_PendingRemovals--;
            if(done) done(result);
            next();
        });
    });
}

void PasswordStore::MoveSpace(const QString &from, const QString &to, std::function<void(int)> done){
    if(from == to){
        if(done) done(0);
        return;
    }
    const QList<Entry> beforeNever = m_Never;
    int never = 0;
    for(int i = m_Never.size() - 1; i >= 0; i--){
        if(m_Never.at(i).space != from) continue;
        const Entry moved{to, m_Never.at(i).origin, QString()};
        if(m_Never.contains(moved)) m_Never.removeAt(i);
        else m_Never[i] = moved;
        never++;
    }
    const int neverFailed = never && !CommitNever() ? never : 0;
    if(neverFailed) m_Never = beforeNever;

    const bool indexOnly = m_Vault->FoldsCase() && from.compare(to, Qt::CaseInsensitive) == 0;
    QPointer<PasswordStore> self(this);
    Enqueue([self, from, to, indexOnly, neverFailed, done](std::function<void()> next){
        std::shared_ptr<MoveWalk> walk(new MoveWalk);
        walk->to = to;
        walk->failed = neverFailed;
        foreach(const Entry &entry, self->m_Entries)
            if(entry.space == from) walk->rest << entry;
        walk->done = done;
        walk->next = next;
        if(indexOnly){
            foreach(const Entry &entry, walk->rest)
                if(!self->Rename(entry, to)) walk->failed++;
            walk->rest.clear();
        }
        self->MoveNext(walk);
    });
}

bool PasswordStore::Rename(const Entry &entry, const QString &to){
    const int at = m_Entries.indexOf(entry);
    if(at < 0) return true;
    const QList<Entry> before = m_Entries;
    Entry moved = m_Entries.at(at);
    moved.space = to;
    if(m_Entries.contains(moved)) m_Entries.removeAt(at);
    else m_Entries[at] = moved;
    if(Commit()) return true;
    m_Entries = before;
    return false;
}

void PasswordStore::MoveNext(std::shared_ptr<MoveWalk> walk){
    if(walk->running){
        walk->again = true;
        return;
    }
    walk->running = true;
    do {
        walk->again = false;
        if(walk->rest.isEmpty()){
            walk->running = false;
            if(walk->done) walk->done(walk->failed);
            walk->next();
            return;
        }
        const Entry entry = walk->rest.takeFirst();
        Entry moved = entry;
        moved.space = walk->to;
        QPointer<PasswordStore> self(this);
        auto finish = [self, walk, entry](){
            if(!self) return;
            if(!self->Rename(entry, walk->to)){
                walk->failed++;
                self->MoveNext(walk);
                return;
            }
            self->m_Vault->Remove(self->Key(entry), [self, walk](Result result){
                if(!self) return;
                if(result == Failed) qWarning() << "passwords: an old entry of a renamed space could not be removed";
                self->MoveNext(walk);
            });
        };
        m_Vault->Read(Key(moved), [self, walk, entry, moved, finish](Result there, const QString&){
            if(!self) return;
            if(there == Failed || (there == Ok && !self->m_Entries.contains(moved))){
                walk->failed++; self->MoveNext(walk); return;
            }
            if(there == Ok){ finish(); return;}
            self->m_Vault->Read(self->Key(entry), [self, walk, entry, moved, finish](Result read, const QString &secret){
                if(!self) return;
                if(read == Failed){ walk->failed++; self->MoveNext(walk); return;}
                if(read == NotFound){
                    const QList<Entry> before = self->m_Entries;
                    if(self->m_Entries.removeAll(entry) && !self->Commit()){
                        self->m_Entries = before;
                        walk->failed++;
                    }
                    self->MoveNext(walk);
                    return;
                }
                self->m_Vault->Write(self->Key(moved), secret, [self, walk, finish](Result written){
                    if(!self) return;
                    if(written != Ok){ walk->failed++; self->MoveNext(walk); return;}
                    finish();
                });
            });
        });
    } while(walk->again);
    walk->running = false;
}

void PasswordStore::Load(){
    m_Entries.clear();
    QFile file(m_IndexPath);
    if(!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    for(const QJsonValue &value : doc.object().value(QStringLiteral("entries")).toArray()){
        const QJsonObject obj = value.toObject();
        Entry entry{obj.value(QStringLiteral("space")).toString(),
                    obj.value(QStringLiteral("origin")).toString(),
                    obj.value(QStringLiteral("username")).toString()};
        entry.passwordless = obj.value(QStringLiteral("passwordless")).toBool();
        if(entry.origin.isEmpty() || m_Entries.contains(entry)) continue;
        m_Entries << entry;
    }
}

void PasswordStore::Mark(const Entry &entry, bool passwordless){
    const int at = m_Entries.indexOf(entry);
    if(at < 0){
        Entry added = entry;
        added.passwordless = passwordless;
        m_Entries << added;
    } else if(m_Entries.at(at).passwordless != passwordless){
        m_Entries[at].passwordless = passwordless;
    } else {
        return;
    }
    Commit();
}

bool PasswordStore::Commit(){
    QJsonArray entries;
    foreach(const Entry &entry, m_Entries){
        QJsonObject obj;
        obj.insert(QStringLiteral("space"), entry.space);
        obj.insert(QStringLiteral("origin"), entry.origin);
        obj.insert(QStringLiteral("username"), entry.username);
        if(entry.passwordless) obj.insert(QStringLiteral("passwordless"), true);
        entries.append(obj);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("entries"), entries);

    QSaveFile file(m_IndexPath);
    if(!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    return file.commit();
}

void PasswordStore::Enqueue(Step step){
    m_Queue << step;
    if(!m_Running) RunNext();
}

void PasswordStore::RunNext(){
    if(m_Queue.isEmpty()){
        m_Running = false;
        return;
    }
    m_Running = true;
    const Step step = m_Queue.takeFirst();
    QPointer<PasswordStore> self(this);
    step([self](){ if(self) self->RunNext();});
}
