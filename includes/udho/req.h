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

#ifndef UDHO_REQ_H
#define UDHO_REQ_H

namespace udho{

template <typename LoggerT=void, typename CacheT=void, typename... T>
struct attachment: LoggerT, CacheT, T...{
    typedef attachment<LoggerT, CacheT, T...> self_type;
    typedef LoggerT logger_type;
    typedef CacheT  cache_type;
    
    attachment(LoggerT& logger, CacheT& cache): LoggerT(logger), CacheT(cache){}
};

template <typename LoggerT>
struct attachment<LoggerT, void>: LoggerT{
    typedef attachment<LoggerT, void> self_type;
    typedef LoggerT logger_type;
    typedef void    cache_type;
    
    attachment(LoggerT& logger): LoggerT(logger){}
};
    
/**
 * @todo write docs
 */
template <typename RequestT, typename AttachmentT>
struct req: RequestT, AttachmentT{
    typedef req<RequestT, AttachmentT> self_type;
    
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
        
    req(attachment_type& attachment): request_type(), attachment_type(attachment){}
    req(const RequestT& request, attachment_type& attachment): request_type(request), attachment_type(attachment){}
    self_type& operator=(const self_type& other){
        request_type::operator=(other);
        
        return *this;
    }
    self_type& operator=(const request_type& other){
        request_type::operator=(other);
        
        return *this;
    }
};

}

#endif // UDHO_REQ_H
