#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "blurbehind.hpp"

#include "titlebar.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVariantAnimation>
#include <QWindow>
#include <cmath>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

#include "mainwindow.hpp"
#include "application.hpp"
#include "treebank.hpp"
#include "gadgets.hpp"
#include "view.hpp"
#include "receiver.hpp"
#include "toolbar.hpp"
#include "settingspage.hpp"
#include "siteinfo.hpp"
#include "extensionbar.hpp"

TitleBar::TitleBar(MainWindow *mainwindow, Mode mode)
    : QWidget(mode == Inner ? mainwindow : nullptr)
    , m_MainWindow(mainwindow)
    , m_Mode(mode)
    , m_Hovered(NoButton)
    , m_Pressed(NoButton)
    , m_Moved(false)
    , m_NativeHovered(false)
    , m_NativePressed(false)
    , m_EditPending(false)
{
    if(mode != Inner){
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    setMouseTracking(true);
}

bool TitleBar::IsCompact() const {
    return m_Mode != Inner && m_MainWindow->isMaximized() && !m_MainWindow->IsStripExpanded();
}

int TitleBar::BarHeight() const {
    return m_Mode == Inner ? height() : ScaleByDevice(TITLE_BAR_HEIGHT);
}

void TitleBar::SetStripGeometry(const QRect &target){
    m_StripTarget = target;
    ApplyRevealGeometry();
}

void TitleBar::ApplyRevealGeometry(){
    if(m_StripTarget.isNull()) return;
    QRect rect = m_StripTarget;
    rect.setHeight(qMax(1, qRound(m_StripTarget.height() * m_RevealShown)));
    const bool round = rect.height() >= ScaleByDevice(18);
    if(m_Mode == Strip && round != m_Rounded){
        m_Rounded = round;
        if(internalWinId()) BlurBehind::RoundCorners(this, round);
    }
    if(rect != geometry()) setGeometry(rect);
}

bool TitleBar::IsSliding() const {
    return m_RevealAnimation && m_RevealAnimation->state() == QAbstractAnimation::Running;
}

bool TitleBar::Reveal(bool on, bool animate){
    if(animate && style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) <= 0)
        animate = false;
    if(!animate){
        if(IsSliding()) m_RevealAnimation->stop();
        m_RevealTarget = on;
        m_RevealShown = 1.0;
        ApplyRevealGeometry();
        if(on == isVisible()) return false;
        setVisible(on);
        return on;
    }
    if(on == m_RevealTarget && (IsSliding() || on == isVisible())) return false;
    m_RevealTarget = on;
    bool shown = false;
    if(on && !isVisible()){
        m_RevealShown = 0.0;
        ApplyRevealGeometry();
        show();
        shown = true;
    }
    if(!on){
        m_EditPending = false;
        EndEdit();
        m_Pressed = NoButton;
        m_Hovered = NoButton;
    }
    if(!m_RevealAnimation){
        m_RevealAnimation = new QVariantAnimation(this);
        m_RevealAnimation->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_RevealAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value){
            m_RevealShown = value.toReal();
            ApplyRevealGeometry();
        });
        connect(m_RevealAnimation, &QAbstractAnimation::finished, this, [this](){
            if(!m_RevealTarget) hide();
        });
    }
    m_RevealAnimation->stop();
    const qreal end = on ? 1.0 : 0.0;
    const int duration = qMax(1, qRound(150 * qAbs(end - m_RevealShown)));
    m_RevealAnimation->setStartValue(m_RevealShown);
    m_RevealAnimation->setEndValue(end);
    m_RevealAnimation->setDuration(duration);
    m_RevealAnimation->start();
    m_RevealAnimation->setCurrentTime(qMin(16, duration));
    return shown;
}

void TitleBar::FinishReveal(){
    if(!IsSliding() && m_RevealShown >= 1.0) return;
    if(!m_RevealTarget) m_MainWindow->HoldStripReveal();
    Reveal(true, false);
}

bool TitleBar::IsEditable() const {
    return m_Mode == Inner ||
        (m_Mode == Strip && m_MainWindow->isMaximized() && m_MainWindow->IsStripExpanded() &&
         !m_MainWindow->IsShaded());
}

