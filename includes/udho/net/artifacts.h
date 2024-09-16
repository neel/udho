#ifndef UDHO_NET_ARTIFACTS_H
#define UDHO_NET_ARTIFACTS_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/stream.h>
#include <udho/exceptions/exceptions.h>
#include <udho/url/router.h>
#include <udho/url/summary.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/store.h>

namespace udho{
namespace net{

/**
 * @brief collection of common information that are relevant to and accessible by the url callbacks over the course of executaions of the server process.
 * It contains the following items:
 * - routing table
 * - resource store
 */
template <typename RouterT, typename ResourcesStoreT>
struct artifacts;

template <typename MountPointsT, typename... Bridges>
struct artifacts<udho::url::router<MountPointsT>, udho::view::resources::store<Bridges...> >{
    using router_type               = udho::url::router<MountPointsT>;
    using resource_store_type       = udho::view::resources::store<Bridges...>;
    using resource_store_proxy_type = udho::view::resources::const_store<Bridges...>;

    artifacts(const router_type& router, resource_store_type& resources): _router(router), _resources_proxy(resources) {}

    const router_type& router() const { return _router; }
    const resource_store_proxy_type& resources() const { return _resources_proxy; }

    private:
        const router_type&         _router;
        resource_store_proxy_type  _resources_proxy;
};

}
}

#endif // UDHO_NET_ARTIFACTS_H
