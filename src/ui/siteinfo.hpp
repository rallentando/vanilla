#ifndef SITEINFO_HPP
#define SITEINFO_HPP

#include "switch.hpp"
#include "devicescale.hpp"
#include "panelwindow.hpp"

#include <QJsonObject>
#include <QPointer>
#include <QToolButton>
#include <QUrl>

#include <functional>

class TreeBank;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace SiteInfo {
    enum Connection {
        Https,
        Http,
        LocalFile,
        Internal,
        Other,
    };
    Connection ConnectionOf(const QUrl &url);
    bool HasSavedException(const QUrl &url, const QStringList &allowed);
    QString Heading(const QUrl &url);
    QString SettingsFragment(const QString &origin, bool hasRows);
}

class SiteDataLink;
class SiteCookies : public QObject {
public:
    enum State { Counting, Counted, Unavailable, Confirming, Deleting };
    SiteCookies(SiteDataLink *link, const QUrl &url, std::function<void()> changed, QObject *parent);

    State GetState() const { return m_State;}
    int Count() const { return static_cast<int>(m_Cookies.length());}
    bool PartlyFailed() const { return m_Partly;}
    void Press();
    void Recount();

    static bool HostSees(const QString &host, const QString &domain);
    static QString ClearedStorageTypes();

private:
    void Call(const QString &method, const QJsonObject &params,
              std::function<void(bool, const QJsonObject&)> then);
    void DeleteFrom(int index);
    void Set(State state);

    QPointer<SiteDataLink> m_Link;
    QString m_Host;
    QString m_Origin;
    QList<QJsonObject> m_Cookies;
    State m_State;
    bool m_Partly;
    std::function<void()> m_Changed;
};

class SiteButton : public QToolButton {
    Q_OBJECT
public:
    explicit SiteButton(QWidget *parent);
    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }
    void ApplyTheme();
};

class SiteInfoPanel : public PanelWindow {
    Q_OBJECT
public:
    static SiteInfoPanel *Open(QWidget *anchor, TreeBank *bank, const QUrl &url,
                               SiteDataLink *link = nullptr);

    QString Origin() const { return m_Origin;}

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    QString SettingsFragmentNow() const;

protected:
    void keyPressEvent(QKeyEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;

private:
    SiteInfoPanel(QWidget *anchor, TreeBank *bank, const QUrl &url, SiteDataLink *link);
    void Dismiss();
    void Build();
    void BuildSiteData(QVBoxLayout *layout);
    void ShowSiteData();
    QPoint Place() const;

    QPointer<QWidget> m_Anchor;
    QPointer<TreeBank> m_TreeBank;
    QUrl m_Url;
    QString m_Origin;
    SiteDataLink *m_Link;
    SiteCookies *m_Cookies;
    QLabel *m_CookieCount;
    QPushButton *m_DeleteData;
    friend class tst_siteinfo;
};

#endif
