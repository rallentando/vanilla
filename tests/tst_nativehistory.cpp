#include <QtTest>
#include <QLibraryInfo>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>

#include "nativehistory.hpp"
#include "loadending.hpp"

class tst_nativehistory : public QObject {
    Q_OBJECT

private slots:
    void anUnusableUrlIsNotAnEntry();
    void theFirstUrlStartsTheList();
    void theUrlAlreadyShownIsNotAnEntry();
    void aLinkBackToTheEntryBeforeIsANewEntryNotAStep();
    void aLinkOnToTheEntryAfterIsANewEntryNotAStep();
    void anEntryPastTheEndDropsTheOldestOne();
    void aNewEntryDropsWhatWasAhead();

    void anAskedForStepDoesNotMoveTheIndexUntilItArrives();
    void aStepWhichLandsSomewhereElseBecomesANewEntry();
    void aStepWhichNeverArrivesLeavesTheIndexAlone();
    void everyStepIsALoadOfItsOwn();
    void aSecondMoveIsRefusedWhileOneIsPending();
    void aSecondMoveIsRefusedUntilTheLoadTheFirstBeganHasEnded();
    void aMoveEndsWhenSomethingElseIsLoadedInstead();
    void aLoadNobodyAskedForStillHoldsTheNextStepOff();
    void aUrlNobodyAskedForIsALoadBeginningNotAnEnding();
    void onlyAUrlWhichWouldLoadCountsAsALoad();
    void aStepWaitsForItsUrlWhateverTheBackendReportsFirst();
    void thereIsNothingToDoAtEitherEnd();
    void rewindGoesToTheOldestEntryInOneLoad();
    void aDestinationAlreadyOnScreenMovesWithoutLoading();
    void aDestinationAlreadyOnScreenLeavesNothingInFlight();
    void aPlainVisitDuringAMoveEndsTheMove();

    void whatWasWrittenIsWhatIsRead();
    void anEmptyListIsWorthNoFile();
    void theEnginesSerializedHistoryIsRefused();
    void ourFirstFourBytesAreOutsideTheEnginesVersions();
    void damagedFilesAreRefusedAndChangeNothing();
    void aFileWhoseCountAndIndexDisagreeIsRefused();
    void aFileWithAnUnusableOrRepeatedUrlIsRefused();
    void onlyAFileWhichReadsWholeCountsAsOurs();

    void theQmlSpellsTheBackendsLoadStatusesTheWayTheBackendDoes();

    void aLoadOwesExactlyOneRelease();
    void anEarlyReleaseLeavesNothingForTheEnding();
    void anEndingLeavesNothingForAStopAfterIt();
    void aSecondLoadOwesItsOwnRelease();
    void aReleaseWithNoLoadRunningIsNotOwed();
    void aForgottenTailOwesNothing();
    void everyReleaseInTheViewGoesThroughTheTail();
    void aStepWaitingBeforeAnyLoadStartedIsStillReleased();

private:
    static QUrl U(const char *path);
    static NativeHistory ListOf(int count, int index);
};

QUrl tst_nativehistory::U(const char *path){
    return QUrl(QStringLiteral("http://example.com/") + QLatin1String(path));
}

NativeHistory tst_nativehistory::ListOf(int count, int index){
    NativeHistory history;
    for(int i = 0; i < count; i++){
        history.Visit(QUrl(QStringLiteral("http://example.com/%1").arg(i)));
        history.Release();
    }
    while(history.Index() > index){
        NativeHistory::Request request = history.RequestBack(false);
        history.Visit(request.url);
        history.Release();
    }
    return history;
}

void tst_nativehistory::anUnusableUrlIsNotAnEntry(){
    NativeHistory history;
    history.Visit(QUrl());
    history.Visit(QUrl(QStringLiteral("")));
    QCOMPARE(history.Count(), 0);
    QCOMPARE(history.Index(), -1);
    QVERIFY(history.IsEmpty());
}

