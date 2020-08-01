/*
 * Copyright (c) 2020, <copyright holder> <email>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_CONTEXT_H
#define UDHO_CONTEXT_H

#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/optional.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/signals2/signal.hpp>
#include <udho/util.h>
#include <udho/cache.h>
#include <udho/logging.h>
#include <udho/defs.h>
#include <udho/form.h>
#include <udho/cookie.h>
#include <udho/session.h>
#include <udho/attachment.h>
#include <udho/compositors.h>

namespace udho{

namespace detail{
 
template <typename RequestT>
struct context_impl{
    typedef RequestT                                     request_type;
    typedef context_impl<request_type>                   self_type;
    typedef udho::form_<request_type>                    form_type;
    typedef udho::cookies_<request_type>                 cookies_type;
    typedef boost::beast::http::header<true>             headers_type;
    typedef urlencoded_form<std::string::const_iterator> query_parser_type;
    typedef std::stack<udho::detail::route>              route_stack_type;
    
    const request_type& _request;
    form_type           _form;
    std::string         _query_string;
    query_parser_type   _query;
    headers_type        _headers;
    cookies_type        _cookies;
    route_stack_type    _routes;
    
    boost::beast::http::status _status;
//     std::string                _pattern;
    
    boost::signals2::signal<void (const udho::logging::messages::error&)>   _error;
    boost::signals2::signal<void (const udho::logging::messages::warning&)> _warning;
    boost::signals2::signal<void (const udho::logging::messages::info&)>    _info;
    boost::signals2::signal<void (const udho::logging::messages::debug&)>   _debug;
    boost::signals2::signal<void (udho::defs::response_type&)>              _respond;    
    
    context_impl(const request_type& request): _request(request), _form(request), _cookies(request, _headers), _status(boost::beast::http::status::ok){
        _query_string = query_string();
        _query.parse(_query_string.begin(), _query_string.end());
    }
    context_impl(const self_type& other) = delete;
    const request_type& request() const{return _request;}
    cookies_type& cookies(){
        return _cookies;
    }
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        res.result(_status);
        for(const auto& header: _headers){
            if(header.name() != boost::beast::http::field::set_cookie){
                res.set(header.name(), header.value());
            }
        }
        res.erase(boost::beast::http::field::set_cookie);
        for(const auto& header: _headers){
            if(header.name() == boost::beast::http::field::set_cookie){
                res.insert(header.name(), header.value());
            }
        }
    }
    void log(const udho::logging::messages::error& msg) const{
        _error(msg);
    }
    void log(const udho::logging::messages::warning& msg) const{
        _warning(msg);
    }
    void log(const udho::logging::messages::info& msg) const{
        _info(msg);
    }
    void log(const udho::logging::messages::debug& msg) const{
        _debug(msg);
    }
    template <typename AuxT, typename LoggerT, typename CacheT>
    void attach(udho::attachment<AuxT, LoggerT, CacheT>& attachment){
        boost::function<void (const udho::logging::messages::error&)> errorf(boost::ref(attachment));
        boost::function<void (const udho::logging::messages::warning&)> warningf(boost::ref(attachment));
        boost::function<void (const udho::logging::messages::info&)> infof(boost::ref(attachment));
        boost::function<void (const udho::logging::messages::debug&)> debugf(boost::ref(attachment));
        
        _error.connect(errorf);
        _warning.connect(warningf);
        _info.connect(infof);
        _debug.connect(debugf);
    }
    void respond(udho::defs::response_type& response){
        _respond(response);
    }
    void status(boost::beast::http::status status){
        _status = status;
    }
    void clear(){
        _status   = boost::beast::http::status::ok;
    }
    void reroute(const std::string& path){
        if(_routes.size()){
            _routes.top().reroute(path);
        }else{
            log(udho::logging::messages::formatted::error("context", "reroute() called with %1% before a push") % path);
        }
    }
    bool rerouted() const{
        if(_routes.size()){
            return _routes.top().rerouted();
        }
        return false;
    }
    std::string alt_path() const{
        if(_routes.size()){
            return _routes.top().rerouted_path();
        }
        return "";
    }
    std::string target() const{
        return _request.target().to_string();
    }
    std::string path() const{
        std::string path; 
        if(rerouted()){
            std::stringstream path_stream(alt_path());
            std::getline(path_stream, path, '?');
        }else{
            std::stringstream path_stream(target());
            std::getline(path_stream, path, '?');
        }
        return path;
    }
    std::string query_string() const{
        std::string path = target();
        std::size_t pos = path.find('?');
        return pos != std::string::npos ? path.substr(pos+1) : std::string();
    }
    const query_parser_type& query() const{
        return _query;
    }
    
    void push(const udho::detail::route& r){
        _routes.push(r);
    }
    udho::detail::route top() const{
        return _routes.top();
    }
    udho::detail::route pop(){
        udho::detail::route last = top();
        _routes.pop();
        return last;
    }
    std::size_t reroutes() const{
        return _routes.size();
    }
};
 
}

/**
 * A Stateful context passed to all callables along with the arguments. 
 * The context is always the first argument to the callable. Even if the callable takes no arguments, it must take the context as the first argument. 
 * A stateful context should be used in callables that need to use session states.
 * 
 * \tparam AuxT bridge between the server and the callable
 * \tparam RequestT HTTP request type
 * \tparam ShadowT the session data structure
 * 
 * \note instead of instantiating this template directly use \ref udho::contexts::stateful
 */
