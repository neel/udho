#ifndef UDHO_NET_CONNECTION_H
#define UDHO_NET_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>
#include <udho/net/stream.h>
#include <udho/net/bridge.h>
#include <udho/net/fwd.h>
#include <chrono>
#include <udho/url/summary.h>

namespace udho{
namespace net{

using buffer_type = std::basic_string<std::uint8_t>;

namespace detail{
    struct connection_stat{
        udho::net::types::stages   _stage;
        std::size_t                _bytes_read, _bytes_written;

        connection_stat(): _stage(types::stages::accepted), _bytes_read(0), _bytes_written(0) {}
    };

    struct on_flush_header{
        detail::connection_stat& _stat;

        on_flush_header(detail::connection_stat& stat): _stat(stat) {}
        void operator()(boost::system::error_code ec, std::size_t bytes_transferred){
            // std::cout << "on_flush_header::operator()" << std::endl;
            if(ec){
                _stat._stage = types::stages::error;
                return;
            }
            _stat._stage = types::stages::headers_written;
            _stat._bytes_written += bytes_transferred;
        }
    };

    template <typename Handler>
    struct on_flush_body{
        boost::asio::io_service& _io;
        detail::connection_stat& _stat;
        Handler                  _handler;

        on_flush_body(boost::asio::io_service& io, connection_stat& stat, Handler&& handler)
            : _io(io), _stat(stat), _handler(std::move(handler))
        {}
        void operator()(boost::system::error_code ec, std::size_t bytes_transferred){
            // std::cout << "on_flush_body::operator()" << std::endl;
            if(ec){
                _stat._stage = types::stages::error;
                std::cout << "error: " << ec << std::endl;
                return;
            }
            _stat._stage = types::stages::body_written;
            _stat._bytes_written += bytes_transferred;

            _io.post(std::bind(std::move(_handler), ec, bytes_transferred));
        }
    };
    /**
     * Flushes the body. Assumes that the headers have already been flushed.
     */
    template <typename Handler>
    struct flush_body{
        using buffer_type  = std::basic_string<std::uint8_t>;
        using buffers_type = std::vector<boost::asio::const_buffer>;

        boost::asio::io_service&     _io;     // io service
        udho::net::types::strand&    _strand; // write strand
        udho::net::types::socket&    _socket; // socket
        buffers_type&                _buffers;
        types::transfer::encoding    _encoding;
        detail::connection_stat&     _stat;
        Handler                      _handler;

        flush_body(boost::asio::io_service& io, udho::net::types::strand& strand, udho::net::types::socket& socket, buffers_type& buffers, types::transfer::encoding encoding, connection_stat& stat, Handler&& handler)
            :_io(io), _strand(strand), _socket(socket), _buffers(buffers), _encoding(encoding), _stat(stat), _handler(std::move(handler))
        {}

        /**
         * Starts flushing
         * @param ec error code is set to boost::system::error_code() if called directly.
         * @param bytes_transferred is set to 0 when called directly.
         *
         * Calls the handler (through the strand) in case of error (depending on ec).
         * Otherwise prepares the buffer to be sent and the starts the async write operation while passing the handler as callback.
         */
        void operator()(boost::system::error_code ec = boost::system::error_code(), std::size_t bytes_transferred = 0){
            // std::cout << "flush_body::operator()" << std::endl;
            _stat._bytes_written += bytes_transferred;
            if(ec){
                _io.post(std::bind(std::move(_handler), ec, bytes_transferred));
            }else{
                write();
            }

        }
        void write(){
            boost::asio::async_write(
                _socket, _buffers,
                boost::asio::bind_executor(_strand, on_flush_body<Handler>(_io, _stat, std::move(_handler)))
            );
        }
    };
    template <typename Handler>
    struct preprocess{
        using buffer_type  = std::basic_string<std::uint8_t>;
        using buffers_type = std::vector<boost::asio::const_buffer>;

        boost::asio::io_service&        _io;     // io service
        udho::net::types::strand&       _strand; // write strand
        buffer_type                     _buffer; // uncompressed data
        buffer_type&                    _compressed; // compressed data
        const types::transfer_encoding& _transfer_encoding;
        buffers_type&                   _buffers;
        Handler                         _handler;

