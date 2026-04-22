#ifndef UDHO_MANIFOLD_FLOW_H
#define UDHO_MANIFOLD_FLOW_H

#include <memory>
#include <iostream>
#include <udho/manifold/fwd.h>
#include <udho/manifold/transition.h>
#include <udho/net/detail.h>
#include <udho/logging/macros.h>
#include <udho/exceptions/exceptions.h>

namespace udho{
namespace manifold{

/**
 * @ingroup manifold
 * @{
 */

/**
 * @brief Individual pipeline execution instance
 *
 * Represents a single execution flow through the pipeline stages.
 * Manages the pipeline execution, stage transitions, and termination.
 * Inherits from patcher to apply stage-specific configuration patches.
 *
 * @tparam LabelT The pipeline label type
 */
template <typename LabelT, typename StreamT>
struct basic_flow: public std::enable_shared_from_this<basic_flow<LabelT, StreamT>>, detail::transitioner<LabelT, StreamT, basic_runtime<LabelT, StreamT>::Count, 0>{
    using label_type        = LabelT;
    using stream_type       = StreamT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = basic_runtime<label_type, stream_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using ptr               = std::shared_ptr<basic_flow<LabelT, StreamT>>;
    using terminal_type     = udho::manifold::basic_terminal<label_type, stream_type>;

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
    using callback_type        = std::function<void (const basic_flow<LabelT, StreamT>&, bool)>;
    /// @}

    using journal_type         = typename start_pipeline_type::full_journal_type;

    template <typename, typename>
    friend struct basic_runtime;

    template <typename, typename>
    friend struct basic_terminal;

    basic_flow() = delete;
    basic_flow(const basic_flow<LabelT, StreamT>&) = delete;
    basic_flow(basic_flow<LabelT, StreamT>&&) = delete;

    std::size_t id() const { return _id; }

    /**
     * @brief Gets a shared pointer to this flow
     * @return Shared pointer to this flow instance
     */
    ptr self() { return std::enable_shared_from_this<basic_flow<LabelT, StreamT>>::shared_from_this(); }

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
    template <int Stage, typename... Args>
    void apply(pipeline_at<Stage>& p, configs_type& config, Args&&... args){
        detail::transitioner<LabelT, StreamT, udho::manifold::basic_runtime<LabelT, StreamT>::Count, Stage>::apply(self(), p, config, std::forward<Args>(args)...);
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
    void start(Args&&... args) {
        namespace p = udho::logging::params;
        UDHO_LOG_INFO("manifold::flow", "Flow starts", p::flow_id(id()));

        _root_pipeline(self(), _stream, std::forward<Args>(args)...);
    }

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
    void start(boost::asio::io_context& io, Args&&... args) { _root_pipeline(io, self(), _stream, std::forward<Args>(args)...); }

    /**
     * @brief reenter determines whether to restart the flow or not after successful evaluation
     *        of all facet pipelines in all stages synchronously.
     * @param args
     * @return bool
     * @note Consults with terminal specialization to make decision through terminal.reenter() method;
     * @note Call originates from finish_pipeline::operator()
     */
    template <typename... Args>
    bool reenter(Args&&... args) {
        bool should_reenter = _terminal.reenter(std::forward<Args>(args)...);
        terminate(should_reenter);
        return should_reenter;
    }

    /**
     * @brief prepare the flow for reentry
     * @param args
     * @note call originates from finish_pipeline's restart method
     */
    template <typename... Args>
    void prepare(Args&&... args) {
        _terminal.prepare(std::forward<Args>(args)...);
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
    void internal_error(udho::manifold::evaluation_result success, Args&&... args) {
        assert(!success);
        _terminal.internal_error(success, *this, std::forward<Args>(args)...); // flow is owned by the runtime
    }

    /**
     * @brief Communicate the errors originating from the usercode to the terminal.
     *
     * @note call originates from usercode running inside the slots that are bound to the url patterns
     *       delivered through the ostream. The terminal creates a 500 Internal server Error response
     *       with the error message and the stack trace.
     *
     * @note When in production the terminal should behave differently and instead of showing this error
     *       as HTTP response it should log the error only.
     *
     * @param capex exception with stacktrace
     * @param args
     */
    template <typename... Args>
    void user_error(const udho::exceptions::captured& capex, Args&&... args) {
        _terminal.user_error(capex, *this, std::forward<Args>(args)...); // flow is owned by the runtime
    }


    const start_pipeline_type& root() const { return _root_pipeline; }

    void then(callback_type&& callback){
        _callback = std::move(callback);
    }

    composition_type& composition() { return _root_pipeline.composition(); }
    const composition_type& composition() const { return _root_pipeline.composition(); }

    const runtime_type& runtime() const { return _runtime; }

    const configs_type& configs() const { return _root_pipeline.configs(); }
    configs_type& configs() { return _root_pipeline.configs(); }

    const configs_type& baseline() const { return _runtime.baseline(); }
    const journal_type& journal() const { return _root_pipeline.journal(); }

    stream_type& stream() { return _stream; }
    const stream_type& stream() const { return _stream; }

private:

    template <typename... Args>
    void restart(Args&&... args) {
        terminate(true);
        _finish_pipeline.restart(self(), std::forward<Args>(args)...);
    }

    void abort() {
        terminate(false);
    }

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
        namespace p = udho::logging::params;
        UDHO_LOG_INFO("manifold::flow", "Terminated", p::flow_id(id()), p::socket_id(udho::utils::misc::native_handle(_stream)));

        if(_callback){
            try{
                _callback(*this, reenter);
            } catch(std::exception ex) {
                std::cout << "Exception thrown from terminate callback: " << ex.what() << std::endl;
            }
        }
        if(!reenter) {
            bool removed = _runtime.remove(self());

            if(!removed) {
                UDHO_LOG_ERROR("manifold::flow", "Failed to remove flow", p::flow_id(id()));
            } else {
                UDHO_LOG_TRACE("manifold::flow", "Flow removed", p::flow_id(id()));
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
    basic_flow(runtime_type& runtime, composition_type& composition, configs_type& baseline, stream_type&& stream)
        : _runtime(runtime), _stream(std::move(stream)), _id(_counter++)
        , _root_pipeline(composition, baseline, _id)
        , _finish_pipeline(_root_pipeline.template at<Count>())
        , _terminal(_root_pipeline.composition(), _root_pipeline.configs(), _root_pipeline.journal()) {}

    /**
     * @brief Factory method for flow creation
     *
     * @param runtime Reference to the managing runtime
     * @return New flow instance
     */
    static ptr create(runtime_type& runtime, stream_type&& stream) {
        return ptr(new basic_flow(runtime, runtime.composition(), runtime.baseline(), std::forward<stream_type>(stream)));
    }

private:
    runtime_type&           _runtime;
    stream_type             _stream;
    std::size_t             _id;
    start_pipeline_type     _root_pipeline;
    finish_pipeline_type&   _finish_pipeline;
    callback_type           _callback;
    terminal_type           _terminal;
    static std::size_t      _counter;
};

template <typename LabelT, typename StreamT>
std::size_t basic_flow<LabelT, StreamT>::_counter = 0;

/**
 * @}
 */


}
}

#endif // UDHO_MANIFOLD_FLOW_H