QRect TitleBar::ButtonRect(Button button) const {
    const int h = BarHeight();
    const int side = ScaleByDevice(SideButtonWidth);
    const int caption = ScaleByDevice(CaptionButtonWidth);
    const bool compact = IsCompact();
    const bool expander = m_Mode == Strip && m_MainWindow->isMaximized();
    const Button lead = !compact ? MenuButton : expander ? ExpandButton : NoButton;
    if(button == NoButton) return QRect();
    if(button == lead) return QRect(0, 0, side, h);
    if(button == MenuButton) return QRect();
    if(button == ExpandButton && (compact || !expander)) return QRect();
    static const Button order[] = {CloseButton, MaximizeButton, MinimizeButton,
                                   ShadeButton, ViewTreeButton, ExpandButton};
    const int reserve = lead == NoButton ? 0 : compact ? side : side + ScaleByDevice(32);
    int right = width();
    for(Button each : order){
        const int w = each == ViewTreeButton || each == ShadeButton || each == ExpandButton ? side : caption;
        const QRect rect(right - w, 0, w, h);
        if(each != CloseButton && rect.left() < reserve) return QRect();
        if(each == button) return rect;
        right -= w;
    }
    return QRect();
}

QRect TitleBar::TitleRect() const {
    if(IsCompact()) return QRect();
    int right = width();
    for(Button each : AllButtons){
        if(each == MenuButton) continue;
        const QRect r = ButtonRect(each);
        if(!r.isEmpty()) right = qMin(right, r.left());
    }
    const int left = ButtonRect(MenuButton).right() + 1 + ScaleByDevice(4);
    const int w = right - ScaleByDevice(8) - left;
    return w > 0 ? QRect(left, 0, w, BarHeight()) : QRect();
}

QRect TitleBar::EditRect() const {
    const QRect area = TitleRect();
    if(area.isEmpty()) return QRect();
    const int text = QFontMetrics(TitleBarTitleFont()).horizontalAdvance(m_MainWindow->windowTitle())
                     + ScaleByDevice(8);
    const int w = qBound(area.width() / 5, text, area.width() * 9 / 10);
    return w > 0 ? QRect(area.left(), 0, w, BarHeight()) : QRect();
}

void TitleBar::BeginEdit(){
    if(!IsEditable() || IsEditing()) return;
    if(TitleRect().isEmpty()) return;
    TreeBank *bank = m_MainWindow->GetTreeBank();
    if(!m_EditRow) m_EditRow = new TitleEditRow(this, bank);
    const SharedView view = bank->GetCurrentView();
    if(view) view->TakeKeyboardBack();
    if(m_Mode == Strip){
        activateWindow();
        raise();
    }
    m_EditRow->Begin(view);
    update();
}

void TitleBar::EndEdit(bool giveBack){
    if(!IsEditing()) return;
    m_EditRow->End();
    if(giveBack) m_MainWindow->SetFocus();
    update();
}

bool TitleBar::IsEditing() const {
    return m_EditRow && m_EditRow->IsEditing();
}

LineEdit *TitleBar::GetAddressDisplay() const {
    return m_EditRow ? m_EditRow->Address() : nullptr;
}

QList<QToolButton*> TitleBar::NavigationButtons() const {
    return m_EditRow ? m_EditRow->NavigationButtons() : QList<QToolButton*>();
}

ExtensionBar *TitleBar::GetExtensionBar() const {
    return m_EditRow ? m_EditRow->Extensions() : nullptr;
}

SiteButton *TitleBar::GetSiteButton() const {
    return m_EditRow ? m_EditRow->Site() : nullptr;
}

void TitleBar::keyPressEvent(QKeyEvent *ev){
    if(IsEditing() && ev->key() == Qt::Key_Escape){
        EndEdit(true);
        ev->accept();
        return;
    }
    QWidget::keyPressEvent(ev);
}

TitleBar::Button TitleBar::ButtonAt(const QPoint &pos) const {
    for(Button each : AllButtons)
        if(ButtonRect(each).contains(pos)) return each;
    return NoButton;
}

void TitleBar::SetNativeHover(bool hovered){
    if(m_NativeHovered == hovered) return;
    m_NativeHovered = hovered;
    update();
}

void TitleBar::SetNativePressed(bool pressed){
    if(m_NativePressed == pressed) return;
    m_NativePressed = pressed;
    update();
}

