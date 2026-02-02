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
#include <udho/cookies/jar.h>
#include <udho/session/catalogue.h>

namespace udho{
namespace net{

namespace detail{

template <typename T>
struct is_router: std::false_type{};
template <typename MountPointsT>
struct is_router<udho::url::basic_router<MountPointsT>>: std::true_type{};

template <typename T>
struct is_resource_store: std::false_type{};
template <typename... Bridges>
struct is_resource_store<udho::view::resources::store<Bridges...>>: std::true_type{};


}

/**
 * @brief collection of common information that are relevant to and accessible by the url callbacks over the course of executaions of the server process.
 * It contains the following items:
 * - routing table
 * - resource store
 * - cookie jar
 * - session catalog
 */
template <typename RouterT, typename ResourcesStoreT>
struct artifacts{
    static_assert(detail::is_router<RouterT>::value);
    static_assert(detail::is_resource_store<ResourcesStoreT>::value);

    using router_type               = RouterT;
    using resource_store_type       = ResourcesStoreT;
    using const_resource_store_type = typename ResourcesStoreT::const_store_type;
    artifacts(router_type& router, const resource_store_type& resources): _router(router), _resources(resources) {}
    artifacts(const artifacts&) = delete;
    artifacts(artifacts&&) = delete;

    const router_type& router() const { return _router; }
    const const_resource_store_type& resources() const { return _resources; }

    private:
        const router_type&         _router;
        const_resource_store_type  _resources;
};

}
}

#endif // UDHO_NET_ARTIFACTS_H
