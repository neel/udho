#ifndef UDHO_MIDDLEWARE_COMPOSITION_H
#define UDHO_MIDDLEWARE_COMPOSITION_H

#include <type_traits>
#include <udho/middleware/fwd.h>
#include <udho/middleware/features.h>
#include <udho/middleware/component.h>

namespace udho{
namespace middleware{

template <typename... Components>
struct composition;

template <typename ComponentT, typename... Rest>
struct composition<ComponentT, Rest...>: private composition<Rest...>{
    using component_type = ComponentT;

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    component_type& get() { return _component; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition<Rest...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const component_type& get() const { return _component; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition<Rest...>::template get<ComponentQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    component_type& at() { return _component; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const component_type& at() const { return _component; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT> + composition<Rest...>::template count<FeatureT>(); }
    /// @}

private:
    component_type _component;
};

template <typename ComponentT>
struct composition<ComponentT> {
    using component_type = ComponentT;

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    component_type& get() { return _component; }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const component_type& get() const { return _component; }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    component_type& at() { return _component; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const component_type& at() const { return _component; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT>; }
    /// @}

private:
    component_type _component;
};

}
}

#endif // UDHO_MIDDLEWARE_COMPOSITION_H
