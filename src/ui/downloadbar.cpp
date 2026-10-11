#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "downloadbar.hpp"

#include "application.hpp"
#include "mainwindow.hpp"
#include "toolbar.hpp"
#include "treebank.hpp"

#include <QAbstractButton>
#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QEnterEvent>
#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHash>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace {

    QHash<MainWindow*, QPointer<DownloadPanel>> &Panels(){
        static QHash<MainWindow*, QPointer<DownloadPanel>> panels;
        return panels;
    }

    QToolButton *IconButton(QWidget *parent, const QString &path, const QString &text){
        QToolButton *button = new QToolButton(parent);
        button->setIcon(QIcon(Theme::Pixmap(path, Theme::PreviewText)));
        const int size = DeviceScale::FromDpi(TOOL_BAR_ICON_SIZE, static_cast<int>(parent->logicalDpiY()));
        button->setIconSize(QSize(size, size));
        button->setToolTip(text);
        button->setAccessibleName(text);
        button->setAutoRaise(true);
        button->setCursor(Qt::PointingHandCursor);
        return button;
    }

    class DownloadRow : public QWidget {
    public:
        explicit DownloadRow(QWidget *parent)
            : QWidget(parent), m_Hovered(false) {}

        void AddHoverAction(QWidget *widget){
            QSizePolicy policy = widget->sizePolicy();
            policy.setRetainSizeWhenHidden(true);
            widget->setSizePolicy(policy);
            widget->setVisible(m_Hovered);
            m_HoverActions << widget;
        }

    protected:
        void enterEvent(QEnterEvent *ev) Q_DECL_OVERRIDE {
            QWidget::enterEvent(ev);
            SetHovered(true);
        }
        void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE {
            QWidget::leaveEvent(ev);
            SetHovered(false);
        }
        void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE {
            Q_UNUSED(ev)
            if(!m_Hovered) return;
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            QColor fill = Theme::Color(Theme::PreviewText);
            fill.setAlphaF(DOWNLOAD_PANEL_HOVER_ALPHA);
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            const int radius = DeviceScale::FromDpi(DOWNLOAD_PANEL_ROW_RADIUS, static_cast<int>(logicalDpiY()));
            painter.drawRoundedRect(rect(), radius, radius);
        }

    private:
        void SetHovered(bool on){
            if(m_Hovered == on) return;
            m_Hovered = on;
            foreach(QWidget *widget, m_HoverActions) widget->setVisible(on);
            update();
        }

        bool m_Hovered;
        QList<QWidget*> m_HoverActions;
    };
}

DownloadBar::DownloadBar(QWidget *parent)
    : QWidget(parent)
    , m_RanOut(false)
    , m_Needed(false)
{
    setObjectName(QStringLiteral("DownloadBar"));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_Button = new QToolButton(this);
    m_Button->setObjectName(QStringLiteral("DownloadsButton"));
    m_Button->setToolTip(tr("Downloads"));
    m_Button->setAccessibleName(tr("Downloads"));
    m_Button->setAutoRaise(true);
    m_Button->setIcon(DownloadIcon());
    m_Button->setIconSize(QSize(ScaleByDevice(TOOL_BAR_ICON_SIZE),
                                ScaleByDevice(TOOL_BAR_ICON_SIZE)));
    layout->addWidget(m_Button);
    connect(m_Button, &QToolButton::clicked, this, [this](){
        if(MainWindow *window = qobject_cast<MainWindow*>(this->window()))
            DownloadPanel::ShowFor(window, true, true);
    });
    DownloadPanel::Initialize();

    m_Timer = new QTimer(this);
    m_Timer->setSingleShot(true);
    connect(m_Timer, &QTimer::timeout, this, [this](){
        m_RanOut = true;
        Reconsider();
    });
    connect(DownloadHistory::Instance(), &DownloadHistory::ListChanged,
            this, &DownloadBar::Reconsider);
    Reconsider();
}

DownloadBar::~DownloadBar(){}

qint64 DownloadBar::WantedFor(const QList<DownloadHistory::Entry> &entries, const QDateTime &now){
    qint64 left = 0;
    foreach(const DownloadHistory::Entry &entry, entries){
        if(entry.state == DownloadHistory::Running) return WantedWhileRunning;
        if(!entry.ended.isValid()) continue;
        const qint64 ms = DOWNLOAD_BUTTON_LINGER - qMax<qint64>(0, entry.ended.msecsTo(now));
        left = qMax(left, ms);
    }
    return left;
}

