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

#ifndef UDHO_ATTACHMENT_H
#define UDHO_ATTACHMENT_H

#include <udho/logging.h>

namespace udho{

/**
 * logged stateful
 */
template <typename AuxT, typename LoggerT=void, typename CacheT=void>
struct attachment: LoggerT, CacheT{
    typedef attachment<AuxT, LoggerT, CacheT> self_type;
    typedef AuxT auxiliary_type;
    typedef LoggerT logger_type;
    typedef CacheT  cache_type;
    typedef typename cache_type::shadow_type shadow_type;
    
    shadow_type _shadow;
    AuxT _aux;
    
    attachment(LoggerT& logger): LoggerT(logger), _shadow(*this){}
    shadow_type& shadow(){
        return _shadow;
    }
    AuxT& aux(){
        return _aux;
    }
};

/**
 * quiet stateful
 */
template <typename AuxT, typename CacheT>
struct attachment<AuxT, void, CacheT>: CacheT{
    typedef attachment<AuxT, void, CacheT> self_type;
    typedef AuxT auxiliary_type;
    typedef void logger_type;
    typedef CacheT  cache_type;
    typedef typename cache_type::shadow_type shadow_type;
    
    shadow_type _shadow;
    AuxT _aux;
    
    attachment(): _shadow(*this){}
    template <udho::logging::status Status>
    self_type& operator()(const udho::logging::message<Status>& /*msg*/){
        return *this;
    }
    shadow_type& shadow(){
        return _shadow;
    }
    AuxT& aux(){
        return _aux;
    }
};

/**
 * logged stateless
 */
template <typename AuxT, typename LoggerT>
struct attachment<AuxT, LoggerT, void>: LoggerT{
    typedef attachment<AuxT, LoggerT, void> self_type;
    typedef AuxT auxiliary_type;
    typedef LoggerT logger_type;
    typedef void shadow_type;
    
    AuxT _aux;
    
    attachment(LoggerT& logger): LoggerT(logger){}
    int shadow(){
        return 0;
    }
    AuxT& aux(){
        return _aux;
    }
};

/**
 * quiet stateless
 */
template <typename AuxT>
struct attachment<AuxT, void, void>{
    typedef attachment<AuxT, void, void> self_type;
    typedef AuxT auxiliary_type;
    typedef void logger_type;
    typedef void cache_type;
    typedef void shadow_type;
    
    AuxT _aux;
    
    attachment(){}
    template <udho::logging::status Status>
    self_type& operator()(const udho::logging::message<Status>& /*msg*/){
        return *this;
    }
    int shadow(){
        return 0;
    }
    AuxT& aux(){
        return _aux;
    }
};

template <typename AuxT, typename LoggerT, typename CacheT, udho::logging::status Status>
attachment<AuxT, LoggerT, CacheT>& operator<<(attachment<AuxT, LoggerT, CacheT>& attachment, const udho::logging::message<Status>& msg){
    attachment(msg);
    return attachment;
}

}

#endif // UDHO_ATTACHMENT_H
