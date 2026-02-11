#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/evaluator.h>
#include <udho/manifold/basic_pipeline.h>
#include <udho/manifold/common_pipeline.h>

namespace udho {
namespace manifold {

/**
 * @ingroup manifold
 * @{
 */

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

    composition_type& composition() { return _composition; }
    const composition_type& composition() const { return _composition; }

    const auto& journal() const { return _previous.journal(); }

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

    template <typename FlowT, typename... Args>
    void next(FlowT flow, Args&&... args){
        _next(flow, std::forward<Args>(args)...);
    }

    template <typename FlowT, typename... Args>
    void abort(FlowT flow, Args&&... args){
        _next.abort(flow, std::forward<Args>(args)...);
    }

    // const common_pipeline_type& common_pipeline() const { return _common_pipeline; }
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
    template <typename FlowT, typename... Args>
    void _then(std::shared_ptr<FlowT> flow, std::tuple<Args...>& args_tuple){
        std::cout << "pipeline<" << udho::manifold::composition_name<CompositionT>::get() << ",OrderT," << Count << "," << Stage << ">::_then(flow, args_tuple)" << std::endl;
        auto lambda = [wflow = std::weak_ptr<FlowT>(flow), this, args_tuple](udho::manifold::exclusive_result success){
            auto flow = wflow.lock();
            assert(!!flow);
            if(success) {
                try{
                    std::apply(
                        [&](auto&&... args) {
                            flow->apply(*this, configs(), std::forward<Args>(args)...);
                        },
                        args_tuple
                    );
                } catch(...) {
                    udho::manifold::exclusive_result result(std::current_exception());
                    std::apply(
                        [&](auto&&... args) {
                            flow->error(std::move(result), std::forward<Args>(args)...);    // inform flow before termination
                        },
                        args_tuple
                    );
                }
            } else {                                            // error occured
                std::cout << "FAIL!!" << __LINE__ << std::endl;
                std::apply(
                    [&](auto&&... args) {
                        flow->error(success, std::forward<Args>(args)...);    // inform flow before termination
                    },
                    args_tuple
                );                                  // flow->error takes care of it.
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

    template <typename FlowT, typename... Args>
    void abort(FlowT flow, Args&&... args){
        std::cout << "pipeline<" << udho::manifold::composition_name<CompositionT>::get() << ",OrderT," << Count << "," << Count << ">";
        std::cout << "::abort()(io, flow, ...)" << std::endl;

        // if(flow->reenter(std::forward<Args>(args)...)){
        //     restart(flow, std::forward<Args>(args)...);
        // }
    }

    /**
     * @brief restart the flow
     * @param flow
     * @param args
     *
     * @note call originates either from basic_flow<LabelT, StreamT>::restart or operator()
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
 * @}
 */

}
}


#endif // UDHO_MANIFOLD_PIPELINE_H
