#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/state.h>

namespace udho {
namespace manifold {

template <typename... Components>
struct pipeline{
    using composition_type = udho::manifold::composition<Components...>;
    using delegates_type   = typename composition_type::delegates_type;
    using states_type      = typename udho::manifold::detail::states_for_delegates<delegates_type>::type;

    pipeline(composition_type& composition): _delegates(composition) {}

    states_type& states() { return _states; }
    const states_type& states() const { return _states; }

    template <typename... Features>
    std::size_t operator()(udho::manifold::evaluator<Features...>&& evaluator) {
        return evaluator(_delegates, _states);
    }


    private:
    delegates_type  _delegates;
    states_type     _states;
};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
