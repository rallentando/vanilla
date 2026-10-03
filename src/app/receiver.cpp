#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "receiver.hpp"

#include <QLineEdit>
#include <QHideEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QPointer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDomDocument>
#include <QDomNodeList>
#include <QDomElement>
#include <QUrl>
#include <QUrlQuery>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "gadgets.hpp"
#include "notifier.hpp"
#include "view.hpp"
#include "commandmap.hpp"
#include "commandframe.hpp"

QLocalServer *Receiver::m_LocalServer = nullptr;

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    ApplyTheme();
    setDragEnabled(true);
}

LineEdit::~LineEdit(){}

void LineEdit::ApplyTheme(){
    const QString active = Theme::StyleSheetColor(Theme::LineEditBackgroundActive);

    QString border;
    QString borderActive;
#if defined(Q_OS_MAC)
    border = QStringLiteral("border: 1px solid %1;")
        .arg(Theme::StyleSheetColor(Theme::LineEditBorder));
    borderActive = QStringLiteral("border: 1px solid %1;")
        .arg(Theme::StyleSheetColor(Theme::LineEditBorderActive));
#endif

    setStyleSheet(QStringLiteral("QLineEdit:!focus{ background: transparent; %1}"
                                 "QLineEdit:focus{ background: %2; %3}"
                                 "QLineEdit:hover{ background: %2; %3}"
                                 "QLineEdit{ border-radius: %4px; padding: %5px %6px;}")
                  .arg(border, active, borderActive)
                  .arg(ScaleByDevice(FIELD_CORNER_RADIUS))
                  .arg(ScaleByDevice(FIELD_PADDING))
                  .arg(ScaleByDevice(FIELD_PADDING_X)));
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
    emit textChanged(text() + ev->preeditString());
}

void LineEdit::mousePressEvent(QMouseEvent *ev){
    if(selectedText() == text()) deselect();
    QLineEdit::mousePressEvent(ev);
}

void LineEdit::mouseReleaseEvent(QMouseEvent *ev){
    QLineEdit::mouseReleaseEvent(ev);
}

void LineEdit::mouseMoveEvent(QMouseEvent *ev){
    QLineEdit::mouseMoveEvent(ev);
}

Receiver::Receiver(TreeBank *parent, bool purge)
    : QWidget(purge ? nullptr : parent)
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
    m_LineEdit->show();
    hide();

    m_LineString = QString();
    m_SuggestStrings = QStringList();
    m_CurrentSuggestIndex = -1;

    connect(m_LineEdit, &LineEdit::FocusIn, m_LineEdit, &QLineEdit::selectAll);
    connect(m_LineEdit, &LineEdit::textChanged,        this, &Receiver::SetString);
    connect(m_LineEdit, &LineEdit::editingFinished,    this, &Receiver::EditingFinished);
    connect(m_LineEdit, &LineEdit::SelectNextSuggest,  this, &Receiver::SelectNextSuggest);
    connect(m_LineEdit, &LineEdit::SelectPrevSuggest,  this, &Receiver::SelectPrevSuggest);
    connect(m_LineEdit, &LineEdit::Returned,           this, &Receiver::OnReturned);
    connect(m_LineEdit, &LineEdit::Aborted,            this, &Receiver::OnAborted);
}

Receiver::~Receiver(){
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
    MakeOwnedWindow();
}

void Receiver::Join(){
    bool v = isVisible();
    setParent(m_TreeBank);
    if(v) show();
    else hide();
}

