#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "extensionbar.hpp"

#include <functional>

#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QMenu>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QScreen>
#include <QShortcut>
#include <QTimer>
#include <QBuffer>
#include <QImageReader>
#include <QFontMetrics>
#include <QResizeEvent>

#include "dialog.hpp"
#include "treebank.hpp"
#include "extensionhost.hpp"
#include "sidepanels.hpp"
#include "mainwindow.hpp"
#include "settingsschema.hpp"
#ifdef EDGEWEBVIEW
#  include "edgewebview.hpp"
#endif
#ifdef WEBENGINEVIEW
#  include <QQuickWidget>
#endif

class ExtensionWindow : public QWidget {
public:
    ExtensionWindow(QWidget *parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        ApplyTheme();
    }
    ~ExtensionWindow() Q_DECL_OVERRIDE {}

    void ApplyTheme(){
        QPalette palette = this->palette();
        palette.setColor(QPalette::Window,     Theme::Color(Theme::PreviewBackground));
        palette.setColor(QPalette::WindowText, Theme::Color(Theme::PreviewText));
        palette.setColor(QPalette::Base,       Theme::Color(Theme::PreviewBackground));
        palette.setColor(QPalette::Text,       Theme::Color(Theme::PreviewText));
        palette.setColor(QPalette::Button,     Theme::Color(Theme::PreviewBackground));
        palette.setColor(QPalette::ButtonText, Theme::Color(Theme::PreviewText));
        setPalette(palette);
        update();
    }

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE {
        Q_UNUSED(ev)
        QPainter painter(this);
        painter.setPen(Theme::Pen(Theme::PreviewBorder));
        painter.setBrush(Theme::Brush(Theme::PreviewBackground));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
};

namespace {

    QPoint Place(const QSize &size, const QWidget *anchor);

    class ExtensionPopup : public ExtensionWindow {
    public:
        QPointer<QWidget> m_Anchor;
        QPointer<QLabel> m_Title;
        QString m_Name;

        ExtensionPopup(QWidget *parent, bool toolWindow)
            : ExtensionWindow(parent, toolWindow
                              ? Qt::Tool | Qt::FramelessWindowHint
                              : Qt::Popup)
        {
            QShortcut *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
            connect(escape, &QShortcut::activated, this, &QWidget::close);
        }

    protected:
        void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE {
            ExtensionWindow::resizeEvent(ev);
            if(m_Title)
                m_Title->setText(m_Title->fontMetrics().elidedText(m_Name, Qt::ElideRight, qMax(0, m_Title->width())));
            if(m_Anchor && isVisible()) move(Place(size(), m_Anchor));
        }
        bool event(QEvent *ev) Q_DECL_OVERRIDE {
            if(ev->type() == QEvent::WindowDeactivate &&
               windowType() == Qt::Tool && isVisible()){
                close();
                return true;
            }
            return ExtensionWindow::event(ev);
        }
    };

    QPoint Place(const QSize &size, const QWidget *anchor){
        const QRect screen = anchor->screen()->availableGeometry();
        QPoint point = anchor->mapToGlobal(QPoint(anchor->width(), anchor->height()));
        point.rx() -= size.width();
        if(point.y() + size.height() > screen.bottom())
            point.setY(anchor->mapToGlobal(QPoint()).y() - size.height());
        point.setX(qBound(screen.left(), point.x(),
                          qMax(screen.left(), screen.right() - size.width() + 1)));
        point.setY(qBound(screen.top(), point.y(),
                          qMax(screen.top(), screen.bottom() - size.height() + 1)));
        return point;
    }

    void Show(QWidget *window, const QWidget *anchor){
        window->adjustSize();
        window->move(Place(window->size(), anchor));
        window->show();
    }

    QToolButton *Button(QWidget *parent, const QString &text){
        QToolButton *button = new QToolButton(parent);
        button->setText(text);
        button->setToolTip(text);
        button->setAccessibleName(text);
        button->setAutoRaise(true);
        return button;
    }

    QIcon PinIcon(){
        const QPixmap &pin = Theme::Pixmap(QStringLiteral(":/resources/toolbar/pin.png"),
                                           Theme::PreviewText);
        QIcon icon(pin);
        icon.addPixmap(Theme::Fill(pin, Theme::PreviewBackground), QIcon::Normal, QIcon::On);
        return icon;
    }
    QIcon CloseIcon(){
        return QIcon(Theme::Pixmap(QStringLiteral(":/resources/treebar/close.png"),
                                   Theme::PreviewText));
    }

