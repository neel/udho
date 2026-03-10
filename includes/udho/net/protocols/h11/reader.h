#ifndef UDHO_NET_H11_READER_H
#define UDHO_NET_H11_READER_H

#include <memory>
#include <udho/net/protocols/h11/header_reader.h>
#include <udho/net/protocols/h11/body_reader.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>

namespace udho{
namespace net{
namespace protocols{

namespace h11{

/**
 * @brief Combined HTTP/1.1 reader that reads both headers and body sequentially.
 *
 * This class composes a `header_reader` and a `body_reader` to provide a complete
 * asynchronous interface for reading an HTTP request from a stream. It is designed
 * to be used with `std::shared_ptr` (inherits from `enable_shared_from_this`) and
 * manages the underlying buffer that is shared between the header and body phases.
 *
 * Typical usage:
 *   1. Call `start()` to read the request headers. The handler receives the parsed
 *      `http_request_type` (a Beast header) and an error code.
 *   2. After headers are successfully read, call one of the `upload()` methods
 *      (or convenience wrappers) to read the request body. The handler receives
 *      a buffer containing the body (or empty for multipart) and the total bytes
 *      read.
 *
 * The same internal buffer (`_header_buffer`) is used for both header and body
 * reads; after headers are parsed, any leftover data in the buffer is automatically
 * consumed by the body reader.
 *
 * @tparam StreamT The stream type (must satisfy the requirements of both
 *                 `header_reader` and `body_reader`).
 */
template <typename StreamT>
struct reader: public std::enable_shared_from_this<reader<StreamT>>{
    using http_request_type     = boost::beast::http::header<true, boost::beast::http::fields>;
    using header_reader_type    = h11::header_reader<StreamT>;
    using stream_type           = StreamT;

    /**
     * @brief Construct a reader associated with a stream.
     * @param stream The underlying stream (must outlive the reader).
     *
     * The internal header buffer is default‑constructed and will grow as needed.
     */
    explicit reader(stream_type& stream): _stream(stream), _header(_stream, _header_buffer) {}

    /**
     * @brief Get a const reference to the internal header buffer.
     * @return The buffer that currently holds (or held) the HTTP headers.
     *
     * After headers are read, this buffer may contain leftover body data. It is
     * used by the body reader and should not be modified externally.
     */
    const boost::beast::flat_buffer& buffer() const { return _header_buffer; }

public:

    /**
     * @brief Start reading the HTTP request headers.
     * @param handler A callable with signature
     *                `void(http_request_type&&, boost::system::error_code, std::size_t)`.
     *                The first argument is the parsed request header (moved).
     * @param seconds Timeout for header reading (see `header_reader::start`).
     *
     * This method forwards the call to the internal `header_reader`. After the
     * headers are successfully read, the handler is invoked. The same reader
     * instance can then be used to read the body via `upload()`.
     */
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _header.start([h = std::move(handler), this](http_request_type&& request, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            h(std::move(request), ec, bytes_transferred);
        }, seconds);
    }

    /**
     * @brief Read the HTTP request body after headers have been parsed.
     *
     * @tparam Buffer The buffer type for the body (e.g., `boost::beast::flat_buffer`).
     * @param request The parsed HTTP request headers (obtained from the `start` handler).
     * @param handler Completion handler with signature
     *                `void(Buffer&&, boost::system::error_code, std::size_t)`.
     *                The buffer contains the body (for non‑multipart) or is empty
     *                (for multipart); the `size_t` is the total raw bytes read.
     * @param seconds Total timeout for body reading.
     * @param limit   Maximum allowed body size (raw bytes).
     *
     * @pre `request` must be the same request that was previously parsed by `start()`.
     * @note The internal header buffer is passed to the body reader, which will
     *       consume any leftover data from the headers. The buffer is then reused
     *       during body reading.
     */
    template <typename Handler, typename Buffer>
    void upload(const http_request_type& request, Handler&& handler, const udho::net::detail::body_parser_config& config){
        using body_reader_type = udho::net::protocols::h11::body_reader<Buffer, StreamT>;

        auto body = std::make_shared<body_reader_type>(request, _stream, config);
        body->start([h = std::move(handler), this, &body](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            auto result = body->release(_header_buffer);
            // moving buffer is always legal irrespective of ec
            // std::cout << "ec.message(): " << ec.message() << std::endl;
            // h(std::move(result.release_buffer()), ec, bytes_transferred);
            h(std::move(result), ec, bytes_transferred);
            // transfer body buffer to header buffer
        }, _header_buffer);
    }

    /**
     * @brief Convenience overload that reads the body into a `boost::beast::flat_buffer`.
     * @copydoc upload
     */
    template <typename Handler>
    void upload_to_flat_buffer(const http_request_type& request, Handler&& handler, const udho::net::detail::body_parser_config& config){
        upload<Handler, boost::beast::flat_buffer>(request, std::forward<Handler>(handler), config);
    }

    /**
     * @brief Convenience overload that reads the body into a `boost::beast::multi_buffer`.
     * @copydoc upload
     */
    template <typename Handler>
    void upload_to_multi_buffer(const http_request_type& request, Handler&& handler, const udho::net::detail::body_parser_config& config){
        upload<Handler, boost::beast::multi_buffer>(request, std::forward<Handler>(handler), config);
    }

public:

    /// @return Reference to the internal header reader.
    header_reader_type& header() { return _header; }

    /// @return Const reference to the internal header reader.
    const header_reader_type& header() const { return _header; }
private:
    stream_type&                  _stream;
    boost::beast::flat_buffer     _header_buffer;
    header_reader_type            _header;
};

}

}
}
}

#endif // UDHO_NET_H11_READER_H
