#include "switch.hpp"

#include <QtTest>

#include "passwordpolicy.hpp"
#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)
#  include "webenginepasswords.hpp"
#  include <QJSEngine>
#  include <QJsonArray>
#  include <QJsonDocument>
#  include <QJsonObject>
#endif

#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)
namespace {
    QString Harness(const QString &top){
        return QStringLiteral(R"JS(
(function(){
  var calls = [];
  var docListeners = {}, winListeners = {};
  var timers = [];
  function setTimeout(f, ms){ timers.push(f); return timers.length; }
  function clearTimeout(id){ if(id) timers[id - 1] = null; }
  var observers = [];
  function MutationObserver(cb){ this.cb = cb; this.on = false; observers.push(this); }
  MutationObserver.prototype.observe = function(){ this.on = true; };
  MutationObserver.prototype.disconnect = function(){ this.on = false; };
  function mutate(){ observers.forEach(function(o){ if(o.on) o.cb([]); }); }
  var crypto = { getRandomValues: function(a){
      for(var i = 0; i < a.length; i++) a[i] = Math.floor(Math.random() * 4294967295); return a; } };
  var Node = { DOCUMENT_POSITION_FOLLOWING: 4 };
  // the form the fields are in, which is the document's fields as they are.
  var theForm = { querySelectorAll: function(s){ return document.querySelectorAll(s); },
                  querySelector: function(s){ return document.querySelector(s); } };
  function El(tag, type, value, order){
      this.tagName = tag; this.type = type; this.value = value || ''; this.order = order || 0;
      this.isConnected = true; this.boxes = 1; this.ls = {}; this.form = theForm; }
  El.prototype.getClientRects = function(){ return { length: this.isConnected ? this.boxes : 0 }; };
  El.prototype.addEventListener = function(t, f){ (this.ls[t] = this.ls[t] || []).push(f); };
  El.prototype.removeEventListener = function(t, f){
      var l = this.ls[t] || []; var i = l.indexOf(f); if(i >= 0) l.splice(i, 1); };
  El.prototype.closest = function(){ return this.button ? this : null; };
  El.prototype.getBoundingClientRect = function(){ return { left: 12, bottom: 34 }; };
  El.prototype.dispatchEvent = function(){};
  El.prototype.getAttribute = function(name){ return (this.attrs && this.attrs[name]) || null; };
  function Event(type){ this.type = type; }
  var HTMLInputElement = { prototype: {} };
  Object.defineProperty(HTMLInputElement.prototype, 'value', { set: function(v){ this.filled = v; } });
  El.prototype.compareDocumentPosition = function(o){ return this.order < o.order ? 4 : 2; };
  var user = new El('INPUT', 'text', 'alice', 1);
  var pass = new El('INPUT', 'password', 'pw', 2);
  var go = new El('BUTTON', 'button'); go.button = true;
  var submit = new El('BUTTON', 'submit'); submit.button = true;
  var inputs = [user, pass];
  var document = {
      addEventListener: function(t, f){ (docListeners[t] = docListeners[t] || []).push(f); },
      querySelectorAll: function(sel){
          if(sel === 'input[type="password"]') return inputs.filter(function(i){ return i.type === 'password'; });
          if(sel === 'input') return inputs;
          return []; },
      querySelector: function(sel){
          if(sel === 'input[type="password"]') return inputs.filter(function(i){ return i.type === 'password'; })[0] || null;
          return null; } };
  var knocks = [];
  var topWindow = { postMessage: function(m, o){ knocks.push(m); } };
  var window = { addEventListener: function(t, f){ (winListeners[t] = winListeners[t] || []).push(f); } };
  window.top = %2;
  var location = { origin: 'https://a.example', href: 'https://a.example/login' };
  var self = { origin: 'https://a.example' };
  var lastTicket = null, lastDoc = null, lastField = null;
  function lastDocument(){ return lastDoc; }
  var clock = 1000000;
  var Date = { now: function(){ return clock; } };
  var bridge = {
      submitted: function(d, t){ calls.push('submitted'); lastDoc = d; lastTicket = t; },
      held: function(d, t){ calls.push('held'); lastDoc = d; lastTicket = t; },
      confirmed: function(){ calls.push('confirmed'); },
      released: function(){ calls.push('released'); },
      activated: function(d, f, left, bottom, heard){ calls.push('activated ' + left + ' ' + bottom + ' ' + heard); lastDoc = d; lastField = f; },
      closed: function(d, f){ calls.push('closed'); },
      filtered: function(d, f, numbers){
          calls.push('filtered ' + Array.prototype.join.call(numbers, '.') + (arguments.length > 3 ? ' and more' : '')); },
      rang: function(loaded){ calls.push(loaded ? 'rang-loaded' : 'rang'); } };
  var qt = { webChannelTransport: {} };
  function QWebChannel(t, cb){ cb({ objects: { vanillaPasswords: bridge } }); }
%1
  function fire(list, e){ (list || []).slice().forEach(function(f){ f(e); }); }
  return {
      calls: calls, timers: timers, pass: pass, inputs: inputs, El: El,
      mutate: mutate, knocks: knocks, win: window,
      drain: function(){ return JSON.stringify(window.__vanillaPasswords.drain()); },
      message: function(data, source){ fire(winListeners.message, { data: data, source: source }); },
      press: function(){ fire(docListeners.mousedown, { isTrusted: true, target: pass, button: 0 }); },
      down: function(){ fire(docListeners.keydown, { isTrusted: true, target: pass, key: 'ArrowDown' }); },
      blur: function(trusted, el){ fire(docListeners.blur, { isTrusted: trusted, target: el || pass }); },
      scroll: function(trusted){ fire(docListeners.scroll, { isTrusted: trusted, target: document }); },
      user: user,
      pressUser: function(){ fire(docListeners.mousedown, { isTrusted: true, target: user, button: 0 }); },
      typeUser: function(value, trusted){ user.value = value; fire(docListeners.input, { isTrusted: trusted, target: user }); },
      offer: function(names, d, f, o, bare){
          return window.__vanillaPasswords.offer(d === undefined ? lastDoc : d, f === undefined ? lastField : f,
                                                 o === undefined ? self.origin : o, names, bare); },
      // the last activation's field filled, as the host would ('fill').
      fillWith: function(u, p){ return window.__vanillaPasswords.fill(lastDoc, lastField, self.origin, u, p); },
      fillLast: function(){ return window.__vanillaPasswords.fill(lastDoc, lastField, self.origin, 'alice', 'pw'); },
      closedByHost: function(d, g, f){
          window.__vanillaPasswords.closed(d === undefined ? lastDoc : d, f === undefined ? lastField : f, g); },
      setOrigin: function(o){ self.origin = o; },
      click: function(el){ fire(docListeners.click, { isTrusted: true, target: el }); },
      fakeClick: function(el){ fire(docListeners.click, { isTrusted: false, target: el }); },
      enter: function(){ fire(docListeners.keydown, { isTrusted: true, target: pass, key: 'Enter' }); },
      type: function(trusted){ fire(pass.ls.input, { isTrusted: trusted }); },
      go: go, submit: submit,
      // the fields of the document now, from [type, value, attributes]; the
      // first is the one Enter is pressed in.
      only: function(specs){
          inputs.length = 0;
          specs.forEach(function(s, i){
              var e = new El('INPUT', s[0], s[1], 1 + i);
              e.attrs = s[2] || {};
              if(e.attrs.maxlength) e.maxLength = Number(e.attrs.maxlength);
              inputs.push(e); });
          pass = inputs[0]; },
      // the field Enter is pressed in now.
      field: function(){ return pass; },
      // the fields out of any form, as a page of script lays them out.
      unform: function(){ user.form = null; pass.form = null; },
      // the fields of the document now: the username, then these passwords.
      fields: function(values, marks){
          inputs.length = 0; inputs.push(user);
          values.forEach(function(v, i){
              var p = new El('INPUT', 'password', v, 2 + i);
              if(marks && marks[i]) p.attrs = { autocomplete: marks[i] };
              inputs.push(p); });
          pass = inputs[1]; },
      reset: function(){ calls.length = 0; pass.isConnected = true; inputs.length = 0; inputs.push(user, pass); },
      // the ticket the last held click left, if the host did not take it.
      untaken: function(){ return window.__vanillaPasswords.take(lastDocument(), lastTicket) ; },
      later: function(){ clock += 5000; },
      take: function(){ return calls.splice(0).join(','); } };
})()
)JS").arg(WebEnginePasswords::PageScript(), top);
    }
}
#endif

