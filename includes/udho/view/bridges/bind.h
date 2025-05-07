#ifndef UDHO_VIEW_BRIDGES_BIND_H
#define UDHO_VIEW_BRIDGES_BIND_H

#include <iostream>
#include <udho/view/bridges/fwd.h>
#include <udho/view/data/type.h>
#include <udho/view/data/detail/bind.h>
#include <udho/view/data/bindings.h>

namespace udho{
namespace view{
namespace data{

namespace bridges{

/**
 * @brief supposed to be instantiated by the bridge itself for binding any type with that bridge.
 * @warning Do not specialize. Not intended to be used directly by the user code.
 * @details checks whether the type is already bound or not. If not then forwards to @ref udho::view::data::bind
 * @details borrows state. does not own anything.
 * @ingroup view
 */
template <typename BridgeT>
struct bind{
    using state_type  = typename BridgeT::state_type;

    bind(state_type& state): _state(state) {}

    template <typename ClassT>
    void operator()(udho::view::data::type<ClassT>){
        // TODO now as we are dealing with multiple states we need to keep tract whether the type was bounded on a particular state or not.
        if(!udho::view::data::bindings<state_type, ClassT>::exists(_state)){
            udho::view::data::bind<BridgeT, ClassT>::apply(_state);
            udho::view::data::bindings<state_type, ClassT>::bound_one(_state);
        } else {
            // bindings already exists no need to do it again.
            std::cout << "udho::view::data::bridges::bind: " << "skipped binding" << std::endl;
        }
    }

private:
    state_type& _state;
};

}

}
}
}

#endif // UDHO_VIEW_BRIDGES_BIND_H
