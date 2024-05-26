#ifndef UDHO_NET_ENVIRONMENT_H
#define UDHO_NET_ENVIRONMENT_H

#include <udho/url/fwd.h>
#include <udho/net/fwd.h>
#include <udho/net/stream.h>

namespace udho{
namespace net{

template <typename DerivedT, typename ListenerT>
struct enironment{
    using listener_type = ListenerT;
    using router_type   = decltype(DerivedT::routes());
    using server_type   = server<listener_type, router_type>;

    router_type router() const {
        return static_cast<const DerivedT&>(*this).routes();
    }


    private:
        boost::asio::io_service        _service;
        boost::asio::ip::tcp::endpoint _endpoint;
};

}
}

#endif // UDHO_NET_ENVIRONMENT_H
