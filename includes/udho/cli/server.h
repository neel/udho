#ifndef UDHO_CLI_SERVER_H
#define UDHO_CLI_SERVER_H

#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <udho/cli/control.h>
#include <udho/cli/termination.h>
#include <udho/net/listener.h>
#include <udho/url/router.h>
#include <udho/view/resources/store.h>
#include <udho/www/framework.h>

namespace udho{
namespace cli{

namespace detail{

template <typename UrlsT>
struct router_for;

template <typename... Mountpoints>
struct router_for<udho::url::mountpoints_table<Mountpoints...>>{
    using type = udho::url::basic_router<udho::url::mountpoints_table<Mountpoints...>>;
};

template <typename StrT, typename ActionsT>
struct router_for<udho::url::mount_point<StrT, ActionsT>>{
    using mountpoint_type = udho::url::mount_point<StrT, ActionsT>;
    using type = udho::url::basic_router<udho::url::mountpoints_table<mountpoint_type>>;
};

}

template <typename FrameworkT, typename ChannelT, typename StoreT, typename UrlsT>
class server{
    using cstore_type       = typename StoreT::const_store_type;
    using router_type       = typename detail::router_for<UrlsT>::type;
    using label_type        = typename FrameworkT::label_type;
    using stream_type       = typename label_type::stream_type;
    using framework_type    = udho::www::runtime_generator<label_type, stream_type, router_type>;
    using runtime_type      = typename framework_type::runtime_type;
    using channel_type      = ChannelT;
    using transport_type    = typename channel_type::transport_type;
    using listener_type     = udho::net::basic_listener<transport_type, runtime_type>;
    using endpoint_type     = typename channel_type::endpoint_type;

    static_assert(std::is_same<stream_type, typename channel_type::stream_type>::value, "Framework label stream must match channel stream");

public:
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    server(server&&) = delete;
    server& operator=(server&&) = delete;

    void run(boost::asio::io_context& io) {
        if(_listener) throw std::logic_error("Server is already running");
        endpoint_type endpoint = _channel.endpoint();
        auto report = [endpoint, &io](const boost::system::error_code& error) {
            if(error) {
                std::cerr << "Unable to start server on " << endpoint << ": " << error.message() << std::endl;
                io.stop();
                return;
            }
            std::cout << " ● " << "Server running on " << endpoint << std::endl;
        };
        _listener.emplace(io, _runtime, _channel);
        _listener->start(std::move(report));
        udho::cli::termination terminator(io);
        terminator.async_wait([this](int) { shutdown(); });
        _control.run(io);
    }

private:
    friend class control;

    void shutdown() {
        if(!_listener) return;
        std::cout << std::endl << " ● " << "Stopping listener \t";
        std::cout.flush();
        _listener->stop();
        std::cout << "done" << std::endl;
    }

    template <typename... Components>
    server(control& control, channel_type& channel, StoreT& store, UrlsT&& urls, Components&&... components)
        : _control(control)
        , _channel(channel)
        , _cstore(udho::view::resources::lock(store))
        , _framework(FrameworkT::apply(udho::url::router(std::forward<UrlsT>(urls), _cstore.assets())))
        , _runtime(_framework.runtime(_cstore, std::forward<Components>(components)...))
    {}

    control& _control;
    channel_type& _channel;
    cstore_type _cstore;
    framework_type _framework;
    runtime_type _runtime;
    std::optional<listener_type> _listener;
};

}
}

#endif // UDHO_CLI_SERVER_H
