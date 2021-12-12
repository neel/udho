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

/**
 * @brief traits of an activity
 * A trait X is generally defined in the `udho::db::pg::traits` namespace. The trait is defined
 * as a template that takes the Activity type (`ActivityT`) as a template parameter. The trait 
 * `X<ActivityT>` must specify a typedef named `value_type` and `constexpr static const` variable
 * named `value` of type `value_type` which will be used for the purpose for which the trait is
 * designed.
 * @code
 * template <typename ActivityT>
 * struct X{
 *     using value_type = <user defined type>
 *     constexpr static const value_type value = <user defined value>;
 * };
 * @endcode 
 * To specializae the behaviour for a trait it is not necessary to specialize `X`. A function
 * with signature `X<ActivityT>::value_type trait(const X<ActivityT>&)` is sufficient for the
 * ADL to resolve the behaviour of `X` specialized for `ActivityT`. The `trait` function should
 * return a value of type `X<ActivityT>::value_type`.
 * @see traits::error_code
 * @see traits::allow_empty
 * @ingroup pg
 */
namespace traits{

    namespace detail{
        /**
         * @brief specifies whether an activity success result is allowed to be empty or not
         * It is used by the default implementation of the allow_empty trait. By default it 
         * considers that every activity result type allows an empty result. However the 
         * specialization for `db::result` enforces that the success result must not be empty.
         * This is because a model uses `db::result` result type when it expects a single row
         * response from the database.
         * @tparam ResultT 
         */
        template <typename ResultT>
        struct empty_allowed{ enum { value = true }; };
        /**
         * @brief The specialization for db::result does not allow any empty result.
         * This is because a model uses `db::result` result type when it expects a single row
         * response from the database.
         * @tparam DataT 
         */
        template <typename DataT>
        struct empty_allowed<db::result<DataT>>{ enum { value = false }; };

        /**
         * @brief Default empty checker always returns false
         * It is used by the default implementation of the allow_empty trait whenever the
         * `detail::empty_allowed<ActivityT::success_type>::value` returns true for the given `ActivityT`.
         * @note The `ResultT` template parameter is the `ActivityT::success_type`
         * @tparam ResultT 
         */
        template <typename ResultT>
        struct check_empty{
            bool operator()(const ResultT&) const { return false; }
        };
        template <typename ResultT>
        bool is_empty(const ResultT& res){
            check_empty<ResultT> empty_checker;
            return empty_checker(res);
        }
        /**
         * @brief Empty checker specialized for `db::result` checks whether the result is empty or not.
         * 
         * @tparam DataT 
         */
        template <typename DataT>
        struct check_empty<db::result<DataT>>{
            bool operator()(const db::result<DataT>& res) const { return res.empty(); }
        };
        template <typename DataT>
        struct check_empty<db::results<DataT>>{
            bool operator()(const db::results<DataT>& res) const { return res.empty(); }
        };

        /**
         * @brief Checks whether a trait is specified for an activity or not.
         * For example `specified<traits::error_code<ActivityT>>::value` is true if there exists a function
         * `trait` function that takes an argument of type `const traits::error_code<ActivityT>&` for the
         * specified activity type `ActivityT`. Otherwise `specified<traits::error_code<ActivityT>>::value`
         * is false.
         * @tparam TraitT 
         * @see adl_trait
         */
        template<typename TraitT, typename = void>
        struct specified: std::false_type {};

        template<typename TraitT>
        struct specified<TraitT, std::void_t< decltype( trait(TraitT{}) ) >>: std::true_type {};

        /**
         * @brief Returns an appropriate value for the provided trait, either by using the user provided
         * `trait` method through ADL or using the value provided by the default implementation of the trait.
         * 
         * @tparam TraitT 
         * @param t 
         * @return TraitT::value_type 
         */
        template <typename TraitT, std::enable_if_t<specified<TraitT>::value, bool> = true>
        typename TraitT::value_type adl_trait(const TraitT& t){
            return trait(t);
        }

        template <typename TraitT, std::enable_if_t<!specified<TraitT>::value, bool> = true>
        typename TraitT::value_type adl_trait(const TraitT&){
            return TraitT::value;
        }
    }

    /**
     * @brief The trait specifies an HTTP error code which is to be used in case of an empty response.
     * By default it uses Not Found as the value for all activities. The behaviour may be changed for 
     * individual activities by specializing.
     * @tparam ActivityT 
     */
    template <typename ActivityT>
    struct error_code{
        using value_type = boost::beast::http::status;
        constexpr static const boost::beast::http::status value = boost::beast::http::status::not_found;
    };
    /**
     * @brief The trait specifies a boolean value indicating whether empty result is allowed by the activity or not.
     * By default it uses the the `detail::empty_allowed<ActivityT::success_type>` to check whether the 
     * success result type is expected to be empty or not. `ActivityT::success_type` is often either of 
     * `db::result` or `db::results`. The default implementation of the  template `detail::empty_allowed`
     * considers every activity to be allowing empty results. However the specialization for `db::result` 
     * does not alow empty result, because `db::result` implies the model expects a single row response.
     * @tparam ActivityT 
     */
    template <typename ActivityT>
    struct allow_empty{
        using value_type = bool;
        constexpr static const bool value = detail::empty_allowed<typename ActivityT::success_type>::value;
    };

}


#define PG_ALLOW_EMPTY(ActivityT, Allowed) inline bool trait(const udho::db::pg::traits::allow_empty<ActivityT>&) { return Allowed; }
#define PG_ERROR_CODE(ActivityT, ErrorC)   inline boost::beast::http::status trait(const udho::db::pg::traits::error_code<ActivityT>&) { return ErrorC; }

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
            return true;
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
            return true;
        }
        private:
            udho::contexts::stateless _ctx;
    };

    /**
     * @brief Specialize to implement custom validations on success result of a pg activity.
     * The specialization must provide an operator() overload that takes the success result type 
     * of the activity and returns a boolean value indicating whether the result is accepted or 
     * rejected. A true return value implies that the result is accepted as valid. And a false 
     * return value indicates an invalid success result.
     * @note The default implementation checks the `traits::allow_empty` trait for the activity.
     *       If empty result is NOT allowed for the success result type then uses the default empty 
     *       checker `traits::detail::check_empty` to check whether the success result is considered
     *       as empty or not.
     * @ingroup pg 
     */
    template <typename ActivityT>
    struct invalidate{
        /**
        * By default cancels none, however if the activity s[pecifies that it does not allow empty result
        * returns true (suggests cancellation) if the result is empty.
        */
        bool operator()(const typename ActivityT::success_type& res){
            if(!traits::detail::adl_trait(traits::allow_empty<ActivityT>{})){
                return traits::detail::is_empty(res);
            }
            return false;
        }
    };
    
}
}
}
}

#endif // UDHO_DB_PG_ON_H
