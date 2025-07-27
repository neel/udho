#ifndef UDHO_MIDDLEWARE_FWD_H
#define UDHO_MIDDLEWARE_FWD_H

#include <udho/middleware/features.h>

namespace udho {
namespace middleware {

template <typename HeadT, typename... Tail>
struct facade_chain;

template <typename HeadT, typename... Tail>
struct states;

template <typename FeatureX, typename... Features>
struct evaluator;


}
}

#endif // UDHO_MIDDLEWARE_FWD_H
