#ifndef EDGEUNADOPTEDCONTROLLER_HPP
#define EDGEUNADOPTEDCONTROLLER_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wrl/client.h>

template <class Controller>
class EdgeUnadoptedControllerOf {
public:
    explicit EdgeUnadoptedControllerOf(Controller *controller)
        : m_Controller(controller)
    {
    }

    ~EdgeUnadoptedControllerOf(){
        if(m_Controller) m_Controller->Close();
    }

    EdgeUnadoptedControllerOf(const EdgeUnadoptedControllerOf&) = delete;
    EdgeUnadoptedControllerOf &operator=(const EdgeUnadoptedControllerOf&) = delete;
    EdgeUnadoptedControllerOf(EdgeUnadoptedControllerOf&&) = delete;
    EdgeUnadoptedControllerOf &operator=(EdgeUnadoptedControllerOf&&) = delete;

    Controller *Get() const { return m_Controller.Get();}
    bool IsHeld() const { return m_Controller;}

    Microsoft::WRL::ComPtr<Controller> Take(){
        Microsoft::WRL::ComPtr<Controller> taken;
        taken.Swap(m_Controller);
        return taken;
    }

private:
    Microsoft::WRL::ComPtr<Controller> m_Controller;
};

template <class ViewPointer, class Controller, class Adopt, class Fail>
void EdgeSettleControllerArrival(const ViewPointer &view, bool succeeded,
                                 Controller *controller,
                                 Adopt adopt, Fail fail){
    if(!succeeded || !controller){
        if(view) fail();
        return;
    }
    EdgeUnadoptedControllerOf<Controller> made(controller);
    if(!view) return;
    adopt(made);
}

#endif

#endif
