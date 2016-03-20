#include "switch.hpp"
#include "const.hpp"

#include "toolbar.hpp"

#include "treebank.hpp"
#include "receiver.hpp"

#include <QLineEdit>
#include <QCompleter>

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
    << QStringLiteral("NewViewNode")
    << QStringLiteral("NewHistNode")
    << QStringLiteral("CloneViewNode")
    << QStringLiteral("CloneHistNode")
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

ToolBar::ToolBar(TreeBank *tb, QWidget *parent)
    : QToolBar(tr("ToolBar"), parent)
{
    m_View = SharedView();
    m_TreeBank = tb;
    addAction(m_BackAction = tb->Action(TreeBank::_Back));
    addAction(m_ForwardAction = tb->Action(TreeBank::_Forward));
    addAction(m_ReloadAction = tb->Action(TreeBank::_Reload));
    addAction(m_StopAction = tb->Action(TreeBank::_Stop));
    m_LineEdit = new LineEdit(this);
    m_Model = new QStringListModel(m_CommandCandidates);
    m_Completer = new QCompleter(m_Model);
    m_Completer->setCaseSensitivity(Qt::CaseInsensitive);

    m_LineEdit->setCompleter(m_Completer);

    addWidget(m_LineEdit);
    setObjectName(QStringLiteral("ToolBar"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setIconSize(QSize(16, 16));

    if(Application::EnableTransparentBar()){
        setAttribute(Qt::WA_TranslucentBackground);
    }

    connect(m_LineEdit, &LineEdit::FocusIn,
            [this](){
                if(m_LineEdit->text() == QStringLiteral("Open [text or url]"))
                    m_LineEdit->setSelection(5, 13);
                else if(m_LineEdit->text() == QStringLiteral("Query [text]"))
                    m_LineEdit->setSelection(6, 6);
                else if(m_LineEdit->text() == QStringLiteral("Load [url]"))
                    m_LineEdit->setSelection(5, 5);
                else if(m_LineEdit->text() == QStringLiteral("Search [text]"))
                    m_LineEdit->setSelection(7, 6);
                else if(m_LineEdit->text() == QStringLiteral("Seek [text]"))
                    m_LineEdit->setSelection(5, 6);
                else if(m_LineEdit->text().startsWith("http://") ||
                        m_LineEdit->text().startsWith("https://") ||
                        m_LineEdit->text().startsWith("javascript:") ||
                        m_LineEdit->text().startsWith("about:"))
                    QTimer::singleShot(0, m_LineEdit, &QLineEdit::selectAll);
            });
    connect(m_LineEdit, &QLineEdit::returnPressed,
            [this](){
                if(m_View &&
                   (m_LineEdit->text().startsWith("http://") ||
                    m_LineEdit->text().startsWith("https://") ||
                    m_LineEdit->text().startsWith("javascript:") ||
                    m_LineEdit->text().startsWith("about:"))){

                    m_CommandCandidates.prepend(m_LineEdit->text());
                    m_Model->setStringList(m_CommandCandidates);

                    m_View->Load(m_LineEdit->text());

                } else if(Receiver *receiver = m_TreeBank->GetReceiver()){
                    if(m_LineEdit->text() != QStringLiteral("Open [text or url]") &&
                       m_LineEdit->text() != QStringLiteral("Query [text]") &&
                       m_LineEdit->text() != QStringLiteral("Load [url]") &&
                       m_LineEdit->text() != QStringLiteral("Search [text]") &&
                       m_LineEdit->text() != QStringLiteral("Seek [text]"))

                        receiver->ReceiveCommand(m_LineEdit->text());
                }
            });
}

ToolBar::~ToolBar(){}

void ToolBar::Initialize(){
    LoadSettings();
}

void ToolBar::LoadSettings(){
    QSettings *s = Application::GlobalSettings();
    if(!s->group().isEmpty()) return;

    Q_UNUSED(s);
    // not yet implemented.
}

void ToolBar::SaveSettings(){
    QSettings *s = Application::GlobalSettings();
    if(!s->group().isEmpty()) return;

    Q_UNUSED(s);
    // not yet implemented.
}

QSize ToolBar::sizeHint() const {
    return QToolBar::sizeHint();
}

QSize ToolBar::minimumSizeHint() const {
    return QToolBar::minimumSizeHint();
}

void ToolBar::Connect(SharedView view){
    m_View = view;
    SetUrl(view->url());
    connect(view->base(), SIGNAL(urlChanged(const QUrl&)),
            this, SLOT(SetUrl(const QUrl&)));
    connect(view->base(), SIGNAL(loadFinished(bool)),
            this, SLOT(SetFinished(bool)));
}

void ToolBar::Disconnect(SharedView view){
    m_View = SharedView();
    disconnect(view->base(), SIGNAL(urlChanged(const QUrl&)),
               this, SLOT(SetUrl(const QUrl&)));
    disconnect(view->base(), SIGNAL(loadFinished(bool)),
               this, SLOT(SetFinished(bool)));
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

    if(m_View = m_TreeBank->GetCurrentView()){

        Connect(m_View);
        m_BackAction->setEnabled(m_View->CanGoBack());
        m_ForwardAction->setEnabled(m_View->CanGoForward());
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
