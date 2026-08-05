#ifndef UDHO_MANIFOLD_JOURNAL_H
#define UDHO_MANIFOLD_JOURNAL_H

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <udho/manifold/features.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/traits.h>
// #include <udho/manifold/wrapper.h>

namespace udho {
namespace manifold {

/**
 * @ingroup DoxyG_manifold
 * @{
 */

/**
 * @brief Optional-style wrapper for facet evaluation results
 *
 * Stores the result of a facet evaluation, tracking whether the result is ready
 * (has been computed) or not. Provides value semantics with move-only support
 * for result types.
 *
 * @tparam ResultT The type of result being stored
 * @tparam Feature The feature type this result corresponds to
 *
 * # Lifecycle
 *
 * @code
 * result_wrapper<State, MyFeature> result;
 * CHECK(!result.ready());  // Initially empty
 *
 * result = State{true};    // Assign result
 * CHECK(result.ready());   // Now contains value
 *
 * State s = result.value(); // Extract value
 * State s2 = *result;       // Dereference syntax
 * result->method();         // Pointer-like access
 * @endcode
 *
 * # Error Handling
 *
 * Accessing an unready result throws `std::runtime_error`:
 * @code
 * result_wrapper<State, F> result;
 * result.value();  // throws std::runtime_error
 * @endcode
 *
 * @warning Result types must be move-constructible and move-assignable
 *
 * @see journal
 */
template <typename ResultT, typename Feature>
struct result_wrapper{
    static_assert(std::is_move_constructible_v<ResultT>);
    // static_assert(std::is_move_assignable_v<ResultT>);
    // static_assert(std::is_copy_constructible_v<ResultT>);

    using type      = ResultT;                  ///< The underlying result type
    using feature   = Feature;                  ///< The feature type producing this result
    using opt_type  = std::optional<type>;      ///< Optional storage type

    /**
     * @brief Constructs an unevaluated result wrapper
     *
     * The wrapper starts in "not ready" state with no result value.
     */
    result_wrapper(): _result(std::nullopt) {}

    /**
     * @brief Constructs a wrapper with an initial result
     *
     * @param result The result value to store (moved into wrapper)
     */
    explicit result_wrapper(type&& result): _result(std::move(result)) {}

    result_wrapper(result_wrapper&&) = default;             ///< Move constructor
    result_wrapper& operator=(result_wrapper&&) = default;  ///< Move assignment

    /**
     * @brief Assigns a new result value
     *
     * @param result The result to store (moved)
     * @return Reference to this wrapper
     */
    result_wrapper& operator=(type&& result) {
        _result.emplace(std::move(result));   // destroy+construct
        return *this;
    }

    template <typename... Args>
    type& emplace(Args&&... args) {
        return _result.emplace(std::forward<Args>(args)...);
    }

    /**
     * @brief Checks if the facet has been evaluated
     *
     * @return true if a result is stored, false if unevaluated
     */
    bool ready() const { return _result.has_value(); }

    /**
     * @brief Access the stored result (const)
     *
     * @return const reference to the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    const type& value() const {
        if(!ready()) throw std::runtime_error{"trying to get result from unevaluated facet"};
        return *_result;
    }

    /**
     * @brief Access the stored result (mutable)
     *
     * @return mutable reference to the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    type& value() {
        if(!ready()) throw std::runtime_error{"trying to get result from unevaluated facet"};
        return *_result;
    }

    /**
     * @brief Implicit conversion to the result type
     *
     * @return Copy of the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    operator const type&() const { return value(); }

    /**
     * @brief Dereference operator
     *
     * @return const reference to the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    const type& operator*() const { return value(); }

    /**
     * @brief Dereference operator
     *
     * @return mutable reference to the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    type& operator*() { return value(); }

    /**
     * @brief Member access operator (const)
     *
     * @return const pointer to the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    const type* operator->() const { return &value(); }

    /**
     * @brief Member access operator (mutable)
     *
     * @return mutable pointer to the stored result
     * @throws std::runtime_error if the facet is unevaluated
     */
    type* operator->() { return &value(); }

    /**
     * @brief Boolean conversion indicating evaluation status
     *
     * @return true if the facet has been evaluated, false otherwise
     */
    operator bool() const { return ready(); }

