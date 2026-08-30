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
#include <QStringList>
#include <QUrl>
#include <QVariant>

struct EdgePendingLoad {
    QUrl url;
    QString method;
    QStringList headers;
    QByteArray body;

    bool IsRequest() const { return !method.isEmpty();}
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

#endif
#endif
