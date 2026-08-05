#ifndef UDHO_MANIFOLD_COMPOSITION_H
#define UDHO_MANIFOLD_COMPOSITION_H

#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/wrapper.h>
#include <udho/manifold/detail.h>
#include <udho/manifold/utils.h>
#include <udho/manifold/config.h>

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
                std::integral_constant<bool, (detail::argument_traits<Components>::template is_feasible<ArgT&&>) >...
            >::value == 1;
        static_assert(feasible_for_atleast_one_component<Index, count>::value, "constraint feasible_for_atleast_one_component<Index, true> must be satisfied for all arguments");
    }
};

template <typename ArgT, typename... Args>
struct feasible_for<ArgT, Args...>: feasible_for<Args...>{
    template <std::size_t Index, typename... Components>
    static constexpr void assert_msg(ArgT&& arg, Args&&... args) {
        constexpr const bool count = udho::utils::traits::accumulate<
                std::integral_constant<bool, (detail::argument_traits<Components>::template is_feasible<ArgT&&>) >...
            >::value == 1;

        static_assert(feasible_for_atleast_one_component<Index, count>::value, "constraint feasible_for_atleast_one_component<Index, true> must be satisfied for all arguments");
        feasible_for<Args...>::template assert_msg<Index+1, Components...>(std::forward<Args>(args)...);
    }
};

}

/**
 * @brief Factory for creating composition instances with flexible argument handling
 * @ingroup DoxyG_manifold
 * The compositor provides static factory methods to create composition instances
 * from a variadic set of component arguments. It handles the complexity of matching
 * arguments to their corresponding components and applying default construction
 * where appropriate.
 *
 * @tparam Components... The component types to be composed
 *
 * # Design Philosophy
 *
 * The compositor solves three key problems:
 *
 * 1. **Unordered Arguments**: Arguments can be provided in any order; they are
 *    matched to components by type, not position
 *
 * 2. **Partial Specification**: Not all components need explicit arguments;
 *    missing components are default-constructed if possible
 *
 * 3. **Compile-Time Validation**: Each argument must match exactly one component,
 *    preventing ambiguity and ensuring type safety
 *
 * # Argument Matching
 *
 * For each provided argument `ArgT`, the compositor searches for a component
 * `ComponentT` where:
 * - `ArgT` is an lvalue reference to `ComponentT`, OR
 * - `ArgT` is an rvalue reference to `ComponentT` AND `ComponentT` is move-constructible
 *
 * If no matching argument exists for a component:
 * - Default construction is attempted if `ComponentT` is default-constructible
 * - Compilation fails otherwise
 *
 * # Example Usage
 *
 * @code
 * // Three components with different construction requirements
 * struct ComponentA {  // Default constructible
 *     ComponentA() = default;
 *     ComponentA(ComponentA&&) = default;
 * };
 *
 * struct ComponentB {  // Move-only
 *     ComponentB() = delete;
 *     ComponentB(ComponentB&&) = default;
 * };
 *
 * struct ComponentC {  // Neither (must be borrowed)
 *     ComponentC() = delete;
 *     ComponentC(ComponentC&&) = delete;
 * };
 *
 * ComponentC component_c;  // Create externally
 *
 * // Compose with unordered arguments
 * auto comp = compositor<ComponentA, ComponentB, ComponentC>::compose(
 *     component_c,           // Borrowed (lvalue)
 *     ComponentB{},          // Owned (rvalue, moved)
 *     // ComponentA omitted - will be default-constructed
 * );
 *
 * // Or reorder arguments - same result
 * auto comp2 = compositor<ComponentA, ComponentB, ComponentC>::compose(
 *     ComponentB{},          // Arguments matched by type,
 *     component_c            // not by position
 * );
 * @endcode
 *
 * # Compile-Time Guarantees
 *
 * Static assertions ensure:
 * - Each argument is feasible for exactly one component
 * - No argument is ambiguous or unused
 * - All components can be constructed (explicitly or by default)
 *
 * @code
 * // ERROR: Ambiguous argument (matches multiple components)
 * auto bad1 = compositor<ComponentA, ComponentA>::compose(ComponentA{});
 * // Static assertion: "feasible_for_atleast_one_component<Index, true> must be satisfied"
 *
 * // ERROR: Unknown argument type
 * auto bad2 = compositor<ComponentA, ComponentB>::compose(ComponentX{});
 * // Static assertion: argument not feasible for any component
 *
 * // ERROR: Missing required component (not default-constructible)
 * auto bad3 = compositor<ComponentA, ComponentC>::compose();
 * // Static assertion: ComponentC requires lvalue reference
 * @endcode
 *
 * @see composition
 * @see detail::arguments
 * @see detail::feasible_for
 */
