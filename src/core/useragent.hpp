#ifndef USERAGENT_HPP
#define USERAGENT_HPP

#include "switch.hpp"

#include "settingsio.hpp"

#include <QMap>
#include <QString>
#include <QVector>

namespace UserAgent {

struct Entry {
    const char *spelling;
    const char *name;
    const char *fallback;
};

const QVector<Entry> &Entries();

QString NameOf(const QString &value);

const Entry *Find(const QString &name);

QString Fallback(const QString &name);

QString SettingsKey(const QString &name);

QString Expand(QString tmpl, const QString &system,
               const QString &location, const QString &chromium);

typedef QMap<QString, QString> Map;

Map Load(const SettingsIO::Map &settings);
void Save(SettingsIO::Map &settings, const Map &agents);

}

#endif
