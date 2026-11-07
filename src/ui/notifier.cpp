#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "notifier.hpp"

#include <QWidget>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QTimer>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QMenu>

#include "application.hpp"
#include "blurbehind.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"

Notifier::Notifier(TreeBank *parent, bool purge)
    : QWidget(purge ? nullptr : parent)
    , m_Position(SouthWest)
    , m_TreeBank(parent)
    , m_HotSpot(QPoint())
    , m_UseLinkText(false)
    , m_LowerIsLink(false)
    , m_StatusTimer(new QTimer(this))
    , m_LinkTimer(new QTimer(this))
{
    if(purge){
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    setMouseTracking(true);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_StatusTimer->setSingleShot(true);
    m_StatusTimer->setInterval(NOTIFIER_STATUS_DURATION);
    connect(m_StatusTimer, &QTimer::timeout, this, [this](){
        if(!m_UseLinkText) m_UpperText = QString();
        if(!m_LowerIsLink) m_LowerText = QString();
        ResizeNotify(m_TreeBank->size());
        repaint();
    });

    m_LinkTimer->setSingleShot(true);
    m_LinkTimer->setInterval(NOTIFIER_LINK_LINGER);
    connect(m_LinkTimer, &QTimer::timeout, this, &Notifier::ResetLink);

    resize(0, 0);
    hide();
}

Notifier::~Notifier(){}

bool Notifier::IsPurged() const {
    return isWindow();
}

void Notifier::MakeOwnedWindow(){
    const bool v = isVisible();

    hide();
    setAttribute(Qt::WA_TranslucentBackground);
    setParent(m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr,
              Qt::FramelessWindowHint | Qt::Tool);

    if(v) show();
}

void Notifier::TakeWindowOwnerIfNeed(){
    if(!isWindow() || !m_TreeBank) return;
    QWidget *win = m_TreeBank->GetMainWindow();
    if(!win || !win->isVisible() || parentWidget() == win) return;
    MakeOwnedWindow();
}

void Notifier::Purge(){
    MakeOwnedWindow();
}

void Notifier::Join(){
    bool v = isVisible();
    setParent(m_TreeBank);
    if(v) show();
    else hide();
}

bool Notifier::ShouldBeVisible() const {
    const bool something = !m_UpperText.isEmpty() || !m_LowerText.isEmpty()
        || TreeBank::EnableNotifierScrollArea() || !m_HotSpot.isNull();
    if(!something) return false;
    if(!IsPurged()) return true;
    MainWindow *win = m_TreeBank->GetMainWindow();
    return win && win->isVisible() && !win->isMinimized() && !win->IsShaded();
}

void Notifier::ResizeNotify(QSize size){
    TakeWindowOwnerIfNeed();
    const bool scrollArea = TreeBank::EnableNotifierScrollArea();
    const int margin = ScaleByDevice(NOTIFIER_MARGIN);
    const int padX = ScaleByDevice(NOTIFIER_PADDING_X);
    const int padY = ScaleByDevice(NOTIFIER_PADDING_Y);
    const int lines = (m_UpperText.isEmpty() ? 0 : 1) + (m_LowerText.isEmpty() ? 0 : 1);
    const int right = size.width() - m_TreeBank->MiniMapStripWidth();
    const int widest = qMin(qMax(right * NOTIFIER_WIDTH_PERCENTAGE / 100,
                                 ScaleByDevice(NOTIFIER_MINIMUM_WIDTH)),
                            qMax(right - margin * 2, 1));
    const QFontMetrics metrics(NotifierFont(), this);
    const QString space = QStringLiteral(" ");
    const int text = qMax(metrics.horizontalAdvance(QString(m_UpperText).replace(QStringLiteral("\n"), space)),
                          metrics.horizontalAdvance(QString(m_LowerText).replace(QStringLiteral("\n"), space)));
    const int textWidth = text ? text + 1 : 0;
    const int areaWidth = scrollArea ? ScaleByDevice(SCROLL_AREA_WIDTH) : 0;
    const int w = qMin(widest, padX * 2 + areaWidth + textWidth + (areaWidth && textWidth ? padX : 0));
    const int status = qMax(lines * ScaleByDevice(NOTIFIER_LINE_HEIGHT),
                            scrollArea ? ScaleByDevice(SCROLL_AREA_HEIGHT) : 0);
    const int h = padY * 2 + status;
    QPoint pos;
    switch(m_Position){
    case NorthWest:
        pos = QPoint(margin, margin);
        break;
    case NorthEast:
        pos = QPoint(right - w - margin, margin);
        break;
    case SouthWest:
        pos = QPoint(margin, size.height() - h - margin);
        break;
    case SouthEast:
        pos = QPoint(right - w - margin, size.height() - h - margin);
        break;
    }
    if(IsPurged()) pos = m_TreeBank->mapToGlobal(pos);
    setGeometry(QRect(pos, QSize(w, h)));
    setVisible(ShouldBeVisible());
}

void Notifier::TextChanged(){
    if((!m_UpperText.isEmpty() && !m_UseLinkText) ||
       (!m_LowerText.isEmpty() && !m_LowerIsLink))
        m_StatusTimer->start();
    else
        m_StatusTimer->stop();
    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Notifier::SetStatus(const QString str){
    if(!str.isEmpty()){
        m_UseLinkText = false;
    }
    m_UpperText = str;
    TextChanged();
}

void Notifier::SetStatus(const QString str1, const QString str2){
    if(!str1.isEmpty() || !str2.isEmpty()){
        m_UseLinkText = false;
    }
    m_UpperText = str1;
    m_LowerText = str2;
    m_LowerIsLink = false;
    TextChanged();
}

void Notifier::ResetStatus(){
    m_UseLinkText = false;
    m_UpperText = QString();
    TextChanged();
}

void Notifier::SetLink(const QString url, const QString title, const QString text){
    if(url.isEmpty()){
        if(m_LowerIsLink) m_LinkTimer->start();
        return;
    }
    m_LinkTimer->stop();
    m_LowerText = url;
    m_LowerIsLink = true;
    if(
       m_UpperText.isEmpty() ||
       m_UpperText == tr("Finished loading.") ||
       m_UpperText == tr("Failed to load.") ||
       m_UpperText == tr("Auto save failed.") ||
       m_UpperText.startsWith(tr("Auto save finished")) ||
       m_UpperText.startsWith(tr("Zoom factor changed to")) ||
       m_UpperText.startsWith(tr("Displaying")) ||
       m_UseLinkText){

        if(!title.isEmpty()){
            m_UseLinkText = true;
            m_UpperText = title;
        } else if(!text.isEmpty()){
            m_UseLinkText = true;
            m_UpperText = text;
        } else {
            m_UseLinkText = false;
            m_UpperText = QString();
        }
    }
    TextChanged();
}

void Notifier::ResetLink(){
    if(m_UseLinkText) m_UpperText = QString();
    m_UseLinkText = false;
    if(m_LowerIsLink) m_LowerText = QString();
    m_LowerIsLink = false;
    TextChanged();
}

void Notifier::AutoSaveStarted(){
    m_UseLinkText = false;
    m_UpperText = tr("Auto save started.");
    TextChanged();
}

void Notifier::AutoSaveFailed(){
    m_UseLinkText = false;
    m_UpperText = tr("Auto save failed.");
    TextChanged();
}

void Notifier::AutoSaveFinished(const QString &info){
    m_UseLinkText = false;
    m_UpperText = tr("Auto save finished (%1).").arg(info);
    TextChanged();
}

void Notifier::SetScroll(QPointF pos){
    m_ScrollPos = pos;
    if(TreeBank::EnableNotifierScrollArea()) repaint();
}

QRect Notifier::ScrollAreaRect() const {
    return QRect(ScaleByDevice(NOTIFIER_PADDING_X), ScaleByDevice(NOTIFIER_PADDING_Y),
                 ScaleByDevice(SCROLL_AREA_WIDTH), ScaleByDevice(SCROLL_AREA_HEIGHT));
}

int Notifier::ScrollMarkerInset() const {
    return ScaleByDevice(4);
}

bool Notifier::EmitScrollRequest(QPoint pos){
    if(!m_HotSpot.isNull() || !TreeBank::EnableNotifierScrollArea()) return false;

    const QRect area = ScrollAreaRect();
    if(!area.contains(pos)) return false;

    const QRect travel = area.adjusted(ScrollMarkerInset(), ScrollMarkerInset(),
                                       -ScrollMarkerInset() - 1, -ScrollMarkerInset() - 1);
    emit ScrollRequest(QPointF(qBound(0.0, static_cast<qreal>(pos.x() - travel.left()) / travel.width(), 1.0),
                               qBound(0.0, static_cast<qreal>(pos.y() - travel.top()) / travel.height(), 1.0)));
    return true;
}

void Notifier::timerEvent(QTimerEvent *ev){
    Q_UNUSED(ev)
}

void Notifier::paintEvent(QPaintEvent *ev){
    const QRect area = ScrollAreaRect();
    const int lineHeight = ScaleByDevice(NOTIFIER_LINE_HEIGHT);
    const int offset = ScaleByDevice(NOTIFIER_PADDING_Y);
    const int pad = ScaleByDevice(NOTIFIER_PADDING_X);
    const bool scrollArea = TreeBank::EnableNotifierScrollArea();

    QPainter painter(this);
    painter.setFont(NotifierFont());

    const auto ElidedText = [&painter](const QString &text, int width){
        return painter.fontMetrics().elidedText(text, Qt::ElideRight, width);
    };

    {
        const int radius = ScaleByDevice(DIALOG_CORNER_RADIUS);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Theme::Pen(Theme::VanillaPageBorder));
        painter.setBrush(Theme::Brush(IsPurged() ? BlurBehind::Surface() : Theme::NotifierBackground));
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.drawRoundedRect(card, radius, radius);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    {
        if(scrollArea){
            const int radius = ScaleByDevice(FIELD_CORNER_RADIUS);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Theme::Pen(Theme::NotifierScrollBorder));
            painter.setBrush(Theme::Brush(Theme::MiniMapViewport));
            const QRectF well = QRectF(area).adjusted(0.5, 0.5, -0.5, -0.5);
            painter.drawRoundedRect(well, radius, radius);
            painter.setRenderHint(QPainter::Antialiasing, false);

            const int inset = ScrollMarkerInset();
            const QRect travel = area.adjusted(inset, inset, -inset - 1, -inset - 1);
            const QPoint at(travel.left() + qRound(qBound(0.0, m_ScrollPos.x(), 1.0) * travel.width()),
                            travel.top()  + qRound(qBound(0.0, m_ScrollPos.y(), 1.0) * travel.height()));

            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::Brush(Theme::NotifierScrollLine));
            painter.drawRect(area.left() + 1, at.y(), area.width() - 2, 1);
            painter.drawRect(at.x(), area.top() + 1, 1, area.height() - 2);

            const qreal size = ScaleByDevice(5) / 2.0;
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setBrush(Theme::Brush(Theme::NotifierScrollMarker));
            painter.drawEllipse(QPointF(at) + QPointF(0.5, 0.5), size, size);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }

        {
            const int x = pad + (scrollArea ? area.width() + pad : 0);
            const int w = width() - x - pad;
            const int lines = (m_UpperText.isEmpty() ? 0 : 1) + (m_LowerText.isEmpty() ? 0 : 1);
            const int top = offset + (height() - offset - offset - lines * lineHeight) / 2
                - ScaleByDevice(1);
            const QString space = QStringLiteral(" ");

            painter.setBrush(Qt::NoBrush);
            if(!m_UpperText.isEmpty()){
                painter.setPen(Theme::Pen(Theme::NotifierTextStrong));
                painter.drawText(QRect(x, top, w, lineHeight), Qt::AlignLeft | Qt::AlignVCenter,
                                 ElidedText(QString(m_UpperText).replace(QStringLiteral("\n"), space), w));
            }
            if(!m_LowerText.isEmpty()){
                painter.setPen(Theme::Pen(lines == 1 ? Theme::NotifierTextStrong : Theme::NotifierText));
                painter.drawText(QRect(x, top + (lines - 1) * lineHeight, w, lineHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 ElidedText(QString(m_LowerText).replace(QStringLiteral("\n"), space), w));
            }
        }
    }
    painter.end();

    ev->setAccepted(true);
}

void Notifier::showEvent(QShowEvent *ev){
    QWidget::showEvent(ev);
    if(IsPurged()){
        BlurBehind::Apply(this);
        BlurBehind::RoundCorners(this);
    }
}

void Notifier::enterEvent(QEnterEvent *ev)
{
    Q_UNUSED(ev)
}

void Notifier::mousePressEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        if(!EmitScrollRequest(ev->pos())){
            m_HotSpot = ev->pos();
            ev->setAccepted(true);
        }
    } else if(ev->button() == Qt::RightButton){
        QMenu *menu = m_TreeBank->GlobalContextMenu();
        menu->exec(ev->globalPosition().toPoint());
        delete menu;
        ev->setAccepted(true);
    }
}

