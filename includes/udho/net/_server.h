#ifndef UDHO_NET_SERVER_H
#define UDHO_NET_SERVER_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/stream.h>
#include <udho/net/context.h>
#include <udho/exceptions/exceptions.h>
#include <udho/url/summary.h>
#include <udho/pages/system.h>

namespace udho{
namespace net{

/**
 * @brief Represents a generic HTTP server using Boost.Asio for network operations and a customizable listener for handling incoming connections.
 * @tparam ListenerT The type of the listener that handles incoming connections.
 * \ingroup server
 */
template <typename ListenerT>
struct server{
    using listener_type = ListenerT;
    using server_type   = server<ListenerT>;

    /**
     * @brief Constructs a server bound to the specified IP address and port.
     * @param io Reference to the Boost.Asio I/O service to use for asynchronous operations.
     * @param port Port number to bind the server.
     * @param ip IP address to bind the server. Defaults to "0.0.0.0" (all interfaces).
     */
    server(boost::asio::io_context& io, std::uint32_t port, const std::string& ip = "0.0.0.0"): _io(io), _endpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(ip), port)) {
        _listener = std::make_shared<listener_type>(_io, _endpoint);
    }

    /**
     * @brief Starts the server with the given artifacts and begins accepting connections.
     * @tparam ArtifactsT The type of the artifacts object that contains essential resources for request handling.
     * @param artifacts The artifacts containing routing and resource information.
     */
    template <typename ArtifactsT>
    void run(const ArtifactsT& artifacts){
        _listener->listen(std::bind(&server_type::serve<ArtifactsT>, this, std::placeholders::_1, std::placeholders::_2, std::cref(artifacts)));
    }

    /**
     * @brief Stops the server and all associated asynchronous operations.
     */
    void stop(){
        _listener->stop();
    }

    private:
        /**
         * @brief Prepares the context for handling a request by setting necessary HTTP headers.
         * @param address The IP address of the requester.
         * @param context The network stream associated with the current request.
         */
        void prepare(boost::asio::ip::address address, udho::net::stream context){
            context.set(boost::beast::http::field::server, "udho");
        }

        /**
         * @brief Handles incoming requests, matches routes, and executes corresponding actions.
         * @tparam ArtifactsT The type of the artifacts object.
         * @param address The IP address of the requester.
         * @param stream The network stream for the current request.
         * @param artifacts The artifacts containing routing and resource information.
         */
        template <typename ArtifactsT>
        void serve(boost::asio::ip::address address, udho::net::stream&& stream, const ArtifactsT& artifacts){
            using router_type = typename ArtifactsT::router_type;
            using const_resource_store_type = typename ArtifactsT::const_resource_store_type;

            const router_type& router = artifacts.router();

            const udho::url::summary::router& summary = router.summary();
            udho::net::basic_context<const_resource_store_type> context{std::move(stream), summary, artifacts.resources()};

            prepare(address, context);
            boost::beast::string_view tgt = context.request().target();
            std::string target(tgt.begin(), tgt.end());
            bool found = false;
            try{
                found = router(target, context);
                // TODO the targetted function may perform async operations which may make this try...catch block unnecessary because you can't catch them like that anyway'
                if(!found){
                    throw udho::http::error(address, context, boost::beast::http::status::not_found);
                }
            } catch(std::exception& ex) {
                fail(address, context, ex);
            } catch(udho::http::exception& ex) {
                fail(context, ex);
            } catch(udho::http::error& error) {
                fail(router, target, context, error);
            }
        }

        /**
         * @brief Handles failures due to standard exceptions.
         * @param stream The network stream associated with the current request.
         * @param ex The caught exception.
         */
        template <typename ContextT>
        void fail(ContextT ctx, const udho::http::exception& ex){
            ctx.response().result(boost::beast::http::status::internal_server_error);
            ctx << udho::url::format("Error: {}", ex.what());
            ctx.finish();
        }

        /**
         * @brief Handles failures due to HTTP-specific errors.
         * @param stream The network stream associated with the current request.
         * @param ex The HTTP error exception.
         */
        template <typename RouterT, typename ContextT>
        void fail(const RouterT& router, const std::string& target, ContextT ctx, const udho::http::error& ex){
            ctx.response().result(ex.status());

            auto layout = udho::pages::system::layouts::listing(ctx);

            namespace places = udho::pages::system::layouts::places;
            namespace placeholders = udho::pages::system::layouts::placeholders;

            layout[placeholders::header] = udho::pages::system::data::listing_header{ex.status()};
            layout[placeholders::footer] = udho::pages::system::data::status_info{};

            if(ex.status() == boost::beast::http::status::not_found) {
                if constexpr (!std::is_void_v<typename RouterT::mountpoints_type>){
                    layout[places::routes] = router.summary();
                }
            }
        }

        /**
         * @brief Handles failures due to unexpected standard exceptions.
         * @param address The IP address of the requester.
         * @param context The network stream associated with the current request.
         * @param ex The caught exception.
         */
        template <typename ContextT>
        void fail(boost::asio::ip::address address, ContextT ctx, const std::exception& ex){
            ctx.response().result(boost::beast::http::status::internal_server_error);
            ctx << udho::url::format("Error: {}", ex.what());
            ctx.finish();
        }
    private:
        boost::asio::io_context&          _io;
        boost::asio::ip::tcp::endpoint    _endpoint;
        std::shared_ptr<listener_type>    _listener;
};

// template <typename ListenerT>
// basic_server<ListenerT> server(boost::asio::io_context& io,  std::uint32_t port, const std::string& ip = "0.0.0.0"){
//     return basic_server<ListenerT>(io, port, ip);
// }

}
}

#endif // UDHO_NET_SERVER_H
