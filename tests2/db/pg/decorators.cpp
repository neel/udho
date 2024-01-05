#define CATCH_CONFIG_MAIN
#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
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
#include <udho/db/pg/schema/field.h>
#include <udho/db/pg/schema/relation.h>
#include <udho/db/pg/decorators/keys.h>
#include <udho/db/pg/decorators/values.h>
#include <udho/db/pg/decorators/assignments.h>
#include <udho/db/pg/decorators/conditions.h>

using namespace udho::db;
using namespace ozo::literals;
using namespace boost::hana::literals;

namespace students{

PG_ELEMENT(id,    std::int64_t);
PG_ELEMENT(name,  std::string);
PG_ELEMENT(grade, std::int64_t);
PG_ELEMENT(marks, std::int64_t);

struct table: pg::relation<table, id, name, grade, marks>{
    static auto name(){
        return "students"_SQL;
    }
};

}

TEST_CASE("postgresql schema decorators", "[pg]"){

    SECTION("postgres decorators keys schema"){
        pg::schema<students::id, students::name, students::grade, students::marks> student;

        CHECK(bool(student.decorate(pg::decorators::keys::unqualified{}).text() == "id, name, grade, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::prefixed("s"_SQL)).text() == "s.id, s.name, s.grade, s.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys{}).text() == "id, name, grade, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::only<students::id, students::name, students::marks>::unqualified{}).text() == "id, name, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::only<students::id, students::name, students::marks>::prefixed("s"_SQL)).text() == "s.id, s.name, s.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::only<students::id, students::name, students::marks>{}).text() == "id, name, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::except<students::grade>::unqualified{}).text() == "id, name, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::except<students::grade>::prefixed("s"_SQL)).text() == "s.id, s.name, s.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::except<students::grade>{}).text() == "id, name, marks"_s));
    }

    SECTION("postgres decorators keys"){
        students::table::schema student;

        CHECK(bool(student.decorate(pg::decorators::keys::unqualified{}).text() == "id, name, grade, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::prefixed("s"_SQL)).text() == "s.id, s.name, s.grade, s.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys{}).text() == "students.id, students.name, students.grade, students.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::only<students::id, students::name, students::marks>::unqualified{}).text() == "id, name, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::only<students::id, students::name, students::marks>::prefixed("s"_SQL)).text() == "s.id, s.name, s.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::only<students::id, students::name, students::marks>{}).text() == "students.id, students.name, students.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::except<students::grade>::unqualified{}).text() == "id, name, marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::except<students::grade>::prefixed("s"_SQL)).text() == "s.id, s.name, s.marks"_s));
        CHECK(bool(student.decorate(pg::decorators::keys::except<students::grade>{}).text() == "students.id, students.name, students.marks"_s));
    }

    SECTION("postgres decorators values"){
        students::table::schema student;
        
        student[students::id::val] = 42;
        student[students::name::val] = std::string("Neel Basu");
        student[students::grade::val] = 1;
        student[students::marks::val] = 100;
        
        CHECK(bool(student.decorate(pg::decorators::values{}).text() == "$1, $2, $3, $4"_s));
        std::cout << student.decorate(pg::decorators::values{}).params()[0_c].value() << std::endl;
        CHECK(bool((student.decorate(pg::decorators::values{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool(student.decorate(pg::decorators::values::only<students::id, students::name, students::marks>{}).text() == "$1, $2, $3"_s));
        CHECK(bool((student.decorate(pg::decorators::values::only<students::id, students::name, students::marks>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool(student.decorate(pg::decorators::values::except<students::grade>{}).text() == "$1, $2, $3"_s));
        CHECK(bool((student.decorate(pg::decorators::values::except<students::grade>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
    }

    SECTION("postgres decorators assignments"){
        students::table::schema student;
        
        student[students::id::val] = 42;
        student[students::name::val] = std::string("Neel Basu");
        student[students::grade::val] = 1;
        student[students::marks::val] = 100;
        
        CHECK(bool(student.decorate(pg::decorators::assignments::unqualified{}).text() == "id = $1, name = $2, grade = $3, marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::prefixed("s"_SQL)).text() == "s.id = $1, s.name = $2, s.grade = $3, s.marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments{}).text() == "students.id = $1, students.name = $2, students.grade = $3, students.marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>::unqualified{}).text() == "id = $1, name = $2, marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>::prefixed("s"_SQL)).text() == "s.id = $1, s.name = $2, s.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>{}).text() == "students.id = $1, students.name = $2, students.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::except<students::grade>::unqualified{}).text() == "id = $1, name = $2, marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::except<students::grade>::prefixed("s"_SQL)).text() == "s.id = $1, s.name = $2, s.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::assignments::except<students::grade>{}).text() == "students.id = $1, students.name = $2, students.marks = $3"_s));
        
        CHECK(bool((student.decorate(pg::decorators::assignments::unqualified{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>::unqualified{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::only<students::id, students::name, students::marks>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::except<students::grade>::unqualified{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::except<students::grade>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::assignments::except<students::grade>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
    }

    SECTION("postgres decorators conditions"){
        students::table::schema student;
        
        student[students::id::val] = 42;
        student[students::name::val] = std::string("Neel Basu");
        student[students::grade::val] = 1;
        student[students::marks::val] = 100;

        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() != "id = $1 and name = $2 and grade = $3 and marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).text() == "s.id = $1 and s.name = $2 and s.grade = $3 and s.marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() == "students.id = $1 and students.name = $2 and students.grade = $3 and students.marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id, students::name, students::marks>{}).text() != "id = $1 and name = $2 and marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id, students::name, students::marks>::prefixed("s"_SQL)).text() == "s.id = $1 and s.name = $2 and s.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id, students::name, students::marks>{}).text() == "students.id = $1 and students.name = $2 and students.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade>{}).text() != "id = $1 and name = $2 and marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade>::prefixed("s"_SQL)).text() == "s.id = $1 and s.name = $2 and s.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade>{}).text() == "students.id = $1 and students.name = $2 and students.marks = $3"_s));
        
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id, students::name, students::marks>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id, students::name, students::marks>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id, students::name, students::marks>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
    }

    SECTION("postgres decorators conditions eq neq"){
        pg::schema<
            students::table::column<students::id::eq>, 
            students::table::column<students::name::neq>, 
            students::table::column<students::grade::eq>, 
            students::table::column<students::marks::eq>
        > student;
        
        student[students::id::eq::val] = 42;
        student[students::name::neq::val] = std::string("Neel Basu");
        student[students::grade::eq::val] = 1;
        student[students::marks::eq::val] = 100;

        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() != "id = $1 and name != $2 and grade = $3 and marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).text() == "s.id = $1 and s.name != $2 and s.grade = $3 and s.marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() == "students.id = $1 and students.name != $2 and students.grade = $3 and students.marks = $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::eq, students::name::neq, students::marks::eq>{}).text() != "id = $1 and name != $2 and marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::eq, students::name::neq, students::marks::eq>::prefixed("s"_SQL)).text() == "s.id = $1 and s.name != $2 and s.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::eq, students::name::neq, students::marks::eq>{}).text() == "students.id = $1 and students.name != $2 and students.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).text() != "id = $1 and name != $2 and marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>::prefixed("s"_SQL)).text() == "s.id = $1 and s.name != $2 and s.marks = $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).text() == "students.id = $1 and students.name != $2 and students.marks = $3"_s));
        
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::eq, students::name::neq, students::marks::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::eq, students::name::neq, students::marks::eq>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::eq, students::name::neq, students::marks::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
    }

    SECTION("postgres decorators conditions lt gt"){
        pg::schema<
            students::table::column<students::id::gt>, 
            students::table::column<students::name::neq>, 
            students::table::column<students::grade::eq>, 
            students::table::column<students::marks::lt>
        > student;
        
        student[students::id::gt::val] = 42;
        student[students::name::neq::val] = std::string("Neel Basu");
        student[students::grade::eq::val] = 1;
        student[students::marks::lt::val] = 100;

        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() != "id > $1 and name != $2 and grade = $3 and marks < $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).text() == "s.id > $1 and s.name != $2 and s.grade = $3 and s.marks < $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() == "students.id > $1 and students.name != $2 and students.grade = $3 and students.marks < $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::gt, students::name::neq, students::marks::lt>{}).text() != "id > $1 and name != $2 and marks < $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::gt, students::name::neq, students::marks::lt>::prefixed("s"_SQL)).text() == "s.id > $1 and s.name != $2 and s.marks < $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::gt, students::name::neq, students::marks::lt>{}).text() == "students.id > $1 and students.name != $2 and students.marks < $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).text() != "id > $1 and name != $2 and marks < $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>::prefixed("s"_SQL)).text() == "s.id > $1 and s.name != $2 and s.marks < $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).text() == "students.id > $1 and students.name != $2 and students.marks < $3"_s));
        
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::gt, students::name::neq, students::marks::lt>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::gt, students::name::neq, students::marks::lt>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::gt, students::name::neq, students::marks::lt>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
    }

    SECTION("postgres decorators conditions lte gte like"){
        pg::schema<
            students::table::column<students::id::gte>, 
            students::table::column<students::name::like>, 
            students::table::column<students::grade::eq>, 
            students::table::column<students::marks::lte>
        > student;
        
        student[students::id::gte::val] = 42;
        student[students::name::like::val] = std::string("Neel Basu");
        student[students::grade::eq::val] = 1;
        student[students::marks::lte::val] = 100;

        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() != "id >= $1 and name like $2 and grade = $3 and marks <= $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).text() == "s.id >= $1 and s.name like $2 and s.grade = $3 and s.marks <= $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions{}).text() == "students.id >= $1 and students.name like $2 and students.grade = $3 and students.marks <= $4"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::gte, students::name::like, students::marks::lte>{}).text() != "id >= $1 and name like $2 and marks <= $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::gte, students::name::like, students::marks::lte>::prefixed("s"_SQL)).text() == "s.id >= $1 and s.name like $2 and s.marks <= $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::only<students::id::gte, students::name::like, students::marks::lte>{}).text() == "students.id >= $1 and students.name like $2 and students.marks <= $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).text() != "id >= $1 and name like $2 and marks <= $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>::prefixed("s"_SQL)).text() == "s.id >= $1 and s.name like $2 and s.marks <= $3"_s));
        CHECK(bool(student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).text() == "students.id >= $1 and students.name like $2 and students.marks <= $3"_s));
        
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::gte, students::name::like, students::marks::lte>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::gte, students::name::like, students::marks::lte>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::only<students::id::gte, students::name::like, students::marks::lte>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>::prefixed("s"_SQL)).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
        CHECK(bool((student.decorate(pg::decorators::conditions::except<students::grade::eq>{}).params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100))));
    }

}
