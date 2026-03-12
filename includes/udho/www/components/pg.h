#ifndef UDHO_WWW_COMPONENTS_DB_PG_H
#define UDHO_WWW_COMPONENTS_DB_PG_H

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <udho/www/features.h>
#include <udho/manifold/portal.h>

namespace udho{
namespace www{

namespace components{
namespace db{

template <typename OidMap = ozo::empty_oid_map, typename Statistics = ozo::no_statistics>
struct pg{
    using features = udho::manifold::features<>;
    static constexpr const char* name = "pg";
    using params      = udho::manifold::params<>;

    using connection_source_type = ozo::connection_info<OidMap, Statistics>;
    using pool_config_type       = ozo::connection_pool_config;
    using connection_pool_type   = ozo::connection_pool<connection_source_type>;

    /**
     * @brief pg
     * @param conn_str connection string which is being used to create connection to a database.
     */
    pg(const std::string& conn_str = "dbname=postgres user=postgres"): _info(conn_str), _pool(_info, _config) {}

    pool_config_type& config() { return _config; }
    const pool_config_type& config() const { return _config; }

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

template <typename OidMap, typename Statistics, typename JournalT>
struct accessor<udho::www::components::db::pg<OidMap, Statistics>, JournalT>: basic_accessor<udho::www::components::db::pg<OidMap, Statistics>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::db::pg<OidMap, Statistics>, JournalT>;
    using component_type        = udho::www::components::db::pg<OidMap, Statistics>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;
};

}   // manifold
}   // udho

#endif // UDHO_WWW_COMPONENTS_DB_PG_H
