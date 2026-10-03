#ifndef EDGEWEBVIEWSTATE_HPP
#define EDGEWEBVIEWSTATE_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QNetworkCookie>
#include <QMultiHash>
#include <QPointer>
#include <QPointF>
#include <QSet>
#include <QSizeF>
#include <QString>
#include <functional>
#include "edgemenuitem.hpp"
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "loadending.hpp"

#include <functional>

struct EdgePendingLoad {
    QUrl url;
    QString method;
    QStringList headers;
    QByteArray body;
    QString html;
    bool isHtml = false;

    bool IsRequest() const { return !method.isEmpty();}
    bool IsHtml() const { return isHtml;}
};

class EdgeControllerState {
public:
    enum class State {
        Constructed,
        AwaitingEnvironment,
        AwaitingController,
        AwaitingScript,
        Ready,
        Failed,
        Retired,
    };

    enum class Effect {
        None,
        RequestEnvironment,
        CreateController,
        AdoptController,
        ApplyPending,
        CloseOrphan,
        CloseOwned,
        ReportFailure,
    };

    EdgeControllerState();

    State GetState() const { return m_State;}
    bool IsRetired() const { return m_State == State::Retired;}

    Effect Start();
    Effect EnvironmentReady();
    Effect EnvironmentFailed();
    Effect ControllerCreated();
    Effect ControllerFailed();

    Effect ScriptRegistered();
    Effect ScriptFailed();

    Effect Retire();

    void SetPendingLoad(const EdgePendingLoad &load);
    bool HasPendingLoad() const { return m_HasPendingLoad;}
    EdgePendingLoad TakePendingLoad();

private:
    State m_State;
    EdgePendingLoad m_PendingLoad;
    bool m_HasPendingLoad;
};

class EdgeContextMenuState {
public:
    EdgeContextMenuState();

    int Take();
    bool HasOutstanding() const { return m_Outstanding;}
    int Generation() const { return m_Generation;}

    bool Complete(int generation);

    void BeginFinish();
    bool EndFinish();
    bool IsFinishing() const { return m_Finishing > 0;}
    int FinishDepth() const { return m_Finishing;}

    enum class Leaving {
        Now,
        WithDeferral,
        AfterFinish,
    };
    Leaving Retire(bool force = false);
    bool IsRetired() const { return m_Retired;}
    bool IsRetireWanted() const { return m_RetireWanted;}

    void NoteDeleteLater();
    bool TakeDeleteLater();

private:
    int m_Generation;
    bool m_Outstanding;
    bool m_Retired;
    int m_Finishing;
    bool m_RetireWanted;
    bool m_DeleteWanted;
};

class EdgeEnvironmentState {
public:
    enum class State {
        Idle,
        Creating,
        Ready,
        Failed,
    };

    enum class Effect {
        None,
        StartCreation,
        NotifyReady,
        NotifyFailed,
    };

    EdgeEnvironmentState();

    State GetState() const { return m_State;}
    int WaiterCount() const { return static_cast<int>(m_Waiters.length());}
    bool HasWaiter(int token) const { return m_Waiters.contains(token);}

    Effect AddWaiter(int token);
    void RemoveWaiter(int token);

    QList<int> CreationSucceeded();
    QList<int> CreationFailed();

private:
    State m_State;
    QList<int> m_Waiters;
};

class EdgeInputLedger {
public:
    enum Button {
        NoButton     = 0x00,
        LeftButton   = 0x01,
        RightButton  = 0x02,
        MiddleButton = 0x04,
        XButton1     = 0x08,
        XButton2     = 0x10,
    };

    enum class Send {
        Nothing,
        ToBackend,
        DownThenUp,
    };

    EdgeInputLedger();

    bool IsInside() const { return m_Inside;}
    int Forwarded() const { return m_Forwarded;}
    int Consumed() const { return m_Consumed;}
    int Kept() const { return m_Kept;}
    bool AnyHeld() const { return (m_Forwarded | m_Consumed | m_Kept) != 0;}

    Send Press(Button button, bool hostWants);

    bool Hold(Button button);

    void Drop(int buttons);

    Send Release(Button button);

    Send Move();
    Send Wheel();

    Send Leave();
    Send Enter();

    int Abandon();

    void SetReady(bool ready);
    bool IsReady() const { return m_Ready;}

private:
    bool m_Ready;
    bool m_Inside;
    int m_Forwarded;
    int m_Consumed;
    int m_Kept;
};

