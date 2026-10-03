#include "switch.hpp"

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "extensionui.hpp"

#include <QRegularExpression>

#include "testsupport.hpp"

class tst_extensionui : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void aTabsValuesLieOverTheExtensionsOwn();
    void aSidePanelsOptionsAreTheExtensionsAndEachTabs();
    void aNotificationIsTakenInChromesWordsOrRefusedInThem();
    void aNotificationsAnswerCountsOnlyWhileItIsTheOneOnShow();
    void aTabsValuesGoWithTheAddressTheyWereSetAt();
    void theTabsKeptAreBounded();
    void aColourIsFoldedToOneSpelling();
    void aPopupsSizeIsReadInTheWidgetsPixels();
    void whatIsReadIsTheEffectiveValue();
    void anIconIsNamedByOnePathOrTheLargestSize();
    void anIconSentIsAPngOfBoundedLength();

    void anItemIsMadeInChromesWordsOrRefusedInThem();
    void anItemGoesUnderItsParentAndOutWithIt();
    void theButtonsOwnItemsAreItsMenu();
    void theButtonsMenuIsChosenByContextsAlone();
    void openPopupIsRefusedWhereThePressWouldDoNothing();
    void whatIsShownIsWhatTheContextNames();
    void thePatternsChooseByAddress();
    void aCheckboxFlipsAndARadioTurnsItsSiblingsOff();
    void theRadiosAreKeptAsChromeKeepsThem();
    void theRegistryComesBackFromItsJson();
    void theInfoOfAClickSaysWhereItWas();
    void aLinkedImageIsMatchedByWhatTheItemSpeaksOf();
    void whatIsBoundedIsRefusedPastTheBound();
    void aTabIdIsAWholeNumberAndAFileIsNamedByADigest();
    void aDownloadIsExpectedThenKnownByItsAddress();
    void whatOnInstalledIsOwedIsDecidedOnceAndKeptUntilTaken();
    void theLedgerComesBackFromItsJson();
    void theAddressOfADownloadIsABlobOrData();
    void aDownloadIsPausedErasedAndNamedInChromesWords();
    void aShortcutIsReadAsChromeReadsIt_data();
    void aShortcutIsReadAsChromeReadsIt();
    void aKeyPressedIsSpelledAsChromeSpellsIt();

private:
    static QJsonObject Obj(const char *json){
        return QJsonDocument::fromJson(json).object();
    }
    static ExtensionUi::TabNow Tab(qint64 id, const char *url = "https://a.example/"){
        ExtensionUi::TabNow tab;
        tab.id = id;
        tab.url = QUrl(QString::fromLatin1(url));
        return tab;
    }
    static QString Json(const QJsonObject &o){ return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)); }
    static QStringList Titles(const QList<ExtensionUi::MenuShown> &shown){
        QStringList out;
        foreach(const ExtensionUi::MenuShown &one, shown){
            out << one.title;
            foreach(const QString &child, Titles(one.children)) out << QStringLiteral("  ") + child;
        }
        return out;
    }
};

