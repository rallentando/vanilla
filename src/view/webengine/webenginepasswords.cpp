#include "switch.hpp"

#include "webenginepasswords.hpp"

#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)

#include "passwordpolicy.hpp"
#include "passwordstore.hpp"
#include "networkcontroller.hpp"
#include "dialog.hpp"
#include "view.hpp"
#include "passwordsuggestions.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QCursor>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#ifdef VANILLA_QUICK_PASSWORD_FRAMES
#  include <QQuickWebEngineProfile>
#  include <QtWebEngineQuick/private/qquickwebenginescriptcollection_p.h>
#endif

#include <memory>

namespace {

    const char *PasswordJsCode = R"JS(
(function(){
'use strict';
if(window.__vanillaPasswords) return;
var inMain = window.top === window;
if(inMain && (typeof qt === 'undefined' || !qt.webChannelTransport)) return;

function mark(){
    var words = new Uint32Array(4);
    crypto.getRandomValues(words);
    return Array.prototype.map.call(words, function(w){
        return ('00000000' + w.toString(16)).slice(-8);
    }).join('');
}

var documentMark = mark();
var pending = null;
var field = null, fieldMark = null;
// the host may show a list for 'field' (D-611): said once when it goes.
var listed = false;
// the last list of this document the host closed, as it said (R-463 中2).
var heard = -1;
// the names the host offered for the field, narrowed here by what is typed
// into it: only the numbers of those which match go back, never the value,
// whatever the field turns out to be (D-611 追記 7).
var offered = null;
var bridge = null;
var lastCapture = null;
// a click which may have sent the form (D-609), and what is watched for it.
var candidate = null;
// what a frame has to say, until the host takes it (D-610).
var box = [];
var KNOCK = 'vanilla-passwords', LOADED = 'vanilla-passwords-loaded';

if(inMain){
    new QWebChannel(qt.webChannelTransport, function(channel){
        bridge = channel.objects.vanillaPasswords;
    });
    // a frame's knock, from another window; its own would be the page's.
    window.addEventListener('message', function(e){
        if(e.source === window || !bridge) return;
        if(e.data === KNOCK) bridge.rang(false);
        else if(e.data === LOADED) bridge.rang(true);
    });
}

function knock(what){
    try { window.top.postMessage(what, '*'); } catch(e){}
}
// in a frame: into the box, sixteen at most, and knock.
// what is older than ten seconds is let go of, as the host would not take
// it anyway: not kept in the clear until a drain which may never come
// (D-615 追記 8).
function prune(){
    var now = Date.now();
    box = box.filter(function(e){ return now - e.at < 10000; });
}
function tell(entry){
    prune();
    entry.at = Date.now();
    box.push(entry);
    if(box.length > 16) box.shift();
    setTimeout(prune, 10000);
    knock(KNOCK);
    return entry;
}
function say(kind){
    if(inMain){ if(bridge) bridge[kind](documentMark); }
    else tell({ kind: kind, document: documentMark });
}
// a frame's document has come: a held click in it may have sent its form.
if(!inMain) knock(LOADED);

function isPassword(e){
    return !!e && e.tagName === 'INPUT' && e.type === 'password';
}
function isUsername(e){
    return !!e && e.tagName === 'INPUT' &&
        (e.type === 'text' || e.type === 'email' || e.type === 'tel');
}
function scopeOf(e){
    return (e && e.form) || document;
}
function filledPasswords(scope){
    return Array.prototype.filter.call(
        scope.querySelectorAll('input[type="password"]'),
        function(p){ return p.value !== ''; });
}
function usernameFor(scope, password){
    var named = scope.querySelector('input[autocomplete~="username"]');
    if(isUsername(named)) return named;
    var found = null;
    Array.prototype.forEach.call(scope.querySelectorAll('input'), function(i){
        if(isUsername(i) &&
           (i.compareDocumentPosition(password) & Node.DOCUMENT_POSITION_FOLLOWING))
            found = i;
    });
    return found;
}

// the password to keep of those filled in (D-613): the only one; of a new
// one and its confirmation -- a sign-up, or the last two of a change -- the
// one they agree on; of two which differ, the one marked as the new one.
// Two which should agree and do not are a slip the page will refuse.
function chosenPassword(passwords){
    if(passwords.length === 1) return passwords[0];
    if(passwords.length !== 2 && passwords.length !== 3) return null;
    var last = passwords[passwords.length - 1];
    if(last.value === passwords[passwords.length - 2].value) return last;
    if(passwords.length !== 2) return null;
    var fresh = passwords.filter(function(p){
        return /(^|\s)new-password(\s|$)/.test(p.getAttribute('autocomplete') || '');
    });
    return fresh.length === 1 ? fresh[0] : null;
}

function collect(scope){
    var password = chosenPassword(filledPasswords(scope));
    if(!password) return null;
    var username = usernameFor(scope, password);
    return { field: password, scope: scope,
             origin: self.origin,
             username: username ? username.value : '',
             password: password.value, watch: password.value };
}

/*
  The name step of a login in two (D-615 改訂 1): a form with no password
  field, nor anything named like one, and one field named like a username
  -- not a code sent to the user, not a search -- with a name in it. Sent
  with no password; the question shows the name before anything is saved,
  for the user to see it is no password (R-468 高).
 */
function attribute(e, name){
    return (e && e.getAttribute && e.getAttribute(name)) || '';
}
function passwordish(e){
    return /pass|pwd|secret/i.test(attribute(e, 'name') + ' ' + attribute(e, 'id')) ||
        /(current|new)-password/.test(attribute(e, 'autocomplete'));
}
function codeish(e){
    return /one-time-code/.test(attribute(e, 'autocomplete')) ||
        /otp|code|pin|token|verif/i.test(attribute(e, 'name') + ' ' + attribute(e, 'id')) ||
        (attribute(e, 'inputmode') === 'numeric' && e.maxLength > 0 && e.maxLength <= 8);
}
function searchish(e){
    return e.type === 'search' || /^(q|query|search)$/i.test(attribute(e, 'name')) ||
        !!(e.closest && e.closest('[role="search"]'));
}
function nameish(e){
    return /username|email/.test(attribute(e, 'autocomplete')) || e.type === 'email' ||
        /user|login|mail|account/i.test(attribute(e, 'name') + ' ' + attribute(e, 'id'));
}
function named(scope){
    var inputs = Array.prototype.slice.call(scope.querySelectorAll('input'));
    if(inputs.some(function(i){ return i.type === 'password' || passwordish(i); })) return null;
    var names = inputs.filter(function(i){
        return isUsername(i) && nameish(i) && !codeish(i) && !searchish(i);
    });
    if(names.length !== 1) return null;
    var value = String(names[0].value || '').trim();
    if(value === '' || value.length > 256) return null;
    return { field: names[0], scope: scope, origin: self.origin,
             username: value, password: '', watch: names[0].value };
}

function capture(scope){
    // a submission of its own: whatever click was held is this one, or older,
    // and is not asked about again (R-459 中3).
    var had = !!candidate;
    drop(true);
    var found = collect(scope) || named(scope);
    // Enter in a field can be followed by the click on the default button
    // which it simulates: the same credentials twice are one.
    var now = Date.now();
    if(!found || (lastCapture && now - lastCapture.at < 2000 &&
                  lastCapture.username === found.username &&
                  lastCapture.password === found.password)){
        // nothing sent: the host's hold goes all the same (R-459 final 中3).
        if(had) say('released');
        return;
    }
    var captured = { at: now, username: found.username, password: found.password };
    lastCapture = captured;
    // kept for the two seconds it is compared within, and no longer.
    setTimeout(function(){ if(lastCapture === captured) lastCapture = null; }, 2000);

    if(!inMain){
        tell({ kind: 'submitted', document: documentMark, origin: found.origin,
               username: found.username, password: found.password });
        return;
    }
    var credentials = { origin: found.origin, username: found.username,
                        password: found.password, ticket: mark() };
    pending = credentials;
    // not taken -- a private view, the setting off -- not kept: as long as
    // a held click's (D-615 追記 8).
    setTimeout(function(){ if(pending === credentials) pending = null; }, 10000);
    if(bridge) bridge.submitted(documentMark, credentials.ticket);
}

)JS"
    R"JS(
/*
  The held click (D-609). Its credentials go to the host at once, under a
  ticket as a submission's do, while this document is still here to hand
  them over; the host asks only if this says the field went, or the engine
  says the page moved on. Watched here only while there is one: the field
  out of the document or not drawn, unless an input of the same value is --
  a page which shows the password by putting a text field in its place
  (R-459 中2). Typing in the field again, ten seconds, another click or a
  submission let it go.
 */
function drawn(element){
    return element.isConnected && element.getClientRects().length > 0;
}
// (by the value watched, which is the name for a name step: an empty
//  password field coming is not one standing in its place, R-469 中4.)
function gone(held){
    if(drawn(held.field)) return false;
    var stand = Array.prototype.some.call(
        document.querySelectorAll('input'),
        function(i){ return i !== held.field && i.value === held.watch && drawn(i); });
    return !stand;
}
function drop(quiet){
    if(!candidate) return;
    var held = candidate;
    candidate = null;
    held.observer.disconnect();
    clearTimeout(held.timer);
    held.field.removeEventListener('input', held.retyped, true);
    if(!quiet){
        // and what the host did not take is not left here (R-459 final 低5).
        if(pending && pending.ticket === held.ticket) pending = null;
        var i = box.indexOf(held.entry);
        if(i >= 0) box.splice(i, 1);
        say('released');
    }
}
// once for each batch of changes: one look, however many there were.
function check(){
    if(!candidate || !gone(candidate)) return;
    drop(true);
    say('confirmed');
}
function hold(scope){
    drop(false);
    var found = collect(scope) || named(scope);
    if(!found) return;
    var held = { field: found.field, password: found.password, watch: found.watch,
                 observer: new MutationObserver(check) };
    // each about its own click only: one replaced is not this one (R-459 低6).
    held.timer = setTimeout(function(){ if(candidate === held) drop(false); }, 10000);
    held.retyped = function(e){ if(e.isTrusted && candidate === held) drop(false); };
    candidate = held;
    held.observer.observe(document, { childList: true, subtree: true, attributes: true,
                                       attributeFilter: ['style', 'class', 'hidden'] });
    held.field.addEventListener('input', held.retyped, true);
    if(!inMain){
        held.entry = tell({ kind: 'held', document: documentMark, origin: found.origin,
                            username: found.username, password: found.password });
        return;
    }
    pending = { origin: found.origin, username: found.username,
                password: found.password, ticket: mark() };
    held.ticket = pending.ticket;
    if(bridge) bridge.held(documentMark, pending.ticket);
}

function activate(target){
    field = target;
    fieldMark = mark();
    listed = true;
    offered = null;
    // in a frame, where the field is in the view is not known here: the
    // list comes at the pointer (D-610).
    if(!inMain){
        tell({ kind: 'activated', document: documentMark, field: fieldMark });
        return;
    }
    // under the field, for a click as for the down arrow (D-611).
    var rect = target.getBoundingClientRect();
    if(bridge) bridge.activated(documentMark, fieldMark, rect.left, rect.bottom, heard);
}
// the numbers of the offered names which begin with what is in the field,
// in any case; all of them for a password field.
function report(){
    if(!offered || offered.mark !== fieldMark || !field) return;
    var typed = isPassword(field) ? '' : String(field.value || '').toLowerCase();
    var numbers = [];
    offered.names.forEach(function(name, i){
        // an account with no password has nothing for a password field.
        if(isPassword(field) && offered.bare[i]) return;
        if(String(name).toLowerCase().indexOf(typed) === 0) numbers.push(i);
    });
    if(inMain){
        if(bridge) bridge.filtered(documentMark, fieldMark, numbers);
        return;
    }
    // the latest only: what was typed before it is behind it.
    box = box.filter(function(e){ return !(e.kind === 'filtered' && e.field === fieldMark); });
    tell({ kind: 'filtered', document: documentMark, field: fieldMark, numbers: numbers });
}
// the field lost the focus, or the page moved under the list (D-611).
function unlist(){
    if(!listed) return;
    listed = false;
    var closing = fieldMark;
    // and nothing is filled into it any more (R-463 中1), nor narrowed.
    fieldMark = null;
    offered = null;
    if(inMain){ if(bridge) bridge.closed(documentMark, closing); }
    else tell({ kind: 'closed', document: documentMark, field: closing });
}

document.addEventListener('click', function(e){
    if(!e.isTrusted || !e.target || !e.target.closest) return;
    var button = e.target.closest('button, input[type="submit"], input[type="image"], ' +
                                  'input[type="button"], [role="button"]');
    if(!button) return;
    var submits = (button.tagName === 'BUTTON' && button.type === 'submit') ||
        (button.tagName === 'INPUT' && (button.type === 'submit' || button.type === 'image'));
    if(submits) capture(scopeOf(button));
    // a 'button' which only does something on the page -- show the
    // password, clear the field -- is not a submission; one which sends the
    // form by script is. Which it was is seen afterwards (D-609).
    else hold(scopeOf(button));
}, true);

document.addEventListener('keydown', function(e){
    if(!e.isTrusted || e.isComposing) return;
    var target = e.target;
    if(!isPassword(target) && !isUsername(target)) return;
    // Enter in a form sends it: the engine does that. Outside one it does
    // only what the page's script makes of it -- maybe nothing -- and is
    // held as a click on a 'type="button"' is, asked about once the field
    // goes or the page moves on (D-614).
    if(e.key === 'Enter'){
        if(target.form) capture(scopeOf(target));
        else hold(scopeOf(target));
    }
    else if(e.key === 'ArrowDown' && !e.altKey && !e.ctrlKey && !e.metaKey && !e.shiftKey)
        activate(target);
}, true);

document.addEventListener('mousedown', function(e){
    if(!e.isTrusted || e.button !== 0) return;
    var target = e.target;
    if(isPassword(target) || isUsername(target)) activate(target);
}, true);

document.addEventListener('blur', function(e){
    if(e.isTrusted && e.target === field) unlist();
}, true);
document.addEventListener('scroll', function(e){
    if(e.isTrusted) unlist();
}, true);
// typing narrows the list there is; in a username field without one --
// closed with Escape, say -- it opens one, which shows if a name matches.
document.addEventListener('input', function(e){
    if(!e.isTrusted) return;
    if(listed && e.target === field) report();
    else if(!e.isComposing && isUsername(e.target)) activate(e.target);
}, true);

function setValue(element, value){
    if(!element) return;
    var setter = Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value').set;
    setter.call(element, value);
    element.dispatchEvent(new Event('input', { bubbles: true }));
    element.dispatchEvent(new Event('change', { bubbles: true }));
}

window.__vanillaPasswords = {
    take: function(document_, ticket){
        if(document_ !== documentMark || !pending || pending.ticket !== ticket) return null;
        var credentials = pending;
        pending = null;
        return { origin: credentials.origin,
                 username: credentials.username,
                 password: credentials.password };
    },
    fill: function(document_, mark_, origin, username, password){
        if(document_ !== documentMark || !fieldMark || mark_ !== fieldMark) return false;
        if(self.origin !== origin || !field || !field.isConnected) return false;
        fieldMark = null;
        // filled: the list is done with, and typing opens another (R-466 中3).
        listed = false;
        offered = null;
        // an account saved without a password: its name, and nothing else.
        if(password === ''){
            if(!isUsername(field) || username === '') return false;
            setValue(field, username);
            return true;
        }
        var scope = scopeOf(field);
        var passwordField = isPassword(field) ? field
            : scope.querySelector('input[type="password"]');
        // no password field here -- the name step of a login in two: the
        // name alone (D-615).
        if(!passwordField){
            if(!isUsername(field)) return false;
            setValue(field, username);
            return true;
        }
        var usernameField = isUsername(field) ? field : usernameFor(scope, passwordField);
        if(usernameField && username !== '') setValue(usernameField, username);
        setValue(passwordField, password);
        return true;
    },
    // the names for the list of the field, to narrow (D-611 追記 7).
    // Only to the document of the origin they are kept for: a sandboxed
    // frame of that address has its own, opaque one (R-466 高).
    offer: function(document_, mark_, origin, names, bare){
        if(document_ !== documentMark || !fieldMark || mark_ !== fieldMark || !listed) return false;
        if(self.origin !== origin) return false;
        offered = { mark: mark_, names: Array.prototype.slice.call(names),
                    bare: Array.prototype.slice.call(bare || []) };
        report();
        return true;
    },
    // the host closed this document's list (R-463 中2): the field's, if it
    // is still the one, is no more.
    closed: function(document_, mark_, generation){
        if(document_ !== documentMark) return;
        heard = generation;
        if(mark_ !== fieldMark) return;
        listed = false;
        offered = null;
        fieldMark = null;
    },
    // a frame's box, once, without what is older than ten seconds (D-610).
    drain: function(){
        var now = Date.now();
        var out = box.filter(function(e){ return now - e.at < 10000; });
        box = [];
        return out.map(function(e){
            var copy = {};
            for(var k in e) if(k !== 'at') copy[k] = e[k];
            return copy;
        });
    }
};
})();
)JS";

    QString QWebChannelJsCode(){
        QFile file(QStringLiteral(":/qtwebchannel/qwebchannel.js"));
        if(!file.open(QFile::ReadOnly)) return QString();
        return QString::fromLatin1(file.readAll());
    }

    QString Arguments(const QStringList &values){
        QJsonArray array;
        foreach(const QString &value, values) array.append(value);
        const QString json = QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
        return json.mid(1, json.length() - 2);
    }
}

