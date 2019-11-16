#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>

std::string hello(udho::request_type req){
    return "Hello World";
}

std::string data(udho::request_type req){
    return "{id: 2, name: 'udho'}";
}

int add(udho::request_type req, int a, int b){
    return a + b;
}

boost::beast::http::response<boost::beast::http::file_body> file(udho::request_type req){
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

    auto router = udho::router()
        | (udho::get(&file).raw() = "^/file")
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
    router.listen(io, 9198, doc_root);
    
    udho::util::print_summary(router);
        
    io.run();
    
    return 0;
}
