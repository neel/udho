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
#include <udho/bridge.h>
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
    
    const request_type& _request;
    form_type           _form;
    std::string         _query_string;
    query_parser_type   _query;
    headers_type        _headers;
    cookies_type        _cookies;
    
    boost::beast::http::status _status;
    std::string                _alt_path;
    std::string               _pattern;
    
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
    void log(const udho::logging::messages::error& msg){
        _error(msg);
    }
    void log(const udho::logging::messages::warning& msg){
        _warning(msg);
    }
    void log(const udho::logging::messages::info& msg){
        _info(msg);
    }
    void log(const udho::logging::messages::debug& msg){
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
        _alt_path = "";
        _pattern  = "";
    }
    void reroute(const std::string& path){
        _alt_path = path;
    }
    bool rerouted() const{
        return !_alt_path.empty();
    }
    std::string alt_path() const{
        return _alt_path;
    }
    void pattern(const std::string& p){
        _pattern = p;
    }
    std::string pattern() const{
        return _pattern;
    }
    std::string target() const{
        return _request.target().to_string();
    }
    std::string path() const{
        std::string path;
        std::stringstream path_stream(target());
        std::getline(path_stream, path, '?');
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
};
 
}

/**
 * @todo write docs
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
    
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    form_type& form(){
        return _pimpl->_form;
    }
    session_type& session(){
        return _session;
    }
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    
    operator request_type() const{
        return _pimpl->request();
    }
    template <udho::logging::status Status>
    void log(const udho::logging::message<Status>& msg){
        _pimpl->log(msg);
    }
    template <typename LoggerT, typename CacheT>
    void attach(udho::attachment<AuxT, LoggerT, CacheT>& attachment){
        _pimpl->attach(attachment);
    }
    AuxT& aux(){
        return _aux;
    }
    void respond(udho::defs::response_type& response){
        _pimpl->respond(response);
    }
    template <typename OutputT>
    void respond(const OutputT& output, const std::string& mime){
        udho::compositors::mimed<OutputT> compositor(mime);
        udho::defs::response_type response = compositor(*this, output);
        respond(response);
    }
    
    std::string target() const{
        return _pimpl->target();
    }
    std::string path() const{
        return _pimpl->path();
    }
    const query_parser_type& query() const{
        return _pimpl->query();
    }
    
    void clear(){
        _pimpl->clear();
    }
    void reroute(const std::string& path){
        _pimpl->reroute(path);
    }
    bool rerouted() const{
        return _pimpl->rerouted();
    }
    std::string alt_path() const{
        return _pimpl->alt_path();
    }
    void pattern(const std::string& p){
        _pimpl->pattern(p);
    }
    std::string pattern() const{
        return _pimpl->pattern();
    }
};

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
    
    const request_type& request() const{return _pimpl->request();}
    
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    form_type& form(){
        return _pimpl->_form;
    }
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    
    operator request_type() const{
        return _pimpl->request();
    }
    template <udho::logging::status Status>
    void log(const udho::logging::message<Status>& msg){
        _pimpl->log(msg);
    }
    template <typename AttachmentT>
    void attach(AttachmentT& attachment){
        _pimpl->attach(attachment);
    }
    AuxT& aux(){
        return _aux;
    }
    void respond(udho::defs::response_type& response){
        _pimpl->respond(response);
    }
    template <typename OutputT>
    void respond(const OutputT& output, const std::string& mime){
        udho::compositors::mimed<OutputT> compositor(mime);
        udho::defs::response_type response = compositor(*this, output);
        respond(response);
    }
    
    std::string target() const{
        return _pimpl->target();
    }
    std::string path() const{
        return _pimpl->path();
    }
    const query_parser_type& query() const{
        return _pimpl->query();
    }
    
    void clear(){
        _pimpl->clear();
    }
    void reroute(const std::string& path){
        _pimpl->reroute(path);
    }
    bool rerouted() const{
        return _pimpl->rerouted();
    }
    std::string alt_path() const{
        return _pimpl->alt_path();
    }
    void pattern(const std::string& p){
        _pimpl->pattern(p);
    }
    std::string pattern() const{
        return _pimpl->pattern();
    }
};

template <typename AuxT, typename RequestT, typename ShadowT, udho::logging::status Status>
context<AuxT, RequestT, ShadowT>& operator<<(context<AuxT, RequestT, ShadowT>& ctx, const udho::logging::message<Status>& msg){
    ctx.log(msg);
    return ctx;
}

namespace contexts{
    using stateless = context<udho::bridge, udho::defs::request_type, void>;
    template <typename... T>
    using stateful = context<udho::bridge, udho::defs::request_type, udho::cache::shadow<udho::defs::session_key_type, T...>>;
}

}

#endif // UDHO_CONTEXT_H
