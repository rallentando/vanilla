#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "devicescale.hpp"

#include "gadgetsstyle.hpp"

#include <QStyle>
#include <QPainter>
#include <QGraphicsScene>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QtMath>

#include "graphicstableview.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "gadgets.hpp"
#include "accessiblewebelement.hpp"
#include "webengineview.hpp"

namespace {
    QBrush MakeBrush(qreal start, qreal stop, QColor beg, QColor end){
        QLinearGradient gradient;
        gradient.setStart(0, start);
        gradient.setFinalStop(0, stop);
        gradient.setColorAt(0.0, beg);
        gradient.setColorAt(1.0, end);
        return QBrush(gradient);
    }

    QBrush TintBrush(qreal start, qreal stop, Theme::Role role){
        return MakeBrush(start, stop, Theme::Color(role, 0), Theme::Color(role));
    }

    void DrawTitleText(AbstractNodeItem *item, QPainter *painter,
                       const QRectF &rect, const QFont &font, int elideWidth = -1){
        const QStaticText &text =
            item->TitleText(item->GetNode()->ReadableTitle(), font, elideWidth);
        painter->setFont(font);
        painter->save();
        painter->setClipRect(rect);
        painter->drawStaticText(QPointF(rect.left(),
                                        rect.center().y() - text.size().height() / 2.0),
                                text);
        painter->restore();
    }

    QRectF ButtonIconRect(GraphicsButton *button, QRectF rect){
        const qreal inset = button->GetTableView()->ScaleByDevice(3);
        return rect.adjusted(inset, inset, -inset, -inset);
    }

    void DrawCardButton(GraphicsButton *button, QPainter *painter, const QIcon &icon){
        painter->save();
        const QRectF rect = button->boundingRect();
        const bool hovered = button->GetState() != GraphicsButton::NotHovered;
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(hovered ? Theme::Brush(Theme::VanillaPageAccent)
                                  : Theme::Brush(Theme::VanillaPageBackground, 204));
        const int radius = button->GetTableView()->ScaleByDevice(CHIP_CORNER_RADIUS);
        painter->drawRoundedRect(rect, radius, radius);
        painter->setRenderHint(QPainter::Antialiasing, false);
        const QRectF inner = ButtonIconRect(button, rect);
        const QPixmap pixmap = Theme::Fill(icon.pixmap(inner.size().toSize()),
                                           hovered ? Theme::VanillaPageBackground
                                                   : Theme::VanillaPageText);
        painter->drawPixmap(inner, pixmap, QRect(QPoint(), pixmap.size()));
        painter->restore();
    }

    void DrawCardCloseButton(CloseButton *button, QPainter *painter){
        static const QIcon icon = Theme::Icon(QStringLiteral(":/resources/tableview/close.png"));
        DrawCardButton(button, painter, icon);
    }
    void DrawCardCloneButton(CloneButton *button, QPainter *painter){
        static const QIcon clone = Theme::Icon(QStringLiteral(":/resources/tableview/clone.png"));
        static const QIcon restore = Theme::Icon(QStringLiteral(":/resources/tableview/restore.png"));
        DrawCardButton(button, painter,
                       button->GetTableView()->IsDisplayingTrashTree() ? restore : clone);
    }
    void DrawCardSoundButton(SoundButton *button, QPainter *painter){
        if(!button->GetNode()) return;
        View *view = button->GetNode()->GetView();
        const bool muted = view && view->IsAudioMuted();
        const bool audible = view && view->RecentlyAudible();
        if(!muted && !audible) return;
        static const QIcon muted_ = Theme::Icon(QStringLiteral(":/resources/tableview/muted.png"));
        static const QIcon audible_ = Theme::Icon(QStringLiteral(":/resources/tableview/audible.png"));
        DrawCardButton(button, painter, muted ? muted_ : audible_);
    }

    struct HeaderColors {
        Theme::Role background;
        Theme::Role button;
        Theme::Role border;
        Theme::Role hovered;
        Theme::Role text;
        Theme::Role strong;
    };

    const HeaderColors GlassHeaderColors = {
        Theme::GlassHeaderBackground, Theme::GlassHeaderButton,
        Theme::GlassHeaderButtonBorder, Theme::GlassHeaderButtonHovered,
        Theme::GlassText, Theme::GlassText,
    };

    const HeaderColors FlatHeaderColors = {
        Theme::FlatHeaderBackground, Theme::FlatHeaderButton,
        Theme::FlatHeaderButtonBorder, Theme::FlatHeaderButtonHovered,
        Theme::FlatText, Theme::FlatTextStrong,
    };

    const HeaderColors PlainHeaderColors = {
        Theme::PlainSurface, Theme::PlainField,
        Theme::PlainDivider, Theme::PlainHovered,
        Theme::PlainText, Theme::PlainTextStrong,
    };

    const HeaderColors PlainTranslucentHeaderColors = {
        Theme::FlatHeaderBackground, Theme::PlainField,
        Theme::PlainDivider, Theme::PlainHovered,
        Theme::PlainText, Theme::PlainTextStrong,
    };

    const HeaderColors PlainHeaderButtonColors = {
        Theme::PlainSurface, Theme::Transparent,
        Theme::Transparent, Theme::PlainHovered,
        Theme::PlainText, Theme::PlainTextStrong,
    };

    QIcon HeaderButtonIcon(GraphicsButton *button){
        static QIcon up = Application::style()->standardIcon(QStyle::SP_TitleBarShadeButton);
        static QIcon table = Theme::Icon(QStringLiteral(":/resources/tableview/table.png"));
        static QIcon trash = Theme::Icon(QStringLiteral(":/resources/tableview/trash.png"));
        static QIcon close = Theme::Icon(QStringLiteral(":/resources/tableview/close.png"));
        if(dynamic_cast<UpDirectoryButton*>(button)) return up;
        if(dynamic_cast<CloseTableButton*>(button)) return close;
        return button->GetTableView()->IsDisplayingTrashTree() ? table : trash;
    }

    void DrawHeaderButton(GraphicsButton *button, QPainter *painter, const HeaderColors &colors){
        GraphicsTableView *gtv = button->GetTableView();
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        if(!button->isEnabled()) painter->setOpacity(0.4);

        const QRectF rect = button->boundingRect().adjusted(0.5, 0.5, -0.5, -0.5);
        const bool hovered = button->isEnabled() && button->GetState() != GraphicsButton::NotHovered;
        painter->setPen(Theme::Pen(colors.border));
        painter->setBrush(Theme::Brush(hovered ? colors.hovered : colors.button));
        painter->drawRoundedRect(rect,
                                 gtv->ScaleByDevice(CHIP_CORNER_RADIUS),
                                 gtv->ScaleByDevice(CHIP_CORNER_RADIUS));

        const int side = gtv->ScaleByDevice(GADGETS_HEADER_ICON_SIZE);
        const QRect icon(qRound(button->boundingRect().left()) + gtv->ScaleByDevice(8),
                         qRound(rect.center().y() - side / 2.0), side, side);
        const QPixmap pixmap = Theme::Fill(HeaderButtonIcon(button).pixmap(icon.size()), colors.strong);
        painter->drawPixmap(icon, pixmap, QRect(QPoint(), pixmap.size()));

        const qreal left = icon.right() + 1 + gtv->ScaleByDevice(5);
        painter->setPen(Theme::Pen(colors.strong));
        painter->setFont(GadgetsHeaderFont());
        painter->drawText(QRectF(left, rect.top(), rect.right() - left, rect.height())
                          .translated(0.0, TableHeader::TextShift(GadgetsHeaderFont())),
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
                          gtv->HeaderButtonLabel(button));
        painter->restore();
    }

    void DrawHeaderCloseButton(GraphicsButton *button, QPainter *painter, const HeaderColors &colors){
        GraphicsTableView *gtv = button->GetTableView();
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const QRectF rect = button->boundingRect().adjusted(0.5, 0.5, -0.5, -0.5);
        if(button->GetState() != GraphicsButton::NotHovered){
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::Brush(colors.hovered));
            painter->drawRoundedRect(rect,
                                     gtv->ScaleByDevice(CHIP_CORNER_RADIUS),
                                     gtv->ScaleByDevice(CHIP_CORNER_RADIUS));
        }

