#include "switch.hpp"
#include "const.hpp"

#include "inputmap.hpp"


namespace InputMap {

namespace {

const QString SLASH     = QStringLiteral("/");
const QString BACKSLASH = QStringLiteral("\\");
const QString SLASH_WORD     = QStringLiteral("Slash");
const QString BACKSLASH_WORD = QStringLiteral("Backslash");
const QString NO_ACTION = QStringLiteral("NoAction");

QString Entry(const QString &group, const QString &name){
    return group + SLASH + name;
}

}

void Hooks::ReportInvalid(const QString &action) const {
    if(ReportInvalidDefault) ReportInvalidDefault(action);
}

QString EscapeKeyName(const QString &name){
    QString escaped = name;
    escaped.replace(BACKSLASH, BACKSLASH_WORD);
    escaped.replace(SLASH, SLASH_WORD);
    return escaped;
}

QString UnescapeKeyName(const QString &name){
    QString plain = name;
    plain.replace(SLASH_WORD, SLASH);
    plain.replace(BACKSLASH_WORD, BACKSLASH);
    return plain;
}

QStringList NamesUnder(const SettingsIO::Map &settings, const QString &group){
    const QString prefix = group + SLASH;
    QStringList names;
    foreach(QString key, settings.keys()){
        if(!key.startsWith(prefix)) continue;
        const QString name = key.mid(prefix.length());
        if(name.isEmpty() || name.contains(SLASH)) continue;
        names << name;
    }
    return names;
}

bool LoadKeyMap(const SettingsIO::Map &settings, const QString &group,
                KeyMap &map, const Hooks &hooks){
    const QStringList names = NamesUnder(settings, group);

    if(names.isEmpty()){
        foreach(QString action, map.values())
            if(!hooks.Valid(action)) hooks.ReportInvalid(action);
        return false;
    }

    map.clear();
    foreach(QString name, names){
        map[hooks.Parse(UnescapeKeyName(name))] =
            settings.value(Entry(group, name), NO_ACTION).toString();
    }
    return true;
}

bool LoadGestureMap(const SettingsIO::Map &settings, const QString &group,
                    GestureMap &map, const Hooks &hooks){
    const QStringList names = NamesUnder(settings, group);

    if(names.isEmpty()){
        foreach(QString action, map.values())
            if(!hooks.Valid(action)) hooks.ReportInvalid(action);
        return false;
    }

    map.clear();
    foreach(QString name, names){
        map[name] = settings.value(Entry(group, name), NO_ACTION).toString();
    }
    return true;
}

void SaveKeyMap(SettingsIO::Map &settings, const QString &group, const KeyMap &map){
    foreach(QKeySequence seq, map.keys()){
        const QString name = seq.toString();
        if(seq.isEmpty() || name.isEmpty()) continue;
        settings.insert(Entry(group, EscapeKeyName(name)), map[seq]);
    }
}

void SaveGestureMap(SettingsIO::Map &settings, const QString &group, const GestureMap &map){
    foreach(QString gesture, map.keys()){
        if(gesture.isEmpty()) continue;
        settings.insert(Entry(group, gesture), map[gesture]);
    }
}

}
