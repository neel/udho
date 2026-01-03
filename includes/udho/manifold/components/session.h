#ifndef UDHO_MANIFOLD_COMPONENTS_SESSION_H
#define UDHO_MANIFOLD_COMPONENTS_SESSION_H

#include <udho/session/catalogue.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/utils/string_view.h>
#include <boost/uuid/uuid.hpp>

namespace udho{
namespace manifold{

namespace components{

template <typename StorageT, udho::session::modes Mode>
struct session{
    using catalogue_type = udho::session::catalogue<StorageT, Mode>;
    using key_type       = typename catalogue_type::key_type;
    using note_type      = typename catalogue_type::note_type;

    UDHO_CONFIG_PARAM(enabled, bool,        false);             // skip if not enabled
    UDHO_CONFIG_PARAM(follow,  bool,        true);              // Don't auto create session id but follow existing sessionid if it already exists
    UDHO_CONFIG_PARAM(sesskey, std::string, "sessid");
    UDHO_CONFIG_PARAM(domain,  std::string, "localhost");
    UDHO_CONFIG_PARAM(path,    std::string, "/");

    using features = udho::manifold::features<udho::manifold::feature::session_load>;
    using params   = udho::manifold::params<enabled, follow, sesskey, domain, path>;

    static constexpr const udho::utils::string_view name = "session";

public:
    session(catalogue_type& cat): _catalogue(cat) {}
public:
    note_type borrow(const key_type& sessid) { return _catalogue.borrow(sessid); }

    void remove(const key_type& sessid) { _catalogue.remove(sessid); }

    bool exists(const key_type& sessid) { return _catalogue.exists(sessid); }

    udho::session::id generate() const { return udho::session::random(); }
private:
    catalogue_type& _catalogue;
};

}

template <typename StorageT, udho::session::modes Mode>
struct facet<components::session<StorageT, Mode>, udho::manifold::feature::session_load>{
    using component_type  = components::session<StorageT, Mode>;
    using config_type     = udho::manifold::config<component_type>;
    using note_type       = typename component_type::note_type;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next) {
        const udho::cookies::jar& jar = journal.template first_of<udho::manifold::feature::cookie_load>();

        const std::string& name   = _config[component_type::sesskey::val].value();
        const std::string& domain = _config[component_type::domain::val].value();
        const std::string& path   = _config[component_type::path::val].value();
        bool enabled              = _config[component_type::enabled::val].value();
        bool follow               = _config[component_type::follow::val].value();

        if(!enabled) {
            next.skip();
            return;
        }

        bool sessid_exists = jar.count(name, domain, path) > 0;
        udho::session::id sessid;
        if(sessid_exists) {
            const udho::cookies::jar::cookie_str_type& cookie = jar.get(name, domain, path);
            sessid = udho::session::from_string(cookie.value());
        } else if(follow) {
            sessid = _component.generate();
        }

        if(sessid.is_nil()) {
            next.skip();
            return;
        } else {
            next.pass(_component.borrow(sessid));
        }
    }

    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) {
        std::cout << "-> facet<omponents::session<StorageT, Mode>, udho::manifold::feature::session_load>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next));
    }
private:
    component_type&     _component;
    const config_type&  _config;
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_SESSION_H
