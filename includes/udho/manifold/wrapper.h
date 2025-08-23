#ifndef UDHO_MANIFOLD_WRAPPER_H
#define UDHO_MANIFOLD_WRAPPER_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <boost/asio/ip/address.hpp>
#include <udho/utils/traits.h>
#include <udho/net/common.h>
#include <udho/manifold/traits.h>
#include <udho/manifold/detail.h>
#include <udho/manifold/config.h>

namespace udho {
namespace manifold {

namespace detail{

// template <typename ComponentT, bool ReferencePreferred = component_traits<ComponentT>::shared, bool DefaultConstructible = std::is_default_constructible_v<ComponentT>>
// struct component_storage{
//     using component_type   = ComponentT;

//     static constexpr component_type* dummy = 0x0;

//     static constexpr bool const is_default_constructible = false;
//     static constexpr bool const is_move_constructible    = false;

//     inline explicit component_storage(): _component(*dummy) {
//         static_assert(is_default_constructible, "Expecting lvalue reference for ComponentT because component_traits<ComponentT>::shared is true, but no feasible argument was found");
//     }

//     inline explicit component_storage(component_type& component_ref): _component(component_ref) {}
//     inline explicit component_storage(default_constructed&&): _component(*dummy) {
//         static_assert(false, "Expecting lvalue reference for ComponentT because component_traits<ComponentT>::shared is true, but no feasible argument was found");
//     }

//     component_type& component() { return _component; }
//     const component_type& component() const { return _component; }

// private:
//     component_type& _component;
// };

// template <typename ComponentT>
// struct component_storage<ComponentT, false, true>{
//     using component_type   = ComponentT;

//     static constexpr bool const is_default_constructible = true;
//     static constexpr bool const is_move_constructible    = true;

//     template <typename Arg, std::enable_if_t<detail::argument_traits<ComponentT>::template should_move<Arg>, bool> = true>
//     inline explicit component_storage(Arg component_rval): _component(std::move(component_rval)) {}

//     inline explicit component_storage(): _component() {}
//     inline explicit component_storage(default_constructed&&): _component() {}

//     component_type& component() { return _component; }
//     const component_type& component() const { return _component; }

// private:

//     component_type _component;
// };

// template <typename ComponentT>
// struct component_storage<ComponentT, false, false>{
//     using component_type   = ComponentT;

//     static constexpr bool const is_default_constructible = false;
//     static constexpr bool const is_move_constructible    = true;

//     template <typename Arg, std::enable_if_t<detail::argument_traits<ComponentT>::template should_move<Arg>, bool> = true>
//     inline explicit component_storage(Arg component_rval): _component(std::move(component_rval)) {}

//     inline explicit component_storage() { static_assert(is_default_constructible, "No argument supplied for non-default constructible Component"); }

//     component_type& component() { return _component; }
//     const component_type& component() const { return _component; }

// private:

//     component_type _component;
// };


template <typename T>
class basic_hybrid_storage{
protected:
    using value_type     = T;
    using variant_type   = std::variant<std::reference_wrapper<value_type>, value_type>;
public:
    using reference_type = std::add_lvalue_reference_t<value_type>;
    using const_reference_type = std::add_const_t<reference_type>;
private:
    variant_type _variant;
protected:
    basic_hybrid_storage(variant_type&& variant): _variant(std::move(variant)) {}
public:
    reference_type component() {
        if (_variant.index() == 0) return std::get<0>(_variant).get();
        else return std::get<1>(_variant);
    }

    const_reference_type component() const {
        if (_variant.index() == 0) return std::get<0>(_variant).get();
        else return std::get<1>(_variant);
    }

    bool owned() const { return _variant.index() == 1; }
    bool borrowed() const { return !owned(); }

    reference_type operator*() { return component(); }
    const_reference_type operator*() const { return component(); }
};

template <typename T, bool MoveConstructible = std::is_move_constructible_v<T>, bool DefaultConstructible = std::is_default_constructible_v<T>>
struct hybrid_storage: basic_hybrid_storage<T>{
    using basic_hybrid_storage_type = basic_hybrid_storage<T>;
    using variant_type   = typename basic_hybrid_storage_type::variant_type;