    void clear() { _result.reset(); }

private:
    opt_type _result;
};

namespace detail{

/**
 * @brief Type-safe storage container for facet evaluation results
 *
 * The result_wrapper provides move-only storage for facet results with
 * optional semantics. It tracks whether a facet has been evaluated and
 * provides safe access to its result.
 *
 * @tparam ResultT The type of result to store (must be move-constructible)
 * @tparam Feature The feature type associated with this result
 *
 * @note Results are stored as std::optional<ResultT> to distinguish between
 *       unevaluated facets (nullopt) and evaluated facets (value present).
 *
 * @see journal
 * @see facet
 * @see feature
 */
template <typename FacetT, bool Skip = !udho::manifold::has_result<FacetT>::value>
struct result_container{
    using facet_type  = FacetT;                                                             ///< The facet type
    using result_type    = typename udho::manifold::facet_traits<FacetT>::result_type;      ///< Result type
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;     ///< Feature type
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;   ///< Component type
    using wrapper_type   = result_wrapper<result_type, feature_type>;                       ///< Wrapper type

    static constexpr const bool skipped = false;                                            ///< Indicates this container stores results

    // static_assert(std::is_default_constructible_v<wrapper_type>);
    static_assert(std::is_move_constructible_v<wrapper_type>);

    template <typename... Features>
    friend struct evaluator;

    result_container() = default;                           ///< Default constructor

    /**
     * @brief Construct from another journal (move semantics)
     *
     * Moves the result from another journal's container of the same facet type.
     *
     * @tparam OtherHeadT First facet type of source journal
     * @tparam OtherTailT Remaining facet types of source journal
     * @param other Source journal to move from
     */
    template <typename OtherHeadT, typename... OtherTail>
    inline explicit result_container(journal<OtherHeadT, OtherTail...>&& other): _result(std::move(other.template get<FacetT>())) {}


    /// @{
    /**
     * @brief Get result wrapper for a specific facet type
     *
     * @tparam FacetQ The facet type to retrieve (must match FacetT)
     * @return Reference to the result wrapper
     */
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    wrapper_type& get() { return _result; }

    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    const wrapper_type& get() const { return _result; }
    /// @}


    /// @{
    /**
     * @brief Get result wrapper by feature and index
     *
     * @tparam FeatureT The feature type to search for
     * @tparam Idx Zero-based index among containers with this feature
     * @return Reference to the result wrapper
     *
     * @note Only matches when FeatureT matches this container's feature and Idx == 0
     */
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return _result; }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return _result; }
    /// @}

    /// @{
    /**
     * @brief Count how many containers provide a specific feature
     *
     * @tparam FeatureT The feature type to count
     * @return 1 if this container provides FeatureT, 0 otherwise
     */
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature_type, FeatureT>; }
    /// @}

    void clear() { _result.clear(); }
private:
    wrapper_type _result;
};

/**
 * @brief Internal helper for facets without results (empty container)
 *
 * Specialization for facets with void result type. This container is empty
 * and provides no storage, but maintains type consistency in the journal.
 *
 * @tparam FacetT The facet type (with void result)
 */
template <typename FacetT>
struct result_container<FacetT, true>{
    using facet_type  = FacetT;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    static constexpr const bool skipped = true;
};

}


#ifndef __DOXYGEN__

template <typename FacetT, typename... Rest>
struct journal<FacetT, Rest...>: private detail::result_container<FacetT>, private journal<Rest...> {
    using component_type = FacetT;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using container_type = detail::result_container<FacetT>;

    using container_type::container_type;

