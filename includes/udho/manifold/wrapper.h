#ifndef UDHO_MANIFOLD_WRAPPER_H
#define UDHO_MANIFOLD_WRAPPER_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <boost/asio/ip/address.hpp>
#include <udho/utils/traits.h>
#include <udho/net/common.h>
#include <udho/manifold/traits.h>
#include <udho/manifold/detail.h>
// #include <udho/manifold/config.h>

namespace udho {
namespace manifold {

#ifndef __DOXYGEN__

namespace detail{

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

    static constexpr const bool has_result = udho::manifold::has_result<ComponentT>::value;

    using storage_type::storage_type;
};

#else

/**
 * @brief Wraps a component instance.
 *
 * This wrapper provides a uniform way to carry a component instance.
 *
 * The wrapper can operate in **two modes**:
 * - **Borrowing mode:** the wrapper references an external component object.
 * - **Owning mode:** the wrapper stores its own component object.
 *
 * ### Ownership / lifetime rules
 *
 * | Constructor                                   | Ownership  | Requirement                                | 
 * |:----------------------------------------------|:-----------|:-------------------------------------------|
 * | `wrapper(ComponentT& c)`                      | Borrowing  | `c` must be a valid lvalue                 |
 * | `wrapper(ComponentT&& c)`                     | Owning     | `ComponentT` must be movable               |
 * | `wrapper()` / `wrapper(default_constructed{})`| Owning     | `ComponentT` must be default-constructible |
 *
 * In borrowing mode, the wrapper never takes ownership. In owning mode, the wrapper’s
 * component value is independent of the original argument (moved-in or default-constructed).
 *
 * ### Accessing the component
 *
 * The wrapper behaves like a handle to the component object:
 * - use dereference (`*w`) or the inherited accessors to reach the component
 *
 * @tparam ComponentT Component type being wrapped.
 *
 * @warning If you construct from `ComponentT&`, you must ensure the referenced component outlives the wrapper (no dangling references).
 */
template <typename ComponentT>
struct wrapper: detail::hybrid_storage<ComponentT>{
    using component_type = ComponentT;
    using storage_type   = detail::hybrid_storage<ComponentT>;
    using config_type    = udho::manifold::config<ComponentT>;
    using reference_type = std::add_lvalue_reference_t<ComponentT>;
    using const_reference_type = std::add_const_t<reference_type>;

    static constexpr const bool has_result = udho::manifold::has_result<ComponentT>::value;

    /// @brief Borrow an external component. The referenced component must outlive this wrapper.
    explicit wrapper(ComponentT& c);

    /// @brief Own a component by moving it into the wrapper.
    explicit wrapper(ComponentT&& c);

    /// @brief Default-construct an owned component (only if ComponentT is default-constructible).
    explicit wrapper();

    /// @brief Tag-based default construction for an owned component.
    explicit wrapper(default_constructed);

    /// @brief Get a mutable reference to the wrapped component (borrowed or owned).
    reference_type component();

    /// @brief Get a const reference to the wrapped component (borrowed or owned).
    const_reference_type component() const;

    /// @brief True if the wrapper owns its component (stored internally).
    bool owned() const;

    /// @brief True if the wrapper borrows its component (refers to external storage).
    bool borrowed() const;

    /// @brief Dereference convenience; equivalent to `component()`.
    reference_type operator*() { return component(); }

    /// @brief Const dereference convenience; equivalent to `component() const`.
    const_reference_type operator*() const { return component(); }

};
#endif // __DOXYGEN__

}
}

#endif // UDHO_MANIFOLD_WRAPPER_H
