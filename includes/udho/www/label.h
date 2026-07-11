#ifndef UDHO_WWW_LABEL_H
#define UDHO_WWW_LABEL_H

#include <udho/net/detail.h>
#include <boost/beast/_experimental/test/stream.hpp>

namespace udho {
namespace www {

/**
 * @brief Type-level label describing a www runtime configuration.
 *
 * A www label binds a stream type, a tag that selects the runtime sketch, and
 * optional extra components. The label is consumed by manifold runtime,
 * transition, terminal, and sketch specializations.
 *
 * @tparam StreamT Stream type used by the runtime.
 * @tparam Tag Tag selecting the www sketch.
 * @tparam ExtraComponents Components appended to the default www composition.
 *
 * @ingroup www
 */
template <typename StreamT, typename Tag, typename... ExtraComponents>
struct basic_label{
    /**
     * @brief Stream type used by this label.
     */
    using stream_type = StreamT;

    /**
     * @brief Produces a new label with additional components appended.
     *
     * @tparam X Component types to append to the label.
     */
    template <typename... X>
    using append = basic_label<StreamT, Tag, ExtraComponents..., X...>;
};

/**
 * @brief Default TCP www label.
 *
 * Uses the default TCP socket type selected by `udho::net::detail::wire_types`.
 *
 * @tparam Tag Tag selecting the www sketch.
 * @tparam ExtraComponents Components appended to the default www composition.
 *
 * @ingroup www
 */
template <typename Tag, typename... ExtraComponents>
using label = basic_label<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type, Tag, ExtraComponents...>;

/**
 * @brief Test-stream www label.
 *
 * Uses `boost::beast::test::stream` as the stream type for tests.
 *
 * @tparam Tag Tag selecting the www sketch.
 * @tparam ExtraComponents Components appended to the default www composition.
 *
 * @ingroup www
 */
template <typename Tag, typename... ExtraComponents>
using test = basic_label<boost::beast::test::stream, Tag, ExtraComponents...>;

}
}

#endif // UDHO_WWW_LABEL_H