void tst_nativehistory::theFirstUrlStartsTheList(){
    NativeHistory history;
    history.Visit(U("a"));
    QCOMPARE(history.Count(), 1);
    QCOMPARE(history.Index(), 0);
    QCOMPARE(history.CurrentUrl(), U("a"));
    QVERIFY(!history.CanGoBack());
    QVERIFY(!history.CanGoForward());
}

void tst_nativehistory::theUrlAlreadyShownIsNotAnEntry(){
    NativeHistory history;
    history.Visit(U("a"));
    history.Visit(U("a"));
    history.Visit(U("a"));
    QCOMPARE(history.Count(), 1);
    QCOMPARE(history.Index(), 0);
}

void tst_nativehistory::aLinkBackToTheEntryBeforeIsANewEntryNotAStep(){
    NativeHistory history;
    history.Visit(U("a"));
    history.Visit(U("b"));
    history.Visit(U("a"));
    QCOMPARE(history.Count(), 3);
    QCOMPARE(history.Index(), 2);
    QCOMPARE(history.UrlAt(0), U("a"));
    QCOMPARE(history.UrlAt(1), U("b"));
}

void tst_nativehistory::aLinkOnToTheEntryAfterIsANewEntryNotAStep(){
    NativeHistory history = ListOf(3, 0);
    QCOMPARE(history.Index(), 0);
    history.Visit(QUrl(QStringLiteral("http://example.com/1")));
    QCOMPARE(history.Count(), 2);
    QCOMPARE(history.Index(), 1);
    QVERIFY(!history.CanGoForward());
}

void tst_nativehistory::anEntryPastTheEndDropsTheOldestOne(){
    NativeHistory history = ListOf(NativeHistory::MaxEntries(), NativeHistory::MaxEntries() - 1);
    QCOMPARE(history.Count(), NativeHistory::MaxEntries());
    const QUrl second = history.UrlAt(1);
    history.Visit(U("one more"));
    QCOMPARE(history.Count(), NativeHistory::MaxEntries());
    QCOMPARE(history.Index(), NativeHistory::MaxEntries() - 1);
    QCOMPARE(history.UrlAt(0), second);
    QCOMPARE(history.CurrentUrl(), U("one more"));
}

void tst_nativehistory::aNewEntryDropsWhatWasAhead(){
    NativeHistory history = ListOf(4, 1);
    QVERIFY(history.CanGoForward());
    history.Visit(U("elsewhere"));
    QCOMPARE(history.Count(), 3);
    QCOMPARE(history.Index(), 2);
    QVERIFY(!history.CanGoForward());
}

void tst_nativehistory::anAskedForStepDoesNotMoveTheIndexUntilItArrives(){
    NativeHistory history = ListOf(3, 2);
    NativeHistory::Request request = history.RequestBack(false);
    QCOMPARE(request.kind, NativeHistory::LoadMove);
    QCOMPARE(request.target, 1);
    QCOMPARE(history.Index(), 2);
    QCOMPARE(history.Pending(), NativeHistory::LoadMove);

    history.Visit(request.url);
    QCOMPARE(history.Index(), 1);
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QCOMPARE(history.Count(), 3);
}

void tst_nativehistory::aStepWhichLandsSomewhereElseBecomesANewEntry(){
    NativeHistory history = ListOf(3, 2);
    history.RequestBack(false);
    history.Visit(U("somewhere else"));
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QCOMPARE(history.Count(), 4);
    QCOMPARE(history.Index(), 3);
    QCOMPARE(history.CurrentUrl(), U("somewhere else"));
}

void tst_nativehistory::aStepWhichNeverArrivesLeavesTheIndexAlone(){
    NativeHistory history = ListOf(3, 2);
    history.RequestBack(false);
    history.Release();
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QCOMPARE(history.Index(), 2);
    QCOMPARE(history.Count(), 3);
    QCOMPARE(history.RequestBack(false).kind, NativeHistory::LoadMove);
}

