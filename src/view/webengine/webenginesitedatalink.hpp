#ifndef WEBENGINESITEDATALINK_HPP
#define WEBENGINESITEDATALINK_HPP

#include "switch.hpp"

#include "sitedatalink.hpp"

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include <functional>

class QWebEnginePage;
class QQuickItem;

class DevToolsSiteDataLink : public SiteDataLink {
public:
    ~DevToolsSiteDataLink() override;

    void Call(const QString &method, const QJsonObject &params, Answer done) override;

    QString Key() const { return m_Key;}
    qint64 Base() const { return m_Base;}

    static QUrl HiddenPageUrl();
    static const qint64 ID_FLOOR = Q_INT64_C(1) << 30;

protected:
    using Evaluated = std::function<void(const QVariant &result)>;

    explicit DevToolsSiteDataLink(QObject *parent);

    virtual void Evaluate(const QString &script, Evaluated done) = 0;
    virtual bool FrontendAttached() const = 0;

    void Install();
    bool Settled() const { return m_Ready || m_Broken;}
    void Shutdown();
    void Break();
    QString ForgetScript() const;

private:
    struct Pending {
        QString method;
        QJsonObject params;
        Answer done;
        QElapsedTimer asked;
    };
    void Send(const Pending &pending);
    void Poll();
    bool StillAttached() const;

    bool m_Ready;
    bool m_Broken;
    QString m_Key;
    qint64 m_Base;
    qint64 m_Next;
    QList<Pending> m_Queued;
    QHash<qint64, Pending> m_Waiting;
    QElapsedTimer m_Clock;
    double m_PageClock;
    QTimer m_Poll;
};

class WebEngineSiteDataLink : public DevToolsSiteDataLink {
public:
    explicit WebEngineSiteDataLink(QWebEnginePage *page, QObject *parent = nullptr);
    ~WebEngineSiteDataLink() override;

    QWebEnginePage *Target() const { return m_Target;}
    bool UsesHiddenPage() const { return m_Hidden != nullptr;}

protected:
    void Evaluate(const QString &script, Evaluated done) override;
    bool FrontendAttached() const override;

private:
    QPointer<QWebEnginePage> m_Page;
    QPointer<QWebEnginePage> m_Target;
    QWebEnginePage *m_Hidden;
};

class QuickWebEngineSiteDataLink : public DevToolsSiteDataLink {
    Q_OBJECT
public:
    explicit QuickWebEngineSiteDataLink(QQuickItem *view, QObject *parent = nullptr);
    ~QuickWebEngineSiteDataLink() override;

    QQuickItem *Target() const;
    bool UsesHiddenView() const { return m_Hidden != nullptr;}

protected:
    void Evaluate(const QString &script, Evaluated done) override;
    bool FrontendAttached() const override;

private slots:
    void Answered(int id, const QVariant &result);
    void HiddenLoaded(bool ok);

private:
    QPointer<QQuickItem> m_View;
    QPointer<QObject> m_Profile;
    QPointer<QQuickItem> m_Target;
    QPointer<QQuickItem> m_Hidden;
    bool m_Installed;
    QHash<int, DevToolsSiteDataLink::Evaluated> m_Evaluations;
};

#endif
