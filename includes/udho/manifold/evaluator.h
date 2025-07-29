#ifndef UDHO_MANIFOLD_EVALUATOR_H
#define UDHO_MANIFOLD_EVALUATOR_H

#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/state.h>

namespace udho {
namespace manifold {


namespace detail {

template <typename... Components>
struct component_evaluator{
    component_evaluator(udho::manifold::states<Components...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _states(states), _address(address), _request(request) {}

    template <typename ComponentT>
    bool operator()(ComponentT& component) {
        return component.eval(_states, _address, _request);
    }

    udho::manifold::states<Components...>& _states;

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

}

template <typename FeatureX, typename... Features>
struct evaluator<FeatureX, Features...>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Components>
    std::size_t operator()(udho::manifold::composition<Components...>& composition, udho::manifold::states<Components...>& states) {
        static_assert(composition.template count<FeatureX>() > 0, "Feature missing in the manifold facade");

        std::size_t count = composition.template apply<FeatureX>( detail::component_evaluator<Components...>{states, _address, _request} );
        evaluator<Features...> ev{_address, _request};
        count += ev(composition, states);
        return count;
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

template <typename FeatureX>
struct evaluator<FeatureX>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Components>
    std::size_t operator()(udho::manifold::composition<Components...>& composition, udho::manifold::states<Components...>& states) {
        static_assert(composition.template count<FeatureX>() > 0, "Feature missing in the manifold facade");

        return composition.template apply<FeatureX>( detail::component_evaluator<Components...>{states, _address, _request} );
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};


}
}

#endif // UDHO_MANIFOLD_EVALUATOR_H
