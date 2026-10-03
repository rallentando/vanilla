#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QMutex>
#include <QSemaphore>
#include <QTimer>

#include "callback.hpp"
#include "saveflow.hpp"
#include "saver.hpp"
#include "shutdownflow.hpp"

#include <atomic>

class tst_shutdown : public QObject {
    Q_OBJECT

private:
    static QString Source(const QString &relative){
        QFile file(QStringLiteral(VANILLA_SOURCE_DIR) + QLatin1Char('/') + relative);
        if(!file.open(QIODevice::ReadOnly)) return QString();
        return QString::fromUtf8(file.readAll());
    }

private slots:
    void anEmptyBarrierFinishesWhenEnumerationIsSealed();
    void aBarrierWaitsForEveryAnswerInAnyOrder();
    void timeoutWinsOnceAndLateAnswersDoNothing();
    void shutdownFlowQueuesEveryAsynchronousBoundaryAndFinishesOnce();
    void aLateSaveForcesAnotherMediaCapture();
    void shutdownReturnsToQtBetweenMediaAnswerAndDestruction();
    void priorSaveHasBothEndingsAndASeparateFinalSave();
    void asynchronousSaveIsClaimedBeforeItsWorkerIsQueued();
    void coalescedSaveWritesTheLatestSnapshotAfterItsPredecessor_data();
    void coalescedSaveWritesTheLatestSnapshotAfterItsPredecessor();
    void aSnapshotFailureEndsTheChainWithoutStartingAWriter();
    void saveFlowKeepsOwnershipAcrossACoalescedRequest();
    void saveStagesContinueAfterARegularFailure();
    void everyMediaBackendSettlesImmediateAndAnsweredRequests();
    void exitIsIssuedOnceByWhicheverSideComesSecond();
    void anExitRequestedBeforeTheLoopIsCarriedOutOnItsFirstTurn();
    void anExitRequestedInsideTheLoopEndsIt();
    void anExitRequestedFromANestedLoopEndsEveryLoop();
    void runEntersTheLoopThroughTheNamedExec();
    void applicationHandsItsExitToTheFlow();
};

void tst_shutdown::anEmptyBarrierFinishesWhenEnumerationIsSealed(){
    int finished = 0;
    CompletionBarrier barrier([&](){ ++finished;});

    QCOMPARE(finished, 0);
    barrier.Seal();
    QCOMPARE(finished, 1);
    QVERIFY(barrier.IsFinished());

    barrier.Seal();
    barrier.Expire();
    QCOMPARE(finished, 1);
}

void tst_shutdown::aBarrierWaitsForEveryAnswerInAnyOrder(){
    int finished = 0;
    CompletionBarrier barrier([&](){ ++finished;});
    barrier.Add();
    barrier.Add();
    barrier.Add();

    barrier.Complete();
    barrier.Seal();
    barrier.Complete();
    QCOMPARE(finished, 0);

    barrier.Complete();
    QCOMPARE(finished, 1);
    QVERIFY(barrier.IsFinished());
}

void tst_shutdown::timeoutWinsOnceAndLateAnswersDoNothing(){
    int finished = 0;
    CompletionBarrier barrier([&](){ ++finished;});
    barrier.Add();
    barrier.Add();
    barrier.Seal();

    barrier.Expire();
    QCOMPARE(finished, 1);

    barrier.Complete();
    barrier.Complete();
    barrier.Expire();
    QCOMPARE(finished, 1);
}

void tst_shutdown::shutdownFlowQueuesEveryAsynchronousBoundaryAndFinishesOnce(){
    int queued = 0;
    ShutdownFlow flow([&](){ ++queued;});

    QCOMPARE(flow.Begin(), ShutdownFlow::Action::WaitForPriorSave);
    QCOMPARE(flow.Begin(), ShutdownFlow::Action::None);
    QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::WaitingForPriorSave);

    flow.PriorSaveSettled();
    flow.PriorSaveSettled();
    QCOMPARE(queued, 1);
    QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::ReadyForMedia);
    QCOMPARE(flow.Continue(), ShutdownFlow::Action::CaptureMedia);
    QCOMPARE(flow.Continue(), ShutdownFlow::Action::None);

    flow.MediaSettled();
    flow.MediaSettled();
    QCOMPARE(queued, 2);
    QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::ReadyForFinalSave);
    QCOMPARE(flow.Continue(), ShutdownFlow::Action::FinalSave);
    QCOMPARE(flow.Continue(), ShutdownFlow::Action::None);
    QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::FinalSave);
}

