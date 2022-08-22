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
namespace activities{

/**
 * @brief Given an instance of CollectorLikeT that contains a collector inside returns the type of collector contained inside and extracts the collector
 * @tparam CollectorLikeT 
 * @ingroup activities
 */
template <typename CollectorLikeT>
struct collector_of{
    using type = void;
    static void apply(){}
};

template <typename ContextT, typename... T>
struct collector_of<std::shared_ptr<collector<ContextT, T...>>>{
    using type = collector<ContextT, T...>;
    static std::shared_ptr<type> apply(std::shared_ptr<collector<ContextT, T...>>& collector){ return collector; }
};

/**
 * @brief Checks whether the given type X has a collector inside (e.g. a collector can be retrieved from and instance of X)
 * 
 * @tparam X 
 * @ingroup activities
 * @see collector_of
 */
template <typename X>
using has_collector = std::integral_constant<bool, !std::is_void<typename collector_of<X>::type>::value>;

/**
 * @brief Retrieve the shared_ptr to the collector from an instance of X that is expected to contain a collector inside
 * @see collector_of
 * @tparam X 
 * @param x 
 * @return std::shared_ptr<collector_of<X>::type>
 * @ingroup activities
 */
template <typename X, std::enable_if_t<has_collector<X>::value, bool> = true>
typename std::shared_ptr<typename collector_of<X>::type> collector_from(X& x){
    return collector_of<X>::apply(x);
}


/**
 * @brief Collects data associated with all activities involved in the subtask graph.
 * @tparam ContextT 
 * @tparam Activities ... Activities in the chains
 *
 * Multiple activities may have different semantics to denote their success and failure. An 
 * activity `X` is defined by subclassing from @ref udho::activities::activity "activity<X, S, F>"
 * where `S` and `F` are two data structures containing information related to the successful
 * and failed invocation of the activity. An asynchronous subtask graph involving multiple such
 * activities, will require storage and retrieval facilities for all the success and failure 
 * data structures. Hence collector provides a heterogenous storage that is accessed by all the
 * activities in the subtask graph.
 * 
 * Collector are usually accessed through an \ref udho::activities::accessor "accessor" which 
 * provides read write access to the collector. A partial accessor may provide access to a subset
 * of data collected by the collector. Whereas a full accessor provides full access. The final
 * callback which concludes the invocation of chains of asynchronous activities reads all data
 * collected by the collector though a full accessor.
 *
 * @par Internal Details
 *       A collector uses @ref udho::activities::result_data "result_data<S, F>" to store 
 *       the activity result. To allow multiple activities to have the same success and failure 
 *       type it labels each such `result_data` with the activity type. The `result_type` type 
 *       in the base class @ref udho::activities::activity "activity" is actually a typedef of 
 *       `result_data<S, F>` where `S` and `F` are the success and failure data type of the activity. 
 *       The labeling is done by internal `detail::labeled` template as shown below. 
 *       @code 
 *       detail::labeled<X, udho::activities::result_data<S, F> >
 *       @endcode 
 *       So, instead of storing the `result_data` the collector stores an labeled `result_data` 
 *       internally.
 * 
 * To execute asynchronous activities `A1`, `A2` and `A3` a collector is created using the 
 * \ref udho::activities::collect "collect" method as shown below.
 * @code 
 * auto collector = udho::activities::collect<A1, A2, A3>(ctx);
 * @endcode 
 * The collect method instantiates a shared pointer to `udho::activities::collector<ContextT, A1, A2, A3>` 
 * where `ContextT` is the type of ctx. While creating subtasks each of these activities create a partial 
 * accessor to access the slice of data required for that activity. 
 *
 * @note Collector extends the lifetime of HTTP context by copying the context object. 
 * @ingroup activities
 */
template <typename ContextT, typename... Activities>
struct collector: std::enable_shared_from_this<collector<ContextT, Activities...>>, private udho::hazo::node<detail::labeled<Activities, typename Activities::result_type>...>{
#ifndef __DOXYGEN__
    typedef udho::hazo::node<detail::labeled<Activities, typename Activities::result_type>...> base_type;
    typedef ContextT context_type;

    context_type _context;
#endif 
    template <typename... X>
    friend struct accessor;
    
    friend struct accessor_of<collector<ContextT, Activities...>>;

    template <typename U>
    friend collector<ContextT, Activities...>& operator<<(collector<ContextT, Activities...>& h, const U& data){
        h.node().template data<U>() = data;
        return h;
    }
    template <typename U>
    friend const collector<ContextT, Activities...>& operator>>(const collector<ContextT, Activities...>& h, U& data){
        data = h.node().template data<U>();
        return h;
    }
    
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

#ifndef __DOXYGEN__
    private:
        base_type& node() { return *this; }
        const base_type& node() const { return *this; }
#endif 
};

/**
 * @brief conveniance function to construct a collector object. Returns a shared pointer to a collector
 * Assuming there are three activities A1, A2, and A3 a collector can be constructed as shown in the example below.
 * @code {.cpp}, 
 * auto collector = activities::collect<A1, A2, A3>(ctx);
 * @endcode
 * @ingroup activities
 */
template <typename... T, typename ContextT>
std::shared_ptr<collector<ContextT, T...>> collect(ContextT& ctx){
    typedef collector<ContextT, T...> collector_type;
    return std::make_shared<collector_type>(ctx);
}
    
}
}
#endif // UDHO_ACTIVITIES_COLLECTOR_H
