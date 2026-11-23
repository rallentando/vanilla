#include "switch.hpp"

#include "menuiconrow.hpp"
#include "devicescale.hpp"
#include "theme.hpp"

#include <QAction>
#include <QWidgetAction>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOptionMenuItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QKeySequence>
#include <QHash>
#include <QtMath>

namespace {

const int IconSide = 16;

void Head(QPainterPath &path, const QPointF &tip, const QPointF &direction, qreal length = 4.0){
    const qreal angle = qAtan2(direction.y(), direction.x());
    for(qreal side : { 0.75, -0.75 }){
        path.moveTo(tip);
        path.lineTo(tip - length * QPointF(qCos(angle + side), qSin(angle + side)));
    }
}

QPointF Arc(QPainterPath &path, const QPointF &centre, qreal radius, qreal start, qreal sweep){
    const QRectF box(centre - QPointF(radius, radius), QSizeF(2 * radius, 2 * radius));
    path.arcMoveTo(box, start);
    path.arcTo(box, start, -sweep);
    const qreal end = qDegreesToRadians(start - sweep);
    return QPointF(qSin(end), qCos(end));
}

void Strokes(const QString &icon, QPainterPath &stroke, QPainterPath &fill){
    auto line = [&](qreal x1, qreal y1, qreal x2, qreal y2){
        stroke.moveTo(x1, y1);
        stroke.lineTo(x2, y2);
    };
    auto chevron = [&](qreal x, qreal dx){
        stroke.moveTo(x + dx, 6);
        stroke.lineTo(x, 12);
        stroke.lineTo(x + dx, 18);
    };

    if(icon == QLatin1String("Back")){
        chevron(9, 6);
    } else if(icon == QLatin1String("Forward")){
        chevron(15, -6);
    } else if(icon == QLatin1String("Rewind")){
        chevron(6, 6);
        chevron(12, 6);
    } else if(icon == QLatin1String("FastForward")){
        chevron(12, -6);
        chevron(18, -6);
    } else if(icon == QLatin1String("Reload")){
        const QPointF centre(12, 12);
        const qreal radius = 7;
        const qreal start = 20, sweep = 300;
        const QPointF direction = Arc(stroke, centre, radius, start, sweep);
        const qreal end = qDegreesToRadians(start - sweep);
        Head(stroke, centre + radius * QPointF(qCos(end), -qSin(end)), direction);
    } else if(icon == QLatin1String("Stop") || icon == QLatin1String("Close")){
        line(6.5, 6.5, 17.5, 17.5);
        line(17.5, 6.5, 6.5, 17.5);
    } else if(icon == QLatin1String("New")){
        line(12, 5, 12, 19);
        line(5, 12, 19, 12);
    } else if(icon == QLatin1String("Open")){
        stroke.moveTo(11, 5);
        stroke.lineTo(5, 5);
        stroke.lineTo(5, 19);
        stroke.lineTo(19, 19);
        stroke.lineTo(19, 13);
        line(11, 13, 19, 5);
        stroke.moveTo(14, 5);
        stroke.lineTo(19, 5);
        stroke.lineTo(19, 10);
    } else if(icon == QLatin1String("Download")){
        line(12, 4, 12, 15);
        stroke.moveTo(7, 10);
        stroke.lineTo(12, 15);
        stroke.lineTo(17, 10);
        line(5, 19.5, 19, 19.5);
    } else if(icon == QLatin1String("Copy")){
        stroke.addRoundedRect(QRectF(9, 8, 10, 12), 1.5, 1.5);
        stroke.moveTo(6, 16);
        stroke.lineTo(6, 5.5);
        stroke.quadTo(6, 4, 7.5, 4);
        stroke.lineTo(15, 4);
    } else if(icon == QLatin1String("Clone")){
        stroke.addRoundedRect(QRectF(9, 9, 10, 10), 1.5, 1.5);
        stroke.moveTo(5, 15);
        stroke.lineTo(5, 6.5);
        stroke.quadTo(5, 5, 6.5, 5);
        stroke.lineTo(15, 5);
    } else if(icon == QLatin1String("Link")){
        for(const QPointF &centre : { QPointF(9, 15), QPointF(15, 9) }){
            QPainterPath link;
            link.addRoundedRect(QRectF(-5.5, -3, 11, 6), 3, 3);
            QTransform transform;
            transform.translate(centre.x(), centre.y());
            transform.rotate(-45);
            stroke.addPath(transform.map(link));
        }
    } else if(icon == QLatin1String("Cut")){
        stroke.addEllipse(QPointF(7.5, 17), 2.5, 2.5);
        stroke.addEllipse(QPointF(16.5, 17), 2.5, 2.5);
        line(9.2, 15.2, 17, 4);
        line(14.8, 15.2, 7, 4);
    } else if(icon == QLatin1String("Paste")){
        stroke.addRoundedRect(QRectF(5.5, 5, 13, 16), 1.5, 1.5);
        stroke.addRoundedRect(QRectF(9, 3, 6, 4), 1, 1);
    } else if(icon == QLatin1String("PlayPause")){
        fill.moveTo(4, 6);
        fill.lineTo(12, 12);
        fill.lineTo(4, 18);
        fill.closeSubpath();
        fill.addRect(QRectF(15, 6, 2, 12));
        fill.addRect(QRectF(19, 6, 2, 12));
    } else if(icon == QLatin1String("Audible") || icon == QLatin1String("Muted")){
        stroke.moveTo(4, 9.5);
        stroke.lineTo(7.5, 9.5);
        stroke.lineTo(12, 5.5);
        stroke.lineTo(12, 18.5);
        stroke.lineTo(7.5, 14.5);
        stroke.lineTo(4, 14.5);
        stroke.closeSubpath();
        if(icon == QLatin1String("Muted")){
            line(15.5, 9.5, 20.5, 14.5);
            line(20.5, 9.5, 15.5, 14.5);
        } else {
            for(qreal radius : { 3.5, 7.0 }){
                const QRectF box(12 - radius, 12 - radius, 2 * radius, 2 * radius);
                stroke.arcMoveTo(box, 45);
                stroke.arcTo(box, 45, -90);
            }
        }
    } else if(icon == QLatin1String("Loop")){
        stroke.moveTo(5, 12);
        stroke.lineTo(5, 9.5);
        stroke.quadTo(5, 7, 7.5, 7);
        stroke.lineTo(19, 7);
        Head(stroke, QPointF(19, 7), QPointF(1, 0), 3.5);
        stroke.moveTo(19, 12);
        stroke.lineTo(19, 14.5);
        stroke.quadTo(19, 17, 16.5, 17);
        stroke.lineTo(5, 17);
        Head(stroke, QPointF(5, 17), QPointF(-1, 0), 3.5);
    } else if(icon == QLatin1String("Restore")){
        stroke.moveTo(5, 9);
        stroke.lineTo(14, 9);
        stroke.arcTo(QRectF(9, 9, 10, 10), 90, -180);
        stroke.lineTo(10, 19);
        Head(stroke, QPointF(5, 9), QPointF(-1, 0));
    } else if(icon == QLatin1String("Trash")){
        line(4, 7, 20, 7);
        stroke.moveTo(9.5, 7);
        stroke.lineTo(9.5, 4.5);
        stroke.lineTo(14.5, 4.5);
        stroke.lineTo(14.5, 7);
        stroke.moveTo(6, 7);
        stroke.lineTo(7, 20);
        stroke.lineTo(17, 20);
        stroke.lineTo(18, 7);
    } else if(icon == QLatin1String("Dots")){
        for(qreal x : { 5.0, 12.0, 19.0 })
            fill.addEllipse(QPointF(x, 12), 1.6, 1.6);
    }
}

}

