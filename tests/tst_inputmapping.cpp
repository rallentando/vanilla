#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QKeyEvent>
#include <QKeySequence>

#include "application.hpp"
#include "page.hpp"

#include "testsupport.hpp"

class tst_inputmapping : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void keySequenceOfAKeyPress();
    void keySequenceOfAKeyPress_data();

    void keySequenceOfAModifierAloneIsEmpty();
    void keySequenceOfAnUnknownKeyIsEmpty();
    void keySequenceFromAString();

    void classifiesKeys();

    void writesTheModifiersOfAGesture();
    void writesTheButtonsOfAGesture();
    void writesTheWheelDirectionOfAGesture();

    void translatesJavascriptKeyCodes();
    void javascriptKeyCodesDoNotRoundTrip();

    void exactMatchIsAnchored();
    void exactMatchIsAnchored_data();

    void tellsAUrlFromSomethingToSearchFor();
    void tellsAUrlFromSomethingToSearchFor_data();
};

void tst_inputmapping::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_inputmapping::keySequenceOfAKeyPress_data(){
    QTest::addColumn<int>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain")
        << int(Qt::Key_T) << Qt::KeyboardModifiers(Qt::NoModifier) << QStringLiteral("T");
    QTest::newRow("ctrl")
        << int(Qt::Key_T) << Qt::KeyboardModifiers(Qt::ControlModifier) << QStringLiteral("Ctrl+T");
    QTest::newRow("ctrl shift")
        << int(Qt::Key_T) << Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier)
        << QStringLiteral("Ctrl+Shift+T");
    QTest::newRow("alt arrow")
        << int(Qt::Key_Left) << Qt::KeyboardModifiers(Qt::AltModifier) << QStringLiteral("Alt+Left");
    QTest::newRow("function key")
        << int(Qt::Key_F10) << Qt::KeyboardModifiers(Qt::NoModifier) << QStringLiteral("F10");
    QTest::newRow("keypad")
        << int(Qt::Key_5) << Qt::KeyboardModifiers(Qt::KeypadModifier) << QStringLiteral("5");
}

void tst_inputmapping::keySequenceOfAKeyPress(){
    QFETCH(int, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QString, expected);

    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    QCOMPARE(Application::MakeKeySequence(&event).toString(), expected);
}

void tst_inputmapping::keySequenceOfAModifierAloneIsEmpty(){
    foreach(int key, QList<int>()
            << Qt::Key_Shift << Qt::Key_Control << Qt::Key_Meta << Qt::Key_Alt){

        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        QVERIFY(Application::IsOnlyModifier(&event));
        QCOMPARE(Application::MakeKeySequence(&event), QKeySequence());
    }
}

void tst_inputmapping::keySequenceOfAnUnknownKeyIsEmpty(){
    QKeyEvent event(QEvent::KeyPress, Qt::Key_unknown, Qt::ControlModifier);
    QCOMPARE(Application::MakeKeySequence(&event), QKeySequence());
}

