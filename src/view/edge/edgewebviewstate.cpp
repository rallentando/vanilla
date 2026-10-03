#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebviewstate.hpp"

#include <QDateTime>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

QUrl EdgeWithRootPath(const QUrl &url){
    if(!url.path().isEmpty()) return url;
    QUrl rooted(url);
    rooted.setPath(QStringLiteral("/"));
    return rooted;
}

bool EdgeIsOwnViewSource(const QUrl &shown, const QUrl &reported){
    if(shown.scheme() != QStringLiteral("view-source")) return false;

    const QUrl inside = QUrl(QString::fromUtf8(shown.toEncoded().mid(12)));
    if(!inside.isValid() || inside.isEmpty()) return false;

    return EdgeWithRootPath(inside) == EdgeWithRootPath(reported);
}

bool EdgeIsOwnStringDocument(const QUrl &shown, const QUrl &stringDocument,
                             const QUrl &reported){
    if(stringDocument.isEmpty() || shown != stringDocument) return false;
    return reported == QUrl(QStringLiteral("about:blank"));
}

QVariant EdgeScriptResultToVariant(const QByteArray &json){
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if(!doc.isNull()) return doc.toVariant();

    const QJsonDocument wrapped =
        QJsonDocument::fromJson(QByteArrayLiteral("[") + json +
                                QByteArrayLiteral("]"));
    if(wrapped.isNull() || !wrapped.isArray() || wrapped.array().size() != 1)
        return QVariant();

    const QJsonValue value = wrapped.array().first();
    if(value.isNull()) return QVariant();
    return value.toVariant();
}

QPointF EdgeScrollRatio(const QPointF &scroll, const QSizeF &contents,
                        const QSizeF &viewport){
    const auto axis = [](qreal value, qreal room) -> qreal {
        if(room <= 0.0) return 0.5;
        return qBound<qreal>(0.0, value / room, 1.0);
    };
    return QPointF(axis(scroll.x(), contents.width()  - viewport.width()),
                   axis(scroll.y(), contents.height() - viewport.height()));
}

QNetworkCookie EdgeCookieFromParts(const QString &name, const QString &value,
                                   const QString &domain, const QString &path,
                                   double expires, bool httpOnly, bool secure){
    QNetworkCookie made;
    made.setName(name.toLatin1());
    made.setValue(value.toLatin1());
    made.setDomain(domain);

    if(!path.isEmpty()) made.setPath(path);

    if(expires > 0)
        made.setExpirationDate(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(expires)));

    made.setHttpOnly(httpOnly);
    made.setSecure(secure);

    return made;
}

EdgeControllerState::EdgeControllerState()
    : m_State(State::Constructed)
    , m_PendingLoad(EdgePendingLoad())
    , m_HasPendingLoad(false)
{
}

EdgeControllerState::Effect EdgeControllerState::Start(){
    if(m_State != State::Constructed) return Effect::None;
    m_State = State::AwaitingEnvironment;
    return Effect::RequestEnvironment;
}

EdgeControllerState::Effect EdgeControllerState::EnvironmentReady(){
    if(m_State != State::AwaitingEnvironment) return Effect::None;
    m_State = State::AwaitingController;
    return Effect::CreateController;
}

EdgeControllerState::Effect EdgeControllerState::EnvironmentFailed(){
    if(m_State != State::AwaitingEnvironment) return Effect::None;
    m_State = State::Failed;
    return Effect::ReportFailure;
}

EdgeControllerState::Effect EdgeControllerState::ControllerCreated(){
    if(m_State != State::AwaitingController) return Effect::CloseOrphan;
    m_State = State::AwaitingScript;
    return Effect::AdoptController;
}

EdgeControllerState::Effect EdgeControllerState::ScriptRegistered(){
    if(m_State != State::AwaitingScript) return Effect::None;
    m_State = State::Ready;
    return Effect::ApplyPending;
}

EdgeControllerState::Effect EdgeControllerState::ScriptFailed(){
    if(m_State != State::AwaitingScript) return Effect::None;
    m_State = State::Ready;
    return Effect::ApplyPending;
}

EdgeControllerState::Effect EdgeControllerState::ControllerFailed(){
    if(m_State != State::AwaitingController) return Effect::None;
    m_State = State::Failed;
    return Effect::ReportFailure;
}

