Query
===================

Given the following model.

@code {.cpp}

namespace authors{
  PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
  PG_ELEMENT(first_name,  pg::types::varchar,   pg::constraints::not_null);
  PG_ELEMENT(last_name,   pg::types::varchar);
  PG_ELEMENT(writer,      pg::types::bigint);
  PG_ELEMENT(name,        pg::types::text);

  struct table: pg::relation<table, id, first_name, last_name>{
      PG_NAME(users)
      using readonly = pg::readonly<id>;
  };

}

namespace articles{
  PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
  PG_ELEMENT(title,       pg::types::varchar,   pg::constraints::unique,   pg::constraints::not_null);
  PG_ELEMENT(abstract,    pg::types::text);
  PG_ELEMENT(author,      pg::types::bigint,    pg::constraints::not_null, pg::constraints::references<students::table::column<students::id>>::restrict);
  PG_ELEMENT(project,     pg::types::bigint);
  PG_ELEMENT(created,     pg::types::timestamp, pg::constraints::not_null, pg::constraints::default_<pg::constants::now>::value);

  struct table: pg::relation<table, id, title, abstract, author, project, created>{
      PG_NAME(articles)
      using readonly = pg::readonly<id, created>;
  };

}

@endcode

There can be various types of SQL queries.

Select
-------

@code {.cpp}
using all = pg::from<authors::table>
   ::fetch
   ::all
   ::apply;
@endcode

A more complicated one below.

- `::fetch` expects `0...*` records in the resultset.
- `::retrieve` expects `0...1` records.
- `::all` includes all fields of the relation in the select query.
- `::exclude` removes a subset of fields from the select query.
- `::include` includes a subset of fields in the select query.
- `::text` casts the field to postgresql `text` datatype.
- `::pg::concat` concats fields with string constants.
- `::as` creates an alias.
- `::by` specifies fields that will go in the where query

@code {.cpp}
using all = pg::from<students::table>
    ::fetch
    ::all
    ::exclude<students::first_name, students::last_name>
    ::include<students::first_name::text, students::last_name::text>
    ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
    ::apply;
@endcode

@code {.cpp}
using by_id = pg::from<students::table>
    ::retrieve
    ::all
    ::exclude<students::first_name, students::last_name>
    ::include<students::first_name::text, students::last_name::text>
    ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
    ::by<students::id>
    ::apply;
@endcode

@code {.cpp}
using fullname = pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>;
using like = pg::from<students::table>
    ::fetch
    ::all
    ::include<fullname::as<students::name>>
    ::by<students::id::gte, fullname::like>
    ::apply;
@endcode


