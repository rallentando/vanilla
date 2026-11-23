#include "switch.hpp"
#include "const.hpp"

#include "menuschema.hpp"
#include "inputmapschema.hpp"

#include <QCoreApplication>
#include <QHash>

namespace MenuSchema {

namespace {

const QString COMMA = QStringLiteral(",");
const QString SEPARATOR = QStringLiteral("Separator");

struct Special {
    const char *name;
    const char *label;
};

const QList<Special> &Specials(){
    static const QList<Special> list = {
        { "Separator",                 QT_TRANSLATE_NOOP("MenuSchema", "Separator") },
        { "LinkUrl",                   QT_TRANSLATE_NOOP("MenuSchema", "Heading: the link's name") },
        { "ImageUrl",                  QT_TRANSLATE_NOOP("MenuSchema", "Heading: the image's name") },
        { "MediaUrl",                  QT_TRANSLATE_NOOP("MenuSchema", "Heading: the media's name") },
        { "OpenLinkWithOtherBrowser",  QT_TRANSLATE_NOOP("MenuSchema", "Open link with another browser (submenu)") },
        { "OpenImageWithOtherBrowser", QT_TRANSLATE_NOOP("MenuSchema", "Open image with another browser (submenu)") },
        { "OpenMediaWithOtherBrowser", QT_TRANSLATE_NOOP("MenuSchema", "Open media with another browser (submenu)") },
        { "OpenWithOtherBrowser",      QT_TRANSLATE_NOOP("MenuSchema", "Open with another browser (submenu)") },
        { "SearchMenu",                QT_TRANSLATE_NOOP("MenuSchema", "Search the selection (submenu)") },
        { "BookmarkletMenu",           QT_TRANSLATE_NOOP("MenuSchema", "Bookmarklets (submenu)") },
        { "ViewOrApplySource",         QT_TRANSLATE_NOOP("MenuSchema", "View source, or apply it on a source page") },
        { "OpenNodeWithOtherBrowser",  QT_TRANSLATE_NOOP("MenuSchema", "Open the tab with another browser (submenu)") },
        { "DirectorySettings",         QT_TRANSLATE_NOOP("MenuSchema", "The directory's settings (submenu)") },
        { "DeleteNodesTogether",       QT_TRANSLATE_NOOP("MenuSchema", "Close tabs together (submenu)") },
        { "ReloadNode",                QT_TRANSLATE_NOOP("MenuSchema", "Reload the tab") },
        { "RecentlyClosed",            QT_TRANSLATE_NOOP("MenuSchema", "Recently closed tabs (submenu)") },
        { "TreeBarMenu",               QT_TRANSLATE_NOOP("MenuSchema", "The tab bar's menu (submenu)") },
        { "SortMenu",                  QT_TRANSLATE_NOOP("MenuSchema", "Sort (submenu)") },
        { "NodeCollectionTypeMenu",    QT_TRANSLATE_NOOP("MenuSchema", "What the tab list collects (submenu)") },
    };
    return list;
}

const Special *FindSpecial(const QString &name){
    for(const Special &special : Specials())
        if(name == QLatin1String(special.name)) return &special;
    return nullptr;
}

QStringList Split(const QString &value){
    return value.split(COMMA, Qt::SkipEmptyParts);
}

}

const QStringList &NodeNames(){
    static const QStringList names = QStringList()
        << QStringLiteral("OpenNode") << QStringLiteral("OpenNodeOnNewWindow")
        << QStringLiteral("OpenNodeWithDefault") << QStringLiteral("RenameNode")
        << QStringLiteral("CopyNodeUrl") << QStringLiteral("CloneNode")
        << QStringLiteral("NewNode") << QStringLiteral("MakeDirectory")
        << QStringLiteral("CutNode") << QStringLiteral("CopyNode") << QStringLiteral("PasteNode")
        << QStringLiteral("MakeDirectoryWithSelectedNode") << QStringLiteral("MakeDirectoryWithSameDomainNode")
        << QStringLiteral("DeleteNode") << QStringLiteral("DeleteRightNode")
        << QStringLiteral("DeleteLeftNode") << QStringLiteral("DeleteOtherNode")
        << QStringLiteral("RestoreNode") << QStringLiteral("UpDirectory")
        << QStringLiteral("DownDirectory") << QStringLiteral("ToggleTrash")
        << QStringLiteral("Refresh") << QStringLiteral("Deactivate");
    return names;
}

const char *const IconsHint =
    QT_TRANSLATE_NOOP("MenuSchema", "The line of icons at the top of the menu. What it holds is not "
                                    "repeated below it; the command bar comes last.");

const QList<Table> &Tables(){
    static const QList<Table> tables = {
        { "LinkMenu", "webview/menu/LinkMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Link"),
          QT_TRANSLATE_NOOP("MenuSchema", "On a link."),
          QStringList() << QStringLiteral("LinkUrl") << QStringLiteral("OpenLinkWithOtherBrowser"),
          QStringLiteral(
              "LinkUrl,"
              "OpenLink,"
              "DownloadLink,"
              "CopyLinkUrl,"
              "OpenLinkWithOtherBrowser") },
        { "ImageMenu", "webview/menu/ImageMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Image"),
          QT_TRANSLATE_NOOP("MenuSchema", "On an image."),
          QStringList() << QStringLiteral("ImageUrl") << QStringLiteral("OpenImageWithOtherBrowser"),
          QStringLiteral(
              "ImageUrl,"
              "OpenImage,"
              "DownloadImage,"
              "CopyImage,"
              "CopyImageUrl,"
              "OpenImageWithOtherBrowser") },
        { "MediaMenu", "webview/menu/MediaMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Video and audio"),
          QT_TRANSLATE_NOOP("MenuSchema", "On a video or an audio player."),
          QStringList() << QStringLiteral("MediaUrl") << QStringLiteral("OpenMediaWithOtherBrowser"),
          QStringLiteral(
              "MediaUrl,"
              "ToggleMediaPlayPause,"
              "ToggleMediaMute,"
              "ToggleMediaLoop,"
              "Separator,"
              "OpenMedia,"
              "DownloadMedia,"
              "CopyMediaUrl,"
              "OpenMediaWithOtherBrowser") },
        { "TextMenu", "webview/menu/TextMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Text field"),
          QT_TRANSLATE_NOOP("MenuSchema", "In a field on the page. The application's own fields (the address bar, "
                                          "the search) follow it too, with only the editing actions, and add Copy "
                                          "while something is selected."),
          QStringList(),
          QStringLiteral(
              "Undo,"
              "Redo,"
              "SelectAll,"
              "Paste,"
              "Cut") },
        { "SelectionMenu", "webview/menu/SelectionMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Selection"),
          QT_TRANSLATE_NOOP("MenuSchema", "While text is selected."),
          QStringList() << QStringLiteral("SearchMenu"),
          QStringLiteral(
              "Copy,"
              "CopySelectedHtml,"
              "Separator,"
              "SearchMenu") },
        { "RegularMenu", "webview/menu/RegularMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Page"),
          QT_TRANSLATE_NOOP("MenuSchema", "At the end of every menu on the page."),
          QStringList() << QStringLiteral("BookmarkletMenu") << QStringLiteral("OpenWithOtherBrowser")
                        << QStringLiteral("ViewOrApplySource"),
          QStringLiteral(
              "NewViewNode,"
              "CloneViewNode,"
              "CopyUrl,"
              "CopyPageAsLink,"
              "OpenWithOtherBrowser,"
              "ViewOrApplySource,"
              "InspectElement") },
        { "TreeBarNodeMenu", "webview/menu/TreeBarNodeMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Tab bar"),
          QT_TRANSLATE_NOOP("MenuSchema", "On a tab of the tab bar. What does not apply to the tab, "
                                          "or to the tab bar, is left out."),
          QStringList() << QStringLiteral("OpenNodeWithOtherBrowser") << QStringLiteral("DirectorySettings")
                        << QStringLiteral("DeleteNodesTogether") << QStringLiteral("ReloadNode")
                        << QStringLiteral("RecentlyClosed") << QStringLiteral("TreeBarMenu"),
          QStringLiteral(
              "OpenNode,"
              "OpenNodeOnNewWindow,"
              "OpenNodeWithOtherBrowser,"
              "Separator,"
              "RenameNode,"
              "DirectorySettings,"
              "ReloadNode,"
              "Separator,"
              "MakeDirectory,"
              "MakeDirectoryWithSelectedNode,"
              "MakeDirectoryWithSameDomainNode,"
              "Separator,"
              "DeleteNodesTogether,"
              "RecentlyClosed,"
              "Separator,"
              "TreeBarMenu"),
          NodeNames() },
        { "TableNodeMenu", "webview/menu/TableNodeMenu",
          QT_TRANSLATE_NOOP("MenuSchema", "Tab list"),
          QT_TRANSLATE_NOOP("MenuSchema", "In the tab list and the trash, over a tab or not. What does not apply "
                                          "there is left out; in the trash, the way back comes first."),
          QStringList() << QStringLiteral("OpenNodeWithOtherBrowser") << QStringLiteral("DirectorySettings")
                        << QStringLiteral("DeleteNodesTogether") << QStringLiteral("SortMenu")
                        << QStringLiteral("NodeCollectionTypeMenu"),
          QStringLiteral(
              "OpenNode,"
              "OpenNodeOnNewWindow,"
              "OpenNodeWithOtherBrowser,"
              "Separator,"
              "RenameNode,"
              "CopyNodeUrl,"
              "DirectorySettings,"
              "Separator,"
              "MakeDirectory,"
              "CutNode,"
              "CopyNode,"
              "PasteNode,"
              "MakeDirectoryWithSelectedNode,"
              "MakeDirectoryWithSameDomainNode,"
              "Separator,"
              "DeleteNodesTogether,"
              "Separator,"
              "UpDirectory,"
              "DownDirectory,"
              "ToggleTrash,"
              "SortMenu,"
              "NodeCollectionTypeMenu,"
              "Separator,"
              "Refresh,"
              "Deactivate"),
          NodeNames() },

        { "PageIcons", "webview/menu/PageIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Page: icons"), IconsHint, QStringList(),
          QStringLiteral("Back,Forward,Reload"),
          QStringList() << QStringLiteral("Back") << QStringLiteral("Forward") << QStringLiteral("Rewind")
                        << QStringLiteral("FastForward") << QStringLiteral("Reload"), true },
        { "LinkIcons", "webview/menu/LinkIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Link: icons"), IconsHint, QStringList(),
          QStringLiteral("OpenLink,DownloadLink"),
          QStringList() << QStringLiteral("OpenLink") << QStringLiteral("DownloadLink")
                        << QStringLiteral("CopyLinkUrl"), true },
        { "ImageIcons", "webview/menu/ImageIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Image: icons"), IconsHint, QStringList(),
          QStringLiteral("OpenImage,DownloadImage"),
          QStringList() << QStringLiteral("OpenImage") << QStringLiteral("DownloadImage")
                        << QStringLiteral("CopyImage") << QStringLiteral("CopyImageUrl"), true },
        { "MediaIcons", "webview/menu/MediaIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Video and audio: icons"), IconsHint, QStringList(),
          QStringLiteral("ToggleMediaPlayPause,ToggleMediaMute,ToggleMediaLoop"),
          QStringList() << QStringLiteral("ToggleMediaPlayPause") << QStringLiteral("ToggleMediaMute")
                        << QStringLiteral("ToggleMediaLoop") << QStringLiteral("OpenMedia")
                        << QStringLiteral("DownloadMedia") << QStringLiteral("CopyMediaUrl"), true },
        { "TextIcons", "webview/menu/TextIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Text field: icons"), IconsHint, QStringList(),
          QStringLiteral("Cut,Copy,Paste"),
          QStringList() << QStringLiteral("Cut") << QStringLiteral("Copy") << QStringLiteral("Paste"), true },
        { "SelectionIcons", "webview/menu/SelectionIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Selection: icons"), IconsHint, QStringList(),
          QStringLiteral("Copy"),
          QStringList() << QStringLiteral("Copy"), true },
        { "TreeBarNodeIcons", "webview/menu/TreeBarNodeIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Tab bar: icons"), IconsHint, QStringList(),
          QStringLiteral("NewNode,CloneNode,DeleteNode"),
          QStringList() << QStringLiteral("NewNode") << QStringLiteral("CloneNode")
                        << QStringLiteral("DeleteNode") << QStringLiteral("ReloadNode"), true },
        { "TableNodeIcons", "webview/menu/TableNodeIcons",
          QT_TRANSLATE_NOOP("MenuSchema", "Tab list: icons"), IconsHint, QStringList(),
          QStringLiteral("NewNode,CloneNode,DeleteNode,RestoreNode"),
          QStringList() << QStringLiteral("NewNode") << QStringLiteral("CloneNode")
                        << QStringLiteral("DeleteNode") << QStringLiteral("RestoreNode"), true },
    };
    return tables;
}

