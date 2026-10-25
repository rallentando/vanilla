#include "switch.hpp"
#include "const.hpp"
#include "docktitlebar.hpp"
#include "theme.hpp"
#include "application.hpp"

#include <QDockWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QFontMetrics>

DockTitleBar::DockTitleBar(QDockWidget *dock)
    : QWidget(dock)
    , m_Dock(dock)
    , m_Hovered(NoButton)
    , m_Pressed(NoButton)
{
    setMouseTracking(true);
    LoadIcons();
    connect(dock, &QDockWidget::featuresChanged, this, [this](){ updateGeometry(); update();});
    connect(dock, &QWidget::windowTitleChanged, this, [this](){ update();});
}

void DockTitleBar::LoadIcons(){
    QStyle *style = Application::style();
    const QSize size = ScaleByDevice(QSize(DOCK_TITLE_ICON_SIZE, DOCK_TITLE_ICON_SIZE));
    m_Float = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarNormalButton).pixmap(size), Theme::BarIcon);
    m_Close = Theme::Ink(style->standardIcon(QStyle::SP_TitleBarCloseButton).pixmap(size), Theme::BarIcon);
    m_IconScheme = Theme::CurrentScheme();
}

QSize DockTitleBar::sizeHint() const {
    const int height = ScaleByDevice(DOCK_TITLE_BAR_HEIGHT);
    const int pad = ScaleByDevice(TOOL_BAR_PADDING);
    const int chip = ScaleByDevice(DOCK_TITLE_BUTTON_SIZE);
    const int spacing = ScaleByDevice(TOOL_BAR_ICON_SPACING);
    return QSize(pad * 2 + chip * 2 + spacing, height);
}

QSize DockTitleBar::minimumSizeHint() const {
    return sizeHint();
}

QRect DockTitleBar::ButtonRect(int fromRight) const {
    const int pad = ScaleByDevice(TOOL_BAR_PADDING);
    const int chip = ScaleByDevice(DOCK_TITLE_BUTTON_SIZE);
    const int spacing = ScaleByDevice(TOOL_BAR_ICON_SPACING);
    const int right = width() - pad - fromRight * (chip + spacing);
    return QRect(right - chip, (height() - chip) / 2, chip, chip);
}

QRect DockTitleBar::CloseRect() const {
    if(!(m_Dock->features() & QDockWidget::DockWidgetClosable)) return QRect();
    return ButtonRect(0);
}

QRect DockTitleBar::FloatRect() const {
    if(!(m_Dock->features() & QDockWidget::DockWidgetFloatable)) return QRect();
    return ButtonRect(CloseRect().isNull() ? 0 : 1);
}

QRect DockTitleBar::TitleRect() const {
    const int pad = ScaleByDevice(TOOL_BAR_PADDING);
    int right = width() - pad;
    const QRect f = FloatRect();
    const QRect c = CloseRect();
    if(!f.isNull()) right = qMin(right, f.left() - pad);
    if(!c.isNull()) right = qMin(right, c.left() - pad);
    return QRect(pad, 0, qMax(0, right - pad), height());
}

bool DockTitleBar::OnTranslucentSurface() const {
    const QWidget *widget = this;
    while(widget && !widget->isWindow()){
        if(widget->internalWinId()) return false;
        widget = widget->parentWidget();
    }
    return widget && widget->testAttribute(Qt::WA_TranslucentBackground);
}

DockTitleBar::Button DockTitleBar::ButtonAt(const QPoint &pos) const {
    if(CloseRect().contains(pos)) return CloseButton;
    if(FloatRect().contains(pos)) return FloatButton;
    return NoButton;
}

void DockTitleBar::SetHovered(Button button){
    if(m_Hovered == button) return;
    m_Hovered = button;
    update();
}

void DockTitleBar::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)
    if(m_IconScheme != Theme::CurrentScheme()) LoadIcons();

    QPainter painter(this);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::Brush(Application::EnableTransparentWindow() && OnTranslucentSurface()
                                  ? Theme::BarBackgroundTranslucent
                                  : Theme::BarBackground));
    painter.drawRect(rect());

    const QRect title = TitleRect();
    if(title.width() > 0){
        painter.setFont(TitleBarTitleFont());
        painter.setPen(Theme::Pen(Theme::BarTitleText));
        const QFontMetrics metrics(painter.font());
        painter.drawText(title, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignLeft,
                         metrics.elidedText(m_Dock->windowTitle(), Qt::ElideRight, title.width()));
    }

    struct { Button button; QRect rect; const QPixmap *icon; } buttons[] = {
        { FloatButton, FloatRect(), &m_Float },
        { CloseButton, CloseRect(), &m_Close },
    };
    for(const auto &b : buttons){
        if(b.rect.isNull()) continue;
        if(m_Hovered == b.button || m_Pressed == b.button){
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::Brush(m_Pressed == b.button
                                          ? Theme::BarButtonPressed
                                          : Theme::BarButtonHovered));
            painter.drawRoundedRect(b.rect, CHIP_CORNER_RADIUS, CHIP_CORNER_RADIUS);
            painter.restore();
        }
        QRect icon(QPoint(), b.icon->size());
        icon.moveCenter(b.rect.center());
        painter.drawPixmap(icon, *b.icon, QRect(QPoint(), b.icon->size()));
    }
}

void DockTitleBar::mousePressEvent(QMouseEvent *ev){
    const Button button = ButtonAt(ev->pos());
    if(ev->button() == Qt::LeftButton && button != NoButton){
        m_Pressed = button;
        update();
        ev->accept();
        return;
    }
    ev->ignore();
}

void DockTitleBar::mouseMoveEvent(QMouseEvent *ev){
    SetHovered(ButtonAt(ev->pos()));
    if(m_Pressed != NoButton){
        ev->accept();
        return;
    }
    ev->ignore();
}

void DockTitleBar::mouseReleaseEvent(QMouseEvent *ev){
    if(m_Pressed == NoButton){
        ev->ignore();
        return;
    }
    const Button pressed = m_Pressed;
    m_Pressed = NoButton;
    update();
    ev->accept();
    if(ev->button() != Qt::LeftButton || ButtonAt(ev->pos()) != pressed) return;
    if(pressed == CloseButton) m_Dock->close();
    else if(pressed == FloatButton) m_Dock->setFloating(!m_Dock->isFloating());
}

void DockTitleBar::mouseDoubleClickEvent(QMouseEvent *ev){
    if(ButtonAt(ev->pos()) != NoButton){
        ev->accept();
        return;
    }
    ev->ignore();
}

void DockTitleBar::leaveEvent(QEvent *ev){
    QWidget::leaveEvent(ev);
    SetHovered(NoButton);
}
