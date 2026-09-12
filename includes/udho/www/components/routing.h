#ifndef UDHO_WWW_COMPONENTS_ROUTING_H
#define UDHO_WWW_COMPONENTS_ROUTING_H

#include <udho/url/router.h>
#include <udho/www/features.h>
#include <udho/manifold/config.h>
#include <udho/exceptions/exceptions.h>
#include <udho/www/components/params.h>
#include <udho/logging/macros.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{

namespace components{

/**
 * @brief Component owning a URL router and providing route-related features.
 * @tparam RouterT URL router type.
 * @ingroup DoxyG_www_components
 */
template <typename RouterT>
class routing{
    static_assert(udho::url::is_router<RouterT>::value);
    using router_type = RouterT;

private:
    router_type _router;
public:
    using features = udho::manifold::features<
        udho::www::feature::locator/*,
        udho::www::feature::responder*/
    >;

    using params   = udho::manifold::params<udho::www::params::routing::use_trie>;

    static constexpr const udho::utils::string_view name = "router";

    /**
     * @brief Constructs the component by moving in a router.
     * @param router Router to store.
     */
    routing(router_type&& router): _router(std::move(router)) {}

    /** @brief Returns the owned router. */
    router_type& router() { return _router; }
    /** @brief Returns the owned router. */
    const router_type& router() const { return _router; }

    /**
     * @brief Finds a route for an HTTP method and subject.
     * @param method HTTP method.
     * @param subject Route subject.
     */
    udho::url::detail::route_index locate(boost::beast::http::verb method, const std::string& subject) {
        udho::url::detail::route_index route = _router.index_of(method, subject);
        return route;
    }
};

} // components
} // www

namespace manifold {

/**
 * @brief Evaluates route lookup for the identified request target.
 * @tparam RouterT URL router type.
 * @ingroup DoxyG_www_components_facets
 */
template <typename RouterT>
struct facet<udho::www::components::routing<RouterT>, udho::www::feature::locator> {
    using component_type = udho::www::components::routing<RouterT>;
    using config_type    = udho::manifold::config<component_type>;

    /**
     * @brief Constructs the facet.
     * @param component Routing component.
     * @param config Component configuration.
     * @param id Flow identifier.
     */
    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config), _id(id) {}

    /**
     * @brief Locates the route identified by the journal results.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @tparam Stream Stream type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        const udho::www::feature::header_reader::result& request = journal.template at<udho::www::feature::header_reader>();
        udho::www::feature::identifier::result res               = journal.template at<udho::www::feature::identifier>();

        udho::url::detail::route_index index = _component.locate(request.method(), res.resource());
        bool is_path = false;
        if(!index.valid()) {
            index   = _component.locate(request.method(), res.path());
            is_path = index.valid();
        }

        namespace p = udho::logging::params;
        if(index.type() != udho::url::detail::route_index::type::none) {
            UDHO_LOG_INFO("udho::www::components::routing::facet::locator", "Resource located", p::uri(is_path ? res.path() : res.resource()), p::method(request.method()), p::flow_id(_id));

            next.pass(std::move(index));
        } else {
            std::string route = res.path();

            UDHO_LOG_INFO("udho::www::components::routing::facet::locator", "Failed to locate Resource", p::uri(route), p::method(request.method()), p::flow_id(_id));
            next.fail(udho::http::error(boost::beast::http::status::not_found, udho::utils::format("route not found {}", route), udho::http::error::options::close));
        }
    }

    /**
     * @brief Invokes route lookup.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @tparam Stream Stream type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        std::cout << "-> facet<components::routing<RoutingTableT>, udho::www::feature::locator>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }

private:
    component_type&     _component;
    const config_type&  _config;
    std::size_t         _id;
};

/**
 * @brief Invokes a previously located route.
 * @tparam RouterT URL router type.
 * @ingroup DoxyG_www_components_facets
 */
// template <typename RouterT>
// struct facet<udho::www::components::routing<RouterT>, udho::www::feature::responder> {
//     using component_type = udho::www::components::routing<RouterT>;
//     using facet_type     = facet<component_type, udho::www::feature::locator>;
//     using config_type    = udho::manifold::config<component_type>;

//     /**
//      * @brief Constructs the facet.
//      * @param component Routing component.
//      * @param config Component configuration.
//      * @param id Flow identifier.
//      */
//     facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

//     /**
//      * @brief Invokes the route stored in the journal.
//      * @tparam Components Components represented by the journal.
//      * @tparam NextT Continuation type.
//      * @tparam Stream Stream type.
//      * @param journal Current flow journal.
//      * @param next Pipeline continuation.
//      * @param stream Stream passed to the router.
//      */
//     template <typename... Components, typename NextT, typename Stream>
//     void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
//         udho::url::detail::route_index route_index = journal.template get<facet_type>();
//         bool success = _component.router().invoke_at(route_index, stream);
//         if(success) next.pass();
//         else        next.fail();
//     }

//     /**
//      * @brief Invokes responder evaluation.
//      * @tparam Components Components represented by the journal.
//      * @tparam NextT Continuation type.
//      * @tparam Stream Stream type.
//      * @param journal Current flow journal.
//      * @param next Pipeline continuation.
//      * @param stream Stream passed to the router.
//      */
//     template <typename... Components, typename NextT, typename Stream>
//     void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
//         std::cout << "-> facet<components::routing<RoutingTableT>, udho::www::feature::responder>::operator()(...)" << std::endl;
//         eval(journal, std::forward<NextT>(next), stream);
//     }

// private:
//     component_type& _component;
//     const config_type& _config;
// };

/**
 * @brief Portal accessor for the routing component.
 * @tparam RouterT URL router type.
 * @tparam JournalT Journal view type.
 * @ingroup DoxyG_www_components_accessors
 */
template <typename RouterT, typename JournalT>
struct accessor<udho::www::components::routing<RouterT>, JournalT>: basic_accessor<udho::www::components::routing<RouterT>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::routing<RouterT>, JournalT>;
    using component_type        = udho::www::components::routing<udho::www::components::routing<RouterT>>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;

};

} // manifold
} // udho

/** @} */

#endif // UDHO_WWW_COMPONENTS_ROUTING_H
