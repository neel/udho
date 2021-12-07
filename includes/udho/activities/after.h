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

#ifndef UDHO_ACTIVITIES_AFTER_H
#define UDHO_ACTIVITIES_AFTER_H

#include "udho/activities/collector.h"
#include <memory>
#include <udho/activities/subtask.h>
#include <udho/activities/joined.h>
#include <udho/activities/fwd.h>

namespace udho{
namespace activities{
    
#ifndef __DOXYGEN__

namespace detail{
    
    /**
     * @brief specializes how a subtask will be attached with the provided subtask
     * 
     * @tparam SubtaskT 
     * @tparam ActivityT 
     * @tparam DependenciesT 
     */
    template <template <typename, typename...> class SubtaskT, typename ActivityT, typename... DependenciesT>
    struct after<SubtaskT<ActivityT, DependenciesT...>>{
        after(SubtaskT<ActivityT, DependenciesT...>& before): _before(before){}
        
        template <typename OtherActivityT, typename... OtherDependenciesT>
        void attach(SubtaskT<OtherActivityT, OtherDependenciesT...>& sub){
            sub.after(_before);
        }
        private:
            SubtaskT<ActivityT, DependenciesT...>& _before;
    };
    
}

template <template <typename, typename...> class SubtaskT, typename HeadT, typename... TailT>
struct basic_after: private basic_after<SubtaskT, TailT...>{
    basic_after(HeadT& head, TailT&... tail): _head(head), basic_after<SubtaskT, TailT...>(tail...){}
    
    template <typename ActivityT, typename... Args>
    SubtaskT<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...> perform(Args&&... args){
        SubtaskT<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...> sub = SubtaskT<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...>::with(args...);
        attach(sub);
        return sub;
    }
    
    template <typename CallbackT, typename CollectorContainingT, std::enable_if_t<has_collector<CollectorContainingT>::value, bool> = true>
    SubtaskT<joined<CallbackT, typename collector_of<CollectorContainingT>::type>, typename HeadT::activity_type, typename TailT::activity_type...> finish(CollectorContainingT& collector_like, CallbackT callback, bool always = true){
        using collector_type = typename collector_of<CollectorContainingT>::type;
        using joined_type = joined<CallbackT, collector_type>;
        using joined_subtask_type = SubtaskT<joined_type, typename HeadT::activity_type, typename TailT::activity_type...>;
        joined_subtask_type sub = joined_subtask_type::with(collector_from(collector_like), callback);
        sub->always(always);
        attach(sub);
        return sub;
    }

    private:
        detail::after<HeadT> _head;
    protected:
        template <typename ActivityT, typename... DependenciesT>
        void attach(SubtaskT<ActivityT, DependenciesT...>& sub){
            _head.attach(sub);
            basic_after<SubtaskT, TailT...>::attach(sub);
        }
};

template <template <typename, typename...> class SubtaskT, typename HeadT>
struct basic_after<SubtaskT, HeadT>{
    basic_after(HeadT& head): _head(head){}
    
    template <typename ActivityT, typename... Args>
    SubtaskT<ActivityT, typename HeadT::activity_type> perform(Args&&... args){
        SubtaskT<ActivityT, typename HeadT::activity_type> sub = SubtaskT<ActivityT, typename HeadT::activity_type>::with(args...);
        attach(sub);
        return sub;
    }
    
    template <typename CallbackT, typename CollectorContainingT, std::enable_if_t<has_collector<CollectorContainingT>::value, bool> = true>
    SubtaskT<joined<CallbackT, typename collector_of<CollectorContainingT>::type>, typename HeadT::activity_type> finish(CollectorContainingT& collector_like, CallbackT callback, bool always = true){
        using collector_type = typename collector_of<CollectorContainingT>::type;
        using joined_type = joined<CallbackT, collector_type>;
        using joined_subtask_type = SubtaskT<joined_type, typename HeadT::activity_type>;
        joined_subtask_type sub = joined_subtask_type::with(collector_from(collector_like), callback);
        sub->always(always);
        attach(sub);
        return sub;
    }