class tst_passwordpolicy : public QObject {
    Q_OBJECT

private slots:

    void aHeldClickIsOfferedOnceItTurnsOutToHaveSent(){
        const QString doc = QStringLiteral("d1");
        const QUrl at(QStringLiteral("https://a.example/login#x"));
        PasswordHold::Credentials alice;
        alice.username = QStringLiteral("alice");
        alice.password = QStringLiteral("pw");

        PasswordHold hold;
        QVERIFY(hold.IsEmpty());
        hold.Begin(doc, at);
        QVERIFY(!hold.Taken(doc, alice));
        QVERIFY(!hold.Confirm(QStringLiteral("other")));
        QVERIFY(hold.Confirm(doc));
        QCOMPARE(hold.Take().password, QStringLiteral("pw"));
        QVERIFY(hold.IsEmpty());
        QVERIFY(!hold.Confirm(doc));

        hold.Begin(doc, at);
        QVERIFY(!hold.Confirm(doc));
        QVERIFY(hold.Taken(doc, alice));
        QCOMPARE(hold.Take().username, QStringLiteral("alice"));

        hold.Begin(doc, at);
        QVERIFY(!hold.Taken(doc, alice));
        QVERIFY(!hold.Navigated(QUrl(QStringLiteral("https://a.example/login#y"))));
        QVERIFY(!hold.Navigated(QUrl(QStringLiteral("https://a.example/login"))));
        QVERIFY(hold.Navigated(QUrl(QStringLiteral("https://a.example/welcome"))));
        QVERIFY(!hold.Take().password.isEmpty());

        hold.Begin(doc, at);
        QVERIFY(!hold.Navigated(QUrl(QStringLiteral("https://a.example/next"))));
        QVERIFY(hold.Taken(doc, alice));
        hold.Clear();

        hold.Begin(doc, at);
        hold.Release(QStringLiteral("other"));
        QVERIFY(!hold.IsEmpty());
        hold.Release(doc);
        QVERIFY(hold.IsEmpty());
        QVERIFY(!hold.Taken(doc, alice));
        hold.Begin(doc, at);
        hold.Begin(QStringLiteral("d2"), at);
        QVERIFY(!hold.Taken(doc, alice));
        QVERIFY(!hold.Confirm(doc));
        hold.Clear();
        QVERIFY(!hold.Navigated(QUrl(QStringLiteral("https://b.example/"))));
    }