EdgeControllerState::Effect EdgeControllerState::Retire(){
    if(m_State == State::Retired) return Effect::None;
    const State before = m_State;
    m_State = State::Retired;
    if(before == State::Ready || before == State::AwaitingScript)
        return Effect::CloseOwned;
    return Effect::None;
}

void EdgeControllerState::SetPendingLoad(const EdgePendingLoad &load){
    m_PendingLoad = load;
    m_HasPendingLoad = true;
}

EdgePendingLoad EdgeControllerState::TakePendingLoad(){
    const EdgePendingLoad load = m_PendingLoad;
    m_PendingLoad = EdgePendingLoad();
    m_HasPendingLoad = false;
    return load;
}

EdgePrivateWipeLedger::Effect EdgePrivateWipeLedger::Enter(const QString &key){
    const auto found = m_States.constFind(key);
    if(found == m_States.constEnd()){
        m_States.insert(key, State::Wiping);
        return Effect::Wipe;
    }
    switch(found.value()){
    case State::Wiping: return Effect::Wait;
    case State::Clean:  return Effect::Proceed;
    case State::Failed: return Effect::Fail;
    }
    return Effect::Fail;
}

void EdgePrivateWipeLedger::Settled(const QString &key, bool ok){
    const auto found = m_States.find(key);
    if(found == m_States.end() || found.value() != State::Wiping) return;
    found.value() = ok ? State::Clean : State::Failed;
}

EdgeEnvironmentState::EdgeEnvironmentState()
    : m_State(State::Idle)
    , m_Waiters(QList<int>())
{
}

EdgeEnvironmentState::Effect EdgeEnvironmentState::AddWaiter(int token){
    if(!token) return Effect::None;

    switch(m_State){
    case State::Ready:
        return Effect::NotifyReady;
    case State::Creating:
        if(!m_Waiters.contains(token)) m_Waiters.append(token);
        return Effect::None;
    case State::Idle:
    case State::Failed:
        m_State = State::Creating;
        if(!m_Waiters.contains(token)) m_Waiters.append(token);
        return Effect::StartCreation;
    }
    return Effect::None;
}

void EdgeEnvironmentState::RemoveWaiter(int token){
    m_Waiters.removeAll(token);
}

QList<int> EdgeEnvironmentState::CreationSucceeded(){
    m_State = State::Ready;
    const QList<int> waiters = m_Waiters;
    m_Waiters.clear();
    return waiters;
}

QList<int> EdgeEnvironmentState::CreationFailed(){
    m_State = State::Failed;
    const QList<int> waiters = m_Waiters;
    m_Waiters.clear();
    return waiters;
}

EdgeMessage::EdgeMessage()
    : m_Kind(Kind::Invalid)
    , m_Code(0)
    , m_Shift(false)
{
}