template <typename... Components>
struct compositor {
    using composition_type = composition<Components...>;

    /**
     * @brief Creates a composition with provided arguments
     *
     * Arguments are matched to components by type. Each argument must be feasible
     * for exactly one component. Components without matching arguments are
     * default-constructed if possible.
     *
     * @tparam ArgT Type of the first argument
     * @tparam Args Types of remaining arguments
     * @param arg First argument (lvalue or rvalue reference to a component)
     * @param args Remaining arguments
     * @return composition_type A composition instance with all components initialized
     *
     * # Storage Semantics
     *
     * The storage mode for each component is determined by how it's constructed:
     *
     * | Argument Type  | Component Requirements    | Storage Mode  | Ownership             |
     * |----------------|---------------------------|---------------|-----------------------|
     * | `ComponentT&`  | Any                       | Borrowed      | User manages lifetime |
     * | `ComponentT&&` | Move-constructible        | Owned         | Composition owns      |
     * | Not provided   | Default-constructible     | Owned         | Composition owns      |
     * | Not provided   | Not default-constructible | Compile error | N/A                   |
     *
     * # Examples
     *
     * @code
     * // Example 1: Mixed storage modes
     * ComponentA a;           // External instance
     *
     * auto comp1 = compositor<ComponentA, ComponentB, ComponentC>::compose(
     *     a,                  // Borrowed (wrapper holds reference)
     *     ComponentB{},       // Owned by move (wrapper owns instance)
     *     ComponentC{}        // Owned by move
     * );
     *
     * // 'a' must outlive comp1
     * // ComponentB and ComponentC are independent of external instances
     *
     * // Example 2: All owned (default-constructed)
     * auto comp2 = compositor<ComponentA, ComponentB>::compose();
     * // Both components default-constructed and owned
     *
     * // Example 3: Unordered matching
     * auto comp3 = compositor<ComponentA, ComponentB, ComponentC>::compose(
     *     ComponentC{},       // Matched to ComponentC (not position 0)
     *     a,                  // Matched to ComponentA (not position 1)
     *     ComponentB{}        // Matched to ComponentB (not position 2)
     * );
     * @endcode
     *
     * @warning For borrowed components (lvalue references), the user must ensure
     *          the referenced component outlives the composition. Accessing a
     *          destroyed component through the composition results in undefined behavior.
     *
     * @note Static assertions enforce that each argument matches exactly one component
     */
    template <typename ArgT, typename... Args>
    static composition_type compose(ArgT&& arg, Args&&... args) {
        detail::feasible_for<ArgT&&, Args&&...>::template assert_msg<0, Components...>(std::forward<ArgT>(arg), std::forward<Args>(args)...);
        return composition_type{detail::arguments<ArgT&&, Args&&...>::template find<Components>(std::forward<ArgT>(arg), std::forward<Args>(args)...)...};
    }