void TitleBar::paintEvent(QPaintEvent *ev){
    QPainter painter(this);

    const QColor ground = Theme::Color(IsTranslucent() ? Theme::BarBackgroundTranslucent
                                                   : Theme::BarBackground);
    const int lift = m_Mode == Inner ? 0 : height() - BarHeight();
    if(m_Mode == Strip){
        const qreal radius = ScaleByDevice(8);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), Qt::transparent);
        painter.translate(0, lift);
        QPainterPath card;
        card.addRoundedRect(QRectF(0, 0, width(), BarHeight()), radius, radius);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillPath(card, ground);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setClipPath(card);
        painter.setRenderHint(QPainter::Antialiasing, false);
    } else {
        painter.fillRect(rect(), ground);
        painter.translate(0, lift);
    }

    const bool active = m_MainWindow == Application::GetCurrentWindow();
    QColor ink = Theme::Color(Theme::TitleBarText);
    if(!active) ink.setAlpha(ink.alpha() * 45 / 100);

    for(Button each : AllButtons){
        const QRect r = ButtonRect(each);
        if(r.isEmpty()) continue;
        const bool native = each == MaximizeButton;
        const bool hovered = m_Hovered == each || (native && m_NativeHovered);
        const bool pressed = hovered && (m_Pressed == each || (native && m_NativePressed));
        QColor glyph = ink;
        if(each == CloseButton && hovered){
            painter.fillRect(r, Theme::Color(Theme::TitleBarCloseHovered, pressed ? 220 : 255));
            glyph = Theme::Color(Theme::TitleBarCloseText);
        } else if(hovered){
            painter.fillRect(r, Theme::Color(Theme::TitleBarButtonHovered));
            if(pressed) painter.fillRect(r, Theme::Color(Theme::TitleBarButtonHovered));
        }
        PaintGlyph(painter, each, r, glyph);
    }

    if(!IsCompact() && !IsEditing()){
        const bool editable = IsEditable();
        const QRect band = editable ? EditRect() : TitleRect();
        if(!band.isEmpty()){
            const int pad = editable ? ScaleByDevice(8) : 0;
            const QRect text = band.adjusted(0, 0, -pad, 0);
            painter.setFont(TitleBarTitleFont());
            painter.setPen(ink);
            painter.drawText(text, Qt::TextSingleLine | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(m_MainWindow->windowTitle(),
                                                              Qt::ElideRight, qMax(0, text.width())));
            if(editable){
                QColor mark = ink;
                mark.setAlpha(mark.alpha() * 40 / 100);
                const int markHeight = BarHeight() / 2;
                const int top = (BarHeight() - markHeight) / 2;
                const int w = qMax(1, ScaleByDevice(1));
                painter.fillRect(QRect(band.right() + 1 - w, top, w, markHeight), mark);
            }
        }
    }
    if(m_EditRow && m_EditRow->IsEditing())
        LineEdit::PaintAddressHover(&painter, this, m_EditRow->Site(), m_EditRow->Address());
    ev->setAccepted(true);
}

void TitleBar::PaintGlyph(QPainter &painter, Button button, const QRect &rect, const QColor &color) const {
    const qreal u = ScaleByDevice(10.0) / 10.0;
    const qreal width = qMax<qreal>(1.0, std::floor(u));
    const qreal half = static_cast<int>(width) % 2 ? 0.5 : 0.0;
    painter.save();
    painter.translate(QPointF(rect.x() + rect.width()/2 + half, rect.y() + rect.height()/2 + half));
    QPen pen(color, width);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    const qreal a = 5*u;
    switch(button){
    case MenuButton:
        for(qreal y : {-3*u, 0.0, 3*u}) painter.drawLine(QPointF(-a, y), QPointF(a, y));
        break;
    case ViewTreeButton:
        for(qreal x : {-a, u}) for(qreal y : {-a, u})
            painter.fillRect(QRectF(x - half, y - half, 4*u, 4*u), color);
        break;
    case ShadeButton: {
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal d = m_MainWindow->IsShaded() ? -1 : 1;
        const QPointF points[] = {QPointF(-4*u, 2*u*d), QPointF(0, -2*u*d), QPointF(4*u, 2*u*d)};
        painter.drawPolyline(points, 3);
        break;
    }
    case MinimizeButton:
        painter.drawLine(QPointF(-a, 0), QPointF(a, 0));
        break;
    case MaximizeButton:
        if(m_MainWindow->isMaximized()){
            painter.drawRect(QRectF(-a, -3*u, 8*u - width, 8*u - width));
            const QPointF points[] = {QPointF(-3*u, -3*u), QPointF(-3*u, -a),
                                      QPointF(a - width, -a), QPointF(a - width, 3*u),
                                      QPointF(3*u, 3*u)};
            painter.drawPolyline(points, 5);
        } else {
            painter.drawRect(QRectF(-a, -a, 10*u - width, 10*u - width));
        }
        break;
    case CloseButton:
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawLine(QPointF(-a, -a), QPointF(a, a));
        painter.drawLine(QPointF(-a, a), QPointF(a, -a));
        break;
    case ExpandButton: {
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal d = m_MainWindow->IsStripExpanded() ? -1 : 1;
        const QPointF points[] = {QPointF(2*u*d, -4*u), QPointF(-2*u*d, 0), QPointF(2*u*d, 4*u)};
        painter.drawPolyline(points, 3);
        break;
    }
    case NoButton:
        break;
    }
    painter.restore();
}

