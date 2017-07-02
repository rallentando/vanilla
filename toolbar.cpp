#include "switch.hpp"
#include "const.hpp"

#include "toolbar.hpp"

#include "treebank.hpp"
#include "receiver.hpp"

#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>
#include <QCompleter>
#include <QStyle>
#include <QStyleOptionToolBar>
#include <QStandardItemModel>
#include <QDomDocument>
#include <QDomNodeList>
#include <QDomElement>

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
    << QStringLiteral("Import")
    << QStringLiteral("Export")
    << QStringLiteral("AboutVanilla")
    << QStringLiteral("AboutQt")
    << QStringLiteral("Quit")
    << QStringLiteral("ToggleNotifier")
    << QStringLiteral("ToggleReceiver")
    << QStringLiteral("ToggleMenuBar")
    << QStringLiteral("ToggleTreeBar")
    << QStringLiteral("ToggleToolBar")
    << QStringLiteral("ToggleFullScreen")
    << QStringLiteral("ToggleMaximized")
    << QStringLiteral("ToggleMinimized")
    << QStringLiteral("ToggleShaded")
    << QStringLiteral("ShadeWindow")
    << QStringLiteral("UnshadeWindow")
    << QStringLiteral("NewWindow")
    << QStringLiteral("CloseWindow")
    << QStringLiteral("SwitchWindow")
    << QStringLiteral("NextWindow")
    << QStringLiteral("PrevWindow")
    << QStringLiteral("Back")
    << QStringLiteral("Forward")
    << QStringLiteral("Rewind")
    << QStringLiteral("FastForward")
    << QStringLiteral("UpDirectory")
    << QStringLiteral("Close")
    << QStringLiteral("Restore")
    << QStringLiteral("Recreate")
    << QStringLiteral("NextView")
    << QStringLiteral("PrevView")
    << QStringLiteral("BuryView")
    << QStringLiteral("DigView")
    << QStringLiteral("FirstView")
    << QStringLiteral("SecondView")
    << QStringLiteral("ThirdView")
    << QStringLiteral("FourthView")
    << QStringLiteral("FifthView")
    << QStringLiteral("SixthView")
    << QStringLiteral("SeventhView")
    << QStringLiteral("EighthView")
    << QStringLiteral("NinthView")
    << QStringLiteral("TenthView")
    << QStringLiteral("LastView")
    << QStringLiteral("NewViewNode")
    << QStringLiteral("NewHistNode")
    << QStringLiteral("CloneViewNode")
    << QStringLiteral("CloneHistNode")
    << QStringLiteral("MakeLocalNode")
    << QStringLiteral("DisplayAccessKey")
    << QStringLiteral("DisplayViewTree")
    << QStringLiteral("DisplayHistTree")
    << QStringLiteral("DisplayTrashTree")
    << QStringLiteral("OpenTextSeeker")
    << QStringLiteral("OpenQueryEditor")
    << QStringLiteral("OpenUrlEditor")
    << QStringLiteral("OpenCommand")
    << QStringLiteral("ReleaseHiddenView")
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
    << QStringLiteral("OpenBookmarklet")
    << QStringLiteral("SearchWith")
    << QStringLiteral("AddSearchEngine")
    << QStringLiteral("AddBookmarklet")
    << QStringLiteral("InspectElement")
    << QStringLiteral("CopyUrl")
    << QStringLiteral("CopyTitle")
    << QStringLiteral("CopyPageAsLink")
    << QStringLiteral("CopySelectedHtml")
    << QStringLiteral("OpenWithIE")
    << QStringLiteral("OpenWithEdge")
    << QStringLiteral("OpenWithFF")
    << QStringLiteral("OpenWithOpera")
    << QStringLiteral("OpenWithOPR")
    << QStringLiteral("OpenWithSafari")
    << QStringLiteral("OpenWithChrome")
    << QStringLiteral("OpenWithSleipnir")
    << QStringLiteral("OpenWithVivaldi")
    << QStringLiteral("OpenWithCustom");

QStringList ToolBar::m_InputUrlHistory = QStringList();