void tst_shutdown::aLateSaveForcesAnotherMediaCapture(){
    int queued = 0;
    ShutdownFlow flow([&](){ ++queued;});

    QCOMPARE(flow.Begin(), ShutdownFlow::Action::WaitForPriorSave);
    flow.PriorSaveSettled();
    QCOMPARE(flow.Continue(), ShutdownFlow::Action::CaptureMedia);
    flow.MediaSettled();

    QVERIFY(flow.WaitForAnotherSave());
    QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::WaitingForPriorSave);
    flow.PriorSaveSettled();
    QCOMPARE(queued, 3);
    QCOMPARE(flow.Continue(), ShutdownFlow::Action::CaptureMedia);
}

void tst_shutdown::shutdownReturnsToQtBetweenMediaAnswerAndDestruction(){
    const QString app = Source(QStringLiteral("app/application.cpp"));
    QVERIFY(!app.isEmpty());

    const int capture = app.indexOf(QStringLiteral("TreeBank::SaveMediaTimesForQuit"));
    const int settled = app.indexOf(QStringLiteral("m_ShutdownFlow.MediaSettled();"), capture);
    const int release = app.indexOf(QStringLiteral("TreeBank::ReleaseAllView();"), capture);
    QVERIFY(capture >= 0);
    QVERIFY(settled > capture);
    QVERIFY(release > settled);
    QVERIFY(app.mid(capture, release - capture)
            .contains(QStringLiteral("MediaSettled")));

    const int queueBegin = app.indexOf(QStringLiteral("void Application::QueueTakeDown"));
    const int waitBegin = app.indexOf(QStringLiteral("void Application::WaitForPriorSave"));
    QVERIFY(queueBegin >= 0 && waitBegin > queueBegin);
    const QString queueBody = app.mid(queueBegin, waitBegin - queueBegin);
    QVERIFY(queueBody.contains(QStringLiteral("QTimer::singleShot(0")));
    QVERIFY(!queueBody.contains(QStringLiteral("[](){ TakeDown();}();")));
}

void tst_shutdown::priorSaveHasBothEndingsAndASeparateFinalSave(){
    const QString app = Source(QStringLiteral("app/application.cpp"));
    QVERIFY(!app.isEmpty());

    const int wait = app.indexOf(QStringLiteral("void Application::WaitForPriorSave"));
    const int takeDown = app.indexOf(QStringLiteral("void Application::TakeDown"));
    QVERIFY(wait >= 0);
    QVERIFY(takeDown > wait);
    const QString waitBody = app.mid(wait, takeDown - wait);
    QVERIFY(waitBody.contains(QStringLiteral("&AutoSaver::Finished")));
    QVERIFY(waitBody.contains(QStringLiteral("&AutoSaver::Failed")));
    QVERIFY(waitBody.contains(QStringLiteral("m_ShutdownFlow.PriorSaveSettled()")));

    const int capture = app.indexOf(QStringLiteral("TreeBank::SaveMediaTimesForQuit"), takeDown);
    const int finalSave = app.indexOf(QStringLiteral("m_AutoSaver->SaveAll();"), capture);
    QVERIFY(capture >= 0);
    QVERIFY(finalSave > capture);
}

