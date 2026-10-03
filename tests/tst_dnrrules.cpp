#include "switch.hpp"

#include <QtTest>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "dnrrules.hpp"

#include "testsupport.hpp"

class tst_dnrrules : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void aFilterIsReadTheWayChromiumReadsIt_data();
    void aFilterIsReadTheWayChromiumReadsIt();

    void aRuleWithNoTypeLeavesThePageItselfAlone();
    void domainsAreTheHostAndWhatIsUnderIt();
    void whoAskedDecidesFirstAndThirdParty();
    void methodsNarrowARule();

    void theHigherPriorityWinsAndAnAllowWinsATie();
    void anAllowSpeaksForItsOwnExtensionOnly();
    void aRedirectGoesWhereTheRuleSays();

    void whatIsNotEvaluatedIsCountedNotPretended();
    void aRuleNotFullyReadIsLeftOutNotWidened();
    void someRequestsAreNobodysToRule();
    void aRedirectNeverComesBackToItself();
    void twoRulesOfOneRankAreSettledByWhichWasWritten();
    void aBrokenRulesetIsOneRulesetLost();
    void anEmptySetAnswersNothing();
    void anAllowAllRequestsOnTheDocumentShieldsItsRequests();
    void theDocumentsAreRememberedByAddressAndGeneration();

    void heldRulesChangeAllOrNothing();
    void heldRulesAreBoundedAsInChrome();
    void sessionRulesGoWithTheVersionAndAreNeverKept();
    void theEnabledRulesetsAreTheManifestsUntilChosen();
    void whatIsHeldReadsBackAndGoesWithTheExtension();

    void aRealSizedSetIsAnsweredQuickly();
    void measureARealExtension();

private:
    static QByteArray Json(const QJsonArray &rules){
        return QJsonDocument(rules).toJson(QJsonDocument::Compact);
    }
    static QJsonObject Rule(int id, const QString &action, const QJsonObject &condition,
                            int priority = 1){
        QJsonObject rule;
        rule[QStringLiteral("id")] = id;
        rule[QStringLiteral("priority")] = priority;
        QJsonObject act;
        act[QStringLiteral("type")] = action;
        rule[QStringLiteral("action")] = act;
        rule[QStringLiteral("condition")] = condition;
        return rule;
    }
    static QJsonObject Filter(const QString &filter){
        QJsonObject condition;
        condition[QStringLiteral("urlFilter")] = filter;
        return condition;
    }
    static Dnr::Request Ask(const QString &url, Dnr::ResourceType type = Dnr::Script,
                            const QString &initiator = QStringLiteral("https://page.example/")){
        Dnr::Request request;
        request.url = QUrl(url);
        request.initiator = QUrl(initiator);
        request.type = type;
        request.method = "GET";
        return request;
    }
    static Dnr::Decision::Kind Kind(const Dnr::Rules &rules, const Dnr::Request &request){
        return rules.Evaluate(request).kind;
    }
};

void tst_dnrrules::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_dnrrules::aFilterIsReadTheWayChromiumReadsIt_data(){
    QTest::addColumn<QString>("filter");
    QTest::addColumn<bool>("caseSensitive");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("blocked");

    QTest::newRow("domain anchor, the host itself")
        << "||ads.example.com^" << false << "https://ads.example.com/x.js" << true;
    QTest::newRow("domain anchor, a subdomain")
        << "||ads.example.com^" << false << "https://sub.ads.example.com/x.js" << true;
    QTest::newRow("domain anchor, not the middle of a label")
        << "||ads.example.com^" << false << "https://notads.example.com/x.js" << false;
    QTest::newRow("domain anchor, not the path")
        << "||ads.example.com^" << false << "https://page.example/ads.example.com/" << false;
    QTest::newRow("domain anchor, past a user name")
        << "||ads.example.com^" << false << "https://user@ads.example.com/x.js" << true;
    QTest::newRow("domain anchor stopping mid label is a prefix")
        << "||ads" << false << "https://adserver.example/x.js" << true;
    QTest::newRow("domain anchor with a path")
        << "||example.com/banner/" << false << "https://cdn.example.com/banner/1.png" << true;
    QTest::newRow("domain anchor with a path, another path")
        << "||example.com/banner/" << false << "https://cdn.example.com/images/1.png" << false;

    QTest::newRow("separator is the slash") << "||a.example^" << false << "https://a.example/" << true;
    QTest::newRow("separator is the port colon") << "||a.example^" << false << "https://a.example:8080/" << true;
    QTest::newRow("separator is the end") << "||a.example^" << false << "https://a.example" << true;
    QTest::newRow("separator is not a dot") << "||a.example^" << false << "https://a.example.org/" << false;
    QTest::newRow("separator is not a letter") << "/ad^" << false << "https://x.example/adx" << false;
    QTest::newRow("separator is the question mark") << "/ad^" << false << "https://x.example/ad?x=1" << true;

    QTest::newRow("only the last separator may be the end") << "/ad^^" << false << "https://x.example/ad" << false;
    QTest::newRow("a separator in the middle is never the end") << "/ad^*x" << false << "https://x.example/ad" << false;

    QTest::newRow("an at sign in the query") << "||ads.example.com^" << false << "https://a.example/?u=x@ads.example.com/" << false;
    QTest::newRow("an at sign in the query, the real host still found") << "||a.example^" << false << "https://a.example/?u=x@ads.example.com/" << true;
    QTest::newRow("an address literal") << "||[2001:db8::1]^" << false << "http://[2001:db8::1]/x.js" << true;
    QTest::newRow("a port behind the host") << "||ads.example.com:8080^" << false << "http://ads.example.com:8080/x.js" << true;

    QTest::newRow("left anchor") << "|https://a.example/" << false << "https://a.example/x" << true;
    QTest::newRow("left anchor, not later") << "|a.example" << false << "https://a.example/x" << false;
    QTest::newRow("right anchor") << ".js|" << false << "https://a.example/x.js" << true;
    QTest::newRow("right anchor, not earlier") << ".js|" << false << "https://a.example/x.js?v=1" << false;

    QTest::newRow("plain text anywhere") << "/banner/" << false << "https://a.example/img/banner/1.png" << true;
    QTest::newRow("wildcard between") << "/ads/*.gif" << false << "https://a.example/ads/2026/x.gif" << true;
    QTest::newRow("wildcard between, order kept") << "/ads/*.gif" << false << "https://a.example/x.gif/ads/" << false;
    QTest::newRow("wildcard with right anchor takes the last") << "/a*.js|" << false << "https://a.example/a.js/b.js" << true;
    QTest::newRow("only a wildcard") << "*" << false << "https://a.example/" << true;

    QTest::newRow("case ignored") << "/Banner/" << false << "https://a.example/BANNER/x" << true;
    QTest::newRow("case kept when asked") << "/Banner/" << true << "https://a.example/banner/x" << false;
    QTest::newRow("case kept when asked, same case") << "/Banner/" << true << "https://a.example/Banner/x" << true;
}

