#ifndef UDHO_WWW_COMPONENTS_SESSION_H
#define UDHO_WWW_COMPONENTS_SESSION_H

#include <udho/session/catalogue.h>
#include <udho/www/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/portal.h>
#include <udho/utils/string_view.h>
#include <boost/uuid/uuid.hpp>
#include <iostream>
#include <udho/www/components/params.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{

namespace components{

/**
 * @brief Component providing access to a session catalogue.
 * @tparam StorageT Session storage type.
 * @tparam Mode Session catalogue mode.
 * @ingroup DoxyG_www_components
 */
template <typename StorageT, udho::session::modes Mode>
struct session{
    using catalogue_type = udho::session::catalogue<StorageT, Mode>;
    using key_type       = typename catalogue_type::key_type;
    using note_type      = typename catalogue_type::note_type;

    using features = udho::manifold::features<udho::www::feature::session_load>;
    using params   = udho::manifold::params<
        udho::www::params::session::enabled,
        udho::www::params::session::sesskey,
        udho::www::params::session::domain,
        udho::www::params::session::path
    >;

    static constexpr const udho::utils::string_view name = "session";

public:
    /**
     * @brief Constructs the component with a referenced catalogue.
     * @param cat Session catalogue referenced by the component.
     */
    session(catalogue_type& cat): _catalogue(cat) {}
public:
    /**
     * @brief Borrows a session note from the catalogue.
     * @param sessid Session identifier.
     * @param expect_existing Whether the catalogue should require an existing session.
     */
    note_type borrow(const key_type& sessid, bool expect_existing = false) { return _catalogue.borrow(sessid, expect_existing); }

    /**
     * @brief Removes a session from the catalogue.
     * @param sessid Session identifier.
     */
    void remove(const key_type& sessid) { _catalogue.remove(sessid); }

    /**
     * @brief Returns whether the catalogue contains the session id.
     * @param sessid Session identifier.
     */
    bool exists(const key_type& sessid) { return _catalogue.exists(sessid); }

    /** @brief Generates a random session id. */
    udho::session::id generate() const { return udho::session::random(); }
private:
    catalogue_type& _catalogue;
};

} // components
} // www

namespace manifold{

/**
 * @brief Loads a configured session from the request cookie jar.
 * @tparam StorageT Session storage type.
 * @tparam Mode Session catalogue mode.
 * @ingroup DoxyG_www_components_facets
 */
template <typename StorageT, udho::session::modes Mode>
struct facet<udho::www::components::session<StorageT, Mode>, udho::www::feature::session_load>{
    using component_type  = udho::www::components::session<StorageT, Mode>;
    using config_type     = udho::manifold::config<component_type>;
    using note_type       = typename component_type::note_type;

    /**
     * @brief Constructs the facet.
     * @param component Session component.
     * @param config Component configuration.
     * @param id Flow identifier.
     */
    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    /**
     * @brief Loads a session selected by the configured cookie.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     */
    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next) {
        const udho::cookies::jar& jar = journal.template first_of<udho::www::feature::cookie_load>();

        const std::string& name   = _config[udho::www::params::session::sesskey::val].value();
        const std::string& domain = _config[udho::www::params::session::domain::val].value();
        const std::string& path   = _config[udho::www::params::session::path::val].value();
        bool enabled              = _config[udho::www::params::session::enabled::val].value();

        if(!enabled) {
            next.skip();
            return;
        }

        // if there is no sessid cookie
        //      then don't create one
        //  else
        //      if sessid exists -> load session
        //      else             -> ignore sessid
        //          but usercode can spot the discrepency by checking existance
        //          of sessid cookie in cookie jar but no session note
        if(jar.count(name, domain, path) > 0) {
            const udho::cookies::jar::cookie_str_type& cookie = jar.get(name, domain, path);
            udho::session::id sessid = udho::session::from_string(cookie.value());
            try{
                udho::session::note note = _component.borrow(sessid, true); // throws if sessid doesn't exist
                next.pass(std::move(note));
            } catch(const std::exception& ex) {
                next.skip();
            }
        } else{
            next.skip();
            return;
        }
    }

    /**
     * @brief Invokes session loading.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @tparam Stream Stream type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) {
        std::cout << "-> facet<omponents::session<StorageT, Mode>, udho::www::feature::session_load>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next));
    }
private:
    component_type&     _component;
    const config_type&  _config;
};

/**
 * @brief Portal accessor for the session component.
 * @tparam StorageT Session storage type.
 * @tparam Mode Session catalogue mode.
 * @tparam JournalT Journal view type.
 * @ingroup DoxyG_www_components_accessors
 */
template <typename StorageT, udho::session::modes Mode, typename JournalT>
struct accessor<udho::www::components::session<StorageT, Mode>, JournalT>: basic_accessor<udho::www::components::session<StorageT, Mode>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::session<StorageT, Mode>, JournalT>;
    using component_type        = udho::www::components::session<StorageT, Mode>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;
};

} // manifold
} // udho

/** @} */

#endif // UDHO_WWW_COMPONENTS_SESSION_H
