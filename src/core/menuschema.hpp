#ifndef MENUSCHEMA_HPP
#define MENUSCHEMA_HPP

#include "switch.hpp"

#include "settingsio.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace MenuSchema {

struct Table {
    const char *name;
    const char *key;
    const char *label;
    const char *hint;
    QStringList specials;
    QString defaults;
    QStringList vocabulary = QStringList();
    bool icons = false;
};

const Table *IconsOf(const QString &lines);

const QStringList &NodeNames();

const QList<Table> &Tables();
const Table *Find(const QString &name);
QString Key(const Table &table);
QString Default(const QString &name);

QStringList Current(const SettingsIO::Map &settings, const Table &table);
bool IsKnown(const Table &table, const QString &name);
QString Label(const QString &name);

QString Write(SettingsIO::Map &settings, const Table &table, const QJsonArray &names);
void Reset(SettingsIO::Map &settings, const Table &table);

QJsonObject DescribeTable(const SettingsIO::Map &settings, const Table &table);
QJsonObject Describe(const SettingsIO::Map &settings);

}

#endif
