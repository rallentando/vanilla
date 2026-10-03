#include "switch.hpp"

#include <QtTest>
#include <QCoreApplication>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "cdpshims.hpp"
#include "extensionhostwire.hpp"

#include "testsupport.hpp"

static QString Fetching();

class tst_cdpshims : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void theShimRunAsAWorkerWouldRunIt();

    void syncIsKeptInLocalUnderAPrefix();
    void nothingIsReadUntilSomebodyListens();
    void onlyAnAreaSomebodyHearsOfIsRead();
    void whatThisContextWritesIsToldOfAtOnce();
    void whatOthersWriteIsFoundByAskingAgainInAPageOnly();
    void anAreaWhichWouldNotBeReadStartsAsEmpty();
    void aLookWhichComesBackLateIsDropped();
    void aValueWithNoJsonDoesNotEndTheLooking();
    void aFailedWriteIsNotASuccess();
    void aMessageSaysWhichTabAndFrameItIsFrom();
    void aMissingNonceKeepsDocumentsApart();
    void aContentScriptNamesItsDocumentToTheHost();
    void theWorkerAsksTheHostWhichTabANumberIsOf();
    void aCertainTabIdOutlivesWorkerCachePressure();
    void theAskingAgainStopsAfterItsRounds();
    void aLaterRoundDeliversWhatItHoldsByItsDeadline();
    void theAskingAgainKeepsOneChainAcrossAnEviction();
    void whatWaitsIsHandedOverInOrderAndTheLostFallBack();
    void manyListenersAreFoldedAsChromeWouldFoldThem();
    void aKeptChannelCanBeWonByAPromise();
    void aKeptChannelIsNotClosedByAPromiseWithNothing();
    void theAnswerMayBeTheSecondPromise();
    void nobodyAnsweringClosesOnlyWhatTheEngineWaitsFor();
    void everyPromiseComingToNothingClosesTheGateOnce();
    void aPromiseAloneIsToldToTheEngineAsAKeptChannel();
    void aLoneListenersPromiseIsWaitedOnByTheShim();
    void aLoneListenerWithNoPromiseGoesToTheEngineAsItIs();
    void aThenableWhichCallsBackAndThrowsIsCountedOnce();
    void theFastPathAnswersOnlyOnce();
    void aThrownFirstResponseDoesNotCloseTheGate();
    void aKeptChannelMayAnswerAfterTheLookupDeadline();
    void aFrameSaysTheAddressOfTheTopOfItsTab();
    void whatTheEngineWillNotLetBePutInIsSaid();
    void whatTheApplicationAnswersForIsAskedOfIt();
    void theTabsAndWindowsTheApplicationChangesAreAskedOfIt();
    void anAnswerWithNoValueResolvesWithNothing();
    void whatTheApplicationAnswersForBeatsTheEnginesOwn();
    void withoutAKeyTheStandInAnswersRatherThanTheEnginesOwn();
    void theEnginesOwnCallsAndEventsAreNotPassedThroughButItsNumbersAre();
    void whatTakesTheEngineDownIsNotPassedThrough();
    void whatTheManifestGrantsIsAnsweredOutOfIt();
    void whichRulesetsAreEnabledIsAskedOfTheHost();
    void anExtensionsOwnRulesAreHeldByTheHost();
    void whatTheExtensionPutsOnANamespaceIsReadBack();
    void theStandInsOutliveTheEnginesOwnBeingBuiltAgain();
    void whatIsNobodysStandInIsReadOffTheEnginesObject();
    void theSecondRootTheEngineHasIsStoodInForToo();
    void anExtensionsPageStandsInOnTheSecondRootToo();
    void whereTheEngineHasNoSecondRootNoneIsMade();
    void theNamesAStandInAnswersForAreChromesAndInIsWider();
    void theIndexOfASendersTabIsReadFromTheHostsAnswers();
    void aTabOfAnswerOfAnotherShapeIsNoAnswer();
    void withoutAKeyNobodyIsAsked();
    void theTreeTheApplicationKeepsIsAskedOfIt();
    void aNodeOfTheTreeIsNoTabHoweverItLooks();
    void theTreeIsReadByIdAndByWordOverTheSameWire();
    void theVisitsAreToldAndTheBookmarksEventsAreNot();
    void theFontsAreAskedOfTheApplication();
    void theTopSitesAreAskedOfTheApplication();
    void whatTurnsAnExtensionOffIsNotPassedThrough();
    void theEnginesIdleStateIsToldByTheWorker();
    void theSearchTheApplicationOpensIsAskedOfIt();
    void theZoomOfATabIsAskedOfTheApplication();
    void theTabsTheApplicationCopiesAndMovesAreAskedOfIt();
    void theClosedTabTheApplicationPutsBackIsAskedOfIt();
    void theMessagesTheCopyWroteAreAnsweredAsChromeAnswersThem();
    void theExtensionNamespaceAnswersNoInEachContext();
    void theButtonAndTheMenuAreAskedOfTheApplication();
    void theWorkerHasADocumentRunItsOwnFiles();
    void theWorkerRegistersContentScriptsForTheDocumentsWhichLink();
    void theListOfThePagesOwnWorldGoesToTheHostByName();
    void theWorkerPutsAStyleSheetIntoADocumentAndTakesItOut();
    void aDocumentRunsARegisteredScriptOnceWhenItIsParsed();
    void aStyleSheetOfTheWorkersIsPutInAndTakenOut();
    void aCarriersDocumentLinksWithNobodyListening();
    void aWorkerTellsACarrierWhetherItIsWanted();
    void aRegistrationReachesOnlyWhereTheManifestMayInject();
    void aStarvedDocumentIsGivenItsScriptsWhenRoomIsMade();
    void theOffscreenDocumentIsAmongTheContexts();
    void theOptionsPageIsOpenedAsATab();
    void aContentScriptTellsItsOwnWorkerItsNonce();
    void aTabWithNoLinkIsFoundByAskingItsDocument();
    void anEdgePageHasItsScriptingRunByTheWorker();
    void anEdgePageRunsAFuncInTheTabItLooksUp();
    void aDownloadIsAskedOfTheApplication();
    void whatIsDoneToADownloadIsAskedOfTheApplication();
    void aContentScriptLinksOnlyOnceSomebodyListens();
    void aHiddenDocumentNeitherGreetsNorPings();
    void thePingIsKeptOnlyWhileTheDocumentIsSeen();
    void everyTryDropsThePortBeforeIt();
    void theTryingIsBoundedAndComesBackOnlyByTheRules();
    void whatTheWorkerAsksIsFoldedAndPostedBack();
    void theLastListenerLeavingClosesTheLink();
    void aTimerWhichWasPutBackDoesNothingWithoutClearTimeout();
    void theDemuxIsToldToTheEngineAtOnceAndForGood();
    void theGreetingIsAnsweredAtOnceAndNotHandedOn();
    void theShimsOwnPortIsNotTheExtensionsToSee();
    void aPortWhichNamesItselfBadlyIsNoLink();
    void theWorkerSendsToEveryFrameOfATab();
    void theWaysASendMessageEnds();
    void getAllFramesTellsOfTheLinkedDocuments();
    void aLinkIsReachedByBothOfItsNumbers();
    void aSecondPortOfOneNonceRetiresTheFirst();
    void theFrameOfTheEnvelopeAndOfTheFramesAreOne();
    void whatTheShimsAnswerThemselvesBeatsTheEnginesOwn();
    void theNavigationEventsAreOursEvenWhereTheEngineHasThem();
    void aPortWhichDiedBeforeItLinkedIsStillAFailure();
    void anAnswerWhichWillNotGoOverThePortLeavesTheGateOpen();
    void aDocumentHiddenAfterItGaveUpTriesThePortAgain();
    void aPortOpenedWithNoGreetingIsNotWaitedOutOnceSeen();
    void aDocumentWhoseVisibilityCannotBeReadIsNotSeen();
    void tabZeroReachesNobody();
    void aDocumentWhichNeverAnswersDoesNotGrowItsTable();
    void aValueWhoseThenThrowsIsTheListenersFailure();
    void whatIsAskedForWronglyIsSaidSo();
    void aDocumentWhichGaveUpKeepsNothingWhenItIsSeenAgain();
    void aDocumentWhichConnectsAgainKeepsBothOfItsNumbers();
    void aTabIdWhichIsNoWholeNumberIsNoAnswer();
    void aFullTableIsOneDocumentAndNotTheOthers();
    void whenNoAnswerWillGoOverThePortTheGateIsClosedWithNothing();
    void outsideAnExtensionTheContentShimDoesNothing();

    void nothingIsAskedUntilTheExtensionListensForTheEvents();
    void theEventsAreHandedToTheListenersOfThatName();
    void theListenersAreThoseOfTheMomentTheAnswerCame();
    void theOrderOfAnAnswerWritesOverTheTableOfIndexes();
    void aStaleAnswerEndsThatLoopAndIsNoFailure();
    void theLastListenerLeavingStopsTheAsking();
    void theAskingIsBoundedAndComesBackWithASuccess();
    void theShimsOwnDeadlineIsAnotherRoundAndNotAFailure();
    void withoutAKeyTheEventsAreTheStandInsWhichNeverFire();
    void theWindowNumbersAreTheOnesChromeGives();
    void theLastListenerGoingWhileTheAnswerIsHandedOutIsNoBarToIt();
    void anOlderCallSettlesNothingOnceANewLoopHasBegun();
    void stoppingTakesBackTheWaitAndStartingAgainIsOneLoop();
    void theCallIsGivenUpWithWhateverTheWorldHas();
    void anAnswerOfTheOrderAloneIsNoEventAndNoFailure();

    void theDocumentsOwnNavigationsAreHeardOnlyOnceSomebodyListens();
    void theKindOfANavigationComesFromTheNavigateEvent();
    void theNavigationsOfOneHundredMillisecondsAreOneMessage();
    void aDocumentNobodyLooksAtTellsOfNoNavigation();
    void aLinkWhichDoesNotSayWhereItIsHeldIsTakenToBeWhereTheDocumentIs();
    void theNavigateHandlerTouchesNothingOfThePages();
    void withoutTheNavigationApiTheWindowsOwnEventsAreHeard();
    void theDocumentsNavigationsAreToldToTheExtension();
    void aNavigationOfAnotherOriginOrShapeIsDropped();
    void aNavigationWithNoNumberYetWaitsForOne();
    void aDocumentsFirstLinkIsItsCommit();
    void aDocumentClaimsFreshUntilItIsLinked();
    void aDocumentsLoadingIsToldOnceAfterItsCommit();
    void aSeenDocumentTellsHowFarItIsLoaded();
    void aFrameNamedByItsIdIsItsNewestDocument();
    void aListenerWithAUrlFilterIsNotRegistered();
    void whatThePageDidWhileThePortWasAnsweredIsCaughtUpWith();
    void aDocumentWhichPushedTellsTheWorkerWhereItIs();
    void theKindOfANavigationOutlivesNotBeingAbleToSendIt();
    void nothingOfThePagesNavigationsIsKeptWithNobodyListening();
    void anAddressTooLongToBelieveIsNotSent();
    void theOriginsWhichMatchAndTheOnesWhichDoNot();

    void outsideAnExtensionsPageTheShimDoesNothing();
    void anExtensionsPageGetsTheShimOnlyOnce();
    void anExtensionsPageHasSyncAndTheChangeEvents();
    void anExtensionsPageAsksTheApplicationNothing();
    void aPageHasNoPortsSoWhatTheyAnswerForFails();
    void aPagesStandInIsNoThenableAndHasNoJson();
    void aPageIsToldItIsNotInAnIncognitoWindow();
    void thePagesShimNeitherWakesTheWorkerNorReachesIt();
    void thePagesShimCarriesTheEnvelopeAndNotTheSubscription();
    void anExtensionsPageSpeaksInTheSameEnvelope();
    void theWorkerNamesAPageOfThisExtensionOrGivesItNoTab();
    void theTopFrameOfAPageIsNotWaitedForAndIsAskedAfterOnce();
    void aPageWithNoTabIsCorrectedWhenTheHostNamesItLate();
    void aPageWhoseSendMessageCannotBeWrappedSaysSoAndStillRelays();

    void aPingIsWhenTheWorkerLooksAtLocalAgain();
    void aWorkerWhichHearsOfNoneOfItLooksAtNothing();
    void theLookingAgainIsAtMostOneEveryTwoSeconds();
    void whatThisContextWritesDoesNotHoldOffThePingsLook();
    void nothingButAPingLooksAgain();
    void aLookingAgainWhichThrowsLeavesThePingAsItWas();
    void aLookWhichFailedSaysNothingOfWhatIsThere();
    void aWriteAndAPingLookingAtOnceTellOfEachChangeOnce();
    void aListenerWhichWritesFromInsideIsToldOfItsOwnWriteOnce();

    void aPageWithTheKeyAsksTheApplicationWithIt();
    void aPageWithTheKeyAsksForTheTreeAndOneWithoutAsksNobody();
    void anExtensionsPageKeepsItsStandInsToo();
    void aPagesSendMessageToATabIsSentToTheWorker();
    void aPagesSendMessageAnswersTheWayChromeDoes();
    void aPagesSendMessageOfTheWrongCountIsNotSentAtAll();
    void theWorkerSendsForAPageOfItsOwn();
    void onlyWhatTheEngineNamedThisExtensionsOwnIsRelayed();
    void whatIsNoCallOfOursIsNotRelayedAndIsSaidSo();
    void aPageHasItsScriptingRunByTheWorker();
    void theWorkerRunsAPagesScriptingForIt();
    void aRelayToAMadeUpNumberReachesNobody();
    void anAnswerNobodyIsLeftToHearIsDropped();
    void aWorkerTakesOverAsSoonAsItIsInstalled();
    void aWorkerAlreadyActiveIsNotAskedTwiceAndAWaitWhichFailsIsSwallowed();
    void neitherAContentScriptNorAPageSkipsWaiting();

    void whichEngineThisIsComesFromOneBrandAndNothingElse();
    void whatWebView2AnswersForItselfIsLeftToTheEngine();
    void whatIsStillTheApplicationsToAnswerOnWebView2();
    void anEdgePageAndContentScriptKeepTheEnginesMessagesAndStorage();
    void aWorkerOnWebView2NeverFetchesTheHost();
    void whatWaitsForTheRelayIsBoundedAndEndsOnItsDeadline();
    void theRelayHookIsWhatAnswersAWaitingCall();

    void theRelayPagesPortIsTakenAndNoOthersAre();
    void whatWaitedGoesOverTheRelaysPortInOrder();
    void theHookLetsGoAndASendWhichThrowsFailsThatCallAlone();
    void thePortGoingFailsWhatWaitedAndFoldsTheEventsRound();
    void nothingIsSentOverThePortWhichHasGone();
    void theRoundsOwnDeadlineOverTheRelayIsAnotherRoundAndNoFailure();
    void anAbortedRoundIsARoundWhichBrokeAndNeverAStaleOne();
    void aSecondRelayPortTakesTheFirstsPlace();
    void theWorkerAsksItsPageForTheRelayWhenItHasNoPort();
    void aTabWhichCameOverTheRelayIsWrittenDown();
    void whatWebView2FiresItselfIsNotSubscribedTo();
    void theRelayMakesOneRequestOfTheHostForEachTicket();
    void theRelayStopsAHeldCallWhenThePortGoesAndTellsTheHost();
    void theRelayOpensAPortAgainOnlyWhenTheWorkerAsks();
    void theRelayKeepsAtMostSixtyFourCallsOut();

    void aContentScriptAsksTheEngineWhichStorageItHas();
    void aStorageWhichAnswersIsLeftToTheEngine();
    void aContentScriptAsksTheEngineForTheMessagesToo();

    void aMenuClickOnWebView2HandsTheApplicationsTab();
    void aMenuClickWithNoTabOfOursIsHandedTheNoneIds();
    void aMenuClickIsAskedOnceAndHandedInTheEnginesOrder();
    void atMostSixteenMenuClicksWaitAndTheRestAreNotAsked();
    void theMenuClicksListenersAreReadAsEachIsHandedOver();
    void theMenuClickStandInIsOnBothRootsAndTheRestIsTheEngines();
    void noMenuClickStandInWhereItIsNotWanted();
    void theEnginesMenuCallsAreToldTogether();
    void aChoiceOfTheButtonsMenuComesOverTheRelayPage();
    void theRelaysScriptCarriesTheChoiceAsData();
    void theEnginesScriptingIsHandedTheEnginesTab();
    void theNewestCertainLinkWithAnEnginesTabIsTheOne();
    void noLinkIsNoTabAndTheEngineIsNotAsked();
    void theScriptingCallbackIsAnsweredAsChromesIs();
    void aTabIdWhichIsNoNumberGoesToTheEngineAsItIs();
    void theScriptingStandInIsOnBothRootsAndTheRestIsTheEngines();
    void noScriptingStandInWhereItIsNotWanted();
    void onInstalledIsToldWhatTheApplicationSaysIsOwed();
    void onInstalledIsTheEnginesWhereTheShimAsksNobody();
    void onStartupIsFiredOnceWhereTheApplicationSaysSo();
    void theShortcutsAreAskedOfAndToldByTheApplication();
    void onWebView2TheEnginesOwnEventsAreNotAskedFor();
    void theButtonIsPressedByTheExtensionItself();
    void theIdentityIsAChromeNobodySignedInto();
    void theEnginesAlarmsAreRungByTheWorker();
    void anAlarmClearedWhileTheEngineAnswersIsNotRung();
    void anAlarmUsedUpBeforeTheFirstLookIsRung();
    void aGuessIsNotPutInForWhatWasClearedOrGivenUpOn();
    void theUserScriptsAreTheApplications();
    void aUserScriptsMessageIsAnsweredOnce();
    void theUserScriptPreludeSendsWithItsSecretAlone();
    void theWordWhichWakesTheWorkerReachesNoListener();
    void theVisibleTabIsCapturedByTheApplication();
    void theSidePanelIsTheApplications();
    void theNotificationsAreTheApplicationsOnQtAndTheEnginesOnEdge();

private:
    static QString World(){
        return QStringLiteral(R"js(
            var self = this;
            var timers = [], timerId = 0;
            function setTimeout(f, ms){ timers.push({ f: f, ms: ms, id: ++timerId }); return timerId; }
            function clearTimeout(id){ timers = timers.filter(function(t){ return t.id !== id; }); }
            function runTimers(){ var due = timers; timers = []; due.forEach(function(t){ t.f(); }); return due.length; }
            function runNextTimer(){ if (!timers.length) return -1; timers.sort(function(a, b){ return a.ms - b.ms; }); var t = timers.shift(); t.f(); return t.ms; }
            var reads = [];
            function area(name){
                var data = {};
                var me = {
                    data: data, refuses: false, held: null,
                    get: function(keys){
                        reads.push(name);
                        if (me.refuses) return Promise.reject(new Error('Access to storage is not allowed from this context.'));
                        var out = {};
                        if (keys === null || keys === undefined) { for (var k in data) out[k] = data[k]; }
                        else if (typeof keys === 'string') { if (keys in data) out[keys] = data[keys]; }
                        else if (Array.isArray(keys)) keys.forEach(function(k){ if (k in data) out[k] = data[k]; });
                        else { for (var d in keys) out[d] = (d in data) ? data[d] : keys[d]; }
                        var copy = JSON.parse(JSON.stringify(out));
                        if (me.held) return new Promise(function(resolve){ me.held.push(function(){ resolve(copy); }); });
                        return Promise.resolve(copy);
                    },
                    set: function(items){
                        if (me.refuses) return Promise.reject(new Error('QUOTA_BYTES quota exceeded'));
                        for (var k in items) data[k] = items[k];
                        return Promise.resolve();
                    },
                    remove: function(keys){ (Array.isArray(keys) ? keys : [keys]).forEach(function(k){ delete data[k]; }); return Promise.resolve(); },
                    clear: function(){ for (var k in data) delete data[k]; return Promise.resolve(); }
                };
                return me;
            }
            var refusing = function(){ return Promise.reject(new Error('"sync" is not available in this instance of Chrome')); };
            var nobody = { addListener: function(){}, removeListener: function(){}, hasListener: function(){ return false; } };
            var localArea = area('local'), sessionArea = area('session');
            var sent = [];
            var replies = [];
            var listeners = [];
            // the ports the engine hands out, and what the other end of one
            // does: 'say' is a message arriving, 'die' is the engine telling
            // this end it is over. A disconnected port throws where it is
            // posted to, as Chrome's does.
            var ports = [], connects = [];
            function aPort(name, sender){
                var p = { name: name, sender: sender, posted: [], disconnected: 0, dead: false,
                          messages: [], gone: [],
                          onMessage: { addListener: function(f){ p.messages.push(f); },
                                       removeListener: function(f){ p.messages = p.messages.filter(function(g){ return g !== f; }); } },
                          onDisconnect: { addListener: function(f){ p.gone.push(f); },
                                          removeListener: function(f){ p.gone = p.gone.filter(function(g){ return g !== f; }); } },
                          throwOn: null,
                          postMessage: function(m){ if (p.dead) throw new Error('Attempting to use a disconnected port object');
                                                    if (p.throwOn !== null && m && m.value === p.throwOn) throw new Error('Could not serialize message.');
                                                    p.posted.push(m); },
                          disconnect: function(){ p.disconnected++; p.dead = true; },
                          say: function(m){ p.messages.slice().forEach(function(f){ f(m, p); }); },
                          die: function(){ p.dead = true; p.gone.slice().forEach(function(f){ f(p); }); },
                          last: function(){ return p.posted.length ? p.posted[p.posted.length - 1] : null; } };
                ports.push(p);
                return p;
            }
            var chrome = {
                runtime: { id: 'abcdefghijklmnopabcdefghijklmnop',
                           sendMessage: function(){ sent.push(Array.prototype.slice.call(arguments));
                                                    var r = replies.shift();
                                                    if (r === undefined) return Promise.resolve('answer');
                                                    return typeof r === 'function' ? r() : Promise.resolve(r); },
                           connect: function(info){ return aPort(info && info.name, { id: chrome.runtime.id, url: location.href, origin: 'https://a.example' }); },
                           onConnect: { addListener: function(f){ connects.push(f); }, removeListener: function(f){ connects = connects.filter(function(g){ return g !== f; }); },
                                        hasListener: function(f){ return connects.indexOf(f) >= 0; }, hasListeners: function(){ return connects.length > 0; } },
                           onMessage: { addListener: function(f){ listeners.push(f); }, removeListener: function(f){ listeners = listeners.filter(function(g){ return g !== f; }); },
                                        hasListener: function(f){ return listeners.indexOf(f) >= 0; },
                                        hasListeners: function(){ return listeners.length > 0; } } },
                storage: { local: localArea, session: sessionArea,
                           sync: { get: refusing, set: refusing, remove: refusing, clear: refusing, onChanged: nobody },
                           onChanged: nobody }
            };
            var document = { visibilityState: 'visible', visibility: [], readyState: 'complete', loaded: [], stages: [],
                             addEventListener: function(name, f){ if (name === 'visibilitychange') document.visibility.push(f);
                                                                  else if (name === 'DOMContentLoaded') document.loaded.push(f);
                                                                  else if (name === 'readystatechange') document.stages.push(f); },
                             onVisibility: function(){ document.visibility.slice().forEach(function(f){ f(); }); },
                             onLoaded: function(){ document.readyState = 'interactive'; document.stages.slice().forEach(function(f){ f({}); });
                                                   var all = document.loaded; document.loaded = []; all.forEach(function(f){ f(); }); } };
            var window = this; window.top = window;
            var pageshown = [], hashed = [], popped = [];
            window.addEventListener = function(name, f){ if (name === 'pageshow') pageshown.push(f);
                                                         else if (name === 'hashchange') hashed.push(f);
                                                         else if (name === 'popstate') popped.push(f); };
            function pageshow(){ pageshown.slice().forEach(function(f){ f({ persisted: true }); }); }
            function hashchange(){ hashed.slice().forEach(function(f){ f({}); }); }
            function popstate(){ popped.slice().forEach(function(f){ f({}); }); }
            function windowLoaded(){ document.readyState = 'complete'; document.stages.slice().forEach(function(f){ f({}); }); }
            var clock = 1000000;
            Date.now = function(){ return clock; };
            var location = { href: 'https://a.example/page' };
            var heard = [];
            var cryptoCounter = 0;
            var crypto = { getRandomValues: function(a){ cryptoCounter++; for (var i = 0; i < a.length; i++) a[i] = (i === 0 ? cryptoCounter : 0); return a; } };
        )js");
    }
    static void Make(QJSEngine *engine, bool content, const QString &before = QString()){
        engine->evaluate(World());
        if(!content) engine->evaluate(QStringLiteral("document = undefined;"));
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(content ? Cdp::ContentShim() : Cdp::WorkerShim());
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        QVERIFY2(made.toString().contains(QStringLiteral("storage.sync")), qPrintable(made.toString()));
    }
    static QString AtAPage(){
        return QStringLiteral("location = { href: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/pages/options.html',"
                              "             protocol: 'chrome-extension:' };");
    }
    static void MakePage(QJSEngine *engine, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(AtAPage());
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(Cdp::PageShim());
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        QVERIFY2(made.toString().contains(QStringLiteral("storage.sync")), qPrintable(made.toString()));
    }
    static void MakeKeyedPage(QJSEngine *engine, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(AtAPage());
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(Cdp::PageShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), PageKey()));
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
    }
    static QString PageKey(){ return QString(64, QLatin1Char('7')); }
    static void MakeAsking(QJSEngine *engine, const QString &fetching, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(QStringLiteral("document = undefined;") + fetching);
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(Cdp::WorkerShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), QString(64, QLatin1Char('5'))));
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
    }
    static QString EnginesOwn(){
        return QStringLiteral(
            "var engineCalls = [];"
            "var enginesTabs = { TAB_ID_NONE: -1,"
            "                    update: function(){ engineCalls.push('tabs.update');"
            "                                        return Promise.reject(new Error('The specified target is not found.')); } };"
            "var enginesWindows = { getCurrent: function(){ engineCalls.push('windows.getCurrent');"
            "                                               return Promise.reject(new Error('the engine\\'s own')); } };"
            "chrome.tabs = enginesTabs; chrome.windows = enginesWindows;");
    }
    static QString TheOtherRoot(){
        return QStringLiteral(
            "var rootCalls = [];"
            "function tabsOf(which){"
            "  return { TAB_ID_NONE: -1,"
            "           query: function(){ rootCalls.push(which + '.tabs.query'); return Promise.resolve([{ id: 945651738 }]); },"
            "           update: function(){ rootCalls.push(which + '.tabs.update'); return Promise.resolve('the engine\\'s own'); },"
            "           setZoom: function(){ rootCalls.push(which + '.tabs.setZoom'); return Promise.resolve('the engine\\'s own'); },"
            "           onUpdated: { addListener: function(){ rootCalls.push(which + '.tabs.onUpdated.addListener'); },"
            "                        removeListener: function(){}, hasListener: function(){ return true; }, hasListeners: function(){ return true; } },"
            "           onReplaced: { addListener: function(){ rootCalls.push(which + '.tabs.onReplaced.addListener'); },"
            "                      removeListener: function(){}, hasListener: function(){ return true; }, hasListeners: function(){ return true; } } }; }"
            "var enginesTabs = tabsOf('chrome'); chrome.tabs = enginesTabs;"
            "var enginesOtherTabs = tabsOf('browser');"
            "var enginesOtherStorage = { local: { mark: 'the engine\\'s own' } };"
            "var enginesBrowser = { tabs: enginesOtherTabs, storage: enginesOtherStorage, runtime: chrome.runtime };"
            "self.browser = enginesBrowser;");
    }
    static QString EnginesAlarms(){
        return QStringLiteral(R"js(
            var clock = 1000000; Date.now = function(){ return clock; };
            var engineAlarms = {}, alarmCalls = [], heldLists = [], heldClears = [], holdLists = false, holdClears = false, failLists = false;
            var heldCreates = [], holdCreates = false, holdCreateAnswers = false;
            // (the timers of the look only, not the ten seconds after which an
            //  unanswered call is given up on: the fake timers keep no time.)
            function runShort(){ var due = timers.filter(function(t){ return t.ms !== 10000; });
                                 timers = timers.filter(function(t){ return t.ms === 10000; }); due.forEach(function(t){ t.f(); }); }
            function giveUp(){ var due = timers.filter(function(t){ return t.ms === 10000; });
                               timers = timers.filter(function(t){ return t.ms !== 10000; }); due.forEach(function(t){ t.f(); }); }
            function listOf(){ return Object.keys(engineAlarms).map(function(k){ return JSON.parse(JSON.stringify(engineAlarms[k])); }); }
            function engineUses(){
                Object.keys(engineAlarms).forEach(function(k){
                    var a = engineAlarms[k];
                    if (a.scheduledTime > clock) return;
                    if (a.periodInMinutes) a.scheduledTime += a.periodInMinutes * 60000; else delete engineAlarms[k];
                });
            }
            function withLastError(message, f){ chrome.runtime.lastError = { message: message }; try { f(); } finally { delete chrome.runtime.lastError; } }
            var enginesAlarms = {
                create: function(){
                    var a = Array.prototype.slice.call(arguments);
                    var callback = typeof a[a.length - 1] === 'function' ? a.pop() : null;
                    var name = typeof a[0] === 'string' ? a.shift() : '';
                    var info = a[0];
                    if (!info || typeof info !== 'object') throw new TypeError('Error in invocation of alarms.create: No matching signature.');
                    alarmCalls.push('create ' + name);
                    if (name === 'bad') { if (callback) { withLastError('bad alarm', callback); return undefined; } return Promise.reject(new Error('bad alarm')); }
                    var one = { name: name, scheduledTime: info.when !== undefined ? info.when
                                  : clock + (info.delayInMinutes !== undefined ? info.delayInMinutes : info.periodInMinutes) * 60000 };
                    // (one named 'min' the engine puts five seconds on at the least.)
                    if (name === 'min') one.scheduledTime = Math.max(one.scheduledTime, clock + 5000);
                    if (info.periodInMinutes !== undefined) one.periodInMinutes = info.periodInMinutes;
                    var store = function(){ engineAlarms[name] = one; };
                    if (holdCreates || holdCreateAnswers) {
                        var later = holdCreates;
                        if (!later) store();
                        return new Promise(function(resolve){ heldCreates.push(function(){ if (later) store(); if (callback) callback(); resolve(); }); });
                    }
                    store();
                    if (callback) { callback(); return undefined; }
                    return Promise.resolve();
                },
                getAll: function(callback){
                    alarmCalls.push('getAll');
                    if (failLists) { withLastError('no list', function(){ callback(undefined); }); return undefined; }
                    var list = listOf();
                    if (holdLists) { heldLists.push(function(){ callback(list); }); return undefined; }
                    callback(list); return undefined;
                },
                get: function(name){ return Promise.resolve(engineAlarms[name]); },
                clear: function(){
                    var a = Array.prototype.slice.call(arguments);
                    var callback = typeof a[a.length - 1] === 'function' ? a.pop() : null;
                    var name = typeof a[0] === 'string' ? a[0] : '';
                    alarmCalls.push('clear ' + name);
                    var act = function(){ var was = name in engineAlarms; delete engineAlarms[name]; return was; };
                    if (holdClears) return new Promise(function(resolve){ heldClears.push(function(){ var was = act(); if (callback) callback(was); resolve(was); }); });
                    var was = act();
                    if (callback) { callback(was); return undefined; }
                    return Promise.resolve(was);
                },
                clearAll: function(callback){
                    alarmCalls.push('clearAll');
                    var was = Object.keys(engineAlarms).length > 0; engineAlarms = {};
                    if (callback) { callback(was); return undefined; }
                    return Promise.resolve(was);
                },
                onAlarm: { addListener: function(){ alarmCalls.push('the engine\'s onAlarm'); }, removeListener: function(){},
                           hasListener: function(){ return false; }, hasListeners: function(){ return false; } }
            };
            chrome.alarms = enginesAlarms;
            var rung = [];
            function hear(a){ rung.push(a.name + '@' + (a.scheduledTime - 1000000) + (a.periodInMinutes !== undefined ? '/' + a.periodInMinutes : '')); }
        )js");
    }
    static void PumpShort(QJSEngine *engine){
        for(int i = 0; i < 16; i++){ engine->evaluate(QStringLiteral("runShort();")); QCoreApplication::processEvents(); }
    }
    static void Pump(QJSEngine *engine){
        for(int i = 0; i < 16; i++){ engine->evaluate(QStringLiteral("runTimers();")); QCoreApplication::processEvents(); }
    }
    static void Settle(QJSEngine *engine){
        const QString state = QStringLiteral("timers.map(function(t){ return t.ms; }).join('.') + '/' + fetched.length + '/' + reading");
        for(int round = 0; round < 8; round++){
            QString seen = Value(engine, state);
            for(int quiet = 0, i = 0; i < 200 && quiet < 3; i++){
                QTest::qWait(1);
                const QString now = Value(engine, state);
                quiet = (now == seen) ? quiet + 1 : 0;
                seen = now;
            }
            engine->evaluate(QStringLiteral(
                "var spun = 0; Promise.resolve().then(function(){}).then(function(){}).then(function(){})"
                "  .then(function(){}).then(function(){ spun = 1; });"));
            for(int i = 0; i < 200 && Value(engine, QStringLiteral("spun")) != QStringLiteral("1"); i++)
                QTest::qWait(1);
            if(Value(engine, state) == seen) return;
        }
    }
    static void Drain(QJSEngine *engine, int limit = 400){
        for(int i = 0; i < limit; i++){
            Settle(engine);
            if(Value(engine, QStringLiteral("timers.length")) == QStringLiteral("0")) return;
            engine->evaluate(QStringLiteral("runNextTimer();"));
        }
    }
    static void DrainBefore(QJSEngine *engine, int ms){
        const QString due = QStringLiteral("timers.some(function(t){ return t.ms < %1; })").arg(ms);
        for(int i = 0; i < 64; i++){
            Settle(engine);
            if(Value(engine, due) != QStringLiteral("true")) return;
            engine->evaluate(QStringLiteral("runNextTimer();"));
        }
    }
    static QString Serving(){
        return QStringLiteral(R"js(
            var served = [];
            var texts = { 'cs/a.js': 'A', 'cs/b.js': 'B', 'cs/s.css': 'S' };
            var hostFetch = fetch;
            fetch = function(url, options){
              if (typeof url === 'string' && url.indexOf('chrome-extension://abcdefghijklmnopabcdefghijklmnop/') === 0) {
                var f = url.slice('chrome-extension://abcdefghijklmnopabcdefghijklmnop/'.length); served.push(f);
                if (!(f in texts)) return Promise.resolve({ ok: false });
                return Promise.resolve({ ok: true, text: function(){ return Promise.resolve(texts[f]); } });
              }
              return hostFetch(url, options);
            };
            chrome.runtime.getURL = function(f){
              var s = String(f), root = 'chrome-extension://' + chrome.runtime.id + '/';
              if (s.slice(0, 2) === '//') return 'chrome-extension://' + s.slice(2);
              var out = [];
              s.replace(/^\/+/, '').split('/').forEach(function(seg){
                var one = seg; try { one = decodeURIComponent(seg); } catch (e) {}
                if (one === '.') return;
                if (one === '..') { out.pop(); return; }
                out.push(seg); });
              return root + out.join('/'); };
            function outcome(promise){ out = 'pending'; promise.then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; }); }
            var out = 'pending';
        )js");
    }
    static QString Linking(){
        return QStringLiteral(
            "function naming(claim, url){"
            "  var p = aPort('__vanilla_link__', { id: chrome.runtime.id, url: url || 'https://a.example/page' });"
            "  connects[0](p);"
            "  if (claim !== undefined) p.say(claim);"
            "  return p;"
            "}"
            "function linking(nonce, frame, url, since){"
            "  return naming({ link: { nonce: nonce, frameId: frame, tabUrl: 'https://a.example/page', since: since } }, url);"
            "}");
    }
    static QString Calls(){
        return QStringLiteral("function call(i){ return JSON.parse(decodeURIComponent(fetched[i].options.headers['X-Vanilla-Call'])); }");
    }
    static QString LastCall(){
        return QStringLiteral("decodeURIComponent(fetched[fetched.length - 1].options.headers['X-Vanilla-Call'])");
    }
    static void EachIsAsked(QJSEngine *worker, const QString &ns, const QStringList &calls){
        for(const QString &call : calls){
            worker->evaluate(QStringLiteral("out = 'pending'; answers.push({ ok: true, value: 'v' });"
                                            "chrome.") + ns + QLatin1Char('.') + call
                             + QStringLiteral(".then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
            QTRY_COMPARE(Value(worker, QStringLiteral("out")), QStringLiteral("resolved v"));
            QVERIFY2(Value(worker, LastCall())
                         .startsWith(QStringLiteral("{\"api\":\"") + ns + QLatin1Char('.') + call.left(call.indexOf(QLatin1Char('(')))), qPrintable(call));
        }
    }
    static QString Aborting(){
        return QStringLiteral(
            "var aborts = 0, rejecters = [];"
            "function AbortController(){ var me = this;"
            "  me.signal = { aborted: false };"
            "  me.abort = function(){ aborts++; me.signal.aborted = true;"
            "                         var all = rejecters; rejecters = [];"
            "                         all.forEach(function(r){ r(new Error('The user aborted a request.')); }); }; }"
            "var plainFetch = fetch;"
            "fetch = function(url, options){"
            "  var coming = plainFetch(url, options);"
            "  if (!options || !options.signal) return coming;"
            "  return new Promise(function(res, rej){ rejecters.push(rej); coming.then(res, rej); });"
            "};");
    }
    static QString Navigating(){
        return QStringLiteral(
            "var navHeard = {}, navTouched = [];"
            "var navigation = { addEventListener: function(name, f){ (navHeard[name] = navHeard[name] || []).push(f); } };"
            "function navFire(name, e){ (navHeard[name] || []).slice().forEach(function(f){ f(e); }); }"
            "function navigate(url, hash, same){"
            "  navFire('navigate', { navigationType: 'push', hashChange: !!hash,"
            "                        destination: { sameDocument: same === undefined ? true : same, url: url },"
            "                        preventDefault: function(){ navTouched.push('preventDefault'); },"
            "                        intercept: function(){ navTouched.push('intercept'); } }); }"
            "function commit(url){ if (url !== undefined) location.href = url; navFire('currententrychange', {}); }");
    }
    static QString Urling(){
        return QStringLiteral(
            "var WITH_HOST = { 'http:': 1, 'https:': 1, 'ws:': 1, 'wss:': 1, 'ftp:': 1 };"
            "var USUAL_PORT = { 'http:': '80', 'https:': '443', 'ws:': '80', 'wss:': '443', 'ftp:': '21' };"
            "function URL(text){"
            "  var m = /^([a-z][a-z0-9+.-]*:)\\/\\/([^\\/?#]*)/i.exec(String(text));"
            "  if (!m) throw new TypeError('Failed to construct URL: Invalid URL');"
            "  var scheme = m[1].toLowerCase(), authority = m[2];"
            "  var at = authority.lastIndexOf('@');"
            "  var host = (at < 0 ? authority : authority.slice(at + 1)).toLowerCase();"
            "  var mark = host.lastIndexOf(':');"
            "  if (mark > host.lastIndexOf(']') && host.slice(mark + 1) === USUAL_PORT[scheme]) host = host.slice(0, mark);"
            "  this.origin = WITH_HOST[scheme] && host ? scheme + '//' + host : 'null'; }");
    }
    static void Linked(QJSEngine *page, const QString &before){
        Make(page, true, Fetching() + Saying() + before);
        page->evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                      "chrome.runtime.onMessage.addListener(function(){ return false; });"));
        for(int i = 0; i < 400 && Value(page, QStringLiteral("ports.length")) == QStringLiteral("0"); i++)
            QTest::qWait(1);
        QCOMPARE(Value(page, QStringLiteral("ports.length")), QStringLiteral("1"));
        page->evaluate(QStringLiteral(
            "ports[0].say({ linked: 1 });"
            "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
            "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"));
    }
    static void Pinging(QJSEngine *worker, const QString &before = QString()){
        MakeAsking(worker, Fetching(), Saying() + before);
        worker->evaluate(Linking());
        worker->evaluate(QStringLiteral(
            "answers.push({ ok: true, value: { id: 555, index: 0 } });"
            "function localReads(){ return reads.filter(function(a){ return a === 'local'; }).length; }"
            "function hearing(){ chrome.storage.onChanged.addListener(function(c, a){ heard.push(a + ':' + JSON.stringify(c)); }); }"
            "var link = linking('b1000000000000000000000000000000', 0, 'https://a.example/page', 100);"));
        Settle(worker);
    }
    static QString Saying(){
        return QStringLiteral("var warned = [], cried = [];"
                              "var console = { warn: function(m){ warned.push(String(m)); },"
                              "                error: function(m){ cried.push(String(m)); } };");
    }
    static void RunAll(QJSEngine *engine, int limit = 64){
        for(int i = 0; i < limit; i++){
            Settle(engine);
            if(Value(engine, QStringLiteral("timers.length")) == QStringLiteral("0")) return;
            engine->evaluate(QStringLiteral("runNextTimer();"));
        }
    }
    static QString EdgeBrands(){
        return QStringLiteral(
            "var navigator = { userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko)"
            " Chrome/153.0.0.0 Safari/537.36 Edg/153.0.0.0',"
            "                  userAgentData: { brands: [{ brand: 'Microsoft Edge WebView2', version: '153' },"
            "                                            { brand: 'Not_A Brand', version: '8' },"
            "                                            { brand: 'Chromium', version: '153' },"
            "                                            { brand: 'Microsoft Edge', version: '153' }] } };");
    }
    static QString SpoofedBrands(){
        return QStringLiteral(
            "var navigator = { userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko)"
            " Chrome/140.0.0.0 Safari/537.36 Edg/140.0.0.0',"
            "                  userAgentData: { brands: [{ brand: 'Not=A?Brand', version: '99' },"
            "                                            { brand: 'Chromium', version: '140' },"
            "                                            { brand: 'Microsoft Edge', version: '140' }] } };");
    }
    static QString EdgesOwn(){
        return QStringLiteral(
            "var edgeCalls = [];"
            "function edgeEvent(name){ return { addListener: function(){ edgeCalls.push(name + '.addListener'); },"
            "                                   removeListener: function(){}, hasListener: function(){ return false; },"
            "                                   hasListeners: function(){ return false; }, addRules: function(){},"
            "                                   removeRules: function(){}, getRules: function(){} }; }"
            "chrome.contextMenus = { create: function(p){ edgeCalls.push('contextMenus.create'); return p && p.id; },"
            "                        update: function(){ edgeCalls.push('contextMenus.update'); return Promise.resolve(); },"
            "                        remove: function(){ return Promise.resolve(); },"
            "                        removeAll: function(){ edgeCalls.push('contextMenus.removeAll'); return Promise.resolve(); },"
            "                        onClicked: edgeEvent('contextMenus.onClicked') };"
            "chrome.downloads = { download: function(){ edgeCalls.push('downloads.download'); return Promise.resolve(77); },"
            "                     search: function(){ edgeCalls.push('downloads.search'); return Promise.resolve([]); },"
            "                     onCreated: edgeEvent('downloads.onCreated'), onChanged: edgeEvent('downloads.onChanged') };"
            "chrome.offscreen = { createDocument: function(){ edgeCalls.push('offscreen.createDocument'); return Promise.resolve(); },"
            "                     closeDocument: function(){ return Promise.resolve(); },"
            "                     hasDocument: function(){ return Promise.resolve(false); } };"
            "chrome.i18n = { getMessage: function(){ edgeCalls.push('i18n.getMessage'); return 'the engine\\'s own'; },"
            "                getUILanguage: function(){ return 'engine'; } };"
            "chrome.runtime.getContexts = function(){ edgeCalls.push('runtime.getContexts'); return Promise.resolve([]); };"
            "chrome.tabs = { TAB_ID_NONE: -1,"
            "                query: function(){ edgeCalls.push('tabs.query'); return Promise.resolve([{ id: 945651738 }]); },"
            "                update: function(){ edgeCalls.push('tabs.update'); return Promise.resolve(); } };"
            "var enginesMenus = chrome.contextMenus, enginesDownloads = chrome.downloads,"
            "    enginesOffscreen = chrome.offscreen, enginesI18n = chrome.i18n,"
            "    enginesSync = chrome.storage.sync, enginesOnChanged = chrome.storage.onChanged,"
            "    enginesGetContexts = chrome.runtime.getContexts, enginesTabs = chrome.tabs;");
    }
    static QString HostFetches(){
        return QStringLiteral("function hostFetches(){ return fetched.filter(function(f){"
                              "  return String(f.url).indexOf('vanilla-extension://') === 0; }).length; }"
                              "function waitingHere(){ return timers.filter(function(t){ return t.ms === 10000; }).length; }");
    }
    static QString Messages(){
        return QStringLiteral("self.__vanillaMessages = { locale: 'ja_JP', messages: { greeting: { message: 'ours' } } };");
    }
    static void MakeEdgeWorker(QJSEngine *engine, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(QStringLiteral("document = undefined;") + Fetching() + HostFetches() + EdgeBrands() + EdgesOwn());
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(Cdp::WorkerShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), QString(64, QLatin1Char('5'))));
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        engine->evaluate(QStringLiteral("var report = '") + made.toString() + QStringLiteral("';"));
    }
    static void MakeEdgeContent(QJSEngine *engine, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(Fetching() + HostFetches() + EdgeBrands() + EdgesOwn());
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(Cdp::ContentShim());
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        engine->evaluate(QStringLiteral("var report = '") + made.toString() + QStringLiteral("';"));
    }
    static void MakeEdgePage(QJSEngine *engine, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(AtAPage());
        engine->evaluate(Fetching() + HostFetches() + EdgeBrands() + EdgesOwn());
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        const QJSValue made = engine->evaluate(Cdp::PageShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), PageKey()));
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        engine->evaluate(QStringLiteral("var report = '") + made.toString() + QStringLiteral("';"));
    }
    static QString Relaying(){
        return QStringLiteral(
            "chrome.runtime.getURL = function(f){ return 'chrome-extension://' + chrome.runtime.id + '/' + f; };"
            "function relaySender(over){"
            "  var s = { id: chrome.runtime.id, url: chrome.runtime.getURL('vanilla_relay.html') };"
            "  for (var k in (over || {})) s[k] = over[k];"
            "  return s; }"
            "function relaying(over){ var p = aPort('__vanilla_relay__', relaySender(over)); connects[0](p); return p; }"
            "function relayCalls(p){ return p.posted.map(function(m){ return m.call.api; }).join(); }") + Waits();
    }
    static QString Waits(){
        return QStringLiteral("function waits(){ return timers.map(function(t){ return t.ms; })"
                              "  .sort(function(a, b){ return a - b; }).join(); }");
    }
    static QString RelayKey(){ return QString(64, QLatin1Char('3')); }
    static void MakeRelay(QJSEngine *engine, const QString &before = QString()){
        engine->evaluate(World());
        engine->evaluate(QStringLiteral("location = { href: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/vanilla_relay.html' };"));
        engine->evaluate(Fetching() + Aborting() + Waits());
        if(!before.isEmpty()) QVERIFY2(!engine->evaluate(before).isError(), "the test's own script");
        QVERIFY(Cdp::RelayScript().contains(QLatin1String(ExtensionHostWire::KEY_PLACE)));
        const QJSValue made = engine->evaluate(Cdp::RelayScript().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), RelayKey()));
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
    }
    static QString Value(QJSEngine *engine, const QString &expression){
        const QJSValue result = engine->evaluate(expression);
        return result.isError() ? QStringLiteral("ERROR ") + result.toString() : result.toString();
    }
    static QString Clicking(){
        return QStringLiteral(
            "var enginesClicked = { list: [],"
            "  addListener: function(f){ edgeCalls.push('contextMenus.onClicked.addListener'); this.list.push(f); },"
            "  removeListener: function(){}, hasListener: function(){ return false; }, hasListeners: function(){ return false; } };"
            "chrome.contextMenus.onClicked = enginesClicked;"
            "function click(info, tab){ enginesClicked.list.forEach(function(f){ f(info, tab); }); }");
    }
    static QString MenuTabs(){
        return QStringLiteral(
            "var asked = [];"
            "self.__vanillaRelay.attach(function(ticket, call){ asked.push({ ticket: ticket, call: call }); });"
            "function answer(i, a){ self.__vanillaRelay.answer(asked[i].ticket, a); }"
            "function menuTab(i, id, index){ answer(i, { ok: true, value: { id: id, index: index } }); }"
            "function menuArgs(){ return asked.map(function(a){ return a.call.api + JSON.stringify(a.call.args); }).join(' '); }"
            "var got = [];"
            "function hear(tag){ return function(info, tab){"
            "  got.push(tag + ':' + (info && info.pageUrl) + '=' + (tab === undefined ? 'none' : tab.id + '@' + tab.index + '/' + tab.windowId)); }; }");
    }
    static QString Scripting(){
        return QStringLiteral(
            "var scriptingCalls = [], scriptingFails = null;"
            "function engineCall(name){ return function(){"
            "  scriptingCalls.push({ name: name, self: this, args: Array.prototype.slice.call(arguments) });"
            "  if (scriptingFails) return Promise.reject(new Error(scriptingFails));"
            "  return Promise.resolve(name === 'executeScript' ? [{ frameId: 0, documentId: 'D', result: 'engine' }]"
            "                        : name === 'registerContentScripts' ? 'the engine\\'s own' : undefined); }; }"
            "var enginesScripting = { executeScript: engineCall('executeScript'), insertCSS: engineCall('insertCSS'),"
            "                         removeCSS: engineCall('removeCSS'), registerContentScripts: engineCall('registerContentScripts'),"
            "                         ExecutionWorld: { ISOLATED: 'ISOLATED', MAIN: 'MAIN' } };"
            "chrome.scripting = enginesScripting;"
            "chrome.runtime.getManifest = function(){ return { permissions: ['scripting', 'contextMenus'] }; };");
    }
    static QString ScriptingTabs(){
        return MenuTabs() + QStringLiteral(
            "function linkingTab(nonce, engineTab, since){"
            "  var sender = { id: chrome.runtime.id, url: 'https://a.example/page' };"
            "  if (engineTab !== null) sender.tab = { id: engineTab, index: 0, windowId: 945650001 };"
            "  var p = aPort('__vanilla_link__', sender);"
            "  connects[0](p);"
            "  p.say({ link: { nonce: nonce, frameId: 0, tabUrl: 'https://a.example/page', since: since } });"
            "  return p; }"
            "function tabOf(nonce, id){ asked.forEach(function(a, i){"
            "  if (!a.done && a.call.api === 'vanilla.tabOf' && a.call.args[0] === nonce) { a.done = 1; menuTab(i, id, 0); } }); }"
            "function runAt(ms){ var due = timers.filter(function(t){ return t.ms === ms; });"
            "  timers = timers.filter(function(t){ return t.ms !== ms; }); due.forEach(function(t){ t.f(); }); return due.length; }"
            "function lastScripting(){ var c = scriptingCalls[scriptingCalls.length - 1];"
            "  return c ? c.name + JSON.stringify(c.args) : 'none'; }"
            "var out = 'pending';"
            "function outcome(p){ out = 'pending'; p.then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; }); }");
    }
    static void ScriptingWorker(QJSEngine *worker, const QString &before = QString()){
        MakeEdgeWorker(worker, Scripting() + before);
        worker->evaluate(ScriptingTabs());
        worker->evaluate(QStringLiteral("var one = linkingTab('a1000000000000000000000000000000', 945653585, 100);"));
        Settle(worker);
        worker->evaluate(QStringLiteral("tabOf('a1000000000000000000000000000000', 132);"));
        Settle(worker);
    }
};

void tst_cdpshims::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_cdpshims::syncIsKeptInLocalUnderAPrefix(){
    QJSEngine engine;
    Make(&engine, false);
    engine.evaluate(QStringLiteral(
        "var out = 'pending';"
        "localArea.data.own = 'the extension\\'s';"
        "chrome.storage.sync.set({ a: 1, b: { c: 2 } })"
        "  .then(function(){ return Promise.all([chrome.storage.sync.get(null), chrome.storage.sync.get('a'), chrome.storage.sync.get(['b', 'zz']),"
        "                                        chrome.storage.sync.get({ a: 0, d: 'default' })]); })"
        "  .then(function(r){ out = JSON.stringify(r); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")),
                 QStringLiteral("[{\"a\":1,\"b\":{\"c\":2}},{\"a\":1},{\"b\":{\"c\":2}},{\"a\":1,\"d\":\"default\"}]"));
    QCOMPARE(Value(&engine, QStringLiteral("Object.keys(localArea.data).sort().join()")),
             QStringLiteral("__vanilla_sync__:a,__vanilla_sync__:b,own"));

    engine.evaluate(QStringLiteral(
        "out = 'pending';"
        "chrome.storage.sync.remove('a').then(function(){ return chrome.storage.sync.clear(); })"
        "  .then(function(){ out = Object.keys(localArea.data).join(); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("own"));

    engine.evaluate(QStringLiteral("out = 'pending'; chrome.storage.sync.set({ e: 5 }, function(){ chrome.storage.sync.get('e', function(items){ out = JSON.stringify(items); }); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("{\"e\":5}"));

    engine.evaluate(QStringLiteral("out = 'pending'; chrome.storage.sync.getBytesInUse(null).then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected'; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("rejected"));
}

void tst_cdpshims::nothingIsReadUntilSomebodyListens(){
    QJSEngine engine;
    Make(&engine, true);
    QCOMPARE(Value(&engine, QStringLiteral("reads.length + ' reads, ' + timers.length + ' timers'")), QStringLiteral("0 reads, 0 timers"));

    engine.evaluate(QStringLiteral("chrome.storage.onChanged.addListener(function(c, a){ heard.push(a + ':' + Object.keys(c).join('+')); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("reads.slice().sort().join()")), QStringLiteral("local,session"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
    QCOMPARE(Value(&engine, QStringLiteral("heard.join()")), QString());
}

void tst_cdpshims::onlyAnAreaSomebodyHearsOfIsRead(){
    QJSEngine engine;
    Make(&engine, true);
    engine.evaluate(QStringLiteral("chrome.storage.session.onChanged.addListener(function(c){ heard.push('session:' + Object.keys(c).join('+')); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
    QCoreApplication::processEvents();
    engine.evaluate(QStringLiteral("localArea.data.big = 'megabytes'; sessionArea.data.mapping = 1; runTimers();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join()")), QStringLiteral("session:mapping"));
    engine.evaluate(QStringLiteral("chrome.storage.local.set({ more: 1 });"));
    QCoreApplication::processEvents();
    QCOMPARE(Value(&engine, QStringLiteral("reads.filter(function(a){ return a !== 'session'; }).length")), QStringLiteral("0"));

    engine.evaluate(QStringLiteral("chrome.storage.sync.onChanged.addListener(function(c){ heard.push('sync:' + Object.keys(c).join('+')); });"));
    QCOMPARE(Value(&engine, QStringLiteral("reads.filter(function(a){ return a === 'local'; }).length")), QStringLiteral("1"));
    QCoreApplication::processEvents();
    engine.evaluate(QStringLiteral("heard = []; localArea.data['__vanilla_sync__:theme'] = 'dark'; runTimers();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join()")), QStringLiteral("sync:theme"));
}

void tst_cdpshims::whatThisContextWritesIsToldOfAtOnce(){
    QJSEngine engine;
    Make(&engine, false);
    engine.evaluate(QStringLiteral(
        "localArea.data.old = 1;"
        "var perArea = [];"
        "chrome.storage.onChanged.addListener(function(c, a){ heard.push(a + ':' + JSON.stringify(c)); });"
        "chrome.storage.sync.onChanged.addListener(function(c){ perArea.push('sync:' + Object.keys(c).join('+')); });"
        "chrome.storage.local.onChanged.addListener(function(c){ perArea.push('local:' + Object.keys(c).join('+')); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("reads.length >= 2")), QStringLiteral("true"));
    QCoreApplication::processEvents();

    engine.evaluate(QStringLiteral("chrome.storage.local.set({ old: 2, fresh: 'x' });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join(' | ')")),
                 QStringLiteral("local:{\"old\":{\"oldValue\":1,\"newValue\":2},\"fresh\":{\"newValue\":\"x\"}}"));

    engine.evaluate(QStringLiteral("heard = []; chrome.storage.sync.set({ setting: true });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join(' | ')")), QStringLiteral("sync:{\"setting\":{\"newValue\":true}}"));
    engine.evaluate(QStringLiteral("heard = []; chrome.storage.session.set({ mapping: [1] }); chrome.storage.local.remove('fresh');"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.slice().sort().join(' | ')")),
                 QStringLiteral("local:{\"fresh\":{\"oldValue\":\"x\"}} | session:{\"mapping\":{\"newValue\":[1]}}"));
    QCOMPARE(Value(&engine, QStringLiteral("perArea.join()")), QStringLiteral("local:old+fresh,sync:setting,local:fresh"));

    QCOMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("0"));
}

void tst_cdpshims::whatOthersWriteIsFoundByAskingAgainInAPageOnly(){
    QJSEngine engine;
    Make(&engine, true);
    engine.evaluate(QStringLiteral("var listener = function(c, a){ heard.push(a + ':' + Object.keys(c).join('+')); }; chrome.storage.onChanged.addListener(listener);"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
    QCoreApplication::processEvents();

    engine.evaluate(QStringLiteral("sessionArea.data.normalModeKeyStateMapping = { j: 1 }; localArea.data['__vanilla_sync__:theme'] = 'dark'; runTimers();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.slice().sort().join(' | ')")), QStringLiteral("session:normalModeKeyStateMapping | sync:theme"));

    QCOMPARE(Value(&engine, QStringLiteral("timers[0].ms")), QStringLiteral("250"));
    engine.evaluate(QStringLiteral("for (var i = 0; i < 30; i++) runTimers();"));
    QCOMPARE(Value(&engine, QStringLiteral("timers[0].ms")), QStringLiteral("3000"));
    engine.evaluate(QStringLiteral("document.onVisibility(); runTimers();"));
    QCOMPARE(Value(&engine, QStringLiteral("timers[0].ms")), QStringLiteral("250"));

    engine.evaluate(QStringLiteral("reads = []; document.visibilityState = 'hidden'; document.onVisibility(); runTimers();"));
    QCOMPARE(Value(&engine, QStringLiteral("timers.length + ' timers, ' + reads.length + ' reads'")), QStringLiteral("0 timers, 0 reads"));
    engine.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&engine, QStringLiteral("timers.length + ' timers in ' + timers[0].ms")), QStringLiteral("1 timers in 0"));
    engine.evaluate(QStringLiteral("document.onVisibility(); runTimers();"));
    QCOMPARE(Value(&engine, QStringLiteral("timers.length + ' timers, ' + reads.length + ' reads'")), QStringLiteral("1 timers, 2 reads"));
    engine.evaluate(QStringLiteral("reads = []; chrome.storage.onChanged.removeListener(listener); runTimers();"));
    QCOMPARE(Value(&engine, QStringLiteral("timers.length + ' timers, ' + reads.length + ' reads'")), QStringLiteral("0 timers, 0 reads"));
    engine.evaluate(QStringLiteral("chrome.storage.onChanged.addListener(listener);"));
    QCOMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
}

void tst_cdpshims::anAreaWhichWouldNotBeReadStartsAsEmpty(){
    QJSEngine engine;
    Make(&engine, true);
    engine.evaluate(QStringLiteral("sessionArea.refuses = true; chrome.storage.onChanged.addListener(function(c, a){ heard.push(a + ':' + Object.keys(c).join('+')); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
    QCoreApplication::processEvents();

    engine.evaluate(QStringLiteral("sessionArea.refuses = false; sessionArea.data.normalModeKeyStateMapping = { j: 1 }; sessionArea.data.secret = 's'; runTimers();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join(' | ')")), QStringLiteral("session:normalModeKeyStateMapping+secret"));
}

void tst_cdpshims::aLookWhichComesBackLateIsDropped(){
    QJSEngine engine;
    Make(&engine, true);
    engine.evaluate(QStringLiteral("chrome.storage.onChanged.addListener(function(c, a){ heard.push(a + ':' + JSON.stringify(c)); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
    QCoreApplication::processEvents();

    engine.evaluate(QStringLiteral(
        "localArea.held = [];"
        "localArea.data.n = 1; runTimers();"
        "localArea.data.n = 2; runTimers();"
        "var late = localArea.held[0], early = localArea.held[1]; localArea.held = null;"
        "early();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join(' | ')")), QStringLiteral("local:{\"n\":{\"newValue\":2}}"));
    engine.evaluate(QStringLiteral("late();"));
    QCoreApplication::processEvents();
    engine.evaluate(QStringLiteral("runTimers();"));
    QCoreApplication::processEvents();
    QCOMPARE(Value(&engine, QStringLiteral("heard.join(' | ')")), QStringLiteral("local:{\"n\":{\"newValue\":2}}"));
}

void tst_cdpshims::aValueWithNoJsonDoesNotEndTheLooking(){
    QJSEngine engine;
    Make(&engine, true, QStringLiteral(
        "var odd = undefined, cycle = {}; cycle.self = cycle;"
        "localArea.get = function(){ reads.push('local'); return Promise.resolve({ odd: odd, n: 1, cycle: cycle }); };"));
    engine.evaluate(QStringLiteral("chrome.storage.local.onChanged.addListener(function(c){ heard.push(JSON.stringify(c)); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("timers.length")), QStringLiteral("1"));
    QCoreApplication::processEvents();
    engine.evaluate(QStringLiteral("odd = 5; runTimers();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("heard.join(' | ')")), QStringLiteral("{\"odd\":{\"oldValue\":null,\"newValue\":5}}"));
}

void tst_cdpshims::aFailedWriteIsNotASuccess(){
    QJSEngine engine;
    Make(&engine, false);
    engine.evaluate(QStringLiteral(
        "localArea.refuses = true; var out = 'pending', promised = 'pending';"
        "chrome.storage.local.set({ a: 1 }, function(){ out = chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR'; });"
        "chrome.storage.sync.set({ a: 1 }).then(function(){ promised = 'resolved'; }, function(e){ promised = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("promised")), QStringLiteral("rejected: QUOTA_BYTES quota exceeded"));
    QTRY_VERIFY(Value(&engine, QStringLiteral("timers.length")) != QStringLiteral("0"));
    engine.evaluate(QStringLiteral("runTimers();"));
    QCOMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("QUOTA_BYTES quota exceeded"));
    QCOMPARE(Value(&engine, QStringLiteral("'lastError' in chrome.runtime")), QStringLiteral("false"));
}

void tst_cdpshims::aMessageSaysWhichTabAndFrameItIsFrom(){
    QJSEngine page, worker;
    Make(&page, true);
    Make(&worker, false);
    QVERIFY(Value(&page, QStringLiteral("1")) == QStringLiteral("1"));

    page.evaluate(QStringLiteral("var answer = 'pending'; chrome.runtime.sendMessage({ handler: 'initializeFrame' }).then(function(v){ answer = v; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("answer")), QStringLiteral("answer"));
    const QString wire = Value(&page, QStringLiteral("JSON.stringify(sent[0][0])"));
    const QJsonObject envelope = QJsonDocument::fromJson(wire.toUtf8()).object();
    QCOMPARE(envelope[QStringLiteral("__vanillaEnvelope")].toInt(), 1);
    QCOMPARE(envelope[QStringLiteral("message")].toObject()[QStringLiteral("handler")].toString(), QStringLiteral("initializeFrame"));
    const QJsonObject from = envelope[QStringLiteral("from")].toObject();
    QCOMPARE(from[QStringLiteral("frameId")].toInt(-1), 0);
    QVERIFY(!from.contains(QStringLiteral("tabId")));
    QVERIFY(QRegularExpression(QStringLiteral("\\A[0-9a-f]{32}\\z")).match(from[QStringLiteral("nonce")].toString()).hasMatch());
    QCOMPARE(from[QStringLiteral("tabUrl")].toString(), QStringLiteral("https://a.example/page"));
    page.evaluate(QStringLiteral("sent = []; chrome.runtime.sendMessage('ping', function(){}); chrome.runtime.sendMessage('ppppppppppppppppppppppppppppppppp', { hello: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("sent[0][0].message + ' ' + typeof sent[0][1] + ' | ' + sent[1][0] + ' ' + JSON.stringify(sent[1][1])")),
             QStringLiteral("ping function | ppppppppppppppppppppppppppppppppp {\"hello\":1}"));

    worker.evaluate(QStringLiteral(
        "var got = [];"
        "var listener = function(message, sender, respond){ got.push(JSON.stringify(message) + ' tab=' + (sender.tab ? sender.tab.id + ',' + sender.tab.url : 'none') + ' frame=' + sender.frameId + ' url=' + sender.url); return 'kept open'; };"
        "chrome.runtime.onMessage.addListener(listener); chrome.runtime.onMessage.addListener(listener);"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length + ' ' + chrome.runtime.onMessage.hasListener(listener)")), QStringLiteral("1 true"));
    worker.evaluate(QStringLiteral("Math.random = function(){ return 0.25; };"));
    const QString returned = Value(&worker, QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'https://a.example/page', origin: 'https://a.example' }, function(){})").arg(wire));
    QCOMPARE(returned, QStringLiteral("kept open"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0]")),
             QStringLiteral("{\"handler\":\"initializeFrame\"} tab=1342177280,https://a.example/page frame=0 url=https://a.example/page"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'https://a.example/page' }, function(){});").arg(wire));
    QCOMPARE(Value(&worker, QStringLiteral("got[1]")),
             QStringLiteral("{\"handler\":\"initializeFrame\"} tab=1342177280,https://a.example/page frame=0 url=https://a.example/page"));
    worker.evaluate(QStringLiteral("listeners[0]({ handler: 'fromOptions' }, { id: chrome.runtime.id, url: 'chrome-extension://x/options.html' }, function(){});"));
    QCOMPARE(Value(&worker, QStringLiteral("got[2]")), QStringLiteral("{\"handler\":\"fromOptions\"} tab=none frame=undefined url=chrome-extension://x/options.html"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: 'somebodyelse', url: 'https://evil.example/' }, function(){});").arg(wire));
    QVERIFY2(Value(&worker, QStringLiteral("got[3]")).contains(QStringLiteral("__vanillaEnvelope")), qPrintable(Value(&worker, QStringLiteral("got[3]"))));
    QVERIFY(Value(&worker, QStringLiteral("got[3]")).contains(QStringLiteral("tab=none")));
    worker.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(listener);"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length + ' ' + chrome.runtime.onMessage.hasListener(listener)")), QStringLiteral("1 false"));
}

void tst_cdpshims::aContentScriptNamesItsDocumentToTheHost(){
    QJSEngine page;
    page.evaluate(World());
    page.evaluate(Fetching());
    QVERIFY(!page.evaluate(Cdp::ContentShim()).isError());

    QCOMPARE(Value(&page, QStringLiteral("fetched.length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'initializeFrame' });"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].url + ' ' + fetched[0].options.method")),
             QStringLiteral("vanilla-extension://host/bind POST"));
    const QString nonce = Value(&page, QStringLiteral("fetched[0].options.headers['X-Vanilla-Nonce']"));
    QVERIFY2(QRegularExpression(QStringLiteral("\\A[0-9a-f]{32}\\z")).match(nonce).hasMatch(), qPrintable(nonce));
    QCOMPARE(Value(&page, QStringLiteral("'body' in fetched[0].options")), QStringLiteral("false"));
    QCOMPARE(Value(&page, QStringLiteral("sent[0][0].from.nonce")), nonce);
    QVERIFY(!Value(&page, QStringLiteral("JSON.stringify(fetched[0].options.headers)")).contains(QStringLiteral("Key")));
    QVERIFY(!Cdp::ContentShim().contains(QStringLiteral("dispatchEvent")));
    QVERIFY(!Cdp::ContentShim().contains(QStringLiteral("CustomEvent")));
    QVERIFY(!Cdp::ContentShim().contains(QLatin1String(ExtensionHostWire::KEY_PLACE)));
    page.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'again' });"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length")), QStringLiteral("1"));
}

void tst_cdpshims::theWorkerAsksTheHostWhichTabANumberIsOf(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "var mine = function(m, s){ got.push(JSON.stringify(m) + ' tab=' + (s.tab ? s.tab.id : 'none') + ' frame=' + s.frameId + ' url=' + s.url); return false; };"
        "chrome.runtime.onMessage.addListener(mine);"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.runtime.onMessage.hasListener(mine)")), QStringLiteral("true"));

    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 555, index: 0 } });"));
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '01000000000000000000000000000000', tabId: 9, frameId: 0, tabUrl: 'https://a.example/' }, message: { handler: 'ping' } }");
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'https://a.example/' }, function(){});").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[fetched.length-1].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"vanilla.tabOf\",\"args\":[\"01000000000000000000000000000000\"]}"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0]")), QStringLiteral("{\"handler\":\"ping\"} tab=555 frame=0 url=https://a.example/"));

    QJSEngine restarted;
    MakeAsking(&restarted, Fetching());
    restarted.evaluate(QStringLiteral(
        "var got = []; chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id); return false; });"
        "answers.push({ ok: true, value: { id: 555, index: 0 } });"));
    restarted.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'https://a.example/' }, function(){});").arg(env));
    QTRY_COMPARE(Value(&restarted, QStringLiteral("got.join()")), QStringLiteral("555"));
    QCOMPARE(Value(&restarted, QStringLiteral("fetched.length")), QStringLiteral("1"));

    const int asked = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'https://a.example/' }, function(){});").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked);
    QCOMPARE(Value(&worker, QStringLiteral("got[1]")), QStringLiteral("{\"handler\":\"ping\"} tab=555 frame=0 url=https://a.example/"));

    worker.evaluate(QStringLiteral("Math.random = function(){ return 0.25; }; answers.push({ ok: false, error: 'no tab' }, { ok: false, error: 'no tab' }, { ok: false, error: 'no tab' }, { ok: false, error: 'no tab' }, { ok: false, error: 'no tab' }, { ok: false, error: 'no tab' });"));
    const int beforeFallback = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral(
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '02000000000000000000000000000000', tabId: 77, frameId: 0, tabUrl: 'u' }, message: { handler: 'p2' } },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    for(int attempt = 1; attempt < 6; attempt++){
        QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms < 3000; })")), QStringLiteral("true"));
        worker.evaluate(QStringLiteral("runNextTimer();"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt() - beforeFallback, attempt + 1);
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("3"));
    QCOMPARE(Value(&worker, QStringLiteral("got[2]")), QStringLiteral("{\"handler\":\"p2\"} tab=1342177280 frame=0 url=u"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt() - beforeFallback, 6);

    const int beforeCorrection = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral(
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '02000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: { handler: 'second' } },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    worker.evaluate(QStringLiteral(
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '02000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: { handler: 'third' } },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("5"));
    QCOMPARE(Value(&worker, QStringLiteral("got[3]")), QStringLiteral("{\"handler\":\"second\"} tab=1342177280 frame=0 url=u"));
    QCOMPARE(Value(&worker, QStringLiteral("got[4]")), QStringLiteral("{\"handler\":\"third\"} tab=1342177280 frame=0 url=u"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), beforeCorrection);

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false });"));
    for(int attempt = 1; attempt <= 6; attempt++){
        QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms < 3000; })")), QStringLiteral("true"));
        worker.evaluate(QStringLiteral("runNextTimer();"));
        QCoreApplication::processEvents();
        QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt() - beforeCorrection, attempt);
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms < 3000; })")), QStringLiteral("true"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 556, index: 0 } }); runNextTimer();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt() - beforeCorrection, 7);
    for(int i = 0; i < 8 && !Value(&worker, QStringLiteral("got[got.length-1]")).contains(QStringLiteral("tab=556")); i++){
        QCoreApplication::processEvents();
        worker.evaluate(QStringLiteral(
            "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '02000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: { handler: 'corrected' } },"
            "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("got[got.length-1]")), QStringLiteral("{\"handler\":\"corrected\"} tab=556 frame=0 url=u"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt() - beforeCorrection, 7);

    const int asked2 = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral("listeners[0]({ handler: 'fromOptions' }, { id: chrome.runtime.id, url: 'chrome-extension://x/options.html' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got[got.length-1]")), QStringLiteral("{\"handler\":\"fromOptions\"} tab=none frame=undefined url=chrome-extension://x/options.html"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked2);

    worker.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(mine);"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0]({ handler: 'nobody home' }, { id: chrome.runtime.id, url: 'u' }, function(){}))")),
             QStringLiteral("undefined"));
}

void tst_cdpshims::whatWaitsIsHandedOverInOrderAndTheLostFallBack(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var order = [], replies = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ order.push(m.n + ':' + (s.tab ? s.tab.id : 'none')); return false; });"));
    worker.evaluate(QStringLiteral("held = []; answers.push('HELD');"));
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '03000000000000000000000000000000', tabId: 5, frameId: 0, tabUrl: 'u' }, message: { n: %1 } }");
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ replies.push(v); });").arg(env.arg(1)));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ replies.push(v); });").arg(env.arg(2)));
    QCOMPARE(Value(&worker, QStringLiteral("order.length")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("answers[answers.length] = { ok: true, value: { id: 400, index: 0 } }; held.forEach(function(f){ f(); }); held = [];"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("order.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("order.join()")), QStringLiteral("1:400,2:400"));
    QCOMPARE(Value(&worker, QStringLiteral("replies.length + ':' + replies.every(function(v){ return v === undefined; })")), QStringLiteral("2:true"));
}

void tst_cdpshims::manyListenersAreFoldedAsChromeWouldFoldThem(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var answered = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ setTimeout(function(){ respond('from the slow one'); }, 0); return true; });"
        "chrome.runtime.onMessage.addListener(function(){ return false; });"
        "chrome.runtime.onMessage.addListener(function(){ throw new Error('the throwing one'); });"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 33, index: 0 } });"));
    worker.evaluate(QStringLiteral(
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '04000000000000000000000000000000', tabId: 1, frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(v); });"));
    Pump(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("from the slow one"));
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
}

void tst_cdpshims::aMissingNonceKeepsDocumentsApart(){
    QJSEngine worker;
    Make(&worker, false);
    worker.evaluate(QStringLiteral(
        "var got = [], n = 0; Math.random = function(){ return (++n % 9) / 10; };"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id); return false; });"
        "function say(frame, url){ listeners[0]({ __vanillaEnvelope: 1, from: { nonce: null, frameId: frame, tabUrl: url }, message: {} },"
        "                                       { id: chrome.runtime.id, url: url }, function(){}); }"
        "say(0, 'u'); say(0, 'u'); say(7, 'u'); say(0, 'other'); say(7, 'u');"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("5"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0] === got[1]")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("got[2] === got[4]")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("new Set(got).size")), QStringLiteral("3"));
    QCOMPARE(Value(&worker, QStringLiteral("got.every(function(v){ return v >= 0x40000000; })")), QStringLiteral("true"));
}

void tst_cdpshims::aCertainTabIdOutlivesWorkerCachePressure(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id); return false; });"
        "var mine = '0c000000000000000000000000000000';"
        "function say(name, n){ listeners[0]({ __vanillaEnvelope: 1, from: { nonce: name, frameId: n, tabUrl: 'u' }, message: {} },"
        "                                    { id: chrome.runtime.id, url: 'u' }, function(){}); }"
        "answers.push({ ok: true, value: { id: 4242, index: 0 } });"
        "say(mine, 0);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.join()")), QStringLiteral("4242"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral(
        "for (var i = 1; i <= 5200; i++) { say(null, i); if (i % 64 === 0) say(mine, 0); }"
        "say(mine, 0);"));
    QCOMPARE(Value(&worker, QStringLiteral("got[got.length - 1]")), QStringLiteral("4242"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));
}

void tst_cdpshims::theAskingAgainStopsAfterItsRounds(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id); return false; });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '07000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0] >= 0x40000000")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("30"));
}

void tst_cdpshims::aLaterRoundDeliversWhatItHoldsByItsDeadline(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id); return false; });"
        "var mine = '08000000000000000000000000000000';"
        "function say(name, n){ listeners[0]({ __vanillaEnvelope: 1, from: { nonce: name, frameId: n, tabUrl: 'u' }, message: {} },"
        "                                    { id: chrome.runtime.id, url: 'u' }, function(){}); }"
        "say(mine, 0);"));
    DrainBefore(&worker, 1000);
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("1"));
    const QString provisional = Value(&worker, QStringLiteral("got[0]"));

    worker.evaluate(QStringLiteral("answers.push('HELD'); runNextTimer();"));
    QCoreApplication::processEvents();
    QCOMPARE(Value(&worker, QStringLiteral("held.length")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("for (var i = 1; i <= 4200; i++) say(null, i);"));
    const int flooded = Value(&worker, QStringLiteral("got.length")).toInt();
    worker.evaluate(QStringLiteral("say(mine, 0);"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length")).toInt(), flooded);

    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got.length")).toInt(), flooded + 1);
    QCOMPARE(Value(&worker, QStringLiteral("got[got.length - 1]")), provisional);
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("25"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("0"));
}

void tst_cdpshims::theAskingAgainKeepsOneChainAcrossAnEviction(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id); return false; });"
        "var mine = '09000000000000000000000000000000';"
        "function say(name, n){ listeners[0]({ __vanillaEnvelope: 1, from: { nonce: name, frameId: n, tabUrl: 'u' }, message: {} },"
        "                                    { id: chrome.runtime.id, url: 'u' }, function(){}); }"
        "say(mine, 0);"));
    DrainBefore(&worker, 1000);
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("6"));

    worker.evaluate(QStringLiteral("for (var i = 1; i <= 4200; i++) say(null, i);"));
    worker.evaluate(QStringLiteral("say(mine, 0);"));
    DrainBefore(&worker, 1000);
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("12"));

    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("36"));
}

void tst_cdpshims::aKeptChannelCanBeWonByAPromise(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '03000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s, r){ return true; });"
        "chrome.runtime.onMessage.addListener(function(m, s, r){ return Promise.resolve('by promise'); });"
        "answers.push({ ok: true, value: { id: 555, index: 0 } });"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("by promise"));

    worker.evaluate(QStringLiteral("answered = [];"));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); }))").arg(env)),
             QStringLiteral("true"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("by promise"));
}

void tst_cdpshims::aKeptChannelIsNotClosedByAPromiseWithNothing(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '0a000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ setTimeout(function(){ respond('from the kept one'); }, 0); return true; });"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve(undefined); });"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.reject(new Error('not mine')); });"
        "answers.push({ ok: true, value: { id: 111, index: 0 } });"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); });").arg(env));
    Pump(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("from the kept one"));

    worker.evaluate(QStringLiteral("answered = [];"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); });").arg(env));
    Pump(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("from the kept one"));
}

void tst_cdpshims::theAnswerMayBeTheSecondPromise(){
    QJSEngine worker;
    Make(&worker, false);
    worker.evaluate(QStringLiteral(
        "var answered = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve(undefined); });"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve('the real answer'); });"
        "listeners[0]({ handler: 1 }, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("the real answer"));
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
}

void tst_cdpshims::nobodyAnsweringClosesOnlyWhatTheEngineWaitsFor(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '0b000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var answered = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(){ return false; });"
        "chrome.runtime.onMessage.addListener(function(){ return undefined; });"
        "answers.push({ ok: true, value: { id: 222, index: 0 } });"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("undefined"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("answered = []; calls = 0;"));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); }))").arg(env)),
             QStringLiteral("undefined"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("0"));
}

void tst_cdpshims::everyPromiseComingToNothingClosesTheGateOnce(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '0d000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var answered = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve(undefined); });"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.reject(new Error('not mine')); });"
        "answers.push({ ok: true, value: { id: 333, index: 0 } });"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("undefined"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("answered = []; calls = 0;"));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); }))").arg(env)),
             QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("0"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("undefined"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
}

void tst_cdpshims::aPromiseAloneIsToldToTheEngineAsAKeptChannel(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '0e000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var answered = [], settle;"
        "chrome.runtime.onMessage.addListener(function(){ return new Promise(function(res){ settle = res; }); });"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve(undefined); });"
        "answers.push({ ok: true, value: { id: 444, index: 0 } });"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("typeof settle")), QStringLiteral("function"));
    worker.evaluate(QStringLiteral("settle('the slow promise');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("the slow promise"));

    worker.evaluate(QStringLiteral("answered = [];"));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); }))").arg(env)),
             QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("settle('the slow promise again');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("the slow promise again"));
}

void tst_cdpshims::aLoneListenersPromiseIsWaitedOnByTheShim(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '0f000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var answered = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve('by the lone promise'); });"
        "answers.push({ ok: true, value: { id: 555, index: 0 } });"));
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("by the lone promise"));
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("answered = []; calls = 0;"));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); }))").arg(env)),
             QStringLiteral("true"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("by the lone promise"));
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
}

void tst_cdpshims::aLoneListenerWithNoPromiseGoesToTheEngineAsItIs(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '10000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");
    worker.evaluate(QStringLiteral(
        "var word, calls = 0;"
        "chrome.runtime.onMessage.addListener(function(){ return word; });"
        "answers.push({ ok: true, value: { id: 666, index: 0 } });"
        "function say(v){ word = v; return String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(){ calls++; })); }").arg(env));
    worker.evaluate(QStringLiteral("word = false; listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(){ calls++; });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("calls = 0;"));
    QCOMPARE(Value(&worker, QStringLiteral("say(true)")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("say(false)")), QStringLiteral("false"));
    QCOMPARE(Value(&worker, QStringLiteral("say(undefined)")), QStringLiteral("undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("say('a word of its own')")), QStringLiteral("a word of its own"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("0"));
}

void tst_cdpshims::aThenableWhichCallsBackAndThrowsIsCountedOnce(){
    QJSEngine worker;
    Make(&worker, false);
    worker.evaluate(QStringLiteral(
        "var answered = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(){ return { then: function(ok){ ok(undefined); throw new Error('and then some'); } }; });"
        "chrome.runtime.onMessage.addListener(function(){ return Promise.resolve('the real answer'); });"
        "listeners[0]({ handler: 1 }, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; answered.push(String(v)); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("the real answer"));
    QCOMPARE(Value(&worker, QStringLiteral("calls")), QStringLiteral("1"));
}

void tst_cdpshims::aThrownFirstResponseDoesNotCloseTheGate(){
    QJSEngine worker;
    Make(&worker, false);
    worker.evaluate(QStringLiteral(
        "var answered = [], tries = 0;"
        "chrome.runtime.onMessage.addListener(function(m, s, r){ r('first'); return false; });"
        "chrome.runtime.onMessage.addListener(function(m, s, r){ r('second'); return false; });"
        "listeners[0]({ handler: 1 }, { id: chrome.runtime.id, url: 'u' },"
        "             function(v){ tries++; if (tries === 1) throw new Error('the engine refuses'); answered.push(String(v)); });"));
    QCOMPARE(Value(&worker, QStringLiteral("tries")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("second"));
}

void tst_cdpshims::theFastPathAnswersOnlyOnce(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var after = [], answersSeen = [], calls = 0;"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ respond('first'); after.push('first'); return false; });"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ respond('second'); after.push('second'); return false; });"
        "answers.push({ ok: true, value: { id: 77, index: 0 } });"));
    const QString env = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: '05000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} }");

    worker.evaluate(QStringLiteral(
        "listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; if (calls > 1) throw new Error('answered twice'); answersSeen.push(v); });").arg(env));
    QTRY_COMPARE(Value(&worker, QStringLiteral("after.join()")), QStringLiteral("first,second"));
    QCOMPARE(Value(&worker, QStringLiteral("answersSeen.join() + ':' + calls")), QStringLiteral("first:1"));

    worker.evaluate(QStringLiteral("after = []; answersSeen = []; calls = 0;"));
    worker.evaluate(QStringLiteral(
        "listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ calls++; if (calls > 1) throw new Error('answered twice'); answersSeen.push(v); });").arg(env));
    QCOMPARE(Value(&worker, QStringLiteral("after.join()")), QStringLiteral("first,second"));
    QCOMPARE(Value(&worker, QStringLiteral("answersSeen.join() + ':' + calls")), QStringLiteral("first:1"));
}

void tst_cdpshims::aKeptChannelMayAnswerAfterTheLookupDeadline(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ setTimeout(function(){ respond('after four seconds'); }, 4000); return true; });"
        "answers.push({ ok: true, value: { id: 88, index: 0 } });"));
    worker.evaluate(QStringLiteral(
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '06000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(v); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms === 4000; })")), QStringLiteral("true"));
    for(int i = 0; i < 8 && Value(&worker, QStringLiteral("answered.length")) == QStringLiteral("0"); i++){
        worker.evaluate(QStringLiteral("runNextTimer();"));
        QCoreApplication::processEvents();
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("after four seconds"));
}

void tst_cdpshims::aFrameSaysTheAddressOfTheTopOfItsTab(){
    const QString from = QStringLiteral("JSON.stringify([sent[0][0].from.frameId !== 0, sent[0][0].from.tabUrl])");

    QJSEngine same;
    Make(&same, true, QStringLiteral("window.top = { location: { href: 'https://mail.example/inbox' } }; location = { href: 'https://mail.example/frame' };"));
    same.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'initializeFrame' });"));
    QCOMPARE(Value(&same, from), QStringLiteral("[true,\"https://mail.example/inbox\"]"));

    QJSEngine other;
    Make(&other, true, QStringLiteral(
        "window.top = { get location(){ throw new Error('cross-origin'); } };"
        "location = { href: 'https://ads.example/banner', ancestorOrigins: ['https://mid.example', 'https://mail.example'] };"));
    other.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'initializeFrame' });"));
    QCOMPARE(Value(&other, from), QStringLiteral("[true,\"https://mail.example/\"]"));

    QJSEngine alone;
    Make(&alone, true, QStringLiteral(
        "window.top = { get location(){ throw new Error('cross-origin'); } };"
        "location = { href: 'about:srcdoc', ancestorOrigins: ['null'] };"));
    alone.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'initializeFrame' });"));
    QCOMPARE(Value(&alone, from), QStringLiteral("[true,\"about:srcdoc\"]"));
}

void tst_cdpshims::whatTheEngineWillNotLetBePutInIsSaid(){
    const QString before = QStringLiteral("var warned = []; var console = { warn: function(m){ warned.push(m); } };");
    QJSEngine fine;
    Make(&fine, true, before);
    QCOMPARE(Value(&fine, QStringLiteral("warned.length")), QStringLiteral("0"));

    foreach(bool content, QList<bool>() << true << false){
        QJSEngine engine;
        engine.evaluate(World());
        engine.evaluate(before + QStringLiteral("Object.freeze(chrome.storage);"));
        const QJSValue made = engine.evaluate(content ? Cdp::ContentShim() : Cdp::WorkerShim());
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        QVERIFY(!made.toString().contains(QStringLiteral("storage.sync")));
        QCOMPARE(Value(&engine, QStringLiteral("warned.length")), QStringLiteral("1"));
        QVERIFY2(Value(&engine, QStringLiteral("warned[0]")).contains(QStringLiteral("storage.sync")), qPrintable(Value(&engine, QStringLiteral("warned[0]"))));
    }
}

static QString Fetching(){
    return QStringLiteral(
        "var fetched = [], answers = [], held = [], reading = 0, named = [], namedDown = 0;"
        "function fetch(url, options){"
        "  var said = options && options.headers && options.headers['X-Vanilla-Call'];"
        "  if (said && JSON.parse(decodeURIComponent(said)).api === 'vanilla.eventNames') {"
        "    named.push(JSON.parse(decodeURIComponent(said)).args);"
        "    if (namedDown > 0) { namedDown--; return Promise.reject(new TypeError('Failed to fetch')); }"
        "    return Promise.resolve({ json: function(){ return Promise.resolve({ ok: true }); } }); }"
        "  fetched.push({ url: url, options: options });"
        "  var answer = answers.shift();"
        "  var read = function(a){"
        "    if (a === 'NOT JSON') return Promise.reject(new SyntaxError('Unexpected token')).then(undefined, function(e){ reading--; throw e; });"
        "    return Promise.resolve(a).then(function(v){ reading--; return v; });"
        "  };"
        "  var coming = function(a){ reading++; return { json: function(){ return read(a); } }; };"
        "  if (answer === 'HELD') return new Promise(function(res){ held.push(function(){ res(coming(answers.shift())); }); });"
        "  if (answer === 'DOWN') { reading++; return Promise.reject(new TypeError('Failed to fetch')).then(undefined, function(e){ reading--; throw e; }); }"
        "  return Promise.resolve(coming(answer));"
        "}");
}

void tst_cdpshims::whatTheApplicationAnswersForIsAskedOfIt(){
    const QString key = QString(64, QLatin1Char('5'));
    QJSEngine engine;
    engine.evaluate(World());
    engine.evaluate(QStringLiteral("document = undefined;") + Fetching());
    QVERIFY(Cdp::WorkerShim().contains(QLatin1String(ExtensionHostWire::KEY_PLACE)));
    QVERIFY(!engine.evaluate(Cdp::WorkerShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), key)).isError());

    engine.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, active: true }] });"
        "chrome.tabs.query({ active: true, currentWindow: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"active\":true}]"));
    QCOMPARE(Value(&engine, QStringLiteral("fetched[0].url + ' ' + fetched[0].options.method + ' ' + ('body' in fetched[0].options)")),
             QStringLiteral("vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&engine, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key']")), key);
    QCOMPARE(Value(&engine, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true,\"currentWindow\":true}]}"));
    QVERIFY(!Value(&engine, QStringLiteral("fetched[0].url + JSON.stringify(Object.keys(fetched[0].options))")).contains(key));

    engine.evaluate(QStringLiteral(
        "var refused = 'pending', down = 'pending', garbled = 'pending', odd = 'pending';"
        "answers.push({ ok: false, error: 'No tab with id: 99.' }, 'DOWN', 'NOT JSON', { value: 'an answer with no ok' });"
        "chrome.tabs.get(99).then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"
        "chrome.tabs.get(1).then(function(){ down = 'resolved'; }, function(e){ down = e.message; });"
        "chrome.tabs.get(2).then(function(){ garbled = 'resolved'; }, function(e){ garbled = e.message; });"
        "chrome.tabs.get(3).then(function(){ odd = 'resolved'; }, function(e){ odd = e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("refused")), QStringLiteral("No tab with id: 99."));
    QTRY_COMPARE(Value(&engine, QStringLiteral("down")), QStringLiteral("chrome.tabs.get is not available in this browser"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("garbled")), QStringLiteral("chrome.tabs.get is not available in this browser"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("odd")), QStringLiteral("chrome.tabs.get is not available in this browser"));

    engine.evaluate(QStringLiteral(
        "var called = [];"
        "answers.push({ ok: true, value: { id: 12 } }, { ok: false, error: 'No tab with id: 98.' });"
        "chrome.tabs.get(12, function(tab){ called.push('tab ' + (tab && tab.id) + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine')); });"
        "chrome.tabs.get(98, function(tab){ called.push('tab ' + tab + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR')); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("called.length + timers.length >= 2")), QStringLiteral("true"));
    QCoreApplication::processEvents();
    engine.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("called.slice().sort().join(' | ')")), QStringLiteral("tab 12 fine | tab undefined No tab with id: 98."));
    QCOMPARE(Value(&engine, QStringLiteral("fetched[fetched.length - 1].options.headers['X-Vanilla-Call'].indexOf('function')")), QStringLiteral("-1"));

    engine.evaluate(QStringLiteral("var before = fetched.length, other = 'pending'; chrome.tabs.group({ tabIds: 12 }).then(function(){ other = 'resolved'; }, function(e){ other = e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("other")), QStringLiteral("chrome.tabs.group is not available in this browser"));
    QCOMPARE(Value(&engine, QStringLiteral("fetched.length - before")), QStringLiteral("0"));
    QCOMPARE(Value(&engine, QStringLiteral("typeof chrome.tabs.onActivated.addListener")), QStringLiteral("function"));
}

void tst_cdpshims::theTabsAndWindowsTheApplicationChangesAreAskedOfIt(){
    QJSEngine engine;
    MakeAsking(&engine, Fetching());
    engine.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: { id: 7, index: 2 } }, { ok: true, value: { id: 7, index: 2 } },"
        "             { ok: true }, { ok: true },"
        "             { ok: true, value: { id: 1, focused: true } }, { ok: true, value: [{ id: 1, incognito: false }] });"
        "Promise.all([chrome.tabs.create({ url: 'https://a.example/', active: true }),"
        "             chrome.tabs.update(7, { url: 'https://a.example/' }),"
        "             chrome.tabs.remove([7, 8]),"
        "             chrome.tabs.reload(7, { bypassCache: true }),"
        "             chrome.windows.getCurrent(),"
        "             chrome.windows.getAll({ populate: true })])"
        "  .then(function(v){ out = v.map(function(x){ return x === undefined ? 'nothing' : JSON.stringify(x); }).join(' | '); },"
        "        function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")),
                 QStringLiteral("{\"id\":7,\"index\":2} | {\"id\":7,\"index\":2} | nothing | nothing"
                                " | {\"id\":1,\"focused\":true} | [{\"id\":1,\"incognito\":false}]"));
    QCOMPARE(Value(&engine, QStringLiteral("fetched.map(function(f){ return decodeURIComponent(f.options.headers['X-Vanilla-Call']); }).join('\\n')")),
             QStringLiteral("{\"api\":\"tabs.create\",\"args\":[{\"url\":\"https://a.example/\",\"active\":true}]}\n"
                            "{\"api\":\"tabs.update\",\"args\":[7,{\"url\":\"https://a.example/\"}]}\n"
                            "{\"api\":\"tabs.remove\",\"args\":[[7,8]]}\n"
                            "{\"api\":\"tabs.reload\",\"args\":[7,{\"bypassCache\":true}]}\n"
                            "{\"api\":\"windows.getCurrent\",\"args\":[]}\n"
                            "{\"api\":\"windows.getAll\",\"args\":[{\"populate\":true}]}"));

    engine.evaluate(QStringLiteral(
        "var called = [];"
        "answers.push({ ok: true }, { ok: true, value: [{ id: 1, incognito: false }] });"
        "chrome.tabs.remove(7, function(v){ called.push('remove ' + v + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine')); });"
        "chrome.windows.getAll(null, function(ws){ called.push('windows ' + JSON.stringify(ws)); });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("called.slice().sort().join(' | ')")),
                 QStringLiteral("remove undefined fine | windows [{\"id\":1,\"incognito\":false}]"));
    QCOMPARE(Value(&engine, QStringLiteral("decodeURIComponent(fetched[fetched.length - 2].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.remove\",\"args\":[7]}"));
    QCOMPARE(Value(&engine, LastCall()),
             QStringLiteral("{\"api\":\"windows.getAll\",\"args\":[null]}"));
}

void tst_cdpshims::anAnswerWithNoValueResolvesWithNothing(){
    QJSEngine engine;
    MakeAsking(&engine, Fetching());
    engine.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true });"
        "chrome.tabs.remove(12).then(function(v){ out = 'resolved ' + (v === undefined ? 'nothing' : JSON.stringify(v)); },"
        "                            function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("resolved nothing"));
}

void tst_cdpshims::whatTheApplicationAnswersForBeatsTheEnginesOwn(){
    QJSEngine engine;
    MakeAsking(&engine, Fetching(), EnginesOwn());
    engine.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: { id: 5, index: 1 } });"
        "chrome.tabs.update(5, { active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("{\"id\":5,\"index\":1}"));
    QCOMPARE(Value(&engine, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.update\",\"args\":[5,{\"active\":true}]}"));
    QCOMPARE(Value(&engine, QStringLiteral("engineCalls.join()")), QString());
    QCOMPARE(Value(&engine, QStringLiteral("chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1"));
    QCOMPARE(Value(&engine, QStringLiteral("['update', 'onActivated', 'create', 'TAB_ID_NONE', 'MAX_A'].map(function(k){ return k in chrome.tabs; }).join()")),
             QStringLiteral("true,true,true,true,false"));

    engine.evaluate(QStringLiteral(
        "var win = 'pending';"
        "answers.push({ ok: true, value: { id: 1, focused: true } });"
        "chrome.windows.getCurrent().then(function(v){ win = JSON.stringify(v); }, function(e){ win = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("win")), QStringLiteral("{\"id\":1,\"focused\":true}"));
    QCOMPARE(Value(&engine, QStringLiteral("engineCalls.join()")), QString());
    QCOMPARE(Value(&engine, QStringLiteral("chrome.windows === enginesWindows")), QStringLiteral("false"));
}

void tst_cdpshims::withoutAKeyTheStandInAnswersRatherThanTheEnginesOwn(){
    QJSEngine engine;
    Make(&engine, false, Fetching() + EnginesOwn());
    engine.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.tabs.update(5, { active: true }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("chrome.tabs.update is not available in this browser"));
    QCOMPARE(Value(&engine, QStringLiteral("engineCalls.join()")), QString());
    QCOMPARE(Value(&engine, QStringLiteral("fetched.length")), QStringLiteral("0"));
    QCOMPARE(Value(&engine, QStringLiteral("chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1"));
    QCOMPARE(Value(&engine, QStringLiteral("typeof chrome.tabs.onActivated.addListener")), QStringLiteral("function"));
    QCOMPARE(Value(&engine, QStringLiteral("chrome.windows === enginesWindows")), QStringLiteral("true"));
}

void tst_cdpshims::theEnginesOwnCallsAndEventsAreNotPassedThroughButItsNumbersAre(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), QStringLiteral(
        "var engineCalls = [];"
        "chrome.tabs = { TAB_ID_NONE: -1,"
        "                discard: function(){ engineCalls.push('tabs.discard'); return Promise.resolve('the engine\\'s own'); },"
        "                onReplaced: { addListener: function(){ engineCalls.push('tabs.onReplaced.addListener'); },"
        "                           removeListener: function(){}, hasListener: function(){ return true; },"
        "                           hasListeners: function(){ return true; } } };"
        "self.__vanillaMessages = { locale: 'ja_JP', messages: { greeting: { message: 'ours' } } };"
        "chrome.i18n = { mark: 'the engine\\'s own', getMessage: function(){ return ''; },"
        "                getUILanguage: function(){ return this && this.mark ? this.mark : 'unbound'; } };"));
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.tabs.discard(5).then(function(v){ out = 'resolved: ' + v; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("chrome.tabs.discard is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("engineCalls.join() + '/' + fetched.length")), QStringLiteral("/0"));

    worker.evaluate(QStringLiteral("var moved = function(){}; chrome.tabs.onReplaced.addListener(moved);"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "chrome.tabs.onReplaced.hasListener(moved) + '/' + chrome.tabs.onReplaced.hasListeners() + '/' + engineCalls.join()")),
             QStringLiteral("false/false/"));

    QCOMPARE(Value(&worker, QStringLiteral("chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1"));

    QCOMPARE(Value(&worker, QStringLiteral(
                 "chrome.i18n.getMessage('greeting') + '/' + chrome.i18n.getUILanguage()")),
             QStringLiteral("ours/the engine's own"));
}

void tst_cdpshims::whatTakesTheEngineDownIsNotPassedThrough(){
    QJSEngine worker;
    worker.evaluate(World());
    worker.evaluate(QStringLiteral("document = undefined;") + Fetching() + QStringLiteral(
        "var engineCalls = [];"
        "chrome.declarativeNetRequest = { MAX_NUMBER_OF_DYNAMIC_RULES: 30000,"
        "    updateSessionRules: function(){ engineCalls.push('updateSessionRules'); return Promise.resolve(); },"
        "    updateDynamicRules: function(){ engineCalls.push('updateDynamicRules'); return Promise.resolve(); },"
        "    getDynamicRules: function(){ engineCalls.push('getDynamicRules'); return Promise.resolve([]); } };"));
    const QJSValue made = worker.evaluate(Cdp::WorkerShim());
    QVERIFY2(!made.isError(), qPrintable(made.toString()));
    worker.evaluate(QStringLiteral(
        "var outs = [];"
        "chrome.declarativeNetRequest.updateSessionRules({}).then(function(){ outs.push('resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.declarativeNetRequest.updateDynamicRules({}, function(){ outs.push('callback: ' + (chrome.runtime.lastError || {}).message); });"
        "chrome.declarativeNetRequest.getDynamicRules().then(function(r){ outs.push('rules ' + r.length); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("outs.slice().sort().join('|')")),
             QStringLiteral("callback: chrome.declarativeNetRequest.updateDynamicRules is not available in this browser|"
                            "chrome.declarativeNetRequest.updateSessionRules is not available in this browser|rules 0"));
    QCOMPARE(Value(&worker, QStringLiteral("engineCalls.join() + '/' + fetched.length")), QStringLiteral("getDynamicRules/0"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.declarativeNetRequest.MAX_NUMBER_OF_DYNAMIC_RULES")), QStringLiteral("30000"));
}

void tst_cdpshims::whatTheManifestGrantsIsAnsweredOutOfIt(){
    const QString manifest = QStringLiteral(
        "chrome.runtime.getManifest = function(){ return { permissions: ['storage', 'scripting'],"
        "    host_permissions: ['https://a.example/*'], optional_permissions: ['tabs'],"
        "    content_scripts: [{ matches: ['https://b.example/*', 'https://a.example/*'] }] }; };"
        "var engineCalls = [];"
        "chrome.permissions = { getAll: function(){ engineCalls.push('getAll'); return Promise.resolve({ permissions: ['engine'], origins: [] }); },"
        "                       onAdded: { addListener: function(){ engineCalls.push('onAdded'); } } };");
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), manifest);
        worker.evaluate(QStringLiteral(
            "var outs = {};"
            "chrome.permissions.getAll().then(function(r){ outs.all = JSON.stringify(r); });"
            "chrome.permissions.contains({ permissions: ['storage'], origins: ['https://b.example/*'] }).then(function(r){ outs.held = r; });"
            "chrome.permissions.contains({ permissions: ['tabs'] }).then(function(r){ outs.optional = r; });"
            "chrome.permissions.contains({ origins: ['https://c.example/*'] }).then(function(r){ outs.other = r; });"
            "chrome.permissions.contains().then(null, function(e){ outs.bad = 'refused'; });"
            "chrome.scripting.getRegisteredContentScripts(function(r){ outs.scripts = JSON.stringify(r); });"
            "chrome.permissions.onAdded.addListener(function(){});"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("Object.keys(outs).length")), QStringLiteral("6"));
        QCOMPARE(Value(&worker, QStringLiteral("outs.all")),
                 QStringLiteral("{\"permissions\":[\"storage\",\"scripting\"],\"origins\":[\"https://a.example/*\",\"https://b.example/*\"]}"));
        QCOMPARE(Value(&worker, QStringLiteral("[outs.held, outs.optional, outs.other, outs.bad, outs.scripts].join()")),
                 QStringLiteral("true,false,false,refused,[]"));
        QCOMPARE(Value(&worker, QStringLiteral("engineCalls.join() + '/' + fetched.length")), QStringLiteral("onAdded/0"));
    }
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), QStringLiteral("chrome.runtime.getManifest = function(){ return { host_permissions: ['<all_urls>'] }; };"));
        worker.evaluate(QStringLiteral("var held = null; chrome.permissions.contains({ origins: ['https://c.example/*'] }).then(function(r){ held = r; });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("held")), QStringLiteral("true"));
    }
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), Saying() + QStringLiteral(
            "chrome.runtime.getManifest = function(){ return { permissions: ['storage'], host_permissions: ['*://*/*'],"
            "    optional_permissions: ['contextMenus'], optional_host_permissions: ['file:///*'] }; };"));
        worker.evaluate(QStringLiteral(
            "var outs = {};"
            "chrome.permissions.request({ origins: ['*://*.a.example/*'] }).then(function(r){ outs.held = r; });"
            "chrome.permissions.request({ permissions: ['storage'] }, function(r){ outs.callback = r; });"
            "chrome.permissions.request({ permissions: ['contextMenus'] }).then(function(r){ outs.optional = r; });"
            "chrome.permissions.request({ permissions: ['storage'], origins: ['file:///*'] }).then(function(r){ outs.optionalOrigin = r; });"
            "chrome.permissions.request({ permissions: ['tabs'] }).then(function(){ outs.unnamed = 'granted?'; }, function(e){ outs.unnamed = e.message; });"
            "chrome.permissions.request({ permissions: ['storage', 'tabs'] }).then(function(){ outs.mixed = 'granted?'; }, function(e){ outs.mixed = 'refused'; });"
            "chrome.permissions.request({ origins: ['ftp://c.example/*'] }).then(function(r){ outs.unplaced = r; });"
            "chrome.permissions.request().then(null, function(e){ outs.bad = 'refused'; });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("Object.keys(outs).length")), QStringLiteral("8"));
        QCOMPARE(Value(&worker, QStringLiteral("outs.mixed + '/' + outs.unplaced")), QStringLiteral("refused/false"));
        QCOMPARE(Value(&worker, QStringLiteral("[outs.held, outs.callback, outs.optional, outs.optionalOrigin, outs.bad].join()")),
                 QStringLiteral("true,true,false,false,refused"));
        QCOMPARE(Value(&worker, QStringLiteral("outs.unnamed")), QStringLiteral("Only permissions specified in the manifest may be requested."));
        QCOMPARE(Value(&worker, QStringLiteral("warned.length + '/' + fetched.length")), QStringLiteral("1/0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, manifest);
        worker.evaluate(QStringLiteral("var all = null; chrome.permissions.getAll().then(function(r){ all = r.permissions.join(); });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("all")), QStringLiteral("engine"));
    }
}

void tst_cdpshims::whichRulesetsAreEnabledIsAskedOfTheHost(){
    const QString engines = QStringLiteral(
        "var engineCalls = [];"
        "chrome.declarativeNetRequest = {"
        "  getEnabledRulesets: function(){ engineCalls.push('get'); return Promise.resolve(['engine']); },"
        "  updateEnabledRulesets: function(){ engineCalls.push('update'); return Promise.resolve(); } };");
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), engines + Calls());
        worker.evaluate(QStringLiteral(
            "var outs = [];"
            "answers.push({ ok: true, value: ['a', 'b'] }, { ok: true });"
            "chrome.declarativeNetRequest.getEnabledRulesets().then(function(r){ outs.push(r.join()); });"
            "chrome.declarativeNetRequest.updateEnabledRulesets({ enableRulesetIds: ['c'] }).then(function(){ outs.push('updated'); });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("outs.slice().sort().join('|')")), QStringLiteral("a,b|updated"));
        QCOMPARE(Value(&worker, QStringLiteral("call(0).api + ' ' + call(1).api + ' ' + JSON.stringify(call(1).args)")),
                 QStringLiteral("declarativeNetRequest.getEnabledRulesets declarativeNetRequest.updateEnabledRulesets [{\"enableRulesetIds\":[\"c\"]}]"));
        QCOMPARE(Value(&worker, QStringLiteral("engineCalls.length")), QStringLiteral("0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, engines);
        worker.evaluate(QStringLiteral("var got = null; chrome.declarativeNetRequest.getEnabledRulesets().then(function(r){ got = r.join(); });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("engine"));
    }
}

void tst_cdpshims::whatTheExtensionPutsOnANamespaceIsReadBack(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Calls());
    worker.evaluate(QStringLiteral(
        "chrome.tabs.render = function(){ return 'rendered'; }; chrome.tabs._mine = 1;"
        "var hosts = chrome.tabs.query;"
        "chrome.tabs.query = function(){ return 'wrapped'; };"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.tabs.render() + ' ' + ('_mine' in chrome.tabs) + ' ' + chrome.tabs.query()")),
             QStringLiteral("rendered true wrapped"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("delete chrome.tabs.query; answers.push({ ok: true, value: [] }); var got = null; chrome.tabs.query({}).then(function(r){ got = r.length; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.tabs.query === hosts")), QStringLiteral("true"));
}

void tst_cdpshims::anExtensionsOwnRulesAreHeldByTheHost(){
    const QString engines = QStringLiteral(
        "var engineCalls = [];"
        "chrome.declarativeNetRequest = {"
        "  getDynamicRules: function(){ engineCalls.push('get'); return Promise.resolve(['engine']); },"
        "  updateSessionRules: function(){ engineCalls.push('update'); return Promise.resolve(); } };");
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), engines + Calls());
        worker.evaluate(QStringLiteral(
            "var outs = [];"
            "answers.push({ ok: true }, { ok: true, value: [{ id: 1 }] }, { ok: true });"
            "chrome.declarativeNetRequest.updateDynamicRules({ addRules: [{ id: 1 }] }).then(function(){ outs.push('dynamic'); });"
            "chrome.declarativeNetRequest.getDynamicRules({ ruleIds: [1] }).then(function(r){ outs.push('got ' + r.length); });"
            "chrome.declarativeNetRequest.updateSessionRules({ removeRuleIds: [2] }, function(){ outs.push('session'); });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("outs.slice().sort().join('|')")), QStringLiteral("dynamic|got 1|session"));
        QCOMPARE(Value(&worker, QStringLiteral(
            "[0, 1, 2].map(function(i){ return call(i).api.split('.')[1] + ' ' + JSON.stringify(call(i).args) + ' ' + fetched[i].options.body; }).join('|')")),
            QStringLiteral("updateDynamicRules [] [{\"addRules\":[{\"id\":1}]}]|getDynamicRules [{\"ruleIds\":[1]}] undefined|"
                           "updateSessionRules [] [{\"removeRuleIds\":[2]}]"));
        QCOMPARE(Value(&worker, QStringLiteral("engineCalls.length")), QStringLiteral("0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, engines);
        worker.evaluate(QStringLiteral("var got = null; chrome.declarativeNetRequest.getDynamicRules().then(function(r){ got = r.join(); });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("engine"));
    }
}

void tst_cdpshims::theStandInsOutliveTheEnginesOwnBeingBuiltAgain(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), EnginesOwn() + QStringLiteral("var enginesChrome = chrome;"));
    QCOMPARE(Value(&worker, QStringLiteral("(chrome !== enginesChrome) + '/' + typeof chrome")), QStringLiteral("true/object"));

    worker.evaluate(QStringLiteral(
        "var rebuilt = { TAB_ID_NONE: -99, MARK: 7,"
        "                update: function(){ engineCalls.push('rebuilt.update'); return 'the rebuilt one'; } };"
        "self.chrome.tabs = rebuilt;"));
    QCOMPARE(Value(&worker, QStringLiteral("(Object.getOwnPropertyDescriptor(enginesChrome, 'tabs').value === rebuilt)"
                                           " + '/' + typeof chrome.tabs.query + '/' + String(chrome.tabs.MARK)")),
             QStringLiteral("true/function/undefined"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, index: 0 }] });"
        "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":0}]"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true}]}"));

    worker.evaluate(QStringLiteral(
        "var up = 'pending';"
        "answers.push({ ok: true, value: { id: 5, index: 1 } });"
        "chrome.tabs.update(5, { active: true }).then(function(v){ up = JSON.stringify(v); }, function(e){ up = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("up")), QStringLiteral("{\"id\":5,\"index\":1}"));
    QCOMPARE(Value(&worker, QStringLiteral("engineCalls.join()")), QString());

    QCOMPARE(Value(&worker, QStringLiteral("chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1"));

    worker.evaluate(QStringLiteral("self.chrome.windows = { WINDOW_ID_NONE: -99, MARK: 8 };"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.windows.WINDOW_ID_NONE + '/' + String(chrome.windows.MARK)"
                                           " + '/' + typeof chrome.windows.getCurrent")),
             QStringLiteral("-1/undefined/function"));
}

void tst_cdpshims::whatIsNobodysStandInIsReadOffTheEnginesObject(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), QStringLiteral(
        "var enginesChrome = chrome;"
        "chrome.saysWho = function(){ return this === enginesChrome ? 'the object' : 'somebody else'; };"));
    QCOMPARE(Value(&worker, QStringLiteral("(chrome !== enginesChrome) + '/' + typeof chrome")), QStringLiteral("true/object"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.saysWho()")), QStringLiteral("the object"));

    QCOMPARE(Value(&worker, QStringLiteral("(chrome.runtime === enginesChrome.runtime) + '/' + (chrome.storage === enginesChrome.storage)")),
             QStringLiteral("true/true"));
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.runtime.onMessage.addListener")), QStringLiteral("function"));

    QCOMPARE(Value(&worker, QStringLiteral("('tabs' in chrome) + '/' + ('runtime' in chrome) + '/' + ('nosuch' in chrome)")),
             QStringLiteral("true/true/false"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "['runtime', 'storage', 'tabs', 'windows', 'saysWho'].map(function(k){ return Object.keys(chrome).indexOf(k) >= 0; }).join()")),
             QStringLiteral("true,true,true,true,true"));

    worker.evaluate(QStringLiteral("chrome.i18n = { getMessage: function(){ return 'said'; } };"));
    QCOMPARE(Value(&worker, QStringLiteral("(enginesChrome.i18n === chrome.i18n) + '/' + chrome.i18n.getMessage()")),
             QStringLiteral("true/said"));

    worker.evaluate(QStringLiteral(
        "Object.defineProperty(enginesChrome, 'contextMenus', { value: { locked: 1 }, writable: false, configurable: false });"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.contextMenus.locked + '/' + (chrome.contextMenus === enginesChrome.contextMenus)")),
             QStringLiteral("1/true"));
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.tabs.query")), QStringLiteral("function"));
}

void tst_cdpshims::theMessagesTheCopyWroteAreAnsweredAsChromeAnswersThem(){
    const QString engines = QStringLiteral(
        "chrome.i18n = { getMessage: function(){ return ''; }, getUILanguage: function(){ return 'engine'; } };"
        "var enginesChrome = chrome;");
    const QString table = QStringLiteral(
        "self.__vanillaMessages = { locale: 'ja_JP', messages: {"
        "  greeting: { message: 'Hello $NAME$, $$ $2 $x', placeholders: { name: { content: '$1' } } },"
        "  plain: { message: '<b>$1' }, odd: { message: 5 } } };");
    const QString asks = QStringLiteral(
        "[chrome.i18n.getMessage('GREETING', ['Ann', 'x']),"
        " chrome.i18n.getMessage('greeting', 'Ann'),"
        " chrome.i18n.getMessage('greeting'),"
        " chrome.i18n.getMessage('nosuch'), chrome.i18n.getMessage(5), chrome.i18n.getMessage('odd'),"
        " chrome.i18n.getMessage('plain', ['<i>'], { escapeLt: true }),"
        " chrome.i18n.getMessage('@@extension_id'), chrome.i18n.getMessage('@@ui_locale'), chrome.i18n.getMessage('@@bidi_dir'),"
        " chrome.i18n.getMessage('plain', ['1', '2', '3', '4', '5', '6', '7', '8', '9']),"
        " String(chrome.i18n.getMessage('plain', ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10'])),"
        " String(chrome.i18n.getMessage('nosuch', ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10'])),"
        " chrome.i18n.getUILanguage()].join('|')");
    const QString answered = QStringLiteral("Hello Ann, $ x $x|Hello Ann, $  $x|Hello , $  $x|||"
                                            "|&lt;b><i>|abcdefghijklmnopabcdefghijklmnop|ja_JP|ltr|<b>1|undefined|undefined|engine");
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), engines + table);
        QCOMPARE(Value(&worker, asks), answered);
        QCOMPARE(Value(&worker, QStringLiteral("('i18n' in chrome) + '/' + ('getMessage' in chrome.i18n)")), QStringLiteral("true/true"));
        worker.evaluate(QStringLiteral("self.chrome.i18n = { getMessage: function(){ return 'rebuilt'; } };"));
        QCOMPARE(Value(&worker, QStringLiteral("chrome.i18n.getMessage('plain', 'x')")), QStringLiteral("<b>x"));
    }
    {
        QJSEngine content;
        Make(&content, true, engines + table);
        QCOMPARE(Value(&content, asks), answered);
        QCOMPARE(Value(&content, QStringLiteral("chrome.i18n === enginesChrome.i18n")), QStringLiteral("true"));
    }
    {
        QJSEngine page;
        MakePage(&page, engines + table);
        QCOMPARE(Value(&page, asks), answered);
    }
    foreach(int kind, QList<int>() << 0 << 1 << 2){
        QJSEngine engine;
        if(kind == 0) MakeAsking(&engine, Fetching(), engines);
        else if(kind == 1) Make(&engine, true, engines);
        else MakePage(&engine, engines);
        QCOMPARE(Value(&engine, QStringLiteral("chrome.i18n.getMessage('greeting') + '/' + (chrome.i18n === enginesChrome.i18n)")),
                 QStringLiteral("/true"));
        engine.evaluate(QStringLiteral("chrome.i18n = { getMessage: function(){ return 'said'; } };"));
        QCOMPARE(Value(&engine, QStringLiteral("(enginesChrome.i18n === chrome.i18n) + '/' + chrome.i18n.getMessage()")),
                 QStringLiteral("true/said"));
    }
}

void tst_cdpshims::theExtensionNamespaceAnswersNoInEachContext(){
    const QString asks = QStringLiteral(
        "var out = 'pending';"
        "chrome.extension.isAllowedFileSchemeAccess(function(v){ out = 'callback:' + v; });"
        "chrome.extension.isAllowedIncognitoAccess().then(function(v){ out += ' promise:' + v; });"
        "String(chrome.extension.inIncognitoContext)");
    foreach(int kind, QList<int>() << 0 << 1 << 2){
        QJSEngine engine;
        if(kind == 0) MakeAsking(&engine, Fetching());
        else if(kind == 1) Make(&engine, true);
        else MakePage(&engine);
        QCOMPARE(Value(&engine, asks), QStringLiteral("false"));
        QTRY_COMPARE(Value(&engine, QStringLiteral("out")), QStringLiteral("callback:false promise:false"));
    }
    QJSEngine own;
    MakeAsking(&own, Fetching(), QStringLiteral("var engines = { inIncognitoContext: true }; chrome.extension = engines;"));
    QCOMPARE(Value(&own, QStringLiteral("(chrome.extension === engines) + '/' + typeof chrome.extension.isAllowedFileSchemeAccess")),
             QStringLiteral("true/undefined"));
}

void tst_cdpshims::theButtonAndTheMenuAreAskedOfTheApplication(){
    const QString before = QStringLiteral(
        "var location = { href: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/background_scripts/main.js',"
        "                 origin: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop' };"
        "self.location = location;"
        "function URL(path, base){"
        "  var root = 'chrome-extension://abcdefghijklmnopabcdefghijklmnop';"
        "  if (/^[a-z]+:/.test(path)) { this.origin = path.slice(0, path.indexOf('/', 20)); this.pathname = path.slice(this.origin.length); return; }"
        "  var dir = base.slice(0, base.lastIndexOf('/') + 1);"
        "  var full = path.charAt(0) === '/' ? root + path : dir + path;"
        "  while (full.indexOf('/../') >= 0) full = full.replace(/\\/[^\\/]+\\/\\.\\.\\//, '/');"
        "  this.origin = root; this.pathname = full.slice(root.length);"
        "}"
        "var drawn = [];"
        "function OffscreenCanvas(w, h){"
        "  this.getContext = function(){ return { putImageData: function(img){ drawn.push(img.width + 'x' + img.height); } }; };"
        "  this.convertToBlob = function(){ return Promise.resolve({ arrayBuffer: function(){ return Promise.resolve(new Uint8Array([137, 80, 78, 71]).buffer); } }); };"
        "}"
        "var btoa = function(s){ return 'B64(' + s.length + ')'; };");
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), before);
    worker.evaluate(Calls());
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.action.setIcon + '/' + typeof chrome.action.getBadgeText + '/' + typeof chrome.contextMenus.create")),
             QStringLiteral("function/function/function"));

    worker.evaluate(QStringLiteral(
        "var out = [];"
        "answers.push({ ok: true }); answers.push({ ok: true }); answers.push({ ok: true });"
        "chrome.action.setIcon({ path: { 16: '../icons/a16.png', 32: '/icons/a32.png' }, tabId: 3 }).then(function(v){ out.push('table:' + v); });"
        "chrome.action.setIcon({ path: '../icons/one.png' }).then(function(){ out.push('one'); });"
        "chrome.action.setIcon({ path: 'https://x.example/i.png' }).then(function(){ out.push('elsewhere'); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.join()")), QStringLiteral("table:undefined,one,elsewhere"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(0))")),
             QStringLiteral("{\"api\":\"action.setIcon\",\"args\":[{\"tabId\":3,\"path\":{\"16\":\"icons/a16.png\",\"32\":\"icons/a32.png\"}}]}"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(1).args)")), QStringLiteral("[{\"path\":\"icons/one.png\"}]"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(2).args)")), QStringLiteral("[{\"path\":\"https://x.example/i.png\"}]"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true });"
        "var image = function(n){ return { width: n, height: n, data: new Uint8Array(n * n * 4) }; };"
        "chrome.action.setIcon({ imageData: { 16: image(16), 48: image(48), 128: image(128) } }, function(){ out.push('drawn ' + (chrome.runtime.lastError ? 'error' : 'ok')); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out[3]")), QStringLiteral("drawn ok"));
    QCOMPARE(Value(&worker, QStringLiteral("drawn.join() + ' ' + JSON.stringify(call(3).args)")),
             QStringLiteral("48x48 [{\"imageData\":{\"png\":\"B64(4)\"}}]"));
    worker.evaluate(QStringLiteral(
        "chrome.action.setIcon({ imageData: { 128: image(128) } }, function(){ out.push('big ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message.slice(0, 26) : 'ok')); });"));
    for(int i = 0; i < 20 && Value(&worker, QStringLiteral("out.length")) != QStringLiteral("5"); i++){ QTest::qWait(10); Pump(&worker); }
    QTRY_COMPARE(Value(&worker, QStringLiteral("out[4]")), QStringLiteral("big Invalid value for argument"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false, error: 'No tab with id: 9.' });"
        "chrome.action.setBadgeText({ text: 'x', tabId: 9 }).then(function(){ out.push('set?'); }, function(e){ out.push('refused ' + e.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out[5]")), QStringLiteral("refused No tab with id: 9."));

    worker.evaluate(QStringLiteral(
        "btoa = function(s){ return new Array(17000).join('x'); };"
        "chrome.action.setIcon({ imageData: image(16) }).then(function(){ out.push('sent?'); }, function(e){ out.push('too large: ' + e.message.slice(-18)); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out[6]")), QStringLiteral("too large: too large to send."));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("5"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: 'top' });"
        "var handed = chrome.contextMenus.create({ id: 'top', title: 'Top' }, function(){ out.push('created ' + (chrome.runtime.lastError ? 'error' : 'ok')); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out[7]")), QStringLiteral("created ok"));
    QCOMPARE(Value(&worker, QStringLiteral("handed")), QStringLiteral("top"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(5))")),
             QStringLiteral("{\"api\":\"contextMenus.create\",\"args\":[{\"id\":\"top\",\"title\":\"Top\"}]}"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false, error: 'Cannot create item with duplicate id top' });"
        "chrome.contextMenus.create({ id: 'top', title: 'Top' }, function(){ out.push('again ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'ok')); });"));
    for(int i = 0; i < 20 && Value(&worker, QStringLiteral("out.length")) != QStringLiteral("9"); i++){ QTest::qWait(10); Pump(&worker); }
    QCOMPARE(Value(&worker, QStringLiteral("out[8]")), QStringLiteral("again Cannot create item with duplicate id top"));
    worker.evaluate(QStringLiteral("answers.push({ ok: false, error: 'nobody' }); var silent = chrome.contextMenus.create({ id: 'two', title: 'Two' });"));
    QCOMPARE(Value(&worker, QStringLiteral("silent")), QStringLiteral("two"));
    QVERIFY(Value(&worker, QStringLiteral("(function(){ try { chrome.contextMenus.create({ id: 'c', title: 'C', onclick: function(){} }); return 'made'; } catch (e) { return e.message; } })()"))
            .startsWith(QStringLiteral("Extensions using event pages or Service Workers cannot pass an onclick")));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("8"));
    worker.evaluate(QStringLiteral(
        "var clicks = [];"
        "chrome.contextMenus.onClicked.addListener(function(info, tab){ clicks.push(info.menuItemId + '@' + tab.id); });"
        "chrome.action.onClicked.addListener(function(tab){ clicks.push('button@' + tab.id); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("9"));
    QCOMPARE(Value(&worker, QStringLiteral("call(8).api")), QStringLiteral("vanilla.events"));
}

void tst_cdpshims::theOptionsPageIsOpenedAsATab(){
    const QString engines = QStringLiteral(
        "var enginesOpen = function(){ return Promise.reject(new Error('Could not create an options page.')); };"
        "chrome.runtime.openOptionsPage = enginesOpen;"
        "var manifest = { options_page: 'dashboard.html' };"
        "chrome.runtime.getManifest = function(){ return manifest; };");
    QJSEngine page;
    MakeKeyedPage(&page, Fetching() + engines);
    QVERIFY(Value(&page, QStringLiteral("String(chrome.runtime.openOptionsPage !== enginesOpen)")) == QStringLiteral("true"));
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: { id: 21, index: 2, active: true } });"
        "chrome.runtime.openOptionsPage().then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.create\",\"args\":[{\"url\":\"/dashboard.html\"}]}"));
    page.evaluate(QStringLiteral(
        "manifest = { options_ui: { page: '/pages/options.html' }, options_page: 'dashboard.html' };"
        "out = 'pending';"
        "answers.push({ ok: true, value: { id: 22, index: 3, active: true } });"
        "chrome.runtime.openOptionsPage(function(){ out = 'called ' + arguments.length + ' ' + !!chrome.runtime.lastError; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("called 0 false"));
    QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[1].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.create\",\"args\":[{\"url\":\"/pages/options.html\"}]}"));
    page.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: false, error: 'refused' });"
        "chrome.runtime.openOptionsPage().then(function(){ out = 'resolved'; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("rejected: refused"));
    page.evaluate(QStringLiteral(
        "manifest = {}; out = 'pending';"
        "chrome.runtime.openOptionsPage().then(function(){ out = 'resolved'; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("rejected: Could not create an options page."));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length")), QStringLiteral("3"));

    QJSEngine worker;
    MakeAsking(&worker, Fetching(), engines);
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: { id: 23, index: 4, active: true } });"
        "chrome.runtime.openOptionsPage().then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched.filter(function(f){ return /tabs.create/.test(decodeURIComponent(f.options.headers['X-Vanilla-Call'])); })[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.create\",\"args\":[{\"url\":\"/dashboard.html\"}]}"));

    QJSEngine open;
    MakePage(&open, Fetching() + engines);
    QCOMPARE(Value(&open, QStringLiteral("String(chrome.runtime.openOptionsPage === enginesOpen)")), QStringLiteral("true"));
}

void tst_cdpshims::theSidePanelIsTheApplications(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true });"
        "chrome.sidePanel.open({ windowId: 1 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"sidePanel.open\",\"args\":[{\"windowId\":1}]}"));
    EachIsAsked(&worker, QStringLiteral("sidePanel"),
                { QStringLiteral("setOptions({ path: 'side.html' })"), QStringLiteral("getOptions({})"),
                  QStringLiteral("setPanelBehavior({ openPanelOnActionClick: true })"), QStringLiteral("getPanelBehavior()"),
                  QStringLiteral("close({ windowId: 1 })") });
    worker.evaluate(QStringLiteral(
        "var told = [];"
        "answers.push('HELD');"
        "chrome.sidePanel.onOpened.addListener(function(info){ told.push('opened ' + info.path + ' ' + info.windowId); });"
        "chrome.sidePanel.onClosed.addListener(function(info){ told.push('closed ' + info.path); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("held.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, LastCall() + QStringLiteral(".indexOf('vanilla.events') > 0")),
             QStringLiteral("true"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'sidePanel.onOpened', args: [{ path: 'side.html', windowId: 1 }] },"
        "                                           { name: 'sidePanel.onClosed', args: [{ path: 'side.html', windowId: 1 }] }], order: [] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("told.join(' | ')")), QStringLiteral("opened side.html 1 | closed side.html"));
}

void tst_cdpshims::theNotificationsAreTheApplicationsOnQtAndTheEnginesOnEdge(){
    const QString engines = QStringLiteral(
        "var engineCalls = [];"
        "var enginesNotifications = { create: function(){ engineCalls.push('create'); return Promise.resolve('engine'); },"
        "                             onClicked: { addListener: function(){}, removeListener: function(){}, hasListener: function(){ return false; } },"
        "                             onClosed: { addListener: function(){ engineCalls.push('onClosed'); }, removeListener: function(){}, hasListener: function(){ return false; } } };"
        "chrome.notifications = enginesNotifications;");
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), engines);
        worker.evaluate(QStringLiteral(
            "var out = 'pending';"
            "answers.push({ ok: true, value: 'n1' });"
            "chrome.notifications.create('n1', { type: 'basic', iconUrl: 'i.png', title: 'T', message: 'M' })"
            "  .then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved n1"));
        QCOMPARE(Value(&worker, LastCall()),
                 QStringLiteral("{\"api\":\"notifications.create\",\"args\":[\"n1\",{\"type\":\"basic\",\"iconUrl\":\"i.png\",\"title\":\"T\",\"message\":\"M\"}]}"));
        EachIsAsked(&worker, QStringLiteral("notifications"),
                    { QStringLiteral("update('n1', { message: 'N' })"), QStringLiteral("clear('n1')"),
                      QStringLiteral("getAll()"), QStringLiteral("getPermissionLevel()") });
        worker.evaluate(QStringLiteral(
            "var told = [];"
            "answers.push('HELD');"
            "chrome.notifications.onClicked.addListener(function(id){ told.push('clicked ' + id); });"
            "chrome.notifications.onClosed.addListener(function(id, byUser){ told.push('closed ' + id + ' ' + byUser); });"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("held.length")), QStringLiteral("1"));
        worker.evaluate(QStringLiteral(
            "answers.push({ ok: true, value: { events: [{ name: 'notifications.onClicked', args: ['n1'] },"
            "                                           { name: 'notifications.onClosed', args: ['n1', true] }], order: [] } }, 'HELD');"
            "held.pop()();"));
        Settle(&worker);
        QTRY_COMPARE(Value(&worker, QStringLiteral("told.join(' | ')")), QStringLiteral("clicked n1 | closed n1 true"));
        QCOMPARE(Value(&worker, QStringLiteral("engineCalls.length")), QStringLiteral("0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, engines);
        worker.evaluate(QStringLiteral(
            "var got = null; chrome.notifications.create('x', {}).then(function(v){ got = v; });"
            "chrome.notifications.onClosed.addListener(function(){});"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("engine"));
        QCOMPARE(Value(&worker, QStringLiteral("engineCalls.join()")), QStringLiteral("create,onClosed"));
        QCOMPARE(Value(&worker, QStringLiteral("(chrome.notifications.onClicked === enginesNotifications.onClicked) + '/' + (chrome.notifications.onClosed === enginesNotifications.onClosed)")),
                 QStringLiteral("true/true"));
        QCOMPARE(Value(&worker, QStringLiteral("fetched.filter(function(f){ return /notifications/.test(decodeURIComponent(f.options.headers['X-Vanilla-Call'] || '')); }).length")),
                 QStringLiteral("0"));
    }
}

void tst_cdpshims::theVisibleTabIsCapturedByTheApplication(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: 'data:image/png;base64,AA' });"
        "chrome.tabs.captureVisibleTab(null, { format: 'png' }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved data:image/png;base64,AA"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.captureVisibleTab\",\"args\":[null,{\"format\":\"png\"}]}"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: false, error: \"Either the '<all_urls>' or 'activeTab' permission is required.\" });"
        "chrome.tabs.captureVisibleTab(function(v){ out = 'called ' + v + ' ' + (chrome.runtime.lastError && chrome.runtime.lastError.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), out)")),
                 QStringLiteral("called undefined Either the '<all_urls>' or 'activeTab' permission is required."));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.captureVisibleTab\",\"args\":[]}"));
    QJSEngine page;
    MakeKeyedPage(&page, Fetching());
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: 'data:image/jpeg;base64,BB' });"
        "chrome.tabs.captureVisibleTab().then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("resolved data:image/jpeg;base64,BB"));
}

void tst_cdpshims::theIdentityIsAChromeNobodySignedInto(){
    const QString engines = QStringLiteral(
        "var platform = 0;"
        "chrome.runtime.getPlatformInfo = function(callback){ platform++; if (callback) callback({ os: 'win' }); };"
        "var manifest = { permissions: ['identity'] };"
        "chrome.runtime.getManifest = function(){ return manifest; };"
        "var AbortSignal = function(){}; AbortSignal.timeout = function(ms){ return { ms: ms }; };");
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), engines);
    QCOMPARE(Value(&worker, QStringLiteral("[typeof chrome.identity, chrome.identity.getRedirectURL('/cb'), chrome.identity.getRedirectURL('//x'),"
                                           " chrome.identity.getRedirectURL()].join(' ')")),
             QStringLiteral("object https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/cb https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/x"
                            " https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/"));
    worker.evaluate(QStringLiteral(
        "var said = [];"
        "chrome.identity.getProfileUserInfo().then(function(v){ said.push('user ' + JSON.stringify(v)); });"
        "chrome.identity.getAuthToken({ interactive: true }).then(function(){ said.push('token'); }, function(e){ said.push('token: ' + e.message); });"
        "chrome.identity.removeCachedAuthToken({ token: 'x' }).then(function(v){ said.push('removed ' + v); });"
        "chrome.identity.clearAllCachedAuthTokens(function(){ said.push('cleared ' + !!chrome.runtime.lastError); });"
        "chrome.identity.getAuthToken({}, function(t){ said.push('token cb ' + t + ' ' + (chrome.runtime.lastError && chrome.runtime.lastError.message)); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), said.slice().sort().join(' | '))")),
                 QStringLiteral("cleared false | removed undefined | token cb undefined The user is not signed in. | token: The user is not signed in. | user {\"email\":\"\",\"id\":\"\"}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.filter(function(f){ return /identity/.test(decodeURIComponent(f.options.headers['X-Vanilla-Call'])); }).length")),
             QStringLiteral("0"));

    worker.evaluate(QStringLiteral(
        "timers = []; var out = 'pending', before = fetched.length;"
        "answers.push('HELD');"
        "chrome.identity.launchWebAuthFlow({ url: 'https://a.example/login', interactive: true, other: 1 })"
        "  .then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("held.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[before].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"identity.launchWebAuthFlow\",\"args\":[{\"url\":\"https://a.example/login\",\"interactive\":true}]}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[before].options.signal.ms")), QStringLiteral("1800000"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',')")), QStringLiteral("20000"));
    QCOMPARE(Value(&worker, QStringLiteral("runNextTimer() + ' ' + platform + ' ' + runNextTimer() + ' ' + platform")), QStringLiteral("20000 1 20000 2"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: 'https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/cb?code=1' }); held.shift()();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/cb?code=1"));
    QCOMPARE(Value(&worker, QStringLiteral("runNextTimer() + ' ' + platform + ' ' + timers.length")), QStringLiteral("20000 2 0"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: false, error: 'The user did not approve access.' });"
        "chrome.identity.launchWebAuthFlow({ url: 'https://a.example/login' }, function(v){"
        "  out = 'called ' + v + ' ' + (chrome.runtime.lastError && chrome.runtime.lastError.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), out)")), QStringLiteral("called undefined The user did not approve access."));
    worker.evaluate(QStringLiteral(
        "out = 'pending'; before = fetched.length;"
        "chrome.identity.launchWebAuthFlow({}).then(function(){ out = 'resolved'; }, function(e){ out = 'rejected ' + e.name; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("rejected TypeError"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - before + ' ' + timers.length")), QStringLiteral("0 0"));

    QJSEngine page;
    MakeKeyedPage(&page, Fetching() + engines);
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: 'https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/' });"
        "chrome.identity.launchWebAuthFlow({ url: 'https://a.example/login' }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("resolved https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/"));
    QCOMPARE(Value(&page, QStringLiteral("platform + ' ' + timers.filter(function(t){ return t.ms === 20000; }).length")), QStringLiteral("0 0"));
    QJSEngine open;
    MakePage(&open, Fetching() + engines);
    open.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.identity.launchWebAuthFlow({ url: 'https://a.example/login' }).then(function(){ out = 'resolved'; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("out")), QStringLiteral("rejected: chrome.identity.launchWebAuthFlow is not available in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("fetched.length + ' ' + chrome.identity.getRedirectURL('a')")),
             QStringLiteral("0 https://abcdefghijklmnopabcdefghijklmnop.chromiumapp.org/a"));
    QJSEngine plain;
    MakeAsking(&plain, Fetching(), QString(engines).replace(QStringLiteral("['identity']"), QStringLiteral("['storage']")));
    QCOMPARE(Value(&plain, QStringLiteral("typeof chrome.identity")), QStringLiteral("undefined"));
    QJSEngine edge;
    MakeEdgeWorker(&edge, engines + QStringLiteral("var enginesIdentity = { getRedirectURL: function(){ return 'the engine'; } }; chrome.identity = enginesIdentity;"));
    QCOMPARE(Value(&edge, QStringLiteral("chrome.identity.getRedirectURL('x')")), QStringLiteral("the engine"));
}

void tst_cdpshims::theEnginesAlarmsAreRungByTheWorker(){
    QJSEngine worker;
    Make(&worker, false, EnginesAlarms() + TheOtherRoot());
    QCOMPARE(Value(&worker, QStringLiteral("[chrome.alarms.onAlarm === enginesAlarms.onAlarm, browser.alarms === chrome.alarms,"
                                           " chrome.alarms.get === enginesAlarms.get || typeof chrome.alarms.get].join(' ')")),
             QStringLiteral("false true function"));
    worker.evaluate(QStringLiteral(
        "var said = [];"
        "var thrower = function(){ throw new Error('a listener which throws'); };"
        "chrome.alarms.onAlarm.addListener(thrower);"
        "chrome.alarms.onAlarm.addListener(hear);"
        "chrome.alarms.create('once', { when: clock + 3000 }).then(function(v){ said.push('created ' + v); });"
        "chrome.alarms.create('p', { periodInMinutes: 0.5 }, function(){ said.push('p ' + !!chrome.runtime.lastError); });"
        "chrome.alarms.create('bad', { when: 1 }, function(){ said.push('bad ' + (chrome.runtime.lastError && chrome.runtime.lastError.message)); });"
        "chrome.alarms.get('p').then(function(a){ said.push('get ' + a.name); });"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("said.join(' | ')")), QStringLiteral("p false | bad bad alarm | created undefined | get p"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',') + ' ' + alarmCalls.indexOf('the engine\\'s onAlarm')")),
             QStringLiteral("3000 -1"));
    worker.evaluate(QStringLiteral("clock += 3000; engineUses();"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("once@3000"));
    worker.evaluate(QStringLiteral("clock = 1030000; engineUses();"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("clock = 1060000;"));
    Pump(&worker); Pump(&worker);
    worker.evaluate(QStringLiteral("engineUses();"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("once@3000 p@30000/0.5 p@60000/0.5"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',')")), QStringLiteral("20000"));

    worker.evaluate(QStringLiteral(
        "rung = []; chrome.alarms.clear('p');"
        "chrome.alarms.create('r', { when: clock + 10000 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("clock += 10000; chrome.alarms.create('r', { when: clock + 10000 });"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral(""));
    worker.evaluate(QStringLiteral("clock += 10000; engineUses();"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("r@80000"));
    worker.evaluate(QStringLiteral(
        "rung = []; said = [];"
        "chrome.alarms.create('c', { when: clock + 5000 });"
        "chrome.alarms.create('d', { when: clock + 1000 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral(
        "chrome.alarms.clear('c').then(function(v){ said.push('c ' + v); });"
        "clock += 1000;"
        "chrome.alarms.clear('d', function(v){ said.push('d ' + v); });"
        "chrome.alarms.clear('none', function(v){ said.push('none ' + v); });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("clock += 4000; engineUses();"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + '/' + said.join(' | ')")), QStringLiteral("/d true | none false | c true"));
    worker.evaluate(QStringLiteral("chrome.alarms.create('keep', { periodInMinutes: 1440 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("engineAlarms.pg = { name: 'pg', scheduledTime: clock + 30000, periodInMinutes: 1 };"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',')")), QStringLiteral("20000"));
    worker.evaluate(QStringLiteral("delete engineAlarms.pg; clock += 30000;"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + ' ' + timers.length")), QStringLiteral(" 1"));
    worker.evaluate(QStringLiteral(
        "var clearing = function(a){ if (a.name === 'a1') chrome.alarms.clear('a2'); };"
        "chrome.alarms.onAlarm.addListener(clearing);"
        "chrome.alarms.create('a1', { when: clock + 1000 }); chrome.alarms.create('a2', { when: clock + 1000 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("clock += 1000;"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("a1@116000"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',')")), QStringLiteral("20000"));
    QCOMPARE(Value(&worker, QStringLiteral("var before = alarmCalls.length; runNextTimer(); alarmCalls.slice(before).join(',')")),
             QStringLiteral("getAll"));
    worker.evaluate(QStringLiteral(
        "[thrower, hear, clearing].forEach(function(f){ chrome.alarms.onAlarm.removeListener(f); });"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("chrome.alarms.onAlarm.hasListeners() + ' ' + timers.length + ' ' + Object.keys(engineAlarms).join(',')")),
             QStringLiteral("false 0 keep,a1"));

    QJSEngine thrown;
    Make(&thrown, false, EnginesAlarms());
    thrown.evaluate(QStringLiteral(
        "var threw = 'no';"
        "chrome.alarms.onAlarm.addListener(hear);"
        "try { chrome.alarms.create(42); } catch (e) { threw = e.name; }"
        "chrome.alarms.create({ when: clock + 1000 });"));
    Pump(&thrown);
    thrown.evaluate(QStringLiteral("clock += 1000; engineUses();"));
    Pump(&thrown);
    QCOMPARE(Value(&thrown, QStringLiteral("threw + ' ' + rung.join(' ') + ' ' + alarmCalls.filter(function(c){ return /^create/.test(c); }).join(',')")),
             QStringLiteral("TypeError @1000 create "));
    QJSEngine late;
    Make(&late, false, EnginesAlarms() + QStringLiteral("engineAlarms.old = { name: 'old', scheduledTime: clock - 5000 };"));
    late.evaluate(QStringLiteral("chrome.alarms.onAlarm.addListener(hear);"));
    Pump(&late);
    QCOMPARE(Value(&late, QStringLiteral("rung.join(' ')")), QStringLiteral("old@-5000"));

    QJSEngine edge;
    MakeEdgeWorker(&edge, EnginesAlarms());
    QCOMPARE(Value(&edge, QStringLiteral("chrome.alarms.onAlarm === enginesAlarms.onAlarm")), QStringLiteral("true"));
    QJSEngine page;
    MakeKeyedPage(&page, EnginesAlarms());
    QCOMPARE(Value(&page, QStringLiteral("chrome.alarms.onAlarm === enginesAlarms.onAlarm && chrome.alarms.create === enginesAlarms.create")),
             QStringLiteral("true"));
    QJSEngine none;
    Make(&none, false);
    none.evaluate(QStringLiteral("var out = 'pending'; chrome.alarms.create('x', {}).then(function(){ out = 'made'; }, function(e){ out = e.message; });"));
    Pump(&none);
    QCOMPARE(Value(&none, QStringLiteral("out")), QStringLiteral("chrome.alarms.create is not available in this browser"));
}

void tst_cdpshims::anAlarmClearedWhileTheEngineAnswersIsNotRung(){
    QJSEngine worker;
    Make(&worker, false, EnginesAlarms());
    worker.evaluate(QStringLiteral(
        "chrome.alarms.onAlarm.addListener(hear);"
        "chrome.alarms.create('h', { when: clock + 1000 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("holdLists = true; clock += 1000; runNextTimer();"));
    QCOMPARE(Value(&worker, QStringLiteral("heldLists.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("holdLists = false; chrome.alarms.clear('h'); heldLists.shift()();"));
    Pump(&worker); Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + '/' + JSON.stringify(engineAlarms)")), QStringLiteral("/{}"));

    worker.evaluate(QStringLiteral("chrome.alarms.create('k', { when: clock + 1000 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("holdClears = true; clock += 1000; chrome.alarms.clear('k');"));
    for(int i = 0; i < 8; i++){ worker.evaluate(QStringLiteral("runShort();")); QCoreApplication::processEvents(); }
    QCOMPARE(Value(&worker, QStringLiteral("heldClears.length + ' ' + rung.length")), QStringLiteral("1 0"));
    worker.evaluate(QStringLiteral("holdClears = false; heldClears.shift()();"));
    Pump(&worker); Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + '/' + JSON.stringify(engineAlarms)")), QStringLiteral("/{}"));
    worker.evaluate(QStringLiteral("chrome.alarms.create('m', { when: clock + 1000 });"));
    Pump(&worker);
    worker.evaluate(QStringLiteral("clock += 1000; engineUses();"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("m@3000"));
}

void tst_cdpshims::anAlarmUsedUpBeforeTheFirstLookIsRung(){
    QJSEngine worker;
    Make(&worker, false, EnginesAlarms());
    worker.evaluate(QStringLiteral(
        "chrome.alarms.onAlarm.addListener(hear);"
        "chrome.alarms.create('now', { when: clock }); engineUses();"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + ' ' + JSON.stringify(engineAlarms)")), QStringLiteral("now@0 {}"));
    worker.evaluate(QStringLiteral(
        "rung = []; var chained = function(a){ if (a.name === 'g') chrome.alarms.create('x', { when: clock + 100000 }); };"
        "chrome.alarms.onAlarm.addListener(chained);"
        "chrome.alarms.create('g', { when: clock }); engineUses();"));
    PumpShort(&worker); PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("chrome.alarms.onAlarm.removeListener(chained), chrome.alarms.clear('x'), rung.join(' ')")), QStringLiteral("g@0"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("rung = []; chrome.alarms.create('min', { when: clock });"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral(""));
    worker.evaluate(QStringLiteral("clock += 5000; engineUses();"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("min@5000"));

    worker.evaluate(QStringLiteral("rung = []; chrome.alarms.create('q', { periodInMinutes: 1 });"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("clock += 60000;"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("var threw = 'no'; try { chrome.alarms.create('q', 42); } catch (e) { threw = e.name; }"));
    PumpShort(&worker); PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("threw + ' ' + rung.join(' ')")), QStringLiteral("TypeError q@65000/1"));

    worker.evaluate(QStringLiteral("engineUses();"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("clock += 60000; failLists = true; timers = timers.filter(function(){ return false; }); chrome.alarms.onAlarm.removeListener(hear); chrome.alarms.onAlarm.addListener(hear);"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',') + ' ' + rung.length")), QStringLiteral("20000 1"));
    worker.evaluate(QStringLiteral("chrome.alarms.onAlarm.removeListener(hear);"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("0"));
}

void tst_cdpshims::aGuessIsNotPutInForWhatWasClearedOrGivenUpOn(){
    QJSEngine worker;
    Make(&worker, false, EnginesAlarms());
    worker.evaluate(QStringLiteral(
        "chrome.alarms.onAlarm.addListener(hear);"
        "holdCreateAnswers = true; chrome.alarms.create('a', { when: clock }); chrome.alarms.clear('a');"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("holdCreateAnswers = false; heldCreates.shift()();"));
    PumpShort(&worker); PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + ' ' + JSON.stringify(engineAlarms)")), QStringLiteral(" {}"));
    worker.evaluate(QStringLiteral("holdCreateAnswers = true; chrome.alarms.create('b', { when: clock }); chrome.alarms.clearAll();"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("holdCreateAnswers = false; heldCreates.shift()();"));
    PumpShort(&worker); PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ') + ' ' + JSON.stringify(engineAlarms)")), QStringLiteral(" {}"));

    worker.evaluate(QStringLiteral("chrome.alarms.create('later', { delayInMinutes: 5 }, function(){}); delete engineAlarms.later;"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral(""));

    worker.evaluate(QStringLiteral("holdCreates = true; chrome.alarms.create('n', { when: clock + 1000 });"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("giveUp();"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("timers.length + ' ' + JSON.stringify(engineAlarms)")), QStringLiteral("0 {}"));
    worker.evaluate(QStringLiteral("holdCreates = false; heldCreates.shift()();"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join(',')")), QStringLiteral("1000"));
    worker.evaluate(QStringLiteral("clock += 1000; engineUses();"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("n@1000"));
    worker.evaluate(QStringLiteral("rung = []; holdCreateAnswers = true; chrome.alarms.create('m', { when: clock + 1000 });"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("giveUp();"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("clock += 1000;"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("engineUses();"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("holdCreateAnswers = false; heldCreates.shift()();"));
    PumpShort(&worker); PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.join(' ')")), QStringLiteral("m@2000"));
    worker.evaluate(QStringLiteral("rung = []; holdCreateAnswers = true; chrome.alarms.create('d', { delayInMinutes: 0.1 });"
                                   "clock += 6000; engineUses();"));
    PumpShort(&worker);
    worker.evaluate(QStringLiteral("holdCreateAnswers = false; heldCreates.shift()();"));
    PumpShort(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("rung.length + ' ' + /^d@/.test(rung[0])")), QStringLiteral("1 true"));
}

void tst_cdpshims::theUserScriptsAreTheApplications(){
    const QString engines = QStringLiteral(
        "var manifest = { permissions: ['userScripts', 'storage'] };"
        "chrome.runtime.getManifest = function(){ return manifest; };");
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), engines);
    worker.evaluate(QStringLiteral(
        "var said = [];"
        "answers.push({ ok: true });"
        "chrome.userScripts.register([{ id: 'a', matches: ['*://*/*'], js: [{ code: 'x()' }] }])"
        "  .then(function(v){ said.push('registered ' + v); }, function(e){ said.push('refused ' + e.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), said.join(' | '))")), QStringLiteral("registered undefined"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"userScripts.register\",\"args\":[]}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[fetched.length - 1].options.body")),
             QStringLiteral("[[{\"id\":\"a\",\"matches\":[\"*://*/*\"],\"js\":[{\"code\":\"x()\"}]}]]"));
    worker.evaluate(QStringLiteral(
        "said = [];"
        "answers.push({ ok: true, value: [{ id: 'a' }] });"
        "chrome.userScripts.getScripts({ ids: ['a'] }, function(v){ said.push('got ' + JSON.stringify(v) + ' ' + !!chrome.runtime.lastError); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), said.join(' | '))")), QStringLiteral("got [{\"id\":\"a\"}] false"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"userScripts.getScripts\",\"args\":[{\"ids\":[\"a\"]}]}"));
    worker.evaluate(QStringLiteral(
        "said = [];"
        "answers.push({ ok: false, error: \"Nonexistent script ID 'z'\" });"
        "chrome.userScripts.unregister({ ids: ['z'] }).then(function(){ said.push('gone'); }, function(e){ said.push('refused ' + e.message); });"
        "chrome.userScripts.execute({ js: [{ code: '1' }], target: { tabId: 1 } }).then(function(){ said.push('ran'); }, function(e){ said.push(e.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), said.slice().sort().join(' | '))")),
                 QStringLiteral("chrome.userScripts.execute is not available in this browser | refused Nonexistent script ID 'z'"));
    worker.evaluate(QStringLiteral(
        "said = [];"
        "answers.push({ ok: true });"
        "chrome.userScripts.update([{ id: 'a', excludeMatches: ['*://x.example/*'] }]).then(function(){ said.push('updated'); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("(runTimers(), said.join(' | '))")), QStringLiteral("updated"));
    QCOMPARE(Value(&worker, LastCall() + QStringLiteral(" + ' ' + fetched[fetched.length - 1].options.body")),
             QStringLiteral("{\"api\":\"userScripts.update\",\"args\":[]} [[{\"id\":\"a\",\"excludeMatches\":[\"*://x.example/*\"]}]]"));
    QCOMPARE(Value(&worker, QStringLiteral("(function(){ try { chrome.userScripts.getScripts().catch(function(){}); return true; } catch (e) { return false; } })()")),
             QStringLiteral("true"));

    QJSEngine page;
    MakeKeyedPage(&page, Fetching() + engines);
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [] });"
        "chrome.userScripts.getScripts().then(function(v){ out = 'got ' + JSON.stringify(v); }, function(e){ out = 'refused ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("got []"));

    QJSEngine plain;
    MakeAsking(&plain, Fetching(), QString(engines).replace(QStringLiteral("'userScripts', "), QString()));
    QCOMPARE(Value(&plain, QStringLiteral("typeof chrome.userScripts")), QStringLiteral("undefined"));
    QJSEngine open;
    MakePage(&open, Fetching() + engines);
    QCOMPARE(Value(&open, QStringLiteral("typeof chrome.userScripts")), QStringLiteral("undefined"));
    QJSEngine edge;
    MakeEdgeWorker(&edge, engines);
    QCOMPARE(Value(&edge, QStringLiteral("typeof chrome.userScripts")), QStringLiteral("undefined"));
}

void tst_cdpshims::aUserScriptsMessageIsAnsweredOnce(){
    const QString engines = QStringLiteral(
        "var engineHeard = 0;"
        "var manifest = { permissions: ['userScripts'] };"
        "chrome.runtime.getManifest = function(){ return manifest; };"
        "chrome.runtime.onUserScriptMessage = { addListener: function(){ engineHeard++; }, removeListener: function(){},"
        "                                       hasListener: function(){ return false; }, hasListeners: function(){ return false; } };"
        "function replies(){ return fetched.filter(function(f){"
        "  return decodeURIComponent(f.options.headers['X-Vanilla-Call']).indexOf('vanilla.userScriptReply') > 0; })"
        "  .map(function(f){ return f.options.body; }); }");
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), engines + Calls() + Saying());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "answers.push('HELD', { ok: true });"
        "var first = function(m, s, respond){ got.push(JSON.stringify(m) + ' ' + s.tab.id + ' ' + s.frameId); if (m.n === 1) respond('answer 1'); };"
        "chrome.runtime.onUserScriptMessage.addListener(first);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("call(0).api + ' ' + call(1).api + ' ' + (call(1).args[0] === call(0).args[0]) + ' ' + engineHeard")),
             QStringLiteral("vanilla.events vanilla.userScriptListen true 0"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.runtime.onUserScriptMessage.hasListener(first) + ' ' + chrome.runtime.onUserScriptMessage.hasListeners()")),
             QStringLiteral("true true"));
    const QString token = Value(&worker, QStringLiteral("call(0).args[0]"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 1 }, { id: 'x', tab: { id: 5, index: 0 }, frameId: 0 }, 't1'] },"
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 2 }, { id: 'x', tab: { id: 5, index: 0 }, frameId: 7 }, 't2'] }] } },"
        "  { ok: true }, { ok: true }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got.join(' | ')")), QStringLiteral("{\"n\":1} 5 0 | {\"n\":2} 5 7"));
    QCOMPARE(Value(&worker, QStringLiteral("replies().join(' | ')")),
             QStringLiteral("[\"t1\",\"%1\",\"value\",\"answer 1\"] | [\"t2\",\"%1\",\"none\"]").arg(token));

    worker.evaluate(QStringLiteral(
        "var later = null;"
        "chrome.runtime.onUserScriptMessage.addListener(function(m, s, respond){ if (m.n === 9) respond(); });"
        "chrome.runtime.onUserScriptMessage.addListener(function(m, s, respond){ if (m.n === 3) { later = respond; return true; } });"
        "chrome.runtime.onUserScriptMessage.addListener(function(m){ if (m.n === 4) return Promise.resolve('promised'); });"
        "chrome.runtime.onUserScriptMessage.removeListener(first);"
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 9 }, { id: 'x' }, 't9'] },"
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 3 }, { id: 'x' }, 't3'] },"
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 4 }, { id: 'x' }, 't4'] }] } },"
        "  { ok: true }, 'HELD', { ok: true });"
        "held.pop()();"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("answers.push({ ok: true }); later('late'); later('again');"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("replies().slice(2).join(' | ')")),
             QStringLiteral("[\"t9\",\"%1\",\"value\"] | [\"t4\",\"%1\",\"value\",\"promised\"] | [\"t3\",\"%1\",\"value\",\"late\"]").arg(token));

    QJSEngine none;
    MakeAsking(&none, Fetching(), engines + Calls() + Saying());
    none.evaluate(QStringLiteral(
        "answers.push('HELD', { ok: true });"
        "var only = function(){}; chrome.runtime.onUserScriptMessage.addListener(only);"
        "chrome.tabs.onCreated.addListener(function(){});"));
    Settle(&none);
    none.evaluate(QStringLiteral(
        "chrome.runtime.onUserScriptMessage.removeListener(only);"
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 5 }, { id: 'x' }, 't5'] },"
        "  { name: 'runtime.onUserScriptMessage', args: [{ n: 6 }, { id: 'x' }, 6] }] } },"
        "  { ok: true }, 'HELD');"
        "held.pop()();"));
    Settle(&none);
    QCOMPARE(Value(&none, QStringLiteral("replies().length + ' ' + /\"t5\",\"[0-9a-f]{32}\",\"nolistener\"/.test(replies()[0])")), QStringLiteral("1 true"));

    QJSEngine plain;
    MakeAsking(&plain, Fetching(), QString(engines).replace(QStringLiteral("['userScripts']"), QStringLiteral("['storage']")));
    plain.evaluate(QStringLiteral("chrome.runtime.onUserScriptMessage.addListener(function(){});"));
    QCOMPARE(Value(&plain, QStringLiteral("engineHeard + ' ' + fetched.length")), QStringLiteral("1 0"));
    QJSEngine edge;
    MakeEdgeWorker(&edge, engines);
    edge.evaluate(QStringLiteral("chrome.runtime.onUserScriptMessage.addListener(function(){});"));
    QCOMPARE(Value(&edge, QStringLiteral("engineHeard")), QStringLiteral("1"));
}

void tst_cdpshims::theUserScriptPreludeSendsWithItsSecretAlone(){
    const QByteArray secret(64, 'a');
    const QString prelude = Cdp::UserScriptPrelude(QStringLiteral("abcdefghijklmnopabcdefghijklmnop"), secret);
    QVERIFY(prelude.contains(QString::fromLatin1(secret)));
    QJSEngine world;
    world.evaluate(World());
    world.evaluate(QStringLiteral("var location = { href: 'https://a.example/page' }; self.top = self; var chrome = {};") + Fetching()
                   + QStringLiteral("var plainFetch = fetch; fetch = function(url, options){"
                                    "  void options.signal; void options.mode; void options.headers.anything; return plainFetch(url, options); };"));
    QVERIFY2(!world.evaluate(prelude).isError(), qPrintable(world.evaluate(prelude).toString()));
    world.evaluate(QStringLiteral("var firstRuntime = chrome.runtime;"));
    world.evaluate(prelude);
    QCOMPARE(Value(&world, QStringLiteral("(chrome.runtime === firstRuntime) + ' ' + chrome.runtime.id + ' ' + typeof chrome.runtime.connect")),
             QStringLiteral("true abcdefghijklmnopabcdefghijklmnop undefined"));

    world.evaluate(QStringLiteral(
        "var leaked = [];"
        "['signal', 'mode', 'anything'].forEach(function(k){"
        "  Object.defineProperty(Object.prototype, k, { configurable: true, get: function(){ leaked.push(JSON.stringify(this)); } }); });"
        "Object.assign = function(){ leaked.push('assign'); return {}; };"
        "Object.create = function(){ leaked.push('create'); return {}; };"));
    world.evaluate(QStringLiteral(
        "var out = [];"
        "answers.push({ ok: true, value: { back: 1 } }, { ok: true, none: true }, { ok: false, error: 'Could not establish connection. Receiving end does not exist.' },"
        "             { ok: true, none: true });"
        "chrome.runtime.sendMessage({ q: 1 }).then(function(v){ out.push('value ' + JSON.stringify(v)); });"
        "chrome.runtime.sendMessage({ q: 2 }).then(function(v){ out.push('none ' + v); });"
        "chrome.runtime.sendMessage({ q: 3 }).then(function(){ out.push('no'); }, function(e){ out.push('failed ' + e.message); });"
        "chrome.runtime.sendMessage('abcdefghijklmnopabcdefghijklmnop', { q: 4 }, function(v){"
        "  out.push('callback ' + v + ' ' + (chrome.runtime.lastError && chrome.runtime.lastError.message)); });"
        "chrome.runtime.sendMessage('another', { q: 5 }).then(function(){ out.push('no'); }, function(e){ out.push('other ' + e.message); });"));
    QTRY_COMPARE(Value(&world, QStringLiteral("(runTimers(), out.slice().sort().join(' | '))")),
                 QStringLiteral("callback undefined The message port closed before a response was received. | failed Could not establish connection. Receiving end does not exist."
                                " | none undefined | other Could not establish connection. Receiving end does not exist. | value {\"back\":1}"));
    QCOMPARE(Value(&world, QStringLiteral("chrome.runtime.lastError")), QStringLiteral("undefined"));
    QCOMPARE(Value(&world, QStringLiteral("leaked.length")), QStringLiteral("0"));
    QCOMPARE(Value(&world, QStringLiteral("fetched.length + ' ' + fetched[0].url + ' ' + fetched[0].options.method")),
             QStringLiteral("4 vanilla-extension://host/userScriptMessage POST"));
    QCOMPARE(Value(&world, QStringLiteral("fetched[0].options.headers['X-Vanilla-World']")), QString::fromLatin1(secret));
    QCOMPARE(Value(&world, QStringLiteral("fetched[0].options.body")), QStringLiteral("{\"message\":{\"q\":1},\"url\":\"https://a.example/page\",\"frame\":0}"));
    QCOMPARE(Value(&world, QStringLiteral("fetched.some(function(f){ return String(f.url).indexOf('aaaa') >= 0 || String(f.options.body).indexOf('aaaa') >= 0; })")),
             QStringLiteral("false"));
    world.evaluate(QStringLiteral(
        "var cyclic = {}; cyclic.self = cyclic; var said = 'pending';"
        "chrome.runtime.sendMessage(cyclic).then(function(){ said = 'sent'; }, function(e){ said = e.message; });"));
    QTRY_COMPARE(Value(&world, QStringLiteral("said")), QStringLiteral("Could not serialize message."));
    QCOMPARE(Value(&world, QStringLiteral("fetched.length")), QStringLiteral("4"));
}

void tst_cdpshims::theWordWhichWakesTheWorkerReachesNoListener(){
    QCOMPARE(Cdp::WakeScript(), QStringLiteral("try { chrome.runtime.sendMessage({ __vanillaWake: 1 }).catch(() => {}); } catch (e) {}\n"));
    QJSEngine page;
    page.evaluate(QStringLiteral("var sent = []; var chrome = { runtime: { sendMessage: function(m){ sent.push(JSON.stringify(m)); return Promise.reject(new Error('closed')); } } };"));
    QVERIFY(!page.evaluate(Cdp::WakeScript()).isError());
    QCOMPARE(Value(&page, QStringLiteral("sent.join()")), QStringLiteral("{\"__vanillaWake\":1}"));

    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var heard = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ heard.push(JSON.stringify(m) + ' ' + s.url); });"
        "var wakeUrl = 'chrome-extension://' + chrome.runtime.id + '/vanilla_wake.html';"
        "function send(m, s){ listeners.forEach(function(f){ f(m, s, function(){}); }); }"
        "send({ __vanillaWake: 1 }, { id: chrome.runtime.id, url: wakeUrl, origin: 'chrome-extension://' + chrome.runtime.id });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral(
        "send({ __vanillaWake: 1 }, { id: chrome.runtime.id, url: 'chrome-extension://' + chrome.runtime.id + '/popup.html' });"
        "send({ __vanillaWake: 1 }, { id: chrome.runtime.id, url: wakeUrl, tab: { id: 3 } });"
        "send({ __vanillaWake: 1 }, { id: 'another', url: 'chrome-extension://another/vanilla_wake.html' });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.length")), QStringLiteral("3"));
}

void tst_cdpshims::aContentScriptTellsItsOwnWorkerItsNonce(){
    QJSEngine content;
    MakeEdgeContent(&content, QStringLiteral("answers.push('HELD');"));
    content.evaluate(QStringLiteral(
        "var heard = 0;"
        "var got = 'none', mine = { id: chrome.runtime.id };"
        "function ask(m, sender){ return String(listeners[0](m, sender, function(r){ got = JSON.stringify(r); })); }"));
    QCOMPARE(Value(&content, QStringLiteral("listeners.length")), QStringLiteral("1"));
    QCOMPARE(Value(&content, QStringLiteral("ask({ a: 1 }, mine) + ask({ __vanillaWho: 1 }, { id: 'another' })"
                                            " + ask({ __vanillaWho: 1 }, { id: chrome.runtime.id, tab: { id: 3 } })")),
             QStringLiteral("falsefalsefalse"));
    QCOMPARE(Value(&content, QStringLiteral("fetched.length")), QStringLiteral("0"));
    QCOMPARE(Value(&content, QStringLiteral("ask({ __vanillaWho: 1 }, mine)")), QStringLiteral("true"));
    QTRY_COMPARE(Value(&content, QStringLiteral("held.length")), QStringLiteral("1"));
    QCOMPARE(Value(&content, QStringLiteral("fetched[0].url + ' ' + got")), QStringLiteral("vanilla-extension://host/bind none"));
    content.evaluate(QStringLiteral("held[0]();"));
    QTRY_COMPARE(Value(&content, QStringLiteral("got")),
                 QStringLiteral("{\"nonce\":\"") + Value(&content, QStringLiteral("fetched[0].options.headers['X-Vanilla-Nonce']")) + QStringLiteral("\"}"));
    QVERIFY(Value(&content, QStringLiteral("got")).contains(QRegularExpression(QStringLiteral("\"[0-9a-f]{32}\""))));
    content.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ heard++; });"
                                    "got = 'none'; ask({ __vanillaWho: 1 }, mine);"));
    QTRY_VERIFY(Value(&content, QStringLiteral("got")) != QStringLiteral("none"));
    QCOMPARE(Value(&content, QStringLiteral("fetched.filter(function(f){ return /bind$/.test(f.url); }).length + '/' + heard")),
             QStringLiteral("1/0"));

    QJSEngine late;
    MakeEdgeContent(&late, QStringLiteral("answers.push('HELD');"));
    late.evaluate(QStringLiteral("var got = 'none'; listeners[0]({ __vanillaWho: 1 }, { id: chrome.runtime.id }, function(r){ got = r.nonce; });"));
    QTRY_COMPARE(Value(&late, QStringLiteral("held.length")), QStringLiteral("1"));
    QVERIFY(Value(&late, QStringLiteral("timers.map(function(t){ return t.ms; }).join()")).contains(QStringLiteral("800")));
    late.evaluate(QStringLiteral("timers.filter(function(t){ return t.ms === 800; }).forEach(function(t){ t.f(); });"));
    QCOMPARE(Value(&late, QStringLiteral("got.length")), QStringLiteral("32"));

    QJSEngine child;
    MakeEdgeContent(&child, QStringLiteral("window.top = {};"));
    QCOMPARE(Value(&child, QStringLiteral("String(listeners[0]({ __vanillaWho: 1 }, { id: chrome.runtime.id }, function(){}))")),
             QStringLiteral("false"));
}

void tst_cdpshims::aTabWithNoLinkIsFoundByAskingItsDocument(){
    const QString engines = QStringLiteral(
        "var engineList = [{ id: 945650010, url: 'https://b.example/x#top' }, { id: 945650011, url: 'https://b.example/x' },"
        "                  { id: 945650012, url: 'https://c.example/' }, { id: 945650013 }];"
        "var whoAsked = [], whoAnswers = { 945650010: { nonce: 'b1000000000000000000000000000000' },"
        "                                  945650011: { nonce: 'b2000000000000000000000000000000' } };"
        "chrome.tabs.query = function(){ return Promise.resolve(engineList); };"
        "var engineSendNever = 0;"
        "chrome.tabs.sendMessage = function(id, m, o){ whoAsked.push(id + JSON.stringify(m) + JSON.stringify(o));"
        "  if (id === engineSendNever) return new Promise(function(){});"
        "  return id in whoAnswers ? Promise.resolve(whoAnswers[id]) : Promise.reject(new Error('Receiving end does not exist.')); };");
    QJSEngine worker;
    MakeEdgeWorker(&worker, Scripting() + engines);
    worker.evaluate(ScriptingTabs() + QStringLiteral(
        "function where(url){ asked.forEach(function(a, i){ if (!a.done && a.call.api === 'tabs.get') { a.done = 1;"
        "  answer(i, url ? { ok: true, value: { id: a.call.args[0], index: 1, url: url } } : { ok: false, error: 'No tab with id: ' + a.call.args[0] + '.' }); } }); }"
        "function tabOfs(){ return asked.filter(function(a){ return a.call.api === 'vanilla.tabOf'; }).length; }"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ target: { tabId: 140 }, files: ['a.js'] }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("where('https://b.example/x');"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("tabOf('b1000000000000000000000000000000', 141);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("tabOf('b2000000000000000000000000000000', 140);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved [{\"frameId\":0,\"documentId\":\"D\",\"result\":\"engine\"}]"));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting()")),
             QStringLiteral("executeScript[{\"target\":{\"tabId\":945650011},\"files\":[\"a.js\"]}]"));
    QCOMPARE(Value(&worker, QStringLiteral("whoAsked.join(' ')")),
             QStringLiteral("945650010{\"__vanillaWho\":1}{\"frameId\":0} 945650011{\"__vanillaWho\":1}{\"frameId\":0}"));

    worker.evaluate(QStringLiteral("whoAsked = []; outcome(chrome.scripting.insertCSS({ target: { tabId: 140 }, css: 'a{}' }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("asked.filter(function(a){ return a.call.api === 'tabs.get' && !a.done; }).length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("where(null);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 140."));
    QCOMPARE(Value(&worker, QStringLiteral("whoAsked.length + '/' + scriptingCalls.length")), QStringLiteral("0/1"));

    const int tabOfs = Value(&worker, QStringLiteral("tabOfs()")).toInt();
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ target: { tabId: 150 }, files: ['a.js'] }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("where('https://d.example/');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 150."));
    worker.evaluate(QStringLiteral("whoAnswers[945650012] = { nonce: 'NOT A NONCE' };"
                                   "outcome(chrome.scripting.executeScript({ target: { tabId: 150 }, files: ['a.js'] }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("where('https://c.example/');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 150."));
    QCOMPARE(Value(&worker, QStringLiteral("tabOfs() + '/' + whoAsked.join(' ')")),
             QString::number(tabOfs) + QStringLiteral("/945650012{\"__vanillaWho\":1}{\"frameId\":0}"));

    worker.evaluate(QStringLiteral("whoAsked = [];"
                                   "engineList = [{ id: 945650020, url: 'https://f.example/' }, { id: 945650021, url: 'https://f.example/' }];"
                                   "whoAnswers[945650021] = { nonce: 'b3000000000000000000000000000000' };"
                                   "engineSendNever = 945650020;"
                                   "outcome(chrome.scripting.executeScript({ target: { tabId: 170 }, files: ['a.js'] }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("where('https://f.example/');"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("whoAsked.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("String(runAt(1000) > 0)")), QStringLiteral("true"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("tabOf('b3000000000000000000000000000000', 170);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.slice(0, 8)")), QStringLiteral("resolved"));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting()")), QStringLiteral("executeScript[{\"target\":{\"tabId\":945650021},\"files\":[\"a.js\"]}]"));

    worker.evaluate(QStringLiteral("whoAsked = []; engineList = [1, 2, 3, 4, 5].map(function(n){ return { id: 945650100 + n, url: 'https://e.example/' }; });"
                                   "outcome(chrome.scripting.executeScript({ target: { tabId: 160 }, files: ['a.js'] }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("where('https://e.example/');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 160."));
    QCOMPARE(Value(&worker, QStringLiteral("whoAsked.length + '/' + scriptingCalls.length")), QStringLiteral("0/2"));
}

void tst_cdpshims::anEdgePageHasItsScriptingRunByTheWorker(){
    QJSEngine page;
    MakeEdgePage(&page, Scripting());
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "function outcome(p){ out = 'pending'; p.then(function(r){ out = 'ran ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; }); }"
        "replies.push({ ok: true, value: [{ frameId: 0 }] });"
        "outcome(chrome.scripting.executeScript({ files: ['/js/scripting/picker.js'], target: { tabId: 5 } }));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("ran [{\"frameId\":0}]"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(sent[0][0])")),
             QStringLiteral("{\"__vanillaRelay\":1,\"api\":\"scripting.executeScript\",\"args\":[{\"files\":[\"/js/scripting/picker.js\"],\"target\":{\"tabId\":5}}]}"));
    page.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([]));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("ran \"the engine's own\""));
    QCOMPARE(Value(&page, QStringLiteral("scriptingCalls.map(function(c){ return c.name; }).join()")), QStringLiteral("registerContentScripts"));
}

void tst_cdpshims::anEdgePageRunsAFuncInTheTabItLooksUp(){
    const QString engines = QStringLiteral(
        "var whoAsked = [];"
        "chrome.tabs.query = function(){ return Promise.resolve([{ id: 945650030, url: 'https://g.example/' }, { id: 945650031, url: 'https://h.example/' }]); };"
        "chrome.tabs.sendMessage = function(id, m, o){ whoAsked.push(id + JSON.stringify(m) + JSON.stringify(o));"
        "  return Promise.resolve(id === 945650030 ? { nonce: 'c1000000000000000000000000000000' } : undefined); };");
    QJSEngine page;
    MakeEdgePage(&page, Scripting() + engines);
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "function outcome(p){ out = 'pending'; p.then(function(r){ out = 'ran ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; }); }"
        "function asked(i){ return decodeURIComponent(fetched[i].options.headers['X-Vanilla-Call']); }"
        "var f = function(a){ return a; };"
        "var injection = { target: { tabId: 180 }, func: f, args: [1] }, was = JSON.stringify(injection);"
        "answers.push({ ok: true, value: { id: 180, index: 0, url: 'https://g.example/#x' } }, { ok: true, value: { id: 180, index: 0 } });"
        "outcome(chrome.scripting.executeScript(injection));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("ran [{\"frameId\":0,\"documentId\":\"D\",\"result\":\"engine\"}]"));
    QCOMPARE(Value(&page, QStringLiteral("scriptingCalls.length + '/' + scriptingCalls[0].args[0].target.tabId + '/' + (scriptingCalls[0].args[0].func === f)"
                                         " + '/' + JSON.stringify(scriptingCalls[0].args[0].args) + '/' + (JSON.stringify(injection) === was)")),
             QStringLiteral("1/945650030/true/[1]/true"));
    QCOMPARE(Value(&page, QStringLiteral("asked(0) + ' ' + asked(1)")),
             QStringLiteral("{\"api\":\"tabs.get\",\"args\":[180]} {\"api\":\"vanilla.tabOf\",\"args\":[\"c1000000000000000000000000000000\"]}"));
    QCOMPARE(Value(&page, QStringLiteral("whoAsked.join(' ') + '/' + sent.length")),
             QStringLiteral("945650030{\"__vanillaWho\":1}{\"frameId\":0}/0"));

    page.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 180, index: 0, url: 'https://g.example/' } }, { ok: true, value: { id: 181, index: 1 } });"
        "outcome(chrome.scripting.executeScript({ target: { tabId: 180 }, func: f }));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("failed No tab with id: 180."));
    QCOMPARE(Value(&page, QStringLiteral("scriptingCalls.length")), QStringLiteral("1"));

    page.evaluate(QStringLiteral(
        "var called = 'no';"
        "chrome.scripting.executeScript({ target: { documentIds: ['X'] }, func: f }, function(r){ called = JSON.stringify(r); });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("called")), QStringLiteral("[{\"frameId\":0,\"documentId\":\"D\",\"result\":\"engine\"}]"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(scriptingCalls[1].args[0].target)")), QStringLiteral("{\"documentIds\":[\"X\"]}"));

    page.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 180, index: 0, url: 'https://g.example/' } }, { ok: true, value: { id: 180, index: 0 } });"
        "var moved = { target: { tabId: 180, frameIds: [0] }, func: f };"
        "outcome(chrome.scripting.executeScript(moved));"
        "moved.target = { tabId: 180, allFrames: true };"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("ran [{\"frameId\":0,\"documentId\":\"D\",\"result\":\"engine\"}]"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(scriptingCalls[2].args[0].target)")), QStringLiteral("{\"tabId\":945650030,\"frameIds\":[0]}"));

    QJSEngine open;
    open.evaluate(World());
    open.evaluate(AtAPage());
    open.evaluate(Fetching() + HostFetches() + EdgeBrands() + EdgesOwn());
    QVERIFY(!open.evaluate(Scripting() + engines).isError());
    QVERIFY(!open.evaluate(Cdp::PageShim()).isError());
    open.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.scripting.executeScript({ target: { tabId: 180 }, func: function(){} }).then(function(){ out = 'ran'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("out")),
                 QStringLiteral("chrome.scripting.executeScript: 'func' is not available from an extension's page in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("fetched.length + '/' + whoAsked.length + '/' + scriptingCalls.length")), QStringLiteral("0/0/0"));
}

void tst_cdpshims::theOffscreenDocumentIsAmongTheContexts(){
    {
        QJSEngine bare;
        MakeAsking(&bare, Fetching(), QStringLiteral(
            "chrome.runtime.getManifest = function(){ return { permissions: ['storage'] }; };"
            "var enginesGetContexts = function(f){ return Promise.resolve([]); };"
            "chrome.runtime.getContexts = enginesGetContexts;"));
        bare.evaluate(Calls());
        QCOMPARE(Value(&bare, QStringLiteral("chrome.runtime.getContexts === enginesGetContexts")), QStringLiteral("true"));
        bare.evaluate(QStringLiteral(
            "var out = 'pending';"
            "chrome.offscreen.hasDocument().then(function(){ out = 'asked'; }, function(e){ out = 'refused'; });"));
        QTRY_COMPARE(Value(&bare, QStringLiteral("out")), QStringLiteral("refused"));
        QCOMPARE(Value(&bare, QStringLiteral("fetched.length")), QStringLiteral("0"));
    }
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), QStringLiteral(
        "chrome.runtime.getManifest = function(){ return { permissions: ['offscreen'] }; };"
        "var enginesContexts = [{ contextType: 'TAB', contextId: 'c1', documentId: 'd1', documentUrl: 'https://a.example/', frameId: 0, tabId: 4, windowId: 1, incognito: false }];"
        "chrome.runtime.getContexts = function(f){ return Promise.resolve(enginesContexts); };"));
    worker.evaluate(Calls());
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.offscreen.createDocument + '/' + typeof chrome.runtime.getContexts")), QStringLiteral("function/function"));
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [] });"
        "chrome.runtime.getContexts({ contextTypes: ['OFFSCREEN_DOCUMENT'] }).then(function(v){ out = JSON.stringify(v); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[]"));
    QCOMPARE(Value(&worker, QStringLiteral("call(0).api")), QStringLiteral("offscreen.contexts"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true });"
        "chrome.offscreen.createDocument({ url: 'lib/off.html', reasons: ['BLOBS'], justification: 'x' }).then(function(){ out = 'made'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("made"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(1))")),
             QStringLiteral("{\"api\":\"offscreen.createDocument\",\"args\":[{\"url\":\"lib/off.html\",\"reasons\":[\"BLOBS\"],\"justification\":\"x\"}]}"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: [{ documentUrl: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html', documentId: 'dd', contextId: 'cc' }] });"
        "chrome.runtime.getContexts({ contextTypes: ['OFFSCREEN_DOCUMENT'], documentUrls: ['chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html'] }).then(function(v){ out = JSON.stringify(v); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")),
                 QStringLiteral("[{\"contextType\":\"OFFSCREEN_DOCUMENT\",\"contextId\":\"cc\",\"documentId\":\"dd\",\"documentUrl\":\"chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html\","
                                "\"documentOrigin\":\"chrome-extension://abcdefghijklmnopabcdefghijklmnop\",\"frameId\":0,\"tabId\":-1,\"windowId\":-1,\"incognito\":false}]"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: [{ documentUrl: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html', documentId: 'dd', contextId: 'cc' }] });"
        "chrome.runtime.getContexts({}).then(function(v){ out = v.map(function(c){ return c.contextType; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("TAB,OFFSCREEN_DOCUMENT"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: [{ documentUrl: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html', documentId: 'dd', contextId: 'cc' }] });"
        "chrome.runtime.getContexts({ contextTypes: ['TAB'], tabIds: [4] }, function(v){ out = 'cb ' + v.map(function(c){ return c.contextType; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("cb TAB"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: [{ documentUrl: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/lib/off.html', documentId: 'dd', contextId: 'cc' }] });"
        "chrome.runtime.getContexts({ documentUrls: ['chrome-extension://abcdefghijklmnopabcdefghijklmnop/other.html'] }).then(function(v){ out = 'n=' + v.length; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("n=0"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: false, error: 'Only a single offscreen document may be created.' });"
        "chrome.offscreen.createDocument({ url: 'lib/off.html' }).then(function(){ out = 'made?'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Only a single offscreen document may be created."));
    worker.evaluate(QStringLiteral(
        "var bad = [], askedBefore = fetched.length;"
        "['x', ['TAB'], { contextTypes: 'TAB' }, { incognito: 'no' }, { tabIds: 4 }, { bogus: 1 }].forEach(function(f){"
        "  chrome.runtime.getContexts(f).then(function(){ bad.push('ok?'); }, function(e){ bad.push(e.message.slice(0, 38)); }); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("bad.length")), QStringLiteral("6"));
    QCOMPARE(Value(&worker, QStringLiteral("bad.join('|')")),
             QStringLiteral("Invalid value for argument 1. Property|Invalid value for argument 1. Property|Invalid value for argument 1. Property|Invalid value for argument 1. Property|Invalid value for argument 1. Property|Invalid value for argument 1. Property"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - askedBefore")), QStringLiteral("0"));
    QJSEngine forms;
    MakeAsking(&forms, Fetching(), QStringLiteral(
        "chrome.runtime.getManifest = function(){ return { permissions: ['offscreen'] }; };"
        "var form = 'promise';"
        "chrome.runtime.getContexts = function(f, cb){"
        "  if (form === 'throw') throw new Error('engine says no');"
        "  if (form === 'reject') return Promise.reject(new Error('engine failed'));"
        "  if (form === 'list') return [{ contextType: 'TAB', contextId: 'sync', tabId: 1 }];"
        "  if (form === 'callback') { cb([{ contextType: 'TAB', contextId: 'cb', tabId: 2 }]); return undefined; }"
        "  if (form === 'lastError') { chrome.runtime.lastError = { message: 'engine said no later' }; try { cb(undefined); } finally { delete chrome.runtime.lastError; } return undefined; }"
        "  if (form === 'silent') return undefined;"
        "  return Promise.resolve([{ contextType: 'TAB', contextId: 'p', tabId: 3 }]); };"));
    forms.evaluate(Calls());
    forms.evaluate(QStringLiteral(
        "var got = [];"
        "function ask(kind, answer){ form = kind; answers.push(answer); return chrome.runtime.getContexts({}).then(function(v){ got.push(kind + ':' + v.map(function(c){ return c.contextId; }).join()); }, function(e){ got.push(kind + ':' + e.message); }); }"
        "ask('throw', { ok: true, value: [] }).then(function(){ return ask('reject', { ok: true, value: [] }); })"
        "  .then(function(){ return ask('list', { ok: true, value: [] }); }).then(function(){ return ask('callback', { ok: true, value: [] }); })"
        "  .then(function(){ return ask('promise', { ok: false, error: 'host says no' }); })"
        "  .then(function(){ return ask('lastError', { ok: true, value: [] }); });"));
    QTRY_COMPARE(Value(&forms, QStringLiteral("got.length")), QStringLiteral("6"));
    QCOMPARE(Value(&forms, QStringLiteral("got.join('|')")),
             QStringLiteral("throw:engine says no|reject:engine failed|list:sync|callback:cb|promise:host says no|lastError:engine said no later"));
    forms.evaluate(QStringLiteral("ask('silent', { ok: true, value: [] });"));
    Settle(&forms);
    QCOMPARE(Value(&forms, QStringLiteral("got.length")), QStringLiteral("6"));
    Pump(&forms);
    QTRY_COMPARE(Value(&forms, QStringLiteral("got.length")), QStringLiteral("7"));
    QCOMPARE(Value(&forms, QStringLiteral("got[6]")), QStringLiteral("silent:"));
}

void tst_cdpshims::theWorkerHasADocumentRunItsOwnFiles(){
    {
        QJSEngine plain;
        MakeAsking(&plain, Fetching(), Saying());
        plain.evaluate(QStringLiteral("var out = 'pending'; chrome.scripting.executeScript({ target: { tabId: 1 }, files: ['a.js'] }).then(function(){ out = 'made?'; }, function(e){ out = e.message; });"));
        QTRY_COMPARE(Value(&plain, QStringLiteral("out")), QStringLiteral("chrome.scripting.executeScript is not available in this browser"));
    }
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting', 'tabs'] }; };"));
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "var served = [];"
        "var texts = { 'lib/a.js': 'self.__a = 1', 'lib/b.js': 'self.__b = 2; 7' };"
        "var hostFetch = fetch;"
        "fetch = function(url, options){"
        "  if (typeof url === 'string' && url.indexOf('chrome-extension://abcdefghijklmnopabcdefghijklmnop/') === 0) {"
        "    var f = url.slice('chrome-extension://abcdefghijklmnopabcdefghijklmnop/'.length); served.push(f);"
        "    if (!(f in texts)) return Promise.resolve({ ok: false });"
        "    return Promise.resolve({ ok: true, text: function(){ return Promise.resolve(texts[f]); } });"
        "  }"
        "  return hostFetch(url, options);"
        "};"
        "chrome.runtime.getURL = function(f){"
        "  var s = String(f), root = 'chrome-extension://' + chrome.runtime.id + '/';"
        "  if (s.slice(0, 2) === '//') return 'chrome-extension://' + s.slice(2);"
        "  var out = [];"
        "  s.replace(/^\\/+/, '').split('/').forEach(function(seg){"
        "    var one = seg; try { one = decodeURIComponent(seg); } catch (e) {}"
        "    if (one === '.') return;"
        "    if (one === '..') { out.pop(); return; }"
        "    out.push(seg); });"
        "  return root + out.join('/'); };"
        "answers.push({ ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } });"
        "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var sub = linking('a2000000000000000000000000000000', 5, 'https://a.example/frame', 200);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.scripting.executeScript")), QStringLiteral("function"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.scripting.executeScript({ target: { tabId: 555 }, files: ['lib/a.js', '/lib/b.js'] })"
        "  .then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("served.join()")), QStringLiteral("lib/a.js,lib/b.js"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(top.last()) + ' ' + sub.posted.length")),
             QStringLiteral("{\"run\":1,\"codes\":[\"self.__a = 1\",\"self.__b = 2; 7\"]} 1"));
    worker.evaluate(QStringLiteral("top.say({ ran: 1, value: 7 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved [{\"frameId\":0,\"result\":7}]"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "chrome.scripting.executeScript({ target: { tabId: 555, allFrames: true }, files: ['lib/a.js'] }, function(r){ out = 'called ' + JSON.stringify(r); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("served.join()")), QStringLiteral("lib/a.js,lib/b.js"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([top.last(), sub.last()])")),
             QStringLiteral("[{\"run\":2,\"codes\":[\"self.__a = 1\"]},{\"run\":1,\"codes\":[\"self.__a = 1\"]}]"));
    worker.evaluate(QStringLiteral("sub.say({ ran: 1, error: 'boom' }); top.say({ ran: 2 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("called [{\"frameId\":0}]"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "chrome.scripting.executeScript({ target: { tabId: 555, frameIds: [5, 5] }, func: function(a, b){ return a + b; }, args: [1, 'x'] })"
        "  .then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sub.last().run + ' ' + sub.last().codes.length + ' ' + sub.last().codes[0].slice(0, 9) + '...' + sub.last().codes[0].slice(-8)")),
             QStringLiteral("2 1 (function...)(1,\"x\")"));
    worker.evaluate(QStringLiteral("sub.say({ ran: 2, error: 'boom' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed boom"));

    QStringList refused;
    foreach(const QString &call, QStringList()
            << "{ target: { tabId: 999 }, files: ['lib/a.js'] }"
            << "{ target: { tabId: 555, frameIds: [9] }, files: ['lib/a.js'] }"
            << "{ target: { tabId: 555 }, files: ['lib/a.js'], func: function(){} }"
            << "{ target: { tabId: 555 } }"
            << "{ target: { tabId: 555 }, files: ['../x.js'] }"
            << "{ target: { tabId: 555 }, files: ['a/%2e%2e/x.js'] }"
            << "{ target: { tabId: 555 }, files: ['//x.js'] }"
            << "{ target: { tabId: 555 }, files: ['lib/nosuch.js'] }"
            << "{ target: { tabId: 555 }, files: ['lib/a.js'], world: 'MAIN' }"
            << "{ target: { tabId: 555 }, func: function(){}, args: [{ deep: [function(){}] }] }"
            << "{ target: { tabId: 555, allFrames: true, frameIds: [0] }, func: function(){} }"
            << "{ target: { tabId: 555, frameIds: [] }, func: function(){} }"
            << "{ target: {}, func: function(){} }"){
        worker.evaluate(QStringLiteral("out = 'pending'; chrome.scripting.executeScript(%1).then(function(){ out = 'made?'; }, function(e){ out = e.message; });").arg(call));
        Settle(&worker);
        refused << Value(&worker, QStringLiteral("out"));
    }
    QCOMPARE(refused, QStringList()
             << "No tab with id: 999."
             << "No frame with id 9 in tab with id 555."
             << "Exactly one of 'files' and 'func' must be specified."
             << "Exactly one of 'files' and 'func' must be specified."
             << "Invalid value for argument 1. Property 'files': expected a list of paths inside the extension."
             << "Invalid value for argument 1. Property 'files': expected a list of paths inside the extension."
             << "Invalid value for argument 1. Property 'files': expected a list of paths inside the extension."
             << "Could not load file: 'lib/nosuch.js'."
             << "world MAIN is not available in this browser"
             << "Invalid value for argument 1. Property 'args': must be JSON-serializable."
             << "Invalid value for argument 1. Cannot specify both 'allFrames' and 'frameIds'."
             << "Invalid value for argument 1. Property 'target.frameIds': expected a list of frame ids."
             << "No tab with id: undefined.");
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + '/' + sub.posted.length")), QStringLiteral("3/3"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "chrome.scripting.executeScript({ target: { tabId: 555, allFrames: true }, files: ['lib/a.js'] })"
        "  .then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("top.say({ ran: top.last().run, error: 'boom' }); sub.say({ ran: sub.last().run, error: 'boom' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved []"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "var lying = function(){ return 1; }; lying.toString = function(){ return 'self.__lied = 1'; };"
        "chrome.scripting.executeScript({ target: { tabId: 555, frameIds: [5] }, func: lying })"
        "  .then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sub.last().codes.join().indexOf('__lied') + ' ' + sub.last().codes[0].slice(0, 9)")),
             QStringLiteral("-1 (function"));
    worker.evaluate(QStringLiteral("sub.say({ ran: sub.last().run });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved [{\"frameId\":5}]"));

    worker.evaluate(QStringLiteral(
        "texts['lib/c.js'] = 'self.__c = 3';"
        "var both = 'pending';"
        "Promise.all([chrome.scripting.executeScript({ target: { tabId: 555 }, files: ['lib/c.js'] }),"
        "             chrome.scripting.executeScript({ target: { tabId: 555 }, files: ['./lib/c.js'] })])"
        "  .then(function(){ both = 'resolved'; }, function(e){ both = 'failed ' + e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("served.filter(function(f){ return f === 'lib/c.js'; }).length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral(
        "chrome.scripting.executeScript({ target: { tabId: 555 }, files: ['./lib/c.js'] }).then(function(){}, function(){});"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("served.filter(function(f){ return f === 'lib/c.js'; }).length")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral(
        "out = 'pending'; texts['lib/nosuch.js'] = 'self.__late = 4';"
        "chrome.scripting.executeScript({ target: { tabId: 555 }, files: ['lib/nosuch.js'] })"
        "  .then(function(r){ out = 'resolved ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("served.filter(function(f){ return f === 'lib/nosuch.js'; }).length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(top.last().codes)")), QStringLiteral("[\"self.__late = 4\"]"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "chrome.scripting.executeScript({ target: { tabId: 555 }, func: function(){ return 1; } }).then(function(){ out = 'made?'; }, function(e){ out = e.message; });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("top.die();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("The frame was removed."));

    QJSEngine content;
    Make(&content, true, Fetching() + Saying());
    content.evaluate(QStringLiteral(
        "replies.push({ linked: 1 });"
        "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("ports.length")), QStringLiteral("1"));
    content.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 1, codes: ['self.__ran = 41', 'self.__ran + 1'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + self.__ran")), QStringLiteral("{\"ran\":1,\"value\":42} 41"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 2, codes: ['throw new Error(\"no\")'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":2,\"error\":\"no\"}"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 3, codes: ['(function(){})'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":3}"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 4 });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":4,\"error\":\"no code to run\"}"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 5, codes: ['Promise.resolve(\"later\")'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":5,\"value\":\"later\"}"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 6, codes: ['Promise.reject(new Error(\"late no\"))'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":6,\"error\":\"late no\"}"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 7, codes: ['throw { get message(){ throw new Error(\"nope\"); } }'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":7,\"error\":\"Error\"}"));
    content.evaluate(QStringLiteral("ports[0].say({ run: 8, codes: ['Promise.reject({ get message(){ throw new Error(\"nope\"); } })'] });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":8,\"error\":\"Error\"}"));
    content.evaluate(QStringLiteral("self.__ran = 0; ports[0].say({ id: 9, message: { run: 7, codes: ['self.__ran = 99'] } });"));
    QCOMPARE(Value(&content, QStringLiteral("String(self.__ran) + ' ' + JSON.stringify(ports[0].last())")), QStringLiteral("0 {\"re\":9}"));

    {
        QJSEngine many;
        MakeAsking(&many, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'] }; };"));
        many.evaluate(Linking());
        many.evaluate(QStringLiteral(
            "var served = [], ownRoot = 'chrome-extension://' + chrome.runtime.id + '/';"
            "var hostFetch = fetch;"
            "fetch = function(url, options){"
            "  if (typeof url === 'string' && url.indexOf(ownRoot) === 0) {"
            "    served.push(url.slice(ownRoot.length));"
            "    return Promise.resolve({ ok: true, text: function(){ return Promise.resolve('void 0'); } });"
            "  }"
            "  return hostFetch(url, options);"
            "};"
            "answers.push({ ok: true, value: { id: 555, index: 0 } });"
            "var one = linking('c1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
            "function names(from, to){ var list = []; for (var i = from; i < to; i++) list.push('k' + i + '.js'); return list; }"
            "function askFor(list){ return chrome.scripting.executeScript({ target: { tabId: 555 }, files: list })"
            "  .then(function(){}, function(){}); }"));
        Settle(&many);
        many.evaluate(QStringLiteral("askFor(names(0, 64));"));
        Settle(&many);
        QCOMPARE(Value(&many, QStringLiteral("served.length")), QStringLiteral("64"));
        many.evaluate(QStringLiteral("askFor(names(0, 64)); askFor(['k64.js']);"));
        Settle(&many);
        QCOMPARE(Value(&many, QStringLiteral("served.length + ' ' + served[64]")), QStringLiteral("65 k64.js"));
        many.evaluate(QStringLiteral("askFor(names(0, 64));"));
        Settle(&many);
        QCOMPARE(Value(&many, QStringLiteral("served.length + ' ' + served[65]")), QStringLiteral("66 k0.js"));
    }
}

void tst_cdpshims::whatIsDoneToADownloadIsAskedOfTheApplication(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var out = [];"
        "answers.push({ ok: true }, { ok: true }, { ok: true }, { ok: true, value: [7] }, { ok: true },"
        "             { ok: false, error: 'Download must be in progress' });"
        "chrome.downloads.cancel(7).then(function(v){ out.push('cancel ' + v); });"
        "chrome.downloads.pause(7).then(function(v){ out.push('pause ' + v); });"
        "chrome.downloads.resume(7).then(function(v){ out.push('resume ' + v); });"
        "chrome.downloads.erase({ id: 7 }).then(function(v){ out.push('erase ' + JSON.stringify(v)); });"
        "chrome.downloads.show(7, function(){ out.push('show ' + (chrome.runtime.lastError ? 'ERROR' : 'fine')); });"
        "chrome.downloads.pause(8).then(function(){ out.push('paused?'); }, function(e){ out.push('refused ' + e.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.length")), QStringLiteral("6"));
    QCOMPARE(Value(&worker, QStringLiteral("out.join(' | ')")),
             QStringLiteral("cancel undefined | pause undefined | resume undefined | erase [7] | show fine | refused Download must be in progress"));
    QCOMPARE(Value(&worker, QStringLiteral("[0, 1, 2, 3, 4, 5].map(function(i){ return call(i).api + JSON.stringify(call(i).args); }).join(' ')")),
             QStringLiteral("downloads.cancel[7] downloads.pause[7] downloads.resume[7] downloads.erase[{\"id\":7}] downloads.show[7] downloads.pause[8]"));
    worker.evaluate(QStringLiteral(
        "var before = fetched.length, danger = 'pending', opened = 'pending';"
        "chrome.downloads.acceptDanger(7).then(function(){ danger = 'resolved'; }, function(e){ danger = e.message; });"
        "chrome.downloads.open(7).then(function(){ opened = 'resolved'; }, function(e){ opened = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("danger")), QStringLiteral("chrome.downloads.acceptDanger is not available in this browser"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("opened")), QStringLiteral("chrome.downloads.open is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - before")), QStringLiteral("0"));

    QJSEngine hearing;
    MakeAsking(&hearing, Fetching(), Saying() + Calls());
    hearing.evaluate(QStringLiteral(
        "var log = [];"
        "held = []; answers.push('HELD');"
        "chrome.downloads.onErased.addListener(function(id){ log.push('erased ' + id); });"
        "answers.push('HELD');"
        "chrome.downloads.download({ url: 'blob:http://a.example/u1' }).then(function(id){ log.push('id ' + id); });"));
    Settle(&hearing);
    QCOMPARE(Value(&hearing, QStringLiteral("call(0).api + '/' + held.length")), QStringLiteral("vanilla.events/2"));
    hearing.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'downloads.onErased', args: [7] }], order: [] } }, 'HELD');"
        "held.shift()();"));
    Settle(&hearing);
    QCOMPARE(Value(&hearing, QStringLiteral("log.join(' | ')")), QStringLiteral("erased 7"));

    QJSEngine edge;
    MakeEdgeWorker(&edge, Saying() + Calls());
    QCOMPARE(Value(&edge, QStringLiteral("typeof chrome.downloads.onErased + '/' + typeof chrome.downloads.onCreated.addListener")),
             QStringLiteral("undefined/function"));
    QCOMPARE(Value(&edge, QStringLiteral("report.indexOf('engine:edge') >= 0")), QStringLiteral("true"));
}

void tst_cdpshims::aDownloadIsAskedOfTheApplication(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.downloads.download + '/' + typeof chrome.downloads.search + '/' + typeof chrome.downloads.onChanged.addListener")),
             QStringLiteral("function/function/function"));
    worker.evaluate(QStringLiteral(
        "var out = [];"
        "answers.push({ ok: true, value: 7 });"
        "chrome.downloads.download({ url: 'blob:http://a.example/u1', filename: 'page.html', saveAs: false, conflictAction: 'uniquify' })"
        "  .then(function(id){ out.push('id ' + id); }, function(e){ out.push('failed ' + e.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.join()")), QStringLiteral("id 7"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(0))")),
             QStringLiteral("{\"api\":\"downloads.expect\",\"args\":[{\"url\":\"blob:http://a.example/u1\",\"filename\":\"page.html\"}]}"));
    QCOMPARE(Value(&worker, QStringLiteral("sent.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false, error: 'Invalid value for argument 1. Property \\'url\\': Invalid URL.' });"
        "chrome.downloads.download({ url: 'data:text/plain,hi' }, function(id){ out.push('cb ' + id + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message.slice(0, 20) : 'ok')); });"));
    for(int i = 0; i < 30 && Value(&worker, QStringLiteral("out.length")) != QStringLiteral("2"); i++){ QTest::qWait(10); Pump(&worker); }
    QCOMPARE(Value(&worker, QStringLiteral("out[1]")), QStringLiteral("cb undefined Invalid value for ar"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(1).args[0])")), QStringLiteral("{\"url\":\"data:text/plain,hi\",\"filename\":\"\"}"));
    worker.evaluate(QStringLiteral(
        "['javascript:alert(1)', 'https://a.example/f.zip', 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/x', 5].forEach(function(url){"
        "  chrome.downloads.download({ url: url }).then(function(){ out.push('made?'); }, function(e){ out.push('bad ' + e.message.slice(0, 30)); }); });"
        "chrome.downloads.download().then(function(){ out.push('made?'); }, function(e){ out.push('bad ' + e.message.slice(0, 30)); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.length")), QStringLiteral("7"));
    QCOMPARE(Value(&worker, QStringLiteral("out.slice(2).join('|')")),
             QStringLiteral("bad Invalid value for argument 1. |bad Invalid value for argument 1. |bad Invalid value for argument 1. |bad Invalid value for argument 1. |bad Invalid value for argument 1. "));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("2"));

    QJSEngine holding;
    MakeAsking(&holding, Fetching(), Saying() + Calls());
    holding.evaluate(QStringLiteral(
        "var log = [];"
        "held = []; answers.push('HELD');"
        "chrome.downloads.onChanged.addListener(function(d){ log.push('changed ' + d.id + ' ' + (d.state && d.state.current)); });"
        "chrome.downloads.onCreated.addListener(function(d){ log.push('created ' + d.id); });"
        "answers.push('HELD');"
        "chrome.downloads.download({ url: 'blob:http://a.example/u1', filename: 'page.html' })"
        "  .then(function(id){ log.push('id ' + id); }, function(e){ log.push('failed ' + e.message); });"));
    Settle(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("held.length")), QStringLiteral("2"));
    holding.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'downloads.onCreated', args: [{ id: 7, url: 'blob:http://a.example/u1', state: 'in_progress' }] },"
        "  { name: 'downloads.onChanged', args: [{ id: 7, state: { previous: 'in_progress', current: 'complete' } }] },"
        "  { name: 'downloads.onChanged', args: [{ id: 6, state: { previous: 'in_progress', current: 'interrupted' } }] }"
        "], order: [] } }, 'HELD');"
        "held.shift()();"));
    Settle(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral(""));
    holding.evaluate(QStringLiteral("answers.push({ ok: true, value: 7 }); held.shift()();"));
    QTRY_COMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 7"));
    Pump(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 7 | created 7 | changed 7 complete | changed 6 interrupted"));
    holding.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'downloads.onChanged', args: [{ id: 7, state: { previous: 'complete', current: 'complete' } }] }], order: [] } }, 'HELD');"
        "held.shift()();"));
    Settle(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 7 | created 7 | changed 7 complete | changed 6 interrupted | changed 7 complete"));
    holding.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "chrome.downloads.download({ url: 'blob:http://a.example/u2' }).then(function(id){ log.push('id ' + id); }, function(e){ log.push('refused'); });"));
    Settle(&holding);
    holding.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'downloads.onChanged', args: [{ id: 7, state: { previous: 'complete', current: 'complete' } }] }], order: [] } }, 'HELD');"
        "held.shift()();"));
    Settle(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.length")), QStringLiteral("5"));
    holding.evaluate(QStringLiteral("answers.push({ ok: false, error: 'no' }); held.shift()();"));
    QTRY_COMPARE(Value(&holding, QStringLiteral("log[5]")), QStringLiteral("refused"));
    Pump(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.length")), QStringLiteral("7"));
    holding.evaluate(QStringLiteral(
        "log = [];"
        "answers.push('HELD', 'HELD');"
        "chrome.downloads.download({ url: 'blob:http://a.example/u3' }).then(function(id){ log.push('id ' + id); });"
        "chrome.downloads.download({ url: 'blob:http://a.example/u4' }).then(function(id){ log.push('id ' + id); });"));
    Settle(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("held.length")), QStringLiteral("3"));
    holding.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'downloads.onCreated', args: [{ id: 9, state: 'in_progress' }] }], order: [] } }, 'HELD');"
        "held.shift()();"));
    Settle(&holding);
    holding.evaluate(QStringLiteral("var second = held.splice(1, 1)[0]; answers.push({ ok: true, value: 9 }); second();"));
    QTRY_COMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 9"));
    Pump(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 9"));
    holding.evaluate(QStringLiteral("answers.push({ ok: true, value: 8 }); held.shift()();"));
    QTRY_COMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 9 | id 8"));
    Pump(&holding);
    QCOMPARE(Value(&holding, QStringLiteral("log.join(' | ')")), QStringLiteral("id 9 | id 8 | created 9"));
}

void tst_cdpshims::theSecondRootTheEngineHasIsStoodInForToo(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), TheOtherRoot());

    QCOMPARE(Value(&worker, QStringLiteral(
                 "(browser.tabs === chrome.tabs) + '/' + (chrome.tabs !== enginesTabs) + '/' + (browser.tabs !== enginesOtherTabs)")),
             QStringLiteral("true/true/true"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 45, index: 0 }] });"
        "browser.tabs.query({ active: true, currentWindow: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"id\":45,\"index\":0}]"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true,\"currentWindow\":true}]}"));
    QCOMPARE(Value(&worker, QStringLiteral("rootCalls.join()")), QString());

    worker.evaluate(QStringLiteral(
        "var up = 'pending', zoom = 'pending';"
        "answers.push({ ok: true, value: { id: 45, index: 1 } });"
        "browser.tabs.update(45, { active: true }).then(function(v){ up = JSON.stringify(v); }, function(e){ up = 'rejected: ' + e.message; });"
        "browser.tabs.setZoom(45, 1.5).then(function(v){ zoom = 'resolved: ' + v; }, function(e){ zoom = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("up")), QStringLiteral("{\"id\":45,\"index\":1}"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("zoom")), QStringLiteral("chrome.tabs.setZoom is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("rootCalls.join()")), QString());

    QCOMPARE(Value(&worker, QStringLiteral(
                 "(browser.tabs.onUpdated === chrome.tabs.onUpdated) + '/' + (browser.tabs.onUpdated !== enginesOtherTabs.onUpdated)")),
             QStringLiteral("true/true"));
    worker.evaluate(QStringLiteral("var moved = function(){}; browser.tabs.onReplaced.addListener(moved);"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "browser.tabs.onReplaced.hasListener(moved) + '/' + browser.tabs.onReplaced.hasListeners() + '/' + rootCalls.join()")),
             QStringLiteral("false/false/"));

    QCOMPARE(Value(&worker, QStringLiteral("browser.tabs.TAB_ID_NONE + '/' + chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1/-1"));

    QCOMPARE(Value(&worker, QStringLiteral(
                 "(browser.storage === enginesOtherStorage) + '/' + browser.storage.local.mark + '/' + (browser.runtime === chrome.runtime)")),
             QStringLiteral("true/the engine's own/true"));

    worker.evaluate(QStringLiteral("self.browser.tabs = { TAB_ID_NONE: -99, MARK: 7, update: function(){} };"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "(browser.tabs === chrome.tabs) + '/' + typeof browser.tabs.query + '/' + String(browser.tabs.MARK)")),
             QStringLiteral("true/function/undefined"));
}

void tst_cdpshims::anExtensionsPageStandsInOnTheSecondRootToo(){
    QJSEngine page;
    MakeKeyedPage(&page, Fetching() + TheOtherRoot());
    QCOMPARE(Value(&page, QStringLiteral(
                 "(browser.tabs === chrome.tabs) + '/' + (browser.tabs !== enginesOtherTabs) + '/' + (browser.storage === enginesOtherStorage)")),
             QStringLiteral("true/true/true"));

    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 45, index: 3 }] });"
        "browser.tabs.query({ active: true, currentWindow: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("[{\"id\":45,\"index\":3}]"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key'] + '/' + rootCalls.join()")), PageKey() + QStringLiteral("/"));
    QCOMPARE(Value(&page, QStringLiteral("browser.tabs.TAB_ID_NONE")), QStringLiteral("-1"));
}

void tst_cdpshims::whereTheEngineHasNoSecondRootNoneIsMade(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), EnginesOwn());
    QCOMPARE(Value(&worker, QStringLiteral("typeof browser + '/' + ('browser' in self)")), QStringLiteral("undefined/false"));
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.tabs.query")), QStringLiteral("function"));

    QJSEngine page;
    MakeKeyedPage(&page, Fetching());
    QCOMPARE(Value(&page, QStringLiteral("typeof browser + '/' + ('browser' in self)")), QStringLiteral("undefined/false"));
    QCOMPARE(Value(&page, QStringLiteral("typeof chrome.tabs.query")), QStringLiteral("function"));
}

void tst_cdpshims::theNamesAStandInAnswersForAreChromesAndInIsWider(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), QStringLiteral(
        "chrome.tabs = { TAB_ID_NONE: -1, _hidden: 1,"
        "                then: function(){ return 'the engine\\'s own'; },"
        "                toJSON: function(){ return 'the engine\\'s own'; },"
        "                update: function(){} };"));

    const QString reads = QStringLiteral(
        "['then', 'toJSON', '_hidden', '_x', '0', '1a'].map(function(k){ return String(%1[k]); }).join()");
    QCOMPARE(Value(&worker, reads.arg(QStringLiteral("chrome.tabs"))),
             QStringLiteral("undefined,undefined,undefined,undefined,undefined,undefined"));
    QCOMPARE(Value(&worker, reads.arg(QStringLiteral("chrome.windows"))),
             QStringLiteral("undefined,undefined,undefined,undefined,undefined,undefined"));

    const QString ins = QStringLiteral(
        "['then', 'toJSON', '_hidden', '_x', '0', '1a'].map(function(k){ return k in %1; }).join()");
    QCOMPARE(Value(&worker, ins.arg(QStringLiteral("chrome.tabs"))),
             QStringLiteral("true,true,true,false,false,false"));
    QCOMPARE(Value(&worker, ins.arg(QStringLiteral("chrome.windows"))),
             QStringLiteral("false,false,false,false,false,false"));

    const QString symbols = QStringLiteral(
        "[Symbol.toPrimitive, Symbol.toStringTag, Symbol.iterator]"
        "  .map(function(s){ return String(%1[s]) + ':' + (s in %1); }).join()");
    QCOMPARE(Value(&worker, symbols.arg(QStringLiteral("chrome.tabs"))),
             QStringLiteral("undefined:false,undefined:false,undefined:false"));
    QCOMPARE(Value(&worker, symbols.arg(QStringLiteral("chrome.windows"))),
             QStringLiteral("undefined:false,undefined:false,undefined:false"));

    QCOMPARE(Value(&worker, QStringLiteral(
                 "typeof chrome.tabs.query + '/' + typeof chrome.windows.getCurrent + '/' + chrome.tabs.TAB_ID_NONE")),
             QStringLiteral("function/function/-1"));
}

void tst_cdpshims::theIndexOfASendersTabIsReadFromTheHostsAnswers(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var senders = []; Math.random = function(){ return 0.25; };"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "answers.push({ ok: true, value: { id: 555, index: 3 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '11000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.id + ':' + senders[0].tab.index")), QStringLiteral("555:3"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 111, index: 0 }, { id: 555, index: 7 }] });"
        "chrome.tabs.query({}).then(function(){ out = 'query saw ' + senders[0].tab.index; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("query saw 7"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.index")), QStringLiteral("7"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(senders[0].tab).indexOf('\"index\":7') >= 0")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("Object.assign({}, senders[0].tab).index")), QStringLiteral("7"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: { id: 1, tabs: [{ id: 555, index: 2 }] } });"
        "chrome.windows.getCurrent({ populate: true }).then(function(){ out = 'window saw ' + senders[0].tab.index; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("window saw 2"));

    worker.evaluate(QStringLiteral(
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: null, frameId: 0, tabUrl: 'elsewhere' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[senders.length - 1].tab.id + ':' + senders[senders.length - 1].tab.index")),
             QStringLiteral("1342177280:0"));
}

void tst_cdpshims::aTabOfAnswerOfAnotherShapeIsNoAnswer(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = []; Math.random = function(){ return 0.25; };"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id + '@' + s.tab.index); return false; });"
        "answers.push({ ok: true, value: 555 },"
        "             { ok: true, value: { id: 555 } },"
        "             { ok: true, value: { id: 555, index: -1 } },"
        "             { ok: true, value: { id: '555', index: 3 } },"
        "             { ok: true, value: { id: 0, index: 0 } },"
        "             { ok: true, value: { id: 555, index: 1.5 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '12000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    DrainBefore(&worker, 1000);
    QCOMPARE(Value(&worker, QStringLiteral("got.join()")), QStringLiteral("1342177280@0"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("6"));
    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("30"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("0"));
}

void tst_cdpshims::withoutAKeyNobodyIsAsked(){
    QJSEngine unkeyed;
    unkeyed.evaluate(World());
    unkeyed.evaluate(QStringLiteral("document = undefined;") + Fetching());
    QVERIFY(!unkeyed.evaluate(Cdp::WorkerShim()).isError());
    unkeyed.evaluate(QStringLiteral("var out = 'pending'; chrome.tabs.query({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&unkeyed, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
    QCOMPARE(Value(&unkeyed, QStringLiteral("fetched.length")), QStringLiteral("0"));

    QJSEngine fetchless;
    fetchless.evaluate(World());
    fetchless.evaluate(QStringLiteral("document = undefined;"));
    QVERIFY(!fetchless.evaluate(Cdp::WorkerShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), QString(64, QLatin1Char('5')))).isError());
    fetchless.evaluate(QStringLiteral("var out = 'pending'; chrome.tabs.query({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&fetchless, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
}

void tst_cdpshims::theTreeTheApplicationKeepsIsAskedOfIt(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: '7', url: 'https://a.example/', title: 'A', lastVisitTime: 1700000000000 },"
        "                                 { id: '9', url: 'https://b.example/', title: 'B', lastVisitTime: 1699000000000 }] });"
        "chrome.history.search({ text: '', maxResults: 20000, startTime: 0 })"
        "  .then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")),
                 QStringLiteral("[{\"id\":\"7\",\"url\":\"https://a.example/\",\"title\":\"A\",\"lastVisitTime\":1700000000000},"
                                "{\"id\":\"9\",\"url\":\"https://b.example/\",\"title\":\"B\",\"lastVisitTime\":1699000000000}]"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[0].url + ' ' + fetched[0].options.method + ' ' + ('body' in fetched[0].options)")),
             QStringLiteral("vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"history.search\",\"args\":[{\"text\":\"\",\"maxResults\":20000,\"startTime\":0}]}"));

    worker.evaluate(QStringLiteral(
        "var tree = 'pending';"
        "answers.push({ ok: true, value: [{ id: '0', title: '', children:"
        "                 [{ id: '3', parentId: '0', index: 0, title: 'work', children:"
        "                    [{ id: '7', parentId: '3', index: 0, title: 'A', url: 'https://a.example/' }] }] }] });"
        "chrome.bookmarks.getTree().then(function(v){ tree = JSON.stringify(v); }, function(e){ tree = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("tree")),
                 QStringLiteral("[{\"id\":\"0\",\"title\":\"\",\"children\":"
                                "[{\"id\":\"3\",\"parentId\":\"0\",\"index\":0,\"title\":\"work\",\"children\":"
                                "[{\"id\":\"7\",\"parentId\":\"3\",\"index\":0,\"title\":\"A\",\"url\":\"https://a.example/\"}]}]}]"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[1].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"bookmarks.getTree\",\"args\":[]}"));

    worker.evaluate(QStringLiteral(
        "var called = 'pending';"
        "answers.push({ ok: true, value: [{ id: '0', title: '', children: [] }] });"
        "chrome.bookmarks.getTree(function(v){ called = JSON.stringify(v) + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine'); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called")),
                 QStringLiteral("[{\"id\":\"0\",\"title\":\"\",\"children\":[]}] fine"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[2].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"bookmarks.getTree\",\"args\":[]}"));

    worker.evaluate(QStringLiteral(
        "var refused = 'pending', denied = 'pending';"
        "answers.push({ ok: false, error: 'chrome.history.search is not available in this browser' },"
        "             { ok: false, error: 'chrome.bookmarks.getTree is not available in this browser' });"
        "chrome.history.search({ text: 'x' }).then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"
        "chrome.bookmarks.getTree().then(function(){ denied = 'resolved'; }, function(e){ denied = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("refused")), QStringLiteral("chrome.history.search is not available in this browser"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("denied")), QStringLiteral("chrome.bookmarks.getTree is not available in this browser"));

    worker.evaluate(QStringLiteral(
        "var before = fetched.length, made = 'pending', wiped = 'pending', gone = 'pending';"
        "chrome.bookmarks.create({ title: 'x' }).then(function(){ made = 'resolved'; }, function(e){ made = e.message; });"
        "chrome.bookmarks.remove('3').then(function(){ gone = 'resolved'; }, function(e){ gone = e.message; });"
        "chrome.history.deleteAll().then(function(){ wiped = 'resolved'; }, function(e){ wiped = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("made")), QStringLiteral("chrome.bookmarks.create is not available in this browser"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("gone")), QStringLiteral("chrome.bookmarks.remove is not available in this browser"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("wiped")), QStringLiteral("chrome.history.deleteAll is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - before")), QStringLiteral("0"));
}

void tst_cdpshims::theTreeIsReadByIdAndByWordOverTheSameWire(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = [];"
        "answers.push({ ok: true, value: [{ id: '3', parentId: '0', index: 0, title: 'work' }] },"
        "             { ok: true, value: [{ id: '7', parentId: '3', index: 0, title: 'A', url: 'https://a.example/' }] },"
        "             { ok: true, value: [{ id: '0', title: '', children: [] }] },"
        "             { ok: true, value: [] });"
        "chrome.bookmarks.get(['3']).then(function(v){ out.push('get ' + JSON.stringify(v)); });"
        "chrome.bookmarks.getChildren('3').then(function(v){ out.push('children ' + JSON.stringify(v)); });"
        "chrome.bookmarks.getSubTree('0').then(function(v){ out.push('subtree ' + JSON.stringify(v)); });"
        "chrome.bookmarks.search({ query: 'w', url: null }).then(function(v){ out.push('search ' + JSON.stringify(v)); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.length")), QStringLiteral("4"));
    QCOMPARE(Value(&worker, QStringLiteral("out.join(' | ')")),
             QStringLiteral("get [{\"id\":\"3\",\"parentId\":\"0\",\"index\":0,\"title\":\"work\"}]"
                            " | children [{\"id\":\"7\",\"parentId\":\"3\",\"index\":0,\"title\":\"A\",\"url\":\"https://a.example/\"}]"
                            " | subtree [{\"id\":\"0\",\"title\":\"\",\"children\":[]}]"
                            " | search []"));
    QCOMPARE(Value(&worker, QStringLiteral("[0, 1, 2, 3].map(function(i){ return decodeURIComponent(fetched[i].options.headers['X-Vanilla-Call']); }).join(' ')")),
             QStringLiteral("{\"api\":\"bookmarks.get\",\"args\":[[\"3\"]]}"
                            " {\"api\":\"bookmarks.getChildren\",\"args\":[\"3\"]}"
                            " {\"api\":\"bookmarks.getSubTree\",\"args\":[\"0\"]}"
                            " {\"api\":\"bookmarks.search\",\"args\":[{\"query\":\"w\",\"url\":null}]}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.every(function(f){ return f.url === 'vanilla-extension://host/call' && f.options.method === 'POST' && !('body' in f.options); })")),
             QStringLiteral("true"));

    worker.evaluate(QStringLiteral(
        "var called = 'pending', refused = 'pending';"
        "answers.push({ ok: true, value: [{ id: '9', parentId: '0', index: 1, title: 'B', url: 'https://b.example/' }] },"
        "             { ok: false, error: 'chrome.bookmarks.get is not available in this browser' });"
        "chrome.bookmarks.search('b', function(v){ called = JSON.stringify(v) + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine'); });"
        "chrome.bookmarks.get('9').then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called")), QStringLiteral("[{\"id\":\"9\",\"parentId\":\"0\",\"index\":1,\"title\":\"B\",\"url\":\"https://b.example/\"}] fine"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("refused")), QStringLiteral("chrome.bookmarks.get is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[4].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"bookmarks.search\",\"args\":[\"b\"]}"));
}

void tst_cdpshims::aNodeOfTheTreeIsNoTabHoweverItLooks(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var senders = []; Math.random = function(){ return 0.25; };"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "answers.push({ ok: true, value: { id: 555, index: 3 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '11000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.id + ':' + senders[0].tab.index")), QStringLiteral("555:3"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending', many = [];"
        "for (var i = 0; i < 5000; i++) many.push({ id: String(i), index: i, url: 'https://x.example/' + i, title: 't' + i, lastVisitTime: i });"
        "many[0].id = '555';"
        "answers.push({ ok: true, value: many },"
        "             { ok: true, value: [{ id: '0', title: '', children:"
        "                 [{ id: '555', parentId: '0', index: 11, title: 'A', url: 'https://a.example/' }] }] });"
        "Promise.all([chrome.history.search({ text: '', maxResults: 20000, startTime: 0 }), chrome.bookmarks.getTree()])"
        "  .then(function(v){ out = v[0].length + ' saw ' + senders[0].tab.index; }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("5000 saw 3"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.index")), QStringLiteral("3"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 555, index: 7 }] });"
        "chrome.tabs.query({}).then(function(){ out = 'query saw ' + senders[0].tab.index; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("query saw 7"));
}

void tst_cdpshims::theVisitsAreToldAndTheBookmarksEventsAreNot(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var log = [], threw = 'no';"
        "answers.push('HELD');"
        "var mine = function(item){ log.push('visited ' + JSON.stringify(item)); };"
        "try { chrome.history.onVisited.addListener(mine);"
        "      chrome.history.onVisitRemoved.addListener(function(r){ log.push('removed ' + JSON.stringify(r)); });"
        "      chrome.bookmarks.onCreated.addListener(function(){ log.push('bookmark made'); });"
        "      chrome.bookmarks.onRemoved.addListener(function(){ log.push('bookmark gone'); }); } catch (e) { threw = String(e && e.message); }"));
    QCOMPARE(Value(&worker, QStringLiteral("threw")), QStringLiteral("no"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + call(0).api")), QStringLiteral("1/vanilla.events"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.history.onVisited.hasListener(mine) + '/' + chrome.history.onVisited.hasListeners()"
                                           " + '/' + chrome.history.onVisitRemoved.hasListeners()")),
             QStringLiteral("true/true/true"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.bookmarks.onCreated.hasListeners() + '/' + chrome.bookmarks.onRemoved.hasListeners()")),
             QStringLiteral("false/false"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'history.onVisitRemoved', args: [{ allHistory: false, urls: ['https://a.example/'] }] },"
        "  { name: 'history.onVisited', args: [{ id: '7', url: 'https://b.example/', title: 'B', lastVisitTime: 1700000000000 }] },"
        "  { name: 'bookmarks.onCreated', args: ['7', { id: '7', title: 'B' }] }"
        "], order: [7] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("removed {\"allHistory\":false,\"urls\":[\"https://a.example/\"]}"
                            " | visited {\"id\":\"7\",\"url\":\"https://b.example/\",\"title\":\"B\",\"lastVisitTime\":1700000000000}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + (call(0).args[0] === call(1).args[0]) + '/' + warned.length + '/' + cried.length")),
             QStringLiteral("2/true/0/0"));
    worker.evaluate(QStringLiteral(
        "chrome.history.onVisited.removeListener(mine);"
        "chrome.history.onVisitRemoved.addListener('not a function');"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.history.onVisited.hasListeners() + '/' + chrome.history.onVisitRemoved.hasListeners()")),
             QStringLiteral("false/true"));
}

void tst_cdpshims::theFontsAreAskedOfTheApplication(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = 'pending', called = 'pending';"
        "answers.push({ ok: true, value: [{ fontId: 'Meiryo', displayName: 'Meiryo' }] },"
        "             { ok: true, value: [] });"
        "chrome.fontSettings.getFontList().then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"
        "chrome.fontSettings.getFontList(function(v){ called = JSON.stringify(v) + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine'); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"fontId\":\"Meiryo\",\"displayName\":\"Meiryo\"}]"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called")), QStringLiteral("[] fine"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call']) + ' ' + fetched[0].options.method + ' ' + ('body' in fetched[0].options)")),
             QStringLiteral("{\"api\":\"fontSettings.getFontList\",\"args\":[]} POST false"));
    worker.evaluate(QStringLiteral(
        "var before = fetched.length, refused = 'pending', font = 'pending';"
        "answers.push({ ok: false, error: 'chrome.fontSettings.getFontList is not available in this browser' });"
        "chrome.fontSettings.getFontList().then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"
        "chrome.fontSettings.getFont({ genericFamily: 'serif' }).then(function(){ font = 'resolved'; }, function(e){ font = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("refused")), QStringLiteral("chrome.fontSettings.getFontList is not available in this browser"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("font")), QStringLiteral("chrome.fontSettings.getFont is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - before")), QStringLiteral("1"));

    QJSEngine page;
    MakeKeyedPage(&page, Fetching());
    page.evaluate(QStringLiteral(
        "var got = 'pending';"
        "answers.push({ ok: true, value: [{ fontId: 'Arial', displayName: 'Arial' }] });"
        "chrome.fontSettings.getFontList(function(v){ got = v.map(function(f){ return f.fontId; }).join(); });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("got")), QStringLiteral("Arial"));
    QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"fontSettings.getFontList\",\"args\":[]}"));
}

void tst_cdpshims::theTopSitesAreAskedOfTheApplication(){
    const QString engines = QStringLiteral(
        "var topCalls = [];"
        "chrome.topSites = { get: function(){ topCalls.push('get'); return Promise.resolve([{ url: 'https://www.office.com/', title: 'Office' }]); } };");
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), engines);
    worker.evaluate(QStringLiteral(
        "var out = 'pending', called = 'pending';"
        "answers.push({ ok: true, value: [{ url: 'https://a.example/', title: 'A' }] }, { ok: true, value: [] });"
        "chrome.topSites.get().then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"
        "chrome.topSites.get(function(v){ called = JSON.stringify(v) + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine'); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"url\":\"https://a.example/\",\"title\":\"A\"}]"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called")), QStringLiteral("[] fine"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call']) + ' ' + topCalls.length")),
             QStringLiteral("{\"api\":\"topSites.get\",\"args\":[]} 0"));
    worker.evaluate(QStringLiteral(
        "var refused = 'pending';"
        "answers.push({ ok: false, error: 'chrome.topSites.get is not available in this browser' });"
        "chrome.topSites.get().then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("refused")), QStringLiteral("chrome.topSites.get is not available in this browser"));

    QJSEngine page;
    MakeEdgePage(&page, engines);
    page.evaluate(QStringLiteral(
        "var got = 'pending';"
        "answers.push({ ok: true, value: [{ url: 'https://b.example/', title: 'B' }] });"
        "chrome.topSites.get(function(v){ got = v.map(function(s){ return s.title; }).join(); });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("got")), QStringLiteral("B"));
    QCOMPARE(Value(&page, QStringLiteral("hostFetches() + ' ' + topCalls.length")), QStringLiteral("1 0"));
    QJSEngine edge;
    MakeEdgeWorker(&edge, engines);
    edge.evaluate(QStringLiteral("chrome.topSites.get().then(function(){}, function(){});"));
    Settle(&edge);
    QCOMPARE(Value(&edge, QStringLiteral("waitingHere() + ' ' + topCalls.length")), QStringLiteral("1 0"));
    QJSEngine keyless;
    keyless.evaluate(World());
    keyless.evaluate(AtAPage());
    keyless.evaluate(Fetching() + HostFetches() + EdgeBrands() + EdgesOwn() + engines);
    QVERIFY(!keyless.evaluate(Cdp::PageShim()).isError());
    keyless.evaluate(QStringLiteral("var theirs = 'pending'; chrome.topSites.get().then(function(v){ theirs = v[0].title; });"));
    QTRY_COMPARE(Value(&keyless, QStringLiteral("theirs + ' ' + topCalls.length + ' ' + hostFetches()")), QStringLiteral("Office 1 0"));
}

void tst_cdpshims::whatTurnsAnExtensionOffIsNotPassedThrough(){
    const QString engines = QStringLiteral(
        "var managed = [];"
        "chrome.management = {"
        "  setEnabled: function(){ managed.push('setEnabled'); return Promise.resolve(); },"
        "  uninstall: function(){ managed.push('uninstall'); return Promise.resolve(); },"
        "  uninstallSelf: function(){ managed.push('uninstallSelf'); return Promise.resolve(); },"
        "  launchApp: function(){ managed.push('launchApp'); return Promise.resolve(); },"
        "  createAppShortcut: function(){ managed.push('createAppShortcut'); return Promise.resolve(); },"
        "  setLaunchType: function(){ managed.push('setLaunchType'); return Promise.resolve(); },"
        "  generateAppForLink: function(){ managed.push('generateAppForLink'); return Promise.resolve(); },"
        "  installReplacementWebApp: function(){ managed.push('installReplacementWebApp'); return Promise.resolve(); },"
        "  getPermissionWarningsByManifest: function(){ managed.push('warnings'); return Promise.resolve([]); },"
        "  getAll: function(){ managed.push('getAll'); return Promise.resolve([{ id: 'x' }]); },"
        "  getSelf: function(){ managed.push('getSelf'); return Promise.resolve({ id: 'me' }); },"
        "  onEnabled: { addListener: function(){ managed.push('onEnabled'); }, removeListener: function(){},"
        "               hasListener: function(){ return false; }, hasListeners: function(){ return false; } } };"
        "if (self.browser) self.browser.management = chrome.management;");
    const QString calls = QStringLiteral(
        "var outs = [];"
        "chrome.management.setEnabled('other', false).then(function(){ outs.push('setEnabled resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.uninstall('other', { showConfirmDialog: false }, function(){ outs.push('callback: ' + (chrome.runtime.lastError || {}).message); });"
        "(self.browser || chrome).management.uninstallSelf({ showConfirmDialog: true }).then(function(){ outs.push('uninstallSelf resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.launchApp('other').then(function(){ outs.push('launchApp resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.createAppShortcut('other').then(function(){ outs.push('shortcut resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.setLaunchType('other', 'OPEN_AS_WINDOW').then(function(){ outs.push('launch type resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.generateAppForLink('https://a.example/', 'a').then(function(){ outs.push('app resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.installReplacementWebApp().then(function(){ outs.push('replacement resolved'); }, function(e){ outs.push(e.message); });"
        "chrome.management.getPermissionWarningsByManifest('{}').then(function(v){ outs.push('warnings ' + v.length); });"
        "chrome.management.getAll().then(function(v){ outs.push('all ' + v.length); });"
        "chrome.management.onEnabled.addListener(function(){});");
    const QString said = QStringLiteral(
        "all 1|callback: chrome.management.uninstall is not available in this browser|"
        "chrome.management.createAppShortcut is not available in this browser|"
        "chrome.management.generateAppForLink is not available in this browser|"
        "chrome.management.installReplacementWebApp is not available in this browser|"
        "chrome.management.launchApp is not available in this browser|"
        "chrome.management.setEnabled is not available in this browser|"
        "chrome.management.setLaunchType is not available in this browser|"
        "chrome.management.uninstallSelf is not available in this browser|warnings 0");
    struct Case { const char *name; int kind; };
    const QList<Case> cases{ {"qt worker", 0}, {"qt worker with the other root", 1}, {"qt page", 2},
                             {"edge worker", 3}, {"edge page", 4} };
    foreach(const Case &c, cases){
        QJSEngine engine;
        switch(c.kind){
        case 0: Make(&engine, false, Fetching() + engines); break;
        case 1: Make(&engine, false, Fetching() + TheOtherRoot() + engines); break;
        case 2: MakePage(&engine, Fetching() + engines); break;
        case 3: MakeEdgeWorker(&engine, engines); break;
        default: MakeEdgePage(&engine, engines); break;
        }
        engine.evaluate(calls);
        Settle(&engine);
        engine.evaluate(QStringLiteral("runTimers();"));
        QTRY_COMPARE_WITH_TIMEOUT(Value(&engine, QStringLiteral("outs.slice().sort().join('|')")), said, 2000);
        QVERIFY2(Value(&engine, QStringLiteral("managed.join()")) == QStringLiteral("warnings,getAll,onEnabled"),
                 qPrintable(QString::fromLatin1(c.name) + QStringLiteral(": ") + Value(&engine, QStringLiteral("managed.join()"))));
    }
}

void tst_cdpshims::theEnginesIdleStateIsToldByTheWorker(){
    const QString engines = QStringLiteral(R"js(
        var idleState = 'active', idleAsked = [], idleHeld = [], holdIdle = false, idleFails = '';
        // the one-second look, and the ten seconds after which a question is given up on.
        function tick(){ var due = timers.filter(function(t){ return t.ms === 1000; });
                         timers = timers.filter(function(t){ return t.ms !== 1000; }); due.forEach(function(t){ t.f(); }); return due.length; }
        function giveUpIdle(){ var due = timers.filter(function(t){ return t.ms === 10000; });
                               timers = timers.filter(function(t){ return t.ms !== 10000; }); due.forEach(function(t){ t.f(); }); }
        function looks(){ return timers.map(function(t){ return t.ms; }).sort(function(a, b){ return a - b; }).join(); }
        var enginesIdle = {
            queryState: function(seconds, callback){
                idleAsked.push(seconds);
                if (idleFails === 'throw') throw new TypeError('No matching signature.');
                if (idleFails === 'lastError') { chrome.runtime.lastError = { message: 'no' }; try { callback(undefined); } finally { delete chrome.runtime.lastError; } return undefined; }
                var state = idleState;
                if (holdIdle) { idleHeld.push(function(){ callback(state); }); return undefined; }
                callback(state); return undefined;
            },
            setDetectionInterval: function(n){ if (typeof n !== 'number') throw new TypeError('No matching signature.'); idleAsked.push('interval ' + n); },
            onStateChanged: { addListener: function(){ idleAsked.push('the engine\'s onStateChanged'); }, removeListener: function(){},
                              hasListener: function(){ return false; }, hasListeners: function(){ return false; } },
            IdleState: { ACTIVE: 'active', IDLE: 'idle', LOCKED: 'locked' }
        };
        chrome.idle = enginesIdle;
        var told = [];
        function heard(s){ told.push(s); }
    )js");
    QJSEngine worker;
    Make(&worker, false, engines + TheOtherRoot());
    QCOMPARE(Value(&worker, QStringLiteral("[chrome.idle.onStateChanged === enginesIdle.onStateChanged, browser.idle === chrome.idle,"
                                           " chrome.idle.IdleState.IDLE, looks()].join(' ')")),
             QStringLiteral("false true idle "));
    worker.evaluate(QStringLiteral(
        "var thrower = function(){ throw new Error('a listener which throws'); };"
        "chrome.idle.onStateChanged.addListener(thrower);"
        "chrome.idle.onStateChanged.addListener(heard);"));
    QCOMPARE(Value(&worker, QStringLiteral("looks()")), QStringLiteral("1000"));
    QCOMPARE(Value(&worker, QStringLiteral("tick() + ' ' + idleAsked.join() + ' [' + told.join() + '] ' + looks()")), QStringLiteral("1 60 [] 1000"));
    QCOMPARE(Value(&worker, QStringLiteral("idleState = 'idle'; tick(); tick(); told.join()")), QStringLiteral("idle"));
    QCOMPARE(Value(&worker, QStringLiteral("idleState = 'locked'; tick(); idleState = 'asleep'; tick(); idleState = 'active'; tick(); told.join()")),
             QStringLiteral("idle,locked,active"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "idleAsked = []; chrome.idle.setDetectionInterval(30); tick(); chrome.idle.setDetectionInterval(5); tick();"
                 "chrome.idle.setDetectionInterval(99999); tick(); idleAsked.join()")),
             QStringLiteral("interval 30,30,interval 5,15,interval 99999,14400"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "var threw = ''; try { chrome.idle.setDetectionInterval('x'); } catch (e) { threw = e.name; }"
                 "idleAsked = []; tick(); threw + ' ' + idleAsked.join()")),
             QStringLiteral("TypeError 14400"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "idleState = 'idle'; idleFails = 'throw'; tick(); idleFails = 'lastError'; tick(); idleFails = '';"
                 "told.join() + ' ' + looks()")),
             QStringLiteral("idle,locked,active 1000"));
    QCOMPARE(Value(&worker, QStringLiteral("tick(); told.join()")), QStringLiteral("idle,locked,active,idle"));
    QCOMPARE(Value(&worker, QStringLiteral("told = []; holdIdle = true; idleState = 'active'; tick(); looks() + ' ' + idleHeld.length")),
             QStringLiteral("10000 1"));
    QCOMPARE(Value(&worker, QStringLiteral("giveUpIdle(); looks()")), QStringLiteral("1000"));
    QCOMPARE(Value(&worker, QStringLiteral("holdIdle = false; idleState = 'locked'; tick(); idleHeld.shift()(); told.join() + ' ' + looks()")),
             QStringLiteral("locked 1000"));

    QCOMPARE(Value(&worker, QStringLiteral(
                 "chrome.idle.onStateChanged.removeListener(thrower); chrome.idle.onStateChanged.removeListener(heard);"
                 "var armed = looks(); idleAsked = []; tick(); armed + '/' + chrome.idle.onStateChanged.hasListeners() + '/' + idleAsked.length")),
             QStringLiteral("/false/0"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "told = []; holdIdle = true; idleState = 'active'; chrome.idle.onStateChanged.addListener(heard); tick();"
                 "chrome.idle.onStateChanged.removeListener(heard); idleHeld.shift()(); holdIdle = false; told.length + ' ' + looks()")),
             QStringLiteral("0 "));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.idle.onStateChanged.addListener(heard); tick(); told.join()")), QStringLiteral("active"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "told = []; holdIdle = true; idleState = 'idle'; tick();"
                 "chrome.idle.onStateChanged.removeListener(heard); chrome.idle.onStateChanged.addListener(heard);"
                 "var armed = looks(); tick(); armed + ' ' + looks() + ' ' + idleHeld.length")),
             QStringLiteral("10000 10000 1"));
    QCOMPARE(Value(&worker, QStringLiteral("holdIdle = false; idleHeld.shift()(); told.join() + ' ' + looks()")), QStringLiteral("idle 1000"));
    QCOMPARE(Value(&worker, QStringLiteral(
                 "enginesIdle.queryState = function(){ return Promise.resolve('locked'); }; tick(); 'asked'")), QStringLiteral("asked"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("told.join()")), QStringLiteral("idle,locked"));

    QJSEngine late;
    Make(&late, false, engines + QStringLiteral("idleState = 'idle';"));
    QCOMPARE(Value(&late, QStringLiteral("chrome.idle.onStateChanged.addListener(heard); tick(); tick(); told.join()")), QStringLiteral("idle"));

    QJSEngine edge;
    MakeEdgeWorker(&edge, engines);
    QCOMPARE(Value(&edge, QStringLiteral("chrome.idle.onStateChanged.addListener(heard);"
                                         "(chrome.idle.onStateChanged === enginesIdle.onStateChanged) + ' ' + idleAsked.join() + ' ' + tick()")),
             QStringLiteral("true the engine's onStateChanged 0"));
    QJSEngine page;
    MakePage(&page, Fetching() + engines);
    QCOMPARE(Value(&page, QStringLiteral("chrome.idle.onStateChanged.addListener(heard);"
                                         "(chrome.idle.onStateChanged === enginesIdle.onStateChanged) + ' ' + tick()")),
             QStringLiteral("true 0"));
    QJSEngine none;
    Make(&none, false, engines + QStringLiteral("delete chrome.idle;"));
    QCOMPARE(Value(&none, QStringLiteral("chrome.idle.onStateChanged.addListener(heard); tick() + ' ' + idleAsked.length")),
             QStringLiteral("0 0"));
}

void tst_cdpshims::theSearchTheApplicationOpensIsAskedOfIt(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true });"
        "chrome.search.query({ disposition: 'NEW_TAB', text: 'a b' })"
        "  .then(function(v){ out = 'resolved ' + (v === undefined ? 'nothing' : JSON.stringify(v)); },"
        "        function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved nothing"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[0].url + ' ' + fetched[0].options.method + ' ' + ('body' in fetched[0].options)")),
             QStringLiteral("vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"search.query\",\"args\":[{\"disposition\":\"NEW_TAB\",\"text\":\"a b\"}]}"));

    worker.evaluate(QStringLiteral(
        "var refused = 'pending', before = fetched.length;"
        "answers.push({ ok: false, error: 'chrome.search.query is not available in this browser' });"
        "chrome.search.query({ text: 'x' }).then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("refused")), QStringLiteral("chrome.search.query is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("(fetched.length - before) + ' ' + ") + LastCall()),
             QStringLiteral("1 {\"api\":\"search.query\",\"args\":[{\"text\":\"x\"}]}"));

    worker.evaluate(QStringLiteral(
        "var other = 'pending'; before = fetched.length;"
        "chrome.search.foo({ text: 'x' }).then(function(){ other = 'resolved'; }, function(e){ other = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("other")), QStringLiteral("chrome.search.foo is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - before")), QStringLiteral("0"));

    QJSEngine page;
    MakeKeyedPage(&page, Fetching());
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true });"
        "chrome.search.query({ text: 'a b' }).then(function(v){ out = 'resolved ' + (v === undefined ? 'nothing' : JSON.stringify(v)); },"
        "                                          function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("resolved nothing"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key']")), PageKey());
    QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"search.query\",\"args\":[{\"text\":\"a b\"}]}"));

    QJSEngine open;
    MakePage(&open, Fetching());
    open.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.search.query({ text: 'a b' }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("out")), QStringLiteral("chrome.search.query is not available in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("fetched.length")), QStringLiteral("0"));
}

void tst_cdpshims::theZoomOfATabIsAskedOfTheApplication(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: 1.25 });"
        "chrome.tabs.getZoom(12).then(function(v){ out = String(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("1.25"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.getZoom\",\"args\":[12]}"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true });"
        "chrome.tabs.setZoom(12, 1.5).then(function(v){ out = String(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("undefined"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.setZoom\",\"args\":[12,1.5]}"));
    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: false, error: 'Zoom value is out of range.' });"
        "chrome.tabs.setZoom(12, 9).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Zoom value is out of range."));
}

void tst_cdpshims::theTabsTheApplicationCopiesAndMovesAreAskedOfIt(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var senders = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "answers.push({ ok: true, value: { id: 13, index: 9 } }, { ok: true, value: { id: 12, index: 1 } },"
        "             { ok: true, value: { id: 15, index: 2 } });"
        "['c1', 'c2', 'c3'].forEach(function(n){"
        "  listeners[0]({ __vanillaEnvelope: 1, from: { nonce: n + '000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "               { id: chrome.runtime.id, url: 'u' }, function(){}); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("3"));
    const QString where = QStringLiteral("senders.map(function(s){ return s.tab.id + '@' + s.tab.index; }).join()");
    QCOMPARE(Value(&worker, where), QStringLiteral("13@9,12@1,15@2"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: { id: 13, index: 2, windowId: 1, active: true } });"
        "chrome.tabs.duplicate(12).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")),
                 QStringLiteral("{\"id\":13,\"index\":2,\"windowId\":1,\"active\":true}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[fetched.length - 1].url + ' ' + fetched[fetched.length - 1].options.method"
                                           " + ' ' + ('body' in fetched[fetched.length - 1].options)")),
             QStringLiteral("vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.duplicate\",\"args\":[12]}"));
    QCOMPARE(Value(&worker, where), QStringLiteral("13@2,12@1,15@2"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: { id: 12, index: 0 } });"
        "chrome.tabs.move(12, { index: 0 }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("{\"id\":12,\"index\":0}"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.move\",\"args\":[12,{\"index\":0}]}"));
    QCOMPARE(Value(&worker, where), QStringLiteral("13@2,12@0,15@2"));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, index: 4 }, { id: 15, index: 5 }] });"
        "chrome.tabs.move([12, 15], { index: 4 }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":4},{\"id\":15,\"index\":5}]"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"tabs.move\",\"args\":[[12,15],{\"index\":4}]}"));
    QCOMPARE(Value(&worker, where), QStringLiteral("13@2,12@4,15@5"));

    worker.evaluate(QStringLiteral(
        "var gone = 'pending', elsewhere = 'pending';"
        "answers.push({ ok: false, error: 'No tab with id: 99.' }, { ok: false, error: 'No window with id: 7.' });"
        "chrome.tabs.duplicate(99).then(function(){ gone = 'resolved'; }, function(e){ gone = e.message; });"
        "chrome.tabs.move(12, { index: 0, windowId: 7 }).then(function(){ elsewhere = 'resolved'; }, function(e){ elsewhere = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("gone")), QStringLiteral("No tab with id: 99."));
    QTRY_COMPARE(Value(&worker, QStringLiteral("elsewhere")), QStringLiteral("No window with id: 7."));

    QJSEngine open;
    MakePage(&open, Fetching());
    open.evaluate(QStringLiteral(
        "var copied = 'pending', moved = 'pending';"
        "chrome.tabs.duplicate(12).then(function(){ copied = 'resolved'; }, function(e){ copied = e.message; });"
        "chrome.tabs.move(12, { index: 0 }).then(function(){ moved = 'resolved'; }, function(e){ moved = e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("copied")), QStringLiteral("chrome.tabs.duplicate is not available in this browser"));
    QTRY_COMPARE(Value(&open, QStringLiteral("moved")), QStringLiteral("chrome.tabs.move is not available in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("fetched.length")), QStringLiteral("0"));
}

void tst_cdpshims::theClosedTabTheApplicationPutsBackIsAskedOfIt(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var senders = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "answers.push({ ok: true, value: { id: 13, index: 5 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: 'd1000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.id + '@' + senders[0].tab.index")), QStringLiteral("13@5"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: { lastModified: 1, tab: { id: 13, index: 2, url: 'https://a.example/', active: true } } });"
        "chrome.sessions.restore(null).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")),
                 QStringLiteral("{\"lastModified\":1,\"tab\":{\"id\":13,\"index\":2,\"url\":\"https://a.example/\",\"active\":true}}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[fetched.length - 1].url + ' ' + fetched[fetched.length - 1].options.method"
                                           " + ' ' + ('body' in fetched[fetched.length - 1].options)")),
             QStringLiteral("vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"sessions.restore\",\"args\":[null]}"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.index")), QStringLiteral("5"));

    worker.evaluate(QStringLiteral(
        "var empty = 'pending';"
        "answers.push({ ok: false, error: 'There are no recently closed tabs.' });"
        "chrome.sessions.restore().then(function(){ empty = 'resolved'; }, function(e){ empty = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("empty")), QStringLiteral("There are no recently closed tabs."));
    QCOMPARE(Value(&worker, LastCall()),
             QStringLiteral("{\"api\":\"sessions.restore\",\"args\":[]}"));

    QCOMPARE(Value(&worker, QStringLiteral("chrome.sessions.MAX_SESSION_RESULTS + '/' + typeof chrome.sessions.MAX_SESSION_RESULTS"
                                           " + '/' + ('MAX_SESSION_RESULTS' in chrome.sessions)")),
             QStringLiteral("25/number/true"));

    worker.evaluate(QStringLiteral(
        "var closed = 'pending', before = fetched.length;"
        "chrome.sessions.getRecentlyClosed({ maxResults: 25 }).then(function(){ closed = 'resolved'; }, function(e){ closed = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("closed")), QStringLiteral("chrome.sessions.getRecentlyClosed is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length - before")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.sessions.onChanged.addListener + '/' + chrome.sessions.onChanged.hasListeners()")),
             QStringLiteral("function/false"));

    QJSEngine open;
    MakePage(&open, Fetching());
    open.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.sessions.restore(null).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("out")), QStringLiteral("chrome.sessions.restore is not available in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("fetched.length")), QStringLiteral("0"));
    QCOMPARE(Value(&open, QStringLiteral("chrome.sessions.MAX_SESSION_RESULTS")), QStringLiteral("25"));
}

void tst_cdpshims::aContentScriptLinksOnlyOnceSomebodyListens(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length + '/' + fetched.length + '/' + timers.length")),
             QStringLiteral("0/0/0/0"));
    QCOMPARE(Value(&page, QStringLiteral("Date.now()")), QStringLiteral("1000000"));

    page.evaluate(QStringLiteral(
        "replies.push({ linked: 1 });"
        "var got = [];"
        "var mine = function(m, s){ got.push(JSON.stringify(m) + ' from ' + s.id + ' tab=' + (s.tab === undefined)); return false; };"
        "chrome.runtime.onMessage.addListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length")), QStringLiteral("1/0"));
    QCOMPARE(Value(&page, QStringLiteral("sent[0][0].__vanillaEnvelope + ':' + sent[0][0].link + ':' + sent[0][0].from.frameId"
                                        " + ':' + sent[0][0].from.tabUrl + ':' + ('message' in sent[0][0])")),
             QStringLiteral("1:1:0:https://a.example/page:false"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].url + ' ' + (fetched[0].options.headers['X-Vanilla-Nonce'] === sent[0][0].from.nonce)")),
             QStringLiteral("vanilla-extension://host/bind true"));
    QCOMPARE(Value(&page, QStringLiteral("listeners.length + '/' + listeners.indexOf(mine)")), QStringLiteral("1/-1"));

    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].name")), QStringLiteral("__vanilla_link__"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(Object.keys(ports[0].posted[0].link).sort())")),
             QStringLiteral("[\"frameId\",\"fresh\",\"nonce\",\"ready\",\"readyFrom\",\"since\",\"tabUrl\"]"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].posted[0].link.nonce === sent[0][0].from.nonce")), QStringLiteral("true"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].posted[0].link.frameId + '/' + ports[0].posted[0].link.since")),
             QStringLiteral("0/1000000"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' in ' + timers[0].ms")), QStringLiteral("1 in 20000"));
}

void tst_cdpshims::aHiddenDocumentNeitherGreetsNorPings(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden';"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length")), QStringLiteral("0/1"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length + ' ' + fetched[0].url")), QStringLiteral("1 vanilla-extension://host/bind"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("0"));

    page.evaluate(QStringLiteral("ports[0].die();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' >= 45s ' + (timers[0].ms >= 45000)")), QStringLiteral("1 >= 45s true"));
    for(int i = 0; i < 8; i++){
        page.evaluate(QStringLiteral("runNextTimer(); ports[ports.length - 1].die();"));
        QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("1"));
    }
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length + '/' + warned.length")), QStringLiteral("0/9/0"));
}

void tst_cdpshims::thePingIsKeptOnlyWhileTheDocumentIsSeen(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    page.evaluate(QStringLiteral("runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' then ' + timers.length + ' in ' + timers[0].ms")),
             QStringLiteral("{\"ping\":1} then 1 in 20000"));
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].posted.length + ' posted, ' + timers.length + ' in ' + timers[0].ms")),
             QStringLiteral("3 posted, 1 in 20000"));
    page.evaluate(QStringLiteral("ports[0].dead = true; runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' in ' + (timers[0].ms >= 200 && timers[0].ms <= 1000)")),
             QStringLiteral("1 in true"));
}

void tst_cdpshims::everyTryDropsThePortBeforeIt(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 }, { linked: 1 });"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].disconnected + '/' + timers.length + ' in ' + timers[0].ms")),
             QStringLiteral("1/1 in 1000"));
    page.evaluate(QStringLiteral("runNextTimer();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].disconnected + '/' + ports[1].disconnected")), QStringLiteral("1/0"));
}

void tst_cdpshims::theTryingIsBoundedAndComesBackOnlyByTheRules(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    const QStringList waits = QStringList() << QStringLiteral("1000") << QStringLiteral("2000")
                                            << QStringLiteral("4000") << QStringLiteral("8000") << QStringLiteral("16000");
    for(int i = 0; i < waits.size(); i++){
        QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + ' in ' + (timers.length ? timers[0].ms : 0)")),
                     QStringLiteral("1 in ") + waits[i]);
        page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length + '/' + warned.length")), QStringLiteral("0/6/1"));
    QVERIFY2(Value(&page, QStringLiteral("warned[0]")).startsWith(QStringLiteral("Vanilla:")), qPrintable(Value(&page, QStringLiteral("warned[0]"))));

    page.evaluate(QStringLiteral("document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' in ' + (timers[0].ms >= 200 && timers[0].ms <= 1000)")), QStringLiteral("1 in true"));
    const QString waiting = QStringLiteral("timers.length + '/' + (timers.length > 0 && timers[0].ms !== 5000)");
    for(int i = 0; i < 6; i++){
        QTRY_COMPARE(Value(&page, waiting), QStringLiteral("1/true"));
        page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length")), QStringLiteral("0/12"));
    page.evaluate(QStringLiteral("document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length")), QStringLiteral("0/12"));
    page.evaluate(QStringLiteral("clock += 30000; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("1"));
    for(int i = 0; i < 6; i++){
        QTRY_COMPARE(Value(&page, waiting), QStringLiteral("1/true"));
        page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length")), QStringLiteral("0/18"));
    page.evaluate(QStringLiteral("pageshow();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("1"));

    QJSEngine held;
    Make(&held, true, Fetching() + Saying());
    held.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    for(int i = 0; i < 2; i++){
        QTRY_COMPARE(Value(&held, waiting), QStringLiteral("1/true"));
        held.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&held, waiting), QStringLiteral("1/true"));
    held.evaluate(QStringLiteral("replies.push({ linked: 1 }); runNextTimer();"));
    QTRY_COMPARE(Value(&held, QStringLiteral("ports.length")), QStringLiteral("1"));
    held.evaluate(QStringLiteral("ports[0].say({ linked: 1 }); clock += 25000; ports[0].die();"));
    QCOMPARE(Value(&held, QStringLiteral("timers.length + ' in ' + (timers[0].ms >= 200 && timers[0].ms <= 1000)")), QStringLiteral("1 in true"));
    for(int i = 0; i < 5; i++){
        QTRY_COMPARE(Value(&held, waiting), QStringLiteral("1/true"));
        held.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&held, QStringLiteral("timers.length + '/' + sent.length + '/' + warned.length")), QStringLiteral("0/9/1"));
}

void tst_cdpshims::whatTheWorkerAsksIsFoldedAndPostedBack(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral(
        "replies.push({ linked: 1 });"
        "var seen = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ seen.push(JSON.stringify(m) + ' from ' + s.id + ' tab=' + (s.tab === undefined)); return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));

    page.evaluate(QStringLiteral("ports[0].say({ id: 1, message: { n: 1 } });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"re\":1}"));
    QCOMPARE(Value(&page, QStringLiteral("seen.join()")),
             QStringLiteral("{\"n\":1} from abcdefghijklmnopabcdefghijklmnop tab=true"));

    page.evaluate(QStringLiteral(
        "chrome.runtime.onMessage.addListener(function(m, s, respond){"
        "  if (m.n === 2) { respond('at once'); return false; }"
        "  if (m.n === 3) return Promise.resolve('by promise');"
        "  if (m.n === 4) return true;"
        "  return false; });"));
    page.evaluate(QStringLiteral("ports[0].say({ id: 2, message: { n: 2 } });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"re\":2,\"value\":\"at once\"}"));
    page.evaluate(QStringLiteral("ports[0].say({ id: 3, message: { n: 3 } });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"re\":3,\"value\":\"by promise\"}"));
    page.evaluate(QStringLiteral("ports[0].say({ id: 4, message: { n: 4 } });"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("ports[0].posted.filter(function(m){ return m.re === 4; }).length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("ports[0].dead = true;"));
    QVERIFY(!page.evaluate(QStringLiteral("ports[0].say({ id: 5, message: { n: 5 } });")).isError());
}

void tst_cdpshims::theLastListenerLeavingClosesTheLink(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 }, { linked: 1 });"
                                 "var mine = function(){ return false; };"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].disconnected + '/' + timers.length + '/' + chrome.runtime.onMessage.hasListeners()")),
             QStringLiteral("1/0/false"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length")), QStringLiteral("2"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
}

void tst_cdpshims::aTimerWhichWasPutBackDoesNothingWithoutClearTimeout(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + QStringLiteral("clearTimeout = undefined;"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("sent.length")), QStringLiteral("1"));
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.map(function(t){ return t.ms; }).sort(function(a, b){ return a - b; }).join()")),
                 QStringLiteral("1000,5000"));
    page.evaluate(QStringLiteral("timers = timers.filter(function(t){ return t.ms === 5000; }); runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length + '/' + timers.length")), QStringLiteral("1/0/0"));

    QJSEngine again;
    Make(&again, true, Fetching() + Saying() + QStringLiteral("clearTimeout = undefined;"));
    again.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&again, QStringLiteral("timers.length")), QStringLiteral("2"));
    again.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&again, QStringLiteral("sent.length")), QStringLiteral("2"));

    QJSEngine pinging;
    Make(&pinging, true, Fetching() + Saying() + QStringLiteral("clearTimeout = undefined;"));
    pinging.evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                    "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&pinging, QStringLiteral("ports.length")), QStringLiteral("1"));
    pinging.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"
                                    "document.visibilityState = 'hidden'; document.onVisibility();"
                                    "document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&pinging, QStringLiteral("ports[0].posted.length + '/' + timers.filter(function(t){ return t.ms === 20000; }).length")),
             QStringLiteral("2/2"));
    pinging.evaluate(QStringLiteral("timers = timers.filter(function(t){ return t.ms === 20000; }).slice(0, 1); runTimers();"));
    Pump(&pinging);
    QCOMPARE(Value(&pinging, QStringLiteral("ports[0].posted.length + '/' + timers.length")), QStringLiteral("2/0"));
}

void tst_cdpshims::theDemuxIsToldToTheEngineAtOnceAndForGood(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length + '/' + chrome.runtime.onMessage.hasListeners()")), QStringLiteral("1/false"));
    worker.evaluate(QStringLiteral("var mine = function(){ return false; }; chrome.runtime.onMessage.addListener(mine);"
                                   "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(mine);"));
    QCOMPARE(Value(&worker, QStringLiteral("listeners.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("connects.length")), QStringLiteral("1"));
}

void tst_cdpshims::theGreetingIsAnsweredAtOnceAndNotHandedOn(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(QStringLiteral(
        "var got = [], answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(JSON.stringify(m)); return false; });"
        "answers.push({ ok: true, value: { id: 900, index: 0 } });"));
    const QString greeting = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: 'c1000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, link: 1 }");
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(JSON.stringify(v)); }))").arg(greeting)),
             QStringLiteral("undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.join() + '/' + got.length")), QStringLiteral("{\"linked\":1}/0"));
    QCOMPARE(Value(&worker, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"vanilla.tabOf\",\"args\":[\"c1000000000000000000000000000000\"]}"));
    Settle(&worker);
    const int asked = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(){});").arg(greeting));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked);
    worker.evaluate(QStringLiteral("listeners[0](%1, { id: 'somebodyelse', url: 'https://evil.example/' }, function(){});").arg(greeting));
    QVERIFY2(Value(&worker, QStringLiteral("got.join()")).contains(QStringLiteral("__vanillaEnvelope")), qPrintable(Value(&worker, QStringLiteral("got.join()"))));
}

void tst_cdpshims::theShimsOwnPortIsNotTheExtensionsToSee(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(QStringLiteral("var seenPorts = []; var watching = function(p){ seenPorts.push(p.name); };"
                                   "chrome.runtime.onConnect.addListener(watching);"));
    worker.evaluate(QStringLiteral("var ours = aPort('__vanilla_link__', { id: chrome.runtime.id, url: 'u' }); connects[0](ours);"));
    QCOMPARE(Value(&worker, QStringLiteral("seenPorts.join() + '/' + ours.disconnected")), QStringLiteral("/0"));
    worker.evaluate(QStringLiteral("var other = aPort('vimium', { id: chrome.runtime.id, url: 'u' }); connects[0](other);"
                                   "var alien = aPort('__vanilla_link__', { id: 'somebodyelsesextension' }); connects[0](alien);"));
    QCOMPARE(Value(&worker, QStringLiteral("seenPorts.join() + '/' + other.disconnected + alien.disconnected")),
             QStringLiteral("vimium,__vanilla_link__/00"));
    worker.evaluate(QStringLiteral("chrome.runtime.onConnect.removeListener(watching);"
                                   "var lonely = aPort('vimium', { id: chrome.runtime.id, url: 'u' }); connects[0](lonely);"));
    QCOMPARE(Value(&worker, QStringLiteral("lonely.disconnected + '/' + chrome.runtime.onConnect.hasListeners()")), QStringLiteral("1/false"));
}

void tst_cdpshims::aPortWhichNamesItselfBadlyIsNoLink(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "var bad = ["
        "  naming({ hello: 1 }),"
        "  naming({ link: {} }),"
        "  naming({ link: { nonce: 'zz000000000000000000000000000000', frameId: 0, tabUrl: 't', since: 1 } }),"
        "  naming({ link: { nonce: null, frameId: -1, tabUrl: 't', since: 1 } }),"
        "  naming({ link: { nonce: null, frameId: 1.5, tabUrl: 't', since: 1 } }),"
        "  naming({ link: { nonce: null, frameId: 0, tabUrl: 5, since: 1 } }),"
        "  naming({ link: { nonce: null, frameId: 0, tabUrl: 't' } })];"
        "var quiet = naming(undefined);"));
    QCOMPARE(Value(&worker, QStringLiteral("bad.map(function(p){ return p.disconnected; }).join()")), QStringLiteral("1,1,1,1,1,1,1"));
    QCOMPARE(Value(&worker, QStringLiteral("quiet.disconnected")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms === 5000; })")), QStringLiteral("true"));
    worker.evaluate(QStringLiteral("runTimers();"));
    QCOMPARE(Value(&worker, QStringLiteral("quiet.disconnected")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 1 }).then(function(f){ out = JSON.stringify(f); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[]"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("0"));
}

void tst_cdpshims::theWorkerSendsToEveryFrameOfATab(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } },"
        "             { ok: true, value: { id: 777, index: 0 } });"
        "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var sub = linking('a2000000000000000000000000000000', 5, 'https://a.example/frame', 200);"
        "var elsewhere = linking('a3000000000000000000000000000000', 0, 'https://b.example/', 300);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([top.last(), sub.last(), elsewhere.last()])")),
             QStringLiteral("[{\"linked\":1,\"url\":\"https://a.example/top\"},"
                            "{\"linked\":1,\"url\":\"https://a.example/frame\"},"
                            "{\"linked\":1,\"url\":\"https://b.example/\"}]"));

    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.tabs.sendMessage(555, { hello: 1 }).then(function(v){ out = 'resolved ' + JSON.stringify(v); }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([top.last(), sub.last(), elsewhere.posted.length])")),
             QStringLiteral("[{\"id\":1,\"message\":{\"hello\":1}},{\"id\":1,\"message\":{\"hello\":1}},1]"));
    worker.evaluate(QStringLiteral("sub.say({ re: 1, value: 'the frame' }); top.say({ re: 1, value: 'too late' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved \"the frame\""));

    worker.evaluate(QStringLiteral(
        "out = 'pending';"
        "chrome.tabs.sendMessage(555, { only: 1 }, { frameId: 5 }).then(function(v){ out = 'resolved ' + JSON.stringify(v); }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([top.last(), sub.last()])")),
             QStringLiteral("[{\"id\":1,\"message\":{\"hello\":1}},{\"id\":2,\"message\":{\"only\":1}}]"));
    worker.evaluate(QStringLiteral("sub.say({ re: 2 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));

    worker.evaluate(QStringLiteral("out = 'pending'; chrome.tabs.sendMessage(555, {}, { frameId: 9 }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Could not establish connection. Receiving end does not exist."));
    worker.evaluate(QStringLiteral("out = 'pending'; chrome.tabs.sendMessage(4242, {}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Could not establish connection. Receiving end does not exist."));
}

void tst_cdpshims::theWaysASendMessageEnds(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
        "var a = linking('b1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var b = linking('b2000000000000000000000000000000', 2, 'https://a.example/frame', 200);"
        "var out = 'pending';"
        "function ask(where, options){"
        "  out = 'pending';"
        "  var args = options === undefined ? [where, { q: 1 }] : [where, { q: 1 }, options];"
        "  chrome.tabs.sendMessage.apply(null, args).then(function(v){ out = 'resolved ' + (v === undefined ? 'nothing' : v); },"
        "                                                 function(e){ out = e.message; });"
        "}"));
    Settle(&worker);

    worker.evaluate(QStringLiteral("ask(5, { frameId: 0 }); a.say({ re: 1, value: 'mine' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved mine"));
    worker.evaluate(QStringLiteral("ask(5); b.say({ re: 2, value: 'not mine' });"));
    Pump(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("pending"));
    worker.evaluate(QStringLiteral("a.say({ re: 2, value: 'mine again' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved mine again"));
    worker.evaluate(QStringLiteral("ask(5); a.say({ re: 3, nobody: 1 }); b.say({ re: 2, nobody: 1 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Could not establish connection. Receiving end does not exist."));
    worker.evaluate(QStringLiteral("ask(5); a.say({ re: 4, nobody: 1 }); b.say({ re: 3 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved nothing"));
    worker.evaluate(QStringLiteral("ask(5, { frameId: 2 }); b.die();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("The message port closed before a response was received."));
    worker.evaluate(QStringLiteral("var frames = 'pending'; chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ frames = f.map(function(x){ return x.frameId; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("frames")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("ask(5, { documentId: 'abc' });"));
    QTRY_VERIFY(Value(&worker, QStringLiteral("out")).contains(QStringLiteral("documentId")));
    worker.evaluate(QStringLiteral(
        "var called = [];"
        "chrome.tabs.sendMessage(5, { q: 1 }, function(v){ called.push('got ' + v + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine')); });"
        "a.say({ re: 5, value: 'by callback' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called.join()")), QStringLiteral("got by callback fine"));
    worker.evaluate(QStringLiteral(
        "chrome.tabs.sendMessage(4242, { q: 1 }, function(v){ called.push('none ' + v + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR')); });"));
    QTRY_VERIFY(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms === 0; })")) == QStringLiteral("true"));
    worker.evaluate(QStringLiteral("runTimers();"));
    QCOMPARE(Value(&worker, QStringLiteral("called[1]")), QStringLiteral("none undefined Could not establish connection. Receiving end does not exist."));
}

void tst_cdpshims::getAllFramesTellsOfTheLinkedDocuments(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
        "var late = linking('d3000000000000000000000000000000', 9, 'https://a.example/late', 300);"
        "var early = linking('d2000000000000000000000000000000', 7, 'https://a.example/early', 200);"
        "var top = linking('d1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("var out = 'pending'; chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ out = JSON.stringify(f); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")),
                 QStringLiteral("[{\"frameId\":0,\"parentFrameId\":-1,\"url\":\"https://a.example/top\",\"errorOccurred\":false,"
                                "\"processId\":-1,\"documentLifecycle\":\"active\",\"frameType\":\"outermost_frame\"},"
                                "{\"frameId\":7,\"parentFrameId\":0,\"url\":\"https://a.example/early\",\"errorOccurred\":false,"
                                "\"processId\":-1,\"documentLifecycle\":\"active\",\"frameType\":\"sub_frame\"},"
                                "{\"frameId\":9,\"parentFrameId\":0,\"url\":\"https://a.example/late\",\"errorOccurred\":false,"
                                "\"processId\":-1,\"documentLifecycle\":\"active\",\"frameType\":\"sub_frame\"}]"));
    worker.evaluate(QStringLiteral("out = 'pending'; chrome.webNavigation.getAllFrames({ tabId: 4242 }).then(function(f){ out = JSON.stringify(f); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[]"));
    worker.evaluate(QStringLiteral("out = 'pending';"
                                   "var back = chrome.webNavigation.getAllFrames({ tabId: 5 }, function(f){ out = f.map(function(x){ return x.frameId; }).join(); });"));
    QCOMPARE(Value(&worker, QStringLiteral("String(back)")), QStringLiteral("undefined"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("0,7,9"));
    worker.evaluate(QStringLiteral("early.die(); out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ out = f.map(function(x){ return x.frameId; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("0,9"));
}

void tst_cdpshims::aLinkIsReachedByBothOfItsNumbers(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("Math.random = function(){ return 0.25; };"
                                   "var p = linking('e1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    DrainBefore(&worker, 1000);
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "function ask(where){ out = 'pending';"
        "  chrome.tabs.sendMessage(where, { q: 1 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; }); }"
        "ask(1342177280); p.say({ re: 1, value: 'by the provisional one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved by the provisional one"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 777, index: 0 } }); runNextTimer();"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("ask(777); p.say({ re: 2, value: 'by the certain one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved by the certain one"));
    worker.evaluate(QStringLiteral("ask(1342177280); p.say({ re: 3, value: 'and still by the provisional one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved and still by the provisional one"));
    worker.evaluate(QStringLiteral("ask(778);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Could not establish connection. Receiving end does not exist."));
    worker.evaluate(QStringLiteral("var frames = 'pending';"
                                   "Promise.all([chrome.webNavigation.getAllFrames({ tabId: 777 }), chrome.webNavigation.getAllFrames({ tabId: 1342177280 })])"
                                   "  .then(function(both){ frames = both.map(function(f){ return f.length; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("frames")), QStringLiteral("1,1"));
}

void tst_cdpshims::aSecondPortOfOneNonceRetiresTheFirst(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
        "var first = linking('f1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.tabs.sendMessage(5, { q: 1 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(first.last())")), QStringLiteral("{\"id\":1,\"message\":{\"q\":1}}"));
    worker.evaluate(QStringLiteral("var second = linking('f1000000000000000000000000000000', 0, 'https://a.example/top', 400);"));
    QCOMPARE(Value(&worker, QStringLiteral("first.disconnected + '/' + JSON.stringify(second.last())")),
             QStringLiteral("1/{\"linked\":1,\"url\":\"https://a.example/top\"}"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("The message port closed before a response was received."));
    worker.evaluate(QStringLiteral("var frames = 'pending'; chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ frames = String(f.length); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("frames")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("var one = linking(null, 3, 'https://a.example/a', 500);"
                                   "var two = linking(null, 3, 'https://a.example/b', 600);"));
    QCOMPARE(Value(&worker, QStringLiteral("one.disconnected + '/' + two.disconnected")), QStringLiteral("0/0"));
}

void tst_cdpshims::theFrameOfTheEnvelopeAndOfTheFramesAreOne(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying()
         + QStringLiteral("window.top = { location: { href: 'https://a.example/top' } };"
                          "location = { href: 'https://a.example/frame' };"));
    page.evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    const QString claim = Value(&page, QStringLiteral("JSON.stringify(ports[0].posted[0])"));
    const QString frame = Value(&page, QStringLiteral("String(sent[0][0].from.frameId)"));
    QVERIFY(frame != QStringLiteral("0"));

    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 42, index: 0 } });"
                                   "var p = naming(%1, 'https://a.example/frame');").arg(claim));
    Settle(&worker);
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 42 }).then(function(f){ out = f.map(function(x){ return x.frameId + ':' + x.parentFrameId + ':' + x.frameType; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), frame + QStringLiteral(":0:sub_frame"));
}

void tst_cdpshims::whatTheShimsAnswerThemselvesBeatsTheEnginesOwn(){
    QJSEngine worker;
    Make(&worker, false, Fetching() + QStringLiteral(
        "var navCalls = [];"
        "var enginesNav = { getAllFrames: function(){ navCalls.push('getAllFrames'); return Promise.reject(new Error('the engine\\'s own')); },"
        "                   getFrame: function(){ navCalls.push('getFrame'); return Promise.reject(new Error('the engine\\'s own')); },"
        "                   onCommitted: { addListener: function(){} } };"
        "chrome.webNavigation = enginesNav;"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.webNavigation === enginesNav")), QStringLiteral("false"));
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 1 }).then(function(f){ out = JSON.stringify(f); }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[]"));
    QCOMPARE(Value(&worker, QStringLiteral("navCalls.join() + '/' + fetched.length")), QStringLiteral("/0"));
    worker.evaluate(QStringLiteral("out = 'pending'; chrome.webNavigation.getFrame({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("chrome.webNavigation.getFrame is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("navCalls.join()")), QString());
    QCOMPARE(Value(&worker, QStringLiteral("['getAllFrames', 'getFrame', 'onCommitted'].map(function(k){ return k in chrome.webNavigation; }).join()")),
             QStringLiteral("true,true,true"));
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.tabs.sendMessage")), QStringLiteral("function"));
}

void tst_cdpshims::theNavigationEventsAreOursEvenWhereTheEngineHasThem(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral(
        "var engineHeard = [];"
        "function enginesEvent(name){ return { addListener: function(){ engineHeard.push(name); },"
        "                                      removeListener: function(){}, hasListener: function(){ return false; },"
        "                                      hasListeners: function(){ return false; } }; }"
        "var enginesNav = { getAllFrames: function(){ return Promise.reject(new Error('the engine\\'s own')); },"
        "                   onCommitted: enginesEvent('onCommitted'),"
        "                   onHistoryStateUpdated: enginesEvent('onHistoryStateUpdated'),"
        "                   onReferenceFragmentUpdated: enginesEvent('onReferenceFragmentUpdated') };"
        "chrome.webNavigation = enginesNav;"));
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var log = [], mine = function(d){ log.push(d.url); };"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(mine);"
        "chrome.webNavigation.onReferenceFragmentUpdated.addListener(function(d){ log.push('#' + d.url); });"
        "var p = linking('61000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral(
                 "(chrome.webNavigation.onHistoryStateUpdated === enginesNav.onHistoryStateUpdated)"
                 " + '/' + (chrome.webNavigation.onReferenceFragmentUpdated === enginesNav.onReferenceFragmentUpdated)"
                 " + '/' + engineHeard.join() + '/' + chrome.webNavigation.onHistoryStateUpdated.hasListener(mine)")),
             QStringLiteral("false/false//true"));
    worker.evaluate(QStringLiteral("p.say({ nav: { kind: 'history', url: 'https://a.example/top/next' } });"
                                   "p.say({ nav: { kind: 'fragment', url: 'https://a.example/top/next#x' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join()")),
             QStringLiteral("https://a.example/top/next,#https://a.example/top/next#x"));
    worker.evaluate(QStringLiteral("var far = function(){}; chrome.webNavigation.onCommitted.addListener(far);"));
    QCOMPARE(Value(&worker, QStringLiteral("(chrome.webNavigation.onCommitted === enginesNav.onCommitted) + '/' + engineHeard.join()"
                                           " + '/' + chrome.webNavigation.onCommitted.hasListener(far)")),
             QStringLiteral("false//true"));
}

void tst_cdpshims::aPortWhichDiedBeforeItLinkedIsStillAFailure(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("for (var i = 0; i < 12; i++) replies.push({ linked: 1 });"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    for(int i = 0; i < 6; i++){
        QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QString::number(i + 1));
        page.evaluate(QStringLiteral("ports[ports.length - 1].die();"));
        if(i < 5) page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + ports.length + '/' + sent.length + '/' + warned.length")),
                 QStringLiteral("0/6/6/1"));
}

void tst_cdpshims::anAnswerWhichWillNotGoOverThePortLeavesTheGateOpen(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral(
        "replies.push({ linked: 1 });"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ respond('will not go'); return false; });"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ respond('the real answer'); return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 }); ports[0].throwOn = 'will not go';"));
    QVERIFY(!page.evaluate(QStringLiteral("ports[0].say({ id: 1, message: {} });")).isError());
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")),
             QStringLiteral("{\"re\":1,\"value\":\"the real answer\"}"));
    QCOMPARE(Value(&page, QStringLiteral("cried.length >= 1")), QStringLiteral("true"));
}

void tst_cdpshims::aDocumentHiddenAfterItGaveUpTriesThePortAgain(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    const QString waiting = QStringLiteral("timers.length + '/' + (timers.length > 0 && timers[0].ms !== 5000)");
    for(int i = 0; i < 5; i++){
        QTRY_COMPARE(Value(&page, waiting), QStringLiteral("1/true"));
        page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + ports.length + '/' + sent.length + '/' + warned.length")), QStringLiteral("0/0/6/1"));

    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' >= 45s ' + (timers[0].ms >= 45000)")), QStringLiteral("1 >= 45s true"));
    const int greeted = Value(&page, QStringLiteral("sent.length")).toInt();
    page.evaluate(QStringLiteral("runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("ports.length + '/' + (sent.length - %1)").arg(greeted)), QStringLiteral("1/0"));
}

void tst_cdpshims::aPortOpenedWithNoGreetingIsNotWaitedOutOnceSeen(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden';"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length")), QStringLiteral("0/1"));
    page.evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                 "document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].disconnected + '/' + timers.length + ' in ' + (timers[0].ms >= 200 && timers[0].ms <= 1000)")),
             QStringLiteral("1/1 in true"));
    page.evaluate(QStringLiteral("runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length")), QStringLiteral("1"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral("runNextTimer();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + ' in ' + timers[0].ms")), QStringLiteral("1 in 1000"));
}

void tst_cdpshims::aDocumentWhoseVisibilityCannotBeReadIsNotSeen(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying()
         + QStringLiteral("document = { get visibilityState(){ throw new Error('not for you'); }, addEventListener: function(){} };"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length")), QStringLiteral("0/1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + '/' + ports[0].posted.length")), QStringLiteral("0/1"));
}

void tst_cdpshims::tabZeroReachesNobody(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("Math.random = function(){ return 0.25; }; answers.push('HELD');"
                                   "var hung = linking('a1000000000000000000000000000000', 0, 'https://a.example/hung', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("var slow = linking('a2000000000000000000000000000000', 1, 'https://a.example/slow', 200);"));
    DrainBefore(&worker, 1000);
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 900, index: 0 } });"
                                   "var named = linking('a3000000000000000000000000000000', 2, 'https://a.example/named', 300);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("[hung, slow, named].map(function(p){ return p.last().linked + ':' + p.last().url; }).join()")),
             QStringLiteral("1:https://a.example/hung,1:https://a.example/slow,1:https://a.example/named"));

    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.tabs.sendMessage(0, { q: 1 }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Could not establish connection. Receiving end does not exist."));
    QCOMPARE(Value(&worker, QStringLiteral("hung.posted.length + '/' + slow.posted.length + '/' + named.posted.length")),
             QStringLiteral("1/1/1"));
    worker.evaluate(QStringLiteral("var frames = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 0 }).then(function(f){ frames = JSON.stringify(f); }, function(e){ frames = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("frames")), QStringLiteral("[]"));

    worker.evaluate(QStringLiteral("out = 'pending';"
                                   "chrome.tabs.sendMessage(900, { q: 2 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; });"
                                   "named.say({ re: 1, value: 'the named one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved the named one"));
    worker.evaluate(QStringLiteral("out = 'pending';"
                                   "chrome.tabs.sendMessage(1342177280, { q: 3 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; });"
                                   "slow.say({ re: 1, value: 'the provisional one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved the provisional one"));
}

void tst_cdpshims::aDocumentWhichNeverAnswersDoesNotGrowItsTable(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 5, index: 0 } });"
                                   "var a = linking('c1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "var outs = [];"
        "for (var i = 0; i < 256; i++) chrome.tabs.sendMessage(5, { q: i }).then(function(v){ outs.push('resolved ' + v); }, function(e){ outs.push(e.message); });"));
    QCOMPARE(Value(&worker, QStringLiteral("a.posted.length + '/' + outs.length")), QStringLiteral("257/0"));
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.tabs.sendMessage(5, { q: 'one too many' }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("The message port closed before a response was received."));
    QCOMPARE(Value(&worker, QStringLiteral("a.posted.length")), QStringLiteral("257"));
    worker.evaluate(QStringLiteral("a.say({ re: 1, value: 'at last' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("outs.join().indexOf('at last') >= 0")), QStringLiteral("true"));
    worker.evaluate(QStringLiteral("out = 'pending';"
                                   "chrome.tabs.sendMessage(5, { q: 'room now' }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&worker, QStringLiteral("a.posted.length")), QStringLiteral("258"));
}

void tst_cdpshims::aValueWhoseThenThrowsIsTheListenersFailure(){
    const QString odd = QStringLiteral("function(){ return { get then(){ throw new Error('no then for you'); } }; }");

    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 }); chrome.runtime.onMessage.addListener(%1);").arg(odd));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    QVERIFY(!page.evaluate(QStringLiteral("ports[0].say({ id: 1, message: {} });")).isError());
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"re\":1}"));

    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(QStringLiteral("var answered = []; chrome.runtime.onMessage.addListener(%1);").arg(odd));
    QCOMPARE(Value(&worker, QStringLiteral("String(listeners[0]({ handler: 1 }, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(String(v)); }))")),
             QStringLiteral("undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.length")), QStringLiteral("0"));
}

void tst_cdpshims::whatIsAskedForWronglyIsSaidSo(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
                                   "var top = linking('d1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
                                   "var sub = linking('d2000000000000000000000000000000', 3, 'https://a.example/sub', 200);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "var outs = [];"
        "function ask(options){ chrome.tabs.sendMessage(5, { q: 1 }, options).then(function(){ outs.push('resolved'); }, function(e){ outs.push(e.message); }); }"
        "ask({ frameId: '0' }); ask({ frameId: 1.5 }); ask({ frameId: -1 }); ask({ frameId: {} });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("outs.length")), QStringLiteral("4"));
    QVERIFY2(Value(&worker, QStringLiteral("outs.every(function(m){ return m.indexOf('frameId must be') >= 0; })")) == QStringLiteral("true"),
             qPrintable(Value(&worker, QStringLiteral("outs.join(' | ')"))));
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + '/' + sub.posted.length")), QStringLiteral("1/1"));
    worker.evaluate(QStringLiteral("outs = []; ask({ frameId: 3 });"));
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + '/' + sub.posted.length")), QStringLiteral("1/2"));

    worker.evaluate(QStringLiteral(
        "var frames = [];"
        "function look(details){ chrome.webNavigation.getAllFrames(details).then(function(f){ frames.push(JSON.stringify(f)); }, function(e){ frames.push(e.message); }); }"
        "look({}); look({ tabId: '5' }); look(); look({ tabId: 5 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("frames.length")), QStringLiteral("4"));
    QCOMPARE(Value(&worker, QStringLiteral("frames.slice(0, 3).every(function(m){ return m.indexOf('a tabId is required') >= 0; })")),
             QStringLiteral("true"));
    QVERIFY2(Value(&worker, QStringLiteral("frames[3]")).contains(QStringLiteral("outermost_frame")), qPrintable(Value(&worker, QStringLiteral("frames[3]"))));
    worker.evaluate(QStringLiteral("var called = [];"
                                   "chrome.webNavigation.getAllFrames({}, function(f){ called.push(String(f) + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR')); });"));
    QTRY_VERIFY(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms === 0; })")) == QStringLiteral("true"));
    worker.evaluate(QStringLiteral("runTimers();"));
    QVERIFY2(Value(&worker, QStringLiteral("called.join()")).contains(QStringLiteral("a tabId is required")),
             qPrintable(Value(&worker, QStringLiteral("called.join()"))));

    worker.evaluate(QStringLiteral("outs = []; frames = [];"
                                   "chrome.tabs.sendMessage(0, { q: 1 }).then(function(){ outs.push('resolved'); }, function(e){ outs.push(e.message); });"
                                   "look({ tabId: 0 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("outs.join() + ' / ' + frames.join()")),
                 QStringLiteral("Could not establish connection. Receiving end does not exist. / []"));
}

void tst_cdpshims::aDocumentWhichGaveUpKeepsNothingWhenItIsSeenAgain(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    const QString waiting = QStringLiteral("timers.length + '/' + (timers.length > 0 && timers[0].ms !== 5000)");
    for(int i = 0; i < 5; i++){
        QTRY_COMPARE(Value(&page, waiting), QStringLiteral("1/true"));
        page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length + '/' + warned.length")), QStringLiteral("0/6/1"));
    page.evaluate(QStringLiteral("document.onVisibility();"));
    for(int i = 0; i < 6; i++){
        QTRY_COMPARE(Value(&page, waiting), QStringLiteral("1/true"));
        page.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length")), QStringLiteral("0/12"));

    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' >= 45s ' + (timers[0].ms >= 45000)")), QStringLiteral("1 >= 45s true"));
    page.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + '/' + sent.length + '/' + ports.length")), QStringLiteral("0/12/0"));

    QJSEngine stale;
    Make(&stale, true, Fetching() + Saying() + QStringLiteral("clearTimeout = undefined;"));
    stale.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    RunAll(&stale);
    QTRY_COMPARE(Value(&stale, QStringLiteral("warned.length")), QStringLiteral("1"));
    stale.evaluate(QStringLiteral("document.onVisibility();"));
    RunAll(&stale);
    const int greeted = Value(&stale, QStringLiteral("sent.length")).toInt();
    stale.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"));
    stale.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility();"));
    RunAll(&stale);
    QCOMPARE(Value(&stale, QStringLiteral("sent.length")).toInt(), greeted);
    QCOMPARE(Value(&stale, QStringLiteral("ports.length")), QStringLiteral("0"));
}

void tst_cdpshims::aDocumentWhichConnectsAgainKeepsBothOfItsNumbers(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("Math.random = function(){ return 0.25; };"
                                   "var first = linking('e1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    DrainBefore(&worker, 1000);
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 777, index: 0 } }); runNextTimer();"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("var second = linking('e1000000000000000000000000000000', 0, 'https://a.example/top', 400);"));
    QCOMPARE(Value(&worker, QStringLiteral("first.disconnected + '/' + JSON.stringify(second.last())")),
             QStringLiteral("1/{\"linked\":1,\"url\":\"https://a.example/top\"}"));
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "function ask(where){ out = 'pending';"
        "  chrome.tabs.sendMessage(where, { q: 1 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; }); }"));
    worker.evaluate(QStringLiteral("ask(777); second.say({ re: 1, value: 'by the certain one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved by the certain one"));
    worker.evaluate(QStringLiteral("ask(1342177280); second.say({ re: 2, value: 'by the provisional one' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved by the provisional one"));
    QCOMPARE(Value(&worker, QStringLiteral("first.posted.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("var frames = 'pending';"
                                   "Promise.all([chrome.webNavigation.getAllFrames({ tabId: 777 }), chrome.webNavigation.getAllFrames({ tabId: 1342177280 })])"
                                   "  .then(function(both){ frames = both.map(function(f){ return f.length + ':' + f.map(function(x){ return x.url; }).join(); }).join(' | '); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("frames")),
                 QStringLiteral("1:https://a.example/top | 1:https://a.example/top"));
}

void tst_cdpshims::aTabIdWhichIsNoWholeNumberIsNoAnswer(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = []; Math.random = function(){ return 0.25; };"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(s.tab.id + '@' + s.tab.index); return false; });"
        "answers.push({ ok: true, value: { id: 555.5, index: 0 } },"
        "             { ok: true, value: { id: Infinity, index: 0 } },"
        "             { ok: true, value: { id: NaN, index: 0 } },"
        "             { ok: true, value: { id: 555, index: 0.5 } },"
        "             { ok: true, value: { id: 555, index: Infinity } },"
        "             { ok: true, value: { id: 555.5, index: 0 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: 'e5000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    DrainBefore(&worker, 1000);
    QCOMPARE(Value(&worker, QStringLiteral("got.join()")), QStringLiteral("1342177280@0"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("6"));
}

void tst_cdpshims::aFullTableIsOneDocumentAndNotTheOthers(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
                                   "var full = linking('e6000000000000000000000000000000', 0, 'https://a.example/top', 100);"
                                   "var room = linking('e7000000000000000000000000000000', 2, 'https://a.example/sub', 200);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("function fill(frame){ for (var i = 0; i < 256; i++)"
                                   "  chrome.tabs.sendMessage(5, { q: i }, { frameId: frame }).then(function(){}, function(){}); }"
                                   "fill(0);"));
    QCOMPARE(Value(&worker, QStringLiteral("full.posted.length + '/' + room.posted.length")), QStringLiteral("257/1"));

    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.tabs.sendMessage(5, { q: 'both' }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&worker, QStringLiteral("full.posted.length + '/' + room.posted.length")), QStringLiteral("257/2"));
    worker.evaluate(QStringLiteral("room.say({ re: 1, value: 'the one with room' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved the one with room"));

    worker.evaluate(QStringLiteral("fill(2);"));
    QCOMPARE(Value(&worker, QStringLiteral("full.posted.length + '/' + room.posted.length")), QStringLiteral("257/258"));
    worker.evaluate(QStringLiteral("out = 'pending';"
                                   "chrome.tabs.sendMessage(5, { q: 'nobody' }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("Could not establish connection. Receiving end does not exist."));
    QCOMPARE(Value(&worker, QStringLiteral("full.posted.length + '/' + room.posted.length")), QStringLiteral("257/258"));
}

void tst_cdpshims::whenNoAnswerWillGoOverThePortTheGateIsClosedWithNothing(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral(
        "replies.push({ linked: 1 });"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ respond('will not go'); return false; });"
        "chrome.runtime.onMessage.addListener(function(m, s, respond){ respond('will not go'); return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 }); ports[0].throwOn = 'will not go';"));
    QVERIFY(!page.evaluate(QStringLiteral("ports[0].say({ id: 1, message: {} });")).isError());
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"re\":1}"));
    QCOMPARE(Value(&page, QStringLiteral("cried.length >= 2")), QStringLiteral("true"));
}

void tst_cdpshims::outsideAnExtensionTheContentShimDoesNothing(){
    QJSEngine engine;
    engine.evaluate(QStringLiteral("var self = this; var chrome = { runtime: {} };"));
    QCOMPARE(engine.evaluate(Cdp::ContentShim()).toString(), QStringLiteral("not an extension"));
    QCOMPARE(Value(&engine, QStringLiteral("String(self.__vanillaContentShim)")), QStringLiteral("undefined"));
    QJSEngine bare;
    bare.evaluate(QStringLiteral("var self = this;"));
    QCOMPARE(bare.evaluate(Cdp::ContentShim()).toString(), QStringLiteral("not an extension"));

    QJSEngine twice;
    Make(&twice, true);
    QCOMPARE(twice.evaluate(Cdp::ContentShim()).toString(), QStringLiteral("already"));
}

void tst_cdpshims::nothingIsAskedUntilTheExtensionListensForTheEvents(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length")), QStringLiteral("0/0"));

    worker.evaluate(QStringLiteral(
        "var log = [];"
        "answers.push('HELD');"
        "var mine = function(t){ log.push('created ' + JSON.stringify(t)); };"
        "chrome.tabs.onCreated.addListener(mine);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + fetched[0].url + ' ' + fetched[0].options.method")),
             QStringLiteral("1/vanilla-extension://host/call POST"));
    QCOMPARE(Value(&worker, QStringLiteral("call(0).api + '/' + JSON.stringify(call(0).args[1])")),
             QStringLiteral("vanilla.events/[\"tabs.onCreated\"]"));
    QVERIFY2(Value(&worker, QStringLiteral("/^[0-9a-f]{32}$/.test(call(0).args[0])")) == QStringLiteral("true"),
             qPrintable(Value(&worker, QStringLiteral("JSON.stringify(call(0).args)"))));
    QCOMPARE(Value(&worker, QStringLiteral("('signal' in fetched[0].options) + '/' + ('body' in fetched[0].options) + '/' + timers.length")),
             QStringLiteral("false/false/0"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key']")), QString(64, QLatin1Char('5')));

    worker.evaluate(QStringLiteral(
        "chrome.tabs.onRemoved.addListener(function(){});"
        "chrome.tabs.onUpdated.addListener(function(){});"
        "chrome.tabs.onActivated.addListener(function(){});"
        "chrome.windows.onFocusChanged.addListener(function(){});"
        "chrome.tabs.onCreated.addListener(mine);"
        "chrome.tabs.onCreated.addListener('not a function');"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("named.length + '/' + (named[0][0] === call(0).args[0]) + '/' + named[0][1].join()")),
             QStringLiteral("1/true/tabs.onCreated,tabs.onRemoved,tabs.onUpdated,tabs.onActivated,windows.onFocusChanged"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.tabs.onCreated.hasListeners() + '/' + chrome.tabs.onCreated.hasListener(mine)")),
             QStringLiteral("true/true"));
    QCOMPARE(Value(&worker, QStringLiteral("['addRules', 'removeRules', 'getRules'].map(function(k){ return typeof chrome.tabs.onCreated[k]; }).join()")),
             QStringLiteral("function,function,function"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 3, index: 0 }] }], order: [3] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")), QStringLiteral("created {\"id\":3,\"index\":0}"));
}

void tst_cdpshims::theEventsAreHandedToTheListenersOfThatName(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var log = [];"
        "function watch(name){ return function(){ log.push(name + ' ' + JSON.stringify(Array.prototype.slice.call(arguments))); }; }"
        "answers.push('HELD');"
        "chrome.tabs.onCreated.addListener(watch('created'));"
        "chrome.tabs.onRemoved.addListener(watch('removed'));"
        "chrome.tabs.onUpdated.addListener(watch('updated'));"
        "chrome.tabs.onActivated.addListener(watch('activated'));"
        "chrome.windows.onFocusChanged.addListener(watch('focus'));"
        "chrome.tabs.onMoved.addListener(watch('moved'));"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'tabs.onRemoved', args: [7, { windowId: 1, isWindowClosing: false }] },"
        "  { name: 'tabs.onCreated', args: [{ id: 8, index: 1 }] },"
        "  { name: 'tabs.onUpdated', args: [9, { url: 'https://a.example/' }, { id: 9, index: 2 }] },"
        "  { name: 'tabs.onActivated', args: [{ tabId: 8, windowId: 1 }] },"
        "  { name: 'windows.onFocusChanged', args: [-1] },"
        "  { name: 'tabs.onMoved', args: [8, { windowId: 1, toIndex: 0 }] },"
        "  { name: 'tabs.onCreated', args: 'not a list' },"
        "  { name: 5, args: [] }, null, 'nonsense'"
        "], order: [7, 8, 9] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("removed [7,{\"windowId\":1,\"isWindowClosing\":false}]"
                            " | created [{\"id\":8,\"index\":1}]"
                            " | updated [9,{\"url\":\"https://a.example/\"},{\"id\":9,\"index\":2}]"
                            " | activated [{\"tabId\":8,\"windowId\":1}]"
                            " | focus [-1]"
                            " | moved [8,{\"windowId\":1,\"toIndex\":0}]"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + (call(0).args[0] === call(1).args[0]) + '/' + timers.length"
                                           " + '/' + warned.length + '/' + cried.length")),
             QStringLiteral("2/true/0/0/0"));

    worker.evaluate(QStringLiteral(
        "log = [];"
        "chrome.tabs.onCreated.addListener(function(){ throw new Error('the extension\\'s own'); });"
        "chrome.tabs.onCreated.addListener(watch('second'));"
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 3, index: 0 }] }], order: [3] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("created [{\"id\":3,\"index\":0}] | second [{\"id\":3,\"index\":0}]"));
    QCOMPARE(Value(&worker, QStringLiteral("cried.length >= 1")), QStringLiteral("true"));
}

void tst_cdpshims::theListenersAreThoseOfTheMomentTheAnswerCame(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(QStringLiteral(
        "var log = [];"
        "answers.push('HELD');"
        "var late = function(){ log.push('late'); };"
        "var second = function(){ log.push('second'); };"
        "var changing = function(){ log.push('changing');"
        "  chrome.tabs.onActivated.addListener(late);"
        "  chrome.tabs.onCreated.removeListener(second); };"
        "chrome.tabs.onCreated.addListener(changing);"
        "chrome.tabs.onCreated.addListener(second);"
        "chrome.tabs.onActivated.addListener(function(){ log.push('there'); });"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] },"
        "  { name: 'tabs.onActivated', args: [{ tabId: 1, windowId: 1 }] }"
        "], order: [1] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join()")), QStringLiteral("changing,second,there"));
    worker.evaluate(QStringLiteral(
        "log = [];"
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onActivated', args: [{ tabId: 1, windowId: 1 }] }], order: [1] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join()")), QStringLiteral("there,late"));
}

void tst_cdpshims::theOrderOfAnAnswerWritesOverTheTableOfIndexes(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(QStringLiteral(
        "var senders = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "answers.push('HELD');"
        "chrome.tabs.onCreated.addListener(function(){});"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 3 } }, { ok: true, value: { id: 777, index: 4 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: 'f5000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: 'f6000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("2"));
    const QString where = QStringLiteral("senders.map(function(s){ return s.tab.id + '@' + s.tab.index; }).join()");
    QCOMPARE(Value(&worker, where), QStringLiteral("555@3,777@4"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onActivated', args: [{ tabId: 555, windowId: 1 }] }],"
        "                                  order: [555, 777] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, where), QStringLiteral("555@0,777@1"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onRemoved', args: [777, { windowId: 1, isWindowClosing: false }] }],"
        "                                  order: [555] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, where), QStringLiteral("555@0,777@0"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 777, index: 0 }] }],"
        "                                  order: [777, 9, 555] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, where), QStringLiteral("555@2,777@0"));

    const QStringList bad = QStringList()
        << QStringLiteral("[777, '555']") << QStringLiteral("[777, 0, 555]")
        << QStringLiteral("[777, 1.5, 555]") << QStringLiteral("[777, null, 555]")
        << QStringLiteral("[777, 555, 777]") << QStringLiteral("[777, 9, 555, 9]");
    foreach(const QString &order, bad){
        worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { events: [], order: %1 } }, 'HELD'); held.pop()();").arg(order));
        Settle(&worker);
        QVERIFY2(Value(&worker, where) == QStringLiteral("555@2,777@0"), qPrintable(order + QStringLiteral(" -> ") + Value(&worker, where)));
    }

    worker.evaluate(QStringLiteral(
        "var big = []; for (var i = 0; i < 5000; i++) big.push(1000 + i);"
        "big[10] = 777; big[4500] = 555; big[4600] = 0; big[4700] = 777;"
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onActivated', args: [{ tabId: 777, windowId: 1 }] }], order: big } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, where), QStringLiteral("555@0,777@10"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onActivated', args: [{ tabId: 555, windowId: 1 }] }] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, where), QStringLiteral("555@0,777@10"));

    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { events: [], stale: true } }); held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, where), QStringLiteral("555@0,777@10"));
}

void tst_cdpshims::aStaleAnswerEndsThatLoopAndIsNoFailure(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var log = [];"
        "answers.push('HELD');"
        "chrome.tabs.onCreated.addListener(function(t){ log.push(JSON.stringify(t)); });"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] }], stale: true } });"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + fetched.length + '/' + timers.length + '/' + warned.length")),
             QStringLiteral("0/1/0/0"));

    worker.evaluate(QStringLiteral("answers.push('HELD'); chrome.tabs.onRemoved.addListener(function(){});"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + (call(0).args[0] === call(1).args[0])")),
             QStringLiteral("2/true"));
}

void tst_cdpshims::theLastListenerLeavingStopsTheAsking(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls() + Aborting());
    worker.evaluate(QStringLiteral(
        "var log = [];"
        "answers.push('HELD');"
        "var one = function(){ log.push('one'); }, two = function(){ log.push('two'); };"
        "chrome.tabs.onCreated.addListener(one);"
        "chrome.tabs.onActivated.addListener(two);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + ' in ' + timers[0].ms + '/' + aborts"
                                           " + '/' + fetched[0].options.signal.aborted")),
             QStringLiteral("1/1 in 120000/0/false"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("named.map(function(n){ return n[1].join('+'); }).join('|')")),
             QStringLiteral("tabs.onCreated+tabs.onActivated"));
    worker.evaluate(QStringLiteral("chrome.tabs.onCreated.removeListener(one);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + '/' + aborts")), QStringLiteral("1/1/0"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("chrome.tabs.onActivated.removeListener(two);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + '/' + aborts + '/' + fetched[0].options.signal.aborted")),
             QStringLiteral("1/0/1/true"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("named.map(function(n){ return n[1].join('+'); }).join('|')")),
             QStringLiteral("tabs.onCreated+tabs.onActivated|tabs.onActivated|"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] }], order: [1] } });"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + fetched.length + '/' + timers.length")), QStringLiteral("0/1/0"));

    worker.evaluate(QStringLiteral("answers.push('HELD'); chrome.tabs.onCreated.addListener(one);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + (call(0).args[0] === call(1).args[0])")),
             QStringLiteral("2/true"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(call(1).args[1]) + '/' + named.length")), QStringLiteral("[\"tabs.onCreated\"]/3"));
    QCOMPARE(Value(&worker, QStringLiteral("call(0).args[2] + '/' + named.map(function(n){ return n[2]; }).join() + '/' + call(1).args[2]")),
             QStringLiteral("1/2,3,4/5"));
    worker.evaluate(QStringLiteral("namedDown = 1; chrome.tabs.onActivated.addListener(two);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("named.length + '/' + timers.filter(function(t){ return t.ms === 1000; }).length")), QStringLiteral("4/1"));
    worker.evaluate(QStringLiteral("var again = timers.filter(function(t){ return t.ms === 1000; })[0]; timers.splice(timers.indexOf(again), 1); again.f();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("named.length + '/' + named[4][2] + '/' + named[4][1].join('+')")),
             QStringLiteral("5/6/tabs.onCreated+tabs.onActivated"));
}

void tst_cdpshims::theAskingIsBoundedAndComesBackWithASuccess(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false, error: 'Too many are listening for this extension\\'s events.' },"
        "             'DOWN', 'NOT JSON', { value: 'an answer with no ok' }, { ok: true }, { ok: true, value: 'no object' });"
        "chrome.tabs.onCreated.addListener(function(){});"));
    const QStringList waits = QStringList() << QStringLiteral("1000") << QStringLiteral("2000")
                                            << QStringLiteral("4000") << QStringLiteral("8000") << QStringLiteral("16000");
    for(int i = 0; i < waits.size(); i++){
        QTRY_COMPARE(Value(&worker, QStringLiteral("timers.length + ' in ' + (timers.length ? timers[0].ms : 0)")),
                     QStringLiteral("1 in ") + waits[i]);
        worker.evaluate(QStringLiteral("runNextTimer();"));
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.length + '/' + fetched.length + '/' + warned.length")), QStringLiteral("0/6/1"));
    QVERIFY2(Value(&worker, QStringLiteral("warned[0]")).startsWith(QStringLiteral("Vanilla:")), qPrintable(Value(&worker, QStringLiteral("warned[0]"))));

    worker.evaluate(QStringLiteral(
        "answers.push({ value: 'no ok' }, { value: 'no ok' }, { ok: true, value: { events: [], order: [] } }, { value: 'no ok' }, 'HELD');"
        "chrome.tabs.onRemoved.addListener(function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.length + ' in ' + (timers.length ? timers[0].ms : 0) + '/' + fetched.length")),
                 QStringLiteral("1 in 1000/7"));
    worker.evaluate(QStringLiteral("runNextTimer();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.length + ' in ' + (timers.length ? timers[0].ms : 0) + '/' + fetched.length")),
                 QStringLiteral("1 in 2000/8"));
    worker.evaluate(QStringLiteral("runNextTimer();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.length + ' in ' + (timers.length ? timers[0].ms : 0) + '/' + fetched.length")),
                 QStringLiteral("1 in 1000/10"));

    worker.evaluate(QStringLiteral("runNextTimer();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + held.length")), QStringLiteral("11/1"));
    worker.evaluate(QStringLiteral("var before = fetched.length;"
                                   "clock += 25000;"
                                   "answers.push({ value: 'no ok' }, 'HELD');"
                                   "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("(fetched.length - before) + '/' + timers.length + '/' + warned.length")),
             QStringLiteral("1/0/1"));
}

void tst_cdpshims::theShimsOwnDeadlineIsAnotherRoundAndNotAFailure(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls() + Aborting());
    worker.evaluate(QStringLiteral("answers.push('HELD', 'HELD');"
                                   "chrome.tabs.onActivated.addListener(function(){});"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + ' in ' + timers[0].ms")),
             QStringLiteral("1/1 in 120000"));
    worker.evaluate(QStringLiteral("runNextTimer();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("aborts + '/' + fetched.length + '/' + timers.length + ' in ' + timers[0].ms"
                                           " + '/' + warned.length + '/' + (call(0).args[0] === call(1).args[0])")),
             QStringLiteral("1/2/1 in 120000/0/true"));
}

void tst_cdpshims::withoutAKeyTheEventsAreTheStandInsWhichNeverFire(){
    QJSEngine unkeyed;
    Make(&unkeyed, false, Fetching() + Saying());
    unkeyed.evaluate(QStringLiteral("var got = 0; var mine = function(){ got++; };"
                                    "chrome.tabs.onCreated.addListener(mine);"
                                    "chrome.tabs.onRemoved.addListener(mine);"
                                    "chrome.tabs.onUpdated.addListener(mine);"
                                    "chrome.tabs.onActivated.addListener(mine);"
                                    "chrome.windows.onFocusChanged.addListener(mine);"));
    QCOMPARE(Value(&unkeyed, QStringLiteral("fetched.length + '/' + timers.length + '/' + got")), QStringLiteral("0/0/0"));
    QCOMPARE(Value(&unkeyed, QStringLiteral("chrome.tabs.onCreated.hasListeners() + '/' + chrome.tabs.onCreated.hasListener(mine)")),
             QStringLiteral("false/false"));

    QJSEngine keyed;
    MakeAsking(&keyed, Fetching(), Saying());
    keyed.evaluate(QStringLiteral("var mine = function(){};"
                                  "chrome.tabs.onReplaced.addListener(mine);"
                                  "chrome.tabs.onAttached.addListener(mine);"
                                  "chrome.windows.onCreated.addListener(mine);"));
    QCOMPARE(Value(&keyed, QStringLiteral("fetched.length + '/' + chrome.tabs.onReplaced.hasListeners()")), QStringLiteral("0/false"));
    keyed.evaluate(QStringLiteral("answers.push('HELD'); chrome.tabs.onCreated.addListener(mine);"));
    QCOMPARE(Value(&keyed, QStringLiteral("fetched.length + '/' + chrome.tabs.onCreated.hasListeners()")), QStringLiteral("1/true"));
}

void tst_cdpshims::theWindowNumbersAreTheOnesChromeGives(){
    QJSEngine keyed;
    MakeAsking(&keyed, Fetching(), EnginesOwn());
    QCOMPARE(Value(&keyed, QStringLiteral("chrome.windows.WINDOW_ID_NONE + '/' + chrome.windows.WINDOW_ID_CURRENT")),
             QStringLiteral("-1/-2"));
    QCOMPARE(Value(&keyed, QStringLiteral("['WINDOW_ID_NONE', 'WINDOW_ID_CURRENT', 'WINDOW_ID_OTHER', 'MAX_WINDOWS']"
                                          "  .map(function(k){ return k in chrome.windows; }).join()")),
             QStringLiteral("true,true,false,false"));
    QCOMPARE(Value(&keyed, QStringLiteral("String(chrome.windows.WINDOW_ID_OTHER)")), QStringLiteral("undefined"));
    QCOMPARE(Value(&keyed, QStringLiteral("chrome.tabs.TAB_ID_NONE + '/' + ('TAB_ID_NONE' in chrome.tabs) + '/' + engineCalls.join()")),
             QStringLiteral("-1/true/"));

    QJSEngine plain;
    Make(&plain, false, Fetching());
    QCOMPARE(Value(&plain, QStringLiteral("chrome.windows.WINDOW_ID_NONE + '/' + chrome.windows.WINDOW_ID_CURRENT")),
             QStringLiteral("-1/-2"));

    QJSEngine its;
    MakeAsking(&its, Fetching(), QStringLiteral("chrome.windows = { WINDOW_ID_NONE: 99 };"));
    QCOMPARE(Value(&its, QStringLiteral("chrome.windows.WINDOW_ID_NONE + '/' + chrome.windows.WINDOW_ID_CURRENT")),
             QStringLiteral("99/-2"));
}

void tst_cdpshims::theLastListenerGoingWhileTheAnswerIsHandedOutIsNoBarToIt(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var log = [];"
        "answers.push('HELD');"
        "var mine = function(t){ log.push(JSON.stringify(t)); chrome.tabs.onCreated.removeListener(mine); };"
        "chrome.tabs.onCreated.addListener(mine);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] },"
        "  { name: 'tabs.onCreated', args: [{ id: 2, index: 1 }] }"
        "], order: [1, 2] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("{\"id\":1,\"index\":0} | {\"id\":2,\"index\":1}"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + '/' + answers.length"
                                           " + '/' + chrome.tabs.onCreated.hasListeners()")),
             QStringLiteral("1/0/1/false"));
    worker.evaluate(QStringLiteral("chrome.tabs.onCreated.addListener(function(){});"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("2"));

    QJSEngine swap;
    MakeAsking(&swap, Fetching(), Saying() + Calls());
    swap.evaluate(QStringLiteral(
        "var log = [], swapped = 0;"
        "answers.push('HELD');"
        "var next = function(t){ log.push('next ' + JSON.stringify(t)); };"
        "var first = function(t){ log.push('first ' + JSON.stringify(t));"
        "  if (swapped) return;"
        "  swapped = 1;"
        "  chrome.tabs.onCreated.removeListener(first);"
        "  answers.push('HELD');"
        "  chrome.tabs.onCreated.addListener(next); };"
        "chrome.tabs.onCreated.addListener(first);"));
    QCOMPARE(Value(&swap, QStringLiteral("fetched.length")), QStringLiteral("1"));
    swap.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: ["
        "  { name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] },"
        "  { name: 'tabs.onCreated', args: [{ id: 2, index: 1 }] }"
        "], order: [1, 2] } });"
        "held.pop()();"));
    Settle(&swap);
    QCOMPARE(Value(&swap, QStringLiteral("log.join(' | ')")),
             QStringLiteral("first {\"id\":1,\"index\":0} | first {\"id\":2,\"index\":1}"));
    QCOMPARE(Value(&swap, QStringLiteral("fetched.length + '/' + held.length + '/' + answers.length + '/' + timers.length")),
             QStringLiteral("2/1/0/0"));
}

void tst_cdpshims::anOlderCallSettlesNothingOnceANewLoopHasBegun(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var log = [];"
        "var mine = function(t){ log.push(JSON.stringify(t)); };"
        "function stopAndStart(){ chrome.tabs.onCreated.removeListener(mine);"
        "                         answers.push('HELD'); chrome.tabs.onCreated.addListener(mine); }"
        "answers.push('HELD'); chrome.tabs.onCreated.addListener(mine);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + held.length + '/' + ('signal' in fetched[0].options)")),
             QStringLiteral("1/1/false"));

    worker.evaluate(QStringLiteral(
        "stopAndStart();"
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] }], order: [1] } });"
        "held.shift()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + fetched.length + '/' + timers.length + '/' + held.length")),
             QStringLiteral("0/2/0/1"));

    worker.evaluate(QStringLiteral("stopAndStart(); answers.push('NOT JSON'); held.shift()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + '/' + warned.length + '/' + held.length")),
             QStringLiteral("3/0/0/1"));

    worker.evaluate(QStringLiteral("stopAndStart(); answers.push({ ok: true, value: { events: [], stale: true } }); held.shift()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + '/' + held.length")), QStringLiteral("4/0/1"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 7, index: 0 }] }], order: [7] } }, 'HELD');"
        "held.shift()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join() + '/' + fetched.length")),
             QStringLiteral("{\"id\":7,\"index\":0}/5"));
}

void tst_cdpshims::stoppingTakesBackTheWaitAndStartingAgainIsOneLoop(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var mine = function(){}, other = function(){};"
        "answers.push({ value: 'an answer with no ok' });"
        "chrome.tabs.onCreated.addListener(mine);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + ' in ' + (timers.length ? timers[0].ms : 0)")),
                 QStringLiteral("1/1 in 1000"));
    worker.evaluate(QStringLiteral("chrome.tabs.onActivated.addListener(other);"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + ' in ' + timers[0].ms")),
             QStringLiteral("1/1 in 1000"));
    worker.evaluate(QStringLiteral("chrome.tabs.onCreated.removeListener(mine);"
                                   "chrome.tabs.onActivated.removeListener(other);"));
    QCOMPARE(Value(&worker, QStringLiteral("timers.length + '/' + fetched.length")), QStringLiteral("0/1"));
    worker.evaluate(QStringLiteral("answers.push({ value: 'an answer with no ok' });"
                                   "chrome.tabs.onCreated.addListener(mine);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length + '/' + timers.length + ' in ' + timers[0].ms")),
                 QStringLiteral("2/1 in 1000"));

    QJSEngine stale;
    MakeAsking(&stale, Fetching(), Saying() + Calls() + QStringLiteral("clearTimeout = undefined;"));
    stale.evaluate(QStringLiteral("var mine = function(){};"
                                  "answers.push({ value: 'an answer with no ok' });"
                                  "chrome.tabs.onCreated.addListener(mine);"));
    QTRY_COMPARE(Value(&stale, QStringLiteral("fetched.length + '/' + timers.length")), QStringLiteral("1/1"));
    stale.evaluate(QStringLiteral("chrome.tabs.onCreated.removeListener(mine); runTimers();"));
    Settle(&stale);
    QCOMPARE(Value(&stale, QStringLiteral("fetched.length + '/' + timers.length + '/' + warned.length")),
             QStringLiteral("1/0/0"));
}

void tst_cdpshims::theCallIsGivenUpWithWhateverTheWorldHas(){
    const QString listening = QStringLiteral(
        "var log = [];"
        "var mine = function(t){ log.push(JSON.stringify(t)); };"
        "answers.push('HELD'); chrome.tabs.onCreated.addListener(mine);");
    const QString stopping = QStringLiteral(
        "chrome.tabs.onCreated.removeListener(mine);"
        "answers.push({ ok: true, value: { events: [{ name: 'tabs.onCreated', args: [{ id: 1, index: 0 }] }], order: [1] } });"
        "held.pop()();");

    QJSEngine timing;
    MakeAsking(&timing, Fetching(), Saying() + Calls() + QStringLiteral(
        "var timeouts = [];"
        "function AbortSignal(){}"
        "AbortSignal.timeout = function(ms){ timeouts.push(ms); return { timeoutMs: ms }; };"));
    timing.evaluate(listening);
    QCOMPARE(Value(&timing, QStringLiteral("timeouts.join() + '/' + fetched[0].options.signal.timeoutMs + '/' + timers.length")),
             QStringLiteral("120000/120000/0"));
    timing.evaluate(stopping);
    Settle(&timing);
    QCOMPARE(Value(&timing, QStringLiteral("log.length + '/' + fetched.length + '/' + timers.length")), QStringLiteral("0/1/0"));

    QJSEngine bare;
    MakeAsking(&bare, Fetching(), Saying() + Calls());
    bare.evaluate(listening);
    QCOMPARE(Value(&bare, QStringLiteral("('signal' in fetched[0].options) + '/' + timers.length")), QStringLiteral("false/0"));
    bare.evaluate(stopping);
    Settle(&bare);
    QCOMPARE(Value(&bare, QStringLiteral("log.length + '/' + fetched.length + '/' + timers.length")), QStringLiteral("0/1/0"));
}

void tst_cdpshims::anAnswerOfTheOrderAloneIsNoEventAndNoFailure(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var senders = [], log = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "answers.push('HELD');"
        "chrome.tabs.onCreated.addListener(function(t){ log.push(JSON.stringify(t)); });"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 3 } });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: 'a9000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.index")), QStringLiteral("3"));

    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { events: [], order: [9, 555] } }, 'HELD'); held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.index + '/' + log.length + '/' + fetched.length"
                                           " + '/' + timers.map(function(t){ return t.ms; }).join() + '/' + warned.length")),
             QStringLiteral("1/0/3/3000/0"));
}

void tst_cdpshims::theDocumentsOwnNavigationsAreHeardOnlyOnceSomebodyListens(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + Navigating());
    QCOMPARE(Value(&page, QStringLiteral("Object.keys(navHeard).length + '/' + hashed.length + '/' + popped.length")),
             QStringLiteral("0/0/0"));

    page.evaluate(QStringLiteral("replies.push({ linked: 1 }, { linked: 1 });"
                                 "var mine = function(){ return false; };"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("navHeard.navigate.length + '/' + navHeard.currententrychange.length")),
             QStringLiteral("1/1"));
    QCOMPARE(Value(&page, QStringLiteral("hashed.length + '/' + popped.length")), QStringLiteral("0/0"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));

    page.evaluate(QStringLiteral("navigate('https://a.example/mid'); commit('https://a.example/mid');"));
    QCOMPARE(Value(&page, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("navHeard.navigate.length + '/' + navHeard.currententrychange.length")),
             QStringLiteral("1/1"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral(
        "ports[1].say({ linked: 1 });"
        "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
        "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"
        "navigate('https://a.example/next'); commit('https://a.example/next'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QStringLiteral("history https://a.example/next"));
}

void tst_cdpshims::theKindOfANavigationComesFromTheNavigateEvent(){
    QJSEngine page;
    Linked(&page, Navigating());
    page.evaluate(QStringLiteral("navigate('https://a.example/page#one', false); commit('https://a.example/page#one'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/page#one"));
    page.evaluate(QStringLiteral("navigate('https://a.example/page#two', true); commit('https://a.example/page#two'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")),
             QStringLiteral("history https://a.example/page#one | fragment https://a.example/page#two"));

    page.evaluate(QStringLiteral("var before = navs();"
                                 "navigate('https://a.example/page#three', true);"
                                 "navigate('https://a.example/deep', false);"
                                 "commit('https://a.example/deep'); runNextTimer();"
                                 "commit(); runTimers();"));
    QCOMPARE(Value(&page, QStringLiteral("navs().slice(before.length)")), QStringLiteral(" | history https://a.example/deep"));

    page.evaluate(QStringLiteral("before = navs();"
                                 "navigate('https://a.example/deep#four', true, true);"
                                 "navigate('https://a.example/deep#four', false, false);"
                                 "commit('https://a.example/deep#four'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs().slice(before.length)")), QStringLiteral(" | fragment https://a.example/deep#four"));

    page.evaluate(QStringLiteral("before = navs();"
                                 "navigate('https://a.example/elsewhere', false);"
                                 "commit('https://a.example/deep#five'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs().slice(before.length)")), QStringLiteral(" | fragment https://a.example/deep#five"));
}

void tst_cdpshims::theNavigationsOfOneHundredMillisecondsAreOneMessage(){
    QJSEngine page;
    Linked(&page, Navigating());
    page.evaluate(QStringLiteral(
        "navigate('https://a.example/a'); commit('https://a.example/a');"
        "navigate('https://a.example/b'); commit('https://a.example/b');"
        "navigate('https://a.example/c'); commit('https://a.example/c');"));
    QCOMPARE(Value(&page, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length + '/' + timers.length")),
             QStringLiteral("1/2"));
    page.evaluate(QStringLiteral("runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs() + '/' + timers.length")),
             QStringLiteral("history https://a.example/c/1"));
    page.evaluate(QStringLiteral("navigate('https://a.example/d'); commit('https://a.example/d');"
                                 "navigate('https://a.example/c'); commit('https://a.example/c');"
                                 "runNextTimer();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/c"));

    QJSEngine blunt;
    Linked(&blunt, Navigating() + QStringLiteral("clearTimeout = undefined;"));
    blunt.evaluate(QStringLiteral("navigate('https://a.example/one'); commit('https://a.example/one');"
                                  "navigate('https://a.example/two'); commit('https://a.example/two');"));
    QCOMPARE(Value(&blunt, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length")), QStringLiteral("2"));
    blunt.evaluate(QStringLiteral("runTimers();"));
    Pump(&blunt);
    QCOMPARE(Value(&blunt, QStringLiteral("navs()")), QStringLiteral("history https://a.example/two"));
}

void tst_cdpshims::aDocumentNobodyLooksAtTellsOfNoNavigation(){
    QJSEngine page;
    Linked(&page, Navigating());
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("navigate('https://a.example/one'); commit('https://a.example/one');"
                                 "navigate('https://a.example/two'); commit('https://a.example/two');"
                                 "runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs() + '/' + ports[0].posted.length")), QStringLiteral("/1"));

    page.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/two"));
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/two"));
}

void tst_cdpshims::aLinkWhichDoesNotSayWhereItIsHeldIsTakenToBeWhereTheDocumentIs(){
    QJSEngine page;
    Linked(&page, Navigating());
    page.evaluate(QStringLiteral("ports[0].die();"
                                 "navigate('https://a.example/gone'); commit('https://a.example/gone');"
                                 "replies.push({ linked: 1 }); runTimers();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QString());
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral("ports[1].say({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QString());
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("runTimers();"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QString());
    page.evaluate(QStringLiteral("navigate('https://a.example/gone#here', true); commit('https://a.example/gone#here'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QStringLiteral("fragment https://a.example/gone#here"));
}

void tst_cdpshims::theNavigateHandlerTouchesNothingOfThePages(){
    QJSEngine page;
    Linked(&page, Navigating());
    page.evaluate(QStringLiteral("navigate('https://a.example/x'); commit('https://a.example/x'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navTouched.join() + '/' + navs()")),
             QStringLiteral("/history https://a.example/x"));
    QVERIFY(!page.evaluate(QStringLiteral("navFire('navigate', { hashChange: true, get destination(){ throw new Error('not for you'); } });")).isError());
    QVERIFY(!page.evaluate(QStringLiteral("navFire('navigate', null);")).isError());
    QVERIFY(!page.evaluate(QStringLiteral("navFire('currententrychange', {});")).isError());
    page.evaluate(QStringLiteral("runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/x"));

    page.evaluate(QStringLiteral("navigate('https://a.example/y', true);"
                                 "navFire('navigate', { hashChange: false, destination: { sameDocument: true, get url(){ throw new Error('no'); } } });"
                                 "commit('https://a.example/y'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")),
             QStringLiteral("history https://a.example/x | history https://a.example/y"));
}

void tst_cdpshims::withoutTheNavigationApiTheWindowsOwnEventsAreHeard(){
    QJSEngine page;
    Linked(&page, QString());
    QCOMPARE(Value(&page, QStringLiteral("hashed.length + '/' + popped.length")), QStringLiteral("1/1"));
    page.evaluate(QStringLiteral("location.href = 'https://a.example/page#one'; hashchange(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("fragment https://a.example/page#one"));
    page.evaluate(QStringLiteral("location.href = 'https://a.example/other'; popstate(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")),
             QStringLiteral("fragment https://a.example/page#one | history https://a.example/other"));
    page.evaluate(QStringLiteral("location.href = 'https://a.example/other#two'; popstate(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs().split(' | ').pop()")), QStringLiteral("fragment https://a.example/other#two"));
    page.evaluate(QStringLiteral("location.href = 'https://a.example/pushed'; runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs().split(' | ').pop()")), QStringLiteral("fragment https://a.example/other#two"));
}

void tst_cdpshims::theDocumentsNavigationsAreToldToTheExtension(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
        "var log = [];"
        "function watch(name){ return function(d){ log.push(name + ' ' + JSON.stringify(d)); }; }"
        "var onHistory = watch('history'), onFragment = watch('fragment');"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(onHistory);"
        "chrome.webNavigation.onReferenceFragmentUpdated.addListener(onFragment);"
        "var top = linking('11000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var sub = linking('12000000000000000000000000000000', 4, 'https://a.example/frame', 200);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("clock = 1234567; top.say({ nav: { kind: 'history', url: 'https://a.example/top/next' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("history {\"tabId\":5,\"frameId\":0,\"url\":\"https://a.example/top/next\",\"timeStamp\":1234567,"
                            "\"processId\":-1,\"parentFrameId\":-1,\"frameType\":\"outermost_frame\",\"documentLifecycle\":\"active\"}"));
    worker.evaluate(QStringLiteral("log = []; sub.say({ nav: { kind: 'fragment', url: 'https://a.example/frame#hint' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("fragment {\"tabId\":5,\"frameId\":4,\"url\":\"https://a.example/frame#hint\",\"timeStamp\":1234567,"
                            "\"processId\":-1,\"parentFrameId\":0,\"frameType\":\"sub_frame\",\"documentLifecycle\":\"active\"}"));
    QVERIFY2(!Value(&worker, QStringLiteral("log.join()")).contains(QStringLiteral("transition")),
             qPrintable(Value(&worker, QStringLiteral("log.join()"))));

    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){"
                                   "  out = f.map(function(x){ return x.frameId + ':' + x.parentFrameId + ':' + x.url; }).join(); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")),
                 QStringLiteral("0:-1:https://a.example/top/next,4:0:https://a.example/frame#hint"));

    worker.evaluate(QStringLiteral(
        "log = [];"
        "var thrower = function(){ throw new Error('the extension\\'s own'); }, onSecond = watch('second');"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(thrower);"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(onSecond);"
        "top.say({ nav: { kind: 'history', url: 'https://a.example/top/third' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + (cried.length >= 1)")), QStringLiteral("2/true"));

    worker.evaluate(QStringLiteral(
        "log = [];"
        "[onHistory, thrower, onSecond].forEach(function(f){ chrome.webNavigation.onHistoryStateUpdated.removeListener(f); });"
        "top.say({ nav: { kind: 'history', url: 'https://a.example/top/quiet' } });"
        "out = 'pending';"
        "chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ out = f[0].url; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("https://a.example/top/quiet"));
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + chrome.webNavigation.onHistoryStateUpdated.hasListeners()")),
             QStringLiteral("0/false"));

    worker.evaluate(QStringLiteral("var never = function(){}; chrome.webNavigation.onBeforeNavigate.addListener(never);"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.webNavigation.onBeforeNavigate.hasListener(never) + '/' + chrome.webNavigation.onBeforeNavigate.hasListeners()")),
             QStringLiteral("false/false"));

    QJSEngine keyless;
    Make(&keyless, false, Fetching() + Saying());
    keyless.evaluate(Linking());
    keyless.evaluate(QStringLiteral("Math.random = function(){ return 0.25; };"
                                    "var seen = [];"
                                    "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ seen.push(d.tabId + ' ' + d.url); });"
                                    "var p = linking('13000000000000000000000000000000', 0, 'https://a.example/top', 100);"
                                    "p.say({ nav: { kind: 'history', url: 'https://a.example/top/own' } });"));
    QCOMPARE(Value(&keyless, QStringLiteral("seen.join() + '/' + fetched.length")),
             QStringLiteral("1342177280 https://a.example/top/own/0"));
    QCOMPARE(Value(&keyless, QStringLiteral("chrome.webNavigation.onHistoryStateUpdated.hasListeners()")), QStringLiteral("true"));
}

void tst_cdpshims::aNavigationOfAnotherOriginOrShapeIsDropped(){
    const QString said = QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var log = [];"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url); });"
        "chrome.webNavigation.onReferenceFragmentUpdated.addListener(function(d){ log.push('#' + d.url); });"
        "var p = linking('31000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "function bad(){"
        "  var many = 'https://a.example/' + new Array(8200).join('x');"
        "  [{ kind: 'history' }, { kind: 'reload', url: 'https://a.example/x' }, { kind: 5, url: 'https://a.example/x' },"
        "   { kind: 'history', url: 5 }, { kind: 'history', url: '' }, { kind: 'history', url: many },"
        "   { kind: 'history', url: 'https://b.example/x' }, { kind: 'history', url: 'http://a.example/x' },"
        "   { kind: 'history', url: 'https://a.example.evil/x' }, { kind: 'history', url: 'about:blank' },"
        "   { kind: 'history', url: 'blob:https://a.example/abc' }, { kind: 'history', url: 'data:text/html,x' },"
        "   { kind: 'history', url: 'foo://a.example/x' }, { kind: 'history', url: '/relative' }"
        "  ].forEach(function(nav){ p.say({ nav: nav }); });"
        "  p.say({ nav: null }); p.say({ nav: 'nonsense' });"
        "}"
        "function frames(){ var out = 'pending';"
        "  chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ out = f.length + ' ' + f.map(function(x){ return x.url; }).join(); });"
        "  return function(){ return out; }; }");
    QJSEngine strict;
    MakeAsking(&strict, Fetching(), Saying() + QStringLiteral("URL = undefined;"));
    strict.evaluate(Linking());
    strict.evaluate(said);
    Settle(&strict);
    strict.evaluate(QStringLiteral("bad(); var read = frames();"));
    QTRY_COMPARE(Value(&strict, QStringLiteral("read()")), QStringLiteral("1 https://a.example/top"));
    QCOMPARE(Value(&strict, QStringLiteral("log.join() + '/' + p.disconnected")), QStringLiteral("/0"));
    strict.evaluate(QStringLiteral("p.say({ nav: { kind: 'fragment', url: 'https://a.example/top#ok' } });"));
    QCOMPARE(Value(&strict, QStringLiteral("log.join()")), QStringLiteral("#https://a.example/top#ok"));

    QJSEngine urling;
    MakeAsking(&urling, Fetching(), Saying() + Urling());
    urling.evaluate(Linking());
    urling.evaluate(said);
    Settle(&urling);
    urling.evaluate(QStringLiteral("bad(); var read = frames();"));
    QTRY_COMPARE(Value(&urling, QStringLiteral("read()")), QStringLiteral("1 https://a.example/top"));
    QCOMPARE(Value(&urling, QStringLiteral("log.join() + '/' + p.disconnected")), QStringLiteral("/0"));
    urling.evaluate(QStringLiteral("p.say({ nav: { kind: 'history', url: 'https://a.example/top/ok' } });"));
    QCOMPARE(Value(&urling, QStringLiteral("log.join()")), QStringLiteral("https://a.example/top/ok"));
}

void tst_cdpshims::aDocumentsFirstLinkIsItsCommit(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
        "var log = [];"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push(JSON.stringify(d)); });"
        "function fresh(nonce, frame, url, word){"
        "  return naming({ link: { nonce: nonce, frameId: frame, tabUrl: 'https://a.example/page', since: 100,"
        "                          fresh: word === undefined ? 1 : word } }, url); }"
        "clock = 4242;"
        "var top = fresh('51000000000000000000000000000000', 0, 'https://a.example/top');"
        "var sub = fresh('52000000000000000000000000000000', 3, 'https://a.example/frame');"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("{\"tabId\":5,\"frameId\":0,\"url\":\"https://a.example/top\",\"timeStamp\":4242,"
                            "\"processId\":-1,\"parentFrameId\":-1,\"frameType\":\"outermost_frame\",\"documentLifecycle\":\"active\"} | "
                            "{\"tabId\":5,\"frameId\":3,\"url\":\"https://a.example/frame\",\"timeStamp\":4242,"
                            "\"processId\":-1,\"parentFrameId\":0,\"frameType\":\"sub_frame\",\"documentLifecycle\":\"active\"}"));
    QCOMPARE(Value(&worker, QStringLiteral("top.posted[0].linked")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("log = []; var again = fresh('51000000000000000000000000000000', 0, 'https://a.example/top');"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + top.disconnected")), QStringLiteral("0/1"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 6, index: 1 } }, { ok: true, value: { id: 6, index: 1 } },"
        "             { ok: true, value: { id: 6, index: 1 } });"
        "linking('53000000000000000000000000000000', 0, 'https://a.example/b', 100);"
        "fresh('54000000000000000000000000000000', 0, 'https://a.example/c', true);"
        "fresh('55000000000000000000000000000000', 0, 'https://a.example/d', '1');"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 7, index: 2 } });"
        "var blank = aPort('__vanilla_link__', { id: chrome.runtime.id }); connects[0](blank);"
        "blank.say({ link: { nonce: '56000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1, fresh: 1 } });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + blank.posted[0].linked")), QStringLiteral("0/1"));

    QJSEngine unheard;
    MakeAsking(&unheard, Fetching(), Saying());
    unheard.evaluate(Linking());
    unheard.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 8, index: 0 } });"
        "var log = [];"
        "naming({ link: { nonce: '57000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1, fresh: 1 } },"
        "       'https://a.example/top');"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push(d.url); });"));
    Settle(&unheard);
    QCOMPARE(Value(&unheard, QStringLiteral("log.length")), QStringLiteral("0"));
    unheard.evaluate(QStringLiteral("var filtered = function(){};"
                                    "chrome.webNavigation.onCommitted.addListener(filtered, { url: [{ hostSuffix: 'a.example' }] });"));
    QCOMPARE(Value(&unheard, QStringLiteral("String(chrome.webNavigation.onCommitted.hasListener(filtered))")), QStringLiteral("false"));

    QJSEngine waiting;
    MakeAsking(&waiting, Fetching(), Saying());
    waiting.evaluate(Linking());
    waiting.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push(d.tabId + ' ' + d.url + ' @' + d.timeStamp); });"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url.split('/').pop()); });"
        "clock = 3000;"
        "var w = naming({ link: { nonce: '58000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1, fresh: 1 } },"
        "               'https://a.example/top');"));
    Settle(&waiting);
    QCOMPARE(Value(&waiting, QStringLiteral("log.length")), QStringLiteral("0"));
    waiting.evaluate(QStringLiteral("for (var i = 0; i < 9; i++) w.say({ nav: { kind: 'history', url: 'https://a.example/' + i } });"
                                    "clock = 9000; answers.push({ ok: true, value: { id: 900, index: 0 } }); held.pop()();"));
    Settle(&waiting);
    QCOMPARE(Value(&waiting, QStringLiteral("log.join(' | ')")),
             QStringLiteral("900 https://a.example/top @3000 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8"));

    QJSEngine retired;
    MakeAsking(&retired, Fetching(), Saying());
    retired.evaluate(Linking());
    retired.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push(d.tabId + ' ' + d.url); });"
        "var first = naming({ link: { nonce: '59000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1, fresh: 1 } },"
        "                   'https://a.example/top');"));
    Settle(&retired);
    retired.evaluate(QStringLiteral(
        "var second = naming({ link: { nonce: '59000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1, fresh: 1 } },"
        "                    'https://a.example/top');"
        "answers.push({ ok: true, value: { id: 901, index: 0 } }); held.pop()();"));
    Settle(&retired);
    QCOMPARE(Value(&retired, QStringLiteral("log.join(' | ') + '/' + first.disconnected")), QStringLiteral("901 https://a.example/top/1"));
}

void tst_cdpshims::aFrameNamedByItsIdIsItsNewestDocument(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['<all_urls>'] }; };"));
    worker.evaluate(Linking() + Serving());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } });"
        "var old = linking('b1000000000000000000000000000000', 0, 'https://a.example/old', 100);"
        "var now = linking('b2000000000000000000000000000000', 0, 'https://a.example/new', 200);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 555, frameIds: [0] }, css: 'a{}' }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("old.posted.length + ' ' + JSON.stringify(now.last())")), QStringLiteral(R"(1 {"style":1,"add":"a{}"})"));
    worker.evaluate(QStringLiteral("var back = linking('b1000000000000000000000000000000', 0, 'https://a.example/old', 100); now.say({ styled: 1 });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 555, frameIds: [0] }, css: 'b{}' }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("back.posted.length + ' ' + JSON.stringify(now.last())")), QStringLiteral(R"(1 {"style":2,"add":"b{}"})"));
}

void tst_cdpshims::aDocumentClaimsFreshUntilItIsLinked(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("for (var r = 0; r < 8; r++) replies.push({ linked: 1 });"
                                 "var mine = function(){ return false; };"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&page, QStringLiteral("String(ports[0].posted[0].link.fresh)")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[0].die();"));
    page.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&page, QStringLiteral("String(ports[1].posted[0].link.fresh)")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("ports[1].say({ linked: 1, url: 'https://a.example/page' }); ports[1].die();"));
    page.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("3"));
    QCOMPARE(Value(&page, QStringLiteral("String(ports[2].posted[0].link.fresh)")), QStringLiteral("undefined"));
    page.evaluate(QStringLiteral("ports[2].say({ linked: 1, url: 'https://a.example/page' });"
                                 "chrome.runtime.onMessage.removeListener(mine);"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("4"));
    QCOMPARE(Value(&page, QStringLiteral("String(ports[3].posted[0].link.fresh)")), QStringLiteral("undefined"));
}

void tst_cdpshims::aDocumentsLoadingIsToldOnceAfterItsCommit(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var log = [];"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push('commit ' + d.url); });"
        "chrome.webNavigation.onDOMContentLoaded.addListener(function(d){ log.push('dom ' + JSON.stringify(d)); });"
        "chrome.webNavigation.onCompleted.addListener(function(d){ log.push('done ' + d.tabId + ' ' + d.frameId + ' ' + d.url); });"
        "function claim(nonce, word){"
        "  var link = { nonce: nonce, frameId: 0, tabUrl: 'https://a.example/page', since: 100 };"
        "  for (var k in word) link[k] = word[k];"
        "  return naming({ link: link }, 'https://a.example/top'); }"
        "clock = 4242;"
        "var top = claim('61000000000000000000000000000000', { fresh: 1, ready: 2, readyFrom: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("commit https://a.example/top | "
                            "dom {\"tabId\":5,\"frameId\":0,\"url\":\"https://a.example/top\",\"timeStamp\":4242,"
                            "\"processId\":-1,\"parentFrameId\":-1,\"frameType\":\"outermost_frame\",\"documentLifecycle\":\"active\"} | "
                            "done 5 0 https://a.example/top"));
    worker.evaluate(QStringLiteral("log = []; var again = claim('61000000000000000000000000000000', { fresh: 1, ready: 2, readyFrom: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length")), QStringLiteral("0"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 6, index: 1 } });"
        "var step = claim('62000000000000000000000000000000', { ready: 1, readyFrom: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.map(function(l){ return l.split(' ')[0]; }).join()")), QStringLiteral("dom"));
    worker.evaluate(QStringLiteral("log = []; step.say({ ready: 2, from: 1 }); step.say({ ready: 2, from: 1 }); step.say({ ready: 2, from: 0 });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")), QStringLiteral("done 6 0 https://a.example/top"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 7, index: 2 } });"
        "log = []; var odd = claim('63000000000000000000000000000000', {});"
        "odd.say({ ready: '2', from: 0 }); odd.say({ ready: 3, from: 0 }); odd.say({ ready: 1, from: 1 });"
        "odd.say({ ready: 2, from: -1 }); odd.say({ ready: 2, from: 0, re: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length + '/' + odd.disconnected")), QStringLiteral("0/0"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 8, index: 3 } });"
        "claim('64000000000000000000000000000000', { ready: true, readyFrom: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.length")), QStringLiteral("0"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 11, index: 4 } });"
        "claim(null, { fresh: 1 });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "log = []; var bare = claim(null, { fresh: 1, ready: 1, readyFrom: 0 });"
        "bare.say({ ready: 2, from: 1 }); bare.say({ ready: 2, from: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.map(function(l){ return l.split(' ')[0]; }).join()")), QStringLiteral("commit,dom,done"));

    QJSEngine half;
    MakeAsking(&half, Fetching(), Saying());
    half.evaluate(Linking());
    half.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 9, index: 0 } });"
        "var log = [];"
        "chrome.webNavigation.onCompleted.addListener(function(d){ log.push('done'); });"
        "var p = naming({ link: { nonce: '65000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1,"
        "                         ready: 2, readyFrom: 0 } }, 'https://a.example/top');"
        "chrome.webNavigation.onDOMContentLoaded.addListener(function(d){ log.push('dom'); });"));
    Settle(&half);
    QCOMPARE(Value(&half, QStringLiteral("log.join()")), QStringLiteral("done"));

    QJSEngine waiting;
    MakeAsking(&waiting, Fetching(), Saying());
    waiting.evaluate(Linking());
    waiting.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push('commit'); });"
        "chrome.webNavigation.onDOMContentLoaded.addListener(function(d){ log.push('dom ' + d.url.split('/').pop() + ' @' + d.timeStamp); });"
        "chrome.webNavigation.onCompleted.addListener(function(d){ log.push('done'); });"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url.split('/').pop()); });"
        "var w = naming({ link: { nonce: '66000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1, fresh: 1 } },"
        "               'https://a.example/top');"
        "w.say({ nav: { kind: 'history', url: 'https://a.example/a' } });"
        "clock = 3000; w.say({ ready: 2, from: 0 });"
        "for (var i = 0; i < 9; i++) w.say({ nav: { kind: 'history', url: 'https://a.example/' + i } });"
        "clock = 9000; answers.push({ ok: true, value: { id: 900, index: 0 } }); held.pop()();"));
    Settle(&waiting);
    QCOMPARE(Value(&waiting, QStringLiteral("log.join(' | ')")),
             QStringLiteral("commit | dom a @3000 | done | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8"));

    QJSEngine many;
    MakeAsking(&many, Fetching(), Saying());
    many.evaluate(Linking());
    many.evaluate(QStringLiteral(
        "var log = [];"
        "chrome.webNavigation.onDOMContentLoaded.addListener(function(d){ log.push('dom ' + d.tabId); });"
        "chrome.webNavigation.onCompleted.addListener(function(d){ log.push('done ' + d.tabId); });"
        "function named(i, word){ answers.push({ ok: true, value: { id: 100 + i, index: 0 } });"
        "  var link = { nonce: (0x10000 + i).toString(16) + '000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/' + i, since: 1 };"
        "  for (var k in word) link[k] = word[k];"
        "  return naming({ link: link }, 'https://a.example/' + i); }"
        "var first = named(0, { ready: 1, readyFrom: 0 });"));
    Settle(&many);
    many.evaluate(QStringLiteral("for (var i = 1; i <= 64; i++) named(i, { ready: 1, readyFrom: 0 });"));
    Settle(&many);
    many.evaluate(QStringLiteral("log = []; named(0, { ready: 2, readyFrom: 1 });"));
    Settle(&many);
    QCOMPARE(Value(&many, QStringLiteral("log.join()")), QStringLiteral("done 100"));

    QJSEngine retired;
    MakeAsking(&retired, Fetching(), Saying());
    retired.evaluate(Linking());
    retired.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onCommitted.addListener(function(d){ log.push('commit ' + d.tabId); });"
        "chrome.webNavigation.onDOMContentLoaded.addListener(function(d){ log.push('dom ' + d.tabId); });"
        "chrome.webNavigation.onCompleted.addListener(function(d){ log.push('done ' + d.tabId); });"
        "var said = { link: { nonce: '67000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1,"
        "                     fresh: 1, ready: 2, readyFrom: 0 } };"
        "var first = naming(said, 'https://a.example/top');"));
    Settle(&retired);
    retired.evaluate(QStringLiteral("var second = naming(said, 'https://a.example/top');"
                                    "answers.push({ ok: true, value: { id: 902, index: 0 } }); held.pop()();"));
    Settle(&retired);
    QCOMPARE(Value(&retired, QStringLiteral("log.join(' | ') + '/' + first.disconnected")),
             QStringLiteral("commit 902 | dom 902 | done 902/1"));

    QJSEngine turning;
    MakeAsking(&turning, Fetching(), Saying());
    turning.evaluate(Linking());
    turning.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 12, index: 0 } });"
        "var log = [];"
        "var doneA = function(){ log.push('doneA'); }, doneB = function(){ log.push('doneB'); };"
        "chrome.webNavigation.onDOMContentLoaded.addListener(function(){ log.push('dom');"
        "  chrome.webNavigation.onCompleted.removeListener(doneA); chrome.webNavigation.onCompleted.addListener(doneB); });"
        "chrome.webNavigation.onCompleted.addListener(doneA);"
        "naming({ link: { nonce: '68000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 1,"
        "                 ready: 2, readyFrom: 0 } }, 'https://a.example/top');"));
    Settle(&turning);
    QCOMPARE(Value(&turning, QStringLiteral("log.join(' | ')")), QStringLiteral("dom | doneB"));

    worker.evaluate(QStringLiteral(
        "var replaced = function(){ log.push('replaced'); };"
        "chrome.webNavigation.onTabReplaced.addListener(replaced);"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.webNavigation.onTabReplaced.hasListener(replaced) + '/' + chrome.webNavigation.onTabReplaced.hasListeners()")),
             QStringLiteral("true/true"));
    worker.evaluate(QStringLiteral("chrome.webNavigation.onTabReplaced.removeListener(replaced);"));
    QCOMPARE(Value(&worker, QStringLiteral("String(chrome.webNavigation.onTabReplaced.hasListeners())")), QStringLiteral("false"));
    worker.evaluate(QStringLiteral("var filtered = function(){};"
                                   "chrome.webNavigation.onCompleted.addListener(filtered, { url: [{ hostSuffix: 'a.example' }] });"));
    QCOMPARE(Value(&worker, QStringLiteral("String(chrome.webNavigation.onCompleted.hasListener(filtered))")), QStringLiteral("false"));
}

void tst_cdpshims::aSeenDocumentTellsHowFarItIsLoaded(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying());
    page.evaluate(QStringLiteral("document.readyState = 'loading';"
                                 "for (var r = 0; r < 8; r++) replies.push({ linked: 1 });"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&page, QStringLiteral("String(ports[0].posted[0].link.ready)")), QStringLiteral("undefined"));
    page.evaluate(QStringLiteral("document.onLoaded(); ports[0].say({ linked: 1, url: 'https://a.example/page' });"
                                 "windowLoaded(); windowLoaded();"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].posted.filter(function(m){ return 'ready' in m; }))")),
             QStringLiteral(R"([{"ready":1,"from":0},{"ready":2,"from":1}])"));

    QJSEngine hidden;
    Make(&hidden, true, Fetching() + Saying());
    hidden.evaluate(QStringLiteral("document.readyState = 'interactive'; document.visibilityState = 'hidden';"
                                   "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&hidden, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&hidden, QStringLiteral("String(ports[0].posted[0].link.ready)")), QStringLiteral("undefined"));
    hidden.evaluate(QStringLiteral("ports[0].say({ linked: 1, url: 'https://a.example/page' }); windowLoaded();"));
    QCOMPARE(Value(&hidden, QStringLiteral("ports[0].posted.filter(function(m){ return 'ready' in m; }).length")), QStringLiteral("0"));
    hidden.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&hidden, QStringLiteral("JSON.stringify(ports[0].posted.filter(function(m){ return 'ready' in m; }))")),
             QStringLiteral(R"([{"ready":2,"from":0}])"));

    QJSEngine claimed;
    Make(&claimed, true, Fetching() + Saying());
    claimed.evaluate(QStringLiteral("for (var r = 0; r < 8; r++) replies.push({ linked: 1 });"
                                    "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&claimed, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&claimed, QStringLiteral("ports[0].posted[0].link.ready + '/' + ports[0].posted[0].link.readyFrom")), QStringLiteral("2/0"));
    claimed.evaluate(QStringLiteral("ports[0].die(); runTimers();"));
    QTRY_COMPARE(Value(&claimed, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&claimed, QStringLiteral("ports[1].posted[0].link.ready + '/' + ports[1].posted[0].link.readyFrom")), QStringLiteral("2/0"));
    claimed.evaluate(QStringLiteral("ports[1].say({ linked: 1, url: 'https://a.example/page' }); ports[1].die(); runTimers();"));
    QTRY_COMPARE(Value(&claimed, QStringLiteral("ports.length")), QStringLiteral("3"));
    QCOMPARE(Value(&claimed, QStringLiteral("String(ports[2].posted[0].link.ready)")), QStringLiteral("undefined"));

    QJSEngine unwanted;
    Make(&unwanted, true, Fetching() + Saying() + QStringLiteral("self.__vanillaCarrier = 1; replies.push({ linked: 1 }, { linked: 1 });"));
    QTRY_COMPARE(Value(&unwanted, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&unwanted, QStringLiteral("String(ports[0].posted[0].link.ready)")), QStringLiteral("2"));
    unwanted.evaluate(QStringLiteral("ports[0].say({ linked: 1, carrying: 0 });"));
    QCOMPARE(Value(&unwanted, QStringLiteral("String(ports[0].disconnected)")), QStringLiteral("1"));
    unwanted.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&unwanted, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&unwanted, QStringLiteral("String(ports[1].posted[0].link.ready)")), QStringLiteral("undefined"));

    QJSEngine connecting;
    Make(&connecting, true, Fetching() + Saying());
    connecting.evaluate(QStringLiteral("document.visibilityState = 'hidden';"
                                       "for (var r = 0; r < 8; r++) replies.push({ linked: 1 });"
                                       "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&connecting, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&connecting, QStringLiteral("String(ports[0].posted[0].link.ready)")), QStringLiteral("undefined"));
    connecting.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility(); runTimers();"));
    QTRY_COMPARE(Value(&connecting, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&connecting, QStringLiteral("ports[0].posted.length + ' ' + ports[1].posted[0].link.ready + '/' + ports[1].posted[0].link.readyFrom")),
             QStringLiteral("1 2/0"));

    QJSEngine stopped;
    Make(&stopped, true, Fetching() + Saying());
    stopped.evaluate(QStringLiteral("document.readyState = 'loading';"
                                    "for (var r = 0; r < 8; r++) replies.push({ linked: 1 });"
                                    "var only = function(){ return false; };"
                                    "chrome.runtime.onMessage.addListener(only);"));
    QTRY_COMPARE(Value(&stopped, QStringLiteral("ports.length")), QStringLiteral("1"));
    stopped.evaluate(QStringLiteral("ports[0].say({ linked: 1, url: 'https://a.example/page' });"
                                    "chrome.runtime.onMessage.removeListener(only); windowLoaded();"));
    QCOMPARE(Value(&stopped, QStringLiteral("ports[0].posted.filter(function(m){ return 'ready' in m; }).length + '/' + ports[0].disconnected")),
             QStringLiteral("0/1"));
    stopped.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(only);"));
    QTRY_COMPARE(Value(&stopped, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&stopped, QStringLiteral("ports[1].posted[0].link.ready + '/' + ports[1].posted[0].link.readyFrom + ' ' + document.stages.length")),
             QStringLiteral("2/0 1"));

    QJSEngine broken;
    Make(&broken, true, Fetching() + Saying());
    broken.evaluate(QStringLiteral("document.readyState = 'interactive';"
                                   "for (var r = 0; r < 8; r++) replies.push({ linked: 1 });"
                                   "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&broken, QStringLiteral("ports.length")), QStringLiteral("1"));
    broken.evaluate(QStringLiteral("ports[0].say({ linked: 1, url: 'https://a.example/page' }); ports[0].dead = true; windowLoaded(); runTimers();"));
    QTRY_COMPARE(Value(&broken, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&broken, QStringLiteral("ports[1].posted[0].link.ready + '/' + ports[1].posted[0].link.readyFrom")), QStringLiteral("2/1"));
}

void tst_cdpshims::aNavigationWithNoNumberYetWaitsForOne(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.tabId + ' ' + d.url + ' @' + d.timeStamp); });"
        "var p = linking('41000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("clock = 5000; p.say({ nav: { kind: 'history', url: 'https://a.example/one' } });"
                                   "clock = 5100; p.say({ nav: { kind: 'history', url: 'https://a.example/two' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 5 }).then(function(f){ out = String(f.length); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("0"));

    worker.evaluate(QStringLiteral("clock = 9000;"
                                   "answers.push({ ok: true, value: { id: 900, index: 0 } }); held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")),
             QStringLiteral("900 https://a.example/one @5000 | 900 https://a.example/two @5100"));
    worker.evaluate(QStringLiteral("log = []; clock = 9500; p.say({ nav: { kind: 'history', url: 'https://a.example/three' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")), QStringLiteral("900 https://a.example/three @9500"));

    QJSEngine many;
    MakeAsking(&many, Fetching(), Saying());
    many.evaluate(Linking());
    many.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url); });"
        "var p = linking('42000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&many);
    many.evaluate(QStringLiteral("for (var i = 0; i < 12; i++) p.say({ nav: { kind: 'history', url: 'https://a.example/' + i } });"
                                 "answers.push({ ok: true, value: { id: 900, index: 0 } }); held.pop()();"));
    Settle(&many);
    QCOMPARE(Value(&many, QStringLiteral("log.map(function(u){ return u.split('/').pop(); }).join()")),
             QStringLiteral("4,5,6,7,8,9,10,11"));

    QJSEngine gone;
    MakeAsking(&gone, Fetching(), Saying());
    gone.evaluate(Linking());
    gone.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 900, index: 0 } }, { ok: true, value: { id: 900, index: 0 } });"
        "var log = [];"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url); });"
        "var first = linking('43000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&gone);
    gone.evaluate(QStringLiteral("first.say({ nav: { kind: 'history', url: 'https://a.example/mine' } });"
                                 "var second = linking('43000000000000000000000000000000', 0, 'https://a.example/top', 200);"
                                 "first.say({ nav: { kind: 'history', url: 'https://a.example/after' } });"));
    QCOMPARE(Value(&gone, QStringLiteral("log.join() + '/' + first.disconnected")),
             QStringLiteral("https://a.example/mine/1"));
    gone.evaluate(QStringLiteral("second.say({ nav: { kind: 'history', url: 'https://a.example/live' } });"));
    QCOMPARE(Value(&gone, QStringLiteral("log.join()")),
             QStringLiteral("https://a.example/mine,https://a.example/live"));

    QJSEngine waiting;
    MakeAsking(&waiting, Fetching(), Saying());
    waiting.evaluate(Linking());
    waiting.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "var log = [];"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url); });"
        "var first = linking('45000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&waiting);
    waiting.evaluate(QStringLiteral("first.say({ nav: { kind: 'history', url: 'https://a.example/waited' } });"
                                    "var second = linking('45000000000000000000000000000000', 0, 'https://a.example/top', 200);"
                                    "answers.push({ ok: true, value: { id: 900, index: 0 } }); held.pop()();"));
    Settle(&waiting);
    QCOMPARE(Value(&waiting, QStringLiteral("log.join() + '/' + first.disconnected")), QStringLiteral("/1"));

    QJSEngine quiet;
    MakeAsking(&quiet, Fetching(), Saying());
    quiet.evaluate(Linking());
    quiet.evaluate(QStringLiteral("answers.push('HELD');"
                                  "var log = [];"
                                  "var p = linking('44000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&quiet);
    quiet.evaluate(QStringLiteral("p.say({ nav: { kind: 'history', url: 'https://a.example/before' } });"
                                  "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url); });"
                                  "answers.push({ ok: true, value: { id: 900, index: 0 } }); held.pop()();"));
    Settle(&quiet);
    QCOMPARE(Value(&quiet, QStringLiteral("log.join()")), QString());
}

void tst_cdpshims::aListenerWithAUrlFilterIsNotRegistered(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var log = [];"
        "var picky = function(){ log.push('picky'); }, plain = function(){ log.push('plain'); };"
        "var nulled = function(){ log.push('nulled'); }, other = function(){ log.push('other'); };"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(picky, { url: [{ hostContains: 'a.example' }] });"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(plain);"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(nulled, null);"
        "chrome.webNavigation.onReferenceFragmentUpdated.addListener(other, {});"
        "var p = linking('51000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("[picky, plain, nulled].map(function(f){ return chrome.webNavigation.onHistoryStateUpdated.hasListener(f); }).join()"
                                           " + '/' + chrome.webNavigation.onReferenceFragmentUpdated.hasListeners()")),
             QStringLiteral("false,true,true/false"));
    QCOMPARE(Value(&worker, QStringLiteral("warned.length")), QStringLiteral("1"));
    QVERIFY2(Value(&worker, QStringLiteral("warned[0]")).startsWith(QStringLiteral("Vanilla:")),
             qPrintable(Value(&worker, QStringLiteral("warned[0]"))));
    worker.evaluate(QStringLiteral("p.say({ nav: { kind: 'history', url: 'https://a.example/next' } });"
                                   "p.say({ nav: { kind: 'fragment', url: 'https://a.example/next#x' } });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join()")), QStringLiteral("plain,nulled"));
    QCOMPARE(Value(&worker, QStringLiteral("['addRules', 'removeRules', 'getRules'].map(function(k){ return typeof chrome.webNavigation.onHistoryStateUpdated[k]; }).join()")),
             QStringLiteral("function,function,function"));
}

void tst_cdpshims::whatThePageDidWhileThePortWasAnsweredIsCaughtUpWith(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + Navigating());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 }, { linked: 1 }, { linked: 1 }, { linked: 1 }, { linked: 1 });"
                                 "var mine = function(){ return false; };"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral(
        "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
        "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"
        "navigate('https://a.example/page?pushed=1'); commit('https://a.example/page?pushed=1');"
        "runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs() + '/' + timers.filter(function(t){ return t.ms === 100; }).length")),
             QStringLiteral("/0"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1, url: 'https://a.example/page' }); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/page?pushed=1"));
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length")), QStringLiteral("0"));

    page.evaluate(QStringLiteral("clock += 25000; ports[0].die(); runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral("ports[1].say({ linked: 1, url: 'https://a.example/page?pushed=1' }); runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1]) + '/' + timers.filter(function(t){ return t.ms === 100; }).length")),
             QStringLiteral("/0"));

    const QStringList quiet = QStringList() << QStringLiteral("{ linked: 1 }")
                                            << QStringLiteral("{ linked: 1, url: 5 }")
                                            << QStringLiteral("{ linked: 1, url: '' }");
    for(int i = 0; i < quiet.size(); i++){
        const int port = 2 + i;
        page.evaluate(QStringLiteral("clock += 25000; ports[%1].die(); runTimers();").arg(port - 1));
        QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QString::number(port + 1));
        page.evaluate(QStringLiteral("navigate('https://a.example/page?pushed=%1'); commit('https://a.example/page?pushed=%1');"
                                     "runNextTimer();"
                                     "ports[%2].say(%3); runTimers();").arg(port).arg(port).arg(quiet[i]));
        Pump(&page);
        QCOMPARE(Value(&page, QStringLiteral("navs(ports[%1]) + '/' + timers.filter(function(t){ return t.ms === 100; }).length").arg(port)),
                 QStringLiteral("/0"));
    }
}

void tst_cdpshims::aDocumentWhichPushedTellsTheWorkerWhereItIs(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + Navigating());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                 "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral(
        "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
        "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"
        "location.href = 'https://a.example/page?pushed=1';"
        "ports[0].say({ linked: 1, url: 'https://a.example/page' }); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/page?pushed=1"));
    QJSEngine hidden;
    Make(&hidden, true, Fetching() + Saying() + Navigating());
    hidden.evaluate(QStringLiteral("document.visibilityState = 'hidden';"
                                   "chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&hidden, QStringLiteral("ports.length")), QStringLiteral("1"));
    hidden.evaluate(QStringLiteral(
        "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
        "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"
        "location.href = 'https://a.example/page?pushed=1';"
        "ports[0].say({ linked: 1, url: 'https://a.example/page' }); runTimers();"));
    Pump(&hidden);
    QCOMPARE(Value(&hidden, QStringLiteral("navs()")), QString());
    hidden.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility(); runNextTimer();"));
    QCOMPARE(Value(&hidden, QStringLiteral("navs()")), QStringLiteral("history https://a.example/page?pushed=1"));

    QJSEngine worker;
    Make(&worker, false, Fetching() + Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral("Math.random = function(){ return 0.25; };"
                                   "var named = linking('71000000000000000000000000000000', 0, 'https://a.example/top', 100);"
                                   "var bare = aPort('__vanilla_link__', { id: chrome.runtime.id }); connects[0](bare);"
                                   "bare.say({ link: { nonce: '72000000000000000000000000000000', frameId: 2, tabUrl: 'u', since: 200 } });"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([named.last(), bare.last()])")),
             QStringLiteral("[{\"linked\":1,\"url\":\"https://a.example/top\"},{\"linked\":1,\"url\":\"\"}]"));
    worker.evaluate(QStringLiteral("var log = [];"
                                   "chrome.webNavigation.onHistoryStateUpdated.addListener(function(d){ log.push(d.url); });"
                                   "named.say({ nav: { kind: 'history', url: 'https://a.example/top?pushed=1' } });"
                                   "var out = 'pending';"
                                   "chrome.webNavigation.getAllFrames({ tabId: 1342177280 }).then(function(f){"
                                   "  out = f.filter(function(x){ return x.frameId === 0; })"
                                   "    .map(function(x){ return x.frameId + ':' + x.url; }).join(); });"));
    QCOMPARE(Value(&worker, QStringLiteral("log.join()")), QStringLiteral("https://a.example/top?pushed=1"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("0:https://a.example/top?pushed=1"));
}

void tst_cdpshims::theKindOfANavigationOutlivesNotBeingAbleToSendIt(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + Navigating());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 });"
                                 "var mine = function(){ return false; };"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral(
        "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
        "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"
        "navigate('https://a.example/page#x', false); commit('https://a.example/page#x');"
        "runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QString());
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1, url: 'https://a.example/page' }); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/page#x"));

    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "navigate('https://a.example/page#y', false); commit('https://a.example/page#y');"
                                 "runTimers();"));
    Pump(&page);
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/page#x"));
    page.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")),
             QStringLiteral("history https://a.example/page#x | history https://a.example/page#y"));

    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "navigate('https://a.example/page#z', false); commit('https://a.example/page#z');"
                                 "runTimers();"
                                 "commit('https://a.example/page#q'); runTimers();"));
    Pump(&page);
    page.evaluate(QStringLiteral("document.visibilityState = 'visible'; document.onVisibility(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs().split(' | ').pop()")), QStringLiteral("fragment https://a.example/page#q"));

    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "navigate('https://a.example/page#r', false); commit('https://a.example/page#r');"
                                 "runTimers();"
                                 "chrome.runtime.onMessage.removeListener(mine);"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral("ports[1].say({ linked: 1, url: 'https://a.example/page' });"
                                 "document.visibilityState = 'visible'; document.onVisibility(); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QStringLiteral("fragment https://a.example/page#r"));
}

void tst_cdpshims::nothingOfThePagesNavigationsIsKeptWithNobodyListening(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + Navigating());
    page.evaluate(QStringLiteral("replies.push({ linked: 1 }, { linked: 1 }, { linked: 1 });"
                                 "var mine = function(){ return false; };"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral(
        "ports[0].say({ linked: 1 });"
        "function navs(p){ return (p || ports[0]).posted.filter(function(m){ return !!m.nav; })"
        "  .map(function(m){ return m.nav.kind + ' ' + m.nav.url; }).join(' | '); }"
        "navigate('https://a.example/third', true);"
        "chrome.runtime.onMessage.removeListener(mine);"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + '/' + ports[0].disconnected")), QStringLiteral("0/1"));

    page.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral("ports[1].say({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QString());
    page.evaluate(QStringLiteral("commit('https://a.example/third'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[1])")), QStringLiteral("history https://a.example/third"));

    page.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(mine);"
                                 "navigate('https://a.example/away', true); commit('https://a.example/away');"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("0"));

    page.evaluate(QStringLiteral("navigate('https://a.example/fourth', true);"
                                 "chrome.runtime.onMessage.addListener(mine);"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("3"));
    page.evaluate(QStringLiteral("ports[2].say({ linked: 1 });"
                                 "commit('https://a.example/fourth'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs(ports[2])")), QStringLiteral("history https://a.example/fourth"));
}

void tst_cdpshims::anAddressTooLongToBelieveIsNotSent(){
    QJSEngine page;
    Linked(&page, Navigating());
    page.evaluate(QStringLiteral("var far = 'https://a.example/' + new Array(8200).join('x');"
                                 "navigate(far); commit(far); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs() + '/' + (far.length > 8192)")), QStringLiteral("/true"));
    page.evaluate(QStringLiteral("document.visibilityState = 'hidden'; document.onVisibility();"
                                 "document.visibilityState = 'visible'; document.onVisibility();"));
    QCOMPARE(Value(&page, QStringLiteral("timers.filter(function(t){ return t.ms === 100; }).length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("navigate('https://a.example/short'); commit('https://a.example/short'); runNextTimer();"));
    QCOMPARE(Value(&page, QStringLiteral("navs()")), QStringLiteral("history https://a.example/short"));
}

void tst_cdpshims::theOriginsWhichMatchAndTheOnesWhichDoNot(){
    const QString table = QStringLiteral(
        "var cases = ["
        "  ['https://a.example/top', 'https://a.example/other', 'both'],"
        "  ['https://a.example/top', 'https://A.EXAMPLE/other', 'both'],"
        "  ['https://a.example/top', 'HTTPS://a.example/other', 'both'],"
        "  ['https://a.example/top', 'https://a.example/x?q=1#f', 'both'],"
        "  ['http://a.example/top', 'http://a.example/x', 'both'],"
        "  ['https://[::1]/top', 'https://[::1]/other', 'both'],"
        "  ['https://a.example/top', 'https://a.example:443/x', 'url'],"
        "  ['https://a.example:443/top', 'https://a.example/x', 'url'],"
        "  ['https://a.example/top', 'https://user:pw@a.example/x', 'url'],"
        "  ['https://a.example/top', 'https://a.example./x', 'neither'],"
        "  ['https://a.example/top', 'https://a.example:8443/x', 'neither'],"
        "  ['https://a.example/top', 'http://a.example/x', 'neither'],"
        "  ['https://a.example/top', 'https://sub.a.example/x', 'neither'],"
        "  ['https://[::1]/top', 'https://[::1]:8443/x', 'neither'],"
        "  ['https://a.example/top', 'blob:https://a.example/x', 'neither'],"
        "  ['https://a.example/top', 'data:text/html,x', 'neither'],"
        "  ['about:blank', 'about:blank', 'neither'],"
        "  ['foo://a.example/x', 'foo://a.example/y', 'neither'],"
        "  ['https://a.example/top', '/relative', 'neither']];"
        "var fired = 0, mine = [];"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(){ fired++; });"
        "cases.forEach(function(c, i){"
        "  answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "  mine.push(linking(('0' + (10 + i)).slice(-2) + '000000000000000000000000000000', 0, c[0], 100 + i));"
        "});");
    const QString asking = QStringLiteral(
        "var got = cases.map(function(c, i){"
        "  var before = fired;"
        "  mine[i].say({ nav: { kind: 'history', url: c[1] } });"
        "  return (fired > before ? 'match' : 'no') + ' ' + c[1];"
        "}).join('\\n');"
        "var want = cases.map(function(c){"
        "  return ((c[2] === 'both' || (hasUrl && c[2] === 'url')) ? 'match' : 'no') + ' ' + c[1];"
        "}).join('\\n');");

    QJSEngine strict;
    MakeAsking(&strict, Fetching(), Saying() + QStringLiteral("URL = undefined; var hasUrl = false;"));
    strict.evaluate(Linking());
    strict.evaluate(table);
    Settle(&strict);
    strict.evaluate(asking);
    QCOMPARE(Value(&strict, QStringLiteral("typeof URL")), QStringLiteral("undefined"));
    QCOMPARE(Value(&strict, QStringLiteral("got")), Value(&strict, QStringLiteral("want")));

    QJSEngine browser;
    MakeAsking(&browser, Fetching(), Saying() + Urling() + QStringLiteral("var hasUrl = true;"));
    QCOMPARE(Value(&browser, QStringLiteral("new URL('https://user:pw@A.EXAMPLE:443/x?q#f').origin")),
             QStringLiteral("https://a.example"));
    browser.evaluate(Linking());
    browser.evaluate(table);
    Settle(&browser);
    browser.evaluate(asking);
    QCOMPARE(Value(&browser, QStringLiteral("got")), Value(&browser, QStringLiteral("want")));
    QCOMPARE(Value(&browser, QStringLiteral("['both', 'url', 'neither'].map(function(k){"
                                            "  return cases.filter(function(c){ return c[2] === k; }).length > 0; }).join()")),
             QStringLiteral("true,true,true"));
}

void tst_cdpshims::outsideAnExtensionsPageTheShimDoesNothing(){
    const QString untouched = QStringLiteral(
        "String(self.__vanillaPageShim) + '/' + (chrome.storage.sync.get === refusing)"
        " + '/' + reads.length + '/' + timers.length");

    QJSEngine noId;
    noId.evaluate(World());
    noId.evaluate(AtAPage() + QStringLiteral("chrome.runtime.id = undefined;"));
    QCOMPARE(noId.evaluate(Cdp::PageShim()).toString(), QStringLiteral("not an extension"));
    QCOMPARE(Value(&noId, untouched), QStringLiteral("undefined/true/0/0"));

    QJSEngine web;
    web.evaluate(World());
    web.evaluate(QStringLiteral("location = { href: 'https://a.example/page', protocol: 'https:' };"));
    QCOMPARE(web.evaluate(Cdp::PageShim()).toString(), QStringLiteral("not an extension"));
    QCOMPARE(Value(&web, untouched), QStringLiteral("undefined/true/0/0"));

    QJSEngine worker;
    worker.evaluate(World());
    worker.evaluate(AtAPage() + QStringLiteral("document = undefined;"));
    QCOMPARE(worker.evaluate(Cdp::PageShim()).toString(), QStringLiteral("not an extension"));
    QCOMPARE(Value(&worker, untouched), QStringLiteral("undefined/true/0/0"));

    QJSEngine bare;
    bare.evaluate(QStringLiteral("var self = this;"));
    QCOMPARE(bare.evaluate(Cdp::PageShim()).toString(), QStringLiteral("not an extension"));
}

void tst_cdpshims::anExtensionsPageGetsTheShimOnlyOnce(){
    QJSEngine twice;
    MakePage(&twice);
    QCOMPARE(twice.evaluate(Cdp::PageShim()).toString(), QStringLiteral("already"));
}

void tst_cdpshims::anExtensionsPageHasSyncAndTheChangeEvents(){
    QJSEngine page;
    MakePage(&page);
    page.evaluate(QStringLiteral(
        "var perArea = [];"
        "localArea.data['__vanilla_sync__:scrollStepSize'] = 60;"
        "chrome.storage.onChanged.addListener(function(c, a){ heard.push(a + ':' + JSON.stringify(c)); });"
        "chrome.storage.sync.onChanged.addListener(function(c){ perArea.push('sync:' + Object.keys(c).join('+')); });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("reads.length >= 2")), QStringLiteral("true"));
    QCoreApplication::processEvents();

    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.storage.sync.get({ scrollStepSize: 0, missing: 'default' })"
        "  .then(function(v){ out = JSON.stringify(v); return chrome.storage.sync.set({ scrollStepSize: 77 }); });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")),
                 QStringLiteral("{\"scrollStepSize\":60,\"missing\":\"default\"}"));
    QTRY_COMPARE(Value(&page, QStringLiteral("heard.join(' | ')")),
                 QStringLiteral("sync:{\"scrollStepSize\":{\"oldValue\":60,\"newValue\":77}}"));
    QCOMPARE(Value(&page, QStringLiteral("perArea.join()")), QStringLiteral("sync:scrollStepSize"));
    QCOMPARE(Value(&page, QStringLiteral("Object.keys(localArea.data).join()")),
             QStringLiteral("__vanilla_sync__:scrollStepSize"));

    QCOMPARE(Value(&page, QStringLiteral("timers.length")), QStringLiteral("1"));
    page.evaluate(QStringLiteral("heard = []; localArea.data['__vanilla_sync__:excluded'] = 'x'; runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("heard.join(' | ')")),
                 QStringLiteral("sync:{\"excluded\":{\"newValue\":\"x\"}}"));
}

void tst_cdpshims::anExtensionsPageAsksTheApplicationNothing(){
    QJSEngine page;
    MakePage(&page, Fetching());
    QVERIFY(Cdp::PageShim().contains(QLatin1String(ExtensionHostWire::KEY_PLACE)));

    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.tabs.query({ active: true }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length")), QStringLiteral("0"));

    page.evaluate(QStringLiteral(
        "var called = 'pending';"
        "chrome.windows.getCurrent(function(w){ called = String(w) + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR'); });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("timers.length >= 1")), QStringLiteral("true"));
    page.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("called")),
                 QStringLiteral("undefined chrome.windows.getCurrent is not available in this browser"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length + '/' + String(chrome.runtime.lastError)")), QStringLiteral("0/undefined"));
}

void tst_cdpshims::aPageHasNoPortsSoWhatTheyAnswerForFails(){
    QJSEngine page;
    MakePage(&page, Fetching() + EnginesOwn());
    page.evaluate(QStringLiteral(
        "var frames = 'pending';"
        "chrome.webNavigation.getAllFrames({ tabId: 1 }).then(function(){ frames = 'resolved'; }, function(e){ frames = e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("frames")), QStringLiteral("chrome.webNavigation.getAllFrames is not available in this browser"));
    QJSEngine mute;
    mute.evaluate(World());
    mute.evaluate(AtAPage() + QStringLiteral("chrome.runtime.sendMessage = undefined;"));
    QVERIFY(!mute.evaluate(Cdp::PageShim()).isError());
    mute.evaluate(QStringLiteral(
        "var toTab = 'pending';"
        "chrome.tabs.sendMessage(1, {}).then(function(){ toTab = 'resolved'; }, function(e){ toTab = e.message; });"));
    QTRY_COMPARE(Value(&mute, QStringLiteral("toTab")), QStringLiteral("chrome.tabs.sendMessage is not available in this browser"));

    page.evaluate(QStringLiteral("var kept = function(){}; chrome.webNavigation.onHistoryStateUpdated.addListener(kept);"));
    QCOMPARE(Value(&page, QStringLiteral(
                 "chrome.webNavigation.onHistoryStateUpdated.hasListener(kept) + '/' + ports.length + '/' + fetched.length + '/' + timers.length")),
             QStringLiteral("false/0/0/0"));

    QCOMPARE(Value(&page, QStringLiteral("chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1"));
}

void tst_cdpshims::aPagesStandInIsNoThenableAndHasNoJson(){
    QJSEngine page;
    MakePage(&page);
    QCOMPARE(Value(&page, QStringLiteral(
                 "String(chrome.tabs.then) + '/' + String(chrome.windows.toJSON)"
                 " + '/' + ('then' in chrome.tabs) + '/' + ('toJSON' in chrome.windows)")),
             QStringLiteral("undefined/undefined/false/false"));
}

void tst_cdpshims::aPageIsToldItIsNotInAnIncognitoWindow(){
    QJSEngine page;
    MakePage(&page);
    QCOMPARE(Value(&page, QStringLiteral("String(chrome.extension.inIncognitoContext)")), QStringLiteral("false"));

    QJSEngine already;
    MakePage(&already, QStringLiteral(
        "var enginesExtension = { inIncognitoContext: true, getURL: function(){ return 'x'; } };"
        "chrome.extension = enginesExtension;"));
    QCOMPARE(Value(&already, QStringLiteral("(chrome.extension === enginesExtension) + '/' + chrome.extension.inIncognitoContext")),
             QStringLiteral("true/true"));
}

void tst_cdpshims::thePagesShimNeitherWakesTheWorkerNorReachesIt(){
    QJSEngine page;
    MakePage(&page, Fetching());
    const QString reached = QStringLiteral("sent.length + '/' + ports.length + '/' + connects.length + '/' + fetched.length");
    QCOMPARE(Value(&page, reached), QStringLiteral("0/0/0/0"));

    page.evaluate(QStringLiteral(
        "chrome.storage.onChanged.addListener(function(c, a){ heard.push(a); });"
        "chrome.storage.sync.set({ a: 1 });"
        "chrome.tabs.query({}).then(function(){}, function(){});"
        "chrome.webNavigation.onHistoryStateUpdated.addListener(function(){});"));
    Pump(&page);
    QCOMPARE(Value(&page, reached), QStringLiteral("0/0/0/0"));
    QCOMPARE(Value(&page, QStringLiteral("(reads.length >= 2) + '/' + heard.join()")), QStringLiteral("true/sync"));
}

void tst_cdpshims::thePagesShimCarriesTheEnvelopeAndNotTheSubscription(){
    const QString page = Cdp::PageShim();
    QVERIFY(page.contains(QStringLiteral("installEnvelope(")));
    QVERIFY(!page.contains(QStringLiteral("installEvents(")));
    QVERIFY(!page.contains(QStringLiteral("__vanilla_link__")));
    QVERIFY(!page.contains(QStringLiteral("installLink(")));
    QVERIFY(Cdp::WorkerShim().contains(QStringLiteral("installEnvelope(")));
    QVERIFY(Cdp::WorkerShim().contains(QStringLiteral("installEvents(")));
    QVERIFY(Cdp::WorkerShim().contains(QStringLiteral("installLink(")));
}

void tst_cdpshims::anExtensionsPageSpeaksInTheSameEnvelope(){
    QJSEngine page;
    MakePage(&page, Fetching());
    QCOMPARE(Value(&page, QStringLiteral("fetched.length + '/' + sent.length")), QStringLiteral("0/0"));

    page.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'x' });"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length + ' ' + fetched[0].url + ' ' + fetched[0].options.method")),
             QStringLiteral("1 vanilla-extension://host/bind POST"));
    const QString nonce = Value(&page, QStringLiteral("fetched[0].options.headers['X-Vanilla-Nonce']"));
    QVERIFY2(QRegularExpression(QStringLiteral("\\A[0-9a-f]{32}\\z")).match(nonce).hasMatch(), qPrintable(nonce));
    QVERIFY(!Value(&page, QStringLiteral("JSON.stringify(fetched[0].options.headers)")).contains(QStringLiteral("Key")));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(sent[0][0])")),
             QStringLiteral("{\"__vanillaEnvelope\":1,\"from\":{\"nonce\":\"%1\",\"frameId\":0,"
                            "\"tabUrl\":\"chrome-extension://abcdefghijklmnopabcdefghijklmnop/pages/options.html\"},"
                            "\"message\":{\"handler\":\"x\"}}").arg(nonce));

    page.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'y' });"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length + ' ' + (sent[1][0].from.nonce === sent[0][0].from.nonce) + ' ' + sent[1][0].message.handler")),
             QStringLiteral("1 true y"));
    page.evaluate(QStringLiteral("chrome.runtime.sendMessage('ppppppppppppppppppppppppppppppppp', { hello: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("sent[2][0] + ' ' + JSON.stringify(sent[2][1])")),
             QStringLiteral("ppppppppppppppppppppppppppppppppp {\"hello\":1}"));

    QJSEngine frame;
    MakePage(&frame, Fetching() + QStringLiteral(
        "window.top = { get location(){ throw new Error('cross-origin'); } };"
        "location = { href: 'chrome-extension://abcdefghijklmnopabcdefghijklmnop/pages/vomnibar.html',"
        "             protocol: 'chrome-extension:', ancestorOrigins: ['https://a.example'] };"));
    frame.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'filterCompletions' });"));
    QCOMPARE(Value(&frame, QStringLiteral("JSON.stringify([sent[0][0].from.frameId !== 0, sent[0][0].from.tabUrl, fetched[0].url])")),
             QStringLiteral("[true,\"https://a.example/\",\"vanilla-extension://host/bind\"]"));

    QJSEngine keyed;
    MakeKeyedPage(&keyed, Fetching());
    keyed.evaluate(QStringLiteral("replies.push({ ok: true, value: 'said' }); chrome.tabs.sendMessage(1, { a: 1 });"));
    QCOMPARE(Value(&keyed, QStringLiteral("JSON.stringify(sent[0][0]) + '/' + fetched.length")),
             QStringLiteral("{\"__vanillaRelay\":1,\"api\":\"tabs.sendMessage\",\"args\":[1,{\"a\":1}]}/0"));
    keyed.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'z' });"));
    QCOMPARE(Value(&keyed, QStringLiteral("sent[1][0].__vanillaEnvelope + '/' + sent[1][0].message.handler + '/' + fetched[0].url")),
             QStringLiteral("1/z/vanilla-extension://host/bind"));
    keyed.evaluate(QStringLiteral("chrome.tabs.sendMessage(2, { b: 2 });"));
    QCOMPARE(Value(&keyed, QStringLiteral("JSON.stringify(sent[2][0])")),
             QStringLiteral("{\"__vanillaRelay\":1,\"api\":\"tabs.sendMessage\",\"args\":[2,{\"b\":2}]}"));
}

void tst_cdpshims::theWorkerNamesAPageOfThisExtensionOrGivesItNoTab(){
    const QString listening = QStringLiteral(
        "var got = [], last = null;"
        "chrome.runtime.onMessage.addListener(function(m, s){ last = s;"
        "  got.push(JSON.stringify(m) + ' tab=' + (('tab' in s) ? s.tab.id : 'none') + ' frame=' + s.frameId"
        "           + ' life=' + s.documentLifecycle); return false; });"
        "var vomnibar = { id: chrome.runtime.id,"
        "                 url: 'chrome-extension://' + chrome.runtime.id + '/pages/vomnibar.html',"
        "                 origin: 'chrome-extension://' + chrome.runtime.id };"
        "var inATab = { id: chrome.runtime.id, url: 'https://a.example/', origin: 'https://a.example' };"
        "function say(nonce, frame, handler, who){"
        "  listeners[0]({ __vanillaEnvelope: 1, from: { nonce: nonce, frameId: frame, tabUrl: 'https://a.example/' },"
        "                 message: { handler: handler } }, who || vomnibar, function(){}); }");
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(listening);

    worker.evaluate(QStringLiteral("held = []; answers.push('HELD');"
                                   "say('a1000000000000000000000000000000', 77, 'filterCompletions');"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length + '/' + fetched.length")), QStringLiteral("0/1"));
    worker.evaluate(QStringLiteral("answers[answers.length] = { ok: true, value: { id: 4, index: 0 } };"
                                   "held.forEach(function(f){ f(); }); held = [];"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0]")),
             QStringLiteral("{\"handler\":\"filterCompletions\"} tab=4 frame=77 life=active"));

    const int before = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false });"
        "say('a2000000000000000000000000000000', 88, 'launchSearchQuery');"));
    for(int attempt = 1; attempt < 6; attempt++){
        QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms < 3000; })")), QStringLiteral("true"));
        worker.evaluate(QStringLiteral("runNextTimer();"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt() - before, attempt + 1);
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("got[1]")),
             QStringLiteral("{\"handler\":\"launchSearchQuery\"} tab=none frame=88 life=active"));
    QCOMPARE(Value(&worker, QStringLiteral("last.url + ' ' + last.origin")),
             QStringLiteral("chrome-extension://abcdefghijklmnopabcdefghijklmnop/pages/vomnibar.html"
                            " chrome-extension://abcdefghijklmnopabcdefghijklmnop"));

    const int after = Value(&worker, QStringLiteral("fetched.length")).toInt();
    worker.evaluate(QStringLiteral("say('a2000000000000000000000000000000', 88, 'again');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("3"));
    QCOMPARE(Value(&worker, QStringLiteral("got[2]")), QStringLiteral("{\"handler\":\"again\"} tab=none frame=88 life=active"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), after);

    QJSEngine web;
    MakeAsking(&web, Fetching());
    web.evaluate(listening + QStringLiteral("Math.random = function(){ return 0.25; };"));
    web.evaluate(QStringLiteral(
        "answers.push({ ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false });"
        "say('a3000000000000000000000000000000', 0, 'fromAPage', inATab);"));
    for(int attempt = 1; attempt < 6; attempt++){
        QTRY_COMPARE(Value(&web, QStringLiteral("timers.some(function(t){ return t.ms < 3000; })")), QStringLiteral("true"));
        web.evaluate(QStringLiteral("runNextTimer();"));
        QTRY_COMPARE(Value(&web, QStringLiteral("fetched.length")).toInt(), attempt + 1);
    }
    QTRY_COMPARE(Value(&web, QStringLiteral("got.length")), QStringLiteral("1"));
    QCOMPARE(Value(&web, QStringLiteral("got[0]")),
             QStringLiteral("{\"handler\":\"fromAPage\"} tab=1342177280 frame=0 life=active"));

    QJSEngine keyless;
    Make(&keyless, false);
    keyless.evaluate(listening + QStringLiteral("Math.random = function(){ return 0.25; };"));
    keyless.evaluate(QStringLiteral("say('a4000000000000000000000000000000', 5, 'nobodyToAsk');"
                                    "say('a5000000000000000000000000000000', 0, 'fromAPage', inATab);"));
    QCOMPARE(Value(&keyless, QStringLiteral("got.join(' | ')")),
             QStringLiteral("{\"handler\":\"nobodyToAsk\"} tab=none frame=5 life=active"
                            " | {\"handler\":\"fromAPage\"} tab=1342177280 frame=0 life=active"));
}

void tst_cdpshims::theTopFrameOfAPageIsNotWaitedForAndIsAskedAfterOnce(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    const QString world = QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){"
        "  got.push(JSON.stringify(m) + ' tab=' + (('tab' in s) ? s.tab.id : 'none') + ' frame=' + s.frameId"
        "           + ' life=' + s.documentLifecycle); return false; });"
        "var popup = { id: chrome.runtime.id, url: 'chrome-extension://' + chrome.runtime.id + '/pages/popup.html',"
        "              origin: 'chrome-extension://' + chrome.runtime.id };"
        "function say(nonce, handler){"
        "  listeners[0]({ __vanillaEnvelope: 1, from: { nonce: nonce, frameId: 0, tabUrl: 'chrome-extension://x/popup.html' },"
        "                 message: { handler: handler } }, popup, function(){}); }");
    worker.evaluate(world);
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false });"
        "say('b1000000000000000000000000000000', 'first');"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0]")), QStringLiteral("{\"handler\":\"first\"} tab=none frame=0 life=active"));

    Drain(&worker);
    const int asked = Value(&worker, QStringLiteral("fetched.length")).toInt();
    QCOMPARE(asked, 6);
    worker.evaluate(QStringLiteral("say('b1000000000000000000000000000000', 'second');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("got[1]")), QStringLiteral("{\"handler\":\"second\"} tab=none frame=0 life=active"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked);
    QCOMPARE(Value(&worker, QStringLiteral("timers.length")), QStringLiteral("0"));

    QJSEngine tabbed;
    MakeAsking(&tabbed, Fetching());
    tabbed.evaluate(world);
    tabbed.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 9, index: 0 } });"
                                   "say('b2000000000000000000000000000000', 'first');"));
    QCOMPARE(Value(&tabbed, QStringLiteral("got[0]")), QStringLiteral("{\"handler\":\"first\"} tab=none frame=0 life=active"));
    Settle(&tabbed);
    tabbed.evaluate(QStringLiteral("say('b2000000000000000000000000000000', 'second');"));
    QTRY_COMPARE(Value(&tabbed, QStringLiteral("got.length")), QStringLiteral("2"));
    QCOMPARE(Value(&tabbed, QStringLiteral("got[1]")), QStringLiteral("{\"handler\":\"second\"} tab=9 frame=0 life=active"));
    QCOMPARE(Value(&tabbed, QStringLiteral("fetched.length")), QStringLiteral("1"));
}

void tst_cdpshims::aPageWithNoTabIsCorrectedWhenTheHostNamesItLate(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){"
        "  got.push(JSON.stringify(m) + ' tab=' + (('tab' in s) ? s.tab.id : 'none') + ' frame=' + s.frameId"
        "           + ' life=' + s.documentLifecycle); return false; });"
        "var vomnibar = { id: chrome.runtime.id,"
        "                 url: 'chrome-extension://' + chrome.runtime.id + '/pages/vomnibar.html',"
        "                 origin: 'chrome-extension://' + chrome.runtime.id };"
        "function say(handler){"
        "  listeners[0]({ __vanillaEnvelope: 1, from: { nonce: 'c1000000000000000000000000000000', frameId: 77,"
        "                                               tabUrl: 'https://a.example/' },"
        "                 message: { handler: handler } }, vomnibar, function(){}); }"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false }, { ok: false });"
        "say('filterCompletions');"));
    for(int attempt = 1; attempt < 6; attempt++){
        QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms < 1000; })")), QStringLiteral("true"));
        worker.evaluate(QStringLiteral("runNextTimer();"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), attempt + 1);
    }
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("got[0]")),
             QStringLiteral("{\"handler\":\"filterCompletions\"} tab=none frame=77 life=active"));
    const int asked = Value(&worker, QStringLiteral("fetched.length")).toInt();
    QCOMPARE(asked, 6);

    QTRY_COMPARE(Value(&worker, QStringLiteral("timers.some(function(t){ return t.ms === 1000; })")), QStringLiteral("true"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true, value: { id: 4, index: 0 } }); runNextTimer();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked + 1);
    Settle(&worker);

    worker.evaluate(QStringLiteral("say('corrected');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("got[1]")),
             QStringLiteral("{\"handler\":\"corrected\"} tab=4 frame=77 life=active"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked + 1);
    Drain(&worker);
    worker.evaluate(QStringLiteral("say('later');"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("3"));
    QCOMPARE(Value(&worker, QStringLiteral("got[2]")), QStringLiteral("{\"handler\":\"later\"} tab=4 frame=77 life=active"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")).toInt(), asked + 1);
}

void tst_cdpshims::aPageWhoseSendMessageCannotBeWrappedSaysSoAndStillRelays(){
    QJSEngine page;
    MakePage(&page, Fetching() + Saying() + QStringLiteral(
        "var realSend = chrome.runtime.sendMessage;"
        "Object.defineProperty(chrome.runtime, 'sendMessage',"
        "                      { value: realSend, writable: false, configurable: false, enumerable: true });"));
    QCOMPARE(Value(&page, QStringLiteral("warned.length")), QStringLiteral("1"));
    QVERIFY2(Value(&page, QStringLiteral("warned[0]")).contains(QStringLiteral("runtime.sendMessage")),
             qPrintable(Value(&page, QStringLiteral("warned[0]"))));
    QCOMPARE(Value(&page, QStringLiteral("chrome.runtime.sendMessage === realSend")), QStringLiteral("true"));

    page.evaluate(QStringLiteral("chrome.runtime.sendMessage({ handler: 'x' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(sent[0][0]) + '/' + fetched.length")),
             QStringLiteral("{\"handler\":\"x\"}/0"));

    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "replies.push({ ok: true, value: 'said' });"
        "chrome.tabs.sendMessage(1, { a: 1 }).then(function(v){ out = 'resolved ' + v; }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(sent[1][0])")),
             QStringLiteral("{\"__vanillaRelay\":1,\"api\":\"tabs.sendMessage\",\"args\":[1,{\"a\":1}]}"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("resolved said"));
}

void tst_cdpshims::aPingIsWhenTheWorkerLooksAtLocalAgain(){
    QJSEngine worker;
    Pinging(&worker);
    worker.evaluate(QStringLiteral("localArea.data.rules = 'old'; hearing();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + heard.length")), QStringLiteral("1/0"));

    worker.evaluate(QStringLiteral("localArea.data['__vanilla_sync__:exclusionRules'] = '[]'; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.join(' | ')")),
             QStringLiteral("sync:{\"exclusionRules\":{\"newValue\":\"[]\"}}"));

    worker.evaluate(QStringLiteral("heard = []; clock += 2000; localArea.data.rules = 'new'; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.join(' | ')")),
             QStringLiteral("local:{\"rules\":{\"oldValue\":\"old\",\"newValue\":\"new\"}}"));

    worker.evaluate(QStringLiteral("heard = []; clock += 2000; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.length + '/' + localReads()")), QStringLiteral("0/4"));
    QCOMPARE(Value(&worker, QStringLiteral("reads.filter(function(a){ return a === 'session'; }).length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("link.posted.length + '/' + link.disconnected")), QStringLiteral("1/0"));
}

void tst_cdpshims::aWorkerWhichHearsOfNoneOfItLooksAtNothing(){
    QJSEngine quiet;
    Pinging(&quiet);
    quiet.evaluate(QStringLiteral("localArea.data.big = 'megabytes'; link.say({ ping: 1 });"));
    Settle(&quiet);
    QCOMPARE(Value(&quiet, QStringLiteral("reads.length + '/' + link.disconnected")), QStringLiteral("0/0"));

    QJSEngine session;
    Pinging(&session);
    session.evaluate(QStringLiteral("chrome.storage.session.onChanged.addListener(function(){ heard.push('session'); });"));
    Settle(&session);
    QCOMPARE(Value(&session, QStringLiteral("reads.join()")), QStringLiteral("session"));
    session.evaluate(QStringLiteral("localArea.data.big = 'megabytes'; link.say({ ping: 1 });"));
    Settle(&session);
    QCOMPARE(Value(&session, QStringLiteral("reads.join()")), QStringLiteral("session"));

    QJSEngine gone;
    Pinging(&gone);
    gone.evaluate(QStringLiteral("var ear = function(c, a){ heard.push(a); }; chrome.storage.onChanged.addListener(ear);"));
    Settle(&gone);
    QCOMPARE(Value(&gone, QStringLiteral("localReads()")), QStringLiteral("1"));
    gone.evaluate(QStringLiteral("chrome.storage.onChanged.removeListener(ear);"
                                 "localArea.data.big = 'megabytes'; link.say({ ping: 1 });"));
    Settle(&gone);
    QCOMPARE(Value(&gone, QStringLiteral("localReads() + '/' + heard.length")), QStringLiteral("1/0"));
}

void tst_cdpshims::theLookingAgainIsAtMostOneEveryTwoSeconds(){
    QJSEngine worker;
    Pinging(&worker);
    worker.evaluate(QStringLiteral("hearing();"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("for (var i = 0; i < 5; i++) link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads()")), QStringLiteral("2"));

    worker.evaluate(QStringLiteral("clock += 1999; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads()")), QStringLiteral("2"));
    worker.evaluate(QStringLiteral("clock += 1; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads()")), QStringLiteral("3"));
}

void tst_cdpshims::whatThisContextWritesDoesNotHoldOffThePingsLook(){
    QJSEngine worker;
    Pinging(&worker);
    worker.evaluate(QStringLiteral("hearing();"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("chrome.storage.local.set({ ours: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + heard.join('')")),
             QStringLiteral("2/local:{\"ours\":{\"newValue\":1}}"));

    worker.evaluate(QStringLiteral("heard = []; localArea.data.theirs = 2; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + heard.join('')")),
             QStringLiteral("3/local:{\"theirs\":{\"newValue\":2}}"));
}

void tst_cdpshims::nothingButAPingLooksAgain(){
    QJSEngine worker;
    Pinging(&worker);
    worker.evaluate(QStringLiteral("hearing();"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "localArea.data.theirs = 1;"
        "link.say({ re: 7 });"
        "link.say({ nav: { kind: 'history', url: 'https://a.example/next' } });"
        "link.say({});"
        "link.say({ ping: 2 });"
        "link.say('ping');"
        "link.say(null);"
        "link.say({ re: 7, ping: 1 });"
        "link.say({ unknown: 1, ping: 1 });"
        "link.say({ nav: { kind: 'history', url: 'https://a.example/next' }, ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + heard.length + '/' + link.disconnected")),
             QStringLiteral("1/0/0"));

    worker.evaluate(QStringLiteral("var early = naming({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + early.disconnected")), QStringLiteral("1/1"));
}

void tst_cdpshims::aLookingAgainWhichThrowsLeavesThePingAsItWas(){
    QJSEngine refusing;
    Pinging(&refusing, QStringLiteral(
        "localArea.get = function(){ reads.push('local');"
        "                            throw new Error('Access to storage is not allowed from this context.'); };"));
    refusing.evaluate(QStringLiteral("hearing();"));
    Settle(&refusing);
    refusing.evaluate(QStringLiteral("link.say({ ping: 1 });"));
    Settle(&refusing);
    QCOMPARE(Value(&refusing, QStringLiteral("localReads() + '/' + heard.length + '/' + link.disconnected")),
             QStringLiteral("2/0/0"));

    QJSEngine clockless;
    Pinging(&clockless);
    clockless.evaluate(QStringLiteral("hearing();"));
    Settle(&clockless);
    clockless.evaluate(QStringLiteral("Date.now = function(){ throw new Error('no clock'); };"));
    QCOMPARE(Value(&clockless, QStringLiteral("link.say({ ping: 1 }); 'said'")), QStringLiteral("said"));
    clockless.evaluate(QStringLiteral(
        "var out = 'pending';"
        "chrome.tabs.sendMessage(555, { hello: 1 }).then(function(v){ out = 'resolved ' + JSON.stringify(v); }, function(e){ out = e.message; });"));
    clockless.evaluate(QStringLiteral("link.say({ re: 1, value: 'still here' });"));
    QTRY_COMPARE(Value(&clockless, QStringLiteral("out")), QStringLiteral("resolved \"still here\""));
}

void tst_cdpshims::aLookWhichFailedSaysNothingOfWhatIsThere(){
    QJSEngine worker;
    Pinging(&worker);
    worker.evaluate(QStringLiteral("localArea.data.rules = 'old'; localArea.data.other = 1;"
                                   "localArea.refuses = true; hearing();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + heard.length")), QStringLiteral("1/0"));

    worker.evaluate(QStringLiteral("localArea.refuses = false; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + heard.length")), QStringLiteral("2/0"));

    worker.evaluate(QStringLiteral("clock += 2000; localArea.data['__vanilla_sync__:exclusionRules'] = '[]';"
                                   "link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.join(' | ')")),
             QStringLiteral("sync:{\"exclusionRules\":{\"newValue\":\"[]\"}}"));
}

void tst_cdpshims::aWriteAndAPingLookingAtOnceTellOfEachChangeOnce(){
    QJSEngine inOrder;
    Pinging(&inOrder);
    inOrder.evaluate(QStringLiteral("hearing();"));
    Settle(&inOrder);
    inOrder.evaluate(QStringLiteral("localArea.held = []; chrome.storage.local.set({ ours: 1 });"));
    Settle(&inOrder);
    inOrder.evaluate(QStringLiteral("localArea.data.theirs = 2; link.say({ ping: 1 });"));
    Settle(&inOrder);
    QCOMPARE(Value(&inOrder, QStringLiteral("localArea.held.length + '/' + heard.length")), QStringLiteral("2/0"));
    inOrder.evaluate(QStringLiteral("var writes = localArea.held[0], pings = localArea.held[1];"
                                    "localArea.held = null; writes(); pings();"));
    Settle(&inOrder);
    QCOMPARE(Value(&inOrder, QStringLiteral("heard.join(' | ')")),
             QStringLiteral("local:{\"ours\":{\"newValue\":1}} | local:{\"theirs\":{\"newValue\":2}}"));

    QJSEngine reversed;
    Pinging(&reversed);
    reversed.evaluate(QStringLiteral("hearing();"));
    Settle(&reversed);
    reversed.evaluate(QStringLiteral("localArea.held = []; chrome.storage.local.set({ ours: 1 });"));
    Settle(&reversed);
    reversed.evaluate(QStringLiteral("localArea.data.theirs = 2; link.say({ ping: 1 });"));
    Settle(&reversed);
    reversed.evaluate(QStringLiteral("var writes = localArea.held[0], pings = localArea.held[1];"
                                     "localArea.held = null; pings(); writes();"));
    Settle(&reversed);
    QCOMPARE(Value(&reversed, QStringLiteral("heard.join(' | ')")),
             QStringLiteral("local:{\"ours\":{\"newValue\":1},\"theirs\":{\"newValue\":2}}"));
}

void tst_cdpshims::aListenerWhichWritesFromInsideIsToldOfItsOwnWriteOnce(){
    QJSEngine worker;
    Pinging(&worker);
    worker.evaluate(QStringLiteral(
        "var wrote = 0;"
        "chrome.storage.onChanged.addListener(function(c, a){"
        "  heard.push(a + ':' + JSON.stringify(c));"
        "  if (!wrote && c.theirs) { wrote = 1; chrome.storage.local.set({ ours: 'answered' }); } });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("localArea.data.theirs = 2; link.say({ ping: 1 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("heard.join(' | ')")),
             QStringLiteral("local:{\"theirs\":{\"newValue\":2}} | local:{\"ours\":{\"newValue\":\"answered\"}}"));
    QCOMPARE(Value(&worker, QStringLiteral("localReads() + '/' + wrote")), QStringLiteral("3/1"));
}

void tst_cdpshims::aPageWithTheKeyAsksTheApplicationWithIt(){
    QJSEngine page;
    MakeKeyedPage(&page, Fetching());
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, index: 3, active: true }] });"
        "chrome.tabs.query({ active: true, currentWindow: true })"
        "  .then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":3,\"active\":true}]"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].url + ' ' + fetched[0].options.method + ' ' + ('body' in fetched[0].options)")),
             QStringLiteral("vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key']")), PageKey());
    QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true,\"currentWindow\":true}]}"));
    QVERIFY(!Value(&page, QStringLiteral("fetched[0].url + JSON.stringify(Object.keys(fetched[0].options))")).contains(PageKey()));
}

void tst_cdpshims::aPageWithTheKeyAsksForTheTreeAndOneWithoutAsksNobody(){
    QJSEngine page;
    MakeKeyedPage(&page, Fetching());
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: '7', url: 'https://a.example/', title: 'A', lastVisitTime: 5 }] });"
        "chrome.history.search({ text: '' }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")),
                 QStringLiteral("[{\"id\":\"7\",\"url\":\"https://a.example/\",\"title\":\"A\",\"lastVisitTime\":5}]"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].url + ' ' + fetched[0].options.headers['X-Vanilla-Key']")),
             QStringLiteral("vanilla-extension://host/call ") + PageKey());
    QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"history.search\",\"args\":[{\"text\":\"\"}]}"));

    QJSEngine open;
    MakePage(&open, Fetching());
    open.evaluate(QStringLiteral(
        "var searched = 'pending', tree = 'pending';"
        "chrome.history.search({ text: '' }).then(function(){ searched = 'resolved'; }, function(e){ searched = e.message; });"
        "chrome.bookmarks.getTree().then(function(){ tree = 'resolved'; }, function(e){ tree = e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("searched")), QStringLiteral("chrome.history.search is not available in this browser"));
    QTRY_COMPARE(Value(&open, QStringLiteral("tree")), QStringLiteral("chrome.bookmarks.getTree is not available in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("fetched.length")), QStringLiteral("0"));
}

void tst_cdpshims::anExtensionsPageKeepsItsStandInsToo(){
    QJSEngine page;
    MakeKeyedPage(&page, Fetching() + QStringLiteral("var enginesChrome = chrome;"));
    page.evaluate(QStringLiteral("self.chrome.tabs = { TAB_ID_NONE: -99, MARK: 7, update: function(){} };"));
    QCOMPARE(Value(&page, QStringLiteral("(chrome !== enginesChrome) + '/' + typeof chrome.tabs.query + '/' + String(chrome.tabs.MARK)")),
             QStringLiteral("true/function/undefined"));

    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, index: 3 }] });"
        "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":3}]"));
    QCOMPARE(Value(&page, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key']")), PageKey());

    QCOMPARE(Value(&page, QStringLiteral("String(chrome.extension.inIncognitoContext) + '/' + (chrome.extension === enginesChrome.extension)")),
             QStringLiteral("false/true"));

    QJSEngine open;
    MakePage(&open, Fetching() + QStringLiteral("var enginesChrome = chrome;"));
    open.evaluate(QStringLiteral(
        "self.chrome.tabs = { TAB_ID_NONE: -99, update: function(){} };"
        "var out = 'pending';"
        "chrome.tabs.query({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&open, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
    QCOMPARE(Value(&open, QStringLiteral("(chrome !== enginesChrome) + '/' + fetched.length")), QStringLiteral("true/0"));
}

void tst_cdpshims::aPagesSendMessageToATabIsSentToTheWorker(){
    QJSEngine page;
    MakePage(&page, Fetching());
    QCOMPARE(Value(&page, QStringLiteral("sent.length")), QStringLiteral("0"));

    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "replies.push({ ok: true, value: 'said' });"
        "chrome.tabs.sendMessage(5, { a: 1 }).then(function(v){ out = 'resolved ' + JSON.stringify(v); }, function(e){ out = e.message; });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + sent[0].length")), QStringLiteral("1/1"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(sent[0][0])")),
             QStringLiteral("{\"__vanillaRelay\":1,\"api\":\"tabs.sendMessage\",\"args\":[5,{\"a\":1}]}"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("resolved \"said\""));

    page.evaluate(QStringLiteral(
        "replies.push({ ok: true, value: 1 }, { ok: true, value: 2 }, { ok: true, value: 3 });"
        "chrome.tabs.sendMessage(5, { a: 1 }, { frameId: 0 });"
        "chrome.tabs.sendMessage(5, { a: 1 }, undefined);"
        "chrome.tabs.sendMessage(5, { a: 1 }, null, function(){});"));
    QCOMPARE(Value(&page, QStringLiteral("sent.slice(1).map(function(s){ return JSON.stringify(s[0].args); }).join(' ')")),
             QStringLiteral("[5,{\"a\":1},{\"frameId\":0}] [5,{\"a\":1}] [5,{\"a\":1}]"));
    QCOMPARE(Value(&page, QStringLiteral("fetched.length + '/' + ports.length + '/' + connects.length")), QStringLiteral("0/0/0"));
}

void tst_cdpshims::aPagesSendMessageAnswersTheWayChromeDoes(){
    QJSEngine page;
    MakePage(&page);
    page.evaluate(QStringLiteral(
        "var out = {}, done = 0, tags = ['value', 'nothing', 'refused', 'silent', 'null', 'bare', 'noError', 'notOk', 'rejected', 'threw'];"
        "function ask(tag){ chrome.tabs.sendMessage(5, {})"
        "  .then(function(v){ out[tag] = 'resolved ' + JSON.stringify(v); done++; },"
        "        function(e){ out[tag] = e.message; done++; }); }"
        "replies.push({ ok: true, value: 'a value' }, { ok: true }, { ok: false, error: 'No tab with id: 9.' });"
        "replies.push(function(){ return Promise.resolve(undefined); }, null, 'answer', { ok: false }, { ok: 'yes', value: 1 },"
        "             function(){ return Promise.reject(new Error('The message port closed before a response was received.')); },"
        "             function(){ throw new Error('Extension context invalidated.'); });"
        "tags.forEach(ask);"));
    const QString nobody = QStringLiteral("Could not establish connection. Receiving end does not exist.");
    QTRY_COMPARE(Value(&page, QStringLiteral("done")), QStringLiteral("10"));
    QCOMPARE(Value(&page, QStringLiteral("tags.map(function(t){ return t + '=' + out[t]; }).join('\\n')")),
             QStringLiteral("value=resolved \"a value\"\nnothing=resolved undefined\nrefused=No tab with id: 9."
                            "\nsilent=%1\nnull=%1\nbare=%1\nnoError=%1\nnotOk=%1\nrejected=%1\nthrew=%1").arg(nobody));

    page.evaluate(QStringLiteral(
        "var called = [], before = sent.length;"
        "replies.push({ ok: true, value: 'by callback' }, { ok: false, error: 'No tab with id: 9.' });"
        "chrome.tabs.sendMessage(5, {}, function(v){ called.push('got ' + v + ' ' + (chrome.runtime.lastError ? 'ERROR' : 'fine')); });"
        "chrome.tabs.sendMessage(5, {}, { frameId: 0 }, function(v){"
        "  called.push('none ' + v + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR')); });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.slice(before).map(function(s){ return JSON.stringify(s[0].args); }).join(' ')")),
             QStringLiteral("[5,{}] [5,{},{\"frameId\":0}]"));
    QTRY_COMPARE(Value(&page, QStringLiteral("called.length + '/' + timers.some(function(t){ return t.ms === 0; })")), QStringLiteral("1/true"));
    page.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("called.join(' | ')")),
                 QStringLiteral("got by callback fine | none undefined No tab with id: 9."));
    QCOMPARE(Value(&page, QStringLiteral("String(chrome.runtime.lastError)")), QStringLiteral("undefined"));
}

void tst_cdpshims::aPagesSendMessageOfTheWrongCountIsNotSentAtAll(){
    QJSEngine page;
    MakePage(&page);
    page.evaluate(QStringLiteral(
        "var out = {}, done = 0, called = [];"
        "function ask(tag, args){ chrome.tabs.sendMessage.apply(null, args)"
        "  .then(function(v){ out[tag] = 'resolved ' + JSON.stringify(v); done++; },"
        "        function(e){ out[tag] = e.message; done++; }); }"
        "ask('four', [5, {}, {}, 'extra']);"
        "ask('five', [5, {}, {}, 'extra', 'more']);"
        "ask('one', [5]);"
        "ask('none', []);"));
    const QString wrong = QStringLiteral("chrome.tabs.sendMessage takes a tabId, a message, and at most one object of options");
    QTRY_COMPARE(Value(&page, QStringLiteral("done")), QStringLiteral("4"));
    QCOMPARE(Value(&page, QStringLiteral("['four', 'five', 'one', 'none'].map(function(t){ return t + '=' + out[t]; }).join('\\n')")),
             QStringLiteral("four=%1\nfive=%1\none=%1\nnone=%1").arg(wrong));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + replies.length")), QStringLiteral("0/0"));

    page.evaluate(QStringLiteral(
        "chrome.tabs.sendMessage(5, {}, {}, 'extra', function(v){"
        "  called.push(String(v) + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR')); });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + timers.some(function(t){ return t.ms === 0; })")), QStringLiteral("0/true"));
    page.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&page, QStringLiteral("called.join()")), QStringLiteral("undefined ") + wrong);
    QCOMPARE(Value(&page, QStringLiteral("String(chrome.runtime.lastError)")), QStringLiteral("undefined"));

    page.evaluate(QStringLiteral(
        "replies.push({ ok: true, value: 1 }, { ok: true, value: 2 });"
        "chrome.tabs.sendMessage(5, { a: 1 }, { frameId: 0 });"
        "chrome.tabs.sendMessage(5, { a: 1 }, undefined, function(){});"));
    QCOMPARE(Value(&page, QStringLiteral("sent.map(function(s){ return JSON.stringify(s[0].args); }).join(' ')")),
             QStringLiteral("[5,{\"a\":1},{\"frameId\":0}] [5,{\"a\":1}]"));
}

void tst_cdpshims::theWorkerSendsForAPageOfItsOwn(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } }, { ok: true, value: { id: 5, index: 0 } });"
        "var got = [], answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(JSON.stringify(m)); return false; });"
        "var a = linking('d1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var b = linking('d2000000000000000000000000000000', 7, 'https://a.example/frame', 200);"
        "function pageSender(){ return { id: chrome.runtime.id,"
        "                                url: 'chrome-extension://' + chrome.runtime.id + '/pages/action.html',"
        "                                origin: 'chrome-extension://' + chrome.runtime.id }; }"
        "function fromPage(m, sender){ return listeners[0](m, sender || pageSender(),"
        "                                                  function(v){ answered.push(JSON.stringify(v)); }); }"
        "function relay(args){ return { __vanillaRelay: 1, api: 'tabs.sendMessage', args: args }; }"));
    Settle(&worker);

    QCOMPARE(Value(&worker, QStringLiteral("String(fromPage(relay([5, { q: 1 }])))")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length + '/' + answered.length")), QStringLiteral("0/0"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([a.last(), b.last()])")),
             QStringLiteral("[{\"id\":1,\"message\":{\"q\":1}},{\"id\":1,\"message\":{\"q\":1}}]"));
    worker.evaluate(QStringLiteral("a.say({ re: 1, value: 'from the content script' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")),
                 QStringLiteral("{\"ok\":true,\"value\":\"from the content script\"}"));

    worker.evaluate(QStringLiteral("answered = []; fromPage(relay([5, { q: 2 }, { frameId: 7 }]));"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([a.last(), b.last()])")),
             QStringLiteral("[{\"id\":1,\"message\":{\"q\":1}},{\"id\":2,\"message\":{\"q\":2}}]"));
    worker.evaluate(QStringLiteral("b.say({ re: 2 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("{\"ok\":true}"));

    worker.evaluate(QStringLiteral("answered = []; fromPage(relay([4242, {}]));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")),
                 QStringLiteral("{\"ok\":false,\"error\":\"Could not establish connection. Receiving end does not exist.\"}"));
    worker.evaluate(QStringLiteral("answered = []; fromPage(relay([5, {}, { documentId: 'abc' }]));"));
    QTRY_VERIFY(Value(&worker, QStringLiteral("answered.join()")).contains(QStringLiteral("documentId")));
    QCOMPARE(Value(&worker, QStringLiteral("got.length + '/' + cried.length")), QStringLiteral("0/0"));
}

void tst_cdpshims::onlyWhatTheEngineNamedThisExtensionsOwnIsRelayed(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var got = [], answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(JSON.stringify(m) + ' tab=' + (s.tab ? s.tab.id : 'none')); return false; });"
        "var a = linking('e1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var mine = 'chrome-extension://' + chrome.runtime.id;"
        "function sender(origin){ var s = { id: chrome.runtime.id, url: mine + '/pages/action.html' };"
        "                         if (origin !== undefined) s.origin = origin; return s; }"
        "function relayed(m, s){ return String(listeners[0](m, s, function(v){ answered.push(JSON.stringify(v)); })); }"
        "var call = { __vanillaRelay: 1, api: 'tabs.sendMessage', args: [5, { q: 1 }] };"));
    Settle(&worker);
    const int posted = Value(&worker, QStringLiteral("a.posted.length")).toInt();

    worker.evaluate(QStringLiteral("var out = [];"
                                   "out.push(relayed(call, sender('http://a.example')));"
                                   "out.push(relayed(call, sender('https://a.example')));"
                                   "out.push(relayed(call, sender(undefined)));"
                                   "out.push(relayed(call, sender('null')));"
                                   "out.push(relayed(call, sender(mine + '/')));"
                                   "out.push(relayed(call, sender('chrome-extension://ponmlkjihgfedcbaponmlkjihgfedcba')));"
                                   "out.push(relayed(call, { id: 'ponmlkjihgfedcbaponmlkjihgfedcba', url: mine + '/x', origin: mine }));"
                                   "out.push(relayed({ __vanillaRelay: 1, __vanillaEnvelope: 1, api: 'tabs.sendMessage', args: [5, { q: 1 }] }, sender(mine)));"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("8"));
    QCOMPARE(Value(&worker, QStringLiteral("got.filter(function(g){ return g.indexOf('__vanillaRelay') >= 0 && g.indexOf('tab=none') >= 0; }).length")),
             QStringLiteral("8"));
    QCOMPARE(Value(&worker, QStringLiteral("a.posted.length")).toInt(), posted);
    QCOMPARE(Value(&worker, QStringLiteral("answered.length")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("out.join()")), QStringLiteral("false,false,false,false,false,false,false,false"));

    worker.evaluate(QStringLiteral(
        "got = [];"
        "relayed({ __vanillaEnvelope: 1, __vanillaRelay: 1, api: 'tabs.sendMessage', args: [5, { q: 1 }],"
        "          from: { nonce: 'e1000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/top' },"
        "          message: { inner: 1 } }, sender('http://a.example'));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.join()")), QStringLiteral("{\"inner\":1} tab=5"));
    QCOMPARE(Value(&worker, QStringLiteral("a.posted.length")).toInt(), posted);
    QCOMPARE(Value(&worker, QStringLiteral("answered.length")), QStringLiteral("0"));
}

void tst_cdpshims::whatIsNoCallOfOursIsNotRelayedAndIsSaidSo(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var got = [], answered = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ got.push(JSON.stringify(m)); return false; });"
        "var a = linking('f1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "function fromPage(m){ return String(listeners[0](m, { id: chrome.runtime.id, url: 'x',"
        "                                                      origin: 'chrome-extension://' + chrome.runtime.id },"
        "                                                 function(v){ answered.push(JSON.stringify(v)); })); }"
        "function bad(api, args){ return fromPage({ __vanillaRelay: 1, api: api, args: args }); }"));
    Settle(&worker);
    const int posted = Value(&worker, QStringLiteral("a.posted.length")).toInt();

    worker.evaluate(QStringLiteral(
        "var out = [];"
        "out.push(bad('tabs.remove', [5, {}]));"
        "out.push(bad('tabs.sendMessage', undefined));"
        "out.push(bad('tabs.sendMessage', { 0: 5, 1: {}, length: 2 }));"
        "out.push(bad('tabs.sendMessage', [5]));"
        "out.push(bad('tabs.sendMessage', [5, {}, { frameId: 0 }, 1, 2]));"
        "out.push(bad('tabs.sendMessage', ['5', {}]));"
        "out.push(bad('tabs.sendMessage', [0, {}]));"
        "out.push(bad('tabs.sendMessage', [1.5, {}]));"
        "out.push(bad('tabs.sendMessage', [5, {}, 7]));"
        "out.push(bad('tabs.sendMessage', [5, {}, null]));"));
    QCOMPARE(Value(&worker, QStringLiteral("out.join()")), QStringLiteral("true,true,true,true,true,true,true,true,true,true"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.length")), QStringLiteral("10"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.filter(function(v){ return v === JSON.stringify("
                                           "{ ok: false, error: 'chrome.tabs.sendMessage: the compatibility layer could not relay this call' }); }).length")),
             QStringLiteral("10"));
    QCOMPARE(Value(&worker, QStringLiteral("a.posted.length")).toInt(), posted);
    QCOMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("0"));
}

void tst_cdpshims::aPageHasItsScriptingRunByTheWorker(){
    QJSEngine page;
    MakeKeyedPage(&page, Fetching() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'] }; };"));
    page.evaluate(QStringLiteral(
        "var out = 'pending';"
        "function outcome(p){ out = 'pending'; p.then(function(r){ out = 'ran ' + JSON.stringify(r); }, function(e){ out = 'failed ' + e.message; }); }"
        "replies.push({ ok: true, value: [{ frameId: 0 }] });"
        "outcome(chrome.scripting.executeScript({ files: ['/js/scripting/tool-overlay.js', '/js/scripting/zapper.js'], target: { tabId: 5 } }));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("ran [{\"frameId\":0}]"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(sent[0][0])")),
             QStringLiteral("{\"__vanillaRelay\":1,\"api\":\"scripting.executeScript\","
                            "\"args\":[{\"files\":[\"/js/scripting/tool-overlay.js\",\"/js/scripting/zapper.js\"],\"target\":{\"tabId\":5}}]}"));
    page.evaluate(QStringLiteral("replies.push({ ok: false, error: 'No tab with id: 9.' }); outcome(chrome.scripting.executeScript({ files: ['a.js'], target: { tabId: 9 } }));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("failed No tab with id: 9."));
    page.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ func: function(){ return 1; }, target: { tabId: 5 } }));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")),
                 QStringLiteral("failed chrome.scripting.executeScript: 'func' is not available from an extension's page in this browser"));
    page.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS([{ css: 'a{}' }]));"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("failed Invalid value for argument 1. Expected an object."));
    QCOMPARE(Value(&page, QStringLiteral("sent.length")), QStringLiteral("2"));
    page.evaluate(QStringLiteral("replies.push({ ok: true }); chrome.scripting.removeCSS({ css: 'a{}', target: { tabId: 5 } }, function(r){ out = 'called ' + r; });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("called undefined"));
    QCOMPARE(Value(&page, QStringLiteral("sent[2][0].api")), QStringLiteral("scripting.removeCSS"));

    QJSEngine bare;
    MakeKeyedPage(&bare, Fetching() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: [] }; };"));
    bare.evaluate(QStringLiteral(
        "var out = 'pending'; chrome.scripting.executeScript({ files: ['a.js'], target: { tabId: 5 } }).then(function(){ out = 'ran'; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&bare, QStringLiteral("out")), QStringLiteral("chrome.scripting.executeScript is not available in this browser"));
    QCOMPARE(Value(&bare, QStringLiteral("sent.length")), QStringLiteral("0"));

    QJSEngine edge;
    MakeKeyedPage(&edge, Fetching() + QStringLiteral(
        "chrome.runtime.getManifest = function(){ return { permissions: ['scripting'] }; };"
        "chrome.scripting = { executeScript: function(){ return Promise.resolve('the engine'); } };"));
    edge.evaluate(QStringLiteral(
        "var out = 'pending'; chrome.scripting.executeScript({ files: ['a.js'], target: { tabId: 5 } }).then(function(r){ out = r; }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&edge, QStringLiteral("out")), QStringLiteral("the engine"));
    QCOMPARE(Value(&edge, QStringLiteral("sent.length")), QStringLiteral("0"));
}

void tst_cdpshims::theWorkerRunsAPagesScriptingForIt(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting', 'tabs'], host_permissions: ['<all_urls>'] }; };"));
    worker.evaluate(Linking() + Serving());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } });"
        "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "var answered = [];"
        "function fromPage(api, args){ return String(listeners[0]({ __vanillaRelay: 1, api: api, args: args },"
        "  { id: chrome.runtime.id, url: 'x', origin: 'chrome-extension://' + chrome.runtime.id },"
        "  function(v){ answered.push(JSON.stringify(v)); })); }"
        "var out = [];"
        "out.push(fromPage('scripting.insertCSS', [{ css: 'a{}' }]));"
        "out.push(fromPage('scripting.executeScript', [{ files: ['a.js'], target: { tabId: 'x' } }]));"
        "out.push(fromPage('scripting.executeScript', [{ func: 'f', target: { tabId: 5 } }]));"
        "out.push(fromPage('scripting.executeScript', [[{ files: ['a.js'] }]]));"
        "out.push(fromPage('scripting.executeScript', [{ files: ['a.js'] }, 2]));"
        "out.push(fromPage('scripting.registerContentScripts', [[]]));"));
    QCOMPARE(Value(&worker, QStringLiteral("out.join()")), QStringLiteral("true,true,true,true,true,true"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.length")), QStringLiteral("6"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.filter(function(v){ return v.indexOf('could not relay') < 0; }).sort().join('|')")),
             QStringLiteral("{\"ok\":false,\"error\":\"Invalid value for argument 1. Property 'target': expected an object.\"}|"
                            "{\"ok\":false,\"error\":\"No tab with id: x.\"}"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.filter(function(v){ return v === JSON.stringify("
                                           "{ ok: false, error: 'chrome.tabs.sendMessage: the compatibility layer could not relay this call' }); }).length")),
             QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("answered.filter(function(v){ return v === JSON.stringify("
                                           "{ ok: false, error: 'chrome.scripting.executeScript: the compatibility layer could not relay this call' }); }).length")),
             QStringLiteral("3"));

    const int before = Value(&worker, QStringLiteral("top.posted.length")).toInt();
    worker.evaluate(QStringLiteral("fromPage('scripting.executeScript', [{ files: ['cs/a.js'], target: { tabId: 555 } }]);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length")).toInt(), before + 1);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(top.last().codes)")), QStringLiteral("[\"A\"]"));

    worker.evaluate(QStringLiteral(
        "var heard = answered.length;"
        "String(listeners[0]({ __vanillaRelay: 1, api: 'scripting.executeScript', args: [{ files: ['cs/a.js'], target: { tabId: 555 } }] },"
        "  { id: chrome.runtime.id, url: 'https://a.example/top', origin: 'https://a.example' },"
        "  function(v){ answered.push(JSON.stringify(v)); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length")).toInt(), before + 1);
    QCOMPARE(Value(&worker, QStringLiteral("answered.filter(function(v){ return v.indexOf('\"ok\":true') >= 0; }).length")), QStringLiteral("0"));
}

void tst_cdpshims::aRelayToAMadeUpNumberReachesNobody(){
    const QString asking = QStringLiteral(
        "var answered = [];"
        "function fromPage(args){ return listeners[0]({ __vanillaRelay: 1, api: 'tabs.sendMessage', args: args },"
        "                                             { id: chrome.runtime.id, url: 'x', origin: 'chrome-extension://' + chrome.runtime.id },"
        "                                             function(v){ answered.push(JSON.stringify(v)); }); }");
    const QString nobody = QStringLiteral("{\"ok\":false,\"error\":\"Could not establish connection. Receiving end does not exist.\"}");

    QJSEngine unasked;
    Make(&unasked, false, Fetching() + Saying());
    unasked.evaluate(Linking() + asking);
    unasked.evaluate(QStringLiteral("var a = linking('a9000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&unasked);
    QCOMPARE(Value(&unasked, QStringLiteral("JSON.stringify(a.last())")),
             QStringLiteral("{\"linked\":1,\"url\":\"https://a.example/top\"}"));
    unasked.evaluate(QStringLiteral("fromPage([5, { q: 1 }]);"));
    QTRY_COMPARE(Value(&unasked, QStringLiteral("answered.join()")), nobody);
    QCOMPARE(Value(&unasked, QStringLiteral("a.posted.length")), QStringLiteral("1"));

    QJSEngine unanswered;
    MakeAsking(&unanswered, Fetching(), Saying());
    unanswered.evaluate(Linking() + asking);
    unanswered.evaluate(QStringLiteral(
        "answers.push('DOWN', 'DOWN', 'DOWN', 'DOWN', 'DOWN', 'DOWN');"
        "var a = linking('a8000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Drain(&unanswered);
    QVERIFY(Value(&unanswered, QStringLiteral("fetched.length")).toInt() > 0);
    unanswered.evaluate(QStringLiteral("answered = []; fromPage([5, { q: 1 }]);"));
    QTRY_COMPARE(Value(&unanswered, QStringLiteral("answered.join()")), nobody);
    QCOMPARE(Value(&unanswered, QStringLiteral("a.posted.length")), QStringLiteral("1"));
}

void tst_cdpshims::anAnswerNobodyIsLeftToHearIsDropped(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying());
    worker.evaluate(Linking());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 5, index: 0 } });"
        "var answered = [];"
        "var a = linking('c9000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "function fromPage(args, respond){ return String(listeners[0]({ __vanillaRelay: 1, api: 'tabs.sendMessage', args: args },"
        "                                                            { id: chrome.runtime.id, url: 'x', origin: 'chrome-extension://' + chrome.runtime.id },"
        "                                                            respond)); }"
        "function closed(){ throw new Error('Attempting to use a disconnected port object'); }"
        "function hears(v){ answered.push(JSON.stringify(v)); }"));
    Settle(&worker);

    QCOMPARE(Value(&worker, QStringLiteral("fromPage([5, { q: 1 }], closed)")), QStringLiteral("true"));
    worker.evaluate(QStringLiteral("a.say({ re: 1, value: 'too late' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("cried.length")), QStringLiteral("1"));
    QVERIFY2(Value(&worker, QStringLiteral("cried.join()")).contains(QStringLiteral("disconnected port")),
             qPrintable(Value(&worker, QStringLiteral("cried.join()"))));

    worker.evaluate(QStringLiteral("cried = [];"));
    QCOMPARE(Value(&worker, QStringLiteral("fromPage(['5', { q: 1 }], closed)")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("cried.length")), QStringLiteral("1"));

    worker.evaluate(QStringLiteral("fromPage([5, { q: 2 }], hears); a.say({ re: 2, value: 'heard' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("answered.join()")), QStringLiteral("{\"ok\":true,\"value\":\"heard\"}"));
}

static const char *A_WORKER_WHICH_WAITS =
    "var events = {}, registered = 0, skipped = 0, claimed = 0;"
    "self.addEventListener = function(name, f){ events[name] = f; registered++; };"
    "self.skipWaiting = function(){ skipped++; return Promise.resolve(); };"
    "self.clients = { claim: function(){ claimed++; return Promise.resolve(); } };";

void tst_cdpshims::aWorkerTakesOverAsSoonAsItIsInstalled(){
    QJSEngine engine;
    Make(&engine, false, QString::fromLatin1(A_WORKER_WHICH_WAITS));
    QCOMPARE(Value(&engine, QStringLiteral("Object.keys(events).sort().join()")), QStringLiteral("activate,install"));
    engine.evaluate(QStringLiteral("events.install(); var waited = null; events.activate({ waitUntil: function(p){ waited = p; } });"));
    QCOMPARE(Value(&engine, QStringLiteral("String(skipped) + ',' + String(claimed) + ',' + typeof (waited && waited.then)")),
             QStringLiteral("1,1,function"));
}

void tst_cdpshims::aWorkerAlreadyActiveIsNotAskedTwiceAndAWaitWhichFailsIsSwallowed(){
    QJSEngine engine;
    Make(&engine, false, QString::fromLatin1(A_WORKER_WHICH_WAITS));
    QCOMPARE(engine.evaluate(Cdp::WorkerShim()).toString(), QStringLiteral("already"));
    QCOMPARE(Value(&engine, QStringLiteral("String(registered)")), QStringLiteral("2"));
    engine.evaluate(QStringLiteral(
        "self.skipWaiting = function(){ throw new Error('InvalidStateError'); };"
        "self.clients.claim = function(){ throw new Error('InvalidStateError'); };"
        "var outcome = (function(){ try { events.install(); events.activate({ waitUntil: function(){} }); return 'ok'; } catch (e) { return 'threw ' + e; } })();"));
    QCOMPARE(Value(&engine, QStringLiteral("outcome")), QStringLiteral("ok"));
}

void tst_cdpshims::neitherAContentScriptNorAPageSkipsWaiting(){
    QJSEngine content;
    Make(&content, true, QString::fromLatin1(A_WORKER_WHICH_WAITS));
    QCOMPARE(Value(&content, QStringLiteral("String('install' in events) + ',' + String('activate' in events)")), QStringLiteral("false,false"));
    QJSEngine page;
    MakePage(&page, QString::fromLatin1(A_WORKER_WHICH_WAITS));
    QCOMPARE(Value(&page, QStringLiteral("String('install' in events) + ',' + String('activate' in events)")), QStringLiteral("false,false"));
}

void tst_cdpshims::whichEngineThisIsComesFromOneBrandAndNothingElse(){
    struct Case { QString world; QString word; QString edge; };
    const QList<Case> cases = QList<Case>()
        << Case{ EdgeBrands(), QStringLiteral("engine:edge"), QStringLiteral("true") }
        << Case{ SpoofedBrands(), QStringLiteral("engine:qt"), QStringLiteral("false") }
        << Case{ QStringLiteral("var navigator = { userAgentData: { brands: [{ brand: 'Microsoft Edge WebView2 Runtime' },"
                                "                                            { brand: 'microsoft edge webview2' }] } };"),
                 QStringLiteral("engine:qt"), QStringLiteral("false") }
        << Case{ QStringLiteral("var navigator = { userAgent: 'Mozilla/5.0 Edg/153.0.0.0' };"),
                 QStringLiteral("engine:qt"), QStringLiteral("false") }
        << Case{ QString(), QStringLiteral("engine:qt"), QStringLiteral("false") }
        << Case{ QStringLiteral("var navigator = { userAgentData: { get brands(){ throw new Error('no'); } } };"),
                 QStringLiteral("engine:qt"), QStringLiteral("false") }
        << Case{ QStringLiteral("var navigator = { userAgentData: { brands: 'Microsoft Edge WebView2' } };"),
                 QStringLiteral("engine:qt"), QStringLiteral("false") };
    foreach(const Case one, cases){
        QJSEngine worker;
        worker.evaluate(World());
        worker.evaluate(QStringLiteral("document = undefined;") + Fetching() + HostFetches() + one.world + EdgesOwn());
        const QJSValue made = worker.evaluate(Cdp::WorkerShim().replace(QLatin1String(ExtensionHostWire::KEY_PLACE), QString(64, QLatin1Char('5'))));
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        QCOMPARE(made.toString().split(QLatin1Char(',')).value(0), one.word);
        QCOMPARE(QString::number(made.toString().split(QLatin1Char(',')).count(one.word)), QStringLiteral("1"));
        QCOMPARE(Value(&worker, QStringLiteral("(chrome.downloads === enginesDownloads) + '/' + (chrome.storage.sync === enginesSync)")),
                 one.edge + QLatin1Char('/') + one.edge);
    }
}

void tst_cdpshims::whatWebView2AnswersForItselfIsLeftToTheEngine(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Messages());
    QCOMPARE(Value(&worker, QStringLiteral("(chrome.contextMenus === enginesMenus) + '/' + (chrome.contextMenus.onClicked === enginesMenus.onClicked)")),
             QStringLiteral("false/false"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.contextMenus.create({ id: 'm', title: 'M' })")), QStringLiteral("m"));
    QCOMPARE(Value(&worker, QStringLiteral("(chrome.downloads === enginesDownloads) + '/' + (chrome.offscreen === enginesOffscreen)")),
             QStringLiteral("true/true"));
    worker.evaluate(QStringLiteral("chrome.downloads.download({ url: 'blob:https://a.example/x' });"
                                   "chrome.offscreen.createDocument({ url: 'off.html' });"));
    QCOMPARE(Value(&worker, QStringLiteral("(chrome.storage.sync === enginesSync) + '/' + (chrome.storage.onChanged === enginesOnChanged)"
                                           " + '/' + (chrome.i18n === enginesI18n) + '/' + (chrome.runtime.getContexts === enginesGetContexts)")),
             QStringLiteral("true/true/true/true"));
    QCOMPARE(Value(&worker, QStringLiteral("typeof self.__vanillaMessages.messages.greeting.message + ' ' + chrome.i18n.getMessage('greeting')")),
             QStringLiteral("string the engine's own"));
    QCOMPARE(Value(&worker, QStringLiteral("edgeCalls.slice().sort().join()")),
             QStringLiteral("contextMenus.create,contextMenus.onClicked.addListener,downloads.download,i18n.getMessage,offscreen.createDocument"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("hostFetches() + '/' + waitingHere()")), QStringLiteral("0/0"));
    QCOMPARE(Value(&worker, QStringLiteral("report.split(',')[0]")), QStringLiteral("engine:edge"));
    QCOMPARE(Value(&worker, QStringLiteral("report.split(',').filter(function(w){"
                                           "  return ['i18n', 'storage.sync', 'storage.onChanged', 'contextMenus.create',"
                                           "          'downloads.download', 'runtime.getContexts'].includes(w); }).join()")),
             QString());

    QJSEngine qt;
    MakeAsking(&qt, Fetching() + HostFetches() + EdgesOwn(), Messages());
    QCOMPARE(Value(&qt, QStringLiteral("(chrome.contextMenus === enginesMenus) + '/' + (chrome.storage.sync === enginesSync)"
                                       " + '/' + (chrome.storage.onChanged === enginesOnChanged) + '/' + (chrome.i18n === enginesI18n)")),
             QStringLiteral("false/false/false/false"));
    QCOMPARE(Value(&qt, QStringLiteral("chrome.i18n.getMessage('greeting')")), QStringLiteral("ours"));
    qt.evaluate(QStringLiteral("var out = 'pending';"
                               "answers.push({ ok: true, value: 1 });"
                               "chrome.contextMenus.create({ id: 'm', title: 'M' });"));
    Settle(&qt);
    QCOMPARE(Value(&qt, QStringLiteral("edgeCalls.join() + '/' + hostFetches()")), QStringLiteral("/1"));
}

void tst_cdpshims::whatIsStillTheApplicationsToAnswerOnWebView2(){
    QJSEngine worker;
    MakeEdgeWorker(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("chrome.tabs === enginesTabs")), QStringLiteral("false"));
    worker.evaluate(QStringLiteral(
        "var relayed = [];"
        "self.__vanillaRelay.attach(function(ticket, call){ relayed.push([ticket, call.api]); });"
        "var badge = 'pending', tabs = 'pending';"
        "chrome.action.setBadgeText({ text: '1' }).then(function(){ badge = 'done'; }, function(e){ badge = e.message; });"
        "chrome.tabs.query({ active: true }).then(function(v){ tabs = JSON.stringify(v); }, function(e){ tabs = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayed.map(function(r){ return r[1]; }).join()")),
             QStringLiteral("action.setBadgeText,tabs.query"));
    QCOMPARE(Value(&worker, QStringLiteral("edgeCalls.join() + '/' + hostFetches()")), QStringLiteral("contextMenus.onClicked.addListener/0"));
    QCOMPARE(Value(&worker, QStringLiteral("report.split(',').filter(function(w){"
                                           "  return ['action.setIcon', 'tabs', 'windows', 'action'].includes(w); }).sort().join()")),
             QStringLiteral("action,action.setIcon,tabs,windows"));
}

void tst_cdpshims::anEdgePageAndContentScriptKeepTheEnginesMessagesAndStorage(){
    {
        QJSEngine content;
        MakeEdgeContent(&content, Messages());
        QCOMPARE(Value(&content, QStringLiteral("(chrome.storage.sync === enginesSync) + '/' + (chrome.i18n === enginesI18n)"
                                                " + '/' + chrome.i18n.getMessage('greeting')")),
                 QStringLiteral("true/true/the engine's own"));
        QCOMPARE(Value(&content, QStringLiteral("report.indexOf('engine:') + '/' + report.split(',').filter(function(w){"
                                                "  return ['i18n'].includes(w); }).join()")),
                 QStringLiteral("-1/"));
        QCOMPARE(Value(&content, QStringLiteral("reads.length + ' reads, ' + timers.length + ' timers'")), QStringLiteral("0 reads, 0 timers"));
    }
    {
        QJSEngine page;
        MakeEdgePage(&page, Messages());
        QCOMPARE(Value(&page, QStringLiteral("(chrome.storage.sync === enginesSync) + '/' + (chrome.storage.onChanged === enginesOnChanged)"
                                             " + '/' + (chrome.i18n === enginesI18n) + '/' + chrome.i18n.getMessage('greeting')")),
                 QStringLiteral("true/true/true/the engine's own"));
        QCOMPARE(Value(&page, QStringLiteral("(chrome.contextMenus === enginesMenus) + '/' + (chrome.downloads === enginesDownloads)"
                                             " + '/' + (chrome.offscreen === enginesOffscreen) + '/' + (chrome.tabs === enginesTabs)")),
                 QStringLiteral("true/true/true/false"));
        QCOMPARE(Value(&page, QStringLiteral("chrome.contextMenus.create({ id: 'p' }) + '/' + edgeCalls.filter(function(c){"
                                             "  return c.indexOf('contextMenus') === 0; }).join()")),
                 QStringLiteral("p/contextMenus.create"));
        page.evaluate(QStringLiteral(
            "var out = 'pending';"
            "answers.push({ ok: true, value: [{ id: 12, index: 3 }] });"
            "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"));
        QTRY_COMPARE(Value(&page, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":3}]"));
        QCOMPARE(Value(&page, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
                 QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true}]}"));
        QCOMPARE(Value(&page, QStringLiteral("report.split(',')[0] + '/' + report.split(',').filter(function(w){"
                                             "  return ['i18n', 'storage.sync', 'storage.onChanged', 'contextMenus.create'].includes(w); }).join()")),
                 QStringLiteral("engine:edge/"));
        QCOMPARE(Value(&page, QStringLiteral("typeof self.__vanillaRelay")), QStringLiteral("undefined"));
    }
}

void tst_cdpshims::aWorkerOnWebView2NeverFetchesTheHost(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Saying());
    worker.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, index: 0 }] });"
        "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"
        "chrome.tabs.onCreated.addListener(function(){ heard.push('created'); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("hostFetches() + '/' + fetched.length + '/' + answers.length")), QStringLiteral("0/0/1"));
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("pending"));
    QCOMPARE(Value(&worker, QStringLiteral("waitingHere() + '/' + timers.map(function(t){ return t.ms; })"
                                           "  .sort(function(a, b){ return a - b; }).join()")),
             QStringLiteral("1/10000,120000"));

    QJSEngine qt;
    MakeAsking(&qt, Fetching() + HostFetches());
    qt.evaluate(QStringLiteral(
        "var out = 'pending';"
        "answers.push({ ok: true, value: [{ id: 12, index: 0 }] });"
        "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&qt, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":0}]"));
    QCOMPARE(Value(&qt, QStringLiteral("hostFetches()")), QStringLiteral("1"));
    QCOMPARE(Value(&qt, QStringLiteral("typeof self.__vanillaRelay")), QStringLiteral("undefined"));
}

void tst_cdpshims::whatWaitsForTheRelayIsBoundedAndEndsOnItsDeadline(){
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker);
        worker.evaluate(QStringLiteral(
            "var out = 'pending';"
            "chrome.tabs.query({ active: true }).then(function(v){ out = 'resolved'; }, function(e){ out = e.message; });"));
        Settle(&worker);
        QCOMPARE(Value(&worker, QStringLiteral("timers.map(function(t){ return t.ms; }).join()")), QStringLiteral("10000"));
        worker.evaluate(QStringLiteral("runTimers();"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
        QCOMPARE(Value(&worker, QStringLiteral("waitingHere() + '/' + hostFetches()")), QStringLiteral("0/0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker);
        worker.evaluate(QStringLiteral(
            "var said = [];"
            "for (var i = 1; i <= 65; i++) (function(n){"
            "  chrome.tabs.get(n).then(function(){ said.push('resolved ' + n); }, function(e){ said.push(n + ': ' + e.message); });"
            "})(i);"));
        Settle(&worker);
        QCOMPARE(Value(&worker, QStringLiteral("said.join(' | ')")),
                 QStringLiteral("65: chrome.tabs.get is not available in this browser"));
        QCOMPARE(Value(&worker, QStringLiteral("waitingHere()")), QStringLiteral("64"));
    }
}

void tst_cdpshims::theRelayHookIsWhatAnswersAWaitingCall(){
    QJSEngine worker;
    MakeEdgeWorker(&worker);
    worker.evaluate(QStringLiteral(
        "var out = 'pending', badge = 'pending', relayed = [];"
        "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"
        "chrome.action.setBadgeText({ text: '1' }).then(function(){ badge = 'done'; }, function(e){ badge = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayed.length + '/' + waitingHere()")), QStringLiteral("0/2"));

    worker.evaluate(QStringLiteral("self.__vanillaRelay.attach(function(ticket, call){ relayed.push([ticket, JSON.stringify(call)]); });"));
    QCOMPARE(Value(&worker, QStringLiteral("relayed.map(function(r){ return r[1]; }).join('\\n')")),
             QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true}]}\n"
                            "{\"api\":\"action.setBadgeText\",\"args\":[{\"text\":\"1\"}]}"));
    worker.evaluate(QStringLiteral("var got = 'pending'; chrome.tabs.get(12).then(function(v){ got = JSON.stringify(v); }, function(e){ got = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayed.length")), QStringLiteral("3"));

    worker.evaluate(QStringLiteral(
        "self.__vanillaRelay.answer(relayed[0][0], { ok: true, value: [{ id: 12, index: 3 }] });"
        "self.__vanillaRelay.answer(relayed[1][0], { ok: false, error: 'this extension has no button' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":3}]"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("badge")), QStringLiteral("this extension has no button"));
    QCOMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("pending"));
    worker.evaluate(QStringLiteral("self.__vanillaRelay.answer(relayed[2][0], { ok: true, value: { id: 12, index: 5 } });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("{\"id\":12,\"index\":5}"));
    QCOMPARE(Value(&worker, QStringLiteral("waitingHere() + '/' + timers.length + '/' + hostFetches()")), QStringLiteral("0/0/0"));
    QCOMPARE(Value(&worker, QStringLiteral("(function(){ try { self.__vanillaRelay.answer(relayed[0][0], { ok: true, value: 'again' });"
                                           "                   self.__vanillaRelay.answer(9999, { ok: true, value: 'nobody' });"
                                           "                   return 'no throw'; } catch (e) { return e.message; } })() + '/' + out")),
             QStringLiteral("no throw/[{\"id\":12,\"index\":3}]"));
}

void tst_cdpshims::theRelayPagesPortIsTakenAndNoOthersAre(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying());
    worker.evaluate(QStringLiteral("var seenPorts = []; var watching = function(p){ seenPorts.push(p.name); };"
                                   "chrome.runtime.onConnect.addListener(watching);"
                                   "var relay = relaying({ tab: { id: 945652895 } });"));
    QCOMPARE(Value(&worker, QStringLiteral("seenPorts.join() + '/' + relay.disconnected")), QStringLiteral("/0"));
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay) + '/' + hostFetches()")), QStringLiteral("tabs.query/0"));

    worker.evaluate(QStringLiteral("var elsewhere = relaying({ url: 'chrome-extension://' + chrome.runtime.id + '/popup.html' });"
                                   "var inATab = relaying({ url: 'chrome-extension://' + chrome.runtime.id + '/popup.html', tab: { id: 5 } });"
                                   "var alien = relaying({ id: 'somebodyelsesextension' });"
                                   "var alienInATab = relaying({ id: 'somebodyelsesextension', tab: { id: 6 } });"));
    QCOMPARE(Value(&worker, QStringLiteral("seenPorts.length + '/' + seenPorts.join()")),
             QStringLiteral("4/__vanilla_relay__,__vanilla_relay__,__vanilla_relay__,__vanilla_relay__"));
    QCOMPARE(Value(&worker, QStringLiteral("relay.disconnected + '/' + (elsewhere.posted.length + inATab.posted.length"
                                           "                            + alien.posted.length + alienInATab.posted.length)")),
             QStringLiteral("0/0"));
}

void tst_cdpshims::whatWaitedGoesOverTheRelaysPortInOrder(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying());
    worker.evaluate(QStringLiteral(
        "var out = 'pending', badge = 'pending';"
        "chrome.tabs.query({ active: true }).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"
        "chrome.action.setBadgeText({ text: '1' }).then(function(){ badge = 'done'; }, function(e){ badge = e.message; });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("var relay = relaying();"));
    QCOMPARE(Value(&worker, QStringLiteral("relay.posted.map(function(m){ return m.call.api + ':' + JSON.stringify(m.call.args); }).join(' | ')")),
             QStringLiteral("tabs.query:[{\"active\":true}] | action.setBadgeText:[{\"text\":\"1\"}]"));
    worker.evaluate(QStringLiteral(
        "relay.say({ ticket: relay.posted[0].ticket, answer: { ok: true, value: [{ id: 12, index: 3 }] } });"
        "relay.say({ ticket: relay.posted[1].ticket, answer: { ok: false, error: 'this extension has no button' } });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"id\":12,\"index\":3}]"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("badge")), QStringLiteral("this extension has no button"));
    QCOMPARE(Value(&worker, QStringLiteral("waitingHere() + '/' + timers.length")), QStringLiteral("0/0"));

    worker.evaluate(QStringLiteral("var got = 'pending'; chrome.tabs.get(12).then(function(v){ got = JSON.stringify(v); }, function(e){ got = e.message; });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("relay.say(5); relay.say(null); relay.say({ ticket: relay.posted[2].ticket });"
                                   "relay.say({ ticket: 'x', answer: { ok: true, value: 1 } });"
                                   "relay.say({ ticket: 9999, answer: { ok: true, value: 1 } });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("pending"));
    worker.evaluate(QStringLiteral("relay.say({ ticket: relay.posted[2].ticket, answer: { ok: true, value: { id: 12, index: 5 } } });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got")), QStringLiteral("{\"id\":12,\"index\":5}"));
}

void tst_cdpshims::theHookLetsGoAndASendWhichThrowsFailsThatCallAlone(){
    QJSEngine worker;
    MakeEdgeWorker(&worker);
    worker.evaluate(QStringLiteral(
        "var got = [];"
        "self.__vanillaRelay.attach(function(ticket, call){ got.push(call.api); });"
        "self.__vanillaRelay.attach(null);"
        "var out = 'pending';"
        "chrome.tabs.query({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got.join() + '/' + out + '/' + waitingHere()")), QStringLiteral("/pending/1"));
    worker.evaluate(QStringLiteral(
        "self.__vanillaRelay.attach(function(ticket, call){ if (call.api === 'tabs.query') throw new Error('the port has gone');"
        "                                                   got.push(call.api); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
    worker.evaluate(QStringLiteral("var next = 'pending'; chrome.tabs.get(3).then(function(){ next = 'resolved'; }, function(e){ next = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got.join() + '/' + next")), QStringLiteral("tabs.get/pending"));

    worker.evaluate(QStringLiteral("var later = [];"
                                   "self.__vanillaRelay.attach(function(ticket, call){ later.push(call.api); });"
                                   "var waiting = 'pending';"
                                   "self.__vanillaRelay.attach(null);"
                                   "chrome.tabs.get(4).then(function(){}, function(e){ waiting = e.message; });"
                                   "self.__vanillaRelay.attach(function(ticket, call){ later.push(call.api); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("later.join()")), QStringLiteral("tabs.get"));
    worker.evaluate(QStringLiteral("self.__vanillaRelay.fail();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("next + '/' + waiting")),
                 QStringLiteral("chrome.tabs.get is not available in this browser/chrome.tabs.get is not available in this browser"));
}

void tst_cdpshims::thePortGoingFailsWhatWaitedAndFoldsTheEventsRound(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying() + Saying());
    worker.evaluate(QStringLiteral(
        "var relay = relaying();"
        "chrome.tabs.onCreated.addListener(function(t){ heard.push('created ' + t.id); });"
        "var out = 'pending';"
        "chrome.tabs.query({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay) + '/' + waits()")), QStringLiteral("vanilla.events,tabs.query/10000,120000"));

    worker.evaluate(QStringLiteral("relay.die();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("chrome.tabs.query is not available in this browser"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("waits() + '/' + relay.posted.length")), QStringLiteral("1000/2"));
    worker.evaluate(QStringLiteral("var second = relaying();"));
    QCOMPARE(Value(&worker, QStringLiteral("second.posted.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(second)")), QStringLiteral("vanilla.events"));
    worker.evaluate(QStringLiteral(
        "second.say({ ticket: second.posted[0].ticket,"
        "             answer: { ok: true, value: { order: [7], events: [{ name: 'tabs.onCreated', args: [{ id: 7 }] }] } } });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("heard.join()")), QStringLiteral("created 7"));
}

void tst_cdpshims::nothingIsSentOverThePortWhichHasGone(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying() + Saying());
    worker.evaluate(QStringLiteral(
        "var relay = relaying();"
        "var out = 'pending';"
        "chrome.tabs.query({}).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay)")), QStringLiteral("tabs.query"));
    worker.evaluate(QStringLiteral(
        "var tries = 0, posting = relay.postMessage;"
        "relay.postMessage = function(m){ tries++; return posting(m); };"
        "var asked = 'not yet', clearing = clearTimeout;"
        "clearTimeout = function(id){"
        "  if (asked === 'not yet') { asked = 'pending';"
        "    chrome.tabs.get(9).then(function(){ asked = 'resolved'; }, function(e){ asked = e.message; }); }"
        "  return clearing(id); };"
        "relay.die();"
        "clearTimeout = clearing;"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("tries + '/' + asked + '/' + out")),
             QStringLiteral("0/pending/chrome.tabs.query is not available in this browser"));
    worker.evaluate(QStringLiteral("var second = relaying();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(second)")), QStringLiteral("tabs.get"));
}

void tst_cdpshims::theRoundsOwnDeadlineOverTheRelayIsAnotherRoundAndNoFailure(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying() + Saying());
    worker.evaluate(QStringLiteral("var relay = relaying();"
                                   "chrome.tabs.onCreated.addListener(function(){ heard.push('created'); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay) + '/' + waits()")), QStringLiteral("vanilla.events/120000"));
    for(int round = 0; round < 3; round++){
        worker.evaluate(QStringLiteral("runTimers();"));
        Settle(&worker);
    }
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay) + '/' + waits() + '/' + warned.length")),
             QStringLiteral("vanilla.events,vanilla.events,vanilla.events,vanilla.events/120000/0"));
}

void tst_cdpshims::anAbortedRoundIsARoundWhichBrokeAndNeverAStaleOne(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying() + Saying());
    worker.evaluate(QStringLiteral("var relay = relaying();"
                                   "chrome.tabs.onCreated.addListener(function(){ heard.push('created'); });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("relay.say({ ticket: relay.posted[0].ticket, answer: { ok: true, value: { aborted: true } } });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("waits() + '/' + relay.posted.length")), QStringLiteral("1000/1"));
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay)")), QStringLiteral("vanilla.events,vanilla.events"));
    worker.evaluate(QStringLiteral("relay.say({ ticket: relay.posted[1].ticket, answer: { ok: true, value: { stale: true } } });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relay.posted.length + '/' + timers.length")), QStringLiteral("2/0"));
}

void tst_cdpshims::aSecondRelayPortTakesTheFirstsPlace(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying());
    worker.evaluate(QStringLiteral("var first = relaying();"
                                   "var out = 'pending';"
                                   "chrome.tabs.create({ url: 'https://a.example/' }).then(function(){ out = 'resolved'; }, function(e){ out = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(first)")), QStringLiteral("tabs.create"));
    worker.evaluate(QStringLiteral("var second = relaying();"));
    QCOMPARE(Value(&worker, QStringLiteral("first.disconnected")), QStringLiteral("1"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("chrome.tabs.create is not available in this browser"));
    QCOMPARE(Value(&worker, QStringLiteral("second.posted.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("var next = 'pending'; chrome.tabs.get(3).then(function(){}, function(e){ next = e.message; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(second)")), QStringLiteral("tabs.get"));
}

void tst_cdpshims::theWorkerAsksItsPageForTheRelayWhenItHasNoPort(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying());
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(sent)")), QStringLiteral("[[{\"__vanillaRelay\":\"wanted\"}]]"));
    worker.evaluate(QStringLiteral("chrome.tabs.query({}); chrome.tabs.get(1);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("clock += 1000; chrome.tabs.get(2);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent.length")), QStringLiteral("2"));
    worker.evaluate(QStringLiteral("clock += 1000; var relay = relaying(); chrome.tabs.get(3);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent.length + '/' + relayCalls(relay).split(',').length")), QStringLiteral("2/4"));

    worker.evaluate(QStringLiteral(
        "var got = [];"
        "chrome.runtime.onMessage.addListener(function(m){ got.push(JSON.stringify(m)); return false; });"
        "var said = String(listeners[0]({ __vanillaRelay: 'wanted' }, { id: chrome.runtime.id, url: 'u' }, function(){}));"));
    QCOMPARE(Value(&worker, QStringLiteral("got.length + '/' + said")), QStringLiteral("0/undefined"));

    QJSEngine qt;
    MakeAsking(&qt, Fetching() + HostFetches());
    QCOMPARE(Value(&qt, QStringLiteral("sent.length")), QStringLiteral("0"));
}

void tst_cdpshims::aTabWhichCameOverTheRelayIsWrittenDown(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying());
    worker.evaluate(QStringLiteral(
        "var relay = relaying(), senders = [];"
        "chrome.runtime.onMessage.addListener(function(m, s){ senders.push(s); return false; });"
        "listeners[0]({ __vanillaEnvelope: 1, from: { nonce: '11000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, message: {} },"
        "             { id: chrome.runtime.id, url: 'u' }, function(){});"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay)")), QStringLiteral("vanilla.tabOf"));
    worker.evaluate(QStringLiteral("relay.say({ ticket: relay.posted[0].ticket, answer: { ok: true, value: { id: 555, index: 3 } } });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("senders.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("senders[0].tab.id + ':' + senders[0].tab.index")), QStringLiteral("555:3"));
    worker.evaluate(QStringLiteral("var out = 'pending';"
                                   "chrome.tabs.query({}).then(function(){ out = 'query saw ' + senders[0].tab.index; });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("relay.say({ ticket: relay.posted[1].ticket,"
                                   "            answer: { ok: true, value: [{ id: 111, index: 0 }, { id: 555, index: 7 }] } });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("query saw 7"));
}

void tst_cdpshims::whatWebView2FiresItselfIsNotSubscribedTo(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying() + QStringLiteral("delete chrome.downloads;"));
    worker.evaluate(QStringLiteral("var relay = relaying();"
                                   "chrome.downloads.onCreated.addListener(function(){ heard.push('download'); });"
                                   "chrome.downloads.onChanged.addListener(function(){ heard.push('changed'); });"
                                   "chrome.contextMenus.onClicked.addListener(function(){ heard.push('clicked'); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relay.posted.length + '/' + timers.length")), QStringLiteral("0/0"));
    worker.evaluate(QStringLiteral("chrome.tabs.onCreated.addListener(function(){});"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay)")), QStringLiteral("vanilla.events"));
}

void tst_cdpshims::theRelayMakesOneRequestOfTheHostForEachTicket(){
    QJSEngine relay;
    MakeRelay(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("ports.length + '/' + ports[0].name")), QStringLiteral("1/__vanilla_relay__"));
    relay.evaluate(QStringLiteral("var port = ports[0];"
                                  "answers.push({ ok: true, value: [{ id: 12, index: 3 }] });"
                                  "port.say({ ticket: 7, call: { api: 'tabs.query', args: [{ active: true }] } });"));
    Settle(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("fetched.length + ' ' + fetched[0].url + ' ' + fetched[0].options.method + ' ' + ('body' in fetched[0].options)")),
             QStringLiteral("1 vanilla-extension://host/call POST false"));
    QCOMPARE(Value(&relay, QStringLiteral("fetched[0].options.headers['X-Vanilla-Key']")), RelayKey());
    QCOMPARE(Value(&relay, QStringLiteral("decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"tabs.query\",\"args\":[{\"active\":true}]}"));
    QVERIFY(!Value(&relay, QStringLiteral("fetched[0].url + JSON.stringify(Object.keys(fetched[0].options))")).contains(RelayKey()));
    QCOMPARE(Value(&relay, QStringLiteral("JSON.stringify(port.posted)")),
             QStringLiteral("[{\"ticket\":7,\"answer\":{\"ok\":true,\"value\":[{\"id\":12,\"index\":3}]}}]"));

    relay.evaluate(QStringLiteral("answers.push({ ok: false, error: 'No tab with id: 99.' }, 'DOWN', 'NOT JSON', { value: 'no ok at all' });"
                                  "port.say({ ticket: 8, call: { api: 'tabs.get', args: [99] } });"
                                  "port.say({ ticket: 9, call: { api: 'tabs.get', args: [1] } });"
                                  "port.say({ ticket: 10, call: { api: 'tabs.get', args: [2] } });"
                                  "port.say({ ticket: 11, call: { api: 'tabs.get', args: [3] } });"));
    Settle(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("port.posted.slice(1).map(function(m){ return m.ticket + ':' + (m.answer.error || 'ok'); })"
                                          "  .sort().join(' | ')")),
             QStringLiteral("10:chrome.tabs.get is not available in this browser"
                            " | 11:chrome.tabs.get is not available in this browser"
                            " | 8:No tab with id: 99."
                            " | 9:chrome.tabs.get is not available in this browser"));

    relay.evaluate(QStringLiteral("var was = fetched.length + '/' + port.posted.length;"
                                  "port.say(5); port.say(null); port.say({ ticket: 12 });"
                                  "port.say({ ticket: 'x', call: { api: 'tabs.get', args: [1] } });"
                                  "port.say({ ticket: 13, call: { api: 5, args: [] } });"
                                  "port.say({ ticket: 14, call: { api: 'tabs.get', args: 'one' } });"));
    Settle(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("(fetched.length + '/' + port.posted.length) === was")), QStringLiteral("true"));
    QCOMPARE(Value(&relay, QStringLiteral("timers.length")), QStringLiteral("0"));
}

void tst_cdpshims::theRelayStopsAHeldCallWhenThePortGoesAndTellsTheHost(){
    QJSEngine relay;
    MakeRelay(&relay);
    relay.evaluate(QStringLiteral("var port = ports[0];"
                                  "answers.push('HELD', 'HELD');"
                                  "port.say({ ticket: 3, call: { api: 'vanilla.events', args: ['a1b2'] } });"
                                  "port.say({ ticket: 4, call: { api: 'tabs.get', args: [1] } });"));
    Settle(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("fetched.length + '/' + (!!fetched[0].options.signal) + (!!fetched[1].options.signal) + '/' + aborts")),
             QStringLiteral("2/truefalse/0"));
    relay.evaluate(QStringLiteral("port.die();"));
    Settle(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("aborts")), QStringLiteral("1"));
    QCOMPARE(Value(&relay, QStringLiteral("decodeURIComponent(fetched[2].options.headers['X-Vanilla-Call'])")),
             QStringLiteral("{\"api\":\"vanilla.abort\",\"args\":[\"a1b2\"]}"));
    QCOMPARE(Value(&relay, QStringLiteral("fetched[2].options.headers['X-Vanilla-Key']")), RelayKey());
    QCOMPARE(Value(&relay, QStringLiteral("port.posted.length + '/' + waits()")), QStringLiteral("0/300000"));
}

void tst_cdpshims::theRelayOpensAPortAgainOnlyWhenTheWorkerAsks(){
    QJSEngine relay;
    MakeRelay(&relay);
    relay.evaluate(QStringLiteral("ports[0].die();"));
    QCOMPARE(Value(&relay, QStringLiteral("ports.length + '/' + waits()")), QStringLiteral("1/300000"));
    relay.evaluate(QStringLiteral("listeners[0]({ __vanillaRelay: 'wanted' }, { id: 'somebodyelsesextension' });"
                                  "listeners[0]({ __vanillaRelay: 'wanted' }, { id: 'somebodyelsesextension', tab: { id: 5 } });"
                                  "listeners[0]({ hello: 1 }, { id: chrome.runtime.id });"
                                  "listeners[0](5, { id: chrome.runtime.id });"));
    QCOMPARE(Value(&relay, QStringLiteral("ports.length")), QStringLiteral("1"));
    relay.evaluate(QStringLiteral("listeners[0]({ __vanillaRelay: 'wanted' }, { id: chrome.runtime.id });"));
    QCOMPARE(Value(&relay, QStringLiteral("ports.length + '/' + ports[1].name")), QStringLiteral("2/__vanilla_relay__"));
    relay.evaluate(QStringLiteral("listeners[0]({ __vanillaRelay: 'wanted' }, { id: chrome.runtime.id });"));
    QCOMPARE(Value(&relay, QStringLiteral("ports.length")), QStringLiteral("2"));
    relay.evaluate(QStringLiteral("ports[1].die();"
                                  "listeners[0]({ __vanillaRelay: 'wanted' }, { id: chrome.runtime.id, tab: { id: 5 } });"));
    QCOMPARE(Value(&relay, QStringLiteral("ports.length + '/' + ports[2].name")), QStringLiteral("3/__vanilla_relay__"));
    relay.evaluate(QStringLiteral("ports[2].die(); var still = timers.length; runNextTimer();"));
    QCOMPARE(Value(&relay, QStringLiteral("still + '/' + ports.length + '/' + timers.length")), QStringLiteral("1/4/0"));
    relay.evaluate(QStringLiteral("ports[3].die();"
                                  "chrome.runtime.connect = function(){ throw new Error('no worker'); };"
                                  "runNextTimer();"));
    QCOMPARE(Value(&relay, QStringLiteral("ports.length + '/' + waits()")), QStringLiteral("4/300000"));
}

void tst_cdpshims::theRelayKeepsAtMostSixtyFourCallsOut(){
    QJSEngine relay;
    MakeRelay(&relay);
    relay.evaluate(QStringLiteral("var port = ports[0];"
                                  "for (var i = 1; i <= 64; i++) { answers.push('HELD');"
                                  "  port.say({ ticket: i, call: { api: 'tabs.get', args: [i] } }); }"
                                  "port.say({ ticket: 65, call: { api: 'tabs.get', args: [65] } });"));
    Settle(&relay);
    QCOMPARE(Value(&relay, QStringLiteral("fetched.length + '/' + JSON.stringify(port.posted)")),
             QStringLiteral("64/[{\"ticket\":65,\"answer\":{\"ok\":false,\"error\":\"chrome.tabs.get is not available in this browser\"}}]"));
}

void tst_cdpshims::aContentScriptAsksTheEngineWhichStorageItHas(){
    const QString world = QStringLiteral(
        "var brandReads = 0;"
        "var navigator = { userAgentData: { get brands(){ brandReads++; throw new Error('no'); } } };"
        "var engineSyncCalls = [];"
        "var engineHeard = [];"
        "var engineEvent = { addListener: function(f){ engineHeard.push(f); },"
        "                    removeListener: function(f){ engineHeard = engineHeard.filter(function(g){ return g !== f; }); },"
        "                    hasListener: function(f){ return engineHeard.indexOf(f) >= 0; },"
        "                    hasListeners: function(){ return engineHeard.length > 0; } };"
        "var engineRefuses = function(verb){ return function(){ engineSyncCalls.push(verb);"
        "  return Promise.reject(new Error('\"sync\" is not available in this instance of Chrome')); }; };"
        "chrome.storage.sync = { get: engineRefuses('get'), set: engineRefuses('set'), remove: engineRefuses('remove'),"
        "                        clear: engineRefuses('clear'), onChanged: engineEvent };"
        "chrome.storage.onChanged = engineEvent;"
        "var enginesSync = chrome.storage.sync;");
    QJSEngine content;
    Make(&content, true, world);
    QCOMPARE(Value(&content, QStringLiteral("brandReads + '/' + (chrome.storage.sync === enginesSync) + '/' + engineSyncCalls.length"
                                            " + '/' + reads.length + '/' + timers.length")),
             QStringLiteral("0/true/0/0/0"));

    content.evaluate(QStringLiteral("var told = [];"
                                    "var listener = function(changes, area){ told.push(area + ':' + JSON.stringify(changes)); };"
                                    "chrome.storage.onChanged.addListener(listener);"));
    QCOMPARE(Value(&content, QStringLiteral("engineHeard.length + '/' + chrome.storage.onChanged.hasListener(listener)")),
             QStringLiteral("1/true"));
    Settle(&content);
    QCOMPARE(Value(&content, QStringLiteral("engineSyncCalls.join() + '/' + engineHeard.length"
                                            " + '/' + (chrome.storage.onChanged === engineEvent)"
                                            " + '/' + chrome.storage.onChanged.hasListener(listener)")),
             QStringLiteral("get/0/false/true"));
    content.evaluate(QStringLiteral("chrome.storage.local.set({ a: 1 });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("told.join()")), QStringLiteral("local:{\"a\":{\"newValue\":1}}"));

    content.evaluate(QStringLiteral("var out = 'pending';"
                                    "chrome.storage.sync.set({ b: 2 }).then(function(){ return chrome.storage.sync.get(null); })"
                                    "  .then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("out")), QStringLiteral("{\"b\":2}"));
    QCOMPARE(Value(&content, QStringLiteral("engineSyncCalls.join() + '/' + (chrome.storage.sync === enginesSync)"
                                            " + '/' + JSON.stringify(Object.keys(localArea.data).sort())")),
             QStringLiteral("get/false/[\"__vanilla_sync__:b\",\"a\"]"));
    QCOMPARE(Value(&content, QStringLiteral("brandReads")), QStringLiteral("0"));

    QJSEngine first;
    Make(&first, true, world);
    first.evaluate(QStringLiteral("var got = 'pending';"
                                  "chrome.storage.sync.set({ c: 3 }).then(function(){ got = 'set'; }, function(e){ got = e.message; });"));
    QTRY_COMPARE(Value(&first, QStringLiteral("got")), QStringLiteral("set"));
    QCOMPARE(Value(&first, QStringLiteral("engineSyncCalls.join() + '/' + JSON.stringify(Object.keys(localArea.data))")),
             QStringLiteral("set/[\"__vanilla_sync__:c\"]"));
    first.evaluate(QStringLiteral("var back = 'pending';"
                                  "chrome.storage.sync.get('c', function(v){ back = JSON.stringify(v); });"));
    QTRY_COMPARE(Value(&first, QStringLiteral("back")), QStringLiteral("{\"c\":3}"));

    QJSEngine vimium;
    Make(&vimium, true, world);
    vimium.evaluate(QStringLiteral("localArea.data['__vanilla_sync__:d'] = 4;"
                                   "chrome.storage.onChanged.addListener(function(){});"
                                   "var read = 'pending';"
                                   "chrome.storage.sync.get(null).then(function(v){ read = JSON.stringify(v); }, function(e){ read = e.message; });"));
    QTRY_COMPARE(Value(&vimium, QStringLiteral("read")), QStringLiteral("{\"d\":4}"));
    QCOMPARE(Value(&vimium, QStringLiteral("engineSyncCalls.join()")), QStringLiteral("get,get"));

    QJSEngine beside;
    Make(&beside, true, world);
    beside.evaluate(QStringLiteral("var both = [];"
                                   "chrome.storage.sync.set({ e: 5 }).then(function(){ both.push('set'); }, function(e){ both.push(e.message); });"
                                   "chrome.storage.sync.get('x').then(function(v){ both.push(JSON.stringify(v)); }, function(e){ both.push(e.message); });"));
    QTRY_COMPARE(Value(&beside, QStringLiteral("both.length")), QStringLiteral("2"));
    QCOMPARE(Value(&beside, QStringLiteral("both.sort().join('|') + '/' + engineSyncCalls.join()")),
             QStringLiteral("set|{}/set,get"));
}

void tst_cdpshims::aStorageWhichAnswersIsLeftToTheEngine(){
    QJSEngine content;
    Make(&content, true, QStringLiteral(
        "var engineSyncCalls = [], engineStore = { a: 1 };"
        "var engineHeard = [];"
        "var engineEvent = { addListener: function(f){ engineHeard.push(f); },"
        "                    removeListener: function(f){ engineHeard = engineHeard.filter(function(g){ return g !== f; }); },"
        "                    hasListener: function(f){ return engineHeard.indexOf(f) >= 0; },"
        "                    hasListeners: function(){ return engineHeard.length > 0; } };"
        "chrome.storage.sync = { get: function(keys){ engineSyncCalls.push('get');"
        "                          return Promise.resolve(JSON.parse(JSON.stringify(engineStore))); },"
        "                        set: function(items){ engineSyncCalls.push('set');"
        "                          for (var k in items) engineStore[k] = items[k]; return Promise.resolve(); },"
        "                        remove: function(){ engineSyncCalls.push('remove'); return Promise.resolve(); },"
        "                        clear: function(){ engineSyncCalls.push('clear'); return Promise.resolve(); },"
        "                        onChanged: engineEvent };"
        "chrome.storage.onChanged = engineEvent;"
        "var enginesSync = chrome.storage.sync;"));
    content.evaluate(QStringLiteral("var listener = function(){};"
                                    "chrome.storage.onChanged.addListener(listener);"
                                    "var out = 'pending';"
                                    "chrome.storage.sync.get(null).then(function(v){ out = JSON.stringify(v); }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("out")), QStringLiteral("{\"a\":1}"));
    Settle(&content);
    QCOMPARE(Value(&content, QStringLiteral("(chrome.storage.sync === enginesSync) + '/' + (chrome.storage.onChanged === engineEvent)"
                                            " + '/' + engineHeard.length + '/' + timers.length + '/' + reads.length")),
             QStringLiteral("true/true/1/0/0"));
    content.evaluate(QStringLiteral("var wrote = 'pending';"
                                    "chrome.storage.sync.set({ b: 2 }).then(function(){ wrote = JSON.stringify(engineStore); }, function(e){ wrote = e.message; });"));
    QTRY_COMPARE(Value(&content, QStringLiteral("wrote")), QStringLiteral("{\"a\":1,\"b\":2}"));
    QCOMPARE(Value(&content, QStringLiteral("engineSyncCalls.join() + '/' + JSON.stringify(Object.keys(localArea.data))")),
             QStringLiteral("get,get,set/[]"));

    QJSEngine calling;
    Make(&calling, true, QStringLiteral(
        "var engineStore = { a: 1 };"
        "chrome.storage.sync = { get: function(){ return Promise.resolve(JSON.parse(JSON.stringify(engineStore))); },"
        "                        set: function(items){ for (var k in items) engineStore[k] = items[k]; return Promise.resolve(); },"
        "                        remove: function(){ return Promise.resolve(); }, clear: function(){ return Promise.resolve(); },"
        "                        onChanged: nobody };"
        "var enginesSync = chrome.storage.sync;"));
    calling.evaluate(QStringLiteral("var out = 'pending';"
                                    "chrome.storage.sync.set({ b: 2 }).then(function(){ out = JSON.stringify(engineStore); }, function(e){ out = e.message; });"));
    QTRY_COMPARE(Value(&calling, QStringLiteral("out")), QStringLiteral("{\"a\":1,\"b\":2}"));
    Settle(&calling);
    QCOMPARE(Value(&calling, QStringLiteral("(chrome.storage.sync === enginesSync) + '/' + JSON.stringify(Object.keys(localArea.data))"
                                            " + '/' + timers.length")),
             QStringLiteral("true/[]/0"));
}

void tst_cdpshims::aContentScriptAsksTheEngineForTheMessagesToo(){
    QJSEngine speaking;
    Make(&speaking, true, Messages() + QStringLiteral(
        "var i18nAsked = [];"
        "chrome.i18n = { getMessage: function(n){ i18nAsked.push(n); return 'the engine\\'s own'; },"
        "                getUILanguage: function(){ return 'engine'; } };"
        "var enginesI18n = chrome.i18n;"));
    QCOMPARE(Value(&speaking, QStringLiteral("i18nAsked.join() + '/' + chrome.i18n.getMessage('greeting')"
                                             " + '/' + (chrome.i18n === enginesI18n)")),
             QStringLiteral("greeting/the engine's own/true"));

    QJSEngine silent;
    Make(&silent, true, Messages() + QStringLiteral(
        "chrome.i18n = { getMessage: function(){ return ''; }, getUILanguage: function(){ return 'engine'; } };"));
    QCOMPARE(Value(&silent, QStringLiteral("chrome.i18n.getMessage('greeting') + '/' + chrome.i18n.getUILanguage()")),
             QStringLiteral("ours/engine"));
}

void tst_cdpshims::aMenuClickOnWebView2HandsTheApplicationsTab(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking());
    worker.evaluate(MenuTabs());
    worker.evaluate(QStringLiteral(
        "var handed = [];"
        "chrome.contextMenus.onClicked.addListener(function(info, tab){ handed.push([info, tab]); });"
        "var info = { menuItemId: 'save', pageUrl: 'https://a.example/', frameId: 0 };"
        "var engineTab = { id: 945653585, index: 0, windowId: 945650001, openerTabId: 945653000, groupId: 945650002,"
        "                  splitViewId: 945650003, title: 'A', url: 'https://a.example/', active: true };"
        "var engineWas = JSON.stringify(engineTab);"
        "click(info, engineTab);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("menuArgs() + '/' + handed.length")),
             QStringLiteral("vanilla.menuTab[\"https://a.example/\"]/0"));
    worker.evaluate(QStringLiteral("menuTab(0, 132, 4);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("handed.length")), QStringLiteral("1"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(handed[0][1])")),
             QStringLiteral("{\"id\":132,\"index\":4,\"windowId\":1,\"groupId\":-1,\"splitViewId\":-1,"
                            "\"title\":\"A\",\"url\":\"https://a.example/\",\"active\":true}"));
    QCOMPARE(Value(&worker, QStringLiteral("(handed[0][0] === info) + '/' + (handed[0][1] !== engineTab) + '/' + (JSON.stringify(engineTab) === engineWas)")),
             QStringLiteral("true/true/true"));
    worker.evaluate(QStringLiteral("click({ menuItemId: 'save', pageUrl: 'https://b.example/' }, { id: 945653586, index: 0, windowId: 9 });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("menuTab(1, 133, 0);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("handed.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(handed[1][1]) + '/' + menuArgs()")),
             QStringLiteral("{\"id\":133,\"index\":0,\"windowId\":1}/vanilla.menuTab[\"https://a.example/\"] vanilla.menuTab[\"https://b.example/\"]"));
    QCOMPARE(Value(&worker, QStringLiteral("hostFetches()")), QStringLiteral("0"));
}

void tst_cdpshims::aMenuClickWithNoTabOfOursIsHandedTheNoneIds(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking());
    worker.evaluate(MenuTabs());
    worker.evaluate(QStringLiteral(
        "var handed = [];"
        "chrome.contextMenus.onClicked.addListener(function(info, tab){"
        "  handed.push(tab === undefined ? 'none' : tab.id + '/' + tab.windowId + '/' + tab.index); });"
        "function engines(){ return { id: 945653585, index: 0, windowId: 945650001 }; }"
        "click({ pageUrl: 'p0' }, engines()); click({ pageUrl: 'p1' }, engines()); click({ pageUrl: 'p2' }, engines());"
        "click({ pageUrl: 'p3' }, engines()); click({ pageUrl: 'p4' }, engines()); click({ pageUrl: 'p5' }, engines());"
        "click({ menuItemId: 'no address' }, engines()); click(undefined, engines());"
        "click({ pageUrl: 5 }, undefined);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("menuArgs()")),
             QStringLiteral("vanilla.menuTab[\"p0\"] vanilla.menuTab[\"p1\"] vanilla.menuTab[\"p2\"] vanilla.menuTab[\"p3\"]"
                            " vanilla.menuTab[\"p4\"] vanilla.menuTab[\"p5\"] vanilla.menuTab[\"\"] vanilla.menuTab[\"\"]"
                            " vanilla.menuTab[\"\"]"));
    worker.evaluate(QStringLiteral(
        "answer(0, { ok: false, error: 'no view' });"
        "answer(1, { ok: true, value: { id: 0, index: 1 } });"
        "answer(2, { ok: true, value: { id: 5 } });"
        "answer(3, { ok: true, value: { id: 5, index: -1 } });"
        "answer(4, { ok: true, value: null });"
        "answer(5, { ok: true, value: 'x' });"
        "menuTab(7, 132, 2);"
        "menuTab(8, 132, 2);"));
    Settle(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("handed.length + '/' + waitingHere()")), QStringLiteral("6/1"));
    worker.evaluate(QStringLiteral("runTimers();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("handed.length")), QStringLiteral("9"));
    QCOMPARE(Value(&worker, QStringLiteral("handed.join()")),
             QStringLiteral("-1/-1/0,-1/-1/0,-1/-1/0,-1/-1/0,-1/-1/0,-1/-1/0,-1/-1/0,132/1/2,none"));
}

void tst_cdpshims::aMenuClickIsAskedOnceAndHandedInTheEnginesOrder(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking());
    worker.evaluate(MenuTabs());
    worker.evaluate(QStringLiteral("click({ pageUrl: 'p0' }, { id: 945653585, index: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("asked.length")), QStringLiteral("1"));
    worker.evaluate(QStringLiteral("menuTab(0, 10, 0);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("asked.length + '/' + got.length")), QStringLiteral("1/0"));

    worker.evaluate(QStringLiteral(
        "chrome.contextMenus.onClicked.addListener(hear('a'));"
        "click({ pageUrl: 'p1' }, { id: 945653585, index: 0 });"
        "click({ pageUrl: 'p2' }, { id: 945653585, index: 0 });"
        "click({ pageUrl: 'p3' }, { id: 945653585, index: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("asked.length")), QStringLiteral("4"));
    worker.evaluate(QStringLiteral("menuTab(3, 13, 3);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("menuTab(2, 12, 2);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("0"));
    worker.evaluate(QStringLiteral("menuTab(1, 11, 1);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("3"));
    QCOMPARE(Value(&worker, QStringLiteral("got.join()")), QStringLiteral("a:p1=11@1/1,a:p2=12@2/1,a:p3=13@3/1"));
    QCOMPARE(Value(&worker, QStringLiteral("asked.length")), QStringLiteral("4"));
}

void tst_cdpshims::atMostSixteenMenuClicksWaitAndTheRestAreNotAsked(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking());
    worker.evaluate(MenuTabs());
    worker.evaluate(QStringLiteral(
        "chrome.contextMenus.onClicked.addListener(hear('a'));"
        "for (var i = 0; i < 17; i++) click({ pageUrl: 'p' + i }, { id: 945653585, index: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("asked.length + '/' + waitingHere()")), QStringLiteral("16/16"));
    worker.evaluate(QStringLiteral("for (var j = 15; j >= 0; j--) menuTab(j, 100 + j, j);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("17"));
    QCOMPARE(Value(&worker, QStringLiteral("got.slice(0, 2).join() + ' ' + got.slice(15).join()")),
             QStringLiteral("a:p0=100@0/1,a:p1=101@1/1 a:p15=115@15/1,a:p16=-1@0/-1"));
    QCOMPARE(Value(&worker, QStringLiteral("got.map(function(g){ return g.split(':')[1].split('=')[0]; }).join()")),
             QStringLiteral("p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16"));
    worker.evaluate(QStringLiteral("click({ pageUrl: 'again' }, { id: 945653585, index: 0 });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("asked.length")), QStringLiteral("17"));
    worker.evaluate(QStringLiteral("menuTab(16, 200, 0);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("18"));
    QCOMPARE(Value(&worker, QStringLiteral("got[17]")), QStringLiteral("a:again=200@0/1"));
}

void tst_cdpshims::theMenuClicksListenersAreReadAsEachIsHandedOver(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking());
    worker.evaluate(MenuTabs());
    worker.evaluate(QStringLiteral(
        "var onClicked = chrome.contextMenus.onClicked;"
        "var thrower = function(){ got.push('threw'); throw new Error('a listener of the extension\\'s'); };"
        "var one = hear('one'), two = hear('two'), three = hear('three');"
        "var before = onClicked.hasListeners() + '/' + onClicked.hasListener(one);"
        "onClicked.addListener(thrower); onClicked.addListener(one); onClicked.addListener(one); onClicked.addListener(two);"
        "var after = onClicked.hasListeners() + '/' + onClicked.hasListener(one) + '/' + onClicked.hasListener(three);"
        "click({ pageUrl: 'A' }, { id: 945653585, index: 0 });"
        "click({ pageUrl: 'B' }, { id: 945653585, index: 0 });"
        "onClicked.removeListener(two); onClicked.addListener(three);"));
    QCOMPARE(Value(&worker, QStringLiteral("before + ' ' + after")), QStringLiteral("false/false true/true/false"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("menuTab(0, 1, 0); menuTab(1, 2, 1);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.length")), QStringLiteral("6"));
    QCOMPARE(Value(&worker, QStringLiteral("got.join()")),
             QStringLiteral("threw,one:A=1@0/1,three:A=1@0/1,threw,one:B=2@1/1,three:B=2@1/1"));
    worker.evaluate(QStringLiteral("onClicked.removeListener(thrower); onClicked.removeListener(one); onClicked.removeListener(three);"));
    QCOMPARE(Value(&worker, QStringLiteral("onClicked.hasListeners() + '/' + onClicked.hasListener(one)")), QStringLiteral("false/false"));
}

void tst_cdpshims::theMenuClickStandInIsOnBothRootsAndTheRestIsTheEngines(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking() + QStringLiteral(
        "enginesMenus.ACTION_MENU_TOP_LEVEL_LIMIT = 6; enginesMenus.ContextType = { PAGE: 'page' };"
        "self.browser = { contextMenus: enginesMenus, runtime: chrome.runtime };"));
    worker.evaluate(MenuTabs());
    QCOMPARE(Value(&worker, QStringLiteral("(chrome.contextMenus === browser.contextMenus) + '/' + (chrome.contextMenus.onClicked === browser.contextMenus.onClicked)"
                                           " + '/' + (chrome.contextMenus.onClicked === enginesClicked)")),
             QStringLiteral("true/true/false"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.contextMenus.create({ id: 'm', title: 'M' }) + '/' + browser.contextMenus.create({ id: 'n' })"
                                           " + '/' + chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT + '/' + (chrome.contextMenus.ContextType === enginesMenus.ContextType)")),
             QStringLiteral("m/n/6/true"));
    worker.evaluate(QStringLiteral("chrome.contextMenus.update('m', {}); chrome.contextMenus.removeAll();"));
    QCOMPARE(Value(&worker, QStringLiteral("edgeCalls.join()")),
             QStringLiteral("contextMenus.onClicked.addListener,contextMenus.create,contextMenus.create,contextMenus.update,contextMenus.removeAll"));
    worker.evaluate(QStringLiteral("browser.contextMenus.onClicked.addListener(hear('b'));"
                                   "click({ pageUrl: 'u' }, { id: 945653585, index: 0 });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("menuTab(0, 132, 3);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("got.join()")), QStringLiteral("b:u=132@3/1"));
    QCOMPARE(Value(&worker, QStringLiteral("enginesClicked.list.length + '/' + chrome.contextMenus.onClicked.hasListeners()")),
             QStringLiteral("1/true"));
    QCOMPARE(Value(&worker, QStringLiteral("hostFetches()")), QStringLiteral("0"));
}

void tst_cdpshims::noMenuClickStandInWhereItIsNotWanted(){
    {
        QJSEngine qt;
        MakeAsking(&qt, Fetching() + HostFetches() + EdgesOwn(), Clicking());
        QCOMPARE(Value(&qt, QStringLiteral("enginesClicked.list.length + '/' + (chrome.contextMenus.onClicked === enginesClicked)")),
                 QStringLiteral("0/false"));
        qt.evaluate(QStringLiteral("chrome.contextMenus.onClicked.addListener(function(){});"));
        QTRY_COMPARE(Value(&qt, QStringLiteral("hostFetches()")), QStringLiteral("1"));
        QCOMPARE(Value(&qt, QStringLiteral("JSON.parse(") + LastCall() + QStringLiteral(").api")),
                 QStringLiteral("vanilla.events"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, QStringLiteral("delete chrome.contextMenus;"));
        worker.evaluate(QStringLiteral("chrome.contextMenus.onClicked.addListener(function(){});"));
        Settle(&worker);
        QCOMPARE(Value(&worker, QStringLiteral("chrome.contextMenus.onClicked.hasListeners() + '/' + waitingHere() + '/' + edgeCalls.join()")),
                 QStringLiteral("false/0/"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, QStringLiteral("enginesMenus.onClicked = {};"));
        QCOMPARE(Value(&worker, QStringLiteral("(chrome.contextMenus === enginesMenus) + '/' + (chrome.contextMenus.onClicked === enginesMenus.onClicked)")),
                 QStringLiteral("true/true"));
    }
    {
        QJSEngine worker;
        worker.evaluate(World());
        worker.evaluate(QStringLiteral("document = undefined;") + Fetching() + HostFetches() + EdgeBrands() + EdgesOwn() + Clicking());
        const QJSValue made = worker.evaluate(Cdp::WorkerShim());
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        QCOMPARE(Value(&worker, QStringLiteral("enginesClicked.list.length + '/' + (chrome.contextMenus === enginesMenus)"
                                               " + '/' + (chrome.contextMenus.onClicked === enginesClicked)")),
                 QStringLiteral("0/true/true"));
    }
}

static QString CopyingEngine(){
    return QStringLiteral(
        "var engineAsked = [], engineWaits = [];"
        "function engineCall(name){ return function(){"
        "  var args = Array.prototype.slice.call(arguments), cb = typeof args[args.length - 1] === 'function' ? args.pop() : null;"
        "  engineAsked.push(name + JSON.stringify(args));"
        "  var refuse = args[0] && args[0].title === 'refused' ? 'Cannot create item with duplicate id ' + args[0].id"
        "             : args[1] && args[1].title === 'refused' ? 'Cannot find menu item with id ' + args[0] : null;"
        "  engineWaits.push({ cb: cb, error: refuse });"
        "  return name === 'create' ? args[0].id : undefined; }; }"
        "function engineAnswers(){ var due = engineWaits; engineWaits = [];"
        "  due.forEach(function(w){ chrome.runtime.lastError = w.error ? { message: w.error } : undefined;"
        "    if (w.cb) w.cb(); chrome.runtime.lastError = undefined; }); }"
        "chrome.contextMenus = { create: engineCall('create'), update: engineCall('update'), remove: engineCall('remove'),"
        "                        removeAll: engineCall('removeAll'), onClicked: enginesClicked };"
        "chrome.runtime.getManifest = function(){ return { permissions: ['contextMenus'] }; };");
}

void tst_cdpshims::theEnginesMenuCallsAreToldTogether(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking() + CopyingEngine());
    worker.evaluate(MenuTabs() + QStringLiteral(
        "function told(){ return asked.filter(function(a){ return a.call.api === 'vanilla.menuMirror'; })"
        "  .map(function(a){ return a.call.args[0].map(function(c){ return c[0] + (c[1][0] && c[1][0].id !== undefined ? ':' + c[1][0].id : c[1].length ? ':' + c[1][0] : ''); }).join('+'); }).join(' / '); }"
        "function ack(){ for (var i = asked.length - 1; i >= 0; i--) if (asked[i].call.api === 'vanilla.menuMirror' && !asked[i].done) { asked[i].done = 1; answer(i, { ok: true }); return; } }"
        "var heard = 'none', made = chrome.contextMenus.create({ id: 'a', title: 'A' });"
        "chrome.contextMenus.create({ id: 'a', title: 'refused' }, function(){ heard = chrome.runtime.lastError && chrome.runtime.lastError.message; });"
        "chrome.contextMenus.create({ id: 5, title: 'Five', onclick: function(){} });"
        "var updated = 'pending'; chrome.contextMenus.update(5, { title: 'V' }).then(function(){ updated = 'resolved'; });"
        "var refused = 'pending'; chrome.contextMenus.update('gone', { title: 'refused' }).then(function(){ refused = 'resolved'; }, function(e){ refused = e.message; });"
        "chrome.contextMenus.update('gone', { title: 'refused' }, function(){});"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("made + '/' + told() + '/' + engineAsked.length")), QStringLiteral("a//6"));
    worker.evaluate(QStringLiteral("engineAnswers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("told() + '/' + heard + '/' + updated + '/' + refused")),
             QStringLiteral("create:a+create:5+update:5/Cannot create item with duplicate id a/resolved/Cannot find menu item with id gone"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(asked.filter(function(a){ return a.call.api === 'vanilla.menuMirror'; })[0].call.args[0][1])")),
             QStringLiteral("[\"create\",[{\"id\":5,\"title\":\"Five\"}]]"));
    worker.evaluate(QStringLiteral("chrome.contextMenus.remove('a'); chrome.contextMenus.removeAll(); engineAnswers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("told()")), QStringLiteral("create:a+create:5+update:5"));
    worker.evaluate(QStringLiteral("for (var i = asked.length - 1; i >= 0; i--) if (asked[i].call.api === 'vanilla.menuMirror') { asked[i].done = 1; answer(i, { ok: false, error: 'no' }); break; }"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("told()")), QStringLiteral("create:a+create:5+update:5 / remove:a+removeAll"));
    worker.evaluate(QStringLiteral("ack(); var long = new Array(1001).join('x');"
                                   "for (var i = 0; i < 20; i++) chrome.contextMenus.create({ id: 'l' + i, title: long });"
                                   "engineAnswers();"));
    Settle(&worker);
    const int batches = Value(&worker, QStringLiteral("asked.filter(function(a){ return a.call.api === 'vanilla.menuMirror'; }).length")).toInt();
    QCOMPARE(batches, 3);
    QCOMPARE(Value(&worker, QStringLiteral("encodeURIComponent(JSON.stringify(asked[asked.length - 1].call.args[0])).length < 9000")), QStringLiteral("true"));
    QCOMPARE(Value(&worker, QStringLiteral("hostFetches()")), QStringLiteral("0"));
}

void tst_cdpshims::aChoiceOfTheButtonsMenuComesOverTheRelayPage(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Clicking() + CopyingEngine());
    worker.evaluate(QStringLiteral(
        "chrome.contextMenus.create({ id: 7, title: 'Seven' });"
        "engineAnswers();"
        "var handed = [], messages = [];"
        "chrome.contextMenus.onClicked.addListener(function(info, tab){ handed.push(JSON.stringify(info) + '@' + (tab && tab.id)); });"
        "chrome.runtime.onMessage.addListener(function(m){ messages.push(JSON.stringify(m)); });"
        "var relayUrl = 'chrome-extension://' + chrome.runtime.id + '/vanilla_relay.html';"
        "function send(m, s){ listeners.forEach(function(f){ f(m, s, function(){}); }); }"
        "var chosen = { __vanillaMenuChosen: 1, args: [{ menuItemId: '7', pageUrl: 'https://a.example/' }, { id: 132 }] };"
        "send(chosen, { id: chrome.runtime.id, url: relayUrl, tab: { id: 1156791862 } });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("handed.join(' ') + '/' + messages.length")),
             QStringLiteral("{\"menuItemId\":7,\"pageUrl\":\"https://a.example/\"}@132/0"));
    worker.evaluate(QStringLiteral(
        "send(chosen, { id: chrome.runtime.id, url: 'chrome-extension://' + chrome.runtime.id + '/popup.html' });"
        "send(chosen, { id: 'another', url: 'chrome-extension://another/vanilla_relay.html' });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("handed.length + '/' + messages.length")), QStringLiteral("1/2"));
}

void tst_cdpshims::theRelaysScriptCarriesTheChoiceAsData(){
    const QString nasty = QStringLiteral("q\"'\\ </script><script>ran=1</script> ") + QChar(0x2028) + QChar(0x2029)
        + QChar(0x01) + QChar(0x1F) + QStringLiteral("\"});ran=1;({\"");
    QJsonObject info;
    info[QStringLiteral("menuItemId")] = QStringLiteral("a");
    info[QStringLiteral("pageUrl")] = QStringLiteral("https://a.example/?") + nasty;
    info[QStringLiteral("selectionText")] = nasty;
    const QJsonArray args = QJsonArray() << info << QJsonObject{{QStringLiteral("id"), 132}, {QStringLiteral("title"), nasty}};
    QJSEngine page;
    page.evaluate(QStringLiteral("var ran = 0, sent = [];"
                                 "var chrome = { runtime: { sendMessage: function(m){ sent.push(m); return Promise.resolve(); } } };"));
    const QJSValue result = page.evaluate(Cdp::MenuChosenScript(args));
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    QCOMPARE(page.evaluate(QStringLiteral("ran + '/' + sent.length")).toString(), QStringLiteral("0/1"));
    QCOMPARE(page.evaluate(QStringLiteral("sent[0].__vanillaMenuChosen")).toInt(), 1);
    QCOMPARE(page.evaluate(QStringLiteral("sent[0].args[0].selectionText")).toString(), nasty);
    QCOMPARE(page.evaluate(QStringLiteral("sent[0].args[0].pageUrl")).toString(), QStringLiteral("https://a.example/?") + nasty);
    QCOMPARE(page.evaluate(QStringLiteral("sent[0].args[1].title + '/' + sent[0].args[1].id")).toString(), nasty + QStringLiteral("/132"));
}

void tst_cdpshims::theEnginesScriptingIsHandedTheEnginesTab(){
    QJSEngine worker;
    ScriptingWorker(&worker);
    worker.evaluate(QStringLiteral(
        "var f = function(a){ return a; };"
        "var injection = { target: { tabId: 132, frameIds: [0, 7] }, files: ['a.js'], world: 'ISOLATED' };"
        "var target = injection.target, was = JSON.stringify(injection);"
        "outcome(chrome.scripting.executeScript(injection));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved [{\"frameId\":0,\"documentId\":\"D\",\"result\":\"engine\"}]"));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting()")),
             QStringLiteral("executeScript[{\"target\":{\"tabId\":945653585,\"frameIds\":[0,7]},\"files\":[\"a.js\"],\"world\":\"ISOLATED\"}]"));
    QCOMPARE(Value(&worker, QStringLiteral("scriptingCalls.length + '/' + (scriptingCalls[0].self === enginesScripting)"
                                           " + '/' + (scriptingCalls[0].args[0] !== injection) + '/' + (scriptingCalls[0].args[0].target !== target)"
                                           " + '/' + (injection.target === target) + '/' + (JSON.stringify(injection) === was)")),
             QStringLiteral("1/true/true/true/true/true"));
    worker.evaluate(QStringLiteral(
        "outcome(chrome.scripting.executeScript({ target: { tabId: 132, allFrames: true, documentIds: ['X'] }, func: f, args: [1, 'x'], world: 'MAIN' }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.slice(0, 8)")), QStringLiteral("resolved"));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting() + '/' + (scriptingCalls[1].args[0].func === f)")),
             QStringLiteral("executeScript[{\"target\":{\"tabId\":945653585,\"allFrames\":true,\"documentIds\":[\"X\"]},\"args\":[1,\"x\"],\"world\":\"MAIN\"}]/true"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 132, frameIds: [3] }, css: 'a{}', origin: 'USER' }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting()")),
             QStringLiteral("insertCSS[{\"target\":{\"tabId\":945653585,\"frameIds\":[3]},\"css\":\"a{}\",\"origin\":\"USER\"}]"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.removeCSS({ target: { tabId: 132 }, files: ['b.css'] }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting() + '/' + scriptingCalls.length")),
             QStringLiteral("removeCSS[{\"target\":{\"tabId\":945653585},\"files\":[\"b.css\"]}]/4"));
    worker.evaluate(QStringLiteral("scriptingFails = 'Cannot access contents of the page.';"
                                   "outcome(chrome.scripting.executeScript({ target: { tabId: 132 }, func: f }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed Cannot access contents of the page."));
    QCOMPARE(Value(&worker, QStringLiteral("one.posted.filter(function(m){ return !!m.run; }).length + '/' + hostFetches()")),
             QStringLiteral("0/0"));
}

void tst_cdpshims::theNewestCertainLinkWithAnEnginesTabIsTheOne(){
    QJSEngine worker;
    ScriptingWorker(&worker);
    worker.evaluate(QStringLiteral(
        "var two = linkingTab('a2000000000000000000000000000000', 945653600, 200);"
        "var none = linkingTab('a3000000000000000000000000000000', null, 300);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("tabOf('a2000000000000000000000000000000', 132); tabOf('a3000000000000000000000000000000', 132);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ target: { tabId: 132 }, files: ['a.js'] }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.slice(0, 8)")), QStringLiteral("resolved"));
    QCOMPARE(Value(&worker, QStringLiteral("scriptingCalls[0].args[0].target.tabId")), QStringLiteral("945653600"));
    worker.evaluate(QStringLiteral("two.die(); outcome(chrome.scripting.executeScript({ target: { tabId: 132 }, files: ['a.js'] }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("scriptingCalls.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("scriptingCalls[1].args[0].target.tabId")), QStringLiteral("945653585"));
    worker.evaluate(QStringLiteral(
        "Math.random = function(){ return 0; };"
        "var guessed = linkingTab('a4000000000000000000000000000000', 945653700, 400);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runAt(3000);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ target: { tabId: 1073741824 }, files: ['a.js'] }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 1073741824."));
    QCOMPARE(Value(&worker, QStringLiteral("scriptingCalls.length")), QStringLiteral("2"));
    worker.evaluate(QStringLiteral("chrome.tabs.sendMessage(1073741824, 'hello');"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(guessed.last())")), QStringLiteral("{\"id\":1,\"message\":\"hello\"}"));
}

void tst_cdpshims::noLinkIsNoTabAndTheEngineIsNotAsked(){
    QJSEngine worker;
    ScriptingWorker(&worker);
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ target: { tabId: 77 }, files: ['a.js'] }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 77."));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 0 }, css: 'a{}' }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 0."));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.removeCSS({ target: { tabId: -1 }, css: 'a{}' }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: -1."));
    worker.evaluate(QStringLiteral("one.die(); outcome(chrome.scripting.executeScript({ target: { tabId: 132 }, func: function(){} }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed No tab with id: 132."));
    QCOMPARE(Value(&worker, QStringLiteral("scriptingCalls.length")), QStringLiteral("0"));
}

void tst_cdpshims::theScriptingCallbackIsAnsweredAsChromesIs(){
    QJSEngine worker;
    ScriptingWorker(&worker);
    worker.evaluate(QStringLiteral(
        "var called = [];"
        "function noting(r){ called.push(JSON.stringify(r) + '/' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'none')); }"
        "var said = chrome.scripting.executeScript({ target: { tabId: 132 }, files: ['a.js'] }, noting);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called.join()")),
                 QStringLiteral("[{\"frameId\":0,\"documentId\":\"D\",\"result\":\"engine\"}]/none"));
    QCOMPARE(Value(&worker, QStringLiteral("(said === undefined) + '/' + scriptingCalls[0].args.length")), QStringLiteral("true/1"));
    worker.evaluate(QStringLiteral("chrome.scripting.insertCSS({ target: { tabId: 77 }, css: 'a{}' }, noting);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runAt(0);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("called[1] + '/' + scriptingCalls.length")), QStringLiteral("undefined/No tab with id: 77./1"));
    worker.evaluate(QStringLiteral("scriptingFails = 'Cannot access contents of the page.';"
                                   "chrome.scripting.removeCSS({ target: { tabId: 132 }, css: 'a{}' }, noting);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runAt(0);"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("called.length")), QStringLiteral("3"));
    QCOMPARE(Value(&worker, QStringLiteral("called[2] + '/' + lastScripting() + '/' + (chrome.runtime.lastError === undefined)")),
             QStringLiteral("undefined/Cannot access contents of the page./removeCSS[{\"target\":{\"tabId\":945653585},\"css\":\"a{}\"}]/true"));
}

void tst_cdpshims::aTabIdWhichIsNoNumberGoesToTheEngineAsItIs(){
    QJSEngine worker;
    ScriptingWorker(&worker);
    worker.evaluate(QStringLiteral(
        "var noting = function(){};"
        "var a = { target: { tabId: '132' }, files: ['a.js'] }, b = { files: ['a.js'] }, c = { target: { tabId: 1.5 }, css: 'a{}' };"
        "chrome.scripting.executeScript(a, noting); chrome.scripting.executeScript(b); chrome.scripting.insertCSS(c);"
        "chrome.scripting.removeCSS();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("scriptingCalls.length")), QStringLiteral("4"));
    QCOMPARE(Value(&worker, QStringLiteral("(scriptingCalls[0].args[0] === a) + '/' + (scriptingCalls[0].args[1] === noting)"
                                           " + '/' + (scriptingCalls[1].args[0] === b) + '/' + (scriptingCalls[2].args[0] === c)"
                                           " + '/' + scriptingCalls[3].args.length + '/' + a.target.tabId")),
             QStringLiteral("true/true/true/true/0/132"));
}

void tst_cdpshims::theScriptingStandInIsOnBothRootsAndTheRestIsTheEngines(){
    QJSEngine worker;
    ScriptingWorker(&worker, QStringLiteral(
        "var enginesOtherScripting = { executeScript: engineCall('browser.executeScript') };"
        "self.browser = { scripting: enginesOtherScripting, runtime: chrome.runtime };"));
    QCOMPARE(Value(&worker, QStringLiteral("(chrome.scripting === browser.scripting) + '/' + (chrome.scripting === enginesScripting)"
                                           " + '/' + (chrome.scripting.executeScript === browser.scripting.executeScript)"
                                           " + '/' + (chrome.scripting.insertCSS === browser.scripting.insertCSS)"
                                           " + '/' + (chrome.scripting.executeScript === enginesScripting.executeScript)"
                                           " + '/' + (chrome.scripting.ExecutionWorld === enginesScripting.ExecutionWorld)")),
             QStringLiteral("true/false/true/true/false/true"));
    worker.evaluate(QStringLiteral("outcome(browser.scripting.registerContentScripts([{ id: 'x', js: ['a.js'], matches: ['<all_urls>'] }]));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved \"the engine's own\""));
    QCOMPARE(Value(&worker, QStringLiteral("lastScripting() + '/' + (scriptingCalls[0].self === enginesScripting)")),
             QStringLiteral("registerContentScripts[[{\"id\":\"x\",\"js\":[\"a.js\"],\"matches\":[\"<all_urls>\"]}]]/true"));
    worker.evaluate(QStringLiteral("outcome(browser.scripting.executeScript({ target: { tabId: 132 }, files: ['a.js'] }));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.slice(0, 8)")), QStringLiteral("resolved"));
    QCOMPARE(Value(&worker, QStringLiteral("scriptingCalls.map(function(c){ return c.name; }).join() + '/' + scriptingCalls[1].args[0].target.tabId")),
             QStringLiteral("registerContentScripts,executeScript/945653585"));
}

void tst_cdpshims::noScriptingStandInWhereItIsNotWanted(){
    {
        QJSEngine qt;
        MakeAsking(&qt, Fetching(), Scripting());
        QCOMPARE(Value(&qt, QStringLiteral("chrome.scripting === enginesScripting")), QStringLiteral("true"));
    }
    {
        QJSEngine qt;
        MakeAsking(&qt, Fetching(), Scripting() + QStringLiteral("delete chrome.scripting;"));
        qt.evaluate(Linking());
        qt.evaluate(QStringLiteral(
            "answers.push({ ok: true, value: { id: 555, index: 0 } });"
            "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
        Settle(&qt);
        qt.evaluate(QStringLiteral("chrome.scripting.executeScript({ target: { tabId: 555 }, func: function(){ return 1; } });"));
        Settle(&qt);
        QCOMPARE(Value(&qt, QStringLiteral("top.last().run + '/' + top.last().codes.length + '/' + scriptingCalls.length")),
                 QStringLiteral("1/1/0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, Scripting() + QStringLiteral("delete chrome.scripting;"));
        worker.evaluate(ScriptingTabs());
        worker.evaluate(QStringLiteral("var one = linkingTab('a1000000000000000000000000000000', 945653585, 100);"));
        Settle(&worker);
        worker.evaluate(QStringLiteral("tabOf('a1000000000000000000000000000000', 132);"));
        Settle(&worker);
        worker.evaluate(QStringLiteral("chrome.scripting.executeScript({ target: { tabId: 132 }, func: function(){ return 1; } });"));
        Settle(&worker);
        QCOMPARE(Value(&worker, QStringLiteral("one.last().run + '/' + one.last().codes.length + '/' + scriptingCalls.length")),
                 QStringLiteral("1/1/0"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, Scripting() + QStringLiteral("delete chrome.scripting; chrome.runtime.getManifest = function(){ return {}; };"));
        worker.evaluate(ScriptingTabs());
        worker.evaluate(QStringLiteral("outcome(chrome.scripting.executeScript({ target: { tabId: 132 }, files: ['a.js'] }));"));
        QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed chrome.scripting.executeScript is not available in this browser"));
    }
    {
        QJSEngine worker;
        worker.evaluate(World());
        worker.evaluate(QStringLiteral("document = undefined;") + Fetching() + HostFetches() + EdgeBrands() + EdgesOwn() + Scripting());
        const QJSValue made = worker.evaluate(Cdp::WorkerShim());
        QVERIFY2(!made.isError(), qPrintable(made.toString()));
        QCOMPARE(Value(&worker, QStringLiteral("(chrome.scripting === enginesScripting) + '/' + (chrome.scripting.executeScript === enginesScripting.executeScript)")),
                 QStringLiteral("true/true"));
    }
}

void tst_cdpshims::onInstalledIsToldWhatTheApplicationSaysIsOwed(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { reason: 'update', previousVersion: '1.0.0', extra: 1 } });"
        "var got = [], late = [];"
        "var one = function(d){ got.push(JSON.stringify(d)); };"
        "chrome.runtime.onInstalled.addListener(one);"
        "chrome.runtime.onInstalled.addListener(function(){ throw new Error('a listener of its own'); });"
        "chrome.runtime.onInstalled.addListener(function(d){ got.push('second ' + d.reason); });"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("0"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.runtime.onInstalled.hasListener(one)")), QStringLiteral("true"));
    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.parse(decodeURIComponent(fetched[0].options.headers['X-Vanilla-Call'])).api")),
             QStringLiteral("vanilla.installed"));
    QCOMPARE(Value(&worker, QStringLiteral("got.join(' | ')")),
             QStringLiteral("{\"reason\":\"update\",\"previousVersion\":\"1.0.0\"} | second update"));
    worker.evaluate(QStringLiteral("chrome.runtime.onInstalled.addListener(function(d){ late.push(d); });"));
    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("late.length + '/' + fetched.length")), QStringLiteral("0/1"));

    {
        QJSEngine none;
        MakeAsking(&none, Fetching());
        Drain(&none);
        QCOMPARE(Value(&none, QStringLiteral("fetched.length")), QStringLiteral("0"));
    }
    for(const QString &answer : {QStringLiteral("{ reason: 'install', previousVersion: '9' }"), QStringLiteral("null"),
                                 QStringLiteral("{ reason: 'chrome_update' }")}){
        QJSEngine other;
        MakeAsking(&other, Fetching());
        other.evaluate(QStringLiteral("answers.push({ ok: true, value: ") + answer + QStringLiteral(" });"
                       "var got = []; chrome.runtime.onInstalled.addListener(function(d){ got.push(JSON.stringify(d)); });"));
        Drain(&other);
        QCOMPARE(Value(&other, QStringLiteral("got.join(' | ')")),
                 answer.contains(QStringLiteral("'install'")) ? QStringLiteral("{\"reason\":\"install\"}") : QString());
    }
}

void tst_cdpshims::onInstalledIsTheEnginesWhereTheShimAsksNobody(){
    const QString engines = QStringLiteral("var enginesInstalled = { addListener: function(){}, removeListener: function(){}, hasListener: function(){ return false; } };"
                                           "chrome.runtime.onInstalled = enginesInstalled;");
    QJSEngine keyless;
    keyless.evaluate(World());
    keyless.evaluate(QStringLiteral("document = undefined;") + Fetching() + engines);
    QVERIFY(!keyless.evaluate(Cdp::WorkerShim()).isError());
    Drain(&keyless);
    QCOMPARE(Value(&keyless, QStringLiteral("(chrome.runtime.onInstalled === enginesInstalled) + '/' + fetched.length")), QStringLiteral("true/0"));

    QJSEngine edge;
    MakeEdgeWorker(&edge, engines);
    Drain(&edge);
    QCOMPARE(Value(&edge, QStringLiteral("chrome.runtime.onInstalled === enginesInstalled")), QStringLiteral("true"));
    QVERIFY(!Value(&edge, QStringLiteral("report")).contains(QStringLiteral("runtime.onInstalled")));
}

void tst_cdpshims::theButtonIsPressedByTheExtensionItself(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Calls());
    worker.evaluate(QStringLiteral(
        "var out = [];"
        "answers.push({ ok: true }, { ok: false, error: 'Extension does not have a popup on the active tab.' });"
        "chrome.action.openPopup().then(function(v){ out.push('opened ' + v); });"
        "chrome.action.openPopup({ windowId: 1 }).then(function(){ out.push('opened?'); }, function(e){ out.push(e.message); });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out.length")), QStringLiteral("2"));
    QCOMPARE(Value(&worker, QStringLiteral("out.join(' | ')")), QStringLiteral("opened undefined | Extension does not have a popup on the active tab."));
    QCOMPARE(Value(&worker, QStringLiteral("call(0).api + JSON.stringify(call(0).args) + ' ' + JSON.stringify(call(1).args)")),
             QStringLiteral("action.openPopup[] [{\"windowId\":1}]"));
}

void tst_cdpshims::theShortcutsAreAskedOfAndToldByTheApplication(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + Calls());
    worker.evaluate(QStringLiteral(
        "var out = 'pending', log = [];"
        "answers.push({ ok: true, value: [{ name: 'go', description: 'Go', shortcut: 'Ctrl+Shift+Y' }] });"
        "chrome.commands.getAll().then(function(v){ out = JSON.stringify(v); }, function(e){ out = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("[{\"name\":\"go\",\"description\":\"Go\",\"shortcut\":\"Ctrl+Shift+Y\"}]"));
    QCOMPARE(Value(&worker, QStringLiteral("call(0).api + JSON.stringify(call(0).args)")), QStringLiteral("commands.getAll[]"));
    worker.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "chrome.commands.onCommand.addListener(function(name, tab){ log.push(name + ' ' + (tab && tab.id)); });"));
    QCOMPARE(Value(&worker, QStringLiteral("call(1).api")), QStringLiteral("vanilla.events"));
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { events: [{ name: 'commands.onCommand', args: ['go', { id: 7, index: 0 }] }], order: [7] } }, 'HELD');"
        "held.pop()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("log.join(' | ')")), QStringLiteral("go 7"));

    QJSEngine edge;
    MakeEdgeWorker(&edge, Saying() + Calls() + QStringLiteral(
        "chrome.commands = { getAll: function(){ return Promise.resolve('the engine\\'s own'); },"
        "                    onCommand: { addListener: function(){}, removeListener: function(){}, hasListener: function(){ return false; } } };"
        "var enginesCommands = chrome.commands;"));
    QCOMPARE(Value(&edge, QStringLiteral("(chrome.commands.onCommand === enginesCommands.onCommand) + '/' + (typeof chrome.commands.getAll)")),
             QStringLiteral("true/function"));
    edge.evaluate(QStringLiteral("var got = 'pending'; chrome.commands.getAll().then(function(v){ got = v; });"));
    QTRY_COMPARE(Value(&edge, QStringLiteral("got")), QStringLiteral("the engine's own"));
}

void tst_cdpshims::onWebView2TheEnginesOwnEventsAreNotAskedFor(){
    QJSEngine worker;
    MakeEdgeWorker(&worker, Relaying() + Saying() + QStringLiteral("delete chrome.commands; delete chrome.notifications;"));
    worker.evaluate(QStringLiteral(
        "var relay = relaying();"
        "chrome.commands.onCommand.addListener(function(){});"
        "chrome.notifications.onClicked.addListener(function(){});"
        "chrome.notifications.onClosed.addListener(function(){});"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay)")), QString());
    worker.evaluate(QStringLiteral("chrome.tabs.onCreated.addListener(function(){});"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("relayCalls(relay) + '/' + JSON.stringify(relay.posted[0].call.args[1])")),
             QStringLiteral("vanilla.events/[\"tabs.onCreated\"]"));
}

void tst_cdpshims::onStartupIsFiredOnceWhereTheApplicationSaysSo(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Calls());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: true });"
        "var got = [], late = 0;"
        "var one = function(){ got.push(arguments.length); };"
        "chrome.runtime.onStartup.addListener(one);"
        "chrome.runtime.onStartup.addListener(function(){ throw new Error('a listener of its own'); });"
        "chrome.runtime.onStartup.addListener(function(){ got.push('second'); });"));
    QCOMPARE(Value(&worker, QStringLiteral("fetched.length")), QStringLiteral("0"));
    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("call(0).api + '/' + call(0).args.length")), QStringLiteral("vanilla.startup/0"));
    QCOMPARE(Value(&worker, QStringLiteral("got.join(' | ')")), QStringLiteral("0 | second"));
    QCOMPARE(Value(&worker, QStringLiteral("chrome.runtime.onStartup.hasListener(one) + '/' + chrome.runtime.onStartup.hasListeners()")),
             QStringLiteral("true/true"));
    worker.evaluate(QStringLiteral("chrome.runtime.onStartup.addListener(function(){ late++; });"));
    Drain(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("late + '/' + fetched.length")), QStringLiteral("0/1"));

    for(const QString &answer : {QStringLiteral("false"), QStringLiteral("null"), QStringLiteral("1"), QStringLiteral("{}")}){
        QJSEngine other;
        MakeAsking(&other, Fetching());
        other.evaluate(QStringLiteral("answers.push({ ok: true, value: ") + answer + QStringLiteral(" });"
                       "var got = 0; chrome.runtime.onStartup.addListener(function(){ got++; });"));
        Drain(&other);
        QCOMPARE(Value(&other, QStringLiteral("got + '/' + fetched.length")), QStringLiteral("0/1"));
    }
    {
        QJSEngine both;
        MakeAsking(&both, Fetching(), Calls());
        both.evaluate(QStringLiteral(
            "answers.push({ ok: true, value: { reason: 'install' } }, { ok: true, value: true });"
            "var got = [];"
            "chrome.runtime.onInstalled.addListener(function(d){ got.push('installed ' + d.reason); });"
            "chrome.runtime.onStartup.addListener(function(){ got.push('startup'); });"));
        Drain(&both);
        QCOMPARE(Value(&both, QStringLiteral("call(0).api + ' ' + call(1).api")), QStringLiteral("vanilla.installed vanilla.startup"));
        QCOMPARE(Value(&both, QStringLiteral("got.slice().sort().join(' | ')")), QStringLiteral("installed install | startup"));
    }
    const QString engines = QStringLiteral("var enginesStartup = { addListener: function(){}, removeListener: function(){}, hasListener: function(){ return false; } };"
                                           "chrome.runtime.onStartup = enginesStartup;");
    QJSEngine keyless;
    keyless.evaluate(World());
    keyless.evaluate(QStringLiteral("document = undefined;") + Fetching() + engines);
    QVERIFY(!keyless.evaluate(Cdp::WorkerShim()).isError());
    Drain(&keyless);
    QCOMPARE(Value(&keyless, QStringLiteral("(chrome.runtime.onStartup === enginesStartup) + '/' + fetched.length")), QStringLiteral("true/0"));
    QJSEngine edge;
    MakeEdgeWorker(&edge, engines);
    Drain(&edge);
    QCOMPARE(Value(&edge, QStringLiteral("chrome.runtime.onStartup === enginesStartup")), QStringLiteral("true"));
    QVERIFY(!Value(&edge, QStringLiteral("report")).contains(QStringLiteral("runtime.onStartup")));
}

void tst_cdpshims::theWorkerRegistersContentScriptsForTheDocumentsWhichLink(){
    {
        QJSEngine plain;
        MakeAsking(&plain, Fetching(), Saying());
        plain.evaluate(QStringLiteral("var out = 'pending'; chrome.scripting.registerContentScripts([]).then(function(){ out = 'made?'; }, function(e){ out = e.message; });"));
        QTRY_COMPARE(Value(&plain, QStringLiteral("out")), QStringLiteral("chrome.scripting.registerContentScripts is not available in this browser"));
        plain.evaluate(QStringLiteral("out = 'pending'; chrome.scripting.getRegisteredContentScripts().then(function(r){ out = JSON.stringify(r); }, function(e){ out = e.message; });"));
        QTRY_COMPARE(Value(&plain, QStringLiteral("out")), QStringLiteral("[]"));
    }
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting', 'tabs'], host_permissions: ['<all_urls>'] }; };"));
    worker.evaluate(Linking() + Serving());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } });"
        "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + ' ' + top.posted[0].linked")), QStringLiteral("1 1"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.updateContentScripts([]));"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed chrome.scripting.updateContentScripts is not available in this browser"));

    worker.evaluate(QStringLiteral(R"js(
        outcome(chrome.scripting.registerContentScripts([
          { id: 'one', js: ['cs/a.js', '/cs/b.js'], css: ['cs/s.css'], matches: ['*://*.example/*'], runAt: 'document_start' },
          { id: 'two', js: ['cs/a.js'], matches: ['<all_urls>'], excludeMatches: ['*://a.example/top'], allFrames: true, persistAcrossSessions: false },
          { id: 'main', js: ['cs/a.js'], matches: ['<all_urls>'], world: 'MAIN', matchOriginAsFallback: true, excludeMatches: [] }]));
    )js"));
    Settle(&worker);
    worker.evaluate(QStringLiteral(
        "answers.unshift({ ok: true });"
        "var sending = timers.filter(function(t){ return t.f.name === 'mainFlush'; });"
        "timers = timers.filter(function(t){ return t.f.name !== 'mainFlush'; }); sending.forEach(function(t){ t.f(); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("served.join()")), QStringLiteral("cs/a.js,cs/b.js,cs/s.css"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(top.posted.slice(1))")),
             QStringLiteral(R"([{"style":1,"add":"S","script":"one","nth":0},{"run":2,"codes":["A","B"],"at":"document_start","script":"one"}])"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.getRegisteredContentScripts());"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")),
             QStringLiteral(R"(resolved [{"id":"one","matches":["*://*.example/*"],"allFrames":false,"matchOriginAsFallback":false,"runAt":"document_start","world":"ISOLATED","persistAcrossSessions":true,"js":["cs/a.js","/cs/b.js"],"css":["cs/s.css"]},)"
                            R"({"id":"two","matches":["<all_urls>"],"allFrames":true,"matchOriginAsFallback":false,"runAt":"document_idle","world":"ISOLATED","persistAcrossSessions":false,"excludeMatches":["*://a.example/top"],"js":["cs/a.js"]},)"
                            R"({"id":"main","matches":["<all_urls>"],"allFrames":false,"matchOriginAsFallback":true,"runAt":"document_idle","world":"MAIN","persistAcrossSessions":true,"excludeMatches":[],"js":["cs/a.js"]}])"));
    worker.evaluate(QStringLiteral("chrome.scripting.getRegisteredContentScripts({ ids: ['two', 'nosuch'] }, function(r){ out = 'called ' + r.length + ' ' + r[0].id; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("called 1 two"));

    worker.evaluate(QStringLiteral(
        "var sub = linking('a2000000000000000000000000000000', 5, 'https://b.example/frame', 200);"
        "var other = linking('a3000000000000000000000000000000', 0, 'https://a.example/other?x=1', 300);"
        "var mute = aPort('__vanilla_link__', { id: chrome.runtime.id, url: '' }); connects[0](mute);"
        "mute.say({ link: { nonce: 'a4000000000000000000000000000000', frameId: 0, tabUrl: 'https://a.example/page', since: 400 } });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(sub.posted.slice(1))")),
             QStringLiteral(R"([{"run":1,"codes":["A"],"at":"document_idle","script":"two"}])"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(other.posted.slice(1))")),
             QStringLiteral(R"([{"style":1,"add":"S","script":"one","nth":0},{"run":2,"codes":["A","B"],"at":"document_start","script":"one"},{"run":3,"codes":["A"],"at":"document_idle","script":"two"}])"));
    QCOMPARE(Value(&worker, QStringLiteral("mute.posted.length + ' ' + mute.posted[0].linked")), QStringLiteral("1 1"));
    worker.evaluate(QStringLiteral("top.say({ styled: 1 }); top.say({ ran: 2 }); sub.say({ ran: 1 }); other.say({ styled: 1 }); other.say({ ran: 2 }); other.say({ ran: 3 });"));

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([{ id: 'three', js: ['cs/b.js'], matches: ['*://a.example/*'] }]));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + '/' + sub.posted.length + '/' + other.posted.length + '/' + mute.posted.length")), QStringLiteral("4/2/5/1"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(top.last())")), QStringLiteral(R"({"run":3,"codes":["B"],"at":"document_idle","script":"three"})"));
    worker.evaluate(QStringLiteral("var again = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 500);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("again.posted.length + ' ' + again.posted.map(function(m){ return m.script; }).join()")), QStringLiteral("4 ,one,one,three"));

    QStringList refused;
    foreach(const QString &call, QStringList()
            << "[{ id: '_x', js: ['cs/a.js'], matches: ['<all_urls>'] }]"
            << "[{ id: '', js: ['cs/a.js'], matches: ['<all_urls>'] }]"
            << "[{ id: 'd', js: ['cs/a.js'], matches: ['<all_urls>'] }, { id: 'd', js: ['cs/a.js'], matches: ['<all_urls>'] }]"
            << "[{ id: 'one', js: ['cs/a.js'], matches: ['<all_urls>'] }]"
            << "[{ id: 'x', matches: ['<all_urls>'] }]"
            << "[{ id: 'x', js: ['cs/a.js'] }]"
            << "[{ id: 'x', js: ['cs/a.js'], matches: [] }]"
            << "[{ id: 'x', js: ['cs/a.js'], matches: ['bogus'] }]"
            << "[{ id: 'x', js: ['cs/a.js'], matches: ['<all_urls>'], excludeMatches: ['http://a.com'] }]"
            << "[{ id: 'x', js: ['cs/nosuch.js'], matches: ['<all_urls>'] }, { id: 'y', js: ['cs/a.js'], matches: ['<all_urls>'] }]"
            << "[{ id: 'x', css: ['../a.css'], matches: ['<all_urls>'] }]"
            << "[{ id: 'x', js: ['cs/a.js'], matches: ['<all_urls>'], runAt: 'now' }]"
            << "[{ id: 'x', js: ['cs/a.js'], matches: ['<all_urls>'], world: 'OTHER' }]"
            << "[{ id: 'x', js: ['cs/a.js'], matches: ['<all_urls>'], allFrames: 'yes' }]"
            << "{ id: 'x' }"){
        worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts(%1));").arg(call));
        Settle(&worker);
        refused << Value(&worker, QStringLiteral("out"));
    }
    QCOMPARE(refused, QStringList()
             << "failed Script's ID '_x' must not start with '_'"
             << "failed Script's ID must not be empty"
             << "failed Duplicate script ID 'd'"
             << "failed Duplicate script ID 'one'"
             << "failed Script with ID 'x' must specify at least one js or css file."
             << "failed Script with ID 'x' must specify 'matches'."
             << "failed Script with ID 'x' must specify at least one match."
             << "failed Script with ID 'x' has invalid value for matches[0]."
             << "failed Script with ID 'x' has invalid value for excludeMatches[0]."
             << "failed Could not load javascript 'cs/nosuch.js' for content script."
             << "failed Could not load css '../a.css' for content script."
             << "failed Script with ID 'x' has invalid value for 'runAt'."
             << "failed Script with ID 'x' has invalid value for 'world'."
             << "failed Script with ID 'x' has invalid value for 'allFrames'."
             << "failed Invalid value for argument 1. Expected a list of scripts.");
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.getRegisteredContentScripts().then(function(r){ return r.map(function(s){ return s.id; }); }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral(R"(resolved ["one","two","main","three"])"));
    QCOMPARE(Value(&worker, QStringLiteral("again.posted.length")), QStringLiteral("4"));

    worker.evaluate(QStringLiteral(
        "var twice = [];"
        "chrome.scripting.registerContentScripts([{ id: 'race', js: ['cs/a.js'], matches: ['<all_urls>'] }]).then(function(){ twice.push('first ok'); }, function(e){ twice.push('first ' + e.message); });"
        "chrome.scripting.registerContentScripts([{ id: 'race', js: ['cs/b.js'], matches: ['<all_urls>'] }]).then(function(){ twice.push('second ok'); }, function(e){ twice.push('second ' + e.message); });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("twice.join(' / ')")), QStringLiteral("first ok / second Duplicate script ID 'race'"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.getRegisteredContentScripts({ ids: ['race'] }).then(function(r){ return r.map(function(s){ return s.js.join(); }); }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral(R"(resolved ["cs/a.js"])"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.unregisterContentScripts({ ids: ['race'] }));"));
    Settle(&worker);

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.unregisterContentScripts({ ids: ['two', 'nosuch'] }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed Nonexistent script ID 'nosuch'"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.unregisterContentScripts({ ids: ['two'] }).then(function(){ return chrome.scripting.getRegisteredContentScripts(); }).then(function(r){ return r.length; }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved 3"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.unregisterContentScripts().then(function(){ return chrome.scripting.getRegisteredContentScripts(); }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved []"));
    worker.evaluate(QStringLiteral("var late = linking('a5000000000000000000000000000000', 0, 'https://a.example/late', 600);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("late.posted.length")), QStringLiteral("1"));
}

void tst_cdpshims::theListOfThePagesOwnWorldGoesToTheHostByName(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['<all_urls>'] }; };"));
    worker.evaluate(Serving() + QStringLiteral(
        "function sent(){ return fetched.filter(function(f){ return decodeURIComponent(f.options.headers['X-Vanilla-Call']).indexOf('vanilla.mainScripts') >= 0; }); }"
        "function lastSent(){ var s = sent(); return s.length ? s[s.length - 1].options.body : 'none'; }"
        "var M = { id: 'm', js: ['/cs/a.js'], matches: ['<all_urls>'], excludeMatches: ['https://b.example/x'], allFrames: true, runAt: 'document_start', world: 'MAIN', matchOriginAsFallback: true };"
        "var I = { id: 'i', js: ['cs/b.js'], matches: ['<all_urls>'] };"));

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([M, I]));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")), QStringLiteral("0 pending"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true }); runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + JSON.parse(decodeURIComponent(sent()[0].options.headers['X-Vanilla-Call'])).args.length")),
             QStringLiteral("1 0"));
    QCOMPARE(Value(&worker, QStringLiteral("lastSent()")),
             QStringLiteral("[[{\"id\":\"m\",\"js\":[\"cs/a.js\"],\"matches\":[\"<all_urls>\"],\"allFrames\":true,\"runAt\":\"document_start\",\"persistAcrossSessions\":true,\"excludeMatches\":[\"https://b.example/x\"]}]]"));
    QCOMPARE(Value(&worker, QStringLiteral("served.join()")), QStringLiteral("cs/a.js,cs/b.js"));

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([{ id: 'j', js: ['cs/b.js'], matches: ['<all_urls>'] }]));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")), QStringLiteral("1 resolved undefined"));

    worker.evaluate(QStringLiteral(
        "outcome(chrome.scripting.unregisterContentScripts().then(function(){"
        "  var again = chrome.scripting.registerContentScripts([M, I]); runTimers(); return again; }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")), QStringLiteral("1 resolved undefined"));

    worker.evaluate(QStringLiteral(
        "answers.push('HELD');"
        "outcome(chrome.scripting.registerContentScripts([{ id: 'm2', js: ['cs/b.js'], matches: ['https://c.example/*'], world: 'MAIN' }]));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")), QStringLiteral("2 pending"));
    worker.evaluate(QStringLiteral(
        "var later = 'pending';"
        "chrome.scripting.unregisterContentScripts({ ids: ['m2'] }).then(function(){"
        "  return chrome.scripting.registerContentScripts([{ id: 'm3', js: ['cs/a.js'], matches: ['https://d.example/*'], world: 'MAIN' }]); })"
        "  .then(function(){ later = 'resolved'; });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + later")), QStringLiteral("2 pending"));
    worker.evaluate(QStringLiteral("answers.push({ ok: true }, { ok: true }); held.shift()();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + later")), QStringLiteral("3 resolved"));
    QCOMPARE(Value(&worker, QStringLiteral("JSON.parse(lastSent())[0].map(function(s){ return s.id; }).join()")), QStringLiteral("m,m3"));

    worker.evaluate(QStringLiteral(
        "answers.push({ ok: false, error: 'no' });"
        "outcome(chrome.scripting.registerContentScripts([{ id: 'm4', js: ['cs/a.js'], matches: ['<all_urls>'], world: 'MAIN' }]));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")), QStringLiteral("4 resolved undefined"));
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length")), QStringLiteral("4"));

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([{ id: 'm5', js: ['cs/nosuch.js'], matches: ['<all_urls>'], world: 'MAIN' }]));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")),
             QStringLiteral("4 failed Could not load javascript 'cs/nosuch.js' for content script."));

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([{ id: 'mc', css: ['cs/s.css'], matches: ['<all_urls>'], world: 'MAIN' }]));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + out")), QStringLiteral("4 resolved undefined"));

    worker.evaluate(QStringLiteral("answers.push({ ok: true }); outcome(chrome.scripting.unregisterContentScripts());"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    worker.evaluate(QStringLiteral("runTimers();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("sent().length + ' ' + lastSent()")), QStringLiteral("5 [[]]"));
}

void tst_cdpshims::theWorkerPutsAStyleSheetIntoADocumentAndTakesItOut(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['<all_urls>'] }; };"));
    worker.evaluate(Linking() + Serving());
    worker.evaluate(QStringLiteral(
        "answers.push({ ok: true, value: { id: 555, index: 0 } }, { ok: true, value: { id: 555, index: 0 } });"
        "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
        "var sub = linking('a2000000000000000000000000000000', 5, 'https://a.example/frame', 200);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("typeof chrome.scripting.insertCSS + ' ' + typeof chrome.scripting.removeCSS")), QStringLiteral("function function"));

    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 555 }, css: 'a{}' }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(top.last()) + ' ' + sub.posted.length")), QStringLiteral(R"({"style":1,"add":"a{}"} 1)"));
    worker.evaluate(QStringLiteral("top.say({ styled: 1 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    worker.evaluate(QStringLiteral("chrome.scripting.removeCSS({ target: { tabId: 555, allFrames: true }, files: ['cs/s.css'] }, function(){ out = 'called ' + chrome.runtime.lastError; });"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([top.last(), sub.last()])")), QStringLiteral(R"([{"style":2,"remove":"S"},{"style":1,"remove":"S"}])"));
    worker.evaluate(QStringLiteral("sub.say({ styled: 1, error: 'boom' }); top.say({ styled: 2 });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("called undefined"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 555, frameIds: [5] }, css: 'b{}', origin: 'USER' }));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify(sub.last())")), QStringLiteral(R"({"style":2,"add":"b{}"})"));
    worker.evaluate(QStringLiteral("sub.say({ styled: 2, error: 'boom' });"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed boom"));
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS({ target: { tabId: 555, frameIds: [5] }, css: 'c{}' }));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("sub.die();"));
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("failed The frame was removed."));

    QStringList refused;
    foreach(const QString &call, QStringList()
            << "{ target: { tabId: 555 }, css: 'a{}', files: ['cs/s.css'] }"
            << "{ target: { tabId: 555 } }"
            << "{ target: { tabId: 555 }, css: 'a{}', origin: 'WEIRD' }"
            << "{ target: { tabId: 999 }, css: 'a{}' }"
            << "{ target: { tabId: 555 }, files: ['../x.css'] }"
            << "{ target: { tabId: 555 }, files: ['cs/nosuch.css'] }"
            << "{ target: { tabId: 555 }, css: 7 }"){
        worker.evaluate(QStringLiteral("outcome(chrome.scripting.insertCSS(%1));").arg(call));
        Settle(&worker);
        refused << Value(&worker, QStringLiteral("out"));
    }
    QCOMPARE(refused, QStringList()
             << "failed Exactly one of 'css' and 'files' must be specified."
             << "failed Exactly one of 'css' and 'files' must be specified."
             << "failed Invalid value for argument 1. Property 'origin'."
             << "failed No tab with id: 999."
             << "failed Invalid value for argument 1. Property 'files': expected a list of paths inside the extension."
             << "failed Could not load file: 'cs/nosuch.css'."
             << "failed Invalid value for argument 1. Property 'css': expected a string.");
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length")), QStringLiteral("3"));
}

void tst_cdpshims::aDocumentRunsARegisteredScriptOnceWhenItIsParsed(){
    QJSEngine page;
    Linked(&page, QStringLiteral("document.readyState = 'loading';"));
    page.evaluate(QStringLiteral("ports[0].say({ run: 7, codes: ['self.__r = (self.__r || 0) + 1', 'throw new Error(\"x\")', 'self.__s = 1'], at: 'document_idle', script: 'one' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + self.__r + ' ' + document.loaded.length")), QStringLiteral("{\"ran\":7} undefined 1"));
    page.evaluate(QStringLiteral("ports[0].say({ run: 8, codes: ['self.__r = 9'], at: 'document_start', script: 'one' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + self.__r")), QStringLiteral("{\"ran\":8} undefined"));
    page.evaluate(QStringLiteral("ports[0].say({ run: 9, codes: ['self.__t = 1'], at: 'document_start', script: 'two' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + self.__t")), QStringLiteral("{\"ran\":9} 1"));
    page.evaluate(QStringLiteral("document.onLoaded();"));
    QCOMPARE(Value(&page, QStringLiteral("self.__r + ' ' + self.__s")), QStringLiteral("1 1"));
    page.evaluate(QStringLiteral("ports[0].say({ run: 10, codes: ['2 + 2'] });"));
    QTRY_COMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"ran\":10,\"value\":4}"));
    page.evaluate(QStringLiteral("ports[0].say({ run: 11, codes: ['self.__u = 1'], at: 'document_end', script: 'three' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + self.__u")), QStringLiteral("{\"ran\":11} 1"));
    page.evaluate(QStringLiteral("runTimers(); document.loaded = []; document.readyState = 'loading';"
                                 "ports[0].say({ run: 12, codes: ['self.__v = (self.__v || 0) + 1'], at: 'document_idle', script: 'five' });"));
    QCOMPARE(Value(&page, QStringLiteral("self.__v + ' ' + document.loaded.length + ' ' + timers.filter(function(t){ return t.ms === 50; }).length")), QStringLiteral("undefined 1 1"));
    page.evaluate(QStringLiteral("runTimers();"));
    QCOMPARE(Value(&page, QStringLiteral("self.__v + ' ' + timers.filter(function(t){ return t.ms === 50; }).length")), QStringLiteral("undefined 1"));
    page.evaluate(QStringLiteral("document.readyState = 'complete'; runTimers(); document.onLoaded(); runTimers();"));
    QCOMPARE(Value(&page, QStringLiteral("self.__v + ' ' + timers.filter(function(t){ return t.ms === 50; }).length")), QStringLiteral("1 0"));
}

void tst_cdpshims::aStyleSheetOfTheWorkersIsPutInAndTakenOut(){
    QJSEngine page;
    Linked(&page, QStringLiteral(
        "function CSSStyleSheet(){ this.text = null; this.replaceSync = function(t){ this.text = t; }; }"
        "document.adoptedStyleSheets = [];"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 1, add: 'a{display:none}' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + document.adoptedStyleSheets.length + ' ' + document.adoptedStyleSheets[0].text")),
             QStringLiteral("{\"styled\":1} 1 a{display:none}"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 2, add: 'a{display:none}' }); ports[0].say({ style: 3, add: 'b{}' });"));
    QCOMPARE(Value(&page, QStringLiteral("document.adoptedStyleSheets.length")), QStringLiteral("3"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 4, remove: 'a{display:none}' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + document.adoptedStyleSheets.map(function(s){ return s.text; }).join()")),
             QStringLiteral("{\"styled\":4} a{display:none},b{}"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 5, remove: 'zzz' });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + document.adoptedStyleSheets.length")), QStringLiteral("{\"styled\":5} 2"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 6 });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last())")), QStringLiteral("{\"styled\":6,\"error\":\"no style sheet to put in or take out\"}"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 8, add: 'r{}', script: 'reg', nth: 0 }); ports[0].say({ style: 9, add: 'r{}', script: 'reg', nth: 0 });"
                                 "ports[0].say({ style: 10, add: 'r2{}', script: 'reg', nth: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].posted.slice(-3).map(function(m){ return m.styled + (m.error ? '!' : ''); }).join() + ' ' + document.adoptedStyleSheets.length")),
             QStringLiteral("8,9,10 4"));
    page.evaluate(QStringLiteral("var Sheet = CSSStyleSheet; CSSStyleSheet = null; ports[0].say({ style: 7, add: 'c{}' });"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].last().styled + ' ' + typeof ports[0].last().error + ' ' + document.adoptedStyleSheets.length")), QStringLiteral("7 string 4"));
    page.evaluate(QStringLiteral("ports[0].say({ style: 11, add: 'q{}', script: 'reg2', nth: 0 });"));
    QCOMPARE(Value(&page, QStringLiteral("typeof ports[0].last().error + ' ' + document.adoptedStyleSheets.length")), QStringLiteral("string 4"));
    page.evaluate(QStringLiteral("CSSStyleSheet = Sheet; ports[0].say({ style: 12, add: 'q{}', script: 'reg2', nth: 0 });"));
    QCOMPARE(Value(&page, QStringLiteral("JSON.stringify(ports[0].last()) + ' ' + document.adoptedStyleSheets.length")), QStringLiteral("{\"styled\":12} 5"));
}

void tst_cdpshims::aCarriersDocumentLinksWithNobodyListening(){
    QJSEngine page;
    Make(&page, true, Fetching() + Saying() + Navigating() + QStringLiteral("self.__vanillaCarrier = 1; replies.push({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + fetched.length + '/' + sent[0][0].link")), QStringLiteral("1/1/1"));
    QTRY_COMPARE(Value(&page, QStringLiteral("ports.length")), QStringLiteral("1"));
    QCOMPARE(Value(&page, QStringLiteral("sent[0][0].carrier + ' ' + ports[0].posted[0].link.carrier")), QStringLiteral("1 1"));
    page.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    QCOMPARE(Value(&page, QStringLiteral("timers.length + ' in ' + timers[0].ms")), QStringLiteral("1 in 20000"));
    QCOMPARE(Value(&page, QStringLiteral("Object.keys(navHeard).length")), QStringLiteral("0"));
    page.evaluate(QStringLiteral("var f = function(){ return false; }; chrome.runtime.onMessage.addListener(f);"));
    QCOMPARE(Value(&page, QStringLiteral("Object.keys(navHeard).length > 0")), QStringLiteral("true"));
    QCOMPARE(Value(&page, QStringLiteral("sent.length + '/' + ports.length + '/' + ports[0].disconnected")), QStringLiteral("1/1/0"));
    page.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(f);"));
    QCOMPARE(Value(&page, QStringLiteral("ports[0].disconnected + '/' + ports.length + '/' + timers.length")), QStringLiteral("0/1/1"));
    QJSEngine hidden;
    Make(&hidden, true, Fetching() + Saying() + QStringLiteral("self.__vanillaCarrier = 1; document.visibilityState = 'hidden';"));
    QCOMPARE(Value(&hidden, QStringLiteral("sent.length + '/' + ports.length")), QStringLiteral("0/1"));
    QJSEngine plain;
    Make(&plain, true, Fetching() + Saying());
    QCOMPARE(Value(&plain, QStringLiteral("sent.length + '/' + ports.length + '/' + timers.length")), QStringLiteral("0/0/0"));

    QJSEngine spare;
    Make(&spare, true, Fetching() + Saying() + QStringLiteral("self.__vanillaCarrier = 1; replies.push({ linked: 1, carrying: 0 });"));
    QCOMPARE(Value(&spare, QStringLiteral("sent.length + '/' + sent[0][0].carrier")), QStringLiteral("1/1"));
    Settle(&spare);
    QCOMPARE(Value(&spare, QStringLiteral("ports.length + '/' + timers.length")), QStringLiteral("0/0"));
    spare.evaluate(QStringLiteral("replies.push({ linked: 1 }); chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&spare, QStringLiteral("sent.length + '/' + (sent[1][0].carrier === undefined)")), QStringLiteral("2/true"));
    QTRY_COMPARE(Value(&spare, QStringLiteral("ports.length")), QStringLiteral("1"));
    QJSEngine dark;
    Make(&dark, true, Fetching() + Saying() + QStringLiteral("self.__vanillaCarrier = 1; document.visibilityState = 'hidden';"));
    QCOMPARE(Value(&dark, QStringLiteral("ports.length + ' ' + ports[0].posted[0].link.carrier")), QStringLiteral("1 1"));
    dark.evaluate(QStringLiteral("ports[0].say({ linked: 1, carrying: 0 });"));
    QCOMPARE(Value(&dark, QStringLiteral("ports[0].disconnected + '/' + timers.length")), QStringLiteral("1/0"));
    QCOMPARE(Value(&dark, QStringLiteral("String(ports[0].posted[0].link.fresh)")), QStringLiteral("1"));
    dark.evaluate(QStringLiteral("chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QTRY_COMPARE(Value(&dark, QStringLiteral("ports.length")), QStringLiteral("2"));
    QCOMPARE(Value(&dark, QStringLiteral("String(ports[1].posted[0].link.fresh)")), QStringLiteral("undefined"));
    QJSEngine held;
    Make(&held, true, Fetching() + Saying() + QStringLiteral("self.__vanillaCarrier = 1; replies.push({ linked: 1, carrying: 0 });"));
    held.evaluate(QStringLiteral("var g = function(){ return false; }; chrome.runtime.onMessage.addListener(g);"));
    QTRY_COMPARE(Value(&held, QStringLiteral("ports.length")), QStringLiteral("1"));
    held.evaluate(QStringLiteral("ports[0].say({ linked: 1 });"));
    QCOMPARE(Value(&held, QStringLiteral("sent[0][0].carrier + ' ' + (ports[0].posted[0].link.carrier === undefined) + ' ' + ports[0].disconnected")), QStringLiteral("1 true 0"));
    held.evaluate(QStringLiteral("chrome.runtime.onMessage.removeListener(g);"));
    QCOMPARE(Value(&held, QStringLiteral("ports[0].disconnected + '/' + timers.length")), QStringLiteral("1/0"));
    QJSEngine tired;
    Make(&tired, true, Fetching() + Saying() + QStringLiteral("self.__vanillaCarrier = 1;"));
    RunAll(&tired);
    QCOMPARE(Value(&tired, QStringLiteral("warned.length + ' ' + timers.length")), QStringLiteral("1 0"));
    const int greeted = Value(&tired, QStringLiteral("sent.length")).toInt();
    tired.evaluate(QStringLiteral("replies.push({ linked: 1 }); chrome.runtime.onMessage.addListener(function(){ return false; });"));
    QCOMPARE(Value(&tired, QStringLiteral("sent.length")).toInt(), greeted + 1);
    QTRY_COMPARE(Value(&tired, QStringLiteral("ports.length")), QStringLiteral("1"));
}

void tst_cdpshims::aWorkerTellsACarrierWhetherItIsWanted(){
    const QString greeting = QStringLiteral("{ __vanillaEnvelope: 1, from: { nonce: 'c1000000000000000000000000000000', frameId: 0, tabUrl: 'u' }, link: 1%1 }");
    const QString both = QStringLiteral("var answered = [];"
                                        "listeners[0](%1, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(JSON.stringify(v)); });"
                                        "listeners[0](%2, { id: chrome.runtime.id, url: 'u' }, function(v){ answered.push(JSON.stringify(v)); });")
                         .arg(greeting.arg(QStringLiteral(", carrier: 1")), greeting.arg(QString()));
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['<all_urls>'] }; };"));
        worker.evaluate(both);
        QCOMPARE(Value(&worker, QStringLiteral("answered.join(' ')")), QStringLiteral("{\"linked\":1,\"carrying\":1} {\"linked\":1}"));
        worker.evaluate(Linking());
        worker.evaluate(QStringLiteral("var named = naming({ link: { nonce: 'a1000000000000000000000000000000', frameId: 0, tabUrl: 'u', since: 1, carrier: 1 } }, 'https://a.example/x');"
                                       "var plain = naming({ link: { nonce: 'a2000000000000000000000000000000', frameId: 0, tabUrl: 'u', since: 1 } }, 'https://a.example/y');"));
        QCOMPARE(Value(&worker, QStringLiteral("JSON.stringify([named.posted[0], plain.posted[0]])")),
                 QStringLiteral("[{\"linked\":1,\"url\":\"https://a.example/x\",\"carrying\":1},{\"linked\":1,\"url\":\"https://a.example/y\"}]"));
    }
    {
        QJSEngine worker;
        MakeAsking(&worker, Fetching(), Saying());
        worker.evaluate(both);
        QCOMPARE(Value(&worker, QStringLiteral("answered.join(' ')")), QStringLiteral("{\"linked\":1,\"carrying\":0} {\"linked\":1}"));
    }
    {
        QJSEngine worker;
        MakeEdgeWorker(&worker, QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['<all_urls>'] }; };"));
        worker.evaluate(both);
        QCOMPARE(Value(&worker, QStringLiteral("answered.join(' ')")), QStringLiteral("{\"linked\":1,\"carrying\":0} {\"linked\":1}"));
    }
}

void tst_cdpshims::aRegistrationReachesOnlyWhereTheManifestMayInject(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['*://a.example/', 'bogus'] }; };"));
    worker.evaluate(Linking() + Serving());
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([{ id: 'wide', js: ['cs/a.js'], matches: ['<all_urls>'] }]));"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    worker.evaluate(QStringLiteral("var inside = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"
                                   "var outside = linking('a2000000000000000000000000000000', 0, 'https://b.example/top', 200);"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("inside.posted.length + '/' + outside.posted.length")), QStringLiteral("2/1"));
}

void tst_cdpshims::aStarvedDocumentIsGivenItsScriptsWhenRoomIsMade(){
    QJSEngine worker;
    MakeAsking(&worker, Fetching(), Saying() + QStringLiteral("chrome.runtime.getManifest = function(){ return { permissions: ['scripting'], host_permissions: ['<all_urls>'] }; };"));
    worker.evaluate(Linking() + Serving());
    worker.evaluate(QStringLiteral("texts['cs/big.js'] = 'x'.repeat(5 * 1024 * 1024);"
                                   "var top = linking('a1000000000000000000000000000000', 0, 'https://a.example/top', 100);"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("outcome(chrome.scripting.registerContentScripts([{ id: 'b1', js: ['cs/big.js'], matches: ['<all_urls>'] }, { id: 'b2', js: ['cs/big.js'], matches: ['<all_urls>'] }]));"));
    Settle(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("out")), QStringLiteral("resolved undefined"));
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + ' ' + top.posted[1].script")), QStringLiteral("2 b1"));
    worker.evaluate(QStringLiteral("top.say({ ran: top.posted[1].run });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runNextTimer();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + ' ' + top.posted[2].script")), QStringLiteral("3 b2"));
    worker.evaluate(QStringLiteral("texts['cs/small.css'] = 'S';"
                                   "outcome(chrome.scripting.registerContentScripts([{ id: 'b3', js: ['cs/big.js'], css: ['cs/small.css'], matches: ['<all_urls>'] }]));"));
    Settle(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("out + ' ' + top.posted.length")), QStringLiteral("resolved undefined 3"));
    worker.evaluate(QStringLiteral("top.say({ ran: top.posted[2].run });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runNextTimer();"));
    Settle(&worker);
    QCOMPARE(Value(&worker, QStringLiteral("top.posted.length + ' ' + top.posted[3].script + ':' + top.posted[3].nth + ' ' + top.posted[4].script + ':' + top.posted[4].at")),
             QStringLiteral("5 b3:0 b3:document_idle"));
    worker.evaluate(QStringLiteral("texts['cs/huge.js'] = 'x'.repeat(9 * 1024 * 1024);"
                                   "outcome(chrome.scripting.registerContentScripts([{ id: 'b4', js: ['cs/huge.js'], matches: ['<all_urls>'] }]));"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("top.say({ styled: top.posted[3].style }); top.say({ ran: top.posted[4].run });"));
    Settle(&worker);
    worker.evaluate(QStringLiteral("runNextTimer();"));
    Settle(&worker);
    QTRY_COMPARE(Value(&worker, QStringLiteral("out + ' ' + top.posted.length")), QStringLiteral("resolved undefined 5"));
}

void tst_cdpshims::theShimRunAsAWorkerWouldRunIt(){
    QJSEngine engine;
    engine.evaluate(QStringLiteral(
        "var self = this;\n"
        "var calledBack = 'not called';\n"
        "function setTimeout(f){ f(); }\n"
        "var chrome = { runtime: { id: 'x' }, cookies: { theEnginesOwn: 1 },\n"
        "               tabs: { TAB_ID_NONE: -1, update: function(){ return this === chrome.__realTabs ? 'native' : 'wrong this'; } } };\n"
        "chrome.__realTabs = chrome.tabs;\n"));

    const QJSValue made = engine.evaluate(Cdp::WorkerShim());
    QVERIFY2(!made.isError(), qPrintable(made.toString()));
    QVERIFY(made.toString().split(QLatin1Char(',')).contains(QStringLiteral("tabs")));
    QVERIFY(made.toString().split(QLatin1Char(',')).contains(QStringLiteral("contextMenus")));
    QVERIFY(!made.toString().split(QLatin1Char(',')).contains(QStringLiteral("cookies")));

    auto value = [&](const QString &expression){
        const QJSValue result = engine.evaluate(expression);
        return result.isError() ? QStringLiteral("ERROR ") + result.toString() : result.toString();
    };

    QCOMPARE(value(QStringLiteral("typeof chrome.tabs.onActivated.addListener")), QStringLiteral("function"));
    QCOMPARE(value(QStringLiteral("typeof chrome.webNavigation.onHistoryStateUpdated.addListener")), QStringLiteral("function"));
    QCOMPARE(value(QStringLiteral("'onClicked' in chrome.contextMenus")), QStringLiteral("true"));
    QCOMPARE(value(QStringLiteral("'onUpdated' in chrome.tabs")), QStringLiteral("true"));
    QCOMPARE(value(QStringLiteral("'create' in chrome.alarms")), QStringLiteral("true"));

    QCOMPARE(value(QStringLiteral("String(chrome.sessions.MAX_SESSION_RESULTS)")), QStringLiteral("25"));
    QCOMPARE(value(QStringLiteral("chrome.sessions.MAX_SESSION_RESULTS || 25")), QStringLiteral("25"));
    QCOMPARE(value(QStringLiteral("'MAX_SESSION_RESULTS' in chrome.sessions")), QStringLiteral("true"));
    QCOMPARE(value(QStringLiteral("String(chrome.sessions.SOME_OTHER_LIMIT)")), QStringLiteral("undefined"));
    QCOMPARE(value(QStringLiteral("typeof chrome.declarativeNetRequest.RuleActionType + ' ' + String(chrome.declarativeNetRequest.RuleActionType.BLOCK)")),
             QStringLiteral("object undefined"));

    QCOMPARE(value(QStringLiteral("chrome.tabs.TAB_ID_NONE")), QStringLiteral("-1"));
    QCOMPARE(value(QStringLiteral("String(chrome.tabs.update())")), QStringLiteral("[object Promise]"));
    QCOMPARE(value(QStringLiteral("chrome.cookies.theEnginesOwn + ' ' + typeof chrome.cookies.getAll")), QStringLiteral("1 undefined"));

    engine.evaluate(QStringLiteral("var outcome = 'pending';"
                                   "chrome.declarativeNetRequest.updateDynamicRules({})"
                                   "  .then(function(){ outcome = 'resolved'; }, function(e){ outcome = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(value(QStringLiteral("outcome")),
                 QStringLiteral("rejected: chrome.declarativeNetRequest.updateDynamicRules is not available in this browser"));
    engine.evaluate(QStringLiteral("chrome.tabs.query({}, function(){ calledBack = arguments.length + ' ' + (chrome.runtime.lastError ? chrome.runtime.lastError.message : 'NO ERROR'); });"));
    QCOMPARE(value(QStringLiteral("calledBack")), QStringLiteral("0 chrome.tabs.query is not available in this browser"));
    QCOMPARE(value(QStringLiteral("String(chrome.runtime.lastError) + ' ' + ('lastError' in chrome.runtime)")), QStringLiteral("undefined false"));
    engine.evaluate(QStringLiteral(
        "var viaPolyfill = 'pending';"
        "new Promise(function(resolve, reject){"
        "  chrome.alarms.get('x', function(){"
        "    if (chrome.runtime.lastError) reject(new Error(chrome.runtime.lastError.message));"
        "    else resolve(arguments[0]); });"
        "}).then(function(v){ viaPolyfill = 'resolved ' + v; }, function(e){ viaPolyfill = 'rejected: ' + e.message; });"));
    QTRY_COMPARE(value(QStringLiteral("viaPolyfill")), QStringLiteral("rejected: chrome.alarms.get is not available in this browser"));

    QCOMPARE(value(QStringLiteral("('then' in chrome.alarms) + ' ' + typeof chrome.alarms.then + ' ' + typeof chrome.tabs.then + ' ' + typeof chrome.alarms.toJSON")),
             QStringLiteral("false undefined undefined undefined"));
    QCOMPARE(value(QStringLiteral("['create', 'onAlarm', 'Kind', 'SOME_LIMIT', 'then', '_x', '0'].map(function(k){ return (k in chrome.alarms) === (chrome.alarms[k] !== undefined); }).join()")),
             QStringLiteral("true,true,true,true,true,true,true"));

    QCOMPARE(engine.evaluate(Cdp::WorkerShim()).toString(), QStringLiteral("already"));

    QJSEngine sealed;
    sealed.evaluate(QStringLiteral(
        "var self = this; var calledBack = 'not called';\n"
        "function setTimeout(f){ f(); }\n"
        "var chrome = { runtime: Object.freeze({ id: 'x' }) };\n"));
    QVERIFY(!sealed.evaluate(Cdp::WorkerShim()).isError());
    sealed.evaluate(QStringLiteral("chrome.alarms.get('x', function(){ calledBack = 'called'; });"));
    QCOMPARE(sealed.evaluate(QStringLiteral("calledBack")).toString(), QStringLiteral("not called"));
}

QTEST_MAIN(tst_cdpshims)
#include "tst_cdpshims.moc"
