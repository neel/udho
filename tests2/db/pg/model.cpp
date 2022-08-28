#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <tuple>
#include <regex>
#include <udho/db/pg/activities/basic.h>
#include <udho/db/pg/activities/activity.h>
#include <udho/db/pg/activities/after.h>
#include <udho/db/pg/activities/start.h>
#include <udho/db/pg/activities/data.h>
#include <boost/hana/string.hpp>
#include <udho/db/pg/schema/defs.h>
#include <udho/db/pg/schema/field.h>
#include <udho/db/pg/schema/relation.h>

#include <udho/db/pg/crud/join.h>
#include <udho/db/pg/crud/into.h>

#include <udho/db/pg/generators/select.h>
#include <udho/db/pg/generators/from.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace db = udho::db;
namespace pg = db::pg;

// { relations

namespace students{

    PG_ELEMENT(id,       pg::types::bigint);
    PG_ELEMENT(name,     pg::types::varchar);
    PG_ELEMENT(project,  pg::types::bigint);
    PG_ELEMENT(marks,    pg::types::real);

    struct table: pg::relation<table, id, name, project, marks>{
        PG_NAME(students)
        using readonly = pg::readonly<id>;
    };

}

namespace articles{

    PG_ELEMENT(id,        pg::types::bigint);
    PG_ELEMENT(title,     pg::types::varchar);
    PG_ELEMENT(author,    pg::types::bigint);
    PG_ELEMENT(project,   pg::types::bigint);
    PG_ELEMENT(published, pg::types::timestamp, pg::traits::not_null, pg::traits::default_<pg::constants::now>::value);
    PG_ELEMENT(content,   pg::types::text);

    struct table: pg::relation<table, id, title, author, project, published, content>{
        PG_NAME(articles)
        using readonly = pg::readonly<id>;
    };

}

namespace projects{

    PG_ELEMENT(id,     pg::types::bigint);
    PG_ELEMENT(title,  pg::types::varchar);
    PG_ELEMENT(admin,  pg::types::bigint);

    struct table: pg::relation<table, id, title, admin>{
        PG_NAME(projects)
        using readonly = pg::readonly<id>;
    };

}

namespace memberships{

