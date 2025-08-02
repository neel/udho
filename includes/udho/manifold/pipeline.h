#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/state.h>

namespace udho {
namespace manifold {

template <typename... Components>
struct pipeline: public udho::manifold::composition<Components...>{
    using composition_type = udho::manifold::composition<Components...>;
    using states_type      = typename udho::manifold::detail::states_for_composition<composition_type>::type;

    using composition_type::composition_type;

    pipeline(composition_type&& composition): composition_type(std::move(composition)) {}

    template <typename... Args>
    static pipeline compose(Args&&... args) {
        composition_type composition = compositor<Components...>::template compose<Args...>(std::forward<Args>(args)...);
        return pipeline{std::move(composition)};
    }

    states_type& states() { return _states; }
    const states_type& states() const { return _states; }

    template <typename... Features>
    std::size_t operator()(udho::manifold::evaluator<Features...>&& evaluator) {
        return evaluator(*this, _states);
    }

    private:
    states_type _states;
};

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
