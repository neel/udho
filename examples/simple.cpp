#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <udho/visitor.h>
#include <iostream>
#include <boost/serialization/access.hpp>

struct user{
    std::string name;
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & name;
    }
    private:
        friend class boost::serialization::access;
};

struct appearence{
    std::string color;
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & color;
    }
    private:
        friend class boost::serialization::access;
};

std::string hello(udho::contexts::stateful<user> ctx){
    std::cout << "user returning: " << ctx.session().returning() << std::endl;
    std::cout << "session id: " << ctx.session().id() << std::endl;
    
    std::cout << "user data exists: " << ctx.session().exists<user>() << std::endl;

    if(ctx.session().exists<user>()){
        user data;
        ctx.session() >> data;
        return data.name;
    }else{
        user data;
        data.name = "Neel Basu";
        ctx.session() << data;
        ctx.cookies() << udho::cookie("planet", 3);
        return "Neel Basu set";
    }
}

std::string see(udho::contexts::stateful<appearence> ctx){
    std::cout << "user returning: " << ctx.session().returning() << std::endl;
    std::cout << "session id: " << ctx.session().id() << std::endl;
    
    std::cout << "appearence data exists: " << ctx.session().exists<appearence>() << std::endl;
    if(ctx.session().exists<appearence>()){
        appearence data;
        ctx.session() >> data;
        return data.color;
    }else{
        appearence data;
        data.color = "red";
        ctx.session() << data;
        return "red set";
    }
}

std::string unset_see(udho::contexts::stateful<appearence> ctx){
    std::cout << "user returning: " << ctx.session().returning() << std::endl;
    std::cout << "session id: " << ctx.session().id() << std::endl;
    
    std::cout << "appearence data exists: " << ctx.session().exists<appearence>() << std::endl;
    if(ctx.session().exists<appearence>()){
        ctx.session().remove<appearence>();
        return "appearence unset";
    }else{
        return "appearence not set";
    }
}

std::string unset(udho::contexts::stateful<appearence> ctx){
    std::cout << "user returning: " << ctx.session().returning() << std::endl;
    std::cout << "session id: " << ctx.session().id() << std::endl;

    if(ctx.session().returning()){
        ctx.session().remove();
        return "session unset";
    }else{
        return "user not returning";
    }
}

std::string hello_see(udho::contexts::stateful<user, appearence> ctx){
    std::cout << "server sessions: " << ctx.session().size() << std::endl;
    std::cout << "server sessions having user: " << ctx.session().exists<user>() << std::endl;
    std::cout << "user returning: " << ctx.session().returning() << std::endl;
    std::cout << "session id: " << ctx.session().id() << std::endl;
    std::cout << "user data exists: " << ctx.session().exists<user>() << std::endl;
    std::cout << "appearence data exists: " << ctx.session().exists<appearence>() << std::endl;
    std::cout << "created " << ctx.session().created() << std::endl;
    std::cout << "updated " << ctx.session().updated() << std::endl;
    std::cout << "age " << ctx.session().age() << std::endl;
    std::cout << "idle " << ctx.session().idle() << std::endl;
    
    std::string name;
    std::string color;
    
    if(ctx.session().exists<user>()){
        user data;
        ctx.session() >> data;
        name = data.name;
    }
    if(ctx.session().exists<appearence>()){
        appearence data;
        ctx.session() >> data;
        color = data.color;
    }
    
    return name+" "+color;
}

std::string data(udho::contexts::stateless ctx){
    ctx << udho::logging::messages::formatted::debug("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
    
    std::cout << "name submitted" << ctx.form().has("name") << std::endl;
    std::cout << "age submitted" << ctx.form().has("age") << std::endl;
    std::cout << "planet submitted" << ctx.form().has("planet") << std::endl;

    if(ctx.form().has("age")){
        int age = ctx.form().field<int>("age")-1;
        std::cout << "age " << age << std::endl;
    }
    return "{id: 2, name: 'udho'}";
}

int sub(udho::contexts::stateless ctx, int a, int b){
    std::cout << "target: " << ctx.target() << std::endl;
    std::cout << "path: " << ctx.path() << std::endl;
    if(ctx.query().has("apiKey")){
        std::cout << "Request have apiKey: " << ctx.query().field<std::string>("apiKey") << std::endl;
    }
    return a - b;
}

int absolute(udho::contexts::stateless ctx, int a, int b){
    std::cout << "target: " << ctx.target() << std::endl;
    std::cout << "path: " << ctx.path() << std::endl;
    if(a > b){
        return a - b;
    }else if(a < b){
        ctx.reroute("/sub/$2/$1");
        return 42;
    }else{
        throw udho::exceptions::reroute("/sub/$2/$1");
    }
}

boost::beast::http::response<boost::beast::http::file_body> file(udho::contexts::stateless ctx){
    std::string path("/etc/passwd");
    boost::beast::error_code err;
    boost::beast::http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, err);
    auto const size = body.size();
    boost::beast::http::response<boost::beast::http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, ctx.request().version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.content_length(size);
    res.keep_alive(ctx.request().keep_alive());
    return res;
}

