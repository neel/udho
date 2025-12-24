#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/evaluator.h>

namespace udho {
namespace manifold {

namespace detail{

template <typename... Components>
struct get_journal_for_all_components{
    using full_fabric_type = typename udho::manifold::detail::flatten_all_of<Components...>::type;
    using type = typename udho::manifold::detail::journal_for_fabric<full_fabric_type>::type;
};

template <typename CompositionT>
struct get_journal_for_full_fabric;

template <typename... Components>
struct get_journal_for_full_fabric<udho::manifold::composition<Components...>>: get_journal_for_all_components<Components...>{};


template <typename JournalT, typename FabricT>
struct get_handler_type;

template <typename JournalT, std::size_t Stage, typename... Facets>
struct get_handler_type<JournalT, udho::manifold::fabric<Stage, Facets...>>{
    template <typename... Features>
    using for_features = typename detail::evaluator_helper<Stage, Features...>::template handler<JournalT, Facets...>;
};

};

template <std::size_t Stage, typename... Components>
struct basic_pipeline{
    using fabric_type       = typename udho::manifold::detail::flatten_all<Stage, Components...>::type;
    // using journal_type     = typename udho::manifold::detail::journal_for_fabric<fabric_type>::type;
    using full_journal_type = typename detail::get_journal_for_all_components<Components...>::type;

    template <typename... XComponents>
    basic_pipeline(udho::manifold::composition<XComponents...>& composition, const udho::manifold::configs<XComponents...>& configs): _fabric(composition, configs) {}

    template <typename... Features>
    struct evaluator{
        using helper_type         = detail::evaluator_helper<Stage, Features...>;
        using handler_type        = typename detail::get_handler_type<full_journal_type, fabric_type>::template for_features<Features...>;
        using safe_success_type   = typename handler_type::safe_success_type;
        using async_callback_type = typename handler_type::async_callback_type;

        evaluator(fabric_type& fabric, full_journal_type& journal, async_callback_type& callback): _handler(fabric, journal, callback) {}

        template <typename... Args>
        void eval(Args&&... args){
            _handler.template operator()<0>(std::forward<Args>(args)...);
        }

        private:
        handler_type _handler;
    };

    fabric_type& fabric() { return _fabric; }
    const fabric_type& fabric() const { return _fabric; }

    // journal_type& journal() { return _journal; }
    // const journal_type& journal() const { return _journal; }

    private:
    fabric_type  _fabric;
    // journal_type _journal;

};

template <typename... Features>
struct order{};

template <std::size_t Stage, typename OrderT, typename CompositionT>
class common_pipepine;

/**
 * @class common_pipepine
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
 * This class follows the principle of **"eval once, re-eval requires re-then"**:
 * - Each evaluation cycle requires setting a new callback via `then()`
 * - Callbacks are automatically cleared after execution
 * - This prevents stale state and memory leaks from retained callbacks
 *
 * # Stage Evaluation Model
 *
 * When `eval()` is called:
 * 1. Features are evaluated in the order specified by `order<Features...>`
 * 2. Each feature triggers evaluation of all components that provide it
 * 3. Results are stored in the provided journal
 * 4. On completion (success or failure), the user callback is invoked once
 * 5. The callback is cleared, requiring a new `then()` call for subsequent evaluation
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

    // using journal_type        = typename basic_pipeline_type::journal_type;

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
    common_pipepine(composition_type& composition, const configs_type& configs, full_journal_type& journal):
        basic_pipeline_type(composition, configs),
        _evaluator(basic_pipeline_type::fabric(), journal, _callback),
        _callback(std::bind(&common_pipepine::on_completion, this, std::placeholders::_1))
    {}
    /**
     * @name Fabric Access
     * Access to the underlying fabric for direct facet manipulation
     * @{
     */
    using basic_pipeline_type::fabric;
    /// @}

