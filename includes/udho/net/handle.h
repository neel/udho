#ifndef UDHO_NET_HANDLE_H
#define UDHO_NET_HANDLE_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/context.h>

namespace udho{
namespace net{

template <typename Routes>
struct handle<udho::url::router<Routes>>{
    using router_type = udho::url::router<Routes>;

    handle(const router_type& router): _router(router) {}

    private:
        const router_type& _router;
};

}
}

#endif // UDHO_NET_HANDLE_H
