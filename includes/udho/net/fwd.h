#ifndef UDHO_NET_FWD_H
#define UDHO_NET_FWD_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>
#include <udho/net/stream.h>
#include <chrono>

namespace udho{
namespace net{

template <typename ProtocolT>
struct connection;

template <typename ListenerT>
struct server;

class stream;

// template <typename RouterT>
// struct handle;

template <typename ResourceStoreT>
struct basic_context;

}
}

#endif // UDHO_NET_FWD_H