void Receiver::ResizeNotify(QSize size){
    TakeWindowOwnerIfNeed();
    Notifier *notifier = m_TreeBank->GetNotifier();

    int len = m_SuggestStrings.length();

    int nw = notifier
        ? qMax(size.width() * NOTIFIER_WIDTH_PERCENTAGE / 100,
               ScaleByDevice(NOTIFIER_MINIMUM_WIDTH))
        : 0;

    int w = size.width() - nw;
    int h = ScaleByDevice(RECEIVER_HEIGHT) + len * ScaleByDevice(SUGGEST_HEIGHT);
    QPoint pos;
    if(notifier){
        switch(notifier->m_Position){
        case Notifier::NorthWest:
            pos = QPoint(nw, 0);
            break;
        case Notifier::NorthEast:
            pos = QPoint(0, 0);
            break;
        case Notifier::SouthWest:
            pos = QPoint(nw, size.height() - h);
            break;
        case Notifier::SouthEast:
            pos = QPoint(0, size.height() - h);
            break;
        }
    } else {
        pos = QPoint(0, size.height() - h);
    }
    if(IsPurged()) pos = m_TreeBank->mapToGlobal(pos);
    setGeometry(QRect(pos, QSize(w, h)));
    m_LineEdit->setGeometry(0, h - ScaleByDevice(LINEEDIT_HEIGHT),
                            width(), ScaleByDevice(LINEEDIT_HEIGHT));
}

void Receiver::RepaintIfNeed(const QRect &rect){
    if(isVisible() && rect.intersects(geometry()))
        repaint(rect.intersected(geometry()).translated(-pos()));
}

void Receiver::InitializeDisplay(Mode mode){
    bool v = isVisible();

    if(mode == Search){
        if(m_Mode != Search){
            m_LineEdit->setText(QString());
            m_LineString = QString();
            m_SuggestStrings = QStringList();
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
        m_CurrentSuggestIndex = -1;
    }

    m_Mode = mode;

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

void Receiver::OpenTextSeeker(View*){
    InitializeDisplay(Search);
}

void Receiver::OpenQueryEditor(View*){
    InitializeDisplay(Query);
}

void Receiver::OpenUrlEditor(View*){
    InitializeDisplay(UrlEdit);
}

void Receiver::OpenCommand(View*){
    InitializeDisplay(Command);
}

void Receiver::OnReturned(){
    SuitableAction();
}

void Receiver::OnAborted(){
    if(m_Mode == Search){
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

    ReceiveCommand(command);
}

void Receiver::SetString(QString str){
    m_LineString = str;

    if(m_Mode == Search){
        emit SeekText(m_LineString, View::HighlightAllOccurrences);
        repaint();
    }
    if(m_Mode == Query && Application::EnableGoogleSuggest()){
        QUrl base = QUrl(tr("https://www.google.com/complete/search"));
        QUrlQuery param;
        param.addQueryItem(QStringLiteral("q"), m_LineString);
        param.addQueryItem(QStringLiteral("output"), QStringLiteral("toolbar"));
        param.addQueryItem(QStringLiteral("hl"), tr("en"));
        base.setQuery(param);
        emit SuggestRequest(base);
    }
    QStringList list = str.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
    if(m_Mode == Command &&
       Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?"), list[0])){
        Settings &s = Application::GlobalSettings();

        switch (list.length()){
        case 1:
            list << QString();
            [[clang::fallthrough]];
        case 2:{
            QString command = list[0];
            QString query = list[1];
            QStringList path = query.split(QStringLiteral("/"));
            QSet<QString> cand;
            foreach(QString key, s.allKeys(query)){
                QStringList split = key.split(QStringLiteral("/"));
                while(split.length() >= path.length() + 1)
                    split.removeLast();
                if(split.join(QStringLiteral("/")) != key)
                    split << QString();
                cand << command + QStringLiteral(" ") + split.join(QStringLiteral("/"));
            }
            QStringList result = cand.values();
            std::sort(result.begin(), result.end(), std::greater<QString>());
            SetSuggest(result);
            break;
        }
        default:{
            SetSuggest(QStringList());
            break;
        }
        }
    }
}

void Receiver::SetSuggest(QStringList list){
    m_CurrentSuggestIndex = -1;
    m_SuggestStrings.clear();
    m_SuggestStrings = list;
    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Receiver::SuitableAction(){
    switch (m_Mode){
    case Query:{
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1 &&
           m_SuggestStrings[m_CurrentSuggestIndex] != m_LineEdit->text())
            m_LineEdit->setText(m_SuggestStrings[m_CurrentSuggestIndex]);
        else
            emit OpenQueryUrl(m_LineString);
        break;
    }
    case UrlEdit:
        emit OpenUrl(Page::DirtyStringToUrls(m_LineString == QStringLiteral("blank")
                                             ? QStringLiteral("about:blank")
                                             : m_LineString));
        break;
    case Search:
        if(m_TreeBank->IsDisplayingTableView()){
            emit SeekText(m_LineString, View::WrapsAroundDocument);
            hide();
        } else {
            if(Application::keyboardModifiers() & Qt::ShiftModifier){
                emit SeekText(m_LineString, View::WrapsAroundDocument | View::FindBackward);
            } else {
                emit SeekText(m_LineString, View::WrapsAroundDocument);
            }
        }
        break;
    case Command:{
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            QString key = m_SuggestStrings[m_CurrentSuggestIndex];
            QStringList list = key.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));

            if(Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?"), list[0])){
                Settings &s = Application::GlobalSettings();
                if(!s.value(list[1]).toString().isEmpty())
                    list << s.value(list[1]).toString();
                m_LineEdit->setText(list.join(" "));
            }
        } else {
            ReceiveCommand(m_LineString);
            hide();
        }
        break;
    }
    }
}

