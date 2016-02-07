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
#include <QThread>
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
    emit FocusIn();
    ev->setAccepted(true);
}

void LineEdit::focusOutEvent(QFocusEvent *ev){
    QLineEdit::focusOutEvent(ev);
    emit FocusOut();
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

Receiver::Receiver(TreeBank *parent, bool purge)
    : QWidget(purge ? 0 : parent)
{
    if(purge){
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
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
    connect(this, SIGNAL(UpDirectory()),      parent, SLOT(UpDirectory()));
    connect(this, SIGNAL(Close()),            parent, SLOT(Close()));
    connect(this, SIGNAL(Restore()),          parent, SLOT(Restore()));
    connect(this, SIGNAL(Recreate()),         parent, SLOT(Recreate()));
    connect(this, SIGNAL(NextView()),         parent, SLOT(NextView()));
    connect(this, SIGNAL(PrevView()),         parent, SLOT(PrevView()));
    connect(this, SIGNAL(BuryView()),         parent, SLOT(BuryView()));
    connect(this, SIGNAL(DigView()),          parent, SLOT(DigView()));
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

bool Receiver::IsPurged(){
    return !parent();
}

void Receiver::Purge(){
    bool v = isVisible();
    setParent(0);
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
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

QString Receiver::WaitForStringInput(){
    QEventLoop loop;
    QString result;

    QMetaObject::Connection signalCon;
    QMetaObject::Connection returnCon;
    QMetaObject::Connection abortCon;

    signalCon =
        connect(m_LineEdit, SIGNAL(FocusOut()),
                m_LineEdit, SIGNAL(Aborted()));

    returnCon =
        connect(m_LineEdit, &LineEdit::Returned, [&](){
                disconnect(signalCon);
                disconnect(returnCon);
                disconnect(abortCon);
                result = m_LineEdit->text();
                loop.quit();
            });

    abortCon =
        connect(m_LineEdit, &LineEdit::Aborted, [&](){
                disconnect(signalCon);
                disconnect(returnCon);
                disconnect(abortCon);
                result = QString();
                loop.quit();
            });

    loop.exec();

    return result;
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
    in.setVersion(QDataStream::Qt_5_5);
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
        QUrl base = QUrl(QStringLiteral("https://www.google.co.jp/complete/search"));
        QUrlQuery param;
        param.addQueryItem(QStringLiteral("q"), m_LineString);
        param.addQueryItem(QStringLiteral("output"), QStringLiteral("toolbar"));
        param.addQueryItem(QStringLiteral("hl"), QStringLiteral("ja"));
        base.setQuery(param);
        emit SuggestRequest(base);
    }
    QStringList list = str.split(QRegExp(QStringLiteral("[\n\r\t ]+")));
    if(m_Mode == Command &&
       QRegExp(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?")).exactMatch(list[0])){
        QSettings *settings = Application::GlobalSettings();
        if(!settings->group().isEmpty()) return;

        switch (list.length()){
        case 1:
            // fall through.
            list << QString();
        case 2:{
            QStringList result;
            QStringList path = list[1].split(QStringLiteral("/"));
            QString command = list[0];
            QString parent = QString();
            for(int i = 0; i < (path.length()-1); i++){
                if(path[i].isEmpty()) break;

                settings->beginGroup(path[i]);

                if(parent.isEmpty())
                    parent = path[i];
                else
                    parent = parent + QString(QStringLiteral("/")) + path[i];
            }
            foreach(QString group, settings->childGroups()){
                if(group.startsWith(path.last())){
                    if(parent.isEmpty())
                        result << command + QStringLiteral(" ") + group + QStringLiteral("/");
                    else
                        result << command + QStringLiteral(" ") + parent + QStringLiteral("/") + group + QStringLiteral("/");
                }
            }
            foreach(QString key, settings->childKeys()){
                if(key.startsWith(path.last())){
                    if(parent.isEmpty())
                        result << command + QStringLiteral(" ") + key;
                    else
                        result << command + QStringLiteral(" ") + parent + QStringLiteral("/") + key;
                }
            }
            while(!settings->group().isEmpty()){
                settings->endGroup();
            }
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
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1)
            m_LineEdit->setText(m_SuggestStrings[m_CurrentSuggestIndex]);
        else
            emit OpenQueryUrl(m_LineString);
        break;
    }
    case UrlEdit:
        emit OpenUrl(Page::DirtyStringToUrlList(m_LineString == QStringLiteral("blank")
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
            QStringList list = key.split(QRegExp(QStringLiteral("[\n\r\t ]+")));

            if(QRegExp(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?")).exactMatch(list[0])){
                QSettings *settings = Application::GlobalSettings();
                if(!settings->value(list[1]).toString().isEmpty())
                    list << settings->value(list[1]).toString();
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
    QStringList list = cmd.split(QRegExp(QStringLiteral("[\n\r\t ]+")));
    cmd = list.takeFirst();

    if(cmd.isEmpty()) return;
    if(QRegExp(QStringLiteral("[uU]p")).exactMatch(cmd)){                            emit Up();                   return; }
    if(QRegExp(QStringLiteral("[dD](?:ow)?n")).exactMatch(cmd)){                     emit Down();                 return; }
    if(QRegExp(QStringLiteral("[rR]ight")).exactMatch(cmd)){                         emit Right();                return; }
    if(QRegExp(QStringLiteral("[lL]eft")).exactMatch(cmd)){                          emit Left();                 return; }
    if(QRegExp(QStringLiteral("[hH]ome")).exactMatch(cmd)){                          emit Home();                 return; }
    if(QRegExp(QStringLiteral("[eE]nd")).exactMatch(cmd)){                           emit End();                  return; }
    if(QRegExp(QStringLiteral("[pP](?:g|age)?[uU]p")).exactMatch(cmd)){              emit PageUp();               return; }
    if(QRegExp(QStringLiteral("[pP](?:g|age)?[dD](?:ow)?n")).exactMatch(cmd)){       emit PageDown();             return; }

    if(QRegExp(QStringLiteral("[iI]mport")).exactMatch(cmd)){                        emit Import();               return; }
    if(QRegExp(QStringLiteral("[eE]xport")).exactMatch(cmd)){                        emit Export();               return; }
    if(QRegExp(QStringLiteral("[aA]bout(?:[vV]anilla)?")).exactMatch(cmd)){          emit AboutVanilla();         return; }
    if(QRegExp(QStringLiteral("[aA]bout[qQ]t")).exactMatch(cmd)){                    emit AboutQt();              return; }
    if(QRegExp(QStringLiteral("[qQ]uit")).exactMatch(cmd)){                          emit Quit();                 return; }

    if(QRegExp(QStringLiteral("(?:[tT]oggle)?[nN]otifier")).exactMatch(cmd)){        emit ToggleNotifier();       return; }
    if(QRegExp(QStringLiteral("(?:[tT]oggle)?[rR]eceiver")).exactMatch(cmd)){        emit ToggleReceiver();       return; }
    if(QRegExp(QStringLiteral("(?:[tT]oggle)?[mM]enu[bB]ar")).exactMatch(cmd)){      emit ToggleMenuBar();        return; }
    if(QRegExp(QStringLiteral("(?:[tT]oggle)?[tT]ree[bB]ar")).exactMatch(cmd)){      emit ToggleTreeBar();        return; }
    if(QRegExp(QStringLiteral("(?:[tT]oggle)?[tT]ool[bB]ar")).exactMatch(cmd)){      emit ToggleToolBar();        return; }
    if(QRegExp(QStringLiteral("(?:[tT]oggle)?[fF]ull[sS]creen")).exactMatch(cmd)){   emit ToggleFullScreen();     return; }
    if(QRegExp(QStringLiteral("[tT]oggle[mM]aximized")).exactMatch(cmd)){            emit ToggleMaximized();      return; }
    if(QRegExp(QStringLiteral("[mM]aximize(?:[wW]indow)?")).exactMatch(cmd)){        emit ToggleMaximized();      return; }
    if(QRegExp(QStringLiteral("[tT]oggle[mM]inimized")).exactMatch(cmd)){            emit ToggleMinimized();      return; }
    if(QRegExp(QStringLiteral("[mM]inimize(?:[wW]indow)?")).exactMatch(cmd)){        emit ToggleMinimized();      return; }
    if(QRegExp(QStringLiteral("[tT]oggle[sS]haded")).exactMatch(cmd)){               emit ToggleShaded();         return; }
    if(QRegExp(QStringLiteral("[sS]hade(?:[wW]indow)?")).exactMatch(cmd)){           emit ShadeWindow();          return; }
    if(QRegExp(QStringLiteral("[uU]nshade(?:[wW]indow)?")).exactMatch(cmd)){         emit UnshadeWindow();        return; }
    if(QRegExp(QStringLiteral("[nN]ew[wW]indow")).exactMatch(cmd)){                  emit NewWindow();            return; }
    if(QRegExp(QStringLiteral("[cC]lose[wW]indow")).exactMatch(cmd)){                emit CloseWindow();          return; }
    if(QRegExp(QStringLiteral("[sS]witch(?:[wW]indow)?")).exactMatch(cmd)){          emit SwitchWindow();         return; }
    if(QRegExp(QStringLiteral("[nN]ext[wW]indow")).exactMatch(cmd)){                 emit NextWindow();           return; }
    if(QRegExp(QStringLiteral("[pP]rev(?:ious)?[wW]indow")).exactMatch(cmd)){        emit PrevWindow();           return; }

    if(QRegExp(QStringLiteral("[bB]ack(?:ward)?")).exactMatch(cmd)){                 emit Back();                 return; }
    if(QRegExp(QStringLiteral("[fF]orward")).exactMatch(cmd)){                       emit Forward();              return; }
    if(QRegExp(QStringLiteral("[uU]pdir(?:ectory)?")).exactMatch(cmd)){              emit UpDirectory();          return; }
    if(QRegExp(QStringLiteral("[cC]lose")).exactMatch(cmd)){                         emit Close();                return; }
    if(QRegExp(QStringLiteral("[rR]estore")).exactMatch(cmd)){                       emit Restore();              return; }
    if(QRegExp(QStringLiteral("[rR]ecreate")).exactMatch(cmd)){                      emit Recreate();             return; }
    if(QRegExp(QStringLiteral("[nN]ext(?:[vV]iew)?")).exactMatch(cmd)){              emit NextView();             return; }
    if(QRegExp(QStringLiteral("[pP]rev(?:ious)?(?:[vV]iew)?")).exactMatch(cmd)){     emit PrevView();             return; }
    if(QRegExp(QStringLiteral("[bB]ury(?:[vV]iew)?")).exactMatch(cmd)){              emit BuryView();             return; }
    if(QRegExp(QStringLiteral("[dD]ig(?:[vV]iew)?")).exactMatch(cmd)){               emit DigView();              return; }
    if(QRegExp(QStringLiteral("[nN]ew(?:[vV]iew)?(?:[nN]ode)?")).exactMatch(cmd)){   emit NewViewNode();          return; }
    if(QRegExp(QStringLiteral("[nN]ew[hH]ist(?:[nN]ode)?")).exactMatch(cmd)){        emit NewHistNode();          return; }
    if(QRegExp(QStringLiteral("[cC]lone(?:[vV]iew)?(?:[nN]ode)?")).exactMatch(cmd)){ emit CloneViewNode();        return; }
    if(QRegExp(QStringLiteral("[cC]lone[hH]ist(?:[nN]ode)?")).exactMatch(cmd)){      emit CloneHistNode();        return; }
    if(QRegExp(QStringLiteral("[lL]ocal[nN]ode")).exactMatch(cmd)){                  emit MakeLocalNode();        return; }
    if(QRegExp(QStringLiteral("(?:[dD]isplay)?[aA]ccess[kK]ey")).exactMatch(cmd)){   emit DisplayAccessKey();     return; }
    if(QRegExp(QStringLiteral("(?:[dD]isplay)?[vV]iew[tT]ree")).exactMatch(cmd)){    emit DisplayViewTree();      return; }
    if(QRegExp(QStringLiteral("(?:[dD]isplay)?[hH]ist[tT]ree")).exactMatch(cmd)){    emit DisplayHistTree();      return; }
    if(QRegExp(QStringLiteral("(?:[dD]isplay)?[tT]rash[tT]ree")).exactMatch(cmd)){   emit DisplayTrashTree();     return; }
    if(QRegExp(QStringLiteral("[rR]elease[hH]idden[vV]iew")).exactMatch(cmd)){       emit ReleaseHiddenView();    return; }

    if(QRegExp(QStringLiteral("[cC]opy")).exactMatch(cmd)){                          emit Copy();                 return; }
    if(QRegExp(QStringLiteral("[cC]ut")).exactMatch(cmd)){                           emit Cut();                  return; }
    if(QRegExp(QStringLiteral("[pP]aste")).exactMatch(cmd)){                         emit Paste();                return; }
    if(QRegExp(QStringLiteral("[uU]ndo")).exactMatch(cmd)){                          emit Undo();                 return; }
    if(QRegExp(QStringLiteral("[rR]edo")).exactMatch(cmd)){                          emit Redo();                 return; }
    if(QRegExp(QStringLiteral("[sS]elect[aA]ll")).exactMatch(cmd)){                  emit SelectAll();            return; }
    if(QRegExp(QStringLiteral("[uU]n[sS]elect")).exactMatch(cmd)){                   emit Unselect();             return; }
    if(QRegExp(QStringLiteral("[rR]eload")).exactMatch(cmd)){                        emit Reload();               return; }
    if(QRegExp(QStringLiteral("[rR]eload[aA]nd[bB]ypass[cC]ache")).exactMatch(cmd)){ emit ReloadAndBypassCache(); return; }
    if(QRegExp(QStringLiteral("[sS]top")).exactMatch(cmd)){                          emit Stop();                 return; }
    if(QRegExp(QStringLiteral("[sS]top[aA]nd[uU]n[sS]elect")).exactMatch(cmd)){      emit StopAndUnselect();      return; }

    if(QRegExp(QStringLiteral("[pP]rint")).exactMatch(cmd)){                         emit Print();                return; }
    if(QRegExp(QStringLiteral("[sS]ave")).exactMatch(cmd)){                          emit Save();                 return; }
    if(QRegExp(QStringLiteral("[zZ]oom[iI]n")).exactMatch(cmd)){                     emit ZoomIn();               return; }
    if(QRegExp(QStringLiteral("[zZ]oom[oO]ut")).exactMatch(cmd)){                    emit ZoomOut();              return; }
    if(QRegExp(QStringLiteral("[vV]iew[sS]ource")).exactMatch(cmd)){                 emit ViewSource();           return; }
    if(QRegExp(QStringLiteral("[aA]pply[sS]ource")).exactMatch(cmd)){                emit ApplySource();          return; }

    if(QRegExp(QStringLiteral("[cC]opy[uU]rl")).exactMatch(cmd)){                    emit CopyUrl();              return; }
    if(QRegExp(QStringLiteral("[cC]opy[tT]itle")).exactMatch(cmd)){                  emit CopyTitle();            return; }
    if(QRegExp(QStringLiteral("[cC]opy[pP]age[aA]s[lL]ink")).exactMatch(cmd)){       emit CopyPageAsLink();       return; }
    if(QRegExp(QStringLiteral("[cC]opy[sS]elected[hH]tml")).exactMatch(cmd)){        emit CopySelectedHtml();     return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?")).exactMatch(cmd)){ emit OpenWithIE(); return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[eE]dge")).exactMatch(cmd)){  emit OpenWithEdge();         return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?")).exactMatch(cmd)){ emit OpenWithFF(); return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[oO]pera")).exactMatch(cmd)){ emit OpenWithOpera();        return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[oO][pP][rR]")).exactMatch(cmd)){ emit OpenWithOPR();      return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[sS]afari")).exactMatch(cmd)){ emit OpenWithSafari();      return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[cC]hrome")).exactMatch(cmd)){ emit OpenWithChrome();      return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[sS]leipnir")).exactMatch(cmd)){ emit OpenWithSleipnir();  return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[vV]ivaldi")).exactMatch(cmd)){ emit OpenWithVivaldi();    return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[wW]ith|[oO]n)[cC]ustom")).exactMatch(cmd)){ emit OpenWithCustom();      return; }

    if(QRegExp(QStringLiteral("[cC]lick(?:[eE]lement)?")).exactMatch(cmd)){                                           emit TriggerElementAction(Page::We_ClickElement);                      return; }
    if(QRegExp(QStringLiteral("[fF]ocus(?:[eE]lement)?")).exactMatch(cmd)){                                           emit TriggerElementAction(Page::We_FocusElement);                      return; }
    if(QRegExp(QStringLiteral("[hH]over(?:[eE]lement)?")).exactMatch(cmd)){                                           emit TriggerElementAction(Page::We_HoverElement);                      return; }

    if(QRegExp(QStringLiteral("[lL]oad[lL]ink")).exactMatch(cmd)){                                                   emit TriggerElementAction(Page::We_LoadLink);                          return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink")).exactMatch(cmd)){                                                   emit TriggerElementAction(Page::We_OpenLink);                          return; }
    if(QRegExp(QStringLiteral("[dD]ownload[lL]ink")).exactMatch(cmd)){                                               emit TriggerElementAction(Page::We_DownloadLink);                      return; }
    if(QRegExp(QStringLiteral("[cC]opy[lL]ink[uU]rl")).exactMatch(cmd)){                                             emit TriggerElementAction(Page::We_CopyLinkUrl);                       return; }
    if(QRegExp(QStringLiteral("[cC]opy[lL]ink[hH]tml")).exactMatch(cmd)){                                            emit TriggerElementAction(Page::We_CopyLinkHtml);                      return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?")).exactMatch(cmd)){  emit TriggerElementAction(Page::We_OpenLinkWithIE);                    return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[eE]dge")).exactMatch(cmd)){                           emit TriggerElementAction(Page::We_OpenLinkWithEdge);                  return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?")).exactMatch(cmd)){           emit TriggerElementAction(Page::We_OpenLinkWithFF);                    return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[oO]pera")).exactMatch(cmd)){                          emit TriggerElementAction(Page::We_OpenLinkWithOpera);                 return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[oO][pP][rR]")).exactMatch(cmd)){                      emit TriggerElementAction(Page::We_OpenLinkWithOPR);                   return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[sS]afari")).exactMatch(cmd)){                         emit TriggerElementAction(Page::We_OpenLinkWithSafari);                return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[cC]hrome")).exactMatch(cmd)){                         emit TriggerElementAction(Page::We_OpenLinkWithChrome);                return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[sS]leipnir")).exactMatch(cmd)){                       emit TriggerElementAction(Page::We_OpenLinkWithSleipnir);              return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[vV]ivaldi")).exactMatch(cmd)){                        emit TriggerElementAction(Page::We_OpenLinkWithVivaldi);               return; }
    if(QRegExp(QStringLiteral("[oO]pen[lL]ink(?:[wW]ith|[oO]n)[cC]ustom")).exactMatch(cmd)){                         emit TriggerElementAction(Page::We_OpenLinkWithCustom);                return; }

    if(QRegExp(QStringLiteral("[lL]oad[iI]mage")).exactMatch(cmd)){                                                  emit TriggerElementAction(Page::We_LoadImage);                         return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage")).exactMatch(cmd)){                                                  emit TriggerElementAction(Page::We_OpenImage);                         return; }
    if(QRegExp(QStringLiteral("[dD]ownload[iI]mage")).exactMatch(cmd)){                                              emit TriggerElementAction(Page::We_DownloadImage);                     return; }
    if(QRegExp(QStringLiteral("[cC]opy[iI]mage")).exactMatch(cmd)){                                                  emit TriggerElementAction(Page::We_CopyImage);                         return; }
    if(QRegExp(QStringLiteral("[cC]opy[iI]mage[uU]rl")).exactMatch(cmd)){                                            emit TriggerElementAction(Page::We_CopyImageUrl);                      return; }
    if(QRegExp(QStringLiteral("[cC]opy[iI]mage[hH]tml")).exactMatch(cmd)){                                           emit TriggerElementAction(Page::We_CopyImageHtml);                     return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?")).exactMatch(cmd)){ emit TriggerElementAction(Page::We_OpenImageWithIE);                   return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[eE]dge")).exactMatch(cmd)){                          emit TriggerElementAction(Page::We_OpenImageWithEdge);                 return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?")).exactMatch(cmd)){          emit TriggerElementAction(Page::We_OpenImageWithFF);                   return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[oO]pera")).exactMatch(cmd)){                         emit TriggerElementAction(Page::We_OpenImageWithOpera);                return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[oO][pP][rR]")).exactMatch(cmd)){                     emit TriggerElementAction(Page::We_OpenImageWithOPR);                  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[sS]afari")).exactMatch(cmd)){                        emit TriggerElementAction(Page::We_OpenImageWithSafari);               return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[cC]hrome")).exactMatch(cmd)){                        emit TriggerElementAction(Page::We_OpenImageWithChrome);               return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[sS]leipnir")).exactMatch(cmd)){                      emit TriggerElementAction(Page::We_OpenImageWithSleipnir);             return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[vV]ivaldi")).exactMatch(cmd)){                       emit TriggerElementAction(Page::We_OpenImageWithVivaldi);              return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage(?:[wW]ith|[oO]n)[cC]ustom")).exactMatch(cmd)){                        emit TriggerElementAction(Page::We_OpenImageWithCustom);               return; }

    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode")).exactMatch(cmd)){                 emit TriggerElementAction(Page::We_OpenInNewViewNode);                 return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode")).exactMatch(cmd)){                 emit TriggerElementAction(Page::We_OpenInNewHistNode);                 return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory")).exactMatch(cmd)){                   emit TriggerElementAction(Page::We_OpenInNewDirectory);                return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot")).exactMatch(cmd)){                              emit TriggerElementAction(Page::We_OpenOnRoot);                        return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground")).exactMatch(cmd)){    emit TriggerElementAction(Page::We_OpenInNewViewNodeForeground);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[fF]oreground")).exactMatch(cmd)){    emit TriggerElementAction(Page::We_OpenInNewHistNodeForeground);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[fF]oreground")).exactMatch(cmd)){      emit TriggerElementAction(Page::We_OpenInNewDirectoryForeground);      return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[fF]oreground")).exactMatch(cmd)){                 emit TriggerElementAction(Page::We_OpenOnRootForeground);              return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground")).exactMatch(cmd)){    emit TriggerElementAction(Page::We_OpenInNewViewNodeBackground);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[bB]ackground")).exactMatch(cmd)){    emit TriggerElementAction(Page::We_OpenInNewHistNodeBackground);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[bB]ackground")).exactMatch(cmd)){      emit TriggerElementAction(Page::We_OpenInNewDirectoryBackground);      return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[bB]ackground")).exactMatch(cmd)){                 emit TriggerElementAction(Page::We_OpenOnRootBackground);              return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow")).exactMatch(cmd)){ emit TriggerElementAction(Page::We_OpenInNewViewNodeThisWindow);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[tT]his[wW]indow")).exactMatch(cmd)){ emit TriggerElementAction(Page::We_OpenInNewHistNodeThisWindow);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[tT]his[wW]indow")).exactMatch(cmd)){   emit TriggerElementAction(Page::We_OpenInNewDirectoryThisWindow);      return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[tT]his[wW]indow")).exactMatch(cmd)){              emit TriggerElementAction(Page::We_OpenOnRootThisWindow);              return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow")).exactMatch(cmd)){  emit TriggerElementAction(Page::We_OpenInNewViewNodeNewWindow);        return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[hH]ist[nN]ode[nN]ew[wW]indow")).exactMatch(cmd)){  emit TriggerElementAction(Page::We_OpenInNewHistNodeNewWindow);        return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow")).exactMatch(cmd)){    emit TriggerElementAction(Page::We_OpenInNewDirectoryNewWindow);       return; }
    if(QRegExp(QStringLiteral("[oO]pen(?:[lL]ink)?[oO]n[rR]oot[nN]ew[wW]indow")).exactMatch(cmd)){               emit TriggerElementAction(Page::We_OpenOnRootNewWindow);               return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode")).exactMatch(cmd)){                     emit TriggerElementAction(Page::We_OpenImageInNewViewNode);            return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode")).exactMatch(cmd)){                     emit TriggerElementAction(Page::We_OpenImageInNewHistNode);            return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory")).exactMatch(cmd)){                       emit TriggerElementAction(Page::We_OpenImageInNewDirectory);           return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot")).exactMatch(cmd)){                                  emit TriggerElementAction(Page::We_OpenImageOnRoot);                   return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[fF]oreground")).exactMatch(cmd)){        emit TriggerElementAction(Page::We_OpenImageInNewViewNodeForeground);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[fF]oreground")).exactMatch(cmd)){        emit TriggerElementAction(Page::We_OpenImageInNewHistNodeForeground);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[fF]oreground")).exactMatch(cmd)){          emit TriggerElementAction(Page::We_OpenImageInNewDirectoryForeground); return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[fF]oreground")).exactMatch(cmd)){                     emit TriggerElementAction(Page::We_OpenImageOnRootForeground);         return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[bB]ackground")).exactMatch(cmd)){        emit TriggerElementAction(Page::We_OpenImageInNewViewNodeBackground);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[bB]ackground")).exactMatch(cmd)){        emit TriggerElementAction(Page::We_OpenImageInNewHistNodeBackground);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[bB]ackground")).exactMatch(cmd)){          emit TriggerElementAction(Page::We_OpenImageInNewDirectoryBackground); return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[bB]ackground")).exactMatch(cmd)){                     emit TriggerElementAction(Page::We_OpenImageOnRootBackground);         return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[tT]his[wW]indow")).exactMatch(cmd)){     emit TriggerElementAction(Page::We_OpenImageInNewViewNodeThisWindow);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[tT]his[wW]indow")).exactMatch(cmd)){     emit TriggerElementAction(Page::We_OpenImageInNewHistNodeThisWindow);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[tT]his[wW]indow")).exactMatch(cmd)){       emit TriggerElementAction(Page::We_OpenImageInNewDirectoryThisWindow); return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[tT]his[wW]indow")).exactMatch(cmd)){                  emit TriggerElementAction(Page::We_OpenImageOnRootThisWindow);         return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[vV]iew[nN]ode[nN]ew[wW]indow")).exactMatch(cmd)){      emit TriggerElementAction(Page::We_OpenImageInNewViewNodeNewWindow);   return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[hH]ist[nN]ode[nN]ew[wW]indow")).exactMatch(cmd)){      emit TriggerElementAction(Page::We_OpenImageInNewHistNodeNewWindow);   return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[iI]n[nN]ew[dD]irectory[nN]ew[wW]indow")).exactMatch(cmd)){        emit TriggerElementAction(Page::We_OpenImageInNewDirectoryNewWindow);  return; }
    if(QRegExp(QStringLiteral("[oO]pen[iI]mage[oO]n[rR]oot[nN]ew[wW]indow")).exactMatch(cmd)){                   emit TriggerElementAction(Page::We_OpenImageOnRootNewWindow);          return; }

    if(QRegExp(QStringLiteral("[dD]eactivate")).exactMatch(cmd)){                                         emit Deactivate();                      return; }
    if(QRegExp(QStringLiteral("[rR]efresh")).exactMatch(cmd)){                                            emit Refresh();                         return; }
    if(QRegExp(QStringLiteral("[rR]efresh[nN]o[sS]croll")).exactMatch(cmd)){                              emit RefreshNoScroll();                 return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode")).exactMatch(cmd)){                                        emit OpenNode();                        return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode[oO]n[nN]ew[wW]indow")).exactMatch(cmd)){                    emit OpenNodeOnNewWindow();             return; }
    if(QRegExp(QStringLiteral("[dD]elete[nN]ode")).exactMatch(cmd)){                                      emit DeleteNode();                      return; }
    if(QRegExp(QStringLiteral("[dD]elete[rR]ight[nN]ode")).exactMatch(cmd)){                              emit DeleteRightNode();                 return; }
    if(QRegExp(QStringLiteral("[dD]elete[lL]eft[nN]ode")).exactMatch(cmd)){                               emit DeleteLeftNode();                  return; }
    if(QRegExp(QStringLiteral("[dD]elete[oO]ther[nN]ode")).exactMatch(cmd)){                              emit DeleteOtherNode();                 return; }
    if(QRegExp(QStringLiteral("[pP]aste[nN]ode")).exactMatch(cmd)){                                       emit PasteNode();                       return; }
    if(QRegExp(QStringLiteral("[rR]estore[nN]ode")).exactMatch(cmd)){                                     emit RestoreNode();                     return; }
    if(QRegExp(QStringLiteral("[nN]ew[nN]ode")).exactMatch(cmd)){                                         emit NewNode();                         return; }
    if(QRegExp(QStringLiteral("[cC]lone[nN]ode")).exactMatch(cmd)){                                       emit CloneNode();                       return; }
    if(QRegExp(QStringLiteral("[uU]p[dD]irectory")).exactMatch(cmd)){                                     emit UpDirectory();                     return; }
    if(QRegExp(QStringLiteral("[dD]own[dD]irectory")).exactMatch(cmd)){                                   emit DownDirectory();                   return; }
    if(QRegExp(QStringLiteral("[mM]ake[lL]ocal[nN]ode")).exactMatch(cmd)){                                emit MakeLocalNode();                   return; }
    if(QRegExp(QStringLiteral("[mM]ake[dD]irectory")).exactMatch(cmd)){                                   emit MakeDirectory();                   return; }
    if(QRegExp(QStringLiteral("[mM]ake[dD]irectory[wW]ith[sS]elected[nN]ode")).exactMatch(cmd)){          emit MakeDirectoryWithSelectedNode();   return; }
    if(QRegExp(QStringLiteral("[mM]ake[dD]irectory[wW]ith[sS]ame[dD]omain[nN]ode")).exactMatch(cmd)){     emit MakeDirectoryWithSameDomainNode(); return; }
    if(QRegExp(QStringLiteral("[rR]ename[nN]ode")).exactMatch(cmd)){                                      emit RenameNode();                      return; }
    if(QRegExp(QStringLiteral("[cC]opy[nN]ode[uU]rl")).exactMatch(cmd)){                                  emit CopyNodeUrl();                     return; }
    if(QRegExp(QStringLiteral("[cC]opy[nN]ode[tT]itle")).exactMatch(cmd)){                                emit CopyNodeTitle();                   return; }
    if(QRegExp(QStringLiteral("[cC]opy[nN]ode[aA]s[lL]ink")).exactMatch(cmd)){                            emit CopyNodeAsLink();                  return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[iI](?:nternet)?[eE](?:xplorer)?")).exactMatch(cmd)){ emit OpenNodeWithIE();        return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[eE]dge")).exactMatch(cmd)){                emit OpenNodeWithEdge();                return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[fF](?:ire)?[fF](?:ox)?")).exactMatch(cmd)){ emit OpenNodeWithFF();                 return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[oO]pera")).exactMatch(cmd)){               emit OpenNodeWithOpera();               return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[oO][pP][rR]")).exactMatch(cmd)){           emit OpenNodeWithOPR();                 return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[sS]afari")).exactMatch(cmd)){              emit OpenNodeWithSafari();              return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[cC]hrome")).exactMatch(cmd)){              emit OpenNodeWithChrome();              return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[sS]leipnir")).exactMatch(cmd)){            emit OpenNodeWithSleipnir();            return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[vV]ivaldi")).exactMatch(cmd)){             emit OpenNodeWithVivaldi();             return; }
    if(QRegExp(QStringLiteral("[oO]pen[nN]ode(?:[wW]ith|[oO]n)[cC]ustom")).exactMatch(cmd)){              emit OpenNodeWithCustom();              return; }

    if(QRegExp(QStringLiteral("[tT]oggle[tT]rash")).exactMatch(cmd)){                                     emit ToggleTrash();                     return; }
    if(QRegExp(QStringLiteral("[sS]croll[uU]p")).exactMatch(cmd)){                                        emit ScrollUp();                        return; }
    if(QRegExp(QStringLiteral("[sS]croll[dD]own")).exactMatch(cmd)){                                      emit ScrollDown();                      return; }
    if(QRegExp(QStringLiteral("[pP]age[uU]p")).exactMatch(cmd)){                                          emit PageUp();                          return; }
    if(QRegExp(QStringLiteral("[pP]age[dD]own")).exactMatch(cmd)){                                        emit PageDown();                        return; }
    if(QRegExp(QStringLiteral("[zZ]oom[iI]n")).exactMatch(cmd)){                                          emit ZoomIn();                          return; }
    if(QRegExp(QStringLiteral("[zZ]oom[oO]ut")).exactMatch(cmd)){                                         emit ZoomOut();                         return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[uU]pper[iI]tem")).exactMatch(cmd)){                           emit MoveToUpperItem();                 return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[lL]ower[iI]tem")).exactMatch(cmd)){                           emit MoveToLowerItem();                 return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[rR]ight[iI]tem")).exactMatch(cmd)){                           emit MoveToRightItem();                 return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[lL]eft[iI]tem")).exactMatch(cmd)){                            emit MoveToLeftItem();                  return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[pP]rev[pP]age")).exactMatch(cmd)){                            emit MoveToPrevPage();                  return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[nN]ext[pP]age")).exactMatch(cmd)){                            emit MoveToNextPage();                  return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[fF]irst[iI]tem")).exactMatch(cmd)){                           emit MoveToFirstItem();                 return; }
    if(QRegExp(QStringLiteral("[mM]ove[tT]o[lL]ast[iI]tem")).exactMatch(cmd)){                            emit MoveToLastItem();                  return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[uU]pper[iI]tem")).exactMatch(cmd)){                         emit SelectToUpperItem();               return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[lL]ower[iI]tem")).exactMatch(cmd)){                         emit SelectToLowerItem();               return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[rR]ight[iI]tem")).exactMatch(cmd)){                         emit SelectToRightItem();               return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[lL]eft[iI]tem")).exactMatch(cmd)){                          emit SelectToLeftItem();                return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[pP]rev[pP]age")).exactMatch(cmd)){                          emit SelectToPrevPage();                return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[nN]ext[pP]age")).exactMatch(cmd)){                          emit SelectToNextPage();                return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[fF]irst[iI]tem")).exactMatch(cmd)){                         emit SelectToFirstItem();               return; }
    if(QRegExp(QStringLiteral("[sS]elect[tT]o[lL]ast[iI]tem")).exactMatch(cmd)){                          emit SelectToLastItem();                return; }
    if(QRegExp(QStringLiteral("[sS]elect[iI]tem")).exactMatch(cmd)){                                      emit SelectItem();                      return; }
    if(QRegExp(QStringLiteral("[sS]elect[rR]ange")).exactMatch(cmd)){                                     emit SelectRange();                     return; }
    if(QRegExp(QStringLiteral("[sS]elect[aA]ll")).exactMatch(cmd)){                                       emit SelectAll();                       return; }
    if(QRegExp(QStringLiteral("[cC]lear[sS]election")).exactMatch(cmd)){                                  emit ClearSelection();                  return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[uU]pper")).exactMatch(cmd)){                              emit TransferToUpper();                 return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[lL]ower")).exactMatch(cmd)){                              emit TransferToLower();                 return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[rR]ight")).exactMatch(cmd)){                              emit TransferToRight();                 return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[lL]eft")).exactMatch(cmd)){                               emit TransferToLeft();                  return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[pP]rev[pP]age")).exactMatch(cmd)){                        emit TransferToPrevPage();              return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[nN]ext[pP]age")).exactMatch(cmd)){                        emit TransferToNextPage();              return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[fF]irst")).exactMatch(cmd)){                              emit TransferToFirst();                 return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[lL]ast")).exactMatch(cmd)){                               emit TransferToLast();                  return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[uU]p[dD]irectory")).exactMatch(cmd)){                     emit TransferToUpDirectory();           return; }
    if(QRegExp(QStringLiteral("[tT]ransfer[tT]o[dD]own[dD]irectory")).exactMatch(cmd)){                   emit TransferToDownDirectory();         return; }
    if(QRegExp(QStringLiteral("[sS]witch[nN]ode[cC]ollection[tT]ype")).exactMatch(cmd)){                  emit SwitchNodeCollectionType();        return; }
    if(QRegExp(QStringLiteral("[sS]witch[nN]ode[cC]ollection[tT]ype[rR]everse")).exactMatch(cmd)){        emit SwitchNodeCollectionTypeReverse(); return; }

    if(QRegExp(QStringLiteral("[rR]econf(?:ig(?:ure)?)?")).exactMatch(cmd)){                              emit Reconfigure();                     return; }
    if(QRegExp(QStringLiteral("[bB]lank")).exactMatch(cmd)){                                              emit OpenUrl(QUrl(QStringLiteral("about:blank"))); return; }

    if(list.isEmpty() && Page::GetBookmarkletMap().contains(cmd)){

        emit OpenBookmarklet(Page::GetBookmarklet(cmd).first());

    } else if(QRegExp(QStringLiteral("[oO]pen")).exactMatch(cmd)){

        QStringList args;
        foreach(QString arg, list){
            if(arg == QStringLiteral("blank")) args << QStringLiteral("about:blank");
            else               args << arg;
        }
        emit OpenUrl(Page::DirtyStringToUrlList(args.join(QStringLiteral(" "))));

    } else if(QRegExp(QStringLiteral("[lL]oad")).exactMatch(cmd)){

        QString args = list.join(QStringLiteral(" "));
        if(args == QStringLiteral("blank")) emit OpenUrl(QUrl(QStringLiteral("about:blank")));
        else                emit OpenUrl(QUrl(args));

    } else if(QRegExp(QStringLiteral("[qQ]uery")).exactMatch(cmd)){

        emit OpenQueryUrl(list.join(QStringLiteral(" ")));

    } else if(QRegExp(QStringLiteral("[dD]ownload")).exactMatch(cmd)){

        if(list.length() >= 2) // ignore excess arguments.
            emit Download(list[0], list[1]);

    } else if(QRegExp(QStringLiteral("[sS]earch|[sS]eek")).exactMatch(cmd)){

        if(Application::keyboardModifiers() & Qt::ShiftModifier){
            emit SeekText(list.join(QStringLiteral(" ")), View::WrapsAroundDocument | View::FindBackward);
        } else {
            emit SeekText(list.join(QStringLiteral(" ")), View::WrapsAroundDocument);
        }

    } else if(QRegExp(QStringLiteral("[sS]et(?:tings?)?")).exactMatch(cmd)){

        if(list.length() >= 2){ // ignore excess arguments.
            QString key = list.takeFirst();
            QString val = list.join(" ");
            Application::GlobalSettings()->setValue(key, val);
            emit Reconfigure();
        }

    } else if(QRegExp(QStringLiteral("[uU]n[sS]et(?:tings?)?")).exactMatch(cmd)){

        if(list.length() >= 1){ // ignore excess arguments.
            Application::GlobalSettings()->remove(list[0]);
            emit Reconfigure();
        }

    } else if(QRegExp(QStringLiteral("[kK]ey")).exactMatch(cmd)){

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
        emit OpenUrl(Page::DirtyStringToUrlList(args.join(QStringLiteral(" "))));
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

    if(IsPurged())
        QTimer::singleShot(0, win, &MainWindow::SetFocus);
    else win->SetFocus();

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
            QStringList list = key.split(QRegExp(QStringLiteral("[\n\r\t ]+")));

            if(QRegExp(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?")).exactMatch(list[0])){
                QSettings *settings = Application::GlobalSettings();
                if(!settings->value(list[1]).toString().isEmpty())
                    list << settings->value(list[1]).toString();
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
    QStringList list = m_LineString.split(QRegExp(QStringLiteral("[\n\r\t ]+")));
    if(m_Mode == Command &&
       QRegExp(QStringLiteral("(?:[uU]n)?[sS]et(?:tings?)?")).exactMatch(list[0])){
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