void tst_nativehistory::everyStepIsALoadOfItsOwn(){
    NativeHistory history = ListOf(4, 3);
    for(int step = 0; step < 3; step++){
        NativeHistory::Request request = history.RequestBack(false);
        QCOMPARE(request.kind, NativeHistory::LoadMove);
        QCOMPARE(request.url, history.UrlAt(request.target));
        history.Visit(request.url);
        history.Release();
    }
    QCOMPARE(history.Index(), 0);
    QCOMPARE(history.RequestForward(false).kind, NativeHistory::LoadMove);
}

void tst_nativehistory::aSecondMoveIsRefusedWhileOneIsPending(){
    NativeHistory history = ListOf(4, 3);
    NativeHistory::Request first = history.RequestBack(false);
    QCOMPARE(first.kind, NativeHistory::LoadMove);

    QCOMPARE(history.RequestBack(false).kind, NativeHistory::NoMove);
    QCOMPARE(history.RequestForward(false).kind, NativeHistory::NoMove);
    QCOMPARE(history.RequestRewind(false).kind, NativeHistory::NoMove);
    QCOMPARE(history.RequestFastForward(false).kind, NativeHistory::NoMove);
    QCOMPARE(history.Pending(), NativeHistory::LoadMove);
    QCOMPARE(history.PendingTarget(), first.target);
    QCOMPARE(history.Index(), 3);
}

void tst_nativehistory::aSecondMoveIsRefusedUntilTheLoadTheFirstBeganHasEnded(){
    NativeHistory history = ListOf(4, 3);
    NativeHistory::Request first = history.RequestBack(false);
    history.Visit(first.url);
    QCOMPARE(history.Index(), 2);
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QVERIFY(history.Busy(true));
    QCOMPARE(history.RequestBack(true).kind, NativeHistory::NoMove);
    QCOMPARE(history.Index(), 2);

    history.Release();
    QVERIFY(!history.Busy(false));
    NativeHistory::Request second = history.RequestBack(false);
    QCOMPARE(second.kind, NativeHistory::LoadMove);
    history.Visit(second.url);
    QCOMPARE(history.Index(), 1);
}

void tst_nativehistory::aMoveEndsWhenSomethingElseIsLoadedInstead(){
    NativeHistory history = ListOf(3, 2);
    NativeHistory::Request request = history.RequestBack(false);
    history.Visit(request.url);
    QVERIFY(history.Busy(true));

    history.Visit(U("elsewhere"));
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QCOMPARE(history.CurrentUrl(), U("elsewhere"));
    QVERIFY(history.Busy(true));
    QCOMPARE(history.RequestBack(true).kind, NativeHistory::NoMove);

    history.Release();
    QCOMPARE(history.RequestBack(false).kind, NativeHistory::LoadMove);
}

void tst_nativehistory::aLoadNobodyAskedForStillHoldsTheNextStepOff(){
    NativeHistory history = ListOf(3, 2);
    QVERIFY(!history.Busy(false));

    QVERIFY(history.Busy(true));
    QCOMPARE(history.RequestBack(true).kind, NativeHistory::NoMove);
    QCOMPARE(history.Index(), 2);
    QCOMPARE(history.RequestBack(false).kind, NativeHistory::LoadMove);
}

void tst_nativehistory::aUrlNobodyAskedForIsALoadBeginningNotAnEnding(){
    NativeHistory history = ListOf(3, 2);
    history.Visit(U("a link nobody asked for"));
    QCOMPARE(history.Count(), 4);
    QCOMPARE(history.Index(), 3);
    QCOMPARE(history.RequestBack(true).kind, NativeHistory::NoMove);
    QCOMPARE(history.RequestBack(false).kind, NativeHistory::LoadMove);
}

