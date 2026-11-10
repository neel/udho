#ifndef UDHO_NET_FWD_H
#define UDHO_NET_FWD_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>

namespace udho{
namespace net{

namespace detail{

template <typename DerivedT, typename RuntimeT, typename SocketT, typename StreamT = SocketT>
struct basic_listener;

}

/** @addtogroup DoxyG_net
 *  @{
 */

// template <typename ProtocolT>
// struct connection;

// template <typename ListenerT>
// struct server;

// class stream;

// template <typename RouterT>
// struct handle;

// template <typename ResourceSubsetT>
// struct basic_context;

template <typename WireT, typename StreamT>
struct basic_transport;

template <typename TransportT>
struct channel;

template <typename TransportT, typename RuntimeT>
struct basic_listener;

template <typename RuntimeT, typename SocketT = typename RuntimeT::stream_type>
struct basic_plain_listener;

/** @} */

}
}

#endif // UDHO_NET_FWD_H