    PG_ELEMENT(id,      pg::types::bigint);
    PG_ELEMENT(student, pg::types::bigint);
    PG_ELEMENT(project, pg::types::bigint);

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
#define SQL_EXPECT(x, y, T) CHECK(sql() % x.text().c_str() == y % sql()); CHECK((x.params() == T))

boost::asio::io_service io;
udho::servers::quiet::stateless::request_type req;
udho::servers::quiet::stateless::attachment_type attachment(io);
udho::contexts::stateless ctx(attachment.aux(), req, attachment);

ozo::connection_pool_config dbconfig;
ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
auto pool = ozo::connection_pool(conn_info, dbconfig);

TEST_CASE("postgresql SELECT query", "[pg]") {

    std::cout << "pg::traits::has::not_null<articles::published>::value "      << pg::traits::has::not_null<articles::published>::value      << std::endl;
    std::cout << "pg::traits::has::not_null<articles::project>::value   "      << pg::traits::has::not_null<articles::project>::value        << std::endl;
    std::cout << "pg::traits::has::default_value<articles::published>::value " << pg::traits::has::default_value<articles::published>::value << std::endl;
    std::cout << "pg::traits::has::default_value<articles::project>::value   " << pg::traits::has::default_value<articles::project>::value   << std::endl;

    using all_students = pg::from<students::table>
                           ::fetch
                           ::all
                           ::apply;
    auto all_students_collector = udho::activities::collect<all_students>(ctx);
    SQL_EXPECT_SAME(
        all_students(all_students_collector, pool, io).sql(),
        "select                \
            students.id,       \
            students.name,     \
            students.project,  \
            students.marks     \
        from students          \
        "
    );

    using search_students = pg::from<students::table>
                           ::fetch
                           ::all
                           ::by<students::name::not_like>
                           ::apply;
    auto search_students_collector = udho::activities::collect<search_students>(ctx);
    search_students act_search_students(search_students_collector, pool, io);
    act_search_students[students::name::not_like::val] = pg::oz::varchar("Neel");
    SQL_EXPECT(
        act_search_students.sql(),
        "select                \
            students.id,       \
            students.name,     \
            students.project,  \
            students.marks     \
        from students          \
        where                          \
            students.name not like $1  \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel"))
    );

    using all_students_limited = pg::from<students::table>
                                   ::fetch
                                   ::all
                                   ::limit<5>
                                   ::apply;
    auto all_students_limited_collector = udho::activities::collect<all_students_limited>(ctx);
    SQL_EXPECT(
        all_students_limited(all_students_limited_collector, pool, io).sql(),
        "select                \
            students.id,       \
            students.name,     \
            students.project,  \
            students.marks     \
        from students          \
        limit $1 offset $2     \
        ",
        (boost::hana::make_tuple(5, 0))
    );

    using all_students_top5 = pg::from<students::table>
                                ::fetch
                                ::all
                                ::descending<students::marks>
                                ::limit<5>
                                ::apply;
    auto all_students_top5_collector = udho::activities::collect<all_students_top5>(ctx);
    SQL_EXPECT(
        all_students_top5(all_students_top5_collector, pool, io).sql(),
        "select                 \
            students.id,        \
            students.name,      \
            students.project,   \
            students.marks      \
        from students           \
        order by                \
            students.marks desc \
        limit $1 offset $2      \
        ",
        (boost::hana::make_tuple(5, 0))
    );

    using one_student = pg::from<students::table>
                          ::retrieve
                          ::all
                          ::by<students::id>
                          ::apply;

    auto one_student_collector = udho::activities::collect<one_student>(ctx);
    one_student act_one_student(one_student_collector, pool, io);
    act_one_student[students::id::val] = 42;
    SQL_EXPECT(
        act_one_student.sql(),
        "select                     \
            students.id,            \
            students.name,          \
            students.project,       \
            students.marks          \
        from students               \
            where                   \
                students.id = $1    \
        ",
        (boost::hana::make_tuple(42))
    );

    using unallocated_students = pg::from<students::table>
                          ::retrieve
                          ::all
                          ::by<students::project::is_null>
                          ::apply;

    auto unallocated_students_collector = udho::activities::collect<unallocated_students>(ctx);
    unallocated_students act_unallocated_students(unallocated_students_collector, pool, io);
    SQL_EXPECT_SAME(
        act_unallocated_students.sql(),
        "select                          \
            students.id,                 \
            students.name,               \
            students.project,            \
            students.marks               \
        from students                    \
            where                        \
                students.project is null \
        "
    );

    using special_students = pg::from<students::table>
                          ::retrieve
                          ::all
                          ::by<students::project::eq_<students::table::column<students::marks>>>
                          ::apply;

    auto special_students_collector = udho::activities::collect<special_students>(ctx);
    special_students act_special_students(special_students_collector, pool, io);
    SQL_EXPECT_SAME(
        act_special_students.sql(),
        "select                                    \
            students.id,                           \
            students.name,                         \
            students.project,                      \
            students.marks                         \
        from students                              \
            where                                  \
                students.project = students.marks  \
        "
    );

    using articles_published_already = pg::from<articles::table>
                                         ::fetch
                                         ::all
                                         ::by<articles::published::lte>
                                         ::apply;

    auto articles_published_already_collector = udho::activities::collect<articles_published_already>(ctx);
    articles_published_already act_articles_published_already(articles_published_already_collector, pool, io);
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    act_articles_published_already[articles::published::lte::val] = now;
    SQL_EXPECT(
        act_articles_published_already.sql(),
        "select                             \
            articles.id,                    \
            articles.title,                 \
            articles.author,                \
            articles.project,               \
            articles.published,             \
            articles.content                \
        from articles                       \
            where                           \
                articles.published <= $1    \
        ",
        (boost::hana::make_tuple(now))
    );

    using articles_published_already_by = pg::from<articles::table>
                                            ::fetch
                                            ::all
                                            ::by<articles::published::lte, articles::author>
                                            ::apply;

    auto articles_published_already_by_collector = udho::activities::collect<articles_published_already_by>(ctx);
    articles_published_already_by act_articles_published_already_by(articles_published_already_by_collector, pool, io);
    act_articles_published_already_by[articles::published::lte::val] = now;
    act_articles_published_already_by[articles::author::val] = 42;
    SQL_EXPECT(
        act_articles_published_already_by.sql(),
        "select                             \
            articles.id,                    \
            articles.title,                 \
            articles.author,                \
            articles.project,               \
            articles.published,             \
            articles.content                \
        from articles                       \
            where                           \
                articles.published <= $1    \
                and                         \
                articles.author = $2        \
        ",
        (boost::hana::make_tuple(now, 42))
    );

    using last5_articles_published_by = pg::from<articles::table>
                                          ::fetch
                                          ::all
                                          ::by<articles::published::lte, articles::author>
                                          ::descending<articles::published>
                                          ::limit<5>
                                          ::apply;

    auto last5_articles_published_by_collector = udho::activities::collect<last5_articles_published_by>(ctx);
    last5_articles_published_by act_last5_articles_published_by(last5_articles_published_by_collector, pool, io);
    act_last5_articles_published_by[articles::published::lte::val] = now;
    act_last5_articles_published_by[articles::author::val] = 42;
    SQL_EXPECT(
        act_last5_articles_published_by.sql(),
        "select                             \
            articles.id,                    \
            articles.title,                 \
            articles.author,                \
            articles.project,               \
            articles.published,             \
            articles.content                \
        from articles                       \
            where                           \
                articles.published <= $1    \
                and                         \
                articles.author = $2        \
        order by                            \
            articles.published desc         \
        limit $3 offset $4                  \
        ",
        (boost::hana::make_tuple(now, 42, 5, 0))
    );

    using students_project_avg = pg::from<students::table>
                                   ::fetch
                                   ::only<students::project, pg::avg<students::marks>>
                                   ::group<students::project>
                                   ::apply;
    auto students_project_avg_collector = udho::activities::collect<students_project_avg>(ctx);
    SQL_EXPECT_SAME(
        students_project_avg(students_project_avg_collector, pool, io).sql(),
        "select                       \
            students.project,         \
            AVG(students.marks)       \
        from students                 \
        group by students.project     \
        "
    );

    using students_project_avg_by = pg::from<students::table>
                                      ::fetch
                                      ::only<students::project, pg::avg<students::marks>>
                                      ::by<students::project::gte>
                                      ::group<students::project>
                                      ::apply;
    auto students_project_avg_by_collector = udho::activities::collect<students_project_avg_by>(ctx);
    students_project_avg_by act_students_project_avg_by(students_project_avg_by_collector, pool, io);
    act_students_project_avg_by[students::project::gte::val] = 2;
    SQL_EXPECT(
        act_students_project_avg_by.sql(),
        "select                        \
            students.project,          \
            AVG(students.marks)        \
        from students                  \
            where                      \
                students.project >= $1 \
        group by students.project      \
        ",
        boost::hana::make_tuple(2)
    );
}

