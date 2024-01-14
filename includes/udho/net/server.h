#ifndef UDHO_NET_SERVER_H
#define UDHO_NET_SERVER_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/context.h>
#include <udho/exceptions/exceptions.h>

namespace udho{
namespace net{

template <typename ListenerT, typename RouterT>
struct server_{
    using listener_type = ListenerT;
    using router_type   = RouterT;
    using server_type   = server_<ListenerT, RouterT>;

    server_(boost::asio::io_service& io, const router_type& router, std::uint32_t port, const std::string& ip = "0.0.0.0"): _io(io), _router(router), _endpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(ip), port)) {
        _listener = std::make_shared<listener_type>(_io, _endpoint);
    }

    void run(){
        _listener->listen(std::bind(&server_type::serve, this, std::placeholders::_1, std::placeholders::_2));
    }

    void stop(){
        _listener->stop();
    }

    private:
        void prepare(boost::asio::ip::address address, udho::net::context context){
            context.set(boost::beast::http::field::server, "udho");
        }
        void serve(boost::asio::ip::address address, udho::net::context&& context){
            prepare(address, context);
            boost::beast::string_view tgt = context.request().target();
            std::string target(tgt.begin(), tgt.end());
            bool found = false;
            try{
                found = _router(target, context);
                if(!found){
                    throw udho::http::error(address, context, boost::beast::http::status::not_found);
                }
            } catch(std::exception& ex) {
                fail(address, context, ex);
            } catch(udho::http::exception& ex) {
                fail(context, ex);
            } catch(udho::http::error& error) {
                fail(context, error);
            }
        }
        void fail(udho::net::context context, const udho::http::exception& ex){
            context << udho::url::format("Error: {}", ex.what());
            context.finish();
        }
        void fail(udho::net::context context, const udho::http::error& ex){
            const udho::http::error& error = dynamic_cast<const udho::http::error&>(ex);
            context.response().result(error.status());
            context << udho::url::format("Error: {}", error.reason());
            context.finish();
        }
        void fail(boost::asio::ip::address address, udho::net::context context, const std::exception& ex){
            context << udho::url::format("Error: {}", ex.what());
            context.finish();
        }
    private:
        boost::asio::io_service&       _io;
        const router_type&             _router;
        boost::asio::ip::tcp::endpoint _endpoint;
        std::shared_ptr<listener_type> _listener;
};

template <typename ListenerT, typename RouterT>
server_<ListenerT, RouterT> server(boost::asio::io_service& io, const RouterT& router, std::uint32_t port, const std::string& ip = "0.0.0.0"){
    return server_<ListenerT, RouterT>(io, router, port, ip);
}

}
}

#endif // UDHO_NET_SERVER_H