MenuIconRow::MenuIconRow(QMenu *menu)
    : QWidget(menu)
    , m_Menu(menu)
    , m_Action(nullptr)
    , m_Divide(false)
    , m_Current(-1)
    , m_Hovered(-1)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

QWidgetAction *MenuIconRow::Install(){
    if(m_Action) return m_Action;
    m_Action = new QWidgetAction(m_Menu);
    m_Action->setDefaultWidget(this);
    QAction *first = m_Menu->actions().value(0);
    m_Menu->insertAction(first, m_Action);
    if(first) m_Menu->insertSeparator(first);
    else m_Menu->addSeparator();
    return m_Action;
}

MenuIconRow *MenuIconRow::Of(QMenu *menu){
    if(!menu) return nullptr;
    for(QAction *action : menu->actions())
        if(QWidgetAction *row = qobject_cast<QWidgetAction*>(action))
            if(MenuIconRow *widget = qobject_cast<MenuIconRow*>(row->defaultWidget()))
                return widget;
    return nullptr;
}

void MenuIconRow::AddCell(const QString &name, QAction *action, const QString &icon, bool enabled){
    if(Holds(name)) return;
    m_Cells << Cell{ action, icon, enabled, false, m_Divide };
    m_Divide = false;
    m_Names << name;
    addAction(action);
    updateGeometry();
}

