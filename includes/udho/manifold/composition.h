#ifndef UDHO_MANIFOLD_COMPOSITION_H
#define UDHO_MANIFOLD_COMPOSITION_H

#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/wrapper.h>
#include <udho/manifold/detail.h>
#include <udho/manifold/utils.h>

namespace udho{
namespace manifold{

template <typename... Components>
struct compositor;

namespace detail{

template <std::size_t Index, bool Status>
struct feasible_for_atleast_one_component{
    static constexpr const bool value = Status;
};

template <typename...>
struct feasible_for;

template <typename ArgT>
struct feasible_for<ArgT>{
    template <std::size_t Index, typename... Components>
    static constexpr void assert_msg(ArgT&& arg) {
        constexpr const bool count = udho::utils::traits::accumulate<
                std::integral_constant<bool, (detail::argument_traits<Components>::template is_feasible<ArgT>) >...
            >::value == 1;
        static_assert(feasible_for_atleast_one_component<Index, count>::value, "constraint feasible_for_atleast_one_component<Index, true> must be satisfied for all arguments");
    }
};

template <typename ArgT, typename... Args>
struct feasible_for<ArgT, Args...>: feasible_for<Args...>{
    template <std::size_t Index, typename... Components>
    static constexpr void assert_msg(ArgT&& arg, Args&&... args) {
        constexpr const bool count = udho::utils::traits::accumulate<
                std::integral_constant<bool, (detail::argument_traits<Components>::template is_feasible<ArgT>) >...
            >::value == 1;

        static_assert(feasible_for_atleast_one_component<Index, count>::value, "constraint feasible_for_atleast_one_component<Index, true> must be satisfied for all arguments");
        feasible_for<Args...>::template assert_msg<Index+1, Components...>(std::forward<Args>(args)...);
    }
};

}

template <typename... Components>
struct compositor {
    using composition_type = composition<Components...>;

    /**
     * @brief creates a composition with inputs for subsets of Components while assuming that the left outs will use default_constructed
     * @param args
     * @return
     */
    template <typename ArgT, typename... Args>
    static composition_type compose(ArgT&& arg, Args&&... args) {
        detail::feasible_for<ArgT, Args...>::template assert_msg<0, Components...>(std::forward<ArgT>(arg), std::forward<Args>(args)...);
        return composition_type{detail::arguments<ArgT, Args...>::template find<Components>(std::forward<ArgT>(arg), std::forward<Args>(args)...)...};
    }

    static composition_type compose() {
        return composition_type{std::enable_if_t<!std::is_void_v<Components>, default_constructed>{}...};
    }
};



template <typename ComponentT, typename... Rest>
struct composition<ComponentT, Rest...>: private wrapper<ComponentT>, private composition<Rest...>{
    using component_type = ComponentT;
    using features_type  = typename ComponentT::features;
    using wrapper_type   = wrapper<component_type>;
    using fabric_type  = typename udho::manifold::detail::flatten_all<ComponentT, Rest...>::type;
    using pipeline_type  = pipeline<ComponentT, Rest...>;

    template <typename... Features>
    friend struct evaluator;

    template <typename... Components>
    friend struct fabric;

    template <typename... Args>
    static composition<ComponentT, Rest...> compose(Args&&... args) { return compositor<ComponentT, Rest...>::compose(std::forward<Args>(args)...); }

    template <typename ArgT, typename... Args>
    composition(ArgT&& arg, Args&&... args): wrapper_type(std::forward<ArgT>(arg)), composition<Rest...>(std::forward<Args>(args)...) {
        static constexpr const std::size_t arguments_provided = sizeof...(Args);
        static constexpr const std::size_t expected_arguments = sizeof...(Rest);
        static_assert(arguments_provided == expected_arguments, "insufficient number of arguments passed to the composition constructor");
    }

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition<Rest...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition<Rest...>::template get<ComponentQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx != 0, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!features_type::template has<FeatureT>::value, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx != 0, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!features_type::template has<FeatureT>::value, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return features_type::template has<FeatureT>::value + composition<Rest...>::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<composition<ComponentT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<composition<ComponentT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

private:
    composition<Rest...>& tail() { return *this; }

    const composition<Rest...>& tail() const { return *this; }
};

template <typename ComponentT>
struct composition<ComponentT>: private wrapper<ComponentT> {
    using component_type = ComponentT;
    using features_type  = typename ComponentT::features;
    using wrapper_type   = wrapper<component_type>;
    using fabric_type = typename udho::manifold::detail::flatten_all<ComponentT>::type;
    using pipeline_type  = pipeline<ComponentT>;

    template <typename... Features>
    friend struct evaluator;

    template <typename... Components>
    friend struct fabric;

    template <typename... Args>
    static composition<ComponentT> compose(Args&&... args) { return compositor<ComponentT>::compose(std::forward<Args>(args)...); }

    template <typename ArgT>
    composition(ArgT&& arg): wrapper_type(std::forward<ArgT>(arg)) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return features_type::template has<FeatureT>::value; }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<composition<ComponentT>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<composition<ComponentT>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}
};

}
}

#endif // UDHO_MANIFOLD_COMPOSITION_H
