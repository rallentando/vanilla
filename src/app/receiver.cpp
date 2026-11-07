#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

#include "receiver.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QHideEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QTimerEvent>
#include <QPointer>
#include <QLocalServer>
#include <QLocalSocket>
#include "searchsuggest.hpp"
#include <QDomElement>
#include <QUrl>
#include <QUrlQuery>

#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "gadgets.hpp"
#include "blurbehind.hpp"
#include "settingspage.hpp"
#include "settingsschema.hpp"
#include "view.hpp"
#include "commandmap.hpp"
#include "commandframe.hpp"
#include "editmenu.hpp"

QLocalServer *Receiver::m_LocalServer = nullptr;
QStringList Receiver::m_History;

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    ApplyTheme();
    setDragEnabled(true);
    EditMenu::Install(this);
}

LineEdit::~LineEdit(){}

void LineEdit::SetPill(bool on){
    m_Pill = on;
    ApplyTheme();
}

void LineEdit::ApplyTheme(){
    if(m_Pill){
        const int height = ScaleByDevice(RECEIVER_FIELD_HEIGHT);
        setStyleSheet(QStringLiteral("QLineEdit{ background: %1; color: %2; placeholder-text-color: %3;"
                                     " border: 1px solid %4; border-radius: %5px; padding-left: %6px; padding-right: %6px;}"
                                     "QLineEdit:focus{ border-color: %7;}")
                      .arg(Theme::StyleSheetColor(Theme::VanillaPageFieldBackground),
                           Theme::StyleSheetColor(Theme::VanillaPageText),
                           Theme::StyleSheetColor(Theme::VanillaPageTextDim),
                           Theme::StyleSheetColor(Theme::VanillaPageBorder))
                      .arg(height / 2)
                      .arg(ScaleByDevice(RECEIVER_FIELD_PADDING_X))
                      .arg(Theme::StyleSheetColor(Theme::VanillaPageAccent)));
        return;
    }

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

bool LineEdit::event(QEvent *ev){
    if(ev->type() == QEvent::KeyPress){
        QKeyEvent *key = static_cast<QKeyEvent*>(ev);
        if((key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) &&
           !(key->modifiers() & (Qt::ControlModifier | Qt::AltModifier))){
            if(Receiver *receiver = qobject_cast<Receiver*>(parentWidget())){
                if(receiver->HandleMappedKey(key)) return true;
            }
        }
    }
    return QLineEdit::event(ev);
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
    if(Receiver *receiver = qobject_cast<Receiver*>(parentWidget())){
        if(receiver->HandleMappedKey(ev)){
            ev->setAccepted(true);
            return;
        }
    }
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
    QString shown = text();
    shown.insert(cursorPosition(), ev->preeditString());
    emit textChanged(shown);
}

void LineEdit::mousePressEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton && selectedText() == text()) deselect();
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
    , m_Mode(Command)
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
    m_LineEdit->SetPill(true);
    setFocusProxy(m_LineEdit);
    m_LineEdit->show();
    hide();

    m_LineString = QString();
    m_SuggestStrings = QStringList();
    m_CurrentSuggestIndex = -1;

    connect(m_LineEdit, &LineEdit::FocusIn, m_LineEdit, &QLineEdit::selectAll);
    connect(m_LineEdit, &LineEdit::textChanged,        this, &Receiver::SetString);
    connect(m_LineEdit, &LineEdit::FocusOut,           this, &Receiver::OnFieldFocusOut);
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

    const bool center = CurrentPlacement() == Center;
    const int margin = ScaleByDevice(RECEIVER_MARGIN);
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int field = ScaleByDevice(RECEIVER_FIELD_HEIGHT);
    const int gap = ScaleByDevice(RECEIVER_LIST_GAP);
    const int row = ScaleByDevice(SUGGEST_HEIGHT);

    const int right = size.width() - m_TreeBank->MiniMapStripWidth();
    const int w = qMin(ScaleByDevice(center ? RECEIVER_CENTER_WIDTH : RECEIVER_CORNER_WIDTH),
                       qMax(right - margin * 2, 1));
    const int top = center ? size.height() / 12 : margin;
    const int bar = ShowsSourceBar() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0;
    const int room = qMax(size.height() - top * 2 - pad * 2 - field - gap - bar, 0);
    int rows = 0, used = 0;
    for(; rows < m_SuggestStrings.length(); rows++){
        used += row + (HasHeading(rows) ? ScaleByDevice(RECEIVER_HEADING_HEIGHT) : 0);
        if(used > room) break;
    }
    if(rows < m_SuggestStrings.length())
        used -= row + (HasHeading(rows) ? ScaleByDevice(RECEIVER_HEADING_HEIGHT) : 0);
    if(m_ActiveTab != 0 && rows < m_SuggestStrings.length()) used = room;
    const int h = pad * 2 + field + (rows || bar ? gap + bar + used : 0);
    if(m_ActiveTab == 0 && m_CurrentSuggestIndex >= rows) m_CurrentSuggestIndex = -1;

    QPoint pos(center ? (right - w) / 2 : right - w - margin, top);
    if(IsPurged()) pos = m_TreeBank->mapToGlobal(pos);
    setGeometry(QRect(pos, QSize(w, h)));
    m_LineEdit->setGeometry(pad, pad, w - pad * 2, field);
    KeepSelectionShown();
}

bool Receiver::SeekWords(const QString &line, QString *words){
    static const QRegularExpression form(QStringLiteral("^(\\S+)[\\n\\r\\t ]+(.*)$"),
                                         QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = form.match(line);
    if(!m.hasMatch()) return false;
    QString action;
    if(CommandMap::Resolve(m.captured(1), &action) != CommandMap::Seek) return false;
    if(words) *words = m.captured(2);
    return true;
}

bool Receiver::IsSeeking() const {
    return m_Mode == Search || (m_Mode == Command && m_ActiveTab == 0 && SeekWords(m_LineString));
}

Receiver::Placement Receiver::CurrentPlacement() const {
    return IsSeeking() ? TopRight : Center;
}

bool Receiver::HasHeading(int index) const {
    if(m_Rows.isEmpty() || index >= m_Rows.length()) return false;
    if(m_ActiveTab != 0){
        return !m_Rows[index].section.isEmpty() &&
            (index == 0 || m_Rows[index].section != m_Rows[index - 1].section);
    }
    return index == 0 || m_Rows[index].source != m_Rows[index - 1].source;
}

int Receiver::VisibleSuggestCount() const {
    const int bottom = height() - ScaleByDevice(RECEIVER_PADDING);
    int count = 0;
    while(m_SuggestScroll + count < m_SuggestStrings.length() &&
          SuggestRect(m_SuggestScroll + count).bottom() < bottom) count++;
    return count;
}

int Receiver::SelectableSuggestCount() const {
    return m_ActiveTab == 0 ? VisibleSuggestCount() : static_cast<int>(m_SuggestStrings.length());
}

int Receiver::LastSuggestScroll() const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int row = ScaleByDevice(SUGGEST_HEIGHT);
    const int room = height() - pad * 2 - ScaleByDevice(RECEIVER_FIELD_HEIGHT) - ScaleByDevice(RECEIVER_LIST_GAP)
        - (ShowsSourceBar() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0);
    int used = 0;
    for(int i = m_SuggestStrings.length() - 1; i >= 0; i--){
        used += row + (HasHeading(i) ? ScaleByDevice(RECEIVER_HEADING_HEIGHT) : 0);
        if(used > room) return qMin(i + 1, static_cast<int>(m_SuggestStrings.length()) - 1);
    }
    return 0;
}

