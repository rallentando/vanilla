#include "switch.hpp"

#include <QtTest>
#include <QRegularExpression>
#include <functional>
#include <memory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "extensionhostwire.hpp"

#include "testsupport.hpp"

class tst_extensionhostwire : public QObject {
    Q_OBJECT

private slots:
    void aUserScriptWorldOpensWithASecretOfItsOwn();
    void aUserScriptsMessageIsAnsweredOnceByWhoeverWasAsked();
    void aWorkerStartedAgainIsListenedToStill();
    void whatTheUserDidIsHeldWhileTheWorkerIsWoken();
    void aWorkerWhichSleepsIsWokenForWhatItListensTo();
    void aUserScriptsMessageWaitsForTheWorkerWoken();
    void initTestCase(){ TestSupport::SilenceDebugOutput(); }

    void thereIsOneAddressAndAHeaderIsNamedOnce();
    void aProfileIsOfTheSpaceItsKeyNames();
    void onlyAnExtensionsOriginIsAnId_data();
    void onlyAnExtensionsOriginIsAnId();
    void theKeyIsOfTheSecretAndTheId();
    void whoIsLetIn_data();
    void whoIsLetIn();
    void whatIsAskedComesInAHeader_data();
    void whatIsAskedComesInAHeader();
    void aNodeWithoutAViewIsOfItsSpaceUnlessPrivate();
    void aViewWhichCannotSayWhoseItIsGoesByItsDirectory();
    void matchPatterns_data();
    void matchPatterns();
    void aQueryIsAnsweredOrRefusedWhole_data();
    void aQueryIsAnsweredOrRefusedWhole();
    void aTabAsItIsTold();
    void aTabByItsId();
    void whatNobodyAnswersForIsAFailure();
    void whereADocumentIsNamed();
    void aStampIsOfThisProcessAndOneView_data();
    void aStampIsOfThisProcessAndOneView();
    void theFirstToNameANumberHasIt();
    void aViewWhichNamesWithoutEndIsLimited();
    void whichTabADocumentIsOf();
    void whereAnExtensionMaySendATab_data();
    void whereAnExtensionMaySendATab();
    void anAddressIsOneOfTheFourKindsOrNothing();
    void whatAPopupMayOpen_data();
    void whatAPopupMayOpen();
    void whatChangesTheTreeIsReadAndNotDone_data();
    void whatChangesTheTreeIsReadAndNotDone();
    void whatAnActHolds();
    void whatMayBeSearchedWith();
    void whatASessionOrAMoveIsAnsweredWith();
    void theOneWindow();
    void whatAnActIsAnsweredWith();
    void oneAtATimeInTheOrderTheyCame();
    void aDrainInsideADrainDoesNothing();
    void whatMayNotRunNowWaits();
    void thereIsRoomForSoManyAndNoMore();
    void oneTimerServesTheLine();
    void theOneWindowIsNotMovedAndSaysSo();
    void onlyATokenNamesWhoListens();
    void whatIsDifferentIsToldInOrder();
    void whereATabSitsIsNoChangeOfTheTab();
    void withoutThePermissionAnAddressIsNeitherToldNorCompared();
    void aSightSeesWhatItsHostPermissionsCover();
    void hostPermissionsTellTheTabsTheyCover();
    void anAddressComingIntoSightIsToldWholeAndGoingOutOfItNothing();
    void toATabTheAskerMayNotSeeNothingIsSaid();
    void theFocusOfTheWindowIsToldByItsNumbers();
    void whoWasToldNothingIsToldWhereItWoke();
    void whoWaitsIsAnsweredOnlyWhenThereIsSomethingToSay();
    void aClickIsToldOnceToWhoeverOfTheExtensionWaits();
    void anOffscreenDocumentIsOfTheExtensionsOwnAddressOnly();
    void whatHappensBetweenTwoCallsIsToldToTheNextOne();
    void aCallWhichComesTwiceMakesTheOlderOneStale();
    void beyondTheRoomACallIsRefusedAndNobodyIsPutOut();
    void aRequestWhichWentTakesItsSubscriptionAlong();
    void whoDoesNotCallAgainIsForgottenAfterAWhile();
    void whatAnExtensionSeesIsAskedWhenItIsAnswered();
    void tabsWhichOnlyChangedPlacesAreToldByTheOrder();
    void theFocusIsNotToldWhileTheUserIsWhereTheAskerMayNotSee();
    void historyIsTheLeavesNewestFirst_data();
    void historyIsTheLeavesNewestFirst();
    void aHistoryItemAsItIsTold();
    void theTreeIsToldAsBookmarks();
    void aBookmarkAsItIsTold();
    void aDateIsNoChangeOfATab();
    void bookmarksAreReadByIdAndByWord_data();
    void bookmarksAreReadByIdAndByWord();
    void aBookmarkReadIsShapedAsTheTreeIs();
    void aVisitIsToldToWhoHasTheHistoryPermission();
    void theFontsAreToldAsChromeNamesThem();
    void theTopSitesAreTheAddressesLookedAtLast();
    void theEventsAppliedInTurnGiveTheListAfter();
    void aTabWhichMovedIsToldOfItself();
    void aLoadIsToldAsItBeginsAndAsItEnds();
    void aZoomAndAHighlightAreToldInChromesShape();
    void whatIsPrunedIsLetGoOfAfterTheWalk();
    void aHeldCallLetGoOfByItsRelayIsAnsweredAbortedAndTheSubscriptionStays();
    void aMenuChoiceIsTakenOnceByItsOwnPage();
    void aMenuChoiceGoesStaleAndTheOldestGoesWhenTheRoomRunsOut();
    void aMenuTabCallNamesItsPage();
    void aCaptureIsReadForItsShape();
    void aCaptureIsOfWhatThePermissionReaches();
    void anActiveTabIsGrantedForOneLoad();
    void anActiveTabIsGrantedByTheUserAlone();
    void aSidePanelButtonIsThisViewsAndThisWindows();
    void aLoginIsReadForItsShape_data();
    void aLoginIsReadForItsShape();
    void aLoginIsSentBackToItsOwnAddressOnly();
    void aZoomCallIsReadAsChromeTakesIt();
    void onlyAnExtensionsRulesTravelInTheBody();

private:
    static const QString ID;
    static QByteArray Secret(){ return QByteArray(32, 's'); }
    static QUrl Origin(const QString &id = ID){ return QUrl(QStringLiteral("chrome-extension://") + id); }
    static QByteArray Header(const QString &json){ return QUrl::toPercentEncoding(json); }
    static QList<ExtensionHostWire::Tab> Tabs(){
        QList<ExtensionHostWire::Tab> tabs;
        auto tab = [&](qint64 id, const char *url, const char *title){
            ExtensionHostWire::Tab t; t.id = id; t.url = QUrl(QString::fromLatin1(url)); t.title = QString::fromLatin1(title); tabs << t; };
        tab(11, "https://a.example/one?x=1", "one");
        tab(12, "https://sub.b.example/two", "two");
        tab(15, "http://c.example/", "three");
        tab(20, "file:///C:/notes.html", "four");
        tabs[1].active = true;
        tabs[2].discarded = true;
        tabs[3].audible = true; tabs[3].muted = true;
        return tabs;
    }
    static QString Ids(const QJsonObject &answer){
        if(!answer[QStringLiteral("ok")].toBool()) return QStringLiteral("ERROR");
        QStringList ids;
        foreach(const QJsonValue &tab, answer[QStringLiteral("value")].toArray()) ids << QString::number(tab.toObject()[QStringLiteral("id")].toInt());
        return ids.join(QLatin1Char(','));
    }
    static QJsonObject Ask(const QString &api, const QString &args,
                           const ExtensionHostWire::Sight &sight = ExtensionHostWire::Sight::All()){
        return ExtensionHostWire::Answer(ExtensionHostWire::ParseCall(Header(QStringLiteral("{\"api\":\"%1\",\"args\":%2}").arg(api, args))), Tabs(), sight);
    }
};

const QString tst_extensionhostwire::ID = QStringLiteral("dbepggeogbaibhgnhhndojpepiihcmeb");

void tst_extensionhostwire::thereIsOneAddressAndAHeaderIsNamedOnce(){
    using ExtensionHostWire::IsCallUrl;
    QVERIFY(IsCallUrl(QUrl(QStringLiteral("vanilla-extension://host/call"))));
    foreach(const QString &other, QStringList()
            << QStringLiteral("vanilla-extension://host/") << QStringLiteral("vanilla-extension://host/call/x")
            << QStringLiteral("vanilla-extension://other/call") << QStringLiteral("vanilla-extension://host:80/call")
            << QStringLiteral("vanilla-extension://u@host/call") << QStringLiteral("vanilla-extension://host/call?key=k")
            << QStringLiteral("vanilla-extension://host/call#answer")
            << QStringLiteral("vanilla://host/call") << QStringLiteral("https://host/call"))
        QVERIFY2(!IsCallUrl(QUrl(other)), qPrintable(other));

    QMap<QByteArray, QByteArray> headers;
    headers.insert("Accept", "*/*");
    headers.insert("x-vanilla-KEY", "  abc \t");
    QCOMPARE(ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::KEY_HEADER), QByteArray("abc"));
    QCOMPARE(ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::CALL_HEADER), QByteArray());
    headers.insert("X-Vanilla-Key", "def");
    QCOMPARE(ExtensionHostWire::HeaderOf(headers, ExtensionHostWire::KEY_HEADER), QByteArray());

    QCOMPARE(ExtensionHostWire::SecretOf(QByteArray(32, 's')), QByteArray(32, 's'));
    QVERIFY(ExtensionHostWire::SecretOf(QByteArray()).isEmpty());
    QVERIFY(ExtensionHostWire::SecretOf(QByteArray(31, 's')).isEmpty());
    QVERIFY(ExtensionHostWire::SecretOf(QByteArray(33, 's')).isEmpty());
}

void tst_extensionhostwire::aProfileIsOfTheSpaceItsKeyNames(){
    using ExtensionHostWire::SpaceOfProfileKey;
    QCOMPARE(SpaceOfProfileKey(QStringLiteral("root")), QStringLiteral("root"));
    QCOMPARE(SpaceOfProfileKey(QStringLiteral("quick:root")), QStringLiteral("root"));
    QCOMPARE(SpaceOfProfileKey(QStringLiteral("quick:work")), QStringLiteral("work"));
    QCOMPARE(SpaceOfProfileKey(QString()), QString());
    QCOMPARE(SpaceOfProfileKey(QStringLiteral("quick-private:root")), QString());
    QCOMPARE(SpaceOfProfileKey(QStringLiteral("somebody:root")), QString());
    QVERIFY(ExtensionHostWire::NodeIsOfProfile(QStringList(), QStringLiteral("root"), SpaceOfProfileKey(QStringLiteral("quick:root"))));
    QVERIFY(!ExtensionHostWire::NodeIsOfProfile(QStringList(), QStringLiteral("root"), SpaceOfProfileKey(QStringLiteral("quick-private:root"))));
}

void tst_extensionhostwire::onlyAnExtensionsOriginIsAnId_data(){
    QTest::addColumn<QString>("initiator");
    QTest::addColumn<QString>("id");
    QTest::newRow("an extension") << QStringLiteral("chrome-extension://") + ID << ID;
    QTest::newRow("with the slash of a root") << QStringLiteral("chrome-extension://") + ID + QStringLiteral("/") << ID;
    QTest::newRow("a path") << QStringLiteral("chrome-extension://") + ID + QStringLiteral("/pages/options.html") << QString();
    QTest::newRow("a port") << QStringLiteral("chrome-extension://") + ID + QStringLiteral(":80") << QString();
    QTest::newRow("a user") << QStringLiteral("chrome-extension://u@") + ID << QString();
    QTest::newRow("a query") << QStringLiteral("chrome-extension://") + ID + QStringLiteral("?x") << QString();
    QTest::newRow("a page") << QStringLiteral("https://dbepggeogbaibhgnhhndojpepiihcmeb") << QString();
    QTest::newRow("a page named like the scheme") << QStringLiteral("https://chrome-extension/") << QString();
    QTest::newRow("an opaque origin") << QStringLiteral("null") << QString();
    QTest::newRow("nobody") << QString() << QString();
    QTest::newRow("too short") << QStringLiteral("chrome-extension://dbepggeogbaibhgn") << QString();
    QTest::newRow("letters past p") << QStringLiteral("chrome-extension://zbepggeogbaibhgnhhndojpepiihcmeb") << QString();
    QTest::newRow("the application's own scheme") << QStringLiteral("vanilla-extension://host") << QString();
}

void tst_extensionhostwire::onlyAnExtensionsOriginIsAnId(){
    QFETCH(QString, initiator);
    QFETCH(QString, id);
    QCOMPARE(ExtensionHostWire::ExtensionIdOf(QUrl(initiator)), id);
}

void tst_extensionhostwire::theKeyIsOfTheSecretAndTheId(){
    const QByteArray key = ExtensionHostWire::KeyFor(Secret(), ID);
    QCOMPARE(key.size(), 64);
    QVERIFY(QRegularExpression(QStringLiteral("\\A[0-9a-f]{64}\\z")).match(QString::fromLatin1(key)).hasMatch());
    QCOMPARE(ExtensionHostWire::KeyFor(Secret(), ID), key);
    QVERIFY(ExtensionHostWire::KeyFor(Secret(), QStringLiteral("abcdefghijklmnopabcdefghijklmnop")) != key);
    QVERIFY(ExtensionHostWire::KeyFor(QByteArray(32, 't'), ID) != key);
    QVERIFY(!key.contains(Secret().toHex()));
    QVERIFY(ExtensionHostWire::KeyFor(QByteArray(), ID).isEmpty());
    QVERIFY(ExtensionHostWire::KeyFor(QByteArray(8, 's'), ID).isEmpty());
    QVERIFY(ExtensionHostWire::KeyFor(Secret(), QString()).isEmpty());

    QVERIFY(ExtensionHostWire::SameKey(key, QByteArray(key)));
    QByteArray other = key; other[63] = other.at(63) == '0' ? '1' : '0';
    QVERIFY(!ExtensionHostWire::SameKey(key, other));
    QVERIFY(!ExtensionHostWire::SameKey(key, key.left(63)));
    QVERIFY(!ExtensionHostWire::SameKey(QByteArray(), QByteArray()));
}

void tst_extensionhostwire::whoIsLetIn_data(){
    QTest::addColumn<QByteArray>("method");
    QTest::addColumn<QUrl>("initiator");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<bool>("shimmed");
    QTest::addColumn<bool>("secret");
    QTest::addColumn<bool>("admitted");
    const QByteArray key = ExtensionHostWire::KeyFor(Secret(), ID), post = "POST";
    QTest::newRow("the extension's shim") << post << Origin() << key << true << true << true;
    QTest::newRow("a GET") << QByteArray("GET") << Origin() << key << true << true << false;
    QTest::newRow("a HEAD") << QByteArray("HEAD") << Origin() << key << true << true << false;
    QTest::newRow("a page which has the key") << post << QUrl(QStringLiteral("https://evil.example")) << key << true << true << false;
    QTest::newRow("nobody, with the key") << post << QUrl() << key << true << true << false;
    QTest::newRow("an extension this profile does not run from a copy") << post << Origin() << key << false << true << false;
    QTest::newRow("no key") << post << Origin() << QByteArray() << true << true << false;
    QTest::newRow("another extension's key") << post << Origin()
        << ExtensionHostWire::KeyFor(Secret(), QStringLiteral("abcdefghijklmnopabcdefghijklmnop")) << true << true << false;
    QTest::newRow("the key in capitals") << post << Origin() << key.toUpper() << true << true << false;
    QTest::newRow("no secret, no key") << post << Origin() << QByteArray() << true << false << false;
    QTest::newRow("no secret, a key of before") << post << Origin() << key << true << false << false;
}

void tst_extensionhostwire::whoIsLetIn(){
    QFETCH(QByteArray, method);
    QFETCH(QUrl, initiator);
    QFETCH(QByteArray, key);
    QFETCH(bool, shimmed);
    QFETCH(bool, secret);
    QFETCH(bool, admitted);
    const QSet<QString> ids = shimmed ? QSet<QString>() << ID : QSet<QString>() << QStringLiteral("abcdefghijklmnopabcdefghijklmnop");
    QCOMPARE(ExtensionHostWire::Admit(method, initiator, key, ids, secret ? Secret() : QByteArray()), admitted ? ID : QString());
}

void tst_extensionhostwire::whatIsAskedComesInAHeader_data(){
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QString>("api");
    QTest::newRow("a call") << Header(QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"title\":\"\u65e5\u672c\"}]}")) << QStringLiteral("tabs.query");
    QTest::newRow("nothing") << QByteArray() << QString();
    QTest::newRow("too long") << Header(QStringLiteral("{\"api\":\"tabs.query\",\"args\":[\"%1\"]}").arg(QString(ExtensionHostWire::CALL_LIMIT, QLatin1Char('a')))) << QString();
    QTest::newRow("no JSON") << QByteArray("tabs.query") << QString();
    QTest::newRow("not an object") << Header(QStringLiteral("[\"tabs.query\"]")) << QString();
    QTest::newRow("an api which is no word") << Header(QStringLiteral("{\"api\":7,\"args\":[]}")) << QString();
    QTest::newRow("no args") << Header(QStringLiteral("{\"api\":\"tabs.query\"}")) << QString();
    QTest::newRow("args which are no list") << Header(QStringLiteral("{\"api\":\"tabs.query\",\"args\":{}}")) << QString();
    QTest::newRow("an escape which is none") << QByteArray("%7B%22api%ZZ") << QString();
}

void tst_extensionhostwire::whatIsAskedComesInAHeader(){
    QFETCH(QByteArray, header);
    QFETCH(QString, api);
    const ExtensionHostWire::Call call = ExtensionHostWire::ParseCall(header);
    QCOMPARE(call.api, api);
    QCOMPARE(call.error.isEmpty(), !api.isEmpty());
    if(api.isEmpty()) QCOMPARE(ExtensionHostWire::Answer(call, Tabs(), ExtensionHostWire::Sight::All())[QStringLiteral("ok")].toBool(true), false);
    else QCOMPARE(call.args.at(0).toObject()[QStringLiteral("title")].toString(), QStringLiteral("\u65e5\u672c"));
}

void tst_extensionhostwire::aViewWhichCannotSayWhoseItIsGoesByItsDirectory(){
    using ExtensionHostWire::TabOwner;
    int asked = 0;
    auto says = [&](bool word){ return [&asked, word](){ asked++; return word; }; };
    QVERIFY(ExtensionHostWire::TabIsTold(TabOwner::NotYet, says(true)));
    QVERIFY(!ExtensionHostWire::TabIsTold(TabOwner::NotYet, says(false)));
    QCOMPARE(asked, 2);
    QVERIFY(ExtensionHostWire::TabIsTold(TabOwner::Asking, says(false)));
    QVERIFY(!ExtensionHostWire::TabIsTold(TabOwner::Other, says(true)));
    QCOMPARE(asked, 2);
}

void tst_extensionhostwire::aNodeWithoutAViewIsOfItsSpaceUnlessPrivate(){
    const QString space = QStringLiteral("work");
    auto of = [&](const QStringList &settings, const QString &nodeSpace = QStringLiteral("work")){
        return ExtensionHostWire::NodeIsOfProfile(settings, nodeSpace, space); };
    QVERIFY(of(QStringList()));
    QVERIFY(of(QStringList() << QStringLiteral("WebEngineView") << QStringLiteral("!Javascript")));
    foreach(const QString &word, QStringList() << QStringLiteral("Private") << QStringLiteral("private") << QStringLiteral("OffTheRecord") << QStringLiteral("offTheRecord"))
        QVERIFY2(!of(QStringList() << word), qPrintable(word));
    QVERIFY(of(QStringList() << QStringLiteral("Private") << QStringLiteral("!Private")));
    QVERIFY(of(QStringList() << QStringLiteral("!private") << QStringLiteral("private")));
    QVERIFY(of(QStringList() << QStringLiteral("PrivateNotes")));
    QVERIFY(!of(QStringList(), QStringLiteral("home")));
    QVERIFY(!ExtensionHostWire::NodeIsOfProfile(QStringList(), QString(), QString()));

    QVERIFY(of(QStringList() << QStringLiteral("!Private")));
    QVERIFY(of(QStringList() << QStringLiteral("!OffTheRecord") << QStringLiteral("WebEngineView")));
}

void tst_extensionhostwire::matchPatterns_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("result");
    auto row = [](const char *name, const char *pattern, const char *url, int result){
        QTest::newRow(name) << QString::fromLatin1(pattern) << QString::fromUtf8(url) << result; };
    row("all urls, https", "<all_urls>", "https://a.example/x", 1);
    row("all urls, file", "<all_urls>", "file:///C:/x.html", 1);
    row("all urls, an extension's page", "<all_urls>", "chrome-extension://abc/x.html", 0);
    row("all urls, the application's page", "<all_urls>", "vanilla://settings/", 0);
    row("any scheme is http", "*://a.example/*", "http://a.example/x", 1);
    row("any scheme is https", "*://a.example/*", "https://a.example/", 1);
    row("any scheme is not file", "*://*/*", "file:///C:/x", 0);
    row("a scheme", "https://a.example/*", "http://a.example/x", 0);
    row("any host", "https://*/*", "https://sub.b.example/two", 1);
    row("a domain and what is under it", "https://*.b.example/*", "https://sub.b.example/two", 1);
    row("a domain itself", "https://*.b.example/*", "https://b.example/", 1);
    row("a domain is not a suffix of a name", "https://*.b.example/*", "https://notb.example/", 0);
    row("a host in capitals", "https://A.Example/*", "https://a.example/x", 1);
    row("a path", "https://a.example/one*", "https://a.example/one?x=1", 1);
    row("a path to its end", "https://a.example/one", "https://a.example/one?x=1", 0);
    row("a path with a dot which is a dot", "https://a.example/a.b", "https://a.example/axb", 0);
    row("a star in the middle", "https://a.example/*/edit", "https://a.example/doc/7/edit", 1);
    row("an address with no path", "https://a.example/*", "https://a.example", 1);
    row("a file", "file:///*", "file:///C:/notes.html", 1);
    row("no pattern: a word", "example", "https://a.example/", -1);
    row("no pattern: no path", "https://a.example", "https://a.example/", -1);
    row("no pattern: a star in a host", "https://a.*.example/*", "https://a.b.example/", -1);
    row("no pattern: a port", "https://a.example:8080/*", "https://a.example:8080/", -1);
    row("no pattern: a scheme nobody matches", "chrome://*/*", "chrome://settings/", -1);
    row("all urls, a socket", "<all_urls>", "wss://a.example/x", 0);
    row("no pattern: a socket", "wss://a.example/*", "wss://a.example/x", -1);
    row("a name which is not ASCII", "https://xn--wgv71a119e.jp/*", "https://\u65e5\u672c\u8a9e.jp/", 1);
    row("no pattern: no host", "https:///x", "https://a.example/x", -1);
}

void tst_extensionhostwire::matchPatterns(){
    QFETCH(QString, pattern);
    QFETCH(QString, url);
    QFETCH(int, result);
    bool ok = false;
    const bool matched = ExtensionHostWire::Matches(pattern, QUrl(url), &ok);
    QCOMPARE(ok ? (matched ? 1 : 0) : -1, result);
}

void tst_extensionhostwire::aQueryIsAnsweredOrRefusedWhole_data(){
    QTest::addColumn<QString>("filter");
    QTest::addColumn<QString>("ids");
    auto row = [](const char *name, const char *filter, const char *ids){
        QTest::newRow(name) << QString::fromLatin1(filter) << QString::fromLatin1(ids); };
    row("everything", "{}", "11,12,15,20");
    row("the active one", "{\"active\":true}", "12");
    row("the ones which are not", "{\"active\":false}", "11,15,20");
    row("as Stands asks", "{\"active\":true,\"currentWindow\":true,\"lastFocusedWindow\":true}", "12");
    row("as Vimium asks", "{\"currentWindow\":true}", "11,12,15,20");
    row("the window by its number", "{\"windowId\":1}", "11,12,15,20");
    row("the window by WINDOW_ID_CURRENT", "{\"windowId\":-2,\"active\":true}", "12");
    row("a window which is not", "{\"windowId\":7}", "");
    row("a window other than the current", "{\"currentWindow\":false}", "");
    row("a window other than the last focused", "{\"lastFocusedWindow\":false}", "");
    row("a normal window", "{\"windowType\":\"normal\"}", "11,12,15,20");
    row("a popup", "{\"windowType\":\"popup\"}", "");
    row("highlighted is active", "{\"highlighted\":true}", "12");
    row("nothing is pinned", "{\"pinned\":true}", "");
    row("everything is unpinned", "{\"pinned\":false}", "11,12,15,20");
    row("nothing is in a group", "{\"groupId\":3}", "");
    row("everything is in no group", "{\"groupId\":-1}", "11,12,15,20");
    row("by its place", "{\"index\":2}", "15");
    row("what makes a sound", "{\"audible\":true}", "20");
    row("what is muted", "{\"muted\":true}", "20");
    row("what has nothing loaded", "{\"discarded\":true}", "15");
    row("status unloaded is the same", "{\"status\":\"unloaded\"}", "15");
    row("status complete is the rest", "{\"status\":\"complete\"}", "11,12,20");
    row("nothing is ever told of as loading", "{\"status\":\"loading\"}", "");
    row("everything may be discarded", "{\"autoDiscardable\":false}", "");
    row("by address", "{\"url\":\"https://*.b.example/*\"}", "12");
    row("by any of several", "{\"url\":[\"http://*/*\",\"file:///*\"]}", "15,20");
    row("as Stands asks after it is installed", "{\"url\":\"<all_urls>\"}", "11,12,15,20");
    row("and with the rest", "{\"url\":\"<all_urls>\",\"active\":true}", "12");
    row("a title", "{\"title\":\"one\"}", "ERROR");
    row("frozen", "{\"frozen\":true}", "ERROR");
    row("a key nobody knows", "{\"active\":true,\"colour\":\"red\"}", "ERROR");
    row("a flag which is no flag", "{\"active\":\"yes\"}", "ERROR");
    row("a number which is none", "{\"windowId\":\"1\"}", "ERROR");
    row("a number which is not whole", "{\"index\":1.5}", "ERROR");
    row("a status nobody has", "{\"status\":\"sleeping\"}", "ERROR");
    row("a window type nobody has", "{\"windowType\":\"dialog\"}", "ERROR");
    row("a pattern which is none", "{\"url\":\"example\"}", "ERROR");
    row("a pattern which is none, among ones which are", "{\"url\":[\"<all_urls>\",7]}", "ERROR");
    row("no pattern at all", "{\"url\":[]}", "ERROR");
}

void tst_extensionhostwire::aQueryIsAnsweredOrRefusedWhole(){
    QFETCH(QString, filter);
    QFETCH(QString, ids);
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[%1]").arg(filter))), ids);

    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[]"))), QStringLiteral("ERROR"));
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[null]"))), QStringLiteral("ERROR"));
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{}, {}]"))), QStringLiteral("ERROR"));
}

void tst_extensionhostwire::aTabAsItIsTold(){
    const QJsonObject active = Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"active\":true}]"))[QStringLiteral("value")].toArray().at(0).toObject();
    QCOMPARE(active[QStringLiteral("id")].toInt(), 12);
    QCOMPARE(active[QStringLiteral("index")].toInt(), 1);
    QCOMPARE(active[QStringLiteral("windowId")].toInt(), 1);
    QCOMPARE(active[QStringLiteral("url")].toString(), QStringLiteral("https://sub.b.example/two"));
    QCOMPARE(active[QStringLiteral("title")].toString(), QStringLiteral("two"));
    QCOMPARE(active[QStringLiteral("status")].toString(), QStringLiteral("complete"));
    QCOMPARE(active[QStringLiteral("highlighted")].toBool(), true);
    QCOMPARE(active[QStringLiteral("incognito")].toBool(true), false);
    QCOMPARE(active[QStringLiteral("pinned")].toBool(true), false);
    QCOMPARE(active[QStringLiteral("mutedInfo")].toObject()[QStringLiteral("muted")].toBool(true), false);

    const QJsonArray all = Ask(QStringLiteral("tabs.query"), QStringLiteral("[{}]"))[QStringLiteral("value")].toArray();
    for(int i = 0; i < all.size(); i++) QCOMPARE(all.at(i).toObject()[QStringLiteral("index")].toInt(), i);
    QCOMPARE(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"audible\":true}]"))[QStringLiteral("value")].toArray().at(0).toObject()[QStringLiteral("index")].toInt(), 3);

    const QJsonObject blind = Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"active\":true}]"), ExtensionHostWire::Sight())[QStringLiteral("value")].toArray().at(0).toObject();
    QCOMPARE(blind[QStringLiteral("id")].toInt(), 12);
    QVERIFY(!blind.contains(QStringLiteral("url")));
    QVERIFY(!blind.contains(QStringLiteral("title")));
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"url\":\"<all_urls>\"}]"), ExtensionHostWire::Sight())), QStringLiteral("ERROR"));
}

