#ifndef UDHO_MANIFOLD_STATE_H
#define UDHO_MANIFOLD_STATE_H

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <udho/manifold/features.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/traits.h>
// #include <udho/manifold/wrapper.h>

namespace udho {
namespace manifold {

#ifndef __DOXYGEN__

/**
 * @brief The state_wrapper class
 * @tparam StateT
 * @note expects StateT to be movable (it is default constructibility and copy constructibility is not necessary)
 */
template <typename StateT, typename Feature>
struct state_wrapper{
    static_assert(std::is_move_constructible_v<StateT>);
    static_assert(std::is_move_assignable_v<StateT>);
    static_assert(std::is_copy_constructible_v<StateT>);

    using type      = StateT;
    using feature   = Feature;
    using opt_type  = std::optional<type>;

    state_wrapper(): _state(std::nullopt) {}
    explicit state_wrapper(type&& state): _state(std::move(state)) {}

    state_wrapper(state_wrapper&&) = default;
    state_wrapper& operator=(state_wrapper&&) = default;

    state_wrapper& operator=(type&& state) {
        _state  = std::move(state);
        return *this;
    }

    bool ready() const { return _state.has_value(); }

    const type& value() const {
        if(!ready()) throw std::runtime_error{"trying to get value from unevaluated state"};
        return *_state;
    }

    type& value() {
        if(!ready()) throw std::runtime_error{"trying to get value from unevaluated state"};
        return *_state;
    }

    operator type() const { return value(); }

    const type& operator*() const { return value(); }
    type& operator*() { return value(); }

    const type* operator->() const { return &value(); }
    type* operator->() { return &value(); }

    operator bool() const { return ready(); }

private:
    opt_type _state;
};



namespace detail{

template <typename ComponentT, bool Skip = !udho::manifold::has_state<ComponentT>::value>
struct state_container{
    using component_type = ComponentT;
    using state_type     = state_wrapper<typename ComponentT::state, typename ComponentT::feature>;

    static constexpr const bool skipped = false;

    static_assert(std::is_default_constructible_v<state_type>);
    static_assert(std::is_move_constructible_v<state_type>);

    template <typename... Features>
    friend struct evaluator;

    state_container() = default;
    template <typename OtherHeadT, typename... OtherTail>
    inline explicit state_container(states<OtherHeadT, OtherTail...>&& other): _state(std::move(other.template get<ComponentT>())) {}


    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    state_type& get() { return _state; }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const state_type& get() const { return _state; }
    /// @}


    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    state_type& at() { return _state; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const state_type& at() const { return _state; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT>; }
    /// @}

private:
    state_type _state;
};


template <typename ComponentT>
struct state_container<ComponentT, true>{
    using component_type = ComponentT;
    // using state_type     = void;

    static constexpr const bool skipped = true;

    // template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    // void get() const { }

    // template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    // void at() { }
};


template <typename... Components>
struct temporary_storage{};

template <typename X, typename StorageT>
struct prepend_helper;

template <typename X, typename... Components>
struct prepend_helper<X, temporary_storage<Components...>>{
    using type = temporary_storage<X, Components...>;
};

template <typename... Components>
struct composition_states_helper;

template <typename ComponentT, typename... Rest>
struct composition_states_helper<ComponentT, Rest...> {
    using type = std::conditional_t<
            !has_state<ComponentT>::value,
            typename composition_states_helper<Rest...>::type,
            typename prepend_helper<ComponentT, typename composition_states_helper<Rest...>::type>::type
        >;
};

template <typename ComponentT>
struct composition_states_helper<ComponentT> {
    using type = std::conditional_t<
            !has_state<ComponentT>::value,
            temporary_storage<>,
            temporary_storage<ComponentT>
        >;
};

template <typename ComponentHelperT>
struct get_states_type_helper;

template <typename... Components>
struct get_states_type_helper<temporary_storage<Components...>>{
    using type = states<Components...>;
};

template <typename CompositionT>
struct states_for_composition;

template <typename... Components>
struct states_for_composition<composition<Components...>>{
    using type = typename get_states_type_helper<typename composition_states_helper<Components...>::type>::type;
};

template <typename... Components>
struct states_for_components{
    using type = typename get_states_type_helper<typename composition_states_helper<Components...>::type>::type;
};

}


template <typename ComponentT, typename... Rest>
struct states<ComponentT, Rest...>: private detail::state_container<ComponentT>, private states<Rest...> {
    using component_type = ComponentT;
    using container_type = detail::state_container<ComponentT>;
    // using state_type     = state_wrapper<typename ComponentT::state, typename ComponentT::feature>;

    // static_assert(std::is_default_constructible_v<state_type>);
    // static_assert(std::is_move_constructible_v<state_type>);

    template <typename... Features>
    friend struct evaluator;

    using container_type::container_type;

    /// @{
    template <typename ComponentQ, std::enable_if_t<!container_type::skipped && std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return container_type::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<!container_type::skipped && std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return container_type::template get<ComponentQ>(); }

    // using container_type::get;

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return states<Rest...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return states<Rest...>::template get<ComponentQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!container_type::skipped && std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    auto& at() { return container_type::template get<FeatureT>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!container_type::skipped && std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const auto& at() const { return container_type::template get<FeatureT>(); }

    // using container_type::at;


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return states<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    auto& at() { return states<Rest...>::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return states<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    const auto& at() const { return states<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return container_type::template count<FeatureT>() + states<Rest...>::template count<FeatureT>(); }
    /// @}

private:
    states<Rest...>& tail() { return *this; }
    const states<Rest...>& tail() const { return *this; }
};

template <typename ComponentT>
struct states<ComponentT> : private detail::state_container<ComponentT>{
    using component_type = ComponentT;
    using container_type = detail::state_container<ComponentT>;

    template <typename... Features>
    friend struct evaluator;

    using container_type::container_type;
    using container_type::get;
    using container_type::at;
    using container_type::count;
};

#else


#endif // __DOXYGEN__


}
}


#endif // UDHO_MANIFOLD_STATE_H
