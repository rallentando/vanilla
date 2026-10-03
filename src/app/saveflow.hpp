#ifndef SAVEFLOW_HPP
#define SAVEFLOW_HPP

class SaveFlow {

public:
    enum class SettleAction { StartPending, Finished, Failed };

    bool Request(){
        if(m_Saving){
            m_Pending = true;
            return false;
        }
        m_Saving = true;
        m_Failed = false;
        return true;
    }

    SettleAction Settle(bool success){
        if(!success) m_Failed = true;
        if(m_Pending){
            m_Pending = false;
            return SettleAction::StartPending;
        }
        m_Saving = false;
        return m_Failed ? SettleAction::Failed : SettleAction::Finished;
    }

    bool IsSaving() const { return m_Saving;}
    bool HasPending() const { return m_Pending;}

private:
    bool m_Saving = false;
    bool m_Pending = false;
    bool m_Failed = false;
};

class SaveStageFlow {

public:
    template<typename Writer>
    void Run(const Writer &writer){
        if(!writer()) m_Succeeded = false;
    }

    void Fail(){ m_Succeeded = false;}
    bool Succeeded() const { return m_Succeeded;}

private:
    bool m_Succeeded = true;
};

#endif
