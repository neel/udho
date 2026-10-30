#ifndef UDHO_CLI_SERVER_H
#define UDHO_CLI_SERVER_H

#include <iostream>
#include <optional>
#include <stdexcept>
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

template <typename FrameworkT, typename StoreT, typename UrlsT>
class server{
    using cstore_type       = typename StoreT::const_store_type;
    using router_type       = typename detail::router_for<UrlsT>::type;
    using label_type        = typename FrameworkT::label_type;
    using stream_type       = typename label_type::stream_type;
    using framework_type    = udho::www::runtime_generator<label_type, stream_type, router_type>;
    using runtime_type      = typename framework_type::runtime_type;
    using protocol_type     = typename runtime_type::stream_type::protocol_type;
    using listener_type     = udho::net::basic_listener<protocol_type, runtime_type>;
    using endpoint_type     = typename listener_type::endpoint_type;

public:
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    server(server&&) = delete;
    server& operator=(server&&) = delete;

    void run(boost::asio::io_context& io, endpoint_type endpoint) {
        if(_listener) throw std::logic_error("Server is already running");
        auto report = [endpoint, &io](const boost::system::error_code& error) {
            if(error) {
                std::cerr << "Unable to start server on " << endpoint << ": " << error.message() << std::endl;
                io.stop();
                return;
            }
            std::cout << " ● " << "Server running on " << endpoint << std::endl;
        };
        _listener.emplace(io, _runtime, endpoint);
        _listener->start(std::move(report));
        termination terminator(io);
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
    server(control& control, StoreT& store, UrlsT&& urls, Components&&... components)
        : _control(control)
        , _cstore(udho::view::resources::lock(store))
        , _framework(FrameworkT::apply(udho::url::router(std::forward<UrlsT>(urls), _cstore.assets())))
        , _runtime(_framework.runtime(_cstore, std::forward<Components>(components)...))
    {}

    control& _control;
    cstore_type _cstore;
    framework_type _framework;
    runtime_type _runtime;
    std::optional<listener_type> _listener;
};

}
}

#endif // UDHO_CLI_SERVER_H
