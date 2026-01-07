#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/evaluator.h>


namespace udho {
namespace manifold {

/**
 * @addtogroup manifold
 * @{
 */


/**
 * @brief Basic pipeline implementation for a single stage
 *
 * Provides the core evaluation infrastructure for a specific pipeline stage,
 * managing the fabric of components and their evaluation order.
 *
 * @tparam Stage The pipeline stage index
 * @tparam Components... The component types available in this composition
 */
template <std::size_t Stage, typename... Components>
struct basic_pipeline{
    using fabric_type       = typename udho::manifold::detail::flatten_all<Stage, Components...>::type;
    using full_journal_type = typename detail::get_journal_for_all_components<Components...>::type;

    /**
     * @brief Constructs a basic pipeline for a composition
     *
     * @tparam XComponents... Component types in the provided composition
     * @param composition The component composition to evaluate
     * @param configs Configuration for all components
     */
    template <typename... XComponents>
    basic_pipeline(udho::manifold::composition<XComponents...>& composition, const udho::manifold::configs<XComponents...>& configs, std::size_t id): _fabric(composition, configs, id) {}

    /**
     * @brief Evaluator for executing features in a specific order
     *
     * Provides type-safe evaluation of features in the specified order,
     * managing the flow control between facets and handling completion callbacks.
     *
     * @tparam Features... The feature types to evaluate in order
     */
    template <typename... Features>
    struct evaluator{
        using helper_type         = detail::evaluator_helper<Stage, Features...>;
        using handler_type        = typename detail::get_handler_type<full_journal_type, fabric_type>::template for_features<Features...>;
        using safe_success_type   = typename handler_type::safe_success_type;
        using async_callback_type = typename handler_type::async_callback_type;

        /**
         * @brief Constructs an evaluator for the pipeline stage
         *
         * @param fabric The fabric containing all facets for this stage
         * @param journal Journal for storing facet results
         * @param callback Completion callback for pipeline stage
         */
        evaluator(fabric_type& fabric, full_journal_type& journal, async_callback_type& callback): _handler(fabric, journal, callback) {}

        /**
         * @brief Evaluates the pipeline stage with the given arguments
         *
         * Initiates evaluation of all features in the specified order,
         * forwarding arguments to each facet.
         *
         * @tparam Args... Argument types to forward to facets
         * @param args Arguments to forward to facets
         */
        template <typename... Args>
        void eval(Args&&... args){
            _handler.template operator()<0>(std::forward<Args>(args)...);
        }

        private:
        handler_type _handler;
    };

    /// @name Fabric Access
    /// @{

    /**
     * @brief Gets the fabric for this pipeline stage
     * @return Reference to the stage's fabric
     */
    fabric_type& fabric() { return _fabric; }

    /**
     * @brief Gets the fabric for this pipeline stage (const)
     * @return Const reference to the stage's fabric
     */
    const fabric_type& fabric() const { return _fabric; }
    /// @}

