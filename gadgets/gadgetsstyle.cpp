#include "switch.hpp"
#include "const.hpp"

#include "gadgetsstyle.hpp"

#include <QPainter>
#include <QGraphicsScene>
#include <QCursor>
#include <QGraphicsDropShadowEffect>

#include "graphicstableview.hpp"
#include "thumbnail.hpp"
#include "nodetitle.hpp"
#include "gadgets.hpp"
#include "accessiblewebelement.hpp"

const QFont GlassStyle::m_ThumbnailTitleFont = QFont(DEFAULT_FONT, 10);

const int GlassStyle::m_ThumbnailPaddingX = 2;
const int GlassStyle::m_ThumbnailPaddingY = 2;
const int GlassStyle::m_ThumbnailTitleHeight = 20;
const int GlassStyle::m_ThumbnailWidthPercentage = 15;
const int GlassStyle::m_ThumbnailDefaultColumnCount = 4;
const int GlassStyle::m_ThumbnailAreaWidthPercentage =
    GlassStyle::m_ThumbnailWidthPercentage *
    GlassStyle::m_ThumbnailDefaultColumnCount;

const bool GlassStyle::m_ThumbnailDrawBorder = false;

const QSize GlassStyle::m_DefaultThumbnailWholeSize =
    QSize(DEFAULT_THUMBNAIL_SIZE.width()  + GlassStyle::m_ThumbnailPaddingX * 2,
          DEFAULT_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2 + GlassStyle::m_ThumbnailTitleHeight);

const QSize GlassStyle::m_MinimumThumbnailWholeSize =
    QSize(MINIMUM_THUMBNAIL_SIZE.width()  + GlassStyle::m_ThumbnailPaddingX * 2,
          MINIMUM_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2 + GlassStyle::m_ThumbnailTitleHeight);;

const QFont GlassStyle::m_NodeTitleFont = QFont(DEFAULT_FONT, 13);

const int GlassStyle::m_NodeTitleHeight = 20;
const bool GlassStyle::m_NodeTitleDrawBorder = false;

const int GlassStyle::m_InPlaceNotifierWidth =
    500 + DEFAULT_THUMBNAIL_SIZE.width() + GlassStyle::m_ThumbnailPaddingX * 3;
const int GlassStyle::m_InPlaceNotifierHeight =
    DEFAULT_THUMBNAIL_SIZE.height() + GlassStyle::m_ThumbnailPaddingY * 2;
const bool GlassStyle::m_InPlaceNotifierDrawBorder = false;

void GlassStyle::ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const {
    const float zoom = gtv->m_CurrentThumbnailZoomFactor; // alias

    int wholeWidth = gtv->m_Size.width() - DISPLAY_PADDING_X * 2;
    int areaWidth = wholeWidth * m_ThumbnailAreaWidthPercentage / 100;
    thumbWidth = (wholeWidth * m_ThumbnailWidthPercentage / 100) * zoom;
    col = m_ThumbnailDefaultColumnCount / zoom;

    int minWidth = m_MinimumThumbnailWholeSize.width();
    int defWidth = m_DefaultThumbnailWholeSize.width() * zoom;

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

    // compute height from width.
    const int marginWidth  = m_DefaultThumbnailWholeSize.width()  - DEFAULT_THUMBNAIL_SIZE.width();
    const int marginHeight = m_DefaultThumbnailWholeSize.height() - DEFAULT_THUMBNAIL_SIZE.height();
    const double aspect =
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.height()) /
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.width());

    thumbHeight = ((thumbWidth - marginWidth) * aspect) + marginHeight;
    line = (gtv->m_Size.height() - DISPLAY_PADDING_Y) / thumbHeight;

    if(line < 1) line = 1;
}

void GlassStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter) const {
    QColor black(0, 0, 0, gtv->IsDisplayingNode() ? 170 : 100);
    QBrush brush(black);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(gtv->boundingRect());
}