QDateTime DownloadBar::NewestEnding(const QList<DownloadHistory::Entry> &entries){
    QDateTime newest;
    foreach(const DownloadHistory::Entry &entry, entries)
        if(entry.ended.isValid() && (!newest.isValid() || entry.ended > newest))
            newest = entry.ended;
    return newest;
}

void DownloadBar::Reconsider(){
    const QList<DownloadHistory::Entry> entries = DownloadHistory::Instance()->Entries();
    const qint64 wanted = WantedFor(entries, QDateTime::currentDateTime());
    bool listWants = wanted != 0;
    if(wanted > 0){
        const QDateTime newest = NewestEnding(entries);
        if(newest != m_ArmedFor){
            m_ArmedFor = newest;
            m_RanOut = false;
            m_Timer->start(static_cast<int>(qMin<qint64>(wanted, INT_MAX)));
        } else if(m_RanOut){
            listWants = false;
        } else if(!m_Timer->isActive()){
            m_Timer->start(static_cast<int>(qMin<qint64>(wanted, INT_MAX)));
        }
    } else {
        m_Timer->stop();
    }
    MainWindow *window = qobject_cast<MainWindow*>(this->window());
    const bool needed = listWants || DownloadPanel::IsOpenFor(window);
    if(needed == m_Needed) return;
    m_Needed = needed;
    emit NeededChanged(needed);
}

QIcon DownloadBar::DownloadIcon(){
    return QIcon(Theme::Pixmap(QStringLiteral(":/resources/toolbar/download.png"),
                               Theme::BarIcon));
}

void DownloadBar::ApplyTheme(){
    m_Button->setIcon(DownloadIcon());
}

DownloadPanel::DownloadPanel(MainWindow *window)
    : QWidget(window, Qt::Tool | Qt::FramelessWindowHint)
    , m_Window(window)
    , m_ButtonDown(false)
    , m_EscapeDown(false)
{
    setObjectName(QStringLiteral("DownloadsPanel"));
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedWidth(ScaleByDevice(DOWNLOAD_PANEL_WIDTH));

    const int padding = ScaleByDevice(DOWNLOAD_PANEL_PADDING);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(padding, padding, padding, padding);

    QLabel *title = new QLabel(tr("Downloads"), this);
    title->setFont(ExtensionListTitleFont());
    title->setContentsMargins(padding / 2, 0, 0, 0);
    layout->addWidget(title);
    layout->addSpacing(padding / 2);

    m_Scroll = new QScrollArea(this);
    m_Scroll->setWidgetResizable(true);
    m_Scroll->setFrameShape(QFrame::NoFrame);
    m_Scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_Scroll->viewport()->setAutoFillBackground(false);
    layout->addWidget(m_Scroll);
    layout->addSpacing(padding / 2);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    QPushButton *all = new QPushButton(tr("See more"), this);
    connect(all, &QPushButton::clicked, this, [this](){
        if(m_Window && m_Window->GetTreeBank()) m_Window->GetTreeBank()->OpenDownloads();
        close();
    });
    buttons->addWidget(all);
    layout->addLayout(buttons);

    m_RebuildTimer = new QTimer(this);
    m_RebuildTimer->setSingleShot(true);
    m_RebuildTimer->setInterval(DOWNLOAD_PANEL_REBUILD_DELAY);
    connect(m_RebuildTimer, &QTimer::timeout, this, &DownloadPanel::Rebuild);
    connect(DownloadHistory::Instance(), &DownloadHistory::Changed,
            this, &DownloadPanel::RebuildSoon);

    qApp->installEventFilter(this);
    window->installEventFilter(this);
    connect(qApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state){
                if(state != Qt::ApplicationActive) close();
            });
    m_PressPoll = new QTimer(this);
#if defined(Q_OS_WIN)
    m_ButtonDown = (GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON) |
                    GetAsyncKeyState(VK_MBUTTON)) & 0x8000;
    m_EscapeDown = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
    m_PressPoll->setInterval(DOWNLOAD_PANEL_PRESS_POLL);
    connect(m_PressPoll, &QTimer::timeout, this, &DownloadPanel::PollButtons);
    m_PressPoll->start();
#endif

    ApplyTheme();
}

DownloadPanel::~DownloadPanel(){}

