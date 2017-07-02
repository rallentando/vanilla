#include "switch.hpp"
#include "const.hpp"

#include "receiver.hpp"

#include <QLineEdit>
#include <QHideEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QTimerEvent>
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

QLocalServer *Receiver::m_LocalServer = 0;

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setStyleSheet("QLineEdit:!focus{ background: transparent}"
                  "QLineEdit:focus{ background: white}"
                  "QLineEdit:hover{ background: white}");
    setDragEnabled(true);
}

LineEdit::~LineEdit(){}

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
    : QWidget(purge ? 0 : parent)
{
    if(purge){
        setWindowFlags(Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    m_TreeBank = parent;

    if(!m_LocalServer){
        m_LocalServer = new QLocalServer();
        m_LocalServer->listen(Application::LocalServerName());
    }
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

    connect(this, SIGNAL(Up()),               parent, SLOT(Up()));
    connect(this, SIGNAL(Down()),             parent, SLOT(Down()));
    connect(this, SIGNAL(Right()),            parent, SLOT(Right()));
    connect(this, SIGNAL(Left()),             parent, SLOT(Left()));
    connect(this, SIGNAL(Home()),             parent, SLOT(Home()));
    connect(this, SIGNAL(End()),              parent, SLOT(End()));
    connect(this, SIGNAL(PageUp()),           parent, SLOT(PageUp()));
    connect(this, SIGNAL(PageDown()),         parent, SLOT(PageDown()));

    connect(this, SIGNAL(Import()),           parent, SLOT(Import()));
    connect(this, SIGNAL(Export()),           parent, SLOT(Export()));
    connect(this, SIGNAL(AboutVanilla()),     parent, SLOT(AboutVanilla()));
    connect(this, SIGNAL(AboutQt()),          parent, SLOT(AboutQt()));
    connect(this, SIGNAL(Quit()),             parent, SLOT(Quit()));

    connect(this, SIGNAL(ToggleNotifier()),   parent, SLOT(ToggleNotifier()));
    connect(this, SIGNAL(ToggleReceiver()),   parent, SLOT(ToggleReceiver()));
    connect(this, SIGNAL(ToggleMenuBar()),    parent, SLOT(ToggleMenuBar()));
    connect(this, SIGNAL(ToggleTreeBar()),    parent, SLOT(ToggleTreeBar()));
    connect(this, SIGNAL(ToggleToolBar()),    parent, SLOT(ToggleToolBar()));
    connect(this, SIGNAL(ToggleFullScreen()), parent, SLOT(ToggleFullScreen()));
    connect(this, SIGNAL(ToggleMaximized()),  parent, SLOT(ToggleMaximized()));
    connect(this, SIGNAL(ToggleMinimized()),  parent, SLOT(ToggleMinimized()));
    connect(this, SIGNAL(ToggleShaded()),     parent, SLOT(ToggleShaded()));
    connect(this, SIGNAL(ShadeWindow()),      parent, SLOT(ShadeWindow()));
    connect(this, SIGNAL(UnshadeWindow()),    parent, SLOT(UnshadeWindow()));
    connect(this, SIGNAL(NewWindow()),        parent, SLOT(NewWindow()));
    connect(this, SIGNAL(CloseWindow()),      parent, SLOT(CloseWindow()));
    connect(this, SIGNAL(SwitchWindow()),     parent, SLOT(SwitchWindow()));
    connect(this, SIGNAL(NextWindow()),       parent, SLOT(NextWindow()));
    connect(this, SIGNAL(PrevWindow()),       parent, SLOT(PrevWindow()));

    connect(this, SIGNAL(Back()),             parent, SLOT(Back()));
    connect(this, SIGNAL(Forward()),          parent, SLOT(Forward()));
    connect(this, SIGNAL(Rewind()),           parent, SLOT(Rewind()));
    connect(this, SIGNAL(FastForward()),      parent, SLOT(FastForward()));
    connect(this, SIGNAL(UpDirectory()),      parent, SLOT(UpDirectory()));
    connect(this, SIGNAL(Close()),            parent, SLOT(Close()));
    connect(this, SIGNAL(Restore()),          parent, SLOT(Restore()));
    connect(this, SIGNAL(Recreate()),         parent, SLOT(Recreate()));
    connect(this, SIGNAL(NextView()),         parent, SLOT(NextView()));
    connect(this, SIGNAL(PrevView()),         parent, SLOT(PrevView()));
    connect(this, SIGNAL(BuryView()),         parent, SLOT(BuryView()));
    connect(this, SIGNAL(DigView()),          parent, SLOT(DigView()));
    connect(this, SIGNAL(FirstView()),        parent, SLOT(FirstView()));
    connect(this, SIGNAL(SecondView()),       parent, SLOT(SecondView()));
    connect(this, SIGNAL(ThirdView()),        parent, SLOT(ThirdView()));
    connect(this, SIGNAL(FourthView()),       parent, SLOT(FourthView()));
    connect(this, SIGNAL(FifthView()),        parent, SLOT(FifthView()));
    connect(this, SIGNAL(SixthView()),        parent, SLOT(SixthView()));
    connect(this, SIGNAL(SeventhView()),      parent, SLOT(SeventhView()));
    connect(this, SIGNAL(EighthView()),       parent, SLOT(EighthView()));
    connect(this, SIGNAL(NinthView()),        parent, SLOT(NinthView()));
    connect(this, SIGNAL(TenthView()),        parent, SLOT(TenthView()));
    connect(this, SIGNAL(LastView()),         parent, SLOT(LastView()));
    connect(this, SIGNAL(NewViewNode()),      parent, SLOT(NewViewNode()));
    connect(this, SIGNAL(NewHistNode()),      parent, SLOT(NewHistNode()));
    connect(this, SIGNAL(CloneViewNode()),    parent, SLOT(CloneViewNode()));
    connect(this, SIGNAL(CloneHistNode()),    parent, SLOT(CloneHistNode()));
    connect(this, SIGNAL(MakeLocalNode()),    parent, SLOT(MakeLocalNode()));
    connect(this, SIGNAL(DisplayAccessKey()), parent, SLOT(DisplayAccessKey()));
    connect(this, SIGNAL(DisplayViewTree()),  parent, SLOT(DisplayViewTree()));
    connect(this, SIGNAL(DisplayHistTree()),  parent, SLOT(DisplayHistTree()));
    connect(this, SIGNAL(DisplayTrashTree()), parent, SLOT(DisplayTrashTree()));
    connect(this, SIGNAL(ReleaseHiddenView()),parent, SLOT(ReleaseHiddenView()));

    connect(this, SIGNAL(Copy()),             parent, SLOT(Copy()));
    connect(this, SIGNAL(Cut()),              parent, SLOT(Cut()));
    connect(this, SIGNAL(Paste()),            parent, SLOT(Paste()));
    connect(this, SIGNAL(Undo()),             parent, SLOT(Undo()));
    connect(this, SIGNAL(Redo()),             parent, SLOT(Redo()));
    connect(this, SIGNAL(SelectAll()),        parent, SLOT(SelectAll()));
    connect(this, SIGNAL(Unselect()),         parent, SLOT(Unselect()));
    connect(this, SIGNAL(Reload()),           parent, SLOT(Reload()));
    connect(this, SIGNAL(ReloadAndBypassCache()), parent, SLOT(ReloadAndBypassCache()));
    connect(this, SIGNAL(Stop()),             parent, SLOT(Stop()));
    connect(this, SIGNAL(StopAndUnselect()),  parent, SLOT(StopAndUnselect()));

    connect(this, SIGNAL(Print()),            parent, SLOT(Print()));
    connect(this, SIGNAL(Save()),             parent, SLOT(Save()));
    connect(this, SIGNAL(ZoomIn()),           parent, SLOT(ZoomIn()));
    connect(this, SIGNAL(ZoomOut()),          parent, SLOT(ZoomOut()));
    connect(this, SIGNAL(ViewSource()),       parent, SLOT(ViewSource()));
    connect(this, SIGNAL(ApplySource()),      parent, SLOT(ApplySource()));

    connect(this, SIGNAL(InspectElement()),   parent, SLOT(InspectElement()));

    connect(this, SIGNAL(CopyUrl()),          parent, SLOT(CopyUrl()));
    connect(this, SIGNAL(CopyTitle()),        parent, SLOT(CopyTitle()));
    connect(this, SIGNAL(CopyPageAsLink()),   parent, SLOT(CopyPageAsLink()));
    connect(this, SIGNAL(CopySelectedHtml()), parent, SLOT(CopySelectedHtml()));
    connect(this, SIGNAL(OpenWithIE()),       parent, SLOT(OpenWithIE()));
    connect(this, SIGNAL(OpenWithEdge()),     parent, SLOT(OpenWithEdge()));
    connect(this, SIGNAL(OpenWithFF()),       parent, SLOT(OpenWithFF()));
    connect(this, SIGNAL(OpenWithOpera()),    parent, SLOT(OpenWithOpera()));
    connect(this, SIGNAL(OpenWithOPR()),      parent, SLOT(OpenWithOPR()));
    connect(this, SIGNAL(OpenWithSafari()),   parent, SLOT(OpenWithSafari()));
    connect(this, SIGNAL(OpenWithChrome()),   parent, SLOT(OpenWithChrome()));
    connect(this, SIGNAL(OpenWithSleipnir()), parent, SLOT(OpenWithSleipnir()));
    connect(this, SIGNAL(OpenWithVivaldi()),  parent, SLOT(OpenWithVivaldi()));
    connect(this, SIGNAL(OpenWithCustom()),   parent, SLOT(OpenWithCustom()));

    connect(this, SIGNAL(Reconfigure()),      parent, SLOT(Reconfigure()));
}

Receiver::~Receiver(){
    m_LineEdit->deleteLater();

    disconnect(m_LocalServer, SIGNAL(newConnection()),
               this, SLOT(ForeignCommandReceived()));

    if(1 > Application::GetMainWindows().size())
        m_LocalServer->deleteLater();
}

bool Receiver::IsPurged() const {
    return !parent();
}

void Receiver::Purge(){
    bool v = isVisible();
    setParent(0);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    if(v) show();
    else hide();
}

void Receiver::Join(){
    bool v = isVisible();
    setParent(m_TreeBank);
    if(v) show();
    else hide();
}

void Receiver::ResizeNotify(QSize size){
    Notifier *notifier = m_TreeBank->GetNotifier();

    int len = m_SuggestStrings.length();

    int nw = notifier
        ? qMax(size.width() * NOTIFIER_WIDTH_PERCENTAGE / 100,
               NOTIFIER_MINIMUM_WIDTH)
        : 0;

    int w = size.width() - nw;
    int h = RECEIVER_HEIGHT + len * SUGGEST_HEIGHT;
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
    m_LineEdit->setGeometry(0, h - LINEEDIT_HEIGHT,
                            width(), LINEEDIT_HEIGHT);
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

    if(v && IsPurged()) activateWindow();

    show(); raise(); repaint();

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
    if(!win || win->GetTreeBank() != m_TreeBank) return;

    QLocalSocket *clientConnection = m_LocalServer->nextPendingConnection();
    if(!clientConnection) return;

    while(clientConnection->bytesAvailable() < static_cast<int>(sizeof(quint32))){
        if(!clientConnection->waitForReadyRead()) return;
    }

    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);

#if QT_VERSION >= 0x050900
    in.setVersion(QDataStream::Qt_5_9);
#elif QT_VERSION >= 0x050800
    in.setVersion(QDataStream::Qt_5_8);
#else
    in.setVersion(QDataStream::Qt_5_7);
#endif

    if(clientConnection->bytesAvailable() < static_cast<int>(sizeof(quint16))){
        return;
    }

    QString command;
    in >> command;
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
            // fall through.
            list << QString();
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
            QStringList result = cand.toList();
            qSort(result.begin(), result.end(), qGreater<QString>());
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
    default: break;
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
    if(Application::ExactMatch(QStringLiteral("[uU]p"), cmd)){                            emit Up();                   return; }
    if(Application::ExactMatch(QStringLiteral("[dD](?:ow)?n"), cmd)){                     emit Down();                 return; }
    if(Application::ExactMatch(QStringLiteral("[rR]ight"), cmd)){                         emit Right();                return; }
    if(Application::ExactMatch(QStringLiteral("[lL]eft"), cmd)){                          emit Left();                 return; }
    if(Application::ExactMatch(QStringLiteral("[hH]ome"), cmd)){                          emit Home();                 return; }
    if(Application::ExactMatch(QStringLiteral("[eE]nd"), cmd)){                           emit End();                  return; }
    if(Application::ExactMatch(QStringLiteral("[pP](?:g|age)?[uU]p"), cmd)){              emit PageUp();               return; }
    if(Application::ExactMatch(QStringLiteral("[pP](?:g|age)?[dD](?:ow)?n"), cmd)){       emit PageDown();             return; }

    if(Application::ExactMatch(QStringLiteral("[iI]mport"), cmd)){                        emit Import();               return; }
    if(Application::ExactMatch(QStringLiteral("[eE]xport"), cmd)){                        emit Export();               return; }
    if(Application::ExactMatch(QStringLiteral("[aA]bout(?:[vV]anilla)?"), cmd)){          emit AboutVanilla();         return; }
    if(Application::ExactMatch(QStringLiteral("[aA]bout[qQ]t"), cmd)){                    emit AboutQt();              return; }
    if(Application::ExactMatch(QStringLiteral("[qQ]uit"), cmd)){                          emit Quit();                 return; }

    if(Application::ExactMatch(QStringLiteral("(?:[tT]oggle)?[nN]otifier"), cmd)){        emit ToggleNotifier();       return; }
    if(Application::ExactMatch(QStringLiteral("(?:[tT]oggle)?[rR]eceiver"), cmd)){        emit ToggleReceiver();       return; }
    if(Application::ExactMatch(QStringLiteral("(?:[tT]oggle)?[mM]enu[bB]ar"), cmd)){      emit ToggleMenuBar();        return; }
    if(Application::ExactMatch(QStringLiteral("(?:[tT]oggle)?(?:[tT]ree|[tT]ab)[bB]ar"), cmd)){ emit ToggleTreeBar();  return; }
    if(Application::ExactMatch(QStringLiteral("(?:[tT]oggle)?(?:[tT]ool|[aA]ddress)[bB]ar"), cmd)){ emit ToggleToolBar(); return; }
    if(Application::ExactMatch(QStringLiteral("(?:[tT]oggle)?[fF]ull[sS]creen"), cmd)){   emit ToggleFullScreen();     return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[mM]aximized"), cmd)){            emit ToggleMaximized();      return; }
    if(Application::ExactMatch(QStringLiteral("[mM]aximize(?:[wW]indow)?"), cmd)){        emit ToggleMaximized();      return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[mM]inimized"), cmd)){            emit ToggleMinimized();      return; }
    if(Application::ExactMatch(QStringLiteral("[mM]inimize(?:[wW]indow)?"), cmd)){        emit ToggleMinimized();      return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[sS]haded"), cmd)){               emit ToggleShaded();         return; }
    if(Application::ExactMatch(QStringLiteral("[sS]hade(?:[wW]indow)?"), cmd)){           emit ShadeWindow();          return; }
    if(Application::ExactMatch(QStringLiteral("[uU]nshade(?:[wW]indow)?"), cmd)){         emit UnshadeWindow();        return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ew[wW]indow"), cmd)){                  emit NewWindow();            return; }
    if(Application::ExactMatch(QStringLiteral("[cC]lose[wW]indow"), cmd)){                emit CloseWindow();          return; }
    if(Application::ExactMatch(QStringLiteral("[sS]witch(?:[wW]indow)?"), cmd)){          emit SwitchWindow();         return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ext[wW]indow"), cmd)){                 emit NextWindow();           return; }
    if(Application::ExactMatch(QStringLiteral("[pP]rev(?:ious)?[wW]indow"), cmd)){        emit PrevWindow();           return; }

    if(Application::ExactMatch(QStringLiteral("[bB]ack(?:ward)?"), cmd)){                 emit Back();                 return; }
    if(Application::ExactMatch(QStringLiteral("[fF]orward"), cmd)){                       emit Forward();              return; }
    if(Application::ExactMatch(QStringLiteral("[rR]ewind"), cmd)){                        emit Rewind();               return; }
    if(Application::ExactMatch(QStringLiteral("[fF]ast[fF]orward"), cmd)){                emit FastForward();          return; }
    if(Application::ExactMatch(QStringLiteral("[uU]pdir(?:ectory)?"), cmd)){              emit UpDirectory();          return; }
    if(Application::ExactMatch(QStringLiteral("[cC]lose"), cmd)){                         emit Close();                return; }
    if(Application::ExactMatch(QStringLiteral("[rR]estore"), cmd)){                       emit Restore();              return; }
    if(Application::ExactMatch(QStringLiteral("[rR]ecreate"), cmd)){                      emit Recreate();             return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ext(?:[vV]iew)?"), cmd)){              emit NextView();             return; }
    if(Application::ExactMatch(QStringLiteral("[pP]rev(?:ious)?(?:[vV]iew)?"), cmd)){     emit PrevView();             return; }
    if(Application::ExactMatch(QStringLiteral("[bB]ury(?:[vV]iew)?"), cmd)){              emit BuryView();             return; }
    if(Application::ExactMatch(QStringLiteral("[dD]ig(?:[vV]iew)?"), cmd)){               emit DigView();              return; }
    if(Application::ExactMatch(QStringLiteral("(?:1|[fF]ir)st(?:[vV]iew)?"), cmd)){       emit FirstView();            return; }
    if(Application::ExactMatch(QStringLiteral("(?:2|[sS]eco)nd(?:[vV]iew)?"), cmd)){      emit SecondView();           return; }
    if(Application::ExactMatch(QStringLiteral("(?:3|[tT]hi)rd(?:[vV]iew)?"), cmd)){       emit ThirdView();            return; }
    if(Application::ExactMatch(QStringLiteral("(?:4|[fF]our)th(?:[vV]iew)?"), cmd)){      emit FourthView();           return; }
    if(Application::ExactMatch(QStringLiteral("(?:5|[fF]if)th(?:[vV]iew)?"), cmd)){       emit FifthView();            return; }
    if(Application::ExactMatch(QStringLiteral("(?:6|[sS]ix)th(?:[vV]iew)?"), cmd)){       emit SixthView();            return; }
    if(Application::ExactMatch(QStringLiteral("(?:7|[sS]even)th(?:[vV]iew)?"), cmd)){     emit SeventhView();          return; }
    if(Application::ExactMatch(QStringLiteral("(?:8|[eE]igh)th(?:[vV]iew)?"), cmd)){      emit EighthView();           return; }
    if(Application::ExactMatch(QStringLiteral("(?:9|[nN]in)th(?:[vV]iew)?"), cmd)){       emit NinthView();            return; }
    if(Application::ExactMatch(QStringLiteral("(?:10|[tT]en)th(?:[vV]iew)?"), cmd)){      emit TenthView();            return; }
    if(Application::ExactMatch(QStringLiteral("[lL]ast(?:[vV]iew)?"), cmd)){              emit LastView();             return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ew(?:[vV]iew)?(?:[nN]ode)?"), cmd)){   emit NewViewNode();          return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ew[hH]ist(?:[nN]ode)?"), cmd)){        emit NewHistNode();          return; }
    if(Application::ExactMatch(QStringLiteral("[cC]lone(?:[vV]iew)?(?:[nN]ode)?"), cmd)){ emit CloneViewNode();        return; }
    if(Application::ExactMatch(QStringLiteral("[cC]lone[hH]ist(?:[nN]ode)?"), cmd)){      emit CloneHistNode();        return; }
    if(Application::ExactMatch(QStringLiteral("[lL]ocal[nN]ode"), cmd)){                  emit MakeLocalNode();        return; }
    if(Application::ExactMatch(QStringLiteral("(?:[dD]isplay)?[aA]ccess[kK]ey"), cmd)){   emit DisplayAccessKey();     return; }
    if(Application::ExactMatch(QStringLiteral("(?:[dD]isplay)?[vV]iew[tT]ree"), cmd)){    emit DisplayViewTree();      return; }
    if(Application::ExactMatch(QStringLiteral("(?:[dD]isplay)?[hH]ist[tT]ree"), cmd)){    emit DisplayHistTree();      return; }
    if(Application::ExactMatch(QStringLiteral("(?:[dD]isplay)?[tT]rash[tT]ree"), cmd)){   emit DisplayTrashTree();     return; }
    if(Application::ExactMatch(QStringLiteral("[rR]elease[hH]idden[vV]iew"), cmd)){       emit ReleaseHiddenView();    return; }

    if(Application::ExactMatch(QStringLiteral("[cC]opy"), cmd)){                          emit Copy();                 return; }
    if(Application::ExactMatch(QStringLiteral("[cC]ut"), cmd)){                           emit Cut();                  return; }
    if(Application::ExactMatch(QStringLiteral("[pP]aste"), cmd)){                         emit Paste();                return; }
    if(Application::ExactMatch(QStringLiteral("[uU]ndo"), cmd)){                          emit Undo();                 return; }
    if(Application::ExactMatch(QStringLiteral("[rR]edo"), cmd)){                          emit Redo();                 return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[aA]ll"), cmd)){                  emit SelectAll();            return; }
    if(Application::ExactMatch(QStringLiteral("[uU]n[sS]elect"), cmd)){                   emit Unselect();             return; }
    if(Application::ExactMatch(QStringLiteral("[rR]eload"), cmd)){                        emit Reload();               return; }
    if(Application::ExactMatch(QStringLiteral("[rR]eload[aA]nd[bB]ypass[cC]ache"), cmd)){ emit ReloadAndBypassCache(); return; }
    if(Application::ExactMatch(QStringLiteral("[sS]top"), cmd)){                          emit Stop();                 return; }
    if(Application::ExactMatch(QStringLiteral("[sS]top[aA]nd[uU]n[sS]elect"), cmd)){      emit StopAndUnselect();      return; }

    if(Application::ExactMatch(QStringLiteral("[pP]rint"), cmd)){                         emit Print();                return; }
    if(Application::ExactMatch(QStringLiteral("[sS]ave"), cmd)){                          emit Save();                 return; }
    if(Application::ExactMatch(QStringLiteral("[zZ]oom[iI]n"), cmd)){                     emit ZoomIn();               return; }
    if(Application::ExactMatch(QStringLiteral("[zZ]oom[oO]ut"), cmd)){                    emit ZoomOut();              return; }
    if(Application::ExactMatch(QStringLiteral("[vV]iew[sS]ource"), cmd)){                 emit ViewSource();           return; }
    if(Application::ExactMatch(QStringLiteral("[aA]pply[sS]ource"), cmd)){                emit ApplySource();          return; }

    if(Application::ExactMatch(QStringLiteral("[iI]nspect(?:[eE]lement)?"), cmd)){        emit InspectElement();       return; }

    if(Application::ExactMatch(QStringLiteral("[cC]opy[uU]rl"), cmd)){                    emit CopyUrl();              return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[tT]itle"), cmd)){                  emit CopyTitle();            return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[pP]age[aA]s[lL]ink"), cmd)){       emit CopyPageAsLink();       return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[sS]elected[hH]tml"), cmd)){        emit CopySelectedHtml();     return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?"), cmd)){ emit OpenWithIE(); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[eE]dge"), cmd)){  emit OpenWithEdge();         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?"), cmd)){ emit OpenWithFF(); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[oO]pera"), cmd)){ emit OpenWithOpera();        return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[oO][pP][rR]"), cmd)){ emit OpenWithOPR();      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[sS]afari"), cmd)){ emit OpenWithSafari();      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[cC]hrome"), cmd)){ emit OpenWithChrome();      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[sS]leipnir"), cmd)){ emit OpenWithSleipnir();  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[vV]ivaldi"), cmd)){ emit OpenWithVivaldi();    return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[cC]ustom"), cmd)){ emit OpenWithCustom();      return; }

    if(Application::ExactMatch(QStringLiteral("[cC]lick(?:[eE]lement)?"), cmd)){                                          emit TriggerElementAction(Page::_ClickElement);                      return; }
    if(Application::ExactMatch(QStringLiteral("[fF]ocus(?:[eE]lement)?"), cmd)){                                          emit TriggerElementAction(Page::_FocusElement);                      return; }
    if(Application::ExactMatch(QStringLiteral("[hH]over(?:[eE]lement)?"), cmd)){                                          emit TriggerElementAction(Page::_HoverElement);                      return; }

    if(Application::ExactMatch(QStringLiteral("[lL]oad[lL]ink"), cmd)){                                                   emit TriggerElementAction(Page::_LoadLink);                          return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink"), cmd)){                                                   emit TriggerElementAction(Page::_OpenLink);                          return; }
    if(Application::ExactMatch(QStringLiteral("[dD]ownload[lL]ink"), cmd)){                                               emit TriggerElementAction(Page::_DownloadLink);                      return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[lL]ink[uU]rl"), cmd)){                                             emit TriggerElementAction(Page::_CopyLinkUrl);                       return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[lL]ink[hH]tml"), cmd)){                                            emit TriggerElementAction(Page::_CopyLinkHtml);                      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?"), cmd)){  emit TriggerElementAction(Page::_OpenLinkWithIE);                    return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[eE]dge"), cmd)){                           emit TriggerElementAction(Page::_OpenLinkWithEdge);                  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?"), cmd)){           emit TriggerElementAction(Page::_OpenLinkWithFF);                    return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[oO]pera"), cmd)){                          emit TriggerElementAction(Page::_OpenLinkWithOpera);                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[oO][pP][rR]"), cmd)){                      emit TriggerElementAction(Page::_OpenLinkWithOPR);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[sS]afari"), cmd)){                         emit TriggerElementAction(Page::_OpenLinkWithSafari);                return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[cC]hrome"), cmd)){                         emit TriggerElementAction(Page::_OpenLinkWithChrome);                return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[sS]leipnir"), cmd)){                       emit TriggerElementAction(Page::_OpenLinkWithSleipnir);              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[vV]ivaldi"), cmd)){                        emit TriggerElementAction(Page::_OpenLinkWithVivaldi);               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[cC]ustom"), cmd)){                         emit TriggerElementAction(Page::_OpenLinkWithCustom);                return; }

    if(Application::ExactMatch(QStringLiteral("[lL]oad[iI]mage"), cmd)){                                                  emit TriggerElementAction(Page::_LoadImage);                         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage"), cmd)){                                                  emit TriggerElementAction(Page::_OpenImage);                         return; }
    if(Application::ExactMatch(QStringLiteral("[dD]ownload[iI]mage"), cmd)){                                              emit TriggerElementAction(Page::_DownloadImage);                     return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[iI]mage"), cmd)){                                                  emit TriggerElementAction(Page::_CopyImage);                         return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[iI]mage[uU]rl"), cmd)){                                            emit TriggerElementAction(Page::_CopyImageUrl);                      return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[iI]mage[hH]tml"), cmd)){                                           emit TriggerElementAction(Page::_CopyImageHtml);                     return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?"), cmd)){ emit TriggerElementAction(Page::_OpenImageWithIE);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[eE]dge"), cmd)){                          emit TriggerElementAction(Page::_OpenImageWithEdge);                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?"), cmd)){          emit TriggerElementAction(Page::_OpenImageWithFF);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[oO]pera"), cmd)){                         emit TriggerElementAction(Page::_OpenImageWithOpera);                return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[oO][pP][rR]"), cmd)){                     emit TriggerElementAction(Page::_OpenImageWithOPR);                  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[sS]afari"), cmd)){                        emit TriggerElementAction(Page::_OpenImageWithSafari);               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[cC]hrome"), cmd)){                        emit TriggerElementAction(Page::_OpenImageWithChrome);               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[sS]leipnir"), cmd)){                      emit TriggerElementAction(Page::_OpenImageWithSleipnir);             return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[vV]ivaldi"), cmd)){                       emit TriggerElementAction(Page::_OpenImageWithVivaldi);              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[cC]ustom"), cmd)){                        emit TriggerElementAction(Page::_OpenImageWithCustom);               return; }

    if(Application::ExactMatch(QStringLiteral("[lL]oad[mM]edia"), cmd)){                                                  emit TriggerElementAction(Page::_LoadMedia);                         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia"), cmd)){                                                  emit TriggerElementAction(Page::_OpenMedia);                         return; }
    if(Application::ExactMatch(QStringLiteral("[dD]ownload[mM]edia"), cmd)){                                              emit TriggerElementAction(Page::_DownloadMedia);                     return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[mM]edia[cC]ontrols"), cmd)){                                     emit TriggerElementAction(Page::_ToggleMediaControls);               return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[mM]edia[lL]oop"), cmd)){                                         emit TriggerElementAction(Page::_ToggleMediaLoop);                   return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[mM]edia[pP]lay[pP]ause"), cmd)){                                 emit TriggerElementAction(Page::_ToggleMediaPlayPause);              return; }
    if(Application::ExactMatch(QStringLiteral("[tT]oggle[mM]edia[mM]ute"), cmd)){                                         emit TriggerElementAction(Page::_ToggleMediaMute);                   return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[mM]edia[uU]rl"), cmd)){                                            emit TriggerElementAction(Page::_CopyMediaUrl);                      return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[mM]edia[hH]tml"), cmd)){                                           emit TriggerElementAction(Page::_CopyMediaHtml);                     return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?"), cmd)){ emit TriggerElementAction(Page::_OpenMediaWithIE);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[eE]dge"), cmd)){                          emit TriggerElementAction(Page::_OpenMediaWithEdge);                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?"), cmd)){          emit TriggerElementAction(Page::_OpenMediaWithFF);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[oO]pera"), cmd)){                         emit TriggerElementAction(Page::_OpenMediaWithOpera);                return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[oO][pP][rR]"), cmd)){                     emit TriggerElementAction(Page::_OpenMediaWithOPR);                  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[sS]afari"), cmd)){                        emit TriggerElementAction(Page::_OpenMediaWithSafari);               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[cC]hrome"), cmd)){                        emit TriggerElementAction(Page::_OpenMediaWithChrome);               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[sS]leipnir"), cmd)){                      emit TriggerElementAction(Page::_OpenMediaWithSleipnir);             return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[vV]ivaldi"), cmd)){                       emit TriggerElementAction(Page::_OpenMediaWithVivaldi);              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia(?:[wW]ith|[oO]n)[cC]ustom"), cmd)){                        emit TriggerElementAction(Page::_OpenMediaWithCustom);               return; }

    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode"), cmd)){                 emit TriggerElementAction(Page::_OpenInNewViewNode);                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode"), cmd)){                 emit TriggerElementAction(Page::_OpenInNewHistNode);                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory"), cmd)){                   emit TriggerElementAction(Page::_OpenInNewDirectory);                return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot"), cmd)){                              emit TriggerElementAction(Page::_OpenOnRoot);                        return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground"), cmd)){    emit TriggerElementAction(Page::_OpenInNewViewNodeForeground);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[fF]oreground"), cmd)){    emit TriggerElementAction(Page::_OpenInNewHistNodeForeground);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[fF]oreground"), cmd)){      emit TriggerElementAction(Page::_OpenInNewDirectoryForeground);      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[fF]oreground"), cmd)){                 emit TriggerElementAction(Page::_OpenOnRootForeground);              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground"), cmd)){    emit TriggerElementAction(Page::_OpenInNewViewNodeBackground);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[bB]ackground"), cmd)){    emit TriggerElementAction(Page::_OpenInNewHistNodeBackground);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[bB]ackground"), cmd)){      emit TriggerElementAction(Page::_OpenInNewDirectoryBackground);      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[bB]ackground"), cmd)){                 emit TriggerElementAction(Page::_OpenOnRootBackground);              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow"), cmd)){ emit TriggerElementAction(Page::_OpenInNewViewNodeThisWindow);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[tT]his[wW]indow"), cmd)){ emit TriggerElementAction(Page::_OpenInNewHistNodeThisWindow);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[tT]his[wW]indow"), cmd)){   emit TriggerElementAction(Page::_OpenInNewDirectoryThisWindow);      return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[tT]his[wW]indow"), cmd)){              emit TriggerElementAction(Page::_OpenOnRootThisWindow);              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow"), cmd)){  emit TriggerElementAction(Page::_OpenInNewViewNodeNewWindow);        return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[nN]ew[wW]indow"), cmd)){  emit TriggerElementAction(Page::_OpenInNewHistNodeNewWindow);        return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow"), cmd)){    emit TriggerElementAction(Page::_OpenInNewDirectoryNewWindow);       return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[nN]ew[wW]indow"), cmd)){               emit TriggerElementAction(Page::_OpenOnRootNewWindow);               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode"), cmd)){                     emit TriggerElementAction(Page::_OpenImageInNewViewNode);            return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode"), cmd)){                     emit TriggerElementAction(Page::_OpenImageInNewHistNode);            return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory"), cmd)){                       emit TriggerElementAction(Page::_OpenImageInNewDirectory);           return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot"), cmd)){                                  emit TriggerElementAction(Page::_OpenImageOnRoot);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground"), cmd)){        emit TriggerElementAction(Page::_OpenImageInNewViewNodeForeground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[fF]oreground"), cmd)){        emit TriggerElementAction(Page::_OpenImageInNewHistNodeForeground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[fF]oreground"), cmd)){          emit TriggerElementAction(Page::_OpenImageInNewDirectoryForeground); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[fF]oreground"), cmd)){                     emit TriggerElementAction(Page::_OpenImageOnRootForeground);         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground"), cmd)){        emit TriggerElementAction(Page::_OpenImageInNewViewNodeBackground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[bB]ackground"), cmd)){        emit TriggerElementAction(Page::_OpenImageInNewHistNodeBackground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[bB]ackground"), cmd)){          emit TriggerElementAction(Page::_OpenImageInNewDirectoryBackground); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[bB]ackground"), cmd)){                     emit TriggerElementAction(Page::_OpenImageOnRootBackground);         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow"), cmd)){     emit TriggerElementAction(Page::_OpenImageInNewViewNodeThisWindow);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[tT]his[wW]indow"), cmd)){     emit TriggerElementAction(Page::_OpenImageInNewHistNodeThisWindow);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[tT]his[wW]indow"), cmd)){       emit TriggerElementAction(Page::_OpenImageInNewDirectoryThisWindow); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[tT]his[wW]indow"), cmd)){                  emit TriggerElementAction(Page::_OpenImageOnRootThisWindow);         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow"), cmd)){      emit TriggerElementAction(Page::_OpenImageInNewViewNodeNewWindow);   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[nN]ew[wW]indow"), cmd)){      emit TriggerElementAction(Page::_OpenImageInNewHistNodeNewWindow);   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow"), cmd)){        emit TriggerElementAction(Page::_OpenImageInNewDirectoryNewWindow);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[nN]ew[wW]indow"), cmd)){                   emit TriggerElementAction(Page::_OpenImageOnRootNewWindow);          return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode"), cmd)){                     emit TriggerElementAction(Page::_OpenMediaInNewViewNode);            return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[hH]ist[nN]ode"), cmd)){                     emit TriggerElementAction(Page::_OpenMediaInNewHistNode);            return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory"), cmd)){                       emit TriggerElementAction(Page::_OpenMediaInNewDirectory);           return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[oO]n[rR]oot"), cmd)){                                  emit TriggerElementAction(Page::_OpenMediaOnRoot);                   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground"), cmd)){        emit TriggerElementAction(Page::_OpenMediaInNewViewNodeForeground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[hH]ist[nN]ode[fF]oreground"), cmd)){        emit TriggerElementAction(Page::_OpenMediaInNewHistNodeForeground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[fF]oreground"), cmd)){          emit TriggerElementAction(Page::_OpenMediaInNewDirectoryForeground); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[oO]n[rR]oot[fF]oreground"), cmd)){                     emit TriggerElementAction(Page::_OpenMediaOnRootForeground);         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground"), cmd)){        emit TriggerElementAction(Page::_OpenMediaInNewViewNodeBackground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[hH]ist[nN]ode[bB]ackground"), cmd)){        emit TriggerElementAction(Page::_OpenMediaInNewHistNodeBackground);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[bB]ackground"), cmd)){          emit TriggerElementAction(Page::_OpenMediaInNewDirectoryBackground); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[oO]n[rR]oot[bB]ackground"), cmd)){                     emit TriggerElementAction(Page::_OpenMediaOnRootBackground);         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow"), cmd)){     emit TriggerElementAction(Page::_OpenMediaInNewViewNodeThisWindow);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[hH]ist[nN]ode[tT]his[wW]indow"), cmd)){     emit TriggerElementAction(Page::_OpenMediaInNewHistNodeThisWindow);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[tT]his[wW]indow"), cmd)){       emit TriggerElementAction(Page::_OpenMediaInNewDirectoryThisWindow); return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[oO]n[rR]oot[tT]his[wW]indow"), cmd)){                  emit TriggerElementAction(Page::_OpenMediaOnRootThisWindow);         return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow"), cmd)){      emit TriggerElementAction(Page::_OpenMediaInNewViewNodeNewWindow);   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[hH]ist[nN]ode[nN]ew[wW]indow"), cmd)){      emit TriggerElementAction(Page::_OpenMediaInNewHistNodeNewWindow);   return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow"), cmd)){        emit TriggerElementAction(Page::_OpenMediaInNewDirectoryNewWindow);  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[mM]edia[oO]n[rR]oot[nN]ew[wW]indow"), cmd)){                   emit TriggerElementAction(Page::_OpenMediaOnRootNewWindow);          return; }

    if(Application::ExactMatch(QStringLiteral("[dD]eactivate"), cmd)){                                         emit Deactivate();                      return; }
    if(Application::ExactMatch(QStringLiteral("[rR]efresh"), cmd)){                                            emit Refresh();                         return; }
    if(Application::ExactMatch(QStringLiteral("[rR]efresh[nN]o[sS]croll"), cmd)){                              emit RefreshNoScroll();                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode"), cmd)){                                        emit OpenNode();                        return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode[oO]n[nN]ew[wW]indow"), cmd)){                    emit OpenNodeOnNewWindow();             return; }
    if(Application::ExactMatch(QStringLiteral("[dD]elete[nN]ode"), cmd)){                                      emit DeleteNode();                      return; }
    if(Application::ExactMatch(QStringLiteral("[dD]elete[rR]ight[nN]ode"), cmd)){                              emit DeleteRightNode();                 return; }
    if(Application::ExactMatch(QStringLiteral("[dD]elete[lL]eft[nN]ode"), cmd)){                               emit DeleteLeftNode();                  return; }
    if(Application::ExactMatch(QStringLiteral("[dD]elete[oO]ther[nN]ode"), cmd)){                              emit DeleteOtherNode();                 return; }
    if(Application::ExactMatch(QStringLiteral("[pP]aste[nN]ode"), cmd)){                                       emit PasteNode();                       return; }
    if(Application::ExactMatch(QStringLiteral("[rR]estore[nN]ode"), cmd)){                                     emit RestoreNode();                     return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ew[nN]ode"), cmd)){                                         emit NewNode();                         return; }
    if(Application::ExactMatch(QStringLiteral("[cC]lone[nN]ode"), cmd)){                                       emit CloneNode();                       return; }
    if(Application::ExactMatch(QStringLiteral("[uU]p[dD]irectory"), cmd)){                                     emit UpDirectory();                     return; }
    if(Application::ExactMatch(QStringLiteral("[dD]own[dD]irectory"), cmd)){                                   emit DownDirectory();                   return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ake[lL]ocal[nN]ode"), cmd)){                                emit MakeLocalNode();                   return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ake[dD]irectory"), cmd)){                                   emit MakeDirectory();                   return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ake[dD]irectory[wW]ith[sS]elected[nN]ode"), cmd)){          emit MakeDirectoryWithSelectedNode();   return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ake[dD]irectory[wW]ith[sS]ame[dD]omain[nN]ode"), cmd)){     emit MakeDirectoryWithSameDomainNode(); return; }
    if(Application::ExactMatch(QStringLiteral("[rR]ename[nN]ode"), cmd)){                                      emit RenameNode();                      return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[nN]ode[uU]rl"), cmd)){                                  emit CopyNodeUrl();                     return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[nN]ode[tT]itle"), cmd)){                                emit CopyNodeTitle();                   return; }
    if(Application::ExactMatch(QStringLiteral("[cC]opy[nN]ode[aA]s[lL]ink"), cmd)){                            emit CopyNodeAsLink();                  return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?"), cmd)){ emit OpenNodeWithIE();        return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[eE]dge"), cmd)){                emit OpenNodeWithEdge();                return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?"), cmd)){ emit OpenNodeWithFF();                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[oO]pera"), cmd)){               emit OpenNodeWithOpera();               return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[oO][pP][rR]"), cmd)){           emit OpenNodeWithOPR();                 return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[sS]afari"), cmd)){              emit OpenNodeWithSafari();              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[cC]hrome"), cmd)){              emit OpenNodeWithChrome();              return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[sS]leipnir"), cmd)){            emit OpenNodeWithSleipnir();            return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[vV]ivaldi"), cmd)){             emit OpenNodeWithVivaldi();             return; }
    if(Application::ExactMatch(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[cC]ustom"), cmd)){              emit OpenNodeWithCustom();              return; }

    if(Application::ExactMatch(QStringLiteral("[tT]oggle[tT]rash"), cmd)){                                     emit ToggleTrash();                     return; }
    if(Application::ExactMatch(QStringLiteral("[sS]croll[uU]p"), cmd)){                                        emit ScrollUp();                        return; }
    if(Application::ExactMatch(QStringLiteral("[sS]croll[dD]own"), cmd)){                                      emit ScrollDown();                      return; }
    if(Application::ExactMatch(QStringLiteral("[pP]age[uU]p"), cmd)){                                          emit PageUp();                          return; }
    if(Application::ExactMatch(QStringLiteral("[pP]age[dD]own"), cmd)){                                        emit PageDown();                        return; }
    if(Application::ExactMatch(QStringLiteral("[nN]ext[pP]age"), cmd)){                                        emit NextPage();                        return; }
    if(Application::ExactMatch(QStringLiteral("[pP]rev(?:ious)[pP]age"), cmd)){                                emit PrevPage();                        return; }
    if(Application::ExactMatch(QStringLiteral("[zZ]oom[iI]n"), cmd)){                                          emit ZoomIn();                          return; }
    if(Application::ExactMatch(QStringLiteral("[zZ]oom[oO]ut"), cmd)){                                         emit ZoomOut();                         return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[uU]pper[iI]tem"), cmd)){                           emit MoveToUpperItem();                 return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[lL]ower[iI]tem"), cmd)){                           emit MoveToLowerItem();                 return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[rR]ight[iI]tem"), cmd)){                           emit MoveToRightItem();                 return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[lL]eft[iI]tem"), cmd)){                            emit MoveToLeftItem();                  return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[pP]rev[pP]age"), cmd)){                            emit MoveToPrevPage();                  return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[nN]ext[pP]age"), cmd)){                            emit MoveToNextPage();                  return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[fF]irst[iI]tem"), cmd)){                           emit MoveToFirstItem();                 return; }
    if(Application::ExactMatch(QStringLiteral("[mM]ove[tT]o[lL]ast[iI]tem"), cmd)){                            emit MoveToLastItem();                  return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[uU]pper[iI]tem"), cmd)){                         emit SelectToUpperItem();               return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[lL]ower[iI]tem"), cmd)){                         emit SelectToLowerItem();               return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[rR]ight[iI]tem"), cmd)){                         emit SelectToRightItem();               return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[lL]eft[iI]tem"), cmd)){                          emit SelectToLeftItem();                return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[pP]rev[pP]age"), cmd)){                          emit SelectToPrevPage();                return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[nN]ext[pP]age"), cmd)){                          emit SelectToNextPage();                return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[fF]irst[iI]tem"), cmd)){                         emit SelectToFirstItem();               return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[tT]o[lL]ast[iI]tem"), cmd)){                          emit SelectToLastItem();                return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[iI]tem"), cmd)){                                      emit SelectItem();                      return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[rR]ange"), cmd)){                                     emit SelectRange();                     return; }
    if(Application::ExactMatch(QStringLiteral("[sS]elect[aA]ll"), cmd)){                                       emit SelectAll();                       return; }
    if(Application::ExactMatch(QStringLiteral("[cC]lear[sS]election"), cmd)){                                  emit ClearSelection();                  return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[uU]pper"), cmd)){                              emit TransferToUpper();                 return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[lL]ower"), cmd)){                              emit TransferToLower();                 return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[rR]ight"), cmd)){                              emit TransferToRight();                 return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[lL]eft"), cmd)){                               emit TransferToLeft();                  return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[pP]rev[pP]age"), cmd)){                        emit TransferToPrevPage();              return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[nN]ext[pP]age"), cmd)){                        emit TransferToNextPage();              return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[fF]irst"), cmd)){                              emit TransferToFirst();                 return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[lL]ast"), cmd)){                               emit TransferToLast();                  return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[uU]p[dD]irectory"), cmd)){                     emit TransferToUpDirectory();           return; }
    if(Application::ExactMatch(QStringLiteral("[tT]ransfer[tT]o[dD]own[dD]irectory"), cmd)){                   emit TransferToDownDirectory();         return; }
    if(Application::ExactMatch(QStringLiteral("[sS]witch[nN]ode[cC]ollection[tT]ype"), cmd)){                  emit SwitchNodeCollectionType();        return; }
    if(Application::ExactMatch(QStringLiteral("[sS]witch[nN]ode[cC]ollection[tT]ype[rR]everse"), cmd)){        emit SwitchNodeCollectionTypeReverse(); return; }

    if(Application::ExactMatch(QStringLiteral("[rR]econf(?:ig(?:ure)?)?"), cmd)){                              emit Reconfigure();                     return; }
    if(Application::ExactMatch(QStringLiteral("[bB]lank"), cmd)){                                              emit OpenUrl(BLANK_URL);                return; }

    if(list.isEmpty() && Page::GetBookmarkletMap().contains(cmd)){

        emit OpenBookmarklet(Page::GetBookmarklet(cmd).first());

    } else if(Application::ExactMatch(QStringLiteral("[oO]pen"), cmd)){

        QStringList args;
        foreach(QString arg, list){
            if(arg == QStringLiteral("blank"))
                args << QStringLiteral("about:blank");
            else args << arg;
        }
        emit OpenUrl(Page::DirtyStringToUrls(args.join(QStringLiteral(" "))));

    } else if(Application::ExactMatch(QStringLiteral("[lL]oad"), cmd)){

        QString args = list.join(QStringLiteral(" "));
        if(args == QStringLiteral("blank"))
            emit OpenUrl(BLANK_URL);
        else emit OpenUrl(QUrl(args));

    } else if(Application::ExactMatch(QStringLiteral("[qQ]uery"), cmd)){

        emit OpenQueryUrl(list.join(QStringLiteral(" ")));

    } else if(Application::ExactMatch(QStringLiteral("[dD]ownload"), cmd)){

        if(list.length() >= 2) // ignore excess arguments.
            emit Download(list[0], list[1]);

    } else if(Application::ExactMatch(QStringLiteral("[sS]earch|[sS]eek"), cmd)){

        if(Application::keyboardModifiers() & Qt::ShiftModifier){
            emit SeekText(list.join(QStringLiteral(" ")), View::WrapsAroundDocument | View::FindBackward);
        } else {
            emit SeekText(list.join(QStringLiteral(" ")), View::WrapsAroundDocument);
        }

    } else if(Application::ExactMatch(QStringLiteral("[sS]et(?:tings?)?"), cmd)){

        if(list.length() >= 2){ // ignore excess arguments.
            QString key = list.takeFirst();
            QString val = list.join(" ");
            Application::GlobalSettings().setValue(key, val);
            emit Reconfigure();
        }

    } else if(Application::ExactMatch(QStringLiteral("[uU]n[sS]et(?:tings?)?"), cmd)){

        if(list.length() >= 1){ // ignore excess arguments.
            Application::GlobalSettings().remove(list[0]);
            emit Reconfigure();
        }

    } else if(Application::ExactMatch(QStringLiteral("[kK]ey"), cmd)){

        emit KeyEvent(list.join(QStringLiteral(" ")));

    } else if(Page::GetSearchEngineMap().contains(cmd)){

        emit SearchWith(cmd, list.join(QStringLiteral(" ")));

    } else {

        QStringList args;
        list.prepend(cmd);
        foreach(QString arg, list){
            if(arg == QStringLiteral("blank"))
                args << QStringLiteral("about:blank");
            else args << arg;
        }
        emit OpenUrl(Page::DirtyStringToUrls(args.join(QStringLiteral(" "))));
    }
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
    Q_UNUSED(ev);
    /* do nothing. */
}

