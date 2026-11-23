#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "receiver.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QHideEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QTimerEvent>
#include <QPointer>
#include <QScreen>
#include <QLocalServer>
#include <QLocalSocket>
#include "searchsuggest.hpp"
#include "actiontext.hpp"
#include <QDomElement>
#include <QUrl>
#include <QUrlQuery>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "gadgets.hpp"
#include "blurbehind.hpp"
#include "settingspage.hpp"
#include "settingsschema.hpp"
#include "view.hpp"
#include "commandmap.hpp"
#include "dialog.hpp"
#include "commandframe.hpp"
#include "editmenu.hpp"
#include "nodepreview.hpp"
#include "menuschema.hpp"

QLocalServer *Receiver::m_LocalServer = nullptr;
QStringList Receiver::m_History;

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    ApplyTheme();
    setDragEnabled(true);
    EditMenu::Install(this);
}

LineEdit::~LineEdit(){}

static QString Selection(){
    return QStringLiteral("selection-background-color: %1; selection-color: %2;")
        .arg(Theme::StyleSheetColor(Theme::VanillaPageAccent),
             Theme::StyleSheetColor(Theme::VanillaPageAccentText));
}

void LineEdit::SetPill(bool on){
    m_Pill = on;
    ApplyTheme();
}

void LineEdit::ApplyTheme(){
    if(m_Pill){
        const int height = ScaleByDevice(RECEIVER_FIELD_HEIGHT);
        setStyleSheet(QStringLiteral("QLineEdit{ background: transparent; color: %1; placeholder-text-color: %2;"
                                     " border: 1px solid transparent; border-radius: %3px; padding-left: %4px; padding-right: %4px;"
                                     " %5}")
                      .arg(Theme::StyleSheetColor(Theme::VanillaPageText),
                           Theme::StyleSheetColor(Theme::VanillaPageTextDim))
                      .arg(height / 2)
                      .arg(ScaleByDevice(RECEIVER_FIELD_PADDING_X))
                      .arg(Selection()));
        return;
    }

    const QString active = Theme::StyleSheetColor(Theme::LineEditBackgroundActive);

    QString border;
    QString borderActive;
#if defined(Q_OS_MAC)
    border = QStringLiteral("border: 1px solid %1;")
        .arg(Theme::StyleSheetColor(Theme::LineEditBorder));
    borderActive = QStringLiteral("border: 1px solid %1;")
        .arg(Theme::StyleSheetColor(Theme::LineEditBorderActive));
#endif

    const bool grouped = property(AddressHoverProperty).toBool();
    setStyleSheet(QStringLiteral("QLineEdit:!focus{ background: transparent; %1}"
                                 "QLineEdit:focus{ background: %2; %3}"
                                 "QLineEdit:hover{ background: %8; %3}"
                                 "QLineEdit{ border-radius: %4px; padding: %5px %6px %5px %9px; %7}")
                  .arg(border, active, borderActive)
                  .arg(ScaleByDevice(FIELD_CORNER_RADIUS))
                  .arg(ScaleByDevice(FIELD_PADDING))
                  .arg(ScaleByDevice(FIELD_PADDING_X))
                  .arg(Selection(), grouped ? QStringLiteral("transparent") : active)
                  .arg(ScaleByDevice(grouped ? FIELD_PADDING_BESIDE_SITE : FIELD_PADDING_X)));
}

const char *const LineEdit::AddressHoverProperty = "addressHover";

void LineEdit::SetCount(const QString &count){
    if(m_Count == count) return;
    m_Count = count;
    UpdateTextMargins();
    update();
}

namespace {
    QRect TagRect(const QWidget *field, const QFontMetrics &metrics, const QString &tag, int left, int pad){
        const int height = metrics.height() + DeviceScale::FromDpi(4, static_cast<int>(field->logicalDpiY()));
        return QRect(left, (field->height() - height) / 2, metrics.horizontalAdvance(tag) + 2 * pad, height);
    }
}

void LineEdit::SetTag(const QString &tag){
    if(m_Tag == tag) return;
    m_Tag = tag;
    UpdateTextMargins();
    update();
}

void LineEdit::SetClosable(bool on){
    if(m_Closable == on) return;
    m_Closable = on;
    if(on) setMouseTracking(true);
    UpdateTextMargins();
    update();
}

QRect LineEdit::CloseRect() const {
    if(!m_Closable) return QRect();
    const int inset = ScaleByDevice(4);
    const int side = qMax(0, height() - 2 * inset);
    return QRect(width() - inset - side, inset, side, side);
}

int LineEdit::CloseReserve() const {
    return m_Closable ? width() - CloseRect().left() : 0;
}

void LineEdit::SetCloseHovered(bool on){
    if(m_CloseHovered == on) return;
    m_CloseHovered = on;
    if(on) setCursor(Qt::ArrowCursor);
    else unsetCursor();
    update(CloseRect());
}

void LineEdit::UpdateTextMargins(){
    const int pad = ScaleByDevice(RECEIVER_FIELD_PADDING_X);
    const int right = (m_Count.isEmpty() ? 0 : fontMetrics().horizontalAdvance(m_Count) + pad)
        + CloseReserve();
    const int left = m_Tag.isEmpty() ? 0
        : TagRect(this, fontMetrics(), m_Tag, 0, ScaleByDevice(6)).width() + ScaleByDevice(6);
    setTextMargins(left, 0, right, 0);
}

void LineEdit::paintEvent(QPaintEvent *ev){
    if(m_Pill){
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF ring = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = qMin(ring.width(), ring.height()) / 2.0;
        painter.setPen(QPen(Theme::Color(hasFocus() ? Theme::VanillaPageAccent
                                                  : Theme::VanillaPageBorder), 1.0));
        painter.setBrush(Theme::Brush(Theme::VanillaPageFieldBackground));
        painter.drawRoundedRect(ring, radius, radius);
    }
    QLineEdit::paintEvent(ev);
    if(!m_Count.isEmpty()){
        QPainter painter(this);
        painter.setPen(Theme::Pen(Theme::VanillaPageTextDim));
        const int right = m_Closable ? CloseReserve() + ScaleByDevice(6)
                                     : ScaleByDevice(RECEIVER_FIELD_PADDING_X);
        painter.drawText(rect().adjusted(0, 0, -right, 0),
                         Qt::AlignRight | Qt::AlignVCenter, m_Count);
    }
    if(m_Closable){
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF button = QRectF(CloseRect());
        if(m_CloseHovered || m_ClosePressed){
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::Brush(m_ClosePressed && m_CloseHovered ? Theme::BarButtonPressed
                                                                          : Theme::FieldButtonHovered));
            const qreal radius = m_Pill ? button.height() / 2.0 : ScaleByDevice(CHIP_CORNER_RADIUS);
            painter.drawRoundedRect(button, radius, radius);
        }
        QPen pen = Theme::Pen(m_CloseHovered ? Theme::VanillaPageText : Theme::VanillaPageTextDim);
        pen.setWidthF(qMax(1.0, ScaleByDevice(10) / 10.0));
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        const qreal a = ScaleByDevice(40) / 10.0;
        const QPointF c = button.center();
        painter.drawLine(c + QPointF(-a, -a), c + QPointF(a, a));
        painter.drawLine(c + QPointF(-a, a), c + QPointF(a, -a));
    }
    if(!m_Tag.isEmpty()){
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const int left = m_Pill ? ScaleByDevice(RECEIVER_FIELD_PADDING_X) : ScaleByDevice(FIELD_PADDING_X);
        const QRectF chip = QRectF(TagRect(this, fontMetrics(), m_Tag, left, ScaleByDevice(6)))
            .adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = ScaleByDevice(4);
        painter.setPen(Theme::Pen(Theme::VanillaPageTextDim));
        painter.setBrush(Theme::Brush(Theme::ReceiverBackgroundContrast));
        painter.drawRoundedRect(chip, radius, radius);
        painter.setPen(Theme::Pen(Theme::ReceiverTextContrast));
        painter.drawText(chip, Qt::AlignCenter, m_Tag);
    }
}

QRect LineEdit::AddressRect(const QWidget *site, const QWidget *field){
    if(!field || !field->isVisible()) return QRect();
    const QRect f = field->geometry();
    if(!site || !site->isVisible() || site->parentWidget() != field->parentWidget()) return f;
    return QRect(QPoint(qMin(site->x(), f.left()), f.top()), f.bottomRight());
}

bool LineEdit::AddressHovered(const QWidget *host, const QWidget *site, const QWidget *field){
    if(!host || !host->isVisible() || !host->window()->isActiveWindow()) return false;
    return AddressRect(site, field).contains(host->mapFromGlobal(QCursor::pos()));
}

void LineEdit::PaintAddressHover(QPainter *painter, const QWidget *host,
                                 const QWidget *site, const QWidget *field){
    if(!AddressHovered(host, site, field)) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Theme::Brush(Theme::LineEditBackgroundActive));
    const int radius = DeviceScale::FromDpi(FIELD_CORNER_RADIUS, static_cast<int>(host->logicalDpiY()));
    painter->drawRoundedRect(AddressRect(site, field), radius, radius);
    painter->restore();
}

bool LineEdit::event(QEvent *ev){
    if(ev->type() == QEvent::KeyPress){
        QKeyEvent *key = static_cast<QKeyEvent*>(ev);
        if((key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) &&
           !(key->modifiers() & (Qt::ControlModifier | Qt::AltModifier))){
            if(Receiver *receiver = qobject_cast<Receiver*>(parentWidget())){
                if(receiver->HandleMappedKey(key)) return true;
            }
        }
    }
    return QLineEdit::event(ev);
}

void LineEdit::focusInEvent(QFocusEvent *ev){
    QLineEdit::focusInEvent(ev);
    emit FocusIn(ev->reason());
    ev->setAccepted(true);
}

void LineEdit::focusOutEvent(QFocusEvent *ev){
    QLineEdit::focusOutEvent(ev);
    emit FocusOut(ev->reason());
    ev->setAccepted(true);
}

void LineEdit::keyPressEvent(QKeyEvent *ev){
    if(Receiver *receiver = qobject_cast<Receiver*>(parentWidget())){
        if(receiver->HandleMappedKey(ev)){
            ev->setAccepted(true);
            return;
        }
    }
    QLineEdit::keyPressEvent(ev);
    if(!ev->isAccepted()){
        if(ev->key() == Qt::Key_Up){
            emit SelectPrevSuggest();
            ev->setAccepted(true);
            return;
        }
        if(ev->key() == Qt::Key_Down){
            emit SelectNextSuggest();
            ev->setAccepted(true);
            return;
        }
        if(ev->key() == Qt::Key_Return){
            emit Returned();
            ev->setAccepted(true);
            return;
        }
        if(ev->key() == Qt::Key_Escape){
            emit Aborted();
            ev->setAccepted(true);
            return;
        }
    }
}

void LineEdit::inputMethodEvent(QInputMethodEvent *ev){
    QLineEdit::inputMethodEvent(ev);
    QString shown = text();
    shown.insert(cursorPosition(), ev->preeditString());
    emit textChanged(shown);
}

void LineEdit::mousePressEvent(QMouseEvent *ev){
    if(CloseRect().contains(ev->position().toPoint())){
        if(ev->button() == Qt::LeftButton){
            m_ClosePressed = true;
            update(CloseRect());
        }
        ev->accept();
        return;
    }
    if(ev->button() == Qt::LeftButton && selectedText() == text()) deselect();
    QLineEdit::mousePressEvent(ev);
}

void LineEdit::mouseReleaseEvent(QMouseEvent *ev){
    if(m_ClosePressed && ev->button() == Qt::LeftButton){
        m_ClosePressed = false;
        update(CloseRect());
        ev->accept();
        if(CloseRect().contains(ev->position().toPoint())){
            SetCloseHovered(false);
            emit Aborted();
        }
        return;
    }
    QLineEdit::mouseReleaseEvent(ev);
}

void LineEdit::mouseMoveEvent(QMouseEvent *ev){
    SetCloseHovered(CloseRect().contains(ev->position().toPoint()));
    if(m_ClosePressed){
        ev->accept();
        return;
    }
    QLineEdit::mouseMoveEvent(ev);
}

void LineEdit::mouseDoubleClickEvent(QMouseEvent *ev){
    if(CloseRect().contains(ev->position().toPoint())){
        mousePressEvent(ev);
        return;
    }
    QLineEdit::mouseDoubleClickEvent(ev);
}

void LineEdit::leaveEvent(QEvent *ev){
    SetCloseHovered(false);
    QLineEdit::leaveEvent(ev);
}

void LineEdit::resizeEvent(QResizeEvent *ev){
    QLineEdit::resizeEvent(ev);
    if(m_Closable) UpdateTextMargins();
}

