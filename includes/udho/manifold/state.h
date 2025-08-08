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
 * @tparam ResultT
 * @note expects ResultT to be movable (it is default constructibility and copy constructibility is not necessary)
 */
template <typename ResultT, typename Feature>
struct result_wrapper{
    static_assert(std::is_move_constructible_v<ResultT>);
    static_assert(std::is_move_assignable_v<ResultT>);
    static_assert(std::is_copy_constructible_v<ResultT>);

    using type      = ResultT;
    using feature   = Feature;
    using opt_type  = std::optional<type>;

    result_wrapper(): _result(std::nullopt) {}
    explicit result_wrapper(type&& result): _result(std::move(result)) {}

    result_wrapper(result_wrapper&&) = default;
    result_wrapper& operator=(result_wrapper&&) = default;

    result_wrapper& operator=(type&& result) {
        _result  = std::move(result);
        return *this;
    }

    bool ready() const { return _result.has_value(); }

    const type& value() const {
        if(!ready()) throw std::runtime_error{"trying to get result from unevaluated delegate"};
        return *_result;
    }

    type& value() {
        if(!ready()) throw std::runtime_error{"trying to get result from unevaluated delegate"};
        return *_result;
    }

    operator type() const { return value(); }

    const type& operator*() const { return value(); }
    type& operator*() { return value(); }

    const type* operator->() const { return &value(); }
    type* operator->() { return &value(); }

    operator bool() const { return ready(); }

private:
    opt_type _result;
};



namespace detail{

template <typename DelegateT, bool Skip = !udho::manifold::has_result<DelegateT>::value>
struct result_container{
    using delegate_type  = DelegateT;
    using result_type    = typename udho::manifold::delegate_traits<DelegateT>::result_type;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using wrapper_type   = result_wrapper<result_type, feature_type>;

    static constexpr const bool skipped = false;

    static_assert(std::is_default_constructible_v<wrapper_type>);
    static_assert(std::is_move_constructible_v<wrapper_type>);

    template <typename... Features>
    friend struct evaluator;

    result_container() = default;
    template <typename OtherHeadT, typename... OtherTail>
    inline explicit result_container(states<OtherHeadT, OtherTail...>&& other): _result(std::move(other.template get<DelegateT>())) {}


    /// @{
    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    wrapper_type& get() { return _result; }

    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const wrapper_type& get() const { return _result; }
    /// @}


    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return _result; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return _result; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature_type, FeatureT>; }
    /// @}

private:
    wrapper_type _result;
};


template <typename DelegateT>
struct result_container<DelegateT, true>{
    using delegate_type  = DelegateT;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    static constexpr const bool skipped = true;
};


template <typename... Delegates>
struct temporary_storage{};

template <typename X, typename StorageT>
struct prepend_helper;

template <typename X, typename... Delegates>
struct prepend_helper<X, temporary_storage<Delegates...>>{
    using type = temporary_storage<X, Delegates...>;
};

template <typename... Delegates>
struct composition_states_helper;

template <typename DelegateT, typename... Rest>
struct composition_states_helper<DelegateT, Rest...> {
    using type = std::conditional_t<
            !has_result<DelegateT>::value,
            typename composition_states_helper<Rest...>::type,
            typename prepend_helper<DelegateT, typename composition_states_helper<Rest...>::type>::type
        >;
};

template <typename DelegateT>
struct composition_states_helper<DelegateT> {
    using type = std::conditional_t<
            !has_result<DelegateT>::value,
            temporary_storage<>,
            temporary_storage<DelegateT>
        >;
};

template <typename>
struct get_states_type_helper;

template <typename... Delegates>
struct get_states_type_helper<temporary_storage<Delegates...>>{
    using type = states<Delegates...>;
};

template <typename DelegatesT>
struct states_for_mediator;

template <typename... Delegates>
struct states_for_mediator<mediator<Delegates...>>{
    using type = typename get_states_type_helper<typename composition_states_helper<Delegates...>::type>::type;
};

template <typename... Delegates>
struct states_for_delegates{
    using type = typename get_states_type_helper<typename composition_states_helper<Delegates...>::type>::type;
};

}


template <typename DelegateT, typename... Rest>
struct states<DelegateT, Rest...>: private detail::result_container<DelegateT>, private states<Rest...> {
    using component_type = DelegateT;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using container_type = detail::result_container<DelegateT>;

    // static_assert(std::is_default_constructible_v<state_type>);
    // static_assert(std::is_move_constructible_v<state_type>);

    template <typename... Features>
    friend struct evaluator;

    using container_type::container_type;

    /// @{
    template <typename DelegateQ, std::enable_if_t<!container_type::skipped && std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    auto& get() { return container_type::template get<DelegateQ>(); }

    template <typename DelegateQ, std::enable_if_t<!container_type::skipped && std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const auto& get() const { return container_type::template get<DelegateQ>(); }

    // using container_type::get;

    template <typename DelegateQ, std::enable_if_t<!std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    auto& get() { return states<Rest...>::template get<DelegateQ>(); }

    template <typename DelegateQ, std::enable_if_t<!std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const auto& get() const { return states<Rest...>::template get<DelegateQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    auto& at() { return container_type::template get<FeatureT>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const auto& at() const { return container_type::template get<FeatureT>(); }

    // using container_type::at;


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return states<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return states<Rest...>::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return states<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
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

template <typename DelegateT>
struct states<DelegateT> : private detail::result_container<DelegateT>{
    using component_type = DelegateT;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using container_type = detail::result_container<DelegateT>;

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
