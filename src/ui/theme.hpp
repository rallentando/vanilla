#ifndef THEME_HPP
#define THEME_HPP

#include "switch.hpp"

#include <QColor>
#include <QBrush>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QRect>
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
    BarScrollThumb,
    BarScrollThumbActive,
    BarScrollThumbRim,
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
    BarTabCurrentBackground,
    BarTabCurrentAccent,

    TitleBarButtonHovered,
    TitleBarText,
    TitleBarCloseHovered,
    TitleBarCloseText,
    WindowEdgeGhost,
    WindowEdgeLine,

    MenuSeparator,

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
    GlassAccessKeyChip,
    GlassAccessKeyText,
    GlassAccessKeyInfoBackground,
    GlassAccessKeyInfoText,
    GlassSelectRectBorder,
    GlassSelectRectFill,
    GlassHeaderBackground,
    GlassHeaderButton,
    GlassHeaderButtonBorder,
    GlassHeaderButtonHovered,

    FlatOverlayBackground,
    FlatBorder,
    FlatText,
    FlatTextStrong,
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
    FlatHeaderBackground,
    FlatHeaderButton,
    FlatHeaderButtonBorder,
    FlatHeaderButtonHovered,

    PlainSurface,
    PlainHovered,
    PlainSelected,
    PlainText,
    PlainTextStrong,
    PlainAccent,
    PlainDivider,
    PlainField,
    PlainPlaceholderDirectory,
    PlainPlaceholderPage,
    PlainScrollThumb,
    PlainScrollThumbActive,

    GadgetsScrollIndicator,
    GadgetsScrollIndicatorSelected,
    GadgetsScrollIndicatorBorder,
    GadgetsAccessKeyLabelText,
    GadgetsAccessKeyLabelBorder,

    NotifierBackground,
    NotifierText,
    NotifierTextStrong,
    NotifierScrollBorder,
    NotifierScrollMarker,
    NotifierScrollLine,

    MiniMapBackground,
    MiniMapBackgroundTranslucent,
    MiniMapViewport,
    MiniMapViewportBorder,
    MiniMapText,
    MiniMapControl,
    MiniMapMedia,
    MiniMapFrame,
    MiniMapPositionedBackground,
    MiniMapMatch,

    ReceiverBackground,
    ReceiverBackgroundContrast,
    ReceiverText,
    ReceiverTextContrast,

    LineEditBackgroundActive,
    LineEditBorder,
    LineEditBorderActive,
    FieldButtonHovered,

    PageBackground,

    PreviewBackground,
    PreviewBackgroundTranslucent,
    PreviewBorder,
    PreviewText,

    VanillaPageBackground,
    VanillaPageBackgroundSunken,
    VanillaPageText,
    VanillaPageTextDim,
    VanillaPageBorder,
    VanillaPageAccent,
    VanillaPageAccentText,
    VanillaPageFieldBackground,
    VanillaPageRaised,
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

QPalette PanelPalette(const QPalette &base);

QPixmap Ink(const QPixmap &pixmap, Role role);

QPixmap Fill(const QPixmap &pixmap, Role role);

const QPixmap &Pixmap(const QString &path, Role role);

QIcon Icon(const QString &path);

void DrawEmptyThumbnail(QPainter *painter, const QRectF &rect,
                        Role fill, Role text, bool isDirectory,
                        const QRectF &captionRect = QRectF());

void DrawBarHandle(QPainter *painter, const QRect &handle,
                   Qt::Orientation bar, int thickness, int length);

}

#endif