void tst_inputmapping::keySequenceFromAString(){
    QCOMPARE(Application::MakeKeySequence(QStringLiteral("Ctrl+T")),
             QKeySequence(QStringLiteral("Ctrl+T")));

    QKeyEvent event(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(Application::MakeKeySequence(&event),
             Application::MakeKeySequence(QStringLiteral("Ctrl+T")));
}

void tst_inputmapping::classifiesKeys(){
    QKeyEvent f1(QEvent::KeyPress, Qt::Key_F1, Qt::NoModifier);
    QKeyEvent f35(QEvent::KeyPress, Qt::Key_F35, Qt::NoModifier);
    QKeyEvent letter(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QKeyEvent home(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);

    QVERIFY(Application::IsFunctionKey(&f1));
    QVERIFY(Application::IsFunctionKey(&f35));
    QVERIFY(!Application::IsFunctionKey(&letter));

    QVERIFY(Application::IsMoveKey(&space));
    QVERIFY(Application::IsMoveKey(&home));
    QVERIFY(!Application::IsMoveKey(&letter));

    QVERIFY(Application::HasNoModifier(&letter));

    QKeyEvent shifted(QEvent::KeyPress, Qt::Key_A, Qt::ShiftModifier);
    QVERIFY(Application::HasShiftModifier(&shifted));
    QVERIFY(!Application::HasAnyModifier(&shifted));

    QKeyEvent controlled(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
    QVERIFY(Application::HasAnyModifier(&controlled));
}

void tst_inputmapping::writesTheModifiersOfAGesture(){
    QString str;
    Application::AddModifiersToString(str, Qt::NoModifier);
    QCOMPARE(str, QString());

    str = QString();
    Application::AddModifiersToString(str, Qt::ControlModifier);
    QCOMPARE(str, QStringLiteral("Ctrl"));

    str = QString();
    Application::AddModifiersToString(str, Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    QCOMPARE(str.split(QStringLiteral("+")).length(), 3);
    QVERIFY(str.contains(QStringLiteral("Shift")));
    QVERIFY(str.contains(QStringLiteral("Ctrl")));
    QVERIFY(str.contains(QStringLiteral("Alt")));

    str = QStringLiteral("LeftButton");
    Application::AddModifiersToString(str, Qt::ControlModifier);
    QCOMPARE(str, QStringLiteral("LeftButton+Ctrl"));
}

void tst_inputmapping::writesTheButtonsOfAGesture(){
    QString str;
    Application::AddMouseButtonToString(str, Qt::LeftButton);
    QCOMPARE(str, QStringLiteral("LeftButton"));

    str = QString();
    Application::AddMouseButtonToString(str, Qt::RightButton);
    QCOMPARE(str, QStringLiteral("RightButton"));

    str = QString();
    Application::AddMouseButtonToString(str, Qt::MiddleButton);
    QCOMPARE(str, QStringLiteral("MidButton"));

    str = QString();
    Application::AddMouseButtonToString(str, Qt::NoButton);
    QCOMPARE(str, QString());

    str = QString();
    Application::AddMouseButtonsToString(str, Qt::LeftButton | Qt::RightButton);
    QCOMPARE(str.split(QStringLiteral("+")).length(), 2);
    QVERIFY(str.contains(QStringLiteral("LeftButton")));
    QVERIFY(str.contains(QStringLiteral("RightButton")));
}

void tst_inputmapping::writesTheWheelDirectionOfAGesture(){
    QString str;
    Application::AddWheelDirectionToString(str, true);
    QCOMPARE(str, QStringLiteral("WheelUp"));

    str = QString();
    Application::AddWheelDirectionToString(str, false);
    QCOMPARE(str, QStringLiteral("WheelDown"));

    str = QStringLiteral("Ctrl");
    Application::AddWheelDirectionToString(str, true);
    QCOMPARE(str, QStringLiteral("Ctrl+WheelUp"));
}

void tst_inputmapping::translatesJavascriptKeyCodes(){
    QCOMPARE(Application::JsKeyToQtKey(0x41), int(Qt::Key_A));
    QCOMPARE(Application::JsKeyToQtKey(0x30), int(Qt::Key_0));
    QCOMPARE(Application::JsKeyToQtKey(0x70), int(Qt::Key_F1));
    QCOMPARE(Application::JsKeyToQtKey(0x08), int(Qt::Key_Backspace));
    QCOMPARE(Application::JsKeyToQtKey(0x1B), int(Qt::Key_Escape));
    QCOMPARE(Application::JsKeyToQtKey(0x28), int(Qt::Key_Down));
    QCOMPARE(Application::JsKeyToQtKey(0x60), int(Qt::Key_0));

    QCOMPARE(Application::QtKeyToJsKey(Qt::Key_A), 0x41);
    QCOMPARE(Application::QtKeyToJsKey(Qt::Key_0), 0x30);
    QCOMPARE(Application::QtKeyToJsKey(Qt::Key_F1), 0x70);
    QCOMPARE(Application::QtKeyToJsKey(Qt::Key_Backspace), 0x08);
    QCOMPARE(Application::QtKeyToJsKey(Qt::Key_Escape), 0x1B);
    QCOMPARE(Application::QtKeyToJsKey(Qt::Key_Down), 0x28);
}

void tst_inputmapping::javascriptKeyCodesDoNotRoundTrip(){
    QCOMPARE(Application::JsKeyToQtKey(0x60), int(Qt::Key_0));
    QCOMPARE(Application::QtKeyToJsKey(Application::JsKeyToQtKey(0x60)), 0x30);
}

void tst_inputmapping::exactMatchIsAnchored_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("command");
    QTest::addColumn<bool>("matches");

    const QString newViewNode = QStringLiteral("[nN]ew(?:[vV]iew)?(?:[nN]ode)?");

    QTest::newRow("plain")        << newViewNode << QStringLiteral("new")         << true;
    QTest::newRow("capital")      << newViewNode << QStringLiteral("New")         << true;
    QTest::newRow("long")         << newViewNode << QStringLiteral("newViewNode") << true;
    QTest::newRow("half")         << newViewNode << QStringLiteral("newView")     << true;
    QTest::newRow("prefix")       << newViewNode << QStringLiteral("newest")      << false;
    QTest::newRow("suffix")       << newViewNode << QStringLiteral("renew")       << false;
    QTest::newRow("space")        << newViewNode << QStringLiteral("new ")        << false;
    QTest::newRow("empty")        << newViewNode << QString()                     << false;
    QTest::newRow("wrong case")   << newViewNode << QStringLiteral("NEW")         << false;

    QTest::newRow("close")
        << QStringLiteral("[cC]lose") << QStringLiteral("close") << true;
    QTest::newRow("clone is not close")
        << QStringLiteral("[cC]lose") << QStringLiteral("clone") << false;
}

void tst_inputmapping::exactMatchIsAnchored(){
    QFETCH(QString, pattern);
    QFETCH(QString, command);
    QFETCH(bool, matches);

    QCOMPARE(Application::ExactMatch(pattern, command), matches);
}

void tst_inputmapping::tellsAUrlFromSomethingToSearchFor_data(){
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("url");

    QTest::newRow("http")
        << QStringLiteral("http://example.com/") << QStringLiteral("http://example.com/");
    QTest::newRow("https")
        << QStringLiteral("https://example.com/") << QStringLiteral("https://example.com/");
    QTest::newRow("about")
        << QStringLiteral("about:blank") << QStringLiteral("about:blank");
    QTest::newRow("file")
        << QStringLiteral("file:///c:/x.txt") << QStringLiteral("file:///c:/x.txt");

    QTest::newRow("chrome")
        << QStringLiteral("chrome://gpu") << QStringLiteral("chrome://gpu");
    QTest::newRow("chrome with path")
        << QStringLiteral("chrome://net-internals/#dns")
        << QStringLiteral("chrome://net-internals/#dns");

    QTest::newRow("vanilla")
        << QStringLiteral("vanilla://settings") << QStringLiteral("vanilla://settings");
    QTest::newRow("vanilla with slash")
        << QStringLiteral("vanilla://settings/") << QStringLiteral("vanilla://settings/");
    QTest::newRow("vanilla directory")
        << QStringLiteral("vanilla://directory") << QStringLiteral("vanilla://directory");

    QTest::newRow("bare domain")
        << QStringLiteral("example.com") << QStringLiteral("http://example.com");

    QTest::newRow("one word")   << QStringLiteral("settings")        << QString();
    QTest::newRow("two words")  << QStringLiteral("how to do a thing") << QString();
    QTest::newRow("empty")      << QString()                         << QString();
}

void tst_inputmapping::tellsAUrlFromSomethingToSearchFor(){
    QFETCH(QString, typed);
    QFETCH(QString, url);

    const QList<QUrl> urls = Page::ExtractUrlsFromText(typed);

    if(url.isEmpty()){
        QVERIFY2(urls.isEmpty(),
                 qPrintable(QStringLiteral("%1 was read as the url %2")
                            .arg(typed, urls.isEmpty() ? QString() : urls.first().toString())));
        return;
    }
    QCOMPARE(urls.length(), 1);
    QCOMPARE(urls.first().toString(), url);
}

QTEST_MAIN(tst_inputmapping)
#include "tst_inputmapping.moc"
