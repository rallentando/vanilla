#ifndef THEME_HPP
#define THEME_HPP

#include "switch.hpp"

#include <QColor>
#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QByteArray>

class QPainter;

namespace Theme {

enum Role {

    Transparent,
    TransparentWhite,

    BarBackground,
    BarBackgroundGhost,
    BarBackgroundTranslucent,
    BarBorder,
    BarButtonHovered,
    BarButtonPressed,
    BarButtonBackdrop,
    BarIcon,
    BarScrollFade,
    BarScrollFadeHovered,
    BarScrollFadePressed,
    BarScrollerHandle,
    BarScrollerHandleHovered,
    BarScrollerHandlePressed,
    BarInsertMarker,
    BarTitleText,
    BarTitleTextContrast,
    BarIconBackdrop,
    BarPlaceholderDirectory,
    BarPlaceholderPage,
    BarPlaceholderText,
    BarTabTranslucent,
    BarTabTranslucentHovered,
    BarTabTranslucentFocused,
    BarTabTranslucentFocusedHovered,
    BarTabHoverOverlay,
    BarTabHoverOverlayTranslucent,
    BarTabHoverOverlayTranslucentFocused,

    TitleBarBackground,
    TitleBarBorder,
    TitleBarButton,
    TitleBarButtonHovered,
    TitleBarText,
    TitleBarIcon,
    WindowEdgeGhost,
    WindowEdgeLine,

    GlassOverlayBackground,
    GlassThumbLoaded,
    GlassThumbPrimary,
    GlassThumbHovered,
    GlassThumbSelected,
    GlassSpotLightLoaded,
    GlassSpotLightLoadedEdge,
    GlassSpotLightPrimary,
    GlassSpotLightPrimaryEdge,
    GlassSpotLightHovered,
    GlassSpotLightHoveredEdge,
    GlassBorder,
    GlassText,
    GlassPlaceholderDirectory,
    GlassPlaceholderPage,
    GlassInPlaceBackground,
    GlassButtonBackground,
    GlassButtonBackgroundSoft,
    GlassAccessKeyChip,
    GlassAccessKeyText,
    GlassAccessKeyInfoBackground,
    GlassAccessKeyInfoText,
    GlassSelectRectBorder,
    GlassSelectRectFill,

    FlatOverlayBackground,
    FlatBorder,
    FlatText,
    FlatTextStrong,
    FlatButtonBackground,
    FlatTitleBackground,
    FlatPlaceholderDirectory,
    FlatPlaceholderPage,
    FlatSelectedBorder,
    FlatSelectedFill,
    FlatPrimaryBorder,
    FlatAccessKeyChip,
    FlatShadowPrimary,
    FlatShadow,
    FlatShadowSoft,
    FlatShadowLight,
    FlatSelectRectBorder,
    FlatSelectRectFill,

    GadgetsScrollIndicator,
    GadgetsScrollIndicatorSelected,
    GadgetsScrollIndicatorBorder,
    GadgetsAccessKeyLabelText,
    GadgetsAccessKeyLabelBorder,

    NotifierBackground,
    NotifierText,
    NotifierTextStrong,
    NotifierDownloadBar,
    NotifierUploadBar,
    NotifierBarBorder,
    NotifierCancelItemHovered,
    NotifierCancelHovered,
    NotifierCancelPressed,
    NotifierScrollBorder,
    NotifierScrollMarker,
    NotifierScrollLine,

    MiniMapBackground,
    MiniMapViewport,
    MiniMapViewportBorder,
    MiniMapText,
    MiniMapMedia,

    ReceiverBackground,
    ReceiverBackgroundContrast,
    ReceiverText,
    ReceiverTextContrast,

    LineEditBackgroundActive,
    LineEditBorder,
    LineEditBorderActive,

    PageBackground,

    DialogShadow,
    DialogBorder,
    DialogText,

    PreviewBackground,
    PreviewBorder,
    PreviewText,
    PreviewPlaceholderDirectory,
    PreviewPlaceholderPage,

    VanillaPageBackground,
    VanillaPageBackgroundSunken,
    VanillaPageText,
    VanillaPageTextDim,
    VanillaPageBorder,
    VanillaPageAccent,
    VanillaPageAccentText,
    VanillaPageFieldBackground,
    VanillaPageInvalidBorder,
    VanillaPageToastBackground,
    VanillaPageToastText,

    RoleCount
};

enum Scheme {
    Light,
    Dark
};

bool ApplyScheme(const QString &setting);

Scheme CurrentScheme();
bool IsDark();

bool SetScheme(Scheme scheme);

QColor Color(Role role);
QColor Color(Role role, int alpha);
QColor Color(Scheme scheme, Role role);

QString StyleSheetColor(Role role);

QByteArray PageColorVariables();

QByteArray PageFontVariables();

QByteArray PageMetricVariables();

QByteArray PageVariables();

inline QBrush Brush(Role role){ return QBrush(Color(role));}
inline QBrush Brush(Role role, int alpha){ return QBrush(Color(role, alpha));}
inline QPen   Pen(Role role){ return QPen(Color(role));}
inline QPen   Pen(Role role, int alpha){ return QPen(Color(role, alpha));}

QPixmap Ink(const QPixmap &pixmap, Role role);

const QPixmap &Pixmap(const QString &path, Role role);

QIcon Icon(const QString &path);

void DrawEmptyThumbnail(QPainter *painter, const QRectF &rect,
                        Role fill, Role text, bool isDirectory,
                        const QRectF &captionRect = QRectF());

}

#endif
