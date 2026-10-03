#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QStringList>
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>

#include "networkcontroller.hpp"

#include "testsupport.hpp"

class tst_urlblock : public QObject {
    Q_OBJECT

private slots:
    void aBareHostCatchesTheWholeAddress();
    void starsMatchWhatTheyLookLikeTheyDo();
    void theMatchIsCaseInsensitive();
    void blankLinesAndCommentsAreNotRules();
    void anAddressNoRuleMentionsIsNotBlocked();
    void anEmptyListBlocksNothing();
    void aRuleMatchesTheQueryAsWellAsTheHost();
    void everyEngineProfileIsGivenTheInterceptor();
    void extensionRulesAreAskedOfOrdinaryEngineProfilesOnly();
    void theExtensionRulesInForceAreTheLastOnesAskedFor();
    void whatAnExtensionPutInIsInForceAheadOfItsFiles();
    void everyKindOfRequestHasItsWordInARule();
};

void tst_urlblock::aBareHostCatchesTheWholeAddress(){
    const QList<QRegularExpression> rules = UrlBlockRules::Compile
        (QStringList() << QStringLiteral("doubleclick.net"));

    QVERIFY(UrlBlockRules::Matches
            (rules, QStringLiteral("https://ad.doubleclick.net/x.js")));
    QVERIFY(UrlBlockRules::Matches
            (rules, QStringLiteral("http://doubleclick.net/")));
    QVERIFY(!UrlBlockRules::Matches
            (rules, QStringLiteral("https://example.com/doubleclick")));
}

void tst_urlblock::starsMatchWhatTheyLookLikeTheyDo(){
    const QList<QRegularExpression> rules = UrlBlockRules::Compile
        (QStringList() << QStringLiteral("*.example.com/ads/*"));

    QVERIFY(UrlBlockRules::Matches
            (rules, QStringLiteral("https://a.example.com/ads/one.png")));
    QVERIFY(!UrlBlockRules::Matches
            (rules, QStringLiteral("https://a.example.com/news/one.png")));
}

void tst_urlblock::theMatchIsCaseInsensitive(){
    const QList<QRegularExpression> rules = UrlBlockRules::Compile
        (QStringList() << QStringLiteral("tracker.example"));

    QVERIFY(UrlBlockRules::Matches
            (rules, QStringLiteral("https://TRACKER.EXAMPLE/beacon")));
}

void tst_urlblock::blankLinesAndCommentsAreNotRules(){
    const QList<QRegularExpression> rules = UrlBlockRules::Compile
        (QStringList()
         << QString()
         << QStringLiteral("   ")
         << QStringLiteral("# tracker.example")
         << QStringLiteral("  # spaced.example  ")
         << QStringLiteral("  real.example  "));

    QCOMPARE(rules.length(), 1);
    QVERIFY(UrlBlockRules::Matches(rules, QStringLiteral("https://real.example/a")));
    QVERIFY(!UrlBlockRules::Matches(rules, QStringLiteral("https://tracker.example/a")));
    QVERIFY(!UrlBlockRules::Matches(rules, QStringLiteral("https://spaced.example/a")));
}

void tst_urlblock::anAddressNoRuleMentionsIsNotBlocked(){
    const QList<QRegularExpression> rules = UrlBlockRules::Compile
        (QStringList() << QStringLiteral("tracker.example")
                       << QStringLiteral("*.ads.example"));

    QVERIFY(!UrlBlockRules::Matches(rules, QStringLiteral("https://example.com/")));
}

void tst_urlblock::anEmptyListBlocksNothing(){
    QVERIFY(UrlBlockRules::Compile(QStringList()).isEmpty());
    QVERIFY(!UrlBlockRules::Matches(QList<QRegularExpression>(),
                                    QStringLiteral("https://example.com/")));
}

void tst_urlblock::aRuleMatchesTheQueryAsWellAsTheHost(){
    const QList<QRegularExpression> rules = UrlBlockRules::Compile
        (QStringList() << QStringLiteral("*utm_source=*"));

    QVERIFY(UrlBlockRules::Matches
            (rules, QStringLiteral("https://example.com/a?utm_source=x")));
    QVERIFY(!UrlBlockRules::Matches
            (rules, QStringLiteral("https://example.com/a?q=x")));
}