void Receiver::KeepSelectionShown(){
    const int last = LastSuggestScroll();
    m_SuggestScroll = qBound(0, m_SuggestScroll, qMax(0, last));
    if(m_CurrentSuggestIndex < 0 || m_SuggestStrings.isEmpty()) return;
    if(m_CurrentSuggestIndex < m_SuggestScroll) m_SuggestScroll = m_CurrentSuggestIndex;
    if(!VisibleSuggestCount()){
        m_CurrentSuggestIndex = -1;
        return;
    }
    while(m_SuggestScroll < last && m_CurrentSuggestIndex >= m_SuggestScroll + VisibleSuggestCount())
        m_SuggestScroll++;
}

void Receiver::ScrollSuggestsBy(int rows){
    if(m_ActiveTab == 0 || !VisibleSuggestCount() || VisibleSuggestCount() >= m_SuggestStrings.length()) return;
    m_SuggestScroll = qBound(0, m_SuggestScroll + rows, qMax(0, LastSuggestScroll()));
    if(m_CurrentSuggestIndex != -1)
        m_CurrentSuggestIndex = qBound(m_SuggestScroll, m_CurrentSuggestIndex,
                                       m_SuggestScroll + VisibleSuggestCount() - 1);
    repaint();
}

QRect Receiver::SuggestRect(int index) const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int row = ScaleByDevice(SUGGEST_HEIGHT);
    int top = pad + ScaleByDevice(RECEIVER_FIELD_HEIGHT) + ScaleByDevice(RECEIVER_LIST_GAP)
        + (index - m_SuggestScroll) * row
        + (ShowsSourceBar() ? ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT) : 0);
    for(int i = m_SuggestScroll; i <= index; i++){
        if(HasHeading(i)) top += ScaleByDevice(RECEIVER_HEADING_HEIGHT);
    }
    return QRect(pad, top, width() - pad * 2, row);
}

int Receiver::SuggestAt(const QPoint &pos) const {
    const int count = VisibleSuggestCount();
    for(int i = m_SuggestScroll; i < m_SuggestScroll + count; i++){
        if(SuggestRect(i).contains(pos)) return i;
    }
    return -1;
}

void Receiver::Dismiss(){
    if(isVisible() && IsSeeking() && !m_TreeBank->IsDisplayingTableView())
        emit SeekText(QString(), View::HighlightAllOccurrences);
    hide();
}

bool Receiver::ShouldCloseOnFocusLeft(bool center, bool fieldHasFocus,
                                      bool popupIsUp, bool anyWindowActive){
    return center && !fieldHasFocus && !popupIsUp && anyWindowActive;
}

void Receiver::OnFieldFocusOut(){
    if(CurrentPlacement() != Center) return;
    QTimer::singleShot(0, this, &Receiver::CloseIfFocusLeft);
}

void Receiver::CloseIfFocusLeft(){
    if(!isVisible()) return;
    if(!ShouldCloseOnFocusLeft(CurrentPlacement() == Center, m_LineEdit->hasFocus(),
                               QApplication::activePopupWidget() != nullptr,
                               QApplication::activeWindow() != nullptr))
        return;
    m_FocusWentElsewhere = true;
    hide();
}

void Receiver::InitializeDisplay(Mode mode){
    bool v = isVisible();

    if(mode == Search){
        if(m_Mode != Search){
            m_LineEdit->setText(QString());
            m_LineString = QString();
            m_SuggestStrings = QStringList();
            m_Rows.clear();
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
        m_Rows.clear();
        m_CurrentSuggestIndex = -1;
    }

    m_Mode = mode;
    m_ActiveTab = 0;
    m_WebSuggests.clear();
    ReadBindings();

    if(m_Mode == Command) SuggestFromSources(QString());

    switch(m_Mode){
    case Command: m_LineEdit->setPlaceholderText(tr("Command Mode.")); break;
    case Query:   m_LineEdit->setPlaceholderText(tr("Input Query.")); break;
    case UrlEdit: m_LineEdit->setPlaceholderText(tr("Enter a URL.")); break;
    case Search:  m_LineEdit->setPlaceholderText(tr("Incremental Search")); break;
    }

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
    if(IsSeeking()){
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

    if(TreeBank::IsMovingSpace()){
        const QPointer<Receiver> self(this);
        TreeBank::WhenItMayChange([self, command](){ if(self) self->ReceiveCommand(command);});
        return;
    }
    ReceiveCommand(command);
}

void Receiver::SetString(QString str){
    const bool wasSeeking = m_Mode == Command && IsSeeking();
    const Placement before = CurrentPlacement();
    m_LineString = str;

    if(m_Mode == Search){
        emit SeekText(m_LineString, View::HighlightAllOccurrences);
        repaint();
    }
    if(m_Mode == Command){
        QString words;
        if(m_ActiveTab == 0 && SeekWords(m_LineString, &words)){
            if(!m_SuggestStrings.isEmpty()) SetSuggest(QStringList());
            emit SeekText(words, View::HighlightAllOccurrences);
        } else if(wasSeeking){
            emit SeekText(QString(), View::HighlightAllOccurrences);
        }
    }
    AskForSuggestions();
    QStringList list = str.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
    if(m_Mode == Command && m_ActiveTab == 0 && list.length() >= 2 &&
       Application::ExactMatch(QStringLiteral("[uU]n[sS]et"), list[0])){
        Settings &s = Application::GlobalSettings();

        switch (list.length()){
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
            std::sort(result.begin(), result.end());
            SetSuggest(result);
            break;
        }
        default:{
            SetSuggest(QStringList());
            break;
        }
        }
    } else if(m_Mode == Command){
        SuggestFromSources(str);
    }
    if(CurrentPlacement() != before) ResizeNotify(m_TreeBank->size());
}

namespace {
    bool MeansSomething(const QString &word){
        return CommandMap::Resolve(word) != CommandMap::NotACommand ||
            Page::GetSearchEngineMap().contains(word);
    }

    const QRegularExpression &LineSpace(){
        static const QRegularExpression space(QStringLiteral("[\\n\\r\\t ]+"));
        return space;
    }
}

Receiver::SuggestRows Receiver::ComposeRows(const QVector<SuggestRows> &sources, int capacity){
    QVector<int> taken(sources.length(), 0);
    int total = 0;
    for(bool any = true; any && total < capacity;){
        any = false;
        for(int i = 0; i < sources.length() && total < capacity; i++){
            if(taken[i] < sources[i].length()){
                taken[i]++;
                total++;
                any = true;
            }
        }
    }
    SuggestRows result;
    for(int i = 0; i < sources.length(); i++) result += sources[i].mid(0, taken[i]);
    return result;
}

int Receiver::RowCapacity(int sources) const {
    const int height = m_TreeBank->height();
    const int room = height - (height / 12) * 2
        - ScaleByDevice(RECEIVER_PADDING) * 2 - ScaleByDevice(RECEIVER_FIELD_HEIGHT)
        - ScaleByDevice(RECEIVER_LIST_GAP) - ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT)
        - sources * ScaleByDevice(RECEIVER_HEADING_HEIGHT);
    return qMax(room / ScaleByDevice(SUGGEST_HEIGHT), sources);
}

