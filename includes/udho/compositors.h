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

#ifndef UDHO_COMPOSITORS_H
#define UDHO_COMPOSITORS_H

#include <string>
#include <boost/beast/http/message.hpp>
#include <boost/format.hpp>

namespace udho{
    
namespace compositors{
    template <typename OutputT>
    struct transparent{
        typedef OutputT response_type;
        
        template <typename ContextT>
        response_type&& operator()(const ContextT& /*ctx*/, OutputT&& out){
            return std::move(out);
        }
        std::string name() const{
            return "UNSPECIFIED";
        }
    };
    template <typename OutputT>
    struct deferred{
        typedef OutputT response_type;
        
        template <typename... T>
        void operator()(T...){}
        std::string name() const{
            return "DEFERRED";
        }
    };

    template <typename OutputT>
    struct mimed{
        typedef boost::beast::http::response<boost::beast::http::string_body> response_type;
        std::string _mime;
        
        mimed(const std::string& mime): _mime(mime){}
        template <typename ContextT>
        response_type operator()(const ContextT& ctx, const OutputT& out){
            std::string content = boost::lexical_cast<std::string>(out);
            response_type res{boost::beast::http::status::ok, ctx.request().version()};
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(boost::beast::http::field::content_type,   _mime);
            res.set(boost::beast::http::field::content_length, content.size());
            res.keep_alive(ctx.request().keep_alive());
            res.body() = content;
            res.prepare_payload();
            return res;
        }
        std::string name() const{
            return (boost::format("MIMED %1%") % _mime).str();
        }
    };
}
    
}

#endif // COMPOSITORS_H
