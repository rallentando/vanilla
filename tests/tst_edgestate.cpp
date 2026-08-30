#include <QtTest>
#include <QDateTime>
#include <QNetworkCookie>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>

#include "switch.hpp"
#include "edgewebviewstate.hpp"
#include "edgeeventsubscriptions.hpp"
#include "edgeunadoptedcontroller.hpp"

#ifdef EDGEWEBVIEW

#define QUOTE "\""

class tst_edgestate : public QObject {
    Q_OBJECT

private slots:
    void aViewAsksForTheEnvironmentOnce();
    void theEnvironmentLeadsToAController();
    void theLastLoadAskedForBeforeReadyIsTheOneRead();
    void aHeldLoadKeepsItsMethodHeadersAndBody();
    void aRetiredViewDoesNotAskForAController();
    void aControllerWhichArrivesForARetiredViewIsClosed();
    void aControllerWhichArrivesForAFailedViewIsClosed();
    void aReadyViewClosesTheControllerItOwns();
    void retiringTwiceClosesNothingTwice();
    void aFailedEnvironmentIsReportedOnce();
    void aFailedControllerIsReportedOnce();
    void aFailureAfterRetirementIsNotReported();

    void theFirstWaiterStartsCreation();
    void aSecondWaiterDuringCreationDoesNotStartASecondOne();
    void aWaiterArrivingAfterTheEnvironmentIsAnsweredAtOnce();
    void everyWaiterIsAnsweredExactlyOnce();
    void aWaiterWhichWentAwayIsNotAnswered();
    void anEnvironmentWhichArrivesForNobodyIsStillReady();
    void aWaiterAfterAFailureStartsCreationAgain();
    void aWaiterWithoutATokenIsNotAWaiter();

    void theControllerIsNotReadyUntilTheScriptIsIn();
    void aViewRetiredWhileTheScriptIsRegisteredStillClosesItsController();
    void aScriptWhichCouldNotBeRegisteredStillLetsThePageThrough();
    void theScriptAnswerAfterRetirementDoesNothing();

    void theFirstViewOfAPrivateProfileEmptiesIt();
    void everyOtherViewOfThatProfileWaitsForTheSameEmptying();
    void anEmptiedProfileIsNotEmptiedAgain();
    void aProfileWhichCouldNotBeEmptiedIsNotAskedAgain();
    void aSettlementForAProfileNobodyIsEmptyingChangesNothing();
    void privateAndOrdinaryProfilesOfOneNameAreNotTheSameProfile();

    void aDownloadOnlyTabWaitsForTheLastDownload();
    void aDownloadNobodyCanWatchStopsTheTabClosingForGood();

    void aProfileWhoseCookiesRemainIsRefusedTheJar();
    void aLaterSuccessfulClearingLetsThatProfileWriteAgain();

    void nothingIsHandedOverBeforeTheViewCanTakeIt();
    void aDragHandedOverIsClosedByItsLeave();
    void aDragHandedOverIsClosedByItsDrop();
    void aDragHandedOverAndAbandonedOwesALeave();
    void aDragThisSideTakesIsNeverHandedOver();
    void aDragOfAnOwnUrlIsForwardedAndRemembered();
    void theDropEchoIsClaimedOnceAndOnlyFresh();
    void aDropWhichNeverReachedTheBackendTakesItsArmBack();
    void hidingDoesNotLoseTheEndTheBackendIsOwed();
    void abandoningADragTwiceOwesNothingTheSecondTime();

    void onlyOneDragLeavesAPageAtATime();
    void aDragWhichCouldNotStartLeavesTheRunningOneAlone();
    void onlyTheOwnerEndsOrCancelsItsDrag();
    void cancellingLeavesTheDragInFlightUntilItEnds();
    void theDataBeingDraggedIsRecognisedByIdentity();
    void endingOrCancellingWithNoDragIsNothing();

    void theCursorsAPageCanAskForBecomeTheirQtShapes();
    void aCursorWithNoSystemIdIsAnArrow();


    void aKeyTheScriptWouldSendIsAccepted();
    void anythingWhichIsNotJsonIsRefused();
    void theWrongVersionOrTagIsRefused();
    void anUnknownKindIsRefused();
    void aKeyTheScriptWouldNeverSendIsRefused();
    void aKeyWithTheWrongTypesIsRefused();
    void aMessageLongerThanTheCapIsRefused();
    void theScrollHintCarriesNothingElse();
    void aScrollReportIsParsedAndSifted();
    void thePrintRequestIsItsOwnKind();
    void aHiddenViewIsSuspendedOnlyWhenAllFiveAgree();
    void aSuspendWhichLandedOnAVisibleViewIsUndoneAtOnce();
    void theFirstExitThroughTheDoorIsTheOnlyOneWhoSpeaks();
    void aRefusedAskDoesNotCloseTheProfilesTurn();
    void onlyAPullBegunAfterTheClearingMayWriteTheJar();
    void aBareScriptAnswerIsStillAnAnswer();
    void theScrollReportBecomesTheNotifiersRatio();

    void nothingIsAskedWhileNoDocumentIsLive();
    void anAnswerFromTheDocumentWhichWasAskedIsUsed();
    void anAnswerWhichCrossedANavigationIsDropped();
    void anAnswerFromBeforeAReloadIsDropped();
    void aGenerationIsNeverReused();

    void nothingIsSentBeforeTheViewCanTakeIt();
    void aButtonHandedOverGetsItsReleaseHandedOver();
    void aButtonTheHostAteIsNeverReleasedToTheBackend();
    void aReleaseWithoutItsPressIsNotInvented();
    void abandoningOwesAnUpForEveryDownItSent();
    void abandoningTwiceOwesNothingTheSecondTime();
    void leavingWithAButtonHeldIsNotALeaving();
    void severalButtonsAreTrackedApart();

    void aPressKeptBackIsNotSentUntilItIsReleased();
    void aKeptPressThisSideSpentIsNeverSent();
    void abandoningOwesNothingForAPressWhichWasNeverSent();
    void aKeptButtonIsStillAButtonWhichIsDown();

    void aSecondPressInTimeAndPlaceIsADouble();
    void aPressTooLateOrTooFarOrOfAnotherButtonIsNotADouble();
    void aThirdPressAfterADoubleIsASingleAgain();
    void aPressTheBackendDidNotHearIsNotTheFirstOfADouble();
    void theBackendHearsDownUpDoubleUpOnlyWhenItHeardBothPresses();

    void aReportIsOfTheDocumentWhateverTheSpellingOfItsUrl();
    void aBareHostAndThatHostWithASlashAreOneAddress();
    void theDocumentWhichArrivesCorrectsTheAddressWhichWasGuessed();
    void nothingHereWritesIntoTheBackendsCookieStore();

    void theInsideOfTheSourceAddressIsStillTheSourceAddress();
    void anAddressTheViewWentToAfterwardsIsTaken();
    void anOrdinaryLoadIsNeverMistakenForItsOwnSource();

    void everySubscriptionIsUndoneExactlyOnce();
    void subscriptionsAreUndoneInTheOrderTheyWereMade();
    void revokingTwiceUndoesNothingTwice();
    void aRevokerWhichComesBackHereFindsNothingLeft();
    void aRegistrationKeptAfterTheListWasRevokedIsDropped();
    void anEmptyRevokerIsNotARegistration();
    void theListIsNeitherCopiedNorMoved();
    void aTokenIsHandedBackToTheInterfaceItWasMadeOn();
    void aRemovalWhichFailsIsNotTriedAgain();
    void nothingIsKeptForASourceWhichIsNotThere();

    void everyRegistrationIsKeptOnTheListItComesOff();
    void everyHandlerListComesOffBeforeTheControllerIsClosed();
    void theDropSiteIsTakenInOnePieceOrNotAtAll();
    void theViewHeaderCarriesNothingButThePointer();

    void theDoorShutsOnANavigationAndOpensOnItsDocument();
    void theBackendsTwoEventsAreWiredToTheDoor();

    void aPrivateViewIsRefusedRatherThanMadeOrdinary();
    void theHostingFallsBackInOneOrder();
    void aControllerNobodyTookIsClosedOnce();
    void aControllerWhichWasTakenIsNotClosed();
    void theHolderIsNeitherCopiedNorMoved();

    void onlyTheAnswersWithSomethingToWaitForJoinTheList();
    void aProfileWhichCouldNotBeEmptiedKeepsNobodyWaiting();
    void aSettlementEmptiesTheListBeforeItHandsItOver();
    void aViewWhichGaveEverythingBackIsOffEveryList();
    void aWaiterWhoseObjectWentIsSweptWithTheNextRetirement();

    void aControllerArrivingAfterTheViewIsGoneIsClosedOnce();
    void aFailureArrivingAfterTheViewIsGoneTouchesNothing();
    void aSuccessWithNothingAfterTheViewIsGoneTouchesNothing();
    void aControllerArrivingForALiveViewIsHandedOver();
    void aFailureArrivingForALiveViewIsReportedOnce();
    void aSuccessWhichCarriesNoControllerIsAFailure();

    void whatTheBackendSaysAboutACookieIsCarriedOver();
    void anExpiryWhichIsNotOneLeavesASessionCookie();
    void aPathTheBackendDidNotGiveIsNotAPath();
};

void tst_edgestate::aViewAsksForTheEnvironmentOnce(){
    EdgeControllerState s;
    QCOMPARE(s.GetState(), EdgeControllerState::State::Constructed);
    QCOMPARE(s.Start(), EdgeControllerState::Effect::RequestEnvironment);
    QCOMPARE(s.GetState(), EdgeControllerState::State::AwaitingEnvironment);
    QCOMPARE(s.Start(), EdgeControllerState::Effect::None);
}

void tst_edgestate::theEnvironmentLeadsToAController(){
    EdgeControllerState s;
    s.Start();
    QCOMPARE(s.EnvironmentReady(), EdgeControllerState::Effect::CreateController);
    QCOMPARE(s.GetState(), EdgeControllerState::State::AwaitingController);
    QCOMPARE(s.ControllerCreated(), EdgeControllerState::Effect::AdoptController);
    QCOMPARE(s.GetState(), EdgeControllerState::State::AwaitingScript);
    QCOMPARE(s.ScriptRegistered(), EdgeControllerState::Effect::ApplyPending);
    QCOMPARE(s.GetState(), EdgeControllerState::State::Ready);
}

void tst_edgestate::theLastLoadAskedForBeforeReadyIsTheOneRead(){
    EdgeControllerState s;
    s.Start();
    QVERIFY(!s.HasPendingLoad());

    EdgePendingLoad one;
    one.url = QUrl(QStringLiteral("https://example.com/one"));
    EdgePendingLoad two;
    two.url = QUrl(QStringLiteral("https://example.com/two"));
    EdgePendingLoad three;
    three.url = QUrl(QStringLiteral("https://example.com/three"));

    s.SetPendingLoad(one);
    s.SetPendingLoad(two);
    s.SetPendingLoad(three);
    QVERIFY(s.HasPendingLoad());
    QCOMPARE(s.TakePendingLoad().url, QUrl(QStringLiteral("https://example.com/three")));
    QVERIFY(!s.HasPendingLoad());
    QCOMPARE(s.TakePendingLoad().url, QUrl());
}

void tst_edgestate::aHeldLoadKeepsItsMethodHeadersAndBody(){
    EdgeControllerState s;
    s.Start();

    EdgePendingLoad post;
    post.url = QUrl(QStringLiteral("https://example.com/form"));
    post.method = QStringLiteral("POST");
    post.headers << QStringLiteral("Content-Type: application/x-www-form-urlencoded");
    post.body = QByteArrayLiteral("a=1&b=2");
    QVERIFY(post.IsRequest());

    s.SetPendingLoad(post);
    const EdgePendingLoad taken = s.TakePendingLoad();
    QVERIFY(taken.IsRequest());
    QCOMPARE(taken.url, QUrl(QStringLiteral("https://example.com/form")));
    QCOMPARE(taken.method, QStringLiteral("POST"));
    QCOMPARE(taken.headers.size(), 1);
    QCOMPARE(taken.body, QByteArrayLiteral("a=1&b=2"));

    EdgePendingLoad plain;
    plain.url = QUrl(QStringLiteral("https://example.com/"));
    QVERIFY(!plain.IsRequest());
}

void tst_edgestate::aRetiredViewDoesNotAskForAController(){
    EdgeControllerState s;
    s.Start();
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::None);
    QCOMPARE(s.EnvironmentReady(), EdgeControllerState::Effect::None);
    QCOMPARE(s.GetState(), EdgeControllerState::State::Retired);
}

void tst_edgestate::aControllerWhichArrivesForARetiredViewIsClosed(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    QCOMPARE(s.GetState(), EdgeControllerState::State::AwaitingController);
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::None);
    QCOMPARE(s.ControllerCreated(), EdgeControllerState::Effect::CloseOrphan);
}

