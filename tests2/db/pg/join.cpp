#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <tuple>
#include <boost/hana/string.hpp>
#include <udho/db/pg/schema/defs.h>
#include <udho/db/pg/schema/field.h>
#include <udho/db/pg/schema/relation.h>
#include <udho/db/pg/crud/join.h>

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

namespace memberships{

PG_ELEMENT(id,      std::int64_t);
PG_ELEMENT(student, std::int64_t);
PG_ELEMENT(project, std::int64_t);

struct table: pg::relation<table, id, student, project>{
    PG_NAME(memberships)
    using readonly = pg::readonly<id>;
};

}

// } relations

TEST_CASE("postgresql crud join", "[pg]"){
    CHECK(0 == 0);
    /**
     * udho::db::pg::basic_join_on<
     *      udho::db::pg::join_types::inner, 
     *      students::table, 
     *      articles::table, 
     *      students::id_<long int>, 
     *      articles::author_<long int>, 
     *      udho::db::pg::join_clause<
     *          udho::db::pg::joined<
     *              udho::db::pg::join_types::inner, 
     *              students::table, 
     *              memberships::table, 
     *              students::id_<long int>, 
     *              memberships::student_<long int> 
     *          >, 
     *          void
     *      > 
     * >
     * 
     */
    using join1 = pg::attached<students::table>
        ::join<memberships::table>::inner::on<students::id, memberships::student>
        ::join<articles::table>::inner::on<students::id, articles::author>;
}