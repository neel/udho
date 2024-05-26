#ifndef UDHO_NET_ARTIFACTS_H
#define UDHO_NET_ARTIFACTS_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/stream.h>
#include <udho/exceptions/exceptions.h>
#include <udho/url/summary.h>

namespace udho{
namespace net{

template <typename RouterT, typename ResourcesStoreT>
struct artifacts{
    using router_type         = RouterT;
    using resource_store_type = ResourcesStoreT;

    artifacts(const router_type& router, const resource_store_type& resources): _router(router), _resources(resources) {}

    const router_type& router() const { return _router; }
    const resource_store_type& resources() const { return _resources; }

    private:
        const router_type&          _router;
        const resource_store_type&  _resources;
};

}
}

#endif // UDHO_NET_ARTIFACTS_H
