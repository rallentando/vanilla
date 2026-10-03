#include "switch.hpp"

#include "theme.hpp"

#include "devicescale.hpp"

#include "const.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QPainter>
#include <QFontMetricsF>
#include <QHash>
#include <QScreen>
#include <QFontDatabase>
#include <QFile>

namespace {

    const QColor LightPalette[] = {

        QColor(  0,   0,   0,   0),
        QColor(255, 255, 255,   0),

        QColor(240, 240, 240, 255),
        QColor(240, 240, 240,   1),
        QColor(  0,   0,   0,   1),
        QColor(150, 150, 150, 255),
        QColor(180, 180, 180, 255),
        QColor(150, 150, 150, 255),
        QColor(210, 210, 210, 255),
        QColor(  0,   0,   0, 255),
        QColor(240, 240, 240, 255),
        QColor(210, 210, 210, 255),
        QColor(128, 128, 128, 255),
        QColor(  0,   0,   0,  50),
        QColor(  0,   0,   0,  80),
        QColor(  0,   0,   0, 110),
        QColor(  0,   0,   0,  48),
        QColor(  0,   0,   0, 255),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 255),
        QColor(217, 217, 217, 255),
        QColor(225, 225, 225, 255),
        QColor(100, 100, 100, 255),
        QColor(255, 255, 255, 185),
        QColor(255, 255, 255, 215),
        QColor(  0,   0,   0, 175),
        QColor(  0,   0,   0, 205),
        QColor(  0,   0,   0,  20),
        QColor(  0,   0,   0,  10),
        QColor(255, 255, 255,  50),

        QColor(224, 237, 255, 255),
        QColor( 23, 100, 192, 255),

        QColor(255, 255, 255, 200),
        QColor(255, 255, 255, 200),
        QColor(  0,   0,   0, 255),
        QColor(  0,   0,   0, 127),
        QColor(  0,   0,   0, 255),
        QColor(  0,   0,   0, 255),
        QColor(  0,   0,   0,   1),
        QColor(  0,   0,   0, 255),

