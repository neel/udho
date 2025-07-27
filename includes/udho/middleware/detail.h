#ifndef UDHO_MIDDLEWARE_DETAIL_H
#define UDHO_MIDDLEWARE_DETAIL_H

#include <utility>
#include <boost/type_traits.hpp>
#include <udho/middleware/traits.h>

namespace udho{
namespace middleware {

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
    static constexpr const bool is_feasible = (component_traits<ComponentT>::prefer_reference && feasible_lvalue_reference<ArgT>) ||
                                              (!component_traits<ComponentT>::prefer_reference && should_move<ArgT>);
};

template <typename ArgT, typename... Args>
struct arguments_lookup{
    template <int Idx, typename ComponentT>
    static constexpr const int index_of = argument_traits<ComponentT>::template is_feasible<ArgT> ? Idx+1 : arguments_lookup<Args...>::template index_of<Idx+1, ComponentT>;

    template <typename ComponentT>
    using type_for = std::conditional_t<argument_traits<ComponentT>::template is_feasible<ArgT>, ArgT, typename arguments_lookup<Args...>::template type_for<ComponentT> >;

    template <typename ComponentT, std::enable_if_t<argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr ArgT arg_for(ArgT&& arg, Args&&... args) { return std::forward<ArgT>(arg); }

    template <typename ComponentT, std::enable_if_t<!argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr type_for<ComponentT> arg_for(ArgT&& arg, Args&&... args) { return arguments_lookup<Args...>::template arg_for<ComponentT>(std::forward<Args>(args)...); }

};

template <typename ArgT>
struct arguments_lookup<ArgT>{
    template <int Idx, typename ComponentT>
    static constexpr const int index_of = arguments_lookup<ComponentT>::template is_feasible<ArgT> ? Idx+1 : -1;

    template <typename ComponentT>
    using type_for = std::conditional_t<argument_traits<ComponentT>::template is_feasible<ArgT>, ArgT, default_constructed>;

    template <typename ComponentT, std::enable_if_t<argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr ArgT arg_for(ArgT&& arg) { return std::forward<ArgT>(arg); }

    template <typename ComponentT, std::enable_if_t<!argument_traits<ComponentT>::template is_feasible<ArgT>, bool> = true>
    static constexpr default_constructed arg_for(ArgT&& arg) { return default_constructed{}; }
};


template <typename...>
struct accumulate : std::integral_constant<std::size_t, 0> {};

template <typename T, typename... Ts>
struct accumulate<T, Ts...> : std::integral_constant<std::size_t,  T::value + accumulate<Ts...>::value> {};

/**
 * @brief Helper class to facilitate variadic unorder arguments while ensuring that no user provided argument has been skipped
 * @tparam Args... variadic arguments
 */
template <typename... Args>
struct arguments {
    template <int Idx, typename ComponentT>
    static constexpr const int index_of = arguments_lookup<Args...>::template index_of<Idx+1, ComponentT>;

    template <typename ComponentT>
    using type_for = typename arguments_lookup<Args...>::template type_for<ComponentT>;

    /**
     * @brief integer sequence of size sizeof...(Components) such that position i of that sequence denote the index of the feasible argument in Args, -1 if none feasible
     */
    template <typename... Components>
    using indexes = std::index_sequence<index_of<0, Components>...>;

    /**
     * @brief boolean sequence of size sizeof...(Components) such that position i of that sequence denote whether passing no argument is okay for that component or not
     */
    template <typename... Components>
    using default_allowed = std::integer_sequence<bool, (std::is_default_constructible_v<Components> && !component_traits<Components>::prefer_reference)...>;

    template <typename... Components>
    static constexpr const std::size_t arguments_mapped = accumulate<std::integral_constant<bool, (index_of<0, Components> > 0) >...>::value;

    template <typename... Components>
    static constexpr const bool no_args_skipped = indexes<Components...>::size() == sizeof...(Args);

    template <typename... Components>
    static constexpr void none_skipped() {
        static_assert(indexes<Components...>::size() == sizeof...(Args), "at least one of the arguments passed is not feasible for any component");
    }

    /**
     * @brief expect
     * @tparam Count expected number of feasible argument
     */
    template <std::size_t Count, typename... Components>
    static constexpr int expect() {
        static_assert(arguments_mapped<Components...> == Count, "at least one of the arguments passed is not feasible for any component");
        return 0;
    }

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



}

}
}

#endif // UDHO_MIDDLEWARE_DETAIL_H
