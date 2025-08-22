#ifndef UDHO_MANIFOLD_JOURNAL_H
#define UDHO_MANIFOLD_JOURNAL_H

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
 * @brief The journal_wrapper class
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
        if(!ready()) throw std::runtime_error{"trying to get result from unevaluated facet"};
        return *_result;
    }

    type& value() {
        if(!ready()) throw std::runtime_error{"trying to get result from unevaluated facet"};
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

template <typename FacetT, bool Skip = !udho::manifold::has_result<FacetT>::value>
struct result_container{
    using facet_type  = FacetT;
    using result_type    = typename udho::manifold::facet_traits<FacetT>::result_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using wrapper_type   = result_wrapper<result_type, feature_type>;

    static constexpr const bool skipped = false;

    static_assert(std::is_default_constructible_v<wrapper_type>);
    static_assert(std::is_move_constructible_v<wrapper_type>);

    template <typename... Features>
    friend struct evaluator;

    result_container() = default;
    template <typename OtherHeadT, typename... OtherTail>
    inline explicit result_container(journal<OtherHeadT, OtherTail...>&& other): _result(std::move(other.template get<FacetT>())) {}


    /// @{
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    wrapper_type& get() { return _result; }

    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
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


template <typename FacetT>
struct result_container<FacetT, true>{
    using facet_type  = FacetT;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    static constexpr const bool skipped = true;
};


template <typename... Facets>
struct temporary_storage{};

template <typename X, typename StorageT>
struct prepend_helper;

template <typename X, typename... Facets>
struct prepend_helper<X, temporary_storage<Facets...>>{
    using type = temporary_storage<X, Facets...>;
};

template <typename... Facets>
struct composition_journal_helper;

template <typename FacetT, typename... Rest>
struct composition_journal_helper<FacetT, Rest...> {
    using type = std::conditional_t<
            !has_result<FacetT>::value,
            typename composition_journal_helper<Rest...>::type,
            typename prepend_helper<FacetT, typename composition_journal_helper<Rest...>::type>::type
        >;
};

template <typename FacetT>
struct composition_journal_helper<FacetT> {
    using type = std::conditional_t<
            !has_result<FacetT>::value,
            temporary_storage<>,
            temporary_storage<FacetT>
        >;
};

template <typename>
struct get_journal_type_helper;

template <typename... Facets>
struct get_journal_type_helper<temporary_storage<Facets...>>{
    using type = journal<Facets...>;
};

template <typename FacetsT>
struct journal_for_fabric;

template <std::size_t Stage, typename... Facets>
struct journal_for_fabric<fabric<Stage, Facets...>>{
    using type = typename get_journal_type_helper<typename composition_journal_helper<Facets...>::type>::type;
};

template <typename... Facets>
struct journal_for_facets{
    using type = typename get_journal_type_helper<typename composition_journal_helper<Facets...>::type>::type;
};

}


template <typename FacetT, typename... Rest>
struct journal<FacetT, Rest...>: private detail::result_container<FacetT>, private journal<Rest...> {
    using component_type = FacetT;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using container_type = detail::result_container<FacetT>;

    // static_assert(std::is_default_constructible_v<journal_type>);
    // static_assert(std::is_move_constructible_v<journal_type>);

    // template <typename... Features>
    // friend struct evaluator;

    using container_type::container_type;

    /// @{
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return container_type::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return container_type::template get<FacetQ>(); }

    // using container_type::get;

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return journal<Rest...>::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return journal<Rest...>::template get<FacetQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    auto& at() { return container_type::template get<FeatureT>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const auto& at() const { return container_type::template get<FeatureT>(); }

    // using container_type::at;


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return journal<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return journal<Rest...>::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return journal<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    const auto& at() const { return journal<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return container_type::template count<FeatureT>() + journal<Rest...>::template count<FeatureT>(); }
    /// @}

private:
    journal<Rest...>& tail() { return *this; }
    const journal<Rest...>& tail() const { return *this; }
};

template <typename FacetT>
struct journal<FacetT> : private detail::result_container<FacetT>{
    using component_type = FacetT;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using container_type = detail::result_container<FacetT>;

    // template <typename... Features>
    // friend struct evaluator;

    using container_type::container_type;
    using container_type::get;
    using container_type::at;
    using container_type::count;
};

#else


#endif // __DOXYGEN__


}
}


#endif // UDHO_MANIFOLD_JOURNAL_H