const Table *IconsOf(const QString &lines){
    static const QHash<QString, QString> icons = {
        { QStringLiteral("RegularMenu"),     QStringLiteral("PageIcons") },
        { QStringLiteral("LinkMenu"),        QStringLiteral("LinkIcons") },
        { QStringLiteral("ImageMenu"),       QStringLiteral("ImageIcons") },
        { QStringLiteral("MediaMenu"),       QStringLiteral("MediaIcons") },
        { QStringLiteral("TextMenu"),        QStringLiteral("TextIcons") },
        { QStringLiteral("SelectionMenu"),   QStringLiteral("SelectionIcons") },
        { QStringLiteral("TreeBarNodeMenu"), QStringLiteral("TreeBarNodeIcons") },
        { QStringLiteral("TableNodeMenu"),   QStringLiteral("TableNodeIcons") },
    };
    return icons.contains(lines) ? Find(icons.value(lines)) : nullptr;
}

const Table *Find(const QString &name){
    for(const Table &table : Tables())
        if(name == QLatin1String(table.name)) return &table;
    return nullptr;
}

QString Key(const Table &table){
    return QString::fromLatin1(table.key);
}

QString Default(const QString &name){
    const Table *table = Find(name);
    return table ? table->defaults : QString();
}

QStringList Current(const SettingsIO::Map &settings, const Table &table){
    return Split(settings.value(Key(table), table.defaults).toString());
}

