#ifndef UDHO_MANIFOLD_WRAPPER_H
#define UDHO_MANIFOLD_WRAPPER_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <boost/asio/ip/address.hpp>
#include <udho/utils/traits.h>
#include <udho/net/common.h>
#include <udho/manifold/traits.h>
#include <udho/manifold/detail.h>

namespace udho {
namespace manifold {

template <typename ComponentT, typename FeatureT>
struct interface;

namespace detail{

template <typename ComponentT, bool ReferencePreferred = component_traits<ComponentT>::prefer_reference, bool DefaultConstructible = std::is_default_constructible_v<ComponentT>>
struct component_member{
    using component_type   = ComponentT;

    static constexpr component_type* dummy = 0x0;

    static constexpr bool const is_default_constructible = false;
    static constexpr bool const is_move_constructible    = false;

    inline explicit component_member(): _component(*dummy) {
        static_assert(is_default_constructible, "Expecting lvalue reference for ComponentT because component_traits<ComponentT>::prefer_reference is true, but no feasible argument was found");
    }

    inline explicit component_member(component_type& component_ref): _component(component_ref) {}
    inline explicit component_member(default_constructed&&): _component(*dummy) {
        static_assert(false, "Expecting lvalue reference for ComponentT because component_traits<ComponentT>::prefer_reference is true, but no feasible argument was found");
    }

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

private:
    component_type& _component;
};

template <typename ComponentT>
struct component_member<ComponentT, false, true>{
    using component_type   = ComponentT;

    static constexpr bool const is_default_constructible = true;
    static constexpr bool const is_move_constructible    = true;

    template <typename Arg, std::enable_if_t<detail::argument_traits<ComponentT>::template should_move<Arg>, bool> = true>
    inline explicit component_member(Arg component_rval): _component(std::move(component_rval)) {}

    inline explicit component_member(): _component() {}
    inline explicit component_member(default_constructed&&): _component() {}

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

private:

    component_type _component;
};

template <typename ComponentT>
struct component_member<ComponentT, false, false>{
    using component_type   = ComponentT;

    static constexpr bool const is_default_constructible = false;
    static constexpr bool const is_move_constructible    = true;

    template <typename Arg, std::enable_if_t<detail::argument_traits<ComponentT>::template should_move<Arg>, bool> = true>
    inline explicit component_member(Arg component_rval): _component(std::move(component_rval)) {}

    inline explicit component_member() {
        static_assert(is_default_constructible, "No argument supplied for non-default constructible Component");
    }

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

private:

    component_type _component;
};

}

template <typename ComponentT, typename FeatureT>
struct interface: protected detail::component_member<ComponentT> {
    using component_type    = ComponentT;
    using member_type       = detail::component_member<ComponentT>;
    using state_type        = typename ComponentT::state;
    using feature           = FeatureT;

    template <typename Head, typename... Tail>
    state_type eval(const udho::manifold::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return member_type::component().eval(states, address, request);
    }

    using member_type::member_type;
};

template <typename ComponentT>
struct interface<ComponentT, features::filter>: protected detail::component_member<ComponentT> {
    using component_type    = ComponentT;
    using member_type       = detail::component_member<ComponentT>;
    using state_type        = typename ComponentT::state;
    using feature           = features::filter;

    inline bool operator()(const boost::asio::ip::address& address){ return member_type::component()(address); }

    template <typename Head, typename... Tail>
    state_type eval(const udho::manifold::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return member_type::component().eval(states, address, request);
    }

    using member_type::member_type;
};

template <typename ComponentT>
struct interface<ComponentT, features::token>: protected detail::component_member<ComponentT>  {
    using component_type = ComponentT;
    using member_type    = detail::component_member<ComponentT>;
    using state_type     = typename ComponentT::state;
    using feature        = features::token;

    using token_type  = typename ComponentT::token_type;
    static_assert(udho::utils::traits::is_ostreamable_v<token_type>);

    inline token_type generate(){ return member_type::component().generate(); }
    inline bool verify(const token_type& token){ return member_type::component().verify(); }

    template <typename Head, typename... Tail>
    state_type eval(const udho::manifold::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return member_type::component().eval(states, address, request);
    }

    using member_type::member_type;
};


/**
 * @brief The component_wrapper class
 */
template <typename ComponentT>
struct wrapper: interface<ComponentT, typename ComponentT::feature>{
    using component_type = ComponentT;
    using feature        = typename ComponentT::feature;
    using interface_type = interface<ComponentT, feature>;
    using state_type     = typename interface_type::state_type;


    static_assert(std::is_class_v<state_type>);

    using interface_type::component;

    wrapper(const wrapper&) = default;

    template <typename ArgX, std::enable_if_t<!std::is_same_v<ArgX, default_constructed>, bool> = true>
    wrapper(ArgX&& arg): interface_type(std::forward<ArgX>(arg)) {}

    template <typename ArgX, std::enable_if_t<std::is_same_v<ArgX, default_constructed>, bool> = true>
    wrapper(ArgX&&): interface_type() {}

    template <typename... Components, typename... Args>
    bool eval(udho::manifold::states<Components...>& states, Args... args) {
        using states_facade_type = udho::manifold::states<Components...>;
        state_type state = std::move(interface_type::eval(states, std::forward<Args>(args)...));
        bool result = state.accepted();
        states.template get<ComponentT>() = std::move(state);
        return result;
    }
};

}
}

#endif // UDHO_MANIFOLD_WRAPPER_H
