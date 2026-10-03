#include "switch.hpp"

#include <QtTest>

#include "page.hpp"

#include "testsupport.hpp"

class tst_searchengine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase(){ TestSupport::SilenceDebugOutput(); }
    void init(){ Page::ClearSearchEngine(); }
    void cleanupTestCase(){ Page::ClearSearchEngine(); Page::RegisterDefaultSearchEngines(); }

    void theOneMarkedPrimaryIsIt();
    void withoutAMarkGoogleIsItAndWithoutGoogleTheFirstIs();
    void choosingWritesNothingAndAsksTwiceTheSame();
    void withNoEngineNothingIsAnswered();
    void theAddressIsTheTemplateWithTheQueryEncodedIn();
};

void tst_searchengine::theOneMarkedPrimaryIsIt(){
    Page::RegisterSearchEngine(QStringLiteral("bing"), SearchEngine() << QStringLiteral("https://www.bing.com/search?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("true"));
    Page::RegisterSearchEngine(QStringLiteral("google"), SearchEngine() << QStringLiteral("https://www.google.com/search?q=%1") << QStringLiteral("UTF-8") << QStringLiteral("false"));
    QCOMPARE(Page::PrimarySearchEngine().at(0), QStringLiteral("https://www.bing.com/search?q=%1"));
}

void tst_searchengine::withoutAMarkGoogleIsItAndWithoutGoogleTheFirstIs(){
    Page::RegisterSearchEngine(QStringLiteral("yahoo"), SearchEngine() << QStringLiteral("https://search.yahoo.com/search?p=%1") << QStringLiteral("UTF-8") << QStringLiteral("false"));
    Page::RegisterSearchEngine(QStringLiteral("google"), SearchEngine() << QStringLiteral("https://www.google.com/search?q=%1") << QStringLiteral("UTF-8"));
    QCOMPARE(Page::PrimarySearchEngine().at(0), QStringLiteral("https://www.google.com/search?q=%1"));
    Page::RemoveSearchEngine(QStringLiteral("google"));
    QCOMPARE(Page::PrimarySearchEngine().at(0), QStringLiteral("https://search.yahoo.com/search?p=%1"));
}

void tst_searchengine::choosingWritesNothingAndAsksTwiceTheSame(){
    Page::RegisterSearchEngine(QStringLiteral("yahoo"), SearchEngine() << QStringLiteral("https://search.yahoo.com/search?p=%1") << QStringLiteral("UTF-8") << QStringLiteral("false"));
    const SearchEngine first = Page::PrimarySearchEngine();
    const SearchEngine second = Page::PrimarySearchEngine();
    QCOMPARE(first, second);
    QCOMPARE(first.size(), 3);
    QCOMPARE(first.at(0), QStringLiteral("https://search.yahoo.com/search?p=%1"));
    QCOMPARE(Page::GetSearchEngineMap().keys(), QStringList() << QStringLiteral("yahoo"));
    QCOMPARE(Page::CreateQueryUrl(QStringLiteral("a")), QUrl(QStringLiteral("https://search.yahoo.com/search?p=a")));
    Page::RegisterSearchEngine(QStringLiteral("short"), SearchEngine() << QStringLiteral("https://s.example/?q=%1"));
    Page::PrimarySearchEngine();
    QCOMPARE(Page::GetSearchEngineMap().value(QStringLiteral("short")).size(), 2);
    Page::PrimarySearchEngine();
    QCOMPARE(Page::GetSearchEngineMap().value(QStringLiteral("short")).size(), 3);
}

void tst_searchengine::withNoEngineNothingIsAnswered(){
    QVERIFY(Page::PrimarySearchEngine().isEmpty());
    QVERIFY(Page::GetSearchEngineMap().isEmpty());
    QVERIFY(Page::CreateQueryUrl(SearchEngine(), QStringLiteral("a")).isEmpty());
}

void tst_searchengine::theAddressIsTheTemplateWithTheQueryEncodedIn(){
    const SearchEngine engine = SearchEngine() << QStringLiteral("https://x.example/s?q=%1&x=1") << QStringLiteral("UTF-8") << QStringLiteral("true");
    QCOMPARE(Page::CreateQueryUrl(engine, QString::fromUtf8("a b&c#d=e/\u30d6")).toString(QUrl::FullyEncoded),
             QStringLiteral("https://x.example/s?q=a%20b%26c%23d%3De%2F%E3%83%96&x=1"));
    Page::RegisterSearchEngine(QStringLiteral("x"), engine);
    QCOMPARE(Page::CreateQueryUrl(QStringLiteral("a b")), QUrl(QStringLiteral("https://x.example/s?q=a%20b&x=1")));
    QCOMPARE(Page::CreateQueryUrl(QStringLiteral("a b"), QStringLiteral("x")), QUrl(QStringLiteral("https://x.example/s?q=a%20b&x=1")));
}

QTEST_MAIN(tst_searchengine)
#include "tst_searchengine.moc"
