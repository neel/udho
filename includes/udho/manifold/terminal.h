#ifndef UDHO_MANIFOLD_TERMINAL_H
#define UDHO_MANIFOLD_TERMINAL_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/runtime.h>
#include <udho/manifold/flow.h>

namespace udho{
namespace manifold{

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

    basic_terminal(composition_type& composition, configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    template <typename... Args>
    bool reenter(Args&&... args) { return false; }

    template <typename... Args>
    void error(udho::manifold::exclusive_result, flow_type& flow, Args&&...){ return; }

    template <typename... Args>
    void prepare(Args&&... args) { }

private:
    composition_type&   _composition;
    configs_type& _configs;
    const journal_type& _journal;
};

}
}

#endif // UDHO_MANIFOLD_TERMINAL_H
