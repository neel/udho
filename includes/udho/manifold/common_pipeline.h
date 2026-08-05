#ifndef UDHO_MANIFOLD_COMMON_PIPELINE_H
#define UDHO_MANIFOLD_COMMON_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/order.h>
#include <udho/manifold/basic_pipeline.h>

namespace udho{
namespace manifold{

/**
 * @ingroup DoxyG_manifold
 * @{
 */

/**
 * @brief A complete pipeline stage that evaluates features in a specified order with callback support
 *
 * The `common_pipepine` class represents a single stage in a multi-stage pipeline processing system.
 * It orchestrates the evaluation of a sequence of features against a composition of components,
 * managing the flow of control through success/failure callbacks and providing integration
 * with asynchronous operations via Boost.Asio.
 *
 * @tparam Stage The pipeline stage index (0, 1, 2, ...) this instance represents
 * @tparam Features... The ordered sequence of features to evaluate in this stage
 * @tparam Components... The components in the composition that provide the features
 *
 * # Design Philosophy
 *
 *
 * # Stage Evaluation Model
 *
 * When `eval()` is called:
 * 1. Features are evaluated in the order specified by `order<Features...>`.
 * 2. Each feature triggers evaluation of all facets that provide it.
 * 3. Results are written into the provided journal.
 * 4. On completion, the currently registered completion callback (if any) is invoked.
 *
 * @note This class does **not** automatically clear the user callback after completion.
 *       The callback remains installed until it is overwritten by another call to `then(...)`.
 *
 * @note When used by @ref udho::manifold::pipeline, the pipeline installs its own stage
 *       transition callback via `then(...)` for each stage invocation, which overwrites
 *       any previously registered user callback for that stage instance.
 *
 * # Asynchronous Support
 *
 * Two callback models are supported:
 * - **Synchronous**: Callback executes immediately in the evaluating thread
 * - **Asynchronous**: Callback is posted to a Boost.Asio io_context for deferred execution
 *
 * # Example Usage
 *
 * @code
 * using composition_type = composition<ComponentA, ComponentB, ComponentC>;
 * using order_type = order<FeatureX, FeatureY, FeatureZ>;
 *
 * composition_type comp = ...;
 * configs_type configs = ...;
 * journal_type journal;
 *
 * // Create stage 1 pipeline with FeatureX, FeatureY, FeatureZ
 * common_pipepine<1, order_type, composition_type> pipeline(comp, configs, journal);
 *
 * // Set completion callback and execute
 * pipeline.then([](auto&& result) {
 *     if (std::holds_alternative<bool>(result)) {
 *         if (std::get<bool>(result)) {
 *             std::cout << "Stage 1 passed\n";
 *         } else {
 *             std::cout << "Stage 1 failed\n";
 *         }
 *     } else {
 *         std::cout << "Stage 1 threw exception\n";
 *     }
 * }).eval(request, context);
 * @endcode
 *
 * # Thread Safety
 *
 * This class is not thread-safe by itself. Concurrent calls to `eval()` or `then()`
 * on the same instance without external synchronization may result in data races.
 *
 * @see basic_pipeline
 * @see evaluator_helper
 * @see pipeline
 * @see flow
 */
template <std::size_t Stage, typename... Features, typename... Components>
struct common_pipepine<Stage, order<Features...>, udho::manifold::composition<Components...>>: private basic_pipeline<Stage, Components...> {
    using composition_type    = udho::manifold::composition<Components...>;
    using basic_pipeline_type = basic_pipeline<Stage, Components...>;
    using evaluator_type      = typename basic_pipeline_type::template evaluator<Features...>;
    using safe_success_type   = typename evaluator_type::safe_success_type;
    using async_callback_type = typename evaluator_type::async_callback_type;

public:

    /**
     * @brief Configuration container for all components in the composition
     *
     * Provides type-safe access to configuration parameters for each component.
     * Each component's configuration is accessible via `get<ComponentT>()`.
     */
    using configs_type        = udho::manifold::configs<Components...>;

    /**
     * @brief Journal type capable of storing results from all facets in the full fabric
     *
     * Contains storage for results from all facets across all stages, not just
     * the current stage. This allows later stages to access results from earlier stages.
     */
    using full_journal_type   = typename basic_pipeline_type::full_journal_type;

public:

    /**
     * @brief Constructs a pipeline stage
     *
     * Initializes the pipeline with a composition, its configurations, and a journal
     * for result storage. The journal is shared across all pipeline stages to allow
     * data flow between stages.
     *
     * @param composition Reference to the composition of components to evaluate
     * @param configs Configuration parameters for all components in the composition
     * @param journal Journal for storing facet evaluation results (shared across stages)
     * @param id Flow identifier
     *
     * @note The journal must outlive this pipeline instance, as it stores results
     *       that may be accessed by subsequent pipeline stages or user code.
     */
    common_pipepine(composition_type& composition, const configs_type& configs, full_journal_type& journal, std::size_t id):
        basic_pipeline_type(composition, configs, id),
        _callback(std::bind(&common_pipepine::on_completion, this, std::placeholders::_1)),
        _evaluator(basic_pipeline_type::fabric(), journal, _callback)
    {}

    common_pipepine(const common_pipepine&) = delete;

