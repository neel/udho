#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/journal.h>

namespace udho {
namespace manifold {

template <typename... Components>
struct pipeline{
    using composition_type = udho::manifold::composition<Components...>;
    using mediator_type    = typename composition_type::mediator_type;
    using journal_type      = typename udho::manifold::detail::journal_for_mediator<mediator_type>::type;

    pipeline(composition_type& composition): _mediator(composition) {}

    journal_type& journal() { return _journal; }
    const journal_type& journal() const { return _journal; }

    template <typename... Features>
    std::size_t operator()(udho::manifold::evaluator<Features...>&& evaluator) {
        return evaluator(_mediator, _journal);
    }


    private:
    mediator_type  _mediator;
    journal_type    _journal;
};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
