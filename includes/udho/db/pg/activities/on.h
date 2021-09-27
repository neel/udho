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

#ifndef UDHO_DB_PG_ON_H
#define UDHO_DB_PG_ON_H

#include <boost/beast/http/message.hpp>
#include <udho/contexts.h>
#include <udho/page.h>

namespace udho{
namespace db{
namespace pg{

struct failure;
  
namespace on{
    
namespace http = boost::beast::http;
    
/**
 * Specialize this template to customize error message on SQL failure
 */
template <typename ActivityT>
struct failure{
    /**
     * Initialized by any context, which is stored as a stateless context.
     * Default status is initialized as internel server error.
     */
    template <typename ContextT>
    failure(ContextT ctx): _ctx(ctx){}
    /**
     * The parenthesis operator is called to throw HTTP errors
     */
    bool operator()(const pg::failure& failure){
        auto cb = ActivityT::on::failure(_ctx);
        cb(failure);
        return false;
    }
    private:
        udho::contexts::stateless _ctx;
};

/**
 * Specialize this template to customize error message on error (unexpected response from SQL query)
 */
template <typename ActivityT>
struct error{
    /**
     * Initialized by any context, which is stored as a stateless context.
     * Default status is initialized as internel server error.
     */
    template <typename ContextT>
    error(ContextT ctx): _ctx(ctx){}
    /**
     * The parenthesis operator is called to throw HTTP errors
     */
    bool operator()(const typename ActivityT::success_type& d){
        auto cb = ActivityT::on::error(_ctx);
        cb(d);
        return false;
    }
    private:
        udho::contexts::stateless _ctx;
};

/**
 * Sppecialize to implement custom validations on success result.
 * \code
 * namespace pg{namespace on{
 * 
 * template <>
 * struct validate<ActivityX>: unless::blank<ActivityX>{
 *     using blank::operator();
 * };
 * 
 * }}
 * \endcode
 */
template <typename ActivityT>
struct validate{
    /**
     * disqualifies none
     */
    bool operator()(const typename ActivityT::success_type&){
        return false;
    }
};

#define PG_ACTIVITY_VALID_UNLESS_BLANK(ActivityX)                 \
    namespace pg{namespace on{                                    \
        template <>                                               \
        struct validate<ActivityX>: unless::blank<ActivityX>{     \
            using blank::operator();                              \
        };                                                        \
    }}                                                            \
        

namespace unless{
    
/**
 * Expects at least one record in the resultset.
 */
template <typename ActivityT>
struct blank{
    /**
     * returns true (invalid) if empty.
     */
    bool operator()(const typename ActivityT::success_type& res){
        return res.empty();
    }
};
    
}
    
}
}
}
}

#endif // UDHO_DB_PG_ON_H
