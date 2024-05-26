#ifndef UDHO_NET_CONTEXT_H
#define UDHO_NET_CONTEXT_H

#include <udho/net/fwd.h>
#include <udho/net/common.h>
#include <udho/net/bridge.h>
#include <udho/net/stream.h>
#include <udho/url/summary.h>

namespace udho{
namespace net{

template <typename ViewBridgeT>
struct basic_context<udho::view::resources::store<ViewBridgeT>>: public udho::net::stream{
    using resource_store = udho::view::resources::store<ViewBridgeT>;

    context(boost::asio::io_service& io, udho::net::bridge& bridge, const udho::url::summary::router& summary, const resource_store& rosources): udho::net::stream(io, bridge), _summary(summary), _resources(resources) {}

    inline const udho::url::summary::mount_point& route(const std::string& name) const {
        return _summary[name];
    }
    template <typename Char, Char... C>
    const udho::url::summary::mount_point& route(udho::hazo::string::str<Char, C...>&& hstr) const {
        return route(hstr.str());
    }
    template <typename XArg>
    const udho::url::summary::mount_point& operator[](XArg&& xarg) const {
        return route(std::forward<XArg>(xarg));
    }

    private:
        const udho::url::summary::router&   _summary;
        const resource_store&               _resources;
};

}
}

#endif // UDHO_NET_CONTEXT_H