void tst_dnrrules::aFilterIsReadTheWayChromiumReadsIt(){
    QFETCH(QString, filter);
    QFETCH(bool, caseSensitive);
    QFETCH(QString, url);
    QFETCH(bool, blocked);

    QJsonObject condition = Filter(filter);
    condition[QStringLiteral("isUrlFilterCaseSensitive")] = caseSensitive;
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), condition)));
    QCOMPARE(rules.Count(), 1);
    QCOMPARE(Kind(rules, Ask(url)) == Dnr::Decision::Block, blocked);

    const Dnr::UrlFilter compiled(filter, caseSensitive);
    const QString token = compiled.IndexToken();
    if(blocked && !token.isEmpty()){
        const QString lower = QString::fromLatin1(QUrl(url).toEncoded()).toLower();
        const QStringList own = lower.split(QRegularExpression(QStringLiteral("[^a-z0-9]+")), Qt::SkipEmptyParts);
        QVERIFY2(own.contains(token),
                 qPrintable(QStringLiteral("filed under '%1', which %2 does not hold").arg(token, lower)));
    }
}

void tst_dnrrules::aRuleWithNoTypeLeavesThePageItselfAlone(){
    Dnr::Rules plain;
    plain.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("block"), Filter(QStringLiteral("||bad.example^")))));
    QCOMPARE(Kind(plain, Ask(QStringLiteral("https://bad.example/"), Dnr::MainFrame)), Dnr::Decision::None);
    QCOMPARE(Kind(plain, Ask(QStringLiteral("https://bad.example/"), Dnr::SubFrame)), Dnr::Decision::Block);
    QCOMPARE(Kind(plain, Ask(QStringLiteral("https://bad.example/x.js"), Dnr::Script)), Dnr::Decision::Block);

    QJsonObject named = Filter(QStringLiteral("||bad.example^"));
    named[QStringLiteral("resourceTypes")] = QJsonArray() << QStringLiteral("main_frame");
    Dnr::Rules page;
    page.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), named)));
    QCOMPARE(Kind(page, Ask(QStringLiteral("https://bad.example/"), Dnr::MainFrame)), Dnr::Decision::Block);
    QCOMPARE(Kind(page, Ask(QStringLiteral("https://bad.example/x.js"), Dnr::Script)), Dnr::Decision::None);

    QJsonObject excluded = Filter(QStringLiteral("||bad.example^"));
    excluded[QStringLiteral("excludedResourceTypes")] = QJsonArray() << QStringLiteral("image");
    Dnr::Rules rest;
    rest.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), excluded)));
    QCOMPARE(Kind(rest, Ask(QStringLiteral("https://bad.example/"), Dnr::MainFrame)), Dnr::Decision::Block);
    QCOMPARE(Kind(rest, Ask(QStringLiteral("https://bad.example/x.png"), Dnr::Image)), Dnr::Decision::None);
}

void tst_dnrrules::domainsAreTheHostAndWhatIsUnderIt(){
    QJsonObject condition;
    condition[QStringLiteral("requestDomains")] = QJsonArray() << QStringLiteral("tracker.example");
    condition[QStringLiteral("excludedRequestDomains")] = QJsonArray() << QStringLiteral("ok.tracker.example");
    condition[QStringLiteral("excludedInitiatorDomains")] = QJsonArray() << QStringLiteral("friend.example");
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), condition)));

    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.tracker.example/p"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://ok.tracker.example/p"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://other.example/p"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://nottracker.example/p"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://example/p"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://tracker.example/p"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.tracker.example/p"), Dnr::Script,
                             QStringLiteral("https://www.friend.example/"))), Dnr::Decision::None);

    QJsonObject from;
    from[QStringLiteral("initiatorDomains")] = QJsonArray() << QStringLiteral("news.example");
    Dnr::Rules only;
    only.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), from)));
    QCOMPARE(Kind(only, Ask(QStringLiteral("https://x.example/a.js"), Dnr::Script,
                            QStringLiteral("https://www.news.example/"))), Dnr::Decision::Block);
    QCOMPARE(Kind(only, Ask(QStringLiteral("https://x.example/a.js"), Dnr::Script, QString())), Dnr::Decision::None);
}

void tst_dnrrules::whoAskedDecidesFirstAndThirdParty(){
    QCOMPARE(Dnr::RegistrableDomain(QStringLiteral("a.b.example.com")), QStringLiteral("example.com"));
    QCOMPARE(Dnr::RegistrableDomain(QStringLiteral("example.com")), QStringLiteral("example.com"));
    QCOMPARE(Dnr::RegistrableDomain(QStringLiteral("a.example.co.jp")), QStringLiteral("example.co.jp"));
    QCOMPARE(Dnr::RegistrableDomain(QStringLiteral("192.168.0.1")), QStringLiteral("192.168.0.1"));
    QCOMPARE(Dnr::RegistrableDomain(QStringLiteral("localhost")), QStringLiteral("localhost"));

    QJsonObject third = Filter(QStringLiteral("/pixel"));
    third[QStringLiteral("domainType")] = QStringLiteral("thirdParty");
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), third)));

    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://img.shop.example/pixel"), Dnr::Image,
                             QStringLiteral("https://www.shop.example/"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://b.example.co.jp/pixel"), Dnr::Image,
                             QStringLiteral("https://a.example.co.jp/"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://other.co.jp/pixel"), Dnr::Image,
                             QStringLiteral("https://a.example.co.jp/"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://ads.example/pixel"), Dnr::Image,
                             QStringLiteral("https://www.shop.example/"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://ads.example/pixel"), Dnr::Image, QString())),
             Dnr::Decision::Block);
}

void tst_dnrrules::methodsNarrowARule(){
    QJsonObject post = Filter(QStringLiteral("/collect"));
    post[QStringLiteral("requestMethods")] = QJsonArray() << QStringLiteral("post");
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), post)));

    Dnr::Request request = Ask(QStringLiteral("https://a.example/collect"), Dnr::XmlHttpRequest);
    QCOMPARE(Kind(rules, request), Dnr::Decision::None);
    request.method = "POST";
    QCOMPARE(Kind(rules, request), Dnr::Decision::Block);
}

void tst_dnrrules::theHigherPriorityWinsAndAnAllowWinsATie(){
    const QJsonObject filter = Filter(QStringLiteral("||cdn.example^"));

    Dnr::Rules allowHigher;
    allowHigher.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("block"), filter, 1)
        << Rule(2, QStringLiteral("allow"), filter, 2000)));
    QCOMPARE(Kind(allowHigher, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::None);

    Dnr::Rules blockHigher;
    blockHigher.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("allow"), filter, 1)
        << Rule(2, QStringLiteral("block"), filter, 2)));
    QCOMPARE(Kind(blockHigher, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::Block);

    Dnr::Rules tie;
    tie.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("block"), filter, 7)
        << Rule(2, QStringLiteral("allow"), filter, 7)));
    QCOMPARE(Kind(tie, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::None);
    Dnr::Rules tieOtherWay;
    tieOtherWay.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("allow"), filter, 7)
        << Rule(2, QStringLiteral("block"), filter, 7)));
    QCOMPARE(Kind(tieOtherWay, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::None);

    Dnr::Rules twoFiles;
    twoFiles.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), filter, 1)));
    twoFiles.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("allow"), filter, 5)));
    QCOMPARE(Kind(twoFiles, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::None);
}