    bool CanOpen(const ExtensionRow &row){
        return row.loaded && row.enabled && row.error.isEmpty() &&
            !row.manifest.popup.isEmpty();
    }
    bool CanClick(const ExtensionRow &row, bool shimmed){
        return row.loaded && row.enabled && row.error.isEmpty() &&
            row.manifest.popup.isEmpty() && shimmed;
    }
    QColor ColorFrom(const QString &folded){
        if(folded.size() != 9) return QColor();
        return QColor(folded.mid(1, 2).toInt(nullptr, 16), folded.mid(3, 2).toInt(nullptr, 16),
                      folded.mid(5, 2).toInt(nullptr, 16), folded.mid(7, 2).toInt(nullptr, 16));
    }
    const int BADGE_ICON_SIZE = 32;
}

static QIcon WithBadge(const QIcon &icon, const QString &text, const QString &background, const QString &foreground){
    if(text.isEmpty()) return icon;
    QPixmap pixmap = icon.pixmap(BADGE_ICON_SIZE, BADGE_ICON_SIZE);
    if(pixmap.isNull()) return icon;
    if(pixmap.size() != QSize(BADGE_ICON_SIZE, BADGE_ICON_SIZE))
        pixmap = pixmap.scaled(BADGE_ICON_SIZE, BADGE_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QColor back = ColorFrom(background);
    if(!back.isValid()) back = QColor(0x5f, 0x63, 0x68);
    QColor fore = ColorFrom(foreground);
    if(!fore.isValid()) fore = back.lightnessF() > 0.6 ? QColor(Qt::black) : QColor(Qt::white);
    const QString shown = text.left(4);
    QFont font;
    font.setPixelSize(BADGE_ICON_SIZE * 5 / 12);
    font.setBold(true);
    const QFontMetrics metrics(font);
    const int height = metrics.height() + 1;
    const int width = qMin(BADGE_ICON_SIZE, metrics.horizontalAdvance(shown) + 4);
    const QRect rect(BADGE_ICON_SIZE - width, BADGE_ICON_SIZE - height, width, height);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(back);
    painter.drawRoundedRect(rect, CHIP_CORNER_RADIUS, CHIP_CORNER_RADIUS);
    painter.setFont(font);
    painter.setPen(fore);
    painter.drawText(rect, Qt::AlignCenter, shown);
    painter.end();
    return QIcon(pixmap);
}

static QIcon IconFromBytes(const QByteArray &bytes){
    if(bytes.isEmpty()) return QIcon();
    QBuffer buffer;
    buffer.setData(bytes);
    if(!buffer.open(QIODevice::ReadOnly)) return QIcon();
    QImageReader reader(&buffer);
    const QSize size = reader.size();
    if(!size.isValid() || size.width() > 2048 || size.height() > 2048) return QIcon();
    reader.setScaledSize(QSize(BADGE_ICON_SIZE, BADGE_ICON_SIZE));
    const QImage image = reader.read();
    return image.isNull() ? QIcon() : QIcon(QPixmap::fromImage(image));
}

QIcon ExtensionBar::PuzzleIcon(){
    return QIcon(Theme::Pixmap(QStringLiteral(":/resources/toolbar/extension.png"),
                               Theme::BarIcon));
}

ExtensionBar::ExtensionBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ExtensionBar"));
    m_Layout = new QHBoxLayout(this);
    m_Layout->setContentsMargins(0, 0, 0, 0);
    m_Layout->setSpacing(ScaleByDevice(TOOL_BAR_ICON_SPACING));
    m_Button = BarButton(tr("Extensions"));
    m_Button->setObjectName(QStringLiteral("ExtensionsButton"));
    m_Button->setIcon(PuzzleIcon());
    m_Layout->addWidget(m_Button);
    connect(m_Button, &QToolButton::clicked, this, &ExtensionBar::ShowList);
}

ExtensionBar::~ExtensionBar(){
    ClosePopup();
}

QToolButton *ExtensionBar::BarButton(const QString &text){
    QToolButton *button = Button(this, text);
    button->setIconSize(QSize(ScaleByDevice(TOOL_BAR_ICON_SIZE),
                              ScaleByDevice(TOOL_BAR_ICON_SIZE)));
    return button;
}

void ExtensionBar::SetView(const SharedView &view){
    disconnect(m_ContextConnection);
    disconnect(m_DestroyConnection);
    disconnect(m_UrlConnection);
    if(m_List) m_List->close();
    ClosePopup();
    if(m_Controller) disconnect(m_Controller, nullptr, this, nullptr);
    m_Controller.clear();
    m_View = view;
    if(view){
        if(view->base()->metaObject()->indexOfSignal("ExtensionContextChanged()") >= 0)
            m_ContextConnection = connect(view->base(), SIGNAL(ExtensionContextChanged()),
                                          this, SLOT(ContextChanged()));
        m_DestroyConnection = connect(view->base(), &QObject::destroyed,
                                      this, &ExtensionBar::ContextChanged);
        if(view->base()->metaObject()->indexOfSignal("urlChanged(QUrl)") >= 0)
            m_UrlConnection = connect(view->base(), SIGNAL(urlChanged(QUrl)), this, SLOT(UrlChanged()));
    }
    Rebind();
}

void ExtensionBar::UrlChanged(){
    Refresh();
}

void ExtensionBar::ContextChanged(){
    ClosePopup();
    if(m_Controller) disconnect(m_Controller, nullptr, this, nullptr);
    m_Controller.clear();
    Refresh();
    QTimer::singleShot(0, this, [this](){ Rebind();});
}

void ExtensionBar::Rebind(){
    const SharedView view = m_View.lock();
    ExtensionController *controller = view ? view->Extensions() : nullptr;
    if(controller != m_Controller){
        ClosePopup();
        if(m_Controller) disconnect(m_Controller, nullptr, this, nullptr);
        m_Controller = controller;
        if(controller){
            connect(controller, &ExtensionController::Changed,
                    this, &ExtensionBar::Refresh, Qt::QueuedConnection);
            connect(controller, &ExtensionController::InvalidatePopups,
                    this, &ExtensionBar::ClosePopup);
            connect(controller, &ExtensionController::ActionChanged,
                    this, &ExtensionBar::ActionChanged, Qt::QueuedConnection);
            connect(controller, &ExtensionController::ActionCommand,
                    this, &ExtensionBar::Execute);
            connect(controller, &QObject::destroyed,
                    this, &ExtensionBar::ContextChanged);
        }
    }
    Refresh();
}

void ExtensionBar::ApplyTheme(){
    if(m_Popup) m_Popup->ApplyTheme();
    if(m_List) m_List->ApplyTheme();
    Refresh();
}

void ExtensionBar::Refresh(){
    foreach(QToolButton *pin, m_Pins){
        m_Layout->removeWidget(pin);
        pin->deleteLater();
    }
    m_Pins.clear();
    m_Button->setIcon(PuzzleIcon());
    if(m_Controller){
        foreach(const ExtensionRow &row, m_Controller->Rows()){
            if(!row.registered || !row.pinned) continue;
            QToolButton *button = BarButton(row.manifest.name);
            button->setObjectName(QStringLiteral("PinnedExtension_") + row.manifest.id);
            Decorate(button, row);
            connect(button, &QToolButton::clicked, this,
                    [this, path = row.manifest.path, popup = !row.manifest.popup.isEmpty()](){ Press(path, popup);});
            button->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(button, &QToolButton::customContextMenuRequested, this,
                    [this, button, path = row.manifest.path](const QPoint &pos){ More(path, button, button->mapToGlobal(pos));});
            m_Layout->insertWidget(m_Layout->count() - 1, button);
            m_Pins.append(button);
        }
    }
    RefreshList();
}

ExtensionUi::ActionShown ExtensionBar::ShownFor(const QString &id) const {
    ExtensionUi::Action *action = m_Controller ? m_Controller->ActionOf(id) : nullptr;
    if(!action) return ExtensionUi::ActionShown();
    ExtensionUi::TabNow tab;
    const SharedView view = m_View.lock();
    if(view && view->GetViewNode()){
        tab.id = static_cast<qint64>(view->GetViewNode()->GetSerial());
        tab.url = view->url();
    }
    return action->Shown(tab);
}

void ExtensionBar::Decorate(QToolButton *button, const ExtensionRow &row){
    const ExtensionUi::ActionShown shown = ShownFor(row.manifest.id);
    QIcon icon = row.manifest.icon.isNull() ? PuzzleIcon() : row.manifest.icon;
    if(shown.hasIcon){
        const QIcon set = IconFromBytes(shown.icon.bytes);
        if(!set.isNull()) icon = set;
    }
    button->setIcon(WithBadge(icon, shown.badgeText, shown.badgeColor, shown.textColor));
    const bool shimmed = m_Controller && m_Controller->ShimmedIds().contains(row.manifest.id);
    const bool busy = !m_Controller || m_Controller->IsBusy();
    button->setEnabled(!busy && shown.enabled && (CanOpen(row) || CanClick(row, shimmed)));
    if(row.manifest.popup.isEmpty() && !shimmed)
        button->setToolTip(tr("This extension has no popup. Browser action clicks are not exposed by this engine."));
    else
        button->setToolTip(shown.hasTitle ? shown.title
                           : !row.manifest.title.isEmpty() ? row.manifest.title : row.manifest.name);
}

void ExtensionBar::Execute(const QString &id, qint64 serial){
    const SharedView view = OwnView();
    if(!view || m_Controller->IsBusy()) return;
    if(!view->GetViewNode() || static_cast<qint64>(view->GetViewNode()->GetSerial()) != serial) return;
    foreach(const ExtensionRow &row, m_Controller->Rows()){
        if(row.manifest.id != id) continue;
        if(!ShownFor(id).enabled) return;
        if(CanOpen(row)) Open(row.manifest.path);
        else if(CanClick(row, m_Controller->ShimmedIds().contains(id))) Click(row.manifest.path);
        return;
    }
}

void ExtensionBar::Pressed(const QString &path){
    const SharedView view = OwnView();
    if(!view || !view->GetViewNode()) return;
    foreach(const ExtensionRow &row, m_Controller->Rows())
        if(row.manifest.path == path)
            ExtensionHost::Invoked(m_Controller, row.manifest.id, static_cast<qint64>(view->GetViewNode()->GetSerial()));
}

void ExtensionBar::Press(const QString &path, bool popup){
    Pressed(path);
    if(PanelInstead(path)) return;
    if(popup) Open(path);
    else Click(path);
}

SharedView ExtensionBar::OwnView() const {
    const SharedView view = m_View.lock();
    return view && m_Controller && view->Extensions() == m_Controller ? view : SharedView();
}

bool ExtensionBar::PanelInstead(const QString &path){
    const SharedView view = OwnView();
    if(!view || !view->MakesSidePanels()) return false;
    MainWindow *window = qobject_cast<MainWindow*>(this->window());
    if(!window || !window->GetSidePanels()) return false;
    foreach(const ExtensionRow &row, m_Controller->Rows()){
        if(row.manifest.path != path) continue;
        if(!m_Controller->HasPermission(row.manifest.id, QStringLiteral("sidePanel")) ||
           !m_Controller->SidePanelFor(row.manifest.id).OpensOnAction()) return false;
        if(m_List) m_List->close();
        window->GetSidePanels()->Toggle(m_Controller, row.manifest.id);
        return true;
    }
    return false;
}

void ExtensionBar::ActionChanged(const QString &id){
    Q_UNUSED(id)
    Refresh();
}

void ExtensionBar::Click(const QString &path){
    const SharedView view = OwnView();
    if(!view || m_Controller->IsBusy()) return;
    foreach(const ExtensionRow &row, m_Controller->Rows()){
        if(row.manifest.path != path || !CanClick(row, m_Controller->ShimmedIds().contains(row.manifest.id))) continue;
        if(m_List) m_List->close();
        const qint64 tab = view->GetViewNode() ? static_cast<qint64>(view->GetViewNode()->GetSerial()) : 0;
        if(tab > 0) m_Controller->ClickAction(row.manifest.id, tab);
        return;
    }
}

void ExtensionBar::ShowList(){
    if(m_List && m_List->isVisible()){
        m_List->close();
        return;
    }
    ClosePopup();
    ExtensionWindow *panel = new ExtensionWindow(this, Qt::Popup);
    m_List = panel;
    panel->setObjectName(QStringLiteral("ExtensionsPanel"));
    panel->setFixedWidth(ScaleByDevice(EXTENSION_LIST_WIDTH));

    const int padding = ScaleByDevice(EXTENSION_PANEL_PADDING);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(padding, padding, padding, padding);

    QLabel *title = new QLabel(tr("Extensions"), panel);
    title->setFont(ExtensionListTitleFont());
    layout->addWidget(title);

    QLabel *note = new QLabel(tr("Registration, enabled state and pins are shared by normal profiles.\n"
                                 "Reload the page after a change."),
                              panel);
    note->setWordWrap(true);
    layout->addWidget(note);

    layout->addSpacing(padding / 2);
    m_Rows = new QScrollArea(panel);
    m_Rows->setWidgetResizable(true);
    m_Rows->setFrameShape(QFrame::NoFrame);
    m_Rows->viewport()->setAutoFillBackground(false);
    layout->addWidget(m_Rows);
    layout->addSpacing(padding / 2);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    QPushButton *add = new QPushButton(tr("Add unpacked extension…"), panel);
    connect(add, &QPushButton::clicked, this, &ExtensionBar::AddDirectory);
    buttons->addWidget(add);
    QPushButton *manage = new QPushButton(tr("Manage extensions"), panel);
    connect(manage, &QPushButton::clicked, this, &ExtensionBar::ManageExtensions);
    buttons->addWidget(manage);
    layout->addLayout(buttons);

    RefreshList();
    Show(panel, m_Button);
}

void ExtensionBar::ManageExtensions(){
    if(m_List) m_List->close();
    const SharedView view = m_View.lock();
    TreeBank *tb = view ? view->GetTreeBank() : nullptr;
    const SettingsSchema::Item *row =
        SettingsSchema::Find(QStringLiteral("network/@Extensions"));
    if(tb) tb->OpenSettings(row ? QString::fromLatin1(row->category) : QString());
}

void ExtensionBar::RefreshList(){
    if(!m_List || !m_Rows) return;

    QWidget *old = m_Rows->takeWidget();
    if(old) old->deleteLater();

    QWidget *body = new QWidget;
    body->setAutoFillBackground(false);
    QVBoxLayout *layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);