void MenuIconRow::AddCommandCell(QAction *action){
    m_Cells << Cell{ action, QStringLiteral("Dots"), true, true, false };
    addAction(action);
    updateGeometry();
}

bool MenuIconRow::Replace(QAction *from, QAction *to){
    if(!from || !to) return false;
    bool replaced = false;
    for(Cell &cell : m_Cells){
        if(cell.action != from) continue;
        cell.action = to;
        replaced = true;
    }
    if(replaced){
        removeAction(from);
        addAction(to);
    }
    return replaced;
}

void MenuIconRow::AddDivider(){
    m_Divide = !m_Cells.isEmpty();
}

QIcon MenuIconRow::Icon(const QString &icon, const QWidget *widget){
    QPainterPath stroke, fill;
    Strokes(icon, stroke, fill);

    const int side = widget ? DeviceScale::FromDpi(IconSide, widget->logicalDpiY()) : IconSide;
    const qreal ratio = widget ? widget->devicePixelRatioF() : 1.0;
    QPixmap pixmap(QSize(side, side) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    const QColor ink = widget ? widget->palette().color(QPalette::Text) : QColor(Qt::black);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(side / 24.0, side / 24.0);
    QPen pen(ink, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(stroke);
    painter.setPen(Qt::NoPen);
    painter.setBrush(ink);
    painter.drawPath(fill);
    painter.end();
    return QIcon(pixmap);
}

QString MenuIconRow::IconOf(const QString &name){
    static const QHash<QString, QString> icons = {
        { QStringLiteral("Back"),                 QStringLiteral("Back") },
        { QStringLiteral("Forward"),              QStringLiteral("Forward") },
        { QStringLiteral("Rewind"),               QStringLiteral("Rewind") },
        { QStringLiteral("FastForward"),          QStringLiteral("FastForward") },
        { QStringLiteral("Reload"),               QStringLiteral("Reload") },
        { QStringLiteral("Stop"),                 QStringLiteral("Stop") },
        { QStringLiteral("OpenLink"),             QStringLiteral("Open") },
        { QStringLiteral("OpenImage"),            QStringLiteral("Open") },
        { QStringLiteral("OpenMedia"),            QStringLiteral("Open") },
        { QStringLiteral("DownloadLink"),         QStringLiteral("Download") },
        { QStringLiteral("DownloadImage"),        QStringLiteral("Download") },
        { QStringLiteral("DownloadMedia"),        QStringLiteral("Download") },
        { QStringLiteral("CopyLinkUrl"),          QStringLiteral("Link") },
        { QStringLiteral("CopyImageUrl"),         QStringLiteral("Link") },
        { QStringLiteral("CopyMediaUrl"),         QStringLiteral("Link") },
        { QStringLiteral("CopyImage"),            QStringLiteral("Copy") },
        { QStringLiteral("Copy"),                 QStringLiteral("Copy") },
        { QStringLiteral("Cut"),                  QStringLiteral("Cut") },
        { QStringLiteral("Paste"),                QStringLiteral("Paste") },
        { QStringLiteral("ToggleMediaPlayPause"), QStringLiteral("PlayPause") },
        { QStringLiteral("ToggleMediaMute"),      QStringLiteral("Audible") },
        { QStringLiteral("ToggleMediaLoop"),      QStringLiteral("Loop") },
        { QStringLiteral("NewNode"),              QStringLiteral("New") },
        { QStringLiteral("CloneNode"),            QStringLiteral("Clone") },
        { QStringLiteral("DeleteNode"),           QStringLiteral("Close") },
        { QStringLiteral("RestoreNode"),          QStringLiteral("Restore") },
        { QStringLiteral("ReloadNode"),           QStringLiteral("Reload") },
    };
    return icons.value(name);
}

int MenuIconRow::Scaled(int length) const {
    return DeviceScale::FromDpi(length, logicalDpiY());
}

int MenuIconRow::Gap() const { return Scaled(9); }

int MenuIconRow::CellWidth() const {
    const int margin = Scaled(4);
    const int n = qMax(1, int(m_Cells.length()));
    int gaps = 0;
    for(const Cell &cell : m_Cells) if(cell.divided) ++gaps;
    return qMin((width() - 2 * margin - gaps * Gap()) / n, Scaled(52));
}

QSize MenuIconRow::sizeHint() const {
    const int margin = Scaled(4);
    int gaps = 0;
    for(const Cell &cell : m_Cells) if(cell.divided) ++gaps;
    return QSize(2 * margin + qMax(1, int(m_Cells.length())) * Scaled(40) + gaps * Gap(),
                 fontMetrics().height() + Scaled(14));
}

QRect MenuIconRow::CellRect(int index) const {
    const int margin = Scaled(4);
    const int width = CellWidth();
    if(m_Cells.value(index).command)
        return QRect(this->width() - margin - width, 0, width, height());
    int gaps = 0;
    for(int i = 0; i <= index && i < m_Cells.length(); ++i) if(m_Cells[i].divided) ++gaps;
    return QRect(margin + index * width + gaps * Gap(), 0, width, height());
}

int MenuIconRow::CellAt(const QPoint &pos) const {
    for(int i = 0; i < m_Cells.length(); ++i)
        if(CellRect(i).contains(pos)) return i;
    return -1;
}

bool MenuIconRow::Chosen() const {
    return m_Action && m_Menu->activeAction() == m_Action && m_Menu->focusWidget() == this;
}

int MenuIconRow::Next(int from, int step) const {
    for(int i = from + step; i >= 0 && i < m_Cells.length(); i += step)
        if(m_Cells[i].enabled && m_Cells[i].action) return i;
    return -1;
}

void MenuIconRow::Run(int index){
    if(index < 0 || index >= m_Cells.length() || !m_Cells[index].enabled) return;
    QAction *action = m_Cells[index].action;
    if(!action) return;
    m_Menu->hide();
    action->trigger();
}

void MenuIconRow::paintEvent(QPaintEvent *){
    if(Chosen() && (m_Current == -1 || !m_Cells.value(m_Current).enabled)) m_Current = Next(-1, 1);
    QPainter painter(this);
    const int lit = m_Hovered != -1 ? m_Hovered : Chosen() ? m_Current : -1;
    const int side = Scaled(IconSide);
    for(int i = 0; i < m_Cells.length(); ++i){
        const QRect rect = CellRect(i);
        const bool enabled = m_Cells[i].enabled;
        if(m_Cells[i].divided){
            const int x = rect.left() - Gap() / 2;
            const int inset = rect.height() / 4;
            painter.setPen(Theme::Pen(Theme::MenuSeparator));
            painter.drawLine(x, rect.top() + inset, x, rect.bottom() - inset);
        }
        if(i == lit && enabled){
            QStyleOptionMenuItem option;
            option.initFrom(m_Menu);
            option.rect = rect;
            option.state = QStyle::State_Enabled | QStyle::State_Selected;
            option.menuItemType = QStyleOptionMenuItem::Normal;
            option.checkType = QStyleOptionMenuItem::NotCheckable;
            m_Menu->style()->drawControl(QStyle::CE_MenuItem, &option, &painter, m_Menu);
        }
        const QRect icon(rect.x() + (rect.width() - side) / 2, rect.y() + (rect.height() - side) / 2, side, side);
        painter.setOpacity(enabled ? 1.0 : 0.3);
        Icon(m_Cells[i].icon, this).paint(&painter, icon, Qt::AlignCenter);
        painter.setOpacity(1.0);
    }
}

void MenuIconRow::mouseMoveEvent(QMouseEvent *ev){
    int at = CellAt(ev->position().toPoint());
    if(at != -1 && !m_Cells[at].enabled) at = -1;
    if(at != m_Hovered){
        m_Hovered = at;
        if(at != -1) m_Current = at;
        update();
    }
    if(at != -1 && m_Menu->focusWidget() != this) setFocus(Qt::MouseFocusReason);
    QWidget::mouseMoveEvent(ev);
}

void MenuIconRow::mouseReleaseEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        Run(CellAt(ev->position().toPoint()));
        return;
    }
    QWidget::mouseReleaseEvent(ev);
}

