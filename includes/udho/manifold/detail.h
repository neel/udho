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
    static constexpr const bool feasible_rvalue_reference = !std::is_lvalue_reference_v<ArgT> &&
                                                            std::is_rvalue_reference_v<ArgT> &&
                                                            std::is_same_v<std::remove_reference_t<ArgT>, ComponentT>;
    template <typename ArgT>
    static constexpr const bool should_move = feasible_rvalue_reference<ArgT>;

    template <typename ArgT>
    static constexpr const bool is_feasible = feasible_lvalue_reference<ArgT> || (std::is_move_constructible_v<ComponentT> && feasible_rvalue_reference<ArgT>);
};

template <typename ArgT, typename... Args>
struct arguments_lookup{
    template <typename ComponentT>
    using type_for = std::conditional_t<argument_traits<ComponentT>::template is_feasible<ArgT&&>, ArgT, typename arguments_lookup<Args&&...>::template type_for<ComponentT> >;

    template <typename ComponentT, std::enable_if_t<argument_traits<ComponentT>::template is_feasible<ArgT&&>, bool> = true>
    static constexpr ArgT arg_for(ArgT&& arg, Args&&... args) { return std::forward<ArgT>(arg); }

    template <typename ComponentT, std::enable_if_t<!argument_traits<ComponentT>::template is_feasible<ArgT&&>, bool> = true>
    static constexpr type_for<ComponentT> arg_for(ArgT&& arg, Args&&... args) { return arguments_lookup<Args&&...>::template arg_for<ComponentT>(std::forward<Args>(args)...); }

};

template <typename ArgT>
struct arguments_lookup<ArgT>{
    template <typename ComponentT>
    using type_for = std::conditional_t<argument_traits<ComponentT>::template is_feasible<ArgT&&>, ArgT, default_constructed>;

    template <typename ComponentT, std::enable_if_t<argument_traits<ComponentT>::template is_feasible<ArgT&&>, bool> = true>
    static constexpr ArgT arg_for(ArgT&& arg) { return std::forward<ArgT>(arg); }

    template <typename ComponentT, std::enable_if_t<!argument_traits<ComponentT>::template is_feasible<ArgT&&>, bool> = true>
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
    static constexpr type_for<ComponentT> find(Args&&... args) { return arguments_lookup<Args&&...>::template arg_for<ComponentT>(std::forward<Args>(args)...); }
};

/**
 * expands a component C having features {F1, F2, ...} into fabric<facet<C, F_i>> \forall i through fabric_type typedef
 * @{
 */

enum stage_comp_op{
    eq, neq, le, gt, leq, gte, all
};

template <stage_comp_op Op, std::size_t Lhs, std::size_t Rhs>
struct stage_compare;

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::eq,  Lhs, Rhs>: std::integral_constant<bool, (Lhs == Rhs)> {};

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::neq,  Lhs, Rhs>: std::integral_constant<bool, (Lhs != Rhs)> {};

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::le,   Lhs, Rhs>: std::integral_constant<bool, (Lhs < Rhs)> {};

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::gt,  Lhs, Rhs>: std::integral_constant<bool, (Lhs > Rhs)> {};

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::leq, Lhs, Rhs>: std::integral_constant<bool, (Lhs <= Rhs)> {};

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::gte, Lhs, Rhs>: std::integral_constant<bool, (Lhs >= Rhs)> {};

template <std::size_t Lhs, std::size_t Rhs>
struct stage_compare<stage_comp_op::all, Lhs, Rhs>: std::integral_constant<bool, true> {};

template <stage_comp_op Op, std::size_t Stage, typename FeatureT>
struct expand_feature_pairs;

template <stage_comp_op Op, std::size_t Stage, typename... EnabledFacets>
struct enabled_facets_set;

template <typename L, typename R>
struct merged_fabric;

template <typename... Rest>
struct facet_container{
    template <typename X>
    using prepend = std::conditional_t<std::is_void_v<X>, facet_container<Rest...>, facet_container<X, Rest...>>;

    template <std::size_t Stage>
    using fabric_type = udho::manifold::fabric<Stage, Rest...>;
};


template <stage_comp_op Op, std::size_t Stage, typename EnabledFacet, typename... Rest>
struct enabled_facets_set<Op, Stage, EnabledFacet, Rest...>{
    using rest_type = enabled_facets_set<Op, Stage, Rest...>;
    using container_type = typename rest_type::container_type::template prepend<EnabledFacet>;
    using fabric_type = typename container_type::template fabric_type<Stage>;
};

template <stage_comp_op Op, std::size_t Stage>
struct enabled_facets_set<Op, Stage>{
    using container_type = facet_container<>;
};

template <stage_comp_op Op, std::size_t Stage, typename... Features>
struct expand_feature_pairs<Op, Stage, udho::manifold::features<Features...>>{
    template <typename ComponentT>
    using fabric_enabled_type = enabled_facets_set<Op, Stage,
            std::conditional_t<stage_compare<Op, Features::stage, Stage>::value, udho::manifold::facet<ComponentT, Features>, void>...
        >;

    template <typename ComponentT>
    using fabric_type = typename fabric_enabled_type<ComponentT>::fabric_type;
};
/// @}


template <stage_comp_op Op, std::size_t Stage, typename ComponentT>
struct get_facets{
    using type = typename expand_feature_pairs<Op, Stage, typename ComponentT::features>::template fabric_type<ComponentT>;
};

template <std::size_t Stage, typename... FacetsSet>
struct flatten_fabric;

template <std::size_t Stage, typename... Facets>
struct flattened_fabric{
    using type = udho::manifold::fabric<Stage, Facets ...>;
};



template <std::size_t Stage, typename... Facets, typename... Rest>
struct flatten_fabric<Stage, udho::manifold::fabric<Stage, Facets...>, Rest...> {
    using type   = flattened_fabric<Stage, Facets...>;
    using rest   = typename flatten_fabric<Stage, Rest...>::merged;
    using merged = typename merged_fabric<type, rest>::type;
};

template <std::size_t Stage>
struct flatten_fabric<Stage>{
    using type   = flattened_fabric<Stage>;
    using merged = flattened_fabric<Stage>;
};


template <std::size_t Stage, typename... X, typename... Y>
struct merged_fabric<flattened_fabric<Stage, X...>, flattened_fabric<Stage, Y...>>{
    using type = flattened_fabric<Stage, X..., Y...>;
};


template <stage_comp_op Op, std::size_t Stage, typename... Components>
struct flatten_all_{
    using type = typename flatten_fabric<Stage, typename get_facets<Op, Stage, Components>::type...>::merged::type;
};

template <std::size_t Stage, typename... Components>
using flatten_all = flatten_all_<stage_comp_op::eq, Stage, Components...>;

template <std::size_t Stage, typename... Components>
using flatten_all_except = flatten_all_<stage_comp_op::neq, Stage, Components...>;

template <std::size_t Stage, typename... Components>
using flatten_all_before = flatten_all_<stage_comp_op::le, Stage, Components...>;

template <std::size_t Stage, typename... Components>
using flatten_all_before_including = flatten_all_<stage_comp_op::leq, Stage, Components...>;

template <std::size_t Stage, typename... Components>
using flatten_all_after = flatten_all_<stage_comp_op::gt, Stage, Components...>;

template <std::size_t Stage, typename... Components>
using flatten_all_after_including = flatten_all_<stage_comp_op::gte, Stage, Components...>;

template <typename... Components>
using flatten_all_of = flatten_all_<stage_comp_op::all, 0, Components...>;

}

}
}

#endif // UDHO_MANIFOLD_DETAIL_H