TEST_CASE("postgresql INSERT query", "[pg]") {

    using student_add = pg::into<students::table>
                          ::insert
                          ::writables
                          ::apply;
    auto student_add_collector = udho::activities::collect<student_add>(ctx);
    student_add act_student_add(student_add_collector, pool, io);
    act_student_add[students::name::val] = pg::oz::varchar("Neel Basu");
    act_student_add[students::project::val] = 5;
    act_student_add[students::marks::val] = 80;
    SQL_EXPECT(
        act_student_add.sql(),
        "insert into students       \
            (name, project, marks)  \
        values                      \
            ($1, $2, $3)            \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel Basu"), 5, 80)
    );

    using student_add_returning = pg::into<students::table>
                          ::insert
                          ::writables
                          ::returning<students::id>
                          ::apply;
    auto student_add_returning_collector = udho::activities::collect<student_add_returning>(ctx);
    student_add_returning act_student_returning_add(student_add_returning_collector, pool, io);
    act_student_returning_add[students::name::val] = pg::oz::varchar("Neel Basu");
    act_student_returning_add[students::project::val] = 5;
    act_student_returning_add[students::marks::val] = 80;
    SQL_EXPECT(
        act_student_returning_add.sql(),
        "insert into students       \
            (name, project, marks)  \
        values                      \
            ($1, $2, $3)            \
        returning id                \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel Basu"), 5, 80)
    );

    using student_add_all_returning = pg::into<students::table>
                          ::insert
                          ::all
                          ::returning<students::id, students::marks>
                          ::apply;
    auto student_add_all_returning_collector = udho::activities::collect<student_add_all_returning>(ctx);
    student_add_all_returning act_student_returning_add_all(student_add_all_returning_collector, pool, io);
    act_student_returning_add_all[students::id::val] = 1;
    act_student_returning_add_all[students::name::val] = pg::oz::varchar("Neel Basu");
    act_student_returning_add_all[students::project::val] = 5;
    act_student_returning_add_all[students::marks::val] = 80;
    SQL_EXPECT(
        act_student_returning_add_all.sql(),
        "insert into students           \
            (id, name, project, marks)  \
        values                          \
            ($1, $2, $3, $4)            \
        returning id, marks             \
        ",
        boost::hana::make_tuple(1, pg::oz::varchar("Neel Basu"), 5, 80)
    );

    using student_add_some_returning = pg::into<students::table>
                          ::insert
                          ::only<students::name, students::marks>
                          ::returning<students::id, students::project>
                          ::apply;
    auto student_add_some_returning_collector = udho::activities::collect<student_add_some_returning>(ctx);
    student_add_some_returning act_student_returning_add_some(student_add_some_returning_collector, pool, io);
    act_student_returning_add_some[students::name::val]  = "Neel Basu";
    act_student_returning_add_some[students::marks::val] = 80;
    SQL_EXPECT(
        act_student_returning_add_some.sql(),
        "insert into students   \
            (name, marks)       \
        values                  \
            ($1, $2)            \
        returning id, project   \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel Basu"), 80)
    );
    
}

