#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QStringList>
#include <QRegularExpression>
#include <QFile>
#include <QDir>

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

    QVERIFY2(source.contains(QStringLiteral("m_Profile->setUrlRequestInterceptor")),
             "the widget profiles no longer get the interceptor");
    QVERIFY2(source.contains(QStringLiteral("profile->setUrlRequestInterceptor")),
             "the quick profiles no longer get the interceptor");
    QCOMPARE(source.count(QStringLiteral("ApplyQuickBlockRules(profile)")), 2);

    QCOMPARE(source.count(QStringLiteral("RequestInterceptor::Instance()")),
             3);
}

QTEST_MAIN(tst_urlblock)
#include "tst_urlblock.moc"