    private:
    fabric_type  _fabric;

};

/**
 * @brief Defines the evaluation order of features within a stage
 *
 * The order template specifies the sequence in which features should be
 * evaluated within a pipeline stage. Features are evaluated in the order
 * they appear in the template parameter list.
 *
 * @tparam Features... The feature types in evaluation order
 */
template <typename... Features>
struct order{};

/**
 * @brief Common pipeline implementation with feature ordering
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
class common_pipepine<Stage, order<Features...>, udho::manifold::composition<Components...>>: private basic_pipeline<Stage, Components...> {
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
     *
     * @note The journal must outlive this pipeline instance, as it stores results
     *       that may be accessed by subsequent pipeline stages or user code.
     */
    common_pipepine(composition_type& composition, const configs_type& configs, full_journal_type& journal, std::size_t id):
        basic_pipeline_type(composition, configs, id),
        _callback(std::bind(&common_pipepine::on_completion, this, std::placeholders::_1)),
        _evaluator(basic_pipeline_type::fabric(), journal, _callback)
    {}
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
private:
    void on_completion(safe_success_type&& success){
        std::cout << "common_pipeline<" << Stage << "," << (std::string(Features::name) + "," + ... ) << "," << components_name<Components...>::get() << ">::on_completion(success_callback)" << std::endl;
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
 * @brief Flow label for pipeline type identification
 *
 * Empty struct used as a tag to identify and specialize pipeline
 * configurations. Each unique flow type should have its own label.
 *
 * @tparam LabelT The label type (typically an empty struct)
 */
template <typename LabelT>
struct flow;

template <typename LabelT>
struct terminal;

/**
 * @brief Complete multi-stage pipeline implementation
 *
 * Manages the complete execution flow across multiple pipeline stages,
 * handling stage transitions, configuration propagation, and flow termination.
 * Provides type-safe access to pipelines at individual stages and manages the
 * shared configuration state across all stages.
 *
 * @note Generally constructed by the pipeline<CompositionT, OrderT, Count, -1> pipeline.
 *       However, it can can be constructed directly for testing.
 *
 * ## Configuration (borrowed)
 *
 * Expects reference of the configuration owned by the root pipeline (Stage -1) to be
 * passed during construction which is not copied but stored as reference only, so that
 * each @ref udho::manifold::flow "flow" can have its own copy of configs, while allowing
 * it to be reconfigured by usercode specialization during stage transition.
 *
 * ## Composition (borrowed)
 *
 * Holds a non-const reference to the composition passed by the previous pipeline starting from
 * the Stage -1 pipeline.
 *
 * ## Journal (borrowed)
 *
 * Not directly borrowed, but passed to the common_pipeline and the subsequent pipelines which borrows
 * the journal indirectly through a non-const reference.
 *
 * ## Next pipelines (owned)
 *
 * Owns the subsequent pipelines starting from Stage+1 to Count.
 *
 * ## Previous pipeline
 *
 * Keeps a non-const reference to the previous pipeline
 *
 * @tparam CompositionT The component composition type
 * @tparam OrderT The feature order type (order<Features...>)
 * @tparam Count The total number of pipeline stages
 * @tparam Stage The current stage index (-1 for start, Count for finish)
 */
template <typename CompositionT, typename OrderT, std::size_t Count, int Stage>
struct pipeline{
    static_assert (Stage < Count);
    using composition_type     = CompositionT;
    using order_type           = OrderT;
    using common_pipeline_type = udho::manifold::common_pipepine<Stage, order_type, composition_type>;
    using configs_type         = typename common_pipeline_type::configs_type;
    using prev_pipeline_type   = pipeline<composition_type, order_type, Count, Stage-1>;
    using next_pipeline_type   = pipeline<composition_type, order_type, Count, Stage+1>;

    const int stage = Stage;
    const int count = Count;

    friend class pipeline<composition_type, order_type, Count, Stage-1>;

    /**
     * @brief Constructs a pipeline stage
     *
     * @tparam JournalT Journal type for storing facet results
     * @param composition The component composition
     * @param configs Configuration shared across all stages
     * @param journal Journal for storing facet results
     * @param previous Reference to the previous pipeline stage
     */
    template <typename JournalT>
    pipeline(composition_type& composition, typename composition_type::configs_type& configs, JournalT& journal, prev_pipeline_type& previous, std::size_t id)
        : _composition(composition), _configs(configs), _common_pipeline(composition, _configs, journal, id), _next(composition, configs, journal, *this, id), _previous(previous)
    {}

    /// @name Configuration Access
    /// @{

    /**
     * @brief Gets the shared configuration for this pipeline
     * @return Reference to the shared configuration
     */
    configs_type& configs() { return _configs; }

    /**
     * @brief Gets the shared configuration for this pipeline (const)
     * @return Const reference to the shared configuration
     */
    const configs_type& configs() const { return _configs; }
    /// @}

    /**
     * @brief Executes the pipeline stage synchronously
     *
     * Evaluates the current pipeline stage with the provided arguments.
     * Sets up completion callbacks to transition to the next stage or
     * terminate the flow on failure.
     *
     * @tparam FlowT The flow type managing this execution
     * @tparam Args... Argument types to forward to facets
     * @param flow Shared pointer to the flow managing this execution
     * @param args Arguments to forward to facets
     *
     * @note Non-lvalue arguments must be CopyConstructible to ensure
     *       safe forwarding between pipeline stages.
     */
    template <typename FlowT, typename... Args>
    void operator()(std::shared_ptr<FlowT> flow, Args&&... args){
        static_assert((... && (std::is_lvalue_reference<Args>::value || std::is_copy_constructible<std::decay_t<Args>>::value)), "Non-lvalue arguments must be CopyConstructible");

        std::cout << "pipeline<" << udho::manifold::composition_name<CompositionT>::get() << ",OrderT," << Count << "," << Stage << ">";
        std::cout << "::operator()(flow, ...)" << std::endl;

        using args_tuple_t = std::tuple<std::conditional_t<std::is_lvalue_reference<Args>::value, Args, std::decay_t<Args>>...>;
        args_tuple_t args_tuple(std::forward<Args>(args)...);

        _then(flow, args_tuple);
        _common_pipeline.eval(std::forward<Args>(args)...);
    }

    /**
     * @brief Executes the pipeline stage asynchronously
     *
     * Similar to the synchronous version but integrates with Boost.Asio
     * for asynchronous completion handling.
     *
     * @tparam FlowT The flow type managing this execution
     * @tparam Args... Argument types to forward to facets
     * @param flow Shared pointer to the flow managing this execution
     * @param io Boost.Asio io_context for async operations
     * @param args Arguments to forward to facets
     */
    template <typename FlowT, typename... Args>
    void operator()(boost::asio::io_context& io, std::shared_ptr<FlowT> flow, Args&&... args){
        static_assert((... && (std::is_lvalue_reference<Args>::value || std::is_copy_constructible<std::decay_t<Args>>::value)), "Non-lvalue arguments must be CopyConstructible");

        std::cout << "pipeline<" << udho::manifold::composition_name<CompositionT>::get() << ",OrderT," << Count << "," << Stage << ">";
        std::cout << "::operator()(io, flow, ...)" << std::endl;

        using args_tuple_t = std::tuple<boost::asio::io_context&, std::conditional_t<std::is_lvalue_reference<Args>::value, Args, std::decay_t<Args>>...>;
        args_tuple_t args_tuple(io, std::forward<Args>(args)...);

        _then(flow, args_tuple);
        _common_pipeline.eval(std::forward<Args>(args)...);
    }

    /// @name Stage Access
    /// @{

    /**
     * @brief Gets a reference to a specific pipeline stage
     *
     * Provides type-safe access to any pipeline stage by index.
     *
     * @tparam N The stage index to access
     * @return Reference to the requested pipeline stage
     */
    template <int N, std::enable_if_t<(N == Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return *this; }

    template <int N, std::enable_if_t<(N == Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return *this; }

    template <int N, std::enable_if_t<(N > Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return _next.template at<N>(); }

    template <int N, std::enable_if_t<(N > Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _next.template at<N>(); }

    template <int N, std::enable_if_t<(N < Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return _previous.template at<N>(); }

    template <int N, std::enable_if_t<(N < Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _previous.template at<N>(); }
    /// @}

private:

    /**
     * @brief Internal method to set up stage transition logic
     *
     * Configures the completion callback for the current stage to either
     * transition to the next stage or terminate the flow on failure.
     *
     * @tparam FlowT The flow type
     * @tparam ArgsTupleT Tuple type capturing forwarded arguments
     * @param flow Shared pointer to the flow
     * @param args_tuple Tuple containing arguments to forward
     */
    template <typename FlowT, typename ArgsTupleT>
    void _then(std::shared_ptr<FlowT> flow, ArgsTupleT& args_tuple){
        std::cout << "pipeline<" << udho::manifold::composition_name<CompositionT>::get() << ",OrderT," << Count << "," << Stage << ">::_then(flow, args_tuple)" << std::endl;
        auto lambda = [flow, this, args_tuple](udho::manifold::exclusive_result success){
            if(success) {
                try{
                    flow->apply(*this, configs());                  // Stage transition -> patch configs
                } catch(...) {
                    udho::manifold::exclusive_result result(std::current_exception());
                    bool reenter = std::apply(
                        [&](auto&... args) -> bool {
                            return flow->error(std::move(result), args...);    // inform flow before termination
                        },
                        args_tuple
                    );
                    (void)reenter;
                }
                std::cout << "_next(flow, ...)" << std::endl;
                std::apply(
                    [&](auto&... args) {
                        _next(flow, args...);
                    },
                    args_tuple
                );
            } else {                                            // error occured
                std::cout << "FAIL!!" << __LINE__ << std::endl;
                bool reenter = std::apply(
                    [&](auto&... args) -> bool {
                        return flow->error(success, args...);    // inform flow before termination
                    },
                    args_tuple
                );
                (void)reenter;                                   // flow->error takes care of it.
            }
        };
        _common_pipeline.then(std::move(lambda));
    }

private:
    composition_type&    _composition;
    prev_pipeline_type&  _previous;
    configs_type&        _configs;
    common_pipeline_type _common_pipeline;
    next_pipeline_type   _next;
};

/**
 * @brief Start pipeline stage (stage index -1)
 *
 * Specialization that serves as the entry point for pipeline execution.
 * Manages the initial journal and provides access to pipeline at each stage.
 *
 * @note generally constructed by the flow, can be constructed directly for testing.
 *
 * ## Composition (borrowed)
 *
 * Composition is not owned by the Stage -1 pipeline. Rather it is expected that the
 * caller ensures that the composition outlives the Stage -1 pipeline. The Stage -1
 * pipeline and subsequent pipelines only hold a non-const reference to the composition.
 *
 * ## Journal (owned)
 *
 * The Stage -1 pipeline owns journal for the full fabric spanning from Stage 0 pipeline
 * till Stage Count pipeline. This journal is passed by reference to the subsequent pipelines.
 *
 * ## Configurattions (owned)
 *
 * @ref udho::manifold::flow "Flow" passes a reference to the baseline configs
 * obtained from the runtime which is copied into a member variable. A reference
 * to that configs is passed to the next pipeline. Subsequent configs hold reference
 * of the configs only.
 *
 * Each @ref udho::manifold::flow "flow" has its own copy of configs, owned by stage
 * -1 pipeline shared accross all other stage > -1 pipelines through mutable references
 * which could be patched by the usercode during stage transition. Patching of that
 * reference would impact pipelines of all stages.
 *
 * ## Next pipelines (owned)
 *
 * The Stage -1 pipeline owns the subsequent pipelines starting from Stage 0 to Stage Count.
 *
 * @tparam CompositionT The component composition type
 * @tparam OrderT The feature order type
 * @tparam Count The total number of pipeline stages
 */
template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline<CompositionT, OrderT, Count, -1> {
    using composition_type     = CompositionT;
    using configs_type         = typename composition_type::configs_type;
    using order_type           = OrderT;
    using full_journal_type    = typename detail::get_journal_for_full_fabric<CompositionT>::type;
    using next_pipeline_type   = pipeline<composition_type, order_type, Count, 0>;
    using self_type            = pipeline<CompositionT, OrderT, Count, -1>;
    using ptr                  = std::shared_ptr<self_type>;

    /**
     * @brief Constructs the start pipeline stage
     *
     * @param composition The component composition
     * @param baseline Baseline configuration shared across all stages
     */
    pipeline(CompositionT& composition, configs_type& baseline, std::size_t id): _composition(composition), _configs(baseline), _next(composition, _configs, _journal, *this, id) {}

    /// @name Configuration Access
    /// @{
    configs_type& configs() { return _configs; }
    const configs_type& configs() const { return _configs; }
    /// @}


    /// @name Composition access
    /// @{
    composition_type& composition() { return _composition; }
    const composition_type& composition() const { return _composition; }
    /// @}

    /// @name Journal Access
    /// @{
    /**
     * @brief Gets the journal containing results from all stages
     * @return Const reference to the complete journal
     */
    const full_journal_type& journal() const { return _journal; }
    /// @}

    /**
     * @brief Starts pipeline execution
     *
     * Forwards execution to the first actual pipeline stage (stage 0).
     *
     * @tparam Args... Argument types to forward
     * @param args Arguments to forward to the first stage
     */
    template <typename... Args>
    void operator()(Args&&... args){
        _next(std::forward<Args>(args)...);
    }

    void prepare_reentry(const configs_type& baseline, std::size_t last_id) {
        _configs = baseline;
        _journal.clear();
    }

    /// @name Stage Access
    /// @{
    template <int N, std::enable_if_t<(N > -1), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return _next.template at<N>(); }

    template <int N, std::enable_if_t<(N > -1), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _next.template at<N>(); }

