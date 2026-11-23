#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include "application.hpp"
#include "menuschema.hpp"
#include "view.hpp"

#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif

namespace {

QString Source(const QString &path){
    QFile file(QStringLiteral(VANILLA_SOURCE_DIR "/") + path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
}

const MenuSchema::Table &TableOf(const char *name){
    const MenuSchema::Table *table = MenuSchema::Find(QString::fromLatin1(name));
    Q_ASSERT(table);
    return *table;
}

QJsonArray Names(const QStringList &names){
    QJsonArray array;
    for(const QString &name : names) array.append(name);
    return array;
}

QStringList Split(const QString &value){
    return value.split(QLatin1Char(','), Qt::SkipEmptyParts);
}

}

class tst_menuschema : public QObject {
    Q_OBJECT

private slots:
    void everyDefaultNameIsKnown();
    void theSpecialNamesAreTheOnesTheViewInterprets();
    void theViewTakesItsDefaultsFromHere();
    void aWriteReadsBackAsWritten();
    void anEmptiedMenuStaysEmpty();
    void aRefusedWriteWritesNothing();
    void aNameTheFileHoldsIsKept();
    void resetGivesTheDefaultBack();
    void theViewKeepsWhatThePageWrote();
    void thePageShowsEachMenuWithItsIconsFirst();
};

void tst_menuschema::everyDefaultNameIsKnown(){
    QCOMPARE(MenuSchema::Tables().size(), 16);
    for(const MenuSchema::Table &table : MenuSchema::Tables()){
        QCOMPARE(MenuSchema::Key(table), QStringLiteral("webview/menu/") + QString::fromLatin1(table.name));
        for(const QString &name : Split(table.defaults))
            QVERIFY2(MenuSchema::IsKnown(table, name),
                     qPrintable(QString::fromLatin1(table.name) + QStringLiteral(": ") + name));
        for(const QString &name : table.specials)
            QVERIFY2(MenuSchema::Label(name) != name, qPrintable(name));
    }
    QVERIFY(!MenuSchema::IsKnown(TableOf("LinkMenu"), QStringLiteral("NoAction")));
    QVERIFY(!MenuSchema::IsKnown(TableOf("LinkMenu"), QStringLiteral("SearchMenu")));
}

void tst_menuschema::theSpecialNamesAreTheOnesTheViewInterprets(){
    const QString source = Source(QStringLiteral("view/view.cpp"));
    const int begin = source.indexOf(QStringLiteral("void View::AddContextMenu("));
    const int end = source.indexOf(QStringLiteral("GetViewTypeName() + QStringLiteral"), begin);
    QVERIFY(begin >= 0);
    QVERIFY(end > begin);
    const QString menus = source.mid(begin, end - begin);

    QSet<QString> interpreted;
    static const QRegularExpression compared(QStringLiteral("str == QStringLiteral\\(\"(\\w+)\"\\)"));
    auto it = compared.globalMatch(menus);
    while(it.hasNext()) interpreted.insert(it.next().captured(1));
    QVERIFY(interpreted.contains(QStringLiteral("Separator")));

    QSet<QString> offered;
    offered.insert(QStringLiteral("Separator"));
    for(const MenuSchema::Table &table : MenuSchema::Tables()){
        if(!table.vocabulary.isEmpty()) continue;
        for(const QString &name : table.specials){
            offered.insert(name);
            QVERIFY2(interpreted.contains(name), qPrintable(name));
        }
    }
    QCOMPARE(offered, interpreted);

    const struct { const char *table; const char *file; const char *function; } tabs[] = {
        { "TreeBarNodeMenu", "ui/treebar.cpp",      "QMenu *NodeItem::NodeMenu()" },
        { "TableNodeMenu",   "gadgets/gadgets.cpp", "QMenu *Gadgets::CreateNodeMenu()" },
    };
    for(const auto &tab : tabs){
        const QString text = Source(QString::fromLatin1(tab.file));
        const int from = text.indexOf(QString::fromLatin1(tab.function));
        QVERIFY(from >= 0);
        const QString body = text.mid(from, text.indexOf(QStringLiteral("\nvoid "), from) - from);
        const MenuSchema::Table &table = TableOf(tab.table);
        for(const QString &name : table.specials)
            QVERIFY2(body.contains(QStringLiteral("name == QStringLiteral(\"%1\")").arg(name)),
                     qPrintable(QString::fromLatin1(tab.table) + QStringLiteral(": ") + name));
    }
}

void tst_menuschema::theViewTakesItsDefaultsFromHere(){
    const QString source = Source(QStringLiteral("view/view.cpp"));
    for(const MenuSchema::Table &table : MenuSchema::Tables()){
        const QString name = QString::fromLatin1(table.name);
        if(!table.vocabulary.isEmpty()) continue;
        QVERIFY2(source.contains(QStringLiteral("= read(\"%1\");").arg(name)), qPrintable(name));
    }
}

void tst_menuschema::aWriteReadsBackAsWritten(){
    const MenuSchema::Table &table = TableOf("LinkMenu");
    SettingsIO::Map settings;
    const QStringList names = { QStringLiteral("OpenLink"), QStringLiteral("Separator"),
                                QStringLiteral("LinkUrl"), QStringLiteral("OpenLink") };
    QCOMPARE(MenuSchema::Write(settings, table, Names(names)), QString());
    QCOMPARE(settings.value(MenuSchema::Key(table)).toString(), names.join(QLatin1Char(',')));
    QCOMPARE(MenuSchema::Current(settings, table), names);
    QVERIFY(MenuSchema::DescribeTable(settings, table)[QStringLiteral("changed")].toBool());

    QCOMPARE(MenuSchema::Write(settings, table, Names(Split(table.defaults))), QString());
    QVERIFY(!MenuSchema::DescribeTable(settings, table)[QStringLiteral("changed")].toBool());
}

void tst_menuschema::anEmptiedMenuStaysEmpty(){
    const MenuSchema::Table &table = TableOf("TextMenu");
    SettingsIO::Map settings;
    QCOMPARE(MenuSchema::Write(settings, table, QJsonArray()), QString());
    QVERIFY(settings.contains(MenuSchema::Key(table)));
    QCOMPARE(settings.value(MenuSchema::Key(table)).toString(), QString());
    QVERIFY(MenuSchema::Current(settings, table).isEmpty());
    const QJsonObject described = MenuSchema::DescribeTable(settings, table);
    QVERIFY(described[QStringLiteral("entries")].toArray().isEmpty());
    QVERIFY(described[QStringLiteral("changed")].toBool());
}

void tst_menuschema::aRefusedWriteWritesNothing(){
    const MenuSchema::Table &table = TableOf("SelectionMenu");
    SettingsIO::Map settings;
    settings.insert(MenuSchema::Key(table), QStringLiteral("Copy,SearchMenu"));
    const SettingsIO::Map before = settings;

    const QList<QJsonArray> refused = {
        Names({ QStringLiteral("Copy"), QStringLiteral("NoSuchAction") }),
        Names({ QStringLiteral("Copy,Cut") }),
        Names({ QString() }),
        Names({ QStringLiteral("NoAction") }),
        Names({ QStringLiteral("LinkUrl") }),
        QJsonArray({ 1 }),
    };
    for(const QJsonArray &names : refused){
        QVERIFY2(!MenuSchema::Write(settings, table, names).isEmpty(),
                 qPrintable(QString::fromUtf8(QJsonDocument(names).toJson(QJsonDocument::Compact))));
        QCOMPARE(settings, before);
    }
}

void tst_menuschema::aNameTheFileHoldsIsKept(){
    const MenuSchema::Table &table = TableOf("RegularMenu");
    SettingsIO::Map settings;
    settings.insert(MenuSchema::Key(table), QStringLiteral("CopyUrl,My bookmarklet"));

    const QJsonArray entries = MenuSchema::DescribeTable(settings, table)[QStringLiteral("entries")].toArray();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[1].toObject()[QStringLiteral("label")].toString(), QStringLiteral("My bookmarklet"));
    QVERIFY(!entries[1].toObject()[QStringLiteral("known")].toBool());
    QVERIFY(entries[0].toObject()[QStringLiteral("known")].toBool());

