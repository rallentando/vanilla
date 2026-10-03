#ifndef EXTENSIONUI_HPP
#define EXTENSIONUI_HPP

#include "switch.hpp"

#include <functional>
#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace ExtensionUi {

struct Icon {
    QByteArray bytes;
    bool isNull() const { return bytes.isEmpty(); }
};

struct ActionValues {
    bool hasIcon = false, hasBadgeText = false, hasBadgeColor = false,
         hasTextColor = false, hasTitle = false, hasEnabled = false;
    Icon icon;
    QString badgeText;
    QString badgeColor;
    QString textColor;
    QString title;
    bool enabled = true;
};

struct ActionShown {
    bool hasIcon = false;
    Icon icon;
    QString badgeText;
    QString badgeColor;
    QString textColor;
    bool hasTitle = false;
    QString title;
    bool enabled = true;
};

struct TabNow {
    qint64 id = 0;
    QUrl url;
};

const int ACTION_TABS_KEPT = 256;
const int ICON_BYTES_LIMIT = 512 * 1024;
const int ICON_SENT_LIMIT = 12 * 1024;

class Action {
public:
    const ActionValues *OfTab(const TabNow &tab);
    ActionShown Shown(const TabNow &tab);
    ActionShown ShownForAll() const;

    void SetIcon(const TabNow &tab, const Icon &icon);
    void SetBadgeText(const TabNow &tab, const QString &text);
    void SetBadgeColor(const TabNow &tab, const QString &color);
    void SetTextColor(const TabNow &tab, const QString &color);
    void SetTitle(const TabNow &tab, const QString &title);
    void SetEnabled(const TabNow &tab, bool enabled);
    void UnsetBadgeText(const TabNow &tab);
    void UnsetTitle(const TabNow &tab);
    int TabsKept() const { return m_Order.size(); }

private:
    ActionValues &Layer(const TabNow &tab);
    struct TabLayer {
        QUrl url;
        ActionValues values;
    };
    ActionValues m_All;
    QHash<qint64, TabLayer> m_Tabs;
    QList<qint64> m_Order;
};

QString ColorOf(const QJsonValue &value);

QSize PopupSizeOf(const QString &measured, qreal ratio, qreal scale);

QJsonObject GetOfAction(const QString &api, Action &action, const TabNow *tab, const QString &manifestTitle);

const int ICON_SIZE_LIMIT = 256;
QString IconPathOf(const QJsonValue &path);
QByteArray IconBytesOf(const QJsonValue &imageData);

struct MenuItem {
    QString id;
    QString parentId;
    QString type = QStringLiteral("normal");
    QString title;
    QStringList contexts;
    bool checked = false;
    bool enabled = true;
    bool visible = true;
    QStringList documentUrlPatterns;
    QStringList targetUrlPatterns;
};

const int MENU_ITEMS_LIMIT = 100;
const int MENU_DEPTH_LIMIT = 8;
const int MENU_ID_LIMIT = 256;
const int MENU_TITLE_LIMIT = 1024;
const int MENU_PATTERNS_LIMIT = 32;

struct MenuContext {
    QSet<QString> contexts;
    QUrl pageUrl;
    QUrl linkUrl;
    QUrl srcUrl;
    QString selectionText;
};

struct MenuShown {
    MenuItem item;
    QString title;
    QList<MenuShown> children;
};

class Menus {
public:
    QJsonObject Create(const QJsonObject &properties);
    QJsonObject Update(const QString &id, const QJsonObject &properties);
    QJsonObject Remove(const QString &id);
    QJsonObject RemoveAll();

    QList<MenuShown> Shown(const MenuContext &context) const;
    QList<MenuShown> ActionShown(const QUrl &pageUrl) const;
    static MenuContext ActionContext(const QUrl &pageUrl);
    struct Clicked {
        bool found = false;
        bool changed = false;
        bool wasChecked = false;
        bool checked = false;
        bool checkable = false;
        QString parentId;
    };
    Clicked Click(const QString &id);

    const QList<MenuItem> &Items() const { return m_Items; }
    bool IsEmpty() const { return m_Items.isEmpty(); }

    QJsonArray ToJson() const;
    static Menus FromJson(const QJsonArray &json);

private:
    int IndexOf(const QString &id) const;
    void Select(int at);
    void Sanitize(const QString &parentId);
    int DepthOf(const QString &id) const;
    int HeightOf(const QString &id) const;
    QList<MenuItem> m_Items;
};

QString MenuIdOf(const QJsonValue &value);

qint64 TabIdOf(const QJsonValue &value, bool *ok);

QString MenusFileName(const QString &profileKey);
QString RulesFileName(const QString &profileKey);

QJsonObject ClickInfo(const MenuItem &item, const Menus::Clicked &clicked, const MenuContext &context);

bool AcceptsDownloadUrl(const QString &text);
bool AcceptsDownloadFilename(const QString &name);
QString PathOfEndedDownload(bool completed, const QString &path);
const int DOWNLOADS_EXPECTED_KEPT = 32;
const qint64 DOWNLOADS_EXPECTED_MS = 60 * 1000;
const int DOWNLOADS_KEPT = 32;

struct DownloadRecord {
    qint64 id = 0;
    QString extension;
    QUrl url;
    QString filename;
    QString mime;
    QString state = QStringLiteral("in_progress");
    QString startTime;
    QString endTime;
    QString error;
    qint64 totalBytes = -1;
    qint64 receivedBytes = 0;
    bool paused = false;
};

