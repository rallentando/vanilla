#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QKeySequence>

#include "inputmap.hpp"

#include "testsupport.hpp"

namespace {

QString Entry(const QString &group, const QString &name){
    return group + QStringLiteral("/") + name;
}

const QString KEYS = QStringLiteral("application/keymap");
const QString MICE = QStringLiteral("application/mouse");

}

class tst_inputmap : public QObject {
    Q_OBJECT

private:
    static InputMap::KeyMap RoundTrip(const InputMap::KeyMap &map){
        SettingsIO::Map settings;
        InputMap::SaveKeyMap(settings, KEYS, map);
        InputMap::KeyMap back;
        InputMap::LoadKeyMap(settings, KEYS, back);
        return back;
    }

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
    }

    void aseparatorInAkeyNameIsWrittenAsAword(){
        QCOMPARE(InputMap::EscapeKeyName(QStringLiteral("/")), QStringLiteral("Slash"));
        QCOMPARE(InputMap::EscapeKeyName(QStringLiteral("\\")), QStringLiteral("Backslash"));
        QCOMPARE(InputMap::EscapeKeyName(QStringLiteral("Ctrl+/")), QStringLiteral("Ctrl+Slash"));
        QCOMPARE(InputMap::EscapeKeyName(QStringLiteral("Ctrl+W")), QStringLiteral("Ctrl+W"));
    }

    void keyNamesWithAseparatorSurviveTheRoundTrip(){
        const QStringList names = QStringList()
            << QStringLiteral("/") << QStringLiteral("\\")
            << QStringLiteral("Ctrl+/") << QStringLiteral("Ctrl+Shift+\\")
            << QStringLiteral("Ctrl+W") << QStringLiteral("F5")
            << QStringLiteral("Alt+Left") << QStringLiteral(",");

        foreach(QString name, names)
            QCOMPARE(InputMap::UnescapeKeyName(InputMap::EscapeKeyName(name)), name);

        foreach(QString name, names)
            QVERIFY(!InputMap::EscapeKeyName(name).contains(QStringLiteral("/")) &&
                    !InputMap::EscapeKeyName(name).contains(QStringLiteral("\\")));
    }

    void abindingSurvivesAwriteAndAread(){
        InputMap::KeyMap map;
        map[QKeySequence(Qt::CTRL | Qt::Key_W)] = QStringLiteral("Close");
        map[QKeySequence(Qt::Key_F5)]           = QStringLiteral("Reload");
        map[QKeySequence(Qt::ALT | Qt::Key_Left)] = QStringLiteral("Back");

        QCOMPARE(RoundTrip(map), map);
    }

    void abindingOnAseparatorKeySurvivesToo(){
        InputMap::KeyMap map;
        map[QKeySequence(Qt::Key_Slash)]                  = QStringLiteral("NoAction");
        map[QKeySequence(Qt::Key_Backslash)]              = QStringLiteral("Stop");
        map[QKeySequence(Qt::CTRL | Qt::Key_Slash)]       = QStringLiteral("OpenCommand");

        QCOMPARE(RoundTrip(map), map);
    }

    void anactionNameIsWrittenAsItIs(){
        SettingsIO::Map settings;
        InputMap::KeyMap map;
        map[QKeySequence(Qt::Key_F5)] = QStringLiteral("Reload");
        InputMap::SaveKeyMap(settings, KEYS, map);

        QCOMPARE(settings.value(Entry(KEYS, QStringLiteral("F5"))).toString(), QStringLiteral("Reload"));
    }

    void agestureSurvivesAwriteAndAread(){
        InputMap::GestureMap map;
        map[QStringLiteral("LeftButton")]            = QStringLiteral("ClickElement");
        map[QStringLiteral("Ctrl+MiddleButton")]     = QStringLiteral("OpenInNewViewNode");
        map[QStringLiteral("RU")]                    = QStringLiteral("NextView");
        map[QStringLiteral("WheelUp")]               = QStringLiteral("ZoomIn");

        SettingsIO::Map settings;
        InputMap::SaveGestureMap(settings, MICE, map);
        InputMap::GestureMap back;
        QVERIFY(InputMap::LoadGestureMap(settings, MICE, back));
        QCOMPARE(back, map);
    }

    void anemptyBindingIsNotWritten(){
        InputMap::KeyMap map;
        map[QKeySequence()]                     = QStringLiteral("Close");
        map[QKeySequence(Qt::CTRL | Qt::Key_W)] = QStringLiteral("Close");

        SettingsIO::Map settings;
        InputMap::SaveKeyMap(settings, KEYS, map);
        QCOMPARE(settings.keys(), QStringList() << Entry(KEYS, QStringLiteral("Ctrl+W")));

        InputMap::GestureMap gestures;
        gestures[QString()]                    = QStringLiteral("Close");
        gestures[QStringLiteral("LeftButton")] = QStringLiteral("ClickElement");

        SettingsIO::Map two;
        InputMap::SaveGestureMap(two, MICE, gestures);
        QCOMPARE(two.keys(), QStringList() << Entry(MICE, QStringLiteral("LeftButton")));
    }

    void anemptyGroupLeavesTheDefaultsAlone(){
        InputMap::KeyMap map;
        map[QKeySequence(Qt::Key_F5)] = QStringLiteral("Reload");

        SettingsIO::Map settings;
        settings.insert(QStringLiteral("application/@ColorScheme"), QStringLiteral("dark"));

        QVERIFY(!InputMap::LoadKeyMap(settings, KEYS, map));
        QCOMPARE(map.value(QKeySequence(Qt::Key_F5)), QStringLiteral("Reload"));
    }

    void agroupWhichIsThereReplacesTheDefaults(){
        InputMap::KeyMap map;
        map[QKeySequence(Qt::Key_F5)] = QStringLiteral("Reload");

        SettingsIO::Map settings;
        settings.insert(Entry(KEYS, QStringLiteral("Ctrl+W")), QStringLiteral("Close"));

        QVERIFY(InputMap::LoadKeyMap(settings, KEYS, map));
        QCOMPARE(map.keys(), QList<QKeySequence>() << QKeySequence(Qt::CTRL | Qt::Key_W));
    }

    void anunknownActionInTheDefaultsIsReported(){
        InputMap::KeyMap map;
        map[QKeySequence(Qt::Key_F5)] = QStringLiteral("Reload");
        map[QKeySequence(Qt::Key_F6)] = QStringLiteral("NoSuchAction");

        QStringList reported;
        InputMap::Hooks hooks;
        hooks.IsValidAction = [](QString a){ return a == QStringLiteral("Reload");};
        hooks.ReportInvalidDefault = [&reported](QString a){ reported << a;};

        InputMap::LoadKeyMap(SettingsIO::Map(), KEYS, map, hooks);
        QCOMPARE(reported, QStringList() << QStringLiteral("NoSuchAction"));

        SettingsIO::Map settings;
        settings.insert(Entry(KEYS, QStringLiteral("Ctrl+W")), QStringLiteral("NoSuchAction"));
        reported.clear();
        InputMap::LoadKeyMap(settings, KEYS, map, hooks);
        QVERIFY(reported.isEmpty());
    }

    void agroupIsAwholePathComponent(){
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("webview/keymap/Ctrl+W"),   QStringLiteral("Close"));
        settings.insert(QStringLiteral("webview/keymapfoo/Ctrl+X"),QStringLiteral("Cut"));
        settings.insert(QStringLiteral("webview/keymap"),          QStringLiteral("not a binding"));
        settings.insert(QStringLiteral("webview/mouse/LeftButton"),QStringLiteral("ClickElement"));

        QCOMPARE(InputMap::NamesUnder(settings, QStringLiteral("webview/keymap")),
                 QStringList() << QStringLiteral("Ctrl+W"));
    }

    void anestedGroupDoesNotCollectItsChildren(){
        SettingsIO::Map settings;
        settings.insert(QStringLiteral("gadgets/accesskey/keymap/A"), QStringLiteral("ClickElement"));
        settings.insert(QStringLiteral("gadgets/accesskey/@AccessKeyMode"), QStringLiteral("BothHands"));

        QCOMPARE(InputMap::NamesUnder(settings, QStringLiteral("gadgets/accesskey/keymap")),
                 QStringList() << QStringLiteral("A"));
        QCOMPARE(InputMap::NamesUnder(settings, QStringLiteral("gadgets/accesskey")),
                 QStringList() << QStringLiteral("@AccessKeyMode"));
    }

    void afullMapIsStableAcrossTwoRounds(){
        InputMap::KeyMap map;
        map[QKeySequence(QKeySequence::Copy)]        = QStringLiteral("Copy");
        map[QKeySequence(Qt::CTRL | Qt::Key_W)]      = QStringLiteral("Close");
        map[QKeySequence(Qt::Key_Slash)]             = QStringLiteral("NoAction");
        map[QKeySequence(Qt::Key_Backslash)]         = QStringLiteral("Stop");
        map[QKeySequence(Qt::ALT | Qt::Key_X)]       = QStringLiteral("OpenCommand");
        map[QKeySequence(Qt::Key_Comma)]             = QStringLiteral("Back");

        SettingsIO::Map first;
        InputMap::SaveKeyMap(first, KEYS, map);

        InputMap::KeyMap read;
        QVERIFY(InputMap::LoadKeyMap(first, KEYS, read));

        SettingsIO::Map second;
        InputMap::SaveKeyMap(second, KEYS, read);

        QCOMPARE(second, first);
        QCOMPARE(read, map);
    }
};

QTEST_MAIN(tst_inputmap)
#include "tst_inputmap.moc"
