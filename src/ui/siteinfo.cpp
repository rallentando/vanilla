#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "blurbehind.hpp"

#include "siteinfo.hpp"

#include <QComboBox>
#include <QFrame>
#include <QJsonArray>
#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QKeyEvent>
#include <QScrollArea>
#include <QVBoxLayout>

#include "application.hpp"
#include "permissiontable.hpp"
#include "settingspage.hpp"
#include "sitepermissions.hpp"
#include "sitedatalink.hpp"
#include "treebank.hpp"
#include "mainwindow.hpp"

SiteInfo::Connection SiteInfo::ConnectionOf(const QUrl &url){
    const QString scheme = url.scheme().toLower();
    if(scheme == QStringLiteral("https")) return Https;
    if(scheme == QStringLiteral("http")) return Http;
    if(scheme == QStringLiteral("file")) return LocalFile;
    if(scheme == VANILLA_SCHEME || scheme == QStringLiteral("about") ||
       scheme == QStringLiteral("chrome") || scheme == QStringLiteral("edge"))
        return Internal;
    return Other;
}

bool SiteInfo::HasSavedException(const QUrl &url, const QStringList &allowed){
    const QString host = url.host().toLower();
    if(host.isEmpty() || ConnectionOf(url) != Https) return false;
    const QString prefix = host + QLatin1Char(':');
    for(const QString &entry : allowed)
        if(entry.toLower().startsWith(prefix)) return true;
    return false;
}

QString SiteInfo::Heading(const QUrl &url){
    switch(ConnectionOf(url)){
    case LocalFile: return QCoreApplication::translate("SiteInfoPanel", "File");
    case Internal:  return QCoreApplication::translate("SiteInfoPanel", "A page of vanilla");
    default: break;
    }
    return url.host().isEmpty() ? url.scheme() : url.host();
}

QString SiteInfoPanel::SettingsFragmentNow() const {
    bool rows = false;
    for(const PermissionTable::Row &row : SitePermissions::Table().Rows())
        if(row.origin == m_Origin) rows = true;
    return SiteInfo::SettingsFragment(m_Origin, rows);
}

QString SiteInfo::SettingsFragment(const QString &origin, bool hasRows){
    if(!hasRows || origin.isEmpty()) return QStringLiteral("permissions");
    return QStringLiteral("permissions?find=") + QString::fromLatin1(QUrl::toPercentEncoding(origin));
}

SiteCookies::SiteCookies(SiteDataLink *link, const QUrl &url, std::function<void()> changed, QObject *parent)
    : QObject(parent)
    , m_Link(link)
    , m_Host(QString::fromLatin1(QUrl::toAce(url.host())).toLower())
    , m_Origin(PermissionTable::Origin(url))
    , m_State(Counting)
    , m_Partly(false)
    , m_Changed(changed)
{
    Recount();
}

bool SiteCookies::HostSees(const QString &host, const QString &domain){
    const QString h = host.toLower();
    const QString d = domain.toLower();
    if(d.isEmpty() || h.isEmpty()) return false;
    if(!d.startsWith(QLatin1Char('.'))) return h == d;
    const QString bare = d.mid(1);
    return h == bare || h.endsWith(d);
}

QString SiteCookies::ClearedStorageTypes(){
    return QStringLiteral("local_storage,indexeddb,cache_storage,service_workers,file_systems,storage_buckets");
}

void SiteCookies::Set(State state){
    m_State = state;
    if(m_Changed) m_Changed();
}

void SiteCookies::Call(const QString &method, const QJsonObject &params,
                       std::function<void(bool, const QJsonObject&)> then){
    if(!m_Link){
        QTimer::singleShot(0, this, [then](){ then(false, QJsonObject());});
        return;
    }
    auto answered = std::make_shared<bool>(false);
    QPointer<SiteCookies> self(this);
    QTimer::singleShot(SiteDataLink::TIMEOUT_MSEC, Qt::PreciseTimer, this, [answered, then](){
        if(*answered) return;
        *answered = true;
        then(false, QJsonObject());
    });
    m_Link->Call(method, params, [self, answered, then](bool ok, const QJsonObject &result){
        if(!self || *answered) return;
        *answered = true;
        then(ok, result);
    });
}

void SiteCookies::Recount(){
    Set(Counting);
    Call(QStringLiteral("Network.getAllCookies"), QJsonObject(), [this](bool ok, const QJsonObject &result){
        if(!ok || !result.value(QStringLiteral("cookies")).isArray()){
            m_Cookies.clear();
            Set(Unavailable);
            return;
        }
        m_Cookies.clear();
        for(const QJsonValue &value : result.value(QStringLiteral("cookies")).toArray()){
            const QJsonObject cookie = value.toObject();
            if(HostSees(m_Host, cookie.value(QStringLiteral("domain")).toString()))
                m_Cookies << cookie;
        }
        Set(Counted);
    });
}

