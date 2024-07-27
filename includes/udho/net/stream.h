#ifndef UDHO_NET_STREAM_H
#define UDHO_NET_STREAM_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/net/common.h>
#include <udho/net/bridge.h>
#include <udho/net/fwd.h>
#include <udho/hazo/string/basic.h>

namespace udho{
namespace net{

/**
 * @brief context is a copiable handle that bridges with the connection object associated with the http request
 * It facilitates sending, flushing and finishing the response. It also provides functionality for providing the
 * transfer encoding of the response. The equest and the response objects can be accessed through the connection.
 * @note the context object may be copied across multiple callbacks while using chunked transfer encoding.
 *       from callback1 one may call `context.flush(std::bind(&callback2, context))` which will call the callback2
 *       function once the already written contents are flushed out.
 */
class stream{
    using handler_type   = std::function<void (boost::system::error_code, std::size_t)>;

    template <typename ProtocolT>
    friend struct connection;

    boost::asio::io_service&            _service;
    udho::net::bridge&                  _bridge;

    stream() = delete;

    protected:
        inline stream(boost::asio::io_service& io, udho::net::bridge& bridge) : _service(io), _bridge(bridge) { }

        struct noop{
            void operator()(boost::system::error_code, std::size_t){}
        };

    public:
        stream(const stream&) = default;
        stream(stream&&) = default;

        inline const udho::net::types::headers::request& request() const { return _bridge.request(); }
        inline udho::net::types::headers::response& response() { return _bridge.response(); }

        template <typename ValueT>
        stream& operator<<(const std::pair<boost::beast::http::field, ValueT>& header){
            _bridge << header;
            return *this;
        }
        template <typename StrT>
        stream& operator<<(const StrT& str){
             _bridge << str;
            return *this;
        }
        template <typename ValueT>
        void set(const boost::beast::http::field& field, const ValueT& value){
            _bridge.set(field, value);
        }
        inline void flush(handler_type&& handler, bool only_headers = false){
            _bridge.flush(std::move(handler), only_headers);
        }
        inline void flush(bool only_headers = false){
            flush(noop{}, only_headers);
        }
        inline void finish(){
            _bridge.finish();
        }
        inline void end(){
            _bridge.flush(std::bind(&stream::finish_, this, std::placeholders::_1, std::placeholders::_2));
        }
        inline void finish_(boost::system::error_code, std::size_t){
            finish();
        }
        inline void encoding(types::transfer::encoding enc) { _bridge.encoding(enc); }
        inline types::transfer::encoding encoding() const { return _bridge.encoding(); }
        inline void compression(types::transfer::compression compress) { _bridge.compression(compress); }
        inline types::transfer::compression compression() const { return _bridge.compression(); }
};

}
}


#endif // UDHO_NET_STREAM_H
