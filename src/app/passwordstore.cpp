#include "switch.hpp"

#include "passwordstore.hpp"

#include "passwordpolicy.hpp"
#include "application.hpp"

#include <QCryptographicHash>
#include <QFile>
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
    , m_Prefix(prefix)
    , m_Running(false)
    , m_PendingRemovals(0)
{
    Load();
}

PasswordStore::~PasswordStore(){}

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

void PasswordStore::Save(const QString &space, const QString &origin, const QString &username,
                         const QString &password, std::function<void(bool)> done){
    const Entry entry{space, origin, username};
    QPointer<PasswordStore> self(this);
    Enqueue([self, entry, password, done](std::function<void()> next){
        self->m_Vault->Write(self->Key(entry), password, [self, entry, done, next](Result result){
            if(!self) return;
            const bool ok = result == Ok;
            if(ok && !self->m_Entries.contains(entry)){
                self->m_Entries << entry;
                self->Commit();
            }
            if(done) done(ok);
            next();
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

void PasswordStore::Load(){
    m_Entries.clear();
    QFile file(m_IndexPath);
    if(!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    foreach(const QJsonValue &value, doc.object().value(QStringLiteral("entries")).toArray()){
        const QJsonObject obj = value.toObject();
        const Entry entry{obj.value(QStringLiteral("space")).toString(),
                          obj.value(QStringLiteral("origin")).toString(),
                          obj.value(QStringLiteral("username")).toString()};
        if(entry.origin.isEmpty() || m_Entries.contains(entry)) continue;
        m_Entries << entry;
    }
}

bool PasswordStore::Commit(){
    QJsonArray entries;
    foreach(const Entry &entry, m_Entries){
        QJsonObject obj;
        obj.insert(QStringLiteral("space"), entry.space);
        obj.insert(QStringLiteral("origin"), entry.origin);
        obj.insert(QStringLiteral("username"), entry.username);
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