    void theFramesAreEmptiedOneAtATime(){
        FrameDrain drain;
        QVERIFY(drain.Knock());
        const quint64 first = drain.Begin();
        QVERIFY(drain.IsRunning());
        QVERIFY(!drain.Knock());
        QVERIFY(!drain.Knock());
        QVERIFY(drain.End(first));
        QVERIFY(!drain.IsRunning());
        QVERIFY(!drain.End(first));

        QVERIFY(drain.Knock());
        const quint64 second = drain.Begin();
        QVERIFY(!drain.End(first));
        QVERIFY(drain.IsRunning());
        QVERIFY(!drain.End(second));
        QVERIFY(!drain.IsRunning());

        drain.Knock();
        const quint64 third = drain.Begin();
        QVERIFY(!drain.Knock());
        drain.Reset();
        QVERIFY(!drain.IsRunning());
        QVERIFY(drain.Knock());
        const quint64 fourth = drain.Begin();
        QVERIFY(!drain.End(third));
        QVERIFY(drain.IsRunning());
        QVERIFY(!drain.End(fourth));
    }

#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)
    void theFramesEmptyingGoesWithThePageOnly(){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                   QStringLiteral("/view/webengine/webenginepasswords.cpp"));
        QVERIFY(file.open(QFile::ReadOnly));
        const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
        auto body = [&](const QString &head){
            const int at = source.indexOf(head);
            if(at < 0) return QString();
            return source.mid(at, source.indexOf(QStringLiteral("\n}\n"), at) - at);
        };
        const QRegularExpression forget(QStringLiteral("\\bForget\\(\\)"));
        foreach(const QString &head, QStringList()
                << QStringLiteral("void WebEnginePasswords::ShowMenu(")
                << QStringLiteral("void WebEnginePasswords::Fill(")){
            const QString text = body(head);
            QVERIFY2(!text.isEmpty(), qPrintable(head));
            QVERIFY2(!text.contains(forget), qPrintable(head));
            QVERIFY2(!text.contains(QStringLiteral("m_Drain.Reset")), qPrintable(head));
        }
        QCOMPARE(source.count(QStringLiteral("m_Drain.Reset()")), 1);
        QVERIFY(body(QStringLiteral("void WebEnginePasswords::Forget(")).contains(QStringLiteral("m_Drain.Reset()")));
    }

    void theFramesScriptIsTheQuickProfiles(){
        auto read = [](const char *path){
            QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) + QLatin1String(path));
            if(!file.open(QFile::ReadOnly)) return QString();
            return QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
        };
        const QString controller = read("/app/networkcontroller.cpp");
        QVERIFY(!controller.isEmpty());
        const int made = controller.count(QStringLiteral("ConnectQuickNotifications(profile);"));
        QVERIFY(made >= 3);
        QCOMPARE(controller.count(QRegularExpression(QStringLiteral(
                     "ConnectQuickNotifications\\(profile\\);\\n\\s*InstallQuickFrameScript\\(profile\\);"))), made);
        const QString qml = read("/view/webengine/quickwebengineview6.qml");
        QVERIFY(!qml.isEmpty());
        QVERIFY(!qml.contains(QStringLiteral("vanilla-passwords-frames")));
    }

    void theListsKeysAreTheHostsOnlyWhileItIsOpen(){
        typedef SuggestionKeys K;
        QCOMPARE(K::Classify(Qt::Key_Up, Qt::NoModifier), K::Up);
        QCOMPARE(K::Classify(Qt::Key_Down, Qt::KeypadModifier), K::Down);
        QCOMPARE(K::Classify(Qt::Key_Return, Qt::NoModifier), K::Enter);
        QCOMPARE(K::Classify(Qt::Key_Enter, Qt::KeypadModifier), K::Enter);
        QCOMPARE(K::Classify(Qt::Key_Escape, Qt::NoModifier), K::Escape);
        QCOMPARE(K::Classify(Qt::Key_Tab, Qt::NoModifier), K::Tab);
        QCOMPARE(K::Classify(Qt::Key_Backtab, Qt::ShiftModifier), K::Tab);
        QCOMPARE(K::Classify(Qt::Key_Down, Qt::ControlModifier), K::Other);
        QCOMPARE(K::Classify(Qt::Key_Return, Qt::AltModifier), K::Other);
        QCOMPARE(K::Classify(Qt::Key_Tab, Qt::ControlModifier), K::Other);
        QCOMPARE(K::Classify(Qt::Key_A, Qt::NoModifier), K::Other);
        QCOMPARE(K::Classify(Qt::Key_Backspace, Qt::NoModifier), K::Other);

        foreach(K::Key key, QList<K::Key>() << K::Up << K::Down << K::Enter << K::Escape << K::Tab << K::Other)
            QCOMPARE(K::Decide(false, false, false, key), K::Pass);
        QCOMPARE(K::Decide(true, true, false, K::Up), K::Move);
        QCOMPARE(K::Decide(true, true, false, K::Down), K::Move);
        QCOMPARE(K::Decide(true, true, true, K::Enter), K::Choose);
        QCOMPARE(K::Decide(true, true, false, K::Enter), K::CloseAndPass);
        QCOMPARE(K::Decide(true, true, false, K::Escape), K::Close);
        QCOMPARE(K::Decide(true, true, true, K::Tab), K::CloseAndPass);
        QCOMPARE(K::Decide(true, true, true, K::Other), K::Edit);
        QCOMPARE(K::Decide(true, false, true, K::Enter), K::Pass);
        QCOMPARE(K::Decide(true, false, false, K::Down), K::Pass);
        QCOMPARE(K::Decide(true, false, false, K::Escape), K::CloseAndPass);
        QCOMPARE(K::Decide(true, false, false, K::Tab), K::CloseAndPass);
        QCOMPARE(K::Decide(true, false, false, K::Other), K::Edit);
        QVERIFY(K::Eaten(K::Move) && K::Eaten(K::Choose) && K::Eaten(K::Close));
        QVERIFY(!K::Eaten(K::Pass) && !K::Eaten(K::Edit) && !K::Eaten(K::CloseAndPass));
    }

    void enterOutsideAFormIsHeld(){
        QJSEngine engine;
        QJSValue frame = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!frame.isError(), qPrintable(frame.toString()));
        auto call = [&](const char *name){
            const QJSValue r = frame.property(QString::fromLatin1(name)).callWithInstance(frame);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto kinds = [&](){
            QStringList list;
            const QJsonArray box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                                           .callWithInstance(frame).toString().toUtf8()).array();
            for(const QJsonValue &e : box) list << e[QStringLiteral("kind")].toString();
            return list.join(QLatin1Char(','));
        };
        kinds();
        call("enter");
        QCOMPARE(kinds(), QStringLiteral("submitted"));
        call("later");
        call("unform");
        call("enter");
        QCOMPARE(kinds(), QStringLiteral("held"));
        frame.property(QStringLiteral("pass")).setProperty(QStringLiteral("isConnected"), false);
        call("mutate");
        QCOMPARE(kinds(), QStringLiteral("confirmed"));
    }

    void theNameStepIsAFormOfANameAlone_data(){
        QTest::addColumn<QString>("fields");
        QTest::addColumn<QString>("sent");
        QTest::newRow("an email") << R"([["email","alice@example.com"]])" << "alice@example.com";
        QTest::newRow("a username by its autocomplete") << R"([["text","alice",{"autocomplete":"username"}]])" << "alice";
        QTest::newRow("a login by its name") << R"([["text","  alice ",{"name":"login"}]])" << "alice";
        QTest::newRow("unnamed text") << R"([["text","alice"]])" << "";
        QTest::newRow("a password beside") << R"([["email","alice@example.com"],["password",""]])" << "";
        QTest::newRow("a field named like a password") << R"([["text","alice",{"name":"login"}],["text","x",{"name":"passwd"}]])" << "";
        QTest::newRow("a password shown, by its autocomplete") << R"([["text","alice",{"name":"login","autocomplete":"current-password"}]])" << "";
        QTest::newRow("a code") << R"([["text","123456",{"name":"login_code","autocomplete":"one-time-code"}]])" << "";
        QTest::newRow("a numeric code") << R"([["text","1234",{"name":"user","inputmode":"numeric","maxlength":"6"}]])" << "";
        QTest::newRow("a search") << R"([["search","alice",{"name":"user"}]])" << "";
        QTest::newRow("a search by its name") << R"([["text","alice",{"name":"q","id":"user"}]])" << "";
        QTest::newRow("two names") << R"([["email","a@x"],["text","b",{"name":"user"}]])" << "";
        QTest::newRow("empty") << R"([["email","  "]])" << "";
    }
    void theNameStepIsAFormOfANameAlone(){
        QFETCH(QString, fields);
        QFETCH(QString, sent);
        QJSEngine engine;
        QJSValue frame = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!frame.isError(), qPrintable(frame.toString()));
        frame.property(QStringLiteral("drain")).callWithInstance(frame);
        QJSValue r = frame.property(QStringLiteral("only")).callWithInstance(frame, QJSValueList() << engine.evaluate(QLatin1Char('(') + fields + QLatin1Char(')')));
        QVERIFY2(!r.isError(), qPrintable(r.toString()));
        r = frame.property(QStringLiteral("enter")).callWithInstance(frame);
        QVERIFY2(!r.isError(), qPrintable(r.toString()));
        const QJsonArray box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                                       .callWithInstance(frame).toString().toUtf8()).array();
        if(sent.isEmpty()){
            QCOMPARE(box.size(), 0);
            return;
        }
        QCOMPARE(box.size(), 1);
        QCOMPARE(box[0][QStringLiteral("kind")].toString(), QStringLiteral("submitted"));
        QCOMPARE(box[0][QStringLiteral("username")].toString(), sent);
        QCOMPARE(box[0][QStringLiteral("password")].toString(), QString());
    }

    void aNameStepIsHeldByItsNameAndFillsItsNameOnly(){
        QJSEngine engine;
        QJSValue frame = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!frame.isError(), qPrintable(frame.toString()));
        auto call = [&](const char *name, const QJSValueList &args = QJSValueList()){
            const QJSValue r = frame.property(QString::fromLatin1(name)).callWithInstance(frame, args);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto kinds = [&](){
            QStringList list;
            const QJsonArray box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                                           .callWithInstance(frame).toString().toUtf8()).array();
            for(const QJsonValue &e : box) list << e[QStringLiteral("kind")].toString();
            return list.join(QLatin1Char(','));
        };
        kinds();
        call("only", QJSValueList() << engine.evaluate(QStringLiteral("([['email','alice@example.com']])")));
        call("unform");
        call("enter");
        QCOMPARE(kinds(), QStringLiteral("held"));
        QJSValue name = frame.property(QStringLiteral("field")).callWithInstance(frame);
        name.setProperty(QStringLiteral("isConnected"), false);
        QJSValue empty = frame.property(QStringLiteral("El")).callAsConstructor(
            QJSValueList() << QStringLiteral("INPUT") << QStringLiteral("password") << QString() << 5);
        frame.property(QStringLiteral("inputs")).property(QStringLiteral("push")).callWithInstance(
            frame.property(QStringLiteral("inputs")), QJSValueList() << empty);
        call("mutate");
        QCOMPARE(kinds(), QStringLiteral("confirmed"));

        QJSValue page = engine.evaluate(Harness(QStringLiteral("window")));
        QVERIFY2(!page.isError(), qPrintable(page.toString()));
        page.property(QStringLiteral("pressUser")).callWithInstance(page);
        QJSValue r = page.property(QStringLiteral("fillWith")).callWithInstance(
            page, QJSValueList() << QStringLiteral("bob") << QString());
        QVERIFY2(!r.isError(), qPrintable(r.toString()));
        QVERIFY(r.toBool());
        QCOMPARE(page.property(QStringLiteral("user")).property(QStringLiteral("filled")).toString(), QStringLiteral("bob"));
        QVERIFY(page.property(QStringLiteral("pass")).property(QStringLiteral("filled")).isUndefined());

        page.property(QStringLiteral("only")).callWithInstance(
            page, QJSValueList() << engine.evaluate(QStringLiteral("([['email','']])")));
        page.property(QStringLiteral("pressUser")).callWithInstance(page);
        r = page.property(QStringLiteral("fillWith")).callWithInstance(
            page, QJSValueList() << QStringLiteral("carol") << QStringLiteral("pw"));
        QVERIFY2(!r.isError(), qPrintable(r.toString()));
        QVERIFY(r.toBool());
        QCOMPARE(page.property(QStringLiteral("user")).property(QStringLiteral("filled")).toString(), QStringLiteral("carol"));
    }

    void aHoldTakesTheCredentialsOfItsOwnTicket(){
        PasswordHold hold;
        PasswordHold::Credentials first{QStringLiteral("alice"), QString()};
        PasswordHold::Credentials second{QStringLiteral("alice"), QStringLiteral("pw")};
        hold.Begin(QStringLiteral("doc"), QUrl(QStringLiteral("https://a.example/")), QStringLiteral("t1"));
        hold.Begin(QStringLiteral("doc"), QUrl(QStringLiteral("https://a.example/")), QStringLiteral("t2"));
        QVERIFY(!hold.Taken(QStringLiteral("doc"), first, QStringLiteral("t1")));
        QVERIFY(!hold.Confirm(QStringLiteral("doc")));
        QVERIFY(hold.Taken(QStringLiteral("doc"), second, QStringLiteral("t2")));
        QCOMPARE(hold.Take().password, QStringLiteral("pw"));
    }

    void whatIsCapturedIsNotKeptLonger(){
        QJSEngine engine;
        QJSValue page = engine.evaluate(Harness(QStringLiteral("window")));
        QVERIFY2(!page.isError(), qPrintable(page.toString()));
        auto call = [&](const char *name){
            const QJSValue r = page.property(QString::fromLatin1(name)).callWithInstance(page);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto take = [&](){ return page.property(QStringLiteral("take")).callWithInstance(page).toString(); };
        auto runTimers = [&](){
            QJSValue timers = page.property(QStringLiteral("timers"));
            const int n = timers.property(QStringLiteral("length")).toInt();
            for(int i = 0; i < n; i++){
                QJSValue f = timers.property(i);
                if(f.isCallable()) f.call();
            }
        };
        call("enter");
        QCOMPARE(take(), QStringLiteral("submitted"));
        call("enter");
        QCOMPARE(take(), QString());
        runTimers();
        QVERIFY(page.property(QStringLiteral("untaken")).callWithInstance(page).isNull());
        call("enter");
        QCOMPARE(take(), QStringLiteral("submitted"));
    }

    void theNewPasswordIsTheOneKept_data(){
        QTest::addColumn<QStringList>("values");
        QTest::addColumn<QStringList>("marks");
        QTest::addColumn<QString>("kept");
        QTest::newRow("a login") << QStringList{"pw"} << QStringList() << "pw";
        QTest::newRow("a sign-up") << QStringList{"new", "new"} << QStringList() << "new";
        QTest::newRow("a sign-up, mistyped") << QStringList{"new", "neu"} << QStringList() << "";
        QTest::newRow("a change") << QStringList{"old", "new", "new"} << QStringList() << "new";
        QTest::newRow("a change, mistyped") << QStringList{"old", "new", "neu"} << QStringList() << "";
        QTest::newRow("a change without a confirmation")
            << QStringList{"old", "new"} << QStringList{"current-password", "new-password"} << "new";
        QTest::newRow("two which differ, unmarked") << QStringList{"old", "new"} << QStringList() << "";
        QTest::newRow("both marked new") << QStringList{"a", "b"} << QStringList{"new-password", "new-password"} << "";
        QTest::newRow("four") << QStringList{"a", "b", "b", "b"} << QStringList() << "";
    }
    void theNewPasswordIsTheOneKept(){
        QFETCH(QStringList, values);
        QFETCH(QStringList, marks);
        QFETCH(QString, kept);
        QJSEngine engine;
        QJSValue frame = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!frame.isError(), qPrintable(frame.toString()));
        frame.property(QStringLiteral("drain")).callWithInstance(frame);
        QJSValue v = engine.newArray(values.size()), m = engine.newArray(marks.size());
        for(int i = 0; i < values.size(); i++) v.setProperty(i, values.at(i));
        for(int i = 0; i < marks.size(); i++) m.setProperty(i, marks.at(i));
        QJSValue r = frame.property(QStringLiteral("fields")).callWithInstance(frame, QJSValueList() << v << m);
        QVERIFY2(!r.isError(), qPrintable(r.toString()));
        r = frame.property(QStringLiteral("enter")).callWithInstance(frame);
        QVERIFY2(!r.isError(), qPrintable(r.toString()));
        const QJsonArray box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                                       .callWithInstance(frame).toString().toUtf8()).array();
        if(kept.isEmpty()){
            QCOMPARE(box.size(), 0);
            return;
        }
        QCOMPARE(box.size(), 1);
        QCOMPARE(box[0][QStringLiteral("password")].toString(), kept);
        QCOMPARE(box[0][QStringLiteral("username")].toString(), QStringLiteral("alice"));
    }

    void theScriptNarrowsTheNamesAndSaysOnlyWhich(){
        QJSEngine engine;
        QJSValue page = engine.evaluate(Harness(QStringLiteral("window")));
        QVERIFY2(!page.isError(), qPrintable(page.toString()));
        auto ask = [&](const char *name, const QJSValueList &args = QJSValueList()){
            return page.property(QString::fromLatin1(name)).callWithInstance(page, args);
        };
        auto call = [&](const char *name, const QJSValueList &args = QJSValueList()){
            const QJSValue r = ask(name, args);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto take = [&](){ return page.property(QStringLiteral("take")).callWithInstance(page).toString(); };
        QJSValue names = engine.newArray(3);
        names.setProperty(0, QStringLiteral("alice"));
        names.setProperty(1, QStringLiteral("bob"));
        names.setProperty(2, QStringLiteral("Al"));

        call("pressUser");
        take();
        QVERIFY(ask("offer", QJSValueList() << names).toBool());
        QCOMPARE(take(), QStringLiteral("filtered 0"));
        call("typeUser", QJSValueList() << QStringLiteral("A") << true);
        QCOMPARE(take(), QStringLiteral("filtered 0.2"));
        call("typeUser", QJSValueList() << QStringLiteral("") << true);
        QCOMPARE(take(), QStringLiteral("filtered 0.1.2"));
        call("typeUser", QJSValueList() << QStringLiteral("z") << true);
        QCOMPARE(take(), QStringLiteral("filtered "));
        call("typeUser", QJSValueList() << QStringLiteral("b") << false);
        QCOMPARE(take(), QString());
        QVERIFY(!ask("offer", QJSValueList() << names << QStringLiteral("another")).toBool());
        QVERIFY(!ask("offer", QJSValueList() << names << QJSValue() << QStringLiteral("another")).toBool());
        QVERIFY(!ask("offer", QJSValueList() << names << QJSValue() << QJSValue() << QStringLiteral("null")).toBool());
        QCOMPARE(take(), QString());

        call("press");
        take();
        QVERIFY(ask("offer", QJSValueList() << names).toBool());
        QCOMPARE(take(), QStringLiteral("filtered 0.1.2"));
        call("press");
        take();
        QVERIFY(ask("offer", QJSValueList() << names << QJSValue() << QJSValue() << QJSValue()
                    << engine.evaluate(QStringLiteral("[false, true, false]"))).toBool());
        QCOMPARE(take(), QStringLiteral("filtered 0.2"));
        call("blur", QJSValueList() << true);
        take();
        call("typeUser", QJSValueList() << QStringLiteral("a") << true);
        QCOMPARE(take(), QStringLiteral("activated 12 34 -1"));
        QVERIFY(ask("offer", QJSValueList() << names).toBool());
        QCOMPARE(take(), QStringLiteral("filtered 0.2"));
        call("typeUser", QJSValueList() << QStringLiteral("b") << true);
        QCOMPARE(take(), QStringLiteral("filtered 1"));
        call("closedByHost", QJSValueList() << QJSValue() << 3 << QStringLiteral("older"));
        call("typeUser", QJSValueList() << QStringLiteral("bo") << true);
        QCOMPARE(take(), QStringLiteral("filtered 1"));
        call("closedByHost", QJSValueList() << QJSValue() << 4);
        call("typeUser", QJSValueList() << QStringLiteral("bob") << true);
        QCOMPARE(take(), QStringLiteral("activated 12 34 4"));
        QVERIFY(ask("fillLast").toBool());
        call("typeUser", QJSValueList() << QStringLiteral("al") << true);
        QCOMPARE(take(), QStringLiteral("activated 12 34 4"));
        call("blur", QJSValueList() << true);
        take();
        call("typeUser", QJSValueList() << QStringLiteral("x") << false);
        QCOMPARE(take(), QString());

        QJSValue frame = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!frame.isError(), qPrintable(frame.toString()));
        frame.property(QStringLiteral("pressUser")).callWithInstance(frame);
        QJsonArray box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                                 .callWithInstance(frame).toString().toUtf8()).array();
        QCOMPARE(box.size(), 1);
        const QString document = box[0][QStringLiteral("document")].toString();
        const QString field = box[0][QStringLiteral("field")].toString();
        QVERIFY(frame.property(QStringLiteral("offer")).callWithInstance(frame, QJSValueList() << names << document << field).toBool());
        frame.property(QStringLiteral("typeUser")).callWithInstance(frame, QJSValueList() << QStringLiteral("b") << true);
        box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                      .callWithInstance(frame).toString().toUtf8()).array();
        QCOMPARE(box.size(), 1);
        QCOMPARE(box[0][QStringLiteral("kind")].toString(), QStringLiteral("filtered"));
        QCOMPARE(box[0][QStringLiteral("numbers")].toArray(), QJsonArray() << 1);
        QVERIFY(!box[0].toObject().contains(QStringLiteral("value")));
        QCOMPARE(frame.property(QStringLiteral("take")).callWithInstance(frame).toString(), QString());
    }

    void theScriptSaysWhenTheListGoes(){
        QJSEngine engine;
        QJSValue page = engine.evaluate(Harness(QStringLiteral("window")));
        QVERIFY2(!page.isError(), qPrintable(page.toString()));
        auto call = [&](const char *name, const QJSValueList &args = QJSValueList()){
            const QJSValue r = page.property(QString::fromLatin1(name)).callWithInstance(page, args);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto take = [&](){ return page.property(QStringLiteral("take")).callWithInstance(page).toString(); };

        call("press");
        QCOMPARE(take(), QStringLiteral("activated 12 34 -1"));
        call("down");
        QCOMPARE(take(), QStringLiteral("activated 12 34 -1"));
        call("blur", QJSValueList() << false);
        call("scroll", QJSValueList() << false);
        call("blur", QJSValueList() << true << page.property(QStringLiteral("user")));
        QCOMPARE(take(), QString());
        call("blur", QJSValueList() << true);
        QCOMPARE(take(), QStringLiteral("closed"));
        call("blur", QJSValueList() << true);
        call("scroll", QJSValueList() << true);
        QCOMPARE(take(), QString());
        call("press");
        call("scroll", QJSValueList() << true);
        QCOMPARE(take(), QStringLiteral("activated 12 34 -1,closed"));
        QCOMPARE(page.property(QStringLiteral("fillLast")).callWithInstance(page).toBool(), false);
        call("press");
        take();
        QCOMPARE(page.property(QStringLiteral("fillLast")).callWithInstance(page).toBool(), true);

        call("closedByHost", QJSValueList() << QStringLiteral("another") << 7);
        call("press");
        QCOMPARE(take(), QStringLiteral("activated 12 34 -1"));
        call("closedByHost", QJSValueList() << QJSValue() << 7);
        call("press");
        QCOMPARE(take(), QStringLiteral("activated 12 34 7"));

        QJSValue frame = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!frame.isError(), qPrintable(frame.toString()));
        frame.property(QStringLiteral("press")).callWithInstance(frame);
        frame.property(QStringLiteral("blur")).callWithInstance(frame, QJSValueList() << true);
        const QJsonArray box = QJsonDocument::fromJson(frame.property(QStringLiteral("drain"))
                                                       .callWithInstance(frame).toString().toUtf8()).array();
        QCOMPARE(box.size(), 2);
        QCOMPARE(box[0][QStringLiteral("kind")].toString(), QStringLiteral("activated"));
        QCOMPARE(box[1][QStringLiteral("kind")].toString(), QStringLiteral("closed"));
        QCOMPARE(box[1][QStringLiteral("field")].toString(), box[0][QStringLiteral("field")].toString());
        QCOMPARE(frame.property(QStringLiteral("take")).callWithInstance(frame).toString(), QString());
    }

    void aFrameKnocksAndIsEmptiedOnce(){
        QJSEngine engine;
        QJSValue page = engine.evaluate(Harness(QStringLiteral("topWindow")));
        QVERIFY2(!page.isError(), qPrintable(page.toString()));
        auto call = [&](const char *name, const QJSValueList &args = QJSValueList()){
            const QJSValue r = page.property(QString::fromLatin1(name)).callWithInstance(page, args);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto drain = [&](){
            return QJsonDocument::fromJson(page.property(QStringLiteral("drain"))
                                           .callWithInstance(page).toString().toUtf8()).array();
        };
        auto knocks = [&](){
            QStringList list = page.property(QStringLiteral("knocks")).toVariant().toStringList();
            page.property(QStringLiteral("knocks")).setProperty(QStringLiteral("length"), 0);
            return list;
        };
        auto take = [&](){ return page.property(QStringLiteral("take")).callWithInstance(page).toString(); };
        QJSValue go = page.property(QStringLiteral("go"));
        QJSValue pass = page.property(QStringLiteral("pass"));

        QCOMPARE(knocks(), QStringList() << QStringLiteral("vanilla-passwords-loaded"));

        call("enter");
        QCOMPARE(knocks(), QStringList() << QStringLiteral("vanilla-passwords"));
        QJsonArray box = drain();
        QCOMPARE(box.size(), 1);
        QCOMPARE(box[0][QStringLiteral("kind")].toString(), QStringLiteral("submitted"));
        QCOMPARE(box[0][QStringLiteral("origin")].toString(), QStringLiteral("https://a.example"));
        QCOMPARE(box[0][QStringLiteral("username")].toString(), QStringLiteral("alice"));
        QCOMPARE(box[0][QStringLiteral("password")].toString(), QStringLiteral("pw"));
        QVERIFY(!box[0].toObject().contains(QStringLiteral("at")));
        QCOMPARE(drain().size(), 0);
        call("later");

        call("click", QJSValueList() << go);
        call("type", QJSValueList() << true);
        box = drain();
        QCOMPARE(box.size(), 1);
        QCOMPARE(box[0][QStringLiteral("kind")].toString(), QStringLiteral("released"));
        call("click", QJSValueList() << go);
        pass.setProperty(QStringLiteral("isConnected"), false);
        call("mutate");
        box = drain();
        QCOMPARE(box.size(), 2);
        QCOMPARE(box[0][QStringLiteral("kind")].toString(), QStringLiteral("held"));
        QCOMPARE(box[0][QStringLiteral("password")].toString(), QStringLiteral("pw"));
        QCOMPARE(box[1][QStringLiteral("kind")].toString(), QStringLiteral("confirmed"));
        call("reset");

        for(int i = 0; i < 20; i++) call("press");
        QCOMPARE(drain().size(), 16);
        call("press");
        call("later");
        call("later");
        QCOMPARE(drain().size(), 0);

        call("setOrigin", QJSValueList() << QStringLiteral("null"));
        call("enter");
        box = drain();
        QCOMPARE(box.size(), 1);
        QCOMPARE(box[0][QStringLiteral("origin")].toString(), QStringLiteral("null"));

        foreach(const QString &knock, knocks())
            QCOMPARE(knock, QStringLiteral("vanilla-passwords"));
        QCOMPARE(take(), QString());

        QJSValue top = engine.evaluate(Harness(QStringLiteral("window")));
        QVERIFY2(!top.isError(), qPrintable(top.toString()));
        QJSValue message = top.property(QStringLiteral("message"));
        QJSValue other = engine.newObject();
        message.callWithInstance(top, QJSValueList() << QStringLiteral("vanilla-passwords") << other);
        message.callWithInstance(top, QJSValueList() << QStringLiteral("vanilla-passwords-loaded") << other);
        message.callWithInstance(top, QJSValueList() << QStringLiteral("vanilla-passwords")
                                                     << top.property(QStringLiteral("win")));
        message.callWithInstance(top, QJSValueList() << QStringLiteral("something") << other);
        QCOMPARE(top.property(QStringLiteral("take")).callWithInstance(top).toString(),
                 QStringLiteral("rang,rang-loaded"));
        QCOMPARE(top.property(QStringLiteral("knocks")).property(QStringLiteral("length")).toInt(), 0);
    }

    void theScriptHoldsAButtonClickUntilTheFieldGoes(){
        QJSEngine engine;
        const QString harness = Harness(QStringLiteral("window"));

        QJSValue page = engine.evaluate(harness);
        QVERIFY2(!page.isError(), qPrintable(page.toString()));
        auto call = [&](const char *name, const QJSValueList &args = QJSValueList()){
            const QJSValue r = page.property(QString::fromLatin1(name)).callWithInstance(page, args);
            QVERIFY2(!r.isError(), qPrintable(r.toString()));
        };
        auto take = [&](){ return page.property(QStringLiteral("take")).callWithInstance(page).toString(); };
        QJSValue go = page.property(QStringLiteral("go"));
        QJSValue pass = page.property(QStringLiteral("pass"));

        call("click", QJSValueList() << go);
        QCOMPARE(take(), QStringLiteral("held"));
        call("mutate");
        QCOMPARE(take(), QString());
        pass.setProperty(QStringLiteral("isConnected"), false);
        call("mutate");
        QCOMPARE(take(), QStringLiteral("confirmed"));
        call("mutate");
        QCOMPARE(take(), QString());
        call("reset");

        call("click", QJSValueList() << go);
        QCOMPARE(take(), QStringLiteral("held"));
        QJSValue text = page.property(QStringLiteral("El")).callAsConstructor
            (QJSValueList() << QStringLiteral("INPUT") << QStringLiteral("text") << QStringLiteral("pw") << 3);
        page.property(QStringLiteral("inputs")).property(QStringLiteral("push")).callWithInstance
            (page.property(QStringLiteral("inputs")), QJSValueList() << text);
        pass.setProperty(QStringLiteral("isConnected"), false);
        call("mutate");
        QCOMPARE(take(), QString());
        call("reset");

        call("click", QJSValueList() << go);
        take();
        call("type", QJSValueList() << false);
        QCOMPARE(take(), QString());
        call("type", QJSValueList() << true);
        QCOMPARE(take(), QStringLiteral("released"));
        call("reset");

        call("click", QJSValueList() << go);
        QJSValue timers = page.property(QStringLiteral("timers"));
        QJSValue first = timers.property(timers.property(QStringLiteral("length")).toInt() - 1);
        QVERIFY(first.isCallable());
        call("click", QJSValueList() << go);
        QCOMPARE(take(), QStringLiteral("held,released,held"));
        first.call();
        QCOMPARE(take(), QString());
        timers.property(timers.property(QStringLiteral("length")).toInt() - 1).call();
        QCOMPARE(take(), QStringLiteral("released"));
        QVERIFY(page.property(QStringLiteral("untaken")).callWithInstance(page).isNull());
        call("reset");

        call("enter");
        take();
        call("click", QJSValueList() << go);
        QCOMPARE(take(), QStringLiteral("held"));
        call("enter");
        QCOMPARE(take(), QStringLiteral("released"));
        call("reset");
        page.property(QStringLiteral("later")).callWithInstance(page);

        call("click", QJSValueList() << go);
        take();
        call("enter");
        QCOMPARE(take(), QStringLiteral("submitted"));
        pass.setProperty(QStringLiteral("isConnected"), false);
        call("mutate");
        QCOMPARE(take(), QString());
        call("reset");
        call("click", QJSValueList() << page.property(QStringLiteral("submit")));
        QCOMPARE(take(), QString());
        call("fakeClick", QJSValueList() << go);
        QCOMPARE(take(), QString());
    }
#endif

    void theOriginIsMadeFromTheAddress_data(){
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("origin");
        QTest::addColumn<QString>("scriptOrigin");

        QTest::newRow("https, the default port filled in")
            << QStringLiteral("https://example.com/login?x=1#top")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("the default port written out is the same origin")
            << QStringLiteral("https://example.com:443/")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("another port is another origin")
            << QStringLiteral("https://example.com:8443/")
            << QStringLiteral("https://example.com:8443")
            << QStringLiteral("https://example.com:8443");
        QTest::newRow("upper case comes out lower")
            << QStringLiteral("HTTPS://Example.COM/")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("an IDN is its punycode")
            << QStringLiteral("https://\u4F8B\u3048.jp/")
            << QStringLiteral("https://xn--r8jz45g.jp:443")
            << QStringLiteral("https://xn--r8jz45g.jp");
        QTest::newRow("user info is not part of it")
            << QStringLiteral("https://user:pass@example.com/")
            << QStringLiteral("https://example.com:443")
            << QStringLiteral("https://example.com");
        QTest::newRow("http on 127.0.0.1")
            << QStringLiteral("http://127.0.0.1:8765/login.html")
            << QStringLiteral("http://127.0.0.1:8765")
            << QStringLiteral("http://127.0.0.1:8765");
        QTest::newRow("http anywhere in 127.0.0.0/8")
            << QStringLiteral("http://127.1.2.3/")
            << QStringLiteral("http://127.1.2.3:80")
            << QStringLiteral("http://127.1.2.3");
        QTest::newRow("http on localhost")
            << QStringLiteral("http://localhost/")
            << QStringLiteral("http://localhost:80")
            << QStringLiteral("http://localhost");
        QTest::newRow("http on ::1, in brackets")
            << QStringLiteral("http://[::1]:8080/")
            << QStringLiteral("http://[::1]:8080")
            << QStringLiteral("http://[::1]:8080");
        QTest::newRow("https on IPv6")
            << QStringLiteral("https://[2001:DB8::1]/")
            << QStringLiteral("https://[2001:db8::1]:443")
            << QStringLiteral("https://[2001:db8::1]");

        QTest::newRow("http over the network: none")
            << QStringLiteral("http://example.com/") << QString() << QString();
        QTest::newRow("a name which only ends like localhost: none")
            << QStringLiteral("http://evil-localhost/") << QString() << QString();
        QTest::newRow("localhost as a subdomain: none")
            << QStringLiteral("http://localhost.example.com/") << QString() << QString();
        QTest::newRow("file: none")
            << QStringLiteral("file:///C:/login.html") << QString() << QString();
        QTest::newRow("data: none")
            << QStringLiteral("data:text/html,<form>") << QString() << QString();
        QTest::newRow("about:blank: none")
            << QStringLiteral("about:blank") << QString() << QString();
        QTest::newRow("vanilla:// none")
            << QStringLiteral("vanilla://settings") << QString() << QString();
        QTest::newRow("empty: none")
            << QString() << QString() << QString();
    }
    void theOriginIsMadeFromTheAddress(){
        QFETCH(QString, url);
        QFETCH(QString, origin);
        QFETCH(QString, scriptOrigin);
        QCOMPARE(PasswordPolicy::Origin(QUrl(url)), origin);
        QCOMPARE(PasswordPolicy::ScriptOrigin(QUrl(url)), scriptOrigin);
    }

    void theKeyKeepsItsPartsApart(){
        const QString prefix = QStringLiteral("vanilla-0123");
        const QString a = PasswordPolicy::VaultKey(prefix, QStringLiteral("space"),
                                                   QStringLiteral("https://a.example:443"),
                                                   QStringLiteral("alice"));
        QCOMPARE(a, QStringLiteral("vanilla-0123|space|https://a.example:443|alice"));

        const QString forged = PasswordPolicy::VaultKey(prefix, QStringLiteral("space"),
                                                        QStringLiteral("https://a.example:443"),
                                                        QStringLiteral("x|https://b.example:443|alice"));
        const QString other = PasswordPolicy::VaultKey(prefix, QStringLiteral("space"),
                                                       QStringLiteral("https://b.example:443"),
                                                       QStringLiteral("alice"));
        QVERIFY(forged != other);
        QCOMPARE(forged, QStringLiteral("vanilla-0123|space|https://a.example:443|x\\|https://b.example:443\\|alice"));

        QCOMPARE(PasswordPolicy::VaultKey(prefix, QStringLiteral("s"), QStringLiteral("o"),
                                          QStringLiteral("a\\")),
                 QStringLiteral("vanilla-0123|s|o|a\\\\"));
        QVERIFY(PasswordPolicy::VaultKey(prefix, QStringLiteral("s\\"), QStringLiteral("|o"), QString()) !=
                PasswordPolicy::VaultKey(prefix, QStringLiteral("s"), QStringLiteral("\\|o"), QString()));

        QVERIFY(PasswordPolicy::VaultKey(prefix, QStringLiteral("one"), QStringLiteral("o"), QStringLiteral("u")) !=
                PasswordPolicy::VaultKey(prefix, QStringLiteral("two"), QStringLiteral("o"), QStringLiteral("u")));
    }
};

QTEST_MAIN(tst_passwordpolicy)
#include "tst_passwordpolicy.moc"
