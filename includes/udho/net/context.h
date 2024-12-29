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

/**
 * @brief A proxy around the context used internally to make a view accessible conveniently
 * @tparam XBridgeT the bridge on which the intended view is registered
 * @tparam Bridges...  The bridges supported by the context
 */
template <typename XBridgeT, typename... Bridges>
struct proxy_wrapper{
    using context_type = basic_context<udho::view::resources::const_store<Bridges...>>;
    using proxy_type   = udho::view::resources::tmpl::proxy<XBridgeT>;
    using self_type    = proxy_wrapper<XBridgeT, Bridges...>;

    proxy_wrapper() = delete;
    proxy_wrapper(const proxy_wrapper&) = delete;
    proxy_wrapper(const context_type& ctx, proxy_type&& proxy): _ctx(ctx), _proxy(std::move(proxy)) {}
    proxy_wrapper(proxy_wrapper&& other): _ctx(other._ctx), _proxy(std::move(other._proxy)) {}


    /**
     * @brief Returns the name of the resource associated with this proxy.
     * @return The name of the resource.
     */
    inline std::string name() const { return _proxy.name(); }

    /**
     * @brief Returns the prefix of the resource associated with this proxy.
     * @return The prefix of the resource.
     */
    inline std::string prefix() const { return _proxy.prefix(); }

    /**
     * @brief Returns const reference to the context
     * @return context
     */
    const context_type& context() const { return _ctx; }


    friend auto metatype(udho::view::data::type<self_type>){
        using namespace udho::view::data;

        return assoc("view_proxy_wrapper"),
            fvar("name",   &self_type::name),
            fvar("prefix", &self_type::prefix);
    }

    private:
        const context_type& _ctx;
        proxy_type    _proxy;
};

template <typename... ViewBridgeT>
struct basic_context<udho::view::resources::const_store<ViewBridgeT...>>: public udho::net::stream{
    using resource_store = udho::view::resources::const_store<ViewBridgeT...>;
    using self_type = basic_context;

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

    const udho::url::summary::router& routes() const {
        return _summary;
    }

    const resource_store& resources() const {
        return _resources;
    }

    template <typename XBridgeT>
    proxy_wrapper<XBridgeT, ViewBridgeT...> view(const std::string& prefix, const std::string& name) const {
        return proxy_wrapper<XBridgeT, ViewBridgeT...>{*this, std::move(_resources.template view<XBridgeT>(prefix, name))};
    }

    friend auto metatype(udho::view::data::type<self_type>){
        using namespace udho::view::data;

        return assoc("context"),
            fvar("routes",      &self_type::routes),
            fvar("resources",   &self_type::resources);
    }

    private:
        const udho::url::summary::router&   _summary;
        const resource_store&               _resources;
};

template <typename... ViewBridgeT>
using context = basic_context<udho::view::resources::const_store<ViewBridgeT...>>;

namespace fake{

template <typename... Bridges>
struct context{
    using context_type   = udho::net::context<Bridges...>;
    using resources_type = udho::view::resources::const_store<Bridges...>;

    context(const udho::net::types::headers::request& request)
        : _request(request),
          _bridge{request, _response, _stream, _encoding, std::move([](udho::net::bridge::handler_type, bool)  -> void {}), std::move([] () -> void {})}
    {}

    template <typename MountPointsT, typename StoreT>
    context_type create(boost::asio::io_service& io, const udho::url::basic_router<MountPointsT, StoreT>& router, const resources_type& store){
        return context_type{io, _bridge, router.summary(), store};
    }

    public:
        udho::net::types::headers::request  _request;
        udho::net::types::headers::response _response;
        std::stringstream                   _stream;
        udho::net::types::transfer_encoding _encoding;
        udho::net::bridge                   _bridge;
};

}

}
}

#endif // UDHO_NET_CONTEXT_H