    /**
     * @brief Creates a composition with all components default-constructed
     *
     * All components must be default-constructible for this overload to compile.
     * All components are owned by the composition.
     *
     * @return composition_type A composition with all default-constructed components
     *
     * # Example
     *
     * @code
     * // All components default-constructible
     * struct ComponentA { ComponentA() = default; };
     * struct ComponentB { ComponentB() = default; };
     *
     * auto comp = compositor<ComponentA, ComponentB>::compose();
     *
     * // Both components owned by composition, default-constructed
     * CHECK(comp.get<ComponentA>().owned());
     * CHECK(comp.get<ComponentB>().owned());
     * @endcode
     *
     * @warning Compilation error if any component is not default-constructible:
     * @code
     * struct NonDefault {
     *     NonDefault() = delete;
     *     NonDefault(int) {}
     * };
     *
     * // ERROR: NonDefault is not default-constructible
     * auto bad = compositor<ComponentA, NonDefault>::compose();
     * @endcode
     */
    static composition_type compose() {
        return composition_type{std::enable_if_t<!std::is_void_v<Components>, default_constructed>{}...};
    }
};

#ifdef __DOXYGEN__
/**
 * @brief A type-safe container of component instances with feature-based access
 * @ingroup DoxyG_manifold
 * The composition class holds instances of multiple components and provides
 * uniform access mechanisms for retrieving components either by type or by
 * the features they provide. This is the central data structure for assembling
 * independent components into a cohesive composition.
 *
 * @tparam Components...
 *
 * # Conceptual Model
 *
 * A composition can be viewed as a collection of N distinct component instances
 *
 *    ```
 *    composition<A, B, C> = { instance_of_A, instance_of_B, instance_of_C }
 *    ```
 *
 * Each specifying the set of features it supports
 *
 *    ```
 *    A -> {F1, F2, F3}
 *    B -> {F4}
 *    C -> {F5}
 *    ```
 *
 * # Ownership Model
 *
 * Each component is wrapped internally, and can be in one of two states:
 *
 * - **Borrowed**: The wrapper holds a reference to an external component.
 *   Lifetime management is the usercode's responsibility.
 *
 * - **Owned**: The wrapper owns the component (moved or default-constructed).
 *   The composition manages the component's lifetime.
 *
 * The ownership mode is transparent to most operations but can be queried:
 *
 * @code
 * ComponentA a;
 * auto comp = compositor<ComponentA, ComponentB>::compose(
 *     a,              // Borrowed
 *     ComponentB{}    // Owned
 * );
 *
 * CHECK(comp.get<ComponentA>().borrowed());  // true
 * CHECK(comp.get<ComponentB>().owned());     // true
 * @endcode
 *
 * # Access Patterns
 *
 * The composition provides three primary access patterns:
 *
 * ## 1. Access by Component Type
 *
 * Direct access to a specific component wrapper:
 *
 * @code
 * auto& wrapper_a = comp.get<ComponentA>();
 * auto& component_a = wrapper_a.component();
 * @endcode
 *
 * ## 2. Access by Feature and Index
 *
 * Access the Nth component that provides a given feature:
 *
 * @code
 * // Given: ComponentA and ComponentC both provide FeatureX
 * auto& first_x = comp.at<FeatureX, 0>();   // ComponentA wrapper
 * auto& second_x = comp.at<FeatureX, 1>();  // ComponentC wrapper
 * @endcode
 *
 * ## 3. Feature Visitation
 *
 * Apply a function to all components providing a feature:
 *
 * @code
 * size_t visited = comp.apply<FeatureX>([](auto& wrapper) {
 *     wrapper.component().process();
 *     return true;  // continue to next
 * });
 * @endcode
 *
 * @see compositor
 * @see wrapper
 * @see fabric
 * @see features
 * @see configs
 */
template <typename... Components>
struct composition{
#else
template <typename ComponentT, typename... Rest>
struct composition<ComponentT, Rest...>: private wrapper<ComponentT>, private composition<Rest...>{
#endif // __DOXYGEN__
    using component_type = ComponentT;
    using features_type  = typename ComponentT::features;
    using wrapper_type   = wrapper<component_type>;
    using configs_type   = udho::manifold::configs<ComponentT, Rest...>;

