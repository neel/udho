#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <iostream>

std::string hello(udho::servers::logged::context ctx){
    udho::cookie color("color", "red");
    ctx.add(color);
    
    return "Hello World";
}

std::string data(udho::servers::logged::context ctx){
    return "{id: 2, name: 'udho'}";
}

int add(udho::servers::logged::context ctx, int a, int b){
    return a + b;
}

boost::beast::http::response<boost::beast::http::file_body> file(udho::servers::logged::context ctx){
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
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    
    boost::asio::io_service io;
    
    udho::servers::logged server(io, std::cout);

    auto router = udho::router<>()
        | (udho::get(&file).raw() = "^/file")
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
        
    udho::util::print_summary(router);
        
    server.serve(router, 9198, doc_root);
        
    io.run();
    
    return 0;
}