void Receiver::EditingFinished(){
    if(m_Mode == Command){
        return;
    }

    if(m_Mode == Query){
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            return;
        } else {
            hide();
            return;
        }
    }

    if(m_Mode == UrlEdit){
        hide();
        return;
    }

    if(m_Mode == Search){
        return;
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
            QString key = list.takeFirst();
            QString val = list.join(" ");
            Application::GlobalSettings().setValue(key, val);
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
    if(!m_SuggestStrings.isEmpty()){
        if(m_CurrentSuggestIndex > 0)
            m_CurrentSuggestIndex -= 1;
        else
            m_CurrentSuggestIndex = m_SuggestStrings.length() - 1;
        repaint();
    }
}

void Receiver::SelectPrevSuggest(){
    if(!m_SuggestStrings.isEmpty()){
        if(m_CurrentSuggestIndex < (m_SuggestStrings.length() - 1))
            m_CurrentSuggestIndex += 1;
        else
            m_CurrentSuggestIndex = 0;
        repaint();
    }
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

    const int len = m_SuggestStrings.length();
    const int suggestHeight  = ScaleByDevice(SUGGEST_HEIGHT);
    const int receiverHeight = ScaleByDevice(RECEIVER_HEIGHT);
    const int lineEditHeight = ScaleByDevice(LINEEDIT_HEIGHT);
    const int pad = ScaleByDevice(STATUS_TEXT_PADDING);

    const auto TextRect = [&](int top, int height){
        return QRect(pad, top, width() - pad * 2, height);
    };
    const auto Elided = [&painter](const QString &text, int width){
        return painter.fontMetrics().elidedText(text, Qt::ElideRight, width);
    };

    for(int i = len; i > 0; i--){
        const QRect back_rect = QRect(-1, (len-i) * suggestHeight - 1,
                                      width()+1, suggestHeight);
        painter.setBrush(Theme::Brush(m_CurrentSuggestIndex == i - 1
                                      ? Theme::ReceiverBackgroundContrast
                                      : Theme::ReceiverBackground));
        painter.setPen(Qt::NoPen);
        painter.drawRect(back_rect);

        const QRect text_rect = TextRect((len-i) * suggestHeight, suggestHeight);
        painter.setPen(Theme::Pen(m_CurrentSuggestIndex == i - 1
                                  ? Theme::ReceiverTextContrast
                                  : Theme::ReceiverText));
        painter.setBrush(Qt::NoBrush);
        painter.drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter,
                         Elided(m_SuggestStrings[i-1], text_rect.width()));
    }

    const QRect back_rect = QRect(-1, len * suggestHeight -1,
                                  width()+1, receiverHeight);
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::ReceiverBackground));
        painter.drawRect(back_rect);
    }

    const QRect text_rect = TextRect(len * suggestHeight, receiverHeight - lineEditHeight);
    {
        painter.setPen(Theme::Pen(Theme::ReceiverText));
        painter.setBrush(Qt::NoBrush);
        const int flags = Qt::AlignLeft | Qt::AlignVCenter;
        switch(m_Mode){
        case Command  : painter.drawText(text_rect, flags, tr("Command Mode.")); break;
        case Query    : painter.drawText(text_rect, flags, tr("Input Query.")); break;
        case UrlEdit  : painter.drawText(text_rect, flags, tr("Input Url.")); break;
        case Search   : painter.drawText(text_rect, flags,
                                         Elided(tr("Incremental Search") + QStringLiteral(" : \"")
                                                + m_LineString + QStringLiteral("\"."),
                                                text_rect.width())); break;
        }
    }

    ev->setAccepted(true);
}

