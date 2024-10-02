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

/**
 * @brief serves as an bridge between the @ref connection and @ref stream.
 * An intermediary that handles HTTP requests and responses. It provides functionalities to write to streams,
 * flush and finish HTTP transactions, and manipulate headers and transfer encoding settings.
 * @note non-copyable.
 */
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

    /**
     * Constructor initializing the bridge with all necessary components for handling a HTTP transaction.
     * @param request Reference to the client's request.
     * @param response Reference to the server's response to be manipulated.
     * @param stream Output stream where the response data is written.
     * @param encoding Encoding settings for the data transfer.
     * @param flush Callback function to handle flushing data.
     * @param finish Callback function to finalize the response.
     */
    inline bridge(const udho::net::types::headers::request& request, udho::net::types::headers::response& response, std::ostream& stream, types::transfer_encoding& encoding, flush_callback&& flush, finish_callback&& finish)
        : _request(request), _response(response), _stream(stream), _transfer_encoding(encoding), _flush(std::move(flush)), _finish(finish)
        {}

    bridge(const bridge&) = delete;
    bridge(bridge&& other): _request(other._request), _response(other._response), _stream(other._stream), _transfer_encoding(other._transfer_encoding), _flush(std::move(other._flush)) {}

    /**
     * @brief Accessor for the request headers.
     * @return Constant reference to the request headers.
     */
    const udho::net::types::headers::request& request() const { return _request; }
    /**
     * @brief Accessor for the response headers.
     * @return reference to the response headers.
     */
    udho::net::types::headers::response& response() const { return _response; }

    /**
     * @brief Writes data to the output stream.
     * @param str Data to write.
     */
    template <typename CharT>
    void write_latter(const std::basic_string<CharT>& str){
        std::copy(str.begin(), str.end(), std::ostream_iterator<CharT>(_stream));
    }

    /**
     * @brief Adds or modifies a header in the response.
     * @param header Pair consisting of the HTTP field to modify and its value.
     * @return Reference to this bridge object.
     */
    template <typename ValueT>
    bridge& operator<<(const std::pair<boost::beast::http::field, ValueT>& header){
        _response[header.first] = header.second;
        return *this;
    }

    /**
     * @brief Writes a string to the response.
     * @param str String to write.
     * @return Reference to this bridge object.
     */
    template <typename CharT>
    bridge& operator<<(const std::basic_string<CharT>& str){
        write_latter<CharT>(str);
        return *this;
    }

    /**
     * Writes a C-style string to the response.
     * @param str C-style string to write.
     * @return Reference to this bridge object.
     */
    template <typename CharT>
    bridge& operator<<(const CharT* str){
        write_latter<CharT>(std::basic_string<CharT>(str));
        return *this;
    }

    /**
     * @brief Sets a specific HTTP header to a value.
     * @param field HTTP header field to set.
     * @param value Value to set the field to.
     */
    template <typename ValueT>
    void set(const boost::beast::http::field& field, const ValueT& value){
        _response.set(field, value);
    }

    /**
     * @brief Flushes the current buffer to the network, optionally only headers.
     * @param handler Callback to be invoked after flushing.
     * @param only_headers Whether to flush only headers (true) or all data (false).
     */
    void flush(handler_type&& handler, bool only_headers = false){
        _flush(handler, only_headers);
    }

    /**
     * @brief Finalizes the response, indicating that no further writing will occur.
     */
    void finish(){
        _finish();
    }

    /**
     * @brief Sets the transfer encoding.
     * @param enc Encoding to set.
     */
    inline void encoding(types::transfer::encoding enc) { _transfer_encoding.encoding(enc); }
    /**
     * @brief Gets the current transfer encoding.
     * @return The current encoding setting.
     */
    inline types::transfer::encoding encoding() const { return _transfer_encoding.encoding(); }
    /**
     * @brief Sets the compression method.
     * @param compress Compression method to use.
     */
    inline void compression(types::transfer::compression compress) { _transfer_encoding.compression(compress); }
    /**
     * @brief Gets the current compression setting.
     * @return The current compression setting.
     */
    inline types::transfer::compression compression() const { return _transfer_encoding.compression(); }
};


}
}

#endif // UDHO_NET_BRIDGE_H