    template <int N, std::enable_if_t<(N == -1), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return *this; }

    template <int N, std::enable_if_t<(N == -1), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return *this; }
    /// @}
private:
    composition_type&   _composition;
    configs_type        _configs;
    full_journal_type   _journal;
    next_pipeline_type  _next;
};

/**
 * @brief Finish pipeline stage (stage index Count)
 *
 * Specialization that serves as the terminal point for pipeline execution.
 * Terminates the flow when reached, indicating successful completion of
 * all pipeline stages.
 *
 * @tparam CompositionT The component composition type
 * @tparam OrderT The feature order type
 * @tparam Count The total number of pipeline stages
 */
template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline<CompositionT, OrderT, Count, static_cast<int>(Count)>{
    using composition_type   = CompositionT;
    using order_type         = OrderT;
    using prev_pipeline_type = pipeline<CompositionT, OrderT, Count, Count-1>;
    using configs_type       = typename prev_pipeline_type::configs_type;

    /**
     * @brief Constructs the finish pipeline stage
     *
     * @param composition The component composition (unused)
     * @param configs Configuration reference
     * @param journal Journal (unused)
     * @param previous Reference to the previous pipeline stage
     */
    template <typename JournalT>
    pipeline(CompositionT&, typename CompositionT::configs_type& configs, JournalT&, prev_pipeline_type& previous, std::size_t id): _configs(configs), _previous(previous), _id(id) {}

    /// @name Configuration Access
    /// @{
    configs_type& configs() { return _configs; }
    const configs_type& configs() const { return _configs; }
    /// @}

    /**
     * @brief Terminates the flow with success
     *
     * Called when all pipeline stages have completed successfully.
     * Notifies the flow of successful termination.
     *
     * @tparam FlowT The flow type
     * @tparam Args... Argument types (unused)
     * @param flow Shared pointer to the flow
     * @param args Arguments (ignored)
     */
    template <typename FlowT, typename... Args>
    void operator()(std::shared_ptr<FlowT> flow, Args&&... args){
        using label_type    = typename FlowT::label_type;

        std::cout << "pipeline<" << udho::manifold::composition_name<CompositionT>::get() << ",OrderT," << Count << "," << Count << ">";
        std::cout << "::operator()(io, flow, ...)" << std::endl;

        if(flow->reenter(std::forward<Args>(args)...)){
            restart(flow, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief restart the flow
     * @param flow
     * @param args
     */
    template <typename FlowT, typename... Args>
    void restart(std::shared_ptr<FlowT> flow, Args&&... args) {
        using start_pipeline_type = pipeline<CompositionT, OrderT, Count, -1>;

        start_pipeline_type& start = _previous.template at<-1>();
        start.prepare_reentry(flow->baseline(), flow->id());
        flow->prepare(std::forward<Args>(args)...);
        start(flow, std::forward<Args>(args)...);
    }

    /// @name Stage Access
    /// @{

    /**
     * @brief Gets a reference to a specific pipeline stage
     *
     * Provides type-safe access to any pipeline stage by index.
     *
     * @tparam N The stage index to access
     * @return Reference to the requested pipeline stage
     */
    template <int N, std::enable_if_t<(N == Count), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return *this; }

    template <int N, std::enable_if_t<(N == Count), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return *this; }

    template <int N, std::enable_if_t<(N < Count), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return _previous.template at<N>(); }

    template <int N, std::enable_if_t<(N < Count), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _previous.template at<N>(); }
    /// @}