void Receiver::hideEvent(QHideEvent *ev){
    MainWindow *win = m_TreeBank->GetMainWindow();

    const bool purged = IsPurged() && isActiveWindow();
    QPointer<Receiver> alive(this);
    QTimer::singleShot(0, win, [win, purged, alive](){
        if(alive && alive->isVisible()) return;
        if(!win->isVisible() || win->isMinimized()) return;
        if(purged || win->isActiveWindow()) win->SetFocus();
    });
    QWidget::hideEvent(ev);
}

void Receiver::showEvent(QShowEvent *ev){
    ResizeNotify(m_TreeBank->size());

    if(m_Mode == Search && !m_LineString.isEmpty()){
        emit SeekText(m_LineString, View::HighlightAllOccurrences);
    }
    m_LineEdit->show();
    m_LineEdit->raise();
    QWidget::showEvent(ev);
}

void Receiver::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
    ev->setAccepted(true);
}

void Receiver::mousePressEvent(QMouseEvent *ev){
    if(m_Mode == Query){
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            emit OpenQueryUrl(m_SuggestStrings[m_CurrentSuggestIndex]);
            hide();
        }
    }
    if(m_Mode == Command){
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            QString key = m_SuggestStrings[m_CurrentSuggestIndex];
            QStringList list = key.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));

            if(Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?"), list[0])){
                Settings &s = Application::GlobalSettings();
                if(!s.value(list[1]).toString().isEmpty())
                    list << s.value(list[1]).toString();
                m_LineEdit->setText(list.join(" "));
            } else {
                m_LineEdit->setText(key);
            }
            m_LineEdit->setFocus();
        }
    }
    QWidget::mousePressEvent(ev);
}

void Receiver::mouseReleaseEvent(QMouseEvent *ev){
    QWidget::mouseReleaseEvent(ev);
}

void Receiver::mouseMoveEvent(QMouseEvent *ev){
    if(m_Mode == Query){
        if(!m_SuggestStrings.isEmpty()){
            m_CurrentSuggestIndex = m_SuggestStrings.length() - ev->pos().y() / ScaleByDevice(SUGGEST_HEIGHT) - 1;
            if(m_CurrentSuggestIndex >= m_SuggestStrings.length() ||
               m_CurrentSuggestIndex < 0)
                m_CurrentSuggestIndex = -1;
            repaint();
        }
    }
    QStringList list = m_LineString.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
    if(m_Mode == Command &&
       Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?"), list[0])){
        if(!m_SuggestStrings.isEmpty()){
            m_CurrentSuggestIndex = m_SuggestStrings.length() - ev->pos().y() / ScaleByDevice(SUGGEST_HEIGHT) - 1;
            if(m_CurrentSuggestIndex >= m_SuggestStrings.length() ||
               m_CurrentSuggestIndex < 0)
                m_CurrentSuggestIndex = -1;
            repaint();
        }
    }
    QWidget::mouseMoveEvent(ev);
}

void Receiver::DisplaySuggest(const QByteArray &ba){

    if(!m_LineEdit->hasFocus()) return;

    if(m_LineString.isEmpty()){
        SetSuggest(QStringList());
        return;
    }
    QDomDocument doc;
    if(doc.setContent(ba)){
        QStringList list;
        QDomNodeList nodelist = doc.elementsByTagName(QStringLiteral("suggestion"));
        for(int i = 0; i < nodelist.length(); i++){
            list << nodelist.item(i).toElement().attribute(QStringLiteral("data"));
        }
        SetSuggest(list);
    }
}
