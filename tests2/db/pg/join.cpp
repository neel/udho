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

#include <udho/db/pg/generators/select.h>
#include <udho/db/pg/generators/from.h>

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
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    ozo::connection_pool_config dbconfig;
    ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
    auto pool = ozo::connection_pool(conn_info, dbconfig);

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
    // using join1 = pg::attached<students::table>
    //     ::join<memberships::table>::inner::on<students::id, memberships::student>
    //     ::join<articles::table>::inner::on<students::id, articles::author>;

    using simple_join_1_t = pg::basic_join_on<
        pg::join_types::inner,
        students::table,
        articles::table,
        students::id,
        articles::author,
        void
    >::fetch::all::apply;
    auto simple_join_1_t_collector = udho::activities::collect<simple_join_1_t>(ctx);
    CHECK(std::string(simple_join_1_t(simple_join_1_t_collector, pool, io).sql().text().c_str()) == "select students.id, students.name, students.project, students.marks, articles.id, articles.title, articles.author, articles.project from students inner join articles on students.id = articles.author ");

    using simple_join_2_t = pg::basic_join_on<
        pg::join_types::inner,
        students::table,
        articles::table,
        students::id,
        articles::author,
        pg::basic_join_on<
            pg::join_types::inner,
            articles::table,
            projects::table,
            articles::project,
            projects::id,
            void
        >::type
    >::fetch::all::apply;
    auto simple_join_2_t_collector = udho::activities::collect<simple_join_2_t>(ctx);
    CHECK(std::string(simple_join_2_t(simple_join_2_t_collector, pool, io).sql().text().c_str()) == "select articles.id, articles.title, articles.author, articles.project, projects.id, projects.title, articles.id, articles.title, articles.author, articles.project from articles inner join projects on articles.project = projects.id  inner join articles on students.id = articles.author");

    
}