void DownloadPanel::Initialize(){
    static bool connected = false;
    if(connected) return;
    connected = true;
    QObject::connect(DownloadHistory::Instance(), &DownloadHistory::EntryStarted, qApp,
                     [](int){
                         if(Application::IsQuitting()) return;
                         if(MainWindow *window = Application::GetCurrentWindow())
                             ShowFor(window, false, false);
                     }, Qt::QueuedConnection);
}

bool DownloadPanel::IsOpenFor(MainWindow *window){
    return window && !Panels().value(window).isNull();
}

void DownloadPanel::ShowFor(MainWindow *window, bool toggle, bool activate){
    if(!window) return;
    for(auto it = Panels().begin(); it != Panels().end();){
        if(it.value()) ++it;
        else it = Panels().erase(it);
    }
    const QPointer<DownloadPanel> shown = Panels().value(window);
    if(shown && shown->isVisible()){
        if(toggle){
            shown->close();
        } else {
            shown->raise();
            if(activate) shown->activateWindow();
        }
        return;
    }
    if(!window->isVisible() || window->isMinimized()) return;
    if(!activate && QGuiApplication::applicationState() != Qt::ApplicationActive) return;
    DownloadPanel *panel = new DownloadPanel(window);
    Panels().insert(window, panel);
    panel->setAttribute(Qt::WA_ShowWithoutActivating, !activate);
    panel->TellBar();
    panel->Rebuild();
    panel->show();
#if defined(Q_OS_WIN)
    {
        const DWORD round = 2;
        DwmSetWindowAttribute(reinterpret_cast<HWND>(panel->winId()),
                              33 ,
                              &round, sizeof(round));
    }
#endif
    panel->raise();
    if(activate){
        panel->activateWindow();
        panel->setFocus();
    }
}

QString DownloadPanel::Size(qint64 bytes){
    const QLocale locale;
    if(bytes < 0) bytes = 0;
    if(bytes < 1024) return tr("%1 B").arg(bytes);
    const qint64 kb = qRound64(bytes / 1024.0);
    if(kb < 1024) return tr("%1 KB").arg(locale.toString(kb));
    const double mb = bytes / (1024.0 * 1024.0);
    if(QString::number(mb, 'f', 1).toDouble() < 1024.0)
        return tr("%1 MB").arg(locale.toString(mb, 'f', 1));
    return tr("%1 GB").arg(locale.toString(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1));
}

bool DownloadPanel::HoldsAPressedButton(QWidget *rows){
    if(!rows) return false;
    foreach(QAbstractButton *button, rows->findChildren<QAbstractButton*>())
        if(button->isDown()) return true;
    return false;
}

QString DownloadPanel::Describe(const DownloadHistory::Entry &entry, bool exists){
    switch(entry.state){
    case DownloadHistory::Running:
        if(entry.total > 0)
            return tr("%1 of %2").arg(Size(entry.received), Size(entry.total));
        return Size(entry.received);
    case DownloadHistory::Completed:
        if(!exists) return tr("Deleted");
        return Size(entry.total > 0 ? entry.total : entry.received)
            + QStringLiteral(" ") + QChar(0x2022) + QStringLiteral(" ") + tr("Completed");
    case DownloadHistory::Cancelled:
        return tr("Cancelled");
    case DownloadHistory::Failed:
        return tr("Failed");
    case DownloadHistory::Interrupted:
        return tr("Interrupted");
    }
    return QString();
}

void DownloadPanel::RebuildSoon(){
    if(!m_RebuildTimer->isActive()) m_RebuildTimer->start();
}