bool IsKnown(const Table &table, const QString &name){
    if(table.icons) return table.vocabulary.contains(name);
    if(name == SEPARATOR || table.specials.contains(name)) return true;
    if(!table.vocabulary.isEmpty()) return table.vocabulary.contains(name);
    return name != QStringLiteral("NoAction") && name != QStringLiteral("OpenCommand") &&
        InputMapSchema::IsAction(InputMapSchema::PageActions, name);
}

QString Label(const QString &name){
    if(const Special *special = FindSpecial(name))
        return QCoreApplication::translate("MenuSchema", special->label);
    if(InputMapSchema::IsAction(InputMapSchema::PageActions, name))
        return InputMapSchema::ActionLabel(InputMapSchema::PageActions, name);
    if(InputMapSchema::IsAction(InputMapSchema::GadgetsActions, name))
        return InputMapSchema::ActionLabel(InputMapSchema::GadgetsActions, name);
    return name;
}

QString Write(SettingsIO::Map &settings, const Table &table, const QJsonArray &names){
    const QStringList now = Current(settings, table);

    QStringList written;
    foreach(QJsonValue value, names){
        if(!value.isString()) return QStringLiteral("a name is not a string");
        const QString name = value.toString();
        if(!IsKnown(table, name) && !now.contains(name))
            return QStringLiteral("unknown name: \"%1\"").arg(name);
        written << name;
    }

    settings.insert(Key(table), written.join(COMMA));
    return QString();
}

