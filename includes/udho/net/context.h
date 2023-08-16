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

class context{
    using handler_type   = std::function<void (boost::system::error_code, std::size_t)>;

    template <typename ProtocolT>
    friend struct udho::net::connection;

    boost::asio::io_service&            _service;
    udho::net::bridge&                  _bridge;

    context() = delete;

    inline context(boost::asio::io_service& io, udho::net::bridge& bridge)
        : _service(io), _bridge(bridge)
        { }

    struct noop{
        void operator()(boost::system::error_code, std::size_t){}
    };

    public:
        context(const context&) = default;
        context(context&&) = default;

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
        template <typename ValueT>
        void set(const boost::beast::http::field& field, const ValueT& value){
            _bridge.set(field, value);
        }
        void flush(handler_type&& handler, bool only_headers = false){
            _bridge.flush(std::move(handler), only_headers);
        }
        void flush(bool only_headers = false){
            flush(noop{}, only_headers);
        }
};

}
}


#endif // UDHO_NET_CONTEXT_H
