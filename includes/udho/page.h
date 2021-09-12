#ifndef UDHO_PAGE_H
#define UDHO_PAGE_H

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
        typedef boost::beast::http::header<false> headers_type;
        
        boost::beast::http::status _status;
        std::string _message;
        headers_type _headers;
        
        http_error(boost::beast::http::status status, const std::string& message = "");
        void add_header(boost::beast::http::field key, const std::string& value);
        void redirect(const std::string& url);
        /**
         * @brief Given an HTTP Request, Generates HTTP Response for the exception
         * 
         * @tparam T 
         * @param request The HTTP Request
         * @return boost::beast::http::response<boost::beast::http::string_body> 
         */
        template <typename T>
        boost::beast::http::response<boost::beast::http::string_body> response(const boost::beast::http::request<T>& request) const{
            boost::beast::http::response<boost::beast::http::string_body> res{_status, request.version()};
            for(const auto& header: _headers){
                res.set(header.name(), header.value());
            }
            res.set(boost::beast::http::field::server, UDHO_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(request.keep_alive());
            res.body() = page(request.target().to_string());
            res.prepare_payload();
            return res;
        }
        /**
         * @brief Given a udho request context generates HTTP response with the HTTP request inside the context
         * 
         * @tparam AuxT 
         * @tparam U 
         * @tparam V 
         * @param ctx The udho Request Context
         * @return boost::beast::http::response<boost::beast::http::string_body> 
         */
        template <typename AuxT, typename U, typename V>
        boost::beast::http::response<boost::beast::http::string_body> response(const udho::context<AuxT, U, V>& ctx) const{
            return response(ctx.request());
        }
        /**
         * @brief Given an HTTP request and the URL Router Generates HTTP response 
         * 
         * @tparam T 
         * @tparam RouterT 
         * @param request HTTP Request
         * @param router The URL Router
         * @return boost::beast::http::response<boost::beast::http::string_body> 
         */
        template <typename T, typename RouterT>
        boost::beast::http::response<boost::beast::http::string_body> response(const boost::beast::http::request<T>& request, RouterT& router) const{
            boost::beast::http::response<boost::beast::http::string_body> res{_status, request.version()};
            for(const auto& header: _headers){
                res.set(header.name(), header.value());
            }
            res.set(boost::beast::http::field::server, UDHO_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(request.keep_alive());
            res.body() = page(request.target().to_string(), router);
            res.prepare_payload();
            return res;
        }
        /**
         * @brief Given a udho request context and the URL Router Generates HTTP response using the HTTP request inside the context
         * 
         * @tparam AuxT 
         * @tparam U 
         * @tparam V 
         * @tparam RouterT 
         * @param ctx udho request context
         * @param router The URL Router
         * @return boost::beast::http::response<boost::beast::http::string_body> 
         */
        template <typename AuxT, typename U, typename V, typename RouterT>
        boost::beast::http::response<boost::beast::http::string_body> response(const udho::context<AuxT, U, V>& ctx, RouterT& router) const{
            return response(ctx.request(), router);
        }
        /**
         * @brief Renders the HTML string using the target and the content
         * 
         * @param target 
         * @param content 
         * @return std::string 
         */
        std::string page(const std::string& target, std::string content="") const;
        /**
         * @brief Generates the HTML string with the summary of the URL router.
         * 
         * @tparam RouterT 
         * @param target 
         * @param router 
         * @return std::string 
         */
        template <typename RouterT>
        std::string page(const std::string& target, RouterT& router) const{
            std::string buffer = internal::html_summary(router);
            return page(target, buffer);
        }
        /**
         * @brief Returns the exception as plain text
         * 
         * @return const char* 
         */
        virtual const char* what() const noexcept;
        /**
         * @brief HTTP Status assoviated with the exception
         * 
         * @return boost::beast::http::status 
         */
        boost::beast::http::status result() const;
    };
    http_error http_redirection(const std::string& location, boost::beast::http::status status = boost::beast::http::status::temporary_redirect);
    struct reroute: public std::exception{
        std::string _alt_path;
        
        explicit inline reroute(const std::string& alt_path): _alt_path(alt_path){}
        inline std::string alt_path() const{
            return _alt_path;
        }
    };
}

template <typename AuxT, typename RequestT, typename ShadowT>
udho::context<AuxT, RequestT, ShadowT>& operator<<(udho::context<AuxT, RequestT, ShadowT>& context, const exceptions::http_error& error){
    std::cout << "ERROR" << error._message << std::endl;
    auto response = error.response(context);
    context.respond(response);
    return context;
}
    
}

#endif // UDHO_PAGE_H
