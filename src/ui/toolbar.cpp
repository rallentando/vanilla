#include "switch.hpp"
#include "const.hpp"
#include "settingspage.hpp"
#include "theme.hpp"

#include "toolbar.hpp"
#include "extensionbar.hpp"
#include "downloadbar.hpp"

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
#include "searchsuggest.hpp"

QStringList ToolBar::m_CommandCandidates = QStringList()
    << QStringLiteral("Open [text or url]")
    << QStringLiteral("Query [text]")
    << QStringLiteral("Load [url]")
    << QStringLiteral("Search [text]")
    << QStringLiteral("Seek [text]")
    << QStringLiteral("Up")
    << QStringLiteral("Down")
    << QStringLiteral("Right")
    << QStringLiteral("Left")
    << QStringLiteral("Home")
    << QStringLiteral("End")
    << QStringLiteral("PageUp")
    << QStringLiteral("PageDown")
    << QStringLiteral("Back")
    << QStringLiteral("Forward")
    << QStringLiteral("Rewind")
    << QStringLiteral("FastForward")
    << QStringLiteral("UpDirectory")
    << QStringLiteral("Close")
    << QStringLiteral("Recreate")
    << QStringLiteral("CloneViewNode")
    << QStringLiteral("DisplayAccessKey")
    << QStringLiteral("OpenTextSeeker")
    << QStringLiteral("Load")
    << QStringLiteral("Copy")
    << QStringLiteral("Cut")
    << QStringLiteral("Paste")
    << QStringLiteral("Undo")
    << QStringLiteral("Redo")
    << QStringLiteral("SelectAll")
    << QStringLiteral("Unselect")
    << QStringLiteral("Reload")
    << QStringLiteral("ReloadAndBypassCache")
    << QStringLiteral("Stop")
    << QStringLiteral("StopAndUnselect")
    << QStringLiteral("Print")
    << QStringLiteral("Save")
    << QStringLiteral("ZoomIn")
    << QStringLiteral("ZoomOut")
    << QStringLiteral("ViewSource")
    << QStringLiteral("ApplySource")
    << QStringLiteral("InspectElement")
    << QStringLiteral("CopyUrl")
    << QStringLiteral("CopyTitle")
    << QStringLiteral("CopyPageAsLink")
    << QStringLiteral("CopySelectedHtml")
    << QStringLiteral("OpenWithDefault");

QStringList ToolBar::m_InputUrlHistory = QStringList();

namespace {
    class CompleterPopup : public QTableView {
    protected:
        int sizeHintForRow(int) const override {
            return verticalHeader()->defaultSectionSize();
        }
    };

    bool IsUrlToLoadInPlace(const QString &text){
        static const QStringList prefixes = QStringList()
            << QStringLiteral("http://")
            << QStringLiteral("https://")
            << QStringLiteral("javascript:")
            << QStringLiteral("about:")
            << QStringLiteral("chrome:")
            << QStringLiteral("edge:")
            << VANILLA_SCHEME + QStringLiteral(":");
        foreach(const QString &prefix, prefixes){
            if(text.startsWith(prefix)) return true;
        }
        return false;
    }
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

    addAction(m_BackAction = tb->Action(TreeBank::_Back));
    addAction(m_ForwardAction = tb->Action(TreeBank::_Forward));
    addAction(m_RewindAction = tb->Action(TreeBank::_Rewind));
    addAction(m_FastForwardAction = tb->Action(TreeBank::_FastForward));
    addAction(m_ReloadAction = tb->Action(TreeBank::_Reload));
    addAction(m_StopAction = tb->Action(TreeBank::_Stop));
    if(View::EnableDestinationInferrer()){
        m_RewindAction->setVisible(false);
        m_FastForwardAction->setVisible(false);
    }
    m_LineEdit = new LineEdit(this);
    m_LineEdit->setFont(ToolBarFieldFont());
    m_LineEdit->setMinimumHeight(ScaleByDevice(TOOL_BAR_FIELD_HEIGHT));
    m_Model = new QStandardItemModel();
    m_Completer = new QCompleter(m_Model);
    m_Completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_Completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