void tst_edgestate::aControllerWhichArrivesForAFailedViewIsClosed(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.ControllerFailed();
    QCOMPARE(s.GetState(), EdgeControllerState::State::Failed);
    QCOMPARE(s.ControllerCreated(), EdgeControllerState::Effect::CloseOrphan);
}

void tst_edgestate::aReadyViewClosesTheControllerItOwns(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.ControllerCreated();
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::CloseOwned);
}

void tst_edgestate::retiringTwiceClosesNothingTwice(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.ControllerCreated();
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::CloseOwned);
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::None);
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::None);
}

void tst_edgestate::aFailedEnvironmentIsReportedOnce(){
    EdgeControllerState s;
    s.Start();
    QCOMPARE(s.EnvironmentFailed(), EdgeControllerState::Effect::ReportFailure);
    QCOMPARE(s.GetState(), EdgeControllerState::State::Failed);
    QCOMPARE(s.EnvironmentFailed(), EdgeControllerState::Effect::None);
}

void tst_edgestate::aFailedControllerIsReportedOnce(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    QCOMPARE(s.ControllerFailed(), EdgeControllerState::Effect::ReportFailure);
    QCOMPARE(s.ControllerFailed(), EdgeControllerState::Effect::None);
}

void tst_edgestate::aFailureAfterRetirementIsNotReported(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.Retire();
    QCOMPARE(s.ControllerFailed(), EdgeControllerState::Effect::None);
    QCOMPARE(s.EnvironmentFailed(), EdgeControllerState::Effect::None);
}

void tst_edgestate::theFirstWaiterStartsCreation(){
    EdgeEnvironmentState e;
    QCOMPARE(e.GetState(), EdgeEnvironmentState::State::Idle);
    QCOMPARE(e.AddWaiter(1), EdgeEnvironmentState::Effect::StartCreation);
    QCOMPARE(e.GetState(), EdgeEnvironmentState::State::Creating);
    QCOMPARE(e.WaiterCount(), 1);
}

void tst_edgestate::aSecondWaiterDuringCreationDoesNotStartASecondOne(){
    EdgeEnvironmentState e;
    e.AddWaiter(1);
    QCOMPARE(e.AddWaiter(2), EdgeEnvironmentState::Effect::None);
    QCOMPARE(e.WaiterCount(), 2);
    QCOMPARE(e.AddWaiter(2), EdgeEnvironmentState::Effect::None);
    QCOMPARE(e.WaiterCount(), 2);
}

void tst_edgestate::aWaiterArrivingAfterTheEnvironmentIsAnsweredAtOnce(){
    EdgeEnvironmentState e;
    e.AddWaiter(1);
    e.CreationSucceeded();
    QCOMPARE(e.AddWaiter(2), EdgeEnvironmentState::Effect::NotifyReady);
    QCOMPARE(e.WaiterCount(), 0);
}

void tst_edgestate::everyWaiterIsAnsweredExactlyOnce(){
    EdgeEnvironmentState e;
    e.AddWaiter(7);
    e.AddWaiter(8);
    e.AddWaiter(9);
    const QList<int> first = e.CreationSucceeded();
    QCOMPARE(first, QList<int>() << 7 << 8 << 9);
    QCOMPARE(e.CreationSucceeded(), QList<int>());
}

void tst_edgestate::aWaiterWhichWentAwayIsNotAnswered(){
    EdgeEnvironmentState e;
    e.AddWaiter(1);
    e.AddWaiter(2);
    e.RemoveWaiter(1);
    QVERIFY(!e.HasWaiter(1));
    QCOMPARE(e.CreationSucceeded(), QList<int>() << 2);
}

void tst_edgestate::anEnvironmentWhichArrivesForNobodyIsStillReady(){
    EdgeEnvironmentState e;
    e.AddWaiter(1);
    e.RemoveWaiter(1);
    QCOMPARE(e.CreationSucceeded(), QList<int>());
    QCOMPARE(e.GetState(), EdgeEnvironmentState::State::Ready);
    QCOMPARE(e.AddWaiter(2), EdgeEnvironmentState::Effect::NotifyReady);
}

void tst_edgestate::aWaiterAfterAFailureStartsCreationAgain(){
    EdgeEnvironmentState e;
    e.AddWaiter(1);
    QCOMPARE(e.CreationFailed(), QList<int>() << 1);
    QCOMPARE(e.GetState(), EdgeEnvironmentState::State::Failed);
    QCOMPARE(e.AddWaiter(2), EdgeEnvironmentState::Effect::StartCreation);
    QCOMPARE(e.GetState(), EdgeEnvironmentState::State::Creating);
}

void tst_edgestate::aWaiterWithoutATokenIsNotAWaiter(){
    EdgeEnvironmentState e;
    QCOMPARE(e.AddWaiter(0), EdgeEnvironmentState::Effect::None);
    QCOMPARE(e.GetState(), EdgeEnvironmentState::State::Idle);
    QCOMPARE(e.WaiterCount(), 0);
}


void tst_edgestate::theControllerIsNotReadyUntilTheScriptIsIn(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    QCOMPARE(s.ControllerCreated(), EdgeControllerState::Effect::AdoptController);
    QCOMPARE(s.GetState(), EdgeControllerState::State::AwaitingScript);
    QCOMPARE(s.ScriptRegistered(), EdgeControllerState::Effect::ApplyPending);
    QCOMPARE(s.GetState(), EdgeControllerState::State::Ready);
    QCOMPARE(s.ScriptRegistered(), EdgeControllerState::Effect::None);
}

void tst_edgestate::aViewRetiredWhileTheScriptIsRegisteredStillClosesItsController(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.ControllerCreated();
    QCOMPARE(s.GetState(), EdgeControllerState::State::AwaitingScript);
    QCOMPARE(s.Retire(), EdgeControllerState::Effect::CloseOwned);
}

void tst_edgestate::aScriptWhichCouldNotBeRegisteredStillLetsThePageThrough(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.ControllerCreated();
    QCOMPARE(s.ScriptFailed(), EdgeControllerState::Effect::ApplyPending);
    QCOMPARE(s.GetState(), EdgeControllerState::State::Ready);
}

void tst_edgestate::theScriptAnswerAfterRetirementDoesNothing(){
    EdgeControllerState s;
    s.Start();
    s.EnvironmentReady();
    s.ControllerCreated();
    s.Retire();
    QCOMPARE(s.ScriptRegistered(), EdgeControllerState::Effect::None);
    QCOMPARE(s.ScriptFailed(), EdgeControllerState::Effect::None);
}

static QString KeyMessage(int version, int tag, const QString &kind,
                          const QString &code, const QString &shift){
    return QStringLiteral("{" QUOTE "v" QUOTE ":%1," QUOTE "tag" QUOTE ":%2,"
                          QUOTE "kind" QUOTE ":" QUOTE "%3" QUOTE ","
                          QUOTE "code" QUOTE ":%4," QUOTE "shift" QUOTE ":%5}")
        .arg(version).arg(tag).arg(kind).arg(code).arg(shift);
}

void tst_edgestate::aKeyTheScriptWouldSendIsAccepted(){
    const EdgeMessage m = EdgeMessage::Parse
        (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("81"),
                    QStringLiteral("false")), 42);
    QVERIFY(m.IsValid());
    QCOMPARE(m.GetKind(), EdgeMessage::Kind::Key);
    QCOMPARE(m.GetCode(), 81);
    QCOMPARE(m.GetShift(), false);
}

void tst_edgestate::anythingWhichIsNotJsonIsRefused(){
    QVERIFY(!EdgeMessage::Parse(QString(), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse(QStringLiteral("keyPressEvent42,81"), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse(QStringLiteral("[1,2,3]"), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse(QStringLiteral("{"), 42).IsValid());
}

void tst_edgestate::theWrongVersionOrTagIsRefused(){
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(2, 42, QStringLiteral("key"), QStringLiteral("81"),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 43, QStringLiteral("key"), QStringLiteral("81"),
                        QStringLiteral("false")), 42).IsValid());
}

void tst_edgestate::anUnknownKindIsRefused(){
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("mouse"), QStringLiteral("81"),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("action"), QStringLiteral("81"),
                        QStringLiteral("false")), 42).IsValid());
}

void tst_edgestate::aKeyTheScriptWouldNeverSendIsRefused(){
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("112"),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("27"),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("8"),
                        QStringLiteral("false")), 42).IsValid());

    QVERIFY(EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("48"),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("90"),
                        QStringLiteral("true")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("47"),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("91"),
                        QStringLiteral("false")), 42).IsValid());
}

void tst_edgestate::aKeyWithTheWrongTypesIsRefused(){
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("\"81\""),
                        QStringLiteral("false")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("81"),
                        QStringLiteral("1")), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("81.5"),
                        QStringLiteral("false")), 42).IsValid());
}

void tst_edgestate::aMessageLongerThanTheCapIsRefused(){
    QString padded = KeyMessage(1, 42, QStringLiteral("key"), QStringLiteral("81"),
                                QStringLiteral("false"));
    padded.chop(1);
    padded += QStringLiteral(",\"pad\":\"") +
              QString(EdgeMessage::MaxLength, QLatin1Char('x')) +
              QStringLiteral("\"}");
    QVERIFY(padded.length() > EdgeMessage::MaxLength);
    QVERIFY(!EdgeMessage::Parse(padded, 42).IsValid());
}

void tst_edgestate::theScrollHintCarriesNothingElse(){
    QVERIFY(EdgeMessage::Parse
            (QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"preventScrollRestoration\"}"),
             42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"preventScrollRestoration\","
                            "\"code\":81}"), 42).IsValid());
}

static QString ScrollMessage(const QString &x, const QString &y,
                             const QString &w, const QString &h,
                             const QString &vw, const QString &vh){
    return QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"scroll\","
                          "\"x\":%1,\"y\":%2,\"w\":%3,\"h\":%4,"
                          "\"vw\":%5,\"vh\":%6}")
        .arg(x, y, w, h, vw, vh);
}

