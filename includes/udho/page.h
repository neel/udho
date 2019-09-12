#ifndef PAGE_H
#define PAGE_H

#include <stdexcept>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace udho{
    
namespace exceptions{
    struct http_error: public std::exception{
        std::string _resource;
        boost::beast::http::status _status;
        
        http_error(boost::beast::http::status status, const std::string& resource);
        template <typename T>
        boost::beast::http::response<boost::beast::http::string_body> response(const boost::beast::http::request<T>& request) const{
            boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, request.version()};
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(request.keep_alive());
            res.body() = page();
            res.prepare_payload();
            return res;
        }
        virtual std::string page() const;
        virtual const char* what() const noexcept;
        boost::beast::http::status result() const;
    };
}
    
struct page{
    static boost::beast::http::response<boost::beast::http::string_body> not_found(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& message);
    static boost::beast::http::response<boost::beast::http::string_body> bad_request(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& message);
    static boost::beast::http::response<boost::beast::http::string_body> internal_error(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& message);
};

}

#endif // PAGE_H
