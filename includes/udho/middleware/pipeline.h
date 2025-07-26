#ifndef UDHO_MIDDLEWARE_PIPELINE_H
#define UDHO_MIDDLEWARE_PIPELINE_H

#include <udho/middleware/features.h>
#include <udho/middleware/facade.h>
#include <udho/middleware/state.h>

namespace udho {
namespace middleware {

template <typename ComponentT, typename... Tail>
struct pipeline: public udho::middleware::facade<ComponentT, Tail...>{
    using facade_type = udho::middleware::facade<ComponentT, Tail...>;
    using states_type = udho::middleware::states<ComponentT, Tail...>;

    using facade_type::facade_type;

    states_type& states() { return _states; }
    const states_type& states() const { return _states; }

    template <typename FeatureX, typename... Features>
    std::size_t operator()(udho::middleware::evaluator<FeatureX, Features...>&& evaluator) {
        using evaluator_type = udho::middleware::evaluator<FeatureX, Features...>;
        return std::forward<evaluator_type>(evaluator)(*this, _states);
    }

    private:
    states_type _states;
};

}
}

#endif // UDHO_MIDDLEWARE_PIPELINE_H
