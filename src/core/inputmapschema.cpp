#include "switch.hpp"
#include "const.hpp"

#include "inputmapschema.hpp"

#include "inputmap.hpp"
#include "actionmapper.hpp"
#include "keymap.hpp"
#include "mousemap.hpp"

#include <QCoreApplication>
#include <QKeySequence>
#include <QSet>

#include <algorithm>

namespace InputMapSchema {

namespace {

const QString SLASH = QStringLiteral("/");
const QString NO_ACTION = QStringLiteral("NoAction");

#define INPUTMAPSCHEMA_NAME(ACTION) << QStringLiteral(#ACTION)

struct Group {
    const char *label;
    QStringList actions;
};

Group NothingGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Nothing"),
                  QStringList() << NO_ACTION };
}
Group ScrollGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Scrolling"),
                  QStringList() FOR_EACH_KEYBOARD_EVENTS(INPUTMAPSCHEMA_NAME) };
}
Group ApplicationGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Application and windows"),
                  QStringList() FOR_EACH_APPLICATION_EVENTS(INPUTMAPSCHEMA_NAME) };
}
Group NavigationGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "History"),
                  QStringList() FOR_EACH_NAVIGATION_EVENTS(INPUTMAPSCHEMA_NAME) };
}
Group ViewGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Tabs"),
                  QStringList() FOR_EACH_VIEW_EVENTS(INPUTMAPSCHEMA_NAME) };
}
Group WebGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Page"),
                  QStringList() FOR_EACH_WEB_EVENTS1(INPUTMAPSCHEMA_NAME) };
}
Group ElementGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Links, images and media"),
                  QStringList() FOR_EACH_WEB_EVENTS2(INPUTMAPSCHEMA_NAME) };
}
Group GadgetsGroup(){
    return Group{ QT_TRANSLATE_NOOP("InputMapSchema", "Tab list"),
                  QStringList() FOR_EACH_GADGETS_EVENTS(INPUTMAPSCHEMA_NAME) };
}

#undef INPUTMAPSCHEMA_NAME

const QList<Group> &Groups(Vocabulary vocabulary){
    static const QList<Group> page = QList<Group>()
        << NothingGroup() << ScrollGroup() << ApplicationGroup() << NavigationGroup()
        << ViewGroup() << WebGroup() << ElementGroup();
    static const QList<Group> treebank = QList<Group>()
        << NothingGroup() << ScrollGroup() << ApplicationGroup() << NavigationGroup()
        << ViewGroup() << WebGroup();
    static const QList<Group> gadgets = QList<Group>()
        << NothingGroup() << ScrollGroup() << ApplicationGroup() << ViewGroup()
        << GadgetsGroup();
    switch(vocabulary){
    case PageActions:     return page;
    case TreeBankActions: return treebank;
    case GadgetsActions:  return gadgets;
    }
    return page;
}

QString VocabularyName(Vocabulary vocabulary){
    switch(vocabulary){
    case PageActions:     return QStringLiteral("page");
    case TreeBankActions: return QStringLiteral("treebank");
    case GadgetsActions:  return QStringLiteral("gadgets");
    }
    return QString();
}

QString KindName(Kind kind){
    switch(kind){
    case Keys:    return QStringLiteral("keys");
    case Mouse:   return QStringLiteral("mouse");
    case Gesture: return QStringLiteral("gesture");
    }
    return QString();
}

QString LabelSource(const QString &action){
    static const QMap<QString, QString> exceptions = {
        { QStringLiteral("Up"),       QStringLiteral("UpKey") },
        { QStringLiteral("Down"),     QStringLiteral("DownKey") },
        { QStringLiteral("Right"),    QStringLiteral("RightKey") },
        { QStringLiteral("Left"),     QStringLiteral("LeftKey") },
        { QStringLiteral("Home"),     QStringLiteral("HomeKey") },
        { QStringLiteral("End"),      QStringLiteral("EndKey") },
        { QStringLiteral("PageUp"),   QStringLiteral("PageUpKey") },
        { QStringLiteral("PageDown"), QStringLiteral("PageDownKey") },
        { QStringLiteral("OpenSettings"),          QStringLiteral("Settings") },
        { QStringLiteral("OpenDirectorySettings"), QStringLiteral("DirectorySettings") },
    };
    return exceptions.value(action, action);
}

