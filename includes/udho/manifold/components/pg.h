#ifndef UDHO_MANIFOLD_COMPONENTS_DB_PG_H
#define UDHO_MANIFOLD_COMPONENTS_DB_PG_H

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <udho/manifold/features.h>

namespace udho{
namespace manifold{

namespace components{
namespace db{

template <typename OidMap = ozo::empty_oid_map, typename Statistics = ozo::no_statistics>
struct pg{
    using features = udho::manifold::features<>;
    static constexpr const char* name = "pg";

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
}



}
}

#endif // UDHO_MANIFOLD_COMPONENTS_DB_PG_H
