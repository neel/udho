#ifndef UDHO_MANIFOLD_COMPOSITION_VIEW_H
#define UDHO_MANIFOLD_COMPOSITION_VIEW_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/wrapper.h>
#include <udho/manifold/utils.h>

namespace udho{
namespace manifold{

template <typename ComponentT, typename... Components>
struct composition_view<ComponentT, Components...>: composition_view<Components...>{
    using wrapper_type   = udho::manifold::wrapper<ComponentT>;
    using component_type = ComponentT;
    using features_type  = typename ComponentT::features;

    template <typename... OtherComponents>
    composition_view(udho::manifold::composition<OtherComponents...>& other): _wrapper(other.template get<ComponentT>()), composition_view<Components...>(other) {}

    template <typename... OtherComponents>
    composition_view(composition_view<OtherComponents...>& other): _wrapper(other.template get<ComponentT>()), composition_view<Components...>(other) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return _wrapper; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition_view<Components...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return _wrapper; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition_view<Components...>::template get<ComponentQ>(); }
    /// @}
private:
    wrapper_type& _wrapper;
};

template <>
struct composition_view<> {
    template <typename... OtherComponents>
    composition_view(udho::manifold::composition<OtherComponents...>& other) {}

    template <typename... OtherComponents>
    composition_view(composition_view<OtherComponents...>& other) {}
};

}
}


#endif // UDHO_MANIFOLD_COMPOSITION_VIEW_H