InputMap::KeyMap DefaultKeyMap(const QString &group){
    if(group == QStringLiteral("webview/keymap")){
        InputMap::KeyMap m_KeyMap; WEBVIEW_KEYMAP return m_KeyMap;
    }
    if(group == QStringLiteral("application/keymap")){
        InputMap::KeyMap m_KeyMap; TREEBANK_KEYMAP return m_KeyMap;
    }
    if(group == QStringLiteral("gadgets/thumblist/keymap")){
        InputMap::KeyMap m_ThumbListKeyMap; THUMBLIST_KEYMAP return m_ThumbListKeyMap;
    }
    if(group == QStringLiteral("gadgets/accesskey/keymap")){
        InputMap::KeyMap m_AccessKeyKeyMap; ACCESSKEY_KEYMAP return m_AccessKeyKeyMap;
    }
    return InputMap::KeyMap();
}

InputMap::GestureMap DefaultGestureMap(const QString &group){
    if(group == QStringLiteral("webview/mouse")){
        InputMap::GestureMap m_MouseMap; WEBVIEW_MOUSEMAP return m_MouseMap;
    }
    if(group == QStringLiteral("application/mouse")){
        InputMap::GestureMap m_MouseMap; TREEBANK_MOUSEMAP return m_MouseMap;
    }
    if(group == QStringLiteral("gadgets/thumblist/mouse")){
        InputMap::GestureMap m_MouseMap; THUMBLIST_MOUSEMAP return m_MouseMap;
    }
    if(group == QStringLiteral("webview/rightgesture")){
        InputMap::GestureMap m_RightGestureMap; WEBVIEW_RIGHTGESTURE return m_RightGestureMap;
    }
    if(group == QStringLiteral("webview/leftgesture")){
        InputMap::GestureMap m_DragGestureMap; WEBVIEW_DRAGGESTURE return m_DragGestureMap;
    }
    if(group == QStringLiteral("webview/scrollgesture")){
        InputMap::GestureMap m_ScrollGestureMap; WEBVIEW_SCROLLGESTURE return m_ScrollGestureMap;
    }
    return InputMap::GestureMap();
}

typedef QList<QPair<QString, QString> > Pairs;

Pairs NamesOf(const InputMap::KeyMap &map){
    Pairs pairs;
    foreach(QKeySequence seq, map.keys()){
        if(seq.isEmpty()) continue;
        bool pressable = true;
        for(int i = 0; i < seq.count(); i++)
            if(seq[i].key() == Qt::Key_unknown) pressable = false;
        const QString name = seq.toString(QKeySequence::PortableText);
        if(!pressable || name.isEmpty()) continue;
        pairs << qMakePair(name, map[seq]);
    }
    return pairs;
}

Pairs NamesOf(const InputMap::GestureMap &map){
    Pairs pairs;
    foreach(QString name, map.keys()){
        if(name.isEmpty() || name == Placeholder()) continue;
        pairs << qMakePair(name, map[name]);
    }
    return pairs;
}

bool TakesEveryClick(Kind kind, const QString &canonical){
    return kind == Mouse && (canonical == QStringLiteral("LeftButton") ||
                             canonical == QStringLiteral("RightButton"));
}

QString Canonical(Kind kind, const QString &name){
    switch(kind){
    case Keys:    return InputMap::CanonicalKeyName(name);
    case Mouse:   return InputMap::CanonicalMouseName(name);
    case Gesture: return InputMap::CanonicalGestureName(name);
    }
    return QString();
}

void SortByAction(Vocabulary vocabulary, Pairs &pairs){
    const QStringList actions = Actions(vocabulary);
    std::stable_sort(pairs.begin(), pairs.end(),
                     [&](const QPair<QString, QString> &a, const QPair<QString, QString> &b){
        int x = actions.indexOf(a.second), y = actions.indexOf(b.second);
        if(x < 0) x = actions.length();
        if(y < 0) y = actions.length();
        if(x != y) return x < y;
        return a.first < b.first;
    });
}