class EdgeClickClock {
public:
    EdgeClickClock();

    bool Press(int button, qint64 when, const QPointF &at,
               int interval, qreal distance, bool reaches = true);
    void Reset();

private:
    bool m_Armed;
    int m_Button;
    qint64 m_When;
    QPointF m_At;
};

class EdgeDropLedger {
public:
    enum class Send {
        Nothing,
        ToBackend,
    };

    enum class Take {
        Forward,
        ForwardOwnUrl,
        Consume,
    };

    EdgeDropLedger();

    void SetReady(bool ready);
    bool IsReady() const { return m_Ready;}

    Send Enter(Take take);

    Send Over() const;

    Send Leave();
    Send Drop();

    bool IsConsuming() const { return m_State == State::Consumed;}

    bool IsForwardingOwnUrl() const { return m_State == State::ForwardedOwnUrl;}

    Send Abandon();

private:
    enum class State {
        Idle,
        Forwarded,
        ForwardedOwnUrl,
        Consumed,
    };

    bool IsForwarded() const {
        return m_State == State::Forwarded || m_State == State::ForwardedOwnUrl;
    }

    bool m_Ready;
    State m_State;
};

class EdgeDropEchoLedger {
public:
    static const qint64 WindowMs = 2000;

    EdgeDropEchoLedger();

    void Arm(const QUrl &url, qint64 nowMs);

    bool Claim(const QUrl &url, qint64 nowMs);

    void Disarm();

    bool IsArmed() const { return m_Armed;}

private:
    bool m_Armed;
    QUrl m_Url;
    qint64 m_ArmedAtMs;
};

class EdgeDragOutLedger {
public:
    EdgeDragOutLedger();

    bool Begin(int owner, quintptr dataKey);

    void End(int owner);

    void Cancel(int owner);

    bool IsActive() const { return m_Active;}
    bool IsCancelled() const { return m_Active && m_Cancelled;}
    int Owner() const { return m_Active ? m_Owner : 0;}

    bool Matches(quintptr dataKey) const {
        return m_Active && dataKey != 0 && dataKey == m_DataKey;
    }

private:
    bool m_Active;
    bool m_Cancelled;
    int m_Owner;
    quintptr m_DataKey;
};

Qt::CursorShape EdgeCursorShape(unsigned int systemCursorId);

class EdgeGeneration {
public:
    EdgeGeneration();

    int Current() const { return m_Current;}
    bool IsLive() const { return m_Current != 0;}

    void Started();
    void Loaded();

    bool StillCurrent(int generation) const {
        return generation != 0 && generation == m_Current;
    }

private:
    int m_Current;
    int m_Next;
};

class EdgeSuspendPolicy {
public:
    static bool ShouldSuspend(bool hiddenViewsStayActive,
                              bool visible,
                              bool audible,
                              bool loading,
                              bool alreadySuspended){
        if(hiddenViewsStayActive) return false;
        if(visible) return false;
        if(audible) return false;
        if(loading) return false;
        if(alreadySuspended) return false;
        return true;
    }

    static bool ShouldResumeAtOnce(bool nowSuspended, bool visible){
        return nowSuspended && visible;
    }
};

class EdgeCookieClearLedger {
public:
    EdgeCookieClearLedger() : m_Era(0), m_Pending(0) {}

    void ClearStarted(){ ++m_Era;}
    void AskPlaced(){ ++m_Pending;}

    void AskSettled(const QString &key, bool ok){
        if(ok) m_Degraded.remove(key);
        else m_Degraded.insert(key);
        if(m_Pending > 0 && --m_Pending == 0) ++m_Era;
    }

    bool IsDegraded(const QString &key) const { return m_Degraded.contains(key);}

    int Ticket() const { return m_Era;}
    bool MayWrite(int ticket, const QString &key) const {
        return m_Pending == 0 && ticket == m_Era && !m_Degraded.contains(key);
    }

private:
    int m_Era;
    int m_Pending;
    QSet<QString> m_Degraded;
};

class EdgeDownloadCloseLedger {
public:
    EdgeDownloadCloseLedger() : m_InFlight(0), m_Untracked(false) {}

    void Started(){ ++m_InFlight;}

    void Untracked(){
        if(m_InFlight > 0) --m_InFlight;
        m_Untracked = true;
    }

