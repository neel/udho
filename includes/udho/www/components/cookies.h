#ifndef UDHO_WWW_COMPONENTS_COOKIES_H
#define UDHO_WWW_COMPONENTS_COOKIES_H

#include <udho/www/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/fwd.h>
#include <udho/cookies/jar.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{

namespace components{

/**
 * @brief Component providing the cookie-loading feature.
 * @ingroup DoxyG_www_components
 */
struct cookies{
    using features = udho::manifold::features<udho::www::feature::cookie_load>;
    using params   = udho::manifold::params<>;

    static constexpr const udho::utils::string_view name = "cookies";
};

}   // components
}   // www

namespace manifold{

template <>
/**
 * @brief Evaluates cookie loading from the request header.
 * @ingroup DoxyG_www_components_facets
 */
struct facet<udho::www::components::cookies, udho::www::feature::cookie_load>{
    using component_type  = www::components::cookies;
    using request_type    = udho::net::types::headers::request;
    using config_type     = udho::manifold::config<component_type>;

    /**
     * @brief Constructs the facet.
     * @param component Cookies component.
     * @param config Component configuration.
     * @param id Flow identifier.
     */
    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    /**
     * @brief Performs cookie-loading evaluation and advances the pipeline.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @tparam Stream Stream type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        const udho::net::types::headers::request& request = journal.template first_of<udho::www::feature::header_reader>();
        udho::cookies::jar jar(request);
        next.pass(std::move(request));
    }

    /**
     * @brief Invokes cookie-loading evaluation.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @tparam Stream Stream type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        // std::cout << "-> facet<omponents::cookies, udho::www::feature::cookie_load>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }
private:
    component_type&     _component;
    const config_type&  _config;
};

}   // manifold
}   // udho

/** @} */

#endif // UDHO_WWW_COMPONENTS_COOKIES_H
