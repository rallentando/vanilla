#include "switch.hpp"
#include "const.hpp"

#include "passwordsuggestions.hpp"

#include "theme.hpp"
#include "blurbehind.hpp"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QStyle>
#include <QStyleOptionMenuItem>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace {
    const QString HIDDEN = QString(8, QChar(0x2022));
}

PasswordSuggestions::PasswordSuggestions(QWidget *window, const QString &manage)
    : QWidget(window, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
    , m_Manage(manage)
    , m_Translucent(BlurBehind::Enabled())
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    if(m_Translucent) setAttribute(Qt::WA_TranslucentBackground);
}

void PasswordSuggestions::MoveChoice(int step){
    const int count = Rows();
    if(m_Chosen < 0) SetChoice(step > 0 ? 0 : count - 1);
    else SetChoice(((m_Chosen + step) % count + count) % count);
}

void PasswordSuggestions::SetAccounts(const QStringList &accounts){
    int chosen = -1;
    if(m_Chosen == ManageRow()) chosen = accounts.size();
    else if(m_Chosen >= 0) chosen = accounts.indexOf(m_Accounts.at(m_Chosen));
    m_Accounts = accounts;
    m_Chosen = chosen;
    update();
}

void PasswordSuggestions::ClearChoice(){
    SetChoice(-1);
}

void PasswordSuggestions::SetChoice(int row){
    if(m_Chosen == row) return;
    m_Chosen = row;
    update();
}

void PasswordSuggestions::ShowAt(const QPoint &at){
    QScreen *screen = QGuiApplication::screenAt(at);
    if(screen){
        const int room = screen->availableGeometry().height();
        while(m_Accounts.size() > 1 && sizeHint().height() > room){
            if(m_Chosen == m_Accounts.size() - 1) m_Chosen = -1;
            else if(m_Chosen == ManageRow()) m_Chosen--;
            m_Accounts.removeLast();
        }
    }
    resize(sizeHint());
    QPoint pos = at;
    if(screen){
        const QRect room = screen->availableGeometry();
        if(pos.y() + height() > room.bottom()) pos.ry() -= height();
        pos.setX(qBound(room.left(), pos.x(), qMax(room.left(), room.right() - width())));
    }
    move(pos);
    if(isVisible()) return;
    show();
#if defined(Q_OS_WIN)
    if(!m_Translucent){
        const DWORD round = 2;
        DwmSetWindowAttribute(reinterpret_cast<HWND>(winId()),
                              33 ,
                              &round, sizeof(round));
    }
#endif
    raise();
}

void PasswordSuggestions::showEvent(QShowEvent *ev){
    QWidget::showEvent(ev);
    if(m_Translucent) BlurBehind::Apply(this);
}

int PasswordSuggestions::Padding() const {
    return fontMetrics().height() / 2;
}

QFont PasswordSuggestions::NameFont() const {
    QFont name = font();
    name.setWeight(QFont::DemiBold);
    return name;
}

int PasswordSuggestions::RowHeight(int row) const {
    const int line = fontMetrics().height();
    return (row < ManageRow() ? 2 * line : line) + 2 * Padding();
}

int PasswordSuggestions::SeparatorHeight() const {
    return Padding() + 1;
}

QRect PasswordSuggestions::RowRect(int row) const {
    int top = Padding();
    for(int i = 0; i < row; i++) top += RowHeight(i);
    if(row == ManageRow()) top += SeparatorHeight();
    return QRect(Padding(), top, width() - 2 * Padding(), RowHeight(row));
}

int PasswordSuggestions::RowAt(const QPoint &pos) const {
    for(int i = 0; i < Rows(); i++)
        if(RowRect(i).contains(pos)) return i;
    return -1;
}

QSize PasswordSuggestions::sizeHint() const {
    const QFontMetrics name(NameFont());
    int words = qMax(fontMetrics().horizontalAdvance(m_Manage), fontMetrics().horizontalAdvance(HIDDEN));
    foreach(const QString &account, m_Accounts) words = qMax(words, name.horizontalAdvance(account));
    words = qMax(words, fontMetrics().averageCharWidth() * 24);
    int height = 2 * Padding() + SeparatorHeight();
    for(int i = 0; i < Rows(); i++) height += RowHeight(i);
    return QSize(words + 4 * Padding(), height);
}

void PasswordSuggestions::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)
    QPainter painter(this);
    if(m_Translucent){
        QStyleOption panel;
        panel.initFrom(this);
        style()->drawPrimitive(QStyle::PE_PanelMenu, &panel, &painter, this);
    } else {
        painter.setPen(Theme::Pen(Theme::PreviewBorder));
        painter.setBrush(Theme::Brush(Theme::PreviewBackground));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

    const QColor ink = Theme::Color(Theme::PreviewText);
    QColor quiet = ink;
    quiet.setAlphaF(DOWNLOAD_PANEL_QUIET_ALPHA);
    const int line = fontMetrics().height();
    for(int i = 0; i < Rows(); i++){
        const QRect row = RowRect(i);
        if(i == m_Chosen){
            QStyleOptionMenuItem band;
            band.initFrom(this);
            band.state = QStyle::State_Enabled | QStyle::State_Selected;
            band.menuItemType = QStyleOptionMenuItem::Normal;
            band.checkType = QStyleOptionMenuItem::NotCheckable;
            band.maxIconWidth = 0;
            band.reservedShortcutWidth = 0;
            band.menuRect = rect();
            band.rect = row;
            style()->drawControl(QStyle::CE_MenuItem, &band, &painter, this);
        }
        const QRect words = row.adjusted(Padding(), Padding(), -Padding(), -Padding());
        if(i == ManageRow()){
            painter.setFont(font());
            painter.setPen(ink);
            painter.drawText(words, Qt::AlignLeft | Qt::AlignVCenter, m_Manage);
            continue;
        }
        painter.setFont(NameFont());
        painter.setPen(ink);
        painter.drawText(QRect(words.left(), words.top(), words.width(), line),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetrics(NameFont()).elidedText(m_Accounts.at(i), Qt::ElideRight, words.width()));
        painter.setFont(font());
        painter.setPen(quiet);
        painter.drawText(QRect(words.left(), words.top() + line, words.width(), line),
                         Qt::AlignLeft | Qt::AlignVCenter, HIDDEN);
    }

    const int y = RowRect(ManageRow()).top() - SeparatorHeight() / 2 - 1;
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Theme::Pen(Theme::MenuSeparator));
    painter.drawLine(0, y, width() - 1, y);
}

void PasswordSuggestions::mouseMoveEvent(QMouseEvent *ev){
    const int row = RowAt(ev->position().toPoint());
    if(row >= 0) SetChoice(row);
}

void PasswordSuggestions::mousePressEvent(QMouseEvent *ev){
    ev->accept();
}

void PasswordSuggestions::mouseReleaseEvent(QMouseEvent *ev){
    if(ev->button() != Qt::LeftButton) return;
    const int row = RowAt(ev->position().toPoint());
    if(row >= 0) emit chosen(row);
}