    void Settled(){ if(m_InFlight > 0) --m_InFlight;}

    bool MayClose() const { return !m_Untracked && m_InFlight == 0;}

    int InFlight() const { return m_InFlight;}
    bool IsUntracked() const { return m_Untracked;}

private:
    int m_InFlight;
    bool m_Untracked;
};

struct EdgeDownloadReport {
    int state;
    int reason;
    bool mayResume;
    bool terminal;
};

class EdgeDownloadCell {
public:
    enum class Backend { InProgress = 0, Interrupted = 1, Completed = 2 };

    enum Reported { ReportedInProgress = 1, ReportedCompleted = 2, ReportedInterrupted = 4 };

    enum ReportedReason { ReasonNone = 0, ReasonNetworkFailed = 20 };

    EdgeDownloadCell()
        : m_State(Backend::InProgress)
        , m_CanResume(false)
        , m_ResumeAttempted(false)
        , m_Received(0)
        , m_Total(-1)
    {}

    void SetState(Backend state, bool canResume){
        m_State = state;
        m_CanResume = canResume;
    }

    void SetProgress(qint64 received, qint64 total){
        m_Received = received > 0 ? received : 0;
        m_Total = total > 0 ? total : -1;
    }

    EdgeDownloadReport Arrive(Backend state, bool canResume){
        SetState(state, canResume);
        EdgeDownloadReport report;
        report.state = ReportedState();
        report.reason = ReportedReason();
        report.mayResume = MayResume();
        report.terminal = IsTerminal();
        return report;
    }

    void MarkResumeAttempted(){ m_ResumeAttempted = true;}
    bool ResumeAttempted() const { return m_ResumeAttempted;}

    void FailResume(){
        m_ResumeAttempted = true;
        m_State = Backend::Interrupted;
        m_CanResume = false;
    }

    Backend State() const { return m_State;}
    qint64 ReceivedBytes() const { return m_Received;}
    qint64 TotalBytes() const { return m_Total;}

    int ReportedState() const {
        switch(m_State){
        case Backend::Completed:   return ReportedCompleted;
        case Backend::Interrupted: return ReportedInterrupted;
        default:                   return ReportedInProgress;
        }
    }

    int ReportedReason() const {
        return MayResume() ? ReasonNetworkFailed : ReasonNone;
    }

    bool MayResume() const {
        return m_State == Backend::Interrupted && m_CanResume && !m_ResumeAttempted;
    }

    bool IsTerminal() const {
        if(m_State == Backend::Completed) return true;
        return m_State == Backend::Interrupted && !MayResume();
    }

private:
    Backend m_State;
    bool m_CanResume;
    bool m_ResumeAttempted;
    qint64 m_Received;
    qint64 m_Total;
};

class EdgeBackendCallLedger {
public:
    EdgeBackendCallLedger() : m_InFlight(0) {}

    void Enter(){ ++m_InFlight;}
    void Leave(){ if(m_InFlight > 0) --m_InFlight;}

    int InFlight() const { return m_InFlight;}
    bool MayRelease() const { return m_InFlight == 0;}

private:
    int m_InFlight;
};

class EdgeBackendCall {
public:
    explicit EdgeBackendCall(EdgeBackendCallLedger &ledger) : m_Ledger(ledger) { m_Ledger.Enter();}
    ~EdgeBackendCall(){ m_Ledger.Leave();}

    EdgeBackendCall(const EdgeBackendCall&) = delete;
    EdgeBackendCall &operator=(const EdgeBackendCall&) = delete;

private:
    EdgeBackendCallLedger &m_Ledger;
};

class EdgeReleaseQueue {
public:
    EdgeReleaseQueue()
        : m_Calls(EdgeBackendCallLedger())
        , m_DrainScheduled(false)
        , m_Draining(false)
        , m_DrainAttempts(0)
        , m_Reservations(0)
    {}

    EdgeBackendCallLedger &Calls(){ return m_Calls;}

    void WhenCallsAreDone(QObject *timerContext, QObject *actionContext,
                          std::function<void()> action){
        Entry entry;
        entry.context = QPointer<QObject>(actionContext);
        entry.action = action;
        m_Pending.append(entry);
        Schedule(QPointer<QObject>(timerContext));
    }

    void DrainNow(QObject *timerContext){
        RunDrain(QPointer<QObject>(timerContext), false);
    }

