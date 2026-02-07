#ifndef UDHO_NET_DETAIL_H
#define UDHO_NET_DETAIL_H

#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>

namespace udho{
namespace net{
namespace detail{

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


template <typename WireT>
struct basic_wire_traits{
    using protocol_type = WireT;
    using wire_types    = detail::wire_types<protocol_type>;
    using socket_type   = typename wire_types::socket_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using endpoint_type = typename wire_types::endpoint_type;

protected:
    boost::system::error_code prepare(acceptor_type&, endpoint_type&) {
        return boost::system::error_code{};
    }
    boost::system::error_code cancel(acceptor_type& acceptor) {
        boost::system::error_code error;
        acceptor.cancel(error);
        acceptor.close(error);
        return error;
    }

    boost::system::error_code cancel(socket_type& socket) {
        socket.cancel();
        return boost::system::error_code{};
    }
};

template <typename WireT>
struct wire_traits: public basic_wire_traits<WireT>{
    using basic_traits_type = basic_wire_traits<WireT>;

    using basic_traits_type::prepare;
    using basic_traits_type::cancel;
};

template <>
struct wire_traits<boost::asio::ip::tcp>: public basic_wire_traits<boost::asio::ip::tcp>{
    using basic_traits_type = basic_wire_traits<boost::asio::ip::tcp>;

    boost::system::error_code prepare(acceptor_type& acceptor, endpoint_type& endpoint) {
        boost::system::error_code error;
        acceptor.set_option(boost::asio::socket_base::reuse_address(true), error);
        return error;
    }
    using basic_traits_type::cancel;
};

template <>
struct wire_traits<boost::asio::local::stream_protocol>: public basic_wire_traits<boost::asio::local::stream_protocol>{
    using basic_traits_type = basic_wire_traits<boost::asio::local::stream_protocol>;

    boost::system::error_code prepare(acceptor_type&, endpoint_type& endpoint) {
        errno = 0;
        if (::unlink(endpoint.path().c_str()) != 0) {
            if (errno == ENOENT) return {}; // no prior socket, OK
            return boost::system::error_code(errno, boost::system::system_category());
        }
        return {};
    }
    using basic_traits_type::cancel;
};

}
}
}

#endif // UDHO_NET_DETAIL_H