void tst_extensionhostwire::aTabByItsId(){
    const QJsonObject found = Ask(QStringLiteral("tabs.get"), QStringLiteral("[15]"));
    QCOMPARE(found[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(found[QStringLiteral("value")].toObject()[QStringLiteral("index")].toInt(), 2);
    QCOMPARE(found[QStringLiteral("value")].toObject()[QStringLiteral("discarded")].toBool(), true);
    QVERIFY(!Ask(QStringLiteral("tabs.get"), QStringLiteral("[15]"), ExtensionHostWire::Sight())[QStringLiteral("value")].toObject().contains(QStringLiteral("url")));

    QCOMPARE(Ask(QStringLiteral("tabs.get"), QStringLiteral("[13]"))[QStringLiteral("error")].toString(), QStringLiteral("No tab with id: 13."));
    foreach(const QString &args, QStringList() << QStringLiteral("[]") << QStringLiteral("[\"15\"]") << QStringLiteral("[15.5]") << QStringLiteral("[15, 16]") << QStringLiteral("[null]"))
        QCOMPARE(Ask(QStringLiteral("tabs.get"), args)[QStringLiteral("ok")].toBool(true), false);
}

void tst_extensionhostwire::whatNobodyAnswersForIsAFailure(){
    const QJsonObject answer = Ask(QStringLiteral("tabs.move"), QStringLiteral("[12, {\"index\": 0}]"));
    QCOMPARE(answer[QStringLiteral("ok")].toBool(true), false);
    QCOMPARE(answer[QStringLiteral("error")].toString(), QStringLiteral("chrome.tabs.move is not available in this browser"));
    QVERIFY(!answer.contains(QStringLiteral("value")));
}

void tst_extensionhostwire::whereADocumentIsNamed(){
    using namespace ExtensionHostWire;
    QVERIFY(IsBindUrl(QUrl(QStringLiteral("vanilla-extension://host/bind"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://host/bind?n=0123"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://host/bind#0123"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://host/bind/"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://host:1/bind"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://u@host/bind"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://other/bind"))));
    QVERIFY(!IsBindUrl(QUrl(QStringLiteral("vanilla-extension://host/call"))));
    QVERIFY(!IsCallUrl(QUrl(QStringLiteral("vanilla-extension://host/bind"))));

    QVERIFY(IsNonce(QByteArrayLiteral("0123456789abcdef0123456789abcdef")));
    QVERIFY(!IsNonce(QByteArray()));
    QVERIFY(!IsNonce(QByteArrayLiteral("0123456789abcdef0123456789abcde")));
    QVERIFY(!IsNonce(QByteArrayLiteral("0123456789abcdef0123456789abcdef0")));
    QVERIFY(!IsNonce(QByteArrayLiteral("0123456789ABCDEF0123456789ABCDEF")));
    QVERIFY(!IsNonce(QByteArrayLiteral("0123456789abcdef0123456789abcde ")));
    QVERIFY(!IsNonce(QByteArrayLiteral("g123456789abcdef0123456789abcdef")));

    const QByteArray nonce = QByteArrayLiteral("0123456789abcdef0123456789abcdef");
    QVERIFY(IsBindRequest(QByteArrayLiteral("POST"), nonce));
    QVERIFY(!IsBindRequest(QByteArrayLiteral("GET"), nonce));
    QVERIFY(!IsBindRequest(QByteArrayLiteral("HEAD"), nonce));
    QVERIFY(!IsBindRequest(QByteArrayLiteral("post"), nonce));
    QVERIFY(!IsBindRequest(QByteArrayLiteral("POST"), QByteArrayLiteral("not-a-nonce")));
}

void tst_extensionhostwire::aStampIsOfThisProcessAndOneView_data(){
    using namespace ExtensionHostWire;
    QTest::addColumn<QByteArray>("stamp");
    QTest::addColumn<quint64>("view");

    const QByteArray good = Stamp(Secret(), 7);
    const QByteArray mac = good.mid(good.indexOf('.') + 1);
    QTest::newRow("as it was made") << good << quint64(7);
    QTest::newRow("not there") << QByteArray() << quint64(0);
    QTest::newRow("a number alone") << QByteArrayLiteral("7") << quint64(0);
    QTest::newRow("a number and a point") << QByteArrayLiteral("7.") << quint64(0);
    QTest::newRow("no number") << QByteArray('.' + mac) << quint64(0);
    QTest::newRow("another view, this one's mac") << QByteArray("8." + mac) << quint64(0);
    QTest::newRow("the same view, spelled otherwise") << QByteArray("07." + mac) << quint64(0);
    QTest::newRow("no view at all") << QByteArray("0." + mac) << quint64(0);
    QTest::newRow("another process' secret") << Stamp(QByteArray(32, 't'), 7) << quint64(0);
    QTest::newRow("one digit of the mac") << QByteArray(good.left(good.size() - 1) + (good.endsWith('0') ? '1' : '0')) << quint64(0);
    QTest::newRow("the mac cut short") << good.left(good.size() - 2) << quint64(0);
    QTest::newRow("something after it") << QByteArray(good + "0") << quint64(0);
    QTest::newRow("no number, a long one") << QByteArray("123456789012345678901234567890." + mac) << quint64(0);
}

void tst_extensionhostwire::aStampIsOfThisProcessAndOneView(){
    using namespace ExtensionHostWire;
    QFETCH(QByteArray, stamp);
    QFETCH(quint64, view);
    QCOMPARE(ViewOfStamp(Secret(), stamp), view);

    QVERIFY(Stamp(QByteArray(), 7).isEmpty());
    QVERIFY(Stamp(QByteArray(31, 's'), 7).isEmpty());
    QVERIFY(Stamp(QByteArray(33, 's'), 7).isEmpty());
    QVERIFY(Stamp(Secret(), 0).isEmpty());
    QCOMPARE(ViewOfStamp(QByteArray(), Stamp(Secret(), 7)), quint64(0));
    QVERIFY(Stamp(Secret(), 7) != Stamp(Secret(), 8));
}

static QByteArray NonceNumber(int n){
    return QByteArray::number(n, 16).rightJustified(32, '0');
}

void tst_extensionhostwire::theFirstToNameANumberHasIt(){
    using namespace ExtensionHostWire;
    BindTable table;
    QVERIFY(table.Bind(1, NonceNumber(10)));
    QVERIFY(table.Bind(1, NonceNumber(12)));
    QCOMPARE(table.ViewOf(NonceNumber(10)), quint64(1));
    QCOMPARE(table.ViewOf(NonceNumber(12)), quint64(1));
    QVERIFY(!table.Bind(2, NonceNumber(10)));
    QVERIFY(!table.Bind(1, NonceNumber(10)));
    QCOMPARE(table.ViewOf(NonceNumber(10)), quint64(1));
    QVERIFY(!table.Bind(0, NonceNumber(11)));
    QVERIFY(!table.Bind(1, QByteArrayLiteral("eleven")));
    QCOMPARE(table.ViewOf(NonceNumber(11)), quint64(0));
    QCOMPARE(table.ViewOf(QByteArray()), quint64(0));
    QCOMPARE(table.Count(), 2);

    QVERIFY(table.Bind(2, NonceNumber(20)));
    table.Forget(1);
    QCOMPARE(table.ViewOf(NonceNumber(10)), quint64(0));
    QCOMPARE(table.ViewOf(NonceNumber(12)), quint64(0));
    QCOMPARE(table.ViewOf(NonceNumber(20)), quint64(2));
    QCOMPARE(table.Count(), 1);
    QVERIFY(table.Bind(3, NonceNumber(10)));
}

void tst_extensionhostwire::aViewWhichNamesWithoutEndIsLimited(){
    using namespace ExtensionHostWire;
    BindTable table;
    QVERIFY(table.Bind(9, NonceNumber(1)));
    for(int n = 0; n < BIND_LIMIT; n++) QVERIFY(table.Bind(5, NonceNumber(1000 + n)));
    QCOMPARE(table.ViewOf(NonceNumber(1000)), quint64(5));
    QVERIFY(table.Bind(5, NonceNumber(1000 + BIND_LIMIT)));
    QCOMPARE(table.ViewOf(NonceNumber(1000)), quint64(5));
    QCOMPARE(table.ViewOf(NonceNumber(1001)), quint64(0));
    for(int n = BIND_LIMIT + 1; n < BIND_LIMIT * 3; n++) QVERIFY(table.Bind(5, NonceNumber(1000 + n)));
    QCOMPARE(table.ViewOf(NonceNumber(1)), quint64(9));
    QCOMPARE(table.Count(), BIND_LIMIT + 1);
    QCOMPARE(table.ViewOf(NonceNumber(1000)), quint64(0));
    QCOMPARE(table.ViewOf(NonceNumber(1000 + BIND_LIMIT * 2 - 1)), quint64(0));
    QCOMPARE(table.ViewOf(NonceNumber(1000 + BIND_LIMIT * 2)), quint64(5));
    QCOMPARE(table.ViewOf(NonceNumber(1000 + BIND_LIMIT * 3 - 1)), quint64(5));
    table.Forget(5);
    QCOMPARE(table.Count(), 1);
    QVERIFY(table.Bind(5, NonceNumber(1000)));
}

void tst_extensionhostwire::whichTabADocumentIsOf(){
    using namespace ExtensionHostWire;
    auto asked = [](const QByteArray &json){ return ParseCall(json.toPercentEncoding()); };
    const QByteArray nonce = NonceNumber(77);

    QCOMPARE(NonceOfTabOf(asked("{\"api\":\"vanilla.tabOf\",\"args\":[\"" + nonce + "\"]}")), nonce);
    QVERIFY(NonceOfTabOf(asked("{\"api\":\"tabs.get\",\"args\":[\"" + nonce + "\"]}")).isEmpty());
    QVERIFY(NonceOfTabOf(asked("{\"api\":\"vanilla.tabOf\",\"args\":[]}")).isEmpty());
    QVERIFY(NonceOfTabOf(asked("{\"api\":\"vanilla.tabOf\",\"args\":[77]}")).isEmpty());
    QVERIFY(NonceOfTabOf(asked("{\"api\":\"vanilla.tabOf\",\"args\":[\"" + nonce + "\",1]}")).isEmpty());
    QVERIFY(NonceOfTabOf(asked("{\"api\":\"vanilla.tabOf\",\"args\":[\"seventy-seven\"]}")).isEmpty());
    QVERIFY(NonceOfTabOf(asked("not json")).isEmpty());

    const QJsonObject known = TabOfAnswer(42, 3);
    QCOMPARE(known.value(QStringLiteral("ok")).toBool(), true);
    const QJsonObject told = known.value(QStringLiteral("value")).toObject();
    QCOMPARE(told.keys(), QStringList() << QStringLiteral("id") << QStringLiteral("index"));
    QCOMPARE(told.value(QStringLiteral("id")).toInt(), 42);
    QCOMPARE(told.value(QStringLiteral("index")).toInt(), 3);
    QCOMPARE(TabOfAnswer(42, 0).value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(TabOfAnswer(0, 0).value(QStringLiteral("ok")).toBool(true), false);
    QCOMPARE(TabOfAnswer(-1, 0).value(QStringLiteral("ok")).toBool(true), false);
    QCOMPARE(TabOfAnswer(42, -1).value(QStringLiteral("ok")).toBool(true), false);
    QCOMPARE(Answer(asked("{\"api\":\"vanilla.tabOf\",\"args\":[\"" + nonce + "\"]}"), QList<Tab>(), ExtensionHostWire::Sight::All())
             .value(QStringLiteral("ok")).toBool(true), false);
}

void tst_extensionhostwire::whereAnExtensionMaySendATab_data(){
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("becomes");
    const QString own = QStringLiteral("chrome-extension://") + ID;
    QTest::newRow("https") << QStringLiteral("https://a.example/x?y=1#z") << QStringLiteral("https://a.example/x?y=1#z");
    QTest::newRow("http") << QStringLiteral("http://127.0.0.1:8765/") << QStringLiteral("http://127.0.0.1:8765/");
    QTest::newRow("about:blank") << QStringLiteral("about:blank") << QStringLiteral("about:blank");
    QTest::newRow("its own page, whole") << own + QStringLiteral("/pages/options.html") << own + QStringLiteral("/pages/options.html");
    QTest::newRow("its own page, from the root") << QStringLiteral("/index.html#no-more-cookie-banners") << own + QStringLiteral("/index.html#no-more-cookie-banners");
    QTest::newRow("its own page, relative") << QStringLiteral("pages/help.html") << own + QStringLiteral("/pages/help.html");

    QTest::newRow("javascript") << QStringLiteral("javascript:alert(1)") << QString();
    QTest::newRow("JaVaScRiPt") << QStringLiteral("JaVaScRiPt:alert(1)") << QString();
    QTest::newRow("file") << QStringLiteral("file:///C:/Windows/win.ini") << QString();
    QTest::newRow("the application's own page") << QStringLiteral("vanilla://settings/") << QString();
    QTest::newRow("the host itself") << QStringLiteral("vanilla-extension://host/call") << QString();
    QTest::newRow("chrome:") << QStringLiteral("chrome://newtab") << QString();
    QTest::newRow("data:") << QStringLiteral("data:text/html,<html></html>") << QString();
    QTest::newRow("view-source:") << QStringLiteral("view-source:https://a.example/") << QString();
    QTest::newRow("blob:") << QStringLiteral("blob:https://a.example/0-0") << QString();
    QTest::newRow("about:srcdoc") << QStringLiteral("about:srcdoc") << QString();
    QTest::newRow("about:blank with more") << QStringLiteral("about:blank#x") << QString();
    QTest::newRow("about:blank with a question") << QStringLiteral("about:blank?x") << QString();
    QTest::newRow("about://blank") << QStringLiteral("about://blank") << QString();
    QTest::newRow("https with no slashes") << QStringLiteral("https:evil.example") << QString();
    QTest::newRow("another extension's page") << QStringLiteral("chrome-extension://lgblnfidahcdcjddiepkckcfdhpknnjh/index.html") << QString();
    QTest::newRow("another extension's, without a scheme") << QStringLiteral("//lgblnfidahcdcjddiepkckcfdhpknnjh/index.html") << QString();
    QTest::newRow("its own with somebody in front") << QStringLiteral("chrome-extension://x@") + ID + QStringLiteral("/a.html") << QString();
    QTest::newRow("its own with a port") << own + QStringLiteral(":81/a.html") << QString();
    QTest::newRow("http with no host") << QStringLiteral("http:///x") << QString();
    QTest::newRow("nothing") << QString() << QString();
}

void tst_extensionhostwire::anAddressIsOneOfTheFourKindsOrNothing(){
    const QStringList texts = QStringList()
        << QStringLiteral(" javascript:alert(1)") << QStringLiteral("java\tscript:alert(1)") << QStringLiteral("JAVASCRIPT:alert(1)")
        << QStringLiteral("javascript\n:alert(1)") << QStringLiteral("\\\\evil.example\\x") << QStringLiteral("chrome-extension:/page.html")
        << QStringLiteral("chrome-extension://") + ID.toUpper() + QStringLiteral("/a.html") << QStringLiteral("chrome-extension://@") + ID + QStringLiteral("/a.html")
        << QStringLiteral("chrome-extension://") + ID + QStringLiteral(".evil.example/a.html") << QStringLiteral("?") << QStringLiteral("#")
        << QStringLiteral("%00") << QStringLiteral("http://a@b.example/") << QStringLiteral("about:BLANK") << QStringLiteral("vbscript:x")
        << QStringLiteral("filesystem:https://a.example/x") << QStringLiteral("ws://a.example/") << QStringLiteral("ftp://a.example/");
    foreach(const QString &text, texts){
        bool ok = false;
        const QUrl url = ExtensionHostWire::TabUrlOf(ID, text, &ok);
        if(!ok){ QVERIFY2(url.isEmpty(), qPrintable(text)); continue; }
        const QString scheme = url.scheme();
        QVERIFY2(scheme == QStringLiteral("http") || scheme == QStringLiteral("https")
                 || scheme == QStringLiteral("chrome-extension") || url == QUrl(QStringLiteral("about:blank")), qPrintable(text + QStringLiteral(" -> ") + url.toString()));
        if(scheme == QStringLiteral("chrome-extension")) QVERIFY2(url.host() == ID, qPrintable(text + QStringLiteral(" -> ") + url.toString()));
        QVERIFY2(!url.toEncoded().startsWith("javascript:"), qPrintable(text));
    }
}

void tst_extensionhostwire::whereAnExtensionMaySendATab(){
    QFETCH(QString, text);
    QFETCH(QString, becomes);
    bool ok = true;
    const QUrl url = ExtensionHostWire::TabUrlOf(ID, text, &ok);
    QCOMPARE(ok, !becomes.isEmpty());
    QCOMPARE(url.toString(QUrl::FullyEncoded), becomes);
    ExtensionHostWire::TabUrlOf(QString(), text, &ok);
    QCOMPARE(ok, false);
}

void tst_extensionhostwire::whatAPopupMayOpen_data(){
    QTest::addColumn<QString>("shown");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("becomes");
    const QString own = QStringLiteral("chrome-extension://") + ID;
    QTest::newRow("its settings, from its popup") << own + QStringLiteral("/index.html") << own + QStringLiteral("/index.html#/settings") << own + QStringLiteral("/index.html#/settings");
    QTest::newRow("the web, from its popup") << own + QStringLiteral("/index.html") << QStringLiteral("https://a.example/") << QStringLiteral("https://a.example/");
    QTest::newRow("the web, from the web") << QStringLiteral("https://b.example/") << QStringLiteral("https://a.example/") << QStringLiteral("https://a.example/");
    QTest::newRow("its page, from the web") << QStringLiteral("https://b.example/") << own + QStringLiteral("/index.html") << QString();
    QTest::newRow("its page, from another's") << QStringLiteral("chrome-extension://lgblnfidahcdcjddiepkckcfdhpknnjh/index.html") << own + QStringLiteral("/index.html") << QString();
    QTest::newRow("its page, from nothing") << QString() << own + QStringLiteral("/index.html") << QString();
    QTest::newRow("another's page, from its popup") << own + QStringLiteral("/index.html") << QStringLiteral("chrome-extension://lgblnfidahcdcjddiepkckcfdhpknnjh/index.html") << QString();
    QTest::newRow("javascript, from its popup") << own + QStringLiteral("/index.html") << QStringLiteral("javascript:alert(1)") << QString();
    QTest::newRow("a file, from its popup") << own + QStringLiteral("/index.html") << QStringLiteral("file:///C:/Windows/win.ini") << QString();
}

void tst_extensionhostwire::whatAPopupMayOpen(){
    QFETCH(QString, shown);
    QFETCH(QString, text);
    QFETCH(QString, becomes);
    bool ok = true;
    const QUrl url = ExtensionHostWire::PopupWindowUrlOf(ID, QUrl(shown), text, &ok);
    QCOMPARE(ok, !becomes.isEmpty());
    QCOMPARE(url.toString(QUrl::FullyEncoded), becomes);
}

void tst_extensionhostwire::whatChangesTheTreeIsReadAndNotDone_data(){
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("args");
    QTest::addColumn<QString>("error");
    auto row = [](const char *name, const char *api, const char *args, const char *error){
        QTest::newRow(name) << QString::fromLatin1(api) << QString::fromLatin1(args) << QString::fromLatin1(error); };
    row("create: Vimium's", "tabs.create", "[{\"windowId\":1,\"index\":1,\"active\":true,\"openerTabId\":12,\"url\":\"https://a.example/\"}]", "");
    row("create: Vimium's, with no address and no index", "tabs.create", "[{\"windowId\":1,\"index\":null,\"active\":true,\"openerTabId\":12}]", "");
    row("create: Stands'", "tabs.create", "[{\"url\":\"/index.html\",\"active\":true}]", "");
    row("create: the current window by its alias", "tabs.create", "[{\"windowId\":-2}]", "");
    row("create: nothing at all", "tabs.create", "[]", "");
    row("create: another window", "tabs.create", "[{\"windowId\":7}]", "No window with id: 7.");
    row("create: pinned", "tabs.create", "[{\"pinned\":true}]", "tabs.create: 'pinned' is not supported by this browser");
    row("create: selected", "tabs.create", "[{\"selected\":true}]", "tabs.create: 'selected' is not supported by this browser");
    row("create: a key of nobody's", "tabs.create", "[{\"colour\":\"red\"}]", "tabs.create: 'colour' is not a key of it");
    row("create: the wrong kind", "tabs.create", "[{\"active\":1}]", "tabs.create: 'active' has a value of the wrong kind");
    row("create: an index which is no number", "tabs.create", "[{\"index\":\"1\"}]", "tabs.create: 'index' has a value of the wrong kind");
    row("create: javascript", "tabs.create", "[{\"url\":\"javascript:alert(1)\"}]", "tabs.create: this browser does not let an extension open that address");
    row("create: an address which is no text", "tabs.create", "[{\"url\":123}]", "tabs.create: 'url' has a value of the wrong kind");
    row("create: an index nobody can count to", "tabs.create", "[{\"index\":1e300}]", "tabs.create: 'index' has a value of the wrong kind");
    row("create: an opener which is no tab", "tabs.create", "[{\"openerTabId\":0}]", "No tab with id: 0.");
    row("create: not an object", "tabs.create", "[\"https://a.example/\"]", "tabs.create takes one object");
    row("create: two", "tabs.create", "[{}, {}]", "tabs.create takes one object");

    row("update: make it the current one", "tabs.update", "[12, {\"active\":true}]", "");
    row("update: the current tab elsewhere", "tabs.update", "[{\"url\":\"https://a.example/\"}]", "");
    row("update: the same, the id not given", "tabs.update", "[null, {\"url\":\"https://a.example/\"}]", "");
    row("update: nothing to do", "tabs.update", "[12, {}]", "");
    row("update: pinned", "tabs.update", "[12, {\"pinned\":true}]", "tabs.update: 'pinned' is not supported by this browser");
    row("update: no object", "tabs.update", "[12]", "tabs.update takes an object");
    row("update: tab 0 is a tab somebody named", "tabs.update", "[0, {\"active\":true}]", "No tab with id: 0.");
    row("update: half a tab", "tabs.update", "[12.5, {}]", "tabs.update takes the id of a tab");
    row("update: file", "tabs.update", "[12, {\"url\":\"file:///C:/x\"}]", "tabs.update: this browser does not let an extension open that address");
    row("update: too much", "tabs.update", "[12, {}, 1]", "tabs.update was handed too much");

    row("remove: one", "tabs.remove", "[12]", "");
    row("remove: some", "tabs.remove", "[[11, 12, 12]]", "");
    row("remove: none", "tabs.remove", "[[]]", "");
    row("remove: tab 0", "tabs.remove", "[[11, 0]]", "No tab with id: 0.");
    row("remove: text", "tabs.remove", "[\"12\"]", "tabs.remove takes the id of a tab, or a list of them");
    row("remove: nothing", "tabs.remove", "[]", "tabs.remove takes the id of a tab, or a list of them");

    row("reload: Vimium's", "tabs.reload", "[12, {\"bypassCache\":false}]", "");
    row("reload: Stands'", "tabs.reload", "[12]", "");
    row("reload: the current tab", "tabs.reload", "[]", "");
    row("reload: the current tab, hard", "tabs.reload", "[{\"bypassCache\":true}]", "");
    row("reload: nobody said anything, twice", "tabs.reload", "[null, null]", "");
    row("update: and a callback which was not one", "tabs.update", "[12, {\"active\":true}, null]", "");
    row("create: and an argument nobody gave", "tabs.create", "[{}, null]", "");
    row("reload: tab 0, which Stands means no tab by", "tabs.reload", "[0]", "No tab with id: 0.");
    row("reload: a key of nobody's", "tabs.reload", "[12, {\"hard\":true}]", "tabs.reload: 'hard' is not a key of it");
    row("search: Vimium's, in a new tab", "search.query", "[{\"disposition\":\"NEW_TAB\",\"text\":\"a b\"}]", "");
    row("search: Vimium's, in the current tab", "search.query", "[{\"disposition\":\"CURRENT_TAB\",\"text\":\"a b\"}]", "");
    row("search: the text alone", "search.query", "[{\"text\":\"x\"}]", "");
    row("search: a new window is a new tab", "search.query", "[{\"text\":\"x\",\"disposition\":\"NEW_WINDOW\"}]", "");
    row("search: in a tab", "search.query", "[{\"text\":\"x\",\"tabId\":12}]", "");
    row("search: a disposition nobody gave", "search.query", "[{\"text\":\"x\",\"disposition\":null}]", "");
    row("search: and an argument nobody gave", "search.query", "[{\"text\":\"x\"}, null]", "");
    row("search: both", "search.query", "[{\"text\":\"x\",\"tabId\":12,\"disposition\":\"NEW_TAB\"}]", "search.query: 'disposition' and 'tabId' cannot both be given");
    row("search: nothing to look for", "search.query", "[{\"text\":\"\"}]", "search.query: 'text' is empty");
    row("search: no text", "search.query", "[{}]", "search.query: 'text' is required");
    row("search: text of the wrong kind", "search.query", "[{\"text\":1}]", "search.query: 'text' has a value of the wrong kind");
    row("search: nothing", "search.query", "[]", "search.query takes one object");
    row("search: bare text", "search.query", "[\"x\"]", "search.query takes one object");
    row("search: two", "search.query", "[{\"text\":\"x\"},{}]", "search.query takes one object");
    row("search: sideways", "search.query", "[{\"text\":\"x\",\"disposition\":\"SIDE\"}]", "search.query: 'disposition' has a value of the wrong kind");
    row("search: tab 0", "search.query", "[{\"text\":\"x\",\"tabId\":0}]", "No tab with id: 0.");
    row("search: half a tab", "search.query", "[{\"text\":\"x\",\"tabId\":1.5}]", "search.query: 'tabId' has a value of the wrong kind");
    row("search: a tab in words", "search.query", "[{\"text\":\"x\",\"tabId\":\"12\"}]", "search.query: 'tabId' has a value of the wrong kind");
    row("search: a key of nobody's", "search.query", "[{\"text\":\"x\",\"engine\":\"bing\"}]", "search.query: 'engine' is not a key of it");
    row("restore: Vimium's", "sessions.restore", "[null]", "");
    row("restore: nothing", "sessions.restore", "[]", "");
    row("restore: a callback the shim took off, and nothing else", "sessions.restore", "[null, null]", "");
    row("restore: a session by its id", "sessions.restore", "[\"abc\"]", "No session with id: abc.");
    row("restore: a number", "sessions.restore", "[1]", "sessions.restore takes the id of a session, or nothing");
    row("restore: an object", "sessions.restore", "[{}]", "sessions.restore takes the id of a session, or nothing");
    row("duplicate: Vimium's", "tabs.duplicate", "[12]", "");
    row("duplicate: nothing", "tabs.duplicate", "[]", "tabs.duplicate takes the id of a tab");
    row("duplicate: null", "tabs.duplicate", "[null]", "tabs.duplicate takes the id of a tab");
    row("duplicate: text", "tabs.duplicate", "[\"12\"]", "tabs.duplicate takes the id of a tab");
    row("duplicate: tab 0", "tabs.duplicate", "[0]", "No tab with id: 0.");
    row("duplicate: half a tab", "tabs.duplicate", "[12.5]", "tabs.duplicate takes the id of a tab");
    row("move: Vimium's", "tabs.move", "[12, {\"index\":0}]", "");
    row("move: to the end", "tabs.move", "[12, {\"index\":-1}]", "");
    row("move: some", "tabs.move", "[[12, 15], {\"index\":0}]", "");
    row("move: in this window", "tabs.move", "[12, {\"index\":0, \"windowId\":-2}]", "");
    row("move: none", "tabs.move", "[[], {\"index\":0}]", "No tabs given.");
    row("move: no index", "tabs.move", "[12, {}]", "tabs.move: 'index' is required");
    row("move: half a place", "tabs.move", "[12, {\"index\":1.5}]", "tabs.move: 'index' has a value of the wrong kind");
    row("move: before the beginning", "tabs.move", "[12, {\"index\":-2}]", "tabs.move: 'index' has a value of the wrong kind");
    row("move: another window", "tabs.move", "[12, {\"index\":0, \"windowId\":7}]", "No window with id: 7.");
    row("move: tab 0", "tabs.move", "[[12, 0], {\"index\":0}]", "No tab with id: 0.");
    row("move: no object", "tabs.move", "[12]", "tabs.move takes the id of a tab, or a list of them, and an object");
    row("move: a key of nobody's", "tabs.move", "[12, {\"index\":0, \"pinned\":true}]", "tabs.move: 'pinned' is not a key of it");
    row("group", "tabs.group", "[{\"tabIds\":12}]", "chrome.tabs.group is not available in this browser");
}

void tst_extensionhostwire::whatChangesTheTreeIsReadAndNotDone(){
    using namespace ExtensionHostWire;
    QFETCH(QString, api);
    QFETCH(QString, args);
    QFETCH(QString, error);
    const Act act = ParseAct(ParseCall(Header(QStringLiteral("{\"api\":\"%1\",\"args\":%2}").arg(api, args))), ID);
    QCOMPARE(act.error, error);
    QCOMPARE(act.kind != Act::None, error.isEmpty());
    QCOMPARE(IsAct(api), api != QStringLiteral("tabs.group"));
}

void tst_extensionhostwire::whatASessionOrAMoveIsAnsweredWith(){
    using namespace ExtensionHostWire;
    const QJsonObject put = SessionAnswer(1700000000, 15, Tabs(), ExtensionHostWire::Sight::All());
    QCOMPARE(put[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(static_cast<qint64>(put[QStringLiteral("value")].toObject()[QStringLiteral("lastModified")].toDouble()), qint64(1700000000));
    QCOMPARE(put[QStringLiteral("value")].toObject()[QStringLiteral("tab")].toObject()[QStringLiteral("id")].toInt(), 15);
    QCOMPARE(put[QStringLiteral("value")].toObject()[QStringLiteral("tab")].toObject()[QStringLiteral("index")].toInt(), 2);
    QVERIFY(!SessionAnswer(1, 15, Tabs(), ExtensionHostWire::Sight())[QStringLiteral("value")].toObject()[QStringLiteral("tab")].toObject().contains(QStringLiteral("url")));
    const QJsonObject unseen = SessionAnswer(5, 99, Tabs(), ExtensionHostWire::Sight::All());
    QCOMPARE(unseen[QStringLiteral("ok")].toBool(), true);
    QVERIFY(!unseen[QStringLiteral("value")].toObject().contains(QStringLiteral("tab")));
    QCOMPARE(static_cast<qint64>(unseen[QStringLiteral("value")].toObject()[QStringLiteral("lastModified")].toDouble()), qint64(5));
    const QJsonObject moved = TabsAnswer(QList<qint64>() << 15 << 11, Tabs(), ExtensionHostWire::Sight::All());
    QCOMPARE(moved[QStringLiteral("value")].toArray().size(), 2);
    QCOMPARE(moved[QStringLiteral("value")].toArray().at(0).toObject()[QStringLiteral("id")].toInt(), 15);
    QCOMPARE(moved[QStringLiteral("value")].toArray().at(1).toObject()[QStringLiteral("index")].toInt(), 0);
    QCOMPARE(TabsAnswer(QList<qint64>() << 15 << 13, Tabs(), ExtensionHostWire::Sight::All())[QStringLiteral("error")].toString(), QStringLiteral("No tab with id: 13."));
    QCOMPARE(TabsAnswer(QList<qint64>(), Tabs(), ExtensionHostWire::Sight::All())[QStringLiteral("value")].toArray().size(), 0);
}

void tst_extensionhostwire::whatMayBeSearchedWith(){
    using namespace ExtensionHostWire;
    QCOMPARE(WhyNotSearchable(QStringLiteral("https://www.bing.com/search?q=%1"), QUrl(QStringLiteral("https://www.bing.com/search?q=a%20b"))), QString());
    QCOMPARE(WhyNotSearchable(QStringLiteral("http://x.example/?q=%1&x=%2"), QUrl(QStringLiteral("http://x.example/?q=a&x=%252"))), QString());
    QCOMPARE(WhyNotSearchable(QStringLiteral("https://www.bing.com/"), QUrl(QStringLiteral("https://www.bing.com/"))), QStringLiteral("the default search engine has no place for the text"));
    QCOMPARE(WhyNotSearchable(QString(), QUrl()), QStringLiteral("the default search engine has no place for the text"));
    QCOMPARE(WhyNotSearchable(QStringLiteral("https://x.example/?q=%2"), QUrl(QStringLiteral("https://x.example/?q=a"))), QStringLiteral("the default search engine has no place for the text"));
    QCOMPARE(WhyNotSearchable(QStringLiteral("https://x.example/?q=%L1"), QUrl(QStringLiteral("https://x.example/?q=a"))), QStringLiteral("the default search engine has no place for the text"));
    const QString notWeb = QStringLiteral("the default search engine is not a web address");
    QCOMPARE(WhyNotSearchable(QStringLiteral("javascript:find('%1')"), QUrl(QStringLiteral("javascript:find('x')"))), notWeb);
    QCOMPARE(WhyNotSearchable(QStringLiteral("file:///C:/search.html?q=%1"), QUrl(QStringLiteral("file:///C:/search.html?q=x"))), notWeb);
    QCOMPARE(WhyNotSearchable(QStringLiteral("ftp://x.example/%1"), QUrl(QStringLiteral("ftp://x.example/x"))), notWeb);
    QCOMPARE(WhyNotSearchable(QStringLiteral("http:///?q=%1"), QUrl(QStringLiteral("http:///?q=x"))), notWeb);
    QCOMPARE(WhyNotSearchable(QStringLiteral("/search?q=%1"), QUrl(QStringLiteral("/search?q=x"))), notWeb);
    QCOMPARE(WhyNotSearchable(QStringLiteral("http://[::/%1"), QUrl(QStringLiteral("http://[::/x"))), notWeb);
}

void tst_extensionhostwire::whatAnActHolds(){
    using namespace ExtensionHostWire;
    auto read = [](const char *api, const char *args){
        return ParseAct(ParseCall(Header(QStringLiteral("{\"api\":\"%1\",\"args\":%2}").arg(QString::fromLatin1(api), QString::fromLatin1(args)))), ID); };

    const Act vimium = read("tabs.create", "[{\"windowId\":1,\"index\":9,\"active\":false,\"openerTabId\":12,\"url\":\"https://a.example/\"}]");
    QCOMPARE(vimium.kind, Act::Create);
    QCOMPARE(vimium.url, QUrl(QStringLiteral("https://a.example/")));
    QCOMPARE(vimium.activate, false);
    QCOMPARE(vimium.opener, qint64(12));
    const Act bare = read("tabs.create", "[{}]");
    QCOMPARE(bare.activate, true);
    QCOMPARE(bare.url, QUrl(QStringLiteral("about:blank")));
    QCOMPARE(bare.opener, qint64(0));
    QCOMPARE(read("tabs.create", "[{\"url\":\"/index.html\"}]").url, QUrl(QStringLiteral("chrome-extension://") + ID + QStringLiteral("/index.html")));

    const Act sought = read("search.query", "[{\"disposition\":\"NEW_TAB\",\"text\":\"a b\"}]");
    QCOMPARE(sought.kind, Act::Search);
    QCOMPARE(sought.text, QStringLiteral("a b"));
    QVERIFY(sought.newTab && !sought.current && sought.tab == 0 && !sought.hasUrl && sought.url.isEmpty());
    const Act here = read("search.query", "[{\"text\":\"x\"}]");
    QVERIFY(here.current && !here.newTab && here.tab == 0);
    const Act there = read("search.query", "[{\"text\":\"x\",\"tabId\":12}]");
    QVERIFY(!there.current && !there.newTab && there.tab == 12);
    QVERIFY(read("search.query", "[{\"text\":\"x\",\"disposition\":\"NEW_WINDOW\"}]").newTab);

    QCOMPARE(read("sessions.restore", "[null]").kind, Act::Restore);
    const Act cloned = read("tabs.duplicate", "[12]");
    QVERIFY(cloned.kind == Act::Duplicate && cloned.tab == 12);
    const Act one = read("tabs.move", "[12, {\"index\":3}]");
    QVERIFY(one.kind == Act::Move && !one.many && one.tabs == (QList<qint64>() << 12) && one.index == 3);
    const Act some = read("tabs.move", "[[15, 12, 15], {\"index\":-1}]");
    QVERIFY(some.many && some.tabs == (QList<qint64>() << 15 << 12) && some.index == -1);

    const Act shown = read("tabs.update", "[12, {\"active\":true}]");
    QCOMPARE(shown.kind, Act::Update);
    QVERIFY(!shown.current && shown.tab == 12 && shown.activate && !shown.hasUrl && !shown.hasMuted);
    QVERIFY(read("tabs.update", "[12, {\"highlighted\":true}]").activate);
    QVERIFY(!read("tabs.update", "[12, {\"active\":false}]").activate);
    const Act elsewhere = read("tabs.update", "[{\"url\":\"https://b.example/\",\"muted\":true}]");
    QVERIFY(elsewhere.current && elsewhere.hasUrl && elsewhere.hasMuted && elsewhere.muted);
    QCOMPARE(elsewhere.url, QUrl(QStringLiteral("https://b.example/")));
    QVERIFY(read("tabs.update", "[12, {\"muted\":false}]").hasMuted);
    QVERIFY(!read("tabs.update", "[12, {\"muted\":false}]").muted);

    QCOMPARE(read("tabs.remove", "[[15, 11, 15]]").tabs, QList<qint64>() << 15 << 11);
    QCOMPARE(read("tabs.remove", "[12]").tabs, QList<qint64>() << 12);
    QCOMPARE(read("tabs.remove", "[[]]").kind, Act::Remove);

    const Act hard = read("tabs.reload", "[12, {\"bypassCache\":true}]");
    QVERIFY(hard.kind == Act::Reload && !hard.current && hard.tab == 12 && hard.bypassCache);
    QVERIFY(read("tabs.reload", "[]").current);
    QVERIFY(read("tabs.reload", "[null, null]").current);
    QVERIFY(!read("tabs.reload", "[12]").bypassCache);
}

void tst_extensionhostwire::theOneWindow(){
    using namespace ExtensionHostWire;
    Window window;
    window.focused = true; window.state = QStringLiteral("maximized");
    window.left = 10; window.top = 20; window.width = 800; window.height = 600;
    auto ask = [&](const char *api, const char *args, const Window *w){
        return Answer(ParseCall(Header(QStringLiteral("{\"api\":\"%1\",\"args\":%2}").arg(QString::fromLatin1(api), QString::fromLatin1(args)))), Tabs(), ExtensionHostWire::Sight::All(), w); };

    const QJsonObject current = ask("windows.getCurrent", "[]", &window)[QStringLiteral("value")].toObject();
    QCOMPARE(current[QStringLiteral("id")].toInt(), 1);
    QCOMPARE(current[QStringLiteral("focused")].toBool(), true);
    QCOMPARE(current[QStringLiteral("incognito")].toBool(true), false);
    QCOMPARE(current[QStringLiteral("type")].toString(), QStringLiteral("normal"));
    QCOMPARE(current[QStringLiteral("state")].toString(), QStringLiteral("maximized"));
    QCOMPARE(current[QStringLiteral("width")].toInt(), 800);
    QVERIFY(!current.contains(QStringLiteral("tabs")));

    const QJsonArray all = ask("windows.getAll", "[null]", &window)[QStringLiteral("value")].toArray();
    QCOMPARE(all.size(), 1);
    QCOMPARE(all.at(0).toObject()[QStringLiteral("id")].toInt(), 1);
    const QJsonArray populated = ask("windows.getAll", "[{\"populate\":true}]", &window)[QStringLiteral("value")].toArray();
    QCOMPARE(populated.at(0).toObject()[QStringLiteral("tabs")].toArray().size(), 4);
    QCOMPARE(populated.at(0).toObject()[QStringLiteral("tabs")].toArray().at(2).toObject()[QStringLiteral("index")].toInt(), 2);

    QCOMPARE(ask("windows.getAll", "[{\"windowTypes\":[\"popup\"]}]", &window)[QStringLiteral("value")].toArray().size(), 0);
    QCOMPARE(ask("windows.getAll", "[{\"windowTypes\":[\"popup\",\"normal\"]}]", &window)[QStringLiteral("value")].toArray().size(), 1);
    QCOMPARE(ask("windows.getCurrent", "[{\"windowTypes\":[\"popup\"]}]", &window)[QStringLiteral("error")].toString(), QStringLiteral("No current window"));

    QCOMPARE(ask("windows.getAll", "[]", nullptr)[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(ask("windows.getAll", "[]", nullptr)[QStringLiteral("value")].toArray().size(), 0);
    QCOMPARE(ask("windows.getCurrent", "[]", nullptr)[QStringLiteral("error")].toString(), QStringLiteral("No current window"));

    foreach(const char *args, QList<const char*>() << "[1]" << "[{}, {}]" << "[{\"populate\":1}]" << "[{\"windowTypes\":\"normal\"}]" << "[{\"colour\":1}]")
        QCOMPARE(ask("windows.getAll", args, &window)[QStringLiteral("ok")].toBool(true), false);
}

void tst_extensionhostwire::whatAnActIsAnsweredWith(){
    using namespace ExtensionHostWire;
    QCOMPARE(Done()[QStringLiteral("ok")].toBool(), true);
    QVERIFY(!Done().contains(QStringLiteral("value")));
    const QJsonObject tab = TabAnswer(15, Tabs(), ExtensionHostWire::Sight::All());
    QCOMPARE(tab[QStringLiteral("value")].toObject()[QStringLiteral("index")].toInt(), 2);
    QVERIFY(!TabAnswer(15, Tabs(), ExtensionHostWire::Sight())[QStringLiteral("value")].toObject().contains(QStringLiteral("url")));
    QCOMPARE(TabAnswer(13, Tabs(), ExtensionHostWire::Sight::All())[QStringLiteral("error")].toString(), QStringLiteral("No tab with id: 13."));
    QCOMPARE(Refused(NotEditable())[QStringLiteral("error")].toString(),
             QStringLiteral("Tabs cannot be edited right now (user may be dragging a tab)."));
    QCOMPARE(Refused(NoCurrentWindow())[QStringLiteral("ok")].toBool(true), false);
}

void tst_extensionhostwire::oneAtATimeInTheOrderTheyCame(){
    using namespace ExtensionHostWire;
    OneAtATime<int> acts(8);
    QList<int> ran;
    auto always = [](){ return true; };
    QCOMPARE(acts.Drain([&](int i){ ran << i; }, always), OneAtATime<int>::Empty);
    acts.Push(1); acts.Push(2); acts.Push(3);
    QCOMPARE(acts.Drain([&](int i){ ran << i; if(i == 2) acts.Push(4); }, always), OneAtATime<int>::Drained);
    QCOMPARE(ran, QList<int>() << 1 << 2 << 3 << 4);
    QCOMPARE(acts.Count(), 0);
    QCOMPARE(acts.Drain([&](int i){ ran << i; }, always), OneAtATime<int>::Empty);
}

void tst_extensionhostwire::aDrainInsideADrainDoesNothing(){
    using namespace ExtensionHostWire;
    OneAtATime<int> acts(8);
    QList<int> ran;
    QList<int> inner;
    auto always = [](){ return true; };
    acts.Push(1); acts.Push(2);
    std::function<void(int)> run = [&](int i){
        ran << i;
        inner << acts.Drain(run, always);
        if(i == 1) QCOMPARE(ran, QList<int>() << 1);
    };
    QCOMPARE(acts.Drain(run, always), OneAtATime<int>::Drained);
    QCOMPARE(ran, QList<int>() << 1 << 2);
    QCOMPARE(inner, QList<int>() << OneAtATime<int>::Nested << OneAtATime<int>::Nested);
    acts.Push(3);
    QCOMPARE(acts.Drain(run, always), OneAtATime<int>::Drained);
    QCOMPARE(ran, QList<int>() << 1 << 2 << 3);
}

void tst_extensionhostwire::whatMayNotRunNowWaits(){
    using namespace ExtensionHostWire;
    OneAtATime<int> acts(8);
    QList<int> ran;
    bool may = false;
    acts.Push(1); acts.Push(2); acts.Push(3);
    QCOMPARE(acts.Drain([&](int i){ ran << i; }, [&](){ return may; }), OneAtATime<int>::Declined);
    QVERIFY(ran.isEmpty());
    QCOMPARE(acts.Count(), 3);
    QVERIFY(acts.First() && *acts.First() == 1);
    QCOMPARE(acts.Count(), 3);
    QVERIFY(!OneAtATime<int>(2).First());

    may = true;
    QCOMPARE(acts.Drain([&](int i){ ran << i; if(i == 1) may = false; }, [&](){ return may; }), OneAtATime<int>::Declined);
    QCOMPARE(ran, QList<int>() << 1);
    QCOMPARE(acts.Count(), 2);

    may = true;
    QCOMPARE(acts.Drain([&](int i){ ran << i; }, [&](){ return may; }), OneAtATime<int>::Drained);
    QCOMPARE(ran, QList<int>() << 1 << 2 << 3);
}

void tst_extensionhostwire::thereIsRoomForSoManyAndNoMore(){
    using namespace ExtensionHostWire;
    OneAtATime<int> acts(2);
    QVERIFY(acts.Push(1));
    QVERIFY(acts.Push(2));
    QVERIFY(!acts.Push(3));
    QCOMPARE(acts.Count(), 2);
    QList<int> ran;
    acts.Drain([&](int i){ ran << i; }, [](){ return true; });
    QCOMPARE(ran, QList<int>() << 1 << 2);
    QVERIFY(acts.Push(3));
}

void tst_extensionhostwire::oneTimerServesTheLine(){
    using namespace ExtensionHostWire;
    OneAtATime<int> acts(8);
    int armed = 0;
    for(int i = 1; i <= 3; i++){ acts.Push(i); if(acts.Reserve()) armed++; }
    QCOMPARE(armed, 1);
    QVERIFY(acts.Reserved());
    QCOMPARE(acts.Reservations(), 1);

    acts.Fired();
    QVERIFY(!acts.Reserved());
    QCOMPARE(acts.Drain([](int){}, [](){ return false; }), OneAtATime<int>::Declined);
    QVERIFY(acts.Reserve());
    QVERIFY(!acts.Reserve());
    acts.Push(4);
    QVERIFY(!acts.Reserve());
    QCOMPARE(acts.Reservations(), 2);

    acts.Prune([](int i){ return i % 2 == 0; });
    QCOMPARE(acts.Count(), 2);
    QList<int> ran;
    acts.Fired();
    acts.Drain([&](int i){ ran << i; }, [](){ return true; });
    QCOMPARE(ran, QList<int>() << 2 << 4);

    acts.Push(5);
    acts.Prune([](int){ return false; });
    QCOMPARE(acts.Count(), 0);
    acts.Push(6); acts.Push(7);
    acts.Drain([&](int i){ ran << i; acts.Prune([](int){ return false; }); }, [](){ return true; });
    QCOMPARE(ran, QList<int>() << 2 << 4 << 6 << 7);
}

namespace {
    using ExtensionHostWire::Tab;
    Tab T(qint64 id, const char *url = "https://a.example/", const char *title = "a", bool active = false){
        Tab t; t.id = id; t.url = QUrl(QString::fromLatin1(url)); t.title = QString::fromLatin1(title); t.active = active; return t;
    }
    QString Said(const QJsonArray &events){
        QStringList out;
        foreach(const QJsonValue &v, events){
            const QJsonObject event = v.toObject();
            QStringList args;
            foreach(const QJsonValue &a, event.value(QStringLiteral("args")).toArray()){
                if(a.isObject()){
                    const QJsonObject o = a.toObject();
                    if(o.contains(QStringLiteral("tabId"))) args << QStringLiteral("tab %1").arg(o.value(QStringLiteral("tabId")).toInt());
                    else if(o.contains(QStringLiteral("id"))) args << QStringLiteral("#%1@%2").arg(o.value(QStringLiteral("id")).toInt()).arg(o.value(QStringLiteral("index")).toInt());
                    else args << QStringList(o.keys()).join(QLatin1Char('+'));
                }
                else args << QString::number(a.toInt());
            }
            out << event.value(QStringLiteral("name")).toString() + QLatin1Char('(') + args.join(QLatin1Char(',')) + QLatin1Char(')');
        }
        return out.join(QLatin1Char(' '));
    }
    QSet<int> &Gone(){ static QSet<int> gone; return gone; }
    bool There(int waiter){ return waiter != 0 && !Gone().contains(waiter); }
    typedef ExtensionHostWire::Subscribers<int> Table;
    const QString EXT = QStringLiteral("abcdefghijklmnopabcdefghijklmnop");
    QByteArray Token(int n){ return QByteArray(31, '0') + QByteArray::number(n % 10); }
}

void tst_extensionhostwire::theOneWindowIsNotMovedAndSaysSo(){
    using namespace ExtensionHostWire;
    Window window; window.focused = false; window.width = 800; window.height = 600;
    auto ask = [&](const char *json, const Window *w){
        const Call call = ParseCall(QUrl::toPercentEncoding(QString::fromLatin1(json)));
        return Answer(call, QList<Tab>() << T(1), Sight::All(), w);
    };
    const QJsonObject ok = ask("{\"api\":\"windows.update\",\"args\":[1,{\"focused\":true}]}", &window);
    QCOMPARE(ok.value(QStringLiteral("ok")).toBool(), true);
    const QJsonObject told = ok.value(QStringLiteral("value")).toObject();
    QCOMPARE(told.value(QStringLiteral("id")).toInt(), 1);
    QCOMPARE(told.value(QStringLiteral("focused")).toBool(true), false);
    QVERIFY(!told.contains(QStringLiteral("tabs")));
    QCOMPARE(ask("{\"api\":\"windows.update\",\"args\":[-2,{\"focused\":true}]}", &window).value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(ask("{\"api\":\"windows.update\",\"args\":[1,{}]}", &window).value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(ask("{\"api\":\"windows.update\",\"args\":[1,{\"focused\":null}]}", &window).value(QStringLiteral("ok")).toBool(), true);

    auto refused = [&](const char *json, const Window *w, const char *words){
        const QJsonObject answer = ask(json, w);
        return answer.value(QStringLiteral("ok")).toBool(true) == false
            && answer.value(QStringLiteral("error")).toString().contains(QString::fromLatin1(words));
    };
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[7,{\"focused\":true}]}", &window, "No window with id: 7."));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1,{\"focused\":false}]}", &window, "moved back"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1,{\"focused\":\"yes\"}]}", &window, "wrong kind"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1,{\"state\":\"maximized\"}]}", &window, "'state' is not supported"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1,{\"focused\":true,\"left\":0}]}", &window, "'left' is not supported"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1]}", &window, "takes a window's id"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[\"1\",{}]}", &window, "takes a window's id"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1.5,{}]}", &window, "takes a window's id"));
    QVERIFY(refused("{\"api\":\"windows.update\",\"args\":[1,{\"focused\":true}]}", nullptr, "No current window"));
}

void tst_extensionhostwire::onlyATokenNamesWhoListens(){
    using namespace ExtensionHostWire;
    auto call = [](const char *json){
        return TokenOfEvents(ParseCall(QUrl::toPercentEncoding(QString::fromLatin1(json))));
    };
    QCOMPARE(call("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\"]}"), QByteArray("0123456789abcdef0123456789abcdef"));
    QVERIFY(call("{\"api\":\"vanilla.events\",\"args\":[]}").isEmpty());
    QVERIFY(call("{\"api\":\"vanilla.events\",\"args\":[\"short\"]}").isEmpty());
    QVERIFY(call("{\"api\":\"vanilla.events\",\"args\":[\"0123456789ABCDEF0123456789ABCDEF\"]}").isEmpty());
    QVERIFY(call("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\",1]}").isEmpty());
    QVERIFY(call("{\"api\":\"vanilla.events\",\"args\":[5]}").isEmpty());
    QVERIFY(call("{\"api\":\"tabs.query\",\"args\":[\"0123456789abcdef0123456789abcdef\"]}").isEmpty());
    auto names = [](const char *json){
        const QStringList list = NamesOfEvents(ParseCall(QUrl::toPercentEncoding(QString::fromLatin1(json)))).values();
        QStringList sorted = list; sorted.sort(); return sorted.join(QLatin1Char(','));
    };
    QCOMPARE(call("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\",[]]}"), QByteArray("0123456789abcdef0123456789abcdef"));
    QCOMPARE(names("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\",[\"tabs.onUpdated\",\"no.such\",5,\"tabs.onUpdated\",\"downloads.onChanged\"]]}"),
             QStringLiteral("downloads.onChanged,tabs.onUpdated"));
    QCOMPARE(names("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\"]}"), QString());
    QCOMPARE(names("{\"api\":\"tabs.query\",\"args\":[\"0123456789abcdef0123456789abcdef\",[\"tabs.onUpdated\"]]}"), QString());
    auto told = [](const char *json){
        return TokenOfEventNames(ParseCall(QUrl::toPercentEncoding(QString::fromLatin1(json))));
    };
    QCOMPARE(told("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[\"tabs.onUpdated\"]]}"), QByteArray("0123456789abcdef0123456789abcdef"));
    QCOMPARE(names("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[\"tabs.onUpdated\"]]}"), QStringLiteral("tabs.onUpdated"));
    QVERIFY(told("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\"]}").isEmpty());
    QVERIFY(told("{\"api\":\"vanilla.eventNames\",\"args\":[\"short\",[]]}").isEmpty());
    QVERIFY(told("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\",[]]}").isEmpty());
    auto number = [](const char *json){ return NumberOfEventNames(ParseCall(QUrl::toPercentEncoding(QString::fromLatin1(json)))); };
    QCOMPARE(told("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],7]}"), QByteArray("0123456789abcdef0123456789abcdef"));
    QCOMPARE(call("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],7]}"), QByteArray("0123456789abcdef0123456789abcdef"));
    QVERIFY(told("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],\"7\"]}").isEmpty());
    QCOMPARE(number("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],7]}"), qint64(7));
    QCOMPARE(number("{\"api\":\"vanilla.events\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],7]}"), qint64(7));
    QCOMPARE(number("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[]]}"), qint64(0));
    QCOMPARE(number("{\"api\":\"vanilla.eventNames\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],-3]}"), qint64(0));
    QCOMPARE(number("{\"api\":\"tabs.query\",\"args\":[\"0123456789abcdef0123456789abcdef\",[],7]}"), qint64(0));
}

void tst_extensionhostwire::whatIsDifferentIsToldInOrder(){
    using namespace ExtensionHostWire;
    const QList<Tab> before = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2) << T(3);
    const QList<Tab> after = QList<Tab>() << T(2, "https://b.example/", "b") << T(4, "https://c.example/", "c", true);
    QCOMPARE(Said(Diff(before, after, true, true, ExtensionHostWire::Sight::All())),
             QStringLiteral("tabs.onRemoved(1,isWindowClosing+windowId) tabs.onRemoved(3,isWindowClosing+windowId) "
                            "tabs.onCreated(#4@1) tabs.onUpdated(2,title+url,#2@0) tabs.onActivated(tab 4) tabs.onHighlighted(tabIds+windowId)"));
    QVERIFY(Diff(before, before, true, true, ExtensionHostWire::Sight::All()).isEmpty());
    QVERIFY(Diff(QList<Tab>(), QList<Tab>(), false, false, ExtensionHostWire::Sight::All()).isEmpty());

    const QJsonArray events = Diff(before, after, true, true, ExtensionHostWire::Sight::All());
    const QJsonArray removed = events.at(0).toObject().value(QStringLiteral("args")).toArray();
    QCOMPARE(removed.at(1).toObject().value(QStringLiteral("windowId")).toInt(), 1);
    QCOMPARE(removed.at(1).toObject().value(QStringLiteral("isWindowClosing")).toBool(true), false);
    const QJsonObject activated = events.at(4).toObject().value(QStringLiteral("args")).toArray().at(0).toObject();
    QCOMPARE(activated.value(QStringLiteral("windowId")).toInt(), 1);

    Tab unloaded = T(2); unloaded.discarded = true;
    const QJsonArray went = Diff(QList<Tab>() << T(2), QList<Tab>() << unloaded, true, true, ExtensionHostWire::Sight::All());
    QCOMPARE(Said(went), QStringLiteral("tabs.onUpdated(2,discarded+status,#2@0)"));
    const QJsonArray args = went.at(0).toObject().value(QStringLiteral("args")).toArray();
    QCOMPARE(args.at(1).toObject().value(QStringLiteral("status")).toString(), QStringLiteral("unloaded"));
    QCOMPARE(args.at(2).toObject().value(QStringLiteral("status")).toString(), QStringLiteral("unloaded"));
    Tab loud = T(2); loud.audible = true; loud.muted = true;
    QCOMPARE(Said(Diff(QList<Tab>() << T(2), QList<Tab>() << loud, true, true, ExtensionHostWire::Sight::All())),
             QStringLiteral("tabs.onUpdated(2,audible+mutedInfo,#2@0)"));
}

void tst_extensionhostwire::whereATabSitsIsNoChangeOfTheTab(){
    using namespace ExtensionHostWire;
    const QList<Tab> before = QList<Tab>() << T(1) << T(2) << T(3);
    const QList<Tab> after = QList<Tab>() << T(3) << T(2) << T(1);
    QCOMPARE(Said(Diff(before, after, true, true, ExtensionHostWire::Sight::All())),
             QStringLiteral("tabs.onMoved(2,fromIndex+toIndex+windowId) tabs.onMoved(1,fromIndex+toIndex+windowId)"));
    const QJsonObject value = EventsAnswer(QJsonArray(), after).value(QStringLiteral("value")).toObject();
    QCOMPARE(EventsAnswer(QJsonArray(), after).value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(QString::fromUtf8(QJsonDocument(value.value(QStringLiteral("order")).toArray()).toJson(QJsonDocument::Compact)), QStringLiteral("[3,2,1]"));
    QVERIFY(!value.contains(QStringLiteral("stale")));
    const QJsonObject stale = StaleAnswer().value(QStringLiteral("value")).toObject();
    QCOMPARE(stale.value(QStringLiteral("stale")).toBool(), true);
    QVERIFY(stale.value(QStringLiteral("events")).toArray().isEmpty());
    QVERIFY(!stale.contains(QStringLiteral("order")));
}

void tst_extensionhostwire::withoutThePermissionAnAddressIsNeitherToldNorCompared(){
    using namespace ExtensionHostWire;
    const QList<Tab> before = QList<Tab>() << T(1, "https://a.example/", "a");
    const QList<Tab> after = QList<Tab>() << T(1, "https://secret.example/path", "secret") << T(2, "https://other.example/", "other");
    const QJsonArray events = Diff(before, after, true, true, ExtensionHostWire::Sight());
    QCOMPARE(Said(events), QStringLiteral("tabs.onCreated(#2@1)"));
    const QByteArray text = QJsonDocument(events).toJson(QJsonDocument::Compact);
    QVERIFY2(!text.contains("example"), text.constData());
    QVERIFY2(!text.contains("secret") && !text.contains("other"), text.constData());
    QVERIFY(QJsonDocument(Diff(before, after, true, true, ExtensionHostWire::Sight::All())).toJson().contains("secret.example"));
}

void tst_extensionhostwire::aSightSeesWhatItsHostPermissionsCover(){
    using namespace ExtensionHostWire;
    auto url = [](const char *text){ return QUrl(QString::fromLatin1(text)); };

    const Sight all = Sight::Of(false, QStringLiteral("self"), QStringList() << QStringLiteral("<all_urls>"));
    QVERIFY(all.Any());
    QVERIFY(all.Sees(url("https://a.example/")));
    QVERIFY(all.Sees(url("http://127.0.0.1:8765/index.html")));
    QVERIFY(all.Sees(url("chrome-extension://self/popup.html")));
    QVERIFY(!all.Sees(url("chrome-extension://another/popup.html")));
    QVERIFY(!all.Sees(url("file:///C:/notes.html")));
    QVERIFY(!all.Sees(url("about:blank")));
    QVERIFY(!all.Sees(url("data:text/html,a")));
    QVERIFY(!all.Sees(url("edge://settings/")));
    QVERIFY(!all.Sees(QUrl()));

    const Sight one = Sight::Of(false, QString(), QStringList() << QStringLiteral("https://a.example/app/*"));
    QVERIFY(one.Sees(url("https://a.example/elsewhere?q=1")));
    QVERIFY(!one.Sees(url("http://a.example/")));
    QVERIFY(!one.Sees(url("https://b.example/")));
    QVERIFY(!one.Sees(url("https://sub.a.example/")));
    const Sight subs = Sight::Of(false, QString(), QStringList() << QStringLiteral("*://*.example.com/*"));
    QVERIFY(subs.Sees(url("http://example.com/")));
    QVERIFY(subs.Sees(url("https://x.example.com/")));
    QVERIFY(!subs.Sees(url("ftp://x.example.com/")));

    const Sight file = Sight::Of(false, QString(), QStringList() << QStringLiteral("file:///*"));
    QVERIFY(!file.Any());
    QVERIFY(!file.Sees(url("file:///C:/notes.html")));
    const Sight mixed = Sight::Of(false, QString(), QStringList()
                                  << QStringLiteral("http://localhost:8080/*") << QStringLiteral("nonsense")
                                  << QStringLiteral("https://b.example/*"));
    QCOMPARE(mixed.hosts, QStringList() << QStringLiteral("https://b.example/*"));
    QVERIFY(!mixed.Sees(url("http://localhost:8080/")));
    QVERIFY(mixed.Sees(url("https://b.example/")));
    QVERIFY(!Sight::Of(false, QString(), QStringList() << QStringLiteral("http://localhost:8080/*")).Any());

    const Sight none = Sight::Of(false, QStringLiteral("self"), QStringList());
    QVERIFY(!none.Any());
    QVERIFY(!none.Sees(url("https://a.example/")));
    QVERIFY(none.Sees(url("chrome-extension://self/options.html")));
    QVERIFY(Sight::Of(true, QString(), QStringList()).Sees(url("file:///C:/notes.html")));
    QVERIFY(Sight::All().Sees(url("about:blank")));
}

void tst_extensionhostwire::hostPermissionsTellTheTabsTheyCover(){
    using namespace ExtensionHostWire;
    const Sight sight = Sight::Of(false, QString(), QStringList() << QStringLiteral("*://*.b.example/*")
                                  << QStringLiteral("http://c.example/*"));
    const QJsonArray all = Ask(QStringLiteral("tabs.query"), QStringLiteral("[{}]"), sight)[QStringLiteral("value")].toArray();
    QCOMPARE(all.size(), 4);
    QStringList told;
    foreach(const QJsonValue &tab, all)
        told << tab.toObject()[QStringLiteral("url")].toString() + QLatin1Char('|') + tab.toObject()[QStringLiteral("title")].toString();
    QCOMPARE(told, QStringList() << QStringLiteral("|") << QStringLiteral("https://sub.b.example/two|two")
                                 << QStringLiteral("http://c.example/|three") << QStringLiteral("|"));
    QVERIFY(!Ask(QStringLiteral("tabs.get"), QStringLiteral("[11]"), sight)[QStringLiteral("value")].toObject().contains(QStringLiteral("url")));
    QCOMPARE(Ask(QStringLiteral("tabs.get"), QStringLiteral("[15]"), sight)[QStringLiteral("value")].toObject()[QStringLiteral("url")].toString(),
             QStringLiteral("http://c.example/"));
    QVERIFY(!TabAnswer(20, Tabs(), sight)[QStringLiteral("value")].toObject().contains(QStringLiteral("url")));
    QCOMPARE(TabAnswer(12, Tabs(), sight)[QStringLiteral("value")].toObject()[QStringLiteral("title")].toString(), QStringLiteral("two"));

    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"url\":\"<all_urls>\"}]"), sight)), QStringLiteral("12,15"));
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"url\":\"https://a.example/*\"}]"), sight)), QString());
    const Sight broad = Sight::Of(false, QString(), QStringList() << QStringLiteral("<all_urls>"));
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"url\":\"file:///*\"}]"), broad)), QString());
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"url\":\"file:///*\"}]"))), QStringLiteral("20"));
    QCOMPARE(Ids(Ask(QStringLiteral("tabs.query"), QStringLiteral("[{\"url\":\"<all_urls>\"}]"), Sight::Of(false, QString(), QStringList()))),
             QStringLiteral("ERROR"));
}

