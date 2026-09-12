// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_PGX_DB_PG_SCHEMA_H
#define UDHO_EXAMPLES_PGX_DB_PG_SCHEMA_H

#include <udho/db/pg.h>
#include <udho/db/pg/schema/constraints.h>
#include <udho/db/pg/schema/relation.h>

namespace pgx::db::pg::schema {

namespace pg = udho::db::pg;

namespace projects {

PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
PG_ELEMENT(title,       pg::types::varchar,   pg::constraints::unique, pg::constraints::not_null);
PG_ELEMENT(description, pg::types::text,      pg::constraints::not_null);
PG_ELEMENT(created,     pg::types::timestamp, pg::constraints::not_null, pg::constraints::default_<pg::constants::now>::value);

struct table: pg::relation<table, id, title, description, created> {
    PG_NAME(projects)
    using readonly = pg::readonly<id, created>;
};

}

namespace tasks {

PG_ELEMENT(id,        pg::types::bigserial, pg::constraints::primary);
PG_ELEMENT(project,   pg::types::bigint,    pg::constraints::not_null, pg::constraints::references<projects::table::column<projects::id>>::cascade);
PG_ELEMENT(title,     pg::types::varchar,   pg::constraints::not_null);
PG_ELEMENT(completed, pg::types::boolean,   pg::constraints::not_null);
PG_ELEMENT(created,   pg::types::timestamp, pg::constraints::not_null, pg::constraints::default_<pg::constants::now>::value);

struct table: pg::relation<table, id, project, title, completed, created> {
    PG_NAME(tasks)
    using readonly = pg::readonly<id, created>;
};

}

}

#endif // UDHO_EXAMPLES_PGX_DB_PG_SCHEMA_H
