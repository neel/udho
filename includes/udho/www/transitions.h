#ifndef UDHO_WWW_TRANSITIONS_H
#define UDHO_WWW_TRANSITIONS_H

#include <udho/www/features.h>
#include <udho/www/label.h>
#include <udho/manifold/transition.h>
#include <udho/manifold/portal.h>
#include <udho/www/components/handler.h>
#include <udho/logging/macros.h>
#include <udho/www/pages.h>

namespace udho {
namespace manifold {

// { transitions

static constexpr const std::size_t route_locator_stage = udho::www::feature::locator::stage;

/**
 * @brief Default www transition that applies route-specific configuration.
 *
 * After the route locator has produced a valid route index, this transition
 * asks the routing component to reconfigure the next-stage configs according
 * to the selected route. It then advances the flow to the next pipeline stage.
 *
 * @tparam StreamT Stream type used by the runtime.
 * @tparam Tag www label tag.
 * @tparam ExtraComponents User-supplied extra components appended to the www label.
 *
 * @ingroup www
 */
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

    /**
     * @brief Applies route-derived configuration and advances the pipeline.
     *
     * Reads the `locator` result from the journal. When a route is found, the
     * routing component is retrieved from the composition and used to patch the
     * runtime configs for the selected route.
     *
     * @tparam Args Additional runtime argument types forwarded to the next stage.
     * @param flow Flow being processed.
     * @param p Current pipeline stage.
     * @param config Configuration object supplied by the transition protocol.
     * @param stream Active stream.
     * @param args Additional runtime arguments.
     */
    template <typename... Args>
    static void apply(flow_type& flow, pipeline_type& p, configs_type& config, stream_type& stream, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::www::feature::locator>();
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
            const auto& routing_component = composition.template at<udho::www::feature::locator, 0>().component();
            const auto& router     = routing_component.router();
            router.reconfigure_for(route_index, configs);
        }
        // }
        p.next(flow, stream, std::forward<Args>(args)...);
    }
};

/**
 * @brief Default www transition that invokes the selected route action.
 *
 * This transition creates an output stream entry in the handler, builds a portal
 * and context for the current request, and invokes the route action selected by
 * the router. Pipeline continuation is deferred until the response stream
 * finishes.
 *
 * @tparam StreamT Stream type used by the runtime.
 * @tparam Tag www label tag.
 * @tparam ExtraComponents User-supplied extra components appended to the www label.
 *
 * @ingroup www
 */
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

    /**
     * @brief Creates request context and invokes the selected route action.
     *
     * Registers response-finish and exception callbacks with the handler component,
     * constructs a portal and request context, logs the selected URI, and dispatches
     * the selected route through `router.invoke_at(route_index, context)`.
     *
     * @tparam Args Additional runtime argument types forwarded when the response
     *         finishes or when a user exception is detected.
     * @param flow Flow being processed.
     * @param p Current pipeline stage.
     * @param config Configuration object supplied by the transition protocol.
     * @param stream Active stream.
     * @param args Additional runtime arguments.
     */
    template <typename... Args>
    static void apply(flow_type& flow, pipeline_type& p, configs_type& config, stream_type& stream, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::www::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        const auto& routing_component = composition.template at<udho::www::feature::locator, 0>().component();
        const auto& router = routing_component.router();
        // }

        // { add finish lambda to handler component
        using handler_type = udho::www::components::basic_handler<StreamT>;
        using ostream_type = udho::net::basic_ostream<StreamT>;

        auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda = [&p, &stream, &flow, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
            if(error) {
                // TODO Error while writing to socket
                return;
            }

            namespace params = udho::logging::params;
            UDHO_LOG_INFO("udho::net::ostream", "Response finished", params::flow_id(flow.id()), params::socket_id(udho::utils::misc::native_handle(stream)));

            std::apply(
                [&](auto&&... args) {
                    p.next(flow, stream, std::forward<Args>(args)...);
                },
                args_tuple
            );
        };

        auto ex_lambda = [&flow, &stream, args_tuple = std::move(args_tuple)](ostream_type& ostream){
            if(ostream.has_exception()){
                const udho::exceptions::captured& capex = ostream.exception();
                std::apply(
                    [&](auto&&... args) {
                        flow.user_error(capex, stream, std::forward<Args>(args)...);
                    },
                    args_tuple
                );
            } else {
                ostream.finish();
            }
        };

        handler_type& handler = composition.template get<handler_type>().component();
        ostream_type& ostream = handler.add(flow.id(), stream, std::move(lambda), std::move(ex_lambda));

        // }

        // { create context
        portal_type portal(composition, configs, journal);
        // std::string resource = portal.resource();
        // std::cout << "resource: " << resource << std::endl;
        context_type context(ostream, portal, flow.id());
        // }

        udho::www::feature::identifier::result res = journal.template at<udho::www::feature::identifier>();

        namespace params = udho::logging::params;
        UDHO_LOG_INFO("www::transition2", "Invoked", params::flow_id(flow.id()), params::uri(route_index.type() == udho::url::detail::route_index::type::registry ? res.path() : res.resource()), params::socket_id(udho::utils::misc::native_handle(stream)));

        // { invoke action
        router.invoke_at(route_index, context);
        // }
    }
};

// } transitions


}
}

#endif // UDHO_WWW_TRANSITIONS_H
