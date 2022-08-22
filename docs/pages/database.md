Database 
===================

Declaration
------------

A field has a name and is associated with a PostgreSQL data type. The postgresql type has
a C++ type associated with it. Get and Set operations on that field expects the value operand
to be of that C++ type. The fields are created using the @ref PG_ELEMENT macro, which takes 
the name of the field and the PostgreSQL type as inputs.

A relation specifies a set of fields that its consists of and it has a name e.g. a table or a 
view, A relation `X` subclasses from the @ref udho::db::pg::relation "pg::relation<X, Fields...>",
where `Fields...` are the fields inside that relation.

@code {.cpp}

namespace authors{
    PG_ELEMENT(id,          pg::types::integer);
    PG_ELEMENT(first_name,  pg::types::varchar);
    PG_ELEMENT(last_name,   pg::types::varchar);

   struct table: pg::relation<table, id, first_name, last_name>{
        PG_NAME(users)
        using readonly = pg::readonly<id>;
    };

}

namespace articles{
    PG_ELEMENT(id,          pg::types::integer);
    PG_ELEMENT(author,      pg::types::integer);
    PG_ELEMENT(title,       pg::types::varchar);
    
    struct table: pg::relation<table, id, author, title>{
        PG_NAME(users)
        using readonly = pg::readonly<id>;
    };

}

@endcode 

@note In this example we put the fields and the relation inside the a namespace. However, it is strictly not
      necessary. Usercode may even reuse one field for multiple relations. But such practice may reduce 
      readability of the code.


Query
------

The select queries start from @ref udho::db::pg::from "pg::from<RelationT>" where the `RelationT` is the struct 
that defines the relation. In the following example `all` represents a `SELECT * from users` query. 

@code {.cpp}
using all = pg::from<authors::table>
   ::fetch
   ::all
   ::apply;
@endcode



Connection
-----------

A connection pool has to be created using ozo

@code {.cpp}
ozo::connection_pool_config dbconfig;
const auto oid_map = ozo::register_types<>();
ozo::connection_info<> conn_info("host=localhost dbname=DBNAME user=USERNAME password=PASSWORD");
auto pool = ozo::connection_pool(conn_info, dbconfig);
@endcode
Activity 
--------- 

@code {.cpp}
auto start = udho::db::pg::start<all>::with(ctx, _pool);
auto fetch_all = pg::after(start).perform<all>(start);
pg::after(fetch_subscribers).finish(start, [ctx](const pg::data<all>& d) mutable{
  auto users = d.success<all>();
  for(const auto& user: users){
    int id                 = user[id];
    std::string first_name = user[first_name];
    std::string last_name  = user[last_name];
  }
});
@endcode