        const int side = gtv->ScaleByDevice(GADGETS_HEADER_ICON_SIZE);
        const QRect icon(qRound(rect.center().x() - side / 2.0),
                         qRound(rect.center().y() - side / 2.0), side, side);
        const QPixmap pixmap = Theme::Fill(HeaderButtonIcon(button).pixmap(icon.size()), colors.strong);
        painter->drawPixmap(icon, pixmap, QRect(QPoint(), pixmap.size()));
        painter->restore();
    }

    void DrawHeaderFilter(HeaderFilter *filter, QPainter *painter, const HeaderColors &colors){
        GraphicsTableView *gtv = filter->GetTableView();
        const QColor ink = Theme::Color(colors.strong);
        if(filter->TextItem()->defaultTextColor() != ink)
            filter->TextItem()->setDefaultTextColor(ink);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = filter->boundingRect().adjusted(0.5, 0.5, -0.5, -0.5);
        painter->setPen(Theme::Pen(filter->IsEditing() ? colors.strong : colors.border));
        painter->setBrush(Theme::Brush(colors.button));
        painter->drawRoundedRect(rect,
                                 gtv->ScaleByDevice(CHIP_CORNER_RADIUS),
                                 gtv->ScaleByDevice(CHIP_CORNER_RADIUS));

        if(filter->Text().isEmpty()){
            const qreal padding = gtv->ScaleByDevice(8);
            painter->setPen(Theme::Pen(colors.text));
            painter->setFont(GadgetsHeaderFont());
            painter->drawText(rect.adjusted(padding, 0, -padding, 0)
                              .translated(0.0, TableHeader::TextShift(GadgetsHeaderFont())),
                              Qt::AlignLeft | Qt::AlignVCenter, gtv->HeaderFilterPlaceholder());
        } else {
            static QIcon close = Theme::Icon(QStringLiteral(":/resources/tableview/close.png"));
            const QRect cross = filter->ClearRect().toRect();
            const QPixmap pixmap = Theme::Fill(close.pixmap(cross.size()), colors.text);
            painter->drawPixmap(cross, pixmap, QRect(QPoint(), pixmap.size()));
        }
        painter->restore();
    }

    void DrawHeader(TableHeader *header, QPainter *painter, const HeaderColors &colors){
        GraphicsTableView *gtv = header->GetTableView();
        const QRectF bar = header->boundingRect();
        painter->save();

        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::Brush(colors.background));
        painter->drawRect(bar);
        painter->setPen(Theme::Pen(colors.border));
        painter->drawLine(QLineF(bar.left(), bar.bottom() - 0.5, bar.right(), bar.bottom() - 0.5));

        const QStringList titles = gtv->BreadcrumbTitles();
        const QList<QRectF> rects = header->SegmentRects(titles);
        const qreal separator = header->SeparatorWidth();
        const int flags = Qt::AlignVCenter | Qt::TextDontClip;
        auto drawSeparator = [&](qreal left){
            const QFont font = TableHeader::SegmentFont(false);
            const QFontMetricsF metrics(font);
            const QString chevron = QString(QChar(0x203A));
            const QRectF glyph = metrics.tightBoundingRect(chevron);
            const qreal shift = metrics.tightBoundingRect(QStringLiteral("H")).center().y()
                              - glyph.center().y()
                              + TableHeader::TextShift(font);
            painter->setFont(font);
            painter->setPen(Theme::Pen(colors.text));
            painter->drawText(QRectF(left, bar.top() + shift, separator, bar.height()),
                              Qt::AlignHCenter | flags, chevron);
        };

        int first = 0;
        while(first < rects.length() && rects[first].isEmpty()) first++;
        if(first > 0 && first < rects.length()){
            const qreal left = header->BreadcrumbLeft();
            painter->setFont(TableHeader::SegmentFont(false));
            painter->setPen(Theme::Pen(colors.text));
            painter->drawText(QRectF(left, bar.top(), header->EllipsisWidth(), bar.height())
                              .translated(0.0, TableHeader::TextShift(TableHeader::SegmentFont(false))),
                              Qt::AlignLeft | flags, QString(QChar(0x2026)));
            drawSeparator(left + header->EllipsisWidth());
        }

        const qreal padding = header->SegmentPadding();
        for(int i = first; i < rects.length(); i++){
            const bool last = i == rects.length() - 1;
            const bool hovered = i == header->HoveredSegment();
            if(hovered){
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->setPen(Qt::NoPen);
                painter->setBrush(Theme::Brush(colors.hovered));
                const QRectF chip = rects[i].adjusted(0.5, 0.5, -0.5, -0.5);
                painter->drawRoundedRect(chip,
                                         gtv->ScaleByDevice(CHIP_CORNER_RADIUS),
                                         gtv->ScaleByDevice(CHIP_CORNER_RADIUS));
                painter->restore();
            }
            const QFont font = TableHeader::SegmentFont(last);
            const QRectF text = rects[i].adjusted(padding, 0, -padding, 0)
                .translated(0.0, TableHeader::TextShift(font));
            painter->setFont(font);
            painter->setPen(Theme::Pen(colors.strong));
            painter->drawText(text, Qt::AlignLeft | flags,
                              last
                              ? QFontMetricsF(font).elidedText(titles[i], Qt::ElideRight, text.width() + 1.0)
                              : titles[i]);
            if(!last) drawSeparator(rects[i].right());
        }

        painter->restore();
    }

    const qreal SHADOW_TAIL[] = {
        0.41569, 0.26275, 0.15686, 0.09804, 0.05490,
        0.03137, 0.01569, 0.00784, 0.00392, 0.00000
    };
    const int SHADOW_BASE_RADIUS = int(sizeof(SHADOW_TAIL) / sizeof(SHADOW_TAIL[0]));

    qreal ShadowSample(int u){
        if(u <= -SHADOW_BASE_RADIUS) return 0.0;
        if(u < 0)                    return SHADOW_TAIL[-u - 1];
        if(u >= SHADOW_BASE_RADIUS)  return 1.0;
        return 1.0 - SHADOW_TAIL[u];
    }

    qreal ShadowStep(int u, int radius){
        if(radius == SHADOW_BASE_RADIUS) return ShadowSample(u);
        const qreal t = qreal(u) * SHADOW_BASE_RADIUS / radius;
        const int lo = qFloor(t);
        const qreal f = t - lo;
        return ShadowSample(lo) * (1.0 - f) + ShadowSample(lo + 1) * f;
    }

    QList<qreal> ShadowProfile(int len, int radius){
        QList<qreal> profile;
        profile.reserve(len + radius * 2);
        for(int i = 0; i < len + radius * 2; i++){
            const int u = i - radius;
            profile << qBound(0.0, ShadowStep(u, radius) - ShadowStep(u - len, radius), 1.0);
        }
        return profile;
    }

    const int SHADOW_CACHE_MAX = 8;

    QHash<QPair<quint64, int>, QPixmap> &ShadowCache(){
        static QHash<QPair<quint64, int>, QPixmap> cache;
        return cache;
    }

    QPair<quint64, int> ShadowKey(QSize size, QColor color, int radius){
        return qMakePair((quint64(size.width()  & 0xffff) << 48)
                       | (quint64(size.height() & 0xffff) << 32)
                       |  quint64(color.rgba()),
                         radius);
    }

    QPixmap ShadowPixmap(QSize size, QColor color, int radius){
        if(size.isEmpty()) return QPixmap();

        const QPair<quint64, int> key = ShadowKey(size, color, radius);

        QHash<QPair<quint64, int>, QPixmap> &cache = ShadowCache();
        const QHash<QPair<quint64, int>, QPixmap>::const_iterator found = cache.constFind(key);
        if(found != cache.constEnd()) return found.value();

        const QList<qreal> xs = ShadowProfile(size.width(), radius);
        const QList<qreal> ys = ShadowProfile(size.height(), radius);

        QImage image(xs.length(), ys.length(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        const qreal alpha = color.alphaF();

        for(int y = 0; y < ys.length(); y++){
            const bool insideY = y >= radius && y < ys.length() - radius;
            QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));

            for(int x = 0; x < xs.length(); x++){
                const bool insideX = x >= radius && x < xs.length() - radius;
                if(insideX && insideY) continue;

                const int a = qRound(xs[x] * ys[y] * alpha * 255.0);
                if(a <= 0) continue;

                line[x] = qRgba(color.red()   * a / 255,
                                color.green() * a / 255,
                                color.blue()  * a / 255, a);
            }
        }

        const QPixmap pixmap = QPixmap::fromImage(image);
        if(cache.size() >= SHADOW_CACHE_MAX) cache.clear();
        cache.insert(key, pixmap);
        return pixmap;
    }

    void DrawShadow(QPainter *painter, QRectF rect, Theme::Role role, int radius){
        const QRect card(qRound(rect.left()), qRound(rect.top()),
                         qRound(rect.right())  - qRound(rect.left()),
                         qRound(rect.bottom()) - qRound(rect.top()));
        if(card.isEmpty()) return;

        const QPixmap shadow = ShadowPixmap(card.size(), Theme::Color(role), radius);
        if(shadow.isNull()) return;

        const int r = radius;
        const int w = card.width();
        const int h = card.height();
        const QPoint at = card.topLeft() - QPoint(r, r);

        painter->drawPixmap(at,                     shadow, QRect(0,     0,     w + r*2, r));
        painter->drawPixmap(at + QPoint(0, r + h),   shadow, QRect(0,     r + h, w + r*2, r));
        painter->drawPixmap(at + QPoint(0, r),       shadow, QRect(0,     r,     r,       h));
        painter->drawPixmap(at + QPoint(r + w, r),   shadow, QRect(r + w, r,     r,       h));
    }

    const int CARD_CORNER_CACHE_MAX = 16;

    struct CardCornerKey {
        QRgb around;
        QRgb ring;
        int radius;
        int ringWidth;
        qreal ratio;
        bool operator==(const CardCornerKey &other) const {
            return around == other.around && ring == other.ring
                && radius == other.radius && ringWidth == other.ringWidth
                && ratio == other.ratio;
        }
    };

    size_t qHash(const CardCornerKey &key, size_t seed = 0){
        return qHashMulti(seed, key.around, key.ring, key.radius, key.ringWidth, key.ratio);
    }

    QPixmap CardCornerPixmap(QColor around, QColor ring, int radius, int ringWidth, qreal ratio){
        static QHash<CardCornerKey, QPixmap> cache;
        const CardCornerKey key = { around.rgba(), ring.rgba(), radius, ringWidth, ratio };
        const auto found = cache.constFind(key);
        if(found != cache.constEnd()) return found.value();

        const int side = qCeil(radius * 2 * ratio);
        QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(ratio);
        image.fill(around);
        const QRectF logical(0.0, 0.0, radius * 2, radius * 2);
        {
            QPainter into(&image);
            into.setRenderHint(QPainter::Antialiasing, true);
            into.setPen(Qt::NoPen);
            into.setBrush(Qt::black);
            into.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            into.drawEllipse(logical);
            if(ringWidth > 0 && ring.alpha() > 0){
                into.setCompositionMode(QPainter::CompositionMode_SourceOver);
                into.setBrush(Qt::NoBrush);
                into.setPen(QPen(ring, ringWidth));
                const qreal half = ringWidth / 2.0;
                into.drawEllipse(logical.adjusted(half, half, -half, -half));
            }
        }
        const QPixmap pixmap = QPixmap::fromImage(image);
        if(cache.size() >= CARD_CORNER_CACHE_MAX) cache.clear();
        cache.insert(key, pixmap);
        return pixmap;
    }

    void DrawCardCorners(QPainter *painter, const QRect &card, QColor around,
                         QColor ring, int radius, int ringWidth){
        const int r = radius;
        const int w = card.width();
        const int h = card.height();
        if(w <= 0 || h <= 0) return;
        const bool rounds = r > 0 && w >= r * 2 && h >= r * 2;
        const int left = card.left();
        const int top = card.top();

        if(!rounds){
            if(ringWidth > 0 && ring.alpha() > 0){
                const int across = qMin(ringWidth, qMin(w, h));
                painter->fillRect(QRect(left, top, w, across), ring);
                painter->fillRect(QRect(left, top + h - across, w, across), ring);
                painter->fillRect(QRect(left, top, across, h), ring);
                painter->fillRect(QRect(left + w - across, top, across, h), ring);
            }
            return;
        }

        const qreal ratio = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
        const QPixmap corners = CardCornerPixmap(around, ring, r, ringWidth, ratio);
        const int d = qCeil(r * ratio);
        painter->drawPixmap(QPoint(left, top),               corners, QRect(0, 0, d, d));
        painter->drawPixmap(QPoint(left + w - r, top),       corners, QRect(d, 0, d, d));
        painter->drawPixmap(QPoint(left, top + h - r),       corners, QRect(0, d, d, d));
        painter->drawPixmap(QPoint(left + w - r, top + h - r), corners, QRect(d, d, d, d));

        if(ringWidth > 0 && ring.alpha() > 0){
            painter->fillRect(QRect(left + r, top, w - r * 2, ringWidth), ring);
            painter->fillRect(QRect(left + r, top + h - ringWidth, w - r * 2, ringWidth), ring);
            painter->fillRect(QRect(left, top + r, ringWidth, h - r * 2), ring);
            painter->fillRect(QRect(left + w - ringWidth, top + r, ringWidth, h - r * 2), ring);
        }
    }

    void DrawRoundedCard(QPainter *painter, const QRect &card, const QBrush &fill,
                         const QColor &edge, int radius, int edgeWidth){
        if(card.width() <= 0 || card.height() <= 0) return;
        const qreal half = edgeWidth / 2.0;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(edge, edgeWidth));
        painter->setBrush(fill);
        painter->drawRoundedRect(QRectF(card).adjusted(half, half, -half, -half),
                                 qMax(0.0, radius - half), qMax(0.0, radius - half));
        painter->restore();
    }

    QBrush CardPictureBrush(const QImage &image, const QRectF &source, const QRect &card){
        QBrush brush(image);
        QTransform transform;
        transform.translate(card.left(), card.top());
        transform.scale(card.width() / source.width(), card.height() / source.height());
        transform.translate(-source.left(), -source.top());
        brush.setTransform(transform);
        return brush;
    }

    QRectF CoverSource(const QSize &image, const QSize &box){
        const qreal scale = qMax(qreal(box.width())  / image.width(),
                                 qreal(box.height()) / image.height());
        const QSizeF size(box.width() / scale, box.height() / scale);
        return QRectF(QPointF((image.width() - size.width()) / 2.0, 0.0), size);
    }

    QPixmap CardIconPixmap(GraphicsTableView *gtv, Node *nd, const QSize &size){
        static QIcon blank    = Theme::Icon(QStringLiteral(":/resources/blank.png"));
        static QIcon page     = Theme::Icon(QStringLiteral(":/resources/page.png"));
        static QIcon folder   = Theme::Icon(QStringLiteral(":/resources/folder.png"));
        static QIcon folded   = Theme::Icon(QStringLiteral(":/resources/folded.png"));
        static QIcon unfolded = Theme::Icon(QStringLiteral(":/resources/unfolded.png"));
        static QIcon blankw    = Theme::Icon(QStringLiteral(":/resources/blankw.png"));
        static QIcon pagew     = Theme::Icon(QStringLiteral(":/resources/pagew.png"));
        static QIcon folderw   = Theme::Icon(QStringLiteral(":/resources/folderw.png"));
        static QIcon foldedw   = Theme::Icon(QStringLiteral(":/resources/foldedw.png"));
        static QIcon unfoldedw = Theme::Icon(QStringLiteral(":/resources/unfoldedw.png"));
        const bool lightInk = Theme::IsDark();
        const bool isDir = nd->IsDirectory();
        const bool foldable = gtv->GetNodeCollectionType() == GraphicsTableView::Foldable;
        const bool isFolded = nd->GetFolded();
        View *view = nd->GetView();
        QIcon icon;
        if(view){
            icon = view->GetIcon();
        }
        if(icon.isNull() || icon.availableSizes().first().width() <= 2){
            icon = nd->GetIcon();
        }
        if(icon.isNull() || icon.availableSizes().first().width() <= 2){
            icon = lightInk
                ? (!isDir ? (view ? pagew : blankw) : !foldable ? folderw : isFolded ? foldedw : unfoldedw)
                : (!isDir ? (view ? page  : blank)  : !foldable ? folder  : isFolded ? folded  : unfolded);
        }
        return icon.pixmap(size, (view || isDir) ? QIcon::Normal : QIcon::Disabled);
    }

    QRect FrameRateRect(GraphicsTableView *gtv){
        return QRect(gtv->ScaleByDevice(3), gtv->ContentsTop() - gtv->ScaleByDevice(DISPLAY_PADDING_Y),
                     gtv->ScaleByDevice(60), gtv->ScaleByDevice(20));
    }
};