private:
    configs_type& _configs;
    prev_pipeline_type& _previous;
    std::size_t _id;
};

/**
 * @brief Pipeline execution plan blueprint
 *
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

namespace detail{

template <typename... Components>
struct component_max_stage;

template <typename Component, typename... Components>
struct component_max_stage<Component, Components...>{
private:
    static constexpr std::size_t value_rest = component_max_stage<Components...>::value;
public:
    static constexpr std::size_t value = Component::features::max_stage >= value_rest ? Component::features::max_stage : value_rest;
};

template <typename Component>
struct component_max_stage<Component>{
private:
public:
    static constexpr std::size_t value = Component::features::max_stage;
};

template <typename CompositionT>
struct composition_max_stage;

template <typename... Components>
struct composition_max_stage<udho::manifold::composition<Components...>>{
    static constexpr std::size_t value = component_max_stage<Components...>::value;
};

}

template <typename CompositionT, typename OrderT>
using start_pipeline = pipeline<CompositionT, OrderT, detail::composition_max_stage<CompositionT>::value +1, -1>;

template <typename CompositionT, typename OrderT>
using finish_pipeline = pipeline<CompositionT, OrderT, detail::composition_max_stage<CompositionT>::value +1, detail::composition_max_stage<CompositionT>::value +1>;

/**
 * @brief Runtime manager for pipeline executions
 *
 * Manages the lifecycle of flows providing:
 * - Composition management
 *   - constructs the composition through the constructor
 * - Thread-safe Flow spawning and tracking
 *   - creates a flow with a reference to this runtime, composition and baseline configurations
 *   - manages a collection of active flows
 *   - terminated flows are removed from the collection
 * - Baseline configuration loading/saving
 *   - default constructs the default configuration
 *   - possible to load from json after instantiation with default values
 *
 * @tparam LabelT The label type identifying the pipeline configuration
 */
