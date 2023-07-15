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

enum class stages{
    accepted,
    headers_read,
    body_read,
    body_skipped,
    headers_written,
    body_written,
    closed,
    rejected
};

namespace headers{
    using request  = boost::beast::http::header<true,  boost::beast::http::fields>;
    using response = boost::beast::http::header<false, boost::beast::http::fields>;
}



}
}
}

#endif // UDHO_NET_COMMON_H