const int GlassStyle::m_ThumbnailPaddingX = 2;
const int GlassStyle::m_ThumbnailPaddingY = 2;
const int GlassStyle::m_ThumbnailTitleHeight = 20;
const int GlassStyle::m_ThumbnailWidthPercentage = 15;
const int GlassStyle::m_ThumbnailDefaultColumnCount = 4;
const int GlassStyle::m_ThumbnailAreaWidthPercentage =
    GlassStyle::m_ThumbnailWidthPercentage *
    GlassStyle::m_ThumbnailDefaultColumnCount;

const bool GlassStyle::m_ThumbnailDrawBorder = false;

const int GlassStyle::m_NodeTitleHeight = 20;
const bool GlassStyle::m_NodeTitleDrawBorder = false;

const int GlassStyle::m_InPlaceNotifierWidth =
    500 + DEFAULT_THUMBNAIL_SIZE.width() + GlassStyle::m_ThumbnailPaddingX * 3;
const int GlassStyle::m_InPlaceNotifierHeight =
    DEFAULT_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2;
const bool GlassStyle::m_InPlaceNotifierDrawBorder = false;

void GlassStyle::ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const {
    const float zoom = gtv->m_CurrentThumbnailZoomFactor;

    const QSize defaultSize = gtv->ScaleByDevice(DEFAULT_THUMBNAIL_SIZE);
    const QSize minimumSize = gtv->ScaleByDevice(MINIMUM_THUMBNAIL_SIZE);

    const QSize defaultThumbnailWholeSize =
        QSize(defaultSize.width()  + gtv->ScaleByDevice(m_ThumbnailPaddingX) * 2,
              defaultSize.height() + gtv->ScaleByDevice(m_ThumbnailPaddingY) * 2
              + gtv->ScaleByDevice(m_ThumbnailTitleHeight));

    const QSize minimumThumbnailWholeSize =
        QSize(minimumSize.width()  + gtv->ScaleByDevice(m_ThumbnailPaddingX) * 2,
              minimumSize.height() + gtv->ScaleByDevice(m_ThumbnailPaddingY) * 2
              + gtv->ScaleByDevice(m_ThumbnailTitleHeight));

    int wholeWidth = static_cast<int>(gtv->m_Size.width())
        - gtv->ScaleByDevice(DISPLAY_PADDING_LEFT)
        - gtv->ScaleByDevice(DISPLAY_PADDING_X);
    int areaWidth = wholeWidth * m_ThumbnailAreaWidthPercentage / 100;
    thumbWidth = (wholeWidth * m_ThumbnailWidthPercentage / 100) * zoom;
    col = m_ThumbnailDefaultColumnCount / zoom;

    int minWidth = minimumThumbnailWholeSize.width();
    int defWidth = defaultThumbnailWholeSize.width() * zoom;

    if(col < 1) col = 1;

    if(defWidth < minWidth) defWidth = minWidth;

    if(thumbWidth > defWidth){
        col += (areaWidth - (defWidth * col)) / defWidth;
        thumbWidth = defWidth;
    }

    if(thumbWidth < minWidth){
        col -= ((minWidth - thumbWidth) * col) / minWidth;
        if(col > 1) col -= 1;
        if(col == 0) col = 1;
        thumbWidth = minWidth;
    }

    const int marginWidth  = defaultThumbnailWholeSize.width()  - defaultSize.width();
    const int marginHeight = defaultThumbnailWholeSize.height() - defaultSize.height();
    const double aspect =
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.height()) /
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.width());

    thumbHeight = ((thumbWidth - marginWidth) * aspect) + marginHeight;
    line = (gtv->m_Size.height() - gtv->ContentsTop()) / thumbHeight;

    if(line < 1) line = 1;
}

void GlassStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter) const {
    painter->setBrush(BackgroundVeil(gtv));
    painter->setPen(Qt::NoPen);
    painter->drawRect(gtv->boundingRect());
    RenderBackgroundOverVeil(gtv, painter);
}

QColor GlassStyle::BackgroundVeil(GraphicsTableView *gtv) const {
    return Theme::Color(Theme::GlassOverlayBackground,
                        gtv->IsDisplayingNode() ? 170 : 100);
}

void GlassStyle::RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const {
    if(gtv->EnableFrameRate()){
        painter->setPen(Theme::Pen(Theme::GlassText));
        painter->drawText(FrameRateRect(gtv),
                          QStringLiteral("%1fps").arg(gtv->m_CurrentFrameRate));
    }
}