        QColor(  0,   0,   0, 170),
        QColor(255, 255, 200,  44),
        QColor( 23, 100, 192,  77),
        QColor(255, 255, 255,  77),
        QColor(255, 200, 220, 170),
        QColor(255, 255, 200,  44),
        QColor(255, 255, 200,  30),
        QColor( 23, 100, 192,  77),
        QColor( 23, 100, 192,  50),
        QColor(255, 255, 255,  77),
        QColor(255, 255, 255,  50),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 255),
        QColor( 72,  72,  72, 150),
        QColor( 80,  80,  80, 150),
        QColor(  0,   0,   0, 128),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 200),
        QColor(  0,   0,   0, 100),
        QColor(255, 255, 255, 200),
        QColor(  0,   0,   0, 100),
        QColor(255, 255, 255, 200),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255,  50),

        QColor(255, 255, 255, 170),
        QColor(255, 255, 255, 255),
        QColor(100, 100, 100, 255),
        QColor(  0,   0,   0, 255),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 200),
        QColor(217, 217, 217, 255),
        QColor(225, 225, 225, 255),
        QColor(100, 100, 255, 255),
        QColor(100, 100, 255,  50),
        QColor( 23, 100, 192, 255),
        QColor(255, 255, 255, 255),
        QColor( 23, 100, 192, 255),
        QColor(120, 120, 120, 255),
        QColor(  0,   0,   0, 127),
        QColor(255, 255, 255, 255),
        QColor(100, 100, 255, 255),
        QColor(100, 100, 255,  50),

        QColor(255, 255, 255, 100),
        QColor(200, 200, 255, 100),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 200),
        QColor(255, 255, 255, 100),

        QColor(240, 240, 240, 180),
        QColor( 20,  20,  20, 220),
        QColor(  0,   0,   0, 255),
        QColor(100, 120, 255, 200),
        QColor(200, 120, 100, 200),
        QColor(  0,   0,   0, 128),
        QColor(180, 180, 180, 255),
        QColor(220, 220, 220, 255),
        QColor(255, 255, 255, 255),
        QColor(  0,   0,   0, 128),
        QColor(  0,   0,   0, 200),
        QColor(  0,   0,   0, 100),

        QColor(245, 245, 245, 255),
        QColor(  0,   0,   0,  25),
        QColor(  0,   0,   0,  70),
        QColor(  0,   0,   0,  90),
        QColor(  0,   0,   0,  65),
        QColor( 70, 110, 160, 110),
        QColor( 70, 110, 160,  35),
        QColor( 80, 120,  80,  50),

        QColor(240, 240, 240, 180),
        QColor( 20,  20,  20, 200),
        QColor( 16,  16,  16, 255),
        QColor(240, 240, 240, 255),

        QColor(255, 255, 255, 255),
        QColor(240, 240, 240, 255),
        QColor(192, 192, 192, 255),

        QColor(255, 255, 255, 255),

        QColor(240, 240, 240, 150),
        QColor(150, 150, 150, 255),
        QColor(  0,   0,   0, 255),

        QColor(250, 250, 250, 255),
        QColor(150, 150, 150, 255),
        QColor( 28,  28,  28, 255),
        QColor(217, 217, 217, 255),
        QColor(225, 225, 225, 255),

        QColor(255, 255, 255, 255),
        QColor(244, 244, 244, 255),
        QColor( 28,  28,  28, 255),
        QColor(102, 102, 102, 255),
        QColor(220, 220, 220, 255),
        QColor( 42,  99, 208, 255),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 255),
        QColor(208,  74,  74, 255),
        QColor( 28,  28,  28, 255),
        QColor(255, 255, 255, 255),
    };

    static_assert(sizeof(LightPalette) / sizeof(LightPalette[0]) == Theme::RoleCount,
                  "the palette and the 'Role' enum have drifted apart.");

    const QColor DarkPalette[] = {

        QColor(  0,   0,   0,   0),
        QColor(255, 255, 255,   0),

        QColor( 32,  32,  32, 255),
        QColor( 32,  32,  32,   1),
        QColor(  0,   0,   0,   1),
        QColor( 70,  70,  70, 255),
        QColor( 58,  58,  58, 255),
        QColor( 80,  80,  80, 255),
        QColor( 96,  96,  96, 255),
        QColor(230, 230, 230, 255),
        QColor( 32,  32,  32, 255),
        QColor( 58,  58,  58, 255),
        QColor( 96,  96,  96, 255),
        QColor(255, 255, 255,  50),
        QColor(255, 255, 255,  80),
        QColor(255, 255, 255, 110),
        QColor(255, 255, 255,  64),
        QColor(230, 230, 230, 255),
        QColor( 24,  24,  24, 255),
        QColor(255, 255, 255, 255),
        QColor( 56,  56,  56, 255),
        QColor( 64,  64,  64, 255),
        QColor(170, 170, 170, 255),
        QColor( 32,  32,  32, 185),
        QColor( 56,  56,  56, 215),
        QColor(235, 235, 235, 175),
        QColor(255, 255, 255, 205),
        QColor(255, 255, 255,  20),
        QColor(255, 255, 255,  10),
        QColor(  0,   0,   0,  50),

        QColor( 38,  62,  89, 255),
        QColor(123, 183, 255, 255),

        QColor( 40,  40,  40, 200),
        QColor( 40,  40,  40, 200),
        QColor(230, 230, 230, 255),
        QColor(255, 255, 255,  60),
        QColor(230, 230, 230, 255),
        QColor(230, 230, 230, 255),
        QColor(  0,   0,   0,   1),
        QColor(230, 230, 230, 255),

        QColor(  0,   0,   0, 190),
        QColor(255, 255, 200,  44),
        QColor(123, 183, 255,  90),
        QColor(255, 255, 255,  60),
        QColor(255, 170, 200, 150),
        QColor(255, 255, 200,  44),
        QColor(255, 255, 200,  30),
        QColor(123, 183, 255,  90),
        QColor(123, 183, 255,  60),
        QColor(255, 255, 255,  60),
        QColor(255, 255, 255,  40),
        QColor(255, 255, 255, 255),
        QColor(240, 240, 240, 255),
        QColor( 56,  56,  56, 180),
        QColor( 64,  64,  64, 180),
        QColor(  0,   0,   0, 190),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 200),
        QColor(  0,   0,   0, 140),
        QColor(255, 255, 255, 220),
        QColor(  0,   0,   0, 140),
        QColor(255, 255, 255, 220),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255,  50),

        QColor( 24,  24,  24, 190),
        QColor( 70,  70,  70, 255),
        QColor(215, 215, 215, 255),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 255),
        QColor( 40,  40,  40, 220),
        QColor( 56,  56,  56, 255),
        QColor( 64,  64,  64, 255),
        QColor(120, 150, 255, 255),
        QColor(120, 150, 255,  60),
        QColor(123, 183, 255, 255),
        QColor( 32,  32,  32, 255),
        QColor(123, 183, 255, 255),
        QColor(140, 140, 140, 255),
        QColor(  0,   0,   0, 127),
        QColor(255, 255, 255, 255),
        QColor(120, 150, 255, 255),
        QColor(120, 150, 255,  60),

        QColor(255, 255, 255, 100),
        QColor(200, 200, 255, 120),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 220),
        QColor(255, 255, 255, 120),

        QColor( 32,  32,  32, 180),
        QColor(235, 235, 235, 220),
        QColor(255, 255, 255, 255),
        QColor(110, 140, 255, 220),
        QColor(215, 130, 110, 220),
        QColor(255, 255, 255, 140),
        QColor(180, 180, 180, 255),
        QColor(220, 220, 220, 255),
        QColor(255, 255, 255, 255),
        QColor(255, 255, 255, 140),
        QColor(255, 255, 255, 220),
        QColor(255, 255, 255, 110),

        QColor( 38,  38,  38, 255),
        QColor(255, 255, 255,  30),
        QColor(255, 255, 255,  80),
        QColor(255, 255, 255, 100),
        QColor(255, 255, 255,  70),
        QColor(120, 160, 210, 120),
        QColor(120, 160, 210,  40),
        QColor(120, 175, 120,  60),

        QColor( 32,  32,  32, 180),
        QColor(235, 235, 235, 200),
        QColor(240, 240, 240, 255),
        QColor( 16,  16,  16, 255),

        QColor( 64,  64,  64, 255),
        QColor( 48,  48,  48, 255),
        QColor( 96,  96,  96, 255),

        QColor( 32,  32,  32, 255),

        QColor( 32,  32,  32, 180),
        QColor( 90,  90,  90, 255),
        QColor(255, 255, 255, 255),

        QColor( 44,  44,  44, 255),
        QColor( 90,  90,  90, 255),
        QColor(230, 230, 230, 255),
        QColor( 56,  56,  56, 255),
        QColor( 64,  64,  64, 255),

        QColor( 32,  32,  32, 255),
        QColor( 25,  25,  25, 255),
        QColor(230, 230, 230, 255),
        QColor(160, 160, 160, 255),
        QColor( 58,  58,  58, 255),
        QColor(122, 162, 247, 255),
        QColor( 25,  25,  25, 255),
        QColor( 42,  42,  42, 255),
        QColor(208,  74,  74, 255),
        QColor(230, 230, 230, 255),
        QColor( 25,  25,  25, 255),
    };

    static_assert(sizeof(DarkPalette) / sizeof(DarkPalette[0]) == Theme::RoleCount,
                  "the dark palette and the 'Role' enum have drifted apart.");

    Theme::Scheme g_Scheme = Theme::Light;
    const QColor *g_Palette = LightPalette;
    QHash<QString, QPixmap> g_Pixmaps;

    const struct { const char *name; Theme::Role role; } PageVariables[] = {
        {"--bg",         Theme::VanillaPageBackground},
        {"--bg-sunken",  Theme::VanillaPageBackgroundSunken},
        {"--fg",         Theme::VanillaPageText},
        {"--fg-dim",     Theme::VanillaPageTextDim},
        {"--line",       Theme::VanillaPageBorder},
        {"--accent",     Theme::VanillaPageAccent},
        {"--accent-fg",  Theme::VanillaPageAccentText},
        {"--field-bg",   Theme::VanillaPageFieldBackground},
        {"--invalid",    Theme::VanillaPageInvalidBorder},
        {"--toast-bg",   Theme::VanillaPageToastBackground},
        {"--toast-fg",   Theme::VanillaPageToastText},
    };

    QByteArray CssColor(const QColor &color){
        if(color.alpha() == 255)
            return QStringLiteral("#%1%2%3")
                .arg(color.red(),   2, 16, QLatin1Char('0'))
                .arg(color.green(), 2, 16, QLatin1Char('0'))
                .arg(color.blue(),  2, 16, QLatin1Char('0')).toLatin1();
        return QStringLiteral("rgb(%1 %2 %3 / %4)")
            .arg(color.red()).arg(color.green()).arg(color.blue())
            .arg(color.alphaF(), 0, 'f', 3).toLatin1();
    }

    QByteArray CssBlock(Theme::Scheme scheme, const QByteArray &indent){
        QByteArray block;
        for(const auto &variable : PageVariables){
            block += indent + variable.name + ": "
                   + CssColor(Theme::Color(scheme, variable.role)) + ";\n";
        }
        return block;
    }

    Theme::Scheme DesktopScheme(){
        if(QStyleHints *hints = QGuiApplication::styleHints())
            return hints->colorScheme() == Qt::ColorScheme::Dark
                ? Theme::Dark : Theme::Light;
        return Theme::Light;
    }
}