    template <typename ComponentQ>
    using has_component  = std::conditional_t<
        std::is_same_v<ComponentQ, ComponentT>,
        std::true_type,
        typename composition<Rest...>::template has_component<ComponentQ>
    >;

    template <typename FeatureT, std::size_t Idx>
    using component_at   = std::conditional_t<
        features_type::template has<FeatureT>::value && Idx == 0,
        component_type,
        std::conditional_t<
            features_type::template has<FeatureT>::value && Idx != 0,
            typename composition<Rest...>::template component_at<FeatureT, Idx - 1>,
            typename composition<Rest...>::template component_at<FeatureT, Idx>
        >
    >;

    /**
     * @brief The fabric type for a given stage
     *
     * Compiles to a `fabric<Stage, Facets...>` containing only the facets
     * where the feature's stage matches the template parameter. This is a
     * compile-time transformation that filters facets by stage.
     *
     * @tparam Stage The pipeline stage number (0, 1, 2, ...)
     *
     * # Example
     *
     * @code
     * struct FeatureX { static constexpr size_t stage = 0; };
     * struct FeatureY { static constexpr size_t stage = 1; };
     * struct FeatureZ { static constexpr size_t stage = 0; };
     *
     * struct ComponentA { using features = features<FeatureX, FeatureY>; };
     * struct ComponentB { using features = features<FeatureZ>; };
     *
     * using comp_type = composition<ComponentA, ComponentB>;
     *
     * // Stage 0: facet<ComponentA, FeatureX>, facet<ComponentB, FeatureZ>
     * using fabric_0 = comp_type::fabric_type<0>;
     *
     * // Stage 1: facet<ComponentA, FeatureY>
     * using fabric_1 = comp_type::fabric_type<1>;
     * @endcode
     */
    template <std::size_t Stage>
    using fabric_type    = typename udho::manifold::detail::flatten_all<Stage, ComponentT, Rest...>::type;

    template <std::size_t, typename... Components>
    friend struct fabric;

    /**
     * @brief Factory method for composition creation
     *
     * Convenience method that forwards to `compositor<ComponentT, Rest...>::compose()`
     *
     * @tparam Args Argument types
     * @param args Arguments for component construction
     * @return composition<ComponentT, Rest...> The composed instance
     *
     * # Example
     *
     * @code
     * // These are equivalent:
     * auto comp1 = compositor<A, B>::compose(a, b);
     * auto comp2 = composition<A, B>::compose(a, b);
     * @endcode
     */
    template <typename... Args>
    static composition<ComponentT, Rest...> compose(Args&&... args) { return compositor<ComponentT, Rest...>::compose(std::forward<Args>(args)...); }

    /**
     * @brief Constructor from explicit arguments
     *
     * Constructs the composition by initializing each component wrapper with
     * its corresponding argument. The number of arguments must exactly match
     * the number of components.
     *
     * @tparam ArgT Type of the first argument
     * @tparam Args Types of remaining arguments
     * @param arg Argument for ComponentT
     * @param args Arguments for Rest... components
     *
     * @pre `sizeof...(Args) == sizeof...(Rest)`
     *
     * # Argument Types
     *
     * Each argument must be one of:
     * - `ComponentT&` - Wrapper borrows the component (borrowed mode)
     * - `ComponentT&&` - Wrapper takes ownership via move (owned mode)
     * - `default_constructed{}` - Wrapper default-constructs component (owned mode)
     *
     * # Example
     *
     * @code
     * ComponentA a;
     *
     * composition<ComponentA, ComponentB, ComponentC> comp{
     *     a,                        // Borrowed
     *     ComponentB{},             // Owned (moved)
     *     default_constructed{}     // Owned (default-constructed)
     * };
     * @endcode
     *
     * @warning This constructor is typically not called directly. Use
     *          `compositor::compose()` for automatic argument matching.
     *
     * @note Static assertion ensures argument count matches component count
     */
    template <typename ArgT, typename... Args>
    composition(ArgT&& arg, Args&&... args): wrapper_type(std::forward<ArgT>(arg)), composition<Rest...>(std::forward<Args>(args)...) {
        static constexpr const std::size_t arguments_provided = sizeof...(Args);
        static constexpr const std::size_t expected_arguments = sizeof...(Rest);
        static_assert(arguments_provided == expected_arguments, "insufficient number of arguments passed to the composition constructor");
    }

