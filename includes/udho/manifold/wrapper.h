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

namespace detail{

template <typename ComponentT, bool ReferencePreferred = component_traits<ComponentT>::shared, bool DefaultConstructible = std::is_default_constructible_v<ComponentT>>
struct component_storage{
    using component_type   = ComponentT;

    static constexpr component_type* dummy = 0x0;

    static constexpr bool const is_default_constructible = false;
    static constexpr bool const is_move_constructible    = false;

    inline explicit component_storage(): _component(*dummy) {
        static_assert(is_default_constructible, "Expecting lvalue reference for ComponentT because component_traits<ComponentT>::shared is true, but no feasible argument was found");
    }

    inline explicit component_storage(component_type& component_ref): _component(component_ref) {}
    inline explicit component_storage(default_constructed&&): _component(*dummy) {
        static_assert(false, "Expecting lvalue reference for ComponentT because component_traits<ComponentT>::shared is true, but no feasible argument was found");
    }

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

private:
    component_type& _component;
};

template <typename ComponentT>
struct component_storage<ComponentT, false, true>{
    using component_type   = ComponentT;

    static constexpr bool const is_default_constructible = true;
    static constexpr bool const is_move_constructible    = true;

    template <typename Arg, std::enable_if_t<detail::argument_traits<ComponentT>::template should_move<Arg>, bool> = true>
    inline explicit component_storage(Arg component_rval): _component(std::move(component_rval)) {}

    inline explicit component_storage(): _component() {}
    inline explicit component_storage(default_constructed&&): _component() {}

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

private:

    component_type _component;
};

template <typename ComponentT>
struct component_storage<ComponentT, false, false>{
    using component_type   = ComponentT;

    static constexpr bool const is_default_constructible = false;
    static constexpr bool const is_move_constructible    = true;

    template <typename Arg, std::enable_if_t<detail::argument_traits<ComponentT>::template should_move<Arg>, bool> = true>
    inline explicit component_storage(Arg component_rval): _component(std::move(component_rval)) {}

    inline explicit component_storage() {
        static_assert(is_default_constructible, "No argument supplied for non-default constructible Component");
    }

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

private:

    component_type _component;
};

}

/**
 * @brief The component_wrapper class
 */
template <typename ComponentT>
struct wrapper: detail::component_storage<ComponentT>{
    using component_type = ComponentT;
    using feature        = typename ComponentT::feature;
    using storage_type   = detail::component_storage<ComponentT>;
    using delegate_type  = delegate<ComponentT>;
    using config_type    = config<ComponentT>;

    static constexpr const bool has_state = udho::manifold::has_state<ComponentT>::vlue;

    using storage_type::storage_type;
};

}
}

#endif // UDHO_MANIFOLD_WRAPPER_H
