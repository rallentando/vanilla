#include "switch.hpp"
#include "const.hpp"

#include "nodepreview.hpp"

#include <QPainter>
#include <QFontMetrics>
#include <QScreen>
#include <QGuiApplication>

#include "theme.hpp"
#include "devicescale.hpp"

namespace {

    const QSize PREVIEW_IMAGE_AREA = QSize(260, 195);
    const int PREVIEW_PADDING = 6;
    const int PREVIEW_CAPTION_HEIGHT = 20;

    int ScaleByDevice(int t){
        return DeviceScale::Primary(t);
    }
    const int PREVIEW_DELAY_MS = 500;
    const int PREVIEW_GAP = 4;
}

NodePreview *NodePreview::Instance(){
    static NodePreview *instance = new NodePreview();
    return instance;
}

NodePreview::NodePreview()
    : QWidget(nullptr,
              Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    resize(SizeForImageArea(QSize(ScaleByDevice(PREVIEW_IMAGE_AREA.width()),
                                  ScaleByDevice(PREVIEW_IMAGE_AREA.height()))));

    m_Timer.setSingleShot(true);
    m_Timer.setInterval(PREVIEW_DELAY_MS);
    connect(&m_Timer, &QTimer::timeout, this, &NodePreview::Appear);
}

QSize NodePreview::SizeForImageArea(const QSize &imageArea){
    return QSize(imageArea.width()
                 + ScaleByDevice(PREVIEW_PADDING) * 2,
                 imageArea.height()
                 + ScaleByDevice(PREVIEW_PADDING) * 2
                 + ScaleByDevice(PREVIEW_CAPTION_HEIGHT));
}

QRect NodePreview::Place(const QRect &itemRect, const QSize &size, const QRect &screen, int gap){
    const int screenLeft   = screen.x();
    const int screenTop    = screen.y();
    const int screenRight  = screen.x() + screen.width();
    const int screenBottom = screen.y() + screen.height();

    int x = itemRect.center().x() - size.width() / 2;
    int y = itemRect.y() + itemRect.height() + gap;

    if(y + size.height() > screenBottom)
        y = itemRect.y() - gap - size.height();

    if(y + size.height() > screenBottom) y = screenBottom - size.height();
    if(y < screenTop) y = screenTop;

    if(x + size.width() > screenRight) x = screenRight - size.width();
    if(x < screenLeft) x = screenLeft;

    return QRect(QPoint(x, y), size);
}

void NodePreview::Request(const QImage &image, const QString &title,
                          const QRect &itemRect, bool isDirectory){
    const bool same = (m_ItemRect == itemRect && m_Title == title);

    m_Image = image;
    m_Title = title;
    m_ItemRect = itemRect;
    m_IsDirectory = isDirectory;

    if(isVisible()){
        Appear();
    } else if(!same || !m_Timer.isActive()){
        m_Timer.start();
    }
}

bool NodePreview::IsAbout(const QString &title, const QRect &itemRect) const {
    return m_ItemRect == itemRect && m_Title == title
        && (isVisible() || m_Timer.isActive());
}

void NodePreview::Dismiss(){
    m_Timer.stop();
    hide();
}

void NodePreview::Appear(){
    if(m_ItemRect.isNull()) return;

    const QSize size = SizeForImageArea(QSize(ScaleByDevice(PREVIEW_IMAGE_AREA.width()),
                                              ScaleByDevice(PREVIEW_IMAGE_AREA.height())));
    QScreen *screen = QGuiApplication::screenAt(m_ItemRect.center());
    if(!screen) screen = QGuiApplication::primaryScreen();
    if(!screen) return;

    setGeometry(Place(m_ItemRect, size, screen->availableGeometry(),
                      DeviceScale::FromDpi(PREVIEW_GAP,
                                           qRound(screen->logicalDotsPerInchY()))));
    update();
    if(!isVisible()) show();
    else raise();
}

void NodePreview::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRect bound = rect().adjusted(0, 0, -1, -1);

    painter.setPen(Theme::Pen(Theme::PreviewBorder));
    painter.setBrush(Theme::Brush(Theme::PreviewBackground));
    painter.drawRect(bound);

    const int padding = ScaleByDevice(PREVIEW_PADDING);
    const int captionHeight = ScaleByDevice(PREVIEW_CAPTION_HEIGHT);
    const QRect area = QRect(padding, padding,
                             width()  - padding * 2,
                             height() - padding * 2 - captionHeight);

    if(m_Image.isNull()){
        Theme::DrawEmptyThumbnail(&painter, area,
                                  m_IsDirectory ? Theme::PreviewPlaceholderDirectory
                                                : Theme::PreviewPlaceholderPage,
                                  Theme::PreviewText, m_IsDirectory);
    } else {
        QSize scaled = m_Image.size();
        scaled.scale(area.size(), Qt::KeepAspectRatio);
        const QRect target(area.x() + (area.width()  - scaled.width())  / 2,
                           area.y() + (area.height() - scaled.height()) / 2,
                           scaled.width(), scaled.height());
        painter.drawImage(target, m_Image);
    }

    const QRect caption = QRect(padding,
                                height() - padding - captionHeight,
                                width() - padding * 2,
                                captionHeight);

    painter.setPen(Theme::Pen(Theme::PreviewText));
    painter.drawText(caption, Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(m_Title, Qt::ElideRight,
                                                      caption.width()));
}
