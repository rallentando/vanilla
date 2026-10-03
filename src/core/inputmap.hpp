#ifndef INPUTMAP_HPP
#define INPUTMAP_HPP

#include "switch.hpp"

#include "settingsio.hpp"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QKeySequence>

#include <functional>

namespace InputMap {

typedef QMap<QKeySequence, QString> KeyMap;
typedef QMap<QString, QString>      GestureMap;

class Hooks {

public:
    Hooks(){}
    explicit Hooks(bool (*valid)(QString)) : IsValidAction(valid) {}

    std::function<QKeySequence(QString)> ParseKeySequence;
    std::function<bool(QString)> IsValidAction;
    std::function<void(QString)> ReportInvalidDefault;

    QKeySequence Parse(const QString &name) const {
        return ParseKeySequence ? ParseKeySequence(name) : QKeySequence(name);
    }
    bool Valid(const QString &action) const { return IsValidAction ? IsValidAction(action) : true;}
    void ReportInvalid(const QString &action) const;
};

QString EscapeKeyName(const QString &name);
QString UnescapeKeyName(const QString &name);

QStringList NamesUnder(const SettingsIO::Map &settings, const QString &group);

bool IsSingleKey(const QKeySequence &seq);

QString CanonicalKeyName(const QString &name);
QString CanonicalMouseName(const QString &name);
QString CanonicalGestureName(const QString &name);

bool LoadKeyMap(const SettingsIO::Map &settings, const QString &group,
                KeyMap &map, const Hooks &hooks = Hooks());
bool LoadGestureMap(const SettingsIO::Map &settings, const QString &group,
                    GestureMap &map, const Hooks &hooks = Hooks());

void SaveKeyMap(SettingsIO::Map &settings, const QString &group, const KeyMap &map);
void SaveGestureMap(SettingsIO::Map &settings, const QString &group, const GestureMap &map);

}

#endif