    /// @{
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return container_type::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return container_type::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return journal<Rest...>::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return journal<Rest...>::template get<FacetQ>(); }
    /// @}

    /// @{
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get_wrapper() { return static_cast<container_type&>(*this); }

    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return static_cast<const container_type&>(*this); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get_wrapper() { return journal<Rest...>::template get_wrapper<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return journal<Rest...>::template get_wrapper<FacetQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    auto& at() { return container_type::template at<FeatureT>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const auto& at() const { return container_type::template at<FeatureT>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return journal<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return journal<Rest...>::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return journal<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    const auto& at() const { return journal<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return container_type::template count<FeatureT>() + journal<Rest...>::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<(count<FeatureT>()-1 > Idx), bool> = true>
    bool ready() const {
        const auto& result = at<FeatureT, Idx>();
        if(!result.ready()){
            return ready<FeatureT, Idx+1>();
        } else {
            return true;
        }
    }
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<(count<FeatureT>()-1 == Idx), bool> = true>
    bool ready() const {
        const auto& result = at<FeatureT, Idx>();
        return result.ready();
    }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<(count<FeatureT>()-1 > Idx), bool> = true>
    const auto& first_of() const {
        const auto& result = at<FeatureT, Idx>();
        if(!result.ready()){
            return first_of<FeatureT, Idx+1>();
        }
    }
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<(count<FeatureT>()-1 == Idx), bool> = true>
    const auto& first_of() const {
        const auto& result = at<FeatureT, Idx>();
        if(!result.ready()){
            throw std::out_of_range{"feature not ready"};
        }
        return result;
    }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<(count<FeatureT>()-1 > Idx), bool> = true>
    auto& first_of() {
        const auto& result = at<FeatureT, Idx>();
        if(!result.ready()){
            return first_of<FeatureT, Idx+1>();
        }
    }
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<(count<FeatureT>()-1 == Idx), bool> = true>
    auto& first_of() {
        const auto& result = at<FeatureT, Idx>();
        if(!result.ready()){
            throw std::out_of_range{"feature not ready"};
        }
        return result;
    }
    /// @}

    /**
     * @brief clear the journal
     */
    void clear() {
        container_type::clear();
        journal<Rest...>::clear();
    }

private:
    journal<Rest...>& tail() { return *this; }
    const journal<Rest...>& tail() const { return *this; }
};

template <typename FacetT>
struct journal<FacetT> : private detail::result_container<FacetT>{
    using component_type = FacetT;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using container_type = detail::result_container<FacetT>;

    using container_type::container_type;
    using container_type::get;
    using container_type::at;
    using container_type::count;
    using container_type::clear;

    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get_wrapper() { return static_cast<container_type&>(*this); }

    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return static_cast<const container_type&>(*this); }

};

#else

/**
 * @brief Compile-time indexed storage for facet evaluation results
 *
 * @tparam Facets... The list of facets whose results will be stored
 *
 * The `journal` class provides type-safe, compile-time indexed storage for
 * results produced by facets during pipeline execution. It automatically:
 *
 * 1. **Filters out facets without results**: Only facets that define a `result_type`
 *    are stored in the journal.
 * 2. **Provides multiple access patterns**: Results can be accessed by facet type,
 *    by feature with index, or through queries.
 * 3. **Ensures safe access**: Results can only be accessed when they are "ready"
 *    (have been evaluated and stored).
 * 4. **Supports recursive queries**: Methods like `ready()` and `first_of()` can
 *    recursively check multiple results.
 *
 * Only facets with results (i.e., `has_result<FacetT>::value == true`) are stored.
 * Facets without results are skipped at compile time.
 *
 * ## Access Patterns
 *
 * ### 1. By Facet Type (Direct Access)
 * ```cpp
 * auto& result = journal.get<facet<ComponentA, FeatureX>>();
 * if(result.ready()) {
 *     auto value = *result; // Access the result
 * }
 * ```
 *
 * ### 2. By Feature and Index (Ordered Access)
 * ```cpp
 * // Get the 0th component providing FeatureX
 * auto& first_x = journal.at<FeatureX, 0>();
 *
 * // Get the 1st component providing FeatureX
 * auto& second_x = journal.at<FeatureX, 1>();
 * ```
 *
 * ### 3. By Query
 * ```cpp
 * // Count how many components provide FeatureX
 * int count = journal.count<FeatureX>();
 *
 * // Check if any component with FeatureX is ready
 * bool any_ready = journal.ready<FeatureX>();
 *
 * // Get the first ready result for FeatureX
 * auto& first_ready = journal.first_of<FeatureX>();
 * ```
 *
 * ## Example Usage
 *
 * @code
 * struct FeatureA { using result = int; };
 * struct FeatureB { using result = std::string; };
 *
 * // Define facets
 * using Facet1 = facet<Component1, FeatureA>;  // Has result
 * using Facet2 = facet<Component2, FeatureB>;  // Has result
 * using Facet3 = facet<Component3, FeatureC>;  // No result
 *
 * // Create journal (Facet3 is automatically skipped)
 * journal<Facet1, Facet2, Facet3> journal;
 *
 * // Store results
 * journal.get<Facet1>() = 42;
 * journal.get<Facet2>() = "hello";
 *
 * // Access results
 * CHECK(journal.get<Facet1>().ready()); // true
 * CHECK(*journal.get<Facet1>() == 42);  // true
 *
 * // Feature-based access
 * CHECK(journal.at<FeatureA, 0>().ready()); // true
 * CHECK(journal.first_of<FeatureB>() == "hello");
 * @endcode
 *
 * @note Attempting to access an unready result throws `std::runtime_error`.
 * @note The journal is designed for use with the pipeline system and is
 *       typically managed by `common_pipeline` or `flow` objects.
 */
template <typename... Facets>
class journal {
public:
    /// @name Type Queries
    /// @{

    /**
     * @brief Count components providing a specific feature
     * @tparam FeatureT The feature type to count
     * @return Number of components in this journal that provide FeatureT
     *
     * This is a compile-time constant evaluated through template recursion.
     * Only facets with results are counted.
     */
    template <typename FeatureT>
    static constexpr int count();

    /// @}

    /// @name Result Access by Facet Type
    /// @{

    /**
     * @brief Get result wrapper for a specific facet
     * @tparam FacetQ The facet type to retrieve
     * @return Reference to the result wrapper for FacetQ
     *
     * Provides O(1) compile-time access to a specific facet's result.
     * The lookup is resolved through recursive template matching.
     *
     * @note Compile error if FacetQ is not in the journal's facet list.
     * @note If FacetQ doesn't have a result, this method is not available.
     */
    template <typename FacetQ>
    auto& get();

    /**
     * @brief Get result wrapper for a specific facet (const)
     * @tparam FacetQ The facet type to retrieve
     * @return Const reference to the result wrapper for FacetQ
     */
    template <typename FacetQ>
    const auto& get() const;

    /// @}

    /// @name Result Access by Feature
    /// @{

    /**
     * @brief Get the Idx-th result for a feature
     * @tparam FeatureT The feature type
     * @tparam Idx Zero-based index among components providing this feature
     * @return Reference to the result wrapper for the Idx-th component with FeatureT
     *
     * Components are indexed in the order they appear in the template parameter
     * list, counting only those that provide the requested feature and have results.
     *
     * @note Compile error if Idx >= count<FeatureT>().
     * @note Compile error if no component provides FeatureT.
     */
    template <typename FeatureT, std::uint32_t Idx = 0>
    auto& at();

    /**
     * @brief Get the Idx-th result for a feature (const)
     * @tparam FeatureT The feature type
     * @tparam Idx Zero-based index
     * @return Const reference to the result wrapper
     */
    template <typename FeatureT, std::uint32_t Idx = 0>
    const auto& at() const;

    /// @}

    /// @name Result State Queries
    /// @{

    /**
     * @brief Check if any component with a feature has a ready result
     * @tparam FeatureT The feature type to check
     * @tparam Idx Starting index (usually 0)
     * @return true if any component providing FeatureT has a ready result
     *
     * Recursively checks components in index order until a ready result is found
     * or all components are checked.
     */
    template <typename FeatureT, std::uint32_t Idx = 0>
    bool ready() const;

    /**
     * @brief Get the first ready result for a feature
     * @tparam FeatureT The feature type
     * @tparam Idx Starting index (usually 0)
     * @return Reference to the first ready result wrapper
     * @throws std::out_of_range if no component with FeatureT has a ready result
     *
     * Recursively searches components in index order for the first ready result.
     */
    template <typename FeatureT, std::uint32_t Idx = 0>
    const auto& first_of() const;

    /**
     * @brief Get the first ready result for a feature (mutable)
     * @tparam FeatureT The feature type
     * @tparam Idx Starting index (usually 0)
     * @return Reference to the first ready result wrapper
     * @throws std::out_of_range if no component with FeatureT has a ready result
     */
    template <typename FeatureT, std::uint32_t Idx = 0>
    auto& first_of();

    /// @}

    /**
     * @brief clear the journal
     */
    void clear();
};

#endif // __DOXYGEN__

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_JOURNAL_H
