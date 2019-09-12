#include "udho/page.h"
#include <boost/format.hpp>

bya::ka::exceptions::http_error::http_error(boost::beast::http::status status, const std::string& resource): _status(status), _resource(resource){

}

const char* bya::ka::exceptions::http_error::what() const noexcept{
    return (boost::format("%1% Error while accessing %2%") % _status % _resource).str().c_str();
}

std::string bya::ka::exceptions::http_error::page() const{
    std::string content = R"page(
    <html>
        <head>
            <title>%1% Error</title>
            <style>
                body{
                    background-color: yellow;
                }
                .block{
                    padding: 10px;
                    border-bottom: 1px solid #888;
                }
            </style>
        </head>
        <body>
            <div class="block">
                %2%
            </div>
        </body>
    </html>
    )page";
    return (boost::format(content) % _status % what()).str();
}

boost::beast::http::status bya::ka::exceptions::http_error::result() const{
    return _status;
}


boost::beast::http::response<boost::beast::http::string_body> bya::ka::page::not_found(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& message){
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, request.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.keep_alive(request.keep_alive());
    res.body() = message;
    res.prepare_payload();
    return res;
}

boost::beast::http::response<boost::beast::http::string_body> bya::ka::page::bad_request(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& message){
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, request.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.keep_alive(request.keep_alive());
    res.body() = message;
    res.prepare_payload();
    return res;
}

boost::beast::http::response<boost::beast::http::string_body> bya::ka::page::internal_error(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& message){
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, request.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.keep_alive(request.keep_alive());
    res.body() = message;
    res.prepare_payload();
    return res;
}

