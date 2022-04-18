#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <catch2/catch.hpp>
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
#include <udho/db/pg/crud/join.h>

#include <udho/db/pg/generators/select.h>
#include <udho/db/pg/generators/from.h>



#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/trim.hpp>

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
PG_ELEMENT(admin,  std::int64_t);

struct table: pg::relation<table, id, title, admin>{
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

struct sql{
    std::string _query;

    sql() = default;
    explicit sql(const char* q): _query(q) {}
    sql(const sql&) = default;

    std::string query() const { 
        std::string q = boost::algorithm::trim_copy(_query);
        return std::regex_replace(q, std::regex("\\s+"), " ");
    }

    friend bool operator==(const sql& lhs, const sql& rhs);
    friend std::ostream& operator<<(std::ostream& stream, const sql& s);
    friend sql operator%(const sql&, const char* str);
    friend sql operator%(const char* str, const sql&);
};

bool operator==(const sql& lhs, const sql& rhs){
    return lhs.query() == rhs.query();
}

sql operator%(const sql&, const char* str){
    return sql(str);
}

sql operator%(const char* str, const sql&){
    return sql(str);
}

std::ostream& operator<<(std::ostream& stream, const sql& s){
    stream << s.query();
    return stream;
}

#define SQL_EXPECT_SAME(x, y) CHECK(sql() % x.text().c_str() == y % sql())

TEST_CASE("postgresql crud join", "[pg]"){
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    ozo::connection_pool_config dbconfig;
    ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
    auto pool = ozo::connection_pool(conn_info, dbconfig);

    using basic_simple_join_1_t = pg::basic_join_on<
        pg::join_types::inner,
        articles::table,
        students::table,        // Join 1
        articles::author,       // From
        students::id,
        void
    >;
    CHECK(std::is_same<basic_simple_join_1_t::data_of<articles::title>, articles::table::column<articles::title>>::value);
    CHECK(std::is_same<basic_simple_join_1_t::data_of<students::name>,  students::table::column<students::name>>::value);
    CHECK(std::is_same<basic_simple_join_1_t::relation_of<articles::title>, articles::table>::value);
    CHECK(std::is_same<basic_simple_join_1_t::relation_of<students::name>,  students::table>::value);
    CHECK(std::is_same<basic_simple_join_1_t::source, articles::table>::value);
    using simple_join_1_t = basic_simple_join_1_t::fetch::all::apply;
    auto simple_join_1_t_collector = udho::activities::collect<simple_join_1_t>(ctx);
    SQL_EXPECT_SAME(
        simple_join_1_t(simple_join_1_t_collector, pool, io).sql(), 
        "select                                 \
            articles.id,                        \
            articles.title,                     \
            articles.author,                    \
            articles.project,                   \
            students.id,                        \
            students.name,                      \
            students.project,                   \
            students.marks                      \
        from articles                           \
        inner join students                     \
            on articles.author = students.id    \
        "
    );
    CHECK(
        std::is_same<
            basic_simple_join_1_t, 
            pg::basic_join<articles::table, students::table>
                ::inner::on<articles::author, students::id>
        >::value
    );

    CHECK(
        std::is_same<
            basic_simple_join_1_t, 
            pg::attached<articles::table>
                ::join<students::table>
                    ::inner::on<articles::author, students::id>
        >::value
    );

    // FROM articles <> (projects, students)
    // articles.project <> projects.id 
    // articles.author  <> students.id
    using basic_simple_join_2_t = pg::basic_join_on<
        pg::join_types::inner,
        articles::table, 
        students::table,                // Join 2
        articles::author,
        students::id,
        pg::basic_join_on<
            pg::join_types::inner,
            articles::table,
            projects::table,            // Join 1
            articles::project,          // From
            projects::id,
            void
        >::type
    >;
    CHECK(std::is_same<basic_simple_join_2_t::data_of<articles::title>, articles::table::column<articles::title>>::value);
    CHECK(std::is_same<basic_simple_join_2_t::data_of<students::name>,  students::table::column<students::name>>::value);
    CHECK(std::is_same<basic_simple_join_2_t::data_of<projects::title>, projects::table::column<projects::title>>::value);
    CHECK(std::is_same<basic_simple_join_2_t::relation_of<articles::title>, articles::table>::value);
    CHECK(std::is_same<basic_simple_join_2_t::relation_of<students::name>,  students::table>::value);
    CHECK(std::is_same<basic_simple_join_2_t::relation_of<projects::title>, projects::table>::value);
    CHECK(std::is_same<basic_simple_join_2_t::source, articles::table>::value);
    using simple_join_2_t = basic_simple_join_2_t::fetch::all::apply;
    auto simple_join_2_t_collector = udho::activities::collect<simple_join_2_t>(ctx);
    SQL_EXPECT_SAME(
        simple_join_2_t(simple_join_2_t_collector, pool, io).sql(), 
        "select                                   \
            articles.id,                          \
            articles.title,                       \
            articles.author,                      \
            articles.project,                     \
            projects.id,                          \
            projects.title,                       \
            projects.admin,                       \
            students.id,                          \
            students.name,                        \
            students.project,                     \
            students.marks                        \
        from articles                             \
        inner join projects                       \
            on articles.project = projects.id     \
        inner join students                       \
            on articles.author = students.id      \
        "
    );

    CHECK(
        std::is_same<
            basic_simple_join_2_t,
            pg::basic_join<articles::table, projects::table>            // FROM table (0), JOIN table (1)
                ::inner::on<articles::project, projects::id>            // lhs (0), rhs (1)
            ::join<students::table>                                     // JOIN table (2) 
                ::inner::on<articles::author, students::id>             // lhs (0), rhs (2)
        >::value
    );

    CHECK(
        std::is_same<
            basic_simple_join_2_t,
            pg::attached<articles::table>                                   // FROM table (0)
                ::join<projects::table>                                     // JOIN table (1)
                    ::inner::on<articles::project, projects::id>            // lhs (0), rhs (1)
                ::join<students::table>                                     // JOIN table (2) 
                    ::inner::on<articles::author, students::id>             // lhs (0), rhs (2)
        >::value
    );

    // FROM articles <> projects <> students
    // articles.project <> projects.id 
    // projects.admin   <> students.id
    using basic_simple_join_2a_t = pg::basic_join_on<
        pg::join_types::inner,
        projects::table, 
        students::table,                // Join 2
        projects::admin,
        students::id,
        pg::basic_join_on<
            pg::join_types::inner,
            articles::table,
            projects::table,            // Join 1
            articles::project,          // From
            projects::id,
            void
        >::type
    >;
    CHECK(std::is_same<basic_simple_join_2a_t::data_of<articles::title>, articles::table::column<articles::title>>::value);
    CHECK(std::is_same<basic_simple_join_2a_t::data_of<students::name>,  students::table::column<students::name>>::value);
    CHECK(std::is_same<basic_simple_join_2a_t::data_of<projects::title>, projects::table::column<projects::title>>::value);
    CHECK(std::is_same<basic_simple_join_2a_t::relation_of<articles::title>, articles::table>::value);
    CHECK(std::is_same<basic_simple_join_2a_t::relation_of<students::name>,  students::table>::value);
    CHECK(std::is_same<basic_simple_join_2a_t::relation_of<projects::title>, projects::table>::value);
    CHECK(std::is_same<basic_simple_join_2a_t::source, articles::table>::value);
    using simple_join_2a_t = basic_simple_join_2a_t::fetch::all::apply;
    auto simple_join_2a_t_collector = udho::activities::collect<simple_join_2a_t>(ctx);
    SQL_EXPECT_SAME(
        simple_join_2a_t(simple_join_2a_t_collector, pool, io).sql(), 
        "select                                   \
            articles.id,                          \
            articles.title,                       \
            articles.author,                      \
            articles.project,                     \
            projects.id,                          \
            projects.title,                       \
            projects.admin,                       \
            students.id,                          \
            students.name,                        \
            students.project,                     \
            students.marks                        \
        from articles                             \
        inner join projects                       \
            on articles.project = projects.id     \
        inner join students                       \
            on projects.admin = students.id       \
        "
    );

    CHECK(
        std::is_same<
            basic_simple_join_2a_t,
            pg::basic_join<articles::table, projects::table>            // FROM table (0), JOIN table (1)
                ::inner::on<articles::project, projects::id>            // lhs (0), rhs (1)
            ::join<students::table, projects::table>                    // JOIN table (2), JOIN table (1)
                ::inner::on<projects::admin, students::id>              // lhs (1), rhs (2)
        >::value
    );

    CHECK(
        std::is_same<
            basic_simple_join_2a_t,
            pg::attached<articles::table>                               // FROM table (0)
                ::join<projects::table>                                 // JOIN table (1)
                    ::inner::on<articles::project, projects::id>        // lhs (0), rhs (1)
                ::join<students::table, projects::table>                // JOIN table (2), JOIN table (1)
                    ::inner::on<projects::admin, students::id>          // lhs (1), rhs (2)
        >::value
    );
    

    // !MALFORMED 
    // FROM articles <> projects | students <> projects
    // articles.project <> projects.id 
    // students.id      <> projects.admin
    using simple_join_2b_t = pg::basic_join_on<
        pg::join_types::inner,
        students::table, 
        projects::table,                // Join 2
        students::id,
        projects::admin,
        pg::basic_join_on<
            pg::join_types::inner,
            articles::table,
            projects::table,            // Join 1
            articles::project,          // From
            projects::id,
            void
        >::type
    >::fetch::all::apply;
    auto simple_join_2b_t_collector = udho::activities::collect<simple_join_2b_t>(ctx);
    SQL_EXPECT_SAME(
        simple_join_2b_t(simple_join_2b_t_collector, pool, io).sql(), 
        "select                                   \
            articles.id,                          \
            articles.title,                       \
            articles.author,                      \
            articles.project,                     \
            projects.id,                          \
            projects.title,                       \
            projects.admin,                       \
            projects.id,                          \
            projects.title,                       \
            projects.admin                        \
        from articles                             \
        inner join projects                       \
            on articles.project = projects.id     \
        inner join projects                       \
            on students.id = projects.admin       \
        "
    );

    using basic_simple_join_3_t = pg::basic_join_on<
        pg::join_types::inner,
        memberships::table,
        projects::table,
        memberships::project,
        projects::id,
        pg::basic_join_on<
            pg::join_types::inner,
            students::table,
            memberships::table,
            students::id,
            memberships::student,
            pg::basic_join_on<
                pg::join_types::inner,
                articles::table,
                students::table,
                articles::author,
                students::id,
                void
            >::type
        >::type
    >;
    CHECK(std::is_same<basic_simple_join_3_t::data_of<articles::title>, articles::table::column<articles::title>>::value);
    CHECK(std::is_same<basic_simple_join_3_t::data_of<students::name>,  students::table::column<students::name>>::value);
    CHECK(std::is_same<basic_simple_join_3_t::data_of<projects::title>, projects::table::column<projects::title>>::value);
    CHECK(std::is_same<basic_simple_join_3_t::data_of<memberships::id>, memberships::table::column<memberships::id>>::value);
    CHECK(std::is_same<basic_simple_join_3_t::relation_of<articles::title>, articles::table>::value);
    CHECK(std::is_same<basic_simple_join_3_t::relation_of<students::name>,  students::table>::value);
    CHECK(std::is_same<basic_simple_join_3_t::relation_of<projects::title>, projects::table>::value);
    CHECK(std::is_same<basic_simple_join_3_t::relation_of<memberships::id>, memberships::table>::value);
    CHECK(std::is_same<basic_simple_join_3_t::source, articles::table>::value);
    using simple_join_3_t = basic_simple_join_3_t::fetch::all::apply;
    auto simple_join_3_t_collector = udho::activities::collect<simple_join_3_t>(ctx);
    SQL_EXPECT_SAME(
        simple_join_3_t(simple_join_3_t_collector, pool, io).sql(), 
        "select                                        \
            articles.id,                               \
            articles.title,                            \
            articles.author,                           \
            articles.project,                          \
            students.id,                               \
            students.name,                             \
            students.project,                          \
            students.marks,                            \
            memberships.id,                            \
            memberships.student,                       \
            memberships.project,                       \
            projects.id,                               \
            projects.title,                            \
            projects.admin                             \
        from articles                                  \
        inner join students                            \
            on articles.author = students.id           \
        inner join memberships                         \
            on students.id = memberships.student       \
        inner join projects                            \
            on memberships.project = projects.id       \
        "
    );
    CHECK(
        std::is_same<
            basic_simple_join_3_t,
            pg::basic_join<articles::table, students::table>            // FROM table (0), JOIN table (1)
                ::inner::on<articles::author, students::id>             // lhs (0), rhs (1)
            ::join<memberships::table, students::table>                 // JOIN table (2), JOIN table (1)
                ::inner::on<students::id, memberships::student>         // lhs (1), rhs (2)
            ::join<projects::table, memberships::table>                 // JOIN table (3), JOIN table (2)
                ::inner::on<memberships::project, projects::id>         // lhs (2), rhs (3)
        >::value
    );

    CHECK(
        std::is_same<
            basic_simple_join_3_t,
            pg::attached<articles::table>                               // FROM table (0)
                ::join<students::table>                                 // JOIN table (1)
                    ::inner::on<articles::author, students::id>         // lhs (0), rhs (1)
                ::join<memberships::table, students::table>             // JOIN table (2), JOIN table (1)
                    ::inner::on<students::id, memberships::student>     // lhs (1), rhs (2)
                ::join<projects::table, memberships::table>             // JOIN table (3), JOIN table (2)
                    ::inner::on<memberships::project, projects::id>     // lhs (2), rhs (3)
        >::value
    );
}