    // using basic_pipeline_type::journal;

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
     * @post The registered callback is invoked and cleared (nullptr)
     *
     * @warning If no callback is registered via `then()`, completion results are discarded
     * @warning Multiple calls to `eval()` without intervening `then()` will not invoke callbacks
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
     * The callback must accept a `safe_success_type` parameter, which is a
     * `std::variant<bool, std::exception_ptr>`:
     * - `bool` (index 0): `true` if all facets passed, `false` if any facet failed
     * - `std::exception_ptr` (index 1): Exception thrown during evaluation
     *
     * # Callback Lifecycle
     *
     * The callback is:
     * - Moved into internal storage (not copied)
     * - Invoked exactly once when `eval()` completes
     * - Cleared (set to nullptr) after invocation
     * - Must be re-registered for subsequent `eval()` calls
     *
     * @code
     * pipeline.then([](auto&& result) {
     *     if (result.index() == 0) {
     *         if (std::get<bool>(result)) {
     *             std::cout << "Success\n";
     *         } else {
     *             std::cout << "Failure\n";
     *         }
     *     } else {
     *         std::cout << "Exception\n";
     *         std::rethrow_exception(std::get<std::exception_ptr>(result));
     *     }
     * }).eval(...);
     * @endcode
     *
     * @note The callback is cleared after execution (following "eval once" principle)
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
     * The callback is wrapped and posted via `boost::asio::post(io, ...)`, ensuring:
     * - Execution occurs in an io_context handler thread
     * - Thread-safe dispatch if io_context runs multiple threads
     * - Proper ordering with other asynchronous operations
     *
     * # Usage Pattern
     *
     * @code
     * boost::asio::io_context io;
     *
     * pipeline.then(io, [&](auto&& result) {
     *     // Executes in io_context thread
     *     if (std::holds_alternative<bool>(result)) {
     *         handle_result(std::get<bool>(result));
     *     }
     * }).eval(request);
     *
     * io.run();  // Process completion callback
     * @endcode
     *
     * @note The io_context reference must remain valid until callback execution
     * @note Callback is cleared after execution (following "eval once" principle)
     */
    common_pipepine& then(boost::asio::io_context& io, async_callback_type&& callback){
        _user_callback = [&io, &callback](safe_success_type&& success){
            boost::asio::post(io, std::bind(std::forward<async_callback_type>(callback), std::move(success)));
        };
        return *this;
    }
private:
    void on_completion(safe_success_type&& success){
        if(_user_callback) {
            // Will be called in case of failure
            _user_callback(std::forward<safe_success_type>(success));
            _user_callback = nullptr;
        }
    }

private:
    async_callback_type _callback;
    async_callback_type _user_callback;
    evaluator_type      _evaluator;
};

template <typename LabelT>
struct flow;

template <typename CompositionT, typename OrderT, std::size_t Count, int Stage = 0>
struct pipeline{
    static_assert (Stage < Count);
    using composition_type   = CompositionT;
    using order_type         = OrderT;
    using pipeline_type      = udho::manifold::common_pipepine<Stage, order_type, composition_type>;
    using configs_type       = typename pipeline_type::configs_type;
    using prev_pipeline_type = pipeline<composition_type, order_type, Count, Stage-1>;
    using next_pipeline_type = pipeline<composition_type, order_type, Count, Stage+1>;

    friend class pipeline<composition_type, order_type, Count, Stage-1>;

    template <typename JournalT>
    pipeline(composition_type& composition, typename composition_type::configs_type& baseline, JournalT& journal, const prev_pipeline_type& previous)
        : _composition(composition), _configs(baseline), _pipeline(composition, _configs, journal), _next(composition, baseline, journal, *this), _previous(previous)
    {
        // _configs copy constructor picks the relevant configs from the baseline
    }

    configs_type& configs() { return _configs; }
    const configs_type& configs() const { return _configs; }

    template <typename FlowT, typename... Args>
    void operator()(std::shared_ptr<FlowT> flow, Args&&... args){
        static_assert((... && (std::is_lvalue_reference<Args>::value || std::is_copy_constructible<std::decay_t<Args>>::value)), "Non-lvalue arguments must be CopyConstructible");

        using args_tuple_t = std::tuple<std::conditional_t<std::is_lvalue_reference<Args>::value, Args, std::decay_t<Args>>...>;
        args_tuple_t args_tuple(std::forward<Args>(args)...);

        _then(flow, args_tuple);
        _pipeline.eval(std::forward<Args>(args)...);
    }

