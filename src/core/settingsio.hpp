#ifndef SETTINGSIO_HPP
#define SETTINGSIO_HPP

#include "switch.hpp"

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QByteArray>

#include <functional>

class QVariant;
class QJsonValue;

namespace SettingsIO {

typedef QSettings::SettingsMap Map;

class Hooks {

public:
    std::function<QStringList()> BackUpFiltersOf;
    std::function<QString()> BackUpPrepositionOf;
    std::function<void(QString)> RestoredFromBackUp;

    QStringList BackUpFilters() const { return BackUpFiltersOf ? BackUpFiltersOf() : QStringList();}
    QString BackUpPreposition() const { return BackUpPrepositionOf ? BackUpPrepositionOf() : QStringLiteral("~");}
    void ReportRestore(const QString &backup) const { if(RestoredFromBackUp) RestoredFromBackUp(backup);}
};

QJsonValue VariantToJson(const QVariant &var);
QVariant   JsonToVariant(const QJsonValue &val);

bool ReadJson(const QByteArray &json, Map &map);
QByteArray WriteJson(const Map &map);

bool ReadJsonFile(const QString &path, Map &map);
bool WriteJsonFile(const QString &path, const Map &map);

void Load(const QString &directory, const QString &filename, Map &map, const Hooks &hooks = Hooks());
bool Save(const QString &directory, const QString &filename, const Map &map, const Hooks &hooks = Hooks());

}

#endif