    const int indent = body->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, body) + 2;
    auto message = [body, layout, indent](const QString &text){
        QLabel *label = new QLabel(text, body);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        label->setContentsMargins(indent, 0, 0, 0);
        layout->addWidget(label);
    };

    if(!m_Controller){
        const SharedView view = m_View.lock();
        message(view ? view->ExtensionStatus()
                     : tr("Select a web page to use extensions."));
    } else {
        const bool busy = m_Controller->IsBusy();
        if(!m_Controller->Error().isEmpty()) message(m_Controller->Error());
        if(busy) message(tr("Applying extension changes…"));
        if(!m_Controller->EngineNote().isEmpty()) message(m_Controller->EngineNote());

        const QList<ExtensionRow> rows = m_Controller->Rows();
        if(rows.isEmpty()) message(tr("No extensions are registered."));

        foreach(const ExtensionRow &row, rows){
            const QString path = row.manifest.path;
            QHBoxLayout *line = new QHBoxLayout;

            QToolButton *open = Button(body, row.manifest.name);
            open->setText(open->fontMetrics().elidedText(row.manifest.name, Qt::ElideRight,
                                                         ScaleByDevice(EXTENSION_ROW_NAME_WIDTH)));
            open->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            open->setIconSize(QSize(ScaleByDevice(TOOL_BAR_ICON_SIZE),
                                    ScaleByDevice(TOOL_BAR_ICON_SIZE)));
            Decorate(open, row);
            if(!row.manifest.popup.isEmpty()) open->setToolTip(path);
            connect(open, &QToolButton::clicked, this,
                    [this, path, popup = !row.manifest.popup.isEmpty()](){ Press(path, popup);});
            line->addWidget(open);
            line->addStretch(1);

            QCheckBox *enabled = new QCheckBox(body);
            enabled->setObjectName(QStringLiteral("EnableExtension_") + row.manifest.id);
            enabled->setAccessibleName(tr("Enable %1").arg(row.manifest.name));
            enabled->setToolTip(tr("Enabled"));
            enabled->setChecked(row.enabled);
            enabled->setEnabled(row.registered && row.error.isEmpty() &&
                                !busy && m_Controller->Error().isEmpty());
            connect(enabled, &QCheckBox::toggled, this,
                    [path](bool on){ ExtensionController::SetEnabled(path, on);});
            line->addWidget(enabled);

            QToolButton *pin = Button(body, row.pinned ? tr("Unpin") : tr("Pin"));
            pin->setIcon(PinIcon());
            pin->setCheckable(true);
            pin->setChecked(row.pinned);
            pin->setEnabled(row.registered);
            connect(pin, &QToolButton::clicked, this,
                    [path](bool on){ ExtensionController::SetPinned(path, on);});
            line->addWidget(pin);

            QToolButton *more = Button(body, tr("More options"));
            more->setObjectName(QStringLiteral("MoreExtension_") + row.manifest.id);
            more->setArrowType(Qt::DownArrow);
            connect(more, &QToolButton::clicked, this,
                    [this, path, more](){ More(path, more);});
            line->addWidget(more);

            layout->addLayout(line);
            if(!row.error.isEmpty()) message(row.error + QLatin1Char('\n') + path);
            else if(!row.note.isEmpty()) message(row.note);
            else if(!row.registered) message(tr("Removing registration…"));
        }
    }
    m_Rows->setWidget(body);

    const QMargins margins = m_List->layout()->contentsMargins();
    const int width = m_List->width() - margins.left() - margins.right();
    const int wanted = body->hasHeightForWidth() ? body->heightForWidth(width)
                                                 : body->sizeHint().height();
    const int height = qMin(ScaleByDevice(EXTENSION_LIST_ROWS_MAX_HEIGHT), wanted);
    m_Rows->setFixedHeight(qMax(m_Rows->fontMetrics().height(), height));
    if(m_List->isVisible()) Show(m_List, m_Button);
}