Receiver::Receiver(TreeBank *parent, bool purge)
    : QWidget(purge ? nullptr : parent)
    , m_Mode(Command)
{
    m_TreeBank = parent;

    if(purge){
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    if(!m_LocalServer){
        QLocalServer::removeServer(Application::LocalServerName());
        m_LocalServer = new QLocalServer();
        m_LocalServer->setSocketOptions(QLocalServer::UserAccessOption);
        if(!m_LocalServer->listen(Application::LocalServerName()))
            qWarning() << "Could not listen on the local command server:"
                       << m_LocalServer->errorString();
    }
    m_Server = m_LocalServer;
    connect(m_LocalServer, &QLocalServer::newConnection,
            this, &Receiver::ForeignCommandReceived);

    m_LineEdit = new LineEdit(this);
    m_LineEdit->SetPill(true);
    m_LineEdit->SetClosable(true);
    setFocusProxy(m_LineEdit);
    m_LineEdit->show();
    hide();

    m_LineString = QString();
    m_SuggestStrings = QStringList();
    m_CurrentSuggestIndex = -1;

    connect(m_LineEdit, &LineEdit::FocusIn, m_LineEdit, &QLineEdit::selectAll);
    connect(m_LineEdit, &LineEdit::textChanged,        this, &Receiver::SetString);
    connect(m_LineEdit, &LineEdit::FocusOut,           this, &Receiver::OnFieldFocusOut);
    connect(m_LineEdit, &LineEdit::SelectNextSuggest,  this, &Receiver::SelectNextSuggest);
    connect(m_LineEdit, &LineEdit::SelectPrevSuggest,  this, &Receiver::SelectPrevSuggest);
    connect(m_LineEdit, &LineEdit::Returned,           this, &Receiver::OnReturned);
    connect(m_LineEdit, &LineEdit::Aborted,            this, &Receiver::OnAborted);
    connect(this, &Receiver::SeekText, this, [this](const QString &words){
        if(words.isEmpty()) m_LineEdit->SetCount(QString());
    });
}

void Receiver::SetFindCount(int active, int total){
    if(!isVisible() || !IsSeeking()) return;
    m_LineEdit->SetCount(QStringLiteral("%1 / %2").arg(qMax(0, active)).arg(qMax(0, total)));
}

Receiver::~Receiver(){
    if(m_Anchor){
        QWidget *field = m_Anchor;
        m_Anchor = nullptr;
        field->removeEventFilter(this);
        emit AnchorReleased(field, false);
    }
    m_LineEdit->deleteLater();

    if(!m_Server || m_Server != m_LocalServer) return;

    disconnect(m_LocalServer, SIGNAL(newConnection()),
               this, SLOT(ForeignCommandReceived()));

    if(1 > Application::GetMainWindows().size()){
        m_LocalServer->close();
        m_LocalServer->deleteLater();
        m_LocalServer = nullptr;
    }
}

bool Receiver::IsPurged() const {
    return isWindow();
}

void Receiver::MakeOwnedWindow(){
    const bool v = isVisible();

    hide();
    setAttribute(Qt::WA_TranslucentBackground);
    setParent(m_TreeBank ? m_TreeBank->GetMainWindow() : nullptr,
              Qt::FramelessWindowHint | Qt::Tool);

    if(v) show();
}

void Receiver::TakeWindowOwnerIfNeed(){
    if(!isWindow() || !m_TreeBank) return;
    QWidget *win = m_TreeBank->GetMainWindow();
    if(!win || !win->isVisible() || parentWidget() == win) return;
    MakeOwnedWindow();
}

void Receiver::Purge(){
    if(m_Anchor) return;
    MakeOwnedWindow();
}

void Receiver::Join(){
    if(m_Anchor) return;
    bool v = isVisible();
    setParent(m_TreeBank);
    if(v) show();
    else hide();
}

void Receiver::ResizeNotify(QSize size){
    TakeWindowOwnerIfNeed();

    const Placement placement = CurrentPlacement();
    const bool center = placement == Center;
    const int margin = ScaleByDevice(RECEIVER_MARGIN);
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int field = LineHeight();
    const int gap = ScaleByDevice(RECEIVER_LIST_GAP);
    const int row = ScaleByDevice(SUGGEST_HEIGHT);

    const int right = size.width() - m_TreeBank->MiniMapStripWidth();
    int w = qMin(ScaleByDevice(center ? RECEIVER_CENTER_WIDTH : RECEIVER_CORNER_WIDTH),
                 qMax(right - margin * 2, 1));
    const int top = center ? size.height() / 12 : margin;
    const int bar = ShowsSourceBar() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0;
    int room = qMax(size.height() - top * 2 - pad * 2 - field - gap - bar, 0);
    QPoint pos(center ? (right - w) / 2 : right - w - margin, top);

    const QRect tree(m_TreeBank->mapToGlobal(QPoint(0, 0)), size);
    const QRect at = m_Anchor ? QRect(m_Anchor->mapToGlobal(QPoint(0, 0)), m_Anchor->size()) : QRect();
    const bool over = IsOverField();
    const bool below = LineBelow();
    int edge = pad;
    if(over){
        const QRect screen = m_Anchor->screen()->availableGeometry();
        w = qMin(qMax(at.width() + pad * 2, ScaleByDevice(RECEIVER_CORNER_WIDTH)), screen.width());
        pos.setX(qBound(screen.left(), at.left() - pad, screen.right() + 1 - w));
        if(below){
            edge = qBound(0, screen.bottom() - at.bottom(), pad);
            const int limit = qMax(tree.top(), screen.top()) + margin;
            room = qMax(at.top() - limit - pad - gap - bar, 0);
        } else {
            edge = qBound(0, at.top() - screen.top(), pad);
            pos.setY(at.top() - edge);
            const int bottom = qMin(tree.bottom(), screen.bottom()) + 1 - margin;
            room = qMax(bottom - pos.y() - pad - edge - field - gap - bar, 0);
        }
    } else if(IsPurged()){
        pos = m_TreeBank->mapToGlobal(pos);
    }
    int rows = 0, used = 0;
    for(; rows < m_SuggestStrings.length(); rows++){
        used += row + (HasHeading(rows) ? ScaleByDevice(RECEIVER_HEADING_HEIGHT) : 0);
        if(used > room) break;
    }
    if(rows < m_SuggestStrings.length())
        used -= row + (HasHeading(rows) ? ScaleByDevice(RECEIVER_HEADING_HEIGHT) : 0);
    if(ListScrolls() && rows < m_SuggestStrings.length()) used = room;
    const int h = pad + edge + field + (rows || bar ? gap + bar + used : 0);
    if(!ListScrolls() && m_CurrentSuggestIndex >= rows) m_CurrentSuggestIndex = -1;

    if(below){
        pos.setY(at.top() - h + edge + field);
        pos.setY(qMax(pos.y(), m_Anchor->screen()->availableGeometry().top()));
    }

    setGeometry(QRect(pos, QSize(w, h)));
    m_LineEdit->setGeometry(pad, below ? h - edge - field : edge, w - pad * 2, field);
    KeepSelectionShown();
}

bool Receiver::IsOverField() const {
    return m_Anchor && IsPurged() && CurrentPlacement() == Anchored;
}

bool Receiver::LineBelow() const {
    if(!IsOverField() || !m_TreeBank) return false;
    const QRect tree(m_TreeBank->mapToGlobal(QPoint(0, 0)), m_TreeBank->size());
    return m_Anchor->mapToGlobal(m_Anchor->rect().center()).y() >= tree.center().y();
}

int Receiver::LineHeight() const {
    return IsOverField() ? qMax(m_Anchor->height(), 1) : ScaleByDevice(RECEIVER_FIELD_HEIGHT);
}

int Receiver::ListTop() const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    return LineBelow() ? pad : m_LineEdit->geometry().bottom() + 1 + ScaleByDevice(RECEIVER_LIST_GAP);
}

int Receiver::ListBottom() const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    return LineBelow() ? m_LineEdit->y() - ScaleByDevice(RECEIVER_LIST_GAP)
                       : height() - pad;
}

bool Receiver::SeekWords(const QString &line, QString *words){
    static const QRegularExpression form(QStringLiteral("^(\\S+)[\\n\\r\\t ]+(.*)$"),
                                         QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = form.match(line);
    if(!m.hasMatch()) return false;
    QString action;
    if(CommandMap::Resolve(m.captured(1), &action) != CommandMap::Seek) return false;
    if(words) *words = m.captured(2);
    return true;
}

bool Receiver::IsSeeking() const {
    return m_Mode == Search || (m_Mode == Command && TabSeeks() && SeekWords(m_LineString));
}

bool Receiver::TabSeeks() const {
    return m_ActiveTab == 0 || m_ActiveTab == SuggestRow::Commands + 1 ||
        (m_Anchor && m_ActiveTab == SuggestRow::Web + 1);
}

Receiver::Placement Receiver::CurrentPlacement() const {
    if(IsSeeking() && !m_Anchor) return TopRight;
    return m_Anchor ? Anchored : Center;
}

bool Receiver::HasHeading(int index) const {
    if(m_Rows.isEmpty() || index >= m_Rows.length()) return false;
    if(m_ActiveTab != 0 || m_Mode == Target){
        return !m_Rows[index].section.isEmpty() &&
            (index == 0 || m_Rows[index].section != m_Rows[index - 1].section);
    }
    return index == 0 || m_Rows[index].source != m_Rows[index - 1].source;
}

int Receiver::VisibleSuggestCount() const {
    const int bottom = ListBottom()
        - (ShowsSourceBar() && LineBelow() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0);
    int count = 0;
    while(m_SuggestScroll + count < m_SuggestStrings.length() &&
          SuggestRect(m_SuggestScroll + count).bottom() < bottom) count++;
    return count;
}

bool Receiver::ListScrolls() const {
    return m_ActiveTab != 0 || m_Rows.isEmpty() || m_Mode == Target;
}

int Receiver::SelectableSuggestCount() const {
    return ListScrolls() ? static_cast<int>(m_SuggestStrings.length()) : VisibleSuggestCount();
}

int Receiver::LastSuggestScroll() const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int row = ScaleByDevice(SUGGEST_HEIGHT);
    Q_UNUSED(pad)
    const int room = ListBottom() - ListTop()
        - (ShowsSourceBar() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0);
    int used = 0;
    for(int i = m_SuggestStrings.length() - 1; i >= 0; i--){
        used += row + (HasHeading(i) ? ScaleByDevice(RECEIVER_HEADING_HEIGHT) : 0);
        if(used > room) return qMin(i + 1, static_cast<int>(m_SuggestStrings.length()) - 1);
    }
    return 0;
}

void Receiver::KeepSelectionShown(){
    const int last = LastSuggestScroll();
    m_SuggestScroll = qBound(0, m_SuggestScroll, qMax(0, last));
    if(m_CurrentSuggestIndex < 0 || m_SuggestStrings.isEmpty()) return;
    if(m_CurrentSuggestIndex < m_SuggestScroll) m_SuggestScroll = m_CurrentSuggestIndex;
    if(!VisibleSuggestCount()){
        m_CurrentSuggestIndex = -1;
        return;
    }
    while(m_SuggestScroll < last && m_CurrentSuggestIndex >= m_SuggestScroll + VisibleSuggestCount())
        m_SuggestScroll++;
}

void Receiver::ScrollSuggestsBy(int rows){
    if(!ListScrolls() || !VisibleSuggestCount() || VisibleSuggestCount() >= m_SuggestStrings.length()) return;
    m_SuggestScroll = qBound(0, m_SuggestScroll + rows, qMax(0, LastSuggestScroll()));
    if(m_CurrentSuggestIndex != -1)
        m_CurrentSuggestIndex = qBound(m_SuggestScroll, m_CurrentSuggestIndex,
                                       m_SuggestScroll + VisibleSuggestCount() - 1);
    repaint();
}

bool Receiver::ScrollToPointer(const QPoint &pos){
    const int count = VisibleSuggestCount();
    if(!ListScrolls() || !count || count >= m_SuggestStrings.length()) return false;
    const int top = SuggestRect(m_SuggestScroll).top();
    const int track = SuggestRect(m_SuggestScroll + count - 1).bottom() - top;
    if(track <= 0) return false;
    const int last = LastSuggestScroll();
    ScrollSuggestsBy(qRound(qreal(pos.y() - top) * last / track) - m_SuggestScroll);
    return true;
}

QRect Receiver::SuggestRect(int index) const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int row = ScaleByDevice(SUGGEST_HEIGHT);
    int top = ListTop()
        + (index - m_SuggestScroll) * row
        + (ShowsSourceBar() && !LineBelow() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0);
    for(int i = m_SuggestScroll; i <= index; i++){
        if(HasHeading(i)) top += ScaleByDevice(RECEIVER_HEADING_HEIGHT);
    }
    return QRect(pad, top, width() - pad * 2, row);
}

int Receiver::SuggestAt(const QPoint &pos) const {
    const int count = VisibleSuggestCount();
    for(int i = m_SuggestScroll; i < m_SuggestScroll + count; i++){
        if(SuggestRect(i).contains(pos)) return i;
    }
    return -1;
}

void Receiver::Dismiss(){
    if(isVisible() && IsSeeking() && !m_TreeBank->IsDisplayingTableView())
        emit SeekText(QString(), View::HighlightAllOccurrences);
    hide();
}

bool Receiver::ShouldCloseOnFocusLeft(bool center, bool fieldHasFocus,
                                      bool popupIsUp, bool anyWindowActive){
    return center && !fieldHasFocus && !popupIsUp && anyWindowActive;
}

void Receiver::OnFieldFocusOut(){
    if(CurrentPlacement() == TopRight) return;
    QTimer::singleShot(0, this, &Receiver::CloseIfFocusLeft);
}

void Receiver::CloseIfFocusLeft(){
    if(!isVisible()) return;
    if(!ShouldCloseOnFocusLeft(CurrentPlacement() != TopRight, m_LineEdit->hasFocus(),
                               QApplication::activePopupWidget() != nullptr,
                               QApplication::activeWindow() != nullptr))
        return;
    DismissFocusLeft();
}

void Receiver::DismissFocusLeft(){
    m_FocusWentElsewhere = true;
    Dismiss();
}

void Receiver::InitializeDisplay(Mode mode, int tab, const QString &text){
    bool v = isVisible();

    if(mode == Search){
        if(m_Mode != Search){
            m_LineEdit->setText(QString());
            m_LineString = QString();
            m_SuggestStrings = QStringList();
            m_Rows.clear();
            m_CurrentSuggestIndex = -1;
        } else if(v && !m_LineString.isEmpty()){
            emit SeekText(m_LineString, View::HighlightAllOccurrences);
        }
    } else {
        if(v && m_Mode == Search)
            emit SeekText(QString(), View::HighlightAllOccurrences);
        m_LineEdit->setText(QString());
        m_LineString = QString();
        m_SuggestStrings = QStringList();
        m_Rows.clear();
        m_CurrentSuggestIndex = -1;
    }
    m_LineEdit->SetTag(QString());
    if(mode != Target){
        m_Target = TargetArgs();
        m_TargetRequest++;
    }

    m_Mode = mode;
    m_ActiveTab = tab;
    m_WebSuggests.clear();
    ReadBindings();

    if(!text.isEmpty()) m_LineEdit->setText(text);
    else if(m_Mode == Command || m_Mode == Target) SuggestFromSources(QString());

    UpdatePlaceholder();

    show(); m_TreeBank->RestackChildWidgets(); repaint();

    if(IsPurged()){
        raise();
        activateWindow();
    } else if(m_TreeBank){
        if(SharedView view = m_TreeBank->GetCurrentView())
            view->TakeKeyboardBack();
    }

    if(v){
        setFocus(Qt::OtherFocusReason);
        ResizeNotify(m_TreeBank->size());
        m_LineEdit->show();
        m_LineEdit->raise();
    }
    m_LineEdit->setFocus(Qt::OtherFocusReason);
}

void Receiver::OpenTextSeeker(View*, const QString &words){
    if(m_Anchor) Dismiss();
    InitializeDisplay(Search);
    if(!words.isEmpty()) m_LineEdit->setText(words.simplified());
}

void Receiver::OpenQueryEditor(View*, const QString &words){
    if(m_Anchor) Dismiss();
    InitializeDisplay(Query);
    if(!words.isEmpty()) m_LineEdit->setText(words.simplified());
}

void Receiver::OpenUrlEditor(View*){
    if(m_Anchor) Dismiss();
    InitializeDisplay(UrlEdit);
}

void Receiver::OpenCommand(View*){
    if(m_Anchor) Dismiss();
    InitializeDisplay(Command);
}

void Receiver::OpenTargetWhenFocused(const TargetArgs &target, QWidget *view){
    const quint64 request = ++m_TargetRequest;
    const QPointer<Receiver> self(this);
    const QPointer<QWidget> base = view;
    const auto connection = std::make_shared<QMetaObject::Connection>();
    const auto open = [self, target, request, connection](){
        QObject::disconnect(*connection);
        if(!self || self->m_TargetRequest != request) return;
        if(target.nodes.isEmpty() && !LiveTarget(target, self->m_TreeBank)) return;
        self->OpenTarget(target);
    };
    *connection = connect(qApp, &QApplication::focusChanged, this,
                          [base, open](QWidget*, QWidget *now){
        if(base && now && (now == base || base->isAncestorOf(now)))
            QTimer::singleShot(0, open);
    });
    QTimer::singleShot(500, this, open);
}

Receiver::TargetTables Receiver::CurrentTargetTables(const TargetArgs &target){
    TargetTables tables;
    tables.link = View::GetLinkMenu();
    tables.image = View::GetImageMenu();
    tables.selection = View::GetSelectionMenu();
    tables.media = View::GetMediaMenu();
    tables.field = View::GetTextMenu();
    if(const MenuSchema::Table *table = MenuSchema::Find(target.nodeTable.isEmpty()
                                                         ? QStringLiteral("TableNodeMenu") : target.nodeTable))
        tables.node = MenuSchema::Current(Application::GlobalSettings(), *table).join(QLatin1Char(','));
    tables.engines = Page::GetSearchEngineMap().keys();
    for(const auto &command : Application::ExternalCommands()) tables.commands << command.first;
    return tables;
}

