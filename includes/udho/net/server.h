#ifndef UDHO_NET_SERVER_H
#define UDHO_NET_SERVER_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/stream.h>
#include <udho/net/context.h>
#include <udho/exceptions/exceptions.h>
#include <udho/url/summary.h>

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
    server(boost::asio::io_service& io, std::uint32_t port, const std::string& ip = "0.0.0.0"): _io(io), _endpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(ip), port)) {
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
            boost::beast::string_view tgt = stream.request().target();
            std::string target(tgt.begin(), tgt.end());
            bool found = false;
            try{
                found = router(target, context);
                if(!found){
                    throw udho::http::error(address, stream, boost::beast::http::status::not_found);
                }
            } catch(std::exception& ex) {
                fail(address, stream, ex);
            } catch(udho::http::exception& ex) {
                fail(stream, ex);
            } catch(udho::http::error& error) {
                fail(stream, error);
            }
        }

        /**
         * @brief Handles failures due to standard exceptions.
         * @param stream The network stream associated with the current request.
         * @param ex The caught exception.
         */
        void fail(udho::net::stream stream, const udho::http::exception& ex){
            stream << udho::url::format("Error: {}", ex.what());
            stream.finish();
        }

        /**
         * @brief Handles failures due to HTTP-specific errors.
         * @param stream The network stream associated with the current request.
         * @param ex The HTTP error exception.
         */
        void fail(udho::net::stream stream, const udho::http::error& ex){
            const udho::http::error& error = dynamic_cast<const udho::http::error&>(ex);
            stream.response().result(error.status());
            stream << udho::url::format("Error: {}", error.reason());
            stream.finish();
        }

        /**
         * @brief Handles failures due to unexpected standard exceptions.
         * @param address The IP address of the requester.
         * @param context The network stream associated with the current request.
         * @param ex The caught exception.
         */
        void fail(boost::asio::ip::address address, udho::net::stream context, const std::exception& ex){
            context << udho::url::format("Error: {}", ex.what());
            context.finish();
        }
    private:
        boost::asio::io_service&          _io;
        boost::asio::ip::tcp::endpoint    _endpoint;
        std::shared_ptr<listener_type>    _listener;
};

// template <typename ListenerT>
// basic_server<ListenerT> server(boost::asio::io_service& io,  std::uint32_t port, const std::string& ip = "0.0.0.0"){
//     return basic_server<ListenerT>(io, port, ip);
// }

}
}

#endif // UDHO_NET_SERVER_H
