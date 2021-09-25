#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::db::pg::generators)"
#include <boost/test/unit_test.hpp>

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
    static auto name(){
        return "students"_SQL;
    }
};

}

BOOST_AUTO_TEST_SUITE(postgres_generators)

BOOST_AUTO_TEST_CASE(postgres_generators_select){
    students::table::schema student;
    
    pg::generators::select<students::table::schema> select(student);
    
    BOOST_CHECK(select.all().text() == "select students.id, students.name, students.grade, students.marks"_s);
    BOOST_CHECK(select.all("s"_SQL).text() == "select s.id, s.name, s.grade, s.marks"_s);
    
    BOOST_CHECK((select.only<students::id, students::name, students::marks>().text() == "select students.id, students.name, students.marks"_s));
    BOOST_CHECK((select.only<students::id, students::name, students::marks>("s"_SQL).text() == "select s.id, s.name, s.marks"_s));
    
    BOOST_CHECK((select.except<students::grade>().text() == "select students.id, students.name, students.marks"_s));
    BOOST_CHECK((select.except<students::grade>("s"_SQL).text() == "select s.id, s.name, s.marks"_s));
}

BOOST_AUTO_TEST_CASE(postgres_generators_keys){
    students::table::schema student;
    
    pg::generators::keys<students::table::schema> keys(student);
    
    BOOST_CHECK(keys.all().text() == "(id, name, grade, marks)"_s);
    
    BOOST_CHECK((keys.only<students::id, students::name, students::marks>().text() == "(id, name, marks)"_s));
    
    BOOST_CHECK((keys.except<students::grade>().text() == "(id, name, marks)"_s));
}

BOOST_AUTO_TEST_CASE(postgres_generators_values){
    students::table::schema student;
    
    student[students::id::val] = 42;
    student[students::name::val] = std::string("Neel Basu");
    student[students::grade::val] = 1;
    student[students::marks::val] = 100;
    
    pg::generators::values<students::table::schema> values(student);
    
    BOOST_CHECK(values.all().text() == "values ($1, $2, $3, $4)"_s);
    BOOST_CHECK((values.all().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)));
    
    BOOST_CHECK((values.only<students::id, students::name, students::marks>().text() == "values ($1, $2, $3)"_s));
    BOOST_CHECK((values.only<students::id, students::name, students::marks>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
    
    BOOST_CHECK((values.except<students::grade>().text() == "values ($1, $2, $3)"_s));
    BOOST_CHECK((values.except<students::grade>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
}

BOOST_AUTO_TEST_CASE(postgres_generators_set){
    students::table::schema student;
    
    student[students::id::val] = 42;
    student[students::name::val] = std::string("Neel Basu");
    student[students::grade::val] = 1;
    student[students::marks::val] = 100;
    
    pg::generators::set<students::table::schema> set(student);
    
    BOOST_CHECK(set.all().text() == "set id = $1, name = $2, grade = $3, marks = $4"_s);
    BOOST_CHECK((set.all().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)));
    
    BOOST_CHECK((set.only<students::id, students::name, students::marks>().text() == "set id = $1, name = $2, marks = $3"_s));
    BOOST_CHECK((set.only<students::id, students::name, students::marks>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
    
    BOOST_CHECK((set.except<students::grade>().text() == "set id = $1, name = $2, marks = $3"_s));
    BOOST_CHECK((set.except<students::grade>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
}

BOOST_AUTO_TEST_CASE(postgres_generators_where){
    students::table::schema student;
    
    student[students::id::val] = 42;
    student[students::name::val] = std::string("Neel Basu");
    student[students::grade::val] = 1;
    student[students::marks::val] = 100;
    
    pg::generators::where<students::table::schema> where(student);

    BOOST_CHECK(where.all().text() == "where students.id = $1 and students.name = $2 and students.grade = $3 and students.marks = $4"_s);
    BOOST_CHECK((where.all().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t, std::int64_t>(42, "Neel Basu", 1, 100)));
    
    BOOST_CHECK((where.only<students::id, students::name, students::marks>().text() == "where students.id = $1 and students.name = $2 and students.marks = $3"_s));
    BOOST_CHECK((where.only<students::id, students::name, students::marks>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
    
    BOOST_CHECK((where.except<students::grade>().text() == "where students.id = $1 and students.name = $2 and students.marks = $3"_s));
    BOOST_CHECK((where.except<students::grade>().params() == boost::hana::tuple<std::int64_t, std::string, std::int64_t>(42, "Neel Basu", 100)));
}

BOOST_AUTO_TEST_CASE(postgres_generators_returning){
    students::table::schema student;

    pg::generators::returning<students::table::schema> returning;

    BOOST_CHECK(returning.all().text() == "returning id, name, grade, marks"_s);
    
    BOOST_CHECK((returning.only<students::id, students::name, students::marks>().text() == "returning id, name, marks"_s));
    
    BOOST_CHECK((returning.except<students::grade>().text() == "returning id, name, marks"_s));
}

BOOST_AUTO_TEST_SUITE_END()