void GlassStyle::Render(Thumbnail *thumb, QPainter *painter) const {
    if(!thumb->GetNode() ||
       !thumb->boundingRect().translated(thumb->pos()).intersects(thumb->scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    QRectF bound = thumb->boundingRect();
    QRectF rect = bound;
    rect.setBottomRight(bound.bottomRight() - QPointF(1,1));

    QImage image = thumb->GetNode()->GetImage();
    QString title = thumb->GetNode()->GetTitle();

    const QUrl url = thumb->GetNode()->GetUrl();

    if(image.isNull()){
        Node *tempnode = thumb->GetNode();
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    const qreal start = bound.top();
    const qreal stop  = bound.bottom();

    if(thumb->GetNode()->GetView()){
        static const QColor beg = QColor(255,255,200,0);
        static const QColor end = QColor(255,255,200,44);
        QLinearGradient hasviewgrad;
        hasviewgrad.setStart(0, start);
        hasviewgrad.setFinalStop(0, stop);
        hasviewgrad.setColorAt(static_cast<qreal>(0), beg);
        hasviewgrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush hasviewbrush(hasviewgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hasviewbrush);
        painter->drawRect(rect);
    }

    if(thumb->IsPrimary()){
        static const QColor beg = QColor(0,100,255,0);
        static const QColor end = QColor(0,100,255,77);
        QLinearGradient primarygrad;
        primarygrad.setStart(0, start);
        primarygrad.setFinalStop(0, stop);
        primarygrad.setColorAt(static_cast<qreal>(0), beg);
        primarygrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush primarybrush(primarygrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(primarybrush);
        painter->drawRect(rect);
    }

    if(thumb->IsHovered()){
        static const QColor beg = QColor(255,255,255,0);
        static const QColor end = QColor(255,255,255,77);
        QLinearGradient hoveredgrad;
        hoveredgrad.setStart(0, start);
        hoveredgrad.setFinalStop(0, stop);
        hoveredgrad.setColorAt(static_cast<qreal>(0), beg);
        hoveredgrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush hoveredbrush(hoveredgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hoveredbrush);
        painter->drawRect(rect);
    }

    if(thumb->isSelected()){
        static const QColor beg = QColor(255,200,220,0);
        static const QColor end = QColor(255,200,220,170);
        QLinearGradient selectedgrad;
        selectedgrad.setStart(0, start);
        selectedgrad.setFinalStop(0, stop);
        selectedgrad.setColorAt(static_cast<qreal>(0), beg);
        selectedgrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush selectedbrush(selectedgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(selectedbrush);
        painter->drawRect(rect);
    }

    if(m_ThumbnailDrawBorder){
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    QRect image_rect = rect.toRect();
    image_rect.translate(m_ThumbnailPaddingX, m_ThumbnailPaddingY);
    image_rect.setWidth (rect.width()  - (m_ThumbnailPaddingX * 2));
    image_rect.setHeight(rect.height() - (m_ThumbnailPaddingY * 2 + m_ThumbnailTitleHeight));

    QRect title_rect = image_rect;
    title_rect.moveTop(image_rect.bottom());
    title_rect.setHeight(m_ThumbnailTitleHeight);

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        image_rect.translate(width_diff/2, height_diff/2);
        painter->drawImage(image_rect, image, QRect(QPoint(), image.size()));
    } else if(thumb->GetNode()->IsDirectory()){
        static const QBrush b = QBrush(QColor(50, 100, 100, 150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(image_rect);

        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        static const QBrush b = QBrush(QColor(50, 100, 120, 150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(image_rect);

        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    {
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setFont(m_ThumbnailTitleFont);
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);

        if(title.isEmpty()){
            if(url.isEmpty()){
                if(thumb->GetNode()->IsDirectory()){
                    title = QStringLiteral("Directory");
                } else {
                    title = QStringLiteral("No Title");
                }
            } else {
                title = url.toString();
            }
        } else if(thumb->GetNode()->IsDirectory()){
            title = QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first();
        }
        painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title);
    }
    painter->restore();
}

void GlassStyle::Render(NodeTitle *title, QPainter *painter) const {
    QRectF bound = title->boundingRect();
    QRectF port = title->GetTableView()->NodeTitleAreaRect();

    if(!title->GetNode() || !port.isValid() ||
       !bound.translated(title->pos()).intersects(title->scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    port.moveBottomRight(port.bottomRight() - QPointF(1, 1));
    if(m_NodeTitleDrawBorder) port = port.intersected(bound);

    QString title_ = title->GetNode()->GetTitle();
    const QUrl url = title->GetNode()->GetUrl();

    const qreal start = bound.top();
    const qreal stop  = bound.bottom();

    if(title->GetNode()->GetView()){
        static const QColor beg = QColor(255,255,200,0);
        static const QColor end = QColor(255,255,200,44);
        QLinearGradient hasviewgrad;
        hasviewgrad.setStart(0, start);
        hasviewgrad.setFinalStop(0, stop);
        hasviewgrad.setColorAt(static_cast<qreal>(0), beg);
        hasviewgrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush hasviewbrush(hasviewgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hasviewbrush);
        painter->drawRect(bound);
    }

    if(title->IsPrimary()){
        static const QColor beg = QColor(0,100,255,0);
        static const QColor end = QColor(0,100,255,77);
        QLinearGradient primarygrad;
        primarygrad.setStart(0, start);
        primarygrad.setFinalStop(0, stop);
        primarygrad.setColorAt(static_cast<qreal>(0), beg);
        primarygrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush primarybrush(primarygrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(primarybrush);
        painter->drawRect(bound);
    }

    if(title->IsHovered()){
        static const QColor beg = QColor(255,255,255,0);
        static const QColor end = QColor(255,255,255,77);
        QLinearGradient hoveredgrad;
        hoveredgrad.setStart(0, start);
        hoveredgrad.setFinalStop(0, stop);
        hoveredgrad.setColorAt(static_cast<qreal>(0), beg);
        hoveredgrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush hoveredbrush(hoveredgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(hoveredbrush);
        painter->drawRect(bound);
    }

    if(title->isSelected()){
        static const QColor beg = QColor(255,200,220,0);
        static const QColor end = QColor(255,200,220,170);
        QLinearGradient selectedgrad;
        selectedgrad.setStart(0, start);
        selectedgrad.setFinalStop(0, stop);
        selectedgrad.setColorAt(static_cast<qreal>(0), beg);
        selectedgrad.setColorAt(static_cast<qreal>(1), end);
        const QBrush selectedbrush(selectedgrad);

        painter->setPen(Qt::NoPen);
        painter->setBrush(selectedbrush);
        painter->drawRect(bound);
    }

    QRectF title_rect = bound;
    title_rect.setTop(bound.top() + 1);
    title_rect.setLeft(bound.left() + title->GetNest() * 20 + 5);
    title_rect.intersected(port);

    if(title_.isEmpty()){
        if(url.isEmpty()){
            if(title->GetNode()->IsDirectory()){
                title_ = QStringLiteral("Directory");
            } else {
                title_ = QStringLiteral("No Title");
            }
        } else {
            title_ = url.toString();
        }
    }

    {
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setFont(m_NodeTitleFont);
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);

        const QString prefix = title->GetTableView()->GetDirectoryPrefix(title->GetNode());

        if(!prefix.isEmpty()){
            painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, prefix);
            title_rect.setLeft(title_rect.left() + 15);
        }
        if(title->GetNode()->IsDirectory()){
            title_ = QStringLiteral("Dir - ") + title_.split(QStringLiteral(";")).first();
        }
        painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title_);
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
        l ? light->GetIndex() : -1; // default : -1

    if(index == -1 ||
       index >= parent->m_DisplayThumbnails.length())
        return;

    painter->save();

    const QSize size = parent->m_Size.toSize() + QSize(1, 1);

    const Thumbnail *thumb = parent->m_DisplayThumbnails[index];
    const NodeTitle *title = parent->m_DisplayNodeTitles[index];

    const int x1  = thumb->pos().x() + thumb->rect().right();
    const int x2  = title->pos().x() + title->rect().left();

    const int y1b = thumb->pos().y() + thumb->rect().top();
    const int y1e = thumb->pos().y() + thumb->rect().bottom();

    const int y2b = title->pos().y() + title->rect().top();
    const int y2e = title->pos().y() + title->rect().bottom();

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
#ifdef QT_DEBUG
    image.fill(0);
#endif

    QPen pen;

    if(p){
        for(x = begx; x < endx; x++){
            progress = (x - x1)/xrange;
            begy = y1b + (ybrange * progress);
            endy = y1e + (yerange * progress);
            yrange = endy - begy;
            for(y = begy+1; y < endy-1; y++)
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(  0, 100, 255, 77*(y-begy)/yrange));
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(  0, 100, 255, 50*(y-begy)/yrange));
        }
        pen.setColor(QColor(  0, 100, 255, 50));
    } else if(h){
        for(x = begx; x < endx; x++){
            progress = (x - x1)/xrange;
            begy = y1b + (ybrange * progress);
            endy = y1e + (yerange * progress);
            yrange = endy - begy;
            for(y = begy+1; y < endy-1; y++)
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 255, 77*(y-begy)/yrange));
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 255, 50*(y-begy)/yrange));
        }
        pen.setColor(QColor(255, 255, 255, 50));
    } else if(l){
        for(x = begx; x < endx; x++){
            progress = (x - x1)/xrange;
            begy = y1b + (ybrange * progress);
            endy = y1e + (yerange * progress);
            yrange = endy - begy;
            for(y = begy+1; y < endy-1; y++)
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 200, 44*(y-begy)/yrange));
            if(rect.contains(x, y)) image.setPixel(x, y, qRgba(255, 255, 200, 30*(y-begy)/yrange));
        }
        pen.setColor(QColor(255, 255, 200, 30));
    }

    painter->drawImage(QPoint(), image);

    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(pen);
    painter->drawLine(x1, y1e,
                      x2, y2e);
    painter->drawLine(x1, y1e-1,
                      x2, y2e-1);
    painter->restore();
}

void GlassStyle::Render(InPlaceNotifier *notifier, QPainter *painter) const {
    if(!notifier->GetNode()) return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    QRectF rect = notifier->boundingRect();
    rect.setBottomRight(rect.bottomRight() - QPointF(1, 1));

    QImage image = notifier->GetNode()->GetImage();
    const QString title = notifier->GetNode()->GetTitle().replace(QStringLiteral("\n"), QStringLiteral(" ")).trimmed();
    const QString url = notifier->GetNode()->GetUrl().toString().replace(QStringLiteral("\n"), QStringLiteral(" ")).trimmed();
    const QString create = notifier->GetNode()->GetCreateDate().toString(Qt::SystemLocaleLongDate);
    const QString lastUpdate = notifier->GetNode()->GetLastUpdateDate().toString(Qt::SystemLocaleLongDate);
    const QString lastAccess = notifier->GetNode()->GetLastAccessDate().toString(Qt::SystemLocaleLongDate);

    if(image.isNull()){
        Node *tempnode = notifier->GetNode();
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    {
        static const QBrush b = QBrush(QColor(0,0,0,128));
        painter->setBrush(b);
        painter->setPen(Qt::NoPen);
        painter->drawRect(rect);
    }

    if(m_InPlaceNotifierDrawBorder){
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(notifier->boundingRect());
    }

    const QRect image_rect =
        QRect(QPoint(m_ThumbnailPaddingX,
                     m_ThumbnailPaddingY),
              DEFAULT_THUMBNAIL_SIZE);

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        painter->drawImage(QRect(image_rect.topLeft()
                                 + QPoint(width_diff/2, height_diff/2),
                                 size),
                           image,
                           QRect(QPoint(), image.size()));
    } else if(notifier->GetNode()->IsDirectory()){
        static const QBrush b = QBrush(QColor(50, 100, 100, 150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(image_rect);

        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        static const QBrush b = QBrush(QColor(50, 100, 120, 150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(image_rect);

        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    const int basex = m_ThumbnailPaddingX * 3 + image_rect.size().width();
    const int basey = 17;
    const int width = 495;
    const int height = 25;

    {
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(NOTIFIER_FONT);

        painter->drawText(QRect(basex, basey+25*0, width, height), Qt::AlignLeft, QObject::tr("Title : ") + title);
        painter->drawText(QRect(basex, basey+25*1, width, height), Qt::AlignLeft, QObject::tr("Url : ") + url);
        painter->drawText(QRect(basex, basey+25*2, width, height), Qt::AlignLeft, QObject::tr("CreatedDate : ") + create);
        painter->drawText(QRect(basex, basey+25*4, width, height), Qt::AlignLeft, QObject::tr("LastUpdatedDate : ") + lastUpdate);
        painter->drawText(QRect(basex, basey+25*3, width, height), Qt::AlignLeft, QObject::tr("LastAccessedDate : ") + lastAccess);
    }

    painter->restore();
}

void GlassStyle::Render(UpDirectoryButton *button, QPainter *painter) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor white(255, 255, 255, button->GetHovered() ? 200 : 100);
    QBrush brush(white);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(button->boundingRect(), 3, 3);

    painter->drawPixmap(QRect(QPoint(4,4), QSize(10, 10)), button->GetIcon(), QRect(QPoint(), button->GetIcon().size()));

    painter->restore();
}

void GlassStyle::Render(AccessibleWebElement *awe, QPainter *painter) const {
    if(awe->GetBoundingPos().isNull() || !awe->GetElement() || awe->GetElement()->IsNull()) return;

    QUrl url = awe->GetElement()->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    painter->save();

    if(!awe->IsSelected()){

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0,0,0, awe->IsCurrentBlock() ? 100 : 50));
        painter->drawRect(awe->CharChipRect());

        painter->setFont(awe->CharChipFont());
        painter->setPen(QColor(255,255,255, awe->IsCurrentBlock() ? 200 : 100));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(awe->CharChipRect(), Qt::AlignCenter, awe->GetGadgets()->IndexToString(awe->GetIndex()));

    } else {
        int width = AccessibleWebElement::GetInfoMetrics().boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(awe->GetBoundingPos() - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        QRect rect = QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT));

        {
            static const QBrush b = QBrush(QColor(0,0,0,100));
            painter->setPen(Qt::NoPen);
            painter->setBrush(b);
            painter->drawRect(rect);
        }

        rect = QRect(QPoint(base.x()+2, base.y()), QSize(width-2, ACCESSKEY_INFO_HEIGHT));

        {
            static const QPen p = QPen(QColor(255,255,255,200));
            painter->setFont(ACCESSKEY_INFO_FONT);
            painter->setPen(p);
            painter->setBrush(Qt::NoBrush);
            painter->drawText(rect, str);
        }

        QMap<QString, QRect> keyrectmap = awe->KeyRects();
        QMap<QString, QRect> exprectmap = awe->ExpRects();

        foreach(QString action, awe->GetGadgets()->GetAccessKeyKeyMap().values().toSet()){

            QStringList list;

            foreach(QKeySequence seq, awe->GetGadgets()->GetAccessKeyKeyMap().keys(action)){
                list << seq.toString();
            }

            QString key = list.join(QStringLiteral(" or "));
            QString exp = action;
            QRect keyrect = keyrectmap[action];
            QRect exprect = exprectmap[action];

            static const QBrush b = QBrush(QColor(0,0,0,100));
            painter->setPen(Qt::NoPen);
            painter->setBrush(b);
            painter->drawRect(keyrect);
            painter->drawRect(exprect);

            static const QPen p = QColor(255,255,255,200);
            painter->setPen(p);
            painter->setBrush(Qt::NoBrush);

            painter->setFont(ACCESSKEY_CHAR_CHIP_S_FONT);
            painter->drawText(keyrect, Qt::AlignCenter, key);

            painter->setFont(ACCESSKEY_INFO_FONT);
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
    if(thumb->graphicsEffect()) thumb->setGraphicsEffect(0);
}

void GlassStyle::OnSetPrimary(NodeTitle *title, bool) const {
    if(title->graphicsEffect()) title->setGraphicsEffect(0);
}

void GlassStyle::OnSetHovered(Thumbnail *thumb, bool hovered) const {
    Q_UNUSED(thumb); Q_UNUSED(hovered);
}

void GlassStyle::OnSetHovered(NodeTitle *title, bool hovered) const {
    Q_UNUSED(title); Q_UNUSED(hovered);
}

void GlassStyle::OnSetHovered(UpDirectoryButton *button, bool hovered) const {
    if(button->graphicsEffect()) button->setGraphicsEffect(0);
    if(hovered){
        button->setCursor(Qt::PointingHandCursor);
    } else {
        button->setCursor(Qt::ArrowCursor);
    }
    button->update();
}

void GlassStyle::OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const {
    Q_UNUSED(elem);
    if(awe->graphicsEffect()) awe->setGraphicsEffect(0);
}

void GlassStyle::OnReshow(QGraphicsRectItem *gri) const {
    static const QPen   p = QPen(QColor(255,255,255,255));
    static const QBrush b = QBrush(QColor(255,255,255,50));
    gri->setPen(p);
    gri->setBrush(b);
}

QRectF GlassStyle::ThumbnailAreaRect(GraphicsTableView *gtv) const {
    return QRectF(DISPLAY_PADDING_X, 0,
                  gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount,
                  gtv->m_Size.height());
}

QRectF GlassStyle::NodeTitleAreaRect(GraphicsTableView *gtv) const {
    return QRectF(DISPLAY_PADDING_X
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + GADGETS_SCROLL_BAR_MARGIN * 2
                  + GADGETS_SCROLL_BAR_WIDTH,
                  (m_NodeTitleDrawBorder ? DISPLAY_PADDING_Y : 0),
                  gtv->m_Size.width()
                  - DISPLAY_PADDING_X
                  - gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  - GADGETS_SCROLL_BAR_MARGIN * 2
                  - GADGETS_SCROLL_BAR_WIDTH -
                  (m_NodeTitleDrawBorder ? DISPLAY_PADDING_X : 0),
                  gtv->m_Size.height() -
                  (m_NodeTitleDrawBorder ? DISPLAY_PADDING_Y * 2 : 0));
}

QRectF GlassStyle::ScrollBarAreaRect(GraphicsTableView *gtv) const {
    return QRectF(DISPLAY_PADDING_X
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + GADGETS_SCROLL_BAR_MARGIN,
                  DISPLAY_PADDING_Y,
                  GADGETS_SCROLL_BAR_WIDTH,
                  gtv->m_Size.height() - DISPLAY_PADDING_Y * 2);
}

QGraphicsRectItem *GlassStyle::CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const {
    return gtv->scene()->addRect(QRectF(pos, pos),
                                 QPen(QColor(255,255,255,255)),
                                 QBrush(QColor(255,255,255,50)));
}


const QFont FlatStyle::m_ThumbnailTitleFont = QFont(DEFAULT_FONT, 10);

const int FlatStyle::m_ThumbnailPaddingX = 15;
const int FlatStyle::m_ThumbnailPaddingY = 15;
const int FlatStyle::m_ThumbnailTitleHeight = 20;
const int FlatStyle::m_ThumbnailWidthPercentage = 20;
const int FlatStyle::m_ThumbnailDefaultColumnCount = 5;
const int FlatStyle::m_ThumbnailAreaWidthPercentage =
    FlatStyle::m_ThumbnailWidthPercentage *
    FlatStyle::m_ThumbnailDefaultColumnCount;

const bool FlatStyle::m_ThumbnailDrawBorder = false;

const QSize FlatStyle::m_DefaultThumbnailWholeSize =
    QSize(DEFAULT_THUMBNAIL_SIZE.width()  + FlatStyle::m_ThumbnailPaddingX * 2,
          DEFAULT_THUMBNAIL_SIZE.height() + FlatStyle::m_ThumbnailPaddingY * 2);

const QSize FlatStyle::m_MinimumThumbnailWholeSize =
    QSize(MINIMUM_THUMBNAIL_SIZE.width()  + FlatStyle::m_ThumbnailPaddingX * 2,
          MINIMUM_THUMBNAIL_SIZE.height() + FlatStyle::m_ThumbnailPaddingY * 2);

void FlatStyle::ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const {
    const float zoom = gtv->m_CurrentThumbnailZoomFactor; // alias

    int wholeWidth = gtv->m_Size.width() - DISPLAY_PADDING_X * 2;
    int areaWidth = wholeWidth;
    thumbWidth = (wholeWidth * m_ThumbnailWidthPercentage / 100) * zoom;
    col = m_ThumbnailDefaultColumnCount / zoom;

    int minWidth = m_MinimumThumbnailWholeSize.width();
    int defWidth = m_DefaultThumbnailWholeSize.width() * zoom;

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

    // compute height from width.
    const int marginWidth  = m_DefaultThumbnailWholeSize.width()  - DEFAULT_THUMBNAIL_SIZE.width();
    const int marginHeight = m_DefaultThumbnailWholeSize.height() - DEFAULT_THUMBNAIL_SIZE.height();
    const double aspect =
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.height()) /
        static_cast<double>(DEFAULT_THUMBNAIL_SIZE.width());

    thumbHeight = ((thumbWidth - marginWidth) * aspect) + marginHeight;
    line = (gtv->m_Size.height() - DISPLAY_PADDING_Y) / thumbHeight;

    if(line < 1) line = 1;
}

void FlatStyle::RenderBackground(GraphicsTableView *gtv, QPainter *painter) const {
    QColor white(255, 255, 255, gtv->IsDisplayingNode() ? 170 : 100);
    QBrush brush(white);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(gtv->boundingRect());
}

void FlatStyle::Render(Thumbnail *thumb, QPainter *painter) const {
    if(!thumb->GetNode() ||
       !thumb->boundingRect().translated(thumb->pos()).intersects(thumb->scene()->sceneRect()))
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, false);

    QRectF bound = thumb->boundingRect();
    QRectF rect = bound;
    rect.setBottomRight(bound.bottomRight() - QPointF(1,1));

    QImage image = thumb->GetNode()->GetImage();
    QString title = thumb->GetNode()->GetTitle();

    const QUrl url = thumb->GetNode()->GetUrl();

    if(image.isNull()){
        Node *tempnode = thumb->GetNode();
        if(tempnode->IsViewNode() && tempnode->IsDirectory()){
            while(!tempnode->HasNoChildren()){
                if(tempnode->GetPrimary()){
                    tempnode = tempnode->GetPrimary();
                } else {
                    tempnode = tempnode->GetFirstChild();
                }
            }
            if(!tempnode->GetImage().isNull()){
                image = tempnode->GetImage();
            }
        }
    }

    if(m_ThumbnailDrawBorder){
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }

    // hovered : enlarge
    // primary : effect
    // selected : frame
    // loaded : none

    QRect image_rect = rect.toRect();
    image_rect.translate(m_ThumbnailPaddingX, m_ThumbnailPaddingY);
    image_rect.setWidth (rect.width()  - (m_ThumbnailPaddingX * 2));
    image_rect.setHeight(rect.height() - (m_ThumbnailPaddingY * 2));

    QRect title_rect = image_rect;
    title_rect.setTop(image_rect.bottom() - m_ThumbnailTitleHeight);

    if(thumb->IsHovered()){
        image_rect = QRect(image_rect.topLeft() - QPoint(3, 3),
                           image_rect.size() + QSize(6, 6));
        title_rect = QRect(title_rect.topLeft() + QPoint(-3, 4),
                           title_rect.size() + QSize(6, 0));
    }

    if(!image.isNull()){
        QSize size = image.size();
        size.scale(image_rect.size(), Qt::KeepAspectRatio);
        const int width_diff  = image_rect.width()  - size.width();
        const int height_diff = image_rect.height() - size.height();
        image_rect.translate(width_diff/2, height_diff/2);
        painter->drawImage(image_rect, image, QRect(QPoint(), image.size()));
    } else if(thumb->GetNode()->IsDirectory()){
        static const QBrush b = QBrush(QColor(50, 100, 100, 150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(image_rect);

        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        image_rect.setBottom(title_rect.center().y());
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("Directory"));
    } else {
        static const QBrush b = QBrush(QColor(50, 100, 120, 150));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(image_rect);

        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(DEFAULT_FONT, image_rect.size().height() / 7.5));
        painter->setRenderHint(QPainter::Antialiasing, true);
        image_rect.setBottom(title_rect.center().y());
        painter->drawText(image_rect, Qt::AlignCenter, QStringLiteral("NoImage"));
    }

    {
        static const QBrush b = QBrush(QColor(0,0,0,128));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(title_rect);
    }

    title_rect = QRect(title_rect.topLeft() + QPoint(2,0),
                       title_rect.size() - QSize(4,0));

    {
        static const QPen p = QPen(QColor(255,255,255,255));
        painter->setFont(m_ThumbnailTitleFont);
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);

        if(title.isEmpty()){
            if(url.isEmpty()){
                if(thumb->GetNode()->IsDirectory()){
                    title = QStringLiteral("Directory");
                } else {
                    title = QStringLiteral("No Title");
                }
            } else {
                title = url.toString();
            }
        } else if(thumb->GetNode()->IsDirectory()){
            title = QStringLiteral("Dir - ") + title.split(QStringLiteral(";")).first();
        }
        painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title);

        if(thumb->isSelected()){
            painter->setPen(QColor(100,100,255,255));
            painter->setBrush(QColor(100,100,255,50));
            painter->drawRect(QRect(image_rect.topLeft(),
                                    image_rect.size()-QSize(1,1)));
        }
    }
    painter->restore();
}

void FlatStyle::Render(NodeTitle *title, QPainter *painter) const {
    Q_UNUSED(title); Q_UNUSED(painter);
}

void FlatStyle::Render(SpotLight *title, QPainter *painter) const {
    Q_UNUSED(title); Q_UNUSED(painter);
}

void FlatStyle::Render(InPlaceNotifier *notifier, QPainter *painter) const {
    Q_UNUSED(notifier); Q_UNUSED(painter);
}

void FlatStyle::Render(UpDirectoryButton *button, QPainter *painter) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    static const QBrush b = QBrush(QColor(255, 255, 255, 255));
    painter->setBrush(b);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(button->boundingRect(), 3, 3);

    painter->drawPixmap(QRect(QPoint(4,4), QSize(10, 10)), button->GetIcon(), QRect(QPoint(), button->GetIcon().size()));

    painter->restore();
}