WebEnginePasswords::WebEnginePasswords(QObject *owner, const Host &host)
    : QObject(owner)
    , m_Host(host)
    , m_Bridge(new WebEnginePasswordBridge(this))
    , m_HoldTimer(this)
{
    m_HoldTimer.setSingleShot(true);
    m_HoldTimer.setInterval(10000);
    connect(&m_HoldTimer, &QTimer::timeout, this, [this](){ m_Hold.Clear();});
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *){
        if(!m_MenuDocument.isEmpty() && !InView()) ForgetMenu();
    });
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state){
        if(state != Qt::ApplicationActive) ForgetMenu();
    });
}

WebEnginePasswords::~WebEnginePasswords(){
    CloseList(false);
}

QObject *WebEnginePasswords::Bridge() const {
    return m_Bridge.data();
}

QString WebEnginePasswords::ScriptSource(){
    static const QString source = QWebChannelJsCode() + QStringLiteral("\n") +
        QString::fromUtf8(PasswordJsCode);
    return source;
}

QString WebEnginePasswords::FrameScriptSource(){
    static const QString source = QStringLiteral("if(window.top !== window){\n") +
        QString::fromUtf8(PasswordJsCode) + QStringLiteral("\n}\n");
    return source;
}

static QWebEngineScript FrameScript(){
    QWebEngineScript frames;
    frames.setName(QStringLiteral("vanilla-passwords-frames"));
    frames.setInjectionPoint(QWebEngineScript::DocumentReady);
    frames.setWorldId(QWebEngineScript::ApplicationWorld);
    frames.setRunsOnSubFrames(true);
    frames.setSourceCode(WebEnginePasswords::FrameScriptSource());
    return frames;
}