void tst_edgestate::aScrollReportIsParsedAndSifted(){
    const EdgeMessage m = EdgeMessage::Parse
        (ScrollMessage(QStringLiteral("10"), QStringLiteral("2500"),
                       QStringLiteral("980"), QStringLiteral("24000"),
                       QStringLiteral("980"), QStringLiteral("640")), 42);
    QVERIFY(m.IsValid());
    QCOMPARE(m.GetKind(), EdgeMessage::Kind::Scroll);
    QCOMPARE(m.GetScrollPosition(), QPointF(10, 2500));
    QCOMPARE(m.GetContentsSize(), QSizeF(980, 24000));
    QCOMPARE(m.GetViewportSize(), QSizeF(980, 640));

    QVERIFY(EdgeMessage::Parse
            (ScrollMessage(QStringLiteral("0.5"), QStringLiteral("0"),
                           QStringLiteral("980.25"), QStringLiteral("100"),
                           QStringLiteral("980"), QStringLiteral("640")),
             42).IsValid());

    QVERIFY(!EdgeMessage::Parse
            (ScrollMessage(QStringLiteral("0"), QStringLiteral("0"),
                           QStringLiteral("980"), QStringLiteral("100"),
                           QStringLiteral("980"), QStringLiteral("640"))
             .replace(QStringLiteral("\"tag\":42"),
                      QStringLiteral("\"tag\":43")), 42).IsValid());

    QVERIFY(!EdgeMessage::Parse
            (QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"scroll\","
                            "\"x\":0,\"y\":0,\"w\":980,\"h\":100,"
                            "\"vw\":980}"), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (ScrollMessage(QStringLiteral("0"), QStringLiteral("\"0\""),
                           QStringLiteral("980"), QStringLiteral("100"),
                           QStringLiteral("980"), QStringLiteral("640")),
             42).IsValid());

    QVERIFY(!EdgeMessage::Parse
            (ScrollMessage(QStringLiteral("-1"), QStringLiteral("0"),
                           QStringLiteral("980"), QStringLiteral("100"),
                           QStringLiteral("980"), QStringLiteral("640")),
             42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (ScrollMessage(QStringLiteral("0"), QStringLiteral("0"),
                           QStringLiteral("0"), QStringLiteral("100"),
                           QStringLiteral("980"), QStringLiteral("640")),
             42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (ScrollMessage(QStringLiteral("0"), QStringLiteral("0"),
                           QStringLiteral("980"), QStringLiteral("100"),
                           QStringLiteral("980"), QStringLiteral("-640")),
             42).IsValid());

    QVERIFY(!EdgeMessage::Parse
            (QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"scroll\","
                            "\"x\":0,\"y\":0,\"w\":980,\"h\":100,"
                            "\"vw\":980,\"vh\":640,\"code\":81}"),
             42).IsValid());
}

void tst_edgestate::thePrintRequestIsItsOwnKind(){
    const EdgeMessage m = EdgeMessage::Parse
        (QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"print\"}"), 42);
    QVERIFY(m.IsValid());
    QCOMPARE(m.GetKind(), EdgeMessage::Kind::Print);

    QVERIFY(!EdgeMessage::Parse
            (QStringLiteral("{\"v\":1,\"tag\":43,\"kind\":\"print\"}"), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (QStringLiteral("{\"v\":2,\"tag\":42,\"kind\":\"print\"}"), 42).IsValid());
    QVERIFY(!EdgeMessage::Parse
            (QStringLiteral("{\"v\":1,\"tag\":42,\"kind\":\"print\",\"code\":81}"),
             42).IsValid());
}

void tst_edgestate::aHiddenViewIsSuspendedOnlyWhenAllFiveAgree(){
    QVERIFY(EdgeSuspendPolicy::ShouldSuspend(false, false, false, false, false));

    QVERIFY(!EdgeSuspendPolicy::ShouldSuspend(true, false, false, false, false));
    QVERIFY(!EdgeSuspendPolicy::ShouldSuspend(false, true, false, false, false));
    QVERIFY(!EdgeSuspendPolicy::ShouldSuspend(false, false, true, false, false));
    QVERIFY(!EdgeSuspendPolicy::ShouldSuspend(false, false, false, true, false));
    QVERIFY(!EdgeSuspendPolicy::ShouldSuspend(false, false, false, false, true));
}

void tst_edgestate::aSuspendWhichLandedOnAVisibleViewIsUndoneAtOnce(){
    QVERIFY(EdgeSuspendPolicy::ShouldResumeAtOnce(true, true));

    QVERIFY(!EdgeSuspendPolicy::ShouldResumeAtOnce(true, false));
    QVERIFY(!EdgeSuspendPolicy::ShouldResumeAtOnce(false, true));
    QVERIFY(!EdgeSuspendPolicy::ShouldResumeAtOnce(false, false));
}

void tst_edgestate::theFirstExitThroughTheDoorIsTheOnlyOneWhoSpeaks(){
    EdgeOnce once;
    QVERIFY(once.Take());
    QVERIFY(!once.Take());
    QVERIFY(!once.Take());
}

void tst_edgestate::aRefusedAskDoesNotCloseTheProfilesTurn(){
    EdgeClearRoster roster;
    const QString name = QStringLiteral("space");

    QVERIFY(roster.ShouldAsk(name));
    roster.Asked(name, false);
    QVERIFY(roster.ShouldAsk(name));
    roster.Asked(name, true);
    QVERIFY(!roster.ShouldAsk(name));
    QVERIFY(roster.ShouldAsk(name + QStringLiteral("|private")));
}

void tst_edgestate::onlyAPullBegunAfterTheClearingMayWriteTheJar(){
    EdgeCookieClearLedger ledger;
    const QString key = QStringLiteral("space");

    QVERIFY(ledger.MayWrite(ledger.Ticket(), key));

    const int before = ledger.Ticket();
    ledger.ClearStarted();
    QVERIFY(!ledger.MayWrite(before, key));

    ledger.AskPlaced();
    ledger.AskPlaced();
    ledger.AskSettled(key, true);

    const int during = ledger.Ticket();
    QVERIFY(!ledger.MayWrite(during, key));
    ledger.AskSettled(key, true);
    QVERIFY(!ledger.MayWrite(during, key));

    QVERIFY(ledger.MayWrite(ledger.Ticket(), key));

    ledger.ClearStarted();
    QVERIFY(ledger.MayWrite(ledger.Ticket(), key));

    const int quiet = ledger.Ticket();
    ledger.AskSettled(key, true);
    QVERIFY(ledger.MayWrite(quiet, key));
}

void tst_edgestate::aBareScriptAnswerIsStillAnAnswer(){
    QCOMPARE(EdgeScriptResultToVariant(QByteArrayLiteral("{\"a\":1}")).toMap()
             .value(QStringLiteral("a")).toInt(), 1);
    QCOMPARE(EdgeScriptResultToVariant(QByteArrayLiteral("[1,2]")).toList().size(), 2);

    QCOMPARE(EdgeScriptResultToVariant(QByteArrayLiteral("2.5")).toFloat(), 2.5f);
    QCOMPARE(EdgeScriptResultToVariant(QByteArrayLiteral("\"hi\"")).toString(),
             QStringLiteral("hi"));
    QCOMPARE(EdgeScriptResultToVariant(QByteArrayLiteral("true")).toBool(), true);

    QVERIFY(!EdgeScriptResultToVariant(QByteArrayLiteral("null")).isValid());
    QVERIFY(!EdgeScriptResultToVariant(QByteArrayLiteral("not json")).isValid());
    QVERIFY(!EdgeScriptResultToVariant(QByteArray()).isValid());
}

void tst_edgestate::theScrollReportBecomesTheNotifiersRatio(){
    QCOMPARE(EdgeScrollRatio(QPointF(0, 500), QSizeF(1000, 2000), QSizeF(1000, 1000)),
             QPointF(0.5, 0.5));

    QCOMPARE(EdgeScrollRatio(QPointF(0, 0), QSizeF(2000, 2000), QSizeF(1000, 1000)),
             QPointF(0.0, 0.0));
    QCOMPARE(EdgeScrollRatio(QPointF(1000, 1000), QSizeF(2000, 2000), QSizeF(1000, 1000)),
             QPointF(1.0, 1.0));

    QCOMPARE(EdgeScrollRatio(QPointF(0, 250), QSizeF(800, 2000), QSizeF(1000, 1000)),
             QPointF(0.5, 0.25));

    QCOMPARE(EdgeScrollRatio(QPointF(0, 1500), QSizeF(1000, 2000), QSizeF(1000, 1000)),
             QPointF(0.5, 1.0));
}

void tst_edgestate::nothingIsAskedWhileNoDocumentIsLive(){
    EdgeGeneration g;
    QVERIFY(!g.IsLive());
    QCOMPARE(g.Current(), 0);

    g.Loaded();
    QVERIFY(g.IsLive());

    g.Started();
    QVERIFY(!g.IsLive());
}

void tst_edgestate::anAnswerFromTheDocumentWhichWasAskedIsUsed(){
    EdgeGeneration g;
    g.Loaded();
    const int asked = g.Current();
    QVERIFY(g.StillCurrent(asked));
}

void tst_edgestate::anAnswerWhichCrossedANavigationIsDropped(){
    EdgeGeneration g;
    g.Loaded();
    const int asked = g.Current();

    g.Started();
    QVERIFY(!g.StillCurrent(asked));

    g.Loaded();
    QVERIFY(!g.StillCurrent(asked));
}

void tst_edgestate::anAnswerFromBeforeAReloadIsDropped(){
    EdgeGeneration g;
    g.Loaded();
    const int before = g.Current();
    g.Started();
    g.Loaded();
    QVERIFY(!g.StillCurrent(before));
    QVERIFY(g.StillCurrent(g.Current()));
}

void tst_edgestate::aGenerationIsNeverReused(){
    EdgeGeneration g;
    QVERIFY(!g.StillCurrent(0));

    QList<int> seen;
    for(int i = 0; i < 5; i++){
        g.Started();
        QVERIFY(!g.StillCurrent(0));
        g.Loaded();
        QVERIFY(g.Current() != 0);
        QVERIFY(!seen.contains(g.Current()));
        seen << g.Current();
    }
}

void tst_edgestate::nothingIsSentBeforeTheViewCanTakeIt(){
    EdgeInputLedger l;
    QVERIFY(!l.IsReady());
    QCOMPARE(l.Press(EdgeInputLedger::LeftButton, false), EdgeInputLedger::Send::Nothing);
    QCOMPARE(l.Move(), EdgeInputLedger::Send::Nothing);
    QCOMPARE(l.Wheel(), EdgeInputLedger::Send::Nothing);

    l.SetReady(true);
    QCOMPARE(l.Release(EdgeInputLedger::LeftButton), EdgeInputLedger::Send::Nothing);
}

void tst_edgestate::aButtonHandedOverGetsItsReleaseHandedOver(){
    EdgeInputLedger l;
    l.SetReady(true);
    QCOMPARE(l.Press(EdgeInputLedger::LeftButton, false), EdgeInputLedger::Send::ToBackend);
    QCOMPARE(l.Forwarded(), int(EdgeInputLedger::LeftButton));
    QCOMPARE(l.Release(EdgeInputLedger::LeftButton), EdgeInputLedger::Send::ToBackend);
    QCOMPARE(l.Forwarded(), 0);
    QVERIFY(!l.AnyHeld());
}

void tst_edgestate::aButtonTheHostAteIsNeverReleasedToTheBackend(){
    EdgeInputLedger l;
    l.SetReady(true);
    QCOMPARE(l.Press(EdgeInputLedger::RightButton, true), EdgeInputLedger::Send::Nothing);
    QCOMPARE(l.Consumed(), int(EdgeInputLedger::RightButton));
    QCOMPARE(l.Forwarded(), 0);
    QCOMPARE(l.Release(EdgeInputLedger::RightButton), EdgeInputLedger::Send::Nothing);
    QVERIFY(!l.AnyHeld());
}

void tst_edgestate::aReleaseWithoutItsPressIsNotInvented(){
    EdgeInputLedger l;
    l.SetReady(true);
    QCOMPARE(l.Release(EdgeInputLedger::MiddleButton), EdgeInputLedger::Send::Nothing);
}

void tst_edgestate::abandoningOwesAnUpForEveryDownItSent(){
    EdgeInputLedger l;
    l.SetReady(true);
    l.Press(EdgeInputLedger::LeftButton, false);
    l.Press(EdgeInputLedger::MiddleButton, false);
    l.Press(EdgeInputLedger::RightButton, true);

    const int owed = l.Abandon();
    QVERIFY(owed & EdgeInputLedger::LeftButton);
    QVERIFY(owed & EdgeInputLedger::MiddleButton);
    QVERIFY(!(owed & EdgeInputLedger::RightButton));
    QVERIFY(!l.AnyHeld());
}

void tst_edgestate::abandoningTwiceOwesNothingTheSecondTime(){
    EdgeInputLedger l;
    l.SetReady(true);
    l.Press(EdgeInputLedger::LeftButton, false);
    QVERIFY(l.Abandon() != 0);
    QCOMPARE(l.Abandon(), 0);
}

void tst_edgestate::leavingWithAButtonHeldIsNotALeaving(){
    EdgeInputLedger l;
    l.SetReady(true);
    l.Move();
    l.Press(EdgeInputLedger::LeftButton, false);
    QCOMPARE(l.Leave(), EdgeInputLedger::Send::Nothing);
    QVERIFY(l.IsInside());

    l.Release(EdgeInputLedger::LeftButton);
    QCOMPARE(l.Leave(), EdgeInputLedger::Send::ToBackend);
    QVERIFY(!l.IsInside());
    QCOMPARE(l.Leave(), EdgeInputLedger::Send::Nothing);
}

void tst_edgestate::severalButtonsAreTrackedApart(){
    EdgeInputLedger l;
    l.SetReady(true);
    l.Press(EdgeInputLedger::LeftButton, false);
    l.Press(EdgeInputLedger::RightButton, true);
    l.Press(EdgeInputLedger::MiddleButton, false);

    QCOMPARE(l.Forwarded(), int(EdgeInputLedger::LeftButton | EdgeInputLedger::MiddleButton));
    QCOMPARE(l.Consumed(), int(EdgeInputLedger::RightButton));

    QCOMPARE(l.Release(EdgeInputLedger::MiddleButton), EdgeInputLedger::Send::ToBackend);
    QCOMPARE(l.Forwarded(), int(EdgeInputLedger::LeftButton));
    QVERIFY(l.AnyHeld());
}

void tst_edgestate::aPressKeptBackIsNotSentUntilItIsReleased(){
    EdgeInputLedger l;
    l.SetReady(true);

    QVERIFY(l.Hold(EdgeInputLedger::RightButton));
    QCOMPARE(l.Kept(), int(EdgeInputLedger::RightButton));
    QCOMPARE(l.Forwarded(), 0);
    QCOMPARE(l.Consumed(), 0);

    QCOMPARE(l.Release(EdgeInputLedger::RightButton), EdgeInputLedger::Send::DownThenUp);
    QCOMPARE(l.Kept(), 0);
    QVERIFY(!l.AnyHeld());
}

void tst_edgestate::aKeptPressThisSideSpentIsNeverSent(){
    EdgeInputLedger l;
    l.SetReady(true);

    QVERIFY(l.Hold(EdgeInputLedger::RightButton));
    l.Drop(EdgeInputLedger::RightButton);
    QCOMPARE(l.Kept(), 0);
    QCOMPARE(l.Release(EdgeInputLedger::RightButton), EdgeInputLedger::Send::Nothing);
}

void tst_edgestate::abandoningOwesNothingForAPressWhichWasNeverSent(){
    EdgeInputLedger l;
    l.SetReady(true);
    l.Press(EdgeInputLedger::LeftButton, false);
    QVERIFY(l.Hold(EdgeInputLedger::RightButton));

    const int owed = l.Abandon();
    QVERIFY(owed & EdgeInputLedger::LeftButton);
    QVERIFY(!(owed & EdgeInputLedger::RightButton));
    QCOMPARE(l.Kept(), 0);
    QVERIFY(!l.AnyHeld());

    EdgeInputLedger k;
    k.SetReady(true);
    QVERIFY(k.Hold(EdgeInputLedger::RightButton));
    QCOMPARE(k.Abandon(), 0);
    QCOMPARE(k.Kept(), 0);
}

void tst_edgestate::aKeptButtonIsStillAButtonWhichIsDown(){
    EdgeInputLedger l;
    l.SetReady(true);
    l.Move();
    QVERIFY(l.Hold(EdgeInputLedger::RightButton));

    QVERIFY(l.AnyHeld());
    QCOMPARE(l.Leave(), EdgeInputLedger::Send::Nothing);
    QVERIFY(l.IsInside());
}

void tst_edgestate::aSecondPressInTimeAndPlaceIsADouble(){
    EdgeClickClock c;
    QVERIFY(!c.Press(1, 1000, QPointF(10, 10), 500, 4));
    QVERIFY(c.Press(1, 1300, QPointF(12, 11), 500, 4));
}

void tst_edgestate::aPressTooLateOrTooFarOrOfAnotherButtonIsNotADouble(){
    EdgeClickClock late;
    QVERIFY(!late.Press(1, 1000, QPointF(10, 10), 500, 4));
    QVERIFY(!late.Press(1, 1500, QPointF(10, 10), 500, 4));

    EdgeClickClock away;
    QVERIFY(!away.Press(1, 1000, QPointF(10, 10), 500, 4));
    QVERIFY(!away.Press(1, 1100, QPointF(10, 14), 500, 4));

    EdgeClickClock other;
    QVERIFY(!other.Press(1, 1000, QPointF(10, 10), 500, 4));
    QVERIFY(!other.Press(2, 1100, QPointF(10, 10), 500, 4));
    QVERIFY(!other.Press(1, 1200, QPointF(10, 10), 500, 4));

    EdgeClickClock reset;
    QVERIFY(!reset.Press(1, 1000, QPointF(10, 10), 500, 4));
    reset.Reset();
    QVERIFY(!reset.Press(1, 1100, QPointF(10, 10), 500, 4));
}

void tst_edgestate::aThirdPressAfterADoubleIsASingleAgain(){
    EdgeClickClock c;
    QVERIFY(!c.Press(1, 1000, QPointF(10, 10), 500, 4));
    QVERIFY(c.Press(1, 1100, QPointF(10, 10), 500, 4));
    QVERIFY(!c.Press(1, 1200, QPointF(10, 10), 500, 4));
    QVERIFY(c.Press(1, 1300, QPointF(10, 10), 500, 4));
}

void tst_edgestate::aPressTheBackendDidNotHearIsNotTheFirstOfADouble(){
    EdgeClickClock taken;
    QVERIFY(!taken.Press(1, 1000, QPointF(10, 10), 500, 4, false));
    QVERIFY(!taken.Press(1, 1100, QPointF(10, 10), 500, 4, true));
    QVERIFY(taken.Press(1, 1200, QPointF(10, 10), 500, 4, true));

    EdgeClickClock between;
    QVERIFY(!between.Press(1, 1000, QPointF(10, 10), 500, 4, true));
    QVERIFY(!between.Press(1, 1100, QPointF(10, 10), 500, 4, false));
    QVERIFY(!between.Press(1, 1200, QPointF(10, 10), 500, 4, true));

    EdgeClickClock second;
    QVERIFY(!second.Press(1, 1000, QPointF(10, 10), 500, 4, true));
    QVERIFY(!second.Press(1, 1100, QPointF(10, 10), 500, 4, false));
}

void tst_edgestate::theBackendHearsDownUpDoubleUpOnlyWhenItHeardBothPresses(){
    typedef EdgeInputLedger::Send Send;
    const EdgeInputLedger::Button left = EdgeInputLedger::LeftButton;

    struct Sent { Send send; bool doubled; };
    auto press = [&](EdgeInputLedger &l, EdgeClickClock &c, qint64 when,
                     bool hostWants) -> Sent {
        const Send send = l.Press(left, hostWants);
        const bool doubled = c.Press(1, when, QPointF(10, 10), 500, 4,
                                     send == Send::ToBackend);
        return Sent{send, doubled};
    };

    {
        EdgeInputLedger l;
        EdgeClickClock c;
        l.SetReady(true);
        l.Move();
        Sent s = press(l, c, 1000, false);
        QCOMPARE(s.send, Send::ToBackend);
        QVERIFY(!s.doubled);
        QCOMPARE(l.Release(left), Send::ToBackend);
        s = press(l, c, 1100, false);
        QCOMPARE(s.send, Send::ToBackend);
        QVERIFY(s.doubled);
        QCOMPARE(l.Release(left), Send::ToBackend);
    }
    {
        EdgeInputLedger l;
        EdgeClickClock c;
        l.SetReady(true);
        l.Move();
        Sent s = press(l, c, 1000, true);
        QVERIFY(s.send != Send::ToBackend);
        QVERIFY(!s.doubled);
        l.Release(left);
        s = press(l, c, 1100, false);
        QCOMPARE(s.send, Send::ToBackend);
        QVERIFY(!s.doubled);
        QCOMPARE(l.Release(left), Send::ToBackend);
    }
}

void tst_edgestate::aReportIsOfTheDocumentWhateverTheSpellingOfItsUrl(){
    const QUrl japanese = QUrl::fromUserInput(
        QStringLiteral("https://www.google.com/search?q=テスト 検索"));
    QVERIFY(EdgeMessage::IsFromDocument(
        QStringLiteral("https://www.google.com/search?q=%E3%83%86%E3%82%B9%E3%83%88%20%E6%A4%9C%E7%B4%A2"),
        japanese));
    QVERIFY(japanese.toString() !=
            QStringLiteral("https://www.google.com/search?q=%E3%83%86%E3%82%B9%E3%83%88%20%E6%A4%9C%E7%B4%A2"));
    QVERIFY(!EdgeMessage::IsFromDocument(
        QStringLiteral("https://www.google.com/frame?q=%E3%83%86"), japanese));
    QVERIFY(!EdgeMessage::IsFromDocument(
        QStringLiteral("https://www.google.com/search?q=%E3%83%86"), japanese));
}

void tst_edgestate::aBareHostAndThatHostWithASlashAreOneAddress(){
    QVERIFY(EdgeMessage::IsFromDocument(
        QStringLiteral("https://example.com/"),
        QUrl(QStringLiteral("https://example.com"))));
    QVERIFY(EdgeMessage::IsFromDocument(
        QStringLiteral("https://example.com"),
        QUrl(QStringLiteral("https://example.com/"))));

    QVERIFY(!EdgeMessage::IsFromDocument(
        QStringLiteral("https://example.com/dir/"),
        QUrl(QStringLiteral("https://example.com/dir"))));
    QVERIFY(!EdgeMessage::IsFromDocument(
        QStringLiteral("https://other.com/"),
        QUrl(QStringLiteral("https://example.com"))));
    QVERIFY(!EdgeMessage::IsFromDocument(
        QStringLiteral("http://example.com/"),
        QUrl(QStringLiteral("https://example.com"))));
    QVERIFY(!EdgeMessage::IsFromDocument(
        QStringLiteral("https://example.com:8443/"),
        QUrl(QStringLiteral("https://example.com"))));

    QCOMPARE(EdgeWithRootPath(QUrl(QStringLiteral("https://example.com"))),
             QUrl(QStringLiteral("https://example.com/")));
    QCOMPARE(EdgeWithRootPath(QUrl(QStringLiteral("https://example.com/dir"))),
             QUrl(QStringLiteral("https://example.com/dir")));
    QCOMPARE(EdgeWithRootPath(QUrl(QStringLiteral("https://example.com?a=b"))),
             QUrl(QStringLiteral("https://example.com/?a=b")));
}

void tst_edgestate::theInsideOfTheSourceAddressIsStillTheSourceAddress(){
    QVERIFY(EdgeIsOwnViewSource(
        QUrl(QStringLiteral("view-source:http://127.0.0.1:8091/jsoff.html")),
        QUrl(QStringLiteral("http://127.0.0.1:8091/jsoff.html"))));

    QVERIFY(EdgeIsOwnViewSource(
        QUrl(QStringLiteral("view-source:http://127.0.0.1:8091")),
        QUrl(QStringLiteral("http://127.0.0.1:8091/"))));
    QVERIFY(!EdgeIsOwnViewSource(
        QUrl(QStringLiteral("view-source:http://host/dir")),
        QUrl(QStringLiteral("http://host/dir/"))));

    QVERIFY(EdgeIsOwnViewSource(
        QUrl(QStringLiteral("view-source:http://host/a?b=c")),
        QUrl(QStringLiteral("http://host/a?b=c"))));
}

void tst_edgestate::anAddressTheViewWentToAfterwardsIsTaken(){
    const QUrl shown(QStringLiteral("view-source:http://127.0.0.1:8091/jsoff.html"));

    QVERIFY(!EdgeIsOwnViewSource(
        shown, QUrl(QStringLiteral("http://127.0.0.1:8091/other.html"))));
    QVERIFY(!EdgeIsOwnViewSource(
        shown, QUrl(QStringLiteral("http://127.0.0.1:8091/jsoff.html?x=1"))));
    QVERIFY(!EdgeIsOwnViewSource(
        shown, QUrl(QStringLiteral("http://example.com/jsoff.html"))));
    QVERIFY(!EdgeIsOwnViewSource(
        shown, QUrl(QStringLiteral("view-source:http://127.0.0.1:8091/jsoff.html"))));
}

void tst_edgestate::anOrdinaryLoadIsNeverMistakenForItsOwnSource(){
    QVERIFY(!EdgeIsOwnViewSource(
        QUrl(QStringLiteral("http://127.0.0.1:8091/jsoff.html")),
        QUrl(QStringLiteral("http://127.0.0.1:8091/jsoff.html"))));
    QVERIFY(!EdgeIsOwnViewSource(QUrl(), QUrl()));
    QVERIFY(!EdgeIsOwnViewSource(QUrl(QStringLiteral("view-source:")), QUrl()));
}

void tst_edgestate::theFirstViewOfAPrivateProfileEmptiesIt(){
    EdgePrivateWipeLedger l;
    const QString key = QStringLiteral("space1|private");
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Wipe);
    QVERIFY(!l.IsClean(key));
    QVERIFY(!l.IsFailed(key));
}