void Receiver::Remember(const QString &line){
    QString action;
    if(CommandMap::Resolve(line.trimmed().split(LineSpace()).first(), &action) != CommandMap::NotACommand)
        RememberAction(action);
}

void Receiver::RememberAction(const QString &action){
    m_History.removeAll(action);
    m_History.prepend(action);
    while(m_History.length() > RECEIVER_HISTORY_LENGTH) m_History.removeLast();
}

namespace {
    void OrderByHistory(QStringList &names, const QStringList &history){
        const auto rank = [&history](const QString &name){
            QString action;
            if(CommandMap::Resolve(name.trimmed(), &action) == CommandMap::NotACommand) action = name.trimmed();
            const int index = history.indexOf(action);
            return index == -1 ? history.length() : index;
        };
        std::stable_sort(names.begin(), names.end(), [&rank](const QString &a, const QString &b){
            return rank(a) < rank(b);
        });
    }

    QStringList NamedCommands(){
        const auto reachable = [](const QString &name){
            return !name.contains(LineSpace()) && CommandMap::Resolve(name) == CommandMap::NotACommand;
        };
        QStringList names;
        const QMap<QString, SearchEngine> engines = Page::GetSearchEngineMap();
        for(auto it = engines.cbegin(); it != engines.cend(); ++it){
            if(reachable(it.key())) names << it.key() + QLatin1Char(' ');
        }
        const QMap<QString, Bookmarklet> bookmarklets = Page::GetBookmarkletMap();
        for(auto it = bookmarklets.cbegin(); it != bookmarklets.cend(); ++it){
            if(reachable(it.key()) && !engines.contains(it.key())) names << it.key();
        }
        return names;
    }
}

bool Receiver::AnswersHere(const QString &action, bool list, bool table){
    const bool inList = InputMapSchema::IsAction(InputMapSchema::GadgetsActions, action);
    const bool elsewhere = InputMapSchema::IsAction(InputMapSchema::PageActions, action) ||
        InputMapSchema::IsAction(InputMapSchema::TreeBankActions, action);
    if(!inList && !elsewhere) return true;
    if(list) return inList;
    return table || elsewhere;
}

Receiver::SuggestRows Receiver::CommandRows(const QString &typed, int limit, bool *exact) const {
    QStringList names;
    const QStringList named = NamedCommands();
    if(typed.isEmpty()){
        names = CommandMap::FirstCommands();
        foreach(const QString &name, CommandMap::ArgumentCommands() + named + CommandMap::AllCommands()){
            if(!names.contains(name)) names << name;
        }
        OrderByHistory(names, m_History);
    } else {
        names = CommandMap::Suggestions(typed, named.length() + CommandMap::AllCommands().length(),
                                        &Receiver::CommandLabel, named);

        const bool overview = m_TreeBank->IsDisplayingTableView();
        const QStringList words = typed.split(LineSpace(), Qt::SkipEmptyParts);
        bool longEnough = !words.isEmpty();
        for(const QString &word : words) if(word.length() < 2) longEnough = false;
        QStringList itself, held;
        for(const InputMapSchema::Binding &binding : m_Bindings){
            if(!overview && binding.vocabulary == InputMapSchema::GadgetsActions) continue;
            if(binding.vocabulary == InputMapSchema::ReceiverActions) continue;
            if(InputMapSchema::IsThisInput(binding, typed)){
                if(!itself.contains(binding.action)) itself << binding.action;
                continue;
            }
            if(!longEnough) continue;
            bool all = true;
            for(const QString &word : words){
                if(!binding.shown.contains(word, Qt::CaseInsensitive) &&
                   !binding.input.contains(word, Qt::CaseInsensitive)) all = false;
            }
            if(all && !held.contains(binding.action)) held << binding.action;
        }
        if(exact) *exact = !itself.isEmpty();
        for(const QString &action : held){
            if(!names.contains(action)) names << action;
        }
        for(const QString &action : itself) names.removeAll(action);
        OrderByHistory(names, m_History);
        names = itself + names;
    }
    Gadgets *gadgets = m_TreeBank->GetGadgets();
    const bool list = gadgets && gadgets->IsActive();
    const bool table = m_TreeBank->IsDisplayingTableView();
    for(int i = names.length() - 1; i >= 0; i--){
        if(!AnswersHere(names[i].trimmed(), list, table)) names.removeAt(i);
    }
    SuggestRows rows;
    foreach(const QString &name, names.mid(0, limit)){
        const QString label = CommandLabel(name.trimmed());
        const QString key = InputsOf(name);
        const SuggestRow::Kind kind =
            name.endsWith(QLatin1Char(' ')) ? SuggestRow::FillField :
            CommandMap::Resolve(name) == CommandMap::NotACommand && !named.contains(name) ?
            SuggestRow::RunAction : SuggestRow::RunCommand;
        rows << SuggestRow{ SuggestRow::Commands, kind, name,
                            label.isEmpty() || key.isEmpty() ? label + key : label + QStringLiteral("   ") + key,
                            name, 0 };
    }
    return rows;
}

namespace {
    QString HostOf(const QUrl &url){
        return url.host().isEmpty() ? url.toString() : url.host();
    }

    QIcon TabIcon(const QIcon &given){
        if(!given.isNull()){
            const QList<QSize> sizes = given.availableSizes();
            if(sizes.isEmpty() || sizes.first().width() > 2) return given;
        }
        static const QIcon blank  = Theme::Icon(QStringLiteral(":/resources/blank.png"));
        static const QIcon blankw = Theme::Icon(QStringLiteral(":/resources/blankw.png"));
        return Theme::IsDark() ? blankw : blank;
    }
}

Receiver::SuggestRows Receiver::TabRows(const QString &typed, int limit) const {
    SuggestRows rows;
    const ViewNode *current = m_TreeBank->GetCurrentViewNode();
    foreach(ViewNode *tab, VanillaPage::FindTabs(typed, m_TreeBank, limit + 1)){
        if(tab == current || rows.length() >= limit) continue;
        const QString title = tab->GetTitle();
        rows << SuggestRow{ SuggestRow::Tabs, SuggestRow::OpenTab,
                            title.isEmpty() ? tab->GetUrl().toString() : title,
                            HostOf(tab->GetUrl()), QString(), tab->GetSerial(),
                            TabIcon(Application::GetIcon(tab->GetUrl())) };
    }
    return rows;
}

Receiver::SuggestRows Receiver::TrashRows(const QString &typed, int limit) const {
    SuggestRows rows;
    foreach(ViewNode *tab, VanillaPage::FindTrashedTabs(typed, m_TreeBank, limit)){
        const QString title = tab->GetTitle();
        rows << SuggestRow{ SuggestRow::Trash, SuggestRow::OpenTab,
                            title.isEmpty() ? tab->GetUrl().toString() : title,
                            HostOf(tab->GetUrl()), QString(), tab->GetSerial(),
                            TabIcon(Application::GetIcon(tab->GetUrl())) };
    }
    return rows;
}

