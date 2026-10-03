#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

#include "inputmap.hpp"
#include "inputmapschema.hpp"
#include "actionmapper.hpp"

#include "testsupport.hpp"

namespace {

typedef QList<QPair<QString, QString> > Pairs;

QString Source(const QString &path){
    QFile file(QStringLiteral(VANILLA_SOURCE_DIR "/") + path);
    if(!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
}

const InputMapSchema::Table &TableOf(const char *group){
    const InputMapSchema::Table *table = InputMapSchema::Find(QString::fromLatin1(group));
    Q_ASSERT(table);
    return *table;
}

QJsonObject Entry(const QString &input, const QString &action){
    QJsonObject entry;
    entry[QStringLiteral("input")] = input;
    entry[QStringLiteral("action")] = action;
    return entry;
}

#define TST_INPUTMAPSCHEMA_NAME(ACTION) << QStringLiteral(#ACTION)

}

class tst_inputmapschema : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){ TestSupport::SilenceDebugOutput();}

    void theTablesAreTheOnesTheClassesRead(){
        QMap<QString, QPair<InputMapSchema::Kind, InputMapSchema::Vocabulary> > read;
        const QList<QPair<QString, InputMapSchema::Vocabulary> > files = QList<QPair<QString, InputMapSchema::Vocabulary> >()
            << qMakePair(QStringLiteral("view/view.cpp"),       InputMapSchema::PageActions)
            << qMakePair(QStringLiteral("ui/treebank.cpp"),     InputMapSchema::TreeBankActions)
            << qMakePair(QStringLiteral("gadgets/gadgets.cpp"), InputMapSchema::GadgetsActions);
        const QRegularExpression load(QStringLiteral(
            "InputMap::Load(Key|Gesture)Map\\(s, QStringLiteral\\(\"([^\"]+)\"\\), \\w+, "
            "InputMap::Hooks\\(((?:\\w+::)?IsValidAction)\\)\\)"));

        typedef QPair<QString, InputMapSchema::Vocabulary> File;
        foreach(const File &file, files){
            const QString source = Source(file.first);
            QVERIFY2(!source.isEmpty(), qPrintable(file.first));
            QRegularExpressionMatchIterator it = load.globalMatch(source);
            while(it.hasNext()){
                const QRegularExpressionMatch match = it.next();
                const InputMapSchema::Kind kind = match.captured(1) == QStringLiteral("Key")
                    ? InputMapSchema::Keys : InputMapSchema::Mouse;
                const InputMapSchema::Vocabulary vocabulary =
                    match.captured(3) == QStringLiteral("Page::IsValidAction")
                    ? InputMapSchema::PageActions : file.second;
                read[match.captured(2)] = qMakePair(kind, vocabulary);
            }
        }
        QCOMPARE(read.size(), 10);
        QCOMPARE(InputMapSchema::Tables().size(), 10);

        foreach(const InputMapSchema::Table &table, InputMapSchema::Tables()){
            const QString group = QString::fromLatin1(table.group);
            QVERIFY2(read.contains(group), qPrintable(group));
            const InputMapSchema::Kind kind =
                table.kind == InputMapSchema::Gesture ? InputMapSchema::Mouse : table.kind;
            QVERIFY2(read[group].first == kind, qPrintable(group));
            QVERIFY2(read[group].second == table.vocabulary, qPrintable(group));
        }
    }

    void theVocabulariesAreTheActionMaps(){
        QCOMPARE(InputMapSchema::Actions(InputMapSchema::PageActions),
                 QStringList() PAGE_FOR_EACH_ACTION(TST_INPUTMAPSCHEMA_NAME));
        QCOMPARE(InputMapSchema::Actions(InputMapSchema::TreeBankActions),
                 QStringList() TREEBANK_FOR_EACH_ACTION(TST_INPUTMAPSCHEMA_NAME));
        QCOMPARE(InputMapSchema::Actions(InputMapSchema::GadgetsActions),
                 QStringList() GADGETS_FOR_EACH_ACTION(TST_INPUTMAPSCHEMA_NAME));
    }

    void anActionIsCalledWhatTheMenusCallIt(){
        const QRegularExpression define(QStringLiteral(
            "^\\s*DEFINE_ACTION\\((\\w+),\\s*tr\\(\"([^\"]*)\"\\)\\)"),
            QRegularExpression::MultilineOption);
        const QList<QPair<QString, InputMapSchema::Vocabulary> > files = QList<QPair<QString, InputMapSchema::Vocabulary> >()
            << qMakePair(QStringLiteral("view/page.cpp"),       InputMapSchema::PageActions)
            << qMakePair(QStringLiteral("ui/treebank.cpp"),     InputMapSchema::TreeBankActions)
            << qMakePair(QStringLiteral("gadgets/gadgets.cpp"), InputMapSchema::GadgetsActions);

        QSet<QString> defined;
        int compared = 0;
        typedef QPair<QString, InputMapSchema::Vocabulary> File;
        foreach(const File &file, files){
            QRegularExpressionMatchIterator it = define.globalMatch(Source(file.first));
            while(it.hasNext()){
                const QRegularExpressionMatch match = it.next();
                const QString action = match.captured(1);
                defined.insert(action);
                if(!InputMapSchema::IsAction(file.second, action)) continue;
                QCOMPARE(InputMapSchema::ActionLabel(file.second, action), match.captured(2));
                compared++;
            }
        }
        QVERIFY(compared > 300);

        foreach(InputMapSchema::Vocabulary vocabulary, QList<InputMapSchema::Vocabulary>()
                << InputMapSchema::PageActions << InputMapSchema::TreeBankActions
                << InputMapSchema::GadgetsActions){
            foreach(QString action, InputMapSchema::Actions(vocabulary)){
                if(action == QStringLiteral("NoAction")) continue;
                QVERIFY2(defined.contains(action), qPrintable(action));
            }
        }
    }

    void everyDefaultCanBePressed(){
        foreach(const InputMapSchema::Table &table, InputMapSchema::Tables()){
            const Pairs defaults = InputMapSchema::Defaults(table);
            QVERIFY2(!defaults.isEmpty(), table.group);
            typedef QPair<QString, QString> Pair;
            foreach(const Pair &pair, defaults){
                QString canonical;
                switch(table.kind){
                case InputMapSchema::Keys:    canonical = InputMap::CanonicalKeyName(pair.first);     break;
                case InputMapSchema::Mouse:   canonical = InputMap::CanonicalMouseName(pair.first);   break;
                case InputMapSchema::Gesture: canonical = InputMap::CanonicalGestureName(pair.first); break;
                }
                QVERIFY2(canonical == pair.first,
                         qPrintable(QStringLiteral("%1: %2").arg(QString::fromLatin1(table.group), pair.first)));
                QVERIFY2(InputMapSchema::IsAction(table.vocabulary, pair.second), qPrintable(pair.second));
            }
        }
    }

    void theSettingsPageHasKeysFirst(){
        const QList<QPair<QString, QString> > places = QList<QPair<QString, QString> >()
            << qMakePair(QStringLiteral("view/webengine/webengineview.hpp"),
                         QStringLiteral("case QEvent::KeyPress:{"))
            << qMakePair(QStringLiteral("view/webengine/quickwebengineview.cpp"),
                         QStringLiteral("void QuickWebEngineView::keyPressEvent(QKeyEvent *ev){"));
        typedef QPair<QString, QString> Place;
        foreach(const Place &place, places){
            const QString source = Source(place.first);
            const int start = source.indexOf(place.second);
            QVERIFY2(start >= 0, qPrintable(place.first));
            const int settings = source.indexOf(QStringLiteral("IsSettingsUrl("), start);
            const int modifier = source.indexOf(QStringLiteral("Application::HasAnyModifier("), start);
            QVERIFY2(settings > start && modifier > settings, qPrintable(place.first));
        }

        const QList<QPair<QString, QString> > restores = QList<QPair<QString, QString> >()
            << qMakePair(QStringLiteral("view/webengine/webengineview.cpp"),
                         QStringLiteral("bool WebEngineView::RestoreScroll(){"))
            << qMakePair(QStringLiteral("view/webengine/quickwebengineview.cpp"),
                         QStringLiteral("bool QuickWebEngineView::RestoreScroll(){"));
        foreach(const Place &place, restores){
            const QString source = Source(place.first);
            const int start = source.indexOf(place.second);
            QVERIFY2(start >= 0, qPrintable(place.first));
            const int settings = source.indexOf(QStringLiteral("IsSettingsUrl(url())) return false;"), start);
            const int end = source.indexOf(QStringLiteral("\n}"), start);
            QVERIFY2(settings > start && settings < end, qPrintable(place.first));
        }
    }

    void nothingInTheFileIsTheDefaults(){
        SettingsIO::Map settings;
        foreach(const InputMapSchema::Table &table, InputMapSchema::Tables())
            QCOMPARE(InputMapSchema::Current(settings, table), InputMapSchema::Defaults(table));
    }

    void aWriteReadsBackInTheSpellingOfApress(){
        const InputMapSchema::Table &mouse = TableOf("webview/mouse");
        SettingsIO::Map settings;
        QCOMPARE(InputMapSchema::Write(settings, mouse, QJsonArray()
                                       << Entry(QStringLiteral("Ctrl+Shift+WheelUp"), QStringLiteral("ZoomIn"))
                                       << Entry(QStringLiteral("ExtraButton1"), QStringLiteral("Back"))),
                 QString());
        QCOMPARE(settings.value(QStringLiteral("webview/mouse/Shift+Ctrl+WheelUp")).toString(), QStringLiteral("ZoomIn"));

        Pairs expected;
        expected << qMakePair(QStringLiteral("ExtraButton1"), QStringLiteral("Back"))
                 << qMakePair(QStringLiteral("Shift+Ctrl+WheelUp"), QStringLiteral("ZoomIn"));
        QCOMPARE(InputMapSchema::Current(settings, mouse), expected);

        InputMap::GestureMap map;
        QVERIFY(InputMap::LoadGestureMap(settings, QStringLiteral("webview/mouse"), map));
        QCOMPARE(map.value(QStringLiteral("Shift+Ctrl+WheelUp")), QStringLiteral("ZoomIn"));
        QCOMPARE(map.size(), 2);
    }

    void aKeyIsWrittenAsTheFileSpellsIt(){
        const InputMapSchema::Table &keys = TableOf("webview/keymap");
        SettingsIO::Map settings;
        QCOMPARE(InputMapSchema::Write(settings, keys, QJsonArray()
                                       << Entry(QStringLiteral("ctrl+/"), QStringLiteral("OpenCommand"))),
                 QString());
        QCOMPARE(settings.keys(), QStringList() << QStringLiteral("webview/keymap/Ctrl+Slash"));

        InputMap::KeyMap map;
        InputMap::LoadKeyMap(settings, QStringLiteral("webview/keymap"), map);
        QCOMPARE(map.value(QKeySequence(Qt::CTRL | Qt::Key_Slash)), QStringLiteral("OpenCommand"));
    }

    void anEmptiedTableStaysEmpty(){
        foreach(const InputMapSchema::Table &table, InputMapSchema::Tables()){
            SettingsIO::Map settings;
            QCOMPARE(InputMapSchema::Write(settings, table, QJsonArray()), QString());
            QVERIFY2(InputMapSchema::Current(settings, table).isEmpty(), table.group);

            const QString group = QString::fromLatin1(table.group);
            if(table.kind == InputMapSchema::Keys){
                InputMap::KeyMap map;
                InputMap::LoadKeyMap(settings, group, map);
                InputMap::SaveKeyMap(settings, group, map);
            } else {
                InputMap::GestureMap map;
                InputMap::LoadGestureMap(settings, group, map);
                InputMap::SaveGestureMap(settings, group, map);
            }
            QVERIFY2(InputMapSchema::Current(settings, table).isEmpty(), table.group);
        }
    }

    void aResetIsTheDefaults(){
        const InputMapSchema::Table &gestures = TableOf("webview/rightgesture");
        SettingsIO::Map settings;
        QCOMPARE(InputMapSchema::Write(settings, gestures, QJsonArray()), QString());
        InputMapSchema::Reset(settings, gestures);
        QCOMPARE(InputMapSchema::Current(settings, gestures), InputMapSchema::Defaults(gestures));
    }

    void aResetReachesTheTableTheClassHolds(){
        const InputMapSchema::Table &keys = TableOf("webview/keymap");
        const QString group = QStringLiteral("webview/keymap");
        SettingsIO::Map settings;
        QCOMPARE(InputMapSchema::Write(settings, keys, QJsonArray()
                                       << Entry(QStringLiteral("Ctrl+Shift+F9"), QStringLiteral("NewViewNode"))),
                 QString());

        typedef QPair<QString, QString> Pair;
        InputMap::KeyMap live;
        const auto load = [&](){
            foreach(const Pair &pair, InputMapSchema::Defaults(keys))
                live[QKeySequence(pair.first)] = pair.second;
            InputMap::LoadKeyMap(settings, group, live);
        };
        load();
        QVERIFY(live.contains(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F9)));

        InputMapSchema::Reset(settings, keys);
        load();
        QVERIFY(!live.contains(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F9)));
        InputMap::SaveKeyMap(settings, group, live);
        QCOMPARE(InputMapSchema::Current(settings, keys), InputMapSchema::Defaults(keys));
    }

    void aPlainClickCannotBeTaken(){
        const InputMapSchema::Table &mouse = TableOf("webview/mouse");
        const InputMapSchema::Table &thumbs = TableOf("gadgets/thumblist/mouse");
        SettingsIO::Map settings;
        QVERIFY(!InputMapSchema::Write(settings, mouse, QJsonArray()
                                       << Entry(QStringLiteral("LeftButton"), QStringLiteral("NoAction"))).isEmpty());
        QVERIFY(!InputMapSchema::Write(settings, mouse, QJsonArray()
                                       << Entry(QStringLiteral("RightButton"), QStringLiteral("Back"))).isEmpty());
        QVERIFY(!InputMapSchema::Write(settings, thumbs, QJsonArray()
                                       << Entry(QStringLiteral("LeftButton"), QStringLiteral("NoAction"))).isEmpty());
        QVERIFY(settings.isEmpty());

        SettingsIO::Map held;
        held.insert(QStringLiteral("webview/mouse/RightButton"), QStringLiteral("Back"));
        const QJsonObject entry = InputMapSchema::DescribeTable(held, mouse)
            [QStringLiteral("entries")].toArray().first().toObject();
        QCOMPARE(entry[QStringLiteral("input")].toString(), QStringLiteral("RightButton"));
        QCOMPARE(entry[QStringLiteral("known")].toBool(), false);
        QCOMPARE(InputMapSchema::Write(held, mouse, QJsonArray()
                                       << Entry(QStringLiteral("RightButton"), QStringLiteral("Back"))),
                 QString());
        QVERIFY(!InputMapSchema::Write(held, mouse, QJsonArray()
                                       << Entry(QStringLiteral("RightButton"), QStringLiteral("Forward"))).isEmpty());

        QCOMPARE(InputMapSchema::Write(settings, mouse, QJsonArray()
                                       << Entry(QStringLiteral("Ctrl+LeftButton"), QStringLiteral("NoAction"))
                                       << Entry(QStringLiteral("RightButton+LeftButton"), QStringLiteral("Back"))
                                       << Entry(QStringLiteral("MidButton"), QStringLiteral("Close"))),
                 QString());
    }

    void aWriteOfWhatCannotBeDoneWritesNothing(){
        const InputMapSchema::Table &gestures = TableOf("webview/rightgesture");
        const InputMapSchema::Table &thumbs = TableOf("gadgets/thumblist/keymap");
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("webview/rightgesture/U"), QStringLiteral("Back"));
        const SettingsIO::Map before = settings;

        QVERIFY(!InputMapSchema::Write(settings, gestures, QJsonArray()
                                       << Entry(QStringLiteral("D"), QStringLiteral("Reload"))
                                       << Entry(QStringLiteral("U,U"), QStringLiteral("Reload"))).isEmpty());
        QVERIFY(!InputMapSchema::Write(settings, gestures, QJsonArray()
                                       << Entry(QStringLiteral("D"), QStringLiteral("NoSuchAction"))).isEmpty());
        QVERIFY(!InputMapSchema::Write(settings, thumbs, QJsonArray()
                                       << Entry(QStringLiteral("F5"), QStringLiteral("Reload"))).isEmpty());
        QVERIFY(!InputMapSchema::Write(settings, gestures, QJsonArray() << QStringLiteral("U")).isEmpty());
        QCOMPARE(settings, before);
    }

    void anEntryTheFileAlreadyHoldsIsKept(){
        const InputMapSchema::Table &gestures = TableOf("webview/rightgesture");
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("webview/rightgesture/RU"), QStringLiteral("NextView"));
        settings.insert(QStringLiteral("webview/rightgesture/D"), QStringLiteral("Retired"));

        QCOMPARE(InputMapSchema::Write(settings, gestures, QJsonArray()
                                       << Entry(QStringLiteral("RU"), QStringLiteral("NextView"))
                                       << Entry(QStringLiteral("D"), QStringLiteral("Retired"))
                                       << Entry(QStringLiteral("L"), QStringLiteral("Back"))),
                 QString());
        QCOMPARE(settings.value(QStringLiteral("webview/rightgesture/RU")).toString(), QStringLiteral("NextView"));
        QCOMPARE(settings.value(QStringLiteral("webview/rightgesture/D")).toString(), QStringLiteral("Retired"));
        QCOMPARE(settings.value(QStringLiteral("webview/rightgesture/L")).toString(), QStringLiteral("Back"));

        QVERIFY(!InputMapSchema::Write(settings, gestures, QJsonArray()
                                       << Entry(QStringLiteral("RU"), QStringLiteral("Back"))).isEmpty());
    }

    void theLaterOfTheSameInputIsKept(){
        const InputMapSchema::Table &keys = TableOf("application/keymap");
        SettingsIO::Map settings;
        QCOMPARE(InputMapSchema::Write(settings, keys, QJsonArray()
                                       << Entry(QStringLiteral("Ctrl+W"), QStringLiteral("Close"))
                                       << Entry(QStringLiteral("F5"), QStringLiteral("Reload"))
                                       << Entry(QStringLiteral("ctrl+w"), QStringLiteral("Quit"))),
                 QString());
        QCOMPARE(settings.size(), 2);
        QCOMPARE(settings.value(QStringLiteral("application/keymap/Ctrl+W")).toString(), QStringLiteral("Quit"));
    }

    void aWriteTouchesItsOwnGroupOnly(){
        const InputMapSchema::Table &keys = TableOf("webview/keymap");
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("webview/keymap/F5"), QStringLiteral("Reload"));
        settings.insert(QStringLiteral("webview/keymapfoo/F5"), QStringLiteral("Reload"));
        settings.insert(QStringLiteral("webview/keymap/deeper/F5"), QStringLiteral("Reload"));
        settings.insert(QStringLiteral("webview/@EnableSingleKeyShortcut"), true);

        QCOMPARE(InputMapSchema::Write(settings, keys, QJsonArray()
                                       << Entry(QStringLiteral("F6"), QStringLiteral("Reload"))),
                 QString());
        QVERIFY(!settings.contains(QStringLiteral("webview/keymap/F5")));
        QVERIFY(settings.contains(QStringLiteral("webview/keymap/F6")));
        QVERIFY(settings.contains(QStringLiteral("webview/keymapfoo/F5")));
        QVERIFY(settings.contains(QStringLiteral("webview/keymap/deeper/F5")));
        QVERIFY(settings.contains(QStringLiteral("webview/@EnableSingleKeyShortcut")));
    }

    void theDescriptionMarksWhatIsChangedAndWhatIsHeldBack(){
        SettingsIO::Map settings;
        const InputMapSchema::Table &keys = TableOf("webview/keymap");
        QJsonObject described = InputMapSchema::DescribeTable(settings, keys);
        QCOMPARE(described[QStringLiteral("changed")].toBool(), false);

        QCOMPARE(InputMapSchema::Write(settings, keys, QJsonArray()
                                       << Entry(QStringLiteral("Ctrl+C"), QStringLiteral("Copy"))
                                       << Entry(QStringLiteral("K"), QStringLiteral("Print"))),
                 QString());
        described = InputMapSchema::DescribeTable(settings, keys);
        QCOMPARE(described[QStringLiteral("changed")].toBool(), true);
        QCOMPARE(described[QStringLiteral("group")].toString(), QStringLiteral("webview/keymap"));

        const QJsonArray entries = described[QStringLiteral("entries")].toArray();
        QCOMPARE(entries.size(), 2);
        foreach(QJsonValue value, entries){
            const QJsonObject entry = value.toObject();
            const bool k = entry[QStringLiteral("input")].toString() == QStringLiteral("K");
            QCOMPARE(entry[QStringLiteral("isDefault")].toBool(), !k);
            QCOMPARE(entry[QStringLiteral("single")].toBool(), k);
            QCOMPARE(entry[QStringLiteral("known")].toBool(), true);
        }

        const InputMapSchema::Table &gestures = TableOf("webview/rightgesture");
        QCOMPARE(InputMapSchema::Write(settings, gestures, QJsonArray()
                                       << Entry(QStringLiteral("U,DR"), QStringLiteral("Close"))),
                 QString());
        const QJsonObject gesture = InputMapSchema::DescribeTable(settings, gestures)
            [QStringLiteral("entries")].toArray().first().toObject();
        QCOMPARE(gesture[QStringLiteral("diagonal")].toBool(), true);

        const QJsonObject all = InputMapSchema::Describe(settings);
        QCOMPARE(all[QStringLiteral("singleKeysOff")].toBool(), true);
        QCOMPARE(all[QStringLiteral("fourWayGestures")].toBool(), true);
        QCOMPARE(all[QStringLiteral("tables")].toArray().size(), 10);
    }
};

QTEST_MAIN(tst_inputmapschema)
#include "tst_inputmapschema.moc"
