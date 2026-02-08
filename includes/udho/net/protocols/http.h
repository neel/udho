#ifndef UDHO_NET_PROTOCOL_HTTP_H
#define UDHO_NET_PROTOCOL_HTTP_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <udho/net/common.h>
#include <boost/beast/core/static_buffer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

namespace udho{
namespace net{
namespace protocols{

namespace detail{

template <typename StreamT>
struct stream_termination{
    static boost::system::error_code apply(StreamT& stream) {
        boost::system::error_code error;
        stream.cancel(error);
        return error;
    }
};

template <>
struct stream_termination<boost::beast::test::stream>{
    static boost::system::error_code apply(boost::beast::test::stream& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
    }
};

}

template <typename StreamT>
struct http_header_reader{
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using opt_http_req_parser_type  = std::optional<http_request_parser_type>;
    using stream_type               = StreamT;
    using timer_type                = boost::asio::steady_timer;

    http_header_reader(stream_type& stream, boost::beast::flat_buffer& buffer)
        : _stream(stream), _buffer(buffer), _finished(false), _timer(_stream.get_executor()) {}

    /**
     * @brief start reading HTTP headers asynchronously
     * @param handler must take the following arguments (udho::net::types::headers::request&& request, std::error_code ec, std::size_t bytes_transferred)
     * @param seconds
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

    bool is_finished() const { return _finished; }

    http_request_parser_type& parser() { return *_parser; }

private:
    void start_timer(std::size_t seconds) {
        _timer.expires_after(std::chrono::seconds(seconds));
        _timer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                _timeout();
            }
        });
    }

    void _timeout(){
        detail::stream_termination<StreamT>::apply(_stream);
    }

private:
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        if(!ec){
            assert(_parser.has_value());
            _request = std::move(_parser->release());
        }
        // _request is empty in case of error but it exists and it is legal to move it
        std::cout << "h finished: " << ec.message() << std::endl;
        handler(std::move(_request), ec, bytes_transferred);
        _finished = true;
    }

private:
    udho::net::types::headers::request  _request; // gets moved in finished
    stream_type&                        _stream;
    boost::beast::flat_buffer&          _buffer;
    opt_http_req_parser_type            _parser;
    bool                                _finished;
    timer_type                          _timer;
};

namespace detail{

template <typename Buffer>
struct transfer_leftover{
    using target_buffer_type = Buffer;

    transfer_leftover(target_buffer_type& target): _target(target) {}

    target_buffer_type& operator()(boost::beast::flat_buffer& source, std::size_t content_length) {
        auto src   = source.data();
        auto limit = std::min(content_length, source.size());
        _target.commit(boost::asio::buffer_copy(_target.prepare(limit), src));
        source.consume(limit);
        _bytes_transferred = limit;
        return _target;
    }

    std::size_t bytes_transferred() const { return _bytes_transferred; }
private:
    target_buffer_type& _target;
    std::size_t         _bytes_transferred;
};

};

template <typename Buffer, typename StreamT>
struct http_body_reader_asio: std::enable_shared_from_this<http_body_reader_asio<Buffer, StreamT>> {
    using stream_type  = StreamT;
    using buffer_type  = Buffer;
    using timer_type   = boost::asio::steady_timer;
    using request_type = udho::net::types::headers::request;

    http_body_reader_asio(const udho::net::types::headers::request& request, stream_type& stream, std::size_t buffer_capacity = std::allocator_traits<typename Buffer::allocator_type>::max_size(typename Buffer::allocator_type{}))
        : _request(request), _stream(stream), _buffer(buffer_capacity), _finished(false), _timer(_stream.get_executor()) {}

    template <typename Handler>
    void start(Handler&& handler, boost::beast::flat_buffer& hbuff, std::size_t seconds, std::size_t limit = 0){
        std::size_t content_length = 0;
        if(_request.count(boost::beast::http::field::content_length)) {
            try{
                content_length = std::stoul(_request.at(boost::beast::http::field::content_length));
            } catch(const std::exception& ex) {
                // TODO log the exception after loggin module is written
                finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::invalid_argument), 0);
                return;
            }
        }

        // { sanity
        // 0 < content_length < limit will be checked before the body reader is instantiated
        // If content_length = 0 or Chunked body or content_length > limit then this body reader won't be used
        if(content_length == 0) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), 0);
            return;
        }

        if(content_length > limit) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::value_too_large), 0);
            return;
        }
        // }

        std::size_t transferred = transfer_leftovers(hbuff, content_length);
        assert(content_length >= transferred);
        std::size_t pending_size  = content_length - transferred;

        if(pending_size == 0) {
            finished(std::move(handler), boost::system::error_code{}, content_length);
        } else {
            start_timer(seconds);
            read_into(std::move(handler), pending_size);
        }
    }

    bool is_finished() const { return _finished; }

    std::shared_ptr<http_body_reader_asio> self() { return std::enable_shared_from_this<http_body_reader_asio<Buffer, StreamT>>::shared_from_this(); }
private:

    std::size_t transfer_leftovers(boost::beast::flat_buffer& hbuff, std::size_t content_length) {
        detail::transfer_leftover transfer(_buffer);
        transfer(hbuff, content_length);
        return transfer.bytes_transferred();
    }
private:

    template <typename Handler>
    void read_into(Handler&& handler, std::size_t pending_size) {
        boost::asio::async_read(
            _stream, _buffer, boost::asio::transfer_exactly(pending_size),
            [self = self(), handler = std::move(handler)](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                self->finished(std::move(handler), ec, bytes_transferred);
            }
        );
    }
private:
    void start_timer(std::size_t seconds) {
        _timer.expires_after(std::chrono::seconds(seconds));
        _timer.async_wait([self = self()](boost::system::error_code ec) {
            if (!ec) {
                self->_timeout();
            }
        });
    }

    void _timeout(){
        detail::stream_termination<StreamT>::apply(_stream);
    }

private:
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        std::cout << "b ec.message(): " << ec.message() << std::endl;
        handler(std::move(_buffer), ec, bytes_transferred);
        _finished = true;
    }
private:
    const request_type& _request;
    stream_type&        _stream;
    buffer_type         _buffer;
    bool                _finished;
    timer_type          _timer;
};

template <typename StreamT>
struct http_reader2: public std::enable_shared_from_this<http_reader2<StreamT>>{
    using header_reader_type    = http_header_reader<StreamT>;
    using stream_type           = StreamT;

    explicit http_reader2(stream_type& stream): _stream(stream), _header(_stream, _header_buffer) {}

public:
    const boost::beast::flat_buffer& buffer() const { return _header_buffer; }
public:
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _header.start([h = std::move(handler), this](udho::net::types::headers::request&& request, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            h(std::move(request), ec, bytes_transferred);
        }, seconds);
    }

    /**
     * @brief reads the contents part of an HTTP request
     * @pre expects the request has already been parsed
     * @param request HTTP request
     * @param handler must take the following arguments (buffer_type&& buffer, std::error_code ec, std::size_t bytes_transferred)
     * @param seconds
     * @param limit
     */
    template <typename Handler, typename Buffer>
    void upload(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        using body_reader_type = http_body_reader_asio<Buffer, StreamT>;

        auto body = std::make_shared<body_reader_type>(request, _stream);
        body->start([h = std::move(handler)](Buffer&& buffer, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            // moving buffer is always legal irrespective of ec
            std::cout << "ec.message(): " << ec.message() << std::endl;
            h(std::move(buffer), ec, bytes_transferred);
        }, _header_buffer, seconds, limit);
    }