QString InterruptReasonOf(int reason);
QString WhyNotDownloadAct(const QString &api, const DownloadRecord *record);

QString WhyNotOpenPopup(bool hasPopup, Action *action, const TabNow *tab);

class Downloads {
public:
    qint64 Expect(const QString &extension, const QUrl &url, const QString &filename, qint64 now);
    struct Gone {
        QString extension;
        QJsonObject delta;
    };
    QList<Gone> Expired(qint64 now);
    qint64 Arrived(const QUrl &url, qint64 now, QString *extension);
    void Describe(qint64 id, const QString &filename, const QString &mime, qint64 totalBytes, qint64 receivedBytes);
    QJsonObject Ended(qint64 id, bool completed, const QString &error);
    QJsonObject Created(qint64 id) const;
    QJsonArray Search(const QString &extension, const QJsonObject &query) const;
    QJsonObject Named(qint64 id, const QString &filename);
    QJsonObject Paused(qint64 id, bool paused);
    QList<qint64> Erase(const QString &extension, const QJsonObject &query);
    const DownloadRecord *RecordOf(qint64 id) const;
    const DownloadRecord *RecordOf(const QString &extension, qint64 id) const;
    int Expected() const { return m_Expected.size(); }
    int Records() const { return m_Records.size(); }

private:
    struct Expectation {
        qint64 id = 0;
        QString extension;
        QUrl url;
        QString filename;
        qint64 since = 0;
    };
    void Forget(qint64 now);
    qint64 m_Next = 1;
    QList<Expectation> m_Expected;
    QList<DownloadRecord> m_Records;
};

QString InstalledFileName(const QString &profileKey);

struct Command {
    QString name;
    QString description;
    QString key;
};
QList<Command> CommandsOf(const QJsonObject &commands, const QString &platform, bool hasAction);
QString ChromeKeyOf(bool ctrl, bool alt, bool shift, int virtualKey, int qtKey);
QString QtKeyTextOf(const QString &chromeKey);
enum class KeyRoute { Vanilla, Extension, Pass };
KeyRoute RouteKey(bool viewMapHit, bool appMapHit, bool commandHit);

class Installed {
public:
    struct Owed {
        QString reason;
        QString previousVersion;
    };
    QStringList Decide(const QList<QPair<QString, QString>> &registered, bool migrating);
    Owed Take(const QString &id);
    Owed OwedTo(const QString &id) const;
    void Restore(const QString &id, const Owed &owed);
    bool Forget(const QSet<QString> &registered);
    bool Contains(const QString &id) const { return m_Entries.contains(id); }

    QJsonObject ToJson() const;
    static Installed FromJson(const QJsonObject &json, bool *damaged);

private:
    struct Entry {
        QString version;
        Owed owed;
    };
    QMap<QString, Entry> m_Entries;
};

class SidePanel {
public:
    typedef std::function<QUrl(const QString &)> Resolve;
    QJsonObject SetOptions(const QJsonArray &args, const Resolve &resolve);
    QJsonObject GetOptions(const QJsonArray &args, const QString &fallback) const;
    QJsonObject SetBehavior(const QJsonArray &args);
    QJsonObject GetBehavior() const;
    QUrl Url(const QString &fallback, const Resolve &resolve) const;
    QString Path(const QString &fallback) const;
    bool Enabled() const { return m_Enabled; }
    bool OpensOnAction() const { return m_OnAction; }
    QString TabPath(qint64 tab) const;
    bool EnabledFor(qint64 tab) const;
    QUrl TabUrl(qint64 tab, const Resolve &resolve) const;
    static const int TABS_KEPT = 256;
private:
    struct Options {
        bool hasPath = false, hasEnabled = false;
        QString path;
        bool enabled = true;
    };
    bool m_HasPath = false;
    QString m_Path;
    bool m_Enabled = true;
    bool m_OnAction = false;
    QHash<qint64, Options> m_Tabs;
};

QJsonObject NoticeOf(const QJsonObject &given, const QJsonObject &previous, bool creating, QString *error);
QString NoticeTitle(const QString &extensionName, const QJsonObject &options);
QString NoticeText(const QJsonObject &options);

class Notices {
public:
    static const int PER_EXTENSION = 3;
    struct Gone {
        QString id;
        quint64 serial = 0;
    };
    struct Added {
        quint64 serial = 0;
        quint64 replaced = 0;
        QList<Gone> pushedOut;
    };
    Added Add(const QString &extension, const QString &id, const QJsonObject &options, const QString &version);
    bool Take(const QString &extension, const QString &id, quint64 serial);
    quint64 SerialOf(const QString &extension, const QString &id) const;
    QJsonObject OptionsOf(const QString &extension, const QString &id) const;
    QString VersionOf(const QString &extension) const;
    QStringList Ids(const QString &extension) const;
    QStringList Extensions() const;
    QList<Gone> Drop(const QString &extension);
private:
    struct Entry {
        QString id;
        quint64 serial = 0;
        QJsonObject options;
    };
    struct Own {
        QString version;
        QList<Entry> entries;
    };
    QHash<QString, Own> m_Own;
    quint64 m_Serial = 0;
};

}

#endif