void DownloadPanel::Rebuild(){
    if(HoldsAPressedButton(m_Scroll->widget())){
        m_RebuildTimer->start();
        return;
    }
    DownloadHistory *history = DownloadHistory::Instance();
    QList<DownloadHistory::Entry> entries = history->Entries();
    if(entries.size() > DOWNLOAD_PANEL_ROWS) entries = entries.mid(0, DOWNLOAD_PANEL_ROWS);

    QWidget *inner = new QWidget;
    inner->setAutoFillBackground(false);
    QVBoxLayout *rowsLayout = new QVBoxLayout(inner);
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(0);
    const int inset = ScaleByDevice(DOWNLOAD_PANEL_PADDING) / 2;
    const int rowWidth = width() - 2 * ScaleByDevice(DOWNLOAD_PANEL_PADDING)
        - ScaleByDevice(DOWNLOAD_PANEL_SCROLL_ROOM);
    if(entries.isEmpty()){
        QLabel *none = new QLabel(tr("No downloads"), inner);
        none->setContentsMargins(inset, 0, 0, 0);
        rowsLayout->addWidget(none);
    }
    foreach(const DownloadHistory::Entry &entry, entries){
        const int id = entry.id;
        const QFileInfo info(entry.path);
        DownloadRow *row = new DownloadRow(inner);
        QHBoxLayout *across = new QHBoxLayout(row);
        across->setContentsMargins(inset, inset, inset, inset);
        across->setSpacing(inset);
        QVBoxLayout *lines = new QVBoxLayout();
        lines->setContentsMargins(0, 0, 0, 0);
        lines->setSpacing(0);
        across->addLayout(lines, 1);

        int actionsWidth = 0;
        auto action = [&](const QString &icon, const QString &text, bool hover){
            QToolButton *button = IconButton(row, icon, text);
            across->addWidget(button, 0, Qt::AlignVCenter);
            if(hover) row->AddHoverAction(button);
            actionsWidth += button->sizeHint().width() + across->spacing();
            return button;
        };
        if(entry.state == DownloadHistory::Running){
            QToolButton *cancel = action(QStringLiteral(":/resources/menu/stop.png"), tr("Cancel"), false);
            connect(cancel, &QToolButton::clicked, this, [id](){
                DownloadHistory::Instance()->Cancel(id);
            });
        } else {
            if(DownloadHistory::CanOpen(entry)){
                QToolButton *open = action(QStringLiteral(":/resources/toolbar/open.png"), tr("Open file"), true);
                connect(open, &QToolButton::clicked, this, [this, id](){
                    if(DownloadHistory::Instance()->Open(id)) close();
                });
            }
            if(info.isFile() || QFileInfo(info.absolutePath()).isDir()){
                QToolButton *show = action(QStringLiteral(":/resources/toolbar/folder.png"), tr("Show in folder"), true);
                connect(show, &QToolButton::clicked, this, [this, id](){
                    if(DownloadHistory::Instance()->ShowInFolder(id)) close();
                });
            }
        }
        const int nameWidth = rowWidth - 2 * inset - actionsWidth;

        QLabel *name = new QLabel(row);
        name->setTextFormat(Qt::PlainText);
        name->setText(name->fontMetrics().elidedText(info.fileName(), Qt::ElideMiddle, nameWidth));
        name->setToolTip(QDir::toNativeSeparators(entry.path));
        lines->addWidget(name);

        QLabel *state = new QLabel(row);
        state->setTextFormat(Qt::PlainText);
        state->setText(Describe(entry, info.isFile()));
        {
            QPalette palette = state->palette();
            QColor quiet = Theme::Color(Theme::PreviewText);
            quiet.setAlphaF(DOWNLOAD_PANEL_QUIET_ALPHA);
            palette.setColor(QPalette::WindowText, quiet);
            state->setPalette(palette);
        }
        lines->addWidget(state);

        if(entry.state == DownloadHistory::Running){
            QProgressBar *bar = new QProgressBar(row);
            bar->setTextVisible(false);
            bar->setFixedHeight(ScaleByDevice(DOWNLOAD_PANEL_PROGRESS_HEIGHT));
            if(entry.total > 0){
                bar->setRange(0, DOWNLOAD_PANEL_PROGRESS_STEPS);
                bar->setValue(int(entry.received * DOWNLOAD_PANEL_PROGRESS_STEPS / entry.total));
            } else {
                bar->setRange(0, 0);
            }
            lines->addWidget(bar);
        }
        rowsLayout->addWidget(row);
    }
    rowsLayout->addStretch();

    const int scrolled = m_Scroll->verticalScrollBar()->value();
    if(QWidget *old = m_Scroll->takeWidget()) old->deleteLater();
    m_Scroll->setWidget(inner);
    inner->show();

    const int rows = inner->sizeHint().height();
    m_Scroll->setFixedHeight(qMin(rows, ScaleByDevice(DOWNLOAD_PANEL_MAX_HEIGHT)));
    m_Scroll->verticalScrollBar()->setValue(scrolled);
    adjustSize();
    Place();
}

QWidget *DownloadPanel::Anchor() const {
    if(!m_Window) return nullptr;
    ToolBar *bar = m_Window->GetToolBar();
    if(!bar || !bar->isVisible()) return nullptr;
    DownloadBar *downloads = bar->GetDownloadBar();
    if(!downloads || !downloads->isVisible()) return nullptr;
    return downloads->Button();
}