void SiteCookies::Press(){
    if(m_State == Counted){
        Set(Confirming);
        return;
    }
    if(m_State != Confirming) return;
    m_Partly = false;
    Set(Deleting);
    DeleteFrom(0);
}

void SiteCookies::DeleteFrom(int index){
    if(index >= m_Cookies.length()){
        QJsonObject params;
        params.insert(QStringLiteral("origin"), m_Origin);
        params.insert(QStringLiteral("storageTypes"), ClearedStorageTypes());
        Call(QStringLiteral("Storage.clearDataForOrigin"), params, [this](bool ok, const QJsonObject&){
            if(!ok) m_Partly = true;
            Recount();
        });
        return;
    }
    const QJsonObject cookie = m_Cookies.at(index);
    if(cookie.value(QStringLiteral("partitionKeyOpaque")).toBool()){
        DeleteFrom(index + 1);
        return;
    }
    QJsonObject params;
    params.insert(QStringLiteral("name"), cookie.value(QStringLiteral("name")));
    params.insert(QStringLiteral("domain"), cookie.value(QStringLiteral("domain")));
    params.insert(QStringLiteral("path"), cookie.value(QStringLiteral("path")));
    if(cookie.contains(QStringLiteral("partitionKey")))
        params.insert(QStringLiteral("partitionKey"), cookie.value(QStringLiteral("partitionKey")));
    Call(QStringLiteral("Network.deleteCookies"), params, [this, index](bool ok, const QJsonObject&){
        if(!ok){
            m_Partly = true;
            Recount();
            return;
        }
        DeleteFrom(index + 1);
    });
}

SiteButton::SiteButton(QWidget *parent)
    : QToolButton(parent)
{
    setObjectName(QStringLiteral("SiteButton"));
    setToolTip(tr("Site information"));
    setAccessibleName(tr("Site information"));
    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);
    setIconSize(QSize(ScaleByDevice(TOOL_BAR_ICON_SIZE), ScaleByDevice(TOOL_BAR_ICON_SIZE)));
    ApplyTheme();
}

void SiteButton::ApplyTheme(){
    setIcon(QIcon(Theme::Pixmap(QStringLiteral(":/resources/toolbar/site.png"), Theme::BarIcon)));
}

SiteInfoPanel::SiteInfoPanel(QWidget *anchor, TreeBank *bank, const QUrl &url, SiteDataLink *link)
    : PanelWindow(anchor, Qt::Popup, BlurBehind::Enabled())
    , m_Anchor(anchor)
    , m_TreeBank(bank)
    , m_Url(url)
    , m_Origin(PermissionTable::Origin(url))
    , m_Link(link)
    , m_Cookies(nullptr)
    , m_CookieCount(nullptr)
    , m_DeleteData(nullptr)
{
    if(m_Link) m_Link->setParent(this);
    setObjectName(QStringLiteral("SiteInfoPanel"));
    setFixedWidth(ScaleByDevice(EXTENSION_LIST_WIDTH));
    setFocusPolicy(Qt::StrongFocus);
    Build();
    SpreadPanelAccent(this);
}

SiteInfoPanel *SiteInfoPanel::Open(QWidget *anchor, TreeBank *bank, const QUrl &url,
                                   SiteDataLink *link){
    SiteInfoPanel *panel = new SiteInfoPanel(anchor, bank, url, link);
    if(anchor) panel->setMaximumHeight(anchor->screen()->availableGeometry().height());
    panel->adjustSize();
    panel->move(panel->Place());
    panel->show();
    panel->setFocus(Qt::PopupFocusReason);
    return panel;
}

void SiteInfoPanel::Dismiss(){
    QPointer<TreeBank> bank = m_TreeBank;
    close();
    if(bank && bank->GetMainWindow()) bank->GetMainWindow()->SetFocus();
}

void SiteInfoPanel::hideEvent(QHideEvent *ev){
    delete m_Link;
    m_Link = nullptr;
    PanelWindow::hideEvent(ev);
}

void SiteInfoPanel::keyPressEvent(QKeyEvent *ev){
    if(ev->key() == Qt::Key_Escape){
        Dismiss();
        ev->accept();
        return;
    }
    PanelWindow::keyPressEvent(ev);
}