template <typename AuxT, typename RequestT, typename ShadowT>
struct context{
    typedef RequestT                                        request_type;
    typedef ShadowT                                         shadow_type;
    typedef typename shadow_type::key_type                  key_type;
    typedef context<AuxT, request_type, shadow_type>        self_type;
    typedef detail::context_impl<request_type>              impl_type;
    typedef boost::shared_ptr<impl_type>                    pimple_type;
    typedef udho::form_<RequestT>                           form_type;
    typedef udho::cookies_<RequestT>                        cookies_type;
    typedef udho::session_<request_type, shadow_type>       session_type;
    typedef urlencoded_form<std::string::const_iterator>    query_parser_type;
    
    pimple_type  _pimpl;
    session_type _session;
    AuxT&        _aux;
        
    template <typename... V>
    context(AuxT& aux, udho::cache::store<key_type, V...>& store): _pimpl(new impl_type(request_type())), _session(_pimpl->cookies(), store), _aux(aux){}
    template <typename... V>
    context(AuxT& aux, const RequestT& request, udho::cache::shadow<key_type, V...>& shadow): _pimpl(new impl_type(request)), _session(_pimpl->cookies(), shadow), _aux(aux){}
    template <typename OtherShadowT>
    context(context<AuxT, RequestT, OtherShadowT>& other): _pimpl(other._pimpl), _session(other._session), _aux(other._aux){}
    
    const request_type& request() const{return _pimpl->request();}
    
    /**
     * patches a HTTP response with the headers added to the context
     */
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    /**
     * The form may be url enocded or multipart
     * \see udho::form_
     * \see udho::urlencoded_form
     * \see udho::multipart_form
     */
    form_type& form(){
        return _pimpl->_form;
    }
    /**
     * access the HTTP Session
     * \see udho::session_
     */
    session_type& session(){
        return _session;
    }
    /**
     * accesses the HTTP cookies
     * \see udho::cookies_
     */
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    
    /**
     * conversion operator to convert to the corresponding beast request type
     */
    operator request_type() const{
        return _pimpl->request();
    }
    
