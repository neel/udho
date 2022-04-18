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

#include <udho/db/pg/generators/select.h>
#include <udho/db/pg/generators/from.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace db = udho::db;
namespace pg = db::pg;

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

TEST_CASE("postgresql activity with plain OZO SQL query", "[pg]") {
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    ozo::connection_pool_config dbconfig;
    ozo::connection_info<> conn_info("host=localhost dbname=postgres user=postgres");
    auto pool = ozo::connection_pool(conn_info, dbconfig);

    using all_students = pg::from<students::table>
                            ::fetch
                            ::all
                            ::apply;

}