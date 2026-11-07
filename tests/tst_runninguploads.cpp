#include "switch.hpp"

#include <QtTest>

#include <type_traits>

#include "runninguploads.hpp"

static_assert(!std::is_invocable_v<decltype(&RunningUploads::Ended), RunningUploads*, const void*>);

class tst_runninguploads : public QObject {
    Q_OBJECT

    static const int Delay = 40;
    int m_Dummy[2] = {0, 0};
    const View *A() const { return reinterpret_cast<const View*>(&m_Dummy[0]);}
    const View *B() const { return reinterpret_cast<const View*>(&m_Dummy[1]);}
    const QUrl U = QUrl(QStringLiteral("https://example.com/up"));

private slots:
    void onlyAFormsFilesCount(){
        QVERIFY(RunningUploads::IsMultipart("multipart/form-data; boundary=----x"));
        QVERIFY(RunningUploads::IsMultipart("Multipart/Form-Data; boundary=x"));
        QVERIFY(RunningUploads::IsMultipart("  multipart/form-data"));
        QVERIFY(!RunningUploads::IsMultipart("application/x-www-form-urlencoded"));
        QVERIFY(!RunningUploads::IsMultipart("multipart/mixed; boundary=x"));
        QVERIFY(!RunningUploads::IsMultipart(QByteArray()));
    }

    void notShownUntilTheDelayIsOver(){
        RunningUploads uploads(Delay);
        QSignalSpy appeared(&uploads, &RunningUploads::Appeared);
        QSignalSpy changed(&uploads, &RunningUploads::Changed);
        uploads.Started(A(), U, nullptr);
        QVERIFY(uploads.Shown().isEmpty());
        QCOMPARE(appeared.size(), 0);
        QTRY_COMPARE(appeared.size(), 1);
        QCOMPARE(changed.size(), 1);
        QCOMPARE(uploads.Shown().size(), 1);
        QCOMPARE(uploads.Shown().first().url, U);
    }

    void aShortOneIsNeverShown(){
        RunningUploads uploads(Delay);
        QSignalSpy appeared(&uploads, &RunningUploads::Appeared);
        QSignalSpy changed(&uploads, &RunningUploads::Changed);
        uploads.Started(A(), U, nullptr);
        QVERIFY(uploads.Ended(A()));
        QTest::qWait(Delay * 3);
        QCOMPARE(appeared.size(), 0);
        QCOMPARE(changed.size(), 0);
        QVERIFY(uploads.Shown().isEmpty());
    }

    void theNextOneWaitsItsOwnDelay(){
        RunningUploads uploads(Delay * 4);
        uploads.Started(A(), U, nullptr);
        QTest::qWait(Delay * 3);
        uploads.Ended(A());
        uploads.Started(A(), U, nullptr);
        QTest::qWait(Delay * 2);
        QVERIFY(uploads.Shown().isEmpty());
        QTRY_COMPARE(uploads.Shown().size(), 1);
    }

    void onePerView(){
        RunningUploads uploads(Delay);
        int firstStopped = 0, secondStopped = 0;
        uploads.Started(A(), U, [&](){ firstStopped++;});
        uploads.Started(A(), QUrl(QStringLiteral("https://example.com/again")), [&](){ secondStopped++;});
        QTRY_COMPARE(uploads.Shown().size(), 1);
        QTest::qWait(Delay * 2);
        QCOMPARE(uploads.Shown().size(), 1);
        QCOMPARE(uploads.Shown().first().url, U);
        uploads.Cancel(uploads.Shown().first().id);
        QCOMPARE(firstStopped, 1);
        QCOMPARE(secondStopped, 0);
    }

    void twoViewsAreTwoRowsAndStopTheirOwn(){
        RunningUploads uploads(Delay);
        int a = 0, b = 0;
        uploads.Started(A(), U, [&](){ a++;});
        uploads.Started(B(), U, [&](){ b++;});
        QTRY_COMPARE(uploads.Shown().size(), 2);
        uploads.Cancel(uploads.Shown().last().id);
        QCOMPARE(a, 0);
        QCOMPARE(b, 1);
        uploads.Ended(B());
        QCOMPARE(uploads.Shown().size(), 1);
    }

    void cancelLeavesItUntilTheLoadEnds(){
        RunningUploads uploads(Delay);
        int stopped = 0;
        uploads.Started(A(), U, [&](){ stopped++;});
        QTRY_COMPARE(uploads.Shown().size(), 1);
        uploads.Cancel(uploads.Shown().first().id);
        QCOMPARE(stopped, 1);
        QCOMPARE(uploads.Shown().size(), 1);
    }

    void theEndMayComeFromInsideTheStop(){
        RunningUploads uploads(Delay);
        QString seen;
        uploads.Started(A(), U, [&](){
            seen = QStringLiteral("before");
            uploads.Ended(A());
            seen += QStringLiteral(" after");
        });
        QTRY_COMPARE(uploads.Shown().size(), 1);
        uploads.Cancel(uploads.Shown().first().id);
        QCOMPARE(seen, QStringLiteral("before after"));
        QVERIFY(uploads.Shown().isEmpty());
    }

    void anOldIdReachesNothing(){
        RunningUploads uploads(Delay);
        int first = 0, second = 0;
        uploads.Started(A(), U, [&](){ first++;});
        QTRY_COMPARE(uploads.Shown().size(), 1);
        const int old = uploads.Shown().first().id;
        uploads.Ended(A());
        uploads.Started(A(), U, [&](){ second++;});
        QTRY_COMPARE(uploads.Shown().size(), 1);
        QVERIFY(uploads.Shown().first().id != old);
        uploads.Cancel(old);
        QCOMPARE(first, 0);
        QCOMPARE(second, 0);
    }

    void endingAViewWithNoneSaysSoAndChangesNothing(){
        RunningUploads uploads(Delay);
        QSignalSpy changed(&uploads, &RunningUploads::Changed);
        QVERIFY(!uploads.Ended(A()));
        QCOMPARE(changed.size(), 0);
    }

    void aShownOneGoingSaysSo(){
        RunningUploads uploads(Delay);
        uploads.Started(A(), U, nullptr);
        QTRY_COMPARE(uploads.Shown().size(), 1);
        QSignalSpy changed(&uploads, &RunningUploads::Changed);
        QVERIFY(uploads.Ended(A()));
        QCOMPARE(changed.size(), 1);
        QVERIFY(uploads.Shown().isEmpty());
    }
};

QTEST_MAIN(tst_runninguploads)
#include "tst_runninguploads.moc"
