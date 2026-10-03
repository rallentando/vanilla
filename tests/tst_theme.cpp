#include "switch.hpp"

#include <QtTest>
#include <QImage>
#include <QPainter>

#include "theme.hpp"

#include "testsupport.hpp"

class tst_theme : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void lightColorsAreTheOnesTheCallSitesUsedToHold();
    void lightColorsAreTheOnesTheCallSitesUsedToHold_data();

    void darkColorsAreTheOnesTheDarkPaletteMeans();
    void darkColorsAreTheOnesTheDarkPaletteMeans_data();

    void everyRoleHasAValue();
    void everyRoleHasAValue_data();

    void schemeSurvivesThePropertiesThatMatter();

    void applySchemeReadsTheSetting();
    void applySchemeReadsTheSetting_data();
    void applySchemeOnlyReportsARealChange();

    void alphaOverloadKeepsTheColorAndReplacesTheAlpha();
    void brushAndPenCarryTheColor();
    void anOutOfRangeRoleIsInvalidRatherThanACrash();

    void inkLeavesTheLightPaletteAlone();
    void inkRepaintsForTheDarkPalette();
    void fillRepaintsForEitherPalette();
    void pixmapCacheFollowsTheScheme();

    void emptyThumbnailFillsTheBoxAndSaysWhichKindItIs();
    void emptyThumbnailLeavesOutACaptionThatWouldNotFit();
};

namespace {

    int InkPixels(const QImage &image, const QColor &fill){
        int n = 0;
        for(int y = 0; y < image.height(); y++)
            for(int x = 0; x < image.width(); x++)
                if(image.pixelColor(x, y) != fill) n++;
        return n;
    }

    QImage Drawn(const QSize &size, Theme::Role fill, bool isDirectory){
        QImage image(size, QImage::Format_ARGB32);
        image.fill(Qt::magenta);
        QPainter painter(&image);
        Theme::DrawEmptyThumbnail(&painter, QRectF(QPointF(), QSizeF(size)),
                                  fill, Theme::FlatText, isDirectory);
        return image;
    }
}

void tst_theme::initTestCase(){
    TestSupport::SilenceDebugOutput();
}

void tst_theme::init(){
    Theme::ApplyScheme(QStringLiteral("light"));
}