void WriteNames(SettingsIO::Map &settings, const Table &table, const Pairs &pairs){
    const QString group = QString::fromLatin1(table.group);
    foreach(QString name, InputMap::NamesUnder(settings, group)) settings.remove(group + SLASH + name);
    if(pairs.isEmpty()){
        settings.insert(group + SLASH + Placeholder(), NO_ACTION);
        return;
    }
    foreach(const Pairs::value_type &pair, pairs){
        const QString name = table.kind == Keys ? InputMap::EscapeKeyName(pair.first) : pair.first;
        settings.insert(group + SLASH + name, pair.second);
    }
}

QJsonArray KeyParts(const QKeySequence &seq){
    QJsonArray strokes;
    for(int i = 0; i < seq.count(); i++){
        const QKeyCombination combination = seq[i];
        const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
        QJsonArray parts;
        if(modifiers & Qt::ControlModifier) parts.append(QStringLiteral("Ctrl"));
        if(modifiers & Qt::AltModifier)     parts.append(QStringLiteral("Alt"));
        if(modifiers & Qt::ShiftModifier)   parts.append(QStringLiteral("Shift"));
        if(modifiers & Qt::MetaModifier)    parts.append(QStringLiteral("Meta"));
        parts.append(QKeySequence(combination.key()).toString(QKeySequence::NativeText));
        strokes.append(parts);
    }
    return strokes;
}

}

const QList<Table> &Tables(){
    static const QList<Table> tables = QList<Table>()
        << Table{ "webview/keymap", Keys, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Keys on a page"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "While a page has the keyboard."),
                  true }
        << Table{ "application/keymap", Keys, TreeBankActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Keys in the window"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "While no page has the keyboard: an empty window, for one."),
                  true }
        << Table{ "gadgets/thumblist/keymap", Keys, GadgetsActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Keys in the tab list"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "While the tab list or the trash is shown."),
                  false }
        << Table{ "gadgets/accesskey/keymap", Keys, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Keys on an access key"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "While access keys are shown: pressed after a label, what is done to that element."),
                  false }
        << Table{ "webview/mouse", Mouse, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Mouse on a page"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "\"A+B\" is B pressed while A is held."),
                  false }
        << Table{ "application/mouse", Mouse, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Mouse in the window"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "Outside a page: an empty window, for one."),
                  false }
        << Table{ "gadgets/thumblist/mouse", Mouse, GadgetsActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Mouse in the tab list"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "While the tab list or the trash is shown."),
                  false }
        << Table{ "webview/rightgesture", Gesture, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Right button gestures"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "Drawn on a page with the right button held."),
                  false }
        << Table{ "webview/leftgesture", Gesture, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Drag gestures"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "Drawn while dragging a link, an image or selected text."),
                  false }
        << Table{ "webview/scrollgesture", Gesture, PageActions,
                  QT_TRANSLATE_NOOP("InputMapSchema", "Scroll gestures"),
                  QT_TRANSLATE_NOOP("InputMapSchema", "Drawn by scrolling with no button held, on a touchpad for one."),
                  false };
    return tables;
}

const Table *Find(const QString &group){
    const QList<Table> &tables = Tables();
    for(int i = 0; i < tables.length(); i++)
        if(QString::fromLatin1(tables[i].group) == group) return &tables[i];
    return nullptr;
}

QStringList Actions(Vocabulary vocabulary){
    QStringList actions;
    foreach(const Group &group, Groups(vocabulary)) actions << group.actions;
    return actions;
}

bool IsAction(Vocabulary vocabulary, const QString &action){
    foreach(const Group &group, Groups(vocabulary))
        if(group.actions.contains(action)) return true;
    return false;
}

QString ActionLabel(Vocabulary vocabulary, const QString &action){
    if(action == NO_ACTION)
        return QCoreApplication::translate("InputMapSchema", "Does nothing");

    QList<const char *> contexts;
    switch(vocabulary){
    case PageActions:     contexts << "Page" << "TreeBank"; break;
    case TreeBankActions: contexts << "TreeBank" << "Page"; break;
    case GadgetsActions:  contexts << "Gadgets" << "Page" << "TreeBank"; break;
    }
    const QByteArray source = LabelSource(action).toUtf8();
    foreach(const char *context, contexts){
        const QString text = QCoreApplication::translate(context, source.constData());
        if(text != QString::fromUtf8(source)) return text;
    }
    return QString::fromUtf8(source);
}

