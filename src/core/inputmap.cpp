#include "switch.hpp"
#include "const.hpp"

#include "inputmap.hpp"

#include <algorithm>

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

bool IsSingleKey(const QKeySequence &seq){
    if(seq.count() != 1) return false;
    const QKeyCombination combination = seq[0];
    if(combination.keyboardModifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return false;
    return combination.key() < 0x01000000;
}

QString CanonicalKeyName(const QString &name){
    const QKeySequence seq = QKeySequence::fromString(name, QKeySequence::PortableText);
    if(seq.isEmpty()) return QString();
    for(int i = 0; i < seq.count(); i++){
        const Qt::Key key = seq[i].key();
        if(key == Qt::Key_unknown || key == 0 ||
           key == Qt::Key_Shift   || key == Qt::Key_Control ||
           key == Qt::Key_Meta    || key == Qt::Key_Alt) return QString();
    }
    const QString canonical = seq.toString(QKeySequence::PortableText);
    if(QKeySequence(canonical) != seq) return QString();
    return canonical;
}

namespace {

const QStringList MODIFIERS = QStringList()
    << QStringLiteral("Shift") << QStringLiteral("Ctrl") << QStringLiteral("Alt")
    << QStringLiteral("Meta")  << QStringLiteral("Keypad");

QStringList Buttons(){
    QStringList buttons;
    buttons << QStringLiteral("LeftButton") << QStringLiteral("RightButton")
            << QStringLiteral("MidButton");
    for(int i = 1; i <= 24; i++) buttons << QStringLiteral("ExtraButton%1").arg(i);
    return buttons;
}

const QStringList WHEELS = QStringList()
    << QStringLiteral("WheelUp") << QStringLiteral("WheelDown");

const QStringList STROKES = QStringList()
    << QStringLiteral("U")  << QStringLiteral("D")  << QStringLiteral("R")  << QStringLiteral("L")
    << QStringLiteral("UR") << QStringLiteral("UL") << QStringLiteral("DR") << QStringLiteral("DL");

}

QString CanonicalMouseName(const QString &name){
    static const QStringList buttons = Buttons();

    QStringList tokens = name.split(QLatin1Char('+'));
    const QString last = tokens.takeLast();
    if(!buttons.contains(last) && !WHEELS.contains(last)) return QString();

    QList<int> modifiers, held;
    foreach(QString token, tokens){
        const int modifier = MODIFIERS.indexOf(token);
        const int button = buttons.indexOf(token);
        if(modifier >= 0 && !modifiers.contains(modifier)) modifiers << modifier;
        else if(button >= 0 && !held.contains(button) && token != last) held << button;
        else return QString();
    }
    std::sort(modifiers.begin(), modifiers.end());
    std::sort(held.begin(), held.end());

    QStringList canonical;
    foreach(int modifier, modifiers) canonical << MODIFIERS[modifier];
    foreach(int button, held) canonical << buttons[button];
    canonical << last;
    return canonical.join(QLatin1Char('+'));
}

QString CanonicalGestureName(const QString &name){
    const QStringList strokes = name.split(QLatin1Char(','));
    for(int i = 0; i < strokes.length(); i++){
        if(!STROKES.contains(strokes[i])) return QString();
        if(i > 0 && strokes[i] == strokes[i - 1]) return QString();
    }
    return strokes.join(QLatin1Char(','));
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
