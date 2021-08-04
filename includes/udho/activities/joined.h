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

#ifndef UDHO_ACTIVITIES_JOINED_H
#define UDHO_ACTIVITIES_JOINED_H

#include <cstdint>
#include <memory>
#include <udho/activities/data.h>
#include <udho/activities/subtask.h>

namespace udho{
/**
 * \ingroup activities
 */
namespace activities{
    
    template <typename CallbackT, typename CollectorT>
    struct joined;
    
    /**
     * \ingroup activities
     */
    template <typename CallbackT, typename... T, typename ContextT>
    struct joined<CallbackT, activities::collector<ContextT, dataset<T...>>>{
        typedef activities::collector<ContextT, dataset<T...>> collector_type;
        typedef typename collector_type::accessor_type accessor_type;
        typedef CallbackT callback_type;
        typedef joined<callback_type, activities::collector<ContextT, dataset<T...>>> self_type;
        typedef int success_type;
        typedef boost::function<bool (const success_type&)> cancel_if_ftor;
        typedef boost::function<bool (const success_type&)> abort_error_ftor;
        
        joined(std::shared_ptr<collector_type> collector, CallbackT callback): _collector(collector), _callback(callback){}
        void operator()(){
            accessor_type accessor(_collector);
            _callback(accessor);
        }
        void cancel(){
            operator()();
        }
        template <typename U>
        void cancel_if(U&){}
        template <typename U>
        void if_failed(U&){}
        template <typename U>
        void if_errored(U&){}
        template <typename U>
        void if_canceled(U&){}
        private:
            std::shared_ptr<collector_type> _collector;
            callback_type _callback;
    };
    
    namespace detail{
        template <typename CollectorT, typename... DependenciesT>
        struct final_intermediate{
            std::shared_ptr<CollectorT> _collector;
            
            final_intermediate(std::shared_ptr<CollectorT> collector): _collector(collector){}
            
            template <typename CallbackT>
            subtask<joined<CallbackT, CollectorT>, DependenciesT...> exec(CallbackT callback){
                return subtask<joined<CallbackT, CollectorT>, DependenciesT...>::with(_collector, callback);
            }
        };
    }
    
}

}

#endif // UDHO_ACTIVITIES_JOINED_H