void Reset(SettingsIO::Map &settings, const Table &table){
    settings.remove(Key(table));
}

QJsonObject DescribeTable(const SettingsIO::Map &settings, const Table &table){
    const QStringList current = Current(settings, table);

    QJsonArray entries;
    for(const QString &name : current){
        QJsonObject entry;
        entry[QStringLiteral("name")]  = name;
        entry[QStringLiteral("label")] = Label(name);
        entry[QStringLiteral("known")] = IsKnown(table, name);
        entries.append(entry);
    }

    QJsonArray groups;
    if(!table.specials.isEmpty()){
        QJsonArray actions;
        for(const QString &name : table.specials){
            QJsonObject action;
            action[QStringLiteral("name")]  = name;
            action[QStringLiteral("label")] = Label(name);
            actions.append(action);
        }
        QJsonObject group;
        group[QStringLiteral("label")] = QCoreApplication::translate("MenuSchema", "Only in menus");
        group[QStringLiteral("actions")] = actions;
        groups.append(group);
    }
    if(!table.vocabulary.isEmpty()){
        QJsonArray actions;
        for(const QString &name : table.vocabulary){
            QJsonObject action;
            action[QStringLiteral("name")]  = name;
            action[QStringLiteral("label")] = Label(name);
            actions.append(action);
        }
        QJsonObject group;
        group[QStringLiteral("label")] = table.icons
            ? QCoreApplication::translate("MenuSchema", "With an icon")
            : QCoreApplication::translate("MenuSchema", "On tabs");
        group[QStringLiteral("actions")] = actions;
        groups.append(group);
    }
    if(table.vocabulary.isEmpty())
    foreach(QJsonValue value, InputMapSchema::DescribeVocabulary(InputMapSchema::PageActions)){
        QJsonObject group = value.toObject();
        QJsonArray actions;
        foreach(QJsonValue action, group[QStringLiteral("actions")].toArray())
            if(IsKnown(table, action.toObject()[QStringLiteral("name")].toString()))
                actions.append(action);
        if(actions.isEmpty()) continue;
        group[QStringLiteral("actions")] = actions;
        groups.append(group);
    }

    QJsonObject object;
    object[QStringLiteral("group")] = QString::fromLatin1(table.name);
    object[QStringLiteral("label")] = QCoreApplication::translate("MenuSchema", table.label);
    object[QStringLiteral("hint")]  = QCoreApplication::translate("MenuSchema", table.hint);
    object[QStringLiteral("changed")] = current != Split(table.defaults);
    object[QStringLiteral("entries")] = entries;
    object[QStringLiteral("groups")]  = groups;
    return object;
}

