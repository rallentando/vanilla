#include "siteinfo.hpp"
#include "sitepermissions.hpp"
#include "permissiontable.hpp"
#include "testsupport.hpp"
#include "sitedatalink.hpp"

#include <QApplication>
#include <QComboBox>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QtTest/QtTest>

namespace {
    struct Asked {
        QString method;
        QJsonObject params;
        SiteDataLink::Answer done;
    };
    class FakeLink : public SiteDataLink {
    public:
        explicit FakeLink(QList<Asked> *asked) : m_Asked(asked){}
        void Call(const QString &method, const QJsonObject &params, Answer done) override {
            m_Asked->append(Asked{method, params, done});
        }
    private:
        QList<Asked> *m_Asked;
    };
    QJsonObject Cookie(const QString &name, const QString &domain, const QString &path){
        QJsonObject cookie;
        cookie.insert(QStringLiteral("name"), name);
        cookie.insert(QStringLiteral("domain"), domain);
        cookie.insert(QStringLiteral("path"), path);
        return cookie;
    }
    QJsonObject Cookies(const QList<QJsonObject> &list){
        QJsonArray array;
        for(const QJsonObject &cookie : list) array.append(cookie);
        QJsonObject result;
        result.insert(QStringLiteral("cookies"), array);
        return result;
    }
}

