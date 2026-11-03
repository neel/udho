#ifndef UDHO_NET_PROFILE_H
#define UDHO_NET_PROFILE_H

#include <string>
#include <utility>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/system/error_code.hpp>
#include <udho/net/detail.h>

namespace udho{
namespace net{
namespace profiles{

/** @brief Selects an unencrypted HTTP channel over TCP. */
struct http{
    using wire_type     = boost::asio::ip::tcp;
    using wire_types    = detail::wire_types<wire_type>;
    using endpoint_type = typename wire_types::endpoint_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using socket_type   = typename wire_types::socket_type;
    using stream_type   = socket_type;

    explicit http(endpoint_type&& endpoint): _endpoint(std::move(endpoint)) {}

    const endpoint_type& endpoint() const { return _endpoint; }

private:
    endpoint_type _endpoint;
};

/** @brief Selects an HTTPS channel over TCP. */
struct https{
    using wire_type        = boost::asio::ip::tcp;
    using wire_types       = detail::wire_types<wire_type>;
    using endpoint_type    = typename wire_types::endpoint_type;
    using acceptor_type    = typename wire_types::acceptor_type;
    using socket_type      = typename wire_types::socket_type;
    using ssl_context_type = boost::asio::ssl::context;
    using stream_type      = boost::asio::ssl::stream<socket_type>;

    explicit https(endpoint_type&& endpoint): _endpoint(std::move(endpoint)), _ssl(ssl_context_type::tls_server) {}

    const endpoint_type& endpoint() const { return _endpoint; }

    ssl_context_type& ssl() { return _ssl; }
    const ssl_context_type& ssl() const { return _ssl; }

    boost::system::error_code credentials(const std::string& certificate_chain_file, const std::string& private_key_file, ssl_context_type::file_format format = ssl_context_type::pem) {
        boost::system::error_code error;
        _ssl.use_certificate_chain_file(certificate_chain_file, error);
        if (error) return error;
        _ssl.use_private_key_file(private_key_file, format, error);
        return error;
    }

private:
    endpoint_type    _endpoint;
    ssl_context_type _ssl;
};

/** @brief Selects a channel over a local stream protocol socket. */
struct local{
    using wire_type     = boost::asio::local::stream_protocol;
    using wire_types    = detail::wire_types<wire_type>;
    using endpoint_type = typename wire_types::endpoint_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using socket_type   = typename wire_types::socket_type;
    using stream_type   = socket_type;

    explicit local(endpoint_type&& endpoint): _endpoint(std::move(endpoint)) {}

    const endpoint_type& endpoint() const { return _endpoint; }

private:
    endpoint_type _endpoint;
};

/** @brief Selects a Beast test-stream channel. */
struct test{
    using socket_type = boost::beast::test::stream;
    using stream_type = socket_type;
};

}
}
}

#endif // UDHO_NET_PROFILE_H
