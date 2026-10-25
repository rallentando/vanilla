#include "switch.hpp"

#include <QtTest>

#include "blurbehind.hpp"

class tst_blurbehind : public QObject {
    Q_OBJECT

private slots:
    void theEnvironmentIsReadAsQtReadsIt_data();
    void theEnvironmentIsReadAsQtReadsIt();
    void onlyARasterOrAnOpenGLSurfaceCarriesAlpha();
    void theBlurFollowsTheSurface();
};

void tst_blurbehind::theEnvironmentIsReadAsQtReadsIt_data(){
    QTest::addColumn<int>("rhi");
    QTest::addColumn<int>("downscale");
    QTest::addColumn<QByteArray>("backend");
    QTest::addColumn<bool>("expected");

    QTest::newRow("raster")            << 0 << 0 << QByteArray()                  << true;
    QTest::newRow("rhi 0")             << 0 << 0 << QByteArrayLiteral("d3d11")    << true;
    QTest::newRow("rhi default")       << 1 << 0 << QByteArray()                  << false;
    QTest::newRow("rhi d3d11")         << 1 << 0 << QByteArrayLiteral("d3d11")    << false;
    QTest::newRow("rhi d3d12")         << 1 << 0 << QByteArrayLiteral("d3d12")    << false;
    QTest::newRow("rhi vulkan")        << 1 << 0 << QByteArrayLiteral("vulkan")   << false;
    QTest::newRow("rhi opengl")        << 1 << 0 << QByteArrayLiteral("opengl")   << true;
    QTest::newRow("rhi gl")            << 1 << 0 << QByteArrayLiteral("gl")       << true;
    QTest::newRow("rhi OpenGL")        << 1 << 0 << QByteArrayLiteral("OpenGL")   << false;
    QTest::newRow("downscale default") << 0 << 1 << QByteArray()                  << false;
    QTest::newRow("downscale opengl")  << 0 << 1 << QByteArrayLiteral("opengl")   << true;
}

void tst_blurbehind::theEnvironmentIsReadAsQtReadsIt(){
    QFETCH(int, rhi);
    QFETCH(int, downscale);
    QFETCH(QByteArray, backend);
    QFETCH(bool, expected);
    QCOMPARE(BlurBehind::MainWindowMayBeTranslucent(rhi, downscale, backend), expected);
}

void tst_blurbehind::onlyARasterOrAnOpenGLSurfaceCarriesAlpha(){
    QVERIFY(BlurBehind::SurfaceMayBeTranslucent(QSurface::RasterSurface));
    QVERIFY(BlurBehind::SurfaceMayBeTranslucent(QSurface::OpenGLSurface));
    QVERIFY(!BlurBehind::SurfaceMayBeTranslucent(QSurface::Direct3DSurface));
    QVERIFY(!BlurBehind::SurfaceMayBeTranslucent(QSurface::VulkanSurface));
    QVERIFY(!BlurBehind::SurfaceMayBeTranslucent(QSurface::MetalSurface));
    QVERIFY(!BlurBehind::SurfaceMayBeTranslucent(QSurface::RasterGLSurface));
}

void tst_blurbehind::theBlurFollowsTheSurface(){
    using BlurBehind::Kind;
    using BlurBehind::Mechanism;
    QCOMPARE(BlurBehind::MechanismFor(Kind::Card,       QSurface::OpenGLSurface,   false), Mechanism::Accent);
    QCOMPARE(BlurBehind::MechanismFor(Kind::Card,       QSurface::OpenGLSurface,   true),  Mechanism::Accent);
    QCOMPARE(BlurBehind::MechanismFor(Kind::Card,       QSurface::RasterSurface,   false), Mechanism::Accent);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::RasterSurface,   true),  Mechanism::Accent);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::RasterSurface,   false), Mechanism::Accent);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::OpenGLSurface,   false), Mechanism::SystemBackdrop);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::OpenGLSurface,   true),  Mechanism::Accent);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::Direct3DSurface, false), Mechanism::None);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::VulkanSurface,   false), Mechanism::None);
    QCOMPARE(BlurBehind::MechanismFor(Kind::MainWindow, QSurface::RasterGLSurface, false), Mechanism::None);
}

QTEST_MAIN(tst_blurbehind)
#include "tst_blurbehind.moc"
