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

#include <udho/bridge.h>
#include <udho/util.h>
#include <udho/page.h>

void udho::bridge::docroot(const boost::filesystem::path& path){
    _docroot = path;
}

boost::filesystem::path udho::bridge::docroot() const{
    return _docroot;
}

std::string udho::bridge::contents(const std::string& path) const{
    boost::filesystem::path local_path = _docroot / path;
    std::ifstream ifs(local_path.c_str());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    
    return content;
}

boost::beast::http::response<boost::beast::http::file_body> udho::bridge::file(const std::string& path, const udho::defs::request_type& req, std::string mime) const{
    boost::filesystem::path local_path = _docroot / path;
    boost::beast::error_code err;
    boost::beast::http::file_body::value_type body;
    body.open(local_path.c_str(), boost::beast::file_mode::scan, err);
    if(err == boost::system::errc::no_such_file_or_directory){
        throw exceptions::http_error(boost::beast::http::status::not_found, path);
    }
    if(err){
        throw exceptions::http_error(boost::beast::http::status::internal_server_error, path);
    }
    auto const size = body.size();
    boost::beast::http::response<boost::beast::http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, req.version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, !mime.empty() ? mime : udho::internal::mime_type(local_path.c_str()));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