void Receiver::OpenTarget(const TargetArgs &target){
    if(TargetRows(target, CurrentTargetTables(target), QString()).isEmpty()){
        OpenCommand();
        return;
    }
    if(isVisible()) Dismiss();
    m_Target = target;
    const quint64 request = ++m_TargetRequest;
    InitializeDisplay(Target);
    m_LineEdit->SetTag(m_Target.nodes.isEmpty() ? TargetTag(m_Target) : NodeTag(m_Target));

    if(!m_Target.selection.isEmpty() && m_Target.selectionHtml.isEmpty()){
        if(const SharedView view = LiveTarget(m_Target, m_TreeBank)){
            const QPointer<Receiver> self(this);
            view->CallWithSelectedHtml([self, request](QString html){
                if(self) self->TakeSelectionHtml(request, html);
            });
        }
    }
}

void Receiver::TakeSelectionHtml(quint64 request, const QString &html){
    if(m_TargetRequest == request) m_Target.selectionHtml = html;
}

ViewNode *Receiver::HeldNode(quint64 serial){
    if(!serial) return nullptr;
    for(ViewNode *root : { TreeBank::GetViewRoot(), TreeBank::GetTrashRoot() }){
        if(!root) continue;
        foreach(Node *nd, root->GetDescendants())
            if(nd->IsViewNode() && nd->GetSerial() == serial) return nd->ToViewNode();
    }
    return nullptr;
}

QString Receiver::NodeTag(const TargetArgs &target){
    static const int length = 25;
    ViewNode *first = target.nodes.isEmpty() ? nullptr : HeldNode(target.nodes.first());
    QString title = first ? first->GetTitle().simplified() : QString();
    if(title.length() > length) title = title.left(length) + QStringLiteral("...");
    const QString part = TargetPartName(TargetNode);
    if(target.nodes.length() > 1) return part + QStringLiteral(" ") + tr("%n tab(s)", nullptr, target.nodes.length());
    return part + QStringLiteral(" ") + title;
}

void Receiver::OpenAtField(QWidget *field){
    if(isVisible()) Dismiss();
    if(field && field->isVisible() && m_TreeBank && m_TreeBank->GetMainWindow()){
        if(!IsPurged()){
            MakeOwnedWindow();
            m_RejoinAfterHide = true;
        }
        m_Anchor = field;
        field->installEventFilter(this);
        m_TreeBank->GetMainWindow()->installEventFilter(this);
        m_LineEdit->setProperty(LineEdit::AddressHoverProperty,
                                field->property(LineEdit::AddressHoverProperty));
        m_LineEdit->SetPill(false);
        m_LineEdit->setFont(ToolBarFieldFont());
    }
    InitializeDisplay(Command, SuggestRow::Web + 1, CurrentAddress().toString());
    m_LineEdit->selectAll();
}

void Receiver::ReleaseAnchor(bool focusLeft){
    if(!m_Anchor) return;
    QWidget *field = m_Anchor;
    m_Anchor->removeEventFilter(this);
    if(m_TreeBank && m_TreeBank->GetMainWindow())
        m_TreeBank->GetMainWindow()->removeEventFilter(this);
    m_Anchor = nullptr;
    m_LineEdit->setProperty(LineEdit::AddressHoverProperty, QVariant());
    m_LineEdit->SetPill(true);
    m_LineEdit->setFont(QFont());
    emit AnchorReleased(field, focusLeft);
}

QStringList Receiver::UrlAncestors(const QString &text, const QUrl &current){
    QStringList list;
    if(!current.isValid()) return list;
    const QString whole = current.toString();
    if(!whole.startsWith(text, Qt::CaseInsensitive) && !text.startsWith(whole, Qt::CaseInsensitive)) return list;

    QUrl up = current;
    up.setFragment(QString());
    up.setQuery(QString());
    if(up != current) list << up.toString();
    QString path = current.path();
    if(!path.endsWith(QLatin1Char('/'))) path = path.left(path.lastIndexOf(QLatin1Char('/')) + 1);
    else if(path.length() > 1) path = path.left(path.lastIndexOf(QLatin1Char('/'), path.length() - 2) + 1);
    else path.clear();
    while(!path.isEmpty()){
        up.setPath(path);
        list << up.toString();
        if(path.length() == 1) break;
        path = path.left(path.lastIndexOf(QLatin1Char('/'), path.length() - 2) + 1);
    }
    list.removeAll(text);
    return list;
}

bool Receiver::IsUrlToLoadInPlace(const QString &text){
    static const QStringList prefixes = QStringList()
        << QStringLiteral("http://")
        << QStringLiteral("https://")
        << QStringLiteral("javascript:")
        << QStringLiteral("about:")
        << QStringLiteral("chrome:")
        << QStringLiteral("edge:")
        << VANILLA_SCHEME + QStringLiteral(":");
    for(const QString &prefix : prefixes){
        if(text.startsWith(prefix)) return true;
    }
    return false;
}

bool Receiver::eventFilter(QObject *watched, QEvent *ev){
    if(m_Anchor && isVisible()){
        if(watched == m_Anchor){
            if(ev->type() == QEvent::Move || ev->type() == QEvent::Resize)
                ResizeNotify(m_TreeBank->size());
            else if(ev->type() == QEvent::Hide)
                Dismiss();
        } else if(m_TreeBank && watched == m_TreeBank->GetMainWindow() &&
                  (ev->type() == QEvent::Move || ev->type() == QEvent::Resize)){
            Dismiss();
        }
    }
    return QWidget::eventFilter(watched, ev);
}

void Receiver::SubmitAddress(const QString &text){
    const SharedView view = m_TreeBank->GetCurrentView();
    const bool shift = Application::keyboardModifiers() & Qt::ShiftModifier;
    hide();
    if(text.trimmed().isEmpty()) return;
    if(!shift && view && IsUrlToLoadInPlace(text)) view->Load(text);
    else ReceiveCommand(text);
}

void Receiver::OnReturned(){
    SuitableAction();
}

void Receiver::OnAborted(){
    if(IsSeeking()){
        emit SeekText(QString(), View::HighlightAllOccurrences);
    }
    hide();
}

void Receiver::ForeignCommandReceived(){
    MainWindow *win = Application::GetCurrentWindow();
    if(!m_Server || !win || win->GetTreeBank() != m_TreeBank) return;

    const QPointer<Receiver> self(this);
    const QPointer<QLocalServer> server = m_Server;
    while(server && server->hasPendingConnections()){
        QLocalSocket *clientConnection = server->nextPendingConnection();
        if(!clientConnection) continue;

        connect(clientConnection, &QLocalSocket::readyRead, this,
                [this, clientConnection](){ ReadForeignCommand(clientConnection);});
        connect(clientConnection, &QLocalSocket::disconnected,
                clientConnection, &QLocalSocket::deleteLater);
        ReadForeignCommand(clientConnection);
        if(!self) return;
    }
}

void Receiver::ReadForeignCommand(QLocalSocket *clientConnection){
    QString command;
    const CommandFrame::Result result = CommandFrame::Read(clientConnection, &command);
    if(result == CommandFrame::Incomplete) return;

    clientConnection->disconnectFromServer();
    if(result != CommandFrame::Complete) return;

    if(TreeBank::IsMovingSpace()){
        const QPointer<Receiver> self(this);
        TreeBank::WhenItMayChange([self, command](){ if(self) self->ReceiveCommand(command);});
        return;
    }
    ReceiveCommand(command);
}

void Receiver::SetString(QString str){
    const bool wasSeeking = m_Mode == Command && IsSeeking();
    const Placement before = CurrentPlacement();
    const bool same = str == m_LineString;
    m_LineString = str;

    if(m_Mode == Search){
        if(!same) emit SeekText(m_LineString, View::HighlightAllOccurrences);
        repaint();
    }
    if(m_Mode == Command){
        QString words;
        if(TabSeeks() && SeekWords(m_LineString, &words)){
            if(!m_SuggestStrings.isEmpty()) SetSuggest(QStringList());
            if(!same) emit SeekText(words, View::HighlightAllOccurrences);
        } else if(wasSeeking){
            emit SeekText(QString(), View::HighlightAllOccurrences);
        }
    }
    AskForSuggestions();
    if(m_Mode == Command || m_Mode == Target){
        SuggestFromSources(str);
    }
    if(CurrentPlacement() != before) ResizeNotify(m_TreeBank->size());
}

namespace {
    bool MeansSomething(const QString &word){
        return CommandMap::Resolve(word) != CommandMap::NotACommand ||
            Page::GetSearchEngineMap().contains(word);
    }

    const QRegularExpression &LineSpace(){
        static const QRegularExpression space(QStringLiteral("[\\n\\r\\t ]+"));
        return space;
    }
}

Receiver::SuggestRows Receiver::ComposeRows(const QVector<SuggestRows> &sources, int capacity){
    QVector<int> taken(sources.length(), 0);
    int total = 0;
    for(bool any = true; any && total < capacity;){
        any = false;
        for(int i = 0; i < sources.length() && total < capacity; i++){
            if(taken[i] < sources[i].length()){
                taken[i]++;
                total++;
                any = true;
            }
        }
    }
    SuggestRows result;
    for(int i = 0; i < sources.length(); i++) result += sources[i].mid(0, taken[i]);
    return result;
}

int Receiver::RowCapacity(int sources) const {
    const int height = m_TreeBank->height();
    const int room = height - (height / 12) * 2
        - ScaleByDevice(RECEIVER_PADDING) * 2 - ScaleByDevice(RECEIVER_FIELD_HEIGHT)
        - ScaleByDevice(RECEIVER_LIST_GAP) - ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT)
        - sources * ScaleByDevice(RECEIVER_HEADING_HEIGHT);
    return qMax(room / ScaleByDevice(SUGGEST_HEIGHT), sources);
}

void Receiver::Remember(const QString &line){
    QString action;
    if(CommandMap::Resolve(line.trimmed().split(LineSpace()).first(), &action) != CommandMap::NotACommand)
        RememberAction(action);
}

void Receiver::RememberAction(const QString &action){
    m_History.removeAll(action);
    m_History.prepend(action);
    while(m_History.length() > RECEIVER_HISTORY_LENGTH) m_History.removeLast();
}

namespace {
    void OrderByHistory(QStringList &names, const QStringList &history){
        const auto rank = [&history](const QString &name){
            QString action;
            if(CommandMap::Resolve(name.trimmed(), &action) == CommandMap::NotACommand) action = name.trimmed();
            const int index = history.indexOf(action);
            return index == -1 ? history.length() : index;
        };
        std::stable_sort(names.begin(), names.end(), [&rank](const QString &a, const QString &b){
            return rank(a) < rank(b);
        });
    }

    QStringList NamedCommands(){
        const auto reachable = [](const QString &name){
            return !name.contains(LineSpace()) && CommandMap::Resolve(name) == CommandMap::NotACommand;
        };
        QStringList names;
        const QMap<QString, SearchEngine> engines = Page::GetSearchEngineMap();
        for(auto it = engines.cbegin(); it != engines.cend(); ++it){
            if(reachable(it.key())) names << it.key() + QLatin1Char(' ');
        }
        const QMap<QString, Bookmarklet> bookmarklets = Page::GetBookmarkletMap();
        for(auto it = bookmarklets.cbegin(); it != bookmarklets.cend(); ++it){
            if(reachable(it.key()) && !engines.contains(it.key())) names << it.key();
        }
        return names;
    }
}

bool Receiver::AnswersHere(const QString &action, bool list, bool table){
    const bool inList = InputMapSchema::IsAction(InputMapSchema::GadgetsActions, action);
    const bool elsewhere = InputMapSchema::IsAction(InputMapSchema::PageActions, action) ||
        InputMapSchema::IsAction(InputMapSchema::TreeBankActions, action);
    if(!inList && !elsewhere) return true;
    if(list) return inList;
    return table || elsewhere;
}

Receiver::SuggestRows Receiver::CommandRows(const QString &typed, int limit, bool *exact) const {
    QStringList names;
    const QStringList named = NamedCommands();
    Gadgets *gadgets = m_TreeBank->GetGadgets();
    const bool list = gadgets && gadgets->IsActive();
    if(typed.isEmpty()){
        names = CommandMap::FirstCommands(list);
        foreach(const QString &name, CommandMap::ArgumentCommands() + named + CommandMap::AllCommands()){
            if(!names.contains(name)) names << name;
        }
        OrderByHistory(names, m_History);
    } else {
        names = CommandMap::Suggestions(typed, named.length() + CommandMap::AllCommands().length(),
                                        &Receiver::CommandLabel, named);

        const bool overview = m_TreeBank->IsDisplayingTableView();
        const QStringList words = typed.split(LineSpace(), Qt::SkipEmptyParts);
        bool longEnough = !words.isEmpty();
        for(const QString &word : words) if(word.length() < 2) longEnough = false;
        QStringList itself, held;
        for(const InputMapSchema::Binding &binding : m_Bindings){
            if(!overview && binding.vocabulary == InputMapSchema::GadgetsActions) continue;
            if(binding.vocabulary == InputMapSchema::ReceiverActions) continue;
            if(InputMapSchema::IsThisInput(binding, typed)){
                if(!itself.contains(binding.action)) itself << binding.action;
                continue;
            }
            if(!longEnough) continue;
            bool all = true;
            for(const QString &word : words){
                if(!binding.shown.contains(word, Qt::CaseInsensitive) &&
                   !binding.input.contains(word, Qt::CaseInsensitive)) all = false;
            }
            if(all && !held.contains(binding.action)) held << binding.action;
        }
        QString spelled;
        if(CommandMap::Resolve(typed.trimmed(), &spelled) == CommandMap::Signal){
            itself.removeAll(spelled);
            itself.prepend(spelled);
        }
        if(exact) *exact = !itself.isEmpty();
        for(const QString &action : held){
            if(!names.contains(action)) names << action;
        }
        for(const QString &action : itself) names.removeAll(action);
        OrderByHistory(names, m_History);
        names = itself + names;
    }
    const bool table = m_TreeBank->IsDisplayingTableView();
    for(int i = names.length() - 1; i >= 0; i--){
        if(!AnswersHere(names[i].trimmed(), list, table)) names.removeAt(i);
    }
    SuggestRows rows;
    foreach(const QString &name, names.mid(0, limit)){
        const QString label = CommandLabel(name.trimmed());
        const QStringList key = InputsOf(name);
        const SuggestRow::Kind kind =
            name.endsWith(QLatin1Char(' ')) ? SuggestRow::FillField :
            CommandMap::Resolve(name) == CommandMap::NotACommand && !named.contains(name) ?
            SuggestRow::RunAction : SuggestRow::RunCommand;
        const QString title = named.contains(name) ? name : name.left(1).toUpper() + name.mid(1);
        rows << SuggestRow{ SuggestRow::Commands, kind, title, label, name, 0,
                            QIcon(), QString(), QString(), key };
    }
    return rows;
}

namespace {
    QString HostOf(const QUrl &url){
        return url.host().isEmpty() ? url.toString() : url.host();
    }