void GlassStyle::Render(Thumbnail *thumb, QPainter *painter) const {
    Node *nd = thumb->GetNode();
    GraphicsTableView *gtv = thumb->GetTableView();
    if(!nd) return;

    QRectF bound = thumb->boundingRect();
    QRectF realRect = bound.translated(thumb->pos());

    if(thumb->scene()){
        if(!realRect.intersects(thumb->scene()->sceneRect()))
            return;
    } else {
        if(!realRect.intersects(QRectF(QPointF(), gtv->Size())))
            return;
    }

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    View *view = nd->GetView();
    bool isDir = nd->IsDirectory();

    QRectF rect = bound;

    QImage image = nd->VisibleImage();

    const qreal start = bound.top();
    const qreal stop  = bound.bottom();

    painter->setPen(Qt::NoPen);

    if(view){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbLoaded));
        painter->drawRect(rect);
    }

    if(thumb->IsPrimary()){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbPrimary));
        painter->drawRect(rect);
    }

    if(thumb->IsHovered()){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbHovered));
        painter->drawRect(rect);
    }

    if(thumb->isSelected()){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbSelected));
        painter->drawRect(rect);
    }

    if(m_ThumbnailDrawBorder){
        painter->setPen(Theme::Pen(Theme::GlassBorder));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    QRectF image_rect = rect;
    image_rect.setLeft(image_rect.left() + gtv->ScaleByDevice(m_ThumbnailPaddingX));
    image_rect.setRight(image_rect.right() - gtv->ScaleByDevice(m_ThumbnailPaddingX));
    image_rect.setTop(image_rect.top() + gtv->ScaleByDevice(m_ThumbnailPaddingY));
    image_rect.setBottom(image_rect.bottom() - gtv->ScaleByDevice(m_ThumbnailPaddingY)
                         - gtv->ScaleByDevice(m_ThumbnailTitleHeight));

    QRectF title_rect = image_rect;
    title_rect.moveTop(image_rect.bottom());
    title_rect.setHeight(gtv->ScaleByDevice(m_ThumbnailTitleHeight));

    if(!image.isNull()){
        QSizeF size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        QPointF diff = QPointF(image_rect.width()  - size.width(),
                               image_rect.height() - size.height());
        painter->drawImage(QRectF(image_rect.topLeft() + diff / 2.0, size),
                           image, QRectF(QPointF(), image.size()));
    } else {
        Theme::DrawEmptyThumbnail(painter, image_rect,
                                  isDir ? Theme::GlassPlaceholderDirectory
                                        : Theme::GlassPlaceholderPage,
                                  Theme::GlassText, isDir);
    }

    title_rect.setLeft(title_rect.left() + 2.0);
    title_rect.setRight(title_rect.right() - gtv->ScaleByDevice(3));
    title_rect.setBottom(title_rect.bottom() - 1.0);

    {
        QIcon icon;
        static QIcon blank    = Theme::Icon(QStringLiteral(":/resources/blankw.png"));
        static QIcon page     = Theme::Icon(QStringLiteral(":/resources/pagew.png"));
        static QIcon folder   = Theme::Icon(QStringLiteral(":/resources/folderw.png"));
        static QIcon folded   = Theme::Icon(QStringLiteral(":/resources/foldedw.png"));
        static QIcon unfolded = Theme::Icon(QStringLiteral(":/resources/unfoldedw.png"));
        bool foldable = gtv->GetNodeCollectionType() == GraphicsTableView::Foldable;
        bool isFolded = nd->GetFolded();
        if(view){
            icon = view->GetIcon();
        }
        if(icon.isNull() || icon.availableSizes().first().width() <= 2){
            icon = nd->GetIcon();
        }
        if(icon.isNull() || icon.availableSizes().first().width() <= 2){
            icon = !isDir ? (view ? page : blank) : !foldable ? folder : isFolded ? folded : unfolded;
        }
        QSize iconSize = gtv->ScaleByDevice(QSize(16, 16));
        QPixmap pixmap = icon.pixmap(iconSize, (view || isDir) ? QIcon::Normal : QIcon::Disabled);
        if(pixmap.width() > 2){
            painter->drawPixmap(QRect(title_rect.topLeft().toPoint() + QPoint(0, 2), iconSize),
                                pixmap, QRect(QPoint(), pixmap.size()));
            title_rect.setLeft(title_rect.left() + gtv->ScaleByDevice(19));
        }
    }

    {
        painter->setPen(Theme::Pen(Theme::GlassText));
        painter->setBrush(Qt::NoBrush);

        painter->setRenderHint(QPainter::Antialiasing, true);
        DrawTitleText(thumb, painter, title_rect, ThumbnailTitleFont());
    }
    painter->restore();
}

void GlassStyle::Render(NodeTitle *title, QPainter *painter) const {
    Node *nd = title->GetNode();
    GraphicsTableView *gtv = title->GetTableView();

    QRectF port = gtv->NodeTitleAreaRect();
    if(!nd || !port.isValid()) return;

    QRectF bound = title->boundingRect();
    QRectF realRect = bound.translated(title->pos());

    if(title->scene()){
        if(!realRect.intersects(title->scene()->sceneRect()))
            return;
    } else {
        if(!realRect.intersects(QRectF(QPointF(), gtv->Size())))
            return;
    }

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    if(m_NodeTitleDrawBorder)
        painter->setClipRect(bound.intersected(port));

    View *view = nd->GetView();
    bool isDir = nd->IsDirectory();

    const qreal start = bound.top();
    const qreal stop  = bound.bottom();

    painter->setPen(Qt::NoPen);

    if(view){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbLoaded));
        painter->drawRect(bound);
    }

    if(title->IsPrimary()){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbPrimary));
        painter->drawRect(bound);
    }

    if(title->IsHovered()){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbHovered));
        painter->drawRect(bound);
    }

    if(title->isSelected()){
        painter->setBrush(TintBrush(start, stop, Theme::GlassThumbSelected));
        painter->drawRect(bound);
    }

    QRectF title_rect = bound;
    title_rect.setTop(bound.top() + 1);
    title_rect.setLeft(bound.left() + gtv->ScaleByDevice(title->GetNest() * 20 + 5));

    {
        QIcon icon;
        static QIcon blank    = Theme::Icon(QStringLiteral(":/resources/blankw.png"));
        static QIcon page     = Theme::Icon(QStringLiteral(":/resources/pagew.png"));
        static QIcon folder   = Theme::Icon(QStringLiteral(":/resources/folderw.png"));
        static QIcon folded   = Theme::Icon(QStringLiteral(":/resources/foldedw.png"));
        static QIcon unfolded = Theme::Icon(QStringLiteral(":/resources/unfoldedw.png"));
        bool foldable = gtv->GetNodeCollectionType() == GraphicsTableView::Foldable;
        bool isFolded = nd->GetFolded();
        if(view){
            icon = view->GetIcon();
        }
        if(icon.isNull() || icon.availableSizes().first().width() <= 2){
            icon = nd->GetIcon();
        }
        if(icon.isNull() || icon.availableSizes().first().width() <= 2){
            icon = !isDir ? (view ? page : blank) : !foldable ? folder : isFolded ? folded : unfolded;
        }
        QSize iconSize = gtv->ScaleByDevice(QSize(16, 16));
        QPixmap pixmap = icon.pixmap(iconSize, (view || isDir) ? QIcon::Normal : QIcon::Disabled);
        if(pixmap.width() > 2){
            painter->drawPixmap(QRect(title_rect.topLeft().toPoint() + QPoint(0, 1), iconSize),
                                pixmap, QRect(QPoint(), pixmap.size()));
            title_rect.setLeft(title_rect.left() + gtv->ScaleByDevice(19));
        }
    }

    {
        painter->setPen(Theme::Pen(Theme::GlassText));
        painter->setBrush(Qt::NoBrush);
        DrawTitleText(title, painter, title_rect, NodeTitleFont());
    }
    painter->restore();
}

void GlassStyle::Render(SpotLight *light, QPainter *painter) const {
    const GraphicsTableView* parent = static_cast<GraphicsTableView*>(light->parentItem());

    const bool p = light->GetType() == GraphicsTableView::PrimarySpotLight;
    const bool h = light->GetType() == GraphicsTableView::HoveredSpotLight;
    const bool l = light->GetType() == GraphicsTableView::LoadedSpotLight;

    const int index =
        p ? parent->m_PrimaryItemIndex :
        h ? parent->m_HoveredItemIndex :
        l ? light->GetIndex() : -1;

    if(index == -1 ||
       index >= parent->m_DisplayThumbnails.length())
        return;

    painter->save();

    const QSize size = parent->m_Size.toSize() + QSize(1, 1);

    const Thumbnail *thumb = parent->m_DisplayThumbnails[index];
    const NodeTitle *title = parent->m_DisplayNodeTitles[index];

    const int x1  = thumb->pos().x() + thumb->boundingRect().right();
    const int x2  = title->pos().x() + title->boundingRect().left();

    const int y1b = thumb->pos().y() + thumb->boundingRect().top();
    const int y1e = thumb->pos().y() + thumb->boundingRect().bottom();

    const int y2b = title->pos().y() + title->boundingRect().top();
    const int y2e = title->pos().y() + title->boundingRect().bottom();

    const int ybrange = y2b - y1b;
    const int yerange = y2e - y1e;

    const double xrange = x2 - x1;
    const int begx = qMin(x1, x2);
    const int endx = qMax(x1, x2);

    double yrange, begy, endy;
    double progress;
    int x, y;

    const QRectF bound = light->boundingRect();
    const QRectF rect = QRectF(bound.topLeft(), bound.size());

    QImage image(size, QImage::Format_ARGB32);
#if defined(QT_DEBUG) || !defined(Q_OS_WIN)
    image.fill(0);
#endif

    QPen pen;

    const QColor fill = Theme::Color(p ? Theme::GlassSpotLightPrimary :
                                     h ? Theme::GlassSpotLightHovered :
                                         Theme::GlassSpotLightLoaded);
    const QColor edge = Theme::Color(p ? Theme::GlassSpotLightPrimaryEdge :
                                     h ? Theme::GlassSpotLightHoveredEdge :
                                         Theme::GlassSpotLightLoadedEdge);

    const int r = fill.red();
    const int g = fill.green();
    const int b = fill.blue();
    const int a1 = fill.alpha();
    const int a2 = edge.alpha();

    for(x = begx; x < endx; x++){
        progress = (x - x1)/xrange;
        begy = y1b + (ybrange * progress);
        endy = y1e + (yerange * progress);
        yrange = endy - begy;
        for(y = begy+1; y < endy-1; y++)
        if(rect.contains(x, y)) image.setPixel(x, y, qRgba(r, g, b, a1*(y-begy)/yrange));
        if(rect.contains(x, y)) image.setPixel(x, y, qRgba(r, g, b, a2*(y-begy)/yrange));
    }
    pen.setColor(edge);

    painter->drawImage(QPointF(), image);

    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(pen);
    painter->drawLine(x1, y1e,
                      x2, y2e);
    painter->drawLine(x1, y1e-1,
                      x2, y2e-1);
    painter->restore();
}