void tst_urlblock::everyEngineProfileIsGivenTheInterceptor(){
    QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
               QStringLiteral("/app/networkcontroller.cpp"));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             "'networkcontroller.cpp' was not read; check VANILLA_SOURCE_DIR");
    const QString source =
        QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

    QVERIFY2(source.count(QStringLiteral("profile->setUrlRequestInterceptor")) >= 2,
             "the widget or the quick profiles no longer get the interceptor");
    QCOMPARE(source.count(QStringLiteral("ApplyQuickBlockRules(profile)")), 3);

    QCOMPARE(source.count(QStringLiteral("setUrlRequestInterceptor(")), 2);
    QCOMPARE(source.count(QStringLiteral
                 ("profile->setUrlRequestInterceptor(RequestInterceptor::For(profile->isOffTheRecord()))")), 2);
}

void tst_urlblock::extensionRulesAreAskedOfOrdinaryEngineProfilesOnly(){
    const QString root = QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR));
    QFile file(root + QStringLiteral("/app/networkcontroller.cpp"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

    QVERIFY2(source.contains(QStringLiteral("if(m_AsksExtensionRules) AskExtensionRules(info);")),
             "the interceptor asks whatever profile it serves");
    const int privateInstance = source.indexOf(QStringLiteral("RequestInterceptor::PrivateInstance(){"));
    QVERIFY(privateInstance >= 0);
    const QString body = source.mid(privateInstance, 400);
    QVERIFY2(body.contains(QStringLiteral("made->m_AsksExtensionRules = false;")),
             "the private interceptor asks the extension rules");

    QDir edge(root + QStringLiteral("/view/edge"));
    const QStringList files = edge.entryList(QStringList() << QStringLiteral("*.cpp") << QStringLiteral("*.hpp"));
    QVERIFY(!files.isEmpty());
    foreach(const QString &name, files){
        QFile each(edge.filePath(name));
        QVERIFY(each.open(QIODevice::ReadOnly));
        QVERIFY2(!each.readAll().contains("ExtensionNetRules"),
                 qPrintable(name + QStringLiteral(" asks the extension rules; WebView2 already does")));
    }
}

void tst_urlblock::theExtensionRulesInForceAreTheLastOnesAskedFor(){
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto write = [&](const QString &name, const QByteArray &json){
        QFile file(dir.filePath(name));
        if(!file.open(QIODevice::WriteOnly)) return QString();
        file.write(json);
        return file.fileName();
    };
    auto asked = [](const QString &url){
        Dnr::Request request;
        request.url = QUrl(url);
        request.initiator = QUrl(QStringLiteral("https://page.example/"));
        request.type = Dnr::Script;
        request.method = "GET";
        return ExtensionNetRules::Current()->Evaluate(request).kind;
    };

    QByteArray large = "[";
    for(int i = 0; i < 60000; i++){
        if(i) large += ',';
        large += "{\"id\":" + QByteArray::number(i + 1)
               + ",\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"||slow"
               + QByteArray::number(i) + ".example^\"}}";
    }
    large += "]";
    const QString slow = write(QStringLiteral("slow.json"), large);
    const QString quick = write(QStringLiteral("quick.json"),
        "[{\"id\":1,\"action\":{\"type\":\"block\"},\"condition\":{\"urlFilter\":\"||quick.example^\"}}]");
    QVERIFY(!slow.isEmpty() && !quick.isEmpty());

    ExtensionNetRules::Source first;  first.id = QStringLiteral("one");  first.files << slow;
    ExtensionNetRules::Source second; second.id = QStringLiteral("two"); second.files << quick;

    QVERIFY(ExtensionNetRules::Current());

    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << first, true);
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << second, false);
    ExtensionNetRules::WaitForLoads();
    QCOMPARE(asked(QStringLiteral("https://quick.example/a.js")), Dnr::Decision::Block);
    QCOMPARE(asked(QStringLiteral("https://slow7.example/a.js")), Dnr::Decision::None);
    QCOMPARE(ExtensionNetRules::Current()->Count(), 1);

    const QSharedPointer<const Dnr::Rules> before = ExtensionNetRules::Current();
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << second, false);
    QCOMPARE(ExtensionNetRules::Current().data(), before.data());

    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>(), false);
    QVERIFY(ExtensionNetRules::Current()->IsEmpty());
    QCOMPARE(asked(QStringLiteral("https://quick.example/a.js")), Dnr::Decision::None);
}