void tst_nativehistory::onlyAUrlWhichWouldLoadCountsAsALoad(){
    const QUrl here = U("here");
    QVERIFY(NativeHistory::StartsALoad(U("elsewhere"), here));
    QVERIFY(!NativeHistory::StartsALoad(here, here));
    QVERIFY(!NativeHistory::StartsALoad(QUrl(), here));
    QVERIFY(!NativeHistory::StartsALoad(QUrl(QStringLiteral("")), here));
}

void tst_nativehistory::aStepWaitsForItsUrlWhateverTheBackendReportsFirst(){
    NativeHistory history = ListOf(3, 2);
    NativeHistory::Request request = history.RequestBack(false);
    QCOMPARE(request.kind, NativeHistory::LoadMove);

    QCOMPARE(history.Pending(), NativeHistory::LoadMove);
    QVERIFY(history.Busy(true));

    history.Visit(request.url);
    QCOMPARE(history.Index(), 1);
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QCOMPARE(history.RequestBack(true).kind, NativeHistory::NoMove);
}

void tst_nativehistory::aDestinationAlreadyOnScreenLeavesNothingInFlight(){
    NativeHistory history;
    history.Visit(U("a"));
    history.Visit(U("b"));
    history.Visit(U("a"));
    history.Release();
    QCOMPARE(history.RequestRewind(false).kind, NativeHistory::ImmediateMove);
    QVERIFY(!history.Busy(false));
    QCOMPARE(history.RequestFastForward(false).kind, NativeHistory::ImmediateMove);
    QCOMPARE(history.Index(), 2);
}

void tst_nativehistory::thereIsNothingToDoAtEitherEnd(){
    NativeHistory oldest = ListOf(3, 0);
    QCOMPARE(oldest.RequestBack(false).kind, NativeHistory::NoMove);
    QCOMPARE(oldest.RequestRewind(false).kind, NativeHistory::NoMove);

    NativeHistory newest = ListOf(3, 2);
    QCOMPARE(newest.RequestForward(false).kind, NativeHistory::NoMove);
    QCOMPARE(newest.RequestFastForward(false).kind, NativeHistory::NoMove);

    NativeHistory empty;
    QCOMPARE(empty.RequestBack(false).kind, NativeHistory::NoMove);
    QCOMPARE(empty.RequestForward(false).kind, NativeHistory::NoMove);
    QCOMPARE(empty.Index(), -1);
}

void tst_nativehistory::rewindGoesToTheOldestEntryInOneLoad(){
    NativeHistory history = ListOf(5, 4);
    NativeHistory::Request request = history.RequestRewind(false);
    QCOMPARE(request.kind, NativeHistory::LoadMove);
    QCOMPARE(request.target, 0);
    QCOMPARE(request.url, history.UrlAt(0));
    history.Visit(request.url);
    QCOMPARE(history.Index(), 0);
    QCOMPARE(history.Count(), 5);
    history.Release();

    NativeHistory::Request forward = history.RequestFastForward(false);
    QCOMPARE(forward.target, 4);
    history.Visit(forward.url);
    QCOMPARE(history.Index(), 4);
}

void tst_nativehistory::aDestinationAlreadyOnScreenMovesWithoutLoading(){
    NativeHistory history;
    history.Visit(U("a"));
    history.Visit(U("b"));
    history.Visit(U("a"));
    history.Release();
    QCOMPARE(history.Index(), 2);

    NativeHistory::Request request = history.RequestRewind(false);
    QCOMPARE(request.kind, NativeHistory::ImmediateMove);
    QCOMPARE(history.Index(), 0);
    QCOMPARE(history.Pending(), NativeHistory::NoMove);
    QCOMPARE(history.Count(), 3);
}

void tst_nativehistory::aPlainVisitDuringAMoveEndsTheMove(){
    NativeHistory history = ListOf(3, 2);
    history.RequestBack(false);
    history.Visit(history.CurrentUrl());
    QCOMPARE(history.Pending(), NativeHistory::LoadMove);
    QCOMPARE(history.Index(), 2);
    history.Visit(QUrl());
    QCOMPARE(history.Pending(), NativeHistory::LoadMove);
}