void FlatStyle::Render(AccessibleWebElement *awe, QPainter *painter) const {
    if(awe->GetBoundingPos().isNull() || !awe->GetElement() || awe->GetElement()->IsNull()) return;

    QUrl url = awe->GetElement()->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    painter->save();

    if(!awe->IsSelected()){

        static const QBrush b = QBrush(QColor(255,255,255,255));
        painter->setPen(Qt::NoPen);
        painter->setBrush(b);
        painter->drawRect(awe->CharChipRect());

        painter->setFont(awe->CharChipFont());
        painter->setPen(QColor(0,0,0, awe->IsCurrentBlock() ? 255 : 127));
        painter->setBrush(Qt::NoBrush);
        painter->drawText(awe->CharChipRect(), Qt::AlignCenter, awe->GetGadgets()->IndexToString(awe->GetIndex()));

    } else {
        int width = AccessibleWebElement::GetInfoMetrics().boundingRect(str).width();
        // is there any good way?
        width = width + 15 - str.length()*0.4;

        if(width > ACCESSKEY_INFO_MAX_WIDTH)
            width = ACCESSKEY_INFO_MAX_WIDTH;

        QPoint base(awe->GetBoundingPos() - QPoint(width/2, ACCESSKEY_INFO_HEIGHT/2));
        QRect rect = QRect(base, QSize(width, ACCESSKEY_INFO_HEIGHT));

        {
            static const QBrush b = QBrush(QColor(255,255,255,255));
            painter->setPen(Qt::NoPen);
            painter->setBrush(b);
            painter->drawRect(rect);
        }

        rect = QRect(QPoint(base.x()+2, base.y()), QSize(width-2, ACCESSKEY_INFO_HEIGHT));

        {
            static const QPen p = QPen(QColor(0,0,0,255));
            painter->setFont(ACCESSKEY_INFO_FONT);
            painter->setPen(p);
            painter->setBrush(Qt::NoBrush);
            painter->drawText(rect, str);
        }

        QMap<QString, QRect> keyrectmap = awe->KeyRects();
        QMap<QString, QRect> exprectmap = awe->ExpRects();

        foreach(QString action, awe->GetGadgets()->GetAccessKeyKeyMap().values().toSet()){

            QStringList list;

            foreach(QKeySequence seq, awe->GetGadgets()->GetAccessKeyKeyMap().keys(action)){
                list << seq.toString();
            }

            QString key = list.join(QStringLiteral(" or "));
            QString exp = action;
            QRect keyrect = keyrectmap[action];
            QRect exprect = exprectmap[action];

            static const QBrush b = QBrush(QColor(255,255,255,255));
            painter->setPen(Qt::NoPen);
            painter->setBrush(b);
            painter->drawRect(keyrect);
            painter->drawRect(exprect);

            static const QPen p = QPen(QColor(0,0,0,255));
            painter->setPen(p);
            painter->setBrush(Qt::NoBrush);

            painter->setFont(ACCESSKEY_CHAR_CHIP_S_FONT);
            painter->drawText(keyrect, Qt::AlignCenter, key);

            painter->setFont(ACCESSKEY_INFO_FONT);
            painter->drawText(exprect.translated(QPoint(0,-1)), Qt::AlignCenter, exp);
        }
    }
    painter->restore();
}