void tst_dnrrules::anAllowSpeaksForItsOwnExtensionOnly(){
    const QJsonObject filter = Filter(QStringLiteral("||cdn.example^"));
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("one"), Json(QJsonArray() << Rule(1, QStringLiteral("allow"), filter, 9999)));
    rules.AddRuleset(QStringLiteral("two"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), filter, 1)));
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::Block);

    QJsonObject redirect = Rule(1, QStringLiteral("redirect"), filter, 1);
    QJsonObject action = redirect[QStringLiteral("action")].toObject();
    QJsonObject to; to[QStringLiteral("url")] = QStringLiteral("https://safe.example/");
    action[QStringLiteral("redirect")] = to;
    redirect[QStringLiteral("action")] = action;
    Dnr::Rules mixed;
    mixed.AddRuleset(QStringLiteral("one"), Json(QJsonArray() << redirect));
    mixed.AddRuleset(QStringLiteral("two"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), filter, 1)));
    QCOMPARE(Kind(mixed, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::Block);
}

void tst_dnrrules::aRedirectGoesWhereTheRuleSays(){
    auto redirectRule = [](int id, const QJsonObject &condition, const QJsonObject &to){
        QJsonObject rule = Rule(id, QStringLiteral("redirect"), condition);
        QJsonObject action = rule[QStringLiteral("action")].toObject();
        action[QStringLiteral("redirect")] = to;
        rule[QStringLiteral("action")] = action;
        return rule;
    };
    QJsonObject page = Filter(QStringLiteral("||malware.example^"));
    page[QStringLiteral("resourceTypes")] = QJsonArray() << QStringLiteral("main_frame");
    QJsonObject toUrl; toUrl[QStringLiteral("url")] = QStringLiteral("chrome-extension://abc/blocked.html?u=1");
    QJsonObject toPath; toPath[QStringLiteral("extensionPath")] = QStringLiteral("/stub.js");

    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("abc"), Json(QJsonArray()
        << redirectRule(1, page, toUrl)
        << redirectRule(2, Filter(QStringLiteral("||lib.example/tracker.js")), toPath)));

    Dnr::Decision first = rules.Evaluate(Ask(QStringLiteral("https://malware.example/"), Dnr::MainFrame, QString()));
    QCOMPARE(first.kind, Dnr::Decision::Redirect);
    QCOMPARE(first.redirect, QUrl(QStringLiteral("chrome-extension://abc/blocked.html?u=1")));

    Dnr::Decision second = rules.Evaluate(Ask(QStringLiteral("https://lib.example/tracker.js")));
    QCOMPARE(second.kind, Dnr::Decision::Redirect);
    QCOMPARE(second.redirect, QUrl(QStringLiteral("chrome-extension://abc/stub.js")));

    Dnr::Rules upgrade;
    upgrade.AddRuleset(QStringLiteral("abc"), Json(QJsonArray()
        << Rule(1, QStringLiteral("upgradeScheme"), Filter(QStringLiteral("||plain.example^")))));
    Dnr::Decision up = upgrade.Evaluate(Ask(QStringLiteral("http://plain.example/a.js?x=1")));
    QCOMPARE(up.kind, Dnr::Decision::Redirect);
    QCOMPARE(up.redirect, QUrl(QStringLiteral("https://plain.example/a.js?x=1")));
    QCOMPARE(Kind(upgrade, Ask(QStringLiteral("https://plain.example/a.js"))), Dnr::Decision::None);
}

void tst_dnrrules::whatIsNotEvaluatedIsCountedNotPretended(){
    QJsonObject headers = Rule(1, QStringLiteral("modifyHeaders"), Filter(QStringLiteral("||a.example^")));
    QJsonObject transform = Rule(2, QStringLiteral("redirect"), Filter(QStringLiteral("||b.example^")));
    {
        QJsonObject action = transform[QStringLiteral("action")].toObject();
        QJsonObject to; to[QStringLiteral("transform")] = QJsonObject();
        action[QStringLiteral("redirect")] = to;
        transform[QStringLiteral("action")] = action;
    }
    QJsonObject tabCondition = Filter(QStringLiteral("||c.example^"));
    tabCondition[QStringLiteral("tabIds")] = QJsonArray() << 3;
    QJsonObject tabs = Rule(3, QStringLiteral("block"), tabCondition);
    QJsonObject regexCondition;
    regexCondition[QStringLiteral("regexFilter")] = QStringLiteral("([unclosed");
    QJsonObject regex = Rule(4, QStringLiteral("block"), regexCondition);
    QJsonObject unknown = Rule(5, QStringLiteral("teleport"), Filter(QStringLiteral("||e.example^")));
    QJsonObject good = Rule(6, QStringLiteral("block"), Filter(QStringLiteral("||f.example^")));
    QJsonObject goodRegexCondition;
    goodRegexCondition[QStringLiteral("regexFilter")] = QStringLiteral("^https://g\\.example/[0-9]+\\.js$");
    QJsonObject goodRegex = Rule(7, QStringLiteral("block"), goodRegexCondition);

    Dnr::Skipped skipped;
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << headers << transform << tabs << regex << unknown << good << goodRegex), &skipped);

    QCOMPARE(skipped.modifyHeaders, 1);
    QCOMPARE(skipped.computedRedirect, 1);
    QCOMPARE(skipped.tabBound, 1);
    QCOMPARE(skipped.badRegex, 1);
    QCOMPARE(skipped.malformed, 1);
    QCOMPARE(skipped.Total(), 5);
    QCOMPARE(rules.Count(), 2);

    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.example/x.js"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://b.example/x.js"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://c.example/x.js"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://f.example/x.js"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://g.example/123.js"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://g.example/abc.js"))), Dnr::Decision::None);
}

