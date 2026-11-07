#ifndef WEBENGINEPASSWORDS_HPP
#define WEBENGINEPASSWORDS_HPP

#include "switch.hpp"

#if defined(WEBENGINEVIEW) && defined(VANILLA_PASSWORDS)

#include "passwordpolicy.hpp"

#include <QTimer>

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QWebEngineFrame>

#include <functional>
#include <optional>

class PasswordSuggestions;
class QWebEngineProfile;
class QQuickWebEngineProfile;

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
        std::function<QWebEngineFrame()> mainFrame;
        std::function<QPoint(const QPoint&)> mapToGlobal;
        std::function<QWidget*()> widget;
        std::function<void()> managePasswords;
        std::function<void(const QString&)> message;
    };

    WebEnginePasswords(QObject *owner, const Host &host);
    ~WebEnginePasswords();

    QObject *Bridge() const;

    static QString ScriptSource();
    static QString FrameScriptSource();
    static void InstallScript(QWebEngineProfile *profile);
#ifdef VANILLA_QUICK_PASSWORD_FRAMES
    static void InstallFrameScript(QQuickWebEngineProfile *profile);
#endif

    void Forget();

    void Submitted(const QString &documentMark, const QString &ticket);
    void Held(const QString &documentMark, const QString &ticket);
    void Confirmed(const QString &documentMark);
    void Released(const QString &documentMark);
    void Navigated(const QUrl &url);
    void Rang(bool loaded);
    void Closed(const QString &documentMark, const QString &fieldMark);
    void Filtered(const QString &documentMark, const QString &fieldMark,
                  const QVariantList &numbers);

    static QString PageScript();
    void Activated(const QString &documentMark, const QString &fieldMark,
                   double left, double bottom, double heard);

protected:
    bool eventFilter(QObject *watched, QEvent *ev) override;

private:
    struct Key {
        QString space;
        QString origin;
        QString scriptOrigin;
        const QObject *profile = nullptr;
        bool isPrivate = false;
    };
    bool CurrentKey(Key *key) const;
    bool KeyAt(const QUrl &url, Key *key) const;
    static bool Same(const Key &a, const Key &b){
        return a.profile == b.profile && a.space == b.space && a.origin == b.origin;
    }
    void ForgetMenu();
    void CloseList(bool tell, bool replacing = false);
    void TellClosed(const QString &field);
    void Take(int row);
    bool InView() const;
    void Offer(const Key &key, const QString &username, const QString &password);
    void Ask(const Key &key, const QString &username, const QString &password);
    bool Saved(const Key &key, const QString &username) const;
    Key m_QuestionKey;
    bool m_QuestionNameStep = false;
    QPointer<QWidget> m_QuestionFields;
    struct PendingName {
        QString space;
        QString origin;
        const QObject *profile;
        QString name;
        quint64 serial;
    };
    QList<PendingName> m_PendingNames;
    quint64 m_PendingSerial = 0;
    void OfferHeld();
    void Fill(const QString &username);
    void ShowMenu(const Key &key, const QString &documentMark, const QString &fieldMark,
                  double left, double bottom, const std::optional<QWebEngineFrame> &frame);

    void Drain();
    void FromFrame(const QWebEngineFrame &frame, const Key &key,
                   const QUrl &frameUrl, const QUrl &mainUrl, const QVariantMap &entry,
                   bool listStale);
    void CheckHeldFrame();
    bool m_DrainQueued = false;
    FrameDrain m_Drain;

    Host m_Host;
    QPointer<WebEnginePasswordBridge> m_Bridge;

    QPointer<PasswordSuggestions> m_List;
    QStringList m_ListUsernames;
    QList<int> m_ListShown;
    QPoint m_ListAnchor;
    bool m_ListReading = false;
    QPointer<QWidget> m_ListView;
    QPointer<QWidget> m_ListWindow;
    quint64 m_ListGeneration = 0;
    quint64 m_EditGeneration = 0;
    QString m_ClosedDocument;
    quint64 m_ClosedGeneration = 0;
    QString m_MenuDocument;
    QString m_MenuField;
    Key m_MenuKey;
    std::optional<QWebEngineFrame> m_MenuFrame;

    QPointer<ModelessDialog> m_Question;

    PasswordHold m_Hold;
    Key m_HeldKey;
    QTimer m_HoldTimer;
    std::optional<QWebEngineFrame> m_HeldFrame;
    QString m_HeldDocument;
    QUrl m_HeldMainUrl;
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
                   double left, double bottom, double heard){
        if(m_Owner) m_Owner->Activated(documentMark, fieldMark, left, bottom, heard);
    }
    void held(const QString &documentMark, const QString &ticket){
        if(m_Owner) m_Owner->Held(documentMark, ticket);
    }
    void confirmed(const QString &documentMark){
        if(m_Owner) m_Owner->Confirmed(documentMark);
    }
    void released(const QString &documentMark){
        if(m_Owner) m_Owner->Released(documentMark);
    }
    void rang(bool loaded){
        if(m_Owner) m_Owner->Rang(loaded);
    }
    void closed(const QString &documentMark, const QString &fieldMark){
        if(m_Owner) m_Owner->Closed(documentMark, fieldMark);
    }
    void filtered(const QString &documentMark, const QString &fieldMark,
                  const QVariantList &numbers){
        if(m_Owner) m_Owner->Filtered(documentMark, fieldMark, numbers);
    }

private:
    QPointer<WebEnginePasswords> m_Owner;
};

#endif

#endif