void FlatStyle::OnSetNest(Thumbnail *thumb, int nest) const {
    Q_UNUSED(thumb); Q_UNUSED(nest);
}

void FlatStyle::OnSetNest(NodeTitle *title, int nest) const {
    Q_UNUSED(title); Q_UNUSED(nest);
}

void FlatStyle::OnSetPrimary(Thumbnail *thumb, bool primary) const {
    if(!thumb->graphicsEffect()){
        QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
        thumb->setGraphicsEffect(effect);
        effect->setBlurRadius(10);
        effect->setOffset(QPointF());
    }
    if(primary){
        static const QColor c = QColor(0,0,100,255);
        static_cast<QGraphicsDropShadowEffect*>(thumb->graphicsEffect())->setColor(c);
    } else {
        static const QColor c = QColor(0,0,0,255);
        static_cast<QGraphicsDropShadowEffect*>(thumb->graphicsEffect())->setColor(c);
    }
}

void FlatStyle::OnSetPrimary(NodeTitle *title, bool primary) const {
    Q_UNUSED(title); Q_UNUSED(primary);
}

void FlatStyle::OnSetHovered(Thumbnail *thumb, bool hovered) const {
    Q_UNUSED(thumb); Q_UNUSED(hovered);
}

void FlatStyle::OnSetHovered(NodeTitle *title, bool hovered) const {
    Q_UNUSED(title); Q_UNUSED(hovered);
}

