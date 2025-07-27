#ifndef UDHO_MIDDLEWARE_COMPOSITION_H
#define UDHO_MIDDLEWARE_COMPOSITION_H

#include <type_traits>
#include <udho/middleware/fwd.h>
#include <udho/middleware/features.h>
#include <udho/middleware/wrapper.h>
#include <udho/middleware/detail.h>

namespace udho{
namespace middleware{

template <typename... Components>
struct composition;

template <typename... Components>
struct compositor;

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

    template <typename... Args>
    static composition<ComponentT, Rest...> compose(Args&&... args) { return compositor<ComponentT, Rest...>::compose(std::forward<Args>(args)...); }

    template <typename ArgT, typename... Args>
    composition(ArgT&& arg, Args&&... args): wrapper_type(std::forward<ArgT>(arg)), composition<Rest...>(std::forward<Args>(args)...) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    component_type& get() { return wrapper_type::component(); }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition<Rest...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const component_type& get() const { return wrapper_type::component(); }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition<Rest...>::template get<ComponentQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    component_type& at() { return wrapper_type::component(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const component_type& at() const { return wrapper_type::component(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT> + composition<Rest...>::template count<FeatureT>(); }
    /// @}
};

template <typename ComponentT>
struct composition<ComponentT>: private wrapper<ComponentT> {
    using component_type = ComponentT;
    using wrapper_type   = wrapper<component_type>;

    template <typename... Args>
    static composition<ComponentT> compose(Args&&... args) { return compositor<ComponentT>::compose(std::forward<Args>(args)...); }

    template <typename ArgT>
    composition(ArgT&& arg): wrapper_type(std::forward<ArgT>(arg)) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    component_type& get() { return wrapper_type::component(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const component_type& get() const { return wrapper_type::component(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    component_type& at() { return wrapper_type::component(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const component_type& at() const { return wrapper_type::component(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT>; }
    /// @}
};

}
}

#endif // UDHO_MIDDLEWARE_COMPOSITION_H
