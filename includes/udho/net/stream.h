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
#include <udho/hazo/detail/is_streamable.h>
#include <udho/session/abstract_catalogue.h>
#include <udho/session/collect.h>

namespace udho{
namespace net{

namespace fake{
    struct stream;
}


/**
 * @brief stream is a copiable interface to the @ref bridge used for communicating the HTTP responses.
 * Copying the stream creates multiple streams reporting to the same bridge
 */
class stream{
    using handler_type   = std::function<void (boost::system::error_code, std::size_t)>;

    template <typename ProtocolT>
    friend struct connection;

    friend struct fake::stream;

    boost::asio::io_context&            _service;
    udho::net::bridge::ptr              _bridge;
    udho::session::abstract_catalogue&  _sessions;

    stream() = delete;

    protected:
    inline stream(boost::asio::io_context& io, udho::net::bridge::ptr bridge, udho::session::abstract_catalogue& sessions) : _service(io), _bridge(bridge), _sessions(sessions) { }

        struct noop{
            void operator()(boost::system::error_code, std::size_t){}
        };

    public:
        stream(const stream&) = default;
        stream(stream&& other): _service(other._service), _bridge(std::move(other._bridge)), _sessions(other._sessions) {}

        inline const udho::net::types::headers::request& request() const { return _bridge->request(); }
        inline udho::net::types::headers::response& response() { return _bridge->response(); }

        const udho::cookies::jar& cookies() const { return _bridge->cookies(); }
        udho::cookies::jar& cookies() { return _bridge->cookies(); }

        template <typename StrategyT>
        udho::session::collection::collector<StrategyT, boost::beast::http::fields> session(StrategyT&& strategy) {
            return udho::session::collection::collector<StrategyT, boost::beast::http::fields>{std::forward<StrategyT>(strategy), request(), response(), _sessions};
        }

        boost::asio::io_context& io() { return _service; }

        template <typename ValueT>
        stream& operator<<(const std::pair<boost::beast::http::field, ValueT>& header){
            *_bridge << header;
            return *this;
        }

        stream& operator<<(const std::exception& ex){
            *_bridge << ex.what();
            return *this;
        }

        template <typename T, std::enable_if_t< !std::is_pointer_v<T> && udho::hazo::detail::is_streamable<std::ostream, T>::value, bool> = true>
        stream& operator<<(const T& str){
             *_bridge << str;
            return *this;
        }
        template <typename CharT>
        stream& write(const CharT* str, std::size_t len){
             _bridge->write_latter(str, len);
            return *this;
        }
        template <typename Iterator>
        stream& write(Iterator begin, Iterator end){
             _bridge->write_latter(begin, end);
            return *this;
        }
        template <typename ValueT>
        void set(const boost::beast::http::field& field, const ValueT& value){
            _bridge->set(field, value);
        }
        inline void flush(handler_type&& handler, bool only_headers = false){
            _bridge->flush(std::move(handler), only_headers);
        }
        inline void flush(bool only_headers = false){
            flush(noop{}, only_headers);
        }
        inline void finish(){
            _bridge->finish();
        }
        inline void end(){
            _bridge->flush(std::bind(&stream::finish_, this, std::placeholders::_1, std::placeholders::_2));
        }
        inline void finish_(boost::system::error_code, std::size_t){
            finish();
        }
        inline void encoding(types::transfer::encoding enc) { _bridge->encoding(enc); }
        inline types::transfer::encoding encoding() const { return _bridge->encoding(); }
        inline void compression(types::transfer::compression compress) { _bridge->compression(compress); }
        inline types::transfer::compression compression() const { return _bridge->compression(); }
};

namespace fake{

struct stream{
    static udho::net::stream create(boost::asio::io_context& io, udho::net::bridge::ptr bridge, udho::session::abstract_catalogue& catalog){
        return udho::net::stream{io, bridge, catalog};
    }
};

}

}
}


#endif // UDHO_NET_STREAM_H