void tst_dnrrules::aRuleNotFullyReadIsLeftOutNotWidened(){
    QJsonObject header = Filter(QStringLiteral("||a.example^"));
    header[QStringLiteral("responseHeaders")] = QJsonArray() << QJsonObject();
    QJsonObject newer = Filter(QStringLiteral("||b.example^"));
    newer[QStringLiteral("somethingAddedLater")] = true;
    QJsonObject zero = Rule(3, QStringLiteral("block"), Filter(QStringLiteral("||c.example^")), 0);
    QJsonObject wild = Rule(4, QStringLiteral("block"), Filter(QStringLiteral("||*d.example^")));
    auto redirect = [](int id, const QString &filter, const QString &key, const QString &to){
        QJsonObject rule = Rule(id, QStringLiteral("redirect"), Filter(filter));
        QJsonObject action = rule[QStringLiteral("action")].toObject();
        QJsonObject target; target[key] = to;
        action[QStringLiteral("redirect")] = target;
        rule[QStringLiteral("action")] = action;
        return rule;
    };

    Dnr::Skipped skipped;
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("block"), header) << Rule(2, QStringLiteral("block"), newer)
        << zero << wild
        << redirect(5, QStringLiteral("||e.example^"), QStringLiteral("url"), QStringLiteral("javascript:alert(1)"))
        << redirect(6, QStringLiteral("||f.example^"), QStringLiteral("extensionPath"), QStringLiteral("stub.js"))), &skipped);
    QCOMPARE(skipped.unreadCondition, 2);
    QCOMPARE(skipped.malformed, 4);
    QVERIFY(rules.IsEmpty());

    QJsonObject old = Filter(QStringLiteral("/pixel"));
    old[QStringLiteral("domains")] = QJsonArray() << QStringLiteral("news.example");
    old[QStringLiteral("excludedDomains")] = QJsonArray() << QStringLiteral("quiet.news.example");
    Dnr::Rules legacy;
    legacy.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), old)));
    QCOMPARE(legacy.Count(), 1);
    QCOMPARE(Kind(legacy, Ask(QStringLiteral("https://t.example/pixel"), Dnr::Image,
                              QStringLiteral("https://www.news.example/"))), Dnr::Decision::Block);
    QCOMPARE(Kind(legacy, Ask(QStringLiteral("https://t.example/pixel"), Dnr::Image,
                              QStringLiteral("https://quiet.news.example/"))), Dnr::Decision::None);
    QCOMPARE(Kind(legacy, Ask(QStringLiteral("https://t.example/pixel"), Dnr::Image,
                              QStringLiteral("https://other.example/"))), Dnr::Decision::None);
}

void tst_dnrrules::someRequestsAreNobodysToRule(){
    QJsonObject everything = Filter(QStringLiteral("*"));
    everything[QStringLiteral("resourceTypes")] = QJsonArray()
        << QStringLiteral("main_frame") << QStringLiteral("script") << QStringLiteral("websocket");
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), everything)));
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.example/x.js"))), Dnr::Decision::Block);

    QCOMPARE(Kind(rules, Ask(QStringLiteral("chrome-extension://abc/index.html?b_url=a.example"), Dnr::MainFrame, QString())),
             Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("vanilla://settings/"), Dnr::MainFrame, QString())), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("data:text/javascript,1"))), Dnr::Decision::None);

    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.example/x.js"), Dnr::Script,
                             QStringLiteral("chrome-extension://other/popup.html"))), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.example/"), Dnr::MainFrame,
                             QStringLiteral("chrome-extension://other/popup.html"))), Dnr::Decision::Block);

    QJsonObject gets = Filter(QStringLiteral("||s.example^"));
    gets[QStringLiteral("requestMethods")] = QJsonArray() << QStringLiteral("get");
    gets[QStringLiteral("resourceTypes")] = QJsonArray() << QStringLiteral("websocket") << QStringLiteral("script");
    Dnr::Rules methods;
    methods.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), gets)));
    QCOMPARE(Kind(methods, Ask(QStringLiteral("https://s.example/a.js"))), Dnr::Decision::Block);
    QCOMPARE(Kind(methods, Ask(QStringLiteral("wss://s.example/socket"), Dnr::WebSocket)), Dnr::Decision::None);
}

void tst_dnrrules::aRedirectNeverComesBackToItself(){
    auto redirectTo = [](const QString &filter, const QString &type, const QString &to){
        QJsonObject condition = Filter(filter);
        condition[QStringLiteral("resourceTypes")] = QJsonArray() << type;
        QJsonObject rule = Rule(1, QStringLiteral("redirect"), condition);
        QJsonObject action = rule[QStringLiteral("action")].toObject();
        QJsonObject target; target[QStringLiteral("url")] = to;
        action[QStringLiteral("redirect")] = target;
        rule[QStringLiteral("action")] = action;
        return rule;
    };
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << redirectTo(QStringLiteral("shim.js"), QStringLiteral("script"), QStringLiteral("https://cdn.example/shim.js"))));
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://ads.example/shim.js"))), Dnr::Decision::Redirect);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://cdn.example/shim.js"))), Dnr::Decision::None);

    Dnr::Rules sockets;
    sockets.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << redirectTo(QStringLiteral("||s.example^"), QStringLiteral("websocket"), QStringLiteral("wss://elsewhere.example/"))));
    QCOMPARE(Kind(sockets, Ask(QStringLiteral("wss://s.example/socket"), Dnr::WebSocket)), Dnr::Decision::None);
}

void tst_dnrrules::twoRulesOfOneRankAreSettledByWhichWasWritten(){
    auto redirectTo = [](int id, const QString &filter, const QString &to){
        QJsonObject rule = Rule(id, QStringLiteral("redirect"), Filter(filter));
        QJsonObject action = rule[QStringLiteral("action")].toObject();
        QJsonObject target; target[QStringLiteral("url")] = to;
        action[QStringLiteral("redirect")] = target;
        rule[QStringLiteral("action")] = action;
        return rule;
    };
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << redirectTo(1, QStringLiteral("/alpha/"), QStringLiteral("https://first.example/"))
        << redirectTo(2, QStringLiteral("/beta/"), QStringLiteral("https://second.example/"))));
    QCOMPARE(rules.Evaluate(Ask(QStringLiteral("https://a.example/alpha/beta/x.js"))).redirect,
             QUrl(QStringLiteral("https://first.example/")));
    QCOMPARE(rules.Evaluate(Ask(QStringLiteral("https://a.example/beta/alpha/x.js"))).redirect,
             QUrl(QStringLiteral("https://first.example/")));
}

void tst_dnrrules::aBrokenRulesetIsOneRulesetLost(){
    Dnr::Skipped skipped;
    Dnr::Rules rules;
    rules.AddRuleset(QStringLiteral("ext"), QByteArray("{ not json"), &skipped);
    rules.AddRuleset(QStringLiteral("ext"), QByteArray("{\"an\": \"object\"}"), &skipped);
    QCOMPARE(skipped.malformed, 2);
    QVERIFY(rules.IsEmpty());

    rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("block"), Filter(QStringLiteral("||f.example^")))), &skipped);
    QCOMPARE(rules.Count(), 1);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://f.example/x.js"))), Dnr::Decision::Block);
}

