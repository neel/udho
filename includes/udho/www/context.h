#ifndef UDHO_WWW_CONTEXT_H
#define UDHO_WWW_CONTEXT_H

#include <udho/manifold/context.h>
#include <udho/www/components/handler.h>
#include <udho/net/detail.h>

namespace udho {
namespace www {

template <typename StreamT, typename... Components>
using basic_context = udho::manifold::basic_context<StreamT, udho::www::components::basic_handler<StreamT>, Components...>;

template <typename... Components>
using context = basic_context<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type, Components...>;

}
}

#endif // UDHO_WWW_CONTEXT_H
