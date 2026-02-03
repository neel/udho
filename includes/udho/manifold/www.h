#ifndef UDHO_MANIFOLD_WWW_H
#define UDHO_MANIFOLD_WWW_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/order.h>
#include <udho/manifold/features.h>
#include <udho/manifold/components/handler.h>
#include <udho/manifold/components/pg.h>
#include <udho/manifold/components/routing.h>
#include <udho/manifold/components/protocol.h>
#include <udho/manifold/components/navigator.h>
#include <udho/manifold/components/cookies.h>
#include <udho/manifold/components/session.h>
#include <udho/net/listener.h>
#include <udho/manifold/transition.h>
#include <udho/manifold/runtime.h>
#include <udho/manifold/flow.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/redis.h>
#include <udho/session/storage/fs_mem.h>
#include <udho/manifold/composition_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/manifold/configs_view.h>
#include <udho/manifold/terminal.h>

namespace udho {
namespace manifold {

namespace www {

namespace tags{

    struct minimal{};

    template <typename SessionStorageT, udho::session::modes Mode, typename... Bridges>
    struct statefulx {
        using sesssion_storage_type = SessionStorageT;
        static constexpr const udho::session::modes session_storage_mode = Mode;
        using resources_component_type = udho::manifold::components::resources<Bridges...>;
    };

    template <typename SessionStorageT = udho::session::storage::fs, udho::session::modes Mode = udho::session::modes::lazy>
    using stateful = statefulx<SessionStorageT, Mode>;

}

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct basic_label{
    using stream_type = StreamT;

    template <typename... X>
    using append = basic_label<StreamT, Tag, ExtraComponents..., X...>;
};

template <typename StreamT, typename... Components>
using basic_context = udho::manifold::basic_context<StreamT, udho::manifold::components::basic_handler<StreamT>, Components...>;

template <typename Tag, typename... ExtraComponents>
using label = basic_label<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type, Tag, ExtraComponents...>;

template <typename Tag, typename... ExtraComponents>
using test = basic_label<boost::beast::test::stream, Tag, ExtraComponents...>;

template <typename... Components>
using context = basic_context<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type, Components...>;

// { convenience labels

using stateless             = www::label<www::tags::minimal>;
namespace stateful{
    using lazy_fs           = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
    using optimistic_fs     = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
    using lazy_memfs        = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
    using optimistic_memfs  = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
    using lazy_redis        = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::lazy>>;
    using optimistic_redis  = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::optimistic>>;
    using immediate_redis   = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::immediate>>;

    namespace lua {
        using lazy_fs           = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
        using optimistic_fs     = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
        using lazy_memfs        = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
        using optimistic_memfs  = www::label<www::tags::statefulx<udho::session::storage::fs, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
        using lazy_redis        = www::label<www::tags::statefulx<udho::session::storage::redis, udho::session::modes::lazy, udho::view::data::bridges::lua>>;
        using optimistic_redis  = www::label<www::tags::statefulx<udho::session::storage::redis, udho::session::modes::optimistic, udho::view::data::bridges::lua>>;
        using immediate_redis   = www::label<www::tags::statefulx<udho::session::storage::redis, udho::session::modes::immediate, udho::view::data::bridges::lua>>;
    }
}

// }

}

// { sketches

template <typename StreamT, typename... ExtraComponents>
struct sketch<www::basic_label<StreamT, www::tags::minimal, ExtraComponents...>>{
    using www_type = www::basic_label<StreamT, www::tags::minimal, ExtraComponents...>;

    using stream_type = StreamT;

    using composition_type = udho::manifold::composition<
        udho::manifold::components::basic_handler<StreamT>,
        udho::manifold::components::db::pg<>,
        udho::manifold::components::protocols::http2<stream_type>,
        udho::manifold::components::navigators::pretty,
        udho::manifold::components::cookies,
        udho::manifold::components::resources<>,
        ExtraComponents...
    >;

    using order_type = udho::manifold::order<
        udho::manifold::feature::header_reader,
        udho::manifold::feature::identifier,
        udho::manifold::feature::locator,
        udho::manifold::feature::cookie_load,
        udho::manifold::feature::session_load,
        udho::manifold::feature::body_reader
    >;
};

template <typename StreamT, typename SessionStorageT, udho::session::modes Mode, typename... Bridges, typename... ExtraComponents>
struct sketch<www::basic_label<StreamT, www::tags::statefulx<SessionStorageT, Mode, Bridges...>, ExtraComponents...>>{
    using www_type = www::basic_label<StreamT, www::tags::statefulx<SessionStorageT, Mode, Bridges...>, ExtraComponents...>;

    using stream_type = StreamT;

    using composition_type = udho::manifold::composition<
        udho::manifold::components::basic_handler<StreamT>,
        udho::manifold::components::db::pg<>,
        udho::manifold::components::protocols::http2<stream_type>,
        udho::manifold::components::navigators::pretty,
        udho::manifold::components::cookies,
        udho::manifold::components::session<SessionStorageT, Mode>,
        typename www::tags::statefulx<SessionStorageT, Mode, Bridges...>::resources_component_type,
        ExtraComponents...
    >;