    int Pending() const { return m_Pending.size();}
    bool DrainScheduled() const { return m_DrainScheduled;}
    bool Draining() const { return m_Draining;}
    int DrainAttempts() const { return m_DrainAttempts;}
    int Reservations() const { return m_Reservations;}

private:
    struct Entry {
        QPointer<QObject> context;
        std::function<void()> action;
    };

    void Schedule(QPointer<QObject> timerContext){
        if(m_DrainScheduled || !timerContext) return;
        m_DrainScheduled = true;
        m_Reservations++;
        QTimer::singleShot(0, timerContext.data(), [this, timerContext](){
            OnReservationFired(timerContext);
        });
    }

    void OnReservationFired(QPointer<QObject> timerContext){
        m_DrainAttempts++;
        m_DrainScheduled = false;
        if(m_Reservations > 0) m_Reservations--;
        RunDrain(timerContext, true);
    }

    void RunDrain(QPointer<QObject> timerContext, bool ownsReservation){
        if(!m_Calls.MayRelease() || m_Draining){
            if(ownsReservation) Schedule(timerContext);
            return;
        }

        m_Draining = true;
        while(!m_Pending.isEmpty()){
            const Entry entry = m_Pending.takeFirst();
            if(!entry.context) continue;
            entry.action();
        }
        m_Draining = false;
    }

    EdgeBackendCallLedger m_Calls;
    QList<Entry> m_Pending;
    bool m_DrainScheduled;
    bool m_Draining;
    int m_DrainAttempts;
    int m_Reservations;
};

class EdgeAbortLatch {
public:
    enum class Verdict {
        Nothing,
        Hold,
        Say,
    };

    static const int Capacity = 4;

    static const qint64 Wait = 1500;

    EdgeAbortLatch()
        : m_CurrentId(0)
        , m_HasCurrent(false)
        , m_StopId(0)
        , m_HasStop(false)
        , m_NextGeneration(1)
        , m_Retired(false)
    {}

    void Started(quint64 id, const QString &url){
        if(m_Retired) return;

        if(m_HasCurrent && id == m_CurrentId){
            m_CurrentUrl = url;
            for(int i = 0; i < m_Entries.size(); i++)
                if(m_Entries[i].id == id) m_Entries[i].url = url;
            return;
        }

        m_CurrentId = id;
        m_HasCurrent = true;
        m_CurrentUrl = url;
        m_HasStop = false;
        for(int i = 0; i < m_Entries.size(); i++) m_Entries[i].superseded = true;
    }

    void Stopped(bool loading){
        if(m_Retired || !loading || !m_HasCurrent) return;
        m_StopId = m_CurrentId;
        m_HasStop = true;
    }

    Verdict Completed(quint64 id, bool success, int webErrorStatus,
                      qint64 now, int *generation){
        if(generation) *generation = 0;
        if(m_Retired) return Verdict::Nothing;
        if(!m_HasCurrent || id != m_CurrentId) return Verdict::Nothing;

        if(success){
            m_HasStop = false;
            return Verdict::Nothing;
        }

        if(webErrorStatus != LoadEnding::EdgeConnectionAborted &&
           webErrorStatus != LoadEnding::EdgeOperationCanceled)
            return Verdict::Say;

        if(m_HasStop && m_StopId == id){
            m_HasStop = false;
            return Verdict::Nothing;
        }

        Entry entry;
        entry.id = id;
        entry.url = m_CurrentUrl;
        entry.generation = m_NextGeneration++;
        entry.arrival = now;
        entry.superseded = false;
        m_Entries.append(entry);
        while(m_Entries.size() > Capacity) m_Entries.removeFirst();

        if(generation) *generation = entry.generation;
        return Verdict::Hold;
    }

    void DownloadStarted(const QString &uri){
        if(m_Retired) return;
        for(int i = 0; i < m_Entries.size(); i++){
            if(m_Entries[i].url != uri) continue;
            m_Entries.removeAt(i);
            return;
        }
    }

    Verdict Elapsed(qint64 now, int generation){
        for(int i = 0; i < m_Entries.size(); i++){
            if(m_Entries[i].generation != generation) continue;
            const bool superseded = m_Entries[i].superseded;
            if(!superseded && now - m_Entries[i].arrival < Wait)
                return Verdict::Nothing;
            m_Entries.removeAt(i);
            return superseded ? Verdict::Nothing : Verdict::Say;
        }
        return Verdict::Nothing;
    }

