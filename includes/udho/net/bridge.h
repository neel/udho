#ifndef UDHO_NET_BRIDGE_H
#define UDHO_NET_BRIDGE_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <udho/net/common.h>
#include <udho/net/context.h>
#include <chrono>

namespace udho{
namespace net{


struct bridge{
    using flash_callback = std::function<void (bool)>;

    const udho::net::types::headers::request&  _request;
    udho::net::types::headers::response&       _response;
    std::ostream&                              _stream;
    flash_callback                             _flush;

    inline bridge(const udho::net::types::headers::request& request, udho::net::types::headers::response& response, std::ostream& stream, flash_callback&& flush)
        : _request(request), _response(response), _stream(stream), _flush(std::move(flush))
        {}

    bridge(const bridge&) = delete;
    bridge(bridge&&) = default;

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
    void flush(bool only_headers = false){
        _flush(only_headers);
    }
};


}
}

#endif // UDHO_NET_BRIDGE_H