void WebEnginePasswords::InstallScript(QWebEngineProfile *profile){
    QWebEngineScript script;
    script.setName(QStringLiteral("vanilla-passwords"));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    script.setRunsOnSubFrames(false);
    script.setSourceCode(ScriptSource());
    profile->scripts()->insert(script);

    profile->scripts()->insert(FrameScript());
}

#ifdef VANILLA_QUICK_PASSWORD_FRAMES
void WebEnginePasswords::InstallFrameScript(QQuickWebEngineProfile *profile){
    profile->userScripts()->insert(FrameScript());
}
#endif

bool WebEnginePasswords::CurrentKey(Key *key) const {
    if(!m_Host.current) return false;
    return KeyAt(m_Host.current().url, key);
}

bool WebEnginePasswords::KeyAt(const QUrl &url, Key *key) const {
    if(!View::SavePasswords() || !m_Host.current) return false;
    const Snapshot now = m_Host.current();
    if(!now.alive || now.space.isEmpty() || !now.profile) return false;
    key->space = now.space;
    key->origin = PasswordPolicy::Origin(url);
    key->scriptOrigin = PasswordPolicy::ScriptOrigin(url);
    key->profile = now.profile;
    key->isPrivate = now.isPrivate;
    return !key->origin.isEmpty();
}

