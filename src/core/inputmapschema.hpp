#ifndef INPUTMAPSCHEMA_HPP
#define INPUTMAPSCHEMA_HPP

#include "switch.hpp"

#include "settingsio.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace InputMapSchema {

enum Kind {
    Keys,
    Mouse,
    Gesture,
};

enum Vocabulary {
    PageActions,
    TreeBankActions,
    GadgetsActions,
};

struct Table {
    const char *group;
    Kind kind;
    Vocabulary vocabulary;
    const char *label;
    const char *hint;
    bool singleKeysHeld;
};

const QList<Table> &Tables();
const Table *Find(const QString &group);

QStringList Actions(Vocabulary vocabulary);
bool IsAction(Vocabulary vocabulary, const QString &action);
QString ActionLabel(Vocabulary vocabulary, const QString &action);

QString Placeholder();

QList<QPair<QString, QString> > Defaults(const Table &table);
QList<QPair<QString, QString> > Current(const SettingsIO::Map &settings, const Table &table);

QString Write(SettingsIO::Map &settings, const Table &table, const QJsonArray &entries);
void Reset(SettingsIO::Map &settings, const Table &table);

QJsonObject DescribeTable(const SettingsIO::Map &settings, const Table &table);
QJsonObject Describe(const SettingsIO::Map &settings);

}

#endif