    QIcon TabIcon(const QIcon &given){
        if(!given.isNull()){
            const QList<QSize> sizes = given.availableSizes();
            if(sizes.isEmpty() || sizes.first().width() > 2) return given;
        }
        static const QIcon blank  = Theme::Icon(QStringLiteral(":/resources/blank.png"));
        static const QIcon blankw = Theme::Icon(QStringLiteral(":/resources/blankw.png"));
        return Theme::IsDark() ? blankw : blank;
    }
}

Receiver::SuggestRows Receiver::TabRows(const QString &typed, int limit) const {
    SuggestRows rows;
    const ViewNode *current = m_TreeBank->GetCurrentViewNode();
    foreach(ViewNode *tab, VanillaPage::FindTabs(typed, m_TreeBank, limit + 1)){
        if(tab == current || rows.length() >= limit) continue;
        const QString title = tab->GetTitle();
        rows << SuggestRow{ SuggestRow::Tabs, SuggestRow::OpenTab,
                            title.isEmpty() ? tab->GetUrl().toString() : title,
                            HostOf(tab->GetUrl()), QString(), tab->GetSerial(),
                            TabIcon(Application::GetIcon(tab->GetUrl())) };
    }
    return rows;
}

Receiver::SuggestRows Receiver::TrashRows(const QString &typed, int limit) const {
    SuggestRows rows;
    foreach(ViewNode *tab, VanillaPage::FindTrashedTabs(typed, m_TreeBank, limit)){
        const QString title = tab->GetTitle();
        rows << SuggestRow{ SuggestRow::Trash, SuggestRow::OpenTab,
                            title.isEmpty() ? tab->GetUrl().toString() : title,
                            HostOf(tab->GetUrl()), QString(), tab->GetSerial(),
                            TabIcon(Application::GetIcon(tab->GetUrl())) };
    }
    return rows;
}

Receiver::SuggestRows Receiver::SettingRows(const QString &typed, int limit, bool unset) const {
    static const QStringList first = QStringList()
        << QStringLiteral("application/@ColorScheme")
        << QStringLiteral("application/@EnableTranslucentPanels");
    Settings &settings = Application::GlobalSettings();
    const QStringList words = typed.split(QRegularExpression(QStringLiteral("\\s+"),
                                                             QRegularExpression::UseUnicodePropertiesOption),
                                          Qt::SkipEmptyParts);
    const bool sectioned = m_ActiveTab != 0 && typed.isEmpty();
    QHash<QString, QString> categories;
    if(sectioned){
        typedef QPair<QString, QString> Category;
        for(const Category &category : SettingsSchema::Categories()) categories[category.first] = category.second;
    }
    SuggestRows firsts, rows;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        if(item.type == SettingsSchema::TextList) continue;
        const QString key = QString::fromLatin1(item.key);
        const QString label = QCoreApplication::translate("SettingsSchema", item.label);
        bool all = true;
        for(const QString &word : words){
            if(!key.contains(word, Qt::CaseInsensitive) && !label.contains(word, Qt::CaseInsensitive)) all = false;
        }
        if(!all) continue;
        if(unset && !settings.contains(key)) continue;
        const QString value = settings.value(key, QString::fromLatin1(item.fallback)).toString();
        const SuggestRow row{ SuggestRow::Settings, SuggestRow::FillField, label, value,
                              SettingLine(key, value, unset), 0, QIcon(),
                              categories.value(QString::fromLatin1(item.category)), key };
        if(typed.isEmpty() && first.contains(key)) firsts << row;
        else rows << row;
    }
    if(words.length() == 1){
        QSet<QString> listed;
        foreach(const SettingsSchema::Item &item, SettingsSchema::Items()) listed << QString::fromLatin1(item.key);
        foreach(const QString &key, settings.allKeys(words.first())){
            if(listed.contains(key)) continue;
            const QString value = settings.value(key).toString();
            rows << SuggestRow{ SuggestRow::Settings, SuggestRow::FillField, key, value,
                                SettingLine(key, value, unset), 0, QIcon(), QString(), key };
        }
    }
    if(unset) return (firsts + rows).mid(0, limit);
    SuggestRows itself, bindings;
    for(const InputMapSchema::Binding &binding : m_Bindings){
        const QString haystack = binding.shown + QLatin1Char(' ') + binding.input + QLatin1Char(' ') +
            binding.actionLabel + QLatin1Char(' ') + binding.action + QLatin1Char(' ') + binding.table;
        bool all = true;
        for(const QString &word : words){
            if(!haystack.contains(word, Qt::CaseInsensitive)) all = false;
        }
        const bool is = InputMapSchema::IsThisInput(binding, typed);
        if(!all && !is) continue;
        const QString does = binding.actionLabel.isEmpty() ? binding.action : binding.actionLabel;
        const SuggestRow row{ SuggestRow::Settings, SuggestRow::EditBinding, binding.shown,
                              sectioned ? does : does + QStringLiteral("   ") + binding.table,
                              BindingFind(binding), 0, QIcon(),
                              sectioned ? binding.table : QString() };
        if(is) itself << row;
        else bindings << row;
    }
    return (itself + firsts + rows + bindings).mid(0, limit);
}

Receiver::SuggestRows Receiver::ValueRows(const QString &rest, bool *matched){
    const QStringList words = rest.trimmed().split(LineSpace());
    const QString key = words.first();
    const QString typed = words.mid(1).join(QLatin1Char(' '));
    if(matched) *matched = false;
    const SettingsSchema::Item *item = SettingsSchema::Find(key);
    const QList<QPair<QString, QString> > values =
        item ? SettingsSchema::Values(*item) : QList<QPair<QString, QString> >();
    if(!values.isEmpty()){
        SuggestRows exact, like, others;
        for(const auto &value : values){
            const bool any = !typed.isEmpty();
            const SuggestRow row{ SuggestRow::Settings, SuggestRow::RunCommand, value.second,
                                  value.first == value.second ? QString() : value.first,
                                  SettingLine(key, value.first, false), 0, QIcon(), QString(), key };
            if(any && (value.first.compare(typed, Qt::CaseInsensitive) == 0 ||
                       value.second.compare(typed, Qt::CaseInsensitive) == 0)) exact << row;
            else if(any && (value.first.contains(typed, Qt::CaseInsensitive) ||
                            value.second.contains(typed, Qt::CaseInsensitive))) like << row;
            else if(!any) others << row;
        }
        if(matched) *matched = !exact.isEmpty() || !like.isEmpty();
        return exact + like + others;
    }
    if(!SettingsSchema::FromText(key, typed).isValid()) return SuggestRows();
    if(matched) *matched = true;
    const QString label = item ? QCoreApplication::translate("SettingsSchema", item->label) : key;
    return SuggestRows{ SuggestRow{ SuggestRow::Settings, SuggestRow::RunCommand, typed, label,
                                    SettingLine(key, typed, false), 0, QIcon(), QString(), key } };
}

QString Receiver::SettingNote(const QString &line){
    SuggestRow::Source source = SuggestRow::SourceCount;
    QString rest;
    bool unset = false;
    if(!SourceWord(line, &source, &rest, &unset) || source != SuggestRow::Settings || unset)
        return QString();
    const QStringList words = rest.trimmed().split(LineSpace());
    if(words.length() < 2 || !words.first().contains(QLatin1Char('/'))) return QString();
    const QString key = words.first();
    if(!SettingsSchema::IsKnownKey(key)) return tr("No such setting");
    if(SettingsSchema::FromText(key, words.mid(1).join(QLatin1Char(' '))).isValid()) return QString();
    return tr("Not a value it takes");
}

QString Receiver::SettingRefusal(const QString &key, const QString &text){
    if(SettingsSchema::FromText(key, text).isValid()) return QString();
    if(!SettingsSchema::IsKnownKey(key)) return tr("There is no setting \"%1\".").arg(key);
    const QString refused = tr("\"%1\" cannot be set to \"%2\".").arg(key, text);
    const SettingsSchema::Item *item = SettingsSchema::Find(key);
    if(!item) return refused;
    QString takes;
    switch(item->type){
    case SettingsSchema::Bool:
    case SettingsSchema::Choice: {
        QStringList values;
        for(const auto &value : SettingsSchema::Values(*item)) values << value.first;
        takes = tr("It takes one of: %1").arg(values.join(QStringLiteral(", ")));
        break;
    }
    case SettingsSchema::Int:      takes = tr("It takes a whole number."); break;
    case SettingsSchema::Color:    takes = tr("It takes a colour as #rrggbb, or nothing."); break;
    case SettingsSchema::TextList: takes = tr("It is a list: change it on the settings page."); break;
    default: break;
    }
    return takes.isEmpty() ? refused : refused + QLatin1Char('\n') + takes;
}

QString Receiver::BindingFind(const InputMapSchema::Binding &binding){
    return binding.input + QLatin1Char(' ') + binding.table;
}

QString Receiver::SourceName(SuggestRow::Source source){
    switch(source){
    case SuggestRow::Commands: return tr("Commands");
    case SuggestRow::Tabs:     return tr("Tabs");
    case SuggestRow::Trash:    return tr("Trash");
    case SuggestRow::Settings: return tr("Settings");
    case SuggestRow::Web:      return tr("Web");
    default: break;
    }
    return QString();
}

QString Receiver::SettingLine(const QString &key, const QString &value, bool unset){
    return unset ? QStringLiteral("unset %1").arg(key) : QStringLiteral("set %1 %2").arg(key, value);
}

bool Receiver::IsAssignment(const QString &rest, bool unset){
    const QStringList words = rest.trimmed().split(LineSpace());
    if(unset) return words.length() == 1 && Application::GlobalSettings().contains(words.first());
    if(words.length() >= 2) return words.first().contains(QLatin1Char('/'));
    const SettingsSchema::Item *item = SettingsSchema::Find(words.first());
    return !rest.isEmpty() && rest.back().isSpace() && item && !SettingsSchema::Values(*item).isEmpty();
}

bool Receiver::SettingCommand(const QString &line, QString *rest, bool *unset){
    static const QRegularExpression form(QStringLiteral("^(\\S+)[\\n\\r\\t ]+(.*)$"),
                                         QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = form.match(line);
    if(!m.hasMatch()) return false;
    const CommandMap::Kind kind = CommandMap::Resolve(m.captured(1));
    if(kind != CommandMap::Set && kind != CommandMap::Unset) return false;
    if(rest) *rest = m.captured(2);
    if(unset) *unset = kind == CommandMap::Unset;
    return true;
}

bool Receiver::SourceWord(const QString &line, SuggestRow::Source *source, QString *rest, bool *unset){
    static const QRegularExpression form(QStringLiteral("^(\\S+)(?:[\\n\\r\\t ]+(.*))?$"),
                                         QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = form.match(line);
    if(!m.hasMatch()) return false;
    const QString word = m.captured(1);

    if(Page::GetSearchEngineMap().contains(word)) return false;

    static const struct { const char *name; SuggestRow::Source source; } names[] = {
        { "commands", SuggestRow::Commands },
        { "tabs",     SuggestRow::Tabs },
        { "trash",    SuggestRow::Trash },
        { "settings", SuggestRow::Settings },
        { "web",      SuggestRow::Web },
    };
    int found = -1;
    if(word.length() >= 3){
        for(const auto &name : names){
            if(QLatin1String(name.name).startsWith(word, Qt::CaseInsensitive)) found = name.source;
        }
    }
    for(int i = 0; i < SuggestRow::SourceCount; i++){
        if(word.compare(SourceName(static_cast<SuggestRow::Source>(i)), Qt::CaseInsensitive) == 0) found = i;
    }
    const bool resetting = CommandMap::Resolve(word) == CommandMap::Unset;
    if(resetting) found = SuggestRow::Settings;
    if(found == -1) return false;
    if(unset) *unset = resetting;
    if(source) *source = static_cast<SuggestRow::Source>(found);
    if(rest) *rest = m.captured(2);
    return true;
}

bool Receiver::ShowsSourceBar() const {
    return m_Mode == Command && !IsSeeking() && (!m_Rows.isEmpty() || m_SuggestStrings.isEmpty());
}

QRect Receiver::SourceBarRect(int index) const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int count = SuggestRow::SourceCount + 1;
    const int width = (this->width() - pad * 2) / count;
    const int bar = ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT);
    return QRect(pad + index * width, LineBelow() ? ListBottom() - bar : ListTop(), width, bar);
}

int Receiver::SourceBarAt(const QPoint &pos) const {
    if(!ShowsSourceBar()) return -1;
    for(int i = 0; i <= SuggestRow::SourceCount; i++){
        if(SourceBarRect(i).contains(pos)) return i;
    }
    return -1;
}

void Receiver::RefillIfShown(){
    if(!isVisible() || m_Mode != Command || m_ActiveTab != 0 || m_Rows.isEmpty()) return;
    RefillKeepingChoice();
}

void Receiver::RefillKeepingChoice(){
    const int chosen = m_CurrentSuggestIndex;
    const QString title = chosen >= 0 && chosen < m_SuggestStrings.length() ? m_SuggestStrings[chosen] : QString();
    SuggestFromSources(m_LineString);
    if(!title.isEmpty() && m_SuggestStrings.contains(title)) m_CurrentSuggestIndex = m_SuggestStrings.indexOf(title);
    KeepSelectionShown();
    repaint();
}

void Receiver::AskForSuggestions(){
    const bool web = m_Mode == Command && m_ActiveTab == SuggestRow::Web + 1;
    const bool empty = m_LineString.trimmed().isEmpty();
    const bool address = web && (!Page::ExtractUrlsFromText(m_LineString).isEmpty() || IsSeeking());
    if(!web || empty || address) m_WebSuggests.clear();
    if((m_Mode != Query && !web) || address) return;
    if(empty){
        if(m_Mode == Query && !m_SuggestStrings.isEmpty()) SetSuggest(QStringList());
        return;
    }
    if(Application::EnableGoogleSuggest())
        emit SuggestRequest(SearchSuggest::Url(Application::SuggestEngine(), m_LineString));
}

void Receiver::SelectTab(int index){
    m_ActiveTab = index;
    UpdatePlaceholder();
    AskForSuggestions();
    SuggestFromSources(m_LineString);
    m_LineEdit->setFocus();
}

void Receiver::UpdatePlaceholder(){
    switch(m_Mode){
    case Command:
        m_LineEdit->setPlaceholderText(m_ActiveTab == 0 ? tr("All")
            : SourceName(static_cast<SuggestRow::Source>(m_ActiveTab - 1)));
        break;
    case Query:   m_LineEdit->setPlaceholderText(tr("Input Query.")); break;
    case UrlEdit: m_LineEdit->setPlaceholderText(tr("Enter a URL.")); break;
    case Search:  m_LineEdit->setPlaceholderText(tr("Incremental Search")); break;
    case Target:  m_LineEdit->setPlaceholderText(tr("What to do with it")); break;
    }
}

QUrl Receiver::CurrentAddress() const {
    const SharedView view = m_TreeBank ? m_TreeBank->GetCurrentView() : SharedView();
    if(!view || VanillaPage::IsStartUrl(view->url())) return QUrl();
    return view->url();
}

