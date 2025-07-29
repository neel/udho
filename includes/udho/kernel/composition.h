#ifndef UDHO_KERNEL_COMPOSITION_H
#define UDHO_KERNEL_COMPOSITION_H

#include <type_traits>
#include <udho/kernel/fwd.h>
#include <udho/kernel/features.h>
#include <udho/kernel/wrapper.h>
#include <udho/kernel/detail.h>

namespace udho{
namespace kernel{

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
        constexpr const bool count = detail::accumulate<
                std::integral_constant<bool, (detail::argument_traits<Components>::template is_feasible<ArgT>) >...
            >::value == 1;
        static_assert(feasible_for_atleast_one_component<Index, count>::value, "constraint feasible_for_atleast_one_component<Index, true> must be satisfied for all arguments");
    }
};

template <typename ArgT, typename... Args>
struct feasible_for<ArgT, Args...>: feasible_for<Args...>{
    template <std::size_t Index, typename... Components>
    static constexpr void assert_msg(ArgT&& arg, Args&&... args) {
        constexpr const bool count = detail::accumulate<
                std::integral_constant<bool, (detail::argument_traits<Components>::template is_feasible<ArgT>) >...
            >::value == 1;

        static_assert(feasible_for_atleast_one_component<Index, count>::value, "constraint feasible_for_atleast_one_component<Index, true> must be satisfied for all arguments");
        feasible_for<Args...>::template assert_msg<Index+1, Components...>(std::forward<Args>(args)...);
    }
};


template <typename CompositionT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< CompositionT::template count<FeatureT>() == Idx , bool> = true>
std::size_t composition_apply(CompositionT& composition, Function&& function) { return 0; }

template <typename CompositionT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< CompositionT::template count<FeatureT>() != Idx , bool> = true>
std::size_t composition_apply(CompositionT& composition, Function&& function) {
    bool result = function(composition.template at<FeatureT, Idx>());
    std::size_t count = result;
    if(result) {
        count += composition_apply<CompositionT, FeatureT, Function, Idx+1>(composition, std::forward<Function>(function));
    }
    return count;
}



template <typename CompositionT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< CompositionT::template count<FeatureT>() == Idx , bool> = true>
std::size_t composition_apply(const CompositionT& composition, Function&& function) { return 0; }

template <typename CompositionT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< CompositionT::template count<FeatureT>() != Idx , bool> = true>
std::size_t composition_apply(const CompositionT& composition, Function&& function) {
    bool result = function(composition.template at<FeatureT, Idx>());
    std::size_t count = result;
    if(result) {
        count += composition_apply<CompositionT, FeatureT, Function, Idx+1>(composition, std::forward<Function>(function));
    }
    return count;
}

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
    using wrapper_type   = wrapper<component_type>;

    template <typename... Features>
    friend struct evaluator;

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
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT> + composition<Rest...>::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return detail::composition_apply<composition<ComponentT, Rest...>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return detail::composition_apply<composition<ComponentT, Rest...>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }
    /// @}

private:
    composition<Rest...>& tail() { return *this; }

    const composition<Rest...>& tail() const { return *this; }
};

template <typename ComponentT>
struct composition<ComponentT>: private wrapper<ComponentT> {
    using component_type = ComponentT;
    using wrapper_type   = wrapper<component_type>;

    template <typename... Features>
    friend struct evaluator;

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
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT>; }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return detail::composition_apply<composition<ComponentT>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return detail::composition_apply<composition<ComponentT>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }
    /// @}
};

}
}

#endif // UDHO_KERNEL_COMPOSITION_H