void tst_edgestate::everyOtherViewOfThatProfileWaitsForTheSameEmptying(){
    EdgePrivateWipeLedger l;
    const QString key = QStringLiteral("space1|private");
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Wipe);
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Wait);
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Wait);
    QVERIFY(!l.IsClean(key));

    l.Settled(key, true);
    QVERIFY(l.IsClean(key));
}

void tst_edgestate::anEmptiedProfileIsNotEmptiedAgain(){
    EdgePrivateWipeLedger l;
    const QString key = QStringLiteral("space1|private");
    l.Enter(key);
    l.Settled(key, true);
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Proceed);
    QVERIFY(l.IsClean(key));
}

void tst_edgestate::aProfileWhichCouldNotBeEmptiedIsNotAskedAgain(){
    EdgePrivateWipeLedger l;
    const QString key = QStringLiteral("space1|private");
    l.Enter(key);
    l.Settled(key, false);
    QVERIFY(l.IsFailed(key));
    QVERIFY(!l.IsClean(key));
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Fail);

    l.Settled(key, true);
    QVERIFY(l.IsFailed(key));
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Fail);
}

void tst_edgestate::aSettlementForAProfileNobodyIsEmptyingChangesNothing(){
    EdgePrivateWipeLedger l;
    const QString key = QStringLiteral("space1|private");
    l.Settled(key, true);
    QVERIFY(!l.IsClean(key));
    QCOMPARE(l.Enter(key), EdgePrivateWipeLedger::Effect::Wipe);
}

void tst_edgestate::privateAndOrdinaryProfilesOfOneNameAreNotTheSameProfile(){
    EdgePrivateWipeLedger l;
    const QString priv = QStringLiteral("space1|private");
    const QString plain = QStringLiteral("space1");
    QCOMPARE(l.Enter(priv), EdgePrivateWipeLedger::Effect::Wipe);
    QCOMPARE(l.Enter(plain), EdgePrivateWipeLedger::Effect::Wipe);
    l.Settled(priv, true);
    QVERIFY(l.IsClean(priv));
    QVERIFY(!l.IsClean(plain));
}

