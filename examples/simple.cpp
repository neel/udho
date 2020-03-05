#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <iostream>

struct user{
    std::string name;
};

struct appearence{
    std::string color;
};

typedef udho::servers::ostreamed::stateful<user, appearence> server_type;

std::string hello(server_type::context ctx){
    std::cout << "user returning: " << ctx.session().returning() << std::endl;
    std::cout << "session id: " << ctx.session().id() << std::endl;
    
    std::cout << "user data exists: " << ctx.session().exists<user>() << std::endl;
    if(ctx.session().exists<user>()){
        user data;
        ctx.session() >> data;
        std::cout << data.name << std::endl;
    }else{
        user data;
        data.name = "Neel Basu";
        ctx.session() << data;
        ctx.cookies() << udho::cookie("planet", 3);
    }
    
    return "Hello World";
}

std::string data(server_type::context ctx){
    std::cout << "name submitted" << ctx.form().has("name") << std::endl;
    std::cout << "age submitted" << ctx.form().has("age") << std::endl;
    std::cout << "planet submitted" << ctx.form().has("planet") << std::endl;

    if(ctx.form().has("age")){
        int age = ctx.form().field<int>("age")-1;
        std::cout << "age " << age << std::endl;
    }
    return "{id: 2, name: 'udho'}";
}

int add(server_type::request_type ctx, int a, int b){
    return a + b;
}

boost::beast::http::response<boost::beast::http::file_body> file(server_type::context ctx){
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

int main(){
    
    typedef udho::cache::store<std::string, user, appearence> store_type;
    store_type store;
    udho::cache::shadow<std::string, user, appearence> shadow_ua(store);
    user data;
    data.name = "Neel";
    shadow_ua.insert("x", data);
    appearence appr;
    appr.color = "red";
    shadow_ua.insert("x", appr);
    std::cout << std::boolalpha << shadow_ua.exists<user>("x") << std::endl;
    std::cout << std::boolalpha << shadow_ua.exists<appearence>("x") << std::endl;
    std::cout << shadow_ua.get<user>("x").name << std::endl;
    std::cout << shadow_ua.get<appearence>("x").color << std::endl;
    std::cout << shadow_ua.exists<appearence>("x") << std::endl;
    udho::cache::shadow<std::string, user> shadow_u(store);
    std::cout << std::boolalpha << shadow_u.exists<user>("x") << std::endl;
    std::cout << std::boolalpha << shadow_u.get<user>("x").name << std::endl;
//     std::cout << shadow_u.exists<appearence>("x") << std::endl;
    udho::cache::shadow<std::string, user> shadow_u2(shadow_u);
    std::cout << shadow_u2.get<user>("x").name << std::endl;
    udho::cache::shadow<std::string, user> shadow_u3(shadow_ua);
    std::cout << shadow_u3.get<user>("x").name << std::endl;
    udho::cache::shadow<std::string> shadow_u4(shadow_ua);
    
//     std::string doc_root("/home/neel/Projects/udho"); // path to static content
//     
//     boost::asio::io_service io;
//     server_type server(io, std::cout);
// 
//     auto router = udho::router<>()
//         | (udho::get(&file).raw() = "^/file")
//         | (udho::get(&hello).plain() = "^/hello$")
//         | (udho::post(&data).json()   = "^/data$")
//         | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
//         
//     udho::util::print_summary(router);
//         
//     server.serve(router, 9198, doc_root);
//         
//     io.run();
    
    return 0;
}