    QCOMPARE(MenuSchema::Write(settings, table, Names({ QStringLiteral("My bookmarklet"),
                                                        QStringLiteral("NewViewNode") })), QString());
    QCOMPARE(settings.value(MenuSchema::Key(table)).toString(), QStringLiteral("My bookmarklet,NewViewNode"));
    QVERIFY(!MenuSchema::Write(settings, table, Names({ QStringLiteral("Another one") })).isEmpty());
}

void tst_menuschema::resetGivesTheDefaultBack(){
    const MenuSchema::Table &table = TableOf("MediaMenu");
    SettingsIO::Map settings;
    QCOMPARE(MenuSchema::Write(settings, table, Names({ QStringLiteral("OpenMedia") })), QString());
    MenuSchema::Reset(settings, table);
    QVERIFY(!settings.contains(MenuSchema::Key(table)));
    QCOMPARE(MenuSchema::Current(settings, table), Split(table.defaults));
    QVERIFY(!MenuSchema::DescribeTable(settings, table)[QStringLiteral("changed")].toBool());
}

void tst_menuschema::theViewKeepsWhatThePageWrote(){
    Settings &s = Application::GlobalSettings();
    const MenuSchema::Table &link = TableOf("LinkMenu");
    const MenuSchema::Table &text = TableOf("TextMenu");
    const MenuSchema::Table &regular = TableOf("RegularMenu");

    QCOMPARE(MenuSchema::Write(s, link, Names({ QStringLiteral("CopyLinkUrl"), QStringLiteral("OpenLink") })), QString());
    QCOMPARE(MenuSchema::Write(s, text, QJsonArray()), QString());
    QCOMPARE(MenuSchema::Write(s, regular, Names({ QStringLiteral("CopyUrl") })), QString());
    View::LoadSettings();
    QCOMPARE(View::GetLinkMenu(), QStringLiteral("CopyLinkUrl,OpenLink"));
    QCOMPARE(View::GetTextMenu(), QString());
    QCOMPARE(View::GetRegularMenu(), QStringLiteral("CopyUrl"));

    MenuSchema::Reset(s, regular);
    View::LoadSettings();
    View::SaveSettings();
    View::LoadSettings();
    QCOMPARE(s.value(MenuSchema::Key(link)).toString(), QStringLiteral("CopyLinkUrl,OpenLink"));
    QCOMPARE(s.value(MenuSchema::Key(text)).toString(), QString());
    QVERIFY(MenuSchema::Current(s, text).isEmpty());
    QVERIFY(!s.contains(MenuSchema::Key(regular)));
    QCOMPARE(View::GetRegularMenu(), regular.defaults);
    for(const char *name : { "ImageMenu", "MediaMenu", "SelectionMenu" })
        QVERIFY2(!s.contains(MenuSchema::Key(TableOf(name))), name);
    QVERIFY(!MenuSchema::DescribeTable(s, regular)[QStringLiteral("changed")].toBool());
}