QPoint SiteInfoPanel::Place() const {
    if(!m_Anchor) return pos();
    const QRect screen = m_Anchor->screen()->availableGeometry();
    QPoint point = m_Anchor->mapToGlobal(QPoint(0, m_Anchor->height()));
    if(point.y() + height() > screen.bottom())
        point.setY(m_Anchor->mapToGlobal(QPoint()).y() - height());
    point.setX(qBound(screen.left(), point.x(), qMax(screen.left(), screen.right() - width() + 1)));
    point.setY(qBound(screen.top(), point.y(), qMax(screen.top(), screen.bottom() - height() + 1)));
    return point;
}

void SiteInfoPanel::Build(){
    const int padding = ScaleByDevice(EXTENSION_PANEL_PADDING);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(padding, padding, padding, padding);
    const int indent = padding / 2;

    auto line = [this, layout](){ layout->addWidget(new PanelRule(this));};
    auto words = [this, layout, indent](const QString &text){
        QLabel *label = new QLabel(text, this);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        label->setContentsMargins(indent, 0, 0, 0);
        layout->addWidget(label);
        return label;
    };

    QHBoxLayout *head = new QHBoxLayout;
    head->setContentsMargins(indent, 0, 0, 0);
    QLabel *heading = new QLabel(SiteInfo::Heading(m_Url), this);
    heading->setObjectName(QStringLiteral("SiteInfoHeading"));
    heading->setFont(ExtensionListTitleFont());
    heading->setTextFormat(Qt::PlainText);
    head->addWidget(heading, 1);
    QToolButton *closer = new QToolButton(this);
    closer->setObjectName(QStringLiteral("SiteInfoClose"));
    closer->setIcon(PanelCloseIcon());
    closer->setToolTip(tr("Close"));
    closer->setAccessibleName(tr("Close"));
    closer->setAutoRaise(true);
    closer->setFocusPolicy(Qt::NoFocus);
    connect(closer, &QToolButton::clicked, this, &SiteInfoPanel::Dismiss);
    head->addWidget(closer);
    layout->addLayout(head);

    QStringList facts;
    switch(SiteInfo::ConnectionOf(m_Url)){
    case SiteInfo::Https:     facts << tr("Connected with HTTPS."); break;
    case SiteInfo::Http:      facts << tr("Connected with HTTP. The connection is not encrypted."); break;
    case SiteInfo::LocalFile: facts << tr("A file on this device."); break;
    default: break;
    }
    if(SiteInfo::HasSavedException(m_Url, Application::GetAllowedCertificates()))
        facts << tr("A certificate exception is saved for this host.");
    for(const QString &fact : std::as_const(facts)) words(fact)->setObjectName(QStringLiteral("SiteInfoConnection"));

    if(m_Origin.isEmpty()) return;

    line();
    QLabel *section = words(tr("Saved permissions (shared by every view)"));
    QFont bold = section->font();
    bold.setBold(true);
    section->setFont(bold);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("SiteInfoRows"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scroll->viewport()->setAutoFillBackground(false);
    QWidget *body = new QWidget;
    body->setObjectName(QStringLiteral("SiteInfoRowsBody"));
    QVBoxLayout *rows = new QVBoxLayout(body);
    rows->setContentsMargins(indent, 0, 0, 0);
    scroll->setWidget(body);
    body->setAutoFillBackground(false);
    layout->addWidget(scroll, 1);

    bool hasRows = false;
    for(const PermissionTable::Row &row : SitePermissions::Table().Rows()){
        if(row.origin != m_Origin) continue;
        hasRows = true;
        QHBoxLayout *entry = new QHBoxLayout;
        const QString label = VanillaPage::PermissionKindLabel(row.kind);
        const QString scope = VanillaPage::PermissionKindScope(row.kind);
        QString text = label.isEmpty() ? row.kind : label;
        if(scope == QStringLiteral("edge"))
            text += QStringLiteral(" (") + QCoreApplication::translate("SettingsPage", "Enabled in Edge") + QLatin1Char(')');
        else if(scope == QStringLiteral("engine"))
            text += QStringLiteral(" (") + QCoreApplication::translate("SettingsPage", "Enabled in WebEngine") + QLatin1Char(')');
        QLabel *name = new QLabel(text, body);
        name->setTextFormat(Qt::PlainText);
        name->setWordWrap(true);
        entry->addWidget(name, 1);

        const QString origin = m_Origin;
        const QString kind = row.kind;
        if(label.isEmpty()){
            QPushButton *remove = new QPushButton(tr("Remove"), body);
            remove->setObjectName(QStringLiteral("SiteInfoRemove_") + kind);
            connect(remove, &QPushButton::clicked, this, [origin, kind, remove](){
                SitePermissions::Set(origin, kind, PermissionTable::Ask);
                remove->setEnabled(false);
            });
            entry->addWidget(remove);
        } else {
            QComboBox *answer = new QComboBox(body);
            answer->setObjectName(QStringLiteral("SiteInfoState_") + kind);
            answer->addItem(tr("Allow"), int(PermissionTable::Allow));
            answer->addItem(tr("Block"), int(PermissionTable::Block));
            answer->addItem(tr("Ask"),   int(PermissionTable::Ask));
            answer->setCurrentIndex(answer->findData(int(row.state)));
            connect(answer, QOverload<int>::of(&QComboBox::activated), this, [origin, kind, answer](int index){
                SitePermissions::Set(origin, kind, PermissionTable::State(answer->itemData(index).toInt()));
            });
            entry->addWidget(answer);
        }
        rows->addLayout(entry);
    }
    if(!hasRows){
        QLabel *none = new QLabel(tr("No permission is saved for this site."), body);
        none->setObjectName(QStringLiteral("SiteInfoNone"));
        none->setWordWrap(true);
        rows->addWidget(none);
    }
    scroll->setMaximumHeight(body->sizeHint().height());

    BuildSiteData(layout);

    line();
    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->addStretch(1);
    QPushButton *settings = new QPushButton(tr("Site settings"), this);
    settings->setObjectName(QStringLiteral("SiteInfoSettings"));
    connect(settings, &QPushButton::clicked, this, [this](){
        const QString fragment = SettingsFragmentNow();
        QPointer<TreeBank> bank = m_TreeBank;
        close();
        if(bank) bank->OpenSettings(fragment);
    });
    bottom->addWidget(settings);
    layout->addLayout(bottom);
}

void SiteInfoPanel::BuildSiteData(QVBoxLayout *layout){
    const SiteInfo::Connection connection = SiteInfo::ConnectionOf(m_Url);
    if(!m_Link || (connection != SiteInfo::Https && connection != SiteInfo::Http)) return;

    layout->addWidget(new PanelRule(this));
    const int indent = ScaleByDevice(EXTENSION_PANEL_PADDING) / 2;

    QLabel *section = new QLabel(tr("Cookies and site data"), this);
    QFont bold = section->font();
    bold.setBold(true);
    section->setFont(bold);
    section->setContentsMargins(indent, 0, 0, 0);
    layout->addWidget(section);

    QHBoxLayout *row = new QHBoxLayout;
    row->setContentsMargins(indent, 0, 0, 0);
    m_CookieCount = new QLabel(this);
    m_CookieCount->setObjectName(QStringLiteral("SiteInfoCookies"));
    m_CookieCount->setTextFormat(Qt::PlainText);
    m_CookieCount->setWordWrap(true);
    row->addWidget(m_CookieCount, 1);
    m_DeleteData = new QPushButton(this);
    m_DeleteData->setObjectName(QStringLiteral("SiteInfoDeleteData"));
    m_DeleteData->setFocusPolicy(Qt::NoFocus);
    row->addWidget(m_DeleteData);
    layout->addLayout(row);

    QLabel *note = new QLabel(tr("Deleting also clears the site's storage and cache."), this);
    note->setObjectName(QStringLiteral("SiteInfoDataNote"));
    note->setWordWrap(true);
    note->setContentsMargins(indent, 0, 0, 0);
    MakePanelQuiet(note);
    layout->addWidget(note);

    m_Cookies = new SiteCookies(m_Link, m_Url, [this](){ ShowSiteData();}, this);
    connect(m_DeleteData, &QPushButton::clicked, this, [this](){ m_Cookies->Press();});
    ShowSiteData();
}

void SiteInfoPanel::ShowSiteData(){
    if(!m_Cookies || !m_CookieCount || !m_DeleteData) return;
    QString count;
    QString button = tr("Delete");
    bool enabled = false;
    switch(m_Cookies->GetState()){
    case SiteCookies::Counting:
        count = tr("Counting...");
        break;
    case SiteCookies::Unavailable:
        count = tr("Not available for this tab.");
        if(m_Cookies->PartlyFailed())
            count += QLatin1Char('\n') + tr("Some of it could not be deleted.");
        break;
    case SiteCookies::Deleting:
        count = tr("Deleting...");
        break;
    case SiteCookies::Confirming:
        button = tr("Press again to delete");
        enabled = true;
        Q_FALLTHROUGH();
    case SiteCookies::Counted:
        count = tr("%n cookie(s)", "", m_Cookies->Count());
        if(m_Cookies->PartlyFailed())
            count += QLatin1Char('\n') + tr("Some of it could not be deleted.");
        if(m_Cookies->GetState() == SiteCookies::Counted) enabled = true;
        break;
    }
    m_CookieCount->setText(count);
    m_DeleteData->setText(button);
    m_DeleteData->setEnabled(enabled);
    if(isVisible()){
        adjustSize();
        move(Place());
    }
}
