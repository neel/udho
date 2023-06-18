/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_DB_PG_TESTS_SCHEMA_H
#define UDHO_DB_PG_TESTS_SCHEMA_H

#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <tuple>
#include <regex>
#include <boost/hana/string.hpp>
#include <udho/db/pg/schema/defs.h>
#include <udho/db/pg/schema/field.h>
#include <udho/db/pg/schema/relation.h>
#include <udho/db/pg/schema/constraints.h>

namespace db = udho::db;
namespace pg = db::pg;

// { relations

namespace students{

    PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
    PG_ELEMENT(first_name,  pg::types::varchar, pg::constraints::not_null);
    PG_ELEMENT(last_name,   pg::types::varchar);
    PG_ELEMENT(marks,       pg::types::integer);
    PG_ELEMENT(age,         pg::types::integer);
    PG_ELEMENT(writer,      pg::types::bigint);
    PG_ELEMENT(name,        pg::types::text);

    struct table: pg::relation<table, id, first_name, last_name, marks, age>{
        PG_NAME(students)
        using readonly = pg::readonly<id>;
    };

}

namespace projects{

    PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
    PG_ELEMENT(title,       pg::types::varchar);
    PG_ELEMENT(started,     pg::types::timestamp, pg::constraints::not_null, pg::constraints::default_<pg::constants::now>::value);
    PG_ELEMENT(admin,       pg::types::bigint,    pg::constraints::references<students::table::column<students::id>>::restrict);

    struct table: pg::relation<table, id, title, started, admin>{
        PG_NAME(projects)
        using readonly = pg::readonly<id>;
    };

}

namespace articles{

    PG_ELEMENT(id,        pg::types::bigserial, pg::constraints::primary);
    PG_ELEMENT(title,     pg::types::varchar,   pg::constraints::unique);
    PG_ELEMENT(author,    pg::types::bigint,    pg::constraints::references<students::table::column<students::id>>::restrict);
    PG_ELEMENT(project,   pg::types::bigint,    pg::constraints::references<projects::table::column<projects::id>>::restrict);
    PG_ELEMENT(published, pg::types::timestamp, pg::constraints::not_null, pg::constraints::default_<pg::constants::now>::value);
    PG_ELEMENT(content,   pg::types::text);

    struct table: pg::relation<table, id, title, author, project, published, content>{
        PG_NAME(articles)
        using readonly = pg::readonly<id>;
    };

}

namespace memberships{

    PG_ELEMENT(id,      pg::types::bigserial, pg::constraints::primary);
    PG_ELEMENT(student, pg::types::bigint, pg::constraints::references<students::table::column<students::id>>::restrict);
    PG_ELEMENT(project, pg::types::bigint, pg::constraints::references<projects::table::column<projects::id>>::restrict);

    struct table: pg::relation<table, id, student, project>{
        PG_NAME(memberships)
        using readonly = pg::readonly<id>;
    };

}

// } relations


#endif // UDHO_DB_PG_TESTS_SCHEMA_H