void ExtensionBar::ClosePopup(){
    if(m_Popup){
        m_Popup->close();
        m_Popup.clear();
    }
}

void ExtensionBar::Open(const QString &path, bool options){
    const SharedView view = OwnView();
    if(!view || m_Controller->IsBusy()) return;

    foreach(const ExtensionRow &row, m_Controller->Rows()){
        if(row.manifest.path != path ||
           !row.loaded || !row.enabled || !row.error.isEmpty()) continue;
        const QUrl url = options ? row.manifest.options : row.manifest.popup;
        if(url.isEmpty()) return;

        if(m_List) m_List->close();
        ClosePopup();

        bool toolWindow = false;
#ifdef WEBENGINEVIEW
        toolWindow = qobject_cast<QQuickWidget*>(view->base()) != nullptr;
#endif
#ifdef EDGEWEBVIEW
        toolWindow = toolWindow || qobject_cast<EdgeWebView*>(view->base()) != nullptr;
#endif
        QPointer<ExtensionPopup> panel = new ExtensionPopup(this, toolWindow);
        panel->setObjectName(QStringLiteral("ExtensionPopup"));

        QVBoxLayout *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(0);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        const int padding = ScaleByDevice(EXTENSION_PANEL_PADDING / 2);
        QHBoxLayout *header = new QHBoxLayout;
        header->setContentsMargins(padding, padding, padding, padding);
        QLabel *title = new QLabel(panel);
        title->setTextFormat(Qt::PlainText);
        title->setText(title->fontMetrics().elidedText(
                           row.manifest.name, Qt::ElideRight,
                           ScaleByDevice(EXTENSION_POPUP_SIZE.width()) - padding * 4));
        title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        panel->m_Title = title;
        panel->m_Name = row.manifest.name;
        header->addWidget(title, 1);
        QToolButton *closeButton = Button(panel, tr("Close"));
        closeButton->setIcon(CloseIcon());
        connect(closeButton, &QToolButton::clicked, panel, &QWidget::close);
        header->addWidget(closeButton);
        layout->addLayout(header);

        m_Popup = panel;
        QWidget *content = view->CreateExtensionView(
            url, options ? View::ExtensionOptionsPage : View::ExtensionActionPage, panel);
        if(!panel) return;
        if(m_Popup != panel){
            delete panel;
            return;
        }
        if(!content){
            delete panel;
            m_Popup.clear();
            return;
        }
        content->setMinimumSize(ScaleByDevice(EXTENSION_POPUP_SIZE.width()),
                                ScaleByDevice(EXTENSION_POPUP_SIZE.height()));
        layout->addWidget(content);

        panel->m_Anchor = m_Button;
        Show(panel, m_Button);
        if(toolWindow) panel->activateWindow();
        content->setFocus();
        return;
    }
}

