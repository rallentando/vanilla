#ifndef WEBENGINEPASSWORDS_HPP
#define WEBENGINEPASSWORDS_HPP

#include "switch.hpp"

#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <functional>

class QMenu;
class QWebEngineProfile;

class WebEnginePasswordBridge;
class ModelessDialog;

class WebEnginePasswords : public QObject {
    Q_OBJECT

public:
    struct Snapshot {
        bool alive = false;
        QUrl url;
        QString space;
        const QObject *profile = nullptr;
        bool isPrivate = false;
        qreal zoom = 1.0;
    };

    struct Host {
        std::function<Snapshot()> current;
        std::function<void(const QString&, std::function<void(const QVariant&)>)> runInApplicationWorld;
        std::function<QPoint(const QPoint&)> mapToGlobal;
        std::function<void(const QString&)> message;
    };

    WebEnginePasswords(QObject *owner, const Host &host);
    ~WebEnginePasswords();

    QObject *Bridge() const;

    static QString ScriptSource();
    static void InstallScript(QWebEngineProfile *profile);

    void Forget();

    void Submitted(const QString &documentMark, const QString &ticket);
    void Activated(const QString &documentMark, const QString &fieldMark,
                   double left, double bottom);

private:
    struct Key {
        QString space;
        QString origin;
        QString scriptOrigin;
        const QObject *profile = nullptr;
        bool isPrivate = false;
    };
    bool CurrentKey(Key *key) const;
    static bool Same(const Key &a, const Key &b){
        return a.profile == b.profile && a.space == b.space && a.origin == b.origin;
    }
    void Offer(const Key &key, const QString &username, const QString &password);
    void Fill(const QString &username);

    Host m_Host;
    QPointer<WebEnginePasswordBridge> m_Bridge;

    QPointer<QMenu> m_Menu;
    QString m_MenuDocument;
    QString m_MenuField;
    Key m_MenuKey;

    QPointer<ModelessDialog> m_Question;
};

class WebEnginePasswordBridge : public QObject {
    Q_OBJECT

public:
    explicit WebEnginePasswordBridge(WebEnginePasswords *owner)
        : QObject(owner), m_Owner(owner){}

public slots:
    void submitted(const QString &documentMark, const QString &ticket){
        if(m_Owner) m_Owner->Submitted(documentMark, ticket);
    }
    void activated(const QString &documentMark, const QString &fieldMark,
                   double left, double bottom){
        if(m_Owner) m_Owner->Activated(documentMark, fieldMark, left, bottom);
    }

private:
    QPointer<WebEnginePasswords> m_Owner;
};

#endif

#endif