    QTableView *popup = new CompleterPopup();
    popup->setFont(ToolBarFieldFont());
    popup->horizontalHeader()->hide();
    popup->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    popup->verticalHeader()->hide();
    popup->verticalHeader()->setDefaultSectionSize(ScaleByDevice(24));

    m_Completer->setPopup(popup);

    m_LineEdit->setCompleter(m_Completer);
    m_LineEdit->installEventFilter(this);

    addWidget(m_LineEdit);
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

    connect(m_LineEdit, &LineEdit::FocusIn,
            [this](Qt::FocusReason reason){
                QString text = m_LineEdit->text();
                if(text == QStringLiteral("Open [text or url]"))
                    m_LineEdit->setSelection(5, 13);
                else if(text == QStringLiteral("Query [text]"))
                    m_LineEdit->setSelection(6, 6);
                else if(text == QStringLiteral("Load [url]"))
                    m_LineEdit->setSelection(5, 5);
                else if(text == QStringLiteral("Search [text]"))
                    m_LineEdit->setSelection(7, 6);
                else if(text == QStringLiteral("Seek [text]"))
                    m_LineEdit->setSelection(5, 6);
                else if(reason != Qt::PopupFocusReason && IsUrlToLoadInPlace(text))
                    QTimer::singleShot(0, m_LineEdit, &QLineEdit::selectAll);
            });
    connect(m_LineEdit, &LineEdit::Returned,
            [this](){
                QString text = m_LineEdit->text();
                if(!(Application::keyboardModifiers() & Qt::ShiftModifier) &&
                   m_View && IsUrlToLoadInPlace(text)){

                    m_InputUrlHistory << text;
                    m_View->Load(text);

                } else if(Receiver *receiver = m_TreeBank->GetReceiver()){
                    if(text != QStringLiteral("Open [text or url]") &&
                       text != QStringLiteral("Query [text]") &&
                       text != QStringLiteral("Load [url]") &&
                       text != QStringLiteral("Search [text]") &&
                       text != QStringLiteral("Seek [text]"))

                        receiver->ReceiveCommand(m_LineEdit->text());
                }
            });
    connect(m_LineEdit, &LineEdit::Aborted,
            [this](){
                clearFocus();
                m_Completer->popup()->hide();
                if(m_View){
                    m_View->setFocus();
                    SetUrl(m_View->url());
                } else {
                    SetUrl(QUrl());
                }
            });
    connect(m_LineEdit, &QLineEdit::textChanged,
            [this](const QString &text){

                if(!m_Model->findItems(text).isEmpty()) return;
                FillCandidates(text);
            });
}

void ToolBar::FillCandidates(const QString &text){
    const QIcon blank(Theme::Pixmap(QStringLiteral(":/resources/blank.png"), Theme::PreviewText));
    const QIcon command(Theme::Pixmap(QStringLiteral(":/resources/toolbar/command.png"), Theme::PreviewText));

    m_Model->clear();

    foreach(QString cmd, m_CommandCandidates){
        if(cmd.startsWith(text, Qt::CaseInsensitive)){
            QStandardItem *item = new QStandardItem(cmd);
            item->setIcon(command);
            m_Model->appendRow(item);
        }
    }
    QStringList nearby = m_InputUrlHistory;
    if(m_View && m_View->GetViewNode()){
        foreach(Node *nd, m_View->GetViewNode()->GetSiblings())
            nearby << nd->GetUrl().toString();
    }
    foreach(QString url, UrlCandidates(text, m_View ? m_View->url() : QUrl(), nearby)){
        QStandardItem *item = new QStandardItem(url);
        QIcon icon = Application::GetIcon(QUrl(url));
        if(icon.isNull() || icon.availableSizes().first().width() <= 2)
            icon = blank;
        item->setIcon(icon);
        m_Model->appendRow(item);
    }
    if(!m_Model->rowCount() && Application::EnableGoogleSuggest() &&
       !text.startsWith(QStringLiteral("http://")) &&
       !text.startsWith(QStringLiteral("https://")) &&
       !text.startsWith(QStringLiteral("javascript:")) &&
       !text.startsWith(QStringLiteral("about:")) &&
       !text.startsWith(QStringLiteral("Open "), Qt::CaseInsensitive) &&
       !text.startsWith(QStringLiteral("Query "), Qt::CaseInsensitive) &&
       !text.startsWith(QStringLiteral("Load "), Qt::CaseInsensitive) &&
       !text.startsWith(QStringLiteral("Search "), Qt::CaseInsensitive) &&
       !text.startsWith(QStringLiteral("Seek "), Qt::CaseInsensitive) &&
       !text.startsWith(QStringLiteral("Set"), Qt::CaseInsensitive) &&
       !text.startsWith(QStringLiteral("Unset"), Qt::CaseInsensitive)){

        emit SuggestRequest(SearchSuggest::Url(Application::SuggestEngine(), text));
    }
}

