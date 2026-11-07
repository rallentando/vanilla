#include "switch.hpp"
#include "const.hpp"

#include "nodepreview.hpp"

#include <QPainter>
#include <QFontMetrics>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QWindow>
#include <QPainterPath>
#include <QPropertyAnimation>

#include "theme.hpp"
#include "devicescale.hpp"
#include "treebar.hpp"

namespace {

    const QSize PREVIEW_IMAGE_AREA = QSize(260, 195);
    const int PREVIEW_BORDER = 1;
    const int PREVIEW_TEXT_PADDING_X = 10;
    const int PREVIEW_TEXT_PADDING_Y = 8;
    const int PREVIEW_LINE_GAP = 4;

    const QFont &TitleFont(){
        return TreeBarTitleFont();
    }
    const QFont &HostFont(){
        static const QFont font = ScaledFont(1.0, 0);
        return font;
    }
    int TitleLineHeight(){
        return QFontMetrics(TitleFont()).height();
    }
    int HostLineHeight(){
        return QFontMetrics(HostFont()).height();
    }
    const int PREVIEW_DELAY_MS = 500;
    const int PREVIEW_GAP = 4;
    const int PREVIEW_SLIDE_MS = 150;
    const int PREVIEW_WARM_MS = 1000;
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
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
    resize(SizeFor(true, true));

    m_Timer.setSingleShot(true);
    m_Timer.setInterval(PREVIEW_DELAY_MS);
    connect(&m_Timer, &QTimer::timeout, this, &NodePreview::Appear);

    m_Slide = new QPropertyAnimation(this, "geometry", this);
    m_Slide->setDuration(PREVIEW_SLIDE_MS);
    m_Slide->setEasingCurve(QEasingCurve::OutCubic);

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *){
        HoldUntilPointerMoves(QCursor::pos());
    });
}

QSize NodePreview::SizeFor(bool hasImage, bool hasHost){
    int height = PREVIEW_BORDER * 2
        + ScaleByDevice(PREVIEW_TEXT_PADDING_Y) * 2
        + TitleLineHeight();
    if(hasImage) height += ScaleByDevice(PREVIEW_IMAGE_AREA.height());
    if(hasHost)  height += ScaleByDevice(PREVIEW_LINE_GAP) + HostLineHeight();
    return QSize(ScaleByDevice(PREVIEW_IMAGE_AREA.width()) + PREVIEW_BORDER * 2, height);
}

QRect NodePreview::Place(const QRect &itemRect, const QRect &barRect, Qt::Edge side,
                         const QSize &size, const QRect &screen, int gap){
    const int screenLeft   = screen.x();
    const int screenTop    = screen.y();
    const int screenRight  = screen.x() + screen.width();
    const int screenBottom = screen.y() + screen.height();

    int x = 0;
    int y = 0;
    switch(side){
    case Qt::TopEdge:
    case Qt::BottomEdge: {
        const int below = barRect.y() + barRect.height() + gap;
        const int above = barRect.y() - gap - size.height();
        if(side == Qt::BottomEdge)
            y = (below + size.height() > screenBottom) ? above : below;
        else
            y = (above < screenTop) ? below : above;
        x = itemRect.center().x() - size.width() / 2;
        break;
    }
    case Qt::LeftEdge:
    case Qt::RightEdge: {
        const int after  = barRect.x() + barRect.width() + gap;
        const int before = barRect.x() - gap - size.width();
        if(side == Qt::RightEdge)
            x = (after + size.width() > screenRight) ? before : after;
        else
            x = (before < screenLeft) ? after : before;
        y = itemRect.center().y() - size.height() / 2;
        break;
    }
    }

    if(y + size.height() > screenBottom) y = screenBottom - size.height();
    if(y < screenTop) y = screenTop;

    if(x + size.width() > screenRight) x = screenRight - size.width();
    if(x < screenLeft) x = screenLeft;

    return QRect(QPoint(x, y), size);
}

void NodePreview::Request(const QImage &image, const QString &title,
                          const QString &host,
                          const QRect &itemRect, const QRect &barRect, Qt::Edge side,
                          const QPoint &pointer){
    if(m_Held){
        if(pointer == m_HeldAt) return;
        m_Held = false;
    }

    const bool same = (m_ItemRect == itemRect && m_Title == title);

    m_Image = image;
    m_Title = title;
    m_Host = host;
    m_ItemRect = itemRect;
    m_BarRect = barRect;
    m_Side = side;

    if(isVisible()){
        Appear();
    } else if(m_SinceShown.isValid() && m_SinceShown.elapsed() < PREVIEW_WARM_MS){
        m_Timer.stop();
        Appear();
    } else if(!same || !m_Timer.isActive()){
        m_Timer.start();
    }
}

