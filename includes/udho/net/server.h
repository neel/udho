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

template <typename ListenerT>
struct server{
    using listener_type = ListenerT;
    using server_type   = server<ListenerT>;

    server(boost::asio::io_service& io, std::uint32_t port, const std::string& ip = "0.0.0.0"): _io(io), _endpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(ip), port)) {
        _listener = std::make_shared<listener_type>(_io, _endpoint);
    }

    template <typename ArtifactsT>
    void run(const ArtifactsT& artifacts){
        _listener->listen(std::bind(&server_type::serve<ArtifactsT>, this, std::placeholders::_1, std::placeholders::_2, artifacts));
    }

    void stop(){
        _listener->stop();
    }

    private:
        void prepare(boost::asio::ip::address address, udho::net::stream context){
            context.set(boost::beast::http::field::server, "udho");
        }
        template <typename ArtifactsT>
        void serve(boost::asio::ip::address address, udho::net::stream&& stream, const ArtifactsT& artifacts){
            using router_type = typename ArtifactsT::router_type;
            using resource_store_proxy_type = typename ArtifactsT::resource_store_proxy_type;

            const router_type& router = artifacts.router();

            udho::url::summary::router summary = router.summary();
            udho::net::basic_context<resource_store_proxy_type> context{std::move(stream), summary, artifacts.resources()};

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
        void fail(udho::net::stream stream, const udho::http::exception& ex){
            stream << udho::url::format("Error: {}", ex.what());
            stream.finish();
        }
        void fail(udho::net::stream stream, const udho::http::error& ex){
            const udho::http::error& error = dynamic_cast<const udho::http::error&>(ex);
            stream.response().result(error.status());
            stream << udho::url::format("Error: {}", error.reason());
            stream.finish();
        }
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