void tst_edgestate::aDownloadOnlyTabWaitsForTheLastDownload(){
    EdgeDownloadCloseLedger l;
    QVERIFY(l.MayClose());

    l.Started();
    l.Started();
    QCOMPARE(l.InFlight(), 2);
    QVERIFY(!l.MayClose());

    l.Settled();
    QVERIFY(!l.MayClose());
    l.Settled();
    QVERIFY(l.MayClose());

    l.Settled();
    QCOMPARE(l.InFlight(), 0);
    l.Started();
    QVERIFY(!l.MayClose());
}

void tst_edgestate::aDownloadNobodyCanWatchStopsTheTabClosingForGood(){
    EdgeDownloadCloseLedger l;
    l.Started();
    l.Untracked();

    QCOMPARE(l.InFlight(), 0);
    QVERIFY(l.IsUntracked());
    QVERIFY(!l.MayClose());

    l.Started();
    l.Settled();
    QCOMPARE(l.InFlight(), 0);
    QVERIFY(!l.MayClose());
}

void tst_edgestate::aProfileWhoseCookiesRemainIsRefusedTheJar(){
    EdgeCookieClearLedger l;
    const QString kept = QStringLiteral("space1");
    const QString cleared = QStringLiteral("space2");

    l.ClearStarted();
    l.AskPlaced();
    l.AskPlaced();
    l.AskSettled(cleared, true);
    l.AskSettled(kept, false);

    QVERIFY(l.IsDegraded(kept));
    QVERIFY(!l.IsDegraded(cleared));

    const int ticket = l.Ticket();
    QVERIFY(!l.MayWrite(ticket, kept));
    QVERIFY(l.MayWrite(ticket, cleared));
}

void tst_edgestate::aLaterSuccessfulClearingLetsThatProfileWriteAgain(){
    EdgeCookieClearLedger l;
    const QString key = QStringLiteral("space1");

    l.ClearStarted();
    l.AskPlaced();
    l.AskSettled(key, false);
    QVERIFY(!l.MayWrite(l.Ticket(), key));

    l.ClearStarted();
    l.AskPlaced();
    l.AskSettled(key, true);
    QVERIFY(!l.IsDegraded(key));
    QVERIFY(l.MayWrite(l.Ticket(), key));
}

void tst_edgestate::nothingIsHandedOverBeforeTheViewCanTakeIt(){
    EdgeDropLedger l;
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Over(),       EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Drop(),       EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::Nothing);
}

void tst_edgestate::aDragHandedOverIsClosedByItsLeave(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Over(),       EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Leave(),      EdgeDropLedger::Send::ToBackend);

    QCOMPARE(l.Leave(),      EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Over(),       EdgeDropLedger::Send::Nothing);
}

void tst_edgestate::aDragHandedOverIsClosedByItsDrop(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Drop(),       EdgeDropLedger::Send::ToBackend);

    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::Nothing);
}

void tst_edgestate::aDragHandedOverAndAbandonedOwesALeave(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Drop(),       EdgeDropLedger::Send::Nothing);
}

void tst_edgestate::aDragThisSideTakesIsNeverHandedOver(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Consume), EdgeDropLedger::Send::Nothing);
    QVERIFY(l.IsConsuming());
    QCOMPARE(l.Over(),      EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Drop(),      EdgeDropLedger::Send::Nothing);
    QVERIFY(!l.IsConsuming());

    EdgeDropLedger m;
    m.SetReady(true);
    QCOMPARE(m.Enter(EdgeDropLedger::Take::Consume), EdgeDropLedger::Send::Nothing);
    QCOMPARE(m.Abandon(),   EdgeDropLedger::Send::Nothing);

    EdgeDropLedger n;
    n.SetReady(true);
    QCOMPARE(n.Enter(EdgeDropLedger::Take::Consume), EdgeDropLedger::Send::Nothing);
    QCOMPARE(n.Leave(),     EdgeDropLedger::Send::Nothing);
}

void tst_edgestate::aDragOfAnOwnUrlIsForwardedAndRemembered(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::ForwardOwnUrl), EdgeDropLedger::Send::ToBackend);
    QVERIFY(l.IsForwardingOwnUrl());
    QVERIFY(!l.IsConsuming());
    QCOMPARE(l.Over(), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Drop(), EdgeDropLedger::Send::ToBackend);
    QVERIFY(!l.IsForwardingOwnUrl());

    EdgeDropLedger m;
    m.SetReady(true);
    QCOMPARE(m.Enter(EdgeDropLedger::Take::ForwardOwnUrl), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(m.Abandon(), EdgeDropLedger::Send::ToBackend);
    QVERIFY(!m.IsForwardingOwnUrl());

    EdgeDropLedger n;
    n.SetReady(true);
    QCOMPARE(n.Enter(EdgeDropLedger::Take::ForwardOwnUrl), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(n.Leave(), EdgeDropLedger::Send::ToBackend);

    EdgeDropLedger o;
    o.SetReady(true);
    QCOMPARE(o.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);
    QVERIFY(!o.IsForwardingOwnUrl());
}

void tst_edgestate::theDropEchoIsClaimedOnceAndOnlyFresh(){
    const QUrl dropped(QStringLiteral("https://example.com/dropped"));
    const QUrl other(QStringLiteral("https://example.com/other"));

    EdgeDropEchoLedger l;
    QVERIFY(!l.IsArmed());
    l.Arm(dropped, 1000);
    QVERIFY(l.IsArmed());
    QVERIFY(l.Claim(dropped, 1001));
    QVERIFY(!l.IsArmed());
    QVERIFY(!l.Claim(dropped, 1002));

    EdgeDropEchoLedger m;
    m.Arm(dropped, 1000);
    QVERIFY(!m.Claim(dropped, 1000 + EdgeDropEchoLedger::WindowMs + 1));
    QVERIFY(!m.IsArmed());
    m.Arm(dropped, 5000);
    QVERIFY(m.Claim(dropped, 5000 + EdgeDropEchoLedger::WindowMs));

    EdgeDropEchoLedger n;
    n.Arm(dropped, 1000);
    QVERIFY(!n.Claim(other, 1001));
    QVERIFY(n.IsArmed());
    QVERIFY(n.Claim(dropped, 1002));

    EdgeDropEchoLedger o;
    o.Arm(dropped, 1000);
    o.Arm(other, 1100);
    QVERIFY(!o.Claim(dropped, 1101));
    QVERIFY(o.Claim(other, 1102));

    EdgeDropEchoLedger p;
    p.Arm(QUrl(), 1000);
    QVERIFY(!p.IsArmed());
    QVERIFY(!p.Claim(QUrl(), 1001));
}

void tst_edgestate::aDropWhichNeverReachedTheBackendTakesItsArmBack(){
    EdgeDropEchoLedger l;
    const QUrl url(QStringLiteral("https://example.com/one"));

    l.Arm(url, 0);
    QVERIFY(l.IsArmed());
    l.Disarm();
    QVERIFY(!l.IsArmed());
    QVERIFY(!l.Claim(url, 1));

    l.Disarm();
    l.Arm(url, 2);
    QVERIFY(l.Claim(url, 3));
}

void tst_edgestate::hidingDoesNotLoseTheEndTheBackendIsOwed(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);

    l.SetReady(false);
    QVERIFY(!l.IsReady());
    QCOMPARE(l.Over(),       EdgeDropLedger::Send::Nothing);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::Nothing);

    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::Nothing);

    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);
}

void tst_edgestate::abandoningADragTwiceOwesNothingTheSecondTime(){
    EdgeDropLedger l;
    l.SetReady(true);
    QCOMPARE(l.Enter(EdgeDropLedger::Take::Forward), EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::ToBackend);
    QCOMPARE(l.Abandon(),    EdgeDropLedger::Send::Nothing);
}

void tst_edgestate::onlyOneDragLeavesAPageAtATime(){
    EdgeDragOutLedger l;
    QVERIFY(l.Begin(7, 0x1000));
    QVERIFY(l.IsActive());
    QVERIFY(!l.Begin(9, 0x2000));

    l.End(7);
    QVERIFY(!l.IsActive());
    QVERIFY(l.Begin(9, 0x2000));
}

void tst_edgestate::aDragWhichCouldNotStartLeavesTheRunningOneAlone(){
    EdgeDragOutLedger l;
    QVERIFY(l.Begin(7, 0x1000));
    QVERIFY(!l.Begin(9, 0x2000));

    QCOMPARE(l.Owner(), 7);
    QVERIFY(l.Matches(0x1000));
    QVERIFY(!l.Matches(0x2000));
    QVERIFY(!l.IsCancelled());
    QVERIFY(l.IsActive());
}

void tst_edgestate::onlyTheOwnerEndsOrCancelsItsDrag(){
    EdgeDragOutLedger l;
    QVERIFY(l.Begin(7, 0x1000));

    l.Cancel(9);
    QVERIFY(!l.IsCancelled());
    l.End(9);
    QVERIFY(l.IsActive());
    QCOMPARE(l.Owner(), 7);

    l.Cancel(7);
    QVERIFY(l.IsCancelled());
    l.End(7);
    QVERIFY(!l.IsActive());
}

void tst_edgestate::cancellingLeavesTheDragInFlightUntilItEnds(){
    EdgeDragOutLedger l;
    QVERIFY(l.Begin(7, 0x1000));
    l.Cancel(7);

    QVERIFY(l.IsActive());
    QVERIFY(l.IsCancelled());
    QVERIFY(l.Matches(0x1000));

    l.End(7);
    QVERIFY(!l.IsCancelled());
    QVERIFY(l.Begin(7, 0x3000));
    QVERIFY(!l.IsCancelled());
}

void tst_edgestate::theDataBeingDraggedIsRecognisedByIdentity(){
    EdgeDragOutLedger l;
    QVERIFY(!l.Matches(0x1000));

    QVERIFY(l.Begin(7, 0x1000));
    QVERIFY(l.Matches(0x1000));
    QVERIFY(!l.Matches(0x2000));
    QVERIFY(!l.Matches(0));

    l.End(7);
    QVERIFY(!l.Matches(0x1000));

    QVERIFY(!l.Begin(7, 0));
    QVERIFY(!l.Begin(0, 0x1000));
    QVERIFY(!l.IsActive());
}

void tst_edgestate::endingOrCancellingWithNoDragIsNothing(){
    EdgeDragOutLedger l;
    l.End(7);
    l.Cancel(7);
    QVERIFY(!l.IsActive());
    QVERIFY(!l.IsCancelled());
    QCOMPARE(l.Owner(), 0);

    QVERIFY(l.Begin(7, 0x1000));
    l.End(7);
    l.End(7);
    l.Cancel(7);
    QVERIFY(!l.IsActive());
    QVERIFY(!l.IsCancelled());
}

void tst_edgestate::theCursorsAPageCanAskForBecomeTheirQtShapes(){
    QCOMPARE(EdgeCursorShape(32512), Qt::ArrowCursor);
    QCOMPARE(EdgeCursorShape(32513), Qt::IBeamCursor);
    QCOMPARE(EdgeCursorShape(32649), Qt::PointingHandCursor);
    QCOMPARE(EdgeCursorShape(32648), Qt::ForbiddenCursor);

    QCOMPARE(EdgeCursorShape(32642), Qt::SizeFDiagCursor);
    QCOMPARE(EdgeCursorShape(32643), Qt::SizeBDiagCursor);
    QCOMPARE(EdgeCursorShape(32644), Qt::SizeHorCursor);
    QCOMPARE(EdgeCursorShape(32645), Qt::SizeVerCursor);
}

void tst_edgestate::aCursorWithNoSystemIdIsAnArrow(){
    QCOMPARE(EdgeCursorShape(0), Qt::ArrowCursor);
    QCOMPARE(EdgeCursorShape(1), Qt::ArrowCursor);
    QCOMPARE(EdgeCursorShape(32647), Qt::ArrowCursor);
    QCOMPARE(EdgeCursorShape(0xffffffffu), Qt::ArrowCursor);
}

namespace {

    struct FakeEventSource {
        ULONG m_Refs;
        int m_Things;
        int m_Others;
        EventRegistrationToken m_LastToken;
        HRESULT m_Answer;

        FakeEventSource()
            : m_Refs(1), m_Things(0), m_Others(0), m_LastToken(), m_Answer(S_OK)
        {
        }

        ULONG STDMETHODCALLTYPE AddRef(){ return ++m_Refs;}
        ULONG STDMETHODCALLTYPE Release(){ return --m_Refs;}