    /**
     * @name Component Access by Type
     *
     * Direct access to component wrappers by their type. These methods provide
     * O(1) compile-time resolution to the requested component.
     *
     * @{
     */

    /**
     * @brief Get wrapper for a specific component type
     *
     * Retrieves the wrapper containing the component of the specified type.
     * The lookup is resolved at compile-time through recursive template matching.
     *
     * @tparam ComponentQ The component type to retrieve
     * @return wrapper_type& Reference to the wrapper for ComponentQ
     *
     * # Example
     *
     * @code
     * auto comp = compositor<ComponentA, ComponentB, ComponentC>::compose();
     *
     * auto& wrapper_b = comp.get<ComponentB>();
     * auto& component_b = wrapper_b.component();
     * @endcode
     *
     * @note Compile error if ComponentQ is not in the composition:
     * @code
     * auto& bad = comp.get<ComponentX>();  // ERROR: ComponentX not in composition
     * @endcode
     */
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return *this; }

#ifndef __DOXYGEN__

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition<Rest...>::template get<ComponentQ>(); }

#endif // __DOXYGEN__

    /**
     * @brief Get const wrapper for a specific component type
     *
     * @tparam ComponentQ The component type to retrieve
     * @return const wrapper_type& Const reference to the wrapper
     */
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return *this; }

#ifndef __DOXYGEN__

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition<Rest...>::template get<ComponentQ>(); }

#endif // __DOXYGEN__
    /** @} */

    /**
     * @name Feature-Based Access
     *
     * Access components by the features they provide. Components are indexed
     * in declaration order, counting only those that provide the requested feature.
     *
     * @{
     */

    /**
     * @brief Access the Idx-th component providing a feature
     *
     * Components are indexed in the order they appear in the template parameter
     * list, counting only those that provide the requested feature. Indexing
     * starts at 0.
     *
     * @tparam FeatureT The feature type to search for
     * @tparam Idx Zero-based index among components providing this feature
     * @return wrapper_type& Reference to wrapper for the Idx-th component with FeatureT
     *
     * # Feature Indexing
     *
     * Given components in declaration order, only components providing FeatureT
     * are counted:
     *
     * @code
     * // Declarations:
     * struct ComponentA { using features = features<FeatureX, FeatureY>; };
     * struct ComponentB { using features = features<FeatureY>; };
     * struct ComponentC { using features = features<FeatureX, FeatureZ>; };
     * struct ComponentD { using features = features<FeatureZ>; };
     *
     * composition<ComponentA, ComponentB, ComponentC, ComponentD> comp = ...;
     *
     * // FeatureX: ComponentA (idx=0), ComponentC (idx=1)
     * auto& wrapper_x0 = comp.at<FeatureX, 0>();  // ComponentA
     * auto& wrapper_x1 = comp.at<FeatureX, 1>();  // ComponentC
     *
     * // FeatureY: ComponentA (idx=0), ComponentB (idx=1)
     * auto& wrapper_y0 = comp.at<FeatureY, 0>();  // ComponentA
     * auto& wrapper_y1 = comp.at<FeatureY, 1>();  // ComponentB
     *
     * // FeatureZ: ComponentC (idx=0), ComponentD (idx=1)
     * auto& wrapper_z0 = comp.at<FeatureZ, 0>();  // ComponentC
     * auto& wrapper_z1 = comp.at<FeatureZ, 1>();  // ComponentD
     * @endcode
     *
     * @note Compile error if Idx >= count<FeatureT>():
     * @code
     * // ERROR: Only 2 components provide FeatureX
     * auto& bad = comp.at<FeatureX, 2>();
     * @endcode
     *
     * @note Compile error if no component provides FeatureT:
     * @code
     * // ERROR: No component provides UnknownFeature
     * auto& bad = comp.at<UnknownFeature, 0>();
     * @endcode
     */
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