Receiver::SuggestRows Receiver::SettingRows(const QString &typed, int limit) const {
    static const QStringList first = QStringList()
        << QStringLiteral("application/@ColorScheme")
        << QStringLiteral("application/@EnableTranslucentPanels");
    Settings &settings = Application::GlobalSettings();
    const QStringList words = typed.split(QRegularExpression(QStringLiteral("\\s+"),
                                                             QRegularExpression::UseUnicodePropertiesOption),
                                          Qt::SkipEmptyParts);
    const bool sectioned = m_ActiveTab != 0 && typed.isEmpty();
    QHash<QString, QString> categories;
    if(sectioned){
        typedef QPair<QString, QString> Category;
        for(const Category &category : SettingsSchema::Categories()) categories[category.first] = category.second;
    }
    SuggestRows firsts, rows;
    foreach(const SettingsSchema::Item &item, SettingsSchema::Items()){
        if(item.type == SettingsSchema::TextList) continue;
        const QString key = QString::fromLatin1(item.key);
        const QString label = QCoreApplication::translate("SettingsSchema", item.label);
        bool all = true;
        for(const QString &word : words){
            if(!key.contains(word, Qt::CaseInsensitive) && !label.contains(word, Qt::CaseInsensitive)) all = false;
        }
        if(!all) continue;
        const QString value = settings.value(key, QString::fromLatin1(item.fallback)).toString();
        const SuggestRow row{ SuggestRow::Settings, SuggestRow::FillField, label, value,
                              QStringLiteral("set %1 %2").arg(key, value), 0, QIcon(),
                              categories.value(QString::fromLatin1(item.category)), key };
        if(typed.isEmpty() && first.contains(key)) firsts << row;
        else rows << row;
    }
    if(words.length() == 1){
        QSet<QString> listed;
        foreach(const SettingsSchema::Item &item, SettingsSchema::Items()) listed << QString::fromLatin1(item.key);
        foreach(const QString &key, settings.allKeys(words.first())){
            if(listed.contains(key)) continue;
            const QString value = settings.value(key).toString();
            rows << SuggestRow{ SuggestRow::Settings, SuggestRow::FillField, key, value,
                                QStringLiteral("set %1 %2").arg(key, value), 0, QIcon(), QString(), key };
        }
    }
    SuggestRows itself, bindings;
    for(const InputMapSchema::Binding &binding : m_Bindings){
        const QString haystack = binding.shown + QLatin1Char(' ') + binding.input + QLatin1Char(' ') +
            binding.actionLabel + QLatin1Char(' ') + binding.action + QLatin1Char(' ') + binding.table;
        bool all = true;
        for(const QString &word : words){
            if(!haystack.contains(word, Qt::CaseInsensitive)) all = false;
        }
        const bool is = InputMapSchema::IsThisInput(binding, typed);
        if(!all && !is) continue;
        const QString does = binding.actionLabel.isEmpty() ? binding.action : binding.actionLabel;
        const SuggestRow row{ SuggestRow::Settings, SuggestRow::EditBinding, binding.shown,
                              sectioned ? does : does + QStringLiteral("   ") + binding.table,
                              BindingFind(binding), 0, QIcon(),
                              sectioned ? binding.table : QString() };
        if(is) itself << row;
        else bindings << row;
    }
    return (itself + firsts + rows + bindings).mid(0, limit);
}

QString Receiver::BindingFind(const InputMapSchema::Binding &binding){
    return binding.input + QLatin1Char(' ') + binding.table;
}

QString Receiver::SourceName(SuggestRow::Source source){
    switch(source){
    case SuggestRow::Commands: return tr("Commands");
    case SuggestRow::Tabs:     return tr("Tabs");
    case SuggestRow::Trash:    return tr("Trash");
    case SuggestRow::Settings: return tr("Settings");
    case SuggestRow::Web:      return tr("Web");
    default: break;
    }
    return QString();
}

bool Receiver::IsAssignment(const QString &rest){
    const QStringList words = rest.trimmed().split(LineSpace());
    return words.length() >= 2 && words.first().contains(QLatin1Char('/'));
}