        HRESULT STDMETHODCALLTYPE remove_Thing(EventRegistrationToken token){
            m_Things++;
            m_LastToken = token;
            return m_Answer;
        }
        HRESULT STDMETHODCALLTYPE remove_Other(EventRegistrationToken token){
            m_Others++;
            m_LastToken = token;
            return m_Answer;
        }
    };

    EventRegistrationToken TokenOf(INT64 value){
        EventRegistrationToken token = {};
        token.value = value;
        return token;
    }
}

static_assert(!std::is_copy_constructible<EdgeEventSubscriptions>::value,
              "EdgeEventSubscriptions must not be copied");
static_assert(!std::is_copy_assignable<EdgeEventSubscriptions>::value,
              "EdgeEventSubscriptions must not be copied");
static_assert(!std::is_move_constructible<EdgeEventSubscriptions>::value,
              "EdgeEventSubscriptions must not be moved");
static_assert(!std::is_move_assignable<EdgeEventSubscriptions>::value,
              "EdgeEventSubscriptions must not be moved");

void tst_edgestate::everySubscriptionIsUndoneExactlyOnce(){
    EdgeEventSubscriptions s;
    QVERIFY(s.IsEmpty());
    QVERIFY(!s.IsRevoked());

    int one = 0, two = 0, three = 0;
    s.Keep([&one](){ one++;});
    s.Keep([&two](){ two++;});
    s.Keep([&three](){ three++;});
    QCOMPARE(s.Count(), 3);

    s.RevokeAll();
    QCOMPARE(one, 1);
    QCOMPARE(two, 1);
    QCOMPARE(three, 1);
    QVERIFY(s.IsEmpty());
    QVERIFY(s.IsRevoked());
}

void tst_edgestate::subscriptionsAreUndoneInTheOrderTheyWereMade(){
    EdgeEventSubscriptions s;
    QStringList order;
    s.Keep([&order](){ order << QStringLiteral("first");});
    s.Keep([&order](){ order << QStringLiteral("second");});
    s.Keep([&order](){ order << QStringLiteral("third");});
    s.RevokeAll();
    QCOMPARE(order, QStringList()
             << QStringLiteral("first")
             << QStringLiteral("second")
             << QStringLiteral("third"));
}

void tst_edgestate::revokingTwiceUndoesNothingTwice(){
    EdgeEventSubscriptions s;
    int calls = 0;
    s.Keep([&calls](){ calls++;});
    s.RevokeAll();
    QCOMPARE(calls, 1);
    s.RevokeAll();
    s.RevokeAll();
    QCOMPARE(calls, 1);
}

void tst_edgestate::aRevokerWhichComesBackHereFindsNothingLeft(){
    EdgeEventSubscriptions s;
    int one = 0, two = 0, three = 0;
    bool insideWasEmpty = false;
    bool insideSaidRevoked = false;

    s.Keep([&](){
        one++;
        insideWasEmpty = s.IsEmpty();
        insideSaidRevoked = s.IsRevoked();
        s.RevokeAll();
    });
    s.Keep([&two, &one](){ two++; QCOMPARE(one, 1);});
    s.Keep([&three, &two](){ three++; QCOMPARE(two, 1);});

    s.RevokeAll();
    QVERIFY(insideWasEmpty);
    QVERIFY(insideSaidRevoked);
    QCOMPARE(one, 1);
    QCOMPARE(two, 1);
    QCOMPARE(three, 1);
    QVERIFY(s.IsEmpty());
}

void tst_edgestate::aRegistrationKeptAfterTheListWasRevokedIsDropped(){
    EdgeEventSubscriptions s;
    int late = 0;

    s.Keep([&](){ s.Keep([&late](){ late++;}); QCOMPARE(late, 0);});
    s.RevokeAll();
    QCOMPARE(late, 0);
    QVERIFY(s.IsEmpty());

    int later = 0;
    s.Keep([&later](){ later++;});
    QCOMPARE(later, 0);
    QVERIFY(s.IsEmpty());
}

void tst_edgestate::anEmptyRevokerIsNotARegistration(){
    EdgeEventSubscriptions s;
    s.Keep(std::function<void()>());
    QVERIFY(s.IsEmpty());
    QCOMPARE(s.Count(), 0);
    s.RevokeAll();
    QVERIFY(s.IsEmpty());
}

void tst_edgestate::theListIsNeitherCopiedNorMoved(){
    QVERIFY(!std::is_copy_constructible<EdgeEventSubscriptions>::value);
    QVERIFY(!std::is_copy_assignable<EdgeEventSubscriptions>::value);
    QVERIFY(!std::is_move_constructible<EdgeEventSubscriptions>::value);
    QVERIFY(!std::is_move_assignable<EdgeEventSubscriptions>::value);
}

void tst_edgestate::aTokenIsHandedBackToTheInterfaceItWasMadeOn(){
    FakeEventSource thing;
    FakeEventSource other;
    QCOMPARE(thing.m_Refs, ULONG(1));

    EdgeEventSubscriptions s;
    KeepEventToken(s, &thing, &FakeEventSource::remove_Thing, TokenOf(7));
    KeepEventToken(s, &other, &FakeEventSource::remove_Other, TokenOf(9));
    QCOMPARE(s.Count(), 2);

    QCOMPARE(thing.m_Refs, ULONG(2));
    QCOMPARE(other.m_Refs, ULONG(2));

    s.RevokeAll();

    QCOMPARE(thing.m_Things, 1);
    QCOMPARE(thing.m_Others, 0);
    QCOMPARE(thing.m_LastToken.value, INT64(7));
    QCOMPARE(other.m_Others, 1);
    QCOMPARE(other.m_Things, 0);
    QCOMPARE(other.m_LastToken.value, INT64(9));

    QCOMPARE(thing.m_Refs, ULONG(1));
    QCOMPARE(other.m_Refs, ULONG(1));
}

void tst_edgestate::aRemovalWhichFailsIsNotTriedAgain(){
    FakeEventSource source;
    source.m_Answer = E_FAIL;

    EdgeEventSubscriptions s;
    KeepEventToken(s, &source, &FakeEventSource::remove_Thing, TokenOf(3));
    s.RevokeAll();

    QCOMPARE(source.m_Things, 1);
    QVERIFY(s.IsEmpty());
    s.RevokeAll();
    QCOMPARE(source.m_Things, 1);
    QCOMPARE(source.m_Refs, ULONG(1));
}

void tst_edgestate::nothingIsKeptForASourceWhichIsNotThere(){
    EdgeEventSubscriptions s;
    FakeEventSource *nothing = nullptr;
    KeepEventToken(s, nothing, &FakeEventSource::remove_Thing, TokenOf(1));
    QVERIFY(s.IsEmpty());
    s.RevokeAll();
    QVERIFY(s.IsEmpty());
}

namespace {

    QString EdgeSourceTextOf(const QString &name){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                   QStringLiteral("/view/edge/") + name);
        if(!file.open(QIODevice::ReadOnly)) return QString();
        return QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
    }

    QString EdgeRegistrationSkeletonOf(const QString &name){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                   QStringLiteral("/view/edge/") + name);
        if(!file.open(QIODevice::ReadOnly)) return QString();

        const QString source =
            QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));

        QString out;
        out.reserve(source.length());

        enum { Code, Line, Block, Text, Char } state = Code;
        for(int i = 0; i < source.length(); i++){
            const QChar c = source.at(i);
            const QChar next = i + 1 < source.length() ? source.at(i + 1) : QChar();
            switch(state){
            case Code:
                if(c == QLatin1Char('/') && next == QLatin1Char('/')){ state = Line;  out += QLatin1Char(' ');}
                else if(c == QLatin1Char('/') && next == QLatin1Char('*')){ state = Block; out += QLatin1Char(' ');}
                else if(c == QLatin1Char('"')){ state = Text; out += QLatin1Char(' ');}
                else if(c == QLatin1Char('\'')){ state = Char; out += QLatin1Char(' ');}
                else out += c;
                break;
            case Line:
                if(c == QLatin1Char('\n')){ state = Code; out += c;}
                else out += QLatin1Char(' ');
                break;
            case Block:
                if(c == QLatin1Char('*') && next == QLatin1Char('/')){ state = Code; i++; out += QLatin1String("  ");}
                else out += c == QLatin1Char('\n') ? c : QLatin1Char(' ');
                break;
            case Text:
            case Char:
                if(c == QLatin1Char('\\')){ i++; out += QLatin1String("  "); break;}
                if((state == Text && c == QLatin1Char('"')) ||
                   (state == Char && c == QLatin1Char('\''))) state = Code;
                out += c == QLatin1Char('\n') ? c : QLatin1Char(' ');
                break;
            }
        }

        static const QRegularExpression handler(QStringLiteral("->\\s*HRESULT\\s*\\{"));
        forever {
            const QRegularExpressionMatch m = handler.match(out);
            if(!m.hasMatch()) break;
            const int open = m.capturedEnd() - 1;
            int depth = 0, close = open;
            for(; close < out.length(); close++){
                if(out.at(close) == QLatin1Char('{')) depth++;
                else if(out.at(close) == QLatin1Char('}')){ depth--; if(!depth) break;}
            }
            QString blank = out.mid(open, close - open + 1);
            for(int i = 0; i < blank.length(); i++)
                if(blank.at(i) != QLatin1Char('\n')) blank[i] = QLatin1Char(' ');
            out.replace(open, close - open + 1, blank);
        }
        return out;
    }
}

void tst_edgestate::everyRegistrationIsKeptOnTheListItComesOff(){
    QHash<QString, QString> listOf;
    listOf[QStringLiteral("webview")]               = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("webview2")]              = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("webview4")]              = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("webview8")]              = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("webview11")]             = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("webview14")]             = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("webview24")]             = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("m_Impl->m_WebView")]     = QStringLiteral("WEBVIEW");
    listOf[QStringLiteral("controller")]            = QStringLiteral("CONTROLLER");
    listOf[QStringLiteral("m_Impl->m_Composition")] = QStringLiteral("COMPOSITION");
    listOf[QStringLiteral("m_Impl->m_DragOut")]     = QStringLiteral("COMPOSITION");

    static const QRegularExpression step
        (QStringLiteral("(?<enters>^[A-Za-z_][\\w:<>&* ]*?EdgeWebView::(?<function>\\w+)\\s*\\()"
                        "|(?<call>(?:(?<source>[\\w>.\\-]+)->)?"
                        "(?<what>add_\\w+|QueryInterface|AddWebResourceRequestedFilter"
                        "|AddScriptToExecuteOnDocumentCreated|RegisterDragDrop)\\s*\\()"
                        "|(?<keep>VANILLA_KEEP_(?<list>\\w+)_EVENT\\s*\\(\\s*"
                        "(?<kept>[^,]+?)\\s*,\\s*(?<name>\\w+)\\s*,)"
                        "|(?<stop>VANILLA_STOP_IF_RETIRED\\s*\\(\\s*\\)"
                        "|m_State\\.IsRetired\\s*\\(\\s*\\))"
                        "|(?<sink>m_Impl->\\w+\\s*=[^=]"
                        "|\\bRegister[A-Z]\\w*\\s*\\(\\s*\\))"
                        "|(?<brace>[{}])"),
         QRegularExpression::MultilineOption);

    QStringList names;
    QDirIterator it(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                    QStringLiteral("/view/edge"),
                    QStringList() << QStringLiteral("*.cpp"), QDir::Files);
    while(it.hasNext()){ it.next(); names << it.fileName();}
    names.sort();
    QVERIFY2(names.length() >= 10,
             "the Edge sources were not found; check VANILLA_SOURCE_DIR");

    int registrations = 0;
    foreach(const QString &name, names){
        const QString source = EdgeRegistrationSkeletonOf(name);
        QVERIFY2(!source.isEmpty(),
                 qPrintable(QStringLiteral("%1 was not read; check VANILLA_SOURCE_DIR").arg(name)));

        bool registering = false;
        int depth = 0;
        QString owedSource, owedEvent;
        bool owed = false;
        int shallowestStop = -1;

        QRegularExpressionMatchIterator i = step.globalMatch(source);
        while(i.hasNext()){
            const QRegularExpressionMatch m = i.next();

            if(!m.captured(QStringLiteral("enters")).isEmpty()){
                QVERIFY2(owedEvent.isEmpty(),
                         qPrintable(QStringLiteral("%1 was registered and never kept")
                                    .arg(owedEvent)));
                registering = m.captured(QStringLiteral("function"))
                    .startsWith(QStringLiteral("Register"));
                depth = 0;
                owed = false;
                shallowestStop = -1;
                continue;
            }
            if(!m.captured(QStringLiteral("brace")).isEmpty()){
                depth += m.captured(QStringLiteral("brace")) == QStringLiteral("{") ? 1 : -1;
                continue;
            }
            if(!registering) continue;

            if(!m.captured(QStringLiteral("stop")).isEmpty()){
                if(shallowestStop == -1 || depth < shallowestStop) shallowestStop = depth;
                continue;
            }

            const bool isCall = !m.captured(QStringLiteral("call")).isEmpty();
            const bool isSink = !m.captured(QStringLiteral("sink")).isEmpty();
            if(isCall || isSink){
                const QString what = isCall ? m.captured(QStringLiteral("what"))
                                            : m.captured(QStringLiteral("sink")).trimmed();

                if(owed)
                    QVERIFY2(shallowestStop != -1 && shallowestStop <= depth,
                             qPrintable(QStringLiteral("%1 in %2 follows a call on the backend "
                                                       "with no retirement gate between")
                                        .arg(what, name)));
                owed = isCall;
                shallowestStop = -1;
                if(!isCall) continue;

                QVERIFY2(owedEvent.isEmpty(),
                         qPrintable(QStringLiteral("%1 was registered and never kept")
                                    .arg(owedEvent)));
                if(what.startsWith(QStringLiteral("add_"))){
                    owedSource = m.captured(QStringLiteral("source"));
                    owedEvent = what.mid(QStringLiteral("add_").length());
                }
                continue;
            }

            QVERIFY2(!owedEvent.isEmpty(),
                     qPrintable(QStringLiteral("a keep of %1 with no registration before it")
                                .arg(m.captured(QStringLiteral("name")))));
            QCOMPARE(m.captured(QStringLiteral("name")), owedEvent);

            QString kept = m.captured(QStringLiteral("kept"));
            if(kept.endsWith(QStringLiteral(".Get()")))
                kept.chop(QStringLiteral(".Get()").length());
            QCOMPARE(kept, owedSource);

            QVERIFY2(listOf.contains(kept),
                     qPrintable(QStringLiteral("%1 is an interface this test does not know")
                                .arg(kept)));
            QCOMPARE(m.captured(QStringLiteral("list")), listOf.value(kept));

            registrations++;
            owedSource.clear();
            owedEvent.clear();
        }
        QVERIFY2(owedEvent.isEmpty(),
                 qPrintable(QStringLiteral("%1 was registered and never kept").arg(owedEvent)));
    }

    QCOMPARE(registrations, 20);
}

