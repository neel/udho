#ifndef UDHO_NET_COMMON_H
#define UDHO_NET_COMMON_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>

namespace udho{
namespace net{
namespace types{

#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket;
#endif
using strand    = boost::asio::strand<boost::asio::io_context::executor_type>;

enum class stages{
    accepted,
    headers_read,
    body_read,
    body_skipped,
    headers_written,
    body_written,
    body_done,
    closed,
    rejected,
    error
};

namespace headers{
    using request  = boost::beast::http::header<true,  boost::beast::http::fields>;
    using response = boost::beast::http::header<false, boost::beast::http::fields>;
}

}

enum class buffering_options{
    first_write_flushes_headers,        // response headers get written to the socket before writting any content to the socket
    last_write_flushes_headers          // neither contents nor headers are sent over the socket untill all contents have been written
};

}
}

#endif // UDHO_NET_COMMON_H
