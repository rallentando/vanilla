#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QImage>
#include <QPainter>

#include <functional>

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
    void indicatorIsCentredAcrossDisplayScales_data();
    void indicatorIsCentredAcrossDisplayScales();
    void paintedIndicatorIsCentredAcrossDisplayScales_data();
    void paintedIndicatorIsCentredAcrossDisplayScales();

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

void tst_gadgetsscroll::indicatorIsCentredAcrossDisplayScales_data(){
    QTest::addColumn<int>("dpi");

    QTest::newRow("100 percent") << 96;
    QTest::newRow("125 percent") << 120;
    QTest::newRow("150 percent") << 144;
    QTest::newRow("200 percent") << 192;
}

void tst_gadgetsscroll::indicatorIsCentredAcrossDisplayScales(){
    QFETCH(int, dpi);

    const qreal inset = DeviceScale::FromDpi(3, dpi);
    const QRectF bar(20.0, 30.0,
                     DeviceScale::FromDpi(GADGETS_SCROLL_BAR_WIDTH, dpi), 200.0);
    const QRectF indicator = GraphicsTableView::ScrollIndicatorRect(bar, inset, 40.0);

    QCOMPARE(indicator.center().x(), bar.center().x());
    QCOMPARE(indicator.left() - bar.left(), bar.right() - indicator.right());
}

void tst_gadgetsscroll::paintedIndicatorIsCentredAcrossDisplayScales_data(){
    indicatorIsCentredAcrossDisplayScales_data();
}

void tst_gadgetsscroll::paintedIndicatorIsCentredAcrossDisplayScales(){
    QFETCH(int, dpi);

    const qreal inset = DeviceScale::FromDpi(3, dpi);
    const QRectF bar(10.0, 10.0,
                     DeviceScale::FromDpi(GADGETS_SCROLL_BAR_WIDTH, dpi), 50.0);
    const QRectF rect = GraphicsTableView::ScrollIndicatorRect(bar, inset, 30.0);
    const qreal travel = GraphicsTableView::ScrollIndicatorTravel
        (bar.height(), rect.height(), inset);

    const auto PaintedBounds = [](const std::function<void(QPainter*)> &draw){
        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, false);
        draw(&painter);
        painter.end();

        QRect bounds;
        for(int y = 0; y < image.height(); y++){
            for(int x = 0; x < image.width(); x++){
                if(qAlpha(image.pixel(x, y)) != 0)
                    bounds = bounds.united(QRect(x, y, 1, 1));
            }
        }
        return bounds;
    };

    const QRect barPixels = PaintedBounds([&bar](QPainter *painter){
        painter->setPen(Qt::white);
        painter->setBrush(Qt::gray);
        painter->drawRect(bar);
    });
    const QRect topIndicatorPixels = PaintedBounds([&rect](QPainter *painter){
        ScrollIndicator indicator;
        indicator.setRect(rect);
        indicator.paint(painter, nullptr, nullptr);
    });
    const QRect bottomIndicatorPixels = PaintedBounds([&rect, travel](QPainter *painter){
        ScrollIndicator indicator;
        indicator.setRect(rect.translated(0.0, travel));
        indicator.paint(painter, nullptr, nullptr);
    });

    const qreal barCenter = barPixels.x() + barPixels.width() / 2.0;
    const qreal indicatorCenter = topIndicatorPixels.x() + topIndicatorPixels.width() / 2.0;
    QCOMPARE(indicatorCenter, barCenter);
    QCOMPARE(topIndicatorPixels.top() - barPixels.top(),
             barPixels.bottom() - bottomIndicatorPixels.bottom());
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
