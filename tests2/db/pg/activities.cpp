#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <tuple>
#include <udho/db/pg/activities/basic.h>
#include <udho/db/pg/activities/after.h>

namespace db = udho::db;
namespace pg = db::pg;


struct OZOStrQCreateNoRes: pg::basic_activity<OZOStrQCreateNoRes>{
    using basic_activity::basic_activity;

    void operator()(){
        using namespace ozo::literals;
        query(
            "CREATE TABLE IF NOT EXISTS students ("
                "id bigserial NOT NULL,"
                "first_name text COLLATE pg_catalog.\"default\" NOT NULL,"
                "last_name text COLLATE pg_catalog.\"default\","
                "CONSTRAINT students_pkey PRIMARY KEY (id)"
            ")"_SQL
        );
    }
    void operator()(const db::none& results){
        basic_activity::success(results);
        basic_activity::clear();
    }
};
struct OZOStrQTruncateNoRes: pg::basic_activity<OZOStrQTruncateNoRes>{
    using basic_activity::basic_activity;

    void operator()(){
        using namespace ozo::literals;
        query("TRUNCATE students RESTART IDENTITY;"_SQL);
    }
    void operator()(const db::none& results){
        basic_activity::success(results);
        basic_activity::clear();
    }
};
struct OZOStrQInsert1Res: pg::basic_activity<OZOStrQInsert1Res, std::int64_t>{
    std::string _first;
    std::string _last;

    using basic_activity::basic_activity;

    void operator()(){
        using namespace ozo::literals;
        query(
            "INSERT INTO students"
                "(first_name, last_name)" 
                "VALUES ("_SQL
                    + _first + ","_SQL
                    + _last +
                ") RETURNING id"_SQL
        );
    }
    void operator()(const db::result<std::int64_t>& result){
        basic_activity::success(result);
        basic_activity::clear();
    }
};
using StudentSchemaTuple = std::tuple<std::int64_t, std::string, std::string>;
struct OZOStrQSelectTupleRes: pg::basic_activity<OZOStrQSelectTupleRes, db::results<StudentSchemaTuple>>{
    using basic_activity::basic_activity;

    void operator()(){
        using namespace ozo::literals;
        query("select id, first_name, last_name from students"_SQL);
    }
    void operator()(const db::results<StudentSchemaTuple>& results){
        basic_activity::success(results);
        basic_activity::clear();
    }
};

TEST_CASE("postgresql activities", "[pg]") {
    CHECK(0 == 0);
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    ozo::connection_pool_config dbconfig;
    ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
    auto pool = ozo::connection_pool(conn_info, dbconfig);

    SECTION("Using udho::activities for pg activities"){
        bool fetched = false;

        auto collector = udho::activities::collect<OZOStrQCreateNoRes, OZOStrQTruncateNoRes, OZOStrQInsert1Res, OZOStrQSelectTupleRes>(ctx);
        auto create    = udho::activities::after().perform<OZOStrQCreateNoRes>(collector, pool, io);
        auto truncate  = udho::activities::after(create).perform<OZOStrQTruncateNoRes>(collector, pool, io);
        auto insert    = udho::activities::after(truncate).perform<OZOStrQInsert1Res>(collector, pool, io);
        auto fetch     = udho::activities::after(insert).perform<OZOStrQSelectTupleRes>(collector, pool, io);
        udho::activities::after(fetch).finish(collector, [ctx, &fetched](const udho::activities::accessor<OZOStrQInsert1Res, OZOStrQSelectTupleRes>& d){
            auto student_id = d.success<OZOStrQInsert1Res>();
            auto students   = d.success<OZOStrQSelectTupleRes>();
            auto student    = students.front();

            CHECK(students.count() == 1);
            CHECK(*student_id == 1);
            CHECK(std::get<0>(student) == 1);
            CHECK(std::get<1>(student) == "Sunanda");
            CHECK(std::get<2>(student) == "Bose");

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
}