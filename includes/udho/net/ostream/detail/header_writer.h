#ifndef UDHO_NET_OSTREAM_DETAIL_HEADER_WRITER_H
#define UDHO_NET_OSTREAM_DETAIL_HEADER_WRITER_H

#include <boost/asio/strand.hpp>
#include <udho/net/common.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>

namespace udho{
namespace net{

namespace detail{

/**
 * @brief Writes HTTP response headers once.
 *
 * Uses Beast serializer over an empty_body response constructed from `udho::net::types::headers::response`.
 * Ensures flush() is idempotent: subsequent calls are ignored once started.
 *
 * @tparam StreamT A Boost.Asio AsyncWriteStream.
 *
 * @thread_safety flush() dispatches on strand; safe from any thread.
 */
template <typename StreamT>
struct basic_header_writer{
    using stream_type               = StreamT;
    using executor_type             = typename stream_type::executor_type;
    using strand_type               = boost::asio::strand<executor_type>;
    using encoding_type             = udho::net::types::transfer_encoding;
    using completion_callback_type  = std::function<void (boost::system::error_code, std::size_t)>;
    using response_type             = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type           = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using opt_serializer_type       = std::optional<serializer_type>;

    basic_header_writer(stream_type& stream, strand_type& strand, response_type& response, completion_callback_type&& callback)
        : _stream(stream), _strand(strand), _response(response), _serializer(_response), _completion(std::move(callback)), _bytes_written(0), _started(false), _finished(false) {}

    /**
     * @brief Asynchronously write headers (only once).
     *
     * If already started, this is a no-op.
     */
    void flush() {
        boost::asio::dispatch(_strand, [this]() {
            if (_started) {
                return;
            }
            _started = true;
            // std::cout << "header started" << std::endl;
            boost::beast::http::async_write_header(
                _stream, *_serializer,
                boost::asio::bind_executor(_strand,
                       [this](boost::system::error_code ec, std::size_t bytes) {
                           // std::cout << "header ended" << std::endl;
                           _bytes_written = bytes;
                           _finished      = true;
                           if (_completion)
                               _completion(ec, bytes);
                       }
                    )
                );
        });
    }

    /// @brief True once a header write has been initiated.
    bool started() const { return _started; }

    /// @brief True once header write completion handler has run.
    bool finished() const { return _finished; }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after the response has been flushed to the socket and the
     *          completion callback has been called
     */
    void reset() {
        assert(_started);
        assert(_finished);
        assert(_serializer->is_header_done());

        _response.clear();
        _serializer.emplace(_response);

        _bytes_written  = 0;
        _started        = false;
        _finished       = false;
    }

private:
    stream_type&                    _stream;
    strand_type&                    _strand;
    response_type&                  _response;
    opt_serializer_type             _serializer;
    completion_callback_type        _completion;
    std::size_t                     _bytes_written;
    bool                            _started;
    bool                            _finished;
};


}

}
}

#endif // UDHO_NET_OSTREAM_DETAIL_HEADER_WRITER_H