#ifndef __DOXYGEN__

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx != 0, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!features_type::template has<FeatureT>::value, bool> = true>
    auto& at() { return composition<Rest...>::template at<FeatureT, Idx>(); }

#endif // __DOXYGEN__

    /**
     * @brief Access the Idx-th component with feature (const)
     *
     * @tparam FeatureT The feature type
     * @tparam Idx Index among components with this feature
     * @return const wrapper_type& Const reference to wrapper
     */
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

#ifndef __DOXYGEN__

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx != 0, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!features_type::template has<FeatureT>::value, bool> = true>
    const auto& at() const { return composition<Rest...>::template at<FeatureT, Idx>(); }

#endif // __DOXYGEN__

    /** @} */

    /**
     * @name Feature Queries
     *
     * Compile-time queries about feature availability
     *
     * @{
     */

    /**
     * @brief Count how many components provide a feature
     *
     * Returns the number of components in this composition that declare
     * the given feature in their features list. This is a compile-time
     * constant evaluated through recursive template instantiation.
     *
     * @tparam FeatureT The feature type to count
     * @return int Number of components providing FeatureT
     *
     * # Example
     *
     * @code
     * struct ComponentA { using features = features<FeatureX, FeatureY>; };
     * struct ComponentB { using features = features<FeatureY>; };
     * struct ComponentC { using features = features<FeatureZ>; };
     *
     * using comp_type = composition<ComponentA, ComponentB, ComponentC>;
     *
     * static_assert(comp_type::count<FeatureX>() == 1);  // Only ComponentA
     * static_assert(comp_type::count<FeatureY>() == 2);  // ComponentA and ComponentB
     * static_assert(comp_type::count<FeatureZ>() == 1);  // Only ComponentC
     * static_assert(comp_type::count<UnknownFeature>() == 0);  // No component
     * @endcode
     */
    template <typename FeatureT>
    static constexpr int count() { return features_type::template has<FeatureT>::value + composition<Rest...>::template count<FeatureT>(); }
    /** @} */

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<composition<ComponentT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<composition<ComponentT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

private:

#ifndef __DOXYGEN__

    composition<Rest...>& tail() { return *this; }

    const composition<Rest...>& tail() const { return *this; }

#endif // __DOXYGEN__
};

#ifndef __DOXYGEN__

template <typename ComponentT>
struct composition<ComponentT>: private wrapper<ComponentT> {
    using component_type = ComponentT;
    using features_type  = typename ComponentT::features;
    using wrapper_type   = wrapper<component_type>;
    using configs_type   = udho::manifold::configs<ComponentT>;

    template <typename ComponentQ>
    using has_component  = std::is_same<ComponentQ, ComponentT>;

    template <typename FeatureT, std::size_t Idx>
    using component_at   = std::conditional_t<
        features_type::template has<FeatureT>::value && Idx == 0,
        component_type,
        void
    >;

    template <std::size_t Stage>
    using fabric_type    = typename udho::manifold::detail::flatten_all<Stage, ComponentT>::type;


    template <std::size_t, typename... Components>
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

#endif // __DOXYGEN__

/**
 * @}
 */

}
}


#endif // UDHO_MANIFOLD_COMPOSITION_H
