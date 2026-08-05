#ifndef UDHO_NET_DETAIL_H
#define UDHO_NET_DETAIL_H

#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>

namespace udho{
namespace net{
namespace detail{

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Associates an Asio transport protocol with its socket, acceptor, and endpoint types.
 * @tparam WireT Asio stream-oriented transport protocol.
 */
template <typename WireT>
struct wire_types{
    using protocol_type = WireT;
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    using socket_type   = boost::asio::basic_stream_socket<protocol_type, boost::asio::io_context::executor_type>;
    using acceptor_type = boost::asio::basic_socket_acceptor<protocol_type, boost::asio::io_context::executor_type>;
#else
    using socket_type   = boost::asio::basic_stream_socket<protocol_type>;
    using acceptor_type = boost::asio::basic_socket_acceptor<protocol_type>;
#endif
    using endpoint_type = typename protocol_type::endpoint;
};


/**
 * @brief Common listener operations shared by all transport protocols.
 * @tparam WireT Asio stream-oriented transport protocol.
 *
 * The listener uses these traits immediately before binding an acceptor and
 * when stopping an acceptor or socket. Protocol specializations may replace
 * `prepare()` while reusing the cancellation behavior.
 */
template <typename WireT>
struct basic_wire_traits{
    using protocol_type = WireT;
    using wire_types    = detail::wire_types<protocol_type>;
    using socket_type   = typename wire_types::socket_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using endpoint_type = typename wire_types::endpoint_type;

protected:
    /**
     * @brief Perform protocol-specific preparation before binding.
     * @param acceptor Acceptor that will be bound.
     * @param endpoint Endpoint to which it will be bound.
     * @return Success for the generic protocol implementation.
     */
    boost::system::error_code prepare(acceptor_type& acceptor, endpoint_type& endpoint) {
        (void)acceptor;
        (void)endpoint;
        return boost::system::error_code{};
    }
    /**
     * @brief Cancel pending accepts and close an acceptor.
     * @param acceptor Acceptor to stop.
     * @return Error reported while closing the acceptor, if any.
     */
    boost::system::error_code cancel(acceptor_type& acceptor) {
        boost::system::error_code error;
        acceptor.cancel(error);
        acceptor.close(error);
        return error;
    }

    /**
     * @brief Cancel pending operations on a socket.
     * @param socket Socket whose operations are cancelled.
     * @return Success if cancellation does not throw.
     */
    boost::system::error_code cancel(socket_type& socket) {
        socket.cancel();
        return boost::system::error_code{};
    }
};

/**
 * @brief Default wire traits for protocols requiring no bind preparation.
 * @tparam WireT Asio stream-oriented transport protocol.
 *
 * This primary template exposes the generic no-op `prepare()` and common
 * `cancel()` overloads from @ref basic_wire_traits.
 */
template <typename WireT>
struct wire_traits: public basic_wire_traits<WireT>{
    using basic_traits_type = basic_wire_traits<WireT>;

    using basic_traits_type::prepare;
    using basic_traits_type::cancel;
};

/**
 * @brief TCP wire traits that enable address reuse before binding.
 *
 * `basic_listener` invokes `prepare()` after opening the acceptor and before
 * binding it, allowing a recently used TCP endpoint to be rebound.
 */
template <>
struct wire_traits<boost::asio::ip::tcp>: public basic_wire_traits<boost::asio::ip::tcp>{
    using basic_traits_type = basic_wire_traits<boost::asio::ip::tcp>;

    /**
     * @brief Enable `reuse_address` on a TCP acceptor.
     * @param acceptor Open acceptor that will be configured.
     * @param endpoint Endpoint that will subsequently be bound.
     * @return Error reported by `set_option`, if any.
     */
    boost::system::error_code prepare(acceptor_type& acceptor, endpoint_type& endpoint) {
        boost::system::error_code error;
        acceptor.set_option(boost::asio::socket_base::reuse_address(true), error);
        return error;
    }
    using basic_traits_type::cancel;
};

/**
 * @brief Local-stream wire traits that remove a stale socket path before binding.
 *
 * Unix-domain socket paths remain in the filesystem after abnormal shutdown.
 * Preparation unlinks the configured endpoint path; a missing path is treated
 * as success.
 */
template <>
struct wire_traits<boost::asio::local::stream_protocol>: public basic_wire_traits<boost::asio::local::stream_protocol>{
    using basic_traits_type = basic_wire_traits<boost::asio::local::stream_protocol>;

    /**
     * @brief Remove the endpoint's existing filesystem entry.
     * @param acceptor Open acceptor that will later be bound.
     * @param endpoint Local-stream endpoint whose path is removed.
     * @return Success, or the system error reported by `unlink`.
     */
    boost::system::error_code prepare(acceptor_type& acceptor, endpoint_type& endpoint) {
        (void)acceptor;
        errno = 0;
        if (::unlink(endpoint.path().c_str()) != 0) {
            if (errno == ENOENT) return {}; // no prior socket, OK
            return boost::system::error_code(errno, boost::system::system_category());
        }
        return {};
    }
    using basic_traits_type::cancel;
};

/** @} */

}
}
}

#endif // UDHO_NET_DETAIL_H