void FlatStyle::OnSetHovered(UpDirectoryButton *button, bool hovered) const {
    if(!button->graphicsEffect()){
        QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
        button->setGraphicsEffect(effect);
        effect->setBlurRadius(10);
        effect->setOffset(QPointF());
    }
    if(hovered){
        static const QColor c = QColor(0,0,0,255);
        static_cast<QGraphicsDropShadowEffect*>(button->graphicsEffect())->setColor(c);
        button->setCursor(Qt::PointingHandCursor);
    } else {
        static const QColor c = QColor(0,0,0,127);
        static_cast<QGraphicsDropShadowEffect*>(button->graphicsEffect())->setColor(c);
        button->setCursor(Qt::ArrowCursor);
    }
    button->update();
}

void FlatStyle::OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const {
    Q_UNUSED(elem);
    if(awe->graphicsEffect()) return;
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
    static const QColor c = QColor(0,0,0,255);
    effect->setColor(c);
    effect->setBlurRadius(10);
    effect->setOffset(QPointF());
    awe->setGraphicsEffect(effect);
}

void FlatStyle::OnReshow(QGraphicsRectItem *gri) const {
    static const QPen   p = QPen(QColor(100,100,255,255));
    static const QBrush b = QBrush(QColor(100,100,255,50));
    gri->setPen(p);
    gri->setBrush(b);
}