void Receiver::paintEvent(QPaintEvent *ev){
    QPainter painter(this);
    painter.setFont(RECEIVER_FONT);

    const int len = m_SuggestStrings.length();

    for(int i = len; i > 0; i--){
        const QRect back_rect = QRect(-1, (len-i) * SUGGEST_HEIGHT - 1,
                                      width()+1, SUGGEST_HEIGHT);
        if(m_CurrentSuggestIndex == i - 1){
            static const QBrush b = QBrush(QColor(255, 255, 255, 128));
            painter.setBrush(b);
        } else {
            static const QBrush b = QBrush(QColor(  0,   0,   0, 128));
            painter.setBrush(b);
        }
        painter.setPen(Qt::NoPen);
        painter.drawRect(back_rect);

        const QRect text_rect = QRect(5, (len-i) * SUGGEST_HEIGHT + 4,
                                      width()-4, SUGGEST_HEIGHT);
        if(m_CurrentSuggestIndex == i - 1){
            static const QPen p = QPen(QColor(  0,   0,   0, 255));
            painter.setPen(p);
        } else {
            static const QPen p = QPen(QColor(255, 255, 255, 255));
            painter.setPen(p);
        }
        painter.setBrush(Qt::NoBrush);
        painter.drawText(text_rect, Qt::AlignLeft, m_SuggestStrings[i-1]);
    }

    const QRect back_rect = QRect(-1, len * SUGGEST_HEIGHT -1,
                                  width()+1, RECEIVER_HEIGHT);
    {
        static const QBrush b = QBrush(QColor(0, 0, 0, 128));
        painter.setPen(Qt::NoPen);
        painter.setBrush(b);
        painter.drawRect(back_rect);
    }

    const QRect text_rect = QRect(5, len * SUGGEST_HEIGHT + 4,
                                  width()-4, RECEIVER_HEIGHT);
    {
        static const QPen p = QPen(QColor(255, 255, 255, 255));
        painter.setPen(p);
        painter.setBrush(Qt::NoBrush);
        switch(m_Mode){
        case Command  : painter.drawText(text_rect, Qt::AlignLeft, tr("Command Mode.")); break;
        case Query    : painter.drawText(text_rect, Qt::AlignLeft, tr("Input Query.")); break;
        case UrlEdit  : painter.drawText(text_rect, Qt::AlignLeft, tr("Input Url.")); break;
        case Search   : painter.drawText(text_rect, Qt::AlignLeft, tr("Incremental Search")+ QStringLiteral(" : \"") + m_LineString + QStringLiteral("\".")); break;
        default : break;
        }
    }

    ev->setAccepted(true);
}

void Receiver::hideEvent(QHideEvent *ev){
    MainWindow *win = m_TreeBank->GetMainWindow();

    QTimer::singleShot(0, win, [win](){
        if(win->isActiveWindow()) win->SetFocus();
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
            m_CurrentSuggestIndex = m_SuggestStrings.length() - ev->pos().y() / SUGGEST_HEIGHT - 1;
            if(m_CurrentSuggestIndex >= m_SuggestStrings.length())
                m_CurrentSuggestIndex = -1;
            repaint();
        }
    }
    QStringList list = m_LineString.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
    if(m_Mode == Command &&
       Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?"), list[0])){
        if(!m_SuggestStrings.isEmpty()){
            m_CurrentSuggestIndex = m_SuggestStrings.length() - ev->pos().y() / SUGGEST_HEIGHT - 1;
            if(m_CurrentSuggestIndex >= m_SuggestStrings.length())
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
        for(uint i = 0; i < static_cast<uint>(nodelist.length()); i++){
            list << nodelist.item(i).toElement().attribute(QStringLiteral("data"));
        }
        SetSuggest(list);
    }
}