void GlassStyle::Render(InPlaceNotifier *notifier, QPainter *painter) const {
    static QLocale locale = QLocale::system();
    Node *nd = notifier->GetNode();
    if(!nd) return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    bool isDir = nd->IsDirectory();

    QRectF rect = notifier->boundingRect();

    QImage image = nd->VisibleImage();

    const QString title = nd->GetTitle().replace(QStringLiteral("\n"), QStringLiteral(" ")).trimmed();
    const QString url = nd->GetUrl().toString().replace(QStringLiteral("\n"), QStringLiteral(" ")).trimmed();
    const QString create = locale.toString(nd->GetCreateDate(), QLocale::LongFormat);
    const QString lastUpdate = locale.toString(nd->GetLastUpdateDate(), QLocale::LongFormat);
    const QString lastAccess = locale.toString(nd->GetLastAccessDate(), QLocale::LongFormat);

    {
        painter->setBrush(Theme::Brush(Theme::GlassInPlaceBackground));
        painter->setPen(Qt::NoPen);
        painter->drawRect(rect);
    }

    if(m_InPlaceNotifierDrawBorder){
        painter->setPen(Theme::Pen(Theme::GlassBorder));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(notifier->boundingRect());
    }

    GraphicsTableView *gtv = notifier->GetTableView();
    const QRectF image_rect =
        QRectF(QPointF(gtv->ScaleByDevice(m_ThumbnailPaddingX),
                       gtv->ScaleByDevice(m_ThumbnailPaddingY)),
               QSizeF(gtv->ScaleByDevice(DEFAULT_THUMBNAIL_SIZE)));

    if(!image.isNull()){
        QSizeF size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        QPointF diff = QPointF(image_rect.width()  - size.width(),
                               image_rect.height() - size.height());
        painter->drawImage(QRectF(image_rect.topLeft() + diff / 2.0, size),
                           image, QRectF(QPointF(), image.size()));
    } else {
        Theme::DrawEmptyThumbnail(painter, image_rect,
                                  isDir ? Theme::GlassPlaceholderDirectory
                                        : Theme::GlassPlaceholderPage,
                                  Theme::GlassText, isDir);
    }

    const int basex = gtv->ScaleByDevice(m_ThumbnailPaddingX * 3) + image_rect.width();
    const int basey = gtv->ScaleByDevice(17);
    const int width = gtv->ScaleByDevice(495);
    const int height = gtv->ScaleByDevice(25);

    {
        painter->setPen(Theme::Pen(Theme::GlassText));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(NotifierFont());

        painter->drawText(QRect(basex, basey+height*0, width, height), Qt::AlignLeft, QObject::tr("Title : ") + title);
        painter->drawText(QRect(basex, basey+height*1, width, height), Qt::AlignLeft, QObject::tr("Url : ") + url);
        painter->drawText(QRect(basex, basey+height*2, width, height), Qt::AlignLeft, QObject::tr("CreatedDate : ") + create);
        painter->drawText(QRect(basex, basey+height*4, width, height), Qt::AlignLeft, QObject::tr("LastUpdatedDate : ") + lastUpdate);
        painter->drawText(QRect(basex, basey+height*3, width, height), Qt::AlignLeft, QObject::tr("LastAccessedDate : ") + lastAccess);
    }

    painter->restore();
}

void GlassStyle::Render(CloseButton *button, QPainter *painter) const {
    DrawCardCloseButton(button, painter);
}

void GlassStyle::Render(CloneButton *button, QPainter *painter) const {
    DrawCardCloneButton(button, painter);
}

void GlassStyle::Render(SoundButton *button, QPainter *painter) const {
    DrawCardSoundButton(button, painter);
}

void GlassStyle::Render(UpDirectoryButton *button, QPainter *painter) const {
    DrawHeaderButton(button, painter, GlassHeaderColors);
}

void GlassStyle::Render(ToggleTrashButton *button, QPainter *painter) const {
    DrawHeaderButton(button, painter, GlassHeaderColors);
}

void GlassStyle::Render(CloseTableButton *button, QPainter *painter) const {
    DrawHeaderCloseButton(button, painter, GlassHeaderColors);
}

void GlassStyle::Render(TableHeader *header, QPainter *painter) const {
    DrawHeader(header, painter, GlassHeaderColors);
}

void GlassStyle::Render(HeaderFilter *filter, QPainter *painter) const {
    DrawHeaderFilter(filter, painter, GlassHeaderColors);
}

void GlassStyle::Render(AccessibleWebElement *awe, QPainter *painter) const {
    if(awe->GetBoundingPos().isNull() || !awe->GetElement() || awe->GetElement()->IsNull()) return;

    QUrl url = awe->GetElement()->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    painter->save();

    if(!awe->IsSelected()){

        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::Brush(Theme::GlassAccessKeyChip,
                                       awe->IsCurrentBlock() ? 100 : 50));
        painter->drawRect(awe->CharChipRect());

        painter->setFont(awe->CharChipFont());
        painter->setPen(Theme::Pen(Theme::GlassAccessKeyText,
                                   awe->IsCurrentBlock() ? 200 : 100));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(AccessibleWebElement::CapCenteredRect(awe->CharChipRect(), awe->CharChipFont()),
                          Qt::AlignCenter, awe->GetGadgets()->IndexToString(awe->GetIndex()));

    } else {
        int width = AccessibleWebElement::GetInfoMetrics().boundingRect(str).width();
        width = width + ScaleByDevice(15) - str.length()*0.4;

        if(width > ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH))
            width = ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH);

        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QPoint base(awe->GetBoundingPos() - QPoint(width/2, infoHeight/2));
        QRect rect = QRect(base, QSize(width, infoHeight));

        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::Brush(Theme::GlassAccessKeyInfoBackground));
            painter->drawRect(rect);
        }

        rect = QRect(QPoint(base.x()+2, base.y()), QSize(width-2, infoHeight));

        {
            painter->setFont(AccessKeyInfoFont());
            painter->setPen(Theme::Pen(Theme::GlassAccessKeyInfoText));
            painter->setBrush(Qt::NoBrush);
            painter->drawText(rect, str);
        }

        QMap<QString, QRect> keyrectmap = awe->KeyRects();
        QMap<QString, QRect> exprectmap = awe->ExpRects();

        QStringList actions = awe->GetGadgets()->GetAccessKeyKeyMap().values();
        actions.removeDuplicates();
        for(QString action : std::as_const(actions)){

            QStringList list;

            for(QKeySequence seq : awe->GetGadgets()->GetAccessKeyKeyMap().keys(action)){
                list << seq.toString();
            }

            QString key = list.join(QStringLiteral(" or "));
            QString exp = action;
            QRect keyrect = keyrectmap[action];
            QRect exprect = exprectmap[action];

            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::Brush(Theme::GlassAccessKeyInfoBackground));
            painter->drawRect(keyrect);
            painter->drawRect(exprect);

            painter->setPen(Theme::Pen(Theme::GlassAccessKeyInfoText));
            painter->setBrush(Qt::NoBrush);

            painter->setFont(AccessKeyChipSFont());
            painter->drawText(keyrect, Qt::AlignCenter, key);

            painter->setFont(AccessKeyInfoFont());
            painter->drawText(exprect.translated(QPoint(0,-1)), Qt::AlignCenter, exp);
        }
    }
    painter->restore();
}

void GlassStyle::OnSetNest(Thumbnail *thumb, int nest) const {
    qreal opacity = 1.0;
    for(int i = 0; i < nest && opacity >= 0.3; i++)
        opacity *= 0.8;
    thumb->setOpacity(opacity);
}

void GlassStyle::OnSetNest(NodeTitle *title, int nest) const {
    qreal opacity = 1.0;
    for(int i = 0; i < nest && opacity >= 0.3; i++)
        opacity *= 0.8;
    title->setOpacity(opacity);
}

void GlassStyle::OnSetPrimary(Thumbnail *thumb, bool) const {
    if(thumb->graphicsEffect()) thumb->setGraphicsEffect(nullptr);
}

void GlassStyle::OnSetPrimary(NodeTitle *title, bool) const {
    if(title->graphicsEffect()) title->setGraphicsEffect(nullptr);
}

void GlassStyle::OnSetHovered(Thumbnail *thumb, bool hovered) const {
    Q_UNUSED(thumb) Q_UNUSED(hovered)
}

void GlassStyle::OnSetHovered(NodeTitle *title, bool hovered) const {
    Q_UNUSED(title) Q_UNUSED(hovered)
}

void GlassStyle::OnSetState(GraphicsButton *button, GraphicsButton::ButtonState state) const {
    if(!button->isVisible()) return;

    if(button->graphicsEffect()) button->setGraphicsEffect(nullptr);
    switch(state){
    case GraphicsButton::NotHovered:  button->setCursor(Qt::ArrowCursor); break;
    case GraphicsButton::Hovered:     button->setCursor(Qt::PointingHandCursor); break;
    case GraphicsButton::Pressed:     button->setCursor(Qt::PointingHandCursor); break;
    }
    button->update();
}

