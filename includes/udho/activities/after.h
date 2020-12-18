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

#include <memory>
#include <udho/activities/subtask.h>
#include <udho/activities/joined.h>
#include <udho/activities/fwd.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    namespace detail{
        
        template <typename ActivityT, typename... DependenciesT>
        struct after<subtask<ActivityT, DependenciesT...>>{
            subtask<ActivityT, DependenciesT...>& _before;
            
            after(subtask<ActivityT, DependenciesT...>& before): _before(before){}
            
            template <typename OtherActivityT, typename... OtherDependenciesT>
            void attach(subtask<OtherActivityT, OtherDependenciesT...>& sub){
                sub.after(_before);
            }
        };
        
    }
    
    template <typename HeadT, typename... TailT>
    struct after: detail::after<HeadT>, after<TailT...>{
        after(HeadT& head, TailT&... tail): detail::after<HeadT>(head), after<TailT...>(tail...){}
        
        template <typename ActivityT, typename... Args>
        subtask<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...> perform(Args&&... args){
            subtask<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...> sub = subtask<ActivityT, typename HeadT::activity_type, typename TailT::activity_type...>::with(args...);
            attach(sub);
            return sub;
        }
        
        template <typename ActivityT, typename... DependenciesT>
        void attach(subtask<ActivityT, DependenciesT...>& sub){
            detail::after<HeadT>::attach(sub);
            after<TailT...>::attach(sub);
        }
        
        template <typename CallbackT, typename StartT>
        subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type, typename TailT::activity_type...> finish(StartT& starter, CallbackT callback){
            subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type, typename TailT::activity_type...> sub = subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type, typename TailT::activity_type...>::with(starter.collector(), callback);
            attach(sub);
            return sub;
        }
    };
    
    template <typename HeadT>
    struct after<HeadT>: detail::after<HeadT>{
        after(HeadT& head): detail::after<HeadT>(head){}
        
        template <typename ActivityT, typename... Args>
        subtask<ActivityT, typename HeadT::activity_type> perform(Args&&... args){
            subtask<ActivityT, typename HeadT::activity_type> sub = subtask<ActivityT, typename HeadT::activity_type>::with(args...);
            attach(sub);
            return sub;
        }
        
        template <typename ActivityT, typename... DependenciesT>
        void attach(subtask<ActivityT, DependenciesT...>& sub){
            detail::after<HeadT>::attach(sub);
        }
        
        template <typename CallbackT, typename StartT>
        subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type> finish(StartT& starter, CallbackT callback){
            subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type> sub = subtask<joined<CallbackT, typename StartT::collector_type>, typename HeadT::activity_type>::with(starter.collector(), callback);
            attach(sub);
            return sub;
        }
    };
    
    struct after_none{
        template <typename ActivityT, typename... Args>
        subtask<ActivityT> perform(Args&&... args){
            return subtask<ActivityT>::with(args...);
        }
    };
    
}

}

#endif // UDHO_ACTIVITIES_AFTER_H