void tst_extensionhostwire::anAddressComingIntoSightIsToldWholeAndGoingOutOfItNothing(){
    using namespace ExtensionHostWire;
    const Sight sight = Sight::Of(false, QString(), QStringList() << QStringLiteral("https://a.example/*"));
    const QList<Tab> seen = QList<Tab>() << T(1, "https://a.example/", "same");
    const QList<Tab> unseen = QList<Tab>() << T(1, "https://secret.example/", "same");
    const QList<Tab> unseenToo = QList<Tab>() << T(1, "https://secret.example/other", "other");
    QVERIFY(Diff(seen, unseen, true, true, sight).isEmpty());
    QVERIFY(Diff(unseen, unseenToo, true, true, sight).isEmpty());
    const QJsonArray back = Diff(unseen, seen, true, true, sight);
    QCOMPARE(back.size(), 1);
    const QJsonObject change = back.at(0).toObject()[QStringLiteral("args")].toArray().at(1).toObject();
    QCOMPARE(change[QStringLiteral("url")].toString(), QStringLiteral("https://a.example/"));
    QCOMPARE(change[QStringLiteral("title")].toString(), QStringLiteral("same"));
    const QList<Tab> retitled = QList<Tab>() << T(1, "https://a.example/", "new");
    const QJsonObject only = Diff(seen, retitled, true, true, sight).at(0).toObject()[QStringLiteral("args")].toArray().at(1).toObject();
    QCOMPARE(only.keys(), QStringList() << QStringLiteral("title"));
}

