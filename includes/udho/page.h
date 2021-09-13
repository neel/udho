#ifndef UDHO_PAGE_H
#define UDHO_PAGE_H

#include "util.h"
#include <sstream>
#include <string>
#include <sstream>
#include <set>
#include <vector>
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

namespace visual{

    /**
     * @brief A block in the error page generated due to an exception
     * 
     */
    class block{
        std::string           _name;
        std::string           _id;
        std::set<std::string> _classes;
        std::string           _content;

        public:
            inline explicit block(const std::string& name): _name(name) {}
            inline explicit block(const std::string& name, const std::string& body): _name(name), _content(body) {}

            inline void id(const std::string& id) { _id = id; }
            inline std::string id() const { return _id; }

            inline void add_class(const std::string& class_name) { _classes.insert(class_name); }
            inline bool has_class(const std::string& class_name) const { return _classes.count(class_name) > 0; }

            inline void content(const std::string& body) { _content = body; }
            inline std::string content() const { return _content; }

            std::string html() const;
    };

    /**
     * @brief An error page consists of an HTTP status and a heading. It may also contain a set of blocks
     * 
     */
    class page{
        boost::beast::http::status _status;
        std::string                _heading;
        std::vector<block>         _blocks;

        public:
            inline explicit page(const boost::beast::http::status& status): _status(status) {}
            inline explicit page(const boost::beast::http::status& status, const std::string& heading): _status(status), _heading(heading) {}

            inline void heading(const std::string& heading) { _heading = heading; }
            inline std::string heading() const { return _heading; }

            boost::beast::http::status status() const { return _status; }

            inline void add_block(const block& blk) { _blocks.push_back(blk); }

            std::string html() const;
    };
}

    /**
     * throw HTTP error exception in order to send a HTTP response other than 200 OK. Uncaught exceptions are presented as html page with routing sumary 
     * @code
     * http_error(boost::beast::http::status::notfound)
     * @endcode
     */
    struct http_error: public std::exception{
        typedef boost::beast::http::header<false> headers_type;
        
        std::string _err_str;
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
            visual::page p = page(request.target().to_string());
            decorate(p);

            boost::beast::http::response<boost::beast::http::string_body> res{_status, request.version()};
            for(const auto& header: _headers){
                res.set(header.name(), header.value());
            }
            res.set(boost::beast::http::field::server, UDHO_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(request.keep_alive());
            res.body() = p.html();
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
            visual::page p = page(request.target().to_string());
            std::string routes = "<div class='routes'>"+internal::html_summary(router)+"</div>";
            visual::block routing("routes", routes);
            p.add_block(routing);
            decorate(p);

            boost::beast::http::response<boost::beast::http::string_body> res{_status, request.version()};
            for(const auto& header: _headers){
                res.set(header.name(), header.value());
            }
            res.set(boost::beast::http::field::server, UDHO_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(request.keep_alive());
            res.body() = p.html();
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
         * @return visual::page
         */
        visual::page page(const std::string& target) const;
        /**
         * @brief Decorate a page by adding blocks
         * 
         * @param p 
         */
        virtual void decorate(visual::page& p) const;
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