    using order_type = udho::manifold::order<
        udho::manifold::feature::header_reader,
        udho::manifold::feature::identifier,
        udho::manifold::feature::locator,
        udho::manifold::feature::cookie_load,
        udho::manifold::feature::session_load,
        udho::manifold::feature::body_reader
    >;
};

// } sketches

// { transitions

static constexpr const std::size_t route_locator_stage = udho::manifold::feature::locator::stage;

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct default_transition<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, route_locator_stage>{
    using label_type             = www::basic_label<StreamT, Tag, ExtraComponents...>;
    using stream_type            = StreamT;
    using sketch_type            = sketch<label_type>;
    using runtime_type           = basic_runtime<label_type, stream_type>;
    using flow_type              = typename runtime_type::flow_type;
    using composition_type       = typename runtime_type::composition_type;
    using journal_type           = typename flow_type::journal_type;
    using pipeline_type          = typename runtime_type::template pipeline_at<route_locator_stage>;
    using configs_type           = typename runtime_type::configs_type;
    using portal_type            = typename udho::manifold::detail::get_portal_type<composition_type>::type;
    using start_pipeline_type    = typename runtime_type::start_pipeline_type;
    // using routing_component_type = typename label_type::routing_component_type;
    // using routing_table_type     = typename routing_component_type::routing_table_type;

    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& config, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::manifold::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        assert(route_index.valid());
        const auto& routing_component = composition.template at<udho::manifold::feature::locator, 0>().component();
        const auto& routing_table     = routing_component.table();
        routing_table.reconfigure_for(route_index, configs);
        // }
        p.next(flow, std::forward<Args>(args)...);
    }
};

static constexpr const std::size_t action_transition_stage = 2;
template <typename StreamT, typename Tag, typename... ExtraComponents>
struct default_transition<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, action_transition_stage>{
    using label_type             = www::basic_label<StreamT, Tag, ExtraComponents...>;
    using stream_type            = StreamT;
    using sketch_type            = sketch<label_type>;
    using runtime_type           = basic_runtime<label_type, stream_type>;
    using flow_type              = typename runtime_type::flow_type;
    using composition_type       = typename runtime_type::composition_type;
    using journal_type           = typename flow_type::journal_type;
    using pipeline_type          = typename runtime_type::template pipeline_at<action_transition_stage>;
    using configs_type           = typename runtime_type::configs_type;
    using portal_type            = typename udho::manifold::detail::get_portal_type<composition_type>::type;
    using context_type           = typename udho::manifold::detail::get_context_for_portal<StreamT, portal_type>::type;
    using start_pipeline_type    = typename runtime_type::start_pipeline_type;
    // using routing_component_type = typename label_type::routing_component_type;
    // using routing_table_type     = typename routing_component_type::routing_table_type;

    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& config, StreamT& stream, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::manifold::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        assert(route_index.valid());
        const auto& routing_component = composition.template at<udho::manifold::feature::locator, 0>().component();
        const auto& routing_table = routing_component.table();
        // }

        // { add finish lambda to handler component
        auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda = [&p, &stream, flow, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
            if(error) {
                // TODO Error while writing to socket
                return;
            }

            std::apply(
                [&](auto&&... args) {
                    p.next(flow, stream, std::forward<Args>(args)...);
                },
                args_tuple
            );
        };
        using handler_type = udho::manifold::components::basic_handler<StreamT>;
        using ostream_type = udho::net::basic_ostream<StreamT>;

        handler_type& handler = composition.template get<handler_type>().component();
        ostream_type& ostream = handler.add(flow->id(), stream, std::move(lambda));
        // }

        // { create context
        portal_type portal(composition, configs, journal);
        std::string resource = portal.resource();
        std::cout << "resource: " << resource << std::endl;
        context_type context(ostream, portal, flow->id());
        // }

        // { invoke action
        routing_table.invoke_at(route_index, resource, context);
        // }
    }
};

// } transitions

// { terminal

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct basic_terminal<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT> {
    using label_type        = www::basic_label<StreamT, Tag, ExtraComponents...>;
    using stream_type       = StreamT;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;

    basic_terminal() = delete;
    basic_terminal(const basic_terminal&) = delete;

    basic_terminal(composition_type& composition, const configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    bool reenter(stream_type& stream) { return true; }
    void prepare(stream_type& stream) { }
    bool error(udho::manifold::exclusive_result success, stream_type& stream){
        if(success.has_exception()) {
            try{
                success.rethrow();
            } catch(const std::exception& ex) {
                std::cout << "exception: " << ex.what() << std::endl;
            }
        }
        return false;
    }

private:
    composition_type&   _composition;
    const configs_type& _configs;
    const journal_type& _journal;
};

// } terminal

namespace detail{

template <typename Label, typename StreamT, typename RoutingTableT>
struct runtime_proxy{
    using router_type   = udho::url::basic_router<RoutingTableT>;
    using handler_type  = udho::manifold::components::basic_handler<StreamT>;
    using routing_type  = udho::manifold::components::routing<router_type>;
    using rlabel_type   = typename Label::template append<routing_type>;
    using runtime_type  = udho::manifold::basic_runtime<rlabel_type, StreamT>;

    runtime_proxy() = delete;
    runtime_proxy(const runtime_proxy&) = delete;

    runtime_proxy(RoutingTableT&& table): _router(std::move(table)), _handler(_router.table().summary()), _routing(_router) {}

    template <typename... Components>
    runtime_type runtime(Components&&... components) {
        return runtime_type(_routing, _handler, std::forward<Components>(components)...);
    }

private:
    router_type  _router;
    handler_type _handler;
    routing_type _routing;
};

}

template <typename LabelT>
struct framework;

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct framework<www::basic_label<StreamT, Tag, ExtraComponents...>>  {
    using endpoint_type = typename StreamT::endpoint_type;

    template <typename RoutingTableT>
    static detail::runtime_proxy<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, RoutingTableT> apply(RoutingTableT&& table) {
        return detail::runtime_proxy<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, RoutingTableT>(std::forward<RoutingTableT>(table));
    }

};

}
}

#endif // UDHO_MANIFOLD_WWW_H