void WebEnginePasswords::Submitted(const QString &documentMark, const QString &ticket){
    m_Hold.Clear();
    m_HoldTimer.stop();
    Key key;
    if(!CurrentKey(&key)) return;
    if(key.isPrivate) return;
    if(!PasswordStore::Instance()) return;

    const QString code = QStringLiteral("window.__vanillaPasswords && "
                                        "window.__vanillaPasswords.take(%1)")
        .arg(Arguments(QStringList() << documentMark << ticket));

    QPointer<WebEnginePasswords> self(this);
    m_Host.runInApplicationWorld(code, [self, key](const QVariant &result){
        if(!self) return;
        const QVariantMap map = result.toMap();
        const QString password = map.value(QStringLiteral("password")).toString();
        const QString username = map.value(QStringLiteral("username")).toString();
        if(password.isEmpty() && username.isEmpty()) return;

        Key now;
        if(!self->CurrentKey(&now) || !Same(now, key) || now.isPrivate ||
           map.value(QStringLiteral("origin")).toString() != key.scriptOrigin) return;

        self->Offer(key, username, password);
    });
}

void WebEnginePasswords::Held(const QString &documentMark, const QString &ticket){
    Key key;
    if(!CurrentKey(&key) || key.isPrivate || !PasswordStore::Instance() || !m_Host.current) return;
    m_Hold.Begin(documentMark, m_Host.current().url, ticket);
    m_HeldKey = key;
    m_HeldFrame.reset();
    m_HoldTimer.start();

    const QString code = QStringLiteral("window.__vanillaPasswords && "
                                        "window.__vanillaPasswords.take(%1)")
        .arg(Arguments(QStringList() << documentMark << ticket));
    QPointer<WebEnginePasswords> self(this);
    m_Host.runInApplicationWorld(code, [self, key, documentMark, ticket](const QVariant &result){
        if(!self || !self->m_Host.current) return;
        const QVariantMap map = result.toMap();
        const QString password = map.value(QStringLiteral("password")).toString();
        if(password.isEmpty() && map.value(QStringLiteral("username")).toString().isEmpty()) return;
        const Snapshot now = self->m_Host.current();
        if(!now.alive || now.profile != key.profile || now.isPrivate ||
           map.value(QStringLiteral("origin")).toString() != key.scriptOrigin) return;
        PasswordHold::Credentials credentials;
        credentials.username = map.value(QStringLiteral("username")).toString();
        credentials.password = password;
        if(self->m_Hold.Taken(documentMark, credentials, ticket)) self->OfferHeld();
    });
}

void WebEnginePasswords::Confirmed(const QString &documentMark){
    if(m_Hold.Confirm(documentMark)) OfferHeld();
}

void WebEnginePasswords::Released(const QString &documentMark){
    m_Hold.Release(documentMark);
    if(m_Hold.IsEmpty()) m_HoldTimer.stop();
}

void WebEnginePasswords::Navigated(const QUrl &url){
    Q_UNUSED(url)
    ForgetMenu();
    if(m_HeldFrame){
        CheckHeldFrame();
        return;
    }
    if(!m_Host.current) return;
    const Snapshot now = m_Host.current();
    if(now.alive && m_Hold.Navigated(now.url)) OfferHeld();
}

void WebEnginePasswords::Rang(bool loaded){
    if(loaded){
        CheckHeldFrame();
        return;
    }
    if(!m_Drain.Knock() || m_DrainQueued) return;
    m_DrainQueued = true;
    QTimer::singleShot(0, this, [this](){
        m_DrainQueued = false;
        Drain();
    });
}

void WebEnginePasswords::Drain(){
    if(!m_Host.mainFrame || !m_Host.current) return;
    const QWebEngineFrame main = m_Host.mainFrame();
    if(!main.isValid()) return;
    QList<QWebEngineFrame> frames;
    QList<QWebEngineFrame> walk = main.children();
    while(!walk.isEmpty()){
        const QWebEngineFrame frame = walk.takeFirst();
        frames << frame;
        walk << frame.children();
    }
    if(frames.isEmpty()) return;

    const quint64 generation = m_Drain.Begin();
    auto left = std::make_shared<int>(frames.size());
    const QUrl mainUrl = m_Host.current().url;
    const quint64 list = m_ListGeneration;
    QPointer<WebEnginePasswords> self(this);
    QTimer::singleShot(3000, this, [this, generation](){
        if(m_Drain.End(generation)) Rang(false);
    });
    foreach(QWebEngineFrame frame, frames){
        const QUrl frameUrl = frame.url();
        Key key;
        const bool keeps = KeyAt(frameUrl, &key);
        frame.runJavaScript(QStringLiteral("window.__vanillaPasswords && "
                                           "window.__vanillaPasswords.drain()"),
                            QWebEngineScript::ApplicationWorld,
                            [self, frame, key, keeps, frameUrl, mainUrl, left, generation, list](const QVariant &result){
            if(!self) return;
            const bool listStale = self->m_ListGeneration != list;
            if(keeps){
                for(const QVariant &entry : result.toList())
                    self->FromFrame(frame, key, frameUrl, mainUrl, entry.toMap(), listStale);
            }
            if(--*left > 0) return;
            if(self->m_Drain.End(generation)) self->Rang(false);
        });
    }
}