bool Receiver::SourceWord(const QString &line, SuggestRow::Source *source, QString *rest){
    static const QRegularExpression form(QStringLiteral("^(\\S+)(?:[\\n\\r\\t ]+(.*))?$"),
                                         QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = form.match(line);
    if(!m.hasMatch()) return false;
    const QString word = m.captured(1);

    if(Page::GetSearchEngineMap().contains(word)) return false;

    static const struct { const char *name; SuggestRow::Source source; } names[] = {
        { "commands", SuggestRow::Commands },
        { "tabs",     SuggestRow::Tabs },
        { "trash",    SuggestRow::Trash },
        { "settings", SuggestRow::Settings },
        { "web",      SuggestRow::Web },
    };
    int found = -1;
    if(word.length() >= 3){
        for(const auto &name : names){
            if(QLatin1String(name.name).startsWith(word, Qt::CaseInsensitive)) found = name.source;
        }
    }
    for(int i = 0; i < SuggestRow::SourceCount; i++){
        if(word.compare(SourceName(static_cast<SuggestRow::Source>(i)), Qt::CaseInsensitive) == 0) found = i;
    }
    if(found == -1) return false;
    if(source) *source = static_cast<SuggestRow::Source>(found);
    if(rest) *rest = m.captured(2);
    return true;
}

bool Receiver::ShowsSourceBar() const {
    return m_Mode == Command && !IsSeeking() && (!m_Rows.isEmpty() || m_SuggestStrings.isEmpty());
}

QRect Receiver::SourceBarRect(int index) const {
    const int pad = ScaleByDevice(RECEIVER_PADDING);
    const int count = SuggestRow::SourceCount + 1;
    const int width = (this->width() - pad * 2) / count;
    return QRect(pad + index * width,
                 pad + ScaleByDevice(RECEIVER_FIELD_HEIGHT) + ScaleByDevice(RECEIVER_LIST_GAP),
                 width, ScaleByDevice(RECEIVER_SOURCE_BAR_HEIGHT));
}

int Receiver::SourceBarAt(const QPoint &pos) const {
    if(!ShowsSourceBar()) return -1;
    for(int i = 0; i <= SuggestRow::SourceCount; i++){
        if(SourceBarRect(i).contains(pos)) return i;
    }
    return -1;
}

void Receiver::RefillIfShown(){
    if(!isVisible() || m_Mode != Command || m_ActiveTab != 0 || m_Rows.isEmpty()) return;
    RefillKeepingChoice();
}

void Receiver::RefillKeepingChoice(){
    const int chosen = m_CurrentSuggestIndex;
    const QString title = chosen >= 0 && chosen < m_SuggestStrings.length() ? m_SuggestStrings[chosen] : QString();
    SuggestFromSources(m_LineString);
    if(!title.isEmpty() && m_SuggestStrings.contains(title)) m_CurrentSuggestIndex = m_SuggestStrings.indexOf(title);
    KeepSelectionShown();
    repaint();
}

void Receiver::AskForSuggestions(){
    const bool web = m_Mode == Command && m_ActiveTab == SuggestRow::Web + 1;
    const bool empty = m_LineString.trimmed().isEmpty();
    const bool address = web && !Page::ExtractUrlsFromText(m_LineString).isEmpty();
    if(!web || empty || address) m_WebSuggests.clear();
    if((m_Mode != Query && !web) || address) return;
    if(empty){
        if(m_Mode == Query && !m_SuggestStrings.isEmpty()) SetSuggest(QStringList());
        return;
    }
    if(Application::EnableGoogleSuggest())
        emit SuggestRequest(SearchSuggest::Url(Application::SuggestEngine(), m_LineString));
}

void Receiver::SelectTab(int index){
    m_ActiveTab = index;
    AskForSuggestions();
    SuggestFromSources(m_LineString);
    m_LineEdit->setFocus();
}

Receiver::SuggestRows Receiver::WebRows(const QString &typed) const {
    if(typed.isEmpty()){
        return SuggestRows{ SuggestRow{ SuggestRow::Web, SuggestRow::FillField, tr("Search the web"),
                                        QString(), QStringLiteral("query "), 0 } };
    }
    SuggestRows rows;
    if(!Page::ExtractUrlsFromText(typed).isEmpty()){
        rows << SuggestRow{ SuggestRow::Web, SuggestRow::OpenAddress, typed, tr("Open addresses"), typed, 0 };
    }
    rows << SuggestRow{ SuggestRow::Web, SuggestRow::SearchWeb, typed, tr("Search the web"), typed, 0 };
    for(const QString &word : m_WebSuggests){
        if(word != typed) rows << SuggestRow{ SuggestRow::Web, SuggestRow::SearchWeb, word, QString(), word, 0 };
    }
    return rows;
}

void Receiver::SuggestFromSources(const QString &typed){
    const QStringList words = typed.split(LineSpace());
    const bool oneWord = words.length() == 1;
    const bool means = MeansSomething(words.first());

    SuggestRows rows;
    int selected = -1;
    if(m_ActiveTab != 0){
        const int limit = RECEIVER_SCROLL_ROWS;
        const QString wanted = typed.trimmed();
        switch(static_cast<SuggestRow::Source>(m_ActiveTab - 1)){
        case SuggestRow::Commands: rows = CommandRows(wanted, limit); break;
        case SuggestRow::Tabs:     rows = TabRows(wanted, limit);     break;
        case SuggestRow::Trash:    rows = TrashRows(wanted, limit);   break;
        case SuggestRow::Settings: rows = SettingRows(wanted, limit); break;
        case SuggestRow::Web:      rows = WebRows(wanted);            break;
        default: break;
        }
        rows = rows.mid(0, limit);
        if(!rows.isEmpty()) selected = 0;
    } else if(SeekWords(typed)){
    } else {
        SuggestRow::Source named = SuggestRow::SourceCount;
        QString rest;
        const bool lifted = SourceWord(typed, &named, &rest);
        const auto wanted = [&](SuggestRow::Source source){
            return lifted && source == named ? rest.trimmed() : typed;
        };
        const bool assignment = lifted && named == SuggestRow::Settings && IsAssignment(rest);

        const int limit = RowCapacity(1);
        const bool argument = !oneWord && means && !lifted;
        const bool commandsToo = !argument;

        SuggestRows commands, tabs, trash, settings, web;
        bool exact = false;
        if(!assignment){
            commands = CommandRows(wanted(SuggestRow::Commands), limit, &exact);
            if(!commandsToo && !exact){
                const bool given = !typed.mid(words.first().length()).trimmed().isEmpty();
                SuggestRows taker;
                for(SuggestRow row : CommandRows(words.first(), INT_MAX)){
                    if(row.kind != SuggestRow::FillField ||
                       row.text.trimmed().compare(words.first(), Qt::CaseInsensitive) != 0) continue;
                    if(given){
                        row.kind = SuggestRow::RunCommand;
                        row.text = typed;
                    }
                    taker << row;
                }
                commands = taker;
            }
            tabs = TabRows(wanted(SuggestRow::Tabs), limit);
            trash = TrashRows(wanted(SuggestRow::Trash), limit);
            settings = SettingRows(wanted(SuggestRow::Settings), limit);
            if(!argument) web = WebRows(wanted(SuggestRow::Web));
        }

        QVector<SuggestRow::Source> order{ SuggestRow::Commands, SuggestRow::Tabs,
                                           SuggestRow::Trash, SuggestRow::Settings, SuggestRow::Web };
        const bool address = !typed.isEmpty() && !Page::ExtractUrlsFromText(typed).isEmpty();
        if(address){
            order.removeAll(SuggestRow::Web);
            order.prepend(SuggestRow::Web);
        }
        if(exact){
            order.removeAll(SuggestRow::Commands);
            order.prepend(SuggestRow::Commands);
        }
        if(lifted){
            order.removeAll(named);
            order.prepend(named);
        }
        QVector<SuggestRows> sources;
        for(const SuggestRow::Source source : order){
            switch(source){
            case SuggestRow::Commands: sources << commands; break;
            case SuggestRow::Tabs:     sources << tabs;     break;
            case SuggestRow::Trash:    sources << trash;    break;
            case SuggestRow::Settings: sources << settings; break;
            default:                   sources << web;      break;
            }
        }
        int present = 0;
        for(const SuggestRows &source : sources) if(!source.isEmpty()) present++;
        rows = ComposeRows(sources, RowCapacity(present));

        const bool asked = lifted && !rest.trimmed().isEmpty() && !rows.isEmpty() && rows.first().source == named;
        if(asked ||
           (!rows.isEmpty() && !typed.isEmpty() && !means && !Page::GetBookmarkletMap().contains(typed)))
            selected = 0;
    }

    m_Rows = rows;
    m_SuggestStrings.clear();
    for(const SuggestRow &row : rows) m_SuggestStrings << row.title;
    m_CurrentSuggestIndex = selected;
    m_SuggestScroll = 0;

    ResizeNotify(m_TreeBank->size());
    repaint();
}

void Receiver::TakeRow(SuggestRow row){
    switch(row.kind){
    case SuggestRow::RunCommand:
        hide();
        Remember(row.text);
        ReceiveCommand(row.text);
        break;
    case SuggestRow::FillField:
        m_LineEdit->setText(row.text);
        m_LineEdit->setFocus();
        break;
    case SuggestRow::RunAction:
        hide();
        RememberAction(row.text);
        DispatchAction(row.text);
        break;
    case SuggestRow::EditBinding:
        OpenSettingsWhenItMayChange(QStringLiteral("input?find=") +
                                    QString::fromLatin1(QUrl::toPercentEncoding(row.text.toLower())));
        break;
    case SuggestRow::OpenTab:{
        hide();
        const quint64 serial = row.serial;
        const QPointer<TreeBank> bank = m_TreeBank;
        TreeBank::WhenItMayChange([bank, serial](){
            ViewNode *target = VanillaPage::TabOfSerial(serial);
            if(!bank || !TreeBank::IsLive(bank) || !target) return;
            if(TreeBank::IsTrash(target)) TreeBank::RestoreBeside(target, bank->GetCurrentViewNode());
            bank->SetCurrent(target);
        });
        break;
    }
    case SuggestRow::SearchWeb:
        hide();
        emit OpenQueryUrl(row.text);
        break;
    case SuggestRow::OpenAddress:
        hide();
        emit OpenUrl(Page::DirtyStringToUrls(row.text));
        break;
    }
}

QVector<Receiver::RowAction> Receiver::RowActions(const SuggestRow &row){
    QVector<RowAction> actions{ RowAction::Take };
    switch(row.source){
    case SuggestRow::Tabs:
        actions << RowAction::CloseTab << RowAction::CopyAddress;
        break;
    case SuggestRow::Trash:
        actions << RowAction::DiscardTab << RowAction::CopyAddress;
        break;
    case SuggestRow::Commands:{
        const QString name = row.text.trimmed();
        if(InputMapSchema::IsAction(InputMapSchema::PageActions, name) ||
           InputMapSchema::IsAction(InputMapSchema::TreeBankActions, name) ||
           InputMapSchema::IsAction(InputMapSchema::GadgetsActions, name))
            actions << RowAction::ChangeAssignment;
        break;
    }
    case SuggestRow::Settings:
        if(row.kind == SuggestRow::FillField){
            if(SettingsSchema::Find(row.key))
                actions << RowAction::OpenInSettings;
            actions << RowAction::ResetSetting;
        }
        break;
    case SuggestRow::Web:
        if(row.kind != SuggestRow::FillField) actions << RowAction::CopyWords;
        break;
    default:
        break;
    }
    return actions;
}

QString Receiver::RowActionLabel(const SuggestRow &row, RowAction action){
    switch(action){
    case RowAction::Take:
        switch(row.source){
        case SuggestRow::Tabs:     return tr("Switch to the tab");
        case SuggestRow::Trash:    return tr("Restore the tab");
        case SuggestRow::Commands: return tr("Run");
        case SuggestRow::Settings:
            return row.kind == SuggestRow::EditBinding ? tr("Change the assignment") : tr("Edit the value");
        case SuggestRow::Web:
            return row.kind == SuggestRow::OpenAddress ? tr("Open addresses") : tr("Search the web");
        default: return QString();
        }
    case RowAction::CloseTab:         return tr("Close the tab");
    case RowAction::DiscardTab:       return tr("Delete for good");
    case RowAction::CopyAddress:      return tr("Copy the address");
    case RowAction::CopyWords:        return tr("Copy the words");
    case RowAction::ChangeAssignment: return tr("Change the assignment");
    case RowAction::OpenInSettings:   return tr("Open in the settings");
    case RowAction::ResetSetting:     return tr("Reset a setting");
    }
    return QString();
}

void Receiver::RunRowAction(const SuggestRow &row, RowAction action){
    switch(action){
    case RowAction::Take:
        TakeRow(row);
        return;
    case RowAction::CloseTab:{
        const QPointer<Receiver> self(this);
        const QPointer<TreeBank> bank = m_TreeBank;
        const quint64 serial = row.serial;
        TreeBank::WhenItMayChange([self, bank, serial](){
            ViewNode *target = VanillaPage::TabOfSerial(serial);
            if(!bank || !TreeBank::IsLive(bank) || !target) return;
            if(TreeBank::WinIndex(target) && bank->GetCurrentViewNode() != target) return;
            bank->DeleteNode(target);
            if(!self || !self->isVisible() || self->m_Mode != Command) return;
            self->RefillKeepingChoice();
            if(self->m_CurrentSuggestIndex < 0 && !self->m_Rows.isEmpty()){
                self->m_CurrentSuggestIndex = 0;
                self->KeepSelectionShown();
                self->repaint();
            }
        });
        return;
    }
    case RowAction::DiscardTab:
        hide();
        TreeBank::DiscardFromTrashAsked(QList<quint64>() << row.serial, 1, SourceName(SuggestRow::Trash));
        return;
    case RowAction::CopyAddress:
        hide();
        if(ViewNode *target = VanillaPage::TabOfSerial(row.serial))
            QApplication::clipboard()->setText(target->GetUrl().toString());
        return;
    case RowAction::CopyWords:
        hide();
        QApplication::clipboard()->setText(row.text);
        return;
    case RowAction::ChangeAssignment:
        OpenSettingsWhenItMayChange(QStringLiteral("input?find=") +
                                    QString::fromLatin1(QUrl::toPercentEncoding(row.text.trimmed().toLower())));
        return;
    case RowAction::OpenInSettings:
        if(const SettingsSchema::Item *item = SettingsSchema::Find(row.key))
            OpenSettingsWhenItMayChange(QString::fromLatin1(item->category) + QStringLiteral("?find=") +
                                        QString::fromLatin1(QUrl::toPercentEncoding(row.key)));
        return;
    case RowAction::ResetSetting:
        hide();
        if(row.key.isEmpty()) return;
        Application::GlobalSettings().remove(row.key);
        m_TreeBank->Reconfigure();
        return;
    }
}

void Receiver::OpenSettingsWhenItMayChange(const QString &fragment){
    hide();
    const QPointer<TreeBank> bank = m_TreeBank;
    TreeBank::WhenItMayChange([bank, fragment](){
        if(bank && TreeBank::IsLive(bank)) bank->OpenSettings(fragment);
    });
}

void Receiver::OpenRowActions(int index, const QPoint &globalPos){
    if(index < 0 || index >= m_Rows.length()) return;
    m_CurrentSuggestIndex = index;
    KeepSelectionShown();
    repaint();
    const SuggestRow row = m_Rows[index];
    QMenu menu;
    const QVector<RowAction> actions = RowActions(row);
    for(int i = 0; i < actions.length(); i++){
        QAction *action = menu.addAction(RowActionLabel(row, actions[i]));
        action->setData(i);
    }
    menu.setDefaultAction(menu.actions().first());
    menu.setActiveAction(menu.actions().first());
    const QPointer<Receiver> self(this);
    QAction *chosen = menu.exec(globalPos);
    if(!self) return;
    QTimer::singleShot(0, this, &Receiver::CloseIfFocusLeft);
    if(chosen) RunRowAction(row, actions[chosen->data().toInt()]);
}

bool Receiver::HandleTabKey(){
    if(m_Mode != Command || m_Rows.isEmpty()) return false;
    const int index = m_CurrentSuggestIndex >= 0 ? m_CurrentSuggestIndex : m_SuggestScroll;
    if(index < m_Rows.length() && VisibleSuggestCount())
        OpenRowActions(index, mapToGlobal(SuggestRect(index).bottomLeft()));
    return true;
}

void Receiver::contextMenuEvent(QContextMenuEvent *ev){
    ev->accept();
    if(View::TakeRightButtonConsumed()) return;
    if(m_Mode != Command || m_Rows.isEmpty()) return;
    const int index = SuggestAt(ev->pos());
    if(index >= 0) OpenRowActions(index, ev->globalPos());
}

bool Receiver::HandleMappedKey(QKeyEvent *ev){
    const QKeySequence seq = InputMap::DisplayKey(Application::MakeKeySequence(ev));
    if(seq.isEmpty()) return false;
    const int count = SuggestRow::SourceCount + 1;
    for(const InputMapSchema::Binding &binding : m_Bindings){
        if(binding.vocabulary != InputMapSchema::ReceiverActions) continue;
        const QKeySequence bound(binding.input, QKeySequence::PortableText);
        if(InputMapSchema::TypesIntoTheField(bound) || InputMap::DisplayKey(bound) != seq) continue;
        const QString &action = binding.action;
        if(action == QStringLiteral("NextCommandLineTab") || action == QStringLiteral("PrevCommandLineTab")){
            if(!ShowsSourceBar()) return false;
            const int step = action == QStringLiteral("NextCommandLineTab") ? 1 : count - 1;
            SelectTab((m_ActiveTab + step) % count);
            return true;
        }
        if(action == QStringLiteral("CommandLineRowActions")) return HandleTabKey();
        if(action == QStringLiteral("NextCommandLineRow")){ SelectNextSuggest(); return true;}
        if(action == QStringLiteral("PrevCommandLineRow")){ SelectPrevSuggest(); return true;}
        if(action == QStringLiteral("NextCommandLinePage") || action == QStringLiteral("PrevCommandLinePage")){
            if(!ShowsSourceBar() || m_ActiveTab == 0) return false;
            const int rows = SelectableSuggestCount();
            const int step = qMax(VisibleSuggestCount() - 1, 1) *
                (action == QStringLiteral("NextCommandLinePage") ? 1 : -1);
            if(rows){
                m_CurrentSuggestIndex = qBound(0, m_CurrentSuggestIndex + step, rows - 1);
                KeepSelectionShown();
                repaint();
            }
            return true;
        }
        if(action == QStringLiteral("CloseCommandLine")){ OnAborted(); return true;}
    }
    return false;
}

int Receiver::HintWidth(int row, int title, int hint, int gap){
    return qMin(hint, qMax(row / 2, row - title - gap));
}

QString Receiver::CommandLabel(const QString &name){
    if(name == QStringLiteral("open"))     return tr("Open addresses");
    if(name == QStringLiteral("load"))     return tr("Load an address here");
    if(name == QStringLiteral("query"))    return tr("Search the web");
    if(name == QStringLiteral("download")) return tr("Download to a file");
    if(name == QStringLiteral("search"))   return tr("Search in the page");
    if(name == QStringLiteral("set"))      return tr("Change a setting");
    if(name == QStringLiteral("unset"))    return tr("Reset a setting");
    if(name == QStringLiteral("key"))      return tr("Send a key");
    if(name == QStringLiteral("blank"))    return tr("Open a blank page");
    if(name == QStringLiteral("Reconfigure")) return tr("Read the settings again");

    const QByteArray source = InputMapSchema::LabelSource(name).toUtf8();
    for(const char *context : { "TreeBank", "Page", "Gadgets" }){
        const QString label = QCoreApplication::translate(context, source.constData());
        if(label != QString::fromUtf8(source)) return label;
    }

    const QMap<QString, SearchEngine> engines = Page::GetSearchEngineMap();
    const auto engine = engines.constFind(name);
    if(engine != engines.cend() && !engine.value().isEmpty()){
        const QString host = QUrl::fromEncoded(engine.value().first().toLatin1()).host();
        return tr("Search %1").arg(host.startsWith(QStringLiteral("www.")) ? host.mid(4) : host);
    }
    if(Page::GetBookmarkletMap().contains(name)) return tr("Run the bookmarklet");
    return QString();
}

void Receiver::ReadBindings(){
    m_Bindings = InputMapSchema::Bindings(Application::GlobalSettings(), View::EnableSingleKeyShortcut());
    const bool overview = m_TreeBank->IsDisplayingTableView();
    std::stable_partition(m_Bindings.begin(), m_Bindings.end(), [overview](const InputMapSchema::Binding &binding){
        return (binding.vocabulary == InputMapSchema::GadgetsActions) == overview;
    });
}

QString Receiver::InputsOf(const QString &name) const {
    const QString action = name.trimmed();
    QStringList inputs;
    for(const InputMapSchema::Binding &binding : m_Bindings){
        if(binding.action == action && !inputs.contains(binding.shown)) inputs << binding.shown;
        if(inputs.length() == 3) break;
    }
    return inputs.join(QStringLiteral(", "));
}

void Receiver::SetSuggest(QStringList list){
    m_Rows.clear();
    m_SuggestScroll = 0;
    m_CurrentSuggestIndex = -1;
    m_SuggestStrings.clear();
    m_SuggestStrings = list;
    ResizeNotify(m_TreeBank->size());
    repaint();
}

namespace {

    QString WithCurrentValue(const QString &key){
        QStringList list = key.split(QRegularExpression(QStringLiteral("[\n\r\t ]+")));
        if(list.size() < 2 ||
           !Application::ExactMatch(QStringLiteral("(?:[uU]n)?[sS]et"), list[0]))
            return QString();
        const QString value = Application::GlobalSettings().value(list[1]).toString();
        if(!value.isEmpty()) list << value;
        return list.join(QStringLiteral(" "));
    }
}

void Receiver::SuitableAction(){
    switch (m_Mode){
    case Query:{
        const QString words = m_CurrentSuggestIndex != -1
            ? m_SuggestStrings[m_CurrentSuggestIndex] : m_LineString;
        hide();
        emit OpenQueryUrl(words);
        break;
    }
    case UrlEdit:
        hide();
        emit OpenUrl(Page::DirtyStringToUrls(m_LineString == QStringLiteral("blank")
                                             ? QStringLiteral("about:blank")
                                             : m_LineString));
        break;
    case Search:
        SeekFromField(m_LineString);
        break;
    case Command:{
        if(!m_Rows.isEmpty() && m_CurrentSuggestIndex != -1){
            TakeRow(m_Rows[m_CurrentSuggestIndex]);
        } else if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            const QString text = WithCurrentValue(m_SuggestStrings[m_CurrentSuggestIndex]);
            if(!text.isEmpty()) m_LineEdit->setText(text);
        } else if(QString words; m_ActiveTab == 0 && SeekWords(m_LineString, &words)){
            Remember(m_LineString);
            SeekFromField(words);
        } else if(SuggestRow::Source only; m_ActiveTab == 0 && SourceWord(m_LineString, &only, &words) &&
                  only == SuggestRow::Settings && IsAssignment(words)){
            const QString line = QStringLiteral("set ") + words.trimmed();
            hide();
            Remember(line);
            ReceiveCommand(line);
        } else if(m_ActiveTab != 0 && !MeansSomething(m_LineString.split(LineSpace()).first())){
        } else {
            hide();
            Remember(m_LineString);
            ReceiveCommand(m_LineString);
        }
        break;
    }
    }
}