EdgeMessage EdgeMessage::Parse(const QString &json, int eventKey){
    EdgeMessage message;

    if(json.isEmpty() || json.length() > MaxLength) return message;

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if(!doc.isObject()) return message;

    const QJsonObject object = doc.object();

    if(!object.value(QStringLiteral("v")).isDouble()) return message;
    if(object.value(QStringLiteral("v")).toInt() != 1) return message;

    if(!object.value(QStringLiteral("tag")).isDouble()) return message;
    if(object.value(QStringLiteral("tag")).toInt() != eventKey) return message;

    const QJsonValue kind = object.value(QStringLiteral("kind"));
    if(!kind.isString()) return message;

    if(kind.toString() == QStringLiteral("preventScrollRestoration")){
        if(object.contains(QStringLiteral("code")) ||
           object.contains(QStringLiteral("shift"))) return message;
        message.m_Kind = Kind::PreventScrollRestoration;
        return message;
    }

    if(kind.toString() == QStringLiteral("print")){
        if(object.contains(QStringLiteral("code")) ||
           object.contains(QStringLiteral("shift"))) return message;
        message.m_Kind = Kind::Print;
        return message;
    }

    if(kind.toString() == QStringLiteral("scroll")){
        if(object.contains(QStringLiteral("code")) ||
           object.contains(QStringLiteral("shift"))) return message;
        const QJsonValue x  = object.value(QStringLiteral("x"));
        const QJsonValue y  = object.value(QStringLiteral("y"));
        const QJsonValue w  = object.value(QStringLiteral("w"));
        const QJsonValue h  = object.value(QStringLiteral("h"));
        const QJsonValue vw = object.value(QStringLiteral("vw"));
        const QJsonValue vh = object.value(QStringLiteral("vh"));
        if(!x.isDouble() || !y.isDouble() || !w.isDouble() ||
           !h.isDouble() || !vw.isDouble() || !vh.isDouble()) return message;
        if(x.toDouble() < 0 || y.toDouble() < 0) return message;
        if(w.toDouble() <= 0 || h.toDouble() <= 0 ||
           vw.toDouble() <= 0 || vh.toDouble() <= 0) return message;
        message.m_Kind = Kind::Scroll;
        message.m_ScrollPosition = QPointF(x.toDouble(), y.toDouble());
        message.m_ContentsSize = QSizeF(w.toDouble(), h.toDouble());
        message.m_ViewportSize = QSizeF(vw.toDouble(), vh.toDouble());
        return message;
    }

    if(kind.toString() != QStringLiteral("key")) return message;

    const QJsonValue code = object.value(QStringLiteral("code"));
    const QJsonValue shift = object.value(QStringLiteral("shift"));
    if(!code.isDouble() || !shift.isBool()) return message;

    const int value = code.toInt();

    const bool digit = value >= 0x30 && value <= 0x39;
    const bool letter = value >= 0x41 && value <= 0x5A;
    const bool escape = value == 0x1B && !shift.toBool();
    if(!digit && !letter && !escape) return message;

    message.m_Kind = Kind::Key;
    message.m_Code = value;
    message.m_Shift = shift.toBool();
    return message;
}

EdgeGeneration::EdgeGeneration()
    : m_Current(0)
    , m_Next(1)
{
}

void EdgeGeneration::Started(){
    m_Current = 0;
}

void EdgeGeneration::Loaded(){
    m_Current = m_Next++;
}

EdgeInputLedger::EdgeInputLedger()
    : m_Ready(false)
    , m_Inside(false)
    , m_Forwarded(0)
    , m_Consumed(0)
    , m_Kept(0)
{
}

void EdgeInputLedger::SetReady(bool ready){
    m_Ready = ready;
    if(!ready){
        m_Forwarded = 0;
        m_Consumed = 0;
        m_Kept = 0;
        m_Inside = false;
    }
}

EdgeInputLedger::Send EdgeInputLedger::Press(Button button, bool hostWants){
    if(!m_Ready || button == NoButton) return Send::Nothing;

    if(hostWants){
        m_Consumed |= button;
        return Send::Nothing;
    }
    m_Forwarded |= button;
    m_Inside = true;
    return Send::ToBackend;
}

bool EdgeInputLedger::Hold(Button button){
    if(!m_Ready || button == NoButton) return false;
    m_Kept |= button;
    m_Inside = true;
    return true;
}

void EdgeInputLedger::Drop(int buttons){
    m_Kept &= ~buttons;
}

EdgeInputLedger::Send EdgeInputLedger::Release(Button button){
    if(button == NoButton) return Send::Nothing;

    if(m_Kept & button){
        m_Kept &= ~button;
        return m_Ready ? Send::DownThenUp : Send::Nothing;
    }

    if(m_Consumed & button){
        m_Consumed &= ~button;
        return Send::Nothing;
    }
    if(m_Forwarded & button){
        m_Forwarded &= ~button;
        return m_Ready ? Send::ToBackend : Send::Nothing;
    }
    return Send::Nothing;
}

EdgeInputLedger::Send EdgeInputLedger::Move(){
    if(!m_Ready) return Send::Nothing;
    m_Inside = true;
    return Send::ToBackend;
}

EdgeInputLedger::Send EdgeInputLedger::Wheel(){
    return m_Ready ? Send::ToBackend : Send::Nothing;
}

EdgeInputLedger::Send EdgeInputLedger::Enter(){
    if(!m_Ready) return Send::Nothing;
    m_Inside = true;
    return Send::ToBackend;
}

