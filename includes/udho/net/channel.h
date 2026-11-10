#ifndef UDHO_NET_CHANNEL_H
#define UDHO_NET_CHANNEL_H

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <cassert>
#include <cstdint>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/system/error_code.hpp>
#include <udho/net/transport.h>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>

namespace udho{
namespace net{

/**
 * @brief Owns connection-opening state for a transport.
 * @tparam TransportT Transport whose accepted sockets are prepared by the channel.
 */
template <typename TransportT>
struct channel;

/** @brief Plain HTTP channel over TCP. */
template <>
struct channel<transports::http>{
    using transport_type = transports::http;
    using endpoint_type  = typename transport_type::endpoint_type;
    using socket_type    = typename transport_type::socket_type;
    using stream_type    = typename transport_type::stream_type;

    explicit channel(endpoint_type&& endpoint): _endpoint(std::move(endpoint)) {}

    const endpoint_type& endpoint() const { return _endpoint; }

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        handler(boost::system::error_code{}, std::move(socket));
    }

    boost::system::error_code abort(stream_type& stream) {
        boost::system::error_code error;
        stream.close(error);
        return error;
    }

private:
    endpoint_type _endpoint;
};

/** @brief HTTPS channel over TCP. */
template <>
struct channel<transports::https>{
    using transport_type    = transports::https;
    using endpoint_type     = typename transport_type::endpoint_type;
    using socket_type       = typename transport_type::socket_type;
    using stream_type       = typename transport_type::stream_type;
    using ssl_context_type  = boost::asio::ssl::context;
    using stream_ptr_type   = std::unique_ptr<stream_type>;

    explicit channel(endpoint_type&& endpoint): _endpoint(std::move(endpoint)), _ssl(ssl_context_type::tls_server), _counter(0) {}

    const endpoint_type& endpoint() const { return _endpoint; }

    ssl_context_type& ssl() { return _ssl; }
    const ssl_context_type& ssl() const { return _ssl; }

    boost::system::error_code credentials(const std::string& certificate_chain_file, const std::string& private_key_file, ssl_context_type::file_format format = ssl_context_type::pem) {
        boost::system::error_code error;
        _ssl.use_certificate_chain_file(certificate_chain_file, error);
        if(error) return error;
        _ssl.use_private_key_file(private_key_file, format, error);
        return error;
    }

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        auto executor = boost::asio::get_associated_executor(handler, socket.get_executor());

        std::uint64_t id = _counter++;
        auto result = _staged.emplace(id, std::make_unique<stream_type>(std::move(socket), _ssl));
        assert(result.second);

        stream_ptr_type& stream_ptr = result.first->second;
        stream_ptr->async_handshake(boost::asio::ssl::stream_base::server,
            boost::asio::bind_executor(executor, [this, id, handler = std::forward<HandlerT>(handler)](boost::system::error_code error) mutable {
                auto it = _staged.find(id);
                assert(it != _staged.end());

                stream_ptr_type stream_ptr = std::move(it->second);
                _staged.erase(it);
                handler(error, std::move(*stream_ptr));
            })
        );
    }

    boost::system::error_code abort(stream_type& stream) {
        boost::system::error_code error;
        stream.next_layer().close(error);
        return error;
    }

private:
    endpoint_type    _endpoint;
    ssl_context_type _ssl;
    std::uint64_t    _counter;
    // TODO Add a strand-serialized periodic TLS-handshake sweep, a staging limit, and stop-time cancellation/draining of staged streams.
    std::map<std::uint64_t, stream_ptr_type> _staged;
};

/** @brief Plain channel over a local stream protocol socket. */
template <>
struct channel<transports::local>{
    using transport_type = transports::local;
    using endpoint_type  = typename transport_type::endpoint_type;
    using socket_type    = typename transport_type::socket_type;
    using stream_type    = typename transport_type::stream_type;

    explicit channel(endpoint_type&& endpoint): _endpoint(std::move(endpoint)) {}

    const endpoint_type& endpoint() const { return _endpoint; }

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        handler(boost::system::error_code{}, std::move(socket));
    }

    boost::system::error_code abort(stream_type& stream) {
        boost::system::error_code error;
        stream.close(error);
        return error;
    }

private:
    endpoint_type _endpoint;
};

/** @brief Pass-through channel for Beast test streams. */
template <>
struct channel<transports::testing>{
    using transport_type = transports::testing;
    using socket_type    = typename transport_type::socket_type;
    using stream_type    = typename transport_type::stream_type;

    channel() = default;

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        handler(boost::system::error_code{}, std::move(socket));
    }

    boost::system::error_code abort(stream_type& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
    }
};

}
}

#endif // UDHO_NET_CHANNEL_H
