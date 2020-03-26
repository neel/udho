#ifndef PAGE_H
#define PAGE_H

#include "util.h"
#include <stdexcept>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <udho/context.h>
#include <iostream>
#include <udho/visitor.h>

namespace udho{
    
namespace internal{
    template <typename RouterT>
    std::string html_summary(RouterT& router){
        std::stringstream stream;
        router /= udho::visitors::print_html<udho::visitors::visitable::both, std::stringstream>(stream);
        return stream.str();
    }
}
    
namespace exceptions{
    /**
     * throw HTTP error exception in order to send a HTTP response other than 200 OK. Uncaught exceptions are presented as html page with routing sumary 
     * @code
     * http_error(boost::beast::http::status::notfound)
     * @endcode
     */
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
        template <typename U, typename V>
        boost::beast::http::response<boost::beast::http::string_body> response(const udho::context<U, V>& ctx) const{
            return response(ctx.request());
        }
        template <typename T, typename RouterT>
        boost::beast::http::response<boost::beast::http::string_body> response(const boost::beast::http::request<T>& request, RouterT& router) const{
            boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, request.version()};
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(request.keep_alive());
            res.body() = page(router);
            res.prepare_payload();
            return res;
        }
        template <typename U, typename V, typename RouterT>
        boost::beast::http::response<boost::beast::http::string_body> response(const udho::context<U, V>& ctx, RouterT& router) const{
            return response(ctx.request(), router);
        }
        virtual std::string page(std::string content="") const;
        template <typename RouterT>
        std::string page(RouterT& router) const{
            std::string buffer = internal::html_summary(router);
            return page(buffer);
        }
        virtual const char* what() const noexcept;
        boost::beast::http::status result() const;
    };
}
    
}

#endif // PAGE_H
