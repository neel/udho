#include "udho/router.h"

bya::ka::response_type bya::ka::failure_callback(bya::ka::request_type req){
    // do nothing
    std::cout << "nothing" << std::endl;
    http::response<http::string_body> res{http::status::unknown, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    res.body() = std::string("nothing");
    res.prepare_payload();
    return res;
}
