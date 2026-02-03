#ifndef UDHO_NET_FWD_H
#define UDHO_NET_FWD_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>

namespace udho{
namespace net{

// template <typename ProtocolT>
// struct connection;

// template <typename ListenerT>
// struct server;

// class stream;

// template <typename RouterT>
// struct handle;

// template <typename ResourceSubsetT>
// struct basic_context;

template <typename WireT, typename RuntimeT>
struct basic_listener;

}
}

#endif // UDHO_NET_FWD_H