void tst_shutdown::asynchronousSaveIsClaimedBeforeItsWorkerIsQueued(){
    const QString saver = Source(QStringLiteral("app/saver.cpp"));
    QVERIFY(!saver.isEmpty());

    const int async = saver.indexOf(QStringLiteral("void AutoSaver::SaveAllAsync"));
    const int claim = saver.indexOf(QStringLiteral("BeginSave()"), async);
    const int start = saver.indexOf(QStringLiteral("StartAsyncWrite()"), claim);
    const int snapshot = saver.indexOf(QStringLiteral("m_Capture(&snapshot)"), start);
    const int dispatch = saver.indexOf(QStringLiteral("QThreadPool::globalInstance()->start"), snapshot);
    const int writer = saver.indexOf(QStringLiteral("bool AutoSaver::WriteProductionSnapshot"), dispatch);
    QVERIFY(async >= 0);
    QVERIFY(claim > async);
    QVERIFY(start > claim);
    QVERIFY(snapshot > start);
    QVERIFY(dispatch > snapshot);
    QVERIFY(writer > dispatch);

    const QString worker = saver.mid(dispatch, writer - dispatch);
    QVERIFY(!worker.contains(QStringLiteral("GetMainWindows")));
    QVERIFY(!worker.contains(QStringLiteral("GlobalSettings")));
    QVERIFY(!worker.contains(QStringLiteral("CookieSnapshot")));

    const QString app = Source(QStringLiteral("app/application.cpp"));
    QVERIFY(app.contains(QStringLiteral("m_AutoSaver->SaveAllAsync();")));
    QVERIFY(!app.contains(QStringLiteral("QThreadPool::globalInstance()->start")));

    const QString window = Source(QStringLiteral("ui/mainwindow.cpp"));
    const int close = window.indexOf(QStringLiteral("void MainWindow::closeEvent"));
    const int resize = window.indexOf(QStringLiteral("void MainWindow::resizeEvent"), close);
    const QString closeBody = window.mid(close, resize - close);
    QVERIFY(closeBody.contains(QStringLiteral("RemoveSettings();")));
    QVERIFY(closeBody.contains(QStringLiteral("GetAutoSaver()->SaveAllAsync();")));
    QVERIFY(!closeBody.contains(QStringLiteral("SaveSettingsFile")));

    const QString network = Source(QStringLiteral("app/networkcontroller.cpp"));
    const int clear = network.indexOf(QStringLiteral("void NetworkController::ClearCookies"));
    const int clearCache = network.indexOf(QStringLiteral("void NetworkController::ClearHttpCache"), clear);
    const QString clearBody = network.mid(clear, clearCache - clear);
    QVERIFY(clearBody.contains(QStringLiteral("saver->SaveAllAsync();")));
    QVERIFY(!clearBody.contains(QStringLiteral("SaveAllCookies();")));

    const QString tree = Source(QStringLiteral("ui/treebank.cpp"));
    const int saveTree = tree.indexOf(QStringLiteral("bool TreeBank::SaveTree"));
    const int media = tree.indexOf(QStringLiteral("void TreeBank::SaveMediaTimesForQuit"), saveTree);
    QVERIFY(saveTree >= 0);
    QVERIFY(media > saveTree);
    const QString saveTreeBody = tree.mid(saveTree, media - saveTree);
    QVERIFY(saveTreeBody.contains(QStringLiteral("TreeSaveHooks(windowIndices)")));
    QVERIFY(saveTreeBody.contains(QStringLiteral("primarySaved && secondarySaved")));
    QVERIFY(!saveTreeBody.contains(QStringLiteral("GetMainWindows")));
    QVERIFY(!saveTreeBody.contains(QStringLiteral("UpdateCurrentThumbnails")));

    const QString productionWriter = saver.mid(writer);
    QVERIFY(productionWriter.contains(QStringLiteral("TreeBank::SaveTree(snapshot.windowIndices)")));
    QVERIFY(productionWriter.contains(QStringLiteral("NetworkController::SaveCookieSnapshot(snapshot.cookies)")));
    QVERIFY(productionWriter.contains(QStringLiteral("return stages.Succeeded();")));
    QVERIFY(!productionWriter.contains(QStringLiteral("return false;")));
}

void tst_shutdown::coalescedSaveWritesTheLatestSnapshotAfterItsPredecessor_data(){
    QTest::addColumn<bool>("firstSucceeds");
    QTest::newRow("success-then-latest") << true;
    QTest::newRow("failure-still-writes-latest") << false;
}