void ExtensionBar::More(const QString &path, QToolButton *button, const QPoint &at){
    if(!m_Controller) return;
    foreach(const ExtensionRow &row, m_Controller->Rows()){
        if(row.manifest.path != path) continue;
        QMenu *menu = new QMenu(button);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        QString name = row.manifest.name;
        name.replace(QLatin1Char('&'), QStringLiteral("&&"));
        const QUrl homepage = row.manifest.homepage;
        QAction *home = menu->addAction(name, this, [this, homepage](){
                const SharedView view = m_View.lock();
                TreeBank *tb = view ? view->GetTreeBank() : nullptr;
                if(tb) tb->OpenInNewViewNode(homepage, true, view->GetViewNode());
            });
        home->setEnabled(!homepage.isEmpty());
        menu->addSeparator();

        const SharedView view = m_View.lock();
        const ExtensionUi::Menus *menus = m_Controller->MenusOf(row.manifest.id);
        if(view && view->Extensions() == m_Controller && menus && row.loaded && row.enabled
           && m_Controller->HasPermission(row.manifest.id, QStringLiteral("contextMenus"))){
            const QUrl page = view->url();
            const QList<ExtensionUi::MenuShown> shown = menus->ActionShown(page);
            const qint64 tab = view->GetViewNode() ? static_cast<qint64>(view->GetViewNode()->GetSerial()) : 0;
            const QPointer<ExtensionController> weak = m_Controller;
            const QString id = row.manifest.id;
            const ExtensionUi::MenuContext context = ExtensionUi::Menus::ActionContext(page);
            std::function<void(QMenu*, const QList<ExtensionUi::MenuShown>&)> add;
            add = [&add, weak, id, context, tab](QMenu *into, const QList<ExtensionUi::MenuShown> &items){
                foreach(const ExtensionUi::MenuShown &one, items){
                    if(one.item.type == QStringLiteral("separator")){ into->addSeparator(); continue; }
                    if(!one.children.isEmpty()){
                        QMenu *sub = into->addMenu(one.title);
                        sub->setEnabled(one.item.enabled);
                        add(sub, one.children);
                        continue;
                    }
                    QAction *action = into->addAction(one.title);
                    action->setEnabled(one.item.enabled);
                    if(one.item.type == QStringLiteral("checkbox") || one.item.type == QStringLiteral("radio")){
                        action->setCheckable(true);
                        action->setChecked(one.item.checked);
                    }
                    const QString itemId = one.item.id;
                    QObject::connect(action, &QAction::triggered, into, [weak, id, itemId, context, tab](){
                        if(weak) weak->ClickMenu(id, itemId, context, tab);
                    });
                }
            };
            add(menu, shown);
            if(!shown.isEmpty()) menu->addSeparator();
        }

        QAction *options = menu->addAction(tr("Extension settings"), this,
                                           [this, path](){ Open(path, true);});
        options->setEnabled(row.loaded && row.enabled &&
                            !row.manifest.options.isEmpty() && !m_Controller->IsBusy());

        const QPointer<ExtensionController> controller = m_Controller;
        QAction *retry = menu->addAction(tr("Retry"), this, [controller, path](){
                if(controller) controller->Retry(path);
            });
        retry->setEnabled(!m_Controller->IsBusy() && m_Controller->Error().isEmpty());

        menu->addAction(tr("Remove from Vanilla"), this, [path](){
                ExtensionController::UnregisterPath(path);
            });
        const bool pinned = row.pinned;
        QAction *pin = menu->addAction(pinned ? tr("Unpin") : tr("Pin"), this, [path, pinned](){
                ExtensionController::SetPinned(path, !pinned);
            });
        pin->setEnabled(row.registered);

        menu->addSeparator();
        menu->addAction(tr("Manage extensions"), this, &ExtensionBar::ManageExtensions);
        menu->popup(at.isNull() ? button->mapToGlobal(QPoint(0, button->height())) : at);
        return;
    }
}

void ExtensionBar::AddDirectory(){
    if(m_List) m_List->close();
    const QString path =
        ModalDialog::GetExistingDirectory(tr("Select the folder containing manifest.json"),
                                          ExtensionController::DefaultPickDirectory());
    if(!path.isEmpty()) ExtensionController::RegisterPath(path);
}
