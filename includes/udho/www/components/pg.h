#ifndef UDHO_WWW_COMPONENTS_DB_PG_H
#define UDHO_WWW_COMPONENTS_DB_PG_H

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <udho/www/features.h>
#include <udho/manifold/portal.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{

namespace components{
namespace db{

/**
 * @brief Component owning an OZO PostgreSQL connection pool.
 * @tparam OidMap OZO OID map type.
 * @tparam Statistics OZO statistics type.
 * @ingroup DoxyG_www_components
 */
template <typename OidMap = ozo::empty_oid_map, typename Statistics = ozo::no_statistics>
struct pg{
    using features = udho::manifold::features<>;
    static constexpr const char* name = "pg";
    using params      = udho::manifold::params<>;

    using connection_source_type = ozo::connection_info<OidMap, Statistics>;
    using pool_config_type       = ozo::connection_pool_config;
    using connection_pool_type   = ozo::connection_pool<connection_source_type>;

    /**
     * @brief Constructs the component and its connection pool.
     * @param conn_str PostgreSQL connection string.
     */
    pg(const std::string& conn_str = "dbname=postgres user=postgres"): _info(conn_str), _pool(_info, _config) {}

    /** @brief Returns the mutable pool configuration. */
    pool_config_type& config() { return _config; }
    /** @brief Returns the pool configuration. */
    const pool_config_type& config() const { return _config; }

    /** @brief Returns the connection pool. */
    connection_pool_type& pool() { return _pool; }

private:
    pool_config_type       _config;
    connection_source_type _info;
    connection_pool_type   _pool;
};

}
} // components
} // www

namespace manifold{

/**
 * @brief Portal accessor for the PostgreSQL connection pool.
 * @tparam OidMap OZO OID map type.
 * @tparam Statistics OZO statistics type.
 * @tparam JournalT Journal view type.
 * @ingroup DoxyG_www_components
 */
template <typename OidMap, typename Statistics, typename JournalT>
struct accessor<udho::www::components::db::pg<OidMap, Statistics>, JournalT>: basic_accessor<udho::www::components::db::pg<OidMap, Statistics>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::db::pg<OidMap, Statistics>, JournalT>;
    using component_type        = udho::www::components::db::pg<OidMap, Statistics>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;
    using connection_pool_type  = typename component_type::connection_pool_type;

    using basic_accessor_type::basic_accessor_type;

    /** @brief Returns the component's connection pool. */
    connection_pool_type& pool() {
        return basic_accessor_type::component().pool();
    }
};

}   // manifold
}   // udho

/** @} */

#endif // UDHO_WWW_COMPONENTS_DB_PG_H
