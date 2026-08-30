#ifndef EDGEEVENTSUBSCRIPTIONS_HPP
#define EDGEEVENTSUBSCRIPTIONS_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wrl/client.h>
#include <eventtoken.h>

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

class EdgeEventSubscriptions {
public:
    EdgeEventSubscriptions();
    ~EdgeEventSubscriptions();

    EdgeEventSubscriptions(const EdgeEventSubscriptions&) = delete;
    EdgeEventSubscriptions &operator=(const EdgeEventSubscriptions&) = delete;
    EdgeEventSubscriptions(EdgeEventSubscriptions&&) = delete;
    EdgeEventSubscriptions &operator=(EdgeEventSubscriptions&&) = delete;

    void Keep(std::function<void()> revoke);

    void RevokeAll();

    bool IsEmpty() const;
    int Count() const;
    bool IsRevoked() const;

private:
    enum class Stage { Active, Revoking, Revoked };

    Stage m_Stage;
    std::vector<std::function<void()>> m_Revokers;
};

template <class Interface>
inline void KeepEventToken
    (EdgeEventSubscriptions &subscriptions, Interface *source,
     HRESULT (STDMETHODCALLTYPE Interface::*remove)(EventRegistrationToken),
     EventRegistrationToken token){

    if(!source || !remove) return;
    Microsoft::WRL::ComPtr<Interface> held(source);
    subscriptions.Keep([held, remove, token](){ (held.Get()->*remove)(token);});
}

#define VANILLA_KEEP_EVENT_TOKEN(subscriptions, source, event, token)   \
    KeepEventToken                                                      \
        ((subscriptions), (source),                                     \
         &std::remove_pointer_t<std::decay_t<decltype(source)>>::remove_##event, \
         (token))

#endif

#endif