void tst_dnrrules::anAllowAllRequestsOnTheDocumentShieldsItsRequests(){
    QJsonObject site = Filter(QStringLiteral("||site.example^"));
    site[QStringLiteral("resourceTypes")] = QJsonArray() << QStringLiteral("main_frame") << QStringLiteral("sub_frame");
    const QJsonObject cdn = Filter(QStringLiteral("||cdn.example^"));
    auto built = [&](int priority, const QString &action = QStringLiteral("block")){
        QJsonObject rule = Rule(1, action, cdn, priority);
        if(action == QStringLiteral("redirect")){
            QJsonObject act = rule[QStringLiteral("action")].toObject();
            QJsonObject to; to[QStringLiteral("url")] = QStringLiteral("https://safe.example/");
            act[QStringLiteral("redirect")] = to;
            rule[QStringLiteral("action")] = act;
        }
        Dnr::Rules rules;
        rules.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << rule << Rule(2, QStringLiteral("allowAllRequests"), site, 5)));
        return rules;
    };
    const quint64 allowAllOfFive = (quint64(5) << 8) | 4;

    Dnr::Rules rules = built(3);
    const Dnr::Decision navigation = rules.Evaluate(Ask(QStringLiteral("https://site.example/page"), Dnr::MainFrame, QString()));
    QCOMPARE(navigation.kind, Dnr::Decision::None);
    QCOMPARE(navigation.frameAllows.size(), 1);
    QCOMPARE(navigation.frameAllows.value(QStringLiteral("ext")), allowAllOfFive);
    QVERIFY(rules.Evaluate(Ask(QStringLiteral("https://site.example/a.js"))).frameAllows.isEmpty());
    QJsonObject typed = Filter(QStringLiteral("||site.example^"));
    typed[QStringLiteral("resourceTypes")] = QJsonArray() << QStringLiteral("main_frame") << QStringLiteral("script");
    Dnr::Rules wide;
    wide.AddRuleset(QStringLiteral("ext"), Json(QJsonArray() << Rule(1, QStringLiteral("allowAllRequests"), typed, 5)));
    QVERIFY(wide.Evaluate(Ask(QStringLiteral("https://site.example/a.js"))).frameAllows.isEmpty());
    QCOMPARE(wide.Evaluate(Ask(QStringLiteral("https://site.example/"), Dnr::MainFrame, QString())).frameAllows.value(QStringLiteral("ext")), allowAllOfFive);
    QVERIFY(rules.Evaluate(Ask(QStringLiteral("https://other.example/"), Dnr::MainFrame, QString())).frameAllows.isEmpty());

    Dnr::Request under = Ask(QStringLiteral("https://cdn.example/a.js"));
    under.frameAllows = navigation.frameAllows;
    QCOMPARE(Kind(rules, under), Dnr::Decision::None);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://cdn.example/a.js"))), Dnr::Decision::Block);
    QCOMPARE(Kind(built(5), under), Dnr::Decision::None);
    QCOMPARE(Kind(built(7), under), Dnr::Decision::Block);
    QCOMPARE(Kind(built(3, QStringLiteral("redirect")), under), Dnr::Decision::None);
    Dnr::Request plain = Ask(QStringLiteral("http://cdn.example/a.js"));
    Dnr::Request plainUnder = plain;
    plainUnder.frameAllows = navigation.frameAllows;
    QCOMPARE(Kind(built(3, QStringLiteral("upgradeScheme")), plain), Dnr::Decision::Redirect);
    QCOMPARE(Kind(built(3, QStringLiteral("upgradeScheme")), plainUnder), Dnr::Decision::None);
    Dnr::Rules two = built(3);
    two.AddRuleset(QStringLiteral("other"), Json(QJsonArray() << Rule(1, QStringLiteral("block"), cdn, 1)));
    QCOMPARE(Kind(two, under), Dnr::Decision::Block);
    Dnr::Request frame = Ask(QStringLiteral("https://cdn.example/frame.html"), Dnr::SubFrame);
    frame.frameAllows = navigation.frameAllows;
    QCOMPARE(Kind(rules, frame), Dnr::Decision::None);
    QCOMPARE(rules.Evaluate(Ask(QStringLiteral("https://site.example/inner"), Dnr::SubFrame)).frameAllows.value(QStringLiteral("ext")), allowAllOfFive);

    Dnr::Rules crowded;
    crowded.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("allow"), site, 9)
        << Rule(2, QStringLiteral("allowAllRequests"), site, 2)
        << Rule(3, QStringLiteral("allowAllRequests"), site, 6)));
    const Dnr::Decision won = crowded.Evaluate(Ask(QStringLiteral("https://site.example/page"), Dnr::MainFrame, QString()));
    QCOMPARE(won.kind, Dnr::Decision::None);
    QCOMPARE(won.frameAllows.value(QStringLiteral("ext")), (quint64(6) << 8) | 4);
    Dnr::Rules blocked;
    blocked.AddRuleset(QStringLiteral("ext"), Json(QJsonArray()
        << Rule(1, QStringLiteral("block"), site, 9)
        << Rule(2, QStringLiteral("allowAllRequests"), site, 5)));
    const Dnr::Decision refused = blocked.Evaluate(Ask(QStringLiteral("https://site.example/page"), Dnr::MainFrame, QString()));
    QCOMPARE(refused.kind, Dnr::Decision::Block);
    QCOMPARE(refused.frameAllows.value(QStringLiteral("ext")), allowAllOfFive);

    Dnr::Request theirs = Ask(QStringLiteral("https://cdn.example/a.js"), Dnr::Script, QStringLiteral("chrome-extension://abc/"));
    theirs.frameAllows = navigation.frameAllows;
    QCOMPARE(Kind(rules, theirs), Dnr::Decision::None);
    QVERIFY(rules.Evaluate(theirs).frameAllows.isEmpty());
}

void tst_dnrrules::theDocumentsAreRememberedByAddressAndGeneration(){
    QCOMPARE(Dnr::Framed::KeyOf(QUrl(QStringLiteral("https://Site.example/p?q=1#frag"))), QStringLiteral("https://site.example/p?q=1"));
    QCOMPARE(Dnr::Framed::KeyOf(QUrl(QStringLiteral("https://site.example/p"))), Dnr::Framed::KeyOf(QUrl(QStringLiteral("https://site.example/p#other"))));
    QVERIFY(Dnr::Framed::KeyOf(QUrl()).isEmpty());

    Dnr::Framed framed(2);
    QHash<QString, quint64> a; a.insert(QStringLiteral("ext"), 5);
    QHash<QString, quint64> got;
    QVERIFY(!framed.Find(1, QStringLiteral("k1"), &got));
    framed.Put(1, QStringLiteral("k1"), a);
    QVERIFY(framed.Find(1, QStringLiteral("k1"), &got));
    QCOMPARE(got.value(QStringLiteral("ext")), quint64(5));
    QVERIFY(!framed.Find(2, QStringLiteral("k1"), &got));
    framed.Put(2, QStringLiteral("k2"), QHash<QString, quint64>());
    QVERIFY(!framed.Find(2, QStringLiteral("k1"), &got));
    QVERIFY(framed.Find(2, QStringLiteral("k2"), &got));
    QVERIFY(got.isEmpty());
    QCOMPARE(framed.Count(), 1);
    framed.Put(2, QStringLiteral("k3"), a);
    QVERIFY(framed.Find(2, QStringLiteral("k2"), &got));
    framed.Put(2, QStringLiteral("k4"), a);
    QVERIFY(framed.Find(2, QStringLiteral("k2"), &got));
    QVERIFY(!framed.Find(2, QStringLiteral("k3"), &got));
    QVERIFY(framed.Find(2, QStringLiteral("k4"), &got));
    QCOMPARE(framed.Count(), 2);
    framed.Put(2, QString(), a);
    framed.Put(2, QStringLiteral("k2"), a);
    QCOMPARE(framed.Count(), 2);
    QVERIFY(framed.Find(2, QStringLiteral("k2"), &got));
    QCOMPARE(got.value(QStringLiteral("ext")), quint64(5));
    framed.Put(2, QStringLiteral("k5"), a);
    QVERIFY(framed.Find(2, QStringLiteral("k2"), &got));
    QVERIFY(!framed.Find(2, QStringLiteral("k4"), &got));
}

