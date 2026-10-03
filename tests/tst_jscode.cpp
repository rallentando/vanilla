#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QJSEngine>
#include <QJSValue>
#include <QFile>
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
        ADD(ElementPathJsCode());
        ADD(ResolveElementPathJsCode());
        ADD(CollectElementsJsCode());
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
        ADD(EdgeHideWebViewJsCode());
        ADD(EdgeExtensionTabQueryJsCode());
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
        int expected = 44;
#ifdef MEDIATIME
        expected += 2;
#endif
#ifdef EDGEWEBVIEW
        expected += 3;
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
            "                        isTrusted: true,\n"
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

    void thePopupSizeIsTheDocumentsMinContent_data(){
        QTest::addColumn<double>("minContent");
        QTest::addColumn<double>("content");
        QTest::addColumn<double>("ratio");
        QTest::addColumn<int>("clientWidth");
        QTest::addColumn<bool>("hadStyle");
        QTest::addColumn<QString>("expected");
        QTest::newRow("uBOL") << 273.0 << 374.6 << 1.25 << 450 << false << QStringLiteral("342,469");
        QTest::newRow("cut to 600, the bar added") << 390.0 << 900.0 << 1.0 << 433 << true << QStringLiteral("407,600");
        QTest::newRow("cut to 800") << 1200.0 << 100.0 << 1.0 << 450 << false << QStringLiteral("800,100");
        QTest::newRow("no smaller than 25") << 5.0 << 3.0 << 2.0 << 450 << true << QStringLiteral("50,50");
    }
    void thePopupSizeIsTheDocumentsMinContent(){
        QFETCH(double, minContent);
        QFETCH(double, content);
        QFETCH(double, ratio);
        QFETCH(int, clientWidth);
        QFETCH(bool, hadStyle);
        QFETCH(QString, expected);
        QJSEngine engine;
        const QString harness = QStringLiteral(
            "(function(minContent, content, ratio, clientWidth, hadStyle, withBody){\n"
            "  var props = {}, attr = hadStyle, original = hadStyle ? 'color: red' : '', dirty = false;\n"
            "  var style = { setProperty: function(n, v, p){ props[n] = v; attr = true; dirty = true; },\n"
            "                get cssText(){ return Object.keys(props).length ? JSON.stringify(props) : original; },\n"
            "                set cssText(t){ props = {}; original = t; attr = true; } };\n"
            "  var root = { style: style, clientWidth: clientWidth,\n"
            "               hasAttribute: function(n){ return n === 'style' && attr; },\n"
            "               getAttribute: function(n){ dirty = false; return attr ? style.cssText : null; },\n"
            "               setAttribute: function(n, v){ props = {}; original = v; attr = true; },\n"
            "               // (a style the CSSOM changed comes back as '' unless it was read first.)\n"
            "               removeAttribute: function(n){ if(n === 'style'){ attr = dirty; props = {}; original = ''; } },\n"
            "               getBoundingClientRect: function(){\n"
            "                 if(minContent < 0) throw new Error('a page which throws');\n"
            "                 var w = props.width === 'min-content' ? minContent : props.width ? parseFloat(props.width) : 450;\n"
            "                 return { width: w, height: props.height === 'auto' ? content : 520 }; } };\n"
            "  var document = { documentElement: root, body: withBody ? {} : null };\n"
            "  var window = { innerWidth: 450, devicePixelRatio: ratio };\n"
            "  var answer; try { answer = %1; } catch(e){ answer = 'threw'; }\n"
            "  return { answer: answer, attr: attr, text: style.cssText };\n"
            "})").arg(View::ExtensionPopupSizeJsCode());
        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(), qPrintable(fn.property(QStringLiteral("message")).toString()));
        const QJSValue out = fn.call(QJSValueList() << minContent << content << ratio << clientWidth << hadStyle << true);
        QVERIFY2(!out.isError(), qPrintable(out.property(QStringLiteral("message")).toString()));
        QCOMPARE(out.property(QStringLiteral("answer")).toString(), expected);
        QCOMPARE(out.property(QStringLiteral("attr")).toBool(), hadStyle);
        QCOMPARE(out.property(QStringLiteral("text")).toString(), hadStyle ? QStringLiteral("color: red") : QString());
        const QJSValue early = fn.call(QJSValueList() << minContent << content << ratio << clientWidth << hadStyle << false);
        QVERIFY(early.property(QStringLiteral("answer")).isNull());
        QCOMPARE(early.property(QStringLiteral("attr")).toBool(), hadStyle);
        const QJSValue thrown = fn.call(QJSValueList() << -1.0 << content << ratio << clientWidth << hadStyle << true);
        QCOMPARE(thrown.property(QStringLiteral("answer")).toString(), QStringLiteral("threw"));
        QCOMPARE(thrown.property(QStringLiteral("attr")).toBool(), hadStyle);
        QCOMPARE(thrown.property(QStringLiteral("text")).toString(), hadStyle ? QStringLiteral("color: red") : QString());
    }

    void theExtensionTabQueryAnswersTheTabTheHostNamed_data(){
        QTest::addColumn<QJSValue>("named");
        QTest::addColumn<int>("answered");
        QTest::newRow("the host names a tab") << QJSValue(1) << 1;
        QTest::newRow("the host names none") << QJSValue(QJSValue::NullValue) << 0;
    }
    void theExtensionTabQueryAnswersTheTabTheHostNamed(){
        QFETCH(QJSValue, named);
        QFETCH(int, answered);
        QJSEngine engine;
        const QString harness = QStringLiteral(
            "(function(isTop, named, holds){\n"
            "  var window, setTimeout, clearTimeout;\n"
            "  var out = { queries: [], posted: [], results: [], errors: 0, timers: 0, cleared: 0, delays: [],\n"
            "              wrapped: false, staleIgnored: false, isPromise: false, promised: -1, rejected: '', thrown: '' };\n"
            "  var listener = null;\n"
            "  var lastError;\n"
            "  var failing = false;\n"
            "  var fired = [];\n"
            "  var tabs = [ { id: 1, windowId: 10, active: true, url: 'http://127.0.0.1:1/', title: 'Page' },\n"
            "               { id: 2, windowId: 20, active: true, url: 'chrome-extension://abc/popup.html', title: 'Popup' },\n"
            "               { id: 3, windowId: 30, active: true },\n"
            "               { id: 4, windowId: 40, active: true, url: 'http://127.0.0.1:1/', title: 'B' } ];\n"
            "  var original = function(info, callback){\n"
            "    out.queries.push(info);\n"
            "    if(info && info.status === 'bogus') throw new Error('bogus');\n"
            "    if(failing){ lastError = { message: 'no' }; callback(undefined); lastError = undefined; return; }\n"
            "    var answered = tabs;\n"
            "    if(info && typeof info.title === 'string') answered = answered.filter(function(t){ return t.title === info.title; });\n"
            "    if(info && Array.isArray(info.url)) answered = answered.filter(function(t){ return info.url.indexOf(t.url) >= 0; });\n"
            "    callback(answered);\n"
            "  };\n"
            "  var chrome = { tabs: { query: original },\n"
            "                 windows: { WINDOW_ID_CURRENT: -2 },\n"
            "                 runtime: { get lastError(){ return lastError; },\n"
            "                            getManifest: function(){ return { permissions: holds === 'tabs' ? ['storage', 'tabs'] : ['storage'],\n"
            "                                                              host_permissions: holds === 'hosts' ? ['<all_urls>'] : [] }; } },\n"
            "                 webview: { postMessage: function(m){ out.posted.push(m); },\n"
            "                            addEventListener: function(t, f){ if(t === 'message'){ listener = f; out.listened = true; } } } };\n"
            "  setTimeout = function(f, ms){ fired.push(f); out.delays.push(ms); out.timers++; return out.timers; };\n"
            "  clearTimeout = function(){ out.cleared++; };\n"
            "  window = { chrome: chrome };\n"
            "  window.top = isTop ? window : {};\n"
            "%1\n"
            "  out.wrapped = chrome.tabs.query !== original;\n"
            "  if(!isTop || !out.wrapped) return out;\n"
            "  var last = function(){ return out.posted[out.posted.length - 1]; };\n"
            "  var answer = function(seq, id, doc){\n"
            "    listener({ data: { vanilla: 'extension-tab', doc: doc === undefined ? last().doc : doc, seq: seq, id: id } });\n"
            "  };\n"
            "  var keep = function(list){ out.results.push(list); };\n"
            "  // this window's active tab, with another key which is kept.\n"
            "  chrome.tabs.query({ active: true, currentWindow: true, status: 'complete' }, keep);\n"
            "  answer(last().seq, named);\n"
            "  // a stale answer, and one for another document, meet no waiter; the right one is still answered.\n"
            "  chrome.tabs.query({ windowId: -2 }, keep);\n"
            "  var before = out.results.length;\n"
            "  answer(last().seq + 100, 1);\n"
            "  answer(last().seq, 1, 'another document');\n"
            "  var untouched = (out.results.length === before);\n"
            "  answer(last().seq, 1);\n"
            "  out.staleIgnored = untouched && (out.results.length === before + 1);\n"
            "  // the engine's own: nothing here means this window.\n"
            "  chrome.tabs.query({}, keep);\n"
            "  chrome.tabs.query({ active: false }, keep);\n"
            "  chrome.tabs.query({ windowId: 30 }, keep);\n"
            "  out.postedAfterPassthrough = out.posted.length;\n"
            "  // the engine's error, as the callback would have seen it.\n"
            "  failing = true;\n"
            "  chrome.tabs.query({ active: true }, function(list){\n"
            "    if(chrome.runtime.lastError) out.errors++;\n"
            "    keep(list);\n"
            "  });\n"
            "  failing = false;\n"
            "  out.postedAfterError = out.posted.length;\n"
            "  // a condition the named tab does not meet: nothing, not the other tab at the address.\n"
            "  chrome.tabs.query({ active: true, currentWindow: true, title: 'B' }, keep);\n"
            "  answer(last().seq, 1);\n"
            "  // a request nobody answers.\n"
            "  chrome.tabs.query({ active: true }, keep);\n"
            "  var beforeTimeout = out.results.length;\n"
            "  fired[fired.length - 1]();\n"
            "  out.timedOut = (out.results.length === beforeTimeout + 1) ? JSON.stringify(out.results[beforeTimeout]) : 'not settled';\n"
            "  // and the promise form: an answer, and an engine error.\n"
            "  var promise = chrome.tabs.query({ lastFocusedWindow: true });\n"
            "  out.isPromise = !!promise && typeof promise.then === 'function';\n"
            "  if(out.isPromise) promise.then(function(list){ out.promised = list.length; });\n"
            "  answer(last().seq, 1);\n"
            "  failing = true;\n"
            "  var failed = chrome.tabs.query({ active: true });\n"
            "  failing = false;\n"
            "  if(failed && typeof failed.then === 'function') failed.then(null, function(e){ out.rejected = e.message; });\n"
            "  // a query the engine refuses outright, in both forms: settled, not left pending.\n"
            "  var refused = chrome.tabs.query({ active: true, status: 'bogus' });\n"
            "  answer(last().seq, 1);\n"
            "  if(refused && typeof refused.then === 'function') refused.then(null, function(e){ out.thrown = e.message; });\n"
            "  chrome.tabs.query({ active: true, status: 'bogus' }, keep);\n"
            "  answer(last().seq, 1);\n"
            "  out.refusedCallback = JSON.stringify(out.results[out.results.length - 1]);\n"
            "  // the conditions are the caller's at the call: changing the object afterwards changes nothing.\n"
            "  var reused = { active: true, currentWindow: true, title: 'Page' };\n"
            "  chrome.tabs.query(reused, keep);\n"
            "  reused.title = 'B';\n"
            "  answer(last().seq, 1);\n"
            "  out.snapshot = JSON.stringify(out.results[out.results.length - 1].map(function(t){ return t.id; }));\n"
            "  // and a list among them is copied, not shared (R-100 中1).\n"
            "  var urls = ['http://127.0.0.1:1/'];\n"
            "  chrome.tabs.query({ active: true, currentWindow: true, url: urls }, keep);\n"
            "  urls[0] = 'http://elsewhere/';\n"
            "  answer(last().seq, 1);\n"
            "  out.urlsCopied = JSON.stringify(out.results[out.results.length - 1].map(function(t){ return t.id; }));\n"
            "  out.delaysOk = out.delays.length > 0 && out.delays.every(function(ms){ return ms === 2000; });\n"
            "  out.queryKeys = out.queries.map(function(q){ return Object.keys(q).sort().join(','); }).join('|');\n"
            "  out.sentKeys = Object.keys(out.posted[0]).sort().join(',');\n"
            "  out.sentTabs = JSON.stringify(out.posted[0].tabs);\n"
            "  out.firstIds = JSON.stringify(out.results[0].map(function(t){ return t.id; }));\n"
            "  out.secondIds = JSON.stringify(out.results[1].map(function(t){ return t.id; }));\n"
            "  out.ownLengths = [out.results[2].length, out.results[3].length, out.results[4].length].join(',');\n"
            "  out.errorResult = JSON.stringify(out.results[5]);\n"
            "  out.narrowed = JSON.stringify(out.results[6]);\n"
            "  if(holds === 'hosts'){\n"
            "    // the engine's own answer fails in turn: the callback with 'lastError' set,\n"
            "    // the promise rejected, a query it refuses outright rejected -- settled, once.\n"
            "    out.fallback = { error: false, list: '', rejected: '', thrown: '', calls: 0 };\n"
            "    chrome.tabs.query({ active: true }, function(list){\n"
            "      out.fallback.calls++; out.fallback.error = !!chrome.runtime.lastError; out.fallback.list = JSON.stringify(list);\n"
            "    });\n"
            "    failing = true; answer(last().seq, null); failing = false;\n"
            "    var failed = chrome.tabs.query({ active: true });\n"
            "    failing = true; answer(last().seq, null); failing = false;\n"
            "    failed.then(null, function(e){ out.fallback.rejected = e.message; });\n"
            "    var refused = chrome.tabs.query({ active: true, status: 'bogus' });\n"
            "    answer(last().seq, null);\n"
            "    refused.then(null, function(e){ out.fallback.thrown = e.message; });\n"
            "  }\n"
            "  return out;\n"
            "})").arg(View::EdgeExtensionTabQueryJsCode());

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));

        const QJSValue top = fn.call(QJSValueList() << QJSValue(true) << named << QJSValue(QStringLiteral("tabs")));
        QVERIFY2(!top.isError(),
                 qPrintable(top.property(QStringLiteral("message")).toString()));
        QVERIFY(top.property(QStringLiteral("wrapped")).toBool());

        QCOMPARE(top.property(QStringLiteral("sentKeys")).toString(),
                 QStringLiteral("doc,seq,tabs,vanilla"));
        QCOMPARE(top.property(QStringLiteral("sentTabs")).toString(),
                 QStringLiteral("[{\"id\":1,\"url\":\"http://127.0.0.1:1/\"},"
                                "{\"id\":2,\"url\":\"chrome-extension://abc/popup.html\"},"
                                "{\"id\":4,\"url\":\"http://127.0.0.1:1/\"}]"));
        QCOMPARE(top.property(QStringLiteral("queryKeys")).toString(),
                 (answered ? QStringLiteral("|status") : QString())
                 + QStringLiteral("||||active|windowId|||title||||||status||status||title||url"));
        QCOMPARE(top.property(QStringLiteral("firstIds")).toString(),
                 answered ? QStringLiteral("[1]") : QStringLiteral("[]"));
        QCOMPARE(top.property(QStringLiteral("secondIds")).toString(), QStringLiteral("[1]"));
        QVERIFY(top.property(QStringLiteral("staleIgnored")).toBool());
        QCOMPARE(top.property(QStringLiteral("ownLengths")).toString(), QStringLiteral("4,4,4"));
        QCOMPARE(top.property(QStringLiteral("postedAfterPassthrough")).toInt(), 2);
        QCOMPARE(top.property(QStringLiteral("errors")).toInt(), 1);
        QCOMPARE(top.property(QStringLiteral("errorResult")).toString(), QStringLiteral("[]"));
        QCOMPARE(top.property(QStringLiteral("postedAfterError")).toInt(), 2);
        QCOMPARE(top.property(QStringLiteral("narrowed")).toString(), QStringLiteral("[]"));
        QCOMPARE(top.property(QStringLiteral("timedOut")).toString(), QStringLiteral("[]"));
        QVERIFY(top.property(QStringLiteral("isPromise")).toBool());
        QCoreApplication::processEvents();
        QCOMPARE(top.property(QStringLiteral("promised")).toInt(), 1);
        QCOMPARE(top.property(QStringLiteral("rejected")).toString(), QStringLiteral("no"));
        QCOMPARE(top.property(QStringLiteral("thrown")).toString(), QStringLiteral("bogus"));
        QCOMPARE(top.property(QStringLiteral("refusedCallback")).toString(), QStringLiteral("[]"));
        QCOMPARE(top.property(QStringLiteral("snapshot")).toString(), QStringLiteral("[1]"));
        QCOMPARE(top.property(QStringLiteral("urlsCopied")).toString(), QStringLiteral("[1]"));
        QCOMPARE(top.property(QStringLiteral("posted")).property(QStringLiteral("length")).toInt(), 9);
        QCOMPARE(top.property(QStringLiteral("timers")).toInt(), 9);
        QCOMPARE(top.property(QStringLiteral("cleared")).toInt(), 9);
        QVERIFY(top.property(QStringLiteral("delaysOk")).toBool());

        const QJSValue frame = fn.call(QJSValueList() << QJSValue(false) << named << QJSValue(QStringLiteral("tabs")));
        QVERIFY2(!frame.isError(),
                 qPrintable(frame.property(QStringLiteral("message")).toString()));
        QVERIFY(!frame.property(QStringLiteral("wrapped")).toBool());

        const QJSValue noTabs = fn.call(QJSValueList() << QJSValue(true) << named << QJSValue(QStringLiteral("none")));
        QVERIFY2(!noTabs.isError(),
                 qPrintable(noTabs.property(QStringLiteral("message")).toString()));
        QVERIFY(!noTabs.property(QStringLiteral("wrapped")).toBool());
        QCOMPARE(noTabs.property(QStringLiteral("posted")).property(QStringLiteral("length")).toInt(), 0);
        QVERIFY(!noTabs.property(QStringLiteral("listened")).toBool());

        const QJSValue hosts = fn.call(QJSValueList() << QJSValue(true) << named << QJSValue(QStringLiteral("hosts")));
        QVERIFY2(!hosts.isError(),
                 qPrintable(hosts.property(QStringLiteral("message")).toString()));
        QVERIFY(hosts.property(QStringLiteral("wrapped")).toBool());
        QCOMPARE(hosts.property(QStringLiteral("sentKeys")).toString(),
                 QStringLiteral("doc,seq,tabs,vanilla"));
        QCOMPARE(hosts.property(QStringLiteral("firstIds")).toString(),
                 answered ? QStringLiteral("[1]") : QStringLiteral("[1,2,3,4]"));
        QVERIFY(hosts.property(QStringLiteral("queryKeys")).toString().startsWith(
                    answered ? QStringLiteral("|status|") : QStringLiteral("|active,currentWindow,status|")));
        QCOMPARE(hosts.property(QStringLiteral("secondIds")).toString(), QStringLiteral("[1]"));
        QCOMPARE(hosts.property(QStringLiteral("narrowed")).toString(), QStringLiteral("[]"));
        QCOMPARE(hosts.property(QStringLiteral("timedOut")).toString().count(QStringLiteral("\"id\"")), 4);
        QCOMPARE(hosts.property(QStringLiteral("posted")).property(QStringLiteral("length")).toInt(), 12);
        QCOMPARE(hosts.property(QStringLiteral("cleared")).toInt(), 12);
        const QJSValue fallback = hosts.property(QStringLiteral("fallback"));
        QCOMPARE(fallback.property(QStringLiteral("calls")).toInt(), 1);
        QVERIFY(fallback.property(QStringLiteral("error")).toBool());
        QCOMPARE(fallback.property(QStringLiteral("list")).toString(), QStringLiteral("[]"));
        QCoreApplication::processEvents();
        QCOMPARE(fallback.property(QStringLiteral("rejected")).toString(), QStringLiteral("no"));
        QCOMPARE(fallback.property(QStringLiteral("thrown")).toString(), QStringLiteral("bogus"));
    }

    void theInputBridgeSeesTheCaretThroughAShadowRoot_data(){
        QTest::addColumn<QString>("focus");
        QTest::addColumn<int>("reported");

        QTest::newRow("body")
            << QStringLiteral("({ active: { tagName: 'BODY' } })") << 1;
        QTest::newRow("input")
            << QStringLiteral("({ active: { tagName: 'INPUT' } })") << 0;
        QTest::newRow("input in a shadow root")
            << QStringLiteral("({ active: { tagName: 'GR-APP', shadowRoot: {\n"
                              "    activeElement: { tagName: 'INPUT' } } } })") << 0;
        QTest::newRow("input two shadow roots deep")
            << QStringLiteral("({ active: { tagName: 'GR-APP', shadowRoot: {\n"
                              "    activeElement: { tagName: 'GR-SEARCH-BAR', shadowRoot: {\n"
                              "        activeElement: { tagName: 'INPUT' } } } } } })") << 0;
        QTest::newRow("div in a shadow root")
            << QStringLiteral("({ active: { tagName: 'GR-APP', shadowRoot: {\n"
                              "    activeElement: { tagName: 'DIV' } } } })") << 1;
        QTest::newRow("target named by the composed path")
            << QStringLiteral("({ active: { tagName: 'GR-APP' },\n"
                              "   path: [ { tagName: 'INPUT' } ] })") << 0;
        QTest::newRow("input behind a closed root")
            << QStringLiteral("({ active: { tagName: 'GR-APP' },\n"
                              "   path: [ { tagName: 'GR-APP' } ] })") << 1;
        QTest::newRow("path without an element")
            << QStringLiteral("({ active: { tagName: 'BODY' }, path: [ {} ] })") << 1;
    }

    void theInputBridgeSeesTheCaretThroughAShadowRoot(){
        QFETCH(QString, focus);
        QFETCH(int, reported);

        QJSEngine engine;
        const QString script =
            View::EdgeInputBridgeJsCode(QList<int>() << 81, QList<int>());

        const QString harness = QStringLiteral(
            "(function(focus){\n"
            "  var window, document;\n"
            "  var out = { reported: 0, prevented: 0 };\n"
            "  var handler = null;\n"
            "  document = { activeElement: focus.active,\n"
            "               addEventListener: function(t, f){\n"
            "                   if(t === 'keydown') handler = f; } };\n"
            "  window = { chrome: { webview: {\n"
            "                 postMessage: function(m){ out.reported++; } } } };\n"
            "  window.top = window;\n"
            "%1\n"
            "  var e = { keyCode: 81, shiftKey: false, repeat: false,\n"
            "            isTrusted: true,\n"
            "            isComposing: false, ctrlKey: false,\n"
            "            altKey: false, metaKey: false,\n"
            "            preventDefault: function(){ out.prevented++; } };\n"
            "  if(focus.path) e.composedPath = function(){ return focus.path; };\n"
            "  handler(e);\n"
            "  return out;\n"
            "})").arg(script);

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));
        const QJSValue arg = engine.evaluate(focus);
        QVERIFY2(!arg.isError(),
                 qPrintable(arg.property(QStringLiteral("message")).toString()));
        const QJSValue out = fn.call(QJSValueList() << arg);
        QVERIFY2(!out.isError(),
                 qPrintable(out.property(QStringLiteral("message")).toString()));
        QCOMPARE(out.property(QStringLiteral("reported")).toInt(), reported);
        QCOMPARE(out.property(QStringLiteral("prevented")).toInt(), reported);
    }

    void theBridgesOutliveTheHiddenWebview(){
        QJSEngine engine;
        const QString script =
            View::EdgeInputBridgeJsCode(QList<int>() << 81, QList<int>())
            + View::EdgeScrollReportJsCode()
            + View::EdgeHideWebViewJsCode();

        const QString harness = QStringLiteral(
            "(function(){\n"
            "  var window, document, location;\n"
            "  var out = { keys: 0, scrolls: 0 };\n"
            "  var keydown = null, scroll = null;\n"
            "  var de = { scrollWidth: 800, scrollHeight: 2000 };\n"
            "  document = { activeElement: null, documentElement: de, body: null,\n"
            "               addEventListener: function(t, f){\n"
            "                   if(t === 'keydown') keydown = f; } };\n"
            "  location = { href: 'https://a/' };\n"
            "  window = { chrome: { webview: {\n"
            "                 postMessage: function(m){\n"
            "                     if(m.kind === 'key') out.keys++;\n"
            "                     if(m.kind === 'scroll') out.scrolls++; } } },\n"
            "             pageXOffset: 0, pageYOffset: 0,\n"
            "             innerWidth: 800, innerHeight: 600,\n"
            "             addEventListener: function(t, f){\n"
            "                 if(t === 'scroll') scroll = f; } };\n"
            "  window.top = window;\n"
            "  var requestAnimationFrame = function(f){ f(); };\n"
            "  var setInterval = function(){};\n"
            "%1\n"
            "  out.hidden = (window.chrome.webview === undefined);\n"
            "  keydown({ keyCode: 81, shiftKey: false, repeat: false,\n"
            "            isTrusted: true,\n"
            "            isComposing: false, ctrlKey: false,\n"
            "            altKey: false, metaKey: false,\n"
            "            preventDefault: function(){} });\n"
            "  scroll();\n"
            "  return out;\n"
            "})").arg(script);

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));
        const QJSValue out = fn.call();
        QVERIFY2(!out.isError(),
                 qPrintable(out.property(QStringLiteral("message")).toString()));
        QVERIFY(out.property(QStringLiteral("hidden")).toBool());
        QCOMPARE(out.property(QStringLiteral("keys")).toInt(), 1);
        QCOMPARE(out.property(QStringLiteral("scrolls")).toInt(), 1);
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
            "}};\n"
            "elem.contentDocument = document;\n"));

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

    static QString ShadowTree(){
        return QStringLiteral(
            "var asked = [];\n"
            "var descendants = function(n){\n"
            "    var out = [];\n"
            "    var visit = function(m){\n"
            "        for(var i = 0; i < m.childNodes.length; i++){\n"
            "            out.push(m.childNodes[i]);\n"
            "            visit(m.childNodes[i]);\n"
            "        }\n"
            "    };\n"
            "    visit(n);\n"
            "    return out;\n"
            "};\n"
            "var selectAll = function(n){\n"
            "    return function(sel){\n"
            "        return descendants(n).filter(function(d){\n"
            "            return sel == '*' || d.tagName.toLowerCase() == sel; });\n"
            "    };\n"
            "};\n"
            "var node = function(tag, kids){\n"
            "    var n = { nodeType: 1, tagName: tag, childNodes: kids || [],\n"
            "              parentNode: null, shadowRoot: null, contentDocument: null };\n"
            "    for(var i = 0; i < n.childNodes.length; i++) n.childNodes[i].parentNode = n;\n"
            "    n.querySelectorAll = selectAll(n);\n"
            "    return n;\n"
            "};\n"
            "var shadow = function(host, kids){\n"
            "    var root = { nodeType: 11, host: host, childNodes: kids, parentNode: null };\n"
            "    for(var i = 0; i < kids.length; i++) kids[i].parentNode = root;\n"
            "    root.querySelectorAll = selectAll(root);\n"
            "    host.shadowRoot = root;\n"
            "    return root;\n"
            "};\n"
            "var input = node('INPUT');\n"
            "var innerA = node('A');\n"
            "var svgA = node('a');\n"
            "var bar = node('GR-SEARCH-BAR');\n"
            "shadow(bar, [node('DIV'), node('DIV', [input, innerA, svgA])]);\n"
            "var shadowA = node('A');\n"
            "var inputInFrame = node('INPUT');\n"
            "var innerHtml = node('HTML', [node('BODY', [inputInFrame])]);\n"
            "var innerDoc = { nodeType: 9, childNodes: [innerHtml],\n"
            "                 evaluate: function(xpath){\n"
            "                     asked.push('inner:' + xpath);\n"
            "                     var found = xpath == '//html/body/input' ? inputInFrame : null;\n"
            "                     return { snapshotItem: function(){ return found; } };\n"
            "                 } };\n"
            "innerHtml.parentNode = innerDoc;\n"
            "innerDoc.querySelectorAll = selectAll(innerDoc);\n"
            "var innerFrame = node('IFRAME');\n"
            "innerFrame.contentDocument = innerDoc;\n"
            "innerDoc.defaultView = { frameElement: innerFrame };\n"
            "var app = node('GR-APP');\n"
            "shadow(app, [node('DIV', [shadowA]), node('DIV', [bar]),\n"
            "             node('DIV', [innerFrame, node('iframe')])]);\n"
            "var lowerApp = node('gr-app');\n"
            "var plain = node('A');\n"
            "var plainSvgA = node('a');\n"
            "var frameInput = node('INPUT');\n"
            "var frameApp = node('GR-APP');\n"
            "shadow(frameApp, [node('DIV', [frameInput])]);\n"
            "var frameHtml = node('HTML', [node('BODY', [frameApp])]);\n"
            "var frameDoc = { nodeType: 9, childNodes: [frameHtml],\n"
            "                 evaluate: function(xpath){\n"
            "                     asked.push('frame:' + xpath);\n"
            "                     var found = xpath == '//html/body/gr-app' ? frameApp : null;\n"
            "                     return { snapshotItem: function(){ return found; } };\n"
            "                 } };\n"
            "frameHtml.parentNode = frameDoc;\n"
            "frameDoc.querySelectorAll = selectAll(frameDoc);\n"
            "var frame = node('IFRAME');\n"
            "frame.contentDocument = frameDoc;\n"
            "frameDoc.defaultView = { frameElement: frame };\n"
            "var blind = node('IFRAME');\n"
            "var body = node('BODY', [plain, plainSvgA, app, lowerApp, frame, blind]);\n"
            "var html = node('HTML', [body]);\n"
            "var document = { nodeType: 9, childNodes: [html],\n"
            "                 evaluate: function(xpath){\n"
            "                     asked.push(xpath);\n"
            "                     var found = xpath == '//html/body/gr-app' ? app :\n"
            "                                 xpath == '//html/body/a' ? plain :\n"
            "                                 xpath == '//html/body/iframe[1]' ? frame :\n"
            "                                 xpath == '//html/body/iframe[2]' ? blind : null;\n"
            "                     return { snapshotItem: function(){ return found; } };\n"
            "                 } };\n"
            "html.parentNode = document;\n"
            "document.querySelectorAll = selectAll(document);\n");
    }

    void anElementPathCrossesShadowRoots(){
        QJSEngine engine;
        const QJSValue setup = engine.evaluate(ShadowTree() + View::ElementPathJsCode());
        QVERIFY2(!setup.isError(), qPrintable(setup.toString()));

        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(plain)")).toString(),
                 QStringLiteral("//html/body/a"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(shadowA)")).toString(),
                 QStringLiteral("//html/body/gr-app|div[1]/a"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(inputInFrame)")).toString(),
                 QStringLiteral("//html/body/gr-app|div[3]/iframe[1],//html/body/input"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(input)")).toString(),
                 QStringLiteral("//html/body/gr-app|div[2]/gr-search-bar|div[2]/input"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(innerA)")).toString(),
                 QStringLiteral("//html/body/gr-app|div[2]/gr-search-bar|div[2]/a[1]"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(svgA)")).toString(),
                 QStringLiteral("//html/body/gr-app|div[2]/gr-search-bar|div[2]/a[2]"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(plainSvgA)")).toString(),
                 QStringLiteral("//html/body/a"));
        QCOMPARE(engine.evaluate(QStringLiteral("elementPath(frameInput)")).toString(),
                 QStringLiteral("//html/body/iframe[1],//html/body/gr-app|div/input"));
    }

    void aPathIntoAShadowRootResolvesToItsElement(){
        QJSEngine engine;
        const QJSValue setup = engine.evaluate
            (ShadowTree() + View::ElementPathJsCode() + View::ResolveElementPathJsCode());
        QVERIFY2(!setup.isError(), qPrintable(setup.toString()));

        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath('//html/body/a').elem === plain")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath('//html/body/gr-app|div[2]/gr-search-bar|div[2]/input')"
            ".elem === input")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath(elementPath(input)).elem === input")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath(elementPath(shadowA)).elem === shadowA")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath('//html/body/gr-app|div[3]/input').elem === null")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("asked.join(' ')")).toString(),
                 QStringLiteral("//html/body/a //html/body/gr-app //html/body/gr-app "
                                "//html/body/gr-app //html/body/gr-app"));

        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath(elementPath(innerA)).elem === innerA")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral(
            "resolveElementPath(elementPath(svgA)).elem === svgA")).toBool());

        QVERIFY(engine.evaluate(QStringLiteral(
            "asked = [];\n"
            "var r = resolveElementPath(elementPath(frameInput));\n"
            "r.elem === frameInput && r.doc === frameDoc")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("asked.join(' ')")).toString(),
                 QStringLiteral("//html/body/iframe[1] frame://html/body/gr-app"));
        QVERIFY(engine.evaluate(QStringLiteral(
            "asked = [];\n"
            "var r2 = resolveElementPath(elementPath(inputInFrame));\n"
            "r2.elem === inputInFrame && r2.doc === innerDoc")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("asked.join(' ')")).toString(),
                 QStringLiteral("//html/body/gr-app inner://html/body/input"));
        QVERIFY(engine.evaluate(QStringLiteral(
            "asked = [];\n"
            "resolveElementPath('//html/body/iframe[2],//html/body/a').elem === null")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("asked.join(' ')")).toString(),
                 QStringLiteral("//html/body/iframe[2]"));
    }

    void aFiredClickLeavesTheShadowTree(){
        QJSEngine engine;
        const QJSValue setup = engine.evaluate(QStringLiteral(
            "var init = null, dispatched = null;\n"
            "var MouseEvent = function(type, i){ init = i; this.type = type; };\n"
            "var elem = { contentDocument: null,\n"
            "             dispatchEvent: function(e){ dispatched = e; } };\n"
            "var document = { defaultView: {}, evaluate: function(){\n"
            "    return { snapshotItem: function(){ return elem; } }; } };\n"));
        QVERIFY2(!setup.isError(), qPrintable(setup.toString()));

        const QJSValue result = engine.evaluate
            (View::FireClickEventJsCode(QStringLiteral("//a[1]"), QPoint(3, 4)));
        QVERIFY2(!result.isError(), qPrintable(result.toString()));

        QVERIFY(engine.evaluate(QStringLiteral("dispatched !== null && dispatched.type == 'click'")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral("init.composed === true")).toBool());
        QVERIFY(engine.evaluate(QStringLiteral("init.bubbles === true")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("init.clientX")).toInt(), 3);
        QCOMPARE(engine.evaluate(QStringLiteral("init.clientY")).toInt(), 4);
    }

    void theCollectorWalksShadowRoots(){
        QJSEngine engine;
        const QJSValue setup = engine.evaluate(ShadowTree() + View::CollectElementsJsCode());
        QVERIFY2(!setup.isError(), qPrintable(setup.toString()));

        QCOMPARE(engine.evaluate(QStringLiteral(
            "document.querySelectorAll('a').length")).toInt(), 2);
        QVERIFY(engine.evaluate(QStringLiteral(
            "var concats = 0;\n"
            "var concat = Array.prototype.concat;\n"
            "Array.prototype.concat = function(){ concats++; return concat.apply(this, arguments); };\n"
            "var got = collectElements(document, 'a');\n"
            "Array.prototype.concat = concat;\n"
            "got.length == 5 && got[0] === plain && got[1] === plainSvgA &&\n"
            "got[2] === shadowA && got[3] === innerA && got[4] === svgA")).toBool());
        QCOMPARE(engine.evaluate(QStringLiteral("concats")).toInt(), 0);
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

    void settingsCategoryIsTheAddressFragment(){
        QFile file(QStringLiteral(VANILLA_RESOURCE_DIR "/settings/settings.js"));
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text),
                 qPrintable(file.fileName()));
        const QString source = QString::fromUtf8(file.readAll());

        const int click = source.indexOf(QStringLiteral("button.addEventListener('click'"));
        const int same = source.indexOf
            (QStringLiteral("if (window.location.hash.slice(1) === category.name) showCategory(category.name);"), click);
        const int write = source.indexOf
            (QStringLiteral("else window.location.hash = category.name;"), same);
        const int clickEnd = source.indexOf(QStringLiteral("});"), click);
        QVERIFY(click >= 0);
        QVERIFY(same > click);
        QVERIFY(write > same);
        QVERIFY(write < clickEnd);

        const int listen = source.indexOf(QStringLiteral("window.addEventListener('hashchange'"));
        const int resolve = source.indexOf(QStringLiteral("const name = categoryOfFragment();"), listen);
        const int show = source.indexOf(QStringLiteral("showCategory(name);"), resolve);
        QVERIFY(listen >= 0);
        QVERIFY(resolve > listen);
        QVERIFY(show > resolve);
        QVERIFY(source.contains(QStringLiteral("state.current = categoryOfFragment();")));

        const int begin = source.indexOf(QStringLiteral("function categoryOfFragment() {"));
        const int end = source.indexOf(QStringLiteral("\n}"), begin);
        QVERIFY(begin >= 0);
        QVERIFY(end > begin);
        const QString resolver = source.mid(begin, end + 2 - begin);
        QJSEngine engine;
        const QJSValue harness = engine.evaluate(
            QStringLiteral("(function(){ var state = { categories: [{name:'general'},{name:'network'}] };"
                           " var window = { location: { hash: '' } };\n")
            + resolver +
            QStringLiteral("\n return function(hash){ window.location.hash = hash; return categoryOfFragment(); }; })()"));
        QVERIFY2(!harness.isError(), qPrintable(harness.toString()));
        QCOMPARE(harness.call({QJSValue(QStringLiteral("#network"))}).toString(), QStringLiteral("network"));
        QCOMPARE(harness.call({QJSValue(QStringLiteral("#general"))}).toString(), QStringLiteral("general"));
        QCOMPARE(harness.call({QJSValue(QStringLiteral(""))}).toString(), QStringLiteral("general"));
        QCOMPARE(harness.call({QJSValue(QStringLiteral("#nowhere"))}).toString(), QStringLiteral("general"));
    }

    void settingsPickerReleasesItsBusyButton(){
        QFile file(QStringLiteral(VANILLA_RESOURCE_DIR "/settings/settings.js"));
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text),
                 qPrintable(file.fileName()));
        const QString source = QString::fromUtf8(file.readAll());

        const int busy = source.indexOf
            (QStringLiteral("browse.dataset.busy = 'true';"));
        const int finallyBlock = source.indexOf
            (QStringLiteral("} finally {"), busy);
        const int finallyEnd = source.indexOf
            (QStringLiteral("\n        }\n    });"), finallyBlock);
        const int release = source.indexOf
            (QStringLiteral("delete browse.dataset.busy;"), finallyBlock);
        const int enable = source.indexOf
            (QStringLiteral("browse.disabled = false;"), release);
        const int reapply = source.indexOf
            (QStringLiteral("refreshApplicability();"), enable);

        QVERIFY(busy >= 0);
        QVERIFY(finallyBlock > busy);
        QVERIFY(finallyEnd > finallyBlock);
        QVERIFY(release > finallyBlock);
        QVERIFY(enable > release);
        QVERIFY(reapply > enable);
        QVERIFY(reapply < finallyEnd);
        QVERIFY(source.contains(QStringLiteral(
            "node.disabled = !applies || node.dataset.busy === 'true';")));
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

    void theEventFilterSeesTheCaretThroughAShadowRoot_data(){
        QTest::addColumn<QString>("focus");
        QTest::addColumn<int>("reported");

        QTest::newRow("body")
            << QStringLiteral("({ active: { tagName: 'BODY' } })") << 1;
        QTest::newRow("input")
            << QStringLiteral("({ active: { tagName: 'INPUT' } })") << 0;
        QTest::newRow("input in a shadow root")
            << QStringLiteral("({ active: { tagName: 'GR-APP', shadowRoot: {\n"
                              "    activeElement: { tagName: 'INPUT' } } } })") << 0;
        QTest::newRow("div in a shadow root")
            << QStringLiteral("({ active: { tagName: 'GR-APP', shadowRoot: {\n"
                              "    activeElement: { tagName: 'DIV' } } } })") << 1;
        QTest::newRow("target named by the composed path")
            << QStringLiteral("({ active: { tagName: 'GR-APP' },\n"
                              "   path: [ { tagName: 'INPUT' } ] })") << 0;
        QTest::newRow("input behind a closed root")
            << QStringLiteral("({ active: { tagName: 'GR-APP' },\n"
                              "   path: [ { tagName: 'GR-APP' } ] })") << 1;
    }

    void theEventFilterSeesTheCaretThroughAShadowRoot(){
        QFETCH(QString, focus);
        QFETCH(int, reported);

        QJSEngine engine;
        const QString script = View::InstallEventFilterJsCode
            (QList<QEvent::Type>() << QEvent::KeyPress << QEvent::KeyRelease);

        const QString harness = QStringLiteral(
            "(function(focus){\n"
            "  var out = { reported: 0, prevented: 0 };\n"
            "  var handlers = [];\n"
            "  var doc = { activeElement: focus.active,\n"
            "              addEventListener: function(t, f){ handlers.push(f); } };\n"
            "  var window = { document: doc };\n"
            "  var frames = [];\n"
            "  var console = { info: function(){ out.reported++; } };\n"
            "%1\n"
            "  for(var i = 0; i < handlers.length; i++){\n"
            "    var e = { keyCode: 81, shiftKey: false, ctrlKey: false,\n"
            "              isTrusted: true,\n"
            "              altKey: false, metaKey: false,\n"
            "              target: { ownerDocument: doc },\n"
            "              preventDefault: function(){ out.prevented++; } };\n"
            "    if(focus.path) e.composedPath = function(){ return focus.path; };\n"
            "    handlers[i](e);\n"
            "  }\n"
            "  out.handlers = handlers.length;\n"
            "  return out;\n"
            "})").arg(script);

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));
        const QJSValue arg = engine.evaluate(focus);
        QVERIFY2(!arg.isError(),
                 qPrintable(arg.property(QStringLiteral("message")).toString()));
        const QJSValue out = fn.call(QJSValueList() << arg);
        QVERIFY2(!out.isError(),
                 qPrintable(out.property(QStringLiteral("message")).toString()));
        QCOMPARE(out.property(QStringLiteral("handlers")).toInt(), 2);
        QCOMPARE(out.property(QStringLiteral("reported")).toInt(), reported * 2);
        QCOMPARE(out.property(QStringLiteral("prevented")).toInt(), reported * 2);
    }

    void noBridgeReportsAnEventThePageMadeUp_data(){
        QTest::addColumn<QString>("script");
        QTest::addColumn<int>("listeners");

        QTest::newRow("the engine views' filter")
            << View::InstallEventFilterJsCode
                (QList<QEvent::Type>()
                 << QEvent::KeyPress << QEvent::KeyRelease << QEvent::MouseMove
                 << QEvent::MouseButtonPress << QEvent::MouseButtonRelease
                 << QEvent::Wheel)
            << 6;
        QTest::newRow("the Edge bridge")
            << View::EdgeInputBridgeJsCode(QList<int>() << 81, QList<int>())
            << 1;
    }

    void noBridgeReportsAnEventThePageMadeUp(){
        QFETCH(QString, script);
        QFETCH(int, listeners);

        QJSEngine engine;
        const QString harness = QStringLiteral(
            "(function(trusted){\n"
            "  var out = { reported: 0, prevented: 0 };\n"
            "  var handlers = [];\n"
            "  var document = { activeElement: { tagName: 'BODY' },\n"
            "                   addEventListener: function(t, f){ handlers.push(f); } };\n"
            "  var doc = document;\n"
            "  var window = { document: doc, chrome: { webview: {\n"
            "                 postMessage: function(){ out.reported++; } } } };\n"
            "  window.top = window;\n"
            "  var frames = [];\n"
            "  var console = { info: function(){ out.reported++; } };\n"
            "%1\n"
            "  for(var i = 0; i < handlers.length; i++){\n"
            "    var e = { keyCode: 81, shiftKey: false, ctrlKey: false,\n"
            "              altKey: false, metaKey: false, repeat: false,\n"
            "              isComposing: false, button: 0, clientX: 1, clientY: 1,\n"
            "              wheelDelta: 120,\n"
            "              target: { ownerDocument: doc },\n"
            "              preventDefault: function(){ out.prevented++; } };\n"
            "    if(trusted !== undefined) e.isTrusted = trusted;\n"
            "    handlers[i](e);\n"
            "  }\n"
            "  out.handlers = handlers.length;\n"
            "  return out;\n"
            "})").arg(script);

        const QJSValue fn = engine.evaluate(harness);
        QVERIFY2(!fn.isError(),
                 qPrintable(fn.property(QStringLiteral("message")).toString()));

        const QJSValue users = fn.call(QJSValueList() << QJSValue(true));
        QVERIFY2(!users.isError(),
                 qPrintable(users.property(QStringLiteral("message")).toString()));
        QCOMPARE(users.property(QStringLiteral("handlers")).toInt(), listeners);
        QCOMPARE(users.property(QStringLiteral("reported")).toInt(), listeners);

        foreach(const QJSValue &made, QJSValueList() << QJSValue(false) << QJSValue()){
            const QJSValue pages = fn.call(QJSValueList() << made);
            QVERIFY2(!pages.isError(),
                     qPrintable(pages.property(QStringLiteral("message")).toString()));
            QCOMPARE(pages.property(QStringLiteral("handlers")).toInt(), listeners);
            QCOMPARE(pages.property(QStringLiteral("reported")).toInt(), 0);
            QCOMPARE(pages.property(QStringLiteral("prevented")).toInt(), 0);
        }
    }

    void thePageCannotReadTheMark(){
        static const QStringList installers = QStringList()
            << QStringLiteral("/app/networkcontroller.cpp")
            << QStringLiteral("/view/webengine/quickwebengineview6.qml");

        foreach(const QString &name, installers){
            QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + name);
            QVERIFY2(file.open(QFile::ReadOnly | QFile::Text),
                     qPrintable(file.fileName()));
            const QString source = QString::fromUtf8(file.readAll());
            file.close();

            const bool qml = name.endsWith(QStringLiteral(".qml"));
            const int from = source.indexOf(qml ? QStringLiteral("function makeDefaultScript")
                                                : QStringLiteral("InstallEventFilterJsCode"));
            const int to = source.indexOf(qml ? QStringLiteral("return script")
                                              : QStringLiteral("scripts()->insert"), from);
            QVERIFY2(from >= 0 && to > from, qPrintable(name));
            const QString part = source.mid(from, to - from);
            QVERIFY2(part.contains(QStringLiteral("ApplicationWorld")), qPrintable(name));
            QVERIFY2(!part.contains(QStringLiteral("MainWorld")), qPrintable(name));
        }

        const QString edge =
            View::EdgeInputBridgeJsCode(QList<int>() << 81, QList<int>() << 81);
        QVERIFY(!edge.contains(QRegularExpression(QStringLiteral("tag:\\s*[0-9]"))));
        QVERIFY(edge.contains(QStringLiteral("tag: tag")));

        const QString token = Application::EventToken();
        QCOMPARE(token.length(), 32);
        QCOMPARE(token, Application::EventToken());
        QVERIFY(QRegularExpression(QStringLiteral("\\A[0-9a-f]{32}\\z")).match(token).hasMatch());
        QVERIFY(token.mid(0, 8) != token.mid(8, 8) || token.mid(8, 8) != token.mid(16, 8));
        QVERIFY(View::InstallEventFilterJsCode(QList<QEvent::Type>() << QEvent::KeyPress)
                .contains(QStringLiteral("keyPressEvent") + token + QLatin1Char(',')));
    }

    void theApplicationsPagesDrawNothingInAFrame_data(){
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("firstDeclaration");
        QTest::newRow("settings") << QStringLiteral("/settings/settings.js") << QStringLiteral("const state");
        QTest::newRow("directory") << QStringLiteral("/directory/directory.js") << QStringLiteral("\nconst ");
    }

    void theApplicationsPagesDrawNothingInAFrame(){
        QFETCH(QString, name);
        QFETCH(QString, firstDeclaration);

        QFile file(QStringLiteral(VANILLA_RESOURCE_DIR) + name);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), qPrintable(file.fileName()));
        const QString source = QString::fromUtf8(file.readAll());
        file.close();

        const int end = source.indexOf(firstDeclaration);
        QVERIFY(end > 0);
        const QString head = source.left(end);
        QVERIFY(!head.contains(QStringLiteral("fetch(")));

        QJSEngine engine;
        const QJSValue fn = engine.evaluate(QStringLiteral(
            "(function(framed){\n"
            "  var document = { documentElement: { textContent: 'the page' } };\n"
            "  var window = { self: null, top: null };\n"
            "  window.self = window; window.top = framed ? {} : window;\n"
            "  var threw = false;\n"
            "  try { (function(){\n%1\n})(); } catch(e){ threw = true; }\n"
            "  return { threw: threw, left: document.documentElement.textContent };\n"
            "})").arg(head));
        QVERIFY2(!fn.isError(), qPrintable(fn.property(QStringLiteral("message")).toString()));

        QJSValue fnCopy = fn;
        const QJSValue top = fnCopy.call(QJSValueList() << QJSValue(false));
        QVERIFY2(!top.isError(), qPrintable(top.toString()));
        QVERIFY(!top.property(QStringLiteral("threw")).toBool());
        QCOMPARE(top.property(QStringLiteral("left")).toString(), QStringLiteral("the page"));

        const QJSValue framed = fnCopy.call(QJSValueList() << QJSValue(true));
        QVERIFY2(!framed.isError(), qPrintable(framed.toString()));
        QVERIFY(framed.property(QStringLiteral("threw")).toBool());
        QCOMPARE(framed.property(QStringLiteral("left")).toString(), QString());
    }
};

QTEST_MAIN(tst_jscode)
#include "tst_jscode.moc"