QString Placeholder(){
    return QStringLiteral("None");
}

QList<QPair<QString, QString> > Defaults(const Table &table){
    return Current(SettingsIO::Map(), table);
}

QList<QPair<QString, QString> > Current(const SettingsIO::Map &settings, const Table &table){
    const QString group = QString::fromLatin1(table.group);
    Pairs pairs;
    if(table.kind == Keys){
        InputMap::KeyMap map = DefaultKeyMap(group);
        InputMap::LoadKeyMap(settings, group, map);
        pairs = NamesOf(map);
    } else {
        InputMap::GestureMap map = DefaultGestureMap(group);
        InputMap::LoadGestureMap(settings, group, map);
        pairs = NamesOf(map);
    }
    SortByAction(table.vocabulary, pairs);
    return pairs;
}

QString Write(SettingsIO::Map &settings, const Table &table, const QJsonArray &entries){
    const Pairs now = Current(settings, table);

    Pairs written;
    foreach(QJsonValue value, entries){
        if(!value.isObject()) return QStringLiteral("an entry is not an object");
        const QJsonObject entry = value.toObject();
        const QString input  = entry[QStringLiteral("input")].toString();
        const QString action = entry[QStringLiteral("action")].toString();
        const bool held = now.contains(qMakePair(input, action));

        QString name = Canonical(table.kind, input);
        if(TakesEveryClick(table.kind, name) && !held)
            return QStringLiteral("takes every click: \"%1\"").arg(input);
        if(name.isEmpty()){
            if(!held) return QStringLiteral("cannot be pressed: \"%1\"").arg(input);
            name = input;
        }
        if(!IsAction(table.vocabulary, action) && !held)
            return QStringLiteral("unknown action: \"%1\"").arg(action);

        for(int i = written.length() - 1; i >= 0; i--)
            if(written[i].first == name) written.removeAt(i);
        written << qMakePair(name, action);
    }

    WriteNames(settings, table, written);
    return QString();
}

void Reset(SettingsIO::Map &settings, const Table &table){
    WriteNames(settings, table, Defaults(table));
}

QJsonObject DescribeTable(const SettingsIO::Map &settings, const Table &table){
    const Pairs defaults = Defaults(table);
    const Pairs current = Current(settings, table);

    QJsonArray entries;
    foreach(const Pairs::value_type &pair, current){
        QJsonObject entry;
        entry[QStringLiteral("input")]  = pair.first;
        entry[QStringLiteral("action")] = pair.second;
        entry[QStringLiteral("label")]  = ActionLabel(table.vocabulary, pair.second);
        entry[QStringLiteral("isDefault")] = defaults.contains(pair);
        const QString canonical = Canonical(table.kind, pair.first);
        entry[QStringLiteral("known")] =
            !canonical.isEmpty() && !TakesEveryClick(table.kind, canonical) &&
            IsAction(table.vocabulary, pair.second);
        if(table.kind == Keys){
            const QKeySequence seq(pair.first, QKeySequence::PortableText);
            entry[QStringLiteral("parts")]  = KeyParts(seq);
            entry[QStringLiteral("single")] = InputMap::IsSingleKey(seq);
        }
        if(table.kind == Gesture){
            bool diagonal = false;
            foreach(QString stroke, pair.first.split(QLatin1Char(',')))
                if(stroke.length() == 2) diagonal = true;
            entry[QStringLiteral("diagonal")] = diagonal;
        }
        entries.append(entry);
    }

    QSet<QPair<QString, QString> > a(defaults.begin(), defaults.end());
    QSet<QPair<QString, QString> > b(current.begin(), current.end());

    QJsonObject object;
    object[QStringLiteral("group")]      = QString::fromLatin1(table.group);
    object[QStringLiteral("kind")]       = KindName(table.kind);
    object[QStringLiteral("vocabulary")] = VocabularyName(table.vocabulary);
    object[QStringLiteral("label")] = QCoreApplication::translate("InputMapSchema", table.label);
    object[QStringLiteral("hint")]  = QCoreApplication::translate("InputMapSchema", table.hint);
    object[QStringLiteral("singleKeysHeld")] = table.singleKeysHeld;
    object[QStringLiteral("changed")] = a != b;
    object[QStringLiteral("entries")] = entries;
    return object;
}