    /**
     * Logs a message to the logger attached with the server. A logging message can be passed to the `log` method or the `operator<<` can be used to log a message.
     * \code
     * ctx << udho::logging::messages::formatted::debug("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * ctx << udho::logging::messages::formatted::info("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * ctx << udho::logging::messages::formatted::warning("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * ctx << udho::logging::messages::formatted::error("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * \endcode
     * \see udho::logging::message
     */
    template <udho::logging::status Status>
    void log(const udho::logging::message<Status>& msg) const{
        _pimpl->log(msg);
    }
    /**
     * attaches a context with its server counter part 
     * \internal
     */
    template <typename LoggerT, typename CacheT>
    void attach(udho::attachment<AuxT, LoggerT, CacheT>& attachment){
        _pimpl->attach(attachment);
    }
    /**
     * returns a reference to the bridge between the callable and the server
     */
    AuxT& aux(){
        return _aux;
    }
    /**
     * respond with a raw Beast HTTP response.
     */
    void respond(udho::defs::response_type& response){
        _pimpl->respond(response);
    }
   /**
     * the responded output will be put inside a beast HTTP response object and a content type header of type mime will be attached
     */
    template <typename OutputT>
    void respond(const OutputT& output, const std::string& mime){
        udho::compositors::mimed<OutputT> compositor(mime);
        udho::defs::response_type response = compositor(*this, output);
        respond(response);
    }
    /**
     * respond with a http status. The responded output will be put inside a beast HTTP response object and a content type header of type mime will be attached
     */
    template <typename OutputT>
    void respond(boost::beast::http::status s, const OutputT& output, const std::string& mime){
        status(s);
        respond<OutputT>(output, mime);
    }
    /**
     * set a status code for the HTTP response
     */
    void status(boost::beast::http::status s){
        _pimpl->status(s);
    }
    /**
     * target of the HTTP request including the `?` if the request includes get parameters
     * \code
     * // https://localhost/user/profile?id=245
     * ctx.target() // /user/profile?id=245
     * \endcode
     */
    std::string target() const{
        return _pimpl->target();
    }
    /**
     * path of the HTTP request before `?` if any
     * \code
     * // https://localhost/user/profile?id=245
     * ctx.path() // /user/profile
     * \endcode
     */
    std::string path() const{
        return _pimpl->path();
    }
    /**
     * The get query of the HTTP request.
     * \code
     * if(!ctx.query().has("type") || ctx.query().field<std::string>("type") == "json"){
     *     // respond with JSON content
     * }else if(ctx.query().field<std::string>("type") == "xml")
     *     // respond with xml content
     * }
     * \endcode
     * \see udho::urlencoded_form
     */
    const query_parser_type& query() const{
        return _pimpl->query();
    }
    
    void clear(){
        _pimpl->clear();
    }
    /**
     * Internally reroute an HTTP request to another request
     */
    void reroute(const std::string& path){
        _pimpl->reroute(path);
    }
    /**
     * check whether this is a rerouted request or not
     */
    bool rerouted() const{
        return _pimpl->rerouted();
    }
    std::string alt_path() const{
        return _pimpl->alt_path();
    }
    /**
     * render a file in path
     */
    std::string render(const std::string& path) const{
        return _aux.render(path);
    }
    /**
     * render a template in path
     */
    template <typename... DataT>
    std::string render(const std::string& path, const DataT&... data) const{
        return _aux.render(path, *this, data...);
    }
    
    void push(const udho::detail::route& r){
        _pimpl->push(r);
    }
    udho::detail::route top() const{
        return _pimpl->top();
    }
    udho::detail::route pop(){
        return _pimpl->pop();
    }
    std::size_t reroutes() const{
        return _pimpl->reroutes();
    }
};

/**
 * A Stateless context passed to all callables along with the arguments. 
 * he context is always the first argument to the callable. Even if the callable takes no arguments, it must take the context as the first argument. 
 * A stateless context should be used in callables that need not to use session states.
 * 
 * \tparam AuxT bridge between the server and the callable
 * \tparam RequestT HTTP request type
 * 
 * \note instead of instantiating this template directly use \ref udho::contexts::stateless
 */
template <typename AuxT, typename RequestT>
struct context<AuxT, RequestT, void>{
    typedef RequestT                                        request_type;
    typedef void                                            shadow_type;
    typedef context<AuxT, request_type, void>               self_type;
    typedef detail::context_impl<request_type>              impl_type;
    typedef boost::shared_ptr<impl_type>                    pimple_type;
    typedef udho::form_<RequestT>                           form_type;
    typedef udho::cookies_<RequestT>                        cookies_type;
    typedef urlencoded_form<std::string::const_iterator>    query_parser_type;
    
    pimple_type _pimpl;
    AuxT&       _aux;
    
    template <typename C>
    context(AuxT& aux, const RequestT& request, const C&): _pimpl(new impl_type(request)), _aux(aux){}
    template <typename ShadowT>
    context(context<AuxT, RequestT, ShadowT>& other): _pimpl(other._pimpl), _aux(other._aux){}
    
