#define CATCH_CONFIG_MAIN
#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/activities.h>
#include <udho/db/pg/schema/relation.h>
#include <udho/db/pg/crud/join.h>
#include <udho/db/pg/crud/from.h>
#include <udho/db/pg/crud/into.h>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/constructs/alias.h>
#include <udho/db/pg/constructs/functions.h>
#include <udho/db/pg/generators/generators.h>
#include <udho/db/pg/activities/start.h>
#include <udho/db/pg/activities/after.h>
#include <udho/db/pg/activities/data.h>
#include <udho/db/pg/io/json.h>

#include <udho/view/bridges/lua.h>
#include <udho/net/context.h>
#include <udho/url/router.h>

#include <udho/session/abstract_catalogue.h>
#include <udho/session/storage/fs.h>
#include <udho/session/catalogue.h>

using session_catalogue = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;

using namespace udho::db;
using namespace ozo::literals;
using namespace boost::hana::literals;

// { relations

namespace students{

PG_ELEMENT(id,       std::int64_t);
PG_ELEMENT(name,     std::string);
PG_ELEMENT(project,  std::int64_t);
PG_ELEMENT(marks,    std::int64_t);
PG_ELEMENT(projects_associated, std::int64_t);
PG_ELEMENT(articles_published, std::int64_t);

struct table: pg::relation<table, id, name, project, marks>{
    PG_NAME(students)
    using readonly = pg::readonly<id>;
};

}

namespace articles{

PG_ELEMENT(id,      std::int64_t);
PG_ELEMENT(title,   std::string);
PG_ELEMENT(author,  std::int64_t);
PG_ELEMENT(project, std::int64_t);

struct table: pg::relation<table, id, title, author, project>{
    PG_NAME(articles)
    using readonly = pg::readonly<id>;
};

}

namespace projects{

PG_ELEMENT(id,     std::int64_t);
PG_ELEMENT(title,  std::string);

struct table: pg::relation<table, id, title>{
    PG_NAME(projects)
    using readonly = pg::readonly<id>;
};

}

namespace membership{

PG_ELEMENT(id,      std::int64_t);
PG_ELEMENT(student, std::int64_t);
PG_ELEMENT(project, std::int64_t);

struct full: pg::relation<full, id, student, project>{
    PG_NAME(memberships)
    using readonly = pg::readonly<id>;
};

}

// } relations

namespace students{
    
using all = pg::from<students::table>
    ::fetch
    ::all
    ::apply;
    
using create = pg::into<students::table>
    ::insert
    ::writables
    ::apply;
    
using ordered_by_marks = pg::from<students::table>
    ::fetch
    ::all
    ::ascending<students::marks>
    ::apply;
    
using top5 = pg::from<students::table>
    ::fetch
    ::all
    ::descending<students::marks>
    ::limit<5>
    ::apply;
    
using top5_names = pg::from<students::table>
    ::fetch
    ::only<students::name>
    ::descending<students::marks>
    ::limit<5>
    ::apply;
    
using top5_short = pg::from<students::table>
    ::fetch
    ::all
    ::exclude<students::project, students::marks>
    ::descending<students::marks>
    ::limit<5>
    ::apply;
    
using top5_marks_str = pg::from<students::table>
    ::fetch
    ::all
    ::exclude<students::project, students::marks>
    ::include<students::marks::cast<pg::types::text>>
    ::descending<students::marks>
    ::limit<5>
    ::apply;
    
namespace by{

using id = pg::from<students::table>
    ::retrieve
    ::all
    ::by<students::id>
    ::apply;
    
using project = pg::from<students::table>
    ::fetch
    ::all
    ::by<students::project>
    ::apply;
    
using name = pg::from<students::table>
    ::fetch
    ::all
    ::by<students::project, students::name::like>
    ::apply;

}

using qualified = pg::from<students::table>
    ::fetch
    ::all
    ::by<students::marks::gte>
    ::descending<students::marks>
    ::apply;

using disqualified = pg::from<students::table>
    ::fetch
    ::all
    ::by<students::marks::lt>
    ::ascending<students::marks>
    ::apply;

using top5_above_cutoff = pg::from<students::table>
    ::fetch
    ::only<students::id, students::name, students::marks>
    ::by<students::marks::gte>
    ::descending<students::marks>
    ::limit<5>
    ::apply;

using detailed = pg::from<students::table>
    ::join<articles::table>::inner::on<students::id, articles::author>
    ::join<membership::full>::inner::on<students::id, membership::student>
    ::fetch
    ::only<students::id, students::name>
    ::include<membership::project::count::as<students::projects_associated>, articles::id::count::as<students::articles_published>>
    ::apply;
}


TEST_CASE("postgresql test single select statement", "[pg]"){
    boost::asio::io_context io;
    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<tabulate::Table>{});
    lua.bind(udho::view::data::type<udho::net::context<udho::view::data::bridges::lua>>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};
    resource_store.assets().base("assets");
    resource_store.lock();
    udho::view::resources::const_store<udho::view::data::bridges::lua> resource_store_proxy{resource_store};

    auto router = udho::url::router();


    udho::net::types::headers::request  request;
    udho::net::fake::context<udho::view::data::bridges::lua> fake_context_generator{request};
    udho::net::context<udho::view::data::bridges::lua> ctx = fake_context_generator.create(io, router, resource_store_proxy);

    ozo::connection_pool_config dbconfig;
    ozo::connection_info<> conn_info("dbname=postgres user=postgres");
    auto pool = ozo::connection_pool(conn_info, dbconfig);

    CHECK((pg::perform<students::all>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students"_s));
    CHECK((pg::perform<students::ordered_by_marks>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students order by students.marks asc"_s));
    CHECK((pg::perform<students::top5>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students order by students.marks desc limit $1 offset $2"_s));
    CHECK((pg::perform<students::top5_names>(io, pool, ctx)->sql().text() == "select students.name from students order by students.marks desc limit $1 offset $2"_s));
    CHECK((pg::perform<students::top5_short>(io, pool, ctx)->sql().text() == "select students.id, students.name from students order by students.marks desc limit $1 offset $2"_s));
    CHECK((pg::perform<students::top5_marks_str>(io, pool, ctx)->sql().text() == "select students.id, students.name, CAST(students.marks as text) from students order by students.marks desc limit $1 offset $2"_s));
    
    CHECK((pg::perform<students::by::id>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students where students.id = $1"_s));
    CHECK((pg::perform<students::by::project>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students where students.project = $1"_s));
    CHECK((pg::perform<students::qualified>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students where students.marks >= $1 order by students.marks desc"_s));
    CHECK((pg::perform<students::disqualified>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students where students.marks < $1 order by students.marks asc"_s));
    CHECK((pg::perform<students::by::name>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.project, students.marks from students where students.project = $1 and students.name like $2"_s));
    
    CHECK((pg::perform<students::top5_above_cutoff>(io, pool, ctx)->sql().text() == "select students.id, students.name, students.marks from students where students.marks >= $1 order by students.marks desc limit $2 offset $3"_s));

    CHECK((pg::perform<students::detailed>(io, pool, ctx)->sql().text() == "select students.id, students.name, COUNT(memberships.project) as projects_associated, COUNT(articles.id) as articles_published from students inner join articles on students.id = articles.author  inner join memberships on students.id = memberships.student"_s));
}