void Receiver::SeekFromField(const QString &words){
    if(m_TreeBank->IsDisplayingTableView()){
        emit SeekText(words, View::WrapsAroundDocument);
        hide();
    } else if(Application::keyboardModifiers() & Qt::ShiftModifier){
        emit SeekText(words, View::WrapsAroundDocument | View::FindBackward);
    } else {
        emit SeekText(words, View::WrapsAroundDocument);
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
    const int count = SelectableSuggestCount();
    if(!count) return;
    m_CurrentSuggestIndex = m_CurrentSuggestIndex + 1 < count ? m_CurrentSuggestIndex + 1 : 0;
    KeepSelectionShown();
    repaint();
}

void Receiver::SelectPrevSuggest(){
    const int count = SelectableSuggestCount();
    if(!count) return;
    m_CurrentSuggestIndex = m_CurrentSuggestIndex > 0 ? m_CurrentSuggestIndex - 1 : count - 1;
    KeepSelectionShown();
    repaint();
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

    {
        const int radius = ScaleByDevice(DIALOG_CORNER_RADIUS);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Theme::Pen(Theme::VanillaPageBorder));
        painter.setBrush(Theme::Brush(IsPurged() ? BlurBehind::Surface() : Theme::ReceiverBackground));
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.drawRoundedRect(card, radius, radius);
    }

    const int count = VisibleSuggestCount();
    const int radius = ScaleByDevice(PAGE_FIELD_CORNER_RADIUS);
    const int padX = ScaleByDevice(RECEIVER_FIELD_PADDING_X);

    if(ShowsSourceBar()){
        const int active = m_ActiveTab;
        for(int i = 0; i <= SuggestRow::SourceCount; i++){
            const QRect chip = SourceBarRect(i);
            if(i == active){
                painter.setPen(Qt::NoPen);
                painter.setBrush(Theme::Brush(Theme::ReceiverBackgroundContrast));
                painter.drawRoundedRect(chip, radius, radius);
            }
            painter.setPen(Theme::Pen(i == active ? Theme::ReceiverTextContrast : Theme::VanillaPageTextDim));
            painter.setBrush(Qt::NoBrush);
            painter.drawText(chip, Qt::AlignCenter,
                             i == 0 ? tr("All") : SourceName(static_cast<SuggestRow::Source>(i - 1)));
        }
    }
    if(count && count < m_SuggestStrings.length()){
        const QRect first = SuggestRect(m_SuggestScroll);
        const int track = SuggestRect(m_SuggestScroll + count - 1).bottom() - first.top();
        const int total = m_SuggestStrings.length();
        const int mark = qMax(track * count / total, ScaleByDevice(RECEIVER_FIELD_HEIGHT) / 2);
        const int width = ScaleByDevice(RECEIVER_SCROLL_MARK_WIDTH);
        const int y = first.top() + (track - mark) * m_SuggestScroll / qMax(total - count, 1);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Brush(Theme::VanillaPageTextDim));
        painter.drawRoundedRect(QRect(this->width() - ScaleByDevice(RECEIVER_PADDING) / 2 - width / 2, y, width, mark),
                                width / 2, width / 2);
    }

    for(int i = m_SuggestScroll; i < m_SuggestScroll + count; i++){
        const QRect row = SuggestRect(i);
        const bool selected = i == m_CurrentSuggestIndex;
        if(selected){
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::Brush(Theme::ReceiverBackgroundContrast));
            painter.drawRoundedRect(row, radius, radius);
        }
        QRect text = row.adjusted(padX, 0, -padX, 0);
        painter.setBrush(Qt::NoBrush);
        if(HasHeading(i)){
            const QRect heading(text.left() - ScaleByDevice(RECEIVER_HEADING_OUTDENT),
                                row.top() - ScaleByDevice(RECEIVER_HEADING_HEIGHT),
                                text.width(), ScaleByDevice(RECEIVER_HEADING_HEIGHT));
            painter.setPen(Theme::Pen(Theme::VanillaPageTextDim));
            painter.drawText(heading, Qt::AlignLeft | Qt::AlignBottom,
                             m_Rows[i].section.isEmpty() ? SourceName(m_Rows[i].source) : m_Rows[i].section);
        }
        if(!m_Rows.isEmpty() && !m_Rows[i].hint.isEmpty()){
            const QString hint = m_Rows[i].hint;
            const int width = HintWidth(text.width(), painter.fontMetrics().horizontalAdvance(m_SuggestStrings[i]),
                                        painter.fontMetrics().horizontalAdvance(hint), padX);
            painter.setPen(Theme::Pen(Theme::VanillaPageTextDim));
            painter.drawText(QRect(text.right() - width + 1, text.top(), width, text.height()),
                             Qt::AlignRight | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(hint, Qt::ElideLeft, width));
            text.setRight(text.right() - width - padX);
        }
        if(!m_Rows.isEmpty() && m_Rows[i].kind == SuggestRow::OpenTab){
            const int size = ScaleByDevice(16);
            m_Rows[i].icon.paint(&painter, QRect(text.left(), text.top() + (text.height() - size) / 2, size, size));
            text.setLeft(text.left() + size + ScaleByDevice(RECEIVER_PADDING));
        }
        painter.setPen(Theme::Pen(selected ? Theme::ReceiverTextContrast : Theme::ReceiverText));
        painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(m_SuggestStrings[i], Qt::ElideRight, text.width()));
    }

    ev->setAccepted(true);
}