    private:
        detail::after<HeadT> _head;
    protected:
        template <typename ActivityT, typename... DependenciesT>
        void attach(SubtaskT<ActivityT, DependenciesT...>& sub){
            _head.attach(sub);
        }
};

/**
 * @brief conveniance function to construct subtasks with no dependencies
 */
struct after_none{
    template <typename ActivityT, typename... Args>
    subtask<ActivityT> perform(Args&&... args){
        return subtask<ActivityT>::with(args...);
    }
};

/**
* @brief conveniance function to construct basic_after for activities with dependencies
* 
* @tparam T...
* @param dependencies... 
* @return udho::activities::after<T...> 
*/
template <typename... T>
udho::activities::basic_after<subtask, T...> after(T&... dependencies){
    return udho::activities::basic_after<subtask, T...>(dependencies...);
}
inline after_none after(){ return after_none{}; }

#else

/**
 * @brief convenience function for creating subtasks 
 * 
 * @tparam Dependencies... 
 *
 * creates a subtask that performs the requested activity after all its dependencies are done.
 * @code {.cpp}
 * auto a3 = activities::after(a1, a2).perform<A3>(args...);
 * @endcode
 * In the above example a3 is an instance of `activities::subtask<A3, A1, A2>` which implies
 * the activity A3 depends on A1 and A2. After creating the subtask, `after()` links a3 with
 * all of its dependencies. In brief the `perform()` function mentioned in the above code 
 * performs in the following operations. Tnen it returns `a3`.
 * @code {.cpp}
 * subtask<A3, A1, A2> a3(args...);
 * a3.after(a1);
 * a3.after(a2);
 * @endcode
 * @see activities::perform
 * @ingroup activities
 */
template <typename... Dependencies>
struct basic_after{
    /**
     * @brief Construct a new after object
     * 
     * @param dependencies 
     */
    basic_after(Dependencies&... dependencies);
    /**
     * @brief creates a subtask to perform activity NextActivityT
     * 
     * @tparam NextActivityT 
     * @tparam Args 
     * @param args 
     * @return subtask<NextActivityT, Dependencies...> 
     */
    template <typename NextActivityT, typename... Args>
    subtask<NextActivityT, typename Dependencies::activity_type...> perform(Args&&... args);

    /**
     * @brief Finish an activity chain by associating a callback, which will be invoked with an accessor containing data of compatiable with the collector.
     * Returns a Joined subtask that depends on the dependencies specified. 
     * @not The returned joined subtask cannot be used as a dependency for some other subtasks
     * @tparam CallbackT 
     * @tparam ContextT 
     * @tparam T 
     * @param collector_like shared pointer to the collector or any other object that has a collector
     * @param callback The callback (lambda function) which is to be executed.
     * @param always if set true executes the callback even if the dependencies fails. Otherwise invokes only when the chain is successful.
     * @return SubtaskT<joined<CallbackT, collector<ContextT, T...>>, typename Dependencies::activity_type...> 
     * @see joined
     */
    template <typename CallbackT, typename CollectorContainingT, std::enable_if_t<has_collector<CollectorContainingT>::value, bool> = true>
    SubtaskT<joined<CallbackT, typename collector_of<CollectorContainingT>::type>, typename HeadT::activity_type> finish(CollectorContainingT& collector_like, CallbackT callback, bool always = true){
};

/**
 * @brief conveniance function to construct basic_after for activities with dependencies
 * 
 * @tparam T...
 * @param dependencies... 
 * @return udho::activities::basic_after<T...> 
 * @see udho::activities::basic_after<T...> 
 * @ingroup activities
 */
template <typename... T>
udho::activities::basic_after<subtask, T...> after(T&... dependencies){
    return udho::activities::basic_after<subtask, T...>(dependencies...);
}

/**
 * @brief conveniance function to construct subtasks with no dependencies
 * Similar to @ref activities::perform with no require
 * @see activities::perform
 * @ingroup activities
 */
inline after_none after(){ return after_none{}; }

#endif // __DOXYGEN__
    
}

}

#endif // UDHO_ACTIVITIES_AFTER_H