    common_pipepine(common_pipepine&&) = delete;

    // common_pipepine(common_pipepine&& other)
    //     : basic_pipeline_type(std::forward<common_pipepine>(other))
    //     , _callback(std::move(other._callback))
    //     , _user_callback(std::move(other._user_callback))
    //     , _evaluator(std::move(other._evaluator))
    // {}

    /**
     * @name Fabric Access
     * Access to the underlying fabric for direct facet manipulation
     * @{
     */
    using basic_pipeline_type::fabric;
    /// @}

    /**
     * @brief Evaluates the pipeline stage with provided arguments
     *
     * Initiates evaluation of all features in the stage's order. Each feature
     * triggers evaluation of components that provide it, with results stored
     * in the journal.
     *
     * @tparam Args Types of arguments to forward to facet evaluation
     * @param args Arguments to pass to each facet's evaluation function
     *
     * # Execution Flow
     *
     * 1. Features are evaluated in `order<Features...>` sequence
     * 2. For each feature, components are evaluated in composition order
     * 3. Each facet may call `pass()` (continue), `fail()` (stop), or `skip()` (bypass)
     * 4. On completion, registered callback is invoked once and cleared
     *
     * @pre A callback must be registered via `then()` before calling `eval()`
     * @note If no callback is registered, completion results are discarded.
     *
     * @note The callback is not cleared automatically after completion. Repeated calls to `eval()`
     *       will reuse the last callback registered via `then(...)`, unless overwritten.
     *
     * @note When invoked through @ref udho::manifold::pipeline, the stage callback is always set
     *       by the pipeline before calling `eval()`, and therefore user-installed callbacks on
     *       the same instance will be overwritten for that invocation.
     *
     * @see then()
     */
    template <typename... Args>
    void eval(Args&&... args){
        _evaluator.eval(std::forward<Args>(args)...);
    }

    /**
     * @brief Registers a synchronous completion callback
     *
     * Sets a callback to be invoked when pipeline evaluation completes (successfully
     * or with failure). The callback executes in the same thread that called `eval()`.
     *
     * @param callback Function to call on completion
     * @return *this for method chaining
     *
     * # Callback Signature
     *
     * The callback must accept a `safe_success_type` parameter, which is a typedef of
     * `udho::manifold::exclusive_result`:
     *
     * # Callback Lifecycle
     *
     * The callback is moved into internal storage and invoked when stage evaluation completes.
     * It remains installed until overwritten by another call to `then(...)`.
     *
     * @code
     * pipeline.then([](auto&& result) {
     *     if (result) {
     *          std::cout << "Success\n";
     *     } else {
     *         if(result.has_exception()) {
     *             std::cout << "Exception\n";
     *             std::rethrow_exception(std::get<std::exception_ptr>(result));
     *         } else {
     *             std::cout << "Failure\n";
     *         }
     *     }
     * }).eval(...);
     * @endcode
     */
    common_pipepine& then(async_callback_type&& callback){
        _user_callback = std::move(callback);
        return *this;
    }

    /**
     * @brief Registers an asynchronous completion callback
     *
     * Sets a callback to be invoked asynchronously via Boost.Asio when pipeline
     * evaluation completes. The callback is posted to the specified `io_context`
     * for deferred execution.
     *
     * @param io Boost Asio io_context to post completion to
     * @param callback Function to call on completion (executed in io_context thread)
     * @return *this for method chaining
     *
     * # Asynchronous Execution
     *
     * The callback is wrapped and posted via `boost::asio::post(io, ...)`, ensuring
     * execution occurs in an io_context handler thread.
     *
     * # Usage Pattern
     *
     * @code
     * boost::asio::io_context io;
     *
     * pipeline.then(io, [&](auto&& result) {
     *     // Executes in io_context thread
     *     if (result) {
     *          std::cout << "Success\n";
     *     } else {
     *         if(result.has_exception()) {
     *             std::cout << "Exception\n";
     *             std::rethrow_exception(std::get<std::exception_ptr>(result));
     *         } else {
     *             std::cout << "Failure\n";
     *         }
     *     }
     * }).eval(request);
     *
     * io.run();  // Process completion callback
     * @endcode
     *
     * @note The io_context reference must remain valid until callback execution
     */
    common_pipepine& then(boost::asio::io_context& io, async_callback_type&& callback){
        _user_callback = [&io, callback = std::move(callback)](safe_success_type&& success){
            boost::asio::post(io, std::bind(std::move(callback), std::move(success)));
        };
        return *this;
    }

    // const basic_pipeline_type& basic() const { return *this; }
private:
    void on_completion(safe_success_type&& success){
        // std::cout << "common_pipeline<" << Stage << "," << (std::string(Features::name) + "," + ... ) << components_name<Components...>::get() << ">::on_completion(success_callback)" << std::endl;
        if(_user_callback) {
            // Will be called in case of failure
            _user_callback(std::forward<safe_success_type>(success));
            // _user_callback = nullptr;
        }
    }

private:
    async_callback_type      _callback;
    async_callback_type      _user_callback;
    evaluator_type           _evaluator;
};

/**
 * @}
 */

}
}

#endif // UDHO_MANIFOLD_COMMON_PIPELINE_H