    qint64 Remaining(qint64 now, int generation) const {
        for(int i = 0; i < m_Entries.size(); i++){
            if(m_Entries[i].generation != generation) continue;
            if(m_Entries[i].superseded) return -1;
            const qint64 left = m_Entries[i].arrival + Wait - now;
            return left > 0 ? left : 0;
        }
        return -1;
    }

    void Retire(){
        m_Entries.clear();
        m_HasCurrent = false;
        m_HasStop = false;
        m_Retired = true;
    }

    int Held() const { return m_Entries.size();}
    bool HasStop() const { return m_HasStop;}
    bool IsRetired() const { return m_Retired;}

private:
    struct Entry {
        quint64 id;
        QString url;
        int generation;
        qint64 arrival;
        bool superseded;
    };

    QList<Entry> m_Entries;
    quint64 m_CurrentId;
    bool m_HasCurrent;
    QString m_CurrentUrl;
    quint64 m_StopId;
    bool m_HasStop;
    int m_NextGeneration;
    bool m_Retired;
};

class EdgePrivateWipeLedger {
public:
    enum class Effect {
        Wipe,
        Wait,
        Proceed,
        Fail,
    };

    Effect Enter(const QString &key);
    void Settled(const QString &key, bool ok);

    bool IsClean(const QString &key) const {
        return m_States.value(key, State::Wiping) == State::Clean;
    }
    bool IsFailed(const QString &key) const {
        return m_States.value(key, State::Wiping) == State::Failed;
    }

private:
    enum class State { Wiping, Clean, Failed };
    QHash<QString, State> m_States;
};

class EdgeClearRoster {
public:
    bool ShouldAsk(const QString &key) const { return !m_Done.contains(key);}
    void Asked(const QString &key, bool placed){ if(placed) m_Done.insert(key);}
private:
    QSet<QString> m_Done;
};

QVariant EdgeScriptResultToVariant(const QByteArray &json);

QUrl EdgeWithRootPath(const QUrl &url);

bool EdgeIsOwnViewSource(const QUrl &shown, const QUrl &reported);

bool EdgeIsOwnStringDocument(const QUrl &shown, const QUrl &stringDocument,
                             const QUrl &reported);

class EdgeStringDocument {
public:
    void LoadStarted(const EdgePendingLoad &load){
        m_Url = load.IsHtml() ? load.url : QUrl();
    }
    void ViewWentElsewhere(){ m_Url = QUrl();}
    bool IsOwn(const QUrl &shown, const QUrl &reported) const {
        return EdgeIsOwnStringDocument(shown, m_Url, reported);
    }
    QUrl Url() const { return m_Url;}

private:
    QUrl m_Url;
};

QPointF EdgeScrollRatio(const QPointF &scroll, const QSizeF &contents,
                        const QSizeF &viewport);

QNetworkCookie EdgeCookieFromParts(const QString &name, const QString &value,
                                   const QString &domain, const QString &path,
                                   double expires, bool httpOnly, bool secure);

class EdgeOnce {
public:
    EdgeOnce() : m_Taken(false) {}
    bool Take(){
        if(m_Taken) return false;
        m_Taken = true;
        return true;
    }
private:
    bool m_Taken;
};

class EdgeDocumentCoordinator {
public:
    EdgeDocumentCoordinator() : m_DocumentActive(false) {}

    bool HasLiveDocument() const { return m_Generation.IsLive();}
    int Generation() const { return m_Generation.Current();}
    bool StillCurrent(int generation) const { return m_Generation.StillCurrent(generation);}
    bool IsDocumentActive() const { return m_DocumentActive;}

    void NavigationStarted(){
        m_DocumentActive = false;
        m_Generation.Started();
    }
    void DocumentArrived(){
        m_DocumentActive = true;
        m_Generation.Loaded();
    }

private:
    EdgeGeneration m_Generation;
    bool m_DocumentActive;
};

template <class View>
class EdgeProfileCoordinatorOf {
public:

    void Opened(View *view){ if(view) m_Live.insert(view);}

    void Closed(View *view){
        m_Live.remove(view);
        for(auto it = m_Waiters.begin(); it != m_Waiters.end();){
            if(it.value().isNull() || it.value() == view) it = m_Waiters.erase(it);
            else ++it;
        }
    }

