#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QKeySequence>

#include "inputmap.hpp"
#include "application.hpp"

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

    void akeyWhichTypesIsAsingleKey(){
        QVERIFY(InputMap::IsSingleKey(QKeySequence(Qt::Key_Q)));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(Qt::Key_Slash)));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(Qt::Key_Semicolon)));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(Qt::Key_Space)));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(Qt::Key_1)));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(Qt::SHIFT | Qt::Key_N)));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(QStringLiteral("Z"))));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(QStringLiteral("Shift+Z"))));
        QVERIFY(InputMap::IsSingleKey(QKeySequence(QStringLiteral(","))));
    }

    void achordOrAkeyWhichTypesNothingIsNot(){
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::CTRL | Qt::Key_W)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::ALT | Qt::Key_X)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::META | Qt::Key_A)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::Key_F5)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::Key_Escape)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::Key_Left)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::Key_PageDown)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::Key_Backspace)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::SHIFT | Qt::Key_Backspace)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence(Qt::Key_G, Qt::Key_G)));
        QVERIFY(!InputMap::IsSingleKey(QKeySequence()));
    }

    void amouseNameIsSpelledTheWayApressIsBuilt(){
        QString press;
        Application::AddModifiersToString(press, Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier);
        Application::AddMouseButtonsToString(press, Qt::RightButton | Qt::MiddleButton);
        Application::AddMouseButtonToString(press, Qt::LeftButton);
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("MidButton+Alt+RightButton+Ctrl+Shift+LeftButton")), press);

        QString wheel;
        Application::AddModifiersToString(wheel, Qt::ControlModifier | Qt::ShiftModifier);
        Application::AddWheelDirectionToString(wheel, true);
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("Ctrl+Shift+WheelUp")), wheel);

        QString extra;
        Application::AddMouseButtonsToString(extra, Qt::ExtraButton2);
        Application::AddMouseButtonToString(extra, Qt::ExtraButton1);
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("ExtraButton2+ExtraButton1")), extra);

        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("RightButton+LeftButton")),
                 QStringLiteral("RightButton+LeftButton"));
    }

    void amouseNameNoPressMakesIsRefused(){
        QCOMPARE(InputMap::CanonicalMouseName(QString()), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("Ctrl")), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("WheelUp+LeftButton")), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("LeftButton+LeftButton")), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("Ctrl+Ctrl+WheelUp")), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("Ctrl+")), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("Hyper+LeftButton")), QString());
        QCOMPARE(InputMap::CanonicalMouseName(QStringLiteral("None")), QString());
    }

    void agestureIsStrokesTheRecognizerMakes(){
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("U")), QStringLiteral("U"));
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("D,R,UL")), QStringLiteral("D,R,UL"));
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("U,D,U")), QStringLiteral("U,D,U"));
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("U,U")), QString());
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("RU")), QString());
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("U,")), QString());
        QCOMPARE(InputMap::CanonicalGestureName(QString()), QString());
        QCOMPARE(InputMap::CanonicalGestureName(QStringLiteral("None")), QString());
    }

    void akeyNameIsTheSpellingTheFileHolds(){
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("ctrl+w")), QStringLiteral("Ctrl+W"));
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Shift+Ctrl+Z")), QStringLiteral("Ctrl+Shift+Z"));
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Ctrl+/")), QStringLiteral("Ctrl+/"));
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("F5")), QStringLiteral("F5"));
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Ctrl+K, Ctrl+C")), QStringLiteral("Ctrl+K, Ctrl+C"));
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Ctrl+W")),
                 QKeySequence(Qt::CTRL | Qt::Key_W).toString());

        QCOMPARE(InputMap::CanonicalKeyName(QString()), QString());
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("None")), QString());
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Ctrl+NoSuchKey")), QString());
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Ctrl")), QString());
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Alt")), QString());
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Shift")), QString());
        QCOMPARE(InputMap::CanonicalKeyName(QStringLiteral("Meta")), QString());
    }
};

QTEST_MAIN(tst_inputmap)
#include "tst_inputmap.moc"