void TitleBar::mousePressEvent(QMouseEvent *ev){
    if(m_Mode != Inner) FinishReveal();
    m_MainWindow->raise();
    if(m_Mode != Inner) raise();
    if(m_Mode == Strip) m_MainWindow->PinStrip();
    m_Moved = false;
    m_Pressed = NoButton;
    m_EditPending = false;
    if(ev->button() == Qt::LeftButton && IsEditable() && !IsEditing() &&
       EditRect().contains(ev->pos())){
        m_EditPending = true;
        m_PressPos = ev->pos();
        return;
    }
    if(ev->button() == Qt::LeftButton){
        m_Pressed = ButtonAt(ev->pos());
        m_Pos = ev->globalPosition().toPoint() - m_MainWindow->pos();
        m_PressPos = ev->pos();
        if(m_Pressed == NoButton && m_Mode == Inner) StartSystemMove();
    }
    update();
}

void TitleBar::mouseMoveEvent(QMouseEvent *ev){
    if(m_EditPending && (ev->buttons() & Qt::LeftButton)){
        if(IsDrag(ev->pos() - m_PressPos)){
            m_EditPending = false;
            if(m_Mode == Inner || m_MainWindow->isMaximized()) StartSystemMove();
        }
        return;
    }
    if(ev->buttons() & Qt::LeftButton){
        if(m_Pressed == NoButton && m_Mode != Inner && m_MainWindow->isMaximized()){
            if(!m_Moved && IsDrag(ev->pos() - m_PressPos)){
                m_Moved = true;
                StartSystemMove();
            }
        } else if(m_Pressed == NoButton && m_Mode != Inner){
            m_Moved = true;
            m_MainWindow->move(ev->globalPosition().toPoint() - m_Pos);
        }
        return;
    }
    const Button before = m_Hovered;
    m_Hovered = ButtonAt(ev->pos());
    if(before != m_Hovered) update();
    if(m_EditRow && m_EditRow->IsEditing())
        update(LineEdit::AddressRect(m_EditRow->Site(), m_EditRow->Address()));
}

void TitleBar::StartSystemMove(){
    if(QWindow *window = m_MainWindow->windowHandle()) window->startSystemMove();
}

bool TitleBar::IsDrag(const QPoint &moved) const {
#if defined(Q_OS_WIN)
    if(m_Mode == Strip && m_MainWindow->isMaximized())
        return qAbs(moved.x()) > ::GetSystemMetricsForDpi(SM_CXDRAG, USER_DEFAULT_SCREEN_DPI) / 2 ||
               qAbs(moved.y()) > ::GetSystemMetricsForDpi(SM_CYDRAG, USER_DEFAULT_SCREEN_DPI) / 2;
#endif
    return moved.manhattanLength() >= QApplication::startDragDistance();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *ev){
    if(m_EditPending){
        m_EditPending = false;
        if(ev->button() == Qt::LeftButton) BeginEdit();
        ev->setAccepted(true);
        return;
    }
    const Button pressed = m_Pressed;
    m_Pressed = NoButton;
    if(m_Moved){
        m_Moved = false;
        m_MainWindow->SetFocus();
    } else if(ev->button() == Qt::LeftButton){
        const Button released = ButtonAt(ev->pos());
        if(released == NoButton) m_MainWindow->SetFocus();
        else if(released == pressed) Trigger(released);
    } else if(ev->button() == Qt::RightButton){
        m_MainWindow->ToggleShaded();
    }
    update();
    ev->setAccepted(true);
}