void tst_dnrrules::anEmptySetAnswersNothing(){
    Dnr::Rules rules;
    QVERIFY(rules.IsEmpty());
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://ads.example/x.js"))), Dnr::Decision::None);
}

namespace {
    QJsonValue Opts(const char *text){
        return QJsonDocument::fromJson(QByteArray(text)).object()[QStringLiteral("v")];
    }
    QString Ids(const QJsonArray &rules){
        QStringList ids;
        for(const QJsonValue &rule : rules) ids << QString::number(rule.toObject()[QStringLiteral("id")].toInt());
        return ids.join(QLatin1Char(','));
    }
    QJsonArray Many(int count, bool regex){
        QJsonArray rules;
        for(int i = 1; i <= count; i++){
            QJsonObject condition;
            condition[regex ? QStringLiteral("regexFilter") : QStringLiteral("urlFilter")] = QStringLiteral("x") + QString::number(i);
            QJsonObject rule;
            rule[QStringLiteral("id")] = i;
            rule[QStringLiteral("action")] = QJsonObject{{QStringLiteral("type"), QStringLiteral("block")}};
            rule[QStringLiteral("condition")] = condition;
            rules.append(rule);
        }
        return rules;
    }
    const char RULE_1[] = "{\"id\":1,\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"a\"}}";
}

void tst_dnrrules::heldRulesChangeAllOrNothing(){
    Dnr::Held held;
    const QString v = QStringLiteral("1.0");
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Dynamic, Opts(
        "{\"v\":{\"addRules\":[{\"id\":1,\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"a\"}},"
        "                      {\"id\":2,\"action\":{\"type\":\"allow\"},\"condition\":{}}]}}"), v), QString());
    QCOMPARE(Ids(held.Get(QStringLiteral("e"), Dnr::Held::Dynamic, QJsonValue(), v)), QStringLiteral("1,2"));

    const char *refused[] = {
        "{\"v\":{\"addRules\":[{\"id\":3,\"action\":{\"type\":\"block\"},\"condition\":{}},{\"id\":2,\"action\":{\"type\":\"block\"},\"condition\":{}}]}}",
        "{\"v\":{\"addRules\":[{\"id\":4,\"action\":{\"type\":\"block\"},\"condition\":{}},{\"id\":4,\"action\":{\"type\":\"block\"},\"condition\":{}}]}}",
        "{\"v\":{\"addRules\":[{\"id\":0,\"action\":{\"type\":\"block\"},\"condition\":{}}]}}",
        "{\"v\":{\"addRules\":[{\"id\":1.5,\"action\":{\"type\":\"block\"},\"condition\":{}}]}}",
        "{\"v\":{\"addRules\":[{\"id\":5,\"action\":{\"type\":\"erase\"},\"condition\":{}}]}}",
        "{\"v\":{\"addRules\":[{\"id\":5,\"action\":{\"type\":\"block\"}}]}}",
        "{\"v\":{\"addRules\":[7]}}",
        "{\"v\":[1]}",
        "{\"v\":{\"addRules\":{}}}",
        "{\"v\":{\"removeRuleIds\":[\"1\"]}}",
    };
    for(const char *one : refused){
        QVERIFY2(!held.Update(QStringLiteral("e"), Dnr::Held::Dynamic, Opts(one), v).isEmpty(), one);
        QCOMPARE(Ids(held.Get(QStringLiteral("e"), Dnr::Held::Dynamic, QJsonValue(), v)), QStringLiteral("1,2"));
    }
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Dynamic, Opts(
        "{\"v\":{\"removeRuleIds\":[2,99],\"addRules\":[{\"id\":2,\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"b\"}}]}}"), v), QString());
    QCOMPARE(held.Get(QStringLiteral("e"), Dnr::Held::Dynamic, QJsonValue(), v).at(1).toObject()
             [QStringLiteral("condition")].toObject()[QStringLiteral("urlFilter")].toString(), QStringLiteral("b"));
    QCOMPARE(Ids(held.Get(QStringLiteral("e"), Dnr::Held::Dynamic, Opts("{\"v\":{\"ruleIds\":[2,5]}}"), v)), QStringLiteral("2"));
    QCOMPARE(held.Get(QStringLiteral("f"), Dnr::Held::Dynamic, QJsonValue(), v).size(), 0);
}

void tst_dnrrules::heldRulesAreBoundedAsInChrome(){
    Dnr::Held held;
    const QString v = QStringLiteral("1");
    QJsonObject options;
    options[QStringLiteral("addRules")] = Many(Dnr::Held::SESSION_LIMIT, false);
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Session, options, v), QString());
    options[QStringLiteral("addRules")] = Many(Dnr::Held::SESSION_LIMIT + 1, false);
    QJsonArray every;
    for(int i = 1; i <= Dnr::Held::SESSION_LIMIT; i++) every.append(i);
    options[QStringLiteral("removeRuleIds")] = every;
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Session, options, v), QStringLiteral("Session rule count exceeded."));
    QCOMPARE(held.Get(QStringLiteral("e"), Dnr::Held::Session, QJsonValue(), v).size(), Dnr::Held::SESSION_LIMIT);

    options = QJsonObject();
    options[QStringLiteral("addRules")] = Many(Dnr::Held::DYNAMIC_LIMIT + 1, false);
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Dynamic, options, v), QStringLiteral("Dynamic rule count exceeded."));

    Dnr::Held regex;
    options[QStringLiteral("addRules")] = Many(600, true);
    QCOMPARE(regex.Update(QStringLiteral("e"), Dnr::Held::Dynamic, options, v), QString());
    options[QStringLiteral("addRules")] = Many(401, true);
    QCOMPARE(regex.Update(QStringLiteral("e"), Dnr::Held::Session, options, v), QStringLiteral("Regular expression rule count exceeded."));
    options[QStringLiteral("addRules")] = Many(400, true);
    QCOMPARE(regex.Update(QStringLiteral("e"), Dnr::Held::Session, options, v), QString());

    Dnr::Held big;
    QJsonArray one = Many(1, false);
    QJsonObject rule = one.at(0).toObject();
    rule[QStringLiteral("condition")] = QJsonObject{{QStringLiteral("urlFilter"), QString(Dnr::Held::BYTES_LIMIT, QLatin1Char('x'))}};
    options[QStringLiteral("addRules")] = QJsonArray() << rule;
    QCOMPARE(big.Update(QStringLiteral("e"), Dnr::Held::Dynamic, options, v), QStringLiteral("The rules are too large."));
    QVERIFY(!big.Holds(QStringLiteral("e")));
}