void Notifier::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        if(!EmitScrollRequest(ev->pos()) && !m_HotSpot.isNull()){
            QRect rect = QRect((IsPurged()
                                ? mapToGlobal(ev->pos())
                                : mapToParent(ev->pos()))
                               - m_HotSpot,
                               size());
            setGeometry(rect);
            bool north = true;
            bool west = true;
            QPoint center1 = rect.center();
            QPoint center2 = QPoint(m_TreeBank->width()/2,
                                    m_TreeBank->height()/2);
            if(IsPurged()) center2 = m_TreeBank->mapToGlobal(center2);
            if(center1.x() > center2.x()) west = false;
            if(center1.y() > center2.y()) north = false;
            if(north){
                if(west){
                    m_Position = NorthWest;
                } else {
                    m_Position = NorthEast;
                }
            } else {
                if(west){
                    m_Position = SouthWest;
                } else {
                    m_Position = SouthEast;
                }
            }
            ev->setAccepted(true);
            return;
        }
    }
}

void Notifier::mouseReleaseEvent(QMouseEvent *ev){
    m_HotSpot = QPoint();
    ResizeNotify(m_TreeBank->size());
    ev->setAccepted(true);
    if(IsPurged()) m_TreeBank->GetMainWindow()->SetFocus();
}