void tst_edgestate::everyHandlerListComesOffBeforeTheControllerIsClosed(){
    const QString source =
        EdgeRegistrationSkeletonOf(QStringLiteral("edgewebview.cpp"));
    QVERIFY2(!source.isEmpty(), "edgewebview.cpp was not read; check VANILLA_SOURCE_DIR");

    static const QRegularExpression opens
        (QStringLiteral("^void EdgeWebView::Retire\\s*\\(\\s*\\)\\s*\\{"),
         QRegularExpression::MultilineOption);
    const QRegularExpressionMatch entered = opens.match(source);
    QVERIFY(entered.hasMatch());

    int depth = 0, ends = entered.capturedEnd() - 1;
    for(; ends < source.length(); ends++){
        if(source.at(ends) == QLatin1Char('{')) depth++;
        else if(source.at(ends) == QLatin1Char('}')){ depth--; if(!depth) break;}
    }
    const QString retire = source.mid(entered.capturedStart(),
                                      ends - entered.capturedStart());

    const QStringList marks = QStringList()
        << QStringLiteral("RemoveCompositionHandlers();")
        << QStringLiteral("RemoveWebViewHandlers();")
        << QStringLiteral("RemoveControllerHandlers();")
        << QStringLiteral("m_Controller->Close();")
        << QStringLiteral("m_Impl->m_WebView.Reset();")
        << QStringLiteral("m_Impl->m_Controller.Reset();");
    QList<int> at;
    foreach(const QString &mark, marks){
        QCOMPARE(retire.count(mark), 1);
        at << retire.indexOf(mark);
    }

    QVERIFY(at.at(0) < at.at(1));
    QVERIFY(at.at(1) < at.at(2));
    QVERIFY(at.at(2) < at.at(3));
    QVERIFY(at.at(3) < at.at(4));
    QVERIFY(at.at(3) < at.at(5));
}

void tst_edgestate::theDropSiteIsTakenInOnePieceOrNotAtAll(){
    const QString source =
        EdgeRegistrationSkeletonOf(QStringLiteral("edgewebviewdragdrop.cpp"));
    QVERIFY2(!source.isEmpty(), "edgewebviewdragdrop.cpp was not read");

    static const QRegularExpression opens
        (QStringLiteral("^void EdgeWebView::RegisterDropTarget\\s*\\(\\s*\\)\\s*\\{"),
         QRegularExpression::MultilineOption);
    const QRegularExpressionMatch entered = opens.match(source);
    QVERIFY(entered.hasMatch());

    int depth = 0, ends = entered.capturedEnd() - 1;
    for(; ends < source.length(); ends++){
        if(source.at(ends) == QLatin1Char('{')) depth++;
        else if(source.at(ends) == QLatin1Char('}')){ depth--; if(!depth) break;}
    }
    const QString taking = source.mid(entered.capturedStart(),
                                      ends - entered.capturedStart());

    QVERIFY2(!taking.contains(QStringLiteral("&m_Impl->")),
             "RegisterDropTarget answers a backend call into a member");

    const QStringList order = QStringList()
        << QStringLiteral("m_State.IsRetired()")
        << QStringLiteral("QueryInterface")
        << QStringLiteral("VANILLA_STOP_IF_RETIRED()")
        << QStringLiteral("RegisterDragDrop(")
        << QStringLiteral("m_State.IsRetired()")
        << QStringLiteral("Detach()")
        << QStringLiteral("RevokeDragDrop(")
        << QStringLiteral("return")
        << QStringLiteral("m_Impl->m_Drag = ")
        << QStringLiteral("m_Impl->m_DropTarget = ");

    int at = 0;
    foreach(const QString &step, order){
        const int found = taking.indexOf(step, at);
        QVERIFY2(found != -1,
                 qPrintable(QStringLiteral("RegisterDropTarget: '%1' is not where it "
                                           "should be, at or after %2").arg(step).arg(at)));
        at = found + step.length();
    }

    QVERIFY(taking.indexOf(QStringLiteral("Detach()")) <
            taking.indexOf(QStringLiteral("RevokeDragDrop(")));
}

void tst_edgestate::theViewHeaderCarriesNothingButThePointer(){
    const QString source = EdgeRegistrationSkeletonOf(QStringLiteral("edgewebview.hpp"));
    QVERIFY2(!source.isEmpty(), "edgewebview.hpp was not read; check VANILLA_SOURCE_DIR");

    static const QRegularExpression opens
        (QStringLiteral("^class EdgeWebView\\s*:[^{]*\\{"), QRegularExpression::MultilineOption);
    const QRegularExpressionMatch entered = opens.match(source);
    QVERIFY(entered.hasMatch());

    int depth = 0, ends = entered.capturedEnd() - 1;
    for(; ends < source.length(); ends++){
        if(source.at(ends) == QLatin1Char('{')) depth++;
        else if(source.at(ends) == QLatin1Char('}')){ depth--; if(!depth) break;}
    }
    const QString body = source.mid(entered.capturedEnd(), ends - entered.capturedEnd());

    static const QRegularExpression member(QStringLiteral("(?<!\\))\\b(m_\\w+)\\s*;"));
    QStringList found;
    QRegularExpressionMatchIterator it = member.globalMatch(body);
    while(it.hasNext()){
        const QRegularExpressionMatch m = it.next();
        int nested = 0;
        for(int i = 0; i < m.capturedStart(1); i++){
            if(body.at(i) == QLatin1Char('{')) nested++;
            else if(body.at(i) == QLatin1Char('}')) nested--;
        }
        if(nested == 0) found << m.captured(1);
    }

    QCOMPARE(found, QStringList() << QStringLiteral("m_Impl"));
    QVERIFY2(body.contains(QStringLiteral("std::unique_ptr<Private> m_Impl;")),
             "the one member is not the owning pointer any more");

    const QString raw = EdgeSourceTextOf(QStringLiteral("edgewebview.hpp"));
    QVERIFY(!raw.isEmpty());
    const QStringList gone = QStringList()
        << QStringLiteral("treebank.hpp")
        << QStringLiteral("mainwindow.hpp")
        << QStringLiteral("notifier.hpp")
        << QStringLiteral("networkcontroller.hpp")
        << QStringLiteral("edgewebviewstate.hpp");
    foreach(const QString &header, gone)
        QVERIFY2(!raw.contains(QStringLiteral("#include \"%1\"").arg(header)),
                 qPrintable(QStringLiteral("edgewebview.hpp includes %1 again").arg(header)));
}

void tst_edgestate::theDoorShutsOnANavigationAndOpensOnItsDocument(){
    EdgeDocumentCoordinator d;
    QVERIFY(!d.IsDocumentActive());
    QVERIFY(!d.HasLiveDocument());

    d.NavigationStarted();
    QVERIFY(!d.IsDocumentActive());
    d.DocumentArrived();
    QVERIFY(d.IsDocumentActive());
    QVERIFY(d.HasLiveDocument());
    const int generation = d.Generation();
    QVERIFY(d.StillCurrent(generation));

    d.NavigationStarted();
    QVERIFY(!d.IsDocumentActive());
    QVERIFY(!d.StillCurrent(generation));
    QVERIFY(!d.HasLiveDocument());

    d.DocumentArrived();
    QVERIFY(d.HasLiveDocument());
    QVERIFY(!d.StillCurrent(generation));
    QVERIFY(d.StillCurrent(d.Generation()));
}

void tst_edgestate::theDocumentWhichArrivesCorrectsTheAddressWhichWasGuessed(){
    const QString source = EdgeSourceTextOf(QStringLiteral("edgewebviewhandlers.cpp"));
    QVERIFY2(!source.isEmpty(), "the Edge sources were not read; check VANILLA_SOURCE_DIR");

    const int begin = source.indexOf(QStringLiteral("add_ContentLoading"));
    QVERIFY2(begin >= 0, "the ContentLoading registration has gone");
    const int end = source.indexOf(QStringLiteral("add_NavigationCompleted"), begin);
    QVERIFY2(end > begin, "the NavigationCompleted registration no longer follows it");

    const QString block = source.mid(begin, end - begin);
    QVERIFY2(block.contains(QStringLiteral("AdoptReportedSource(reported)")),
             "nothing corrects the address when 'SourceChanged' stays silent");
    QVERIFY2(block.contains(QStringLiteral("reported != m_Impl->m_Url")),
             "the guard has gone: the ordinary load would take a second correction");

    QVERIFY2(source.contains(QStringLiteral("AdoptReportedSource(ReportedSourceOf(sender))")),
             "'SourceChanged' no longer adopts the address it reports");
}

void tst_edgestate::nothingHereWritesIntoTheBackendsCookieStore(){
    QString sources;
    QDirIterator it(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)),
                    QStringList() << QStringLiteral("*.cpp") << QStringLiteral("*.hpp"),
                    QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext()){
        QFile file(it.next());
        if(!file.open(QIODevice::ReadOnly)) continue;
        sources += QString::fromUtf8(file.readAll()).remove(QLatin1Char('\r'));
    }
    QVERIFY2(!sources.isEmpty(), "the sources were not read; check VANILLA_SOURCE_DIR");

    QVERIFY2(!sources.contains(QStringLiteral("AddOrUpdateCookie")),
             "something puts a cookie into a backend's store again");
    QVERIFY2(!sources.contains(QStringLiteral("CreateCookie")),
             "something makes a cookie for a backend's store again");
    QVERIFY2(!sources.contains(QStringLiteral("cookieStore()->setCookie")),
             "something sets a cookie on an engine profile again");
    QVERIFY2(!sources.contains(QStringLiteral("\"setCookie\"")),
             "the native view seeds its store again");
    QVERIFY2(!sources.contains(QStringLiteral("PushJarCookies")),
             "the push is back");

    QVERIFY2(sources.contains(QStringLiteral("MirrorScope")),
             "the pull no longer mirrors the store into the jar");
}

void tst_edgestate::theBackendsTwoEventsAreWiredToTheDoor(){
    QString sources;
    QDirIterator it(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR)) +
                    QStringLiteral("/view/edge"),
                    QStringList() << QStringLiteral("*.cpp"), QDir::Files);
    while(it.hasNext()){
        it.next();
        sources += EdgeSourceTextOf(it.fileName());
    }
    QVERIFY2(!sources.isEmpty(), "the Edge sources were not read; check VANILLA_SOURCE_DIR");

    QVERIFY2(sources.contains(QStringLiteral("m_Document.NavigationStarted()")),
             "nothing shuts the door: a navigation has to reach the coordinator");
    QVERIFY2(sources.contains(QStringLiteral("m_Document.DocumentArrived()")),
             "nothing opens the door: the document has to reach the coordinator");
}