        preprocess(boost::asio::io_service& io, udho::net::types::strand& strand, buffer_type&& buffer, buffer_type& compressed, const types::transfer_encoding& transfer_encoding, buffers_type& buffers, Handler&& handler):
            _io(io), _strand(strand), _buffer(std::move(buffer)), _compressed(compressed), _transfer_encoding(transfer_encoding), _buffers(buffers), _handler(std::move(handler))
        {}
        void operator()(){
            // std::cout << "preprocess::operator()" << std::endl;
            compress();
        }
        void compress(){
            if(_transfer_encoding.compression() == types::transfer::compression::none){
                _compressed = _buffer;
            }else{
                // do compress _buffer
            }
            encode();
            finish();
        }
        void encode(){
            static std::array<char, 2> crlf = {'\r', '\n'};
            if(_transfer_encoding.encoding() == types::transfer::encoding::chunked){
                std::string chunk_header = (boost::format("%x") % _compressed.size()).str() + "\r\n";
                buffer_type chunk_buffer;
                chunk_buffer.resize(chunk_header.size());
                std::copy(chunk_header.begin(), chunk_header.end(), chunk_buffer.begin());
                _compressed.insert(0, chunk_buffer);
                _buffers = std::vector<boost::asio::const_buffer>{
                    boost::asio::buffer(_compressed),
                    boost::asio::buffer(crlf)
                };
            }else{
                _buffers = std::vector<boost::asio::const_buffer>{
                    boost::asio::buffer(_compressed)
                };
            }
        }
        void finish(){
            std::string compressed_print;
            std::copy(_compressed.begin(), _compressed.end(), std::back_inserter(compressed_print));

            // std::cout << "preprocess::finish() " << compressed_print << std::endl;
            _io.post(boost::asio::bind_executor(_strand, std::bind(std::move(_handler))));
        }
    };
}


/**
 * \brief A connection object wraps a socket.
 * Follows a protocol (e.g. HTTP, FastCGI, SCGI, wscgi etc..) to parse the headers.
 * Uses ProtocolT to follow the protocol and prepare a request object.
 * Once the request object is created it passes that to an asynchronous resolver to resolve a slot that will process that request.
 */
template <typename ProtocolT>
struct connection: public std::enable_shared_from_this<connection<ProtocolT>>, private types::transfer_encoding, private detail::connection_stat{
    using socket_type     = udho::net::types::socket;
    using protocol_type   = ProtocolT;
    using reader_type     = typename protocol_type::reader;
    using writer_type     = typename protocol_type::writer;
    using clock_type      = std::chrono::time_point<std::chrono::system_clock>;
    using self_type       = connection<ProtocolT>;
    using connection_type = connection<ProtocolT>;
    using processer_type  = std::function<void (udho::net::stream&&)>;
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using bridge_ptr      = std::shared_ptr<udho::net::bridge>;
    using buffer_type     = std::basic_string<std::uint8_t>;
    using buffers_type    = std::vector<boost::asio::const_buffer>;

    using types::transfer_encoding::encoding;
    using types::transfer_encoding::compression;

    friend struct detail::on_flush_header;
    template <typename Handler>
    friend struct detail::flush_body;
    template <typename Handler>
    friend struct detail::on_flush_body;
    template <typename Handler>
    friend struct detail::preprocess;

    using on_flush_header = detail::on_flush_header;
    template <typename Handler>
    using    flush_body   = detail::flush_body<Handler>;
    template <typename Handler>
    using on_flush_body   = detail::on_flush_body<Handler>;
    template <typename Handler>
    using preprocess      = detail::preprocess<Handler>;