void long_poll(udho::contexts::stateful<appearence> ctx){
    typedef boost::beast::http::response<boost::beast::http::string_body>  response_type;
    std::string content = "Hello World";
    response_type res{boost::beast::http::status::ok, ctx.request().version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type,   "text/plain");
    res.set(boost::beast::http::field::content_length, content.size());
    res.keep_alive(ctx.request().keep_alive());
    res.body() = content;
    res.prepare_payload();
    ctx.respond(res);
}

boost::beast::http::response<boost::beast::http::file_body> local(udho::contexts::stateless ctx){
    return ctx.aux().file("README.md", ctx.request(), "text/plain");
}

void fetch_good(udho::contexts::stateless ctx){
    ctx.client().get("https://tls-v1-2.badssl.com:1012")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            ctx.status(status);
            ctx.respond(body, "text/html");
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            ctx.status(boost::beast::http::status::internal_server_error);
            ctx.respond(ec.message(), "text/plain");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, true);
}

void fetch_bad(udho::contexts::stateless ctx){   
    ctx.client().get("http://expired.badssl.com")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            ctx.status(status);
            ctx.respond(body, "text/html");
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            ctx.status(boost::beast::http::status::internal_server_error);
            ctx.respond(ec.message(), "text/plain");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, false);
}

void fetch_echo(udho::contexts::stateless ctx){   
    ctx.client().get(udho::url::build("https://postman-echo.com/get", {
            {"foo", "bar"},
            {"key", "shajdhwuhuemkzwef5454rcjkrjm"}
        }))
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            ctx.status(status);
            ctx.respond(body, "text/json");
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            ctx.status(boost::beast::http::status::internal_server_error);
            ctx.respond(ec.message(), "text/plain");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, false);
}

int main(){    
    boost::asio::io_service io;
    udho::servers::ostreamed::stateful<udho::cache::storage::disk, user, appearence> server(io, std::cout);
    server[udho::configs::server::template_root] = TMPL_PATH;
    server[udho::configs::server::document_root] = WWW_PATH;
    server[udho::configs::session::path] = "sessions";
    auto router = udho::router()
        | (udho::get(&file).raw()            = "^/file")
        | (udho::get(&long_poll).deferred()  = "^/poll")
        | (udho::get(&fetch_good).deferred() = "^/fetch/good")
        | (udho::get(&fetch_bad).deferred()  = "^/fetch/bad")
        | (udho::get(&fetch_echo).deferred() = "^/fetch/echo")
        | (udho::get(&local).raw()           = "^/local")
        | (udho::get(&hello).plain()         = "^/hello$")
        | (udho::get(&see).plain()           = "^/see$")
        | (udho::get(&hello_see).plain()     = "^/hello_see$")
        | (udho::get(&unset_see).plain()     = "^/unset_see$")
        | (udho::get(&unset).plain()         = "^/unset$")
        | (udho::post(&data).json()          = "^/data$")
        | (udho::get(&sub).plain()           = "^/sub/(\\d+)/(\\d+)$")
        | (udho::get(udho::reroute("/sub/$2/$1")).raw()  = "^/bus/(\\d+)/(\\d+)$")
        | (udho::get(&absolute).plain()      = "^/abs/(\\d+)/(\\d+)$");
        
    router /= udho::visitors::print<udho::visitors::visitable::both, std::ostream>(std::cout);
        
    server.serve(router, 9198);
        
    io.run();
    
    return 0;
}