    template <typename FlowT, typename... Args>
    void operator()(std::shared_ptr<FlowT> flow, boost::asio::io_context& io, Args&&... args){
        static_assert((... && (std::is_lvalue_reference<Args>::value || std::is_copy_constructible<std::decay_t<Args>>::value)), "Non-lvalue arguments must be CopyConstructible");

        using args_tuple_t = std::tuple<boost::asio::io_context&, std::conditional_t<std::is_lvalue_reference<Args>::value, Args, std::decay_t<Args>>...>;
        args_tuple_t args_tuple(io, std::forward<Args>(args)...);

        _then(flow, args_tuple);
        _pipeline.eval(std::forward<Args>(args)...);
    }

    template <std::size_t N, std::enable_if_t<(N == Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return *this; }

    template <std::size_t N, std::enable_if_t<(N == Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return *this; }

    template <std::size_t N, std::enable_if_t<(N > Stage), bool> = true>
    pipeline<composition_type, order_type, Count, N>& at() { return _next.template at<N>(); }

    template <std::size_t N, std::enable_if_t<(N > Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _next.template at<N>(); }

    template <std::size_t N, std::enable_if_t<(N < Stage), bool> = true>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _previous.template at<N>(); }

private:
    template <typename FlowT, typename ArgsTupleT>
    void _then(std::shared_ptr<FlowT> flow, ArgsTupleT& args_tuple){
        auto lambda = [flow, this, args_tuple](std::variant<bool, std::exception_ptr> success){
            if(success.index() == 0 && std::get<0>(success)) {
                flow->apply(*this, configs());
                std::apply(
                    [&](auto&... args) {
                        _next(flow, args...);
                    },
                    args_tuple
                );
            } else {
                flow->terminate(false);
            }
        };
        _pipeline.then(std::move(lambda));
    }

private:
    composition_type&   _composition;
    const prev_pipeline_type& _previous;
    configs_type&        _configs;
    pipeline_type       _pipeline;
    next_pipeline_type  _next;
};

template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline<CompositionT, OrderT, Count, -1> {
    using composition_type   = CompositionT;
    using configs_type       = typename composition_type::configs_type;
    using order_type         = OrderT;
    using full_journal_type  = typename detail::get_journal_for_full_fabric<CompositionT>::type;
    using next_pipeline_type = pipeline<composition_type, order_type, Count, 0>;
    using self_type          = pipeline<CompositionT, OrderT, Count, -1>;
    using ptr                = std::shared_ptr<self_type>;

    pipeline(CompositionT& composition, typename CompositionT::configs_type& baseline): _composition(composition), _next(composition, baseline, _journal, *this) {}

    configs_type& configs() { return _configs; }
    const configs_type& configs() const { return _configs; }

    template <typename... Args>
    void operator()(Args&&... args){
        _next(std::forward<Args>(args)...);
    }

    template <std::size_t N>
    pipeline<composition_type, order_type, Count, N>& at() { return _next.template at<N>(); }

    template <std::size_t N>
    const pipeline<composition_type, order_type, Count, N>& at() const { return _next.template at<N>(); }

    const full_journal_type& journal() const { return _journal; }

private:
    composition_type&   _composition;
    configs_type        _configs;
    full_journal_type   _journal;
    next_pipeline_type  _next;
};

template <typename CompositionT, typename OrderT, std::size_t Count>
struct pipeline<CompositionT, OrderT, Count, static_cast<int>(Count)>{
    using prev_pipeline_type = pipeline<CompositionT, OrderT, Count, Count-1>;
    using configs_type       = typename prev_pipeline_type::configs_type;

    template <typename JournalT>
    pipeline(CompositionT&, typename CompositionT::configs_type& configs, JournalT&, const prev_pipeline_type& previous): _configs(configs), _previous(previous) {}

    configs_type& configs() { return _configs; }
    const configs_type& configs() const { return _configs; }

    template <typename FlowT, typename... Args>
    void operator()(std::shared_ptr<FlowT> flow, Args&&... args){
        flow->terminate(true);
    }

private:
    configs_type& _configs;
    const prev_pipeline_type& _previous;
};

