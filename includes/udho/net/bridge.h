#ifndef UDHO_NET_BRIDGE_H
#define UDHO_NET_BRIDGE_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>
#include <udho/net/stream.h>
#include <chrono>

namespace udho{
namespace net{


struct bridge{
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using flush_callback  = std::function<void (handler_type, bool)>;
    using finish_callback = std::function<void ()>;

    const udho::net::types::headers::request&  _request;
    udho::net::types::headers::response&       _response;
    std::ostream&                              _stream;
    types::transfer_encoding&                  _transfer_encoding;
    flush_callback                             _flush;
    finish_callback                            _finish;

    inline bridge(const udho::net::types::headers::request& request, udho::net::types::headers::response& response, std::ostream& stream, types::transfer_encoding& encoding, flush_callback&& flush, finish_callback&& finish)
        : _request(request), _response(response), _stream(stream), _transfer_encoding(encoding), _flush(std::move(flush)), _finish(finish)
        {}

    bridge(const bridge&) = delete;
    bridge(bridge&& other): _request(other._request), _response(other._response), _stream(other._stream), _transfer_encoding(other._transfer_encoding), _flush(std::move(other._flush)) {}

    const udho::net::types::headers::request& request() const { return _request; }
    udho::net::types::headers::response& response() const { return _response; }

    template <typename CharT>
    void write_latter(const std::basic_string<CharT>& str){
        std::copy(str.begin(), str.end(), std::ostream_iterator<CharT>(_stream));
    }
    template <typename ValueT>
    bridge& operator<<(const std::pair<boost::beast::http::field, ValueT>& header){
        _response[header.first] = header.second;
        return *this;
    }
    template <typename CharT>
    bridge& operator<<(const std::basic_string<CharT>& str){
        write_latter<CharT>(str);
        return *this;
    }
    template <typename CharT>
    bridge& operator<<(const CharT* str){
        write_latter<CharT>(std::basic_string<CharT>(str));
        return *this;
    }
    template <typename ValueT>
    void set(const boost::beast::http::field& field, const ValueT& value){
        _response.set(field, value);
    }
    void flush(handler_type&& handler, bool only_headers = false){
        _flush(handler, only_headers);
    }
    void finish(){
        _finish();
    }

    inline void encoding(types::transfer::encoding enc) { _transfer_encoding.encoding(enc); }
    inline types::transfer::encoding encoding() const { return _transfer_encoding.encoding(); }
    inline void compression(types::transfer::compression compress) { _transfer_encoding.compression(compress); }
    inline types::transfer::compression compression() const { return _transfer_encoding.compression(); }
};


}
}

#endif // UDHO_NET_BRIDGE_H