    QList<View*> LiveViews() const { return m_Live.values();}

    typename EdgePrivateWipeLedger::Effect
    EnterPrivateProfile(const QString &key, View *view){
        const typename EdgePrivateWipeLedger::Effect effect = m_Wipes.Enter(key);
        if(effect == EdgePrivateWipeLedger::Effect::Wipe ||
           effect == EdgePrivateWipeLedger::Effect::Wait)
            m_Waiters.insert(key, QPointer<View>(view));
        return effect;
    }

    QList<QPointer<View>> SettlePrivateWipe(const QString &key, bool ok){
        m_Wipes.Settled(key, ok);
        const QList<QPointer<View>> waiters = m_Waiters.values(key);
        m_Waiters.remove(key);
        return waiters;
    }

    bool IsPrivateProfileClean(const QString &key) const { return m_Wipes.IsClean(key);}
    int WaitersFor(const QString &key) const { return m_Waiters.values(key).length();}

    EdgeCookieClearLedger &Cookies(){ return m_Cookies;}

private:
    QSet<View*> m_Live;

    EdgePrivateWipeLedger m_Wipes;
    QMultiHash<QString, QPointer<View>> m_Waiters;
    EdgeCookieClearLedger m_Cookies;
};

enum class EdgeControllerOptionsAnswer {
    Proceed,
    ShareDefaultProfile,
    RefusePrivate,
};

inline EdgeControllerOptionsAnswer
EdgeAnswerForControllerOptions(bool privateMode, bool optionsCarried){
    if(optionsCarried) return EdgeControllerOptionsAnswer::Proceed;
    return privateMode ? EdgeControllerOptionsAnswer::RefusePrivate
                       : EdgeControllerOptionsAnswer::ShareDefaultProfile;
}

enum class EdgeControllerCreation { Composition, WindowedWithOptions, WindowedPlain };

inline EdgeControllerCreation
EdgeChooseControllerCreation(bool canUseOptions, bool hasCompositionTree){
    if(!canUseOptions) return EdgeControllerCreation::WindowedPlain;
    return hasCompositionTree ? EdgeControllerCreation::Composition
                              : EdgeControllerCreation::WindowedWithOptions;
}

inline bool EdgeFailureShowsCode(long result){ return result < 0;}
inline bool EdgeFailureMayBeFolderClash(long result){
    return static_cast<unsigned long>(result) == 0x8007139FUL;
}

class EdgeMessage {
public:
    enum class Kind {
        Invalid,
        Key,
        PreventScrollRestoration,
        Print,
        Scroll,
    };

    static const int MaxLength = 512;

    EdgeMessage();

    Kind GetKind() const { return m_Kind;}
    bool IsValid() const { return m_Kind != Kind::Invalid;}
    int GetCode() const { return m_Code;}
    bool GetShift() const { return m_Shift;}
    QPointF GetScrollPosition() const { return m_ScrollPosition;}
    QSizeF GetContentsSize() const { return m_ContentsSize;}
    QSizeF GetViewportSize() const { return m_ViewportSize;}

    static EdgeMessage Parse(const QString &json, int eventKey);

    static bool IsFromDocument(const QString &source, const QUrl &document){
        return EdgeWithRootPath(QUrl(source)) == EdgeWithRootPath(document);
    }

private:
    Kind m_Kind;
    int m_Code;
    bool m_Shift;
    QPointF m_ScrollPosition;
    QSizeF m_ContentsSize;
    QSizeF m_ViewportSize;
};

class EdgeExtensionPageState {
public:
    enum class Effect { None, Navigate, Fail, Answer };

    EdgeExtensionPageState() : m_Closed(false), m_Settled(false) {}

    Effect ScriptRegistered(bool succeeded){
        if(m_Closed || m_Settled) return Effect::None;
        m_Settled = true;
        return succeeded ? Effect::Navigate : Effect::Fail;
    }
    Effect AnswerDue() const {
        return m_Closed ? Effect::None : Effect::Answer;
    }
    void Close(){ m_Closed = true;}
    bool IsClosed() const { return m_Closed;}

private:
    bool m_Closed;
    bool m_Settled;
};

class EdgeExtensionOperation {
public:
    enum class Kind { Enable, Remove };
    enum class Effect {
        None,
        ReportSuccess,
        ReportFailure,
        ReportGone,
        Snapshot,
        Retry
    };

