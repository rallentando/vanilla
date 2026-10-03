#ifndef EXTENSIONCONTROLLER_HPP
#define EXTENSIONCONTROLLER_HPP

#include "switch.hpp"

#include "extensionhostwire.hpp"
#include "dnrrules.hpp"

#include <QObject>
#include <QJsonArray>

#include "extensionui.hpp"
#include <QIcon>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QUrl>

#include <functional>

struct ExtensionManifest {
    QString path;
    QString folder;
    QString id;
    QString name;
    QString error;
    QIcon icon;
    QString title;
    QUrl popup;
    QUrl options;
    QUrl homepage;
    QString sidePanelPath;
    bool tabsPermission = false;
    QStringList hostPermissions;
    QStringList permissions;
    QString version;
    QList<ExtensionUi::Command> commands;
    struct RuleResource {
        QString id;
        bool enabled = true;
        QString file;
    };
    QList<RuleResource> ruleResources;
    QList<Dnr::Held::Ruleset> Rulesets() const;

    static QString NormalizePath(const QString &path);
    static QString PathKey(const QString &path);
    static ExtensionManifest Read(const QString &path);
    static QString CurrentFolder(const QString &path);
    static QString Resource(const QString &root, const QString &relative);
};

struct ExtensionRuleFiles {
    QString id;
    QStringList files;
    QByteArray session;
    QByteArray dynamic;
};

struct ExtensionItem {
    QString id;
    QString name;
    QString path;
    bool enabled = false;
    QUrl popup;
    bool gone = false;
};

struct ExtensionRow {
    ExtensionManifest manifest;
    QString error;
    bool registered = true;
    bool loaded = false;
    bool enabled = false;
    bool wanted = true;
    bool pinned = false;
    QString note;
};

class ExtensionController : public QObject {
    Q_OBJECT

public:
    explicit ExtensionController(QObject *parent, const QString &journal = QString(),
                                 int timeoutMs = 20000);
    ~ExtensionController() Q_DECL_OVERRIDE;

    QList<ExtensionRow> Rows() const;
    bool IsReady() const { return m_Ready;}
    bool IsBusy() const { return m_Busy;}
    bool IsStarted() const { return m_Started;}
    QString Error() const { return m_Error;}

    void Reconcile(const QStringList &paths, const QStringList &disabled,
                   const QStringList &pinned);
    void Start();
    void Retry(const QString &path);

    static void ReloadAll();
    static void RegisterPath(const QString &path);
    static void UnregisterPath(const QString &path);
    static void SetEnabled(const QString &path, bool enabled);
    static void SetPinned(const QString &path, bool pinned);
    static ExtensionController *Of(QObject *profile);
    static QString CopyNote(const QString &note, const QString &detail, bool withheld);
    virtual QString EngineNote() const { return QString(); }
    virtual QSet<QString> ShimmedIds() const { return QSet<QString>(); }
    virtual QString RunFolderOf(const QString &id) const { Q_UNUSED(id); return QString(); }
    virtual bool HasKeyedShims(const QString &id) const { Q_UNUSED(id); return false; }
    ExtensionHostWire::Sight SightOf(const QString &id) const;
    bool HasPermission(const QString &id, const QString &name) const;
    ExtensionRow RowOf(const QString &id) const;

    ExtensionUi::Action &ActionFor(const QString &id);
    ExtensionUi::SidePanel &SidePanelFor(const QString &id);
    QJsonObject SidePanelCall(const QString &id, const QString &api, const QJsonArray &args);
    QUrl SidePanelUrl(const QString &id);
    QString SidePanelPath(const QString &id);
    QUrl SidePanelTabUrl(const QString &id, qint64 tab);
    QUrl SidePanelResource(const QString &id, const QString &path) const { return ResourceUrlOf(id, path); }
    ExtensionUi::Action *ActionOf(const QString &id);
    void ActionSet(const QString &id);
    void ClickAction(const QString &id, qint64 tab);

    const ExtensionUi::Menus *MenusOf(const QString &id) const;
    QJsonObject MenuCall(const QString &id, const QString &api, const QJsonArray &args);
    void ClickMenu(const QString &id, const QString &itemId, const ExtensionUi::MenuContext &context, qint64 tab);
    void SetMenusMirrored(bool mirrored){ m_MenusMirrored = mirrored; }
    bool MenusMirrored() const { return m_MenusMirrored; }
    QJsonObject MirrorCall(const QString &id, const QJsonArray &args);
    virtual bool SendToWorker(const QString &id, const QJsonArray &args){ Q_UNUSED(id); Q_UNUSED(args); return false; }
    void SetMenusFile(const QString &path);
    void SaveMenusNow();

