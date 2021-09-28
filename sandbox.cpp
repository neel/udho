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
#include <udho/db/pg/crud/from.h>
#include <udho/db/pg/crud/into.h>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/constructs/alias.h>
#include <udho/db/pg/constructs/functions.h>
#include <udho/db/pg/constructs/concat.h>
#include <udho/db/pg/generators/generators.h>
#include <udho/db/pg/io/json.h>
#include <udho/db/pg/ozo/io.h>
#include <udho/db/pg/io/pretty.h>

namespace pg = udho::db::pg;

namespace students{

PG_ELEMENT(id,          pg::types::bigint);
PG_ELEMENT(first_name,  pg::types::varchar);
PG_ELEMENT(last_name,   pg::types::varchar);
PG_ELEMENT(writer,      pg::types::bigint);
PG_ELEMENT(name,        pg::types::text);

struct table: pg::relation<table, id, first_name, last_name>{
    PG_NAME(students)
    using readonly = pg::readonly<id>;
};

}

namespace articles{

PG_ELEMENT(id,          pg::types::bigint);
PG_ELEMENT(title,       pg::types::varchar);
PG_ELEMENT(abstract,    pg::types::text);
PG_ELEMENT(author,      pg::types::bigint);
PG_ELEMENT(project,     pg::types::bigint);
PG_ELEMENT(created,     pg::types::timestamp);

struct table: pg::relation<table, id, title, abstract, author, project, created>{
    PG_NAME(articles)
    using readonly = pg::readonly<id, created>;
};

}

namespace projects{

PG_ELEMENT(id,          pg::types::bigint);
PG_ELEMENT(title,       pg::types::varchar);
PG_ELEMENT(started,     pg::types::timestamp);

struct table: pg::relation<table, id, title, started>{
    PG_NAME(projects)
    using readonly = pg::readonly<id, started>;
};

}

namespace students{
    
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
    ::apply;

PG_ERROR_CODE(by_id, boost::beast::http::status::forbidden)
    
using fullname = pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>;
using like = pg::from<students::table>
    ::fetch
    ::all
    ::include<fullname::as<students::name>>
    ::by<students::id::gte, students::id::max::gte, fullname::like>
    ::apply;
       
using create = pg::into<students::table>
    ::insert
    ::writables
    ::returning<students::id>
    ::apply;
    
}

namespace articles{
    
using all = pg::from<students::table>
    ::join<articles::table>::inner::on<students::id, articles::author>
    ::fetch
    ::only<articles::table::all>
    ::exclude<articles::created, articles::title>
    ::include<articles::title::text>
    ::include<students::id::as<students::writer>>
    ::include<students::first_name::text, students::last_name::text>
    ::apply;
    
}

template <typename FieldT>
using relation_of = typename pg::from<students::table>::relation_of<FieldT>;

int main(){
    typedef udho::contexts::stateless context_type;
    typedef udho::servers::quiet::stateless server_type;
    
    pg::connection::config dbconfig;
    pg::connection::info conn_info("host=localhost dbname=udho user=postgres password=postgres");
    auto pool = pg::connection::pool(conn_info, dbconfig);
    
    boost::asio::io_service io;

    context_type::request_type req;
    server_type::attachment_type attachment(io);
    context_type ctx(attachment.aux(), req, attachment);
    
    auto start = pg::start<students::all, students::like, students::by_id, articles::all, students::create>::with(ctx, pool);
    auto fetch_students = pg::after(start).perform<students::all>(start);
    auto students_like  = pg::after(start).perform<students::like>(start);
    auto fetch_student  = pg::after(start).perform<students::by_id>(start);
    
    students_like[students::id::gte::val] = 2;
    students_like[students::id::max::gte::val] = 2;
    students_like[students::fullname::like::val] = "Hello";
    auto fetch_articles = pg::after(start).perform<articles::all>(start);
    auto create_student = pg::after(fetch_students, fetch_articles).perform<students::create>(start, pg::types::varchar::val("Neel"), pg::types::varchar::val("Basu"));
    create_student[students::first_name::val] = "NeeleX";
    create_student->element(students::last_name::val).null(true);
    
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
                std::cout << result[articles::id::val] << " " << result[articles::author::val] << " " << result[students::writer::val] << " " << result[students::first_name::text::val] << " " << result[students::last_name::text::val] << std::endl;
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
