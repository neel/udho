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
struct delegate_evaluator{
    using states_type = typename udho::manifold::detail::states_for_components<Components...>::type;

    delegate_evaluator(states_type& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _states(states), _address(address), _request(request) {}

    template <typename ComponentT>
    bool operator()(udho::manifold::delegate_wrapper<ComponentT, true>& wrapper) { return wrapper.eval(_states, _address, _request); }

    template <typename ComponentT>
    bool operator()(udho::manifold::delegate_wrapper<ComponentT, false>&) { return false; }

    states_type& _states;

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

}

template <typename FeatureX, typename... Features>
struct evaluator<FeatureX, Features...>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Components>
    std::size_t operator()(udho::manifold::delegates<Components...>& delegates, typename udho::manifold::detail::states_for_components<Components...>::type& states) {
        static_assert(delegates.template count<FeatureX>() > 0, "Feature missing in the manifold facade");

        std::size_t count = delegates.template apply<FeatureX>( detail::delegate_evaluator<Components...>{states, _address, _request} );
        evaluator<Features...> ev{_address, _request};
        count += ev(delegates, states);
        return count;
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

template <typename FeatureX>
struct evaluator<FeatureX>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Components>
    std::size_t operator()(udho::manifold::delegates<Components...>& delegates, typename udho::manifold::detail::states_for_components<Components...>::type& states) {
        static_assert(delegates.template count<FeatureX>() > 0, "Feature missing in the manifold facade");

        return delegates.template apply<FeatureX>( detail::delegate_evaluator<Components...>{states, _address, _request} );
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};


}
}

#endif // UDHO_MANIFOLD_EVALUATOR_H
