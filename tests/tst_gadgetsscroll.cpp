#include "switch.hpp"
#include "const.hpp"

#include <QtTest>

#include "graphicstableview.hpp"

#include "testsupport.hpp"

class tst_gadgetsscroll : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void indicatorFollowsTheScroll();
    void indicatorFollowsTheScroll_data();

    void indicatorStaysAtTheTopWhenNothingScrolls();
    void scrollFollowsTheIndicator();
    void scrollFollowsTheIndicator_data();
    void scrollIsZeroWhenNothingScrolls();

    void neitherEverReturnsANaN();
    void neitherEverReturnsANaN_data();

    void scrollStopsWhereTheContentsEnd();
    void scrollStopsWhereTheContentsEnd_data();
};

void tst_gadgetsscroll::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_gadgetsscroll::indicatorFollowsTheScroll_data(){
    QTest::addColumn<qreal>("maxY");
    QTest::addColumn<qreal>("scroll");
    QTest::addColumn<qreal>("maxScroll");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("top")     << 200.0 << 0.0  << 40.0 << 0.0;
    QTest::newRow("middle")  << 200.0 << 20.0 << 40.0 << 100.0;
    QTest::newRow("bottom")  << 200.0 << 40.0 << 40.0 << 200.0;
    QTest::newRow("quarter") << 100.0 << 10.0 << 40.0 << 25.0;
    QTest::newRow("past the end") << 200.0 << 80.0 << 40.0 << 200.0;
    QTest::newRow("negative")     << 200.0 << -8.0 << 40.0 << 0.0;
}

void tst_gadgetsscroll::indicatorFollowsTheScroll(){
    QFETCH(qreal, maxY);
    QFETCH(qreal, scroll);
    QFETCH(qreal, maxScroll);
    QFETCH(qreal, expected);

    QCOMPARE(GraphicsTableView::ScrollIndicatorY(maxY, scroll, maxScroll), expected);
}

void tst_gadgetsscroll::indicatorStaysAtTheTopWhenNothingScrolls(){
    QCOMPARE(GraphicsTableView::ScrollIndicatorY(200.0, 0.0, 0.0), 0.0);
    QCOMPARE(GraphicsTableView::ScrollIndicatorY(200.0, 5.0, 0.0), 0.0);
    QCOMPARE(GraphicsTableView::ScrollIndicatorY(-4.0, 0.0, 40.0), 0.0);
    QCOMPARE(GraphicsTableView::ScrollIndicatorY(0.0, 0.0, 40.0), 0.0);
}

void tst_gadgetsscroll::scrollFollowsTheIndicator_data(){
    QTest::addColumn<qreal>("y");
    QTest::addColumn<qreal>("maxY");
    QTest::addColumn<qreal>("maxScroll");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("top")    << 0.0   << 200.0 << 40.0 << 0.0;
    QTest::newRow("middle") << 100.0 << 200.0 << 40.0 << 20.0;
    QTest::newRow("bottom") << 200.0 << 200.0 << 40.0 << 40.0;
    QTest::newRow("past the end") << 400.0 << 200.0 << 40.0 << 40.0;
    QTest::newRow("negative")     << -10.0 << 200.0 << 40.0 << 0.0;
}

void tst_gadgetsscroll::scrollFollowsTheIndicator(){
    QFETCH(qreal, y);
    QFETCH(qreal, maxY);
    QFETCH(qreal, maxScroll);
    QFETCH(qreal, expected);

    QCOMPARE(GraphicsTableView::ScrollFromIndicatorY(y, maxY, maxScroll), expected);
}

void tst_gadgetsscroll::scrollIsZeroWhenNothingScrolls(){
    QCOMPARE(GraphicsTableView::ScrollFromIndicatorY(0.0, 0.0, 40.0), 0.0);
    QCOMPARE(GraphicsTableView::ScrollFromIndicatorY(10.0, 0.0, 40.0), 0.0);
    QCOMPARE(GraphicsTableView::ScrollFromIndicatorY(10.0, -4.0, 40.0), 0.0);
    QCOMPARE(GraphicsTableView::ScrollFromIndicatorY(10.0, 200.0, 0.0), 0.0);
}

void tst_gadgetsscroll::neitherEverReturnsANaN_data(){
    QTest::addColumn<qreal>("a");
    QTest::addColumn<qreal>("b");
    QTest::addColumn<qreal>("c");

    const qreal nan = qQNaN();
    const qreal inf = qInf();

    QTest::newRow("all zero")        << 0.0 << 0.0 << 0.0;
    QTest::newRow("zero over zero")  << 200.0 << 0.0 << 0.0;
    QTest::newRow("nan first")       << nan << 0.0 << 40.0;
    QTest::newRow("nan second")      << 200.0 << nan << 40.0;
    QTest::newRow("nan third")       << 200.0 << 0.0 << nan;
    QTest::newRow("inf first")       << inf << 0.0 << 40.0;
    QTest::newRow("inf second")      << 200.0 << inf << 40.0;
    QTest::newRow("inf third")       << 200.0 << 0.0 << inf;
    QTest::newRow("negative")        << -200.0 << -5.0 << -40.0;
}

void tst_gadgetsscroll::neitherEverReturnsANaN(){
    QFETCH(qreal, a);
    QFETCH(qreal, b);
    QFETCH(qreal, c);

    QVERIFY(qIsFinite(GraphicsTableView::ScrollIndicatorY(a, b, c)));
    QVERIFY(qIsFinite(GraphicsTableView::ScrollFromIndicatorY(a, b, c)));
}

void tst_gadgetsscroll::scrollStopsWhereTheContentsEnd_data(){
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("lines");
    QTest::addColumn<int>("titles");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("half a screen")  <<  4 << 5 << 5 << 40 <<  0.0;
    QTest::newRow("exactly a screen") << 25 << 5 << 5 << 40 <<  0.0;
    QTest::newRow("one line over")  << 30 << 5 << 5 << 40 <<  5.0;
    QTest::newRow("ragged last line") << 27 << 5 << 5 << 40 <<  5.0;
    QTest::newRow("many lines over") << 100 << 5 << 5 << 40 << 75.0;

    QTest::newRow("titles decide")  << 30 << 5 << 5 << 10 << 20.0;
    QTest::newRow("no titles")      << 30 << 5 << 5 <<  0 <<  5.0;

    QTest::newRow("empty")          <<  0 << 5 << 5 << 40 <<  0.0;
    QTest::newRow("no columns yet") << 10 << 0 << 0 <<  0 <<  0.0;
    QTest::newRow("no lines yet")   << 30 << 5 << 0 <<  0 << 25.0;
}

void tst_gadgetsscroll::scrollStopsWhereTheContentsEnd(){
    QFETCH(int, count);
    QFETCH(int, columns);
    QFETCH(int, lines);
    QFETCH(int, titles);
    QFETCH(qreal, expected);

    QCOMPARE(GraphicsTableView::MaxScrollOf(count, columns, lines, titles), expected);
}

QTEST_MAIN(tst_gadgetsscroll)
#include "tst_gadgetsscroll.moc"