void tst_nativehistory::whatWasWrittenIsWhatIsRead(){
    NativeHistory history = ListOf(4, 1);
    NativeHistory read;
    QVERIFY(NativeHistory::Deserialize(history.Serialize(), &read));
    QCOMPARE(read.Entries(), history.Entries());
    QCOMPARE(read.Index(), history.Index());
    QCOMPARE(read.Pending(), NativeHistory::NoMove);
}

void tst_nativehistory::anEmptyListIsWorthNoFile(){
    NativeHistory empty;
    QVERIFY(empty.Serialize().isEmpty());
    NativeHistory read;
    QVERIFY(!NativeHistory::Deserialize(QByteArray(), &read));
}

void tst_nativehistory::theEnginesSerializedHistoryIsRefused(){
    for(qint32 version = 1; version <= 5; version++){
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_0);
        stream << version << static_cast<qint32>(1) << static_cast<qint32>(0)
               << QUrl(QStringLiteral("http://example.com/"));
        NativeHistory read;
        QVERIFY2(!NativeHistory::Deserialize(data, &read),
                 qPrintable(QStringLiteral("engine history version %1").arg(version)));
        QVERIFY(read.IsEmpty());
    }
}

void tst_nativehistory::ourFirstFourBytesAreOutsideTheEnginesVersions(){
    QVERIFY(NativeHistory::Marker() < 3 || NativeHistory::Marker() > 4);
    QCOMPARE(NativeHistory::Marker(), 0x564E5648);

    const QByteArray data = ListOf(2, 1).Serialize();
    QVERIFY(data.size() > 4);
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);
    qint32 first = 0;
    stream >> first;
    QCOMPARE(first, NativeHistory::Marker());
}

void tst_nativehistory::damagedFilesAreRefusedAndChangeNothing(){
    const QByteArray whole = ListOf(3, 1).Serialize();

    NativeHistory read;
    for(int length = 1; length < whole.size(); length++)
        QVERIFY2(!NativeHistory::Deserialize(whole.left(length), &read),
                 qPrintable(QStringLiteral("cut to %1 bytes").arg(length)));
    QVERIFY(!NativeHistory::Deserialize(whole + QByteArray("more"), &read));
    QVERIFY(read.IsEmpty());
    QCOMPARE(read.Index(), -1);

    QVERIFY(NativeHistory::Deserialize(whole, &read));
}

void tst_nativehistory::aFileWhoseCountAndIndexDisagreeIsRefused(){
    struct Case { qint32 version; qint32 index; qint32 count; };
    const QList<Case> cases = QList<Case>()
        << Case{NativeHistory::Version() + 1, 0, 1}
        << Case{NativeHistory::Version(), 0, 0}
        << Case{NativeHistory::Version(), -1, 1}
        << Case{NativeHistory::Version(), 1, 1}
        << Case{NativeHistory::Version(), -2, 2}
        << Case{NativeHistory::Version(), 0, NativeHistory::MaxEntries() + 1};

    foreach(const Case &c, cases){
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_0);
        stream << NativeHistory::Marker() << c.version << c.index << c.count;
        for(qint32 i = 0; i < c.count && i < 4; i++)
            stream << QUrl(QStringLiteral("http://example.com/%1").arg(i));
        NativeHistory read;
        QVERIFY2(!NativeHistory::Deserialize(data, &read),
                 qPrintable(QStringLiteral("version %1 index %2 count %3")
                            .arg(c.version).arg(c.index).arg(c.count)));
    }
}

