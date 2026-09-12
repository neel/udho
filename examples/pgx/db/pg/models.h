// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_PGX_DB_PG_MODELS_H
#define UDHO_EXAMPLES_PGX_DB_PG_MODELS_H

#include "schema.h"
#include <udho/db/pg/generators/generators.h>

namespace pgx::db::pg::models {

namespace pg = ::udho::db::pg;
namespace schema = pgx::db::pg::schema;

namespace projects {

using all = pg::from<schema::projects::table>
    ::fetch
    ::all
    ::descending<schema::projects::created>
    ::apply;

using by_id = pg::from<schema::projects::table>
    ::retrieve
    ::all
    ::by<schema::projects::id>
    ::apply;

using create = pg::into<schema::projects::table>
    ::insert
    ::writables
    ::returning<schema::projects::id>
    ::apply;

using update = pg::into<schema::projects::table>
    ::update
    ::writables
    ::by<schema::projects::id>
    ::apply;

using remove = pg::from<schema::projects::table>
    ::remove
    ::by<schema::projects::id>
    ::apply;

}

namespace tasks {

using all = pg::from<schema::tasks::table>
    ::fetch
    ::all
    ::descending<schema::tasks::created>
    ::apply;

using by_id = pg::from<schema::tasks::table>
    ::retrieve
    ::all
    ::by<schema::tasks::id>
    ::apply;

using create = pg::into<schema::tasks::table>
    ::insert
    ::writables
    ::returning<schema::tasks::id>
    ::apply;

using update = pg::into<schema::tasks::table>
    ::update
    ::writables
    ::by<schema::tasks::id>
    ::apply;

using remove = pg::from<schema::tasks::table>
    ::remove
    ::by<schema::tasks::id>
    ::apply;

// Additional queries used by the project-summary pipeline.
using by_project = pg::from<schema::tasks::table>
    ::fetch
    ::all
    ::by<schema::tasks::project>
    ::ascending<schema::tasks::created>
    ::apply;

using incomplete_by_project = pg::from<schema::tasks::table>
    ::fetch
    ::all
    ::by<schema::tasks::project, schema::tasks::completed>
    ::ascending<schema::tasks::created>
    ::apply;

}

}

#endif // UDHO_EXAMPLES_PGX_DB_PG_MODELS_H
