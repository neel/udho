// SPDX-License-Identifier: BSD-3-Clause
#ifndef UDHO_EXAMPLES_PGX_SETUP_DB_PG_MODELS_H
#define UDHO_EXAMPLES_PGX_SETUP_DB_PG_MODELS_H

#include "../../../db/pg/schema.h"
#include <udho/db/pg/crud/create.h>
#include <udho/db/pg/generators/generators.h>

namespace pgx::setup::db::pg::models {

namespace pg     = udho::db::pg;
namespace schema = pgx::db::pg::schema;

namespace projects {

using create = pg::ddl<schema::projects::table>
    ::create
    ::if_exists
    ::skip
    ::apply;

}

namespace tasks {

using create = pg::ddl<schema::tasks::table>
    ::create
    ::if_exists
    ::skip
    ::apply;

}

}
#endif
