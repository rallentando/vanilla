#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QObject>

#include "view.hpp"

#include "testsupport.hpp"

class BareView : public QObject, public View {

public:
    BareView() : QObject(nullptr), View(nullptr) {}

    QObject *base() Q_DECL_OVERRIDE { return this;}

    QSize size() Q_DECL_OVERRIDE { return QSize();}
    void resize(QSize) Q_DECL_OVERRIDE {}
    void show() Q_DECL_OVERRIDE {}
    void hide() Q_DECL_OVERRIDE {}
    void raise() Q_DECL_OVERRIDE {}
    void lower() Q_DECL_OVERRIDE {}
    void repaint() Q_DECL_OVERRIDE {}
    bool visible() Q_DECL_OVERRIDE { return false;}
    void setFocus(Qt::FocusReason = Qt::OtherFocusReason) Q_DECL_OVERRIDE {}
};

class tst_viewinterface : public QObject {
    Q_OBJECT

private slots:

    void everyQuestionIsAnsweredOnceAndEmpty(){
        BareView view;
        int called = 0;

        view.CallWithGotBaseUrl([&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 1);

        view.CallWithGotCurrentBaseUrl([&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 2);

        view.CallWithHitLinkUrl(QPoint(1, 1), [&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 3);

        view.CallWithHitImageUrl(QPoint(1, 1), [&](const QUrl &url){ called++; QVERIFY(url.isEmpty());});
        QCOMPARE(called, 4);

        view.CallWithSelectedText([&](const QString &text){ called++; QVERIFY(text.isEmpty());});
        QCOMPARE(called, 5);

        view.CallWithSelectedHtml([&](const QString &html){ called++; QVERIFY(html.isEmpty());});
        QCOMPARE(called, 6);

        view.CallWithWholeText([&](const QString &text){ called++; QVERIFY(text.isEmpty());});
        QCOMPARE(called, 7);

        view.CallWithWholeHtml([&](const QString &html){ called++; QVERIFY(html.isEmpty());});
        QCOMPARE(called, 8);

        view.CallWithSelectionRegion([&](const QRegion &region){ called++; QVERIFY(region.isEmpty());});
        QCOMPARE(called, 9);

        view.CallWithEvaluatedJavaScriptResult(QStringLiteral("1 + 1"),
                                               [&](const QVariant &v){ called++; QVERIFY(!v.isValid());});
        QCOMPARE(called, 10);

        view.CallWithFoundElements(Page::FindElementsOption(),
                                   [&](SharedWebElementList list){ called++; QVERIFY(list.isEmpty());});
        QCOMPARE(called, 11);

        view.CallWithHitElement(QPoint(1, 1),
                                [&](SharedWebElement elem){ called++; QVERIFY(!elem);});
        QCOMPARE(called, 12);
    }

    void thesynchronousOneDoesNotWaitForAnAnswerItAlreadyHas(){
        BareView view;

        QElapsedTimer timer;
        timer.start();
        const QString html = view.WholeHtml();

        QVERIFY(html.isEmpty());
        QVERIFY2(timer.elapsed() < 1000, qPrintable(QString::number(timer.elapsed())));
    }

    void aViewIsNamedAfterTheObjectItReallyIs(){
        BareView view;
        QCOMPARE(view.GetViewTypeName(),
                 QString::fromLatin1(view.base()->metaObject()->className()));
        QVERIFY(!view.GetViewTypeName().isEmpty());
    }

    void loadingStateIsNotSomethingAnImplementationDecides(){
        BareView view;
        QCOMPARE(view.LoadProgress(), 0);
        QCOMPARE(view.IsLoading(), false);
    }

    void anEngineNotchIsFiveThirdsOfTheMouseNotch(){
        const QPoint notch(0, 120);
        QCOMPARE(View::EngineWheelAngle(notch), QPoint(0, 200));
        QCOMPARE(View::EngineWheelAngle(QPoint(0, -120)), QPoint(0, -200));
        QCOMPARE(View::EngineWheelAngle(QPoint(120, 0)), QPoint(200, 0));
        QCOMPARE(View::EngineWheelAngle(QPoint(-120, 120)), QPoint(-200, 200));
        QCOMPARE(View::EngineWheelAngle(QPoint(0, 30)), QPoint(0, 50));
        QCOMPARE(notch, QPoint(0, 120));
    }

    void theLedgerRecoversUntilTheLimitThenGivesUpOnce(){
        RenderProcessLedger ledger;
        QCOMPARE(ledger.Deaths(), 0);
        QVERIFY(!ledger.GaveUp());

        for(int i = 1; i < RenderProcessLedger::Limit; i++){
            QVERIFY(ledger.Count() == RenderProcessLedger::Verdict::Recover);
            QCOMPARE(ledger.Deaths(), i);
            QVERIFY(!ledger.GaveUp());
        }

        QVERIFY(ledger.Count() == RenderProcessLedger::Verdict::GiveUp);
        QCOMPARE(ledger.Deaths(), RenderProcessLedger::Limit);
        QVERIFY(ledger.GaveUp());

        for(int i = 0; i < 5; i++)
            QVERIFY(ledger.Count() == RenderProcessLedger::Verdict::Silent);
    }

    void onlyALoadWhichFinishedClearsTheLedger(){
        BareView view;
        for(int i = 0; i < RenderProcessLedger::Limit; i++)
            view.RenderProcessDeaths().Count();
        QVERIFY(view.RenderProcessDeaths().GaveUp());

        view.OnLoadFinished(false);
        QVERIFY(view.RenderProcessDeaths().GaveUp());
        QCOMPARE(view.RenderProcessDeaths().Deaths(), RenderProcessLedger::Limit);

        view.OnLoadFinished(true);
        QVERIFY(!view.RenderProcessDeaths().GaveUp());
        QCOMPARE(view.RenderProcessDeaths().Deaths(), 0);
        QVERIFY(view.RenderProcessDeaths().Count() == RenderProcessLedger::Verdict::Recover);
    }
};

QTEST_MAIN(tst_viewinterface)
#include "tst_viewinterface.moc"