Receiver::SuggestRows Receiver::WebRows(const QString &typed, const QUrl &here) const {
    if(typed.isEmpty()){
        return SuggestRows{ SuggestRow{ SuggestRow::Web, SuggestRow::FillField, tr("Search the web"),
                                        QString(), QStringLiteral("query "), 0 } };
    }
    SuggestRows up;
    for(const QString &url : UrlAncestors(typed, here))
        up << SuggestRow{ SuggestRow::Web, SuggestRow::OpenAddress, url, QString(), url, 0 };
    SuggestRows rows;
    const bool address = !Page::ExtractUrlsFromText(typed).isEmpty();
    if(address){
        rows << SuggestRow{ SuggestRow::Web, SuggestRow::OpenAddress, typed, tr("Open addresses"), typed, 0 };
        rows << up;
    }
    rows << SuggestRow{ SuggestRow::Web, SuggestRow::SearchWeb, typed, tr("Search the web"), typed, 0 };
    if(!address) rows << up;
    for(const QString &word : m_WebSuggests){
        if(word != typed) rows << SuggestRow{ SuggestRow::Web, SuggestRow::SearchWeb, word, QString(), word, 0 };
    }
    return rows;
}

void Receiver::SuggestFromSources(const QString &typed){
    const QStringList words = typed.split(LineSpace());
    const bool oneWord = words.length() == 1;
    const bool means = MeansSomething(words.first());

    SuggestRows rows;
    int selected = -1;
    bool valued = true;
    if(m_Mode == Target){
        rows = TargetRows(m_Target, CurrentTargetTables(m_Target), typed);
        if(!rows.isEmpty()) selected = 0;
    } else if(TabSeeks() && SeekWords(typed)){
    } else if(m_ActiveTab != 0){
        const int limit = RECEIVER_SCROLL_ROWS;
        const QString wanted = typed.trimmed();
        switch(static_cast<SuggestRow::Source>(m_ActiveTab - 1)){
        case SuggestRow::Commands:{
            QString rest;
            bool unset = false;
            if(SettingCommand(typed, &rest, &unset)){
                if(!IsAssignment(rest, unset)) rows = SettingRows(rest.trimmed(), limit, unset);
                else if(!unset) rows = ValueRows(rest, &valued);
            } else {
                rows = CommandRows(wanted, limit);
            }
            break;
        }
        case SuggestRow::Tabs:     rows = TabRows(wanted, limit);     break;
        case SuggestRow::Trash:    rows = TrashRows(wanted, limit);   break;
        case SuggestRow::Settings: rows = SettingRows(wanted, limit); break;
        case SuggestRow::Web:      rows = WebRows(wanted, CurrentAddress()); break;
        default: break;
        }
        rows = rows.mid(0, limit);
        if(!rows.isEmpty() && valued) selected = 0;
    } else {
        SuggestRow::Source named = SuggestRow::SourceCount;
        QString rest;
        bool unset = false;
        const bool lifted = SourceWord(typed, &named, &rest, &unset);
        const auto wanted = [&](SuggestRow::Source source){
            return lifted && source == named ? rest.trimmed() : typed;
        };
        const bool assignment = lifted && named == SuggestRow::Settings && IsAssignment(rest, unset);

        const int limit = RowCapacity(1);
        const bool argument = !oneWord && means && !lifted;
        const bool commandsToo = !argument;

        SuggestRows commands, tabs, trash, settings, web;
        bool exact = false;
        if(!assignment){
            commands = CommandRows(wanted(SuggestRow::Commands), limit, &exact);
            if(!commandsToo && !exact){
                const bool given = !typed.mid(words.first().length()).trimmed().isEmpty();
                SuggestRows taker;
                for(SuggestRow row : CommandRows(words.first(), INT_MAX)){
                    if(row.kind != SuggestRow::FillField ||
                       row.text.trimmed().compare(words.first(), Qt::CaseInsensitive) != 0) continue;
                    if(given){
                        row.kind = SuggestRow::RunCommand;
                        row.text = typed;
                    }
                    taker << row;
                }
                commands = taker;
            }
            tabs = TabRows(wanted(SuggestRow::Tabs), limit);
            trash = TrashRows(wanted(SuggestRow::Trash), limit);
            settings = SettingRows(wanted(SuggestRow::Settings), limit, unset);
            if(!argument) web = WebRows(wanted(SuggestRow::Web), CurrentAddress());
        } else if(!unset){
            settings = ValueRows(rest, &valued);
        }

        QVector<SuggestRow::Source> order{ SuggestRow::Commands, SuggestRow::Tabs,
                                           SuggestRow::Trash, SuggestRow::Settings, SuggestRow::Web };
        const bool address = !typed.isEmpty() && !Page::ExtractUrlsFromText(typed).isEmpty();
        if(address){
            order.removeAll(SuggestRow::Web);
            order.prepend(SuggestRow::Web);
        }
        if(exact){
            order.removeAll(SuggestRow::Commands);
            order.prepend(SuggestRow::Commands);
        }
        if(lifted){
            order.removeAll(named);
            order.prepend(named);
        }
        QVector<SuggestRows> sources;
        for(const SuggestRow::Source source : order){
            switch(source){
            case SuggestRow::Commands: sources << commands; break;
            case SuggestRow::Tabs:     sources << tabs;     break;
            case SuggestRow::Trash:    sources << trash;    break;
            case SuggestRow::Settings: sources << settings; break;
            default:                   sources << web;      break;
            }
        }
        int present = 0;
        for(const SuggestRows &source : sources) if(!source.isEmpty()) present++;
        rows = ComposeRows(sources, RowCapacity(present));

        const bool asked = lifted && !rest.trimmed().isEmpty() && !rows.isEmpty() && rows.first().source == named;
        if(asked ||
           (!rows.isEmpty() && !typed.isEmpty() && !means && !Page::GetBookmarkletMap().contains(typed)))
            selected = 0;
        if(!valued) selected = -1;
    }

    if(m_Anchor && m_ActiveTab == SuggestRow::Web + 1) selected = -1;

    const QString note = rows.isEmpty() && m_Mode == Command ? SettingNote(typed) : QString();
    if(!note.isEmpty() || m_SettingNoted) m_LineEdit->SetCount(note);
    m_SettingNoted = !note.isEmpty();

    m_Rows = rows;
    m_SuggestStrings.clear();
    for(const SuggestRow &row : rows) m_SuggestStrings << row.title;
    m_CurrentSuggestIndex = selected;
    m_SuggestScroll = 0;

    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Receiver::TakeRow(SuggestRow row){
    switch(row.kind){
    case SuggestRow::RunCommand:
        hide();
        Remember(row.text);
        ReceiveCommand(row.text);
        break;
    case SuggestRow::FillField:
        m_LineEdit->setText(row.text);
        m_LineEdit->setFocus();
        break;
    case SuggestRow::RunAction:
        hide();
        RememberAction(row.text);
        DispatchAction(row.text);
        break;
    case SuggestRow::EditBinding:
        OpenSettingsWhenItMayChange(QStringLiteral("input?find=") +
                                    QString::fromLatin1(QUrl::toPercentEncoding(row.text.toLower())));
        break;
    case SuggestRow::OpenTab:{
        hide();
        const quint64 serial = row.serial;
        const QPointer<TreeBank> bank = m_TreeBank;
        TreeBank::WhenItMayChange([bank, serial](){
            ViewNode *target = VanillaPage::TabOfSerial(serial);
            if(!bank || !TreeBank::IsLive(bank) || !target) return;
            if(TreeBank::IsTrash(target)) TreeBank::RestoreBeside(target, bank->GetCurrentViewNode());
            bank->SetCurrent(target);
        });
        break;
    }
    case SuggestRow::SearchWeb:
        hide();
        emit OpenQueryUrl(row.text);
        break;
    case SuggestRow::OpenAddress:
        if(m_Anchor){
            const SharedView view = m_TreeBank->GetCurrentView();
            hide();
            if(view) view->Load(row.text);
            break;
        }
        hide();
        emit OpenUrl(Page::DirtyStringToUrls(row.text));
        break;
    case SuggestRow::TargetAction:
    case SuggestRow::TargetSearch:
    case SuggestRow::TargetCommand:
        RunTargetRow(row);
        break;
    }
}

namespace {
    QStringList OpenVariants(const QString &what){
        static const QStringList wheres = QStringList() << QStringLiteral("InNewViewNode")
            << QStringLiteral("InNewDirectory") << QStringLiteral("OnRoot");
        static const QStringList hows = QStringList() << QString() << QStringLiteral("Foreground")
            << QStringLiteral("Background") << QStringLiteral("ThisWindow") << QStringLiteral("NewWindow");
        QStringList names;
        for(const QString &where : wheres)
            for(const QString &how : hows)
                names << QStringLiteral("Open") + what + where + how;
        return names;
    }
    const QStringList &TargetLinkActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("LoadLink") << QStringLiteral("OpenLink")
            << QStringLiteral("DownloadLink") << QStringLiteral("CopyLinkUrl")
            << QStringLiteral("CopyLinkHtml") << QStringLiteral("OpenLinkWithDefault")
            << OpenVariants(QString());
        return names;
    }
    const QStringList &TargetImageActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("LoadImage") << QStringLiteral("OpenImage")
            << QStringLiteral("DownloadImage") << QStringLiteral("CopyImage")
            << QStringLiteral("CopyImageUrl") << QStringLiteral("CopyImageHtml")
            << QStringLiteral("OpenImageWithDefault") << OpenVariants(QStringLiteral("Image"));
        return names;
    }
    const QStringList &TargetMediaActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("ToggleMediaPlayPause") << QStringLiteral("ToggleMediaMute")
            << QStringLiteral("ToggleMediaLoop") << QStringLiteral("ToggleMediaControls")
            << QStringLiteral("LoadMedia") << QStringLiteral("OpenMedia")
            << QStringLiteral("DownloadMedia") << QStringLiteral("CopyMediaUrl")
            << QStringLiteral("CopyMediaHtml") << QStringLiteral("OpenMediaWithDefault")
            << OpenVariants(QStringLiteral("Media"));
        return names;
    }
    const QStringList &TargetElementActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("ClickElement") << QStringLiteral("FocusElement") << QStringLiteral("HoverElement");
        return names;
    }
    const QStringList &TargetFieldActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("Undo") << QStringLiteral("Redo") << QStringLiteral("Cut")
            << QStringLiteral("Paste") << QStringLiteral("SelectAll");
        return names;
    }
    const QStringList &TargetSelectionActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("Copy") << QStringLiteral("CopySelectedHtml")
            << QStringLiteral("OpenTextSeeker") << QStringLiteral("OpenAllUrl") << QStringLiteral("SaveAllUrl")
            << QStringLiteral("OpenAllImage") << QStringLiteral("SaveAllImage")
            << QStringLiteral("OpenTextAsUrl") << QStringLiteral("SaveTextAsUrl");
        return names;
    }
    const QStringList &TargetNodeActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("OpenNode") << QStringLiteral("OpenNodeOnNewWindow")
            << QStringLiteral("OpenNodeWithDefault") << QStringLiteral("RenameNode")
            << QStringLiteral("CopyNodeUrl") << QStringLiteral("CopyNodeTitle")
            << QStringLiteral("CopyNodeAsLink") << QStringLiteral("CloneNode")
            << QStringLiteral("ReloadNode") << QStringLiteral("CutNode")
            << QStringLiteral("CopyNode") << QStringLiteral("PasteNode") << QStringLiteral("NewNode")
            << QStringLiteral("MakeDirectory") << QStringLiteral("MakeDirectoryWithSelectedNode")
            << QStringLiteral("MakeDirectoryWithSameDomainNode") << QStringLiteral("DeleteNode")
            << QStringLiteral("DeleteRightNode") << QStringLiteral("DeleteLeftNode")
            << QStringLiteral("DeleteOtherNode");
        return names;
    }
    const QStringList &TargetTrashActions(){
        static const QStringList names = QStringList()
            << QStringLiteral("RestoreNode") << QStringLiteral("OpenNode")
            << QStringLiteral("OpenNodeWithDefault") << QStringLiteral("CopyNodeUrl")
            << QStringLiteral("CopyNodeTitle") << QStringLiteral("CopyNodeAsLink")
            << QStringLiteral("DeleteNode");
        return names;
    }

    QString TargetFileName(const QUrl &url, bool query){
        QStringList list = url.toString().split(QLatin1Char('/'));
        QString name;
        while(name.isEmpty() && !list.isEmpty()) name = list.takeLast();
        if(query){
            list = name.split(QLatin1Char('?'));
            name.clear();
            while(name.isEmpty() && !list.isEmpty()) name = list.takeFirst();
        }
        return name;
    }
}

bool Receiver::TargetHits(const QString &action){
    return action == QStringLiteral("CopyImage") || TargetElementActions().contains(action);
}

bool Receiver::IsSameTarget(const WebElement &held, const WebElement &found){
    if(found.IsNull()) return false;
    if(!held.XPath().isEmpty()) return found.XPath() == held.XPath();
    if(held.LinkUrl().isEmpty() && held.ImageUrl().isEmpty()) return false;
    return found.LinkUrl() == held.LinkUrl() && found.ImageUrl() == held.ImageUrl();
}

bool Receiver::IsSameField(const WebElement &held, const WebElement &found){
    if(found.IsNull()) return false;
    if(!held.XPath().isEmpty()) return found.XPath() == held.XPath();
    return found.IsFocusedField();
}

QString Receiver::TargetPartName(TargetPart part){
    switch(part){
    case TargetLink:      return tr("Link");
    case TargetImage:     return tr("Image");
    case TargetMedia:     return tr("Video and audio");
    case TargetSelection: return tr("Selection");
    case TargetElement:   return tr("Element");
    case TargetNode:      return tr("Tab");
    case TargetField:     return tr("Text field");
    }
    return QString();
}

QString Receiver::TargetTag(const TargetArgs &target){
    static const int length = 25;
    const auto cut = [](QString text){
        text = text.simplified();
        return text.length() > length ? text.left(length) + QStringLiteral("...") : text;
    };
    const QUrl link = target.element ? target.element->LinkUrl() : QUrl();
    const QUrl image = target.element ? target.element->ImageUrl() : QUrl();
    if(!image.isEmpty())
        return TargetPartName(target.playable ? TargetMedia : TargetImage) + QStringLiteral(" ") + cut(TargetFileName(image, true));
    if(!link.isEmpty()) return TargetPartName(TargetLink) + QStringLiteral(" ") + cut(TargetFileName(link, false));
    if(!target.selection.isEmpty())
        return TargetPartName(TargetSelection) + QStringLiteral(" 「") + cut(target.selection) + QStringLiteral("」");
    if(target.element && target.element->IsEditableElement())
        return TargetPartName(TargetField);
    if(target.element && !target.element->XPath().isEmpty())
        return TargetPartName(TargetElement) + QStringLiteral(" ") + target.element->TagName().toLower();
    return QString();
}