    template <typename Handler>
    void upload_to_flat_buffer(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        upload<Handler, boost::beast::flat_buffer>(request, std::forward<Handler>(handler), seconds, limit);
    }

    template <typename Handler>
    void upload_to_multi_buffer(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        upload<Handler, boost::beast::multi_buffer>(request, std::forward<Handler>(handler), seconds, limit);
    }

public:
    header_reader_type& header() { return _header; }
    const header_reader_type& header() const { return _header; }
private:
    stream_type&                  _stream;
    boost::beast::flat_buffer     _header_buffer;
    header_reader_type            _header;
};

template <typename Handler, typename StreamT>
struct http_writer_internal: public std::enable_shared_from_this<http_writer_internal<Handler, StreamT>>{
    using self_type             = http_writer_internal<Handler, StreamT>;
    using handler_type          = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type = udho::net::types::headers::response;
    using response_type         = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type       = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type           = StreamT;

    explicit http_writer_internal(boost::asio::io_context& io, udho::net::types::strand& strand, const types::headers::response& headers, stream_type& stream, Handler&& handler)
        : _io(io), _strand(strand), _headers(headers), _response(_headers), _serializer(_response), _stream(stream), _handler(std::move(handler)) {}
    http_writer_internal(const http_writer_internal&) = delete;
    ~http_writer_internal() { std::cout << "~http_writer_internal" << std::endl; }

    void start() {
        boost::beast::http::async_write_header(
            _stream, _serializer,
            std::bind(&self_type::finished, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    void finished(boost::system::error_code ec, std::size_t bytes_transferred){
        boost::asio::dispatch(_io,
            boost::asio::bind_executor(
                _strand,
                std::bind(std::move(_handler), ec, bytes_transferred)
            )
        );
    }
    private:
        auto shared_from_this() {
            return std::enable_shared_from_this<self_type>::shared_from_this();
        }

        auto weak_from_this() {
            return std::enable_shared_from_this<self_type>::weak_from_this();
        }

    private:
        boost::asio::io_context&        _io;
        udho::net::types::strand&       _strand;
        const response_headers_type&    _headers;
        response_type                   _response;
        serializer_type                 _serializer;
        stream_type&                    _stream;
        handler_type                    _handler;
};

template <typename StreamT>
struct http_writer{
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using response_type   = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type     = StreamT;

    explicit http_writer(const types::headers::response& headers, stream_type& stream): _headers(headers), _stream(stream) {}
    http_writer(const http_writer&) = delete;
    ~http_writer() { std::cout << "~http_writer" << std::endl; }

    template <typename Handler>
    void operator()(boost::asio::io_context& io, udho::net::types::strand& strand_write, udho::net::types::strand& strand_finished, Handler&& handler){
        using internal_writer_type = http_writer_internal<Handler, stream_type>;
        auto internal_writer = std::make_shared<internal_writer_type>(io, strand_finished, _headers, _stream, std::move(handler));
        boost::asio::dispatch(io,
            boost::asio::bind_executor(
                strand_write,
                std::bind(&internal_writer_type::start, internal_writer)
            )
        );
    }

    private:
        const udho::net::types::headers::response& _headers;
        stream_type&                               _stream;
};

}
}
}

#endif // UDHO_NET_PROTOCOL_HTTP_H