void tst_extensionhostwire::toATabTheAskerMayNotSeeNothingIsSaid(){
    using namespace ExtensionHostWire;
    const QList<Tab> before = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2);
    const QList<Tab> after = QList<Tab>() << T(1) << T(2);
    QVERIFY(Diff(before, after, true, true, ExtensionHostWire::Sight::All()).isEmpty());
    QCOMPARE(Said(Diff(after, before, true, true, ExtensionHostWire::Sight::All())), QStringLiteral("tabs.onActivated(tab 1) tabs.onHighlighted(tabIds+windowId)"));
    QVERIFY(Diff(after, after, true, true, ExtensionHostWire::Sight::All()).isEmpty());
}

void tst_extensionhostwire::theFocusOfTheWindowIsToldByItsNumbers(){
    using namespace ExtensionHostWire;
    const QList<Tab> tabs = QList<Tab>() << T(1, "https://a.example/", "a", true);
    QCOMPARE(Said(Diff(tabs, tabs, true, false, ExtensionHostWire::Sight::All())), QStringLiteral("windows.onFocusChanged(-1)"));
    QCOMPARE(Said(Diff(tabs, tabs, false, true, ExtensionHostWire::Sight::All())), QStringLiteral("windows.onFocusChanged(1)"));
    QCOMPARE(Said(Diff(QList<Tab>() << T(1) << T(2, "https://a.example/", "a", true), QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2), false, true, ExtensionHostWire::Sight::All())),
             QStringLiteral("tabs.onActivated(tab 1) tabs.onHighlighted(tabIds+windowId) windows.onFocusChanged(1)"));
}

void tst_extensionhostwire::whoWasToldNothingIsToldWhereItWoke(){
    using namespace ExtensionHostWire;
    QCOMPARE(Said(Greeting(QList<Tab>() << T(1) << T(2, "https://a.example/", "a", true))), QStringLiteral("tabs.onActivated(tab 2)"));
    QVERIFY(Greeting(QList<Tab>() << T(1) << T(2)).isEmpty());
    QVERIFY(Greeting(QList<Tab>()).isEmpty());

    Gone().clear();
    Table table;
    QStringList answered;
    auto answer = [&](int waiter, const QJsonObject &reply){
        answered << QString::number(waiter) + QStringLiteral(": ") + Said(reply.value(QStringLiteral("value")).toObject().value(QStringLiteral("events")).toArray());
    };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    const QList<Tab> tabs = QList<Tab>() << T(1) << T(2, "https://a.example/", "a", true) << T(3);
    QCOMPARE(table.Take(EXT, Token(1), 11, 0, There, nullptr), Table::Held);
    table.Publish(tabs, true, 10, There, sees, answer);
    QCOMPARE(answered, QStringList() << QStringLiteral("11: tabs.onActivated(tab 2)"));
    QCOMPARE(table.Take(EXT, Token(2), 12, 20, There, nullptr), Table::Held);
    table.Publish(QList<Tab>() << T(1) << T(3), true, 30, There, sees, answer);
    QCOMPARE(answered.size(), 1);
    table.Publish(QList<Tab>() << T(1) << T(3) << T(4), true, 40, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("12: tabs.onCreated(#4@2)"));
}

void tst_extensionhostwire::aClickIsToldOnceToWhoeverOfTheExtensionWaits(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QStringList answered;
    auto answer = [&](int waiter, const QJsonObject &reply){
        answered << QString::number(waiter) + QStringLiteral(": ") + Said(reply.value(QStringLiteral("value")).toObject().value(QStringLiteral("events")).toArray());
    };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    auto click = [](const char *name, int n){
        QJsonObject event;
        event[QStringLiteral("name")] = QString::fromLatin1(name);
        event[QStringLiteral("args")] = QJsonArray() << n;
        return event;
    };
    const QString OTHER = QStringLiteral("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    const QList<Tab> tabs = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2);

    QVERIFY(!table.Fire(EXT, click("action.onClicked", 1)));
    QCOMPARE(table.Take(EXT, Token(1), 11, 0, There, nullptr), Table::Held);
    QCOMPARE(table.Take(OTHER, Token(2), 21, 0, There, nullptr), Table::Held);
    table.Publish(tabs, true, 1, There, sees, answer);
    QCOMPARE(answered, QStringList() << QStringLiteral("11: tabs.onActivated(tab 1)") << QStringLiteral("21: tabs.onActivated(tab 1)"));

    table.Take(EXT, Token(1), 12, 2, There, nullptr);
    table.Take(OTHER, Token(2), 22, 2, There, nullptr);
    QVERIFY(table.Fire(EXT, click("contextMenus.onClicked", 2)));
    const QList<Tab> more = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2) << T(3);
    table.Publish(more, true, 3, There, sees, answer);
    QCOMPARE(answered.size(), 4);
    QCOMPARE(answered.at(2), QStringLiteral("12: tabs.onCreated(#3@2) contextMenus.onClicked(2)"));
    QCOMPARE(answered.at(3), QStringLiteral("22: tabs.onCreated(#3@2)"));
    table.Take(EXT, Token(1), 13, 4, There, nullptr);
    table.Publish(more, true, 5, There, sees, answer);
    QCOMPARE(answered.size(), 4);

    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 6, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("13: tabs.onRemoved(2,isWindowClosing+windowId)"));
    QVERIFY(table.Fire(EXT, click("action.onClicked", 3)));
    QVERIFY(table.Fire(EXT, click("action.onClicked", 4)));
    table.Take(EXT, Token(1), 14, 7, There, nullptr);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 8, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("14: action.onClicked(3) action.onClicked(4)"));

    for(int i = 100; i < 100 + SUBSCRIBER_QUEUE + 5; i++) table.Fire(EXT, click("action.onClicked", i));
    table.Take(EXT, Token(1), 15, 9, There, nullptr);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 10, There, sees, answer);
    const QStringList told = answered.last().mid(4).split(QLatin1Char(' '));
    QCOMPARE(told.size(), SUBSCRIBER_QUEUE);
    QCOMPARE(told.first(), QStringLiteral("action.onClicked(105)"));
    QCOMPARE(told.last(), QStringLiteral("action.onClicked(%1)").arg(100 + SUBSCRIBER_QUEUE + 4));

    const QString THIRD = QStringLiteral("cccccccccccccccccccccccccccccccc");
    QCOMPARE(table.Take(THIRD, Token(3), 31, 10, There, nullptr), Table::Held);
    QVERIFY(table.Fire(THIRD, click("action.onClicked", 9)));
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 10, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("31: tabs.onActivated(tab 1) action.onClicked(9)"));
    QCOMPARE(table.Take(THIRD, Token(3), 32, 10, There, nullptr), Table::Held);
    QVERIFY(table.Fire(THIRD, click("action.onClicked", 10)));
    int old = 0;
    QCOMPARE(table.Take(THIRD, Token(3), 33, 10, There, &old), Table::Replaced);
    QCOMPARE(old, 32);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 10, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("33: action.onClicked(10)"));

    table.Take(EXT, Token(1), 16, 11, There, nullptr);
    QVERIFY(table.Fire(EXT, click("action.onClicked", 5)));
    Gone().insert(16);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 12, There, sees, answer);
    QVERIFY(!answered.last().startsWith(QStringLiteral("16:")));
    QVERIFY(!table.Fire(EXT, click("action.onClicked", 6)));
    table.Take(EXT, Token(1), 17, 13, There, nullptr);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 14, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("17: tabs.onActivated(tab 1)"));
}

void tst_extensionhostwire::anOffscreenDocumentIsOfTheExtensionsOwnAddressOnly(){
    using namespace ExtensionHostWire;
    const QString ID = QStringLiteral("abcdefghijklmnopabcdefghijklmnop");
    auto of = [&](const char *text){ bool ok = false; const QUrl url = OffscreenUrlOf(ID, QString::fromLatin1(text), &ok); return ok ? url.toString() : QStringLiteral("refused"); };
    QCOMPARE(of("lib/offscreen.html"), QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/offscreen.html"));
    QCOMPARE(of("/lib/offscreen.html"), QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/offscreen.html"));
    QCOMPARE(of("chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html?x=1#y"), QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html?x=1#y"));
    QCOMPARE(of("lib/../off.html"), QStringLiteral("refused"));
    QCOMPARE(of("../off.html"), QStringLiteral("refused"));
    QCOMPARE(of("lib/%2e%2e/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("lib/%2E%2E/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/../off.html"), QStringLiteral("refused"));
    QCOMPARE(of("lib/./off.html"), QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html"));
    QCOMPARE(of("https://a.example/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://ppppppppppppppppppppppppppppppppp/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("//another/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("lib\\off.html"), QStringLiteral("refused"));
    QCOMPARE(of(""), QStringLiteral("refused"));
    QCOMPARE(of("lib%2Foff.html"), QStringLiteral("refused"));
    QCOMPARE(of("lib%5coff.html"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://abcdefghijklmnopabcdefghijklmnop"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://abcdefghijklmnopabcdefghijklmnop/"), QStringLiteral("refused"));
    QCOMPARE(of("/"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://user@abcdefghijklmnopabcdefghijklmnop/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://abcdefghijklmnopabcdefghijklmnop:80/off.html"), QStringLiteral("refused"));
    QCOMPARE(of("chrome-extension://ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP/off.html"), QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/off.html"));
}

void tst_extensionhostwire::whoWaitsIsAnsweredOnlyWhenThereIsSomethingToSay(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QList<int> answered;
    QList<QJsonObject> replies;
    auto answer = [&](int waiter, const QJsonObject &reply){ answered << waiter; replies << reply; };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    const QList<Tab> tabs = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2);
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(tabs, true, 1, There, sees, answer);
    QCOMPARE(answered, QList<int>() << 11);
    table.Take(EXT, Token(1), 12, 2, There, nullptr);
    for(int i = 0; i < 5; i++) table.Publish(tabs, true, 3 + i, There, sees, answer);
    QCOMPARE(answered, QList<int>() << 11);
    QVERIFY(table.AnyWaiting(There));
    const QList<Tab> more = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2) << T(3);
    table.Publish(more, true, 10, There, sees, answer);
    table.Publish(more, true, 11, There, sees, answer);
    QCOMPARE(answered, QList<int>() << 11 << 12);
    QVERIFY(!table.AnyWaiting(There));
    const QJsonObject value = replies.last().value(QStringLiteral("value")).toObject();
    QCOMPARE(Said(value.value(QStringLiteral("events")).toArray()), QStringLiteral("tabs.onCreated(#3@2)"));
    QCOMPARE(QString::fromUtf8(QJsonDocument(value.value(QStringLiteral("order")).toArray()).toJson(QJsonDocument::Compact)), QStringLiteral("[1,2,3]"));
}

void tst_extensionhostwire::whatHappensBetweenTwoCallsIsToldToTheNextOne(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QStringList answered;
    auto answer = [&](int waiter, const QJsonObject &reply){
        answered << QString::number(waiter) + QStringLiteral(": ") + Said(reply.value(QStringLiteral("value")).toObject().value(QStringLiteral("events")).toArray());
    };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true), true, 1, There, sees, answer);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2), true, 2, There, sees, answer);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2) << T(3), true, 3, There, sees, answer);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 4, There, sees, answer);
    QCOMPARE(answered.size(), 1);
    table.Take(EXT, Token(1), 12, 5, There, nullptr);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(3), true, 6, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("12: tabs.onCreated(#3@1)"));
}

void tst_extensionhostwire::aCallWhichComesTwiceMakesTheOlderOneStale(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    int old = 0;
    QCOMPARE(table.Take(EXT, Token(1), 11, 0, There, &old), Table::Held);
    QCOMPARE(old, 0);
    QCOMPARE(table.Take(EXT, Token(1), 12, 1, There, &old), Table::Replaced);
    QCOMPARE(old, 11);
    QCOMPARE(table.Count(), 1);
    QCOMPARE(table.Entries().first().waiter, 12);
    Gone() << 12; old = 0;
    QCOMPARE(table.Take(EXT, Token(1), 13, 2, There, &old), Table::Held);
    QCOMPARE(old, 0);
}

void tst_extensionhostwire::beyondTheRoomACallIsRefusedAndNobodyIsPutOut(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    for(int i = 0; i < SUBSCRIBERS_OF_ONE; i++)
        QCOMPARE(table.Take(EXT, Token(i), 100 + i, 0, There, nullptr), Table::Held);
    QCOMPARE(table.Take(EXT, Token(9), 199, 0, There, nullptr), Table::Refused);
    QCOMPARE(table.Count(), SUBSCRIBERS_OF_ONE);
    for(int i = 0; i < SUBSCRIBERS_OF_ONE; i++) QCOMPARE(table.Entries().at(i).waiter, 100 + i);
    int kept = SUBSCRIBERS_OF_ONE;
    for(int e = 0; kept < SUBSCRIBERS_KEPT; e++)
        for(int i = 0; i < SUBSCRIBERS_OF_ONE && kept < SUBSCRIBERS_KEPT; i++, kept++)
            QCOMPARE(table.Take(QStringLiteral("other%1").arg(e), Token(i), 1000 + kept, 0, There, nullptr), Table::Held);
    QCOMPARE(table.Take(QStringLiteral("one too many"), Token(1), 5000, 0, There, nullptr), Table::Refused);
    QCOMPARE(table.Count(), SUBSCRIBERS_KEPT);
    QCOMPARE(table.Take(EXT, Token(0), 300, 1, There, nullptr), Table::Replaced);
}

void tst_extensionhostwire::aRequestWhichWentTakesItsSubscriptionAlong(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QList<int> answered;
    auto answer = [&](int waiter, const QJsonObject &){ answered << waiter; };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    for(int i = 0; i < SUBSCRIBERS_OF_ONE; i++) table.Take(EXT, Token(i), 100 + i, 0, There, nullptr);
    Gone() << 100 << 101 << 102 << 103;
    QVERIFY(!table.AnyWaiting(There));
    QCOMPARE(table.Take(EXT, Token(7), 200, 1, There, nullptr), Table::Held);
    QCOMPARE(table.Count(), 1);
    table.Publish(QList<Tab>() << T(1, "https://a.example/", "a", true), true, 2, There, sees, answer);
    QCOMPARE(answered, QList<int>() << 200);
}

void tst_extensionhostwire::whoDoesNotCallAgainIsForgottenAfterAWhile(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QList<int> answered;
    auto answer = [&](int waiter, const QJsonObject &){ answered << waiter; };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    const QList<Tab> tabs = QList<Tab>() << T(1, "https://a.example/", "a", true);
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(tabs, true, 1000, There, sees, answer);
    QCOMPARE(answered, QList<int>() << 11);
    table.Prune(There, 1000 + SUBSCRIBER_IDLE_MS);
    QCOMPARE(table.Count(), 1);
    table.Prune(There, 1001 + SUBSCRIBER_IDLE_MS);
    QCOMPARE(table.Count(), 0);
    table.Take(EXT, Token(2), 12, 0, There, nullptr);
    table.Prune(There, 100 * SUBSCRIBER_IDLE_MS);
    QCOMPARE(table.Count(), 1);
    QVERIFY(table.AnyWaiting(There));
}

void tst_extensionhostwire::whatAnExtensionSeesIsAskedWhenItIsAnswered(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QList<QByteArray> replies;
    auto answer = [&](int, const QJsonObject &reply){ replies << QJsonDocument(reply).toJson(QJsonDocument::Compact); };
    bool permitted = false;
    QStringList asked;
    auto sees = [&](const QString &id){ asked << id; return permitted ? ExtensionHostWire::Sight::All() : ExtensionHostWire::Sight(); };
    const QList<Tab> before = QList<Tab>() << T(1, "https://a.example/", "a", true);
    const QList<Tab> after = QList<Tab>() << T(1, "https://b.example/", "b", true);
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(before, true, 1, There, sees, answer);
    table.Take(EXT, Token(1), 12, 2, There, nullptr);
    table.Publish(after, true, 3, There, sees, answer);
    QCOMPARE(replies.size(), 1);
    QVERIFY(asked.contains(EXT));
    permitted = true;
    table.Publish(after, true, 4, There, sees, answer);
    QCOMPARE(replies.size(), 1);
    table.Publish(QList<Tab>() << T(1, "https://c.example/", "c", true), true, 5, There, sees, answer);
    QCOMPARE(replies.size(), 2);
    QVERIFY(replies.last().contains("c.example"));
}

void tst_extensionhostwire::tabsWhichOnlyChangedPlacesAreToldByTheOrder(){
    using namespace ExtensionHostWire;
    Gone().clear();
    Table table;
    QList<QByteArray> replies;
    auto answer = [&](int, const QJsonObject &reply){ replies << QJsonDocument(reply.value(QStringLiteral("value")).toObject()).toJson(QJsonDocument::Compact); };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    const QList<Tab> before = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2) << T(3);
    const QList<Tab> after = QList<Tab>() << T(3) << T(1, "https://a.example/", "a", true) << T(2);
    QVERIFY(SameOrder(before, before));
    QVERIFY(!SameOrder(before, after));
    QVERIFY(!SameOrder(before, QList<Tab>() << T(1) << T(2)));
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(before, true, 1, There, sees, answer);
    QCOMPARE(replies.size(), 1);
    table.Take(EXT, Token(1), 12, 2, There, nullptr);
    table.Publish(after, true, 3, There, sees, answer);
    QCOMPARE(replies.size(), 2);
    QCOMPARE(replies.last(), QByteArray("{\"events\":[{\"args\":[3,{\"fromIndex\":2,\"toIndex\":0,\"windowId\":1}],\"name\":\"tabs.onMoved\"}],\"order\":[3,1,2]}"));
    table.Take(EXT, Token(1), 13, 4, There, nullptr);
    table.Publish(after, true, 5, There, sees, answer);
    table.Publish(after, true, 6, There, sees, answer);
    QCOMPARE(replies.size(), 2);
    table.Take(EXT, Token(2), 21, 7, There, nullptr);
    table.Publish(QList<Tab>() << T(3) << T(1) << T(2), true, 8, There, sees, answer);
    table.Publish(QList<Tab>() << T(3) << T(1) << T(2), true, 9, There, sees, answer);
    QCOMPARE(replies.size(), 2);
}

void tst_extensionhostwire::theFocusIsNotToldWhileTheUserIsWhereTheAskerMayNotSee(){
    using namespace ExtensionHostWire;
    const QList<Tab> nowhere = QList<Tab>() << T(1) << T(2);
    const QList<Tab> atOne = QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2);
    QVERIFY(Diff(nowhere, nowhere, true, false, ExtensionHostWire::Sight::All()).isEmpty());
    QVERIFY(Diff(nowhere, nowhere, false, true, ExtensionHostWire::Sight::All()).isEmpty());
    QVERIFY(Diff(atOne, nowhere, true, false, ExtensionHostWire::Sight::All()).isEmpty());
    QCOMPARE(Said(Diff(nowhere, atOne, false, true, ExtensionHostWire::Sight::All())), QStringLiteral("tabs.onActivated(tab 1) tabs.onHighlighted(tabIds+windowId) windows.onFocusChanged(1)"));

    Gone().clear();
    Table table;
    QStringList answered;
    auto answer = [&](int waiter, const QJsonObject &reply){
        answered << QString::number(waiter) + QStringLiteral(": ") + Said(reply.value(QStringLiteral("value")).toObject().value(QStringLiteral("events")).toArray());
    };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(atOne, true, 1, There, sees, answer);
    table.Take(EXT, Token(1), 12, 2, There, nullptr);
    table.Publish(nowhere, true, 3, There, sees, answer);
    table.Publish(nowhere, false, 4, There, sees, answer);
    table.Publish(nowhere, true, 5, There, sees, answer);
    table.Publish(nowhere, false, 6, There, sees, answer);
    QCOMPARE(answered, QStringList() << QStringLiteral("11: tabs.onActivated(tab 1)"));
    table.Publish(atOne, false, 7, There, sees, answer);
    QCOMPARE(answered.last(), QStringLiteral("12: tabs.onActivated(tab 1) tabs.onHighlighted(tabIds+windowId)"));
}

namespace {
    Tab Leaf(qint64 id, const char *url, const char *title, qint64 visited, qint64 added = 0){
        Tab t = T(id, url, title); t.visited = visited; t.added = added; return t;
    }
    const qint64 NOW = 1000LL * 24 * 60 * 60 * 1000;
    const qint64 DAY = 24 * 60 * 60 * 1000;
    QList<Tab> Leaves(){
        return QList<Tab>()
            << Leaf(11, "https://a.example/one?x=1", "One", NOW - 3 * DAY)
            << Leaf(12, "https://b.example/two", "two", NOW - 1000)
            << Leaf(15, "https://c.example/", "Three", NOW - 2 * DAY, NOW - 5 * DAY)
            << Leaf(20, "https://b.example/two", "two again", NOW - 1000)
            << Leaf(21, "file:///C:/notes.html", "notes", 0, NOW - 30 * DAY)
            << Leaf(22, "https://d.example/", "none", 0, 0);
    }
    QJsonObject AskHistory(const QString &args, qint64 now = NOW){
        return ExtensionHostWire::History(ExtensionHostWire::ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"history.search\",\"args\":%1}").arg(args))), Leaves(), now);
    }
    QString HistoryIds(const QJsonObject &answer){
        if(!answer[QStringLiteral("ok")].toBool()) return QStringLiteral("ERROR");
        QStringList ids;
        foreach(const QJsonValue &item, answer[QStringLiteral("value")].toArray()) ids << item.toObject()[QStringLiteral("id")].toString();
        return ids.join(QLatin1Char(','));
    }
    using ExtensionHostWire::Bookmark;
    Bookmark Folder(qint64 id, const char *title, bool told, const QList<Bookmark> &children, qint64 added = 0){
        Bookmark b; b.id = id; b.folder = true; b.told = told; b.title = QString::fromLatin1(title); b.children = children; b.added = added; return b;
    }
    Bookmark Page(qint64 id, const char *title, bool told = true, qint64 added = 0){
        Bookmark b; b.id = id; b.told = told; b.title = QString::fromLatin1(title); b.url = QUrl(QStringLiteral("https://x.example/") + QString::number(id)); b.added = added; return b;
    }
    QString Drawn(const Bookmark &node){
        if(!node.folder) return node.title;
        QStringList children;
        foreach(const Bookmark &child, node.children) children << Drawn(child);
        return node.title + QLatin1Char('(') + children.join(QLatin1Char(' ')) + QLatin1Char(')');
    }
    Bookmark Root(){
        return Folder(1, "root", true, QList<Bookmark>()
            << Folder(2, "A", true, QList<Bookmark>() << Page(3, "a1", true, 5000) << Folder(4, "P", false, QList<Bookmark>() << Page(5, "p-view") << Page(6, "p-noview", false)))
            << Folder(7, "X", false, QList<Bookmark>() << Page(8, "x-leaf", false) << Folder(9, "Y", true, QList<Bookmark>() << Page(10, "y1")) << Page(11, "x-view"))
            << Folder(12, "E", false, QList<Bookmark>() << Page(13, "e", false) << Folder(14, "EE", false, QList<Bookmark>()))
            << Page(15, "top"), 7000);
    }
    QJsonObject AskTree(const QString &args){
        return ExtensionHostWire::BookmarkTree(ExtensionHostWire::ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"bookmarks.getTree\",\"args\":%1}").arg(args))), Root());
    }
}

