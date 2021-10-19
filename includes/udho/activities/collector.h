/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
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

#ifndef UDHO_ACTIVITIES_COLLECTOR_H
#define UDHO_ACTIVITIES_COLLECTOR_H

#include <string>
#include <memory>
#include <udho/hazo/node.h>
#include <udho/activities/detail.h>
#include <udho/activities/fwd.h>

namespace udho{
/**
 * @ingroup activities
 */
namespace activities{

/**
 * @brief Collects data associated with all activities involved in the subtask graph
 * For storage the @ref hazo::node is used internally which expects each ActivityT to 
 * have typedef ActivityT::result_type which is and instance of result_data<SuccessT, FailureT>
 * where SuccessT and FailureT denotes success and failure types associated with ActivityT.
 * Collector extends the lifetime of HTTP request by copying the context object. 
 * @ingroup data
 * @tparam ContextT 
 * @tparam T ... Activities in the chains
 */
template <typename ContextT, typename... T>
struct collector: udho::hazo::node<detail::labeled<T, typename T::result_type>...>, std::enable_shared_from_this<collector<ContextT, T...>>{
    typedef udho::hazo::node<detail::labeled<T, typename T::result_type>...> base_type;
    typedef ContextT context_type;
    
    context_type _context;
    
    /**
     * @brief Construct a new collector object
     * 
     * @param ctx 
     * @param name 
     */
    collector(context_type ctx): _context(ctx) {}
    /**
     * @brief Get the context
     * 
     * @return context_type& 
     */
    context_type& context() { return _context; }
    /**
     * @brief Get the context
     * 
     * @return const context_type& 
     */
    const context_type& context() const { return _context; }
};

/**
 * @ingroup data
 */
template <typename U, typename ContextT, typename... T>
collector<ContextT, T...>& operator<<(collector<ContextT, T...>& h, const U& data){
    h.template data<U>() = data;
    return h;
}

/**
 * @ingroup data
 */
template <typename U, typename ContextT, typename... T>
const collector<ContextT, T...>& operator>>(const collector<ContextT, T...>& h, U& data){
    data = h.template data<U>();
    return h;
}

/**
 * @ingroup data
 */
template <typename... T, typename ContextT>
std::shared_ptr<collector<ContextT, T...>> collect(ContextT& ctx){
    typedef collector<ContextT, T...> collector_type;
    return std::make_shared<collector_type>(ctx);
}
    
}
}
#endif // UDHO_ACTIVITIES_COLLECTOR_H
