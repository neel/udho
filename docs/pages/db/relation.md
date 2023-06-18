Pg Relations {#PgRelations}
===================

Given the following model.

@code {.cpp}


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

@endcode

There can be various types of SQL queries.
