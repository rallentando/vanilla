#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QSize>

#include "devicescale.hpp"

#include "testsupport.hpp"

class tst_devicescale : public QObject {
    Q_OBJECT

private slots:

    void nothingAtOrBelowTheBaseDpiIsScaled(){
        QCOMPARE(DeviceScale::BaseDpi, 96);

        QCOMPARE(DeviceScale::FromDpi(10, 96), 10);
        QCOMPARE(DeviceScale::FromDpi(10, 95), 10);
        QCOMPARE(DeviceScale::FromDpi(10, 90), 10);
        QCOMPARE(DeviceScale::FromDpi(10, 73), 10);
        QCOMPARE(DeviceScale::FromDpi(10, 72), 10);
        QCOMPARE(DeviceScale::FromDpi(10,  0), 10);
    }

    void thescalesTheDisplaysActuallyReport(){
        QCOMPARE(DeviceScale::FromDpi(10, 120), 12);
        QCOMPARE(DeviceScale::FromDpi(10, 144), 15);
        QCOMPARE(DeviceScale::FromDpi(10, 192), 20);
        QCOMPARE(DeviceScale::FromDpi(16, 120), 20);
    }

    void thearithmeticTruncates(){
        QCOMPARE(DeviceScale::FromDpi(10, 150), 15);
        QCOMPARE(DeviceScale::FromDpi(7, 120), 8);
        QCOMPARE(DeviceScale::FromDpi(100, 120), 125);
    }

    void asizeIsBuiltOneSideAtATime(){
        const int dpi = 120;
        const QSize size(7, 10);
        const QSize built(DeviceScale::FromDpi(size.width(),  dpi),
                          DeviceScale::FromDpi(size.height(), dpi));
        QCOMPARE(built, QSize(8, 12));
        QCOMPARE(size * 120 / 96, QSize(9, 13));
        QVERIFY(built != size * 120 / 96);
    }
};

QTEST_MAIN(tst_devicescale)
#include "tst_devicescale.moc"
