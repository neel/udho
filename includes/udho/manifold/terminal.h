#ifndef UDHO_MANIFOLD_TERMINAL_H
#define UDHO_MANIFOLD_TERMINAL_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/runtime.h>
#include <udho/manifold/flow.h>

namespace udho{
namespace manifold{

/**
 * @ingroup DoxyG_manifold
 * @{
 */

/**
 * @brief The terminal class determines whether the flow will be deleted or not.
 *
 * By default the reenter() and error() return false which implies the following.
 *  1. **reender() -> false** the flow will be terminated (no reentered) after
 *      successful completion from 0...Count stages
 *  2. **error() -> false** the flow will be terminated (no reentered) after
 *      encountering failure in any facet during evaluation in 0...Count stages
 */
template <typename LabelT, typename StreamT>
struct basic_terminal{
    using label_type        = LabelT;
    using stream_type       = StreamT;
    using runtime_type      = basic_runtime<label_type, stream_type>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;

    basic_terminal() = delete;
    basic_terminal(const basic_terminal&) = delete;

    /**
     * @brief Constructs a terminal attached to runtime state.
     *
     * @param composition Runtime component composition.
     * @param configs Runtime configuration collection.
     * @param journal Flow journal containing evaluation results.
     *
     * @warning The referenced composition, configs, and journal must outlive this
     *          terminal.
     */
    basic_terminal(composition_type& composition, configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    basic_terminal(basic_terminal&&) = delete;

    /**
     * @brief Determines whether a completed flow should re-enter evaluation.
     *
     * The default implementation returns `false`, meaning the flow terminates after
     * successful completion.
     *
     * @tparam Args Additional terminal argument types.
     * @param args Additional arguments supplied by the runtime.
     * @return `true` to re-enter the flow, `false` to terminate it.
     */
    template <typename... Args>
    bool reenter(Args&&... args) { return false; }

    /**
     * @brief Handles an internal evaluation error.
     *
     * The default implementation does nothing.
     *
     * @tparam Args Additional terminal argument types.
     * The unnamed evaluation result and additional arguments are ignored.
     * @param flow Flow that encountered the error.
     */
    template <typename... Args>
    void internal_error(udho::manifold::evaluation_result, flow_type& flow, Args&&...){ return; }

    /**
     * @brief Handles a user-code exception captured during flow execution.
     *
     * The default implementation does nothing.
     *
     * @tparam Args Additional terminal argument types.
     * @param capex Captured exception thrown or propagated by user code.
     * @param flow Flow that encountered the error.
     * The unnamed additional arguments are ignored.
     */
    template <typename... Args>
    void user_error(const udho::exceptions::captured& capex, flow_type& flow, Args&&...){ return; }

    /**
     * @brief Prepares terminal state before flow processing.
     *
     * The default implementation does nothing. Custom terminal policies may use this
     * hook to initialize per-flow or per-cycle state.
     *
     * @tparam Args Additional terminal argument types.
     * @param args Additional arguments supplied by the runtime.
     */
    template <typename... Args>
    void prepare(Args&&... args) { }

private:
    composition_type&   _composition;
    configs_type& _configs;
    const journal_type& _journal;
};

/**
 * @}
 */

}
}

#endif // UDHO_MANIFOLD_TERMINAL_H
