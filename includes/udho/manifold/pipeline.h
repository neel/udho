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
    using fabric_type    = typename composition_type::fabric_type;
    using journal_type      = typename udho::manifold::detail::journal_for_fabric<fabric_type>::type;

    pipeline(composition_type& composition): _fabric(composition) {}

    journal_type& journal() { return _journal; }
    const journal_type& journal() const { return _journal; }

    template <typename... Features>
    std::size_t operator()(udho::manifold::evaluator<Features...>&& evaluator) {
        return evaluator(_fabric, _journal);
    }


    private:
    fabric_type  _fabric;
    journal_type    _journal;
};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
