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
struct connection: public std::enable_shared_from_this<connection<ProtocolT>>, private bridge{
    using protocol_type  = ProtocolT;
    using reader_type    = typename protocol_type::reader;
    using writer_type    = typename protocol_type::writer;
    using clock_type     = std::chrono::time_point<std::chrono::system_clock>;
    using self_type      = connection<ProtocolT>;
    using processer_type = std::function<void (udho::net::context&&)>;
    using handler_type   = std::function<void (boost::system::error_code, std::size_t)>;


    connection(boost::asio::io_service& service, udho::net::types::socket socket)
      : bridge(_request, _response, _streambuf, std::bind(&self_type::flush, std::enable_shared_from_this<self_type>::shared_from_this(), std::placeholders::_1)),
        _stage(types::stages::accepted), _bytes_read(0), _bytes_written(0),
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
            udho::net::context context(_io, *this);
            _processor(std::move(context));
            flush_headers();
        }

        void flush_headers(){
            _writer->start(
                _socket,
                std::bind(&self_type::on_flush_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
            );
        }

        void on_flush_header(boost::system::error_code ec, std::size_t bytes_transferred){
            if(ec){
                _stage = types::stages::error;
                return;
            }
            _stage = types::stages::headers_written;
            _bytes_written += bytes_transferred;
            if(_then){
                _then(ec, _bytes_written);
            }
        }

        // template <typename CharT>
        // void write_latter(const std::string_view& str){
        //     std::copy(str.begin(), str.end(), std::ostream_iterator<CharT>(_stream));
        // }

        void flush_body(){
            boost::asio::async_write(
                _socket, _streambuf,
                boost::asio::bind_executor(_strand, std::bind(&self_type::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2))
            );
        }

        template <typename CharT>
        void write(const std::string_view& str){
            write_latter<CharT>(str);
            flush_body();
        }

        void on_write(boost::system::error_code ec, std::size_t bytes_transferred){
            if(ec){
                _stage = types::stages::error;
                return;
            }
            _stage = types::stages::body_written;
            _bytes_written += bytes_transferred;
            if(_then){
                _then(ec, _bytes_written);
            }
        }

        void then(handler_type&& handler){
            _then = std::move(handler);
        }

        // --------

        // template <typename ValueT>
        // self_type& operator<<(const std::pair<boost::beast::http::field, ValueT>& header){
        //     _response[header.first] = header.second;
        //     return *this;
        // }
        // template <typename StrT>
        // self_type& operator<<(const StrT& str){
        //     write_latter(str);
        //     return *this;
        // }
        void flush(bool only_headers = false){
            if(_stage < types::stages::headers_written){
                if(!only_headers){
                    then(std::bind(&self_type::flush_body, shared_from_this()));
                }
                flush_headers();
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
        handler_type                        _then;
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
