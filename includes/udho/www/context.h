#ifndef UDHO_WWW_CONTEXT_H
#define UDHO_WWW_CONTEXT_H

#include <udho/manifold/context.h>
#include <udho/www/components/connection.h>

namespace udho {
namespace www {

/**
 * @brief Stream-neutral context passed to WWW actions and layouts.
 *
 * The non-templated connection component is always exposed. Components named by
 * user code are appended after that default WWW capability.
 */
template <typename... Components>
using context = udho::manifold::basic_context<udho::www::components::connection, Components...>;

} // namespace www
} // namespace udho

#endif // UDHO_WWW_CONTEXT_H