void tst_extensionhostwire::historyIsTheLeavesNewestFirst_data(){
    QTest::addColumn<QString>("args");
    QTest::addColumn<QString>("ids");
    QTest::newRow("everything, newest first")   << "[{\"text\":\"\",\"startTime\":0}]" << "12,20,15,11,21,22";
    QTest::newRow("a day, when nobody said")    << "[{\"text\":\"\"}]" << "12,20";
    QTest::newRow("since two days ago")         << "[{\"text\":\"\",\"startTime\":%1}]" << "12,20,15";
    QTest::newRow("a fraction of a moment")     << "[{\"text\":\"\",\"startTime\":%1.5}]" << "12,20";
    QTest::newRow("before, not at, the end")    << "[{\"text\":\"\",\"startTime\":0,\"endTime\":%1}]" << "11,21,22";
    QTest::newRow("an end before the start")    << "[{\"text\":\"\",\"startTime\":%1,\"endTime\":0}]" << "";
    QTest::newRow("so many")                    << "[{\"text\":\"\",\"startTime\":0,\"maxResults\":2}]" << "12,20";
    QTest::newRow("zero is no bound")           << "[{\"text\":\"\",\"startTime\":0,\"maxResults\":0}]" << "12,20,15,11,21,22";
    QTest::newRow("a word, whatever the case")  << "[{\"text\":\"ONE\",\"startTime\":0}]" << "11,22";
    QTest::newRow("every word must be there")   << "[{\"text\":\"two again\",\"startTime\":0}]" << "20";
    QTest::newRow("in the address or the title") << "[{\"text\":\"c.example\",\"startTime\":0}]" << "15";
    QTest::newRow("blanks are nothing")         << "[{\"text\":\"  \",\"startTime\":0,\"maxResults\":1}]" << "12";
    QTest::newRow("nulls nobody gave")          << "[{\"text\":\"\",\"startTime\":0,\"endTime\":null},null]" << "12,20,15,11,21,22";
    QTest::newRow("no text")                    << "[{\"startTime\":0}]" << "ERROR";
    QTest::newRow("text of the wrong kind")     << "[{\"text\":1}]" << "ERROR";
    QTest::newRow("nothing")                    << "[]" << "ERROR";
    QTest::newRow("not an object")              << "[\"one\"]" << "ERROR";
    QTest::newRow("two objects")                << "[{\"text\":\"\"},{\"text\":\"\"}]" << "ERROR";
    QTest::newRow("a key nobody knows")         << "[{\"text\":\"\",\"foo\":1}]" << "ERROR";
    QTest::newRow("a key nobody knows, null")   << "[{\"text\":\"\",\"startTime\":0,\"foo\":null}]" << "ERROR";
    QTest::newRow("a count nobody gave")        << "[{\"text\":\"\",\"startTime\":null,\"maxResults\":null}]" << "12,20";
    QTest::newRow("a negative count")           << "[{\"text\":\"\",\"maxResults\":-1}]" << "ERROR";
    QTest::newRow("a fraction of a count")      << "[{\"text\":\"\",\"maxResults\":1.5}]" << "ERROR";
    QTest::newRow("a count in words")           << "[{\"text\":\"\",\"maxResults\":\"3\"}]" << "ERROR";
    QTest::newRow("a time in words")            << "[{\"text\":\"\",\"startTime\":\"0\"}]" << "ERROR";
}

void tst_extensionhostwire::historyIsTheLeavesNewestFirst(){
    QFETCH(QString, args);
    QFETCH(QString, ids);
    QCOMPARE(HistoryIds(AskHistory(args.contains(QLatin1String("%1")) ? args.arg(NOW - 2 * DAY) : args)), ids);
}

void tst_extensionhostwire::aHistoryItemAsItIsTold(){
    const QJsonArray items = AskHistory(QStringLiteral("[{\"text\":\"\",\"startTime\":0}]"))[QStringLiteral("value")].toArray();
    QCOMPARE(items.size(), 6);
    const QJsonObject first = items.at(0).toObject();
    QVERIFY(first[QStringLiteral("id")].isString());
    QCOMPARE(first[QStringLiteral("id")].toString(), QStringLiteral("12"));
    QCOMPARE(first[QStringLiteral("url")].toString(), QStringLiteral("https://b.example/two"));
    QCOMPARE(first[QStringLiteral("title")].toString(), QStringLiteral("two"));
    QCOMPARE(static_cast<qint64>(first[QStringLiteral("lastVisitTime")].toDouble()), NOW - 1000);
    QVERIFY(!first.contains(QStringLiteral("visitCount")));
    QVERIFY(!first.contains(QStringLiteral("typedCount")));
    QCOMPARE(items.at(1).toObject()[QStringLiteral("url")].toString(), first[QStringLiteral("url")].toString());
    QCOMPARE(items.at(1).toObject()[QStringLiteral("id")].toString(), QStringLiteral("20"));
    QCOMPARE(static_cast<qint64>(items.at(4).toObject()[QStringLiteral("lastVisitTime")].toDouble()), NOW - 30 * DAY);
    QCOMPARE(static_cast<qint64>(items.at(5).toObject()[QStringLiteral("lastVisitTime")].toDouble()), 0);
    const QJsonObject tab = Ask(QStringLiteral("tabs.get"), QStringLiteral("[11]"))[QStringLiteral("value")].toObject();
    QCOMPARE(items.at(3).toObject()[QStringLiteral("url")].toString(), tab[QStringLiteral("url")].toString());
}

void tst_extensionhostwire::theTreeIsToldAsBookmarks(){
    using namespace ExtensionHostWire;
    QCOMPARE(Drawn(Shown(Root())), QStringLiteral("root(A(a1 p-view) Y(y1) x-view top)"));
    Bookmark untold = Root(); untold.told = false;
    QCOMPARE(Drawn(Shown(untold)), QStringLiteral("root(A(a1 p-view) Y(y1) x-view top)"));
    QCOMPARE(Drawn(Shown(Folder(1, "r", true, QList<Bookmark>() << Folder(2, "u", false, QList<Bookmark>() << Folder(3, "uu", false, QList<Bookmark>() << Page(4, "deep"))) << Page(5, "after")))),
             QStringLiteral("r(deep after)"));
    QCOMPARE(Drawn(Shown(Folder(1, "r", true, QList<Bookmark>() << Folder(2, "u", false, QList<Bookmark>() << Page(3, "x", false))))), QStringLiteral("r()"));
}

void tst_extensionhostwire::aBookmarkAsItIsTold(){
    const QJsonObject answer = AskTree(QStringLiteral("[]"));
    QCOMPARE(answer[QStringLiteral("ok")].toBool(), true);
    const QJsonArray trees = answer[QStringLiteral("value")].toArray();
    QCOMPARE(trees.size(), 1);
    const QJsonObject root = trees.at(0).toObject();
    QCOMPARE(root[QStringLiteral("id")].toString(), QStringLiteral("0"));
    QCOMPARE(root[QStringLiteral("title")].toString(), QString());
    QVERIFY(root.contains(QStringLiteral("title")));
    QVERIFY(!root.contains(QStringLiteral("parentId")) && !root.contains(QStringLiteral("index")) && !root.contains(QStringLiteral("url")));
    QCOMPARE(static_cast<qint64>(root[QStringLiteral("dateAdded")].toDouble()), 7000);
    const QJsonArray top = root[QStringLiteral("children")].toArray();
    QCOMPARE(top.size(), 4);
    const QJsonObject a = top.at(0).toObject();
    QCOMPARE(a[QStringLiteral("id")].toString(), QStringLiteral("2"));
    QCOMPARE(a[QStringLiteral("parentId")].toString(), QStringLiteral("0"));
    QCOMPARE(a[QStringLiteral("index")].toInt(), 0);
    QCOMPARE(a[QStringLiteral("title")].toString(), QStringLiteral("A"));
    QVERIFY(!a.contains(QStringLiteral("url")));
    QVERIFY(!a.contains(QStringLiteral("dateAdded")));
    const QJsonArray inA = a[QStringLiteral("children")].toArray();
    QCOMPARE(inA.size(), 2);
    const QJsonObject a1 = inA.at(0).toObject();
    QCOMPARE(a1[QStringLiteral("id")].toString(), QStringLiteral("3"));
    QCOMPARE(a1[QStringLiteral("parentId")].toString(), QStringLiteral("2"));
    QCOMPARE(a1[QStringLiteral("index")].toInt(), 0);
    QCOMPARE(a1[QStringLiteral("url")].toString(), QStringLiteral("https://x.example/3"));
    QCOMPARE(static_cast<qint64>(a1[QStringLiteral("dateAdded")].toDouble()), 5000);
    QVERIFY(!a1.contains(QStringLiteral("children")));
    QCOMPARE(inA.at(1).toObject()[QStringLiteral("title")].toString(), QStringLiteral("p-view"));
    QCOMPARE(inA.at(1).toObject()[QStringLiteral("parentId")].toString(), QStringLiteral("2"));
    QCOMPARE(inA.at(1).toObject()[QStringLiteral("index")].toInt(), 1);
    QCOMPARE(top.at(1).toObject()[QStringLiteral("title")].toString(), QStringLiteral("Y"));
    QCOMPARE(top.at(1).toObject()[QStringLiteral("parentId")].toString(), QStringLiteral("0"));
    QCOMPARE(top.at(3).toObject()[QStringLiteral("title")].toString(), QStringLiteral("top"));
    QCOMPARE(top.at(3).toObject()[QStringLiteral("index")].toInt(), 3);
    const QByteArray text = QJsonDocument(answer).toJson(QJsonDocument::Compact);
    foreach(const char *hidden, QList<const char *>() << "\"P\"" << "\"X\"" << "\"E\"" << "\"EE\"" << "p-noview" << "x-leaf" << "\"e\"" << "\"4\"" << "\"7\"" << "\"12\"")
        QVERIFY2(!text.contains(hidden), hidden);
    QCOMPARE(AskTree(QStringLiteral("[null]"))[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(AskTree(QStringLiteral("[1]"))[QStringLiteral("ok")].toBool(true), false);
    QCOMPARE(AskTree(QStringLiteral("[{}]"))[QStringLiteral("ok")].toBool(true), false);
    QCOMPARE(ExtensionHostWire::NotAvailable(QStringLiteral("bookmarks.getTree")), QStringLiteral("chrome.bookmarks.getTree is not available in this browser"));
}

void tst_extensionhostwire::aDateIsNoChangeOfATab(){
    using namespace ExtensionHostWire;
    const QJsonObject told = Answer(ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{}]}"))), Leaves(), Sight::All())[QStringLiteral("value")].toArray().at(0).toObject();
    QVERIFY(!told.contains(QStringLiteral("lastAccessed")));
    QVERIFY(!QJsonDocument(told).toJson().contains("isit"));
    const QList<Tab> before = QList<Tab>() << Leaf(1, "https://a.example/", "a", 100, 50) << Leaf(2, "https://b.example/", "b", 100, 50);
    const QList<Tab> after = QList<Tab>() << Leaf(1, "https://a.example/", "a", 900, 50) << Leaf(2, "https://b.example/", "b", 100, 60);
    QVERIFY(Diff(before, after, true, true, ExtensionHostWire::Sight::All()).isEmpty());
    QVERIFY(SameOrder(before, after));
    Gone().clear();
    Table table;
    int answered = 0;
    auto answer = [&](int, const QJsonObject &){ answered++; };
    auto sees = [](const QString &){ return ExtensionHostWire::Sight::All(); };
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(before, true, 1, There, sees, answer);
    QCOMPARE(answered, 0);
    table.Publish(after, true, 2, There, sees, answer);
    QCOMPARE(answered, 0);
}

namespace {
    QJsonObject AskBookmarks(const QString &api, const QString &args){
        return ExtensionHostWire::Bookmarks(ExtensionHostWire::ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"bookmarks.%1\",\"args\":%2}").arg(api, args))), Root());
    }
    QString BookmarkIds(const QJsonObject &answer){
        if(!answer[QStringLiteral("ok")].toBool()) return QStringLiteral("ERROR:") + answer[QStringLiteral("error")].toString();
        QStringList ids;
        foreach(const QJsonValue &item, answer[QStringLiteral("value")].toArray()) ids << item.toObject()[QStringLiteral("id")].toString();
        return ids.join(QLatin1Char(','));
    }
}

void tst_extensionhostwire::bookmarksAreReadByIdAndByWord_data(){
    QTest::addColumn<QString>("api");
    QTest::addColumn<QString>("args");
    QTest::addColumn<QString>("ids");
    QTest::newRow("get the root")               << "get" << "[\"0\"]" << "0";
    QTest::newRow("get two, in that order")     << "get" << "[[\"15\",\"3\"]]" << "15,3";
    QTest::newRow("get none")                   << "get" << "[[]]" << "";
    QTest::newRow("get a folder not told")      << "get" << "[\"4\"]" << "ERROR:Can't find bookmark for id.";
    QTest::newRow("get a leaf not told")        << "get" << "[\"6\"]" << "ERROR:Can't find bookmark for id.";
    QTest::newRow("get the root by its number") << "get" << "[\"1\"]" << "ERROR:Can't find bookmark for id.";
    QTest::newRow("get by a word")              << "get" << "[\"abc\"]" << "ERROR:Bookmark id is invalid.";
    QTest::newRow("get by nothing")             << "get" << "[\"\"]" << "ERROR:Bookmark id is invalid.";
    QTest::newRow("get by a minus")             << "get" << "[\"-3\"]" << "ERROR:Bookmark id is invalid.";
    QTest::newRow("get by a number")            << "get" << "[3]" << "ERROR:bookmarks.get takes an id or a list of ids";
    QTest::newRow("get the root as 00")         << "get" << "[\"00\"]" << "0";
    QTest::newRow("get by a leading zero")      << "get" << "[\"03\"]" << "3";
    QTest::newRow("get by eighteen digits")     << "get" << "[\"999999999999999999\"]" << "ERROR:Can't find bookmark for id.";
    QTest::newRow("get by nineteen digits")     << "get" << "[\"1000000000000000000\"]" << "ERROR:Bookmark id is invalid.";
    QTest::newRow("get a list with a number")   << "get" << "[[\"3\",4]]" << "ERROR:Bookmark id is invalid.";
    QTest::newRow("get with a null nobody gave") << "get" << "[\"3\",null]" << "3";
    QTest::newRow("get nothing at all")         << "get" << "[]" << "ERROR:bookmarks.get takes one argument";
    QTest::newRow("the root's children")        << "getChildren" << "[\"0\"]" << "2,9,11,15";
    QTest::newRow("a folder's children")        << "getChildren" << "[\"2\"]" << "3,5";
    QTest::newRow("a leaf's children")          << "getChildren" << "[\"3\"]" << "";
    QTest::newRow("children of what is not told") << "getChildren" << "[\"7\"]" << "ERROR:Can't find bookmark for id.";
    QTest::newRow("children of a list")         << "getChildren" << "[[\"0\"]]" << "ERROR:Bookmark id is invalid.";
    QTest::newRow("children of nothing")        << "getChildren" << "[]" << "ERROR:bookmarks.getChildren takes one argument";
    QTest::newRow("children of two")            << "getChildren" << "[\"0\",\"2\"]" << "ERROR:bookmarks.getChildren takes one argument";
    QTest::newRow("a subtree")                  << "getSubTree" << "[\"9\"]" << "9";
    QTest::newRow("a subtree not told")         << "getSubTree" << "[\"12\"]" << "ERROR:Can't find bookmark for id.";
    QTest::newRow("a word")                     << "search" << "[\"a1\"]" << "3";
    QTest::newRow("no word")                    << "search" << "[\"\"]" << "";
    QTest::newRow("blanks")                     << "search" << "[\"   \"]" << "";
    QTest::newRow("a word of what is not told") << "search" << "[\"noview\"]" << "";
    QTest::newRow("a word, whatever the case")  << "search" << "[\"VIEW\"]" << "5,11";
    QTest::newRow("every word must be there")   << "search" << "[\"view p-\"]" << "5";
    QTest::newRow("a word of the address")      << "search" << "[\"x.example/1\"]" << "10,11,15";
    QTest::newRow("a query")                    << "search" << "[{\"query\":\"y1\"}]" << "10";
    QTest::newRow("an address")                 << "search" << "[{\"url\":\"https://x.example/10\"}]" << "10";
    QTest::newRow("an address, as an address")  << "search" << "[{\"url\":\"https://X.example/10\"}]" << "10";
    QTest::newRow("an address and a title")     << "search" << "[{\"url\":\"https://x.example/10\",\"title\":\"nope\"}]" << "";
    QTest::newRow("a query and an address")     << "search" << "[{\"query\":\"Y\",\"url\":\"https://x.example/10\"}]" << "10";
    QTest::newRow("a title, whole")             << "search" << "[{\"title\":\"Y\"}]" << "9";
    QTest::newRow("a title, as it is")          << "search" << "[{\"title\":\"y\"}]" << "";
    QTest::newRow("nothing in particular")      << "search" << "[{}]" << "2,3,5,9,10,11,15";
    QTest::newRow("nulls nobody gave")          << "search" << "[{\"query\":null,\"url\":null,\"title\":null}]" << "2,3,5,9,10,11,15";
    QTest::newRow("a key nobody knows")         << "search" << "[{\"foo\":1}]" << "ERROR:bookmarks.search: 'foo' is not a key of it";
    QTest::newRow("a query of the wrong kind")  << "search" << "[{\"query\":1}]" << "ERROR:bookmarks.search: 'query' has a value of the wrong kind";
    QTest::newRow("a number")                   << "search" << "[1]" << "ERROR:bookmarks.search takes words or an object";
    QTest::newRow("two words apart")            << "search" << "[\"a\",\"b\"]" << "ERROR:bookmarks.search takes one argument";
}

void tst_extensionhostwire::bookmarksAreReadByIdAndByWord(){
    QFETCH(QString, api);
    QFETCH(QString, args);
    QFETCH(QString, ids);
    QCOMPARE(BookmarkIds(AskBookmarks(api, args)), ids);
}

void tst_extensionhostwire::aBookmarkReadIsShapedAsTheTreeIs(){
    const QJsonObject a = AskBookmarks(QStringLiteral("get"), QStringLiteral("[\"2\"]"))[QStringLiteral("value")].toArray().at(0).toObject();
    QCOMPARE(a[QStringLiteral("id")].toString(), QStringLiteral("2"));
    QCOMPARE(a[QStringLiteral("parentId")].toString(), QStringLiteral("0"));
    QCOMPARE(a[QStringLiteral("index")].toInt(), 0);
    QCOMPARE(a[QStringLiteral("title")].toString(), QStringLiteral("A"));
    QVERIFY(!a.contains(QStringLiteral("children")) && !a.contains(QStringLiteral("url")));
    const QJsonObject root = AskBookmarks(QStringLiteral("get"), QStringLiteral("[\"0\"]"))[QStringLiteral("value")].toArray().at(0).toObject();
    QCOMPARE(root[QStringLiteral("id")].toString(), QStringLiteral("0"));
    QCOMPARE(root[QStringLiteral("title")].toString(), QString());
    QVERIFY(!root.contains(QStringLiteral("parentId")) && !root.contains(QStringLiteral("index")) && !root.contains(QStringLiteral("children")));
    const QJsonArray inA = AskBookmarks(QStringLiteral("getChildren"), QStringLiteral("[\"2\"]"))[QStringLiteral("value")].toArray();
    QCOMPARE(inA.size(), 2);
    QCOMPARE(inA.at(0).toObject()[QStringLiteral("id")].toString(), QStringLiteral("3"));
    QCOMPARE(inA.at(0).toObject()[QStringLiteral("parentId")].toString(), QStringLiteral("2"));
    QCOMPARE(inA.at(0).toObject()[QStringLiteral("index")].toInt(), 0);
    QCOMPARE(inA.at(0).toObject()[QStringLiteral("url")].toString(), QStringLiteral("https://x.example/3"));
    QCOMPARE(static_cast<qint64>(inA.at(0).toObject()[QStringLiteral("dateAdded")].toDouble()), 5000);
    QCOMPARE(inA.at(1).toObject()[QStringLiteral("title")].toString(), QStringLiteral("p-view"));
    QCOMPARE(inA.at(1).toObject()[QStringLiteral("index")].toInt(), 1);
    QVERIFY(!inA.at(1).toObject().contains(QStringLiteral("children")));
    const QJsonArray sub = AskBookmarks(QStringLiteral("getSubTree"), QStringLiteral("[\"2\"]"))[QStringLiteral("value")].toArray();
    QCOMPARE(sub.size(), 1);
    QCOMPARE(sub.at(0).toObject()[QStringLiteral("id")].toString(), QStringLiteral("2"));
    QCOMPARE(sub.at(0).toObject()[QStringLiteral("parentId")].toString(), QStringLiteral("0"));
    QCOMPARE(sub.at(0).toObject()[QStringLiteral("children")].toArray(), inA);
    QCOMPARE(AskBookmarks(QStringLiteral("getSubTree"), QStringLiteral("[\"0\"]"))[QStringLiteral("value")].toArray(),
             AskTree(QStringLiteral("[]"))[QStringLiteral("value")].toArray());
    const QJsonObject found = AskBookmarks(QStringLiteral("search"), QStringLiteral("[\"Y\"]"))[QStringLiteral("value")].toArray().at(0).toObject();
    QCOMPARE(found[QStringLiteral("id")].toString(), QStringLiteral("9"));
    QCOMPARE(found[QStringLiteral("parentId")].toString(), QStringLiteral("0"));
    QCOMPARE(found[QStringLiteral("index")].toInt(), 1);
    QVERIFY(!found.contains(QStringLiteral("children")));
    foreach(const QString &answer, QStringList()
            << QJsonDocument(AskBookmarks(QStringLiteral("getSubTree"), QStringLiteral("[\"0\"]"))).toJson(QJsonDocument::Compact)
            << QJsonDocument(AskBookmarks(QStringLiteral("search"), QStringLiteral("[{}]"))).toJson(QJsonDocument::Compact)
            << QJsonDocument(AskBookmarks(QStringLiteral("getChildren"), QStringLiteral("[\"0\"]"))).toJson(QJsonDocument::Compact))
        foreach(const char *hidden, QList<const char *>() << "\"P\"" << "\"X\"" << "\"E\"" << "\"EE\"" << "p-noview" << "x-leaf" << "\"e\"" << "\"4\"" << "\"7\"" << "\"12\"")
            QVERIFY2(!answer.contains(QLatin1String(hidden)), hidden);
    QVERIFY(!ExtensionHostWire::IsBookmarksRead(QStringLiteral("bookmarks.create")));
    QCOMPARE(AskBookmarks(QStringLiteral("create"), QStringLiteral("[{\"title\":\"x\"}]"))[QStringLiteral("error")].toString(),
             QStringLiteral("chrome.bookmarks.create is not available in this browser"));
}