void tst_theme::lightColorsAreTheOnesTheCallSitesUsedToHold_data(){
    QTest::addColumn<int>("role");
    QTest::addColumn<QColor>("expected");

#define ROW(role, r, g, b, a) \
    QTest::newRow(#role) << static_cast<int>(Theme::role) << QColor(r, g, b, a)

    ROW(Transparent,                          0,   0,   0,   0);
    ROW(TransparentWhite,                   255, 255, 255,   0);

    ROW(BarBackground,                      240, 240, 240, 255);
    ROW(BarBackgroundGhost,                 240, 240, 240,   1);
    ROW(BarBackgroundTranslucent,             0,   0,   0,   1);
    ROW(BarBorder,                          150, 150, 150, 255);
    ROW(BarButtonHovered,                   180, 180, 180, 255);
    ROW(BarButtonPressed,                   150, 150, 150, 255);
    ROW(BarButtonBackdrop,                  210, 210, 210, 255);
    ROW(BarIcon,                              0,   0,   0, 255);
    ROW(BarScrollFade,                      240, 240, 240, 255);
    ROW(BarScrollFadeHovered,               210, 210, 210, 255);
    ROW(BarScrollFadePressed,               128, 128, 128, 255);
    ROW(BarScrollerHandle,                    0,   0,   0,  50);
    ROW(BarScrollerHandleHovered,             0,   0,   0,  80);
    ROW(BarScrollerHandlePressed,             0,   0,   0, 110);
    ROW(BarInsertMarker,                      0,   0,   0,  48);
    ROW(BarTitleText,                         0,   0,   0, 255);
    ROW(BarTitleTextContrast,               255, 255, 255, 255);
    ROW(BarIconBackdrop,                    255, 255, 255, 255);
    ROW(BarPlaceholderDirectory,            217, 217, 217, 255);
    ROW(BarPlaceholderPage,                 225, 225, 225, 255);
    ROW(BarPlaceholderText,                 100, 100, 100, 255);
    ROW(BarTabTranslucent,                  255, 255, 255, 185);
    ROW(BarTabTranslucentHovered,           255, 255, 255, 215);
    ROW(BarTabTranslucentFocused,             0,   0,   0, 175);
    ROW(BarTabTranslucentFocusedHovered,      0,   0,   0, 205);
    ROW(BarTabHoverOverlay,                   0,   0,   0,  20);
    ROW(BarTabHoverOverlayTranslucent,        0,   0,   0,  10);
    ROW(BarTabHoverOverlayTranslucentFocused, 255, 255, 255, 50);
    ROW(BarTabCurrentBackground,             224, 237, 255, 255);
    ROW(BarTabCurrentAccent,                  23, 100, 192, 255);

    ROW(TitleBarBackground,                 255, 255, 255, 200);
    ROW(TitleBarBorder,                     255, 255, 255, 200);
    ROW(TitleBarButton,                       0,   0,   0, 255);
    ROW(TitleBarButtonHovered,                0,   0,   0, 127);
    ROW(TitleBarText,                         0,   0,   0, 255);
    ROW(TitleBarIcon,                         0,   0,   0, 255);
    ROW(WindowEdgeGhost,                      0,   0,   0,   1);
    ROW(WindowEdgeLine,                       0,   0,   0, 255);

    ROW(GlassOverlayBackground,               0,   0,   0, 170);
    ROW(GlassThumbLoaded,                   255, 255, 200,  44);
    ROW(GlassThumbPrimary,  23, 100, 192,  77);
    ROW(GlassThumbHovered,                  255, 255, 255,  77);
    ROW(GlassThumbSelected,                 255, 200, 220, 170);
    ROW(GlassSpotLightLoaded,               255, 255, 200,  44);
    ROW(GlassSpotLightLoadedEdge,           255, 255, 200,  30);
    ROW(GlassSpotLightPrimary,  23, 100, 192,  77);
    ROW(GlassSpotLightPrimaryEdge,  23, 100, 192,  50);
    ROW(GlassSpotLightHovered,              255, 255, 255,  77);
    ROW(GlassSpotLightHoveredEdge,          255, 255, 255,  50);
    ROW(GlassBorder,                        255, 255, 255, 255);
    ROW(GlassText,                          255, 255, 255, 255);
    ROW(GlassPlaceholderDirectory,           72,  72,  72, 150);
    ROW(GlassPlaceholderPage,                80,  80,  80, 150);
    ROW(GlassInPlaceBackground,               0,   0,   0, 128);
    ROW(GlassButtonBackground,              255, 255, 255, 255);
    ROW(GlassButtonBackgroundSoft,          255, 255, 255, 200);
    ROW(GlassAccessKeyChip,                   0,   0,   0, 100);
    ROW(GlassAccessKeyText,                 255, 255, 255, 200);
    ROW(GlassAccessKeyInfoBackground,         0,   0,   0, 100);
    ROW(GlassAccessKeyInfoText,             255, 255, 255, 200);
    ROW(GlassSelectRectBorder,              255, 255, 255, 255);
    ROW(GlassSelectRectFill,                255, 255, 255,  50);

    ROW(FlatOverlayBackground,              255, 255, 255, 170);
    ROW(FlatBorder,                         255, 255, 255, 255);
    ROW(FlatText,                           100, 100, 100, 255);
    ROW(FlatTextStrong,                       0,   0,   0, 255);
    ROW(FlatButtonBackground,               255, 255, 255, 255);
    ROW(FlatTitleBackground,                255, 255, 255, 200);
    ROW(FlatPlaceholderDirectory,           217, 217, 217, 255);
    ROW(FlatPlaceholderPage,                225, 225, 225, 255);
    ROW(FlatSelectedBorder,                 100, 100, 255, 255);
    ROW(FlatSelectedFill,                   100, 100, 255,  50);
    ROW(FlatPrimaryBorder,  23, 100, 192, 255);
    ROW(FlatAccessKeyChip,                  255, 255, 255, 255);
    ROW(FlatShadowPrimary,  23, 100, 192, 255);
    ROW(FlatShadow,                         120, 120, 120, 255);
    ROW(FlatShadowSoft,                       0,   0,   0, 127);
    ROW(FlatShadowLight,                    255, 255, 255, 255);
    ROW(FlatSelectRectBorder,               100, 100, 255, 255);
    ROW(FlatSelectRectFill,                 100, 100, 255,  50);

    ROW(GadgetsScrollIndicator,             255, 255, 255, 100);
    ROW(GadgetsScrollIndicatorSelected,     200, 200, 255, 100);
    ROW(GadgetsScrollIndicatorBorder,       255, 255, 255, 255);
    ROW(GadgetsAccessKeyLabelText,          255, 255, 255, 200);
    ROW(GadgetsAccessKeyLabelBorder,        255, 255, 255, 100);

    ROW(NotifierBackground,                 240, 240, 240, 180);
    ROW(NotifierText,                        20,  20,  20, 220);
    ROW(NotifierTextStrong,                   0,   0,   0, 255);
    ROW(NotifierDownloadBar,                100, 120, 255, 200);
    ROW(NotifierUploadBar,                  200, 120, 100, 200);
    ROW(NotifierBarBorder,                    0,   0,   0, 128);
    ROW(NotifierCancelItemHovered,          180, 180, 180, 255);
    ROW(NotifierCancelHovered,              220, 220, 220, 255);
    ROW(NotifierCancelPressed,              255, 255, 255, 255);
    ROW(NotifierScrollBorder,                 0,   0,   0, 128);
    ROW(NotifierScrollMarker,                 0,   0,   0, 200);
    ROW(NotifierScrollLine,                   0,   0,   0, 100);

    ROW(ReceiverBackground,                 240, 240, 240, 180);
    ROW(ReceiverBackgroundContrast,          20,  20,  20, 200);
    ROW(ReceiverText,                        16,  16,  16, 255);
    ROW(ReceiverTextContrast,               240, 240, 240, 255);

    ROW(LineEditBackgroundActive,           255, 255, 255, 255);
    ROW(LineEditBorder,                     240, 240, 240, 255);
    ROW(LineEditBorderActive,               192, 192, 192, 255);

    ROW(PageBackground,                     255, 255, 255, 255);

    ROW(DialogShadow,                       240, 240, 240, 150);
    ROW(DialogBorder,                       150, 150, 150, 255);
    ROW(DialogText,                           0,   0,   0, 255);

    ROW(PreviewBackground,                  250, 250, 250, 255);
    ROW(PreviewBorder,                      150, 150, 150, 255);
    ROW(PreviewText,                         28,  28,  28, 255);
    ROW(PreviewPlaceholderDirectory,        217, 217, 217, 255);
    ROW(PreviewPlaceholderPage,             225, 225, 225, 255);

    ROW(VanillaPageBackground,              255, 255, 255, 255);
    ROW(VanillaPageBackgroundSunken,        244, 244, 244, 255);
    ROW(VanillaPageText,                     28,  28,  28, 255);
    ROW(VanillaPageTextDim,                 102, 102, 102, 255);
    ROW(VanillaPageBorder,                  220, 220, 220, 255);
    ROW(VanillaPageAccent,                   42,  99, 208, 255);
    ROW(VanillaPageAccentText,              255, 255, 255, 255);
    ROW(VanillaPageFieldBackground,         255, 255, 255, 255);
    ROW(VanillaPageInvalidBorder,           208,  74,  74, 255);
    ROW(VanillaPageToastBackground,          28,  28,  28, 255);
    ROW(VanillaPageToastText,               255, 255, 255, 255);

#undef ROW
}

void tst_theme::lightColorsAreTheOnesTheCallSitesUsedToHold(){
    QFETCH(int, role);
    QFETCH(QColor, expected);

    QCOMPARE(Theme::Color(static_cast<Theme::Role>(role)), expected);
}

void tst_theme::darkColorsAreTheOnesTheDarkPaletteMeans_data(){
    QTest::addColumn<int>("role");
    QTest::addColumn<QColor>("expected");

#define ROW(role, r, g, b, a) \
    QTest::newRow(#role) << static_cast<int>(Theme::role) << QColor(r, g, b, a)

    ROW(Transparent,                          0,   0,   0,   0);
    ROW(TransparentWhite,                   255, 255, 255,   0);

    ROW(BarBackground,                       32,  32,  32, 255);
    ROW(BarBackgroundGhost,                  32,  32,  32,   1);
    ROW(BarBackgroundTranslucent,             0,   0,   0,   1);
    ROW(BarBorder,                           70,  70,  70, 255);
    ROW(BarButtonHovered,                    58,  58,  58, 255);
    ROW(BarButtonPressed,                    80,  80,  80, 255);
    ROW(BarButtonBackdrop,                   96,  96,  96, 255);
    ROW(BarIcon,                            230, 230, 230, 255);
    ROW(BarScrollFade,                       32,  32,  32, 255);
    ROW(BarScrollFadeHovered,                58,  58,  58, 255);
    ROW(BarScrollFadePressed,                96,  96,  96, 255);
    ROW(BarScrollerHandle,                  255, 255, 255,  50);
    ROW(BarScrollerHandleHovered,           255, 255, 255,  80);
    ROW(BarScrollerHandlePressed,           255, 255, 255, 110);
    ROW(BarInsertMarker,                    255, 255, 255,  64);
    ROW(BarTitleText,                       230, 230, 230, 255);
    ROW(BarTitleTextContrast,                24,  24,  24, 255);
    ROW(BarIconBackdrop,                    255, 255, 255, 255);
    ROW(BarPlaceholderDirectory,             56,  56,  56, 255);
    ROW(BarPlaceholderPage,                  64,  64,  64, 255);
    ROW(BarPlaceholderText,                 170, 170, 170, 255);
    ROW(BarTabTranslucent,                   32,  32,  32, 185);
    ROW(BarTabTranslucentHovered,            56,  56,  56, 215);
    ROW(BarTabTranslucentFocused,           235, 235, 235, 175);
    ROW(BarTabTranslucentFocusedHovered,    255, 255, 255, 205);
    ROW(BarTabHoverOverlay,                 255, 255, 255,  20);
    ROW(BarTabHoverOverlayTranslucent,      255, 255, 255,  10);
    ROW(BarTabHoverOverlayTranslucentFocused, 0,   0,   0,  50);
    ROW(BarTabCurrentBackground,              38,  62,  89, 255);
    ROW(BarTabCurrentAccent,                 123, 183, 255, 255);

    ROW(TitleBarBackground,                  40,  40,  40, 200);
    ROW(TitleBarBorder,                      40,  40,  40, 200);
    ROW(TitleBarButton,                     230, 230, 230, 255);
    ROW(TitleBarButtonHovered,              255, 255, 255,  60);
    ROW(TitleBarText,                       230, 230, 230, 255);
    ROW(TitleBarIcon,                       230, 230, 230, 255);
    ROW(WindowEdgeGhost,                      0,   0,   0,   1);
    ROW(WindowEdgeLine,                     230, 230, 230, 255);

    ROW(GlassOverlayBackground,               0,   0,   0, 190);
    ROW(GlassThumbLoaded,                   255, 255, 200,  44);
    ROW(GlassThumbPrimary, 123, 183, 255,  90);
    ROW(GlassThumbHovered,                  255, 255, 255,  60);
    ROW(GlassThumbSelected,                 255, 170, 200, 150);
    ROW(GlassSpotLightLoaded,               255, 255, 200,  44);
    ROW(GlassSpotLightLoadedEdge,           255, 255, 200,  30);
    ROW(GlassSpotLightPrimary, 123, 183, 255,  90);
    ROW(GlassSpotLightPrimaryEdge, 123, 183, 255,  60);
    ROW(GlassSpotLightHovered,              255, 255, 255,  60);
    ROW(GlassSpotLightHoveredEdge,          255, 255, 255,  40);
    ROW(GlassBorder,                        255, 255, 255, 255);
    ROW(GlassText,                          240, 240, 240, 255);
    ROW(GlassPlaceholderDirectory,           56,  56,  56, 180);
    ROW(GlassPlaceholderPage,                64,  64,  64, 180);
    ROW(GlassInPlaceBackground,               0,   0,   0, 190);
    ROW(GlassButtonBackground,              255, 255, 255, 255);
    ROW(GlassButtonBackgroundSoft,          255, 255, 255, 200);
    ROW(GlassAccessKeyChip,                   0,   0,   0, 140);
    ROW(GlassAccessKeyText,                 255, 255, 255, 220);
    ROW(GlassAccessKeyInfoBackground,         0,   0,   0, 140);
    ROW(GlassAccessKeyInfoText,             255, 255, 255, 220);
    ROW(GlassSelectRectBorder,              255, 255, 255, 255);
    ROW(GlassSelectRectFill,                255, 255, 255,  50);

    ROW(FlatOverlayBackground,               24,  24,  24, 190);
    ROW(FlatBorder,                          70,  70,  70, 255);
    ROW(FlatText,                           215, 215, 215, 255);
    ROW(FlatTextStrong,                     255, 255, 255, 255);
    ROW(FlatButtonBackground,               255, 255, 255, 255);
    ROW(FlatTitleBackground,                 40,  40,  40, 220);
    ROW(FlatPlaceholderDirectory,            56,  56,  56, 255);
    ROW(FlatPlaceholderPage,                 64,  64,  64, 255);
    ROW(FlatSelectedBorder,                 120, 150, 255, 255);
    ROW(FlatSelectedFill,                   120, 150, 255,  60);
    ROW(FlatPrimaryBorder, 123, 183, 255, 255);
    ROW(FlatAccessKeyChip,                   32,  32,  32, 255);
    ROW(FlatShadowPrimary, 123, 183, 255, 255);
    ROW(FlatShadow,                         140, 140, 140, 255);
    ROW(FlatShadowSoft,                       0,   0,   0, 127);
    ROW(FlatShadowLight,                    255, 255, 255, 255);
    ROW(FlatSelectRectBorder,               120, 150, 255, 255);
    ROW(FlatSelectRectFill,                 120, 150, 255,  60);

    ROW(GadgetsScrollIndicator,             255, 255, 255, 100);
    ROW(GadgetsScrollIndicatorSelected,     200, 200, 255, 120);
    ROW(GadgetsScrollIndicatorBorder,       255, 255, 255, 255);
    ROW(GadgetsAccessKeyLabelText,          255, 255, 255, 220);
    ROW(GadgetsAccessKeyLabelBorder,        255, 255, 255, 120);

    ROW(NotifierBackground,                  32,  32,  32, 180);
    ROW(NotifierText,                       235, 235, 235, 220);
    ROW(NotifierTextStrong,                 255, 255, 255, 255);
    ROW(NotifierDownloadBar,                110, 140, 255, 220);
    ROW(NotifierUploadBar,                  215, 130, 110, 220);
    ROW(NotifierBarBorder,                  255, 255, 255, 140);
    ROW(NotifierCancelItemHovered,          180, 180, 180, 255);
    ROW(NotifierCancelHovered,              220, 220, 220, 255);
    ROW(NotifierCancelPressed,              255, 255, 255, 255);
    ROW(NotifierScrollBorder,               255, 255, 255, 140);
    ROW(NotifierScrollMarker,               255, 255, 255, 220);
    ROW(NotifierScrollLine,                 255, 255, 255, 110);

    ROW(ReceiverBackground,                  32,  32,  32, 180);
    ROW(ReceiverBackgroundContrast,         235, 235, 235, 200);
    ROW(ReceiverText,                       240, 240, 240, 255);
    ROW(ReceiverTextContrast,                16,  16,  16, 255);

    ROW(LineEditBackgroundActive,            64,  64,  64, 255);
    ROW(LineEditBorder,                      48,  48,  48, 255);
    ROW(LineEditBorderActive,                96,  96,  96, 255);

    ROW(PageBackground,                      32,  32,  32, 255);

    ROW(DialogShadow,                        32,  32,  32, 180);
    ROW(DialogBorder,                        90,  90,  90, 255);
    ROW(DialogText,                         255, 255, 255, 255);

    ROW(PreviewBackground,                   44,  44,  44, 255);
    ROW(PreviewBorder,                       90,  90,  90, 255);
    ROW(PreviewText,                        230, 230, 230, 255);
    ROW(PreviewPlaceholderDirectory,         56,  56,  56, 255);
    ROW(PreviewPlaceholderPage,              64,  64,  64, 255);

    ROW(VanillaPageBackground,               32,  32,  32, 255);
    ROW(VanillaPageBackgroundSunken,         25,  25,  25, 255);
    ROW(VanillaPageText,                    230, 230, 230, 255);
    ROW(VanillaPageTextDim,                 160, 160, 160, 255);
    ROW(VanillaPageBorder,                   58,  58,  58, 255);
    ROW(VanillaPageAccent,                  122, 162, 247, 255);
    ROW(VanillaPageAccentText,               25,  25,  25, 255);
    ROW(VanillaPageFieldBackground,          42,  42,  42, 255);
    ROW(VanillaPageInvalidBorder,           208,  74,  74, 255);
    ROW(VanillaPageToastBackground,         230, 230, 230, 255);
    ROW(VanillaPageToastText,                25,  25,  25, 255);

#undef ROW
}

void tst_theme::darkColorsAreTheOnesTheDarkPaletteMeans(){
    QFETCH(int, role);
    QFETCH(QColor, expected);

    Theme::ApplyScheme(QStringLiteral("dark"));
    QCOMPARE(Theme::Color(static_cast<Theme::Role>(role)), expected);
}

void tst_theme::everyRoleHasAValue_data(){
    QTest::addColumn<QString>("scheme");
    QTest::newRow("light") << QStringLiteral("light");
    QTest::newRow("dark")  << QStringLiteral("dark");
}

void tst_theme::everyRoleHasAValue(){
    QFETCH(QString, scheme);
    Theme::ApplyScheme(scheme);

    for(int i = 0; i < Theme::RoleCount; i++){
        const Theme::Role role = static_cast<Theme::Role>(i);
        const QColor color = Theme::Color(role);
        QVERIFY2(color.isValid(), qPrintable(QStringLiteral("role %1 has no colour").arg(i)));
        if(role != Theme::Transparent)
            QVERIFY2(color != QColor(0, 0, 0, 0),
                     qPrintable(QStringLiteral("role %1 is transparent black; a row is probably missing").arg(i)));
    }

    QCOMPARE(static_cast<int>(Theme::RoleCount), 134);
}

void tst_theme::schemeSurvivesThePropertiesThatMatter(){

    struct { Theme::Role role; const char *why; } lightInBothSchemes[] = {
        { Theme::GlassButtonBackground,     "gadgets buttons" },
        { Theme::GlassButtonBackgroundSoft, "gadgets up and trash buttons" },
        { Theme::FlatButtonBackground,      "flat gadgets buttons" },
        { Theme::NotifierCancelPressed,     "the notifier's cancel button" },
        { Theme::NotifierCancelHovered,     "the notifier's cancel button" },
        { Theme::NotifierCancelItemHovered, "the notifier's cancel button" },
        { Theme::BarIconBackdrop,           "the backdrop behind a favicon" },
    };

    const int readable = 160;

    for(unsigned i = 0; i < sizeof(lightInBothSchemes)/sizeof(lightInBothSchemes[0]); i++){
        const Theme::Role role = lightInBothSchemes[i].role;
        Theme::ApplyScheme(QStringLiteral("light"));
        const QColor light = Theme::Color(role);
        Theme::ApplyScheme(QStringLiteral("dark"));
        const QColor dark = Theme::Color(role);
        QVERIFY2(light.lightness() >= readable && dark.lightness() >= readable,
                 lightInBothSchemes[i].why);
    }

    struct { Theme::Role ink, surface; } contrasts[] = {
        { Theme::BarTitleText,     Theme::BarBackground },
        { Theme::BarPlaceholderText, Theme::BarPlaceholderPage },
        { Theme::FlatText,         Theme::FlatOverlayBackground },
        { Theme::FlatText,         Theme::FlatPlaceholderPage },
        { Theme::GlassText,        Theme::GlassOverlayBackground },
        { Theme::GlassText,        Theme::GlassPlaceholderPage },
        { Theme::PreviewText,      Theme::PreviewPlaceholderPage },
        { Theme::ReceiverText,     Theme::ReceiverBackground },
        { Theme::DialogText,       Theme::DialogShadow },
        { Theme::NotifierText,     Theme::NotifierBackground },
        { Theme::TitleBarText,     Theme::TitleBarBackground },
        { Theme::VanillaPageAccentText, Theme::VanillaPageAccent },
    };

    const QString schemes[] = { QStringLiteral("light"), QStringLiteral("dark") };
    for(int s = 0; s < 2; s++){
        Theme::ApplyScheme(schemes[s]);
        for(Theme::Role role : {Theme::GlassThumbPrimary, Theme::GlassSpotLightPrimary,
                               Theme::GlassSpotLightPrimaryEdge, Theme::FlatPrimaryBorder,
                               Theme::FlatShadowPrimary}){
            QCOMPARE(Theme::Color(role).rgb(), Theme::Color(Theme::BarTabCurrentAccent).rgb());
        }
        for(unsigned i = 0; i < sizeof(contrasts)/sizeof(contrasts[0]); i++){
            const int ink = Theme::Color(contrasts[i].ink).lightness();
            const int surface = Theme::Color(contrasts[i].surface).lightness();
            QVERIFY2(qAbs(ink - surface) > 60,
                     qPrintable(QStringLiteral("%1: role %2 on role %3 is only %4 apart")
                                .arg(schemes[s])
                                .arg(static_cast<int>(contrasts[i].ink))
                                .arg(static_cast<int>(contrasts[i].surface))
                                .arg(qAbs(ink - surface))));
        }
        const int frameAlpha = Theme::Color(Theme::MiniMapFrame).alpha();
        QVERIFY(frameAlpha > 0);
        QVERIFY(frameAlpha < Theme::Color(Theme::MiniMapMedia).alpha());
        const int controlAlpha = Theme::Color(Theme::MiniMapControl).alpha();
        QVERIFY(controlAlpha > frameAlpha);
        const int textAlpha = Theme::Color(Theme::MiniMapText).alpha();
        QVERIFY(controlAlpha * 10 >= textAlpha * 7);
        QVERIFY(controlAlpha * 4 <= textAlpha * 3);
        const QColor positioned =
            Theme::Color(Theme::MiniMapPositionedBackground);
        QVERIFY(positioned.alpha() > 0);
        QVERIFY(positioned.hsvHue() >= 90);
        QVERIFY(positioned.hsvHue() <= 170);
    }

    Theme::ApplyScheme(QStringLiteral("light"));
    QVERIFY(Theme::Color(Theme::BarBackground).lightness() > 180);
    QVERIFY(Theme::Color(Theme::BarIcon).lightness() < 80);
    QVERIFY(!Theme::IsDark());

    Theme::ApplyScheme(QStringLiteral("dark"));
    QVERIFY(Theme::Color(Theme::BarBackground).lightness() < 80);
    QVERIFY(Theme::Color(Theme::BarIcon).lightness() > 180);
    QVERIFY(Theme::IsDark());

    Theme::ApplyScheme(QStringLiteral("light"));
    QVERIFY(Theme::Color(Theme::BarButtonHovered).lightness()
            < Theme::Color(Theme::BarBackground).lightness());
    Theme::ApplyScheme(QStringLiteral("dark"));
    QVERIFY(Theme::Color(Theme::BarButtonHovered).lightness()
            > Theme::Color(Theme::BarBackground).lightness());
}

void tst_theme::applySchemeReadsTheSetting_data(){
    QTest::addColumn<QString>("setting");
    QTest::addColumn<int>("expected");

    QTest::newRow("light")  << QStringLiteral("light") << static_cast<int>(Theme::Light);
    QTest::newRow("dark")   << QStringLiteral("dark")  << static_cast<int>(Theme::Dark);
    QTest::newRow("Dark")   << QStringLiteral("Dark")  << static_cast<int>(Theme::Dark);
    QTest::newRow("padded") << QStringLiteral("  dark  ") << static_cast<int>(Theme::Dark);
}

void tst_theme::applySchemeReadsTheSetting(){
    QFETCH(QString, setting);
    QFETCH(int, expected);

    Theme::ApplyScheme(setting);
    QCOMPARE(static_cast<int>(Theme::CurrentScheme()), expected);
}

void tst_theme::applySchemeOnlyReportsARealChange(){
    QVERIFY(Theme::ApplyScheme(QStringLiteral("dark")));
    QVERIFY(!Theme::ApplyScheme(QStringLiteral("dark")));
    QVERIFY(Theme::ApplyScheme(QStringLiteral("light")));
    QVERIFY(!Theme::ApplyScheme(QStringLiteral("light")));

    Theme::ApplyScheme(QStringLiteral("nonsense"));
    QVERIFY(Theme::CurrentScheme() == Theme::Light ||
            Theme::CurrentScheme() == Theme::Dark);
}

void tst_theme::alphaOverloadKeepsTheColorAndReplacesTheAlpha(){
    const QColor base = Theme::Color(Theme::GlassThumbPrimary);
    const QColor faded = Theme::Color(Theme::GlassThumbPrimary, 0);

    QCOMPARE(faded.red(),   base.red());
    QCOMPARE(faded.green(), base.green());
    QCOMPARE(faded.blue(),  base.blue());
    QCOMPARE(faded.alpha(), 0);

    QCOMPARE(Theme::Color(Theme::TitleBarBackground, 128), QColor(255, 255, 255, 128));
}

void tst_theme::brushAndPenCarryTheColor(){
    QCOMPARE(Theme::Brush(Theme::BarBackground).color(), QColor(240, 240, 240, 255));
    QCOMPARE(Theme::Pen(Theme::BarBorder).color(), QColor(150, 150, 150, 255));
    QCOMPARE(Theme::Brush(Theme::BarButtonHovered, 100).color(), QColor(180, 180, 180, 100));
    QCOMPARE(Theme::Pen(Theme::GlassText, 200).color(), QColor(255, 255, 255, 200));
}

void tst_theme::anOutOfRangeRoleIsInvalidRatherThanACrash(){
    QVERIFY(!Theme::Color(static_cast<Theme::Role>(-1)).isValid());
    QVERIFY(!Theme::Color(Theme::RoleCount).isValid());
}

namespace {
    QPixmap Glyph(){
        QImage image(2, 1, QImage::Format_ARGB32);
        image.setPixelColor(0, 0, QColor(40, 30, 20, 255));
        image.setPixelColor(1, 0, QColor(0, 0, 0, 0));
        return QPixmap::fromImage(image);
    }
}

void tst_theme::inkLeavesTheLightPaletteAlone(){
    const QImage before = Glyph().toImage();
    const QImage after = Theme::Ink(Glyph(), Theme::BarIcon).toImage();

    QCOMPARE(after.pixelColor(0, 0), before.pixelColor(0, 0));
    QCOMPARE(after.pixelColor(0, 0), QColor(40, 30, 20, 255));
}

void tst_theme::inkRepaintsForTheDarkPalette(){
    Theme::ApplyScheme(QStringLiteral("dark"));

    const QImage after = Theme::Ink(Glyph(), Theme::BarIcon).toImage();

    QCOMPARE(after.pixelColor(0, 0), Theme::Color(Theme::BarIcon));
    QCOMPARE(after.pixelColor(1, 0).alpha(), 0);
}

void tst_theme::fillRepaintsForEitherPalette(){
    const QImage light = Theme::Fill(Glyph(), Theme::PreviewBackground).toImage();
    QVERIFY(!Theme::IsDark());
    QCOMPARE(light.pixelColor(0, 0), Theme::Color(Theme::PreviewBackground));
    QVERIFY(light.pixelColor(0, 0) != QColor(40, 30, 20, 255));
    QCOMPARE(light.pixelColor(1, 0).alpha(), 0);

    Theme::ApplyScheme(QStringLiteral("dark"));
    const QImage dark = Theme::Fill(Glyph(), Theme::PreviewBackground).toImage();
    QCOMPARE(dark.pixelColor(0, 0), Theme::Color(Theme::PreviewBackground));
    QVERIFY(dark.pixelColor(0, 0) != light.pixelColor(0, 0));
    QCOMPARE(dark.pixelColor(1, 0).alpha(), 0);
}

void tst_theme::pixmapCacheFollowsTheScheme(){
    const QString path = QStringLiteral(":/does/not/exist.png");

    QVERIFY(Theme::Pixmap(path, Theme::BarIcon).isNull());

    Theme::ApplyScheme(QStringLiteral("dark"));
    QVERIFY(Theme::Pixmap(path, Theme::BarIcon).isNull());
    Theme::ApplyScheme(QStringLiteral("light"));

    const QPixmap &first = Theme::Pixmap(path, Theme::BarIcon);
    const QPixmap &second = Theme::Pixmap(path, Theme::BarIcon);
    QCOMPARE(&first, &second);
}

void tst_theme::emptyThumbnailFillsTheBoxAndSaysWhichKindItIs(){
    const QSize size(400, 300);

    const QImage page = Drawn(size, Theme::FlatPlaceholderPage, false);
    const QImage folder = Drawn(size, Theme::FlatPlaceholderDirectory, true);

    QCOMPARE(page.pixelColor(0, 0), Theme::Color(Theme::FlatPlaceholderPage));
    QCOMPARE(page.pixelColor(size.width() - 1, size.height() - 1),
             Theme::Color(Theme::FlatPlaceholderPage));
    QCOMPARE(folder.pixelColor(0, 0), Theme::Color(Theme::FlatPlaceholderDirectory));

    QVERIFY(InkPixels(page, Theme::Color(Theme::FlatPlaceholderPage)) > 0);
    QVERIFY(InkPixels(folder, Theme::Color(Theme::FlatPlaceholderDirectory)) > 0);

    QVERIFY(Drawn(size, Theme::FlatPlaceholderPage, true)
            != Drawn(size, Theme::FlatPlaceholderPage, false));
}

void tst_theme::emptyThumbnailLeavesOutACaptionThatWouldNotFit(){
    const QSize size(12, 8);
    const QImage image = Drawn(size, Theme::FlatPlaceholderPage, false);

    QCOMPARE(InkPixels(image, Theme::Color(Theme::FlatPlaceholderPage)), 0);
}

QTEST_MAIN(tst_theme)
#include "tst_theme.moc"
