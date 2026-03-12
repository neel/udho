#ifndef UDHO_WWW_LABEL_H
#define UDHO_WWW_LABEL_H

#include <udho/net/detail.h>
#include <boost/beast/_experimental/test/stream.hpp>

namespace udho {
namespace www {

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct basic_label{
    using stream_type = StreamT;

    template <typename... X>
    using append = basic_label<StreamT, Tag, ExtraComponents..., X...>;
};

template <typename Tag, typename... ExtraComponents>
using label = basic_label<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type, Tag, ExtraComponents...>;

template <typename Tag, typename... ExtraComponents>
using test = basic_label<boost::beast::test::stream, Tag, ExtraComponents...>;

}
}

#endif // UDHO_WWW_LABEL_H