void TitleBar::Trigger(Button button){
    switch(button){
    case MenuButton: {
        QMenu *menu = m_MainWindow->GetTreeBank()->GlobalContextMenu();
        menu->exec(mapToGlobal(ButtonRect(MenuButton).bottomLeft() + QPoint(0, 1)));
        delete menu;
        if(Application::GetCurrentWindow() == m_MainWindow) m_MainWindow->SetFocus();
        break;
    }
    case ViewTreeButton: {
        Gadgets *g = m_MainWindow->GetTreeBank()->GetGadgets();
        if(g && g->IsActive()) g->Deactivate();
        else m_MainWindow->GetTreeBank()->DisplayViewTree();
        m_MainWindow->SetFocus();
        break;
    }
    case ShadeButton:    m_MainWindow->ToggleShaded();    break;
    case MinimizeButton: m_MainWindow->ToggleMinimized(); break;
    case MaximizeButton: m_MainWindow->ToggleMaximized(); break;
    case ExpandButton:   m_MainWindow->SetStripExpanded(!m_MainWindow->IsStripExpanded()); break;
    case CloseButton:    m_MainWindow->close();           break;
    case NoButton: break;
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton && ButtonAt(ev->pos()) != NoButton)
        return mousePressEvent(ev);
    const bool onTitle = IsEditable() && EditRect().contains(ev->pos());
    if(ButtonAt(ev->pos()) == NoButton && ev->button() == Qt::LeftButton && !onTitle)
        m_MainWindow->ToggleMaximized();
    ev->setAccepted(true);
}

void TitleBar::leaveEvent(QEvent *ev){
    Q_UNUSED(ev)
    m_Hovered = NoButton;
    update();
}

void TitleBar::resizeEvent(QResizeEvent *ev){
    QWidget::resizeEvent(ev);
    if(IsEditing()){
        if(TitleRect().isEmpty()) EndEdit();
        else m_EditRow->Layout();
    }
}

void TitleBar::showEvent(QShowEvent *ev){
    QWidget::showEvent(ev);
    if(m_Mode == Strip && windowHandle() && m_MainWindow->windowHandle() &&
       windowHandle()->transientParent() != m_MainWindow->windowHandle())
        windowHandle()->setTransientParent(m_MainWindow->windowHandle());
    if(m_Mode == Strip) BlurBehind::RoundCorners(this, m_Rounded);
    if(m_Mode != Inner && IsTranslucent()) BlurBehind::Blur(this);
}

bool TitleBar::IsTranslucent() const {
    return m_MainWindow->testAttribute(Qt::WA_TranslucentBackground) &&
           Application::EnableTransparentWindow();
}

void TitleBar::hideEvent(QHideEvent *ev){
    m_EditPending = false;
    EndEdit();
    QWidget::hideEvent(ev);
}

bool TitleBar::event(QEvent *ev){
    if(ev->type() == QEvent::WindowDeactivate) m_EditPending = false;
    if(ev->type() == QEvent::LayoutRequest && IsEditing()) m_EditRow->Layout();
    return QWidget::event(ev);
}

#if defined(Q_OS_WIN)
bool TitleBar::nativeEvent(const QByteArray &eventType, void *message, qintptr *result){
    if(m_Mode == Inner && eventType == "windows_generic_MSG" && message &&
       static_cast<const MSG*>(message)->message == WM_NCHITTEST){
        *result = HTTRANSPARENT;
        return true;
    }
    return QWidget::nativeEvent(eventType, message, result);
}
#endif