Receiver::SuggestRows Receiver::TargetRows(const TargetArgs &target, const TargetTables &tables,
                                           const QString &typed){
    const QString wanted = typed.trimmed();
    SuggestRows rows;
    const auto add = [&](TargetPart part, SuggestRow::Kind kind, const QString &name,
                         const QString &title, const QString &key){
        if(!wanted.isEmpty() && !title.contains(wanted, Qt::CaseInsensitive) &&
           !name.contains(wanted, Qt::CaseInsensitive)) return;
        for(const SuggestRow &row : std::as_const(rows))
            if(row.kind == kind && row.serial == quint64(part) && row.text == name) return;
        SuggestRow row{ SuggestRow::Commands, kind, title, QString(), name, quint64(part),
                        QIcon(), TargetPartName(part), key, QStringList() };
        rows << row;
    };
    const auto action = [&](TargetPart part, const QString &name){
        add(part, SuggestRow::TargetAction, name, ActionText::MenuLabel(name), QString());
    };
    const auto commands = [&](TargetPart part, const QUrl &url){
        for(const QString &command : tables.commands)
            add(part, SuggestRow::TargetCommand, command, command, url.toString());
    };
    const auto engines = [&](){
        for(const QString &engine : tables.engines)
            add(TargetSelection, SuggestRow::TargetSearch, engine,
                CommandLabel(engine).isEmpty() ? engine : CommandLabel(engine), QString());
    };
    const auto part = [&](TargetPart part, const QString &table, const QString &other,
                          const QStringList &allowed, const QString &withDefault, const QUrl &url){
        for(const QString &name : table.split(QLatin1Char(','), Qt::SkipEmptyParts)){
            if(name == other){
                action(part, withDefault);
                commands(part, url);
            } else if(allowed.contains(name)){
                action(part, name);
            }
        }
        for(const QString &name : allowed) action(part, name);
        commands(part, url);
    };

    if(!target.nodes.isEmpty()){
        const QStringList &allowed = target.trash ? TargetTrashActions() : TargetNodeActions();
        const auto node = [&](const QString &name){
            if(allowed.contains(name))
                add(TargetNode, SuggestRow::TargetAction, name, MenuSchema::Label(name), QString());
        };
        const auto nodeCommands = [&](){
            for(const QString &command : tables.commands)
                add(TargetNode, SuggestRow::TargetCommand, command, command, QString());
        };
        for(const QString &name : tables.node.split(QLatin1Char(','), Qt::SkipEmptyParts)){
            if(name == QStringLiteral("OpenNodeWithOtherBrowser")){
                node(QStringLiteral("OpenNodeWithDefault"));
                nodeCommands();
            } else if(name == QStringLiteral("DeleteNodesTogether")){
                node(QStringLiteral("DeleteRightNode"));
                node(QStringLiteral("DeleteLeftNode"));
                node(QStringLiteral("DeleteOtherNode"));
            } else {
                node(name);
            }
        }
        for(const QString &name : allowed) node(name);
        nodeCommands();
        return rows;
    }

    const QUrl link = target.element ? target.element->LinkUrl() : QUrl();
    const QUrl image = target.element ? target.element->ImageUrl() : QUrl();
    if(!link.isEmpty())
        part(TargetLink, tables.link, QStringLiteral("OpenLinkWithOtherBrowser"),
             TargetLinkActions(), QStringLiteral("OpenLinkWithDefault"), link);
    if(!image.isEmpty() && !target.playable)
        part(TargetImage, tables.image, QStringLiteral("OpenImageWithOtherBrowser"),
             TargetImageActions(), QStringLiteral("OpenImageWithDefault"), image);
    if(!image.isEmpty() && target.playable)
        part(TargetMedia, tables.media, QStringLiteral("OpenMediaWithOtherBrowser"),
             TargetMediaActions(), QStringLiteral("OpenMediaWithDefault"), image);
    if(target.element && target.element->IsEditableElement()){
        for(const QString &name : tables.field.split(QLatin1Char(','), Qt::SkipEmptyParts))
            if(TargetFieldActions().contains(name)) action(TargetField, name);
        for(const QString &name : TargetFieldActions()) action(TargetField, name);
    }
    if(!target.selection.isEmpty()){
        for(const QString &name : tables.selection.split(QLatin1Char(','), Qt::SkipEmptyParts)){
            if(name == QStringLiteral("SearchMenu")) engines();
            else if(TargetSelectionActions().contains(name)) action(TargetSelection, name);
        }
        for(const QString &name : TargetSelectionActions()) action(TargetSelection, name);
        engines();
    }
    if(target.element && (!target.element->XPath().isEmpty() || !link.isEmpty() || !image.isEmpty())){
        for(const QString &name : TargetElementActions()) action(TargetElement, name);
        action(TargetElement, QStringLiteral("InspectElement"));
    }
    return rows;
}

bool Receiver::IsLiveTarget(const View *held, const View *nodeView, const View *current,
                            quint64 heldLoad, quint64 loadNow){
    return held && nodeView == held && current == held && loadNow == heldLoad;
}

SharedView Receiver::LiveTarget(const TargetArgs &target, TreeBank *bank){
    const SharedView view = target.view.lock();
    if(!view || !bank || !TreeBank::IsLive(bank)) return SharedView();
    ViewNode *node = TreeBank::TabOfSerial(target.node);
    return IsLiveTarget(view.get(), node ? node->GetView() : nullptr, bank->GetCurrentView().get(),
                        target.loadSerial, view->LoadSerial())
        ? view : SharedView();
}

void Receiver::RunTargetRow(const SuggestRow &row){
    const TargetArgs target = m_Target;
    const QPointer<TreeBank> bank = m_TreeBank;
    hide();
    const quint64 request = m_TargetRequest;
    if(!target.nodes.isEmpty()){
        RunNodeRow(row, target, request);
        return;
    }
    const SharedView view = LiveTarget(target, bank);
    if(!view) return;

    switch(row.kind){
    case SuggestRow::TargetCommand:
        Application::RunExternalCommand(row.text, QUrl(row.key));
        return;
    case SuggestRow::TargetSearch:
        if(QObject *page = view->page())
            QMetaObject::invokeMethod(page, "OpenInNew", Q_ARG(QString, row.text), Q_ARG(QString, target.selection));
        return;
    case SuggestRow::TargetAction:
        break;
    default:
        return;
    }

    if(row.serial == quint64(TargetField) && target.element){
        const QPointer<Receiver> self(this);
        const QString name = row.text;
        view->CallWithHitElement(target.hitPos, [self, bank, target, request, name](SharedWebElement found){
            if(!self || self->m_TargetRequest != request) return;
            const SharedView again = LiveTarget(target, bank);
            if(!again || !found || !IsSameField(*target.element, *found)) return;
            again->setFocus();
            if(!target.element->XPath().isEmpty()) found->SetFocus();
            again->TriggerAction(name);
        });
        return;
    }
    if(row.serial == quint64(TargetSelection)){
        Page *page = view->base() ? view->base()->findChild<Page*>() : nullptr;
        const auto still = [target, bank](){ return LiveTarget(target, bank) != nullptr;};
        const QString &text = target.selection;
        const QString &html = target.selectionHtml;
        if(row.text == QStringLiteral("Copy")){
            QApplication::clipboard()->setText(text);
        } else if(row.text == QStringLiteral("CopySelectedHtml")){
            if(!html.isEmpty()) QApplication::clipboard()->setText(html);
        } else if(row.text == QStringLiteral("OpenTextSeeker")){
            OpenTextSeeker(nullptr, text);
        } else if(!page){
            return;
        } else if(row.text == QStringLiteral("OpenTextAsUrl")){
            page->OpenHeldUrls(Page::AddressesOfText, text, target.base, still);
        } else if(row.text == QStringLiteral("SaveTextAsUrl")){
            page->SaveHeldUrls(Page::AddressesOfText, text, target.base);
        } else if(html.isEmpty()){
            return;
        } else if(row.text == QStringLiteral("OpenAllUrl")){
            page->OpenHeldUrls(Page::LinksOfHtml, html, target.base, still);
        } else if(row.text == QStringLiteral("OpenAllImage")){
            page->OpenHeldUrls(Page::ImagesOfHtml, html, target.base, still);
        } else if(row.text == QStringLiteral("SaveAllUrl")){
            page->SaveHeldUrls(Page::LinksOfHtml, html, target.base);
        } else if(row.text == QStringLiteral("SaveAllImage")){
            page->SaveHeldUrls(Page::ImagesOfHtml, html, target.base);
        }
        return;
    }
    if(!target.element) return;

    if(TargetHits(row.text)){
        const QPointer<Receiver> self(this);
        const QString name = row.text;
        view->CallWithHitElement(target.hitPos, [self, bank, target, request, name](SharedWebElement found){
            if(!self || self->m_TargetRequest != request) return;
            const SharedView again = LiveTarget(target, bank);
            if(!again || !found || !IsSameTarget(*target.element, *found)) return;
            if(name == QStringLiteral("FocusElement")){
                again->setFocus();
                found->SetFocus();
                return;
            }
            again->TriggerAction(name, QVariant::fromValue(found));
        });
        return;
    }
    view->TriggerAction(row.text, QVariant::fromValue(target.element));
}

void Receiver::RunNodeRow(const SuggestRow &row, const TargetArgs &target, quint64 request){
    const QPointer<Receiver> self(this);
    const QPointer<TreeBank> bank = m_TreeBank;
    const QList<quint64> serials = target.nodes;
    const bool trash = target.trash;
    const QString name = row.text;
    const bool command = row.kind == SuggestRow::TargetCommand;
    TreeBank::WhenItMayChange([self, request, bank, serials, trash, name, command](){
        if(!self || self->m_TargetRequest != request) return;
        if(!bank || !TreeBank::IsLive(bank)) return;
        QList<ViewNode*> nodes;
        for(const quint64 serial : serials){
            ViewNode *vn = HeldNode(serial);
            if(vn && TreeBank::IsTrash(vn) == trash) nodes << vn;
        }
        if(nodes.isEmpty()) return;
        ViewNode *first = nodes.first();
        NodeList all;
        QList<quint64> held;
        for(ViewNode *vn : std::as_const(nodes)){ all << vn; held << vn->GetSerial();}
        QList<ViewNode*> pages;
        for(ViewNode *vn : std::as_const(nodes)) if(!vn->IsDirectory()) pages << vn;

        if(command){
            for(ViewNode *vn : std::as_const(pages)) Application::RunExternalCommand(name, vn->GetUrl());
        } else if(name == QStringLiteral("OpenNode")){
            if(pages.isEmpty()) return;
            if(trash) TreeBank::RestoreBeside(pages.first(), bank->GetCurrentViewNode());
            bank->SetCurrent(pages.first());
        } else if(name == QStringLiteral("OpenNodeOnNewWindow")){
            if(pages.isEmpty()) return;
            bank->NewWindow()->GetTreeBank()->SetCurrent(pages.first());
        } else if(name == QStringLiteral("OpenNodeWithDefault")){
            for(ViewNode *vn : std::as_const(pages)) Application::OpenUrlWithDefaultBrowser(vn->GetUrl());
        } else if(name == QStringLiteral("CopyNodeUrl")){
            QStringList urls;
            for(ViewNode *vn : std::as_const(pages)) urls << vn->GetUrl().toString();
            if(urls.isEmpty()) return;
            QApplication::clipboard()->setText(urls.join(QLatin1Char('\n')));
        } else if(name == QStringLiteral("CopyNodeTitle")){
            QStringList titles;
            for(ViewNode *vn : std::as_const(nodes)) titles << vn->GetTitle();
            QApplication::clipboard()->setText(titles.join(QLatin1Char('\n')));
        } else if(name == QStringLiteral("CopyNodeAsLink")){
            QString html;
            for(ViewNode *vn : std::as_const(pages))
                html += QStringLiteral("<a href=\"%1\">%2</a>\n")
                    .arg(QString::fromUtf8(vn->GetUrl().toEncoded()), vn->GetTitle());
            if(html.isEmpty()) return;
            QApplication::clipboard()->setText(html);
        } else if(name == QStringLiteral("CutNode") || name == QStringLiteral("CopyNode")){
            TreeBank::SetClipboardNodes(all, name == QStringLiteral("CutNode"));
        } else if(name == QStringLiteral("PasteNode")){
            TreeBank::PasteClipboardNodes(first->GetParent(), first);
        } else if(name == QStringLiteral("RenameNode")){
            bank->RenameNode(first);
        } else if(name == QStringLiteral("CloneNode")){
            for(ViewNode *vn : std::as_const(nodes)) bank->CloneViewNode(vn);
        } else if(name == QStringLiteral("ReloadNode")){
            for(ViewNode *vn : std::as_const(pages))
                if(View *view = vn->GetView()) view->TriggerAction(Page::_Reload);
        } else if(name == QStringLiteral("NewNode")){
            bank->NewViewNode(first);
        } else if(name == QStringLiteral("MakeDirectory")){
            bank->MakeSiblingDirectory(first);
        } else if(name == QStringLiteral("MakeDirectoryWithSelectedNode")){
            Node *parent = bank->MakeSiblingDirectory(first);
            TreeBank::SetChildrenOrder(parent, all);
        } else if(name == QStringLiteral("MakeDirectoryWithSameDomainNode")){
            ViewNode *parent = first->GetParent() ? first->GetParent()->ToViewNode() : nullptr;
            if(!parent) return;
            QMap<QString, NodeList> groups;
            for(Node *nd : first->GetSiblings())
                if(!nd->IsDirectory()) groups[nd->GetUrl().host()] << nd;
            if(groups.count() <= 1) return;
            for(const QString &domain : groups.keys()){
                Node *directory = parent->MakeChild();
                directory->SetTitle(domain);
                TreeBank::SetChildrenOrder(directory, groups[domain]);
            }
        } else if(name == QStringLiteral("DeleteNode")){
            if(trash) TreeBank::DiscardFromTrashAsked(held, held.length(), TargetPartName(TargetNode));
            else bank->DeleteNode(all);
        } else if(name == QStringLiteral("DeleteRightNode") || name == QStringLiteral("DeleteLeftNode") ||
                  name == QStringLiteral("DeleteOtherNode")){
            const NodeList siblings = first->GetSiblings();
            const int index = siblings.indexOf(first);
            NodeList list;
            for(int i = 0; i < siblings.length(); i++){
                if(name == QStringLiteral("DeleteRightNode") ? i > index
                   : name == QStringLiteral("DeleteLeftNode") ? i < index : i != index)
                    list << siblings[i];
            }
            bank->DeleteNode(list);
        } else if(name == QStringLiteral("RestoreNode")){
            for(ViewNode *vn : std::as_const(nodes)) TreeBank::RestoreBeside(vn, bank->GetCurrentViewNode());
        }
    });
}

QVector<Receiver::RowAction> Receiver::RowActions(const SuggestRow &row){
    QVector<RowAction> actions{ RowAction::Take };
    switch(row.source){
    case SuggestRow::Tabs:
        actions << RowAction::CloseTab << RowAction::CopyAddress << RowAction::OpenCard;
        break;
    case SuggestRow::Trash:
        actions << RowAction::DiscardTab << RowAction::CopyAddress << RowAction::OpenCard;
        break;
    case SuggestRow::Commands:{
        const QString name = row.text.trimmed();
        if(InputMapSchema::IsAction(InputMapSchema::PageActions, name) ||
           InputMapSchema::IsAction(InputMapSchema::TreeBankActions, name) ||
           InputMapSchema::IsAction(InputMapSchema::GadgetsActions, name))
            actions << RowAction::ChangeAssignment;
        break;
    }
    case SuggestRow::Settings:
        if(row.kind == SuggestRow::FillField){
            if(SettingsSchema::Find(row.key))
                actions << RowAction::OpenInSettings;
            actions << RowAction::ResetSetting;
        }
        break;
    case SuggestRow::Web:
        if(row.kind != SuggestRow::FillField) actions << RowAction::CopyWords;
        break;
    default:
        break;
    }
    return actions;
}

