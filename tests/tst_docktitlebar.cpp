#include "switch.hpp"
#include "const.hpp"
#include "docktitlebar.hpp"
#include "theme.hpp"
#include "testsupport.hpp"

#include <QtTest>
#include <QMainWindow>
#include <QDockWidget>
#include <QImage>
#include <QMouseEvent>

class tst_docktitlebar : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void theButtonsSitAtTheRightAndFollowTheFeatures();
    void theButtonsCloseAndFloatTheDock();
    void aPressOffTheButtonsIsLeftToTheDock();
    void theGroundIsTheBarsOwn();
    void theTranslucentGroundIsOnlyForTheWindowsOwnSurface();

private:
    struct Fixture {
        QMainWindow window;
        QDockWidget *dock;
        DockTitleBar *bar;
        Fixture()
            : dock(new QDockWidget(QStringLiteral("Inspector"), &window))
            , bar(new DockTitleBar(dock))
        {
            dock->setTitleBarWidget(bar);
            window.addDockWidget(Qt::RightDockWidgetArea, dock);
            window.resize(600, 400);
            window.show();
            QTest::qWaitForWindowExposed(&window);
            bar->resize(300, bar->sizeHint().height());
        }
    };
};

void tst_docktitlebar::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_docktitlebar::init(){
    Theme::ApplyScheme(QStringLiteral("light"));
}

void tst_docktitlebar::theButtonsSitAtTheRightAndFollowTheFeatures(){
    Fixture f;
    DockTitleBar *bar = f.bar;
    const int pad = bar->ScaleByDevice(TOOL_BAR_PADDING);
    const int chip = bar->ScaleByDevice(DOCK_TITLE_BUTTON_SIZE);
    const int spacing = bar->ScaleByDevice(TOOL_BAR_ICON_SPACING);

    QCOMPARE(bar->sizeHint().height(), bar->ScaleByDevice(DOCK_TITLE_BAR_HEIGHT));

    const QRect close = bar->CloseRect();
    const QRect flt = bar->FloatRect();
    const QRect title = bar->TitleRect();
    QCOMPARE(close.right(), bar->width() - pad - 1);
    QCOMPARE(close.size(), QSize(chip, chip));
    QCOMPARE(flt.right(), close.left() - spacing - 1);
    QCOMPARE(flt.size(), QSize(chip, chip));
    QVERIFY(title.right() < flt.left());
    QVERIFY(title.left() == pad);
    QVERIFY(!close.intersects(flt));
    QVERIFY(bar->rect().contains(close) && bar->rect().contains(flt));

    f.dock->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    QVERIFY(bar->CloseRect().isNull());
    QCOMPARE(bar->FloatRect(), close);
    f.dock->setFeatures(QDockWidget::DockWidgetMovable);
    QVERIFY(bar->CloseRect().isNull());
    QVERIFY(bar->FloatRect().isNull());
    QCOMPARE(bar->TitleRect().right(), bar->width() - pad - 1);
}

void tst_docktitlebar::theButtonsCloseAndFloatTheDock(){
    Fixture f;
    QVERIFY(f.dock->isVisible());
    QVERIFY(!f.dock->isFloating());

    QTest::mouseClick(f.bar, Qt::LeftButton, Qt::NoModifier, f.bar->FloatRect().center());
    QVERIFY(f.dock->isFloating());
    QTest::mouseClick(f.bar, Qt::LeftButton, Qt::NoModifier, f.bar->FloatRect().center());
    QVERIFY(!f.dock->isFloating());

    QTest::mouseClick(f.bar, Qt::LeftButton, Qt::NoModifier, f.bar->CloseRect().center());
    QVERIFY(!f.dock->isVisible());
}

void tst_docktitlebar::aPressOffTheButtonsIsLeftToTheDock(){
    Fixture f;
    DockTitleBar *bar = f.bar;

    const QPointF onTitle = bar->TitleRect().center();
    QMouseEvent press(QEvent::MouseButtonPress, onTitle, bar->mapToGlobal(onTitle.toPoint()),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    static_cast<QObject*>(bar)->event(&press);
    QVERIFY(!press.isAccepted());
    QMouseEvent release(QEvent::MouseButtonRelease, onTitle, bar->mapToGlobal(onTitle.toPoint()),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    static_cast<QObject*>(bar)->event(&release);
    QVERIFY(!release.isAccepted());

    const QPointF onClose = bar->CloseRect().center();
    QMouseEvent pressClose(QEvent::MouseButtonPress, onClose, bar->mapToGlobal(onClose.toPoint()),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    static_cast<QObject*>(bar)->event(&pressClose);
    QVERIFY(pressClose.isAccepted());
    QMouseEvent releaseAway(QEvent::MouseButtonRelease, onTitle, bar->mapToGlobal(onTitle.toPoint()),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    static_cast<QObject*>(bar)->event(&releaseAway);
    QVERIFY(releaseAway.isAccepted());
    QVERIFY(f.dock->isVisible());
}

void tst_docktitlebar::theGroundIsTheBarsOwn(){
    Fixture f;
    QImage image(f.bar->size(), QImage::Format_ARGB32);
    image.fill(Qt::magenta);
    f.bar->render(&image);
    QCOMPARE(image.pixelColor(1, 1), Theme::Color(Theme::BarBackground));
    QCOMPARE(image.pixelColor(image.width() - 2, 1), Theme::Color(Theme::BarBackground));
    const QRect title = f.bar->TitleRect();
    int ink = 0;
    for(int y = title.top(); y <= title.bottom(); y++)
        for(int x = title.left(); x <= title.right(); x++)
            if(image.pixelColor(x, y) != Theme::Color(Theme::BarBackground)) ink++;
    QVERIFY(ink > 0);
}

void tst_docktitlebar::theTranslucentGroundIsOnlyForTheWindowsOwnSurface(){
    Fixture f;
    f.window.setAttribute(Qt::WA_TranslucentBackground);
    QVERIFY(f.bar->OnTranslucentSurface());

    f.dock->setAttribute(Qt::WA_NativeWindow);
    f.dock->winId();
    QVERIFY(!f.bar->OnTranslucentSurface());
    f.dock->setAttribute(Qt::WA_NativeWindow, false);

    f.dock->setFloating(true);
    QVERIFY(f.dock->isWindow());
    QVERIFY(!f.bar->OnTranslucentSurface());
    f.dock->setFloating(false);
}

QTEST_MAIN(tst_docktitlebar)
#include "tst_docktitlebar.moc"
