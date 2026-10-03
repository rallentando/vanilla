#ifndef SHUTDOWNFLOW_HPP
#define SHUTDOWNFLOW_HPP

#include "callback.hpp"

#include <QCoreApplication>
#include <QTimer>

class ShutdownFlow {
public:
    enum class Stage {
        Start,
        WaitingForPriorSave,
        ReadyForMedia,
        WaitingForMedia,
        ReadyForFinalSave,
        FinalSave
    };
    enum class Action {
        None,
        WaitForPriorSave,
        CaptureMedia,
        FinalSave,
        Exit
    };

    explicit ShutdownFlow(VoidCallBack queueContinuation)
        : m_Stage(Stage::Start), m_QueueContinuation(queueContinuation),
          m_MainLoopEntered(false), m_ExitRequested(false), m_ExitIssued(false) {}

    Stage CurrentStage() const { return m_Stage;}

    Action Begin(){
        if(m_Stage != Stage::Start) return Action::None;
        m_Stage = Stage::WaitingForPriorSave;
        return Action::WaitForPriorSave;
    }

    bool WaitForAnotherSave(){
        if(m_Stage != Stage::ReadyForFinalSave) return false;
        m_Stage = Stage::WaitingForPriorSave;
        return true;
    }

    void PriorSaveSettled(){
        if(m_Stage != Stage::WaitingForPriorSave) return;
        m_Stage = Stage::ReadyForMedia;
        QueueContinuation();
    }

    void MediaSettled(){
        if(m_Stage != Stage::WaitingForMedia) return;
        m_Stage = Stage::ReadyForFinalSave;
        QueueContinuation();
    }

    Action Continue(){
        if(m_Stage == Stage::ReadyForMedia){
            m_Stage = Stage::WaitingForMedia;
            return Action::CaptureMedia;
        }
        if(m_Stage == Stage::ReadyForFinalSave){
            m_Stage = Stage::FinalSave;
            return Action::FinalSave;
        }
        return Action::None;
    }

    Action EnterMainLoop(){
        m_MainLoopEntered = true;
        return IssueExit();
    }

    Action RequestExit(){
        m_ExitRequested = true;
        return IssueExit();
    }

private:
    Action IssueExit(){
        if(!m_MainLoopEntered || !m_ExitRequested || m_ExitIssued)
            return Action::None;
        m_ExitIssued = true;
        return Action::Exit;
    }

    void QueueContinuation(){
        if(m_QueueContinuation) m_QueueContinuation();
    }

    Stage m_Stage;
    VoidCallBack m_QueueContinuation;
    bool m_MainLoopEntered;
    bool m_ExitRequested;
    bool m_ExitIssued;
};

namespace ExitHandoff {

template <class App>
inline int Run(QCoreApplication *app, ShutdownFlow &flow){
    if(flow.EnterMainLoop() == ShutdownFlow::Action::Exit)
        QTimer::singleShot(0, app, &QCoreApplication::quit);
    return App::exec();
}

inline void RequestExit(QCoreApplication *app, ShutdownFlow &flow){
    if(flow.RequestExit() == ShutdownFlow::Action::Exit)
        app->quit();
}

}

#endif
