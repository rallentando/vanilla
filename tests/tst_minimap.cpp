#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QObject>

#include "minimap.hpp"

#include "testsupport.hpp"

class tst_minimap : public QObject {
    Q_OBJECT

private slots:

    void slideStaysAtZeroWhileTheDrawingFits(){
        QCOMPARE(MiniMap::SlideOffset(500.0, 800.0, 0.0), 0.0);
        QCOMPARE(MiniMap::SlideOffset(500.0, 800.0, 0.5), 0.0);
        QCOMPARE(MiniMap::SlideOffset(500.0, 800.0, 1.0), 0.0);
        QCOMPARE(MiniMap::SlideOffset(800.0, 800.0, 1.0), 0.0);
    }

    void slideSpendsTheSpareHeightLinearly(){
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, 0.0),   0.0);
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, 0.5), 250.0);
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, 1.0), 500.0);

        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0, -1.0),  0.0);
        QCOMPARE(MiniMap::SlideOffset(1300.0, 800.0,  2.0), 500.0);
    }

    void theWindowMovesLikeAScrollBarThumb(){
        struct Regime { qreal contentsHeight, viewportHeight, scale, strip; };
        const Regime regimes[] = {
            { 10000.0, 500.0, 0.08, 400.0 },
            {  4000.0, 500.0, 0.08, 400.0 },
        };
        for(const Regime &regime : regimes){
            const qreal drawing = regime.contentsHeight * regime.scale;
            const qreal indicator = regime.viewportHeight * regime.scale;
            const qreal room = regime.contentsHeight - regime.viewportHeight;
            const qreal travel =
                MiniMap::IndicatorTravel(drawing, regime.strip, indicator);
            QVERIFY(travel > 0);
            for(qreal r = 0.0; r <= 1.0; r += 0.25){
                const qreal top =
                    r * room * regime.scale
                    - MiniMap::SlideOffset(drawing, regime.strip, r);
                QVERIFY2(qAbs(top - r * travel) < 1e-9,
                         qPrintable(QStringLiteral("r=%1: top %2, thumb %3")
                                    .arg(r).arg(top).arg(r * travel)));
            }
        }
    }

    void aClickAimsTheMiddleOfTheViewport(){
        QCOMPARE(MiniMap::JumpRatio(50.0, 0.1, 0.0, 100.0, 1000.0), 0.5);

        const qreal slid = MiniMap::JumpRatio(300.0, 0.1, 200.0, 500.0, 10000.0);
        QVERIFY(qAbs(slid - 0.5) < 1e-9);

        QCOMPARE(MiniMap::JumpRatio(   0.0, 0.1, 0.0, 100.0, 1000.0), 0.0);
        QCOMPARE(MiniMap::JumpRatio(1000.0, 0.1, 0.0, 100.0, 1000.0), 1.0);

        QCOMPARE(MiniMap::JumpRatio(50.0, 0.1, 0.0, 1000.0, 1000.0), 0.5);
        QCOMPARE(MiniMap::JumpRatio(50.0, 0.1, 0.0, 2000.0, 1000.0), 0.5);
    }

    void aDragMapsTheThumbLinearly(){
        QCOMPARE(MiniMap::DragRatio(  0.0, 0.0, 300.0), 0.0);
        QCOMPARE(MiniMap::DragRatio(150.0, 0.0, 300.0), 0.5);
        QCOMPARE(MiniMap::DragRatio(300.0, 0.0, 300.0), 1.0);
        QCOMPARE(MiniMap::DragRatio(310.0, 0.0, 300.0), 1.0);
        QCOMPARE(MiniMap::DragRatio( -10.0, 0.0, 300.0), 0.0);

        QCOMPARE(MiniMap::DragRatio(200.0, 50.0, 300.0), 0.5);

        QCOMPARE(MiniMap::DragRatio(100.0, 0.0, 0.0), 0.5);
        QCOMPARE(MiniMap::DragRatio(100.0, 0.0, -5.0), 0.5);
    }

    void parseSiftsTheAnswerOfThePage(){
        QVariantMap answer;
        answer[QStringLiteral("w")] = 1000;
        answer[QStringLiteral("h")] = 5000;
        answer[QStringLiteral("over")] = true;
        QVariantList rows;
        rows << QVariant(QVariantList()
                         << 0 << 10.0 << 20.0 << 100.0 << 10.0)
             << QVariant(QVariantList()
                         << 1 << 0.0 << 100.0 << 50.0 << 50.0)
             << QVariant(QVariantList()
                         << 2 << 5.0 << 5.0 << 30.0 << 10.0)
             << QVariant(QVariantList() << 0 << 1.0 << 2.0)
             << QVariant(QVariantList()
                         << 7 << 0.0 << 0.0 << 10.0 << 10.0)
             << QVariant(QVariantList()
                         << 0 << QStringLiteral("x")
                         << 0.0 << 10.0 << 10.0)
             << QVariant(QVariantList()
                         << 0 << 0.0 << 0.0 << 0.0 << 10.0);
        answer[QStringLiteral("blocks")] = rows;

        QSizeF pageSize;
        bool overflowed = false;
        const QList<MiniMap::Block> blocks =
            MiniMap::ParseBlocks(answer, &pageSize, &overflowed);

        QCOMPARE(pageSize, QSizeF(1000, 5000));
        QVERIFY(overflowed);
        QCOMPARE(blocks.size(), 3);
        QCOMPARE(blocks.at(0).kind, static_cast<int>(MiniMap::TextBlock));
        QCOMPARE(blocks.at(0).rect, QRectF(10, 20, 100, 10));
        QCOMPARE(blocks.at(1).kind, static_cast<int>(MiniMap::MediaBlock));
        QCOMPARE(blocks.at(2).kind, static_cast<int>(MiniMap::ControlBlock));
    }

    void theViewportWindowIsNeverThinnerThanItIsDrawn(){
        const qreal scale = 0.02;
        const qreal minimum = 3.0;
        QCOMPARE(MiniMap::IndicatorHeight(800.0, scale, minimum), 16.0);
        QCOMPARE(MiniMap::IndicatorHeight(100.0, scale, minimum), 3.0);
        QCOMPARE(MiniMap::IndicatorHeight(0.0, scale, minimum), 3.0);

        const qreal drawing = 40000.0 * scale;
        const qreal strip = 600.0;
        const qreal height = MiniMap::IndicatorHeight(100.0, scale, minimum);
        QCOMPARE(MiniMap::IndicatorTravel(drawing, strip, height),
                 strip - height);

        QVERIFY(MiniMap::IndicatorTravel(drawing, strip, 100.0 * scale) >
                MiniMap::IndicatorTravel(drawing, strip, height));
    }

    void aStripWithNoViewIsNeverActive(){
        MiniMap map(nullptr);
        QVERIFY(!map.IsActive());

        map.SetView(nullptr);
        QVERIFY(!map.IsActive());
        QVERIFY(map.isHidden());

        map.SetShelved(true);
        QVERIFY(!map.IsActive());
        QVERIFY(map.isHidden());

        map.SetShelved(false);
        QVERIFY(!map.IsActive());
        QVERIFY(map.isHidden());
    }

    void aBrokenEnvelopeLeavesTheStripBlank(){
        QSizeF pageSize(1, 1);
        bool overflowed = true;

        QVERIFY(MiniMap::ParseBlocks(QVariant(), &pageSize, &overflowed)
                .isEmpty());
        QVERIFY(pageSize.isEmpty());
        QVERIFY(!overflowed);

        QVariantMap noSize;
        noSize[QStringLiteral("blocks")] = QVariantList();
        QVERIFY(MiniMap::ParseBlocks(noSize, &pageSize, &overflowed)
                .isEmpty());
        QVERIFY(pageSize.isEmpty());
    }

    void scrolledBooksOneSettledCollection(){
        MiniMap map(nullptr);
        QVERIFY(!map.m_RecollectTimer.isActive());
        map.Scrolled();
        QVERIFY(map.m_RecollectTimer.isActive());
        QVERIFY(map.m_RecollectTimer.isSingleShot());
        map.Scrolled();
        QVERIFY(map.m_RecollectTimer.isActive());
        QTest::qWait(map.m_RecollectTimer.interval() + 100);
        QVERIFY(!map.m_RecollectTimer.isActive());
    }

    void clearingTheStripCancelsTheBooking(){
        MiniMap map(nullptr);
        map.Scrolled();
        QVERIFY(map.m_RecollectTimer.isActive());
        map.ClearStrip();
        QVERIFY(!map.m_RecollectTimer.isActive());
    }
};

QTEST_MAIN(tst_minimap)
#include "tst_minimap.moc"
