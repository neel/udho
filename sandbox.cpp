#include <iostream>
#include <udho/db/pg.h>
#include <udho/db/pg/schema/relation.h>
#include <udho/db/pg/crud/limit.h>
#include <udho/db/pg/crud/order.h>
#include <udho/db/pg/activities/start.h>
#include <udho/db/pg/activities/after.h>
#include <udho/db/pg/activities/data.h>
#include <udho/context.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <udho/activities.h>
#include <boost/hana.hpp>
#include <udho/db/pg/crud/join.h>
#include <udho/db/pg/crud/create.h>
#include <udho/db/pg/crud/from.h>
#include <udho/db/pg/crud/into.h>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/constructs/alias.h>
#include <udho/db/pg/constructs/functions.h>
#include <udho/db/pg/constructs/concat.h>
#include <udho/db/pg/generators/generators.h>
#include <udho/db/pg/io/json.h>
#include <udho/db/pg/ozo/io.h>
#include <udho/pretty/pretty.h>

namespace pg = udho::db::pg;

namespace students{

PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
PG_ELEMENT(first_name,  pg::types::varchar, pg::constraints::not_null);
PG_ELEMENT(last_name,   pg::types::varchar);
PG_ELEMENT(writer,      pg::types::bigint);
PG_ELEMENT(name,        pg::types::text);

struct table: pg::relation<table, id, first_name, last_name>{
    PG_NAME(students)
    using readonly = pg::readonly<id>;
};

}

namespace articles{

PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
PG_ELEMENT(title,       pg::types::varchar, pg::constraints::unique, pg::constraints::not_null);
PG_ELEMENT(abstract,    pg::types::text);
PG_ELEMENT(author,      pg::types::bigint, pg::constraints::not_null, pg::constraints::references<students::table::column<students::id>>::restrict);
PG_ELEMENT(project,     pg::types::bigint);
PG_ELEMENT(created,     pg::types::timestamp, pg::constraints::not_null, pg::constraints::default_<pg::constants::now>::value);

struct table: pg::relation<table, id, title, abstract, author, project, created>{
    PG_NAME(articles)
    using readonly = pg::readonly<id, created>;
};

}

namespace projects{

PG_ELEMENT(id,          pg::types::bigserial, pg::constraints::primary);
PG_ELEMENT(title,       pg::types::varchar);
PG_ELEMENT(started,     pg::types::timestamp);

struct table: pg::relation<table, id, title, started>{
    PG_NAME(projects)
    using readonly = pg::readonly<id, started>;
};

}

namespace students{
    
using destruct = pg::ddl<students::table>
                 ::drop
                 ::apply;

using construct = pg::ddl<students::table>
                 ::create
                 ::if_exists
                 ::skip
                 ::apply;

using all = pg::from<students::table>
    ::fetch
    ::all
    ::exclude<students::first_name, students::last_name>
    ::include<students::first_name::text, students::last_name::text>
    ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
    ::apply;

using by_id = pg::from<students::table>
    ::retrieve
    ::all
    ::exclude<students::first_name, students::last_name>
    ::include<students::first_name::text, students::last_name::text>
    ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
    ::by<students::id>
    ::apply;

PG_ERROR_CODE(by_id, boost::beast::http::status::forbidden)
    
using fullname = pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>;
using like = pg::from<students::table>
    ::fetch
    ::all
    ::include<fullname::as<students::name>>
    ::by<students::id::gte, fullname::like>
    ::apply;
       
using create = pg::into<students::table>
    ::insert
    ::only<students::first_name, students::last_name>
    ::returning<students::id>
    ::apply;

}