namespace {
    QStringList Of(const QJsonArray &events, const char *name){
        QStringList out;
        foreach(const QJsonValue &v, events){
            const QJsonObject event = v.toObject();
            if(event[QStringLiteral("name")].toString() != QLatin1String(name)) continue;
            out << QString::fromUtf8(QJsonDocument(event[QStringLiteral("args")].toArray().at(0).toObject()).toJson(QJsonDocument::Compact));
        }
        return out;
    }
    QStringList NamesOf(const QJsonArray &events){
        QStringList out;
        foreach(const QJsonValue &v, events) out << v.toObject()[QStringLiteral("name")].toString();
        return out;
    }
}

void tst_extensionhostwire::aVisitIsToldToWhoHasTheHistoryPermission(){
    using namespace ExtensionHostWire;
    Sight hist = Sight::All();
    hist.history = true;
    const QList<Tab> before = QList<Tab>()
        << Leaf(1, "https://a.example/", "a", 100, 50) << Leaf(2, "https://b.example/", "b", 100, 50) << Leaf(3, "https://b.example/", "b again", 100, 50);

    QList<Tab> looked = before;
    looked[0].visited = 900;
    QCOMPARE(NamesOf(Diff(before, looked, true, true, hist)), QStringList() << QStringLiteral("history.onVisited"));
    QCOMPARE(Of(Diff(before, looked, true, true, hist), "history.onVisited"),
             QStringList() << QStringLiteral("{\"id\":\"1\",\"lastVisitTime\":900,\"title\":\"a\",\"url\":\"https://a.example/\"}"));
    QVERIFY(Diff(before, looked, true, true, Sight::All()).isEmpty());

    QList<Tab> come = before;
    come << Leaf(4, "https://c.example/", "c", 0, 700);
    QCOMPARE(NamesOf(Diff(before, come, true, true, hist)), QStringList() << QStringLiteral("tabs.onCreated") << QStringLiteral("history.onVisited"));
    QCOMPARE(Of(Diff(before, come, true, true, hist), "history.onVisited"),
             QStringList() << QStringLiteral("{\"id\":\"4\",\"lastVisitTime\":700,\"title\":\"c\",\"url\":\"https://c.example/\"}"));
    QCOMPARE(NamesOf(Diff(before, come, true, true, Sight::All())), QStringList() << QStringLiteral("tabs.onCreated"));

    QList<Tab> moved = before;
    moved[0].url = QUrl(QStringLiteral("https://a.example/next"));
    QCOMPARE(NamesOf(Diff(before, moved, true, true, hist)),
             QStringList() << QStringLiteral("tabs.onUpdated") << QStringLiteral("history.onVisitRemoved") << QStringLiteral("history.onVisited"));
    QCOMPARE(Of(Diff(before, moved, true, true, hist), "history.onVisitRemoved"),
             QStringList() << QStringLiteral("{\"allHistory\":false,\"urls\":[\"https://a.example/\"]}"));
    QCOMPARE(Of(Diff(before, moved, true, true, hist), "history.onVisited"),
             QStringList() << QStringLiteral("{\"id\":\"1\",\"lastVisitTime\":100,\"title\":\"a\",\"url\":\"https://a.example/next\"}"));
    QList<Tab> joined = before;
    joined[0].url = QUrl(QStringLiteral("https://b.example/"));
    QCOMPARE(Of(Diff(before, joined, true, true, hist), "history.onVisitRemoved"),
             QStringList() << QStringLiteral("{\"allHistory\":false,\"urls\":[\"https://a.example/\"]}"));
    QCOMPARE(Of(Diff(before, joined, true, true, hist), "history.onVisited").size(), 1);

    QList<Tab> oneLess = before;
    oneLess.removeLast();
    QCOMPARE(NamesOf(Diff(before, oneLess, true, true, hist)), QStringList() << QStringLiteral("tabs.onRemoved"));
    QList<Tab> bothGone = before;
    bothGone.removeLast();
    bothGone.removeLast();
    QCOMPARE(NamesOf(Diff(before, bothGone, true, true, hist)),
             QStringList() << QStringLiteral("tabs.onRemoved") << QStringLiteral("tabs.onRemoved") << QStringLiteral("history.onVisitRemoved"));
    QCOMPARE(Of(Diff(before, bothGone, true, true, hist), "history.onVisitRemoved"),
             QStringList() << QStringLiteral("{\"allHistory\":false,\"urls\":[\"https://b.example/\"]}"));

    Sight narrow = Sight::Of(false, QString(), QStringList() << QStringLiteral("https://z.example/*"));
    narrow.history = true;
    const QJsonArray events = Diff(before, moved, true, true, narrow);
    QCOMPARE(NamesOf(events), QStringList() << QStringLiteral("history.onVisitRemoved") << QStringLiteral("history.onVisited"));
    QCOMPARE(Of(events, "history.onVisited").first(), QStringLiteral("{\"id\":\"1\",\"lastVisitTime\":100,\"title\":\"a\",\"url\":\"https://a.example/next\"}"));

    QList<Tab> blank = before;
    blank[0].url = QUrl();
    QCOMPARE(NamesOf(Diff(before, blank, true, true, hist)), QStringList() << QStringLiteral("tabs.onUpdated") << QStringLiteral("history.onVisitRemoved"));
    QCOMPARE(Of(Diff(before, blank, true, true, hist), "history.onVisitRemoved"),
             QStringList() << QStringLiteral("{\"allHistory\":false,\"urls\":[\"https://a.example/\"]}"));
    QCOMPARE(NamesOf(Diff(blank, before, true, true, hist)), QStringList() << QStringLiteral("tabs.onUpdated") << QStringLiteral("history.onVisited"));
    QList<Tab> blankAgain = blank;
    blankAgain[0].visited = 900;
    QVERIFY(Diff(blank, blankAgain, true, true, hist).isEmpty());
    QCOMPARE(History(ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"history.search\",\"args\":[{\"text\":\"\",\"startTime\":0}]}"))), blank, 1000)[QStringLiteral("value")].toArray().size(), 2);

    Gone().clear();
    Table table;
    int answered = 0;
    QJsonObject last;
    auto answer = [&](int, const QJsonObject &reply){ answered++; last = reply; };
    auto sees = [&](const QString &){ return hist; };
    table.Take(EXT, Token(1), 11, 0, There, nullptr);
    table.Publish(before, true, 1, There, sees, answer);
    QCOMPARE(answered, 0);
    table.Publish(looked, true, 2, There, sees, answer);
    QCOMPARE(answered, 1);
    QCOMPARE(NamesOf(last[QStringLiteral("value")].toObject()[QStringLiteral("events")].toArray()), QStringList() << QStringLiteral("history.onVisited"));
}

namespace {
    QList<qint64> Applied(QList<qint64> ids, const QJsonArray &events, QString *bad){
        foreach(const QJsonValue &v, events){
            const QJsonObject event = v.toObject();
            const QString name = event.value(QStringLiteral("name")).toString();
            const QJsonArray args = event.value(QStringLiteral("args")).toArray();
            if(name == QStringLiteral("tabs.onRemoved")){
                ids.removeOne(static_cast<qint64>(args.at(0).toDouble()));
            } else if(name == QStringLiteral("tabs.onMoved")){
                const qint64 id = static_cast<qint64>(args.at(0).toDouble());
                const int from = args.at(1).toObject().value(QStringLiteral("fromIndex")).toInt(-1);
                const int to = args.at(1).toObject().value(QStringLiteral("toIndex")).toInt(-1);
                if(from < 0 || from >= ids.size() || ids.at(from) != id){ *bad = QStringLiteral("%1 is not at %2").arg(id).arg(from); return ids; }
                ids.removeAt(from);
                if(to < 0 || to > ids.size()){ *bad = QStringLiteral("no place %1").arg(to); return ids; }
                ids.insert(to, id);
            } else if(name == QStringLiteral("tabs.onCreated")){
                const QJsonObject tab = args.at(0).toObject();
                const int at = tab.value(QStringLiteral("index")).toInt(-1);
                if(at < 0 || at > ids.size()){ *bad = QStringLiteral("no place %1").arg(at); return ids; }
                ids.insert(at, static_cast<qint64>(tab.value(QStringLiteral("id")).toDouble()));
            }
        }
        return ids;
    }
    QList<qint64> IdsOf(const QList<Tab> &tabs){
        QList<qint64> ids;
        foreach(const Tab &tab, tabs) ids << tab.id;
        return ids;
    }
    QString MovesOf(const QJsonArray &events){
        QStringList out;
        foreach(const QJsonValue &v, events){
            const QJsonObject event = v.toObject();
            if(event.value(QStringLiteral("name")).toString() != QStringLiteral("tabs.onMoved")) continue;
            const QJsonArray args = event.value(QStringLiteral("args")).toArray();
            out << QStringLiteral("%1:%2>%3").arg(static_cast<qint64>(args.at(0).toDouble()))
                   .arg(args.at(1).toObject().value(QStringLiteral("fromIndex")).toInt())
                   .arg(args.at(1).toObject().value(QStringLiteral("toIndex")).toInt());
        }
        return out.join(QLatin1Char(' '));
    }
    QList<Tab> TabsOfIds(const QList<qint64> &ids){
        QList<Tab> tabs;
        foreach(qint64 id, ids) tabs << T(id);
        return tabs;
    }
    QString UpdatesOf(const QJsonArray &events){
        QStringList out;
        foreach(const QJsonValue &v, events){
            const QJsonObject event = v.toObject();
            if(event.value(QStringLiteral("name")).toString() != QStringLiteral("tabs.onUpdated")) continue;
            const QJsonArray args = event.value(QStringLiteral("args")).toArray();
            const QJsonObject change = args.at(1).toObject();
            QStringList keys = change.keys();
            if(change.contains(QStringLiteral("status"))) keys.replaceInStrings(QRegularExpression(QStringLiteral("^status$")), QStringLiteral("status=") + change.value(QStringLiteral("status")).toString());
            out << keys.join(QLatin1Char('+')) + QLatin1Char('[') + args.at(2).toObject().value(QStringLiteral("status")).toString() + QLatin1Char(']');
        }
        return out.join(QLatin1Char(' '));
    }
}

void tst_extensionhostwire::theEventsAppliedInTurnGiveTheListAfter(){
    using namespace ExtensionHostWire;
    QRandomGenerator random(415);
    for(int round = 0; round < 2000; round++){
        const int size = random.bounded(0, 12);
        QList<qint64> before;
        for(int i = 1; i <= size; i++) before << i;
        QList<qint64> after;
        foreach(qint64 id, before) if(random.bounded(0, 5)) after << id;
        for(int i = after.size() - 1; i > 0; i--) after.swapItemsAt(i, random.bounded(0, i + 1));
        const int made = random.bounded(0, 4);
        for(int i = 0; i < made; i++) after.insert(random.bounded(0, after.size() + 1), 100 + i);
        const QJsonArray events = Diff(TabsOfIds(before), TabsOfIds(after), true, true, Sight::All());
        QString bad;
        const QList<qint64> applied = Applied(before, events, &bad);
        QVERIFY2(bad.isEmpty(), qPrintable(bad));
        QCOMPARE(applied, after);
        QVERIFY(MovesOf(events).count(QLatin1Char(':')) <= after.size());
    }
}

void tst_extensionhostwire::aTabWhichMovedIsToldOfItself(){
    using namespace ExtensionHostWire;
    auto moves = [](const QList<qint64> &before, const QList<qint64> &after){
        return MovesOf(Diff(TabsOfIds(before), TabsOfIds(after), true, true, Sight::All()));
    };
    QCOMPARE(moves({1, 2, 3}, {2, 3, 1}), QStringLiteral("1:0>2"));
    QCOMPARE(moves({1, 2, 3}, {3, 1, 2}), QStringLiteral("3:2>0"));
    QCOMPARE(moves({1, 2}, {2, 1}), QStringLiteral("1:0>1"));
    QCOMPARE(moves({1, 2, 3}, {1, 3}), QString());
    QCOMPARE(moves({1, 2}, {9, 1, 2}), QString());
    QCOMPARE(moves({1, 2, 3, 4}, {4, 1, 3}), QStringLiteral("4:2>0"));
    QCOMPARE(Said(Diff(TabsOfIds({1, 2, 3}), TabsOfIds({3, 9, 1}), true, true, Sight::All())),
             QStringLiteral("tabs.onRemoved(2,isWindowClosing+windowId) tabs.onMoved(1,fromIndex+toIndex+windowId) tabs.onCreated(#9@1)"));
}

void tst_extensionhostwire::aLoadIsToldAsItBeginsAndAsItEnds(){
    using namespace ExtensionHostWire;
    auto tab = [](qint64 loads, bool loading, const char *url = "https://a.example/", const char *title = "a"){
        Tab t = T(7, url, title); t.loads = loads; t.loading = loading; return t;
    };
    auto updates = [](const Tab &then, const Tab &now, const Sight &sight = Sight::All()){
        return UpdatesOf(Diff(QList<Tab>() << then, QList<Tab>() << now, true, true, sight));
    };
    QCOMPARE(updates(tab(0, false), tab(5, true)), QStringLiteral("status=loading[loading]"));
    QCOMPARE(updates(tab(3, false), tab(5, false)), QStringLiteral("status=loading[loading] status=complete[complete]"));
    QCOMPARE(updates(tab(3, false), tab(5, false, "https://b.example/", "b")),
             QStringLiteral("status=loading+url[loading] status=complete+title[complete]"));
    QCOMPARE(updates(tab(5, true), tab(5, false)), QStringLiteral("status=complete[complete]"));
    QCOMPARE(updates(tab(5, true), tab(6, true, "https://b.example/", "a")), QStringLiteral("url[loading]"));
    QCOMPARE(updates(tab(5, true), tab(6, true)), QString());
    QCOMPARE(updates(tab(5, true), tab(6, false, "https://b.example/", "a")), QStringLiteral("status=complete+url[complete]"));
    const Sight narrow = Sight::Of(false, QString(), QStringList() << QStringLiteral("https://z.example/*"));
    QCOMPARE(updates(tab(3, false), tab(5, false, "https://b.example/", "b"), narrow),
             QStringLiteral("status=loading[loading] status=complete[complete]"));
    Tab gone = tab(0, false); gone.discarded = true;
    QCOMPARE(updates(tab(5, false), gone), QStringLiteral("discarded+status=unloaded[unloaded]"));
    Tab back = tab(9, true); back.discarded = false;
    QCOMPARE(updates(gone, back), QStringLiteral("status=loading[loading] discarded[loading]"));
    QCOMPARE(updates(tab(5, false), tab(5, false)), QString());
    const QJsonObject asked = Answer(ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"status\":\"loading\"}]}"))),
                                     QList<Tab>() << tab(5, true) << T(8), Sight::All());
    QCOMPARE(asked.value(QStringLiteral("value")).toArray().size(), 1);
    QCOMPARE(asked.value(QStringLiteral("value")).toArray().at(0).toObject().value(QStringLiteral("status")).toString(), QStringLiteral("loading"));
}

void tst_extensionhostwire::aZoomAndAHighlightAreToldInChromesShape(){
    using namespace ExtensionHostWire;
    auto zoomed = [](double then, double now){
        Tab a = T(7); a.zoom = then;
        Tab b = T(7); b.zoom = now;
        return Diff(QList<Tab>() << a, QList<Tab>() << b, true, true, Sight::All());
    };
    const QJsonArray events = zoomed(1.0, 1.25);
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.at(0).toObject().value(QStringLiteral("name")).toString(), QStringLiteral("tabs.onZoomChange"));
    QCOMPARE(QString::fromUtf8(QJsonDocument(events.at(0).toObject().value(QStringLiteral("args")).toArray().at(0).toObject()).toJson(QJsonDocument::Compact)),
             QStringLiteral("{\"newZoomFactor\":1.25,\"oldZoomFactor\":1,\"tabId\":7,\"zoomSettings\":{\"mode\":\"automatic\",\"scope\":\"per-tab\"}}"));
    QVERIFY(zoomed(0, 1.0).isEmpty());
    QVERIFY(zoomed(1.1, 1.0999).isEmpty());
    QCOMPARE(zoomed(0, 0.5).size(), 1);
    Tab a = T(7); Tab b = T(7, "https://b.example/"); b.zoom = 2;
    QCOMPARE(Said(Diff(QList<Tab>() << a, QList<Tab>() << b, true, true, Sight::All())),
             QStringLiteral("tabs.onUpdated(7,url,#7@0) tabs.onZoomChange(tab 7)"));
    const QJsonArray activated = Diff(QList<Tab>() << T(1, "https://a.example/", "a", true) << T(2),
                                      QList<Tab>() << T(1) << T(2, "https://a.example/", "a", true), true, true, Sight::All());
    QCOMPARE(activated.size(), 2);
    QCOMPARE(activated.at(1).toObject().value(QStringLiteral("name")).toString(), QStringLiteral("tabs.onHighlighted"));
    QCOMPARE(QString::fromUtf8(QJsonDocument(activated.at(1).toObject().value(QStringLiteral("args")).toArray().at(0).toObject()).toJson(QJsonDocument::Compact)),
             QStringLiteral("{\"tabIds\":[2],\"windowId\":1}"));
    QCOMPARE(Said(Greeting(QList<Tab>() << T(2, "https://a.example/", "a", true))), QStringLiteral("tabs.onActivated(tab 2)"));
}

void tst_extensionhostwire::theFontsAreToldAsChromeNamesThem(){
    using namespace ExtensionHostWire;
    auto ask = [](const QString &args){
        return FontList(ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"fontSettings.getFontList\",\"args\":%1}").arg(args))),
                        QStringList() << QStringLiteral("Meiryo") << QStringLiteral("Nimbus Sans [URW]") << QStringLiteral("Arial")
                                      << QStringLiteral("Nimbus Sans [Adobe]") << QStringLiteral("Nimbus Sans") << QString());
    };
    const QJsonObject answer = ask(QStringLiteral("[]"));
    QCOMPARE(answer[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(QString::fromUtf8(QJsonDocument(answer[QStringLiteral("value")].toArray()).toJson(QJsonDocument::Compact)),
             QStringLiteral("[{\"displayName\":\"Meiryo\",\"fontId\":\"Meiryo\"},{\"displayName\":\"Nimbus Sans\",\"fontId\":\"Nimbus Sans\"},"
                            "{\"displayName\":\"Arial\",\"fontId\":\"Arial\"}]"));
    QCOMPARE(ask(QStringLiteral("[null]"))[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(ask(QStringLiteral("[1]"))[QStringLiteral("ok")].toBool(true), false);
    QCOMPARE(ask(QStringLiteral("[{}]"))[QStringLiteral("ok")].toBool(true), false);
    QCOMPARE(FontList(ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"fontSettings.getFontList\",\"args\":[]}"))), QStringList())[QStringLiteral("value")].toArray().size(), 0);
}

void tst_extensionhostwire::theTopSitesAreTheAddressesLookedAtLast(){
    using namespace ExtensionHostWire;
    auto leaf = [](qint64 id, const char *url, const char *title, qint64 visited, qint64 added = 0){
        Tab tab; tab.id = id; tab.url = QUrl(QString::fromLatin1(url)); tab.title = QString::fromLatin1(title);
        tab.visited = visited; tab.added = added; return tab;
    };
    auto ask = [](const QString &args, const QList<Tab> &tabs){
        return TopSites(ParseCall(QUrl::toPercentEncoding(QStringLiteral("{\"api\":\"topSites.get\",\"args\":%1}").arg(args))), tabs);
    };
    auto told = [](const QJsonObject &answer){
        return QString::fromUtf8(QJsonDocument(answer[QStringLiteral("value")].toArray()).toJson(QJsonDocument::Compact));
    };
    const QList<Tab> tabs = QList<Tab>()
        << leaf(1, "https://a.example/", "a old", 100)
        << leaf(2, "https://b.example/", "b", 300)
        << leaf(3, "https://a.example/", "a new", 500)
        << leaf(4, "file:///C:/x.html", "file", 900)
        << leaf(5, "about:blank", "blank", 900)
        << leaf(6, "chrome-extension://abcdefghijklmnopabcdefghijklmnop/page.html", "extension", 900)
        << leaf(7, "", "not loaded", 900)
        << leaf(8, "http://c.example/", "c made", 0, 200)
        << leaf(9, "https://b.example/", "b older", 50)
        << leaf(10, "https://d.example/", "d", 300);
    const QJsonObject answer = ask(QStringLiteral("[]"), tabs);
    QCOMPARE(answer[QStringLiteral("ok")].toBool(), true);
    QCOMPARE(told(answer), QStringLiteral("[{\"title\":\"a new\",\"url\":\"https://a.example/\"},{\"title\":\"b\",\"url\":\"https://b.example/\"},"
                                         "{\"title\":\"d\",\"url\":\"https://d.example/\"},{\"title\":\"c made\",\"url\":\"http://c.example/\"}]"));
    QCOMPARE(told(ask(QStringLiteral("[null]"), tabs)), told(answer));
    QCOMPARE(told(ask(QStringLiteral("[]"), QList<Tab>())), QStringLiteral("[]"));
    QCOMPARE(ask(QStringLiteral("[1]"), tabs)[QStringLiteral("ok")].toBool(true), false);
    QCOMPARE(ask(QStringLiteral("[{}]"), tabs)[QStringLiteral("ok")].toBool(true), false);

    QList<Tab> many;
    for(int i = 1; i <= 12; i++) many << leaf(i, qPrintable(QStringLiteral("https://s%1.example/").arg(i)), "s", i * 10);
    const QJsonArray ten = ask(QStringLiteral("[]"), many)[QStringLiteral("value")].toArray();
    QCOMPARE(ten.size(), 10);
    QCOMPARE(ten.first().toObject()[QStringLiteral("url")].toString(), QStringLiteral("https://s12.example/"));
    QCOMPARE(ten.last().toObject()[QStringLiteral("url")].toString(), QStringLiteral("https://s3.example/"));
}

void tst_extensionhostwire::whatIsPrunedIsLetGoOfAfterTheWalk(){
    using namespace ExtensionHostWire;
    struct Item { std::shared_ptr<int> hold; int n; };
    OneAtATime<Item> acts(8);
    int looked = 0;
    QList<int> lookedWhenLetGo;
    QList<int> countWhenLetGo;
    auto holder = [&](int n){
        return Item{ std::shared_ptr<int>(new int(n), [&](int *p){
            lookedWhenLetGo << looked;
            countWhenLetGo << acts.Count();
            acts.Prune([](const Item &){ return true; });
            delete p;
        }), n };
    };
    for(int i = 1; i <= 4; i++) acts.Push(holder(i));
    acts.Prune([&](const Item &item){ looked++; return item.n % 2 == 0; });
    QCOMPARE(acts.Count(), 2);
    QCOMPARE(lookedWhenLetGo, QList<int>() << 4 << 4);
    QCOMPARE(countWhenLetGo, QList<int>() << 2 << 2);
}

void tst_extensionhostwire::aHeldCallLetGoOfByItsRelayIsAnsweredAbortedAndTheSubscriptionStays(){
    using namespace ExtensionHostWire;
    typedef Subscribers<int> Table;
    Table table;
    auto alive = [](int waiter){ return waiter > 0; };
    const QByteArray token(32, 'a');
    int old = 0;
    QCOMPARE(table.Take(QStringLiteral("ext"), token, 7, 1000, alive, &old), Table::Held);
    QList<Tab> tabs; Tab tab; tab.id = 1; tab.active = true; tabs << tab;
    QList<QPair<int, QJsonObject> > answered;
    table.Publish(tabs, true, 1001, alive, [](const QString&){ return ExtensionHostWire::Sight::All(); },
                  [&](int waiter, const QJsonObject &reply){ answered.append(qMakePair(waiter, reply)); });
    QCOMPARE(answered.size(), 1);
    QCOMPARE(table.Take(QStringLiteral("ext"), token, 8, 1002, alive, &old), Table::Held);
    QVERIFY(table.AnyWaiting(alive));

    int waiter = 0;
    QVERIFY(!table.Abort(QStringLiteral("ext"), QByteArray(32, 'b'), &waiter));
    QVERIFY(!table.Abort(QStringLiteral("other"), token, &waiter));
    QCOMPARE(waiter, 0);
    QVERIFY(table.Abort(QStringLiteral("ext"), token, &waiter));
    QCOMPARE(waiter, 8);
    QVERIFY(!table.AnyWaiting(alive));
    QCOMPARE(table.Count(), 1);
    QVERIFY(!table.Abort(QStringLiteral("ext"), token, &waiter));
    {
        Table aged = table;
        aged.Prune(alive, 1002 + SUBSCRIBER_IDLE_MS + 1);
        QCOMPARE(aged.Count(), 0);
    }
    QCOMPARE(table.Take(QStringLiteral("ext"), token, 9, 1005, alive, &old), Table::Held);
    answered.clear();
    table.Publish(tabs, true, 1006, alive, [](const QString&){ return ExtensionHostWire::Sight::All(); },
                  [&](int waiter, const QJsonObject &reply){ answered.append(qMakePair(waiter, reply)); });
    QVERIFY(answered.isEmpty());
    Tab second; second.id = 2; tabs << second;
    table.Publish(tabs, true, 1007, alive, [](const QString&){ return ExtensionHostWire::Sight::All(); },
                  [&](int waiter, const QJsonObject &reply){ answered.append(qMakePair(waiter, reply)); });
    QCOMPARE(answered.size(), 1);
    QCOMPARE(answered.first().first, 9);
    QCOMPARE(answered.first().second.value(QStringLiteral("value")).toObject().value(QStringLiteral("events")).toArray().size(), 1);

    Call call; call.api = QStringLiteral("vanilla.abort"); call.args = QJsonArray() << QString::fromLatin1(token);
    QCOMPARE(TokenOfAbort(call), token);
    Call events; events.api = QStringLiteral("vanilla.events"); events.args = call.args;
    QVERIFY(TokenOfAbort(events).isEmpty());
    Call bad; bad.api = QStringLiteral("vanilla.abort"); bad.args = QJsonArray() << QStringLiteral("no");
    QVERIFY(TokenOfAbort(bad).isEmpty());
    const QJsonObject aborted = AbortedAnswer();
    QVERIFY(aborted.value(QStringLiteral("ok")).toBool());
    QVERIFY(aborted.value(QStringLiteral("value")).toObject().value(QStringLiteral("aborted")).toBool());
    QVERIFY(!aborted.value(QStringLiteral("value")).toObject().contains(QStringLiteral("stale")));
}

void tst_extensionhostwire::aMenuChoiceIsTakenOnceByItsOwnPage(){
    using namespace ExtensionHostWire;
    const QString a = QStringLiteral("http://a.example/"), b = QStringLiteral("http://b.example/");
    MenuPicks picks;
    picks.Push(7, a, 1000);
    picks.Push(9, b, 1100);
    picks.Push(11, a, 1200);
    QCOMPARE(picks.Take(b, 1300), qint64(9));
    QCOMPARE(picks.Take(b, 1300), qint64(0));
    QCOMPARE(picks.Take(a, 1300), qint64(7));
    QCOMPARE(picks.Take(a, 1300), qint64(11));
    QCOMPARE(picks.Take(a, 1300), qint64(0));
    QCOMPARE(picks.Count(), 0);
    picks.Push(0, a, 1400);
    picks.Push(-3, a, 1400);
    picks.Push(5, QString(), 1400);
    QCOMPARE(picks.Count(), 0);
    picks.Push(5, a, 1400);
    QCOMPARE(picks.Take(QString(), 1400), qint64(0));
    QCOMPARE(picks.Count(), 1);
    QCOMPARE(picks.Take(a + QStringLiteral("#x"), 1400), qint64(0));
    QCOMPARE(picks.Take(a, 1400), qint64(5));
}