void tst_nativehistory::aFileWithAnUnusableOrRepeatedUrlIsRefused(){
    const QList<QList<QUrl>> cases = QList<QList<QUrl>>()
        << (QList<QUrl>() << U("a") << QUrl())
        << (QList<QUrl>() << U("a") << U("a"))
        << (QList<QUrl>() << U("a") << QUrl() << U("b"));

    foreach(const QList<QUrl> &urls, cases){
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_0);
        stream << NativeHistory::Marker() << NativeHistory::Version()
               << static_cast<qint32>(0) << static_cast<qint32>(urls.count());
        foreach(const QUrl &url, urls) stream << url;
        NativeHistory read;
        QVERIFY(!NativeHistory::Deserialize(data, &read));
    }
}

void tst_nativehistory::onlyAFileWhichReadsWholeCountsAsOurs(){
    const QByteArray whole = ListOf(3, 2).Serialize();
    QVERIFY(NativeHistory::LooksLikeOurs(whole));
    QVERIFY(!NativeHistory::LooksLikeOurs(whole.left(8)));
    QVERIFY(!NativeHistory::LooksLikeOurs(whole + QByteArray("more")));
    QVERIFY(!NativeHistory::LooksLikeOurs(QByteArray()));
}

void tst_nativehistory::aLoadOwesExactlyOneRelease(){
    QuickNativeLoadTail tail;
    QVERIFY(!tail.Owed());

    tail.Started();
    QVERIFY(tail.Owed());

    QVERIFY2(tail.Take(), "the first caller does not release");
    QVERIFY2(!tail.Take(), "a second caller releases as well");
    QVERIFY2(!tail.Take(), "and a third");
    QVERIFY(!tail.Owed());
}

void tst_nativehistory::anEarlyReleaseLeavesNothingForTheEnding(){
    QuickNativeLoadTail tail;
    tail.Started();

    QVERIFY(tail.Take());
    QVERIFY(!tail.Take());
}

void tst_nativehistory::anEndingLeavesNothingForAStopAfterIt(){
    QuickNativeLoadTail tail;
    tail.Started();

    QVERIFY(tail.Take());
    QVERIFY(!tail.Take());
    QVERIFY(!tail.Take());
}

void tst_nativehistory::aSecondLoadOwesItsOwnRelease(){
    QuickNativeLoadTail tail;
    tail.Started();
    tail.Started();

    QVERIFY(tail.Take());
    QVERIFY2(!tail.Take(), "two loads left two debts");

    tail.Started();
    QVERIFY(tail.Take());
}

void tst_nativehistory::aReleaseWithNoLoadRunningIsNotOwed(){
    QuickNativeLoadTail tail;
    QVERIFY(!tail.Take());
    QVERIFY(!tail.Owed());
}

void tst_nativehistory::aForgottenTailOwesNothing(){
    QuickNativeLoadTail tail;
    tail.Started();
    tail.Forget();
    QVERIFY(!tail.Owed());
    QVERIFY(!tail.Take());
}

void tst_nativehistory::aStepWaitingBeforeAnyLoadStartedIsStillReleased(){
    NativeHistory history = ListOf(3, 2);
    QuickNativeLoadTail tail;

    const NativeHistory::Request request = history.RequestBack(false);
    QVERIFY(request.kind != NativeHistory::NoMove);
    QVERIFY2(history.Busy(false), "the list is not waiting for anything");
    QVERIFY2(!tail.Owed(), "the tail owes something before any load started");

    const bool release = tail.Take(history.Pending() != NativeHistory::NoMove);
    QVERIFY2(release, "the replacement does not release the waiting step");
    history.Release();

    QVERIFY2(!history.Busy(false), "the list is still waiting for a url");

    tail.Started();
    QVERIFY(tail.Take(history.Pending() != NativeHistory::NoMove));
    QVERIFY(!tail.Take(history.Pending() != NativeHistory::NoMove));
}