class tst_siteinfo : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        SitePermissions::Clear();
    }
    void cleanupTestCase(){
        SitePermissions::Clear();
    }

    void theAddressSaysHowItWasReached_data(){
        QTest::addColumn<QString>("url");
        QTest::addColumn<int>("connection");
        QTest::newRow("https")     << "https://example.com/a"     << int(SiteInfo::Https);
        QTest::newRow("http")      << "http://example.com/"       << int(SiteInfo::Http);
        QTest::newRow("file")      << "file:///C:/x.html"         << int(SiteInfo::LocalFile);
        QTest::newRow("vanilla")   << "vanilla://settings"        << int(SiteInfo::Internal);
        QTest::newRow("about")     << "about:blank"               << int(SiteInfo::Internal);
        QTest::newRow("edge")      << "edge://version"            << int(SiteInfo::Internal);
        QTest::newRow("data")      << "data:text/plain,x"         << int(SiteInfo::Other);
    }
    void theAddressSaysHowItWasReached(){
        QFETCH(QString, url);
        QFETCH(int, connection);
        QCOMPARE(int(SiteInfo::ConnectionOf(QUrl(url))), connection);
    }

    void aSavedExceptionIsTheHostsOnly(){
        const QStringList allowed{ QStringLiteral("example.com:ABCDEF"), QStringLiteral("other.org:123") };
        QVERIFY(SiteInfo::HasSavedException(QUrl(QStringLiteral("https://example.com/x")), allowed));
        QVERIFY(SiteInfo::HasSavedException(QUrl(QStringLiteral("https://EXAMPLE.com/")), allowed));
        QVERIFY(!SiteInfo::HasSavedException(QUrl(QStringLiteral("https://example.co/")), allowed));
        QVERIFY(!SiteInfo::HasSavedException(QUrl(QStringLiteral("https://sub.example.com/")), allowed));
        QVERIFY(!SiteInfo::HasSavedException(QUrl(QStringLiteral("http://example.com/")), allowed));
    }

    void theHeadingAndTheWayToTheSettings(){
        QCOMPARE(SiteInfo::Heading(QUrl(QStringLiteral("https://example.com/a?b"))), QStringLiteral("example.com"));
        QVERIFY(!SiteInfo::Heading(QUrl(QStringLiteral("file:///C:/x"))).isEmpty());
        QVERIFY(!SiteInfo::Heading(QUrl(QStringLiteral("about:blank"))).isEmpty());
        QCOMPARE(SiteInfo::SettingsFragment(QStringLiteral("https://example.com"), true),
                 QStringLiteral("permissions?find=https%3A%2F%2Fexample.com"));
        QCOMPARE(SiteInfo::SettingsFragment(QStringLiteral("https://example.com"), false),
                 QStringLiteral("permissions"));
    }

    void theChoicesWriteTheOriginItWasOpenedFor(){
        QVERIFY(SitePermissions::Set(QStringLiteral("https://example.com"), QStringLiteral("notifications"),
                                     PermissionTable::Allow));
        QVERIFY(SitePermissions::Set(QStringLiteral("https://elsewhere.com"), QStringLiteral("camera"),
                                     PermissionTable::Block));
        QWidget anchor;
        anchor.setGeometry(100, 100, 30, 30);
        anchor.show();

        SiteInfoPanel *panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://example.com/page")));
        QCOMPARE(panel->Origin(), QStringLiteral("https://example.com"));
        QComboBox *notifications = panel->findChild<QComboBox*>(QStringLiteral("SiteInfoState_notifications"));
        QVERIFY(notifications);
        QVERIFY(!panel->findChild<QComboBox*>(QStringLiteral("SiteInfoState_camera")));
        QCOMPARE(PermissionTable::State(notifications->currentData().toInt()), PermissionTable::Allow);
        QVERIFY(!panel->findChild<QLabel*>(QStringLiteral("SiteInfoNone")));
        QVERIFY(panel->findChild<QPushButton*>(QStringLiteral("SiteInfoSettings")));
        QVERIFY((panel->pos() - anchor.mapToGlobal(QPoint(0, anchor.height()))).manhattanLength() <= 6);

        emit notifications->activated(notifications->findData(int(PermissionTable::Block)));
        QCOMPARE(SitePermissions::Lookup(QUrl(QStringLiteral("https://example.com")), QStringLiteral("notifications")),
                 PermissionTable::Block);
        QCOMPARE(SitePermissions::Lookup(QUrl(QStringLiteral("https://elsewhere.com")), QStringLiteral("camera")),
                 PermissionTable::Block);
        QCOMPARE(panel->SettingsFragmentNow(), QStringLiteral("permissions?find=https%3A%2F%2Fexample.com"));
        emit notifications->activated(notifications->findData(int(PermissionTable::Ask)));
        QCOMPARE(SitePermissions::Lookup(QUrl(QStringLiteral("https://example.com")), QStringLiteral("notifications")),
                 PermissionTable::Ask);
        QCOMPARE(panel->SettingsFragmentNow(), QStringLiteral("permissions"));
        panel->close();

        panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://example.com/page")));
        QVERIFY(panel->findChild<QLabel*>(QStringLiteral("SiteInfoNone")));
        QWidget *body = panel->findChild<QWidget*>(QStringLiteral("SiteInfoRowsBody"));
        QVERIFY(body && !body->autoFillBackground());
        QVERIFY(!panel->findChild<QScrollArea*>(QStringLiteral("SiteInfoRows"))->viewport()->autoFillBackground());
        panel->close();

        {
            for(const QString &kind : PermissionTable::Kinds())
                SitePermissions::Set(QStringLiteral("https://many.example"), kind, PermissionTable::Block);
            QPointer<SiteInfoPanel> many = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://many.example/")));
            const QRect screen = anchor.screen()->availableGeometry();
            QVERIFY(many->height() <= screen.height());
            QPushButton *settings = many->findChild<QPushButton*>(QStringLiteral("SiteInfoSettings"));
            QVERIFY(settings);
            const QRect button(settings->mapToGlobal(QPoint(0, 0)), settings->size());
            QVERIFY(screen.contains(button));
            many->close();
            SitePermissions::Clear();
        }

        {
            QPointer<SiteInfoPanel> open = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://example.com/")));
            QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(open.data()));
            QTest::keyClick(open.data(), Qt::Key_Escape);
            QTRY_VERIFY(!open);
        }

        panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("file:///C:/x.html")));
        QVERIFY(panel->Origin().isEmpty());
        QVERIFY(!panel->findChild<QPushButton*>(QStringLiteral("SiteInfoSettings")));
        QVERIFY(!panel->findChild<QLabel*>(QStringLiteral("SiteInfoNone")));
        panel->close();
        SitePermissions::Clear();
    }

    void theWidgetsTakeThePanelsAccentUnderAStyleSheet(){
        SitePermissions::Set(QStringLiteral("https://example.com"), QStringLiteral("notifications"),
                             PermissionTable::Allow);
        QWidget bar;
        bar.setStyleSheet(QStringLiteral("QToolBar{ padding: 2px;}"));
        QWidget *anchor = new QWidget(&bar);
        anchor->setGeometry(0, 0, 30, 30);
        bar.setGeometry(100, 100, 200, 40);
        bar.show();

        QPointer<SiteInfoPanel> panel = SiteInfoPanel::Open(anchor, nullptr, QUrl(QStringLiteral("https://example.com/page")));
        const QColor accent = Theme::Color(Theme::VanillaPageAccent);
        for(const QString &name : { QStringLiteral("SiteInfoState_notifications"),
                                    QStringLiteral("SiteInfoSettings"), QStringLiteral("SiteInfoClose") }){
            QWidget *widget = panel->findChild<QWidget*>(name);
            QVERIFY2(widget, qPrintable(name));
            QCOMPARE(widget->palette().color(QPalette::Accent), accent);
            QCOMPARE(widget->palette().color(QPalette::Highlight), accent);
            QVERIFY(!widget->palette().isBrushSet(QPalette::Active, QPalette::Button));
        }
        panel->close();
        SitePermissions::Clear();
    }

    void theHostSeesItsOwnAndItsDomainsCookies(){
        QVERIFY(SiteCookies::HostSees(QStringLiteral("www.example.com"), QStringLiteral("www.example.com")));
        QVERIFY(SiteCookies::HostSees(QStringLiteral("www.example.com"), QStringLiteral(".example.com")));
        QVERIFY(SiteCookies::HostSees(QStringLiteral("example.com"), QStringLiteral(".example.com")));
        QVERIFY(SiteCookies::HostSees(QStringLiteral("WWW.Example.com"), QStringLiteral(".EXAMPLE.com")));
        QVERIFY(!SiteCookies::HostSees(QStringLiteral("www.example.com"), QStringLiteral("example.com")));
        QVERIFY(!SiteCookies::HostSees(QStringLiteral("www.example.com"), QStringLiteral("mail.example.com")));
        QVERIFY(!SiteCookies::HostSees(QStringLiteral("badexample.com"), QStringLiteral(".example.com")));
        QVERIFY(!SiteCookies::HostSees(QStringLiteral("www.example.com"), QString()));
        QVERIFY(!SiteCookies::ClearedStorageTypes().split(QLatin1Char(',')).contains(QStringLiteral("cookies")));
        QVERIFY(SiteCookies::ClearedStorageTypes().split(QLatin1Char(',')).contains(QStringLiteral("local_storage")));
    }

    void theSiteDataIsCountedAndDeletedOnTheSecondPress(){
        QWidget anchor;
        anchor.setGeometry(100, 100, 30, 30);
        anchor.show();
        QList<Asked> asked;
        QPointer<SiteInfoPanel> panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://www.example.com/a")),
                                                            new FakeLink(&asked));
        QLabel *count = panel->findChild<QLabel*>(QStringLiteral("SiteInfoCookies"));
        QPushButton *button = panel->findChild<QPushButton*>(QStringLiteral("SiteInfoDeleteData"));
        QVERIFY(count && button);
        QVERIFY(!button->isEnabled());
        QCOMPARE(asked.length(), 1);
        QCOMPARE(asked.at(0).method, QStringLiteral("Network.getAllCookies"));

        QJsonObject partitioned = Cookie(QStringLiteral("c"), QStringLiteral("www.example.com"), QStringLiteral("/"));
        QJsonObject key;
        key.insert(QStringLiteral("topLevelSite"), QStringLiteral("https://top.test"));
        key.insert(QStringLiteral("hasCrossSiteAncestor"), true);
        partitioned.insert(QStringLiteral("partitionKey"), key);
        QJsonObject opaque = Cookie(QStringLiteral("d"), QStringLiteral("www.example.com"), QStringLiteral("/"));
        opaque.insert(QStringLiteral("partitionKeyOpaque"), true);
        asked.at(0).done(true, Cookies({
            Cookie(QStringLiteral("a"), QStringLiteral("www.example.com"), QStringLiteral("/")),
            Cookie(QStringLiteral("b"), QStringLiteral(".example.com"), QStringLiteral("/account")),
            partitioned, opaque,
            Cookie(QStringLiteral("e"), QStringLiteral("other.com"), QStringLiteral("/")),
            Cookie(QStringLiteral("f"), QStringLiteral("example.com"), QStringLiteral("/")) }));
        QCOMPARE(panel->m_Cookies->GetState(), SiteCookies::Counted);
        QCOMPARE(panel->m_Cookies->Count(), 4);
        QVERIFY(count->text().contains(QLatin1Char('4')));
        QVERIFY(button->isEnabled());

        button->click();
        QCOMPARE(panel->m_Cookies->GetState(), SiteCookies::Confirming);
        QCOMPARE(asked.length(), 1);

        button->click();
        QCOMPARE(panel->m_Cookies->GetState(), SiteCookies::Deleting);
        QVERIFY(!button->isEnabled());
        QCOMPARE(asked.length(), 2);
        QCOMPARE(asked.at(1).method, QStringLiteral("Network.deleteCookies"));
        QCOMPARE(asked.at(1).params.value(QStringLiteral("name")).toString(), QStringLiteral("a"));
        QCOMPARE(asked.at(1).params.value(QStringLiteral("domain")).toString(), QStringLiteral("www.example.com"));
        QVERIFY(!asked.at(1).params.contains(QStringLiteral("partitionKey")));
        asked.at(1).done(true, QJsonObject());
        QCOMPARE(asked.at(2).params.value(QStringLiteral("name")).toString(), QStringLiteral("b"));
        QCOMPARE(asked.at(2).params.value(QStringLiteral("domain")).toString(), QStringLiteral(".example.com"));
        QCOMPARE(asked.at(2).params.value(QStringLiteral("path")).toString(), QStringLiteral("/account"));
        asked.at(2).done(true, QJsonObject());
        QCOMPARE(asked.at(3).params.value(QStringLiteral("name")).toString(), QStringLiteral("c"));
        QCOMPARE(asked.at(3).params.value(QStringLiteral("partitionKey")).toObject(), key);
        asked.at(3).done(true, QJsonObject());
        QCOMPARE(asked.length(), 5);
        QCOMPARE(asked.at(4).method, QStringLiteral("Storage.clearDataForOrigin"));
        QCOMPARE(asked.at(4).params.value(QStringLiteral("origin")).toString(), QStringLiteral("https://www.example.com"));
        QCOMPARE(asked.at(4).params.value(QStringLiteral("storageTypes")).toString(), SiteCookies::ClearedStorageTypes());
        asked.at(4).done(true, QJsonObject());
        QCOMPARE(asked.length(), 6);
        QCOMPARE(asked.at(5).method, QStringLiteral("Network.getAllCookies"));
        asked.at(5).done(true, Cookies({ opaque }));
        QCOMPARE(panel->m_Cookies->Count(), 1);
        QVERIFY(!panel->m_Cookies->PartlyFailed());

        button->click();
        button->click();
        QCOMPARE(asked.length(), 7);
        QCOMPARE(asked.at(6).method, QStringLiteral("Storage.clearDataForOrigin"));
        asked.at(6).done(false, QJsonObject());
        QCOMPARE(asked.at(7).method, QStringLiteral("Network.getAllCookies"));
        asked.at(7).done(true, Cookies({ Cookie(QStringLiteral("a"), QStringLiteral("www.example.com"), QStringLiteral("/")),
                                         Cookie(QStringLiteral("g"), QStringLiteral("www.example.com"), QStringLiteral("/")) }));
        QVERIFY(panel->m_Cookies->PartlyFailed());
        button->click();
        button->click();
        QCOMPARE(asked.at(8).method, QStringLiteral("Network.deleteCookies"));
        asked.at(8).done(false, QJsonObject());
        QCOMPARE(asked.length(), 10);
        QCOMPARE(asked.at(9).method, QStringLiteral("Network.getAllCookies"));
        QVERIFY(panel->m_Cookies->PartlyFailed());

        asked.at(9).done(true, Cookies({ Cookie(QStringLiteral("a"), QStringLiteral("www.example.com"), QStringLiteral("/")) }));
        button->click();
        button->click();
        QCOMPARE(asked.at(10).method, QStringLiteral("Network.deleteCookies"));
        asked.at(10).done(false, QJsonObject());
        QCOMPARE(asked.at(11).method, QStringLiteral("Network.getAllCookies"));
        asked.at(11).done(false, QJsonObject());
        QCOMPARE(panel->m_Cookies->GetState(), SiteCookies::Unavailable);
        QVERIFY(count->text().contains(QCoreApplication::translate("SiteInfoPanel", "Some of it could not be deleted.")));

        panel->m_Cookies->Recount();
        asked.at(12).done(true, Cookies({ Cookie(QStringLiteral("a"), QStringLiteral("www.example.com"), QStringLiteral("/")),
                                          Cookie(QStringLiteral("b"), QStringLiteral("www.example.com"), QStringLiteral("/")) }));
        button->click();
        button->click();
        QCOMPARE(asked.length(), 14);
        QCOMPARE(asked.at(13).method, QStringLiteral("Network.deleteCookies"));
        panel->close();
        QVERIFY(panel);
        asked.at(13).done(true, QJsonObject());
        QCOMPARE(asked.length(), 14);
        QTRY_VERIFY(!panel);

        asked.clear();
        panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://www.example.com/a")), new FakeLink(&asked));
        QTRY_COMPARE_WITH_TIMEOUT(panel->m_Cookies->GetState(), SiteCookies::Unavailable, SiteDataLink::TIMEOUT_MSEC + 2000);
        QVERIFY(!panel->findChild<QPushButton*>(QStringLiteral("SiteInfoDeleteData"))->isEnabled());
        asked.at(0).done(true, Cookies({ Cookie(QStringLiteral("a"), QStringLiteral("www.example.com"), QStringLiteral("/")) }));
        QCOMPARE(panel->m_Cookies->GetState(), SiteCookies::Unavailable);
        panel->close();

        asked.clear();
        panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://bücher.de/")), new FakeLink(&asked));
        asked.at(0).done(true, Cookies({ Cookie(QStringLiteral("a"), QStringLiteral("xn--bcher-kva.de"), QStringLiteral("/")),
                                         Cookie(QStringLiteral("b"), QStringLiteral(".xn--bcher-kva.de"), QStringLiteral("/")) }));
        QCOMPARE(panel->m_Cookies->Count(), 2);
        panel->close();

        panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("https://www.example.com/a")));
        QVERIFY(!panel->findChild<QPushButton*>(QStringLiteral("SiteInfoDeleteData")));
        panel->close();
        asked.clear();
        panel = SiteInfoPanel::Open(&anchor, nullptr, QUrl(QStringLiteral("file:///C:/x.html")), new FakeLink(&asked));
        QVERIFY(!panel->findChild<QPushButton*>(QStringLiteral("SiteInfoDeleteData")));
        QVERIFY(asked.isEmpty());
        panel->close();
    }
};

QTEST_MAIN(tst_siteinfo)
#include "tst_siteinfo.moc"