    explicit EdgeExtensionOperation(Kind kind) : m_Kind(kind), m_Stage(Stage::First) {}

    Kind GetKind() const { return m_Kind;}
    bool IsDone() const { return m_Stage == Stage::Done;}

    Effect First(bool succeeded){
        if(m_Stage != Stage::First) return Effect::None;
        if(succeeded){
            m_Stage = Stage::Done;
            return Effect::ReportSuccess;
        }
        m_Stage = Stage::Snapshot;
        return Effect::Snapshot;
    }
    Effect SnapshotResult(bool succeeded, bool listed){
        if(m_Stage != Stage::Snapshot) return Effect::None;
        if(!succeeded){
            m_Stage = Stage::Done;
            return Effect::ReportFailure;
        }
        if(!listed){
            m_Stage = Stage::Done;
            return m_Kind == Kind::Remove ? Effect::ReportSuccess : Effect::ReportGone;
        }
        m_Stage = Stage::Retry;
        return Effect::Retry;
    }
    Effect RetryResult(bool succeeded){
        if(m_Stage != Stage::Retry) return Effect::None;
        m_Stage = Stage::Done;
        return succeeded ? Effect::ReportSuccess : Effect::ReportFailure;
    }

private:
    enum class Stage { First, Snapshot, Retry, Done };
    Kind m_Kind;
    Stage m_Stage;
};

struct EdgeExtensionTabCandidate {
    int id = -1;
    QUrl url;
};

class EdgeExtensionTabRequest {
public:
    static EdgeExtensionTabRequest Parse(const QString &json){
        EdgeExtensionTabRequest request;
        const QJsonObject object = QJsonDocument::fromJson(json.toUtf8()).object();
        if(object.value(QStringLiteral("vanilla")).toString() != QStringLiteral("extension-tab"))
            return request;
        const QJsonValue document = object.value(QStringLiteral("doc"));
        const QJsonValue sequence = object.value(QStringLiteral("seq"));
        const QJsonValue tabs = object.value(QStringLiteral("tabs"));
        if(!document.isString() || document.toString().isEmpty() || document.toString().size() > 64 ||
           !sequence.isDouble() || !tabs.isArray()) return request;
        foreach(const QJsonValue &value, tabs.toArray()){
            const QJsonObject tab = value.toObject();
            const QJsonValue id = tab.value(QStringLiteral("id"));
            const QJsonValue url = tab.value(QStringLiteral("url"));
            if(!id.isDouble() || !url.isString()) return request;
            EdgeExtensionTabCandidate candidate;
            candidate.id = id.toInt(-1);
            candidate.url = QUrl(url.toString());
            request.m_Candidates.append(candidate);
        }
        request.m_Document = document.toString();
        request.m_Sequence = sequence.toInt();
        request.m_Valid = true;
        return request;
    }

    bool IsValid() const { return m_Valid;}
    QString Document() const { return m_Document;}
    int Sequence() const { return m_Sequence;}
    QList<EdgeExtensionTabCandidate> Candidates() const { return m_Candidates;}

    static int Find(const QList<EdgeExtensionTabCandidate> &candidates, const QUrl &source){
        if(source.isEmpty()) return -1;
        int found = -1;
        foreach(const EdgeExtensionTabCandidate &candidate, candidates){
            if(candidate.url != source) continue;
            if(found >= 0) return -1;
            found = candidate.id;
        }
        return found;
    }

    static int Answer(const QList<EdgeExtensionTabCandidate> &candidates,
                      const QUrl &sourceAtArrival, const QUrl &sourceNow, bool sees){
        if(!sees || sourceAtArrival != sourceNow) return -1;
        return Find(candidates, sourceNow);
    }

    static QString Reply(const QString &document, int sequence, int id){
        QJsonObject object;
        object.insert(QStringLiteral("vanilla"), QStringLiteral("extension-tab"));
        object.insert(QStringLiteral("doc"), document);
        object.insert(QStringLiteral("seq"), sequence);
        object.insert(QStringLiteral("id"), id < 0 ? QJsonValue() : QJsonValue(id));
        return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
    }

private:
    bool m_Valid = false;
    QString m_Document;
    int m_Sequence = 0;
    QList<EdgeExtensionTabCandidate> m_Candidates;
};

#endif
#endif