void tst_nativehistory::everyReleaseInTheViewGoesThroughTheTail(){
    QString source;
    foreach(const QString &name, QStringList()
            << QStringLiteral("/view/quicknativewebview.cpp")
            << QStringLiteral("/view/quicknativewebview.hpp")){
        QFile file(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR) + name));
        QVERIFY2(file.open(QIODevice::ReadOnly), "check VANILLA_SOURCE_DIR");
        source += QString::fromUtf8(file.readAll());
    }

    QCOMPARE(source.count(QStringLiteral("m_History.Release()")), 1);
    QVERIFY2(source.contains(QStringLiteral("m_LoadTail.Take(m_History.Pending() != NativeHistory::NoMove)")),
             "the one release does not ask the tail and the list");

    QVERIFY2(source.contains(QStringLiteral("m_LoadTail.Started()")),
             "nothing ever owes a release");
}

void tst_nativehistory::theQmlSpellsTheBackendsLoadStatusesTheWayTheBackendDoes(){
    QFile qml(QDir::cleanPath(QStringLiteral(VANILLA_SOURCE_DIR "/view/quicknativewebview.qml")));
    QVERIFY2(qml.open(QIODevice::ReadOnly), "check VANILLA_SOURCE_DIR");
    const QString source = QString::fromUtf8(qml.readAll());

    const QString handler = source.section(QStringLiteral("onLoadingChanged"), 1)
                                  .section(QStringLiteral("onLoadProgressChanged"), 0, 0);
    QVERIFY2(!handler.isEmpty(), "the load handler moved");
    foreach(const QString &name, QStringList()
            << QStringLiteral("LoadStartedStatus") << QStringLiteral("LoadSucceededStatus")
            << QStringLiteral("LoadFailedStatus")  << QStringLiteral("LoadStoppedStatus"))
        QVERIFY2(handler.contains(QStringLiteral("WebView.") + name),
                 qPrintable(QStringLiteral("the load handler never looks at %1").arg(name)));

    QVERIFY2(handler.section(QStringLiteral("LoadFailedStatus"), 1)
             .contains(QStringLiteral("loadFinished(false)")),
             "a failed load reports nothing");
    QVERIFY2(handler.contains(QStringLiteral("loadFinished(true)")),
             "a finished load reports nothing");

    const QString stopped = handler.section(QStringLiteral("LoadStoppedStatus"), 1);
    QVERIFY2(stopped.contains(QStringLiteral("loadStopped()")),
             "a stopped load ends nothing");
    QVERIFY2(!stopped.contains(QStringLiteral("loadFinished(")),
             "a stopped load is reported as a finished or failed one");
    QVERIFY2(!handler.contains(QStringLiteral("LoadFailedStatus ||")),
             "stopping is folded into failing again");

    QVERIFY(LoadEnding::QuickEnding(LoadEnding::QuickFailed).saysFailure);
    QVERIFY(!LoadEnding::QuickEnding(LoadEnding::QuickStopped).saysFailure);
    QVERIFY(LoadEnding::QuickEnding(LoadEnding::QuickStopped).endsLoad);

    QSet<QString> asked;
    QRegularExpression spelling(QStringLiteral("WebView\\.(\\w*Status)\\b"));
    QRegularExpressionMatchIterator it = spelling.globalMatch(source);
    while(it.hasNext()) asked.insert(it.next().captured(1));
    QVERIFY(!asked.isEmpty());

    const QString types = QDir::cleanPath
        (QLibraryInfo::path(QLibraryInfo::QmlImportsPath) +
         QStringLiteral("/QtWebView/plugins.qmltypes"));
    QFile known(types);
    if(!known.open(QIODevice::ReadOnly))
        QSKIP("no QtWebView plugins.qmltypes to check the spelling against");
    const QString described = QString::fromUtf8(known.readAll());

    foreach(const QString &name, asked)
        QVERIFY2(described.contains(QStringLiteral("\"%1\"").arg(name)),
                 qPrintable(QStringLiteral("the backend has no %1").arg(name)));
}

QTEST_MAIN(tst_nativehistory)
#include "tst_nativehistory.moc"
