#ifndef EXTENSIONALARMS_HPP
#define EXTENSIONALARMS_HPP

#include "switch.hpp"

#include <functional>
#include <QHash>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>

namespace ExtensionAlarms {

const int MOST = 500;
const double DUE_KEPT_MS = 60 * 60 * 1000;

class Book {
public:
    void Create(const QString &id, const QJsonArray &args, double now, QString *error);
    QJsonValue Get(const QString &id, const QJsonArray &args, QString *error) const;
    QJsonArray GetAll(const QString &id) const;
    bool Clear(const QString &id, const QJsonArray &args, QString *error);
    bool ClearAll(const QString &id);

    bool Advance(double now, const std::function<bool(const QString &)> &mayRing);
    QJsonArray Take(const QString &id);
    bool HasAlarms(const QString &id) const;
    QStringList WithDue() const;
    double NextAt(const std::function<bool(const QString &)> &mayRing) const;
    void Drop(const QString &id);

    QByteArray ToJson() const;
    static Book FromJson(const QByteArray &bytes);

private:
    struct Alarm {
        QString name;
        double scheduledTime = 0;
        double periodInMinutes = -1;
        double since = 0;
    };
    struct Of {
        QList<Alarm> alarms;
        QList<Alarm> due;
    };
    QHash<QString, Of> m_Of;
    void DropIfEmpty(const QString &id);
};

QString StoreFileName(const QString &profileKey);

}

#endif