void tst_extensionhostwire::aMenuChoiceGoesStaleAndTheOldestGoesWhenTheRoomRunsOut(){
    using namespace ExtensionHostWire;
    const QString a = QStringLiteral("http://a.example/");
    MenuPicks picks;
    picks.Push(3, a, 0);
    QCOMPARE(picks.Take(a, MenuPicks::FRESH), qint64(3));
    picks.Push(4, a, 0);
    QCOMPARE(picks.Take(a, MenuPicks::FRESH + 1), qint64(0));
    QCOMPARE(picks.Count(), 0);
    picks.Push(4, a, 0);
    picks.Push(6, QStringLiteral("http://b.example/"), 5000);
    QCOMPARE(picks.Take(QStringLiteral("http://c.example/"), MenuPicks::FRESH + 1), qint64(0));
    QCOMPARE(picks.Count(), 1);
    MenuPicks full;
    for(int i = 1; i <= MenuPicks::KEPT + 1; i++) full.Push(i, a, 100);
    QCOMPARE(full.Count(), int(MenuPicks::KEPT));
    QCOMPARE(full.Take(a, 100), qint64(2));
}

void tst_extensionhostwire::aLoginIsReadForItsShape_data(){
    using namespace ExtensionHostWire;
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<QString>("error");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("interactive");
    QTest::addColumn<bool>("abortOnLoad");
    QTest::addColumn<int>("timeoutMs");
    const QString none, notLoaded = QString::fromLatin1(AUTH_NOT_LOADED);
    const QString shape = QStringLiteral("identity.launchWebAuthFlow takes the details of a flow");
    QTest::newRow("the defaults") << QByteArray("[{\"url\":\"https://a.example/login?x=1\"}]") << none << "https://a.example/login?x=1" << false << true << 60000;
    QTest::newRow("as DeepL, nobody seeing it") << QByteArray("[{\"url\":\"https://a.example/\",\"interactive\":false,\"abortOnLoadForNonInteractive\":false,\"timeoutMsForNonInteractive\":30000}]")
                                               << none << "https://a.example/" << false << false << 30000;
    QTest::newRow("interactive") << QByteArray("[{\"url\":\"http://a.example/\",\"interactive\":true}]") << none << "http://a.example/" << true << true << 60000;
    QTest::newRow("no booleans") << QByteArray("[{\"url\":\"https://a.example/\",\"interactive\":\"yes\",\"abortOnLoadForNonInteractive\":0}]") << none << "https://a.example/" << false << true << 60000;
    QTest::newRow("more than a minute") << QByteArray("[{\"url\":\"https://a.example/\",\"timeoutMsForNonInteractive\":600000}]") << none << "https://a.example/" << false << true << 60000;
    QTest::newRow("a fraction") << QByteArray("[{\"url\":\"https://a.example/\",\"timeoutMsForNonInteractive\":1500.7}]") << none << "https://a.example/" << false << true << 1500;
    QTest::newRow("less than nothing") << QByteArray("[{\"url\":\"https://a.example/\",\"timeoutMsForNonInteractive\":-5}]") << none << "https://a.example/" << false << true << 0;
    QTest::newRow("NaN, which comes as null") << QByteArray("[{\"url\":\"https://a.example/\",\"timeoutMsForNonInteractive\":null}]") << none << "https://a.example/" << false << true << 0;
    QTest::newRow("a string of a number") << QByteArray("[{\"url\":\"https://a.example/\",\"timeoutMsForNonInteractive\":\"5000\"}]") << none << "https://a.example/" << false << true << 0;
    QTest::newRow("javascript") << QByteArray("[{\"url\":\"javascript:alert(1)\"}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("the extension's own") << QByteArray("[{\"url\":\"chrome-extension://abcdefghijklmnopabcdefghijklmnop/a.html\"}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("a file") << QByteArray("[{\"url\":\"file:///C:/a.html\"}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("data") << QByteArray("[{\"url\":\"data:text/html,x\"}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("no host") << QByteArray("[{\"url\":\"https:///a\"}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("not an address") << QByteArray("[{\"url\":\"a b\"}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("no url") << QByteArray("[{\"interactive\":true}]") << notLoaded << "" << false << true << 60000;
    QTest::newRow("no details") << QByteArray("[]") << shape << "" << false << true << 60000;
    QTest::newRow("details which are no object") << QByteArray("[\"https://a.example/\"]") << shape << "" << false << true << 60000;
    QTest::newRow("two") << QByteArray("[{\"url\":\"https://a.example/\"},{}]") << shape << "" << false << true << 60000;
}

void tst_extensionhostwire::aLoginIsReadForItsShape(){
    using namespace ExtensionHostWire;
    QFETCH(QByteArray, json);
    QFETCH(QString, error);
    QFETCH(QString, url);
    QFETCH(bool, interactive);
    QFETCH(bool, abortOnLoad);
    QFETCH(int, timeoutMs);
    const AuthFlow flow = ParseAuthFlow(ParseCall(("{\"api\":\"identity.launchWebAuthFlow\",\"args\":" + json + "}").toPercentEncoding()));
    QCOMPARE(flow.error, error);
    if(!error.isEmpty()) return;
    QCOMPARE(flow.url.toString(QUrl::FullyEncoded), url);
    QCOMPARE(flow.interactive, interactive);
    QCOMPARE(flow.abortOnLoad, abortOnLoad);
    QCOMPARE(flow.timeoutMs, timeoutMs);
}

void tst_extensionhostwire::aLoginIsSentBackToItsOwnAddressOnly(){
    using namespace ExtensionHostWire;
    const QString id = QStringLiteral("abcdefghijklmnopabcdefghijklmnop");
    QVERIFY(IsAuthRedirect(id, QUrl(QStringLiteral("https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/cb?code=1"))));
    QVERIFY(IsAuthRedirect(id, QUrl(QStringLiteral("https://ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP.chromiumapp.org/"))));
    QVERIFY(IsAuthRedirect(id, QUrl(QStringLiteral("https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org:8443/"))));
    QVERIFY(!IsAuthRedirect(id, QUrl(QStringLiteral("http://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/"))));
    QVERIFY(!IsAuthRedirect(id, QUrl(QStringLiteral("https://bbcdefghijklmnopabcdefghijklmnop.chromiumapp.org/"))));
    QVERIFY(!IsAuthRedirect(id, QUrl(QStringLiteral("https://x.abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/"))));
    QVERIFY(!IsAuthRedirect(id, QUrl(QStringLiteral("https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org.a.example/"))));
    QVERIFY(!IsAuthRedirect(id, QUrl(QStringLiteral("https://a.example/abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/"))));
    QVERIFY(!IsAuthRedirect(QString(), QUrl(QStringLiteral("https://.chromiumapp.org/"))));
    QVERIFY(!ParseAuthFlow(ParseCall(QByteArray("{\"api\":\"tabs.query\",\"args\":[{\"url\":\"https://a.example/\"}]}").toPercentEncoding())).error.isEmpty());
}

void tst_extensionhostwire::aCaptureIsReadForItsShape(){
    using namespace ExtensionHostWire;
    auto parse = [](const QByteArray &args){
        return ParseCapture(ParseCall(("{\"api\":\"tabs.captureVisibleTab\",\"args\":" + args + "}").toPercentEncoding()));
    };
    auto shape = [&](const QByteArray &args){
        const Capture c = parse(args);
        return c.error.isEmpty() ? QString::fromLatin1(c.format) + QLatin1Char(' ') + QString::number(c.quality) : QStringLiteral("error");
    };
    QCOMPARE(shape("[]"), QStringLiteral("jpeg 92"));
    QCOMPARE(shape("[null]"), QStringLiteral("jpeg 92"));
    QCOMPARE(shape("[1]"), QStringLiteral("jpeg 92"));
    QCOMPARE(shape("[{\"format\":\"png\"}]"), QStringLiteral("png 92"));
    QCOMPARE(shape("[null,{\"format\":\"jpeg\",\"quality\":40}]"), QStringLiteral("jpeg 40"));
    QCOMPARE(shape("[-2,{\"quality\":0}]"), QStringLiteral("jpeg 0"));
    QCOMPARE(shape("[{\"quality\":100}]"), QStringLiteral("jpeg 100"));
    QCOMPARE(shape("[{\"format\":\"gif\"}]"), QStringLiteral("error"));
    QCOMPARE(shape("[{\"quality\":101}]"), QStringLiteral("error"));
    QCOMPARE(shape("[{\"quality\":-1}]"), QStringLiteral("error"));
    QCOMPARE(shape("[{\"quality\":50.5}]"), QStringLiteral("error"));
    QCOMPARE(shape("[{\"quality\":\"50\"}]"), QStringLiteral("error"));
    QCOMPARE(shape("[\"1\"]"), QStringLiteral("error"));
    QCOMPARE(shape("[\"1\",{}]"), QStringLiteral("error"));
    QCOMPARE(shape("[1,2]"), QStringLiteral("error"));
    QCOMPARE(shape("[1,{},3]"), QStringLiteral("error"));
    QVERIFY(parse("[{\"format\":\"gif\"}]").error.startsWith(QStringLiteral("Error in invocation of tabs.captureVisibleTab(")));
}

void tst_extensionhostwire::aCaptureIsOfWhatThePermissionReaches(){
    using namespace ExtensionHostWire;
    const QString self = QStringLiteral("abcdefghijklmnopabcdefghijklmnop");
    const QString none = QStringLiteral("Either the '<all_urls>' or 'activeTab' permission is required.");
    const QString page = QStringLiteral("Cannot access contents of the page. Extension manifest must request permission to access the respective host.");
    const QStringList all = QStringList() << QStringLiteral("<all_urls>");
    const QStringList web = QStringList() << QStringLiteral("*://*/*");
    const QStringList some = QStringList() << QStringLiteral("*://*.deepl.com/*");
    auto refused = [&](const QString &url, const QStringList &hosts, bool granted){
        return CaptureRefusal(QUrl(url), self, hosts, granted);
    };
    QCOMPARE(refused(QStringLiteral("https://a.example/"), some, false), none);
    QCOMPARE(refused(QStringLiteral("https://www.deepl.com/"), some, false), none);
    QCOMPARE(refused(QStringLiteral("https://a.example/"), QStringList(), false), none);
    for(const QStringList &hosts : { all, web, some }){
        QCOMPARE(refused(QStringLiteral("https://a.example/"), hosts, hosts == some), QString());
        QCOMPARE(refused(QStringLiteral("http://a.example/"), hosts, hosts == some), QString());
    }
    QCOMPARE(refused(QStringLiteral("data:text/html,x"), all, false), QString());
    QCOMPARE(refused(QStringLiteral("data:text/html,x"), some, true), QString());
    QCOMPARE(refused(QStringLiteral("data:text/html,x"), web, false), page);
    QCOMPARE(refused(QStringLiteral("chrome-extension://") + self + QStringLiteral("/popup.html"), all, false), QString());
    QCOMPARE(refused(QStringLiteral("chrome-extension://") + self + QStringLiteral("/popup.html"), web, false), page);
    QCOMPARE(refused(QStringLiteral("chrome-extension://bbcdefghijklmnopabcdefghijklmnop/popup.html"), all, true), page);
    QCOMPARE(refused(QStringLiteral("file:///C:/a.html"), all, true), page);
    QCOMPARE(refused(QStringLiteral("vanilla://settings/"), all, true), page);
    QCOMPARE(refused(QStringLiteral("view-source:https://a.example/"), all, true), page);
    QCOMPARE(refused(QStringLiteral("about:blank"), all, true), page);
}

void tst_extensionhostwire::aSidePanelButtonIsThisViewsAndThisWindows(){
    auto read = [](const QString &name){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) + QLatin1Char('/') + name);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r')) : QString();
    };
    auto body = [](const QString &source, const QString &head){
        const int at = source.indexOf(head);
        return at < 0 ? QString() : source.mid(at, source.indexOf(QStringLiteral("\n}\n"), at) - at);
    };
    const QString bar = body(read(QStringLiteral("ui/extensionbar.cpp")), QStringLiteral("bool ExtensionBar::PanelInstead("));
    QVERIFY(bar.contains(QStringLiteral("!view->MakesSidePanels()) return false;")));
    QVERIFY(bar.contains(QStringLiteral("window->GetSidePanels()->Toggle(")));
    QVERIFY(!bar.contains(QStringLiteral("SidePanels::Toggle(")));
    const QString toggle = body(read(QStringLiteral("ui/sidepanels.cpp")), QStringLiteral("QString SidePanels::Toggle("));
    QVERIFY(!toggle.isEmpty() && !toggle.contains(QStringLiteral("All()")));
    QVERIFY(read(QStringLiteral("view/webengine/webengineview.hpp")).contains(QStringLiteral("bool MakesSidePanels() const Q_DECL_OVERRIDE { return true; }")));
    QVERIFY(!read(QStringLiteral("view/webengine/quickwebengineview.hpp")).contains(QStringLiteral("MakesSidePanels")));
    QVERIFY(read(QStringLiteral("view/edge/edgewebview.hpp")).contains(QStringLiteral("bool MakesSidePanels() const Q_DECL_OVERRIDE { return true; }")));
    const QString edge = read(QStringLiteral("view/edge/edgeextensions.cpp"));
    const int hide = edge.indexOf(QStringLiteral("        void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE {"));
    const QString hidden = edge.mid(hide, edge.indexOf(QStringLiteral("\n        }\n"), hide) - hide);
    QVERIFY(hidden.contains(QStringLiteral("if(m_Docked){")) && hidden.contains(QStringLiteral("controller->put_IsVisible(FALSE);")));
    QVERIFY(hidden.contains(QStringLiteral("} else {\n                Shutdown();")));
    QVERIFY(edge.contains(QStringLiteral("if(self->m_Docked) return S_OK;")));
    QVERIFY(edge.contains(QStringLiteral("if(m_Docked && ev->type() == View::SidePanelShutdownEvent()){")));
    const QString drop = body(read(QStringLiteral("ui/sidepanels.cpp")), QStringLiteral("void SidePanels::Drop("));
    QVERIFY(drop.indexOf(QStringLiteral("postEvent(panel.page, new QEvent(View::SidePanelShutdownEvent()));")) >= 0);
    QVERIFY(drop.indexOf(QStringLiteral("postEvent(")) < drop.indexOf(QStringLiteral("deleteLater()")));
    QVERIFY(read(QStringLiteral("ui/mainwindow.cpp")).contains(QStringLiteral("if(m_SidePanels) m_SidePanels->ShutdownPages();")));
    const QString app = read(QStringLiteral("app/application.cpp"));
    QVERIFY(app.indexOf(QStringLiteral("SidePanels::ShutdownEverywhere();")) >= 0 &&
            app.indexOf(QStringLiteral("SidePanels::ShutdownEverywhere();")) < app.indexOf(QStringLiteral("TreeBank::ReleaseAllView();")));

    const QString panels = read(QStringLiteral("ui/sidepanels.cpp"));
    const QString update = body(panels, QStringLiteral("void SidePanels::Update("));
    QVERIFY(!update.isEmpty() && !update.contains(QStringLiteral("CreateExtensionView")));
    QVERIFY(update.contains(QStringLiteral("!options.TabPath(tab).isEmpty() || !options.EnabledFor(tab) ||")));
    QVERIFY(update.contains(QStringLiteral("(own >= 0 && m_Panels.at(own).id == m_Panels.at(all).id)) all = -1;")));
    QVERIFY(update.contains(QStringLiteral("Show(m_Dock, m_Stack, all >= 0 ? &m_Panels.at(all) : nullptr);")));
    QVERIFY(update.contains(QStringLiteral("Show(m_TabDock, m_TabStack, own >= 0 ? &m_Panels.at(own) : nullptr);")));
    QVERIFY(panels.contains(QStringLiteral("window->tabifyDockWidget(m_Dock, m_TabDock);")));
    QCOMPARE(panels.count(QStringLiteral("CreateExtensionView(")), 1);
    QVERIFY(body(panels, QStringLiteral("QString SidePanels::OpenHere(")).contains(QStringLiteral("CreateExtensionView(")));
    const QString prune = body(panels, QStringLiteral("void SidePanels::Prune("));
    QVERIFY(prune.contains(QStringLiteral(": options.Enabled())")));
    QVERIFY(prune.contains(QStringLiteral("                && AnyViewOf(TreeBank::GetViewRoot(), panel.controller);")));
    const QString open = body(panels, QStringLiteral("QString SidePanels::Open("));
    QVERIFY(open.contains(QStringLiteral("if(one == target) continue;")));
    const QString close = body(panels, QStringLiteral("QString SidePanels::Close("));
    QVERIFY(close.contains(QStringLiteral("if(closed) return QString();")));
    QVERIFY(!prune.contains(QStringLiteral("EnabledFor(tab)")));
    QVERIFY(prune.contains(QStringLiteral("if(path != panel.path) moved << qMakePair(")));
    QVERIFY(!prune.contains(QStringLiteral("sendEvent(panel.page")));
    QVERIFY(body(panels, QStringLiteral("void SidePanels::ShutdownPages(")).contains(QStringLiteral("QList<QPointer<QWidget> > pages;")));
    const QString host = read(QStringLiteral("app/extensionhost.cpp"));
    QVERIFY(!host.contains(QStringLiteral("ask->Reply(SidePanelCall(asked, id));")));
}

void tst_extensionhostwire::anActiveTabIsGrantedForOneLoad(){
    using namespace ExtensionHostWire;
    ActiveTabs tabs;
    const QString a = QStringLiteral("a"), b = QStringLiteral("b");
    QVERIFY(!tabs.Granted(a, 7, 3));
    tabs.Grant(a, 7, 3);
    QVERIFY(tabs.Granted(a, 7, 3));
    QVERIFY(!tabs.Granted(a, 7, 4));
    QVERIFY(!tabs.Granted(a, 8, 3));
    QVERIFY(!tabs.Granted(b, 7, 3));
    tabs.Grant(a, 7, 5);
    QVERIFY(!tabs.Granted(a, 7, 3));
    QVERIFY(tabs.Granted(a, 7, 5));
    tabs.Grant(a, 9, 0);
    QVERIFY(!tabs.Granted(a, 9, 0));
    ActiveTabs zero;
    zero.Grant(a, 9, 0);
    QVERIFY(zero.Ids().isEmpty());
    tabs.Grant(a, 0, 5);
    tabs.Grant(QString(), 7, 5);
    QCOMPARE(tabs.Ids(), QStringList() << a);
    for(int i = 0; i < ActiveTabs::KEPT; i++) tabs.Grant(a, 100 + i, 1);
    QVERIFY(!tabs.Granted(a, 7, 5));
    QVERIFY(tabs.Granted(a, 100, 1));
    QVERIFY(tabs.Granted(a, 100 + ActiveTabs::KEPT - 1, 1));
    tabs.Grant(b, 7, 5);
    tabs.Forget(a);
    QVERIFY(!tabs.Granted(a, 100, 1));
    QVERIFY(tabs.Granted(b, 7, 5));

    QVERIFY(CaptureAllowedAt(0, 1000));
    QVERIFY(CaptureAllowedAt(1000, 1500));
    QVERIFY(!CaptureAllowedAt(1000, 1499));
    QVERIFY(!CaptureAllowedAt(1000, 1000));
    QVERIFY(CaptureAllowedAt(1000, 999));
}

void tst_extensionhostwire::anActiveTabIsGrantedByTheUserAlone(){
    auto read = [](const QString &name){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) + QLatin1Char('/') + name);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r')) : QString();
    };
    auto body = [](const QString &source, const QString &head){
        const int at = source.indexOf(head);
        if(at < 0) return QString();
        const int end = source.indexOf(QStringLiteral("\n}\n"), at);
        return source.mid(at, end - at);
    };
    const QString grant = QStringLiteral("ExtensionHost::Invoked(");
    const QString bar = read(QStringLiteral("ui/extensionbar.cpp"));
    const QString controller = read(QStringLiteral("app/extensioncontroller.cpp"));
    const QString host = read(QStringLiteral("app/extensionhost.cpp"));
    QVERIFY(!bar.isEmpty() && !controller.isEmpty() && !host.isEmpty());
    QCOMPARE(bar.count(grant), 1);
    QVERIFY(body(bar, QStringLiteral("void ExtensionBar::Pressed(")).contains(grant));
    QCOMPARE(bar.count(QStringLiteral("Pressed(path);")), 1);
    QVERIFY(body(bar, QStringLiteral("void ExtensionBar::Press(")).contains(QStringLiteral("Pressed(path);")));
    QCOMPARE(bar.count(QStringLiteral("Press(path, popup);")), 2);
    for(const QString &head : { QStringLiteral("void ExtensionBar::Execute("), QStringLiteral("void ExtensionBar::Open("),
                                QStringLiteral("void ExtensionBar::Click(") }){
        const QString b = body(bar, head);
        QVERIFY2(!b.isEmpty() && !b.contains(grant) && !b.contains(QStringLiteral("Pressed("))
                 && !b.contains(QStringLiteral("Press(")), qPrintable(head));
    }
    QCOMPARE(controller.count(grant), 2);
    QVERIFY(body(controller, QStringLiteral("void ExtensionController::ClickMenu(")).contains(grant));
    QVERIFY(body(controller, QStringLiteral("bool ExtensionController::RunCommand(")).contains(grant));
    QVERIFY(!read(QStringLiteral("app/extensioncontroller.hpp")).contains(grant));
    QCOMPARE(host.count(grant), 1);
    const QString capture = body(host, QStringLiteral("QJsonObject ExtensionHost::CaptureCall("));
    QVERIFY(capture.contains(QStringLiteral("CaptureRefusal(view->CommittedUrl(),")));
    QVERIFY(!capture.contains(QStringLiteral("view->url()")));
    QVERIFY(capture.contains(QStringLiteral("HasPermission(id, QStringLiteral(\"activeTab\"))")));
    QVERIFY(host.contains(QStringLiteral("void ExtensionHost::Invoked(")));
}

void tst_extensionhostwire::aMenuTabCallNamesItsPage(){
    using namespace ExtensionHostWire;
    auto asked = [](const QByteArray &json){ return ParseCall(json.toPercentEncoding()); };
    bool ok = false;
    QCOMPARE(PageUrlOfMenuTab(asked("{\"api\":\"vanilla.menuTab\",\"args\":[\"http://a.example/\"]}"), &ok), QStringLiteral("http://a.example/"));
    QVERIFY(ok);
    QVERIFY(PageUrlOfMenuTab(asked("{\"api\":\"vanilla.menuTab\",\"args\":[\"\"]}"), &ok).isEmpty());
    QVERIFY(ok);
    PageUrlOfMenuTab(asked("{\"api\":\"vanilla.menuTab\",\"args\":[]}"), &ok);
    QVERIFY(!ok);
    PageUrlOfMenuTab(asked("{\"api\":\"vanilla.menuTab\",\"args\":[7]}"), &ok);
    QVERIFY(!ok);
    PageUrlOfMenuTab(asked("{\"api\":\"vanilla.menuTab\",\"args\":[\"http://a.example/\",1]}"), &ok);
    QVERIFY(!ok);
    PageUrlOfMenuTab(asked("{\"api\":\"vanilla.tabOf\",\"args\":[\"http://a.example/\"]}"), &ok);
    QVERIFY(!ok);
}

void tst_extensionhostwire::aZoomCallIsReadAsChromeTakesIt(){
    using namespace ExtensionHostWire;
    auto zoom = [](const QByteArray &api, const QByteArray &args){
        return ParseZoom(ParseCall(("{\"api\":\"" + api + "\",\"args\":" + args + "}").toPercentEncoding()));
    };
    ZoomCall z = zoom("tabs.getZoom", "[12]");
    QVERIFY(z.error.isEmpty());
    QVERIFY(!z.set);
    QCOMPARE(z.tab, qint64(12));
    QCOMPARE(zoom("tabs.getZoom", "[]").tab, qint64(0));
    QVERIFY(zoom("tabs.getZoom", "[]").error.isEmpty());
    QCOMPARE(zoom("tabs.getZoom", "[null]").tab, qint64(0));
    QVERIFY(zoom("tabs.getZoom", "[null]").error.isEmpty());
    QVERIFY(!zoom("tabs.getZoom", "[1.5]").error.isEmpty());
    QVERIFY(!zoom("tabs.getZoom", "[0]").error.isEmpty());
    QVERIFY(!zoom("tabs.getZoom", "[\"12\"]").error.isEmpty());
    QVERIFY(!zoom("tabs.getZoom", "[12,1]").error.isEmpty());

    z = zoom("tabs.setZoom", "[12,1.5]");
    QVERIFY(z.error.isEmpty());
    QVERIFY(z.set);
    QCOMPARE(z.tab, qint64(12));
    QCOMPARE(z.factor, 1.5);
    z = zoom("tabs.setZoom", "[2]");
    QVERIFY(z.error.isEmpty());
    QCOMPARE(z.tab, qint64(0));
    QCOMPARE(z.factor, 2.0);
    QCOMPARE(zoom("tabs.setZoom", "[null,2]").tab, qint64(0));
    QVERIFY(zoom("tabs.setZoom", "[null,2]").error.isEmpty());
    QCOMPARE(zoom("tabs.setZoom", "[12,0]").factor, 1.0);
    QVERIFY(zoom("tabs.setZoom", "[12,0.25]").error.isEmpty());
    QVERIFY(zoom("tabs.setZoom", "[12,5]").error.isEmpty());
    QCOMPARE(zoom("tabs.setZoom", "[12,0.2]").error, QStringLiteral("Zoom value is out of range."));
    QCOMPARE(zoom("tabs.setZoom", "[12,5.5]").error, QStringLiteral("Zoom value is out of range."));
    QCOMPARE(zoom("tabs.setZoom", "[12,-1]").error, QStringLiteral("Zoom value is out of range."));
    QVERIFY(!zoom("tabs.setZoom", "[12,\"2\"]").error.isEmpty());
    QVERIFY(!zoom("tabs.setZoom", "[]").error.isEmpty());
    QVERIFY(!zoom("tabs.setZoom", "[12,2,3]").error.isEmpty());
    QVERIFY(!zoom("tabs.setZoom", "[-3,2]").error.isEmpty());
    QVERIFY(!zoom("tabs.get", "[12]").error.isEmpty());

    const QJsonObject told = ZoomAnswer(1.25);
    QCOMPARE(told.value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(told.value(QStringLiteral("value")).toDouble(), 1.25);
}

void tst_extensionhostwire::aUserScriptWorldOpensWithASecretOfItsOwn(){
    const QByteArray process(32, 'p');
    const QByteArray one = ExtensionHostWire::WorldSecret(process, QStringLiteral("space"), QStringLiteral("ext"), QStringLiteral("C:/copy"), QStringLiteral("w"));
    QCOMPARE(one.size(), 64);
    QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(QString::fromLatin1(one)).hasMatch());
    QCOMPARE(ExtensionHostWire::WorldSecret(process, QStringLiteral("space"), QStringLiteral("ext"), QStringLiteral("C:/copy"), QStringLiteral("w")), one);
    const QList<QByteArray> others = QList<QByteArray>()
        << ExtensionHostWire::WorldSecret(QByteArray(32, 'q'), QStringLiteral("space"), QStringLiteral("ext"), QStringLiteral("C:/copy"), QStringLiteral("w"))
        << ExtensionHostWire::WorldSecret(process, QStringLiteral("other"), QStringLiteral("ext"), QStringLiteral("C:/copy"), QStringLiteral("w"))
        << ExtensionHostWire::WorldSecret(process, QStringLiteral("space"), QStringLiteral("ext2"), QStringLiteral("C:/copy"), QStringLiteral("w"))
        << ExtensionHostWire::WorldSecret(process, QStringLiteral("space"), QStringLiteral("ext"), QStringLiteral("C:/copy2"), QStringLiteral("w"))
        << ExtensionHostWire::WorldSecret(process, QStringLiteral("space"), QStringLiteral("ext"), QStringLiteral("C:/copy"), QString())
        << ExtensionHostWire::WorldSecret(process, QStringLiteral("spac"), QStringLiteral("eext"), QStringLiteral("C:/copy"), QStringLiteral("w"));
    foreach(const QByteArray &other, others) QVERIFY(other != one && !other.isEmpty());
    QVERIFY(ExtensionHostWire::WorldSecret(QByteArray(31, 'p'), QStringLiteral("space"), QStringLiteral("ext"), QStringLiteral("C:/copy"), QStringLiteral("w")).isEmpty());
    QVERIFY(ExtensionHostWire::IsUserScriptMessageUrl(QUrl(QStringLiteral("vanilla-extension://host/userScriptMessage"))));
    QVERIFY(!ExtensionHostWire::IsUserScriptMessageUrl(QUrl(QStringLiteral("vanilla-extension://host/userScriptMessage?x"))));
    QVERIFY(!ExtensionHostWire::IsUserScriptMessageUrl(QUrl(QStringLiteral("vanilla-extension://host/call"))));
    QVERIFY(!ExtensionHostWire::IsUserScriptMessageUrl(QUrl(QStringLiteral("https://host/userScriptMessage"))));
}