void tst_dnrrules::sessionRulesGoWithTheVersionAndAreNeverKept(){
    Dnr::Held held;
    const QJsonValue one = Opts((QByteArray("{\"v\":{\"addRules\":[") + RULE_1 + "]}}").constData());
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Session, one, QStringLiteral("1")), QString());
    QCOMPARE(held.Get(QStringLiteral("e"), Dnr::Held::Session, QJsonValue(), QStringLiteral("1")).size(), 1);
    QCOMPARE(held.Get(QStringLiteral("e"), Dnr::Held::Session, QJsonValue(), QStringLiteral("2")).size(), 0);
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Session, one, QStringLiteral("2")), QString());
    QCOMPARE(held.Get(QStringLiteral("e"), Dnr::Held::Session, QJsonValue(), QStringLiteral("2")).size(), 1);
    QVERIFY(held.ToJson().isEmpty());
    held.KeepSessionsOf(QSet<QString>() << QStringLiteral("other"));
    QCOMPARE(held.Get(QStringLiteral("e"), Dnr::Held::Session, QJsonValue(), QStringLiteral("2")).size(), 0);
    QVERIFY(!held.Holds(QStringLiteral("e")));
}

void tst_dnrrules::theEnabledRulesetsAreTheManifestsUntilChosen(){
    QList<Dnr::Held::Ruleset> manifest;
    const char *names[] = { "a", "b", "c", "d" };
    for(int i = 0; i < 4; i++){
        Dnr::Held::Ruleset ruleset;
        ruleset.id = QString::fromLatin1(names[i]);
        ruleset.enabled = i < 2;
        manifest << ruleset;
    }
    Dnr::Held held;
    const QString v = QStringLiteral("1");
    QCOMPARE(held.Enabled(QStringLiteral("e"), manifest, v).join(QLatin1Char(',')), QStringLiteral("a,b"));
    QCOMPARE(held.UpdateEnabled(QStringLiteral("e"), Opts("{\"v\":{\"enableRulesetIds\":[\"d\",\"a\"],\"disableRulesetIds\":[\"a\",\"b\"]}}"), manifest, v), QString());
    QCOMPARE(held.Enabled(QStringLiteral("e"), manifest, v).join(QLatin1Char(',')), QStringLiteral("a,d"));
    QCOMPARE(held.UpdateEnabled(QStringLiteral("e"), Opts("{\"v\":{\"enableRulesetIds\":[\"c\",\"z\"]}}"), manifest, v), QStringLiteral("Invalid ruleset id: z."));
    QCOMPARE(held.Enabled(QStringLiteral("e"), manifest, v).join(QLatin1Char(',')), QStringLiteral("a,d"));
    QCOMPARE(held.Enabled(QStringLiteral("e"), manifest, QStringLiteral("2")).join(QLatin1Char(',')), QStringLiteral("a,b"));

    QList<Dnr::Held::Ruleset> big;
    QJsonArray all;
    for(int i = 0; i <= Dnr::Held::ENABLED_LIMIT; i++){
        Dnr::Held::Ruleset ruleset;
        ruleset.id = QString::number(i);
        ruleset.enabled = false;
        big << ruleset;
        all.append(ruleset.id);
    }
    QJsonObject options;
    options[QStringLiteral("enableRulesetIds")] = all;
    QCOMPARE(held.UpdateEnabled(QStringLiteral("big"), options, big, v), QStringLiteral("The number of enabled static rulesets exceeds the limit."));
    QCOMPARE(held.Enabled(QStringLiteral("big"), big, v).size(), 0);
}

void tst_dnrrules::whatIsHeldReadsBackAndGoesWithTheExtension(){
    QList<Dnr::Held::Ruleset> manifest;
    Dnr::Held::Ruleset a; a.id = QStringLiteral("a"); a.enabled = true;
    Dnr::Held::Ruleset b; b.id = QStringLiteral("b"); b.enabled = false;
    manifest << a << b;
    Dnr::Held held;
    const QString v = QStringLiteral("1");
    QCOMPARE(held.Update(QStringLiteral("e"), Dnr::Held::Dynamic, Opts((QByteArray("{\"v\":{\"addRules\":[") + RULE_1 + "]}}").constData()), v), QString());
    QCOMPARE(held.UpdateEnabled(QStringLiteral("f"), Opts("{\"v\":{\"enableRulesetIds\":[\"b\"]}}"), manifest, v), QString());

    bool damaged = true;
    const Dnr::Held back = Dnr::Held::FromJson(held.ToJson(), &damaged);
    QVERIFY(!damaged);
    QCOMPARE(Ids(back.Get(QStringLiteral("e"), Dnr::Held::Dynamic, QJsonValue(), QStringLiteral("9"))), QStringLiteral("1"));
    QCOMPARE(back.Enabled(QStringLiteral("f"), manifest, v).join(QLatin1Char(',')), QStringLiteral("a,b"));

    const Dnr::Held broken = Dnr::Held::FromJson(QJsonDocument::fromJson(
        "{\"e\":{\"dynamic\":{}},\"f\":7,\"g\":{\"enabled\":[\"a\"],\"enabledVersion\":\"1\"}}").object(), &damaged);
    QVERIFY(damaged);
    QVERIFY(!broken.Holds(QStringLiteral("e")));
    QCOMPARE(broken.Enabled(QStringLiteral("g"), manifest, v).join(QLatin1Char(',')), QStringLiteral("a"));

    held.Forget(QStringLiteral("e"));
    QVERIFY(!held.Holds(QStringLiteral("e")));
    QVERIFY(!held.ToJson().contains(QStringLiteral("e")));

    const QByteArray written = "[{\"id\":9,\"condition\":{\"urlFilter\":\"z\"},\"action\":{\"type\":\"allow\"}},"
                               "{\"id\":3,\"action\":{\"type\":\"block\"},\"condition\":{}}]";
    QJsonObject options;
    options[QStringLiteral("addRules")] = QJsonDocument::fromJson(written).array();
    QCOMPARE(held.Update(QStringLiteral("g"), Dnr::Held::Dynamic, options, v), QString());
    QCOMPARE(held.Get(QStringLiteral("g"), Dnr::Held::Dynamic, QJsonValue(), v), QJsonDocument::fromJson(written).array());

    QCOMPARE(held.Update(QStringLiteral("f"), Dnr::Held::Session, Opts((QByteArray("{\"v\":{\"addRules\":[") + RULE_1 + "]}}").constData()), v), QString());
    QVERIFY(!held.Settle(QStringLiteral("f"), v));
    QVERIFY(held.Settle(QStringLiteral("f"), QStringLiteral("2")));
    QVERIFY(!held.Holds(QStringLiteral("f")));
    QVERIFY(!held.Settle(QStringLiteral("g"), QStringLiteral("2")));
    QCOMPARE(held.Get(QStringLiteral("g"), Dnr::Held::Dynamic, QJsonValue(), QStringLiteral("2")).size(), 2);
}

