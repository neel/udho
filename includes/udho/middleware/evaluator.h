#ifndef UDHO_MIDDLEWARE_EVALUATOR_H
#define UDHO_MIDDLEWARE_EVALUATOR_H

#include <udho/middleware/fwd.h>
#include <udho/middleware/features.h>
#include <udho/middleware/component.h>
#include <udho/middleware/facade.h>
#include <udho/middleware/state.h>

namespace udho {
namespace middleware {


namespace detail {

template <typename HeadT, typename... Tail>
struct component_evaluator{
    component_evaluator(udho::middleware::states<HeadT, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _states(states), _address(address), _request(request) {}

    template <typename ComponentT>
    bool operator()(ComponentT& component) {
        return component.eval(_states, _address, _request);
    }

    udho::middleware::states<HeadT, Tail...>& _states;
    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

}

template <typename FeatureX, typename... Features>
struct evaluator{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename HeadT, typename... Tail>
    std::size_t operator()(udho::middleware::facade_chain<HeadT, Tail...>& facade, udho::middleware::states<HeadT, Tail...>& states) {
        static_assert(facade.template count<FeatureX>() > 0, "Feature missing in the middleware facade");

        std::size_t count = facade.template apply<FeatureX>( detail::component_evaluator<HeadT, Tail...>{states, _address, _request} );
        evaluator<Features...> ev{_address, _request};
        count += ev(facade.tail(), states.tail());
        return count;
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

template <typename FeatureX>
struct evaluator<FeatureX>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename HeadT, typename... Tail>
    std::size_t operator()(udho::middleware::facade_chain<HeadT, Tail...>& facade, udho::middleware::states<HeadT, Tail...>& states) {
        static_assert(facade.template count<FeatureX>() > 0, "Feature missing in the middleware facade");

        return facade.template apply<FeatureX>( detail::component_evaluator<HeadT, Tail...>{states, _address, _request} );
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};


}
}

#endif // UDHO_MIDDLEWARE_EVALUATOR_H
