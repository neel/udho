#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <tuple>
#include <regex>

#include "schema.h"
#include "common.h"

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

#include <udho/db/pg/decorators/definitions.h>
#include <udho/db/pg/crud/create.h>

namespace db = udho::db;
namespace pg = db::pg;

boost::asio::io_service io;
udho::servers::quiet::stateless::request_type req;
udho::servers::quiet::stateless::attachment_type attachment(io);
udho::contexts::stateless ctx(attachment.aux(), req, attachment);

ozo::connection_pool_config dbconfig;
ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
auto pool = ozo::connection_pool(conn_info, dbconfig);

TEST_CASE("postgresql CREATE query", "[pg]") {
    using create_articles_after_skip = pg::ddl<articles::table>
                                         ::create
                                         ::if_exists
                                         ::skip
                                         ::apply;

    auto create_articles_after_skip_collector = udho::activities::collect<create_articles_after_skip>(ctx);
    SQL_EXPECT_SAME(
        create_articles_after_skip(create_articles_after_skip_collector, pool, io).sql(),
        "create table if not exists articles(                               \
            id bigserial primary key,                                       \
            title varchar unique,                                           \
            author bigint references students(id) on delete restrict,       \
            project bigint references projects(id) on delete restrict,      \
            published timestamp without time zone not null default now(),   \
            content text                                                    \
        )"
    );

    using create_articles_except_after_skip = pg::ddl<articles::table>
                                                ::create
                                                ::except<articles::content>
                                                ::if_exists
                                                ::skip
                                                ::apply;

    auto create_articles_except_after_skip_collector = udho::activities::collect<create_articles_except_after_skip>(ctx);
    SQL_EXPECT_SAME(
        create_articles_except_after_skip(create_articles_except_after_skip_collector, pool, io).sql(),
        "create table if not exists articles(                               \
            id bigserial primary key,                                       \
            title varchar unique,                                           \
            author bigint references students(id) on delete restrict,       \
            project bigint references projects(id) on delete restrict,      \
            published timestamp without time zone not null default now()    \
        )"
    );

    using create_articles_only_after_skip = pg::ddl<articles::table>
                                              ::create
                                              ::only<articles::id, articles::title, articles::author, articles::project, articles::published>
                                              ::if_exists
                                              ::skip
                                              ::apply;

    auto create_articles_only_after_skip_collector = udho::activities::collect<create_articles_only_after_skip>(ctx);
    SQL_EXPECT_SAME(
        create_articles_only_after_skip(create_articles_only_after_skip_collector, pool, io).sql(),
        "create table if not exists articles(                               \
            id bigserial primary key,                                       \
            title varchar unique,                                           \
            author bigint references students(id) on delete restrict,       \
            project bigint references projects(id) on delete restrict,      \
            published timestamp without time zone not null default now()    \
        )"
    );
}

TEST_CASE("postgresql DROP query", "[pg]") {
    using drop_students = pg::ddl<students::table>
                            ::drop
                            ::apply;

    auto drop_students_collector = udho::activities::collect<drop_students>(ctx);
    SQL_EXPECT_SAME(
        drop_students(drop_students_collector, pool, io).sql(),
        "drop table if exists students"
    );
}