void Receiver::hideEvent(QHideEvent *ev){
    if(m_FocusWentElsewhere){
        m_FocusWentElsewhere = false;
        QWidget::hideEvent(ev);
        return;
    }

    MainWindow *win = m_TreeBank->GetMainWindow();

    const bool purged = IsPurged() && isActiveWindow();
    QPointer<Receiver> alive(this);
    QTimer::singleShot(0, win, [win, purged, alive](){
        if(alive && alive->isVisible()) return;
        if(!win->isVisible() || win->isMinimized()) return;
        QWidget *active = QApplication::activeWindow();
        if(active && active != win) return;
        if(purged || win->isActiveWindow()) win->SetFocus();
    });
    QWidget::hideEvent(ev);
}

void Receiver::showEvent(QShowEvent *ev){
    ResizeNotify(m_TreeBank->size());

    if(m_Mode == Search && !m_LineString.isEmpty()){
        emit SeekText(m_LineString, View::HighlightAllOccurrences);
    } else if(QString words; IsSeeking() && m_Mode == Command && SeekWords(m_LineString, &words)){
        emit SeekText(words, View::HighlightAllOccurrences);
    }
    m_LineEdit->show();
    m_LineEdit->raise();
    QWidget::showEvent(ev);
    if(IsPurged()){
        BlurBehind::Apply(this);
        BlurBehind::RoundCorners(this);
    }
}

