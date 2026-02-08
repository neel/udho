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
#include <udho/exceptions/exceptions.h>

namespace udho {
namespace manifold {

namespace www {

namespace tags{

    template <typename... Bridges>
    struct minimal{
        using resources_component_type = udho::manifold::components::resources<Bridges...>;
    };

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

namespace stateless {
    using rest                  = www::label<www::tags::minimal<>>;
    using lua                   = www::label<www::tags::minimal<udho::view::data::bridges::lua>>;
}
namespace stateful {
    using lazy_fs               = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
    using optimistic_fs         = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
    using lazy_memfs            = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>>;
    using optimistic_memfs      = www::label<www::tags::stateful<udho::session::storage::fs, udho::session::modes::optimistic>>;
    using lazy_redis            = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::lazy>>;
    using optimistic_redis      = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::optimistic>>;
    using immediate_redis       = www::label<www::tags::stateful<udho::session::storage::redis, udho::session::modes::immediate>>;

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

template <typename StreamT, typename... Bridges, typename... ExtraComponents>
struct sketch<www::basic_label<StreamT, www::tags::minimal<Bridges...>, ExtraComponents...>>{
    using www_type = www::basic_label<StreamT, www::tags::minimal<Bridges...>, ExtraComponents...>;

    using stream_type = StreamT;

    using composition_type = udho::manifold::composition<
        udho::manifold::components::basic_handler<StreamT>,
        udho::manifold::components::db::pg<>,
        udho::manifold::components::protocols::http<stream_type>,
        udho::manifold::components::navigators::pretty,
        udho::manifold::components::cookies,
        udho::manifold::components::resources<Bridges...>,
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
        udho::manifold::components::protocols::http<stream_type>,
        udho::manifold::components::navigators::pretty,
        udho::manifold::components::cookies,
        udho::manifold::components::session<SessionStorageT, Mode>,
        udho::manifold::components::resources<Bridges...>,
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

    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& config, stream_type& stream, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::manifold::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        // if(route_index.type() == udho::url::detail::route_index::type::none){
        //     // {{ add finish lambda to handler component
        //     auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        //     auto lambda = [&p, &stream, flow, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
        //         if(error) {
        //             // TODO Error while writing to socket
        //             return;
        //         }

        //         std::apply(
        //             [&](auto&&... args) {
        //                 p.abort(flow, stream, std::forward<Args>(args)...);
        //             },
        //             args_tuple
        //         );
        //     };
        //     using handler_type = udho::manifold::components::basic_handler<StreamT>;
        //     using ostream_type = udho::net::basic_ostream<StreamT>;

        //     handler_type& handler = composition.template get<handler_type>().component();
        //     ostream_type& ostream = handler.add(flow->id(), stream, std::move(lambda));
        //     // }}

