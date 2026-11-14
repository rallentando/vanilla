#include "switch.hpp"
#include "const.hpp"
#include "settingspage.hpp"
#include "theme.hpp"

#include "toolbar.hpp"
#include "extensionbar.hpp"
#include "downloadbar.hpp"
#include "siteinfo.hpp"

#include "treebank.hpp"
#include "receiver.hpp"

#include <QLayout>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>
#include <QCompleter>
#include <QStyle>
#include <QStyleOptionToolBar>
#include <QStandardItemModel>
#include <QMouseEvent>
#include <QHBoxLayout>

namespace {
    class AddressGroup : public QWidget {
    public:
        AddressGroup(QWidget *parent, QWidget *site, LineEdit *field)
            : QWidget(parent), m_Site(site), m_Field(field)
        {
            QHBoxLayout *layout = new QHBoxLayout(this);
            layout->setContentsMargins(DeviceScale::FromDpi(SITE_BUTTON_APART, static_cast<int>(logicalDpiY())),
                                       0, 0, 0);
            layout->setSpacing(0);
            site->setParent(this);
            field->setParent(this);
            layout->addWidget(site);
            layout->addWidget(field, 1);
            site->installEventFilter(this);
            field->installEventFilter(this);
        }

    protected:
        void paintEvent(QPaintEvent *ev) override {
            Q_UNUSED(ev)
            QPainter painter(this);
            LineEdit::PaintAddressHover(&painter, this, m_Site, m_Field);
        }
        bool eventFilter(QObject *watched, QEvent *ev) override {
            if(ev->type() == QEvent::Enter || ev->type() == QEvent::Leave) update();
            return QWidget::eventFilter(watched, ev);
        }

    private:
        QWidget *m_Site;
        LineEdit *m_Field;
    };
}

