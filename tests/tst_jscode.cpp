#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QJSEngine>
#include <QJSValue>
#include <QPoint>
#include <QPointF>
#include <QRegularExpression>

#include "view.hpp"
#include "page.hpp"

#include "testsupport.hpp"

class tst_jscode : public QObject {
    Q_OBJECT

private:
    typedef QPair<QString, QString> NamedScript;

    static QList<NamedScript> Helpers(){
        const QString xpath = QStringLiteral("//div[@id=\"a\"]/input");
        const QString text  = QStringLiteral("hello");
        const QString data  = QStringLiteral("{\"user\":\"a\"}");
        const QPoint  pos   = QPoint(12, 34);
        const QPointF ratio = QPointF(0.25, 0.5);

        QList<QEvent::Type> types;
        types << QEvent::KeyPress << QEvent::KeyRelease
              << QEvent::MouseMove << QEvent::MouseButtonPress
              << QEvent::MouseButtonRelease << QEvent::Wheel;

        QList<NamedScript> list;
#define ADD(exp) list << NamedScript(QStringLiteral(#exp), View::exp)
        ADD(GetBaseUrlJsCode());
        ADD(GetCurrentBaseUrlJsCode());
        ADD(UpKeyEventJsCode());
        ADD(DownKeyEventJsCode());
        ADD(RightKeyEventJsCode());
        ADD(LeftKeyEventJsCode());
        ADD(PageDownKeyEventJsCode());
        ADD(PageUpKeyEventJsCode());
        ADD(HomeKeyEventJsCode());
        ADD(EndKeyEventJsCode());
        ADD(SetFocusToElementJsCode(xpath));
        ADD(FireClickEventJsCode(xpath, pos));
        ADD(GetScrollValuePointJsCode());
        ADD(SetScrollValuePointJsCode(pos));
#ifdef MEDIATIME
        ADD(GetMediaTimeJsCode());
        ADD(SetMediaTimeJsCode(12.5f));
#endif
        ADD(GetScrollBarStateJsCode());
        ADD(GetScrollRatioPointJsCode());
        ADD(SetScrollRatioPointJsCode(ratio));
        ADD(HitElementJsCode(pos));
        ADD(HitLinkUrlJsCode(pos));
        ADD(HitImageUrlJsCode(pos));
        ADD(SelectedTextJsCode());
        ADD(SelectedHtmlJsCode());
        ADD(WholeTextJsCode());
        ADD(WholeHtmlJsCode());
        ADD(SelectionRegionJsCode());
        ADD(SetTextValueJsCode(xpath, text));
        ADD(ExecCommandJsCode(QStringLiteral("insertOrderedList")));
        ADD(ChangeTextDirectionJsCode(QStringLiteral("rtl")));
        ADD(ToggleMediaControlsJsCode());
        ADD(ToggleMediaLoopJsCode());
        ADD(ToggleMediaPlayPauseJsCode());
        ADD(InstallWebChannelJsCode());
        ADD(InstallSubmitEventJsCode());
        ADD(DecorateFormFieldJsCode(data));
        ADD(SubmitFormDataJsCode(data));
        ADD(InstallEventFilterJsCode(types));
#ifdef EDGEWEBVIEW
        ADD(EdgeInputBridgeJsCode(QList<int>() << 81 << 87, QList<int>() << 81));
#endif
#undef ADD
        static const QList<Page::FindElementsOption> options =
            QList<Page::FindElementsOption>()
            << Page::ForAccessKey << Page::HaveSource << Page::HaveReference
            << Page::RelIsNext << Page::RelIsPrev;
        foreach(Page::FindElementsOption option, options){
            list << NamedScript
                (QStringLiteral("FindElementsJsCode(%1)").arg(int(option)),
                 View::FindElementsJsCode(option));
        }
        return list;
    }

