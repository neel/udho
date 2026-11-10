#ifndef UDHO_NET_TRANSPORT_H
#define UDHO_NET_TRANSPORT_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/net/detail.h>

namespace udho{
namespace net{

/**
 * @brief Describes the accepted socket and application stream for a transport.
 * @tparam WireT Asio protocol used by the accepting listener.
 * @tparam StreamT Stream exposed to the application after channel opening.
 */
template <typename WireT, typename StreamT = typename detail::wire_types<WireT>::socket_type>
struct basic_transport{
    using wire_type     = WireT;
    using wire_types    = detail::wire_types<wire_type>;
    using endpoint_type = typename wire_types::endpoint_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using socket_type   = typename wire_types::socket_type;
    using stream_type   = StreamT;
};

/** @brief Non-listenable transport used by Beast stream-based tests. */
template <>
struct basic_transport<boost::beast::test::stream, boost::beast::test::stream>{
    using socket_type = boost::beast::test::stream;
    using stream_type = socket_type;
};

namespace transports{

/** @brief Plain HTTP transport over TCP. */
using http = basic_transport<boost::asio::ip::tcp>;

/** @brief HTTPS transport over TLS and TCP. */
using https = basic_transport<boost::asio::ip::tcp, boost::asio::ssl::stream<typename detail::wire_types<boost::asio::ip::tcp>::socket_type>>;

/** @brief Local stream protocol transport. */
using local = basic_transport<boost::asio::local::stream_protocol>;

/** @brief Non-listenable Beast test-stream transport. */
using testing = basic_transport<boost::beast::test::stream, boost::beast::test::stream>;

}
}
}

#endif // UDHO_NET_TRANSPORT_H