namespace articles{
    
using destruct = pg::ddl<articles::table>
                 ::drop
                 ::apply;

using construct = pg::ddl<articles::table>
                 ::create
                 ::if_exists
                 ::skip
                 ::apply;

using all = pg::from<students::table>
    ::join<articles::table>::inner::on<students::id, articles::author>
    ::fetch
    ::only<articles::table::all>
    ::exclude<articles::created, articles::title>
    ::include<articles::title::text>
    ::include<students::id::as<students::writer>>
    ::include<students::first_name::text, students::last_name::text>
    ::apply;

using create = pg::into<articles::table>
    ::insert
    ::only<articles::title, articles::author>
    ::returning<articles::id>
    ::apply;
    
}

template <typename FieldT>
using relation_of = typename pg::from<students::table>::relation_of<FieldT>;

int main(){
    typedef udho::contexts::stateless context_type;
    typedef udho::servers::quiet::stateless server_type;
    
    pg::connection::config dbconfig;
    pg::connection::info conn_info("host=localhost dbname=postgres user=postgres");
    auto pool = pg::connection::pool(conn_info, dbconfig);

    boost::asio::io_service io;

    context_type::request_type req;
    server_type::attachment_type attachment(io);
    context_type ctx(attachment.aux(), req, attachment);
    
    auto start = pg::start<articles::destruct, students::destruct, articles::construct, students::construct, students::all, students::like, students::by_id, articles::all, students::create, articles::create>::with(ctx, pool);
    auto drop_articles   = pg::after(start).perform<articles::destruct>(start);
    auto drop_students   = pg::after(drop_articles).perform<students::destruct>(start);
    auto create_students = pg::after(drop_articles, drop_students).perform<students::construct>(start);
    auto create_articles = pg::after(drop_articles, drop_students).perform<articles::construct>(start);
    auto fetch_students  = pg::after(create_articles, create_students).perform<students::all>(start);
    auto create_student  = pg::after(fetch_students).perform<students::create>(start, pg::types::varchar::val("Neel"), pg::types::varchar::val("Basu"));
    create_student[students::first_name::val] = pg::oz::varchar("NeeleX");
    create_student->element(students::last_name::val).null(true);
    auto create_article  = pg::after(create_student).perform<articles::create>(start, pg::types::varchar::val("Article Title"), 1);
    auto fetch_articles  = pg::after(create_article).perform<articles::all>(start);
    auto fetch_student   = pg::after(create_student).perform<students::by_id>(start);
    fetch_student[students::id::val] = 1;
    auto students_like   = pg::after(fetch_student).perform<students::like>(start);
    students_like[students::id::gte::val] = 2;
    students_like[students::fullname::like::val] = std::string("Hello");
    
    pg::after(fetch_students, fetch_articles, create_student).finish(start, [&](const pg::data<students::all, articles::all, students::create>& d){
        if(d.failed<students::all>()){
            const auto& failure = d.failure<students::all>();
            std::cout << failure.reason << std::endl;
        }else{
            const auto& results = d.success<students::all>();
            nlohmann::json json = results;
            std::cout << json << std::endl;
            std::cout << results.count() << " records" << std::endl;
            for(const auto& result: results){
                result[students::id::val];
                std::cout << result[students::id::val] << " " << result[students::first_name::text::val] << " " << result[students::last_name::text::val] << std::endl;
            }
        }
        
        if(d.failed<articles::all>()){
            const auto& failure = d.failure<articles::all>();
            std::cout << failure.reason << std::endl;
        }else{
            const auto& results = d.success<articles::all>();
            nlohmann::json json = results;
            std::cout << json << std::endl;
            std::cout << results.count() << " records" << std::endl;
            for(const auto& result: results){
                std::cout << result[articles::id::val].value() << " " << result[articles::author::val] << " " << result[students::writer::val] << " " << result[students::first_name::text::val] << " " << result[students::last_name::text::val] << std::endl;
                std::cout << result << std::endl;
            }
        }
        
        if(d.failed<students::create>()){
            const auto& failure = d.failure<students::create>();
            std::cout << failure.reason << std::endl;
        }else{
            const auto& result = *d.success<students::create>();
            std::cout << "student created " << result[students::id::val] << std::endl;
        }
    });
    
    
    start();
    
    io.run();
    
    return 0;
}