    explicit hybrid_storage(T& value): basic_hybrid_storage_type(variant_type{std::ref(value)}) {}
    explicit hybrid_storage(T&& value): basic_hybrid_storage_type(variant_type{std::move(value)}) {}
    explicit hybrid_storage(): basic_hybrid_storage_type(variant_type{T{}}) {}
    explicit hybrid_storage(default_constructed&&): basic_hybrid_storage_type(variant_type{T{}}) {}
};

template <typename T>
struct hybrid_storage<T, true, false>: basic_hybrid_storage<T>{
    using basic_hybrid_storage_type = basic_hybrid_storage<T>;
    using variant_type   = typename basic_hybrid_storage_type::variant_type;

    explicit hybrid_storage(T& value): basic_hybrid_storage_type(variant_type{std::ref(value)}) {}
    explicit hybrid_storage(T&& value): basic_hybrid_storage_type(variant_type{std::move(value)}) {}
    explicit hybrid_storage(default_constructed&&): basic_hybrid_storage_type(variant_type{*dummy}) {
        static_assert(false, "Default construction not available for CompopnentT, expecting lvalue or rvalue reference");
    }
    explicit hybrid_storage(): basic_hybrid_storage_type(variant_type{*dummy}) {
        static_assert(false, "Default construction not available for CompopnentT, expecting lvalue or rvalue reference");
    }

private:
    static constexpr typename basic_hybrid_storage_type::value_type* dummy = 0x0;
};

template <typename T>
struct hybrid_storage<T, false, true>: basic_hybrid_storage<T>{
    using basic_hybrid_storage_type = basic_hybrid_storage<T>;
    using variant_type   = typename basic_hybrid_storage_type::variant_type;

    explicit hybrid_storage(T& value): basic_hybrid_storage_type(variant_type{std::ref(value)}) {}
    explicit hybrid_storage(): basic_hybrid_storage_type(variant_type{T{}}) {}
    explicit hybrid_storage(default_constructed&&): basic_hybrid_storage_type(variant_type{T{}}) {}

    explicit hybrid_storage(T&&): basic_hybrid_storage_type(variant_type{T{}}) {
        static_assert(false, "Move construction not available for CompopnentT, expected to be default constructed or use lvalue reference");
    }
};

template <typename T>
struct hybrid_storage<T, false, false>: basic_hybrid_storage<T>{
    using basic_hybrid_storage_type = basic_hybrid_storage<T>;
    using variant_type   = typename basic_hybrid_storage_type::variant_type;

    explicit hybrid_storage(T& value): basic_hybrid_storage_type(variant_type{std::ref(value)}) {}

    explicit hybrid_storage(default_constructed&&): basic_hybrid_storage_type(variant_type{*dummy}) {
        static_assert(false, "Default construction not available for CompopnentT, expecting lvalue or rvalue reference");
    }
    explicit hybrid_storage(): basic_hybrid_storage_type(variant_type{*dummy}) {
        static_assert(false, "Default construction not available for CompopnentT, expecting lvalue or rvalue reference");
    }
    explicit hybrid_storage(T&&): basic_hybrid_storage_type(variant_type{*dummy}) {
        static_assert(false, "Move construction not available for CompopnentT, expected to be constructed using lvalue reference");
    }

private:
    static constexpr typename basic_hybrid_storage_type::value_type* dummy = 0x0;
};

}

/**
 * @brief The component_wrapper class
 */
template <typename ComponentT>
struct wrapper: detail::hybrid_storage<ComponentT>{
    using component_type = ComponentT;
    using storage_type   = detail::hybrid_storage<ComponentT>;
    using config_type    = udho::manifold::config<ComponentT>;

    static constexpr const bool has_result = udho::manifold::has_result<ComponentT>::vlue;

    using storage_type::storage_type;

    const config_type& config() const { return _config; }

    config_type& config() { return _config; }

    private:
    config_type _config;
};

}
}

#endif // UDHO_MANIFOLD_WRAPPER_H
