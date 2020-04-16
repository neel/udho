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

namespace udho{
/**
 * @todo write docs
 */
struct bridge{
    boost::filesystem::path _docroot;
    
    void docroot(const boost::filesystem::path& path);
    boost::filesystem::path docroot() const;
    
    std::string contents(const std::string& path) const;
    boost::beast::http::response<boost::beast::http::file_body> file(const std::string& path, const ::udho::defs::request_type& req, std::string mime = "") const;
    
    template <typename GroupT>
    std::string render(const std::string& path, ::udho::lookup_table<GroupT>& scope) const{
        std::string template_contents = contents(path);
        auto parser = ::udho::view::parse_xml(scope, contents);
        return parser.output();
    }
    template <typename DataT>
    std::string render(const std::string& path, ::udho::prepared<DataT>& data) const{
        auto scope = ::udho::scope(data);
        return render(path, scope);
    }
};

}

#endif // UDHO_BRIDGE_H
