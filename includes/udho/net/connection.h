#ifndef UDHO_NET_CONNECTION_H
#define UDHO_NET_CONNECTION_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <udho/net/common.h>
#include <udho/net/context.h>
#include <udho/net/bridge.h>
#include <chrono>

namespace udho{
namespace net{


/**
 * \brief A connection object wraps a socket.
 * Follows a protocol (e.g. HTTP, FastCGI, SCGI, wscgi etc..) to parse the headers.
 * Uses ProtocolT to follow the protocol and prepare a request object.
 * Once the request object is created it passes that to an asynchronous resolver to resolve a slot that will process that request.
 */
template <typename ProtocolT>
struct connection: public std::enable_shared_from_this<connection<ProtocolT>>{
    using protocol_type   = ProtocolT;
    using reader_type     = typename protocol_type::reader;
    using writer_type     = typename protocol_type::writer;
    using clock_type      = std::chrono::time_point<std::chrono::system_clock>;
    using self_type       = connection<ProtocolT>;
    using connection_type = connection<ProtocolT>;
    using processer_type  = std::function<void (udho::net::context&&)>;
    // using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;


    connection(boost::asio::io_service& service, udho::net::types::socket socket)
      : _stage(types::stages::accepted), _bytes_read(0), _bytes_written(0),
        _io(service), _socket(std::move(socket)), _strand(_socket.get_executor()),
        _reader(std::make_shared<reader_type>(_request)), _writer(std::make_shared<writer_type>(_response)),
        _stream(&_streambuf)
    {}
    void start(processer_type&& processor){
        _processor = std::move(processor);
        _start = std::chrono::system_clock::now();
        _reader->start(
            _socket,
            std::bind(&self_type::on_read_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    private:
        template <typename Handler>
        struct on_flush_header{
            connection_type& _connection;
            Handler          _handler;

            on_flush_header(connection_type& conn, Handler&& handler): _connection(conn), _handler(std::move(handler)) {}
            void operator()(boost::system::error_code ec, std::size_t bytes_transferred){
                if(ec){
                    _connection._stage = types::stages::error;
                    return;
                }
                _connection._stage = types::stages::headers_written;
                _connection._bytes_written += bytes_transferred;
                _handler(ec, bytes_transferred);
            }
        };

        auto shared_from_this() {
            return std::enable_shared_from_this<self_type>::shared_from_this();
        }

        void on_read_header(boost::system::error_code ec, std::size_t bytes_transferred){
            if(ec){
                _stage = types::stages::rejected;
                return;
            }
            _stage = types::stages::headers_read;
            _io.post(std::bind(&self_type::process, shared_from_this()));
            _bytes_read += bytes_transferred;
        }

        void process(){
            udho::net::bridge bridge(
                _request, _response, _stream,
                std::bind(&self_type::flush, shared_from_this(), std::placeholders::_1)
            );
            udho::net::context context(_io, std::move(bridge));
            _processor(std::move(context));
        }

        void prepare_headers(bool only_headers){
            auto transfer_encoding = _response[boost::beast::http::field::transfer_encoding];
            // transfer encoding not specified
            // transfer encoding     specified but doesn't contain 'chunked'
            //                       while body is empty (which implies body will be written in future)
            // set transfer encoding to chunked
            if((only_headers && transfer_encoding.empty()) || (transfer_encoding.find("chunked") == boost::beast::string_view::npos && _streambuf.size() == 0)){
                std::string value = transfer_encoding;
                if(!value.empty()){
                    value = value + ", ";
                }
                value = value + "chunked";
                _response.set(boost::beast::http::field::transfer_encoding, value);
            }else if(!only_headers && _streambuf.size() != 0){
                _response.set(boost::beast::http::field::content_length, std::to_string(_streambuf.size()));
            }
        }
        template <typename Handler>
        void flush_headers(Handler&& handler){
            _writer->start(_strand, _socket, on_flush_header<Handler>(*this, std::move(handler)));
        }
        void flush_headers(){
            _writer->start(
                _strand, _socket,
                std::bind(&self_type::on_flush_header_end, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
            );
        }

        void on_flush_header_end(boost::system::error_code ec, std::size_t bytes_transferred){
            if(ec){
                _stage = types::stages::error;
                return;
            }
            _stage = types::stages::headers_written;
            _bytes_written += bytes_transferred;
        }

        void flush_body(){
            boost::asio::async_write(
                _socket, _streambuf,
                boost::asio::bind_executor(_strand, std::bind(&self_type::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2))
            );
        }

        void on_write(boost::system::error_code ec, std::size_t bytes_transferred){
            if(ec){
                _stage = types::stages::error;
                return;
            }
            _stage = types::stages::body_written;
            _bytes_written += bytes_transferred;
        }

        void flush(bool only_headers = false){
            if(_stage < types::stages::headers_written){
                prepare_headers(only_headers);
                if(!only_headers){
                    flush_headers(std::bind(&self_type::flush_body, shared_from_this()));
                }else{
                    flush_headers();
                }
            }else{
                flush_body();
            }
        }
    private:
        udho::net::types::stages            _stage;
        std::size_t                         _bytes_read, _bytes_written;
        clock_type                          _start, _end;
        boost::asio::io_service&            _io;
        processer_type                      _processor;
        udho::net::types::socket            _socket;
        udho::net::types::strand            _strand;
        udho::net::types::headers::request  _request;
        udho::net::types::headers::response _response;
        std::shared_ptr<reader_type>        _reader;
        std::shared_ptr<writer_type>        _writer;
        boost::asio::streambuf              _streambuf;
        std::ostream                        _stream;
};



}
}

#endif // UDHO_NET_CONNECTION_H
