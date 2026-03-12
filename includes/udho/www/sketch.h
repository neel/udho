#ifndef UDHO_WWW_SKETCH_H
#define UDHO_WWW_SKETCH_H

#include <udho/www/label.h>
#include <udho/www/tags.h>
#include <udho/manifold/order.h>
#include <udho/manifold/fwd.h>
#include <udho/www/components/handler.h>
#include <udho/www/components/pg.h>
#include <udho/www/components/protocol.h>
#include <udho/www/components/navigator.h>
#include <udho/www/components/cookies.h>
#include <udho/www/components/resources.h>
#include <udho/www/components/session.h>

namespace udho {
namespace manifold {

// { sketches

template <typename StreamT, typename... Bridges, typename... ExtraComponents>
struct sketch<www::basic_label<StreamT, www::tags::minimal<Bridges...>, ExtraComponents...>>{
    using www_type = www::basic_label<StreamT, www::tags::minimal<Bridges...>, ExtraComponents...>;

    using stream_type = StreamT;

    using composition_type = udho::manifold::composition<
        udho::www::components::basic_handler<StreamT>,
        udho::www::components::db::pg<>,
        udho::www::components::protocols::http<stream_type>,
        udho::www::components::navigators::pretty,
        udho::www::components::cookies,
        udho::www::components::resources<Bridges...>,
        ExtraComponents...
    >;

    using order_type = udho::manifold::order<
        udho::www::feature::header_reader,
        udho::www::feature::identifier,
        udho::www::feature::locator,
        udho::www::feature::cookie_load,
        udho::www::feature::session_load,
        udho::www::feature::body_reader
    >;
};

template <typename StreamT, typename SessionStorageT, udho::session::modes Mode, typename... Bridges, typename... ExtraComponents>
struct sketch<www::basic_label<StreamT, www::tags::statefulx<SessionStorageT, Mode, Bridges...>, ExtraComponents...>>{
    using www_type = www::basic_label<StreamT, www::tags::statefulx<SessionStorageT, Mode, Bridges...>, ExtraComponents...>;

    using stream_type = StreamT;

    using composition_type = udho::manifold::composition<
        udho::www::components::basic_handler<StreamT>,
        udho::www::components::db::pg<>,
        udho::www::components::protocols::http<stream_type>,
        udho::www::components::navigators::pretty,
        udho::www::components::cookies,
        udho::www::components::session<SessionStorageT, Mode>,
        udho::www::components::resources<Bridges...>,
        ExtraComponents...
    >;

    using order_type = udho::manifold::order<
        udho::www::feature::header_reader,
        udho::www::feature::identifier,
        udho::www::feature::locator,
        udho::www::feature::cookie_load,
        udho::www::feature::session_load,
        udho::www::feature::body_reader
    >;
};

// } sketches


}
}

#endif // UDHO_WWW_SKETCH_H