void tst_shutdown::coalescedSaveWritesTheLatestSnapshotAfterItsPredecessor(){
    QFETCH(bool, firstSucceeds);

    int marker = 1;
    bool hasClosedWindow = true;
    QSemaphore firstWriterEntered;
    QSemaphore releaseFirstWriter;
    QMutex writtenLock;
    QList<Settings> written;
    std::atomic_int activeWriters{0};
    std::atomic_int maximumWriters{0};
    std::atomic_int writerCount{0};

    AutoSaver saver(
        [&](SaveSnapshot *snapshot){
            snapshot->settings.setValue(QStringLiteral("marker"), marker);
            if(hasClosedWindow)
                snapshot->settings.setValue(QStringLiteral("mainwindow/window"), true);
            return true;
        },
        [&](const SaveSnapshot &snapshot){
            const int active = ++activeWriters;
            int maximum = maximumWriters.load();
            while(active > maximum && !maximumWriters.compare_exchange_weak(maximum, active)) {}
            const int count = ++writerCount;
            {
                QMutexLocker locker(&writtenLock);
                written.append(snapshot.settings);
            }
            if(count == 1){
                firstWriterEntered.release();
                releaseFirstWriter.acquire();
            }
            --activeWriters;
            return count != 1 || firstSucceeds;
        });

    QSignalSpy started(&saver, &AutoSaver::Started);
    QSignalSpy finished(&saver, &AutoSaver::Finished);
    QSignalSpy failed(&saver, &AutoSaver::Failed);

    saver.SaveAllAsync();
    const bool firstEntered = firstWriterEntered.tryAcquire(1, 3000);
    const bool startedOnce = started.count() == 1;
    const bool savingFirst = saver.IsSaving();

    marker = 2;
    hasClosedWindow = false;
    saver.SaveAllAsync();
    const bool savingPending = saver.IsSaving();
    releaseFirstWriter.release();

    QVERIFY(firstEntered);
    QVERIFY(startedOnce);
    QVERIFY(savingFirst);
    QVERIFY(savingPending);

    QTRY_COMPARE_WITH_TIMEOUT(writerCount.load(), 2, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!saver.IsSaving(), 3000);
    QCOMPARE(maximumWriters.load(), 1);
    QCOMPARE(started.count(), 1);
    QCOMPARE(finished.count(), firstSucceeds ? 1 : 0);
    QCOMPARE(failed.count(), firstSucceeds ? 0 : 1);

    QMutexLocker locker(&writtenLock);
    QCOMPARE(written.size(), 2);
    QCOMPARE(written[0].value(QStringLiteral("marker")).toInt(), 1);
    QVERIFY(written[0].contains(QStringLiteral("mainwindow/window")));
    QCOMPARE(written[1].value(QStringLiteral("marker")).toInt(), 2);
    QVERIFY(!written[1].contains(QStringLiteral("mainwindow/window")));
}

void tst_shutdown::aSnapshotFailureEndsTheChainWithoutStartingAWriter(){
    int captures = 0;
    int writes = 0;
    AutoSaver saver(
        [&](SaveSnapshot*){ ++captures; return false; },
        [&](const SaveSnapshot&){ ++writes; return true; });
    QSignalSpy failed(&saver, &AutoSaver::Failed);

    saver.SaveAllAsync();
    QCOMPARE(captures, 1);
    QCOMPARE(writes, 0);
    QCOMPARE(failed.count(), 1);
    QVERIFY(!saver.IsSaving());
}

void tst_shutdown::saveFlowKeepsOwnershipAcrossACoalescedRequest(){
    SaveFlow flow;
    QVERIFY(flow.Request());
    QVERIFY(!flow.Request());
    QVERIFY(flow.IsSaving());
    QCOMPARE(flow.Settle(false), SaveFlow::SettleAction::StartPending);
    QVERIFY(flow.IsSaving());
    QCOMPARE(flow.Settle(true), SaveFlow::SettleAction::Failed);
    QVERIFY(!flow.IsSaving());
}

void tst_shutdown::saveStagesContinueAfterARegularFailure(){
    SaveStageFlow stages;
    QStringList order;
    stages.Run([&](){ order.append(QStringLiteral("config")); return false;});
    stages.Run([&](){ order.append(QStringLiteral("tree")); return true;});
    stages.Run([&](){ order.append(QStringLiteral("icon")); return true;});
    stages.Run([&](){ order.append(QStringLiteral("cookie")); return true;});

    QCOMPARE(order, QStringList({QStringLiteral("config"), QStringLiteral("tree"),
                                 QStringLiteral("icon"), QStringLiteral("cookie")}));
    QVERIFY(!stages.Succeeded());
}