QString Receiver::RowActionLabel(const SuggestRow &row, RowAction action){
    switch(action){
    case RowAction::Take:
        switch(row.source){
        case SuggestRow::Tabs:     return tr("Switch to the tab");
        case SuggestRow::Trash:    return tr("Restore the tab");
        case SuggestRow::Commands: return tr("Run");
        case SuggestRow::Settings:
            return row.kind == SuggestRow::EditBinding ? tr("Change the assignment") : tr("Edit the value");
        case SuggestRow::Web:
            return row.kind == SuggestRow::OpenAddress ? tr("Open addresses") : tr("Search the web");
        default: return QString();
        }
    case RowAction::CloseTab:         return tr("Close the tab");
    case RowAction::DiscardTab:       return tr("Delete for good");
    case RowAction::CopyAddress:      return tr("Copy the address");
    case RowAction::CopyWords:        return tr("Copy the words");
    case RowAction::ChangeAssignment: return tr("Change the assignment");
    case RowAction::OpenInSettings:   return tr("Open in the settings");
    case RowAction::ResetSetting:     return tr("Reset a setting");
    case RowAction::OpenCard:         return ActionText::MenuLabel(QStringLiteral("OpenCommand"));
    }
    return QString();
}

void Receiver::RunRowAction(const SuggestRow &row, RowAction action){
    switch(action){
    case RowAction::Take:
        TakeRow(row);
        return;
    case RowAction::CloseTab:{
        const QPointer<Receiver> self(this);
        const QPointer<TreeBank> bank = m_TreeBank;
        const quint64 serial = row.serial;
        TreeBank::WhenItMayChange([self, bank, serial](){
            ViewNode *target = VanillaPage::TabOfSerial(serial);
            if(!bank || !TreeBank::IsLive(bank) || !target) return;
            if(TreeBank::WinIndex(target) && bank->GetCurrentViewNode() != target) return;
            bank->DeleteNode(target);
            if(!self || !self->isVisible() || self->m_Mode != Command) return;
            self->RefillKeepingChoice();
            if(self->m_CurrentSuggestIndex < 0 && !self->m_Rows.isEmpty()){
                self->m_CurrentSuggestIndex = 0;
                self->KeepSelectionShown();
                self->repaint();
            }
        });
        return;
    }
    case RowAction::DiscardTab:
        hide();
        TreeBank::DiscardFromTrashAsked(QList<quint64>() << row.serial, 1, SourceName(SuggestRow::Trash));
        return;
    case RowAction::CopyAddress:
        hide();
        if(ViewNode *target = VanillaPage::TabOfSerial(row.serial))
            QApplication::clipboard()->setText(target->GetUrl().toString());
        return;
    case RowAction::CopyWords:
        hide();
        QApplication::clipboard()->setText(row.text);
        return;
    case RowAction::ChangeAssignment:
        OpenSettingsWhenItMayChange(QStringLiteral("input?find=") +
                                    QString::fromLatin1(QUrl::toPercentEncoding(row.text.trimmed().toLower())));
        return;
    case RowAction::OpenInSettings:
        if(const SettingsSchema::Item *item = SettingsSchema::Find(row.key))
            OpenSettingsWhenItMayChange(QString::fromLatin1(item->category) + QStringLiteral("?find=") +
                                        QString::fromLatin1(QUrl::toPercentEncoding(row.key)));
        return;
    case RowAction::OpenCard:{
        TargetArgs target;
        target.nodes << row.serial;
        target.trash = row.source == SuggestRow::Trash;
        target.nodeTable = QStringLiteral("TableNodeMenu");
        OpenTarget(target);
        return;
    }
    case RowAction::ResetSetting:
        hide();
        if(row.key.isEmpty()) return;
        Application::GlobalSettings().remove(row.key);
        m_TreeBank->Reconfigure();
        return;
    }
}

void Receiver::OpenSettingsWhenItMayChange(const QString &fragment){
    hide();
    const QPointer<TreeBank> bank = m_TreeBank;
    TreeBank::WhenItMayChange([bank, fragment](){
        if(bank && TreeBank::IsLive(bank)) bank->OpenSettings(fragment);
    });
}

void Receiver::OpenRowActions(int index, const QPoint &globalPos){
    if(index < 0 || index >= m_Rows.length()) return;
    m_CurrentSuggestIndex = index;
    KeepSelectionShown();
    repaint();
    const SuggestRow row = m_Rows[index];
    QMenu menu;
    const QVector<RowAction> actions = RowActions(row);
    for(int i = 0; i < actions.length(); i++){
        QAction *action = menu.addAction(RowActionLabel(row, actions[i]));
        action->setData(i);
    }
    menu.setDefaultAction(menu.actions().first());
    menu.setActiveAction(menu.actions().first());
    const QPointer<Receiver> self(this);
    QAction *chosen = menu.exec(globalPos);
    if(!self) return;
    QTimer::singleShot(0, this, &Receiver::CloseIfFocusLeft);
    if(chosen) RunRowAction(row, actions[chosen->data().toInt()]);
}

bool Receiver::HandleTabKey(){
    if(m_Mode != Command || m_Rows.isEmpty()) return false;
    const int index = m_CurrentSuggestIndex >= 0 ? m_CurrentSuggestIndex : m_SuggestScroll;
    if(index < m_Rows.length() && VisibleSuggestCount())
        OpenRowActions(index, mapToGlobal(SuggestRect(index).bottomLeft()));
    return true;
}

void Receiver::contextMenuEvent(QContextMenuEvent *ev){
    ev->accept();
    if(View::TakeRightButtonConsumed()) return;
    if(m_Mode != Command || m_Rows.isEmpty()) return;
    const int index = SuggestAt(ev->pos());
    if(index >= 0) OpenRowActions(index, ev->globalPos());
}

bool Receiver::HandleMappedKey(QKeyEvent *ev){
    const QKeySequence seq = InputMap::DisplayKey(Application::MakeKeySequence(ev));
    if(seq.isEmpty()) return false;
    const int count = SuggestRow::SourceCount + 1;
    for(const InputMapSchema::Binding &binding : m_Bindings){
        if(binding.vocabulary != InputMapSchema::ReceiverActions) continue;
        const QKeySequence bound(binding.input, QKeySequence::PortableText);
        if(InputMapSchema::TypesIntoTheField(bound) || InputMap::DisplayKey(bound) != seq) continue;
        const QString &action = binding.action;
        if(action == QStringLiteral("NextCommandLineTab") || action == QStringLiteral("PrevCommandLineTab")){
            if(!ShowsSourceBar()) return false;
            const int step = action == QStringLiteral("NextCommandLineTab") ? 1 : count - 1;
            SelectTab((m_ActiveTab + step) % count);
            return true;
        }
        if(action == QStringLiteral("CommandLineRowActions")) return HandleTabKey();
        if(action == QStringLiteral("NextCommandLineRow")){ SelectNextSuggest(); return true;}
        if(action == QStringLiteral("PrevCommandLineRow")){ SelectPrevSuggest(); return true;}
        if(action == QStringLiteral("NextCommandLinePage") || action == QStringLiteral("PrevCommandLinePage")){
            if(!ListScrolls() || m_SuggestStrings.isEmpty()) return false;
            const int rows = SelectableSuggestCount();
            const int step = qMax(VisibleSuggestCount() - 1, 1) *
                (action == QStringLiteral("NextCommandLinePage") ? 1 : -1);
            if(rows){
                m_CurrentSuggestIndex = qBound(0, m_CurrentSuggestIndex + step, rows - 1);
                KeepSelectionShown();
                repaint();
            }
            return true;
        }
        if(action == QStringLiteral("CloseCommandLine")){ OnAborted(); return true;}
    }
    return false;
}

int Receiver::HintWidth(int row, int title, int hint, int gap){
    return qMin(hint, qMax(row / 2, row - title - gap));
}

int Receiver::InputsWidth(const QList<int> &widths, int shown, int gap, int more){
    int width = 0;
    for(int i = 0; i < shown; i++) width += (i ? gap : 0) + widths[i];
    if(shown < widths.length()) width += (shown ? gap : 0) + more;
    return width;
}

QString Receiver::CommandLabel(const QString &name){
    if(name == QStringLiteral("open"))     return tr("Open addresses");
    if(name == QStringLiteral("load"))     return tr("Load an address here");
    if(name == QStringLiteral("query"))    return tr("Search the web");
    if(name == QStringLiteral("download")) return tr("Download to a file");
    if(name == QStringLiteral("search"))   return tr("Search in the page");
    if(name == QStringLiteral("set"))      return tr("Change a setting");
    if(name == QStringLiteral("unset"))    return tr("Reset a setting");
    if(name == QStringLiteral("key"))      return tr("Send a key");
    if(name == QStringLiteral("blank"))    return tr("Open a blank page");
    if(name == QStringLiteral("Reconfigure")) return tr("Read the settings again");

    const QString label = ActionText::Label(name);
    if(label != ActionText::Source(name)) return label;

    const QMap<QString, SearchEngine> engines = Page::GetSearchEngineMap();
    const auto engine = engines.constFind(name);
    if(engine != engines.cend() && !engine.value().isEmpty()){
        const QString host = QUrl::fromEncoded(engine.value().first().toLatin1()).host();
        return tr("Search %1").arg(host.startsWith(QStringLiteral("www.")) ? host.mid(4) : host);
    }
    if(Page::GetBookmarkletMap().contains(name)) return tr("Run the bookmarklet");
    return QString();
}

void Receiver::ReadBindings(){
    m_Bindings = InputMapSchema::Bindings(Application::GlobalSettings(), View::EnableSingleKeyShortcut());
    const bool overview = m_TreeBank->IsDisplayingTableView();
    std::stable_partition(m_Bindings.begin(), m_Bindings.end(), [overview](const InputMapSchema::Binding &binding){
        return (binding.vocabulary == InputMapSchema::GadgetsActions) == overview;
    });
}

QStringList Receiver::InputsOf(const QString &name) const {
    const QString action = name.trimmed();
    QStringList inputs;
    for(const InputMapSchema::Binding &binding : m_Bindings){
        if(binding.action == action && !inputs.contains(binding.shown)) inputs << binding.shown;
    }
    return inputs;
}

void Receiver::SetSuggest(QStringList list){
    m_Rows.clear();
    m_SuggestScroll = 0;
    m_CurrentSuggestIndex = -1;
    m_SuggestStrings.clear();
    m_SuggestStrings = list;
    ResizeNotify(m_TreeBank->size());
    repaint();
}

namespace {

    QString WithCurrentValue(const QString &key){
        QStringList list = key.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
        if(list.size() < 2 ||
           !Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et"), list[0]))
            return QString();
        const QString value = Application::GlobalSettings().value(list[1]).toString();
        if(!value.isEmpty()) list << value;
        return list.join(QStringLiteral(" "));
    }
}

void Receiver::SuitableAction(){
    switch (m_Mode){
    case Query:{
        const QString words = m_CurrentSuggestIndex != -1
            ? m_SuggestStrings[m_CurrentSuggestIndex] : m_LineString;
        hide();
        emit OpenQueryUrl(words);
        break;
    }
    case UrlEdit:
        hide();
        emit OpenUrl(Page::DirtyStringToUrls(m_LineString == QStringLiteral("blank")
                                             ? QStringLiteral("about:blank")
                                             : m_LineString));
        break;
    case Search:
        SeekFromField(m_LineString);
        break;
    case Target:
        if(!m_Rows.isEmpty() && m_CurrentSuggestIndex != -1)
            TakeRow(m_Rows[m_CurrentSuggestIndex]);
        break;
    case Command:{
        bool unset = false;
        if(QString words; TabSeeks() && SeekWords(m_LineString, &words)){
            Remember(m_LineString);
            SeekFromField(words);
        } else if(m_Anchor && m_ActiveTab == SuggestRow::Web + 1 && m_CurrentSuggestIndex == -1){
            SubmitAddress(m_LineString);
        } else if(!m_Rows.isEmpty() && m_CurrentSuggestIndex != -1){
            TakeRow(m_Rows[m_CurrentSuggestIndex]);
        } else if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            const QString text = WithCurrentValue(m_SuggestStrings[m_CurrentSuggestIndex]);
            if(!text.isEmpty()) m_LineEdit->setText(text);
        } else if(SuggestRow::Source only; m_ActiveTab == 0 && SourceWord(m_LineString, &only, &words, &unset) &&
                  only == SuggestRow::Settings && IsAssignment(words, unset)){
            const QString line = (unset ? QStringLiteral("unset ") : QStringLiteral("set ")) + words.trimmed();
            if(!SettingNote(line).isEmpty()) break;
            hide();
            Remember(line);
            ReceiveCommand(line);
        } else if(m_ActiveTab != 0 && !MeansSomething(m_LineString.split(LineSpace()).first())){
        } else if(!SettingNote(m_LineString).isEmpty()){
        } else {
            hide();
            Remember(m_LineString);
            ReceiveCommand(m_LineString);
        }
        break;
    }
    }
}

void Receiver::SeekFromField(const QString &words){
    if(m_TreeBank->IsDisplayingTableView()){
        emit SeekText(words, View::WrapsAroundDocument);
        hide();
    } else if(Application::keyboardModifiers() & Qt::ShiftModifier){
        emit SeekText(words, View::WrapsAroundDocument | View::FindBackward);
    } else {
        emit SeekText(words, View::WrapsAroundDocument);
    }
}

void Receiver::ReceiveCommand(QString cmd){
    QStringList list = cmd.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
    cmd = list.takeFirst();

    if(cmd.isEmpty()) return;

    QString action;
    const CommandMap::Kind kind = CommandMap::Resolve(cmd, &action);

    switch(kind){
    case CommandMap::Signal:
        DispatchAction(action);
        return;
    case CommandMap::Element:
        emit TriggerElementAction(Page::StringToAction(action));
        return;
    case CommandMap::OpenWith:
        if(!list.isEmpty()) m_TreeBank->OpenWithCommand(list.join(QStringLiteral(" ")));
        return;
    case CommandMap::OpenNodeWith:
        if(!list.isEmpty()) emit OpenNodeWithCommand(list.join(QStringLiteral(" ")));
        return;
    case CommandMap::Blank:
        emit OpenUrl(BLANK_URL);
        return;
    default:
        break;
    }

    if(list.isEmpty() && Page::GetBookmarkletMap().contains(cmd)){
        emit OpenBookmarklet(Page::GetBookmarklet(cmd).first());
        return;
    }

    switch(kind){
    case CommandMap::Open:{
        QStringList args;
        foreach(QString arg, list){
            if(arg == QStringLiteral("blank"))
                args << QStringLiteral("about:blank");
            else args << arg;
        }
        emit OpenUrl(Page::DirtyStringToUrls(args.join(QStringLiteral(" "))));
        return;
    }
    case CommandMap::Load:{
        QString args = list.join(QStringLiteral(" "));
        if(args == QStringLiteral("blank"))
            emit OpenUrl(BLANK_URL);
        else emit OpenUrl(QUrl(args));
        return;
    }
    case CommandMap::Query:
        emit OpenQueryUrl(list.join(QStringLiteral(" ")));
        return;
    case CommandMap::Download:
        if(list.length() >= 2)
            emit Download(list[0], list[1]);
        return;
    case CommandMap::Seek:
        if(Application::keyboardModifiers() & Qt::ShiftModifier){
            emit SeekText(list.join(QStringLiteral(" ")), View::WrapsAroundDocument | View::FindBackward);
        } else {
            emit SeekText(list.join(QStringLiteral(" ")), View::WrapsAroundDocument);
        }
        return;
    case CommandMap::Set:
        if(list.length() >= 2){
            const QString key = list.takeFirst();
            const QString text = list.join(QStringLiteral(" "));
            const QVariant value = SettingsSchema::FromText(key, text);
            if(!value.isValid()){
                ModelessDialog::Information(tr("Cannot change this setting."), SettingRefusal(key, text));
                return;
            }
            Application::GlobalSettings().setValue(key, value);
            m_TreeBank->Reconfigure();
        }
        return;
    case CommandMap::Unset:
        if(list.length() >= 1){
            Application::GlobalSettings().remove(list[0]);
            m_TreeBank->Reconfigure();
        }
        return;
    case CommandMap::Key:
        emit KeyEvent(list.join(QStringLiteral(" ")));
        return;
    default:
        break;
    }

    if(Page::GetSearchEngineMap().contains(cmd)){
        emit SearchWith(cmd, list.join(QStringLiteral(" ")));
        return;
    }

    QStringList args;
    list.prepend(cmd);
    foreach(QString arg, list){
        if(arg == QStringLiteral("blank"))
            args << QStringLiteral("about:blank");
        else args << arg;
    }
    emit OpenUrl(Page::DirtyStringToUrls(args.join(QStringLiteral(" "))));
}

