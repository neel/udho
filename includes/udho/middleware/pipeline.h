#ifndef UDHO_MIDDLEWARE_PIPELINE_H
#define UDHO_MIDDLEWARE_PIPELINE_H

#include <udho/middleware/features.h>
#include <udho/middleware/composition.h>
#include <udho/middleware/state.h>

namespace udho {
namespace middleware {

template <typename... Components>
struct pipeline: public udho::middleware::composition<Components...>{
    using composition_type = udho::middleware::composition<Components...>;
    using states_type      = udho::middleware::states<Components...>;

    using composition_type::composition_type;

    states_type& states() { return _states; }
    const states_type& states() const { return _states; }

    template <typename... Features>
    std::size_t operator()(udho::middleware::evaluator<Features...>&& evaluator) {
        return evaluator(*this, _states);
    }

    private:
    states_type _states;
};

}
}

#endif // UDHO_MIDDLEWARE_PIPELINE_H