void tst_edgestate::aPrivateViewIsRefusedRatherThanMadeOrdinary(){
    QCOMPARE(EdgeAnswerForControllerOptions(true, true),
             EdgeControllerOptionsAnswer::Proceed);
    QCOMPARE(EdgeAnswerForControllerOptions(false, true),
             EdgeControllerOptionsAnswer::Proceed);

    QCOMPARE(EdgeAnswerForControllerOptions(true, false),
             EdgeControllerOptionsAnswer::RefusePrivate);
    QCOMPARE(EdgeAnswerForControllerOptions(false, false),
             EdgeControllerOptionsAnswer::ShareDefaultProfile);
}

void tst_edgestate::theHostingFallsBackInOneOrder(){
    QCOMPARE(EdgeChooseControllerCreation(true, true),
             EdgeControllerCreation::Composition);
    QCOMPARE(EdgeChooseControllerCreation(true, false),
             EdgeControllerCreation::WindowedWithOptions);
    QCOMPARE(EdgeChooseControllerCreation(false, true),
             EdgeControllerCreation::WindowedPlain);
    QCOMPARE(EdgeChooseControllerCreation(false, false),
             EdgeControllerCreation::WindowedPlain);
}

namespace {

    struct FakeController {
        ULONG m_Refs;
        int m_Closes;

        FakeController() : m_Refs(1), m_Closes(0) {}

        ULONG STDMETHODCALLTYPE AddRef(){ return ++m_Refs;}
        ULONG STDMETHODCALLTYPE Release(){ return --m_Refs;}
        HRESULT STDMETHODCALLTYPE Close(){ m_Closes++; return S_OK;}
    };
}

void tst_edgestate::aControllerNobodyTookIsClosedOnce(){
    FakeController controller;
    QCOMPARE(controller.m_Refs, ULONG(1));

    {
        EdgeUnadoptedControllerOf<FakeController> made(&controller);
        QVERIFY(made.IsHeld());
        QCOMPARE(made.Get(), &controller);
        QCOMPARE(controller.m_Closes, 0);
        QCOMPARE(controller.m_Refs, ULONG(2));
    }

    QCOMPARE(controller.m_Closes, 1);
    QCOMPARE(controller.m_Refs, ULONG(1));
}

void tst_edgestate::aControllerWhichWasTakenIsNotClosed(){
    FakeController controller;
    Microsoft::WRL::ComPtr<FakeController> adopted;
    {
        EdgeUnadoptedControllerOf<FakeController> made(&controller);
        adopted = made.Take();
        QCOMPARE(adopted.Get(), &controller);
        QVERIFY(!made.IsHeld());
        QVERIFY(!made.Take());
    }

    QCOMPARE(controller.m_Closes, 0);
    QVERIFY(adopted);
    QCOMPARE(controller.m_Refs, ULONG(2));
}

void tst_edgestate::theHolderIsNeitherCopiedNorMoved(){
    QVERIFY(!std::is_copy_constructible<EdgeUnadoptedControllerOf<FakeController>>::value);
    QVERIFY(!std::is_copy_assignable<EdgeUnadoptedControllerOf<FakeController>>::value);
    QVERIFY(!std::is_move_constructible<EdgeUnadoptedControllerOf<FakeController>>::value);
    QVERIFY(!std::is_move_assignable<EdgeUnadoptedControllerOf<FakeController>>::value);
}

namespace {

    class FakeView : public QObject {};
}

void tst_edgestate::onlyTheAnswersWithSomethingToWaitForJoinTheList(){
    EdgeProfileCoordinatorOf<FakeView> profiles;
    FakeView first, second, third;
    const QString key = QStringLiteral("p|private");

    QCOMPARE(profiles.EnterPrivateProfile(key, &first),
             EdgePrivateWipeLedger::Effect::Wipe);
    QCOMPARE(profiles.WaitersFor(key), 1);

    QCOMPARE(profiles.EnterPrivateProfile(key, &second),
             EdgePrivateWipeLedger::Effect::Wait);
    QCOMPARE(profiles.WaitersFor(key), 2);

    profiles.SettlePrivateWipe(key, true);
    QCOMPARE(profiles.EnterPrivateProfile(key, &third),
             EdgePrivateWipeLedger::Effect::Proceed);
    QCOMPARE(profiles.WaitersFor(key), 0);
}

void tst_edgestate::aProfileWhichCouldNotBeEmptiedKeepsNobodyWaiting(){
    EdgeProfileCoordinatorOf<FakeView> profiles;
    FakeView first, second;
    const QString key = QStringLiteral("p|private");

    profiles.EnterPrivateProfile(key, &first);
    profiles.SettlePrivateWipe(key, false);

    QCOMPARE(profiles.EnterPrivateProfile(key, &second),
             EdgePrivateWipeLedger::Effect::Fail);
    QCOMPARE(profiles.WaitersFor(key), 0);
    QVERIFY(!profiles.IsPrivateProfileClean(key));
}

void tst_edgestate::aSettlementEmptiesTheListBeforeItHandsItOver(){
    EdgeProfileCoordinatorOf<FakeView> profiles;
    FakeView first, second;
    const QString key = QStringLiteral("p|private");
    profiles.EnterPrivateProfile(key, &first);
    profiles.EnterPrivateProfile(key, &second);

    const QList<QPointer<FakeView>> waiters = profiles.SettlePrivateWipe(key, true);
    QCOMPARE(waiters.length(), 2);
    QCOMPARE(profiles.WaitersFor(key), 0);
    QVERIFY(profiles.IsPrivateProfileClean(key));

    QCOMPARE(profiles.SettlePrivateWipe(key, true).length(), 0);
}

void tst_edgestate::aViewWhichGaveEverythingBackIsOffEveryList(){
    EdgeProfileCoordinatorOf<FakeView> profiles;
    FakeView staying, going;
    const QString key = QStringLiteral("p|private");

    profiles.Opened(&staying);
    profiles.Opened(&going);
    QCOMPARE(profiles.LiveViews().length(), 2);

    profiles.EnterPrivateProfile(key, &staying);
    profiles.EnterPrivateProfile(key, &going);
    QCOMPARE(profiles.WaitersFor(key), 2);

    profiles.Closed(&going);
    QCOMPARE(profiles.LiveViews(), QList<FakeView*>() << &staying);
    QCOMPARE(profiles.WaitersFor(key), 1);

    profiles.Closed(&going);
    QCOMPARE(profiles.LiveViews().length(), 1);
    QCOMPARE(profiles.WaitersFor(key), 1);

    const QList<QPointer<FakeView>> waiters = profiles.SettlePrivateWipe(key, true);
    QCOMPARE(waiters.length(), 1);
    QCOMPARE(waiters.first().data(), &staying);
}

void tst_edgestate::aWaiterWhoseObjectWentIsSweptWithTheNextRetirement(){
    EdgeProfileCoordinatorOf<FakeView> profiles;
    FakeView staying;
    const QString key = QStringLiteral("p|private");

    {
        FakeView going;
        profiles.EnterPrivateProfile(key, &going);
        profiles.EnterPrivateProfile(key, &staying);
        QCOMPARE(profiles.WaitersFor(key), 2);
    }
    QCOMPARE(profiles.WaitersFor(key), 2);

    profiles.Closed(&staying);
    QCOMPARE(profiles.WaitersFor(key), 0);
}

void tst_edgestate::aControllerArrivingAfterTheViewIsGoneIsClosedOnce(){
    FakeController controller;
    bool adopted = false;
    bool failed = false;

    QPointer<FakeView> view;
    {
        FakeView v;
        view = &v;
    }
    QVERIFY(view.isNull());

    EdgeSettleControllerArrival
        (view, true, &controller,
         [&](EdgeUnadoptedControllerOf<FakeController>&){ adopted = true;},
         [&](){ failed = true;});

    QVERIFY(!adopted);
    QVERIFY(!failed);
    QCOMPARE(controller.m_Closes, 1);
    QCOMPARE(controller.m_Refs, ULONG(1));
}

void tst_edgestate::aFailureArrivingAfterTheViewIsGoneTouchesNothing(){
    bool failed = false;

    QPointer<FakeView> view;
    {
        FakeView v;
        view = &v;
    }

    EdgeSettleControllerArrival
        (view, false, static_cast<FakeController*>(nullptr),
         [&](EdgeUnadoptedControllerOf<FakeController>&){},
         [&](){ failed = true;});

    QVERIFY(!failed);
}

void tst_edgestate::aSuccessWithNothingAfterTheViewIsGoneTouchesNothing(){
    bool adopted = false;
    bool failed = false;

    QPointer<FakeView> view;
    {
        FakeView v;
        view = &v;
    }

    EdgeSettleControllerArrival
        (view, true, static_cast<FakeController*>(nullptr),
         [&](EdgeUnadoptedControllerOf<FakeController>&){ adopted = true;},
         [&](){ failed = true;});

    QVERIFY(!adopted);
    QVERIFY(!failed);
}

void tst_edgestate::aControllerArrivingForALiveViewIsHandedOver(){
    FakeController controller;
    FakeView v;
    QPointer<FakeView> view(&v);
    bool failed = false;
    Microsoft::WRL::ComPtr<FakeController> kept;

    EdgeSettleControllerArrival
        (view, true, &controller,
         [&](EdgeUnadoptedControllerOf<FakeController> &made){
        QVERIFY(made.IsHeld());
        kept = made.Take();
    },
         [&](){ failed = true;});

    QVERIFY(!failed);
    QVERIFY(kept);
    QCOMPARE(controller.m_Closes, 0);
}

void tst_edgestate::aFailureArrivingForALiveViewIsReportedOnce(){
    FakeView v;
    QPointer<FakeView> view(&v);
    bool adopted = false;
    int failures = 0;

    EdgeSettleControllerArrival
        (view, false, static_cast<FakeController*>(nullptr),
         [&](EdgeUnadoptedControllerOf<FakeController>&){ adopted = true;},
         [&](){ ++failures;});

    QVERIFY(!adopted);
    QCOMPARE(failures, 1);
}

void tst_edgestate::aSuccessWhichCarriesNoControllerIsAFailure(){
    FakeView v;
    QPointer<FakeView> view(&v);
    bool adopted = false;
    int failures = 0;

    EdgeSettleControllerArrival
        (view, true, static_cast<FakeController*>(nullptr),
         [&](EdgeUnadoptedControllerOf<FakeController>&){ adopted = true;},
         [&](){ ++failures;});

    QVERIFY(!adopted);
    QCOMPARE(failures, 1);
}

void tst_edgestate::whatTheBackendSaysAboutACookieIsCarriedOver(){
    const QNetworkCookie made =
        EdgeCookieFromParts(QStringLiteral("sid"), QStringLiteral("abc"),
                            QStringLiteral(".example.com"), QStringLiteral("/dir"),
                            1000000000.0, true, true);

    QCOMPARE(made.name(), QByteArrayLiteral("sid"));
    QCOMPARE(made.value(), QByteArrayLiteral("abc"));
    QCOMPARE(made.domain(), QStringLiteral(".example.com"));
    QCOMPARE(made.path(), QStringLiteral("/dir"));
    QVERIFY(made.isHttpOnly());
    QVERIFY(made.isSecure());

    QVERIFY(!made.isSessionCookie());
    QCOMPARE(made.expirationDate(), QDateTime::fromSecsSinceEpoch(1000000000));

    const QNetworkCookie plain =
        EdgeCookieFromParts(QStringLiteral("sid"), QStringLiteral("abc"),
                            QStringLiteral(".example.com"), QStringLiteral("/"),
                            1000000000.0, false, false);
    QVERIFY(!plain.isHttpOnly());
    QVERIFY(!plain.isSecure());
}

void tst_edgestate::anExpiryWhichIsNotOneLeavesASessionCookie(){
    for(const double expires : {-1.0, 0.0}){
        const QNetworkCookie made =
            EdgeCookieFromParts(QStringLiteral("sid"), QStringLiteral("abc"),
                                QStringLiteral("example.com"), QStringLiteral("/"),
                                expires, false, false);
        QVERIFY2(made.isSessionCookie(), QByteArray::number(expires));
    }
}

void tst_edgestate::aPathTheBackendDidNotGiveIsNotAPath(){
    const QNetworkCookie made =
        EdgeCookieFromParts(QStringLiteral("sid"), QStringLiteral("abc"),
                            QStringLiteral("example.com"), QString(),
                            -1.0, false, false);
    QVERIFY(made.path().isEmpty());
    QCOMPARE(made.domain(), QStringLiteral("example.com"));
}

QTEST_MAIN(tst_edgestate)
#include "tst_edgestate.moc"

#else

class tst_edgestate : public QObject {
    Q_OBJECT
private slots:
    void theEdgeViewIsNotBuilt(){ QSKIP("built without EDGEWEBVIEW.");}
};

QTEST_MAIN(tst_edgestate)
#include "tst_edgestate.moc"

#endif