bool NodePreview::IsAbout(const QString &title, const QRect &itemRect) const {
    return m_ItemRect == itemRect && m_Title == title
        && (isVisible() || m_Timer.isActive());
}

void NodePreview::LeaveItem(){
    if(!isVisible()) m_Timer.stop();
}

void NodePreview::Dismiss(){
    if(isVisible()) m_SinceShown.start();
    m_Timer.stop();
    m_Slide->stop();
    hide();
}

void NodePreview::HoldUntilPointerMoves(const QPoint &pointer){
    Dismiss();
    m_SinceShown.invalidate();
    m_Held = true;
    m_HeldAt = pointer;
}

void NodePreview::Appear(){
    if(m_ItemRect.isNull()) return;

    const QSize size = SizeFor(!m_Image.isNull(), !m_Host.isEmpty());
    QScreen *screen = QGuiApplication::screenAt(m_ItemRect.center());
    if(!screen) screen = QGuiApplication::primaryScreen();
    if(!screen) return;

    const QRect place = Place(m_ItemRect, m_BarRect, m_Side, size, screen->availableGeometry(),
                              DeviceScale::FromDpi(PREVIEW_GAP,
                                                   qRound(screen->logicalDotsPerInchY())));

    if(isVisible() && TreeBar::EnableAnimation() && geometry() != place){
        if(m_Slide->state() != QAbstractAnimation::Running
           || m_Slide->endValue().toRect() != place){
            m_Slide->stop();
            m_Slide->setStartValue(geometry());
            m_Slide->setEndValue(place);
            m_Slide->start();
        }
    } else {
        m_Slide->stop();
        setGeometry(place);
    }
    update();
    if(!isVisible()) show();
    else raise();
}

void NodePreview::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int radius = ScaleByDevice(DIALOG_CORNER_RADIUS);
    const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath shape;
    shape.addRoundedRect(card, radius, radius);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::Brush(Theme::PreviewBackground));
    painter.drawPath(shape);

    const int border = PREVIEW_BORDER;
    const int imageHeight = m_Image.isNull() ? 0 : ScaleByDevice(PREVIEW_IMAGE_AREA.height());
    const QRect area = QRect(QPoint(border, border),
                             QSize(width() - border * 2, imageHeight));

    if(!m_Image.isNull()){
        QSize scaled = m_Image.size();
        scaled.scale(area.size(), Qt::KeepAspectRatioByExpanding);
        const QRect target(area.x() + (area.width()  - scaled.width())  / 2,
                           area.y() + (area.height() - scaled.height()) / 2,
                           scaled.width(), scaled.height());
        painter.save();
        painter.setClipPath(shape);
        painter.setClipRect(area, Qt::IntersectClip);
        painter.drawImage(target, m_Image);
        painter.restore();
    }

    painter.setPen(Theme::Pen(Theme::PreviewBorder));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shape);

    const int paddingX = ScaleByDevice(PREVIEW_TEXT_PADDING_X);
    const int textWidth = width() - paddingX * 2;
    const int titleTop = border + imageHeight + ScaleByDevice(PREVIEW_TEXT_PADDING_Y);
    const QRect titleRect(paddingX, titleTop, textWidth, TitleLineHeight());

    painter.setPen(Theme::Pen(Theme::PreviewText));
    painter.setFont(TitleFont());
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(m_Title, Qt::ElideRight,
                                                      titleRect.width()));

    if(!m_Host.isEmpty()){
        const QRect hostRect(paddingX,
                             titleRect.bottom() + 1 + ScaleByDevice(PREVIEW_LINE_GAP),
                             textWidth, HostLineHeight());
        QColor quiet = Theme::Color(Theme::PreviewText);
        quiet.setAlphaF(DOWNLOAD_PANEL_QUIET_ALPHA);
        painter.setPen(quiet);
        painter.setFont(HostFont());
        painter.drawText(hostRect, Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(m_Host, Qt::ElideMiddle,
                                                          hostRect.width()));
    }
}
