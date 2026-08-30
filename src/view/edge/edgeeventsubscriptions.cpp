#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgeeventsubscriptions.hpp"

#include <QtGlobal>

EdgeEventSubscriptions::EdgeEventSubscriptions()
    : m_Stage(Stage::Active)
    , m_Revokers(std::vector<std::function<void()>>())
{
}

EdgeEventSubscriptions::~EdgeEventSubscriptions(){
    Q_ASSERT(m_Revokers.empty());
}

void EdgeEventSubscriptions::Keep(std::function<void()> revoke){
    if(!revoke) return;

    if(m_Stage != Stage::Active) return;
    m_Revokers.push_back(std::move(revoke));
}

void EdgeEventSubscriptions::RevokeAll(){
    if(m_Stage != Stage::Active) return;
    m_Stage = Stage::Revoking;

    std::vector<std::function<void()>> revokers;
    revokers.swap(m_Revokers);

    for(const auto &revoke : revokers) revoke();

    m_Stage = Stage::Revoked;
}

bool EdgeEventSubscriptions::IsEmpty() const {
    return m_Revokers.empty();
}

int EdgeEventSubscriptions::Count() const {
    return static_cast<int>(m_Revokers.size());
}

bool EdgeEventSubscriptions::IsRevoked() const {
    return m_Stage != Stage::Active;
}

#endif