ToolBar::ToolBar(TreeBank *tb, QWidget *parent)
    : QToolBar(tr("ToolBar"), parent)
{
    m_View = SharedView();
    m_TreeBank = tb;

    const int pad = ScaleByDevice(TOOL_BAR_PADDING);
    const int spacing = ScaleByDevice(TOOL_BAR_ICON_SPACING);
    setStyleSheet(QStringLiteral("QToolBar{ padding: %1px; spacing: %2px;}")
                  .arg(pad).arg(spacing));

    addActions(NavigationActions(tb));
    if(View::EnableDestinationInferrer()){
        tb->Action(TreeBank::_Rewind)->setVisible(false);
        tb->Action(TreeBank::_FastForward)->setVisible(false);
    }

    m_LineEdit = new LineEdit(this);
    m_LineEdit->setFont(ToolBarFieldFont());
    m_LineEdit->setMinimumHeight(m_LineEdit->ScaleByDevice(TOOL_BAR_FIELD_HEIGHT));
    m_LineEdit->setReadOnly(true);
    m_LineEdit->setFocusPolicy(Qt::NoFocus);
    m_LineEdit->setCursor(Qt::IBeamCursor);
    m_LineEdit->installEventFilter(this);
    m_LineEdit->setProperty(LineEdit::AddressHoverProperty, true);
    m_LineEdit->ApplyTheme();
    m_SiteButton = new SiteButton(this);
    connect(m_SiteButton, &QToolButton::clicked, this, [this](){
        if(m_SitePanel){
            m_SitePanel->close();
            return;
        }
        if(!m_View) return;
        m_View->TakeKeyboardBack();
        m_SitePanel = SiteInfoPanel::Open(m_SiteButton, m_TreeBank, m_View->url(), m_View->OpenSiteDataLink());
    });
    m_SiteButton->setProperty(LineEdit::AddressHoverProperty, true);
    auto clear = [this](){
        QWidget *space = new QWidget(this);
        space->setFixedWidth(ScaleByDevice(RECEIVER_PADDING));
        addWidget(space);
    };
    m_SiteButton->setFixedWidth(ScaleByDevice(TOOL_BAR_ICON_SIZE + SITE_BUTTON_SIDE));
    addWidget(new AddressGroup(this, m_SiteButton, m_LineEdit));
    clear();
    m_ExtensionBar = new ExtensionBar(this);
    addWidget(m_ExtensionBar);
    m_DownloadBar = new DownloadBar(this);
    QAction *downloads = addWidget(m_DownloadBar);
    downloads->setVisible(m_DownloadBar->IsNeeded());
    connect(m_DownloadBar, &DownloadBar::NeededChanged, downloads, [this, downloads](bool needed){
        downloads->setVisible(needed);
        if(needed) layout()->activate();
    });
    setObjectName(QStringLiteral("ToolBar"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setIconSize(QSize(ScaleByDevice(TOOL_BAR_ICON_SIZE), ScaleByDevice(TOOL_BAR_ICON_SIZE)));

    if(Application::EnableTransparentWindow()){
        setAttribute(Qt::WA_TranslucentBackground);
    }

}

ToolBar::~ToolBar(){}

void ToolBar::Initialize(){
    LoadSettings();
}

void ToolBar::LoadSettings(){
    Settings &s = Application::GlobalSettings();
    Q_UNUSED(s)
}

void ToolBar::SaveSettings(){
    Settings &s = Application::GlobalSettings();
    Q_UNUSED(s)
}

QSize ToolBar::sizeHint() const {
    return QToolBar::sizeHint();
}

QSize ToolBar::minimumSizeHint() const {
    return QSize(0, 0);
}

void ToolBar::Connect(SharedView view){
    m_View = view;
    m_ExtensionBar->SetView(view);
    SetUrl(view->url());
    connect(view->base(), SIGNAL(urlChanged(const QUrl&)),
            this, SLOT(SetUrl(const QUrl&)));
    connect(view->base(), SIGNAL(loadStarted()),
            this, SLOT(SetStarted()));
    connect(view->base(), SIGNAL(loadFinished(bool)),
            this, SLOT(SetFinished(bool)));
}

void ToolBar::Disconnect(SharedView view){
    m_View = SharedView();
    m_ExtensionBar->SetView({});
    disconnect(view->base(), SIGNAL(urlChanged(const QUrl&)),
               this, SLOT(SetUrl(const QUrl&)));
    disconnect(view->base(), SIGNAL(loadStarted()),
               this, SLOT(SetStarted()));
    disconnect(view->base(), SIGNAL(loadFinished(bool)),
               this, SLOT(SetFinished(bool)));
}

void ToolBar::SetUrl(const QUrl &url){
    m_LineEdit->setText(VanillaPage::IsStartUrl(url) ? QString() : url.toString());
    m_LineEdit->setCursorPosition(0);
    if(m_View) SyncActions(m_View->IsLoading());
}

void ToolBar::SetStarted(){
    SyncActions(true);
}

void ToolBar::SetFinished(bool ok){
    Q_UNUSED(ok)
    SyncActions(false);
}

void ToolBar::SyncActions(bool loading){
    SyncNavigation(m_TreeBank, m_View.get(), loading);
}

QList<QAction*> ToolBar::NavigationActions(TreeBank *tb){
    QList<QAction*> actions;
    for(TreeBank::TreeBankAction id : {TreeBank::_Back, TreeBank::_Forward, TreeBank::_Rewind,
                                       TreeBank::_FastForward, TreeBank::_Reload, TreeBank::_Stop})
        actions << tb->Action(id);
    return actions;
}

void ToolBar::SyncNavigation(TreeBank *tb, View *view, bool loading){
    if(!tb || !view) return;
    tb->Action(TreeBank::_Back)->setEnabled(view->CanGoBack());
    tb->Action(TreeBank::_Forward)->setEnabled(view->CanGoForward());
    tb->Action(TreeBank::_Rewind)->setEnabled(view->CanGoBack());
    tb->Action(TreeBank::_FastForward)->setEnabled(true );
    tb->Action(TreeBank::_Reload)->setVisible(!loading);
    tb->Action(TreeBank::_Stop)->setVisible(loading);
}

void ToolBar::paintEvent(QPaintEvent *ev){
    QPainter painter(this);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::Brush(Application::EnableTransparentWindow()
                                  ? Theme::BarBackgroundTranslucent
                                  : Theme::BarBackground));
    foreach(QRect rect, ev->region()){
        painter.drawRect(rect);
    }
    QStyleOptionToolBar opt;
    initStyleOption(&opt);
    const QRect handle = style()->subElementRect(QStyle::SE_ToolBarHandle, &opt, this);
    if(handle.isValid() && ev->region().intersects(handle))
        Theme::DrawBarHandle(&painter, handle, orientation(),
                             ScaleByDevice(BAR_HANDLE_THICKNESS),
                             ScaleByDevice(BAR_HANDLE_LENGTH));
}

void ToolBar::resizeEvent(QResizeEvent *ev){
    QToolBar::resizeEvent(ev);
}

void ToolBar::timerEvent(QTimerEvent *ev){
    QToolBar::timerEvent(ev);
}

void ToolBar::showEvent(QShowEvent *ev){
    QToolBar::showEvent(ev);

    m_View = m_TreeBank->GetCurrentView();
    if(m_View){

        Connect(m_View);
        SyncActions(m_View->IsLoading());
    }
}

void ToolBar::hideEvent(QHideEvent *ev){
    QToolBar::hideEvent(ev);
    if(m_View){
        Disconnect(m_View);
        m_View = SharedView();
    }
}

void ToolBar::enterEvent(QEnterEvent *ev)
{
    QToolBar::enterEvent(ev);
}

void ToolBar::leaveEvent(QEvent *ev){
    QToolBar::leaveEvent(ev);
}

void ToolBar::mouseMoveEvent(QMouseEvent *ev){
    QToolBar::mouseMoveEvent(ev);
}

void ToolBar::mousePressEvent(QMouseEvent *ev){
    QToolBar::mousePressEvent(ev);
}

void ToolBar::mouseReleaseEvent(QMouseEvent *ev){
    QToolBar::mouseReleaseEvent(ev);
}

bool ToolBar::eventFilter(QObject *watched, QEvent *ev){
    if(watched == m_LineEdit && ev->type() == QEvent::MouseButtonPress &&
       static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton){
        if(m_View) m_View->TakeKeyboardBack();
        m_TreeBank->OpenCommandAtField(m_LineEdit);
        return true;
    }
    return QToolBar::eventFilter(watched, ev);
}

LineEdit *ToolBar::Field() const {
    return m_LineEdit;
}

