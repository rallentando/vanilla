#include "switch.hpp"
#include "const.hpp"

#include <QtTest>

#include "loadending.hpp"

class tst_loadending : public QObject {
    Q_OBJECT

private slots:
    void aLoadWhichSucceededIsNotReported();
    void aLoadWhichWasStoppedTakesTheFailureBack();
    void aLoadWhichFailedIsReported();
    void aFailureWithNothingToAddIsLeftAsItWas();
    void everyOtherStatusIsNotAnEnding();

    void anHttpErrorNamesTheAddressItCouldNotFind();
    void anHttpErrorNamesTheHostWhichCouldNotAnswer();
    void anHttpErrorWithNoPlaceholderLosesOnlyItsTags();
    void aReasonFromAnotherDomainIsLeftAsItIs();

    void theEdgeStatusNumbersAreTheOnesMeasured();

    void aQuickLoadWhichWasStoppedEndsWithoutFailing();
    void aQuickLoadWhichFailedEndsAndSaysSo();
    void aQuickLoadWhichStartedIsNotAnEnding();
};

void tst_loadending::aLoadWhichWasStoppedTakesTheFailureBack(){
    QCOMPARE(LoadEnding::WebEngineEnding(LoadEnding::WebEngineStopped, 1),
             LoadEnding::Verdict::Clear);

    QCOMPARE(LoadEnding::WebEngineEnding(LoadEnding::WebEngineStopped,
                                         LoadEnding::WebEngineNoErrorDomain),
             LoadEnding::Verdict::Clear);
}

void tst_loadending::aLoadWhichSucceededIsNotReported(){
    QCOMPARE(LoadEnding::WebEngineEnding(LoadEnding::WebEngineSucceeded, 7),
             LoadEnding::Verdict::Nothing);
}

void tst_loadending::aLoadWhichFailedIsReported(){
    QCOMPARE(LoadEnding::WebEngineEnding(LoadEnding::WebEngineFailed, 7),
             LoadEnding::Verdict::Report);
}

void tst_loadending::aFailureWithNothingToAddIsLeftAsItWas(){
    QCOMPARE(LoadEnding::WebEngineEnding(LoadEnding::WebEngineFailed,
                                         LoadEnding::WebEngineNoErrorDomain),
             LoadEnding::Verdict::Nothing);
}

void tst_loadending::everyOtherStatusIsNotAnEnding(){
    QCOMPARE(LoadEnding::WebEngineEnding(LoadEnding::WebEngineStarted, 0),
             LoadEnding::Verdict::Nothing);
}

void tst_loadending::anHttpErrorNamesTheAddressItCouldNotFind(){
    const QUrl url(QStringLiteral("https://example.com/a?b=<c>"));
    const QString address = url.toString();

    QCOMPARE(LoadEnding::WebEngineErrorText
             (QStringLiteral("No webpage was found for the web address: <strong>$1</strong>"),
              LoadEnding::WebEngineHttpStatusCodeDomain, 404, url),
             QStringLiteral("No webpage was found for the web address: ") + address);

    QCOMPARE(LoadEnding::WebEngineErrorText
             (QStringLiteral("次の URL のウェブページは見つかりませんでした:<strong>$1</strong>"),
              LoadEnding::WebEngineHttpStatusCodeDomain, 404, url),
             QStringLiteral("次の URL のウェブページは見つかりませんでした:") + address);
}

void tst_loadending::anHttpErrorNamesTheHostWhichCouldNotAnswer(){
    const QUrl url(QStringLiteral("https://example.com/a"));

    QCOMPARE(LoadEnding::WebEngineErrorText
             (QStringLiteral("<strong>$1</strong> is currently unable to handle this request."),
              LoadEnding::WebEngineHttpStatusCodeDomain, 503, url),
             QStringLiteral("example.com is currently unable to handle this request."));

    QCOMPARE(LoadEnding::WebEngineErrorText
             (QStringLiteral("<strong>$1</strong> では現在このリクエストを処理できません。"),
              LoadEnding::WebEngineHttpStatusCodeDomain, 500, url),
             QStringLiteral("example.com では現在このリクエストを処理できません。"));
}

void tst_loadending::anHttpErrorWithNoPlaceholderLosesOnlyItsTags(){
    const QUrl url(QStringLiteral("https://example.com/a"));
    QCOMPARE(LoadEnding::WebEngineErrorText
             (QStringLiteral("You don't have authorization to view this page."),
              LoadEnding::WebEngineHttpStatusCodeDomain, 403, url),
             QStringLiteral("You don't have authorization to view this page."));
}

void tst_loadending::aReasonFromAnotherDomainIsLeftAsItIs(){
    const QString reason = QStringLiteral("net::ERR_NAME_NOT_RESOLVED <$1>");
    QCOMPARE(LoadEnding::WebEngineErrorText
             (reason, 2, -105, QUrl(QStringLiteral("https://example.com/"))),
             reason);
}

void tst_loadending::theEdgeStatusNumbersAreTheOnesMeasured(){
    QCOMPARE(int(LoadEnding::EdgeUnknown), 0);
    QCOMPARE(int(LoadEnding::EdgeConnectionAborted), 9);
    QCOMPARE(int(LoadEnding::EdgeOperationCanceled), 13);
}

void tst_loadending::aQuickLoadWhichWasStoppedEndsWithoutFailing(){
    const LoadEnding::QuickVerdict verdict = LoadEnding::QuickEnding(LoadEnding::QuickStopped);
    QVERIFY2(verdict.endsLoad, "a stopped load ends nothing");
    QVERIFY2(!verdict.saysFailure, "a stopped load is called a failure");
}

void tst_loadending::aQuickLoadWhichFailedEndsAndSaysSo(){
    const LoadEnding::QuickVerdict verdict = LoadEnding::QuickEnding(LoadEnding::QuickFailed);
    QVERIFY(verdict.endsLoad);
    QVERIFY(verdict.saysFailure);

    const LoadEnding::QuickVerdict done = LoadEnding::QuickEnding(LoadEnding::QuickSucceeded);
    QVERIFY(done.endsLoad);
    QVERIFY(!done.saysFailure);
}

void tst_loadending::aQuickLoadWhichStartedIsNotAnEnding(){
    const LoadEnding::QuickVerdict verdict = LoadEnding::QuickEnding(LoadEnding::QuickStarted);
    QVERIFY(!verdict.endsLoad);
    QVERIFY(!verdict.saysFailure);
}

QTEST_MAIN(tst_loadending)
#include "tst_loadending.moc"