void tst_extensionui::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_extensionui::aSidePanelsOptionsAreTheExtensionsAndEachTabs(){
    using ExtensionUi::SidePanel;
    SidePanel panel;
    const SidePanel::Resolve resolve = [](const QString &path){
        return path == QStringLiteral("side.html") || path == QStringLiteral("other.html")
            ? QUrl(QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/") + path) : QUrl();
    };
    auto args = [](const QByteArray &json){ return QJsonDocument::fromJson("[" + json + "]").array(); };
    auto value = [](const QJsonObject &reply){ return QString::fromUtf8(QJsonDocument(reply.value(QStringLiteral("value")).toObject()).toJson(QJsonDocument::Compact)); };
    const QString fallback = QStringLiteral("side.html");
    QCOMPARE(value(panel.GetOptions(args("{}"), fallback)), QStringLiteral("{\"enabled\":true,\"path\":\"side.html\"}"));
    QCOMPARE(value(panel.GetOptions(args("{}"), QString())), QStringLiteral("{\"enabled\":true}"));
    QCOMPARE(panel.Url(fallback, resolve), QUrl(QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/side.html")));
    QCOMPARE(panel.Url(QString(), resolve), QUrl());
    QVERIFY(panel.SetOptions(args("{\"path\":\"other.html\"}"), resolve).value(QStringLiteral("ok")).toBool());
    QCOMPARE(panel.Path(fallback), QStringLiteral("other.html"));
    QCOMPARE(panel.Url(fallback, resolve).path(), QStringLiteral("/other.html"));
    QVERIFY(panel.SetOptions(args("{\"enabled\":false}"), resolve).value(QStringLiteral("ok")).toBool());
    QCOMPARE(panel.Url(fallback, resolve), QUrl());
    QCOMPARE(value(panel.GetOptions(args("{}"), fallback)), QStringLiteral("{\"enabled\":false,\"path\":\"other.html\"}"));
    QVERIFY(panel.SetOptions(args("{\"enabled\":true}"), resolve).value(QStringLiteral("ok")).toBool());
    for(const QByteArray &bad : { QByteArray("{\"path\":\"https://a.example/\"}"), QByteArray("{\"path\":\"\"}"),
                                  QByteArray("{\"path\":\"missing.html\",\"enabled\":false}") }){
        const QJsonObject reply = panel.SetOptions(args(bad), resolve);
        QVERIFY2(!reply.value(QStringLiteral("ok")).toBool(), bad.constData());
        QCOMPARE(reply.value(QStringLiteral("error")).toString(), QStringLiteral("Invalid path."));
    }
    QVERIFY(panel.Enabled());
    for(const QByteArray &bad : { QByteArray("{\"path\":1}"), QByteArray("{\"enabled\":\"no\"}"), QByteArray("{\"tabId\":\"3\"}"),
                                  QByteArray("{\"tabId\":-1}"), QByteArray("{\"tabId\":1.5}"), QByteArray("1") })
        QVERIFY2(panel.SetOptions(args(bad), resolve).value(QStringLiteral("error")).toString().startsWith(QStringLiteral("Error in invocation of sidePanel.setOptions(")), bad.constData());
    QVERIFY(!panel.SetOptions(QJsonArray(), resolve).value(QStringLiteral("ok")).toBool());
    QVERIFY(panel.SetOptions(args("{\"tabId\":7,\"enabled\":false}"), resolve).value(QStringLiteral("ok")).toBool());
    QCOMPARE(value(panel.GetOptions(args("{\"tabId\":7}"), fallback)), QStringLiteral("{\"enabled\":false,\"path\":\"other.html\",\"tabId\":7}"));
    QCOMPARE(value(panel.GetOptions(args("{\"tabId\":8}"), fallback)), QStringLiteral("{\"enabled\":true,\"path\":\"other.html\",\"tabId\":8}"));
    QVERIFY(panel.SetOptions(args("{\"tabId\":8,\"path\":\"side.html\"}"), resolve).value(QStringLiteral("ok")).toBool());
    QCOMPARE(value(panel.GetOptions(args("{\"tabId\":8}"), fallback)), QStringLiteral("{\"enabled\":true,\"path\":\"side.html\",\"tabId\":8}"));
    QCOMPARE(panel.Path(fallback), QStringLiteral("other.html"));
    QVERIFY(panel.Enabled());
    QCOMPARE(value(panel.GetBehavior()), QStringLiteral("{\"openPanelOnActionClick\":false}"));
    QVERIFY(panel.SetBehavior(args("{\"openPanelOnActionClick\":true}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(panel.OpensOnAction());
    QVERIFY(panel.SetBehavior(args("{}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(panel.OpensOnAction());
    QVERIFY(!panel.SetBehavior(args("{\"openPanelOnActionClick\":1}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(panel.OpensOnAction());

    QCOMPARE(panel.TabPath(8), QStringLiteral("side.html"));
    QCOMPARE(panel.TabPath(7), QString());
    QCOMPARE(panel.TabPath(9), QString());
    QVERIFY(!panel.EnabledFor(7));
    QVERIFY(panel.EnabledFor(8));
    QVERIFY(panel.EnabledFor(9));
    QCOMPARE(panel.TabUrl(8, resolve).path(), QStringLiteral("/side.html"));
    QCOMPARE(panel.TabUrl(9, resolve), QUrl());
    QCOMPARE(panel.TabUrl(7, resolve), QUrl());
    QVERIFY(panel.SetOptions(args("{\"tabId\":8,\"enabled\":false}"), resolve).value(QStringLiteral("ok")).toBool());
    QCOMPARE(panel.TabUrl(8, resolve), QUrl());
    QVERIFY(panel.SetOptions(args("{\"enabled\":false}"), resolve).value(QStringLiteral("ok")).toBool());
    QVERIFY(!panel.EnabledFor(9));
    QVERIFY(panel.SetOptions(args("{\"tabId\":9,\"enabled\":true}"), resolve).value(QStringLiteral("ok")).toBool());
    QVERIFY(panel.EnabledFor(9));
}

void tst_extensionui::aTabsValuesLieOverTheExtensionsOwn(){
    ExtensionUi::Action action;
    ExtensionUi::ActionShown shown = action.Shown(Tab(5));
    QVERIFY(!shown.hasIcon && !shown.hasTitle && shown.badgeText.isEmpty() && shown.enabled);

    action.SetTitle(Tab(0), QStringLiteral("all"));
    action.SetBadgeText(Tab(0), QStringLiteral("12"));
    action.SetBadgeText(Tab(5), QStringLiteral("5"));
    action.SetEnabled(Tab(5), false);
    ExtensionUi::Icon icon; icon.bytes = "PNG-ISH";
    action.SetIcon(Tab(0), icon);

    shown = action.Shown(Tab(5));
    QCOMPARE(shown.title, QStringLiteral("all"));
    QCOMPARE(shown.badgeText, QStringLiteral("5"));
    QVERIFY(!shown.enabled);
    QVERIFY(shown.hasIcon);
    QCOMPARE(shown.icon.bytes, QByteArray("PNG-ISH"));
    shown = action.Shown(Tab(6));
    QCOMPARE(shown.badgeText, QStringLiteral("12"));
    QVERIFY(shown.enabled);
    QCOMPARE(action.ShownForAll().badgeText, QStringLiteral("12"));
}

void tst_extensionui::aTabsValuesGoWithTheAddressTheyWereSetAt(){
    ExtensionUi::Action action;
    action.SetBadgeText(Tab(5, "https://a.example/one"), QStringLiteral("one"));
    QCOMPARE(action.Shown(Tab(5, "https://a.example/one")).badgeText, QStringLiteral("one"));
    QCOMPARE(action.Shown(Tab(5, "https://a.example/two")).badgeText, QString());
    QVERIFY(action.OfTab(Tab(5, "https://a.example/two")) == nullptr);
    QCOMPARE(action.Shown(Tab(5, "https://a.example/one")).badgeText, QString());
    QCOMPARE(action.TabsKept(), 0);
    action.SetBadgeText(Tab(5, "https://a.example/one"), QStringLiteral("one"));
    action.SetTitle(Tab(5, "https://a.example/two"), QStringLiteral("t"));
    QCOMPARE(action.Shown(Tab(5, "https://a.example/two")).badgeText, QString());
    QCOMPARE(action.Shown(Tab(5, "https://a.example/two")).title, QStringLiteral("t"));
    QCOMPARE(action.Shown(Tab(5, "https://a.example/one")).badgeText, QString());
    QCOMPARE(action.Shown(Tab(5, "https://a.example/two")).title, QString());
    QCOMPARE(action.TabsKept(), 0);
    action.SetBadgeText(Tab(0), QStringLiteral("all"));
    action.SetBadgeText(Tab(6), QStringLiteral("six"));
    action.SetTitle(Tab(6), QStringLiteral("six"));
    action.UnsetBadgeText(Tab(6));
    action.UnsetTitle(Tab(6));
    QCOMPARE(action.Shown(Tab(6)).badgeText, QStringLiteral("all"));
    QVERIFY(!action.Shown(Tab(6)).hasTitle);
}

void tst_extensionui::theTabsKeptAreBounded(){
    ExtensionUi::Action action;
    for(int i = 1; i <= ExtensionUi::ACTION_TABS_KEPT + 3; i++) action.SetBadgeText(Tab(i), QString::number(i));
    QCOMPARE(action.TabsKept(), ExtensionUi::ACTION_TABS_KEPT);
    QCOMPARE(action.Shown(Tab(1)).badgeText, QString());
    QCOMPARE(action.Shown(Tab(3)).badgeText, QString());
    QCOMPARE(action.Shown(Tab(4)).badgeText, QStringLiteral("4"));
    QCOMPARE(action.Shown(Tab(ExtensionUi::ACTION_TABS_KEPT + 3)).badgeText, QString::number(ExtensionUi::ACTION_TABS_KEPT + 3));
}

void tst_extensionui::aPopupsSizeIsReadInTheWidgetsPixels(){
    using ExtensionUi::PopupSizeOf;
    QCOMPARE(PopupSizeOf(QStringLiteral("\"342,469\""), 1.0, 1.25), QSize(342, 469));
    QCOMPARE(PopupSizeOf(QStringLiteral("342,469"), 1.0, 1.25), QSize(342, 469));
    QCOMPARE(PopupSizeOf(QStringLiteral("342,469"), 2.0, 1.25), QSize(171, 235));
    QCOMPARE(PopupSizeOf(QStringLiteral("4000,4000"), 1.0, 1.25), QSize(1000, 750));
    QCOMPARE(PopupSizeOf(QStringLiteral("1000,750"), 1.0, 1.25), QSize(1000, 750));
    QVERIFY(!PopupSizeOf(QStringLiteral("3,4"), 1.0, 0).isValid());
    foreach(const QString &bad, QStringList() << QStringLiteral("null") << QString() << QStringLiteral("0,5")
                                               << QStringLiteral("5,0") << QStringLiteral("4001,5") << QStringLiteral("1,2,3")
                                               << QStringLiteral("-3,4") << QStringLiteral("3.5,4") << QStringLiteral("\"3,4\" "))
        QVERIFY2(!PopupSizeOf(bad, 1.0, 1.0).isValid(), qPrintable(bad));
    QVERIFY(!PopupSizeOf(QStringLiteral("3,4"), 0, 1.0).isValid());
}

void tst_extensionui::aColourIsFoldedToOneSpelling(){
    QCOMPARE(ExtensionUi::ColorOf(QJsonValue(QStringLiteral("#E96C4C"))), QStringLiteral("#e96c4cff"));
    QCOMPARE(ExtensionUi::ColorOf(QJsonValue(QStringLiteral("#abc"))), QStringLiteral("#aabbccff"));
    QCOMPARE(ExtensionUi::ColorOf(QJsonValue(QStringLiteral("#11223344"))), QStringLiteral("#11223344"));
    QCOMPARE(ExtensionUi::ColorOf(QJsonArray() << 255 << 0 << 16 << 128), QStringLiteral("#ff001080"));
    QCOMPARE(ExtensionUi::ColorOf(QJsonValue(QStringLiteral("red"))), QString());
    QCOMPARE(ExtensionUi::ColorOf(QJsonArray() << 255 << 0 << 16), QString());
    QCOMPARE(ExtensionUi::ColorOf(QJsonArray() << 256 << 0 << 0 << 0), QString());
    QCOMPARE(ExtensionUi::ColorOf(QJsonValue(5)), QString());
}

void tst_extensionui::whatIsReadIsTheEffectiveValue(){
    ExtensionUi::Action action;
    action.SetBadgeColor(Tab(0), ExtensionUi::ColorOf(QJsonValue(QStringLiteral("#e96c4c"))));
    action.SetBadgeText(Tab(7), QStringLiteral("7"));
    action.SetEnabled(Tab(0), false);
    const ExtensionUi::TabNow tab = Tab(7);
    QCOMPARE(Json(ExtensionUi::GetOfAction(QStringLiteral("action.getBadgeText"), action, &tab, QString())), QStringLiteral("{\"ok\":true,\"value\":\"7\"}"));
    QCOMPARE(Json(ExtensionUi::GetOfAction(QStringLiteral("action.getBadgeText"), action, nullptr, QString())), QStringLiteral("{\"ok\":true,\"value\":\"\"}"));
    QCOMPARE(Json(ExtensionUi::GetOfAction(QStringLiteral("action.getTitle"), action, &tab, QStringLiteral("Manifest"))), QStringLiteral("{\"ok\":true,\"value\":\"Manifest\"}"));
    QCOMPARE(Json(ExtensionUi::GetOfAction(QStringLiteral("action.getBadgeBackgroundColor"), action, &tab, QString())), QStringLiteral("{\"ok\":true,\"value\":[233,108,76,255]}"));
    QCOMPARE(Json(ExtensionUi::GetOfAction(QStringLiteral("action.getBadgeTextColor"), action, &tab, QString())), QStringLiteral("{\"ok\":true,\"value\":[0,0,0,0]}"));
    QCOMPARE(Json(ExtensionUi::GetOfAction(QStringLiteral("action.isEnabled"), action, &tab, QString())), QStringLiteral("{\"ok\":true,\"value\":false}"));
    QVERIFY(!ExtensionUi::GetOfAction(QStringLiteral("action.getPopup"), action, &tab, QString()).value(QStringLiteral("ok")).toBool());
}

void tst_extensionui::anIconIsNamedByOnePathOrTheLargestSize(){
    QCOMPARE(ExtensionUi::IconPathOf(QJsonValue(QStringLiteral("icons/on.png"))), QStringLiteral("icons/on.png"));
    QCOMPARE(ExtensionUi::IconPathOf(Obj("{\"16\": \"a16.png\", \"32\": \"a32.png\", \"512\": \"huge.png\"}")), QStringLiteral("a32.png"));
    QCOMPARE(ExtensionUi::IconPathOf(Obj("{\"x\": \"a.png\", \"32\": 5}")), QString());
    QCOMPARE(ExtensionUi::IconPathOf(QJsonValue(5)), QString());
}

void tst_extensionui::anIconSentIsAPngOfBoundedLength(){
    const QByteArray png = QByteArray("\x89PNG\r\n\x1a\n", 8) + QByteArray(40, 'x');
    QJsonObject sent;
    sent[QStringLiteral("png")] = QString::fromLatin1(png.toBase64());
    QCOMPARE(ExtensionUi::IconBytesOf(sent), png);
    sent[QStringLiteral("png")] = QString::fromLatin1(QByteArray("GIF89a").toBase64());
    QVERIFY(ExtensionUi::IconBytesOf(sent).isEmpty());
    sent[QStringLiteral("png")] = QString::fromLatin1((png + QByteArray(ExtensionUi::ICON_SENT_LIMIT, 'y')).toBase64());
    QVERIFY(ExtensionUi::IconBytesOf(sent).isEmpty());
    sent[QStringLiteral("png")] = QStringLiteral("not base64 !!");
    QVERIFY(ExtensionUi::IconBytesOf(sent).isEmpty());
    QVERIFY(ExtensionUi::IconBytesOf(QJsonValue(QStringLiteral("x"))).isEmpty());
}

void tst_extensionui::anItemIsMadeInChromesWordsOrRefusedInThem(){
    ExtensionUi::Menus menus;
    QCOMPARE(Json(menus.Create(Obj("{\"id\": \"top\", \"title\": \"Dark Reader\"}"))), QStringLiteral("{\"ok\":true,\"value\":\"top\"}"));
    QCOMPARE(Json(menus.Create(Obj("{\"id\": 7, \"title\": \"seven\"}"))), QStringLiteral("{\"ok\":true,\"value\":\"7\"}"));
    QCOMPARE(menus.Create(Obj("{\"id\": \"top\", \"title\": \"again\"}")).value(QStringLiteral("error")).toString(),
             QStringLiteral("Cannot create item with duplicate id top"));
    QCOMPARE(menus.Create(Obj("{\"title\": \"no id\"}")).value(QStringLiteral("error")).toString(),
             QStringLiteral("Extensions using event pages or Service Workers must pass an id parameter to chrome.contextMenus.create"));
    QCOMPARE(menus.Create(Obj("{\"id\": \"x\"}")).value(QStringLiteral("error")).toString(),
             QStringLiteral("All menu items except for separators must have a title"));
    QVERIFY(menus.Create(Obj("{\"id\": \"sep\", \"type\": \"separator\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!menus.Create(Obj("{\"id\": \"c\", \"title\": \"c\", \"checked\": true}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Create(Obj("{\"id\": \"c\", \"title\": \"c\", \"type\": \"checkbox\", \"checked\": true}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!menus.Create(Obj("{\"id\": \"t\", \"title\": \"t\", \"type\": \"toggle\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!menus.Create(Obj("{\"id\": \"t\", \"title\": \"t\", \"contexts\": [\"everywhere\"]}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!menus.Create(Obj("{\"id\": \"t\", \"title\": \"t\", \"contexts\": []}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!menus.Create(Obj("{\"id\": \"t\", \"title\": \"t\", \"documentUrlPatterns\": [\"nonsense\"]}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Create(Obj("{\"id\": \"t\", \"title\": \"t\", \"documentUrlPatterns\": [\"*://*.example/*\"], \"targetUrlPatterns\": [\"<all_urls>\"]}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(menus.Create(Obj("{\"id\": \"o\", \"title\": \"o\", \"onclick\": 1}")).value(QStringLiteral("error")).toString().left(30),
             QStringLiteral("Extensions using event pages o"));
    QCOMPARE(menus.Create(Obj("{\"id\": \"orphan\", \"title\": \"o\", \"parentId\": \"nosuch\"}")).value(QStringLiteral("error")).toString(),
             QStringLiteral("Cannot find menu item with id nosuch"));
    QCOMPARE(menus.Items().first().contexts, QStringList() << QStringLiteral("page"));

    QCOMPARE(menus.Update(QStringLiteral("nosuch"), Obj("{\"title\": \"x\"}")).value(QStringLiteral("error")).toString(),
             QStringLiteral("Cannot find menu item with id nosuch"));
    QVERIFY(menus.Update(QStringLiteral("top"), Obj("{\"title\": \"Renamed\", \"enabled\": false}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(menus.Items().first().title, QStringLiteral("Renamed"));
    QVERIFY(!menus.Items().first().enabled);
    QVERIFY(!menus.Update(QStringLiteral("top"), Obj("{\"parentId\": \"top\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!menus.Update(QStringLiteral("top"), Obj("{\"title\": 5}")).value(QStringLiteral("ok")).toBool());

    ExtensionUi::Menus many;
    for(int i = 0; i < ExtensionUi::MENU_ITEMS_LIMIT; i++)
        QVERIFY(many.Create(Obj(qPrintable(QStringLiteral("{\"id\": \"i%1\", \"title\": \"t\"}").arg(i)))).value(QStringLiteral("ok")).toBool());
    QVERIFY(!many.Create(Obj("{\"id\": \"one more\", \"title\": \"t\"}")).value(QStringLiteral("ok")).toBool());
}

void tst_extensionui::theButtonsOwnItemsAreItsMenu(){
    ExtensionUi::Menus menus;
    QVERIFY(menus.Create(Obj("{\"id\": \"page\", \"title\": \"on the page\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Create(Obj("{\"id\": \"everywhere\", \"title\": \"all\", \"contexts\": [\"all\"]}")).value(QStringLiteral("ok")).toBool());
    for(int i = 1; i <= 6; i++)
        QVERIFY(menus.Create(Obj(QStringLiteral("{\"id\": \"a%1\", \"title\": \"button %1\", \"contexts\": [\"action\"]}").arg(i).toUtf8().constData())).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Create(Obj("{\"id\": \"sep\", \"type\": \"separator\", \"contexts\": [\"action\"]}")).value(QStringLiteral("ok")).toBool());
    const QList<ExtensionUi::MenuShown> shown = menus.ActionShown(QUrl(QStringLiteral("https://a.example/")));
    QStringList ids;
    foreach(const ExtensionUi::MenuShown &one, shown) ids << one.item.id;
    QCOMPARE(ids.join(QLatin1Char(' ')), QStringLiteral("everywhere a1 a2 a3 a4 a5"));
    const ExtensionUi::MenuContext context = ExtensionUi::Menus::ActionContext(QUrl(QStringLiteral("https://a.example/")));
    QVERIFY(context.contexts.contains(QStringLiteral("action")));
    const QJsonObject info = ExtensionUi::ClickInfo(shown.at(1).item, ExtensionUi::Menus::Clicked(), context);
    QCOMPARE(info.value(QStringLiteral("menuItemId")).toString(), QStringLiteral("a1"));
    QCOMPARE(info.value(QStringLiteral("pageUrl")).toString(), QStringLiteral("https://a.example/"));
    QCOMPARE(info.value(QStringLiteral("editable")).toBool(true), false);
    ExtensionUi::MenuContext page;
    page.contexts.insert(QStringLiteral("page"));
    page.pageUrl = QUrl(QStringLiteral("https://a.example/"));
    QStringList onPage;
    foreach(const ExtensionUi::MenuShown &one, menus.Shown(page)) onPage << one.item.id;
    QCOMPARE(onPage.join(QLatin1Char(' ')), QStringLiteral("page everywhere"));
}

void tst_extensionui::theButtonsMenuIsChosenByContextsAlone(){
    ExtensionUi::Menus menus;
    QVERIFY(menus.Create(Obj("{\"id\": \"doc\", \"title\": \"d\", \"contexts\": [\"all\"], \"documentUrlPatterns\": [\"https://b.example/*\"]}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Create(Obj("{\"id\": \"target\", \"title\": \"t\", \"contexts\": [\"all\"], \"targetUrlPatterns\": [\"https://b.example/*\"]}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Create(Obj("{\"id\": \"hidden\", \"title\": \"h\", \"contexts\": [\"action\"], \"visible\": false}")).value(QStringLiteral("ok")).toBool());
    QStringList ids;
    foreach(const ExtensionUi::MenuShown &one, menus.ActionShown(QUrl(QStringLiteral("https://a.example/")))) ids << one.item.id;
    QCOMPARE(ids.join(QLatin1Char(' ')), QStringLiteral("doc target"));
    ExtensionUi::MenuContext page;
    page.contexts.insert(QStringLiteral("page"));
    page.pageUrl = QUrl(QStringLiteral("https://a.example/"));
    QVERIFY(menus.Shown(page).isEmpty());
}

void tst_extensionui::openPopupIsRefusedWhereThePressWouldDoNothing(){
    using ExtensionUi::WhyNotOpenPopup;
    const QString none = QStringLiteral("Extension does not have a popup on the active tab.");
    ExtensionUi::TabNow tab = Tab(7);
    QCOMPARE(WhyNotOpenPopup(false, nullptr, &tab), none);
    QCOMPARE(WhyNotOpenPopup(true, nullptr, nullptr), QStringLiteral("Could not find an active browser window."));
    QCOMPARE(WhyNotOpenPopup(true, nullptr, &tab), QString());
    ExtensionUi::Action action;
    QCOMPARE(WhyNotOpenPopup(true, &action, &tab), QString());
    action.SetEnabled(tab, false);
    QCOMPARE(WhyNotOpenPopup(true, &action, &tab), none);
    ExtensionUi::TabNow other = Tab(8);
    QCOMPARE(WhyNotOpenPopup(true, &action, &other), QString());
    ExtensionUi::TabNow all;
    action.SetEnabled(all, false);
    QCOMPARE(WhyNotOpenPopup(true, &action, &other), none);
}

void tst_extensionui::anItemGoesUnderItsParentAndOutWithIt(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"top\", \"title\": \"Top\"}"));
    menus.Create(Obj("{\"id\": \"a\", \"title\": \"A\", \"parentId\": \"top\"}"));
    menus.Create(Obj("{\"id\": \"aa\", \"title\": \"AA\", \"parentId\": \"a\"}"));
    menus.Create(Obj("{\"id\": \"b\", \"title\": \"B\", \"parentId\": \"top\"}"));
    menus.Create(Obj("{\"id\": \"other\", \"title\": \"Other\"}"));
    ExtensionUi::MenuContext context;
    context.contexts.insert(QStringLiteral("page"));
    QCOMPARE(Titles(menus.Shown(context)), QStringList() << "Top" << "  A" << "    AA" << "  B" << "Other");
    QVERIFY(!menus.Update(QStringLiteral("top"), Obj("{\"parentId\": \"aa\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.Update(QStringLiteral("b"), Obj("{\"parentId\": null}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(Titles(menus.Shown(context)), QStringList() << "Top" << "  A" << "    AA" << "Other" << "B");
    ExtensionUi::Menus deep;
    QString parent;
    for(int i = 1; i <= ExtensionUi::MENU_DEPTH_LIMIT; i++){
        QJsonObject props;
        props[QStringLiteral("id")] = QStringLiteral("d%1").arg(i);
        props[QStringLiteral("title")] = QStringLiteral("t");
        if(!parent.isEmpty()) props[QStringLiteral("parentId")] = parent;
        QVERIFY2(deep.Create(props).value(QStringLiteral("ok")).toBool(), qPrintable(QString::number(i)));
        parent = props.value(QStringLiteral("id")).toString();
    }
    QVERIFY(!deep.Create(Obj("{\"id\": \"too deep\", \"title\": \"t\", \"parentId\": \"d8\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(deep.Create(Obj("{\"id\": \"beside\", \"title\": \"t\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(deep.Create(Obj("{\"id\": \"under beside\", \"title\": \"t\", \"parentId\": \"beside\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(deep.Update(QStringLiteral("beside"), Obj("{\"parentId\": \"d6\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!deep.Update(QStringLiteral("beside"), Obj("{\"parentId\": \"d7\"}")).value(QStringLiteral("ok")).toBool());

    QVERIFY(menus.Remove(QStringLiteral("a")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(Titles(menus.Shown(context)), QStringList() << "Top" << "Other" << "B");
    QCOMPARE(menus.Items().size(), 3);
    QVERIFY(!menus.Remove(QStringLiteral("a")).value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.RemoveAll().value(QStringLiteral("ok")).toBool());
    QVERIFY(menus.IsEmpty());
}

void tst_extensionui::whatIsShownIsWhatTheContextNames(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"page\", \"title\": \"Page\"}"));
    menus.Create(Obj("{\"id\": \"sel\", \"title\": \"Search %s\", \"contexts\": [\"selection\"]}"));
    menus.Create(Obj("{\"id\": \"link\", \"title\": \"Link\", \"contexts\": [\"link\", \"image\"]}"));
    menus.Create(Obj("{\"id\": \"all\", \"title\": \"All\", \"contexts\": [\"all\"]}"));
    menus.Create(Obj("{\"id\": \"hidden\", \"title\": \"Hidden\", \"contexts\": [\"all\"], \"visible\": false}"));
    menus.Create(Obj("{\"id\": \"under hidden\", \"title\": \"Under\", \"contexts\": [\"all\"], \"parentId\": \"hidden\"}"));
    menus.Create(Obj("{\"id\": \"parent\", \"title\": \"Parent\", \"contexts\": [\"all\"]}"));
    menus.Create(Obj("{\"id\": \"child\", \"title\": \"Child\", \"contexts\": [\"link\"], \"parentId\": \"parent\"}"));

    ExtensionUi::MenuContext page;
    page.contexts.insert(QStringLiteral("page"));
    QCOMPARE(Titles(menus.Shown(page)), QStringList() << "Page" << "All" << "Parent");

    ExtensionUi::MenuContext selection;
    selection.contexts.insert(QStringLiteral("selection"));
    selection.selectionText = QStringLiteral("words");
    QCOMPARE(Titles(menus.Shown(selection)), QStringList() << "Search words" << "All" << "Parent");

    ExtensionUi::MenuContext link;
    link.contexts.insert(QStringLiteral("link"));
    link.contexts.insert(QStringLiteral("image"));
    QCOMPARE(Titles(menus.Shown(link)), QStringList() << "Link" << "All" << "Parent" << "  Child");
}

void tst_extensionui::thePatternsChooseByAddress(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"doc\", \"title\": \"Doc\", \"contexts\": [\"all\"], \"documentUrlPatterns\": [\"https://a.example/*\"]}"));
    menus.Create(Obj("{\"id\": \"target\", \"title\": \"Target\", \"contexts\": [\"all\"], \"targetUrlPatterns\": [\"*://*.b.example/*\"]}"));
    ExtensionUi::MenuContext context;
    context.contexts.insert(QStringLiteral("page"));
    context.pageUrl = QUrl(QStringLiteral("https://a.example/page"));
    QCOMPARE(Titles(menus.Shown(context)), QStringList() << "Doc");
    context.pageUrl = QUrl(QStringLiteral("https://c.example/page"));
    context.contexts.insert(QStringLiteral("link"));
    context.linkUrl = QUrl(QStringLiteral("http://x.b.example/"));
    QCOMPARE(Titles(menus.Shown(context)), QStringList() << "Target");
    context.linkUrl = QUrl(QStringLiteral("http://b.example.net/"));
    QCOMPARE(Titles(menus.Shown(context)), QStringList());
}

void tst_extensionui::aLinkedImageIsMatchedByWhatTheItemSpeaksOf(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"l\", \"title\": \"L\", \"contexts\": [\"link\"], \"targetUrlPatterns\": [\"https://link.example/*\"]}"));
    menus.Create(Obj("{\"id\": \"i\", \"title\": \"I\", \"contexts\": [\"image\"], \"targetUrlPatterns\": [\"https://link.example/*\"]}"));
    menus.Create(Obj("{\"id\": \"i2\", \"title\": \"I2\", \"contexts\": [\"image\"], \"targetUrlPatterns\": [\"https://img.example/*\"]}"));
    menus.Create(Obj("{\"id\": \"a\", \"title\": \"A\", \"contexts\": [\"all\"], \"targetUrlPatterns\": [\"https://img.example/*\"]}"));
    ExtensionUi::MenuContext context;
    context.contexts.insert(QStringLiteral("link"));
    context.contexts.insert(QStringLiteral("image"));
    context.pageUrl = QUrl(QStringLiteral("https://a.example/"));
    context.linkUrl = QUrl(QStringLiteral("https://link.example/to"));
    context.srcUrl = QUrl(QStringLiteral("https://img.example/i.png"));
    QCOMPARE(Titles(menus.Shown(context)), QStringList() << "L" << "I2" << "A");
    const QJsonObject info = ExtensionUi::ClickInfo(menus.Items().at(0), menus.Click(QStringLiteral("l")), context);
    QCOMPARE(info.value(QStringLiteral("linkUrl")).toString(), QStringLiteral("https://link.example/to"));
    QCOMPARE(info.value(QStringLiteral("srcUrl")).toString(), QStringLiteral("https://img.example/i.png"));
    QCOMPARE(info.value(QStringLiteral("mediaType")).toString(), QStringLiteral("image"));
}

void tst_extensionui::whatIsBoundedIsRefusedPastTheBound(){
    ExtensionUi::Menus menus;
    QJsonObject props;
    props[QStringLiteral("id")] = QString(ExtensionUi::MENU_ID_LIMIT + 1, QLatin1Char('i'));
    props[QStringLiteral("title")] = QStringLiteral("t");
    QVERIFY(!menus.Create(props).value(QStringLiteral("ok")).toBool());
    props[QStringLiteral("id")] = QStringLiteral("ok");
    props[QStringLiteral("title")] = QString(ExtensionUi::MENU_TITLE_LIMIT + 1, QLatin1Char('t'));
    QVERIFY(!menus.Create(props).value(QStringLiteral("ok")).toBool());
    props[QStringLiteral("title")] = QString(ExtensionUi::MENU_TITLE_LIMIT, QLatin1Char('t'));
    QJsonArray many;
    for(int i = 0; i <= ExtensionUi::MENU_PATTERNS_LIMIT; i++) many.append(QStringLiteral("https://p%1.example/*").arg(i));
    props[QStringLiteral("documentUrlPatterns")] = many;
    QVERIFY(!menus.Create(props).value(QStringLiteral("ok")).toBool());
    many.removeLast();
    props[QStringLiteral("documentUrlPatterns")] = many;
    QVERIFY(menus.Create(props).value(QStringLiteral("ok")).toBool());
}

void tst_extensionui::aTabIdIsAWholeNumberAndAFileIsNamedByADigest(){
    bool ok = false;
    QCOMPARE(ExtensionUi::TabIdOf(QJsonValue(7), &ok), qint64(7)); QVERIFY(ok);
    ExtensionUi::TabIdOf(QJsonValue(1.5), &ok); QVERIFY(!ok);
    ExtensionUi::TabIdOf(QJsonValue(0), &ok); QVERIFY(!ok);
    ExtensionUi::TabIdOf(QJsonValue(-3), &ok); QVERIFY(!ok);
    ExtensionUi::TabIdOf(QJsonValue(QStringLiteral("7")), &ok); QVERIFY(!ok);
    ExtensionUi::TabIdOf(QJsonValue(1e300), &ok); QVERIFY(!ok);
    const QString a = ExtensionUi::MenusFileName(QStringLiteral("a:b")), b = ExtensionUi::MenusFileName(QStringLiteral("a/b"));
    QVERIFY(a != b);
    QVERIFY(a.startsWith(QStringLiteral("extension-menus-")) && a.endsWith(QStringLiteral(".json")));
    static const QRegularExpression plain(QStringLiteral("\\Aextension-menus-[0-9a-f]{24}\\.json\\z"));
    QVERIFY(plain.match(a).hasMatch());
    QCOMPARE(ExtensionUi::MenusFileName(QStringLiteral("a:b")), a);
}

void tst_extensionui::aCheckboxFlipsAndARadioTurnsItsSiblingsOff(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"box\", \"title\": \"Box\", \"type\": \"checkbox\"}"));
    menus.Create(Obj("{\"id\": \"r1\", \"title\": \"R1\", \"type\": \"radio\", \"checked\": true}"));
    menus.Create(Obj("{\"id\": \"r2\", \"title\": \"R2\", \"type\": \"radio\"}"));
    menus.Create(Obj("{\"id\": \"group\", \"title\": \"G\"}"));
    menus.Create(Obj("{\"id\": \"r3\", \"title\": \"R3\", \"type\": \"radio\", \"parentId\": \"group\", \"checked\": true}"));
    menus.Create(Obj("{\"id\": \"plain\", \"title\": \"P\"}"));

    ExtensionUi::Menus::Clicked clicked = menus.Click(QStringLiteral("box"));
    QVERIFY(clicked.found && clicked.checkable && clicked.changed && !clicked.wasChecked && clicked.checked);
    QVERIFY(menus.Items().at(0).checked);
    clicked = menus.Click(QStringLiteral("box"));
    QVERIFY(clicked.wasChecked && !clicked.checked);

    clicked = menus.Click(QStringLiteral("r2"));
    QVERIFY(clicked.checkable && clicked.changed && !clicked.wasChecked && clicked.checked);
    QVERIFY(!menus.Items().at(1).checked && menus.Items().at(2).checked);
    QVERIFY(menus.Items().at(4).checked);
    menus.Create(Obj("{\"id\": \"r4\", \"title\": \"R4\", \"type\": \"radio\", \"checked\": true}"));
    menus.Create(Obj("{\"id\": \"r5\", \"title\": \"R5\", \"type\": \"radio\"}"));
    clicked = menus.Click(QStringLiteral("r5"));
    QVERIFY(clicked.changed);
    QVERIFY(!menus.Items().at(6).checked && menus.Items().at(7).checked);
    QVERIFY(menus.Items().at(2).checked);
    clicked = menus.Click(QStringLiteral("r2"));
    QVERIFY(clicked.wasChecked && clicked.checked && !clicked.changed);

    clicked = menus.Click(QStringLiteral("plain"));
    QVERIFY(clicked.found && !clicked.checkable && !clicked.changed);
    QVERIFY(!menus.Click(QStringLiteral("nosuch")).found);
}

void tst_extensionui::theRadiosAreKeptAsChromeKeepsThem(){
    ExtensionUi::Menus menus;
    auto on = [&menus](){
        QStringList out;
        foreach(const ExtensionUi::MenuItem &item, menus.Items()) if(item.checked) out << item.id;
        return out.join(QLatin1Char(','));
    };
    menus.Create(Obj("{\"id\": \"a\", \"title\": \"A\", \"type\": \"radio\"}"));
    QCOMPARE(on(), QStringLiteral("a"));
    menus.Create(Obj("{\"id\": \"b\", \"title\": \"B\", \"type\": \"radio\", \"checked\": true}"));
    menus.Create(Obj("{\"id\": \"c\", \"title\": \"C\", \"type\": \"radio\"}"));
    QCOMPARE(on(), QStringLiteral("b"));
    QVERIFY(menus.Update(QStringLiteral("c"), Obj("{\"checked\": true}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(on(), QStringLiteral("c"));
    QVERIFY(menus.Update(QStringLiteral("c"), Obj("{\"checked\": false}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(on(), QStringLiteral("c"));
    QVERIFY(menus.Remove(QStringLiteral("c")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(on(), QStringLiteral("a"));
    menus.Create(Obj("{\"id\": \"g\", \"title\": \"G\"}"));
    menus.Create(Obj("{\"id\": \"x\", \"title\": \"X\", \"type\": \"radio\", \"parentId\": \"g\", \"checked\": true}"));
    QCOMPARE(on(), QStringLiteral("a,x"));
    QVERIFY(menus.Update(QStringLiteral("b"), Obj("{\"parentId\": \"g\", \"checked\": true}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(menus.Items().last().id, QStringLiteral("b"));
    QCOMPARE(on(), QStringLiteral("a,b"));
    menus.Create(Obj("{\"id\": \"box\", \"title\": \"Box\", \"type\": \"checkbox\", \"checked\": true}"));
    QVERIFY(menus.Update(QStringLiteral("box"), Obj("{\"checked\": false}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(on(), QStringLiteral("a,b"));

    ExtensionUi::Menus runs;
    runs.Create(Obj("{\"id\": \"a\", \"title\": \"A\", \"type\": \"radio\"}"));
    runs.Create(Obj("{\"id\": \"s\", \"type\": \"separator\"}"));
    runs.Create(Obj("{\"id\": \"p\", \"title\": \"P\", \"type\": \"radio\"}"));
    runs.Create(Obj("{\"id\": \"q\", \"title\": \"Q\", \"type\": \"radio\"}"));
    auto runsOn = [&runs](){
        QStringList out;
        foreach(const ExtensionUi::MenuItem &item, runs.Items()) out << item.id + (item.checked ? QStringLiteral("*") : QString());
        return out.join(QLatin1Char(','));
    };
    QCOMPARE(runsOn(), QStringLiteral("a*,s,p*,q"));
    QVERIFY(runs.Update(QStringLiteral("a"), Obj("{\"parentId\": null}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(runsOn(), QStringLiteral("s,p,q,a*"));

    QVERIFY(runs.Update(QStringLiteral("a"), Obj("{\"type\": \"normal\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!runs.Update(QStringLiteral("a"), Obj("{\"checked\": true}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(runs.Update(QStringLiteral("q"), Obj("{\"checked\": true}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(runsOn(), QStringLiteral("s,p,q*,a*"));
    QVERIFY(runs.Update(QStringLiteral("a"), Obj("{\"type\": \"radio\"}")).value(QStringLiteral("ok")).toBool());
    QCOMPARE(runsOn(), QStringLiteral("s,p,q,a*"));

    ExtensionUi::Menus turned;
    turned.Create(Obj("{\"id\": \"r\", \"title\": \"R\", \"type\": \"radio\", \"checked\": true}"));
    turned.Create(Obj("{\"id\": \"b\", \"title\": \"B\", \"type\": \"checkbox\", \"checked\": true}"));
    QVERIFY(turned.Update(QStringLiteral("b"), Obj("{\"type\": \"radio\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(!turned.Items().at(0).checked && turned.Items().at(1).checked);

    ExtensionUi::Menus kept;
    kept.Create(Obj("{\"id\": \"a\", \"title\": \"A\", \"type\": \"checkbox\", \"checked\": true}"));
    kept.Create(Obj("{\"id\": \"b\", \"title\": \"B\", \"type\": \"radio\", \"checked\": true}"));
    QVERIFY(kept.Update(QStringLiteral("a"), Obj("{\"type\": \"normal\"}")).value(QStringLiteral("ok")).toBool());
    ExtensionUi::Menus back = ExtensionUi::Menus::FromJson(kept.ToJson());
    QCOMPARE(back.Items().size(), 2);
    QVERIFY(back.Update(QStringLiteral("a"), Obj("{\"type\": \"radio\"}")).value(QStringLiteral("ok")).toBool());
    QVERIFY(back.Items().at(0).checked && !back.Items().at(1).checked);
}

void tst_extensionui::theRegistryComesBackFromItsJson(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"top\", \"title\": \"Top\", \"contexts\": [\"all\"]}"));
    menus.Create(Obj("{\"id\": \"box\", \"title\": \"Box\", \"type\": \"checkbox\", \"parentId\": \"top\", \"checked\": true, \"enabled\": false, \"documentUrlPatterns\": [\"https://a.example/*\"]}"));
    menus.Create(Obj("{\"id\": \"sep\", \"type\": \"separator\", \"visible\": false}"));
    const QJsonArray json = menus.ToJson();
    const ExtensionUi::Menus back = ExtensionUi::Menus::FromJson(json);
    QCOMPARE(back.ToJson(), json);
    QCOMPARE(back.Items().size(), 3);
    QVERIFY(back.Items().at(1).checked && !back.Items().at(1).enabled);
    QCOMPARE(back.Items().at(1).parentId, QStringLiteral("top"));
    QCOMPARE(back.Items().at(1).documentUrlPatterns, QStringList() << QStringLiteral("https://a.example/*"));

    ExtensionUi::Menus moved;
    moved.Create(Obj("{\"id\": \"first\", \"title\": \"F\"}"));
    moved.Create(Obj("{\"id\": \"later\", \"title\": \"L\"}"));
    QVERIFY(moved.Update(QStringLiteral("first"), Obj("{\"parentId\": \"later\"}")).value(QStringLiteral("ok")).toBool());
    const ExtensionUi::Menus movedBack = ExtensionUi::Menus::FromJson(moved.ToJson());
    QCOMPARE(movedBack.Items().size(), 2);
    QCOMPARE(movedBack.Items().at(1).parentId, QStringLiteral("later"));

    QJsonArray damaged = json;
    damaged.append(QJsonValue(5));
    damaged.append(Obj("{\"id\": \"top\", \"title\": \"dup\"}"));
    damaged.append(Obj("{\"id\": \"orphan\", \"title\": \"o\", \"parentId\": \"gone\"}"));
    damaged.append(Obj("{\"id\": \"bad\", \"title\": \"b\", \"documentUrlPatterns\": [\"::\"]}"));
    damaged.append(Obj("{\"id\": \"checked plain\", \"title\": \"c\", \"checked\": true}"));
    const ExtensionUi::Menus survived = ExtensionUi::Menus::FromJson(damaged);
    QCOMPARE(survived.Items().size(), 4);
    QCOMPARE(survived.Items().last().id, QStringLiteral("checked plain"));
    QVERIFY(survived.Items().last().checked);
}

void tst_extensionui::theInfoOfAClickSaysWhereItWas(){
    ExtensionUi::Menus menus;
    menus.Create(Obj("{\"id\": \"top\", \"title\": \"Top\"}"));
    menus.Create(Obj("{\"id\": \"box\", \"title\": \"Box\", \"type\": \"checkbox\", \"parentId\": \"top\"}"));
    ExtensionUi::MenuContext context;
    context.contexts.insert(QStringLiteral("link"));
    context.contexts.insert(QStringLiteral("image"));
    context.contexts.insert(QStringLiteral("editable"));
    context.pageUrl = QUrl(QStringLiteral("https://a.example/p?q=1"));
    context.linkUrl = QUrl(QStringLiteral("https://a.example/i.png"));
    context.srcUrl = QUrl(QStringLiteral("https://a.example/i.png"));
    context.selectionText = QStringLiteral("sel");
    const ExtensionUi::Menus::Clicked clicked = menus.Click(QStringLiteral("box"));
    QCOMPARE(Json(ExtensionUi::ClickInfo(menus.Items().at(1), clicked, context)),
             QStringLiteral("{\"checked\":true,\"editable\":true,\"frameId\":0,\"linkUrl\":\"https://a.example/i.png\",\"mediaType\":\"image\","
                            "\"menuItemId\":\"box\",\"pageUrl\":\"https://a.example/p?q=1\",\"parentMenuItemId\":\"top\","
                            "\"selectionText\":\"sel\",\"srcUrl\":\"https://a.example/i.png\",\"wasChecked\":false}"));
    ExtensionUi::MenuContext page;
    page.contexts.insert(QStringLiteral("page"));
    page.pageUrl = QUrl(QStringLiteral("https://a.example/"));
    QCOMPARE(Json(ExtensionUi::ClickInfo(menus.Items().at(0), menus.Click(QStringLiteral("top")), page)),
             QStringLiteral("{\"editable\":false,\"frameId\":0,\"menuItemId\":\"top\",\"pageUrl\":\"https://a.example/\"}"));
}

void tst_extensionui::theAddressOfADownloadIsABlobOrData(){
    QVERIFY(ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/3f1a-4b")));
    QVERIFY(ExtensionUi::AcceptsDownloadUrl(QStringLiteral("BLOB:chrome-extension://abcdefghijklmnopabcdefghijklmnop/x")));
    QVERIFY(ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:http://127.0.0.1:8765/1e5b-4c")));
    QVERIFY(ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:https://a.example/x")));
    QVERIFY(ExtensionUi::AcceptsDownloadUrl(QStringLiteral("data:text/plain,hi")));
    QVERIFY(ExtensionUi::AcceptsDownloadUrl(QStringLiteral("data:text/html;base64,PGI+")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("data:")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:file:///x")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:x")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:https://a.example")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("blob:javascript:alert(1)")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("https://a.example/f.zip")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("http://a.example/f.zip")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("javascript:alert(1)")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/x.html")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("")));
    QVERIFY(!ExtensionUi::AcceptsDownloadUrl(QStringLiteral("nocolon")));

    foreach(const QString &fine, QStringList() << QString() << QStringLiteral("page.html") << QStringLiteral("pages/a b.html")
                                               << QStringLiteral("pages\\deep\\x.txt") << QStringLiteral("日本語.html") << QStringLiteral("a.b.c"))
        QVERIFY2(ExtensionUi::AcceptsDownloadFilename(fine), qPrintable(fine));
    foreach(const QString &bad, QStringList() << QStringLiteral("/etc/x") << QStringLiteral("\\x") << QStringLiteral("C:/x.txt") << QStringLiteral("C:\\x.txt")
                                              << QStringLiteral("//share/p.exe/x") << QStringLiteral("\\\\share\\p.exe\\x") << QStringLiteral("../x") << QStringLiteral("a/../x")
                                              << QStringLiteral("./x") << QStringLiteral("a//x") << QStringLiteral("a/") << QStringLiteral("x.exe ") << QStringLiteral("x.")
                                              << QStringLiteral("a*b") << QStringLiteral("a?b") << QStringLiteral("a\"b") << QStringLiteral("a<b") << QStringLiteral("a|b")
                                              << QStringLiteral("a\nb") << QString(QChar(0x1f)) + QStringLiteral("x")
                                              << QStringLiteral("invoice.pdf.lnk") << QStringLiteral("sub/x.LNK") << QStringLiteral("x.local") << QStringLiteral("x.{20D04FE0-3AEA-1069-A2D8-08002B30309D}")
                                              << QStringLiteral("photo") + QChar(0x202e) + QStringLiteral("gpj.exe") << QStringLiteral("a") + QChar(0x200b) + QStringLiteral("b") << QString(QChar(0x7f)) + QStringLiteral("x")
                                              << QStringLiteral(".hidden") << QStringLiteral("a/.b")
                                              << QStringLiteral("CON") << QStringLiteral("nul.txt") << QStringLiteral("com1") << QStringLiteral("sub/LPT9.log") << QStringLiteral("desktop.ini") << QStringLiteral("Thumbs.db"))
        QVERIFY2(!ExtensionUi::AcceptsDownloadFilename(bad), qPrintable(bad));
    foreach(const QString &fine, QStringList() << QStringLiteral("CONTRACT.txt") << QStringLiteral("COM10") << QStringLiteral("lnk.txt") << QStringLiteral("a.lnkx") << QStringLiteral("x.{notaclsid"))
        QVERIFY2(ExtensionUi::AcceptsDownloadFilename(fine), qPrintable(fine));
}

void tst_extensionui::aDownloadIsExpectedThenKnownByItsAddress(){
    ExtensionUi::Downloads downloads;
    const QUrl blob(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/1234"));
    const qint64 first = downloads.Expect(QStringLiteral("ext-a"), blob, QStringLiteral("page.html"), 1000);
    QCOMPARE(first, qint64(1));
    QCOMPARE(downloads.Expect(QStringLiteral("ext-b"), QUrl(QStringLiteral("data:text/plain,hi")), QString(), 1000), qint64(2));
    QString whose;
    QCOMPARE(downloads.Arrived(QUrl(QStringLiteral("https://x.example/f.zip")), 1001, &whose), qint64(0));
    QCOMPARE(downloads.Arrived(blob, 1001, &whose), first);
    QCOMPARE(whose, QStringLiteral("ext-a"));
    QCOMPARE(downloads.Expected(), 1);
    QCOMPARE(downloads.Created(first).value(QStringLiteral("state")).toString(), QStringLiteral("in_progress"));
    QCOMPARE(downloads.Arrived(blob, 1002, &whose), qint64(0));

    downloads.Describe(first, QStringLiteral("C:/down/page.html"), QStringLiteral("text/html"), 500, 500);
    const QJsonObject ended = downloads.Ended(first, true, QString());
    QCOMPARE(QString::fromUtf8(QJsonDocument(ended).toJson(QJsonDocument::Compact)),
             QStringLiteral("{\"id\":1,\"state\":{\"current\":\"complete\",\"previous\":\"in_progress\"}}"));
    QVERIFY(downloads.Ended(first, true, QString()).isEmpty());
    QVERIFY(downloads.Ended(99, true, QString()).isEmpty());
    QJsonObject query;
    query[QStringLiteral("id")] = 1;
    const QJsonArray found = downloads.Search(QStringLiteral("ext-a"), query);
    QCOMPARE(found.size(), 1);
    const QJsonObject item = found.first().toObject();
    QCOMPARE(item.value(QStringLiteral("filename")).toString(), QStringLiteral("C:/down/page.html"));
    QCOMPARE(item.value(QStringLiteral("state")).toString(), QStringLiteral("complete"));
    QCOMPARE(item.value(QStringLiteral("exists")).toBool(), true);
    QCOMPARE(item.value(QStringLiteral("totalBytes")).toInt(), 500);
    QCOMPARE(downloads.Search(QStringLiteral("ext-b"), query).size(), 0);
    QCOMPARE(downloads.Search(QStringLiteral("ext-a"), QJsonObject()).size(), 0);
    query[QStringLiteral("id")] = 1.5;
    QCOMPARE(downloads.Search(QStringLiteral("ext-a"), query).size(), 0);

    const QUrl data(QStringLiteral("data:text/plain,hi"));
    const qint64 second = downloads.Arrived(data, 1003, &whose);
    QCOMPARE(second, qint64(2));
    const QJsonObject cancelled = downloads.Ended(second, false, QStringLiteral("USER_CANCELED"));
    QCOMPARE(cancelled.value(QStringLiteral("error")).toObject().value(QStringLiteral("current")).toString(), QStringLiteral("USER_CANCELED"));
    QCOMPARE(cancelled.value(QStringLiteral("state")).toObject().value(QStringLiteral("current")).toString(), QStringLiteral("interrupted"));
    QCOMPARE(downloads.Search(QStringLiteral("ext-b"), QJsonObject{{QStringLiteral("id"), 2}}).first().toObject().value(QStringLiteral("exists")).toBool(), false);

    QVERIFY(!item.value(QStringLiteral("startTime")).toString().isEmpty());
    QVERIFY(!item.value(QStringLiteral("endTime")).toString().isEmpty());
    QCOMPARE(downloads.Search(QStringLiteral("ext-b"), QJsonObject{{QStringLiteral("id"), 2}}).first().toObject().value(QStringLiteral("totalBytes")).toInt(), 0);

    const qint64 late = downloads.Expect(QStringLiteral("ext-a"), QUrl(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/late")), QString(), 2000);
    QCOMPARE(downloads.Expired(2000 + ExtensionUi::DOWNLOADS_EXPECTED_MS).size(), 0);
    const QList<ExtensionUi::Downloads::Gone> gone = downloads.Expired(2000 + ExtensionUi::DOWNLOADS_EXPECTED_MS + 1);
    QCOMPARE(gone.size(), 1);
    QCOMPARE(gone.first().extension, QStringLiteral("ext-a"));
    QCOMPARE(QString::fromUtf8(QJsonDocument(gone.first().delta).toJson(QJsonDocument::Compact)),
             QStringLiteral("{\"error\":{\"current\":\"FILE_FAILED\"},\"id\":%1,\"state\":{\"current\":\"interrupted\",\"previous\":\"in_progress\"}}").arg(late));
    QCOMPARE(downloads.Arrived(QUrl(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/late")), 2000 + ExtensionUi::DOWNLOADS_EXPECTED_MS + 2, &whose), qint64(0));
    const qint64 theirs = downloads.Expect(QStringLiteral("ext-b"), QUrl(QStringLiteral("blob:https://b.example/theirs")), QString(), 3000);
    for(int i = 0; i < ExtensionUi::DOWNLOADS_EXPECTED_KEPT + 2; i++)
        downloads.Expect(QStringLiteral("ext-a"), QUrl(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/n%1").arg(i)), QString(), 3000);
    QCOMPARE(downloads.Expected(), ExtensionUi::DOWNLOADS_EXPECTED_KEPT + 1);
    QCOMPARE(downloads.Arrived(QUrl(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/n0")), 3001, &whose), qint64(0));
    QVERIFY(downloads.Arrived(QUrl(QStringLiteral("blob:chrome-extension://abcdefghijklmnopabcdefghijklmnop/n2")), 3001, &whose) > 0);
    QCOMPARE(downloads.Arrived(QUrl(QStringLiteral("blob:https://b.example/theirs")), 3001, &whose), theirs);
}

void tst_extensionui::whatOnInstalledIsOwedIsDecidedOnceAndKeptUntilTaken(){
    typedef QPair<QString, QString> R;
    auto said = [](const ExtensionUi::Installed::Owed &o){ return o.reason + QLatin1Char('/') + o.previousVersion; };

    ExtensionUi::Installed migrated;
    QCOMPARE(migrated.Decide({R("a", "1.0"), R("b", "2.0")}, true), QStringList({"a", "b"}));
    QCOMPARE(said(migrated.Take("a")), QStringLiteral("update/1.0"));
    QCOMPARE(said(migrated.Take("a")), QStringLiteral("/"));
    QVERIFY(migrated.Decide({R("a", "1.0"), R("b", "2.0")}, false).isEmpty());
    QCOMPARE(said(migrated.OwedTo("b")), QStringLiteral("update/2.0"));

    ExtensionUi::Installed l;
    QCOMPARE(l.Decide({R("a", "1.0")}, false), QStringList({"a"}));
    QCOMPARE(said(l.Take("a")), QStringLiteral("install/"));
    QVERIFY(l.Decide({R("a", "1.0")}, false).isEmpty());
    QCOMPARE(said(l.Take("a")), QStringLiteral("/"));
    QCOMPARE(l.Decide({R("a", "1.1")}, false), QStringList({"a"}));
    QCOMPARE(l.Decide({R("a", "1.2")}, false), QStringList({"a"}));
    QCOMPARE(said(l.OwedTo("a")), QStringLiteral("update/1.0"));
    const ExtensionUi::Installed::Owed taken = l.Take("a");
    l.Restore("a", taken);
    QCOMPARE(said(l.OwedTo("a")), QStringLiteral("update/1.0"));
    l.Take("a");
    QCOMPARE(l.Decide({R("a", "1.2"), R("n", "1")}, false), QStringList({"n"}));
    QCOMPARE(l.Decide({R("a", "1.2"), R("n", "2")}, false), QStringList({"n"}));
    QCOMPARE(said(l.Take("n")), QStringLiteral("install/"));
    QVERIFY(l.Forget(QSet<QString>({"a"})));
    QVERIFY(!l.Contains("n"));
    QCOMPARE(l.Decide({R("a", "1.2"), R("n", "2")}, false), QStringList({"n"}));
    QCOMPARE(said(l.Take("n")), QStringLiteral("install/"));
    QVERIFY(l.Decide({R("", "1")}, false).isEmpty());
}

void tst_extensionui::theLedgerComesBackFromItsJson(){
    typedef QPair<QString, QString> R;
    ExtensionUi::Installed l;
    l.Decide({R("a", "1.0"), R("b", "2.0")}, true);
    l.Take("a");
    bool damaged = true;
    ExtensionUi::Installed back = ExtensionUi::Installed::FromJson(l.ToJson(), &damaged);
    QVERIFY(!damaged);
    QVERIFY(back.Contains("a"));
    QCOMPARE(back.OwedTo("a").reason, QString());
    QCOMPARE(back.OwedTo("b").reason, QStringLiteral("update"));
    QCOMPARE(back.OwedTo("b").previousVersion, QStringLiteral("2.0"));
    QVERIFY(back.Decide({R("a", "1.0"), R("b", "2.0")}, false).isEmpty());
    QJsonObject bad = l.ToJson();
    bad["c"] = QJsonObject{{"version", 3}};
    bad["d"] = QJsonObject{{"version", "1"}, {"owed", "reinstall"}};
    back = ExtensionUi::Installed::FromJson(bad, &damaged);
    QVERIFY(damaged);
    QVERIFY(back.Contains("b"));
    QVERIFY(!back.Contains("c"));
    QVERIFY(!back.Contains("d"));
    QVERIFY(ExtensionUi::InstalledFileName("x").startsWith("extension-installed-"));
    QVERIFY(ExtensionUi::InstalledFileName("x") != ExtensionUi::InstalledFileName("y"));
}

void tst_extensionui::aDownloadIsPausedErasedAndNamedInChromesWords(){
    using namespace ExtensionUi;
    Downloads downloads;
    QString whose;
    const QUrl blob(QStringLiteral("blob:http://a.example/1"));
    const qint64 going = downloads.Expect(QStringLiteral("ext-a"), blob, QString(), 1000);
    QCOMPARE(downloads.Arrived(blob, 1001, &whose), going);
    const QUrl data(QStringLiteral("data:text/plain,hi"));
    const qint64 done = downloads.Expect(QStringLiteral("ext-a"), data, QString(), 1000);
    QCOMPARE(downloads.Arrived(data, 1001, &whose), done);
    downloads.Describe(done, QStringLiteral("C:/down/hi.txt"), QStringLiteral("text/plain"), 2, 2);
    QVERIFY(!downloads.Ended(done, true, QString()).isEmpty());

    QCOMPARE(QString::fromUtf8(QJsonDocument(downloads.Named(going, QStringLiteral("C:/down/page.html"))).toJson(QJsonDocument::Compact)),
             QStringLiteral("{\"filename\":{\"current\":\"C:/down/page.html\",\"previous\":\"\"},\"id\":1}"));
    QVERIFY(downloads.Named(going, QStringLiteral("C:/down/other.html")).isEmpty());
    QVERIFY(downloads.Named(going, QString()).isEmpty());
    QVERIFY(downloads.Named(99, QStringLiteral("x")).isEmpty());
    QCOMPARE(downloads.RecordOf(going)->filename, QStringLiteral("C:/down/page.html"));

    QCOMPARE(QString::fromUtf8(QJsonDocument(downloads.Paused(going, true)).toJson(QJsonDocument::Compact)),
             QStringLiteral("{\"canResume\":{\"current\":true,\"previous\":false},\"id\":1,\"paused\":{\"current\":true,\"previous\":false}}"));
    QVERIFY(downloads.Paused(going, true).isEmpty());
    QCOMPARE(downloads.Search(QStringLiteral("ext-a"), QJsonObject{{QStringLiteral("id"), 1}}).first().toObject().value(QStringLiteral("paused")).toBool(), true);
    QCOMPARE(downloads.Search(QStringLiteral("ext-a"), QJsonObject{{QStringLiteral("id"), 1}}).first().toObject().value(QStringLiteral("canResume")).toBool(), true);
    QVERIFY(downloads.Paused(done, true).isEmpty());
    QVERIFY(downloads.Paused(99, true).isEmpty());
    QCOMPARE(downloads.Paused(going, false).value(QStringLiteral("paused")).toObject().value(QStringLiteral("current")).toBool(), false);

    const DownloadRecord *record = downloads.RecordOf(going), *ended = downloads.RecordOf(done);
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.cancel"), nullptr), QString());
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.cancel"), ended), QString());
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.pause"), nullptr), QStringLiteral("Invalid download id"));
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.pause"), record), QString());
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.pause"), ended), QStringLiteral("Download must be in progress"));
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.resume"), record), QStringLiteral("DownloadItem.canResume == false"));
    downloads.Paused(going, true);
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.resume"), record), QString());
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.open"), ended), QStringLiteral("chrome.downloads.open is not available in this browser"));
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.show"), ended), QString());
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.show"), nullptr), QStringLiteral("Invalid download id"));
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.acceptDanger"), ended), QStringLiteral("chrome.downloads.acceptDanger is not available in this browser"));
    const qint64 nameless = downloads.Expect(QStringLiteral("ext-b"), QUrl(QStringLiteral("data:text/plain,x")), QString(), 1000);
    QCOMPARE(downloads.Arrived(QUrl(QStringLiteral("data:text/plain,x")), 1001, &whose), nameless);
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.show"), downloads.RecordOf(nameless)), QStringLiteral("Invalid download id"));

    const QJsonObject over = downloads.Ended(going, false, QStringLiteral("USER_CANCELED"));
    QCOMPARE(over.value(QStringLiteral("paused")).toObject().value(QStringLiteral("current")).toBool(true), false);
    QCOMPARE(over.value(QStringLiteral("canResume")).toObject().value(QStringLiteral("previous")).toBool(), true);
    QCOMPARE(downloads.RecordOf(going)->paused, false);
    QCOMPARE(downloads.Search(QStringLiteral("ext-a"), QJsonObject{{QStringLiteral("id"), 1}}).first().toObject().value(QStringLiteral("canResume")).toBool(true), false);
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.resume"), record), QStringLiteral("DownloadItem.canResume == false"));
    QVERIFY(!downloads.Ended(nameless, false, QStringLiteral("USER_CANCELED")).contains(QStringLiteral("paused")));
    downloads.Describe(nameless, QString(), QStringLiteral("text/plain"), 1, 0);
    QVERIFY(downloads.RecordOf(nameless)->filename.isEmpty());
    QCOMPARE(WhyNotDownloadAct(QStringLiteral("downloads.show"), downloads.RecordOf(nameless)), QStringLiteral("Invalid download id"));

    QCOMPARE(downloads.Erase(QStringLiteral("ext-b"), QJsonObject{{QStringLiteral("id"), 1}}), QList<qint64>());
    QCOMPARE(downloads.Erase(QStringLiteral("ext-a"), QJsonObject()), QList<qint64>());
    QCOMPARE(downloads.Erase(QStringLiteral("ext-a"), QJsonObject{{QStringLiteral("id"), 1.5}}), QList<qint64>());
    QCOMPARE(downloads.Erase(QStringLiteral("ext-a"), QJsonObject{{QStringLiteral("id"), 1}}), QList<qint64>() << 1);
    QCOMPARE(downloads.Erase(QStringLiteral("ext-a"), QJsonObject{{QStringLiteral("id"), 1}}), QList<qint64>());
    QVERIFY(!downloads.RecordOf(going));
    QCOMPARE(downloads.Records(), 2);
    QCOMPARE(downloads.RecordOf(QStringLiteral("ext-b"), done), nullptr);
    QVERIFY(downloads.RecordOf(QStringLiteral("ext-a"), done));

    QCOMPARE(InterruptReasonOf(40), QStringLiteral("USER_CANCELED"));
    QCOMPARE(InterruptReasonOf(3), QStringLiteral("FILE_NO_SPACE"));
    QCOMPARE(InterruptReasonOf(22), QStringLiteral("NETWORK_DISCONNECTED"));
    QCOMPARE(InterruptReasonOf(37), QStringLiteral("SERVER_UNREACHABLE"));
    QCOMPARE(InterruptReasonOf(0), QStringLiteral("FILE_FAILED"));
    QCOMPARE(InterruptReasonOf(99), QStringLiteral("FILE_FAILED"));
    QCOMPARE(InterruptReasonOf(41), QStringLiteral("USER_SHUTDOWN"));
    QCOMPARE(InterruptReasonOf(50), QStringLiteral("CRASH"));
    QCOMPARE(InterruptReasonOf(15), QStringLiteral("FILE_SAME_AS_SOURCE"));
    QCOMPARE(InterruptReasonOf(39), QStringLiteral("SERVER_CROSS_ORIGIN_REDIRECT"));

    QCOMPARE(PathOfEndedDownload(true, QStringLiteral("C:/down/x.txt")), QStringLiteral("C:/down/x.txt"));
    QCOMPARE(PathOfEndedDownload(false, QStringLiteral("C:/Windows/System32/cmd.exe/x.txt")), QString());
}

void tst_extensionui::aShortcutIsReadAsChromeReadsIt_data(){
    QTest::addColumn<QString>("suggested");
    QTest::addColumn<QString>("platform");
    QTest::addColumn<QString>("key");
    QTest::newRow("default")                  << R"({"default": "Ctrl+Shift+Y"})" << "windows" << "Ctrl+Shift+Y";
    QTest::newRow("the system's own")         << R"({"default": "Alt+Q", "windows": "Ctrl+Shift+U"})" << "windows" << "Ctrl+Shift+U";
    QTest::newRow("another system's")         << R"({"default": "Alt+Q", "mac": "Command+Shift+U"})" << "windows" << "Alt+Q";
    QTest::newRow("its own unreadable")       << R"({"default": "Alt+Q", "windows": "Ctrl+Alt+U"})" << "windows" << "";
    QTest::newRow("a string")                 << R"("Alt+Shift+P")" << "windows" << "Alt+Shift+P";
    QTest::newRow("Ctrl and Alt")             << R"("Ctrl+Alt+Y")" << "windows" << "";
    QTest::newRow("Shift alone")              << R"("Shift+Y")" << "windows" << "";
    QTest::newRow("no modifier")              << R"("Y")" << "windows" << "";
    QTest::newRow("a function key")           << R"("Ctrl+F5")" << "windows" << "";
    QTest::newRow("a media key")              << R"("MediaPlayPause")" << "windows" << "";
    QTest::newRow("a media key, modified")    << R"("Ctrl+MediaPlayPause")" << "windows" << "";
    QTest::newRow("a comma")                  << R"("Ctrl+Comma")" << "windows" << "Ctrl+Comma";
    QTest::newRow("an arrow")                 << R"("Alt+Shift+Up")" << "windows" << "Alt+Shift+Up";
    QTest::newRow("tab")                      << R"("Ctrl+Tab")" << "windows" << "Ctrl+Tab";
    QTest::newRow("a digit")                  << R"("Ctrl+Shift+1")" << "windows" << "Ctrl+Shift+1";
    QTest::newRow("a small letter")           << R"("Ctrl+Shift+y")" << "windows" << "";
    QTest::newRow("a small modifier")         << R"("ctrl+Shift+Y")" << "windows" << "";
    QTest::newRow("in another order")         << R"("Shift+Y+Ctrl")" << "windows" << "Ctrl+Shift+Y";
    QTest::newRow("two keys")                 << R"("Ctrl+Y+U")" << "windows" << "";
    QTest::newRow("four tokens")              << R"("Ctrl+Shift+Alt+Y")" << "windows" << "";
    QTest::newRow("Command off a Mac")        << R"("Command+Shift+Y")" << "windows" << "";
    QTest::newRow("Command on a Mac")         << R"({"mac": "Command+Shift+Y"})" << "mac" << "Ctrl+Shift+Y";
    QTest::newRow("nothing suggested")        << R"(null)" << "windows" << "";
}

void tst_extensionui::aShortcutIsReadAsChromeReadsIt(){
    QFETCH(QString, suggested);
    QFETCH(QString, platform);
    QFETCH(QString, key);
    const QJsonObject commands = QJsonDocument::fromJson(QStringLiteral(R"({"go": {"description": "Go", "suggested_key": %1}})").arg(suggested).toUtf8()).object();
    QVERIFY(!commands.isEmpty());
    const QList<ExtensionUi::Command> read = ExtensionUi::CommandsOf(commands, platform, true);
    QCOMPARE(read.size(), 1);
    QCOMPARE(read.first().name, QStringLiteral("go"));
    QCOMPARE(read.first().description, QStringLiteral("Go"));
    QCOMPARE(read.first().key, key);
}

void tst_extensionui::aKeyPressedIsSpelledAsChromeSpellsIt(){
    using namespace ExtensionUi;
    const QJsonObject many = QJsonDocument::fromJson(R"({
        "e": {"suggested_key": "Ctrl+Shift+5"}, "a": {"suggested_key": "Ctrl+Shift+1"},
        "_execute_action": {"suggested_key": "Alt+Shift+A"}, "c": {"suggested_key": "Ctrl+Shift+3"},
        "b": {"suggested_key": "Ctrl+Shift+2"}, "d": {"description": "no key"},
        "_execute_browser_action": {"suggested_key": "Alt+Shift+B"}})").object();
    QStringList said;
    foreach(const Command &command, CommandsOf(many, QStringLiteral("windows"), true)) said << command.name + QLatin1Char('=') + command.key;
    QCOMPARE(said.join(QLatin1Char(' ')), QStringLiteral("_execute_action=Alt+Shift+A a=Ctrl+Shift+1 b=Ctrl+Shift+2 c=Ctrl+Shift+3 d= e="));
    said.clear();
    foreach(const Command &command, CommandsOf(many, QStringLiteral("windows"), false)) said << command.name;
    QCOMPARE(said.join(QLatin1Char(' ')), QStringLiteral("a b c d e"));

    QCOMPARE(ChromeKeyOf(true, false, true, 0x31, Qt::Key_Exclam), QStringLiteral("Ctrl+Shift+1"));
    QCOMPARE(ChromeKeyOf(true, false, true, 0xBC, Qt::Key_Less), QStringLiteral("Ctrl+Shift+Comma"));
    QCOMPARE(ChromeKeyOf(true, false, true, 0x09, Qt::Key_Backtab), QStringLiteral("Ctrl+Shift+Tab"));
    QCOMPARE(ChromeKeyOf(false, true, false, 0x26, Qt::Key_Up), QStringLiteral("Alt+Up"));
    QCOMPARE(ChromeKeyOf(true, false, false, 0x59, Qt::Key_Y), QStringLiteral("Ctrl+Y"));
    QCOMPARE(ChromeKeyOf(true, false, true, 0, Qt::Key_Y), QStringLiteral("Ctrl+Shift+Y"));
    QCOMPARE(ChromeKeyOf(true, false, true, 0, Qt::Key_Backtab), QStringLiteral("Ctrl+Shift+Tab"));
    QCOMPARE(ChromeKeyOf(true, false, false, 0, Qt::Key_PageDown), QStringLiteral("Ctrl+PageDown"));
    QCOMPARE(ChromeKeyOf(false, false, true, 0x59, Qt::Key_Y), QString());
    QCOMPARE(ChromeKeyOf(true, false, false, 0x74, Qt::Key_F5), QString());
    QCOMPARE(ChromeKeyOf(true, false, false, 0, Qt::Key_F5), QString());
    QCOMPARE(QtKeyTextOf(QStringLiteral("Ctrl+Shift+Comma")), QStringLiteral("Ctrl+Shift+,"));
    QCOMPARE(QtKeyTextOf(QStringLiteral("Alt+PageDown")), QStringLiteral("Alt+PgDown"));
    QCOMPARE(QtKeyTextOf(QStringLiteral("Ctrl+Delete")), QStringLiteral("Ctrl+Del"));
    QCOMPARE(QtKeyTextOf(QStringLiteral("Ctrl+Shift+Y")), QStringLiteral("Ctrl+Shift+Y"));
    QCOMPARE(RouteKey(true, false, true), KeyRoute::Vanilla);
    QCOMPARE(RouteKey(false, true, true), KeyRoute::Vanilla);
    QCOMPARE(RouteKey(false, false, true), KeyRoute::Extension);
    QCOMPARE(RouteKey(false, false, false), KeyRoute::Pass);
}

void tst_extensionui::aNotificationIsTakenInChromesWordsOrRefusedInThem(){
    using ExtensionUi::NoticeOf;
    const QJsonObject whole = QJsonDocument::fromJson(
        "{\"type\":\"basic\",\"iconUrl\":\"i.png\",\"title\":\"T\",\"message\":\"M\"}").object();
    QString error;
    QCOMPARE(NoticeOf(whole, QJsonObject(), true, &error), whole);
    QVERIFY(error.isEmpty());

    for(const char *missing : { "type", "iconUrl", "title", "message" }){
        QJsonObject lacking = whole;
        lacking.remove(QLatin1String(missing));
        QVERIFY(NoticeOf(lacking, QJsonObject(), true, &error).isEmpty());
        QCOMPARE(error, QStringLiteral("Some of the required properties are missing: type, iconUrl, title and message."));
    }
    QJsonObject fancy = whole;
    fancy[QStringLiteral("type")] = QStringLiteral("fancy");
    QVERIFY(NoticeOf(fancy, QJsonObject(), true, &error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("basic, image, list, progress")));
    QJsonObject numbered = whole;
    numbered[QStringLiteral("title")] = 1;
    QVERIFY(NoticeOf(numbered, QJsonObject(), true, &error).isEmpty());
    QVERIFY(error.contains(QStringLiteral("'title'")));

    const QJsonObject later = QJsonDocument::fromJson("{\"message\":\"N\",\"contextMessage\":\"C\"}").object();
    const QJsonObject merged = NoticeOf(later, whole, false, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(merged.value(QStringLiteral("title")).toString(), QStringLiteral("T"));
    QCOMPARE(merged.value(QStringLiteral("message")).toString(), QStringLiteral("N"));

    QCOMPARE(ExtensionUi::NoticeTitle(QStringLiteral("Vimium"), merged), QStringLiteral("Vimium: T"));
    QCOMPARE(ExtensionUi::NoticeTitle(QStringLiteral("Vimium"), QJsonObject()), QStringLiteral("Vimium"));
    QCOMPARE(ExtensionUi::NoticeText(merged), QStringLiteral("N\nC"));
    QJsonObject lengthy = whole;
    lengthy[QStringLiteral("message")] = QString(5000, QLatin1Char('x'));
    QVERIFY(ExtensionUi::NoticeText(lengthy).size() < 1100);
}

void tst_extensionui::aNotificationsAnswerCountsOnlyWhileItIsTheOneOnShow(){
    using ExtensionUi::Notices;
    Notices notices;
    const QString ext = QStringLiteral("e"), other = QStringLiteral("o");

    const Notices::Added a = notices.Add(ext, QStringLiteral("a"), QJsonObject(), QStringLiteral("1"));
    QVERIFY(a.serial && !a.replaced && a.pushedOut.isEmpty());
    const Notices::Added again = notices.Add(ext, QStringLiteral("a"), QJsonObject(), QStringLiteral("1"));
    QCOMPARE(again.replaced, a.serial);
    QVERIFY(again.serial != a.serial);
    QVERIFY2(!notices.Take(ext, QStringLiteral("a"), a.serial), "the replaced dialog's answer took out the new one");
    QCOMPARE(notices.SerialOf(ext, QStringLiteral("a")), again.serial);

    notices.Add(ext, QStringLiteral("b"), QJsonObject(), QStringLiteral("1"));
    notices.Add(ext, QStringLiteral("c"), QJsonObject(), QStringLiteral("1"));
    notices.Add(other, QStringLiteral("a"), QJsonObject(), QStringLiteral("9"));
    const Notices::Added d = notices.Add(ext, QStringLiteral("d"), QJsonObject(), QStringLiteral("1"));
    QCOMPARE(d.pushedOut.size(), 1);
    QCOMPARE(d.pushedOut.first().id, QStringLiteral("a"));
    QCOMPARE(d.pushedOut.first().serial, again.serial);
    QCOMPARE(notices.Ids(ext), QStringList({ QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d") }));
    QCOMPARE(notices.Ids(other), QStringList({ QStringLiteral("a") }));
    QVERIFY(!notices.Take(ext, QStringLiteral("a"), again.serial));

    const quint64 b = notices.SerialOf(ext, QStringLiteral("b"));
    QVERIFY(notices.Take(ext, QStringLiteral("b"), b));
    QVERIFY(!notices.Take(ext, QStringLiteral("b"), b));
    QVERIFY(!notices.Take(ext, QStringLiteral("a"), notices.SerialOf(other, QStringLiteral("a"))));

    QCOMPARE(notices.VersionOf(other), QStringLiteral("9"));
    QCOMPARE(notices.Drop(ext).size(), 2);
    QVERIFY(notices.Ids(ext).isEmpty());
    QCOMPARE(notices.Extensions(), QStringList({ other }));
}

QTEST_MAIN(tst_extensionui)
#include "tst_extensionui.moc"