TitleEditRow::TitleEditRow(TitleBar *bar, TreeBank *bank)
    : QObject(bar)
    , m_Bar(bar)
    , m_TreeBank(bank)
{
    m_Address = new LineEdit(bar);
    m_Address->setFont(ToolBarFieldFont());
    m_Address->setMinimumHeight(m_Address->ScaleByDevice(TOOL_BAR_FIELD_HEIGHT));
    m_Address->setReadOnly(true);
    m_Address->setFocusPolicy(Qt::NoFocus);
    m_Address->setCursor(Qt::IBeamCursor);
    m_Address->hide();
    m_Address->installEventFilter(this);
    m_Address->setProperty(LineEdit::AddressHoverProperty, true);
    m_Address->ApplyTheme();
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *old, QWidget *now){
        if(!m_Kept || !m_Editing) return;
        Receiver *line = m_TreeBank->GetReceiver();
        if(line && old && (old == line || line->isAncestorOf(old))) return;
        if(!IsOurs(now) && !IsOurs(QApplication::activePopupWidget())) m_Bar->EndEdit();
    });
    for(QAction *action : ToolBar::NavigationActions(bank)){
        QToolButton *button = new QToolButton(bar);
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        const int icon = bar->ScaleByDevice(TOOL_BAR_ICON_SIZE);
        button->setIconSize(QSize(icon, icon));
        button->hide();
        connect(action, &QAction::changed, this, [this](){ if(m_Editing) Layout();});
        m_Navigation << button;
    }
    m_Site = new SiteButton(bar);
    m_Site->hide();
    m_Site->setProperty(LineEdit::AddressHoverProperty, true);
    m_Site->installEventFilter(this);
    connect(m_Site, &QToolButton::clicked, this, &TitleEditRow::OpenSitePanel);
    m_Extensions = new ExtensionBar(bar);
    m_Extensions->hide();
}