void DownloadPanel::Place(){
    if(!m_Window) return;
    const QSize size = this->size();
    QPoint point;
    QScreen *screen = nullptr;
    if(QWidget *anchor = Anchor()){
        screen = anchor->screen();
        point = anchor->mapToGlobal(QPoint(anchor->width(), anchor->height()));
        point.rx() -= size.width();
        if(screen && point.y() + size.height() > screen->availableGeometry().bottom())
            point.setY(anchor->mapToGlobal(QPoint()).y() - size.height());
    } else {
        screen = m_Window->screen();
        const int margin = ScaleByDevice(DOWNLOAD_PANEL_MARGIN);
        point = m_Window->mapToGlobal(QPoint(m_Window->width() - size.width() - margin, margin));
    }
    if(screen){
        const QRect area = screen->availableGeometry();
        point.setX(qBound(area.left(), point.x(),
                          qMax(area.left(), area.right() - size.width() + 1)));
        point.setY(qBound(area.top(), point.y(),
                          qMax(area.top(), area.bottom() - size.height() + 1)));
    }
    move(point);
}

bool DownloadPanel::IsOutside(const QPoint &global) const {
    if(frameGeometry().contains(global)) return false;
    if(QWidget *anchor = Anchor())
        if(QRect(anchor->mapToGlobal(QPoint()), anchor->size()).contains(global)) return false;
    return true;
}

void DownloadPanel::PollButtons(){
#if defined(Q_OS_WIN)
    auto state = [](int key){ return GetAsyncKeyState(key);};
    const SHORT left = state(VK_LBUTTON), right = state(VK_RBUTTON), middle = state(VK_MBUTTON);
    const bool down = (left | right | middle) & 0x8000;
    const bool since = (left | right | middle) & 0x0001;
    const bool pressed = (down && !m_ButtonDown) || since;
    m_ButtonDown = down;
    if(pressed && isVisible() && IsOutside(QCursor::pos())){
        close();
        return;
    }
    const SHORT escape = state(VK_ESCAPE);
    const bool escaped = ((escape & 0x8000) && !m_EscapeDown) || (escape & 0x0001);
    m_EscapeDown = escape & 0x8000;
    if(escaped && isVisible() && QGuiApplication::applicationState() == Qt::ApplicationActive) close();
#endif
}

bool DownloadPanel::eventFilter(QObject *watched, QEvent *ev){
    switch(ev->type()){
    case QEvent::MouseButtonPress:
    case QEvent::NonClientAreaMouseButtonPress:
        if(isVisible()){
            const QPoint global = static_cast<QMouseEvent*>(ev)->globalPosition().toPoint();
            if(IsOutside(global)) close();
        }
        return false;
    case QEvent::KeyPress:
        if(isVisible() && static_cast<QKeyEvent*>(ev)->key() == Qt::Key_Escape) close();
        return false;
    default: break;
    }
    if(watched == m_Window){
        switch(ev->type()){
        case QEvent::Move:
        case QEvent::Resize:
            Place();
            break;
        case QEvent::Hide:
        case QEvent::Close:
            close();
            break;
        default: break;
        }
    }
    return false;
}

void DownloadPanel::hideEvent(QHideEvent *ev){
    QWidget::hideEvent(ev);
    if(m_Window && Panels().value(m_Window) == this) Panels().remove(m_Window);
    TellBar();
}

void DownloadPanel::TellBar(){
    if(!m_Window) return;
    ToolBar *bar = m_Window->GetToolBar();
    if(!bar) return;
    if(DownloadBar *downloads = bar->GetDownloadBar()) downloads->Reconsider();
}

void DownloadPanel::keyPressEvent(QKeyEvent *ev){
    if(ev->key() == Qt::Key_Escape){
        close();
        return;
    }
    QWidget::keyPressEvent(ev);
}

void DownloadPanel::ApplyTheme(){
    setPalette(Theme::PanelPalette(palette()));
    if(m_Scroll){
        m_Scroll->setAutoFillBackground(false);
        m_Scroll->viewport()->setAutoFillBackground(false);
        if(m_Scroll->widget()){
            m_Scroll->widget()->setAutoFillBackground(false);
            RebuildSoon();
        }
    }
    update();
}

void DownloadPanel::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)
    QPainter painter(this);
    painter.setPen(Theme::Pen(Theme::PreviewBorder));
    painter.setBrush(Theme::Brush(Theme::PreviewBackground));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}
