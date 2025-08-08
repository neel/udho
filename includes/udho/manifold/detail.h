#ifndef UDHO_MANIFOLD_DETAIL_H
#define UDHO_MANIFOLD_DETAIL_H

#include <utility>
#include <boost/type_traits.hpp>
#include <udho/manifold/traits.h>
#include <udho/manifold/features.h>

namespace udho{
namespace manifold {

namespace detail {

/**
 * @brief helper class to check wheather an argument ArgT is feasible initialization argument for the component ComponentT
 * @tparam ComponentT The component type
 */
template <typename ComponentT>
struct argument_traits {
    template <typename ArgT>
    static constexpr const bool feasible_lvalue_reference = std::is_lvalue_reference_v<ArgT> &&
                                                            std::is_same_v<std::remove_reference_t<ArgT>, ComponentT>;

    template <typename ArgT>
    static constexpr const bool feasible_rvalue_reference =  std::is_same_v<std::remove_reference_t<ArgT>, ComponentT>;
    template <typename ArgT>
    static constexpr const bool should_move = feasible_rvalue_reference<ArgT>;


    template <typename ArgT>
    static constexpr const bool is_feasible = (component_traits<ComponentT>::shared && feasible_lvalue_reference<ArgT>) ||
                                              (!component_traits<ComponentT>::shared && should_move<ArgT>);
};

template <typename ArgT, typename... Args>
struct arguments_lookup{
    template <typename ComponentT>
    using type_for = std::conditional_t<argument_traits<ComponentT>::template is_feasible<ArgT>, ArgT, typename arguments_lookup<Args...>::template type_for<ComponentT> >;

    template <typename ComponentT, std::enable_if_t<argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr ArgT arg_for(ArgT&& arg, Args&&... args) { return std::forward<ArgT>(arg); }

    template <typename ComponentT, std::enable_if_t<!argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr type_for<ComponentT> arg_for(ArgT&& arg, Args&&... args) { return arguments_lookup<Args...>::template arg_for<ComponentT>(std::forward<Args>(args)...); }

};

template <typename ArgT>
struct arguments_lookup<ArgT>{
    template <typename ComponentT>
    using type_for = std::conditional_t<argument_traits<ComponentT>::template is_feasible<ArgT>, ArgT, default_constructed>;

    template <typename ComponentT, std::enable_if_t<argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr ArgT arg_for(ArgT&& arg) { return std::forward<ArgT>(arg); }

    template <typename ComponentT, std::enable_if_t<!argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr default_constructed arg_for(ArgT&& arg) { return default_constructed{}; }
};

/**
 * @brief Helper class to facilitate variadic unorder arguments while ensuring that no user provided argument has been skipped
 * @tparam Args... variadic arguments
 */
template <typename... Args>
struct arguments {
    template <typename ComponentT>
    using type_for = typename arguments_lookup<Args...>::template type_for<ComponentT>;

    /**
     * @brief finds the first suitable argument for the given component
     * @tparam ComponentT component for which a feasible type has to be found
     * @tparam Args... types of the arguments from where to select
     * @param args... variadic arguments from which to select the first feasible argument
     * @return the first feasible argument or default_constructed instance
     */
    template <typename ComponentT>
    static constexpr type_for<ComponentT> find(Args&&... args) { return arguments_lookup<Args...>::template arg_for<ComponentT>(std::forward<Args>(args)...); }
};

/**
 * expands a component C having features {F1, F2, ...} into mediator<delegate<C, F_i>> \forall i through mediator_type typedef
 * @{
 */
template <typename FeatureT>
struct expand_feature_pairs;

template <typename... Features>
struct expand_feature_pairs<udho::manifold::features<Features...>>{
    template <typename ComponentT>
    using mediator_type = udho::manifold::mediator<udho::manifold::delegate<ComponentT, Features>...>;
};
/// @}


template <typename ComponentT>
struct get_delegates{
    using type = typename expand_feature_pairs<typename ComponentT::features>::template mediator_type<ComponentT>;
};

template <typename... DelegatesSet>
struct flatten;

template <typename... Delegates>
struct flattened{
    using type = udho::manifold::mediator<Delegates ...>;
};

template <typename L, typename R>
struct combined;

template <typename... X, typename... Y>
struct combined<flattened<X...>, flattened<Y...>>{
    using type = flattened<X..., Y...>;
};

template <typename... Delegates, typename... Rest>
struct flatten<udho::manifold::mediator<Delegates...>, Rest...> {
    using type = flattened<Delegates...>;
    using rest = typename flatten<Rest...>::combined;
    using combined = typename combined<type, rest>::type;
};

template <>
struct flatten<>{
    using type = flattened<>;
    using combined = flattened<>;
};

template <typename... Components>
struct flatten_all{
    using type = typename flatten<typename get_delegates<Components>::type...>::combined::type;
};


}

}
}

#endif // UDHO_MANIFOLD_DETAIL_H