void WebEnginePasswords::FromFrame(const QWebEngineFrame &frame, const Key &key,
                                   const QUrl &frameUrl, const QUrl &mainUrl,
                                   const QVariantMap &entry, bool listStale){
    const QString kind = entry.value(QStringLiteral("kind")).toString();
    const QString document = entry.value(QStringLiteral("document")).toString();
    if(document.isEmpty()) return;

    if(kind == QLatin1String("confirmed")){ Confirmed(document); return;}
    if(kind == QLatin1String("released")){ Released(document); return;}
    if(kind == QLatin1String("activated")){
        const QString field = entry.value(QStringLiteral("field")).toString();
        if(!listStale){
            ShowMenu(key, document, field, -1, -1, frame);
            return;
        }
        QWebEngineFrame target = frame;
        if(target.isValid())
            target.runJavaScript(QStringLiteral("window.__vanillaPasswords && "
                                                "window.__vanillaPasswords.closed(%1, %2)")
                                 .arg(Arguments(QStringList() << document << field))
                                 .arg(m_ListGeneration),
                                 QWebEngineScript::ApplicationWorld, [](const QVariant&){});
        return;
    }
    if(kind == QLatin1String("closed")){
        if(!listStale) Closed(document, entry.value(QStringLiteral("field")).toString());
        return;
    }
    if(kind == QLatin1String("filtered")){
        if(!listStale)
            Filtered(document, entry.value(QStringLiteral("field")).toString(),
                     entry.value(QStringLiteral("numbers")).toList());
        return;
    }
    if(kind != QLatin1String("submitted") && kind != QLatin1String("held")) return;

    const QString password = entry.value(QStringLiteral("password")).toString();
    if((password.isEmpty() && entry.value(QStringLiteral("username")).toString().isEmpty()) ||
       entry.value(QStringLiteral("origin")).toString() != key.scriptOrigin) return;
    if(key.isPrivate || !PasswordStore::Instance() || !m_Host.current) return;
    const Snapshot now = m_Host.current();
    if(!now.alive || now.profile != key.profile || now.isPrivate) return;
    const QString username = entry.value(QStringLiteral("username")).toString();

    if(kind == QLatin1String("submitted")){
        m_Hold.Clear();
        m_HoldTimer.stop();
        m_HeldFrame.reset();
        Offer(key, username, password);
        return;
    }
    m_Hold.Begin(document, frameUrl);
    m_HeldKey = key;
    m_HeldFrame = frame;
    m_HeldDocument = document;
    m_HeldMainUrl = mainUrl;
    m_HoldTimer.start();
    PasswordHold::Credentials credentials;
    credentials.username = username;
    credentials.password = password;
    m_Hold.Taken(document, credentials);
    CheckHeldFrame();
}

void WebEnginePasswords::CheckHeldFrame(){
    if(!m_HeldFrame || m_Hold.IsEmpty() || !m_Host.current) return;
    const Snapshot now = m_Host.current();
    if(!now.alive) return;
    if(now.url.adjusted(QUrl::RemoveFragment) != m_HeldMainUrl.adjusted(QUrl::RemoveFragment)){
        if(m_Hold.Confirm(m_HeldDocument)) OfferHeld();
        return;
    }
    if(m_HeldFrame->isValid() && m_Hold.Navigated(m_HeldFrame->url())) OfferHeld();
}

void WebEnginePasswords::OfferHeld(){
    m_HoldTimer.stop();
    m_HeldFrame.reset();
    const PasswordHold::Credentials credentials = m_Hold.Take();
    if((credentials.password.isEmpty() && credentials.username.isEmpty()) ||
       !View::SavePasswords()) return;
    Offer(m_HeldKey, credentials.username, credentials.password);
}

QString WebEnginePasswords::PageScript(){
    return QString::fromUtf8(PasswordJsCode);
}

namespace {
    class SaveFields : public QWidget {
    public:
        SaveFields(const QStringList &saved, const QString &username, const QString &password,
                   std::function<bool(const QString&)> isSaved,
                   std::function<bool(const QString&)> hasPassword)
            : m_IsSaved(isSaved)
            , m_HasPassword(hasPassword)
        {
            QVBoxLayout *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            QHBoxLayout *names = new QHBoxLayout();
            m_Username = new QLineEdit(username, this);
            m_Username->setFixedHeight(FieldHeight());
            m_Username->setPlaceholderText(WebEnginePasswords::tr("Username"));
            EditMenu::Install(m_Username);
            names->addWidget(m_Username, 1);
            QStringList listed;
            foreach(const QString &name, saved)
                if(!name.isEmpty()) listed << name;
            if(!listed.isEmpty()){
                QToolButton *pick = new QToolButton(this);
                pick->setFixedHeight(FieldHeight());
                pick->setFixedWidth(ButtonWidth());
                pick->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; width: 0px; }"));
                pick->setText(QStringLiteral("\u25BE"));
                pick->setPopupMode(QToolButton::InstantPopup);
                QMenu *menu = new QMenu(pick);
                for(const QString &name : std::as_const(listed)){
                    QObject::connect(menu->addAction(name), &QAction::triggered, this, [this, name](){
                        m_Username->setText(name);
                    });
                }
                pick->setMenu(menu);
                names->addWidget(pick);
            }
            layout->addLayout(names);

