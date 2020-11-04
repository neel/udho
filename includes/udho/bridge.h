/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_BRIDGE_H
#define UDHO_BRIDGE_H

#include <string>
#include <boost/filesystem.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/file_body.hpp>
#include <udho/defs.h>
#include <udho/access.h>
#include <udho/scope.h>
#include <udho/parser.h>
#include <udho/util.h>
#include <udho/page.h>
#include <udho/context.h>
#include <udho/configuration.h>
#include <udho/client.h>
#include <udho/url.h>

namespace udho{
    
/**
 * @todo write docs
 * \ingroup server
 */
template <typename ConfigT>
struct bridge{
    typedef ConfigT configuration_type;
    typedef bridge<ConfigT> self_type;
    
    boost::asio::io_service& _io;
    configuration_type _config; 

    bridge(boost::asio::io_service& io): _io(io){}
    
    configuration_type& config(){
        return _config;
    }
    const configuration_type& config() const{
        return _config;
    }
    boost::filesystem::path docroot() const{
        return _config[udho::configs::server::document_root];
    }
    boost::filesystem::path tmplroot() const{
        return _config[udho::configs::server::template_root];
    }
    std::string contents(const boost::filesystem::path& local_path) const{
        std::ifstream ifs(local_path.c_str());
        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        return content;
    }
    boost::beast::http::response<boost::beast::http::file_body> file(const std::string& path, const ::udho::defs::request_type& req, std::string mime = "") const{
        boost::filesystem::path doc_root = docroot();
        boost::filesystem::path local_path = internal::path_cat(doc_root, path);
        if(!internal::path_inside(doc_root, local_path)){
            throw exceptions::http_error(boost::beast::http::status::forbidden, (boost::format("Access denied to %1%") % local_path).str());
        }
        std::string extension = local_path.extension().string();
        std::string mime_type = _config[udho::configs::server::mime_default];
        if(!extension.empty() && extension.front() == '.'){
            extension = extension.substr(1);
            mime_type = _config[udho::configs::server::mimes].at(extension);
        }
        boost::beast::error_code err;
        boost::beast::http::file_body::value_type body;
        body.open(local_path.c_str(), boost::beast::file_mode::scan, err);
        if(err == boost::system::errc::no_such_file_or_directory){
            throw udho::exceptions::http_error(boost::beast::http::status::not_found, (boost::format("File `%1%` not found in disk") % local_path).str());
        }
        if(err){
            throw udho::exceptions::http_error(boost::beast::http::status::internal_server_error, (boost::format("Error %1% while reading file `%2%` from disk") % err % local_path).str());
        }
        auto const size = body.size();
        boost::beast::http::response<boost::beast::http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, req.version())};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, !mime.empty() ? mime : mime_type);
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }
    
    template <typename RequestT, typename ShadowT, typename GroupT>
    std::string render(const std::string& path, const udho::context<self_type, RequestT, ShadowT>& ctx, ::udho::lookup_table<GroupT>& scope) const{
        std::string template_contents = render(path);
        auto processor = ::udho::view::processor(scope, ctx);
        return processor.process(template_contents);
    }
    template <typename RequestT, typename ShadowT, typename DataT>
    std::string render(const std::string& path, const udho::context<self_type, RequestT, ShadowT>& ctx, ::udho::prepared<DataT>& data) const{
        auto scope = ::udho::scope(data);
        return render(path, ctx, scope);
    }
    template <typename RequestT, typename ShadowT, typename U, typename V>
    std::string render(const std::string& path, const udho::context<self_type, RequestT, ShadowT>& ctx, ::udho::prepared_group<U, V>& group) const{
        auto scope = ::udho::scope(group);
        return render(path, ctx, scope);
    }
    template <typename RequestT, typename ShadowT, typename... DataT>
    std::string render(const std::string& path, const udho::context<self_type, RequestT, ShadowT>& ctx, const DataT&... data) const{
        auto prepared = udho::data(data...);
        auto scope = ::udho::scope(prepared);
        return render(path, ctx, scope);
    }
    std::string render(const std::string& path) const{
        boost::filesystem::path tmpl_root  = tmplroot();
        boost::filesystem::path local_path = tmpl_root / path;
        return contents(local_path);
    }
    
    template <typename ContextT>
    detail::client_connection_wrapper<ContextT> client(ContextT ctx, udho::config<udho::client_options> options){
        return detail::client_connection_wrapper<ContextT>(_io, ctx, options);
    }
};

}

#endif // UDHO_BRIDGE_H