void TitleEditRow::Begin(const SharedView &view){
    if(m_Editing) return;
    m_Editing = true;
    m_View = view;
    if(view){
        m_Address->setText(VanillaPage::IsStartUrl(view->url()) ? QString() : view->url().toString());
        m_Address->setCursorPosition(0);
        ToolBar::SyncNavigation(m_TreeBank, view.get(), view->IsLoading());
        connect(view->base(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(SyncNavigation()));
        connect(view->base(), SIGNAL(loadStarted()), this, SLOT(SyncNavigation()));
        connect(view->base(), SIGNAL(loadFinished(bool)), this, SLOT(SyncNavigation()));
        m_Extensions->SetView(view);
    } else {
        m_Address->clear();
    }
    Layout();
    m_Address->show();
    if(!m_Address->isVisible()){
        m_Bar->EndEdit();
        return;
    }
    OpenLine();
}

void TitleEditRow::OpenLine(){
    SetKept(false);
    m_TreeBank->OpenCommandAtField(m_Address);
    if(Receiver *receiver = m_TreeBank->GetReceiver()){
        connect(receiver, &Receiver::AnchorReleased, this, &TitleEditRow::OnAnchorReleased,
                Qt::UniqueConnection);
        if(!receiver->IsAnchored()) m_Bar->EndEdit();
    }
}

void TitleEditRow::End(){
    if(!m_Editing) return;
    m_Editing = false;
    SetKept(false);
    if(m_View) disconnect(m_View->base(), nullptr, this, SLOT(SyncNavigation()));
    m_View.reset();
    m_Address->hide();
    for(QToolButton *button : std::as_const(m_Navigation)) button->hide();
    if(m_SitePanel) m_SitePanel->close();
    m_Site->hide();
    m_Extensions->SetView({});
    m_Extensions->hide();
}

void TitleEditRow::OnAnchorReleased(QWidget *field, bool focusLeft){
    if(!m_Editing || field != m_Address) return;
    if(m_Kept) return;
    QWidget *active = QApplication::activeWindow();
    const bool here = !active || active == m_Bar->window() ||
        active == m_TreeBank->GetReceiver();
    if(focusLeft && (IsOurs(QApplication::focusWidget()) ||
                     (here && IsOurs(QApplication::widgetAt(QCursor::pos()))) ||
                     IsOurs(QApplication::activePopupWidget()))){
        SetKept(true);
        return;
    }
    m_Bar->EndEdit();
}

void TitleEditRow::OpenSitePanel(){
    if(!m_Editing) return;
    if(m_SitePanel){
        m_SitePanel->close();
        return;
    }
    SetKept(true);
    if(Receiver *line = m_TreeBank->GetReceiver())
        if(line->IsAnchored()) line->Dismiss();
    if(m_View) m_View->TakeKeyboardBack();
    m_SitePanel = SiteInfoPanel::Open(m_Site, m_TreeBank, m_View ? m_View->url() : QUrl(),
                                        m_View ? m_View->OpenSiteDataLink() : nullptr);
    connect(m_SitePanel, &QObject::destroyed, this, [this](){
        if(m_Editing) m_Bar->EndEdit();
    });
}

void TitleEditRow::SetKept(bool on){
    if(m_Kept == on) return;
    m_Kept = on;
    if(on) qApp->installEventFilter(this);
    else qApp->removeEventFilter(this);
}

bool TitleEditRow::IsOurs(const QObject *object) const {
    for(const QObject *o = object; o; o = o->parent()) if(o == m_Bar) return true;
    return false;
}

bool TitleEditRow::eventFilter(QObject *watched, QEvent *ev){
    if((watched == m_Address || watched == m_Site) &&
       (ev->type() == QEvent::Enter || ev->type() == QEvent::Leave))
        m_Bar->update(LineEdit::AddressRect(m_Site, m_Address));
    if(m_Kept && m_Editing && ev->type() == QEvent::KeyPress &&
       static_cast<QKeyEvent*>(ev)->key() == Qt::Key_Escape){
        QTimer::singleShot(0, this, [this](){ if(m_Editing && m_Kept) m_Bar->EndEdit();});
        return QObject::eventFilter(watched, ev);
    }
    if(ev->type() != QEvent::MouseButtonPress || !m_Editing) return QObject::eventFilter(watched, ev);
    if(watched == m_Address && static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton){
        OpenLine();
        return true;
    }
    if(m_Kept && watched->isWidgetType() && !IsOurs(watched))
        QTimer::singleShot(0, this, [this](){ if(m_Editing && m_Kept) m_Bar->EndEdit();});
    return QObject::eventFilter(watched, ev);
}

void TitleEditRow::SyncNavigation(){
    if(!m_Editing) return;
    if(m_View) ToolBar::SyncNavigation(m_TreeBank, m_View.get(), m_View->IsLoading());
}

void TitleEditRow::Layout(){
    const QRect area = m_Bar->TitleRect();
    const int h = m_Bar->height();
    const int spacing = m_Bar->ScaleByDevice(TOOL_BAR_ICON_SPACING);
    const int clear = spacing * 2 + m_Bar->ScaleByDevice(RECEIVER_PADDING);
    const int fieldMin = qMin(area.width(), m_Bar->ScaleByDevice(120));
    int room = area.width() - fieldMin;
    int left = area.left();
    int buttonHeight = 0;
    for(QToolButton *button : std::as_const(m_Navigation)){
        const QSize size = button->sizeHint().boundedTo(QSize(h, h));
        const int before = left > area.left() ? spacing : 0;
        const bool shown = button->defaultAction() && button->defaultAction()->isVisible() &&
                           left - area.left() + before + size.width() <= room;
        button->setVisible(m_Editing && shown);
        if(!shown) continue;
        left += before;
        button->setGeometry(left, (h - size.height()) / 2, size.width(), size.height());
        left += size.width();
        buttonHeight = qMax(buttonHeight, size.height());
    }
    const int apart = left > area.left() ? spacing + m_Bar->ScaleByDevice(SITE_BUTTON_APART) : 0;
    const int siteWidth = m_Bar->ScaleByDevice(TOOL_BAR_ICON_SIZE + SITE_BUTTON_SIDE);
    const int siteHeight = qMin(h, buttonHeight ? buttonHeight : m_Site->sizeHint().height());
    const bool site = left - area.left() + apart + siteWidth <= room;
    m_Site->setVisible(m_Editing && site);
    int fieldClear = clear;
    if(site){
        left += apart;
        m_Site->setGeometry(left, (h - siteHeight) / 2, siteWidth, siteHeight);
        left += siteWidth;
        fieldClear = 0;
    }
    if(left > area.left()) room -= left - area.left() + fieldClear;
    const QSize extensions = m_Extensions->sizeHint().expandedTo(m_Extensions->minimumSize());
    const bool withExtensions = room >= extensions.width() + clear;
    const int ew = withExtensions ? extensions.width() : 0;
    m_Extensions->setVisible(m_Editing && withExtensions);
    const int eh = qMin(h, extensions.height());
    m_Extensions->setGeometry(area.right() + 1 - ew, (h - eh) / 2, ew, eh);
    LineEdit *field = m_Address;
    const int fieldLeft = left == area.left() ? left : left + fieldClear;
    const int fieldRight = area.right() + 1 - (withExtensions ? ew + clear : 0);
    const int fh = qMin(h, field->sizeHint().expandedTo(field->minimumSize()).height());
    field->setGeometry(fieldLeft, (h - fh) / 2, qMax(0, fieldRight - fieldLeft), fh);
}