            QHBoxLayout *row = new QHBoxLayout();
            m_Password = new QLineEdit(password, this);
            m_Password->setFixedHeight(FieldHeight());
            m_Password->setEchoMode(QLineEdit::Password);
            m_Password->setPlaceholderText(WebEnginePasswords::tr("No password"));
            EditMenu::Install(m_Password);
            row->addWidget(m_Password, 1);
            QToolButton *reveal = new QToolButton(this);
            reveal->setFixedHeight(FieldHeight());
            reveal->setFixedWidth(ButtonWidth());
            reveal->setCheckable(true);
            reveal->setText(WebEnginePasswords::tr("Show"));
            QObject::connect(reveal, &QToolButton::toggled, this, [this, reveal](bool on){
                m_Password->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
                reveal->setText(on ? WebEnginePasswords::tr("Hide") : WebEnginePasswords::tr("Show"));
            });
            row->addWidget(reveal);
            layout->addLayout(row);

            m_Note = new QLabel(this);
            m_Note->setWordWrap(true);
            m_Note->setMinimumHeight(m_Note->fontMetrics().height());
            layout->addWidget(m_Note);
            QObject::connect(m_Username, &QLineEdit::textChanged, this, [this](){ Refresh();});
            QObject::connect(m_Password, &QLineEdit::textChanged, this, [this](){ Refresh();});
            Refresh();
        }

        QString Username() const { return m_Username->text();}
        int FieldHeight() const { return fontMetrics().height() * 2;}
        int ButtonWidth() const {
            const QFontMetrics metrics = fontMetrics();
            return qMax(metrics.horizontalAdvance(WebEnginePasswords::tr("Show")),
                        metrics.horizontalAdvance(WebEnginePasswords::tr("Hide"))) + metrics.height() * 2;
        }
        QString Password() const { return m_Password->text();}
        bool ShownSaved() const { return m_ShownSaved;}
        bool ShownWithPassword() const { return m_ShownWithPassword;}

    private:
        void Refresh(){
            m_ShownSaved = m_IsSaved(Username());
            m_ShownWithPassword = m_ShownSaved && m_HasPassword(Username());
            const bool empty = Password().isEmpty();
            QString note;
            if(m_ShownSaved && empty)
                note = WebEnginePasswords::tr("This account is saved already. Nothing changes.");
            else if(m_ShownSaved)
                note = m_ShownWithPassword
                    ? WebEnginePasswords::tr("This replaces the saved password.")
                    : WebEnginePasswords::tr("The password is added to this account.");
            else if(empty)
                note = WebEnginePasswords::tr("The account is saved without a password.");
            m_Note->setText(note);
        }

        std::function<bool(const QString&)> m_IsSaved;
        std::function<bool(const QString&)> m_HasPassword;
        QLineEdit *m_Username;
        QLineEdit *m_Password;
        QLabel *m_Note;
        bool m_ShownSaved = false;
        bool m_ShownWithPassword = false;
    };
}

void WebEnginePasswords::Offer(const Key &key, const QString &username, const QString &password){
    PasswordStore *store = PasswordStore::Instance();
    if(!store || store->IsNever(key.space, key.origin)) return;

    if(password.isEmpty()){
        if(!username.isEmpty() && !Saved(key, username)) Ask(key, username, password);
        return;
    }
    QPointer<WebEnginePasswords> self(this);
    if(m_Question && m_QuestionNameStep && Same(m_QuestionKey, key)){
        if(username.isEmpty()){
            Ask(key, username, password);
            return;
        }
        m_Question->Discard();
        m_Question = nullptr;
        m_QuestionFields.clear();
    }
    if(username.isEmpty()){
        QStringList names = store->Usernames(key.space, key.origin);
        names.removeAll(QString());
        auto next = std::make_shared<std::function<void(int)>>();
        std::weak_ptr<std::function<void(int)>> weak = next;
        *next = [self, key, password, names, weak](int i){
            if(!self) return;
            PasswordStore *store = PasswordStore::Instance();
            const std::shared_ptr<std::function<void(int)>> again = weak.lock();
            if(!store || !again) return;
            if(i >= names.size()){
                self->Ask(key, QString(), password);
                return;
            }
            store->Read(key.space, key.origin, names.at(i), false,
                        [self, password, again, i](bool ok, const QString &saved){
                if(!self) return;
                if(ok && saved == password) return;
                (*again)(i + 1);
            });
        };
        (*next)(0);
        return;
    }
    if(!store->Has(key.space, key.origin, username)){
        Ask(key, username, password);
        return;
    }
    store->Read(key.space, key.origin, username, false,
                [self, key, username, password](bool ok, const QString &saved){
        if(!self || (ok && saved == password)) return;
        self->Ask(key, username, password);
    });
}

bool WebEnginePasswords::Saved(const Key &key, const QString &username) const {
    PasswordStore *store = PasswordStore::Instance();
    if(store && store->Has(key.space, key.origin, username)) return true;
    foreach(const PendingName &pending, m_PendingNames)
        if(pending.space == key.space && pending.origin == key.origin &&
           pending.profile == key.profile && pending.name == username) return true;
    return false;
}

