#ifndef UDHO_NET_H11_HEADER_READER_H
#define UDHO_NET_H11_HEADER_READER_H

#include <optional>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <udho/net/protocols/h11/detail.h>
#include <udho/utils/misc.h>

namespace udho{
namespace net{
namespace protocols{

namespace h11{

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Asynchronous HTTP/1.1 header reader with timeout support.
 *
 * This class reads the HTTP request headers from a stream using Boost.Beast's
 * `http::async_read_header`. It manages the lifetime of the parser and provides
 * a timeout mechanism: if the headers are not received within the specified
 * seconds, the stream is terminated and the handler is invoked with an error.
 *
 * The class holds a reference to the stream and to a buffer that will contain
 * the header data. After a successful read, the parsed header can be obtained
 * via the handler argument (as a `http_request_type` rvalue). The internal
 * buffer is left with any data beyond the headers (i.e., beginning of the body),
 * which can later be used by a body reader.
 *
 * @tparam StreamT The stream type (e.g., `boost::asio::ip::tcp::socket`).
 *                 Must satisfy the requirements of `boost::beast::async_read_header`.
 */
template <typename StreamT>
struct header_reader{
    using http_request_type         = boost::beast::http::header<true, boost::beast::http::fields>;
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using opt_http_req_parser_type  = std::optional<http_request_parser_type>;
    using stream_type               = StreamT;
    using timer_type                = boost::asio::steady_timer;

    /**
     * @brief Construct a header reader.
     * @param stream The underlying stream (must outlive the reader).
     * @param buffer A `flat_buffer` that will receive the header data.
     *               The buffer is passed by reference and must remain valid
     *               until the operation completes.
     *
     * @note The buffer may already contain data (e.g., from a previous read);
     *       `async_read_header` will append to it. After completion, the buffer
     *       holds the complete headers plus any extra bytes (body prefix).
     */
    header_reader(stream_type& stream, boost::beast::flat_buffer& buffer)
        : _stream(stream), _buffer(buffer), _finished(false), _timer(_stream.get_executor()) {}

    /**
     * @brief Start reading the HTTP headers.
     *
     * @param handler A callable with signature
     *                `void(http_request_type&&, boost::system::error_code, std::size_t)`.
     *                The first argument is the parsed request header (moved).
     *                The `size_t` is the number of bytes read from the socket
     *                (including any data already in the buffer).
     * @param seconds Total timeout in seconds. If the headers are not completely
     *                received within this time, the operation is aborted and the
     *                handler is invoked with `boost::asio::error::operation_aborted`.
     *
     * @note The parser is lazily constructed inside this call. The handler will
     *       be invoked exactly once.
     */
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _parser.emplace();
        assert(_parser.has_value());
        start_timer(seconds);
        boost::beast::http::async_read_header(
            _stream, _buffer, parser(),
            [this, handler = std::move(handler)] (boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                assert(_parser.has_value());
                finished(std::move(handler), ec, bytes_transferred);
            }
        );
    }

    /// @return `true` if the header read has completed (success or error).
    bool is_finished() const { return _finished; }

    /**
     * @brief Access the underlying Beast parser.
     * @return Reference to the `http_request_parser_type` instance.
     * @pre `start()` must have been called and the parser must be engaged.
     *
     * This can be used to inspect the parser state (e.g., after a successful read)
     * or to obtain additional information. It is primarily intended for internal
     * use or advanced scenarios.
     */
    http_request_parser_type& parser() { return *_parser; }

private:

    /// Starts the timeout timer.
    void start_timer(std::size_t seconds) {
        _timer.expires_after(std::chrono::seconds(seconds));
        _timer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                _timeout();
            }
        });
    }

    /// Timeout handler: forcibly terminates the stream.
    void _timeout(){
        udho::utils::misc::detail::stream_termination<StreamT>::apply(_stream);
    }

private:

    /**
     * @brief Completion handler for the asynchronous read.
     * @param handler           User‑provided handler.
     * @param ec                Result of the read operation.
     * @param bytes_transferred Number of bytes consumed from the stream.
     *
     * On success, extracts the parsed header from the parser and moves it into
     * `_request`. The user handler is then invoked. The timer is cancelled
     * unconditionally.
     */
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        if(!ec){
            assert(_parser.has_value());
            _request = std::move(_parser->release());
        }
        // _request is empty in case of error but it exists and it is legal to move it
        // std::cout << "h finished: " << ec.message() << std::endl;
        handler(std::move(_request), ec, bytes_transferred);
        _finished = true;
    }

private:
    http_request_type                   _request; // gets moved in finished
    stream_type&                        _stream;
    boost::beast::flat_buffer&          _buffer;
    opt_http_req_parser_type            _parser;
    bool                                _finished;
    timer_type                          _timer;
};

/** @} */

}


}
}
}

#endif // UDHO_NET_H11_HEADER_READER_H