    /**
     * returns the boost beast HTTP request
     */
    const request_type& request() const{return _pimpl->request();}
    /**
     * patches the response with the headers added in the context from inside the callable
     */
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    /**
     * returns the form handler object.
     * \see udho::form_
     * \see udho::urlencoded_form
     * \see udho::multipart_form
     */
    form_type& form(){
        return _pimpl->_form;
    }
    /**
     * accesses the HTTP cookies
     * \see udho::cookies_
     */
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    /**
     * conversion operator to convert to the corresponding beast request type
     */
    operator request_type() const{
        return _pimpl->request();
    }
    /**
     * Logs a message to the logger attached with the server. A logging message can be passed to the `log` method or the `operator<<` can be used to log a message.
     * \code
     * ctx << udho::logging::messages::formatted::debug("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * ctx << udho::logging::messages::formatted::info("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * ctx << udho::logging::messages::formatted::warning("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * ctx << udho::logging::messages::formatted::error("data", "testing log functionality of %1% Hi %2%") % "Neel Basu" % 42;
     * \endcode
     * \see udho::logging::message
     */
    template <udho::logging::status Status>
    void log(const udho::logging::message<Status>& msg) const{
        _pimpl->log(msg);
    }
    /**
     * attaches a context with its server counter part 
     * \internal
     */
    template <typename AttachmentT>
    void attach(AttachmentT& attachment){
        _pimpl->attach(attachment);
    }
    /**
     * returns a reference to the bridge between the callable and the server
     */
    AuxT& aux(){
        return _aux;
    }
    /**
     * respond with a http status. The responded output will be put inside a beast HTTP response object and a content type header of type mime will be attached
     */
    void respond(udho::defs::response_type& response){
        _pimpl->respond(response);
    }
    /**
     * the responded output will be put inside a beast HTTP response object and a content type header of type mime will be attached
     */
    template <typename OutputT>
    void respond(const OutputT& output, const std::string& mime){
        udho::compositors::mimed<OutputT> compositor(mime);
        udho::defs::response_type response = compositor(*this, output);
        respond(response);
    }
    /**
     * respond with a http status. The responded output will be put inside a beast HTTP response object and a content type header of type mime will be attached
     */
    template <typename OutputT>
    void respond(boost::beast::http::status s, const OutputT& output, const std::string& mime){
        status(s);
        respond<OutputT>(output, mime);
    }
    /**
     * set a status code for the HTTP response
     */
    void status(boost::beast::http::status s){
        _pimpl->status(s);
    }
    /**
     * target of the HTTP request including the `?` if the request includes get parameters
     * \code
     * // https://localhost/user/profile?id=245
     * ctx.target() // /user/profile?id=245
     * \endcode
     */
    std::string target() const{
        return _pimpl->target();
    }
    /**
     * path of the HTTP request before `?` if any
     * \code
     * // https://localhost/user/profile?id=245
     * ctx.path() // /user/profile
     * \endcode
     */
    std::string path() const{
        return _pimpl->path();
    }
    /**
     * The get query of the HTTP request.
     * \code
     * if(!ctx.query().has("type") || ctx.query().field<std::string>("type") == "json"){
     *     // respond with JSON content
     * }else if(ctx.query().field<std::string>("type") == "xml")
     *     // respond with xml content
     * }
     * \endcode
     * \see udho::urlencoded_form
     */
    const query_parser_type& query() const{
        return _pimpl->query();
    }
    
    void clear(){
        _pimpl->clear();
    }
    /**
     * Internally reroute an HTTP request to another request
     */
    void reroute(const std::string& path){
        _pimpl->reroute(path);
    }
    /**
     * check whether this is a rerouted request or not
     */
    bool rerouted() const{
        return _pimpl->rerouted();
    }
    std::string alt_path() const{
        return _pimpl->alt_path();
    }
    /**
     * render a file in path
     */
    std::string render(const std::string& path) const{
        return _aux.render(path);
    }
    /**
     * render a template in path
     */
    template <typename... DataT>
    std::string render(const std::string& path, const DataT&... data) const{
        return _aux.render(path, *this, data...);
    }
    
    void push(const udho::detail::route& r){
        _pimpl->push(r);
    }
    udho::detail::route top() const{
        return _pimpl->top();
    }
    udho::detail::route pop(){
        return _pimpl->pop();
    }
    std::size_t reroutes() const{
        return _pimpl->reroutes();
    }
};

template <typename AuxT, typename RequestT, typename ShadowT, udho::logging::status Status>
const context<AuxT, RequestT, ShadowT>& operator<<(const context<AuxT, RequestT, ShadowT>& ctx, const udho::logging::message<Status>& msg){
    ctx.log(msg);
    return ctx;
}

}

#endif // UDHO_CONTEXT_H