void GlassStyle::OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const {
    Q_UNUSED(elem)
    if(awe->graphicsEffect()) awe->setGraphicsEffect(nullptr);
}

void GlassStyle::OnReshow(QGraphicsRectItem *gri) const {
    gri->setPen(Theme::Pen(Theme::GlassSelectRectBorder));
    gri->setBrush(Theme::Brush(Theme::GlassSelectRectFill));
}

QRectF GlassStyle::ThumbnailAreaRect(GraphicsTableView *gtv) const {
    return QRectF(gtv->ScaleByDevice(DISPLAY_PADDING_LEFT), 0,
                  gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount,
                  gtv->m_Size.height());
}

QRectF GlassStyle::NodeTitleAreaRect(GraphicsTableView *gtv) const {
    const int paddingX = gtv->ScaleByDevice(DISPLAY_PADDING_X);
    const int paddingLeft = gtv->ScaleByDevice(DISPLAY_PADDING_LEFT);
    const int paddingY = gtv->ScaleByDevice(DISPLAY_PADDING_Y);
    const int scrollBar = gtv->ScaleByDevice(GADGETS_SCROLL_BAR_MARGIN) * 2
                        + gtv->ScaleByDevice(GADGETS_SCROLL_BAR_WIDTH);
    return QRectF(paddingLeft
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + scrollBar,
                  (m_NodeTitleDrawBorder ? paddingY : 0),
                  gtv->m_Size.width()
                  - paddingLeft
                  - gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  - scrollBar -
                  (m_NodeTitleDrawBorder ? paddingX : 0),
                  gtv->m_Size.height() -
                  (m_NodeTitleDrawBorder ? paddingY * 2 : 0));
}

QRectF GlassStyle::ScrollBarAreaRect(GraphicsTableView *gtv) const {
    const int paddingY = gtv->ScaleByDevice(DISPLAY_PADDING_Y);
    const int top = gtv->ContentsTop();
    return QRectF(gtv->ScaleByDevice(DISPLAY_PADDING_LEFT)
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + gtv->ScaleByDevice(GADGETS_SCROLL_BAR_MARGIN),
                  top,
                  gtv->ScaleByDevice(GADGETS_SCROLL_BAR_WIDTH),
                  gtv->m_Size.height() - top - paddingY);
}

QGraphicsRectItem *GlassStyle::CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const {
    return gtv->scene()->addRect(QRectF(pos, pos),
                                 Theme::Pen(Theme::GlassSelectRectBorder),
                                 Theme::Brush(Theme::GlassSelectRectFill));
}

QRectF GlassStyle::ThumbnailHitRect(const Thumbnail *thumb) const {
    return thumb->boundingRect();
}

int GlassStyle::NodeTitleHeight(GraphicsTableView *gtv) const {
    return gtv->ScaleByDevice(m_NodeTitleHeight);
}

const int FlatStyle::m_ThumbnailPaddingX = 15;
const int FlatStyle::m_ThumbnailPaddingY = 15;
const int FlatStyle::m_ThumbnailTitleHeight = 20;
const int FlatStyle::m_ThumbnailWidthPercentage = 20;
const int FlatStyle::m_ThumbnailDefaultColumnCount = 5;
const int FlatStyle::m_ThumbnailAreaWidthPercentage =
    FlatStyle::m_ThumbnailWidthPercentage *
    FlatStyle::m_ThumbnailDefaultColumnCount;

const bool FlatStyle::m_ThumbnailDrawBorder = false;

void FlatStyle::ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const {
    const float zoom = gtv->m_CurrentThumbnailZoomFactor;

    const QSize defaultSize = gtv->ScaleByDevice(DEFAULT_THUMBNAIL_SIZE);
    const QSize minimumSize = gtv->ScaleByDevice(MINIMUM_THUMBNAIL_SIZE);

    const QSize defaultThumbnailWholeSize =
        QSize(defaultSize.width()  + gtv->ScaleByDevice(m_ThumbnailPaddingX) * 2,
              defaultSize.height() + gtv->ScaleByDevice(m_ThumbnailPaddingY) * 2);

    const QSize minimumThumbnailWholeSize =
        QSize(minimumSize.width()  + gtv->ScaleByDevice(m_ThumbnailPaddingX) * 2,
              minimumSize.height() + gtv->ScaleByDevice(m_ThumbnailPaddingY) * 2);

    int wholeWidth = gtv->m_Size.width()
        - gtv->ScaleByDevice(DISPLAY_PADDING_LEFT)
        - gtv->ScaleByDevice(DISPLAY_PADDING_X);
    int areaWidth = wholeWidth;
    thumbWidth = (wholeWidth * m_ThumbnailWidthPercentage / 100) * zoom;
    col = m_ThumbnailDefaultColumnCount / zoom;

    int minWidth = minimumThumbnailWholeSize.width();
    int defWidth = defaultThumbnailWholeSize.width() * zoom;

    if(col < 1) col = 1;

    if(defWidth < minWidth) defWidth = minWidth;

    if(thumbWidth > defWidth){
        col = areaWidth / defWidth;
    }

    if(thumbWidth < minWidth){
        col -= ((minWidth - thumbWidth) * col) / minWidth;
        if(col > 1) col -= 1;
        if(col == 0) col = 1;
    }

    thumbWidth = areaWidth / col;

    const int marginWidth  = defaultThumbnailWholeSize.width()  - defaultSize.width();
    const int marginHeight = defaultThumbnailWholeSize.height() - defaultSize.height();
    const double aspect =
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.height()) /
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.width());

    thumbHeight = ((thumbWidth - marginWidth) * aspect) + marginHeight;
    line = (gtv->m_Size.height() - gtv->ContentsTop()) / thumbHeight;

    if(line < 1) line = 1;
}

void FlatStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter) const {
    painter->setBrush(BackgroundVeil(gtv));
    painter->setPen(Qt::NoPen);
    painter->drawRect(gtv->boundingRect());
    RenderBackgroundOverVeil(gtv, painter);
}

QColor FlatStyle::BackgroundVeil(GraphicsTableView *gtv) const {
    return Theme::Color(Theme::FlatOverlayBackground,
                        gtv->IsDisplayingNode() ? 170 : 100);
}

void FlatStyle::RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const {
    if(gtv->EnableFrameRate()){
        painter->setPen(Theme::Pen(Theme::FlatTextStrong));
        painter->drawText(FrameRateRect(gtv),
                          QStringLiteral("%1fps").arg(gtv->m_CurrentFrameRate));
    }
}

void FlatStyle::Render(Thumbnail *thumb, QPainter *painter) const {
    Node *nd = thumb->GetNode();
    GraphicsTableView *gtv = thumb->GetTableView();
    if(!nd) return;

    QRectF bound = thumb->boundingRect();
    QRectF realRect = bound.translated(thumb->pos());

    if(thumb->scene()){
        if(!realRect.intersects(thumb->scene()->sceneRect()))
            return;
    } else {
        if(!realRect.intersects(QRectF(QPointF(), gtv->Size())))
            return;
    }

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    bool isDir = nd->IsDirectory();

    QRectF rect = bound;
    rect.setBottom(rect.bottom() - 1.0);
    rect.setRight(rect.right() - 1.0);

    QImage image = nd->VisibleImage();

    if(m_ThumbnailDrawBorder){
        painter->setPen(Theme::Pen(Theme::FlatBorder));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    QRectF image_rect = bound;
    image_rect.setLeft(image_rect.left() + gtv->ScaleByDevice(m_ThumbnailPaddingX));
    image_rect.setRight(image_rect.right() - gtv->ScaleByDevice(m_ThumbnailPaddingX));
    image_rect.setTop(image_rect.top() + gtv->ScaleByDevice(m_ThumbnailPaddingY));
    image_rect.setBottom(image_rect.bottom() - gtv->ScaleByDevice(m_ThumbnailPaddingY));

    if(thumb->IsHovered()){
        const qreal inset = gtv->ScaleByDevice(3);
        image_rect.adjust(inset, inset, -inset, -inset);
    }

    QRectF title_rect = image_rect;
    title_rect.setTop(image_rect.bottom() -
                      gtv->ScaleByDevice(m_ThumbnailTitleHeight));

    DrawShadow(painter, image_rect,
               thumb->IsPrimary() ? Theme::FlatShadowPrimary : Theme::FlatShadow,
               gtv->ScaleByDevice(SHADOW_BASE_RADIUS));

    if(!image.isNull()){
        QSizeF size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        QPointF diff = QPointF(image_rect.width()  - size.width(),
                               image_rect.height() - size.height());
        painter->drawImage(QRectF(image_rect.topLeft() + diff / 2.0, size),
                           image, QRectF(QPointF(), image.size()));
    } else {
        QRectF caption_rect = image_rect;
        caption_rect.setBottom(title_rect.top());
        Theme::DrawEmptyThumbnail(painter, image_rect,
                                  isDir ? Theme::FlatPlaceholderDirectory
                                        : Theme::FlatPlaceholderPage,
                                  Theme::FlatText, isDir, caption_rect);
    }

    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::Brush(Theme::FlatTitleBackground));
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->drawRect(title_rect);
    }

    title_rect.setLeft(title_rect.left() + gtv->ScaleByDevice(3));
    title_rect.setRight(title_rect.right() - gtv->ScaleByDevice(4));
    title_rect.setBottom(title_rect.bottom() - 1.0);

    {
        QSize iconSize = gtv->ScaleByDevice(QSize(16, 16));
        QPixmap pixmap = CardIconPixmap(gtv, nd, iconSize);
        if(pixmap.width() > 2){
            painter->drawPixmap(QRect(title_rect.topLeft().toPoint() + QPoint(0, 2), iconSize),
                                pixmap, QRect(QPoint(), pixmap.size()));
            title_rect.setLeft(title_rect.left() + gtv->ScaleByDevice(19));
        }
    }

    {
        painter->setPen(Theme::Pen(Theme::FlatText));
        painter->setBrush(Qt::NoBrush);

        painter->setRenderHint(QPainter::Antialiasing, true);
        DrawTitleText(thumb, painter, title_rect, ThumbnailTitleFont());
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    if(thumb->isSelected()){
        painter->setPen(Theme::Pen(Theme::FlatSelectedBorder));
        painter->setBrush(Theme::Brush(Theme::FlatSelectedFill));
        painter->drawRect(image_rect);
    } else if(thumb->IsPrimary()){
        painter->setPen(Theme::Pen(Theme::FlatPrimaryBorder));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(image_rect);
    }
    painter->restore();
}