TEST_CASE("postgresql SELECT query", "[pg]") {

    std::cout << "pg::constraints::has::not_null<articles::published>::value "      << pg::constraints::has::not_null<articles::published>::value      << std::endl;
    std::cout << "pg::constraints::has::not_null<articles::project>::value   "      << pg::constraints::has::not_null<articles::project>::value        << std::endl;
    std::cout << "pg::constraints::has::default_value<articles::published>::value " << pg::constraints::has::default_value<articles::published>::value << std::endl;
    std::cout << "pg::constraints::has::default_value<articles::project>::value   " << pg::constraints::has::default_value<articles::project>::value   << std::endl;

    using all_students = pg::from<students::table>
                           ::fetch
                           ::all
                           ::apply;
    auto all_students_collector = udho::activities::collect<all_students>(ctx);
    SQL_EXPECT_SAME(
        all_students(all_students_collector, pool, io).sql(),
        "select                  \
            students.id,         \
            students.first_name, \
            students.last_name,  \
            students.marks,      \
            students.age         \
        from students            \
        "
    );

    using all_students_name = pg::from<students::table>
                           ::fetch
                           ::all
                           ::exclude<students::first_name, students::last_name>
                           ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
                           ::apply;
    auto all_students_name_collector = udho::activities::collect<all_students_name>(ctx);
    SQL_EXPECT_SAME(
        all_students_name(all_students_name_collector, pool, io).sql(),
        "select                  \
            students.id,         \
            students.marks,      \
            students.age,        \
            concat(students.first_name,' ',students.last_name) as name  \
        from students            \
        "
    );

    using search_students = pg::from<students::table>
                           ::fetch
                           ::all
                           ::by<students::first_name::not_like>
                           ::apply;
    auto search_students_collector = udho::activities::collect<search_students>(ctx);
    search_students act_search_students(search_students_collector, pool, io);
    act_search_students[students::first_name::not_like::val] = pg::oz::varchar("Neel");
    SQL_EXPECT(
        act_search_students.sql(),
        "select                  \
            students.id,         \
            students.first_name, \
            students.last_name,  \
            students.marks,      \
            students.age         \
        from students            \
        where                          \
            students.first_name not like $1  \
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
        "select                  \
            students.id,         \
            students.first_name, \
            students.last_name,  \
            students.marks,      \
            students.age         \
        from students            \
        limit $1 offset $2       \
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
        "select                  \
            students.id,         \
            students.first_name, \
            students.last_name,  \
            students.marks,      \
            students.age         \
        from students            \
        order by                 \
            students.marks desc  \
        limit $1 offset $2       \
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
            students.first_name,    \
            students.last_name,     \
            students.marks,         \
            students.age            \
        from students               \
            where                   \
                students.id = $1    \
        ",
        (boost::hana::make_tuple(42))
    );

    using noage_students = pg::from<students::table>
                          ::retrieve
                          ::all
                          ::by<students::age::is_null>
                          ::apply;

    auto noage_students_collector = udho::activities::collect<noage_students>(ctx);
    noage_students act_noage_students(noage_students_collector, pool, io);
    SQL_EXPECT_SAME(
        act_noage_students.sql(),
        "select                          \
            students.id,                 \
            students.first_name,         \
            students.last_name,          \
            students.marks,              \
            students.age                 \
        from students                    \
            where                        \
                students.age is null     \
        "
    );

    using special_students = pg::from<students::table>
                          ::retrieve
                          ::all
                          ::by<students::age::eq_<students::table::column<students::marks>>>
                          ::apply;

    auto special_students_collector = udho::activities::collect<special_students>(ctx);
    special_students act_special_students(special_students_collector, pool, io);
    SQL_EXPECT_SAME(
        act_special_students.sql(),
        "select                                    \
            students.id,                           \
            students.first_name,                   \
            students.last_name,                    \
            students.marks,                        \
            students.age                           \
        from students                              \
            where                                  \
                students.age = students.marks      \
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
                                   ::only<students::first_name, pg::avg<students::marks>>
                                   ::group<students::first_name>
                                   ::apply;
    auto students_project_avg_collector = udho::activities::collect<students_project_avg>(ctx);
    SQL_EXPECT_SAME(
        students_project_avg(students_project_avg_collector, pool, io).sql(),
        "select                       \
            students.first_name,      \
            AVG(students.marks)       \
        from students                 \
        group by students.first_name  \
        "
    );

    using students_project_avg_by = pg::from<students::table>
                                      ::fetch
                                      ::only<students::first_name, pg::avg<students::marks>>
                                      ::by<students::marks::gte>
                                      ::group<students::first_name>
                                      ::apply;
    auto students_project_avg_by_collector = udho::activities::collect<students_project_avg_by>(ctx);
    students_project_avg_by act_students_project_avg_by(students_project_avg_by_collector, pool, io);
    act_students_project_avg_by[students::marks::gte::val] = 2;
    SQL_EXPECT(
        act_students_project_avg_by.sql(),
        "select                        \
            students.first_name,       \
            AVG(students.marks)        \
        from students                  \
            where                      \
                students.marks >= $1   \
        group by students.first_name   \
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
    act_student_add[students::first_name::val] = pg::oz::varchar("Neel");
    act_student_add[students::last_name::val] = pg::oz::varchar("Basu");
    act_student_add[students::marks::val] = 80;
    act_student_add[students::age::val] = 20;
    SQL_EXPECT(
        act_student_add.sql(),
        "insert into students       \
            (first_name, last_name, marks, age)  \
        values                      \
            ($1, $2, $3, $4)            \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel"), pg::oz::varchar("Basu"), 80, 20)
    );

    using student_add_returning = pg::into<students::table>
                          ::insert
                          ::writables
                          ::returning<students::id>
                          ::apply;
    auto student_add_returning_collector = udho::activities::collect<student_add_returning>(ctx);
    student_add_returning act_student_returning_add(student_add_returning_collector, pool, io);
    act_student_returning_add[students::first_name::val] = pg::oz::varchar("Neel");
    act_student_returning_add[students::last_name::val]  = pg::oz::varchar("Basu");
    act_student_returning_add[students::marks::val] = 80;
    act_student_returning_add[students::age::val] = 20;
    SQL_EXPECT(
        act_student_returning_add.sql(),
        "insert into students       \
            (first_name, last_name, marks, age)  \
        values                      \
            ($1, $2, $3, $4)            \
        returning id                \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel"), pg::oz::varchar("Basu"), 80, 20)
    );

    using student_add_all_returning = pg::into<students::table>
                          ::insert
                          ::all
                          ::returning<students::id, students::marks>
                          ::apply;
    auto student_add_all_returning_collector = udho::activities::collect<student_add_all_returning>(ctx);
    student_add_all_returning act_student_returning_add_all(student_add_all_returning_collector, pool, io);
    act_student_returning_add_all[students::id::val] = 1;
    act_student_returning_add_all[students::first_name::val] = pg::oz::varchar("Neel");
    act_student_returning_add_all[students::last_name::val]  = pg::oz::varchar("Basu");
    act_student_returning_add_all[students::marks::val] = 80;
    act_student_returning_add_all[students::age::val] = 20;
    SQL_EXPECT(
        act_student_returning_add_all.sql(),
        "insert into students           \
            (id, first_name, last_name, marks, age)  \
        values                          \
            ($1, $2, $3, $4, $5)            \
        returning id, marks             \
        ",
        boost::hana::make_tuple(1, pg::oz::varchar("Neel"), pg::oz::varchar("Basu"), 80, 20)
    );

    using student_add_some_returning = pg::into<students::table>
                          ::insert
                          ::only<students::first_name, students::last_name, students::marks>
                          ::returning<students::id, students::age>
                          ::apply;
    auto student_add_some_returning_collector = udho::activities::collect<student_add_some_returning>(ctx);
    student_add_some_returning act_student_returning_add_some(student_add_some_returning_collector, pool, io);
    act_student_returning_add_some[students::first_name::val] = pg::oz::varchar("Neel");
    act_student_returning_add_some[students::last_name::val]  = pg::oz::varchar("Basu");
    act_student_returning_add_some[students::marks::val] = 80;
    SQL_EXPECT(
        act_student_returning_add_some.sql(),
        "insert into students   \
            (first_name, last_name, marks)       \
        values                  \
            ($1, $2, $3)            \
        returning id, age     \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Neel"), pg::oz::varchar("Basu"), 80)
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
    act_student_update[students::id::val]          = 1;
    act_student_update[students::first_name::val]  = pg::oz::varchar("Sunanda");
    act_student_update[students::last_name::val]   = pg::oz::varchar("Bose");
    act_student_update[students::age::val]         = 4;
    act_student_update[students::marks::val]       = 90;
    SQL_EXPECT(
        act_student_update.sql(),
        "update students       \
            set first_name = $1,  \
                last_name  = $2,  \
                marks      = $3,  \
                age        = $4   \
            where id       = $5   \
        ",
        boost::hana::make_tuple(pg::oz::varchar("Sunanda"), pg::oz::varchar("Bose"), 90, 4, 1)
    );

    using student_update_all = pg::into<students::table>
                          ::update
                          ::all
                          ::apply;
    auto student_update_all_collector = udho::activities::collect<student_update_all>(ctx);
    student_update_all act_student_update_all(student_update_all_collector, pool, io);
    act_student_update_all[students::id::val]          = 1;
    act_student_update_all[students::first_name::val]  = pg::oz::varchar("Sunanda");
    act_student_update_all[students::last_name::val]   = pg::oz::varchar("Bose");
    act_student_update_all[students::age::val]         = 4;
    act_student_update_all[students::marks::val]       = 90;
    SQL_EXPECT(
        act_student_update_all.sql(),
        "update students       \
            set id         = $1,  \
                first_name = $2,  \
                last_name  = $3,  \
                marks      = $4,  \
                age        = $5   \
        ",
        boost::hana::make_tuple(1, pg::oz::varchar("Sunanda"), pg::oz::varchar("Bose"), 90, 4)
    );
    
}

TEST_CASE("postgresql REMOVE query", "[pg]") {

    using student_remove = pg::from<students::table>
                          ::remove
                          ::by<students::id>
                          ::apply;
    auto student_remove_collector = udho::activities::collect<student_remove>(ctx);
    student_remove act_student_remove(student_remove_collector, pool, io);
    act_student_remove[students::id::val] = 1;
    SQL_EXPECT(
        act_student_remove.sql(),
        "delete from students where id = $1",
        boost::hana::make_tuple(1)
    );

}