void tst_shutdown::everyMediaBackendSettlesImmediateAndAnsweredRequests(){
    const QStringList files = {
        QStringLiteral("view/webengine/webengineview.cpp"),
        QStringLiteral("view/webengine/quickwebengineview.cpp"),
        QStringLiteral("view/quicknativewebview.cpp"),
        QStringLiteral("view/edge/edgewebviewpage.cpp")
    };

    for(const QString &name : files){
        const QString source = Source(name);
        QVERIFY2(!source.isEmpty(), qPrintable(name));
        const int begin = source.indexOf(QStringLiteral("::SaveMediaTime(VoidCallBack settled)"));
        const int restore = source.indexOf(QStringLiteral("::RestoreMediaTime"), begin);
        QVERIFY2(begin >= 0 && restore > begin, qPrintable(name));
        const QString body = source.mid(begin, restore - begin);
        QVERIFY2(body.count(QStringLiteral("if(settled) settled();")) >= 2,
                 qPrintable(name));
        QVERIFY2(body.contains(QStringLiteral("settled](")), qPrintable(name));
    }

    const QString edge = Source(QStringLiteral("view/edge/edgewebviewpage.cpp"));
    const int call = edge.indexOf(QStringLiteral("void EdgeWebView::CallWithScriptResult"));
    const int scale = edge.indexOf(QStringLiteral("qreal EdgeWebView::PageScale"), call);
    QVERIFY(call >= 0 && scale > call);
    const QString callBody = edge.mid(call, scale - call);
    QVERIFY(callBody.contains(QStringLiteral("if(FAILED(started) && discarded) discarded();")));
    QVERIFY(callBody.count(QStringLiteral("if(discarded) discarded();")) >= 3);
}

void tst_shutdown::exitIsIssuedOnceByWhicheverSideComesSecond(){
    using Action = ShutdownFlow::Action;
    {
        ShutdownFlow flow(nullptr);
        QCOMPARE(flow.RequestExit(), Action::None);
        QCOMPARE(flow.RequestExit(), Action::None);
        QCOMPARE(flow.EnterMainLoop(), Action::Exit);
        QCOMPARE(flow.EnterMainLoop(), Action::None);
        QCOMPARE(flow.RequestExit(), Action::None);
    }
    {
        ShutdownFlow flow(nullptr);
        QCOMPARE(flow.EnterMainLoop(), Action::None);
        QCOMPARE(flow.EnterMainLoop(), Action::None);
        QCOMPARE(flow.RequestExit(), Action::Exit);
        QCOMPARE(flow.RequestExit(), Action::None);
        QCOMPARE(flow.EnterMainLoop(), Action::None);
    }
    {
        ShutdownFlow flow(nullptr);
        QCOMPARE(flow.Begin(), Action::WaitForPriorSave);
        QCOMPARE(flow.RequestExit(), Action::None);
        QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::WaitingForPriorSave);
        QCOMPARE(flow.EnterMainLoop(), Action::Exit);
        QCOMPARE(flow.CurrentStage(), ShutdownFlow::Stage::WaitingForPriorSave);
    }
}

namespace {
struct LoopWatch {
    int aboutToQuit = 0;
    bool guardFired = false;
    QMetaObject::Connection connection;
    QTimer guard;
    LoopWatch(){
        connection = QObject::connect(qApp, &QCoreApplication::aboutToQuit,
                                      [this](){ ++aboutToQuit;});
        guard.setSingleShot(true);
        QObject::connect(&guard, &QTimer::timeout, [this](){
            guardFired = true;
            qApp->exit(1);
        });
        guard.start(3000);
    }
    ~LoopWatch(){
        guard.stop();
        QObject::disconnect(connection);
    }
};
}

void tst_shutdown::anExitRequestedBeforeTheLoopIsCarriedOutOnItsFirstTurn(){
    ShutdownFlow flow(nullptr);
    LoopWatch watch;
    int requestsFromAboutToQuit = 0;
    const auto reentry = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&](){
        if(++requestsFromAboutToQuit == 1) ExitHandoff::RequestExit(qApp, flow);
    });

    ExitHandoff::RequestExit(qApp, flow);
    const int code = ExitHandoff::Run<QApplication>(qApp, flow);
    QObject::disconnect(reentry);

    QVERIFY2(!watch.guardFired, "the exit requested before the loop was lost");
    QCOMPARE(code, 0);
    QCOMPARE(watch.aboutToQuit, 1);
    QCOMPARE(requestsFromAboutToQuit, 1);
}