void FlatStyle::Render(NodeTitle *title, QPainter *painter) const {
    Q_UNUSED(title) Q_UNUSED(painter)
}

void FlatStyle::Render(SpotLight *title, QPainter *painter) const {
    Q_UNUSED(title) Q_UNUSED(painter)
}

void FlatStyle::Render(InPlaceNotifier *notifier, QPainter *painter) const {
    Q_UNUSED(notifier) Q_UNUSED(painter)
}

void FlatStyle::Render(CloseButton *button, QPainter *painter) const {
    DrawCardCloseButton(button, painter);
}

void FlatStyle::Render(CloneButton *button, QPainter *painter) const {
    DrawCardCloneButton(button, painter);
}

void FlatStyle::Render(SoundButton *button, QPainter *painter) const {
    DrawCardSoundButton(button, painter);
}

void FlatStyle::Render(UpDirectoryButton *button, QPainter *painter) const {
    DrawHeaderButton(button, painter, FlatHeaderColors);
}

void FlatStyle::Render(ToggleTrashButton *button, QPainter *painter) const {
    DrawHeaderButton(button, painter, FlatHeaderColors);
}

void FlatStyle::Render(CloseTableButton *button, QPainter *painter) const {
    DrawHeaderCloseButton(button, painter, FlatHeaderColors);
}

void FlatStyle::Render(TableHeader *header, QPainter *painter) const {
    DrawHeader(header, painter, FlatHeaderColors);
}

void FlatStyle::Render(HeaderFilter *filter, QPainter *painter) const {
    DrawHeaderFilter(filter, painter, FlatHeaderColors);
}

void FlatStyle::Render(AccessibleWebElement *awe, QPainter *painter) const {
    if(awe->GetBoundingPos().isNull() || !awe->GetElement() || awe->GetElement()->IsNull()) return;

    QUrl url = awe->GetElement()->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    painter->save();

    if(!awe->IsSelected()){

        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::Brush(Theme::FlatAccessKeyChip));
        painter->drawRect(awe->CharChipRect());

        painter->setFont(awe->CharChipFont());
        painter->setPen(Theme::Pen(Theme::FlatText,
                                   awe->IsCurrentBlock() ? 255 : 127));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(AccessibleWebElement::CapCenteredRect(awe->CharChipRect(), awe->CharChipFont()),
                          Qt::AlignCenter, awe->GetGadgets()->IndexToString(awe->GetIndex()));

    } else {
        int width = AccessibleWebElement::GetInfoMetrics().boundingRect(str).width();
        width = width + ScaleByDevice(15) - str.length()*0.4;

        if(width > ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH))
            width = ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH);

        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QPoint base(awe->GetBoundingPos() - QPoint(width/2, infoHeight/2));
        QRect rect = QRect(base, QSize(width, infoHeight));

        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::Brush(Theme::FlatAccessKeyChip));
            painter->drawRect(rect);
        }

        rect = QRect(QPoint(base.x()+2, base.y()), QSize(width-2, infoHeight));

        {
            painter->setFont(AccessKeyInfoFont());
            painter->setPen(Theme::Pen(Theme::FlatText));
            painter->setBrush(Qt::NoBrush);
            painter->drawText(rect, str);
        }

        QMap<QString, QRect> keyrectmap = awe->KeyRects();
        QMap<QString, QRect> exprectmap = awe->ExpRects();

        QStringList actions = awe->GetGadgets()->GetAccessKeyKeyMap().values();
        actions.removeDuplicates();
        for(QString action : std::as_const(actions)){

            QStringList list;

            for(QKeySequence seq : awe->GetGadgets()->GetAccessKeyKeyMap().keys(action)){
                list << seq.toString();
            }

            QString key = list.join(QStringLiteral(" or "));
            QString exp = action;
            QRect keyrect = keyrectmap[action];
            QRect exprect = exprectmap[action];

            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::Brush(Theme::FlatAccessKeyChip));
            painter->drawRect(keyrect);
            painter->drawRect(exprect);

            painter->setPen(Theme::Pen(Theme::FlatText));
            painter->setBrush(Qt::NoBrush);

            painter->setFont(AccessKeyChipSFont());
            painter->drawText(keyrect, Qt::AlignCenter, key);

            painter->setFont(AccessKeyInfoFont());
            painter->drawText(exprect.translated(QPoint(0,-1)), Qt::AlignCenter, exp);
        }
    }
    painter->restore();
}

void FlatStyle::OnSetNest(Thumbnail *thumb, int nest) const {
    Q_UNUSED(nest)
    thumb->setOpacity(1.0);
}

void FlatStyle::OnSetNest(NodeTitle *title, int nest) const {
    Q_UNUSED(nest)
    title->setOpacity(1.0);
}

void FlatStyle::OnSetPrimary(Thumbnail *thumb, bool) const {
    if(thumb->graphicsEffect()) thumb->setGraphicsEffect(nullptr);
}

void FlatStyle::OnSetPrimary(NodeTitle *title, bool primary) const {
    Q_UNUSED(title) Q_UNUSED(primary)
}

void FlatStyle::OnSetHovered(Thumbnail *thumb, bool hovered) const {
    Q_UNUSED(thumb) Q_UNUSED(hovered)
}

void FlatStyle::OnSetHovered(NodeTitle *title, bool hovered) const {
    Q_UNUSED(title) Q_UNUSED(hovered)
}

void FlatStyle::OnSetState(GraphicsButton *button, GraphicsButton::ButtonState state) const {
    if(!button->isVisible()) return;

    if(dynamic_cast<HeaderButton*>(button)){
        if(button->graphicsEffect()) button->setGraphicsEffect(nullptr);
        button->setCursor(state == GraphicsButton::NotHovered ? Qt::ArrowCursor : Qt::PointingHandCursor);
        button->update();
        return;
    }

    if(!button->graphicsEffect()){
        QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
        button->setGraphicsEffect(effect);
        effect->setBlurRadius(button->GetTableView()->ScaleByDevice(10));
        effect->setOffset(QPointF());
    }
    QGraphicsDropShadowEffect *shadow =
        static_cast<QGraphicsDropShadowEffect*>(button->graphicsEffect());
    switch(state){
    case GraphicsButton::NotHovered:
        shadow->setColor(Theme::Color(Theme::FlatShadowSoft));
        button->setCursor(Qt::ArrowCursor);
        break;
    case GraphicsButton::Hovered:
        shadow->setColor(Theme::Color(Theme::FlatShadow));
        button->setCursor(Qt::PointingHandCursor);
        break;
    case GraphicsButton::Pressed:
        shadow->setColor(Theme::Color(Theme::FlatShadowLight));
        button->setCursor(Qt::PointingHandCursor);
        break;
    }
    button->update();
}

void FlatStyle::OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const {
    Q_UNUSED(elem)
    if(awe->graphicsEffect()) return;
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
    effect->setColor(Theme::Color(Theme::FlatShadow));
    effect->setBlurRadius(ScaleByDevice(10));
    effect->setOffset(QPointF());
    awe->setGraphicsEffect(effect);
}

void FlatStyle::OnReshow(QGraphicsRectItem *gri) const {
    gri->setPen(Theme::Pen(Theme::FlatSelectRectBorder));
    gri->setBrush(Theme::Brush(Theme::FlatSelectRectFill));
}

QRectF FlatStyle::ThumbnailAreaRect(GraphicsTableView *gtv) const {
    return QRectF(gtv->ScaleByDevice(DISPLAY_PADDING_LEFT), 0,
                  gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount,
                  gtv->m_Size.height());
}

QRectF FlatStyle::NodeTitleAreaRect(GraphicsTableView *gtv) const {
    return QRectF(gtv->ScaleByDevice(DISPLAY_PADDING_LEFT)
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + gtv->ScaleByDevice(GADGETS_SCROLL_BAR_MARGIN) * 2
                  + gtv->ScaleByDevice(GADGETS_SCROLL_BAR_WIDTH),
                  gtv->ScaleByDevice(DISPLAY_PADDING_Y),
                  0,0);
}

QRectF FlatStyle::ScrollBarAreaRect(GraphicsTableView *gtv) const {
    const int paddingY = gtv->ScaleByDevice(DISPLAY_PADDING_Y);
    const int top = gtv->ContentsTop();
    return QRectF(gtv->m_Size.width() - gtv->ScaleByDevice(DISPLAY_PADDING_X) - 1
                  + gtv->ScaleByDevice(GADGETS_SCROLL_BAR_MARGIN),
                  top,
                  gtv->ScaleByDevice(GADGETS_SCROLL_BAR_WIDTH),
                  gtv->m_Size.height() - top - paddingY);
}

QGraphicsRectItem *FlatStyle::CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const {
    return gtv->scene()->addRect(QRectF(pos, pos),
                                 Theme::Pen(Theme::FlatSelectRectBorder),
                                 Theme::Brush(Theme::FlatSelectRectFill));
}

QRectF FlatStyle::ThumbnailHitRect(const Thumbnail *thumb) const {
    GraphicsTableView *gtv = thumb->GetTableView();
    const qreal dx = gtv->ScaleByDevice(m_ThumbnailPaddingX);
    const qreal dy = gtv->ScaleByDevice(m_ThumbnailPaddingY);
    return thumb->boundingRect().adjusted(dx, dy, -dx, -dy);
}

