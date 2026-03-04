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

namespace udho{
namespace net{
namespace protocols{

namespace h11{

template <typename StreamT>
struct header_reader{
    using http_request_type         = boost::beast::http::header<true, boost::beast::http::fields>;
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using opt_http_req_parser_type  = std::optional<http_request_parser_type>;
    using stream_type               = StreamT;
    using timer_type                = boost::asio::steady_timer;


    header_reader(stream_type& stream, boost::beast::flat_buffer& buffer)
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

}


}
}
}

#endif // UDHO_NET_H11_HEADER_READER_H