void tst_shutdown::anExitRequestedInsideTheLoopEndsIt(){
    ShutdownFlow flow(nullptr);
    LoopWatch watch;
    bool requestedInsideTheLoop = false;
    QTimer::singleShot(0, qApp, [&](){
        requestedInsideTheLoop = true;
        ExitHandoff::RequestExit(qApp, flow);
    });

    const int code = ExitHandoff::Run<QApplication>(qApp, flow);

    QVERIFY(requestedInsideTheLoop);
    QVERIFY2(!watch.guardFired, "the exit requested inside the loop was lost");
    QCOMPARE(code, 0);
    QCOMPARE(watch.aboutToQuit, 1);
}

void tst_shutdown::anExitRequestedFromANestedLoopEndsEveryLoop(){
    ShutdownFlow flow(nullptr);
    LoopWatch watch;
    bool nestedReturned = false;
    int nestedCode = -1;
    QTimer::singleShot(0, qApp, [&](){
        QEventLoop nested;
        QTimer::singleShot(0, qApp, [&](){ ExitHandoff::RequestExit(qApp, flow);});
        nestedCode = nested.exec();
        nestedReturned = true;
    });

    const int code = ExitHandoff::Run<QApplication>(qApp, flow);

    QVERIFY2(nestedReturned, "the nested loop did not end");
    QVERIFY2(!watch.guardFired, "the exit requested from the nested loop was lost");
    QCOMPARE(nestedCode, 0);
    QCOMPARE(code, 0);
    QCOMPARE(watch.aboutToQuit, 1);
}

namespace {
struct NamedExec {
    static int calls;
    static int exec(){ ++calls; return QApplication::exec();}
};
int NamedExec::calls = 0;
}

void tst_shutdown::runEntersTheLoopThroughTheNamedExec(){
    ShutdownFlow flow(nullptr);
    LoopWatch watch;
    QTimer::singleShot(0, qApp, [&](){ ExitHandoff::RequestExit(qApp, flow);});

    NamedExec::calls = 0;
    const int code = ExitHandoff::Run<NamedExec>(qApp, flow);

    QCOMPARE(NamedExec::calls, 1);
    QVERIFY(!watch.guardFired);
    QCOMPARE(code, 0);
}

void tst_shutdown::applicationHandsItsExitToTheFlow(){
    const QString main = Source(QStringLiteral("app/main.cpp"));
    QVERIFY(!main.isEmpty());
    QVERIFY(main.contains(QStringLiteral("return a.Run();")));
    QVERIFY(!main.contains(QStringLiteral("a.exec()")));

    const QString app = Source(QStringLiteral("app/application.cpp"));
    QVERIFY(!app.isEmpty());
    const int run = app.indexOf(QStringLiteral("int Application::Run()"));
    const int request = app.indexOf(QStringLiteral("void Application::RequestExit()"));
    QVERIFY(run >= 0 && request > run);
    QVERIFY(app.mid(run, request - run)
            .contains(QStringLiteral("return ExitHandoff::Run<Application>(this, m_ShutdownFlow);")));
    const int requestEnd = app.indexOf(QLatin1Char('}'), request);
    QVERIFY(app.mid(request, requestEnd - request)
            .contains(QStringLiteral("ExitHandoff::RequestExit(GetInstance(), m_ShutdownFlow);")));

    const int takeDown = app.indexOf(QStringLiteral("void Application::TakeDown"));
    const int finish = app.indexOf(QStringLiteral("const auto finish = [](){"), takeDown);
    const int wired = app.indexOf(QStringLiteral("&AutoSaver::Finished, GetInstance(), finish"), finish);
    QVERIFY(takeDown >= 0 && finish > takeDown && wired > finish);
    const QString finishBody = app.mid(finish, wired - finish);
    QVERIFY(finishBody.contains(QStringLiteral("RequestExit();")));
    QVERIFY(!finishBody.contains(QStringLiteral("&Application::quit")));

    QVERIFY(!app.contains(QStringLiteral("QUIT_AFTER_DIALOG_DELAY")));
    QVERIFY(!Source(QStringLiteral("core/const.hpp")).contains(QStringLiteral("QUIT_AFTER_DIALOG_DELAY")));
}

QTEST_MAIN(tst_shutdown)
#include "tst_shutdown.moc"
