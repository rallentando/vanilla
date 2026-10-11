#include "switch.hpp"

#include "webenginepasswords.hpp"

#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)

#include "passwordpolicy.hpp"
#include "passwordstore.hpp"
#include "networkcontroller.hpp"
#include "dialog.hpp"
#include "view.hpp"

#include <QCursor>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#ifdef USE_WEBCHANNEL
#  error "USE_WEBCHANNEL and VANILLA_PASSWORDS cannot be on together."
#endif

namespace {

    const char *PasswordJsCode = R"JS(
(function(){
'use strict';
if(window.__vanillaPasswords) return;
if(typeof qt === 'undefined' || !qt.webChannelTransport) return;

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
var bridge = null;
var lastCapture = null;

new QWebChannel(qt.webChannelTransport, function(channel){
    bridge = channel.objects.vanillaPasswords;
});

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

function capture(scope){
    var passwords = filledPasswords(scope);
    if(passwords.length !== 1) return;
    var username = usernameFor(scope, passwords[0]);
    var credentials = {
        origin: location.origin,
        username: username ? username.value : '',
        password: passwords[0].value
    };
    // Enter in a field can be followed by the click on the default button
    // which it simulates: the same credentials twice are one.
    var now = Date.now();
    if(lastCapture && now - lastCapture.at < 2000 &&
       lastCapture.username === credentials.username &&
       lastCapture.password === credentials.password) return;
    lastCapture = { at: now, username: credentials.username, password: credentials.password };

    credentials.ticket = mark();
    pending = credentials;
    if(bridge) bridge.submitted(documentMark, credentials.ticket);
}

function activate(target, keyboard){
    field = target;
    fieldMark = mark();
    var rect = target.getBoundingClientRect();
    if(bridge) bridge.activated(documentMark, fieldMark,
                                keyboard ? rect.left : -1,
                                keyboard ? rect.bottom : -1);
}

document.addEventListener('click', function(e){
    if(!e.isTrusted || !e.target || !e.target.closest) return;
    var button = e.target.closest('button, input[type="submit"], input[type="image"]');
    if(!button) return;
    // a 'button' which only does something on the page -- show the
    // password, clear the field -- is not a submission.
    if(button.tagName === 'BUTTON' && button.type !== 'submit') return;
    capture(scopeOf(button));
}, true);

document.addEventListener('keydown', function(e){
    if(!e.isTrusted || e.isComposing) return;
    var target = e.target;
    if(!isPassword(target) && !isUsername(target)) return;
    if(e.key === 'Enter') capture(scopeOf(target));
    else if(e.key === 'ArrowDown' && !e.altKey && !e.ctrlKey && !e.metaKey && !e.shiftKey)
        activate(target, true);
}, true);

document.addEventListener('mousedown', function(e){
    if(!e.isTrusted || e.button !== 0) return;
    var target = e.target;
    if(isPassword(target) || isUsername(target)) activate(target, false);
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
        if(location.origin !== origin || !field || !field.isConnected) return false;
        fieldMark = null;
        var scope = scopeOf(field);
        var passwordField = isPassword(field) ? field
            : scope.querySelector('input[type="password"]');
        if(!passwordField) return false;
        var usernameField = isUsername(field) ? field : usernameFor(scope, passwordField);
        if(usernameField && username !== '') setValue(usernameField, username);
        setValue(passwordField, password);
        return true;
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
{
}

WebEnginePasswords::~WebEnginePasswords(){
    if(m_Menu) m_Menu->deleteLater();
}

QObject *WebEnginePasswords::Bridge() const {
    return m_Bridge.data();
}

QString WebEnginePasswords::ScriptSource(){
    static const QString source = QWebChannelJsCode() + QStringLiteral("\n") +
        QString::fromUtf8(PasswordJsCode);
    return source;
}

void WebEnginePasswords::InstallScript(QWebEngineProfile *profile){
    QWebEngineScript script;
    script.setName(QStringLiteral("vanilla-passwords"));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    script.setRunsOnSubFrames(false);
    script.setSourceCode(ScriptSource());
    profile->scripts()->insert(script);
}

bool WebEnginePasswords::CurrentKey(Key *key) const {
    if(!View::SavePasswords() || !m_Host.current) return false;
    const Snapshot now = m_Host.current();
    if(!now.alive || now.space.isEmpty() || !now.profile) return false;
    key->space = now.space;
    key->origin = PasswordPolicy::Origin(now.url);
    key->scriptOrigin = PasswordPolicy::ScriptOrigin(now.url);
    key->profile = now.profile;
    key->isPrivate = now.isPrivate;
    return !key->origin.isEmpty();
}

void WebEnginePasswords::Submitted(const QString &documentMark, const QString &ticket){
    Key key;
    if(m_Question || !CurrentKey(&key)) return;
    if(key.isPrivate) return;
    if(!PasswordStore::Instance()) return;

    const QString code = QStringLiteral("window.__vanillaPasswords && "
                                        "window.__vanillaPasswords.take(%1)")
        .arg(Arguments(QStringList() << documentMark << ticket));

    QPointer<WebEnginePasswords> self(this);
    m_Host.runInApplicationWorld(code, [self, key](const QVariant &result){
        if(!self || self->m_Question) return;
        const QVariantMap map = result.toMap();
        const QString password = map.value(QStringLiteral("password")).toString();
        if(password.isEmpty()) return;

        Key now;
        if(!self->CurrentKey(&now) || !Same(now, key) || now.isPrivate ||
           map.value(QStringLiteral("origin")).toString() != key.scriptOrigin) return;

        self->Offer(key, map.value(QStringLiteral("username")).toString(), password);
    });
}

void WebEnginePasswords::Offer(const Key &key, const QString &username, const QString &password){
    PasswordStore *store = PasswordStore::Instance();
    QPointer<WebEnginePasswords> self(this);

    auto ask = [self, store, key, username, password](bool update){
        if(!self || self->m_Question || !self->m_Host.current) return;
        const Snapshot before = self->m_Host.current();
        if(!before.alive || before.profile != key.profile || before.isPrivate) return;
        const QString who = username.isEmpty() ? tr("(no username)") : username;

        auto answered = [self, store, key, username, password](bool yes){
            if(!self) return;
            self->m_Question = nullptr;
            if(!yes) return;

            if(!self->m_Host.current) return;
            const Snapshot now = self->m_Host.current();
            if(!now.alive || now.profile != key.profile || now.isPrivate) return;

            store->Save(key.space, key.origin, username, password, [self](bool ok){
                if(!ok && self && self->m_Host.message)
                    self->m_Host.message(tr("The password could not be saved."));
            });
        };
        self->m_Question = update
            ? ModelessDialog::Question(tr("Update password"),
                                       tr("Update the saved password of %1 for %2?")
                                       .arg(who, key.scriptOrigin),
                                       answered, self.data())
            : ModelessDialog::Question(tr("Save password"),
                                       tr("Save the password of %1 for %2?")
                                       .arg(who, key.scriptOrigin),
                                       answered, self.data());
    };

    if(!store->Has(key.space, key.origin, username)){
        ask(false);
        return;
    }
    store->Read(key.space, key.origin, username, false,
                [self, ask, password](bool ok, const QString &saved){
        if(!self || (ok && saved == password)) return;
        ask(ok);
    });
}

void WebEnginePasswords::Activated(const QString &documentMark, const QString &fieldMark,
                                   double left, double bottom){
    Key key;
    if(!CurrentKey(&key)) return;
    PasswordStore *store = PasswordStore::Instance();
    if(!store) return;
    const QStringList usernames = store->Usernames(key.space, key.origin);
    if(usernames.isEmpty()) return;

    Forget();
    m_MenuDocument = documentMark;
    m_MenuField = fieldMark;
    m_MenuKey = key;

    QMenu *menu = new QMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    foreach(const QString &username, usernames){
        QAction *action = menu->addAction(username.isEmpty() ? tr("(no username)") : username);
        QPointer<WebEnginePasswords> self(this);
        connect(action, &QAction::triggered, this, [self, username](){
            if(self) self->Fill(username);
        });
    }
    m_Menu = menu;

    QPoint at = QCursor::pos();
    if(left >= 0 && bottom >= 0 && m_Host.mapToGlobal && m_Host.current){
        const qreal zoom = m_Host.current().zoom;
        at = m_Host.mapToGlobal(QPoint(qRound(left * zoom), qRound(bottom * zoom)));
    }
    menu->popup(at);
}

void WebEnginePasswords::Fill(const QString &username){
    PasswordStore *store = PasswordStore::Instance();
    if(!store || m_MenuDocument.isEmpty()) return;

    const QString document = m_MenuDocument;
    const QString field = m_MenuField;
    const Key key = m_MenuKey;

    QPointer<WebEnginePasswords> self(this);
    store->Read(key.space, key.origin, username, false,
                [self, document, field, key, username](bool ok, const QString &password){
        if(!self || !ok) return;
        Key now;
        if(self->m_MenuDocument != document || self->m_MenuField != field ||
           !self->CurrentKey(&now) || !Same(now, key)) return;

        const QString code = QStringLiteral("window.__vanillaPasswords && "
                                            "window.__vanillaPasswords.fill(%1)")
            .arg(Arguments(QStringList() << document << field << now.scriptOrigin
                                         << username << password));
        self->m_Host.runInApplicationWorld(code, [](const QVariant&){});
        self->Forget();
    });
}

void WebEnginePasswords::Forget(){
    m_MenuDocument.clear();
    m_MenuField.clear();
    m_MenuKey = Key();
    if(m_Menu) m_Menu->close();
}

#endif