QJsonObject Describe(const SettingsIO::Map &settings){
    QJsonArray tables;
    foreach(const Table &table, Tables()) tables.append(DescribeTable(settings, table));

    QJsonObject vocabularies;
    foreach(Vocabulary vocabulary, QList<Vocabulary>() << PageActions << TreeBankActions << GadgetsActions){
        QJsonArray groups;
        foreach(const Group &group, Groups(vocabulary)){
            QJsonArray actions;
            foreach(QString action, group.actions){
                QJsonObject object;
                object[QStringLiteral("name")]  = action;
                object[QStringLiteral("label")] = ActionLabel(vocabulary, action);
                actions.append(object);
            }
            QJsonObject object;
            object[QStringLiteral("label")] = QCoreApplication::translate("InputMapSchema", group.label);
            object[QStringLiteral("actions")] = actions;
            groups.append(object);
        }
        vocabularies[VocabularyName(vocabulary)] = groups;
    }

    QJsonObject strings;
    strings[QStringLiteral("edit")] =
        QCoreApplication::translate("InputMapSchema", "Edit");
    strings[QStringLiteral("done")] =
        QCoreApplication::translate("InputMapSchema", "Done");
    strings[QStringLiteral("resetTable")] =
        QCoreApplication::translate("InputMapSchema", "Back to the defaults");
    strings[QStringLiteral("changed")] =
        QCoreApplication::translate("InputMapSchema", "Changed");
    strings[QStringLiteral("remove")] =
        QCoreApplication::translate("InputMapSchema", "Remove");
    strings[QStringLiteral("add")] =
        QCoreApplication::translate("InputMapSchema", "Add");
    strings[QStringLiteral("pressKey")] =
        QCoreApplication::translate("InputMapSchema", "Click, then press the keys");
    strings[QStringLiteral("recording")] =
        QCoreApplication::translate("InputMapSchema", "Press the keys...");
    strings[QStringLiteral("held")] =
        QCoreApplication::translate("InputMapSchema", "Held");
    strings[QStringLiteral("nothingHeld")] =
        QCoreApplication::translate("InputMapSchema", "No button held");
    strings[QStringLiteral("clear")] =
        QCoreApplication::translate("InputMapSchema", "Clear");
    strings[QStringLiteral("empty")] =
        QCoreApplication::translate("InputMapSchema", "Nothing is assigned.");
    strings[QStringLiteral("unknown")] =
        QCoreApplication::translate("InputMapSchema", "Not an input or action this version knows. Kept as it is.");
    strings[QStringLiteral("singleKeyOff")] =
        QCoreApplication::translate("InputMapSchema", "Not used while single key shortcuts are off (Pages).");
    strings[QStringLiteral("diagonalOff")] =
        QCoreApplication::translate("InputMapSchema", "Not used while gestures are four way (Pages).");
    strings[QStringLiteral("LeftButton")] =
        QCoreApplication::translate("InputMapSchema", "Left button");
    strings[QStringLiteral("RightButton")] =
        QCoreApplication::translate("InputMapSchema", "Right button");
    strings[QStringLiteral("MidButton")] =
        QCoreApplication::translate("InputMapSchema", "Middle button");
    strings[QStringLiteral("ExtraButton1")] =
        QCoreApplication::translate("InputMapSchema", "Side button 1");
    strings[QStringLiteral("ExtraButton2")] =
        QCoreApplication::translate("InputMapSchema", "Side button 2");
    strings[QStringLiteral("WheelUp")] =
        QCoreApplication::translate("InputMapSchema", "Wheel up");
    strings[QStringLiteral("WheelDown")] =
        QCoreApplication::translate("InputMapSchema", "Wheel down");

    QJsonObject root;
    root[QStringLiteral("category")] = QStringLiteral("input");
    root[QStringLiteral("tables")] = tables;
    root[QStringLiteral("vocabularies")] = vocabularies;
    root[QStringLiteral("strings")] = strings;
    root[QStringLiteral("singleKeysOff")] =
        !settings.value(QStringLiteral("webview/@EnableSingleKeyShortcut"), false).toBool();
    root[QStringLiteral("fourWayGestures")] =
        settings.value(QStringLiteral("webview/@GestureMode"), 4).toInt() != 8;
    return root;
}

}
