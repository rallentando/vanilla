#ifndef DOWNLOADHISTORY_HPP
#define DOWNLOADHISTORY_HPP

#include "switch.hpp"

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QString>
#include <QUrl>

class DownloadItem;

class DownloadHistory : public QObject {
    Q_OBJECT

public:
    enum State { Running, Completed, Cancelled, Failed, Interrupted };

    struct Entry {
        int id = 0;
        QUrl url;
        QString path;
        QDateTime started;
        QDateTime ended;
        State state = Running;
        qint64 received = 0;
        qint64 total = -1;
    };

    static const int Limit = 500;

    static DownloadHistory *Instance();

    explicit DownloadHistory(const QString &path, QObject *parent = nullptr);
    ~DownloadHistory();

    int Started(const QUrl &url, const QString &path, DownloadItem *item);
    void Progress(int id, qint64 received, qint64 total);
    void Ended(int id, State state);

    bool Remove(int id);
    void Clear();
    bool Cancel(int id);
    static bool CanOpen(const Entry &entry);
    bool Open(int id);
    bool ShowInFolder(int id);

    QList<Entry> Entries() const;
    const Entry *Find(int id) const;

    void SetClosing();

    static QString StateName(State state);

    static bool IsRunnable(const QString &path, bool byContent = true);

signals:
    void EntryStarted(int id);
    void Changed();
    void ListChanged();

private:
    Entry *FindMutable(int id);
    void Load();
    void Commit();
    void Trim();

    QString m_Path;
    QList<Entry> m_Entries;
    QHash<int, QPointer<DownloadItem>> m_Live;
    int m_NextId;
    bool m_Closing;
};

#endif
