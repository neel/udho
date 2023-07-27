#ifndef UDHO_NET_CONTEXT_H
#define UDHO_NET_CONTEXT_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/net/common.h>
#include <udho/net/bridge.h>
#include <udho/net/fwd.h>

namespace udho{
namespace net{

// boost::asio::bind_executor(_strand, std::bind(&self_type::on_read_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2))

class context{
    template <typename ProtocolT>
    friend struct udho::net::connection;

    boost::asio::io_service&            _service;
    udho::net::bridge&                  _bridge;

    context() = delete;
    context(const context&) = delete;
    inline context(boost::asio::io_service& io, udho::net::bridge& bridge)
        : _service(io), _bridge(bridge)
        { }

    public:
        context(context&& other) = default;

        inline const udho::net::types::headers::request& request() const { return _bridge.request(); }
        inline udho::net::types::headers::response& response() { return _bridge.response(); }

        template <typename ValueT>
        context& operator<<(const std::pair<boost::beast::http::field, ValueT>& header){
            _bridge << header;
            return *this;
        }
        template <typename StrT>
        context& operator<<(const StrT& str){
             _bridge << str;
            return *this;
        }
        void flush(bool only_headers = false){
            _bridge.flush(only_headers);
        }
};

}
}


#endif // UDHO_NET_CONTEXT_H
