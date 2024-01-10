#ifndef UDHO_NET_FWD_H
#define UDHO_NET_FWD_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>
#include <udho/net/context.h>
#include <chrono>

namespace udho{
namespace net{

template <typename ProtocolT>
struct connection;

class context;

template <typename RouterT>
struct handle;

}
}

#endif // UDHO_NET_FWD_H