namespace Theme {

Scheme CurrentScheme(){
    return g_Scheme;
}

bool IsDark(){
    return g_Scheme == Dark;
}

bool SetScheme(Scheme scheme){
    if(scheme == g_Scheme) return false;
    g_Scheme = scheme;
    g_Palette = scheme == Dark ? DarkPalette : LightPalette;
    g_Pixmaps.clear();
    return true;
}

bool ApplyScheme(const QString &setting){
    const QString value = setting.trimmed().toLower();
    const bool dark  = value == QStringLiteral("dark");
    const bool light = value == QStringLiteral("light");

    if(QStyleHints *hints = QGuiApplication::styleHints()){
        if(dark)       hints->setColorScheme(Qt::ColorScheme::Dark);
        else if(light) hints->setColorScheme(Qt::ColorScheme::Light);
        else           hints->unsetColorScheme();
    }

    return SetScheme(dark ? Dark : light ? Light : DesktopScheme());
}

QColor Color(Role role){
    if(role < 0 || role >= RoleCount) return QColor();
    return g_Palette[role];
}

QColor Color(Role role, int alpha){
    QColor color = Color(role);
    color.setAlpha(alpha);
    return color;
}

QColor Color(Scheme scheme, Role role){
    if(role < 0 || role >= RoleCount) return QColor();
    return (scheme == Dark ? DarkPalette : LightPalette)[role];
}

QString StyleSheetColor(Role role){
    return Color(role).name(QColor::HexRgb);
}

QByteArray PageFontVariables(){
    const QByteArray ui = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family().toUtf8();

    return QByteArrayLiteral(":root {\n")
         + QByteArrayLiteral("    --font-family: \"") + ui
         + QByteArrayLiteral("\", system-ui, sans-serif;\n")
         + QByteArrayLiteral("    --font-mono: ") + MONOSPACE_FAMILIES.toUtf8()
         + QByteArrayLiteral(";\n")
         + QByteArrayLiteral("}\n\n");
}

QByteArray PageMetricVariables(){
    const struct { const char *name; int px; } metrics[] = {
        {"--radius-small", CHIP_CORNER_RADIUS},
        {"--radius",       PAGE_FIELD_CORNER_RADIUS},
        {"--radius-pill",  PAGE_PILL_CORNER_RADIUS},
    };

    QByteArray css = QByteArrayLiteral(":root {\n");
    for(const auto &metric : metrics){
        css += QByteArrayLiteral("    ") + metric.name + QByteArrayLiteral(": ")
             + QByteArray::number(metric.px) + QByteArrayLiteral("px;\n");
    }
    css += QByteArrayLiteral("}\n\n");
    return css;
}

QByteArray PageVariables(){
    return PageColorVariables() + PageFontVariables() + PageMetricVariables();
}

QByteArray PageColorVariables(){
    QByteArray css = QByteArrayLiteral(":root {\n    color-scheme: light dark;\n\n");
    css += CssBlock(Light, QByteArrayLiteral("    "));
    css += QByteArrayLiteral("}\n\n@media (prefers-color-scheme: dark) {\n    :root {\n");
    css += CssBlock(Dark, QByteArrayLiteral("        "));
    css += QByteArrayLiteral("    }\n}\n\n");
    return css;
}

QPixmap Ink(const QPixmap &pixmap, Role role){
    if(!IsDark() || pixmap.isNull()) return pixmap;
    return Fill(pixmap, role);
}

QPixmap Fill(const QPixmap &pixmap, Role role){
    if(pixmap.isNull()) return pixmap;

    QPixmap tinted = pixmap;
    QPainter painter(&tinted);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), Color(role));
    painter.end();
    return tinted;
}

