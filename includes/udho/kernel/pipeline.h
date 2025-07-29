#ifndef UDHO_KERNEL_PIPELINE_H
#define UDHO_KERNEL_PIPELINE_H

#include <udho/kernel/features.h>
#include <udho/kernel/composition.h>
#include <udho/kernel/state.h>

namespace udho {
namespace kernel {

template <typename... Components>
struct pipeline: public udho::kernel::composition<Components...>{
    using composition_type = udho::kernel::composition<Components...>;
    using states_type      = udho::kernel::states<Components...>;

    using composition_type::composition_type;

    states_type& states() { return _states; }
    const states_type& states() const { return _states; }

    template <typename... Features>
    std::size_t operator()(udho::kernel::evaluator<Features...>&& evaluator) {
        return evaluator(*this, _states);
    }

    private:
    states_type _states;
};

}
}

#endif // UDHO_KERNEL_PIPELINE_H
