#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <tuple>
#include <udho/db/pg/activities/basic.h>
#include <udho/db/pg/activities/activity.h>
#include <udho/db/pg/activities/after.h>
#include <udho/db/pg/activities/start.h>
#include <udho/db/pg/activities/data.h>

namespace db = udho::db;
namespace pg = db::pg;

struct OZOStrQCreateNoRes: pg::activity<OZOStrQCreateNoRes>{
    using activity::activity;
    using activity::operator();
    void operator()(){
        using namespace ozo::literals;
        query(
            "CREATE TABLE IF NOT EXISTS udho_unit_test__table__students ("
                "id bigserial NOT NULL,"
                "first_name text COLLATE pg_catalog.\"default\" NOT NULL,"
                "last_name text COLLATE pg_catalog.\"default\","
                "CONSTRAINT udho_unit_test__table__students_pkey PRIMARY KEY (id)"
            ")"_SQL
        );
    }
};
struct OZOStrQTruncateNoRes: pg::activity<OZOStrQTruncateNoRes>{
    using activity::activity;
    using activity::operator();
    void operator()(){
        using namespace ozo::literals;
        query("TRUNCATE udho_unit_test__table__students RESTART IDENTITY;"_SQL);
    }
};
struct OZOStrQInsert1Res: pg::activity<OZOStrQInsert1Res, std::int64_t>{
    std::string _first;
    std::string _last;

    using activity::activity;
    using activity::operator();
    void operator()(){
        using namespace ozo::literals;
        query(
            "INSERT INTO udho_unit_test__table__students"
                "(first_name, last_name)" 
                "VALUES ("_SQL
                    + _first + ","_SQL
                    + _last +
                ") RETURNING id"_SQL
        );
    }
};
using StudentSchemaTuple = std::tuple<std::int64_t, std::string, std::string>;
struct OZOStrQSelectTupleRes: pg::activity<OZOStrQSelectTupleRes, db::results<StudentSchemaTuple>>{
    using activity::activity;
    using activity::operator();
    void operator()(){
        using namespace ozo::literals;
        query("select id, first_name, last_name from udho_unit_test__table__students"_SQL);
    }
};

struct student_t{
    std::int64_t id;
    std::string  first;
    std::string  last;
};

struct OZOStrQSelectStructRes: pg::activity<OZOStrQSelectStructRes, db::results<student_t>, StudentSchemaTuple>{
    using activity::activity;
    using activity::operator();
    void operator()(){
        using namespace ozo::literals;
        query("select id, first_name, last_name from udho_unit_test__table__students"_SQL);
    }
    student_t operator()(const StudentSchemaTuple& tuple) const {
        student_t s;
        s.id     = std::get<0>(tuple);
        s.first  = std::get<1>(tuple);
        s.last   = std::get<2>(tuple);
        return s;
    }
};

struct student_t2{
    std::int64_t id;
    std::string  first;
    std::string  last;

    student_t2() = default;
    student_t2(const student_t2&) = default;
    inline student_t2(const StudentSchemaTuple& tuple): id(std::get<0>(tuple)), first(std::get<1>(tuple)), last(std::get<2>(tuple)) {}
};

struct OZOStrQSelectStructRes2: pg::activity<OZOStrQSelectStructRes2, db::results<student_t2>, StudentSchemaTuple>{
    using activity::activity;
    using activity::operator();
    void operator()(){
        using namespace ozo::literals;
        query("select id, first_name, last_name from udho_unit_test__table__students"_SQL);
    }
};

