#ifndef UDHO_MANIFOLD_FWD_H
#define UDHO_MANIFOLD_FWD_H

#include <cstdint>

/** 
 * @defgroup manifold Manifold
 * @brief Manifold Subsystem.
 * @{
 */
/** @} */

namespace udho {
namespace manifold {

#ifndef __DOXYGEN__

template <typename...>
struct composition;

template <typename...>
struct params;

template <std::size_t, typename...>
struct fabric;

template <typename...>
struct journal;

// template <std::size_t, typename...>
// struct evaluator;

// template <std::size_t, typename...>
// struct pipeline;

template <typename ComponentT>
struct wrapper;

namespace detail{

template <typename ComponentT, bool>
struct facet_wrapper;

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT, bool YieldsResult>
class next_evaluator_helper_internal;

/**
 * @brief Encapsulates the stage and order of features for facet evaluation
 *
 * This template class provides the infrastructure for evaluating facets in a
 * specific order based on their features. It handles the recursive evaluation
 * of facets within and across features.
 *
 * @tparam Stage Filter facets by the stage
 * @tparam Features... The ordered set of features by which the facets will be evaluated
 *
 * @note The evaluation order is determined by the Features... template parameter
 *       list. Facets are evaluated feature-by-feature, and within each feature,
 *       they are evaluated in the order they appear in the composition.
 */
template <std::size_t, typename...>
struct evaluator_helper;

}

#endif // __DOXYGEN__

/**
 * @brief Implements the evaluation logic for a component-feature pair
 * @ingroup manifold
 * Facets are the units of pipeline evaluation, each representing one
 * component providing one feature, and defines how that feature is
 * provided for that component.
 *
 * @tparam ComponentT The component providing the feature
 * @tparam FeatureT The feature being provided
 *
 * # Specialization Required
 *
 * Usercode must specialize facet for each component-feature pair:
 *
 * @code
 * template <>
 * struct facet<AuthComponent, feature::filter> {
 *     using component_type = AuthComponent;
 *     using config_type    = udho::manifold::config<component_type>;
 *
 *     facet(component_type& component, const config& conf): _component(component), _config(conf) {}
 *
 *     template <typename JournalT, typename NextT, typename... Args>
 *     void operator()(const JournalT& journal, NextT&& next, Args&&... args) const {
 *         ...
 *         feature::filter::result res; // construct result of evaluation
 *         next.pass(std::move(res));
 *         ...
 *         next.pass(); // if the feture doesn't have any result
 *         ...
 *         next.fail(); // if the evaluation fails
 *     }
 *     private:
 *         component_type&    _component;
 *         const config_type& _config;
 *
 * };
 * @endcode
 *
 * # Evaluation Signature
 *
 * The operator() receives:
 * - **journal**: Read-only access to results from previous facets
 * - **next**: Continuation object for control flow (pass/fail/skip)
 * - **args**: Arguments forwarded from pipeline::eval()
 *
 * # Control Flow
 *
 * Facets signal completion via the next object:
 *
 * @code
 * void operator()(const JournalT& journal, NextT&& next, Args&&... args) const {
 *     // use journal to get the results from prebious facets evaluation in the pipeline
 *     if (success) {
 *         result r{true};
 *         next.pass(std::move(r));  // Continue pipeline
 *     } else {
 *         result r{false};
 *         next.fail();  // Stop pipeline
 *     }
 * }
 * @endcode
 *
 * # Result Handling
 *
 * - **Facets with results** (feature defines `result` type):
 *   - Must call `next.pass(result)` or `next.fail(result)`
 *   - Results are stored in journal before continuation
 *
 * - **Facets without results** (no `result` type):
 *   - Call `next.pass()` or `next.fail()`
 *   - No journal entry created
 *
 * # Journal Access
 *
 * Facets can access results from earlier facets:
 *
 * @code
 * void operator()(const JournalT& journal, NextT&& next, ...) {
 *     // Access result by facet type
 *     auto& auth_result = journal.get<facet<AuthComp, FeatureX>>();
 *     if (auth_result.ready() && auth_result->success()) {
 *         // ...
 *     }
 *
 *     // Access result by feature and index
 *     auto& first_filter = journal.at<feature::filter, 0>();
 * }
 * @endcode
 *
 * # Exception Handling
 *
 * Exceptions thrown during facet evaluation are caught by the pipeline:
 * - Exception is captured as std::exception_ptr
 * - Completion callback receives exception
 * - Pipeline terminates
 *
 * @warning However, any exception thrown asynchronously may not be caught.
 *          Such exceptions should be caught and handled by the fabric itself
 *          and moved to the fail() function
 *
 * @code
 * void operator()(...) {
 *     if (error_condition) {
 *         throw std::runtime_error("Something went wrong");
 *         // Pipeline catches, stores exception, terminates
 *     }
 * }
 * @endcode
 *
 * # Best Practices
 *
 * 1. **Keep facets focused**: One concern per facet
 * 2. **Prefer composition**: Use journal to share data between facets
 * 3. **Handle errors gracefully**: Use fail() rather than exceptions when possible
 * 4. **Document requirements**: Specify which journal entries are required
 * 5. **Test independently**: Facets are unit-testable
 *
 * @see fabric
 * @see journal
 * @see evaluator_helper
 */
template <typename ComponentT, typename FeatureT>
struct facet;

#ifndef __DOXYGEN__

template <typename ComponentT>
struct config;

template <typename...>
struct configs;

template <typename... Components>
struct composition_view;

template <typename... Components>
struct configs_view;

template <typename...Facets>
struct journal_const_view;

/**
 * @brief Common pipeline implementation with feature ordering
 *
 *
 * Extends basic_pipeline to provide ordered feature evaluation within a stage.
 * Manages evaluation callbacks and provides a fluent interface for chaining
 * completion handlers.
 *
 * @tparam Stage The pipeline stage index
 * @tparam OrderT The order<Features...> specifying feature evaluation order
 * @tparam CompositionT The composition type being evaluated
 */
template <std::size_t Stage, typename OrderT, typename CompositionT>
class common_pipepine;

template <typename LabelT, typename StreamT>
struct basic_runtime;

/**
 * @brief Flow label for pipeline type identification
 * Empty struct used as a tag to identify and specialize pipeline
 * configurations. Each unique flow type should have its own label.
 *
 * @tparam LabelT The label type (typically an empty struct)
 */
template <typename LabelT, typename StreamT>
struct basic_flow;

template <typename LabelT, typename StreamT>
struct basic_terminal;

#endif // __DOXYGEN__

/**
 * @brief Pipeline execution plan blueprint
 * @ingroup manifold
 * Provides the static configuration for a pipeline type, defining:
 * - The component composition
 * - Feature evaluation order
 *
 * Must be specialized for each pipeline label with the appropriate
 * type definitions.
 *
 * @code
 * namespace testing{
 *     struct Label;
 * }
 * template <>
 * struct sketch<testing::Label> {
 *     using composition_type = composition<
 *         testing::C00,
 *         testing::C01,
 *         testing::C10,
 *         ...
 *     >;
 *     using order_type = order<
 *         testing::F00,
 *         testing::F01,
 *         testing::F10,
 *         ...
 *     >;
 * };
 * @endcode
 *
 * @tparam LabelT The label type identifying this pipeline configuration
 */
template <typename LabelT>
struct sketch;

#ifndef __DOXYGEN__

template <typename... Components>
struct portal;

template <typename StreamT, typename... Components>
struct basic_context;

#endif // #ifndef __DOXYGEN__

}
}


#endif // UDHO_MANIFOLD_FWD_H
