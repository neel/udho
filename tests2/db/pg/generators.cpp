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
#include <udho/db/pg/generators/select.h>
#include <udho/db/pg/generators/keys.h>
#include <udho/db/pg/generators/values.h>
#include <udho/db/pg/generators/set.h>
#include <udho/db/pg/generators/where.h>
#include <udho/db/pg/generators/returning.h>

using namespace udho::db;
using namespace ozo::literals;
using namespace boost::hana::literals;

namespace students{

PG_ELEMENT(id,    std::int64_t);
PG_ELEMENT(name,  std::string);
PG_ELEMENT(grade, std::int64_t);
PG_ELEMENT(marks, std::int64_t);

struct table: pg::relation<table, id, name, grade, marks>{
    PG_NAME("students")
};

}

TEST_CASE("postgresql query generators", "[pg]") {
    SECTION("postgres_generators_select"){
        students::table::schema student;
        
        pg::generators::select<students::table::schema> select(student);
        
        CHECK(std::string(select.all().text().c_str()) == "select \"students\".id, \"students\".name, \"students\".grade, \"students\".marks");
        CHECK(std::string(select.all("s"_SQL).text().c_str()) == "select s.id, s.name, s.grade, s.marks");
        
        CHECK(std::string(select.only<students::id, students::name, students::marks>().text().c_str()) == "select \"students\".id, \"students\".name, \"students\".marks");
        CHECK(std::string(select.only<students::id, students::name, students::marks>("s"_SQL).text().c_str()) == "select s.id, s.name, s.marks");
        
        CHECK(std::string(select.except<students::grade>().text().c_str()) == "select \"students\".id, \"students\".name, \"students\".marks");
        CHECK(std::string(select.except<students::grade>("s"_SQL).text().c_str()) == "select s.id, s.name, s.marks");
    }

    SECTION("postgres_generators_keys"){
        students::table::schema student;
        
        pg::generators::keys<students::table::schema> keys(student);
        
        CHECK(std::string(keys.all().text().c_str()) == "(id, name, grade, marks)");
        
        CHECK(std::string(keys.only<students::id, students::name, students::marks>().text().c_str()) == "(id, name, marks)");
        
        CHECK(std::string(keys.except<students::grade>().text().c_str()) == "(id, name, marks)");
    }

    SECTION("postgres_generators_values"){
        students::table::schema student;
        
        student[students::id::val] = 42;
        student[students::name::val] = std::string("Neel Basu");
        student[students::grade::val] = 1;
        student[students::marks::val] = 100;
        
        pg::generators::values<students::table::schema> values(student);
        
        CHECK(std::string(values.all().text().c_str()) == "values ($1, $2, $3, $4)");
        CHECK((values.all().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)));
        
        CHECK(std::string(values.only<students::id, students::name, students::marks>().text().c_str()) == "values ($1, $2, $3)");
        CHECK((values.only<students::id, students::name, students::marks>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
        
        CHECK(std::string(values.except<students::grade>().text().c_str()) == "values ($1, $2, $3)");
        CHECK((values.except<students::grade>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
    }

    SECTION("postgres_generators_set"){
        students::table::schema student;
        
        student[students::id::val] = 42;
        student[students::name::val] = std::string("Neel Basu");
        student[students::grade::val] = 1;
        student[students::marks::val] = 100;
        
        pg::generators::set<students::table::schema> set(student);
        
        CHECK(std::string(set.all().text().c_str()) == "set id = $1, name = $2, grade = $3, marks = $4");
        CHECK((set.all().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)));
        
        CHECK(std::string(set.only<students::id, students::name, students::marks>().text().c_str()) == "set id = $1, name = $2, marks = $3");
        CHECK((set.only<students::id, students::name, students::marks>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
        
        CHECK(std::string(set.except<students::grade>().text().c_str()) == "set id = $1, name = $2, marks = $3");
        CHECK((set.except<students::grade>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
    }

    SECTION("postgres_generators_where"){
        students::table::schema student;
        
        student[students::id::val] = 42;
        student[students::name::val] = std::string("Neel Basu");
        student[students::grade::val] = 1;
        student[students::marks::val] = 100;
        
        pg::generators::where<students::table::schema> where(student);

        CHECK(std::string(where.all().text().c_str()) == "where \"students\".id = $1 and \"students\".name = $2 and \"students\".grade = $3 and \"students\".marks = $4");
        CHECK((where.all().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)));
        
        CHECK(std::string(where.only<students::id, students::name, students::marks>().text().c_str()) == "where \"students\".id = $1 and \"students\".name = $2 and \"students\".marks = $3");
        CHECK((where.only<students::id, students::name, students::marks>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
        
        CHECK(std::string(where.except<students::grade>().text().c_str()) == "where \"students\".id = $1 and \"students\".name = $2 and \"students\".marks = $3");
        CHECK((where.except<students::grade>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
    }

    SECTION("postgres_generators_returning"){
        students::table::schema student;

        pg::generators::returning<students::table::schema> returning;

        CHECK(std::string(returning.all().text().c_str()) == "returning id, name, grade, marks");
        
        CHECK(std::string(returning.only<students::id, students::name, students::marks>().text().c_str()) == "returning id, name, marks");
        
        CHECK(std::string(returning.except<students::grade>().text().c_str()) == "returning id, name, marks");
    }
}