    QJsonObject RulesCall(const QString &id, const QString &api, const QJsonArray &args);
    void SetRulesFile(const QString &path);
    void SaveRulesNow();
    void SetInstalledFile(const QString &path);
    QJsonObject TakeInstalled(const QString &id, bool *written);
    bool TakeStartup(const QString &id);
    bool HasCommand(const QString &key) const;
    bool RunCommand(const QString &key, qint64 serial);
    void OpenPopup(const QString &id, qint64 serial){ emit ActionCommand(id, serial); }
    QJsonArray CommandsOf(const QString &id, const std::function<bool(const QString &)> &taken) const;
    const ExtensionUi::Installed &InstalledLedger() const { return m_Installed; }
    const Dnr::Held &HeldRules() const { return m_Rules; }
    static QList<ExtensionRuleFiles> EnabledRuleFiles();
    static QSet<QString> EnabledIds();
    static QString DefaultPickDirectory();
    static QString PickDirectoryFrom(const QString &root, const QString &home);

signals:
    void Changed();
    void Ready();
    void ActionChanged(const QString &id);
    void WorkerEvent(const QString &id, const QString &name, const QJsonArray &args, qint64 tab);
    void Unlisted(const QString &id);
    void InvalidatePopups();
    void ActionCommand(const QString &id, qint64 serial);

protected:
    virtual bool HandlesCommands() const { return false; }
    virtual bool WorkerIsOfThisRun(const QString &id) const { Q_UNUSED(id); return true; }
    void Note(const QString &path, const QString &text);
    const QMap<QString, QString> &OwnedByPath() const { return m_Owned; }

    using SnapshotDone = std::function<void(QList<ExtensionItem>, QString)>;
    using ItemDone = std::function<void(ExtensionItem, QString)>;
    using Done = std::function<void(QString)>;

    virtual void Snapshot(SnapshotDone done) = 0;
    virtual void Add(const ExtensionManifest &manifest, ItemDone done) = 0;
    virtual void Enable(const QString &id, bool enabled, ItemDone done) = 0;
    virtual void Remove(const QString &id, Done done) = 0;

private:
    void Step();
    void Schedule();
    void ReadSettings();
    void MarkReady();
    quint64 Begin();
    bool Complete(quint64 sequence);
    bool SaveOwned();
    bool ReadOwned();

    QString m_Journal;
    QString m_Error;
    QMap<QString, ExtensionManifest> m_Desired;
    QMap<QString, ExtensionManifest> m_Manifests;
    QStringList m_Order;
    QSet<QString> m_Disabled;
    QSet<QString> m_Pinned;
    QMap<QString, QString> m_Owned;
    QMap<QString, QString> m_Errors;
    QMap<QString, QString> m_Notes;
    QHash<QString, ExtensionUi::Action> m_Actions;
    QHash<QString, ExtensionUi::SidePanel> m_SidePanels;
    QUrl ResourceUrlOf(const QString &id, const QString &path) const;
    QHash<QString, ExtensionUi::Menus> m_Menus;
    struct CommandOwner { QString path; QString id; QString name; };
    QHash<QString, QList<CommandOwner>> m_Commands;
    const CommandOwner *CommandFor(const QString &key) const;
    QString m_MenusFile;
    bool m_MenusDirty = false;
    bool m_MenusMirrored = false;
    void SaveMenusSoon();
    void LoadMenus();
    Dnr::Held m_Rules;
    QString m_RulesFile;
    bool m_RulesDirty = false;
    bool m_RulesKept = false;
    void SaveRulesSoon();
    void LoadRules();
    ExtensionUi::Installed m_Installed;
    QSet<QString> m_StartupOwed;
    QString m_InstalledFile;
    bool m_InstalledKept = false;
    void DecideInstalled();
    void ApplyInstalled(bool starting, bool migrating);
    bool m_InstalledRead = false;
    bool SaveInstalledNow();
    QMap<QString, ExtensionItem> m_Actual;
    bool m_Started = false;
    bool m_Busy = false;
    bool m_Ready = false;
    bool m_Scheduled = false;
    bool m_Fatal = false;
    quint64 m_Sequence = 0;
    int m_TimeoutMs;
};

class ExtensionNavigation : public QObject {
    Q_OBJECT

public:
    explicit ExtensionNavigation(QObject *parent) : QObject(parent) {}

    void Request(ExtensionController *controller, std::function<void()> action);
    void Cancel();
    static ExtensionNavigation *Of(QObject *target);

private:
    QMetaObject::Connection m_Connection;
    std::function<void()> m_Action;
};

#endif
