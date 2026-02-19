#ifndef UDHO_NET_PROTOCOL_PROTOCOLS_H
#define UDHO_NET_PROTOCOL_PROTOCOLS_H

#include <udho/net/protocols/http.h>
#include <udho/net/protocols/scgi.h>

namespace udho{
namespace net{
namespace protocols{

template <typename StreamT>
struct http{
    using reader = h11_reader<StreamT>;
    using writer = h11_writer<StreamT>;
};

template <typename StreamT>
struct scgi{
    using reader = scgi_reader<StreamT>;
    using writer = h11_writer<StreamT>;
};

template <typename StreamT>
struct scgi2{
    using reader = scgi_reader2<StreamT>;
    using writer = h11_writer<StreamT>;
};


}
}
}

#endif // UDHO_NET_PROTOCOL_PROTOCOLS_H