void tst_urlblock::whatAnExtensionPutInIsInForceAheadOfItsFiles(){
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto redirect = [](const char *to){
        return QByteArray("[{\"id\":1,\"action\":{\"type\":\"redirect\",\"redirect\":{\"url\":\"https://") + to
             + ".example/\"}},\"condition\":{\"urlFilter\":\"||ads.example^\"}}]";
    };
    QFile file(dir.filePath("static.json"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(redirect("file"));
    file.close();
    auto whereTo = [](){
        Dnr::Request request;
        request.url = QUrl(QStringLiteral("https://ads.example/a.js"));
        request.initiator = QUrl(QStringLiteral("https://page.example/"));
        request.type = Dnr::Script;
        request.method = "GET";
        return ExtensionNetRules::Current()->Evaluate(request).redirect.host();
    };
    ExtensionNetRules::Source source;
    source.id = QStringLiteral("one");
    source.files << file.fileName();
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << source, false);
    QCOMPARE(whereTo(), QStringLiteral("file.example"));
    source.dynamic = redirect("dynamic");
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << source, false);
    QCOMPARE(whereTo(), QStringLiteral("dynamic.example"));
    source.session = redirect("session");
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << source, false);
    QCOMPARE(whereTo(), QStringLiteral("session.example"));
    source.files.clear();
    source.session.clear();
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>() << source, false);
    QCOMPARE(whereTo(), QStringLiteral("dynamic.example"));
    ExtensionNetRules::Load(QList<ExtensionNetRules::Source>(), false);
}

void tst_urlblock::everyKindOfRequestHasItsWordInARule(){
#ifdef WEBENGINEVIEW
    typedef QWebEngineUrlRequestInfo Info;
    const struct { Info::ResourceType engine; Dnr::ResourceType rule; } table[] = {
        { Info::ResourceTypeMainFrame, Dnr::MainFrame },
        { Info::ResourceTypeSubFrame, Dnr::SubFrame },
        { Info::ResourceTypeStylesheet, Dnr::Stylesheet },
        { Info::ResourceTypeScript, Dnr::Script },
        { Info::ResourceTypeImage, Dnr::Image },
        { Info::ResourceTypeFontResource, Dnr::Font },
        { Info::ResourceTypeSubResource, Dnr::Other },
        { Info::ResourceTypeObject, Dnr::Object },
        { Info::ResourceTypeMedia, Dnr::Media },
        { Info::ResourceTypeWorker, Dnr::Script },
        { Info::ResourceTypeSharedWorker, Dnr::Script },
        { Info::ResourceTypePrefetch, Dnr::Other },
        { Info::ResourceTypeFavicon, Dnr::Image },
        { Info::ResourceTypeXhr, Dnr::XmlHttpRequest },
        { Info::ResourceTypePing, Dnr::Ping },
        { Info::ResourceTypeServiceWorker, Dnr::Script },
        { Info::ResourceTypeCspReport, Dnr::CspReport },
        { Info::ResourceTypePluginResource, Dnr::Object },
        { Info::ResourceTypeNavigationPreloadMainFrame, Dnr::MainFrame },
        { Info::ResourceTypeNavigationPreloadSubFrame, Dnr::SubFrame },
        { Info::ResourceTypeJson, Dnr::Script },
        { Info::ResourceTypeWebSocket, Dnr::WebSocket },
        { Info::ResourceTypeUnknown, Dnr::Other },
    };
    for(const auto &row : table)
        QCOMPARE(RequestInterceptor::DnrTypeOf(row.engine), row.rule);

    QVERIFY(RequestInterceptor::For(true) != RequestInterceptor::For(false));
    QCOMPARE(RequestInterceptor::For(false), RequestInterceptor::Instance());
    QCOMPARE(RequestInterceptor::For(true), RequestInterceptor::PrivateInstance());
#else
    QSKIP("no engine in this build");
#endif
}

QTEST_MAIN(tst_urlblock)
#include "tst_urlblock.moc"