        //     ostream.status(boost::beast::http::status::not_found);
        //     ostream.finish();
        //     return;
        // }
        if(route_index.type() != udho::url::detail::route_index::type::none) {
            const auto& routing_component = composition.template at<udho::manifold::feature::locator, 0>().component();
            const auto& router     = routing_component.router();
            router.reconfigure_for(route_index, configs);
        }
        // }
        p.next(flow, stream, std::forward<Args>(args)...);
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


    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& config, stream_type& stream, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::manifold::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        const auto& routing_component = composition.template at<udho::manifold::feature::locator, 0>().component();
        const auto& router = routing_component.router();
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
        router.invoke_at(route_index, context);
        // }
    }
};

// } transitions

// { terminal

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct basic_terminal<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT> {
    using label_type        = www::basic_label<StreamT, Tag, ExtraComponents...>;
    using stream_type       = StreamT;
    using ostream_type      = udho::net::basic_ostream<StreamT>;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using handler_type      = udho::manifold::components::basic_handler<StreamT>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;

    basic_terminal() = delete;
    basic_terminal(const basic_terminal&) = delete;

    basic_terminal(composition_type& composition, const configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    /**
     * @brief reenter is synchronously called after successful evaluation of all facet pipelines in all stages
     *        to determine whether to process next request or abort.
     * @param stream
     * @return bool
     * @note Call originates from basic_flow<LabelT, StreamT>::reenter()
     */
    bool reenter(stream_type& stream) { return true; }

    /**
     * @brief prepare's the flow for reentry in both success and failure circumstances
     * @param stream
     * @param args
     */
    template <typename... Args>
    void prepare(stream_type& stream, Args&&... args) { }

    /**
     * @brief error function is called to handle the error situation occurred during evaluation of some facet.
     *
     * This function asynchronously determines the either of the following two pathways
     *
     * 1. flow.restart() respond, reset and process the next request in the same socket
     * 2. flow.abort()   remove the flow, terminate the socket
     *
     * @param success
     * @param flow
     * @param stream
     * @param args
     * @pre flow is owned by runtime and outlives the invocation of this callback
     * @post flow either restarts or aborts
     * @note Call originate from basic_flow<LabelT, StreamT>::error which originates from failure handler
     *       passed to facet triggered by calling next.fail(...)
     */
    template <typename... Args>
    void error(udho::manifold::exclusive_result success, flow_type& flow, stream_type& stream, Args&&... args){
        if(success.has_exception()) {
            try{
                success.rethrow();
            } catch(const udho::http::error& error) {
                std::cout << "exception: " << error.what() << std::endl;
                handle_error(flow, error, stream, std::forward<Args>(args)...);
            } catch(boost::system::error_code error) {
                std::cout << "system error: " << error << std::endl;
                handle_error(flow, error, stream, std::forward<Args>(args)...);
            }catch(const std::exception& ex) {
                std::cout << "exception: " << ex.what() << std::endl;
                handle_error(flow, ex, stream, std::forward<Args>(args)...);
            }
        }
    }

private:

    template <typename... Args>
    void handle_error(flow_type& flow, const udho::http::error& error, stream_type& stream, Args&&... args) {
        ostream_type& ostream = get_ostream(flow, true, stream, std::forward<Args>(args)...);
        ostream.status(error.status());
        ostream.finish();
    }

    template <typename... Args>
    void handle_error(flow_type& flow, boost::system::error_code error, stream_type& stream, Args&&... args) {
        if(error == boost::asio::error::eof) {
            flow.abort();
        }

        flow.abort();
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const std::exception& error, stream_type& stream, Args&&... args) {
        flow.abort();
    }

private:

    template <typename... Args>
    ostream_type& get_ostream(flow_type& flow, bool restart, stream_type& stream, Args&&... args) {
        auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda = [&flow, restart, &stream, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
            if(error) {
                // TODO Error while writing to socket
                return;
            }

            if(restart) {
                std::apply(
                    [&](auto&&... args) {
                        flow.restart(stream, std::forward<Args>(args)...);
                    },
                    args_tuple
                );
            } else {
                flow.abort();
            }
        };

        handler_type& handler = _composition.template get<handler_type>().component();
        ostream_type& ostream = handler.add(flow.id(), stream, std::move(lambda));
        return ostream;
    }

private:
    composition_type&   _composition;
    const configs_type& _configs;
    const journal_type& _journal;
};

// } terminal

namespace detail{

template <typename Label, typename StreamT, typename RouterT>
struct runtime_proxy{
    using router_type   = RouterT;
    using handler_type  = udho::manifold::components::basic_handler<StreamT>;
    using routing_type  = udho::manifold::components::routing<router_type>;
    using rlabel_type   = typename Label::template append<routing_type>;
    using runtime_type  = udho::manifold::basic_runtime<rlabel_type, StreamT>;

    runtime_proxy() = delete;
    runtime_proxy(const runtime_proxy&) = delete;

    runtime_proxy(RouterT&& router): _handler(router.summary()), _routing(std::move(router)) {}

    template <typename... Components>
    runtime_type runtime(Components&&... components) {
        return runtime_type(_routing, _handler, std::forward<Components>(components)...);
    }

    const router_type& router() const { return _routing.router(); }

private:
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
    static detail::runtime_proxy<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, udho::url::basic_router<RoutingTableT>> apply(udho::url::basic_router<RoutingTableT>&& router) {
        return detail::runtime_proxy<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, udho::url::basic_router<RoutingTableT>>(std::forward<udho::url::basic_router<RoutingTableT>>(router));
    }

};

}
}

#endif // UDHO_MANIFOLD_WWW_H