void WebEnginePasswords::Ask(const Key &key, const QString &username, const QString &password){
    PasswordStore *store = PasswordStore::Instance();
    if(!store || !m_Host.current) return;
    QString name = username;
    if(m_Question && m_QuestionNameStep && !password.isEmpty() && Same(m_QuestionKey, key)){
        if(name.isEmpty() && m_QuestionFields)
            name = static_cast<SaveFields*>(m_QuestionFields.data())->Username();
        m_Question->Discard();
        m_Question = nullptr;
    }
    if(m_Question) return;
    const Snapshot before = m_Host.current();
    if(!before.alive || before.profile != key.profile || before.isPrivate) return;
    if(store->IsNever(key.space, key.origin)) return;

    QStringList names = store->Usernames(key.space, key.origin);
    foreach(const PendingName &pending, m_PendingNames)
        if(pending.space == key.space && pending.origin == key.origin &&
           pending.profile == key.profile && !names.contains(pending.name)) names << pending.name;
    QPointer<WebEnginePasswords> self(this);
    SaveFields *fields = new SaveFields(names, name, password, [self, key](const QString &name){
        return self && self->Saved(key, name);
    }, [key](const QString &name){
        PasswordStore *store = PasswordStore::Instance();
        return store && store->HasPassword(key.space, key.origin, name);
    });
    QPointer<SaveFields> held(fields);

    auto answered = [self, store, key, held](bool yes){
        if(!self) return;
        const bool never = !yes && self->m_Question &&
            self->m_Question->ClickedButton() == Dialog::Never;
        self->m_Question = nullptr;
        self->m_QuestionFields.clear();
        if(never){
            if(!self->m_Host.current) return;
            const Snapshot now = self->m_Host.current();
            if(!now.alive || now.profile != key.profile || now.isPrivate) return;
            if(!store->SetNever(key.space, key.origin, true) && self->m_Host.message)
                self->m_Host.message(tr("The site could not be remembered."));
            return;
        }
        if(!yes || !held) return;
        const QString name = held->Username();
        const QString secret = held->Password();
        const bool shownSaved = held->ShownSaved();
        const bool shownWithPassword = held->ShownWithPassword();

        if(!View::SavePasswords() || store->IsNever(key.space, key.origin)) return;
        if(!self->m_Host.current) return;
        const Snapshot now = self->m_Host.current();
        if(!now.alive || now.profile != key.profile || now.isPrivate) return;

        PasswordStore *index = PasswordStore::Instance();
        if(self->Saved(key, name) != shownSaved ||
           (shownSaved && index && index->HasPassword(key.space, key.origin, name) != shownWithPassword)){
            self->Ask(key, name, secret);
            return;
        }
        if(shownSaved && secret.isEmpty()) return;
        const quint64 serial = ++self->m_PendingSerial;
        if(secret.isEmpty())
            self->m_PendingNames << PendingName{key.space, key.origin, key.profile, name, serial};
        store->SaveExpecting(key.space, key.origin, name, secret,
                             !shownSaved ? PasswordStore::Absent
                             : shownWithPassword ? PasswordStore::Present : PasswordStore::Bare,
                             [self, key, name, secret, serial](PasswordStore::SaveResult result){
            if(!self) return;
            for(int i = self->m_PendingNames.size() - 1; i >= 0; i--)
                if(self->m_PendingNames.at(i).serial == serial) self->m_PendingNames.removeAt(i);
            if(result == PasswordStore::Changed){
                if(!secret.isEmpty()) self->Ask(key, name, secret);
                return;
            }
            if(result == PasswordStore::SaveFailed && self->m_Host.message)
                self->m_Host.message(tr("The password could not be saved."));
        });
    };

    m_Question = ModelessDialog::Question(tr("Save password"),
                                          tr("Save the password for %1?").arg(key.scriptOrigin),
                                          answered, this);
    if(!m_Question){
        delete fields;
        return;
    }
    m_Question->SetContent(fields);
    m_QuestionKey = key;
    m_QuestionNameStep = password.isEmpty();
    m_QuestionFields = fields;
    m_Question->SetButtons(Dialog::Yes | Dialog::No | Dialog::Never);
}

void WebEnginePasswords::Activated(const QString &documentMark, const QString &fieldMark,
                                   double left, double bottom, double heard){
    if(documentMark == m_ClosedDocument && heard != double(m_ClosedGeneration)){
        TellClosed(fieldMark);
        return;
    }
    Key key;
    if(!CurrentKey(&key)) return;
    ShowMenu(key, documentMark, fieldMark, left, bottom, std::nullopt);
}

void WebEnginePasswords::ShowMenu(const Key &key, const QString &documentMark,
                                  const QString &fieldMark, double left, double bottom,
                                  const std::optional<QWebEngineFrame> &frame){
    if(key.origin.isEmpty() || fieldMark.isEmpty()) return;
    PasswordStore *store = PasswordStore::Instance();
    if(!store) return;
    const QStringList usernames = store->Usernames(key.space, key.origin);
    if(usernames.isEmpty()) return;
    QWidget *view = m_Host.widget ? m_Host.widget() : nullptr;
    if(!view || !view->isVisible()) return;

    CloseList(false, true);
    m_MenuDocument = documentMark;
    m_MenuField = fieldMark;
    m_MenuKey = key;
    m_MenuFrame = frame;
    m_ListUsernames = usernames;
    m_ListView = view;
    m_ListWindow = view->window();

    PasswordSuggestions *list = new PasswordSuggestions(view->window(), tr("Manage passwords"));
    connect(list, &PasswordSuggestions::chosen, this, &WebEnginePasswords::Take);
    m_List = list;
    qApp->installEventFilter(this);

    m_ListAnchor = QCursor::pos();
    if(left >= 0 && bottom >= 0 && m_Host.mapToGlobal && m_Host.current){
        const qreal zoom = m_Host.current().zoom;
        m_ListAnchor = m_Host.mapToGlobal(QPoint(qRound(left * zoom), qRound(bottom * zoom)));
    }

    QJsonArray names;
    QJsonArray bare;
    for(const QString &username : usernames){
        names.append(username);
        bare.append(!store->HasPassword(key.space, key.origin, username));
    }
    const QString code = QStringLiteral("window.__vanillaPasswords && "
                                        "window.__vanillaPasswords.offer(%1, %2, %3)")
        .arg(Arguments(QStringList() << documentMark << fieldMark << key.scriptOrigin))
        .arg(QString::fromUtf8(QJsonDocument(names).toJson(QJsonDocument::Compact)),
             QString::fromUtf8(QJsonDocument(bare).toJson(QJsonDocument::Compact)));
    if(frame){
        QWebEngineFrame target = *frame;
        target.runJavaScript(code, QWebEngineScript::ApplicationWorld, [](const QVariant&){});
    } else {
        m_Host.runInApplicationWorld(code, [](const QVariant&){});
    }
}

void WebEnginePasswords::Filtered(const QString &documentMark, const QString &fieldMark,
                                  const QVariantList &numbers){
    if(m_MenuDocument.isEmpty() || documentMark != m_MenuDocument ||
       fieldMark != m_MenuField || m_ListReading || !m_List) return;
    QList<int> shown;
    QStringList labels;
    foreach(const QVariant &number, numbers){
        bool ok = false;
        const int index = number.toInt(&ok);
        if(!ok || index < 0 || index >= m_ListUsernames.size() || shown.contains(index)) continue;
        shown << index;
        const QString &username = m_ListUsernames.at(index);
        labels << (username.isEmpty() ? tr("(no username)") : username);
    }
    m_ListShown = shown;
    if(shown.isEmpty()){
        m_List->hide();
        return;
    }
    m_List->SetAccounts(labels);
    m_List->ShowAt(m_ListAnchor);
}

void WebEnginePasswords::Closed(const QString &documentMark, const QString &fieldMark){
    if(m_MenuDocument.isEmpty() || documentMark != m_MenuDocument || fieldMark != m_MenuField) return;
    ForgetMenu();
}

