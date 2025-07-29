#ifndef UDHO_MIDDLEWARE_EVALUATOR_H
#define UDHO_MIDDLEWARE_EVALUATOR_H

#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>
#include <udho/middleware/fwd.h>
#include <udho/middleware/features.h>
#include <udho/middleware/state.h>

namespace udho {
namespace middleware {


namespace detail {

template <typename... Components>
struct component_evaluator{
    component_evaluator(udho::middleware::states<Components...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _states(states), _address(address), _request(request) {}

    template <typename ComponentT>
    bool operator()(ComponentT& component) {
        return component.eval(_states, _address, _request);
    }

    udho::middleware::states<Components...>& _states;

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

}

template <typename FeatureX, typename... Features>
struct evaluator<FeatureX, Features...>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Components>
    std::size_t operator()(udho::middleware::composition<Components...>& facade, udho::middleware::states<Components...>& states) {
        static_assert(facade.template count<FeatureX>() > 0, "Feature missing in the middleware facade");

        std::size_t count = facade.template apply<FeatureX>( detail::component_evaluator<Components...>{states, _address, _request} );
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

    template <typename... Components>
    std::size_t operator()(udho::middleware::composition<Components...>& facade, udho::middleware::states<Components...>& states) {
        static_assert(facade.template count<FeatureX>() > 0, "Feature missing in the middleware facade");

        return facade.template apply<FeatureX>( detail::component_evaluator<Components...>{states, _address, _request} );
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};


}
}

#endif // UDHO_MIDDLEWARE_EVALUATOR_H