const QPixmap &Pixmap(const QString &path, Role role){
    const QString key = QStringLiteral("%1#%2").arg(path).arg(static_cast<int>(role));

    QHash<QString, QPixmap>::const_iterator it = g_Pixmaps.constFind(key);
    if(it != g_Pixmaps.constEnd()) return it.value();

    QPixmap pixmap(path);
    const QSize size = DeviceScale::PrimarySize(pixmap.size());
    if(!pixmap.isNull() && size != pixmap.size()){
        const int dot = path.lastIndexOf(QLatin1Char('.'));
        if(dot >= 0){
            const QPixmap twice(path.left(dot) + QStringLiteral("@2x") + path.mid(dot));
            if(!twice.isNull())
                pixmap = twice.scaled(size, Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation);
        }
    }
    return *g_Pixmaps.insert(key, Ink(pixmap, role));
}

QIcon Icon(const QString &path){
    QIcon icon(path);
    const int dot = path.lastIndexOf(QLatin1Char('.'));
    if(dot >= 0){
        const QString twice = path.left(dot) + QStringLiteral("@2x") + path.mid(dot);
        if(QFile::exists(twice)) icon.addFile(twice);
    }
    return icon;
}

void DrawEmptyThumbnail(QPainter *painter, const QRectF &rect,
                        Role fill, Role text, bool isDirectory,
                        const QRectF &captionRect){
    if(!painter || !rect.isValid()) return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Brush(fill));
    painter->drawRect(rect);

    const QString caption = isDirectory
        ? QCoreApplication::translate("Theme", "Directory")
        : QCoreApplication::translate("Theme", "No image");

    painter->setFont(DefaultFont());

    const QRectF where = captionRect.isValid() ? captionRect : rect;
    const QFontMetricsF metrics(painter->font());
    const qreal margin = metrics.height() / 2.0;
    if(metrics.horizontalAdvance(caption) + margin * 2.0 <= where.width() &&
       metrics.height() <= where.height()){

        painter->setBrush(Qt::NoBrush);
        painter->setPen(Pen(text));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(where, Qt::AlignCenter, caption);
    }

    painter->restore();
}

}