void tst_menuschema::thePageShowsEachMenuWithItsIconsFirst(){
    SettingsIO::Map settings;
    const QJsonArray tables = MenuSchema::Describe(settings)[QStringLiteral("tables")].toArray();
    QCOMPARE(tables.size(), MenuSchema::Tables().size());
    QStringList order;
    for(const QJsonValue &value : tables){
        const QJsonObject table = value.toObject();
        order << table[QStringLiteral("menu")].toString() + QLatin1Char('/') + table[QStringLiteral("part")].toString()
                 + QLatin1Char('/') + table[QStringLiteral("group")].toString();
        QVERIFY(!table[QStringLiteral("menuLabel")].toString().isEmpty());
    }
    QCOMPARE(order, QStringList()
             << QStringLiteral("LinkMenu/icons/LinkIcons") << QStringLiteral("LinkMenu/items/LinkMenu")
             << QStringLiteral("ImageMenu/icons/ImageIcons") << QStringLiteral("ImageMenu/items/ImageMenu")
             << QStringLiteral("MediaMenu/icons/MediaIcons") << QStringLiteral("MediaMenu/items/MediaMenu")
             << QStringLiteral("TextMenu/icons/TextIcons") << QStringLiteral("TextMenu/items/TextMenu")
             << QStringLiteral("SelectionMenu/icons/SelectionIcons") << QStringLiteral("SelectionMenu/items/SelectionMenu")
             << QStringLiteral("RegularMenu/icons/PageIcons") << QStringLiteral("RegularMenu/items/RegularMenu")
             << QStringLiteral("TreeBarNodeMenu/icons/TreeBarNodeIcons") << QStringLiteral("TreeBarNodeMenu/items/TreeBarNodeMenu")
             << QStringLiteral("TableNodeMenu/icons/TableNodeIcons") << QStringLiteral("TableNodeMenu/items/TableNodeMenu"));
    const QJsonObject strings = MenuSchema::Describe(settings)[QStringLiteral("strings")].toObject();
    QVERIFY(!strings[QStringLiteral("icons")].toString().isEmpty());
    QVERIFY(!strings[QStringLiteral("items")].toString().isEmpty());

}

#ifdef WEBENGINEVIEW
int main(int argc, char **argv){
    QtWebEngineQuick::initialize();
    QApplication application(argc, argv);
    tst_menuschema test;
    return QTest::qExec(&test, argc, argv);
}
#else
QTEST_MAIN(tst_menuschema)
#endif
#include "tst_menuschema.moc"