void GadgetsStyle::Render(ScrollIndicator *indicator, QPainter *painter) const {
    painter->save();

    painter->setPen(Theme::Pen(Theme::GadgetsScrollIndicatorBorder));
    painter->setBrush(Theme::Brush(indicator->isSelected()
                                   ? Theme::GadgetsScrollIndicatorSelected
                                   : Theme::GadgetsScrollIndicator));

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->drawRect(indicator->rect());

    painter->restore();
}

const int PlainStyle::m_CardMargin = 3;
const int PlainStyle::m_ImagePadding = 7;
const int PlainStyle::m_TitleHeight = 22;
const int PlainStyle::m_CornerRadius = 5;
const int PlainStyle::m_HoverGrowth = 2;
const int PlainStyle::m_RingWidth = 2;
const int PlainStyle::m_EdgeWidth = 1;
const int PlainStyle::m_ScrollThumbWidth = 6;
const int PlainStyle::m_RoundedCellMinWidth = 100;

namespace {
    const int PLAIN_TRANSLUCENT_ALPHA = 170;
}

void PlainStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter) const {
    if(!gtv->IsDisplayingNode()){
        FlatStyle::RenderBackground(gtv, painter);
        return;
    }
    painter->fillRect(gtv->boundingRect(), BackgroundVeil(gtv));
    RenderBackgroundOverVeil(gtv, painter);
}

QColor PlainStyle::BackgroundVeil(GraphicsTableView *gtv) const {
    if(!gtv->IsDisplayingNode()) return FlatStyle::BackgroundVeil(gtv);
    return Application::EnableTransparentWindow()
        ? Theme::Color(Theme::PlainSurface, PLAIN_TRANSLUCENT_ALPHA)
        : Theme::Color(Theme::PlainSurface);
}

void PlainStyle::RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const {
    if(!gtv->IsDisplayingNode()){
        FlatStyle::RenderBackgroundOverVeil(gtv, painter);
        return;
    }
    if(gtv->EnableFrameRate()){
        painter->setPen(Theme::Pen(Theme::PlainTextStrong));
        painter->drawText(FrameRateRect(gtv),
                          QStringLiteral("%1fps").arg(gtv->m_CurrentFrameRate));
    }
}

bool PlainStyle::CoversViewBehind(GraphicsTableView *gtv) const {
    return gtv->IsDisplayingNode() && !Application::EnableTransparentWindow();
}

void PlainStyle::Render(Thumbnail *thumb, QPainter *painter) const {
    Node *nd = thumb->GetNode();
    GraphicsTableView *gtv = thumb->GetTableView();
    if(!nd) return;

    const QRectF bound = thumb->boundingRect();
    const QRectF realRect = bound.translated(thumb->pos());

    if(thumb->scene()){
        if(!realRect.intersects(thumb->scene()->sceneRect()))
            return;
    } else {
        if(!realRect.intersects(QRectF(QPointF(), gtv->Size())))
            return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(Qt::NoPen);

    const bool selected = thumb->isSelected();
    const bool hovered  = thumb->IsHovered();
    const bool primary  = thumb->IsPrimary();

    const QColor surface = Theme::Color(Theme::PlainSurface);
    const QColor accent  = Theme::Color(Theme::PlainAccent);
    const QColor around  = selected ? Theme::Color(Theme::PlainSelected)
                         : hovered  ? Theme::Color(Theme::PlainHovered)
                         : surface;
    const int radius = gtv->ScaleByDevice(m_CornerRadius);

    const QRect cell(qRound(bound.left()), qRound(bound.top()),
                     qRound(bound.right())  - qRound(bound.left()),
                     qRound(bound.bottom()) - qRound(bound.top()));
    const int margin = gtv->ScaleByDevice(m_CardMargin);
    const QRect plate = cell.adjusted(margin, margin, -margin, -margin);

    const bool translucent = Application::EnableTransparentWindow();
    const bool shaped = translucent
        && gtv->m_CurrentThumbnailWidth >= gtv->ScaleByDevice(m_RoundedCellMinWidth);

    if(around != surface && !plate.isEmpty()){
        if(translucent){
            QColor over = around;
            over.setAlpha(PLAIN_TRANSLUCENT_ALPHA);
            if(shaped){
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->setBrush(over);
                painter->drawRoundedRect(QRectF(plate), radius, radius);
                painter->restore();
            } else {
                painter->fillRect(plate, over);
            }
        } else {
            painter->fillRect(plate, around);
            DrawCardCorners(painter, plate, surface, QColor(Qt::transparent), radius, 0);
        }
    }

    const int padding = gtv->ScaleByDevice(m_ImagePadding);
    const int titleHeight = gtv->ScaleByDevice(m_TitleHeight);
    const QRect box = plate.adjusted(padding, padding, -padding, -titleHeight);

    if(box.width() > 0 && box.height() > 0){
        const int growth = hovered ? gtv->ScaleByDevice(m_HoverGrowth) : 0;
        const QRect picture = box.adjusted(-growth, -growth, growth, growth);
        const QImage image = nd->VisibleImage();
        const bool isDir = nd->IsDirectory();
        const Theme::Role placeholder = isDir ? Theme::PlainPlaceholderDirectory
                                              : Theme::PlainPlaceholderPage;
        const bool ringed = selected || primary;
        const QColor edge = ringed ? accent : Theme::Color(Theme::PlainDivider);
        const int edgeWidth = gtv->ScaleByDevice(ringed ? m_RingWidth : m_EdgeWidth);
        if(shaped){
            DrawRoundedCard(painter, picture,
                            image.isNull() ? Theme::Brush(placeholder)
                            : CardPictureBrush(image, CoverSource(image.size(), picture.size()), picture),
                            edge, radius, edgeWidth);
            if(image.isNull())
                Theme::DrawEmptyThumbnail(painter, picture, Theme::Transparent, Theme::PlainText, isDir);
        } else {
            if(!image.isNull())
                painter->drawImage(picture, image, CoverSource(image.size(), picture.size()));
            else
                Theme::DrawEmptyThumbnail(painter, picture, placeholder, Theme::PlainText, isDir);
            DrawCardCorners(painter, picture, around, edge, translucent ? 0 : radius, edgeWidth);
        }
    }

    QRectF titleRect(box.left(), box.bottom() + 1, box.width(), titleHeight);
    if(titleRect.width() > 0 && titleRect.top() >= plate.top()){
        int elideWidth = gtv->m_CurrentThumbnailWidth - (margin + padding) * 2;
        const QSize iconSize = gtv->ScaleByDevice(QSize(16, 16));
        const QPixmap pixmap = CardIconPixmap(gtv, nd, iconSize);
        if(pixmap.width() > 2){
            const QPoint at(qRound(titleRect.left()),
                            qRound(titleRect.center().y() - iconSize.height() / 2.0));
            painter->drawPixmap(QRect(at, iconSize), pixmap, QRect(QPoint(), pixmap.size()));
            const int taken = iconSize.width() + gtv->ScaleByDevice(4);
            titleRect.setLeft(titleRect.left() + taken);
            elideWidth -= taken;
        }

        painter->setPen(Theme::Pen(selected || hovered || primary
                                   ? Theme::PlainTextStrong : Theme::PlainText));
        painter->setBrush(Qt::NoBrush);
        painter->setRenderHint(QPainter::Antialiasing, true);
        DrawTitleText(thumb, painter, titleRect, ThumbnailTitleFont(), qMax(0, elideWidth));
    }

    painter->restore();
}

void PlainStyle::Render(CloseButton *button, QPainter *painter) const {
    DrawCardCloseButton(button, painter);
}

void PlainStyle::Render(CloneButton *button, QPainter *painter) const {
    DrawCardCloneButton(button, painter);
}

void PlainStyle::Render(SoundButton *button, QPainter *painter) const {
    DrawCardSoundButton(button, painter);
}

void PlainStyle::Render(UpDirectoryButton *button, QPainter *painter) const {
    DrawHeaderButton(button, painter, PlainHeaderButtonColors);
}

void PlainStyle::Render(ToggleTrashButton *button, QPainter *painter) const {
    DrawHeaderButton(button, painter, PlainHeaderButtonColors);
}

void PlainStyle::Render(CloseTableButton *button, QPainter *painter) const {
    DrawHeaderCloseButton(button, painter, PlainHeaderButtonColors);
}

void PlainStyle::Render(TableHeader *header, QPainter *painter) const {
    DrawHeader(header, painter, Application::EnableTransparentWindow()
               ? PlainTranslucentHeaderColors : PlainHeaderColors);
}

void PlainStyle::Render(HeaderFilter *filter, QPainter *painter) const {
    DrawHeaderFilter(filter, painter, PlainHeaderColors);
}

void PlainStyle::Render(ScrollIndicator *indicator, QPainter *painter) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Theme::Brush(indicator->isSelected() ? Theme::PlainScrollThumbActive
                                                           : Theme::PlainScrollThumb));
    const QRectF rect = indicator->rect();
    const qreal width = static_cast<GraphicsTableView*>(indicator->parentItem())
        ->ScaleByDevice(m_ScrollThumbWidth);
    painter->drawRoundedRect(QRectF(rect.center().x() - width / 2.0, rect.top(), width, rect.height()),
                             width / 2.0, width / 2.0);
    painter->restore();
}

void PlainStyle::OnSetState(GraphicsButton *button, GraphicsButton::ButtonState state) const {
    if(!button->isVisible()) return;

    if(button->graphicsEffect()) button->setGraphicsEffect(nullptr);
    button->setCursor(state == GraphicsButton::NotHovered ? Qt::ArrowCursor : Qt::PointingHandCursor);
    button->update();
}

QRectF PlainStyle::ThumbnailHitRect(const Thumbnail *thumb) const {
    const qreal margin = thumb->GetTableView()->ScaleByDevice(m_CardMargin);
    return thumb->boundingRect().adjusted(margin, margin, -margin, -margin);
}
