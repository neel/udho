#ifndef UDHO_MANIFOLD_COOKIES_H
#define UDHO_MANIFOLD_COOKIES_H

#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/fwd.h>
#include <udho/cookies/jar.h>

namespace udho{
namespace manifold{

namespace components{

struct cookies{
    using features = udho::manifold::features<udho::manifold::feature::cookie_load>;
    using params   = udho::manifold::params<>;

    static constexpr const udho::utils::string_view name = "navigator";
};

}

template <>
struct facet<components::cookies, udho::manifold::feature::cookie_load>{
    using component_type  = components::cookies;
    using request_type    = udho::net::types::headers::request;
    using config_type     = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        const udho::net::types::headers::request& request = journal.template first_of<udho::manifold::feature::header_reader>();
        udho::cookies::jar jar(request);
        next.pass(std::move(request));
    }

    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        std::cout << "-> facet<omponents::cookies, udho::manifold::feature::cookie_load>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }
private:
    component_type&     _component;
    const config_type&  _config;
};

}
}

#endif // UDHO_MANIFOLD_COOKIES_H