ToolBar::ToolBar(TreeBank *tb, QWidget *parent)
    : QToolBar(tr("ToolBar"), parent)
{
    m_View = SharedView();
    m_TreeBank = tb;

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
    m_Model = new QStandardItemModel();
    m_Completer = new QCompleter(m_Model);
    m_Completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_Completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

    QTableView *popup = new QTableView();
    popup->horizontalHeader()->hide();
    popup->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    popup->verticalHeader()->hide();
    popup->verticalHeader()->setDefaultSectionSize(20);

    m_Completer->setPopup(popup);

    m_LineEdit->setCompleter(m_Completer);

    addWidget(m_LineEdit);
    setObjectName(QStringLiteral("ToolBar"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setIconSize(QSize(16, 16));

    if(Application::EnableTransparentBar()){
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
                else if(reason != Qt::PopupFocusReason &&
                        (text.startsWith(QStringLiteral("http://")) ||
                         text.startsWith(QStringLiteral("https://")) ||
                         text.startsWith(QStringLiteral("javascript:")) ||
                         text.startsWith(QStringLiteral("about:"))))
                    QTimer::singleShot(0, m_LineEdit, &QLineEdit::selectAll);
            });
    connect(m_LineEdit, &LineEdit::Returned,
            [this](){
                QString text = m_LineEdit->text();
                if(!(Application::keyboardModifiers() & Qt::ShiftModifier) &&
                   m_View &&
                   (text.startsWith(QStringLiteral("http://")) ||
                    text.startsWith(QStringLiteral("https://")) ||
                    text.startsWith(QStringLiteral("javascript:")) ||
                    text.startsWith(QStringLiteral("about:")))){

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

                static QIcon blank = QIcon(":/resources/blank.png");
                static QIcon command = QIcon(":/resources/toolbar/command.png");

                if(!m_Model->findItems(text).isEmpty()) return;

                m_Model->clear();

                foreach(QString cmd, m_CommandCandidates){
                    if(cmd.startsWith(text, Qt::CaseInsensitive)){
                        QStandardItem *item = new QStandardItem(cmd);
                        item->setIcon(command);
                        m_Model->appendRow(item);
                    }
                }
                foreach(QString url, m_InputUrlHistory){
                    if(url.startsWith(text, Qt::CaseInsensitive)){
                        QStandardItem *item = new QStandardItem(url);
                        QIcon icon = Application::GetIcon(QUrl(url).host());
                        if(icon.isNull() || icon.availableSizes().first().width() <= 2)
                            icon = blank;
                        item->setIcon(icon);
                        m_Model->appendRow(item);
                    }
                }
                if(m_View && m_View->GetViewNode()){
                    foreach(Node *nd, m_View->GetViewNode()->GetSiblings()){
                        QString url = nd->GetUrl().toString();
                        if(url.startsWith(text, Qt::CaseInsensitive)){
                            QStandardItem *item = new QStandardItem(url);
                            QIcon icon = Application::GetIcon(QUrl(url).host());
                            if(icon.isNull() || icon.availableSizes().first().width() <= 2)
                                icon = blank;
                            item->setIcon(icon);
                            m_Model->appendRow(item);
                        }
                    }
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

                    QUrl base = QUrl(tr("https://www.google.com/complete/search"));
                    QUrlQuery param;
                    param.addQueryItem(QStringLiteral("q"), text);
                    param.addQueryItem(QStringLiteral("output"), QStringLiteral("toolbar"));
                    param.addQueryItem(QStringLiteral("hl"), tr("en"));
                    base.setQuery(param);
                    emit SuggestRequest(base);
                }
            });
}

ToolBar::~ToolBar(){}

void ToolBar::Initialize(){
    LoadSettings();
}

void ToolBar::LoadSettings(){
    Settings &s = Application::GlobalSettings();
    Q_UNUSED(s);
    // not yet implemented.
}

void ToolBar::SaveSettings(){
    Settings &s = Application::GlobalSettings();
    Q_UNUSED(s);
    // not yet implemented.
}

QSize ToolBar::sizeHint() const {
    return QToolBar::sizeHint();
}

QSize ToolBar::minimumSizeHint() const {
    return QSize(0, 0);
}

void ToolBar::Connect(SharedView view){
    m_View = view;
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
    disconnect(view->base(), SIGNAL(urlChanged(const QUrl&)),
               this, SLOT(SetUrl(const QUrl&)));
    disconnect(view->base(), SIGNAL(loadFinished(bool)),
               this, SLOT(SetFinished(bool)));
    disconnect(this, SIGNAL(SuggestRequest(const QUrl&)),
               view->page(), SLOT(DownloadSuggest(const QUrl&)));
    disconnect(view->page(), SIGNAL(SuggestResult(const QByteArray&)),
               this, SLOT(DisplaySuggest(const QByteArray&)));
}

QMenu *ToolBar::ToolBarMenu(){
    QMenu *menu = new QMenu(this);
    return menu;
}

void ToolBar::SetUrl(const QUrl &url){
    m_LineEdit->setText(url.toString());
    if(m_View){
        m_BackAction->setEnabled(m_View->CanGoBack());
        m_ForwardAction->setEnabled(m_View->CanGoForward());
        m_RewindAction->setEnabled(m_View->CanGoBack());
        m_FastForwardAction->setEnabled(true /*m_View->CanGoForward()*/);
        if(m_View->IsLoading()){
            m_ReloadAction->setVisible(false);
            m_StopAction->setVisible(true);
        } else {
            m_ReloadAction->setVisible(true);
            m_StopAction->setVisible(false);
        }
    }
}

void ToolBar::SetFinished(bool ok){
    Q_UNUSED(ok);
    m_BackAction->setEnabled(m_View->CanGoBack());
    m_ForwardAction->setEnabled(m_View->CanGoForward());
    m_RewindAction->setEnabled(m_View->CanGoBack());
    m_FastForwardAction->setEnabled(true /*m_View->CanGoForward()*/);
    m_ReloadAction->setVisible(true);
    m_StopAction->setVisible(false);
}

void ToolBar::paintEvent(QPaintEvent *ev){
    QPainter painter(this);

    static const QBrush tb = QBrush(QColor(0, 0, 0, 1));
    static const QBrush nb = QBrush(QColor(240, 240, 240, 255));
    painter.setPen(Qt::NoPen);
    painter.setBrush(Application::EnableTransparentBar() ? tb : nb);
    foreach(QRect rect, ev->region().rects()){
        painter.drawRect(rect);
    }
    QStyle *s = style();
    QStyleOptionToolBar opt;
    initStyleOption(&opt);
    opt.rect = s->subElementRect(QStyle::SE_ToolBarHandle, &opt, this);
    if(opt.rect.isValid() && ev->region().contains(opt.rect))
        s->drawPrimitive(QStyle::PE_IndicatorToolBarHandle, &opt, &painter, this);
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
        m_BackAction->setEnabled(m_View->CanGoBack());
        m_ForwardAction->setEnabled(m_View->CanGoForward());
        m_RewindAction->setEnabled(m_View->CanGoBack());
        m_FastForwardAction->setEnabled(true /*m_View->CanGoForward()*/);
        if(m_View->IsLoading()){
            m_ReloadAction->setVisible(false);
            m_StopAction->setVisible(true);
        } else {
            m_ReloadAction->setVisible(true);
            m_StopAction->setVisible(false);
        }
    }
}

void ToolBar::hideEvent(QHideEvent *ev){
    QToolBar::hideEvent(ev);
    if(m_View){
        Disconnect(m_View);
        m_View = SharedView();
    }
}

void ToolBar::enterEvent(QEvent *ev){
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

void ToolBar::DisplaySuggest(const QByteArray &ba){

    static QIcon search = QIcon(":/resources/toolbar/search.png");

    if(!m_LineEdit->hasFocus()) return;

    m_Model->clear();

    QDomDocument doc;
    if(doc.setContent(ba)){
        QDomNodeList nodelist = doc.elementsByTagName(QStringLiteral("suggestion"));
        for(uint i = 0; i < static_cast<uint>(nodelist.length()); i++){
            QString cand = nodelist.item(i).toElement().attribute(QStringLiteral("data"));
            QStandardItem *item = new QStandardItem(cand);
            item->setIcon(search);
            m_Model->appendRow(item);
        }
        if(m_Model->rowCount()) m_Completer->popup()->show();
    }
}
