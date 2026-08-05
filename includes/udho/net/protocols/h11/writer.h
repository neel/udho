#ifndef UDHO_NET_H11_WRITER_H
#define UDHO_NET_H11_WRITER_H

#include <memory>
#include <iostream>
#include <functional>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>


namespace udho{
namespace net{
namespace protocols{

namespace h11{

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Performs one asynchronous HTTP/1.1 header write.
 * @tparam Handler Completion handler type.
 * @tparam StreamT Asynchronous stream type.
 */
template <typename Handler, typename StreamT>
struct writer_internal: public std::enable_shared_from_this<writer_internal<Handler, StreamT>>{
    using self_type             = writer_internal<Handler, StreamT>;
    using handler_type          = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type = boost::beast::http::header<false, boost::beast::http::fields>;
    using response_type         = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type       = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type           = StreamT;
    using strand_type           = boost::asio::strand<typename stream_type::executor_type>;

    /**
     * @brief Construct an asynchronous header write operation.
     * @param io I/O context used to dispatch completion.
     * @param strand Strand on which completion is invoked.
     * @param headers Response headers to write.
     * @param stream Destination stream.
     * @param handler Completion handler.
     */
    explicit writer_internal(boost::asio::io_context& io, strand_type& strand, const response_headers_type& headers, stream_type& stream, Handler&& handler)
        : _io(io), _strand(strand), _headers(headers), _response(_headers), _serializer(_response), _stream(stream), _handler(std::move(handler)) {}
    /** @brief Copying an active write operation is disabled. */
    writer_internal(const writer_internal&) = delete;
    /** @brief Destroy the write operation. */
    ~writer_internal() { std::cout << "~http_writer_internal" << std::endl; }

    /** @brief Start writing the response headers. */
    void start() {
        boost::beast::http::async_write_header(
            _stream, _serializer,
            std::bind(&self_type::finished, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    /**
     * @brief Dispatch the completion handler.
     * @param ec Result of the write operation.
     * @param bytes_transferred Number of bytes written.
     */
    void finished(boost::system::error_code ec, std::size_t bytes_transferred){
        boost::asio::dispatch(_io,
                              boost::asio::bind_executor(
                                  _strand,
                                  std::bind(std::move(_handler), ec, bytes_transferred)
                                  )
                              );
    }
private:
    /** @brief Return shared ownership of this operation. */
    auto shared_from_this() {
        return std::enable_shared_from_this<self_type>::shared_from_this();
    }

    /** @brief Return a weak reference to this operation. */
    auto weak_from_this() {
        return std::enable_shared_from_this<self_type>::weak_from_this();
    }

private:
    boost::asio::io_context&        _io;
    strand_type&                    _strand;
    const response_headers_type&    _headers;
    response_type                   _response;
    serializer_type                 _serializer;
    stream_type&                    _stream;
    handler_type                    _handler;
};

/**
 * @brief Starts asynchronous HTTP/1.1 response header writes.
 * @tparam StreamT Asynchronous stream type.
 */
template <typename StreamT>
struct writer{
    using handler_type          = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type = boost::beast::http::header<false, boost::beast::http::fields>;
    using response_type         = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type       = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type           = StreamT;
    using strand_type           = boost::asio::strand<typename stream_type::executor_type>;

    /**
     * @brief Construct a writer for a response and stream.
     * @param headers Response whose headers will be written.
     * @param stream Destination stream.
     */
    explicit writer(const response_type& headers, stream_type& stream): _headers(headers), _stream(stream) {}
    /** @brief Copying a writer is disabled. */
    writer(const writer&) = delete;
    /** @brief Destroy the writer. */
    ~writer() { std::cout << "~h11:writer" << std::endl; }

    /**
     * @brief Schedule an asynchronous response header write.
     * @tparam Handler Completion handler type.
     * @param io I/O context used to dispatch the operation.
     * @param strand_write Strand on which the write starts.
     * @param strand_finished Strand on which completion is invoked.
     * @param handler Completion handler.
     */
    template <typename Handler>
    void operator()(boost::asio::io_context& io, strand_type& strand_write, strand_type& strand_finished, Handler&& handler){
        using internal_writer_type = h11::writer_internal<Handler, stream_type>;
        auto internal_writer = std::make_shared<internal_writer_type>(io, strand_finished, _headers, _stream, std::move(handler));
        boost::asio::dispatch(io,
                              boost::asio::bind_executor(
                                  strand_write,
                                  std::bind(&internal_writer_type::start, internal_writer)
                                  )
                              );
    }

private:
    const response_type& _headers;
    stream_type&         _stream;
};

/** @} */

}

}
}
}

#endif // UDHO_NET_H11_WRITER_H
