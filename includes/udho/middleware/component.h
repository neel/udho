#ifndef UDHO_MIDDLEWARE_COMPONENT_H
#define UDHO_MIDDLEWARE_COMPONENT_H

#include <udho/middleware/fwd.h>
#include <udho/middleware/features.h>
#include <boost/asio/ip/address.hpp>
#include <udho/utils/traits.h>
#include <udho/net/common.h>

namespace udho {
namespace middleware {

template <typename ComponentT, typename FeatureT>
struct interface;

/**
 * @brief The component_traits class
 * @details prefer_reference determines whether the component ComponentT will be stored using reference or not
 *          by default componentts that are either not movable or not default constructible will be stored as a reference.
 *          usercode must manage lifetime of these components and provide a reference to them in the facade constructor.
 *
 * @note specialize component_traits<ComponentX> for any ComponentX to override the default settings
 */
template <typename ComponentT>
struct component_traits{
    static constexpr const bool prefer_reference = !std::is_move_constructible_v<ComponentT> || !std::is_default_constructible_v<ComponentT>;
};

namespace detail{


/**
 * @brief helper class to check wheather an argument ArgT is feasible initialization argument for the component ComponentT
 * @tparam ComponentT The component type
 */
template <typename ComponentT>
struct argument {
    template <typename ArgT>
    static constexpr const bool feasible_lvalue_reference =  std::is_lvalue_reference_v<ArgT> &&
                                                             std::is_same_v<std::remove_reference_t<ArgT>, ComponentT>;

    template <typename ArgT>
    static constexpr const bool feasible_rvalue_reference =  std::is_same_v<std::remove_reference_t<ArgT>, ComponentT>;
    template <typename ArgT>
    static constexpr const bool should_move = feasible_rvalue_reference<ArgT>;


    template <typename ArgT>
    static constexpr const bool is_feasible = (component_traits<ComponentT>::prefer_reference && feasible_lvalue_reference<ArgT>) ||
                                              (!component_traits<ComponentT>::prefer_reference && should_move<ArgT>);
};

/**
 * @brief helper class to select an argument feasible as initializer for the component ComponentT
 */
template <typename ArgT, typename... Args>
struct arguments{
    template <typename ComponentT>
    using first_feasible_arg_type = std::conditional_t<argument<ComponentT>::template is_feasible<ArgT>, ArgT, typename arguments<Args...>::template first_feasible_arg_type<Args...> >;

    template <std::size_t Idx, typename ComponentT, std::enable_if_t<argument<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr ArgT first_feasible_arg(ArgT&& arg, Args&&... args) { return std::forward<ArgT>(arg); }

    template <std::size_t Idx, typename ComponentT, std::enable_if_t<!argument<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr first_feasible_arg_type<ComponentT> first_feasible_arg(ArgT&& arg, Args&&... args) { return std::forward<first_feasible_arg_type<ComponentT>>( arguments<Args...>::template first_feasible_arg<Idx+1, ComponentT>(std::forward<Args>(args)...) ); }
};

template <typename ArgT>
struct arguments<ArgT>{
    template <typename ComponentT>
    using first_feasible_arg_type = std::enable_if_t<argument<ComponentT>::template is_feasible<ArgT>, ArgT>;

    template <std::size_t Idx, typename ComponentT, std::enable_if_t<argument<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr ArgT first_feasible_arg(ArgT&& arg) { return std::forward<ArgT>(arg); }

    template <std::size_t Idx, typename ComponentT, std::enable_if_t<!argument<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr std::false_type first_feasible_arg(ArgT&& arg) { return std::false_type{}; }
};

template <typename ComponentT, bool ReferencePreferred = component_traits<ComponentT>::prefer_reference, bool DefaultConstructible = std::is_default_constructible_v<ComponentT>>
struct component_member{
    using component_type   = ComponentT;

    static constexpr component_type* dummy = 0x0;

    static constexpr bool const is_default_constructible = false;
    static constexpr bool const is_move_constructible    = false;

    inline explicit component_member(component_type& component_ref): _component(component_ref) {}
    inline explicit component_member(std::false_type&&): _component(*dummy) {
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

    template <typename Arg, std::enable_if_t<detail::argument<ComponentT>::template should_move<Arg>, bool> = true>
    inline explicit component_member(Arg component_rval): _component(std::move(component_rval)) {}

    inline explicit component_member(): _component() {}
    inline explicit component_member(std::false_type&&): _component() {}

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

    template <typename Arg, std::enable_if_t<detail::argument<ComponentT>::template should_move<Arg>, bool> = true>
    inline explicit component_member(Arg component_rval): _component(std::move(component_rval)) {}

    inline explicit component_member() = delete;

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
    state_type eval(const udho::middleware::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
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
    state_type eval(const udho::middleware::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
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
    state_type eval(const udho::middleware::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return member_type::component().eval(states, address, request);
    }

    using member_type::member_type;
};



/**
 * @brief The component_wrapper class
 */
template <typename ComponentT>
struct component_wrapper: interface<ComponentT, typename ComponentT::feature>{
    using component_type = ComponentT;
    using feature        = typename ComponentT::feature;
    using interface_type = interface<ComponentT, feature>;
    using state_type     = typename interface_type::state_type;


    static_assert(std::is_class_v<state_type>);

    using interface_type::component;

    component_wrapper(const component_wrapper&) = default;
    template <typename ArgX, typename... Args>
    component_wrapper(ArgX&& arg, Args&&... args): interface_type(detail::arguments<ArgX, Args...>::template first_feasible_arg<0, ComponentT>(std::forward<ArgX>(arg), std::forward<Args>(args)...)) {}

    template <typename Head, typename... Tail, typename... Args>
    bool eval(udho::middleware::states<Head, Tail...>& states, Args... args) {
        using states_facade_type = udho::middleware::states<Head, Tail...>;
        state_type state = std::move(interface_type::eval(states, std::forward<Args>(args)...));
        bool result = state.accepted();
        states.template get<ComponentT>() = std::move(state);
        return result;
    }
};

}
}

#endif // UDHO_MIDDLEWARE_COMPONENT_H