template <typename LabelT>
struct runtime{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using flow_type         = flow<label_type>;
    using flow_ptr_type     = std::shared_ptr<flow_type>;
    using collection_type   = std::vector<flow_ptr_type>;

    static constexpr std::size_t Count = detail::composition_max_stage<composition_type>::value +1;

    /// @name Pipeline Type Aliases
    /// @{
    template <int Stage>
    using pipeline_at          = pipeline<composition_type, order_type, Count, Stage>;
    using start_pipeline_type  = pipeline_at<-1>;
    using finish_pipeline_type = pipeline_at<Count>;
    /// @}

    /**
     * @brief Constructs a runtime
     *
     * @param composition Component composition for this runtime
     */
    template <typename... Args>
    static composition_type compose(Args&&... args) { return composition_type::compose(std::forward<Args>(args)...); }

    runtime(runtime&&) = delete;
    runtime& operator=(runtime&&) = delete;

    runtime(composition_type&& composition): _composition(std::move(composition)) {}

    template <typename... Components>
    runtime(Components&&... components): _composition(composition_type::compose(std::forward<Components>(components)...)) {}

    /// @name Composition Access
    /// @{
    composition_type& composition() { return _composition; }
    const composition_type& composition() const { return _composition; }
    /// @}

    /// @name Configuration
    /// @{
    const configs_type& baseline() const {return _baseline; }
    configs_type& baseline() {return _baseline; }