void tst_extensionhostwire::aUserScriptsMessageIsAnsweredOnceByWhoeverWasAsked(){
    typedef ExtensionHostWire::UserScriptMessages<int> Messages;
    Messages messages;
    const QSet<QByteArray> two = QSet<QByteArray>() << "w1" << "w2";
    QVERIFY(messages.Add("t1", QStringLiteral("e"), 1, two, 0));
    QVERIFY(messages.Reply(QStringLiteral("other"), "t1", "w1", Messages::Value, QJsonValue(1)).isEmpty());
    QVERIFY(messages.Reply(QStringLiteral("e"), "t1", "w3", Messages::Value, QJsonValue(1)).isEmpty());
    QVERIFY(messages.Reply(QStringLiteral("e"), "t1", "w1", Messages::NoAnswer, QJsonValue()).isEmpty());
    QList<Messages::Done> done = messages.Reply(QStringLiteral("e"), "t1", "w2", Messages::Value, QJsonValue(QStringLiteral("v")));
    QCOMPARE(done.size(), 1);
    QCOMPARE(done.at(0).waiter, 1);
    QCOMPARE(QJsonDocument(done.at(0).answer).toJson(QJsonDocument::Compact), QByteArray("{\"ok\":true,\"value\":\"v\"}"));
    QVERIFY(messages.Reply(QStringLiteral("e"), "t1", "w2", Messages::Value, QJsonValue(2)).isEmpty());
    QCOMPARE(messages.Count(), 0);
    QVERIFY(messages.Add("t2", QStringLiteral("e"), 2, two, 0));
    QVERIFY(messages.Reply(QStringLiteral("e"), "t2", "w1", Messages::NoListener, QJsonValue()).isEmpty());
    done = messages.Reply(QStringLiteral("e"), "t2", "w2", Messages::NoAnswer, QJsonValue());
    QCOMPARE(done.size(), 1);
    QCOMPARE(QJsonDocument(done.at(0).answer).toJson(QJsonDocument::Compact), QByteArray("{\"none\":true,\"ok\":true}"));
    QVERIFY(messages.Add("t3", QStringLiteral("e"), 3, two, 0));
    messages.Reply(QStringLiteral("e"), "t3", "w1", Messages::NoListener, QJsonValue());
    done = messages.Reply(QStringLiteral("e"), "t3", "w2", Messages::NoListener, QJsonValue());
    QCOMPARE(done.size(), 1);
    QCOMPARE(done.at(0).answer.value(QStringLiteral("error")).toString(), ExtensionHostWire::NoReceiver());
    QVERIFY(messages.Add("t4", QStringLiteral("e"), 4, two, 0));
    QVERIFY(messages.Add("t5", QStringLiteral("e"), 5, QSet<QByteArray>() << "w1", 0));
    QVERIFY(messages.Add("t6", QStringLiteral("e"), 6, QSet<QByteArray>() << "w1", 1000));
    done = messages.Sweep(2000, [](const QString &, const QByteArray &token){ return token != "w2"; },
                          [](int waiter){ return waiter != 5; });
    QCOMPARE(done.size(), 0);
    QCOMPARE(messages.Count(), 2);
    done = messages.Reply(QStringLiteral("e"), "t4", "w1", Messages::NoListener, QJsonValue());
    QCOMPARE(done.size(), 1);
    QCOMPARE(QJsonDocument(done.at(0).answer).toJson(QJsonDocument::Compact), QByteArray("{\"none\":true,\"ok\":true}"));
    done = messages.Sweep(1000 + ExtensionHostWire::USER_SCRIPT_WAIT + 1, [](const QString &, const QByteArray &){ return true; },
                          [](int){ return true; });
    QCOMPARE(done.size(), 1);
    QCOMPARE(done.at(0).answer.value(QStringLiteral("error")).toString(), ExtensionHostWire::PortClosed());
    QVERIFY(messages.Add("t7", QStringLiteral("e"), 7, two, 0));
    QVERIFY(messages.Add("t8", QStringLiteral("f"), 8, two, 0));
    done = messages.Drop(QStringLiteral("e"));
    QCOMPARE(done.size(), 1);
    QCOMPARE(done.at(0).waiter, 7);
    QCOMPARE(messages.Extensions(), QStringList() << QStringLiteral("f"));
    for(int i = 1; i < ExtensionHostWire::USER_SCRIPT_PENDING; i++) QVERIFY(messages.Add(QByteArray::number(i), QStringLiteral("f"), i, two, 0));
    QVERIFY(!messages.Add("full", QStringLiteral("f"), 0, two, 0));
    QVERIFY(!messages.Add("nobody", QStringLiteral("g"), 0, QSet<QByteArray>(), 0));
    QCOMPARE(messages.Drop().size(), ExtensionHostWire::USER_SCRIPT_PENDING);

    ExtensionHostWire::Subscribers<int> subscribers;
    auto alive = [](int){ return true; };
    subscribers.Take(QStringLiteral("e"), "s1", 1, 0, alive, nullptr);
    subscribers.Take(QStringLiteral("e"), "s2", 2, 0, alive, nullptr);
    subscribers.Take(QStringLiteral("f"), "s1", 3, 0, alive, nullptr);
    const QSet<QByteArray> told = subscribers.FireTo(QStringLiteral("e"), QSet<QByteArray>() << "s1" << "s3", QJsonObject{ { QStringLiteral("name"), QStringLiteral("x") } }, alive, 0);
    QCOMPARE(told, QSet<QByteArray>() << "s1");
    int queued = 0;
    foreach(const auto &entry, subscribers.Entries()) queued += entry.queued.size();
    QCOMPARE(queued, 1);
    QVERIFY(subscribers.Has(QStringLiteral("e"), "s2"));
    QVERIFY(!subscribers.Has(QStringLiteral("e"), "s3"));
}

void tst_extensionhostwire::aWorkerStartedAgainIsListenedToStill(){
    ExtensionHostWire::UserScriptListeners listeners;
    QSet<QByteArray> there;
    auto has = [&there](const QString &, const QByteArray &token){ return there.contains(token); };
    for(int i = 1; i <= 6; i++){
        const QByteArray token = "w" + QByteArray::number(i);
        listeners.Name(QStringLiteral("e"), token, i * 1000, has);
        there = QSet<QByteArray>() << token;
        listeners.Prune(i * 1000 + 1, has);
        QVERIFY(listeners.Of(QStringLiteral("e")).contains(token));
    }
    QCOMPARE(listeners.Of(QStringLiteral("e")), QSet<QByteArray>() << "w6");
    listeners.Name(QStringLiteral("e"), "new", 10000, has);
    listeners.Prune(10001, has);
    QVERIFY(listeners.Of(QStringLiteral("e")).contains("new"));
    listeners.Prune(10000 + ExtensionHostWire::SUBSCRIBER_IDLE_MS + 1, has);
    QVERIFY(!listeners.Of(QStringLiteral("e")).contains("new"));
    there = QSet<QByteArray>() << "a" << "b" << "c" << "d" << "e1";
    foreach(const QByteArray &token, QList<QByteArray>() << "a" << "b" << "c" << "d") listeners.Name(QStringLiteral("f"), token, 20000, has);
    listeners.Name(QStringLiteral("f"), "e1", 20001, has);
    QVERIFY(listeners.Of(QStringLiteral("f")).contains("e1"));
    QVERIFY(listeners.Of(QStringLiteral("f")).size() <= ExtensionHostWire::SUBSCRIBERS_OF_ONE);
    QVERIFY(listeners.Of(QStringLiteral("g")).isEmpty());

    const QStringList hosts = QStringList() << QStringLiteral("http://127.0.0.1/*") << QStringLiteral("https://*.example/*");
    QVERIFY(ExtensionHostWire::WithinHostPermissions(QUrl(QStringLiteral("http://127.0.0.1:8765/x")), hosts));
    QVERIFY(ExtensionHostWire::WithinHostPermissions(QUrl(QStringLiteral("https://a.example/")), hosts));
    QVERIFY(!ExtensionHostWire::WithinHostPermissions(QUrl(QStringLiteral("https://a.other/")), hosts));
    QVERIFY(!ExtensionHostWire::WithinHostPermissions(QUrl(QStringLiteral("http://a.example/")), hosts));
    QVERIFY(!ExtensionHostWire::WithinHostPermissions(QUrl(QStringLiteral("https://a.example/")), QStringList()));
    QVERIFY(ExtensionHostWire::WithinHostPermissions(QUrl(QStringLiteral("https://any.where/p")), QStringList() << QStringLiteral("<all_urls>")));

    ExtensionHostWire::Subscribers<int> subscribers;
    auto alive = [](int){ return true; };
    subscribers.Take(QStringLiteral("e"), "s1", 1, 0, alive, nullptr);
    const QJsonObject message{ { QStringLiteral("name"), QStringLiteral("runtime.onUserScriptMessage") } };
    QCOMPARE(subscribers.FireTo(QStringLiteral("e"), QSet<QByteArray>() << "s1", message, alive, 0).size(), 1);
    for(int i = 0; i < 2 * ExtensionHostWire::SUBSCRIBER_QUEUE; i++)
        subscribers.Fire(QStringLiteral("e"), QJsonObject{ { QStringLiteral("name"), QStringLiteral("tabs.onUpdated") } });
    const QList<QJsonObject> queued = subscribers.Entries().at(0).queued;
    QCOMPARE(queued.size(), ExtensionHostWire::SUBSCRIBER_QUEUE);
    QCOMPARE(queued.at(0), message);
    QVERIFY(subscribers.FireTo(QStringLiteral("e"), QSet<QByteArray>() << "s1", message, alive, 0).isEmpty());
}

void tst_extensionhostwire::aWorkerWhichSleepsIsWokenForWhatItListensTo(){
    using namespace ExtensionHostWire;
    QSet<int> gone;
    auto alive = [&](int w){ return w != 0 && !gone.contains(w); };
    auto sees = [](const QString &){ return Sight::All(); };
    QHash<int, QStringList> heard;
    auto answer = [&](int w, const QJsonObject &reply){
        foreach(const QJsonValue &e, reply.value(QStringLiteral("value")).toObject().value(QStringLiteral("events")).toArray())
            heard[w] << e.toObject().value(QStringLiteral("name")).toString();
        heard[w] << QStringLiteral("|");
    };
    const QSet<QString> updated = QSet<QString>() << QStringLiteral("tabs.onUpdated");
    const QString e = QStringLiteral("e");
    const QList<Tab> a = QList<Tab>() << T(1, "https://a.example/", "a", true);
    const QList<Tab> b = QList<Tab>() << T(1, "https://b.example/", "b", true);
    const QList<Tab> c = QList<Tab>() << T(1, "https://b.example/", "b", true) << T(2, "https://c.example/", "c");
    Subscribers<int> table;
    QCOMPARE(table.Take(e, "s1", 1, 0, alive, nullptr, updated, true), Subscribers<int>::Held);
    QVERIFY(!table.Watches());
    table.Publish(a, true, 0, alive, sees, answer);
    QCOMPARE(heard.value(1), QStringList() << QStringLiteral("tabs.onActivated") << QStringLiteral("|"));
    QVERIFY(table.Watches());
    QCOMPARE(table.Watched(), QStringList() << e);
    table.Take(e, "s1", 2, 10, alive, nullptr, updated, true);
    QVERIFY(table.Watch(b, true, 20, alive, sees).isEmpty());
    gone << 2;
    QVERIFY(table.Watch(a, true, 30, alive, sees).isEmpty());
    QCOMPARE(table.Watch(b, true, 40, alive, sees), QStringList() << e);
    QCOMPARE(table.Watch(b, true, 50, alive, sees), QStringList() << e);
    QCOMPARE(table.Take(e, "s2", 3, 60, alive, nullptr, updated, true), Subscribers<int>::Held);
    table.Publish(b, true, 60, alive, sees, answer);
    QCOMPARE(heard.value(3), QStringList() << QStringLiteral("tabs.onUpdated") << QStringLiteral("|"));
    table.Take(e, "s2", 4, 70, alive, nullptr, updated, true);
    gone << 4;
    QVERIFY(table.Watch(c, true, 80, alive, sees).isEmpty());
    table.Take(e, "s3", 5, 90, alive, nullptr, updated, true);
    table.Publish(c, true, 90, alive, sees, answer);
    QVERIFY(!heard.contains(5));
    gone << 5;
    table.Take(e, "s4", 6, 100, alive, nullptr, updated, false);
    table.Publish(c, true, 100, alive, sees, answer);
    QCOMPARE(heard.value(6), QStringList() << QStringLiteral("tabs.onActivated") << QStringLiteral("|"));
    gone << 6;
    table.Names(e, "s3", QSet<QString>());
    QCOMPARE(table.NamesOf(e), updated);
    table.Names(e, "s4", QSet<QString>());
    QVERIFY(table.NamesOf(e).isEmpty());
    QVERIFY(!table.Watches());
    QVERIFY(table.Watch(a, true, 110, alive, sees).isEmpty());
    table.Names(e, "s4", updated);
    QCOMPARE(table.NamesOf(e), updated);
    table.Names(e, "s4", QSet<QString>(), 5);
    table.Names(e, "s4", updated, 4);
    QVERIFY(table.NamesOf(e).isEmpty());
    table.Names(e, "s4", updated, 6);
    QCOMPARE(table.NamesOf(e), updated);
    for(int i = 0; i < SUBSCRIBERS_OF_ONE; i++) table.Take(e, QByteArray("t") + QByteArray::number(i), 20 + i, 130, alive, nullptr, updated, true);
    QCOMPARE(table.Take(e, "t9", 29, 130, alive, nullptr, QSet<QString>(), true), Subscribers<int>::Refused);
    QCOMPARE(table.NamesOf(e), updated);
    table.Take(e, "t3", 30, 140, alive, nullptr, QSet<QString>() << QStringLiteral("tabs.onRemoved"), true);
    QCOMPARE(table.NamesOf(e), QSet<QString>() << QStringLiteral("tabs.onRemoved"));
    const QString f = QStringLiteral("f");
    table.Take(f, "u1", 40, 0, alive, nullptr, updated, true);
    table.Publish(a, true, 0, alive, sees, answer);
    table.Take(f, "u1", 41, 10, alive, nullptr, updated, true);
    table.Take(f, "u2", 42, 10, alive, nullptr, updated, true);
    table.Publish(a, true, 10, alive, sees, answer);
    QCOMPARE(heard.value(42), QStringList() << QStringLiteral("tabs.onActivated") << QStringLiteral("|"));
    QVERIFY(!heard.contains(41));
    const QString h = QStringLiteral("h");
    table.Take(h, "w1", 60, 0, alive, nullptr, updated, true);
    table.Publish(a, true, 0, alive, sees, answer);
    table.Fire(h, QJsonObject{ { QStringLiteral("name"), QStringLiteral("downloads.onChanged") } });
    QVERIFY(table.Watch(b, true, 5, alive, sees).contains(h));
    table.Take(h, "w2", 61, 5, alive, nullptr, updated, true);
    table.Publish(b, true, 5, alive, sees, answer);
    QCOMPARE(heard.value(61), QStringList() << QStringLiteral("tabs.onUpdated") << QStringLiteral("downloads.onChanged") << QStringLiteral("|"));
    QCOMPARE(table.Take(h, "w1", 62, 6, alive, nullptr, QSet<QString>() << QStringLiteral("tabs.onRemoved"), true), Subscribers<int>::Refused);
    QVERIFY(!table.Has(h, "w1"));
    QCOMPARE(table.NamesOf(h), updated);
    QVERIFY(!table.Holding(h, alive));
    QVERIFY(table.Retired(h, "w1"));
    QVERIFY(!table.Retired(h, "w2"));
    const QString k = QStringLiteral("k");
    table.Take(k, "x1", 70, 0, alive, nullptr, updated, true);
    table.Take(k, "x2", 71, 0, alive, nullptr, updated, true);
    table.Publish(a, true, 0, alive, sees, answer);
    table.Fire(k, QJsonObject{ { QStringLiteral("name"), QStringLiteral("action.onClicked") } });
    table.FireTo(k, QSet<QByteArray>() << "x2", QJsonObject{ { QStringLiteral("name"), QStringLiteral("runtime.onUserScriptMessage") } }, alive, 0);
    table.Take(k, "x3", 72, 1, alive, nullptr, updated, true);
    table.Publish(a, true, 1, alive, sees, answer);
    QCOMPARE(heard.value(72), QStringList() << QStringLiteral("action.onClicked") << QStringLiteral("|"));
    const QString g = QStringLiteral("g");
    table.Take(g, "v1", 50, 0, alive, nullptr, QSet<QString>() << QStringLiteral("commands.onCommand"), true);
    table.Publish(a, true, 0, alive, sees, answer);
    QVERIFY(!table.Watched().contains(g));
    QVERIFY(table.Watched().contains(f));
    table.Forget(f);
    QVERIFY(!table.Watched().contains(f));
    QVERIFY(table.Kept().contains(g));
    table.Forget(g);
    QVERIFY(!table.Kept().contains(g));
}

void tst_extensionhostwire::whatTheUserDidIsHeldWhileTheWorkerIsWoken(){
    using namespace ExtensionHostWire;
    auto event = [](int n){ return QJsonObject{ { QStringLiteral("n"), n } }; };
    Wakes wakes;
    QVERIFY(!wakes.Hold(QStringLiteral("e"), event(0), 0));
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), 0));
    QVERIFY(wakes.Subscribed(QStringLiteral("e"), 0).isEmpty());
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), 0));
    QVERIFY(wakes.Hold(QStringLiteral("e"), event(1), 100));
    QVERIFY(wakes.Hold(QStringLiteral("e"), event(2), 200));
    QVERIFY(wakes.ShouldWake(QStringLiteral("e"), 200));
    wakes.Woke(QStringLiteral("e"), 200);
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), 300));
    QVERIFY(wakes.Waking(QStringLiteral("e")));
    QVERIFY(wakes.Expire(200 + WAKE_WAIT - 1).isEmpty());
    const QList<QJsonObject> got = wakes.Subscribed(QStringLiteral("e"), 500);
    QCOMPARE(got.size(), 2);
    QCOMPARE(got.at(0), event(1));
    QCOMPARE(got.at(1), event(2));
    QVERIFY(!wakes.Waking(QStringLiteral("e")));
    QVERIFY(wakes.Subscribed(QStringLiteral("e"), 600).isEmpty());
    QVERIFY(wakes.Hold(QStringLiteral("e"), event(3), 1000));
    QVERIFY(wakes.Subscribed(QStringLiteral("e"), 1000 + WAKE_KEEP + 1).isEmpty());
    for(int i = 0; i <= WAKE_QUEUE; i++) QVERIFY(wakes.Hold(QStringLiteral("e"), event(10 + i), 50000));
    const QList<QJsonObject> many = wakes.Subscribed(QStringLiteral("e"), 50000);
    QCOMPARE(many.size(), WAKE_QUEUE);
    QCOMPARE(many.at(0), event(11));
    qint64 now = 100000;
    for(int i = 0; i < WAKE_TRIES; i++){
        QVERIFY(wakes.Hold(QStringLiteral("e"), event(100 + i), now));
        QVERIFY(wakes.ShouldWake(QStringLiteral("e"), now));
        wakes.Woke(QStringLiteral("e"), now);
        now += WAKE_WAIT;
        QCOMPARE(wakes.Expire(now), QStringList() << QStringLiteral("e"));
        QVERIFY(!wakes.AnyWaking());
    }
    QVERIFY(!wakes.Hold(QStringLiteral("e"), event(200), now + 1));
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), now + 1));
    QCOMPARE(wakes.RestLeft(QStringLiteral("e"), now + 1), WAKE_REST - 1);
    QCOMPARE(wakes.RestLeft(QStringLiteral("e"), now + WAKE_REST), qint64(0));
    QVERIFY(wakes.Hold(QStringLiteral("e"), event(201), now + WAKE_REST));
    QCOMPARE(wakes.Extensions(), QStringList() << QStringLiteral("e"));
    wakes.Woke(QStringLiteral("e"), now + WAKE_REST);
    wakes.Forget(QStringLiteral("e"));
    QVERIFY(!wakes.Waking(QStringLiteral("e")));
    QVERIFY(wakes.Subscribed(QStringLiteral("e"), now + WAKE_REST).isEmpty());

    Subscribers<int> subscribers;
    bool there = true;
    auto alive = [&there](int){ return there; };
    QVERIFY(!subscribers.Listening(QStringLiteral("e"), alive, 0));
    subscribers.Take(QStringLiteral("e"), "s1", 1, 0, alive, nullptr);
    QVERIFY(subscribers.Listening(QStringLiteral("e"), alive, 0));
    QVERIFY(!subscribers.Listening(QStringLiteral("f"), alive, 0));
    there = false;
    QVERIFY(!subscribers.Listening(QStringLiteral("e"), alive, 0));
    int held = 0;
    QVERIFY(subscribers.Abort(QStringLiteral("e"), "s1", &held));
    QVERIFY(subscribers.Listening(QStringLiteral("e"), alive, SUBSCRIBER_BETWEEN_MS - 1));
    QVERIFY(!subscribers.Listening(QStringLiteral("e"), alive, SUBSCRIBER_BETWEEN_MS));
}

void tst_extensionhostwire::aUserScriptsMessageWaitsForTheWorkerWoken(){
    using namespace ExtensionHostWire;
    typedef UserScriptMessages<int> Messages;
    Messages messages;
    const QJsonObject event{ { QStringLiteral("name"), QStringLiteral("runtime.onUserScriptMessage") } };
    QVERIFY(messages.Wait("t1", QStringLiteral("e"), 1, event, 0));
    QVERIFY(messages.HasWaiting(QStringLiteral("e")));
    QVERIFY(!messages.HasWaiting(QStringLiteral("f")));
    QCOMPARE(messages.Waiting(QStringLiteral("e")).size(), 1);
    QCOMPARE(messages.Waiting(QStringLiteral("e")).at(0).first, QByteArray("t1"));
    auto everyone = [](const QString &, const QByteArray &){ return true; };
    auto alive = [](int){ return true; };
    QVERIFY(messages.Sweep(WAKE_KEEP, everyone, alive).isEmpty());
    QCOMPARE(messages.Count(), 1);
    QStringList fired;
    auto fire = [&fired](const QJsonObject &e){ fired << e.value(QStringLiteral("name")).toString(); return true; };
    QCOMPARE(DeliverWaiting(messages, QStringLiteral("e"), "new", false, true, 100, fire), 0);
    QCOMPARE(DeliverWaiting(messages, QStringLiteral("e"), "new", true, false, 100, fire), 0);
    QVERIFY(fired.isEmpty());
    QCOMPARE(DeliverWaiting(messages, QStringLiteral("e"), "full", true, true, 100, [](const QJsonObject &){ return false; }), 0);
    QVERIFY(messages.HasWaiting(QStringLiteral("e")));
    QCOMPARE(DeliverWaiting(messages, QStringLiteral("e"), "new", true, true, 100, fire), 1);
    QCOMPARE(fired, QStringList() << QStringLiteral("runtime.onUserScriptMessage"));
    QCOMPARE(DeliverWaiting(messages, QStringLiteral("e"), "new", true, true, 200, fire), 0);
    QCOMPARE(fired.size(), 1);
    QVERIFY(!messages.HasWaiting(QStringLiteral("e")));
    QVERIFY(messages.Reply(QStringLiteral("e"), "t1", "old", Messages::Value, QJsonValue(1)).isEmpty());
    QVERIFY(messages.Sweep(100 + USER_SCRIPT_WAIT, everyone, alive).isEmpty());
    QList<Messages::Done> done = messages.Reply(QStringLiteral("e"), "t1", "new", Messages::Value, QJsonValue(2));
    QCOMPARE(done.size(), 1);
    QCOMPARE(done.at(0).answer.value(QStringLiteral("value")).toInt(), 2);
    QVERIFY(messages.Wait("t2", QStringLiteral("e"), 2, event, 1000));
    QVERIFY(messages.Wait("t3", QStringLiteral("e"), 3, event, 1000));
    done = messages.Sweep(1000 + WAKE_KEEP + 1, everyone, [](int waiter){ return waiter != 3; });
    QCOMPARE(done.size(), 1);
    QCOMPARE(done.at(0).waiter, 2);
    QCOMPARE(done.at(0).answer.value(QStringLiteral("error")).toString(), NoReceiver());
    QCOMPARE(done.at(0).unheardOf, QStringLiteral("e"));
    QCOMPARE(messages.Count(), 0);
    QVERIFY(WaitForWorker(true, true, false, false));
    QVERIFY(!WaitForWorker(false, true, false, false));
    QVERIFY(!WaitForWorker(true, false, false, false));
    QVERIFY(!WaitForWorker(true, true, true, false));
    QVERIFY(!WaitForWorker(true, true, false, true));
    for(int i = 0; i < USER_SCRIPT_PENDING; i++) QVERIFY(messages.Wait(QByteArray::number(i), QStringLiteral("g"), i, event, 0));
    QVERIFY(!messages.Wait("over", QStringLiteral("g"), 0, event, 0));
    QVERIFY(!messages.Add("over", QStringLiteral("g"), 0, QSet<QByteArray>() << "w", 0));
    QCOMPARE(messages.Drop(QStringLiteral("g")).size(), USER_SCRIPT_PENDING);

    Wakes wakes;
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), 0, true));
    wakes.Subscribed(QStringLiteral("e"), 0);
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), 0, false));
    QVERIFY(wakes.ShouldWake(QStringLiteral("e"), 0, true));
    QVERIFY(!wakes.Resting(QStringLiteral("e"), 0));
    qint64 now = 0;
    for(int i = 0; i < WAKE_TRIES; i++){
        wakes.Woke(QStringLiteral("e"), now);
        QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), now, true));
        now += WAKE_WAIT;
        wakes.Expire(now);
    }
    QVERIFY(wakes.Resting(QStringLiteral("e"), now));
    QVERIFY(!wakes.ShouldWake(QStringLiteral("e"), now, true));
    QVERIFY(!wakes.Resting(QStringLiteral("e"), now + WAKE_REST));
    QVERIFY(!wakes.Resting(QStringLiteral("f"), 0));
}

QTEST_MAIN(tst_extensionhostwire)
void tst_extensionhostwire::onlyAnExtensionsRulesTravelInTheBody(){
    QVERIFY(ExtensionHostWire::TakesBody(QStringLiteral("declarativeNetRequest.updateDynamicRules")));
    QVERIFY(ExtensionHostWire::TakesBody(QStringLiteral("declarativeNetRequest.updateSessionRules")));
    const char *others[] = { "declarativeNetRequest.getDynamicRules", "declarativeNetRequest.updateEnabledRulesets",
                             "tabs.create", "contextMenus.create", "vanilla.events", "" };
    for(const char *api : others) QVERIFY2(!ExtensionHostWire::TakesBody(QString::fromLatin1(api)), api);

    QString error = QStringLiteral("stale");
    const QJsonArray args = ExtensionHostWire::ArgsOfBody("[{\"addRules\":[]}]", &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(args.size(), 1);
    QVERIFY(args.at(0).toObject().contains(QStringLiteral("addRules")));
    const char *refused[] = { "{\"addRules\":[]}", "[1,", "", "null" };
    for(const char *body : refused){
        error.clear();
        QVERIFY(ExtensionHostWire::ArgsOfBody(QByteArray(body), &error).isEmpty());
        QVERIFY2(!error.isEmpty(), body);
    }
}

#include "tst_extensionhostwire.moc"