void tst_dnrrules::aRealSizedSetIsAnsweredQuickly(){
    QJsonArray array;
    int id = 1;
    for(int i = 0; i < 17600; i++)
        array << Rule(id++, QStringLiteral("block"),
                      Filter(QStringLiteral("||ads%1.network%2.example^").arg(i).arg(i % 97)));
    for(int i = 0; i < 1200; i++)
        array << Rule(id++, QStringLiteral("block"),
                      Filter(QStringLiteral("/track/%1/pixel*.gif").arg(i)));
    QJsonArray hosts;
    for(int i = 0; i < 50000; i++) hosts << QStringLiteral("host%1.blocklist.example").arg(i);
    QJsonObject listed;
    listed[QStringLiteral("requestDomains")] = hosts;
    array << Rule(id++, QStringLiteral("block"), listed);

    Dnr::Rules rules;
    QElapsedTimer timer;
    timer.start();
    rules.AddRuleset(QStringLiteral("ext"), Json(array));
    const qint64 built = timer.elapsed();
    QCOMPARE(rules.Count(), 18801);

    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://ads4242.network71.example/x.js"))), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://cdn.example/track/77/pixel-1.gif"), Dnr::Image)), Dnr::Decision::Block);
    QCOMPARE(Kind(rules, Ask(QStringLiteral("https://a.host49999.blocklist.example/x.js"))), Dnr::Decision::Block);

    const int rounds = 2000;
    auto perRequest = [&](const Dnr::Rules &set, int *blocked){
        QElapsedTimer t;
        t.start();
        for(int i = 0; i < rounds; i++){
            const Dnr::Request request = Ask(QStringLiteral("https://static%1.site%2.example/assets/app.%1.js?v=%2")
                                             .arg(i).arg(i % 13));
            if(Kind(set, request) == Dnr::Decision::Block) (*blocked)++;
        }
        return double(t.nsecsElapsed()) / 1e6 / rounds;
    };
    Dnr::Rules few;
    QJsonArray ten;
    for(int i = 0; i < 10; i++) ten << Rule(i + 1, QStringLiteral("block"), Filter(QStringLiteral("||few%1.example^").arg(i)));
    few.AddRuleset(QStringLiteral("ext"), Json(ten));

    int blocked = 0, blockedByFew = 0;
    const double baselineMs = perRequest(few, &blockedByFew);
    const double perRequestMs = perRequest(rules, &blocked);
    QCOMPARE(blocked, 0);
    QCOMPARE(blockedByFew, 0);

    QVERIFY2(perRequestMs < baselineMs * 8 + 0.02,
             qPrintable(QStringLiteral("%1 ms per request against %2 ms for ten rules (built in %3 ms)")
                        .arg(perRequestMs).arg(baselineMs).arg(built)));
}

void tst_dnrrules::measureARealExtension(){
    const QString root = qEnvironmentVariable("VANILLA_DNR_EXTENSION");
    if(root.isEmpty()) QSKIP("VANILLA_DNR_EXTENSION is not set");

    QFile manifestFile(root + QStringLiteral("/manifest.json"));
    QVERIFY(manifestFile.open(QIODevice::ReadOnly));
    const QJsonObject manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    const QJsonArray resources = manifest[QStringLiteral("declarative_net_request")].toObject()
        [QStringLiteral("rule_resources")].toArray();

    Dnr::Rules rules, regexOnly, plainOnly;
    Dnr::Skipped skipped;
    QElapsedTimer timer;
    timer.start();
    qint64 bytes = 0;
    foreach(const QJsonValue &value, resources){
        QFile file(root + QLatin1Char('/') + value.toObject()[QStringLiteral("path")].toString());
        if(!file.open(QIODevice::ReadOnly)) continue;
        const QByteArray json = file.readAll();
        bytes += json.size();
        rules.AddRuleset(QStringLiteral("real"), json, &skipped);
        QJsonArray withRegex, without;
        foreach(const QJsonValue &rule, QJsonDocument::fromJson(json).array()){
            if(rule.toObject()[QStringLiteral("condition")].toObject().contains(QStringLiteral("regexFilter"))) withRegex << rule;
            else without << rule;
        }
        regexOnly.AddRuleset(QStringLiteral("real"), Json(withRegex));
        plainOnly.AddRuleset(QStringLiteral("real"), Json(without));
    }
    const qint64 built = timer.elapsed();

    const int rounds = 5000;
    timer.restart();
    int acted = 0;
    for(int i = 0; i < rounds; i++){
        const Dnr::Request request = Ask(QStringLiteral("https://static%1.site%2.example/assets/app.%1.js?v=%2")
                                         .arg(i).arg(i % 13));
        if(Kind(rules, request) != Dnr::Decision::None) acted++;
    }
    const double perRequestMs = double(timer.nsecsElapsed()) / 1e6 / rounds;

    auto perRequest = [&](const Dnr::Rules &set){
        QElapsedTimer t; t.start();
        for(int i = 0; i < rounds; i++)
            Kind(set, Ask(QStringLiteral("https://static%1.site%2.example/assets/app.%1.js?v=%2").arg(i).arg(i % 13)));
        return double(t.nsecsElapsed()) / 1e6 / rounds;
    };
    const double regexMs = perRequest(regexOnly), plainMs = perRequest(plainOnly), emptyMs = perRequest(Dnr::Rules());
    Q_UNUSED(emptyMs)
    QElapsedTimer longTimer; longTimer.start();
    const QString longUrl = QStringLiteral("https://104.154.1.2/") + QString(4000, QLatin1Char('a')) + QStringLiteral("?q=") + QString(2000, QLatin1Char('b'));
    for(int i = 0; i < 200; i++) Kind(rules, Ask(longUrl, Dnr::Script));
    const double longMs = double(longTimer.nsecsElapsed()) / 1e6 / 200;

    const Dnr::Decision ad = rules.Evaluate(Ask(QStringLiteral("https://securepubads.g.doubleclick.net/tag/js/gpt.js")));
    QSKIP(qPrintable(QStringLiteral("%1 rules from %2 bytes in %3 ms; skipped %4 (headers %5, computed %6, tabs %7, regex %8, malformed %9); "
                                    "%10 ms per unmatched request (%11 of %12 acted on); doubleclick gpt.js -> kind %13; regex rules alone %14 ms, the rest %15 ms; a 6,000 character address %16 ms")
                     .arg(rules.Count()).arg(bytes).arg(built).arg(skipped.Total()).arg(skipped.modifyHeaders)
                     .arg(skipped.computedRedirect).arg(skipped.tabBound).arg(skipped.badRegex).arg(skipped.malformed)
                     .arg(perRequestMs, 0, 'f', 4).arg(acted).arg(rounds).arg(int(ad.kind)).arg(regexMs, 0, 'f', 4).arg(plainMs, 0, 'f', 4).arg(longMs, 0, 'f', 4)));
}

QTEST_MAIN(tst_dnrrules)
#include "tst_dnrrules.moc"