bool WebEnginePasswords::InView() const {
    if(!m_ListView || !m_ListView->isVisible()) return false;
    QWidget *focus = QApplication::focusWidget();
    return focus && (focus == m_ListView || m_ListView->isAncestorOf(focus));
}

bool WebEnginePasswords::eventFilter(QObject *watched, QEvent *ev){
    if(m_MenuDocument.isEmpty()) return false;
    switch(ev->type()){
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress: {
        if(!InView()) return false;
        QKeyEvent *key = static_cast<QKeyEvent*>(ev);
        const SuggestionKeys::Action action =
            SuggestionKeys::Decide(true, m_List && m_List->isVisible(),
                                   m_List && m_List->Chosen() >= 0,
                                   SuggestionKeys::Classify(key->key(), key->modifiers()));
        if(ev->type() == QEvent::ShortcutOverride){
            if(!SuggestionKeys::Eaten(action)) return false;
            ev->accept();
            return true;
        }
        switch(action){
        case SuggestionKeys::Edit:
            m_EditGeneration++;
            if(m_List) m_List->ClearChoice();
            return false;
        case SuggestionKeys::Move:
            m_List->MoveChoice(SuggestionKeys::Classify(key->key(), key->modifiers())
                               == SuggestionKeys::Up ? -1 : 1);
            return true;
        case SuggestionKeys::Choose:
            Take(m_List->Chosen());
            return true;
        case SuggestionKeys::Close:
            ForgetMenu();
            return true;
        case SuggestionKeys::CloseAndPass:
            ForgetMenu();
            return false;
        case SuggestionKeys::Pass:
            return false;
        }
        return false;
    }
    case QEvent::InputMethod:
        if(InView() && !static_cast<QInputMethodEvent*>(ev)->preeditString().isEmpty())
            ForgetMenu();
        return false;
    case QEvent::Hide:
        if(watched == m_ListView || watched == m_ListWindow) ForgetMenu();
        return false;
    case QEvent::Move:
    case QEvent::Resize:
    case QEvent::WindowStateChange:
        if(watched == m_ListWindow) ForgetMenu();
        return false;
    default:
        return false;
    }
}

void WebEnginePasswords::Take(int row){
    if(m_MenuDocument.isEmpty() || m_ListReading || !m_List) return;
    if(row == m_List->ManageRow()){
        ForgetMenu();
        if(m_Host.managePasswords) m_Host.managePasswords();
        return;
    }
    if(row >= 0 && row < m_ListShown.size() && row < m_List->ManageRow()){
        Fill(m_ListUsernames.at(m_ListShown.at(row)));
        return;
    }
}

void WebEnginePasswords::Fill(const QString &username){
    PasswordStore *store = PasswordStore::Instance();
    if(!store || m_MenuDocument.isEmpty()) return;

    const QString document = m_MenuDocument;
    const QString field = m_MenuField;
    const Key key = m_MenuKey;
    const std::optional<QWebEngineFrame> frame = m_MenuFrame;
    const quint64 list = m_ListGeneration;
    const quint64 edit = m_EditGeneration;
    m_ListReading = true;
    if(m_List) m_List->close();

    QPointer<WebEnginePasswords> self(this);
    store->Read(key.space, key.origin, username, false,
                [self, document, field, key, frame, username, list, edit](bool ok, const QString &password){
        if(!self) return;
        if(self->m_ListGeneration != list ||
           self->m_MenuDocument != document || self->m_MenuField != field) return;
        Key now;
        bool still = ok && self->m_EditGeneration == edit;
        if(still && frame)
            still = frame->isValid() && self->KeyAt(frame->url(), &now) && Same(now, key);
        else if(still)
            still = self->CurrentKey(&now) && Same(now, key);
        if(!still){
            self->ForgetMenu();
            return;
        }

        const QString code = QStringLiteral("window.__vanillaPasswords && "
                                            "window.__vanillaPasswords.fill(%1)")
            .arg(Arguments(QStringList() << document << field << now.scriptOrigin
                                         << username << password));
        if(frame){
            QWebEngineFrame target = *frame;
            target.runJavaScript(code, QWebEngineScript::ApplicationWorld,
                                 [](const QVariant&){});
        } else {
            self->m_Host.runInApplicationWorld(code, [](const QVariant&){});
        }
        self->ForgetMenu();
    });
}

void WebEnginePasswords::Forget(){
    ForgetMenu();
    m_Drain.Reset();
}

void WebEnginePasswords::ForgetMenu(){
    CloseList(true);
}

void WebEnginePasswords::CloseList(bool tell, bool replacing){
    const bool open = !m_MenuDocument.isEmpty() || m_List;
    const QString document = m_MenuDocument;
    const QString field = m_MenuField;
    const std::optional<QWebEngineFrame> frame = m_MenuFrame;
    const bool inMain = !frame;
    if(!replacing) m_ListGeneration++;
    m_MenuDocument.clear();
    m_MenuField.clear();
    m_MenuKey = Key();
    m_MenuFrame.reset();
    m_ListUsernames.clear();
    m_ListShown.clear();
    m_ListReading = false;
    m_ListView.clear();
    m_ListWindow.clear();
    if(m_List) m_List->close();
    m_List.clear();
    if(!open) return;
    if(qApp) qApp->removeEventFilter(this);
    if(!tell || document.isEmpty()) return;
    if(inMain){
        m_ClosedDocument = document;
        m_ClosedGeneration = m_ListGeneration;
        TellClosed(field);
        return;
    }
    QWebEngineFrame target = *frame;
    if(target.isValid())
        target.runJavaScript(QStringLiteral("window.__vanillaPasswords && "
                                            "window.__vanillaPasswords.closed(%1, %2)")
                             .arg(Arguments(QStringList() << document << field))
                             .arg(m_ListGeneration),
                             QWebEngineScript::ApplicationWorld, [](const QVariant&){});
}

void WebEnginePasswords::TellClosed(const QString &field){
    if(m_ClosedDocument.isEmpty() || !m_Host.runInApplicationWorld) return;
    m_Host.runInApplicationWorld(QStringLiteral("window.__vanillaPasswords && "
                                                "window.__vanillaPasswords.closed(%1, %2)")
                                 .arg(Arguments(QStringList() << m_ClosedDocument << field))
                                 .arg(m_ClosedGeneration),
                                 [](const QVariant&){});
}

#endif