    static QString ParseError(QJSEngine &engine, const QString &script){
        const QJSValue value = engine.evaluate
            (QStringLiteral("(function(){\n") + script + QStringLiteral("\n})"));
        if(value.isError())
            return value.property(QStringLiteral("message")).toString();
        return QString();
    }

private slots:
    void everyHelperIsJavascript(){
        QJSEngine engine;
        const QList<NamedScript> helpers = Helpers();
        int expected = 41;
#ifdef MEDIATIME
        expected += 2;
#endif
#ifdef EDGEWEBVIEW
        expected += 1;
#endif
        QCOMPARE(helpers.length(), expected);
        foreach(const NamedScript &helper, helpers){
            QVERIFY2(!helper.second.isEmpty(),
                     qPrintable(helper.first + QStringLiteral(" is empty")));
            const QString error = ParseError(engine, helper.second);
            QVERIFY2(error.isEmpty(),
                     qPrintable(helper.first + QStringLiteral(": ") + error));
        }
    }

#ifdef EDGEWEBVIEW

    void theInputBridgeTakesKeysOnlyInTheTopDocument(){
        QJSEngine engine;

        const QString script =
            View::EdgeInputBridgeJsCode(QList<int>() << 81, QList<int>());

        const QString harness = QStringLiteral(
            "(function(isTop){\n"
            "  var window, document;\n"
            "  var out = { reported: 0, prevented: 0, listened: false, print: false };\n"
            "  var handler = null;\n"
            "  document = { activeElement: null,\n"
            "               addEventListener: function(t, f){\n"
            "                   if(t === 'keydown'){ handler = f; out.listened = true; } } };\n"
            "  window = { chrome: { webview: {\n"
            "                 postMessage: function(m){ out.reported++; } } } };\n"
            "  window.top = isTop ? window : {};\n"
            "%1\n"
            "  out.print = (typeof window.print === 'function');\n"
            "  if(handler) handler({ keyCode: 81, shiftKey: false, repeat: false,\n"
            "                        isComposing: false, ctrlKey: false,\n"
            "                        altKey: false, metaKey: false,\n"
            "                        preventDefault: function(){ out.prevented++; } });\n"
            "  return out;\n"
            "})").arg(script);

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));

        const QJSValue top = fn.call(QJSValueList() << QJSValue(true));
        QVERIFY2(!top.isError(),
                 qPrintable(top.property(QStringLiteral("message")).toString()));
        QVERIFY(top.property(QStringLiteral("listened")).toBool());
        QCOMPARE(top.property(QStringLiteral("reported")).toInt(), 1);
        QCOMPARE(top.property(QStringLiteral("prevented")).toInt(), 1);

        const QJSValue frame = fn.call(QJSValueList() << QJSValue(false));
        QVERIFY2(!frame.isError(),
                 qPrintable(frame.property(QStringLiteral("message")).toString()));
        QVERIFY(!frame.property(QStringLiteral("listened")).toBool());
        QCOMPARE(frame.property(QStringLiteral("reported")).toInt(), 0);
        QCOMPARE(frame.property(QStringLiteral("prevented")).toInt(), 0);

        QVERIFY(top.property(QStringLiteral("print")).toBool());
        QVERIFY(frame.property(QStringLiteral("print")).toBool());
    }

    void theScrollReporterRepeatsItselfWhenTheUrlMoves(){
        QJSEngine engine;
        const QString script = View::EdgeScrollReportJsCode();

        const QString harness = QStringLiteral(
            "(function(){\n"
            "  var window, document, location;\n"
            "  var out = { posted: 0 };\n"
            "  var scroll = null;\n"
            "  var de = { scrollWidth: 800, scrollHeight: 2000 };\n"
            "  document = { documentElement: de, body: null,\n"
            "               addEventListener: function(){} };\n"
            "  location = { href: 'https://a/?q=1' };\n"
            "  window = { chrome: { webview: {\n"
            "                 postMessage: function(m){ out.posted++; } } },\n"
            "             pageXOffset: 0, pageYOffset: 100,\n"
            "             innerWidth: 800, innerHeight: 600,\n"
            "             addEventListener: function(t, f){\n"
            "                 if(t === 'scroll') scroll = f; } };\n"
            "  window.top = window;\n"
            "  var requestAnimationFrame = function(f){ f(); };\n"
            "  var setInterval = function(){};\n"
            "%1\n"
            "  scroll();\n"
            "  out.first = out.posted;\n"
            "  scroll();\n"
            "  out.unchanged = out.posted;\n"
            "  location.href = 'https://a/?q=2';\n"
            "  scroll();\n"
            "  out.moved = out.posted;\n"
            "  window.pageYOffset = 200;\n"
            "  scroll();\n"
            "  out.scrolled = out.posted;\n"
            "  return out;\n"
            "})").arg(script);

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));
        const QJSValue out = fn.call();
        QVERIFY2(!out.isError(),
                 qPrintable(out.property(QStringLiteral("message")).toString()));
        QCOMPARE(out.property(QStringLiteral("first")).toInt(), 1);
        QCOMPARE(out.property(QStringLiteral("unchanged")).toInt(), 1);
        QCOMPARE(out.property(QStringLiteral("moved")).toInt(), 2);
        QCOMPARE(out.property(QStringLiteral("scrolled")).toInt(), 3);
    }
