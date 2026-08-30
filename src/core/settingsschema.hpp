#ifndef SETTINGSSCHEMA_HPP
#define SETTINGSSCHEMA_HPP

#include "switch.hpp"

#include <QString>
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

    const char *appliesWhenKey;
    bool appliesWhen;
};

const QList<Item> &Items();

const Item *Find(const QString &key);

QList<QPair<QString, QString> > Categories();

QVariant Fallback(const Item &item);
QVariant FromJson(const Item &item, const QJsonValue &value);
QJsonValue ToJson(const Item &item, const QVariant &value);

QJsonObject PageStrings();

QJsonObject Describe();

}

#endif
