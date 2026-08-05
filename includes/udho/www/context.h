#ifndef UDHO_WWW_CONTEXT_H
#define UDHO_WWW_CONTEXT_H

#include <udho/manifold/context.h>
#include <udho/www/components/handler.h>
#include <udho/net/detail.h>

namespace udho {
namespace www {

/**
 * @brief www context alias that always includes the stream handler component.
 *
 * This alias adapts the generic manifold context for www code by prepending the
 * `basic_handler<StreamT>` component to the component list exposed through the
 * context portal.
 *
 * @tparam StreamT Stream type used by the context.
 * @tparam Components Additional www components exposed by the context.
 *
 * @ingroup DoxyG_www
 */
template <typename StreamT, typename... Components>
using basic_context = udho::manifold::basic_context<StreamT, udho::www::components::basic_handler<StreamT>, Components...>;

/**
 * @brief TCP www context alias.
 *
 * Uses the default TCP socket type selected by `udho::net::detail::wire_types`
 * and exposes the handler component plus any additional components.
 *
 * @tparam Components Additional www components exposed by the context.
 *
 * @ingroup DoxyG_www
 */
template <typename... Components>
using context = basic_context<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type, Components...>;

}
}

#endif // UDHO_WWW_CONTEXT_H
