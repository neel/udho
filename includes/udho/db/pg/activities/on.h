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

#include <udho/db/common/none.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <udho/contexts.h>
#include <udho/page.h>
#include <udho/db/pg/activities/failure.h>
#include <udho/db/common/results.h>
#include <udho/db/common/result.h>

namespace udho{
namespace db{
namespace pg{
  
namespace traits{

namespace detail{
    template <typename ResultT>
    struct empty_allowed{ enum { value = true }; };
    template <typename DataT>
    struct empty_allowed<db::result<DataT>>{ enum { value = false }; };

    /**
    * @brief Default empty checker always returns false
    * 
    * @tparam ResultT 
    */
    template <typename ResultT>
    struct check_empty{
        bool operator()(const ResultT&) const { return false; }
    };

    template <typename DataT>
    struct check_empty<db::result<DataT>>{
        bool operator()(const db::result<DataT>& res) const { return res.empty(); }
    };
    template <typename DataT>
    struct check_empty<db::results<DataT>>{
        bool operator()(const db::results<DataT>& res) const { return res.empty(); }
    };

    template<typename TraitT, typename = void>
    struct specified: std::false_type {};

    template<typename TraitT>
    struct specified<TraitT, std::void_t< decltype( trait(TraitT{}) ) >>: std::true_type {};

    template <typename TraitT, std::enable_if_t<specified<TraitT>::value, bool> = true>
    typename TraitT::value_type adl_trait(const TraitT& t){
        return trait(t);
    }

    template <typename TraitT, std::enable_if_t<!specified<TraitT>::value, bool> = true>
    typename TraitT::value_type adl_trait(const TraitT&){
        return TraitT::value;
    }
}

template <typename ActivityT>
struct error_code{
    using value_type = boost::beast::http::status;
    constexpr static const boost::beast::http::status value = boost::beast::http::status::not_found;
};

template <typename ActivityT>
struct allow_empty{
    using value_type = bool;
    constexpr static const bool value = detail::empty_allowed<typename ActivityT::success_type>::value;
};

}

#define PG_ALLOW_EMPTY(ActivityT, Allowed) bool trait(const udho::db::pg::traits::allow_empty<ActivityT>&) { return Allowed; }
#define PG_ERROR_CODE(ActivityT, ErrorC)   boost::beast::http::status trait(const udho::db::pg::traits::error_code<ActivityT>&) { return ErrorC; }

namespace on{
    
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
    bool operator()(const pg::failure& f){
        std::string message = f.error.message() + f.reason;
        _ctx << pg::exception(f, message);
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
        boost::beast::http::status status = traits::detail::adl_trait(traits::error_code<ActivityT>{});
        _ctx << udho::exceptions::http_error(status);
        return false;
    }
    private:
        udho::contexts::stateless _ctx;
};

/**
 * Sppecialize to implement custom validations on success result.
 */
template <typename ActivityT>
struct validate{
    /**
     * disqualifies none
     */
    bool operator()(const typename ActivityT::success_type& res){
        if(traits::detail::adl_trait(traits::allow_empty<ActivityT>{})){
            traits::detail::check_empty<typename ActivityT::success_type> empty_checker;
            return empty_checker(res);
        }
        return false;
    }
};
    
}
}
}
}

#endif // UDHO_DB_PG_ON_H
