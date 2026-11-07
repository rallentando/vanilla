#ifndef RUNNINGUPLOADS_HPP
#define RUNNINGUPLOADS_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QUrl>

#include <functional>

class View;

class RunningUploads : public QObject {
    Q_OBJECT

public:
    struct Entry {
        int id = 0;
        QUrl url;
    };

    static RunningUploads *Instance();

    explicit RunningUploads(int delay, QObject *parent = nullptr);

    static bool IsMultipart(const QByteArray &contentType);

    void Started(const View *key, const QUrl &url, const std::function<void()> &stop);
    bool Ended(const View *key);
    void Cancel(int id);

    QList<Entry> Shown() const;

signals:
    void Appeared();
    void Changed();

private:
    struct Running {
        int id = 0;
        const View *key = nullptr;
        QUrl url;
        std::function<void()> stop;
        bool shown = false;
    };

    const int m_Delay;
    int m_NextId;
    QList<Running> m_Running;
};

#endif
