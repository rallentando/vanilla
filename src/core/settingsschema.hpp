#ifndef SETTINGSSCHEMA_HPP
#define SETTINGSSCHEMA_HPP

#include "switch.hpp"

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVariant>
#include <QJsonObject>
#include <QJsonValue>

namespace SettingsSchema {

enum Type {
    Bool,
    Int,
    Text,
    Choice,
    Directory,
    TextList,
    Color,
};

struct Item {
    const char *key;
    Type type;
    const char *category;
    const char *label;
    const char *hint;
    const char *choices;
    const char *fallback;
    bool needsRestart;

    const char *appliesWhen;

    const char *picker;
};

const QList<Item> &Items();

struct Group {
    const char *key;
    const char *label;
};

const QList<Group> &Groups();

const Item *Find(const QString &key);

QList<QPair<QString, QString> > AppliesWhenClauses(const Item &item);

QList<QPair<QString, QString> > Categories();

QStringList CategoriesBeginningARun();

struct ChoiceLabel {
    const char *key;
    const char *choice;
    const char *label;
};
const QList<ChoiceLabel> &ChoiceLabels();

QVariant Fallback(const Item &item);
QVariant FromJson(const Item &item, const QJsonValue &value);
QJsonValue ToJson(const Item &item, const QVariant &value);

QString EmptyDirectory(const Item &item);

QJsonObject PageStrings();

QJsonObject Describe();

QStringList UnlistedKeys();
QStringList UnlistedPrefixes();
bool IsKnownKey(const QString &key);
QStringList DropUnknownKeys(QMap<QString, QVariant> &map);

}

#endif
