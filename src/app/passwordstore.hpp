#ifndef PASSWORDSTORE_HPP
#define PASSWORDSTORE_HPP

#include "switch.hpp"

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

class PasswordStore : public QObject {
    Q_OBJECT

public:
    enum Result { Ok, NotFound, Failed };

    class Vault {
    public:
        virtual ~Vault(){}
        virtual void Write(const QString &key, const QString &secret,
                           std::function<void(Result)> done) = 0;
        virtual void Read(const QString &key,
                          std::function<void(Result, const QString&)> done) = 0;
        virtual void Remove(const QString &key, std::function<void(Result)> done) = 0;
        virtual bool FoldsCase() const { return false;}
    };

    struct Entry {
        QString space;
        QString origin;
        QString username;
        bool passwordless = false;
        bool operator==(const Entry &other) const {
            return space == other.space && origin == other.origin && username == other.username;
        }
    };

    static PasswordStore *Instance();

    PasswordStore(std::unique_ptr<Vault> vault, const QString &indexPath,
                  const QString &prefix, QObject *parent = nullptr);
    ~PasswordStore();

    QStringList Usernames(const QString &space, const QString &origin) const;
    bool Has(const QString &space, const QString &origin, const QString &username) const;
    bool HasPassword(const QString &space, const QString &origin, const QString &username) const;
    QList<Entry> Entries() const { return m_Entries;}

    void Save(const QString &space, const QString &origin, const QString &username,
              const QString &password, std::function<void(bool)> done);

    enum Expect { Absent, Bare, Present };
    enum SaveResult { Saved, Changed, SaveFailed };
    void SaveExpecting(const QString &space, const QString &origin, const QString &username,
                       const QString &password, Expect expect,
                       std::function<void(SaveResult)> done);

    void Read(const QString &space, const QString &origin, const QString &username,
              bool mayPrune, std::function<void(bool, const QString&)> done);

    void RemoveAll(std::function<void(int)> done);
    void RemoveSpace(const QString &space, std::function<void(int)> done);

    void Remove(const QString &space, const QString &origin, const QString &username,
                std::function<void(Result)> done);
    int PendingRemovals() const { return m_PendingRemovals;}

    bool IsNever(const QString &space, const QString &origin) const;
    QList<Entry> NeverEntries() const { return m_Never;}
    bool SetNever(const QString &space, const QString &origin, bool never);

    void MoveSpace(const QString &from, const QString &to, std::function<void(int)> done);

private:
    QString Key(const Entry &entry) const;
    void Load();
    void Mark(const Entry &entry, bool passwordless);
    void LoadNever();
    bool CommitNever() const;
    bool Commit();

    using Step = std::function<void(std::function<void()> next)>;
    void Enqueue(Step step);
    void RunNext();

    struct RemoveWalk {
        QList<Entry> rest;
        int failed = 0;
        bool running = false;
        bool again = false;
        std::function<void(int)> done;
        std::function<void()> next;
    };
    void RemoveNext(std::shared_ptr<RemoveWalk> walk);

    struct MoveWalk {
        QString to;
        QList<Entry> rest;
        int failed = 0;
        bool running = false;
        bool again = false;
        std::function<void(int)> done;
        std::function<void()> next;
    };
    void MoveNext(std::shared_ptr<MoveWalk> walk);
    bool Rename(const Entry &entry, const QString &to);

    std::unique_ptr<Vault> m_Vault;
    QString m_IndexPath;
    QString m_NeverPath;
    QList<Entry> m_Never;
    QString m_Prefix;
    QList<Entry> m_Entries;
    QList<Step> m_Queue;
    bool m_Running;
    int m_PendingRemovals;
};

#endif