#endif

    void noPlaceholderSurvives(){
        static const QRegularExpression placeholder
            (QStringLiteral("%[1-9][0-9]*"));
        foreach(const NamedScript &helper, Helpers()){
            const QRegularExpressionMatch match =
                placeholder.match(helper.second);
            QVERIFY2(!match.hasMatch(),
                     qPrintable(helper.first + QStringLiteral(" left ") +
                                match.captured()));
        }
    }

    void textReachesTheElement_data(){
        QTest::addColumn<QString>("value");

        QTest::newRow("plain")           << QStringLiteral("hello");
        QTest::newRow("quote")           << QStringLiteral("a\"b");
        QTest::newRow("tab")             << QStringLiteral("a\tb");
        QTest::newRow("placeholder")     << QStringLiteral("a%1b%2c");
        QTest::newRow("script close")    << QStringLiteral("a</script>b");
        QTest::newRow("japanese")        << QStringLiteral("あいう");

        QTest::newRow("backslash")       << QStringLiteral("a\\b");
        QTest::newRow("trailing back")   << QStringLiteral("a\\");
        QTest::newRow("escaped quote")   << QStringLiteral("a\\\"b");
        QTest::newRow("newline")         << QStringLiteral("a\nb");
        QTest::newRow("carriage return") << QStringLiteral("a\rb");
        QTest::newRow("two urls")        << QStringLiteral("http://a/\nhttp://b/");
        QTest::newRow("line separator")
            << (QString(QChar(0x2028)) + QStringLiteral("b"));
        QTest::newRow("para separator")
            << (QString(QChar(0x2029)) + QStringLiteral("b"));
        QTest::newRow("nul")     << (QString(QChar(0x0000)) + QStringLiteral("b"));
        QTest::newRow("c0 low")  << (QString(QChar(0x0001)) + QStringLiteral("b"));
        QTest::newRow("c0 high") << (QString(QChar(0x001F)) + QStringLiteral("b"));
        QTest::newRow("backspace")   << QStringLiteral("a\bb");
        QTest::newRow("form feed")   << QStringLiteral("a\fb");
    }

    void textReachesTheElement(){
        QFETCH(QString, value);

        QJSEngine engine;
        engine.evaluate(QStringLiteral(
            "var given = null;\n"
            "var asked = [];\n"
            "var elem = { setAttribute: function(name, v){ given = v;},\n"
            "             focus: function(){}, contentDocument: null };\n"
            "var document = { evaluate: function(xpath){\n"
            "    asked.push(xpath);\n"
            "    return { snapshotItem: function(){ return elem;}};\n"
            "}};\n"));

        const QJSValue result = engine.evaluate
            (View::SetTextValueJsCode(QStringLiteral("//a[1],//b[2]"), value));

        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QCOMPARE(engine.globalObject().property(QStringLiteral("given")).toString(),
                 value);

        const QJSValue asked = engine.globalObject().property(QStringLiteral("asked"));
        QCOMPARE(asked.property(QStringLiteral("length")).toInt(), 2);
        QCOMPARE(asked.property(0).toString(), QStringLiteral("//a[1]"));
        QCOMPARE(asked.property(1).toString(), QStringLiteral("//b[2]"));
    }

    void aPlaceholderInTheXpathDoesNotEatTheText(){
        QJSEngine engine;
        engine.evaluate(QStringLiteral(
            "var given = null;\n"
            "var asked = [];\n"
            "var elem = { setAttribute: function(name, v){ given = v;},\n"
            "             focus: function(){}, contentDocument: null };\n"
            "var document = { evaluate: function(xpath){\n"
            "    asked.push(xpath);\n"
            "    return { snapshotItem: function(){ return elem;}};\n"
            "}};\n"));

        const QJSValue result = engine.evaluate
            (View::SetTextValueJsCode(QStringLiteral("//input[@name=\"%2\"]"),
                                      QStringLiteral("hello")));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));

        QCOMPARE(engine.globalObject().property(QStringLiteral("given")).toString(),
                 QStringLiteral("hello"));
        QCOMPARE(engine.globalObject().property(QStringLiteral("asked"))
                 .property(0).toString(),
                 QStringLiteral("//input[@name=\"%2\"]"));
    }

    void noDropHandlerEscapesTheTextItself(){
        static const QStringList sources = QStringList()
            << QStringLiteral("/view/webengine/webengineview.cpp")
            << QStringLiteral("/view/webengine/quickwebengineview.cpp")
            << QStringLiteral("/view/quicknativewebview.cpp");

        foreach(const QString &name, sources){
            QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + name);
            QVERIFY2(file.open(QFile::ReadOnly | QFile::Text),
                     qPrintable(file.fileName()));
            const QString source = QString::fromUtf8(file.readAll());
            file.close();

            QVERIFY2(source.contains(QStringLiteral("::dropEvent(")),
                     qPrintable(name + QStringLiteral(" has no dropEvent")));
            QVERIFY2(!source.contains(QStringLiteral("text.replace(")),
                     qPrintable(name + QStringLiteral(" escapes the dropped "
                                                      "text itself (A-25)")));
        }
    }

    void eventFilterInstallsWhatWasAsked_data(){
        QTest::addColumn<QList<QEvent::Type> >("types");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("none")
            << QList<QEvent::Type>() << QStringList();
        QTest::newRow("key press")
            << (QList<QEvent::Type>() << QEvent::KeyPress)
            << (QStringList() << QStringLiteral("keydown"));
        QTest::newRow("key release")
            << (QList<QEvent::Type>() << QEvent::KeyRelease)
            << (QStringList() << QStringLiteral("keyup"));
        QTest::newRow("mouse move")
            << (QList<QEvent::Type>() << QEvent::MouseMove)
            << (QStringList() << QStringLiteral("mousemove"));
        QTest::newRow("mouse press")
            << (QList<QEvent::Type>() << QEvent::MouseButtonPress)
            << (QStringList() << QStringLiteral("mousedown"));
        QTest::newRow("mouse release")
            << (QList<QEvent::Type>() << QEvent::MouseButtonRelease)
            << (QStringList() << QStringLiteral("mouseup"));
        QTest::newRow("wheel")
            << (QList<QEvent::Type>() << QEvent::Wheel)
            << (QStringList() << QStringLiteral("mousewheel"));
        QTest::newRow("what the views ask for")
            << (QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease)
            << (QStringList() << QStringLiteral("keydown") << QStringLiteral("keyup"));
        QTest::newRow("all")
            << (QList<QEvent::Type>()
                << QEvent::KeyPress << QEvent::KeyRelease << QEvent::MouseMove
                << QEvent::MouseButtonPress << QEvent::MouseButtonRelease
                << QEvent::Wheel)
            << (QStringList() << QStringLiteral("keydown") << QStringLiteral("keyup")
                              << QStringLiteral("mousemove") << QStringLiteral("mousedown")
                              << QStringLiteral("mouseup") << QStringLiteral("mousewheel"));
    }

    void eventFilterInstallsWhatWasAsked(){
        QFETCH(QList<QEvent::Type>, types);
        QFETCH(QStringList, expected);

        QJSEngine engine;
        engine.evaluate(QStringLiteral(
            "var registered = [];\n"
            "var doc = { addEventListener: function(name){ registered.push(name);}};\n"
            "var window = { document: doc };\n"
            "var frames = [];\n"));

        const QJSValue result =
            engine.evaluate(View::InstallEventFilterJsCode(types));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));

        const QJSValue registered =
            engine.globalObject().property(QStringLiteral("registered"));
        QStringList installed;
        const int count = registered.property(QStringLiteral("length")).toInt();
        for(int i = 0; i < count; i++)
            installed << registered.property(i).toString();
        QCOMPARE(installed, expected);
    }
};

QTEST_MAIN(tst_jscode)
#include "tst_jscode.moc"
