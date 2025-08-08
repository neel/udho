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
    using mediator_type    = typename composition_type::mediator_type;
    using states_type      = typename udho::manifold::detail::states_for_mediator<mediator_type>::type;

    pipeline(composition_type& composition): _mediator(composition) {}

    states_type& states() { return _states; }
    const states_type& states() const { return _states; }

    template <typename... Features>
    std::size_t operator()(udho::manifold::evaluator<Features...>&& evaluator) {
        return evaluator(_mediator, _states);
    }


    private:
    mediator_type  _mediator;
    states_type    _states;
};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
