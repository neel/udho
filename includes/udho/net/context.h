#ifndef UDHO_NET_CONTEXT_H
#define UDHO_NET_CONTEXT_H

#include <udho/net/fwd.h>
#include <udho/net/common.h>
#include <udho/net/bridge.h>
#include <udho/net/stream.h>
#include <udho/url/summary.h>
#include <udho/view/resources/store.h>

namespace udho{
namespace net{

template <typename ViewBridgeT>
struct basic_context<udho::view::resources::store<ViewBridgeT>>: public udho::net::stream{
    using resource_store = udho::view::resources::store<ViewBridgeT>;

    basic_context(boost::asio::io_service& io, udho::net::bridge& bridge, const udho::url::summary::router& summary, const resource_store& resources): udho::net::stream(io, bridge), _summary(summary), _resources(resources) {}
    basic_context(udho::net::stream&& stream, const udho::url::summary::router& summary, const resource_store& resources): udho::net::stream(std::move(stream)), _summary(summary), _resources(resources) {}

    const udho::url::summary::mount_point& route(const std::string& name) const {
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

template <typename ViewBridgeT>
using context = basic_context<udho::view::resources::store<ViewBridgeT>>;

}
}

#endif // UDHO_NET_CONTEXT_H