TEST_CASE("postgresql UPDATE query", "[pg]") {

    using student_update = pg::into<students::table>
                          ::update
                          ::writables
                          ::by<students::id>
                          ::apply;
    auto student_update_collector = udho::activities::collect<student_update>(ctx);
    student_update act_student_update(student_update_collector, pool, io);
    act_student_update[students::id::val]      = 1;
    act_student_update[students::name::val]    = pg::oz::varchar("Sunanda Bose");
    act_student_update[students::project::val] = 4;
    act_student_update[students::marks::val]   = 90;
    SQL_EXPECT(
        act_student_update.sql(),
        "update students       \
            set name    = $1,  \
                project = $2,  \
                marks   = $3   \
            where id    = $4   \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Sunanda Bose"), 4, 90, 1)
    );

    using student_update_all = pg::into<students::table>
                          ::update
                          ::all
                          ::apply;
    auto student_update_all_collector = udho::activities::collect<student_update_all>(ctx);
    student_update_all act_student_update_all(student_update_all_collector, pool, io);
    act_student_update_all[students::id::val]      = 1;
    act_student_update_all[students::name::val]    = pg::oz::varchar("Sunanda Bose");
    act_student_update_all[students::project::val] = 4;
    act_student_update_all[students::marks::val]   = 90;
    SQL_EXPECT(
        act_student_update_all.sql(),
        "update students       \
            set id      = $1,  \
                name    = $2,  \
                project = $3,  \
                marks   = $4   \
        ",
        boost::hana::make_tuple(1, pg::oz::varchar("Sunanda Bose"), 4, 90)
    );
    
}