    /**
     * @brief Loads baseline configuration from JSON
     *
     * @param json JSON object containing configuration for all components
     */
    void load(const nlohmann::json& json){
        _baseline.load(json);
    }
    /// @}

    /// @name Flow Management
    /// @{

    /**
     * @brief Spawns a new flow for pipeline execution
     *
     * Creates a new flow instance associated with this runtime.
     * The flow is tracked internally until it completes.
     *
     * @return Shared pointer to the new flow
     */
    flow_ptr_type spawn() {
        std::scoped_lock lock(_mutex);
        flow_ptr_type flow_ptr = flow_type::create(*this);
        _flows.push_back(flow_ptr);
        return flow_ptr;
    }

    /**
     * @brief Gets the current number of active flows
     *
     * @return Number of flows currently being tracked
     */
    std::size_t count() const {
        std::scoped_lock<std::mutex> lock(_mutex);
        return _flows.size();
    }

    /**
     * @brief Removes a completed flow from tracking
     *
     * Called by flows when they terminate to clean up resources.
     *
     * @param flow The flow to remove
     */
    bool remove(const flow_ptr_type& flow) {
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = std::find_if(_flows.begin(), _flows.end(), [id = flow->id()](const auto& f){
            return f->id() == id;
        });
        if(it != _flows.end()) {
            std::cerr << "remove() found flow: ptr=" << flow.get() << " id=" << flow->id() << " tracked=" << _flows.size() << "\n";
            _flows.erase(it);
            return true;
        } else {
            std::cerr << "remove() missing flow: ptr=" << flow.get() << " id=" << flow->id() << " tracked=" << _flows.size() << "\n";
            return false;
        }
    }
    /// @}

private:
    composition_type _composition;
    configs_type     _baseline;
private:
    collection_type    _flows;
    mutable std::mutex _mutex;
};

/**
 * @brief Configuration patching between pipeline stages
 *
 * The default configuration patching during stage transition. The apply() method
 * is called after a stage completes successfully, before the next stage begins, to
 * implement custom configuration.
 *
 * @warning Usercode should not specialize this template. Instead specialize patch
 * and call patch_config::apply in order to apply the default patching for the stage.
 *
 * @tparam LabelT The pipeline label type
 * @tparam Stage The stage index from which the transition occurs
 */
template <typename LabelT, std::size_t Stage>
struct patch_config{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    /**
     * @brief Applies configuration patches between stages
     *
     * Called after stage completion to modify configuration before
     * the next stage begins. The default implementation does nothing.
     *
     * @param p The completed pipeline stage
     * @param config The configuration to modify for the next stage
     */
    static void apply(const pipeline_type& p, configs_type& config) { /* nothing unless specialized */ }
};

/**
 * @brief Configuration patching between pipeline stages
 *
 * Usercode should specialize this template to implement custom configuration
 * modifications when transitioning between pipeline stages. The apply() method
 * is called after a stage completes successfully, before the next stage begins.
 *
 * @tparam LabelT The pipeline label type
 * @tparam Stage The stage index from which the transition occurs
 */