EdgeInputLedger::Send EdgeInputLedger::Leave(){
    if(!m_Ready) return Send::Nothing;
    if(AnyHeld()) return Send::Nothing;
    if(!m_Inside) return Send::Nothing;
    m_Inside = false;
    return Send::ToBackend;
}

int EdgeInputLedger::Abandon(){
    const int owed = m_Forwarded;
    m_Forwarded = 0;
    m_Consumed = 0;
    m_Kept = 0;
    m_Inside = false;
    return owed;
}

EdgeClickClock::EdgeClickClock()
    : m_Armed(false), m_Button(0), m_When(0), m_At(){}

bool EdgeClickClock::Press(int button, qint64 when, const QPointF &at,
                           int interval, qreal distance, bool reaches){
    if(!reaches){
        Reset();
        return false;
    }
    const bool dbl =
        m_Armed && button == m_Button &&
        when - m_When < interval &&
        qAbs(at.x() - m_At.x()) < distance &&
        qAbs(at.y() - m_At.y()) < distance;
    m_Armed = !dbl;
    m_Button = button;
    m_When = when;
    m_At = at;
    return dbl;
}

void EdgeClickClock::Reset(){
    m_Armed = false;
}

EdgeDropLedger::EdgeDropLedger()
    : m_Ready(false)
    , m_State(State::Idle)
{
}

void EdgeDropLedger::SetReady(bool ready){
    m_Ready = ready;
}

EdgeDropLedger::Send EdgeDropLedger::Enter(Take take){
    if(!m_Ready){
        return Send::Nothing;
    }
    switch(take){
    case Take::Consume:
        m_State = State::Consumed;
        return Send::Nothing;
    case Take::ForwardOwnUrl:
        m_State = State::ForwardedOwnUrl;
        return Send::ToBackend;
    case Take::Forward:
    default:
        m_State = State::Forwarded;
        return Send::ToBackend;
    }
}

EdgeDropLedger::Send EdgeDropLedger::Over() const {
    if(!m_Ready) return Send::Nothing;
    return IsForwarded() ? Send::ToBackend : Send::Nothing;
}

EdgeDropLedger::Send EdgeDropLedger::Leave(){
    const bool owed = IsForwarded();
    m_State = State::Idle;
    return owed ? Send::ToBackend : Send::Nothing;
}

EdgeDropLedger::Send EdgeDropLedger::Drop(){
    const bool forwarded = IsForwarded();
    m_State = State::Idle;
    return forwarded ? Send::ToBackend : Send::Nothing;
}

EdgeDropLedger::Send EdgeDropLedger::Abandon(){
    const bool owed = IsForwarded();
    m_State = State::Idle;
    return owed ? Send::ToBackend : Send::Nothing;
}

EdgeDragOutLedger::EdgeDragOutLedger()
    : m_Active(false)
    , m_Cancelled(false)
    , m_Owner(0)
    , m_DataKey(0)
{
}

bool EdgeDragOutLedger::Begin(int owner, quintptr dataKey){
    if(m_Active) return false;
    if(owner == 0 || dataKey == 0) return false;
    m_Active = true;
    m_Cancelled = false;
    m_Owner = owner;
    m_DataKey = dataKey;
    return true;
}

void EdgeDragOutLedger::End(int owner){
    if(!m_Active || owner != m_Owner) return;
    m_Active = false;
    m_Owner = 0;
    m_DataKey = 0;
}

void EdgeDragOutLedger::Cancel(int owner){
    if(!m_Active || owner != m_Owner) return;
    m_Cancelled = true;
}

EdgeDropEchoLedger::EdgeDropEchoLedger()
    : m_Armed(false)
    , m_Url(QUrl())
    , m_ArmedAtMs(0)
{
}

void EdgeDropEchoLedger::Arm(const QUrl &url, qint64 nowMs){
    if(url.isEmpty()) return;
    m_Armed = true;
    m_Url = url;
    m_ArmedAtMs = nowMs;
}

bool EdgeDropEchoLedger::Claim(const QUrl &url, qint64 nowMs){
    if(!m_Armed) return false;

    if(nowMs - m_ArmedAtMs > WindowMs){
        m_Armed = false;
        m_Url = QUrl();
        return false;
    }

    if(url != m_Url) return false;

    m_Armed = false;
    m_Url = QUrl();
    return true;
}