    connection(boost::asio::io_service& service, udho::net::types::socket socket)
      : _io(service), _socket(std::move(socket)), _compression_strand(_socket.get_executor()), _write_strand(_socket.get_executor()), _stat_strand(_socket.get_executor()),
        _reader(std::make_shared<reader_type>(_request, _socket)), _writer(std::make_shared<writer_type>(_response, _socket)),
        _stream(&_streambuf)
    {}
    void start(processer_type&& processor){
        _processor = std::move(processor);
        _start = std::chrono::system_clock::now();
        _reader->start(
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
            _bridge_ptr = std::make_shared<udho::net::bridge>(
                _request, _response, _stream, static_cast<types::transfer_encoding&>(*this),
                std::bind(&self_type::flush<handler_type>, shared_from_this(), std::placeholders::_1, std::placeholders::_2),
                std::bind(&self_type::finish, shared_from_this())
            );
            udho::net::stream context(_io, *_bridge_ptr);
            _processor(std::move(context));
        }

        void flush_header() {
            _writer->start(_io, _write_strand, _stat_strand, on_flush_header(*this));
        }

        std::size_t copy(buffer_type& buffer){
            auto size = _streambuf.size();
            if(size > 0){
                std::copy_n(boost::asio::buffer_cast<const std::uint8_t*>(_streambuf.data()), size, std::back_inserter(buffer));
                _streambuf.consume(size);
            }
            return size;
        }

        bool chunked() const {
            return encoding() == types::transfer::encoding::chunked;
        }
        void prepare_headers(){
            types::transfer_encoding::prepare(_response);
            if(!chunked()){
                _response.set(boost::beast::http::field::content_length, std::to_string(_streambuf.size()));
            }
        }
        template <typename Handler>
        void flush_body_(buffer_type&& buffer, Handler&& handler){
            flush_body<Handler> body_flusher(_io, _stat_strand, _socket, _buffers, encoding(), stat(), std::move(handler));
            _io.dispatch(
                boost::asio::bind_executor(
                    _compression_strand,
                    preprocess<flush_body<Handler>>(_io, _write_strand, std::move(buffer), _compressed, *this, _buffers, std::move(body_flusher))
                )
            );
        }
        /**
         * Flush the content in the buffer
         * @note Expect that the write buffer has non-zero amount of data.
         * @param handler Expects a callback function of signature void (boost::system::error_code, std::size_t)
         * @param only_headers flush headers only
         *
         * 0. (If header is not yet flushed) First a sync copy is performed on the streambuf to consume the contents already written.
         * 1. The contents that are already in the write buffer will NOT be written immediately.
         * 2. But It is garunteed that the contents written after calling flush() from the same thread will NOT be written.
         * 3. However calling flush() on the same connection simoultanously from multiple threads may lead to undefined behaviour.
         * 4. For the chunked encoding flush first flushes the headers and the flush() can be called multiple times to flush contents in the buffer.
         * 5. For plain encoding flush function cannot be called. Only finish can be called.
         */
        template <typename Handler>
        void flush(Handler&& handler, bool only_headers = false){
            buffer_type buffer;
            std::size_t payload_size = 0;
            if(!only_headers){
                payload_size = copy(buffer);
            }
            if(_stage < types::stages::headers_written){
                types::transfer_encoding::prepare(_response);
                if(!chunked()){
                    _response.set(boost::beast::http::field::content_length, std::to_string(payload_size));
                }
                flush_header();
            }
            if(!only_headers && payload_size > 0){
                flush_body_(std::move(buffer), std::move(handler));
            }
        }

        /**
         * Uses io.post to call the on_finish handler which shutdowns the socket.
         * In case of the chunked encoding send an empty chunk before shutting down.
         * @note Expects that the write buffer is empty.
         */
        void finish(){
            if(_streambuf.size() != 0){
                // TODO need to flush if there is something pending in the buffer
                flush(std::bind(&self_type::finish, shared_from_this()));
                return;
            }
            if(chunked()){
                auto handler = std::bind(&self_type::on_finish, shared_from_this(), std::placeholders::_1, std::placeholders::_2);
                flush_body<decltype(handler)> body_flusher(_io, _stat_strand, _socket, _buffers, encoding(), stat(), std::move(handler));
                _io.dispatch(
                    boost::asio::bind_executor(
                        _compression_strand,
                        preprocess<flush_body<decltype(handler)>>(_io, _compression_strand, {}, _compressed, *this, _buffers, std::move(body_flusher))
                    )
                );
            }else{
                // prepare_headers();
                _io.post(
                    boost::asio::bind_executor(
                        _write_strand,              // why ?
                        std::bind(&self_type::on_finish, shared_from_this(), boost::system::error_code(), 0)
                    )
                );
            }
        }
        void on_finish(boost::system::error_code ec, std::size_t bytes_transferred){
            boost::ignore_unused(bytes_transferred);
            // std::cout << "socket close" << std::endl;
            boost::system::error_code error;
            _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, error);
        }
        detail::connection_stat& stat(){
            return *this;
        }
    private:
        clock_type                          _start, _end;
        boost::asio::io_service&            _io;
        processer_type                      _processor;
        udho::net::types::socket            _socket;
        udho::net::types::strand            _compression_strand, _write_strand, _stat_strand;
        udho::net::types::headers::request  _request;
        udho::net::types::headers::response _response;
        std::shared_ptr<reader_type>        _reader;
        std::shared_ptr<writer_type>        _writer;
        boost::asio::streambuf              _streambuf;
        std::ostream                        _stream;
        bridge_ptr                          _bridge_ptr;
        buffer_type                         _compressed;
        buffers_type                        _buffers;
};



}
}

#endif // UDHO_NET_CONNECTION_H