QJsonObject Describe(const SettingsIO::Map &settings){
    QJsonArray tables;
    for(const Table &table : Tables()){
        if(table.icons) continue;
        const QString menu = QString::fromLatin1(table.name);
        QList<QPair<const Table*, QString>> parts;
        if(const Table *icons = IconsOf(menu)) parts << qMakePair(icons, QStringLiteral("icons"));
        parts << qMakePair(&table, QStringLiteral("items"));
        for(const auto &part : parts){
            QJsonObject object = DescribeTable(settings, *part.first);
            object[QStringLiteral("menu")] = menu;
            object[QStringLiteral("menuLabel")] = QCoreApplication::translate("MenuSchema", table.label);
            object[QStringLiteral("menuHint")] = QCoreApplication::translate("MenuSchema", table.hint);
            object[QStringLiteral("part")] = part.second;
            tables.append(object);
        }
    }

    QJsonObject strings;
    strings[QStringLiteral("menus")] =
        QCoreApplication::translate("MenuSchema", "Menus");
    strings[QStringLiteral("icons")] =
        QCoreApplication::translate("MenuSchema", "Icons");
    strings[QStringLiteral("items")] =
        QCoreApplication::translate("MenuSchema", "Items");
    strings[QStringLiteral("separator")] =
        QCoreApplication::translate("MenuSchema", "Separator");
    strings[QStringLiteral("addSeparator")] =
        QCoreApplication::translate("MenuSchema", "Add a separator");
    strings[QStringLiteral("moveUp")] =
        QCoreApplication::translate("MenuSchema", "Move up");
    strings[QStringLiteral("moveDown")] =
        QCoreApplication::translate("MenuSchema", "Move down");
    strings[QStringLiteral("empty")] =
        QCoreApplication::translate("MenuSchema", "The menu has nothing of its own.");
    strings[QStringLiteral("unknown")] =
        QCoreApplication::translate("MenuSchema", "Not an action this version knows. Kept as it is.");

    QJsonObject root;
    root[QStringLiteral("category")] = QStringLiteral("menu");
    root[QStringLiteral("tables")] = tables;
    root[QStringLiteral("strings")] = strings;
    return root;
}

}