template <typename LabelT, std::size_t Stage>
struct patch{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    /**
     * @brief Applies configuration patches between stages
     *
     * Called after stage completion to modify configuration before
     * the next stage begins. The default implementation does nothing.
     *
     * @param p The completed pipeline stage
     * @param config The configuration to modify for the next stage
     */
    static void apply(const pipeline_type& p, configs_type& config) {
        udho::manifold::patch_config<LabelT, Stage>::apply(p, config);
    }
};

/**
 * @brief The terminal class determines whether the flow will be deleted or not.
 *
 * By default the reenter() and error() return false which implies the following.
 *  1. **reender() -> false** the flow will be terminated (no reentered) after
 *      successful completion from 0...Count stages
 *  2. **error() -> false** the flow will be terminated (no reentered) after
 *      encountering failure in any facet during evaluation in 0...Count stages
 */
template <typename LabelT>
struct terminal{
    using label_type        = LabelT;
    using runtime_type      = runtime<label_type>;
    using flow_type         = flow<label_type>;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;

    terminal() = delete;
    terminal(const terminal&) = delete;

    terminal(composition_type& composition, const configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    template <typename... Args>
    bool reenter(Args&&... args) { return false; }

    template <typename... Args>
    bool error(udho::manifold::exclusive_result, Args&&...){ return false; }

    template <typename... Args>
    void prepare(Args&&... args) { }

private:
    composition_type&   _composition;
    const configs_type& _configs;
    const journal_type& _journal;
};

namespace detail {

/**
 * @brief Compile-time patcher for all pipeline stages
 *
 * Recursively applies patch_config specializations for each stage
 * transition in the pipeline.
 *
 * @tparam LabelT The pipeline label type
 * @tparam Count Total number of pipeline stages
 * @tparam Stage Current stage index
 */
template <typename LabelT, std::size_t Count, std::size_t Stage>
struct patcher: public detail::patcher<LabelT, Count, Stage+1>/*, private udho::manifold::patch<LabelT, Stage>*/{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    /**
     * @brief apply patch_config on the configs with the current pipeline
     * @param p
     * @param configs
     */
    void apply(pipeline_type& p, configs_type& configs){
        udho::manifold::patch<LabelT, Stage>::apply(p, configs);
    }
};

template <typename LabelT, std::size_t Count>
struct patcher<LabelT, Count, Count>{};

}

/**
 * @brief Individual pipeline execution instance
 *
 * Represents a single execution flow through the pipeline stages.
 * Manages the pipeline execution, stage transitions, and termination.
 * Inherits from patcher to apply stage-specific configuration patches.
 *
 * @tparam LabelT The pipeline label type
 */
template <typename LabelT>
struct flow: public std::enable_shared_from_this<flow<LabelT>>, detail::patcher<LabelT, runtime<LabelT>::Count, 0>{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using ptr               = std::shared_ptr<flow<LabelT>>;

    static constexpr std::size_t Count = runtime_type::Count;

    /// @name Pipeline Type Aliases
    /// @{
    template <int Stage>
    using pipeline_at          = typename runtime_type::template pipeline_at<Stage>;
    using start_pipeline_type  = typename runtime_type::start_pipeline_type;
    using finish_pipeline_type = typename runtime_type::finish_pipeline_type;
    /// @}

    /// @name Pipeline sompletion callback
    /// @{
    using callback_type        = std::function<void (const flow<LabelT>&, bool)>;
    /// @}

    using journal_type         = typename start_pipeline_type::full_journal_type;

    template <typename>
    friend struct runtime;

    flow() = delete;
    flow(const flow<LabelT>&) = delete;
    flow(flow<LabelT>&&) = delete;

    std::size_t id() const { return _id; }

    /**
     * @brief Gets a shared pointer to this flow
     * @return Shared pointer to this flow instance
     */
    ptr self() { return std::enable_shared_from_this<flow<LabelT>>::shared_from_this(); }

    /**
     * @brief Applies configuration patches for a specific stage
     *
     * Called internally during stage transitions to apply any
     * patch_config specializations for the completed stage.
     *
     * @tparam Stage The completed stage index
     * @param p The completed pipeline stage
     * @param config Configuration to modify for next stage
     */
    template <int Stage>
    void apply(pipeline_at<Stage>& p, configs_type& config){
        detail::patcher<LabelT, udho::manifold::runtime<LabelT>::Count, Stage>::apply(p, config);
    }

    /**
     * @brief Starts synchronous pipeline execution
     *
     * Begins execution of the pipeline with the provided arguments.
     * Execution proceeds synchronously through all stages.
     *
     * @tparam Args... Argument types to forward to pipeline stages
     * @param args Arguments to forward to pipeline stages
     */
    template <typename... Args>
    void start(Args&&... args) { _root_pipeline(self(), std::forward<Args>(args)...); }