void Receiver::DispatchAction(const QString &action){
    if(action == QStringLiteral("Reconfigure")){
        m_TreeBank->Reconfigure();
        return;
    }

    Gadgets *gadgets = m_TreeBank->GetGadgets();
    if(gadgets && gadgets->IsActive()){
        if(static_cast<View*>(gadgets)->TriggerAction(action)) return;
    } else if(SharedView view = m_TreeBank->GetCurrentView()){
        if(view->TriggerAction(action)) return;
    }

    if(action == QStringLiteral("MakeLocalNode")){
        m_TreeBank->MakeLocalNode();
        return;
    }

    m_TreeBank->TriggerAction(action);
}

void Receiver::SelectNextSuggest(){
    const int count = SelectableSuggestCount();
    if(!count) return;
    m_CurrentSuggestIndex = m_CurrentSuggestIndex + 1 < count ? m_CurrentSuggestIndex + 1 : 0;
    KeepSelectionShown();
    repaint();
}

void Receiver::SelectPrevSuggest(){
    const int count = SelectableSuggestCount();
    if(!count) return;
    m_CurrentSuggestIndex = m_CurrentSuggestIndex > 0 ? m_CurrentSuggestIndex - 1 : count - 1;
    KeepSelectionShown();
    repaint();
}

void Receiver::focusInEvent(QFocusEvent *ev){
    QWidget::focusInEvent(ev);
}

void Receiver::focusOutEvent(QFocusEvent *ev){
    QWidget::focusOutEvent(ev);
}

void Receiver::timerEvent(QTimerEvent *ev){
    Q_UNUSED(ev)
}

void Receiver::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setFont(ReceiverFont());

    {
        const int radius = ScaleByDevice(DIALOG_CORNER_RADIUS);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Theme::Pen(Theme::VanillaPageBorder));
        painter.setBrush(Theme::Brush(IsPurged() ? BlurBehind::Surface() : Theme::ReceiverBackground));
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.drawRoundedRect(card, radius, radius);
    }

    const int count = VisibleSuggestCount();
    const int radius = ScaleByDevice(PAGE_FIELD_CORNER_RADIUS);
    const int padX = ScaleByDevice(RECEIVER_FIELD_PADDING_X);

    if(ShowsSourceBar()){
        const int active = m_ActiveTab;
        for(int i = 0; i <= SuggestRow::SourceCount; i++){
            const QRect chip = SourceBarRect(i);
            if(i == active){
                painter.setPen(Qt::NoPen);
                painter.setBrush(Theme::Brush(Theme::ReceiverBackgroundContrast));
                painter.drawRoundedRect(chip, radius, radius);
            }
            painter.setPen(Theme::Pen(i == active ? Theme::ReceiverTextContrast : Theme::VanillaPageTextDim));
            painter.setBrush(Qt::NoBrush);
            painter.drawText(chip, Qt::AlignCenter,
                             i == 0 ? tr("All") : SourceName(static_cast<SuggestRow::Source>(i - 1)));
        }
    }
    if(count && count < m_SuggestStrings.length()){
        const QRect first = SuggestRect(m_SuggestScroll);
        const int track = SuggestRect(m_SuggestScroll + count - 1).bottom() - first.top();
        const int total = m_SuggestStrings.length();
        const int mark = qMax(track * count / total, ScaleByDevice(RECEIVER_FIELD_HEIGHT) / 2);
        const int width = ScaleByDevice(RECEIVER_SCROLL_MARK_WIDTH);
        const int y = first.top() + (track - mark) * m_SuggestScroll / qMax(total - count, 1);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::VanillaPageTextDim));
        painter.drawRoundedRect(QRect(this->width() - ScaleByDevice(RECEIVER_PADDING) / 2 - width / 2, y, width, mark),
                                width / 2, width / 2);
    }

    for(int i = m_SuggestScroll; i < m_SuggestScroll + count; i++){
        const QRect row = SuggestRect(i);
        const bool selected = i == m_CurrentSuggestIndex;
        if(selected){
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::Brush(Theme::ReceiverBackgroundContrast));
            painter.drawRoundedRect(row, radius, radius);
        }
        QRect text = row.adjusted(padX, 0, -padX, 0);
        painter.setBrush(Qt::NoBrush);
        if(HasHeading(i)){
            const QRect heading(text.left() - ScaleByDevice(RECEIVER_HEADING_OUTDENT),
                                row.top() - ScaleByDevice(RECEIVER_HEADING_HEIGHT),
                                text.width(), ScaleByDevice(RECEIVER_HEADING_HEIGHT));
            painter.setPen(Theme::Pen(Theme::VanillaPageTextDim));
            painter.drawText(heading, Qt::AlignLeft | Qt::AlignBottom,
                             m_Rows[i].section.isEmpty() ? SourceName(m_Rows[i].source) : m_Rows[i].section);
        }
        if(!m_Rows.isEmpty() && (!m_Rows[i].hint.isEmpty() || !m_Rows[i].inputs.isEmpty())){
            const QFontMetrics metrics = painter.fontMetrics();
            const QString hint = m_Rows[i].hint;
            const QStringList inputs = m_Rows[i].inputs;
            const int chipPad = ScaleByDevice(6);
            const int chipGap = ScaleByDevice(4);
            QList<int> chipWidths;
            for(const QString &input : inputs) chipWidths << metrics.horizontalAdvance(input) + 2 * chipPad;
            const auto more = [&](int count){
                return metrics.horizontalAdvance(QStringLiteral("+%1").arg(count));
            };
            const auto inputsWidth = [&](int shown){
                return InputsWidth(chipWidths, shown, chipGap, shown < chipWidths.length() ? more(chipWidths.length() - shown) : 0);
            };
            const int gap = hint.isEmpty() || inputs.isEmpty() ? 0 : padX;
            const int wanted = qMin<int>(inputs.length(), RECEIVER_MAX_INPUTS);
            const int width = HintWidth(text.width(), metrics.horizontalAdvance(m_SuggestStrings[i]),
                                        metrics.horizontalAdvance(hint) + gap + inputsWidth(wanted), padX);
            int shown = wanted;
            while(shown > 0 && inputsWidth(shown) > width) shown--;
            const int used = inputs.isEmpty() || inputsWidth(shown) > width ? 0 : inputsWidth(shown);
            const int hintWidth = qMax(0, width - used - (used ? gap : 0));
            painter.setPen(Theme::Pen(Theme::VanillaPageTextDim));
            if(used){
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing, true);
                const int height = metrics.height() + ScaleByDevice(4);
                const qreal chipRadius = ScaleByDevice(4);
                int left = text.right() + 1 - used;
                for(int j = 0; j < shown; j++){
                    const QRectF chip(left + 0.5, text.top() + (text.height() - height) / 2 + 0.5,
                                      chipWidths[j] - 1, height - 1);
                    painter.setOpacity(0.45);
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(chip, chipRadius, chipRadius);
                    painter.setOpacity(0.8);
                    painter.drawText(chip, Qt::AlignCenter, inputs[j]);
                    left += chipWidths[j] + chipGap;
                }
                if(shown < inputs.length()){
                    painter.setOpacity(0.8);
                    painter.drawText(QRect(left, text.top(), text.right() + 1 - left, text.height()),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     QStringLiteral("+%1").arg(inputs.length() - shown));
                }
                painter.restore();
            }
            if(!hint.isEmpty() && hintWidth > 0)
                painter.drawText(QRect(text.right() - width + 1, text.top(), hintWidth, text.height()),
                                 Qt::AlignRight | Qt::AlignVCenter,
                                 metrics.elidedText(hint, Qt::ElideLeft, hintWidth));
            text.setRight(text.right() - width - padX);
        }
        if(!m_Rows.isEmpty() && m_Rows[i].kind == SuggestRow::OpenTab){
            const int size = ScaleByDevice(16);
            m_Rows[i].icon.paint(&painter, QRect(text.left(), text.top() + (text.height() - size) / 2, size, size));
            text.setLeft(text.left() + size + ScaleByDevice(RECEIVER_PADDING));
        }
        painter.setPen(Theme::Pen(selected ? Theme::ReceiverTextContrast : Theme::ReceiverText));
        painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(m_SuggestStrings[i], Qt::ElideRight, text.width()));
    }

    PreviewChosenTab();
    ev->setAccepted(true);
}

void Receiver::PreviewChosenTab(){
    NodePreview *preview = NodePreview::Instance();
    const int index = m_CurrentSuggestIndex;
    const bool tab = isVisible() && window()->isActiveWindow()
        && index >= m_SuggestScroll && index < m_SuggestScroll + VisibleSuggestCount()
        && index < m_Rows.length() && m_Rows[index].kind == SuggestRow::OpenTab;
    if(!tab){
        if(m_Previewing) preview->Dismiss();
        m_Previewing = false;
        return;
    }
    const SuggestRow &row = m_Rows[index];
    const QRect local = SuggestRect(index);
    const QRect rect(mapToGlobal(local.topLeft()), local.size());
    m_Previewing = true;
    if(preview->IsAbout(row.title, rect)) return;
    ViewNode *node = VanillaPage::TabOfSerial(row.serial);
    preview->Request(node ? node->VisibleLargeImage() : QImage(),
                     row.title, row.hint, rect,
                     QRect(mapToGlobal(QPoint(0, 0)), size()), Qt::RightEdge,
                     QPoint());
}

void Receiver::hideEvent(QHideEvent *ev){
    m_TargetRequest++;
    QTimer::singleShot(0, this, [this](){
        if(!isVisible()) m_Target = TargetArgs();
    });
    m_LineEdit->SetCount(QString());
    m_SettingNoted = false;
    if(m_Previewing) NodePreview::Instance()->Dismiss();
    m_Previewing = false;
    ReleaseAnchor(m_FocusWentElsewhere);
    const auto rejoin = [this](){
        if(!m_RejoinAfterHide) return;
        QTimer::singleShot(0, this, [this](){
            if(isVisible() || m_Anchor) return;
            m_RejoinAfterHide = false;
            if(m_TreeBank->WantsReceiverJoined()) Join();
        });
    };

    if(m_FocusWentElsewhere){
        m_FocusWentElsewhere = false;
        QWidget::hideEvent(ev);
        rejoin();
        return;
    }

    MainWindow *win = m_TreeBank->GetMainWindow();

    const bool purged = IsPurged() && isActiveWindow();
    QPointer<Receiver> alive(this);
    QTimer::singleShot(0, win, [win, purged, alive](){
        if(alive && alive->isVisible()) return;
        if(!win->isVisible() || win->isMinimized()) return;
        QWidget *active = QApplication::activeWindow();
        if(active && active != win) return;
        if(purged || win->isActiveWindow()) win->SetFocus();
    });
    QWidget::hideEvent(ev);
    rejoin();
}

QString Receiver::SoughtWords() const {
    if(m_Mode == Search) return m_LineString;
    QString words;
    if(IsSeeking() && m_Mode == Command && SeekWords(m_LineString, &words)) return words;
    return QString();
}

void Receiver::showEvent(QShowEvent *ev){
    ResizeNotify(m_TreeBank->size());

    const QString words = SoughtWords();
    if(!words.isEmpty()) emit SeekText(words, View::HighlightAllOccurrences);
    m_LineEdit->show();
    m_LineEdit->raise();
    QWidget::showEvent(ev);
    if(IsPurged()){
        BlurBehind::Apply(this);
        BlurBehind::RoundCorners(this);
    }
}

void Receiver::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
    ev->setAccepted(true);
}

void Receiver::mousePressEvent(QMouseEvent *ev){
    if(ev->button() != Qt::LeftButton){
        if(ev->button() == Qt::RightButton) View::TakeRightButtonConsumed();
        QWidget::mousePressEvent(ev);
        return;
    }
    if(ev->pos().x() >= width() - ScaleByDevice(RECEIVER_PADDING) && ScrollToPointer(ev->pos())){
        m_DraggingScroll = true;
        return;
    }
    if(!m_SuggestStrings.isEmpty()) m_CurrentSuggestIndex = SuggestAt(ev->pos());

    const int source = SourceBarAt(ev->pos());
    if(source != -1){
        SelectTab(source);
        return;
    }
    if(m_Mode == Query && m_CurrentSuggestIndex != -1){
        SuitableAction();
        return;
    }
    if((m_Mode == Command || m_Mode == Target) && !m_Rows.isEmpty()){
        if(m_CurrentSuggestIndex != -1){
            TakeRow(m_Rows[m_CurrentSuggestIndex]);
            return;
        }
    } else if(m_Mode == Command){
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            const QString key = m_SuggestStrings[m_CurrentSuggestIndex];
            const QString text = WithCurrentValue(key);
            m_LineEdit->setText(text.isEmpty() ? key : text);
            m_LineEdit->setFocus();
        }
    }
    QWidget::mousePressEvent(ev);
}

void Receiver::mouseReleaseEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton) m_DraggingScroll = false;
    QWidget::mouseReleaseEvent(ev);
}

void Receiver::mouseMoveEvent(QMouseEvent *ev){
    if(m_DraggingScroll && (ev->buttons() & Qt::LeftButton)){
        ScrollToPointer(ev->pos());
        return;
    }
    m_DraggingScroll = false;
    if(!m_SuggestStrings.isEmpty()){
        const int index = SuggestAt(ev->pos());
        if(index != m_CurrentSuggestIndex){
            m_CurrentSuggestIndex = index;
            repaint();
        }
    }
    QWidget::mouseMoveEvent(ev);
}

void Receiver::wheelEvent(QWheelEvent *ev){
    const int notches = ev->angleDelta().y() / 120;
    if(notches) ScrollSuggestsBy(-notches * RECEIVER_WHEEL_ROWS);
    ev->accept();
}

void Receiver::DisplaySuggest(const QByteArray &ba){

    if(!m_LineEdit->hasFocus() || m_LineString.trimmed().isEmpty()) return;

    if(m_Mode == Query){
        SetSuggest(SearchSuggest::Parse(ba));
    } else if(m_Mode == Command && m_ActiveTab == SuggestRow::Web + 1 &&
              Page::ExtractUrlsFromText(m_LineString).isEmpty()){
        m_WebSuggests = SearchSuggest::Parse(ba);
        RefillKeepingChoice();
    }
}