void MenuIconRow::leaveEvent(QEvent *ev){
    m_Hovered = -1;
    update();
    QWidget::leaveEvent(ev);
}

void MenuIconRow::keyPressEvent(QKeyEvent *ev){
    if(!Chosen()){
        ev->ignore();
        return;
    }
    if(m_Current == -1 || !m_Cells.value(m_Current).enabled) m_Current = Next(-1, 1);
    switch(ev->key()){
    case Qt::Key_Left:
    case Qt::Key_Right: {
        const bool back = (ev->key() == Qt::Key_Left) != (layoutDirection() == Qt::RightToLeft);
        const int next = Next(m_Current, back ? -1 : 1);
        if(next == -1) break;
        m_Current = next;
        m_Hovered = -1;
        update();
        ev->accept();
        return;
    }
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        ev->accept();
        Run(m_Current);
        return;
    default: break;
    }
    ev->ignore();
}

void MenuIconRow::focusInEvent(QFocusEvent *ev){
    if(m_Current == -1 || !m_Cells.value(m_Current).enabled)
        m_Current = Next(-1, 1);
    update();
    QWidget::focusInEvent(ev);
}

void MenuIconRow::focusOutEvent(QFocusEvent *ev){
    update();
    QWidget::focusOutEvent(ev);
}

bool MenuIconRow::event(QEvent *ev){
    if(ev->type() == QEvent::ToolTip){
        QHelpEvent *help = static_cast<QHelpEvent*>(ev);
        const int at = CellAt(help->pos());
        QAction *action = at == -1 ? nullptr : m_Cells[at].action.data();
        if(!action){
            QToolTip::hideText();
            ev->ignore();
            return true;
        }
        QString text = action->text();
        text.remove(QLatin1Char('&'));
        const QString key = action->shortcut().toString(QKeySequence::NativeText);
        if(!key.isEmpty()) text += QStringLiteral(" (") + key + QStringLiteral(")");
        QToolTip::showText(help->globalPos(), text, this, CellRect(at));
        return true;
    }
    return QWidget::event(ev);
}