    /**
     * @brief Starts asynchronous pipeline execution
     *
     * Begins execution of the pipeline with asynchronous completion
     * handling via Boost.Asio.
     *
     * @tparam Args... Argument types to forward to pipeline stages
     * @param io Boost.Asio io_context for async operations
     * @param args Arguments to forward to pipeline stages
     */
    template <typename... Args>
    void start(boost::asio::io_context& io, Args&&... args) { _root_pipeline(io, self(), std::forward<Args>(args)...); }

    template <typename... Args>
    bool reenter(Args&&... args) {
        using terminal_type = udho::manifold::terminal<label_type>;

        terminal_type terminal(composition(), configs(), journal());
        bool should_reenter = terminal.reenter(std::forward<Args>(args)...);
        terminate(should_reenter);
        return should_reenter;
    }

    template <typename... Args>
    void prepare(Args&&... args) {
        using terminal_type = udho::manifold::terminal<label_type>;

        terminal_type terminal(composition(), configs(), journal());
        terminal.prepare(std::forward<Args>(args)...);
    }

    /**
     * @brief Conditionally terminates the flow after encountering failure in a facet during evaluation of the pipeline.
     *
     * Call's the terminal's error function. If it returns true then reenters. Otherwise terminates.
     *
     * @note call originates from next_evaluator_helper_internal::fail triggered by calling
     *       next.fail(...) from some facet while evaluating the pipeline. The callback is set
     *       from pipeline::_then
     *
     * @param success
     */
    template <typename... Args>
    bool error(udho::manifold::exclusive_result success, Args&&... args) {
        assert(!success);

        using terminal_type = udho::manifold::terminal<label_type>;
        terminal_type terminal(composition(), configs(), journal());
        bool should_reenter = terminal.error(success, std::forward<Args>(args)...);

        terminate(should_reenter);

        if(should_reenter){
            _finish_pipeline.restart(self(), std::forward<Args>(args)...);
        }
        return should_reenter;
    }

    const start_pipeline_type& root() const { return _root_pipeline; }

    void then(callback_type&& callback){
        _callback = std::move(callback);
    }

    composition_type& composition() { return _root_pipeline.composition(); }
    const composition_type& composition() const { return _root_pipeline.composition(); }

    const configs_type& configs() const { return _root_pipeline.configs(); }
    const configs_type& baseline() const { return _runtime.baseline(); }
    const journal_type& journal() const { return _root_pipeline.journal(); }

private:

    /**
     * @brief Finalizes the current request and optionally keeps the flow alive for reentry
     *
     * Invokes the optional user callback (installed via flow::then) and, if `reenter == false`,
     * removes the flow from the runtime tracking collection.
     *
     * @param reenter If true, the flow remains tracked and is expected to be restarted
     *                via the finish pipeline restart path. If false, the flow is removed.
     *
     * @note This function does not itself restart the pipeline. Restart is performed by
     *       the finish pipeline (stage Count) or by flow::error() on failure.
     */
    void terminate(bool reenter) {
        if(_callback){
            _callback(*this, reenter);
        }
        if(!reenter) {
            bool removed = _runtime.remove(self());
            if(!removed) {
                std::cout << "Flow doesn't exist in collection" << std::endl;
            }
        }
    }

private:

    /**
     * @brief Private constructor for flow creation
     *
     * @param runtime Reference to the managing runtime
     * @param composition Reference to the component composition
     * @param baseline Reference to baseline configuration
     */
    flow(runtime_type& runtime, composition_type& composition, configs_type& baseline)
        : _runtime(runtime), _id(_counter++), _root_pipeline(composition, baseline, _id), _finish_pipeline(_root_pipeline.template at<Count>()) {}

    /**
     * @brief Factory method for flow creation
     *
     * @param runtime Reference to the managing runtime
     * @return New flow instance
     */
    static ptr create(runtime_type& runtime) {
        return ptr(new flow(runtime, runtime.composition(), runtime.baseline()));
    }

private:
    runtime_type&           _runtime;
    std::size_t             _id;
    start_pipeline_type     _root_pipeline;
    finish_pipeline_type&   _finish_pipeline;
    callback_type           _callback;
    static std::size_t      _counter;
};

template <typename LabelT>
std::size_t flow<LabelT>::_counter = 0;

/**
 * @}
 */

}
}


#endif // UDHO_MANIFOLD_PIPELINE_H