TEST_CASE("postgresql activity with plain OZO SQL query", "[pg]") {
    CHECK(0 == 0);
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    ozo::connection_pool_config dbconfig;
    ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
    auto pool = ozo::connection_pool(conn_info, dbconfig);

    SECTION("Using udho::activities"){
        bool fetched = false;

        auto collector = udho::activities::collect<OZOStrQCreateNoRes, OZOStrQTruncateNoRes, OZOStrQInsert1Res, OZOStrQSelectTupleRes, OZOStrQSelectStructRes, OZOStrQSelectStructRes2>(ctx);
        auto create    = udho::activities::after().perform<OZOStrQCreateNoRes>(collector, pool, io);
        auto truncate  = udho::activities::after(create).perform<OZOStrQTruncateNoRes>(collector, pool, io);
        auto insert    = udho::activities::after(truncate).perform<OZOStrQInsert1Res>(collector, pool, io);
        auto fetch     = udho::activities::after(insert).perform<OZOStrQSelectTupleRes>(collector, pool, io);
        auto fetch2    = udho::activities::after(insert).perform<OZOStrQSelectStructRes>(collector, pool, io);
        auto fetch3    = udho::activities::after(insert).perform<OZOStrQSelectStructRes2>(collector, pool, io);
        udho::activities::after(fetch, fetch2, fetch3).finish(collector, [ctx, &fetched](const udho::activities::accessor<OZOStrQInsert1Res, OZOStrQSelectTupleRes, OZOStrQSelectStructRes, OZOStrQSelectStructRes2>& d){
            auto student_id  = d.success<OZOStrQInsert1Res>();
            auto students    = d.success<OZOStrQSelectTupleRes>();
            auto students_s  = d.success<OZOStrQSelectStructRes>();
            auto students_s2 = d.success<OZOStrQSelectStructRes2>();
            auto student     = students.front();
            auto student_s   = students_s.front();
            auto student_s2  = students_s2.front();

            CHECK(students.count()       == 1);
            CHECK(*student_id            == 1);
            CHECK(std::get<0>(student)   == 1);
            CHECK(std::get<1>(student)   == "Sunanda");
            CHECK(std::get<2>(student)   == "Bose");
            CHECK(student_s.id           == 1);
            CHECK(student_s.first        == "Sunanda");
            CHECK(student_s.last         == "Bose");
            CHECK(student_s2.id          == 1);
            CHECK(student_s2.first       == "Sunanda");
            CHECK(student_s2.last        == "Bose");

            fetched = true;
        });

        insert->_first = "Sunanda";
        insert->_last  = "Bose";

        create();

        io.run();

        REQUIRE(fetched);
        CHECK(fetch.completed());
        CHECK(fetch.okay());
        CHECK(!fetch.failed());
        CHECK(!fetch.canceled());
    }

    SECTION("Using db::pg::activities"){
        bool fetched = false;

        auto start     = pg::start<OZOStrQCreateNoRes, OZOStrQTruncateNoRes, OZOStrQInsert1Res, OZOStrQSelectTupleRes, OZOStrQSelectStructRes, OZOStrQSelectStructRes2>::with(ctx, pool);
        auto create    = pg::after(start).perform<OZOStrQCreateNoRes>(start);
        auto truncate  = pg::after(create).perform<OZOStrQTruncateNoRes>(start);
        auto insert    = pg::after(truncate).perform<OZOStrQInsert1Res>(start);
        auto fetch     = pg::after(insert).perform<OZOStrQSelectTupleRes>(start);
        auto fetch2    = pg::after(insert).perform<OZOStrQSelectStructRes>(start);
        auto fetch3    = pg::after(insert).perform<OZOStrQSelectStructRes2>(start);
        pg::after(fetch, fetch2, fetch3).finish(start, [ctx, &fetched](const pg::data<OZOStrQInsert1Res, OZOStrQSelectTupleRes, OZOStrQSelectStructRes, OZOStrQSelectStructRes2>& d){
            auto student_id  = d.success<OZOStrQInsert1Res>();
            auto students    = d.success<OZOStrQSelectTupleRes>();
            auto students_s  = d.success<OZOStrQSelectStructRes>();
            auto students_s2 = d.success<OZOStrQSelectStructRes2>();
            auto student     = students.front();
            auto student_s   = students_s.front();
            auto student_s2  = students_s2.front();

            CHECK(students.count()       == 1);
            CHECK(*student_id            == 1);
            CHECK(std::get<0>(student)   == 1);
            CHECK(std::get<1>(student)   == "Neel");
            CHECK(std::get<2>(student)   == "Basu");
            CHECK(student_s.id           == 1);
            CHECK(student_s.first        == "Neel");
            CHECK(student_s.last         == "Basu");
            CHECK(student_s2.id          == 1);
            CHECK(student_s2.first       == "Neel");
            CHECK(student_s2.last        == "Basu");

            fetched = true;
        });

        insert->_first = "Neel";
        insert->_last  = "Basu";

        start();

        io.run();

        REQUIRE(fetched);
        CHECK(fetch.completed());
        CHECK(fetch.okay());
        CHECK(!fetch.failed());
        CHECK(!fetch.canceled());
    }
}