QRectF FlatStyle::ThumbnailAreaRect(GraphicsTableView *gtv) const {
    return QRectF(DISPLAY_PADDING_X, 0,
                  gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount,
                  gtv->m_Size.height());
}

QRectF FlatStyle::NodeTitleAreaRect(GraphicsTableView *gtv) const {
    return QRectF(DISPLAY_PADDING_X
                  + gtv->m_CurrentThumbnailWidth * gtv->m_CurrentThumbnailColumnCount
                  + GADGETS_SCROLL_BAR_MARGIN * 2
                  + GADGETS_SCROLL_BAR_WIDTH,
                  DISPLAY_PADDING_Y,
                  0,0);
}

QRectF FlatStyle::ScrollBarAreaRect(GraphicsTableView *gtv) const {
    return QRectF(gtv->m_Size.width() - DISPLAY_PADDING_X - 1
                  + GADGETS_SCROLL_BAR_MARGIN,
                  DISPLAY_PADDING_Y,
                  GADGETS_SCROLL_BAR_WIDTH,
                  gtv->m_Size.height() - DISPLAY_PADDING_Y * 2);
}

QGraphicsRectItem *FlatStyle::CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const {
    return gtv->scene()->addRect(QRectF(pos, pos),
                                 QPen(QColor(100,100,255,255)),
                                 QBrush(QColor(100,100,255,50)));
}