void Receiver::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
    ev->setAccepted(true);
}

void Receiver::mousePressEvent(QMouseEvent *ev){
    if(ev->button() != Qt::LeftButton){
        if(ev->button() == Qt::RightButton) View::TakeRightButtonConsumed();
        QWidget::mousePressEvent(ev);
        return;
    }
    if(!m_SuggestStrings.isEmpty()) m_CurrentSuggestIndex = SuggestAt(ev->pos());

    const int source = SourceBarAt(ev->pos());
    if(source != -1){
        SelectTab(source);
        return;
    }
    if(m_Mode == Query && m_CurrentSuggestIndex != -1){
        SuitableAction();
        return;
    }
    if(m_Mode == Command && !m_Rows.isEmpty()){
        if(m_CurrentSuggestIndex != -1){
            TakeRow(m_Rows[m_CurrentSuggestIndex]);
            return;
        }
    } else if(m_Mode == Command){
        if(!m_SuggestStrings.isEmpty() && m_CurrentSuggestIndex != -1){
            const QString key = m_SuggestStrings[m_CurrentSuggestIndex];
            const QString text = WithCurrentValue(key);
            m_LineEdit->setText(text.isEmpty() ? key : text);
            m_LineEdit->setFocus();
        }
    }
    QWidget::mousePressEvent(ev);
}

void Receiver::mouseReleaseEvent(QMouseEvent *ev){
    QWidget::mouseReleaseEvent(ev);
}

void Receiver::mouseMoveEvent(QMouseEvent *ev){
    if(!m_SuggestStrings.isEmpty()){
        const int index = SuggestAt(ev->pos());
        if(index != m_CurrentSuggestIndex){
            m_CurrentSuggestIndex = index;
            repaint();
        }
    }
    QWidget::mouseMoveEvent(ev);
}

void Receiver::wheelEvent(QWheelEvent *ev){
    const int notches = ev->angleDelta().y() / 120;
    if(notches) ScrollSuggestsBy(-notches * RECEIVER_WHEEL_ROWS);
    ev->accept();
}

void Receiver::DisplaySuggest(const QByteArray &ba){

    if(!m_LineEdit->hasFocus() || m_LineString.trimmed().isEmpty()) return;

    if(m_Mode == Query){
        SetSuggest(SearchSuggest::Parse(ba));
    } else if(m_Mode == Command && m_ActiveTab == SuggestRow::Web + 1 &&
              Page::ExtractUrlsFromText(m_LineString).isEmpty()){
        m_WebSuggests = SearchSuggest::Parse(ba);
        RefillKeepingChoice();
    }
}
