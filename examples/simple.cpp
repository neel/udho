#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/req.h>
#include <iostream>

typedef udho::attachment<udho::loggers::plain> attachment_type;
typedef udho::req<boost::beast::http::request<boost::beast::http::string_body>, attachment_type> request_type;
typedef udho::server<attachment_type> server_type;

std::string hello(request_type req){
    return "Hello World";
}

std::string data(request_type req){
    return "{id: 2, name: 'udho'}";
}

int add(request_type req, int a, int b){
    return a + b;
}

boost::beast::http::response<boost::beast::http::file_body> file(request_type req){
    std::string path("/etc/passwd");
    boost::beast::error_code err;
    boost::beast::http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, err);
    auto const size = body.size();
    boost::beast::http::response<boost::beast::http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, req.version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    
    boost::asio::io_service io;
    udho::loggers::plain logger(std::cout);
    attachment_type attachment(logger);
    
    server_type server(io, attachment);

    auto router = udho::router<udho::loggers::plain>()
        | (udho::get(&file).raw() = "^/file")
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
//     router.listen<udho::req>(io, 9198, doc_root);
        
    server.serve(router, 9198, doc_root);
    
    udho::util::print_summary(router);
        
    io.run();
    
    return 0;
}