void EdgeDropEchoLedger::Disarm(){
    m_Armed = false;
    m_Url = QUrl();
    m_ArmedAtMs = 0;
}

Qt::CursorShape EdgeCursorShape(unsigned int systemCursorId){
    switch(systemCursorId){
    case 32512: return Qt::ArrowCursor;
    case 32513: return Qt::IBeamCursor;
    case 32514: return Qt::WaitCursor;
    case 32515: return Qt::CrossCursor;
    case 32516: return Qt::UpArrowCursor;
    case 32642: return Qt::SizeFDiagCursor;
    case 32643: return Qt::SizeBDiagCursor;
    case 32644: return Qt::SizeHorCursor;
    case 32645: return Qt::SizeVerCursor;
    case 32646: return Qt::SizeAllCursor;
    case 32648: return Qt::ForbiddenCursor;
    case 32649: return Qt::PointingHandCursor;
    case 32650: return Qt::BusyCursor;
    case 32651: return Qt::WhatsThisCursor;
    default:    return Qt::ArrowCursor;
    }
}

void AddEdgeMenuItems(QMenu *menu, const QList<EdgeMenuItem> &items,
                      const std::function<void(int)> &chosen){
    QActionGroup *radios = nullptr;
    for(const EdgeMenuItem &item : items){
        QString text = item.label;
        text.replace(QLatin1Char('&'), QStringLiteral("&&"));
        if(item.kind != EdgeMenuItem::Kind::Radio) radios = nullptr;
        switch(item.kind){
        case EdgeMenuItem::Kind::Separator:
            menu->addSeparator();
            break;
        case EdgeMenuItem::Kind::Submenu: {
            QMenu *sub = menu->addMenu(text);
            sub->menuAction()->setEnabled(item.enabled);
            AddEdgeMenuItems(sub, item.children, chosen);
            break;
        }
        default: {
            QAction *action = menu->addAction(text);
            action->setEnabled(item.enabled);
            if(item.kind != EdgeMenuItem::Kind::Command){
                action->setCheckable(true);
                action->setChecked(item.checked);
            }
            if(item.kind == EdgeMenuItem::Kind::Radio){
                if(!radios){
                    radios = new QActionGroup(menu);
                    radios->setExclusive(true);
                }
                radios->addAction(action);
            }
            action->setData(item.commandId);
            const int id = item.commandId;
            QObject::connect(action, &QAction::triggered, menu, [chosen, id](){ chosen(id);});
            break;
        }
        }
    }
}

EdgeContextMenuState::EdgeContextMenuState()
    : m_Generation(0), m_Outstanding(false), m_Retired(false)
    , m_Finishing(0), m_RetireWanted(false), m_DeleteWanted(false)
{
}

void EdgeContextMenuState::NoteDeleteLater(){
    m_DeleteWanted = true;
}

bool EdgeContextMenuState::TakeDeleteLater(){
    const bool wanted = m_DeleteWanted;
    m_DeleteWanted = false;
    return wanted;
}

int EdgeContextMenuState::Take(){
    if(m_Retired || m_Outstanding) return 0;
    m_Generation++;
    m_Outstanding = true;
    return m_Generation;
}

bool EdgeContextMenuState::Complete(int generation){
    if(!m_Outstanding || generation != m_Generation) return false;
    m_Outstanding = false;
    return true;
}

void EdgeContextMenuState::BeginFinish(){
    m_Finishing++;
}

bool EdgeContextMenuState::EndFinish(){
    if(m_Finishing > 0) m_Finishing--;
    if(m_Finishing > 0) return false;
    const bool wanted = m_RetireWanted;
    m_RetireWanted = false;
    return wanted;
}

EdgeContextMenuState::Leaving EdgeContextMenuState::Retire(bool force){
    if(m_Finishing > 0 && !force){
        m_RetireWanted = true;
        return Leaving::AfterFinish;
    }
    m_Retired = true;
    m_RetireWanted = false;
    const bool outstanding = m_Outstanding;
    m_Outstanding = false;
    return outstanding ? Leaving::WithDeferral : Leaving::Now;
}

#endif