/**
 * @brief provides the sketch of executaion plan for the label
 * The label can be any emoty struct
 * The sketch specialization must contain the following typedefs
 * - composition_type
 * - order_type
 * - Count
 *
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

template <typename LabelT>
struct runtime{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using flow_type         = flow<label_type>;
    using flow_ptr_type     = std::shared_ptr<flow_type>;
    // using flow_wptr_type    = std::weak_ptr<flow_type>;
    using collection_type   = std::vector<flow_ptr_type>;

    // static constexpr std::size_t Count = sketch_type::Count;

    static constexpr std::size_t Count = detail::composition_max_stage<composition_type>::value +1;

    template <int Stage>
    using pipeline_at          = pipeline<composition_type, order_type, Count, Stage>;
    using start_pipeline_type  = pipeline_at<-1>;
    using finish_pipeline_type = pipeline_at<Count>;

    template <typename... Args>
    static composition_type compose(Args&&... args) { return composition_type::compose(std::forward<Args>(args)...); }

    runtime(runtime&&) = delete;

    runtime& operator=(runtime&&) = delete;

    runtime(composition_type&& composition): _composition(std::move(composition)) {}

    composition_type& composition() { return _composition; }

    const composition_type& composition() const { return _composition; }

    const configs_type& baseline() const {return _baseline; }

    configs_type& baseline() {return _baseline; }

    flow_ptr_type spawn() {
        std::scoped_lock lock(_mutex);
        flow_ptr_type flow_ptr = flow_type::create(*this);
        _flows.push_back(flow_ptr);
        return flow_ptr;
    }

    std::size_t count() const {
        std::scoped_lock<std::mutex> lock(_mutex);
        return _flows.size();
    }

    // void cleanup() {
    //     std::scoped_lock<std::mutex> lock(_mutex);
    //     _flows.erase(std::remove_if(_flows.begin(), _flows.end(), [](flow_wptr_type& w){ return w.expired(); }), _flows.end());
    // }

    void remove(const flow_ptr_type& flow, bool success) {
        std::scoped_lock<std::mutex> lock(_mutex);
        _flows.erase(std::find(_flows.begin(), _flows.end(), flow));
    }

    void load(const nlohmann::json& json){
        _baseline.load(json);
    }

private:
    composition_type _composition;
    configs_type     _baseline;
private:
    collection_type    _flows;
    mutable std::mutex _mutex;
};

template <typename LabelT, std::size_t Stage>
struct patch_config{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    void apply(const pipeline_type& p, configs_type& config) { /* nothing unless specialized */ }
};

namespace detail {

template <typename LabelT, std::size_t Count, std::size_t Stage>
struct patcher: public detail::patcher<LabelT, Count, Stage+1>, private patch_config<LabelT, Stage>{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    void apply(const pipeline_type& p, configs_type& configs){
        patch_config<LabelT, Stage>::apply(p, configs);
    }
};

template <typename LabelT, std::size_t Count>
struct patcher<LabelT, Count, Count>{};

}

template <typename LabelT>
struct flow: public std::enable_shared_from_this<flow<LabelT>>, detail::patcher<LabelT, runtime<LabelT>::Count, 0>{
    using label_type        = LabelT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = runtime<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using ptr               = std::shared_ptr<flow<LabelT>>;

    // static constexpr std::size_t Count = sketch_type::Count;
    static constexpr std::size_t Count = runtime_type::Count;

    template <int Stage>
    using pipeline_at          = typename runtime_type::template pipeline_at<Stage>;
    using start_pipeline_type  = typename runtime_type::start_pipeline_type;
    using finish_pipeline_type = typename runtime_type::finish_pipeline_type;

    template <typename>
    friend struct runtime;

    flow() = delete;
    flow(const flow<LabelT>&) = delete;
    flow(flow<LabelT>&&) = delete;

    ptr self() { return std::enable_shared_from_this<flow<LabelT>>::shared_from_this(); }

    template <int Stage>
    void apply(const pipeline_at<Stage>& p, configs_type& config){
        detail::patcher<LabelT, runtime<LabelT>::Count, Stage>::apply(p, config);
    }

    template <typename... Args>
    void start(Args&&... args) { _pipeline(self(), std::forward<Args>(args)...); }

    template <typename... Args>
    void start(boost::asio::io_context& io, Args&&... args) { _pipeline(self(), io, std::forward<Args>(args)...); }

    void terminate(bool success) {
        _runtime.remove(self(), success);
    }

private:
    flow(runtime_type& runtime, composition_type& composition, configs_type& baseline): _runtime(runtime), _pipeline(composition, baseline) {}

    static ptr create(runtime_type& runtime) { return ptr(new flow(runtime, runtime.composition(), runtime.baseline())); }

private:
    runtime_type&       _runtime;
    start_pipeline_type _pipeline;

};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
