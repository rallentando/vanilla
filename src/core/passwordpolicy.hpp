#ifndef PASSWORDPOLICY_HPP
#define PASSWORDPOLICY_HPP

#include "switch.hpp"

#include <QString>
#include <QUrl>
#include <Qt>

namespace PasswordPolicy {

QString Origin(const QUrl &url);

QString ScriptOrigin(const QUrl &url);

bool IsLoopbackHost(const QString &host);

QString VaultKey(const QString &prefix, const QString &space,
                 const QString &origin, const QString &username);

}

class PasswordHold {
public:
    struct Credentials {
        QString username;
        QString password;
    };

    void Begin(const QString &document, const QUrl &url, const QString &ticket = QString()){
        Clear();
        m_State = Taking;
        m_Document = document;
        m_Url = url;
        m_Ticket = ticket;
    }
    bool Taken(const QString &document, const Credentials &credentials,
               const QString &ticket = QString()){
        if(m_State != Taking || document != m_Document || ticket != m_Ticket) return false;
        m_Credentials = credentials;
        m_State = Holding;
        return m_Confirmed;
    }
    bool Confirm(const QString &document){
        if(m_State == None || document != m_Document) return false;
        return Confirmed();
    }
    bool Navigated(const QUrl &url){
        if(m_State == None) return false;
        if(url.adjusted(QUrl::RemoveFragment) == m_Url.adjusted(QUrl::RemoveFragment)) return false;
        return Confirmed();
    }
    void Release(const QString &document){
        if(m_State != None && document == m_Document) Clear();
    }
    Credentials Take(){
        const Credentials credentials = m_Credentials;
        Clear();
        return credentials;
    }
    void Clear(){
        m_State = None;
        m_Document.clear();
        m_Url = QUrl();
        m_Ticket.clear();
        m_Credentials = Credentials();
        m_Confirmed = false;
    }
    bool IsEmpty() const { return m_State == None;}

private:
    bool Confirmed(){
        if(m_State == Holding) return true;
        m_Confirmed = true;
        return false;
    }

    enum State { None, Taking, Holding } m_State = None;
    QString m_Document;
    QUrl m_Url;
    QString m_Ticket;
    Credentials m_Credentials;
    bool m_Confirmed = false;
};

class FrameDrain {
public:
    bool Knock(){
        if(m_Running){
            m_Again = true;
            return false;
        }
        return true;
    }
    quint64 Begin(){
        m_Running = true;
        return ++m_Generation;
    }
    bool End(quint64 generation){
        if(!m_Running || generation != m_Generation) return false;
        m_Running = false;
        const bool again = m_Again;
        m_Again = false;
        return again;
    }
    void Reset(){
        m_Running = false;
        m_Again = false;
    }
    bool IsRunning() const { return m_Running;}

private:
    quint64 m_Generation = 0;
    bool m_Running = false;
    bool m_Again = false;
};

class SuggestionKeys {
public:
    enum Key { Up, Down, Enter, Escape, Tab, Other };
    enum Action {
        Pass,
        Edit,
        Move,
        Choose,
        Close,
        CloseAndPass
    };

    static Key Classify(int key, Qt::KeyboardModifiers modifiers){
        const Qt::KeyboardModifiers others = modifiers & ~Qt::KeypadModifier;
        if(key == Qt::Key_Backtab || (key == Qt::Key_Tab && (others & ~Qt::ShiftModifier) == 0))
            return Tab;
        if(others != 0) return Other;
        switch(key){
        case Qt::Key_Up: return Up;
        case Qt::Key_Down: return Down;
        case Qt::Key_Return: case Qt::Key_Enter: return Enter;
        case Qt::Key_Escape: return Escape;
        case Qt::Key_Tab: return Tab;
        default: return Other;
        }
    }

    static Action Decide(bool open, bool shown, bool chosen, Key key){
        if(!open) return Pass;
        if(key == Other) return Edit;
        if(!shown) return key == Escape || key == Tab ? CloseAndPass : Pass;
        switch(key){
        case Up: case Down: return Move;
        case Enter: return chosen ? Choose : CloseAndPass;
        case Escape: return Close;
        case Tab: return CloseAndPass;
        default: return Pass;
        }
    }

    static bool Eaten(Action action){
        return action == Move || action == Choose || action == Close;
    }
};

#endif