QStringList ToolBar::CommandCandidates(){
    return m_CommandCandidates;
}

QStringList ToolBar::UrlCandidates(const QString &text, const QUrl &current, const QStringList &nearby){
    QStringList list = UrlAncestors(text, current);
    for(const QString &url : nearby){
        if(url.startsWith(text, Qt::CaseInsensitive) && url != current.toString()) list << url;
    }
    list.removeDuplicates();
    return list;
}

QStringList ToolBar::UrlAncestors(const QString &text, const QUrl &current){
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
    connect(view->base(), SIGNAL(loadFinished(bool)),
            this, SLOT(SetFinished(bool)));
    connect(this, SIGNAL(SuggestRequest(const QUrl&)),
            view->page(), SLOT(DownloadSuggest(const QUrl&)));
    connect(view->page(), SIGNAL(SuggestResult(const QByteArray&)),
            this, SLOT(DisplaySuggest(const QByteArray&)));
}

void ToolBar::Disconnect(SharedView view){
    m_View = SharedView();
    m_ExtensionBar->SetView({});
    disconnect(view->base(), SIGNAL(urlChanged(const QUrl&)),
               this, SLOT(SetUrl(const QUrl&)));
    disconnect(view->base(), SIGNAL(loadFinished(bool)),
               this, SLOT(SetFinished(bool)));
    disconnect(this, SIGNAL(SuggestRequest(const QUrl&)),
               view->page(), SLOT(DownloadSuggest(const QUrl&)));
    disconnect(view->page(), SIGNAL(SuggestResult(const QByteArray&)),
               this, SLOT(DisplaySuggest(const QByteArray&)));
}

void ToolBar::SetUrl(const QUrl &url){
    m_LineEdit->setText(VanillaPage::IsStartUrl(url) ? QString() : url.toString());
    if(m_View) SyncActions(m_View->IsLoading());
}

void ToolBar::SetFinished(bool ok){
    Q_UNUSED(ok)
    SyncActions(false);
}

void ToolBar::SyncActions(bool loading){
    m_BackAction->setEnabled(m_View->CanGoBack());
    m_ForwardAction->setEnabled(m_View->CanGoForward());
    m_RewindAction->setEnabled(m_View->CanGoBack());
    m_FastForwardAction->setEnabled(true );
    m_ReloadAction->setVisible(!loading);
    m_StopAction->setVisible(loading);
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
    if(watched == m_LineEdit && ev->type() == QEvent::KeyPress &&
       static_cast<QKeyEvent*>(ev)->key() == Qt::Key_Down && !m_Completer->popup()->isVisible() && m_View){
        FillCandidates(m_LineEdit->text());
        if(m_Model->rowCount()){
            m_Completer->setCompletionPrefix(m_LineEdit->text());
            m_Completer->complete();
            return true;
        }
    }
    if(watched == m_LineEdit && ev->type() == QEvent::MouseButtonPress && m_View)
        m_View->TakeKeyboardBack();
    return QToolBar::eventFilter(watched, ev);
}

void ToolBar::DisplaySuggest(const QByteArray &ba){

    const QIcon search(Theme::Pixmap(QStringLiteral(":/resources/toolbar/search.png"), Theme::PreviewText));

    if(!m_LineEdit->hasFocus()) return;

    m_Model->clear();

    foreach(QString cand, SearchSuggest::Parse(ba)){
        QStandardItem *item = new QStandardItem(cand);
        item->setIcon(search);
        m_Model->appendRow(item);
    }
    if(m_Model->rowCount()) m_Completer->popup()->show();
}
