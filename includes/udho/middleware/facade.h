#ifndef UDHO_MIDDLEWARE_FACADE_H
#define UDHO_MIDDLEWARE_FACADE_H

#include <type_traits>
#include <udho/middleware/fwd.h>
#include <udho/middleware/features.h>
#include <udho/middleware/component.h>

namespace udho {
namespace middleware {

#ifndef __DOXYGEN__

namespace detail{

template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() == Idx , bool> = true>
std::size_t facade_apply(FacadeT& facade, Function&& function) { return 0; }

template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() != Idx , bool> = true>
std::size_t facade_apply(FacadeT& facade, Function&& function) {
    bool result = function(facade.template get<FeatureT, Idx>());
    std::size_t count = result;
    if(result) {
        count += facade_apply<FacadeT, FeatureT, Function, Idx+1>(facade, std::forward<Function>(function));
    }
    return count;
}



template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() == Idx , bool> = true>
std::size_t facade_apply(const FacadeT& facade, Function&& function) { return 0; }

template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() != Idx , bool> = true>
std::size_t facade_apply(const FacadeT& facade, Function&& function) {
    bool result = function(facade.template get<FeatureT, Idx>());
    std::size_t count = result;
    if(result) {
        count += facade_apply<FacadeT, FeatureT, Function, Idx+1>(facade, std::forward<Function>(function));
    }
    return count;
}


}

template <typename ComponentT, typename... Tail>
struct facade_chain: private facade_chain<Tail...> {
    using component_type = component_wrapper<ComponentT>;

    template <typename FeatureX, typename... Features>
    friend struct evaluator;

    template <typename ComponentQ>
    using result_type = std::conditional_t<std::is_same_v<ComponentQ, ComponentT>, component_type, typename facade_chain<Tail...>::template result_type<ComponentQ>>;

    template <typename F>
    using result_type_by_feature = std::conditional_t<std::is_same_v<typename component_type::feature, F>, component_type, typename facade_chain<Tail...>::template result_type_by_feature<F>>;

    template <typename ComponentQ>
    using has_component = std::conditional_t<std::is_same_v<ComponentQ, ComponentT>, std::true_type, typename facade_chain<Tail...>::template has_component<ComponentQ>>;

    static_assert(!facade_chain<Tail...>::template has_component<ComponentT>::value, "Duplicate components are not allowed in a middleware facade");


    template <typename ArgT, typename... Args>
    inline explicit facade_chain(ArgT&& arg, Args&&... args): _component(std::forward<ArgT>(arg), std::forward<Args>(args)...), facade_chain<Args...>(std::forward<ArgT>(arg), std::forward<Args>(args)...) { }


    template <typename OtherComponentT, typename... OtherTail>
    inline explicit facade_chain(const facade_chain<OtherComponentT, OtherTail...>& other): _component(other.template get<ComponentT>()), facade_chain<Tail...>(other) {}

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    component_type& get() { return _component; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    result_type<ComponentQ>& get() { return facade_chain<Tail...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const component_type& get() const { return _component; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const result_type<ComponentQ>& get() const { return facade_chain<Tail...>::template get<ComponentQ>(); }


    template <typename FeatureT, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return 1+facade_chain<Tail...>::template count<FeatureT>(); }

    template <typename FeatureT, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return facade_chain<Tail...>::template count<FeatureT>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    component_type& get() { return _component; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const component_type& get() const { return _component; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    result_type_by_feature<FeatureT>& get() { return facade_chain<Tail...>::template get<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    const result_type_by_feature<FeatureT>& get() const { return facade_chain<Tail...>::template get<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    result_type_by_feature<FeatureT>& get() { return facade_chain<Tail...>::template get<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    const result_type_by_feature<FeatureT>& get() const { return facade_chain<Tail...>::template get<FeatureT, Idx>(); }


    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return detail::facade_apply<facade_chain<ComponentT, Tail...>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return detail::facade_apply<facade_chain<ComponentT, Tail...>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }

private:
    facade_chain<Tail...>& tail() { return *this; }
    const facade_chain<Tail...>& tail() const { return *this; }


private:
    component_type _component;
};

template <typename ComponentT>
struct facade_chain<ComponentT> {
    using component_type = component_wrapper<ComponentT>;

    template <typename FeatureX, typename... Features>
    friend struct evaluator;

    template <typename ComponentQ>
    using result_type = std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, component_type>;

    template <typename F>
    using result_type_by_feature = std::enable_if_t<std::is_same_v<typename component_type::feature, F>, component_type>;

    template <typename ComponentQ>
    using has_component = std::is_same<ComponentQ, ComponentT>;


    template < typename ArgT, typename... Args>
    inline explicit facade_chain(ArgT&& arg, Args&&... args): _component(std::forward<ArgT>(arg), std::forward<Args>(args)...) {}


    template <typename OtherComponentT, typename... OtherTail>
    inline explicit facade_chain(const facade_chain<OtherComponentT, OtherTail...>& other): _component(other.template get<ComponentT>()) {}

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    component_type& get() { return _component; }    

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const component_type& get() const { return _component; }




    template <typename FeatureT, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return 1; }

    template <typename FeatureT, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    static constexpr int count() { return 0; }



    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    component_type& get() { return _component; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const component_type& get() const { return _component; }


    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return detail::facade_apply<facade_chain<ComponentT>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return detail::facade_apply<facade_chain<ComponentT>, FeatureT, Function, 0>(*this, std::forward<Function>(f)); }

private:
    component_type _component;
};

template <typename... Components>
class facade: public facade_chain<Components...>{
    using facade_chain_type = facade_chain<Components...>;

public:
    using facade_chain_type::facade_chain_type;

    template <typename... Args>
    facade(Args&&... args): facade_chain_type(std::forward<Args>(args)...) {
        detail::arguments<Args...>::template expect<sizeof...(Args), Components...>();
    }

    using facade_chain_type::get;
    using facade_chain_type::count;
    using facade_chain_type::apply;
};

#else


#endif // __DOXYGEN__


}
}

#endif // UDHO_MIDDLEWARE_FACADE_H
